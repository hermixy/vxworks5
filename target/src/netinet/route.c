/* route.c - packet routing routines */

/* Copyright 1984 - 2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
 * Copyright (c) 1980, 1986, 1991, 1993
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
 *	@(#)route.c	8.3 (Berkeley) 1/9/95
 */

/*
modification history
--------------------
01s,14mar02,vvv  fixed route addition check for dest=gateway in rt_setgate
		 (SPR #74244)
01r,24jan02,niq  fixed call to new address message hook (SPR #71670)
01q,03jan02,vvv  added check for rt_setgate return value in rtredirect 
		 (SPR #71518)
01p,05dec01,vvv  fixed NT simulator exception (SPR #71659)
01o,12oct01,rae  merge from truestack ver 02e, base 01l (SPR #69683 etc.)
01n,20feb01,ijm  corrected additional bug in rtalloc1.  Precedence bits
		 must be cleared before searching the table.
01m,02nov00,ijm  fixed rtalloc1 TOS bug, SPR# 30195
01l,04mar99,spm  fixed errors in SPR #23301 fix causing build failures
01k,02mar99,spm  eliminated EEXIST error caused by ARP entries (SPR #23301)
01j,26aug98,n_s  added return val check for mBufClGet in rtinit. spr #22238.
01i,07jul97,rjc  newer version of rtentry incorporated.
01h,01jul97,vin  added route socket message hooks for scalability, fixed
		 warnings.
01g,30jun97,rjc  restored old rtentry size.
01f,03jun97,rjc  netmask with RTF_HOST set to 0.
01e,13feb97,rjc  more changes for tos routing,
01e,05feb97,rjc  changes for tos routing,
01d,05dec96,vin  moved ifafree() to if.c
01c,22nov96,vin  added cluster support replaced m_get(..) with mBufClGet(..).
01b,24sep96,vin  rtioctl() fixed for multicast addresses.
01a,03mar96,vin  created from BSD4.4 stuff. added rtioctl for backward
		 compatibility. moved ifa_ifwithroute() to if.c.
*/

#include "vxWorks.h"
#include "logLib.h"
#include "sysLib.h"
#include "wdLib.h"
#include "net/mbuf.h"
#include "net/domain.h"
#include "net/protosw.h"
#include "sys/socket.h"
#include "sys/ioctl.h"
#include "net/socketvar.h"
#include "errno.h"

#include "net/if.h"
#include "net/if_types.h"
#include "net/route.h"
#include "net/systm.h"
#include "net/raw_cb.h"

#include "routeEnhLib.h"

#include "netinet/in.h"
#include "netinet/in_var.h"

#ifdef VIRTUAL_STACK
#include "netinet/vsLib.h"
#endif

#ifdef NS
#include <netns/ns.h>
#endif

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET
#include "wvNetLib.h"
#endif
#endif


#ifdef RTALLOC_DEBUG
#include "stdio.h"
#include "net/if_dl.h"
#include "inetLib.h"
LOCAL int routeEntryDebugShow (struct radix_node * newrt);
#endif /* RTALLOC_DEBUG */

/* externs */
int	tickGet();

#define	SA(p) ((struct sockaddr *)(p))
#define SOCKADDR_IN(s) (((struct sockaddr_in*)(s))->sin_addr.s_addr)

#define ROUTE_KILL_FREQUENCY 600    /* Check for old routes every 10 minutes */
#define ROUTE_PEND_MAXIMUM  3600    /* Remove old routes after 1 hour (max). */
#define ROUTE_PEND_MINIMUM  10      /* 10 second minimum for old routes. */

#ifndef VIRTUAL_STACK
struct	rtstat	rtstat;
struct	radix_node_head *rt_tables[AF_MAX+1];
#endif

struct rtqk_arg
    {
    struct radix_node_head *rnh;
    int draining;
    int killed;
    int found;
    int updating;
    ULONG nextstop;
    };

#ifndef VIRTUAL_STACK
int	rttrash;		/* routes not in table but not freed */
int     rtmodified = 0; 	/* route table modified */
struct	sockaddr wildcard; 	/* zero valued cookie for wildcard searches */

LOCAL WDOG_ID routeKillTimer; 	/* Schedules removal of expired routes. */
LOCAL ULONG routeKillInterval; 	/* Removal frequency, in ticks. */

LOCAL int routePendInterval; 	/* Time to keep unused routes, in ticks. */
LOCAL int routePendMinimum; 	/* Minimum preservation time. */

LOCAL int routePendLimit = 128;    /*
                                    * Number of unused routes before
                                    * reduced removal time.
                                    */

LOCAL int routeExpireChange = 0;
                                   /* Most recent removal time adjustment. */
#endif

struct radix_node * routeSwap (struct radix_node_head *, struct rtentry *,
                               struct sockaddr *, struct sockaddr *);

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET
    /* Set common fields of event identifiers for this module. */

LOCAL UCHAR wvNetModuleId = WV_NET_ROUTE_MODULE;   /* Value for route.c */
LOCAL UCHAR wvNetLocalFilter = WV_NET_NONE;     /* Available event filter */

LOCAL ULONG wvNetEventId;       /* Event identifier: see wvNetLib.h */
#endif    /* INCLUDE_WVNET */
#endif

LOCAL void routeKillCheck (void);
LOCAL int routeUpdate (struct radix_node *, void *);

IMPORT STATUS   netJobAdd (FUNCPTR routine, int param1, int param2, int param3,
                           int param4, int param5);

#ifndef VIRTUAL_STACK
/*
 * The routeStorageCreate routine replaces this version with an implementation
 * to initialize radix trees for a virtual stack.
 */

void
rtable_init(table)
	void **table;
{
	struct domain *dom;
	for (dom = domains; dom; dom = dom->dom_next)
		if (dom->dom_rtattach)
			dom->dom_rtattach(&table[dom->dom_family],
			    dom->dom_rtoffset);
}
#endif

#ifdef VIRTUAL_STACK
/*******************************************************************************
*
* routeStorageCreate - initialize the routing database
*
* This routine sets up the radix tree and initializes the routing storage
* for addresses in the AF_INET family. It mimics the more generic route_init
* routine which uses the domains list to create storage for any address
* type. That original routine can be compiled and used if support for
* other domains is needed.
*
* INTERNAL
* Currently, this routine assumes that the caller has assigned the
* correct virtual stack number allowing creation of isolated routing storage.
* That process could occur internally (with the stack number as an argument)
* if that approach is more convenient.
*
* RETURNS: OK, or ERROR if initialization failed.
*/

STATUS routeStorageCreate (void)
    {
    VS_RADIX * pGlobals;
    STATUS result;

    pGlobals = malloc (sizeof (VS_RADIX));
    if (pGlobals == NULL)
        return (ERROR);

    bzero ( (char *)pGlobals, sizeof (VS_RADIX));

    vsTbl [myStackNum]->pRadixGlobals = pGlobals;

    /*
     * The radixInit() routine replaces the rn_init() routine to avoid access
     * to the (currently non-virtualized) "domains" global.
     */

    result = radixInit (); /* initialize all zeroes, all ones, mask table */
    if (result == ERROR)
        {
        free (pGlobals);
        return (ERROR);
        }

   /*
    * Setup any routing tables required for the available domains. 
    * Originally, this initialization routine executed indirectly
    * through the rtable_init() routine which searched the domains
    * list. This process skips that search since the only applicable
    * entry involves AF_INET addresses with an offset of 27 bits
    * (originally 32). The original offset skips the length, family,
    * and port fields in a sockaddr_in structure. The adjusted offset
    * examines five additional bits to incorporate TOS values into
    * routing lookups.
    *
    * NOTE: The original route_init() routine (if compiled and used)
    * will use the rtable_init() routine to complete the routing
    * storage setup for other domains, if that capability is necessary.
    */

    result = rn_inithead (&rt_tables[AF_INET], 27);
    if (result == 0)
        {
        /* Tree creation failed: release allocated memory and return. */

        Free (rn_zeros);
        Free (mask_rnhead);

        free (pGlobals);

        return (ERROR);
        }

    routeKillTimer = wdCreate ();
    if (routeKillTimer == NULL)
        {
        Free (rn_zeros);
        Free (mask_rnhead);
        Free (rt_tables[AF_INET]);

        free (pGlobals);

        return (ERROR);
        }

    routeKillInterval = sysClkRateGet() * ROUTE_KILL_FREQUENCY;
    routePendInterval = sysClkRateGet() * ROUTE_PEND_MAXIMUM;
    routePendMinimum = sysClkRateGet() * ROUTE_PEND_MINIMUM;

    wdStart (routeKillTimer, routeKillInterval, netJobAdd,
             (int)routeKillCheck);

    return (OK);
    }
#else
/*
 * The routeStorageCreate routine replaces this version with an implementation
 * which creates an isolated set of (previously global) variables for a single
 * virtual stack.
 */

int
route_init()
{
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_VERBOSE event */
    WV_NET_MARKER_0 (NET_AUX_EVENT, WV_NET_VERBOSE, 33, 13,
                     WV_NETEVENT_ROUTEINIT_START)
#endif  /* INCLUDE_WVNET */
#endif

	rn_init();	/* initialize all zeroes, all ones, mask table */
	rtable_init((void **)rt_tables);

        routeKillTimer = wdCreate ();
        if (routeKillTimer == NULL)
            return (ERROR);

        routeKillInterval = sysClkRateGet() * ROUTE_KILL_FREQUENCY;
        routePendInterval = sysClkRateGet() * ROUTE_PEND_MAXIMUM;
        routePendMinimum = sysClkRateGet() * ROUTE_PEND_MINIMUM;

        wdStart (routeKillTimer, routeKillInterval, netJobAdd, (int) routeKillCheck);

	return (OK);
}
#endif

/*
 * Look for cloned routes to remote destinations which are no longer in
 * use. Remove any expired route entries.
 */

void routeKillCheck (void)
    {
    struct radix_node_head *pHead;
    struct rtqk_arg   count;
    int s;

    ULONG now;

    pHead = rt_tables [AF_INET];

    if (pHead == NULL)    /* Missing routing table. */
        return;

    count.found = count.killed = 0;
    count.rnh = pHead;
    count.nextstop = tickGet() + routeKillInterval;
    count.draining = count.updating = 0;

    /* Remove any old (currently unused) routes which expired. */

    s = splnet ();
    rn_walktree (pHead, routeUpdate, &count);
    splx (s);

    if (count.found - count.killed > routePendLimit)
        {
        /*
         * The routing table still contains excessive unused routes.
         * Reduce the expiration time and try to remove more entries.
         */

        now = tickGet ();
        if ( (now - routeExpireChange >= routeKillInterval) &&
            routePendInterval > routePendMinimum)
            {
            routePendInterval = 2 * routePendInterval / 3;
            if (routePendInterval < routePendMinimum)
                routePendInterval = routePendMinimum;

            routeExpireChange = now;

            /*
             * Reset the expiration time for old (currently unused) routes
             * to the new (smaller) value. Also update the "nextstop" field
             * to the nearest (new) expiration time.
             */

            count.found = count.killed = 0;
            count.updating = 1;

            s = splnet ();
            rn_walktree (pHead, routeUpdate, &count);
            splx (s);
            }
        }

    /*
     * The routeUpdate() routine may reduce the "nextstop" value to the
     * nearest expiration time of a pending route. Set the timer to 
     * restart at that point, or after the default interval if no old routes
     * are found.
     */

    now = count.nextstop - tickGet ();
    wdStart (routeKillTimer, now, netJobAdd, (int)routeKillCheck); 

    return;
    }

/*
 * This routine performs garbage collection on unused route entries.
 * The system clones those routes (from entries to remote destinations)
 * to store the path MTU results. 
 *
 * The <pArg> argument selects the operation to perform. Normally, both
 * the "draining" and "updating" fields are 0. In that case, this routine
 * completes the deletion of (unused) cloned entries triggered when rtfree()
 * set the RTF_DELETE flag. Since the RTF_DELETE flag and RTF_ANNOUNCE flags
 * have the same value, this routine also tests the RTF_STATIC flag to prevent
 * the removal of manually added proxy ARP entries.
 *
 * If too many routes exist, the "updating" flag reduces the lifetime of
 * appropriate entries. The "draining" flag removes all entries immediately
 * due to severe memory limitations.
 * 
 * As a side effect, this routine resets the "nextstop" field to the 
 * nearest expiration time of a route entry waiting for deletion, if any.
 */
      
LOCAL int routeUpdate
    (
    struct radix_node * pNode,
    void * pArg
    )
    {
    struct rtqk_arg * pCount = pArg;
    struct rtentry * pRoute = (struct rtentry *)pNode;
    int error;
    ULONG now;

    now = tickGet();

    if ( (pRoute->rt_flags & RTF_DELETE) && !(pRoute->rt_flags & RTF_STATIC))
        {
        pCount->found++;

        if (pCount->draining || pRoute->rt_rmx.rmx_expire <= now)
            {
            if (pRoute->rt_refcnt > 0)
                panic ("routeKill removing route in use?");

            error = rtrequest (RTM_DELETE, (struct sockaddr *)rt_key (pRoute),
                               pRoute->rt_gateway, rt_mask (pRoute),
                               pRoute->rt_flags, 0);
            if (!error)
                pCount->killed++;
            }
        else
            {
            if (pCount->updating &&
                (pRoute->rt_rmx.rmx_expire - now > routePendInterval))
                pRoute->rt_rmx.rmx_expire = now + routePendInterval;

            if (pRoute->rt_rmx.rmx_expire < pCount->nextstop)
                pCount->nextstop = pRoute->rt_rmx.rmx_expire;
            }
        }
    return (0);
    }

    /*
     * Remove all unused routes cloned from entries to remote destinations,
     * regardless of expiration time.
     */

void routeDrain (void)
    {
    struct radix_node_head *pHead;
    struct rtqk_arg   count;
    int s;

    pHead = rt_tables [AF_INET];

    if (pHead == NULL)    /* Missing routing table. */
        return;

    count.found = count.killed = 0;
    count.rnh = pHead;
    count.nextstop = 0;
    count.draining = 1;
    count.updating = 0;

    s = splnet ();
    rn_walktree (pHead, routeUpdate, &count);
    splx (s);
    }

/*
 * Packet routing routines.
 */
void
rtalloc(ro)
	register struct route *ro;
{
	if (ro->ro_rt && ro->ro_rt->rt_ifp && (ro->ro_rt->rt_flags & RTF_UP))
		return;				 /* XXX */
	ro->ro_rt = rtalloc1(&ro->ro_dst, 1);
}

struct rtentry *
rtalloc1(dst, report)
	register struct sockaddr *dst;
	int report;
{
	register struct radix_node_head *rnh = rt_tables[dst->sa_family];
	register struct rtentry *rt;
	register struct radix_node *rn;
	struct rtentry *newrt = 0;
	struct rt_addrinfo info;
	u_long tosRtMask;	/* TOS route mask */
	u_long newRtMask;	/* TOS 0 route mask */
	int  s = splnet(), err = 0, msgtype = RTM_MISS;
	struct rtentry *   tosRt = NULL;
        u_char     savedTos;

      /* 
       * save original tos since we overwrite it temporarily in the 
       * dst socketaddr
       */

       savedTos = TOS_GET (dst);

       /*
	* The Type of Service octet consists of three fields:
	*
	*       0     1     2     3     4     5     6     7
	*     +-----+-----+-----+-----+-----+-----+-----+-----+
	*     |                 |                       |     |
	*     |   PRECEDENCE    |          TOS          | MBZ |
	*     |                 |                       |     |
	*     +-----+-----+-----+-----+-----+-----+-----+-----+
	*
	* Precedence bits should not be considered when searching
	* the table.  An existing matching route would not be found
	* and if a new route were created, since dst is copied to 
	* generate the key, precedence bits would be part
	* of the new route's TOS field which is wrong.  This
	* would create a leak if this wrong route were created
	* for a local address.  ARP would hold on to a packet,
	* while it resolves the local address.  When the reply
	* comes back, since ARP does not look for routes with
	* TOS set, the packet would never be sent.
	*
	*/

	TOS_SET(dst, savedTos & 0x1f);

match:

	if (rnh && (rn = rnh->rnh_matchaddr((caddr_t)dst, rnh)) &&
	    ((rn->rn_flags & RNF_ROOT) == 0)) {
		newrt = rt = (struct rtentry *)rn;
                if ( (dst->sa_family == AF_INET) && (rt->rt_refcnt == 0) &&
                    (rt->rt_flags & RTF_DELETE) && !(rt->rt_flags & RTF_STATIC))
                    {
                    /*
                     * Reusing a (cloned) route to a remote destination.
                     * Remove deletion tag and reset timer.
                     *
                     * NOTE: All manually added proxy ARP entries include
                     * the RTF_ANNOUNCE flag which uses the same value as
                     * the RTF_DELETE flag. The preceding test for RTF_STATIC
                     * preserves the flag settings for those ARP entries,
                     * even though the removal of RTF_ANNOUNCE should be
                     * harmless since its only purpose is to trigger the
                     * gratuitous ARP request when the entry is created.
                     */

                    rt->rt_flags &= ~RTF_DELETE;
                    rt->rt_rmx.rmx_expire = 0;
                    }

		if (report && (rt->rt_flags & RTF_CLONING)) {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_INFO event */
            WV_NET_MARKER_2 (NET_AUX_EVENT, WV_NET_INFO, 17, 9,
                             WV_NETEVENT_RTALLOC_CLONE,
                         ((struct sockaddr_in *)rt_key (rt))->sin_addr.s_addr, 
                         ((struct sockaddr_in *)dst)->sin_addr.s_addr)
#endif  /* INCLUDE_WVNET */
#endif

			err = rtrequest(RTM_RESOLVE, dst, SA(0),
					      SA(0), 0, &newrt);
			if (err) {
				newrt = rt;
				rt->rt_refcnt++;
				goto miss;
			}
			if ((rt = newrt) && (rt->rt_flags & RTF_XRESOLVE)) {
				msgtype = RTM_RESOLVE;
				goto miss;
			}
		} else
                      {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_INFO event */
        WV_NET_DSTADDROUT_MARKER_1 (NET_AUX_EVENT, WV_NET_INFO, 18, 10,
                               ((struct sockaddr_in *)dst)->sin_addr.s_addr,
                                    WV_NETEVENT_RTALLOC_SEARCHGOOD,
                               ((struct sockaddr_in *)dst)->sin_addr.s_addr)
#endif  /* INCLUDE_WVNET */
#endif

                      rt->rt_refcnt++;
                      }
	} else {
		rtstat.rts_unreach++;
	miss:
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_CRITICAL event */
        WV_NET_MARKER_1 (NET_AUX_EVENT, WV_NET_CRITICAL, 26, 5,
                         WV_NETEVENT_RTALLOC_SEARCHFAIL,
                         ((struct sockaddr_in *)dst)->sin_addr.s_addr)
#endif  /* INCLUDE_WVNET */
#endif

                if (report) {
			bzero((caddr_t)&info, sizeof(info));
			info.rti_info[RTAX_DST] = dst;
                        if (rtMissMsgHook)
                            (*rtMissMsgHook) (msgtype, &info, 0, err);
		}
	}

        /*
         *  RFC 1583 Section 11.1:
         *
         * "Select the routing table entry whose value matches the TOS found
         *  in the packet header.  If there is no routing table entry for
         *  this TOS, select the routing table entry for TOS 0.  In other
         *  words, packets requesting TOS X are routed along the TOS 0
         *  path if a TOS X path does not exist."
	 *  See also RFC 1349, Appendix B.4.
         */

        if (TOS_GET (dst) != 0)
            {
            TOS_SET (dst, 0);
            tosRt = newrt;
            newrt = NULL;
            goto match;
            }

        /*
         * tosRt is the TOS route match, if any. If none exists, tosRt is the
         * default route ("0.0.0.0"), if any.  Otherwise, tosRt is NULL.
         * newrt is the TOS 0 route.
         */

        if (tosRt != NULL)
            {
            if (newrt != NULL)
                {
                /*
                 * Host route entries created by ARP have null masks with
                 * implied mask = 0xffffffff
                 */

                tosRtMask =  (rt_mask (tosRt) == NULL) ? 0xffffffff :
                    ((struct sockaddr_in *) rt_mask (tosRt))->sin_addr.s_addr;
#ifdef RTALLOC_DEBUG
                printf("\nBEST MATCHING TOS ROUTE:\n"); 
                routeEntryDebugShow ((struct radix_node *)tosRt);
                printf("\nTOS 0 ROUTE:\n"); 
                routeEntryDebugShow ((struct radix_node *)newrt);
#endif /* RTALLOC_DEBUG */

                newRtMask =  (rt_mask (newrt) == NULL) ? 0xffffffff :
                    ((struct sockaddr_in *) rt_mask (newrt))->sin_addr.s_addr;

                /*
                 * select the route with longest netmask. we assume
                 * contiguous masks starting at the same bit position.
                 * The default route has a mask = 0x00000000
                 */

                if (tosRtMask >= newRtMask)
                    {
                    RTFREE (newrt);
                    newrt = tosRt;
                    }
                else
                    {
                    /* newrt is more specific: keep it */
                    RTFREE (tosRt);
                   }
                }
            else /* newrt is NULL */
                {
                /*
                 * Restore previously found TOS route.  Can happen if there is a
                 * route entered with TOS set, but no default route exists nor
                 * route match with 0 TOS.
                 */
                newrt = tosRt;
                }
            }

#ifdef RTALLOC_DEBUG
	if (savedTos && newrt != NULL)
	    {
	    printf ("\nSELECTED ROUTE:\n");
	    routeEntryDebugShow ((struct radix_node *)newrt);
	    }
#endif /* RTALLOC_DEBUG */

       /* Restore the saved TOS. */ 

       TOS_SET (dst, savedTos);

       splx(s);

       return (newrt);
}

    /*
     * This version of the route search routine is specifically designed
     * for retrieving a route when forwarding packets. Unlike the original
     * case, this route search never clones routes to remote destinations.
     * Those cloned routes are necessary for path MTU discovery, but should
     * only exist when the Internet host originates the traffic, not when
     * it forwards it as a router.
     */

struct rtentry *
rtalloc2(dst)
	register struct sockaddr *dst;
{
	register struct radix_node_head *rnh = rt_tables[dst->sa_family];
	register struct rtentry *rt;
	register struct radix_node *rn;
	struct rtentry *newrt = 0;
	struct rt_addrinfo info;
	int  s = splnet(), err = 0, msgtype = RTM_MISS;
	struct rtentry *   tosRt = NULL;
        u_long tosRtMask;       /* TOS route mask */
        u_long newRtMask;       /* TOS 0 route mask */
        u_char     savedTos;

      /* 
       * save original tos since we overwrite it temporarily in the 
       * dst socketaddr
       */

       savedTos = TOS_GET (dst);

match:

	if (rnh && (rn = rnh->rnh_matchaddr((caddr_t)dst, rnh)) &&
	    ((rn->rn_flags & RNF_ROOT) == 0)) {
		newrt = rt = (struct rtentry *)rn;
                if ( (dst->sa_family == AF_INET) && (rt->rt_refcnt == 0) &&
                    (rt->rt_flags & RTF_DELETE) && !(rt->rt_flags & RTF_STATIC))
                    {
                    /*
                     * Reusing a (cloned) route to a remote destination.
                     * Remove deletion tag and reset timer.
                     *
                     * NOTE: All manually added proxy ARP entries include
                     * the RTF_ANNOUNCE flag which uses the same value as
                     * the RTF_DELETE flag. The preceding test for RTF_STATIC
                     * preserves the flag settings for those ARP entries,
                     * even though the removal of RTF_ANNOUNCE should be
                     * harmless since its only purpose is to trigger the
                     * gratuitous ARP request when the entry is created.
                     */

                    rt->rt_flags &= ~RTF_DELETE;
                    rt->rt_rmx.rmx_expire = 0;
                    }

                /*
                 * Disable cloning when forwarding data to a remote
                 * destination. This search will still create a 
                 * cloned entry if the destination is directly reachable
                 * to hold the ARP information for the target host.
                 */

		if ( !(rt->rt_flags & RTF_GATEWAY) &&
                    (rt->rt_flags & RTF_CLONING)) {
			err = rtrequest(RTM_RESOLVE, dst, SA(0),
					      SA(0), 0, &newrt);
			if (err) {
				newrt = rt;
				rt->rt_refcnt++;
				goto miss;
			}
			if ((rt = newrt) && (rt->rt_flags & RTF_XRESOLVE)) {
				msgtype = RTM_RESOLVE;
				goto miss;
			}
		} else
			rt->rt_refcnt++;
	} else {
		rtstat.rts_unreach++;
	miss:
		bzero((caddr_t)&info, sizeof(info));
		info.rti_info[RTAX_DST] = dst;
                if (rtMissMsgHook)
                    (*rtMissMsgHook) (msgtype, &info, 0, err);
	}

    /*
     *  RFC 1583 Section 11.1:
     *
     * "Select the routing table entry whose value matches the TOS found
     *  in the packet header.  If there is no routing table entry for
     *  this TOS, select the routing table entry for TOS 0.  In other
     *  words, packets requesting TOS X are routed along the TOS 0
     *  path if a TOS X path does not exist."
     *  See also RFC 1349, Appendix B.4.
     */

    if (TOS_GET (dst) != 0)   
        {
        TOS_SET (dst, 0);
        tosRt = newrt;
        newrt = NULL;
        goto match;
        }

    /*
     * tosRt is the TOS route match, if any. If none exists, tosRt is the
     * default route ("0.0.0.0"), if any.  Otherwise, tosRt is NULL.
     * newrt is the TOS 0 route.
     */

    if (tosRt != NULL)
        {
        if (newrt != NULL)
            {
            /*
             * Host route entries created by ARP have null masks with
             * implied mask = 0xffffffff
             */

            tosRtMask =  (rt_mask (tosRt) == NULL) ? 0xffffffff :
                ((struct sockaddr_in *) rt_mask (tosRt))->sin_addr.s_addr;

            newRtMask =  (rt_mask (newrt) == NULL) ? 0xffffffff :
                ((struct sockaddr_in *) rt_mask (newrt))->sin_addr.s_addr;

            /*
             * select the route with longest netmask. we assume
             * contiguous masks starting at the same bit position.
             * The default route has a mask = 0x00000000
             */

            if (tosRtMask >= newRtMask)
                {
                RTFREE (newrt);
                newrt = tosRt;
                }
            else
                {
                /* newrt is more specific: keep it */
                RTFREE (tosRt);
               }
            }
        else /* newrt is NULL */
            {
            /*
             * Restore previously found TOS route.  Can happen if there is a
             * route entered with TOS set, but no default route exists nor
             * route match with 0 TOS.
             */

            newrt = tosRt;
            }
        }

       /* 
        * restore the saved tos. is redundant but harmless in case tos
        * was default to  start with.
        */

        TOS_SET (dst, savedTos);

        splx(s);

        return (newrt);
    }

void
rtfree(rt)
	register struct rtentry *rt;
{
	register struct ifaddr *ifa;

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_INFO event */
    if (rt)
        {
        WV_NET_MARKER_2 (NET_AUX_EVENT, WV_NET_INFO, 19, 11,
                         WV_NETEVENT_RTFREE_START,
                         ((struct sockaddr_in *)rt_key (rt))->sin_addr.s_addr,
                         rt->rt_refcnt)
        }
    else
        {
        WV_NET_MARKER_2 (NET_AUX_EVENT, WV_NET_INFO, 19, 11,
                         WV_NETEVENT_RTFREE_START, 0, 0)
        }
#endif  /* INCLUDE_WVNET */
#endif

	if (rt == 0)
            {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_EMERGENCY event */
            WV_NET_MARKER_0 (NET_AUX_EVENT, WV_NET_EMERGENCY, 21, 1,
                     WV_NETEVENT_RTFREE_PANIC)
#endif  /* INCLUDE_WVNET */
#endif

            panic("rtfree");
            }
	rt->rt_refcnt--;

        if (rt_key(rt)->sa_family == AF_INET)
            {
            /*
             * If route was cloned for path MTU results, mark for later
             * removal (if not reused) instead of deleting immediately.
             */

            if ( (rt->rt_refcnt == 0) && (rt->rt_flags & RTF_UP) &&
                (rt->rt_flags & RTF_HOST) && !(rt->rt_flags & RTF_LLINFO) &&
                (rt->rt_flags & RTF_CLONED))
                {
                rt->rt_flags |= RTF_DELETE;

                if (rt->rt_rmx.rmx_expire == 0)    /* Not yet assigned. */
                    rt->rt_rmx.rmx_expire = tickGet() + routePendInterval;
                }
            }

	if (rt->rt_refcnt <= 0 && (rt->rt_flags & RTF_UP) == 0) {
		if (rt->rt_nodes->rn_flags & (RNF_ACTIVE | RNF_ROOT))
                    {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_EMERGENCY event */
                    WV_NET_MARKER_0 (NET_AUX_EVENT, WV_NET_EMERGENCY, 21, 1,
                                     WV_NETEVENT_RTFREE_PANIC)
#endif  /* INCLUDE_WVNET */
#endif

                    panic ("rtfree 2");
                    }
		rttrash--;
		if (rt->rt_refcnt < 0) {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_ALERT event */
                    WV_NET_MARKER_0 (NET_AUX_EVENT, WV_NET_ALERT, 8, 4,
                                     WV_NETEVENT_RTFREE_BADREFCNT)
#endif  /* INCLUDE_WVNET */
#endif

                    logMsg ("rtfree: %x not freed (neg refs)\n", (int)rt,
                            0, 0, 0, 0, 0);
                    return;
                    }

		ifa = rt->rt_ifa;
		IFAFREE(ifa);
                if (rt->rt_parent)
                    {
                    RTFREE (rt->rt_parent)
                    }
		Free(rt_key(rt));

#ifdef ROUTER_STACK
                /*
                 * Entries which are not directly attached to the
                 * tree use a different free routine since the netmask
                 * information is stored in a uniquely allocated buffer,
                 * not shared among multiple route entries.
                 */

		if ( ((ROUTE_ENTRY*)rt)->primaryRouteFlag == FALSE)
		    routeEntryFree ((ROUTE_ENTRY*)rt, FALSE);
                else
		    Free ((ROUTE_ENTRY*)rt);
#else
                Free (rt);
#endif /* ROUTER_STACK */
	}
}

/*
 * Force a routing table entry to the specified
 * destination to go through the given gateway.
 * Normally called as a result of a routing redirect
 * message from the network layer.
 *
 * N.B.: must be called at splnet
 *
 */
int
rtredirect(dst, gateway, netmask, flags, src, rtp)
	struct sockaddr *dst, *gateway, *netmask, *src;
	int flags;
	struct rtentry **rtp;
{
	register struct rtentry *rt;
	int error = 0;
	short *stat = 0;
	struct rt_addrinfo info;
	struct ifaddr *ifa;

	/* verify the gateway is directly reachable */
	if ((ifa = ifa_ifwithnet(gateway)) == 0) {
		error = ENETUNREACH;
		goto out;
	}
	rt = rtalloc1(dst, 0);
	/*
	 * If the redirect isn't from our current router for this dst,
	 * it's either old or wrong.  If it redirects us to ourselves,
	 * we have a routing loop, perhaps as a result of an interface
	 * going down recently.
	 */
#define	equal(a1, a2) (bcmp((caddr_t)(a1), (caddr_t)(a2), (a1)->sa_len) == 0)
	if (!(flags & RTF_DONE) && rt &&
	     (!equal(src, rt->rt_gateway) || rt->rt_ifa != ifa))
		error = EINVAL;
	else if (ifa_ifwithaddr(gateway))
		error = EHOSTUNREACH;
	if (error)
		goto done;
	/*
	 * Create a new entry if we just got back a wildcard entry
	 * or the the lookup failed.  This is necessary for hosts
	 * which use routing redirects generated by smart gateways
	 * to dynamically build the routing tables.
	 */
	if ((rt == 0) || (rt_mask(rt) && rt_mask(rt)->sa_len < 2))
		goto create;
	/*
	 * Don't listen to the redirect if it's
	 * for a route to an interface. 
	 */
	if (rt->rt_flags & RTF_GATEWAY) {
		if (((rt->rt_flags & RTF_HOST) == 0) && (flags & RTF_HOST)) {
			/*
			 * Changing from route to net => route to host.
			 * Create new route, rather than smashing route to net.
			 */
		create:
			flags |=  RTF_GATEWAY | RTF_DYNAMIC;

                        /*
                         * Create the new route entry using the default
                         * weight. Do not report the change since the
                         * later hook generates both types of messages.
                         */

                        error = rtrequestAddEqui (dst, netmask, gateway, flags,
                                                  M2_ipRouteProto_icmp, 0,
                                                  FALSE, FALSE, NULL);

			/* error = rtrequest((int)RTM_ADD, dst, gateway,
				    netmask, flags,
				    (struct rtentry **)0);
                        */
			stat = &rtstat.rts_dynamic;
		} else {
			/*
			 * Smash the current notion of the gateway to
			 * this destination.  Should check about netmask!!!
			 */

			if (!(error = rt_setgate (rt, rt_key(rt), gateway)))
			    {
			    rt->rt_flags |= RTF_MODIFIED;
			    flags |= RTF_MODIFIED;
			    stat = &rtstat.rts_newgateway;
			    rt->rt_mod = tickGet(); 	    /* last modified */
			    rtmodified++; 
			    }
		}
	} else
		error = EHOSTUNREACH;
done:
	if (rt) {
		if (rtp && !error)
			*rtp = rt;
		else
			rtfree(rt);
	}
out:
	if (error)
            {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_CRITICAL event */
            WV_NET_MARKER_2 (NET_AUX_EVENT, WV_NET_CRITICAL, 27, 6,
                             WV_NETEVENT_RTRESET_BADDEST,
                             error, 
                             ((struct sockaddr_in *)gateway)->sin_addr.s_addr)
#endif  /* INCLUDE_WVNET */
#endif

            rtstat.rts_badredirect++;
            }
	else if (stat != NULL)
		(*stat)++;
	bzero((caddr_t)&info, sizeof(info));
	info.rti_info[RTAX_DST] = dst;
	info.rti_info[RTAX_GATEWAY] = gateway;
	info.rti_info[RTAX_NETMASK] = netmask;
	info.rti_info[RTAX_AUTHOR] = src;
        if (rtMissMsgHook)
            (*rtMissMsgHook) (RTM_REDIRECT, &info, flags, error);

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_VERBOSE event */
        WV_NET_MARKER_4 (NET_AUX_EVENT, WV_NET_VERBOSE, 34, 14,
                         WV_NETEVENT_RTRESET_STATUS,
                         error, 
                         ((struct sockaddr_in *)dst)->sin_addr.s_addr, 
                         ((struct sockaddr_in *)gateway)->sin_addr.s_addr, 
                         ((struct sockaddr_in *)netmask)->sin_addr.s_addr)
#endif  /* INCLUDE_WVNET */
#endif

	return (error); 
}

/*
* Routing table ioctl interface.
*
* WRS MODS support for this function is being for backward compatibility.
* This function has be moved else where because the routing code has 
* nothing to do with internet specific addresses since routing is independent
* of the sockaddress family.
*/
int
rtioctl(req, data)
	int req;
	caddr_t data;
{
	struct ortentry * pORE = NULL;
	struct sockaddr netMask;
	struct sockaddr * pNetMask = &netMask; 
	register u_long i; 
	register u_long net;
	register struct in_ifaddr *ia;

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_INFO event */
    WV_NET_MARKER_1 (NET_AUX_EVENT, WV_NET_INFO, 20, 12,
                     WV_NETEVENT_RTIOCTL_START, req)
#endif  /* INCLUDE_WVNET */
#endif

        if (req != SIOCADDRT && req != SIOCDELRT)
            {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_ERROR event */
            WV_NET_MARKER_1 (NET_AUX_EVENT, WV_NET_ERROR, 37, 8,
                             WV_NETEVENT_RTIOCTL_BADREQ, req)
#endif  /* INCLUDE_WVNET */
#endif

            return (EINVAL);
            }

	pORE = (struct ortentry *)data;

	/* BSD4.3 to BSD4.4 conversion */

	if (req == SIOCADDRT)
	    req = RTM_ADD;
	else
	    req = RTM_DELETE;

	/* 
	 * Set the netmask to the netmask of the interface address.
	 * This has to be removed if provision is made with routeAdd
	 * to add the network mask. 
	 */
	bzero ((caddr_t)&netMask, sizeof (struct sockaddr));

	/* check for default gateway address */

	if (((struct sockaddr_in *)(&pORE->rt_dst))->sin_addr.s_addr 
	    != INADDR_ANY )
	    {
	    i = ntohl(((struct sockaddr_in*)&pORE->rt_dst)->sin_addr.s_addr);
		
	    pNetMask->sa_family = AF_INET; 
	    pNetMask->sa_len    = 8;

	    if (IN_CLASSA(i))
		{
		((struct sockaddr_in *)pNetMask)->sin_addr.s_addr = 
		    htonl(IN_CLASSA_NET);
		net = i & IN_CLASSA_NET;
		}
	    else if (IN_CLASSB(i))
		{
		((struct sockaddr_in *)pNetMask)->sin_addr.s_addr = 
		    htonl(IN_CLASSB_NET);
		net = i & IN_CLASSB_NET;
		}
	    else if (IN_CLASSC(i))
		{
		((struct sockaddr_in *)pNetMask)->sin_addr.s_addr = 
		    htonl(IN_CLASSC_NET);
		net = i & IN_CLASSC_NET;
		}
	    else
		{
		((struct sockaddr_in *)pNetMask)->sin_addr.s_addr = 
		    htonl(IN_CLASSD_NET);
		net = i & IN_CLASSD_NET;
		}
	    /*
	     * Check whether network is a subnet;
	     * if so, return subnet number.
	     */
#ifdef VIRTUAL_STACK
	    for (ia = _in_ifaddr; ia; ia = ia->ia_next)
#else
	    for (ia = in_ifaddr; ia; ia = ia->ia_next)
#endif /* VIRTUAL_STACK */
		if (net == ia->ia_net)
		    ((struct sockaddr_in *)pNetMask)->sin_addr.s_addr =
			htonl(ia->ia_subnetmask); 
	    in_socktrim ((struct sockaddr_in *)pNetMask); 
	    }
       pORE->rt_flags |= RTF_GATEWAY;

       if(pORE->rt_flags & RTF_HOST)
	   pNetMask=0;
 
#ifdef DEBUG
       logMsg("rtIoctl:before rtrequestAddEqui/DelEqui\n",0,0,0,0,0,0);
#endif

        /*
         * Add the requested route using the default weight value or delete
         * the matching entry. In either case, report the change using both
         * routing socket messages and direct callbacks.
         */

       if(req==RTM_ADD)
	   return (rtrequestAddEqui (&pORE->rt_dst, pNetMask,
                                     &pORE->rt_gateway, pORE->rt_flags,
                                     M2_ipRouteProto_other, 0,
                                     TRUE, TRUE, NULL));
       else
	   return (rtrequestDelEqui (&pORE->rt_dst, pNetMask,
                                     &pORE->rt_gateway, pORE->rt_flags,
                                     M2_ipRouteProto_other, TRUE, TRUE, NULL));
}

#define ROUNDUP(a) (a>0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))

int rt_fixdelete (struct radix_node *, void *);
static int rt_fixchange (struct radix_node *, void *);

int
rtrequest(req, dst, gateway, netmask, flags, ret_nrt)
	int req, flags;
	struct sockaddr *dst, *gateway, *netmask;
	struct rtentry **ret_nrt;
{
	int s = splnet(); int error = 0;
	register struct rtentry *rt;
	register struct radix_node *rn;
	register struct radix_node_head *rnh;
	struct ifaddr *ifa;
	struct sockaddr *ndst;
#define senderr(x) { error = x ; goto bad; }

	if ((rnh = rt_tables[dst->sa_family]) == 0)
		senderr(ESRCH);
	if (flags & RTF_HOST)
            {
            netmask = 0;
	    }
        if (netmask)
	    {
	    TOS_SET (netmask, 0x1f);  /* set the 5 bits in the mask corresponding
					 to the TOS bits */
	    }
	switch (req) {
	case RTM_DELETE:
		if ((rn = rnh->rnh_deladdr(dst, netmask, rnh)) == 0)
			senderr(ESRCH);
		if (rn->rn_flags & (RNF_ACTIVE | RNF_ROOT))
                    {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_EMERGENCY event */
                    WV_NET_MARKER_0 (NET_AUX_EVENT, WV_NET_EMERGENCY, 22, 2,
                                     WV_NETEVENT_RTREQ_PANIC)
#endif  /* INCLUDE_WVNET */
#endif

                    panic ("rtrequest delete");
                    }

		rt = (struct rtentry *)rn;

                /*
                 * Search the appropriate subtree to remove any children
                 * (created for path MTU results). This cleanup must occur
                 * before clearing the RTF_UP flag or the system might
                 * release the current node's memory prematurely if the
                 * search deletes the last reference.
                 */

                if (dst->sa_family == AF_INET &&
                    (rt->rt_flags & RTF_CLONING) && netmask)
                    {
                    rn_walksubtree (rnh, dst, netmask, rt_fixdelete, rt);
                    }

		rt->rt_flags &= ~RTF_UP;

		if (rt->rt_gwroute) {
			rt = rt->rt_gwroute; RTFREE(rt);
			(rt = (struct rtentry *)rn)->rt_gwroute = 0;
		}
		if ((ifa = rt->rt_ifa) && ifa->ifa_rtrequest)
			ifa->ifa_rtrequest(RTM_DELETE, rt, SA(0));
		rttrash++;
		if (ret_nrt)
			*ret_nrt = rt;
		else if (rt->rt_refcnt <= 0) {
			rt->rt_refcnt++;
			rtfree(rt);
		}
		rtmodified++;
		break;

	case RTM_RESOLVE:
		if (ret_nrt == 0 || (rt = *ret_nrt) == 0)
                        {
#ifdef DEBUG
                        logMsg("rtrequest: RTM_RESOLVE, EINVAL error\n",0,0,0,0,0,0);
#endif
			senderr(EINVAL);
                        }

		ifa = rt->rt_ifa;
		flags = rt->rt_flags & ~RTF_CLONING;
		gateway = rt->rt_gateway;
		if ((netmask = rt->rt_genmask) == 0)
			{
			flags |= RTF_HOST;
			}

		goto makeroute;

	case RTM_ADD:
		if ((ifa = ifa_ifwithroute(flags, dst, gateway)) == 0)
			senderr(ENETUNREACH);
	makeroute:
		R_Malloc(rt, struct rtentry *, sizeof(ROUTE_ENTRY));
		if (rt == 0)
			senderr(ENOBUFS);
		Bzero(rt, sizeof (ROUTE_ENTRY));
		rt->rt_flags = RTF_UP | flags;
		if (rt_setgate(rt, dst, gateway)) {
			Free(rt);
			senderr(ENOBUFS);
		}
		ndst = rt_key(rt);
		if (netmask) {
			rt_maskedcopy(dst, ndst, netmask);
		} else
			Bcopy(dst, ndst, dst->sa_len);

            /* Set IP routes to create children (for path MTU results). */

            if (ndst->sa_family == AF_INET &&
                !IN_MULTICAST
                    (ntohl ( ((struct sockaddr_in *)ndst)->sin_addr.s_addr)) &&
                ! (rt->rt_flags & RTF_HOST))
                {
                if (((struct sockaddr_in *)netmask)->sin_addr.s_addr 
                         != 0xffffffff)
                    {
                    rt->rt_flags |= RTF_CLONING;
                    }
                }

		rn = rnh->rnh_addaddr((caddr_t)ndst, (caddr_t)netmask,
					rnh, rt->rt_nodes);
		/* update proto field of rt entry, in the gateway sockaddr */
		if ((req == RTM_ADD) && (gateway->sa_family == AF_INET))
		    RT_PROTO_SET (ndst, RT_PROTO_GET (dst));

                if (rn == 0 && (rt->rt_flags & RTF_HOST))
                    {
                    /* Replace matching (cloned) entry if possible. */

                    rn = routeSwap (rnh, rt, ndst, netmask);
                    }

		if (rn == 0) {
			if (rt->rt_gwroute)
				rtfree(rt->rt_gwroute);
			Free(rt_key(rt));
			Free(rt);
			senderr(EEXIST);
		}
		ifa->ifa_refcnt++;
		rt->rt_ifa = ifa;
		rt->rt_ifp = ifa->ifa_ifp;
		if (req == RTM_RESOLVE)
                    {
                    rt->rt_rmx = (*ret_nrt)->rt_rmx; /* copy metrics */
                    rt->rt_parent = (*ret_nrt);   /* record source route */
                    (*ret_nrt)->rt_refcnt++;
                    rt->rt_flags |= RTF_CLONED;
                    
                    /*
                     * Mark the cloned route as a representative route
                     * so that when it is deleted, Fastpath is called with
                     * ROUTE_REP_DEL operation and the entry gets removed
                     * from the Fastpath cache
                     */
 
                    ((ROUTE_ENTRY*)rt)->primaryRouteFlag = TRUE;

                    }

                if (rt->rt_ifp->if_type == IFT_PMP)
                    {
                    /*
                     * Point-to-multipoint devices do not use a common
                     * MTU value for all destinations or perform address
                     * resolution, so cloning the interface route to store
                     * that information is not necessary.
                     */

                    rt->rt_flags &= ~RTF_CLONING;
                    }

                /* Assign initial path MTU estimate, if enabled for route. */

                if (!rt->rt_rmx.rmx_mtu && !(rt->rt_rmx.rmx_locks & RTV_MTU)
                    && rt->rt_ifp)
                    rt->rt_rmx.rmx_mtu = rt->rt_ifp->if_mtu;

		if (ifa->ifa_rtrequest)
			ifa->ifa_rtrequest(req, rt, SA(ret_nrt ? *ret_nrt : 0));

                if (ndst->sa_family == AF_INET)
                    {
                    /*
                     * Remove any cloned routes which a new
                     * network route overrides.
                     */

                    if (!(rt->rt_flags & RTF_HOST) && rt_mask (rt) != 0)
                        {
                        rn_walksubtree (rnh, ndst, netmask, rt_fixchange, rt);
                        }
                    }

		if (ret_nrt) {
			*ret_nrt = rt;
			rt->rt_refcnt++;
		}
		rt->rt_mod = tickGet(); 	/* last modified */
		rtmodified++;
		break;
	}
bad:
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_CRITICAL event */
        if (error)
            {
            WV_NET_MARKER_4 (NET_AUX_EVENT, WV_NET_CRITICAL, 28, 7,
                             WV_NETEVENT_RTREQ_BADROUTE,
                             error, 
                             ((struct sockaddr_in *)dst)->sin_addr.s_addr, 
                             ((struct sockaddr_in *)gateway)->sin_addr.s_addr, 
                             ((struct sockaddr_in *)netmask)->sin_addr.s_addr)
            }
#endif  /* INCLUDE_WVNET */
#endif

	splx(s);
	return (error);
}

 /* 
  * This routine is called when adding a host-specific route to
  * the routing tree fails. That failure can occur because an
  * ARP entry for the destination already exists in the tree.
  * Alternatively, a cloned entry to a remote destination (for
  * path MTU results) might already exist. In either case, the
  * host-specific entry is added after the existing entry is removed.
  */

struct radix_node * routeSwap
    (
    struct radix_node_head * 	pNodeHead,
    struct rtentry * 		pRoute,
    struct sockaddr * 		pDest,
    struct sockaddr * 		pNetmask
    )
    {
    struct radix_node * 	pNode;
    struct rtentry * 		pTwinRt;
    struct ifaddr * 		pIfAddr;
    struct rtentry * 		pArpRt;
    struct radix_node_head * 	pTwinHead;
    struct sockaddr * 		pTwinKey;

    /*
     * Existing entries will prevent the addition of
     * matching host routes. Delete the entry and try
     * to add the host route again. The search is based
     * on the rtalloc1() routine.
     */

    pNode = rn_search ((caddr_t)pDest, pNodeHead->rnh_treetop);
    if ( (pNode->rn_flags & RNF_ROOT) == 0)
        {
        /* Check leaf node for ARP information or existing child route. */

        pTwinRt = (struct rtentry *)pNode;
        if ( (pTwinRt->rt_flags & RTF_CLONED) ||
                (pTwinRt->rt_flags & RTF_LLINFO &&
                 pTwinRt->rt_flags & RTF_HOST &&
                 pTwinRt->rt_gateway &&
                 pTwinRt->rt_gateway->sa_family == AF_LINK))
            {
            /* Mimic RTM_DELETE case from rtrequest() routine. */

            pTwinKey = (struct sockaddr *)rt_key (pTwinRt);
            pTwinHead = rt_tables [pTwinKey->sa_family];
            if (pTwinHead)
                {
                pNode = pTwinHead->rnh_deladdr (pTwinKey, 0, pTwinHead);
                if (pNode)
                    {
                    pArpRt = (struct rtentry *)pNode;
                    pArpRt->rt_flags &= ~RTF_UP;
                    if (pArpRt->rt_gwroute)
                        {
                        pArpRt = pArpRt->rt_gwroute;
                        RTFREE (pArpRt);
                        pArpRt = (struct rtentry *)pNode;
                        pArpRt->rt_gwroute = 0;
                        }
                    pIfAddr = pArpRt->rt_ifa;
                    if (pIfAddr && pIfAddr->ifa_rtrequest)
                        pIfAddr->ifa_rtrequest (RTM_DELETE, pArpRt, SA(0));
                    rttrash++;
                    if (pArpRt->rt_refcnt <= 0)
                        {
                        pArpRt->rt_refcnt++;
                        rtfree (pArpRt);
                        }
                    rtmodified++;
                    }
                }    /* RTM_DELETE completed. */
            pNode = pNodeHead->rnh_addaddr ( (caddr_t)pDest, (caddr_t)pNetmask, 
                                            pNodeHead, pRoute->rt_nodes);
            }  /* End ARP entry replacement. */
        else
            {
            /*
             * Matching node was not a host entry. Reset node reference.
             */

            pNode = 0;
            }
        }    /* End handling for RNF_ROOT != 0 */
    else
        {
        /* 
         * No valid entry found: remove reference to leftmost/rightmost node.
         */

        pNode = 0;
        }
    return (pNode);
    }

/*
 * The rt_fixchange() routine adjusts the routing tree after adding a new
 * network route. If the system inserts the route between an older network
 * route and the corresponding (cloned) host routes, the new route might
 * incorporate existing cloned entries. This routine removes those (obsolete)
 * cloned routes. Unfortunately, it also needlessly examines a subtree and
 * removes valid cloned routes when adding a new (less specific) route above
 * a pre-existing entry. Since those routes will be regenerated if still in
 * use, this (wasteful) behavior does not prevent correct operation.
 */

static int
rt_fixchange(rn, vp)
        struct radix_node *rn;
        void *vp;
{
        struct rtentry *rt = (struct rtentry *)rn;
        struct rtentry *rt0 = vp;
        struct radix_node_head *rnh = rt_tables [AF_INET];
        u_char *xk1, *xm1, *xk2;
        int i, len;

        if (!rt->rt_parent)     /* Ignore uncloned entries. */
            return 0;

        /*
         * Check if the network number of the route entry matches
         * the destination of the new route.
         */

        len = min (rt_key (rt0)->sa_len, rt_key (rt)->sa_len);

        xk1 = (u_char *)rt_key (rt0);
        xm1 = (u_char *)rt_mask (rt0);
        xk2 = (u_char *)rt_key (rt);

        for (i = rnh->rnh_treetop->rn_off; i < len; i++) {
                if ((xk2[i] & xm1[i]) != xk1[i]) {
                        return 0;
                }
        }

        /* Remove the cloned route which is part of the new route's network. */

        return rtrequest(RTM_DELETE, rt_key(rt), (struct sockaddr *)0,
                         rt_mask(rt), rt->rt_flags, (struct rtentry **)0);
}

/*
 * The rt_fixdelete() routine removes any cloned routes (for storing path
 * MTU information) when the original route to the remote destination is
 * deleted or the gateway is changed. It has no affect on cloned entries
 * containing ARP information.
 */

int rt_fixdelete(rn, vp)
        struct radix_node *rn;
        void *vp;
{
        struct rtentry *rt = (struct rtentry *)rn;
        struct rtentry *rt0 = vp;

        if (rt->rt_parent == rt0 && !(rt->rt_flags & RTF_LLINFO)) {
                return rtrequest(RTM_DELETE, rt_key(rt),
                                 (struct sockaddr *)0, rt_mask(rt),
                                 rt->rt_flags, (struct rtentry **)0);
        }
        return 0;
}

int
rt_setgate(rt0, dst, gate)
	struct rtentry *rt0;
	struct sockaddr *dst, *gate;
{
	caddr_t new, old;
	int dlen = ROUNDUP(dst->sa_len), glen = ROUNDUP(gate->sa_len);
	register struct rtentry *rt = rt0;

        /*
	 * Prevent addition of a host route with destination equal to
         * the gateway. Addition of such a route can cause infinite
	 * recursion while doing a route search using rtalloc which might
	 * result in stack overflow in tNetTask.
	 */

	if (((rt0->rt_flags & (RTF_HOST | RTF_GATEWAY | RTF_LLINFO)) ==
	     (RTF_HOST | RTF_GATEWAY)) && (dst->sa_len == gate->sa_len) &&
	     (((struct sockaddr_in *) dst)->sin_addr.s_addr ==
	      ((struct sockaddr_in *) gate)->sin_addr.s_addr))
	    return 1;

	if (rt->rt_gateway == 0 || glen > ROUNDUP(rt->rt_gateway->sa_len)) {
		old = (caddr_t)rt_key(rt);
		R_Malloc(new, caddr_t, dlen + glen);
		if (new == 0)
                    {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_EMERGENCY event */
                    WV_NET_MARKER_0 (NET_AUX_EVENT, WV_NET_EMERGENCY, 23, 3,
                                     WV_NETEVENT_RTSET_NOBUFS)
#endif  /* INCLUDE_WVNET */
#endif

                   return 1;
                   }
		rt->rt_nodes->rn_key = new;
	} else {
		new = rt->rt_nodes->rn_key;
		old = 0;
	}
	Bcopy(gate, (rt->rt_gateway = (struct sockaddr *)(new + dlen)), glen);
	if (old) {
		Bcopy(dst, new, dlen);
		Free(old);
	}
	if (rt->rt_gwroute) {
		rt = rt->rt_gwroute; RTFREE(rt);
		rt = rt0; rt->rt_gwroute = 0;
	}
	if (rt->rt_flags & RTF_GATEWAY) {
		rt->rt_gwroute = rtalloc1(gate, 1);
	}
	return 0;
}

void
rt_maskedcopy(src, dst, netmask)
	struct sockaddr *src, *dst, *netmask;
{
	register u_char *cp1 = (u_char *)src;
	register u_char *cp2 = (u_char *)dst;
	register u_char *cp3 = (u_char *)netmask;
	u_char *cplim = cp2 + *cp3;
	u_char *cplim2 = cp2 + *cp1;

	*cp2++ = *cp1++; *cp2++ = *cp1++; /* copies sa_len & sa_family */
	cp3 += 2;
	if (cplim > cplim2)
		cplim = cplim2;
	while (cp2 < cplim)
		*cp2++ = *cp1++ & *cp3++;
	if (cp2 < cplim2)
		bzero((caddr_t)cp2, (unsigned)(cplim2 - cp2));
}

/*
 * Set up a routing table entry, normally
 * for an interface.
 */
int
rtinit(ifa, cmd, flags)
	register struct ifaddr *ifa;
	int cmd, flags;
{
	register struct rtentry *rt;
	register struct sockaddr *dst;
	register struct sockaddr *deldst;
	struct mbuf *m = 0;
	struct rtentry *nrt = 0;
	int error=0;

	dst = flags & RTF_HOST ? ifa->ifa_dstaddr : ifa->ifa_addr;

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_VERBOSE event */
        WV_NET_MARKER_2 (NET_AUX_EVENT, WV_NET_VERBOSE, 35, 15,
                         WV_NETEVENT_RTINIT_START, cmd, 
                         ((struct sockaddr_in *)dst)->sin_addr.s_addr)
#endif  /* INCLUDE_WVNET */
#endif

	if (cmd == RTM_DELETE) {
		if ((flags & RTF_HOST) == 0 && ifa->ifa_netmask) {
			m = mBufClGet(M_WAIT, MT_SONAME, CL_SIZE_128, TRUE);
			if (m == (struct mbuf *) NULL)
			    {
			    return (ENOBUFS);
			    }
			deldst = mtod(m, struct sockaddr *);
			rt_maskedcopy(dst, deldst, ifa->ifa_netmask);
			dst = deldst;
		}
		if ((rt = rtalloc1(dst, 0))) {
			rt->rt_refcnt--;
			if (rt->rt_ifa != ifa) {
				if (m)
					(void) m_free(m);
				return (flags & RTF_HOST ? EHOSTUNREACH
							: ENETUNREACH);
			}
		}
	}

        /*
         * Create the new route using the default weight value or delete
         * the matching entry. In either case, do not report the change
         * since the later hook generates both types of messages.
         */

       if(cmd==RTM_ADD)
	   error=rtrequestAddEqui(dst, ifa->ifa_netmask, ifa->ifa_addr, 
	   		          flags | ifa->ifa_flags,
                                  M2_ipRouteProto_local, 0, FALSE, FALSE,
				  (ROUTE_ENTRY**)&nrt);
       if(cmd==RTM_DELETE)
	   error=rtrequestDelEqui(dst, ifa->ifa_netmask, ifa->ifa_addr,
			          flags | ifa->ifa_flags,
                                  M2_ipRouteProto_local, FALSE, FALSE,
				  (ROUTE_ENTRY**)&nrt);

	if (m)
		(void) m_free(m);

	if (cmd == RTM_DELETE && error == 0 && (rt = nrt)) {
        	if (rtNewAddrMsgHook) 
#ifdef ROUTER_STACK
                    (*rtNewAddrMsgHook) (cmd, ifa, nrt);
#else
                    (*rtNewAddrMsgHook) (cmd, ifa, error, nrt);
#endif /* ROUTER_STACK */
		if (rt->rt_refcnt <= 0) {
			rt->rt_refcnt++;
			rtfree(rt);
		}
	}
	if (cmd == RTM_ADD && error == 0 && (rt = nrt)) {
		rt->rt_refcnt--;
		if (rt->rt_ifa != ifa) {
			logMsg("rtinit: wrong ifa (%x) was (%x)\n", (int) ifa,
				(int) rt->rt_ifa, 0, 0, 0, 0);
			if (rt->rt_ifa->ifa_rtrequest)
			    rt->rt_ifa->ifa_rtrequest(RTM_DELETE, rt, SA(0));
			IFAFREE(rt->rt_ifa);
			rt->rt_ifa = ifa;
			rt->rt_ifp = ifa->ifa_ifp;
			ifa->ifa_refcnt++;
			if (ifa->ifa_rtrequest)
			    ifa->ifa_rtrequest(RTM_ADD, rt, SA(0));
		}
		if (rtNewAddrMsgHook)
#ifdef ROUTER_STACK
                    (*rtNewAddrMsgHook) (cmd, ifa, nrt);
#else
                    (*rtNewAddrMsgHook) (cmd, ifa, error, nrt);
#endif /* ROUTER_STACK */
	}
	return (error);
}

#ifdef RTALLOC_DEBUG
/*******************************************************************************
*
* routeEntryDebugShow - print an entry of the routing table
*
* This routine prints a routing table entry, displaying its mask, protocol,
* and type of service.
*
* RETURNS: OK or ERROR.
*
* NOMANUAL
*/

LOCAL int routeEntryDebugShow
    (
    struct radix_node * rn              /* pointer to the node structure */
    )
    {

    FAST struct sockaddr *      destAddr = NULL;
    FAST struct sockaddr *      gateAddr;
    FAST struct rtentry *       pRt;
    char                        aStr [INET_ADDR_LEN];
    uint32_t                    mask;
    FAST UCHAR *                pChr;
 

    pRt = (struct rtentry *)rn;

    destAddr = rt_key(pRt);             /* get the destination address */
    gateAddr = pRt->rt_gateway;         /* get the gateway address */


    /*  arp entries */
    /*if ((gateAddr->sa_family == AF_LINK) && (pRt->rt_flags & RTF_HOST))
        return (0);*/

    /* print destination internet address */

    inet_ntoa_b (((struct sockaddr_in *)destAddr)->sin_addr, aStr);
    printf ("if_name: %s%d\n",  pRt->rt_ifp->if_name, pRt->rt_ifp->if_unit);
    printf ("destination: %-16s\n", (destAddr->sa_family == AF_INET) ?
                      aStr : "not AF_INET\n");

    /* print netmask */

    if (rt_mask (pRt) == NULL)
        {
        mask = 0xffffffff;
        }
    else
        {
        mask = ((struct sockaddr_in *) rt_mask (pRt))->sin_addr.s_addr;
        mask = ntohl (mask);
        }

    printf ("mask: %-10lx\n", mask);

    /* print the gateway address which: internet or linklevel */

    if (gateAddr->sa_family == AF_LINK)
        {
        if ( ((struct sockaddr_dl *)gateAddr)->sdl_alen == 6)
            {
            pChr = (UCHAR *)(LLADDR((struct sockaddr_dl *)gateAddr));
            printf ("ARP ROUTE: %02x:%02x:%02x:%02x:%02x:%-6x\n",
                pChr[0], pChr[1], pChr[2], pChr[3], pChr[4], pChr[5]);

            }
        else if ( ((struct sockaddr_dl *)gateAddr)->sdl_alen == 0)
            {
            printf("ARP Interface Route Entry\n");
            }
        else
            {
            printf("Unknown Entry\n");
            return(0);
            }
        }
    else
        {
        inet_ntoa_b (((struct sockaddr_in *)gateAddr)->sin_addr, aStr);
        printf ("Gateway: %-20s \n", aStr);
        }

    /* print type of service */
    printf ("tos: %-5x\n", TOS_GET (destAddr));

    printf ("flags: ");
    if (pRt->rt_flags & RTF_UP)
        printf("U");

    if (pRt->rt_flags & RTF_HOST)
        printf("H");

    if (pRt->rt_flags & RTF_LLINFO)
        printf("L");

    if (pRt->rt_flags & RTF_CLONING)
        printf("C ");

    if (pRt->rt_flags & RTF_STATIC)
        printf("S");

    if (pRt->rt_flags & RTF_GATEWAY)
        {
        printf("G");
        if (pRt->rt_gwroute)
            printf(": Gateway route entry = %p ", pRt->rt_gwroute);
        }

    printf ("\n");

    printf ("refCnt: %-7d\n",   pRt->rt_refcnt);
    printf ("rt_use: %-7ld\n",  pRt->rt_use);
    printf ("proto: %-5d\n",  RT_PROTO_GET (destAddr));

    return (0);
    }
#endif /* RTALLOC_DEBUG */

