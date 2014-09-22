/* msgQDistLib.c - distributed objects message queue library (VxFusion option) */

/* Copyright 1999 - 2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01o,23oct01,jws  fix compiler warnings (SPR 71154); fix man pages (SPR 71239)
01n,24jul01,r_s  changed code to be ANSI compatible so that it compiles with
                 diab. made asm macro changes for diab
01m,09jun99,drm  Changing an "errno = " to errnoSet()
01l,01jun99,drm  Changing documentation for msgQDistReceive() to indicate that
                 the return value is the number of bytes received or ERROR
                 rather than OK or ERROR.
01k,24may99,drm  added vxfusion prefix to VxFusion related includes
01j,23feb99,drm  adding S_distLib_UNREACHABLE to documentation
01i,23feb99,drm  returning different errno when overallTimeout expires
01h,18feb99,wlf  doc cleanup
01g,28oct98,drm  documentation modifications
01f,12aug98,drm  added #include stmt for distLibP.h
01e,08may98,ur   removed 8 bit node id restriction
01d,15apr98,ur   retransmit errors, if failed to send/receive
01c,09apr98,ur   added some errno setting, for remote errors
01b,04mar98,ur   patched memory leak in msgQDistInput/RECV_REQ.
01a,06jun97,ur   written.
*/

/*
DESCRIPTION
This library provides the interface to distributed message queues.
Any task on any node in the system can send messages to or receive
from a distributed messsage queue. Full duplex communication between
two tasks generally requires two distributed messsage queues, one for
each direction.

Distributed messsage queues are created with msgQDistCreate().  After
creation, they can be manipulated using the generic routines for local
message queues; for more information on the use of these routines, see the
manual entry for msgQLib.  The msgQDistLib library also provides the 
msgQDistSend(), msgQDistReceive(), and msgQDistNumMsgs() routines which 
support additional parameters that are useful for working with distributed 
message queues.

The distributed objects message queue library is initialized by calling
distInit().

AVAILABILITY
This module is distributed as a component of the unbundled distributed
message queues option, VxFusion.

INCLUDE FILES: msgQDistLib.h

SEE ALSO: msgQLib, msgQDistShow, distLib
*/

#include "vxWorks.h"
#if defined (MSG_Q_DIST_REPORT) || defined (DIST_DIAGNOSTIC)
#include "stdio.h"
#endif
#include "stdlib.h"
#include "string.h"
#include "sllLib.h"
#include "errnoLib.h"
#include "msgQLib.h"
#include "semLib.h"
#include "taskLib.h"
#include "netinet/in.h"
#include "private/semLibP.h"
#include "private/msgQLibP.h"
#include "vxfusion/msgQDistLib.h"
#include "vxfusion/msgQDistGrpLib.h"
#include "vxfusion/distIfLib.h"
#include "vxfusion/distLib.h"
#include "vxfusion/distStatLib.h"
#include "vxfusion/private/msgQDistLibP.h"
#include "vxfusion/private/msgQDistGrpLibP.h"
#include "vxfusion/private/distPktLibP.h"
#include "vxfusion/private/distNetLibP.h"
#include "vxfusion/private/distLibP.h"

/* globals */

TBL_NODE                *pMsgQDistTbl;    /* windSh needs this global */

/* locals */

LOCAL SL_LIST            msgQDistTblFreeList;
LOCAL SEMAPHORE          msgQDistTblLock;
LOCAL int                msgQDistTblSize;
LOCAL BOOL               msgQDistLibInstalled = FALSE;

/* local prototypes */

LOCAL STATUS                msgQDistTblPut (MSG_Q_ID msgQId, TBL_IX *pTblIx);
#ifdef __SUPPORT_MSG_Q_DIST_DELETE
LOCAL STATUS                msgQDistTblDelete (TBL_IX tblIx);
#endif
LOCAL MSG_Q_ID              msgQDistTblGet (TBL_IX tblIx);
LOCAL DIST_STATUS           msgQDistInput (DIST_NODE_ID nodeIdSrc,
                                           DIST_TBUF_HDR *pTBufHdr);
LOCAL STATUS                msgQDistSendStatus (DIST_NODE_ID nodeIdDest,
                                                DIST_INQ_ID inqId,
                                                short error);
LOCAL DIST_MSG_Q_STATUS     msgQDistRecvReply (DIST_NODE_ID nodeIdReceiver,
                                               DIST_INQ_ID inqIdReceiver,
                                               MSG_Q_ID msgQId,
                                               char *buffer,
                                               UINT maxNBytes,
                                               int timeout,
                                               BOOL lastTry);
LOCAL DIST_MSG_Q_STATUS     msgQDistSendReply (DIST_NODE_ID nodeIdSender,
                                               DIST_INQ_ID inqIdSender,
                                               MSG_Q_ID msgQId,
                                               char *buffer,
                                               UINT nBytes,
                                               int timeout,
                                               int priority,
                                               BOOL lastTry);

/***************************************************************************
*
* msgQDistLibInit - initialize the distributed message queue package (VxFusion option)
*
* This routine initializes the distributed message queue package.
* It currently does nothing.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void msgQDistLibInit (void)
    {
    }

/***************************************************************************
*
* msgQDistInit - initialize distributed message queue library (VxFusion option)
*
* This routine initializes the distributed message queue library.
* It must be called before any other routine in the library.
* The argument <msgQDistMax> limits the number of distributed message
* queues created on this node. The maximum number of distributed message
* queues is DIST_MSG_Q_MAX_QS .
*
* RETURNS: OK or ERROR.
*
* NOMANUAL
*/

STATUS msgQDistInit
    (
    int        sizeLog2        /* create 2^^sizeLog2 msgQ's */
    )
    {
    TBL_IX    tblIx;
    STATUS    status;
    int       size;
    int       msgQDistMax;

    msgQDistMax = 1 << sizeLog2;

    if (msgQDistMax > DIST_MSG_Q_MAX_QS)
        {
#ifdef DIST_DIAGNOSTIC
        printf ("msgQDistInit: number of message queues is limited to %d\n",
                DIST_MSG_Q_MAX_QS);
#endif
        return (ERROR);    /* too many local msgQs for underlying layer */
        }

    if (msgQDistLibInstalled == TRUE)
        return (OK);

    if (distInqInit (DIST_INQ_HASH_TBL_SZ_LOG2) == ERROR)
        return (ERROR);

    size = MEM_ROUND_UP (sizeof (TBL_NODE));
    pMsgQDistTbl = (TBL_NODE *) malloc (msgQDistMax * size);
    if (pMsgQDistTbl == NULL)
        {
#ifdef DIST_DIAGNOSTIC
        printf ("msgQDistInit: memory allocation failed\n");
#endif
        return (ERROR);    /* out of memory */
        }

    semBInit (&msgQDistTblLock, SEM_Q_PRIORITY, SEM_EMPTY);
    msgQDistTblSize = msgQDistMax;

    sllInit (&msgQDistTblFreeList);
    for (tblIx = 0; tblIx < msgQDistMax; tblIx++)
        {
        pMsgQDistTbl[tblIx].tblIx = tblIx;
        sllPutAtHead (&msgQDistTblFreeList,
                      (SL_NODE *) &(pMsgQDistTbl[tblIx]));
        }

    msgQDistTblUnlock();

    /* Add message queue service to table of services. */

    status = distNetServAdd (DIST_PKT_TYPE_MSG_Q, msgQDistInput,
                             DIST_MSG_Q_SERV_NAME, DIST_MSG_Q_SERV_NET_PRIO,
                             DIST_MSG_Q_SERV_TASK_PRIO,
                             DIST_MSG_Q_SERV_TASK_STACK_SZ);

    if (status == ERROR)
        {
#ifdef DIST_DIAGNOSTIC
        printf ("msgQDistInit: cannot attach service\n");
#endif
        return (ERROR);
        }

    msgQDistSendRtn       = (FUNCPTR) msgQDistSend;
    msgQDistReceiveRtn    = (FUNCPTR) msgQDistReceive;
    msgQDistNumMsgsRtn    = (FUNCPTR) msgQDistNumMsgs;

    msgQDistLibInstalled = TRUE;
    return (OK);
    }

/***************************************************************************
*
* msgQDistCreate - create a distributed message queue (VxFusion option)
*
* This routine creates a distributed message queue capable of
* holding up to <maxMsgs> messages, each up to <maxMsgLength> bytes long.
* This routine returns a message queue ID used to identify the created
* message queue. The queue can be created with the following options:
* \is
* \i MSG_Q_FIFO (0x00)
* The queue pends tasks in FIFO order.
* \i MSG_Q_PRIORITY (0x01)
* The queue pends tasks in priority order. Remote tasks share the same
* priority level.
* \ie
*
* The global message queue identifier returned can be used directly by generic
* message queue handling routines in msgQLib, such as, msgQSend(), 
* msgQReceive(), and msgQNumMsgs().
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS:
* MSG_Q_ID, or NULL if there is an error.
*
* ERRNO:
* \is
* \i S_memLib_NOT_ENOUGH_MEMORY
* If the routine is unable to allocate memory for message queues and message 
* buffers.
* \i S_intLib_NOT_ISR_CALLABLE
* If the routine is called from an interrupt service routine.
* \i S_msgQLib_INVALID_QUEUE_TYPE
* If the type of queue is invalid.
* \i S_msgQDistLib_INVALID_MSG_LENGTH
* If the message is too long for the VxFusion network layer.
* \ie
*
* SEE ALSO: msgQLib
*/

MSG_Q_ID msgQDistCreate
    (
    int maxMsgs,         /* max messages that can be queued */
    int maxMsgLength,    /* max bytes in a message */
    int options          /* message queue options */
    )
    {
    DIST_OBJ_NODE *   pObjNode;
    MSG_Q_ID          msgQId;
    TBL_IX            tblIx;
    int               maxMsgLen;

    if (!msgQDistLibInstalled)
        return (NULL);    /* call msgQDistInit() first */

    maxMsgLen = (DIST_IF_MAX_FRAGS * (DIST_IF_MTU - DIST_IF_HDR_SZ)) -
                 DIST_PKT_HDR_SIZEOF (DIST_PKT_MSG_Q_SEND);

    if (maxMsgLength > maxMsgLen)
        {
        errnoSet (S_msgQDistLib_INVALID_MSG_LENGTH);
        return (NULL);    /* msg too long for network layer */
        }

    if (options &~ MSG_Q_TYPE_MASK)
        {
        errnoSet (S_msgQLib_INVALID_QUEUE_TYPE);
        return (NULL);    /* illegal option */
        }

    if ((msgQId = msgQCreate (maxMsgs, maxMsgLength, options)) == NULL)
        return (NULL);    /* msgQCreate() failed */

    if (msgQDistTblPut (msgQId, &tblIx) == ERROR)
        {
        msgQDelete (msgQId);
        return (NULL);    /* table full */
        }    

    pObjNode = distObjNodeGet();

    pObjNode->objNodeType      = DIST_OBJ_TYPE_MSG_Q;
    pObjNode->objNodeReside    = distNodeLocalGetId();
    pObjNode->objNodeId        = TBL_IX_TO_DIST_OBJ_ID (tblIx);

#ifdef MSG_Q_DIST_REPORT
    printf ("msgQDistCreate: dMsgQId 0x%lx, msgQId %p\n",
            dMsgQId, DIST_OBJ_NODE_TO_MSG_Q_ID (pObjNode));
#endif

    return (DIST_OBJ_NODE_TO_MSG_Q_ID (pObjNode));
    }

/***************************************************************************
*
* msgQDistSend - send a message to a distributed message queue (VxFusion option)
*
* This routine sends the message specified by <buffer> of length <nBytes> to 
* the distributed message queue or group specified by <msgQId>.
*
* The argument <msgQTimeout> specifies the time in ticks to wait for the 
* queuing of the message. The argument <overallTimeout> specifies the time in
* ticks to wait for both the sending and queuing of the message.
* While it is an error to set <overallTimeout> to NO_WAIT (0), 
* WAIT_FOREVER (-1) is allowed for both <msgQTimeout> and <overallTimeout>.
*
* The <priority> parameter specifies the priority of the message being sent.
* It ranges between DIST_MSG_PRI_0 (highest priority) and DIST_MSG_PRI_7 
* (lowest priority).  A priority of MSG_PRI_URGENT is mapped
* to DIST_MSG_PRI_0; MSG_PRI_NORMAL is mapped to DIST_MSG_PRI_4 .
* Messages sent with high priorities (DIST_MSG_PRI_0 to DIST_MSG_PRI_3)
* are put to the head of the list of queued messages.
* Lower priority messages (DIST_MSG_PRI_4 to DIST_MSG_PRI_7) are placed
* at the queue's tail.
*
* NOTE: When msgQDistSend() is called through msgQSend(), <msgQTimeout> is 
* set to <timeout> and <overallTimeout> to WAIT_FOREVER .
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: OK, or ERROR if the operation fails.
*
* ERRNO:
* \is
* \i S_distLib_OBJ_ID_ERROR
* The argument <msgQId> is invalid.
* \i S_distLib_UNREACHABLE
* Could not establish communications with the remote node.
* \i S_msgQDistLib_INVALID_PRIORITY
* The argument <priority> is invalid.
* \i S_msgQDistLib_INVALID_TIMEOUT
* The argument <overallTimeout> is NO_WAIT .
* \i S_msgQDistLib_RMT_MEMORY_SHORTAGE
* There is not enough memory on the remote node.
* \i S_objLib_OBJ_UNAVAILABLE
* The argument <msgQTimeout> is set to NO_WAIT, and the queue is full.
* \i S_objLib_OBJ_TIMEOUT
* The queue is full for <msgQTimeout> ticks.
* \i S_msgQLib_INVALID_MSG_LENGTH
* The argument <nBytes> is larger than the <maxMsgLength> set for the 
* message queue.
* \i S_msgQDistLib_OVERALL_TIMEOUT
* There was no response from the remote side in <overallTimeout> ticks.
* \ie
*
* SEE ALSO: msgQLib
*/

STATUS msgQDistSend
    (
    MSG_Q_ID    msgQId,         /* message queue on which to send */
    char *      buffer,         /* message to send                */
    UINT        nBytes,         /* length of message              */
    int         msgQTimeout,    /* ticks to wait at message queue */
    int         overallTimeout, /* ticks to wait overall          */
    int         priority        /* priority                       */
    )
    {
    DIST_MSG_Q_ID     dMsgQId;
    DIST_OBJ_NODE *   pObjNode;
    MSG_Q_ID          lclMsgQId;
    int               lclPriority;    /* MSG_PRI_URGENT or MSG_PRI_NORMAL */
    DIST_MSG_Q_GRP_ID    distMsgQGrpId;
    STATUS               status;

    DIST_PKT_MSG_Q_SEND    pktSendHdr;
    DIST_MSG_Q_SEND_INQ    inquiryNode;
    DIST_INQ_ID            inquiryId;
    DIST_IOVEC             distIOVec[2];

    if (DIST_OBJ_VERIFY (msgQId) == ERROR)
        {
        errnoSet (S_distLib_OBJ_ID_ERROR);
        return (ERROR);
        }

    if (overallTimeout == NO_WAIT)
        {
        errnoSet (S_msgQDistLib_INVALID_TIMEOUT);
        return (ERROR);    /* makes no sense */
        }

    if (! DIST_MSG_Q_PRIO_VERIFY (priority))
        {
        errnoSet (S_msgQDistLib_INVALID_PRIORITY);
        return (ERROR);    /* invalid priority */
        }

    pObjNode = MSG_Q_ID_TO_DIST_OBJ_NODE (msgQId);
    if (! IS_DIST_MSG_Q_OBJ (pObjNode))
        {
        errnoSet (S_distLib_OBJ_ID_ERROR);
        return (ERROR);    /* legal object id, but not a message queue */
        }

    dMsgQId = (DIST_MSG_Q_ID) pObjNode->objNodeId;

#ifdef MSG_Q_DIST_REPORT
    printf ("msgQDistSend: msgQId %p, dMsgQId 0x%lx\n", msgQId, dMsgQId);
#endif

    if (IS_DIST_MSG_Q_TYPE_GRP (dMsgQId))
        {
        /* Message queue id points to a group. */

        distMsgQGrpId = DIST_MSG_Q_ID_TO_DIST_MSG_Q_GRP_ID (dMsgQId);
        status = msgQDistGrpSend (distMsgQGrpId, buffer, nBytes,
                                  msgQTimeout, overallTimeout, priority);
#ifdef MSG_Q_DIST_REPORT
        printf ("msgQDistSend: msgQDistGrpSend returned = %d\n", status);
#endif
        return (status);
        }

    if (!IS_DIST_OBJ_LOCAL (pObjNode))
        {
        /* Message queue id points to a remote queue. */

        inquiryNode.sendInq.inqType = DIST_MSG_Q_INQ_TYPE_SEND;
        semBInit (&(inquiryNode.sendInqWait), SEM_Q_FIFO, SEM_EMPTY);
        inquiryNode.remoteError = FALSE;
        inquiryNode.sendInqMsgQueued = FALSE;
        inquiryNode.sendInqTask = taskIdSelf();

        inquiryId = distInqRegister ((DIST_INQ *) &inquiryNode);

        pktSendHdr.sendHdr.pktType = DIST_PKT_TYPE_MSG_Q;
        pktSendHdr.sendHdr.pktSubType = DIST_PKT_TYPE_MSG_Q_SEND;
        pktSendHdr.sendTblIx = DIST_MSG_Q_ID_TO_TBL_IX (dMsgQId);
        pktSendHdr.sendInqId = (uint32_t) inquiryId;
        pktSendHdr.sendTimeout =
                htonl ((uint32_t) DIST_TICKS_TO_MSEC (msgQTimeout));

        /* use IOV stuff here, since we do not want to copy data */

        distIOVec[0].pIOBuffer = &pktSendHdr;
        distIOVec[0].IOLen = DIST_PKT_HDR_SIZEOF (DIST_PKT_MSG_Q_SEND);
        distIOVec[1].pIOBuffer = buffer;
        distIOVec[1].IOLen = nBytes;

        status = distNetIOVSend (pObjNode->objNodeReside, &distIOVec[0], 2,
                                 WAIT_FOREVER,
                                 DIST_MSG_Q_PRIO_TO_NET_PRIO (priority));
        if (status == ERROR)
            {
            distInqCancel ((DIST_INQ *) &inquiryNode);
            errnoSet (S_distLib_UNREACHABLE);
            return (ERROR);
            }

        /*
         * semTake() blocks the requesting task until the service
         * task gives the semaphore, because the request has
         * been processed.
         */

        semTake (&(inquiryNode.sendInqWait), overallTimeout);

        distInqCancel ((DIST_INQ *) &inquiryNode);

        if (inquiryNode.sendInqMsgQueued)
            return (OK);

        /* If errno = S_objLib_OBJ_TIMEOUT, it could either be a result
         * of the timeout from the semaphore or the remote errno.  We must
         * check the remoteError flag of inquiryNode to determine what the
         * source of the error was.  If it is a result of the semaphore, we
         * will set errno to S_msgQDistLib_OVERALLTIMEOUT.  Otherwise, we'll
         * leave the errno as it is.
         */

        if (inquiryNode.remoteError == FALSE)
            errno = S_msgQDistLib_OVERALL_TIMEOUT;

        return (ERROR);
        }

    /* Message queue id points to a local queue. */

    lclPriority = DIST_MSG_Q_PRIO_TO_MSG_Q_PRIO (priority);
    lclMsgQId = msgQDistTblGet (DIST_MSG_Q_ID_TO_TBL_IX (dMsgQId));
    if (lclMsgQId == NULL)
        return (ERROR);    /* does not exist */

    if (msgQSend (lclMsgQId, buffer, nBytes, msgQTimeout, lclPriority)
          == ERROR)
        {
        return (ERROR);    /* error in msgQSend() */
        }

    return (OK);
    }

/***************************************************************************
*
* msgQDistReceive - receive a message from a distributed message queue (VxFusion option)
*
* This routine receives a message from the distributed message queue specified 
* by <msgQId>.  The received message is copied into the specified buffer, 
* <buffer>, which is <maxNBytes> in length.  If the message is longer than 
* <maxNBytes>, the remainder of the message is discarded (no error indication
* is returned).
*
* The argument <msgQTimeout> specifies the time in ticks to wait for the 
* queuing of the message. The argument <overallTimeout> specifies the time
* in ticks to wait for both the sending and queuing of the message.
* While it is an error to set <overallTimeout> to NO_WAIT (0), 
* WAIT_FOREVER (-1) is allowed for both <msgQTimeout> and <overallTimeout>.
*
* Calling msgQDistReceive() on a distributed message group returns an
* error.
*
* NOTE: When msgQDistReceive() is called through msgQReceive(), 
* <msgQTimeout> is set to <timeout> and <overallTimeout> to WAIT_FOREVER .
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: The number of bytes copied to <buffer>, or ERROR. 
*
* ERRNO:
* \is
* \i S_distLib_OBJ_ID_ERROR
* The argument <msgQId> is invalid.
* \i S_distLib_UNREACHABLE
* Could not establish communications with the remote node.
* \i S_msgQLib_INVALID_MSG_LENGTH
* The argument <maxNBytes> is less than 0.
* \i S_msgQDistLib_INVALID_TIMEOUT
* The argument <overallTimeout> is NO_WAIT .
* \i S_msgQDistLib_RMT_MEMORY_SHORTAGE
* There is not enough memory on the remote node.
* \i S_objLib_OBJ_UNAVAILABLE
* The argument <msgQTimeout> is set to NO_WAIT, and no messages are available.
* \i S_objLib_OBJ_TIMEOUT
* No messages were received in <msgQTimeout> ticks.
* \i S_msgQDistLib_OVERALL_TIMEOUT
* There was no response from the remote side in <overallTimeout> ticks.
* \ie
*
*
* SEE ALSO: msgQLib
*/

int msgQDistReceive
    (
    MSG_Q_ID    msgQId,         /* message queue from which to receive */
    char *      buffer,         /* buffer to receive message           */
    UINT        maxNBytes,      /* length of buffer                    */
    int         msgQTimeout,    /* ticks to wait at the message queue  */
    int         overallTimeout  /* ticks to wait overall               */
    )
    {
    DIST_MSG_Q_ID    dMsgQId;
    DIST_OBJ_NODE *  pObjNode;
    MSG_Q_ID         lclMsgQId;

    DIST_PKT_MSG_Q_RECV_REQ    pktReq;
    DIST_MSG_Q_RECV_INQ        inquiryNode;
    DIST_INQ_ID                inquiryId;
    STATUS                     status;
    int                        nBytes;

    if (DIST_OBJ_VERIFY (msgQId) == ERROR)
        {
        errnoSet (S_distLib_OBJ_ID_ERROR);
        return (ERROR);
        }

    if (overallTimeout == NO_WAIT)
        {
        errnoSet (S_msgQDistLib_INVALID_TIMEOUT);
        return (ERROR);    /* makes no sense */
        }

    /*
     * Even though <maxNBytes> is unsigned, check for < 0 to catch
     * possible caller errors.
     */

    if ((int) maxNBytes < 0)
        {
        errnoSet (S_msgQLib_INVALID_MSG_LENGTH);
        return (ERROR);
        }

    pObjNode = MSG_Q_ID_TO_DIST_OBJ_NODE (msgQId);
    if (! IS_DIST_MSG_Q_OBJ (pObjNode))
        {
        errnoSet (S_distLib_OBJ_ID_ERROR);
        return (ERROR); /* legal object id, but not a message queue */
        }

    dMsgQId = (DIST_MSG_Q_ID) pObjNode->objNodeId;

#ifdef MSG_Q_DIST_REPORT
    printf ("msgQDistReceive: msgQId %p, dMsgQId 0x%lx\n", msgQId, dMsgQId);
#endif

    if (IS_DIST_MSG_Q_TYPE_GRP (dMsgQId))
        {
        /* MSG_Q_ID is a group id. */

        errnoSet (S_msgQDistLib_NOT_GROUP_CALLABLE);
        return (ERROR);    /* error to call msgQReceive() on groups */
        }

    if (!IS_DIST_OBJ_LOCAL (pObjNode))
        {
        /*
         * Queue is remote.
         *
         * Create a inquiry node and send a request to the remote
         * node. Block until timeout exceeds or the request is
         * answered.
         */

        inquiryNode.recvInq.inqType = DIST_MSG_Q_INQ_TYPE_RECV;
        semBInit (&(inquiryNode.recvInqWait), SEM_Q_FIFO, SEM_EMPTY);
        inquiryNode.recvInqTask = taskIdSelf();
        inquiryNode.pRecvInqBuffer = buffer;
        inquiryNode.recvInqMaxNBytes = maxNBytes;
        inquiryNode.recvInqMsgArrived = FALSE;
        inquiryNode.remoteError = FALSE;

        inquiryId = distInqRegister ((DIST_INQ *) &inquiryNode);

        pktReq.recvReqHdr.pktType = DIST_PKT_TYPE_MSG_Q;
        pktReq.recvReqHdr.pktSubType = DIST_PKT_TYPE_MSG_Q_RECV_REQ;
        pktReq.recvReqTblIx = DIST_MSG_Q_ID_TO_TBL_IX (dMsgQId);
        pktReq.recvReqInqId = (uint32_t) inquiryId;
        pktReq.recvReqMaxNBytes = htonl ((uint32_t) maxNBytes);
        pktReq.recvReqTimeout = 
                htonl ((uint32_t) DIST_TICKS_TO_MSEC (msgQTimeout));

        status = distNetSend (pObjNode->objNodeReside, (DIST_PKT *) &pktReq,
                              DIST_PKT_HDR_SIZEOF (DIST_PKT_MSG_Q_RECV_REQ),
                              WAIT_FOREVER,
                              DIST_MSG_Q_RECV_PRIO);
        if (status == ERROR)
            {
            distInqCancel ((DIST_INQ *) &inquiryNode);
            errnoSet (S_distLib_UNREACHABLE);
            return (ERROR);
            }

        /*
         * semTake() blocks the requesting task until
         * the service task gives the semaphore, because
         * the request has been processed.
         */

        semTake (&(inquiryNode.recvInqWait), overallTimeout);

        if (inquiryNode.recvInqMsgArrived)
            {
            /*
             * If <recvInqMsgArrived> is true, <recvInqMaxNBytes> has
             * the number of bytes received.
             */

            nBytes = inquiryNode.recvInqMaxNBytes;

            distInqCancel ((DIST_INQ *) &inquiryNode);
            return (nBytes);
            }

        distInqCancel ((DIST_INQ *) &inquiryNode);

        /* If errno = S_objLib_OBJ_TIMEOUT, it could either be a result
         * of the timeout from the semaphore or the remote errno.  We must
         * check the remoteError flag of inquiryNode to determine what the
         * source of the error was.  If it is a result of the semaphore, we
         * will set errno to S_msgQDistLib_OVERALLTIMEOUT.  Otherwise, we'll
         * leave the errno as it is.
         */

        if (inquiryNode.remoteError == FALSE)
            errnoSet (S_msgQDistLib_OVERALL_TIMEOUT);

        return (ERROR);
        }

    /* The message queue is local to this node. This will be simple. */

    lclMsgQId = msgQDistTblGet (DIST_MSG_Q_ID_TO_TBL_IX(dMsgQId));
    if (lclMsgQId == NULL)
        {
#ifdef DIST_DIAGNOSTIC
        printf ("msgQDistReceive: distributed message queue does not exist\n");
#endif
        return (ERROR);    /* does not exist */
        }

    return (msgQReceive (lclMsgQId, buffer, maxNBytes, msgQTimeout));
    }

/***************************************************************************
*
* msgQDistNumMsgs - get the number of messages in a distributed message queue (VxFusion option)
*
* This routine returns the number of messages currently queued to a specified
* distributed message queue.
*
* NOTE:
* When msgQDistNumMsgs() is called through msgQNumMsgs(), <overallTimeout>
* is set to WAIT_FOREVER . You cannot set <overallTimeout> to NO_WAIT (0)
* because the process of sending a message from the local node to the remote
* node always takes a finite amount of time.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS:
* The number of messages queued, or ERROR if the operation fails.
*
* ERRNO:
* \is
* \i S_distLib_OBJ_ID_ERROR
* The argument <msgQId> is invalid.
* \i S_distLib_UNREACHABLE
* Could not establish communications with the remote node.
* \i S_msgQDistLib_INVALID_TIMEOUT
* The argument <overallTimeout> is NO_WAIT .
* \ie
*
* SEE ALSO: msgQLib
*/

int msgQDistNumMsgs
    (
    MSG_Q_ID    msgQId,            /* message queue to examine */
    int         overallTimeout     /* ticks to wait overall    */
    )
    {
    DIST_MSG_Q_ID    dMsgQId;
    DIST_OBJ_NODE *  pObjNode;
    MSG_Q_ID         lclMsgQId;

    DIST_PKT_MSG_Q_NUM_MSGS_REQ    pktReq;
    DIST_MSG_Q_NUM_MSGS_INQ        inquiryNode;
    DIST_INQ_ID                    inquiryId;
    int                            numMsgs;
    STATUS                         status;

    if (DIST_OBJ_VERIFY (msgQId) == ERROR)
        {
        errnoSet (S_distLib_OBJ_ID_ERROR);
        return (ERROR);
        }

    if (overallTimeout == NO_WAIT)
        {
        errnoSet (S_msgQDistLib_INVALID_TIMEOUT);
        return (ERROR);    /* makes no sense */
        }

    pObjNode = MSG_Q_ID_TO_DIST_OBJ_NODE (msgQId);
    if (! IS_DIST_MSG_Q_OBJ (pObjNode))
        {
        errnoSet (S_distLib_OBJ_ID_ERROR);
        return (ERROR);    /* legal object id, but not a message queue */
        }

    dMsgQId = (DIST_MSG_Q_ID) pObjNode->objNodeId;

#ifdef MSG_Q_DIST_REPORT
    printf ("msgQDistNumMsgs: msgQId %p, dMsgQId 0x%lx\n", msgQId, dMsgQId);
#endif

    if (IS_DIST_MSG_Q_TYPE_GRP (dMsgQId))
        {
        errnoSet (S_msgQDistLib_NOT_GROUP_CALLABLE);
        return (ERROR);    /* error to call msgQNumMsgs() on groups */
        }

    if (!IS_DIST_OBJ_LOCAL (pObjNode))    /* message queue is remote */
        {

        inquiryNode.numMsgsInq.inqType = DIST_MSG_Q_INQ_TYPE_NUM_MSGS;
        semBInit (&(inquiryNode.numMsgsInqWait), SEM_Q_FIFO, SEM_EMPTY);
        inquiryNode.numMsgsInqNum = ERROR;
        inquiryNode.numMsgsInqTask = taskIdSelf();

        inquiryId = distInqRegister ((DIST_INQ *) &inquiryNode);
        
        pktReq.numMsgsReqHdr.pktType = DIST_PKT_TYPE_MSG_Q;
        pktReq.numMsgsReqHdr.pktSubType = DIST_PKT_TYPE_MSG_Q_NUM_MSGS_REQ;
        pktReq.numMsgsReqTblIx = DIST_MSG_Q_ID_TO_TBL_IX (dMsgQId);
        pktReq.numMsgsReqInqId = (uint32_t) inquiryId;

        status = distNetSend (pObjNode->objNodeReside, (DIST_PKT *) &pktReq,
                DIST_PKT_HDR_SIZEOF (DIST_PKT_MSG_Q_NUM_MSGS_REQ),
                WAIT_FOREVER, DIST_MSG_Q_NUM_MSGS_PRIO);
        if (status == ERROR)
            {
            distInqCancel ((DIST_INQ *) &inquiryNode);
            errnoSet (S_distLib_UNREACHABLE);
            return (ERROR);
            }

        semTake (&(inquiryNode.numMsgsInqWait), overallTimeout);

        numMsgs = inquiryNode.numMsgsInqNum;
        distInqCancel ((DIST_INQ *) &inquiryNode);

        return (numMsgs);
        }

    lclMsgQId = msgQDistTblGet (DIST_MSG_Q_ID_TO_TBL_IX(dMsgQId));
    if (lclMsgQId == NULL)
        return (ERROR);    /* does not exist */

    return (msgQNumMsgs (lclMsgQId));
    }

/***************************************************************************
*
* msgQDistGetMapped - retrieve entry from distributed msgQ table (VxFusion option)
*
* This routine gets an entry from the distributed message queue table.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: MSG_Q_ID, or NULL.
*
* NOMANUAL
*/

MSG_Q_ID msgQDistGetMapped
    (
    MSG_Q_ID msgQId     /* msgQ ID to map */
    )
    {
    DIST_MSG_Q_ID     dMsgQId;
    DIST_OBJ_NODE *   pObjNode;
    TBL_IX            tblIx;
    
    if (DIST_OBJ_VERIFY (msgQId) == ERROR)
        return (NULL);

    pObjNode = MSG_Q_ID_TO_DIST_OBJ_NODE (msgQId);

    if (! IS_DIST_MSG_Q_OBJ (pObjNode))
        return (NULL); /* legal object id, but not a message queue */

    if (! IS_DIST_OBJ_LOCAL (pObjNode))
        return (NULL);

    dMsgQId = (DIST_MSG_Q_ID) pObjNode->objNodeId;

    if (IS_DIST_MSG_Q_TYPE_GRP (dMsgQId))
        return (NULL);

    tblIx = DIST_MSG_Q_ID_TO_TBL_IX (dMsgQId);
    return (msgQDistTblGet (tblIx));
    }

/***************************************************************************
*
* msgQDistTblPut - put a message queue to the queue table (VxFusion option)
*
* This routine puts a MSG_Q_ID in the queue table.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: OK, if successful
*
* NOMANUAL
*/

LOCAL STATUS msgQDistTblPut
    (
    MSG_Q_ID  msgQId,      /* ID to put in table */
    TBL_IX *  pTblIx       /* where to return index in table */
    )
    {
    TBL_NODE * pNode;

    msgQDistTblLock();
    pNode = (TBL_NODE *) sllGet (&msgQDistTblFreeList);
    msgQDistTblUnlock();

    if (pNode == NULL)
        return (ERROR);    /* all elements of the table are in use */

    pNode->tblMsgQId = msgQId;
    *pTblIx = pNode->tblIx;

#ifdef MSG_Q_DIST_REPORT
    printf ("msgQDistTblPut: pTblNode %p (tblIx 0x%x), msgQId 0x%lx\n", 
            pNode, pNode->tblIx, (uint32_t) pNode->tblMsgQId);
#endif

    return (OK);
    }

#ifdef __SUPPORT_MSG_Q_DIST_DELETE

/***************************************************************************
*
* msgQDistTblDelete - delete a message queue from the table (VxFusion option)
*
* This routine deletes in queue ID at table index <tblIx>.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: OK, if successfully deleted.
*
* NOMANUAL
*/

LOCAL STATUS msgQDistTblDelete
    (
    TBL_IX tblIx      /* index in queue table */
    )
    {
    TBL_NODE * pNode;

    if (tblIx >=  msgQDistTblSize)
        return(ERROR);    /* invalid argument */

    pNode = &(pMsgQDistTbl[tblIx]);

    msgQDistTblLock();
    sllPutAtHead (&msgQDistTblFreeList, (SL_NODE *) pNode);
    msgQDistTblUnlock();

    return(OK);
    }

#endif /* __SUPPORT_MSG_Q_DIST_DELETE */

/***************************************************************************
*
*  msgQDistTblGet - get message queue ID from table (VxFusion option)
*
* This routine takes a message queue table index, <tblIx>, and returns
* the corresponding MSG_Q_ID, or NULL.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: MSG_Q_ID or NULL.
*
* NOMANUAL
*/

LOCAL MSG_Q_ID msgQDistTblGet
    (
    TBL_IX tblIx            /* index in queue table */
    )
    {
    if (tblIx >=  msgQDistTblSize)
        return(NULL);    /* invalid argument */

#ifdef MSG_Q_DIST_REPORT
    printf ("msgQDistTblGet: tblIx 0x%x, msgQId 0x%lx\n", 
            tblIx, (uint32_t) pMsgQDistTbl[tblIx].tblMsgQId);
#endif

    return (pMsgQDistTbl[tblIx].tblMsgQId);
    }

/***************************************************************************
*
*  msgQDistInput - called everytime a new message arrives at the system (VxFusion option)
*
* This routine processes messages received by VxFusion.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: The status of message processing.
*
* NOMANUAL
*/

LOCAL DIST_STATUS msgQDistInput
    (
    DIST_NODE_ID      nodeIdSrc,    /* source node ID */
    DIST_TBUF_HDR *   pTBufHdr      /* ptr to the message */
    )
    {
    DIST_PKT *   pPkt;
    int          pktLen;

            DIST_PKT_MSG_Q_SEND    pktSend;
            DIST_INQ_ID            inqIdSrc;
            MSG_Q_ID               msgQId;
            char *                 buffer;
            UINT                   nBytes;
            int                    prio;
            int                    ret;
            int                    tid;

            DIST_PKT_MSG_Q_RECV_REQ  pktReq;    /* incoming request packet */
            UINT                     maxBytes;

    pktLen = pTBufHdr->tBufHdrOverall;

    if (pktLen < sizeof (DIST_PKT))
        distPanic ("msgQDistInput: packet too short\n");

    pPkt = (DIST_PKT *) ((DIST_TBUF_GET_NEXT (pTBufHdr))->pTBufData);

    switch (pPkt->pktSubType)
        {
        case DIST_PKT_TYPE_MSG_Q_SEND:
            {
            /*
             * Received a message from a remote sender.
             * Find id of local message queue, and call msgQSend().
             */


            if (pktLen < DIST_PKT_HDR_SIZEOF (DIST_PKT_MSG_Q_SEND))
                distPanic ("msgQDistInput/SEND: packet too short\n");

            distTBufCopy (DIST_TBUF_GET_NEXT (pTBufHdr), 0,
                          (char *) &pktSend,
                          DIST_PKT_HDR_SIZEOF (DIST_PKT_MSG_Q_SEND));

            inqIdSrc = (DIST_INQ_ID) pktSend.sendInqId;
            nBytes = pktLen - DIST_PKT_HDR_SIZEOF (DIST_PKT_MSG_Q_SEND);

            /*
             * Using malloc() here is not very satisfiing. Maybe we can
             * extend msgQLib with a routine, that directly sends a list
             * of tBufs to a message queue.
             */

            if ((buffer = (char *) malloc (nBytes)) == NULL)
                {
#ifdef MSG_Q_DIST_REPORT
                printf ("msgQDistInput/SEND: out of memory\n");
#endif
                msgQDistSendStatus (nodeIdSrc, inqIdSrc,
                                    MSG_Q_DIST_STATUS_NOT_ENOUGH_MEMORY);

                return (MSG_Q_DIST_STATUS_NOT_ENOUGH_MEMORY);
                }

            distTBufCopy (DIST_TBUF_GET_NEXT (pTBufHdr),
                          DIST_PKT_HDR_SIZEOF (DIST_PKT_MSG_Q_SEND),
                          buffer, nBytes);

            prio = NET_PRIO_TO_DIST_MSG_Q_PRIO (pTBufHdr->tBufHdrPrio);
            msgQId = msgQDistTblGet (pktSend.sendTblIx);
            if (msgQId == NULL)
                {
                free (buffer);

#ifdef MSG_Q_DIST_REPORT
                printf ("msgQDistInput/SEND: unknown message queue id\n");
#endif
                msgQDistSendStatus (nodeIdSrc, inqIdSrc,
                                    MSG_Q_DIST_STATUS_ILLEGAL_OBJ_ID);

                return (MSG_Q_DIST_STATUS_ILLEGAL_OBJ_ID);
                }

            /*
             * First try to send with NO_WAIT.  We set the lastry
             * argument to FALSE, because we will try again.
             */

            ret = msgQDistSendReply (nodeIdSrc, inqIdSrc, msgQId, buffer,
                                     nBytes, NO_WAIT, prio, FALSE);
            switch (ret)
                {
                case MSG_Q_DIST_STATUS_OK:
                    free (buffer);
                    return (MSG_Q_DIST_STATUS_OK);

                case MSG_Q_DIST_STATUS_ERROR:
                    free (buffer);
                    return (MSG_Q_DIST_STATUS_ERROR);

                case MSG_Q_DIST_STATUS_UNAVAIL:
                    {
                    int            timeout;

                    timeout =
                        DIST_MSEC_TO_TICKS (ntohl (pktSend.sendTimeout));

                    if (timeout != NO_WAIT)
                        {
#ifdef MSG_Q_DIST_REPORT
                        printf ("msgQDistInput/SEND: timeout = %d\n", timeout);
#endif
                        /*
                         * Send with NO_WAIT has failed and user
                         * supplied timeout differs from NO_WAIT.
                         * Spawn a task and wait on message queue.
                         */
                        
                        tid = taskSpawn (NULL,
                                         DIST_MSG_Q_WAIT_TASK_PRIO,
                                         0,
                                         DIST_MSG_Q_WAIT_TASK_STACK_SZ,
                                         (FUNCPTR) msgQDistSendReply,
                                         (int) nodeIdSrc,
                                         (int) inqIdSrc,
                                         (int) msgQId,
                                         (int) buffer,
                                         nBytes,
                                         timeout,
                                         prio,
                                         TRUE,
                                         0, 0);
                        if (tid != ERROR)
                            {
                            /* msgQDistSendReply () frees <buffer> */

                            return (MSG_Q_DIST_STATUS_OK);
                            }
                        }

                    free (buffer);

                    /* For this case where the user specified NO_WAIT
                     * we must send back a status now.  We didn't do this
                     * in msgQDistSendReply() because we didn't know if
                     * the user specified NO_WAIT or whether it was the first
                     * try before spawning a task. 
                     */

                    msgQDistSendStatus (nodeIdSrc, inqIdSrc, (INT16) ret);
                    return (MSG_Q_DIST_STATUS_UNAVAIL);
                    }

                default:
                    free (buffer);
#ifdef MSG_Q_DIST_REPORT
                    printf ("msgQDistInput/SEND: illegal status\n");
#endif
                    return (MSG_Q_DIST_STATUS_INTERNAL_ERROR);
                }
            }

        case DIST_PKT_TYPE_MSG_Q_RECV_REQ:
            {
            /*
             * A remote node requests to receive data from local queue.
             * Find id of local message queue, and call msgQReceive().
             */

            DIST_MSG_Q_STATUS        ret;

            if (pktLen != DIST_PKT_HDR_SIZEOF (DIST_PKT_MSG_Q_RECV_REQ))
                distPanic ("msgQDistInput/RECV_REQ: packet too short\n");

            distTBufCopy (DIST_TBUF_GET_NEXT (pTBufHdr), 0, (char *) &pktReq,
                          DIST_PKT_HDR_SIZEOF (DIST_PKT_MSG_Q_RECV_REQ));

            inqIdSrc = (DIST_INQ_ID) pktReq.recvReqInqId;

            msgQId = msgQDistTblGet (pktReq.recvReqTblIx);
            if (msgQId == NULL)
                {
#ifdef MSG_Q_DIST_REPORT
                printf ("msgQDistInput/RECV_REQ: unknown message queue id\n");
#endif
                msgQDistSendStatus (nodeIdSrc, inqIdSrc,
                                    MSG_Q_DIST_STATUS_ILLEGAL_OBJ_ID);

                return (MSG_Q_DIST_STATUS_ILLEGAL_OBJ_ID);
                }

            maxBytes = ntohl ((uint32_t) pktReq.recvReqMaxNBytes);

            buffer = (char *) malloc (maxBytes);
            if (buffer == NULL)
                {
#ifdef MSG_Q_DIST_REPORT
                printf ("msgQDistInput/RECV_REQ: out of memory\n");
#endif
                msgQDistSendStatus (nodeIdSrc, inqIdSrc,
                                    MSG_Q_DIST_STATUS_NOT_ENOUGH_MEMORY);

                return (MSG_Q_DIST_STATUS_NOT_ENOUGH_MEMORY);
                }

            /* First try to receive a message with NO_WAIT. */
            
            ret = msgQDistRecvReply (nodeIdSrc, inqIdSrc, msgQId,
                                     buffer, maxBytes, NO_WAIT,
                                     FALSE);

            switch (ret)
                {
                case MSG_Q_DIST_STATUS_OK:
                    free (buffer);
                    return (MSG_Q_DIST_STATUS_OK);

                case MSG_Q_DIST_STATUS_ERROR:
                    free (buffer);
                    return (MSG_Q_DIST_STATUS_ERROR);

                case MSG_Q_DIST_STATUS_UNAVAIL:
                    {
                    uint32_t    timeout_msec;
                    int            timeout;

                    timeout_msec = ntohl ((uint32_t) pktReq.recvReqTimeout);
                    timeout = DIST_MSEC_TO_TICKS (timeout_msec);

                    if (timeout != NO_WAIT)
                        {
                        /*
                         * Receiving with NO_WAIT has failed and user
                         * supplied timeout differs from NO_WAIT.
                         * Spawn a task and wait on message queue.
                         */

                        int tid;
#ifdef MSG_Q_DIST_REPORT
                        printf ("msgQDistInput/RECV_REQ: timeout = %d\n",
                                timeout);
#endif
                        tid = taskSpawn (NULL,
                                         DIST_MSG_Q_WAIT_TASK_PRIO,
                                         0,
                                         DIST_MSG_Q_WAIT_TASK_STACK_SZ,
                                        /* task entry point */
                                        (FUNCPTR) msgQDistRecvReply,
                                        /* who is the receiver */
                                        (int) nodeIdSrc,
                                        (int) inqIdSrc,
                                        /* receiving options */
                                        (int) msgQId,
                                        (int) buffer,
                                        maxBytes,
                                        timeout,
                                        /* some options */
                                        TRUE /* lastTry */,
                                        0, 0, 0);

                        if (tid != ERROR)
                            return (MSG_Q_DIST_STATUS_OK);
                        }

                    free (buffer);

                    /* For this case where the user specified NO_WAIT
                     * we must send back a status now.  We didn't do this
                     * in msgQDistRecvReply() because we didn't know if
                     * the user specified NO_WAIT or whether it was the first
                     * try before spawning a task. 
                     */

                    msgQDistSendStatus (nodeIdSrc, inqIdSrc, ret);
                    return (MSG_Q_DIST_STATUS_UNAVAIL);
                    }

                default:
#ifdef MSG_Q_DIST_REPORT
                    printf ("msgQDistInput/SEND: illegal status\n");
#endif
                    free (buffer);
                    return (MSG_Q_DIST_STATUS_INTERNAL_ERROR);
                }
            }

        case DIST_PKT_TYPE_MSG_Q_RECV_RPL:
            {
            DIST_PKT_MSG_Q_RECV_RPL    pktRpl;
            DIST_MSG_Q_RECV_INQ *      pInq;
            DIST_INQ_ID                inqId;
            int                        nBytes;

            if (pktLen < DIST_PKT_HDR_SIZEOF (DIST_PKT_MSG_Q_RECV_RPL))
                distPanic ("msgQDistInput/RECV_RPL: packet too short\n");

            /* First copy the reply header. */
            distTBufCopy (DIST_TBUF_GET_NEXT (pTBufHdr), 0,
                          (char *) &pktRpl,
                          DIST_PKT_HDR_SIZEOF (DIST_PKT_MSG_Q_RECV_RPL));

            inqId = (DIST_INQ_ID) pktRpl.recvRplInqId;
            if (! (pInq = (DIST_MSG_Q_RECV_INQ *) distInqFind (inqId)))
                return (MSG_Q_DIST_STATUS_LOCAL_TIMEOUT);

            /* Now copy message directly to user's buffer. */
            
            nBytes = distTBufCopy (DIST_TBUF_GET_NEXT (pTBufHdr),
                                   DIST_PKT_HDR_SIZEOF (DIST_PKT_MSG_Q_RECV_RPL),
                                   pInq->pRecvInqBuffer, pInq->recvInqMaxNBytes);

            pInq->recvInqMaxNBytes = nBytes;
            pInq->recvInqMsgArrived = TRUE;

            semGive (&pInq->recvInqWait);
            return (MSG_Q_DIST_STATUS_OK);
            }

        case DIST_PKT_TYPE_MSG_Q_NUM_MSGS_REQ:
            {
            /*
             * Remote note requests numMsgs service from local queue.
             * Find id of local message queue, and call msgQNumMsgs().
             */

            DIST_PKT_MSG_Q_NUM_MSGS_REQ    pktReq;
            DIST_PKT_MSG_Q_NUM_MSGS_RPL    pktRpl;
            DIST_INQ_ID                    inqIdSrc;    /* remote inquiry id */
            MSG_Q_ID                       lclMsgQId;
            STATUS                         status;

            if (pktLen != DIST_PKT_HDR_SIZEOF (DIST_PKT_MSG_Q_NUM_MSGS_REQ))
                distPanic ("msgQDistInput/NUM_MSGS_REQ: packet too short\n");

            distTBufCopy (DIST_TBUF_GET_NEXT (pTBufHdr), 0,
                          (char *) &pktReq,
                          DIST_PKT_HDR_SIZEOF (DIST_PKT_MSG_Q_NUM_MSGS_REQ));

            inqIdSrc = (DIST_INQ_ID) pktReq.numMsgsReqInqId;

            lclMsgQId = msgQDistTblGet (pktReq.numMsgsReqTblIx);
            if (lclMsgQId == NULL)
                {
#ifdef MSG_Q_DIST_REPORT
                printf ("msgQDistInput/RECV_REQ: unknown message queue id\n");
#endif
                msgQDistSendStatus (nodeIdSrc, inqIdSrc,
                                    MSG_Q_DIST_STATUS_ILLEGAL_OBJ_ID);
                return (MSG_Q_DIST_STATUS_ILLEGAL_OBJ_ID);
                }

            pktRpl.numMsgsRplHdr.pktType = DIST_PKT_TYPE_MSG_Q;
            pktRpl.numMsgsRplHdr.pktSubType = DIST_PKT_TYPE_MSG_Q_NUM_MSGS_RPL;
            pktRpl.numMsgsRplInqId = (uint32_t) inqIdSrc;
            pktRpl.numMsgsRplNum = htonl (msgQNumMsgs (lclMsgQId));

            status = distNetSend (nodeIdSrc, (DIST_PKT *) &pktRpl,
                                  DIST_PKT_HDR_SIZEOF (DIST_PKT_MSG_Q_NUM_MSGS_RPL),
                                  WAIT_FOREVER, DIST_MSG_Q_NUM_MSGS_PRIO);

            if (status == ERROR)
                {
#ifdef DIST_DIAGNOSTIC
                distLog ("msgQDistInput: reply to NumMsgsReq failed\n");
#endif
                return (MSG_Q_DIST_STATUS_UNREACH);
                }

            return (MSG_Q_DIST_STATUS_OK);
            }

        case DIST_PKT_TYPE_MSG_Q_NUM_MSGS_RPL:
            {
            DIST_PKT_MSG_Q_NUM_MSGS_RPL    pktRpl;
            DIST_MSG_Q_NUM_MSGS_INQ *      pInq;
            DIST_INQ_ID                    inqId;

            if (pktLen != DIST_PKT_HDR_SIZEOF (DIST_PKT_MSG_Q_NUM_MSGS_RPL))
                distPanic ("msgQDistInput/NUM_MSGS_RPL: packet too short\n");

            distTBufCopy (DIST_TBUF_GET_NEXT (pTBufHdr), 0,
                    (char *) &pktRpl,
                    DIST_PKT_HDR_SIZEOF (DIST_PKT_MSG_Q_NUM_MSGS_RPL));

            inqId = (DIST_INQ_ID) pktRpl.numMsgsRplInqId;
            if (! (pInq = (DIST_MSG_Q_NUM_MSGS_INQ *) distInqFind (inqId)))
                return (MSG_Q_DIST_STATUS_LOCAL_TIMEOUT);

            pInq->numMsgsInqNum = (int) ntohl (pktRpl.numMsgsRplNum);

            semGive (&(pInq->numMsgsInqWait));

            return (MSG_Q_DIST_STATUS_OK);
            }

        case DIST_PKT_TYPE_MSG_Q_STATUS:
            {
            DIST_PKT_MSG_Q_STATUS    pktStatus;
            DIST_MSG_Q_STATUS        msgQStatus;
            DIST_INQ_ID              inqId;
            DIST_INQ *               pGenInq;
            int                      errnoRemote;

            if (pktLen != DIST_PKT_HDR_SIZEOF (DIST_PKT_MSG_Q_STATUS))
                distPanic ("msgQDistInput/STATUS: packet too short\n");

            /* First copy the error packet form the TBuf list. */
            
            distTBufCopy (DIST_TBUF_GET_NEXT (pTBufHdr), 0,
                          (char *) &pktStatus,
                           DIST_PKT_HDR_SIZEOF (DIST_PKT_MSG_Q_STATUS));

            msgQStatus = (DIST_MSG_Q_STATUS) ntohs (pktStatus.statusDStatus);
            errnoRemote = ntohl (pktStatus.statusErrno);

            inqId = (DIST_INQ_ID) pktStatus.statusInqId;
            if (! (pGenInq = distInqFind (inqId)))
                return (MSG_Q_DIST_STATUS_LOCAL_TIMEOUT);

            /* See who is addressed by the STATUS telegram. */
            
            switch (pGenInq->inqType)
                {
                case DIST_MSG_Q_INQ_TYPE_NUM_MSGS:
                    {
                    DIST_MSG_Q_NUM_MSGS_INQ    *pInq;

                    pInq = (DIST_MSG_Q_NUM_MSGS_INQ *) pGenInq;

                    /*
                     * Possible errors are:
                     *    MSG_Q_DIST_STATUS_ILLEGAL_OBJ_ID
                     */
                     
                    switch (msgQStatus)
                        {
                        case MSG_Q_DIST_STATUS_ILLEGAL_OBJ_ID:
                            errnoOfTaskSet (pInq->numMsgsInqTask,
                                            errnoRemote);
                            break;
                        default:
#ifdef MSG_Q_DIST_REPORT
                            printf ("msgQDistInput/STATUS/NUM_MSGS: status?\n");
#endif
                            break;
                        }

                    semGive (&pInq->numMsgsInqWait);
                    return (MSG_Q_DIST_STATUS_OK);
                    }

                case DIST_MSG_Q_INQ_TYPE_SEND:
                    {
                    DIST_MSG_Q_SEND_INQ * pInq;

                    pInq = (DIST_MSG_Q_SEND_INQ *) pGenInq;

                    /*
                     * Possible errors here:
                     *    MSG_Q_DIST_STATUS_OK
                     *    MSG_Q_DIST_STATUS_ERROR
                     *    MSG_Q_DIST_STATUS_UNAVAIL
                     *    MSG_Q_DIST_STATUS_TIMEOUT
                     *     MSG_Q_DIST_STATUS_ILLEGAL_OBJ_ID
                     *    MSG_Q_DIST_STATUS_NOT_ENOUGH_MEMORY
                     */

                    switch (msgQStatus)
                        {
                        case MSG_Q_DIST_STATUS_OK:
                            pInq->sendInqMsgQueued = TRUE;
                            break;
                        case MSG_Q_DIST_STATUS_ERROR:
                        case MSG_Q_DIST_STATUS_UNAVAIL:
                        case MSG_Q_DIST_STATUS_TIMEOUT:
                            pInq->remoteError = TRUE;
                            errnoOfTaskSet (pInq->sendInqTask, errnoRemote);
                            break;

                        case MSG_Q_DIST_STATUS_ILLEGAL_OBJ_ID:
                            pInq->remoteError = TRUE;    
                            errnoOfTaskSet (pInq->sendInqTask,
                                            S_distLib_OBJ_ID_ERROR);
                            break;

                        case MSG_Q_DIST_STATUS_NOT_ENOUGH_MEMORY:
                            pInq->remoteError = TRUE;
                            errnoOfTaskSet (pInq->sendInqTask,
                                         S_msgQDistLib_RMT_MEMORY_SHORTAGE);
                            break;

                        default:
#ifdef MSG_Q_DIST_REPORT
                            printf ("msgQDistInput/STATUS/SEND: status?\n");
#endif
                            break;
                        }

                    semGive (&pInq->sendInqWait);
                    return (MSG_Q_DIST_STATUS_OK);
                    }

                case DIST_MSG_Q_INQ_TYPE_RECV:
                    {
                    DIST_MSG_Q_RECV_INQ * pInq;
                    
                    pInq = (DIST_MSG_Q_RECV_INQ *) pGenInq;

                    /*
                     * Possible errors here:
                     *    MSG_Q_DIST_STATUS_ERROR
                     *    MSG_Q_DIST_STATUS_UNAVAIL
                     *    MSG_Q_DIST_STATUS_TIMEOUT
                     *    MSG_Q_DIST_STATUS_ILLEGAL_OBJ_ID
                     *    MSG_Q_DIST_STATUS_NOT_ENOUGH_MEMORY
                     */

                    switch (msgQStatus)
                        {
                        case MSG_Q_DIST_STATUS_UNAVAIL:
                        case MSG_Q_DIST_STATUS_TIMEOUT:
                        case MSG_Q_DIST_STATUS_ERROR:
                            pInq->remoteError = TRUE;
                            errnoOfTaskSet (pInq->recvInqTask, errnoRemote);
                            break;

                        case MSG_Q_DIST_STATUS_ILLEGAL_OBJ_ID:
                            pInq->remoteError = TRUE;
                            errnoOfTaskSet (pInq->recvInqTask,
                                            S_distLib_OBJ_ID_ERROR);
                            break;

                        case MSG_Q_DIST_STATUS_NOT_ENOUGH_MEMORY:
                            pInq->remoteError = TRUE;
                            errnoOfTaskSet (pInq->recvInqTask,
                                          S_msgQDistLib_RMT_MEMORY_SHORTAGE);
                            break;

                        default:
#ifdef MSG_Q_DIST_REPORT
                            printf ("msgQDistInput/STATUS/RECV_REQ: status?\n");
#endif
                            break;
                        }

                    semGive (&pInq->recvInqWait);
                    return (MSG_Q_DIST_STATUS_OK);
                    }

                default:
#ifdef MSG_Q_DIST_REPORT
                    printf ("msgQDistInput/STATUS: unexpected inquiry (%d)\n",
                            pGenInq->inqType);
#endif
                    return (MSG_Q_DIST_STATUS_INTERNAL_ERROR);
                }
            }

        default:
#ifdef MSG_Q_DIST_REPORT
            printf ("msgQDistInput: unknown message queue subtype (%d)\n",
                    pPkt->pktSubType);
#endif
            return (MSG_Q_DIST_STATUS_PROTOCOL_ERROR);
        }
    }

/***************************************************************************
*
* msgQDistSendReply - send message to message queue and respond (VxFusion option)
*
* This routine is used internally to do a msgQSend().
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: Status of message processsing.
*
* NOMANUAL
*/

LOCAL DIST_MSG_Q_STATUS msgQDistSendReply
    (
    DIST_NODE_ID        nodeIdSender,   /* sending node ID */
    DIST_INQ_ID         inqIdSender,    /* sending inquiry ID */
    MSG_Q_ID            msgQId,         /* ID destination Q */
    char *              buffer,         /* start of data to send */
    UINT                nBytes,         /* number of bytes to send */
    int                 timeout,        /* msgQSend() timeout */
    int                 priority,       /* message priority */
    BOOL                lastTry         /* clean-up if this is last attempt */
    )
    {
    STATUS               status;
    DIST_MSG_Q_STATUS    msgQStatus = MSG_Q_DIST_STATUS_OK;

    status = msgQSend (msgQId, buffer, nBytes, timeout, priority);


    if (status == ERROR)
        {
        switch (errnoGet())
            {
            case S_objLib_OBJ_UNAVAILABLE:    /* timeout == NO_WAIT */
                msgQStatus = MSG_Q_DIST_STATUS_UNAVAIL;
                break;
            case S_objLib_OBJ_TIMEOUT:        /* timeout != NO_WAIT */
                msgQStatus = MSG_Q_DIST_STATUS_TIMEOUT;
                break;
            default:
                msgQStatus = MSG_Q_DIST_STATUS_ERROR;
            }
        }
    
    if (lastTry)
        {
        free (buffer);
        msgQDistSendStatus (nodeIdSender, inqIdSender, msgQStatus);
        }
    else
        {
        if (msgQStatus != MSG_Q_DIST_STATUS_UNAVAIL)
            msgQDistSendStatus (nodeIdSender, inqIdSender, msgQStatus);
        }
    return (msgQStatus);
    }

/***************************************************************************
*
* msgQDistRecvReply - receive message from message queue and respond (VxFusion option)
*
* This routine is used internally to call msgQReceive().
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: Status of operation.
*
* NOMANUAL
*/

LOCAL DIST_MSG_Q_STATUS msgQDistRecvReply
    (
    DIST_NODE_ID    nodeIdRespond,   /* node wanting the message */
    DIST_INQ_ID     inqIdRespond,    /* inquiry ID of that node */
    MSG_Q_ID        msgQId,          /* message Q to read */
    char *          buffer,          /* start of buffer for data */
    UINT            maxNBytes,       /* maximum number of bytes to read */
    int             timeout,         /* timeout for msgQReceive() */
    BOOL            lastTry          /* clean-up, if last attempt */
    )
    {
    DIST_PKT_MSG_Q_RECV_RPL  pktRpl;
    DIST_MSG_Q_STATUS        msgQStatus;
    DIST_IOVEC               distIOVec[2];
    STATUS                   status;
    int                      nBytes;

    nBytes = msgQReceive (msgQId, buffer, maxNBytes, timeout);

    if (nBytes == ERROR)
        {
        switch (errnoGet())
            {
            case S_objLib_OBJ_UNAVAILABLE:    /* timeout == NO_WAIT */
                msgQStatus = MSG_Q_DIST_STATUS_UNAVAIL;
                break;
            case S_objLib_OBJ_TIMEOUT:        /* timeout != NO_WAIT */
                msgQStatus = MSG_Q_DIST_STATUS_TIMEOUT;
                break;
            default:
                msgQStatus = MSG_Q_DIST_STATUS_ERROR;
            }

        if (lastTry)
            {
            free(buffer);
            msgQDistSendStatus (nodeIdRespond, inqIdRespond, msgQStatus);
            }
        else
            {
                if (msgQStatus != MSG_Q_DIST_STATUS_UNAVAIL)
                    msgQDistSendStatus (nodeIdRespond,
                                        inqIdRespond,msgQStatus);
            }

        return (msgQStatus);
        }

    /* We have received a message, now respond to the request. */
    
    pktRpl.recvRplHdr.pktType = DIST_PKT_TYPE_MSG_Q;
    pktRpl.recvRplHdr.pktSubType = DIST_PKT_TYPE_MSG_Q_RECV_RPL;
    pktRpl.recvRplInqId = (uint32_t) inqIdRespond;

    distIOVec[0].pIOBuffer = &pktRpl;
    distIOVec[0].IOLen = DIST_PKT_HDR_SIZEOF (DIST_PKT_MSG_Q_RECV_RPL);
    distIOVec[1].pIOBuffer = buffer;
    distIOVec[1].IOLen = nBytes;

    status = distNetIOVSend (nodeIdRespond, &distIOVec[0], 2, WAIT_FOREVER,
            DIST_MSG_Q_RECV_PRIO);
    if (status == ERROR)
        {
        STATUS status;

        status = msgQSend (msgQId, buffer, nBytes, NO_WAIT, MSG_PRI_URGENT);
        if (status == ERROR)
            {
            /* XXX TODO this one can also fail; panic for now */

            distPanic ("msgQDistRecvReply: msgQSend failed\n");
            }

#ifdef MSG_Q_DIST_REPORT
        printf ("msgQDistRecvReply: remote node is unreachable\n");
#endif
        if (lastTry)
            free (buffer);
        return (MSG_Q_DIST_STATUS_UNREACH);
        }

#ifdef MSG_Q_DIST_REPORT
    printf ("msgQDistRecvReply: received a message; forwarded it to remote\n");
#endif

    if (lastTry)
        free (buffer);
    return (MSG_Q_DIST_STATUS_OK);    /* packet already acknowledged */
    }

/***************************************************************************
*
* msgQDistSendStatus - send status and errno (VxFusion option)
*
* This routine sends operation status and errno inform to a remote node.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: N/A
*
* NOMANUAL
*/

LOCAL STATUS msgQDistSendStatus
    (
    DIST_NODE_ID        nodeIdDest,   /* the node to send status to */
    DIST_INQ_ID         inqId,        /* inquiry ID of that node */
    DIST_MSG_Q_STATUS   msgQStatus    /* the status to send */
    )
    {
    DIST_PKT_MSG_Q_STATUS    pktStatus;
    STATUS                   status;

    pktStatus.statusHdr.pktType = DIST_PKT_TYPE_MSG_Q;
    pktStatus.statusHdr.pktSubType = DIST_PKT_TYPE_MSG_Q_STATUS;
    pktStatus.statusInqId = (uint32_t) inqId;
    pktStatus.statusErrno = htonl ((uint32_t) errnoGet());
    pktStatus.statusDStatus = htons ((uint16_t) msgQStatus);

    status = distNetSend (nodeIdDest, (DIST_PKT *) &pktStatus,
            DIST_PKT_HDR_SIZEOF (DIST_PKT_MSG_Q_STATUS), WAIT_FOREVER,
            DIST_MSG_Q_ERROR_PRIO);

    return (status);
    }

