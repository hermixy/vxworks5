/* output.c - routines for generating outgoing RIP messages */

/* Copyright 1984 - 2001 Wind River Systems, Inc. */
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
 *            @(#)output.c	8.2 (Berkeley) 4/28/95";
 */

/*
modification history
--------------------
01m,22mar02,niq  Merged from Synth view, tor3_x.synth branch, ver 01p
01l,15oct01,rae  merge from truestack ver 01n, base 01i (VIRTUAL_STACK etc.)
01k,21nov00,spm  fixed handling of interface for default route (SPR #62533)
01j,10nov00,spm  merged from version 01j of tor3_x branch (SPR #33692 fix)
01i,11sep98,spm  added option to disable gateway filtering and removed
                 all references to bloated trace commands (SPR #22350)
01h,01sep98,spm  extended gateway filtering tests to handle classless 
                 netmasks and host-specific routes
01g,26jul98,spm  removed duplicate condition from test in supply routine;
                 corrected stray pointer in ripBuildPacket call and added
                 parameter needed for RIPv2 updates; removed compiler warnings
01f,06oct97,gnn  added sendHook functionality to sending
01e,08may97,gnn  fixed an authentication bug.
01d,07apr97,gnn  cleared up some of the more egregious warnings.
                 added MIB-II interface and option support.
01c,13mar97,gnn  fixed a minor bug in the output routine
01b,24feb97,gnn  added rip version 2 functionality
01a,26nov96,gnn  created from BSD4.4 routed
*/

/*
DESCRIPTION
*/

/*
 * Routing Table Management Daemon
 */
#include "vxWorks.h"
#include "logLib.h"
#include "rip/defs.h"
#include "rip/m2RipLib.h"

#ifdef VIRTUAL_STACK
#include "netinet/vsLib.h"
#include "netinet/vsRip.h"
#else
IMPORT int routedDebug;
IMPORT BOOL ripFilterFlag;
#endif

/*
 * Apply the function "f" to all non-passive
 * interfaces.  If the interface supports the
 * use of broadcasting use it, otherwise address
 * the output to the known router.
 */
void toall(f, rtstate, skipif)
	int (*f)();
	int rtstate;
	struct interface *skipif;
{
	register struct interface *ifp;
	register struct sockaddr *dst;
	register int flags;

#ifndef VIRTUAL_STACK
	extern struct interface *ripIfNet;
#endif

	for (ifp = ripIfNet; ifp; ifp = ifp->int_next)
            {
            if (ifp->int_flags & IFF_PASSIVE || ifp == skipif || 
                (ifp->int_flags & IFF_UP) == 0)
                continue;
            dst = ifp->int_flags & IFF_BROADCAST ? &ifp->int_broadaddr :
                ifp->int_flags & IFF_POINTOPOINT ? &ifp->int_dstaddr :
                &ifp->int_addr;
            flags = ifp->int_flags & IFF_INTERFACE ? MSG_DONTROUTE : 0;
            
            if (f == supply)
                (*f)(dst, flags, ifp, rtstate, 0);
            else
                (*f)(dst, flags, ifp, rtstate);
	}
}

/*
 * Output a preformed packet.
 */
/*ARGSUSED*/
STATUS sndmsg(dst, flags, ifp, rtstate)
	struct sockaddr *dst;
	int flags;
	struct interface *ifp;
	int rtstate;
{
#ifndef VIRTUAL_STACK
        IMPORT RIP ripState;
#endif

	(*afswitch[dst->sa_family].af_output)(ripState.s, flags,
		dst, sizeof (RIP_PKT));

       if (routedDebug > 2)
           logMsg ("Transmitting RIP message.\n", 0, 0, 0, 0, 0, 0);

        return (OK);
}

/*
 * Supply dst with the contents of the routing tables.
 * If this won't fit in one packet, chop it up into several.
 */
STATUS supply
    (
    struct sockaddr *dst,
    int flags,
    register struct interface *ifp,
    int rtstate,
    int version
    )
    {
#ifndef VIRTUAL_STACK
    IMPORT RIP ripState;
#endif

    register struct rt_entry *rt;
    register struct netinfo *pNetinfo = ripState.msg->rip_nets;
    register struct rthash *rh;
    struct rthash *base = hosthash;
    int doinghost = 1, size;
    int (*output)() = afswitch[dst->sa_family].af_output;
    int (*sendroute)() = afswitch[dst->sa_family].af_sendroute;
    int npackets = 0;
    struct interface* pIfp;

#ifdef RIP_MD5
    RIP2_AUTH_PKT_HDR * pAuthHdr;
    RIP_AUTH_KEY * pAuthKey;
#endif /* RIP_MD5 */

    ripState.msg->rip_cmd = RIPCMD_RESPONSE;
    
    if (ifp == NULL)
        pIfp = ripIfLookup(dst);
    else
        pIfp = ifp;
    
    if (pIfp == NULL)
        return (ERROR);
    
     /*
      * Check actual interface status before transmitting
      * The interface might have been disabled. 
      */

     if ((pIfp->int_flags & IFF_UP) == 0)
         return (ERROR);

    /*
     * Next, check the status from the MIB-II RIP group. If this
     * interface has been turned off then silently drop packets on it.
     */

    if (pIfp->ifConf.rip2IfConfStatus != M2_rip2IfConfStatus_valid)
        return (ERROR);

    if (pIfp->ifConf.rip2IfConfSend == M2_rip2IfConfSend_doNotSend)
        return (ERROR);
    else if (pIfp->ifConf.rip2IfConfSend == M2_rip2IfConfSend_ripVersion1)
            ripState.msg->rip_vers = 1;
    else if ((pIfp->ifConf.rip2IfConfSend == M2_rip2IfConfSend_rip1Compatible)
             && version != 0)
             ripState.msg->rip_vers = version;
    else if ((pIfp->ifConf.rip2IfConfSend == M2_rip2IfConfSend_rip1Compatible)
             || (pIfp->ifConf.rip2IfConfSend == M2_rip2IfConfSend_ripVersion2))
             ripState.msg->rip_vers = 2;
    else
        return (ERROR);
             

    memset(ripState.msg->rip_domain, 0, sizeof(ripState.msg->rip_domain));
    /*
     * If we are doing authentication then properly fill in the first field.
     *
     */

#ifdef RIP_MD5
    if (pIfp->ifConf.rip2IfConfAuthType == M2_rip2IfConfAuthType_md5)
        {
        if (ripAuthKeyOut1MD5 (pIfp, pNetinfo, &pAuthHdr, &pAuthKey) == ERROR)
            return (ERROR);

        pNetinfo++;
        }
    else if (pIfp->ifConf.rip2IfConfAuthType ==
             M2_rip2IfConfAuthType_simplePassword)
#else
    if (pIfp->ifConf.rip2IfConfAuthType ==
        M2_rip2IfConfAuthType_simplePassword)
#endif /* RIP_MD5 */
        {
        bzero((char *)pNetinfo, sizeof(RIP2PKT));
        ((RIP2PKT *)pNetinfo)->family = RIP2_AUTH;
        ((RIP2PKT *)pNetinfo)->tag = M2_rip2IfConfAuthType_simplePassword;
        bcopy((char *)pIfp->ifConf.rip2IfConfAuthKey,
              (char *)pNetinfo + 4, RIP2_AUTH_LEN);
        pNetinfo++;
        }

    again:
    for (rh = base; rh < &base[ROUTEHASHSIZ]; rh++)
	for (rt = rh->rt_forw; rt != (struct rt_entry *)rh; rt = rt->rt_forw)
            {
            if (routedDebug > 1)
                logMsg ("Looking at IF %s, route to %d %d %d %d \n",
                        (int)(rt->rt_ifp ? rt->rt_ifp->int_name : "?"),
                        (u_char)rt->rt_dst.sa_data[2], 
                        (u_char)rt->rt_dst.sa_data[3],
                        (u_char)rt->rt_dst.sa_data[4], 
                        (u_char)rt->rt_dst.sa_data[5], 0);

            /*
             * This calls the per interface send hook.  The user may
             * add their own hook that allows them to decide on a route
             * by route basis what routes to add to the update.
             */

            if (pIfp->sendHook != NULL)
                if (!pIfp->sendHook(rt))
                    continue;

            /*
             * When sending in response to a query, ifp is zero, disabling
             * this test. Otherwise, information is not sent on the network 
             * from which it was received (simple split horizon).
             */

            if (ifp && (rt->rt_ifp == ifp))
                continue;

            /*
             * If the route's interface is down, then we don't really
             * want to advertise that route. Skip it
             */

            if (rt->rt_ifp && (rt->rt_ifp->int_flags & IFF_UP) == 0)
                continue;

            /* 
             * "External" routes are only created for a loopback interface and
             * are never sent.
             */

            if (rt->rt_state & RTS_EXTERNAL)
                continue;

            /*
             * This test detects routes with the specified state. It limits 
             * the contents of dynamic updates to route entries with 
             * the RTS_CHANGED flag set. 
             */

            if (rtstate && (rt->rt_state & rtstate) == 0)
                continue;

            /*
             * This test implements the network-related border gateway 
             * filtering specified by RFC 1058 as well as the restrictions 
             * in section 3.3 of RFC 1723 needed for compatibility between 
             * RIPv1 and RIPv2. It selects between the internally generated
             * routes sent to "distant" hosts (which are not directly
             * connected to the destination) and the (possibly classless)
             * route entries which are only sent to neighbors on the same 
             * logical network.
             */

            if (doinghost == 0 && rt->rt_state & RTS_SUBNET)
                {
                if (rt->rt_dst.sa_family != dst->sa_family)
                    continue;

                if (ripFilterFlag)
                    {
                    /* 
                     * Perform border gateway filtering if enabled. The 
                     * restrictions are only needed if RIP-1 routers are
                     * in use on the network.
                     */

                    if ( (*sendroute)(rt, dst, pIfp) == 0)
                        continue;
                    }
                else
                    {
                    /* 
                     * Border gateway filtering is disabled. Internally 
                     * generated routes (which represent the network as
                     * a whole) are never sent. All other classless routes
                     * are included unconditionally.
                     */

                    if (rt->rt_state & RTS_INTERNAL)
                        continue;
                    }
                }

            /* 
             * Limit any host route to neighbors within the same logical
             * network. This test also handles some network routes that
             * appear to be host routes to a router because they use a 
             * longer prefix than the receiving interface. These
             * restrictions are only necessary if border gateway filtering
             * is enabled to support an environment with mixed RIP-1 and
             * RIP-2 routers.
             */

            if (doinghost == 1 && ripFilterFlag)
                {
                if (rt->rt_dst.sa_family != dst->sa_family)
                    continue;
                if ((*sendroute)(rt, dst, pIfp) == 0)
                    continue;
                }

            size = (char *)pNetinfo - ripState.packet;

#ifdef RIP_MD5
            if (pIfp->ifConf.rip2IfConfAuthType == M2_rip2IfConfAuthType_md5)
                {
                /* must save a trailing entry for the MD5 auth digest */
                if (size > (MAXPACKETSIZE - (2 * sizeof(struct netinfo))))
                    {
                    ripAuthKeyOut2MD5(ripState.msg, &size, pNetinfo,
                                      pAuthHdr, pAuthKey);

                    if (routedDebug > 2)
                        logMsg ("Transmitting RIP message.\n",
                                0, 0, 0, 0, 0, 0);
                    (*output)(ripState.s, flags, dst, size);
                    /*
                     * If only sending to ourselves,
                     * one packet is enough to monitor interface.
                     */
                    if (ifp && (ifp->int_flags &
                                (IFF_BROADCAST | IFF_POINTOPOINT | IFF_REMOTE))
                        == 0)
                        return (ERROR);

                    /* set pNetinfo to second entry because first is auth */
                    pNetinfo = (ripState.msg->rip_nets +
                                sizeof(struct netinfo));
                    npackets++;
                    }
                }
            else
                {
#endif /* RIP_MD5 */
                if (size > MAXPACKETSIZE - sizeof (struct netinfo))
                    {
                    if (routedDebug > 2)
                        logMsg ("Transmitting RIP message.\n",
                                0, 0, 0, 0, 0, 0);
                    (*output)(ripState.s, flags, dst, size);
                    /*
                     * If only sending to ourselves,
                     * one packet is enough to monitor interface.
                     */
                    if (ifp && (ifp->int_flags &
                                (IFF_BROADCAST | IFF_POINTOPOINT | IFF_REMOTE))
                        == 0)
                        return (ERROR);

                    if (pIfp->ifConf.rip2IfConfAuthType ==
                        M2_rip2IfConfAuthType_simplePassword)
                        {
                        /* set pNetinfo to second entry because first is auth */
                        pNetinfo = (ripState.msg->rip_nets +
                                    sizeof(struct netinfo));
                        }
                    else
                        {
                        /* else set pNetinfo to the first entry */
                        pNetinfo = ripState.msg->rip_nets;
                        }
                    npackets++;
                    }
#ifdef RIP_MD5
                }
#endif /* RIP_MD5 */

#define osa(x) ((struct osockaddr *)(&(x)))
            osa(pNetinfo->rip_dst)->sa_family =
                htons(rt->rt_dst.sa_family);
            ripBuildPacket((RIP2PKT *)pNetinfo, rt,
                           pIfp, pIfp->ifConf.rip2IfConfSend);
            pNetinfo++;
            }

	if (doinghost)
            {
            doinghost = 0;
            base = nethash;
            goto again;
            }

	/*
	 * If we have something to send or if someone explicitly
	 * requested a dump of whatever we have, send the packet.
	 * If we are doing a regular/triggered update and there is
	 * nothing to send, skip the update
	 */

	if (pNetinfo != ripState.msg->rip_nets || version != 0)
            {
            size = (char *)pNetinfo - ripState.packet;

#ifdef RIP_MD5
            if (pIfp->ifConf.rip2IfConfAuthType == M2_rip2IfConfAuthType_md5)
                {
                ripAuthKeyOut2MD5(ripState.msg, &size, pNetinfo,
                                  pAuthHdr, pAuthKey);
                }
#endif /* RIP_MD5 */

            if (routedDebug > 2)
                logMsg ("Transmitting RIP message.\n", 0, 0, 0, 0, 0, 0);
            (*output)(ripState.s, flags, dst, size);
            }
        return (OK);
    }
