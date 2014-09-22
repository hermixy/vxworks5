/* distTBufLibP.h - telegram buffer library header (VxFusion) */

/* Copyright 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01b,29oct98,drm  moved distTBufLibInit() to private header file
01a,12aug98,drm  initial version - portions moved from private header file
*/

#ifndef __INCdistTBufLibh
#define __INCdistTBufLibh

#include "vxWorks.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* defines */

#ifdef DIST_DIAGNOSTIC
#define tBufHeader			tBufGen.tBufGenHeader
#define tBufHdrHeader		tBufGen.tBufGenHeader
#endif

#define DIST_TBUF_FLAG_HDR			0x01			 /* telegram is header */
#define DIST_TBUF_FLAG_MF			0x02             /* more fragments */
#define DIST_TBUF_FLAG_BROADCAST	0x04             /* broadcast telegram */

#define DIST_TBUF_IS_HDR(pTBuf) \
	((pTBuf)->tBufFlags & DIST_TBUF_FLAG_HDR)		 /* check if hdr flag set */
#define DIST_TBUF_HAS_MF(pTBuf) \
	((pTBuf)->tBufFlags & DIST_TBUF_FLAG_MF)         /* check if MF flag set */
#define DIST_TBUF_IS_BROADCAST(pTBuf) \
	((pTBuf)->tBufFlags & DIST_TBUF_FLAG_BROADCAST)  /* check if bcast flg set*/

#define DIST_TBUF_TTYPE_DTA			0      /* data */
#define DIST_TBUF_TTYPE_ACK			1      /* acknowledge */
#define DIST_TBUF_TTYPE_BDTA		2      /* broadcast data */
#define DIST_TBUF_TTYPE_BACK		3      /* broadcast acknowledge */
#define DIST_TBUF_TTYPE_BOOTSTRAP	4      /* bootstrap message */
#define DIST_TBUF_TTYPE_NACK		5      /* negative acknowledge */

/* typedefs */

typedef struct _DIST_TBUF_GEN	/* DIST_TBUF_GEN */
	{
	struct _DIST_TBUF_GEN	*pTBufGenNext; /* next TBuf */
	struct _DIST_TBUF_GEN	*pTBufGenLast; /* previous TBuf */
#ifdef DIST_DIAGNOSTIC
	BOOL					tBufGenHeader;
#endif
	} DIST_TBUF_GEN;

typedef struct	/* DIST_TBUF */
	{
	DIST_TBUF_GEN			tBufGen;    /* TBufGen struct */
	void					*pTBufData; /* pointer to the data */
	uint16_t				tBufId;     /* id of the packet */
	uint16_t				tBufAck;    /* id of packet last received and */
                                        /* ackowledged without error */
	uint16_t				tBufSeq;    /* sequence number of the fragment */
	uint16_t				tBufNBytes; /* number of non-network header data */
                                        /* bytes */ 
	uint16_t				tBufType;   /* type of telegram */
	uint16_t				tBufFlags;  /* telegrams flags */
	} DIST_TBUF;

/* function declarations */

#if defined(__STDC__) || defined(__cplusplus)

DIST_TBUF *distTBufAlloc (void);
void distTBufFree (DIST_TBUF *pTBuf);

#else	/* __STDC__ */

DIST_TBUF *distTBufAlloc ();
void distTBufFree ();

#endif	/* __STDC__ */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __INCdistTBufLibh */

