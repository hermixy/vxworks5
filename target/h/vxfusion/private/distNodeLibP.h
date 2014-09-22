/* distNodeLibP - distributed objects node layer private header (VxFusion) */

/* Copyright 1999 - 2001 Wind River Systems, Inc. */

/*
modification history
--------------------
01e,04oct01,jws  final fixes for SPR 34770
01d,27sep01,p_r  Fixes for SPR#34770
01c,24may99,drm  added vxfusion to VxFusion related includes
01b,10nov98,drm  moved node-related control function codes to public .h file
01a,18jul97,ur   written.
*/

#ifndef __INCdistNodeLibPh
#define __INCdistNodeLibPh

#ifdef __cplusplus
extern "C" {
#endif

#include "vxWorks.h"
#include "hashLib.h"
#include "sysLib.h"
#include "vxfusion/distNodeLib.h"
#include "vxfusion/private/distTBufLibP.h"
#include "private/semLibP.h"

/* defines */

/*
 * Modification 01d changed wakeup interval from 500 ms to 100ms
 * to allow better granularity in timeouts.  This is the
 * vxWorks AE interval at the time of this writing.
 */
 
#define DIST_NODE_MGR_WAKEUP_MSEC  100

#define DIST_NODE_MGR_PRIO	   10
#define DIST_NODE_MGR_STACK_SZ	   5000

#define DIST_BOOTSTRAP_PRIO	   0

/*
 * The following timeouts are in units of DIST_NODE_MGR_WAKEUP_MSEC.
 */
 
#define DIST_NODE_RETRY_TIMEOUT	   2 /* 2 times DIST_NODE_MGR_WAKEUP_MSEC */

#define	DIST_NODE_MAX_RETRIES	   5 /* max retries before giving up */

#define DIST_NODE_STATE_UNREACH	   0x00
#define DIST_NODE_STATE_ALIVE	   0x80

#define DIST_NODE_STATE_IS_ALIVE(state) \
	((state) & DIST_NODE_STATE_ALIVE)

#define DIST_NODE_IS_ALIVE(pNode) \
	(DIST_NODE_STATE_IS_ALIVE ((pNode)->nodeState))

#define DIST_NODE_STATE_IS_ACTIVE(state) \
	((state) >= DIST_NODE_STATE_NETWORK)

#define DIST_NODE_STATE_IS_PASSIVE(state) \
	((state) < DIST_NODE_STATE_NETWORK)

#define DIST_NODE_STATE_CRASHED		(1 | DIST_NODE_STATE_UNREACH)

#define DIST_NODE_STATE_BOOT		(1 | DIST_NODE_STATE_ALIVE)
#define DIST_NODE_STATE_RESYNC		(2 | DIST_NODE_STATE_ALIVE)
#define DIST_NODE_STATE_NETWORK		(3 | DIST_NODE_STATE_ALIVE)
#define DIST_NODE_STATE_OPERATIONAL	(4 | DIST_NODE_STATE_ALIVE)

/*
 * distNodeGetNumNodes() types
 */

#define DIST_NODE_NUM_NODES_ALL		0
#define DIST_NODE_NUM_NODES_ALIVE	1
#define DIST_NODE_NUM_NODES_OPERATIONAL	2

/*
 * distNodePktAck() options
 */

#define DIST_NODE_ACK_IMMEDIATELY	0x01
#define DIST_NODE_ACK_EXTENDED		0x02

#define DIST_NODE_WIN_SZ												\
	((DIST_IF_RNG_BUF_SZ + 1) >> 1)

#define DIST_NODE_MGR_WAKEUP_TICKS										\
	(DIST_NODE_MGR_WAKEUP_MSEC * sysClkRateGet () / 1000)

#define DIST_NODE_IN_PKT_ZERO(pNode)									\
	{																	\
	(pNode)->nodeInPkt.low = 0;											\
	(pNode)->nodeInPkt.high = 0;										\
	}

#define DIST_NODE_IN_PKT_INC(pNode)										\
	{																	\
	if (!(++(pNode)->nodeInPkt.low))									\
		(pNode)->nodeInPkt.high++;										\
	}

#define DIST_NODE_BFLD_SIZEOF(numBits) \
	(((numBits) + 7) / 8)

#define DIST_NODE_BSET(varName, bit) \
	(varName[((bit) / 8)] |= (1 << ((bit) % 8)))

#define DIST_NODE_BCLR(varName, bit) \
	(varName[((bit) / 8)] &= ~(1 << ((bit) % 8)))

#define DIST_NODE_BTST(varName, bit) \
    (varName[((bit) / 8)] & (1 << ((bit) % 8)))

#define DIST_NODE_BPRINT(varName, numBits) \
	{ \
	int i, j; \
	unsigned char c; \
	for (i=0; i<(((numBits) + 7) / 8); i++) \
		{ \
		c = varName[i]; \
		for (j=0; j<8; j++) \
			{ \
			printf ("%c", ((c & 0x1) ? 'x' : '.')); \
			c >>= 1; \
			} \
		} \
	}

#define distNodeGetState(pNode)	((pNode)->nodeState)

#define distNodeDbLock()	semTake (distNodeDbSem, WAIT_FOREVER)
#define distNodeDbUnlock()	semGive (distNodeDbSem);

/* typedefs */

typedef uint8_t		DIST_NODE_BFLD;		/* DIST_NODE_BFLD */

typedef struct					/* DIST_NODE_COMM */
    {

/* incoming */

    DIST_NODE_BFLD *	pCommCompleted;
    DIST_TBUF_HDR *	pCommQReass;		/* telegrams to be reassembled */
    DIST_TBUF_HDR *	pCommQNextDeliver;	/* ptr to packet to deliver next */
    short		commPktNextExpect;	/* id of packet expected next */
    short		commAckLastRecvd;	/* last ACK received */

    /* outgoing */

    DIST_TBUF_HDR *	pCommQWinOut;		/* packets waiting for a slide */
    DIST_TBUF_HDR *	pCommQAck;		/* packets waiting for an ACK */
    short		commAckNextExpect;	/* id of ACK expected next */
    short		commPktNextSend;	/* id of packet to send next */
    BOOL		commAckDelayed;		/* do an ACK if no piggybacking */
    } DIST_NODE_COMM;

typedef struct					/* DIST_NODE_QUAD */
    {
    u_long	high;
    u_long	low;
    } DIST_NODE_QUAD;

/*
 * Next structure forms a node of the network node DB
 */

typedef struct				/* DIST_NODE_DB_NODE */
    {
    HASH_NODE		nodeHashNode;

    /* information about the node itself */

    DIST_NODE_ID	nodeId;
    int			nodeState;	/* state of the node (ALIVE, etc.) */
    DIST_NODE_QUAD	nodeInPkt;

    /* communication related information */

    DIST_NODE_COMM	nodeUnicast;
    DIST_NODE_COMM	nodeBroadcast;

    } DIST_NODE_DB_NODE;

typedef struct				/* DIST_XACK */
    {
    uint16_t	xAckPktNextSend;
    uint16_t	xAckPktNodeState;
    } DIST_XACK;

typedef struct				/* DIST_NODE_BTIMO */
    {
    BOOL	btimoTimedOut;
    short	btimoWinLo;
    short	btimoWinHi;
    } DIST_NODE_BTIMO;

/* Globals */

extern SEM_ID distNodeDbSem;   /* used to lock node DB */

/* function declarations */

#if defined(__STDC__) || defined(__cplusplus)

void			distNodeLibInit (void);
STATUS			distNodeInit (int sizeLog2);
STATUS			distNodeCtl (int function, int arg);
DIST_NODE_DB_NODE *	distNodeCreate (DIST_NODE_ID distNodeId, int state);
DIST_NODE_DB_NODE *	distNodeOperational (DIST_NODE_ID distNodeId);
DIST_NODE_DB_NODE *	distNodeCrashed (DIST_NODE_ID distNodeId);
DIST_NODE_DB_NODE *	distNodeEach (FUNCPTR routine, int routineArg);
STATUS			distNodeGetGodfatherId (DIST_NODE_ID *pNodeId);
int			distNodeGetNumNodes (int typeSet);
DIST_TBUF_HDR *		distNodeGetReassembled (DIST_NODE_COMM *pComm);
DIST_NODE_COMM *	distNodeReassemble (DIST_NODE_ID nodeIdSrc,
                                            int prio,
					    DIST_TBUF *pTBufNew);
STATUS			distNodePktAck (DIST_NODE_ID nodeIdSrc,
					DIST_TBUF_HDR *pTBufHdr,
				        int options);
STATUS			distNodePktDiscard (DIST_NODE_ID nodeIdSrc,
					    DIST_TBUF_HDR *pTBufHdr);
STATUS			distNodePktSend (DIST_TBUF_HDR *pTBufHdr);
void			distNodeLocalSetId (DIST_NODE_ID id);
DIST_NODE_ID		distNodeLocalGetId (void);
void			distNodeLocalSetState (int state);
int			distNodeLocalGetState (void);
STATUS			distNodeBootstrap (int timeout);

#else   /* __STDC__ */

void			distNodeLibInit ();
STATUS			distNodeInit ();
STATUS			distNodeCtl ();
DIST_NODE_DB_NODE *	distNodeCreate ();
DIST_NODE_DB_NODE *	distNodeOperational ();
DIST_NODE_DB_NODE *	distNodeCrashed ();
DIST_NODE_DB_NODE *	distNodeEach ();
STATUS			distNodeGetGodfatherId ();
int			distNodeGetNumNodes ();
DIST_TBUF_HDR *		distNodeGetReassembled ();
DIST_NODE_COMM *	distNodeReassemble ();
STATUS			distNodePktAck ();
STATUS			distNodePktDiscard ();
STATUS			distNodePktSend ();
void			distNodeLocalSetId ();
DIST_NODE_ID		distNodeLocalGetId ();
void			distNodeLocalSetState ();
int			distNodeLocalGetState ();
STATUS			distNodeBootstrap ();

#endif  /* __STDC__ */

#ifdef __cplusplus
}
#endif

#endif  /* __INCdistNodeLibPh */

