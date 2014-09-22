/* distStatLib.h - statistics library header (VxFusion) */

/* Copyright 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01b,11sep98,drm  removed forward function declarations
01a,01sep97,ur   written.
*/

#ifndef __INCdistStatLibh
#define __INCdistStatLibh

#ifdef __cplusplus
extern "C" {
#endif

#include "vxWorks.h"

typedef struct					/* DIST_STAT */
	{
	/* generel */
	u_long tBufShortage;		/* times no more tBufs were available */
	u_long memShortage;			/* times no memory was available */

	/* interface layer */
	u_long ifInReceived;		/* incoming telegrams received by if layer */
	u_long ifOutReceived;		/* outgoing telegrams received by if layer */
	u_long ifInDiscarded;		/* incoming telegrams discarded by if layer */
	u_long ifOutDiscarded;		/* outgoing telegrams discarded by if layer */
	u_long ifInLength;			/* incoming telegrams discarded: wrong length */

	/* net layer */
	u_long netInReceived;		/* incoming pkts received by net layer */
	u_long netOutReceived;		/* outgoing pkts received by net layer */
	u_long netInDiscarded;		/* incoming pkts discarded by net layer */
	u_long netOutDiscarded;		/* outgoing pkts discarded by net layer */
	u_long netReassembled;		/* pkts reassembled */

	/* node layer */
	u_long nodeOutReceived;		/* outgoing pkts received */
	u_long nodeInDiscarded;		/* pkts discarded */
	u_long nodeAcked;			/* pkts aknowleded */
	u_long nodeNotAlive;		/* pkts tried to send to node not alive */
	u_long nodePktResend;		/* pkts resent */
	u_long nodeFragResend;		/* fragments resent */
	u_long nodeDBNoMatch;		/* node not found in node DB */
	u_long nodeDBFatal;			/* fatal error in node DB */

	/* msg queue objects */
	u_long msgQInDiscarded;		/* #pkts discarded */
	u_long msgQInTooShort;		/* #pkts too short */

	/* msg queue group objects */
	u_long msgQGrpInDiscarded;	/* #pkts discarded */
	u_long msgQGrpInTooShort;	/* #pkts too short */

	/* distributed name database */
	u_long dndbInReceived;		/* #pkts received by DNDB */
	u_long dndbInDiscarded;		/* #pkts discarded */

	/* incorporation protocol */
	u_long incoInDiscarded;		/* #pkts discarded */

	} DIST_STAT;

extern DIST_STAT distStat;

#endif __INCdistStatLibh
