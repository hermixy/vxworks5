/* msgQDistGrpLibP.h - distributed msg Q group library private hdr (VxFusion) */

/* Copyright 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01b,24may99,drm  added vxfusion prefix to VxFusion related includes
01a,11jun97,ur   written.
*/

#ifndef __INCmsgQDistGrpLibPh
#define __INCmsgQDistGrpLibPh

#ifdef __cplusplus
extern "C" {
#endif

#include "vxWorks.h"
#include "sllLib.h"
#include "dllLib.h"
#include "hashLib.h"
#include "sysLib.h"
#include "vxfusion/distLib.h"
#include "vxfusion/private/msgQDistLibP.h"
#include "vxfusion/private/distPktLibP.h"
#include "vxfusion/private/distNetLibP.h"
#include "vxfusion/private/distNodeLibP.h"
#include "vxfusion/private/distNameLibP.h"

extern SEMAPHORE			distGrpDbSemaphore;
extern DIST_MSG_Q_GRP_ID	distGrpIdNext;

/*
 * defines
 */

#define DIST_DGDB_SERV_NAME					"tServDgdb"
#define DIST_DGDB_SERV_TASK_PRIO			50
#define DIST_DGDB_SERV_TASK_STACK_SZ		5000

#define DIST_MSG_Q_GRP_SERV_NAME			"tServGrp"
#define DIST_MSG_Q_GRP_SERV_TASK_PRIO		50
#define DIST_MSG_Q_GRP_SERV_TASK_STACK_SZ	5000

#define DIST_GAP_SERV_NAME					"tServGap"
#define DIST_GAP_SERV_TASK_PRIO				50
#define DIST_GAP_SERV_TASK_STACK_SZ			5000

#define DIST_DGDB_SERV_NET_PRIO				0
#define DIST_MSG_Q_GRP_SERV_NET_PRIO		0
#define DIST_GAP_SERV_NET_PRIO				0

#define DIST_GAP_MGR_WAKEUP_DSEC			10	/* wakeup manager once a sec */
#define DIST_GAP_MGR_PRIO					50
#define DIST_GAP_MGR_STACK_SZ				5000

#define DIST_MSG_Q_GRP_WAIT_TASK_PRIO		50
#define DIST_MSG_Q_GRP_WAIT_TASK_STACK_SZ	5000

/*
 * The following timeouts are in units of DIST_GAP_MGR_WAKEUP_DSEC.
 */
#define DIST_GAP_TRY_TIMO			2	/* 2 times DIST_GAP_MGR_WAKEUP_DSEC */
#define DIST_GAP_WAIT_TIMO			2	/* 2 times DIST_GAP_MGR_WAKEUP_DSEC */

#define DIST_GAP_MAX_RETRIES		1	/* a single retry */

#define DIST_MSG_Q_GRP_PRIO \
	(servTable[DIST_ID_MSG_Q_GRP_SERV].servNetPrio)

#define DIST_DGDB_PRIO \
	(servTable[DIST_ID_DGDB_SERV].servNetPrio)

#define DIST_GAP_PRIO \
	(servTable[DIST_ID_GAP_SERV].servNetPrio)

/* subtypes of DIST_PKT_TYPE_DGDB */

#define DIST_PKT_TYPE_DGDB_ADD			0	

/* subtypes of DIST_PKT_TYPE_MSG_Q_GRP */

#define DIST_PKT_TYPE_MSG_Q_GRP_SEND	0
#define DIST_PKT_TYPE_MSG_Q_GRP_STATUS	1

/* subtypes of DIST_PKT_TYPE_GAP */

#define DIST_PKT_TYPE_GAP_OK			0
#define DIST_PKT_TYPE_GAP_REJECT		1
#define DIST_PKT_TYPE_GAP_TRY			2
#define DIST_PKT_TYPE_GAP_ASK_WAIT		3
#define DIST_PKT_TYPE_GAP_SET			4

/* status codes for distributed groups */
#define MSG_Q_DIST_GRP_STATUS_OK					OK
#define MSG_Q_DIST_GRP_STATUS_ERROR					1
#define MSG_Q_DIST_GRP_STATUS_NOT_ENOUGH_MEMORY		2
#define MSG_Q_DIST_GRP_STATUS_ILLEGAL_OBJ_ID		3
#define MSG_Q_DIST_GRP_STATUS_PROTOCOL_ERROR		4
#define MSG_Q_DIST_GRP_STATUS_INTERNAL_ERROR		5
#define MSG_Q_DIST_GRP_STATUS_UNAVAIL				6
#define MSG_Q_DIST_GRP_STATUS_LOCAL_TIMEOUT			7

/* status codes for DGDB */
#define DIST_GDB_STATUS_OK							OK
#define DIST_GDB_STATUS_PROTOCOL_ERROR				1

/* status codes for GAP */
#define DIST_GAP_STATUS_OK							OK

#define DIST_GRP_STATE_LOCAL_TRY	0	/* local node tries to create group */
#define DIST_GRP_STATE_REMOTE_TRY	1	/* remote node tries to create group */
#define DIST_GRP_STATE_WAIT			2	/* wait for id from remote node */
#define DIST_GRP_STATE_WAIT_TRY		3	/* retry after wait */
#define DIST_GRP_STATE_GLOBAL		5	/* group is global */

#define DIST_MSG_Q_GRP_INQ_TYPE_SEND	((0 << 8) | DIST_MSG_Q_GRP_INQ)

#define DIST_GAP_MGR_WAKEUP_TICKS \
	(DIST_GAP_MGR_WAKEUP_DSEC * (sysClkRateGet () / 10))

#define msgQDistGrpLclGetId(distGrpDbNode) \
		((distGrpDbNode)->grpDbId)
/* msgQDistGrpLclSetId() is a function */

#define msgQDistGrpLclGetState(distGrpDbNode) \
		((distGrpDbNode)->grpDbState)
#define msgQDistGrpLclSetState(distGrpDbNode, distGrpState) \
		((distGrpDbNode)->grpDbState = (distGrpState))

#define msgQDistGrpLclSetCreator(distGrpDbNode, distNodeId) \
		((distGrpDbNode)->grpDbNodeId = (distNodeId))

#define msgQDistGrpDbLockInit()	\
			semBInit (&distGrpDbSemaphore, SEM_Q_PRIORITY, SEM_FULL)
#define msgQDistGrpDbUnlock() \
			semGive (&distGrpDbSemaphore)
#define msgQDistGrpDbLock() \
			semTake (&distGrpDbSemaphore, WAIT_FOREVER)

#define distGapLockInit() \
			semBInit (&distGapSemaphore, SEM_Q_PRIORITY, SEM_FULL)
#define distGapUnlock() \
			semGive (&distGapSemaphore)
#define distGapLock() \
			semTake (&distGapSemaphore, WAIT_FOREVER)

#define msgQGrpSendInqLockInit(pInqId) \
			semBInit (&((pInqId)->sendInqLock), SEM_Q_PRIORITY, SEM_FULL)
#define msgQGrpSendInqUnlock(pInqId) \
			semGive (&((pInqId)->sendInqLock))
#define msgQGrpSendInqLock(pInqId) \
			semTake (&((pInqId)->sendInqLock), WAIT_FOREVER)

#define DIST_PKT_GAP_MAX_LEN	(sizeof (DIST_PKT_GAP_OK))

/*
 * typedefs
 */

/*
 * Forward structure declarations [sic].
 */

#ifdef __STDC__
struct _DIST_GRP_DB_NODE;
#endif

typedef struct						/* DIST_GAP_NODE */
	{
	DL_NODE						gapLink;
	struct _DIST_GRP_DB_NODE	*pGapGrp;
	SL_LIST						gapOutstand;
	short						gapTimeout;
	short						gapRetries;
	SEMAPHORE					gapWaitFor;
	} DIST_GAP_NODE;

typedef struct						/* DIST_GAP_RESPONSE */
	{
	SL_NODE				gapResponseNext;
	DIST_NODE_DB_NODE	*pGapResponseNode;
	} DIST_GAP_RESPONSE;

/* MSG_Q_GRP packets */

typedef struct						/* DIST_PKT_MSG_Q_GRP_SEND */
	{
	DIST_PKT			pktMsgQGrpSendHdr;
	uint32_t			pktMsgQGrpSendInqId;
	uint32_t			pktMsgQGrpSendTimeout;
	DIST_MSG_Q_GRP_ID	pktMsgQGrpSendId;			/* uint16_t */
	__DIST_PKT_HDR_END__
	/* message follows */
	} DIST_PKT_MSG_Q_GRP_SEND;

typedef struct						/* DIST_PKT_MSG_Q_GRP_STATUS */
	{
	DIST_PKT			pktMsgQGrpStatusHdr;
	uint32_t			pktMsgQGrpStatusInqId;
	uint32_t			pktMsgQGrpStatusErrno;
	uint16_t			pktMsgQGrpStatusDStatus;
	__DIST_PKT_HDR_END__
	} DIST_PKT_MSG_Q_GRP_STATUS;

/* DGDB (distributed group database) telegrams */

typedef struct						/* DIST_PKT_DGDB_ADD */
	{
	DIST_PKT			pktDgdbAddHdr;
	DIST_NODE_ID		pktDgdbAddCreator;			/* uint32_t */
	DIST_MSG_Q_GRP_ID	pktDgdbAddId;				/* uint16_t */
	__DIST_PKT_HDR_END__
	/* group name follows */
	} DIST_PKT_DGDB_ADD;

/* GAP (group agreement protocol) telegrams */

typedef struct						/* DIST_PKT_GAP_OK */
	{
	DIST_PKT			okHdr;
	DIST_MSG_Q_GRP_ID	okId;						/* uint16_t */
	char				okName[DIST_NAME_MAX_LENGTH + 1];
	} DIST_PKT_GAP_OK;

typedef struct						/* DIST_PKT_GAP_REJECT */
	{
	DIST_PKT			rejectHdr;
	DIST_MSG_Q_GRP_ID	rejectId;					/* uint16_t */
	char				rejectName[DIST_NAME_MAX_LENGTH + 1];
	} DIST_PKT_GAP_REJECT;

typedef struct						/* DIST_PKT_GAP_TRY */
	{
	DIST_PKT			tryHdr;
	DIST_MSG_Q_GRP_ID	tryId;						/* uint16_t */
	char				tryName[DIST_NAME_MAX_LENGTH + 1];
	} DIST_PKT_GAP_TRY;

typedef struct						/* DIST_PKT_GAP_ASK_WAIT */
	{
	DIST_PKT			askWaitHdr;
	DIST_MSG_Q_GRP_ID	askWaitId;					/* uint16_t */
	char				askWaitName[DIST_NAME_MAX_LENGTH + 1];
	} DIST_PKT_GAP_ASK_WAIT;

typedef struct						/* DIST_PKT_GAP_SET */
	{
	DIST_PKT			setHdr;
	DIST_MSG_Q_GRP_ID	setId;						/* uint16_t */
	char				setName[DIST_NAME_MAX_LENGTH + 1];
	} DIST_PKT_GAP_SET;

/* other typedefs */

typedef int DIST_GRP_STATE;			/* DIST_GRP_STATE */

typedef struct _DIST_GRP_DB_NODE	/* DIST_GRP_DB_NODE */
	{
	union
		{
		SL_NODE freeNode;	/* used to link free nodes */
		struct
			{
			char				distGrpName[DIST_NAME_MAX_LENGTH + 1];
			DIST_MSG_Q_GRP_ID	distGrpId;		/* distributed group id */
			MSG_Q_ID			msgQId;			/* message queue id */
			DIST_NODE_ID		distNodeId;		/* id of the creating node */
			SL_LIST				msgQIdLst;		/* list of members */
			DIST_GRP_STATE		grpState;		/* current state of group */
			DIST_GAP_NODE		*pDistGapNode;	/* ptr to GAP structure */
			} dbNode;
		} stateNode;
	int ixNode;
	} DIST_GRP_DB_NODE;

#define grpDbFreeNode	stateNode.freeNode
#define grpDbName		stateNode.dbNode.distGrpName
#define grpDbId			stateNode.dbNode.distGrpId
#define grpDbNodeId		stateNode.dbNode.distNodeId
#define grpDbMsgQIdLst	stateNode.dbNode.msgQIdLst
#define grpDbState		stateNode.dbNode.grpState
#define pGrpDbGapNode	stateNode.dbNode.pDistGapNode
#define grpDbMsgQId		stateNode.dbNode.msgQId

typedef struct						/* DIST_GRP_HASH_NODE */
	{
	HASH_NODE			hashNode;
	DIST_GRP_DB_NODE	*pDbNode;
	} DIST_GRP_HASH_NODE;

typedef struct						/* DIST_GRP_MSG_Q_NODE */
	{
	SL_NODE		slNode;
	MSG_Q_ID	msgQId;
	} DIST_GRP_MSG_Q_NODE;

typedef struct						/* DIST_GRP_BURST */
	{
	DIST_NODE_ID	burstNodeId;
	STATUS			burstStatus;
	} DIST_GRP_BURST;

typedef struct						/* DIST_MSG_Q_GRP_SEND_INQ */
	{
	DIST_INQ	sendInq;
	SEMAPHORE	sendInqLock;			/* lock this group-send inquiry */
    SEMAPHORE	sendInqWait;
	int			sendInqTask;
	int			sendInqNumBlocked;
	int			sendInqNumOutstanding;
	DIST_STATUS	sendInqStatus;
	} DIST_MSG_Q_GRP_SEND_INQ;

/* declarations */

#if defined(__STDC__) || defined(__cplusplus)

DIST_MSG_Q_GRP_ID	msgQDistGrpAgree (DIST_GRP_DB_NODE *pDistGrpDbNode);
STATUS				msgQDistGrpAddMember (DIST_GRP_DB_NODE *pDistGrpDbNode,
						MSG_Q_ID msgQId);
STATUS				msgQDistGrpSend (DIST_MSG_Q_GRP_ID distMsgQGrpId,
						char *buffer, UINT nBytes, int msgQTimeout,
						int overallTimeout, int priority);
STATUS				msgQDistGrpBurst (DIST_NODE_ID nodeId);

DIST_GRP_DB_NODE	*msgQDistGrpLclCreate (char *distGrpName,
						DIST_MSG_Q_GRP_ID grpId, DIST_GRP_STATE grpState);
void				msgQDistGrpLclSetId (DIST_GRP_DB_NODE *pDistGrpDbNode,
						DIST_MSG_Q_GRP_ID grpId);
STATUS				msgQDistGrpLclAddMember (DIST_GRP_DB_NODE *pDbNode,
						MSG_Q_ID msgQId);
DIST_GRP_DB_NODE	*msgQDistGrpLclFindByName (char *distGrpName);
DIST_GRP_DB_NODE	*msgQDistGrpLclFindById (DIST_MSG_Q_GRP_ID uniqGrpId);
void				msgQDistGrpLclEach (FUNCPTR routine, int routineArg);

STATUS				distGapLibInit (void);
DIST_MSG_Q_GRP_ID	distGapStart (DIST_GRP_DB_NODE *pDistGrpDbNode);
void				distGapNodeInit (DIST_GAP_NODE *pDistGapNode,
						DIST_GRP_DB_NODE *pDistGrpDbNode, BOOL link);
void				distGapNodeDelete (DIST_GAP_NODE *pDistGapNode);

#else   /* __STDC__ */

DIST_MSG_Q_GRP_ID	msgQDistGrpAgree ();
STATUS				msgQDistGrpAddMember ();
STATUS				msgQDistGrpSend ();
STATUS				msgQDistGrpBurst ();

DIST_GRP_DB_NODE	*msgQDistGrpLclCreate ();
void				msgQDistGrpLclSetId ();
STATUS				msgQDistGrpLclAddMember ();
DIST_GRP_DB_NODE	*msgQDistGrpLclFindByName ();
DIST_GRP_DB_NODE	*msgQDistGrpLclFindById ();
void				msgQDistGrpLclEach ();

STATUS				distGapLibInit ();
DIST_MSG_Q_GRP_ID	distGapStart ();
void				distGapNodeInit ();
void				distGapNodeDelete ();

#endif  /* __STDC__ */


#ifdef __cplusplus
}
#endif

#endif	/* __INCmsgQDistGrpLibPh */

