/* msgQLib.c - message queue library */

/* Copyright 1990-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
02n,10dec01,bwa  Added comment about MSG_Q_EVENTSEND_ERR_NOTIFY msgQCreate
                 option (SPR 72058).
02m,26oct01,bwa  Added msgQEvLib and eventLib to the list of 'SEE ALSO'
                 modules.
02l,12oct01,cjj  Added documentation regarding S_eventLib_EVENTSEND_FAILED
                 error
02k,09oct01,gls  merged in 02j, 02i, 02h, 02g below from AE.
02j,22feb01,ahm  ensured errno does'nt change if msgQDelete() is successful
                 (SPR#34643)
02i,19sep00,bwa  passed extra argument (msgQid) to calls to qJobGet()
                 added errnoSet after calls to qJobGet()(SPR #34057) 
02h,06sep00,aeg  doc: mentioned INCLUDE_MSG_Q component name.
02g,11aug00,aeg  fixed updating of timeout stats in msgQSend/Receive (SPR 33683)
02f,07sep01,bwa  Added VxWorks events support. Fixed SPR #31241 (corrected
		 msgQReceive comment for NO_WAIT).
02e,18dec00,pes  Correct compiler warnings
02d,19may98,drm  merged code from 3rd party to add distributed message queue 
                 support.
                 - merged code was originally based on version 02a 
02c,11may98,cjtc fix problem with problem fix!! Multiple calls to object
                 instrumentation in msgQDestroy
02b,11may98,nps  fixed msgQDestroy instrumentation anomaly.
02c,17apr98,rlp  canceled msgQInit and msgQSend modifications for backward
                 compatibility.
02b,04nov97,rlp  modified msgQInit and msgQSend for tracking messages sent.
02a,24jun96,sbs  made windview instrumentation conditionally compiled
01z,22oct95,jdi  doc: added bit values for options (SPR 4276).
02d,14apr94,smb  fixed class dereferencing for instrumentation macros
02c,15mar94,smb  modified instrumentation macros
02b,24jan94,smb  added instrumentation macros
02a,18jan94,smb  added instrumentation corrections for msgQDelete
01z,10dec93,smb  added instrumentation
01y,30jun93,jmm  changed msgQDestroy to look at msgQ as well as freeQ (spr 2070)
01x,02feb93,jdi  documentation cleanup for 5.1.
01w,13nov92,jcf  package init called with object creation.
01v,19oct92,pme  added reference to shared message queue documentation.
01u,23aug92,jcf  balanced taskSafe with taskUnsafe.
01t,11aug92,jcf  fixed msgQDestroy safety problem.
01s,30jul92,rrr  added restart and msgQ fix
01r,29jul92,pme  added NULL function pointer check for smObj routines.
01q,29jul92,jcf  package init called with object initialization.
01p,22jul92,pme  made msgQDestroy return S_smObjLib_NO_OBJECT_DESTROY when
		 trying to destroy a shared message queue.
01o,19jul92,pme  added shared message queue support.
01n,18jul92,smb  Changed errno.h to errnoLib.h.
01m,04jul92,jcf	 show routine removed.
01l,26may92,rrr  the tree shuffle
01k,13dec91,gae  ANSI fixes.  
01j,19nov91,rrr  shut up some ansi warnings.
01i,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed includes to have absolute path from h/
		  -changed VOID to void
		  -changed copyright notice
01h,10aug91,del  changed interface to qInit to pass all "optional" args.
01g,05apr91,jdi	 documentation -- removed header parens and x-ref numbers;
		 doc review by dnw.
01f,22jan91,jaa	 documentation.
01e,08oct90,dnw  lint
01d,01oct90,dnw  fixed bug of msgQSend() not returning ERROR on timeout.
		 removed msgQSendArgs()/ReceiveArgs().
		 reordered parameters of msgQ{Send,Receive}().
		 simplified msgQInfoGet() interface.
		 added msgQNumMsgs().
		 made msgQDestroy() LOCAL.
		 made msgQ{LibInit,Init,Terminate}() be NOMANUAL.
		 finished documentation.
01c,19jul90,dnw  added VARARGS to msgQ{Send,Receive}Args()
01b,19jul90,dnw  changed call to objAlloc() to objAllocExtra()
		 made msgQShow() prettier
		 lint
01a,10may90,dnw  written
*/

/*
DESCRIPTION
This library contains routines  for creating and using message queues, the 
primary intertask communication mechanism within a single CPU.  Message
queues allow a variable number of messages (varying in length) to be
queued in first-in-first-out (FIFO) order.  Any task or interrupt service
routine can send messages to a message queue.  Any task can receive
messages from a message queue.  Multiple tasks can send to and receive
from the same message queue.  Full-duplex communication between two tasks
generally requires two message queues, one for each direction.

To provide message queue support for a system, VxWorks must be configured
with the INCLUDE_MSG_Q component.

CREATING AND USING MESSAGE QUEUES
A message queue is created with msgQCreate().  Its parameters specify the
maximum number of messages that can be queued to that message queue and 
the maximum length in bytes of each message.  Enough buffer space will 
be pre-allocated to accommodate the specified number of messages of 
specified length.

A task or interrupt service routine sends a message to a message queue
with msgQSend().  If no tasks are waiting for messages on the message queue,
the message is simply added to the buffer of messages for that queue.
If any tasks are already waiting to receive a message from the message
queue, the message is immediately delivered to the first waiting task.

A task receives a message from a message queue with msgQReceive().
If any messages are already available in the message queue's buffer,
the first message is immediately dequeued and returned to the caller.
If no messages are available, the calling task will block and be added to
a queue of tasks waiting for messages.  This queue of waiting tasks can
be ordered either by task priority or FIFO, as specified in an option
parameter when the queue is created.

TIMEOUTS
Both msgQSend() and msgQReceive() take timeout parameters.  When sending a
message, if no buffer space is available to queue the message, the timeout
specifies how many ticks to wait for space to become available.  When
receiving a message, the timeout specifies how many ticks to wait if no
message is immediately available.  The <timeout> parameter can
have the special values NO_WAIT (0) or WAIT_FOREVER (-1).  NO_WAIT 
means the routine should return immediately; WAIT_FOREVER means the routine
should never time out.

URGENT MESSAGES
The msgQSend() routine allows the priority of a message to be specified
as either normal or urgent, MSG_PRI_NORMAL (0) and MSG_PRI_URGENT (1),
respectively.  Normal priority messages are added to the tail of the list
of queued messages, while urgent priority messages are added to the head
of the list.

VXWORKS EVENTS
If a task has registered with a message queue via msgQEvStart(), events will
be sent to that task when a message arrives on that message queue, on the
condition that no other task is pending on the queue.

INTERNAL:
	WINDVIEW INSTRUMENTATION
Level 1:
	msgQCreate() causes EVENT_MSGQCREATE
	msgQDestroy() causes EVENT_MSGQDELETE
	msgQSend() causes EVENT_MSGQSEND
	msgQReceive() causes EVENT_MSGQRECEIVE

Level 2:
	N/A

Level 3:
	N/A

INCLUDE FILES: msgQLib.h

SEE ALSO: pipeDrv, msgQSmLib, msgQEvLib, eventLib,
.pG "Basic OS"
*/

#include "vxWorks.h"
#include "stdlib.h"
#include "string.h"
#include "taskLib.h"
#include "intLib.h"
#include "smObjLib.h"
#include "errnoLib.h"
#include "private/eventLibP.h"
#include "private/classLibP.h"
#include "private/objLibP.h"
#include "private/msgQLibP.h"
#include "private/msgQSmLibP.h"
#include "private/distObjTypeP.h"
#include "private/sigLibP.h"
#include "private/eventP.h"
#include "private/windLibP.h"
#include "private/kernelLibP.h"


/* locals */

LOCAL OBJ_CLASS msgQClass;

/* globals */

CLASS_ID msgQClassId = &msgQClass;

/* Instrumentation locals and globals */

#ifdef WV_INSTRUMENTATION
LOCAL OBJ_CLASS msgQInstClass;
CLASS_ID msgQInstClassId = &msgQInstClass;
#endif

/* shared memory objects function pointers */

FUNCPTR  msgQSmSendRtn;
FUNCPTR  msgQSmReceiveRtn;
FUNCPTR  msgQSmNumMsgsRtn;

/* distributed objects function pointers */

FUNCPTR  msgQDistSendRtn;
FUNCPTR  msgQDistReceiveRtn;
FUNCPTR  msgQDistNumMsgsRtn;

/* forward declarations */

LOCAL STATUS msgQDestroy (MSG_Q_ID msgQId, BOOL dealloc);

/* locals */

LOCAL BOOL msgQLibInstalled;	/* protect from muliple inits */


/*******************************************************************************
*
* msgQLibInit - initialize message queue library
*
* This routine initializes message queue facility.
* It is called once in kernelInit().
*
* RETURNS: OK or ERROR
*
* NOMANUAL
*/

STATUS msgQLibInit (void)
    {
    /* initialize shared memory objects function pointers */

    if (!msgQLibInstalled)
	{
	if (classInit (msgQClassId, sizeof (MSG_Q), OFFSET (MSG_Q, objCore),
		       (FUNCPTR)msgQCreate, (FUNCPTR)msgQInit,
		       (FUNCPTR)msgQDestroy) == OK)
	    {

#ifdef WV_INSTRUMENTATION
	    /* Instrumented class for windview */
	    msgQClassId->initRtn = msgQInstClassId;
            classInstrument (msgQClassId,msgQInstClassId);
#endif

	    msgQEvLibInit (); /* pull msgQLib in kernel */

	    msgQLibInstalled = TRUE;
	    }
	}

    return ((msgQLibInstalled) ? OK : ERROR);
    }

/*******************************************************************************
*
* msgQCreate - create and initialize a message queue
*
* This routine creates a message queue capable of holding up to <maxMsgs>
* messages, each up to <maxMsgLength> bytes long.  The routine returns 
* a message queue ID used to identify the created message queue in all 
* subsequent calls to routines in this library.  The queue can be created 
* with the following options:
* .iP "MSG_Q_FIFO  (0x00)" 8
* queue pended tasks in FIFO order.
* .iP "MSG_Q_PRIORITY  (0x01)"
* queue pended tasks in priority order.
* .iP "MSG_Q_EVENTSEND_ERR_NOTIFY (0x02)"
* When a message is sent, if a task is registered for events and the
* actual sending of events fails, a value of ERROR is returned and the errno
* is set accordingly. This option is off by default.
* .LP
*
* RETURNS:
* MSG_Q_ID, or NULL if error.
*
* ERRNO: S_memLib_NOT_ENOUGH_MEMORY, S_intLib_NOT_ISR_CALLABLE
*
* SEE ALSO: msgQSmLib
*/

MSG_Q_ID msgQCreate
    (
    int         maxMsgs,        /* max messages that can be queued */
    int         maxMsgLength,   /* max bytes in a message */
    int         options         /* message queue options */
    )
    {
    MSG_Q_ID	msgQId;
    void *	pPool;		/* pointer to memory for messages */
    UINT	size = (UINT) maxMsgs * MSG_NODE_SIZE (maxMsgLength);

    if (INT_RESTRICT () != OK)		/* restrict ISR from calling */
	return (NULL);

    if ((!msgQLibInstalled) && (msgQLibInit () != OK))
	return (NULL);			/* package init problem */

    if ((msgQId = (MSG_Q_ID)objAllocExtra (msgQClassId, size, &pPool)) == NULL)
	return (NULL);

    if (msgQInit (msgQId, maxMsgs, maxMsgLength, options, pPool) != OK)
	{
	objFree (msgQClassId, (char *) msgQId);
	return (NULL);
	}

#ifdef WV_INSTRUMENTATION
    /* windview - level 1 event logging routine */
    EVT_OBJ_4 (OBJ, msgQId, msgQClassId, EVENT_MSGQCREATE, 
		msgQId, maxMsgs, maxMsgLength, options);
#endif

    return ((MSG_Q_ID) msgQId);
    }

/*******************************************************************************
*
* msgQInit - initialize a message queue
*
* This routine initializes a message queue data structure.  Like msgQCreate()
* the resulting message queue is capable of holding up to <maxMsgs> messages,
* each of up to <maxMsgLength> bytes long.
* However, instead of dynamically allocating the MSG_Q data structure,
* this routine takes a pointer <pMsgQ> to the MSG_Q data structure to be
* initialized, and a pointer <pMsgPool> to the buffer to be use to hold
* queued messages.  <pMsgPool> must point to a 4 byte aligned buffer
* that is (<maxMsgs> * MSG_NODE_SIZE (<maxMsgLength>)).
*
* The queue can be created with the following options:
*
*	MSG_Q_FIFO	queue pended tasks in FIFO order
*	MSG_Q_PRIORITY	queue pended tasks in priority order
*
* RETURNS: OK or ERROR.
*
* ERRNO: S_msgQLib_INVALID_QUEUE_TYPE
*
* SEE ALSO: msgQCreate()
*
* NOMANUAL
*/

STATUS msgQInit
    (
    FAST MSG_Q *pMsgQ,          /* pointer to msg queue to initialize */
    int         maxMsgs,        /* max messages that can be queued */
    int         maxMsgLength,   /* max bytes in a message */
    int         options,        /* message queue options */
    void *      pMsgPool        /* pointer to memory for messages */
    )
    {
    FAST int		nodeSize = MSG_NODE_SIZE (maxMsgLength);
    FAST int		ix;
    FAST Q_CLASS_ID	msgQType;

    if ((!msgQLibInstalled) && (msgQLibInit () != OK))
	return (ERROR);				/* package init problem */

    bzero ((char *) pMsgQ, sizeof (*pMsgQ));	/* clear out msg q structure */

    /* initialize internal job queues */

    switch (options & MSG_Q_TYPE_MASK)
        {
        case MSG_Q_FIFO:	msgQType = Q_FIFO;	break;
        case MSG_Q_PRIORITY:	msgQType = Q_PRI_LIST;	break;

        default:
            errnoSet (S_msgQLib_INVALID_QUEUE_TYPE);
	    return (ERROR);
        }

    if ((qInit ((Q_HEAD *) &pMsgQ->msgQ, qJobClassId, msgQType,
	 0, 0, 0, 0, 0, 0, 0, 0, 0) != OK) ||
	(qInit ((Q_HEAD *) &pMsgQ->freeQ, qJobClassId, msgQType,
	 0, 0, 0, 0, 0, 0, 0, 0, 0) != OK))
	return (ERROR);


    /* put msg nodes on free list */

    for (ix = 0; ix < maxMsgs; ix++)
	{
	qJobPut (pMsgQ, &pMsgQ->freeQ, (Q_JOB_NODE *) pMsgPool,
		 Q_JOB_PRI_DONT_CARE);
	pMsgPool = (void *) (((char *) pMsgPool) + nodeSize);
	}


    /* initialize rest of msg q */

    pMsgQ->options	= options;
    pMsgQ->maxMsgs	= maxMsgs;
    pMsgQ->maxMsgLength	= maxMsgLength;

    eventInit (&pMsgQ->events);

#ifdef WV_INSTRUMENTATION
    if (wvObjIsEnabled)
    {
    /* windview - connect level 1 event logging routine */
    objCoreInit (&pMsgQ->objCore, msgQInstClassId);
    }
    else
#endif
    objCoreInit (&pMsgQ->objCore, msgQClassId);

    return (OK);
    }

/******************************************************************************
*
* msgQTerminate - terminate message queue
*
* This routine terminates a static message queue that was initialized with
* msgQInit.
*
* RETURNS: OK or ERROR.
*
* NOMANUAL
*/

STATUS msgQTerminate
    (
    MSG_Q_ID msgQId     /* message queue id to terminate */
    )
    {
    return (msgQDestroy (msgQId, FALSE));
    }

/*******************************************************************************
*
* msgQDelete - delete a message queue
*
* This routine deletes a message queue.  All tasks pending on either
* msgQSend(), msgQReceive() or pending for the reception of events
* meant to be sent from the message queue will unblock and return
* ERROR.  When this function returns, <msgQId> is no longer a valid 
* message queue ID.
*
* RETURNS: OK on success or ERROR otherwise.
*
* ERRNO:
* .iP "S_objLib_OBJ_ID_ERROR"
* Message queue ID is invalid
* .iP "S_intLib_NOT_ISR_CALLABLE"
* Routine cannot be called from ISR
* .iP "S_distLib_NO_OBJECT_DESTROY"
* Deleting a distributed message queue is not permitted
* .iP "S_smObjLib_NO_OBJECT_DESTROY"
* Deleting a shared message queue is not permitted
* SEE ALSO: msgQSmLib
*/

STATUS msgQDelete
    (
    MSG_Q_ID msgQId     /* message queue to delete */
    )
    {
    return (msgQDestroy (msgQId, TRUE));
    }

/*******************************************************************************
*
* msgQDestroy - destroy message queue
*
* RETURNS: OK or ERROR.
*
* ERRNO: S_distLib_NO_OBJECT_DESTROY
*
*/

LOCAL STATUS msgQDestroy
    (
    MSG_Q_ID msgQId,    /* message queue to destroy */
    BOOL     dealloc    /* deallocate memory associated with message queue */
    )
    {
    Q_JOB_NODE *pNode;
    FAST int	timeout;
    FAST int	nMsgs;

    int errnoCopy;

    if (ID_IS_SHARED (msgQId))  		/* message Q is shared?*/
	{
		if (ID_IS_DISTRIBUTED (msgQId)) /* message queue is distributed? */
			{
			errno = S_distLib_NO_OBJECT_DESTROY;
		 	return (ERROR);             /* cannot delete distributed msgQ */
			}
	
		errno = S_smObjLib_NO_OBJECT_DESTROY;
        return (ERROR);				/* cannot delete sm. msgQ */
	}

    if (INT_RESTRICT () != OK)			/* restrict isr use */
	return (ERROR);

    TASK_SAFE ();				/* TASK SAFE */
    TASK_LOCK ();				/* LOCK PREEMPTION */

#ifdef WV_INSTRUMENTATION

    /* Indicate that msgQDelete has been initiated */

    /* windview - level 1 event logging routine */
    EVT_OBJ_1 (OBJ, msgQId, msgQClassId, EVENT_MSGQDELETE, msgQId);
#endif

    if (OBJ_VERIFY (msgQId, msgQClassId) != OK)	/* validate message queue id */
	{
	TASK_UNLOCK ();				/* UNLOCK PREEMPTION */
	TASK_UNSAFE ();				/* TASK UNSAFE */
	return (ERROR);
	}

    objCoreTerminate (&msgQId->objCore);	/* INVALIDATE */

#ifdef WV_INSTRUMENTATION

    /*  Indicate that the msgQDelete has succeeded (before TASK_UNLOCK, as
     *  that causes unnecessary WV parser confusion).
     */

    /* windview - level 2 instrumentation
     * EVENT_OBJ_MSGDELETE needs to return the msgQId so MSG_OFFSET is
     * used to calulate the msgQId from the pQHead
     */
    EVT_TASK_1 (EVENT_OBJ_MSGDELETE, msgQId);
#endif

    TASK_UNLOCK ();				/* UNLOCK PREEMPTION */


    /* gobble up all messages in the message and free queues */

    timeout = NO_WAIT;		/* first time through gobble without waiting */
    nMsgs = 0;

    errnoCopy = errnoGet ();

    while (nMsgs < msgQId->maxMsgs)
	{
	while (((pNode = qJobGet (msgQId, &msgQId->freeQ, timeout)) != NULL) &&
	       (pNode != (Q_JOB_NODE *) NONE))
	    nMsgs++;

	while (((pNode = qJobGet (msgQId, &msgQId->msgQ, timeout)) != NULL) &&
	       (pNode != (Q_JOB_NODE *) NONE))
	    nMsgs++;

	timeout = 1;		/* after first time, wait a bit */
	}

    errnoSet (errnoCopy);

    /* terminate both the queues */

    /*
     * Since eventTerminate() can wake up a task, we want to put all tasks
     * in the ready queue before doing a windExit(), so that it is sure that
     * the task of highest priority runs first. To achieve that, the
     * statements 'kernelState = TRUE;' and 'windExit();' have been moved
     * from qJobTerminate() to here. This is the only place that qJobTerminate
     * is called.
     */

    kernelState = TRUE;

    qJobTerminate (&msgQId->msgQ);
    qJobTerminate (&msgQId->freeQ);

    eventTerminate (&msgQId->events);/* free task waiting for events if any */

    windExit ();

    if (dealloc)
	objFree (msgQClassId, (char *) msgQId);

    TASK_UNSAFE ();				/* TASK UNSAFE */

    return (OK);
    }

/*******************************************************************************
*
* msgQSend - send a message to a message queue
*
* This routine sends the message in <buffer> of length <nBytes> to the message
* queue <msgQId>.  If any tasks are already waiting to receive messages
* on the queue, the message will immediately be delivered to the first
* waiting task.  If no task is waiting to receive messages, the message
* is saved in the message queue and if a task has previously registered to 
* receive events from the message queue, these events are sent in the context 
* of this call.  This may result in the unpending of the task waiting for 
* the events.  If the message queue fails to send events and if it was 
* created using the MSG_Q_EVENTSEND_ERR_NOTIFY option, ERROR is returned 
* even though the send operation was successful.
*
* The <timeout> parameter specifies the number of ticks to wait for free
* space if the message queue is full.  The <timeout> parameter can also have 
* the following special values:
* .iP "NO_WAIT  (0)" 8
* return immediately, even if the message has not been sent.  
* .iP "WAIT_FOREVER  (-1)"
* never time out.
* .LP
*
* The <priority> parameter specifies the priority of the message being sent.
* The possible values are:
* .iP "MSG_PRI_NORMAL  (0)" 8
* normal priority; add the message to the tail of the list of queued 
* messages.
* .iP "MSG_PRI_URGENT  (1)"
* urgent priority; add the message to the head of the list of queued messages.
* .LP
*
* USE BY INTERRUPT SERVICE ROUTINES
* This routine can be called by interrupt service routines as well as
* by tasks.  This is one of the primary means of communication
* between an interrupt service routine and a task.  When called from an
* interrupt service routine, <timeout> must be NO_WAIT.
*
* RETURNS: OK on success or ERROR otherwise.
*
* ERRNO:
* .iP "S_distLib_NOT_INITIALIZED"
* Distributed objects message queue library (VxFusion) not initialized.
* .iP "S_smObjLib_NOT_INITIALIZED"
* Shared memory message queue library (VxMP Option) not initialized.
* .iP "S_objLib_OBJ_ID_ERROR"
* Invalid message queue ID.
* .iP "S_objLib_OBJ_DELETED"
* Message queue deleted while calling task was pended.
* .iP "S_objLib_OBJ_UNAVAILABLE"
* No free buffer space when NO_WAIT timeout specified.
* .iP "S_objLib_OBJ_TIMEOUT"
* Timeout occurred while waiting for buffer space.
* .iP "S_msgQLib_INVALID_MSG_LENGTH"
* Message length exceeds limit.
* .iP "S_msgQLib_NON_ZERO_TIMEOUT_AT_INT_LEVEL"
* Called from ISR with non-zero timeout.
* .iP "S_eventLib_EVENTSEND_FAILED"
* Message queue failed to send events to registered task.  This errno 
* value can only exist if the message queue was created with the 
* MSG_Q_EVENTSEND_ERR_NOTIFY option.
* .LP
*
* SEE ALSO: msgQSmLib, msgQEvStart
*/

STATUS msgQSend
    (
    FAST MSG_Q_ID       msgQId,         /* message queue on which to send */
    char *              buffer,         /* message to send */
    FAST UINT           nBytes,         /* length of message */
    int                 timeout,        /* ticks to wait */
    int                 priority        /* MSG_PRI_NORMAL or MSG_PRI_URGENT */
    )
    {
    FAST MSG_NODE *	pMsg;

    if (ID_IS_SHARED (msgQId))			/* message Q is shared? */
        {
		if (ID_IS_DISTRIBUTED (msgQId)) /* message queue is distributed? */
			{
			if (msgQDistSendRtn == NULL)
				{
				errno = S_distLib_NOT_INITIALIZED; 
				return (ERROR);
				}
		 	return ((*msgQDistSendRtn) (msgQId, buffer, nBytes,
			 	timeout, WAIT_FOREVER, priority));
			}

        if (msgQSmSendRtn == NULL)
            {
            errno = S_smObjLib_NOT_INITIALIZED;
            return (ERROR);
            }

        return ((*msgQSmSendRtn) (SM_OBJ_ID_TO_ADRS (msgQId), buffer, nBytes,
				  timeout, priority));
	}

    /* message queue is local */

    if (!INT_CONTEXT ())
	TASK_LOCK ();
    else
	{
	if (timeout != 0)
	    {
	    errnoSet (S_msgQLib_NON_ZERO_TIMEOUT_AT_INT_LEVEL);
	    return (ERROR);
	    }
	}

restart:
    if (OBJ_VERIFY (msgQId, msgQClassId) != OK)
	{
	if (!INT_CONTEXT ())
	    TASK_UNLOCK ();
	return (ERROR);
	}

#ifdef WV_INSTRUMENTATION
    /* windview - level 1 event logging routine */
    EVT_OBJ_5 (OBJ, msgQId, msgQClassId, EVENT_MSGQSEND, msgQId, 
	       buffer, nBytes, timeout, priority);
#endif

    if (nBytes > msgQId->maxMsgLength)
	{
	if (!INT_CONTEXT ())
	    TASK_UNLOCK ();
	errnoSet (S_msgQLib_INVALID_MSG_LENGTH);
	return (ERROR);
	}

    pMsg = (MSG_NODE *) qJobGet (msgQId, &msgQId->freeQ, timeout);

    if (pMsg == (MSG_NODE *) NONE)
	{
	timeout = SIG_TIMEOUT_RECALC(timeout);
	goto restart;
	}

    if (pMsg == NULL)
	{
#if FALSE
	msgQId->sendTimeouts++;
#else

	/*
	 * The timeout stat should only be updated if a timeout has occured.
	 * An OBJ_VERIFY needs to be performed to catch the case where a 
	 * timeout indeed occured, but the message queue is subsequently 
	 * deleted before the current task is rescheduled.
	 */

	if (errnoGet() == S_objLib_OBJ_TIMEOUT) 
            {
	    if (OBJ_VERIFY (msgQId, msgQClassId) == OK)
	        msgQId->sendTimeouts++;
            else
                errnoSet(S_objLib_OBJ_DELETED);
            }

#endif /* FALSE */

	if (!INT_CONTEXT ())
	    TASK_UNLOCK ();
	return (ERROR);
	}

    pMsg->msgLength = nBytes;
    bcopy (buffer, MSG_NODE_DATA (pMsg), (int) nBytes);

    if (qJobPut (msgQId, &msgQId->msgQ, &pMsg->node, priority) != OK)
	{
	if (!INT_CONTEXT ())
	    TASK_UNLOCK ();

	return (ERROR); /* errno set by qJobPut() */
	}

    if (!INT_CONTEXT ())
	TASK_UNLOCK ();

    return (OK);
    }

/*******************************************************************************
*
* msgQReceive - receive a message from a message queue
*
* This routine receives a message from the message queue <msgQId>.
* The received message is copied into the specified <buffer>, which is
* <maxNBytes> in length.  If the message is longer than <maxNBytes>,
* the remainder of the message is discarded (no error indication
* is returned).
*
* The <timeout> parameter specifies the number of ticks to wait for 
* a message to be sent to the queue, if no message is available when
* msgQReceive() is called.  The <timeout> parameter can also have 
* the following special values: 
* .iP "NO_WAIT  (0)" 8
* return immediately, whether a message has been received or not.  
* .iP "WAIT_FOREVER  (-1)"
* never time out.
* .LP
*
* WARNING: This routine must not be called by interrupt service routines.
*
* RETURNS:
* The number of bytes copied to <buffer>, or ERROR.
*
* ERRNO: S_distLib_NOT_INITIALIZED, S_smObjLib_NOT_INITIALIZED,
*        S_objLib_OBJ_ID_ERROR, S_objLib_OBJ_DELETED,
*        S_objLib_OBJ_UNAVAILABLE, S_objLib_OBJ_TIMEOUT,
*        S_msgQLib_INVALID_MSG_LENGTH, S_intLib_NOT_ISR_CALLABLE
*
* SEE ALSO: msgQSmLib
*/

int msgQReceive
    (
    FAST MSG_Q_ID       msgQId,         /* message queue from which to receive */
    char *              buffer,         /* buffer to receive message */
    UINT                maxNBytes,      /* length of buffer */
    int                 timeout         /* ticks to wait */
    )
    {
    FAST MSG_NODE *	pMsg;
    FAST int		bytesReturned;

    if (INT_RESTRICT() != OK) /* errno set by INT_RESTRICT() */
	return ERROR;

    if (ID_IS_SHARED (msgQId))			/* message Q is shared? */
        {
		if (ID_IS_DISTRIBUTED (msgQId)) /* message queue is distributed? */
			{
			if (msgQDistReceiveRtn == NULL)
				{
				errno = S_distLib_NOT_INITIALIZED;
			 	return (ERROR);
				}
			return ((*msgQDistReceiveRtn) (msgQId, buffer,
				maxNBytes, timeout, WAIT_FOREVER));
			}

        if (msgQSmReceiveRtn == NULL)
            {
            errno = S_smObjLib_NOT_INITIALIZED;
            return (ERROR);
            }

        return ((*msgQSmReceiveRtn) (SM_OBJ_ID_TO_ADRS (msgQId), buffer,
				     maxNBytes, timeout));
	}

    /* message queue is local */

    /* even though maxNBytes is unsigned, check for < 0 to catch possible
     * caller errors
     */
    if ((int) maxNBytes < 0)
	{
	errnoSet (S_msgQLib_INVALID_MSG_LENGTH);
	return (ERROR);
	}

    TASK_LOCK ();

restart:
    if (OBJ_VERIFY (msgQId, msgQClassId) != OK)
	{
	TASK_UNLOCK ();
	return (ERROR);
	}

#ifdef WV_INSTRUMENTATION
    /* windview - level 1 event logging routine */
    EVT_OBJ_4 (OBJ, msgQId, msgQClassId, EVENT_MSGQRECEIVE, msgQId, 
	       buffer, maxNBytes, timeout);
#endif

    pMsg = (MSG_NODE *) qJobGet (msgQId, &msgQId->msgQ, timeout);

    if (pMsg == (MSG_NODE *) NONE)
	{
	timeout = SIG_TIMEOUT_RECALC(timeout);
	goto restart;
	}

    if (pMsg == NULL)
	{
#if FALSE
	msgQId->recvTimeouts++;
#else
	/*
	 * The timeout stat should only be updated if a timeout has occured.
	 * An OBJ_VERIFY needs to be performed to catch the case where a 
	 * timeout indeed occured, but the message queue is subsequently 
	 * deleted before the current task is rescheduled.
	 */

	if (errnoGet() == S_objLib_OBJ_TIMEOUT) 
            {
	    if (OBJ_VERIFY (msgQId, msgQClassId) == OK)
	        msgQId->sendTimeouts++;
            else
                errnoSet(S_objLib_OBJ_DELETED);
            }

#endif /* FALSE */

	TASK_UNLOCK ();
	return (ERROR);
	}

    bytesReturned = min (pMsg->msgLength, maxNBytes);
    bcopy (MSG_NODE_DATA (pMsg), buffer, bytesReturned);

    qJobPut (msgQId, &msgQId->freeQ, &pMsg->node, Q_JOB_PRI_DONT_CARE);

    TASK_UNLOCK ();

    return (bytesReturned);
    }

/*******************************************************************************
*
* msgQNumMsgs - get the number of messages queued to a message queue
*
* This routine returns the number of messages currently queued to a specified
* message queue.
*
* RETURNS:
* The number of messages queued, or ERROR.
*
* ERRNO:  S_distLib_NOT_INITIALIZED, S_smObjLib_NOT_INITIALIZED,
*         S_objLib_OBJ_ID_ERROR
*
* SEE ALSO: msgQSmLib
*/

int msgQNumMsgs
    (
    FAST MSG_Q_ID       msgQId          /* message queue to examine */
    )
    {
    if (ID_IS_SHARED (msgQId))			/* message Q is shared? */
        {
		if (ID_IS_DISTRIBUTED (msgQId)) /* message queue is distributed? */
			{
			if (msgQDistNumMsgsRtn == NULL)
				{
				errno = S_distLib_NOT_INITIALIZED;
				return (ERROR);
				}
			return ((*msgQDistNumMsgsRtn) (msgQId, WAIT_FOREVER));
			}

        if (msgQSmNumMsgsRtn == NULL)
            {
            errno = S_smObjLib_NOT_INITIALIZED;
            return (ERROR);
            }

        return ((*msgQSmNumMsgsRtn) (SM_OBJ_ID_TO_ADRS (msgQId)));
	}

    /* message queue is local */

    if (OBJ_VERIFY (msgQId, msgQClassId) != OK)
	return (ERROR);

    return (msgQId->msgQ.count);
    }
