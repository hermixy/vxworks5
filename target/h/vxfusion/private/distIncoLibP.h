/* distIncoLibP.h - header file for incorporation protocol */

/* Copyright 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,24may99,drm  added vxfusion to VxFusion related includes
01b,12aug98,drm  changed distTBufLibP.h to distTBufLib.h
01a,08sep97,ur   written.
*/

#ifndef __INCdistIncoLibPh
#define __INCdistIncoLibPh

#ifdef __cplusplus
extern "C" {
#endif

#include "vxWorks.h"
#include "vxfusion/private/distPktLibP.h"
#include "vxfusion/private/distNodeLibP.h"
#include "vxfusion/distTBufLib.h"

/* defines */

#define DIST_INCO_SERV_NAME				"tServInco"
#define DIST_INCO_SERV_TASK_PRIO		50
#define DIST_INCO_SERV_TASK_STACK_SZ	5000

#define DIST_INCO_SERV_NET_PRIO			0

#define DIST_INCO_PRIO \
	(servTable[DIST_ID_INCO_SERV].servNetPrio)

#define DIST_PKT_TYPE_INCO_REQ		0
#define DIST_PKT_TYPE_INCO_DONE		1
#define DIST_PKT_TYPE_INCO_UPNOW	2

#define DIST_INCO_STATUS_OK					OK
#define DIST_INCO_STATUS_PROTOCOL_ERROR		1

/* typedefs */

typedef struct						/* DIST_PKT_INCO */
	{
	DIST_PKT	pktIncoHdr;
	} DIST_PKT_INCO;

/* function prototypes */

#if defined(__STDC__) || defined(__cplusplus)

STATUS distIncoInit (void);
STATUS distIncoStart (int waitNTicks);

#else   /* __STDC__ */

STATUS distIncoInit ();
STATUS distIncoStart ();

#endif  /* __STDC__ */

#ifdef __cplusplus
}
#endif

#endif  /* __INCdistIncoLibPh */

