/* igmp.c - internet group management protocol routines */

/* Copyright 1984 - 2002 Wind River Systems, Inc. */

/*
 * Copyright (c) 1988 Stephen Deering.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Stephen Deering of Stanford University.
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
 *	@(#)igmp.c	8.2 (Berkeley) 5/3/95
 * $FreeBSD: src/sys/netinet/igmp.c,v 1.28 1999/08/28 00:49:14 peter Exp $
 * Internet Group Management Protocol (IGMP) routines.
 *
 * Written by Steve Deering, Stanford, May 1988.
 * Modified by Rosen Sharma, Stanford, Aug 1994.
 * Modified by Bill Fenner, Xerox PARC, Feb 1995.
 * Modified to fully comply to IGMPv2 by Bill Fenner, Oct 1995.
 *
 * MULTICAST Revision: 3.5.1.4
 */
/*
modification history
--------------------
01i,21jun02,rae  Removed unnecessary #include
01h,04dec01,rae  cleanup
01g,05nov01,vvv  fixed compilation warning
01f,12oct01,rae  merge from truestack, upgrade to V2
01e,05oct97,vin  changes for multicast hashing.
01d,08mar97,vin  added mCastRouteFwdHook to access mcast routing code.
01c,31jan97,vin  changed declaration according to prototype decl in protosw.h
01b,11nov96,vin  added cluster support, changed m_gethdr mHdlClGet.
01a,03mar96,vin  created from BSD4.4 stuff.
*/

/* Internet Group Management Protocol (IGMP) routines. */

#include "vxWorks.h"
#include "errno.h"
#include "stdio.h"
#include "net/mbuf.h"
#include "sys/socket.h"
#include "net/protosw.h"

#include "net/if.h"
#include "net/route.h"

#include "netinet/in.h"
#include "netinet/in_var.h"
#include "netinet/in_systm.h"
#include "netinet/ip.h"
#include "netinet/ip_var.h"
#include "netinet/igmp.h"
#include "netinet/igmp_var.h"
#include "net/systm.h"
/* #include "ppp/random.h" */   /*  XXX -- illegal cross-product reference */

#ifdef VIRTUAL_STACK
#include "netinet/vsLib.h"
#include "netinet/vsIgmp.h"
#include "netinet/vsMcast.h"
#endif /* VIRTUAL_STACK */



#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET
#include "wvNetLib.h"
#endif /* INCLUDE_WVNET */
#endif

/* externs */

extern VOIDFUNCPTR _igmpJoinGrpHook; 
extern VOIDFUNCPTR _igmpLeaveGrpHook;
extern int         random ();
 
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET
    /* Set common fields of event identifiers for this module. */

LOCAL UCHAR wvNetModuleId = WV_NET_IGMP_MODULE;   /* Value for igmp.c */
LOCAL UCHAR wvNetLocalFilter = WV_NET_NONE;     /* Available event filter */

LOCAL ULONG wvNetEventId;       /* Event identifier: see wvNetLib.h */
#endif  /* INCLUDE_WVNET */
#endif


#ifndef VIRTUAL_STACK
extern struct ifnet loif[];

extern FUNCPTR _mCastRouteFwdHook;	/* WRS mcast forward command hook */


/* globals */

struct igmpstat igmpstat;
static int igmp_timers_are_running = 0;
static struct router_info *Head;
#endif VIRTUAL_STACK
VOIDFUNCPTR _igmpJoinAlertHook; 
VOIDFUNCPTR _igmpLeaveAlertHook; 
VOIDFUNCPTR _igmpQuerierTimeUpdateHook;
VOIDFUNCPTR _igmpMessageHook;

static u_long igmp_all_hosts_group;
static u_long igmp_all_rtrs_group;
static struct mbuf *router_alert;

/* forward declarations */
static void igmp_joingroup (struct in_multi *inm);
static void igmp_leavegroup (struct in_multi *inm);

static struct router_info *
		find_rti(struct ifnet *ifp);





static void igmp_sendpkt (struct in_multi *, int, unsigned long);

void
igmp_init()
{
	struct ipoption *ra;

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_VERBOSE event */
            WV_NET_MARKER_0 (NET_AUX_EVENT, WV_NET_VERBOSE, 15, 8,
                            WV_NETEVENT_IGMPINIT_START)
#endif  /* INCLUDE_WVNET */
#endif

	/*
	 * To avoid byte-swapping the same value over and over again.
	 */
	igmp_all_hosts_group = htonl(INADDR_ALLHOSTS_GROUP);
	igmp_all_rtrs_group = htonl(INADDR_ALLRTRS_GROUP);
	_igmpJoinGrpHook = igmp_joingroup; 
	_igmpLeaveGrpHook = igmp_leavegroup;
	igmp_timers_are_running = 0;

	/*
	 * Construct a Router Alert option to use in outgoing packets
	 */
	router_alert = mBufClGet(M_DONTWAIT, MT_DATA, CL_SIZE_128, TRUE);
	ra = mtod(router_alert, struct ipoption *);
	ra->ipopt_dst.s_addr = 0;
	ra->ipopt_list[0] = IPOPT_RA;	/* Router Alert Option */
	ra->ipopt_list[1] = 0x04;	/* 4 bytes long */
	ra->ipopt_list[2] = 0x00;
	ra->ipopt_list[3] = 0x00;
	router_alert->m_len = sizeof(ra->ipopt_dst) + ra->ipopt_list[1];
        Head = (struct router_info *) 0;
}

static struct router_info *
find_rti(ifp)
	struct ifnet *ifp;
{
        register struct router_info *rti = Head;

#ifdef IGMP_DEBUG
	printf("[igmp.c, _find_rti] --> entering \n");
#endif
        while (rti) {
                if (rti->rti_ifp == ifp) {
#ifdef IGMP_DEBUG
			printf("[igmp.c, _find_rti] --> found old entry \n");
#endif
                        return rti;
                }
                rti = rti->rti_next;
        }
	if((rti = malloc(sizeof(struct router_info))) == NULL)
	  return NULL;
    /*	MALLOC(rti, struct router_info *, sizeof *rti, M_IGMP, M_NOWAIT); */
        rti->rti_ifp = ifp;
        rti->rti_type = IGMP_V2_ROUTER;
        rti->rti_time = 0;
        rti->rti_next = Head;
        Head = rti;
#ifdef IGMP_DEBUG
	printf("[igmp.c, _find_rti] --> created an entry \n");
#endif
        return rti;
}

void
igmp_input(m, iphlen)
	register struct mbuf *m;
	register int iphlen;
{
	register struct igmp *igmp;
	register struct ip *ip;
	register int igmplen;
	register struct ifnet *ifp = m->m_pkthdr.rcvif;
	register int minlen;
	register struct in_multi *inm;
	register struct in_ifaddr *ia;
	struct router_info *rti;
	
	int timer; /** timer value in the igmp query header **/

	++igmpstat.igps_rcv_total;

	ip = mtod(m, struct ip *);
	igmplen = ip->ip_len;

	/*
	 * Validate lengths
	 */
	if (igmplen < IGMP_MINLEN) {
		++igmpstat.igps_rcv_tooshort;
		m_freem(m);
		return;
	}
	minlen = iphlen + IGMP_MINLEN;
	if ((m->m_flags & M_EXT || m->m_len < minlen) &&
	    (m = m_pullup(m, minlen)) == 0) {
		++igmpstat.igps_rcv_tooshort;
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_CRITICAL event */
                WV_NET_EVENT_0 (NET_CORE_EVENT, WV_NET_CRITICAL, 6, 2,  
                                WV_NETEVENT_IGMPIN_SHORTMSG, WV_NET_RECV)
#endif  /* INCLUDE_WVNET */
#endif

		return;
	}

	/*
	 * Validate checksum
	 */
	m->m_data += iphlen;
	m->m_len -= iphlen;
	igmp = mtod(m, struct igmp *);
	if (in_cksum(m, igmplen)) {
		++igmpstat.igps_rcv_badsum;

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_CRITICAL event */
                WV_NET_EVENT_0 (NET_CORE_EVENT, WV_NET_CRITICAL, 7, 3,  
                                 WV_NETEVENT_IGMPIN_BADSUM, WV_NET_RECV)
#endif  /* INCLUDE_WVNET */
#endif

		m_freem(m);
		return;
	}
	m->m_data -= iphlen;
	m->m_len += iphlen;

	ip = mtod(m, struct ip *);
	timer = igmp->igmp_code * PR_FASTHZ / IGMP_TIMER_SCALE;
	if (timer == 0)
		timer = 1;
	rti = find_rti(ifp);

	/*
	 * In the IGMPv2 specification, there are 3 states and a flag.
	 *
	 * In Non-Member state, we simply don't have a membership record.
	 * In Delaying Member state, our timer is running (inm->inm_timer)
	 * In Idle Member state, our timer is not running (inm->inm_timer==0)
	 *
	 * The flag is inm->inm_state, it is set to IGMP_OTHERMEMBER if
	 * we have heard a report from another member, or IGMP_IREPORTEDLAST
	 * if I sent the last report.
	 */
	switch (igmp->igmp_type) {

	case IGMP_MEMBERSHIP_QUERY:
		++igmpstat.igps_rcv_queries;

		if (ifp->if_flags & IFF_LOOPBACK)
			break;

		if (igmp->igmp_code == 0) {
			/*
			 * Old router.  Remember that the querier on this
			 * interface is old, and set the timer to the
			 * value in RFC 1112.
			 */

			rti->rti_type = IGMP_V1_ROUTER;
			rti->rti_time = 0;
                        if(_igmpQuerierTimeUpdateHook != NULL)
                            _igmpQuerierTimeUpdateHook(rti->rti_ifp->if_index,
                                                       rti->rti_time);

			timer = IGMP_MAX_HOST_REPORT_DELAY * PR_FASTHZ;

			if (ip->ip_dst.s_addr != igmp_all_hosts_group ||
			    igmp->igmp_group.s_addr != 0) {
				++igmpstat.igps_rcv_badqueries;
				m_freem(m);
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_WARNING event */
            WV_NET_DSTADDRIN_EVENT_3 (NET_CORE_EVENT, WV_NET_WARNING, 5, 4,  
                                      ip->ip_dst.s_addr,
                                      WV_NETEVENT_IGMPIN_BADADDR, WV_NET_RECV,
                                      igmp->igmp_type, ip->ip_dst.s_addr, 0)
#endif  /* INCLUDE_WVNET */
#endif

				return;
			}
		} else {
			/*
			 * New router.  Simply do the new validity check.
			 */
			
			if (igmp->igmp_group.s_addr != 0 &&
			    !IN_MULTICAST(ntohl(igmp->igmp_group.s_addr))) {
				++igmpstat.igps_rcv_badqueries;
				m_freem(m);
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_WARNING event */
            WV_NET_DSTADDRIN_EVENT_3 (NET_CORE_EVENT, WV_NET_WARNING, 5, 4,  
                                      ip->ip_dst.s_addr,
                                      WV_NETEVENT_IGMPIN_BADADDR, WV_NET_RECV,
                                      igmp->igmp_type, ip->ip_dst.s_addr, 0)
#endif  /* INCLUDE_WVNET */
#endif

				return;
			}
		}

		/*
		 * - Start the timers in all of our membership records
		 *   that the query applies to for the interface on
		 *   which the query arrived excl. those that belong
		 *   to the "all-hosts" group (224.0.0.1).
		 * - Restart any timer that is already running but has
		 *   a value longer than the requested timeout.
		 * - Use the value specified in the query message as
		 *   the maximum timeout.
		 */

                {
                IN_MULTI_HEAD * inMultiHead;
                int		ix;

                for (ix = 0; ix <= mCastHashInfo.hashMask; ix++)
                    {
                    inMultiHead = &mCastHashInfo.hashBase [ix];
                    if (inMultiHead != NULL)
                        {
                        for (inm = inMultiHead->lh_first;
                             inm != NULL;
                             inm = inm->inm_hash.le_next)
                            {
                            if (inm->inm_ifp == ifp &&
                                inm->inm_addr.s_addr != igmp_all_hosts_group &&
                                (igmp->igmp_group.s_addr == 0 ||
                                 igmp->igmp_group.s_addr == inm->inm_addr.s_addr))
                                {
                                if (inm->inm_timer == 0 ||
                                    inm->inm_timer > timer)
                                    {
                                    inm->inm_timer =
                                        IGMP_RANDOM_DELAY(timer);
                                    igmp_timers_are_running = 1;
                                    }
                                }
                            
                            }
                        }
                    }
                }
        
		break;

	case IGMP_V1_MEMBERSHIP_REPORT:
	case IGMP_V2_MEMBERSHIP_REPORT:
		/*
		 * For fast leave to work, we have to know that we are the
		 * last person to send a report for this group.  Reports
		 * can potentially get looped back if we are a multicast
		 * router, so discard reports sourced by me.
		 */
		IFP_TO_IA(ifp, ia);
		if (ia && ip->ip_src.s_addr == IA_SIN(ia)->sin_addr.s_addr)
			break;

		++igmpstat.igps_rcv_reports;

		if (ifp->if_flags & IFF_LOOPBACK)
			break;

		if (!IN_MULTICAST(ntohl(igmp->igmp_group.s_addr))) {
			++igmpstat.igps_rcv_badreports;
			m_freem(m);
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_WARNING event */
            WV_NET_DSTADDRIN_EVENT_3 (NET_CORE_EVENT, WV_NET_WARNING, 5, 4,  
                                      ip->ip_dst.s_addr,
                                      WV_NETEVENT_IGMPIN_BADADDR, WV_NET_RECV,
                                      igmp->igmp_type, ip->ip_dst.s_addr,
                                      igmp->igmp_group.s_addr)
#endif  /* INCLUDE_WVNET */
#endif
			return;
		}

		/*
		 * KLUDGE: if the IP source address of the report has an
		 * unspecified (i.e., zero) subnet number, as is allowed for
		 * a booting host, replace it with the correct subnet number
		 * so that a process-level multicast routing demon can
		 * determine which subnet it arrived from.  This is necessary
		 * to compensate for the lack of any way for a process to
		 * determine the arrival interface of an incoming packet.
		 */
		if ((ntohl(ip->ip_src.s_addr) & IN_CLASSA_NET) == 0)
			if (ia) ip->ip_src.s_addr = htonl(ia->ia_subnet);

		/*
		 * If we belong to the group being reported, stop
		 * our timer for that group.
		 */
		IN_LOOKUP_MULTI(igmp->igmp_group, ifp, inm);

		if (inm != NULL) {
			inm->inm_timer = 0;
			++igmpstat.igps_rcv_ourreports;

			inm->inm_state = IGMP_OTHERMEMBER;
		}

		break;
	}

	/*
	 * Pass all valid IGMP packets up to any process(es) listening
	 * on a raw IGMP socket.
	 */

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_NOTICE event */
        WV_NET_ADDRIN_EVENT_2 (NET_CORE_EVENT, WV_NET_NOTICE, 5, 5,
                               ip->ip_src.s_addr, ip->ip_dst.s_addr,
                            WV_NETEVENT_IGMPIN_FINISH, WV_NET_RECV,
                               ip->ip_src.s_addr, ip->ip_dst.s_addr)
#endif  /* INCLUDE_WVNET */
#endif

        if (_igmpMessageHook) /* if IGMP routing is on */
            {
            _igmpMessageHook (m->m_pkthdr.rcvif->if_index,
                         &(ip->ip_src), igmp);
            m_freem(m);
            }
        else
            rip_input(m);
}


void
igmp_joingroup(inm)
struct in_multi *inm;
{
int s = splnet();
    
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_VERBOSE event */
    WV_NET_MARKER_1 (NET_AUX_EVENT, WV_NET_VERBOSE, 16, 9,
                     WV_NETEVENT_IGMPJOIN_START, inm->inm_addr.s_addr)
#endif  /* INCLUDE_WVNET */
#endif
    
        if (_igmpJoinAlertHook != NULL)
            _igmpJoinAlertHook(inm->inm_ifp->if_index);
    if (inm->inm_addr.s_addr == igmp_all_hosts_group
        || inm->inm_ifp->if_flags & IFF_LOOPBACK) {
    inm->inm_timer = 0;
    inm->inm_state = IGMP_OTHERMEMBER;
    } else {
    inm->inm_rti = find_rti(inm->inm_ifp);
    igmp_sendpkt(inm, inm->inm_rti->rti_type, 0);
		inm->inm_timer = IGMP_RANDOM_DELAY(IGMP_MAX_HOST_REPORT_DELAY *
                                                             PR_FASTHZ);
		inm->inm_state = IGMP_IREPORTEDLAST;
		igmp_timers_are_running = 1;
	}
    splx(s);
    }

void
igmp_leavegroup(inm)
struct in_multi *inm;
{
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_VERBOSE event */
         WV_NET_MARKER_1 (NET_AUX_EVENT, WV_NET_VERBOSE, 17, 10,
                             WV_NETEVENT_IGMPLEAVE_START, inm->inm_addr.s_addr)
#endif  /* INCLUDE_WVNET */
#endif

	 if (_igmpLeaveAlertHook != NULL)
            _igmpLeaveAlertHook(inm->inm_ifp->if_index);
         if (inm->inm_state == IGMP_IREPORTEDLAST &&
	    inm->inm_addr.s_addr != igmp_all_hosts_group &&
	    !(inm->inm_ifp->if_flags & IFF_LOOPBACK) &&
	    inm->inm_rti->rti_type != IGMP_V1_ROUTER)
		igmp_sendpkt(inm, IGMP_V2_LEAVE_GROUP, igmp_all_rtrs_group);
}

void
igmp_fasttimo()
{
	register struct in_multi *inm;
	int s;	

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_INFO event */
        WV_NET_MARKER_0 (NET_AUX_EVENT, WV_NET_INFO, 4, 7,
                         WV_NETEVENT_IGMPFASTTIMER_START)
#endif  /* INCLUDE_WVNET */
#endif

	/*
	 * Quick check to see if any work needs to be done, in order
	 * to minimize the overhead of fasttimo processing.
	 */

	if (!igmp_timers_are_running)
		return;

	s = splnet();
	igmp_timers_are_running = 0;

        {
        IN_MULTI_HEAD * inMultiHead;
        int		ix;

        for (ix = 0; ix <= mCastHashInfo.hashMask; ix++)
            {
            inMultiHead = &mCastHashInfo.hashBase [ix];
            if (inMultiHead != NULL)
                {
                for (inm = inMultiHead->lh_first;
                     inm != NULL;
                     inm = inm->inm_hash.le_next)
                    {
                    if (inm->inm_timer == 0)
                        {
                        /* do nothing */
                        }
                    else if (--inm->inm_timer == 0)
                        {
			igmp_sendpkt(inm, inm->inm_rti->rti_type, 0);
			inm->inm_state = IGMP_IREPORTEDLAST;
			}
                    else
                        {
                        igmp_timers_are_running = 1;
                        }
                    }
                }
            }
    }
	splx(s);
}

void
igmp_slowtimo()
{
	int s = splnet();
	register struct router_info *rti =  Head;

#ifdef IGMP_DEBUG_XXX
	printf("[igmp.c,_slowtimo] -- > entering \n");
#endif
	while (rti) {
	    if (rti->rti_type == IGMP_V1_ROUTER) {
		rti->rti_time++;
                if(_igmpQuerierTimeUpdateHook != NULL)
                            _igmpQuerierTimeUpdateHook(rti->rti_ifp->if_index, rti->rti_time);
		if (rti->rti_time >= IGMP_AGE_THRESHOLD) {
			rti->rti_type = IGMP_V2_ROUTER;
		}
	    }
	    rti = rti->rti_next;
	}
#ifdef IGMP_DEBUG_XXX
	printf("[igmp.c,_slowtimo] -- > exiting \n");
#endif
	splx(s);
}

static struct route igmprt;

static void
igmp_sendpkt(inm, type, addr)
	struct in_multi *inm;
	int type;
	unsigned long addr;
{
        struct mbuf *m;
        struct igmp *igmp;
        struct ip *ip;
        struct ip_moptions imo;

        m = mHdrClGet(M_DONTWAIT, MT_HEADER, CL_SIZE_128, TRUE);
	if (m == NULL)
            {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_EMERGENCY event */
            WV_NET_MARKER_0 (NET_AUX_EVENT, WV_NET_EMERGENCY, 8, 1,
                            WV_NETEVENT_IGMPSENDREP_NOBUFS)
#endif  /* INCLUDE_WVNET */
#endif

            return;
            }

	m->m_pkthdr.rcvif = loif;
	m->m_pkthdr.len = sizeof(struct ip) + IGMP_MINLEN;
	MH_ALIGN(m, IGMP_MINLEN + sizeof(struct ip));
	m->m_data += sizeof(struct ip);
        m->m_len = IGMP_MINLEN;
        igmp = mtod(m, struct igmp *);
        igmp->igmp_type   = type;
        igmp->igmp_code   = 0;
        igmp->igmp_group  = inm->inm_addr;
        igmp->igmp_cksum  = 0;
        igmp->igmp_cksum  = in_cksum(m, IGMP_MINLEN);

        m->m_data -= sizeof(struct ip);
        m->m_len += sizeof(struct ip);
        ip = mtod(m, struct ip *);
        ip->ip_tos        = 0;
        ip->ip_len        = sizeof(struct ip) + IGMP_MINLEN;
        ip->ip_off        = 0;
        ip->ip_p          = IPPROTO_IGMP;
        ip->ip_src.s_addr = INADDR_ANY;
        ip->ip_dst.s_addr = addr ? addr : igmp->igmp_group.s_addr;

        imo.imo_multicast_ifp  = inm->inm_ifp;
        imo.imo_multicast_ttl  = 1;
	/*	imo.imo_multicast_vif  = -1; */
        /*
         * Request loopback of the report if we are acting as a multicast
         * router, so that the process-level routing demon can hear it.
         */
       imo.imo_multicast_loop = (_mCastRouteFwdHook != NULL);

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_NOTICE event */
        WV_NET_DSTADDROUT_EVENT_1 (NET_CORE_EVENT, WV_NET_NOTICE, 6, 6,
                                   ip->ip_dst.s_addr,
                                   WV_NETEVENT_IGMPSENDREP_FINISH, WV_NET_SEND,
                                   ip->ip_dst.s_addr)
#endif  /* INCLUDE_WVNET */
#endif

	/*
	 * XXX
	 * Do we have to worry about reentrancy here?  Don't think so.
	 */
        ip_output(m, router_alert, &igmprt, 0, &imo);

        ++igmpstat.igps_snd_reports;
}
