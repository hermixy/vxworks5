/* msgQDistLibP.h - distributed message queue library private hdr (VxFusion) */

/* Copyright 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01e,24may99,drm  added vxfusion prefix to VxFusion related includes
01d,19feb99,drm  added remoteError member to send/receive _INQ packets
01c,21may98,drm  removed redundant #include statement
01b,08may98,ur   removed 8 bit node id restriction
01a,06jun97,ur   written
*/

#ifndef __INCmsgQDistLibPh
#define __INCmsgQDistLibPh

#ifdef __cplusplus
extern "C" {
#endif

#include "vxWorks.h"
#include "sllLib.h"
#include "msgQLib.h"
#include "private/semLibP.h"
#include "vxfusion/private/distNodeLibP.h"
#include "vxfusion/private/distPktLibP.h"
#include "vxfusion/private/distObjLibP.h"

/* defines */

#define msgQDistTblLock()		semTake(&msgQDistTblLock, WAIT_FOREVER)
#define msgQDistTblUnlock()		semGive(&msgQDistTblLock)

#define IS_DIST_MSG_Q_OBJ(pObjNode) \
	((pObjNode)->objNodeType == DIST_OBJ_TYPE_MSG_Q)

/* DIST_MSG_Q_ID looks like this: |----tblIx-or-grpId----|-type-| */

#define DIST_MSG_Q_TYPE_SHIFT		1
#define DIST_MSG_Q_TYPE_MSG_Q		0x0		/* message queue */
#define DIST_MSG_Q_TYPE_MSG_Q_GRP	0x1		/* message queue group */

#define IS_DIST_MSG_Q_TYPE_GRP(distMsgQId) \
	(((distMsgQId) & ((1 << DIST_MSG_Q_TYPE_SHIFT) - 1)) == \
	 DIST_MSG_Q_TYPE_MSG_Q_GRP)

/* message queue service */

#define DIST_MSG_Q_SERV_NAME			"tServMsgQ"
#define DIST_MSG_Q_SERV_TASK_PRIO		50
#define DIST_MSG_Q_SERV_TASK_STACK_SZ	5000

#define DIST_MSG_Q_SERV_NET_PRIO		0

#define DIST_MSG_Q_WAIT_TASK_PRIO		50
#define DIST_MSG_Q_WAIT_TASK_STACK_SZ	5000

/* priority stuff */

#define DIST_MSG_PRI_MASK				(0x07)

#define DIST_MSG_Q_PRIO_VERIFY(msgQPrio) \
	(((msgQPrio) == MSG_PRI_URGENT) || \
	 ((msgQPrio) == MSG_PRI_NORMAL) || \
	 (((msgQPrio) >= DIST_MSG_PRI_0) && ((msgQPrio) <= DIST_MSG_PRI_7)))

#define DIST_MSG_Q_PRIO_TO_NET_PRIO(msgQPrio) \
	(((msgQPrio) == MSG_PRI_URGENT) ? \
		((int) (DIST_MSG_PRI_0 & DIST_MSG_PRI_MASK)) : \
	 ((msgQPrio) == MSG_PRI_NORMAL) ? \
		((int) (DIST_MSG_PRI_4 & DIST_MSG_PRI_MASK)) : \
	 ((int) ((msgQPrio) & DIST_MSG_PRI_MASK)))

#define DIST_MSG_Q_PRIO_TO_MSG_Q_PRIO(msgQPrio) \
	(((msgQPrio) == MSG_PRI_URGENT) ? MSG_PRI_URGENT : \
	 ((msgQPrio) == MSG_PRI_NORMAL) ? MSG_PRI_NORMAL : \
	 (((msgQPrio) <= DIST_MSG_PRI_3) ?  MSG_PRI_URGENT : MSG_PRI_NORMAL))

#define NET_PRIO_TO_DIST_MSG_Q_PRIO(netPrio) \
	(((netPrio) <= 3) ?  MSG_PRI_URGENT : MSG_PRI_NORMAL)

/* packet subtypes of DIST_PKT_TYPE_MSG_Q */
#define DIST_PKT_TYPE_MSG_Q_SEND			0	/* send a message */
#define DIST_PKT_TYPE_MSG_Q_RECV_REQ		1	/* ask for a message */
#define DIST_PKT_TYPE_MSG_Q_RECV_RPL		2	/* return message */
#define DIST_PKT_TYPE_MSG_Q_NUM_MSGS_REQ	3	/* ask for num messages */
#define DIST_PKT_TYPE_MSG_Q_NUM_MSGS_RPL	4	/* reply with num messages */
#define DIST_PKT_TYPE_MSG_Q_STATUS			64	/* status */

#define DIST_MSG_Q_INQ_TYPE_NUM_MSGS		((0 << 8) | DIST_MSG_Q_INQ)
#define DIST_MSG_Q_INQ_TYPE_RECV			((1 << 8) | DIST_MSG_Q_INQ)
#define DIST_MSG_Q_INQ_TYPE_SEND			((2 << 8) | DIST_MSG_Q_INQ)

#define MSG_Q_DIST_STATUS_OK				OK	/* OK--no error */
#define MSG_Q_DIST_STATUS_ERROR				1	/* general error */
#define MSG_Q_DIST_STATUS_TIMEOUT			2	/* timeout */
#define MSG_Q_DIST_STATUS_UNAVAIL			3	/* message unavailable */
#define MSG_Q_DIST_STATUS_UNREACH			4	/* remote node is unreachable */
#define MSG_Q_DIST_STATUS_ILLEGAL_OBJ_ID	5	/* illegal object id */
#define MSG_Q_DIST_STATUS_NOT_ENOUGH_MEMORY	6	/* out of memory */
#define MSG_Q_DIST_STATUS_INTERNAL_ERROR	7	/* internal error */
#define MSG_Q_DIST_STATUS_PROTOCOL_ERROR	8	/* protocol error */
#define MSG_Q_DIST_STATUS_LOCAL_TIMEOUT		9	/* local timeout */

#define DIST_MSG_Q_NUM_MSGS_PRIO	(servTable[DIST_ID_MSG_Q_SERV].servNetPrio)
#define DIST_MSG_Q_RECV_PRIO		(servTable[DIST_ID_MSG_Q_SERV].servNetPrio)
#define DIST_MSG_Q_ERROR_PRIO		(servTable[DIST_ID_MSG_Q_SERV].servNetPrio)

#define DIST_MSG_Q_TYPE_NBITS		1
#define DIST_MSG_Q_TYPE_MASK		0x1L
#define TYPE_DIST_MSG_Q				0L
#define TYPE_DIST_MSG_Q_GRP			1L

/* maximum number of nodes on the net */
#define DIST_MSG_Q_MAX_NODES_LOG2	8
#define DIST_MSG_Q_MAX_NODES		256

/* maximum number of local message queues */
#define DIST_MSG_Q_MAX_QS_LOG2		8
#define DIST_MSG_Q_MAX_QS			256

/* size of table for local message queues */
#define TBL_SIZE					64
#if TBL_SIZE > DIST_MSG_Q_MAX_QS
#error "Too many local message queues for underlying layer!"
#endif

/*
 * MSG_Q_ID -> foo
 */

/* MSG_Q_ID -> DIST_OBJ_NODE */
#define MSG_Q_ID_TO_DIST_OBJ_NODE(msgQId) \
	((DIST_OBJ_NODE *) (((uint32_t) msgQId) & ~VX_TYPE_OBJ_MASK))

/*
 * DIST_OBJ_NODE -> foo
 */

/* DIST_OBJ_NODE -> MSG_Q_ID */
#define DIST_OBJ_NODE_TO_MSG_Q_ID(pObjNode) \
	((MSG_Q_ID) (((uint32_t) pObjNode) | VX_TYPE_DIST_OBJ))

/*
 * DIST_MSG_Q_ID -> foo
 */

/* DIST_MSG_Q_ID -> DIST_MSG_Q_GRP_ID */
#define DIST_MSG_Q_ID_TO_DIST_MSG_Q_GRP_ID(distMsgQId) \
	((DIST_MSG_Q_GRP_ID) ((distMsgQId) >> DIST_MSG_Q_TYPE_SHIFT))

/* DIST_MSG_Q_ID -> TBL_IX */
#define DIST_MSG_Q_ID_TO_TBL_IX(distMsgQId) \
	((TBL_IX) ((distMsgQId) >> DIST_MSG_Q_TYPE_SHIFT))

/*
 * DIST_MSG_Q_GRP_ID -> foo
 */

/* DIST_MSG_Q_GRP_ID -> DIST_MSG_Q_ID */
#define DIST_MSG_Q_GRP_ID_TO_DIST_MSG_Q_ID(distMsgQGrpId) \
	((DIST_MSG_Q_ID) \
	 (((uint32_t) (distMsgQGrpId) << DIST_MSG_Q_TYPE_SHIFT) | \
	  DIST_MSG_Q_TYPE_MSG_Q_GRP))

/*
 * TBL_IX -> foo
 */

/* TBL_IX -> DIST_MSG_Q_ID */
#define TBL_IX_TO_DIST_MSG_Q_ID(tblIx) \
	((DIST_MSG_Q_ID) \
	 (((uint32_t) (tblIx) << DIST_MSG_Q_TYPE_SHIFT) | \
	  DIST_MSG_Q_TYPE_MSG_Q))

/*
 * IS_xxx
 */

/* typedefs and structures */

typedef uint32_t	DIST_MSG_Q_ID;			/* DIST_MSG_Q_ID */
typedef uint16_t	DIST_MSG_Q_SNGL_ID;		/* DIST_MSG_Q_SNGL_ID */
typedef uint16_t	DIST_MSG_Q_GRP_ID;		/* DIST_MSG_Q_GRP_ID */
typedef uint8_t		TBL_IX;					/* TBL_IX */

typedef short		DIST_MSG_Q_STATUS;		/* DIST_MSG_Q_STATUS */

typedef struct 								/* TBL_NODE */
	{
	union
		{
		SL_NODE		nxtFree;
		MSG_Q_ID	msgQId;
		} tblNodeType;
	TBL_IX   tblIx;
	} TBL_NODE;

#define tblNxtFree	tblNodeType.nxtFree
#define tblMsgQId	tblNodeType.msgQId

/*
 * Header for a packet addressed to a remote message queue.
 */

typedef struct								/* DIST_PKT_MSG_Q_SEND */
	{
	DIST_PKT		sendHdr;
	uint32_t		sendInqId;
	uint32_t		sendTimeout;
	TBL_IX			sendTblIx;				/* uint8_t */
	__DIST_PKT_HDR_END__
	/* message follows */
	} DIST_PKT_MSG_Q_SEND;

typedef struct								/* DIST_PKT_MSG_Q_RECV_REQ */
	{
	DIST_PKT		recvReqHdr;
	uint32_t		recvReqInqId;
	uint32_t		recvReqMaxNBytes;
	uint32_t		recvReqTimeout;
	TBL_IX			recvReqTblIx;			/* uint8_t */
	__DIST_PKT_HDR_END__
	} DIST_PKT_MSG_Q_RECV_REQ;

typedef struct								/* DIST_PKT_MSG_Q_RECV_RPL */
	{
	DIST_PKT		recvRplHdr;
	uint32_t		recvRplInqId;
	__DIST_PKT_HDR_END__
	/* message follows */
	} DIST_PKT_MSG_Q_RECV_RPL;

typedef struct								/* DIST_PKT_MSG_Q_NUM_MSGS_REQ */
	{
	DIST_PKT		numMsgsReqHdr;
	uint32_t		numMsgsReqInqId;
	TBL_IX			numMsgsReqTblIx;		/* uint8_t */
	__DIST_PKT_HDR_END__
	} DIST_PKT_MSG_Q_NUM_MSGS_REQ;

typedef struct								/* DIST_PKT_MSG_Q_NUM_MSGS_RPL */
	{
	DIST_PKT		numMsgsRplHdr;
	uint32_t		numMsgsRplInqId;
	uint32_t		numMsgsRplNum;
	__DIST_PKT_HDR_END__
	} DIST_PKT_MSG_Q_NUM_MSGS_RPL;

typedef struct								/* DIST_PKT_MSG_Q_STATUS */
	{
	DIST_PKT		statusHdr;
	uint32_t		statusInqId;
	uint32_t		statusErrno;
	uint16_t		statusDStatus;
	__DIST_PKT_HDR_END__
	} DIST_PKT_MSG_Q_STATUS;

/*
 * Inquiry nodes.
 */

typedef struct								/* DIST_MSG_Q_SEND_INQ */
	{
	DIST_INQ	sendInq;
	SEMAPHORE	sendInqWait;
	int			sendInqTask;
	BOOL		sendInqMsgQueued;	
	BOOL        remoteError;
	} DIST_MSG_Q_SEND_INQ;

typedef struct								/* DIST_MSG_Q_RECV_INQ */
	{
	DIST_INQ	recvInq;
	SEMAPHORE	recvInqWait;
	int			recvInqTask;
	char		*pRecvInqBuffer;
	UINT		recvInqMaxNBytes;
	BOOL		recvInqMsgArrived;	
	BOOL        remoteError;
	} DIST_MSG_Q_RECV_INQ;

typedef struct								/* DIST_MSG_Q_NUM_MSGS_INQ */
	{
	DIST_INQ	numMsgsInq;
	SEMAPHORE	numMsgsInqWait;
	int			numMsgsInqTask;
	int			numMsgsInqNum;
	} DIST_MSG_Q_NUM_MSGS_INQ;

extern FUNCPTR msgQDistShowRtn;
extern FUNCPTR msgQDistInfoGetRtn;

/* function declarations */

#if defined(__STDC__) || defined(__cplusplus)

MSG_Q_ID	msgQDistGetMapped (MSG_Q_ID msgQId);

#else   /* __STDC__ */

MSG_Q_ID	msgQDistGetMapped ();

#endif  /* __STDC__ */

#ifdef __cplusplus
}
#endif

#endif /* __INCmsgQDistLibPh */

