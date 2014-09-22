/* if_subr.c - common routines for network interface drivers */

/* Copyright 1990 - 2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
 * Copyright (c) 1982, 1989, 1993
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
 *	@(#)if_ethersubr.c	8.1 (Berkeley) 6/10/93
 */

/*
modification history
--------------------
01q,23may02,vvv  removed unnecessary m_copy from ether_output (SPR #77775)
01p,22apr02,wap  call ip_mloopback() rather than calling looutput() directly
                 (SPR #72246)
01o,15apr02,wap  reinstate netJobAdd removal changes
01n,29mar02,wap  back out the previous change (needs more investigation)
01m,21mar02,rae  remove unecessary netJobAdd() SPR #
01l,21mar02,vvv  updated man page for copyFromMbufs (SPR #20787)
01k,01nov01,rae  fixed compile warnings, corrected WV event (SPR #71284)
01j,12oct01,rae  merge from truestack ver 01o, base 01h (SPR #69112 etc.)
01i,17oct00,spm  updated ether_attach to report memory allocation failures
01h,21aug98,n_s  updated if_omcast in ether_output () if_noproto in 
                 do_protocol_with_type (). spr21074
01f,19sep97,vin  modified build_cluster to incorporate cluster blks,
		 removed pointers to refcounts.
01g,11aug97,vin  changed mBlkGet to use _pNetDpool.
01e,10jul97,vin  removed unsupported revarpinput() code.
01f,05jun97,vin  corrected processing for promiscuous interfaces in
		 ether_input.
01e,02jun97,spm  updated man page for bcopy_to_mbufs()
01d,15may97,vin  reworked ether_input, cleanup, added copyFromMbufs().
		 removed support for check_trailer().
01c,16dec96,vin  removed unnecessary ntohs(). 
01b,13nov96,vin  replaced m_gethdr with mBlkGet() in buildCluster.
01a,03mar96,vin  created from BSD4.4 stuff,integrated with 02p of if_subr.c
*/

#include "vxWorks.h"
#include "stdlib.h"
#include "stdio.h"
#include "tickLib.h"
#include "logLib.h"
#include "errno.h"
#include "netinet/in.h"
#include "netinet/in_systm.h"
#include "netinet/in_var.h"
#include "netinet/ip.h"
#include "net/if.h"
#include "net/if_arp.h"
#include "netinet/if_ether.h"
#include "net/if_subr.h"
#include "netinet/in_var.h"
#include "netinet/ip_var.h"    /* for ipintr() prototype */
#include "net/mbuf.h"
#include "net/route.h"
#include "net/if_llc.h"
#include "net/if_dl.h"
#include "net/if_types.h"
#include "lstLib.h"
#include "netLib.h"
#include "muxLib.h"
#include "memPartLib.h"

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET
#include "wvNetLib.h"
#endif /* INCLUDE_WVNET */
#endif

#ifdef VIRTUAL_STACK
#include "netinet/vsLib.h"
#endif

typedef struct net_type
    {
    NODE     	node;
    FUNCPTR     inputRtn;
    int     	etherType;
    } NET_TYPE;

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET
    /* Set common fields of event identifiers for this module. */

LOCAL UCHAR wvNetModuleId = WV_NET_IFSUBR_MODULE;   /* Value for if_subr.c */
LOCAL UCHAR wvNetLocalFilter = WV_NET_NONE;     /* Available event filter */

LOCAL ULONG wvNetEventId;       /* Event identifier: see wvNetLib.h */
#endif    /* INCLUDE_WVNET */
#endif

/* externs */

#ifndef VIRTUAL_STACK
extern	struct ifnet loif;
extern  void ip_mloopback(struct ifnet *, struct mbuf *, struct sockaddr_in *,
                          struct rtentry *rt);
#endif /* VIRTUAL_STACK */

u_char	etherbroadcastaddr[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
u_char	ether_ipmulticast_min[6] = { 0x01, 0x00, 0x5e, 0x00, 0x00, 0x00 };
u_char	ether_ipmulticast_max[6] = { 0x01, 0x00, 0x5e, 0x7f, 0xff, 0xff };

#define senderr(e) { error = (e); goto bad;}

LOCAL LIST netTypeList;

/*
 * Ethernet output routine.
 * Encapsulate a packet of type family for the local net.
 * Use trailer local net encapsulation if enough data in first
 * packet leaves a multiple of 512 bytes of data in remainder.
 * Assumes that ifp is actually pointer to arpcom structure.
 */
int
ether_output(ifp, m0, dst, rt0)
	register struct ifnet *ifp;
	struct mbuf *m0;
	struct sockaddr *dst;
	struct rtentry *rt0;
{
	u_short etype;
	int s, error = 0;
 	u_char edst[6];
	register struct mbuf *m = m0;
	register struct rtentry *rt;
	register struct ether_header *eh;
	int off, len = m->m_pkthdr.len;
	struct arpcom *ac = (struct arpcom *)ifp;

	if ((ifp->if_flags & (IFF_UP|IFF_RUNNING)) != (IFF_UP|IFF_RUNNING))
            {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_CRITICAL event */
            WV_NET_EVENT_1 (NET_CORE_EVENT, WV_NET_CRITICAL, 2, 3,
                            WV_NETEVENT_ETHEROUT_IFDOWN, WV_NET_SEND, ifp)
#endif  /* INCLUDE_WVNET */
#endif

            senderr(ENETDOWN);
            }
	ifp->if_lastchange = tickGet();
	if ((rt = rt0)) {
		if ((rt->rt_flags & RTF_UP) == 0) {
			if ((rt0 = rt = rtalloc1(dst, 1)))
				rt->rt_refcnt--;
			else {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_CRITICAL event */
            WV_NET_DSTADDROUT_EVENT_1 (NET_CORE_EVENT, WV_NET_CRITICAL, 4, 5,
                            ((struct sockaddr_in *)dst)->sin_addr.s_addr,
                            WV_NETEVENT_ETHEROUT_NOROUTE, WV_NET_SEND,
                            ((struct sockaddr_in *)dst)->sin_addr.s_addr)
#endif  /* INCLUDE_WVNET */
#endif

                             senderr(EHOSTUNREACH);
                             }
		}
		if (rt->rt_flags & RTF_GATEWAY) {
			if (rt->rt_gwroute == 0)
				goto lookup;
			if (((rt = rt->rt_gwroute)->rt_flags & RTF_UP) == 0) {
				rtfree(rt); rt = rt0;
			lookup: rt->rt_gwroute = rtalloc1(rt->rt_gateway, 1);
				if ((rt = rt->rt_gwroute) == 0) {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_CRITICAL event */
            WV_NET_DSTADDROUT_EVENT_1 (NET_CORE_EVENT, WV_NET_CRITICAL, 4, 5,
                       ((struct sockaddr_in *)rt->rt_gateway)->sin_addr.s_addr,
                                    WV_NETEVENT_ETHEROUT_NOROUTE, WV_NET_SEND,
                       ((struct sockaddr_in *)rt->rt_gateway)->sin_addr.s_addr)
#endif  /* INCLUDE_WVNET */
#endif
                                    senderr(EHOSTUNREACH);
                                }
			}
		}
		if (rt->rt_flags & RTF_REJECT)
			if (rt->rt_rmx.rmx_expire == 0 ||
			    tickGet() < rt->rt_rmx.rmx_expire) {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_CRITICAL event */
        WV_NET_EVENT_0 (NET_CORE_EVENT, WV_NET_CRITICAL, 5, 6,
                        WV_NETEVENT_ETHEROUT_RTREJECT, WV_NET_SEND)
#endif  /* INCLUDE_WVNET */
#endif
                            senderr(rt == rt0 ? EHOSTDOWN : EHOSTUNREACH);
                            }
	}
	switch (dst->sa_family) {

#ifdef INET
	case AF_INET:
		if (!arpresolve(ac, rt, m, dst, edst))
			return (0);	/* if not yet resolved */
		/* If broadcasting on a simplex interface, loopback a copy */
		if ((m->m_flags & M_BCAST) && (ifp->if_flags & IFF_SIMPLEX))
			ip_mloopback (ifp, m, (struct sockaddr_in *)dst,
				      (struct rtentry*)rt);
		off = m->m_pkthdr.len - m->m_len;
		etype = ETHERTYPE_IP;
		break;
#endif
	case AF_UNSPEC:
		eh = (struct ether_header *)dst->sa_data;
 		bcopy((caddr_t)eh->ether_dhost, (caddr_t)edst, sizeof (edst));
		etype = eh->ether_type;
		break;

	default:
		printf("%s%d: can't handle af%d\n", ifp->if_name, ifp->if_unit,
			dst->sa_family);
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_ERROR event */
                WV_NET_EVENT_1 (NET_CORE_EVENT, WV_NET_ERROR, 3, 7,
                                WV_NETEVENT_ETHEROUT_AFNOTSUPP, WV_NET_SEND,
                                dst->sa_family)
#endif  /* INCLUDE_WVNET */
#endif
		senderr(EAFNOSUPPORT);
	}

	/*
	 * Add local net header.  If no space in first mbuf,
	 * allocate another.
	 */
	M_PREPEND(m, sizeof (struct ether_header), M_DONTWAIT);

	if (m == 0)
            {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_CRITICAL event */
            WV_NET_EVENT_1 (NET_CORE_EVENT, WV_NET_CRITICAL, 3, 4,
                            WV_NETEVENT_ETHEROUT_NOBUFS, WV_NET_SEND, ifp)
#endif  /* INCLUDE_WVNET */
#endif

            senderr(ENOBUFS);
            }

	eh = mtod(m, struct ether_header *);
	etype = htons(etype);
	bcopy((caddr_t)&etype,(caddr_t)&eh->ether_type,
		sizeof(eh->ether_type));
 	bcopy((caddr_t)edst, (caddr_t)eh->ether_dhost, sizeof (edst));
 	bcopy((caddr_t)ac->ac_enaddr, (caddr_t)eh->ether_shost,
	    sizeof(eh->ether_shost));
	s = splimp();
	/*
	 * Queue message on interface, and start output if interface
	 * not yet active.
	 */
	if (IF_QFULL(&ifp->if_snd)) {
		IF_DROP(&ifp->if_snd);
		splx(s);
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_CRITICAL event */
            WV_NET_EVENT_1 (NET_CORE_EVENT, WV_NET_CRITICAL, 3, 4,
                            WV_NETEVENT_ETHEROUT_NOBUFS, WV_NET_SEND, ifp)
#endif  /* INCLUDE_WVNET */
#endif

		senderr(ENOBUFS);
	}
	IF_ENQUEUE(&ifp->if_snd, m);

	if ((ifp->if_flags & IFF_OACTIVE) == 0)
		(*ifp->if_start)(ifp);
	splx(s);
	ifp->if_obytes += len + sizeof (struct ether_header);
	if (m->m_flags & M_MCAST || m->m_flags & M_BCAST)
		ifp->if_omcasts++;

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_NOTICE event */
        WV_NET_EVENT_2 (NET_CORE_EVENT, WV_NET_NOTICE, 3, 12,
                        WV_NETEVENT_ETHEROUT_FINISH, WV_NET_SEND, ifp, error)
#endif  /* INCLUDE_WVNET */
#endif
	return (error);

bad:
	if (m)
		m_freem(m);
	return (error);
}

/*******************************************************************************
*
* do_protocol - check the link-level type field and process IP and ARP data
*
* This routine must not be called at interrupt level.
* Process a received Ethernet packet;
* the packet is in the mbuf chain m without
* the ether header, which is provided separately.
*
* NOMANUAL
*/

void
ether_input(ifp, eh, m)
	struct ifnet *ifp;
	register struct ether_header *eh;
	struct mbuf *m;
{
	u_short 		etype;
	struct arpcom *		ac = (struct arpcom *)ifp;

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_NOTICE event */
        WV_NET_EVENT_1 (NET_CORE_EVENT, WV_NET_NOTICE, 4, 13,
                        WV_NETEVENT_ETHERIN_START, WV_NET_RECV, ifp)
#endif  /* INCLUDE_WVNET */
#endif

	if ((ifp->if_flags & IFF_UP) == 0) {
		m_freem(m);
		return;
	}
	ifp->if_lastchange = tickGet();
	ifp->if_ibytes += m->m_pkthdr.len + sizeof (*eh);
	if (eh->ether_dhost[0] & 1) {
		if (bcmp((caddr_t)etherbroadcastaddr, (caddr_t)eh->ether_dhost,
		    sizeof(etherbroadcastaddr)) == 0)
			m->m_flags |= M_BCAST;
		else
			m->m_flags |= M_MCAST;
		ifp->if_imcasts++;
	}

        if (ifp->if_flags & IFF_PROMISC)
            {
            /*
             * do not hand over the non multicast packets to the ip stack
             * if they are not destined to us, orelse it confuses the
             * ip forwarding logic and keeps sending unnecessary redirects.
             * If packets destined for other hosts have to be snooped they
             * have to done through driver level hooks.
             */
            
            if (!(m->m_flags & (M_BCAST | M_MCAST)))
                {
                if (bcmp ((caddr_t)ac->ac_enaddr, (caddr_t)eh->ether_dhost,
                          sizeof(eh->ether_dhost)) != 0)
                    {
                    m_freem (m); 	/* free the packet chain */
                    return; 
                    }
                }
            }
        
	etype = ntohs(eh->ether_type);

        /* add code here to process IEEE802.3 LLC format, Hook!! */

        /* demux the packet and hand off the ip layer */
        do_protocol_with_type (etype, m , ac, m->m_pkthdr.len); 
}

/*
 * Perform common duties while attaching to interface list
 */
void
ether_ifattach(ifp)
	register struct ifnet *ifp;
{
	register struct ifaddr *ifa;
	register struct sockaddr_dl *sdl;

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_VERBOSE event */
        WV_NET_MARKER_1 (NET_AUX_EVENT, WV_NET_VERBOSE, 11, 14,
                         WV_NETEVENT_ETHERIFATTACH_START, ifp)
#endif  /* INCLUDE_WVNET */
#endif

	ifp->if_type = IFT_ETHER;
	ifp->if_addrlen = 6;
	ifp->if_hdrlen = 14;
	ifp->if_mtu = ETHERMTU;
	for (ifa = ifp->if_addrlist; ifa; ifa = ifa->ifa_next)
		if ((sdl = (struct sockaddr_dl *)ifa->ifa_addr) &&
		    sdl->sdl_family == AF_LINK) {
			sdl->sdl_type = IFT_ETHER;
			sdl->sdl_alen = ifp->if_addrlen;
			bcopy((caddr_t)((struct arpcom *)ifp)->ac_enaddr,
			      LLADDR(sdl), ifp->if_addrlen);
			break;
		}
}

/*
 * Add an Ethernet multicast address or range of addresses to the list for a
 * given interface.
 */
int
ether_addmulti(ifr, ac)
	struct ifreq *ifr;
	register struct arpcom *ac;
{
	register struct ether_multi *enm;
	struct sockaddr_in *sin;
	u_char addrlo[6];
	u_char addrhi[6];
	int s = splimp();

/*
 * XXX - This event cannot currently occur: the ether_addmulti() routine
 *       provides multicasting support which is not available for VxWorks
 *       BSD Ethernet drivers.

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /@ WV_NET_VERBOSE event @/
        WV_NET_MARKER_1 (NET_AUX_EVENT, WV_NET_VERBOSE, 12, 15,
                         WV_NETEVENT_ADDMULT_START, ac->ac_if)
#endif  /@ INCLUDE_WVNET @/
#endif

 * XXX - end of unused event
 */

	switch (ifr->ifr_addr.sa_family) {

	case AF_UNSPEC:
		bcopy((char *)ifr->ifr_addr.sa_data, (char *)addrlo, 6);
		bcopy((char *) addrlo, (char *)addrhi, 6);
		break;

#ifdef INET
	case AF_INET:
		sin = (struct sockaddr_in *)&(ifr->ifr_addr);
		if (sin->sin_addr.s_addr == INADDR_ANY) {
			/*
			 * An IP address of INADDR_ANY means listen to all
			 * of the Ethernet multicast addresses used for IP.
			 * (This is for the sake of IP multicast routers.)
			 */
			bcopy((char *)ether_ipmulticast_min, (char *)addrlo, 6);
			bcopy((char *)ether_ipmulticast_max, (char *)addrhi, 6);
		}
		else {
			ETHER_MAP_IP_MULTICAST(&sin->sin_addr, addrlo);
			bcopy((char *)addrlo, (char *)addrhi, 6);
		}
		break;
#endif

	default:
		splx(s);
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_ERROR event */
                WV_NET_MARKER_1 (NET_AUX_EVENT, WV_NET_ERROR, 4, 8,
                                WV_NETEVENT_ADDMULT_AFNOTSUPP,
                                ifr->ifr_addr.sa_family) 
#endif  /* INCLUDE_WVNET */
#endif
		return (EAFNOSUPPORT);
	}

	/*
	 * Verify that we have valid Ethernet multicast addresses.
	 */
	if ((addrlo[0] & 0x01) != 1 || (addrhi[0] & 0x01) != 1) {
		splx(s);
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_ERROR event */
                WV_NET_MARKER_1 (NET_AUX_EVENT, WV_NET_ERROR, 5, 9,
                                WV_NETEVENT_ADDMULT_BADADDR,
                                ac->ac_if) 
#endif  /* INCLUDE_WVNET */
#endif

		return (EINVAL);
	}
	/*
	 * See if the address range is already in the list.
	 */
	ETHER_LOOKUP_MULTI(addrlo, addrhi, ac, enm);
	if (enm != NULL) {
		/*
		 * Found it; just increment the reference count.
		 */
		++enm->enm_refcount;
		splx(s);
		return (0);
	}
	/*
	 * New address or range; malloc a new multicast record
	 * and link it into the interface's multicast list.
	 */
        MALLOC (enm, struct ether_multi *, sizeof(*enm), MT_IFMADDR, 
		M_DONTWAIT);
	if (enm == NULL) {
		splx(s);

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_EMERGENCY event */
                WV_NET_MARKER_0 (NET_AUX_EVENT, WV_NET_EMERGENCY, 6, 1,
                                 WV_NETEVENT_ADDMULT_NOBUFS)
#endif  /* INCLUDE_WVNET */
#endif
		return (ENOBUFS);
	}
	bcopy((char *)addrlo, (char *)enm->enm_addrlo, 6);
	bcopy((char *)addrhi, (char *)enm->enm_addrhi, 6);
	enm->enm_ac = ac;
	enm->enm_refcount = 1;
	enm->enm_next = ac->ac_multiaddrs;
	ac->ac_multiaddrs = enm;
	ac->ac_multicnt++;
	splx(s);
	/*
	 * Return ENETRESET to inform the driver that the list has changed
	 * and its reception filter should be adjusted accordingly.
	 */
	return (ENETRESET);
}

/*
 * Delete a multicast address record.
 */
int
ether_delmulti(ifr, ac)
	struct ifreq *ifr;
	register struct arpcom *ac;
{
	register struct ether_multi *enm;
	register struct ether_multi **p;
	struct sockaddr_in *sin;
	u_char addrlo[6];
	u_char addrhi[6];
	int s = splimp();

/*
 * XXX - This event cannot currently occur: the ether_delmulti() routine
 *       provides multicasting support which is not available for VxWorks
 *       BSD Ethernet drivers.

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /@ WV_NET_VERBOSE event @/
        WV_NET_MARKER_1 (NET_AUX_EVENT, WV_NET_VERBOSE, 13, 16,
                            WV_NETEVENT_DELMULT_START, ac->ac_if)
#endif  /@ INCLUDE_WVNET @/
#endif

 * XXX - end of unused event
 */

	switch (ifr->ifr_addr.sa_family) {

	case AF_UNSPEC:
		bcopy((char *)ifr->ifr_addr.sa_data, (char *)addrlo, 6);
		bcopy((char *)addrlo, (char *)addrhi, 6);
		break;

#ifdef INET
	case AF_INET:
		sin = (struct sockaddr_in *)&(ifr->ifr_addr);
		if (sin->sin_addr.s_addr == INADDR_ANY) {
			/*
			 * An IP address of INADDR_ANY means stop listening
			 * to the range of Ethernet multicast addresses used
			 * for IP.
			 */
			bcopy((char *)ether_ipmulticast_min, (char *)addrlo, 6);
			bcopy((char *)ether_ipmulticast_max, (char *)addrhi, 6);
		}
		else {
			ETHER_MAP_IP_MULTICAST(&sin->sin_addr, addrlo);
			bcopy((char *)addrlo, (char *)addrhi, 6);
		}
		break;
#endif

	default:
		splx(s);
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_ERROR event */
                WV_NET_MARKER_1 (NET_AUX_EVENT, WV_NET_ERROR, 6, 10,
                                 WV_NETEVENT_DELMULT_AFNOTSUPP,
                                 ifr->ifr_addr.sa_family)
#endif  /* INCLUDE_WVNET */
#endif
		return (EAFNOSUPPORT);
	}

	/*
	 * Look up the address in our list.
	 */
	ETHER_LOOKUP_MULTI(addrlo, addrhi, ac, enm);
	if (enm == NULL) {
		splx(s);
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_ERROR event */
                WV_NET_MARKER_1 (NET_AUX_EVENT, WV_NET_ERROR, 7, 11,
                                 WV_NETEVENT_DELMULT_BADADDR, ac->ac_if)
#endif  /* INCLUDE_WVNET */
#endif
		return (ENXIO);
	}
	if (--enm->enm_refcount != 0) {
		/*
		 * Still some claims to this record.
		 */
		splx(s);
		return (0);
	}
	/*
	 * No remaining claims to this record; unlink and free it.
	 */
	for (p = &enm->enm_ac->ac_multiaddrs;
	     *p != enm;
	     p = &(*p)->enm_next)
		continue;
	*p = (*p)->enm_next;
	FREE(enm, MT_IFMADDR);
	ac->ac_multicnt--;
	splx(s);
	/*
	 * Return ENETRESET to inform the driver that the list has changed
	 * and its reception filter should be adjusted accordingly.
	 */
	return (ENETRESET);
}

/*******************************************************************************
*
* check_trailer - check ethernet frames that have trailers following them
*
* Decypher the ethernet frames that are encoded via trailer protocol which
* has a link level type field that contains a value which is an encoding of
* the size of the data portion.  The original BSD 4.2 and 4.3 TCP/IP
* implementations use the trailer protocol to accomplish more efficient
* copying of data.  Basically the idea was to put the variable lenght protocol
* headers (TCP, UDP, IP headers) at the end of a page aligned data and use
* the link-level type field (ethernet type) to indicate both the use of a
* specialized framing protocol as well as the length of the data following
* the ethernet header.  The original link-level type field and the length
* of the protocol headers following the data portion are attached between
* the data portion (which should be of length multiple of 512 bytes) and
* protocol headers.  (size of data portion == (type - 0x1000) * 415)
*
* Upon return, 'pOff' will contain NULL if trailer protocol is not being
* used.  Otherwise, 'pOff' points to the trailer header and 'pLen'
* will contain the size of the frame.
*
* NOMANUAL
*/

void check_trailer (eh, pData, pLen, pOff, ifp)
    FAST struct ether_header 	*eh;	/* ethernet header */
    FAST unsigned char 		*pData;	/* data immediately after 'eh' */
    FAST int 			*pLen;	/* value/result input data length */
    FAST int 			*pOff;	/* result -- points to trailer header */
    FAST struct ifnet 		*ifp;	/* network interface */

    {
#if 0 /* XXX No support for trailers in BSD44 vxWorks */    
    FAST int resid;

    eh->ether_type = ntohs((u_short)eh->ether_type);

    if ((eh->ether_type >= ETHERTYPE_TRAIL)  &&
	(eh->ether_type < ETHERTYPE_TRAIL+ETHERTYPE_NTRAILER))
	{
	*pOff = (eh->ether_type - ETHERTYPE_TRAIL) * 512;

	if (*pOff >= ifp->if_mtu)
	    return;		/* sanity */

	eh->ether_type = ntohs (*(u_short *) (pData + *pOff));
	resid          = ntohs (*(u_short *) (pData + *pOff + 2));

	if ((*pOff + resid) > *pLen)
	    return;		/* sanity */

	*pLen = *pOff + resid;
	}
    else
	*pOff = 0;
#endif /* XXX No trailer support in BSD44 */    
    }

/*******************************************************************************
*
* bcopy_to_mbufs - copy data into mbuf chain
*
* Copies up to <totlen> bytes of data from <buf0> into a freshly allocated
* mbuf. The <off0> parameter indicates the amount of trailer encapsulation,
* which was formerly handled according to the trailer protocol described in
* check_trailer(). Since trailer encapsulation is not supported in BSD 4.4,
* that parameter is unused. The <width> argument supports machines which 
* have special byte alignment requirements when copying data from device 
* I/O buffers to main memory.  A macro function called 'copy_to_mbufs' is 
* provided in the "mbuf.h" file for compatability with SunOS, which doesn't 
* support the <width> parameter. This function uses mbuf clusters when possible
* to enhance network throughput.
*
* RETURNS: first mbuf or 0 if error
*
* NOMANUAL
*/

struct mbuf *bcopy_to_mbufs (buf0, totlen, off0, ifp, width)
    u_char		*buf0;		/* buffer to be copied into mbufs */
    int			totlen;		/* number of bytes to be copied */
    int			off0;		/* trailer protocol indicative */
    struct ifnet	*ifp;		/* network interface */
    int			width;		/* copy data by this unit width */

    {
    return (m_devget ((char *) buf0, totlen, width, ifp, NULL));
    }

/*******************************************************************************
* copyFromMbufs - copy data from an mbuf chain
*
* This routine copies data from an mbuf chain into a given buffer.
* The length of the data copied from the mbuf chain is returned.
* The argument width is provided for machines that have special byte alignment
* requirements when copying data to an I/O buffer of the device from main
* memory. This routine frees the mbuf chain passed to it. 
*
* This routine should not be used since it can cause unaligned memory accesses
* resulting in target crashes.
* 
* RETURNS: length of data copied.
*
* NOMANUAL
*/

int copyFromMbufs
    (
    char * 		pIoBuf,		/* buffer to copy mbufs into */
    struct mbuf * 	pMbuf,		/* pointer to an mbuf chain */
    int			width		/* width of the copy */
    )
    {
    FAST char *		pBuf = (char *) (pIoBuf);
    FAST struct mbuf * 	pMbufHead = pMbuf;
    FAST char *		adjBuf;
    FAST int 		adjLen;
    FAST int 		odd = 0;
    FAST int 		ix;
    char 		temp [4];

    if (width == NONE)
        {
        for (; pMbuf != NULL; pBuf += pMbuf->m_len, pMbuf = pMbuf->m_next)
            bcopy (mtod (pMbuf, char *), pBuf, pMbuf->m_len);
        }

    else if (width == 1)
        {
	for (; pMbuf != NULL; pBuf += pMbuf->m_len, pMbuf = pMbuf->m_next)
	    bcopyBytes (mtod (pMbuf, char *), pBuf, pMbuf->m_len);
        }
    
    else if (width == 2)
        {
	for (; pMbuf != NULL; pMbuf = pMbuf->m_next)
	    {
	    adjLen = pMbuf->m_len;
	    adjBuf = mtod(pMbuf, char *);
	    if (odd > 0)
		{
		--pBuf;
		*((UINT16 *) temp) = *((UINT16 *) pBuf);
		temp [1] = *adjBuf++;
		--adjLen;
		*((UINT16 *) pBuf) = *((UINT16 *) temp);
		pBuf += 2;
		}
	    bcopyWords (adjBuf, pBuf, (adjLen + 1) >> 1);
	    pBuf += adjLen;
            odd = adjLen & 0x1;
	    }
        }

    else if (width == 4)
        {
	for (; pMbuf != NULL; pMbuf = pMbuf->m_next)
	    {
	    adjLen = pMbuf->m_len;
	    adjBuf = mtod(pMbuf, char *);
	    if (odd > 0)
		{
		pBuf -= odd;
		*((UINT32 *) temp) = *((UINT32 *) pBuf);
		for (ix = odd; ix < 4; ix++, adjBuf++, --adjLen)
		    temp [ix] = *adjBuf;
		*((UINT32 *) pBuf) = *((UINT32 *) temp);
		pBuf += 4;
		}
	    bcopyLongs (adjBuf, pBuf, (adjLen + 3) >> 2);
	    pBuf += adjLen;
            odd = adjLen & 0x3;
	    }
        }

    else
        {
        panic ("copyFromMbufs");
        }

    m_freem (pMbufHead); 			/* free the mbuf chain */
    
    return ((int) pBuf - (int) pIoBuf);		/* return length copied */
    }

/*******************************************************************************
*
* build_cluster - encapsulate a passed buffer into a cluster mbuf.
*
* This routine is used to surround the passed buffer <buf0> with a
* mbuf header and make it a pseudo cluster.  It is used primarily by the
* network interface drivers to avoid copying large chunk of data incoming
* from the network.  Network interface device drivers typically call this
* routine with a pointer to the memory location where the IP data reside.
* This buffer pointer is used as a cluster buffer pointer and inserted
* into mbuf along with other relevant pieces information such as the
* index into the reference count array, pointer to the reference count
* array and the driver level free routine.
*
* RETURNS: NULL if unsuccessful, pointer to <struct mbuf> if successful.
*
* SEE ALSO: do_protocol_with_type(), uipc_mbuf.c
*
* NOMANUAL
*/

struct mbuf *build_cluster (buf0, totlen, ifp, ctype, pRefCnt,
			    freeRtn, arg1, arg2, arg3)
    u_char 		*buf0;		/* the buffer containing data */
    int			totlen;		/* len of the buffer */
    struct ifnet	*ifp;		/* network interface device pointer */
    u_char		ctype;		/* type of this cluster being built */
    u_char		*pRefCnt;	/* type-specific ref count array */
    FUNCPTR		freeRtn;	/* driver level free callback routine */
    int			arg1;		/* first argument to freeRtn() */
    int			arg2;		/* second argument to freeRtn() */
    int			arg3;		/* third argument to freeRtn() */

    {
    FAST struct mbuf    *m;
    CL_BLK_ID		pClBlk;

    /* get an mbuf header, header will have ifp info in the packet header */

    m = mBlkGet(_pNetDpool, M_DONTWAIT, MT_DATA);

    if (m == NULL)			/* out of mbufs! */
	return (NULL);

    pClBlk = clBlkGet (_pNetDpool, M_DONTWAIT); 
    
    if (pClBlk == NULL)			/* out of cl Blks */
        {
        m_free (m); 
	return (NULL);
        }

    /* intialize the header */

    m->m_pkthdr.rcvif = ifp;
    m->m_pkthdr.len = totlen;

    m->m_data		= (caddr_t) buf0; 
    m->m_len		= m->m_pkthdr.len;
    m->m_flags		= (M_EXT | M_PKTHDR) ;
    m->pClBlk		= pClBlk;
    m->m_extBuf		= (caddr_t) buf0;
    m->m_extSize	= m->m_pkthdr.len;
    m->m_extFreeRtn	= freeRtn;
    m->m_extArg1	= arg1;
    m->m_extArg2	= arg2;
    m->m_extArg3	= arg3;
    m->m_extRefCnt	= 1;

    return (m);
    }

/*******************************************************************************
*
* do_protocol_with_type - demultiplex incoming packet to the protocol handlers
*
* Demultiplexes the incoming packet to the associated protocol handler.  This
* version supports IP, ARP, and a generic mechanism via an external lookup
* table.
*
* This routine differs from do_protocol() in that the link-level frame type
* is passed directly, instead of passing a pointer to an Ethernet header,
* respectively.
*
* This routine must not be called interrupt level.
*
* NOMANUAL
*/

void do_protocol_with_type
    (
    u_short        	type,		/* the frame type field */
    struct mbuf *	pMbuf,		/* the frame data, in mbuf chain */
    struct arpcom *	pArpcom,	/* the source interface information */
    int			len		/* length of the mbuf data */
    )
    {
    int s;
    FAST struct ifqueue *	inq;
    FUNCPTR 	pInputFunc; 	/* message handler */

    switch (type)
        {
	case ETHERTYPE_IP:
            ipintr(pMbuf);
	    return;
	case ETHERTYPE_ARP:
            inq = &arpintrq;
            pInputFunc = (FUNCPTR) arpintr;
            break;
        default:
            {
	    /* No protocol. Drop packet */
	    
	    ((struct ifnet *) pArpcom)->if_noproto++;
	    m_freem (pMbuf);	/* free the mbuf chain */
	    return;
            }
        }

    s = splnet ();

    if (IF_QFULL(inq))
        {
        IF_DROP (inq);
        m_freem (pMbuf);		/* free the mbuf chain */
	splx (s);
	}
    else
        {
        IF_ENQUEUE (inq, pMbuf);	/* put the mbuf chain in the packet queue */
	splx (s);

        /* Schedule a new job to finish the handling of the IP or ARP data. */

#ifdef VIRTUAL_STACK
        netJobAdd (pInputFunc, pArpcom->ac_if.vsNum, 0, 0, 0, 0);
#else
	arpintr();
#endif /* VIRTUAL_STACK */
        }
    }

/*******************************************************************************
*
* set_if_addr - handle SIOCSIFADDR ioctl
*
* Sets a protocol address associated with this network interface.
* This function is called by various ioctl handlers in each of the network
* interface drivers.
*
* RETURNS: 0 or appropriate errno
*
* NOMANUAL
*/

int set_if_addr (ifp, data, enaddr)
    struct ifnet 	*ifp;  	/* interface */
    char 		*data;  /* usually "struct ifaddr *" */
    u_char 		*enaddr;/* ethernet address */

    {
    int 		error = 0;
    FAST struct ifaddr *ifa = (struct ifaddr *) data;

    ifp->if_flags |= IFF_UP;

    if ((error = (*ifp->if_init) (ifp->if_unit)) != 0)
        return (error);

    switch (ifa->ifa_addr->sa_family)
        {
#ifdef  INET
        case AF_INET:
            ((struct arpcom *) ifp)->ac_ipaddr = IA_SIN (ifa)->sin_addr;
            arpwhohas ((struct arpcom *) ifp, &IA_SIN (ifa)->sin_addr);
            break;
#endif	/* INET */
	default:
	    logMsg ("eh: set_if_addr unknown family 0x%x\n",
			ifa->ifa_addr->sa_family,0,0,0,0,0);
	    break;
        }

    return (0);
    }

/*******************************************************************************
*
* ether_attach - attach a new network interface to the network layer.
*
* Fill in the ifnet structure and pass it off to if_attach () routine
* which will attach the network interface described in that structure
* to the network layer software for future use in routing, packet reception,
* and transmisison.
*
* NOMANUAL
*/

STATUS ether_attach (ifp, unit, name, initRtn, ioctlRtn, outputRtn, resetRtn)
    struct ifnet 	*ifp;		/* network interface data structure */
    int 		unit;		/* unit number */
    char 		*name;		/* name of the interface */
    FUNCPTR 		initRtn;	/* initialization routine */
    FUNCPTR 		ioctlRtn;	/* ioctl handler */
    FUNCPTR 		outputRtn;	/* output routine */
    FUNCPTR 		resetRtn;	/* reset routine */

    {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_VERBOSE event */
    WV_NET_MARKER_1 (NET_AUX_EVENT, WV_NET_VERBOSE, 14, 17,
                     WV_NETEVENT_ETHERATTACH_START, ifp)
#endif  /* INCLUDE_WVNET */
#endif

    bzero ((char *) ifp, sizeof (*ifp));

    ifp->if_unit   = unit;
    ifp->if_name   = name;
    ifp->if_mtu    = ETHERMTU;
    ifp->if_init   = initRtn;
    ifp->if_ioctl  = ioctlRtn;
    ifp->if_output = outputRtn;
    ifp->if_reset  = resetRtn;
    ifp->if_flags  = IFF_BROADCAST;

    return (if_attach (ifp));
    }

/*******************************************************************************
*
* netTypeAdd - add a network type to list of network types.
*
* RETURNS: OK if successful, otherwise ERROR.
*
* NOMANUAL
*/

STATUS netTypeAdd (etherType, inputRtn)
    int             	etherType;	/* ethernet type */
    FUNCPTR             inputRtn;	/* input routine */

    {
    NET_TYPE *          pType;

    if ((pType = (NET_TYPE *) KHEAP_ALLOC(sizeof(NET_TYPE))) == NULL)
        {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_EMERGENCY event */
        WV_NET_MARKER_1 (NET_AUX_EVENT, WV_NET_EMERGENCY, 7, 2,
                         WV_NETEVENT_TYPEADD_FAIL, etherType)
#endif  /* INCLUDE_WVNET */
#endif

        return (ERROR);
        }
    bzero((char *)pType, sizeof (NET_TYPE));

    pType->etherType    = etherType;
    pType->inputRtn     = inputRtn;
    lstAdd (&netTypeList, &pType->node);
    return (OK);
    }

/*******************************************************************************
*
* netTypeDelete - delete a network type from the list of network types.
*
* RETURNS: OK if successful, otherwise ERROR.
*
* NOMANUAL
*/

STATUS netTypeDelete (etherType)
    int		etherType;		/* ethernet type */

    {
    NET_TYPE *	pType;

    for (pType = (NET_TYPE *) lstFirst (&netTypeList);
         pType != NULL; pType = (NET_TYPE *) lstNext (&pType->node))
	{
        if (etherType == pType->etherType)
            {
            lstDelete (&netTypeList, &pType->node);
	    KHEAP_FREE((caddr_t)pType);
            return (OK);
            }
	}
    return (ERROR);
    }

/*******************************************************************************
*
* netTypeInit - initialize list of network types.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void netTypeInit ()
    {
    lstInit (&netTypeList);
    }
