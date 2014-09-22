/* tables.c - routines for managing RIP internal routing table */

/* Copyright 1984 - 2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
 * Copyright (c) 1983, 1988, 1993
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
 * @(#)tables.c	8.2 (Berkeley) 4/28/95
 */

/*
modification history
--------------------
01r,22mar02,niq  Merged from Synth view, tor3_x.synth branch, ver 02c
01q,24jan02,niq  SPR 72415 - Added support for Route tags
01p,15oct01,rae  merge from truestack ver 01w, base 01m (SPRs 70188, 69983,
                 65547, etc.)
01o,21nov00,spm  fixed creation of internal default route (SPR #62533)
01n,10nov00,spm  merged from version 01n of tor3_x branch (code cleanup)
01m,16mar99,spm  recovered orphaned code from tor1_0_1.sens1_1 (SPR #25770)
01l,29sep98,spm  added support for IP group MIB (SPR #9374); removed unneeded
                 routine (SPR #21109).
01k,11sep98,spm  moved mutual exclusion semaphore to prevent any collisions
                 between RIP tasks (SPR #22350), removed all references to 
                 bloated trace commands (SPR #22350)
01j,01sep98,spm  corrected processing of RIPv2 route updates to support
                 version changes (SPR #22158); added support for default
                 routes (SPR #22067)
01i,26jul98,spm  removed compiler warnings
01h,06oct97,gnn  fixed insque/remque casting bug and cleaned up warnings
01g,12aug97,gnn  fixed prototypes for mRouteEntryDelete().
01f,15may97,gnn  added code to implement route leaking.
01e,14apr97,gnn  fixed calls to mRouteEntryAdd to follow Rajesh's changes.
01d,07apr97,gnn  cleared up some of the more egregious warnings.
                 added MIB-II interfaces and options.
01c,24feb97,gnn  added rip version 2 functionality.
01b,05dec96,gnn  changed insque/remque to _insque/_remque for compatability.
01a,26nov96,gnn  created from BSD4.4 routed
*/

/*
DESCRIPTION
*/

/*
 * Routing Table Management Daemon
 */
#include "vxWorks.h"
#include "rip/defs.h"
#include "sys/ioctl.h"
#include "netinet/in.h"
#include "routeLib.h"
#include "errno.h"
#include "inetLib.h"
#include "semLib.h"
#include "logLib.h"
#include "m2Lib.h" 
#include "stdlib.h"
#include "routeEnhLib.h"
#ifdef VIRTUAL_STACK
#include "netinet/vsLib.h"
#include "netinet/vsRip.h"
#endif /* VIRTUAL_STACK */

/* defines */

#ifdef RTM_ADD
#define FIXLEN(s) {if ((s)->sa_len == 0) (s)->sa_len = sizeof *(s);}
#else
#define FIXLEN(s) { }
#endif /* RTM_ADD */

/* globals */

#ifndef VIRTUAL_STACK
IMPORT int routedDebug;
IMPORT RIP ripState;
#endif /* VIRTUAL_STACK */

/* locals */

LOCAL struct sockaddr_in inet_default = {
#ifdef RTM_ADD
        sizeof (inet_default),
#endif
        AF_INET, INADDR_ANY };

/* forward declarations */

IMPORT void ripSockaddrPrint (struct sockaddr * pSockAddr);
LOCAL void ripInsque (NODE * pNode, NODE * pPrev);
LOCAL void ripRemque (NODE * pNode);
LOCAL STATUS ripSystemRouteAdd (long dstIp, long gateIp, long mask, int flags);
LOCAL STATUS ripSystemRouteDelete (long dstIp, long gateIp, long mask, int flags);
void ripRouteMetricSet (struct rt_entry * pRtEntry);

/*
 * Lookup dst in the tables for an exact match.
 */
struct rt_entry *
rtlookup (dst)
	struct sockaddr *dst;
{
	register struct rt_entry *rt;
	register struct rthash *rh;
	register u_int hash;
	struct sockaddr target;
	struct afhash h;
	int doinghost = 1;

	if (dst->sa_family >= AF_MAX)
		return (0);
	(*afswitch[dst->sa_family].af_hash) (dst, &h);
	hash = h.afh_hosthash;
	rh = &hosthash[hash & ROUTEHASHMASK];

        /* 
         * The equal() comparison within the following loop expects all
         * members of the structure except the family, length, and address
         * fields to be zero. This condition may not be met when processing a
         * RIPv2 packet. In that case, the destination passed to this routine 
         * accesses a route within the payload of a RIP message, so some
         * fields of the overlayed structure will not be zero as expected.
         * Duplicate or incorrect routes will be added because matching routes
         * are not detected. To avoid this problem, copy the required fields 
         * from the route destination value to a clean structure.
         */

        bzero ( (char *)&target, sizeof (struct sockaddr));
        target.sa_family = dst->sa_family;
        target.sa_len = dst->sa_len;
        ((struct sockaddr_in *)&target)->sin_addr.s_addr = 
                    ((struct sockaddr_in *)dst)->sin_addr.s_addr;
        
again:
	for (rt = rh->rt_forw; rt != (struct rt_entry *)rh; rt = rt->rt_forw) {
		if (rt->rt_hash != hash)
			continue;
		if (equal (&rt->rt_dst, &target))
			return (rt);
	}
	if (doinghost) {
		doinghost = 0;
		hash = h.afh_nethash;
		rh = &nethash[hash & ROUTEHASHMASK];
		goto again;
	}
	return (0);
}


#ifdef notyet
#ifndef VIRTUAL_STACK
struct sockaddr wildcard;	/* zero valued cookie for wildcard searches */
#endif
#endif

/*
 * Find a route to dst as the kernel would.
 */
struct rt_entry *
rtfind (dst)
	struct sockaddr *dst;
{
	register struct rt_entry *rt;
	register struct rthash *rh;
	register u_int hash;
	struct sockaddr target;
	struct afhash h;
	int af = dst->sa_family;
	int doinghost = 1;
        int (*match) () = NULL; 	/* Assigned later for network hash search. */

	if (af >= AF_MAX)
		return (0);
	(*afswitch[af].af_hash) (dst, &h);
	hash = h.afh_hosthash;
	rh = &hosthash[hash & ROUTEHASHMASK];

        /* 
         * The equal() comparison within the following loop expects all
         * members of the structure except the family, length, and address
         * fields to be zero. This condition may not be met when processing a
         * RIPv2 packet. In that case, the destination passed to this routine 
         * accesses a route within the payload of a RIP message, so some
         * fields of the overlayed structure will not be zero as expected,
         * and existing host routes would be missed. To avoid this problem,              * copy the required fields from the route destination value to a 
         * clean structure.
         */

        bzero ( (char *)&target, sizeof (struct sockaddr));
        target.sa_family = dst->sa_family;
        target.sa_len = dst->sa_len;
        ((struct sockaddr_in *)&target)->sin_addr.s_addr = 
                    ((struct sockaddr_in *)dst)->sin_addr.s_addr;
        
again:
	for (rt = rh->rt_forw; rt != (struct rt_entry *)rh; rt = rt->rt_forw) {
		if (rt->rt_hash != hash)
			continue;
		if (doinghost) {
			if (equal (&rt->rt_dst, &target))
				return (rt);
		} else {
			if (rt->rt_dst.sa_family == af &&
			    (*match) (&rt->rt_dst, dst))
				return (rt);
		}
	}
	if (doinghost) {
		doinghost = 0;
		hash = h.afh_nethash;
		rh = &nethash[hash & ROUTEHASHMASK];
		match = afswitch[af].af_netmatch;
		goto again;
	}
#ifdef notyet
	/*
	 * Check for wildcard gateway, by convention network 0.
	 */
	if (dst != &wildcard) {
		dst = &wildcard, hash = 0;
		goto again;
	}
#endif
	return (0);
}

struct rt_entry * rtadd
    (
    struct sockaddr *dst, 
    struct sockaddr *gate, 
    int metric, 
    int state, 
    struct sockaddr *netmask, 
    int proto, 
    int tag, 
    int from, 
    struct interface *ifp
    )
    {
    struct afhash h;
    register struct rt_entry *rt;
    struct rthash *rh;
    int af = dst->sa_family, flags;
    struct sockaddr_in* pDsin;
    struct sockaddr_in* pGsin;
    struct sockaddr_in* pNsin;
    u_int hash;
    struct interface* pIfp;
    int ret = ERROR;

    if (af >= AF_MAX)
        return ((struct rt_entry *)NULL);
    (*afswitch[af].af_hash) (dst, &h);
    flags = (*afswitch[af].af_rtflags) (dst);
    /*
     * Subnet flag isn't visible to kernel, move to state.	XXX
     */
    FIXLEN (dst);
    FIXLEN (gate);
    if (flags & RTF_SUBNET)
        {
        state |= RTS_SUBNET;
        flags &= ~RTF_SUBNET;
        }
    if (flags & RTF_HOST)
        {
        hash = h.afh_hosthash;
        rh = &hosthash[hash & ROUTEHASHMASK];
	}
    else
        {
        hash = h.afh_nethash;
        rh = &nethash[hash & ROUTEHASHMASK];
	}
    rt = (struct rt_entry *)malloc (sizeof (*rt));
    if (rt == 0)
        return ((struct rt_entry *)NULL);
    rt->inKernel = FALSE;
    rt->rt_hash = hash;
    rt->rt_dst = *dst;
    rt->rt_router = *gate;
    rt->rt_timer = 0;
    rt->rt_flags = RTF_UP | flags;
    rt->rt_state = state | RTS_CHANGED;
    rt->rt_proto = proto;
    rt->rt_tag = tag;
    rt->rt_refcnt = 0;
    rt->rt_subnets_cnt = 0;
    rt->rt_orgrouter = from;

    /*
     * A matching interface exists for all standard destinations or
     * the RIP session will not begin the add operation, but no
     * interface exists for the default route. The rt_ifp field is
     * equal to zero in that case.
     * Also, we need to search for the gateway address for the non-rip 
     * route.
     */

    if (ifp)
        rt->rt_ifp = ifp;
    else
        {
        rt->rt_ifp = ripIfWithDstAddr (&rt->rt_dst, &rt->rt_router);
        if (rt->rt_ifp == 0) 
            rt->rt_ifp = ripIfWithNet (&rt->rt_router);
        }

    if ((state & RTS_INTERFACE) == 0)
        {
        rt->rt_flags |= RTF_GATEWAY;
        flags |= RTF_GATEWAY;
        }
    rt->rt_metric = metric;

    /* 
     * Set the netmask. The interface's netmask is used if none is included
     * in the RIP message. Otherwise, the specified netmask is used, except
     * for default routes, which always use a netmask of 0.
     */

    if (netmask != (struct sockaddr *)NULL && 
         ((struct sockaddr_in *)dst)->sin_addr.s_addr)
        {
        rt->rt_netmask = *netmask; 
        }
    else
        {
        /* Leave the netmask value as zero if a default route is used. */

        bzero ((char *)&rt->rt_netmask, sizeof (rt->rt_netmask));
        rt->rt_netmask.sa_family = AF_INET;
        rt->rt_netmask.sa_len = 16;
        pNsin = (struct sockaddr_in *)&rt->rt_netmask;
        if ( ((struct sockaddr_in *)dst)->sin_addr.s_addr)
            {
            pNsin->sin_addr.s_addr = htonl (rt->rt_ifp->int_subnetmask);

            /* 
             * Learned routes and locally generated interface routes use the
             * (possibly classless) subnet mask value. Internally generated 
             * routes provided to implement border gateway filtering will
             * use a class-based netmask in all updates to be understood 
             * by RIP-2 routers on "distant" hosts not directly connected to 
             * the destination. Since supernet routes must use the classless 
             * netmask during border gateway filtering to correctly detect 
             * matching supernet numbers, they retain that value internally 
             * while the subnetted routes replace it with the class-based 
             * value.
             */

            if (rt->rt_state & RTS_INTERNAL)
                {
                /* 
                 * Check if the subnetmask is shorter than the (class-based)
                 * netmask. This test works because equal values are not 
                 * possible for these routes (RTS_SUBNET is always set).
                 */

                if ( (rt->rt_ifp->int_subnetmask | rt->rt_ifp->int_netmask) ==
                    rt->rt_ifp->int_subnetmask)
                    {
                    /* Longer mask. Replace with class-based value. */ 

                    pNsin->sin_addr.s_addr = htonl (rt->rt_ifp->int_netmask);
                    }
                }
            }
        }

    ripRouteToAddrs (rt, &pDsin, &pGsin, &pNsin);
    
    pIfp = rt->rt_ifp;

    if (((state & RTS_OTHER) == 0) && (pIfp) && (pIfp->leakHook))
        {
        if (pIfp->leakHook (pDsin->sin_addr.s_addr,
                            pGsin->sin_addr.s_addr,
                            pNsin->sin_addr.s_addr) == OK)
            {
            free ((char *)rt);
            return ((struct rt_entry *)NULL);
            }
        }

    ripInsque ((NODE *)rt, (NODE *)rh);

    if (routedDebug > 1)
        logMsg ("Adding route to %x through %x (mask %x).\n", 
                pDsin->sin_addr.s_addr, 
                pGsin->sin_addr.s_addr, 
                pNsin->sin_addr.s_addr, 0, 0, 0);


    if ((rt->rt_state & (RTS_INTERNAL | RTS_EXTERNAL | RTS_OTHER)) == 0 &&
        (ret = ripSystemRouteAdd (pDsin->sin_addr.s_addr, 
                                  pGsin->sin_addr.s_addr, 
                                  pNsin->sin_addr.s_addr, flags)) == ERROR)
        {
        if (errno != EEXIST && gate->sa_family < AF_MAX)
            {
            if (routedDebug)
                logMsg ("Error %x adding route to %s through gateway %s.\n",
                        errno, 
                        (int)(*afswitch[dst->sa_family].af_format) (dst),
                        (int)(*afswitch[gate->sa_family].af_format) (gate),
                        0, 0, 0);
            }

        if (routedDebug)
            logMsg ("Error %x adding new route.\n", errno, 0, 0, 0, 0, 0);

        if (errno == ENETUNREACH)
            {
            ripRemque ((NODE *)rt);
            free ((char *)rt);
            rt = 0;
            }
        }
    else
        {
        /* Store copy of metric in kernel routing table for IP group MIB. */

        if (ret == OK)
            {
            rt->inKernel = TRUE;
            ripRouteMetricSet (rt);
            }

        ripState.ripGlobal.rip2GlobalRouteChanges++;
        }

    return ((struct rt_entry *)rt);
    }

STATUS rtchange
    (
    struct rt_entry *rt,
    struct sockaddr *gate,
    short metric,
    struct sockaddr *netmask, 
    int tag,
    int from,
    struct interface *ifp
    )
    {
    int add = 0, delete = 0, newgateway = 0;
    struct rt_entry oldroute;
    struct sockaddr_in *pDsin;
    struct sockaddr_in *pGsin;
    struct sockaddr_in *pNsin;
    char address[32];
    int flags;
    BOOL wasInterfaceRoute = FALSE;

    FIXLEN (gate);
    FIXLEN (&(rt->rt_router));
    FIXLEN (&(rt->rt_dst));
    if (!equal (&rt->rt_router, gate))
        {
        newgateway++;
        if (routedDebug > 1)
            logMsg ("Changing route gateway.\n", 0, 0, 0, 0, 0, 0);
        }
    else if (metric != rt->rt_metric)
        if (routedDebug > 1)
            logMsg ("Changing route metric.\n", 0, 0, 0, 0, 0, 0);
    if (tag != rt->rt_tag)
        if (routedDebug > 1)
            logMsg ("Changing route tag.\n", 0, 0, 0, 0, 0, 0);
    
    if ((rt->rt_state & RTS_INTERNAL) == 0)
        {
        /*
         * If changing to different router, we need to add
         * new route and delete old one if in the kernel.
         * If the router is the same, we need to delete
         * the route if has become unreachable, or re-add
         * it if it had been unreachable.
         */
        if (newgateway)
            {
            add++;
            if ((rt->rt_metric != HOPCNT_INFINITY) || rt->inKernel)
                delete++;
            }
        else if (metric == HOPCNT_INFINITY)
            delete++;
        else if (rt->rt_metric == HOPCNT_INFINITY)
            add++;
	}

    /*
     * If routedInput() had set the metric to HOPCNT_INFINITY + 1 to 
     * trick us into adding a route with orignal metric of 15 (adjusted to 16
     * to accommodate the additional hop), reset the metric to
     * HOPCNT_INFINITY
     */

    if (metric > HOPCNT_INFINITY)
        metric = HOPCNT_INFINITY;

    if (delete)
        oldroute = *rt;

    /* Record the route flags ignoring the UP flag */

    flags = rt->rt_flags & ~RTF_UP;

    if ((rt->rt_state & RTS_INTERFACE) && delete)
        {
        wasInterfaceRoute = TRUE;
        rt->rt_state &= ~RTS_INTERFACE;
        rt->rt_flags |= RTF_GATEWAY;
        if (metric > rt->rt_metric && delete)
            if (routedDebug)
                logMsg ("Warning! %s route to interface %s (timed out).\n",
                        (int)(add ? "Changing" : "Deleting"),
                        (int)(rt->rt_ifp ? rt->rt_ifp->int_name : "?"), 
                        0, 0, 0, 0);
        }
    if (add)
        {
        rt->rt_router = *gate;
        if (ifp)
            rt->rt_ifp = ifp;
        else
            {
            rt->rt_ifp = ripIfWithDstAddr (&rt->rt_router, NULL);
            if (rt->rt_ifp == 0) 
                rt->rt_ifp = ripIfWithNet (&rt->rt_router);
            }
        if (netmask != (struct sockaddr *)NULL)
            {
            rt->rt_netmask = *netmask; 
            }
        else
            {
            /*
             * Leave the netmask value as zero if a default route is used.
             * The rt_ifp field is zero in that case, so no other value
             * is available.
             */

            bzero ((char *)&rt->rt_netmask, sizeof (rt->rt_netmask));
            rt->rt_netmask.sa_family = AF_INET;
            rt->rt_netmask.sa_len = 16;
            pNsin = (struct sockaddr_in *)&rt->rt_netmask;

            if ( ((struct sockaddr_in *)&rt->rt_dst)->sin_addr.s_addr)
                pNsin->sin_addr.s_addr = htonl (rt->rt_ifp->int_subnetmask);
            }
        }
    rt->rt_metric = metric;
    rt->rt_tag = tag;
    if (from != 0)
        rt->rt_orgrouter = from;
    rt->rt_state |= RTS_CHANGED;
    if (newgateway)
        if (routedDebug > 1)
            logMsg ("Changing route gateway.\n", 0, 0, 0, 0, 0, 0);
    
    if (delete && !add)
        {
        ripRouteToAddrs (&oldroute, &pDsin, &pGsin, &pNsin);
        inet_ntoa_b (pDsin->sin_addr, (char *)&address);
        if (ripSystemRouteDelete (pDsin->sin_addr.s_addr, 
                                  pGsin->sin_addr.s_addr, 
                                  pNsin->sin_addr.s_addr, flags) == ERROR)
            {
            if (routedDebug)
                logMsg ("Route change - error %x removing old route.\n",
                        errno, 0, 0, 0, 0, 0);
            }
        else
            {
            rt->inKernel = FALSE;
            ripState.ripGlobal.rip2GlobalRouteChanges++;
            }
        }
    else if (!delete && add)
        {
        ripRouteToAddrs (rt, &pDsin, &pGsin, &pNsin);
        if (ripSystemRouteAdd (pDsin->sin_addr.s_addr, 
                               pGsin->sin_addr.s_addr, 
                               pNsin->sin_addr.s_addr, flags) == ERROR)
            {
            if (routedDebug)
                logMsg ("Route change - error %x adding new route.\n",
                        errno, 0, 0, 0, 0, 0);
            }
        else
            {
            /* Store metric in kernel routing table for IP group MIB. */

            rt->inKernel = TRUE;
            ripRouteMetricSet (rt);
            ripState.ripGlobal.rip2GlobalRouteChanges++;
            }
        }
    else if (delete && add)
        {
        ripRouteToAddrs (&oldroute, &pDsin, &pGsin, &pNsin);
        inet_ntoa_b (pDsin->sin_addr, (char *)&address);
        if (ripSystemRouteDelete (pDsin->sin_addr.s_addr, 
                                  pGsin->sin_addr.s_addr, 
                                  pNsin->sin_addr.s_addr, flags) == ERROR)
            {
            if (routedDebug)
                logMsg ("Route change - error %x replacing old route.\n",
                        errno, 0, 0, 0, 0, 0);
            }
        else
            {
            /* 
             * If it was an interface route, now we are changing it to a 
             * gateway route. Update the flags to reflect that
             */

            if (wasInterfaceRoute)
                flags |= RTF_GATEWAY;

            ripRouteToAddrs (rt, &pDsin, &pGsin, &pNsin);
            if (ripSystemRouteAdd (pDsin->sin_addr.s_addr, 
                                   pGsin->sin_addr.s_addr, 
                                   pNsin->sin_addr.s_addr, flags) == ERROR)
                {
                if (routedDebug)
                    logMsg ("Route change - error %x creating new route.\n",
                            errno, 0, 0, 0, 0, 0);
                rt->inKernel = FALSE;
                }
            else
                {
                /* Store metric in kernel routing table for IP group MIB. */

                rt->inKernel = TRUE;
                ripRouteMetricSet (rt);
                ripState.ripGlobal.rip2GlobalRouteChanges++;
                }
            }
        }
    else
        {
        /* Neither delete nor add - must be a metric change */ 

        ripRouteMetricSet (rt);
        }

    return (OK);
    }

STATUS rtdelete (rt)
    struct rt_entry *rt;
    {
    
    char address[32];
    struct sockaddr_in *pDsin;
    struct sockaddr_in *pGsin;
    struct sockaddr_in *pNsin;

    if (routedDebug > 1)
        logMsg ("Removing route entry.\n", 0, 0, 0, 0, 0, 0);

    FIXLEN (&(rt->rt_router));
    FIXLEN (&(rt->rt_dst));
    if (rt->rt_metric < HOPCNT_INFINITY || rt->inKernel)
        {
        if ( (rt->rt_state & (RTS_INTERFACE|RTS_INTERNAL)) == RTS_INTERFACE)
            {
            if (routedDebug)
                logMsg ("Deleting route to interface %s? (timed out?).\n",
                        (int)rt->rt_ifp->int_name, 0, 0, 0, 0, 0);
            }
        if ( (rt->rt_state & (RTS_INTERNAL | RTS_EXTERNAL | RTS_OTHER)) == 0)
            {
            ripRouteToAddrs (rt, &pDsin, &pGsin, &pNsin);
            inet_ntoa_b (pDsin->sin_addr, (char *)&address);
            if (ripSystemRouteDelete (pDsin->sin_addr.s_addr, 
                                      pGsin->sin_addr.s_addr, 
                                      pNsin->sin_addr.s_addr, 
                                      rt->rt_flags & ~RTF_UP) == ERROR)
                {
                if (routedDebug)
                    logMsg ("Route delete - error %x removing route.\n",
                            errno, 0, 0, 0, 0, 0);
                }
            else
                {
                rt->inKernel = FALSE;
                ripState.ripGlobal.rip2GlobalRouteChanges++;
                }
            }
        }

    ripRemque ((NODE *)rt);

    /*
     * If this is an interface route and there exists reference to it,
     * then that means that there are other interfaces that have
     * addresses that match this route. We need to add new routes for
     * those interfaces.
     * if rt_refcnt == rt_subnets_cnt, it means that all the interfaces
     * are subnetted, ie 164.10.1.1/24, 164.10.2.2/24, etc
     * if rt_refcnt != rt_subnets_cnt, it means that atleast one 
     * interface is not subnetted, e.g 164.10.1.1/16, 164.10.2.2/24, etc
     */

    if (rt->rt_state & RTS_INTERFACE && rt->rt_refcnt && 
        ! (rt->rt_state & RTS_EXTERNAL))
        {
        if (rt->rt_refcnt != rt->rt_subnets_cnt) 
            {
            /*
             * We are deleting a non-subnetted interface route and there
             * exist other non-subnetted interfaces on the same network.
             * We need to add a route for the network through one of those
             * interfaces
             */
            ifRouteAdd (&(rt->rt_dst), rt->rt_refcnt - 1, rt->rt_subnets_cnt,
                        FALSE, rt->rt_ifp->int_subnetmask);
            }
        else
            {
            /*
             * We are deleting an "internal" route that we had generated
             * for border gateway filtering. There exist other interfaces
             * that need this route. We need to add another route through
             * one of those interfaces
             */
            ifRouteAdd (&(rt->rt_dst), rt->rt_refcnt - 1, 
                        rt->rt_subnets_cnt - 1, TRUE, 
                        rt->rt_ifp->int_subnetmask);
            }
        }
    free ((char *)rt);
    
    return (OK);
    }

void rtdeleteall (void)
    {
    register struct rthash *rh;
    register struct rt_entry *rt;

    struct rthash *base = hosthash;
    int doinghost = 1;
    
    char address[32];
    struct sockaddr_in *pDsin;
    struct sockaddr_in *pGsin;
    struct sockaddr_in *pNsin;

    again:
    for (rh = base; rh < &base[ROUTEHASHSIZ]; rh++)
        {
        rt = rh->rt_forw;
	for (; rt != (struct rt_entry *)rh; 
                 rt = ( (struct rt_entry *)rh)->rt_forw)
            {
            /* Remove the entry from the kernel routing table if present. */

            if ( (rt->rt_state & (RTS_INTERNAL|RTS_EXTERNAL|RTS_OTHER)) == 0)
                {
                ripRouteToAddrs (rt, &pDsin, &pGsin, &pNsin);
                inet_ntoa_b (pDsin->sin_addr, (char *)&address);

                /*
                 * Expired route entries were already removed from the kernel
                 * routing table, but may remain in the RIP routing table
                 * if the garbage collection interval has not passed. 
                 * These entries are usually detectable by their infinite 
                 * metric, but all metrics are infinite in this case.
                 * However, the pointer to the kernel route will be
                 * NULL, so the following call will not attempt to
                 * delete a (non-existent) expired entry.
                 */

                if (rt->inKernel)
                    {
                    if (ripSystemRouteDelete (pDsin->sin_addr.s_addr, 
                                              pGsin->sin_addr.s_addr, 
                                              pNsin->sin_addr.s_addr, 
                                              0) == ERROR)
                        {
                        if (routedDebug) 
                            logMsg ("Error %x removing route from kernel table.\n", 
                                    errno, 0, 0, 0, 0, 0);
                        }
                    else
                        {
                        rt->inKernel = FALSE;
                        ripState.ripGlobal.rip2GlobalRouteChanges++;
                        }
                    }
                }

            /* Remove the entry from the RIP routing table. */

            ((struct rt_entry *)rh)->rt_forw = rt->rt_forw;
            free ((char *)rt);
            }
        }
    if (doinghost)
        {
        doinghost = 0;
        base = nethash;
        goto again;
        }
    return;
    }

/*
 * If we have an interface to the wide, wide world,
 * add an entry for an Internet default route (wildcard) to the internal
 * tables and advertise it.  This route is not added to the kernel routes,
 * but this entry prevents us from listening to other people's defaults
 * and installing them in the kernel here.
 */
void rtdefault (void)
    {
    rtadd ( (struct sockaddr *)&inet_default, (struct sockaddr *)&inet_default,
            1, RTS_CHANGED | RTS_PASSIVE | RTS_INTERNAL, NULL,
            M2_ipRouteProto_rip, 0, 0, NULL);
    }

void routedTableInit (void)
    {
    register struct rthash *rh;
    
    for (rh = nethash; rh < &nethash[ROUTEHASHSIZ]; rh++)
        rh->rt_forw = rh->rt_back = (struct rt_entry *)rh;
    for (rh = hosthash; rh < &hosthash[ROUTEHASHSIZ]; rh++)
        rh->rt_forw = rh->rt_back = (struct rt_entry *)rh;
    }

/******************************************************************************
*
* ripSystemRouteAdd - add a RIP route to the system routing database
*
* This routine adds the route to the system routing database using the
* method which the particular product supports.
*
* RETURNS: ERROR or OK (return value from selected add method)
*
* ERRNO: N/A
*
* NOMANUAL
*/

LOCAL STATUS ripSystemRouteAdd 
    (
    long             dstIp,    	/* destination address, network order */
    long             gateIp,    	/* gateway address, network order */
    long             mask,      	/* mask for destination, network order */
    int              flags      	/* route flags */ 
    )
    {
#ifdef ROUTER_STACK
    ROUTE_DESC		routeDesc;
    struct sockaddr_in	dstAddr;
    struct sockaddr_in	netmask;
    struct sockaddr_in	gateway;

    /* Initialize the routeDesc structure and the sockaddr structures */

    bzero ((char *)&routeDesc, sizeof (routeDesc));

    routeDesc.pDstAddr = (struct sockaddr *)&dstAddr;
    routeDesc.pNetmask = (struct sockaddr *)&netmask;
    routeDesc.pGateway = (struct sockaddr *)&gateway;

    bzero ((char *)&dstAddr, sizeof (struct sockaddr_in));
    bzero ((char *)&netmask, sizeof (struct sockaddr_in));
    bzero ((char *)&gateway, sizeof (struct sockaddr_in));

    dstAddr.sin_len = sizeof (struct sockaddr_in);
    netmask.sin_len = sizeof (struct sockaddr_in);
    gateway.sin_len = sizeof (struct sockaddr_in);

    dstAddr.sin_family = AF_INET;
    netmask.sin_family = AF_INET;
    gateway.sin_family = AF_INET;

    dstAddr.sin_addr.s_addr = dstIp;
    netmask.sin_addr.s_addr = mask;
    gateway.sin_addr.s_addr = gateIp;

    routeDesc.flags = flags;
    routeDesc.protoId = M2_ipRouteProto_rip;

    /* 
     * If it is a host route, set netmask to NULL. RIP internally
     * assigns a host route the netmask of the interface. But the system
     * expect a NULL or all zero's netmask.
     */

    if (flags & RTF_HOST)
        routeDesc.pNetmask = NULL;

    /* Now add the route */

    return (routeEntryAdd (&routeDesc));

#else
    /* 
     * If it is a host route, set netmask to NULL. RIP internally
     * assigns a host route the netmask of the interface. But the system
     * expect a NULL netmask
     */

    if (flags & RTF_HOST)
        mask = 0;

    return (mRouteEntryAdd (dstIp, gateIp, mask, 0, flags, 
                            M2_ipRouteProto_rip));
#endif /* ROUTER_STACK */
    }

/******************************************************************************
*
* ripSystemRouteDelete - delete a RIP route from the system routing database
*
* This routine deletes a route from the system routing database using the
* method which the particular product supports.
*
* RETURNS: ERROR or OK (return value from selected delete method)
*
* ERRNO: N/A
*
* NOMANUAL
*/

LOCAL STATUS ripSystemRouteDelete 
    (
    long		dstIp,    	/* destination address, network order */
    long		gateIp,    	/* gateway address, network order */
    long		mask,      	/* mask for destination, network order */
    int		flags      	/* route flags */ 
    )
    {
#ifdef ROUTER_STACK
    ROUTE_DESC		routeDesc;
    struct sockaddr_in	dstAddr;
    struct sockaddr_in	netmask;
    struct sockaddr_in	gateway;

    /* Initialize the routeDesc structure and the sockaddr structures */

    bzero ((char *)&routeDesc, sizeof (routeDesc));

    routeDesc.pDstAddr = (struct sockaddr *)&dstAddr;
    routeDesc.pNetmask = (struct sockaddr *)&netmask;
    routeDesc.pGateway = (struct sockaddr *)&gateway;

    bzero ((char *)&dstAddr, sizeof (struct sockaddr_in));
    bzero ((char *)&netmask, sizeof (struct sockaddr_in));
    bzero ((char *)&gateway, sizeof (struct sockaddr_in));

    dstAddr.sin_len = sizeof (struct sockaddr_in);
    netmask.sin_len = sizeof (struct sockaddr_in);
    gateway.sin_len = sizeof (struct sockaddr_in);

    dstAddr.sin_family = AF_INET;
    netmask.sin_family = AF_INET;
    gateway.sin_family = AF_INET;

    dstAddr.sin_addr.s_addr = dstIp;
    netmask.sin_addr.s_addr = mask;
    gateway.sin_addr.s_addr = gateIp;

    routeDesc.flags = flags;
    routeDesc.protoId = M2_ipRouteProto_rip;

    /* 
     * If it is a host route, set netmask to NULL. RIP internally
     * assigns a host route the netmask of the interface. But the system
     * expect a NULL or all zero's netmask.
     */

    if (flags & RTF_HOST)
        routeDesc.pNetmask = NULL;

    /* Now delete the route */

    return (routeEntryDelete (&routeDesc));
#else
    /* 
     * If it is a host route, set netmask to NULL. RIP internally
     * assigns a host route the netmask of the interface. But the system
     * expect a NULL or all zero's netmask.
     */

    if (flags & RTF_HOST)
        mask = 0;

    return (mRouteEntryDelete (dstIp, gateIp, mask, 0, flags, 
                               M2_ipRouteProto_rip));
#endif /* ROUTER_STACK */
    }


/******************************************************************************
*
* ripRouteMetricSet - Set the metric for the RIP route
*
* This routine changes the metric of the RIP route that is kept in the
* system Routing database.
* The parameter <pRtEntry> describes the RIP route that is kept in RIP's
* private database. It also contains the metric value that is to be set.
*
* This routine calls the routing extensions function routeMetricSet() 
* to set the metric
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*/

void ripRouteMetricSet 
    (
    struct rt_entry *	pRtEntry	/* Route entry describing the */
    					/* Kernel route to update */
    )
    {
    struct sockaddr_in * pDsin;
    struct sockaddr_in * pGsin;
    struct sockaddr_in * pNsin;
    
    /*
     * Retrieve the destination and netmask values from the
     * corresponding fields in the RIP route entry structure
     */

    ripRouteToAddrs (pRtEntry, &pDsin, &pGsin, &pNsin);

    /* 
     * If it is a host route, set netmask to NULL. RIP internally
     * assigns a host route the netmask of the interface. But the system
     * overrides that and stores the route as a host route with a NULL mask.
     * (which is the right thing to do). So we set the netmask field to
     * NULL so that the route lookup happens fine.
     */

    if (pRtEntry->rt_flags & RTF_HOST)
        pNsin = NULL;

    if (routedDebug > 2)
        {
        logMsg ("ripRouteMetricSet: setting new metric = %d for\n", 
                pRtEntry->rt_metric , 0, 0, 0, 0, 0);
        ripSockaddrPrint ((struct sockaddr *)pDsin);
        ripSockaddrPrint ((struct sockaddr *)pNsin);
        ripSockaddrPrint ((struct sockaddr *)pGsin);
        }

    /* Now set the route metric */

    if (routeMetricSet ((struct sockaddr *)pDsin, (struct sockaddr *)pNsin,
                        M2_ipRouteProto_rip, pRtEntry->rt_metric) == ERROR) 
        {
        if (routedDebug) 
            logMsg ("Couldn't set metric for rtEntry = %x.\n", (int)pRtEntry, 0, 
                    0, 0, 0, 0);
        }
    }

/*****************************************************************************
*
* ripInsque - insert node in list after specified node.
*
* Portable version of _insque ().
*
* NOMANUAL
*/
 
LOCAL void ripInsque
    (
    NODE *pNode,
    NODE *pPrev
    )
    {
    NODE *pNext;
 
    pNext = pPrev->next;
    pPrev->next = pNode;
    pNext->previous = pNode;
    pNode->next = pNext;
    pNode->previous = pPrev;
    }
 
/*****************************************************************************
*
* ripRemque - remove specified node in list.
*
* Portable version of _remque ().
*
* NOMANUAL
*/
 
LOCAL void ripRemque 
    (
    NODE *	pNode	/* Node to remove */
    )
    {
    pNode->previous->next = pNode->next;
    pNode->next->previous = pNode->previous;
    }
