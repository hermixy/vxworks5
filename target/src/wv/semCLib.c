/* semCLib.c - counting semaphore library */

/* Copyright 1984-2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
02e,03may02,pcm  removed possible NULL dereference in semCCreate () (SPR 76721)
02d,09nov01,dee  add CPU_FAMILY != COLDFIRE in portable test
02c,18oct01,bwa  Fixed problem when the semaphore could be deleted while
                 sending events.
02b,07sep01,bwa  Added VxWorks events support.
02a,04sep98,cdp  make ARM CPUs with ARM_THUMB==TRUE use portable routines.
02a,03mar00,zl   merged SH support into T2
01z,17feb98,cdp  undo 01y: put ARM back in list of optimised CPUs.
01y,21oct97,kkk  undo 01x, take out ARM from list of optimized CPUs.
01x,20may97,jpd  added ARM to list of optimised CPUs.
01w,24jun96,sbs  made windview instrumentation conditionally compiled
01v,22oct95,jdi  doc: added bit values for semCCreate() options (SPR 4276).
01u,19mar95,dvs  removing tron references.
01t,09jun93,hdn  added a support for I80X86
01w,14apr94,smb  fixed class dereferencing for instrumentation macros
01v,15mar94,smb  modified instrumentation macros
01u,24jan94,smb  added instrumentation macros
01t,10dec93,smb  added instrumentation
01s,20jan93,jdi  documentation cleanup for 5.1.
01r,13nov92,jcf  semCCreate call semCLibInit for robustness.
01q,28jul92,jcf  semC{Create,Init} call semCLibInit for robustness.
01p,09jul92,rrr  changed xsignal.h to private/sigLibP.h
01o,04jul92,jcf  added semCLibInit() to fill in semLib tables.
01n,26may92,rrr  the tree shuffle
01m,27apr92,rrr  added signal restarting
01l,15sep91,ajm  added MIPS to list of optimized CPU's
01k,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed includes to have absolute path from h/
		  -fixed #else and #endif
		  -changed VOID to void
		  -changed copyright notice
01j,26sep91,hdn  added conditional flag for TRON optimized code.
01i,05apr91,jdi	 documentation -- removed header parens and x-ref numbers;
		 doc review by jcf.
01h,24mar91,jdi  documentation cleanup.
01g,05oct90,dnw  made semCInit() be NOMANUAL.
01f,29aug90,jcf  documentation.
01e,03aug90,jcf  documentation.
01d,17jul90,dnw  changed to new objAlloc() call.
01c,05jul90,jcf  optimized version now available.
01b,26jun90,jcf  merged into one semaphore class.
01a,20oct89,jcf  written based on v1g of semLib.
*/

/*
DESCRIPTION
This library provides the interface to VxWorks counting
semaphores.  Counting semaphores are useful for guarding multiple
instances of a resource.

A counting semaphore may be viewed as a cell in memory whose contents
keep track of a count.  When a task takes a counting semaphore, using
semTake(), subsequent action depends on the state of the count:
.IP (1) 4
If the count is non-zero, it is decremented and the calling task
continues executing.
.IP (2)
If the count is zero, the task will be blocked, pending the availability
of the semaphore.  If a timeout is specified and the timeout expires, the
pended task will be removed from the queue of pended tasks and enter the
ready state with an ERROR status.  A pended task is ineligible for CPU
allocation.  Any number of tasks may be pended simultaneously on the same
counting semaphore.
.LP
When a task gives a semaphore, using semGive(), the next available task in
the pend queue is unblocked.  If no task is pending on this semaphore, the
semaphore count is incremented.  Note that if a semaphore is given, and a
task is unblocked that is of higher priority than the task that called
semGive(), the unblocked task will preempt the calling task.

A semFlush() on a counting semaphore will atomically unblock all pended
tasks in the semaphore queue.  So all tasks will be made ready before any
task actually executes.  The count of the semaphore will remain unchanged.

INTERRUPT USAGE
Counting semaphores may be given but not taken from interrupt level.

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
	semCCreate() causes EVENT_SEMCCREATE

Level 2 (for portable only):
	semGive() causes EVENT_OBJ_SEMGIVE
	semTake() causes EVENT_OBJ_SEMTAKE

Level 3:
	N/A

INCLUDE FILES: semLib.h

SEE ALSO: semLib, semBLib, semMLib,
.pG "Basic OS"
*/

/* LINTLIBRARY */

#include "vxWorks.h"
#include "classLib.h"
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
#define semCLib_PORTABLE
#endif

/* locals */

LOCAL BOOL	semCLibInstalled;		/* protect from muliple inits */


/*******************************************************************************
*
* semCLibInit - initialize the binary semaphore management package
* 
* SEE ALSO: semLibInit(1).
* NOMANUAL
*/

STATUS semCLibInit (void)

    {
    if (!semCLibInstalled)
	{
	semGiveTbl [SEM_TYPE_COUNTING]		= (FUNCPTR) semCGive;
	semTakeTbl [SEM_TYPE_COUNTING]		= (FUNCPTR) semCTake;
	semFlushTbl [SEM_TYPE_COUNTING]		= (FUNCPTR) semQFlush;
	semGiveDeferTbl [SEM_TYPE_COUNTING]	= (FUNCPTR) semCGiveDefer;
	semFlushDeferTbl [SEM_TYPE_COUNTING]	= (FUNCPTR) semQFlushDefer;

	if (semLibInit () == OK)
	    semCLibInstalled = TRUE;
	}

    return ((semCLibInstalled) ? OK : ERROR);
    }

/*******************************************************************************
*
* semCCreate - create and initialize a counting semaphore
*
* This routine allocates and initializes a counting semaphore.  The
* semaphore is initialized to the specified initial count.
*
* The <options> parameter specifies the queuing style for blocked tasks.
* Tasks may be queued on a priority basis or a first-in-first-out basis.
* These options are SEM_Q_PRIORITY (0x1) and SEM_Q_FIFO (0x0), respectively.
* That parameter also specifies if semGive() should return ERROR when
* the semaphore fails to send events. This option is turned off by default;
* it is activated by doing a bitwise-OR of SEM_EVENTSEND_ERR_NOTIFY (0x10) 
* with the queuing style of the semaphore.
*
* RETURNS: The semaphore ID, or NULL if memory cannot be allocated.
*/

SEM_ID semCCreate
    (
    int         options,                /* semaphore option modes */
    int         initialCount            /* initial count */
    )
    {
    SEM_ID semId;

    if ((!semCLibInstalled) && (semCLibInit () != OK))	/* initialize package */
	return (NULL);

    if ((semId = (SEM_ID) objAlloc (semClassId)) == NULL)
	return (NULL);

    /* initialize allocated semaphore */

    if (semCInit (semId, options, initialCount) != OK)
	{
	objFree (semClassId, (char *) semId);
	return (NULL);
	}

#ifdef WV_INSTRUMENTATION
    /* windview - level 1 event logging */
    EVT_OBJ_3 (OBJ, semId, semClassId,
	       EVENT_SEMCCREATE, semId, options, initialCount);
#endif

    return (semId);
    }

/*******************************************************************************
*
* semCInit - initialize a declared counting semaphore
*
* The initialization of a static counting semaphore, or a counting semaphore
* embedded in some larger object need not deal with allocation.
* This routine may be called to initialize such a semaphore.  The semaphore
* is initialized to the specified initial count.
*
* Counting semaphore options include the queuing style for blocked tasks.
* Tasks may be queued on the basis of their priority or first-in-first-out.
* These options are SEM_Q_PRIORITY and SEM_Q_FIFO respectively.
*
* SEE ALSO: semCCreate
*
* NOMANUAL
*/

STATUS semCInit
    (
    SEMAPHORE   *pSemaphore,            /* pointer to semaphore to init */
    int         options,                /* semaphore options */
    int         initialCount            /* initial count */
    )
    {
    if ((!semCLibInstalled) && (semCLibInit () != OK))	/* initialize package */
	return (ERROR);

    if (semQInit (pSemaphore, options) != OK)		/* initialize queue */
	return (ERROR);

    return (semCCoreInit (pSemaphore, options, initialCount));
    }

/*******************************************************************************
*
* semCCoreInit - initialize a counting semaphore with queue already initialized
*
* To initialize a semaphore with some special queuing algorithm, this routine
* is used.  This routine will initialize a counting semaphore without
* initializing the queue.
*
* ERRNO: S_semLib_INVALID_OPTION
*
* NOMANUAL
*/

STATUS semCCoreInit
    (
    SEMAPHORE   *pSemaphore,            /* pointer to semaphore to init */
    int         options,                /* semaphore options */
    int         initialCount            /* initial count */
    )
    {
    if ((options & SEM_INVERSION_SAFE) || (options & SEM_DELETE_SAFE))
	{
	errno = S_semLib_INVALID_OPTION;
	return (ERROR);
	}

    pSemaphore->semCount = initialCount;		/* initialize count */
    pSemaphore->recurse  = 0;				/* no recursive takes */
    pSemaphore->options  = options;			/* stow away options */
    pSemaphore->semType	 = SEM_TYPE_COUNTING;		/* type is counting */

    eventInit (&pSemaphore->events);			/* initialize events */

    /* initialize the semaphore object core information */

#ifdef WV_INSTRUMENTATION
    /* windview - connect instrumented class for level 1 event logging */
    if (wvObjIsEnabled) objCoreInit (&pSemaphore->objCore, semInstClassId);
    else
#endif
    objCoreInit (&pSemaphore->objCore, semClassId);

    return (OK);
    }

#ifdef semCLib_PORTABLE

/*******************************************************************************
*
* semCGive - give a semaphore
*
* Gives the semaphore.  If a higher priority task has already taken
* the semaphore (so that it is now pended waiting for it), that task
* will now become ready to run, and preempt the task that does the semGive().
* If the semaphore is already full (it has been given but not taken) this
* call is essentially a no-op.
*
* NOMANUAL
*/

STATUS semCGive
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
	int    	oldErrno;
	STATUS  evStatus;
	STATUS 	retStatus;

	semId->semCount ++;			/* give semaphore */

	retStatus = OK;

	if (semId->events.taskId != (int)NULL)
	    {
	    kernelState = TRUE;
	    intUnlock (level);

	    oldErrno = errno;

	    evStatus = eventRsrcSend (semId->events.taskId,
				      semId->events.registered);

	    if(evStatus != OK)
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
* semCTake - take a semaphore
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

STATUS semCTake
    (
    SEM_ID semId,       /* semaphore ID to take */
    int timeout         /* timeout in ticks */
    )
    {
    int level;
    int status;

    if (INT_RESTRICT () != OK)			/* restrict isr use */
	return (ERROR);

again:
    level = intLock ();				/* LOCK INTERRUPTS */

    if (OBJ_VERIFY (semId, semClassId) != OK)
	{
	intUnlock (level);			/* UNLOCK INTERRUPTS */
	return (ERROR);
	}

    if (semId->semCount > 0)
	{
	semId->semCount --;			/* take semaphore */
	intUnlock (level);			/* UNLOCK INTERRUPTS */
	return (OK); }
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

#endif	/* semCLib_PORTABLE */

/*******************************************************************************
*
* semCGiveDefer - give a semaphore as deferred work
*
* Gives the semaphore.  If a higher priority task has already taken
* the semaphore (so that it is now pended waiting for it), that task
* will now become ready to run, and preempt the task that does the semGive().
* If the semaphore is already full (it has been given but not taken) this
* call is essentially a no-op.
*
* NOMANUAL
*/

void semCGiveDefer
    (
    SEM_ID semId        /* semaphore ID to give */
    )
    {
     if (Q_FIRST (&semId->qHead) == NULL)		/* anyone blocked? */
	{
	semId->semCount ++;				/* give semaphore */

	/* sem is free, send events if registered */
	if (semId->events.taskId != (int)NULL)
	    {
	    if (eventRsrcSend (semId->events.taskId,
			       semId->events.registered) != OK)
		{
		semId->events.taskId = (int)NULL;
		return;
		}

	    if ((semId->events.options & EVENTS_SEND_ONCE) == EVENTS_SEND_ONCE)
		semId->events.taskId = (int)NULL;
	    }
	}
     else
	{

#ifdef WV_INSTRUMENTATION
        /* windview - level 2 event logging */
	EVT_TASK_1 (EVENT_OBJ_SEMGIVE, semId);
#endif

	windPendQGet (&semId->qHead);			/* unblock a task */
	}
    }

