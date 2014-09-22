/* routed_if.c -  RIP routines for handling available interfaces */

/* Copyright 1984 - 2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
 * Copyright (c) 1983, 1993
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
 * @(#)if.c	8.2 (Berkeley) 4/28/95";
 */

/*
modification history
--------------------
01f,22mar02,niq  Merged from Synth view, tor3_x.synth branch, ver 01h
01e,15oct01,rae  merge from truestack ver 01f, base 01c (SPR #69983 etc.)
01d,10nov00,spm  merged from version 01d of tor3_x branch (namespace cleanup)
01c,01sep98,spm  extended the interface search routines to handle 
                 supernets (SPR #22220)
01b,24feb97,gnn  changed routine names to avoid conflicts.
01a,26nov96,gnn  created from BSD4.4 routed if.c
*/

/*
DESCRIPTION
*/

/*
 * Routing Table Management Daemon
 */
#include "vxWorks.h"
#include "rip/defs.h"

#ifdef VIRTUAL_STACK
#include "netinet/vsLib.h"
#include "netinet/vsRip.h"
#else
extern	struct interface *ripIfNet;
#endif

/*
 * Find the interface with address addr.
 */
struct interface *
ripIfWithAddr(addr)
	struct sockaddr *addr;
{
	register struct interface *ifp;

#define	same(a1, a2) \
	(memcmp((a1)->sa_data, (a2)->sa_data, 14) == 0)
	for (ifp = ripIfNet; ifp; ifp = ifp->int_next) {
		if (ifp->int_flags & IFF_REMOTE)
			continue;
		if (ifp->int_addr.sa_family != addr->sa_family)
			continue;
		if (same(&ifp->int_addr, addr))
			break;
		if ((ifp->int_flags & IFF_BROADCAST) &&
		    same(&ifp->int_broadaddr, addr))
			break;
	}
	return (ifp);
}

/*
 * Find the interface with address addr.
 */
struct interface *
ripIfWithAddrAndIndex(addr, index)
	struct sockaddr *addr;
        int index;
{
	register struct interface *ifp;

#define	same(a1, a2) \
	(memcmp((a1)->sa_data, (a2)->sa_data, 14) == 0)
	for (ifp = ripIfNet; ifp; ifp = ifp->int_next) {
		if (ifp->int_flags & IFF_REMOTE)
			continue;
		if (ifp->int_addr.sa_family != addr->sa_family)
			continue;
		if (same(&ifp->int_addr, addr) && index == ifp->int_index)
			break;
		if ((ifp->int_flags & IFF_BROADCAST) &&
		    same(&ifp->int_broadaddr, addr) && index == ifp->int_index)
			break;
	}
	return (ifp);
}

/*
 * Find the point-to-point interface with destination address addr.
 */
struct interface *
ripIfWithDstAddr(addr, gateaddr)
	struct sockaddr *addr;
	struct sockaddr *gateaddr;
{
	register struct interface *ifp;

	for (ifp = ripIfNet; ifp; ifp = ifp->int_next) {
		if ((ifp->int_flags & IFF_POINTOPOINT) == 0)
			continue;
		if (same(&ifp->int_dstaddr, addr) || 
                    (gateaddr && same(&ifp->int_dstaddr, gateaddr)))
			break;
	}
	return (ifp);
}

/*
 * Find the point-to-point interface with destination address addr and index.
 */
struct interface *
ripIfWithDstAddrAndIndex(addr, gateaddr, index)
	struct sockaddr *addr;
	struct sockaddr *gateaddr;
{
	register struct interface *ifp;

	for (ifp = ripIfNet; ifp; ifp = ifp->int_next) {
		if ((ifp->int_flags & IFF_POINTOPOINT) == 0)
			continue;
		if ((same(&ifp->int_dstaddr, addr) || 
                     (gateaddr && same(&ifp->int_dstaddr, gateaddr))) &&
                    ifp->int_index == index)
			break;
	}
	return (ifp);
}

/*
 * Find the interface on the network 
 * of the specified address. INTERNET SPECIFIC.
 */
struct interface *
ripIfWithNet(addr)
	register struct sockaddr *addr;
{
	register struct interface *ifp;
	register struct interface *pIfMax;
	register int af = addr->sa_family;
        register u_long dstaddr;

	if (af >= AF_MAX)
		return (0);

        pIfMax = 0;

        dstaddr = ntohl ( ((struct sockaddr_in *)addr)->sin_addr.s_addr);

	for (ifp = ripIfNet; ifp; ifp = ifp->int_next) {
		if (ifp->int_flags & IFF_REMOTE)
			continue;
		if (af != ifp->int_addr.sa_family)
			continue;

                if ( (dstaddr & ifp->int_subnetmask) == ifp->int_subnet)
                    {
                    if (pIfMax)
                        {
                        if ( (ifp->int_subnetmask | pIfMax->int_subnetmask)
                             == ifp->int_subnetmask)
                            pIfMax = ifp;
                        }
                    else
                        pIfMax = ifp;
                    }
	}
	return (pIfMax);
}

/*
 * Find an interface matching the specified source address. This routine
 * is used to select an interface for tracking errors in RIP responses
 * and verifying the source of RIP updates before altering the routing
 * table.
 */

struct interface * ripIfLookup(addr)
	struct sockaddr *addr;
{
	register struct interface *ifp, *maybe;
	register int af = addr->sa_family;
	register u_long dstaddr;

	if (af >= AF_MAX)
		return (0);

	maybe = 0;
        dstaddr = ntohl ( ((struct sockaddr_in *)addr)->sin_addr.s_addr);

	for (ifp = ripIfNet; ifp; ifp = ifp->int_next) {
		if (ifp->int_addr.sa_family != af)
			continue;

                /*
                 * When this routine is called by inet_output() via the
                 * toall() routine, one of the following comparisons is 
                 * always true and the results of the network number 
                 * tests are not used.
                 */

		if (same(&ifp->int_addr, addr))
			break;
		if ((ifp->int_flags & IFF_BROADCAST) &&
		    same(&ifp->int_broadaddr, addr))
			break;
		if ((ifp->int_flags & IFF_POINTOPOINT) &&
		    same(&ifp->int_dstaddr, addr))
			break;

                /*
                 * If the given address does not match any of the 
                 * available interfaces directly, search for the
                 * matching network number with the longest prefix.
                 * The use of the if_subnetmask value instead of
                 * if_netmask accounts for any classless routing
                 * (supernets or subnets). The results of this test
                 * are generally used when verifying the source of
                 * a RIP message before accepting any route updates.
                 */

		if ( (dstaddr & ifp->int_subnetmask) == ifp->int_subnet)
                    {
                    if (maybe)
                        {
                        if ( (ifp->int_subnetmask | maybe->int_subnetmask)
                            == ifp->int_subnetmask)
                            maybe = ifp;
                        }
                    else
                        maybe = ifp;
                    }
	}
	if (ifp == 0)
		ifp = maybe;
	return (ifp);
}
