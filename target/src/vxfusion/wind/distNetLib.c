/* distNetLib - distributed objects network layer (VxFusion option) */

/* Copyright 1999 - 2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01k,18apr02,jws  fix SPR74878 (not lock node database properly)
01j,23oct01,jws  fix compiler warnings (SPR 71117)
01i,04oct01,jws  final fixes for SPR 34770
01h,27sep01,p_r  Fixes for SPR#34770
01g,24may99,drm  added vxfusion prefix to VxFusion related includes
01f,23feb99,drm  library documentation update
01e,22feb99,drm  added documentation for library and distNetInput() function
01d,12aug98,drm  added #include stmt for distObjTypeP.h
01c,08aug98,drm  added code to set broadcast flag when sending broadcast msgs
01b,20may98,drm  removed some warning messages by initializing pointers to NULL
01a,05sep97,ur   written.
*/

/*
DESCRIPTION
This library contains the distributed objects network layer.  The network layer
puts messages into telegram buffers and reassembles them from telegram 
buffers.  When a message exceeds the space allocated for message data in a 
telegram, the network layer fragments the message into multiple telegram 
buffers, and reassembles a single message from multiple telegram buffers.  

After placing a message into a telegram buffer(s), this layer calls the
adapter's send routine and sends a telegram out over the
transport.  If the message has to be fragmented into more than one telegram 
buffer, the adapter send routine is called once for each buffer used.

Upon receiving a telegram from a remote node, an adapter must call this
library's distNetInput() routine to forward a telegram buffer to VxFusion.

AVAILABILITY
This module is distributed as a component of the unbundled distributed
message queues option, VxFusion.

INCLUDE FILES: distNetLib.h

SEE ALSO: distIfLib
*/

#include "vxWorks.h"
#if defined (DIST_NET_REPORT) || defined (DIST_DIAGNOSTIC)
#include "stdio.h"
#endif
#include "errnoLib.h"
#include "string.h"
#include "taskLib.h"
#include "semLib.h"
#include "netinet/in.h"
#include "private/semLibP.h"
#include "private/distObjTypeP.h"
#include "vxfusion/distLib.h"
#include "vxfusion/distIfLib.h"
#include "vxfusion/distStatLib.h"
#include "vxfusion/private/distNetLibP.h"
#include "vxfusion/private/distNodeLibP.h"
#include "vxfusion/private/distTBufLibP.h"
#include "vxfusion/private/distPktLibP.h"

/* defines */

#ifndef MAX
#define MAX(a, b) (((a) < (b)) ? (b) : (a))
#endif

#ifndef MIN
#define MIN(a, b) (((a) > (b)) ? (b) : (a))
#endif

DIST_SERV_NODE    servTable[DIST_SERV_MAX];

/* locals */

LOCAL BOOL       distNetLibInstalled = FALSE;
LOCAL FUNCPTR    distNetServiceHook = NULL;

/* local prototypes */

LOCAL void distNetServTask (DIST_SERV_FUNC servInput,
                            DIST_SERV_NODE *servNode);

LOCAL STATUS distNetServCall (int servId, DIST_TBUF_HDR *pDeliver);

/***************************************************************************
*
* distNetLibInit - initialize this module (VxFusion option)
*
* This routine currently does nothing.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: N/A
* NOMANUAL
*/

void distNetLibInit (void)
    {
    }

/***************************************************************************
*
* distNetInit - initialize server table (VxFusion option)
*
* This routine initializes the server table.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: N/A
* NOMANUAL
*/

void distNetInit (void)
    {
    int i;

    bzero ((char *) &servTable[0], sizeof(servTable));
    for (i = 0; i < DIST_SERV_MAX; i++)
        {
        servTable[i].servTaskPrio = servTable[i].servNetPrio = (-1);
        }
    distNetLibInstalled = TRUE;
    }

/***************************************************************************
*
* distNetServAdd - add a service (VxFusion option)
*
* This routine adds a service to the server table and spawns a task to
* implement the service.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: OK, if successful.
* NOMANUAL
*/

STATUS distNetServAdd
    (
    int            servId,          /* id of service to add */
    DIST_SERV_FUNC servInput,       /* service input function to call */
    char *         servTaskName,    /* name of service task */
    int            servNetPrio,     /* priority of service on the network */
    int            servTaskPrio,    /* priority of service task */
    int            servTaskStackSz  /* stack size for service task */
    )
    {
    DIST_SERV_NODE    *pServNode = (DIST_SERV_NODE *) &servTable[servId];
    int        taskPrio;
    int        tid;

    if (!distNetLibInstalled || servId < 0 || servId > DIST_SERV_MAX - 1)
        return (ERROR);

    semBInit (&pServNode->servQLock, SEM_Q_FIFO, SEM_FULL);
    semBInit (&pServNode->servWait4Jobs, SEM_Q_FIFO, SEM_EMPTY);
    pServNode->servId = servId;
    pServNode->servUp = FALSE;
    pServNode->pServQ = NULL;

    if (pServNode->servTaskPrio == -1)
        pServNode->servTaskPrio = servTaskPrio;

    if (pServNode->servNetPrio == -1)
        pServNode->servNetPrio = servNetPrio;

    taskPrio = pServNode->servTaskPrio;

    tid = taskSpawn (servTaskName,
                     taskPrio,
                     VX_SUPERVISOR_MODE | VX_UNBREAKABLE,
                     servTaskStackSz,
                     (FUNCPTR) distNetServTask,
                     (int) servInput,
                     (int) pServNode,
                     0, 0, 0, 0, 0, 0, 0, 0);
    if (tid == ERROR)
        return (ERROR);

    pServNode->servTaskId = tid;

    return (OK);
    }

/***************************************************************************
*
* distNetServTask - service task (VxFusion option)
*
* This is the common entry point for all network service tasks. It
* calls the individual service functions.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: N/A
* NOMANUAL
*/

LOCAL void distNetServTask
    (
    DIST_SERV_FUNC   servInput,     /* function to process input */
    DIST_SERV_NODE * pServNode      /* server node */
    )
    {
    DIST_TBUF_HDR * pDeliver;
    DIST_NODE_ID    nodeSrcId;
    DIST_STATUS     dStatus;

    FOREVER
        {
        semTake (&pServNode->servWait4Jobs, WAIT_FOREVER);

        while (pServNode->pServQ)
            {
            semTake (&pServNode->servQLock, WAIT_FOREVER);
            DIST_TBUF_HDR_DEQUEUE (pServNode->pServQ, pDeliver);
            semGive (&pServNode->servQLock);
            nodeSrcId = pDeliver->tBufHdrSrc;

            if ((dStatus = servInput (nodeSrcId, pDeliver)) != OK)
                {
#ifdef DIST_NET_REPORT
                printf ("distNetServTask/#%d: service task returned %d\n",
                        pServNode->servId, dStatus);
#endif  
                if (distNetServiceHook)
                    (* distNetServiceHook) (pServNode->servId, dStatus);
                }
                
            /* locking probably not required, but doesn't cost much */

            distNodeDbLock ();
            DIST_TBUF_FREEM (pDeliver);
            distNodeDbUnlock ();
            }
        }
    }

/***************************************************************************
*
* distNetServConfig - configure service (VxFusion option)
*
* This routine is used to change a service's task and network priority.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: OK, if successful.
* NOMANUAL
*/

STATUS distNetServConfig
    (
    int        servId,      /* ID of service to configure */
    int        taskPrio,    /* new task priority */
    int        netPrio      /* new network priority */
    )
    {
    DIST_SERV_NODE * pServNode = (DIST_SERV_NODE *) &servTable[servId];

    if (servId >= 0 && servId < DIST_SERV_MAX)
        {
        if (netPrio != -1)
            {
            if (netPrio < 0 || netPrio > 7)
                return (ERROR);

            pServNode->servNetPrio = netPrio;
            }

        if (taskPrio != -1)
            {
            if (taskPrio < 0 || taskPrio > 255)
                return (ERROR);

            pServNode->servTaskPrio = taskPrio;

            if (DIST_NET_SERV_INST (servId))
                return (taskPrioritySet (pServNode->servTaskId, taskPrio));
            }

        return (OK);
        }

    return (ERROR);
    }

/***************************************************************************
*
* distNetServUp - change state of service to up (VxFusion option)
*
* This routine marks to state of a service as UP.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: ERROR, if no such service exists.
* NOMANUAL
*/

STATUS distNetServUp
    (
    int servId      /* ID of service */
    )
    {
    DIST_SERV_NODE * pServNode;

    if (servId >= 0  &&  servId < DIST_SERV_MAX
                     &&  DIST_NET_SERV_INST (servId))
        {
        pServNode = (DIST_SERV_NODE *) &servTable[servId];
        pServNode->servUp = TRUE;
        return (OK);
        }

    return (ERROR);
    }

/***************************************************************************
*
* distNetServDown - change state of service to down (VxFusion option)
*
* This routine changes the state of a service to DOWN.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: ERROR, if no such service exists.
* NOMANUAL
*/

STATUS distNetServDown
    (
    int servId    /* ID of service */
    )
    {
    DIST_SERV_NODE * pServNode;

    if (servId >= 0 && servId < DIST_SERV_MAX
                    && DIST_NET_SERV_INST (servId))
    {
    pServNode = (DIST_SERV_NODE *) &servTable[servId];
    pServNode->servUp = FALSE;
    return (OK);
    }

    return (ERROR);
    }

/***************************************************************************
*
* distNetServCall - call a service with an incoming packet (VxFusion option)
*
* This routine passes incoming data to a service.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: OK, if successful.
* NOMANUAL
*/

LOCAL STATUS distNetServCall
    (
    int             servId,      /* ID of service */
    DIST_TBUF_HDR * pDeliver     /* the message */
    )
    {
    DIST_SERV_NODE * pServNode;

    if (servId >= 0 && servId < DIST_SERV_MAX
                    && DIST_NET_SERV_INST (servId))
        {
        pServNode = (DIST_SERV_NODE *) &servTable[servId];

        /*
         * Although the service is not up yet, we return OK and
         * force an acknowledge for the packet. We have to keep
         * the window rotating.
         */
        if (pServNode->servUp == FALSE)
            return (OK);

        semTake (&pServNode->servQLock, WAIT_FOREVER);
        DIST_TBUF_HDR_ENQUEUE (pServNode->pServQ, pDeliver);
        semGive (&pServNode->servQLock);
        semGive (&pServNode->servWait4Jobs);
        return (OK);
        }

    return (ERROR);
    }

/***************************************************************************
*
* distNetCtl - control function for network layer (VxFusion option)
*
* This routine performs control functions on the network layer.
* The following functions are accepted:
* \is
* \i DIST_CTL_SERVICE_HOOK
* Set a function to be called, each time a service called by a
* remote node fails.
* \i DIST_CTL_SERVICE_CONF
* Unsed to configure a certain service. The <argument> parameter is
* a pointer to DIST_SERV_CONF, which holds the service id and its
* configuration to set.
* \ie
*
* RETURNS: Return value of function called, or ERROR.
*
* ERRNO:
* S_distLib_UNKNOWN_REQUEST
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* NOMANUAL
*/

int distNetCtl
    (
    int    function,        /* function code      */
    int    argument         /* arbitrary argument */
    )
    {
    DIST_SERV_CONF    *pServConfig;


    switch (function)
        {
        case DIST_CTL_SERVICE_HOOK:
            distNetServiceHook = (FUNCPTR) argument;
            return (OK);
        case DIST_CTL_SERVICE_CONF:
            {

            if ((pServConfig = (DIST_SERV_CONF *) argument) == NULL)
                return (ERROR);
        
            return (distNetServConfig (pServConfig->servId,
                    pServConfig->taskPrio, pServConfig->netPrio));
            }
        default:
            errnoSet (S_distLib_UNKNOWN_REQUEST);
            return (ERROR);
        }
    }

/***************************************************************************
*
* distNetSend - send message from a continuous buffer to the network (VxFusion option)
*
* This routine sends a message of length <nBytes> starting at <buffer>
* to node <distNodeDest>. If necessary, the message is fragmented.
*
* The routine blocks until the message is acknowledged or timeout
* has expired.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: OK, if successful.
* NOMANUAL
*/

STATUS distNetSend
    (
    DIST_NODE_ID nodeIdDest,  /* node to which message is destinated */
    DIST_PKT *   pPkt,        /* packet to send                      */
    UINT         nBytes,      /* length of message                   */
    int          timo,        /* ticks to wait                       */
    int          prio         /* priority level to send message on   */
    )
    {
    char *          pFrag;
    char *          pBufEnd;
    DIST_TBUF *     pTBuf;
    DIST_TBUF_HDR * pTBufHdr;
    int             mtu;
    int             tBufNBytes;
    short           seq = 0;

    distStat.netOutReceived++;

    if (nodeIdDest == DIST_IF_BROADCAST_ADDR &&
            distNodeGetNumNodes (DIST_NODE_NUM_NODES_ALIVE) <= 1)
        return (OK);

    pPkt->pktLen = htons ((UINT16)nBytes);

    /* Get first TBuf for header. */

    if ((pTBufHdr = DIST_TBUF_HDR_ALLOC ()) == NULL)
        {
        distStat.netOutDiscarded++;
        return (ERROR);                /* out of TBufs */
        }

    pTBufHdr->tBufHdrDest = nodeIdDest;
    pTBufHdr->tBufHdrOverall = (INT16) nBytes;
    pTBufHdr->tBufHdrTimo = timo;
    pTBufHdr->tBufHdrPrio = prio;

    /* Split the packet to n telegrams and copy them to TBufs. */

    pBufEnd = (pFrag = (char *) pPkt) + nBytes;
    mtu = DIST_IF_MTU - DIST_IF_HDR_SZ;

    while ((pTBuf = DIST_TBUF_ALLOC ()) != NULL)
        {
        DIST_TBUF_ENQUEUE (pTBufHdr, pTBuf);

        tBufNBytes = MIN (pBufEnd - pFrag, mtu);

        pTBuf->tBufFlags = DIST_TBUF_FLAG_MF;
        if (seq == 0)
            pTBuf->tBufFlags |= DIST_TBUF_FLAG_HDR;

        if (nodeIdDest == DIST_IF_BROADCAST_ADDR)
            {
            pTBuf->tBufType = DIST_TBUF_TTYPE_BDTA;
            pTBuf->tBufFlags |= DIST_TBUF_FLAG_BROADCAST;
            }
        else
            pTBuf->tBufType = DIST_TBUF_TTYPE_DTA;

        pTBuf->tBufSeq = seq++;
        pTBuf->tBufNBytes = (UINT16) tBufNBytes;

        bcopy (pFrag, pTBuf->pTBufData, tBufNBytes);

        pFrag += tBufNBytes;

        if (pFrag == pBufEnd)    /* done? */
            {
            pTBuf->tBufFlags &= ~DIST_TBUF_FLAG_MF;
#ifdef DIST_NET_REPORT
            printf ("distNetSend:%p: node %lu, numSeq %d, lenOverall %d\n",
                    pTBufHdr, nodeIdDest, seq, pTBufHdr->tBufHdrOverall);
#endif
            return (distNodePktSend (pTBufHdr));
            }

        }

    /* TBuf shortage */

    DIST_TBUF_FREEM (pTBufHdr);
    distStat.netOutDiscarded++;
    return (ERROR);    
    }

/***************************************************************************
*
* distNetIOVSend - send message from discontinuous buffer to the network (VxFusion option)
*
* This routine sends a message from a scatter/gather buffer (IOV)
* to node <nodeIdDest>. If necessary, the message is fragmented.
*
* This routine blocks until the message is acknowledged or timeout
* has exceeded.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: OK, if successful.
* NOMANUAL
*/

STATUS distNetIOVSend
    (
    DIST_NODE_ID  nodeIdDest, /* node to which message is destinated */
    DIST_IOVEC *  pIOV,       /* list of buffers to gather           */
    int           numIOV,     /* number of buffers                   */
    int           timo,       /* ticks to wait                       */
    int           prio        /* priority level to send message on   */
    )
    {
    DIST_IOVEC *    p;
    DIST_TBUF_HDR * pTBufHdr;
    DIST_TBUF *     pTBuf = NULL;
    DIST_PKT *      pPkt;
    int             szSrc, szDest, sz;
    int             seq;
    char *          pSrc;
    char *          pDest = NULL;

    distStat.netOutReceived++;

    if (numIOV <= 0)
        return (ERROR);

    if (nodeIdDest == DIST_IF_BROADCAST_ADDR
              &&
        distNodeGetNumNodes (DIST_NODE_NUM_NODES_ALIVE) <= 1)
        {
        return (OK);
        }

    /* Get first TBuf for header. */

    if ((pTBufHdr = DIST_TBUF_HDR_ALLOC ()) == NULL)
        {
        distStat.netOutDiscarded++;
        return (ERROR);
        }

    pTBufHdr->tBufHdrDest = nodeIdDest;
    pTBufHdr->tBufHdrTimo = timo;
    pTBufHdr->tBufHdrPrio = prio;
    pTBufHdr->tBufHdrOverall = 0;    /* later */

    /* Copy the discontinious buffer to a list of tBufs. */

    p = pIOV;
    szDest = 0;
    seq = 0;
    while (numIOV--)
        {
        pTBufHdr->tBufHdrOverall += (szSrc = p->IOLen);
        pSrc = p->pIOBuffer;
        do
            {
            if (szDest == 0)
                {
                if ((pTBuf = DIST_TBUF_ALLOC ()) == NULL)
                    {
                    distStat.netOutDiscarded++;
                    DIST_TBUF_FREEM (pTBufHdr);
                    return (ERROR);
                    }

                pTBuf->tBufFlags = 0;
                if (seq == 0)
                    pTBuf->tBufFlags |= DIST_TBUF_FLAG_HDR;
                pTBuf->tBufFlags |= DIST_TBUF_FLAG_MF;

                if (nodeIdDest == DIST_IF_BROADCAST_ADDR)
                    {
                    pTBuf->tBufType = DIST_TBUF_TTYPE_BDTA;
                    pTBuf->tBufFlags |= DIST_TBUF_FLAG_BROADCAST;
                    }
                else
                    pTBuf->tBufType = DIST_TBUF_TTYPE_DTA;

                pTBuf->tBufSeq = (UINT16) seq++;
                pTBuf->tBufNBytes = 0;
                DIST_TBUF_ENQUEUE (pTBufHdr, pTBuf);
                szDest = DIST_IF_MTU - DIST_IF_HDR_SZ;
                pDest = pTBuf->pTBufData;
                }
            sz = MIN (szSrc, szDest);
            bcopy (pSrc, pDest, sz);
            pTBuf->tBufNBytes += sz;
            szSrc -= sz;
            szDest -= sz;
            pSrc += sz;
            pDest += sz;
            }
        while (szSrc > 0);
        p++;
        }

    pTBuf->tBufFlags &= ~DIST_TBUF_FLAG_MF;

#ifdef DIST_NET_REPORT
    printf ("distNetSend:%p: node %lu, numSeq %d, lenOverall %d\n",
            pTBufHdr, nodeIdDest, seq, pTBufHdr->tBufHdrOverall);
#endif
    pPkt = (DIST_PKT *) ((DIST_TBUF_GET_NEXT (pTBufHdr))->pTBufData);
    pPkt->pktLen = htons (pTBufHdr->tBufHdrOverall);

    return (distNodePktSend (pTBufHdr));
    }

/***************************************************************************
*
* distNetInput - accept a telegram buffer and send it to upper VxFusion layers (VxFusion option)
*
* This routine accepts a telegram buffer from an adapter and tries to 
* reassemble a message from it.  If the telegram buffer contains an entire 
* message, distNetInput() simply forwards the message to the upper VxFusion 
* layers and sends an acknowledgment for the message.  If the telegram buffer 
* contains a message fragment, the message fragment is added to existing 
* message fragments, if any.  If the message fragment completes a message, the 
* message is forwarded to the upper VxFusion layers and distNetInput() 
* sends an acknowledgment for the message.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: N/A
* NOMANUAL
*/

void distNetInput
    (
    DIST_NODE_ID nodeIdIn, /* source node of the telegram buffer */
    int          prioIn,   /* priority the telegram arrived with */
    DIST_TBUF *  pTBufIn   /* the telegram buffer itself         */
    )
    {
    void * pComm; /* from distNodeReassemble(), to distNodeGetReassembled() */

    DIST_TBUF_HDR *  pReassembled;
    DIST_PKT *       pPkt;

    distStat.netInReceived++;

    /* Try to reassemble. */
    
    if ((pComm = distNodeReassemble (nodeIdIn, prioIn, pTBufIn)) == NULL)
        return;

    /*
     * Now that we are here, a packet has been reassembled.
     * <pComm> points to the communication, that has one or
     * more packets.
     */

    while ((pReassembled = distNodeGetReassembled (pComm)) != NULL)
        {
#ifdef DIST_NET_REPORT
            {
            char * buffer;
            int    len;

            printf ("distNetInput: packet %p reassembled\n", pReassembled);
            buffer = malloc ((len = pReassembled->tBufHdrOverall));
            if (buffer)
                {
                distTBufCopy (DIST_TBUF_GET_NEXT (pReassembled), 0,
                        buffer, len);
                distDump (buffer, len);
                free (buffer);
                }
            }
#endif
        distStat.netReassembled++;

        pPkt = (DIST_PKT *) ((DIST_TBUF_GET_NEXT (pReassembled))->pTBufData);

        if ((ntohs (pPkt->pktLen) != pReassembled->tBufHdrOverall) ||
                (distNetServCall (pPkt->pktType, pReassembled) == ERROR))
            {
#ifdef DIST_DIAGNOSTIC
            if (ntohs (pPkt->pktLen) != pReassembled->tBufHdrOverall)
                {
                distLog("distNetInput: wrong length (has %d, expected %d)\n",
                        pReassembled->tBufHdrOverall, ntohs (pPkt->pktLen));
                }
            else
                distLog("distNetInput: unknown type (%d) or service down\n",
                        pPkt->pktType);
#endif
            distNodePktDiscard (pReassembled->tBufHdrSrc, pReassembled);
            }
        else
            distNodePktAck (pReassembled->tBufHdrSrc, pReassembled, 0);

        /* loop back for the next packet. */

        }
    }

