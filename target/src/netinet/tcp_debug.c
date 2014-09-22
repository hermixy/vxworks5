/* tcp_debug.c - TCP debug routines */

/* Copyright 1984 - 2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
 * Copyright (c) 1982, 1986, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)tcp_debug.c	8.1 (Berkeley) 6/10/93
 */

/*
modification history
--------------------
01d,20may02,vvv  moved tcpstates definition and pTcpstates initialization to
                 tcpLib.c (SPR #62272)
01c,12oct01,rae  merge from truestack (VIRTUAL_STACK, printf formats)
01b,14nov00,ham  fixed messed tcpstates declaration(SPR 62272).
01a,03mar96,vin  created from BSD4.4 stuff,integrated with 02l of tcp_debug.c
*/

/*
DESCRIPTION
*/

/* includes */

#define TCPDEBUG
#ifdef TCPDEBUG
/* load symbolic names */
#define	PRUREQUESTS
#define	TCPSTATES
#define	TCPTIMERS
#define	TANAMES
#endif

#include "vxWorks.h"
#include "stdioLib.h"
#include "net/systm.h"
#include "net/unixLib.h"
#include "net/mbuf.h"
#include "sys/socket.h"
#include "net/socketvar.h"
#include "net/protosw.h"
#include "errno.h"

#include "net/route.h"
#include "net/if.h"

#include "netinet/in.h"
#include "netinet/in_pcb.h"
#include "netinet/in_systm.h"
#include "netinet/ip.h"
#include "netinet/ip_var.h"
#include "netinet/tcp.h"
#include "netinet/tcp_fsm.h"
#include "netinet/tcp_seq.h"
#include "netinet/tcp_timer.h"
#include "netinet/tcp_var.h"
#include "netinet/tcpip.h"
#include "netinet/tcp_debug.h"

/* globals */

#ifdef VIRTUAL_STACK
#include "netinet/vsLib.h"
#else
int	tcpDebugCons = 0;
BOOL	tcpDebugConsVerbose = TRUE;
#endif

/* locals */

#ifndef VIRTUAL_STACK
LOCAL struct tcp_debug	tcp_debug[TCP_NDEBUG];
LOCAL int	tcp_debx;
LOCAL int	tcp_debug_valid = 0;
#endif

/* forward declarations */

LOCAL void tcp_trace (short act, short ostate, struct tcpcb *tp,
    struct tcpiphdr *ti, int req);
LOCAL void tcpDebugPrint (struct tcp_debug *td, BOOL verbose);
LOCAL void tcp_report (int num, BOOL verbose);

void tcpTraceInit (void)
    {
    tcpTraceRtn = (VOIDFUNCPTR) tcp_trace;
    tcpReportRtn = (VOIDFUNCPTR) tcp_report;

#ifdef VIRTUAL_STACK
    /*
     * Assign (former) global variables previously initialized by the compiler.
     * Setting 0 is repeated for clarity - the vsLib.c setup zeroes all values.
     */

    tcpDebugCons = 0;
    tcpDebugConsVerbose = TRUE;
    tcp_debug_valid = 0;
#endif
    }

LOCAL void tcp_trace
    (
    short act,
    short ostate,
    struct tcpcb *tp,
    struct tcpiphdr *ti,
    int req
    )
    {
#ifdef VIRTUAL_STACK
    /* Virtual stack requires different variable name from structure tag. */

    struct tcp_debug *td = &tcp_tracelog[tcp_debx++];
#else
    struct tcp_debug *td = &tcp_debug[tcp_debx++];
#endif

    if (tcp_debug_valid < TCP_NDEBUG)
        tcp_debug_valid++;
    if (tcp_debx == TCP_NDEBUG)
	tcp_debx = 0;

    td->td_time = iptime();
    td->td_act = act;
    td->td_ostate = ostate;
    td->td_req = req;

    if (tp)
	{
	td->td_cb = *tp;
	td->td_tcb = (caddr_t) tp;
	}
    else
	td->td_tcb = NULL;

    if (ti)
	{
	td->td_ti = *ti;
	td->td_tiphdr = (caddr_t) ti;
	}
    else
	td->td_tiphdr = NULL;

    if (tcpDebugCons)
        tcpDebugPrint (td, tcpDebugConsVerbose);
    }

LOCAL void tcpDebugPrint
    (
    struct tcp_debug *td,
    BOOL verbose
    )
    {
    struct tcpcb *tp;
    struct tcpiphdr *ti;
    tcp_seq seq, ack;
    int len;

    printf ("%lu  %s  %s", td->td_time, tanames [td->td_act],
	pTcpstates [td->td_ostate]);

    if (!verbose)
        printf ("\n");
    else
	{
        if (td->td_tcb != NULL)
	    printf(" -> %s , &tcpcb=%x\n", pTcpstates[td->td_cb.t_state],
		(UINT32) td->td_tcb);
	switch (td->td_act)
	    {
	    case TA_INPUT:
	    case TA_OUTPUT:
	    case TA_DROP:
		if (td->td_tiphdr == NULL)
		    break;

                ti = &td->td_ti;
		if (td->td_act == TA_OUTPUT)
		    {
		    seq = ntohl(ti->ti_seq);
		    ack = ntohl(ti->ti_ack);
		    len = ntohs(ti->ti_len);
		    }
                else
		    {
		    seq = ti->ti_seq;
		    ack = ti->ti_ack;
		    len = ti->ti_len;
		    }

		if (td->td_act == TA_OUTPUT)
		    len -= sizeof (struct tcphdr);

		if (len)
			printf("\tseq=[%lx..%lx]", seq, seq+len);
		else
			printf("\tseq=%lx", seq);
		printf(" ack=%lx, urp=%x, ", ack, ti->ti_urp);

		if (ti->ti_flags)
		    {
		    printf ("flags=<");
		    if (ti->ti_flags & TH_FIN)
			printf (" FIN");
		    if (ti->ti_flags & TH_SYN)
			printf (" SYN");
		    if (ti->ti_flags & TH_RST)
			printf (" RST");
		    if (ti->ti_flags & TH_PUSH)
			printf (" PUSH");
		    if (ti->ti_flags & TH_ACK)
			printf (" ACK");
		    if (ti->ti_flags & TH_URG)
			printf (" URG");
		    printf (" >\n");
		    }

		break;

	    case TA_USER:
		printf("%s", prurequests[td->td_req&0xff]);
		if ((td->td_req & 0xff) == PRU_SLOWTIMO)
			printf("<%s>\n", tcptimers[td->td_req>>8]);
                break;
	    }

	if (td->td_tcb == NULL)
	    {
	    printf ("\n\n");
	    return;
	    }

        tp = &td->td_cb;
	printf("\trcv_(nxt,wnd,up)  = (%lx,%lx,%lx)\n",
	    tp->rcv_nxt, tp->rcv_wnd, tp->rcv_up);
	printf("\tsnd_(una,nxt,max) = (%lx,%lx,%lx)\n",
	    tp->snd_una, tp->snd_nxt, tp->snd_max);
	printf("\tsnd_(wl1,wl2,wnd) = (%lx,%lx,%lx)\n\n",
	    tp->snd_wl1, tp->snd_wl2, tp->snd_wnd);
        }
    }

LOCAL void tcp_report 
    (
    int num,		/* number of entries to print */
    BOOL verbose
    )
    {
    int s = splnet();	/* accessing structures that change async */

    num = max (num, 0);
    num = tcp_debx - min (num, tcp_debug_valid);

    if (num < 0)		/* adjust for warap-around */
	num += (TCP_NDEBUG + 1);

    for (; num != tcp_debx; num++)
	{
	if (num >= TCP_NDEBUG)	/* start over at beginning of array */
	    num = 0;

#ifdef VIRTUAL_STACK
    /* Virtual stack requires different variable name from structure tag. */

        tcpDebugPrint (&tcp_tracelog [num], verbose);
#else
        tcpDebugPrint (&tcp_debug [num], verbose);
#endif
        }

    splx (s);
    }
