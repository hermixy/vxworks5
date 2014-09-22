/* msgQDistLib.h - distributed message queue library header (VxFusion) */

/* Copyright 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,18feb99,drm  Added error code S_msgQDistLib_OVERALL_TIMEOUT
01b,12aug98,drm  removed #include of private files
01a,06jun97,ur   written.
*/

#ifndef __INCmsgQDistLibh
#define __INCmsgQDistLibh

#include "vxWorks.h"
#include "vwModNum.h"
#include "msgQLib.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* defines */

#define DIST_MSG_PRI_0	((VX_TYPE_DIST_OBJ << 8) | 0x0)	/* highest priority */
#define DIST_MSG_PRI_1	((VX_TYPE_DIST_OBJ << 8) | 0x1) /* priority 1 */
#define DIST_MSG_PRI_2	((VX_TYPE_DIST_OBJ << 8) | 0x2) /* priority 2 */
#define DIST_MSG_PRI_3	((VX_TYPE_DIST_OBJ << 8) | 0x3) /* priority 3 */
#define DIST_MSG_PRI_4	((VX_TYPE_DIST_OBJ << 8) | 0x4) /* priority 4 */
#define DIST_MSG_PRI_5	((VX_TYPE_DIST_OBJ << 8) | 0x5) /* priority 5 */
#define DIST_MSG_PRI_6	((VX_TYPE_DIST_OBJ << 8) | 0x6) /* priority 6 */
#define DIST_MSG_PRI_7	((VX_TYPE_DIST_OBJ << 8) | 0x7)	/* lowest priority */

#define	S_msgQDistLib_INVALID_PRIORITY		(M_msgQDistLib | 1) /* error code */
#define	S_msgQDistLib_INVALID_MSG_LENGTH	(M_msgQDistLib | 2) /* error code */
#define S_msgQDistLib_INVALID_TIMEOUT		(M_msgQDistLib | 3) /* error code */
#define S_msgQDistLib_NOT_GROUP_CALLABLE	(M_msgQDistLib | 4) /* error code */
#define S_msgQDistLib_RMT_MEMORY_SHORTAGE	(M_msgQDistLib | 5) /* error code */
#define S_msgQDistLib_OVERALL_TIMEOUT		(M_msgQDistLib | 6) /* error code */

/* function declarations */

#if defined(__STDC__) || defined(__cplusplus)

void		msgQDistLibInit (void);
STATUS		msgQDistInit (int msgQDistMax);
MSG_Q_ID	msgQDistCreate (int maxMsgs, int maxMsgLength, int options);
STATUS		msgQDistSend (MSG_Q_ID msgQId, char *buffer, UINT nBytes,
				int msgQTimeout, int overallTimeout, int priority);
int			msgQDistReceive (MSG_Q_ID msgQId, char *buffer,
				UINT maxNBytes, int msgQTimeout, int overallTimeout);
int			msgQDistNumMsgs (MSG_Q_ID msgQId, int overallTimeout);

#else   /* __STDC__ */

void		msgQDistLibInit ();
STATUS		msgQDistInit ();
MSG_Q_ID	msgQDistCreate ();
STATUS		msgQDistSend ();
int			msgQDistReceive ();
int			msgQDistNumMsgs ();

#endif  /* __STDC__ */


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif	/* __INCmsgQDistLibh */

