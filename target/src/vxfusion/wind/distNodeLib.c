/* distNodeLib - node library (VxFusion option) */

/* Copyright 1999 - 2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01q,15apr02,jws  fix SPR74878 (not locking node database properly)
01p,23oct01,jws  fix compiler warnings (SPR 71117)
01o,04oct01,jws  final fixes for SPR 34770
01n,27sep01,p_r  Fixes for SPR#34770
01m,11jun99,drm  Changing unicast piggy-backing default to FALSE.
01l,24may99,drm  added vxfusion prefix to VxFusion related includes
01k,11sep98,drm  added #include to pick up distPanic()
01j,12aug98,drm  added #include stmt for distObjTypeP.h
01i,08aug98,drm  added code to set broadcast flag when sending broadcast msgs
01h,08apr98,ur   made `piggy backing' switchable with distNodeCtl()
01g,13mar98,ur   added support for multiple crashed and operational hooks
01f,29jan98,ur   added support for sending negative acknowledgments
01e,23jan98,ur   bug fixed in the delivery of reassembled packets
01d,20jan98,ur   splited distNodeInit() in two parts
01c,19jan98,ur   bug fixed in distNodePktAck()
01b,16jan98,ur   XACK contains state of sending node
01a,10jun97,ur   written.
*/

/*
DESCRIPTION
This library contains the node database, and routines to handle it.
For every node in the system, one entry should be placed in the database.
The database knows about the state of the node (as it looks from the local
node) and about communication between the local node and the remote one.

INTERNAL
The semaphore <distNodeDbSem> is used to lock the node database.  The
database is a hash table of nodes in the system.  Packets chained for
input/output are part of each node's entry.  The table must be locked
when entering an entry.  It is also locked when finding and entry, although
it is not clear that this is necessary because entries are never removed.

The DIST_NODE_COMM structures are the packet queues.  The database must
be locked when dealing with these structures.  It is believed that this
is now true.  Because of the way the communications routines are called,
<distNodeDbSem> is now a mutex, and is sometimes called recursively.

AVAILABILITY
This module is distributed as a component of the unbundled distributed
message queues option, VxFusion.
*/

#include "vxWorks.h"

#undef  DIST_DIAGNOSTIC   /* defining this seems to break VxFusion! */
#undef  DIST_DIAGNOSTIC_SHOW

#if defined (DIST_NODE_REPORT) \
 || defined (DIST_DIAGNOSTIC) \
 || defined (DIST_DIAGNOSTIC_SHOW)

#include "stdio.h"

#endif

#include "stdlib.h"
#include "hashLib.h"
#include "semLib.h"
#include "taskLib.h"
#include "string.h"
#include "errnoLib.h"
#include "netinet/in.h"
#include "private/distObjTypeP.h"
#include "vxfusion/distLib.h"
#include "vxfusion/distIfLib.h"
#include "vxfusion/distNodeLib.h"
#include "vxfusion/distStatLib.h"
#include "vxfusion/private/distLibP.h"
#include "vxfusion/private/distNodeLibP.h"
#include "vxfusion/private/distTBufLibP.h"
#include "vxfusion/private/distPktLibP.h"

/* defines */

/* make two macros look like macros */

#define DIST_NODE_DB_LOCK    distNodeDbLock()
#define DIST_NODE_DB_UNLOCK  distNodeDbUnlock()


#define UNUSED_ARG(x)  if(sizeof(x)) {} /* to suppress compiler warnings */

/* test if b is within [a,c) */
#define winWithin(a, b, c) \
    (((a <= b) && (b < c)) || ((c < a) && (a <= b)) || ((b < c) && (c < a)))

/* add a to b and fold it back */
#define winAdd(a, b) \
    (((a) + (b)) & (DIST_IF_RNG_BUF_SZ - 1))

/* sub a by b and fold it back */
#define winSub(a, b) \
    (((a) - (b)) & (DIST_IF_RNG_BUF_SZ - 1))

#define KEY_ARG              65537        /* used by hash function */
#define KEY_CMP_ARG          0            /* unused */

#define DIST_NODE_MAX_HOOKS  8

/* global variables */

DIST_NODE_ID distNodeLocalId;   /* windSh needs this global */

/*
 * The semaphore used to be binary, but is now initialized as mutex.
 * It cannot be changed back, because it is sometimes taken recursively,
 * and will block if not mutex.
 */
SEM_ID       distNodeDbSem;     /* Should be taken before DIST_TBUF_FREEM */

/* local variables */

LOCAL HASH_ID     distNodeDbId;
LOCAL BOOL        distNodeLibInstalled = FALSE;

LOCAL int        distNodeNumNodesAll = 0;
LOCAL int        distNodeNumNodesAlive = 0;
LOCAL int        distNodeNumNodesOperational = 0;

LOCAL int        distNodeLocalState;

LOCAL DIST_NODE_ID    distNodeGodfatherId;

LOCAL FUNCPTR        distNodeOperationalHook[DIST_NODE_MAX_HOOKS];
LOCAL FUNCPTR        distNodeCrashedHook[DIST_NODE_MAX_HOOKS];

LOCAL int        distNodeMaxRetries = DIST_NODE_MAX_RETRIES;
LOCAL int        distNodeRetryTimeout = DIST_NODE_RETRY_TIMEOUT;

LOCAL BOOL        distNodeSupportNACK = TRUE;   /* negative acknowlege */
LOCAL BOOL        distNodeSupportPBB = FALSE;   /* piggy backing broadcast */
LOCAL BOOL        distNodeSupportPBU = FALSE;   /* piggy backing unicast */

/* local prototypes */

LOCAL DIST_NODE_DB_NODE * distNodeFindById (DIST_NODE_ID nodeId);

LOCAL BOOL   distNodeHCmp (DIST_NODE_DB_NODE *pMatchNode,
                           DIST_NODE_DB_NODE *pHNode,
                           int keyArg);
LOCAL BOOL   distNodeHFunc (int elements, DIST_NODE_DB_NODE *pHNode,
                            int keyArg);
LOCAL STATUS distNodePktResend (DIST_NODE_COMM *pComm,
                                DIST_TBUF_HDR *pTBufHdr);
LOCAL void   distNodeDBTimerTask (void);
LOCAL void   distNodeDBTimer (void);
LOCAL BOOL   distNodeDBNodeTimer (DIST_NODE_DB_NODE *pNode,
                                  DIST_NODE_BTIMO *pBtimo);
LOCAL void   distNodeDBCommTimer (DIST_NODE_DB_NODE *pNode,
                                  DIST_NODE_BTIMO *pBtimo,
                                  BOOL isBroadcastComm);
LOCAL STATUS distNodeSendNegAck (DIST_NODE_DB_NODE *pNode,
                                 short id,
                                 short seq);
LOCAL STATUS distNodeSendAck (DIST_NODE_DB_NODE *pNode,
                              int ackBroadcast,
                              int options);
LOCAL void   distNodeCleanup (DIST_NODE_DB_NODE *pNode);
LOCAL void   distNodeSetState (DIST_NODE_DB_NODE *pNode,
                               int state);
LOCAL STATUS distNodeSendBootstrap (DIST_NODE_ID dest,
                                    int type,
                                    int timeout);

#if defined(DIST_DIAGNOSTIC) || defined(DIST_DIAGNOSTIC_SHOW)
LOCAL BOOL         distNodeNodeShow (DIST_NODE_DB_NODE *pNode,
                                     int dummy);
LOCAL const char * distNodeStateToName (int state);
#endif

/***************************************************************************
*
* distNodeLibInit - initialize this module (VxFusion option)
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

void distNodeLibInit (void)
    {
    }

/***************************************************************************
*
* distNodeInit - initializes the node library (VxFusion option)
*
* This routine initializes the node database. The database can handle
* up to 2^<sizeLog2> entries.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: OK, if successful.
* NOMANUAL
*/

STATUS distNodeInit
    (
    int sizeLog2   /* init database with 2^^sizeLog2 entries */
    )
    {
    int    hashTblSizeLog2;
    int    i, tid;

    if (sizeLog2 < 1)
        return (ERROR);

    if (distNodeLibInstalled == TRUE)
        return (OK);

    for (i = 0; i < DIST_NODE_MAX_HOOKS; i++)
        {
        distNodeOperationalHook[i] = NULL;
        distNodeCrashedHook[i] = NULL;
        }

    distNodeLocalState = DIST_NODE_STATE_BOOT;

    /* Initialize the node database. */

    if (hashLibInit () == ERROR)
        return (ERROR);    /* hashLibInit() failed */

    hashTblSizeLog2 = sizeLog2 - 1;
    distNodeDbId = hashTblCreate (hashTblSizeLog2, distNodeHCmp,
            distNodeHFunc, KEY_ARG);
    if (distNodeDbId == NULL)
        return (ERROR);    /* hashTblCreate() failed */

#ifdef UNDEFINED
    distNodeDbSem = semBCreate (SEM_Q_PRIORITY, SEM_FULL) ; 
#else
    distNodeDbSem = semMCreate (SEM_Q_PRIORITY + SEM_INVERSION_SAFE) ; 
#endif

    /* Get the node database manager running. */

    tid = taskSpawn ("tDiNodeMgr",
                     DIST_NODE_MGR_PRIO,
                     VX_SUPERVISOR_MODE | VX_UNBREAKABLE,
                     DIST_NODE_MGR_STACK_SZ,
                     (FUNCPTR) distNodeDBTimerTask,
                     0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    if (tid == ERROR)
        return (ERROR);    /* taskSpawn() failed */

    distNodeGodfatherId = DIST_IF_BROADCAST_ADDR;

    /*
     * Add a virtual broadcast node.
     * State information (sliding windows, etc.) for outgoing broadcasts
     * must be handled at a centralized place. This is the virtual
     * broadcast node.
     * State information for incoming broadcasts is managed within
     * the broadcast communication substructure for the corresponding
     * node. When a note enters the incorporation phase, all other
     * nodes in the system get aware of it and update their node
     * databases.
     */

    if (distNodeCreate (DIST_IF_BROADCAST_ADDR, DIST_NODE_STATE_OPERATIONAL)
         ==
        NULL)
        {
        return (ERROR);
        }
        
    distNodeLibInstalled = TRUE;

    return (OK);
    }

/***************************************************************************
*
* distNodeHCmp - compare function for hashing in node DB (VxFusion option)
*
* This routine is the hash compare function for node IDs.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: True, if node IDs match.
* NOMANUAL
*/

LOCAL BOOL distNodeHCmp
    (
    DIST_NODE_DB_NODE * pMatchNode, /* first node */
    DIST_NODE_DB_NODE * pHNode,     /* second node */
    int                 keyArg      /* unused arg */
    )
    {
    DIST_NODE_ID  distNodeId1 = pMatchNode->nodeId;
    DIST_NODE_ID  distNodeId2 = pHNode->nodeId;
    
    UNUSED_ARG(keyArg);

    return (distNodeId1 == distNodeId2);
    }

/***************************************************************************
*
* distNodeHFunc - hash function for node DB (VxFusion option)
*
* This is the hash function for node IDs.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: A hash index.
* NOMANUAL
*/

LOCAL BOOL distNodeHFunc
    (
    int                 elements,  /* size of hash table */
    DIST_NODE_DB_NODE * pHNode,    /* node whose ID to hash */
    int                 divisor    /* used by hash computation */
    )
    {
    return ((pHNode->nodeId % divisor) & (elements - 1));
    }

/***************************************************************************
*
* distNodeCtl - control function for node database (VxFusion option)
*
* This routine performs a control function on the node database.
* The following functions are accepted:
* \is
* \i DIST_CTL_RETRY_TIMEOUT
* Set send-timeout in ticks to start with. When no ACK is received
* within the timeout, the packet is resent. The timeout for the
* <n>th sending is: <n> * DIST_CTL_RETRY_TIMEOUT .
* \i DIST_CTL_MAX_RETRIES
* Set a limit for number of retries, when sending fails.
* \i DIST_CTL_GET_LOCAL_ID
* Get local node id.
* \i DIST_CTL_GET_LOCAL_STATE
* Get state of local node.
* \i DIST_CTL_NACK_SUPPORT
* Negative acknowledges (NACKs) are used for requesting a resend of a single
* missing fragment from a packet. NACKs are sent immediately after a
* fragment is found to be missing.
* If <arg> is FALSE (0), the sending of negative acknowledges is disabled.
* If <arg> is TRUE (1), sending of NACKs is enabled. This is the default.
* \i DIST_CTL_PGGYBAK_UNICST_SUPPORT
* If this is enabled, the system waits a version dependent time until it
* sends an acknowledge for a previously received packet.
* If a data packet is sent to
* the acknowledge awaiting host in meantime, the acknowlege is delivered
* in that packet. This switch turns on/off piggy backing for unicast
* communication only.
* If <arg> is FALSE (0), piggy backing is disabled.
* If <arg> is TRUE (1), piggy backing is enabled.
* Piggy backing is enabled for unicast communication by default.
* \i DIST_CTL_PGGYBAK_BRDCST_SUPPORT
* If this is enabled, the system waits
* a version dependent time until it sends an
* acknowledge for a previously received packet. If a data packet is sent to
* the acknowledge awaiting host in meantime, the acknowlege is delivered
* in that packet. This switch turns on/off piggy backing for broadcast
* communication only.
* If <arg> is FALSE (0), piggy backing is disabled.
* If <arg> is TRUE (1), piggy backing is enabled.
* Piggy backing is disabled for broadcast communication by default.
* \i DIST_CTL_OPERATIONAL_HOOK
* Set a function to be called, each time a node shifts to operational state.
* \i DIST_CTL_CRASHED_HOOK
* Set a function to be called, each time a node shifts to crashed state.
* \ie
*
* The prototype of the function hooked to the database, should look
* like this:
* \cs
*    void fnc (DIST_NODE_ID nodeStateChanged);
* \ce
* 
* RETURNS: OK, the value requested, or ERROR if <function> is unknown.
*
* ERRNO:
* S_distLib_UNKNOWN_REQUEST
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
* NOMANUAL
*/

int distNodeCtl
    (
    int function,        /* function code      */
    int arg              /* arbitrary argument */
    )
    {
    int i;

    switch (function)
        {
        case DIST_CTL_MAX_RETRIES:
            if (arg < 0)
                return (ERROR);
            distNodeMaxRetries = arg;
            return (OK);

        case DIST_CTL_RETRY_TIMEOUT:
            if (arg < 0)
                return (ERROR);
            /*
             * <arg> is in ticks;
             * <distNodeRetryTimeout> is in manager wakeups;
             */

            distNodeRetryTimeout = (1000 * arg / sysClkRateGet() +
                    DIST_NODE_MGR_WAKEUP_MSEC - 1) / DIST_NODE_MGR_WAKEUP_MSEC;

            return (OK);

        case DIST_CTL_OPERATIONAL_HOOK:
            {
            for (i = 0; i < DIST_NODE_MAX_HOOKS; i++)
                {
                if (distNodeOperationalHook[i] == NULL)
                    {
                    distNodeOperationalHook[i] = (FUNCPTR) arg;
                    return (OK);
                    }
                }
            return (ERROR);
            }

        case DIST_CTL_CRASHED_HOOK:
            {
            for (i = 0; i < DIST_NODE_MAX_HOOKS; i++)
                {
                if (distNodeCrashedHook[i] == NULL)
                    {
                    distNodeCrashedHook[i] = (FUNCPTR) arg;
                    return (OK);
                    }
                }
            return (ERROR);
            }

        case DIST_CTL_GET_LOCAL_ID:
            return (distNodeLocalId);

        case DIST_CTL_GET_LOCAL_STATE:
            return (distNodeLocalState);

        case DIST_CTL_NACK_SUPPORT:
            if ((BOOL) arg != TRUE && (BOOL) arg != FALSE)
                return (ERROR);
            distNodeSupportNACK = (BOOL) arg;
            return (OK);

        case DIST_CTL_PGGYBAK_BRDCST_SUPPORT:
            if ((BOOL) arg != TRUE && (BOOL) arg != FALSE)
                return (ERROR);
            distNodeSupportPBB = (BOOL) arg;
            return (OK);

        case DIST_CTL_PGGYBAK_UNICST_SUPPORT:
            if ((BOOL) arg != TRUE && (BOOL) arg != FALSE)
                return (ERROR);
            distNodeSupportPBU = (BOOL) arg;
            return (OK);

        default:
            errnoSet (S_distLib_UNKNOWN_REQUEST);
            return (ERROR);
        }
    }

/***************************************************************************
*
* distNodeSendBootstrap - send a bootstrap packet (VxFusion option)
*
* This routine sends a bootstrap packet.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: OK, if packet sent.
* NOMANUAL
*/

LOCAL STATUS distNodeSendBootstrap
    (
    DIST_NODE_ID dest,     /* destination node */
    int          type,     /* bootstrap type */
    int          timeout   /* xmit timeout */
    )
    {
    DIST_TBUF_HDR * pTBufHdr;
    DIST_PKT_BOOT * pPkt;
    DIST_TBUF *     pTBuf;
    
    if ((pTBufHdr = DIST_TBUF_HDR_ALLOC ()) == NULL)
        return (ERROR);             /* out of TBufs */

    pTBufHdr->tBufHdrPrio = DIST_BOOTSTRAP_PRIO;
    pTBufHdr->tBufHdrDest = dest;
    pTBufHdr->tBufHdrOverall = sizeof (*pPkt);
    pTBufHdr->tBufHdrTimo = timeout;
    
    
    if ((pTBuf = DIST_TBUF_ALLOC ()) == NULL)
        {
        DIST_TBUF_FREEM (pTBufHdr);
        return (ERROR);                /* out of TBufs */
        }
        
    pTBuf->tBufFlags = DIST_TBUF_FLAG_HDR | DIST_TBUF_FLAG_BROADCAST;
    pTBuf->tBufType = DIST_TBUF_TTYPE_BOOTSTRAP;
    pTBuf->tBufSeq = 0;
    pTBuf->tBufNBytes = sizeof (*pPkt);

    pPkt = (DIST_PKT_BOOT *) pTBuf->pTBufData;
    

    DIST_TBUF_ENQUEUE (pTBufHdr, pTBuf);

    pPkt->pktBootType = (UINT8) type;
    return (distNodePktSend (pTBufHdr));
    }

/***************************************************************************
*
* distNodeBootstrap - broadcast a bootstrap packet (VxFusion option)
*
* This routine broadcasts a bootstrap packet.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: OK, if broadcast succeeds.
* NOMANUAL
*/

STATUS distNodeBootstrap
    (
    int    timeout   /* xmit timeout */
    )
    {
    STATUS status;
    status = distNodeSendBootstrap (DIST_IF_BROADCAST_ADDR,
                                    DIST_BOOTING_REQ, timeout);
    return (status);
    }

/***************************************************************************
*
* distNodeFindById - find node in node database (VxFusion option)
*
* This routine tries to find the information node with node id <nodeId>
* in the node database.
*
* This routine takes <distNodeDbLock>.  Usually, it will be called with
* the lock already taken, but not always, so the lock is taken here
* to be safe.
*
* RETURNS: Pointer to the node entry, or NULL if not found.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* NOMANUAL
*/

LOCAL DIST_NODE_DB_NODE * distNodeFindById
    (
    DIST_NODE_ID    nodeId   /* ID of node to find */
    )
    {
    DIST_NODE_DB_NODE   distNodeMatch;
    DIST_NODE_DB_NODE * pDistNodeFound;

    distNodeMatch.nodeId = nodeId;
    
    DIST_NODE_DB_LOCK;

    pDistNodeFound =
       (DIST_NODE_DB_NODE *) hashTblFind (distNodeDbId,
                                          (HASH_NODE *) &distNodeMatch,
                                          KEY_CMP_ARG);
    DIST_NODE_DB_UNLOCK;

    return (pDistNodeFound);
    }

/***************************************************************************
*
* distNodeCreate - create a new node (VxFusion option)
*
* This routine creates a new node with id <distNodeId>.
* distNodeCreate() returns a pointer to the DB entry.
*
* This function takes the DB node database lock before inserting the
* node into the database.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: Pointer to node, or NULL.
* NOMANUAL
*/

DIST_NODE_DB_NODE * distNodeCreate
    (
    DIST_NODE_ID    nodeId,   /* ID for new node */
    int             state     /* state of new node */
    )
    {
    DIST_NODE_DB_NODE * pNodeNew;
    DIST_NODE_BFLD *    pBFld;
    int                 sz;

    /* This node is new to the database. */

#ifdef DIST_NODE_REPORT
    distLog("distNodeCreate: Creating node 0x%lx with state %d\n",
            nodeId, state);
#endif

    /* allocate needed space; quit if we can't get it */
    
    pNodeNew = (DIST_NODE_DB_NODE *) malloc (sizeof (DIST_NODE_DB_NODE));
    if (pNodeNew == NULL)
        {
        distStat.memShortage++;
        return (NULL);
        }

    sz = DIST_NODE_BFLD_SIZEOF (DIST_IF_RNG_BUF_SZ);
    pBFld = (DIST_NODE_BFLD *) malloc (4 * sz);
    if (pBFld == NULL)
        {
        free(pNodeNew);          /* no memory leaks here */
        distStat.memShortage++;
        return (NULL);
        }

    /* now fill in DIST_NODE_DB_NODE info */

    bzero ((char *) pNodeNew, sizeof (DIST_NODE_DB_NODE));
    pNodeNew->nodeId = nodeId;
    pNodeNew->nodeBroadcast.commAckLastRecvd = (INT16) winSub (0, 1);
    pNodeNew->nodeState = state;

    distNodeNumNodesAll++;

    if (DIST_NODE_STATE_IS_ALIVE (state))
        distNodeNumNodesAlive++;

    if (state == DIST_NODE_STATE_OPERATIONAL)
        distNodeNumNodesOperational++;

    pNodeNew->nodeState = state;


    bzero ((char *) pBFld, 2 * sz);
    pNodeNew->nodeUnicast.pCommCompleted = pBFld;
    pNodeNew->nodeBroadcast.pCommCompleted = ((char *) pBFld) + sz;
    
    /* now link the node into the data base */
    
    DIST_NODE_DB_LOCK;

    hashTblPut (distNodeDbId, (HASH_NODE *)pNodeNew);

    DIST_NODE_DB_UNLOCK;

    return (pNodeNew);
    }

/***************************************************************************
*
* distNodeSetState - set state of a node (VxFusion option)
*
* This routine is used to change the state of a node.  It is only called
* from two places in distNodeReassemble() which takes the node DB lock.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: N/A
* NOMANUAL
*/

LOCAL void distNodeSetState
    (
    DIST_NODE_DB_NODE * pNode,     /* node */
    int                 newState   /* new state */
    )
    {
    int             curState = pNode->nodeState;
    DIST_NODE_ID    nodeId   = pNode->nodeId;
    int             i;

#ifdef DIST_DIAGNOSTIC
    if (curState != newState)
        distLog ("distNodeSetState: state of node 0x%lx shifts from %s to %s\n",
                 nodeId,
                 distNodeStateToName (curState),
                 distNodeStateToName (newState));
#endif

    if ((! DIST_NODE_STATE_IS_ALIVE (curState)) &&
         DIST_NODE_STATE_IS_ALIVE (newState))
        {
        distNodeNumNodesAlive++;
        }
        
    if (DIST_NODE_STATE_IS_ALIVE (curState) &&
        (! DIST_NODE_STATE_IS_ALIVE (newState)))
        {
        distNodeNumNodesAlive--;
        }

    if (curState < DIST_NODE_STATE_OPERATIONAL &&
            newState == DIST_NODE_STATE_OPERATIONAL)
        {

        distNodeNumNodesOperational++;

        for (i = 0; i < DIST_NODE_MAX_HOOKS; i++)
            {
            if (distNodeOperationalHook[i] != NULL)
                (* (distNodeOperationalHook[i])) (nodeId);
            }

        pNode->nodeState = newState;
        return;
        }

    if (curState == DIST_NODE_STATE_OPERATIONAL &&
        newState < DIST_NODE_STATE_OPERATIONAL)
        {
        distNodeNumNodesOperational--;

        if (newState == DIST_NODE_STATE_CRASHED)
            {
            for (i = 0; i < DIST_NODE_MAX_HOOKS; i++)
                {
                if (distNodeCrashedHook[i] != NULL)
                    (* (distNodeCrashedHook[i])) (nodeId);
                }
            }

        pNode->nodeState = newState;
        return;
        }

    pNode->nodeState = newState;
    }

/***************************************************************************
*
* distNodeOperational - sets a node to be alive (VxFusion option)
*
* This routine changes the state of the node with the id <distNodeId>
* to OPERATIONAL.
*
* This routine takes <distNodeDbLock>.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: Pointer to node, or NULL.
* NOMANUAL
*/

DIST_NODE_DB_NODE * distNodeOperational
    (
    DIST_NODE_ID   distNodeId  /* node ID */
    )
    {
    DIST_NODE_DB_NODE * pNodeFound;
    int                 i;

    DIST_NODE_DB_LOCK;
     
    if ((pNodeFound = distNodeFindById (distNodeId)) != NULL)
        {

        /*
         * This node is already in the node database. If state is already
         * OPERATIONAL, do nothing - we even have not recognized the absence.
         */

        if (! DIST_NODE_IS_ALIVE (pNodeFound))
            distNodeNumNodesAlive++;

        if (pNodeFound->nodeState < DIST_NODE_STATE_OPERATIONAL)
            {

#ifdef DIST_DIAGNOSTIC
        distLog ("distNodeOperational: Set node 0x%lx to state OPERATIONAL\n",
                distNodeId);
#endif
            distNodeNumNodesOperational++;

            for (i = 0; i < DIST_NODE_MAX_HOOKS; i++)
                {
                if (distNodeOperationalHook[i] != NULL)
                    (* (distNodeOperationalHook[i])) (distNodeId);
                }
            }

        pNodeFound->nodeState = DIST_NODE_STATE_OPERATIONAL;
        }
    
    DIST_NODE_DB_UNLOCK;
    
    return (pNodeFound);
    }

/***************************************************************************
*
* distNodeCrashed - sets a node to be dead (VxFusion option)
*
* This routine changes the state of the node with the id <distNodeId>
* to CRASHED.
*
* This routine takes <distNodeDbLock>.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: Pointer to node, or NULL.
* NOMANUAL
*/

DIST_NODE_DB_NODE * distNodeCrashed
    (
    DIST_NODE_ID    distNodeId  /* node ID */
    )
    {
    DIST_NODE_DB_NODE * pNodeFound;
    int                 i;

    /* you cannot kill the broadcast node */
    
    if (distNodeId == DIST_IF_BROADCAST_ADDR)
        return (NULL);
    
    DIST_NODE_DB_LOCK;

    /* find the node */

    if ((pNodeFound = distNodeFindById (distNodeId)) != NULL)
        {

        if (DIST_NODE_IS_ALIVE (pNodeFound))
            distNodeNumNodesAlive--;

        if (pNodeFound->nodeState >= DIST_NODE_STATE_OPERATIONAL)
            {

#ifdef DIST_DIAGNOSTIC
        distLog ("distNodeCrashed: Set node 0x%lx to state CRASHED\n",
                distNodeId);
#endif
            distNodeNumNodesOperational--;

            for (i = 0; i < DIST_NODE_MAX_HOOKS; i++)
                {
                if (distNodeCrashedHook[i] != NULL)
                    (* (distNodeCrashedHook[i])) (distNodeId);
                }
            }

        pNodeFound->nodeState = DIST_NODE_STATE_CRASHED;
        }
    
    DIST_NODE_DB_UNLOCK;
    
    return (pNodeFound);
    }

/***************************************************************************
*
* distNodeLocalGetId - returns the local node id (VxFusion option)
*
* This routine returns the id of the local node.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: Local node ID.
* NOMANUAL
*/

DIST_NODE_ID distNodeLocalGetId (void)
    {
    return (distNodeLocalId);
    }

/***************************************************************************
*
* distNodeLocalSetId - sets the local node id (VxFusion option)
*
* This routine sets the id of the local node.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: N/A
* NOMANUAL
*/

void distNodeLocalSetId
    (
    DIST_NODE_ID    myNodeId    /* node ID */
    )
    {
    distNodeLocalId = myNodeId;
    }

/***************************************************************************
*
* distNodeLocalSetState - sets the state of the local node (VxFusion option)
*
* This routine sets the state of the local node.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: N/A
* NOMANUAL
*/

void distNodeLocalSetState
    (
    int    state    /* new state for local node */
    )
    {
    distNodeLocalState = state;
    }

/***************************************************************************
*
* distNodeLocalGetState - get state of the local node (VxFusion option)
*
* This routine returns the state of the local node.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: Node state.
* NOMANUAL
*/

int distNodeLocalGetState (void)
    {
    return (distNodeLocalState);
    }

/***************************************************************************
*
* distNodeGetNumNodes - returns the number of nodes in a specified state (VxFusion option)
*
* This routine returns the number of nodes currently in a specified state.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: Count of nodes, or -1 if <typeSet> is invalid.
* NOMANUAL
*/

int distNodeGetNumNodes
    (
    int    typeSet   /* state to look for */
    )
    {
    switch (typeSet)
        {
        case DIST_NODE_NUM_NODES_ALL:
            return (distNodeNumNodesAll);

        case DIST_NODE_NUM_NODES_ALIVE:
            return (distNodeNumNodesAlive);

        case DIST_NODE_NUM_NODES_OPERATIONAL:
            return (distNodeNumNodesOperational);

        default:
            return (-1);
        }
    }

/***************************************************************************
*
* distNodeGetGodfatherId - get the id of our godfather (VxFusion option)
*
* This routine returns the id of this node's godfather.
* If no godfather is available, distNodeGetGodfatherId() returns ERROR,
* else OK.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: OK, if godfather exists.
* NOMANUAL
*/

STATUS distNodeGetGodfatherId
    (
    DIST_NODE_ID * nodeId  /* where to return godfather ID */
    )
    {
    if ((*nodeId = distNodeGodfatherId) == DIST_IF_BROADCAST_ADDR)
        return (ERROR);

    return (OK);
    }

#ifdef UNDEFINED

/* This routine does not seem to be used */

/***************************************************************************
*
* distNodeTouch - touch a node (increment message counter) (VxFusion option)
*
* This routine touches a node.
*
* NOTE: Must take <distNodeDbLock> before.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: OK, unless <distNodeId> is invalid.
* NOMANUAL
*/

STATUS distNodeTouch
    (
    DIST_NODE_ID    distNodeId   /* ID of node to touch */
    )
    {
    DIST_NODE_DB_NODE * pDistNodeFound;

    if ((pDistNodeFound = distNodeFindById (distNodeId)) == NULL)
        {
        /*  Node does not exist. Should never happen. */

        distStat.nodeDBNoMatch++;
        return (ERROR);
        }

    DIST_NODE_IN_PKT_INC(pDistNodeFound);

    return (OK);
    }
#endif

/***************************************************************************
*
* distNodeEach - call a routine for each node in the database (VxFusion option)
*
* This routine calls a user-supplied routine once for each node.
* The user-supplied routine should return TRUE if distNodeEach() is to
* continue calling it with the remaining nodes, or FALSE if it is done and
* distNodeEach() can exit.
*
* distNodeEach() returns NULL if traversed whole database, or pointer to
* node entry that distNodeEach() ended with.
*
* NOTE: Takes <distNodeDbLock>.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: Pointer to last node visited.
* NOMANUAL
*/

DIST_NODE_DB_NODE * distNodeEach
    (
    FUNCPTR    routine,      /* function to call at each node */
    int        routineArg    /* argument to function */
    )
    {
    DIST_NODE_DB_NODE * lastNode;

    distNodeDbLock ();
    lastNode = (DIST_NODE_DB_NODE *) hashTblEach (distNodeDbId,
                                                  routine,
                                                  routineArg);

    distNodeDbUnlock ();

    return (lastNode);
    }

/***************************************************************************
*
* distNodeCleanup - Flush out all communication queues (VxFusion option)
*
* This routine flushes all communication queues of <pNode>.  It is only
* called from two places in distNodeReassemble() which takes the node
* DB lock first.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: N/A
* NOMANUAL
*/

LOCAL void distNodeCleanup
    (
    DIST_NODE_DB_NODE * pNode    /* node to clean-up */
    )
    {
    DIST_TBUF_HDR *  pPkt;
    DIST_NODE_COMM * pCommUnicast, *pCommBroadcast;

    pCommUnicast = &pNode->nodeUnicast;
    pCommBroadcast = &pNode->nodeBroadcast;

    /*
     * Clean up communication structures of bootstrapping
     * node.
     *
     * Delete all unicast packets waiting for reassemly.
     */

    while (pCommUnicast->pCommQReass)
        {
        DIST_TBUF_HDR_DEQUEUE (pCommUnicast->pCommQReass, pPkt);
        DIST_TBUF_FREEM (pPkt);
        }

    /*
     * Dequeue all unicast packets waiting for a slide in
     * the window. Wakeup sender.
     */

    while (pCommUnicast->pCommQWinOut)
        {
        DIST_TBUF_HDR_DEQUEUE (pCommUnicast->pCommQWinOut, pPkt);
        pPkt->pTBufHdrTm->tmStatus = DIST_TM_STATUS_UNREACH;
        semGive (&pPkt->pTBufHdrTm->tmWait4);
        /* distNodePktSend() frees packet */
        }

    /*
     * Dequeue all unicast packets waiting for an ACK.
     * Wakeup sender.
     */

    while (pCommUnicast->pCommQAck)
        {
        DIST_TBUF_HDR_DEQUEUE (pCommUnicast->pCommQAck, pPkt);
        pPkt->pTBufHdrTm->tmStatus = DIST_TM_STATUS_UNREACH;
        semGive (&pPkt->pTBufHdrTm->tmWait4);
        /* distNodePktSend() frees packet */
        }
    
    bzero ((char *) pCommUnicast->pCommCompleted,
            DIST_NODE_BFLD_SIZEOF (DIST_IF_RNG_BUF_SZ));

    pCommUnicast->commPktNextExpect = 0;
    pCommUnicast->commAckNextExpect = 0;
    pCommUnicast->commPktNextSend = 0;
    pCommUnicast->commAckDelayed = FALSE;

    /* Delete all broadcast packets waiting for reassembly. */

    while (pCommBroadcast->pCommQReass)
        {
        DIST_TBUF_HDR_DEQUEUE (pCommBroadcast->pCommQReass, pPkt);
        DIST_TBUF_FREEM (pPkt);
        }
    
    bzero ((char *) pCommBroadcast->pCommCompleted,
            DIST_NODE_BFLD_SIZEOF (DIST_IF_RNG_BUF_SZ));

    }

/***************************************************************************
*
* distNodeGetReassembled - get a reassembled packet (VxFusion option)
*
* This routine gets a reassembled data packet.  It takes the node DB lock.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: Ptr to tbuf header.
* NOMANUAL
*/

DIST_TBUF_HDR * distNodeGetReassembled
    (
    DIST_NODE_COMM * pComm  /* ptr to comm node */
    )
    {
    DIST_TBUF_HDR *  pHead;         /* head of tbufs */
    short            nextExpected;  /* next expected seq number */
    DIST_TBUF_HDR *  pNext;         /* next tbuf */

    distNodeDbLock ();    

    pHead = pComm->pCommQNextDeliver;
    nextExpected = pComm->commPktNextExpect;

    if (pHead && pHead->tBufHdrId == nextExpected &&
            DIST_NODE_BTST (pComm->pCommCompleted, nextExpected))
        {

        /* Remove the packet from the reassembly queue. */

        DIST_TBUF_HDR_UNLINK (pComm->pCommQReass, pHead);

        if (nextExpected == (DIST_IF_RNG_BUF_SZ - 1))
            pNext = pComm->pCommQReass;
        else
            pNext = pHead->pTBufHdrNext;

        if (pNext && winAdd (nextExpected, 1) != pNext->tBufHdrId)
            {
            /* if a complete packet is missing, we fall in here */

            pComm->pCommQNextDeliver = NULL;
            }
        else
            {
            /* next packet in ring is the one, we expect next */
            
            pComm->pCommQNextDeliver = pNext;
            }

        distNodeDbUnlock ();            /* UNLOCK */

#ifdef DIST_NODE_REPORT
        /* this is a hack */

        printf ("distNodeGetReassembled: return %p (type %d/%d)\n", pHead,
                *((char *) (DIST_TBUF_GET_NEXT (pHead)->pTBufData)),
                *((char *) (DIST_TBUF_GET_NEXT (pHead)->pTBufData) + 1));
#endif
        return (pHead);
        }

    distNodeDbUnlock ();                /* UNLOCK */

#ifdef DIST_NODE_REPORT
    printf ("distNodeGetReassembled: no packet available\n");
#endif
    return (NULL);
    }

/***************************************************************************
*
* distNodeReassemble - reassemble a packet (VxFusion option)
*
* This routine takes a telegram and tries to reassemble it with other
* fragments already stored in the reassembly list.
* distNodeReassemble() updates the communication windows managed
* within the database.
*
* If <nodeIdSrc> is unknown by now, distNodeReassemble() creates a
* new node in the node database. Creating a node is only allowed when
* receiving a XACK telegram in bootstrap mode or a BOOTSTRAP telegram.
*
* If a packet has been reassembled successfully, distNodeReassemble()
* returns a pointer to the communication structure. Else a NULL pointer
* is returned.
*
* NOTE: Takes <distNodeDbLock>.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: Ptr to comm node.
* NOMANUAL
*/

DIST_NODE_COMM * distNodeReassemble
    (
    DIST_NODE_ID nodeIdSrc,    /* source node */
    int          prio,         /* priority */
    DIST_TBUF *  pTBufNew      /* ptr to tbuf */
    )
    {
    DIST_NODE_DB_NODE *   pNode;
    DIST_NODE_DB_NODE *   pNodeSrc;
    DIST_NODE_COMM *      pCommIn;
    DIST_NODE_COMM *      pCommOut;
    DIST_TBUF_HDR *       pReassHdr;
    DIST_TBUF_HDR *       pReassHdrPrev;
    DIST_TBUF_HDR *       pTBufAck;
    DIST_TBUF_HDR *       pTBufAckNext;
    DIST_TBUF_HDR *       pTBufHdrNew;
    short                 winLo, winHi, pktNext;
    short                 id;
    short                 ack;
    short                 seq;
    short                 nBytes;
    short                 type;
    int                   hasMf;

    /* Get information from telegram */
    
    id = pTBufNew->tBufId;
    ack = pTBufNew->tBufAck;
    seq = pTBufNew->tBufSeq;
    type = pTBufNew->tBufType;
    nBytes = pTBufNew->tBufNBytes;
    hasMf = DIST_TBUF_HAS_MF (pTBufNew);

    distNodeDbLock ();

    /* Find node in hash table. */
    
    pNodeSrc = distNodeFindById (nodeIdSrc);
    
    if (pNodeSrc == NULL)
        {
        if (nodeIdSrc == distNodeLocalId)
            distPanic ("distNodePktReass: someone has same node id we have\n");

        /*
         * Node is unknown by now. Create new node, if this is a
         * XACK in bootstrapping mode or this is a BOOTSTRAP telegram.
         * Else ignore telegram.
         */

        if (distNodeLocalState == DIST_NODE_STATE_BOOT
                && type == DIST_TBUF_TTYPE_BACK
                && nBytes == sizeof (DIST_XACK))
            {
            DIST_XACK    *pXAck = (DIST_XACK *) pTBufNew->pTBufData;
            int            state;

            state = ntohs (pXAck->xAckPktNodeState);

            if ((pNodeSrc = distNodeCreate (nodeIdSrc, state)) == NULL)
                {
                /*
                 * We had problems creating the new node.
                 * This is a fatal error, since we will miss that one.
                 */

                distStat.nodeDBFatal++;
                DIST_TBUF_FREE (pTBufNew);
                distPanic ("distNodePktReass: error accepting node 0x%lx\n",
                        nodeIdSrc);
                }
            }
        else if (type == DIST_TBUF_TTYPE_BOOTSTRAP)
            {
            int state = DIST_NODE_STATE_BOOT;

#ifdef DIST_NODE_REPORT
            printf ("distNodeReassemble: got bootstrap pkt from node 0x%lx\n",
                    nodeIdSrc);
#endif
            if ((pNodeSrc = distNodeCreate (nodeIdSrc, state)) == NULL)
                {
                /*
                 * We had problems creating the new node.
                 * This is a fatal error, since we will miss that one.
                 */

                distStat.nodeDBFatal++;
                DIST_TBUF_FREE (pTBufNew);
                distPanic ("distNodePktReass: error accepting node 0x%lx\n",
                        nodeIdSrc);
                }

            /*
             * If this is a BOOTSTRAP telegram, we have to clean up
             * the communication structures.
             */

            distNodeCleanup (pNodeSrc);
            pNodeSrc->nodeBroadcast.commPktNextExpect = (INT16)winAdd (id, 1);
            }
        else
            {
            /*
             * Packets from unknown nodes are ignored, unless they
             * are of type XACK or BOOTSTRAP.
             */

#ifdef DIST_DIAGNOSTIC
            distLog ("distNodePktReass: packet from unknown node 0x%lx \
                     --ignored\n", nodeIdSrc);
#endif
            DIST_TBUF_FREE (pTBufNew);
            distNodeDbUnlock();
            return (NULL);
            }

#ifdef DIST_DIAGNOSTIC
        distLog ("distNodePktReass: new node 0x%lx created (state %s)\n",
                 nodeIdSrc,
                 distNodeStateToName (distNodeGetState (pNodeSrc)));
#endif
        }

    if (DIST_TBUF_IS_BROADCAST (pTBufNew))
        {
        if ((pNode = distNodeFindById (DIST_IF_BROADCAST_ADDR)) == NULL)
            {
            distStat.nodeDBNoMatch++;
            DIST_TBUF_FREE (pTBufNew);
            distNodeDbUnlock();
            return (NULL);
            }
        pCommIn = &pNodeSrc->nodeBroadcast;
        pCommOut = &pNode->nodeBroadcast;
        }
    else
        pCommIn = pCommOut = &pNodeSrc->nodeUnicast;

#ifdef DIST_NODE_REPORT
    {
    char * typeName;
    switch (type)
        {
        case DIST_TBUF_TTYPE_DTA:
            typeName = "DATA";
            break;
        case DIST_TBUF_TTYPE_ACK:
            typeName = "ACK";
            break;
        case DIST_TBUF_TTYPE_BDTA:
            typeName = "B-DATA";
            break;
        case DIST_TBUF_TTYPE_BACK:
            typeName = "ACK-B";
            break;
        case DIST_TBUF_TTYPE_BOOTSTRAP:
            typeName = "BOOTSTRAP";
            break;
        case DIST_TBUF_TTYPE_NACK:
            typeName = "NACK";
            break;
        default:
            typeName = "unknown";
        }
    printf ("dist..Reass:0x%lx/%s: ", nodeIdSrc, typeName);
    printf ("id %d, ack %d, seq %d, type %d, len %d, hasMf %d\n",
            id, ack, seq, pTBufNew->tBufType, nBytes, hasMf);
    }
#endif

    if (type == DIST_TBUF_TTYPE_BOOTSTRAP)
        {
        int    bootTp;

        if (distNodeGetState (pNodeSrc) != DIST_NODE_STATE_BOOT)
            {
            /*
             * If we receive a BOOTSTARP telegram from a node, that is
             * not in booting mode, it must be rebooting. Cleanup
             * the structure.
             */

            distNodeCleanup (pNodeSrc);
            }

        pNodeSrc->nodeBroadcast.commPktNextExpect = (INT16) winAdd (id, 1);

        bootTp = ((DIST_PKT_BOOT *) pTBufNew->pTBufData)->pktBootType;
        switch (bootTp)
            {
            case DIST_BOOTING_REQ:
                /* Send XACK. */
                
                distNodeSetState (pNodeSrc, DIST_NODE_STATE_BOOT);
                distNodeSendAck (pNodeSrc, 1, DIST_NODE_ACK_EXTENDED);
                break;

#ifdef DIST_DIAGNOSTIC
            default:
                distLog ("dist..Reass: unknown type of BOOTSTRAP telegram\n");
#endif
            }

        distNodeDbUnlock ();
        DIST_TBUF_FREE (pTBufNew);
        return (NULL);
        }

    /* We will fall in here, when we receive a XACK. */
    
    if (type == DIST_TBUF_TTYPE_BACK && nBytes == sizeof (DIST_XACK))
        {
        DIST_XACK    *pXAck = (DIST_XACK *) pTBufNew->pTBufData;

        distNodeSetState (pNodeSrc, ntohs (pXAck->xAckPktNodeState));

        /* In bootstrapping mode. */

        if (distNodeLocalState == DIST_NODE_STATE_BOOT)
            {
            short        pktNextSend;

#ifdef DIST_NODE_REPORT
            printf ("dist..Reass: XACK in bootstrap mode\n");
#endif
            /*
             * The first node that answers to our BOOTSTRAP packet,
             * will be our godfather.
             */

            if (distNodeGodfatherId == DIST_IF_BROADCAST_ADDR)
                distNodeGodfatherId = nodeIdSrc;

            pktNextSend = ntohs (pXAck->xAckPktNextSend);

            pCommIn->commPktNextExpect = pktNextSend;
            }

        distNodeDbUnlock ();
        DIST_TBUF_FREE (pTBufNew);
        return (NULL);
        }

#ifdef DIST_NODE_REPORT
    if (! DIST_NODE_IS_ALIVE (pNodeSrc))
        {
        printf ("dist..Reass: got a telegram from crashed node\n");
        }
#endif

    /*
     * Process ACK field. An ACK commits all packets with
     * an id lower or equal to the value of the ACK.
     *
     * Traverse the list of non-acknowledged packets and
     * decrease the ACK counters of all acknowledged packets
     * in the list. If no ACK is missing for a certain packet,
     * dequeue it and wakeup the sender.
     */

    pktNext = pCommOut->commPktNextSend;
    if (winWithin (pCommOut->commAckNextExpect, ack, pktNext))
        {
        short    ackLo = pCommOut->commAckNextExpect;
        short    ackHi = (INT16) winAdd (ack, 1);

        pTBufAck = pCommOut->pCommQAck;
        pCommIn->commAckLastRecvd = ack;

#ifdef DIST_NODE_REPORT
        printf ("dist..Reass: ACK %d, AckQ %p:\n", ack, pTBufAck);
#endif
        while (pTBufAck)
            {
            pTBufAckNext = DIST_TBUF_HDR_GET_NEXT (pTBufAck);

            if (winWithin (ackLo, pTBufAck->tBufHdrId, ackHi))
                {
#ifdef DIST_NODE_REPORT
                printf ("dist..Reass: within window, wait for %d more ACKs\n",
                        pTBufAck->pTBufHdrTm->tmAckCnt - 1);
#endif
                if (--pTBufAck->pTBufHdrTm->tmAckCnt == 0)
                    {
                    pCommOut->commAckNextExpect =
                        (INT16) winAdd (pCommOut->commAckNextExpect, 1);

                    DIST_TBUF_HDR_UNLINK (pCommOut->pCommQAck, pTBufAck);
                    pTBufAck->pTBufHdrTm->tmStatus = DIST_TM_STATUS_OK;

#ifdef DIST_NODE_REPORT
                    printf ("dist..Reass: %d ACKed\n", pTBufAck->tBufHdrId);
#endif
                    semGive (&pTBufAck->pTBufHdrTm->tmWait4);
                    
                    /*
                     * distNodePktSend()--which should awake now--frees
                     * the packet.
                     */
                    }
                }

            pTBufAck = pTBufAckNext;
            }
        }

    /*
     * If this is a NACK telegram, resend the single fragment, the
     * NACK asks us for. <id> holds the id of the packet, <seq> the
     * sequence number of the fragment within the packet.
     * The NACK telegram can be destroyed at this point, since
     * it contains no data.
     */

    if (type == DIST_TBUF_TTYPE_NACK)
        {
        DIST_TBUF_FREE (pTBufNew);

#ifdef DIST_NODE_REPORT
        printf ("dist..Reass: got NACK for (id %d, seq %d) from node 0x%lx\n",
                id, seq, nodeIdSrc);
#endif
        pTBufAck = pCommOut->pCommQAck;
        while (pTBufAck)
            {
            if (pTBufAck->tBufHdrId == id)
                {
                DIST_TBUF    *pFrag;

                for (pFrag = DIST_TBUF_GET_NEXT (pTBufAck);
                        pFrag && pFrag->tBufSeq != seq;
                        pFrag = DIST_TBUF_GET_NEXT (pFrag));

                pFrag->tBufAck =
                      (UINT16)winSub (pCommIn->commPktNextExpect, 1);

#ifdef DIST_NODE_REPORT
                if (DIST_IF_SEND (nodeIdSrc, pFrag, 0) == ERROR)
                    printf ("dist..Reass: error sending NACK to node 0x%lx\n",
                            nodeIdSrc);
#else
                /* If this fails, there is no need to mess around. */

                DIST_IF_SEND (nodeIdSrc, pFrag, 0);
#endif
                distStat.nodeFragResend++;

                distNodeDbUnlock ();
                return (NULL);
                }
            
            pTBufAck = DIST_TBUF_HDR_GET_NEXT (pTBufAck);
            }

        distNodeDbUnlock ();
        return (NULL);
        }

    /* If this is not a DTA (data) telegram, destroy it. */
    
    if (type != DIST_TBUF_TTYPE_DTA && type != DIST_TBUF_TTYPE_BDTA)
        {
        distNodeDbUnlock ();
        DIST_TBUF_FREE (pTBufNew);
        return (NULL);
        }

    /*
     * Go on processing this telegram?
     * 1) The packet id must be within the window.
     *    The window starts with the id of the packet
     *    expected next and has a size of DIST_NODE_WIN_SZ.
     * 2) The packet should not be reassembled already.
     */

    winLo = pCommIn->commPktNextExpect;
    winHi = (INT16) winAdd (winLo, DIST_NODE_WIN_SZ);
    if (! winWithin (winLo, id, winHi) ||
            DIST_NODE_BTST (pCommIn->pCommCompleted, id))
        {
        /* Not within the window. Destroy the telegram. */

        distNodeDbUnlock ();

#ifdef DIST_NODE_REPORT
        if (DIST_NODE_BTST (pCommIn->pCommCompleted, id))
            printf ("distNodePktReass: packet already reassembled\n");
        else
            printf ("distNodePktReass: not within window (%d..%d)\n",
                    winLo, winSub (winHi, 1));
#endif
        DIST_TBUF_FREE (pTBufNew);
        return (NULL);
        }

    /* Find queue for packet in reassembly list. */
     
    pReassHdr = pCommIn->pCommQReass;
    pReassHdrPrev = NULL;
    while (pReassHdr)
        {
        if (pReassHdr->tBufHdrId > id)
            break;    /* insert a new packet */

        if (pReassHdr->tBufHdrId == id)
            {
            /* We have found the packet, the new telegram belongs to. */

            DIST_TBUF    *pTBuf, *pTBufPrev;
            DIST_TBUF    *pTBufLast = DIST_TBUF_GET_LAST (pReassHdr);
            short        seqLast = pTBufLast->tBufSeq;

#ifdef DIST_NODE_REPORT
            printf ("dist..Reass: packet found, inserting telegram\n");
#endif
            if (seqLast < seq)
                {
                /*
                 * This telegram has the highest sequence number ever
                 * received. Put it to the tail of the TBuf list.
                 */

                DIST_TBUF_ENQUEUE (pReassHdr, pTBufNew);
                pReassHdr->tBufHdrNLeaks += seq - seqLast - 1;

#ifdef UNDEFINED  /* unused, but be careful before excising for good */
                pTBufLast = pTBufNew;
#endif
                if (! DIST_TBUF_IS_BROADCAST (pTBufNew)
                    && (seq - seqLast) > 1)
                    {
                    INT16 i;

                    /*
                     * unlock database--not strictly necessary, but
                     * we are consuming time, holding the lock...
                     */

                    for (i = seqLast + 1; i < seq; i++)
                        distNodeSendNegAck (pNodeSrc, id, i);
                    }
                }
            else
                {
                /*
                 * This telegram must be inserted somewhere in the TBuf list.
                 */

                pTBufPrev = (DIST_TBUF *) pReassHdr;
                pTBuf = DIST_TBUF_GET_NEXT (pReassHdr);
                while (pTBuf != NULL)
                    {
                    if (pTBuf->tBufSeq == seq)
                        {
                        /* We have already received this one. */
                        
                        distNodeDbUnlock ();
                        DIST_TBUF_FREE (pTBufNew);
                        return (NULL);
                        }
    
                    if (pTBuf->tBufSeq > seq)
                        {
                        DIST_TBUF_INSERT_AFTER (pReassHdr,
                                                pTBufPrev,
                                                pTBufNew);
                        pReassHdr->tBufHdrNLeaks--;
                        break;
                        }

                    pTBufPrev = pTBuf;
                    pTBuf = DIST_TBUF_GET_NEXT (pTBuf);
                    }
                }

            /* The telegram is in the reassembly list now. */

            pReassHdr->tBufHdrOverall += nBytes;

            /*
             * Test if the packet is fully reassembled (there
             * are no leaks within the packet and the last
             * fragment has the 'more fragments' bit cleared).
             */

            pTBufLast = DIST_TBUF_GET_LAST (pReassHdr);
            if (!pReassHdr->tBufHdrNLeaks && !(DIST_TBUF_HAS_MF (pTBufLast)))
                {
                /*
                 * We have received a complete packet and
                 * do not expect to receive more fragments.
                 */

#ifdef DIST_NODE_REPORT
                {
                DIST_TBUF    *pFirst = DIST_TBUF_GET_NEXT (pReassHdr);
                
                /* this is a hack */
                
                printf ("dist..Reass: packet %d complete (type %d/%d)\n", id,
                        *((char *) (pFirst->pTBufData)),
                        *((char *) (pFirst->pTBufData) + 1));
                }
#endif
                DIST_NODE_BSET (pCommIn->pCommCompleted, id);

                /*
                 * Test if the reassembled packet is the one
                 * we expect next.
                 */

                if (pReassHdr->tBufHdrId == pCommIn->commPktNextExpect)
                    {
                    /*
                     * This is the packet, we expect next.
                     * Return communication pointer to caller,
                     * so that it knows about the job.
                     */

                    distNodeDbUnlock ();
                    return (pCommIn);
                    }
                }
            distNodeDbUnlock ();
            return (NULL);
            }

        pReassHdrPrev = pReassHdr;
        pReassHdr = DIST_TBUF_HDR_GET_NEXT (pReassHdr);
        }
    
    /*
     * This is the first fragment of a new packet. There are either
     * more fragments, or this packet is not the one, we are waiting
     * for.
     */
     
#ifdef DIST_NODE_REPORT
    printf ("dist..Reass: first fragment of new packet\n");
#endif

    if ((pTBufHdrNew = DIST_TBUF_HDR_ALLOC ()) == NULL)
        {
        distStat.netInDiscarded++; /* nodeInDiscarded counts pkts discarded */
        distNodeDbUnlock ();
        DIST_TBUF_FREE (pTBufNew);
        return (NULL);
        }

    pTBufHdrNew->tBufHdrSrc = nodeIdSrc;
    pTBufHdrNew->tBufHdrId = id;
    pTBufHdrNew->tBufHdrPrio = prio;
    pTBufHdrNew->tBufHdrOverall = nBytes;
    pTBufHdrNew->tBufHdrNLeaks = seq;
    pTBufHdrNew->tBufHdrTimo = WAIT_FOREVER;    /* reassembly timeout */

    DIST_TBUF_SET_NEXT (pTBufHdrNew, pTBufNew);
    DIST_TBUF_SET_LAST (pTBufHdrNew, pTBufNew);

    DIST_TBUF_HDR_INSERT_AFTER (pCommIn->pCommQReass, pReassHdrPrev,
        pTBufHdrNew);

    if (pCommIn->commPktNextExpect == id)
        /* this is the packet we want to deliver next */
        pCommIn->pCommQNextDeliver = pTBufHdrNew;

    if (seq == 0 && ! hasMf)
        {
        /* We have received a complete packet in a single telegram. */

#ifdef DIST_NODE_REPORT
        /* this is a hack */
        
        printf ("dist..Reass: packet %d complete (type %d/%d)\n", id,
                *((char *) (pTBufNew->pTBufData)),
                *((char *) (pTBufNew->pTBufData) + 1));
#endif
        DIST_NODE_BSET (pCommIn->pCommCompleted, id);
        if (id == pCommIn->commPktNextExpect)
            {
            /*
             * This is the packet, we expect next.
             * Return communication pointer to caller,
             * so that it knows about the job.
             */

            distNodeDbUnlock ();
            return (pCommIn);
            }
        }

    distNodeDbUnlock ();

    /* Check if we have to send negative acknowledges (NACKs). */
    
    if (! DIST_TBUF_IS_BROADCAST (pTBufNew))
        {
        DIST_TBUF_HDR    *pPrev;
        DIST_TBUF        *pLastOfPrev;

        /*
         * Check if we have received the last fragment of the previous
         * packet. We cannot find out how many fragments are missing at
         * the end of the packet. So just send a NACK for the successor
         * of the last telegram received.
         */

        if ((pPrev = DIST_TBUF_HDR_GET_PREV (pTBufHdrNew)) != NULL &&
                (pLastOfPrev = DIST_TBUF_GET_LAST (pTBufHdrNew)) != NULL &&
                DIST_TBUF_HAS_MF (pLastOfPrev))
            {
            distNodeSendNegAck (pNodeSrc, pPrev->tBufHdrId,
                    pLastOfPrev->tBufSeq + 1);
            }

        /*
         * Check if some telegrams from the beginning of the new packet
         * are missing. Send a NACK for each of them.
         */

        if (seq > 0)
            {
            INT16 i;
            for (i = 0; i < seq; i++)
                distNodeSendNegAck (pNodeSrc, id, i);
            }

        }

    return (NULL);
    }

/***************************************************************************
*
* distNodePktSend - send a packet already stored in TBufs (VxFusion option)
*
* This routine waits for an open send window and transmits the packet,
* by sending the data of each single TBuf.
* Afterwards it blocks for an ACK from the remote node.
*
* NOTE: Takes <distNodeDbLock>.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: OK, if successful.
* NOMANUAL
*/

STATUS distNodePktSend
    (
    DIST_TBUF_HDR * pTBufHdr    /* message to send */
    )
    {
    DIST_NODE_DB_NODE *  pDistNodeFound;
    DIST_TRANSMISSION    distTm;
    DIST_NODE_COMM *     pComm;
    DIST_NODE_ID         nodeIdDest = pTBufHdr->tBufHdrDest;
    DIST_TBUF *          pTBuf;
    short                winLo, winHi;
    short                pktId;
    int                  priority;
    int                  wait4NumAcks;

    distNodeDbLock ();

    distStat.nodeOutReceived++;

    if ((pDistNodeFound = distNodeFindById (nodeIdDest)) == NULL)
        {
#ifdef DIST_DIAGNOSTIC
        distLog ("distNodePktSend: unknown node 0x%lx\n", nodeIdDest);
#endif
        distStat.nodeDBNoMatch++;
        distNodeDbUnlock ();
        DIST_TBUF_FREEM (pTBufHdr);
        return (ERROR);
        }

    if (! DIST_NODE_IS_ALIVE (pDistNodeFound))
        {
#ifdef DIST_DIAGNOSTIC
        distLog ("distNodePktSend: destination not alive\n");
#endif
        distStat.nodeNotAlive++;
        distNodeDbUnlock ();
        DIST_TBUF_FREEM (pTBufHdr);
        return (ERROR);
        }

#ifdef DIST_DIAGNOSTIC
    if (DIST_TBUF_GET_NEXT (pTBufHdr) == NULL)
        {
        distLog ("distNodePktSend: TBuf chain contains no data\n");
        }
#endif

    if (nodeIdDest == DIST_IF_BROADCAST_ADDR)
        {
        pComm = &pDistNodeFound->nodeBroadcast;
        wait4NumAcks = distNodeNumNodesAlive - 1;
        }
    else
        {
        pComm = &pDistNodeFound->nodeUnicast;
        wait4NumAcks = 1;
        }

    semBInit (&distTm.tmWait4, SEM_Q_FIFO, SEM_EMPTY);
    distTm.tmStatus = DIST_TM_STATUS_OK;
    distTm.tmRetmTimo = distNodeRetryTimeout;
    distTm.tmNumTm = 1;
    distTm.tmAckCnt = wait4NumAcks;

    pTBufHdr->pTBufHdrTm = &distTm;

    /*
     * Each node has two queues for outgoing packets.
     * One is the queue for packets that should been
     * send, but the "window is closed" (full).
     * The other is the queue for packets send, and
     * waiting for an ACK.
     * In both of the two queues, packets get older.
     */

    pTBufHdr->tBufHdrId = pktId = pComm->commPktNextSend;
    winLo = pComm->commAckNextExpect;
    winHi = (INT16) winAdd (pComm->commAckNextExpect, DIST_NODE_WIN_SZ);

    /*
     * Can we send the packet directly or do we have to wait
     * until the "window opens".
     */

    if (! winWithin (winLo, pktId, winHi))
        {
        /* Put packet to the WaitToSend queue. */
        
        DIST_TBUF_HDR_ENQUEUE (pComm->pCommQWinOut, pTBufHdr);

        distNodeDbUnlock ();    /* unlock before sleep */

        /* Wait for a place within the window. */
        
        semTake (&distTm.tmWait4, WAIT_FOREVER);

        if (distTm.tmStatus != DIST_TM_STATUS_OK)
            {
            distNodeDbUnlock ();       /* this is not needed here! */
            DIST_TBUF_FREEM (pTBufHdr);
            return (ERROR);            /* e.g. timeout, node removed */
            }

        distNodeDbLock ();
        
        /* Remove it from the WaitToSend queue. */
        
        DIST_TBUF_HDR_UNLINK (pComm->pCommQWinOut, pTBufHdr);
        }

    pComm->commPktNextSend = (INT16) winAdd (pComm->commPktNextSend, 1);

    /* Put packet to the WaitForAck queue and send it. */
    
    DIST_TBUF_HDR_ENQUEUE (pComm->pCommQAck, pTBufHdr);
    
    pTBuf = (DIST_TBUF *) pTBufHdr;
    priority = pTBufHdr->tBufHdrPrio;
    while ((pTBuf = DIST_TBUF_GET_NEXT (pTBuf)) != NULL)
        {
        pTBuf->tBufId = pktId;
        pTBuf->tBufAck = (UINT16) winSub (pComm->commPktNextExpect, 1);

#ifdef DIST_NODE_REPORT
        printf ("distNodePktSend:%p: id %d, ack %d, seq %d, len %d, type %d\n",
                pTBuf, pTBuf->tBufId, pTBuf->tBufAck, pTBuf->tBufSeq,
                pTBuf->tBufNBytes, pTBuf->tBufType);
#endif
        if (DIST_IF_SEND (nodeIdDest, pTBuf, priority) == ERROR)
            {
            distNodeDbUnlock ();
            DIST_TBUF_HDR_UNLINK (pComm->pCommQAck, pTBufHdr);
            DIST_TBUF_FREEM (pTBufHdr);
            return (ERROR);
            }

        pComm->commAckDelayed = FALSE;
        }
    
    distNodeDbUnlock ();    /* unlock before sleep */

    /* Wait for ACK. */
    
#ifdef DIST_NODE_REPORT
    distLog ("distNodePktSend: sleep while waiting for ACK\n");
#endif
    semTake (&distTm.tmWait4, WAIT_FOREVER);
#ifdef DIST_NODE_REPORT
    distLog ("distNodePktSend: awacked: status of tm is %d\n",
             distTm.tmStatus);
#endif

    /*
     * distNodePktReass() or distNodeTimer() has already
     * dequeued the packet.
     */

    DIST_TBUF_FREEM (pTBufHdr);

    if (distTm.tmStatus != DIST_TM_STATUS_OK)
        return (ERROR);                    /* e.g. timeout */

    return (OK);
    }

/***************************************************************************
*
* distNodePktResend - resend a packet (VxFusion option)
*
* Resend a packet already transmitted with distNodePktSend ().
*
* Currently called one place in distNodeDbCommTimer() which
* has <distNodeDbLock> taken before it is called.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: OK, if successful.
* NOMANUAL
*/

LOCAL STATUS distNodePktResend
    (
    DIST_NODE_COMM * pComm,          /* communication channel */
    DIST_TBUF_HDR *  pTBufHdr        /* packet to retransmit */
    )
    {
    DIST_TBUF *     pTBuf;
    DIST_NODE_ID    nodeIdDest = pTBufHdr->tBufHdrDest;
    int             priority = pTBufHdr->tBufHdrPrio;

    distStat.nodePktResend++;

    pTBuf = (DIST_TBUF *) pTBufHdr;
    while ((pTBuf = DIST_TBUF_GET_NEXT (pTBuf)) != NULL)
        {
        pTBuf->tBufAck = (UINT16) winSub (pComm->commPktNextExpect, 1);
#ifdef DIST_NODE_REPORT
        printf ("distNodePktResend: resend with ACK for %d (%d)\n",
                pTBuf->tBufAck, pComm->commPktNextExpect);
#endif

        if (DIST_IF_SEND (nodeIdDest, pTBuf, priority) == ERROR)
            {
            DIST_TBUF_HDR_UNLINK (pComm->pCommQAck, pTBufHdr);
            DIST_TBUF_FREEM (pTBufHdr);
            return (ERROR);
            }
        }

    return (OK);
    }
    
/***************************************************************************
*
* distNodePktAck - ask for an ACK for a received and consumed packet (VxFusion option)
*
* This routine acknowledeges a packet. If DIST_NODE_ACK_IMMEDIATELY is
* not set in <options>, an ACK telegram is not transmitted immediately
* but a flag is set. If data is sent to the ACK awaiting node, within a
* certain range of time, the ACK is sent within the data telegram.
* Else an ACK telegram is transmitted. This is known as `piggy-backing'.
* Due to the design, piggy-backing only works for unicasts. For broadcasts,
* an ACK is always sent immediately.
*
* NOTE: Takes <distNodeDbLock>.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: OK, if successful.
* NOMANUAL
*/

STATUS distNodePktAck
    (
    DIST_NODE_ID    nodeIdSrc,  /* source node */
    DIST_TBUF_HDR * pPktAck,    /* ack packet */
    int             options     /* options */
    )
    {
    DIST_NODE_DB_NODE * pDistNodeFound;
    DIST_NODE_COMM *    pComm;
    short               id = pPktAck->tBufHdrId;
    int                 ackBroadcast;

#ifdef DIST_DIAGNOSTIC
    if (nodeIdSrc == DIST_IF_BROADCAST_ADDR)
        distPanic ("distNodePktAck: cannot send ACK to broadcast node.\n");
#endif

    distNodeDbLock ();

    /*
     * Find node, the acknowledge is destinated to.
     */

    if ((pDistNodeFound = distNodeFindById (nodeIdSrc)) == NULL)
        {
#ifdef DIST_DIAGNOSTIC
        distLog ("distNodePktAck: try to send ACK to unknown node 0x%lx\n",
                 nodeIdSrc);
#endif
        distStat.nodeDBNoMatch++;
        distNodeDbUnlock ();
        return (ERROR);
        }

    /* Do we respond to a broadcast? */

    ackBroadcast = DIST_TBUF_IS_BROADCAST (DIST_TBUF_GET_NEXT (pPktAck));

    if (ackBroadcast)
        {
        pComm = &pDistNodeFound->nodeBroadcast;
        if (!distNodeSupportPBB)
            options |= DIST_NODE_ACK_IMMEDIATELY;
#ifdef DIST_NODE_REPORT
        printf ("distNodePktAck: ack broadcast %d form node 0x%lx immediately\n",
                id, nodeIdSrc);
#endif
        }
    else
        {
        pComm = &pDistNodeFound->nodeUnicast;
        if (!distNodeSupportPBU)
            options |= DIST_NODE_ACK_IMMEDIATELY;
#ifdef DIST_NODE_REPORT
        printf ("distNodePktAck: ack unicast %d form node 0x%lx %s\n",
                id, nodeIdSrc,
                options
                & DIST_NODE_ACK_IMMEDIATELY ? "immediately" : "delayed");
#endif
        }
    
    if (id != pComm->commPktNextExpect)    /* should never happen */
        distPanic ("distNodePktAck: acknowledge in wrong order\n");

    DIST_NODE_BCLR (pComm->pCommCompleted, id);
    pComm->commPktNextExpect = (INT16) winAdd (pComm->commPktNextExpect, 1);

    if (options & DIST_NODE_ACK_IMMEDIATELY)
        {
        /* Ack immediately. */
        
        distNodeSendAck (pDistNodeFound, ackBroadcast, options);
        }
    else
        {
        /*
         * Ask for a delayed ACK. If data is transfered before, the ACK
         * is piggy-backed with the data packet.
         */

        pComm->commAckDelayed = TRUE;
        }

    distNodeDbUnlock ();

    distStat.nodeAcked++;
    return (OK);
    }

/***************************************************************************
*
* distNodeSendNegAck - send a negative acknowledge (NACK) (VxFusion option)
*
* Send a negative acknowledgment, asking for a resend of fragment <seq>
* of packet <id>. A NACK (like an ACK) also holds the id of the last
* correctly received packet.
*
* NOTE: Currently, NACKs cannot be used in broadcast communication.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: OK, if sent.
* NOMANUAL
*/

LOCAL STATUS distNodeSendNegAck
    (
    DIST_NODE_DB_NODE * pNode,   /* target node */
    short               id,      /* id */
    short               seq      /* seq number */
    )
    {
    DIST_TBUF * pTBuf;
    STATUS      status;

    if (! distNodeSupportNACK)
        return (OK);

#ifdef DIST_NODE_REPORT
    printf ("distNodeSendNegAck: NACK (id %d, seq %d) to node 0x%lx\n",
            id, seq, pNode->nodeId);
#endif
    if ((pTBuf = DIST_TBUF_ALLOC ()) == NULL)
        return (ERROR);

    /* fill in telegram header */
    
    pTBuf->tBufId = id;              /* id of missing telegram */
    pTBuf->tBufSeq = seq;            /* sequence number of missing telegram */
    pTBuf->tBufAck = (UINT16)winSub (pNode->nodeUnicast.commPktNextExpect, 1);
    pTBuf->tBufNBytes = 0;
    pTBuf->tBufType = DIST_TBUF_TTYPE_NACK;
    pTBuf->tBufFlags = DIST_TBUF_FLAG_HDR;

    /* Send NACK. */
    
    if ((status = DIST_IF_SEND (pNode->nodeId, pTBuf, 0)) == OK)
        pNode->nodeUnicast.commAckDelayed = FALSE;

    DIST_TBUF_FREE (pTBuf);
    return (status);
    }

/***************************************************************************
*
* distNodeSendAck - send an ACK to the network (VxFusion option)
*
* Send an acknowledge telegram to the network. The ACK holds the
* id of the last correctly received packet.
*
* This is called from three places, all of which take <distNodeDbLock>.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: OK, if successful.
* NOMANUAL
*/

LOCAL STATUS distNodeSendAck
    (
    DIST_NODE_DB_NODE * pNode,           /* target node */
    int                 ackBroadcast,    /* broadcast flag */
    int                 options          /* options */
    )
    {
    DIST_NODE_COMM * pComm;
    DIST_NODE_ID     nodeIdDest;
    DIST_TBUF *      pTBuf;
    STATUS           status;

    nodeIdDest = pNode->nodeId;
    pComm = (ackBroadcast ? &pNode->nodeBroadcast : &pNode->nodeUnicast);

    if ((pTBuf = DIST_TBUF_ALLOC ()) == NULL)
        return (ERROR);

    pTBuf->tBufId = 0;        /* the telegram id is ignored */
    pTBuf->tBufAck = (UINT16) winSub (pComm->commPktNextExpect, 1);
    pTBuf->tBufSeq = 0;
    pTBuf->tBufNBytes = 0;
    pTBuf->tBufFlags = DIST_TBUF_FLAG_HDR;

    if (ackBroadcast)
        {
        /* This is an ACK for a broadcast. */
        
        pTBuf->tBufType = DIST_TBUF_TTYPE_BACK;
        pTBuf->tBufFlags |= DIST_TBUF_FLAG_BROADCAST;
#ifdef DIST_NODE_REPORT
        printf ("distNodeSendAck: acknowledge broadcast %d from node 0x%lx\n",
                pTBuf->tBufAck, nodeIdDest);
#endif
        if (options & DIST_NODE_ACK_EXTENDED)
            {
            DIST_NODE_DB_NODE * pBroadcastNode;
            DIST_XACK *         pXAck = pTBuf->pTBufData;
            int                 nextSend;

            pBroadcastNode = distNodeFindById (DIST_IF_BROADCAST_ADDR);
            if (pBroadcastNode == NULL)
                {
                DIST_TBUF_FREE (pTBuf);
                return (ERROR);
                }

            nextSend = htons (pBroadcastNode->nodeBroadcast.commPktNextSend);
            pXAck->xAckPktNextSend = (UINT16) nextSend;
            pXAck->xAckPktNodeState = htons ((uint16_t) distNodeLocalState);
            pTBuf->tBufNBytes += sizeof (*pXAck);
            }
        }
    else
        {
        /* This is an ACK for a unicast. */
        
        pTBuf->tBufType = DIST_TBUF_TTYPE_ACK;
#ifdef DIST_NODE_REPORT
        printf ("distNodeSendAck: acknowledge unicast %d from node 0x%lx\n",
                pTBuf->tBufAck, nodeIdDest);
#endif
        if (options & DIST_NODE_ACK_EXTENDED)
            {
            DIST_XACK * pXAck = pTBuf->pTBufData;

            pXAck->xAckPktNextSend =
                         htons (pNode->nodeUnicast.commPktNextSend);
            pXAck->xAckPktNodeState = htons ((uint16_t) distNodeLocalState);
            pTBuf->tBufNBytes += sizeof (*pXAck);
            }
        }

    /* Send ACK. */
    
    if ((status = DIST_IF_SEND (nodeIdDest, pTBuf, 0)) == OK)
        pComm->commAckDelayed = FALSE;

    DIST_TBUF_FREE (pTBuf);
    return (status);
    }

/***************************************************************************
*
* distNodePktDiscard - discard a packet (VxFusion option)
*
* This routine discards a packet. No ACK is send.
*
* NOTE: Takes <distNodeDbLock>.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: OK, if successful.
* NOMANUAL
*/

STATUS distNodePktDiscard
    (
    DIST_NODE_ID    nodeIdSrc,
    DIST_TBUF_HDR * pPktDiscard
    )
    {
    DIST_NODE_DB_NODE * pDistNodeFound;
    DIST_NODE_COMM *    pComm;
    short               id = pPktDiscard->tBufHdrId;

    distNodeDbLock ();

    if ((pDistNodeFound = distNodeFindById (nodeIdSrc)) == NULL)
        {
#ifdef DIST_DIAGNOSTIC
        distLog ("distNodePktDel: try to delete from unknown node 0x%lx\n",
                 nodeIdSrc);
#endif
        distStat.nodeDBNoMatch++;
        distNodeDbUnlock ();
        return (ERROR);
        }

#ifdef DIST_NODE_REPORT
    printf ("distNodePktDiscard: discard packet %d from node 0x%lx\n",
            id, nodeIdSrc);
#endif

    if (DIST_TBUF_IS_BROADCAST (DIST_TBUF_GET_NEXT (pPktDiscard)))
        pComm = &pDistNodeFound->nodeBroadcast;
    else
        pComm = &pDistNodeFound->nodeUnicast;
    
    distStat.nodeInDiscarded++;

    /* Clear id in bitfield, but do not move the window */
    
    DIST_NODE_BCLR (pComm->pCommCompleted, id);

    distNodeDbUnlock ();

    /* Free the discarded packet. */
    
    DIST_TBUF_FREEM (pPktDiscard);

    return (OK);
    }

/***************************************************************************
*
* distNodeDBTimerTask - node database manager task (VxFusion option)
*
* For now, distNodeDBTimer() is called from a task. The caller may be a
* watchdog in future.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: N/A
* NOMANUAL
*/

LOCAL void distNodeDBTimerTask (void)
    {
    while (1)
        {
        distNodeDBTimer ();
        taskDelay (DIST_NODE_MGR_WAKEUP_TICKS);
        }
    }

/***************************************************************************
*
* distNodeDBTimer - periodic routine, to update the node database (VxFusion option)
*
* Updates each node of the database.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: N/A
* NOMANUAL
*/

LOCAL void distNodeDBTimer (void)
    {
    DIST_NODE_DB_NODE * pNode;
    DIST_NODE_BTIMO     btimo;

    DIST_NODE_DB_LOCK;
    
    if ((pNode = distNodeFindById (DIST_IF_BROADCAST_ADDR)) != NULL)
        distNodeDBCommTimer (pNode, &btimo, TRUE);

    DIST_NODE_DB_UNLOCK;

    distNodeEach ((FUNCPTR) distNodeDBNodeTimer, (int) &btimo);
    }

/***************************************************************************
*
* distNodeDBNodeTimer - periodic routine, to update single node of node DB (VxFusion option)
*
* This routine handles unicast and broadcast communication of a specific
* node.
*
* NOTE: <distNodeDbLock> must be taken before this function is called.
* Currently, it is only invoked by distNodeEach(), which takes the lock.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: TRUE, if updated.
* NOMANUAL
*/

LOCAL BOOL distNodeDBNodeTimer
    (
    DIST_NODE_DB_NODE * pNode,   /* node to update */
    DIST_NODE_BTIMO *   pBtimo   /* time interval */
    )
    {
    if (pNode->nodeId == DIST_IF_BROADCAST_ADDR)
        return (TRUE);

    distNodeDBCommTimer (pNode, pBtimo, FALSE);    /* unicast */
    distNodeDBCommTimer (pNode, pBtimo, TRUE);     /* broadcast */
    return (TRUE);    /* continue */
    }

/***************************************************************************
*
* distNodeDBCommTimer - periodic routine, to update a communication link (VxFusion option)
*
* NOTE: Must take <distNodeDbLock> before.  Currently called from
* distNodeDBTimer() and distNodeDBNodeTimer(), which take the lock.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: N/A
* NOMANUAL
*/

LOCAL void distNodeDBCommTimer
    (
    DIST_NODE_DB_NODE * pNode,            /* node to update */
    DIST_NODE_BTIMO *   pBtimo,           /* time interval */
    BOOL                isBroadcastComm   /* broadcast boolean */
    )
    {
    DIST_NODE_COMM * pComm;
    DIST_TBUF_HDR *  pTBufWin;
    DIST_TBUF_HDR *  pTBufReass;
    DIST_TBUF_HDR *  pTBufAck;
    DIST_TBUF_HDR *  pTBufNext;

    /*
     * Define, what part of the node we are working on:
     * broadcast communication or unicast communication
     */

    if (isBroadcastComm)
        pComm = &pNode->nodeBroadcast;
    else
        pComm = &pNode->nodeUnicast;

    if (pNode->nodeId == DIST_IF_BROADCAST_ADDR)
        {
        pBtimo->btimoWinLo = pComm->commAckNextExpect;
        pBtimo->btimoWinHi = pComm->commPktNextSend;
        pBtimo->btimoTimedOut = FALSE;
        }
    else
        {
        if (isBroadcastComm && pBtimo->btimoTimedOut)
            {
            short    wl = pBtimo->btimoWinLo;
            short    wh = pBtimo->btimoWinHi;

            /*
             * Last ACK should fall in between this range. If not,
             * the node is declared dead.
             */

            if (! winWithin (wl, pComm->commAckLastRecvd, wh))
                {
                distNodeCrashed (pNode->nodeId);
                return;
                }
            }
        }

    /* Look for delayed ACKs. */

    if (pComm->commAckDelayed)
        {
        /* Send ACK. */
        
        distNodeSendAck (pNode, isBroadcastComm, 0);
        }

    /*
     * Look for packets that waited for first transmission,
     * but have timed out meanwhile.
     */

    pTBufNext = pComm->pCommQWinOut;
    while ((pTBufWin = pTBufNext) != NULL)
        {
        pTBufNext = DIST_TBUF_HDR_GET_NEXT (pTBufWin);

        /* Has user's timeout expired?  */
        
        if ((pTBufWin->tBufHdrTimo != WAIT_FOREVER) &&
            (pTBufWin->tBufHdrTimo -= DIST_NODE_MGR_WAKEUP_TICKS) <= 0)
            {
            /* Timed out. */
            
            pTBufWin->pTBufHdrTm->tmStatus = DIST_TM_STATUS_TIMEOUT;
            semGive (&pTBufWin->pTBufHdrTm->tmWait4);
            }
        }

    /*
     * Look for packets that were send, but not acknowledged
     * by now.
     */

    pTBufNext = pComm->pCommQAck;
    while ((pTBufAck = pTBufNext) != NULL)
        {
        pTBufNext = DIST_TBUF_HDR_GET_NEXT (pTBufAck);

        /* Has user's timeout expired? */
        
        if ((pTBufAck->tBufHdrTimo != WAIT_FOREVER) &&
            (pTBufAck->tBufHdrTimo -= DIST_NODE_MGR_WAKEUP_TICKS) <= 0)
            {
            /* Timed out. */
            
            if (distNodeLocalState == DIST_NODE_STATE_BOOT)
                {
                pComm->commAckNextExpect =
                    (INT16) winAdd (pComm->commAckNextExpect, 1);
                pTBufAck->pTBufHdrTm->tmStatus = DIST_TM_STATUS_OK;
                distNodeLocalState = DIST_NODE_STATE_NETWORK;
                }
            else
                pTBufAck->pTBufHdrTm->tmStatus = DIST_TM_STATUS_TIMEOUT;

            DIST_TBUF_HDR_UNLINK (pComm->pCommQAck, pTBufAck);
            semGive (&pTBufAck->pTBufHdrTm->tmWait4);
            continue;
            }

        /* Timeout for retransmisson. */

        if (pTBufAck->pTBufHdrTm->tmRetmTimo--  <= 0)
            {
            /* Timed out. Retransmit. */
    
            if (pTBufAck->pTBufHdrTm->tmNumTm >= distNodeMaxRetries + 1)
                {
                if (distNodeLocalState == DIST_NODE_STATE_BOOT)
                    {
                    pComm->commAckNextExpect =
                        (INT16) winAdd (pComm->commAckNextExpect, 1);
                    pTBufAck->pTBufHdrTm->tmStatus = DIST_TM_STATUS_OK;
                    distNodeLocalState = DIST_NODE_STATE_NETWORK;
                    }
                else
                    {
                    if (pNode->nodeId == DIST_IF_BROADCAST_ADDR)
                        {
                        short    wl = pBtimo->btimoWinLo;
                        short    wh = pComm->commPktNextSend;

                        if (winWithin (wl, pTBufAck->tBufHdrId, wh))
                            pBtimo->btimoWinLo = pTBufAck->tBufHdrId;

                        pBtimo->btimoTimedOut = TRUE;
                        }
                    else
                        {
                        /*
                         * The remote note cannot be reached. Declare the
                         * node to be dead and set the transmission status
                         * to "unreachable". Wake up sender.
                         */

                        (void) distNodeCrashed (pNode->nodeId);
                        }

                    pTBufAck->pTBufHdrTm->tmStatus = DIST_TM_STATUS_UNREACH;
                    }

                DIST_TBUF_HDR_UNLINK (pComm->pCommQAck, pTBufAck);
                semGive (&pTBufAck->pTBufHdrTm->tmWait4);
                }
            else
                {
                distNodePktResend (pComm, pTBufAck);

                pTBufAck->pTBufHdrTm->tmNumTm++;
                pTBufAck->pTBufHdrTm->tmRetmTimo =
                        pTBufAck->pTBufHdrTm->tmNumTm * distNodeRetryTimeout;
                }
            }
        }

    /*
     * Look for packets that were received in fragments, but not completed
     * by now.
     */

    pTBufNext = pComm->pCommQReass;
    while ((pTBufReass = pTBufNext) != NULL)
        {
        pTBufNext = DIST_TBUF_HDR_GET_NEXT (pTBufReass);

        if ((pTBufReass->tBufHdrTimo != WAIT_FOREVER) &&
            (pTBufReass->tBufHdrTimo -= DIST_NODE_MGR_WAKEUP_TICKS) <= 0)
            {
            distNodeCrashed (pTBufReass->tBufHdrSrc);
            DIST_TBUF_HDR_UNLINK (pComm->pCommQReass, pTBufReass);
            pComm->commPktNextExpect =
                             (INT16)winAdd (pTBufReass->tBufHdrId, 1);
            DIST_TBUF_FREEM (pTBufReass);
            }
        }
    }

#ifdef DIST_DIAGNOSTIC_SHOW

/***************************************************************************
*
* distNodeShow - show routine for a node from node database (VxFusion option)
*
* This routine looks up node <distNodeId> and displays information
* from node database.
* Only intended for internal use.
*
* NOTE: Takes <distNodeDbLock>.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: OK, if processed.
* NOMANUAL
*/

STATUS distNodeShow
    (
    DIST_NODE_ID    distNodeId    /* node ID to show */
    )
    {
    DIST_NODE_DB_NODE * pDistNodeFound;

    distNodeDbLock ();

    if ((pDistNodeFound = distNodeFindById (distNodeId)) == NULL)
            {
            distNodeDbUnlock ();
            return (ERROR);
            }

    printf ("%d: state %d\n",
            (int) pDistNodeFound->nodeId,
            pDistNodeFound->nodeState);

    printf ("  unicast/in: buffer: ");
    DIST_NODE_BPRINT (pDistNodeFound->nodeUnicast.pCommCompleted,
            DIST_IF_RNG_BUF_SZ);
    printf ("\n");
    printf ("  unicast/in: reassQ %p, dlvr nxt %p\n",
            pDistNodeFound->nodeUnicast.pCommQReass,
            pDistNodeFound->nodeUnicast.pCommQNextDeliver);
    printf ("  unicast/in: expect nxt %d, last ack rcvd %d\n",
            pDistNodeFound->nodeUnicast.commPktNextExpect,
            pDistNodeFound->nodeUnicast.commAckLastRecvd);
    printf ("  unicast/out: winOutQ %p, ackQ %p,\n",
            pDistNodeFound->nodeUnicast.pCommQWinOut,
            pDistNodeFound->nodeUnicast.pCommQAck);
    printf ("  unicast/out: nxt ack xpct %d, nxt snd %d, ack dlyd %d\n",
            pDistNodeFound->nodeUnicast.commAckNextExpect,
            pDistNodeFound->nodeUnicast.commPktNextSend,
            pDistNodeFound->nodeUnicast.commAckDelayed);

    printf ("  broadcast/in: buffer: ");
    DIST_NODE_BPRINT (pDistNodeFound->nodeBroadcast.pCommCompleted,
            DIST_IF_RNG_BUF_SZ);
    printf ("\n");
    printf ("  broadcast/in: reassQ %p, dlvr nxt %p\n",
            pDistNodeFound->nodeBroadcast.pCommQReass,
            pDistNodeFound->nodeBroadcast.pCommQNextDeliver);
    printf ("  broadcast/in: expect next %d, last ack rcvd %d\n",
            pDistNodeFound->nodeBroadcast.commPktNextExpect,
            pDistNodeFound->nodeBroadcast.commAckLastRecvd);
    printf ("  broadcast/out: winOutQ %p, ackQ %p,\n",
            pDistNodeFound->nodeBroadcast.pCommQWinOut,
            pDistNodeFound->nodeBroadcast.pCommQAck);
    printf ("  broadcast/out: nxt ack xpct %d, nxt snd %d, ack dlyd %d\n",
            pDistNodeFound->nodeBroadcast.commAckNextExpect,
            pDistNodeFound->nodeBroadcast.commPktNextSend,
            pDistNodeFound->nodeBroadcast.commAckDelayed);

    distNodeDbUnlock ();
    return (OK);
    }

/***************************************************************************
*
* distNodeDbShow - brief overview of node database (VxFusion option)
*
* Print a summary of the database.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: OK.
* NOMANUAL
*/

STATUS distNodeDbShow (void)
    {
    printf ("                       UNICAST------------------- ");
    printf ("BROADCAST-----------------\n");
    printf ("NODE ID    STATE       PKT XPCT ACK XPCT  PKT SND ");
    printf ("PKT XPCT ACK XPCT  PKT SND\n");
    printf ("---------- ----------- -------- -------- -------- ");
    printf ("-------- -------- --------\n");
    distNodeEach ((FUNCPTR) distNodeNodeShow, 0);
    return (OK);
    }

/***************************************************************************
*
* distNodeNodeShow - helper for distNodeDbShow() (VxFusion option)
*
* Print database node info.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: TRUE.
* NOMANUAL
*/

LOCAL BOOL distNodeNodeShow
    (
    DIST_NODE_DB_NODE * pNode,    /* node to display */
    int                 dummy     /* unused argument */
    )
    {
    char * stateNm;

    printf ("0x%8lx ", pNode->nodeId);
    switch (pNode->nodeState)
        {
        case DIST_NODE_STATE_BOOT:
            stateNm = "booting";
            break;
        case DIST_NODE_STATE_NETWORK:
            stateNm = "network";
            break;
        case DIST_NODE_STATE_OPERATIONAL:
            stateNm = "operational";
            break;
        case DIST_NODE_STATE_CRASHED:
            stateNm = "crashed";
            break;
        case DIST_NODE_STATE_RESYNC:
            stateNm = "resync";
            break;
        default:
            stateNm = "unknown";
        }
    printf ("%11s ", stateNm);

    if (pNode->nodeId == DIST_IF_BROADCAST_ADDR)
        {
        printf ("      --       --       -- ");
        printf ("      -- %8d %8d\n",
                pNode->nodeBroadcast.commAckNextExpect,
                pNode->nodeBroadcast.commPktNextSend);
        }
    else
        {
        printf ("%8d %8d %8d ",
                pNode->nodeUnicast.commPktNextExpect,
                pNode->nodeUnicast.commAckNextExpect,
                pNode->nodeUnicast.commPktNextSend);
        printf ("%8d       --       --\n",
                pNode->nodeBroadcast.commPktNextExpect);
        }

    return (TRUE);
    }



/***************************************************************************
*
* distNodeStateToName - helper for distNodeDbShow() (VxFusion option)
*
* Return string describing constant.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: Descriptive string.
* NOMANUAL
*/
LOCAL const char *distNodeStateToName
    (
    int state      /* state to decode */
    )
    {
LOCAL const char *const dead[] =
    {"CRASHED"};

LOCAL const char *const alive[] =
    {"BOOTING", "RESYNC", "NETWORK", "OPERATIONAL"};

    if (DIST_NODE_STATE_IS_ALIVE (state))
        return (alive[((state & ~DIST_NODE_STATE_ALIVE) - 1)]);
    else
        return (dead[((state & ~DIST_NODE_STATE_ALIVE) - 1)]);
    }

#endif    /* DIST_DIAGNOSTIC_SHOW */

