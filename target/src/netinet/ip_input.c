/* ip_input.c - internet input routines */

/* Copyright 1984 - 2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
 * Copyright (c) 1982, 1986, 1988, 1993
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
 *	@(#)ip_input.c	8.2 (Berkeley) 1/4/94
 */

/*
modification history
--------------------
01y,15apr02,wap  reinstate netJobAdd removal changes
01x,29mar02,wap  back out the previous change (needs more investigation)
01w,21mar02,rae  ipintr called directly (SPR #74604)
01v,03jan02,vvv  fixed reassembly problem for large packets with reordered
		 fragments (SPR #71746)
01u,12oct01,rae  merge from truestack ver 02f, base 01r (SPRs 69722,
                 69112, 27706, etc)
01t,07dec00,rae  added IPsec input hook
01s,27sep00,rae  fixed ping of death bug
01r,08nov99,pul  merging T2 cumulative patch 2
01q,04jun99,spm  corrected ip_forward() routine to handle source routing
                 options for hosts (SPR #27727)
01p,28feb99,ham  corrected comment below - (wrong)34040 -> (right)32767.
01o,28feb99,ham  fixed reassembly problem larger ip_off 34040, spr#23250.
01n,09jan98,vin  changed IP_CKSUM_RECV_MASK to IP_DO_CHECKSUM_RCV
01m,13nov97,vin  added HTONS(ip->ip_len) in ip_forward before icmp_error,
		 consolidated all flags into ipCfgFlags.
01l,28oct97,vin  reverted on the fix in version01j, what was vin thinking.
01k,15jul97,spm  made IP checksum calculation configurable (SPR #7836)
01j,05jun97,vin  fixed problem in ip_forward().HTONS(ip_len).
01i,17apr97,vin  added more functionality to the ipFilterHook.
01h,08mar97,vin  added mCastRouteFwdHook to access mcast routing code.
01g,11apr97,rjc  added ipFilter Hook and strongEnded flag processing.
01f,04mar97,vin  added fix to null domain pointer in ip_init SPR 8078.
01e,05feb97,rjc  tos routing changes
01d,31jan97,vin  changed declaration according to prototype decl in protosw.h
01c,07jan96,vin  moved iptime() from ip_icmp, added _icmpErrorHook instead
		 of icmp_error() for scalability, added ipfragttl. 
01b,30oct96,vin  re-worked ipreassembly, made it free from dtom macros.
		 added ipReAssembly, replace m_get(..) with mBufClGet(..)
01a,03mar96,vin  created from BSD4.4 stuff,integrated with 02r of ip_input.c
*/

#include "vxWorks.h"
#include "tickLib.h"
#include "netLib.h"
#include "net/systm.h"
#include "net/mbuf.h"
#include "net/domain.h"
#include "net/protosw.h"
#include "sys/socket.h"
#include "errno.h"

#include "net/if.h"
#include "net/route.h"

#include "netinet/in.h"
#include "netinet/in_pcb.h"
#include "netinet/in_systm.h"
#include "netinet/in_var.h"
#include "netinet/ip.h"
#include "netinet/ip_var.h"
#include "netinet/ip_icmp.h"

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET
#include "wvNetLib.h"
#endif
#endif

#ifdef VIRTUAL_STACK
#include "netinet/vsLib.h"
#include "netinet/vsMcast.h"
#endif /* VIRTUAL_STACK */

extern void _insque();
extern void _remque();
extern int sysClkRateGet();
extern int routeDrain();

#ifndef VIRTUAL_STACK
extern VOIDFUNCPTR _icmpErrorHook;
extern FUNCPTR _mCastRouteFwdHook;	/* WRS mcast forward command hook */
#endif /* VIRTUAL_STACK */

extern FUNCPTR _ipFilterHook;           /* ipFilter hook defined in netLib.c */

/*IPsec input hook defined in /target/src/wrn/ipsec/ipsec_io.c */
typedef int     (*IPSEC_INPUT_FUNCPTR)  (struct mbuf** m, int hlen, struct ip** ip);
IPSEC_INPUT_FUNCPTR             _func_ipsecInput;


/*IPsec filter hook is defined in /target/src/wrn/ipsec/ipsec_io.c */ 

typedef int    (*IPSEC_FILTER_HOOK_FUNCPTR) (struct mbuf** m, struct ip** ip, int hlen);

IPSEC_FILTER_HOOK_FUNCPTR       _func_ipsecFilterHook; 

/* globals */

#ifndef	IPFORWARDING
#ifdef GATEWAY
#define	IPFORWARDING	1	/* forward IP packets not for us */
#else /* GATEWAY */
#define	IPFORWARDING	0	/* don't forward IP packets not for us */
#endif /* GATEWAY */
#endif /* IPFORWARDING */
#ifndef	IPSENDREDIRECTS
#define	IPSENDREDIRECTS	1
#endif

#define FORWARD_BROADCASTS

#ifdef FORWARD_BROADCASTS
#define DIRECTED_BROADCAST
FUNCPTR	proxyBroadcastHook = NULL;
#endif

#ifndef VIRTUAL_STACK
int	_ipCfgFlags;			/* ip configurations, bit field flag */
int	ip_defttl = IPDEFTTL;		/* default IP ttl */
int	ipfragttl = IPFRAGTTL;		/* default IP fragment timeout */
#ifdef DIAGNOSTIC
int	ipprintfs = 0;
#endif
int     ipStrongEnded = FALSE;             /* default ip ended'ness */

extern	struct domain inetdomain;
extern	struct protosw inetsw[];

u_char	ip_protox[IPPROTO_MAX];

int	ipqmaxlen = IFQ_MAXLEN;
struct	in_ifaddr *in_ifaddr;		/* first inet address */
struct	ipstat	ipstat;
struct	ipq	ipq;			/* ip reass. queue */
u_short	ip_id;				/* ip packet ctr, for ids */

int	ip_nhops = 0;
#endif /* VIRTUAL_STACK */

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET
    /* Set common fields of event identifiers for this module. */

LOCAL UCHAR wvNetModuleId = WV_NET_IPINPUT_MODULE;   /* Value for ip_input.c */
LOCAL UCHAR wvNetLocalFilter = WV_NET_NONE;     /* Available event filter */

LOCAL ULONG wvNetEventId;       /* Event identifier: see wvNetLib.h */
#endif    /* INCLUDE_WVNET */
#endif

struct	ifqueue ipintrq;

/* forward declarations */

#ifdef SRCRT
static void save_rte (u_char *, struct in_addr);
#endif /* SRCRT */
LOCAL struct mbuf * ipReAssemble (FAST struct mbuf * pMbuf, 
				  FAST struct ipq *  pIpFragQueue); 
#ifdef SYSCTL_SUPPORT
/* ipforwarding set to on or off depending upon compile time flag */
int ipforwarding = IPFORWARDING;
#endif /* SYSCTL_SUPPORT */

struct rtentry *rtalloc2 (struct sockaddr *);

/*
 * IP initialization: fill in IP protocol switch table.
 * All protocols not implemented in kernel go to raw IP protocol handler.
 */
void
ip_init()
{
	register struct protosw *pr;
	register int i;

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_VERBOSE event */
    WV_NET_MARKER_0 (NET_AUX_EVENT, WV_NET_VERBOSE, 27, 20,
                     WV_NETEVENT_IPINIT_START)
#endif  /* INCLUDE_WVNET */
#endif

#ifdef VIRTUAL_STACK
        ip_defttl = IPDEFTTL;
        ipfragttl = IPFRAGTTL;
#ifdef DIAGNOSTIC
        ipprintfs = 0;
#endif
        ipStrongEnded = FALSE;
        ipqmaxlen = IFQ_MAXLEN;
        ip_nhops = 0;
        input_ipaddr.sin_len = sizeof (struct sockaddr_in);
        input_ipaddr.sin_family = AF_INET;
#endif /* VIRTUAL_STACK */

	pr = pffindproto(PF_INET, IPPROTO_RAW, SOCK_RAW);
	if (pr == 0)
            {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_EMERGENCY event */
            WV_NET_MARKER_0 (NET_AUX_EVENT, WV_NET_EMERGENCY, 15, 1,
                             WV_NETEVENT_IPINIT_PANIC)
#endif  /* INCLUDE_WVNET */
#endif

            panic("ip_init");
            }
	for (i = 0; i < IPPROTO_MAX; i++)
		ip_protox[i] = pr - inetsw;
	for (pr = inetdomain.dom_protosw;
	    pr < inetdomain.dom_protoswNPROTOSW; pr++)
                if ((pr->pr_domain != NULL) &&
                    pr->pr_domain->dom_family == PF_INET &&
                    pr->pr_protocol && pr->pr_protocol != IPPROTO_RAW)
                    ip_protox[pr->pr_protocol] = pr - inetsw;
#ifdef VIRTUAL_STACK
	_ipq.next = _ipq.prev = &_ipq;
	_ip_id = iptime() & 0xffff;
#else
	ipq.next = ipq.prev = &ipq;
	ip_id = iptime() & 0xffff;
#endif
	ipintrq.ifq_maxlen = ipqmaxlen;
	return;
}

#ifndef VIRTUAL_STACK
struct	sockaddr_in input_ipaddr = { sizeof(input_ipaddr), AF_INET };
struct	route ipforward_rt;
#endif /* VIRTUAL_STACK */

/*
 * Job entry point for net task.
 * Ip input routine.  Checksum and byte swap header.  If fragmented
 * try to reassemble.  Process options.  Pass to next level.
 */

void
ipintr(struct mbuf *m)
{
	struct ip *ip;
	register struct ipq *fp;
	register struct in_ifaddr *ia;
	int hlen, s;

        if (m==0)
            return;
        s = splnet();

#ifdef	DIAGNOSTIC
	if ((m->m_flags & M_PKTHDR) == 0)
            {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_EMERGENCY event */
            WV_NET_EVENT_0 (NET_CORE_EVENT, WV_NET_EMERGENCY, 16, 2, 
                            WV_NETEVENT_IPIN_PANIC, WV_NET_RECV)
#endif  /* INCLUDE_WVNET */
#endif

            panic("ipintr no HDR");
            }
#endif

	/*
	 * If no IP addresses have been set yet but the interfaces
	 * are receiving, can't do anything with incoming packets yet.
	 */
#ifdef VIRTUAL_STACK
	if (_in_ifaddr == NULL)
#else
	if (in_ifaddr == NULL)
#endif /* VIRTUAL_STACK */
		goto bad;
#ifdef VIRTUAL_STACK
	_ipstat.ips_total++;
#else
	ipstat.ips_total++;
#endif /* VIRTUAL_STACK */
	if (m->m_len < sizeof (struct ip) &&
	    (m = m_pullup(m, sizeof (struct ip))) == 0) {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_CRITICAL event */
                WV_NET_EVENT_0 (NET_CORE_EVENT, WV_NET_CRITICAL, 12, 3, 
                                WV_NETEVENT_IPIN_SHORTMSG, WV_NET_RECV)
#endif  /* INCLUDE_WVNET */
#endif

#ifdef VIRTUAL_STACK
		_ipstat.ips_toosmall++;
#else
		ipstat.ips_toosmall++;
#endif /* VIRTUAL_STACK */
		goto done;
	}
	ip = mtod(m, struct ip *);
	if (ip->ip_v != IPVERSION) {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_CRITICAL event */
        WV_NET_EVENT_1 (NET_CORE_EVENT, WV_NET_CRITICAL, 13, 4, 
                        WV_NETEVENT_IPIN_BADVERS, WV_NET_RECV, ip->ip_v)
#endif  /* INCLUDE_WVNET */
#endif

#ifdef VIRTUAL_STACK
		_ipstat.ips_badvers++;
#else
		ipstat.ips_badvers++;
#endif /* VIRTUAL_STACK */
		goto bad;
	}
	hlen = ip->ip_hl << 2;
	if (hlen < sizeof(struct ip)) {	/* minimum header length */
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_CRITICAL event */
        WV_NET_EVENT_1 (NET_CORE_EVENT, WV_NET_CRITICAL, 14, 5, 
                        WV_NETEVENT_IPIN_BADHLEN, WV_NET_RECV, hlen)
#endif  /* INCLUDE_WVNET */
#endif

#ifdef VIRTUAL_STACK
		_ipstat.ips_badhlen++;
#else
		ipstat.ips_badhlen++;
#endif /* VIRTUAL_STACK */
		goto bad;
	}
	if (hlen > m->m_len) {
		if ((m = m_pullup(m, hlen)) == 0) {

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_CRITICAL event */
            WV_NET_EVENT_1 (NET_CORE_EVENT, WV_NET_CRITICAL, 14, 5, 
                            WV_NETEVENT_IPIN_BADHLEN, WV_NET_RECV, hlen)
#endif  /* INCLUDE_WVNET */
#endif

#ifdef VIRTUAL_STACK
			_ipstat.ips_badhlen++;
#else
			ipstat.ips_badhlen++;
#endif /* VIRTUAL_STACK */
			goto done;
		}
		ip = mtod(m, struct ip *);
	}

        if (_ipCfgFlags & IP_DO_CHECKSUM_RCV)
            {
	    if ( (ip->ip_sum = in_cksum(m, hlen)) != 0) 
                {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_CRITICAL event */
                WV_NET_EVENT_0 (NET_CORE_EVENT, WV_NET_CRITICAL, 15, 6, 
                                WV_NETEVENT_IPIN_BADSUM, WV_NET_RECV)
#endif  /* INCLUDE_WVNET */
#endif

#ifdef VIRTUAL_STACK
		_ipstat.ips_badsum++;
#else
		ipstat.ips_badsum++;
#endif /* VIRTUAL_STACK */
		goto bad;
	        }
            }

	/*
	 * Convert fields to host representation.
	 */
	NTOHS(ip->ip_len);
	if (ip->ip_len < hlen) {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_CRITICAL event */
        WV_NET_ADDRIN_EVENT_2 (NET_CORE_EVENT, WV_NET_CRITICAL, 16, 7, 
                               ip->ip_src.s_addr, ip->ip_dst.s_addr,
                               WV_NETEVENT_IPIN_BADLEN, WV_NET_RECV,
                               ip->ip_len, hlen)
#endif  /* INCLUDE_WVNET */
#endif

#ifdef VIRTUAL_STACK
		_ipstat.ips_badlen++;
#else
		ipstat.ips_badlen++;
#endif /* VIRTUAL_STACK */
		goto bad;
	}
	NTOHS(ip->ip_id);
	NTOHS(ip->ip_off);

	/*
	 * Check that the amount of data in the buffers
	 * is as at least much as the IP header would have us expect.
	 * Trim mbufs if longer than we expect.
	 * Drop packet if shorter than we expect.
	 */
	if (m->m_pkthdr.len < ip->ip_len) {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_CRITICAL event */
        WV_NET_ADDRIN_EVENT_2 (NET_CORE_EVENT, WV_NET_CRITICAL, 17, 8,
                               ip->ip_src.s_addr, ip->ip_dst.s_addr,
                               WV_NETEVENT_IPIN_BADMBLK, WV_NET_RECV,
                               m->m_pkthdr.len, ip->ip_len)
#endif  /* INCLUDE_WVNET */
#endif

#ifdef VIRTUAL_STACK
		_ipstat.ips_tooshort++;
#else
		ipstat.ips_tooshort++;
#endif /* VIRTUAL_STACK */
		goto bad;
	}
	if (m->m_pkthdr.len > ip->ip_len) {
		if (m->m_len == m->m_pkthdr.len) {
			m->m_len = ip->ip_len;
			m->m_pkthdr.len = ip->ip_len;
		} else
			m_adj(m, ip->ip_len - m->m_pkthdr.len);
	}
   /* IPsec filter Hook */
   if (_func_ipsecFilterHook != NULL)
      {
      if (_func_ipsecFilterHook (&m, &ip, hlen) == ERROR)
         {
         goto done; /* IPsec filter hook has already freed the mbuf */
         }
      }
	
	/* ipFilterHook example fireWall Hook processing */

   if (_ipFilterHook != NULL)
     {
	    /* process the packet if the hook returns other than TRUE */

     if ((*_ipFilterHook) (m->m_pkthdr.rcvif, &m, &ip, hlen) == TRUE)
        goto done;	/* the hook should free the mbuf */
     }

	/*
	 * Process options and, if not destined for us,
	 * ship it on.  ip_dooptions returns 1 when an
	 * error was detected (causing an icmp message
	 * to be sent and the original packet to be freed).
	 */
	ip_nhops = 0;		/* for source routed packets */
	if (hlen > sizeof (struct ip) && ip_dooptions(m))
		goto done;

	/*
	 * Check our list of addresses, to see if the packet is for us.
	 */
#ifdef VIRTUAL_STACK
	for (ia = _in_ifaddr; ia; ia = ia->ia_next) {
#else
	for (ia = in_ifaddr; ia; ia = ia->ia_next) {
#endif /* VIRTUAL_STACK */
#define	satosin(sa)	((struct sockaddr_in *)(sa))
		/* if ip is strong ended we will accept packets on  an interface
		 * only if destination in ip hdr matches ip  address on the
		 * receiving interface and not just any interface. 
		 */
                if ((ipStrongEnded == TRUE) && 
		    (ia->ia_ifp != m->m_pkthdr.rcvif))
		    continue;

		if (IA_SIN(ia)->sin_addr.s_addr == ip->ip_dst.s_addr)
			goto ours;
		if (
#ifdef	DIRECTED_BROADCAST
		    ia->ia_ifp == m->m_pkthdr.rcvif &&
#endif
		    (ia->ia_ifp->if_flags & IFF_BROADCAST)) {
			u_long t;

			if (satosin(&ia->ia_broadaddr)->sin_addr.s_addr ==
			    ip->ip_dst.s_addr)
				goto ours;
			if (ip->ip_dst.s_addr == ia->ia_netbroadcast.s_addr)
				goto ours;
			/*
			 * Look for all-0's host part (old broadcast addr),
			 * either for subnet or net.
			 */
			t = ntohl(ip->ip_dst.s_addr);
			if (t == ia->ia_subnet)
				goto ours;
			if (t == ia->ia_net)
				goto ours;
		}
	}
	if (IN_MULTICAST(ntohl(ip->ip_dst.s_addr))) {
		struct in_multi *inm;
		if (_mCastRouteFwdHook != NULL) {
			/*
			 * If we are acting as a multicast router, all
			 * incoming multicast packets are passed to the
			 * kernel-level multicast forwarding function.
			 * The packet is returned (relatively) intact; if
			 * ip_mforward() returns a non-zero value, the packet
			 * must be discarded, else it may be accepted below.
			 *
			 * (The IP ident field is put in the same byte order
			 * as expected when ip_mforward() is called from
			 * ip_output().)
			 */
			ip->ip_id = htons(ip->ip_id);
                        if ((*_mCastRouteFwdHook)(m, m->m_pkthdr.rcvif, ip, NULL) != 0) {
#ifdef VIRTUAL_STACK
				_ipstat.ips_cantforward++;
#else
				ipstat.ips_cantforward++;
#endif /* VIRTUAL_STACK */
				m_freem(m);
				goto done;
			}
			ip->ip_id = ntohs(ip->ip_id);

			/*
			 * The process-level routing demon needs to receive
			 * all multicast IGMP packets, whether or not this
			 * host belongs to their destination groups.
			 */
			if (ip->ip_p == IPPROTO_IGMP)
				goto ours;
#ifdef VIRTUAL_STACK
			_ipstat.ips_forward++;
#else
			ipstat.ips_forward++;
#endif /* VIRTUAL_STACK */
		}
		/*
		 * See if we belong to the destination multicast group on the
		 * arrival interface.
		 */
		IN_LOOKUP_MULTI(ip->ip_dst, m->m_pkthdr.rcvif, inm);
		if (inm == NULL) {
#ifdef VIRTUAL_STACK
			_ipstat.ips_cantforward++;
#else
			ipstat.ips_cantforward++;
#endif /* VIRTUAL_STACK */
			m_freem(m);
			goto done;
		}
		goto ours;
	}
	if (ip->ip_dst.s_addr == (u_long)INADDR_BROADCAST)
		goto ours;
	if (ip->ip_dst.s_addr == INADDR_ANY)
		goto ours;

	/*
	 * Not for us; forward if possible and desirable.
	 */
	if ((_ipCfgFlags & IP_DO_FORWARDING) == 0) {
#ifdef VIRTUAL_STACK
		_ipstat.ips_cantforward++;
#else
		ipstat.ips_cantforward++;
#endif /* VIRTUAL_STACK */
		m_freem(m);
	} else
		ip_forward(m, 0);
	goto done;

ours:
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_INFO event */
    WV_NET_ADDRIN_EVENT_2 (NET_CORE_EVENT, WV_NET_INFO, 6, 16,
                           ip->ip_src.s_addr, ip->ip_dst.s_addr,
                           WV_NETEVENT_IPIN_GOODMBLK, WV_NET_RECV,
                           ip->ip_src.s_addr, ip->ip_dst.s_addr)
#endif  /* INCLUDE_WVNET */
#endif

	/*
	 * If offset or IP_MF are set, must reassemble.
	 * Otherwise, nothing need be done.
	 * (We could look in the reassembly queue to see
	 * if the packet was previously fragmented,
	 * but it's not worth the time; just let them time out.)
	 */
	if (ip->ip_off &~ IP_DF) {
		/*
		 * Look for queue of fragments
		 * of this datagram.
		 */
#ifdef VIRTUAL_STACK
		for (fp = _ipq.next; fp != &_ipq; fp = fp->next)
#else
		for (fp = ipq.next; fp != &ipq; fp = fp->next)
#endif
			if (ip->ip_id == fp->ipq_id &&
			    ip->ip_src.s_addr == fp->ipq_src.s_addr &&
			    ip->ip_dst.s_addr == fp->ipq_dst.s_addr &&
			    ip->ip_p == fp->ipq_p)
				goto found;
		fp = 0;
found:

		/*
		 * Adjust ip_len to not reflect header,
		 * set ip_mff if more fragments are expected,
		 * convert offset of this to bytes.
		 */
		ip->ip_len -= hlen;
		((struct ipasfrag *)ip)->ipf_mff &= ~1;
		if (ip->ip_off & IP_MF)
			((struct ipasfrag *)ip)->ipf_mff |= 1;
		ip->ip_off <<= 3;

		/*
		 * If datagram marked as having more fragments
		 * or if this is not the first fragment,
		 * attempt reassembly; if it succeeds, proceed.
		 */
		if (((struct ipasfrag *)ip)->ipf_mff & 1 || ip->ip_off) {
#ifdef VIRTUAL_STACK
			_ipstat.ips_fragments++;
#else
			ipstat.ips_fragments++;
#endif /* VIRTUAL_STACK */
			if ((m = ipReAssemble (m, fp)) == NULL)
				goto done;
                        ip = mtod (m, struct ip *);
			hlen = ip->ip_hl << 2;
#ifdef VIRTUAL_STACK
			_ipstat.ips_reassembled++;
#else
			ipstat.ips_reassembled++;
#endif /* VIRTUAL_STACK */
		} else
			if (fp)
				ip_freef(fp);
	} else
		ip->ip_len -= hlen;

#ifdef  FORWARD_BROADCASTS
        if ((m->m_flags & M_BCAST) && (proxyBroadcastHook != NULL))
            (* proxyBroadcastHook) (m, m->m_pkthdr.rcvif);
#endif
        if (_func_ipsecInput != NULL)
            {
            /* Deliver the packet to ipsec for further processing/filtering. */
            if (_func_ipsecInput (&m, hlen, &ip) == ERROR)
                {
                goto done;
                }
            }
	/*
	 * Switch out to protocol's input routine.
	 */

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_NOTICE event */
    WV_NET_ADDRIN_EVENT_2 (NET_CORE_EVENT, WV_NET_NOTICE, 10, 14,
                           ip->ip_src.s_addr, ip->ip_dst.s_addr,
                           WV_NETEVENT_IPIN_FINISH, WV_NET_RECV,
                           ip->ip_src.s_addr, ip->ip_dst.s_addr)
#endif  /* INCLUDE_WVNET */
#endif

#ifdef VIRTUAL_STACK
	_ipstat.ips_delivered++;
#else
	ipstat.ips_delivered++;
#endif /* VIRTUAL_STACK */
	(*inetsw[ip_protox[ip->ip_p]].pr_input) (m, hlen);
        splx (s);
        return;

bad:
	m_freem(m);
done:        
        splx (s);
        return;
        }

/*******************************************************************************
*
* ipReAssemble - reassemble ip fragments
*
* This function reassembles the ip fragments and returns the pointer to the
* mbuf chain if reassembly is complete orelse returns null.
*
* RETURNS: pMbuf/NULL
*
* SEE ALSO: ,
*
* NOMANUAL
*/

LOCAL struct mbuf * ipReAssemble
    (
    FAST struct mbuf *	pMbuf,		/* pointer to mbuf chain fragment */
    FAST struct ipq *	pIpFragQueue	/* pointer to ip fragment queue */
    )
    {
    FAST struct mbuf ** 	pPtrMbuf; 	   /* pointer to ptr to mbuf */
    FAST struct mbuf * 		pMbPktFrag = NULL; /* pointer to the packet */
    FAST struct ip * 		pIpHdr;   	   /* pointer to ip header */
    FAST struct ipasfrag *	pIpHdrFrag = NULL; /* ipfragment header */
    FAST int		        len; 
    FAST struct mbuf *		pMbufTmp;	   /* pointer to mbuf */
    
    pIpHdr = mtod (pMbuf, struct ip *); 
    pMbuf->m_nextpkt = NULL; 
    /*
     * If first fragment to arrive, create a reassembly queue.
     */
    if (pIpFragQueue == 0)
	{
	if ((pMbufTmp = mBufClGet (M_DONTWAIT, MT_FTABLE, sizeof(struct ipq),
				   TRUE)) == NULL)
	    goto dropFrag;
	pIpFragQueue = mtod(pMbufTmp, struct ipq *);
#ifdef VIRTUAL_STACK
	insque(pIpFragQueue, &_ipq);
#else
	insque(pIpFragQueue, &ipq);
#endif
	pIpFragQueue->ipq_ttl = ipfragttl;	/* configuration parameter */
	pIpFragQueue->ipq_p = pIpHdr->ip_p;
	pIpFragQueue->ipq_id = pIpHdr->ip_id;
	pIpFragQueue->ipq_src = ((struct ip *)pIpHdr)->ip_src;
	pIpFragQueue->ipq_dst = ((struct ip *)pIpHdr)->ip_dst;
	pIpFragQueue->pMbufHdr   = pMbufTmp; 	/* back pointer to mbuf */
	pIpFragQueue->pMbufPkt = pMbuf; 	/* first fragment received */
	goto ipChkReAssembly; 
	}

    for (pPtrMbuf = &(pIpFragQueue->pMbufPkt); *pPtrMbuf != NULL;
	 pPtrMbuf = &(*pPtrMbuf)->m_nextpkt)
	{
	pMbPktFrag = *pPtrMbuf; 
	pIpHdrFrag = mtod(pMbPktFrag, struct ipasfrag *); 

	if ((USHORT)pIpHdr->ip_off > (USHORT)pIpHdrFrag->ip_off)
	    {
	    if ((len = (signed int)((USHORT)pIpHdrFrag->ip_off +
				    pIpHdrFrag->ip_len -
				    (USHORT)pIpHdr->ip_off)) > 0)
		{
		if (len >= pIpHdr->ip_len)
		    goto dropFrag;		/* drop the fragment */
		pIpHdrFrag->ip_len -= len; 
		m_adj(pMbPktFrag, -len); 	/* trim from the tail */
		}
	    if (pMbPktFrag->m_nextpkt == NULL)
		{ 	/* this is the likely case most of the time */
		pMbPktFrag->m_nextpkt = pMbuf; 	/* insert the fragment */
		pMbuf->m_nextpkt = NULL;
		break;
		}
	    }
	else 		/* if pIpHdr->ip_off <= pIpHdrFrag->ip_off */
	    {
            /* if complete overlap */
            while (((USHORT)pIpHdr->ip_off + pIpHdr->ip_len) >=
                   ((USHORT)pIpHdrFrag->ip_off + pIpHdrFrag->ip_len))
                {
                *pPtrMbuf = (*pPtrMbuf)->m_nextpkt;
                pMbPktFrag->m_nextpkt = NULL;
                m_freem (pMbPktFrag);           /* drop the fragment */
                pMbPktFrag = *pPtrMbuf;
                if (pMbPktFrag == NULL)
                    break;
                pIpHdrFrag = mtod(pMbPktFrag, struct ipasfrag *);
                }
            /* if partial overlap trim my data at the end*/
            if ((pMbPktFrag != NULL) &&
                ((len = (((USHORT) pIpHdr->ip_off + pIpHdr->ip_len) -
                         (USHORT) pIpHdrFrag->ip_off)) > 0))
                {
                pIpHdr->ip_len -= len;
                m_adj (pMbuf, -len);        	/* trim from the tail */
                }
	    pMbuf->m_nextpkt = pMbPktFrag; 
	    *pPtrMbuf = pMbuf; 	/* insert the current fragment */	    
	    break; 
	    }
	}

    ipChkReAssembly:
    len = 0; 
    for (pMbPktFrag = pIpFragQueue->pMbufPkt; pMbPktFrag != NULL;
	 pMbPktFrag = pMbPktFrag->m_nextpkt)
	{
	pIpHdrFrag = mtod(pMbPktFrag, struct ipasfrag *); 
	if ((USHORT)pIpHdrFrag->ip_off != len)
	    return (NULL); 
	len += pIpHdrFrag->ip_len; 
	}    
    if (pIpHdrFrag->ipf_mff & 1)	/* last fragment's mff bit still set */
	return (NULL); 

    /* reassemble and concatenate all fragments */
    pMbuf = pIpFragQueue->pMbufPkt; 
    pMbufTmp = pMbuf->m_nextpkt; 
    pMbuf->m_nextpkt = NULL; 

    while (pMbufTmp != NULL)
	{
	pMbPktFrag = pMbufTmp; 
	pIpHdrFrag = mtod(pMbPktFrag, struct ipasfrag *); 
	pMbPktFrag->m_data += pIpHdrFrag->ip_hl << 2;	/* remove the header */
	pMbPktFrag->m_len  -= pIpHdrFrag->ip_hl << 2;
	pMbufTmp = pMbPktFrag->m_nextpkt; 
	pMbPktFrag->m_nextpkt = NULL ; 
	m_cat (pMbuf, pMbPktFrag); 	/* concatenate the fragment */
	}

    pIpHdrFrag = mtod(pMbuf, struct ipasfrag *); 
    pIpHdrFrag->ip_len = len; 		/* length of the ip packet */
    if (len > 0xffff - (pIpHdrFrag->ip_hl << 2))  /* ping of death */
        goto dropFrag;                            /* drop entire chain */
    pIpHdrFrag->ipf_mff &= ~1;
    remque(pIpFragQueue);
    (void) m_free (pIpFragQueue->pMbufHdr); 

    /* some debugging cruft by sklower, below, will go away soon */
    if (pMbuf->m_flags & M_PKTHDR)
	{ /* XXX this should be done elsewhere */
	len = 0; 
	for (pMbufTmp = pMbuf; pMbufTmp; pMbufTmp = pMbufTmp->m_next)
	    len += pMbufTmp->m_len;
	pMbuf->m_pkthdr.len = len;
	}

    return (pMbuf);			/* return the assembled packet */

    dropFrag:
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_WARNING event */
    WV_NET_ADDRIN_EVENT_1 (NET_CORE_EVENT, WV_NET_WARNING, 8, 13,
                           pIpHdr->ip_src.s_addr, pIpHdr->ip_dst.s_addr,
                           WV_NETEVENT_IPIN_FRAGDROP, WV_NET_RECV, pIpHdrFrag)
#endif  /* INCLUDE_WVNET */
#endif

#ifdef VIRTUAL_STACK
    _ipstat.ips_fragdropped++;
#else
    ipstat.ips_fragdropped++;
#endif /* VIRTUAL_STACK */
    m_freem (pMbuf);			/* free the fragment */
    return (NULL);
    }

/*
 * Free a fragment reassembly header and all
 * associated datagrams.
 */
void
ip_freef(fp)
	struct ipq *fp;
{
	struct mbuf **  pPtrMbuf; 
	struct mbuf *  pMbuf;

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_INFO event */
    WV_NET_ADDRIN_MARKER_2 (NET_AUX_EVENT, WV_NET_INFO, 7, 17,
                            fp->ipq_src.s_addr, fp->ipq_dst.s_addr,
                            WV_NETEVENT_IPFRAGFREE_START,
                            fp->ipq_src.s_addr, fp->ipq_dst.s_addr)
#endif  /* INCLUDE_WVNET */
#endif

	/* free all fragments */
	pPtrMbuf = &(fp->pMbufPkt); 
	while (*pPtrMbuf)
	    {
	    pMbuf = (*pPtrMbuf)->m_nextpkt; 
	    m_freem (*pPtrMbuf); 
	    *pPtrMbuf = pMbuf; 
	    }
	remque(fp);
	(void) m_free(fp->pMbufHdr);
}

n_time
iptime()
{
	u_long t;

	/* t is time in milliseconds (i hope) */

	t = ((int) tickGet () * 1000) / sysClkRateGet ();

	return (htonl(t));
}

/*
 * IP timer processing;
 * if a timer expires on a reassembly
 * queue, discard it.
 */
void
ip_slowtimo()
{
	register struct ipq *fp;
	int s = splnet();

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_INFO event */
    WV_NET_MARKER_0 (NET_AUX_EVENT, WV_NET_INFO, 8, 18, 
                     WV_NETEVENT_IPTIMER_FRAGFREE)
#endif  /* INCLUDE_WVNET */
#endif

#ifdef VIRTUAL_STACK
	fp = _ipq.next;
#else
	fp = ipq.next;
#endif
	if (fp == 0) {
		splx(s);
		return;
	}
#ifdef VIRTUAL_STACK
	while (fp != &_ipq) {
#else
	while (fp != &ipq) {
#endif
		--fp->ipq_ttl;
		fp = fp->next;
		if (fp->prev->ipq_ttl == 0) {
#ifdef VIRTUAL_STACK
			_ipstat.ips_fragtimeout++;
#else
			ipstat.ips_fragtimeout++;
#endif /* VIRTUAL_STACK */
			ip_freef(fp->prev);
		}
	}
	splx(s);
	return;
}

/*
 * Drain off all datagram fragments and excess (unused) route entries.
 */
void
ip_drain()
{

#ifdef VIRTUAL_STACK
	while (_ipq.next != &_ipq) {
		_ipstat.ips_fragdropped++;
		ip_freef(_ipq.next);
	}
#else
	while (ipq.next != &ipq) {
		ipstat.ips_fragdropped++;
		ip_freef(ipq.next);
	}
#endif /* VIRTUAL_STACK */

        routeDrain();
	return;
}

/*
 * Do option processing on a datagram,
 * possibly discarding it if bad options are encountered,
 * or forwarding it if source-routed.
 * Returns 1 if packet has been forwarded/freed,
 * 0 if the packet should be processed further.
 */
int
ip_dooptions(m)
	struct mbuf *m;
{
	register struct ip *ip = mtod(m, struct ip *);
	register u_char *cp;
	register struct ip_timestamp *ipt;
	register struct in_ifaddr *ia;
	int opt, optlen, cnt, off, code, type = ICMP_PARAMPROB, forward = 0;
	struct in_addr *sin, dst;
	n_time ntime;

	dst = ip->ip_dst;
	cp = (u_char *)(ip + 1);
	cnt = (ip->ip_hl << 2) - sizeof (struct ip);
	for (; cnt > 0; cnt -= optlen, cp += optlen) {
		opt = cp[IPOPT_OPTVAL];
		if (opt == IPOPT_EOL)
			break;
		if (opt == IPOPT_NOP)
			optlen = 1;
		else {
			optlen = cp[IPOPT_OLEN];
			if (optlen <= 0 || optlen > cnt) {
				code = &cp[IPOPT_OLEN] - (u_char *)ip;
				goto bad;
			}
		}
		switch (opt) {

		default:
			break;

		/*
		 * Source routing with record.
		 * Find interface with current destination address.
		 * If none on this machine then drop if strictly routed,
		 * or do nothing if loosely routed.
		 * Record interface address and bring up next address
		 * component.  If strictly routed make sure next
		 * address is on directly accessible net.
		 */
		case IPOPT_LSRR:
		case IPOPT_SSRR:
			if ((off = cp[IPOPT_OFFSET]) < IPOPT_MINOFF) {
				code = &cp[IPOPT_OFFSET] - (u_char *)ip;
				goto bad;
			}
			input_ipaddr.sin_addr = ip->ip_dst;
			TOS_SET (&input_ipaddr, ip->ip_tos);  /* tos routing */

			ia = (struct in_ifaddr *)
				ifa_ifwithaddr((struct sockaddr *)&input_ipaddr);
			if (ia == 0) {
				if (opt == IPOPT_SSRR) {
					type = ICMP_UNREACH;
					code = ICMP_UNREACH_SRCFAIL;
					goto bad;
				}
				/*
				 * Loose routing, and not at next destination
				 * yet; nothing to do except forward.
				 */
				break;
			}
			off--;			/* 0 origin */
			if (off > optlen - sizeof(struct in_addr)) {
				/*
				 * End of source route.  Should be for us.
				 */
#ifdef SRCRT
				save_rte(cp, ip->ip_src);
#endif /* SRCRT */
				break;
			}
			/*
			 * locate outgoing interface
			 */
			bcopy((caddr_t)(cp + off), (caddr_t)&input_ipaddr.sin_addr,
			    sizeof(input_ipaddr.sin_addr));
			if (opt == IPOPT_SSRR) {
#define	INA	struct in_ifaddr *
#define	SA	struct sockaddr *
			    if ((ia = (INA)ifa_ifwithdstaddr((SA)&input_ipaddr)) == 0)
				ia = (INA)ifa_ifwithnet((SA)&input_ipaddr);
			} else
				ia = ip_rtaddr(input_ipaddr.sin_addr);
			if (ia == 0) {
				type = ICMP_UNREACH;
				code = ICMP_UNREACH_SRCFAIL;
				goto bad;
			}
			ip->ip_dst = input_ipaddr.sin_addr;
			bcopy((caddr_t)&(IA_SIN(ia)->sin_addr),
			    (caddr_t)(cp + off), sizeof(struct in_addr));
			cp[IPOPT_OFFSET] += sizeof(struct in_addr);
			/*
			 * Let ip_intr's mcast routing check handle mcast pkts
			 */
			forward = !IN_MULTICAST(ntohl(ip->ip_dst.s_addr));
			break;

		case IPOPT_RR:
			if ((off = cp[IPOPT_OFFSET]) < IPOPT_MINOFF) {
				code = &cp[IPOPT_OFFSET] - (u_char *)ip;
				goto bad;
			}
			/*
			 * If no space remains, ignore.
			 */
			off--;			/* 0 origin */
			if (off > optlen - sizeof(struct in_addr))
				break;
			bcopy((caddr_t)(&ip->ip_dst), (caddr_t)&input_ipaddr.sin_addr,
			    sizeof(input_ipaddr.sin_addr));
			/*
			 * locate outgoing interface; if we're the destination,
			 * use the incoming interface (should be same).
			 */
			if ((ia = (INA)ifa_ifwithaddr((SA)&input_ipaddr)) == 0 &&
			    (ia = ip_rtaddr(input_ipaddr.sin_addr)) == 0) {
				type = ICMP_UNREACH;
				code = ICMP_UNREACH_HOST;
				goto bad;
			}
			bcopy((caddr_t)&(IA_SIN(ia)->sin_addr),
			    (caddr_t)(cp + off), sizeof(struct in_addr));
			cp[IPOPT_OFFSET] += sizeof(struct in_addr);
			break;

		case IPOPT_TS:
			code = cp - (u_char *)ip;
			ipt = (struct ip_timestamp *)cp;
			if (ipt->ipt_len < 5)
				goto bad;
			if (ipt->ipt_ptr > ipt->ipt_len - sizeof (long)) {
				if (++ipt->ipt_oflw == 0)
					goto bad;
				break;
			}
			sin = (struct in_addr *)(cp + ipt->ipt_ptr - 1);
			switch (ipt->ipt_flg) {

			case IPOPT_TS_TSONLY:
				break;

			case IPOPT_TS_TSANDADDR:
				if (ipt->ipt_ptr + sizeof(n_time) +
				    sizeof(struct in_addr) > ipt->ipt_len)
					goto bad;
				input_ipaddr.sin_addr = dst;
				ia = (INA)ifaof_ifpforaddr((SA)&input_ipaddr,
							    m->m_pkthdr.rcvif);
				if (ia == 0)
					continue;
				bcopy((caddr_t)&IA_SIN(ia)->sin_addr,
				    (caddr_t)sin, sizeof(struct in_addr));
				ipt->ipt_ptr += sizeof(struct in_addr);
				break;

			case IPOPT_TS_PRESPEC:
				if (ipt->ipt_ptr + sizeof(n_time) +
				    sizeof(struct in_addr) > ipt->ipt_len)
					goto bad;
				bcopy((caddr_t)sin, (caddr_t)&input_ipaddr.sin_addr,
				    sizeof(struct in_addr));
				if (ifa_ifwithaddr((SA)&input_ipaddr) == 0)
					continue;
				ipt->ipt_ptr += sizeof(struct in_addr);
				break;

			default:
				goto bad;
			}
			ntime = iptime();
			bcopy((caddr_t)&ntime, (caddr_t)cp + ipt->ipt_ptr - 1,
			    sizeof(n_time));
			ipt->ipt_ptr += sizeof(n_time);
		}
	}
	if (forward) {
		ip_forward(m, 1);
		return (1);
	}
	return (0);
bad:
	ip->ip_len -= ip->ip_hl << 2;   /* XXX icmp_error adds in hdr length */
	if (_icmpErrorHook != NULL)
	    (*_icmpErrorHook) (m, type, code, 0, 0); 

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_CRITICAL event */
        WV_NET_EVENT_2 (NET_CORE_EVENT, WV_NET_CRITICAL, 18, 9, 
                        WV_NETEVENT_IPIN_BADOPT, WV_NET_RECV, type, code)
#endif  /* INCLUDE_WVNET */
#endif

#ifdef VIRTUAL_STACK
	_ipstat.ips_badoptions++;
#else
	ipstat.ips_badoptions++;
#endif /* VIRTUAL_STACK */
	return (1);
}

/*
 * Given address of next destination (final or next hop),
 * return internet address info of interface to be used to get there.
 */
struct in_ifaddr *
ip_rtaddr(dst)
	 struct in_addr dst;
{
	register struct sockaddr_in *sin;

	sin = (struct sockaddr_in *) &ipforward_rt.ro_dst;

	if (ipforward_rt.ro_rt == 0 ||
            (ipforward_rt.ro_rt->rt_flags & RTF_UP) == 0 ||
            dst.s_addr != sin->sin_addr.s_addr) {
		if (ipforward_rt.ro_rt) {
			RTFREE(ipforward_rt.ro_rt);
			ipforward_rt.ro_rt = 0;
		}
		sin->sin_family = AF_INET;
		sin->sin_len = sizeof(*sin);
		sin->sin_addr = dst;

		ipforward_rt.ro_rt = rtalloc2 (&ipforward_rt.ro_dst);
	}
	if (ipforward_rt.ro_rt == 0)
		return ((struct in_ifaddr *)0);
	return ((struct in_ifaddr *) ipforward_rt.ro_rt->rt_ifa);
}

#ifdef SRCRT
/*
 * Save incoming source route for use in replies,
 * to be picked up later by ip_srcroute if the receiver is interested.
 */
void
save_rte(option, dst)
	u_char *option;
	struct in_addr dst;
{
	unsigned olen;

	olen = option[IPOPT_OLEN];
#ifdef DIAGNOSTIC
	if (ipprintfs)
		printf("save_rte: olen %d\n", olen);
#endif
	if (olen > sizeof(ip_srcrt) - (1 + sizeof(dst)))
		return;
	bcopy((caddr_t)option, (caddr_t)ip_srcrt.srcopt, olen);
	ip_nhops = (olen - IPOPT_OFFSET - 1) / sizeof(struct in_addr);
	ip_srcrt.dst = dst;
}

/*
 * Retrieve incoming source route for use in replies,
 * in the same form used by setsockopt.
 * The first hop is placed before the options, will be removed later.
 */
struct mbuf *
ip_srcroute()
{
	register struct in_addr *p, *q;
	register struct mbuf *m;

	if (ip_nhops == 0)
		return ((struct mbuf *)0);
	m= mBufClGet(M_DONTWAIT, MT_SOOPTS, CL_SIZE_128, TRUE);
	if (m == 0)
		return ((struct mbuf *)0);

#define OPTSIZ	(sizeof(ip_srcrt.nop) + sizeof(ip_srcrt.srcopt))

	/* length is (nhops+1)*sizeof(addr) + sizeof(nop + srcrt header) */
	m->m_len = ip_nhops * sizeof(struct in_addr) + sizeof(struct in_addr) +
	    OPTSIZ;
#ifdef DIAGNOSTIC
	if (ipprintfs)
		printf("ip_srcroute: nhops %d mlen %d", ip_nhops, m->m_len);
#endif

	/*
	 * First save first hop for return route
	 */
	p = &ip_srcrt.route[ip_nhops - 1];
	*(mtod(m, struct in_addr *)) = *p--;
#ifdef DIAGNOSTIC
	if (ipprintfs)
		printf(" hops %lx", ntohl(mtod(m, struct in_addr *)->s_addr));
#endif

	/*
	 * Copy option fields and padding (nop) to mbuf.
	 */
	ip_srcrt.nop = IPOPT_NOP;
	ip_srcrt.srcopt[IPOPT_OFFSET] = IPOPT_MINOFF;
	bcopy((caddr_t)&ip_srcrt.nop,
	    mtod(m, caddr_t) + sizeof(struct in_addr), OPTSIZ);
	q = (struct in_addr *)(mtod(m, caddr_t) +
	    sizeof(struct in_addr) + OPTSIZ);
#undef OPTSIZ
	/*
	 * Record return path as an IP source route,
	 * reversing the path (pointers are now aligned).
	 */
	while (p >= ip_srcrt.route) {
#ifdef DIAGNOSTIC
		if (ipprintfs)
			printf(" %lx", ntohl(q->s_addr));
#endif
		*q++ = *p--;
	}
	/*
	 * Last hop goes to final destination.
	 */
	*q = ip_srcrt.dst;
#ifdef DIAGNOSTIC
	if (ipprintfs)
		printf(" %lx\n", ntohl(q->s_addr));
#endif
	return (m);
}

#endif /* SRCRT */

/*
 * Strip out IP options, at higher
 * level protocol in the kernel.
 * Second argument is buffer to which options
 * will be moved, and return value is their length.
 * XXX should be deleted; last arg currently ignored.
 */
void
ip_stripoptions(m, mopt)
	register struct mbuf *m;
	struct mbuf *mopt;
{
	register int i;
	struct ip *ip = mtod(m, struct ip *);
	register caddr_t opts;
	int olen;

	olen = (ip->ip_hl<<2) - sizeof (struct ip);
	opts = (caddr_t)(ip + 1);
	i = m->m_len - (sizeof (struct ip) + olen);
	bcopy(opts  + olen, opts, (unsigned)i);
	m->m_len -= olen;
	if (m->m_flags & M_PKTHDR)
		m->m_pkthdr.len -= olen;
	ip->ip_hl = sizeof(struct ip) >> 2;
}

u_char inetctlerrmap[PRC_NCMDS] = {
	0,		0,		0,		0,
	0,		EMSGSIZE,	EHOSTDOWN,	EHOSTUNREACH,
	EHOSTUNREACH,	EHOSTUNREACH,	ECONNREFUSED,	ECONNREFUSED,
	EMSGSIZE,	EHOSTUNREACH,	0,		0,
	0,		0,		0,		0,
	ENOPROTOOPT
};

/*
 * Forward a packet.  If some error occurs return the sender
 * an icmp packet.  Note we can't always generate a meaningful
 * icmp message because icmp doesn't have a large enough repertoire
 * of codes and types.
 *
 * If not forwarding, just drop the packet.  This could be confusing
 * if (_ipCfgFlags & IP_DO_FORWARDING) was zero but some routing protocol was
 * advancing us as a gateway to somewhere.  However, we must let the routing
 * protocol deal with that.
 *
 * The srcrt parameter indicates whether the packet is being forwarded
 * via a source route.
 */
void
ip_forward(m, srcrt)
	struct mbuf *m;
	int srcrt;
{
	register struct ip *ip = mtod(m, struct ip *);
	register struct sockaddr_in *sin;
	register struct rtentry *rt;
	int error, type = 0, code = 0;
	struct mbuf *mcopy;
	n_long dest;
	struct ifnet *destifp;

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET
        u_long srcAddr; /* Source IP address, unavailable after ip_output() */
        u_long dstAddr; /* Dest. IP address, unavailable after ip_output() */
#endif
#endif

	dest = 0;
#ifdef DIAGNOSTIC
	if (ipprintfs)
		printf("forward: src %x dst %x ttl %x\n", ip->ip_src,
			ip->ip_dst, ip->ip_ttl);
#endif
	if (m->m_flags & M_BCAST || in_canforward(ip->ip_dst) == 0) {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_CRITICAL event */
    WV_NET_ADDROUT_EVENT_1 (NET_CORE_EVENT, WV_NET_CRITICAL, 19, 10, 
                            ip->ip_src.s_addr, ip->ip_dst.s_addr,
                            WV_NETEVENT_IPFWD_BADADDR, WV_NET_RECV,
                            ip->ip_dst.s_addr)
#endif  /* INCLUDE_WVNET */
#endif
		
#ifdef VIRTUAL_STACK
		_ipstat.ips_cantforward++;
#else
		ipstat.ips_cantforward++;
#endif /* VIRTUAL_STACK */
		m_freem(m);
		return;
	}
	HTONS(ip->ip_id);
	if (ip->ip_ttl <= IPTTLDEC) {
		if (_icmpErrorHook != NULL)
                    {
                    HTONS(ip->ip_len);
                    (*_icmpErrorHook)(m, ICMP_TIMXCEED, ICMP_TIMXCEED_INTRANS,
                                      dest, 0);
                    }
		return;
	}
	ip->ip_ttl -= IPTTLDEC;

	sin = (struct sockaddr_in *)&ipforward_rt.ro_dst;
	if ((rt = ipforward_rt.ro_rt) == 0 ||
            (rt->rt_flags & RTF_UP) == 0 ||
	    ip->ip_dst.s_addr != sin->sin_addr.s_addr) {
		if (ipforward_rt.ro_rt) {
			RTFREE(ipforward_rt.ro_rt);
			ipforward_rt.ro_rt = 0;
		}
		sin->sin_family = AF_INET;
		sin->sin_len = sizeof(*sin);
		sin->sin_addr = ip->ip_dst;

		ipforward_rt.ro_rt = rtalloc2 (&ipforward_rt.ro_dst);
		if (ipforward_rt.ro_rt == 0) {
#ifdef VIRTUAL_STACK
			_ipstat.ips_noroute++;
#else
			ipstat.ips_noroute++;
#endif /* VIRTUAL_STACK */
			if (_icmpErrorHook != NULL)
			    (*_icmpErrorHook)(m, ICMP_UNREACH, 
					     ICMP_UNREACH_HOST, dest, 0);
			return;
		}
		rt = ipforward_rt.ro_rt;
	}

	/*
	 * Save at most 64 bytes of the packet in case
	 * we need to generate an ICMP message to the src.
	 */
	mcopy = m_copy(m, 0, min((int)ip->ip_len, 64));

	/*
	 * If forwarding packet using same interface that it came in on,
	 * perhaps should send a redirect to sender to shortcut a hop.
	 * Only send redirect if source is sending directly to us,
	 * and if packet was not source routed (or has any options).
	 * Also, don't send redirect if forwarding using a default route
	 * or a route modified by a redirect.
	 */
#define	satosin(sa)	((struct sockaddr_in *)(sa))
	if (rt->rt_ifp == m->m_pkthdr.rcvif &&
	    (rt->rt_flags & (RTF_DYNAMIC|RTF_MODIFIED)) == 0 &&
	    satosin(rt_key(rt))->sin_addr.s_addr != 0 &&
	    (_ipCfgFlags & IP_DO_REDIRECT) && !srcrt) {
#define	RTA(rt)	((struct in_ifaddr *)(rt->rt_ifa))
		u_long src = ntohl(ip->ip_src.s_addr);

		if (RTA(rt) &&
		    (src & RTA(rt)->ia_subnetmask) == RTA(rt)->ia_subnet) {
		    if (rt->rt_flags & RTF_GATEWAY)
			dest = satosin(rt->rt_gateway)->sin_addr.s_addr;
		    else
			dest = ip->ip_dst.s_addr;
		    /* Router requirements says to only send host redirects */
		    type = ICMP_REDIRECT;
		    code = ICMP_REDIRECT_HOST;
#ifdef DIAGNOSTIC
		    if (ipprintfs)
		        printf("redirect (%d) to %lx\n", code, (u_long)dest);
#endif
		}
	}

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET
        /*
         * Save the source and destination address from the original packet,
         * since they will be unavailable after the ip_output() call returns.
         */

        srcAddr = ip->ip_src.s_addr;
        dstAddr = ip->ip_dst.s_addr;
#endif
#endif

        /*
         * Set the M_FORWARD flag to nominate the route for the fast path.
         */

        m->m_flags |= M_FORWARD;

	error = ip_output(m, (struct mbuf *)0, &ipforward_rt, IP_FORWARDING
#ifdef DIRECTED_BROADCAST
			    | IP_ALLOWBROADCAST
#endif
                          , 0);

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_NOTICE event */
    WV_NET_ADDROUT_EVENT_3 (NET_CORE_EVENT, WV_NET_NOTICE, 11, 15,
                            srcAddr, dstAddr,
                            WV_NETEVENT_IPFWD_STATUS, WV_NET_SEND,
                            error, srcAddr, dstAddr)
#endif  /* INCLUDE_WVNET */
#endif

	if (error)
#ifdef VIRTUAL_STACK
		_ipstat.ips_cantforward++;
#else
		ipstat.ips_cantforward++;
#endif /* VIRTUAL_STACK */
	else {
#ifdef VIRTUAL_STACK
		_ipstat.ips_forward++;
#else
		ipstat.ips_forward++;
#endif /* VIRTUAL_STACK */
		if (type)
#ifdef VIRTUAL_STACK
			_ipstat.ips_redirectsent++;
#else
			ipstat.ips_redirectsent++;
#endif /* VIRTUAL_STACK */
		else {
			if (mcopy)
				m_freem(mcopy);
			return;
		}
	}
	if (mcopy == NULL)
		return;
	destifp = NULL;

	switch (error) {

	case 0:				/* forwarded, but need redirect */
		/* type, code set above */
		break;

	case ENETUNREACH:		/* shouldn't happen, checked above */
	case EHOSTUNREACH:
	case ENETDOWN:
	case EHOSTDOWN:
	default:
		type = ICMP_UNREACH;
		code = ICMP_UNREACH_HOST;
		break;

	case EMSGSIZE:
		type = ICMP_UNREACH;
		code = ICMP_UNREACH_NEEDFRAG;
		if (ipforward_rt.ro_rt)
			destifp = ipforward_rt.ro_rt->rt_ifp;
#ifdef VIRTUAL_STACK
		_ipstat.ips_cantfrag++;
#else
		ipstat.ips_cantfrag++;
#endif /* VIRTUAL_STACK */
		break;

	case ENOBUFS:
		type = ICMP_SOURCEQUENCH;
		code = 0;
		break;
	}
	if (_icmpErrorHook != NULL)
	    (*_icmpErrorHook)(mcopy, type, code, dest, destifp);
}

#ifdef SYSCTL_SUPPORT
int
ip_sysctl(name, namelen, oldp, oldlenp, newp, newlen)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
{
/*
 * XXX - This event cannot currently occur: the ip_sysctl() routine
 *       is only called by the Unix sysctl command which is not supported
 *       by VxWorks

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /@ WV_NET_INFO event @/
    WV_NET_MARKER_1 (NET_AUX_EVENT, WV_NET_INFO, 9, 19, 
                     WV_NETEVENT_IPCTRL_START, name[0])
#endif  /@ INCLUDE_WVNET @/
#endif

 * XXX - end of unused event
 */ 

	/* All sysctl names at this level are terminal. */
	if (namelen != 1)
            {
/*
 * XXX - This event cannot currently occur: the ip_sysctl() routine
 *       is only called by the Unix sysctl command which is not supported
 *       by VxWorks

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /@ WV_NET_ERROR event @/
            WV_NET_MARKER_1 (NET_AUX_EVENT, WV_NET_ERROR, 20, 11, 
                             WV_NETEVENT_IPCTRL_BADCMDLEN, namelen)
#endif  /@ INCLUDE_WVNET @/
#endif

 * XXX - end of unused event
 */ 
            return (ENOTDIR);
            }

	switch (name[0]) {
	case IPCTL_FORWARDING:
		return (sysctl_int(oldp, oldlenp, newp, newlen, &ipforwarding));
	case IPCTL_SENDREDIRECTS:
		return (sysctl_int(oldp, oldlenp, newp, newlen,
			&ipsendredirects));
	case IPCTL_DEFTTL:
		return (sysctl_int(oldp, oldlenp, newp, newlen, &ip_defttl));
#ifdef notyet
	case IPCTL_DEFMTU:
		return (sysctl_int(oldp, oldlenp, newp, newlen, &ip_mtu));
#endif
	default:

/*
 * XXX - This event cannot currently occur: the ip_sysctl() routine
 *       is only called by the Unix sysctl command which is not supported
 *       by VxWorks

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /@ WV_NET_ERROR event @/
            WV_NET_MARKER_1 (NET_AUX_EVENT, WV_NET_ERROR, 21, 12, 
                             WV_NETEVENT_IPCTRL_BADCMD, name[0])
#endif  /@ INCLUDE_WVNET @/
#endif

 * XXX - end of unused event
 */ 

		return (EOPNOTSUPP);
	}
	/* NOTREACHED */
}
#endif /* SYSCTL_SUPPORT */
