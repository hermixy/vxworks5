/* smNameShow.c - shared memory objects name database show routines (VxMP Option) */

/* Copyright 1984-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01i,03may02,mas  fix compiler warnings from volatile pointers (SPR 68334)
01h,24oct01,mas  doc update (SPR 71149)
01g,14feb93,jdi  documentation cleanup for 5.1.
01f,29jan93,pme  added little endian support. Changed name copy to use strcpy().
01e,21nov92,jdi  documentation cleanup.
01d,13nov92,dnw  added include of taskLib.h
01c,02sep92,pme  added SPARC support. documentation cleanup.
01b,29sep92,pme  changed objId to value. cleanup.
01a,19jul92,pme  added level to smNameShow.
                 written.
*/

/*
DESCRIPTION
This library provides a routine to show the contents of the shared memory
objects name database.  The shared memory objects name database facility is
provided by the smNameLib module.

CONFIGURATION
The routines in this library are included by default if the component
INCLUDE_SM_OBJ is included.

AVAILABILITY
This module is distributed as a component of the unbundled shared memory
objects support option, VxMP.

INCLUDE FILES: smNameLib.h

SEE ALSO: smNameLib, smObjLib,
\tb VxWorks Programmer's Guide: Shared Memory Objects
*/

#include "vxWorks.h"
#include "errno.h"
#include "smDllLib.h"
#include "smObjLib.h"
#include "smMemLib.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "taskLib.h"
#include "netinet/in.h"
#include "private/smNameLibP.h"
#include "private/semSmLibP.h"

/* globals */

char * smTypeString[MAX_DEF_TYPE] = 
    {
    "SM_SEM_B", "SM_SEM_C", "SM_MSG_Q", "SM_PART_ID", "SM_BLOCK"
    };


/******************************************************************************
*
* smNameShowInit - initialize shared name databse show routine
*
* This routine links the shared memory objects show routine into the VxWorks
* system.  These routines are included automatically by defining
* \%INCLUDE_SHOW_ROUTINES in configAll.h.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void smNameShowInit (void)
    {
    }

/******************************************************************************
*
* smNameInfoGet - support routine for smNameShow
*
* This routine fills a table containing names, values and types stored in the 
* shared name database.
*
* WARNING
* This routine locks access to the shared name database while
* getting its contents.  This can compromise the access time to
* the name database from other CPU in the system.  Generally this routine is
* used for debugging purposes only.
*
* RETURNS: OK or ERROR if name facility is not initialized 
*
* ERRNO: 
*  S_smObjLib_LOCK_TIMEOUT
*
* SEE ALSO: smNameLib
*
* NOMANUAL
*/


LOCAL STATUS smNameInfoGet
    (
    SM_OBJ_NAME_INFO nameList[], /* list of names extracted from database */
    int *            pCurNumName,/* current number of names in data base */
    int              maxNames 	 /* max names nameList[] can accomodate */
    )
    {
    SM_OBJ_NAME    * smName;
    int              ix;

    TASK_SAFE ();
    if (semSmTake ((SM_SEM_ID)&pSmNameDb->sem, WAIT_FOREVER) != OK)
	{				/* get exclusive access */
	TASK_UNSAFE ();
	return (ERROR);	
	}

    smName = (SM_OBJ_NAME *) SM_DL_FIRST (&pSmNameDb->nameList);
    ix = 0;

    *pCurNumName = ntohl (pSmNameDb->curNumName);

    /* fill the table with names, value and type */

    while ((smName != LOC_NULL) && (ix <= maxNames)) 
	{
        nameList[ix].value = (void *) ntohl ((int) smName->value); 
        nameList[ix].type  = ntohl (smName->type); 
        strcpy ((char *) &nameList[ix].name, (char *) &smName->name); 

	ix ++;				/* incr name counter */

	/* get next name in database */

	smName = (SM_OBJ_NAME *) SM_DL_NEXT (smName);
	}

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
* smNameShow - show the contents of the shared memory objects name database (VxMP Option)
*
* This routine displays the names, values, and types of objects stored 
* in the shared memory objects name database.  Predefined types
* are shown, using their ASCII representations; all other types are printed
* in hexadecimal.
*
* The <level> parameter defines the level of database information
* displayed.  If <level> is 0, only statistics on the database contents are
* displayed.  If <level> is greater than 0, then both statistics and
* database contents are displayed.
*
* WARNING
* This routine locks access to the shared memory objects name database while
* displaying its contents.  This can compromise the access time to the name
* database from other CPUs in the system.  Generally, this routine is used
* for debugging purposes only.
*
* EXAMPLE:
* \cs
* -> smNameShow
*
* Names in Database  Max : 30  Current : 6  Free : 24
*
* -> smNameShow 1
*
* Names in Database  Max : 30  Current : 6  Free : 24
*
* Name                Value         Type
* ---------------- ----------- -------------
* inputImage        0x802340    SM_MEM_BLOCK
* ouputImage        0x806340    SM_MEM_BLOCK
* imagePool         0x802001    SM_MEM_PART
* imageInSem        0x8e0001    SM_SEM_B
* imageOutSem       0x8e0101    SM_SEM_C
* actionQ           0x8e0201    SM_MSG_Q
* userObject        0x8e0400    0x1b0
* \ce
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled shared memory
* objects support option, VxMP.
* 
* RETURNS: OK, or ERROR if the name facility is not initialized.
*
* ERRNO: 
*  S_smNameLib_NOT_INITIALIZED 
*  S_smObjLib_LOCK_TIMEOUT
*
* SEE ALSO: smNameLib
*/

STATUS smNameShow 
    (
    int level		/* information level */
    )
    {
    SM_OBJ_NAME_INFO * nameInfo;
    int                curNumName; 	/* current # of names in database */
    int                ix;

    if (pSmNameDb == NULL)		/* name facility initialized ? */
	{
	errno = S_smNameLib_NOT_INITIALIZED;
	return (ERROR);
	}
    
    /* allocate name info table */

    nameInfo = (SM_OBJ_NAME_INFO *) 
		malloc (sizeof(SM_OBJ_NAME_INFO) * ntohl (pSmNameDb->maxName));
    if (nameInfo == NULL)
        {
	return (ERROR);
        }

    /* fill name info table */

    if (smNameInfoGet (nameInfo, &curNumName, ntohl (pSmNameDb->maxName)) !=OK)
	{
	free (nameInfo);
	return (ERROR);
	}

    /* print header lines */

    printf ("\nName in Database  Max : %d  Current : %d  Free : %d\n\n",
	       ntohl (pSmNameDb->maxName),
	       curNumName,
	       ntohl (pSmNameDb->maxName) - curNumName);

    /* print database contents if requested */

    if (level > 0)
	{
    	printf ("Name                   Value        Type\n");
    	printf ("------------------- ------------ ------------\n");

    	/* print each name, value and type */

    	for (ix=0; ix < curNumName; ix++)
	    {
            printf ("%-19s  %#10x   ", nameInfo[ix].name, 
		    (int) nameInfo[ix].value);

            if (nameInfo[ix].type < MAX_DEF_TYPE)
                {
           	printf ("%-12s\n", smTypeString[nameInfo[ix].type]);
                }

       	    else
       	        {
           	printf ("%-#10x\n", nameInfo[ix].type);
       	        }
	    }
	}
    
    free (nameInfo);			/* free nameInfo table */

    return (OK);
    }

