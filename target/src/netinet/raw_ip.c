/* raw_ip.c - raw interface to IP protocol */

/* Copyright 1984 - 2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
 * Copyright (c) 1982, 1986, 1988, 1993, 1995
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
 *	@(#)raw_ip.c	8.7 (Berkeley) 5/15/95
 */

/*
modification history
--------------------
01j,20may02,vvv  fixed rip_input to send ICMP protocol unreachable errors 
		 only when called directly from ip_input (SPR #75405)
01i,21mar02,rae  Allow ICMP to be excluded (SPR #73703)
01h,12oct01,rae  merge from truestack ver 01o, base 01g (SPRs 69657,
                 35258, etc)
01g,26aug98,n_s  corrected rip_output for IP_HDRINCL case. spr #22246
01f,26aug98,n_s  added return val check for M_PREPEND in rip_output and
                 mBufClGet in rip_ctloutput. spr # 22238.
01e,16apr97,vin	 added PRU_CONTROL support to rip_usrreq().
01d,08apr97,vin  include pcb hashing changes from FREEBSD2.2.1, removed
		 ip_mrouter, added mCastRouteCmdHook. removed all MROUTING
01c,31jan97,vin  changed declaration according to prototype decl in protosw.h
01b,22nov96,vin  added cluster support replaced m_get(..) with mBufClGet(..).
01a,03mar96,vin  created from BSD4.4 stuff,integrated with 02i of raw_usrreq.c
*/

/*
DESCRIPTION
*/

#include "vxWorks.h"
#include "net/mbuf.h"
#include "sys/socket.h"
#include "net/protosw.h"
#include "net/socketvar.h"
#include "errno.h"

#include "net/if.h"
#include "net/route.h"
#include "net/raw_cb.h"

#include "netinet/in.h"
#include "netinet/in_systm.h"
#include "netinet/in_var.h"
#include "netinet/ip.h"
#include "netinet/ip_icmp.h"
#include "netinet/ip_var.h"
#include "netinet/ip_mroute.h"
#include "netinet/in_pcb.h"

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET
#include "wvNetLib.h"
#endif
#endif

/* externs */
extern FUNCPTR _mCastRouteCmdHook;	/* WRS mcast route command hook */
extern VOIDFUNCPTR _icmpErrorHook;
extern int icmp_ctloutput (int, struct socket *, int, int, struct mbuf **);
extern struct protosw inetsw[];
extern u_char ip_protox[IPPROTO_MAX];

/* globals */

#ifndef VIRTUAL_STACK
static struct inpcbhead ripcb;
static struct inpcbinfo ripcbinfo;
static struct sockaddr_in ripsrc = { sizeof(ripsrc), AF_INET };
#endif /* VIRTUAL_STACK */

#ifdef VIRTUAL_STACK
#include "netinet/vsLib.h"
#endif

/*
 * Nominal space allocated to a raw ip socket.
 */
#define	RIPSNDQ		8192
#define	RIPRCVQ		8192

/* locals */
static int _mCastCtlOutput (int option, int optName, struct socket * sockPtr,
                            struct mbuf ** pPtrMbuf); 

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET
    /* Set common fields of event identifiers for this module. */

LOCAL UCHAR wvNetModuleId = WV_NET_RAWIP_MODULE;   /* Value for raw_ip.c */
LOCAL UCHAR wvNetLocalFilter = WV_NET_NONE;     /* Available event filter */

LOCAL ULONG wvNetEventId;       /* Event identifier: see wvNetLib.h */
#endif    /* INCLUDE_WVNET */
#endif

/*
 * Raw interface to IP protocol.
 */

/*
 * Initialize raw connection block q.
 */
void
rip_init()
{
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_VERBOSE event */
    WV_NET_MARKER_0 (NET_AUX_EVENT, WV_NET_VERBOSE, 31, 11,
                     WV_NETEVENT_RAWIPINIT_START)
#endif  /* INCLUDE_WVNET */
#endif

	LIST_INIT(&ripcb);
        ripcbinfo.listhead = &ripcb;
        /*
         * XXX We don't use the hash list for raw IP, but it's easier
         * to allocate a one entry hash list than it is to check all
         * over the place for hashbase == NULL.
         */
        ripcbinfo.hashbase = hashinit(1, MT_PCB, &ripcbinfo.hashmask);

#ifdef VIRTUAL_STACK
        rip_sendspace = RIPSNDQ;
        rip_recvspace = RIPRCVQ;
        ripsrc.sin_len = sizeof(struct sockaddr_in);
        ripsrc.sin_family = AF_INET;
#endif /* VIRTUAL_STACK */
}

/*
 * Setup generic address and protocol structures
 * for raw_input routine, then pass them along with
 * mbuf chain.
 */
void
rip_input(m)
	struct mbuf *m;
{
	register struct ip *ip = mtod(m, struct ip *);
	register struct inpcb *inp;
	struct socket *last = 0;

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_NOTICE event */
    WV_NET_EVENT_0 (NET_CORE_EVENT, WV_NET_NOTICE, 14, 7,
                     WV_NETEVENT_RAWIPIN_START, WV_NET_RECV)
#endif  /* INCLUDE_WVNET */
#endif

	ripsrc.sin_addr = ip->ip_src;
        * (int *)ripsrc.sin_zero = m->m_pkthdr.rcvif->if_index;
        for (inp = ripcb.lh_first; inp != NULL; inp = inp->inp_list.le_next) {
		if (inp->inp_ip.ip_p && inp->inp_ip.ip_p != ip->ip_p)
			continue;
		if (inp->inp_laddr.s_addr &&
		    inp->inp_laddr.s_addr != ip->ip_dst.s_addr)
			continue;
		if (inp->inp_faddr.s_addr &&
		    inp->inp_faddr.s_addr != ip->ip_src.s_addr)
			continue;
		if (last) {
			struct mbuf *n;
			if ((n = m_copy(m, 0, (int)M_COPYALL))) {
				if (sbappendaddr(&last->so_rcv,
				    (struct sockaddr *)&ripsrc, n,
				    (struct mbuf *)0) == 0)
                                    {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_WARNING event */
                WV_NET_ADDRIN_EVENT_3 (NET_CORE_EVENT, WV_NET_WARNING, 9, 6,
                                       ip->ip_src.s_addr, ip->ip_dst.s_addr,
                                       WV_NETEVENT_RAWIPIN_LOST, WV_NET_RECV,
                                       last->so_fd, ip->ip_src.s_addr,
                                       ip->ip_dst.s_addr)
#endif  /* INCLUDE_WVNET */
#endif

					/* should notify about lost packet */
					m_freem(n);
                                    }
				else
					sorwakeup(last);
			}
		}
		last = inp->inp_socket;
	}
	if (last) {
		if (sbappendaddr(&last->so_rcv, (struct sockaddr *)&ripsrc,
		    m, (struct mbuf *)0) == 0)
                    {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_WARNING event */
                WV_NET_ADDRIN_EVENT_3 (NET_CORE_EVENT, WV_NET_WARNING, 9, 6,
                                       ip->ip_src.s_addr, ip->ip_dst.s_addr,
                                       WV_NETEVENT_RAWIPIN_LOST, WV_NET_RECV,
                                       last->so_fd, ip->ip_src.s_addr,
                                       ip->ip_dst.s_addr)
#endif  /* INCLUDE_WVNET */
#endif

			m_freem(m);
                    }
		else
			sorwakeup(last);
	} else {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_ERROR event */
            WV_NET_ADDRIN_EVENT_2 (NET_CORE_EVENT, WV_NET_ERROR, 35, 5,
                                   ip->ip_src.s_addr, ip->ip_dst.s_addr,
                                   WV_NETEVENT_RAWIPIN_NOSOCK, WV_NET_RECV,
                                   ip->ip_src.s_addr, ip->ip_dst.s_addr)
#endif  /* INCLUDE_WVNET */
#endif

                if ((_icmpErrorHook) && 
		    (inetsw[ip_protox[ip->ip_p]].pr_protocol == IPPROTO_RAW))
                    _icmpErrorHook (m, ICMP_UNREACH, ICMP_UNREACH_PROTOCOL, 
                                    ip->ip_src.s_addr, 0);
		m_freem(m);

#ifdef VIRTUAL_STACK
		_ipstat.ips_noproto++;
		_ipstat.ips_delivered--;
#else
		ipstat.ips_noproto++;
		ipstat.ips_delivered--;
#endif /* VIRTUAL_STACK */
	}
	return;
}

/*
 * Generate IP header and pass packet to ip_output.
 * Tack on options user may have setup with control call.
 */
int
rip_output(m, so, dst)
	register struct mbuf *m;
	struct socket *so;
	u_long dst;
{
	register struct ip *ip;
	register struct inpcb *inp = sotoinpcb(so);
	struct mbuf *opts;
	int flags = (so->so_options & SO_DONTROUTE) | IP_ALLOWBROADCAST;
        int result;

	/*
	 * If the user handed us a complete IP packet, use it.
	 * Otherwise, allocate an mbuf for a header and fill it in.
	 */
	if ((inp->inp_flags & INP_HDRINCL) == 0) {
		M_PREPEND(m, sizeof(struct ip), M_WAIT);
		if (m == (struct mbuf *) NULL)
		    {
		    return (ENOBUFS);
		    }
		    
		ip = mtod(m, struct ip *);
		ip->ip_tos = 0;
		ip->ip_off = 0;
		ip->ip_p = inp->inp_ip.ip_p;
		ip->ip_len = m->m_pkthdr.len;
		ip->ip_src = inp->inp_laddr;
		ip->ip_dst.s_addr = dst;
		ip->ip_ttl = inp->inp_ip.ip_ttl;

		opts = inp->inp_options;
	} else {
	        if (m->m_len < sizeof (struct ip) &&
		    (m = m_pullup (m, sizeof (struct ip))) == NULL)
		    {
		    return (ENOBUFS);
		    }

		ip = mtod(m, struct ip *);
		if (ip->ip_id == 0)
#ifdef VIRTUAL_STACK
			ip->ip_id = htons(_ip_id++);
#else
			ip->ip_id = htons(ip_id++);
#endif /* VIRTUAL_STACK */
		opts = NULL;
		/* XXX prevent ip_output from overwriting header fields */
		flags |= IP_RAWOUTPUT;
#ifdef VIRTUAL_STACK
		_ipstat.ips_rawout++;
#else
		ipstat.ips_rawout++;
#endif /* VIRTUAL_STACK */
	}
    result = ip_output(m, opts, &inp->inp_route, flags, inp->inp_moptions);

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_NOTICE event */
    WV_NET_ADDROUT_EVENT_3 (NET_CORE_EVENT, WV_NET_NOTICE, 15, 8,
                            inp->inp_laddr.s_addr, dst,
                            WV_NETEVENT_RAWIPOUT_FINISH, WV_NET_SEND,
                            result, inp->inp_laddr.s_addr, dst)
#endif  /* INCLUDE_WVNET */
#endif

    return (result);
}

/*
 * Raw IP socket option processing.
 */
int
rip_ctloutput(op, so, level, optname, m)
	int op;
	struct socket *so;
	int level, optname;
	struct mbuf **m;
{
	register struct inpcb *inp = sotoinpcb(so);
	register int error;

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_INFO event */
    WV_NET_MARKER_4 (NET_AUX_EVENT, WV_NET_INFO, 13, 9,
                     WV_NETEVENT_RAWCTLOUT_START,
                     so->so_fd, op, level, optname)
#endif  /* INCLUDE_WVNET */
#endif

        if (level == IPPROTO_ICMP)
            return (icmp_ctloutput (op, so, level, optname, m));

	if (level != IPPROTO_IP) {
		if (op == PRCO_SETOPT && *m)
			(void) m_free(*m);
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_ERROR event */
                WV_NET_MARKER_4 (NET_AUX_EVENT, WV_NET_ERROR, 32, 2,
                                 WV_NETEVENT_RAWCTLOUT_BADLEVEL,
                                 so->so_fd, op, level, optname)
#endif  /* INCLUDE_WVNET */
#endif

		return (EINVAL);
	}

	switch (optname) {

	case IP_HDRINCL:
		error = 0;
		if (op == PRCO_SETOPT) {
			if (*m == 0 || (*m)->m_len < sizeof (int))
                            {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_ERROR event */
                    WV_NET_MARKER_4 (NET_AUX_EVENT, WV_NET_ERROR, 33, 3,
                                     WV_NETEVENT_RAWCTLOUT_BADMEM,
                                     so->so_fd, op, level, optname)
#endif  /* INCLUDE_WVNET */
#endif

				error = EINVAL;
                            }
			else if (*mtod(*m, int *))
				inp->inp_flags |= INP_HDRINCL;
			else
				inp->inp_flags &= ~INP_HDRINCL;
			if (*m)
				(void)m_free(*m);
		} else {
			*m = mBufClGet(M_WAIT, MT_SOOPTS, CL_SIZE_128, TRUE);
			if (*m == (struct mbuf *) NULL)
			    {
			    error = ENOBUFS;
			    }
			else
			    {
			    (*m)->m_len = sizeof (int);
			    *mtod(*m, int *) = inp->inp_flags & INP_HDRINCL;
			    }
		}
		return (error);

	case MRT_INIT:
	case MRT_DONE:
	case MRT_ADD_VIF:
	case MRT_DEL_VIF:
	case MRT_ADD_MFC:
	case MRT_DEL_MFC:
	case MRT_VERSION:
	case MRT_ASSERT:

            	return (_mCastCtlOutput (op, optname, so, m));

	default:
		if (optname >= MRT_INIT)
            		return (_mCastCtlOutput (op, optname, so, m));
	}
	return (ip_ctloutput(op, so, level, optname, m));
}

#ifndef VIRTUAL_STACK
u_long	rip_sendspace = RIPSNDQ;
u_long	rip_recvspace = RIPRCVQ;
#endif /* VIRTUAL_STACK */

/*ARGSUSED*/
int
rip_usrreq(so, req, m, nam, control)
	register struct socket *so;
	int req;
	struct mbuf *m, *nam, *control;
{
	register int error = 0;
	register struct inpcb *inp = sotoinpcb(so);

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_INFO event */
    WV_NET_MARKER_2 (NET_CORE_EVENT, WV_NET_INFO, 14, 10,
                     WV_NETEVENT_RAWIPREQ_START, so->so_fd, req)
#endif  /* INCLUDE_WVNET */
#endif

        if (req == PRU_CONTROL)
            return (in_control(so, (u_long)m, (caddr_t)nam,
                    (struct ifnet *)control));

	switch (req) {

	case PRU_ATTACH:
		if (inp)
                    {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_EMERGENCY event */
                WV_NET_MARKER_2 (NET_CORE_EVENT, WV_NET_EMERGENCY, 19, 1,
                                 WV_NETEVENT_RAWIPREQ_PANIC, so->so_fd, req)
#endif  /* INCLUDE_WVNET */
#endif

			panic("rip_attach");
                    }
		if ((so->so_state & SS_PRIV) == 0) {
			error = EACCES;
			break;
		}
		if ((error = soreserve(so, rip_sendspace, rip_recvspace)) ||
		    (error = in_pcballoc(so, &ripcbinfo)))
			break;
		inp = (struct inpcb *)so->so_pcb;
		inp->inp_ip.ip_p = (int)nam;
		inp->inp_ip.ip_ttl = ip_defttl;
		break;

	case PRU_DISCONNECT:
		if ((so->so_state & SS_ISCONNECTED) == 0) {
			error = ENOTCONN;
			break;
		}
		/* FALLTHROUGH */
	case PRU_ABORT:
		soisdisconnected(so);
		/* FALLTHROUGH */
	case PRU_DETACH:
		if (inp == 0)
                    {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_EMERGENCY event */
                WV_NET_MARKER_2 (NET_CORE_EVENT, WV_NET_EMERGENCY, 19, 1,
                                 WV_NETEVENT_RAWIPREQ_PANIC, so->so_fd, req)
#endif  /* INCLUDE_WVNET */
#endif

			panic("rip_detach");
                    }

		in_pcbdetach(inp);
		break;

	case PRU_BIND:
	    {
		struct sockaddr_in *addr = mtod(nam, struct sockaddr_in *);

		if (nam->m_len != sizeof(*addr)) {
			error = EINVAL;
			break;
		}
#ifdef VIRTUAL_STACK
		if ((_ifnet == 0) ||
#else
		if ((ifnet == 0) ||
#endif /* VIRTUAL_STACK */
		    ((addr->sin_family != AF_INET) &&
		     (addr->sin_family != AF_IMPLINK)) ||
		    (addr->sin_addr.s_addr &&
		     ifa_ifwithaddr((struct sockaddr *)addr) == 0)) {
			error = EADDRNOTAVAIL;
			break;
		}
		inp->inp_laddr = addr->sin_addr;
		break;
	    }
	case PRU_CONNECT:
	    {
		struct sockaddr_in *addr = mtod(nam, struct sockaddr_in *);

		if (nam->m_len != sizeof(*addr)) {
			error = EINVAL;
			break;
		}
#ifdef VIRTUAL_STACK
		if (_ifnet == 0) {
#else
		if (ifnet == 0) {
#endif /* VIRTUAL_STACK */
			error = EADDRNOTAVAIL;
			break;
		}
		if ((addr->sin_family != AF_INET) &&
		     (addr->sin_family != AF_IMPLINK)) {
			error = EAFNOSUPPORT;
			break;
		}
		inp->inp_faddr = addr->sin_addr;
		soisconnected(so);
		break;
	    }

	case PRU_CONNECT2:
		error = EOPNOTSUPP;
		break;

	/*
	 * Mark the connection as being incapable of further input.
	 */
	case PRU_SHUTDOWN:
		socantsendmore(so);
		break;

	/*
	 * Ship a packet out.  The appropriate raw output
	 * routine handles any massaging necessary.
	 */
	case PRU_SEND:
	    {
		register u_long dst;

		if (so->so_state & SS_ISCONNECTED) {
			if (nam) {
				error = EISCONN;
				break;
			}
			dst = inp->inp_faddr.s_addr;
		} else {
			if (nam == NULL) {
				error = ENOTCONN;
				break;
			}
			dst = mtod(nam, struct sockaddr_in *)->sin_addr.s_addr;
		}
		error = rip_output(m, so, dst);
		m = NULL;
		break;
	    }

	case PRU_SENSE:
		/*
		 * stat: don't bother with a blocksize.
		 */
		return (0);

	/*
	 * Not supported.
	 */
	case PRU_RCVOOB:
	case PRU_RCVD:
	case PRU_LISTEN:
	case PRU_ACCEPT:
	case PRU_SENDOOB:
		error = EOPNOTSUPP;
		break;

	case PRU_SOCKADDR:
		in_setsockaddr(inp, nam);
		break;

	case PRU_PEERADDR:
		in_setpeeraddr(inp, nam);
		break;

	default:
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_EMERGENCY event */
            WV_NET_MARKER_2 (NET_CORE_EVENT, WV_NET_EMERGENCY, 19, 1,
                             WV_NETEVENT_RAWIPREQ_PANIC, so->so_fd, req)
#endif  /* INCLUDE_WVNET */
#endif

		panic("rip_usrreq");
	}
	if (m != NULL)
		m_freem(m);

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_ERROR event */
    if (error)
        {
        WV_NET_MARKER_3 (NET_CORE_EVENT, WV_NET_ERROR, 34, 4,
                         WV_NETEVENT_RAWIPREQ_FAIL, so->so_fd, req, error)
        }
#endif  /* INCLUDE_WVNET */
#endif

	return (error);
}

/*******************************************************************************
* mCastCtlOutput - send commands to multicast routing engine.
*
* This function sends commands to multicast routing engine.
*
* INTERNAL
* This function can be further expanded to receive multicast options also.
* That is the reason for passing a pointer to a pointer to an mbuf as one of
* the argument.
*
* RETURNS: OK/ERROR
*
* NOMANUAL
*/

static int _mCastCtlOutput
    (
    int 		option,		/* type of option */
    int 		optName,	/* name of the option */
    struct socket * 	sockPtr,	/* pointer to the socket */
    struct mbuf **	pPtrMbuf	/* pointer to pointer to an mbuf */
    )
    {
    register int error = EINVAL;	/* default error value */

    if (pPtrMbuf == NULL)
        return (error);
    
    if (option == PRCO_SETOPT)
        {
        if (_mCastRouteCmdHook != NULL)
            error = (*_mCastRouteCmdHook)(optName, sockPtr, *pPtrMbuf);
        else
            error = EOPNOTSUPP; 
        }

    if (*pPtrMbuf != NULL)
        (void) m_free (*pPtrMbuf);

    return (error);
    }
