/* msgQDistGrpLib.c - distributed message queue group library (VxFusion option) */

/* Copyright 1999-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01q,23oct01,jws  fix compiler warnings (SPR 71117); fix man pages (SPR 71239)
01p,24jul01,r_s  changed code to be ANSI compatible so that it compiles with
                 diab. made asm macro changes for diab
01o,09jun99,drm  Allowing check for empty groups.
01n,09jun99,drm  Adding some htonl() / ntohl()
01m,09jun99,drm  Adding code to return S_msgQDistLib_OVERALL_TIMEOUT
01l,01jun99,drm  Adding check to make sure that only distributed message
                 queues (and not groups) can be added to a group.
01k,24may99,drm  added vxfusion prefix to VxFusion related includes
01j,23feb99,wlf  doc edits
01i,18feb99,wlf  doc cleanup
01h,29oct98,drm  documentation modifications
01g,09oct98,drm  fixed a bug reported by Oce
01f,13may98,ur   cleanup when msgQDistGrpInit() fails
                 locking on group send inquiries
01e,08may98,ur   removed 8 bit node id restriction
01d,08apr98,ur   optional enhancement CHECK_FOR_EMPTY_GROUP--untested
01c,20mar98,ur   set errno when database is full
01b,01jul97,ur   tested, ok.
01a,11jun97,ur   written.
*/

/*
DESCRIPTION
This library provides the grouping facility for distributed message queues.  
Single distributed message queues can join one or more groups.  A message
sent to a group is sent to all message queues that are members of that
group.  A group, however, is prohibited from sending messages.  Also, it is
an error to call msgQDistNumMsgs() with a distributed message queue group ID.

Groups are created with symbolic names and identified by a unique ID,
 MSG_Q_ID, as with normal message queues.

If the group is new to the distributed system, the group agreement 
protocol (GAP) is employed to determine a globally unique identifier.  
As part of the protocol's negotiation, all group databases throughout 
the system are updated.

The distributed message queue group library is initialized by calling
distInit().

AVAILABILITY
This module is distributed as a component of the unbundled distributed
message queues option, VxFusion.

INCLUDE FILES: msgQDistGrpLib.h

SEE ALSO: distLib, msgQDistGrpShow
*/

#include "vxWorks.h"
#if defined (MSG_Q_DIST_GRP_REPORT) || defined (DIST_DIAGNOSTIC)
#include "stdio.h"
#endif
#include "string.h"
#include "stdlib.h"
#include "taskLib.h"
#include "hashLib.h"
#include "sllLib.h"
#include "errnoLib.h"
#include "msgQLib.h"
#include "netinet/in.h"
#include "vxfusion/msgQDistLib.h"
#include "vxfusion/msgQDistGrpLib.h"
#include "vxfusion/distIfLib.h"
#include "vxfusion/distStatLib.h"
#include "vxfusion/private/msgQDistGrpLibP.h"
#include "vxfusion/private/distNetLibP.h"
#include "vxfusion/private/distNodeLibP.h"
#include "vxfusion/private/distObjLibP.h"
#include "vxfusion/private/distLibP.h"

/* defines */

#define UNUSED_ARG(x)  if(sizeof(x)) {} /* to suppress compiler warnings */

#define KEY_ARG_STR        13
#define KEY_CMP_ARG_STR    0       /* not used */

#define KEY_ARG_ID        65537
#define KEY_CMP_ARG_ID    0        /* not used */

/* global data */

SEMAPHORE                    distGrpDbSemaphore;
DIST_MSG_Q_GRP_ID            distGrpIdNext = 0;

/* local data */

LOCAL HASH_ID                distGrpDbNmId = NULL;
LOCAL DIST_GRP_HASH_NODE *   distGrpDbNm = NULL;

LOCAL HASH_ID                distGrpDbIdId = NULL;
LOCAL DIST_GRP_HASH_NODE *   distGrpDbId = NULL;

LOCAL DIST_GRP_DB_NODE *     distGrpDb = NULL;
LOCAL SL_LIST                msgQDistGrpFreeList;

LOCAL BOOL                   msgQDistGrpLibInstalled = FALSE;

/* local prototypes */

LOCAL BOOL          msgQDistGrpHCmpStr (DIST_GRP_HASH_NODE *pMatchNode,
                                        DIST_GRP_HASH_NODE *pHNode,
                                        int keyArg);
LOCAL INT32         msgQDistGrpHFuncStr (int elements,
                                         DIST_GRP_HASH_NODE *pHNode,
                                         int keyArg);
LOCAL BOOL          msgQDistGrpHCmpId (DIST_GRP_HASH_NODE *pMatchNode,
                                       DIST_GRP_HASH_NODE *pHNode,
                                       int keyArg);
LOCAL INT32         msgQDistGrpHFuncId (int elements,
                                        DIST_GRP_HASH_NODE *pHNode,
                                        int keyArg);
LOCAL DIST_STATUS    msgQDistGdbInput (DIST_NODE_ID nodeIdSrc,
                                       DIST_TBUF_HDR *pTBufHdr);
LOCAL DIST_STATUS    msgQDistGrpInput (DIST_NODE_ID nodeIdSrc,
                                       DIST_TBUF_HDR *pTBufHdr);
LOCAL STATUS         msgQDistGrpLclSend (DIST_MSG_Q_GRP_SEND_INQ *pInq,
                                         DIST_MSG_Q_GRP_ID distMsgQGrpId,
                                         char *buffer,
                                         UINT nBytes, int timeout,
                                         int priority);
LOCAL void           msgQDistGrpLclSendCanWait (DIST_INQ_ID inqId,
                                                MSG_Q_ID msgQId,
                                                char *buffer, UINT nBytes,
                                                int timeout, int priority);
LOCAL STATUS         msgQDistGrpAgent (DIST_NODE_ID nodeIdSender,
                                       DIST_INQ_ID inqIdSender,
                                       DIST_MSG_Q_GRP_ID distMsgQGrpId,
                                       char *buffer,
                                       UINT nBytes, int timeout,
                                       int priority);
LOCAL STATUS         msgQDistGrpSendStatus (DIST_NODE_ID nodeIdDest,
                                            DIST_INQ_ID inqId,
                                            DIST_MSG_Q_STATUS msgQStatus);
LOCAL BOOL           msgQDistGrpBurstOne (DIST_GRP_HASH_NODE *pNode,
                                          DIST_GRP_BURST *pBurst);

/***************************************************************************
*
* msgQDistGrpLibInit - initialize the distributed message queue group package (VxFusion option)
*
* This routine currently does nothing.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void msgQDistGrpLibInit (void)
    {
    }

/***************************************************************************
*
* msgQDistGrpInit - initialize the group database (VxFusion option)
*
* This routine initializes the group database.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: OK, if successful; ERROR, if unsucessful.
*
* NOMANUAL
*/

STATUS msgQDistGrpInit
    (
    int               sizeLog2   /* init 2^^sizeLog2 entries */
    )
    {
    DIST_GRP_DB_NODE * pDbNode;
    int                hashTblSizeLog2;
    int                grpHNBytes;
    int                grpDbNBytes;
    int                grpDbSize;
    int                ix;
    STATUS             grpDbServAddStatus;
    STATUS             msgQGrpServAddStatus;

    if (sizeLog2 < 1)
        return (ERROR);

    if (msgQDistGrpLibInstalled == TRUE)
        return (OK);

    if (hashLibInit () == ERROR)
        return (ERROR); /* hashLibInit() failed */

    if (distInqInit (DIST_INQ_HASH_TBL_SZ_LOG2) == ERROR)
        return (ERROR);

    hashTblSizeLog2 = sizeLog2 - 1;

    distGrpDbNmId = hashTblCreate (hashTblSizeLog2, msgQDistGrpHCmpStr,
                                   msgQDistGrpHFuncStr, KEY_ARG_STR);
    distGrpDbIdId = hashTblCreate (hashTblSizeLog2, msgQDistGrpHCmpId,
                                   msgQDistGrpHFuncId, KEY_ARG_ID);

    if (distGrpDbNmId && distGrpDbIdId)
        {
        grpDbSize = 1 << sizeLog2;
        grpHNBytes = grpDbSize * sizeof (DIST_GRP_HASH_NODE);
        grpDbNBytes = grpDbSize * sizeof (DIST_GRP_DB_NODE);

        distGrpDbNm = (DIST_GRP_HASH_NODE *) malloc (grpHNBytes);
        distGrpDbId = (DIST_GRP_HASH_NODE *) malloc (grpHNBytes);
        distGrpDb = (DIST_GRP_DB_NODE *) malloc (grpDbNBytes);

        if (distGrpDbNm && distGrpDbId && distGrpDb)
            {
            sllInit (&msgQDistGrpFreeList);

            for (ix = 0; ix < grpDbSize; ix++)
                {
                pDbNode = &distGrpDb[ix];
                pDbNode->ixNode = ix;
                sllPutAtHead (&msgQDistGrpFreeList, (SL_NODE *) pDbNode);
                distGrpDbNm[ix].pDbNode = pDbNode;
                distGrpDbId[ix].pDbNode = pDbNode;
                }

            msgQDistGrpDbLockInit();

            /* Add distributed group database service to table of services. */

            grpDbServAddStatus = distNetServAdd (
                    DIST_PKT_TYPE_DGDB,
                    msgQDistGdbInput,
                    DIST_DGDB_SERV_NAME,
                    DIST_DGDB_SERV_NET_PRIO,
                    DIST_DGDB_SERV_TASK_PRIO,
                    DIST_DGDB_SERV_TASK_STACK_SZ);

            /* Add message group service to table of services. */
            
            msgQGrpServAddStatus =
                distNetServAdd ( DIST_PKT_TYPE_MSG_Q_GRP,
                                 msgQDistGrpInput,
                                 DIST_MSG_Q_GRP_SERV_NAME,
                                 DIST_MSG_Q_GRP_SERV_NET_PRIO,
                                 DIST_MSG_Q_GRP_SERV_TASK_PRIO,
                                 DIST_MSG_Q_GRP_SERV_TASK_STACK_SZ);

            if (grpDbServAddStatus != ERROR && msgQGrpServAddStatus != ERROR)
                {
                msgQDistGrpLibInstalled = TRUE;
                return (OK);
                }
            }
        }

        /* cleanup, when error */

        if (distGrpDbNmId)    hashTblDelete (distGrpDbNmId);
        if (distGrpDbIdId)    hashTblDelete (distGrpDbIdId);
        if (distGrpDbNm)      free (distGrpDbNm);
        if (distGrpDbId)      free (distGrpDbId);
        if (distGrpDb)        free (distGrpDb);

        return (ERROR);
    }

/***************************************************************************
*
* msgQDistGrpAdd - add a distributed message queue to a group (VxFusion option)
*
* This routine adds the queue identified by the argument <msgQId> to a group
* with the ASCII name specified by the argument <distGrpName>. 
*
* Multicasting is based on distributed message queue groups.  If the group 
* does not exist, one is created.  Any number of message queues from different 
* nodes can be bound to a single group. In addition, a message queue can
* be added into any number of groups; msgQDistGrpAdd() must be called for each
* group of which the message queue is to be a member.
*
* The <options> parameter is presently unused and must be set to 0.
*
* This routine returns a message queue ID, MSG_Q_ID, that can be used directly 
* by msgQDistSend() or by the generic msgQSend() routine.  Do not call the
* msgQReceive() or msgQNumMsgs() routines or their distributed counterparts,
* msgQDistReceive() and msgQDistNumMsgs(), with a group message queue ID.
*
* As with msgQDistCreate(), use distNameAdd() to add the group message 
* queue ID returned by this routine to the distributed name database so 
* that the ID can be used by tasks on other nodes.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: MSG_Q_ID, or NULL if there is an error.
*
* ERRNO:
* \is
* \i S_msgQDistGrpLib_NAME_TOO_LONG
* The name of the group is too long.
* \i S_msgQDistGrpLib_INVALID_OPTION
* The <options> parameter is invalid.
* \i S_msgQDistGrpLib_DATABASE_FULL
* The group database is full.
* \i S_distLib_OBJ_ID_ERROR
* The <msgQId> parameter is not a distributed message queue.
* \ie
*
* SEE ALSO: msgQLib, msgQDistLib, distNameLib
* 
* INTERNAL NOTE: Takes <distGrpDbSemaphore>.
*/

MSG_Q_ID msgQDistGrpAdd
    (
    char *            distGrpName,  /* new or existing group name           */
    MSG_Q_ID          msgQId,       /* message queue to add to the group    */
    DIST_GRP_OPT      options       /* group message queue options - UNUSED */
    )
    {
    DIST_GRP_DB_NODE * pDistGrpDbNode;
    DIST_MSG_Q_ID      dMsgQId;      /* distributed message queue ID */
    DIST_OBJ_NODE * pObjNode;     /* ptr to object containing real ID */ 
    DIST_GAP_NODE   dGapNode;
    DIST_GAP_NODE * pDGapNodeTemp;

    /*
     * Check the parameters:  
     * - msgQId must be a distributed message queue. It cannot be a
     *   a standard message queue or a group.
     * - the group name cannot exceed DIST_NAME_MAX_LENGTH
     * - options must be 0
     *
     * If any of the parameters are invalid, set errno and return
     * NULL to indicate failure.
     */

    if (!ID_IS_DISTRIBUTED (msgQId)) /* not a distributed message queue */
        {
        errnoSet (S_distLib_OBJ_ID_ERROR);
        return NULL;
        }
    else                             /* is distributed msgQ */
        {

        pObjNode = MSG_Q_ID_TO_DIST_OBJ_NODE (msgQId);
        if (!IS_DIST_MSG_Q_OBJ (pObjNode))
            {
            /* legal object ID, but not a message queue */

            errnoSet (S_distLib_OBJ_ID_ERROR);
            return NULL;    
            }
       
        dMsgQId = (DIST_MSG_Q_ID) pObjNode->objNodeId;
        if (IS_DIST_MSG_Q_TYPE_GRP(dMsgQId))
            {
            /* ID refers to a group, not a plain dist. message queue */

            errnoSet (S_distLib_OBJ_ID_ERROR);
            return NULL;
            }
        }

    if (strlen (distGrpName) > DIST_NAME_MAX_LENGTH)
        {
        errnoSet (S_msgQDistGrpLib_NAME_TOO_LONG);
        return (NULL);    /* name too long */
        }

    if (options != 0)
        {
        errnoSet (S_msgQDistGrpLib_INVALID_OPTION);
        return (NULL);    /* options parameter currently unused */
        }

    /* Lock the database and try to find the group. */

    msgQDistGrpDbLock();

    pDistGrpDbNode = msgQDistGrpLclFindByName (distGrpName);
    if (pDistGrpDbNode == NULL)
        {
#ifdef MSG_Q_DIST_GRP_REPORT
        DIST_MSG_Q_GRP_ID    grpId;
#endif
        /* Group is unknown by now, create it. */

#ifdef MSG_Q_DIST_GRP_REPORT
        printf ("msgQDistGrpAdd: group is unknown by now--create it\n");
#endif
        pDistGrpDbNode = msgQDistGrpLclCreate (distGrpName, distGrpIdNext++,
                                               DIST_GRP_STATE_LOCAL_TRY);

        msgQDistGrpDbUnlock();

        if (pDistGrpDbNode == NULL)
            {
#ifdef DIST_DIAGNOSTIC
            distLog ("msgQDistGrpAdd: failed to create new group\n");
#endif
            errnoSet (S_msgQDistGrpLib_DATABASE_FULL);
            return (NULL);
            }

        /*
         * Agree on a global unique identifier for the group and
         * add the fist group member.
         */

#ifdef MSG_Q_DIST_GRP_REPORT
        printf ("msgQDistGrpAdd: enter agreement phase, propose 0x%lx\n",
                (u_long) msgQDistGrpLclGetId (pDistGrpDbNode));
        grpId =
#endif
            msgQDistGrpAgree (pDistGrpDbNode);
#ifdef MSG_Q_DIST_GRP_REPORT
        printf ("msgQDistGrpAdd: agreed on 0x%lx\n", (u_long) grpId);
#endif
        msgQDistGrpLclAddMember (pDistGrpDbNode, msgQId);
        }
    else
        {
        /* Group already exists. Check the state. */

#ifdef MSG_Q_DIST_GRP_REPORT
        printf ("msgQDistGrpAdd: group already exists--add object\n");
#endif
        while (pDistGrpDbNode->grpDbState < DIST_GRP_STATE_GLOBAL)
            {
                /*
                 * Group already exists, but has a local state. This means,
                 * somebody else tries to install the group.
                 * We unlock the group database and wait for a go-ahead.
                 */

                if (! pDistGrpDbNode->pGrpDbGapNode)
                    {
                    /* GAP node is not initialized by now */

                    distGapNodeInit (&dGapNode, pDistGrpDbNode, FALSE);
                    pDistGrpDbNode->pGrpDbGapNode = &dGapNode;
                    }

                 msgQDistGrpDbUnlock();
    
                 semTake (&dGapNode.gapWaitFor, WAIT_FOREVER);

                 msgQDistGrpDbLock();
            }

        if ((pDGapNodeTemp = pDistGrpDbNode->pGrpDbGapNode) != NULL)
            {
            pDistGrpDbNode->pGrpDbGapNode = NULL;
            distGapNodeDelete (pDGapNodeTemp);
            }

        msgQDistGrpDbUnlock();

        /*
         * Group exists and has a global state (at least now).
         * Add member to group.
         */

#ifdef MSG_Q_DIST_GRP_REPORT
        printf ("msgQDistGrpAdd: add member\n");
#endif
        msgQDistGrpLclAddMember (pDistGrpDbNode, msgQId);
        }

    return (pDistGrpDbNode->grpDbMsgQId);   /* retrun MSG_Q_ID */
    }

/***************************************************************************
*
* msgQDistGrpDelete - delete a distributed message queue from a group (VxFusion option)
*
* This routine deletes a distributed message queue from a group. 
*
* NOTE: For this release, it is not possible to remove a distributed message
* queue from a group.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: ERROR, always.
*
* ERRNO: S_distLib_NO_OBJECT_DESTROY
*
* INTERNAL NOTE: Takes <distGrpDbSemaphore>.
*/

STATUS msgQDistGrpDelete
    (
    char *   distGrpName,   /* group containing the queue to be deleted */
    MSG_Q_ID msgQId         /* ID of the message queue to delete */
    )
    {
#if 0
    DIST_MSG_Q_ID  distMsgQId;
    DIST_OBJ_ID    objId;
#else
        UNUSED_ARG(distGrpName);
        UNUSED_ARG(msgQId);
        
    errnoSet (S_distLib_NO_OBJECT_DESTROY);
    return (ERROR);    /* BY NOW */
#endif

    /* the rest of this function is not compiled */

#if 0
    if (ID_IS_DISTRIBUTED (msgQId))
        {
        objId = ((DIST_OBJ_NODE *) msgQId)->objNodeId;
        distMsgQId = DIST_OBJ_ID_TO_DIST_MSG_Q_ID (objId);
        }

    if (! (ID_IS_DISTRIBUTED (msgQId)) || IS_DIST_MSG_Q_LOCAL (distMsgQId))
        {
        /* msgQ is local */

        SL_NODE                *pNode;
        DIST_GRP_DB_NODE    *pDistGrpDbNode;
        DIST_GRP_MSG_Q_NODE *pDistGrpMsgQNode;
        DIST_GRP_MSG_Q_NODE    *pPrevNode = NULL;

        msgQDistGrpDbLock();
        pDistGrpDbNode = msgQDistGrpLclFindByName (distGrpName);
        msgQDistGrpDbUnlock();
        if (pDistGrpDbNode == NULL)
            return (ERROR);

        for (pNode = SLL_FIRST ((SL_LIST *) &pDistGrpDbNode->grpDbMsgQIdLst);
             pNode != NULL;
             pNode = SLL_NEXT (pNode))
            {
            pDistGrpMsgQNode = (DIST_GRP_MSG_Q_NODE *) pNode;
            if (pDistGrpMsgQNode->msgQId == msgQId)
                {
                sllRemove ((SL_LIST *) &pDistGrpDbNode->grpDbMsgQIdLst,
                        (SL_NODE *) pDistGrpMsgQNode, (SL_NODE *) pPrevNode);
                break;
                }
                pPrevNode = pDistGrpMsgQNode;
            }
        }
    else
        {
        /* msgQ is remote */

        return (ERROR);
        }

    return (OK);
#endif
    }

/***************************************************************************
*
* msgQDistGrpAgree - agree on a group identifier (VxFusion option)
*
* This routine determines the DIST_MSG_Q_GRP_ID associated with a
* DIST_GRP_DB_NODE .
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: A DIST_MSG_Q_GRP_ID .
*
* NOMANUAL
*/

DIST_MSG_Q_GRP_ID msgQDistGrpAgree
    (
    DIST_GRP_DB_NODE *pDistGrpDbNode  /* ptr to database node */
    )
    {
    DIST_MSG_Q_GRP_ID    grpId;

    /* Start GAP. */

    grpId = distGapStart (pDistGrpDbNode);

    return (grpId);
    }

/***************************************************************************
*
* msgQDistGrpLclSend - send to all group members registrated locally (VxFusion option)
*
* This routine sends a message to locally registered members of a group.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: OK, if successful; ERROR, if not.
*
* NOMANUAL
*/

LOCAL STATUS msgQDistGrpLclSend
    (
    DIST_MSG_Q_GRP_SEND_INQ * pInq,          /* pointer to inquiry node */
    DIST_MSG_Q_GRP_ID         distMsgQGrpId, /* group on which to send  */
    char *                    buffer,        /* message to send         */
    UINT                      nBytes,        /* length of message       */
    int                       timeout,       /* ticks to wait           */
    int                       priority       /* priority                */
    )
    {
    DIST_GRP_DB_NODE * pDistGrpDbNode;
    SL_NODE * pNode;
    DIST_GRP_MSG_Q_NODE * pDistGrpMsgQNode;
    STATUS                status;
    int                   tid;

    msgQDistGrpDbLock();
    pDistGrpDbNode = msgQDistGrpLclFindById (distMsgQGrpId);
    msgQDistGrpDbUnlock();

    if (pDistGrpDbNode == NULL)
        {
#ifdef MSG_Q_DIST_GRP_REPORT
        printf ("msgQDistGrpLclSend: group 0x%lx is unknown\n",
                (u_long) distMsgQGrpId);
#endif
        pInq->sendInqStatus = MSG_Q_DIST_GRP_STATUS_ILLEGAL_OBJ_ID;
        return (ERROR);                    /* illegal object id */
        }

    /*
     * For all members of this group; send'em the message.
     * Note: a member does not need to be a local message queue.
     */

    for (pNode = SLL_FIRST ((SL_LIST *) &pDistGrpDbNode->grpDbMsgQIdLst);
            pNode != NULL;
            pNode = SLL_NEXT (pNode))
        {

        pDistGrpMsgQNode = (DIST_GRP_MSG_Q_NODE *) pNode;

#ifdef MSG_Q_DIST_GRP_REPORT
        printf ("msgQDistGrpLclSend: send message to msgQId %p\n",
                pDistGrpMsgQNode->msgQId);
#endif
        /*
         * First try to send with NO_WAIT. If this fails due to a full
         * message queue and timeout is not NO_WAIT, spawn a task
         * and call msgQSend with the user specified timeout.
         */

        status = msgQDistSend (pDistGrpMsgQNode->msgQId, buffer, nBytes,
                               NO_WAIT, WAIT_FOREVER, priority);
        if (status == ERROR)
            {
#ifdef MSG_Q_DIST_GRP_REPORT
            printf ("msgQDistGrpLclSend: msgQDistSend() returned error\n");
#endif
            if (timeout != NO_WAIT && errno == S_objLib_OBJ_UNAVAILABLE)
                {

                msgQGrpSendInqLock(pInq);    /* lock inquiry node */

                /* timeout is not NO_WAIT and msgQ is full */

                pInq->sendInqNumBlocked++;

                msgQGrpSendInqUnlock(pInq);    /* unlock inquiry node */
    
                /* spawn a task that can wait */

                tid = taskSpawn (NULL,
                                 DIST_MSG_Q_GRP_WAIT_TASK_PRIO,
                                 0,
                                 DIST_MSG_Q_GRP_WAIT_TASK_STACK_SZ,
                                 (FUNCPTR) msgQDistGrpLclSendCanWait,
                                 (int) distInqGetId ((DIST_INQ *) pInq),
                                 (int) pDistGrpMsgQNode->msgQId,
                                 (int) buffer,
                                 (int) nBytes,
                                 (int) timeout,
                                 (int) priority,
                                 0, 0, 0, 0);
                if (tid == ERROR)
                    pInq->sendInqStatus = MSG_Q_DIST_GRP_STATUS_ERROR;
                }
            else
                {

                pInq->sendInqStatus = MSG_Q_DIST_GRP_STATUS_ERROR;
                }
            }
        }

    if (pInq->sendInqNumBlocked == 0)
        {
        /* If we leave nobody blocked, decrease the outstanding counter. */

        msgQGrpSendInqLock(pInq);    /* lock inquiry node */
        if (--pInq->sendInqNumOutstanding == 0)
            {
            msgQGrpSendInqUnlock(pInq);    /* unlock inquiry node */
            semGive (&pInq->sendInqWait);
            }
        else
            msgQGrpSendInqUnlock(pInq);    /* unlock inquiry node */
        }



    return (OK);
    }

/***************************************************************************
*
* msgQDistGrpLclSendCanWait - send buffer to local members of a group (VxFusion option)
*
* This routine sends a message to local group members.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: N/A
*
* NOMANUAL
*/

LOCAL void msgQDistGrpLclSendCanWait
    (
    DIST_INQ_ID    inqId,         /* inquiry id             */
    MSG_Q_ID       msgQId,        /* queue on which to send */
    char *         buffer,        /* message to send        */
    UINT           nBytes,        /* length of message      */
    int            timeout,       /* ticks to wait          */
    int            priority       /* priority               */
    )
    {
    DIST_MSG_Q_GRP_SEND_INQ * pInq;
    STATUS                    status;

    status = msgQDistSend (msgQId, buffer, nBytes, timeout,
            WAIT_FOREVER, priority);
    if (status == OK)
        {
        if ((pInq = (DIST_MSG_Q_GRP_SEND_INQ *) distInqFind (inqId)) == NULL)
            return;

        msgQGrpSendInqLock(pInq);            /* lock inquiry node */

        if (--pInq->sendInqNumBlocked == 0)
            if (--pInq->sendInqNumOutstanding == 0)
                {
                msgQGrpSendInqUnlock(pInq);    /* unlock inquiry node */
                semGive (&pInq->sendInqWait);
                return;
                }

        msgQGrpSendInqUnlock(pInq);        /* unlock inquiry node */
        }
    }

/***************************************************************************
*
* msgQDistGrpSend - send buffer to local and remote members of a group (VxFusion option)
*
* This routine sends a message to local and remote members of a group.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: OK, if successful; ERROR, is unsuccessful.
*
* NOMANUAL
*/

STATUS msgQDistGrpSend
    (
    DIST_MSG_Q_GRP_ID   distMsgQGrpId,  /* group on which to send          */
    char *              buffer,         /* message to send                 */
    UINT                nBytes,         /* length of message               */
    int                 msgQTimeout,    /* ticks to wait at message queues */
    int                 overallTimeout, /* ticks to wait overall           */
    int                 priority        /* priority                        */
    )
    {
    DIST_PKT_MSG_Q_GRP_SEND    pktSend;
    DIST_MSG_Q_GRP_SEND_INQ    inquiryNode;
    DIST_INQ_ID                inquiryId;
    DIST_IOVEC                 distIOVec[2];
    STATUS                     status;

    inquiryNode.sendInq.inqType = DIST_MSG_Q_GRP_INQ_TYPE_SEND;
    msgQGrpSendInqLockInit (&inquiryNode);
    semBInit (&(inquiryNode.sendInqWait), SEM_Q_FIFO, SEM_EMPTY);
    inquiryNode.sendInqTask = taskIdSelf();
    inquiryNode.sendInqNumBlocked = 0;

    inquiryNode.sendInqNumOutstanding =
            distNodeGetNumNodes(DIST_NODE_NUM_NODES_ALIVE);

    inquiryNode.sendInqStatus = MSG_Q_DIST_GRP_STATUS_OK;

    inquiryId = distInqRegister ((DIST_INQ *) &inquiryNode);

    status = msgQDistGrpLclSend (&inquiryNode, distMsgQGrpId, buffer, nBytes,
                                 msgQTimeout, priority);
    if (status == ERROR)
        {
#ifdef MSG_Q_DIST_GRP_REPORT
        printf ("msgQDistGrpSend: group is unknown\n");
#endif
        distInqCancel ((DIST_INQ *) &inquiryNode);
        return (ERROR);
        }

#ifdef MSG_Q_DIST_GRP_REPORT
    if (inquiryNode.sendInqNumBlocked)
        printf ("msgQDistGrpSend: %d local agents blocked on queues\n",
                inquiryNode.sendInqNumBlocked);
#endif

    /*
     * Broadcast message to the net. Every message addressed to a group
     * is broadcasted to the net.
     */

    pktSend.pktMsgQGrpSendHdr.pktType = DIST_PKT_TYPE_MSG_Q_GRP;
    pktSend.pktMsgQGrpSendHdr.pktSubType = DIST_PKT_TYPE_MSG_Q_GRP_SEND;
    pktSend.pktMsgQGrpSendInqId = htonl((uint32_t) inquiryId);
    pktSend.pktMsgQGrpSendTimeout =
            htonl ((uint32_t) DIST_TICKS_TO_MSEC (msgQTimeout));
    pktSend.pktMsgQGrpSendId = htons (distMsgQGrpId);

    /* use IOV stuff here, since we do not want to copy data */

    distIOVec[0].pIOBuffer = &pktSend;
    distIOVec[0].IOLen = DIST_PKT_HDR_SIZEOF (DIST_PKT_MSG_Q_GRP_SEND);
    distIOVec[1].pIOBuffer = buffer;
    distIOVec[1].IOLen = nBytes;

#ifdef MSG_Q_DIST_GRP_REPORT
    printf ("msgQDistGrpSend: broadcast message\n");
#endif
    status = distNetIOVSend (DIST_IF_BROADCAST_ADDR, &distIOVec[0], 2,
            WAIT_FOREVER, DIST_MSG_Q_PRIO_TO_NET_PRIO (priority));
    if (status == ERROR)
        {
#ifdef MSG_Q_DIST_GRP_REPORT
        printf ("msgQDistGrpSend: broadcast failed\n");
#endif
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

    if (inquiryNode.sendInqNumOutstanding > 0)
        {
        /* overall timeout */

        errnoSet (S_msgQDistLib_OVERALL_TIMEOUT);
        return (ERROR);
        }
 
    if (inquiryNode.sendInqStatus != MSG_Q_DIST_GRP_STATUS_OK)
        return (ERROR);

    return (OK);
    }

/***************************************************************************
*
* msgQDistGrpAgent - send an inquiry to a group (VxFusion option)
*
* This routine sends an inquiry message to a message group.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: OK, if successfully sent; ERROR, if not sent.
*
* NOMANUAL
*/

LOCAL STATUS msgQDistGrpAgent
    (
    DIST_NODE_ID            nodeIdSender,   /* node id of the sender  */
    DIST_INQ_ID             inqIdSender,    /* inquiry id at sender   */
    DIST_MSG_Q_GRP_ID       distMsgQGrpId,  /* group on which to send */
    char *                  buffer,         /* message to send        */
    UINT                    nBytes,         /* length of message      */
    int                     timeout,        /* ticks to wait          */
    int                     priority        /* priority               */
    )
    {
    DIST_MSG_Q_GRP_SEND_INQ    inquiryNode;

    /*
     * Our caller cannot wait, so we have to create
     * the inquiry id.
     */

    inquiryNode.sendInq.inqType = DIST_MSG_Q_GRP_INQ_TYPE_SEND;
    semBInit (&inquiryNode.sendInqWait, SEM_Q_FIFO, SEM_EMPTY);

    /* inquiryNode.sendInqTask = taskIdSelf(); */

    inquiryNode.sendInqNumBlocked = 0;
    inquiryNode.sendInqNumOutstanding = 1;
    inquiryNode.sendInqStatus = MSG_Q_DIST_GRP_STATUS_OK;

    distInqRegister ((DIST_INQ *) &inquiryNode);

    if (msgQDistGrpLclSend (&inquiryNode, distMsgQGrpId, buffer, nBytes,
            timeout, priority) == ERROR)
        {
        free (buffer);
        msgQDistGrpSendStatus (nodeIdSender, inqIdSender,
                MSG_Q_DIST_GRP_STATUS_ILLEGAL_OBJ_ID);
        return (ERROR);
        }

    semTake (&inquiryNode.sendInqWait, WAIT_FOREVER);
    
    distInqCancel ((DIST_INQ *) &inquiryNode);
    
    free (buffer);

    if (inquiryNode.sendInqStatus != MSG_Q_DIST_GRP_STATUS_OK)
        {
#ifdef MSG_Q_DIST_GRP_REPORT
        printf ("msgQDistGrpAgent: something failed\n");
#endif
        msgQDistGrpSendStatus (nodeIdSender, inqIdSender,
                (INT16) inquiryNode.sendInqStatus);
        return (ERROR);
        }

#ifdef MSG_Q_DIST_GRP_REPORT
    printf ("msgQDistGrpAgent: respond with OK\n");
#endif
    msgQDistGrpSendStatus (nodeIdSender, inqIdSender,
            MSG_Q_DIST_GRP_STATUS_OK);
    return (OK);
    }


/***************************************************************************
*
* msgQDistGrpSendStatus - send status and errno (VxFusion option)
*
* This routine sends local status to an inquirying node.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: OK, if status sent; ERROR, if not sent.
*
* NOMANUAL
*/

LOCAL STATUS msgQDistGrpSendStatus
    (
    DIST_NODE_ID        nodeIdDest,   /* node ID of destination */
    DIST_INQ_ID         inqId,        /* the inquiry ID */
    DIST_MSG_Q_STATUS   dStatus       /* status to send */
    )
    {
    DIST_PKT_MSG_Q_GRP_STATUS    pktStatus;
    STATUS                        status;

    pktStatus.pktMsgQGrpStatusHdr.pktType = DIST_PKT_TYPE_MSG_Q_GRP;
    pktStatus.pktMsgQGrpStatusHdr.pktSubType = DIST_PKT_TYPE_MSG_Q_GRP_STATUS;
    pktStatus.pktMsgQGrpStatusInqId = htonl ((uint32_t) inqId);
    pktStatus.pktMsgQGrpStatusErrno = htonl ((uint32_t) errnoGet());
    pktStatus.pktMsgQGrpStatusDStatus = htons ((uint16_t) dStatus);

    status = distNetSend (nodeIdDest, (DIST_PKT *) &pktStatus,
                          DIST_PKT_HDR_SIZEOF (DIST_PKT_MSG_Q_GRP_STATUS),
                          WAIT_FOREVER, DIST_MSG_Q_GRP_PRIO);

    return (status);
    }

/***************************************************************************
*
* msgQDistGdbInput - called for distributed group database updates (VxFusion option)
*
* This routine updates the local group database.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: Status of update.
*
* NOMANUAL
*/

LOCAL DIST_STATUS msgQDistGdbInput
    (
    DIST_NODE_ID    nodeIdSrc,   /* unused */
    DIST_TBUF_HDR * pTBufHdr     /* ptr to received TBUF header */
    )
    {
    DIST_PKT * pPkt;
    int        pktLen;
    DIST_PKT_DGDB_ADD    pktAdd;
    DIST_MSG_Q_GRP_ID    grpId;
    DIST_GRP_DB_NODE *   pDbNodeNew;
    DIST_NODE_ID         grpCreator;
    char                 grpName[DIST_NAME_MAX_LENGTH + 1];
    int                  sz;
    
    UNUSED_ARG(nodeIdSrc);
    
    pktLen = pTBufHdr->tBufHdrOverall;

    if (pktLen < sizeof (DIST_PKT))
        distPanic ("msgQDistGdbInput: packet too short\n");

    pPkt = (DIST_PKT *) (DIST_TBUF_GET_NEXT (pTBufHdr))->pTBufData;

    switch (pPkt->pktSubType)
        {
        case DIST_PKT_TYPE_DGDB_ADD:
            {
            if (pktLen < DIST_PKT_HDR_SIZEOF (DIST_PKT_DGDB_ADD))
                distPanic ("msgQDistGdbInput: packet too short\n");

            distTBufCopy (DIST_TBUF_GET_NEXT (pTBufHdr), 0, (char *) &pktAdd,
                          DIST_PKT_HDR_SIZEOF (DIST_PKT_DGDB_ADD));
            
            if ((sz = pktLen - DIST_PKT_HDR_SIZEOF (DIST_PKT_DGDB_ADD)) >
                    DIST_NAME_MAX_LENGTH + 1)
                distPanic ("msgQDistGdbInput: name too long\n");

            distTBufCopy (DIST_TBUF_GET_NEXT (pTBufHdr),
                          DIST_PKT_HDR_SIZEOF (DIST_PKT_DGDB_ADD),
                          (char *) &grpName, sz);

            grpId = ntohs (pktAdd.pktDgdbAddId);
            grpCreator = ntohl (pktAdd.pktDgdbAddCreator);

            if (grpId >= distGrpIdNext)
                distGrpIdNext = grpId + 1;

#ifdef MSG_Q_DIST_GRP_REPORT
            printf ("msgQDistGrpInput: group database add from node %ld\n",
                    nodeIdSrc);
            printf ("msgQDistGrpInput: bind `%s' to group id %d\n",
                    (char *) &grpName, grpId);
#endif
            msgQDistGrpDbLock();

            pDbNodeNew = msgQDistGrpLclCreate ((char *) &grpName, grpId,
                                               DIST_GRP_STATE_GLOBAL);
            if (pDbNodeNew == NULL)
                {
                msgQDistGrpDbUnlock();
                distPanic ("msgQDistGdbInput: group creation failed\n");
                }

            msgQDistGrpLclSetCreator (pDbNodeNew, grpCreator);

            msgQDistGrpDbUnlock();
            return (DIST_GDB_STATUS_OK);
            }

        default:
            return (DIST_GDB_STATUS_PROTOCOL_ERROR);
        }
    }

/***************************************************************************
*
* msgQDistGrpInput - called when a new group message arrives at the system (VxFusion option)
*
* This routine is called whenever a group message is received.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: Status of message processing.
*
* NOMANUAL
*/

LOCAL DIST_STATUS msgQDistGrpInput
    (
    DIST_NODE_ID    nodeIdSrc,        /* source node ID */
    DIST_TBUF_HDR * pTBufHdr          /* ptr to message */
    )
    {
    DIST_PKT * pPkt;
    int        pktLen;
    DIST_PKT_MSG_Q_GRP_SEND  pktSend;
    DIST_MSG_Q_GRP_ID        dMsgQGrpId;
    DIST_GRP_DB_NODE *       pDistGrpDbNode;
    DIST_INQ_ID              inqIdSrc;
    uint32_t                 timeout_msec;
    UINT                     nBytes;
    char *                   buffer;
    int                      tid;
    int                      timeout;
    int                      prio;
    DIST_PKT_MSG_Q_GRP_STATUS    pktStatus;
    DIST_STATUS                  dStatus;
    DIST_INQ_ID                  inqId;
    DIST_INQ *                   pGenInq;
    int                          errnoRemote;
    DIST_MSG_Q_GRP_SEND_INQ *    pInq;

    pktLen = pTBufHdr->tBufHdrOverall;

    if (pktLen < sizeof (DIST_PKT))
        distPanic ("msgQDistGrpInput: packet too short\n");

    pPkt = (DIST_PKT *) ((DIST_TBUF_GET_NEXT (pTBufHdr))->pTBufData);

    switch (pPkt->pktSubType)
        {
        case DIST_PKT_TYPE_MSG_Q_GRP_SEND:
            {
            if (pktLen < DIST_PKT_HDR_SIZEOF (DIST_PKT_MSG_Q_GRP_SEND))
                 distPanic ("msgQDistGrpInput/SEND: packet too short\n");

            distTBufCopy (DIST_TBUF_GET_NEXT (pTBufHdr), 0, (char *)&pktSend,
                          DIST_PKT_HDR_SIZEOF (DIST_PKT_MSG_Q_GRP_SEND));

            inqIdSrc = (DIST_INQ_ID) 
                       htonl(pktSend.pktMsgQGrpSendInqId);
            timeout_msec = ntohl (pktSend.pktMsgQGrpSendTimeout);
            timeout = DIST_MSEC_TO_TICKS (timeout_msec);
            nBytes = pktLen - DIST_PKT_HDR_SIZEOF (DIST_PKT_MSG_Q_GRP_SEND);
            dMsgQGrpId = ntohs (pktSend.pktMsgQGrpSendId);

            /* Check for an empty group */

            msgQDistGrpDbLock();
            pDistGrpDbNode = msgQDistGrpLclFindById (dMsgQGrpId);
            msgQDistGrpDbUnlock();

            if (pDistGrpDbNode == NULL)
                {
                msgQDistGrpSendStatus (nodeIdSrc, inqIdSrc,
                                       MSG_Q_DIST_GRP_STATUS_UNAVAIL);
                return (MSG_Q_DIST_GRP_STATUS_UNAVAIL);
                }

            if (SLL_FIRST ((SL_LIST *)&pDistGrpDbNode->grpDbMsgQIdLst)
                        == NULL)
                {
                msgQDistGrpSendStatus (nodeIdSrc, inqIdSrc,
                                       MSG_Q_DIST_GRP_STATUS_OK);
#ifdef MSG_Q_DIST_GRP_REPORT
                printf ("msgQDistGrpInput/SEND: group has no local members\n");
#endif
                return (MSG_Q_DIST_GRP_STATUS_OK);
                }

            /* Using malloc() here is not very satisfiing. */

            if ((buffer = (char *) malloc (nBytes)) == NULL)
                {
#ifdef MSG_Q_DIST_GRP_REPORT
                printf ("msgQDistGrpInput/SEND: out of memory\n");
#endif
                msgQDistGrpSendStatus (nodeIdSrc,
                                    inqIdSrc,
                                    MSG_Q_DIST_GRP_STATUS_NOT_ENOUGH_MEMORY);

                distStat.memShortage++;
                distStat.msgQGrpInDiscarded++;
                return (MSG_Q_DIST_GRP_STATUS_NOT_ENOUGH_MEMORY);
                }
    
            distTBufCopy (DIST_TBUF_GET_NEXT (pTBufHdr),
                          DIST_PKT_HDR_SIZEOF (DIST_PKT_MSG_Q_GRP_SEND),
                          buffer, nBytes);

            prio = NET_PRIO_TO_DIST_MSG_Q_PRIO (pTBufHdr->tBufHdrPrio);

#ifdef MSG_Q_DIST_GRP_REPORT
            printf ("msgQDistGrpInput: message from node %ld to group 0x%x\n",
                    nodeIdSrc, dMsgQGrpId);
#endif
            /*
             * Send message to all members, registrated on the local node for
             * this group.
             */

            tid = taskSpawn (NULL,
                            DIST_MSG_Q_GRP_WAIT_TASK_PRIO,
                            0,
                            DIST_MSG_Q_GRP_WAIT_TASK_STACK_SZ,
                            (FUNCPTR) msgQDistGrpAgent,
                            (int) nodeIdSrc,
                            (int) inqIdSrc,
                            (int) dMsgQGrpId,
                            (int) buffer,
                            (int) nBytes,
                            (int) timeout,
                            (int) prio,
                            0, 0, 0);
            if (tid == ERROR)
                {
                free (buffer);
                msgQDistGrpSendStatus (nodeIdSrc, inqIdSrc,
                                       MSG_Q_DIST_GRP_STATUS_UNAVAIL);
                return (MSG_Q_DIST_GRP_STATUS_UNAVAIL);
                }

            return (MSG_Q_DIST_GRP_STATUS_OK);
            }

        case DIST_PKT_TYPE_MSG_Q_GRP_STATUS:
            {
            if (pktLen != DIST_PKT_HDR_SIZEOF (DIST_PKT_MSG_Q_GRP_STATUS))
                distPanic ("msgQDistGrpInput/STATUS: packet too short\n");

            /* First copy the error packet form the TBuf list. */
            
            distTBufCopy (DIST_TBUF_GET_NEXT (pTBufHdr), 0,
                          (char *) &pktStatus,
                          DIST_PKT_HDR_SIZEOF (DIST_PKT_MSG_Q_GRP_STATUS));

            dStatus = (DIST_STATUS) ntohs (pktStatus.pktMsgQGrpStatusDStatus);
            errnoRemote = ntohl (pktStatus.pktMsgQGrpStatusErrno);

            inqId = (DIST_INQ_ID) ntohl (pktStatus.pktMsgQGrpStatusInqId);
            if (! (pGenInq = distInqFind (inqId)))
                return (MSG_Q_DIST_GRP_STATUS_LOCAL_TIMEOUT);

            /* See who is addressed by the STATUS telegram. */

            switch (pGenInq->inqType)
                {
                case DIST_MSG_Q_GRP_INQ_TYPE_SEND:
                    {
                    pInq = (DIST_MSG_Q_GRP_SEND_INQ *) pGenInq;

                    /*
                     * Possible errors here:
                     *    MSG_Q_DIST_GRP_STATUS_OK
                     *    MSG_Q_DIST_GRP_STATUS_ERROR
                     *    MSG_Q_DIST_GRP_STATUS_UNAVAIL
                     *    MSG_Q_DIST_GRP_STATUS_ILLEGAL_OBJ_ID
                     *    MSG_Q_DIST_GRP_STATUS_NOT_ENOUGH_MEMORY
                     */

                    switch (dStatus)
                        {
                        case MSG_Q_DIST_GRP_STATUS_OK:
                            if (--pInq->sendInqNumOutstanding == 0)
                                semGive (&pInq->sendInqWait);
                            break;
                        case MSG_Q_DIST_GRP_STATUS_ERROR:
                        case MSG_Q_DIST_GRP_STATUS_UNAVAIL:
                        case MSG_Q_DIST_GRP_STATUS_ILLEGAL_OBJ_ID:
                        case MSG_Q_DIST_GRP_STATUS_NOT_ENOUGH_MEMORY:
                            errnoOfTaskSet (pInq->sendInqTask, errnoRemote);
                            break;
                        default:
#ifdef MSG_Q_DIST_GRP_REPORT
                            printf ("msgQDistGrpInput/STATUS/SEND: status?\n");
#endif
                            break;
                        }

                    return (MSG_Q_DIST_GRP_STATUS_OK);
                    }
                default:
#ifdef MSG_Q_DIST_GRP_REPORT
                    printf ("msgQDistGrpInput/STATUS: unexpected inq (%d)\n",
                            pGenInq->inqType);
#endif
                    return (MSG_Q_DIST_GRP_STATUS_INTERNAL_ERROR);
                }
            }

        default:
#ifdef MSG_Q_DIST_GRP_REPORT
            printf ("msgQDistGrpInput/STATUS: unknown group subtype (%d)\n",
                    pPkt->pktSubType);
#endif
            return (MSG_Q_DIST_GRP_STATUS_PROTOCOL_ERROR);
        }

#ifdef UNDEFINED   /* supposedly unreached */
    if (status == ERROR)
        return (MSG_Q_DIST_GRP_STATUS_ERROR);

    return (MSG_Q_DIST_GRP_STATUS_OK);
#endif
    }

/***************************************************************************
*
* msgQDistGrpLclSetId - change id of a group in local database (VxFusion option)
*
* This routine changes the group id of a node in the local database.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: N/A
*
* NOTE: Call with <distGrpDbSemaphore> taken.
*
* NOMANUAL
*/

void msgQDistGrpLclSetId
    (
    DIST_GRP_DB_NODE *   pGrpDbNode,   /* the node to change */
    DIST_MSG_Q_GRP_ID    grpId         /* the new ID */
    )
    {
    DIST_OBJ_NODE *   pObjNode;
    DIST_MSG_Q_ID     dMsgQId;
    int               ix = pGrpDbNode->ixNode;

    hashTblRemove (distGrpDbIdId, (HASH_NODE *) &(distGrpDbId[ix]));
    pGrpDbNode->grpDbId = grpId;

    pObjNode = MSG_Q_ID_TO_DIST_OBJ_NODE (pGrpDbNode->grpDbMsgQId);
    dMsgQId = DIST_MSG_Q_GRP_ID_TO_DIST_MSG_Q_ID (grpId);
    pObjNode->objNodeId = DIST_MSG_Q_ID_TO_DIST_OBJ_ID (dMsgQId);

    hashTblPut (distGrpDbIdId, (HASH_NODE *) &(distGrpDbId[ix]));
    }

/***************************************************************************
*
* msgQDistGrpLclCreate - create a new group in local database (VxFusion option)
*
* This routine creates a new group in the local database.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: A ptr to the group node.
*
* NOTE: Call with <distGrpDbSemaphore> taken.
*
* NOMANUAL
*/

DIST_GRP_DB_NODE * msgQDistGrpLclCreate
    (
    char *               grpName,     /* group name */
    DIST_MSG_Q_GRP_ID    grpId,       /* group ID */
    DIST_GRP_STATE       grpState     /* initial state */
    )
    {
    DIST_GRP_DB_NODE *   pDbNode;
    DIST_OBJ_NODE *      pObjNode;

    /* get a free database node */

    if (! (pDbNode = (DIST_GRP_DB_NODE *) sllGet (&msgQDistGrpFreeList)))
        {
        return (NULL);    /* database is full */
        }

    /* init database node */

    bcopy (grpName, (char *) &(pDbNode->grpDbName), strlen (grpName) + 1);
    sllInit (&(pDbNode->grpDbMsgQIdLst));
    pDbNode->grpDbState = grpState;
    pDbNode->grpDbId = grpId;
    pDbNode->pGrpDbGapNode = NULL;

    /* Create message queue id. */

    pObjNode = distObjNodeGet();
    pObjNode->objNodeType    = DIST_OBJ_TYPE_MSG_Q;
    pObjNode->objNodeId      = DIST_MSG_Q_GRP_ID_TO_DIST_OBJ_ID (grpId);

    pDbNode->grpDbMsgQId = DIST_OBJ_NODE_TO_MSG_Q_ID (pObjNode);

    /* Link group name hash node in name hash table. */

    hashTblPut (distGrpDbNmId, (HASH_NODE *) &(distGrpDbNm[pDbNode->ixNode]));

    /* Link group id hash node in id hash table. */

    hashTblPut (distGrpDbIdId, (HASH_NODE *) &(distGrpDbId[pDbNode->ixNode]));

#ifdef DIST_MSG_Q_GRP_REPORT
    printf ("msgQDistGrpLclCreate: `%s' (id 0x%lx, state %d) created\n",
            grpName, grpId, grpState);
#endif

    return (pDbNode);
    }

/***************************************************************************
*
* msgQDistGrpLclAddMember - add a member to a group in the local database (VxFusion option)
*
* This routine adds a member to a group in the local database.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: OK, if the member was added; ERROR, if not.
*
* NOMANUAL
*/

STATUS msgQDistGrpLclAddMember
    (
    DIST_GRP_DB_NODE * pDbNode,      /* ptr to node of group */
    MSG_Q_ID           msgQId        /* msgQ ID to add to group */
    )
    {
    DIST_GRP_MSG_Q_NODE * distGrpMsgQNode;

    distGrpMsgQNode = (DIST_GRP_MSG_Q_NODE *)
                        malloc (sizeof (DIST_GRP_MSG_Q_NODE));

    if (distGrpMsgQNode == NULL)
        return (ERROR);    /* out of memory */

    distGrpMsgQNode->msgQId = msgQId;

    msgQDistGrpDbLock();

    sllPutAtHead (&(pDbNode->grpDbMsgQIdLst), (SL_NODE *) distGrpMsgQNode);

    msgQDistGrpDbUnlock();

    return (OK);
    }

/***************************************************************************
*
* msgQDistGrpLclFindById - Find a group's node given in group ID (VxFusion option)
*
* This routine looks up a group node when given the group ID.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: A pointer to a group node, or NULL.
*
* NOTE: Take <distGrpDbSemaphore> before calling.
*
* NOMANUAL
*/

DIST_GRP_DB_NODE * msgQDistGrpLclFindById
    (
    DIST_MSG_Q_GRP_ID distGrpId    /* the group ID to look for */
    )
    {
    DIST_GRP_HASH_NODE    hMatchNode;
    DIST_GRP_HASH_NODE *  pHNode;
    DIST_GRP_DB_NODE      dbMatchNode;

    hMatchNode.pDbNode = &dbMatchNode;
    dbMatchNode.grpDbId = distGrpId;

    pHNode = (DIST_GRP_HASH_NODE *) hashTblFind (distGrpDbIdId,
                                                 (HASH_NODE *) &hMatchNode,
                                                 KEY_CMP_ARG_ID);

    if (pHNode == NULL)
        return (NULL);    /* not found */

    return (pHNode->pDbNode);
    }

/***************************************************************************
*
* msgQDistGrpLclFindByName - Find a group node given a group name (VxFusion option)
*
* This routine looks up a group node when given the name of the group.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: A pointer to a group node, or NULL.
*
* NOTE: Call with <distGrpDbSemaphore> taken.
*
* NOMANUAL
*/

DIST_GRP_DB_NODE * msgQDistGrpLclFindByName
    (
    char * grpName         /* group name to find */
    )
    {
    DIST_GRP_HASH_NODE  hMatchNode, *pHNode;
    DIST_GRP_DB_NODE    dbMatchNode;

    hMatchNode.pDbNode = &dbMatchNode;
    bcopy (grpName, (char *) &(dbMatchNode.grpDbName), strlen (grpName) + 1);

    pHNode = (DIST_GRP_HASH_NODE *) hashTblFind (distGrpDbNmId,
                                                 (HASH_NODE *) &hMatchNode,
                                                 KEY_CMP_ARG_STR);

    if (pHNode == NULL)
        return (NULL);    /* not found */

    return (pHNode->pDbNode);
    }

/***************************************************************************
*
* msgQDistGrpLclEach - Step through nodes in the database (VxFusion option)
*
* This routine steps through all nodes in the database, calling <routine>
* with argument <routineArg> for each node.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: N/A
*
* NOTE: Takes <distGrpDbSemaphore>.
*
* NOMANUAL
*/

void msgQDistGrpLclEach
    (
    FUNCPTR routine,        /* routine to call for each node */
    int     routineArg      /* argument to pass to routine */
    )
    {

    msgQDistGrpDbLock();
    
    hashTblEach (distGrpDbNmId, routine, routineArg);

    msgQDistGrpDbUnlock();

    }

/***************************************************************************
*
* msgQDistGrpBurst - burst out group database (VxFusion option)
*
* This routine is used by INCO to update the remote group database on
* node <nodeId>. All entries in the database are transmitted.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: STATUS of remote operation.
*
* NOMANUAL
*/

STATUS msgQDistGrpBurst
    (
    DIST_NODE_ID    nodeId          /* node to update */
    )
    {
    DIST_GRP_BURST    burst;

    burst.burstNodeId = nodeId;
    burst.burstStatus = OK;

    msgQDistGrpLclEach ((FUNCPTR) msgQDistGrpBurstOne, (int) &burst);

    return (burst.burstStatus);
    }

/***************************************************************************
*
* msgQDistGrpBurstOne - burst out single group database entry (VxFusion option)
*
* This routine bursts out a database entry to a node.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: TRUE, if successfully; FALSE, if not.
*
* NOMANUAL
*/

BOOL msgQDistGrpBurstOne
    (
    DIST_GRP_HASH_NODE * pGrpHashNode,   /* specifies node to send */
    DIST_GRP_BURST *     pBurst          /* target node */
    )
    {
    DIST_PKT_DGDB_ADD    pktAdd;
    DIST_GRP_DB_NODE *   pNode = pGrpHashNode->pDbNode;
    DIST_IOVEC           distIOVec[2];
    STATUS               status;

    pktAdd.pktDgdbAddHdr.pktType = DIST_PKT_TYPE_DGDB;
    pktAdd.pktDgdbAddHdr.pktSubType = DIST_PKT_TYPE_DGDB_ADD;
    pktAdd.pktDgdbAddId = htons (pNode->grpDbId);
    pktAdd.pktDgdbAddCreator = htonl (pNode->grpDbNodeId);

    /* use IOV stuff here, since we do not want to copy data */

    distIOVec[0].pIOBuffer = &pktAdd;
    distIOVec[0].IOLen = DIST_PKT_HDR_SIZEOF (DIST_PKT_DGDB_ADD);
    distIOVec[1].pIOBuffer = (char *) &pNode->grpDbName;
    distIOVec[1].IOLen = strlen ((char *) &pNode->grpDbName) + 1;

    status = distNetIOVSend (pBurst->burstNodeId, &distIOVec[0], 2,
            WAIT_FOREVER, DIST_DGDB_PRIO);
    if ((pBurst->burstStatus = status) == ERROR)
        return (FALSE);

    return (TRUE);
    }

/***************************************************************************
*
* msgQDistGrpHCmpStr - determine if two nodes have the same name (VxFusion option)
*
* This routine is the hash compare function for group names.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: TRUE, if the nodes have the same name; FALSE, otherwise.
*
* NOMANUAL
*/

LOCAL BOOL msgQDistGrpHCmpStr
    (
    DIST_GRP_HASH_NODE * pMatchHNode,   /* first node */
    DIST_GRP_HASH_NODE * pHNode,        /* second node */
    int                  keyCmpArg      /* not used */
    )
    {
    
    UNUSED_ARG(keyCmpArg);
    
    if (strcmp ((char *) &pMatchHNode->pDbNode->grpDbName,
                (char *) &pHNode->pDbNode->grpDbName) == 0)
        return (TRUE);
    else
        return (FALSE);
    }

/***************************************************************************
*
* msgQDistGrpHFuncStr - hash function for strings (VxFusion option)
*
* This routine computes the hash value for a node's group name.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: The hash value.
*
* NOMANUAL
*/

LOCAL INT32 msgQDistGrpHFuncStr
    (
    int                    elements,    /* elements in hash table */
    DIST_GRP_HASH_NODE *   pHNode,      /* node whose name to hash */
    int                    seed         /* seed for hashing */
    )
    {
    char * tkey;
    int    hash = 0;

    for (tkey = (char *) &pHNode->pDbNode->grpDbName; *tkey != '\0'; tkey++)
        hash = hash * seed + (unsigned int) *tkey;

    return (hash & (elements - 1));
    }

/***************************************************************************
*
* msgQDistGrpHCmpId - compare two group ID's (VxFusion option)
*
* This routine is the hash compare function for group ID's.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: TRUE, if ID's match; FALSE, otherwise.
*
* NOMANUAL
*/

LOCAL BOOL msgQDistGrpHCmpId
    (
    DIST_GRP_HASH_NODE *   pMatchNode,  /* first node */
    DIST_GRP_HASH_NODE *   pHNode,      /* second node */
    int                    keyArg       /* unused */
    )
    {
    DIST_MSG_Q_GRP_ID    distGrpId1 = pMatchNode->pDbNode->grpDbId;
    DIST_MSG_Q_GRP_ID    distGrpId2 = pHNode->pDbNode->grpDbId;
    
    UNUSED_ARG(keyArg);
    
    if (distGrpId1 == distGrpId2)
        return (TRUE);
    else
        return (FALSE);
    }

/***************************************************************************
*
* msgQDistGrpHFuncId - hash function for group ID's (VxFusion option)
*
* This routine computes a hash value for a group's ID.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: The hash value.
*
* NOMANUAL
*/

LOCAL INT32 msgQDistGrpHFuncId
    (
    int                    elements,   /* elements in hash table */
    DIST_GRP_HASH_NODE *   pHNode,     /* node whose ID to hash */
    int                    divisor     /* used by hash computation */
    )
    {
    return ((pHNode->pDbNode->grpDbId % divisor) & (elements - 1));
    }

