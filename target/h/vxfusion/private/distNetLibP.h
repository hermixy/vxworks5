/* distNetLibP.h - distributed objects network layer private header (VxFusion)*/

/* Copyright 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,24may99,drm  added vxfusion to VxFusion related includes
01b,10nov98,drm  moved net-related distCtl() parameters to public .h file
01a,17jul97,ur   written.
*/

#ifndef __INCdistNetLibPh
#define __INCdistNetLibPh

#ifdef __cplusplus
extern "C" {
#endif

#include "vxWorks.h"
#include "vxfusion/distLib.h"
#include "vxfusion/distTBufLib.h"
#include "vxfusion/distNetLib.h"
#include "vxfusion/private/distNodeLibP.h"
#include "vxfusion/private/distPktLibP.h"

/* defines */

#define DIST_SERV_MAX			16

#define DIST_NET_SERV_INST(servId)	(servTable[(servId)].servTaskId)

/* typedefs */

typedef struct							/* DIST_IOVEC */
	{
	void	*pIOBuffer;
	UINT	IOLen;
	} DIST_IOVEC;

typedef struct							/* DIST_SERV_NODE */
	{
	SEMAPHORE		servWait4Jobs;
	SEMAPHORE		servQLock;
	int				servId;
	BOOL			servUp;
	DIST_TBUF_HDR	*pServQ;
	int				servTaskId;
	int				servTaskPrio;
	int				servNetPrio;
	} DIST_SERV_NODE;

typedef int	DIST_STATUS;				/* DIST_STATUS */

typedef DIST_STATUS (* DIST_SERV_FUNC) (DIST_NODE_ID, DIST_TBUF_HDR *);

/* extern */

extern DIST_SERV_NODE servTable[];

/* function declarations */

#if defined(__STDC__) || defined(__cplusplus)

void	distNetInit (void);
int		distNetCtl (int function, int argument);
STATUS	distNetServAdd (int servId, DIST_SERV_FUNC servInput,
			char *servTaskName, int servNetPrio, int servTaskPrio,
			int servTaskStackSz);
STATUS	distNetServConfig (int servId, int taskPrio, int netPrio);
STATUS	distNetServUp (int servId);
STATUS	distNetServDown (int servId);
STATUS	distNetSend (DIST_NODE_ID nodeId, DIST_PKT *pPkt, UINT size,
			int timo, int prio);
STATUS	distNetIOVSend (DIST_NODE_ID nodeId, DIST_IOVEC *iov, int num,
			int timo, int prio);

#else	/* __STDC__ */

void	distNetInit ();
int		distNetCtl ();
STATUS	distNetServAdd ();
STATUS	distNetServConfig ();
STATUS	distNetServUp ();
STATUS	distNetServDown ();
STATUS	distNetSend ();
STATUS	distNetIOVSend ();

#endif	/* __STDC__ */

#ifdef __cplusplus
}
#endif

#endif  /* __INCdistNetLibPh */

