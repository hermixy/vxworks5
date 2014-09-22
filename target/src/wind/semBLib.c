/* semBLib.c - binary semaphore library */

/* Copyright 1984-2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
02h,03may02,pcm  removed possible NULL dereference in semBCreate () (SPR 76721)
02g,09nov01,dee  add CPU_FAMILY != COLDFIRE in portable test
02f,18oct01,bwa  Fixed problem when the semaphore could be deleted while
                 sending events.
02e,06sep01,bwa  Added VxWorks events support.
02d,04sep98,cdp  make ARM CPUs with ARM_THUMB==TRUE use portable routines.
02d,03mar00,zl   merged SH support into T2
02c,17feb98,cdp  undo 02b: put ARM back in list of optimised CPUs.
02b,21oct97,kkk  undo 02a, take out ARM from list of optimized CPUs.
02a,19may97,jpd  added ARM to list of optimised CPUs.
01z,24jun96,sbs  made windview instrumentation conditionally compiled
01y,22oct95,jdi  doc: added bit values for semBCreate() options (SPR 4276).
01w,19mar95,dvs  removed tron references.
01v,09jun93,hdn  added a support for I80X86
01y,14apr94,smb  fixed class dereferencing for instrumentation macros
01x,15mar94,smb  modified instrumentation macros
01w,24jan94,smb  added instrumentation macros
01v,10dec93,smb  added instrumentation
01u,23jan93,jdi  documentation cleanup for 5.1.
01t,13nov92,jcf  semBCreate call semBLibInit for robustness.
01s,28jul92,jcf  semB{Create,Init} call semBLibInit for robustness.
01r,09jul92,rrr  changed xsignal.h to private/sigLibP.h
01q,04jul92,jcf  tables now start as null to reduce coupling; private headers
01p,26may92,rrr  the tree shuffle
01o,27apr92,rrr  added signal restarting
01n,15sep91,ajm  added MIPS to list of optimized CPU's
01m,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed includes to have absolute path from h/
		  -fixed #else and #endif
		  -changed VOID to void
		  -changed copyright notice
01l,26sep91,hdn  added conditional flag for TRON optimized code.
01k,05apr91,jdi	 documentation -- removed header parens and x-ref numbers;
		 doc review by jcf.
01j,25mar91,del  fixed bug introduced in 02i that wouldn't allow build.
01i,24mar91,jdi  documentation cleanup.
01h,05oct90,dnw  made semBInit() be NOMANUAL.
01g,29aug90,jcf  documentation.
01f,03aug90,jcf  documentation.
01e,17jul90,dnw  changed to new objAlloc() call
01d,05jul90,jcf  optimized version now available.
01c,26jun90,jcf  merged to one semaphore class.
01b,11may90,jcf  fixed up PORTABLE definition.
01a,20oct89,jcf  written based on v1g of semLib.
*/

/*
DESCRIPTION
This library provides the interface to VxWorks binary semaphores.
Binary semaphores are the most versatile, efficient, and conceptually
simple type of semaphore.  They can be used to: (1) control mutually
exclusive access to shared devices or data structures, or (2) synchronize
multiple tasks, or task-level and interrupt-level processes.  Binary
semaphores form the foundation of numerous VxWorks facilities.

A binary semaphore can be viewed as a cell in memory whose contents are in
one of two states, full or empty.  When a task takes a binary semaphore,
using semTake(), subsequent action depends on the state of the semaphore:
.IP (1) 4
If the semaphore is full, the semaphore is made empty, and the calling task
continues executing.
.IP (2)
If the semaphore is empty, the task will be blocked, pending the
availability of the semaphore.  If a timeout is specified and the timeout
expires, the pended task will be removed from the queue of pended tasks
and enter the ready state with an ERROR status.  A pended task
is ineligible for CPU allocation.  Any number of tasks may be pended
simultaneously on the same binary semaphore.
.LP
When a task gives a binary semaphore, using semGive(), the next available
task in the pend queue is unblocked.  If no task is pending on this
semaphore, the semaphore becomes full.  Note that if a semaphore is given,
and a task is unblocked that is of higher priority than the task that called
semGive(), the unblocked task will preempt the calling task.

MUTUAL EXCLUSION
To use a binary semaphore as a means of mutual exclusion, first create it
with an initial state of full.  For example:
.CS
.ne 4
    SEM_ID semMutex;

    /@ create a binary semaphore that is initially full @/
    semMutex = semBCreate (SEM_Q_PRIORITY, SEM_FULL);
.CE
Then guard a critical section or resource by taking the semaphore with
semTake(), and exit the section or release the resource by giving
the semaphore with semGive().  For example:
.CS
.ne 4
    semTake (semMutex, WAIT_FOREVER);
        ...  /@ critical region, accessible only by one task at a time @/
    
    semGive (semMutex);
.CE

While there is no restriction on the same semaphore being given, taken, or
flushed by multiple tasks, it is important to ensure the proper
functionality of the mutual-exclusion construct.  While there is no danger
in any number of processes taking a semaphore, the giving of a semaphore
should be more carefully controlled.  If a semaphore is given by a task that
did not take it, mutual exclusion could be lost.

SYNCHRONIZATION
To use a binary semaphore as a means of synchronization, create it
with an initial state of empty.  A task blocks by taking a semaphore
at a synchronization point, and it remains blocked until the semaphore is given
by another task or interrupt service routine.

Synchronization with interrupt service routines is a particularly common need.
Binary semaphores can be given, but not taken, from interrupt level.  Thus, a
task can block at a synchronization point with semTake(), and an interrupt
service routine can unblock that task with semGive().

In the following example, when init() is called, the binary semaphore is
created, an interrupt service routine is attached to an event, and a task
is spawned to process the event.  Task 1 will run until it calls semTake(),
at which point it will block until an event causes the interrupt service
routine to call semGive().  When the interrupt service routine completes,
task 1 can execute to process the event.
.CS
.ne 8
    SEM_ID semSync;    /@ ID of sync semaphore @/

    init ()
	{
	intConnect (..., eventInterruptSvcRout, ...);
	semSync = semBCreate (SEM_Q_FIFO, SEM_EMPTY);
	taskSpawn (..., task1);
	}

.ne 6
    task1 ()
	{
	...
	semTake (semSync, WAIT_FOREVER);    /@ wait for event @/
	...    /@ process event @/
	}

.ne 6
    eventInterruptSvcRout ()
	{
	...
	semGive (semSync);    /@ let task 1 process event @/
	...
	}
.CE
A semFlush() on a binary semaphore will atomically unblock all pended
tasks in the semaphore queue, i.e., all tasks will be unblocked at once,
before any actually execute.

CAVEATS
There is no mechanism to give back or reclaim semaphores automatically when
tasks are suspended or deleted.  Such a mechanism, though desirable, is not
currently feasible.  Without explicit knowledge of the state of the guarded
resource or region, reckless automatic reclamation of a semaphore could
leave the resource in a partial state.  Thus, if a task ceases execution
unexpectedly, as with a bus error, currently owned semaphores will not be
given back, effectively leaving a resource permanently unavailable.  The
mutual-exclusion semaphores provided by semMLib offer protection from
unexpected task deletion.

INTERNAL:
	WINDVIEW INSTRUMENTATION
Level 1:
	semBCreate() causes EVENT_SEMBCREATE

Level 2 (portable only):
	semGive() causes EVENT_OBJ_SEMGIVE
	semTake() causes EVENT_OBJ_SEMTAKE

Level 3:
	N/A

INCLUDE FILES: semLib.h

SEE ALSO: semLib, semCLib, semMLib,
.pG "Basic OS"
*/

#include "vxWorks.h"
#include "errno.h"
#include "taskLib.h"
#include "intLib.h"
#include "errnoLib.h"
#include "eventLib.h"
#include "private/eventLibP.h"
#include "private/sigLibP.h"
#include "private/objLibP.h"
#include "private/semLibP.h"
#include "private/windLibP.h"
#include "private/eventP.h"

/* optimized version available for 680X0, MIPS, I80X86, SH, */
/* COLDFIRE, ARM (excluding Thumb) */

#if (defined(PORTABLE) || \
     ((CPU_FAMILY != MC680X0) && \
      (CPU_FAMILY != MIPS) && \
      (CPU_FAMILY != I80X86) && \
      (CPU_FAMILY != SH) && \
      (CPU_FAMILY != COLDFIRE) && \
      (CPU_FAMILY != ARM)) || \
     ((CPU_FAMILY == ARM) && ARM_THUMB))
#define semBLib_PORTABLE
#endif

/* locals */

LOCAL BOOL	semBLibInstalled;		/* protect from muliple inits */


/*******************************************************************************
*
* semBLibInit - initialize the binary semaphore management package
* 
* SEE ALSO: semLibInit(1).
* NOMANUAL
*/

STATUS semBLibInit (void)

    {
    if (!semBLibInstalled)
	{
	semGiveTbl [SEM_TYPE_BINARY]		= (FUNCPTR) semBGive;
	semTakeTbl [SEM_TYPE_BINARY]		= (FUNCPTR) semBTake;
	semFlushTbl [SEM_TYPE_BINARY]		= (FUNCPTR) semQFlush;
	semGiveDeferTbl [SEM_TYPE_BINARY]	= (FUNCPTR) semBGiveDefer;
	semFlushDeferTbl [SEM_TYPE_BINARY]	= (FUNCPTR) semQFlushDefer;

	if (semLibInit () == OK)
	    semBLibInstalled = TRUE;
	}

    return ((semBLibInstalled) ? OK : ERROR);
    }

/*******************************************************************************
*
* semBCreate - create and initialize a binary semaphore
*
* This routine allocates and initializes a binary semaphore.  The semaphore
* is initialized to the <initialState> of either SEM_FULL (1) or SEM_EMPTY (0).
*
* The <options> parameter specifies the queuing style for blocked tasks.
* Tasks can be queued on a priority basis or a first-in-first-out basis.
* These options are SEM_Q_PRIORITY (0x1) and SEM_Q_FIFO (0x0), respectively.
* That parameter also specifies if semGive() should return ERROR when
* the semaphore fails to send events. This option is turned off by default;
* it is activated by doing a bitwise-OR of SEM_EVENTSEND_ERR_NOTIFY (0x10)
* with the queuing style of the semaphore.
*
*
* RETURNS: The semaphore ID, or NULL if memory cannot be allocated.
*/

SEM_ID semBCreate
    (
    int         options,                /* semaphore options */
    SEM_B_STATE initialState            /* initial semaphore state */
    )
    {
    SEM_ID semId;

    if ((!semBLibInstalled) && (semBLibInit () != OK))	/* initialize package */
	return (NULL);

    if ((semId = (SEM_ID) objAlloc (semClassId)) == NULL)
	return (NULL);

    /* initialize allocated semaphore */

    if (semBInit (semId, options, initialState) != OK)
	{
	objFree (semClassId, (char *) semId);
	return (NULL);
	}

#ifdef WV_INSTRUMENTATION
    /* windview - level 1 event logging */
    EVT_OBJ_3 (OBJ, semId, semClassId,
	       EVENT_SEMBCREATE, semId, options, semId->state);
#endif

    return (semId);
    }

/*******************************************************************************
*
* semBInit - initialize a declared binary semaphore
*
* The initialization of a static binary semaphore, or a binary semaphore
* embedded in some larger object need not deal with allocation.
* This routine may be called to initialize such a semaphore.  The semaphore
* is initialized to the specified initial state of either SEM_FULL or SEM_EMPTY.
*
* Binary semaphore options include the queuing style for blocked tasks.
* Tasks can be queued on the basis of their priority or first-in-first-out.
* These options are SEM_Q_PRIORITY and SEM_Q_FIFO respectively.
*
* SEE ALSO: semBCreate()
*
* NOMANUAL
*/

STATUS semBInit
    (
    SEMAPHORE  *pSemaphore,             /* pointer to semaphore to initialize */
    int         options,                /* semaphore options */
    SEM_B_STATE initialState            /* initial semaphore state */
    )
    {
    if ((!semBLibInstalled) && (semBLibInit () != OK))	/* initialize package */
	return (ERROR);

    if (semQInit (pSemaphore, options) != OK)		/* initialize queue */
	return (ERROR);

    return (semBCoreInit (pSemaphore, options, initialState));
    }

/*******************************************************************************
*
* semBCoreInit - initialize a binary semaphore with queue already initialized
*
* To initialize a semaphore with some special queuing algorithm, this routine
* is used.  This routine will initialize a binary semaphore without
* initializing the queue.
*
* ERRNO: S_semLib_INVALID_OPTION, S_semLib_INVALID_STATE
*
* NOMANUAL
*/

STATUS semBCoreInit
    (
    SEMAPHORE  *pSemaphore,             /* pointer to semaphore to initialize */
    int         options,                /* semaphore options */
    SEM_B_STATE initialState            /* initial semaphore state */
    )
    {
    if ((options & SEM_INVERSION_SAFE) || (options & SEM_DELETE_SAFE))
	{
	errno = S_semLib_INVALID_OPTION;
	return (ERROR);
	}

    switch (initialState)
	{
	case SEM_EMPTY:
	    pSemaphore->semOwner = taskIdCurrent;	/* semaphore empty */
	    break;
	case SEM_FULL:
	    pSemaphore->semOwner = NULL;		/* semaphore full */
	    break;
	default:
	    errno = S_semLib_INVALID_STATE;
	    return (ERROR);
	}

    pSemaphore->recurse = 0;				/* no recursive takes */
    pSemaphore->options = options;			/* stow away options */
    pSemaphore->semType = SEM_TYPE_BINARY;		/* type is binary */

    eventInit (&pSemaphore->events);			/* initialize events */

#ifdef WV_INSTRUMENTATION
    /* windview - connect instrumented class for level 1 event logging */
    if (wvObjIsEnabled)
        objCoreInit (&pSemaphore->objCore, semInstClassId);
    else 
#endif
    objCoreInit (&pSemaphore->objCore, semClassId); /* initialize core */

    return (OK);
    }

#ifdef semBLib_PORTABLE

/*******************************************************************************
*
* semBGive - give semaphore
*
* Gives the semaphore.  If a higher priority task has already taken
* the semaphore (so that it is now pended waiting for it), that task
* will now become ready to run, and preempt the task that does the semGive().
* If the semaphore is already full (it has been given but not taken) this
* call is essentially a no-op.
*
* NOMANUAL
*/

STATUS semBGive
    (
    SEM_ID semId        /* semaphore ID to give */
    )
    {
    int level = intLock ();			/* LOCK INTERRUPTS */
    WIND_TCB * pOwner;

    if (OBJ_VERIFY (semId, semClassId) != OK)
	{
	intUnlock (level);			/* UNLOCK INTERRUPTS */
	return (ERROR);
	}

    pOwner = semId->semOwner;

    if ((semId->semOwner = (WIND_TCB *) Q_FIRST (&semId->qHead)) == NULL)
	{
	int    	oldErrno;
	STATUS  evStatus;
	STATUS 	retStatus;

	retStatus = OK;

	/* check change of state; if so, send events */

	if ((semId->events.taskId != (int)NULL) && (pOwner != NULL))
	    {
	    kernelState = TRUE;
	    intUnlock (level);			/* UNLOCK INTERRUPTS */

	    oldErrno = errno;

	    evStatus = eventRsrcSend(semId->events.taskId,
				     semId->events.registered);

	    if (evStatus != OK)
		{
		if ((semId->options & SEM_EVENTSEND_ERR_NOTIFY) != 0x0)
		    {
		    retStatus = ERROR;
		    oldErrno = S_eventLib_EVENTSEND_FAILED;
		    }

		semId->events.taskId = (int)NULL;
		}
	    else if ((semId->events.options & EVENTS_SEND_ONCE) != 0x0)
		semId->events.taskId = (int)NULL;

	    windExit ();

	    errno = oldErrno;
	    return (retStatus);
	    }
	else
	    intUnlock (level);
	}
    else
	{
	kernelState = TRUE;			/* KERNEL ENTER */ 
	intUnlock (level);			/* UNLOCK INTERRUPTS */

#ifdef WV_INSTRUMENTATION
	/* windview - level 2 event logging */
	EVT_TASK_1 (EVENT_OBJ_SEMGIVE, semId);
#endif
	windPendQGet (&semId->qHead);		/* unblock a task */

	windExit ();				/* KERNEL EXIT */
	}

    return (OK);
    }

/*******************************************************************************
*
* semBTake - take semaphore
*
* Takes the semaphore.  If the semaphore is empty, i.e., it has not been given
* since the last semTake() or semInit(), this task will become pended until
* the semaphore becomes available by some other task doing a semGive()
* of it.  If the semaphore is already available, this call will empty
* the semaphore, so that no other task can take it until this task gives
* it back, and this task will continue running.
*
* WARNING
* This routine may not be used from interrupt level.
*
* NOMANUAL
*/

STATUS semBTake
    (
    SEM_ID semId,       /* semaphore ID to take */
    int timeout         /* timeout in ticks */
    )
    {
    int level;
    int status;

    if (INT_RESTRICT () != OK)
	return (ERROR);

again:
    level = intLock ();				/* LOCK INTERRUPTS */

    if (OBJ_VERIFY (semId, semClassId) != OK)
	{
	intUnlock (level);			/* UNLOCK INTERRUPTS */
	return (ERROR);
	}

    if (semId->semOwner == NULL)
	{
	semId->semOwner = taskIdCurrent;	/* update semaphore state */
	intUnlock (level);			/* UNLOCK INTERRUPTS */
	return (OK);
	}
    kernelState = TRUE;				/* KERNEL ENTER */
    intUnlock (level);				/* UNLOCK INTERRUPTS */

#ifdef WV_INSTRUMENTATION
    /* windview - level 2 event logging */
    EVT_TASK_1 (EVENT_OBJ_SEMTAKE, semId);
#endif

    if (windPendQPut (&semId->qHead, timeout) != OK)
	{
	windExit ();				/* KERNEL EXIT */
	return (ERROR);
	}

    if ((status = windExit ()) == RESTART)	/* KERNEL EXIT */
	{
	timeout = SIG_TIMEOUT_RECALC(timeout);
	goto again;
	}

    return (status);
    }

#endif	/* semBLib_PORTABLE */

/*******************************************************************************
*
* semBGiveDefer - give semaphore as deferred work
*
* Gives the semaphore.  If a higher priority task has already taken
* the semaphore (so that it is now pended waiting for it), that task
* will now become ready to run, and preempt the task that does the semGive().
* If the semaphore is already full (it has been given but not taken) this
* call is essentially a no-op.
*
* NOMANUAL
*/

void semBGiveDefer
    ( SEM_ID semId        /* semaphore ID to give */
    )
    {
    WIND_TCB * pOwner = semId->semOwner;

    if ((semId->semOwner = (WIND_TCB *) Q_FIRST (&semId->qHead)) != NULL)
	{

#ifdef WV_INSTRUMENTATION
    	/* windview - level 2 event logging */
	EVT_TASK_1 (EVENT_OBJ_SEMGIVE, semId);
#endif

	windPendQGet (&semId->qHead);		/* unblock a task */
	}
    else /* check for change of state, send events if registered */
	{
	if ((semId->events.taskId != (int)NULL) && (pOwner != NULL))
	    {
	    if (eventRsrcSend(semId->events.taskId,
			      semId->events.registered) != OK)
		{
		semId->events.taskId = (int)NULL;
		}
	    else if ((semId->events.options & EVENTS_SEND_ONCE) != 0x0 )
		semId->events.taskId = (int)NULL;
	    }
	}
    }

