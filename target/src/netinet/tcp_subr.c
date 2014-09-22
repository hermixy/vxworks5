/* tcp_subr.c - TCP routines */

/* Copyright 1984 - 2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
 * Copyright (c) 1982, 1986, 1988, 1990, 1993, 1995
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
 *	@(#)tcp_subr.c	8.2 (Berkeley) 5/24/95
 */

/*
modification history
--------------------
01e,18apr02,vvv  trigger slow-start in response to ICMP fragmentation-needed
		 message (SPR #75058)
01d,12oct01,rae  merge from truestack ver 01o, base 01c (SPRs 70049,
                 70330, 34005, 31244, etc)
01c,31mar97,vin  modified for hash look ups for pcbs(FREEBSD 2.2.1).
01b,30oct96,vin  changed tcp_template to return mbuf * instead of tcpiphdr.
		 changed m_free(dtom(...)) to m_free(tp->t_template).
01a,03mar96,vin  created from BSD4.4 stuff,integrated with 02i.
*/

/*
DESCRIPTION
*/

#include "vxWorks.h"
#include "net/systm.h"
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
#include "netinet/ip_icmp.h"
#include "netinet/tcp.h"
#include "netinet/tcp_fsm.h"
#include "netinet/tcp_seq.h"
#include "netinet/tcp_timer.h"
#include "netinet/tcp_var.h"
#include "netinet/tcpip.h"

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET
#include "wvNetLib.h"
#endif
#endif

#ifdef VIRTUAL_STACK
#include "netinet/vsLib.h"
#else

/*
 * Target size of TCP PCB hash table. Will be rounded down to a prime
 * number.
 */
#ifndef TCBHASHSIZE
#define TCBHASHSIZE     128
#endif

/* patchable/settable parameters for tcp */
int 	tcp_mssdflt = TCP_MSS;
int 	tcp_rttdflt = TCPTV_SRTTDFLT / PR_SLOWHZ;
int	tcp_do_rfc1323 = 1;
u_short tcp_pcbhashsize = TCBHASHSIZE;

extern	struct inpcb * tcp_last_inpcb;

#endif /* VIRTUAL_STACK */
extern void _remque();
extern int random();

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET
    /* Set common fields of event identifiers for this module. */

LOCAL UCHAR wvNetModuleId = WV_NET_TCPSUBR_MODULE;  /* Value for tcp_subr.c */
LOCAL UCHAR wvNetLocalFilter = WV_NET_NONE;     /* Available event filter */

LOCAL ULONG wvNetEventId;       /* Event identifier: see wvNetLib.h */
#endif    /* INCLUDE_WVNET */
#endif


/* globals */

unsigned long (*pTcpRandHook)(void) = (unsigned long (*)(void))random;

/*******************************************************************************
*
* tcpRandHookAdd - Changes pTcpRandHook to a user defined functioned
*
* The user supplies a random number generator to tcp_init via pTcpRandHook.
*
* RETURNS: OK if the function pointer is not null.
*/
int tcpRandHookAdd
    (
    FUNCPTR pTcpRandFunc
    )
    {
    if (pTcpRandFunc != NULL)
	{
        pTcpRandHook = (unsigned long (*)(void))pTcpRandFunc;
	return (OK);
	}
    else
    	return (ERROR);
    }

/*******************************************************************************
*
* tcpRandHookDelete - Changes pTcpRandHook back to the default setting
*
* This routine is not needed.
*
*/

void tcpRandHookDelete (void)
    {
    /*
     * XXX
     * Since random() returns 31bit random number, the highest bit
     * is not set, therefore casting unsigned long will not affect
     * against caller badly.    
     */
    pTcpRandHook = (unsigned long (*)(void))random;	/* XXX */
    }


/*
 * Tcp initialization
 */
void
tcp_init()
{
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_VERBOSE event */
    WV_NET_MARKER_0 (NET_AUX_EVENT, WV_NET_VERBOSE, 39, 9,
                     WV_NETEVENT_TCPINIT_START)
#endif  /* INCLUDE_WVNET */
#endif

	tcp_iss = pTcpRandHook();
        LIST_INIT(&tcb);
        tcbinfo.listhead = &tcb;
        tcbinfo.hashbase = hashinit(tcp_pcbhashsize, MT_PCB, &tcbinfo.hashmask);
	if (max_protohdr < sizeof(struct tcpiphdr))
		max_protohdr = sizeof(struct tcpiphdr);
	if (max_linkhdr + sizeof(struct tcpiphdr) > CL_SIZE_128)
            {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_EMERGENCY event */
        WV_NET_MARKER_0 (NET_AUX_EVENT, WV_NET_EMERGENCY, 30, 1,
                         WV_NETEVENT_TCPINIT_HDRPANIC)
#endif  /* INCLUDE_WVNET */
#endif

            panic("tcp_init");
            }
}

/*
 * Create template to be used to send tcp packets on a connection.
 * Call after host entry created, allocates an mbuf and fills
 * in a skeletal tcp/ip header, minimizing the amount of work
 * necessary when the connection is used.
 */
struct mbuf *
tcp_template(tp)
	struct tcpcb *tp;
{
	register struct inpcb *inp = tp->t_inpcb;
	register struct mbuf *m;
	register struct tcpiphdr *n;

	if ((m = tp->t_template) == NULL) {
		m = mBufClGet (M_DONTWAIT, MT_HEADER, sizeof (struct tcpiphdr),
			       TRUE); 
		if (m == NULL)
			return (0);
		m->m_len = sizeof (struct tcpiphdr);
	}
	n = mtod (m, struct tcpiphdr *);
	n->ti_next = n->ti_prev = 0;
	n->ti_x1 = 0;
	n->ti_pr = IPPROTO_TCP;
	n->ti_len = htons(sizeof (struct tcpiphdr) - sizeof (struct ip));
	n->ti_src = inp->inp_laddr;
	n->ti_dst = inp->inp_faddr;
	n->ti_sport = inp->inp_lport;
	n->ti_dport = inp->inp_fport;
	n->ti_seq = 0;
	n->ti_ack = 0;
	n->ti_x2 = 0;
	n->ti_off = 5;
	n->ti_flags = 0;
	n->ti_win = 0;
	n->ti_sum = 0;
	n->ti_urp = 0;
	return (m);
}

/*
 * Send a single message to the TCP at address specified by
 * the given TCP/IP header.  If m == 0, then we make a copy
 * of the tcpiphdr at ti and send directly to the addressed host.
 * This is used to force keep alive messages out using the TCP
 * template for a connection tp->t_template.  If flags are given
 * then we send a message back to the TCP which originated the
 * segment ti, and discard the mbuf containing it and any other
 * attached mbufs.
 *
 * In any case the ack and sequence number of the transmitted
 * segment are as specified by the parameters.
 */
void
tcp_respond(tp, ti, m, ack, seq, flags)
	struct tcpcb *tp;
	register struct tcpiphdr *ti;
	register struct mbuf *m;
	tcp_seq ack, seq;
	int flags;
{
	register int tlen;
	int win = 0;
	struct route *ro = 0;

	if (tp) {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_NOTICE event */
    WV_NET_PORTOUT_EVENT_4 (NET_CORE_EVENT, WV_NET_NOTICE, 20, 2,
                            tp->t_inpcb->inp_lport, tp->t_inpcb->inp_fport,
                            WV_NETEVENT_TCPRESPOND_START, WV_NET_SEND,
                            tp->t_inpcb->inp_socket->so_fd, ack, seq, flags)
#endif  /* INCLUDE_WVNET */
#endif
		win = sbspace(&tp->t_inpcb->inp_socket->so_rcv);
		ro = &tp->t_inpcb->inp_route;
	}
	if (m == 0) {
		m = mHdrClGet(M_DONTWAIT, MT_HEADER, CL_SIZE_128, TRUE);
		if (m == NULL)
			return;
#ifdef TCP_COMPAT_42
		tlen = 1;
#else
		tlen = 0;
#endif
		m->m_data += max_linkhdr;
		*mtod(m, struct tcpiphdr *) = *ti;
		ti = mtod(m, struct tcpiphdr *);
		flags = TH_ACK;
	} else {
		m_freem(m->m_next);
		m->m_next = 0;
		m->m_data = (caddr_t)ti;
		m->m_len = sizeof (struct tcpiphdr);
		tlen = 0;
#define xchg(a,b,type) { type t; t=a; a=b; b=t; }
		xchg(ti->ti_dst.s_addr, ti->ti_src.s_addr, u_long);
		xchg(ti->ti_dport, ti->ti_sport, u_short);
#undef xchg
	}
	ti->ti_len = htons((u_short)(sizeof (struct tcphdr) + tlen));
	tlen += sizeof (struct tcpiphdr);
	m->m_len = tlen;
	m->m_pkthdr.len = tlen;
	m->m_pkthdr.rcvif = (struct ifnet *) 0;
	ti->ti_next = ti->ti_prev = 0;
	ti->ti_x1 = 0;
	ti->ti_seq = htonl(seq);
	ti->ti_ack = htonl(ack);
	ti->ti_x2 = 0;
	ti->ti_off = sizeof (struct tcphdr) >> 2;
	ti->ti_flags = flags;
	if (tp)
		ti->ti_win = htons((u_short) (win >> tp->rcv_scale));
	else
		ti->ti_win = htons((u_short)win);
	ti->ti_urp = 0;
	ti->ti_sum = 0;
	ti->ti_sum = in_cksum(m, tlen);
	((struct ip *)ti)->ip_len = tlen;
	((struct ip *)ti)->ip_ttl = ip_defttl;
	if (tp)
	    ((struct ip *)ti)->ip_tos = tp->t_inpcb->inp_ip.ip_tos;

	(void) ip_output(m, NULL, ro, 0, NULL);
}

/*
 * Create a new TCP control block, making an
 * empty reassembly queue and hooking it to the argument
 * protocol control block.
 */
struct tcpcb *
tcp_newtcpcb(inp)
	struct inpcb *inp;
{
	register struct tcpcb *tp;

	MALLOC(tp, struct tcpcb *, sizeof(*tp), MT_PCB, M_DONTWAIT);
	if (tp == NULL)
		return ((struct tcpcb *)0);
	bzero((char *) tp, sizeof(struct tcpcb));
	tp->seg_next = tp->seg_prev = (struct tcpiphdr *)tp;
	tp->t_maxseg = tp->t_maxsize = tcp_mssdflt;

	tp->t_flags = tcp_do_rfc1323 ? (TF_REQ_SCALE|TF_REQ_TSTMP) : 0;
	tp->t_inpcb = inp;
	/*
	 * Init srtt to TCPTV_SRTTBASE (0), so we can tell that we have no
	 * rtt estimate.  Set rttvar so that srtt + 2 * rttvar gives
	 * reasonable initial retransmit time.
	 */
	tp->t_srtt = TCPTV_SRTTBASE;
	tp->t_rttvar = tcp_rttdflt * PR_SLOWHZ << 2;
	tp->t_rttmin = TCPTV_MIN;
	TCPT_RANGESET(tp->t_rxtcur, 
	    ((TCPTV_SRTTBASE >> 2) + (TCPTV_SRTTDFLT << 2)) >> 1,
	    TCPTV_MIN, TCPTV_REXMTMAX);
	tp->snd_cwnd = TCP_MAXWIN << TCP_MAX_WINSHIFT;
	tp->snd_ssthresh = TCP_MAXWIN << TCP_MAX_WINSHIFT;
	inp->inp_ip.ip_ttl = ip_defttl;
	inp->inp_ppcb = (caddr_t)tp;
	return (tp);
}

/*
 * Drop a TCP connection, reporting
 * the specified error.  If connection is synchronized,
 * then send a RST to peer.
 */
struct tcpcb *
tcp_drop(tp, error)
	register struct tcpcb *tp;
	int error;
{
	struct socket *so = tp->t_inpcb->inp_socket;

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_INFO event */
    WV_NET_PORTOUT_MARKER_1 (NET_AUX_EVENT, WV_NET_INFO, 38, 3,
                            tp->t_inpcb->inp_lport, tp->t_inpcb->inp_fport,
                            WV_NETEVENT_TCPDROP_START, so->so_fd)
#endif  /* INCLUDE_WVNET */
#endif

	if (TCPS_HAVERCVDSYN(tp->t_state)) {
		tp->t_state = TCPS_CLOSED;
		(void) tcp_output(tp);
		tcpstat.tcps_drops++;
	} else
		tcpstat.tcps_conndrops++;
	if (error == ETIMEDOUT && tp->t_softerror)
		error = tp->t_softerror;
	so->so_error = error;
	return (tcp_close(tp));
}

/*
 * Close a TCP control block:
 *	discard all space held by the tcp
 *	discard internet protocol block
 *	wake up any sleepers
 */
struct tcpcb *
tcp_close(tp)
	register struct tcpcb *tp;
{
	register struct tcpiphdr *t;
	struct inpcb *inp = tp->t_inpcb;
	struct socket *so = inp->inp_socket;
	register struct mbuf *m;

	register struct rtentry *rt;

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_INFO event */
    WV_NET_MARKER_1 (NET_AUX_EVENT, WV_NET_INFO, 39, 4,
                     WV_NETEVENT_TCPCLOSE_START, so->so_fd)
#endif  /* INCLUDE_WVNET */
#endif

	/*
	 * If we sent enough data to get some meaningful characteristics,
	 * save them in the routing entry.  'Enough' is arbitrarily 
	 * defined as the sendpipesize (default 4K) * 16.  This would
	 * give us 16 rtt samples assuming we only get one sample per
	 * window (the usual case on a long haul net).  16 samples is
	 * enough for the srtt filter to converge to within 5% of the correct
	 * value; fewer samples and we could save a very bogus rtt.
	 *
	 * Don't update the default route's characteristics and don't
	 * update anything that the user "locked".
	 */
	if (SEQ_LT(tp->iss + so->so_snd.sb_hiwat * 16, tp->snd_max) &&
	    (rt = inp->inp_route.ro_rt) &&
	    ((struct sockaddr_in *)rt_key(rt))->sin_addr.s_addr != INADDR_ANY) {
		register u_long i = 0;

		if ((rt->rt_rmx.rmx_locks & RTV_RTT) == 0) {
			i = tp->t_srtt *
			    (RTM_RTTUNIT / (PR_SLOWHZ * TCP_RTT_SCALE));
			if (rt->rt_rmx.rmx_rtt && i)
				/*
				 * filter this update to half the old & half
				 * the new values, converting scale.
				 * See route.h and tcp_var.h for a
				 * description of the scaling constants.
				 */
				rt->rt_rmx.rmx_rtt =
				    (rt->rt_rmx.rmx_rtt + i) / 2;
			else
				rt->rt_rmx.rmx_rtt = i;
		}
		if ((rt->rt_rmx.rmx_locks & RTV_RTTVAR) == 0) {
			i = tp->t_rttvar *
			    (RTM_RTTUNIT / (PR_SLOWHZ * TCP_RTTVAR_SCALE));
			if (rt->rt_rmx.rmx_rttvar && i)
				rt->rt_rmx.rmx_rttvar =
				    (rt->rt_rmx.rmx_rttvar + i) / 2;
			else
				rt->rt_rmx.rmx_rttvar = i;
		}
		/*
		 * update the pipelimit (ssthresh) if it has been updated
		 * already or if a pipesize was specified & the threshhold
		 * got below half the pipesize.  I.e., wait for bad news
		 * before we start updating, then update on both good
		 * and bad news.
                 *
                 * Update the slow start threshold when all of the
                 * following conditions apply:
                 *   1) The value is not locked (via a routing socket).
                 *   2) Some packet loss occurred during the connection
                 *       (snd_ssthresh is not zero)
                 *   3) Packet loss also occurred during an earlier
                 *      connection (rmx_ssthresh is not zero). This
                 *      condition also applies if a user set a value
                 *      in advance via a routing socket.
                 *
                 * If none of the above conditions apply, the system
                 * still updates the slow start threshold if the
                 * value for this connection's packet loss is less
                 * than half the rmx_sendpipe value set via a routing
                 * socket. If that pipe size is available, the update
                 * occurs even if no packet loss happened.
		 */

		if (((rt->rt_rmx.rmx_locks & RTV_SSTHRESH) == 0 &&
		       (i = tp->snd_ssthresh) && rt->rt_rmx.rmx_ssthresh) ||
		    i < (rt->rt_rmx.rmx_sendpipe / 2)) {
			/*
			 * convert the limit from user data bytes to
			 * packets then to packet data bytes.
			 */
			i = (i + tp->t_maxseg / 2) / tp->t_maxseg;
			if (i < 2)
				i = 2;
			i *= (u_long)(tp->t_maxseg + sizeof (struct tcpiphdr));
			if (rt->rt_rmx.rmx_ssthresh)
				rt->rt_rmx.rmx_ssthresh =
				    (rt->rt_rmx.rmx_ssthresh + i) / 2;
			else
				rt->rt_rmx.rmx_ssthresh = i;
		}
	}

	/* free the reassembly queue, if any */
	t = tp->seg_next;
	while (t != (struct tcpiphdr *)tp) {
		t = (struct tcpiphdr *)t->ti_next;
		m = REASS_MBUF((struct tcpiphdr *)t->ti_prev);
		remque(t->ti_prev);
		m_freem(m);
	}
	if (tp->t_template)
		(void) m_free(tp->t_template);
	FREE(tp, MT_PCB); 
	inp->inp_ppcb = 0;
	soisdisconnected(so);
	/* clobber input pcb cache if we're closing the cached connection */
	if (inp == tcp_last_inpcb)
            tcp_last_inpcb = NULL;
	in_pcbdetach(inp);
	tcpstat.tcps_closed++;
	return ((struct tcpcb *)0);
}

void
tcp_drain()
{

}

/*
 * Notify a tcp user of an asynchronous error;
 * store error as soft error, but wake up user
 * (for now, won't do anything until can select for soft error).
 */
void tcp_notify
    (
    struct inpcb *inp, 
    int error
    )
    {
    register struct tcpcb *tp = (struct tcpcb *)inp->inp_ppcb;
    register struct socket *so = inp->inp_socket;

    /*
     * Ignore some errors if we are hooked up.
     * If connection hasn't completed, has retransmitted several times,
     * and receives a second error, give up now.  This is better
     * than waiting a long time to establish a connection that
     * can never complete.
     */

    if (tp->t_state == TCPS_ESTABLISHED &&
        (error == EHOSTUNREACH || error == ENETUNREACH ||
         error == EHOSTDOWN))
        {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_INFO event */
        WV_NET_MARKER_2 (NET_AUX_EVENT, WV_NET_INFO, 40, 5,
                         WV_NETEVENT_TCPNOTIFY_IGNORE, so->so_fd, error)
#endif  /* INCLUDE_WVNET */
#endif

        return;
        }
    else if (tp->t_state < TCPS_ESTABLISHED && tp->t_rxtshift > 3 &&
             tp->t_softerror)
        {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_INFO event */
        WV_NET_MARKER_2 (NET_AUX_EVENT, WV_NET_INFO, 41, 6,
                         WV_NETEVENT_TCPNOTIFY_KILL, so->so_fd, error)
#endif  /* INCLUDE_WVNET */
#endif

        so->so_error = error;
        }
    else 
        {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_INFO event */
        WV_NET_MARKER_2 (NET_AUX_EVENT, WV_NET_INFO, 42, 7,
                         WV_NETEVENT_TCPNOTIFY_ERROR, so->so_fd, error)
#endif  /* INCLUDE_WVNET */
#endif

        tp->t_softerror = error;
        }

    if (so->so_timeoSem)
        wakeup(so->so_timeoSem);

    sorwakeup(so);
    sowwakeup(so);
    }


void
tcp_ctlinput(cmd, sa, ip)
	int cmd;
	struct sockaddr *sa;
	register struct ip *ip;
{
	register struct tcphdr *th;
	extern struct in_addr zeroin_addr;
	extern u_char inetctlerrmap[];
	void (*notify) (struct inpcb *, int) = tcp_notify;

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_INFO event */
    WV_NET_MARKER_1 (NET_AUX_EVENT, WV_NET_INFO, 43, 8,
                     WV_NETEVENT_TCPCTLIN_START, cmd)
#endif  /* INCLUDE_WVNET */
#endif

	if (cmd == PRC_QUENCH)
		notify = tcp_quench;
        else if (cmd == PRC_MSGSIZE)
                notify = tcp_updatemtu;
	else if (!PRC_IS_REDIRECT(cmd) &&
		 ((unsigned)cmd > PRC_NCMDS || inetctlerrmap[cmd] == 0))
		return;
	if (ip) {
		th = (struct tcphdr *)((caddr_t)ip + (ip->ip_hl << 2));
		in_pcbnotify(&tcb, sa, th->th_dport, ip->ip_src, th->th_sport,
			cmd, notify);
	} else
		in_pcbnotify(&tcb, sa, 0, zeroin_addr, 0, cmd, notify);
}

/*
 * When a source quench is received, close congestion window
 * to one segment.  We will gradually open it again as we proceed.
 */
void
tcp_quench(inp, error)
	struct inpcb *inp;
	int error;
{
	struct tcpcb *tp = intotcpcb(inp);

	if (tp)
		tp->snd_cwnd = tp->t_maxseg;
}

/*
 * Look-up the routing entry to the peer of this inpcb.  If no route
 * is found and it cannot be allocated the return NULL.  This routine
 * is called by TCP routines that access the rmx structure.
 */
struct rtentry *
tcp_rtlookup(inp)
        struct inpcb *inp;
{
        struct route *ro;
        struct rtentry *rt;

        ro = &inp->inp_route;
        rt = ro->ro_rt;
        if (rt == NULL || !(rt->rt_flags & RTF_UP)) {
                /* No route yet, so try to acquire one */
                if (inp->inp_faddr.s_addr != INADDR_ANY) {
                        ro->ro_dst.sa_family = AF_INET;
                        ro->ro_dst.sa_len = sizeof(ro->ro_dst);
                        ((struct sockaddr_in *) &ro->ro_dst)->sin_addr =
                                inp->inp_faddr;
                        TOS_SET (&ro->ro_dst, inp->inp_ip.ip_tos);
                        rtalloc(ro);
                        rt = ro->ro_rt;
                }
        }
        return rt;
}


/*
 * When an ICMP error "need fragmentation" message causes the stack to
 * update the MSS based on the (already updated) path MTU estimate and
 * signal TCP to send data so that the dropped packet is detected. This 
 * routine duplicates some code from the tcp_mss() routine in tcp_input.c
 */

void tcp_updatemtu
    (
    struct inpcb * inp,
    int error
    )
    {
    struct tcpcb *tp = intotcpcb(inp);
    struct rtentry *rt;
    struct socket *so = inp->inp_socket;
    int mss;
    int maxseg;
    int cwnd = 0;

    if (tp)
        {
        rt = tcp_rtlookup (inp);
        if ( (rt == NULL) || (rt->rt_rmx.rmx_locks & RTV_MTU))
            {
            /*
             * New path MTU estimate unavailable or discovery process
             * ended - set segment size to default value if less than
             * current estimate.
             */

            tp->t_maxsize = min (tp->t_maxsize, tcp_mssdflt);
            tp->t_maxseg = min (so->so_snd.sb_hiwat, tp->t_maxsize);
            return;
            }

        maxseg = rt->rt_rmx.rmx_mtu - sizeof(struct tcpiphdr);

        mss = rt->rt_rmx.rmx_mss;

        /*
         * The following comparison violates the TCP specification by
         * permitting IP datagrams larger than the mandatory default
         * if the peer has not sent the initial SYN (which assigns the
         * MSS value). Although TCP is forbidden to exceed the default
         * unless explicitly allowed by the MSS option from a peer, that
         * behavior would probably prevent path MTU discovery from occurring
         * since the default is already less than many typical MTU sizes.
         *
         * Avoiding the conservative default might not have a significant
         * impact. Any unexpectedly large segments will only occur when
         * sending an initial SYN which includes options and/or data to a
         * host which has never been connected. Once the remote peer sends
         * a return SYN (during the connection handshake), the system will
         * record a MSS value and reduce the maximum segment size if
         * necessary. If that remote peer ignores the generally recommended
         * "liberal acceptance" policy and discards the segment which its
         * network layer successfully received, the failed connection attempts
         * will reveal the problem.
         */

        if (mss)
            {
            /*
             * If necessary, reduce the path MTU estimate according to the
             * known segment size which the peer is required to accept.
             */

            maxseg = min (maxseg, mss);
            }

        /*
         * Do not allow ICMP error messages to increase the maximum
         * segment size above the initial value determined in the
         * tcp_mss() routine.
         */

        if (tp->t_maxsize <= maxseg)
            return;
        tp->t_maxsize = maxseg;

        /* Update the mss to ignore newly received options? */

        if ((tp->t_flags & (TF_REQ_TSTMP|TF_NOOPT)) == TF_REQ_TSTMP &&
            (tp->t_flags & TF_RCVD_TSTMP) == TF_RCVD_TSTMP)
            maxseg -= TCPOLEN_TSTAMP_APPA;

#if     (MCLBYTES & (MCLBYTES - 1)) == 0
        if (maxseg > MCLBYTES)
            maxseg &= ~(MCLBYTES-1);
#else
        if (maxseg > MCLBYTES)
            maxseg = maxseg / MCLBYTES * MCLBYTES;
#endif
        if (so->so_snd.sb_hiwat < maxseg)
            maxseg = so->so_snd.sb_hiwat;

        tp->t_maxseg = maxseg;

        tp->t_rtt = 0;
        tp->snd_nxt = tp->snd_una;

        /*
	 * If tcp_updatemtu was called in response to an ICMP fragmentation-
	 * needed message (error = EMSGSIZE), trigger slow-start without 
	 * changing the congestion window. Temporarily set cwnd to 1 segment 
	 * and then restore it to original value after tcp_output. tcp_updatemtu
	 * is also called from tcp_output but in that case error is 0. We do
	 * not want to trigger slow-start for that scenario.
	 */

        if (error == EMSGSIZE)
	    {
	    cwnd = tp->snd_cwnd;
	    tp->snd_cwnd = maxseg;
	    }

        tcp_output(tp);

	if (error == EMSGSIZE)
	    tp->snd_cwnd = cwnd;
        }
    }
