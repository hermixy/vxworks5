/* distIfLib.h - defines and protypes for interface adapters (VxFusion) */

/* Copyright 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01d,24may99,drm  adding vxfusion prefix to vxfusion related includes
01c,22feb99,drm  removed distNetInput()
01b,12aug98,drm  added distNetInput() to function prototypes
01a,01sep97,ur   written.
*/

#ifndef __INCdistIfLibh
#define __INCdistIfLibh

#include "vxWorks.h"
#include "vxfusion/distNodeLib.h"
#include "vxfusion/distTBufLib.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* DIST_IF is a structure which is used to communicate information about
 * an interface such as MTU size to the upper layers. */

/* defines */

#define DIST_IF_NAME \
	(pDistIf->distIfName)          /* macro to access name field */

#define DIST_IF_MTU \
	(pDistIf->distIfMTU)           /* macro to access MTU size field */ 

#define DIST_IF_HDR_SZ \
	(pDistIf->distIfHdrSize)       /* macro to access network hdr size field */

#define DIST_IF_BROADCAST_ADDR \
	(pDistIf->distIfBroadcastAddr) /* macro to access broadcast addr field */

#define DIST_IF_RNG_BUF_SZ \
	(pDistIf->distIfRngBufSz)      /* macro to access ring buffer size field */

#define DIST_IF_MAX_FRAGS \
	(pDistIf->distIfMaxFrags)      /* macro to access max fragments field */

#define DIST_IF_FILTERADD(mask) \
	((pDistIf->distIfIoctl) (IOFILTERADD, (mask))) /* macro to call IOCTL */

#define DIST_IF_SEND(destId, pTBuf, prio) \
	((pDistIf->distIfSend) ((destId), (pTBuf), (prio))) /* macro to call send */

/* typedefs */

typedef struct   /* DIST_IF */
	{
	char         *distIfName;         /* name of the interface */
	int          distIfMTU;           /* MTU size of interface's transport */
	int          distIfHdrSize;       /* network header size */
	DIST_NODE_ID distIfBroadcastAddr; /* broadcast address for the interface */
	short        distIfRngBufSz;      /* # buffers in sliding window protocol */
	short        distIfMaxFrags;      /* max frags a msg can be broken into */ 

	int		(*distIfIoctl) (int fnc, ...); /* adapter IOCTL function */
	STATUS	(*distIfSend) (DIST_NODE_ID destId, DIST_TBUF *pTBuf, int prio);
                                           /* send function of the adapter */ 
	} DIST_IF;

extern DIST_IF	*pDistIf;             /* ptr to DIST_IF struct in adapter */


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __INCdistIfLibh */

