/* smNameLib.c - shared memory objects name database library (VxMP Option) */

/* Copyright 1984-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01k,03may02,mas  cache flush and volatile fix (SPR 68334); bridge flush fix
		 (SPR 68844)
01j,24oct01,mas  fixed gnu warnings (SPR 71113) and diab warnings (SPR 71120);
		 doc update (SPR 71149)
01i,30mar99,jdi  doc: fixed bad table style that refgen can't cope with.
01h,14mar99,jdi  doc: removed refs to config.h and/or configAll.h (SPR 25663).
01g,12mar99,p_m  Fixed SPR 20159 by adding missing parameters in examples.
01f,14feb93,jdi  documentation cleanup for 5.1.
01e,29jan93,pme  added little endian support.
		 modified name facility initialized test.
		 changed name copying to use strcpy().
01d,21nov92,jdi  documentation cleanup.
01c,02oct92,pme  added SPARC support. documentation cleanup.
01b,29sep92,pme  changed function names for coherency. 
		 cleanup.
                 changed previous release number to 01a.
01a,19jul92,pme  moved smNameInfoGet() and smNameShow to smNameShow.c
                 code review cleanup.
                 added smNameInfoGet ().
                 written.
*/

/*
DESCRIPTION
This library provides facilities for managing the shared memory objects
name database.  The shared memory objects name database associates a name
and object type with a value and makes that information available to all 
CPUs.  A name is an arbitrary, null-terminated string.  An object 
type is a small integer, and its value is a global (shared) ID or
a global shared memory address.

Names are added to the shared memory name database with smNameAdd().  They
are removed by smNameRemove().

Objects in the database can be accessed by either name or value.  The
routine smNameFind() searches the shared memory name database for an
object of a specified name.  The routine smNameFindByValue() searches the
shared memory name database for an object of a specified identifier or
address.

Name database contents can be viewed using smNameShow().

The maximum number of names to be entered in the database is defined in the
configuration parameter SM_OBJ_MAX_NAME .  This  value is used to determine
the size of a dedicated shared memory partition from which name database
fields are allocated. 

The estimated memory size required for the name database can be calculated as 
follows:
 
\cs
    name database pool size = SM_OBJ_MAX_NAME * 40 (bytes)
\ce

The display facility for the shared memory objects name database is provided by
the smNameShow module.

EXAMPLE
The following code fragment allows a task on one CPU to enter the
name, associated ID, and type of a created shared semaphore into
the name database.  Note that CPU numbers can belong to any CPU using the
shared memory objects facility.

On CPU 1 :
\cs
    #include "vxWorks.h"
    #include "semLib.h"
    #include "smNameLib.h"
    #include "semSmLib.h"
    #include "stdio.h"

    testSmSem1 (void)
        {
        SEM_ID smSemId;

	/@ create a shared semaphore @/

        if ((smSemId = semBSmCreate(SEM_Q_FIFO, SEM_EMPTY)) == NULL)
            {
            printf ("Shared semaphore creation error.");
            return (ERROR);
            }

        /@ 
         * make created semaphore Id available to all CPUs in 
         * the system by entering its name in shared name database.
         @/

        if (smNameAdd ("smSem", smSemId, T_SM_SEM_B) != OK )
            {
            printf ("Cannot add smSem into shared database.");
            return (ERROR);
            }
	    ...

        /@ now use the semaphore @/

        semGive (smSemId);
	    ...	
        }
\ce
On CPU 2 :
\cs
    #include "vxWorks.h"
    #include "semLib.h"
    #include "smNameLib.h"
    #include "stdio.h"

    testSmSem2 (void)
        {
        SEM_ID smSemId;
	int    objType;		/@ place holder for smNameFind() object type @/

        /@ get semaphore ID from name database @/ 
    
        smNameFind ("smSem", (void **) &smSemId, &objType, WAIT_FOREVER);
	    ...
        /@ now that we have the shared semaphore ID, take it @/
    
        semTake (smSemId, WAIT_FOREVER);
	    ...
        }
\ce

INTERNAL
The data base is protected against concurrent access by a shared binary
semaphore.  Each routine in this library provides task deletion safety
while holding the database semaphore to prevent database access lock.

Shared name database is based on a doubly linked list created during
shared memory objects initialization.  A doubly linked list is used to allow
future extensions like alphabeticaly ordered database and global management
speed up.  Each database field requires 44 bytes and is allocated from the
shared memory name database pool using smMemPartAlloc().  Names are added in
FIFO order to the list.  The search for a name or a value is done by scanning
the list from first to last node.

CONFIGURATION
Before routines in this library can be called, the shared memory object
facility must be initialized by calling usrSmObjInit().
This is done automatically during VxWorks initialization when the component
INCLUDE_SM_OBJ is included.

AVAILABILITY
This module is distributed as a component of the unbundled shared memory
objects support option, VxMP.

INCLUDE FILES: smNameLib.h

SEE ALSO: smNameShow, smObjLib, smObjShow, usrSmObjInit(),
\tb VxWorks Programmer's Guide: Shared Memory Objects
*/

#include "vxWorks.h"
#include "errno.h"
#include "semLib.h"
#include "string.h"
#include "taskLib.h"
#include "cacheLib.h"
#include "smDllLib.h"
#include "smObjLib.h"
#include "smMemLib.h"
#include "netinet/in.h"
#include "private/smNameLibP.h"
#include "private/smMemLibP.h"

/* forward declarations */

LOCAL STATUS smNameFindOnce (char * name, void ** pValue, int * pType);

/* globals */

SM_OBJ_NAME_DB volatile * pSmNameDb; /* pointer to name database header */


/******************************************************************************
*
* smNameLibInit - dummy function to drag in smNameLib
*
* This routine is required to reference the shared name library 
* during linking.
*
* NOMANUAL
*/

void smNameLibInit(void)
    {
    }

/******************************************************************************
*
* smNameInit - initialize the shared memory objects name database
*
* This routine initializes the shared memory objects name facility by filling
* reserved fields in the shared memory header. 
*
* It is called during shared memory objects setup if the configuration
* macro INCLUDE_SM_OBJ is defined.
*
* RETURNS: OK, or ERROR if database cannot be initialized.
*
* ERRNO:
*  S_smObjLib_NOT_INITIALIZED
*
* SEE ALSO: smNameLib
*
* NOMANUAL
*/

STATUS smNameInit 
    (
    int maxNames			/* max # of names in database */
    )
    {
    if ((pSmObjHdr == NULL) || !(pSmObjHdr->initDone))	/* smObj initialized?*/
	{
	errno = S_smObjLib_NOT_INITIALIZED;
	return (ERROR);
	}

    pSmNameDb = &pSmObjHdr->nameDtb;	/* get name database header */ 

    /* initialize name data base shared semaphore */

    semSmBInit ((SM_SEM_ID)&pSmNameDb->sem, SEM_Q_FIFO, SEM_FULL);

    smDllInit ((SM_DL_LIST *)&pSmNameDb->nameList); /* initialise name list */

    pSmNameDb->maxName    = htonl (maxNames);	 /* fill header */
    pSmNameDb->curNumName = 0;

    pSmNameDb->initDone   = htonl (TRUE);	 /* name facility initialized*/

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    maxNames = pSmNameDb->maxName;		/* BRIDGE FLUSH  [SPR 68334] */

    return (OK);
    }

/******************************************************************************
*
* smNameAdd - add a name to the shared memory name database (VxMP Option)
*
* This routine adds a name of specified object type and value to the shared 
* memory objects name database.
*
* The <name> parameter is an arbitrary null-terminated string with a 
* maximum of 20 characters, including EOS.
*
* By convention, <type> values of less than 0x1000 are reserved by VxWorks;
* all other values are user definable.  The following types are predefined
* in smNameLib.h :
*
* \ts
* Name         | Value | Type
* -------------+-------+-------------------------------
* T_SM_SEM_B   | =  0  | shared binary semaphore
* T_SM_SEM_C   | =  1  | shared counting semaphore 
* T_SM_MSG_Q   | =  2  | shared message queue 
* T_SM_PART_ID | =  3  | shared memory Partition 
* T_SM_BLOCK   | =  4  | shared memory allocated block 
* \te
*
* A name can be entered only once in the database, but there can be more
* than one name associated with an object ID. 
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled shared memory
* objects support option, VxMP.
* 
* RETURNS: OK, or ERROR if there is insufficient memory for <name> to be 
* allocated, if <name> is already in the database, or if the database is 
* already full.
*
* ERRNO: 
*  S_smNameLib_NOT_INITIALIZED  
*  S_smNameLib_NAME_TOO_LONG  
*  S_smNameLib_NAME_ALREADY_EXIST 
*  S_smNameLib_DATABASE_FULL 
*  S_smObjLib_LOCK_TIMEOUT
*
* SEE ALSO: smNameShow
*
* INTERNAL
* Database fields are allocated from a dedicated shared memory partition
* called smNamePartId.
*/

STATUS smNameAdd
    (
    char *	name,		/* name string to enter in database */
    void *	value,		/* value associated with name */
    int		type		/* type associated with name */
    )
    {
    SM_OBJ_NAME volatile * smName;	/* name database field pointer */
    void *		   dummyValue;	/* dummy value to call smNameFindOnce*/
    int			   dummyType;	/* dummy type to call smNameFindOnce */
    int                    tmp;         /* temp storage */

    if (pSmNameDb == NULL)		/* name facility initialized ? */
	{
	errno = S_smNameLib_NOT_INITIALIZED;
	return (ERROR);
	}

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    tmp = pSmNameDb->maxName;			/* PCI bug       [SPR 68844] */

    if (pSmNameDb->maxName == pSmNameDb->curNumName) 	/* database full ? */
	{
	errno = S_smNameLib_DATABASE_FULL;
	return (ERROR);
	}

    if (strlen (name) > MAX_NAME_LENGTH)	/* name too long */
	{
	errno = S_smNameLib_NAME_TOO_LONG;
	return (ERROR);
	}

    /* check if name already in database */
    
    if (smNameFindOnce (name, &dummyValue, &dummyType) == OK)
        {
	errno = S_smNameLib_NAME_ALREADY_EXIST;
	return (ERROR);
	}

    /* get exclusive access to DB */

    TASK_SAFE ();
    if (semSmTake ((SM_SEM_ID)&pSmNameDb->sem, WAIT_FOREVER) != OK)
	{
	TASK_UNSAFE ();
	return (ERROR);	
	}

    /* allocate space for name in dedicated shared memory partition */

    smName = (SM_OBJ_NAME volatile *) smMemPartAlloc ((SM_PART_ID)smNamePartId,
						      sizeof(SM_OBJ_NAME));
    if (smName == NULL)
	{
    	semSmGive ((SM_SEM_ID)&pSmNameDb->sem); 	/* release access */
	TASK_UNSAFE ();
	return (ERROR);
	}

    bzero ((char *) smName, sizeof(SM_OBJ_NAME));	/* clear element */
    smName->value = (void *) htonl ((int) value);	/* fill element */
    strcpy ((char *) smName->name, name);
    smName->type = htonl (type);

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    dummyType = smName->type;			/* BRIDGE FLUSH  [SPR 68334] */

    smDllAdd ((SM_DL_LIST *) &pSmNameDb->nameList, (SM_DL_NODE *) smName);
						/* add element to list */
   
    						/* update element number */
    pSmNameDb->curNumName = htonl (ntohl (pSmNameDb->curNumName) + 1);
    						/* update shared infos data */
    pSmObjHdr->curNumName = htonl (ntohl (pSmObjHdr->curNumName) + 1);

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    dummyType = smName->type;			/* BRIDGE FLUSH  [SPR 68334] */

    if (semSmGive ((SM_SEM_ID)&pSmNameDb->sem) != OK) 	/* release access */
	{
	TASK_UNSAFE ();
	return (ERROR);
	}

    TASK_UNSAFE ();
    return (OK);
    }

/******************************************************************************
*
* smNameFindOnce - look up a shared symbol by name one time
*
* This routine searches the shared name database for an object matching a
* specified name <name>.  If the object is found, its value and type are copied
* to <pValue> and <pType>. 
*
* RETURNS: OK, or ERROR if the object is not found.
*
* ERRNO: 
*  S_smNameLib_NAME_NOT_FOUND 
*  S_smObjLib_LOCK_TIMEOUT
*
* NOMANUAL
*/

LOCAL STATUS smNameFindOnce 
    (
    char *  name,		/* name to search for */ 
    void ** pValue,		/* pointer where to return value */
    int  *  pType		/* pointer where to return object type */
    )
    {
    SM_OBJ_NAME volatile * smName;	/* name database field pointer */

    TASK_SAFE ();
    if (semSmTake ((SM_SEM_ID)&pSmNameDb->sem, WAIT_FOREVER) != OK)
	{				/* get exclusive access */
	TASK_UNSAFE ();
	return (ERROR);	
	}

    /* get first name in database */

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    smName = (SM_OBJ_NAME volatile *) SM_DL_FIRST (&pSmNameDb->nameList);

    /* scan name list until name is found or end of list is reached */

    while (smName != LOC_NULL) 
	{
	if (strcmp((char *)smName->name, name) == 0)	/* name found */
	   {
	   *pValue = (void *) ntohl ((int) smName->value);/* return value */
	   *pType  = ntohl (smName->type);	/* return object type */

    	   if (semSmGive ((SM_SEM_ID)&pSmNameDb->sem) != OK)
       		{				/* release access */
		TASK_UNSAFE ();
		return (ERROR);
		}

	   TASK_UNSAFE ();
	   return (OK);
	   }

        /* get next name in list */	

        CACHE_PIPE_FLUSH ();                    /* CACHE FLUSH   [SPR 68334] */
	smName = (SM_OBJ_NAME volatile *) SM_DL_NEXT (smName);
	}

    if (semSmGive ((SM_SEM_ID)&pSmNameDb->sem) != OK) 	/* release access */
	{
	TASK_UNSAFE ();
	return (ERROR);
	}

    TASK_UNSAFE ();

    errno = S_smNameLib_NAME_NOT_FOUND;
    return (ERROR);
    }

/******************************************************************************
*
* smNameFind - look up a shared memory object by name (VxMP Option)
*
* This routine searches the shared memory objects name database for an object
* matching a specified <name>.  If the object is found, its value and type
* are copied to the addresses pointed to by <pValue> and <pType>.  The value of 
* <waitType> can be one of the following:
* \is
* \i `NO_WAIT (0)'
* The call returns immediately, even if <name> is not in the database.
* \i `WAIT_FOREVER (-1)'
* The call returns only when <name> is available in the database.  If <name>
* is not already in, the database is scanned periodically as the routine
* waits for <name> to be entered.
* \ie
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled shared memory
* objects support option, VxMP.
* 
* RETURNS: OK, or ERROR if the object is not found, if <name> is too long, or
* the wait type is invalid.
*
* ERRNO: 
*  S_smNameLib_NOT_INITIALIZED  
*  S_smNameLib_NAME_TOO_LONG 
*  S_smNameLib_NAME_NOT_FOUND 
*  S_smNameLib_INVALID_WAIT_TYPE 
*  S_smObjLib_LOCK_TIMEOUT
*
* SEE ALSO: smNameShow
*/

STATUS smNameFind 
    (
    char *	name,		/* name to search for */ 
    void **	pValue,		/* pointer where to return value */
    int  *	pType,		/* pointer where to return object type */
    int		waitType	/* NO_WAIT or WAIT_FOREVER */
    )
    {
    BOOL        found;

    if (pSmNameDb == NULL)		/* name facility initialized ? */
        {
        errno = S_smNameLib_NOT_INITIALIZED;
        return (ERROR);
        }

    if (strlen (name) > MAX_NAME_LENGTH)	/* name too long */
        {
        errno = S_smNameLib_NAME_TOO_LONG;
        return (ERROR);
        }
 
    /* 
     * Now if waitType is NO_WAIT, look for name in database once
     * an return OK if name is found or ERROR if not found.
     * if waitType is WAIT_FOREVER, do a loop until name is entered
     * in the database by another task.
     * In order to avoid CPU and BUS over use we use taskDelay (1) to
     * delay and reschedule between each loop.
     */

    switch (waitType)
	{
	case NO_WAIT : 
	    {
	    return (smNameFindOnce (name, pValue, pType));
	    }

	case WAIT_FOREVER :
	    {
    	    do
       	        {
                found = smNameFindOnce (name, pValue, pType);
                                            /* look for name in database */

                /* ERROR not because name is not in database */

	        if ((found == ERROR) && (errno != S_smNameLib_NAME_NOT_FOUND)) 
		    {
		    return (ERROR);
		    }

                taskDelay (1);              /* force reschedule */
                } while (found != OK);
            return (found);    
	    }

	default : 
	    {
	    errno = S_smNameLib_INVALID_WAIT_TYPE;
	    return (ERROR);
            }
	}
    }

/******************************************************************************
*
* smNameFindByValueOnce - look up a shared symbol by value one time
*
* This routine searches the shared name database for an object matching a
* specified identifier.  If the object is found, its name and type are copied
* to <name> and <pType>. 
*
* RETURNS: OK, or ERROR if the object is not found.
*
* ERRNO: 
*  S_smNameLib_VALUE_NOT_FOUND  
*  S_smObjLib_LOCK_TIMEOUT
*
* NOMANUAL
*/

LOCAL STATUS smNameFindByValueOnce 
    (
    void *	value,		/* value to search for */
    char *	name,		/* pointer where to return name */ 
    int  *	pType		/* pointer where to return object type */
    )
    {
    SM_OBJ_NAME volatile * smName;

    TASK_SAFE ();
    if (semSmTake ((SM_SEM_ID)&pSmNameDb->sem, WAIT_FOREVER) != OK)
	{				/* get exclusive access */
	TASK_UNSAFE ();
	return (ERROR);	
	}

    /* get first name in list */

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    smName = (SM_OBJ_NAME volatile *) SM_DL_FIRST (&pSmNameDb->nameList);

    /* scan name list until id is found or end of list reached */

    while (smName != LOC_NULL) 
	{
	if ((void *) ntohl ((int) smName->value) == value)/* value found */
	   {
	   /* copy name and type */

	   strcpy (name, (char *) smName->name);
	   *pType = ntohl (smName->type);

    	   if (semSmGive ((SM_SEM_ID)&pSmNameDb->sem) != OK) 
       		{					/* release access */
		TASK_UNSAFE ();
		return (ERROR);
		}

	   TASK_UNSAFE ();
	   return (OK);
	   }

	/* get next name in list */

        CACHE_PIPE_FLUSH ();                    /* CACHE FLUSH   [SPR 68334] */
	smName = (SM_OBJ_NAME volatile *) SM_DL_NEXT (smName);
	}

    if (semSmGive ((SM_SEM_ID)&pSmNameDb->sem) != OK) 	/* release access */
	{
	TASK_UNSAFE ();
	return (ERROR);
	}

    TASK_UNSAFE ();
    errno = S_smNameLib_VALUE_NOT_FOUND;

    return (ERROR);
    }

/******************************************************************************
*
* smNameFindByValue - look up a shared memory object by value (VxMP Option)
*
* This routine searches the shared memory name database for an object matching
* a specified value.  If the object is found, its name and type are copied
* to the addresses pointed to by <name> and <pType>.  The value of <waitType> 
* can be one of the following:
*
* \is
* \i `NO_WAIT (0)'
* The call returns immediately, even if the object value is not in the database.
* \i `WAIT_FOREVER (-1)'
* The call returns only when the object value is available in the database.
* \ie
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled shared memory
* objects support option, VxMP.
* 
* RETURNS: OK, or ERROR if <value> is not found or if the wait type is invalid.
*
* ERRNO: 
*  S_smNameLib_NOT_INITIALIZED 
*  S_smNameLib_VALUE_NOT_FOUND 
*  S_smNameLib_INVALID_WAIT_TYPE  
*  S_smObjLib_LOCK_TIMEOUT
*
* SEE ALSO: smNameShow
*/

STATUS smNameFindByValue 
    (
    void *	value,		/* value to search for */
    char *	name,		/* pointer where to return name */ 
    int  *	pType,		/* pointer where to return object type */
    int		waitType	/* NO_WAIT or WAIT_FOREVER */
    )
    {
    BOOL	found;

    if (pSmNameDb == NULL)		/* name facility initialized ? */
	{
	errno = S_smNameLib_NOT_INITIALIZED;
	return (ERROR);
	}

    /* 
     * Now if waitType is NO_WAIT, look for id in database once
     * an return OK if id is found or ERROR if not found.
     * if waitType is WAIT_FOREVER, do a loop until id is entered
     * in the database by another task.
     * In order to avoid CPU and BUS over use we use taskDelay (1) to
     * delay and reschedule between each loop.
     */

    switch (waitType)
	{
	case NO_WAIT : 
	    {
	    return (smNameFindByValueOnce (value, name, pType));
	    }

	case WAIT_FOREVER :
	    {
    	    do
       	        {
                found = smNameFindByValueOnce (value, name, pType);
                                            /* look for name in database */

                /* ERROR not because id is not in database */

	        if ((found == ERROR) && (errno != S_smNameLib_VALUE_NOT_FOUND)) 
		    {
		    return (ERROR);
		    }

                taskDelay (1);              /* force reschedule */

                }while (found != OK);

            return (found);    
	    }

	default : 
	    {
	    errno = S_smNameLib_INVALID_WAIT_TYPE;
	    return (ERROR);
            }
	}
    }

/******************************************************************************
*
* smNameRemove - remove an object from the shared memory objects name database (VxMP Option)
*
* This routine removes an object called <name> from the shared memory objects
* name database. 
* 
* AVAILABILITY
* This routine is distributed as a component of the unbundled shared memory
* objects support option, VxMP.
* 
* RETURNS: OK, or ERROR if the object name is not in the database or if
* <name> is too long.
*
* ERRNO: 
*  S_smNameLib_NOT_INITIALIZED  
*  S_smNameLib_NAME_TOO_LONG 
*  S_smNameLib_NAME_NOT_FOUND 
*  S_smObjLib_LOCK_TIMEOUT
*
* SEE ALSO: smNameShow
*/

STATUS smNameRemove
    (
    char *	name	/* name of object to remove */
    )
    {
    SM_OBJ_NAME volatile * smName;
    int			   temp;	/* value for bus bridge flush */

    if (pSmNameDb == NULL)		/* name facility initialized ? */
	{
	errno = S_smNameLib_NOT_INITIALIZED;
	return (ERROR);
	}

    if (strlen (name) > MAX_NAME_LENGTH)	/* name too long */
	{
	errno = S_smNameLib_NAME_TOO_LONG;
	return (ERROR);
	}

    TASK_SAFE ();				/* get exclusive access */
    if (semSmTake ((SM_SEM_ID)&pSmNameDb->sem, WAIT_FOREVER) != OK)
	{				
	TASK_UNSAFE ();
	return (ERROR);	
	}

    /* get first name in list */

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    smName = (SM_OBJ_NAME volatile *) SM_DL_FIRST (&pSmNameDb->nameList);

    /* loop until name is found or end of list */

    while (smName != LOC_NULL) 
	{
	if (strcmp((char *)smName->name, name) == 0)	/* name found */
	    {
	    smDllRemove ((SM_DL_LIST *)&pSmNameDb->nameList,
	                 (SM_DL_NODE *)smName);
	    smMemPartFree ((SM_PART_ID)smNamePartId, (char *) smName);
    	   					/* update element number */
    	    pSmNameDb->curNumName = htonl (ntohl (pSmNameDb->curNumName) - 1);
           					/* update shared infos data */
            pSmObjHdr->curNumName = htonl (ntohl (pSmObjHdr->curNumName) - 1);

            CACHE_PIPE_FLUSH ();                /* CACHE FLUSH   [SPR 68334] */
            temp = pSmNameDb->curNumName;	/* BRIDGE FLUSH  [SPR 68334] */

    	    if (semSmGive ((SM_SEM_ID)&pSmNameDb->sem) != OK)
       		{				/* release access */
		TASK_UNSAFE ();
		return (ERROR);
		}

	    TASK_UNSAFE ();
	    return (OK);
	    }
	
	/* get next name in list */

        CACHE_PIPE_FLUSH ();                    /* CACHE FLUSH   [SPR 68334] */
	smName = (SM_OBJ_NAME volatile *) SM_DL_NEXT (smName);
	}
    
    if (semSmGive ((SM_SEM_ID)&pSmNameDb->sem) != OK) 	/* release access */
	{
	TASK_UNSAFE ();
	return (ERROR);
	}

    TASK_UNSAFE ();
    errno = S_smNameLib_NAME_NOT_FOUND;

    return (ERROR);
    }

