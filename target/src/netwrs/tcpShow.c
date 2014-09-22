/* tcpShow.c - TCP information display routines */

/* Copyright 1984 - 2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01g,10may02,kbw  making man page edits
01f,15oct01,rae  merge from truestack ver 01h, base 01d (VIRTUAL_STACK)
01e,14nov00,ham  fixed unnecessary dependency against tcp_debug(SPR 62272).
01d,14dec97,jdi  doc: cleanup.
01c,04aug97,kwb  fixed man page problems found in beta review
01b,20arp97,kwb  fixed man page format, spell check.
01a,08mar97,vin  written.
*/

/*
DESCRIPTION
This library provides routines to show TCP related
statistics.

Interpreting these statistics requires detailed knowledge of Internet
network protocols.  Information on these protocols can be found in
the following books:
.iP
.I "TCP/IP Illustrated Volume II, The Implementation,"
by Richard Stevens
.iP
.I "The Design and Implementation of the 4.4 BSD UNIX Operating System,"
by Leffler, McKusick, Karels and Quarterman
.LP
The tcpShowInit() routine links the TCP show facility into the VxWorks
system.  This is performed automatically if INCLUDE_TCP_SHOW is defined.

SEE ALSO: netLib, netShow
*/

#include "vxWorks.h"
#include "sys/types.h"
#include "netinet/in.h"
#include "netinet/in_systm.h"
#include "netinet/ip.h"
#include "netinet/ip_var.h"
#include "netinet/in_pcb.h"
#include "netinet/tcp.h"
#include "netinet/tcp_debug.h"
#include "netinet/tcp_fsm.h"
#include "netinet/tcp_timer.h"
#include "netinet/tcp_var.h"
#include "errno.h"
#include "string.h"
#include "stdio.h"

#define plural(num)	((num) > 1 ? "s": "")

#ifdef VIRTUAL_STACK
#include "netinet/vsLib.h"
#else

/* externs */

IMPORT struct inpcbhead		tcpcb;		/* defined in tcp_input.c */
IMPORT struct inpcbhead * 	_pTcpPcbHead;	/* defined in netShow.c */
#endif /* VIRTUAL_STACK */

/*
 * The available TCP states and the function pointer to the protocol-specific
 * print routine are common across all virtual stacks.
 */

IMPORT VOIDFUNCPTR		_pTcpPcbPrint;	/* defined in netShow.c */

/* forward declarations */

LOCAL void _tcpPcbPrint (struct inpcb * pInPcb);

/******************************************************************************
*
* tcpShowInit - initialize TCP show routines
*
* This routine links the TCP show facility into the VxWorks system.
* These routines are included automatically if INCLUDE_TCP_SHOW is defined.
*
* RETURNS: N/A
*/

void tcpShowInit (void)
    {
#ifdef VIRTUAL_STACK
    /* 
     * To avoid introducing a conflict with the "tcpcb" structure tag,
     * virtual stacks do not alias the head of the pcb list.
     */

    _pTcpPcbHead = &tcb;
#else
    _pTcpPcbHead = &tcpcb; 	/* initialize the pcb for generic show rtns */
#endif

    /*
     * Assigning the (shared) print routine for each virtual stack is
     * redundant, but unavoidable since it must be setup the first time.
     */

    _pTcpPcbPrint = _tcpPcbPrint;	/* initialize tcp specific print rtn */
    }

/*******************************************************************************
*
* _tcpPcbPrint - print TCP protocol control block info
*
* Prints TCP protocol control block information.
*
* RETURNS: N/A.
*
* NOMANUAL
*/

LOCAL void _tcpPcbPrint
    (
    struct inpcb * pInPcb	/* pointer to the protocol control block */
    )
    {
    struct tcpcb *	pTcpCb;	/* pointer to tcp Control block */

    pTcpCb = (struct tcpcb *) pInPcb->inp_ppcb;

    if (pTcpCb->t_state < 0 || pTcpCb->t_state >= TCP_NSTATES)
        printf(" %d", pTcpCb->t_state);
    else if (pTcpstates != NULL && pTcpstates[pTcpCb->t_state] != NULL)
        printf(" %s", pTcpstates[pTcpCb->t_state]);

    }

/*****************************************************************************
*
* tcpDebugShow - display debugging information for the TCP protocol
*
* This routine displays debugging information for the TCP protocol.
* To include TCP debugging facilities, define INCLUDE_TCP_DEBUG when
* building the system image.  To enable information gathering, turn on
* the SO_DEBUG option for the relevant socket(s).
*
* RETURNS: N/A
*/

void tcpDebugShow
    (
    int numPrint,	/* no. of entries to print, default (0) = 20 */
    int verbose		/* 1 = verbose */
    )
    {
    if (numPrint <=0)
	numPrint = TCP_DEBUG_NUM_DEFAULT;

    (*tcpReportRtn) (numPrint, verbose);
    }

/*****************************************************************************
*
* tcpstatShow - display all statistics for the TCP protocol
*
* This routine displays detailed statistics for the TCP protocol.
*
* RETURNS: N/A
*/

void tcpstatShow (void)
    {
    /*
     * I know it's ugly to use these macros in the following way but this is
     * the way Unix 'netstat' is written and I'd like to keep this part
     * look similar to that of Unix 'netstat'.
     */
#define	p(f, m)		printf(m, (int)tcpstat.f, plural(tcpstat.f))
#define	p2(f1, f2, m)	printf(m, (int)tcpstat.f1, plural(tcpstat.f1), \
				(int)tcpstat.f2, plural(tcpstat.f2))

#ifdef VIRTUAL_STACK
    printf ("TCP: (stack number %d)\n", myStackNum);
#else
    printf ("TCP:\n");
#endif /* VIRTUAL_STACK */
    p (tcps_sndtotal, "\t%d packet%s sent\n");
    p2 (tcps_sndpack,tcps_sndbyte,
       "\t\t%d data packet%s (%d byte%s)\n");
    p2 (tcps_sndrexmitpack, tcps_sndrexmitbyte,
       "\t\t%d data packet%s (%d byte%s) retransmitted\n");
    printf ("\t\t%d ack-only packet%s (%d delayed)\n",
            (int)tcpstat.tcps_sndacks,
            plural ((int)tcpstat.tcps_sndacks), (int)tcpstat.tcps_delack);
    p (tcps_sndurg, "\t\t%d URG only packet%s\n");
    p (tcps_sndprobe, "\t\t%d window probe packet%s\n");
    p (tcps_sndwinup, "\t\t%d window update packet%s\n");
    p (tcps_sndctrl, "\t\t%d control packet%s\n");
    p (tcps_rcvtotal, "\t%d packet%s received\n");
    p2 (tcps_rcvackpack, tcps_rcvackbyte, "\t\t%d ack%s (for %d byte%s)\n");
    p (tcps_rcvdupack, "\t\t%d duplicate ack%s\n");
    p (tcps_rcvacktoomuch, "\t\t%d ack%s for unsent data\n");
    p2 (tcps_rcvpack, tcps_rcvbyte,
       "\t\t%d packet%s (%d byte%s) received in-sequence\n");
    p2 (tcps_rcvduppack, tcps_rcvdupbyte,
       "\t\t%d completely duplicate packet%s (%d byte%s)\n");
    p2 (tcps_rcvpartduppack, tcps_rcvpartdupbyte,
       "\t\t%d packet%s with some dup. data (%d byte%s duped)\n");
    p2 (tcps_rcvoopack, tcps_rcvoobyte,
       "\t\t%d out-of-order packet%s (%d byte%s)\n");
    p2 (tcps_rcvpackafterwin, tcps_rcvbyteafterwin,
       "\t\t%d packet%s (%d byte%s) of data after window\n");
    p (tcps_rcvwinprobe, "\t\t%d window probe%s\n");
    p (tcps_rcvwinupd, "\t\t%d window update packet%s\n");
    p (tcps_rcvafterclose, "\t\t%d packet%s received after close\n");
    p (tcps_rcvbadsum, "\t\t%d discarded for bad checksum%s\n");
    p (tcps_rcvbadoff, "\t\t%d discarded for bad header offset field%s\n");
    printf ("\t\t%d discarded because packet too short\n",
        (int)tcpstat.tcps_rcvshort);
    p (tcps_connattempt, "\t%d connection request%s\n");
    p (tcps_accepts, "\t%d connection accept%s\n");
    p (tcps_connects, "\t%d connection%s established (including accepts)\n");
    p2 (tcps_closed, tcps_drops,
       "\t%d connection%s closed (including %d drop%s)\n");
    p (tcps_conndrops, "\t%d embryonic connection%s dropped\n");
    p2 (tcps_rttupdated, tcps_segstimed,
       "\t%d segment%s updated rtt (of %d attempt%s)\n");
    p (tcps_rexmttimeo, "\t%d retransmit timeout%s\n");
    p (tcps_timeoutdrop, "\t\t%d connection%s dropped by rexmit timeout\n");
    p (tcps_persisttimeo, "\t%d persist timeout%s\n");
    p (tcps_keeptimeo, "\t%d keepalive timeout%s\n");
    p (tcps_keepprobe, "\t\t%d keepalive probe%s sent\n");
    p (tcps_keepdrops, "\t\t%d connection%s dropped by keepalive\n");
    p (tcps_pcbcachemiss, "\t%d pcb cache lookup%s failed\n");

#undef p
#undef p2
    }
