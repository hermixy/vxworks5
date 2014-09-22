/* ip_icmp.c - internet ICMP routines */

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
 *	@(#)ip_icmp.c	8.2 (Berkeley) 1/4/94
 */

/*
modification history
--------------------
03h,18apr02,vvv  removed incorrect icmpmaskrepl initialization from icmp_init
		 (SPR #74338)
03g,18dec01,vvv  reflect messages with correct source address (SPR #71684)
03f,12oct01,rae  merge from truestack ver 03j, base 03e (SPRs 69344, 69683 etc)
03e,16mar99,spm  recovered orphaned code from tor2_0_x branch (SPR #25770)
03d,09feb99,ham  fixed SPR#24975.
03c,31jan97,vin  changed declaration according to prototype decl in protosw.h
03b,07jan96,vin  added icmp_init(..) for scalability, moved iptime to 
		 ip_input.c
03a,31oct96,vin  changed m_gethdr(..) to mHdrClGet(..).
02u,24aug96,vin  integrated with BSD44 and 02t of ip_icmp.c.
*/

/*
 * ICMP routines: error generation, receive packet processing, and
 * routines to turnaround packets back to the originator, and
 * host table maintenance routines.
 */

#include "vxWorks.h"
#include "net/mbuf.h"
#include "net/protosw.h"
#include "sys/socket.h"
#include "sys/ioctl.h"
#include "net/systm.h"
#include "net/route.h"
#include "net/if.h"
#include "net/if_types.h"

#include "netinet/in.h"
#include "netinet/in_systm.h"
#include "netinet/in_var.h"
#include "netinet/ip.h"
#include "netinet/ip_var.h"
#include "netinet/ip_icmp.h"
#include "netinet/icmp_var.h"

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET
#include "wvNetLib.h"
#endif
#endif

#ifdef VIRTUAL_STACK
#include "netinet/vsLib.h"
#endif /* VIRTUAL_STACK */

/* defines */

#ifndef VIRTUAL_STACK
#define MAXTABLESIZE 64		/*
                                 * smallest cluster limits setsockopt() 
                                 * routine to 64 2-byte integers
                                 */
#endif /* VIRTUAL_STACK */

#define PATHMTU_MIN  68 	/* Lowest allowable path MTU estimate */

/* externs */

extern void pfctlinput ();

#ifndef VIRTUAL_STACK
extern	struct protosw inetsw[];
extern VOIDFUNCPTR	_icmpErrorHook; 
#endif /* VIRTUAL_STACK */

/* globals */

#ifndef VIRTUAL_STACK
struct icmpstat icmpstat;
struct sockaddr_in icmpmask = { 8, 0 };

int	icmpmaskrepl = 0;
#ifdef ICMPPRINTFS
int	icmpprintfs = 0;
#endif
#endif /* VIRTUAL_STACK */

/* locals */

#ifndef VIRTUAL_STACK
static struct sockaddr_in icmpsrc = { sizeof (struct sockaddr_in), AF_INET };
static struct sockaddr_in icmpdst = { sizeof (struct sockaddr_in), AF_INET };
static struct sockaddr_in icmpgw = { sizeof (struct sockaddr_in), AF_INET };

LOCAL u_short mtuTable [MAXTABLESIZE] = {68, 296, 508, 1006, 1492, 2002,
                                         4352, 8166, 17914, 32000, 65535};
LOCAL int mtuTableSize = 11;
#endif /* VIRTUAL_STACK */

/* forward declarations */
static void	icmp_error (struct mbuf *, int, int, n_long, struct ifnet *);
static void	icmp_reflect (struct mbuf *);
static void	icmp_send (struct mbuf *, struct mbuf *);
LOCAL void ip_next_mtu (struct sockaddr *, int);

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET
    /* Set common fields of event identifiers for this module. */

LOCAL UCHAR wvNetModuleId = WV_NET_IPICMP_MODULE;   /* Value for ip_icmp.c */
LOCAL UCHAR wvNetLocalFilter = WV_NET_NONE;     /* Available event filter */

LOCAL ULONG wvNetEventId;       /* Event identifier: see wvNetLib.h */
#endif    /* INCLUDE_WVNET */
#endif

void
icmp_init()
{
	if (_icmpErrorHook == NULL)	/* initialize the hook if not null */
	    _icmpErrorHook = icmp_error; 

#ifdef ICMPPRINTFS
        icmpprintfs = 0;
#endif

        icmpsrc.sin_len = sizeof(struct sockaddr_in);
        icmpsrc.sin_family = AF_INET;
        icmpdst.sin_len = sizeof(struct sockaddr_in);
        icmpdst.sin_family = AF_INET;
        icmpgw.sin_len = sizeof(struct sockaddr_in);
        icmpgw.sin_family = AF_INET;

        mtuTable[0] = 68;
        mtuTable[1] = 296;
        mtuTable[2] = 508;
        mtuTable[3] = 1006;
        mtuTable[4] = 1492;
        mtuTable[5] = 2002;
        mtuTable[6] = 4352;
        mtuTable[7] = 8166;
        mtuTable[8] = 17914;
        mtuTable[9] = 32000;
        mtuTable[10] = 65535;
        mtuTableSize = 11;
}

/*
 * ICMP routines: error generation, receive packet processing, and
 * routines to turnaround packets back to the originator, and
 * host table maintenance routines.
 */

/*
 * Generate an error packet of type error
 * in response to bad packet ip.
 */
static void
icmp_error(n, type, code, dest, destifp)
	struct mbuf *n;
	int type, code;
	n_long dest;
	struct ifnet *destifp;
{
	register struct ip *oip = mtod(n, struct ip *), *nip;
	register unsigned oiplen = oip->ip_hl << 2;
	register struct icmp *icp;
	register struct mbuf *m;
        MTU_QUERY mtuQuery;
	unsigned icmplen;
	short    offset;

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_EMERGENCY or WV_NET_ALERT event */
        if (type == ICMP_SOURCEQUENCH) 	/* ENOBUFS error: out of memory. */
            {
            WV_NET_MARKER_2 (NET_CORE_EVENT, WV_NET_EMERGENCY, 9, 2, 
                             WV_NETEVENT_ICMPERR_START, type, code)
            }
        else
            {
            WV_NET_MARKER_2 (NET_CORE_EVENT, WV_NET_CRITICAL, 8, 2, 
                             WV_NETEVENT_ICMPERR_START, type, code)
            }
#endif  /* INCLUDE_WVNET */
#endif

#ifdef ICMPPRINTFS
	if (icmpprintfs)
		printf("icmp_error(%x, %d, %d)\n", oip, type, code);
#endif
	if (type != ICMP_REDIRECT)
	    {
#ifdef VIRTUAL_STACK
	    _icmpstat.icps_error++;
#else
	    icmpstat.icps_error++;
#endif /* VIRTUAL_STACK */
            offset = oip->ip_off;
	    }
        else
	    {
            /* 
	     * If ICMP_REDIRECT, offset field is in network host order
	     * (changed when ip_output forwarded this packet). We need it 
	     * to be in host order for the processing.
	     */

	    offset = ntohs (oip->ip_off);
	    }

	/*
	 * Don't send error if not the first fragment of message.
	 * Don't error if the old packet protocol was ICMP
	 * error message, only known informational types.
	 */
	if (offset &~ (IP_MF|IP_DF))
		goto freeit;
	if (oip->ip_p == IPPROTO_ICMP && type != ICMP_REDIRECT &&
	  n->m_len >= oiplen + ICMP_MINLEN &&
	  !ICMP_INFOTYPE(((struct icmp *)((caddr_t)oip + oiplen))->icmp_type)) {
#ifdef VIRTUAL_STACK
		_icmpstat.icps_oldicmp++;
#else
		icmpstat.icps_oldicmp++;
#endif /* VIRTUAL_STACK */
		goto freeit;
	}
	/* Don't send error in response to a multicast or broadcast packet */
	if (n->m_flags & (M_BCAST|M_MCAST))
		goto freeit;
	/*
	 * First, formulate icmp message
	 */
	if ((m = mHdrClGet(M_DONTWAIT, MT_HEADER, CL_SIZE_128, TRUE)) == NULL)
		goto freeit;
	icmplen = oiplen + min(8, oip->ip_len);
	m->m_len = icmplen + ICMP_MINLEN;
	MH_ALIGN(m, m->m_len);
	icp = mtod(m, struct icmp *);
	if ((u_int)type > ICMP_MAXTYPE)
            {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_EMERGENCY event */
            WV_NET_MARKER_1 (NET_CORE_EVENT, WV_NET_EMERGENCY, 14, 1, 
                             WV_NETEVENT_ICMPERR_PANIC, type)
#endif  /* INCLUDE_WVNET */
#endif

            panic("icmp_error");
            }
#ifdef VIRTUAL_STACK
	_icmpstat.icps_outhist[type]++;
#else
	icmpstat.icps_outhist[type]++;
#endif /* VIRTUAL_STACK */
	icp->icmp_type = type;
	if (type == ICMP_REDIRECT)
		icp->icmp_gwaddr.s_addr = dest;
	else {
		icp->icmp_void = 0;
		/* 
		 * The following assignments assume an overlay with the
		 * zeroed icmp_void field.
		 */
		if (type == ICMP_PARAMPROB) {
			icp->icmp_pptr = code;
			code = 0;
		} else if (type == ICMP_UNREACH &&
			code == ICMP_UNREACH_NEEDFRAG && destifp) {

                        /*
                         * Point-to-multipoint devices allow a separate MTU
                         * for each destination address. Get that value or
                         * leave the next MTU unspecified if not available.
                         */

                        if (destifp->if_type == IFT_PMP)
                            {
                            mtuQuery.family = AF_INET;
                            mtuQuery.dstIpAddr = dest;
                            if (destifp->if_ioctl (destifp, SIOCGMTU,
                                                   (caddr_t) &mtuQuery) == 0)
                                {
                                icp->icmp_nextmtu = htons (mtuQuery.mtu);
                                }
                            }
                        else
                            {
                            icp->icmp_nextmtu = htons(destifp->if_mtu);
                            }
		}
	}

	icp->icmp_code = code;
	bcopy((caddr_t)oip, (caddr_t)&icp->icmp_ip, icmplen);
	nip = &icp->icmp_ip;

	if (type != ICMP_REDIRECT)
	    {
	    /*
	     * If ICMP_REDIRECT, these changes have already been made in 
	     * ip_output when the packet was forwarded.
	     */

	    nip->ip_len = htons((u_short)(nip->ip_len + oiplen));
	    HTONS (nip->ip_id);
	    HTONS (nip->ip_off);
	    }

	/*
	 * Now, copy old ip header (without options)
	 * in front of icmp message.
	 */
	if (m->m_data - sizeof(struct ip) < m->m_extBuf)
		panic("icmp len");
	m->m_data -= sizeof(struct ip);
	m->m_len += sizeof(struct ip);
	m->m_pkthdr.len = m->m_len;
	m->m_pkthdr.rcvif = n->m_pkthdr.rcvif;
	nip = mtod(m, struct ip *);
	bcopy((caddr_t)oip, (caddr_t)nip, sizeof(struct ip));
	nip->ip_len = m->m_len;
	nip->ip_hl = sizeof(struct ip) >> 2;
	nip->ip_p = IPPROTO_ICMP;
	nip->ip_tos = 0;
	icmp_reflect(m);

freeit:
	m_freem(n);
}

/*
 * Process a received ICMP message.
 */
void
icmp_input(m, hlen)
	register struct mbuf *m;
	int hlen;
{
	register struct icmp *icp;
	register struct ip *ip = mtod(m, struct ip *);
	int icmplen = ip->ip_len;
	register int i;
	struct in_ifaddr *ia;
	int (*ctlfunc) (int, struct sockaddr *, struct ip *);
	int code;
#ifndef VIRTUAL_STACK
	extern u_char ip_protox[];
#endif /* VIRTUAL_STACK */

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_NOTICE event */
    WV_NET_EVENT_0 (NET_CORE_EVENT, WV_NET_NOTICE, 7, 8, 
                    WV_NETEVENT_ICMPIN_START, WV_NET_RECV)
#endif  /* INCLUDE_WVNET */
#endif

	/*
	 * Locate icmp structure in mbuf, and check
	 * that not corrupted and of at least minimum length.
	 */
#ifdef ICMPPRINTFS
	if (icmpprintfs)
		printf("icmp_input from %x to %x, len %d\n",
			ntohl(ip->ip_src.s_addr), ntohl(ip->ip_dst.s_addr),
			icmplen);
#endif
	if (icmplen < ICMP_MINLEN) {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_CRITICAL event */
        WV_NET_ADDRIN_EVENT_0 (NET_CORE_EVENT, WV_NET_CRITICAL, 9, 3,
                               ip->ip_src.s_addr, ip->ip_dst.s_addr,
                               WV_NETEVENT_ICMPIN_SHORTMSG, WV_NET_RECV)
#endif  /* INCLUDE_WVNET */
#endif

#ifdef VIRTUAL_STACK
		_icmpstat.icps_tooshort++;
#else
		icmpstat.icps_tooshort++;
#endif /* VIRTUAL_STACK */
		goto freeit;
	}
	i = hlen + min(icmplen, ICMP_ADVLENMIN);
	if (m->m_len < i && (m = m_pullup(m, i)) == 0)  {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_CRITICAL event */
        WV_NET_ADDRIN_EVENT_0 (NET_CORE_EVENT, WV_NET_CRITICAL, 9, 3,
                               ip->ip_src.s_addr, ip->ip_dst.s_addr,
                               WV_NETEVENT_ICMPIN_SHORTMSG, WV_NET_RECV)
#endif  /* INCLUDE_WVNET */
#endif

#ifdef VIRTUAL_STACK
		_icmpstat.icps_tooshort++;
#else
		icmpstat.icps_tooshort++;
#endif /* VIRTUAL_STACK */
		return;
	}
	ip = mtod(m, struct ip *);
	m->m_len -= hlen;
	m->m_data += hlen;
	icp = mtod(m, struct icmp *);
	if (in_cksum(m, icmplen)) {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_CRITICAL event */
        WV_NET_ADDRIN_EVENT_0 (NET_CORE_EVENT, WV_NET_CRITICAL, 10, 4,
                               ip->ip_src.s_addr, ip->ip_dst.s_addr,
                               WV_NETEVENT_ICMPIN_BADSUM, WV_NET_RECV)
#endif  /* INCLUDE_WVNET */
#endif

#ifdef VIRTUAL_STACK
		_icmpstat.icps_checksum++;
#else
		icmpstat.icps_checksum++;
#endif /* VIRTUAL_STACK */
		goto freeit;
	}
	m->m_len += hlen;
	m->m_data -= hlen;

#ifdef ICMPPRINTFS
	/*
	 * Message type specific processing.
	 */
	if (icmpprintfs)
		printf("icmp_input, type %d code %d\n", icp->icmp_type,
		    icp->icmp_code);
#endif
	if (icp->icmp_type > ICMP_MAXTYPE)
		goto raw;
#ifdef VIRTUAL_STACK
	_icmpstat.icps_inhist[icp->icmp_type]++;
#else
	icmpstat.icps_inhist[icp->icmp_type]++;
#endif /* VIRTUAL_STACK */
	code = icp->icmp_code;
	switch (icp->icmp_type) {

	case ICMP_UNREACH:
		switch (code) {
			case ICMP_UNREACH_NET:
			case ICMP_UNREACH_HOST:
			case ICMP_UNREACH_PROTOCOL:
			case ICMP_UNREACH_PORT:
			case ICMP_UNREACH_SRCFAIL:
				code += PRC_UNREACH_NET;
				break;

			case ICMP_UNREACH_NEEDFRAG:
				code = PRC_MSGSIZE;
				break;
				
			case ICMP_UNREACH_NET_UNKNOWN:
			case ICMP_UNREACH_NET_PROHIB:
			case ICMP_UNREACH_TOSNET:
				code = PRC_UNREACH_NET;
				break;

			case ICMP_UNREACH_HOST_UNKNOWN:
			case ICMP_UNREACH_ISOLATED:
			case ICMP_UNREACH_HOST_PROHIB:
			case ICMP_UNREACH_TOSHOST:
				code = PRC_UNREACH_HOST;
				break;

			default:
				goto badcode;
		}
		goto deliver;

	case ICMP_TIMXCEED:
		if (code > 1)
			goto badcode;
		code += PRC_TIMXCEED_INTRANS;
		goto deliver;

	case ICMP_PARAMPROB:
		if (code > 1)
			goto badcode;
		code = PRC_PARAMPROB;
		goto deliver;

	case ICMP_SOURCEQUENCH:
		if (code)
			goto badcode;
		code = PRC_QUENCH;
	deliver:
		/*
		 * Problem with datagram; advise higher level routines.
		 */
		if (icmplen < ICMP_ADVLENMIN || icmplen < ICMP_ADVLEN(icp) ||
		    icp->icmp_ip.ip_hl < (sizeof(struct ip) >> 2)) {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_CRITICAL event */
            WV_NET_ADDRIN_EVENT_2 (NET_CORE_EVENT, WV_NET_CRITICAL, 11, 5, 
                                   ip->ip_src.s_addr, ip->ip_dst.s_addr,
                                   WV_NETEVENT_ICMPIN_BADLEN, WV_NET_RECV,
                                   icmplen, 4 * icp->icmp_ip.ip_hl)
#endif  /* INCLUDE_WVNET */
#endif

#ifdef VIRTUAL_STACK
			_icmpstat.icps_badlen++;
#else
			icmpstat.icps_badlen++;
#endif /* VIRTUAL_STACK */
			goto freeit;
		}
		NTOHS(icp->icmp_ip.ip_len);
#ifdef ICMPPRINTFS
		if (icmpprintfs)
			printf("deliver to protocol %d\n", icp->icmp_ip.ip_p);
#endif
		icmpsrc.sin_addr = icp->icmp_ip.ip_dst;

                /* Path MTU discovery: get new MTU value. */

                if (code == PRC_MSGSIZE)
                    {
                    i = ntohs (icp->icmp_nextmtu);

                    ip_next_mtu ( (struct sockaddr *)&icmpsrc, i);
                    }

		if ((ctlfunc =
                   (FUNCPTR)(inetsw[ip_protox[icp->icmp_ip.ip_p]].pr_ctlinput)))		
			(*ctlfunc)(code, (struct sockaddr *)&icmpsrc,
			    &icp->icmp_ip);
		break;

	badcode:
#ifdef VIRTUAL_STACK
		_icmpstat.icps_badcode++;
#else
		icmpstat.icps_badcode++;
#endif /* VIRTUAL_STACK */
		break;

	case ICMP_ECHO:
		icp->icmp_type = ICMP_ECHOREPLY;
		goto reflect;

	case ICMP_TSTAMP:
		if (icmplen < ICMP_TSLEN) {
#ifdef VIRTUAL_STACK
			_icmpstat.icps_badlen++;
#else
			icmpstat.icps_badlen++;
#endif /* VIRTUAL_STACK */
			break;
		}
		icp->icmp_type = ICMP_TSTAMPREPLY;
		icp->icmp_rtime = iptime();
		icp->icmp_ttime = icp->icmp_rtime;	/* bogus, do later! */
		goto reflect;
		
	case ICMP_MASKREQ:
#define	satosin(sa)	((struct sockaddr_in *)(sa))
		if (icmpmaskrepl == 0)
			break;
		/*
		 * We are not able to respond with all ones broadcast
		 * unless we receive it over a point-to-point interface.
		 */
		if (icmplen < ICMP_MASKLEN)
			break;
		switch (ip->ip_dst.s_addr) {

		case INADDR_BROADCAST:
		case INADDR_ANY:
			icmpdst.sin_addr = ip->ip_src;
			break;

		default:
			icmpdst.sin_addr = ip->ip_dst;
		}
		ia = (struct in_ifaddr *)ifaof_ifpforaddr(
			    (struct sockaddr *)&icmpdst, m->m_pkthdr.rcvif);
		if (ia == 0 || ia->ia_ifp == 0)
			break;
		icp->icmp_type = ICMP_MASKREPLY;
		icp->icmp_mask = ia->ia_sockmask.sin_addr.s_addr;
		if (ip->ip_src.s_addr == 0) {
			if (ia->ia_ifp->if_flags & IFF_BROADCAST)
			    ip->ip_src = satosin(&ia->ia_broadaddr)->sin_addr;
			else if (ia->ia_ifp->if_flags & IFF_POINTOPOINT)
			    ip->ip_src = satosin(&ia->ia_dstaddr)->sin_addr;
		}
reflect:
		ip->ip_len += hlen;	/* since ip_input deducts this */
#ifdef VIRTUAL_STACK
		_icmpstat.icps_reflect++;
		_icmpstat.icps_outhist[icp->icmp_type]++;
#else
		icmpstat.icps_reflect++;
		icmpstat.icps_outhist[icp->icmp_type]++;
#endif /* VIRTUAL_STACK */
		icmp_reflect(m);
		return; 

	case ICMP_REDIRECT:
		if (code > 3)
			goto badcode;
		if (icmplen < ICMP_ADVLENMIN || icmplen < ICMP_ADVLEN(icp) ||
		    icp->icmp_ip.ip_hl < (sizeof(struct ip) >> 2)) {
#ifdef VIRTUAL_STACK
			_icmpstat.icps_badlen++;
#else
			icmpstat.icps_badlen++;
#endif /* VIRTUAL_STACK */
			break;
		}
		/*
		 * Short circuit routing redirects to force
		 * immediate change in the kernel's routing
		 * tables.  The message is also handed to anyone
		 * listening on a raw socket (e.g. the routing
		 * daemon for use in updating its tables).
		 */
		icmpgw.sin_addr = ip->ip_src;
		icmpdst.sin_addr = icp->icmp_gwaddr;
#ifdef	ICMPPRINTFS
		if (icmpprintfs)
			printf("redirect dst %x to %x\n", icp->icmp_ip.ip_dst,
				icp->icmp_gwaddr);
#endif
		icmpsrc.sin_addr = icp->icmp_ip.ip_dst;
		rtredirect((struct sockaddr *)&icmpsrc,
		  (struct sockaddr *)&icmpdst,
		  (struct sockaddr *)0, RTF_GATEWAY | RTF_HOST,
		  (struct sockaddr *)&icmpgw, (struct rtentry **)0);
		pfctlinput(PRC_REDIRECT_HOST, (struct sockaddr *)&icmpsrc);
		break;

	/*
	 * No kernel processing for the following;
	 * just fall through to send to raw listener.
	 */
	case ICMP_ECHOREPLY:
	case ICMP_ROUTERADVERT:
	case ICMP_ROUTERSOLICIT:
	case ICMP_TSTAMPREPLY:
	case ICMP_IREQREPLY:
	case ICMP_MASKREPLY:
	default:
		break;
	}

raw:
	rip_input(m);
	return;

freeit:
	m_freem(m);
	return;
}

/*
 * Reflect the ip packet back to the source
 */
static void
icmp_reflect(m)
	struct mbuf *m;
{
	register struct ip *ip = mtod(m, struct ip *);
	register struct in_ifaddr *ia;
	struct in_addr t;
	struct mbuf *opts = 0, *ip_srcroute();
	int optlen = (ip->ip_hl << 2) - sizeof(struct ip);

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_NOTICE event */
    WV_NET_EVENT_1 (NET_CORE_EVENT, WV_NET_NOTICE, 8, 9, 
                    WV_NETEVENT_ICMPREFLECT_START, WV_NET_SEND,
                    ip->ip_src.s_addr)
#endif  /* INCLUDE_WVNET */
#endif

	if (!in_canforward(ip->ip_src) &&
	    ((ntohl(ip->ip_src.s_addr) & IN_CLASSA_NET) !=
	     (IN_LOOPBACKNET << IN_CLASSA_NSHIFT))) {
		m_freem(m);	/* Bad return address */
		goto done;	/* Ip_output() will check for broadcast */
	}
	t = ip->ip_dst;
	ip->ip_dst = ip->ip_src;
	/*
	 * If the incoming packet was addressed directly to us,
	 * use dst as the src for the reply.  Otherwise (broadcast
	 * or anonymous), use the address which corresponds
	 * to the incoming interface.
	 */
#ifdef VIRTUAL_STACK
	for (ia = _in_ifaddr; ia; ia = ia->ia_next) {
#else
	for (ia = in_ifaddr; ia; ia = ia->ia_next) {
#endif /* VIRTUAL_STACK */
		if (t.s_addr == IA_SIN(ia)->sin_addr.s_addr)
			break;
		if (ia->ia_ifp && (ia->ia_ifp->if_flags & IFF_BROADCAST) &&
		    t.s_addr == satosin(&ia->ia_broadaddr)->sin_addr.s_addr)
			break;
	}
	icmpsrc.sin_addr = ip->ip_src;
	if (ia == (struct in_ifaddr *)0)
		ia = (struct in_ifaddr *)ifaof_ifpforaddr(
			(struct sockaddr *)&icmpsrc, m->m_pkthdr.rcvif);
	/*
	 * The following happens if the packet was not addressed to us,
	 * and was received on an interface with no IP address.
	 */
	if (ia == (struct in_ifaddr *)0)
#ifdef VIRTUAL_STACK
		ia = _in_ifaddr;
#else
		ia = in_ifaddr;
#endif /* VIRTUAL_STACK */
	t = IA_SIN(ia)->sin_addr;
	ip->ip_src = t;
	ip->ip_ttl = ipTimeToLive;

	if (optlen > 0) {
		register u_char *cp;
		int opt, cnt;
		u_int len;

		/*
		 * Retrieve any source routing from the incoming packet;
		 * add on any record-route or timestamp options.
		 */
		cp = (u_char *) (ip + 1);
#ifdef SRCRT
		if ((opts = ip_srcroute()) == 0 &&
		    (opts = mHdrClGet(M_DONTWAIT, MT_HEADER, CL_SIZE_128,
				      TRUE))) {
			opts->m_len = sizeof(struct in_addr);
			mtod(opts, struct in_addr *)->s_addr = 0;
		}
#endif /* SRCRT */
		if (opts) {
#ifdef ICMPPRINTFS
		    if (icmpprintfs)
			    printf("icmp_reflect optlen %d rt %d => ",
				optlen, opts->m_len);
#endif
		    for (cnt = optlen; cnt > 0; cnt -= len, cp += len) {
			    opt = cp[IPOPT_OPTVAL];
			    if (opt == IPOPT_EOL)
				    break;
			    if (opt == IPOPT_NOP)
				    len = 1;
			    else {
				    len = cp[IPOPT_OLEN];
				    if (len <= 0 || len > cnt)
					    break;
			    }
			    /*
			     * Should check for overflow, but it "can't happen"
			     */
			    if (opt == IPOPT_RR || opt == IPOPT_TS || 
				opt == IPOPT_SECURITY) {
				    bcopy((caddr_t)cp,
					mtod(opts, caddr_t) + opts->m_len, len);
				    opts->m_len += len;
			    }
		    }
		    /* Terminate & pad, if necessary */
		    if ((cnt = opts->m_len % 4)) {
			    for (; cnt < 4; cnt++) {
				    *(mtod(opts, caddr_t) + opts->m_len) =
					IPOPT_EOL;
				    opts->m_len++;
			    }
		    }
#ifdef ICMPPRINTFS
		    if (icmpprintfs)
			    printf("%d\n", opts->m_len);
#endif
		}
		/*
		 * Now strip out original options by copying rest of first
		 * mbuf's data back, and adjust the IP length.
		 */
		ip->ip_len -= optlen;
		ip->ip_hl = sizeof(struct ip) >> 2;
		m->m_len -= optlen;
		if (m->m_flags & M_PKTHDR)
			m->m_pkthdr.len -= optlen;
		optlen += sizeof(struct ip);
		bcopy((caddr_t)ip + optlen, (caddr_t)(ip + 1),
			 (unsigned)(m->m_len - sizeof(struct ip)));
	}
	m->m_flags &= ~(M_BCAST|M_MCAST);
	icmp_send(m, opts);
done:
	if (opts)
		(void)m_free(opts);
}

/*
 * Send an icmp packet back to the ip level,
 * after supplying a checksum.
 */
static void
icmp_send(m, opts)
	register struct mbuf *m;
	struct mbuf *opts;
{
	register struct ip *ip = mtod(m, struct ip *);
	register int hlen;
	register struct icmp *icp;

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_NOTICE event */
    WV_NET_ADDROUT_EVENT_2 (NET_CORE_EVENT, WV_NET_NOTICE, 9, 10, 
                            ip->ip_src.s_addr, ip->ip_dst.s_addr,
                            WV_NETEVENT_ICMPSEND_START, WV_NET_SEND,
                            ip->ip_src.s_addr, ip->ip_dst.s_addr)
#endif  /* INCLUDE_WVNET */
#endif

	hlen = ip->ip_hl << 2;
	m->m_data += hlen;
	m->m_len -= hlen;
	icp = mtod(m, struct icmp *);
	icp->icmp_cksum = 0;
	icp->icmp_cksum = in_cksum(m, ip->ip_len - hlen);
	m->m_data -= hlen;
	m->m_len += hlen;
#ifdef ICMPPRINTFS
	if (icmpprintfs)
		printf("icmp_send dst %x src %x\n", ip->ip_dst, ip->ip_src);
#endif
	(void) ip_output(m, opts, NULL, 0, NULL);
}

    /*
     * This routine handles the ICMP "need fragmentation" error which
     * triggers the selection of a new MTU value for the path MTU discovery
     * process. The new MTU value is equal to the suggested value from the
     * ICMP error message (if any) or the next value from the current path
     * MTU table. If the new MTU value is less than the acceptable minimum,
     * the routine marks the corresponding route to disable the path MTU
     * discovery process.
     */

LOCAL void ip_next_mtu
    (
    struct sockaddr * pDstAddr,     /* destination of original IP packet */
    int nextmtu
    )
    {
    struct rtentry * 	pDestRoute;
    u_long 		oldmtu = 0;
    u_long 		newmtu;
    int 		loop;

    pDestRoute = rtalloc1 (pDstAddr, 0);
    if (pDestRoute && (pDestRoute->rt_flags & RTF_HOST) &&
            !(pDestRoute->rt_rmx.rmx_locks & RTV_MTU))
        {
        oldmtu = pDestRoute->rt_rmx.rmx_mtu;

        if (oldmtu == 0)
            {
            /* Current estimate unavailable - end discovery process. */

            newmtu = 0;
            }
        else if (nextmtu == 0)
            {
            /* No value included in ICMP message. Use entry from table. */

            for (loop = 0; loop < mtuTableSize; loop++)
                 if (oldmtu > mtuTable [ (mtuTableSize - 1) - loop])
                     break;

            if (loop != mtuTableSize)
                {
                /* Set the new MTU value. */

                newmtu = mtuTable [loop];
                }
            else
                newmtu = 0;   /* Match not found - disable MTU discovery. */
            }
        else
            newmtu = nextmtu;    /* Use value from ICMP message. */

        if (newmtu < PATHMTU_MIN)
            {
            /* Set lock to disable path MTU discovery for this destination. */

            pDestRoute->rt_rmx.rmx_locks |= RTV_MTU;
            }
        else if (oldmtu > newmtu)
            {
            /*
             * Decrease the estimated MTU size to the new value. The test
             * handles the (illegal) case where a downstream router sends
             * a larger MTU size in the "need fragment" message.
             */

            pDestRoute->rt_rmx.rmx_mtu = newmtu;
            }
        }

    if (pDestRoute)
        {
        RTFREE (pDestRoute)
        }

    return;
    }

/* ICMP socket option processing. */

int icmp_ctloutput
    (
    int op,
    struct socket *so,
    int level,
    int optname,
    struct mbuf **mp
    )
    {
    register struct mbuf *m = *mp;
#if 0    /* For timer options (not yet supported). */
    int optval;
#endif
    int optlen;
    int error = 0;

    switch (op)
        {
        case PRCO_SETOPT:
            optlen = m->m_len;

            switch (optname)
                {
                case SO_PATHMTU_TBL:
                    if (m->m_len % sizeof (u_short))
                        error = EINVAL;
                    else
                        { 
                        bcopy ((char *)mtod (m, u_short *),
                               (char *)mtuTable, m->m_len);
                        mtuTableSize = m->m_len / sizeof (u_short);
                        }
                    break;

#if 0    /* Timers for increasing path MTU value (not yet supported). */
                case SO_PATHMTU_LIFETIME:
                    if (m->m_len != sizeof (int))
                        error = EINVAL;
                    else
                        optval = *mtod (m, int *);
                    break;
                case SO_PATHMTU_TIMEOUT:
                    break;
#endif
                default:
                    error = ENOPROTOOPT;
                    break;
                }
            if (m)
                (void)m_free(m);
            break;

        case PRCO_GETOPT:
            *mp = m = mBufClGet (M_WAIT, MT_SOOPTS, CL_SIZE_128, TRUE);
            if (m == (struct mbuf *) NULL)
                {
                error = ENOBUFS;
                break;
                }

            switch (optname)
                {
                case SO_PATHMTU_TBL:
                    m->m_len = mtuTableSize * sizeof (u_short);
                    bcopy ((char *)mtuTable, (char *)mtod(m, u_short *),
                           (unsigned)m->m_len);
                    break;

#if 0    /* Timers for increasing path MTU value (not yet supported). */
                case SO_PATHMTU_LIFETIME:
                    m->m_len = sizeof (u_int);
                    /* optval = lifetime; */
                    *mtod (m, u_int *) = optval;
                    break;

                case SO_PATHMTU_TIMEOUT:
                    break;
#endif
                default:
                    error = ENOPROTOOPT;
                    break;
                }
            break;
        }
    return (error);
    }

#ifdef SYSCTL_SUPPORT
int
icmp_sysctl(name, namelen, oldp, oldlenp, newp, newlen)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
{

/*
 * XXX - This event cannot currently occur: the icmp_sysctl() routine
 *       is only called by the Unix sysctl command which is not supported
 *       by VxWorks

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /@ WV_NET_INFO event @/
    WV_NET_MARKER_1 (NET_AUX_EVENT, WV_NET_INFO, 5, 11, 
                     WV_NETEVENT_ICMPCTRL_START, name[0])
#endif  /@ INCLUDE_WVNET @/
#endif

 * XXX - end of unused event
 */

	/* All sysctl names at this level are terminal. */
	if (namelen != 1)
            {
/*
 * XXX - This event cannot currently occur: the icmp_sysctl() routine
 *       is only called by the Unix sysctl command which is not supported
 *       by VxWorks

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /@ WV_NET_ERROR event @/
        WV_NET_MARKER_1 (NET_AUX_EVENT, WV_NET_ERROR, 18, 6, 
                         WV_NETEVENT_ICMPCTRL_BADLEN, namelen)
#endif  /@ INCLUDE_WVNET @/
#endif

 * XXX - end of unused event
 */
            return (ENOTDIR);
            }

	switch (name[0]) {
	case ICMPCTL_MASKREPL:
		return (sysctl_int(oldp, oldlenp, newp, newlen, &icmpmaskrepl));
	default:
/*
 * XXX - This event cannot currently occur: the icmp_sysctl() routine
 *       is only called by the Unix sysctl command which is not supported
 *       by VxWorks

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /@ WV_NET_ERROR event @/
            WV_NET_MARKER_1 (NET_AUX_EVENT, WV_NET_ERROR, 19, 7, 
                            WV_NETEVENT_ICMPCTRL_BADCMD, name[0]) 
#endif  /@ INCLUDE_WVNET @/
#endif

 * XXX - end of unused event
 */
		return (ENOPROTOOPT);
	}
	/* NOTREACHED */
}
#endif /* SYSCTL_SUPPORT */
