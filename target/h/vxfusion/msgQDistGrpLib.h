/* msgQDistGrpLib.h - distributed message queue group library hdr (VxFusion) */

/* Copyright 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01a,11jun97,ur   written.
*/

#ifndef __INCmsgQDistGrpLibh
#define __INCmsgQDistGrpLibh

#include "vxWorks.h"
#include "vwModNum.h"

#ifdef __cplusplus
extern "C" {
#endif

/* defines */

#define S_msgQDistGrpLib_NAME_TOO_LONG	(M_msgQDistGrpLib | 1) /* error code */
#define S_msgQDistGrpLib_INVALID_OPTION	(M_msgQDistGrpLib | 2) /* error code */
#define S_msgQDistGrpLib_DATABASE_FULL	(M_msgQDistGrpLib | 3) /* error code */
#define S_msgQDistGrpLib_NO_MATCH		(M_msgQDistGrpLib | 4) /* error code */

/* typedefs */

typedef int DIST_GRP_OPT;		/* distributed msgQ group options */

/* function declarations */

#if defined(__STDC__) || defined(__cplusplus)

void     msgQDistGrpLibInit (void);
STATUS   msgQDistGrpInit (int sizeLog2);
MSG_Q_ID msgQDistGrpAdd (char *grpName, MSG_Q_ID msgQId, DIST_GRP_OPT options);
STATUS   msgQDistGrpDelete (char *grpName, MSG_Q_ID msgQId);

#else   /* __STDC__ */

void     msgQDistGrpLibInit ();
STATUS   msgQDistGrpInit ();
MSG_Q_ID msgQDistGrpAdd ();
STATUS   msgQDistGrpDelete ();

#endif  /* __STDC__ */


#ifdef __cplusplus
}
#endif

#endif	/* __INCmsgQDistGrpLibh */

