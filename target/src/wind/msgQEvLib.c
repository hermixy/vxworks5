/* msgQEvLib.c - VxWorks events support for message queues */

/* Copyright 2001 Wind River Systems, Inc. */

#include "copyright_wrs.h"

/*
modification history
--------------------
01b,18oct01,bwa  Modified comment for options in msgQEvStart(). Added check
		 for current task in msgQEvStop().
01a,21sep01,bwa  written
*/

/*
DESCRIPTION

This library is an extension to eventLib, the events library. Its purpose 
is to support events for message queues.

The functions in this library are used to control registration of tasks on a
message queue. The routine msgQEvStart() registers a task and starts the
notification process. The function msgQEvStop() un-registers the task, which 
stops the notification mechanism. 

When a task is registered and a message arrives on the queue, the events
specified are sent to that task, on the condition that no other task is pending
on that message queue. However, if a msgQReceive() is to be done afterwards
to get the message, there is no guarantee that it will still be available.

INCLUDE FILES: msgQEvLib.h

SEE ALSO: eventLib,
.pG "Basic OS"

*/

#include "vxWorks.h"
#include "taskLib.h"
#include "errnoLib.h"
#include "intLib.h"
#include "eventLib.h"
#include "msgQLib.h"
#include "taskArchLib.h"
#include "private/msgQLibP.h"
#include "private/windLibP.h"

/* externs */

extern CLASS_ID msgQClassId;

/* locals */

LOCAL BOOL msgQEvIsFree (MSG_Q_ID msgQId);

/*******************************************************************************
*
* msgQEvLibInit - dummy lib init fcn, used to pull lib into kernel
*
* NOMANUAL
*
*/

void msgQEvLibInit (void)
    {
    }

/*******************************************************************************
*
* msgQEvStart - start event notification process for a message queue
*
* This routine turns on the event notification process for a given message
* queue. When a message becomes available but not wanted in that 
* particular message queue, the events specified will be sent to the task
* registered by this function. A task can overwrite its own registration
* without first invoking msgQEvStop() or specifying the ALLOW_OVERWRITE option.
*
* The <options> parameter is used for 3 user options:
* - Specify if the events are to be sent only once or every time a message
* arrives until msgQEvStop() is called. The option 
* .iP "EVENTS_SEND_ONCE (0x1)"
* tells the message queue to send the events one time only.
* - Specify if another task can register itself while the current task is
* still registered. If so, the current task registration is overwritten without
* any warning. The option 
* .iP "EVENTS_ALLOW_OVERWRITE (0x2)"
* allow subsequent registrations to overwrite the current one.
* - Specify if events are to be sent right away in the case a message is
* waiting to be picked up. The option
* .iP "EVENTS_SEND_IF_FREE (0x4)"
* tells the registration process to send events if a message is present on the
* message queue.
* If none of those three options is to be used, then the option
* .iP "EVENTS_OPTIONS_NONE (0x0)"
* has to be passed to the <options> parameter.
*
* RETURNS: OK on success, or ERROR.
*
* ERRNO
* .iP "S_objLib_OBJ_ID_ERROR"
* The message queue ID is invalid.
* .iP "S_eventLib_ALREADY_REGISTERED"
* A task is already registered on the message queue.
* .iP "S_intLib_NOT_ISR_CALLABLE"
* Routine has been called from interrupt level.
* .iP "S_eventLib_EVENTSEND_FAILED"
* User chose to send events right away and that operation failed.
* .iP "S_eventLib_ZERO_EVENTS"
* User passed in a value of zero to the <events> parameter.
*
* SEE ALSO: eventLib, msgQLib, msgQEvStop()
*/

STATUS msgQEvStart
    (
    MSG_Q_ID msgQId,	/* msg Q for which to register events */
    UINT32 events,	/* 32 possible events                 */
    UINT8 options 	/* event-related msg Q options        */
    )
    {
    if (events == 0x0)
	{
	errnoSet (S_eventLib_ZERO_EVENTS);
	return (ERROR);
	}

    if (INT_RESTRICT () != OK)
	return (ERROR);    /* errno set by INT_RESTRICT() */

    TASK_LOCK (); /* to prevent msg Q from being deleted */

    if (OBJ_VERIFY(msgQId,msgQClassId) != OK)
	{
	TASK_UNLOCK ();
	return (ERROR);    /* errno is set by OBJ_VERIFY */
	}

    /* TASK_UNLOCK() will be done by eventStart() */

    return (eventStart ((OBJ_ID)msgQId, &msgQId->events, &msgQEvIsFree,
	    events, options));
    }

/*******************************************************************************
*
* msgQEvIsFree - Determines if a msg is present on the queue
*
* RETURNS: TRUE if a msg is present, FALSE if not
*
* NOTE: the msgQId has to be validated before calling this function since it
*       doesn't validate it itself before dereferencing the pointer.
*
* NOMANUAL
*/

LOCAL BOOL msgQEvIsFree (MSG_Q_ID msgQId)
    {
    if (msgQId->msgQ.first != NULL)
	return TRUE;

    return FALSE;
    }

/*******************************************************************************
*
* msgQEvStop - stop event notification process for a message queue
*
* This routine turns off the event notification process for a given message
* queue. It thus allows another task to register itself for event notification
* on that particular message queue.
*
* RETURNS: OK on success, or ERROR.
*
* ERRNO
* .iP "S_objLib_OBJ_ID_ERROR"
* The message queue ID is invalid.
* .iP "S_intLib_NOT_ISR_CALLABLE"
* Routine has been called from interrupt level.
* .iP "S_eventLib_TASK_NOT_REGISTERED"
* Routine has not been called by registered task.
*
* SEE ALSO: eventLib, msgQLib, msgQEvStart()
*/

STATUS msgQEvStop
    (
    MSG_Q_ID msgQId
    )
    {
    int level;

    if (INT_RESTRICT() != OK)
	return (ERROR);
    /*
     * intLock is used instead of TASK_LOCK because it is faster (since
     * TASK_UNLOCK calls intLock anyway) and time the ints are locked is
     * really short
     */

    level = intLock();

    if (OBJ_VERIFY(msgQId,msgQClassId) != OK)
	{
	intUnlock (level);
	return (ERROR);    /* errno is set by OBJ_VERIFY */
	}

    if (msgQId->events.taskId != (int)taskIdCurrent)
	{
	intUnlock (level);
	errnoSet (S_eventLib_TASK_NOT_REGISTERED);
	return (ERROR);
	}

    msgQId->events.taskId = (int)NULL;

    intUnlock (level);

    return (OK);
    }

