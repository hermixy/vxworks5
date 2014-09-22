/* distStatLib.c - distributed objects statistics library (VxFusion) */

/* Copyright 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01d,24may99,drm  added vxfusion prefix to VxFusion related includes
01c,30oct98,drm  documentation updates
01b,11sep98,drm  moved distDump() to distLib.c
01a,04sep97,ur   written.
*/

/*
DESCRIPTION
This library maintains the following C structure:

.CS
typedef struct                  /@ DIST_STAT @/
    {
    /@ general @/
    u_long tBufShortage;        /@ times no more tBufs were available @/
    u_long memShortage;         /@ times no memory was available @/

    /@ interface layer @/
    u_long ifInReceived;        /@ incoming telegrams received by if layer @/
    u_long ifOutReceived;       /@ outgoing telegrams received by if layer @/
    u_long ifInDiscarded;       /@ incoming telegrams discarded by if layer @/
    u_long ifOutDiscarded;      /@ outgoing telegrams discarded by if layer @/
    u_long ifInLength;          /@ incoming telegrams discarded: wrong length @/

    /@ net layer @/
    u_long netInReceived;       /@ incoming pkts received by net layer @/
    u_long netOutReceived;      /@ outgoing pkts received by net layer @/
    u_long netInDiscarded;      /@ incoming pkts discarded by net layer @/
    u_long netOutDiscarded;     /@ outgoing pkts discarded by net layer @/
    u_long netReassembled;      /@ pkts reassembled @/

    /@ node layer @/
    u_long nodeOutReceived;     /@ outgoing pkts received @/
    u_long nodeInDiscarded;     /@ pkts discarded @/
    u_long nodeAcked;           /@ pkts aknowleded @/
    u_long nodeNotAlive;        /@ pkts tried to send to node not alive @/
    u_long nodePktResend;       /@ pkts resent @/
    u_long nodeFragResend;      /@ fragments resent @/
    u_long nodeDBNoMatch;       /@ node not found in node DB @/
    u_long nodeDBFatal;         /@ fatal error in node DB @/

    /@ msg queue objects @/
    u_long msgQInDiscarded;     /@ #pkts discarded @/
    u_long msgQInTooShort;      /@ #pkts too short @/

    /@ msg queue group objects @/
    u_long msgQGrpInDiscarded;  /@ #pkts discarded @/
    u_long msgQGrpInTooShort;   /@ #pkts too short @/

    /@ distributed name database @/
    u_long dndbInReceived;      /@ #pkts received by DNDB @/
    u_long dndbInDiscarded;     /@ #pkts discarded @/

    /@ incorporation protocol @/
    u_long incoInDiscarded;     /@ #pkts discarded @/

    } DIST_STAT;
.CE

With the exception of the interface layer values, the rest of the values are
incremented by the distributed objects product at runtime.  The interface
layer values are incremented by the installed interface adapter.

When the distIfShow library routine distIfShow() is called, it will display
some of the interface layer values.

INCLUDE FILES: distStatLib.h

SEE ALSO: distIfShow
*/

#include "vxWorks.h"
#include "stdio.h"
#include "ctype.h"
#include "string.h"
#include "vxfusion/distStatLib.h"
#include "vxfusion/private/distStatLibP.h"

/* global variables */

DIST_STAT	distStat;

/*******************************************************************************
*
* distStatLibInit - initialize the distributed objects statistics library
*
* NOMANUAL
*/

void distStatLibInit ()
	{
	}

/*******************************************************************************
*
* distStatInit - initialize the statistics table
*
* NOMANUAL
*/

void distStatInit ()
	{
	bzero ((char *) &distStat, sizeof (DIST_STAT));
	}

/*******************************************************************************
*
* distStatShow - show the statistics table
*
* NOMANUAL
*/

void distStatShow ()
	{
	printf ("Gen:  tBufShortage = %ld, memShortage = %ld\n", 
			distStat.tBufShortage,
			distStat.memShortage);

	printf ("If:   ifInReceived = %ld, ifOutReceived = %ld\n",
			distStat.ifInReceived,
			distStat.ifOutReceived);
	printf ("      ifInDiscarded = %ld, ifOutDiscarded = %ld\n",
			distStat.ifInDiscarded,
			distStat.ifOutDiscarded);
	printf ("      ifInLength = %ld\n", distStat.ifInLength);

	printf ("Net:  netInReceived = %ld, netOutReceived = %ld\n",
			distStat.netInReceived,
			distStat.netOutReceived);
	printf ("      netInDiscarded = %ld, netOutDiscarded = %ld\n",
			distStat.netInDiscarded,
			distStat.netOutDiscarded);
	printf ("      netReassembled = %ld\n", distStat.netReassembled);

	printf ("Node: nodeOutReceived = %ld, nodeInDiscarded = %ld\n",
			distStat.nodeOutReceived,
			distStat.nodeInDiscarded);
	printf ("      nodeAcked = %ld, nodeNotAlive = %ld\n",
			distStat.nodeAcked,
			distStat.nodeNotAlive);
	printf ("      nodeFragResend = %ld, nodePktResend = %ld\n",
			distStat.nodeFragResend, distStat.nodePktResend);
	printf ("      nodeDBNoMatch = %ld, nodeDBFatal = %ld\n",
			distStat.nodeDBNoMatch,
			distStat.nodeDBFatal);

	printf ("MsgQ: msgQInDiscarded = %ld, msgQInTooShort = %ld\n",
			distStat.msgQInDiscarded,
			distStat.msgQInTooShort);

	printf ("Grp:  msgQGrpInDiscarded = %ld, msgQGrpInTooShort = %ld\n",
			distStat.msgQGrpInDiscarded,
			distStat.msgQGrpInTooShort);

	printf ("DNDB: dndbInReceived = %ld, dndbInDiscarded = %ld\n",
			distStat.dndbInReceived,
			distStat.dndbInDiscarded);

	printf ("Inco: incoInDiscarded = %ld\n", distStat.incoInDiscarded);
	}

