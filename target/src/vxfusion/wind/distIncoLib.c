/* distIncoLib.c - incorporation protocol library (VxFusion option) */

/* Copyright 1999-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01e,23oct01,jws  fix compiler warnings (SPR 71117)
01d,24may99,drm  added vxfusion prefix to VxFusion related includes
01c,11sep98,drm  changed distStatLib.h to private/distLibP.h
01b,20jan98,ur   changed booting behaviour, do always send an INCO_UPNOW.
01a,10sep97,ur   written.
*/

/*
DESCRIPTION
The incorporation library handles the appearence of a new node.

AVAILABILITY
This module is distributed as a component of the unbundled distributed
message queues option, VxFusion.

*/

#include "vxWorks.h"
#if defined (DIST_INCO_REPORT) || defined (DIST_DIAGNOSTIC)
#include "stdio.h"
#endif
#include "stdlib.h"
#include "semLib.h"
#include "vxfusion/distIfLib.h"
#include "vxfusion/private/distLibP.h"
#include "vxfusion/private/distNodeLibP.h"
#include "vxfusion/private/msgQDistGrpLibP.h"
#include "vxfusion/private/distNameLibP.h"
#include "vxfusion/private/distTBufLibP.h"
#include "vxfusion/private/distIncoLibP.h"
#include "vxfusion/private/distNetLibP.h"
#include "vxfusion/private/distPktLibP.h"

/* locals */

LOCAL SEMAPHORE           distIncoWait4Done;
LOCAL BOOL                distIncoLibInstalled = FALSE;

/* local prototypes */

LOCAL DIST_STATUS    distIncoInput (DIST_NODE_ID nodeIdIn,
                                    DIST_TBUF_HDR *pReassembled);

/***************************************************************************
*
* distIncoLibInit - initialize this module (VxFusion option)
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

void distIncoLibInit (void)
    {
    }

/***************************************************************************
*
* distIncoInit - start incorporation service (VxFusion option)
*
* This routine starts the incorporation service.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: OK, if service is started.
* NOMANUAL
*/

STATUS distIncoInit (void)
    {
    STATUS    status;

    if (distIncoLibInstalled)
        return (OK);

    semBInit (&distIncoWait4Done, SEM_Q_FIFO, SEM_EMPTY);

    /* Add INCO service to table of services. */

    status = distNetServAdd (DIST_PKT_TYPE_INCO,
                             distIncoInput,
                             DIST_INCO_SERV_NAME,
                             DIST_INCO_SERV_NET_PRIO,
                             DIST_INCO_SERV_TASK_PRIO,
                             DIST_INCO_SERV_TASK_STACK_SZ);

    if (status == ERROR)
        return (ERROR);

    distIncoLibInstalled = TRUE;
    return (OK);
    }

/***************************************************************************
*
* distIncoStart - start the incorporation protocol (VxWorks option)
*
* This routine starts the incorporation protocol.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: OK, is successful.
* NOMANUAL
*/

STATUS distIncoStart
    (
    int    waitNTicks        /* ticks distNodeBootstrap() should wait */
    )
    {
    DIST_PKT_INCO    pktInco;
    DIST_NODE_ID     nodeId;
    STATUS           status, s1, s2, s3, s4;
    DIST_PKT_INCO    pktUpNow;

    s1 = distNetServUp (DIST_PKT_TYPE_INCO);    /* incorporation service   */
    s2 = distNetServUp (DIST_PKT_TYPE_DGDB);    /* group database service  */
    s3 = distNetServUp (DIST_PKT_TYPE_GAP);     /* group agreement service */
    s4 = distNetServUp (DIST_PKT_TYPE_DNDB);    /* name database service   */

    if (s1 == ERROR || s2 == ERROR || s3 == ERROR || s4 == ERROR)
        return (ERROR);

#ifdef DIST_DIAGNOSTIC
    distLog ("distIncoStart: bootstrapping\n");
#endif

    distNodeBootstrap (waitNTicks);

#ifdef DIST_DIAGNOSTIC
    distLog ("distIncoStart: bootstrapping done--ok\n");
#endif

    if (distNodeGetGodfatherId (&nodeId) == ERROR)
        {
        /*
         * Since we found no godfather, we have to assume,
         * that we got up first. There is no need to try
         * to send an INCO_REQ.
         * But there are some rare situations, where some
         * other nodes that are also booting, do know about
         * us, unless we don't. They have registrated us in
         * BOOTING state. Therefore we send an INCO_UPNOW
         * broadcast, to force them to shift our state to
         * OPERATIONAL.
         */

        s1 = distNetServUp (DIST_PKT_TYPE_MSG_Q);     /* message queue serv */
        s2 = distNetServUp (DIST_PKT_TYPE_MSG_Q_GRP); /* message group serv */

        if (s1 == ERROR || s2 == ERROR)
            distPanic ("distIncoInput/DONE: Cannot activate service.\n");

        distNodeLocalSetState (DIST_NODE_STATE_OPERATIONAL);

        pktUpNow.pktIncoHdr.pktType = DIST_PKT_TYPE_INCO;
        pktUpNow.pktIncoHdr.pktSubType = DIST_PKT_TYPE_INCO_UPNOW;

        distNetSend (DIST_IF_BROADCAST_ADDR,
                     (DIST_PKT *) &pktUpNow, sizeof (pktUpNow),
                      WAIT_FOREVER, DIST_INCO_PRIO);

#ifdef DIST_DIAGNOSTIC
        distLog ("distIncoStart: incorporation done--we are 1st\n");
#endif
        return (OK);
        }

    pktInco.pktIncoHdr.pktType = DIST_PKT_TYPE_INCO;
    pktInco.pktIncoHdr.pktSubType = DIST_PKT_TYPE_INCO_REQ;

#ifdef DIST_DIAGNOSTIC
    distLog ("distIncoStart: sending incorporation request to node 0x%lx\n",
            nodeId);
#endif

    status = distNetSend (nodeId, (DIST_PKT *) &pktInco, sizeof (pktInco),
                          WAIT_FOREVER, DIST_INCO_PRIO);
    if (status == ERROR)
        return (ERROR);

#ifdef DIST_DIAGNOSTIC
    distLog ("distIncoStart: incorporation request sent--ok\n");
#endif

    semTake (&distIncoWait4Done, WAIT_FOREVER);

#ifdef DIST_DIAGNOSTIC
    distLog ("distIncoStart: we are incorporated--ok\n");
#endif

    return (OK);
    }

/***************************************************************************
*
* distIncoInput - handle incorporation input (VxFusion option)
*
* This routine handles incorporation input.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: Status of input processing.
* NOMANUAL
*/

LOCAL DIST_STATUS distIncoInput
    (
    DIST_NODE_ID    nodeIdSrc,    /* source node's Id */
    DIST_TBUF_HDR * pTBufHdr      /* the message to process */
    )
    {
    DIST_PKT_INCO       pktInco;
    DIST_PKT_INCO       pktDone;
    int                 pktLen = pTBufHdr->tBufHdrOverall;
    DIST_PKT_INCO       pktUpNow;
    STATUS              s1, s2, s3;
    DIST_NODE_DB_NODE * pNode;

#ifdef DIST_DIAGNOSTIC
    STATUS    status;
#endif

    if (pktLen != sizeof (DIST_PKT_INCO))
        distPanic ("distIncoInput: packet too short\n");

    distTBufCopy (DIST_TBUF_GET_NEXT (pTBufHdr), 0, (char *)&pktInco, pktLen);

    switch (pktInco.pktIncoHdr.pktSubType)
        {
        case DIST_PKT_TYPE_INCO_REQ:
            {
            /*
             * A new node introduces itself. Send name database
             * and group information.
             */

#ifdef DIST_DIAGNOSTIC
            distLog ("distIncoInput/REQ: node 0x%lx wants to get incorporated\n",
                     nodeIdSrc);
#endif
#ifdef DIST_INCO_REPORT
            printf ("distIncoInput: we are in state %d\n",
                    distNodeLocalGetState());
#endif
            if (msgQDistGrpBurst (nodeIdSrc) == ERROR)
                {
#ifdef DIST_DIAGNOSTIC
                distLog ("distIncoInput/REQ: group db update failed\n");
#endif
                break;
                }

            if (distNameBurst (nodeIdSrc) == ERROR)
                {
#ifdef DIST_DIAGNOSTIC
                distLog ("distIncoInput/REQ: name db update failed\n");
#endif
                break;
                }

            distNodeOperational (nodeIdSrc);

            /* Signal DONE to godchild. */

            pktDone.pktIncoHdr.pktType = DIST_PKT_TYPE_INCO;
            pktDone.pktIncoHdr.pktSubType = DIST_PKT_TYPE_INCO_DONE;

#ifdef DIST_DIAGNOSTIC
            status = distNetSend (nodeIdSrc, (DIST_PKT *) &pktDone,
                                  sizeof (pktDone), WAIT_FOREVER,
                                  DIST_INCO_PRIO);

            if (status == ERROR)
                distLog ("distIncoInput/REQ: failed sending DONE\n");
            else
                distLog ("distIncoInput/REQ: node 0x%lx is updated, DONE sent\n",
                         nodeIdSrc);
#else
            distNetSend (nodeIdSrc, (DIST_PKT *) &pktDone,
                         sizeof (pktDone), WAIT_FOREVER, DIST_INCO_PRIO);
#endif
            break;
            }

        case DIST_PKT_TYPE_INCO_DONE:
            {

#ifdef DIST_INCO_REPORT
            printf ("distIncoInput/DONE: we are incorporated\n");
#endif
            distNodeLocalSetState (DIST_NODE_STATE_OPERATIONAL);

            s1 = distNetServUp (DIST_PKT_TYPE_MSG_Q);
            s2 = distNetServUp (DIST_PKT_TYPE_GAP);
            s3 = distNetServUp (DIST_PKT_TYPE_MSG_Q_GRP);

            if (s1 == ERROR || s2 == ERROR || s3 == ERROR)
                distPanic ("distIncoInput/DONE: Cannot activate service.\n");

            pktUpNow.pktIncoHdr.pktType = DIST_PKT_TYPE_INCO;
            pktUpNow.pktIncoHdr.pktSubType = DIST_PKT_TYPE_INCO_UPNOW;

#ifdef DIST_DIAGNOSTIC
            status = distNetSend (DIST_IF_BROADCAST_ADDR,
                                  (DIST_PKT *) &pktUpNow, sizeof (pktUpNow),
                                  WAIT_FOREVER, DIST_INCO_PRIO);
            if (status == ERROR)
                distLog ("distIncoInput/DONE: failed sending UPNOW\n");
#else
            distNetSend (DIST_IF_BROADCAST_ADDR,
                         (DIST_PKT *) &pktUpNow, sizeof (pktUpNow),
                         WAIT_FOREVER, DIST_INCO_PRIO);
#endif
            semGive (&distIncoWait4Done);

            break;
            }

        case DIST_PKT_TYPE_INCO_UPNOW:
            {

#ifdef DIST_INCO_REPORT
            printf ("distIncoInput/UPNOW: node 0x%lx is up now\n",
                    nodeIdSrc);
#endif
            pNode = distNodeOperational (nodeIdSrc);
            if (pNode == NULL)
                {
                /*
                 * This should never happen. But if it happens,
                 * try to create that node.
                 */

#ifdef DIST_DIAGNOSTIC
                distLog ("distIncoInput/UPNOW: UPNOW from unknown node 0x%lx\n",
                        nodeIdSrc);
                pNode =
                    distNodeCreate (nodeIdSrc, DIST_NODE_STATE_OPERATIONAL);

                if (pNode == NULL)
                    distLog ("distIncoInput/UPNOW: creation of node failed\n");
#else
                distNodeCreate (nodeIdSrc, DIST_NODE_STATE_OPERATIONAL);
#endif
                }
            break;
            }

        default:
#ifdef DIST_DIAGNOSTIC
            distLog ("distIncoInput: unknown subtype (%d) of INCO protocol\n",
                    pktInco.pktIncoHdr.pktSubType);
#endif
            return (DIST_INCO_STATUS_PROTOCOL_ERROR);
        }

    return (DIST_INCO_STATUS_OK);
    }

/***************************************************************************
*
* distIncoIntro - introduce this node to the system (VxFusion option)
*
* This routine introduces this node to the system.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: OK
* NOMANUAL
*/

STATUS distIncoIntro (void)
    {
    return (OK);
    }

