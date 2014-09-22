/* rtsock.c - routines for routing sockets */

/* Copyright 1990 - 2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
 * Copyright (c) 1988, 1991, 1993
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
 *	@(#)rtsock.c	8.6 (Berkeley) 2/11/95
 */

/*
modification history
--------------------
01g,12oct01,rae  merge from truestack ver 01u, base o1f (SPR #70185 ROUTER_STACK)
01f,26aug98,n_s  added return val check for MALLOC in route_usrreq. spr #22238.
01e,04dec97,vin  moved back m_copyback and m_copydata from uipc_mbuf.c as
		 these are only used in rt_sock. Ifdefed m_copyback and
                 m_copydata in uipc_mbuf.c these need some rework so that
                 they can be tied up efficiently with the new buffering scheme.
01d,01jul97,vin  added rtSockInit, removed route_init from routedomain, it is
		 called from netLibInit. Added route message hooks for
                 scalability.
01c,05dec96,vin  changed malloc(..) to MALLOC(..) to use only network buffers,
		 changed free(..) to FREE(..).
01b,22nov96,vin  added cluster support replaced m_get(..) with mBufClGet(..).
		 m_gethdr(..) with mHdrClGet(..), moved m_copyback(..) to 
		 uipc_mbuf.c
01a,03mar96,vin  created from BSD4.4lite2.
*/

#include "net/systm.h"
#include "net/mbuf.h"
#include "sys/socket.h"
#include "net/socketvar.h"
#include "net/domain.h"
#include "net/protosw.h"
#include "net/unixLib.h"

#include "net/if.h"
#include "net/route.h"
#include "net/raw_cb.h"
#include "netinet/in.h"

#include "routeEnhLib.h"

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET
#include "wvNetLib.h"
#endif
#endif

#ifdef ROUTER_STACK
#define SOCKADDR_IN(s) (((struct sockaddr_in*)(s))->sin_addr.s_addr)
#endif

struct walkarg {
	int	w_op, w_arg, w_given, w_needed, w_tmemsize;
	caddr_t	w_where, w_tmem;
};

#ifndef VIRTUAL_STACK
    /*
     * This global counts the number of routing socket listeners
     * for a particular family. It is not used outside this module.
     */

struct  route_cb route_cb;
#endif

static struct	sockaddr route_dst = { 2, PF_ROUTE, };
static struct	sockaddr route_src = { 2, PF_ROUTE, };

#ifndef VIRTUAL_STACK
    /*
     * The following protocol structure is the only global in this module
     * which differs between virtual stacks. It limits the routing messages
     * based on the associated address family, if a specific protocol value
     * is chosen during the socket creation.
     *
     * The source address variable above is read, but never altered. It
     * is the preassigned remote address for all routing sockets, which
     * disables the use of any alternatives when writing to a socket and
     * limits the delivery of any routing message to members of the routing
     * socket family.
     *
     * Since the bind operation is not supported, a local address can't be
     * assigned to a routing socket. As a result, the preceding destination
     * address variable, although passed to the routine which sends routing
     * messages to any listening sockets, is never actually used.
     */ 
     
static struct	sockproto route_proto = { PF_ROUTE, };
#endif /* VIRTUAL_STACK */

#ifdef ROUTER_STACK
IMPORT void routeIfStatusUpdate (struct ifnet *);
IMPORT void routeIfAddressUpdate (int, struct ifaddr *, struct rtentry *);
IMPORT void routeMissUpdate (int, struct rt_addrinfo *, int, int);
#endif /* ROUTER_STACK */

IMPORT int rt_fixdelete (struct radix_node *, void *);
struct mbuf *
		rt_msg1 (int, struct rt_addrinfo *);
static int	rt_msg2 (int, struct rt_addrinfo *, caddr_t, 
			 struct walkarg *);
static void	rt_xaddrs (caddr_t, caddr_t, struct rt_addrinfo *);

static int	route_usrreq (struct socket *, int, struct mbuf *,
                              struct mbuf *, struct mbuf *);
void	rt_ifmsg (struct ifnet *);
void	rt_missmsg (int, struct rt_addrinfo *, int, int);
void	rt_newaddrmsg (int, struct ifaddr *, int, struct rtentry *);
static int	route_output (struct mbuf *, struct socket *);
static void	rt_setmetrics (u_long, struct rt_metrics *,
                               struct rt_metrics *);

static void	m_copyback (struct mbuf *, int, int, caddr_t);
static void 	m_copydata(struct mbuf *, int, int, caddr_t);

#ifdef ROUTER_STACK
LOCAL int routeGatewayFetch (struct rtentry *, struct rt_msghdr *);
LOCAL ROUTE_ENTRY * routeMatch (struct rtentry *, short, struct sockaddr *);
#endif /* ROUTER_STACK */

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET
    /* Set common fields of event identifiers for this module. */

LOCAL UCHAR wvNetModuleId = WV_NET_RTSOCK_MODULE;   /* Value for rtsock.c */
LOCAL UCHAR wvNetLocalFilter = WV_NET_NONE;     /* Available event filter */

LOCAL ULONG wvNetEventId;       /* Event identifier: see wvNetLib.h */
#endif    /* INCLUDE_WVNET */
#endif

/* Sleazy use of local variables throughout file, warning!!!! */
#define dst	info.rti_info[RTAX_DST]
#define gate	info.rti_info[RTAX_GATEWAY]
#define netmask	info.rti_info[RTAX_NETMASK]
#define genmask	info.rti_info[RTAX_GENMASK]
#define ifpaddr	info.rti_info[RTAX_IFP]
#define ifaaddr	info.rti_info[RTAX_IFA]
#define brdaddr	info.rti_info[RTAX_BRD]


/*******************************************************************************
* rtSockInit - intialize route sockets
*
* This function initializes the route msg hooks used in various places in the
* system. The hooks are provided to scale out route sockets. The same hooks
* also generate the internal callback messages for the router stack product.
*
* The domain_init entry point of the route socket domain calls this function.
*
* RETURNS: N/A
*
* NOMANUAL
*/

LOCAL void rtSockInit (void)
    {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_VERBOSE event */
    WV_NET_MARKER_0 (NET_AUX_EVENT, WV_NET_VERBOSE, 36, 8,
                     WV_NETEVENT_RTSOCKINIT_START)
#endif  /* INCLUDE_WVNET */
#endif

    /* Set the function hooks for receiving status changes from the stack. */

#ifdef ROUTER_STACK
    rtIfaceMsgHook = routeIfStatusUpdate;
    rtNewAddrMsgHook = routeIfAddressUpdate;
    rtMissMsgHook = routeMissUpdate;
#else
    rtIfaceMsgHook = rt_ifmsg;
    rtNewAddrMsgHook = rt_newaddrmsg;
    rtMissMsgHook = rt_missmsg;
#endif /* ROUTER_STACK */

    raw_init ();     			 /* initialize the raw control block */
    }
    
/*ARGSUSED*/
static int
route_usrreq(so, req, m, nam, control)
	register struct socket *so;
	int req;
	struct mbuf *m, *nam, *control;
{
	register int error = 0;
	register struct rawcb *rp = sotorawcb(so);
	int s;

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_INFO event */
    WV_NET_MARKER_2 (NET_AUX_EVENT, WV_NET_INFO, 21, 6,
                     WV_NETEVENT_ROUTEREQ_START, so->so_fd, req)
#endif  /* INCLUDE_WVNET */
#endif

	if (req == PRU_ATTACH) {
		MALLOC(rp, struct rawcb *, sizeof(*rp), MT_PCB, M_WAIT);
		if (rp == (struct rawcb *) NULL)
		    {
		    return (ENOBUFS);
		    }
		if ((so->so_pcb = (caddr_t)rp))
			bzero(so->so_pcb, sizeof(*rp));

	}
	if (req == PRU_DETACH && rp) {
		int af = rp->rcb_proto.sp_protocol;
		if (af == AF_INET)
			route_cb.ip_count--;
		else if (af == AF_NS)
			route_cb.ns_count--;
		else if (af == AF_ISO)
			route_cb.iso_count--;
		route_cb.any_count--;
	}
	s = splnet();
	error = raw_usrreq(so, req, m, nam, control);
	rp = sotorawcb(so);
	if (req == PRU_ATTACH && rp) {
		int af = rp->rcb_proto.sp_protocol;
		if (error) {
			FREE(rp, MT_PCB);
			splx(s);
			return (error);
		}
		if (af == AF_INET)
			route_cb.ip_count++;
		else if (af == AF_NS)
			route_cb.ns_count++;
		else if (af == AF_ISO)
			route_cb.iso_count++;
		rp->rcb_faddr = &route_src;
		route_cb.any_count++;
		soisconnected(so);
		so->so_options |= SO_USELOOPBACK;
	}
	splx(s);
	return (error);
}

/*ARGSUSED*/
static int
route_output(m, so)
	register struct mbuf *m;
	struct socket *so;
{
	register struct rt_msghdr *rtm = 0;
	register struct rtentry *rt = 0;
	struct rtentry *saved_nrt = 0;
	struct radix_node_head *rnh;
	struct rt_addrinfo info;
	int len, error = 0;
	struct ifnet *ifp = 0;
	struct ifaddr *ifa = 0;

#ifdef ROUTER_STACK
        struct sockaddr * pGateway = 0;
        short             protoId = 0;
        ROUTE_ENTRY *     pRoute = NULL;
#endif

#define senderr(e) { error = e; goto flush;}

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_NOTICE event */
    WV_NET_MARKER_1 (NET_AUX_EVENT, WV_NET_NOTICE, 5, 5, 
                     WV_NETEVENT_RTOUTPUT_START, so->so_fd)
#endif  /* INCLUDE_WVNET */
#endif

	if (m == 0 || ((m->m_len < sizeof(long)) &&
		       (m = m_pullup(m, sizeof(long))) == 0))
            {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_EMERGENCY event */
            WV_NET_EVENT_1 (NET_AUX_EVENT, WV_NET_EMERGENCY, 24, 1, 
                            WV_NETEVENT_RTOUT_BADMSG, WV_NET_SEND,
                            so->so_fd)
#endif  /* INCLUDE_WVNET */
#endif

            return (ENOBUFS);
            }

	if ((m->m_flags & M_PKTHDR) == 0)
            {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_EMERGENCY event */
            WV_NET_EVENT_1 (NET_AUX_EVENT, WV_NET_EMERGENCY, 25, 2, 
                            WV_NETEVENT_RTOUT_PANIC, WV_NET_SEND,
                            so->so_fd)
#endif  /* INCLUDE_WVNET */
#endif

            panic("route_output");
            }
	len = m->m_pkthdr.len;
	if (len < sizeof(*rtm) ||
	    len != mtod(m, struct rt_msghdr *)->rtm_msglen) {
		dst = 0;
		senderr(EINVAL);
	}
	R_Malloc(rtm, struct rt_msghdr *, len);
	if (rtm == 0) {
		dst = 0;
		senderr(ENOBUFS);
	}
	m_copydata(m, 0, len, (caddr_t)rtm);
	if (rtm->rtm_version != RTM_VERSION) {
		dst = 0;
		senderr(EPROTONOSUPPORT);
	}
	info.rti_addrs = rtm->rtm_addrs;
	rt_xaddrs((caddr_t)(rtm + 1), len + (caddr_t)rtm, &info);
	if (dst == 0)
		senderr(EINVAL);
	if (genmask) {
		struct radix_node *t;
		t = rn_addmask((caddr_t)genmask, 0, 1);
		if (t && Bcmp(genmask, t->rn_key, *(u_char *)genmask) == 0)
			genmask = (struct sockaddr *)(t->rn_key);
		else
			senderr(ENOBUFS);
	}
	switch (rtm->rtm_type) {

	case RTM_ADD:
		if (gate == 0)
			senderr(EINVAL);

                /*
                 * Create the new route using the default weight value.
                 * Report the change using direct callbacks, but do not
                 * generate a routing socket message to avoid duplicates.
                 */

                error = rtrequestAddEqui(dst, netmask, gate, rtm->rtm_flags,
					 M2_ipRouteProto_other, 0, TRUE,
                                         FALSE, (ROUTE_ENTRY **) &saved_nrt);
		if (error == 0 && saved_nrt) {
			rt_setmetrics(rtm->rtm_inits,
				&rtm->rtm_rmx, &saved_nrt->rt_rmx);

#ifdef ROUTER_STACK
                        /* Set the initial weight value, if any. */

                        if (rtm->rtm_inits & RTV_WEIGHT)
                            saved_nrt->rt_rmx.weight = rtm->rtm_rmx.weight;
#endif

			saved_nrt->rt_refcnt--;
			saved_nrt->rt_genmask = genmask;

#ifdef ROUTER_STACK
                /* Use different message type for additional routes. */

                if ( ((ROUTE_ENTRY *)saved_nrt)->primaryRouteFlag == FALSE)
                    rtm->rtm_type = RTM_ADDEXTRA;
#endif
		}
		break;

	case RTM_DELETE:
                /*
                 * Delete the matching route entry. Report the change using
                 * direct callbacks, but do not generate a routing socket
                 * message to avoid duplicates.
                 */

                error = rtrequestDelEqui(dst, netmask, gate, rtm->rtm_flags,
					 M2_ipRouteProto_other, TRUE, FALSE,
					 (ROUTE_ENTRY **) &saved_nrt);
		if (error == 0) {
			(rt = saved_nrt)->rt_refcnt++;

#ifdef ROUTER_STACK
                /* Use different message type for additional routes. */

                if ( ((ROUTE_ENTRY *)saved_nrt)->primaryRouteFlag == FALSE)
                    rtm->rtm_type = RTM_DELEXTRA;
#endif

			goto report;
		}
		break;

	case RTM_GET:
	case RTM_CHANGE:
	case RTM_LOCK:
#ifdef ROUTER_STACK
        case RTM_NEWCHANGE:
        case RTM_NEWGET:
        case RTM_GETALL:
#endif
		if ((rnh = rt_tables[dst->sa_family]) == 0) {
			senderr(EAFNOSUPPORT);
		} else if ((rt = (struct rtentry *)
				rnh->rnh_lookup(dst, netmask, rnh)))
			rt->rt_refcnt++;
		else
			senderr(ESRCH);

#ifdef ROUTER_STACK
		switch(rtm->rtm_type) {

                case RTM_GETALL:
                    /*
                     * Create a message containing information about all
                     * routes with the matching destination and netmask.
                     */

                    len = routeGatewayFetch (rt, NULL);
                    if (len > rtm->rtm_msglen)
                        {
                        struct rt_msghdr * pNewMessage;
                        R_Malloc(pNewMessage, struct rt_msghdr *, len);
	                if (pNewMessage == NULL)
                            senderr(ENOBUFS);
                        Free(rtm);
                        rtm = pNewMessage;
			}
                    routeGatewayFetch (rt, rtm);
                    break;

                case RTM_NEWCHANGE:
		case RTM_NEWGET:

                    /*
                     * If the search command specifies a gateway address, use
                     * that value and the included protocol ID (if any) to
                     * select a matching entry from any duplicate routes.
                     */

                    pGateway = gate;
                    if (gate)
                        {
                        protoId = RT_PROTO_GET (gate);
                        pRoute = routeMatch (rt, protoId, gate);
                        if (pRoute)
                            {
                            /*
                             * Matching entry found. Increase the
                             * reference count for the exact match.
                             */

                            pRoute->rtEntry.rt_refcnt++;
                            }     
                        else
                            senderr(ESRCH);
                        }
                    break;
                }
#endif

		switch(rtm->rtm_type) {

		case RTM_GET:
#ifdef ROUTER_STACK
                case RTM_NEWGET:
                        if (pGateway)
                            {
                            /* The command used the gateway address to get
                             * an exact match. Use that entry instead of
                             * the original reference to the primary route.
                             */ 

                            rtfree (rt);
                            rt = (struct rtentry *)pRoute;
                            }
#endif
		report:
			dst = rt_key(rt);
			gate = rt->rt_gateway;
			netmask = rt_mask(rt);
			genmask = rt->rt_genmask;
			if ((rtm->rtm_addrs & (RTA_IFP | RTA_IFA))) {
				if ((ifp = rt->rt_ifp)) {
					ifpaddr = ifp->if_addrlist->ifa_addr;
					ifaaddr = rt->rt_ifa->ifa_addr;
					if (ifp->if_flags & IFF_POINTOPOINT)
						brdaddr = rt->rt_ifa->ifa_dstaddr;
					else
						brdaddr = 0;
					rtm->rtm_index = ifp->if_index;
				} else {
					ifpaddr = 0;
					ifaaddr = 0;
			    }
			}
			len = rt_msg2(rtm->rtm_type, &info, (caddr_t)0,
				(struct walkarg *)0);
			if (len > rtm->rtm_msglen) {
				struct rt_msghdr *new_rtm;
				R_Malloc(new_rtm, struct rt_msghdr *, len);
				if (new_rtm == 0)
					senderr(ENOBUFS);
				Bcopy(rtm, new_rtm, rtm->rtm_msglen);
				Free(rtm); rtm = new_rtm;
			}
			(void)rt_msg2(rtm->rtm_type, &info, (caddr_t)rtm,
				(struct walkarg *)0);
			rtm->rtm_flags = rt->rt_flags;
			rtm->rtm_rmx = rt->rt_rmx;
			rtm->rtm_addrs = info.rti_addrs;
			break;

#ifdef ROUTER_STACK
		case RTM_NEWCHANGE:
                        /*
                         * Use the exact match selected with the gateway
                         * address, if available. Save the primary route
                         * in case the change operation alters the weight.
                         */

                        if (pGateway)
                            {
                            saved_nrt = rt;
                            rt = (struct rtentry *)pRoute;
                            }

                    /* Access the new gateway value for a route, if any. */

                    gate = info.rti_info[RTAX_AUTHOR];
#endif
		case RTM_CHANGE:
			if (gate && rt_setgate(rt, rt_key(rt), gate))
				senderr(ENOSPC);

#ifdef ROUTER_STACK
       /* Only remove cloned routes when altering the primary entry. */
                    if ( ((ROUTE_ENTRY *)rt)->primaryRouteFlag)
                        {
#endif /* ROUTER_STACK */
                        if (gate && (rt_key(rt)->sa_family == AF_INET))
                            {
                            /*
                             * Remove any cloned routes using the
                             * (now obsolete) gateway.
                             */

                           if ( (rt->rt_flags & RTF_CLONING) && rt_mask (rt))
                               rn_walksubtree (rnh, rt_key (rt), rt_mask (rt),
                                               rt_fixdelete, rt);
                           }

#ifdef ROUTER_STACK
    /* End of condition for cloned route removal */
                        }
#endif

			/* new gateway could require new ifaddr, ifp;
			   flags may also be different; ifp may be specified
			   by ll sockaddr when protocol address is ambiguous */
	if (ifpaddr && (ifa = ifa_ifwithnet(ifpaddr)) &&
			    (ifp = ifa->ifa_ifp))
		ifa = ifaof_ifpforaddr(ifaaddr ? ifaaddr : gate, ifp);
	else if ((ifaaddr && (ifa = ifa_ifwithaddr(ifaaddr))) ||
			 (ifa = ifa_ifwithroute(rt->rt_flags,
						rt_key(rt), gate)))
				ifp = ifa->ifa_ifp;
			if (ifa) {
				register struct ifaddr *oifa = rt->rt_ifa;
				if (oifa != ifa) {
				    if (oifa && oifa->ifa_rtrequest)
					oifa->ifa_rtrequest(RTM_DELETE,
								rt, gate);
				    IFAFREE(rt->rt_ifa);
				    rt->rt_ifa = ifa;
				    ifa->ifa_refcnt++;
				    rt->rt_ifp = ifp;
				}
			}

#ifdef ROUTER_STACK
    /*
     * Note: Setting the metrics does not change the new weight
     *       value for the selected route entry, since that
     *       alteration could replace the current route before
     *       completing all the changes required.
     */
#endif

			rt_setmetrics(rtm->rtm_inits, &rtm->rtm_rmx,
					&rt->rt_rmx);
			if (rt->rt_ifa && rt->rt_ifa->ifa_rtrequest)
			       rt->rt_ifa->ifa_rtrequest(RTM_ADD, rt, gate);
			if (genmask)
				rt->rt_genmask = genmask;
			/*
			 * Fall into
			 */
		case RTM_LOCK:
			rt->rt_rmx.rmx_locks &= ~(rtm->rtm_inits);
			rt->rt_rmx.rmx_locks |=
				(rtm->rtm_inits & rtm->rtm_rmx.rmx_locks);
			break;
		}

#ifdef ROUTER_STACK
                /*
                 * Since a weight change could replace the primary route
                 * visible to the IP forwarding process and the original
                 * routing socket messages, reorganizing the routes must
                 * wait until after all the prior changes so that any new
                 * copy of the selected route is correct.
                 */

		switch(rtm->rtm_type) {
		case RTM_CHANGE:
		case RTM_NEWCHANGE:

                    /* Do nothing if the weight did not change. */

                    if ( (rtm->rtm_inits & RTV_WEIGHT) == 0)
                        break;

                    /*
                     * Get the primary route to find the matching group
                     * for an existing entry when updating the weight.
                     */

                    if (saved_nrt)
                        {
                        /*
                         * The new change operation set a gateway to get
                         * an exact match and saved the primary route.
                         */

                        pRoute = (ROUTE_ENTRY *)saved_nrt;
                        }
                    else
                        {
                        /*
                         * The original change operation only modifies the
                         * primary route. The new change operation also
                         * alters that entry if no gateway is set.
                         */

                        pRoute = (ROUTE_ENTRY *)rt;
                        }

                    routeWeightUpdate (pRoute, protoId, (ROUTE_ENTRY *)rt, 
                                       gate, rtm->rtm_rmx.weight);

                    if (saved_nrt)
                        {
                        /*
                         * Release the reference to the primary route
                         * if a specified gateway used an exact match.
                         */

                        rtfree (saved_nrt);
                        }

                    break;
                }
#endif /* ROUTER_STACK */

		break;

	default:
		senderr(EOPNOTSUPP);
	}

flush:
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_CRITICAL event */
        if (error)
            {
            WV_NET_EVENT_2 (NET_AUX_EVENT, WV_NET_CRITICAL, 29, 4, 
                            WV_NETEVENT_RTOUTPUT_FAIL, WV_NET_SEND,
                            so->so_fd, error)
            }
#endif  /* INCLUDE_WVNET */
#endif

	if (rtm) {
		if (error)
			rtm->rtm_errno = error;
		else 
			rtm->rtm_flags |= RTF_DONE;
	}
	if (rt)
		rtfree(rt);
    {
	register struct rawcb *rp = 0;
	/*
	 * Check to see if we don't want our own messages.
	 */
	if ((so->so_options & SO_USELOOPBACK) == 0) {
		if (route_cb.any_count <= 1) {
			if (rtm)
				Free(rtm);
			m_freem(m);
			return (error);
		}
		/* There is another listener, so construct message */
		rp = sotorawcb(so);
	}
	if (rtm) {
		m_copyback(m, 0, rtm->rtm_msglen, (caddr_t)rtm);
		Free(rtm);
	}
	if (rp)
		rp->rcb_proto.sp_family = 0; /* Avoid us */
	if (dst)
		route_proto.sp_protocol = dst->sa_family;
	raw_input(m, &route_proto, &route_src, &route_dst);
	if (rp)
		rp->rcb_proto.sp_family = PF_ROUTE;
    }
	return (error);
}

static void
rt_setmetrics(which, in, out)
	u_long which;
	register struct rt_metrics *in, *out;
{
#define metric(f, e) if (which & (f)) out->e = in->e;
	metric(RTV_RPIPE, rmx_recvpipe);
	metric(RTV_SPIPE, rmx_sendpipe);
	metric(RTV_SSTHRESH, rmx_ssthresh);
	metric(RTV_RTT, rmx_rtt);
	metric(RTV_RTTVAR, rmx_rttvar);
	metric(RTV_HOPCOUNT, rmx_hopcount);
	metric(RTV_MTU, rmx_mtu);
	metric(RTV_EXPIRE, rmx_expire);

#ifdef ROUTER_STACK
    /*
     * Set the values for the new additional metrics,
     * except the weight, which must be postponed when
     * altering that value.
     */

    metric(RTV_VALUE1, value1)
    metric(RTV_VALUE2, value2)
    metric(RTV_VALUE3, value3)
    metric(RTV_VALUE4, value4)
    metric(RTV_VALUE5, value5)
    metric(RTV_ROUTETAG, routeTag)

#endif /* ROUTER_STACK */

#undef metric
}

#define ROUNDUP(a) \
	((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))
#define ADVANCE(x, n) (x += ROUNDUP((n)->sa_len))

static void
rt_xaddrs(cp, cplim, rtinfo)
	register caddr_t cp, cplim;
	register struct rt_addrinfo *rtinfo;
{
	register struct sockaddr *sa;
	register int i;

	bzero((char *)rtinfo->rti_info, sizeof(rtinfo->rti_info));
	for (i = 0; (i < RTAX_MAX) && (cp < cplim); i++) {
		if ((rtinfo->rti_addrs & (1 << i)) == 0)
			continue;
		rtinfo->rti_info[i] = sa = (struct sockaddr *)cp;
		ADVANCE(cp, sa);
	}
}

struct mbuf *
rt_msg1(type, rtinfo)
	int type;
	register struct rt_addrinfo *rtinfo;
{
	register struct rt_msghdr *rtm;
	register struct mbuf *m;
	register int i;
	register struct sockaddr *sa;
	int len, dlen;

	m = mHdrClGet(M_DONTWAIT, MT_DATA, CL_SIZE_128, TRUE);
	if (m == 0)
		return (m);
	switch (type) {

	case RTM_DELADDR:
	case RTM_NEWADDR:
		len = sizeof(struct ifa_msghdr);
		break;

	case RTM_IFINFO:
		len = sizeof(struct if_msghdr);
		break;

	default:
		len = sizeof(struct rt_msghdr);
	}
	if (len > m->m_extSize)
		panic("rt_msg1");
	m->m_pkthdr.len = m->m_len = len;
	m->m_pkthdr.rcvif = 0;
	rtm = mtod(m, struct rt_msghdr *);
	bzero((caddr_t)rtm, len);
	for (i = 0; i < RTAX_MAX; i++) {
		if ((sa = rtinfo->rti_info[i]) == NULL)
			continue;
		rtinfo->rti_addrs |= (1 << i);
		dlen = ROUNDUP(sa->sa_len);
		m_copyback(m, len, dlen, (caddr_t)sa);
		len += dlen;
	}
	if (m->m_pkthdr.len != len) {
		m_freem(m);
		return (NULL);
	}
	rtm->rtm_msglen = len;
	rtm->rtm_version = RTM_VERSION;
	rtm->rtm_type = type;
	return (m);
}

static int
rt_msg2(type, rtinfo, cp, w)
	int type;
	register struct rt_addrinfo *rtinfo;
	caddr_t cp;
	struct walkarg *w;
{
	register int i;
	int len, dlen, second_time = 0;
	caddr_t cp0;

	rtinfo->rti_addrs = 0;
again:
	switch (type) {

	case RTM_DELADDR:
	case RTM_NEWADDR:
		len = sizeof(struct ifa_msghdr);
		break;

	case RTM_IFINFO:
		len = sizeof(struct if_msghdr);
		break;

	default:
		len = sizeof(struct rt_msghdr);
	}
	if ((cp0 = cp))
		cp += len;
	for (i = 0; i < RTAX_MAX; i++) {
		register struct sockaddr *sa;

		if ((sa = rtinfo->rti_info[i]) == 0)
			continue;
		rtinfo->rti_addrs |= (1 << i);
		dlen = ROUNDUP(sa->sa_len);
		if (cp) {
			bcopy((caddr_t)sa, cp, (unsigned)dlen);
			cp += dlen;
		}
		len += dlen;
	}
	if (cp == 0 && w != NULL && !second_time) {
		register struct walkarg *rw = w;

		rw->w_needed += len;
		if (rw->w_needed <= 0 && rw->w_where) {
			if (rw->w_tmemsize < len) {
				if (rw->w_tmem)
				        {
					FREE(rw->w_tmem, MT_RTABLE);
					}
				MALLOC(rw->w_tmem, caddr_t, len, MT_RTABLE,
				       M_DONTWAIT); 
				if (rw->w_tmem)
					rw->w_tmemsize = len;
			}
			if (rw->w_tmem) {
				cp = rw->w_tmem;
				second_time = 1;
				goto again;
			} else
				rw->w_where = 0;
		}
	}
	if (cp) {
		register struct rt_msghdr *rtm = (struct rt_msghdr *)cp0;

		rtm->rtm_version = RTM_VERSION;
		rtm->rtm_type = type;
		rtm->rtm_msglen = len;
	}
	return (len);
}

/*
 * This routine is called to generate a message from the routing
 * socket indicating that a redirect has occured, a routing lookup
 * has failed, or that a protocol has detected timeouts to a particular
 * destination.
 */
void
rt_missmsg(type, rtinfo, flags, error)
	int type, flags, error;
	register struct rt_addrinfo *rtinfo;
{
	register struct rt_msghdr *rtm;
	register struct mbuf *m;
	struct sockaddr *sa = rtinfo->rti_info[RTAX_DST];

	if (route_cb.any_count == 0)
		return;
	m = rt_msg1(type, rtinfo);
	if (m == 0)
		return;
	rtm = mtod(m, struct rt_msghdr *);
	rtm->rtm_flags = RTF_DONE | flags;
	rtm->rtm_errno = error;
	rtm->rtm_addrs = rtinfo->rti_addrs;
	route_proto.sp_protocol = sa ? sa->sa_family : 0;
	raw_input(m, &route_proto, &route_src, &route_dst);
}

/*
 * This routine is called to generate a message from the routing
 * socket indicating that the status of a network interface has changed.
 */
void
rt_ifmsg(ifp)
	register struct ifnet *ifp;
{
	register struct if_msghdr *ifm;
	struct mbuf *m;
	struct rt_addrinfo info;

	if (route_cb.any_count == 0)
		return;
	bzero((caddr_t)&info, sizeof(info));
	m = rt_msg1(RTM_IFINFO, &info);
	if (m == 0)
		return;
	ifm = mtod(m, struct if_msghdr *);
	ifm->ifm_index = ifp->if_index;
	ifm->ifm_flags = ifp->if_flags;
	ifm->ifm_data = ifp->if_data;
	ifm->ifm_addrs = 0;
	route_proto.sp_protocol = 0;
	raw_input(m, &route_proto, &route_src, &route_dst);
}

/*
 * This is called to generate messages from the routing socket
 * indicating a network interface has had addresses associated with it.
 * if we ever reverse the logic and replace messages TO the routing
 * socket indicate a request to configure interfaces, then it will
 * be unnecessary as the routing socket will automatically generate
 * copies of it.
 */
void
rt_newaddrmsg(cmd, ifa, error, rt)
	int cmd, error;
	register struct ifaddr *ifa;
	register struct rtentry *rt;
{
	struct rt_addrinfo info;
	struct sockaddr *sa = NULL;
	int pass;
	struct mbuf *m = NULL;
	struct ifnet *ifp = ifa->ifa_ifp;
        int ncmd;

	if (route_cb.any_count == 0)
		return;
	for (pass = 1; pass < 3; pass++) {
		bzero((caddr_t)&info, sizeof(info));
		if ((cmd == RTM_ADD && pass == 1) ||
		    (cmd == RTM_DELETE && pass == 2)) {
			register struct ifa_msghdr *ifam;

			ncmd = cmd == RTM_ADD ? RTM_NEWADDR : RTM_DELADDR;

			ifaaddr = sa = ifa->ifa_addr;
			ifpaddr = ifp->if_addrlist->ifa_addr;
			netmask = ifa->ifa_netmask;
			brdaddr = ifa->ifa_dstaddr;
			if ((m = rt_msg1(ncmd, &info)) == NULL)
				continue;
			ifam = mtod(m, struct ifa_msghdr *);
			ifam->ifam_index = ifp->if_index;
			ifam->ifam_metric = ifa->ifa_metric;
			ifam->ifam_flags = ifa->ifa_flags;
			ifam->ifam_addrs = info.rti_addrs;
		}
		if ((cmd == RTM_ADD && pass == 2) ||
		    (cmd == RTM_DELETE && pass == 1)) {
			register struct rt_msghdr *rtm;
			
			if (rt == 0)
				continue;

                        ncmd = cmd;
			netmask = rt_mask(rt);
			dst = sa = rt_key(rt);
			gate = rt->rt_gateway;

#ifdef ROUTER_STACK
                /* Use a different message type for additional routes. */

                if ( ((ROUTE_ENTRY *)rt)->primaryRouteFlag == FALSE)
                    {
                    if (ncmd == RTM_ADD)
                        ncmd = RTM_ADDEXTRA;
                    else
                        ncmd = RTM_DELEXTRA;
                    }
#endif /* ROUTER_STACK */

			if ((m = rt_msg1(ncmd, &info)) == NULL)
				continue;
			rtm = mtod(m, struct rt_msghdr *);
			rtm->rtm_index = ifp->if_index;
			rtm->rtm_flags |= rt->rt_flags;
			rtm->rtm_errno = error;
			rtm->rtm_addrs = info.rti_addrs;
		}
		route_proto.sp_protocol = sa ? sa->sa_family : 0;
		raw_input(m, &route_proto, &route_src, &route_dst);
	}
}

/*
 * This is used in dumping the kernel table via sysctl().
 */
int
sysctl_dumpentry(rn, w)
	struct radix_node *rn;
	register struct walkarg *w;
{
	register struct rtentry *rt = (struct rtentry *)rn;
	int error = 0, size;
	struct rt_addrinfo info;

	if (w->w_op == NET_RT_FLAGS && !(rt->rt_flags & w->w_arg))
		return 0;
	bzero((caddr_t)&info, sizeof(info));
	dst = rt_key(rt);
	gate = rt->rt_gateway;
	netmask = rt_mask(rt);
	genmask = rt->rt_genmask;
	if (rt->rt_ifp) {
		ifpaddr = rt->rt_ifp->if_addrlist->ifa_addr;
		ifaaddr = rt->rt_ifa->ifa_addr;
		if (rt->rt_ifp->if_flags & IFF_POINTOPOINT)
			brdaddr = rt->rt_ifa->ifa_dstaddr;
	}
	size = rt_msg2(RTM_GET, &info, 0, w);
	if (w->w_where && w->w_tmem) {
		register struct rt_msghdr *rtm = (struct rt_msghdr *)w->w_tmem;

		rtm->rtm_flags = rt->rt_flags;
		rtm->rtm_use = rt->rt_use;
		rtm->rtm_rmx = rt->rt_rmx;
		rtm->rtm_index = rt->rt_ifp->if_index;
		rtm->rtm_errno = rtm->rtm_pid = rtm->rtm_seq = 0;
		rtm->rtm_addrs = info.rti_addrs;
		if ((error = copyout((caddr_t)rtm, w->w_where, size)))
			w->w_where = NULL;
		else
			w->w_where += size;
	}
	return (error);
}

int
sysctl_iflist(af, w)
	int	af;
	register struct	walkarg *w;
{
	register struct ifnet *ifp;
	register struct ifaddr *ifa;
	struct	rt_addrinfo info;
	int	len, error = 0;

	bzero((caddr_t)&info, sizeof(info));
#ifdef VIRTUAL_STACK
	for (ifp = _ifnet; ifp; ifp = ifp->if_next) {
#else
	for (ifp = ifnet; ifp; ifp = ifp->if_next) {
#endif
		if (w->w_arg && w->w_arg != ifp->if_index)
			continue;
		ifa = ifp->if_addrlist;
		ifpaddr = ifa->ifa_addr;
		len = rt_msg2(RTM_IFINFO, &info, (caddr_t)0, w);
		ifpaddr = 0;
		if (w->w_where && w->w_tmem) {
			register struct if_msghdr *ifm;

			ifm = (struct if_msghdr *)w->w_tmem;
			ifm->ifm_index = ifp->if_index;
			ifm->ifm_flags = ifp->if_flags;
			ifm->ifm_data = ifp->if_data;
			ifm->ifm_addrs = info.rti_addrs;
			if ((error = copyout((caddr_t)ifm, w->w_where, len)))
				return (error);
			w->w_where += len;
		}
		while ((ifa = ifa->ifa_next)) {
			if (af && af != ifa->ifa_addr->sa_family)
				continue;
			ifaaddr = ifa->ifa_addr;
			netmask = ifa->ifa_netmask;
			brdaddr = ifa->ifa_dstaddr;
			len = rt_msg2(RTM_NEWADDR, &info, 0, w);
			if (w->w_where && w->w_tmem) {
				register struct ifa_msghdr *ifam;

				ifam = (struct ifa_msghdr *)w->w_tmem;
				ifam->ifam_index = ifa->ifa_ifp->if_index;
				ifam->ifam_flags = ifa->ifa_flags;
				ifam->ifam_metric = ifa->ifa_metric;
				ifam->ifam_addrs = info.rti_addrs;
				if ((error = copyout(w->w_tmem, w->w_where,
                                                     len)))
					return (error);
				w->w_where += len;
			}
		}
		ifaaddr = netmask = brdaddr = 0;
	}
	return (0);
}

int
sysctl_rtable(name, namelen, where, given, new, newlen)
	int	*name;
	int	namelen;
	caddr_t	where;
	size_t	*given;
	caddr_t	*new;
	size_t	newlen;
{
	register struct radix_node_head *rnh;
	int	i, s, error = EINVAL;
	u_char  af;
	struct	walkarg w;

/*
 * XXX - This event does not usually occur: the sysctl_rtable() routine
 *       is typically only called by the Unix sysctl command which is not
 *       supported by VxWorks. It is also called by the RIP implementation
 *       as a legacy of the port from BSD, but that implementation detail
 *       is not of sufficient interest to include the event.

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /@ WV_NET_INFO event @/
        if (namelen != 3)
            {
            WV_NET_MARKER_1 (NET_AUX_EVENT, WV_NET_INFO, 22, 7, 
                             WV_NETEVENT_ROUTEWALK_START, 0)
            }
        else
            {
            WV_NET_MARKER_1 (NET_AUX_EVENT, WV_NET_INFO, 22, 7, 
                             WV_NETEVENT_ROUTEWALK_START, name[1])
            }
#endif  /@ INCLUDE_WVNET @/
#endif

 * XXX - end of unused event
 */

	if (new)
		return (EPERM);
	if (namelen != 3)
		return (EINVAL);
	af = name[0];
	Bzero(&w, sizeof(w));
	w.w_where = where;
	w.w_given = *given;
	w.w_needed = 0 - w.w_given;
	w.w_op = name[1];
	w.w_arg = name[2];

	s = splnet();
	switch (w.w_op) {

	case NET_RT_DUMP:
	case NET_RT_FLAGS:
		for (i = 1; i <= AF_MAX; i++)
			if ((rnh = rt_tables[i]) && (af == 0 || af == i) &&
			    (error = rnh->rnh_walktree(rnh,
							sysctl_dumpentry, &w)))
				break;
		break;

	case NET_RT_IFLIST:
		error = sysctl_iflist(af, &w);
	}
	splx(s);
	if (w.w_tmem){
		FREE(w.w_tmem, MT_RTABLE);
	}
	w.w_needed += w.w_given;
	if (where) {
		*given = w.w_where - where;
		if (*given < w.w_needed)
			return (ENOMEM);
	} else {
		*given = (11 * w.w_needed) / 10;
	}
	return (error);
}

/*
 * Definitions of protocols supported in the ROUTE domain.
 */
extern	struct domain routedomain;		/* or at least forward */

struct protosw routesw[] = {
{ SOCK_RAW,	&routedomain,	0,		PR_ATOMIC|PR_ADDR,
  raw_input,	route_output,	raw_ctlinput,	0,
  route_usrreq,
  rtSockInit,	0,		0,		0,
  sysctl_rtable,
}
};

struct domain routedomain =
    { PF_ROUTE, "route", 0, 0, 0,
      routesw, &routesw[sizeof(routesw)/sizeof(routesw[0])] };

/*
 * Copy data from a buffer back into the indicated mbuf chain,
 * starting "off" bytes from the beginning, extending the mbuf
 * chain if necessary.
 */
static void
m_copyback(m0, off, len, cp)
	struct	mbuf *m0;
	register int off;
	register int len;
	caddr_t cp;
{
	register int mlen;
	register struct mbuf *m = m0, *n;
	int totlen = 0;

	if (m0 == 0)
		return;
	while (off > (mlen = m->m_len)) {
		off -= mlen;
		totlen += mlen;
		if (m->m_next == 0) {
			n = m_getclr(M_DONTWAIT, m->m_type, CL_SIZE_128, TRUE);
			if (n == 0)
				goto out;
			n->m_len = min(n->m_extSize, len + off);
			m->m_next = n;
		}
		m = m->m_next;
	}
	while (len > 0) {
		mlen = min (m->m_len - off, len);
		bcopy(cp, off + mtod(m, caddr_t), (unsigned)mlen);
		cp += mlen;
		len -= mlen;
		mlen += off;
		off = 0;
		totlen += mlen;
		if (len == 0)
			break;
		if (m->m_next == 0) {
			n = mBufClGet(M_DONTWAIT, m->m_type, CL_SIZE_128, TRUE);
			if (n == 0)
				break;
			n->m_len = min(n->m_extSize, len);
			m->m_next = n;
		}
		m = m->m_next;
	}
out:	if (((m = m0)->m_flags & M_PKTHDR) && (m->m_pkthdr.len < totlen))
		m->m_pkthdr.len = totlen;
}

/*******************************************************************************
*
* m_copydata - copy data from an mbuf chain into a buff
* Copy data from an mbuf chain starting "off" bytes from the beginning,
* continuing for "len" bytes, into the indicated buffer.
*/
static void m_copydata(m, off, len, cp)
    register struct mbuf *m;
    register int off;
    register int len;
    caddr_t cp;
    {
    register unsigned count;

    if (off < 0 || len < 0)
	panic("m_copydata");
    while (off > 0)
	{
	if (m == 0)
            {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_EMERGENCY event */
            WV_NET_MARKER_0 (NET_AUX_EVENT, WV_NET_EMERGENCY, 26, 3, 
                             WV_NETEVENT_COPYDATA_PANIC)
#endif  /* INCLUDE_WVNET */
#endif

            panic("m_copydata");
            }

	if (off < m->m_len)
	    break;
	off -= m->m_len;
	m = m->m_next;
	}
    while (len > 0)
	{
	if (m == 0)
	    panic("m_copydata");
	count = min(m->m_len - off, len);
	bcopy(mtod(m, caddr_t) + off, cp, count);
	len -= count;
	cp += count;
	off = 0;
	m = m->m_next;
	}
    }

#ifdef ROUTER_STACK
/*******************************************************************************
*
* routeGatewayFetch - build a message which includes a list of gateways
*
* This routine constructs a routing socket message which supplies each
* next-hop router for all duplicate routes with a given destination and
* netmask. When the <pMessage> parameter is NULL, the routine simply
* counts the required length for the message. When that parameter provides
* an actual value, the routine stores the appropriate information in
* the supplied buffer. The RTAX_GATEWAY pointer for the resulting message
* accesses a series of customized address structures which include the
* protocol identifier and all additional metrics for each entry.
*
* RETURNS: Length of message for all route entries
*
* NOMANUAL
*/

LOCAL int routeGatewayFetch
    (
    struct rtentry * pRoute,     /* primary route for destination/netmask */
    struct rt_msghdr * pMessage  /* list of routes for routing socket */
    )
    {
    int length;
    char * pBuffer;
    struct sockaddr * pAddress = NULL;
    struct sockaddr_gate * pGate;

    int addrlen;

    ROUTE_ENTRY * pGroup;    /* route entries for each protocol */
    ROUTE_ENTRY * pMatch;    /* current route entry for message */

    length = sizeof (struct rt_msghdr);

    if (pMessage)
        {
        /* Fill in the initial message header. */

        pMessage->rtm_version = RTM_VERSION;
        pMessage->rtm_type = RTM_GETALL;

        pMessage->rtm_flags = pRoute->rt_flags;

        pMessage->rtm_addrs = RTA_DST | RTA_GATEWAY;
        }

    pAddress = rt_key (pRoute);
    addrlen = ROUNDUP(pAddress->sa_len);

    pBuffer = (char *)pMessage;
    if (pBuffer)
        {
        /* Copy the destination address for the route entries. */

        pBuffer += length;

        bcopy ( (char *)pAddress, pBuffer, addrlen);
        pBuffer += addrlen;
        }

    length += addrlen;

    /*
     * Find the length of all entries for the specified destination
     * and netmask. Copy the relevant values if appropriate.
     */

    for (pGroup = (ROUTE_ENTRY *)pRoute; pGroup != NULL;
                 pGroup = pGroup->diffNode.pFrwd)
        {
        for (pMatch = pGroup; pMatch != NULL; pMatch = pMatch->sameNode.pFrwd)
            {
            pAddress = ROUTE_ENTRY_GATEWAY (pMatch);
            if (pAddress->sa_family == AF_LINK)
                {
                /*
                 * The entry contains an ARP template. Use the
                 * interface's IP address as the gateway value.
                 */

                pAddress = pMatch->rtEntry.rt_ifa->ifa_addr;
                }

            addrlen = ROUNDUP(sizeof (struct sockaddr_gate));

            if (pBuffer)
                {
                /* Copy the gateway address for the route entry. */

                pGate = (struct sockaddr_gate *)pBuffer;

                pGate->gate_family = pAddress->sa_family;
                pGate->gate_addr = 
                    ( (struct sockaddr_in *)pAddress)->sin_addr.s_addr;

                /* Set length to reflect increased size for metrics. */

                pGate->gate_len = sizeof (struct sockaddr_gate);

                /* Copy additional information into gateway field. */

                pGate->routeProto = RT_PROTO_GET (ROUTE_ENTRY_KEY (pMatch));
                pGate->weight = pMatch->rtEntry.rt_rmx.weight;
                pGate->value1 = pMatch->rtEntry.rt_rmx.value1;
                pGate->value2 = pMatch->rtEntry.rt_rmx.value2;
                pGate->value3 = pMatch->rtEntry.rt_rmx.value3;
                pGate->value4 = pMatch->rtEntry.rt_rmx.value4;
                pGate->value5 = pMatch->rtEntry.rt_rmx.value5;
                pGate->routeTag = pMatch->rtEntry.rt_rmx.routeTag;

                pBuffer += addrlen;
                }

            length += addrlen;
            }
        }

    /* Copy any netmask information for the route. */

    pAddress = rt_mask (pRoute);

    if (pAddress)
        {
        addrlen = ROUNDUP(pAddress->sa_len);

        if (pBuffer)
            {
            bcopy ( (char *)pAddress, pBuffer, addrlen);
            pBuffer += addrlen;

            /* Indicate that netmask information is present in the message. */

            pMessage->rtm_addrs |= RTA_NETMASK;
            }

        length += addrlen;
        }

    if (pMessage)
        {
        /* Set the total message length. */

        pMessage->rtm_msglen = length;
        }

    return (length);
    }

/*******************************************************************************
*
* routeMatch - find a unique entry within a set of duplicate routes
*
* This routine searches the extended lists attached to the <pRoute>
* primary route entry to find a matching route with the same next-hop
* gateway as the <pGateway> argument and a creator which matches the
* <protoId> value. That combination is unique for a particular destination
* address and netmask. This routine allows routing sockets to retrieve
* a duplicate route entry which is not visible to the IP forwarding process.
*
* RETURNS: Pointer to existing route, or NULL if none.
*
* NOMANUAL
*/

LOCAL ROUTE_ENTRY * routeMatch
    (
    struct rtentry * pRoute,    /* primary route for destination/netmask */
    short protoId,              /* protocol identifer for matching route */
    struct sockaddr * pGateway  /* next hop gateway for matching route */
    )
    {
    ROUTE_ENTRY * pGroup;    /* route entries with matching protocol */
    ROUTE_ENTRY * pMatch;    /* exact match for protocol and gateway */

    if (protoId)
        {
        /* Find the list of entries for the specified protocol. */

        for (pGroup = (ROUTE_ENTRY *)pRoute; pGroup != NULL;
                 pGroup = pGroup->diffNode.pFrwd)
            {
            if (RT_PROTO_GET(ROUTE_ENTRY_KEY(pGroup)) == protoId)
                break;
            }
        }
    else
        {
        /* No protocol ID specified: use the initial group. */

        pGroup = (ROUTE_ENTRY *)pRoute;
        }

    /* Find matching gateway if matching protocol found. */

    for (pMatch = pGroup; pMatch != NULL; pMatch = pMatch->sameNode.pFrwd)
        {
        if (ROUTE_ENTRY_GATEWAY (pMatch)->sa_family == AF_LINK)
            {
            /*
             * The gateway field contains an ARP template. The entry
             * only matches for a search using the interface's IP address
             * as the gateway.
             */

            if (SOCKADDR_IN (pMatch->rtEntry.rt_ifa->ifa_addr)
                                    == SOCKADDR_IN (pGateway))
                {
                /* Matching route entry found - halt search. */

                break;
                }
            }
        else if (SOCKADDR_IN (ROUTE_ENTRY_GATEWAY (pMatch))
                                    == SOCKADDR_IN (pGateway))
            {
            /* Matching route entry found - halt search. */

            break;
            }
        }

    return (pMatch);
    }
#endif /* ROUTER_STACK */
