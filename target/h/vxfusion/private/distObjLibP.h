/* distObjLibP.h - distributed objects library header file */

/* Copyright 1984-1992 Wind River Systems, Inc. */

/*
modification history
--------------------
01d,28may99,drm  Changing DIST_OBJ_VERIFY to check that the ID is a
                 distributed message queue ID.
01c,24may99,drm  added vxfusion to VxFusion related includes
01b,08may98,ur   removed 8 bit node id restriction
01a,12Jun97,ur   written.
*/

#ifndef __INCdistObjLibPh
#define __INCdistObjLibPh

#include "vxWorks.h"
#include "hashLib.h"
#include "vxfusion/distLib.h"
#include "private/distObjTypeP.h"

#ifdef __cplusplus
extern "C" {
#endif

#define distInqLockInit()	semBInit (&distInqLock, SEM_Q_PRIORITY, SEM_FULL)
#define distInqLock()		semTake (&distInqLock, WAIT_FOREVER)
#define distInqUnlock()		semGive (&distInqLock)

#define DIST_INQ_HASH_TBL_SZ_LOG2	7	/* 2^7 = 128 */

#define DIST_MSG_Q_INQ				DIST_ID_MSG_Q_SERV
#define DIST_MSG_Q_GRP_INQ			DIST_ID_MSG_Q_GRP_SERV

#define TBL_IX_TO_DIST_OBJ_ID(tblIx) \
	((DIST_OBJ_ID) TBL_IX_TO_DIST_MSG_Q_ID (tblIx))

#define DIST_MSG_Q_GRP_ID_TO_DIST_OBJ_ID(grpId) \
	((DIST_OBJ_ID) DIST_MSG_Q_GRP_ID_TO_DIST_MSG_Q_ID (grpId))

#define DIST_OBJ_ID_TO_DIST_MSG_Q_ID(objId) \
	((DIST_MSG_Q_ID) (objId))

#define DIST_MSG_Q_ID_TO_DIST_OBJ_ID(dMsgQId) \
	((DIST_OBJ_ID) (dMsgQId))

#define DIST_OBJ_VERIFY(msgQId)	 (ID_IS_DISTRIBUTED (msgQId) ? OK : ERROR )

#define DIST_OBJ_TYPE_GRP		0x1

#define IS_DIST_OBJ_LOCAL(pObjNode) \
	((pObjNode)->objNodeReside == distNodeLocalGetId())

typedef uint32_t		DIST_OBJ_ID;		/* DIST_OBJ_ID */

typedef struct								/* DIST_OBJ_NODE */
	{
	DIST_NODE_ID		objUniqReside;		/* node where object resides */
	DIST_OBJ_ID			objUniqId;			/* id of object */
	/* Note: <objNodeReside> is only valid, if <objNodeId> is a non-group id */
	} DIST_OBJ_UNIQ_ID;

typedef struct								/* DIST_OBJ_NODE */
	{
	u_long				objNodeVerify;		/* for verification */
	int					objNodeType;		/* type of object */
	DIST_OBJ_UNIQ_ID	objNodeUniqId;		/* system unique object id */
	} DIST_OBJ_NODE;

#define objNodeReside	objNodeUniqId.objUniqReside
#define objNodeId		objNodeUniqId.objUniqId

typedef uint32_t		DIST_INQ_ID;		/* DIST_INQ_ID */
typedef int				DIST_INQ_TYPE;		/* DIST_INQ_TYPE */

typedef struct							/* DIST_INQ */
	{
	HASH_NODE		inqHNode;
	DIST_INQ_ID		inqId;
	DIST_INQ_TYPE	inqType;
	} DIST_INQ;

#if defined(__STDC__) || defined(__cplusplus)

DIST_OBJ_NODE	*distObjNodeGet (void);
STATUS			distInqInit (int hashTblSzLog2);
DIST_INQ_ID		distInqRegister (DIST_INQ *pInqRegister);
DIST_INQ		*distInqFind (DIST_INQ_ID InqIdFind);
DIST_INQ_ID		distInqGetId (DIST_INQ *pInqGetIdOf);
void			distInqCancel (DIST_INQ *pInqCancel);

#else /* __STDC__ */

DIST_OBJ_NODE	*distObjNodeGet ();
STATUS			distInqInit ();
DIST_INQ_ID		distInqRegister ();
DIST_INQ		*distInqFind ();
DIST_INQ_ID		distInqGetId ();
void			distInqCancel ();

#endif /* __STDC__ */

#ifdef __cplusplus
}
#endif

#endif	/* __INCdistObjLibPh */

