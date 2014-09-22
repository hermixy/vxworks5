/* inet.c - inet family routines for routed/rip */

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
 *
 * @(#)inet.c	8.3 (Berkeley) 12/30/94
 */

/*
modification history
--------------------
01g,22mar02,niq  Merged from Synth view, tor3_x.synth branch, ver 01g
01f,15oct01,rae  merge from truestack ver 01f, base o1e (VIRTUAL_STACK)
01e,11sep98,spm  replaced ripMakeAddr with optimized results (SPR #22350)
01d,01sep98,spm  added support for supernets (SPR #22220); removed unused
                 inet_maskof routine
01c,07apr97,gnn  cleared up some of the more egregious warnings.
01b,24feb97,gnn  Reworked several routines to be in line with WRS standards.
01a,26nov96,gnn  created from BSD4.4 routed
*/

/*
DESCRIPTION
*/

/*
 * Temporarily, copy these routines from the kernel,
 * as we need to know about subnets.
 */
#include "vxWorks.h"
#include "rip/defs.h"

#ifdef VIRTUAL_STACK
#include "netinet/vsLib.h"
#include "netinet/vsRip.h"
#else
extern struct interface *ripIfNet;
#endif

#define same(a1, a2) \
        (memcmp((a1)->sa_data, (a2)->sa_data, 14) == 0)

/*
 * Return RTF_HOST if the address is
 * for an Internet host, RTF_SUBNET for a subnet,
 * 0 for a network.
 */
int inet_rtflags(sin)
	struct sockaddr_in *sin;
{
	register u_long i = ntohl(sin->sin_addr.s_addr);
	register u_long net, host;
	register struct interface *ifp;
	register struct interface *maybe = NULL;
	register u_long dstaddr;

        dstaddr = ntohl ( ((struct sockaddr_in *)sin)->sin_addr.s_addr);
	if (IN_CLASSA(i)) {
		net = i & IN_CLASSA_NET;
		host = i & IN_CLASSA_HOST;
	} else if (IN_CLASSB(i)) {
		net = i & IN_CLASSB_NET;
		host = i & IN_CLASSB_HOST;
	} else {
		net = i & IN_CLASSC_NET;
		host = i & IN_CLASSC_HOST;
	}

	/*
	 * Check whether this network is subnetted;
	 * if so, check whether this is a subnet or a host.
         * If we have point to point interfaces, check if the address
         * matches either the local or the remote end of the link.
         * If it does, then return HOST.
         * While going thru the interface list looking for a
         * matching metwork number, we want to find the
         * interface that has the longest subnetmask.
	 */
	for (ifp = ripIfNet; ifp; ifp = ifp->int_next)
            {
            if (net == ifp->int_net) 
                {
                if ((ifp->int_flags & IFF_POINTOPOINT) && 
                    (same (&ifp->int_dstaddr, (struct sockaddr *)sin) 
                     || same (&ifp->int_addr, (struct sockaddr *)sin)))
                        return (RTF_HOST);
                }
            if ((dstaddr & ifp->int_subnetmask) == ifp->int_subnet)
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
        if ((ifp = maybe) != NULL)
            {
            if (host &~ ifp->int_subnetmask)
                return (RTF_HOST);
            else if (ifp->int_subnetmask != ifp->int_netmask)
                return (RTF_SUBNET);
            else
                return (0);		/* network */
            }
        else
            {
            if (host == 0)
                return (0);	/* network */
            else
                return (RTF_HOST);
            }
}

/*
 * Return true if a route to subnet/host of route rt should be sent to dst.
 * Send it only if dst is on the same logical network if not "internal",
 * otherwise only if the route is the "internal" route for the logical net.
 */
int inet_sendroute
    (
    struct rt_entry *rt,
    struct sockaddr_in *dst,
    struct interface *ifp
    )
    {
    register u_long r =
        ntohl(((struct sockaddr_in *)&rt->rt_dst)->sin_addr.s_addr);
    register u_long d = ntohl(dst->sin_addr.s_addr);
    register u_long dmask = ifp->int_subnetmask;
    register u_long rmask =
        ntohl ( ((struct sockaddr_in *)&rt->rt_netmask)->sin_addr.s_addr);

    if (IN_CLASSA(r))
        {
        /* Check if destinations share a class-based logical network. */

        if ((r & IN_CLASSA_NET) == (d & IN_CLASSA_NET))
            {
            if ((r & IN_CLASSA_HOST) == 0)
                return ((rt->rt_state & RTS_INTERNAL) == 0);
            return (1);
            }

        /* Also test for a network match based on a possible supernet. */

        if ( ((r & dmask) == (d & dmask)) || 
                 ((rmask != 0) && (r & rmask) == (d & rmask)))
            {
            if ((r & IN_CLASSA_HOST) == 0)
                return ((rt->rt_state & RTS_INTERNAL) == 0);
            return (1);
            }

        /* 
         * The route and update destinations are on different logical
         * networks. Only send an internally generated route to the 
         * entire network. If supernetting is used, this test causes
         * the border gateway to substitute "natural" network routes
         * based on the available interface addresses for the route to
         * the entire supernet.
         */

        if (r & IN_CLASSA_HOST)
            return (0);
        return ((rt->rt_state & RTS_INTERNAL) != 0);
	}
    else if (IN_CLASSB(r))
        {
        if ((r & IN_CLASSB_NET) == (d & IN_CLASSB_NET))
            {
            if ((r & IN_CLASSB_HOST) == 0)
                return ((rt->rt_state & RTS_INTERNAL) == 0);
            return (1);
            }

        if ( ((r & dmask) == (d & dmask)) || 
                 ((rmask != 0) && (r & rmask) == (d & rmask)))
            {
            if ((r & IN_CLASSB_HOST) == 0)
                return ((rt->rt_state & RTS_INTERNAL) == 0);
            return (1);
            }

        if (r & IN_CLASSB_HOST)
            return (0);
        return ((rt->rt_state & RTS_INTERNAL) != 0);
	}
    else
        {
        if ((r & IN_CLASSC_NET) == (d & IN_CLASSC_NET))
            {
            if ((r & IN_CLASSC_HOST) == 0)
                return ((rt->rt_state & RTS_INTERNAL) == 0);
            return (1);
            }

        if ( ((r & dmask) == (d & dmask)) || 
                 ((rmask != 0) && (r & rmask) == (d & rmask)))
            {
            if ((r & IN_CLASSC_HOST) == 0)
                return ((rt->rt_state & RTS_INTERNAL) == 0);
            return (1);
            }

        if (r & IN_CLASSC_HOST)
            return (0);
        return ((rt->rt_state & RTS_INTERNAL) != 0);
	}
    }
