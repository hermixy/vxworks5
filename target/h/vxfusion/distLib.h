/* distLib.h - defines for distributed objects (VxFusion) */

/* Copyright 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01e,24may99,drm  adding vxfusion prefix to VxFusion related includes
01d,10nov98,drm  added typedef for DIST_NODE_ID
01c,08sep98,drm  added #defines for default startup parameters
01b,12aug98,drm  removed #includes for private files.
01a,04sep97,ur   written.
*/

#ifndef __INCdistLibh
#define __INCdistLibh

#include "vxWorks.h"
#include "errno.h"
#include "vwModNum.h"
#include "vxfusion/distNodeLib.h"

#ifdef __cplusplus
extern "C" {
#endif

/* defines */

#define DIST_MAX_TBUFS_LOG2_DFLT           9 /* max # TBufs log 2 */
#define DIST_MAX_NODES_LOG2_DFLT           5 /* max # nodes in node DB log 2 */
#define DIST_MAX_QUEUES_LOG2_DFLT          7 /* max # queues per node log 2 */
#define DIST_MAX_GROUPS_LOG2_DFLT          6 /* max # msg Q groups log 2*/
#define DIST_MAX_NAME_DB_ENTRIES_LOG2_DFLT 8 /* max # entries in name DB log 2*/
#define DIST_MAX_TICKS_TO_WAIT_DFLT (4*sysClkRateGet()) /* max ticks to wait */
                                                        /* for other nodes to */
                                                        /* respond at startup */

#define DIST_OBJ_TYPE_MSG_Q     0x0     /* message queue object type */

#define DIST_ID_MSG_Q_SERV      0       /* message queue service */
#define DIST_ID_MSG_Q_GRP_SERV	1       /* message queue group service */
#define DIST_ID_DNDB_SERV       2       /* distributed name database */
#define DIST_ID_DGDB_SERV       3       /* distributed group database */
#define DIST_ID_INCO_SERV       4       /* incorporation protocol */
#define DIST_ID_GAP_SERV        5       /* group agreement protocol */

#define DIST_ID_DIST_LIB		16      /* */
#define DIST_ID_NODE_LIB		17      /* node library */
#define DIST_ID_NET_LIB			18      /* network library */

#define DIST_CTL_TYPE_MASK		0xff    /* control functions 0-255 */

#define DIST_CTL_TYPE_DIST		DIST_ID_DIST_LIB

#define DIST_CTL_LOG_HOOK		((0 << 8) | DIST_CTL_TYPE_DIST)
#define DIST_CTL_PANIC_HOOK		((1 << 8) | DIST_CTL_TYPE_DIST)

#define DIST_NAME_MAX_LENGTH	19      /* max name length for dist name DB */

/* typedefs */

typedef uint32_t    DIST_NODE_ID;       /* unique node identifier */

#if defined(__STDC__) || defined(__cplusplus)

STATUS	distInit (DIST_NODE_ID me, FUNCPTR ifIntiRtn, void *pIfInitConf,
				int maxTBufsLog2, int maxNodesLog2, int maxQueuesLog2,
				int maxGroupsLog2, int maxNamesLog2, int waitNTicks);
int		distCtl (int function, int argument);

#else   /* __STDC__ */

STATUS	distInit ();  /* function to startup distributed objects */
int		distCtl ();   /* control function for distrubted objects */

#endif  /* __STDC__ */


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif	/* __INCdistLibh */

