/* semMLib.c - mutual-exclusion semaphore library */

/* Copyright 1984-2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
02k,03may02,pcm  removed possible NULL dereference in semMCreate () (SPR 76721)
02j,19nov01,bwa  Corrected doc for semMCreate return value (SPR #71921).
02i,09nov01,dee  add CPU_FAMILY != COLDFIRE in portable test
02h,30oct01,bwa  Fixed semMGive() behaviour.
02g,07sep01,bwa  Added VxWorks events support.
02f,04sep98,cdp  make ARM CPUs with ARM_THUMB==TRUE use portable routines.
02f,03mar00,zl   merged SH support into T2
02f,18dec00,pes  Correct compiler warnings
02e,17feb98,cdp  undo 02d: put ARM back in list of optimised CPUs.
02d,21oct97,kkk  undo 02c, take out ARM from list of optimized CPUs.
02c,20may97,jpd  added ARM to list of optimised CPUs.
02b,15oct96,dgp  doc: add caveats on priority inversion per SPR #6884
02a,24jun96,sbs  made windview instrumentation conditionally compiled
01z,22oct95,jdi  doc: added bit values for semMCreate() options (SPR 4276).
01y,19mar95,dvs  removed tron references.
01x,03feb95,rhp  doc: propagate warning about semaphore deletion from semLib
01w,09jun93,hdn  added a support for I80X86
01z,14apr94,smb  fixed class dereferencing for instrumentation macros
01y,15mar94,smb  modified instrumentation macros
01x,24jan94,smb  added instrumentation macros
01w,10dec93,smb  added instrumentation
01v,23jan93,jdi  documentation cleanup for 5.1.
01u,13nov92,jcf  semMCreate call semMLibInit for robustness.
01t,02oct92,jcf  made semMGiveForce() work if called by owner.
01s,28jul92,jcf  semM{Create,Init} call semMLibInit for robustness.
01r,28jul92,jcf  changed semMTakeKern to semMPendQPut;semMTake uses semMPendQPut
01q,18jul92,smb  Changed errno.h to errnoLib.h.
01p,09jul92,rrr  changed xsignal.h to private/sigLibP.h
01o,04jul92,jcf  added semMLibInit() to fill in semLib tables.
01n,26may92,rrr  the tree shuffle
01m,27apr92,rrr  added signal restarting
01l,15sep91,ajm  added MIPS to list of optimized CPU's
01k,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed includes to have absolute path from h/
		  -fixed #else and #endif
		  -changed copyright notice
01j,26sep91,hdn  added conditional flag for TRON optimized code.
01i,05apr91,jdi	 documentation -- removed header parens and x-ref numbers;
		 doc review by jcf.
01h,24mar91,jdi  documentation cleanup.
01g,16oct90,jcf  fixed priority inversion race in semMGive.
01f,05oct90,dnw  made semMInit() be NOMANUAL.
01e,30sep90,jcf  added semMGiveForce().
01d,03aug90,jcf  documentation.
01c,17jul90,dnw  changed to new objAlloc() call.
01b,05jul90,jcf  optimized version now available.
01a,20oct89,jcf  written based on v1b of semILib.
*/

/*
DESCRIPTION
This library provides the interface to VxWorks mutual-exclusion
semaphores.  Mutual-exclusion semaphores offer convenient options
suited for situations requiring mutually exclusive access to resources.
Typical applications include sharing devices and protecting data
structures.  Mutual-exclusion semaphores are used by many higher-level
VxWorks facilities.

The mutual-exclusion semaphore is a specialized version of the binary
semaphore, designed to address issues inherent in mutual exclusion, such
as recursive access to resources, priority inversion, and deletion safety.
The fundamental behavior of the mutual-exclusion semaphore is identical
to the binary semaphore (see the manual entry for semBLib), except for
the following restrictions:

    - It can only be used for mutual exclusion.
    - It can only be given by the task that took it.
    - It may not be taken or given from interrupt level.
    - The semFlush() operation is illegal.

These last two operations have no meaning in mutual-exclusion situations.

RECURSIVE RESOURCE ACCESS
A special feature of the mutual-exclusion semaphore is that it may be
taken "recursively," i.e., it can be taken more than once by the task that
owns it before finally being released.  Recursion is useful for a set of
routines that need mutually exclusive access to a resource, but may need
to call each other.

Recursion is possible because the system keeps track of which task
currently owns a mutual-exclusion semaphore.  Before being released, a
mutual-exclusion semaphore taken recursively must be given the same number
of times it has been taken; this is tracked by means of a count which is
incremented with each semTake() and decremented with each semGive().

The example below illustrates recursive use of a mutual-exclusion semaphore.
Function A requires access to a resource which it acquires by taking `semM';
function A may also need to call function B, which also requires `semM':
.CS
.ne 3
    SEM_ID semM;

    semM = semMCreate (...);

.ne 8
    funcA ()
	{
	semTake (semM, WAIT_FOREVER);
	...
	funcB ();
	...
	semGive (semM);
	}

.ne 6
    funcB ()
	{
	semTake (semM, WAIT_FOREVER);
	...
	semGive (semM);
	}
.CE

PRIORITY-INVERSION SAFETY
If the option SEM_INVERSION_SAFE is selected, the library adopts a
priority-inheritance protocol to resolve potential occurrences of
"priority inversion," a problem stemming from the use semaphores for
mutual exclusion.  Priority inversion arises when a higher-priority task
is forced to wait an indefinite period of time for the completion of a
lower-priority task.

Consider the following scenario:  T1, T2, and T3 are tasks of high,
medium, and low priority, respectively.  T3 has acquired some resource by
taking its associated semaphore.  When T1 preempts T3 and contends for the
resource by taking the same semaphore, it becomes blocked.  If we could be
assured that T1 would be blocked no longer than the time it normally takes
T3 to finish with the resource, the situation would not be problematic.
However, the low-priority task is vulnerable to preemption by medium-priority
tasks; a preempting task, T2, could inhibit T3 from relinquishing the resource.
This condition could persist, blocking T1 for an indefinite period of time.

The priority-inheritance protocol solves the problem of priority inversion
by elevating the priority of T3 to the priority of T1 during the time T1 is
blocked on T3.  This protects T3, and indirectly T1, from preemption by T2.
Stated more generally, the priority-inheritance protocol assures that
a task which owns a resource will execute at the priority of the highest
priority task blocked on that resource.  Once the task priority has been
elevated, it remains at the higher level until all mutual-exclusion semaphores
that the task owns are released; then the task returns to its normal, or 
standard, priority.  Hence, the "inheriting" task is protected from preemption 
by any intermediate-priority tasks.

The priority-inheritance protocol also takes into consideration a task's
ownership of more than one mutual-exclusion semaphore at a time.  Such a
task will execute at the priority of the highest priority task blocked on
any of its owned resources.  The task will return to its normal priority
only after relinquishing all of its mutual-exclusion semaphores that have
the inversion-safety option enabled.

SEMAPHORE DELETION
The semDelete() call terminates a semaphore and deallocates any
associated memory.  The deletion of a semaphore unblocks tasks pended
on that semaphore; the routines which were pended return ERROR.  Take
special care when deleting mutual-exclusion semaphores to avoid
deleting a semaphore out from under a task that already owns (has
taken) that semaphore.  Applications should adopt the protocol of only
deleting semaphores that the deleting task owns.

TASK-DELETION SAFETY
If the option SEM_DELETE_SAFE is selected, the task owning the semaphore
will be protected from deletion as long as it owns the semaphore.  This
solves another problem endemic to mutual exclusion.  Deleting a task
executing in a critical region can be catastrophic.  The resource could be
left in a corrupted state and the semaphore guarding the resource would be
unavailable, effectively shutting off all access to the resource.

As discussed in taskLib, the primitives taskSafe() and taskUnsafe()
offer one solution, but as this type of protection goes hand in hand with
mutual exclusion, the mutual-exclusion semaphore provides the option
SEM_DELETE_SAFE, which enables an implicit taskSafe() with each semTake(),
and a taskUnsafe() with each semGive().  This convenience is also more
efficient, as the resulting code requires fewer entrances to the kernel.

CAVEATS
There is no mechanism to give back or reclaim semaphores automatically when
tasks are suspended or deleted.  Such a mechanism, though desirable, is not
currently feasible.  Without explicit knowledge of the state of the guarded
resource or region, reckless automatic reclamation of a semaphore could
leave the resource in a partial state.  Thus if a task ceases execution
unexpectedly, as with a bus error, currently owned semaphores will not be
given back, effectively leaving a resource permanently unavailable.  The
SEM_DELETE_SAFE option partially protects an application, to the extent
that unexpected deletions will be deferred until the resource is released.

Because the priority of a task which has been elevated by the taking of a
mutual-exclusion semaphore remains at the higher priority until all
mutexes held by that task are released, unbounded priority inversion situations
can result when nested mutexes are involved.  If nested mutexes are required,
consider the following alternatives:

.iP 1. 4
Avoid overlapping critical regions.
.iP 2. 
Adjust priorities of tasks so that there are no tasks at intermediate
priority levels.
.iP 3. 
Adjust priorities of tasks so that priority inheritance protocol is not
needed.
.iP 4. 
Manually implement a static priority ceiling protocol using a 
non-inversion-save mutex.  This involves setting all blockers on a mutex 
to the ceiling priority, then taking the mutex.  After semGive, set the 
priorities back to the base priority. Note that this implementation 
reduces the queue to a fifo queue.
.LP

INTERNAL:
	WINDVIEW INSTRUMENTATION
Level 1:
	semMCreate() causes EVENT_SEMMCREATE
	semMGiveForce() causes EVENT_SEMMGIVEFORCE

Level 2:
	N/A

Level 3:
	N/A

INCLUDE FILES: semLib.h

SEE ALSO: semLib, semBLib, semCLib,
.pG "Basic OS"
*/

#include "vxWorks.h"
#include "errnoLib.h"
#include "taskLib.h"
#include "intLib.h"
#include "eventLib.h"
#include "private/eventLibP.h"
#include "private/sigLibP.h"
#include "private/objLibP.h"
#include "private/semLibP.h"
#include "private/windLibP.h"
#include "private/workQLibP.h"
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
#define semMLib_PORTABLE
#endif

/* globals */

int semMGiveKernWork;			/* parameter space for semMGiveKern() */

/* locals */

LOCAL BOOL	semMLibInstalled;	/* protect from muliple inits */


/*******************************************************************************
*
* semMLibInit - initialize the binary semaphore management package
* 
* SEE ALSO: semLibInit(1).
* NOMANUAL
*/

STATUS semMLibInit (void)

    {
    if (!semMLibInstalled)
	{
	semGiveTbl [SEM_TYPE_MUTEX]		= (FUNCPTR) semMGive;
	semTakeTbl [SEM_TYPE_MUTEX]		= (FUNCPTR) semMTake;

	if (semLibInit () == OK)
	    semMLibInstalled = TRUE;
	}

    return ((semMLibInstalled) ? OK : ERROR);
    }

/*******************************************************************************
*
* semMCreate - create and initialize a mutual-exclusion semaphore
*
* This routine allocates and initializes a mutual-exclusion semaphore.  The
* semaphore state is initialized to full.
*
* Semaphore options include the following:
* .iP "SEM_Q_PRIORITY  (0x1)" 8
* Queue pended tasks on the basis of their priority.
* .iP "SEM_Q_FIFO  (0x0)"
* Queue pended tasks on a first-in-first-out basis.
* .iP "SEM_DELETE_SAFE  (0x4)"
* Protect a task that owns the semaphore from unexpected deletion.  This
* option enables an implicit taskSafe() for each semTake(), and an implicit
* taskUnsafe() for each semGive().
* .iP "SEM_INVERSION_SAFE  (0x8)"
* Protect the system from priority inversion.  With this option, the task
* owning the semaphore will execute at the highest priority of the tasks
* pended on the semaphore, if it is higher than its current priority.  This
* option must be accompanied by the SEM_Q_PRIORITY queuing mode.
* .iP "SEM_EVENTSEND_ERR_NOTIFY (0x10)"
* When the semaphore is given, if a task is registered for events and the
* actual sending of events fails, a value of ERROR is returned and the errno
* is set accordingly. This option is off by default.
* .LP
*
* RETURNS: The semaphore ID, or NULL if the semaphore cannot be created.
*
* ERRNO
* .iP "S_semLib_INVALID_OPTION"
* Invalid option was passed to semMCreate.
* .iP "S_memLib_NOT_ENOUGH_MEMORY"
* Not enough memory available to create the semaphore.
* .LP
*
* SEE ALSO: semLib, semBLib, taskSafe(), taskUnsafe()
*/

SEM_ID semMCreate
    (
    int options                 /* mutex semaphore options */
    )
    {
    SEM_ID semId;

    if ((!semMLibInstalled) && (semMLibInit () != OK))	/* initialize package */
        return (NULL);

    if ((semId = (SEM_ID) objAlloc (semClassId)) == NULL)
	return (NULL);

    /* initialize allocated semaphore */

    if (semMInit (semId, options) != OK)
	{
	objFree (semClassId, (char *) semId);
	return (NULL);
	}

#ifdef WV_INSTRUMENTATION
    /* windview - level 1 event logging */
    EVT_OBJ_3 (OBJ, semId, semClassId,
	       EVENT_SEMMCREATE, semId, options, semId->state);
#endif

    return (semId);
    }

/*******************************************************************************
*
* semMInit - initialize a mutual-exclusion semaphore
*
* The initialization of a static mutual exclusion semaphore, or a mutual
* exclusion semaphore embedded in some larger object need not deal
* with allocation.  This routine may be called to initialize such a
* semaphore.  The semaphore state is initialized to the full state.
*
* Mutual exclusion semaphore options include the queuing style for blocked
* tasks.  Tasks may be queued on the basis of their priority or
* first-in-first-out.  These options are SEM_Q_PRIORITY and SEM_Q_FIFO
* respectively.
*
* The SEM_DELETE_SAFE option protects the task that currently owns the
* semaphore from unexpected deletion.  This option enables an implicit
* taskSafe() on each semTake(), and an implicit taskUnsafe() of every semGive().
*
* The SEM_INVERSION_SAFE option protects the system from priority inversion.
* This option must be accompanied by the SEM_Q_PRIORITY queuing mode.  With
* this option selected, the task owning the semaphore will execute at the
* highest priority of the tasks pended on the semaphore if it is higher than
* its current priority.
*
* RETURNS: OK, or ERROR if semaphore could not be initialized.
*
* ERRNO: S_semLib_INVALID_OPTION
*
* SEE ALSO: semLib, semBLib, semMCreate(), taskSafe(), taskUnsafe()
*
* NOMANUAL
*/

STATUS semMInit
    (
    SEMAPHORE * pSemaphore,     /* pointer to mutex semaphore to initialize */
    int         options         /* mutex semaphore options */
    )
    {
    if ((!semMLibInstalled) && (semMLibInit () != OK))	/* initialize package */
	return (ERROR);

    if ((options & SEM_INVERSION_SAFE) && ((options & SEM_Q_MASK)==SEM_Q_FIFO))
	{
	errno = S_semLib_INVALID_OPTION;
	return (ERROR);
	}

    if (semQInit (pSemaphore, options) != OK)
	return (ERROR);

    return (semMCoreInit (pSemaphore, options));
    }

/*******************************************************************************
*
* semMCoreInit - initialize a mutex semaphore with queue already initialized
*
* To initialize a semaphore with some special queuing algorithm, this routine
* is used.  This routine will initialize a mutual exclusion semaphore without
* initializing the queue.
*
* NOMANUAL
*/

STATUS semMCoreInit
    (
    SEMAPHORE   *pSemaphore,    /* pointer to mutex semaphore to initialize */
    int         options         /* mutex semaphore options */
    )
    {
    pSemaphore->semOwner = NULL;		/* no owner */
    pSemaphore->recurse	 = 0;			/* init take recurse count */
    pSemaphore->options	 = options;		/* initialize options */
    pSemaphore->semType  = SEM_TYPE_MUTEX;	/* type is mutex */

    eventInit (&pSemaphore->events);		/* initialize events */

#ifdef WV_INSTRUMENTATION
    /* windview - connect instrumented class for level 1 event logging */
    if (wvObjIsEnabled)
        objCoreInit (&pSemaphore->objCore, semInstClassId);
    else
        objCoreInit (&pSemaphore->objCore, semClassId);
#else
    objCoreInit (&pSemaphore->objCore, semClassId);
#endif

    return (OK);
    }

#ifdef semMLib_PORTABLE

/*******************************************************************************
*
* semMGive - give a semaphore
*
* Gives the semaphore.  If a higher priority task has already taken
* the semaphore (so that it is now pended waiting for it), that task
* will now become ready to run, and preempt the task that does the semGive().
* If the semaphore is already full (it has been given but not taken) this
* call is essentially a no-op.
*
* If deletion safe option is enabled, an implicit taskUnsafe() operation will
* occur.
*
* If priority inversion safe option is enabled, and this is the last priority
* inversion safe semaphore to be released, the calling task will revert to
* its normal priority.
*
* WARNING
* This routine may not be used from interrupt level.
*
* ERRNO: S_semLib_INVALID_OPERATION
*
* INTERNAL
* The use of the variables kernWork and semMGiveKernWork looks pretty lame.  It
* is, in fact, necessary to facilitate the optimized version of this routine.
* An optimized version would utilize a register for kernWork and stick the
* value in semMGiveKernWork as a parameter to semMGiveKern().  This form of
* parameter passing saves costly stack manipulation.
*
* NOMANUAL
*/

STATUS semMGive
    (
    FAST SEM_ID semId   /* semaphore ID to give */
    )
    {
    FAST int level;
    FAST int kernWork = 0;

    if (INT_RESTRICT () != OK)			/* restrict isr use */
	return (ERROR);

    level = intLock ();				/* LOCK INTERRUPTS */

    if (OBJ_VERIFY (semId, semClassId) != OK)	/* check validity */
	{
	intUnlock (level);
	return (ERROR);
	}

    if (taskIdCurrent != semId->semOwner)	/* check for ownership */
	{
	intUnlock (level);
	errnoSet (S_semLib_INVALID_OPERATION);
	return (ERROR);
	}

    if (semId->recurse > 0)			/* check recurse count */
	{
	semId->recurse --;			/* decrement recurse count */
	intUnlock (level);
	return (OK);
	}

    if ((semId->options & SEM_INVERSION_SAFE) &&
	(-- taskIdCurrent->priMutexCnt == 0))
        {
	if (taskIdCurrent->priority != taskIdCurrent->priNormal)
	    kernWork |= SEM_M_PRI_RESORT;
	}

    if ((semId->semOwner = (WIND_TCB *) Q_FIRST (&semId->qHead)) != NULL)
	kernWork |= SEM_M_Q_GET;
    else if (semId->events.taskId != (int)NULL)
	kernWork |= SEM_M_SEND_EVENTS;

    if ((semId->options & SEM_DELETE_SAFE) &&
 	(-- taskIdCurrent->safeCnt == 0) &&
	(Q_FIRST (&taskIdCurrent->safetyQHead) != NULL))
	kernWork |= SEM_M_SAFE_Q_FLUSH;

    if (kernWork == 0)
	{
	intUnlock (level);			/* UNLOCK INTERRUPTS */
	return (OK);
	}

    kernelState = TRUE;				/* KERNEL ENTER */
    intUnlock (level);				/* UNLOCK INTERRUPTS */

    semMGiveKernWork = kernWork;		/* setup the work to do */

    return (semMGiveKern (semId));		/* do the work */
    }

/*******************************************************************************
*
* semMTake - take a semaphore
*
* Takes the semaphore.  If the semaphore is empty, i.e., it has not been given
* since the last semTake() or semInit(), this task will become pended until
* the semaphore becomes available by some other task doing a semGive()
* of it.  If the semaphore is already available, this call will empty
* the semaphore, so that no other task can take it until this task gives
* it back, and this task will continue running.
*
* If deletion safe option is enabled, an implicit taskSafe() operation will
* occur.
*
* If priority inversion safe option is enabled, and the calling task blocks,
* and the priority of the calling task is greater than the semaphore owner,
* the owner will inherit the caller's priority.
*
* WARNING
* This routine may not be used from interrupt level.
*
* NOMANUAL
*/

STATUS semMTake
    (
    FAST SEM_ID semId,  /* semaphore ID to take */
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

	if (semId->options & SEM_INVERSION_SAFE)
	    taskIdCurrent->priMutexCnt ++;	/* update inherit count */

	if (semId->options & SEM_DELETE_SAFE)
	    taskIdCurrent->safeCnt ++;		/* update safety count */

	intUnlock (level);			/* UNLOCK INTERRUPTS */
	return (OK);
	}

    if (semId->semOwner == taskIdCurrent)	/* check for recursion */
	{
	semId->recurse ++;			/* keep recursion count */
	intUnlock (level);			/* UNLOCK INTERRUPTS */
	return (OK);
	}

    kernelState = TRUE;				/* KERNEL ENTER */
    intUnlock (level);				/* UNLOCK INTERRUPTS */
	
    if (semMPendQPut (semId, timeout) != OK)
	{
	windExit ();				/* windPendQPut failed */
	return (ERROR);
	}

    if ((status = windExit ()) == RESTART)	/* KERNEL EXIT */
	{
	timeout = SIG_TIMEOUT_RECALC(timeout);
	goto again;				/* we got signalled */
	}

    return (status);
    }

#endif	/* semMLib_PORTABLE */

/*******************************************************************************
*
* semMGiveForce - give a mutual-exclusion semaphore without restrictions
*
* This routine gives a mutual-exclusion semaphore, regardless of semaphore
* ownership.  It is intended as a debugging aid only.
*
* The routine is particularly useful when a task dies while holding some
* mutual-exclusion semaphore, because the semaphore can be resurrected.  The
* routine will give the semaphore to the next task in the pend queue or make
* the semaphore full if no tasks are pending.  In effect, execution will
* continue as if the task owning the semaphore had actually given the
* semaphore.
*
* CAVEATS
* This routine should be used only as a debugging aid, when the condition of
* the semaphore is known.
*
* RETURNS: OK, or ERROR if the semaphore ID is invalid.
*
* SEE ALSO: semGive()
*/

STATUS semMGiveForce
    (
    FAST SEM_ID semId   /* semaphore ID to give */
    )
    {
    STATUS status = OK;
    int oldErrno = errno;

    if (INT_RESTRICT () != OK)			/* restrict isr use */
	return (ERROR);
    if (OBJ_VERIFY (semId, semClassId) != OK)	/* check validity */
	return (ERROR);

#ifdef WV_INSTRUMENTATION
    /* windview - level 1 event logging */
    EVT_OBJ_3 (OBJ, semId, semClassId,
	       EVENT_SEMMGIVEFORCE, semId, semId->options, semId->state);
#endif

    /* if semaphore is not taken, nothing has to be done. */

    if (semId->semOwner == NULL)
	return OK;

    /* first see if we are giving away one of calling task's semaphores */

    if (semId->semOwner == taskIdCurrent)	/* give semaphore until avail */
	{
	while (semId->semOwner == taskIdCurrent)/* loop until recurse == 0 */
	    semGive (semId);			/* give semaphore */

	return (OK);				/* done */
	}

    /* give another task's semaphore away.  Djkstra be damned... */

    kernelState = TRUE;				/* KERNEL ENTER */
    semId->recurse = 0;				/* initialize recurse count */

    if ((semId->semOwner = (WIND_TCB *) Q_FIRST (&semId->qHead)) != NULL)
	{

#ifdef WV_INSTRUMENTATION
    	/* windview - level 2 event logging */
	EVT_TASK_1 (EVENT_OBJ_SEMGIVE, semId);
#endif

	windPendQGet (&semId->qHead);		/* unblock receiver */

	semId->semOwner->pPriMutex = NULL;	/* receiver no longer pended */

	if (semId->options & SEM_DELETE_SAFE)
	    semId->semOwner->safeCnt ++;	/* increment receiver safety */

	if (semId->options & SEM_INVERSION_SAFE)
	    semId->semOwner->priMutexCnt ++;	/* update inherit count */
	}
    else
	{
	if (semId->events.taskId != (int)NULL)	/* sem is free, send events */
	    {
	    if (eventRsrcSend (semId->events.taskId,
			       semId->events.registered) != OK)
		{
		if ((semId->options & SEM_EVENTSEND_ERR_NOTIFY) != 0x0)
		    {
		    status = ERROR;
		    }

		semId->events.taskId = (int)NULL;
		}
	    else if ((semId->events.options & EVENTS_SEND_ONCE) != 0x0)
		semId->events.taskId = (int)NULL;
	    }
	}

    if (status == ERROR)
	{
	windExit ();			/* KERNEL EXIT */
	errnoSet (S_eventLib_EVENTSEND_FAILED);
	}
    else
	{
	errnoSet (oldErrno);
	return (windExit ());		/* KERNEL EXIT */
	}

    return (status);
    }

/*******************************************************************************
*
* semMGiveKern - put current task on mutex semaphore pend queue
*
* This routine is called if something of consequence occured in giving the
* mutual exclusion semaphore that involved more lengthy processing.  Things
* of consequence are: task to be unblocked, priority to be uninherited, or
* task safety queue of deleters to be flushed.
*
* INTERNAL
* The use of the variables kernWork and semMGiveKernWork looks pretty lame.
* It is, in fact, necessary to facilitate the optimized version of this routine.
* An optimized version would utilize a register for kernWork and stick the
* value in semMGiveKernWork as a parameter to semMGiveKern().  This form of
* parameter passing saves costly stack manipulation.
*
* NOMANUAL
*/

STATUS semMGiveKern
    (
    FAST SEM_ID semId   /* semaphore ID to take */
    )
    {
    STATUS evStatus;
    STATUS retStatus = OK;
    int oldErrno = errno;

    if (semMGiveKernWork & SEM_M_Q_GET)
	{

#ifdef WV_INSTRUMENTATION
    	/* windview - level 2 event logging */
	EVT_TASK_1 (EVENT_OBJ_SEMGIVE, semId);
#endif

	windPendQGet (&semId->qHead);		/* unblock receiver */

	semId->semOwner->pPriMutex = NULL;	/* receiver no longer pended */

	if (semId->options & SEM_DELETE_SAFE)
	    semId->semOwner->safeCnt ++;	/* increment receiver safety */

	if (semId->options & SEM_INVERSION_SAFE)
	semId->semOwner->priMutexCnt ++;	/* update inherit count */
	}

    if (semMGiveKernWork & SEM_M_PRI_RESORT)
	{
	windPrioritySet (taskIdCurrent, taskIdCurrent->priNormal);
	}

    if (semMGiveKernWork & SEM_M_SAFE_Q_FLUSH)
	{

#ifdef WV_INSTRUMENTATION
    	/* windview - level 2 event logging */
	EVT_TASK_1 (EVENT_OBJ_SEMGIVE, semId);
#endif
	windPendQFlush (&taskIdCurrent->safetyQHead);
	}

    if ((semMGiveKernWork & SEM_M_SEND_EVENTS) != 0x0)	/* send events */
	{
	evStatus = eventRsrcSend (semId->events.taskId,
			          semId->events.registered);

	if (evStatus != OK)
	    {
	    if ((semId->options & SEM_EVENTSEND_ERR_NOTIFY) != 0x0)
		{
		oldErrno = S_eventLib_EVENTSEND_FAILED;
		retStatus = ERROR;
		}

	    semId->events.taskId = (int)NULL;
	    }
	else if ((semId->events.options & EVENTS_SEND_ONCE) != 0x0)
	    semId->events.taskId = (int)NULL;
	}

    if (retStatus == ERROR)
	{
	windExit ();
	errnoSet (oldErrno);
	}
    else
	{
	errnoSet (oldErrno);
	retStatus = windExit ();			/* KERNEL EXIT */
	}

    return (retStatus);
    }

/*******************************************************************************
*
* semMPendQPut - put current task on mutex semaphore pend queue
*
* This routine is called if the mutex semaphore is empty and the caller must
* pend.  It is called inside the kernel with kernelState == TRUE!  It is
* pulled out of semMTake so optimized versions need not bother with the
* following code.
*
* RETURNS: OK, or ERROR if windPendQPut failed.
* NOMANUAL
*/

STATUS semMPendQPut
    (
    FAST SEM_ID semId,  /* semaphore ID to take */
    int timeout         /* timeout in ticks */
    )
    {

#ifdef WV_INSTRUMENTATION
    /* windview - level 2 event logging */
    EVT_TASK_1 (EVENT_OBJ_SEMTAKE, semId);
#endif

    if (windPendQPut (&semId->qHead, timeout) != OK)
	return (ERROR);

    /* if taskIdCurrent is of a higher priority than the task that owns
     * this semaphore, reprioritize the owner.
     */

    if (semId->options & SEM_INVERSION_SAFE)
	{
	taskIdCurrent->pPriMutex = semId;	/* track mutex we pend on */

        if (taskIdCurrent->priority < semId->semOwner->priority)
	    {
	    windPrioritySet (semId->semOwner, taskIdCurrent->priority);
	    }
	}

    return (OK);
    }
