/* eventLib.c - VxWorks events library */

/* Copyright 2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01d,09nov01,aeg  changed eventPendQ to global symbol vxEventPendQ for WindSh;
		 fixed problem in eventStart() where a re-registration would
		 not delete the previous one.
01c,30oct01,bwa  Corrected use of windview instrumentation for task-state
                 transition logging. Added eventRsrcSend().
01b,25oct01,tcr  remove #undef WV_INSTRUMENTATION
01a,20sep01,bwa  written
*/

/*
DESCRIPTION
Events are a means of communication between tasks and interrupt routines,
based on a synchronous model. Only tasks can receive events, and both tasks
and ISRs can send them.

Events are similar to signals in that they are directed at one task but differ
in the fact that they are synchronous in nature. Thus, the receiving task must
pend when waiting for events to occur. Also, unlike signals, a handler is not
needed since, when wanted events are received, the pending task continues its
execution (like after a call to msgQReceive() or semTake()).

Each task has its own events field that can be filled by having tasks (even
itself) and/or ISRs sending events to the task. Each event's meaning is
different for every task. Event X when received can be interpreted differently
by separate tasks. Also, it should be noted that events are not accumulated.
If the same event is received several times, it counts as if it were received
only once. It is not possible to track how many times each event has been sent 
to a task.

There are some VxWorks objects that can send events when they become available.
They are referred to as 'resources' in the context of events. They include
semaphores and message queues. For example, when a semaphore becomes free,
events can be sent to a task that asked for it.

INTERNAL:
	WINDVIEW INSTRUMENTATION
Level 1:
	eventSend () causes EVENT_EVENTSEND
	eventReceive() causes EVENT_EVENTRECEIVE

Level 2:
	eventSend () causes EVENT_OBJ_EVENTSEND
	eventReceive () causes EVENT_OBJ_EVENTRECEIVE

Level 3:
	N/A

INCLUDE FILES: eventLib.h

SEE ALSO: taskLib, semLib, semBLib, semCLib, semMLib, msgQLib,
<VxWorks Programmer's Guide: Basic OS>
*/

/* includes */

#include "vxWorks.h"
#include "qLib.h"
#include "taskLib.h"
#include "intLib.h"
#include "errnoLib.h"
#include "eventLib.h"
#include "taskArchLib.h"
#include "private/workQLibP.h"
#include "private/eventLibP.h"
#include "private/kernelLibP.h"
#include "private/windLibP.h"
#include "private/sigLibP.h"

/* function prototypes */

LOCAL void eventPendQRemove (WIND_TCB * pTcb);

/* globals */

Q_HEAD vxEventPendQ;

#ifdef WV_INSTRUMENTATION
VOIDFUNCPTR eventEvtRtn; /* windview - level 1 event logging */
#endif

/*******************************************************************************
*
* eventLibInit - Initialize events library
*
* Initialize the pending queue that will be used for all event operations.
* This routine is called automatically when the events module is included.
*
* NOMANUAL
*/

void eventLibInit (void)
    {
    qInit (&vxEventPendQ, Q_FIFO);
    }

/*******************************************************************************
*
* eventInit - initializes event-related info for a resource
*
* Initializes event-related data for a given resource. It shouldn't be used
* outside of resource initialization.
*
* NOMANUAL
*/

void eventInit
    (
    EVENTS_RSRC * evRsrc
    )
    {
    evRsrc->taskId	= (int)NULL;
    evRsrc->registered	= 0x0;
    evRsrc->options	= 0x0;
    }

/*******************************************************************************
*
* eventTerminate - Terminate event-related operations for a resource
*
* Wakes up task waiting on events that should be sent from a resource.
*
* WARNING: - This function shouldn't be used outside of resource destruction.
*          - Function must be called with kernelState == TRUE
*
* NOMANUAL
*/

void eventTerminate
    (
    const EVENTS_RSRC * evRsrc
    )
    {
    WIND_TCB * pTcb = (WIND_TCB *)evRsrc->taskId;

    if (pTcb != NULL)
	{
	int level;

	/*
	 * must intLock to prevent an ISR from doing an eventSend that also
	 * wakes up the task.
	 */

	level = intLock ();

	if (TASK_ID_VERIFY(pTcb) != OK)
	    {
	    intUnlock (level);
	    return;
	    }

	/* only wakeup task if it is waiting */

	if (pTcb->events.sysflags & EVENTS_SYSFLAGS_WAITING)
	    {
	    /* task will not be waiting anymore */

	    pTcb->events.sysflags &= ~EVENTS_SYSFLAGS_WAITING;

	    intUnlock (level);

	    /* remove task from the pend Q and put it back on the ready Q */

            windPendQRemove (pTcb);
            windReadyQPut (pTcb);

	    /*
	     * windExit() will return ERROR: errorStatus is set so that 
	     * task will know what caused the error
	     */

            taskRtnValueSet (pTcb, ERROR);
	    pTcb->errorStatus = S_objLib_OBJ_DELETED;
	    }
	else
	    intUnlock (level);
	}
    }

/*******************************************************************************
*
* eventReceive - Wait for event(s)
*
* Pends task until one or all specified <events> have occurred. When
* they have,<pEventsReceived> will be filled with those that did occur.
*
* The <options> parameter is used for three user options. Firstly, it is used 
* to specify if the task is going to wait for all events to occur or only one 
* of them. One of the following has to be selected:
* .iP "EVENTS_WAIT_ANY (0x1)"
* only one event has to occur
* .iP "EVENTS_WAIT_ALL (0x0)"
* will wait until all events occur.
* .LP
* Secondly, it is used to specify if the events returned in <pEventsReceived>
* will be only those received and wanted, or all events received (even the
* ones received before eventReceive() was called). By default it returns
* only the events wanted. Performing a bitwise-OR of the following: 
* .iP "EVENTS_RETURN_ALL (0x2)"
* causes the function to return received events, both wanted and unwanted.
* .LP
* Thirdly, it can be used to retrieve what events have been received by the
* current task. If the option
* .iP "EVENTS_FETCH (0x80)"
* is chosen by the user, then <pEventsReceived> will be filled with the events
* that have already been received and will return immediately. In this case,
* the parameters <events> and <timeout>, as well as all the other options, are
* ignored. Also, events are not cleared, allowing to get a peek at the events
* that have already been received.
* .LP
*
* The <timeout> parameter specifies the number of ticks to wait for wanted
* events to be sent to the waiting task. It can also have the following 
* special values:
* .iP "NO_WAIT  (0)"
* return immediately, even if no events have arrived.
* .iP "WAIT_FOREVER  (-1)"
* never time out.
* .LP
*
* It must also be noted that events sent to the receiving task are cleared
* prior to returning, as if a call to eventClear() was done.
*
* The parameter <pEventsReceived> is always filled with the events received
* even when the function returns an error, except if a value of NULL was
* passed.
*
* WARNING: This routine may not be used from interrupt level.
*
* RETURNS:
* OK on success or ERROR.
*
* ERRNO
* .iP "S_eventLib_TIMEOUT"
* Wanted events not received before specified time expired.
* .iP "S_eventLib_NOT_ALL_EVENTS"
* Specified NO_WAIT as the timeout parameter and wanted events were not already
* received when the routine was called.
* .iP "S_objLib_OBJ_DELETED"
* Task is waiting for some events from a resource that is subsequently deleted.
* .iP "S_intLib_NOT_ISR_CALLABLE"
* Function has been called from ISR.
* .iP "S_eventLib_ZERO_EVENTS"
* The <events> parameter has been passed a value of 0.
*
* SEE ALSO: semEvLib, msgQEvLib, eventSend()
*/

STATUS eventReceive 
    (
    UINT32 events,		/* events task is waiting to occur */
    UINT8 options,		/* user options */
    int timeout,		/* ticks to wait */
    UINT32 *pEventsReceived	/* events occured are returned through this */
    )
    {
    int level;
    STATUS status;
    UINT32 received;

    if (INT_RESTRICT () != OK)
	return (ERROR);

    /* special case: only return received events */

    if (options & EVENTS_FETCH)
	{
	if (pEventsReceived != NULL)
	    *pEventsReceived = taskIdCurrent->events.received;
	return (OK);
	}

    if (events == 0x0)
	{
	errnoSet (S_eventLib_ZERO_EVENTS);
	return ERROR;
	}

#ifdef WV_INSTRUMENTATION
    /* windview - level 1 event logging */
    EVT_OBJ_EVENT(EVENT_EVENTRECEIVE, 3, events, timeout, options);
#endif /* WV_INSTRUMENTATION */

again:
    /*
     * This caching of value is done to reduce the amount of time we spend
     * with the interrupts locked. If not used, the whole function has to
     * be within an intLock()...intUnlock() pair.
     */

    /*
     * Caching the value causes a compiler warning since we remove the
     * volatile property of the structure member. This is what we want to
     * achieve so that error must NOT be corrected.
     */

    received = taskIdCurrent->events.received;

    /* check if wanted events have not already been received */

    if ((options & EVENTS_WAIT_MASK) == EVENTS_WAIT_ALL)
	{
	/*
	 * In the case that ALL events are wanted, all the events must already
	 * have been received.
	 */

	if ((events & received) == events)
	    {
	    /* only return events if user passed a valid return container */

	    level = intLock ();

	    if (pEventsReceived != NULL)
		{
		if (options & EVENTS_RETURN_ALL)
		    *pEventsReceived = taskIdCurrent->events.received;
		else
		    *pEventsReceived = events;
		}

	    /* clear events */

	    taskIdCurrent->events.received = 0x0;

	    intUnlock (level);
	    return (OK);
	    }
	}
    else
	{
	/*
	 * In the case that ANY events are wanted, if even only one of them has
	 * already been received, we have what we wanted.
	 */

	if ((events & received) != 0x0)
	    {
	    /* only return events if user passed a valid return container */

	    level = intLock ();

	    if (pEventsReceived != NULL)
		{
		if (options & EVENTS_RETURN_ALL)
		    *pEventsReceived = taskIdCurrent->events.received;
		else
		    *pEventsReceived = events & taskIdCurrent->events.received;
		}

	    /* clear events */

	    taskIdCurrent->events.received = 0x0;

	    intUnlock (level);
	    return (OK);
	    }
	}

    /* if we got here, we have not already received the events we wanted */

    /* look if events have been received while we were not intLocked */

    level = intLock ();

    if (taskIdCurrent->events.received != received)
	{
	intUnlock (level);
	goto again;
	}

    /*
     * if we do not want to wait, no more events will be received so the 
     * request is not satisfied.
     */

    if (timeout == NO_WAIT)
	{
	/* only return events if user passed a valid return container */

	if (pEventsReceived != NULL)
	    {
	    if (options & EVENTS_RETURN_ALL)
		*pEventsReceived = taskIdCurrent->events.received;
	    else
		*pEventsReceived = events & taskIdCurrent->events.received;
	    }

	/* clear events */

	taskIdCurrent->events.received = 0x0;

	intUnlock (level);
	errnoSet (S_eventLib_NOT_ALL_EVENTS);
	return (ERROR);
	}

    /* We want to wait: fill needed fields */

    taskIdCurrent->events.options = options;
    taskIdCurrent->events.wanted  = events;
    taskIdCurrent->events.sysflags = EVENTS_SYSFLAGS_WAITING;

    kernelState = TRUE;
    intUnlock (level);

#ifdef WV_INSTRUMENTATION
    /* windview - level 2 event logging */
    EVT_TASK_0(EVENT_OBJ_EVENTRECEIVE);
#endif /* WV_INSTRUMENTATION */

    windPendQPut (&vxEventPendQ, timeout);

    if ((status = windExit()) == RESTART)
	{
	timeout = SIG_TIMEOUT_RECALC(timeout);
	goto again;
	}
    else if (status == ERROR)
	{
	if (taskIdCurrent->errorStatus == S_objLib_OBJ_TIMEOUT)
	    {
	    taskIdCurrent->events.sysflags &= ~EVENTS_SYSFLAGS_WAITING;

	    /*
	     * errno will be set to S_objLib_OBJ_TIMEOUT in windTickAnnounce()
	     * but events are not an object so change it to eventLib's timeout
	     */

	    errnoSet (S_eventLib_TIMEOUT);
	    }
	}

    /*
     * fill return variable with events received only if the user provided
     * a valid container
     */

    level = intLock ();

    if (pEventsReceived != NULL)
	{
	if (options & EVENTS_RETURN_ALL)
	    *pEventsReceived = taskIdCurrent->events.received;
	else
	    *pEventsReceived = events & taskIdCurrent->events.received;
	}

    /* task has finished receiving events: clear events */

    taskIdCurrent->events.received = 0x0;

    intUnlock (level);

    return (status);
    }

/*******************************************************************************
*
* eventSend - Send event(s)
*
* Sends specified event(s) to specified task. Passing a taskId of NULL sends 
* events to the calling task.
*
* RETURNS:
* OK on success or ERROR.
*
* ERRNO
* .iP "S_objLib_OBJ_ID_ERROR"
* Task ID is invalid.
* .iP "S_eventLib_NULL_TASKID_AT_INT_LEVEL"
* Routine was called from ISR with a taskId of NULL.
*
* SEE ALSO: eventReceive()
*/

STATUS eventSend
    (
    int taskId,		/* task events will be sent to */
    UINT32 events	/* events to send 	       */
    )
    {
    WIND_TCB *pTcb;
    UINT8 options;
    UINT32 wanted;
    UINT32 received;
    int level;

    if ((INT_CONTEXT()) && (taskId == (int)NULL))
	{
	errnoSet (S_eventLib_NULL_TASKID_AT_INT_LEVEL);
	return (ERROR);
	}

    pTcb = (taskId == (int)NULL) ? taskIdCurrent : (WIND_TCB*)taskId;

    level = intLock ();

    if (TASK_ID_VERIFY (pTcb) != OK)
	{ 
	intUnlock (level);
	return (ERROR);    /* errno is set by TASK_ID_VERIFY */
	}

#ifdef WV_INSTRUMENTATION
    /* windview - level 1 event logging */
    EVT_OBJ_EVENT(EVENT_EVENTSEND, 2, taskId, events, 0);
#endif /* WV_INSTRUMENTATION */

    pTcb->events.received |= events;

    /* if task is not waiting for events, work is done */

    if (!(pTcb->events.sysflags & EVENTS_SYSFLAGS_WAITING))
	{
	intUnlock (level);
	return (OK);
	}

    /* helps readability of code */

    received = pTcb->events.received;
    wanted   = pTcb->events.wanted;
    options  = pTcb->events.options;

    /*
     * look for one of two scenarios: 
     * (1) all events wanted have been received; or
     * (2) some events received and waiting for any of them.
     */

    if (((wanted & received) == wanted) ||
	(((options & EVENTS_WAIT_MASK) == EVENTS_WAIT_ANY) &&
					((wanted & received) != 0x0)))
	{

	/* received correct events: task is not waiting anymore... */

	pTcb->events.sysflags &= ~EVENTS_SYSFLAGS_WAITING;

	/* ...then put it back on the ready queue */

	if (kernelState)
	    {
	    intUnlock (level);
	    workQAdd1 ((FUNCPTR)eventPendQRemove, (int)pTcb);
	    return (OK);
	    }

	kernelState = TRUE;
	intUnlock (level);

	windPendQRemove (pTcb);

#ifdef WV_INSTRUMENTATION
    /* windview - level 2 event logging */
    EVT_TASK_0(EVENT_OBJ_EVENTSEND);
#endif /* WV_INSTRUMENTATION */

	windReadyQPut (pTcb);

	windExit ();

	return (OK);
	}

    /* task is still waiting but everything went fine */

    intUnlock (level);
    return (OK);
    }

/*******************************************************************************
*
* eventPendQRemove - remove task from the pend Q and put it on ready Q
*
* Deferred removal from vxEventPendQ. Only executed as draining of the work Q.
*
* NOMANUAL
*/

LOCAL void eventPendQRemove
    (
    WIND_TCB * pTcb
    )
    {
    windPendQRemove (pTcb);

#ifdef WV_INSTRUMENTATION
    /* windview - level 2 event logging */
    EVT_TASK_0(EVENT_OBJ_EVENTSEND);
#endif /* WV_INSTRUMENTATION */

    windReadyQPut   (pTcb);
    }

/*******************************************************************************
*
* eventRsrcSend - Send event(s) from a resource
*
* Sends specified event(s) to specified task.
*
* WARNING: Routine must be called with kernelState == TRUE.
*
* RETURNS:
* OK on success or ERROR.
*
* ERRNO
* .iP "S_objLib_OBJ_ID_ERROR"
* Task ID is invalid.
*
* NOMANUAL
*/

STATUS eventRsrcSend
    (
    int taskId,		/* task events will be sent to */
    UINT32 events	/* events to send 	       */
    )
    {
    WIND_TCB *pTcb;
    UINT8 options;
    UINT32 wanted;
    UINT32 received;
    int level;

    pTcb = (WIND_TCB*)taskId;

    level = intLock ();

    if (TASK_ID_VERIFY (pTcb) != OK)
	{ 
	intUnlock (level);
	return (ERROR);    /* errno is set by TASK_ID_VERIFY */
	}

#ifdef WV_INSTRUMENTATION
    /* windview - level 1 event logging */
    EVT_OBJ_EVENT(EVENT_EVENTSEND, 2, taskId, events, 0);
#endif /* WV_INSTRUMENTATION */

    pTcb->events.received |= events;

    /* if task is not waiting for events, work is done */

    if ((pTcb->events.sysflags & EVENTS_SYSFLAGS_WAITING) == 0x0)
	{
	intUnlock (level);
	return (OK);
	}

    /* helps readability of code */

    received = pTcb->events.received;
    wanted   = pTcb->events.wanted;
    options  = pTcb->events.options;

    /*
     * look for one of two scenarios: 
     * (1) all events wanted have been received; or
     * (2) some events received and waiting for any of them.
     */

    if (((wanted & received) == wanted) ||
	(((options & EVENTS_WAIT_MASK) == EVENTS_WAIT_ANY) &&
					((wanted & received) != 0x0)))
	{

	/* received correct events: task is not waiting anymore... */

	pTcb->events.sysflags &= ~EVENTS_SYSFLAGS_WAITING;

	/* ...then put it back on the ready queue */

	intUnlock (level);

	windPendQRemove (pTcb);

#ifdef WV_INSTRUMENTATION
    /* windview - level 2 event logging */
    EVT_TASK_0(EVENT_OBJ_EVENTSEND);
#endif /* WV_INSTRUMENTATION */

	windReadyQPut (pTcb);

	return (OK);
	}

    /* task is still waiting but everything went fine */

    intUnlock (level);
    return (OK);
    }

/*******************************************************************************
*
* eventClear - Clear all events for current task.
*
* This function clears all received events for the calling task.
*
* RETURNS: OK on success or ERROR.
*
* ERRNO
* .iP "S_intLib_NOT_ISR_CALLABLE"
* Routine has been called from interrupt level.
*
* SEE ALSO: 
*/

STATUS eventClear
    (
    void
    )
    {
    if (INT_RESTRICT () != OK)
	return (ERROR);    /* errno set by INT_RESTRICT() */

    /* if task is currently receiving events, abort */

    taskIdCurrent->events.received = 0x0;

    return (OK);
    }

/*******************************************************************************
*
* eventStart - Common code for resource registration
*
* NOTE: Routine must be called with TASK_LOCK() already done. This function is
* responsible for doing the TASK_UNLOCK().
*
* RETURNS: OK on success or ERROR.
*
* ERRNO
* .iP "S_eventLib_ALREADY_REGISTERED"
* A task is already registered.
* .iP "S_eventLib_EVENTSEND_FAILED"
* A bad taskId was passed to eventSend().
*
* NOMANUAL
*/

STATUS eventStart
    (
    OBJ_ID objId,	/* resource on which to register events  */
    EVENTS_RSRC * pRsrc,/* ptr to resource control block	 */
    FUNCPTR isRsrcFree, /* ptr to fcn determining if rsrc is free*/
    UINT32 events, 	/* 32 possible events to register        */
    UINT8 options	/* event-related resource options	 */
    )
    {
    int level;	/* interrupt lock level */

    /*
     * Refuse to register if another task has already done so, but only if
     * the EVENTS_ALLOW_OVERWRITE option is not in use AND if the task trying
     * to do the registration is not the one currently registered.
     */

    if ((pRsrc->options & EVENTS_ALLOW_OVERWRITE) != EVENTS_ALLOW_OVERWRITE)
	{
	if ((pRsrc->taskId!=(int)NULL) && (pRsrc->taskId != (int)taskIdCurrent))
	    {
	    TASK_UNLOCK ();
	    errnoSet (S_eventLib_ALREADY_REGISTERED);
	    return (ERROR);
	    }
	}

    /* check if user wants to send events */

    level = intLock ();

    if ((options & EVENTS_SEND_IF_FREE) == EVENTS_SEND_IF_FREE)
	{
	/* find out if resource is free */

	if (isRsrcFree (objId))
	    {
	    intUnlock (level);

	    /* only register if user didn't choose one shot */

	    if ((options & EVENTS_SEND_ONCE) != EVENTS_SEND_ONCE)
		{
		pRsrc->registered = events;
		pRsrc->options    = options;
		pRsrc->taskId     = (int)taskIdCurrent;
		}
	    else
		pRsrc->taskId	  = (int) NULL;

	    TASK_UNLOCK ();

	    if (eventSend ((int)taskIdCurrent, events) != OK)
		{
		errnoSet (S_eventLib_EVENTSEND_FAILED);
		return (ERROR);
		}
	    else
		return (OK);
	    }
	}

    /*
     * resource not free or user chose not to send events right away:
     * register needed info
     */

    pRsrc->registered = events;
    pRsrc->options    = options;
    pRsrc->taskId     = (int)taskIdCurrent;

    intUnlock (level);

    TASK_UNLOCK ();

    return (OK);
    }

