/* semPxLib.c - semaphore synchronization library (POSIX) */

/* Copyright 1984-2002 Wind River Systems, Inc.  */
#include "copyright_wrs.h"

/*
modification history
--------------------
01l,17jul00,jgn  merge DOT-4 pthreads changes
01k,03feb95,rhp  strengthen warning re semaphore deletion
01j,25jan95,rhp  restructure library man page, other doc tweaks.
    19jan95,jdi  doc cleanup.
01i,08apr94,dvs  changed semClass to semPxClass (SCD #3119).
01h,08apr94,dvs  fixed error in args when calling symFindByName (SCD #3091).
01g,01feb94,dvs  documentation cleanup.
01f,12jan94,kdl	 changed semaphoreInit() to semPxLibInit().
01e,05jan94,kdl	 changed param names to match POSIX spec; changed sem_t
		 "close" field to "refCnt"; general cleanup.
01d,21dec93,kdl	 made sem_destroy() return error if sem has tasks blocked.
01c,13dec93,dvs  added initialization of posix name tbl in semaphoreInit().
	   +rrr  fixed sem_open bug
01b,15nov93,dvs  initial cleanup
01a,06apr93,smb  created
*/

/*
DESCRIPTION:
This library implements the POSIX 1003.1b semaphore interface.  For
alternative semaphore routines designed expressly for VxWorks, see
the manual page for semLib and other semaphore libraries
mentioned there.  POSIX semaphores are counting semaphores; as
such they are most similar to the semCLib VxWorks-specific semaphores.

The main advantage of POSIX semaphores is portability (to the extent
that alternative operating systems also provide these POSIX
interfaces).  However, VxWorks-specific semaphores provide
the following features absent from the semaphores implemented in this
library: priority inheritance, task-deletion safety, the ability for a
single task to take a semaphore multiple times, ownership of
mutual-exclusion semaphores, semaphore timeout, and the choice of
queuing mechanism.

POSIX defines both named and unnamed semaphores; semPxLib includes
separate routines for creating and deleting each kind.  For other
operations, applications use the same routines for both kinds of
semaphore.

TERMINOLOGY
The POSIX standard uses the terms \f2wait\f1 or \f2lock\f1 where
\f2take\f1 is normally used in VxWorks, and the terms \f2post\f1 or
\f2unlock\f1 where \f2give\f1 is normally used in VxWorks.  VxWorks
documentation that is specific to the POSIX interfaces (such as the
remainder of this manual entry, and the manual entries for subroutines
in this library) uses the POSIX terminology, in order to make it
easier to read in conjunction with other references on POSIX.

SEMAPHORE DELETION
The sem_destroy() call terminates an unnamed semaphore and deallocates
any associated memory; the combination of sem_close() and sem_unlink()
has the same effect for named semaphores.  Take care when deleting
semaphores, particularly those used for mutual exclusion, to avoid
deleting a semaphore out from under a task that has already locked
that semaphore.  Applications should adopt the protocol of only
deleting semaphores that the deleting task has successfully locked.
(Similarly, for named semaphores, applications should take care to
only close semaphores that the closing task has opened.)

If there are tasks blocked waiting for the semaphore, sem_destroy()
fails and sets `errno' to EBUSY.

INTERNAL:
POSIX indicates that semaphores may be implemented using a file
descriptor.  I have chosen not to include this functionality in this
implementation of semaphores.  POSIX specs do not insist on a file descriptor 
implementation.

There is an attempt to deal with the issues of a task connecting to a 
semaphore and the persistence of that semaphore.  Persistence implies that
a semaphore and its associated state remains valid until the reference is
released.  Ref.  sem_close() and sem_unlink().

Detection of deadlock is not considered in this implementation.  POSIX
considers the issue but does not require it to be addressed for POSIX.

The routines in this library are compliant to POSIX 1003.4 draft 14.

INCLUDE FILES: semaphore.h

SEE ALSO: POSIX 1003.1b document, semLib, 
.pG "Basic OS"
*/

#include "vxWorks.h"
#include "errno.h"
#include "semaphore.h"
#include "semLib.h"
#include "symLib.h"
#include "posixName.h"
#include "fcntl.h"
#include "taskLib.h"
#include "stdarg.h"
#include "symSync.h"
#include "string.h"
#include "objLib.h"
#include "qLib.h"
#define __PTHREAD_SRC
#include "pthread.h"

 /* defines */

#define MAX_TASKS	100

/* locals */

LOCAL OBJ_CLASS semPxClass;                        /* sem object class */
LOCAL BOOL	semInitialized;

CLASS_ID semPxClassId = &semPxClass;               /* sem class id */

/*******************************************************************************
*
* semPxLibInit - initialize POSIX semaphore support
*
* This routine must be called before using POSIX semaphores.
*
* RETURNS: OK, or ERROR if there is an error installing the semaphore library.
*/

STATUS semPxLibInit (void)
    {
    if ((!semInitialized) &&
        (classInit (semPxClassId, sizeof (struct sem_des), 
		    OFFSET (struct sem_des, objCore),
                    (FUNCPTR) NULL, (FUNCPTR) NULL, (FUNCPTR) NULL) == OK))
        {
	/* initialize the posix name table */

	posixNameTblInit (0);			/* use default hashing */


	/* init counting semaphore library */

        if (semCLibInit () == ERROR)
            {
            return (ERROR);
            }

        semInitialized = TRUE;        /* we've finished the initialization */
        }

    return (OK);
    }

/*******************************************************************************
*
* sem_init - initialize an unnamed semaphore (POSIX)
*
* This routine is used to initialize the unnamed semaphore <sem>.
* The value of the initialized semaphore is <value>.  Following a successful
* call to sem_init() the semaphore may be used in subsequent calls to
* sem_wait(), sem_trywait(), and sem_post().  This semaphore remains usable
* until the semaphore is destroyed.
*
* The <pshared> parameter currently has no effect.
*
* Only <sem> itself may be used for synchronization.
*
* RETURNS: 0 (OK), or -1 (ERROR) if unsuccessful.
*
* ERRNO:
*  EINVAL
*     - <value> exceeds SEM_VALUE_MAX.
*  ENOSPC 
*     - unable to initialize semaphore due to resource constraints.
*	
* SEE ALSO: sem_wait(), sem_trywait(), sem_post()
*
*/

int sem_init 
    (
    sem_t *      sem, 		/* semaphore to be initialized */
    int          pshared, 	/* process sharing */
    unsigned int value		/* semaphore initialization value */
    )
    {
    /* validate value */
    if (value > SEM_VALUE_MAX)
	{
	errno = EINVAL;
	return (ERROR);
	}

    /* initialize the semaphore library */

    if (semPxLibInit () == OK)
        {
	/* create semaphore */

        if ((sem->semId = semCCreate (SEM_Q_PRIORITY, value)) == NULL)
	    {
	    errno = ENOSPC;
	    return (ERROR);
	    }

        sem->sem_name = NULL; 		           /* init the structure */
        objCoreInit (&sem->objCore, semPxClassId); /* validate object */
        }
    else
	{
	errno = ENOSPC;
        return (ERROR);
        }

    return (OK);
    }

/*******************************************************************************
*
* sem_destroy - destroy an unnamed semaphore (POSIX)
*
* This routine is used to destroy the unnamed semaphore indicated by <sem>.
*
* The sem_destroy() call can only destroy a semaphore created by sem_init().
* Calling sem_destroy() with a named semaphore will cause a EINVAL error.
* Subsequent use of the <sem> semaphore will cause an EINVAL error in the
* calling function.
*
* If one or more tasks is blocked on the semaphore, the semaphore is not
* destroyed.
* 
* WARNING
* Take care when deleting semaphores, particularly those used for
* mutual exclusion, to avoid deleting a semaphore out from under a
* task that has already locked that semaphore.  Applications should
* adopt the protocol of only deleting semaphores that the deleting
* task has successfully locked.
* 
* RETURNS: 0 (OK), or -1 (ERROR) if unsuccessful.
*
* ERRNO:
*  EINVAL 
*     - invalid semaphore descriptor.
*  EBUSY
*     - one or more tasks is blocked on the semaphore.
*	
* SEE ALSO: sem_init()
*
*/

int sem_destroy 
    (
    sem_t * sem		/* semaphore descriptor */
    )
    {

    taskLock ();                                /* TASK LOCK */

    if (OBJ_VERIFY (sem, semPxClassId) != OK)
        {
        taskUnlock ();                          /* TASK UNLOCK */
        errno = EINVAL;
        return (ERROR);                         /* invalid object */
        }

    if (sem->sem_name != NULL)			/* close via sem_close/unlink */
	{
        taskUnlock ();                          /* TASK UNLOCK */
	errno = EINVAL;
	return (ERROR);
	}

    if (qFirst (&(sem->semId->qHead)) != NULL) 
	{
        taskUnlock ();                          /* TASK UNLOCK */
	errno = EBUSY;
	return (ERROR);				/* someone waiting on sem */
	}

    objCoreTerminate (&sem->objCore);  		/* terminate object */
    semDelete (sem->semId);
    taskUnlock ();				/* TASK UNLOCK */

    return (OK);
    }

/*******************************************************************************
*
* sem_open - initialize/open a named semaphore (POSIX)
*
* This routine establishes a connection between a named semaphore and
* a task.  Following a call to sem_open() with a semaphore name <name>,
* the task may reference the semaphore associated with <name> using
* the address returned by this call.  This semaphore may be used in 
* subsequent calls to sem_wait(), sem_trywait(), and sem_post().  The 
* semaphore remains usable until the semaphore is closed by a successful
* call to sem_close().  
*
* The <oflag> argument controls whether the semaphore is created or merely
* accessed by the call to sem_open().  The following flag bits may be set
* in <oflag>:
*
* .iP O_CREAT
* Use this flag to create a semaphore if it does not already exist.  If
* O_CREAT is set and the semaphore already exists, O_CREAT has no effect
* except as noted below under O_EXCL.  Otherwise, sem_open() creats a
* semaphore.  O_CREAT requires a third and fourth argument: <mode>, which is
* of type mode_t, and <value>, which is of type unsigned int.  <mode> has no
* effect in this implementation.  The semaphore is created with an initial
* value of <value>.  Valid initial values for semaphores must be less than
* or equal to SEM_VALUE_MAX.
*
* .iP O_EXCL
* If O_EXCL and O_CREAT are set, sem_open() will fail if the semaphore name
* exists.  If O_EXCL is set and O_CREAT is not set, the named semaphore 
* is not created.
* .LP
* 
* To determine whether a named semaphore already exists in the system,
* call sem_open() with the flags `O_CREAT | O_EXCL'.  If the
* sem_open() call fails, the semaphore exists.
* 
* If a task makes multiple calls to sem_open() with the same value
* for <name>, then the same semaphore address is returned for each such
* call, provided that there have been no calls to sem_unlink()
* for this semaphore.
*
* References to copies of the semaphore will produce undefined results.
*
* NOTE
* The current implementation has the following limitations:
*
*     - A semaphore cannot be closed with calls to _exit() or exec().
*     - A semaphore cannot be implemented as a file.
*     - Semaphore names will not appear in the file system.  
*
* RETURNS: A pointer to sem_t, or  -1 (ERROR) if unsuccessful.  
*
* ERRNO:
*  EEXIST
*     - O_CREAT | O_EXCL are set and the semaphore already exists.
*  EINVAL
*     - <value> exceeds SEM_VALUE_MAX or the semaphore name is invalid.
*  ENAMETOOLONG
*     - the semaphore name is too long.
*  ENOENT
*     - the named semaphore does not exist and O_CREAT is not set.
*  ENOSPC
*     - the semaphore could not be initialized due to resource constraints.
*
* SEE ALSO: sem_unlink()
*
* INTERNAL:
* Note that if the sem already exists and O_CREAT is not set then this call
* has no effect.  If the sem does not exist and only the O_EXCL flag
* is set then ENOENT is set and an -1 returned.  These are not clear from the 
* POSIX specifications.
*
*/

sem_t * sem_open 
    (
    const char * name, 	/* semaphore name */
    int 	 oflag, 	/* semaphore creation flags */
    ...				/* extra optional parameters */
    )
    {
    va_list 	 vaList;     	 /* traverses argument list */
    mode_t  	 mode;		 /* mode of semaphore */
    unsigned int value;		 /* initial value of semaphore */
    sem_t *      pSemDesc;	 /* semaphore descriptor */
    BOOL	 found = FALSE;	 /* find named object */
    void *       pPool = NULL;   /* area for name */
    SYM_TYPE 	 dummy;		 /* dummy var for calling symFindByName */
    
#if _POSIX_NO_TRUNC
    /* check name length */
    if (strlen (name) > NAME_MAX)
        {
        errno = ENAMETOOLONG;
        return ((sem_t *) ERROR);
        }
#endif

    /* Initialize semaphore library */
    if (semPxLibInit () == ERROR)
        {
        errno = ENOSPC;
        return ((sem_t *) ERROR);
        }

    /* The following symTblLock is used to insure that no other task
     * can create a semaphore of the same name as this one. This
     * could have occurred if another task tried to access the name table
     * between symFindByName returning not found and the name being
     * added to the name table via symAdd.
     */

    symTblLock (posixNameTbl);			/* LOCK NAME TABLE */

    if (symFindByName (posixNameTbl, (char *) name, (char **) &pSemDesc, 
		       &dummy) == OK)
	{
    	found = TRUE;
	}

    if (found) 					/* found */
	{
	if (O_EXCL & oflag)
            {
    	    symTblUnlock (posixNameTbl);	/* UNLOCK NAME TABLE */
            errno = EEXIST;
            return ((sem_t *) ERROR);
            }
        else 
	    {
    	    symTblUnlock (posixNameTbl);	/* UNLOCK NAME TABLE */
	    /* validate semaphore descriptor */
	    if (OBJ_VERIFY (pSemDesc, semPxClassId) != 0)
		{
		errno = EINVAL;
		return ((sem_t *) ERROR);
		}
	    pSemDesc->refCnt++;			/* attach */
            return (pSemDesc);
	    }
	}
    else 					/* not found */
	if (!(O_CREAT & oflag))			/* if not creating */
            {
    	    symTblUnlock (posixNameTbl);	/* UNLOCK NAME TABLE */
            errno = ENOENT;
            return ((sem_t *) ERROR);
            }

    /* retrieve optional parameters */
    va_start (vaList, oflag);
    mode = va_arg (vaList, mode_t);
    value = va_arg (vaList, uint_t);
    va_end (vaList);

    /* validate parameter */
    if (value > SEM_VALUE_MAX)
        {
        symTblUnlock (posixNameTbl);		/* UNLOCK NAME TABLE */
        errno = EINVAL;
        return ((sem_t *) ERROR);
        }

    if ((pSemDesc = (sem_t *) objAllocExtra (semPxClassId,
                                             strlen (name) + 1,
                                             &pPool)) == NULL)
        {
        symTblUnlock (posixNameTbl);            /* UNLOCK NAME TABLE */
        errno = ENOSPC;
        return ((sem_t *) ERROR);
        }

    strcpy ((char *) pPool, name); /* initialize name */

    /* create a semaphore & initialize semaphore structure */
    if ((pSemDesc->semId = semCCreate (SEM_Q_PRIORITY, value)) == NULL)
        {
        symTblUnlock (posixNameTbl);            /* UNLOCK NAME TABLE */
        objFree (semPxClassId, (char *) pSemDesc);
        errno = ENOSPC;
        return ((sem_t *) ERROR);
        }

    objCoreInit (&pSemDesc->objCore, semPxClassId); /* validate file object */
    pSemDesc->refCnt = 1;			    /* first opening */
    pSemDesc->sem_name = pPool;		  	    /* initialize object name */

    /* add name to name table */
    if ((symAdd (posixNameTbl, (char *) name, 
		(char *) pSemDesc, 0, 0)) == ERROR)
        {
        symTblUnlock (posixNameTbl);            /* UNLOCK NAME TABLE */
        semDelete (pSemDesc->semId);
        objFree (semPxClassId, (char *) pSemDesc);
        errno = EINVAL;
        return ((sem_t *) ERROR);
        }

    symTblUnlock (posixNameTbl);		/* UNLOCK NAME TABLE */
    return (pSemDesc);
    }

/*******************************************************************************
* sem_close - close a named semaphore (POSIX)
*
* This routine is called to indicate that the calling task is finished with
* the specified named semaphore, <sem>.  Do not call this routine
* with an unnamed semaphore (i.e., one created by sem_init()); the
* effects are undefined.  The sem_close() call deallocates any system
* resources allocated by the system for use by this task for this semaphore.
*
* If the semaphore has not been removed with a call to sem_unlink(),
* then sem_close() has no effect on the state of the semaphore.
* However, if the semaphore has been unlinked, the semaphore vanishes
* when the last task closes it.
*
* WARNING
* Take care to avoid risking the deletion of a semaphore that another
* task has already locked.  Applications should only close semaphores
* that the closing task has opened.
*
* RETURNS: 0 (OK), or -1 (ERROR) if unsuccessful.
*
* ERRNO:
*  EINVAL
*     - invalid semaphore descriptor.
*
* SEE ALSO: sem_unlink(), sem_open(), sem_init()
*
*/

int sem_close 
    (
    sem_t * sem			/* semaphore descriptor */
    )
    {

    taskLock ();                                        /* TASK LOCK */

    /* validate semaphore */
    if (OBJ_VERIFY (sem, semPxClassId) != 0)
        {
        taskUnlock ();                                  /* TASK UNLOCK */
        errno = EINVAL;
        return (ERROR);                                 /* invalid object */
        }

    /* semaphore already closed */
    if (sem->refCnt != 0)
	{
        sem->refCnt--;
	}

    /* No effect unless the name has been unlinked from name table */
    if ((sem->sem_name == NULL) && (sem->refCnt == 0))
	{
        /* release semaphore descriptor  */
        objCoreTerminate (&sem->objCore);  	/* terminate object */
        taskUnlock ();                                  /* TASK UNLOCK */
	semDelete (sem->semId);
        objFree (semPxClassId, (char *) sem);
	}
    else
	{
        taskUnlock ();					/* TASK UNLOCK */
	}

    return (OK);
    }

/*******************************************************************************
*
* sem_unlink - remove a named semaphore (POSIX)
*
* This routine removes the string <name> from the semaphore name
* table, and marks the corresponding semaphore for destruction.  An
* unlinked semaphore is destroyed when the last task closes it with
* sem_close().  After a particular name is removed from the table,
* calls to sem_open() using the same name cannot connect to the same
* semaphore, even if other tasks are still using it.  Instead, such
* calls refer to a new semaphore with the same name.
*
* RETURNS: 0 (OK), or -1 (ERROR) if unsuccessful.
*
* ERRNO:
*  ENAMETOOLONG
*     - semaphore name too long.
*  ENOENT
*     - named semaphore does not exist.
*
* SEE ALSO: sem_open(), sem_close()
*
* INTERNAL:
* This routine should have no immediate effect if  the semaphore is
* currently referenced by other tasks.
*
*/

int sem_unlink 
    (
    const char * name	/* semaphore name */
    )
    {
    sem_t *  pSemDesc = NULL;
    SYM_TYPE dummy; 		/* dummy var for calling symFindByName */

#if _POSIX_NO_TRUNC
    if (strlen (name) > NAME_MAX) 		/* check name length */
        {
        errno = ENAMETOOLONG;
        return (ERROR);
        }
#endif

    symTblLock (posixNameTbl);				/* LOCK NAME TABLE */

    /* find name in the name table */
    if (symFindByName (posixNameTbl, (char *)name, 
		       (char **) &pSemDesc, &dummy) == ERROR)
	{
	symTblUnlock (posixNameTbl);			/* UNLOCK NAME TABLE */
	errno = ENOENT;					/* name not found */
	return (ERROR);
	}

    /* remove name from table*/
    symRemove (posixNameTbl, (char *) name, 0);
    symTblUnlock (posixNameTbl);			/* UNLOCK NAME TABLE */

    /* The following taskLock is used to insure that sem_close cannot
     * destroy pSemDesc between unlinking the semaphore name and
     * going on to invalidate the semaphore descriptor.
     */

    taskLock ();					/* TASK LOCK */

    /* release semaphore descriptor  */

    if (OBJ_VERIFY (pSemDesc, semPxClassId) != 0)
        {
        taskUnlock ();                                  /* TASK UNLOCK */
        errno = ENOENT;
        return (ERROR);                         	/* invalid object */
        }


    /* initialize name string */
    pSemDesc->sem_name = NULL;

    if (pSemDesc->refCnt == 0)
	{
        /* invalidate semaphore descriptor  */
        objCoreTerminate (&pSemDesc->objCore);
        taskUnlock ();                                  /* TASK UNLOCK */
	semDelete (pSemDesc->semId);
        objFree (semPxClassId, (char *) pSemDesc);
	}
    else
	{
        taskUnlock ();					/* TASK UNLOCK */
	}

    return (OK);
    }

/*******************************************************************************
*
* sem_wait - lock (take) a semaphore, blocking if not available (POSIX)
*
* This routine locks the semaphore referenced by <sem> by performing the
* semaphore lock operation on that semaphore.  If the semaphore value is
* currently zero, the calling task will not return from the call
* to sem_wait() until it either locks the semaphore or the call is
* interrupted by a signal.  
*
* On return, the state of the semaphore is locked and will remain locked
* until sem_post() is executed and returns successfully.
*
* Deadlock detection is not implemented.
*
* Note that the POSIX term \f2lock\f1 corresponds to the term \f2take\f1 used
* in other VxWorks documentation regarding semaphores.
*
* RETURNS: 0 (OK), or -1 (ERROR) if unsuccessful.
*
* ERRNO:
*  EINVAL
*     - invalid semaphore descriptor, or semaphore destroyed while task waiting.
*	
* SEE ALSO: sem_trywait(), sem_post()
*
*/

int sem_wait 
    (
    sem_t * sem		/* semaphore descriptor */
    )
    {
    int savtype;

    /* validate the semaphore */
    if (OBJ_VERIFY (sem, semPxClassId) != OK)
        {
        errno = EINVAL;
        return (ERROR);                              /* invalid object */
        }

    /* Link to pthreads support code */

    if (_func_pthread_setcanceltype != NULL)
        {
        _func_pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &savtype);
        }

    /* lock the semaphore */

    if (semTake (sem->semId, WAIT_FOREVER) == ERROR)
	{
	/* Link to pthreads support code */

        if (_func_pthread_setcanceltype != NULL)
            {
            _func_pthread_setcanceltype(savtype, NULL);
            }

	errno = EINVAL;
	return (ERROR);
	}

    /* Link to pthreads support code */

    if (_func_pthread_setcanceltype != NULL)
        {
        _func_pthread_setcanceltype(savtype, NULL);
        }

     return (OK);
    }

/*******************************************************************************
*
* sem_trywait - lock (take) a semaphore, returning error if unavailable (POSIX)
*
* This routine locks the semaphore referenced by <sem> only if the 
* semaphore is currently not locked; that is, if the semaphore value is
* currently positive.  Otherwise, it does not lock the semaphore.
* In either case, this call returns immediately without blocking.
*
* Upon return, the state of the semaphore is always locked (either 
* as a result of this call or by a previous sem_wait() or sem_trywait()).
* The semaphore will remain locked until sem_post() is executed and returns
* successfully.
*
* Deadlock detection is not implemented.
*
* Note that the POSIX term \f2lock\f1 corresponds to the term \f2take\f1 used
* in other VxWorks semaphore documentation.
*
* RETURNS: 0 (OK), or -1 (ERROR) if unsuccessful.
*
* ERRNO:
*  EAGAIN
*     - semaphore is already locked.
*  EINVAL
*     - invalid semaphore descriptor.
*   
* SEE ALSO: sem_wait(), sem_post()
*
*/

int sem_trywait 
    (
    sem_t * sem		/* semaphore descriptor */
    )
    {
    /* validate the semaphore */
    if (OBJ_VERIFY (sem, semPxClassId) != OK)
        {
        errno = EINVAL;
        return (ERROR);                              /* invalid object */
        }

    /* lock the semaphore */
    if (semTake (sem->semId, NO_WAIT) == ERROR)
	{
	errno = EAGAIN;
	return (ERROR);
	}

    return (OK);
    }

/*******************************************************************************
*
* sem_post - unlock (give) a semaphore (POSIX)
*
* This routine unlocks the semaphore referenced by <sem> by performing
* the semaphore unlock operation on that semaphore.
*
* If the semaphore value resulting from the operation is positive, then no
* tasks were blocked waiting for the semaphore to become unlocked;
* the semaphore value is simply incremented.
*
* If the value of the semaphore resulting from this semaphore is zero, then
* one of the tasks blocked waiting for the semaphore will
* return successfully from its call to sem_wait().  
*
* NOTE
* The _POSIX_PRIORITY_SCHEDULING functionality is not yet supported.
*
* Note that the POSIX terms \f2unlock\f1 and \f2post\f1 correspond to
* the term \f2give\f1 used in other VxWorks semaphore documentation.
*
* RETURNS: 0 (OK), or -1 (ERROR) if unsuccessful.
*
* ERRNO:
*  EINVAL
*     - invalid semaphore descriptor.
*
* SEE ALSO: sem_wait(), sem_trywait()
*
*/

int sem_post 
    (
    sem_t * sem		/* semaphore descriptor */
    )
    {
    /* validate the semaphore */
    if (OBJ_VERIFY (sem, semPxClassId) != OK)
        {
        errno = EINVAL;
        return (ERROR);                              /* invalid object */
        }

    /* unlock the semaphore */
    if (semGive (sem->semId) == ERROR)
	{
	errno = EINVAL;
	return (ERROR);
	}

    return (OK);
    }

/*******************************************************************************
*
* sem_getvalue - get the value of a semaphore (POSIX)
*
* This routine updates the location referenced by the <sval> argument
* to have the value of the semaphore referenced by <sem> without affecting
* the state of the semaphore.  The updated value represents an actual semaphore
* value that occurred at some unspecified time during the call, but may
* not be the actual value of the semaphore when it is returned to the calling 
* task.
*
* If <sem> is locked, the value returned by sem_getvalue() will either be 
* zero or a negative number whose absolute value represents the number 
* of tasks waiting for the semaphore at some unspecified time during the call.
*
* RETURNS: 0 (OK), or -1 (ERROR) if unsuccessful.
*
* ERRNO:
*  EINVAL
*     - invalid semaphore descriptor.
*
* SEE ALSO: sem_post(), sem_trywait(), sem_trywait()
*
*/

int sem_getvalue 
    (
    sem_t *   	   sem, 	/* semaphore descriptor */
    int * 	   sval		/* buffer by which the value is returned */
    )
    {
    int        	taskIdList [MAX_TASKS];
    int		count;
    SEM_ID	temp;

    /* validate the semaphore */
    if (OBJ_VERIFY (sem, semPxClassId) != OK)
        {
        errno = EINVAL;
        return (ERROR);                         /* invalid object */
        }

    /* determine semaphore count */
    temp  = sem->semId;
    count = temp->semCount;

    if (count == 0)
	{
	/* number of tasks waiting */
	*sval = -semInfo (sem->semId, &taskIdList[0], MAX_TASKS);
        return (OK);
	}

    *sval = count;
    return (OK);
    }

