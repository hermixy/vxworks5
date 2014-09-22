/* windLib.c - internal VxWorks kernel library */

/* Copyright 1984-2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
02a,22may02,jgn  updated tick counter to be 64 bit - SPR #70255
01z,20mar02,bwa  windPendQRemove() now moves task out of the tick Q. (SPR
                 #74673).
01y,18dec00,pes  Correct compiler warnings
01x,29oct97,cth  removed references to scrPad for WV2.0
01w,03dec96,dbt  Added intLock and intUnlock in windWdStart to protect the
                 decrement of the defer start counter (fixed spr #7070).
01v,24jun96,sbs  made windview instrumentation conditionally compiled
01t,31jan95,tmk  added OBJ_VERIFY clause to windSemDelete()
	   +rrr
01s,27jan95,ms   windDestroy flushes the right saftey queue now
	   +rrr
01u,02feb94,smb  windPendQFlush no longer causes scratch-pad overflow SPR #2981
01t,24jan94,smb  added tid to EVENT_WINDTICKTIMEOUT
01s,10dec93,smb  added instrumentation
01r,13nov92,dnw  added include of smObjLib.h
01q,19jul92,pme  Added windReadyQPut, made windDelete and windPendQRemove
                 return STATUS, made windTickAnnounce set task return
                 value acording to possible shared memory objects errors. 
01p,04jul92,jcf  private headers.
01n,05jun92,ajm  intCnt is now fixed, see excALib.s
01o,26may92,rrr  the tree shuffle
01n,15oct91,ajm  intCnt change for mips, this must be FIXED correctly soon
01m,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed includes to have absolute path from h/
		  -changed VOID to void
		  -changed copyright notice
01l,28sep90,jcf  documentation.
01k,04sep90,jcf  documentation.
01j,03aug90,jcf  fixed bug in watchdog routine calling wdStart.
01i,15jul90,jcf  fixed bug in watchdog handling which allowed work q to overflow
01h,03jul90,jcf  added windPendQTerminate().
		 added timer queue rollover algorithm.
01g,26jun90,jcf  simplified priority inheritance algorithm.
		 changed windSem*() to windPendQ*().
01f,28aug89,jcf  modified to support version 2.0 of wind.
01e,09aug89,gae  SPARCintized windDummyCreate.
01d,19nov88,jcf  validated task id before operations undertaken.
		   changed lstFirst, lstPrevious, lstNext to macros.
01c,23aug88,gae  documentation.
01b,12aug88,jcf	 cleanup.
01a,12mar88,jcf	 written.
*/

/*
DESCRIPTION
The VxWorks kernel provides tasking control services to an application.  The
libraries kernelLib, taskLib, semLib, tickLib, and wdLib comprise the kernel
functionality.  This library is internal to the VxWorks kernel and contains
no user callable routines.

INTERNAL
This is a description of the architecture of Wind version 2.0.  The information
here applies to the following libraries: kernelLib, taskLib, semLib, tickLib,
wdLib, schedLib, workQLib, windLib, windALib, semALib, and workQALib.

STATE MACHINE
The kernel is a small state machine.  Each task contains state information in
the status field.  The possible states in wind 2.0 are:

    WIND_READY,				Ready for execution, in readyQHead.
    WIND_DELAY,				Sleeping, in tickQHead.
    WIND_PEND,				Blocked, in resource pendQHead.
    WIND_PEND | WIND_DELAY,		Blocked w/timeout in pendQHead/tickQHead
    WIND_SUSPEND,			Suspended, in no queue.
    WIND_DELAY | WIND_SUSPEND,		Suspended sleeping, in tickQHead
    WIND_PEND | WIND_SUSPEND,		Suspended blocked, in pendQHead.
    WIND_PEND|WIND_DELAY|WIND_SUSPEND,  Susp blocked w/tout, pendQHead/tickQHead
    WIND_DEAD				Extinct, gone, deceased, an Ex-Task.

We provide a rich set of routines to move routines from state to another in
the aforementioned libraries.  The actual work of changing a task's state and
shuffling it into the appropriate queue is done within this library as a
lower level call to the user routines elsewhere.  For instance taskSuspend()
calls windSuspend().  This decomposition is not purely aesthetic, the separation
is necessary as part of the mutual exclusion method used by the kernel.

MUTUAL EXCLUSION
By the time we arrive in the routines located here, higher level routines have
obtained mutual exclusion to all kernel queues by entering kernel state.  The
routines located here have free reign over any kernel data structure.  Kernel
state is a powerful means of mutual exclusion, unfortunately preemption is
disabled for the duration of kernel state.  High preemptive latency undermines
the reactive latency of a real time system, so it is crucial to use this
mechanism sparingly.  Indeed the basic design philosophy of the kernel is to
keep it as minimal as possible, while still capable of supporting higher level
facilities.

Kernel routines fit the template illuminated by taskResume():

    if (kernelState)				/@ defer work if in kernel @/
	{
	workQAdd1 (windResume, tid);		/@ add work to kernel work q @/
	return (OK);
	}

    kernelState = TRUE;				/@ KERNEL ENTER @/
    windResume ((WIND_TCB *) tid);		/@ resume task @/
    windExit ();				/@ KERNEL EXIT @/

To enter kernel state one simply sets the global variable kernelState to TRUE.
From that point, all kernel data structures are protected from contention.
When finished with the kernel operation, kernel state is left via windExit()
which sets kernelState back to FALSE.  Lets consider who, in fact, contends
for the internal data structures.  An interrupt service routine is the only
possible initiator of additional kernel work.  That is, once a system enters
kernel state the only way an application could request more kernel work is
from an ISR.

WORK QUEUE
Notice in the above template that kernelState is always checked before it
is set.  This is the backbone of the mutual exclusion.  The kernel uses a
technique of work deferral as a means of mutual exclusion.  When kernelState
is TRUE, the function of interest is not invoked directly, but rather
the work is deferred by enqueueing the job to the kernel work queue.
The kernel work queue is emptied by windExit(), (or intExit in the case of an
ISR entering the kernel first), before loading the context of the appropriate
task.  At the very end of kernel state we lock interrupts, check the work
queue is still empty, and enter a selected task's context.  The deferral of
kernel work is necessary for all kernel routines which are ISR callable.

SPECIAL PROBLEMS
Some facilities are quite elaborate and require special discussions.  Most are
areas still under research or involve some form of tricky code.

WATCHDOGS
Watchdogs present two ugly problems.  The first problem is the routine wdStart()
takes four parameters: the watchdog id, the timeout, the routine, and a routine
parameter.  A work queue job only has room for 3 parameters.  So if an ISR
calls wdStart() after interrupting the kernel, we cannot simply defer the
work to the work queue.  The solution is to write windWdStart to take only
two parameters, the watchdog id, and a timeout.  The routine and parmeter
are stored in the watchdog itself.  We're not out of the woods yet, because
what if five different ISRs which interrupt the kernel perform a wdStart on the
same watchdog?  Where do we keep all the parameters?  The answer is to simply
clobber the existing routine and parameter with the new routine and new
parameter and bump a defer count variable within the watchdog.  Every time
windWdStart() is called as kernel work (or otherwise), the defer count is
decremented.  If the resulting defer count is zero, the watchdog is sorted to
the appropriate timeout in the tick queue.  We know that the timeout
corresponds to the watchdog routine and parameter resident in the watchdog
structure because the defer count has been decremented as many times as it
was incremented.

The second problem is even uglier.  Watchdogs routines may be any arbitrary
routine including kernel routines.  The problem is watchdog routines are
now called from deep within the kernel, in kernel state.  All kernel routines
made from a watchdog will be deferred to the work queue.  The kicker is that
there is no limit to the number of watchdogs that can go off in the same tick.
If all kernel work is deferred by a limitless number of watchdog routines
expiring at the same tick, there is a chance that we will overflow the kernel
work queue.  The work queue is implemented as circular queue with a capacity
of 64 jobs.  The solution to this problem is to empty the work queue after
each watchdog routine is called.  We assume that a watchdog routine will never
make more than 64 kernel calls.

Watchdogs are being rethought.  They were not initially considered as part
of the kernel, but to make them utilize the tick queue, they got dragged in.
As is evident from the problems outlined above, its not a marriage made in
heaven.  They are serviceable, but a future version of VxWorks will probably
elect to demote watchdog routines to execute at task level within the context
of a dedicated alarm task.

PRIORITY INHERITANCE
Priority inversion arises when a higher-priority task is forced to wait an
indefinite period of time for the completion of a lower-priority task.
Consider the following scenario: T1, T2, and T3 are tasks of high, medium,
and low priority, respectively.  T3 has acquired some resource by taking
its associated semaphore.  When T1 preempts T3 and contends for the
resource by taking the same semaphore, it becomes blocked.  If we could be
assured that T1 would be blocked no longer than the time it normally takes
T3 to finish with the resource, the situation would not be particularly
problematic.  After all the resource is non-preemptible.  However, the
low-priority task is vulnerable to preemption by medium-priority tasks; a
preempting task, T2, will inhibit T3 from relinquishing the resource.  This
condition could persist, blocking T1 for an indefinite period of time.

A priority inheritance protocal solves the problem of priority inversion by
elevating the priority of T3 to the priority of T1 during the time T1 is
blocked on T3.  This protects T3, and indirectly T1, from preemption by
T2.  Stated more generally, the priority inheritance protocol assures that
a task which owns a resource will execute at the priority of the highest
priority task blocked on that resource.  When execution is complete, the
task gives up the resource and returns to its normal, or standard,
priority.  Hence, the "inheriting" task is protected from preemption by any
intermediate-priority tasks.

Special circumstances apply to nested priority inheritance.  When a task
owns multiple resources guarded from priority inversion, the task's priority
will `ratchet up' as higher priority tasks contend for the resources.  For
efficiency and simplicity of implementation, task priorities are only allowed
to be lowered when a task owns no resources guarded from priority inversion.
This rule is useful for situations where a high priority task contends for
an owned resource, the priority is inheritted by the owner, but then the high
priority task is deleted.  Strictly speaking, it would be fair game to lower
the resource owner back to its original priority, but the implemenation is
a collosal waste of time and space.  The small price to pay is that tasks
may inherit a priority for a little longer than is actually necessary but I
won't tell anyone if you won't.

DELETION PROBLEM
The deletion problem has two prongs.  The first is that a resource
may be deleted right ought from under a task.  Nothing protects a task
from a semaphore, for example, from being deleted by another task.  We expect
that applications will discipline object deletion to take place safely.  In
the case of a semaphore, for example, the semaphore should be taken by the
deleter before deletion.  Also note, that the actual invalidation of an
object must be done with mutual exclusion.  Objects which may be utilized
from interrupt level must, therefore, protect the object invalidation with
interrupt locks.

Object reference counts were considered as an alternative, the difficulty
lies in the fact that objects are shared in VxWorks without prior consent
from the operating system.  Some OS require tasks to bind to an object by
name before gaining the capability of access.  Object binding does not fit
with current thinking on the matter, but as we explore
distributed/multiprocessor extensions to VxWorks, object id binding may
resurface.

The second prong of the deletion problem is harder.  It is possible that
a task may be deleted while engaged in some operation on some resource.  This
could leave the resource in an unusable, unavailable state.  Tasks are a
special class of object, an active object which engages passive objects.  The
deletion of an active object must account for the passive objects currently
engaged.  We solve this problem in the same way we approach kernel work
deferral.  The deletion of a task is deferred until the task is safe to delete.
A deleter will block while a task is protected from deletion, otherwise a
deleter might get the false impression that the task deleted is acutually gone.
Applications must assist the kernel in determining when a task should be
protected from deletion.  The routine taskSafe() and taskUnsafe() are the
basic operations controlling task deletion protection.  For convenience, a
mutual exclusion semaphore type is available which has an implicit taskSafe()
call on a semTake(), and an implicit taskUnsafe() call on semGive().  Resource
deletion protection by use of mutual exclusion semaphores is strong step
towards making VxWorks truly capable of dynamic object deletion.


FILE STRUCTURE
                                     ---------
                                     |semBLib|
			             |semMLib|
                                     |semCLib|
                U  S  E  R           |semOLib|  I  N  T  E  R  F  A  C  E
   ----------- ----------- --------- -.-.-.-.- ------- ------------ ---------
   |kernelLib| |<msgQLib>| |taskLib| |semLib | |wdLib| |<eventLib>| |tickLib|
   ----------- ----------- --------- -.-.-.-.- ------- ------------ ---------
        |           |          |     |semALib|    |         |           |
        |           |          |     ---------    |         |           |
        |           |          |         |        |         |           |
        \           \          \     ---------    /         /           /
         ==========================> |windLib| <========================
         |           |          |    ---------    |         |          |
         |           |          |                 |         |          |
         |           |          |                 |         |          |
         \           \          \    ----------   /         /          /
          =========================> |windALib| <======================
			             |schedLib|
			             ----------


ROUTINE STRUCTURE
			    taskDelete	       taskSpawn
                                |              /      |
			        |        taskCreat    |
                                |            /        |
			 taskTerminate  taskInit  taskActivate
                                |           |         |
 taskPrioritySet@* taskDelay* taskDestroy*  |    taskResume@*
   |                   |         |          |         |
   \  tickAnnounce@*   |         |          |         |       taskSuspend@*
    \     \            |         |          |         |          /
     \	   \       windDelay windDelete windSpawn windResume    /
      \     \	                                               /
       \  windTickAnnounce               *             windSuspend
        \		                 |
  windPriNormalSet--windPrioritySet   windExit	        windPendQPut-semTake*
           			 	 |   \	                    \msgQRecv*
 wdStart@*                               |    \
    \                                    |     \         windPendQGet-semGive@*
     \----windWdStart                reschedule \                    \msgQSend@*
                                         |   \   |
 wdCancel@*                    @         |    \  |     windPendQFlush-semFlush@*
     \------windWdCancel       |         |     \ |              \msgQBroadcast@*
			    workQAdd#    |     workQDoWork
				     loadContext

.CS

windview INSTRUMENTATION
------------------------

        LEVEL 1 N/A
        LEVEL 2
                windSpawn causes EVENT_WINDSPAWN
                windDelete causes EVENT_WINDDELETE
                windSuspend causes EVENT_WINDSUSPEND
                windResume causes EVENT_WINDRESUME
                windPrioritySet causes EVENT_WINDPRIORITYSETLOWER
                windPrioritySet causes EVENT_WINDPRIORITYSETRAISE
                windSemDelete causes EVENT_WINDSEMDELETE
                windTickAnnounce causes EVENT_WINDTICKUNDELAY
                windTickAnnounce causes EVENT_WINDTICKANNOUNCETMRWD
                windTickAnnounce causes EVENT_WINDTICKANNOUNCETMRSLC
                windTickAnnounce causes EVENT_WINDTICKANNOUNCETMR
                windDelay causes EVENT_WINDDELAY
                windUndelay causes EVENT_WINDUNDELAY
                windWdStart causes EVENT_WINDWDSTART
                windWdCancel causes EVENT_WINDWDCANCEL
                windPendQGet causes EVENT_WINDPENDQGET
                windPendQFlush causes EVENT_WINDPENDQFLUSH
                windPendQPut causes EVENT_WINDPENDQPUT
                windPendQTerminate causes EVENT_WINDPENDQTERMINATE

        LEVEL 3 N/A
.CE

SEE ALSO: "Basic OS", windALib
*/

#include "vxWorks.h"
#include "tickLib.h"
#include "taskArchLib.h"
#include "semLib.h"
#include "smLib.h"
#include "wdLib.h"
#include "errno.h"
#include "qFifoLib.h"
#include "qFifoGLib.h"
#include "intLib.h"
#include "smObjLib.h"
#include "private/eventP.h"
#include "private/funcBindP.h"
#include "private/objLibP.h"
#include "private/windLibP.h"
#include "private/taskLibP.h"
#include "private/workQLibP.h"
#include "private/kernelLibP.h"


/* global variables */

BOOL kernelState;			/* mutex to enter kernel state */
BOOL kernelIsIdle;			/* boolean reflecting idle state */

/* shared memory objects globals */

int  smObjPoolMinusOne;      /* set to smObj pool local address - 1 */
int  localToGlobalOffset;    /* localAdrs - globalAdrs */

/*******************************************************************************
*
* windSpawn - create a task and leave it in the suspend state
*
* This routine inserts the specified tcb into the active queue.
* A newly created task is initially in the suspended state.
*
* NOMANUAL
*/
void windSpawn
    (
    FAST WIND_TCB *pTcb         /* address of new task's tcb */
    )
    {

#ifdef WV_INSTRUMENTATION
    /* windview - level 2 event logging */
    EVT_TASK_2 (EVENT_WINDSPAWN, (int) pTcb, pTcb->priority); 
#endif

    Q_PUT (&activeQHead, &pTcb->activeNode, FIFO_KEY_TAIL);	/* in active q*/
    }

/*******************************************************************************
*
* windDelete - delete a task
*
* Delete task and reorganize any queue the task was in.
* Assumes that the task owns no inheritance semaphores.
*
* NOMANUAL
*/

STATUS windDelete
    (
    FAST WIND_TCB *pTcb         /* address of task's tcb */
    )
    {
    FAST USHORT mask;
    FAST int ix;
    int  status = OK;		/* status return by windPendQRemove */

#ifdef WV_INSTRUMENTATION
    /* windview - level 2 event logging */
    EVT_TASK_1 (EVENT_WINDDELETE, (int) pTcb); 	/* log event */
#endif

    if (pTcb->status == WIND_READY)			/* if task is ready */
	Q_REMOVE (&readyQHead, pTcb);			/* remove from queue */
    else
	{
	if (pTcb->status & WIND_PEND)			/* if task is pended */
            status = windPendQRemove (pTcb);            /* remove from queue */

	if (pTcb->status & WIND_DELAY)			/* if task is delayed */
	    Q_REMOVE (&tickQHead, &pTcb->tickNode);	/* remove from queue */
	}

    /* disconnect all the swap in hooks the task has connected to */

    for (ix = 0, mask = pTcb->swapInMask; mask != 0; ix++, mask = mask << 1)
	if (mask & 0x8000)
	   taskSwapReference [ix] --;

    /* disconnect all the swap out hooks the task has connected to */

    for (ix = 0, mask = pTcb->swapOutMask; mask != 0; ix++, mask = mask << 1)
	if (mask & 0x8000)
	   taskSwapReference [ix] --;

    Q_REMOVE (&activeQHead, &pTcb->activeNode);		/* deactivate it */

    pTcb->status = WIND_DEAD;		       		/* kill it */

    return (status);
    }

/*******************************************************************************
*
* windSuspend - suspend a task
*
* Suspension is an additive state.  When a task is on the ready queue it
* is removed and changed to suspended.  Otherwise, the status is updated
* to include suspended in addition to the state it is already in.
*
* NOMANUAL
*/

void windSuspend
    (
    FAST WIND_TCB *pTcb         /* address of task's tcb, 0 = current task */
    )
    {

#ifdef WV_INSTRUMENTATION
    /* windview - level 2 event logging */
    EVT_TASK_1 (EVENT_WINDSUSPEND, (int) pTcb); 	/* log event */
#endif

    if (pTcb->status == WIND_READY)
	Q_REMOVE (&readyQHead, pTcb);

    pTcb->status |= WIND_SUSPEND;		/* update status */
    }

/*******************************************************************************
*
* windResume - resume a task
*
* Resume the specified task and place in the ready queue if necessary.
*
* NOMANUAL
*/

void windResume
    (
    FAST WIND_TCB *pTcb         /* task to resume */
    )
    {

#ifdef WV_INSTRUMENTATION
    /* windview - level 2 event logging */
    EVT_TASK_1 (EVENT_WINDRESUME, (int) pTcb); 	/* log event */
#endif

    if (pTcb->status == WIND_SUSPEND)			/* just suspended so */
	Q_PUT (&readyQHead, pTcb, pTcb->priority);	/* put in ready queue */

    pTcb->status &= ~WIND_SUSPEND;		/* mask out the suspend state */
    }

/*******************************************************************************
*
* windPriNormalSet - set the normal priority of a task
*
* Set the normal priority of a task.  A task will execute at its normal priority
* as long as no priority inheritance has taken place.
*
* NOMANUAL
*/

void windPriNormalSet
    (
    WIND_TCB    *pTcb,          /* address of task's tcb */
    UINT         priNormal      /* new normal priority */
    )
    {
    pTcb->priNormal = priNormal;	/* update normal priority */

    windPrioritySet (pTcb, priNormal);	/* resort to appropriate priority */
    }

/*******************************************************************************
*
* windPrioritySet - resort the specified task to the appropriate priority
*
* This routine sets the actual priority of task, taking into account priority
* inversion safety.  If a task owns any priority inversion safety semaphores
* then the priority will not be allowed to lower.
*
* NOMANUAL
*/

void windPrioritySet
    (
    FAST WIND_TCB *pTcb,        /* address of task's tcb */
    FAST UINT      priority     /* new priority */
    )
    {
    /* if lowering the current priority and the task doesn't own any mutex
     * semaphores with inheritance, then go ahead and lower current priority.
     */

    if ((pTcb->priMutexCnt == 0) && (pTcb->priority < priority))
	{

#ifdef WV_INSTRUMENTATION
        /* windview - level 2 event logging */
	EVT_TASK_3 (EVENT_WINDPRIORITYSETLOWER, (int) pTcb, 
		pTcb->priority, priority);
#endif

	pTcb->priority = priority;		/* lower current priority */

	if (pTcb->status == WIND_READY)		/* task is in ready Q? */
	    {
	    Q_RESORT (&readyQHead, pTcb, priority);
	    }
	else if (pTcb->status & WIND_PEND)	/* task is in pend Q? */
	    {
	    Q_RESORT (pTcb->pPendQ, pTcb, priority);
	    }

	return;					/* no inheritance if lowering */
	}

    while (pTcb->priority > priority)		/* current priority too low? */
	{
#ifdef WV_INSTRUMENTATION
        /* windview - level 2 event logging */
	EVT_TASK_3 (EVENT_WINDPRIORITYSETRAISE, (int) pTcb, 
		pTcb->priority, priority);
#endif

	pTcb->priority = priority;		/* raise current priority */

	if (pTcb->status == WIND_READY)		/* task is in ready Q? */
	    {
	    Q_RESORT (&readyQHead, pTcb, priority);
	    }
	else if (pTcb->status & WIND_PEND)	/* task is in pend Q? */
	    {
	    Q_RESORT (pTcb->pPendQ, pTcb, priority);

	    if (pTcb->pPriMutex != NULL)	/* chain up for inheritance */
		pTcb = pTcb->pPriMutex->semOwner;
	    }
	}
    }

/*******************************************************************************
*
* windSemDelete - delete a semaphore
*
* Delete a semaphore and unblock any tasks pended on it.  Unblocked tasks
* will be returned ERROR.
*
* NOMANUAL
*/

void windSemDelete
    (
    FAST SEM_ID semId   /* semaphore to delete */
    )
    {
    FAST WIND_TCB *pTcb;

#ifdef WV_INSTRUMENTATION
    /* windview - level 2 event logging */
    EVT_TASK_1 (EVENT_WINDSEMDELETE, (int) semId);      /* log event */
#endif

    windPendQTerminate (&semId->qHead);			/* terminate pend q */

    if ((semId->semType == SEM_TYPE_MUTEX) &&		/* mutex semaphore? */
	((pTcb = semId->semOwner) != NULL) &&		/* is there an owner? */
        (OBJ_VERIFY (&((WIND_TCB *)(pTcb))->objCore, taskClassId) == OK))
	{
	if ((semId->options & SEM_INVERSION_SAFE) &&	/* inversion safe opt?*/
	    (-- pTcb->priMutexCnt == 0) && 		/* last mutex owned? */
	    (pTcb->priority != pTcb->priNormal))	/* priority inherited?*/
	    {
	    windPrioritySet (pTcb, pTcb->priNormal);	/* back to normal pri */
	    }

	if ((semId->options & SEM_DELETE_SAFE) &&	/* deletion safe opt? */
	    ((-- pTcb->safeCnt) == 0) &&		/* unsafe now? */
	    (Q_FIRST (&pTcb->safetyQHead) != NULL))	/* deleters pended? */
	    {
	    windPendQFlush (&pTcb->safetyQHead); /* flush deleters */
	    }
	}
    }

/*******************************************************************************
*
* windTickAnnounce - acknowledge the passing of time
*
* Process delay list.  Make tasks at the end of their delay, ready.
* Perform round robin scheduling if selected.
* Call any expired watchdog routines.
*
* NOMANUAL
*/

void windTickAnnounce (void)
    {
    FAST Q_NODE *	pNode;	/* node of tick queue */
    FAST WIND_TCB *	pTcb;	/* pointer to task control block */
    FAST WDOG *		wdId;	/* pointer to a watchdog */
    FUNCPTR		wdRtn;	/* watch dog routine to invoke */
    int			wdArg;	/* watch dog argument to pass with wdRtn */
    int 		status;	/* status return by Q_REMOVE */

    /* advance and manage the tick queue */

    vxTicks ++;						/* advance rel time */
    vxAbsTicks++;					/* advance abs time */

    Q_ADVANCE (&tickQHead);				/* advance tick queue */

    while ((pNode = (Q_NODE *) Q_GET_EXPIRED (&tickQHead)) != NULL)
	{
	pTcb = (WIND_TCB *) ((int)pNode - OFFSET (WIND_TCB, tickNode));

	if  (
	    (pTcb->objCore.pObjClass == taskClassId)	/* is it a task? */
#ifdef WV_INSTRUMENTATION 
	    || (pTcb->objCore.pObjClass == taskInstClassId)
#endif
	    )
	    { pTcb->status &= ~WIND_DELAY;		/* out of delay state */

	    if (pTcb->status == WIND_READY)		/* taskDelay done */
		{

#ifdef WV_INSTRUMENTATION
    		/* windview - level 2 event logging */
		EVT_TASK_1 (EVENT_WINDTICKUNDELAY, (int) pTcb); 
#endif

		taskRtnValueSet (pTcb, OK);		/* return OK */
		}
	    else if (pTcb->status & WIND_PEND)		/* semaphore timeout */
		{

                /*
                 * if the task was pended on a shared semaphore
                 * windPendQRemove can return :
                 * ERROR if lock cannot be taken
                 * ALREADY_REMOVED if the shared tcb is already removed
                 * from the pend Q on the give side but has not
                 * shows up on this side.
                 */

#ifdef WV_INSTRUMENTATION
		 EVT_TASK_1 (EVENT_WINDTICKTIMEOUT, (int )pTcb); 
#endif

                status = windPendQRemove (pTcb);        /* out of semaphore q */

                switch (status)
                    {
                    case ALREADY_REMOVED:
                        {
                        /* the semaphore was given in time, return OK */
                        taskRtnValueSet (pTcb, OK);
                        break;
                        }
                    case ERROR:
                        {
                        taskRtnValueSet (pTcb, ERROR);      /* return ERROR */
                        pTcb->errorStatus = S_smObjLib_LOCK_TIMEOUT;
                        break;
                        }
                    default:
                        {
                        taskRtnValueSet (pTcb, ERROR);  /* return ERROR */
                        pTcb->errorStatus = S_objLib_OBJ_TIMEOUT;
                        }
                    }
		}

	    if (pTcb->status == WIND_READY)		/* if ready, enqueue */
		Q_PUT (&readyQHead, pTcb, pTcb->priority);
	    }
	else						/* must be a watchdog */
	    {
	    wdId = (WDOG *) ((int)pNode - OFFSET (WDOG, tickNode));

#ifdef WV_INSTRUMENTATION
    	    /* windview - level 2 event logging */
	    EVT_TASK_1 (EVENT_WINDTICKANNOUNCETMRWD, (int) wdId);
#endif
	    wdId->status = WDOG_OUT_OF_Q;		/* mark as out of q */

	    /* We get the watch dog routine and argument before testing
	     * deferStartCnt to avoid having to lock interrupts.  The RACE
	     * to worry about is a wdStart () or wdCancel () occuring after
	     * the test of deferStartCnt and before invoking the routine.
	     */

	    wdRtn = wdId->wdRoutine;			/* get watch dog rtn */
	    wdArg = wdId->wdParameter;			/* get watch dog arg */

	    intCnt ++;                                  /* fake ISR context */

	    if (wdId->deferStartCnt == 0)               /* check validity */
		(* wdRtn) (wdArg);                      /* invoke wdog rtn */
            intCnt --;                                  /* unfake ISR context */

	    workQDoWork ();				/* do deferred work */
	    }
	}

    /* perform round robin scheduling if selected */

    if ((roundRobinOn) && (taskIdCurrent->lockCnt == 0) &&
	(taskIdCurrent->status == WIND_READY) &&
	(++ taskIdCurrent->tslice >= roundRobinSlice))
	{

#ifdef WV_INSTRUMENTATION
    	/* windview - level 2 event logging */
	EVT_TASK_0 (EVENT_WINDTICKANNOUNCETMRSLC);	/* log event */
#endif

	taskIdCurrent->tslice = 0;			/* reset time slice  */
	Q_REMOVE (&readyQHead, taskIdCurrent);		/* no Q_RESORT here! */
	Q_PUT (&readyQHead, taskIdCurrent, taskIdCurrent->priority);
	}

    }

/*******************************************************************************
*
* windDelay - put running task asleep for specified ticks
*
* Insert task in delay queue at correct spot dictated by the specified duration.
*
* RETURNS: OK
*
* NOMANUAL
*/

STATUS windDelay
    (
    FAST int timeout
    )
    {

#ifdef WV_INSTRUMENTATION
    /* windview - level 2 event logging */
    EVT_TASK_1 (EVENT_WINDDELAY, timeout); 	/* log event */
#endif

    Q_REMOVE (&readyQHead, taskIdCurrent);		/* out of ready queue */

    if ((unsigned)(vxTicks + timeout) < vxTicks)	/* rollover? */
	{
	Q_CALIBRATE (&tickQHead, ~vxTicks + 1);
	vxTicks = 0;
	}

    Q_PUT (&tickQHead, &taskIdCurrent->tickNode, timeout + vxTicks);
    taskIdCurrent->status |= WIND_DELAY;		/* set delay status */

    return (OK);
    }

/*******************************************************************************
*
* windUndelay - wake a sleeping task up
*
* Remove task from the delay queue and return to it ERROR.
*
* RETURNS: OK
*
* NOMANUAL
*/

STATUS windUndelay
    (
    WIND_TCB *pTcb
    )
    {
    if ((pTcb->status & WIND_DELAY) == 0)	/* in not delayed then done */
	return (OK);

#ifdef WV_INSTRUMENTATION
    /* windview - level 2 event logging */
    EVT_TASK_1 (EVENT_WINDUNDELAY, (int) pTcb); 	/* log event */
#endif

    pTcb->status &= ~WIND_DELAY;		/* undelay status */

    Q_REMOVE (&tickQHead, &pTcb->tickNode);	/* remove from tick queue */

    taskRtnValueSet (pTcb, ERROR);		/* return ERROR */
    pTcb->errorStatus = S_taskLib_TASK_UNDELAYED;

    if (pTcb->status == WIND_READY)		/* if ready, enqueue */
	Q_PUT (&readyQHead, pTcb, pTcb->priority);

    return (OK);
    }

/*******************************************************************************
*
* windWdStart - start a watchdog timer
*
* This routine will start or restart a watchdog.  If the watchdog is already
* in the queue, it is resorted to the appropriate timeout.  If there are
* more windWdStart() jobs still in the kernel work queue, as counted by the
* variable deferStartCnt, then no action is taken.
*
* RETURNS: OK.
*
* NOMANUAL
*/

STATUS windWdStart
    (
    WDOG *wdId,         /* watch dog to start */
    int   timeout       /* timeout in ticks */
    )
    {
    int deferStartCnt;	/* temp variable to hold deferStartCnt */
    int level; 

#ifdef WV_INSTRUMENTATION
    /* windview - level 2 event logging */
    EVT_TASK_1 (EVENT_WINDWDSTART, (int) wdId); 	/* log event */
#endif

    /* Protect the decrement from interrupts to avoid the race condition
       that spr 7070 describes */

    level = intLock ();
	deferStartCnt = -- wdId->deferStartCnt;
    intUnlock (level);

    if (deferStartCnt == 0)
	{
	if ((unsigned)(vxTicks + timeout) < vxTicks)	/* rollover? */
	    {
	    Q_CALIBRATE (&tickQHead, ~vxTicks + 1);
	    vxTicks = 0;
	    }

	if (wdId->status == WDOG_IN_Q)			/* resort if in queue */
	    Q_RESORT (&tickQHead, &wdId->tickNode, timeout + vxTicks);
	else
	    Q_PUT (&tickQHead, &wdId->tickNode, timeout + vxTicks);

	wdId->status = WDOG_IN_Q;
	}

    return (OK);
    }

/*******************************************************************************
*
* windWdCancel - cancel a watchdog timer
*
* This routine cancels a watchdog, removing it from the timer queue if needed.
*
* NOMANUAL
*/

void windWdCancel
    (
    WDOG *wdId          /* watch dog to cancel */
    )
    {

#ifdef WV_INSTRUMENTATION
    /* windview - level 2 event logging */
    EVT_TASK_1 (EVENT_WINDWDCANCEL, (int) wdId); 	/* log event */
#endif 

    if (wdId->status == WDOG_IN_Q)
	{
	Q_REMOVE (&tickQHead, &wdId->tickNode);	/* remove from queue */
	wdId->status = WDOG_OUT_OF_Q;
	}
    }

/*******************************************************************************
*
* windPendQGet - remove and next task on pend queue and make ready
*
* Take the next task on the specified pend queue and unpend it.  It will either
* be ready or suspended.  If ready it is added to the ready queue.
*
* NOMANUAL
*/

void windPendQGet
    (
    Q_HEAD *pQHead      /* pend queue to get first task off */
    )
    {
    FAST WIND_TCB *pTcb = (WIND_TCB *) Q_GET (pQHead);

#ifdef WV_INSTRUMENTATION
    /* windview - level 2 event logging */
    EVT_TASK_1 (EVENT_WINDPENDQGET, (int) pTcb); 	/* log event */
#endif

    pTcb->status &= ~WIND_PEND;			/* clear pended flag */
    taskRtnValueSet (pTcb, OK);			/* return OK */

    if (pTcb->status & WIND_DELAY)		/* task was timing out */
	{
	pTcb->status &= ~WIND_DELAY;
	Q_REMOVE (&tickQHead, &pTcb->tickNode);	/* remove from queue */
	}

    if (pTcb->status == WIND_READY)		/* task is now ready */
	Q_PUT (&readyQHead, pTcb, pTcb->priority);
    }

/*******************************************************************************
*
* windReadyQPut - put a task on the ready queue
*
* This routine adds a task previously blocked on a shared
* semaphore to the ready Queue.
*
* The shared TCB of this task has been removed from the shared semaphore
* pending queue by the CPU that gives the semaphore.
*
* NOMANUAL
*/

void windReadyQPut
    (
    WIND_TCB * pTcb              /* TCB to add to ready Q */
    )
    {

    pTcb->status &= ~WIND_PEND;                 /* clear pended flag */
    taskRtnValueSet (pTcb, OK);                 /* return OK */

    if (pTcb->status & WIND_DELAY)              /* task was timing out */
        {
        pTcb->status &= ~WIND_DELAY;
        Q_REMOVE (&tickQHead, &pTcb->tickNode); /* remove from queue */
        }

    if (pTcb->status == WIND_READY)             /* task is now ready */
        Q_PUT (&readyQHead, pTcb, pTcb->priority);
    }

/*******************************************************************************
*
* windReadyQRemove - remove pend task from the ready Q
*
* This routines removes the current task from the pend. The task must have
* been previously added to a shared semaphore pend Q by using  Q_PUT.
* If the task is timing out, also add the task to the delay queue. 
*
* RETURNS: N/A
*
* NOMANUAL
*/

void windReadyQRemove
    (
    Q_HEAD *pQHead,        /* pend queue to put taskIdCurrent on */
    int    timeout         /* timeout in ticks */
    )
    {

    Q_REMOVE (&readyQHead, taskIdCurrent);              /* out of ready q */

    taskIdCurrent->status |= WIND_PEND;                 /* update status */

    taskIdCurrent->pPendQ = pQHead;                     /* pQHead pended on */
    if (timeout != WAIT_FOREVER)                        /* timeout specified? */
        {
        if ((unsigned)(vxTicks + timeout) < vxTicks)    /* rollover? */
            {
            Q_CALIBRATE (&tickQHead, ~vxTicks + 1);
            vxTicks = 0;
            }

        Q_PUT (&tickQHead, &taskIdCurrent->tickNode, timeout + vxTicks);
        taskIdCurrent->status |= WIND_DELAY;
        }
    }

/*******************************************************************************
*
* windPendQFlush - flush all tasks off of pend queue making them ready
*
* Take all of the tasks on the specified pend queue and unpend them.  They will
* either be ready or suspended.  All ready tasks will be added to the ready
* queue.
*
* NOMANUAL
*/

void windPendQFlush
    (
    Q_HEAD *pQHead      /* pend queue to flush tasks from */
    )
    {
    FAST WIND_TCB *pTcb;

#if FALSE
#ifdef WV_INSTRUMENTATION
    int level = 0;
#endif
#endif /* FALSE */

    while ((pTcb = (WIND_TCB *) Q_GET (pQHead)) != NULL)/* get next task */
	{

#ifdef WV_INSTRUMENTATION
	/* windview - level 2 event logging */
	EVT_TASK_1 (EVENT_WINDPENDQFLUSH, (int) pTcb); 	
#endif

	pTcb->status &= ~WIND_PEND;			/* clear pended flag */
	taskRtnValueSet (pTcb, OK);			/* return OK */

	if (pTcb->status & WIND_DELAY)			/* task is timeout */
	    {
	    pTcb->status &= ~WIND_DELAY;
	    Q_REMOVE (&tickQHead, &pTcb->tickNode);	/* remove from queue */
	    }
	if (pTcb->status == WIND_READY)			/* task is now ready */
	    Q_PUT (&readyQHead, pTcb, pTcb->priority);
	}
    }

/*******************************************************************************
*
* windPendQPut - add current task to a pend queue
*
* Add a taskIdCurrent to a pend queue.  If the task is timing out, also add the
* task to the delay queue.  If the NO_WAIT value is passed for a timeout,
* return ERROR.
*
* RETURNS: OK, or ERROR if NO_WAIT timeout is specified.
*
* ERRNO: S_objLib_OBJ_UNAVAILABLE
*
* NOMANUAL
*/

STATUS windPendQPut
    (
    FAST Q_HEAD *pQHead,        /* pend queue to put taskIdCurrent on */
    FAST int    timeout         /* timeout in ticks */
    )
    {
    if (timeout == NO_WAIT)				/* NO_WAIT = no block */
	{
	errno = S_objLib_OBJ_UNAVAILABLE;		/* resource gone */
	return (ERROR);
	}

#ifdef WV_INSTRUMENTATION
    /* windview - level 2 event logging */
    EVT_TASK_0 (EVENT_WINDPENDQPUT); 	/* log event */
#endif

    Q_REMOVE (&readyQHead, taskIdCurrent);		/* out of ready q */

    taskIdCurrent->status |= WIND_PEND;			/* update status */

    taskIdCurrent->pPendQ = pQHead;			/* pQHead pended on */

    Q_PUT (pQHead, taskIdCurrent, taskIdCurrent->priority);

    if (timeout != WAIT_FOREVER)			/* timeout specified? */
	{
	if ((unsigned)(vxTicks + timeout) < vxTicks)	/* rollover? */
	    {
	    Q_CALIBRATE (&tickQHead, ~vxTicks + 1);
	    vxTicks = 0;
	    }

	Q_PUT (&tickQHead, &taskIdCurrent->tickNode, timeout + vxTicks);
	taskIdCurrent->status |= WIND_DELAY;
	}

    return (OK);
    }

/*******************************************************************************
*
* windPendQRemove - move a task out of the pend queue
*
* This routine removes a task from a pend queue.
*
* RETURNS: OK
*
* NOMANUAL
*/

STATUS windPendQRemove
    (
    WIND_TCB *pTcb              /* task to remove from pend queue */
    )
    {
    int status = Q_REMOVE (pTcb->pPendQ, pTcb);	/* out of pend queue */

    /*
     * When shared semaphore are used Q_REMOVE can return ERROR if lock
     * cannot be taken, or ALREADY_REMOVED if the shared tcb is currently
     * in the event list, ie removed from the shared semaphore pend Q.
     */

    pTcb->status   &= ~WIND_PEND;		/* turn off pend bit */
    pTcb->pPriMutex = NULL;			/* clear pPriMutex */

    if (pTcb->status & WIND_DELAY)		/* task was timing out */
	{
	pTcb->status &= ~WIND_DELAY;
	Q_REMOVE (&tickQHead, &pTcb->tickNode);	/* remove from queue */
	}

    return (status);
    }

/*******************************************************************************
*
* windPendQTerminate - flush all tasks off of pend queue making them ready
*
* Take all of the tasks on the specified pend queue and unpend them.  They will
* either be ready or suspended.  All ready tasks will be added to the ready
* queue.  Unlike windPendQFlush (2), this routine returns the ERROR
* S_objLib_OBJ_DELETED.
*
* NOMANUAL
*/

void windPendQTerminate
    (
    Q_HEAD *pQHead      /* pend queue to terminate */
    )
    {
    FAST WIND_TCB *pTcb;

    while ((pTcb = (WIND_TCB *) Q_GET (pQHead)) != NULL)/* get next task */
	{

#ifdef WV_INSTRUMENTATION
        /* windview - level 2 event logging */
	EVT_TASK_1 (EVENT_WINDPENDQTERMINATE, (int) pTcb);
#endif

	pTcb->status     &= ~WIND_PEND;			/* clear pended flag */
	pTcb->pPriMutex	  = NULL;			/* clear mutex pend */
	pTcb->errorStatus = S_objLib_OBJ_DELETED;	/* set error status */

	taskRtnValueSet (pTcb, ERROR);			/* return ERROR */

	if (pTcb->status & WIND_DELAY)			/* task is timeout */
	    {
	    pTcb->status &= ~WIND_DELAY;
	    Q_REMOVE (&tickQHead, &pTcb->tickNode);	/* remove from queue */
	    }

	if (pTcb->status == WIND_READY)			/* task is now ready */
	    Q_PUT (&readyQHead, pTcb, pTcb->priority);
	}
    }
