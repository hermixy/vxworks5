/* distGapLib.c - distributed group agreement protocol library */

/* Copyright 1999 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01i,24may99,drm  added vxfusion prefix to VxFusion related includes
01h,11feb99,drm  fixed endian bugs in GAP protocol - added htons() / ntohs()
01g,11sep98,drm  changed #include "distStatLib.h" to "private/distLibP.h"
01f,20may98,drm  removed local variable distNodeLclState which was no longer
                 being used.
01e,13may98,ur   review of some comments
01d,15apr98,ur   replaced misleading comments
01c,13mar98,ur   fixed crash detection
01b,07jan98,ur   fixed "group id already in use"
01a,15jul97,ur   written
*/

/*
DESCRIPTION
This library contains the "group agreement protocol" (GAP).
*/

#include "vxWorks.h"
#if defined (DIST_GAP_REPORT) || defined (DIST_DIAGNOSTIC)
#include "stdio.h"
#endif
#include "stdlib.h"
#include "string.h"
#include "sllLib.h"
#include "dllLib.h"
#include "msgQLib.h"
#include "taskLib.h"
#include "netinet/in.h"
#include "private/semLibP.h"
#include "vxfusion/msgQDistGrpLib.h"
#include "vxfusion/distIfLib.h"
#include "vxfusion/private/distLibP.h"
#include "vxfusion/private/distNameLibP.h"
#include "vxfusion/private/msgQDistGrpLibP.h"
#include "vxfusion/private/distNetLibP.h"
#include "vxfusion/private/distNodeLibP.h"

LOCAL DL_LIST			gapInProgress;
LOCAL SEMAPHORE			distGapSemaphore;

/* local prototypes */

LOCAL void			distGapCatchCrashed (DIST_NODE_ID nodeId);
LOCAL void			distGapPh1Done (DIST_GRP_DB_NODE *pDistGrp);
LOCAL STATUS		distGapTry (DIST_NODE_ID distNodeId,
						DIST_GRP_DB_NODE *pDbNode);
LOCAL BOOL			distGapOutstandConc (DIST_NODE_DB_NODE *pDistNodeDbNode,
						SL_LIST *pOutstand);
LOCAL BOOL			distGapOutstandOk (DIST_GAP_NODE *pDistGapNode,
						DIST_NODE_ID distNodeIdOk);
LOCAL void			distGapOutstandDel (DIST_GAP_NODE *pDistGapNode,
						DIST_GAP_RESPONSE *pDistGapResponse,
						DIST_GAP_RESPONSE *pDistGapResponsePrev);
LOCAL DIST_STATUS	distGapInput (DIST_NODE_ID nodeIdSrc,
						DIST_TBUF_HDR *pTBufHdr);
#ifdef DIST_GAP_TASK
LOCAL void			distGapTimerTask (void);
LOCAL void			distGapTimer (void);
#endif

/*******************************************************************************
*
* distGapLibInit - 
*
* NOMANUAL
*/

STATUS distGapLibInit()
	{
#ifdef DIST_GAP_TASK
	int tid;
#endif

	dllInit (&gapInProgress);

#ifdef DIST_GAP_TASK
	tid = taskSpawn ("tDistGAP",
			DIST_GAP_MGR_PRIO,
			VX_SUPERVISOR_MODE | VX_UNBREAKABLE,
			DIST_GAP_MGR_STACK_SZ,
			(FUNCPTR) distGapTimerTask,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
			
	if (tid == ERROR)
		return (ERROR);
#endif

	distCtl (DIST_CTL_CRASHED_HOOK, (int) distGapCatchCrashed);

	/*
	 * Add GAP input function to table of services.
	 */
	return (distNetServAdd (DIST_PKT_TYPE_GAP, distGapInput,
			DIST_GAP_SERV_NAME, DIST_GAP_SERV_NET_PRIO,
			DIST_GAP_SERV_TASK_PRIO, DIST_GAP_SERV_TASK_STACK_SZ));
	}

/*******************************************************************************
*
* distGapCatchCrashed - get notified by a node crash
*
* NOMANUAL
*/

LOCAL void distGapCatchCrashed
	(
	DIST_NODE_ID crashedId
	)
	{
	DIST_GRP_DB_NODE	*pGrpDone;
	DIST_GAP_NODE		*pGapNode;
	BOOL				more;

#ifdef DIST_GAP_REPORT
	printf ("distGapCatchCrashed: node %ld crashed\n", crashedId);
#endif
	pGapNode = (DIST_GAP_NODE *) DLL_FIRST (&gapInProgress);

	while (pGapNode != NULL)
		{
		msgQDistGrpDbLock();

		pGrpDone = pGapNode->pGapGrp;

		distGapLock();
		more = distGapOutstandOk (pGapNode, crashedId);
		distGapUnlock();

		pGapNode = (DIST_GAP_NODE *) DLL_NEXT (pGapNode);

		if (more == FALSE)	/* this was the last node we waited for */
			{
			msgQDistGrpLclSetState (pGrpDone, DIST_GRP_STATE_GLOBAL);
			msgQDistGrpLclSetCreator (pGrpDone, distNodeLocalGetId());

			msgQDistGrpDbUnlock();

			distGapPh1Done (pGrpDone);
			}
		else
			msgQDistGrpDbUnlock();
		}
	}

/*******************************************************************************
*
* distGapOutstandLink - link all operational nodes to GAP structure
*
* NOMANUAL
*/

void distGapOutstandLink
	(
	DIST_GAP_NODE	*distGapNode
	)
	{
	sllInit (&distGapNode->gapOutstand);
	distNodeEach (distGapOutstandConc, (int) &distGapNode->gapOutstand);
	}

/*******************************************************************************
*
* distGapOutstandConc - called by distGapOutstandLink()
*
* NOMANUAL
*/

LOCAL BOOL distGapOutstandConc
	(
	DIST_NODE_DB_NODE	*pDistNodeDbNode,
	SL_LIST				*pOutstand
	)
	{
	DIST_GAP_RESPONSE	*pDistGapResp;

	if (pDistNodeDbNode->nodeState == DIST_NODE_STATE_OPERATIONAL &&
		pDistNodeDbNode->nodeId != DIST_IF_BROADCAST_ADDR)
		{
		pDistGapResp =
				(DIST_GAP_RESPONSE *) malloc (sizeof (DIST_GAP_RESPONSE));
		if (pDistGapResp == NULL)
			return (ERROR);

		pDistGapResp->pGapResponseNode = pDistNodeDbNode;

		sllPutAtHead (pOutstand, (SL_NODE *) pDistGapResp);
		}

	return (TRUE);	/* continue */
	}

/*******************************************************************************
*
* distGapOutstandUnlink - Free all nodes linked to the GAP structure
*
* NOMANUAL
*/

void distGapOutstandUnlink
	(
	DIST_GAP_NODE	*distGapNode
	)
	{
	DIST_GAP_RESPONSE	*pDistGapResponse;
	DIST_GAP_RESPONSE	*pDistGapResponsePrev = NULL;

	if (distGapNode == NULL)
		return;

	pDistGapResponse = (DIST_GAP_RESPONSE *)
			SLL_FIRST (&distGapNode->gapOutstand);

	while (pDistGapResponse != NULL)
		{
		sllRemove (&distGapNode->gapOutstand, (SL_NODE *) pDistGapResponse,
				(SL_NODE *) pDistGapResponsePrev);
		pDistGapResponsePrev = pDistGapResponse;
		pDistGapResponse = (DIST_GAP_RESPONSE *) SLL_NEXT (pDistGapResponse);
		free (pDistGapResponsePrev);
		}

	}

/*******************************************************************************
*
* distGapOutstandOk - Delete a single node from the GAP structure's node list
*
* RETURNS: TRUE if there are more outstanding responses, or FALSE if the
* job is done.
*
* NOMANUAL
*/

LOCAL BOOL distGapOutstandOk
	(
	DIST_GAP_NODE	*pDistGapNode,
	DIST_NODE_ID	distNodeIdOk
	)
	{
	DIST_GAP_RESPONSE	*pDistGapResp;
	DIST_GAP_RESPONSE	*pDistGapRespPrev;

	pDistGapResp = (DIST_GAP_RESPONSE *)
			SLL_FIRST (&pDistGapNode->gapOutstand);
	pDistGapRespPrev = NULL;

	while (pDistGapResp != NULL &&
		   pDistGapResp->pGapResponseNode->nodeId != distNodeIdOk)
		{
		pDistGapRespPrev = pDistGapResp;
		pDistGapResp = (DIST_GAP_RESPONSE *) SLL_NEXT (pDistGapResp);
		}

	if (pDistGapResp != NULL)
		{
		distGapOutstandDel (pDistGapNode, pDistGapResp, pDistGapRespPrev);
		if (SLL_FIRST (&pDistGapNode->gapOutstand) == NULL)
			return (FALSE);
		}

	return (TRUE);
	}

/*******************************************************************************
*
* distGapOutstandDel - Delete a single node 
*
* NOMANUAL
*/

LOCAL void distGapOutstandDel
	(
	DIST_GAP_NODE		*pDistGapNode,
	DIST_GAP_RESPONSE	*pDistGapResponse,
	DIST_GAP_RESPONSE	*pDistGapResponsePrev
	)
	{
	sllRemove ((SL_LIST *) &pDistGapNode->gapOutstand,
			(SL_NODE *) pDistGapResponse, (SL_NODE *) pDistGapResponsePrev);

	free (pDistGapResponse);

#ifdef DIST_GAP_REPORT
	{
	SL_NODE	*pNode = SLL_FIRST (&pDistGapNode->gapOutstand);
	int		i;

	for (i=0; pNode; i++)
		pNode = SLL_NEXT (pNode);
	printf ("distGapOutstandDel: %d nodes outstanding\n", i);
	}
#endif
	}

/*******************************************************************************
*
* distGapPh1Done - phase 1 is done
*
* NOMANUAL
*/

LOCAL void distGapPh1Done
	(
	DIST_GRP_DB_NODE	*pDistGrp
	)
	{
	/*
	 * List of outstanding responses is empty now. We are done.
	 * Send SET telegram and wakeup sleeping task.
	 */

	DIST_PKT_GAP_SET	distPktGapSet;

#ifdef DIST_GAP_REPORT
	printf ("distGapInput/OK: GAP phase 1 done.\n");
#endif
	/*
	 * Build SET telegram and broadcast it.
	 */
	distPktGapSet.setHdr.pktType = DIST_PKT_TYPE_GAP;
	distPktGapSet.setHdr.pktSubType = DIST_PKT_TYPE_GAP_SET;
	distPktGapSet.setId = htons (pDistGrp->grpDbId);
	strcpy ((char *) &distPktGapSet.setName,
			(char *) &pDistGrp->grpDbName);

#ifdef DIST_GAP_REPORT
	printf ("distGapInput/OK: GAP phase 2 -- broadcast SET `%s'/%d.\n",
			distPktGapSet.setName, distPktGapSet.setId);
#endif
	distNetSend (DIST_IF_BROADCAST_ADDR, (DIST_PKT *) &distPktGapSet,
			sizeof (distPktGapSet), WAIT_FOREVER, DIST_GAP_PRIO);

	/*
	 * Wakeup initiator.
	 */
	semGive (&pDistGrp->pGrpDbGapNode->gapWaitFor);
	}

/*******************************************************************************
*
* distGapNodeInit - Initialize a GAP node
*
* NOMANUAL
*/

void distGapNodeInit
	(
	DIST_GAP_NODE		*pDistGapNode,
	DIST_GRP_DB_NODE	*pDistGrpDbNode,
	BOOL				link
	)
	{
	if (link)
		distGapOutstandLink(pDistGapNode);

	semBInit (&(pDistGapNode->gapWaitFor), SEM_Q_FIFO, SEM_EMPTY);
	pDistGapNode->pGapGrp		= pDistGrpDbNode;
	pDistGapNode->gapTimeout	= DIST_GAP_TRY_TIMO;
	pDistGapNode->gapRetries	= 1;

	distGapLock();
	dllInsert (&gapInProgress, NULL, &(pDistGapNode->gapLink));
	distGapUnlock();
	}

/*******************************************************************************
*
* distGapNodeDelete - Delete a GAP node
*
* NOMANUAL
*/

void distGapNodeDelete
	(
	DIST_GAP_NODE	*pDistGapNode
	)
	{
	distGapLock();
	dllRemove (&gapInProgress, (DL_NODE *) &(pDistGapNode->gapLink));
	distGapUnlock();
	}

/*******************************************************************************
*
* distGapStart - Start GAP (group agreement protocol)
*
* Do a TRY in order to create a new group.
*
* NOMANUAL
*/

DIST_MSG_Q_GRP_ID distGapStart
	(
	DIST_GRP_DB_NODE	*pDistGrpDbNode
	)
	{
	DIST_GAP_NODE		distGapNode;
	DIST_MSG_Q_GRP_ID	uniqGrpId;

	if (distNodeGetNumNodes(DIST_NODE_NUM_NODES_ALIVE) == 1)
		{
		/* we are alone */
		msgQDistGrpLclSetState (pDistGrpDbNode, DIST_GRP_STATE_GLOBAL);
		msgQDistGrpLclSetCreator (pDistGrpDbNode, distNodeLocalGetId());
		return (pDistGrpDbNode->grpDbId);
		}

	/*
	 * Init GAP Node and link it to the group database node and
	 * the list of active GAPs.
	 * GAP initialization includes the linking of all nodes,
	 * currently known in the system. When a response comes in,
	 * the linked list is updated (the node is removed from the
	 * list).
	 */

	distGapNodeInit(&distGapNode, pDistGrpDbNode, TRUE);
	pDistGrpDbNode->pGrpDbGapNode = &distGapNode;

	/*
	 * Send TRY.
	 */

	distGapTry (DIST_IF_BROADCAST_ADDR, pDistGrpDbNode);

	/*
	 * Wait on GAP Node.
	 */

	semTake (&distGapNode.gapWaitFor, WAIT_FOREVER);

	/*
	 * Cleanup
	 */

	if (pDistGrpDbNode->pGrpDbGapNode)
		{
		DIST_GAP_NODE	*pGapNode = pDistGrpDbNode->pGrpDbGapNode;

		pDistGrpDbNode->pGrpDbGapNode = NULL;
		distGapNodeDelete (pGapNode);
		}

#ifdef DIST_DIAGNOSTIC
	if (pDistGrpDbNode->grpDbState != DIST_GRP_STATE_GLOBAL)
		distLog ("distGapStart: group has no global state\n");
#endif

	/*
	 * Ask for group id.
	 */

	msgQDistGrpDbLock();
	uniqGrpId = msgQDistGrpLclGetId (pDistGrpDbNode);
	msgQDistGrpDbUnlock();

	return (uniqGrpId);
	}


/*******************************************************************************
*
* distGapTry - Send a TRY, for a specified group
*
* NOMANUAL
*/

LOCAL STATUS distGapTry
	(
	DIST_NODE_ID		distNodeId,		/* node address */
	DIST_GRP_DB_NODE	*pDistGrpDbNode	/* group to try */
	)
	{
	DIST_PKT_GAP_TRY	distPktGapTry;
	STATUS				status;

	/*
	 * Build TRY telegram and send it.
	 */

	distPktGapTry.tryHdr.pktType = DIST_PKT_TYPE_GAP;
	distPktGapTry.tryHdr.pktSubType = DIST_PKT_TYPE_GAP_TRY;
	distPktGapTry.tryId = htons (pDistGrpDbNode->grpDbId);  /* uint16 */
	strcpy ((char *) &distPktGapTry.tryName,
			(char *) &pDistGrpDbNode->grpDbName);

#ifdef DIST_GAP_REPORT
	printf ("distGapTry: send a TRY for `%s'/%d to node %ld\n",
			(char *) &pDistGrpDbNode->grpDbName,
			pDistGrpDbNode->grpDbId, distNodeId);
#endif
	status = distNetSend (distNodeId, (DIST_PKT *) &distPktGapTry,
			sizeof (distPktGapTry), WAIT_FOREVER, DIST_GAP_PRIO);

	return (status);
	}

/*******************************************************************************
*
* distGapInput - GAP incoming message dispatcher
*
* NOMANUAL
*/

LOCAL DIST_STATUS distGapInput
	(
	DIST_NODE_ID	distNodeIdSrc,
	DIST_TBUF_HDR	*pTBufHdr
	)
	{
	char		distPktGap[DIST_PKT_GAP_MAX_LEN];

	distTBufCopy (DIST_TBUF_GET_NEXT (pTBufHdr), 0, (char *) &distPktGap,
			DIST_PKT_GAP_MAX_LEN);

	switch (((DIST_PKT *) &distPktGap)->pktSubType)
	{

	/*
	 * We have received a TRY. Look if there are conflicts. If not,
	 * respond with OK, else compare the node ids (buddy algorithm)
	 * and send either OK or REJECT.
	 */

	case DIST_PKT_TYPE_GAP_TRY:
		{
		DIST_PKT_GAP_TRY	*pDistPktGapTry = (DIST_PKT_GAP_TRY *) &distPktGap;
		DIST_GRP_DB_NODE	*pGrpFoundById;
		DIST_GRP_DB_NODE	*pGrpFoundByName;
		DIST_GRP_STATE		grpState;

		/* Convert tryId (uint16) to host order */
		pDistPktGapTry->tryId = ntohs (pDistPktGapTry->tryId);

		/*
		 * What can we read out form the TRY-id?
		 *
		 * If we receive a TRY for an id that is greater the one
		 * we would try, we have missed a TRY.
		 * This can happen, if some node sent a TRY, which is correctly
		 * received by some other node, but not by us--retransmission
		 * is enforced, but takes some time. The other node immediately
		 * starts sendind a TRY of its own, which in turn is received by us
		 * correctly. As you can see, it seams for us that we have
		 * missed a TRY--in fact the missing TRY is just delayed.
		 *
		 * If we receive a TRY for an id that is lower the one
		 * we would try, it is *NOT* for sure, that the remote
		 * node has missed a TRY. This can happen if an outgoing
		 * TRY is delayed at the remote node.
		 */

#ifdef DIST_GAP_REPORT
		printf ("distGapInput: received TRY for `%s'/%d from node %ld\n",
				pDistPktGapTry->tryName, pDistPktGapTry->tryId, distNodeIdSrc);
#endif
		msgQDistGrpDbLock();

		/*
		 * Test for conflicts. Try to find id and name in local database.
		 * If found, ask for GLOBAL state.
		 */

		pGrpFoundById = msgQDistGrpLclFindById (pDistPktGapTry->tryId);
		if (pGrpFoundById)
			{
			grpState = msgQDistGrpLclGetState (pGrpFoundById);
#ifdef DIST_GAP_REPORT
			printf ("distGapInput/TRY: group id %d already in %s use\n",
					pDistPktGapTry->tryId,
					(grpState == DIST_GRP_STATE_GLOBAL) ? "global" : "local");
#endif
			if (grpState == DIST_GRP_STATE_GLOBAL)
				{
				/*
				 * Someone sends a TRY for an id, already confirmed
				 * by GAP. Give him a reject.
				 */

				DIST_PKT_GAP_REJECT	*pDistPktGapReject;

				msgQDistGrpDbUnlock();

#ifdef DIST_GAP_REPORT
				printf ("distGapInput:TRY: group id already in use; reject\n");
#endif
				pDistPktGapReject = (DIST_PKT_GAP_REJECT *) &distPktGap;
				pDistPktGapReject->rejectHdr.pktSubType =
						DIST_PKT_TYPE_GAP_REJECT;
				pDistPktGapReject->rejectId = htons (pDistPktGapReject->rejectId);
				distNetSend(distNodeIdSrc, (DIST_PKT *) pDistPktGapReject,
						sizeof (*pDistPktGapReject), WAIT_FOREVER,
						DIST_GAP_PRIO);

				break;
				}
			}

		pGrpFoundByName = msgQDistGrpLclFindByName (pDistPktGapTry->tryName);
		if (pGrpFoundByName)
			{
			grpState = msgQDistGrpLclGetState (pGrpFoundByName);
#ifdef DIST_GAP_REPORT
			printf ("distGapInput/TRY: group name `%s' already in %s use\n",
					pDistPktGapTry->tryName,
					(grpState == DIST_GRP_STATE_GLOBAL) ? "global" : "local");
#endif
			if (grpState == DIST_GRP_STATE_GLOBAL)
				{
				DIST_PKT_GAP_SET	*pDistPktGapSet;

				/*
				 * Someone sends a TRY for a group that is in GLOBAL
				 * state here.
				 * This can happen, if
				 * 1) someone did a TRY before receiving our
				 *    SET (remote TRYs do not force creation of a
				 *    database entry).
				 * 2) a WAIT timed out and the state shifts to
				 *    WAIT_TRY. In this case the remote node has
				 *    missed the SET. Directly respond with SET.
				 */

				/*
				 * Respond with SET.
				 */

				pDistPktGapSet = (DIST_PKT_GAP_SET *) &distPktGap;

				pDistPktGapSet->setHdr.pktSubType = DIST_PKT_TYPE_GAP_SET;
				pDistPktGapSet->setId = msgQDistGrpLclGetId (pGrpFoundByName);

				msgQDistGrpDbUnlock();

#ifdef DIST_GAP_REPORT
				printf ("distGapInput/TRY: respond with SET `%s'/%d\n",
						pDistPktGapSet->setName, pDistPktGapSet->setId);
#endif
				/* Convert setId (uint16) to network byte order */
				pDistPktGapSet->setId = htons (pDistPktGapSet->setId);

				distNetSend(distNodeIdSrc, (DIST_PKT *) pDistPktGapSet,
						sizeof (*pDistPktGapSet), WAIT_FOREVER,
						DIST_GAP_PRIO);

				break;

				}
			}
		else
			{
			DIST_GRP_DB_NODE	*pDistGrp;

			/*
			 * Create DB entry for new group.
			 */
			pDistGrp = msgQDistGrpLclCreate (pDistPktGapTry->tryName,
					pDistPktGapTry->tryId, DIST_GRP_STATE_REMOTE_TRY);
			if (pDistGrp == NULL)
				{
				/*
				 * We cannot create a new node in the group database.
				 * This is a fatal error.
				 */
				distPanic ("distGapInput:TRY: creation of new node failed\n");
				}

			/* only temporary, creator may change */
			msgQDistGrpLclSetCreator (pDistGrp, distNodeIdSrc);
			pDistGrp->pGrpDbGapNode = NULL;
			}

		/*
		 * If there is a local group with the same name as currently
		 * tried, it is not in GLOBAL state. We have checked this
		 * before.
		 */

		if (pGrpFoundById == NULL && pGrpFoundByName == NULL)
			{
			DIST_PKT_GAP_OK	*pDistPktGapOk;

			/*
			 * No conflicts. Neither group id nor group name
			 * are used in the local database.
			 */

			distGrpIdNext++;

			msgQDistGrpDbUnlock();

			/*
			 * Respond with OK.
			 */

#ifdef DIST_GAP_REPORT
			printf ("distGapInput/TRY: neither id nor name used; respond OK\n");
#endif
			pDistPktGapOk = (DIST_PKT_GAP_OK *) &distPktGap;

			pDistPktGapOk->okHdr.pktSubType = DIST_PKT_TYPE_GAP_OK;

			/* Convert okId (uint16) to network byte order */
			pDistPktGapOk->okId = htons (pDistPktGapOk->okId);

			distNetSend(distNodeIdSrc, (DIST_PKT *) pDistPktGapOk,
					sizeof (*pDistPktGapOk), WAIT_FOREVER, DIST_GAP_PRIO);

			break;
			}

		if (pGrpFoundById == pGrpFoundByName)
			{
			/*
			 * No conflict. There is already an object that
			 * exactly matches the remote one.
			 */

			DIST_PKT_GAP_OK	*pDistPktGapOk = (DIST_PKT_GAP_OK *) &distPktGap;

			msgQDistGrpDbUnlock();

			/*
			 * Respond with OK.
			 */

#ifdef DIST_GAP_REPORT
			printf ("distGapInput/TRY: id and name match; respond OK\n");
#endif
			pDistPktGapOk->okHdr.pktSubType = DIST_PKT_TYPE_GAP_OK;

			/* Convert okId (uint16) to network byte order */
            pDistPktGapOk->okId = htons (pDistPktGapOk->okId);

			distNetSend(distNodeIdSrc, (DIST_PKT *) pDistPktGapOk,
					sizeof (*pDistPktGapOk), WAIT_FOREVER, DIST_GAP_PRIO);

			break;
			}

		/*
		 * There is a conflict. Compare node ids to solve the conflict.
		 * This is kind of a game: the one with the higher node id wins.
		 */

		if (distNodeLocalGetId() < distNodeIdSrc)
			{
			DIST_PKT_GAP_OK *pDistPktGapOk;

			/*
			 * We have lost. Do a local cleanup and send OK.
			 */

			distGrpIdNext++;

#ifdef DIST_GAP_REPORT
			printf ("distGapInput/TRY: we have lost, trying %d\n",
					pDistPktGapTry->tryId);
#endif
			if (pGrpFoundByName)
				{

				/*
				 * Someone else is trying to create the same group, but has
				 * a different group id. We have lost, so let us wait until
				 * we receive a SET.
				 */

				/*
				 * Remove linked list of outstanding responses.
				 */

				distGapOutstandUnlink (pGrpFoundByName->pGrpDbGapNode);

				/*
				 * Set WAIT state.
				 */

				msgQDistGrpLclSetState (pGrpFoundByName, DIST_GRP_STATE_WAIT);
				pGrpFoundByName->pGrpDbGapNode->gapTimeout = DIST_GAP_WAIT_TIMO;
				msgQDistGrpDbUnlock ();

				}
			else
				{

				/*
				 * Someone else is trying to create a group with an
				 * id, that was also proposed by us. We have lost,
				 * so forget about the id and try another one.
				 */
				
				/*
				 * Cleanup and reinitialize the GAP structure.
				 */

				distGapOutstandUnlink (pGrpFoundById->pGrpDbGapNode);
				distGapOutstandLink (pGrpFoundById->pGrpDbGapNode);
				pGrpFoundById->pGrpDbGapNode->gapTimeout = DIST_GAP_TRY_TIMO;
				pGrpFoundById->pGrpDbGapNode->gapRetries = 1;

				/*
				 * Get a new id and reset state.
				 */

				msgQDistGrpLclSetId (pGrpFoundByName, distGrpIdNext++);
				msgQDistGrpLclSetState (pGrpFoundByName,
						DIST_GRP_STATE_LOCAL_TRY);

				msgQDistGrpDbUnlock();

				/*
				 * Have a different TRY.
				 */

				distGapTry (DIST_IF_BROADCAST_ADDR, pGrpFoundByName);

				}

#ifdef DIST_GAP_REPORT
			printf ("distGapInput/TRY: respond with OK\n");
#endif
			pDistPktGapOk = (DIST_PKT_GAP_OK *) &distPktGap;
			pDistPktGapOk->okHdr.pktSubType = DIST_PKT_TYPE_GAP_OK;

			/* Convert okId (uint16) to network byte order */
            pDistPktGapOk->okId = htons (pDistPktGapOk->okId);

			distNetSend(distNodeIdSrc, (DIST_PKT *) pDistPktGapOk,
					sizeof (*pDistPktGapOk), WAIT_FOREVER, DIST_GAP_PRIO);

			}
		else
			{
			/*
			 * We won.
			 */

#ifdef DIST_GAP_REPORT
			printf ("distGapInput/TRY: we have won, trying %d\n",
					pDistPktGapTry->tryId);
#endif
			msgQDistGrpDbUnlock();

			if (pGrpFoundByName)
				{
				/*
				 * Someone is trying to create the same group, we
				 * are trying, with a different group id. We won
				 * the game, ask him to wait (ASK_WAIT).
				 */

				DIST_PKT_GAP_ASK_WAIT	*pDistPktGapAskWait;

#ifdef DIST_GAP_REPORT
				printf ("distGapInput/TRY: respond with ASK_WAIT\n");
#endif
				pDistPktGapAskWait = (DIST_PKT_GAP_ASK_WAIT *) &distPktGap;
				pDistPktGapAskWait->askWaitHdr.pktSubType =
						DIST_PKT_TYPE_GAP_ASK_WAIT;

				/* Convert askWaitId (uint16) to network byte order */
            	pDistPktGapAskWait->askWaitId = htons (pDistPktGapAskWait->askWaitId);

				distNetSend(distNodeIdSrc, (DIST_PKT *) pDistPktGapAskWait,
						sizeof (*pDistPktGapAskWait), WAIT_FOREVER,
						DIST_GAP_PRIO);
				}
			else
				{
				/*
				 * Someone is trying to create a group with an
				 * id, already in use by us. We won the game, so
				 * give him a REJECT.
				 */

				DIST_PKT_GAP_REJECT	*pDistPktGapReject;

#ifdef DIST_GAP_REPORT
				printf ("distGapInput/TRY: respond with REJECT\n");
#endif
				pDistPktGapReject = (DIST_PKT_GAP_REJECT *) &distPktGap;
				pDistPktGapReject->rejectHdr.pktSubType =
						DIST_PKT_TYPE_GAP_REJECT;

				/* Convert rejectId (uint16) to network byte order */
                pDistPktGapReject->rejectId = htons (pDistPktGapReject->rejectId);

				distNetSend(distNodeIdSrc, (DIST_PKT *) pDistPktGapReject,
						sizeof (*pDistPktGapReject), WAIT_FOREVER,
						DIST_GAP_PRIO);
				}

			}

		break;
		}

	/*
	 * We have received an OK in response to an earlier TRY.
	 */

	case DIST_PKT_TYPE_GAP_OK:
		{
		DIST_PKT_GAP_OK		*pDistPktGapOk = (DIST_PKT_GAP_OK *) &distPktGap;
		DIST_GRP_DB_NODE	*pDistGrp;
		BOOL				more;

		/* Convert okId (uint16) to host order */
		pDistPktGapOk->okId = ntohs (pDistPktGapOk->okId);
		
#ifdef DIST_GAP_REPORT
		printf ("distGapInput: received OK for `%s'/%d from node %ld\n",
				pDistPktGapOk->okName, pDistPktGapOk->okId, distNodeIdSrc);
#endif
		msgQDistGrpDbLock();

		/*
		 * Try to find the confirmed id.
		 */

		pDistGrp = msgQDistGrpLclFindById (pDistPktGapOk->okId);
		if (pDistGrp == NULL ||
			msgQDistGrpLclGetState (pDistGrp) == DIST_GRP_STATE_GLOBAL)
			{
			/*
			 * This is an OK for an id, that
			 * 1) does not exist anymore (due to earlier REJECTs):
			 *    This means, someone sent us a REJECT. As a
			 *    consequence of this, we destoyed the local entry
			 *    for this group and tried again with a new id.
			 * 2) was confirmed by GAP earlier (group is in GLOBAL state).
			 *
			 * All responses to the old group id go here and will be ignored.
			 */

			msgQDistGrpDbUnlock();
			break;
			}

		distGapLock();

		/*
		 * Update list of outstanding responses.
		 */
		more = distGapOutstandOk(pDistGrp->pGrpDbGapNode, distNodeIdSrc);

		distGapUnlock();

		if (more == FALSE)
			{
			msgQDistGrpLclSetState (pDistGrp, DIST_GRP_STATE_GLOBAL);
			msgQDistGrpLclSetCreator (pDistGrp, distNodeLocalGetId());

			msgQDistGrpDbUnlock();

			distGapPh1Done (pDistGrp);
			break;
			}

		msgQDistGrpDbUnlock();
		break;
		}

	/*
	 * We have received a REJECT.
	 * Someone else is trying to create another group with an
	 * id, that was also proposed by us. It seams, we have
	 * lost a remote comparison, so forget about the old group
	 * id and try a new one.
	 */

	case DIST_PKT_TYPE_GAP_REJECT:
		{
		DIST_PKT_GAP_REJECT *pDistPktGapReject;
		DIST_GRP_DB_NODE	*pDistGrp;

		pDistPktGapReject = (DIST_PKT_GAP_REJECT *) &distPktGap;

		/* Convert rejectId (uint16) to host order */
		pDistPktGapReject->rejectId = ntohs (pDistPktGapReject->rejectId);
		

#ifdef DIST_GAP_REPORT
		printf ("distGapInput: received REJECT for `%s'/%d from node %ld\n",
				pDistPktGapReject->rejectName, pDistPktGapReject->rejectId,
				distNodeIdSrc);
#endif
		msgQDistGrpDbLock();

		pDistGrp = msgQDistGrpLclFindById (pDistPktGapReject->rejectId);
		if (pDistGrp == NULL ||
			msgQDistGrpLclGetState (pDistGrp) == DIST_GRP_STATE_GLOBAL)
			{
			/*
			 * This is a REJECT for an id, that
			 * 1) does not exist anymore (due to earlier REJECTs):
			 *    This means, someone sent us a REJECT. As a
			 *    consequence of this, we destoyed the local entry
			 *    for this group and tried again with a new id.
			 * 2) was confirmed by GAP earlier (group is in GLOBAL state).
			 *
			 * All responses to the old group id go here and will be ignored.
			 */

			msgQDistGrpDbUnlock();
			break;
			}

		/*
		 * Cleanup and reinitialize the GAP structure.
		 */

		distGapOutstandUnlink (pDistGrp->pGrpDbGapNode);
		distGapOutstandLink (pDistGrp->pGrpDbGapNode);
		pDistGrp->pGrpDbGapNode->gapTimeout = DIST_GAP_TRY_TIMO;
		pDistGrp->pGrpDbGapNode->gapRetries = 1;

		/*
		 * Get a new id and reset state.
		 */

		msgQDistGrpLclSetId (pDistGrp, distGrpIdNext++);
		msgQDistGrpLclSetState (pDistGrp, DIST_GRP_STATE_LOCAL_TRY);

		msgQDistGrpDbUnlock();

		/*
		 * Have a different TRY.
		 */

		distGapTry (DIST_IF_BROADCAST_ADDR, pDistGrp);

		break;
		}

	/*
	 * We received an ASK_WAIT. We have send a TRY before. Someone else
	 * found a conflict, where it is trying to create the same group, but
	 * with an alternate id. Since the other node has a higher node id
	 * it asks us to wait for a SET.
	 */

	case DIST_PKT_TYPE_GAP_ASK_WAIT:
		{
		DIST_PKT_GAP_ASK_WAIT	*pDistPktGapAskWait;
		DIST_GRP_DB_NODE		*pDistGrp;

		pDistPktGapAskWait = (DIST_PKT_GAP_ASK_WAIT *) &distPktGap;

		/* Convert askWaitId (uint16) to host byte order */
		pDistPktGapAskWait->askWaitId = ntohs (pDistPktGapAskWait->askWaitId);

#ifdef DIST_GAP_REPORT
		printf ("distGapInput: received ASK_WAIT for `%s'/%d from node %ld\n",
				pDistPktGapAskWait->askWaitName, pDistPktGapAskWait->askWaitId,
				distNodeIdSrc);
#endif
		msgQDistGrpDbLock();

		pDistGrp = msgQDistGrpLclFindByName (pDistPktGapAskWait->askWaitName);
		if (pDistGrp == NULL ||
			msgQDistGrpLclGetState (pDistGrp) == DIST_GRP_STATE_GLOBAL)
			{
			/*
			 * This is an ASK_WAIT for an id, that
			 * 1) does not exist anymore (due to earlier REJECTs):
			 *    This means, someone sent us a REJECT. As a
			 *    consequence of this, we destoyed the local entry
			 *    for this group and tried again with a new id.
			 * 2) was confirmed by GAP earlier (group is in GLOBAL state).
			 *
			 * All responses to the old group id go here and will be ignored.
			 */

			msgQDistGrpDbUnlock();
			break;
			}

		if (msgQDistGrpLclGetState (pDistGrp) != DIST_GRP_STATE_WAIT)
			{
			/*
			 * Remove the list of outstanding responses. Do *NOT*
			 * remove the GAP structure.
			 */

			distGapOutstandUnlink (pDistGrp->pGrpDbGapNode);

			msgQDistGrpLclSetState (pDistGrp, DIST_GRP_STATE_WAIT);
			pDistGrp->pGrpDbGapNode->gapTimeout = DIST_GAP_WAIT_TIMO;
			}

		msgQDistGrpDbUnlock();
		break;
		}

	/*
	 * We have received a SET. When receiving a SET, we already should
	 * have received a TRY and responded to it with OK.
	 */

	case DIST_PKT_TYPE_GAP_SET:
		{
		DIST_PKT_GAP_SET	*pDistPktGapSet;
		DIST_GRP_DB_NODE	*pGrpFoundById;
		DIST_GRP_DB_NODE	*pGrpFoundByName;
		DIST_GRP_DB_NODE	*pDistGrp;

		pDistPktGapSet = (DIST_PKT_GAP_SET *) &distPktGap;

		/* Convert setId (uint16) to host byte order */
		pDistPktGapSet->setId = ntohs (pDistPktGapSet->setId);

#ifdef DIST_GAP_REPORT
		printf ("distGapInput: received SET for `%s'/%d from node %ld\n",
				pDistPktGapSet->setName, pDistPktGapSet->setId, distNodeIdSrc);
#endif
		/*
		 * Sanity check: Is this a SET from future?
		 */

		if (pDistPktGapSet->setId >= distGrpIdNext)
			{
			/*
			 * There was no TRY for this SET! Just ignore this.
			 */
#ifdef DIST_DIAGNOSTIC
			distLog ("distGapInput/SET: no TRY for SET\n");
#endif
			break;
			}

		msgQDistGrpDbLock();

		pGrpFoundById = msgQDistGrpLclFindById (pDistPktGapSet->setId);
		pGrpFoundByName = msgQDistGrpLclFindByName (pDistPktGapSet->setName);

		if (pGrpFoundByName &&
			(!pGrpFoundById || pGrpFoundById == pGrpFoundByName) &&
			(msgQDistGrpLclGetState (pGrpFoundByName) != DIST_GRP_STATE_GLOBAL))
			{
			/*
			 * When do we get here?
			 * We were waiting for this one. An earlier TRY asked us to
			 * wait. Here is the SET we waited for.
			 */
#ifdef DIST_GAP_REPORT
			printf ("distGapInput/SET: got SET for existing nonglobal group");
#endif
			msgQDistGrpLclSetId (pGrpFoundByName, pDistPktGapSet->setId);
			msgQDistGrpLclSetState (pGrpFoundByName, DIST_GRP_STATE_GLOBAL);
			msgQDistGrpLclSetCreator (pGrpFoundByName, distNodeIdSrc);

			if (pGrpFoundByName->pGrpDbGapNode)
				{
				distGapOutstandUnlink (pGrpFoundByName->pGrpDbGapNode);
				semFlush (&pGrpFoundByName->pGrpDbGapNode->gapWaitFor);
				}

			msgQDistGrpDbUnlock();

			break;
			}

		if (pGrpFoundById != pGrpFoundByName)
			{
			/*
			 * Id or name are already in database but do not point
			 * to the same object. This can happen, if we receive
			 * multiple SETs.
			 */

			msgQDistGrpDbUnlock();

#ifdef DIST_GAP_REPORT
			printf ("distGapInput/SET: id or name already in database\n");
#endif
			break;
			}

		if (pGrpFoundById && pGrpFoundById == pGrpFoundByName)
			{
			/*
			 * An exact copy of this object is already in the database.
			 * Ignore state.
			 */
#ifdef DIST_GAP_REPORT
			printf ("distGapInput/SET: `%s' (0x%lx) is already in database\n",
				pDistPktGapSet->setName, (u_long) pDistPktGapSet->setId);
#endif
			msgQDistGrpLclSetState (pGrpFoundByName, DIST_GRP_STATE_GLOBAL);
			msgQDistGrpLclSetCreator (pGrpFoundByName, distNodeIdSrc);

			if (pGrpFoundByName->pGrpDbGapNode != NULL)
				semFlush (&pGrpFoundByName->pGrpDbGapNode->gapWaitFor);

			msgQDistGrpDbUnlock();
			break;
			}

		/*
		 * Neither id nor name are found in the database.
		 * Create node in group database.
		 */
#ifdef DIST_GAP_REPORT
		printf ("distGapInput/SET: creating local db entry `%s' (0x%lx)\n",
				pDistPktGapSet->setName, (u_long) pDistPktGapSet->setId);
#endif
		pDistGrp = msgQDistGrpLclCreate (pDistPktGapSet->setName,
				pDistPktGapSet->setId, DIST_GRP_STATE_GLOBAL);
		msgQDistGrpLclSetCreator (pDistGrp, distNodeLocalGetId ());

		if (pDistGrp == NULL)
			{
			/*
			 * We have a SET from remote and cannot create a new
			 * node in the group database. This is a fatal error.
			 */

			distPanic ("distGapInput:SET: creation of new node failed\n");
			}

		msgQDistGrpLclSetCreator (pDistGrp, distNodeIdSrc);
		msgQDistGrpDbUnlock();
		
		break;
		}

	/*
	 * This type of telegram is unknown.
	 */

	default:
		{
		/*
		 * When we get here, the packet is already acknowledged.
		 * But this makes no difference since an unknown type of
		 * telegram is a fatal error. Let's panic!
		 */
#ifdef DIST_DIAGNOSTIC
		distPanic ("distGapInput: unknown type of telegram\n");
#endif
		}
	}

	return (DIST_GAP_STATUS_OK);
	}

#ifdef DIST_GAP_TASK

/*******************************************************************************
*
* distGapTimerTask - Timeout management task
*
*
* NOMANUAL
*/

LOCAL void distGapTimerTask ()
	{
	while (1)
		{
		distGapTimer ();
		taskDelay (DIST_GAP_MGR_WAKEUP_TICKS);
		}
	}

/*******************************************************************************
*
* distGapTimer - Timeout management
*
* The job of the distGapTimer() is to go through the list of active
* GAPs to see if a GAP has timed out.
*
* NOMANUAL
*/

LOCAL void distGapTimer ()
	{
	DIST_GAP_NODE *pGapNode;

	distGapLock();

	/*
	 * Run through list of open GAPs.
	 */

	pGapNode = (DIST_GAP_NODE *) DLL_FIRST (&gapInProgress);

	while (pGapNode != NULL)
	{

	DIST_GAP_RESPONSE	*pDistGapResponse =
			(DIST_GAP_RESPONSE *) SLL_FIRST (&pGapNode->gapOutstand);

	switch (msgQDistGrpLclGetState (pGapNode->pGapGrp))
		{

		case DIST_GRP_STATE_LOCAL_TRY:	/* try initiated by us */
		case DIST_GRP_STATE_WAIT_TRY:	/* retry after wait */
		{

		/*
		 * This GAP is open and we are still trying.
		 */

		if (pGapNode->gapTimeout == 0)
			{

			/*
			 * GAP has timed out. Lookup the missing nodes.
			 */

			DIST_GAP_RESPONSE	*pDistGapResponsePrev;

			while (pDistGapResponse != NULL)
			{

			if (distNodeGetState (pDistGapResponse->pGapResponseNode) ==
					DIST_NODE_STATE_OPERATIONAL)
				{

				/*
				 * According to our local database the node is OPERATIONAL.
				 * Check the number of retries.
				 */

				DIST_NODE_ID	distNodeIdMissing =
						pDistGapResponse->pGapResponseNode->nodeId;

				if (pGapNode->gapRetries++ > DIST_GAP_MAX_RETRIES)
					{

					/*
					 * The number of retries is within the limit.
					 * Send a unicast TRY with a higher timeout to the
					 * missing node.
					 */

					pGapNode->gapTimeout =
							pGapNode->gapRetries * DIST_GAP_TRY_TIMO;

					distGapTry (distNodeIdMissing, pGapNode->pGapGrp);

					}
				else
					{

					/*
					 * We have exceeded the limit of max retries.
					 * The remote node seams to be dead. Tell the node
					 * database about our assumption.
					 */

					(void) distNodeCrashed (distNodeIdMissing);
					distGapOutstandDel (pGapNode, pDistGapResponse,
							pDistGapResponsePrev);

					}
				}
			else
				{

				/*
				 * Node has died, while we were waiting for a response.
				 * Dying is something like agreeing.
				 */

				distGapOutstandDel (pGapNode, pDistGapResponse,
						pDistGapResponsePrev);

				}

			pDistGapResponsePrev = pDistGapResponse;
			pDistGapResponse = (DIST_GAP_RESPONSE *)
					SLL_NEXT (pDistGapResponse);

			}	/* while (pDistGapResponse != NULL) */

			}

		pGapNode->gapTimeout--;

		break;
		}

		case DIST_GRP_STATE_WAIT:
		{
		/*
		 * Someone told us to wait.
		 */

		if (pGapNode->gapTimeout == 0)
			{
			/*
			 * GAP timed out and we are still waiting for a SET. We will
			 * return to try again, but we must not forget the WAITing
			 * state we were in, since the missing SET might come in at
			 * any time.
			 */

			DIST_GRP_DB_NODE	*pDistGrp;

			/*
			 * Reinitialize the GAP structure. Cleanup was done,
			 * before entering WAITing state.
			 */
	
			distGapOutstandLink (pGapNode);
			pDistGrp->pGrpDbGapNode->gapTimeout = DIST_GAP_TRY_TIMO;
			pDistGrp->pGrpDbGapNode->gapRetries = 1;

			/*
			 * Get a new id and reset state.
			 */

			pDistGrp = pGapNode->pGapGrp;
			
			msgQDistGrpLclSetId (pDistGrp, distGrpIdNext++);
			msgQDistGrpLclSetState (pDistGrp, DIST_GRP_STATE_WAIT_TRY);

			/*
			 * Have another TRY with a different id.
			 */

			distGapTry (DIST_IF_BROADCAST_ADDR, pDistGrp);
			}

		pGapNode->gapTimeout--;

		break;
		}

		default:
		{
#ifdef DIST_DIAGNOSTIC
			distLog ("distGapTimer: `%s' in unknown or unexpected state %d\n",
					&pGapNode->pGapGrp->grpDbName,
					pGapNode->pGapGrp->grpDbState);
#endif
		}

		}	/* switch (state) */

	pGapNode = (DIST_GAP_NODE *) DLL_NEXT (pGapNode);

	}	/* while (pGapNode != NULL) */

	distGapUnlock();

	}

#endif

