/* msgQSmLib.c - shared memory message queue library (VxMP Option) */

/* Copyright 1990-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01q,06may02,mas  cache flush and volatile fix (SPR 68334); bridge flush fix
		 (SPR 68844)
01p,24oct01,mas  doc update (SPR 71149)
01o,13sep01,tcr  Fix SPR 29673 - Windview event logging macro
01n,14mar99,jdi  doc: removed refs to config.h and/or configAll.h (SPR 25663).
01m,22feb99,wsl  doc: correct typo in name of errno code
01l,05oct98,jmp  doc: cleanup.
01k,17apr98,rlp  canceled msgQSmInit and msgQSmSend modifications for backward
                 compatibility.
01j,04nov97,rlp  modified msgQSmInit and msgQSmSend for tracking messages
                 sent.
01i,16oct95,jdi  doc: added msgQSmLib.h to list of include files (SPR 4353).
01h,22mar94,pme  added WindView level 2 event logging.
01g,23feb93,jdi  more documentation cleanup for 5.1 manual.
01f,14feb93,jdi  documentation cleanup 5.1.
01e,29jan93,pme  added little endian support.
		 added #include "private/smObjLibP.h"
01d,23nov92,jdi  documentation cleanup.
01c,02oct92,pme  added SPARC support. documentation cleanup.
01b,29sep92,pme  changed calls to smFree and smMalloc to smMemFree 
		 and smMemMalloc.	
01a,19jul92,pme  added msgQSmLibInit() to reduce coupling.
		 changed function name to msgQSm...
		 cleaned up after code review.
		 written from 01k of msgQLib
*/

/*
DESCRIPTION
This library provides the interface to shared memory message queues.
Shared memory message queues allow a variable number of messages (varying
in length) to be queued in first-in-first-out order.  Any task running on
any CPU in the system can send messages to or receive messages from a
shared message queue.  Tasks can also send to and receive from the
same shared message queue.  Full-duplex communication between two
tasks generally requires two shared message queues, one for each
direction.

Shared memory message queues are created with msgQSmCreate().  Once
created, they can be manipulated using the generic routines for local
message queues; for more information on the use of these routines, see the
manual entry for msgQLib.

MEMORY REQUIREMENTS
The shared memory message queue structure is allocated from a dedicated 
shared memory partition.  This shared memory partition is initialized by
the shared memory objects master CPU.  The size of this partition is
defined by the maximum number of shared message queues, SM_OBJ_MAX_MSG_Q.

The message queue buffers are allocated from the shared memory system 
partition. 

RESTRICTIONS
Shared memory message queues differ from local message queues in the
following ways:

\is
\i `Interrupt Use:'
Shared memory message queues may not be used (sent to or received from) at
interrupt level.

\i `Deletion:'
There is no way to delete a shared memory message queue and free its associated
shared memory.  Attempts to delete a shared message queue return ERROR and
set `errno' to S_smObjLib_NO_OBJECT_DESTROY.

\i `Queuing Style:'
The shared message queue task queueing order specified when a message queue is
created must be FIFO.
\ie

CONFIGURATION
Before routines in this library can be called, the shared memory objects
facility must be initialized by calling usrSmObjInit().  This is done
automatically during VxWorks initialization if the component INCLUDE_SM_OBJ
is included.

AVAILABILITY: This module is distributed as a component of the unbundled shared
objects memory support option, VxMP.

INCLUDE FILES: msgQSmLib.h, msgQLib.h, smMemLib.h, smObjLib.h

SEE ALSO: msgQLib, smObjLib, msgQShow, usrSmObjInit(),
\tb VxWorks Programmer's Guide: Shared Memory Objects
*/

/* LINTLIBRARY */

#include "vxWorks.h"
#include "errnoLib.h"
#include "objLib.h"
#include "intLib.h"
#include "taskLib.h"
#include "cacheLib.h"
#include "semLib.h"
#include "smMemLib.h"
#include "smObjLib.h"
#include "msgQLib.h"
#include "stdarg.h"
#include "string.h"
#include "netinet/in.h"
#include "private/msgQSmLibP.h"
#include "private/msgQLibP.h"
#include "private/semSmLibP.h"
#include "private/smMemLibP.h"
#include "private/smObjLibP.h"
#include "private/eventP.h"

/*****************************************************************************
*
* msgSmLibInit - initialize the shared memory message queue package
*
* SEE ALSO: smObjLibInit().
*
* RETURNS: OK
*
* NOMANUAL
*/

void msgQSmLibInit (void)
    {
    msgQSmSendRtn    = (FUNCPTR) msgQSmSend;
    msgQSmReceiveRtn = (FUNCPTR) msgQSmReceive;
    msgQSmNumMsgsRtn = (FUNCPTR) msgQSmNumMsgs;
    }

/*****************************************************************************
*
* msgQSmCreate - create and initialize a shared memory message queue (VxMP Option)
*
* This routine creates a shared memory message queue capable of holding up
* to <maxMsgs> messages, each up to <maxMsgLength> bytes long.  It returns a
* message queue ID used to identify the created message queue.  The queue
* can only be created with the option MSG_Q_FIFO (0), thus queuing pended
* tasks in FIFO order.
*
* The global message queue identifier returned can be used directly by generic
* message queue handling routines in msgQLib -- msgQSend(), msgQReceive(), 
* and msgQNumMsgs() -- and by the show routines show() and msgQShow().
*
* If there is insufficient memory to store the message queue structure
* in the shared memory message queue partition or if the shared memory system
* pool cannot handle the requested message queue size, shared memory message 
* queue creation will fail with `errno' set to S_memLib_NOT_ENOUGH_MEMORY.
* This problem can be solved by incrementing the value of SM_OBJ_MAX_MSG_Q
* and/or the shared memory objects dedicated memory size SM_OBJ_MEM_SIZE .
*
* Before this routine can be called, the shared memory objects facility must
* be initialized (see msgQSmLib).
*
* AVAILABILITY:
* This routine is distributed as a component of the unbundled shared memory
* objects support option, VxMP.
*
* RETURNS: MSG_Q_ID, or NULL if error.
*
* ERRNO: S_memLib_NOT_ENOUGH_MEMORY, S_intLib_NOT_ISR_CALLABLE,
* S_msgQLib_INVALID_QUEUE_TYPE, S_smObjLib_LOCK_TIMEOUT
*
* SEE ALSO: smObjLib, msgQLib, msgQShow
*/

MSG_Q_ID msgQSmCreate 
    (
    int	maxMsgs,	/* max messages that can be queued */
    int	maxMsgLength,	/* max bytes in a message */
    int	options		/* message queue options */
    )
    {
    SM_MSG_Q_ID	smMsgQId;
    void *	pSmMsgPool;	/* pointer to memory for messages */
    int         nodeSize = SM_MSG_NODE_SIZE (maxMsgLength);
    int         temp;           /* temp storage */

    if (INT_RESTRICT () != OK)	/* restrict ISR from calling */
        {
	return (NULL);
        }

    /* 
     * allocate shared memory message queue descriptor from 
     * dedicated shared memory pool.
     */

    smMsgQId = (SM_MSG_Q_ID) smMemPartAlloc ((SM_PART_ID) smMsgQPartId,
                                             sizeof (SM_MSG_Q));
    if (smMsgQId == NULL)
        {
	return (NULL);
        }

    /* 
     * allocate shared memory message queue data buffers from 
     * shared memory system pool.
     */

    pSmMsgPool = smMemMalloc (nodeSize * maxMsgs);
    if (pSmMsgPool == NULL)
    	{ 
	smMemPartFree ((SM_PART_ID) smMsgQPartId, (char *) smMsgQId);
	return (NULL);
	}

    /* Initialize shared memory message queue structure */

    if (msgQSmInit (smMsgQId, maxMsgs, maxMsgLength, options, pSmMsgPool) !=OK)
	{
	smMemPartFree ((SM_PART_ID) smMsgQPartId, (char *) smMsgQId);
	smMemFree ((char *) pSmMsgPool);
	return (NULL);
	}

    /* update shared infos data */

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    temp = pSmObjHdr->curNumMsgQ;               /* PCI bridge bug [SPR 68844]*/

    pSmObjHdr->curNumMsgQ = htonl (ntohl (pSmObjHdr->curNumMsgQ) + 1);

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    temp = pSmObjHdr->curNumMsgQ;               /* BRIDGE FLUSH  [SPR 68334] */

    return ((MSG_Q_ID) (SM_OBJ_ADRS_TO_ID (smMsgQId)));
    }

/*****************************************************************************
*
* msgQSmInit - initialize a shared memory message queue
*
* This routine initializes a shared memory message queue data structure.  Like
* msgQSmCreate() the resulting message queue is capable of holding up to
* <maxMsgs> messages, each of up to <maxMsgLength> bytes long.  However,
* instead of dynamically allocating the SM_MSG_Q data structure, this
* routine takes a pointer <pSmMsgQ> to the SM_MSG_Q data structure to be
* initialized, and a local pointer <pSmMsgPool> to the buffer to be use to
* hold queued messages.  <pSmMsgPool> must point to a 4-byte aligned buffer
* that is (<maxMsgs> * SM_MSG_NODE_SIZE (<maxMsgLength>)).
*
* The queue can be created with the following option:
*
*     MSG_Q_FIFO      queue pended tasks in FIFO order 
*
* RETURNS: OK or ERROR.
*
* ERRNO: S_msgQLib_INVALID_QUEUE_TYPE
*
* SEE ALSO: msgQSmCreate()
*
* NOMANUAL
*/

STATUS msgQSmInit 
    (
    SM_MSG_Q * pSmMsgQ,      /* local pointer to msg queue to initialize */
    int        maxMsgs,      /* max messages that can be queued */
    int        maxMsgLength, /* max bytes in a message */
    int        options,      /* message queue options */
    void *     pSmMsgPool    /* local pointer to memory for messages */
    )
    {
    int                 ix;
    int                 nodeSize = SM_MSG_NODE_SIZE (maxMsgLength);
    SM_MSG_Q volatile * pSmMsgQv = (SM_MSG_Q volatile *) pSmMsgQ;
    int                 temp;           /* temp storage */

    bzero ((char *) pSmMsgQ, sizeof (*pSmMsgQ));/* clear out msg q structure */

    /* initialize msgQ semaphores */

    switch (options & MSG_Q_TYPE_MASK)
        {
        case MSG_Q_FIFO:    
	    {
	    semSmCInit (&pSmMsgQ->msgQSem, SEM_Q_FIFO, 0);
            semSmCInit (&pSmMsgQ->freeQSem, SEM_Q_FIFO, maxMsgs);
    	    break;
	    }

        default:
            errno = S_msgQLib_INVALID_QUEUE_TYPE;
            return (ERROR);
        }

    /* put msg nodes on free list */

    for (ix = 0; ix < maxMsgs; ix++)
	{
	smDllAdd (&pSmMsgQ->freeQ, (SM_DL_NODE *) pSmMsgPool);
	pSmMsgPool = (void *) (((char *) pSmMsgPool) + nodeSize);
	}

    /* initialize rest of message queue */

    pSmMsgQv->options	   = htonl (options);
    pSmMsgQv->maxMsgs	   = htonl (maxMsgs);
    pSmMsgQv->maxMsgLength = htonl (maxMsgLength);

    pSmMsgQv->objType      = htonl (MSG_Q_TYPE_SM);

    pSmMsgQv->verify       = (UINT32) htonl (LOC_TO_GLOB_ADRS (pSmMsgQ));

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    temp = pSmMsgQv->objType;                   /* BRIDGE FLUSH  [SPR 68334] */

    return (OK);
    }

/*****************************************************************************
*
* msgQSmSend - send a message to a shared memory message queue
*
* This routine sends the message in <buffer> of length <nBytes> to the
* shared memory message queue <smMsgQId>.  If any tasks are already waiting to
* receive messages on the queue, the message will immediately be delivered
* to the first waiting task.  If no task is waiting to receive messages, the
* message is saved in the message queue.
*
* The <timeout> parameter specifies the number of ticks to wait for free
* space if the message queue is full.  <timeout> can have the special value
* NO_WAIT that indicates msgQSend() will return immediately even if the
* message has not been sent.  Another value for <timeout> is WAIT_FOREVER
* that indicates that msgQSend() will never timeout.
*
* The <priority> parameter specifies the priority of the message being sent.
* Normal priority messages (MSG_PRI_NORMAL) are added to the tail
* of the list of queued messages.  Urgent priority messages
* (MSG_PRI_URGENT) are added to the head of the list.
*
* This routine is normally called by the generic msgQSend() system call. 
* The lower bit of the identifier passed to msgQSend() is used to 
* differentiate local and shared memory message queues.
*
* This routine not callable at interrupt level.
*
* RETURNS: OK or ERROR.
*
* ERRNO: S_objLib_OBJ_ID_ERROR, S_objLib_OBJ_UNAVAILABLE,
*        S_objLib_OBJ_TIMEOUT, S_msgQLib_INVALID_MSG_LENGTH,
*        S_smObjLib_LOCK_TIMEOUT
*
* NOMANUAL
*/

STATUS msgQSmSend 
    (
    SM_MSG_Q_ID    smMsgQId,       /* message queue on which to send */
    char *         buffer,         /* message to send */
    UINT           nBytes,         /* length of message */
    int            timeout,        /* ticks to wait */
    int            priority        /* MSG_PRI_NORMAL or MSG_PRI_URGENT */
    )
    {
    SM_MSG_NODE volatile * pMsg;
    SM_DL_NODE *           prevNode;
    int                    level;
    SM_MSG_Q_ID volatile   smMsgQIdv = (SM_MSG_Q_ID volatile) smMsgQId;
    int                    temp;   /* temp storage */

    if (INT_RESTRICT () != OK)          /* not ISR callable */
        {
        return (ERROR);
        }

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    temp = smMsgQIdv->verify;                   /* PCI bridge bug [SPR 68844]*/

    if (SM_OBJ_VERIFY (smMsgQIdv) != OK)
        {
	return (ERROR);
        }

    if (nBytes > ntohl (smMsgQId->maxMsgLength))
	{
	errno = S_msgQLib_INVALID_MSG_LENGTH;
	return (ERROR);
	}

    TASK_LOCK ();

    /* windview level 2 event logging, uses the OSE logging routine */

    EVT_OBJ_SM_MSGQ (EVENT_MSGQSEND, smMsgQId, buffer, nBytes, timeout, 
		     priority,5);

    /* get a free message by taking free msg Q shared counting semaphore */

    if (semSmTake (&smMsgQId->freeQSem, timeout) != OK)
	{
	smMsgQId->sendTimeouts = htonl (ntohl (smMsgQId->sendTimeouts) + 1);
	TASK_UNLOCK ();
	return (ERROR);
	}

    /* a free message was available before timeout, get it */

    if (SM_OBJ_LOCK_TAKE (&smMsgQId->freeQLock, &level) != OK)
        {
	smObjTimeoutLogMsg ("msgQSend", (char *) &smMsgQId->freeQLock);
        TASK_UNLOCK ();
        return (ERROR);                         /* can't take lock */
	}
    pMsg = (SM_MSG_NODE volatile *) smDllGet (&smMsgQId->freeQ);

    SM_OBJ_LOCK_GIVE (&smMsgQId->freeQLock, level);/* release lock */

    pMsg->msgLength = htonl (nBytes);

    bcopy (buffer, SM_MSG_NODE_DATA (pMsg), (int) nBytes);

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    temp = pMsg->msgLength;                     /* BRIDGE FLUSH  [SPR 68334] */

    /* now send message */

    if (SM_OBJ_LOCK_TAKE (&smMsgQId->msgQLock, &level) != OK)
        {
	smObjTimeoutLogMsg ("msgQSend", (char *) &smMsgQId->msgQLock);
        TASK_UNLOCK ();
        return (ERROR);                         /* can't take lock */
	}

    /* insert message in message list according to its priority */

    if (priority == MSG_PRI_NORMAL) 
	{
	prevNode = (SM_DL_NODE *) SM_DL_LAST (&smMsgQId->msgQ);
	}
    else prevNode = NULL; 

    smDllInsert (&smMsgQId->msgQ, prevNode, (SM_DL_NODE *) pMsg);

    SM_OBJ_LOCK_GIVE (&smMsgQId->msgQLock, level);/* release lock */
    
      /*
       * If another CPU is currently doing a msgQ send
       * we can have a case where the other CPU has put a message and
       * is delayed by an interrupt before Giving the shared
       * semaphore. In that case, this CPU can put its message and Give
       * the shared semaphore, the receiver will be unblocked by
       * the Give done by this CPU but the message obtained will
       * be the one put by the other CPU (FIFO order of messages will be kept).
       */

    /* unblock receiver */

    if (semSmGive (&smMsgQId->msgQSem) != OK)
        {
        TASK_UNLOCK (); 
	return (ERROR);
	}	

    TASK_UNLOCK ();

    return (OK);
    }

/*****************************************************************************
*
* msgQSmReceive - receive a message from a shared memory message queue
*
* This routine receives a message from the shared memory message queue
* <smMsgQId>.  The received message is copied into the specified <buffer>,
* which is <maxNBytes> in length.  If the message is longer than <maxNBytes>,
* the remainder of the message is discarded (no error indication is returned).
*
* The <timeout> parameter specifies the number of ticks to wait for a
* message to be sent to the queue, if no message is available when
* msgQSmReceive() is called. <timeout> can have the special value NO_WAIT
* that indicates msgQSmReceive() will return immediately even if a message
* is available.  Another value for <timeout> is WAIT_FOREVER that indicates
* that msgQSmReceive() will never timeout.
*
* This routine is normally called by the generic msgQReceive() system call. 
* The lower bit of the identifier passed to msgQReceive() is used to 
* differentiate local and shared memory message queues.
*
* This routine cannot be called by interrupt service routines.
*
* RETURNS:
* The number of bytes copied to <buffer>, or ERROR.
*
* ERRNO: S_smObjLib_OBJ_ID_ERROR, S_objLib_OBJ_UNAVAILABLE,
*        S_objLib_OBJ_TIMEOUT, S_msgQLib_INVALID_MSG_LENGTH,
*        S_smObjLib_LOCK_TIMEOUT
*
* NOMANUAL
*/

int msgQSmReceive 
    (
    SM_MSG_Q_ID    smMsgQId,       /* message queue from which to receive*/
    char *         buffer,         /* buffer to receive message */
    UINT           maxNBytes,      /* length of buffer */
    int            timeout         /* ticks to wait */
    )
    {
    SM_MSG_NODE volatile * pMsg;
    int	                   bytesReturned;
    int                    level;
    SM_MSG_Q_ID volatile   smMsgQIdv = (SM_MSG_Q_ID volatile) smMsgQId;
    int                    temp;   /* temp storage */

    /* 
     * even though maxNBytes is unsigned, check for < 0 to catch 
     * possible caller errors
     */

    if ((int) maxNBytes < 0)
	{
	errno = S_msgQLib_INVALID_MSG_LENGTH;
	return (ERROR);
	}

    TASK_LOCK ();

    /* windview level 2 event logging, uses the OSE logging routine */

    EVT_OBJ_SM_MSGQ (EVENT_MSGQRECEIVE, smMsgQId, buffer, maxNBytes, 
		     timeout, 0,4);

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    temp = smMsgQIdv->verify;                   /* PCI bridge bug [SPR 68844]*/

    if (SM_OBJ_VERIFY (smMsgQIdv) != OK)
	{
	TASK_UNLOCK ();
	return (ERROR);
	}

    /* block until a message is available */

    if (semSmTake (&smMsgQId->msgQSem, timeout) != OK)
        {
        smMsgQId->recvTimeouts = htonl (ntohl (smMsgQId->recvTimeouts) + 1);
        TASK_UNLOCK ();
        return (ERROR);
        }

    /* a message was available before timeout, get it */

    if (SM_OBJ_LOCK_TAKE (&smMsgQId->msgQLock, &level) != OK)
        {
	smObjTimeoutLogMsg ("msgQReceive", (char *) &smMsgQId->msgQLock);
        TASK_UNLOCK ();
        return (ERROR);                         /* can't take lock */
        }

    pMsg = (SM_MSG_NODE volatile *) smDllGet (&smMsgQId->msgQ);

    SM_OBJ_LOCK_GIVE (&smMsgQId->msgQLock, level);/* release lock */

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    temp = pMsg->msgLength;                     /* PCI bridge bug [SPR 68844]*/

    bytesReturned = min (ntohl (pMsg->msgLength), maxNBytes);

    bcopy (SM_MSG_NODE_DATA (pMsg), buffer, bytesReturned);

    /* now give back message to free queue */

    if (SM_OBJ_LOCK_TAKE (&smMsgQId->freeQLock, &level) != OK)
        {
	smObjTimeoutLogMsg ("msgQReceive", (char *) &smMsgQId->freeQLock);
        TASK_UNLOCK ();
        return (ERROR);                         /* can't take lock */
        }

    smDllAdd (&smMsgQId->freeQ, (SM_DL_NODE *) pMsg);

    SM_OBJ_LOCK_GIVE (&smMsgQId->freeQLock, level);/* release lock */

    /* now give free queue semaphore */

    if (semSmGive (&smMsgQId->freeQSem) != OK)
        {
        TASK_UNLOCK ();
        return (ERROR);
        }

    TASK_UNLOCK ();

    return (bytesReturned);
    }

/*****************************************************************************
*
* msgQSmNumMsgs - get number of messages queued to shared memory message queue
*
* This routine returns the number of messages currently queued to a specified
* message queue <smMsgQId>.
*
* This routine is normally called by the generic msgQNumMsgs() system call. 
* The lower bit of the identifier passed to msgQNumMsgs() is used to 
* differentiate local and shared memory message queues.
*
* RETURNS:
* The number of messages queued, or ERROR.
*
* ERRNO: S_objLib_OBJ_ID_ERROR
*
* NOMANUAL
*/

int msgQSmNumMsgs 
    (
    SM_MSG_Q_ID       smMsgQId          /* message queue to examine */
    )
    {
    SM_MSG_Q_ID volatile smMsgQIdv = (SM_MSG_Q_ID volatile) smMsgQId;
    int                  temp;         /* temp storage */

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    temp = smMsgQIdv->verify;                   /* PCI bridge bug [SPR 68844]*/

    if (SM_OBJ_VERIFY (smMsgQIdv) != OK)
        {
	return (ERROR);
        }

    temp = smMsgQIdv->msgQSem.state.count;      /* PCI bridge bug [SPR 68844]*/
    temp = smMsgQIdv->msgQSem.state.count;

    return (ntohl (temp));
    }

