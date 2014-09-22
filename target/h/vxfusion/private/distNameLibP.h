/* distNameLibP.h - distributed name database private header (VxFusion) */

/* Copyright 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01d,24may99,drm  added vxfusion to VxFusion related includes
01c,12aug98,drm  changed distTBufLibP.h to distTBufLib.h
01b,09may98,ur   removed 8 bit node id restriction
01a,10jun97,ur   written.
*/

#ifndef __INCdistNameLibPh
#define __INCdistNameLibPh

#ifdef __cplusplus
extern "C" {
#endif

#include "hashLib.h"
#include "vxfusion/distNameLib.h"
#include "vxfusion/distTBufLib.h"
#include "vxfusion/private/distPktLibP.h"
#include "vxfusion/private/distNodeLibP.h"
#include "vxfusion/private/distObjLibP.h"

/* defines */

#define DIST_DNDB_SERV_NAME				"tServDndb"
#define DIST_DNDB_SERV_TASK_PRIO		50
#define DIST_DNDB_SERV_TASK_STACK_SZ	5000

#define DIST_DNDB_SERV_NET_PRIO			0

#define DIST_NAME_MAX_LENGTH	19
#define DIST_VALUE_MAX_LENGTH	8

#define DIST_PKT_DNDB_PRIO		(servTable[DIST_ID_DNDB_SERV].servNetPrio)

#define DIST_PKT_TYPE_DNDB_ADD	0
#define DIST_PKT_TYPE_DNDB_RM	1

#define DIST_NAME_STATUS_OK					OK
#define DIST_NAME_STATUS_PROTOCOL_ERROR		1

#define distNameLclLock()		semTake (&distNameDbLock, WAIT_FOREVER)
#define distNameLclUnlock()		semGive (&distNameDbLock)
#define distNameLclBlockOnAdd()	semTake (&distNameDbUpdate, WAIT_FOREVER)
#define distNameLclSigAdd()		semFlush (&distNameDbUpdate)

/* typedefs */

typedef union			/* DIST_NAME_TYPE_ALL */
	{
	uint8_t				uint8;
	uint16_t			uint16;
	uint32_t			uint32;
	uint32_t			uint64[2];
	float				float32;
	double				float64;
	uint8_t				field[DIST_VALUE_MAX_LENGTH];
	DIST_OBJ_UNIQ_ID	objUniqId;		/* for complex objects */
	} DIST_NAME_TYPE_ALL;

typedef union			/* DIST_NAME_DB_VALUE */
	{
	uint8_t				uint8;
	uint16_t			uint16;
	uint32_t			uint32;
	uint32_t			uint64[2];
	float				float32;
	double				float64;
	uint8_t				field[DIST_VALUE_MAX_LENGTH];
	MSG_Q_ID			msgQId;
	} DIST_NAME_DB_VALUE;

typedef struct			/* DIST_NAME_DB_NODE */
	{
	HASH_NODE			node;
	DIST_NAME_TYPE		type;
	int					valueLen;
	DIST_NAME_DB_VALUE	value;
	char				symName[DIST_NAME_MAX_LENGTH + 1];
	} DIST_NAME_DB_NODE;

/* packets sent by name DNDB */

typedef struct			/* DIST_PKT_DNDB_ADD */
	{
	DIST_PKT		dndbAddHdr;
	DIST_NAME_TYPE	dndbAddType;
	uint8_t			dndbAddValueLen;
	uint8_t			dndbAddNameLen;
	__DIST_PKT_HDR_END__
	/* value follows */
	/* name follows  */
	} DIST_PKT_DNDB_ADD;

typedef struct			/* DIST_PKT_DNDB_RM */
	{
	DIST_PKT		dndbRmHdr;
	uint8_t			dndbRmNameLen;
	__DIST_PKT_HDR_END__
	/* name follows */
	} DIST_PKT_DNDB_RM;

/* some helping structures */

typedef struct			/* DIST_NAME_MATCH */
	{
	void			*pMatchValue;
	DIST_NAME_TYPE	matchType;
	} DIST_NAME_MATCH;

typedef struct			/* DIST_NAME_BURST */
	{
	DIST_NODE_ID	burstNodeId;
	STATUS			burstStatus;
	} DIST_NAME_BURST;

/* function declarations */

#if defined(__STDC__) || defined(__cplusplus)

DIST_NAME_DB_NODE	*distNameEach (FUNCPTR routine, int routineArg);
STATUS				distNameBurst (DIST_NODE_ID nodeId);

#else   /* __STDC__ */

DIST_NAME_DB_NODE	*distNameEach ();
STATUS				distNameBurst ();

#endif  /* __STDC__ */

#ifdef __cplusplus
}
#endif

#endif /* __INCdistNameLibPh */

