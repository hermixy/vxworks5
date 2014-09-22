/* semLib.c - general semaphore library */

/* Copyright 1984-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
02j,09nov01,dee  add CPU_FAMILY != COLDFIRE in portable test
02i,26oct01,bwa  Added semEvLib and eventLib to the list of 'SEE ALSO'
                 modules.
02h,12oct01,cjj  Added documentation regarding S_eventLib_EVENTSEND_FAILED
                 error
02g,06sep01,bwa  Added VxWorks events support.
02f,16nov98,cdp  make ARM CPUs with ARM_THUMB==TRUE use portable routines.
02f,03mar00,zl   merged SH support into T2
02e,05oct98,jmp  doc: cleanup.
02d,17feb98,cdp  undo 02c: put ARM back in list of optimised CPUs.
02c,21oct97,kkk  undo 02d, take out ARM from list of optimized CPUs.
02d,19may97,jpd  added ARM to list of optimised CPUs.
02c,17oct96,dgp  doc: add errnos for user-visible routines per SPR 6893
02b,13jun96,ms   conditionally compiled the windview instrumentation 
02a,22oct95,jdi  doc: added bit values for semTake() timeout param (SPR 4276).
01z,19mar95,dvs  removed tron references.
01y,03feb95,rhp  edit SEMAPHORE DELETION manpage text, duplicate under semDelete()
01x,18jan95,rhp  Doc errno value on semTake() return due to timeout (SPR#1930)
01w,09jun93,hdn  added a support for I80X86
01z,14apr94,smb  fixed class dereferencing for instrumentation macros
01y,15mar94,smb  modified instrumentation macros
01x,24jan94,smb  added instrumentation macros
01w,10dec93,smb  added instrumentation
01v,29jan93,pme  added little endian support for shared sempahores.
01u,20jan93,jdi  documentation cleanup for 5.1.
01t,13nov92,dnw  added include of smObjLib.h
01s,02oct92,pme  added reference to shared semaphores documentation.
01r,22jul92,pme  made semDestroy return S_smObjLib_NO_OBJECT_DESTROY when
		 trying to destroy a shared semaphore.
01q,19jul92,pme  added shared memory semaphores support.
01p,04jul92,jcf  tables now initialized to null to reduce coupling.
01o,26may92,rrr  the tree shuffle
01n,15sep91,ajm  added MIPS to list of optimized CPU's
01m,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed includes to have absolute path from h/
		  -fixed #else and #endif
		  -changed VOID to void
		  -changed copyright notice
01l,26sep91,hdn  added conditional flag for TRON optimized code.
01k,15aug91,del  changed interface to qInit to pass all "optional" args.
01j,05apr91,jdi	 documentation -- removed header parens and x-ref numbers;
		 doc review by jcf.
01i,24mar91,jdi  documentation cleanup.
01h,05oct90,dnw  made semTerminate() be NOMANUAL.
01g,28aug90,jcf  documentation.
01f,10aug90,dnw  corrected forward declaration of semQFlushDefer().
01e,03aug90,jcf  documentation.
01d,05jul90,jcf  optimized version now available.
01c,26jun90,jcf  merged into one semaphore class.
		 added semFlush ()
01b,11may90,jcf  fixed up PORTABLE definition.
01a,20oct89,jcf  written based on v1g of semLib.
*/

/*
DESCRIPTION
Semaphores are the basis for synchronization and mutual exclusion in
VxWorks.  They are powerful in their simplicity and form the foundation
for numerous VxWorks facilities.

Different semaphore types serve different needs, and while the behavior
of the types differs, their basic interface is the same.  This library
provides semaphore routines common to all VxWorks semaphore types.  For
all types, the two basic operations are semTake() and semGive(), the
acquisition or relinquishing of a semaphore.

Semaphore creation and initialization is handled by other libraries,
depending on the type of semaphore used.  These libraries contain
full functional descriptions of the semaphore types:

    semBLib  - binary semaphores
    semCLib  - counting semaphores
    semMLib  - mutual exclusion semaphores
    semSmLib - shared memory semaphores

Binary semaphores offer the greatest speed and the broadest applicability.

The semLib library provides all other semaphore operations, including
routines for semaphore control, deletion, and information.  Semaphores
must be validated before any semaphore operation can be undertaken.  An
invalid semaphore ID results in ERROR, and an appropriate `errno' is set.

SEMAPHORE CONTROL
The semTake() call acquires a specified semaphore, blocking the calling
task or making the semaphore unavailable.  All semaphore types support a
timeout on the semTake() operation.  The timeout is specified as the
number of ticks to remain blocked on the semaphore.  Timeouts of
WAIT_FOREVER and NO_WAIT codify common timeouts.  If a semTake() times
out, it returns ERROR.  Refer to the library of the specific semaphore 
type for the exact behavior of this operation.

The semGive() call relinquishes a specified semaphore, unblocking a pended
task or making the semaphore available.  Refer to the library of the
specific semaphore type for the exact behavior of this operation.

The semFlush() call may be used to atomically unblock all tasks pended on
a semaphore queue, i.e., all tasks will be unblocked before any are allowed
to run.  It may be thought of as a broadcast operation in
synchronization applications.  The state of the semaphore is unchanged by
the use of semFlush(); it is not analogous to semGive().

SEMAPHORE DELETION
The semDelete() call terminates a semaphore and deallocates any
associated memory.  The deletion of a semaphore unblocks tasks pended
on that semaphore; the routines which were pended return ERROR.  Take
care when deleting semaphores, particularly those used for mutual
exclusion, to avoid deleting a semaphore out from under a task that
already has taken (owns) that semaphore.  Applications should adopt
the protocol of only deleting semaphores that the deleting task has
successfully taken.

SEMAPHORE INFORMATION
The semInfo() call is a useful debugging aid, reporting all tasks
blocked on a specified semaphore.  It provides a snapshot of the queue at
the time of the call, but because semaphores are dynamic, the information
may be out of date by the time it is available.  As with the current state of
the semaphore, use of the queue of pended tasks should be restricted to
debugging uses only.

VXWORKS EVENTS
If a task has registered for receiving events with a semaphore, events will be
sent when that semaphore becomes available. By becoming available, it is
implied that there is a change of state. For a binary semaphore, there is only 
a change of state when a semGive() is done on a semaphore that was taken. 
For a counting semaphore, there is always a change of state when the semaphore 
is available, since the count is incremented each time. For a mutex, a 
semGive() can only be performed if the current task is the owner, implying that 
the semaphore has been taken; thus, there is always a change of state.

INTERNAL:
	WINDVIEW INSTRUMENTATION
Level 1:
	semGive() causes EVENT_SEMGIVE		(portable only)
	semTake() causes EVENT_SEMTAKE		(portable only)
	semFlush() causes EVENT_SEMFLUSH
	semDestroy() causes EVENT_SEMDELETE

Level 2:
	semGive() causes EVENT_OBJ_SEMGIVE	(portable only)
	semTake() causes EVENT_OBJ_SEMTAKE	(portable only)
	semFlush() causes EVENT_OBJ_SEMFLUSH	

Level 3:
	N/A

INCLUDE FILES: semLib.h

SEE ALSO: taskLib, semBLib, semCLib, semMLib, semSmLib, semEvLib, eventLib,
.pG "Basic OS"
*/

#include "vxWorks.h"
#include "taskLib.h"
#include "qLib.h"
#include "intLib.h"
#include "errno.h"
#include "smObjLib.h"
#include "private/eventLibP.h"
#include "private/classLibP.h"
#include "private/eventP.h"
#include "private/objLibP.h"
#include "private/semLibP.h"
#include "private/windLibP.h"
#include "private/workQLibP.h"
#include "private/semSmLibP.h"

/* optimized version available for 680X0, MIPS, i86, SH, */
/* COLDFIRE and ARM (excluding Thumb) */

#if (defined(PORTABLE) || \
     ((CPU_FAMILY != MC680X0) && \
      (CPU_FAMILY != MIPS) && \
      (CPU_FAMILY != I80X86) && \
      (CPU_FAMILY != SH) && \
      (CPU_FAMILY != COLDFIRE) && \
      (CPU_FAMILY != ARM)) || \
     ((CPU_FAMILY == ARM) && ARM_THUMB))
#define semLib_PORTABLE
#endif

/* locals */

LOCAL BOOL	semLibInstalled;		/* protect from muliple inits */

/* globals */

OBJ_CLASS semClass;				/* semaphore object class */
CLASS_ID  semClassId = &semClass;		/* semaphore class ID */

#ifdef	WV_INSTRUMENTATION
OBJ_CLASS semInstClass;				/* semaphore object class */
CLASS_ID  semInstClassId = &semInstClass;	/* semaphore class ID */
#endif

FUNCPTR semGiveTbl [MAX_SEM_TYPE] =		/* semGive() methods by type */
    {
    (FUNCPTR) semInvalid, (FUNCPTR) semInvalid, (FUNCPTR) semInvalid,
    (FUNCPTR) semInvalid, (FUNCPTR) semInvalid, (FUNCPTR) semInvalid,
    (FUNCPTR) semInvalid, (FUNCPTR) semInvalid
    };

FUNCPTR semTakeTbl [MAX_SEM_TYPE] =		/* semTake() methods by type */
    {
    (FUNCPTR) semInvalid, (FUNCPTR) semInvalid, (FUNCPTR) semInvalid,
    (FUNCPTR) semInvalid, (FUNCPTR) semInvalid, (FUNCPTR) semInvalid,
    (FUNCPTR) semInvalid, (FUNCPTR) semInvalid
    };

FUNCPTR semFlushTbl [MAX_SEM_TYPE] =		/* semFlush() methods by type */
    {
    (FUNCPTR) semInvalid, (FUNCPTR) semInvalid, (FUNCPTR) semInvalid,
    (FUNCPTR) semInvalid, (FUNCPTR) semInvalid, (FUNCPTR) semInvalid,
    (FUNCPTR) semInvalid, (FUNCPTR) semInvalid
    };

FUNCPTR semGiveDeferTbl [MAX_SEM_TYPE] =	/* semGiveDefer() methods */
    {
    (FUNCPTR) NULL, (FUNCPTR) NULL, (FUNCPTR) NULL, (FUNCPTR) NULL,
    (FUNCPTR) NULL, (FUNCPTR) NULL, (FUNCPTR) NULL, (FUNCPTR) NULL
    };

FUNCPTR semFlushDeferTbl [MAX_SEM_TYPE] =	/* semFlushDefer() methods */
    {
    (FUNCPTR) NULL, (FUNCPTR) NULL, (FUNCPTR) NULL, (FUNCPTR) NULL,
    (FUNCPTR) NULL, (FUNCPTR) NULL, (FUNCPTR) NULL, (FUNCPTR) NULL
    };


/*******************************************************************************
*
* semLibInit - initialize the semaphore management package
*
* This routine initializes the semaphore object class.  No semaphore
* operation will work until this is called.  This routine is called during
* system configuration within kernelInit().  This routine should only be
* called once.
*
* SEE ALSO: classLib
* NOMANUAL
*/

STATUS semLibInit (void)
    {
    if ((!semLibInstalled) &&
	(classInit (semClassId, sizeof(SEMAPHORE), OFFSET(SEMAPHORE,objCore),
         (FUNCPTR) NULL, (FUNCPTR) NULL, (FUNCPTR) semDestroy) == OK))
	{
#ifdef	WV_INSTRUMENTATION
	/* initialise instrumented class for semaphores */
	semClassId->initRtn = semInstClassId;
        classInstrument (semClassId, semInstClassId);
#endif

	semEvLibInit (); /* pull semaphore-related events lib in kernel */

	semLibInstalled = TRUE;
	}

    return ((semLibInstalled) ? OK : ERROR);
    }

#ifdef semLib_PORTABLE

/*******************************************************************************
*
* semGive - give a semaphore
*
* This routine performs the give operation on a specified semaphore.
* Depending on the type of semaphore, the state of the semaphore and of the
* pending tasks may be affected.  If no tasks are pending on the sempahore and
* a task has previously registered to receive events from the semaphore, these
* events are sent in the context of this call.  This may result in the
* unpending of the task waiting for the events.  If the semaphore fails to
* send events and if it was created using the SEM_EVENTSEND_ERR_NOTIFY 
* option, ERROR is returned even though the give operation was successful.
* The behavior of semGive() is discussed fully in the library description 
* of the specific semaphore type being used.
*
* RETURNS: OK on success or ERROR otherwise
*
* ERRNO:
* .iP "S_intLib_NOT_ISR_CALLABLE"
* Routine was called from an ISR for a mutex semaphore.
* .iP "S_objLib_OBJ_ID_ERROR"
* Semaphore ID is invalid.
* .iP "S_semLib_INVALID_OPERATION"
* Current task not owner of mutex semaphore.
* .iP "S_eventLib_EVENTSEND_FAILED"
* Semaphore failed to send events to the registered task.  This errno 
* value can only exist if the semaphore was created with the 
* SEM_EVENTSEND_ERR_NOTIFY option.
* .LP
*
* SEE ALSO: semBLib, semCLib, semMLib, semSmLib, semEvStart
*/

STATUS semGive
    (
    SEM_ID semId        /* semaphore ID to give */
    )
    {
    SM_SEM_ID  smObjSemId;	/* shared semaphore ID */

    /* check if semaphore is shared */

    if (ID_IS_SHARED (semId))  
	{
	smObjSemId = (SM_SEM_ID) SM_OBJ_ID_TO_ADRS(semId);
	return ((*semGiveTbl[ntohl (smObjSemId->objType) & SEM_TYPE_MASK])
		(smObjSemId));
	}

    /* otherwise the semaphore is local */

#ifdef	WV_INSTRUMENTATION
    /* windview - level 1 event logging */
    EVT_OBJ_3 (OBJ, semId, semClassId, EVENT_SEMGIVE, semId, 
		semId->state, semId->recurse);
#endif

    if (kernelState)
	return (semGiveDefer (semId));      /* defer semaphore give */
    else
	return((* semGiveTbl[semId->semType & SEM_TYPE_MASK]) (semId));
    }

/*******************************************************************************
*
* semTake - take a semaphore
*
* This routine performs the take operation on a specified semaphore.
* Depending on the type of semaphore, the state of the semaphore and
* the calling task may be affected.  The behavior of semTake() is
* discussed fully in the library description of the specific semaphore 
* type being used.
*
* A timeout in ticks may be specified.  If a task times out, semTake() will
* return ERROR.  Timeouts of WAIT_FOREVER (-1) and NO_WAIT (0) indicate to wait
* indefinitely or not to wait at all.
*
* When semTake() returns due to timeout, it sets the errno to
* S_objLib_OBJ_TIMEOUT (defined in objLib.h).
* 
* The semTake() routine is not callable from interrupt service routines.
*
* RETURNS: OK, or ERROR if the semaphore ID is invalid or the task timed out.
*
* ERRNO: S_intLib_NOT_ISR_CALLABLE, S_objLib_OBJ_ID_ERROR,
*        S_objLib_OBJ_UNAVAILABLE
*
* SEE ALSO: semBLib, semCLib, semMLib, semSmLib
*/

STATUS semTake
    (
    SEM_ID semId,       /* semaphore ID to take */
    int timeout         /* timeout in ticks */
    )
    {
    SM_SEM_ID  smObjSemId;      /* shared semaphore ID */

    /* check if semaphore is global (shared) */

    if (ID_IS_SHARED (semId))
	{
	smObjSemId = (SM_SEM_ID) SM_OBJ_ID_TO_ADRS(semId);
        return ((* semTakeTbl[ntohl (smObjSemId->objType) & SEM_TYPE_MASK ]) 
		 (smObjSemId, timeout));
	}

    /* otherwise sempahore is local */

#ifdef	WV_INSTRUMENTATION
    /* windview - level 1 event logging */
    EVT_OBJ_3 (OBJ, semId, semClassId, EVENT_SEMTAKE, semId, 
		semId->state, semId->recurse);
#endif

    return ((* (semTakeTbl[semId->semType & SEM_TYPE_MASK])) (semId, timeout));
    }

#endif	/* semLib_PORTABLE */

/*******************************************************************************
*
* semFlush - unblock every task pended on a semaphore
*
* This routine atomically unblocks all tasks pended on a specified
* semaphore, i.e., all tasks will be unblocked before any is allowed
* to run.  The state of the underlying semaphore is unchanged.  All
* pended tasks will enter the ready queue before having a chance to
* execute.
*
* The flush operation is useful as a means of broadcast in
* synchronization applications.  Its use is illegal for mutual-exclusion
* semaphores created with semMCreate().
*
* RETURNS: OK, or ERROR if the semaphore ID is invalid
* or the operation is not supported.
*
* ERRNO: S_objLib_OBJ_ID_ERROR
*
* SEE ALSO: semBLib, semCLib, semMLib, semSmLib
*/

STATUS semFlush
    (
    SEM_ID semId        /* semaphore ID to unblock everyone for */
    )
    {
    SM_SEM_ID  smObjSemId;      /* shared semaphore ID */

    /* check if semaphore is global (shared) */

    if (ID_IS_SHARED (semId))
	{
        smObjSemId = (SM_SEM_ID) SM_OBJ_ID_TO_ADRS(semId);
        return ((*semFlushTbl[ntohl (smObjSemId->objType) &SEM_TYPE_MASK])
		(smObjSemId));
	}

    /* otherwise semaphore is local */
    
#ifdef	WV_INSTRUMENTATION
    /* windview - level 1 event logging */
    EVT_OBJ_4 (OBJ, semId, semClassId, EVENT_SEMFLUSH, semId, 
		semId->state, semId->recurse, &semId->qHead);
#endif

    if (kernelState)
	return (semFlushDefer (semId));
    else
	return((* semFlushTbl[semId->semType & SEM_TYPE_MASK]) (semId));
    }

/*******************************************************************************
*
* semDelete - delete a semaphore
*
* This routine terminates and deallocates any memory associated with a
* specified semaphore.  All tasks pending on the semaphore or pending
* for the reception of events meant to be sent from the semaphore
* will unblock and return ERROR.
*
* WARNING
* Take care when deleting semaphores, particularly those used for
* mutual exclusion, to avoid deleting a semaphore out from under a
* task that already has taken (owns) that semaphore.  Applications
* should adopt the protocol of only deleting semaphores that the
* deleting task has successfully taken.
*
* RETURNS: OK, or ERROR if the semaphore ID is invalid.
*
* ERRNO:
* .iP "S_intLib_NOT_ISR_CALLABLE"
* Routine cannot be called from ISR.
* .iP "S_objLib_OBJ_ID_ERROR"
* Semaphore ID is invalid.
* .iP "S_smObjLib_NO_OBJECT_DESTROY"
* Deleting a shared semaphore is not permitted
*
* SEE ALSO: semBLib, semCLib, semMLib, semSmLib
*/

STATUS semDelete
    (
    SEM_ID semId        /* semaphore ID to delete */
    )
    {
    return (semDestroy (semId, TRUE));
    }

/******************************************************************************
*
* semTerminate - terminate a semaphore
*
* This routine terminates a semaphore.  Any pended tasks will be unblocked
* and returned ERROR.  Unlike semDelete(), this routine does not free any
* associated memory.
*
* RETURNS: OK, or ERROR if invalid semaphore ID.
*
* SEE ALSO: semDelete, semBLib, semCLib, semMLib
*
* NOMANUAL
*/

STATUS semTerminate
    (
    SEM_ID semId        /* semaphore ID to initialize */
    )
    {
    return (semDestroy (semId, FALSE));
    }

/*******************************************************************************
*
* semDestroy - destroy a semaphore
*
* This routine underlies semDelete() and semTerminate() as the function that
* terminates the semaphore and optionally deallocates memory.
*
* RETURNS: OK, or ERROR if invalid semaphore ID.
*
* ERRNO: S_smObjLib_NO_OBJECT_DESTROY
*
* NOMANUAL
*/

STATUS semDestroy
    (
    SEM_ID semId,       /* binary semaphore to destroy */
    BOOL   dealloc      /* deallocate memory associated with semaphore */
    )
    {
    int level;

    if (INT_RESTRICT () != OK)			/* restrict isr use */
	return (ERROR);

    if (ID_IS_SHARED (semId))           	/* if semaphore is shared */
	{
	errno = S_smObjLib_NO_OBJECT_DESTROY;
        return (ERROR);				/* cannot delete shared sem. */
	}

#ifdef	WV_INSTRUMENTATION
    /* windview - level 1 event logging */
    EVT_OBJ_4 (OBJ, semId, semClassId, EVENT_SEMDELETE, 
	       semId, semId->state, semId->recurse, &semId->qHead);
#endif

    level = intLock ();				/* LOCK INTERRUPTS */

    if (OBJ_VERIFY (semId, semClassId) != OK)	/* validate semaphore ID */
	{
	intUnlock (level);			/* UNLOCK INTERRUPTS */
	return (ERROR);
	}

    objCoreTerminate (&semId->objCore);		/* INVALIDATE */

    kernelState = TRUE;				/* KERNEL ENTER */

    intUnlock (level);				/* UNLOCK INTERRUPTS */

    windSemDelete (semId);			/* unblock any pending tasks */

    eventTerminate (&semId->events);      /* unblock task waiting for events*/
 
    TASK_SAFE ();				/* TASK SAFE */

    windExit ();				/* EXIT KERNEL */

    if (dealloc)				/* dealloc memory if selected */
	objFree (semClassId, (char *) semId);

    TASK_UNSAFE ();				/* TASK UNSAFE */

    return (OK);
    }

/*******************************************************************************
*
* semGiveDefer - add appropriate semGive routine to kernel work q
*
* This routine defers the work of giving a semaphore by adding the work to the
* kernel work queue.  If the semaphore ID is invalid or the semaphore type is
* is inheritance this routine will return ERROR, otherwise OK.
*
* ERRNO: S_intLib_NOT_ISR_CALLABLE
*
* NOMANUAL
*/

STATUS semGiveDefer
    (
    SEM_ID semId        /* semaphore ID to give */
    )
    {
    if (OBJ_VERIFY (semId, semClassId) != OK)	/* verify semaphore */
	return (ERROR);

    if (semGiveDeferTbl [semId->semType] == NULL)
	{
	errno = S_intLib_NOT_ISR_CALLABLE;
	return (ERROR);
	}

    workQAdd1 (semGiveDeferTbl [semId->semType], (int) semId);

    return (OK);
    }

/*******************************************************************************
*
* semFlushDefer - add appropriate semFlush routine to kernel work q
*
* This routine defers the work of giving a semaphore by adding the work to the
* kernel work queue.  If the semaphore ID is invalid or the semaphore type is
* is inheritance this routine will return ERROR, otherwise OK.
*
* RETURNS: OK or ERROR
*
* ERRNO: S_intLib_NOT_ISR_CALLABLE
*
* NOMANUAL
*/

STATUS semFlushDefer
    (
    SEM_ID semId        /* semaphore ID to give */
    )
    {
    if (OBJ_VERIFY (semId, semClassId) != OK)	/* verify semaphore */
	return (ERROR);

    if (semFlushDeferTbl [semId->semType] == NULL)
	{
	errno = S_intLib_NOT_ISR_CALLABLE;
	return (ERROR);
	}

    workQAdd1 (semFlushDeferTbl [semId->semType], (int) semId);

    return (OK);
    }

/******************************************************************************
*
* semInvalid - invalid semaphore ID or operation
*
* RETURNS: ERROR with appropriate errno.
*
* ERROR: S_semLib_INVALID_OPERATION
* NOMANUAL
*/

STATUS semInvalid
    (
    SEM_ID semId        /* semId of invalid operation */
    )
    {
    if (OBJ_VERIFY (semId, semClassId) != OK)		/* verify semaphore */
	return (ERROR);

    errno = S_semLib_INVALID_OPERATION;			/* set errno */

    return (ERROR);
    }

/******************************************************************************
*
* semIntRestrict - operation restricted from ISR use
*
* NOMANUAL
* ARGSUSED
*/

STATUS semIntRestrict
    (
    SEM_ID semId        /* semId of invalid operation */
    )
    {
    return (INT_RESTRICT ());			/* restrict ISR use */
    }

/******************************************************************************
*
* semQInit - inialize the semaphore queue to the specified queue type
*
* This routine initialized the semaphore queue to the specified type.
*
* RETURNS: OK, or ERROR if queue type not supported.
*
* ERRNO S_semLib_INVALID_QUEUE_TYPE
*
* NOMANUAL
*/

STATUS semQInit
    (
    SEMAPHORE   *pSemaphore,    /* semphore core to initialize queue for */
    int         options         /* semphore options */
    )
    {
    STATUS status = OK;

    switch (options & SEM_Q_MASK)
	{
	case SEM_Q_FIFO:
	    qInit (&pSemaphore->qHead, Q_FIFO, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	    break;

	case SEM_Q_PRIORITY:
	    qInit (&pSemaphore->qHead, Q_PRI_LIST, 0, 0, 0, 0, 0, 0, 0, 0, 0,0);
	    break;

	default:
    	    errno  = S_semLib_INVALID_QUEUE_TYPE;	/* set errno */
	    status = ERROR;
	    break;
	}

    return (status);
    }

/*******************************************************************************
*
* semQFlush - unblock all tasks pending on this semaphore
*
* Unblock pending tasks. If a higher priority task has already taken
* the semaphore (so that it is now pended waiting for it), that task
* will now become ready to run, and preempt the task that does the semGive().
* If the semaphore is already full (it has been given but not taken) this
* call is essentially a no-op.
*
* NOMANUAL
*/

STATUS semQFlush
    (
    SEM_ID semId        /* semaphore ID to give */
    )
    {
    int level = intLock ();			/* LOCK INTERRUPTS */

    if (OBJ_VERIFY (semId, semClassId) != OK)
	{
	intUnlock (level);			/* UNLOCK INTERRUPTS */
	return (ERROR);
	}

    if (Q_FIRST (&semId->qHead) == NULL)
	{
	intUnlock (level);			/* UNLOCK INTERRUPTS */
	}
    else
	{
	kernelState = TRUE;			/* KERNEL ENTER */
	intUnlock (level);			/* UNLOCK INTERRUPTS */

#ifdef	WV_INSTRUMENTATION
	/* windview - level 2 event logging */
	EVT_TASK_1 (EVENT_OBJ_SEMFLUSH, semId);
#endif

	windPendQFlush (&semId->qHead);		/* flush blocked tasks */

	windExit ();				/* KERNEL EXIT */
	}

    return (OK);
    }

/*******************************************************************************
*
* semQFlushDefer - unblock pending tasks as deferred work
*
* Unblock pending tasks. If a higher priority task has already taken
* the semaphore (so that it is now pended waiting for it), that task
* will now become ready to run, and preempt the task that does the semGive().
* If the semaphore is already full (it has been given but not taken) this
* call is essentially a no-op.
*
* NOMANUAL
*/

void semQFlushDefer
    (
    SEM_ID semId        /* semaphore ID to give */
    )
    {

    if (Q_FIRST (&semId->qHead) != NULL)
	{
#ifdef	WV_INSTRUMENTATION
	/* windview - level 2 event logging */
	EVT_TASK_1 (EVENT_OBJ_SEMFLUSH, semId);
#endif

	windPendQFlush (&semId->qHead);		/* flush blocked tasks */
	}
    }

