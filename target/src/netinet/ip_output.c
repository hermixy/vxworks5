/* ip_output.c - internet output routines */

/* Copyright 1984 - 2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
 * Copyright (c) 1982, 1986, 1988, 1990, 1993
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
 *	@(#)ip_output.c	8.3 (Berkeley) 1/21/94
 */

/*
modification history
--------------------
01t,22mar02,wap  export ip_mloopback() so that ipOutput() can use it (SPR
                 #72246)
01s,29oct01,wap  Fixed mbuf leak in ip_output() (SPR #67966)
01r,12oct01,rae  merge from truestack ver 01w, base 01o (SPR #69683 etc.)
01q,07dec00,rae  Add IPsec output hook
01p,17nov00,rae  allow IP_DROP_MEMBERSHIP with INADDR_ANY (SPR# 33093)
01o,03sep98,n_s  fixed fragmentation problem for mcast and bcast. spr #21071.
01n,26aug98,n_s  added return val check for mBufClGet in ip_ctloutput and
                 ip_getmoptions. spr #22238.
01m,09jan98,vin  changed IP_CKSUM_SEND_MASK to IP_DO_CHECKSUM_SND
01l,08dec97,vin  fixed ip_freemoptions() SPR 9970. 
01k,19nov97,vin  changed extern declaration of _ipCfgFlags to int from UCHAR.
01j,13nov97,vin  provided an additional flag for sending large BCAST pkts.
01i,05oct97,vin  added fast multicasting.
01h,15jul97,spm  made IP checksum calculation configurable (SPR #7836)
01g,08mar97,vin  added mCastRouteFwdHook to access mcast routing code.
01f,24mar97,vin  bug fixed in ip_mloopback() ip hdr being shared, SPR8252
01e,13feb97,rjc  fixed bad parameter to TOS_SET	
01d,05feb97,rjc  changes for tos routing and fix for multicast bug.
01c,05dec96,vin  changed malloc(..) to MALLOC(..) to use only network buffers,
		 changed free(..) to FREE(..).
01b,22nov96,vin  added cluster support replaced m_get(..) with mBufClGet(..)
		 replaced m_gethdr with mHdrClGet(..)
01a,03mar96,vin  created from BSD4.4 stuff,integrated with 02n of ip_output.c
*/

#include "vxWorks.h"
#include "netLib.h"
#include "net/mbuf.h"
#include "errno.h"
#include "net/protosw.h"
#include "sys/socket.h"
#include "sys/ioctl.h"
#include "net/socketvar.h"

#include "net/if.h"
#include "net/if_types.h"
#include "net/route.h"

#include "netinet/in.h"
#include "netinet/in_pcb.h"
#include "netinet/in_systm.h"
#include "netinet/in_var.h"
#include "netinet/ip.h"
#include "netinet/ip_var.h"
#include "net/systm.h"

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET
#include "wvNetLib.h"
#endif
#endif


/*IPsec output hook defined in /target/src/wrn/ipsec/ipsec_io.c */
typedef int (*IPSEC_OUTPUT_FUNCPTR) (struct mbuf** m0,
                                     struct mbuf* opt,
                                     struct route* ro, int flags,
                                     struct ip_moptions* imo,struct sockaddr_in* dst,
                                     struct in_ifaddr* ia);

IPSEC_OUTPUT_FUNCPTR    _func_ipsecOutput;

#ifdef VIRTUAL_STACK
#include "netinet/vsLib.h"
#include "netinet/vsMcast.h"
#endif

/* externs */

#ifndef VIRTUAL_STACK
IMPORT int 	_ipCfgFlags; 		/* Calculate IP checksum? */

extern FUNCPTR _mCastRouteFwdHook;	/* WRS mcast forward command hook */
#endif /* VIRTUAL_STACK */

extern int looutput();

static struct mbuf *ip_insertoptions (struct mbuf *, struct mbuf *, int *);
void ip_mloopback (struct ifnet *, struct mbuf *, struct sockaddr_in *,
                   struct rtentry *rt);

static int  ip_setmoptions (int, struct inpcb *, struct mbuf *);

/* locals */

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET
    /* Set common fields of event identifiers for this module. */

LOCAL UCHAR wvNetModuleId = WV_NET_IPOUTPUT_MODULE; /* Value for ip_output.c */
LOCAL UCHAR wvNetLocalFilter = WV_NET_NONE;     /* Available event filter */

LOCAL ULONG wvNetEventId;       /* Event identifier: see wvNetLib.h */
#endif    /* INCLUDE_WVNET */
#endif

/*
 * IP output.  The packet in mbuf chain m contains a skeletal IP
 * header (with len, off, ttl, proto, tos, src, dst).
 * The mbuf chain containing the packet will be freed.
 * The mbuf opt, if present, will not be freed.
 */
int
ip_output(m0, opt, ro, flags, imo)
	struct mbuf *m0;
	struct mbuf *opt;
	struct route *ro;
	int flags;
	struct ip_moptions *imo;
{
	register struct ip *ip, *mhip;
	register struct ifnet *ifp = NULL;
	register struct mbuf *m = m0;
	register int hlen = sizeof (struct ip);
	int len, off, error = 0;
	struct route iproute;
	struct sockaddr_in *dst;
        MTU_QUERY mtuQuery;
        int mtu = 0;
	struct in_ifaddr *ia = NULL;

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET
        u_long srcAddr; /* Source IP address, unavailable when finished */
        u_long dstAddr; /* Dest. IP address, unavailable when finished */
#endif
#endif

#ifdef	DIAGNOSTIC
	if ((m->m_flags & M_PKTHDR) == 0)
		panic("ip_output no HDR");
#endif
	if (opt) {
		m = ip_insertoptions(m, opt, &len);
		hlen = len;
	}
	ip = mtod(m, struct ip *);
	/*
	 * Fill in IP header.
	 */
	if ((flags & (IP_FORWARDING|IP_RAWOUTPUT)) == 0) {
		ip->ip_v = IPVERSION;
		ip->ip_off &= IP_DF;
#ifdef VIRTUAL_STACK
		ip->ip_id = htons(_ip_id++);
#else
		ip->ip_id = htons(ip_id++);
#endif /* VIRTUAL_STACK */
		ip->ip_hl = hlen >> 2;
#ifdef VIRTUAL_STACK
		_ipstat.ips_localout++;
#else
		ipstat.ips_localout++;
#endif /* VIRTUAL_STACK */
	} else {
		hlen = ip->ip_hl << 2;
	}

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET
        /*
         * Save the source and destination address from the original packet,
         * since they will be unavailable at the "done:" label.
         */

        srcAddr = ip->ip_src.s_addr;
        dstAddr = ip->ip_dst.s_addr;
#endif
#endif

	/*
	 * Route packet.
	 */
	if (ro == 0) {
		ro = &iproute;
		bzero((caddr_t)ro, sizeof (*ro));
	}
	dst = (struct sockaddr_in *)&ro->ro_dst;
	/*
	 * If there is a cached route,
	 * check that it is to the same destination
	 * and is still up.  If not, free it and try again.
	 */
	if (ro->ro_rt && ((ro->ro_rt->rt_flags & RTF_UP) == 0 ||
	   dst->sin_addr.s_addr != ip->ip_dst.s_addr)) {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_VERBOSE event */
        WV_NET_DSTADDROUT_EVENT_2 (NET_CORE_EVENT, WV_NET_VERBOSE, 57, 8,
                                   ip->ip_dst.s_addr,
                                   WV_NETEVENT_IPOUT_CACHEMISS, WV_NET_SEND,
                                   ip->ip_dst.s_addr, dst->sin_addr.s_addr)
#endif  /* INCLUDE_WVNET */
#endif

		RTFREE(ro->ro_rt);
		ro->ro_rt = (struct rtentry *)0;
	}
	if (ro->ro_rt == 0) {
		dst->sin_family = AF_INET;
		dst->sin_len = sizeof(*dst);
		dst->sin_addr = ip->ip_dst;
		TOS_SET (dst, ip->ip_tos);
	}
	/*
	 * If routing to interface only,
	 * short circuit routing lookup.
	 */
#define ifatoia(ifa)	((struct in_ifaddr *)(ifa))
#define sintosa(sin)	((struct sockaddr *)(sin))
	if (flags & IP_ROUTETOIF ) {
		if ((ia = ifatoia(ifa_ifwithdstaddr(sintosa(dst)))) == 0 &&
		    (ia = ifatoia(ifa_ifwithnet(sintosa(dst)))) == 0) {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_CRITICAL event */
        WV_NET_DSTADDROUT_EVENT_1 (NET_CORE_EVENT, WV_NET_CRITICAL, 20, 1,
                                   ip->ip_dst.s_addr,
                                   WV_NETEVENT_IPOUT_BADADDR, WV_NET_SEND,
                                   dst->sin_addr.s_addr)
#endif  /* INCLUDE_WVNET */
#endif
			
#ifdef VIRTUAL_STACK
			_ipstat.ips_noroute++;
#else
			ipstat.ips_noroute++;
#endif /* VIRTUAL_STACK */
			error = ENETUNREACH;
			goto bad;
		}
		ifp = ia->ia_ifp;
		ip->ip_ttl = 1;
	} else if (! (IN_MULTICAST(ntohl(ip->ip_dst.s_addr)) && imo &&
	              ((imo->imo_multicast_ifp != NULL)))) {
		if (ro->ro_rt == 0)
			rtalloc(ro);
		if (ro->ro_rt == 0) {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_CRITICAL event */
        WV_NET_DSTADDROUT_EVENT_1 (NET_CORE_EVENT, WV_NET_CRITICAL, 20, 1,
                                   ip->ip_dst.s_addr,
                                   WV_NETEVENT_IPOUT_BADADDR, WV_NET_SEND,
                                   dst->sin_addr.s_addr)
#endif  /* INCLUDE_WVNET */
#endif

#ifdef VIRTUAL_STACK
			_ipstat.ips_noroute++;
#else
			ipstat.ips_noroute++;
#endif /* VIRTUAL_STACK */
			error = EHOSTUNREACH;
			goto bad;
		}
		ia = ifatoia(ro->ro_rt->rt_ifa);
		ifp = ro->ro_rt->rt_ifp;
		ro->ro_rt->rt_use++;
		if (ro->ro_rt->rt_flags & RTF_GATEWAY)
			dst = (struct sockaddr_in *)ro->ro_rt->rt_gateway;
	}
	if (IN_MULTICAST(ntohl(ip->ip_dst.s_addr))) {
		struct in_multi *inm;
#ifndef VIRTUAL_STACK
		extern struct ifnet loif[];
#endif /* VIRTUAL_STACK */

		m->m_flags |= M_MCAST;
		/*
		 * IP destination address is multicast.  Make sure "dst"
		 * still points to the address in "ro".  (It may have been
		 * changed to point to a gateway address, above.)
		 */
		dst = (struct sockaddr_in *)&ro->ro_dst;

               if (imo && imo->imo_multicast_ifp != NULL) 
                   {

                   register struct in_ifaddr *ia;

#ifdef VIRTUAL_STACK
                   for (ia = _in_ifaddr; ia; ia = ia->ia_next)
#else
                   for (ia = in_ifaddr; ia; ia = ia->ia_next)
#endif /* VIRTUAL_STACK */
                       if (ia->ia_ifp == ifp) 
                           {
		           dst->sin_addr.s_addr = IA_SIN(ia)->sin_addr.s_addr;
	                   break;
			   }

                   if (ro->ro_rt)
                       {
                       RTFREE(ro->ro_rt);
                       ro->ro_rt = (struct rtentry *)0;
                       }
                   }

		/*
		 * See if the caller provided any multicast options
		 */
		if (imo != NULL) {
			ip->ip_ttl = imo->imo_multicast_ttl;
			if (imo->imo_multicast_ifp != NULL)
				ifp = imo->imo_multicast_ifp;
		} else
			ip->ip_ttl = IP_DEFAULT_MULTICAST_TTL;
		/*
		 * Confirm that the outgoing interface supports multicast.
		 */
		if ((ifp->if_flags & IFF_MULTICAST) == 0) {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_CRITICAL event */
        WV_NET_DSTADDROUT_EVENT_2 (NET_CORE_EVENT, WV_NET_CRITICAL, 21, 2,
                                   ip->ip_dst.s_addr,
                                   WV_NETEVENT_IPOUT_BADFLAGS, WV_NET_SEND,
                                   dst->sin_addr.s_addr, ifp)
#endif  /* INCLUDE_WVNET */
#endif

#ifdef VIRTUAL_STACK
			_ipstat.ips_noroute++;
#else
			ipstat.ips_noroute++;
#endif /* VIRTUAL_STACK */
			error = ENETUNREACH;
			goto bad;
		}
		/*
		 * If source address not specified yet, use address
		 * of outgoing interface.
		 */
		if (ip->ip_src.s_addr == INADDR_ANY) {
			register struct in_ifaddr *ia;

#ifdef VIRTUAL_STACK
			for (ia = _in_ifaddr; ia; ia = ia->ia_next)
#else
			for (ia = in_ifaddr; ia; ia = ia->ia_next)
#endif /* VIRTUAL_STACK */
				if (ia->ia_ifp == ifp) {
					ip->ip_src = IA_SIN(ia)->sin_addr;
					break;
				}
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET
        /*
         * Update the source destination address if not previously
         * specified, since it will be unavailable at the "done:" label.
         */

        srcAddr = ip->ip_src.s_addr;
#endif
#endif
		}

		IN_LOOKUP_MULTI(ip->ip_dst, ifp, inm);
		if (inm != NULL &&
		   (imo == NULL || imo->imo_multicast_loop)) {
			/*
			 * If we belong to the destination multicast group
			 * on the outgoing interface, and the caller did not
			 * forbid loopback, loop back a copy.
			 */
			ip_mloopback(ifp, m, dst, NULL);
		}
		else {
			/*
			 * If we are acting as a multicast router, perform
			 * multicast forwarding as if the packet had just
			 * arrived on the interface to which we are about
			 * to send.  The multicast forwarding function
			 * recursively calls this function, using the
			 * IP_FORWARDING flag to prevent infinite recursion.
			 *
			 * Multicasts that are looped back by ip_mloopback(),
			 * above, will be forwarded by the ip_input() routine,
			 * if necessary.
			 */
                	if ((_mCastRouteFwdHook != NULL) &&
                            ((flags & IP_FORWARDING) == 0)) {
				if ((*_mCastRouteFwdHook)(m, ifp, ip, NULL) != 0) {
					m_freem(m);
					goto done;
				}
			}
		}
		/*
		 * Multicasts with a time-to-live of zero may be looped-
		 * back, above, but must not be transmitted on a network.
		 * Also, multicasts addressed to the loopback interface
		 * are not sent -- the above call to ip_mloopback() will
		 * loop back a copy if this host actually belongs to the
		 * destination group on the loopback interface.
		 */
		if (ip->ip_ttl == 0 || ifp == loif) {
			m_freem(m);
			goto done;
		}

                /*
                 * Use the predefined MTU size for outgoing packets. The
                 * point-to-multipoint devices which permit a different
                 * value do not support multicast addresses.
                 */

                mtu = ifp->if_mtu;

		goto sendit;
	}
#ifndef notdef
	/*
	 * If source address not specified yet, use address
	 * of outgoing interface.
	 */
	if (ip->ip_src.s_addr == INADDR_ANY)
		ip->ip_src = IA_SIN(ia)->sin_addr;

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET
        /*
         * Update the source destination address if not previously 
         * specified, since it will be unavailable at the "done:" label.
         */

        srcAddr = ip->ip_src.s_addr;
#endif
#endif

#endif

        /*
         * Get the MTU size. Point-to-multipoint devices allow a separate
         * MTU for each destination address. All other devices use a
         * predefined size.
         */

        if (ifp->if_type == IFT_PMP)
            {
            /*
             * The SIOCGMTU operation reads the address from the
             * argument and returns the corresponding MTU size.
             */

            mtuQuery.family = AF_INET;
            mtuQuery.dstIpAddr = dst->sin_addr.s_addr;
            error = ifp->if_ioctl (ifp, SIOCGMTU, (caddr_t)&mtuQuery);
            if (error)
                {
                error = EHOSTUNREACH;
                goto bad;
                }
            mtu = mtuQuery.mtu;
            }
        else
            mtu = ifp->if_mtu;

	/*
	 * Look for broadcast address and
	 * and verify user is allowed to send
	 * such a packet.
	 */
	if (in_broadcast(dst->sin_addr, ifp)) {
		if ((ifp->if_flags & IFF_BROADCAST) == 0) {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_CRITICAL event */
            WV_NET_ADDROUT_EVENT_2 (NET_CORE_EVENT, WV_NET_CRITICAL, 21, 2,
                                    ip->ip_src.s_addr, ip->ip_dst.s_addr,
                                    WV_NETEVENT_IPOUT_BADFLAGS, WV_NET_SEND,
                                    dst->sin_addr.s_addr, ifp)
#endif  /* INCLUDE_WVNET */
#endif

			error = EADDRNOTAVAIL;
			goto bad;
		}
		if ((flags & IP_ALLOWBROADCAST) == 0) {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_CRITICAL event */
            WV_NET_ADDROUT_EVENT_2 (NET_CORE_EVENT, WV_NET_CRITICAL, 21, 2,
                                    ip->ip_src.s_addr, ip->ip_dst.s_addr,
                                    WV_NETEVENT_IPOUT_BADFLAGS, WV_NET_SEND,
                                    dst->sin_addr.s_addr, 0)
#endif  /* INCLUDE_WVNET */
#endif

			error = EACCES;
			goto bad;
		}
		/* allow broadcast messages to be fragmented if flag set */
		if (((u_short)ip->ip_len > mtu) &&
                    !(_ipCfgFlags & IP_DO_LARGE_BCAST)) {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_CRITICAL event */
            WV_NET_ADDROUT_EVENT_2 (NET_CORE_EVENT, WV_NET_CRITICAL, 22, 3,
                                    ip->ip_src.s_addr, ip->ip_dst.s_addr,
                                    WV_NETEVENT_IPOUT_CANTFRAG, WV_NET_SEND,
                                    dst->sin_addr.s_addr, ifp)
#endif  /* INCLUDE_WVNET */
#endif

			error = EMSGSIZE;
			goto bad;
		}
		m->m_flags |= M_BCAST;
	} else
		m->m_flags &= ~M_BCAST;

sendit:
        if (_func_ipsecOutput != NULL)
            {
            /* Deliver the packet to ipsec for further processing/filtering. */
            error = _func_ipsecOutput (&m0, opt, ro, flags, imo, dst, ia);
            m = m0;
            ip = mtod(m, struct ip *);
            if (error != 0)
                {
                /* If there is an error we should have already freed the packet */
                if (error == ERROR)
                    {
                    /* Unless the error is very specific we do not propagate the error; so we reset it */
                    error = OK;
                    }
                goto done;
                }
            }
	/*
	 * If small enough for interface, can just send directly.
	 */
	if ((u_short)ip->ip_len <= mtu) {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_INFO event */
            WV_NET_ADDROUT_EVENT_4 (NET_CORE_EVENT, WV_NET_INFO, 54, 10,
                                    ip->ip_src.s_addr, ip->ip_dst.s_addr,
                                    WV_NETEVENT_IPOUT_SENDALL, WV_NET_SEND,
                                    ip->ip_src.s_addr, ip->ip_dst.s_addr,
                                    (u_long)ip->ip_len, mtu)
#endif  /* INCLUDE_WVNET */
#endif
		ip->ip_len = htons((u_short)ip->ip_len);
		ip->ip_off = htons((u_short)ip->ip_off);

		ip->ip_sum = 0;
                if (_ipCfgFlags & IP_DO_CHECKSUM_SND)
                    ip->ip_sum = in_cksum(m, hlen);

		error = (*ifp->if_output)(ifp, m,
				(struct sockaddr *)dst, ro->ro_rt);
		goto done;
	}
	/*
	 * Too large for interface; fragment if possible.
	 * Must be able to put at least 8 bytes per fragment.
	 */
	if (ip->ip_off & IP_DF) {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_CRITICAL event */
            WV_NET_ADDROUT_EVENT_2 (NET_CORE_EVENT, WV_NET_CRITICAL, 22, 3,
                                    ip->ip_src.s_addr, ip->ip_dst.s_addr,
                                    WV_NETEVENT_IPOUT_CANTFRAG, WV_NET_SEND,
                                    dst->sin_addr.s_addr, ifp)
#endif  /* INCLUDE_WVNET */
#endif

		error = EMSGSIZE;

            /*
             * If the interface is using a new (smaller) MTU size than
             * originally assigned, update the current path MTU estimate
             * (if any) to the new (largest possible) value.
             */

            if (ro->ro_rt && (ro->ro_rt->rt_flags & (RTF_UP | RTF_HOST))
                && !(ro->ro_rt->rt_rmx.rmx_locks & RTV_MTU)
                && (ro->ro_rt->rt_rmx.rmx_mtu > mtu))
                {
                ro->ro_rt->rt_rmx.rmx_mtu = mtu;
                }

#ifdef VIRTUAL_STACK
		_ipstat.ips_cantfrag++;
#else
		ipstat.ips_cantfrag++;
#endif /* VIRTUAL_STACK */
		goto bad;
	}
	len = (mtu - hlen) &~ 7;
	if (len < 8) {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_CRITICAL event */
            WV_NET_ADDROUT_EVENT_2 (NET_CORE_EVENT, WV_NET_CRITICAL, 23, 4,
                                    ip->ip_src.s_addr, ip->ip_dst.s_addr,
                                    WV_NETEVENT_IPOUT_SHORTMSG, WV_NET_SEND,
                                    dst->sin_addr.s_addr, ifp)
#endif  /* INCLUDE_WVNET */
#endif

		error = EMSGSIZE;
		goto bad;
	}

    {
	int mhlen, firstlen = len;
	struct mbuf **mnext = &m->m_nextpkt;

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_INFO event */
        WV_NET_ADDROUT_EVENT_4 (NET_CORE_EVENT, WV_NET_INFO, 55, 11,
                                ip->ip_src.s_addr, ip->ip_dst.s_addr,
                                WV_NETEVENT_IPOUT_SENDFRAG, WV_NET_SEND,
                                dst->sin_addr.s_addr, (u_long)ip->ip_len,
                                hlen, mtu);
#endif  /* INCLUDE_WVNET */
#endif

	/*
	 * Loop through length of segment after first fragment,
	 * make new header and copy data of each part and link onto chain.
	 */
	m0 = m;
	mhlen = sizeof (struct ip);
	for (off = hlen + len; off < (u_short)ip->ip_len; off += len) {
		m= mHdrClGet(M_DONTWAIT, MT_HEADER, CL_SIZE_128, TRUE);
		if (m == 0) {
			error = ENOBUFS;
#ifdef VIRTUAL_STACK
			_ipstat.ips_odropped++;
#else
			ipstat.ips_odropped++;
#endif /* VIRTUAL_STACK */
			goto sendorfree;
		}
		m->m_flags = m0->m_flags;
		m->m_data += max_linkhdr;
		mhip = mtod(m, struct ip *);
		*mhip = *ip;
		if (hlen > sizeof (struct ip)) {
			mhlen = ip_optcopy(ip, mhip) + sizeof (struct ip);
			mhip->ip_hl = mhlen >> 2;
		}
		m->m_len = mhlen;
		mhip->ip_off = ((off - hlen) >> 3) + (ip->ip_off & ~IP_MF);
		if (ip->ip_off & IP_MF)
			mhip->ip_off |= IP_MF;
		if (off + len >= (u_short)ip->ip_len)
			len = (u_short)ip->ip_len - off;
		else
			mhip->ip_off |= IP_MF;
		mhip->ip_len = htons((u_short)(len + mhlen));
		m->m_next = m_copy(m0, off, len);
		if (m->m_next == 0) {
			(void) m_free(m);
			error = ENOBUFS;	/* ??? */
#ifdef VIRTUAL_STACK
			_ipstat.ips_odropped++;
#else
			ipstat.ips_odropped++;
#endif /* VIRTUAL_STACK */
			goto sendorfree;
		}
		m->m_pkthdr.len = mhlen + len;
		m->m_pkthdr.rcvif = (struct ifnet *)0;
		mhip->ip_off = htons((u_short)mhip->ip_off);

                mhip->ip_sum = 0;
                if (_ipCfgFlags & IP_DO_CHECKSUM_SND)
                    mhip->ip_sum = in_cksum(m, mhlen);

		*mnext = m;
		mnext = &m->m_nextpkt;
#ifdef VIRTUAL_STACK
		_ipstat.ips_ofragments++;
#else
		ipstat.ips_ofragments++;
#endif /* VIRTUAL_STACK */
	}
	/*
	 * Update first fragment by trimming what's been copied out
	 * and updating header, then send each fragment (in order).
	 */
	m = m0;
	m_adj(m, hlen + firstlen - (u_short)ip->ip_len);
	m->m_pkthdr.len = hlen + firstlen;
	ip->ip_len = htons((u_short)m->m_pkthdr.len);
	ip->ip_off = htons((u_short)(ip->ip_off | IP_MF));

        ip->ip_sum = 0;
        if (_ipCfgFlags & IP_DO_CHECKSUM_SND)
            ip->ip_sum = in_cksum(m, hlen);
sendorfree:
	for (m = m0; m; m = m0) {
		m0 = m->m_nextpkt;
		m->m_nextpkt = 0;
		if (error == 0)
			error = (*ifp->if_output)(ifp, m,
			    (struct sockaddr *)dst, ro->ro_rt);
		else
			m_freem(m);
	}

	if (error == 0)
#ifdef VIRTUAL_STACK
		_ipstat.ips_fragmented++;
#else
		ipstat.ips_fragmented++;
#endif /* VIRTUAL_STACK */
    }
done:

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_ERROR event */
    WV_NET_ADDROUT_EVENT_4 (NET_CORE_EVENT, WV_NET_ERROR, 31, 9,
                            srcAddr, dstAddr,
                            WV_NETEVENT_IPOUT_FINISH, WV_NET_SEND,
                            error, ifp, srcAddr, dstAddr)
#endif  /* INCLUDE_WVNET */
#endif

	if ((ro == &iproute) && ((flags & IP_ROUTETOIF) == 0) && ro->ro_rt) {
		RTFREE(ro->ro_rt);
	}	
	return (error);
bad:
	m_freem(m);
	goto done;
}

/*
 * Insert IP options into preformed packet.
 * Adjust IP destination as required for IP source routing,
 * as indicated by a non-zero in_addr at the start of the options.
 */
static struct mbuf *
ip_insertoptions(m, opt, phlen)
	register struct mbuf *m;
	struct mbuf *opt;
	int *phlen;
{
	register struct ipoption *p = mtod(opt, struct ipoption *);
	struct mbuf *n;
	register struct ip *ip = mtod(m, struct ip *);
	unsigned optlen;

	optlen = opt->m_len - sizeof(p->ipopt_dst);
	if (optlen + (u_short)ip->ip_len > IP_MAXPACKET)
		return (m);		/* XXX should fail */
	if (p->ipopt_dst.s_addr)
		ip->ip_dst = p->ipopt_dst;
#if 0	/* XXX changed for default cluster support vinai
         * The reason why always new clusters are allocted,
         * is that, data could be overwritten since clusters are shared.
         */
	if (m->m_flags & M_EXT || m->m_data - optlen < m->m_pktdat) {
#endif
		n = mHdrClGet(M_DONTWAIT, MT_HEADER, CL_SIZE_128, TRUE);
		if (n == 0)
			return (m);
		n->m_pkthdr.len = m->m_pkthdr.len + optlen;
		m->m_len -= sizeof(struct ip);
		m->m_data += sizeof(struct ip);
		n->m_next = m;
		m = n;
		m->m_len = optlen + sizeof(struct ip);
		m->m_data += max_linkhdr;
		bcopy((caddr_t)ip, mtod(m, caddr_t), sizeof(struct ip));
#if 0 	/* XXX changed for default cluster support vinai */               
	} else {
		m->m_data -= optlen;
		m->m_len += optlen;
		m->m_pkthdr.len += optlen;
		ovbcopy((caddr_t)ip, mtod(m, caddr_t), sizeof(struct ip));
	}
#endif        
	ip = mtod(m, struct ip *);
	bcopy((caddr_t)p->ipopt_list, (caddr_t)(ip + 1), (unsigned)optlen);
	*phlen = sizeof(struct ip) + optlen;
	ip->ip_len += optlen;
	return (m);
}

/*
 * Copy options from ip to jp,
 * omitting those not copied during fragmentation.
 */
int
ip_optcopy(ip, jp)
	struct ip *ip, *jp;
{
	register u_char *cp, *dp;
	int opt, optlen, cnt;

	cp = (u_char *)(ip + 1);
	dp = (u_char *)(jp + 1);
	cnt = (ip->ip_hl << 2) - sizeof (struct ip);
	for (; cnt > 0; cnt -= optlen, cp += optlen) {
		opt = cp[0];
		if (opt == IPOPT_EOL)
			break;
		if (opt == IPOPT_NOP) {
			/* Preserve for IP mcast tunnel's LSRR alignment. */
			*dp++ = IPOPT_NOP;
			optlen = 1;
			continue;
		} else
			optlen = cp[IPOPT_OLEN];
		/* bogus lengths should have been caught by ip_dooptions */
		if (optlen > cnt)
			optlen = cnt;
		if (IPOPT_COPIED(opt)) {
			bcopy((caddr_t)cp, (caddr_t)dp, (unsigned)optlen);
			dp += optlen;
		}
	}
	for (optlen = dp - (u_char *)(jp+1); optlen & 0x3; optlen++)
		*dp++ = IPOPT_EOL;
	return (optlen);
}

/*
 * IP socket option processing.
 */
int
ip_ctloutput(op, so, level, optname, mp)
	int op;
	struct socket *so;
	int level, optname;
	struct mbuf **mp;
{
	register struct inpcb *inp = sotoinpcb(so);
	register struct mbuf *m = *mp;
	register int optval = 0;
	int error = 0;

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_INFO event */
    WV_NET_MARKER_4 (NET_AUX_EVENT, WV_NET_INFO, 11, 6,
                     WV_NETEVENT_IPOUTCTRL_START,
                     so->so_fd, op, level, optname)
#endif  /* INCLUDE_WVNET */
#endif

	if (level != IPPROTO_IP) {
		error = EINVAL;
		if (op == PRCO_SETOPT && *mp)
			(void) m_free(*mp);
	} else switch (op) {

	case PRCO_SETOPT:
		switch (optname) {
		case IP_OPTIONS:
#ifdef notyet
		case IP_RETOPTS:
			return (ip_pcbopts(optname, &inp->inp_options, m));
#else
			return (ip_pcbopts(&inp->inp_options, m));
#endif

		case IP_TOS:
		case IP_TTL:
		case IP_RECVOPTS:
		case IP_RECVRETOPTS:
		case IP_RECVDSTADDR:
			if (m->m_len != sizeof(int))
				error = EINVAL;
			else {
				optval = *mtod(m, int *);
				switch (optname) {

				case IP_TOS:
					inp->inp_ip.ip_tos = optval;
					break;

				case IP_TTL:
					inp->inp_ip.ip_ttl = optval;
					break;
#define	OPTSET(bit) \
	if (optval) \
		inp->inp_flags |= bit; \
	else \
		inp->inp_flags &= ~bit;

				case IP_RECVOPTS:
					OPTSET(INP_RECVOPTS);
					break;

				case IP_RECVRETOPTS:
					OPTSET(INP_RECVRETOPTS);
					break;

				case IP_RECVDSTADDR:
					OPTSET(INP_RECVDSTADDR);
					break;
				}
			}
			break;
#undef OPTSET

		case IP_MULTICAST_IF:
		case IP_MULTICAST_TTL:
		case IP_MULTICAST_LOOP:
		case IP_ADD_MEMBERSHIP:
		case IP_DROP_MEMBERSHIP:
			error = ip_setmoptions(optname, inp, m);
			break;

		default:
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_ERROR event */
                    WV_NET_MARKER_4 (NET_AUX_EVENT, WV_NET_ERROR, 30, 5,
                                     WV_NETEVENT_IPOUTCTRL_BADCMD,
                                     so->so_fd, op, level, optname)
#endif  /* INCLUDE_WVNET */
#endif

			error = ENOPROTOOPT;
			break;
		}
		if (m)
			(void)m_free(m);
		break;

	case PRCO_GETOPT:
		switch (optname) {
		case IP_OPTIONS:
		case IP_RETOPTS:
			*mp = m = mBufClGet(M_WAIT, MT_SOOPTS, CL_SIZE_128,
					    TRUE);
			if (m == NULL)
			    {
			    error = ENOBUFS;
			    break;
			    }

			if (inp->inp_options) {
				m->m_len = inp->inp_options->m_len;
				bcopy(mtod(inp->inp_options, caddr_t),
				    mtod(m, caddr_t), (unsigned)m->m_len);
			} else
				m->m_len = 0;
			break;

		case IP_TOS:
		case IP_TTL:
		case IP_RECVOPTS:
		case IP_RECVRETOPTS:
		case IP_RECVDSTADDR:
			*mp = m = mBufClGet(M_WAIT, MT_SOOPTS, CL_SIZE_128,
					    TRUE);
			if (m == NULL)
			    {
			    error = ENOBUFS;
			    break;
			    }

			m->m_len = sizeof(int);
			switch (optname) {

			case IP_TOS:
				optval = inp->inp_ip.ip_tos;
				break;

			case IP_TTL:
				optval = inp->inp_ip.ip_ttl;
				break;

#define	OPTBIT(bit)	(inp->inp_flags & bit ? 1 : 0)

			case IP_RECVOPTS:
				optval = OPTBIT(INP_RECVOPTS);
				break;

			case IP_RECVRETOPTS:
				optval = OPTBIT(INP_RECVRETOPTS);
				break;

			case IP_RECVDSTADDR:
				optval = OPTBIT(INP_RECVDSTADDR);
				break;
			}
			*mtod(m, int *) = optval;
			break;

		case IP_MULTICAST_IF:
		case IP_MULTICAST_TTL:
		case IP_MULTICAST_LOOP:
		case IP_ADD_MEMBERSHIP:
		case IP_DROP_MEMBERSHIP:
			error = ip_getmoptions(optname, inp->inp_moptions, mp);
			break;

		default:
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_ERROR event */
                    WV_NET_MARKER_4 (NET_AUX_EVENT, WV_NET_ERROR, 30, 5,
                                     WV_NETEVENT_IPOUTCTRL_BADCMD,
                                     so->so_fd, op, level, optname)
#endif  /* INCLUDE_WVNET */
#endif

			error = ENOPROTOOPT;
			break;
		}
		break;
	}
	return (error);
}

/*
 * Set up IP options in pcb for insertion in output packets.
 * Store in mbuf with pointer in pcbopt, adding pseudo-option
 * with destination address if source routed.
 */
int
#ifdef notyet
ip_pcbopts(optname, pcbopt, m)
	int optname;
#else
ip_pcbopts(pcbopt, m)
#endif
	struct mbuf **pcbopt;
	register struct mbuf *m;
{
	register int cnt, optlen;
	register u_char *cp;
	u_char opt;

	/* turn off any old options */
	if (*pcbopt)
		(void)m_free(*pcbopt);
	*pcbopt = 0;
	if (m == (struct mbuf *)0 || m->m_len == 0) {
		/*
		 * Only turning off any previous options.
		 */
		if (m)
			(void)m_free(m);
		return (0);
	}

#ifndef	vax
	if (m->m_len % sizeof(long))
		goto bad;
#endif
	/*
	 * IP first-hop destination address will be stored before
	 * actual options; move other options back
	 * and clear it when none present.
	 */
#if 0 /*XXX changed for default cluster support vinai*/
	if (m->m_data + m->m_len + sizeof(struct in_addr) >= &m->m_dat[MLEN])
		goto bad;
#else
	if (m->m_data + m->m_len + sizeof(struct in_addr) >= 
	    (m->m_extBuf + m->m_extSize))
		goto bad;
#endif
	cnt = m->m_len;
	m->m_len += sizeof(struct in_addr);
	cp = mtod(m, u_char *) + sizeof(struct in_addr);
	ovbcopy(mtod(m, caddr_t), (caddr_t)cp, (unsigned)cnt);
	bzero(mtod(m, caddr_t), sizeof(struct in_addr));

	for (; cnt > 0; cnt -= optlen, cp += optlen) {
		opt = cp[IPOPT_OPTVAL];
		if (opt == IPOPT_EOL)
			break;
		if (opt == IPOPT_NOP)
			optlen = 1;
		else {
			optlen = cp[IPOPT_OLEN];
			if (optlen <= IPOPT_OLEN || optlen > cnt)
				goto bad;
		}
		switch (opt) {

		default:
			break;

		case IPOPT_LSRR:
		case IPOPT_SSRR:
			/*
			 * user process specifies route as:
			 *	->A->B->C->D
			 * D must be our final destination (but we can't
			 * check that since we may not have connected yet).
			 * A is first hop destination, which doesn't appear in
			 * actual IP option, but is stored before the options.
			 */
			if (optlen < IPOPT_MINOFF - 1 + sizeof(struct in_addr))
				goto bad;
			m->m_len -= sizeof(struct in_addr);
			cnt -= sizeof(struct in_addr);
			optlen -= sizeof(struct in_addr);
			cp[IPOPT_OLEN] = optlen;
			/*
			 * Move first hop before start of options.
			 */
			bcopy((caddr_t)&cp[IPOPT_OFFSET+1], mtod(m, caddr_t),
			    sizeof(struct in_addr));
			/*
			 * Then copy rest of options back
			 * to close up the deleted entry.
			 */
			ovbcopy((caddr_t)(&cp[IPOPT_OFFSET+1] +
			    sizeof(struct in_addr)),
			    (caddr_t)&cp[IPOPT_OFFSET+1],
			    (unsigned)cnt + sizeof(struct in_addr));
			break;
		}
	}
	if (m->m_len > MAX_IPOPTLEN + sizeof(struct in_addr))
		goto bad;
	*pcbopt = m;
	return (0);

bad:
	(void)m_free(m);
	return (EINVAL);
}

/*
 * Set the IP multicast options in response to user setsockopt().
 */
static int ip_setmoptions
    (
    int			optname,
    struct inpcb * 	pInPcb,
    struct mbuf *	m
    )
{
	register int error = 0;
	u_char loop;
	struct in_addr addr;
	register struct ip_mreq *mreq;
	register struct ifnet *ifp;
        register struct ip_moptions ** imop = &pInPcb->inp_moptions;
	register struct ip_moptions *imo = *imop;

	struct route ro;
	register struct sockaddr_in *dst;
        struct in_multi * 	pInMulti;
        M_BLK_ID 		pInmMblk = NULL;
        M_BLK **		ppMblk;

	if (imo == NULL) {
		/*
		 * No multicast option buffer attached to the pcb;
		 * allocate one and initialize to default values.
		 */
		MALLOC(imo,struct ip_moptions *, sizeof (*imo),
		       MT_IPMOPTS, M_WAIT);
		if (imo == NULL)
			return (ENOBUFS);
		*imop = imo;
		imo->imo_multicast_ifp = NULL;
		imo->imo_multicast_ttl = IP_DEFAULT_MULTICAST_TTL;
		imo->imo_multicast_loop = IP_DEFAULT_MULTICAST_LOOP;
		imo->imo_num_memberships = 0;
                imo->pInmMblk = NULL; 
	}

	switch (optname) {

	case IP_MULTICAST_IF:
		/*
		 * Select the interface for outgoing multicast packets.
		 */
		if (m == NULL || m->m_len != sizeof(struct in_addr)) {
			error = EINVAL;
			break;
		}
		addr = *(mtod(m, struct in_addr *));
		/*
		 * INADDR_ANY is used to remove a previous selection.
		 * When no interface is selected, a default one is
		 * chosen every time a multicast packet is sent.
		 */
		if (addr.s_addr == INADDR_ANY) {
			imo->imo_multicast_ifp = NULL;
			break;
		}
		/*
		 * The selected interface is identified by its local
		 * IP address.  Find the interface and confirm that
		 * it supports multicasting.
		 */
		INADDR_TO_IFP(addr, ifp);
		if (ifp == NULL || (ifp->if_flags & IFF_MULTICAST) == 0) {
			error = EADDRNOTAVAIL;
			break;
		}
		imo->imo_multicast_ifp = ifp;
		break;

	case IP_MULTICAST_TTL:
		/*
		 * Set the IP time-to-live for outgoing multicast packets.
		 */
		if (m == NULL || m->m_len != 1) {
			error = EINVAL;
			break;
		}
		imo->imo_multicast_ttl = *(mtod(m, u_char *));
		break;

	case IP_MULTICAST_LOOP:
		/*
		 * Set the loopback flag for outgoing multicast packets.
		 * Must be zero or one.
		 */
		if (m == NULL || m->m_len != 1 ||
		   (loop = *(mtod(m, u_char *))) > 1) {
			error = EINVAL;
			break;
		}
		imo->imo_multicast_loop = loop;
		break;

	case IP_ADD_MEMBERSHIP:
		/*
		 * Add a multicast group membership.
		 * Group must be a valid IP multicast address.
		 */
		if (m == NULL || m->m_len != sizeof(struct ip_mreq)) {
			error = EINVAL;
			break;
		}
		mreq = mtod(m, struct ip_mreq *);
		if (!IN_MULTICAST(ntohl(mreq->imr_multiaddr.s_addr))) {
			error = EINVAL;
			break;
		}
		/*
		 * If no interface address was provided, use the interface of
		 * the route to the given multicast address.
		 */
		if (mreq->imr_interface.s_addr == INADDR_ANY) {
			ro.ro_rt = NULL;
			dst = (struct sockaddr_in *)&ro.ro_dst;
			dst->sin_len = sizeof(*dst);
			dst->sin_family = AF_INET;
			dst->sin_addr = mreq->imr_multiaddr;
			rtalloc(&ro);
			if (ro.ro_rt == NULL) {
				error = EADDRNOTAVAIL;
				break;
			}
			ifp = ro.ro_rt->rt_ifp;
			rtfree(ro.ro_rt);
		}
		else {
			INADDR_TO_IFP(mreq->imr_interface, ifp);
		}
		/*
		 * See if we found an interface, and confirm that it
		 * supports multicast.
		 */
		if (ifp == NULL || (ifp->if_flags & IFF_MULTICAST) == 0) {
			error = EADDRNOTAVAIL;
			break;
		}

                ppMblk = &imo->pInmMblk;
                while (*ppMblk)
                    {
                    pInMulti = mtod (*ppMblk, struct in_multi*);
                    if ((pInMulti->inm_ifp == ifp) &&
                        (pInMulti->inm_addr.s_addr ==
                         mreq->imr_multiaddr.s_addr))
                        break;		/* jump out of the while loop */

                    ppMblk = &(*ppMblk)->mBlkHdr.mNext;
                    }

                if (*ppMblk == NULL)
                    {
                    if ((pInmMblk = in_addmulti (&mreq->imr_multiaddr,
                                                 ifp, pInPcb)) == NULL)
                        {
			error = ENOBUFS;
                        break;
                        }
                    *ppMblk = pInmMblk;		/* add to the list */
                    ++imo->imo_num_memberships;
                    }
                else
                    error = EADDRINUSE;

                break;

	case IP_DROP_MEMBERSHIP:
		/*
		 * Drop a multicast group membership.
		 * Group must be a valid IP multicast address.
		 */
		if (m == NULL || m->m_len != sizeof(struct ip_mreq)) {
			error = EINVAL;
			break;
		}
		mreq = mtod(m, struct ip_mreq *);
		if (!IN_MULTICAST(ntohl(mreq->imr_multiaddr.s_addr))) {
			error = EINVAL;
			break;
		}
		/*
		 * If an interface address was specified, get a pointer
		 * to its ifnet structure.
		 */
		if (mreq->imr_interface.s_addr == INADDR_ANY)
			ifp = NULL;
		else {
			INADDR_TO_IFP(mreq->imr_interface, ifp);
			if (ifp == NULL) {
				error = EADDRNOTAVAIL;
				break;
			}
		}

                {
                ppMblk = &imo->pInmMblk;
                while (*ppMblk)
                    {
                    pInMulti = mtod (*ppMblk, struct in_multi*);

                    if ((ifp == NULL || pInMulti->inm_ifp == ifp) &&
                        (pInMulti->inm_addr.s_addr ==
                         mreq->imr_multiaddr.s_addr))
                        {
                        pInmMblk = *ppMblk;
                        /* delete from the list */
                        *ppMblk = (*ppMblk)->mBlkHdr.mNext; 
                        break;		/* jump out of the while loop */
                        }
                    ppMblk = &(*ppMblk)->mBlkHdr.mNext;
                    }

                if (pInmMblk == NULL)
                    {
                    error = EADDRNOTAVAIL;
                    break;
                    }
                
		/*
		 * Give up the multicast address record to which the
		 * membership points.
		 */
		in_delmulti (pInmMblk, pInPcb);

		--imo->imo_num_memberships;
                
		break;
                }

	default:
		error = EOPNOTSUPP;
		break;
	}

	/*
	 * If all options have default values, no need to keep the mbuf.
	 */
	if (imo->imo_multicast_ifp == NULL &&
	    imo->imo_multicast_ttl == IP_DEFAULT_MULTICAST_TTL &&
	    imo->imo_multicast_loop == IP_DEFAULT_MULTICAST_LOOP &&
	    imo->imo_num_memberships == 0) {
		 FREE(*imop, MT_IPMOPTS);
		*imop = NULL;
	}

	return (error);
}

/*
 * Return the IP multicast options in response to user getsockopt().
 */
int
ip_getmoptions(optname, imo, mp)
	int optname;
	register struct ip_moptions *imo;
	register struct mbuf **mp;
{
	u_char *ttl;
	u_char *loop;
	struct in_addr *addr;
	struct in_ifaddr *ia;

	*mp = mBufClGet(M_WAIT, MT_SOOPTS, CL_SIZE_128, TRUE);

	if (*mp == NULL)
	    return (ENOBUFS);
	    
	switch (optname) {

	case IP_MULTICAST_IF:
		addr = mtod(*mp, struct in_addr *);
		(*mp)->m_len = sizeof(struct in_addr);
		if (imo == NULL || imo->imo_multicast_ifp == NULL)
			addr->s_addr = INADDR_ANY;
		else {
			IFP_TO_IA(imo->imo_multicast_ifp, ia);
			addr->s_addr = (ia == NULL) ? INADDR_ANY
					: IA_SIN(ia)->sin_addr.s_addr;
		}
		return (0);

	case IP_MULTICAST_TTL:
		ttl = mtod(*mp, u_char *);
		(*mp)->m_len = 1;
		*ttl = (imo == NULL) ? IP_DEFAULT_MULTICAST_TTL
				     : imo->imo_multicast_ttl;
		return (0);

	case IP_MULTICAST_LOOP:
		loop = mtod(*mp, u_char *);
		(*mp)->m_len = 1;
		*loop = (imo == NULL) ? IP_DEFAULT_MULTICAST_LOOP
				      : imo->imo_multicast_loop;
		return (0);

	default:
		return (EOPNOTSUPP);
	}
}

/*
 * Discard the IP multicast options.
 */
void
ip_freemoptions(imo, pInPcb)
	register struct ip_moptions *imo;
	struct inpcb * 	pInPcb;
        
{
        M_BLK_ID	pInmMblk;
        M_BLK_ID	pInmMblkNext;

	if (imo != NULL) {
                pInmMblk = imo->pInmMblk;
                pInmMblkNext = imo->pInmMblk;
        	while ((pInmMblk = pInmMblkNext) != NULL)
                    {
                    pInmMblkNext = pInmMblk->mBlkHdr.mNext; 
                    in_delmulti (pInmMblk, pInPcb);
                    }
		FREE(imo, MT_IPMOPTS);
	}
}

/*
 * Routine called from ip_output() to loop back a copy of an IP multicast
 * packet to the input queue of a specified interface.  Note that this
 * calls the output routine of the loopback "driver", but with an interface
 * pointer that might NOT be &loif -- easier than replicating that code here.
 */
void
ip_mloopback(ifp, m, dst, rt)
	struct ifnet *ifp;
	register struct mbuf *m;
	register struct sockaddr_in *dst;
	struct rtentry *rt;
{
	register struct ip *ip;
	struct mbuf *copym;
        struct mbuf * pMbuf;

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_INFO event */
    WV_NET_EVENT_0 (NET_AUX_EVENT, WV_NET_INFO, 12, 7,
                    WV_NETEVENT_IPMLOOP_START, WV_NET_RECV)
#endif  /* INCLUDE_WVNET */
#endif

	copym = m_copy(m, 0, M_COPYALL);
	if (copym != NULL) {
		/*
		 * We don't bother to fragment if the IP length is greater
		 * than the interface's MTU.  Can this possibly matter?
		 */
		ip = mtod(copym, struct ip *);

                /*
                 * This copying of ip header is done because the same
                 * ip header is shared by looutput and the code under the
                 * the label sendit: in ip_output. so a fresh mbuf has to be
                 * linked to the data.
                 */
                if ((pMbuf = mBufClGet (M_DONTWAIT, MT_DATA, sizeof(struct ip),
                                   TRUE)) == NULL)
                    {
                    m_freem (copym); 
                    return;
                    }

                /* copy the ip header */
                bcopy ((char *)ip , (char *)(mtod(pMbuf, char *)),
                       sizeof(struct ip));

                pMbuf->m_len   = sizeof (struct ip); 
                pMbuf->m_flags = copym->m_flags;	/* copy the flags */
                pMbuf->m_pkthdr = copym->m_pkthdr;	/* copy the pkt hdr */
                pMbuf->m_next = copym;
                
                copym->m_flags &= ~M_PKTHDR;
                copym->m_len -= sizeof(struct ip);
                copym->m_data += sizeof(struct ip);
		ip = mtod(pMbuf, struct ip *);
                copym = pMbuf; 

		ip->ip_len = htons((u_short)ip->ip_len);
		ip->ip_off = htons((u_short)ip->ip_off);

                ip->ip_sum = 0;
                if (_ipCfgFlags & IP_DO_CHECKSUM_SND)
                    ip->ip_sum = in_cksum(copym, ip->ip_hl << 2);

		(void) looutput(ifp, copym, (struct sockaddr *)dst, rt);
	}
}
