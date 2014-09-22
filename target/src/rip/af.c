/* af.c - RIP routines supporting specific address families */

/* Copyright 1984 - 2001 Wind River Systems, Inc. */
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
 */

/*
modification history
--------------------
01n,22mar02,niq  Merged from Synth view, tor3_x.synth branch, ver 01o
01m,15oct01,rae  merge from truestack ver 01n, base 01k (VIRTUAL_STACK)
01l,10nov00,spm  merged from version 01m of tor3_x branch (code cleanup)
01k,11sep98,spm  updated multicast conversion test to detect all broadcast 
                 addresses (SPR #22350)
01j,01sep98,spm  refined inet_checkhost test to support default routes 
                 (SPR #22067) and classless netmasks (SPR #22065)
01i,26jun98,spm  changed RIP_MCAST_ADDR constant from string to value
01h,06oct97,gnn  added includes to remove warnings
01g,02jun97,gnn  fixed SPR 8672 which was a byte order problem.
01f,08may97,gnn  fixed muticasting issues in inet_send.
01e,17apr97,gnn  fixed errors pointed out by ANVL.
01d,07apr97,gnn  fixed some of the more egregious warnings
                 added code to handle MIB-II interfaces and options
01c,12mar97,gnn  added multicast support.
                 added time variables.
01b,24feb97,gnn  Modified code to handle new global state variables.
01a,26nov96,gnn  created from BSD4.4 routed 
*/

/*
DESCRIPTION
*/

#include "vxWorks.h"
#include "rip/defs.h"
#include "ioctl.h"
#include "inetLib.h"
#include "sockLib.h"
#include "logLib.h"

#ifdef VIRTUAL_STACK
#include  "netinet/vsLib.h"
#include  "netinet/vsRip.h"
#else
IMPORT int routedDebug;
#endif

/*
 * Address family support routines
 */
int	inet_hash(), inet_netmatch(), inet_output(),
	inet_portmatch(), inet_portcheck(),
	inet_checkhost(), inet_rtflags(), inet_sendroute(), inet_canon();

char	*inet_format();

#define NIL	{ 0 }
#define	RIP_INET \
	{ inet_hash,		inet_netmatch,		inet_output, \
	  inet_portmatch,	inet_portcheck,		inet_checkhost, \
	  inet_rtflags,		inet_sendroute,		inet_canon, \
	  inet_format \
	}

struct afswitch afswitch[AF_MAX] = {
	NIL,		/* 0- unused */
	NIL,		/* 1- Unix domain, unused */
	RIP_INET,		/* Internet */
};

int inet_hash(sin, hp)
	register struct sockaddr_in *sin;
	struct afhash *hp;
{
	register u_long n;

	n = inet_netof(sin->sin_addr);
	if (n)
	    while ((n & 0xff) == 0)
		n >>= 8;
	hp->afh_nethash = n;
	hp->afh_hosthash = ntohl(sin->sin_addr.s_addr);
	hp->afh_hosthash &= 0x7fffffff;

        return (OK);
}

/*
 * Verify the message is from the right port.
 */
int inet_portmatch(sin)
	register struct sockaddr_in *sin;
{
#ifndef VIRTUAL_STACK
        IMPORT RIP ripState;
#endif

	return (sin->sin_port == ripState.port);
}

/*
 * Verify the message is from a "trusted" port.
 */
int inet_portcheck(sin)
	struct sockaddr_in *sin;
{

	return (ntohs(sin->sin_port) <= IPPORT_RESERVED);
}

/******************************************************************************
* inet_output - RIP Internet output routine.
*
* RETURNS: N/A
* 
* NOMANUAL
*/
int inet_output
    (
    int s,
    int flags,
    struct sockaddr_in *sin,
    int size
    )
    {
    struct sockaddr_in dst;
    struct in_addr localIpAddr;
    struct interface* pIfp;
    u_long host;
    u_long mask;

#ifndef VIRTUAL_STACK
    IMPORT RIP ripState;
#endif

    dst = *sin;
    sin = &dst;
    if (sin->sin_len == 0)
        sin->sin_len = sizeof (*sin);

    pIfp = ripIfLookup((struct sockaddr *)&dst);

    /* 
     * Do not assign any value to sin_port before ripIfLookup is called
     * as ripIflookup might fail since it compares all 16 bytes
     */

    if (sin->sin_port == 0)
        sin->sin_port = ripState.port;

    if (pIfp->ifConf.rip2IfConfSend == M2_rip2IfConfSend_ripVersion2)
        {
        /*
         * If we're sending to the broadcast address then change
         * to multicast.
         */

        host = ntohl (sin->sin_addr.s_addr);
        mask = pIfp->int_subnetmask;

        mask = ~mask;

        if ( (host & mask) == mask)     /* Host part is all ones? */
            {
            localIpAddr.s_addr = (uint32_t) ((struct sockaddr_in *)&pIfp->int_addr)->sin_addr.s_addr;
    
            if (setsockopt (s, IPPROTO_IP, IP_MULTICAST_IF,
                            (char *) & localIpAddr, sizeof localIpAddr) != OK)
                {
                return (ERROR);
                }

            sin->sin_addr.s_addr = htonl (RIP_MCAST_ADDR);
            }
        flags = 0;
        }

    if (sendto(s, ripState.packet, size, flags,
               (struct sockaddr *)sin, sizeof (*sin)) < 0)
        {
        if (routedDebug)
            logMsg ("Error %x sending RIP message.\n", errno, 0, 0, 0, 0, 0);
        }

    return (OK);
    }

/*
 * Return 1 if the given address is valid. This routine tests the
 * destination address for a route. Forbidden values are:
 *      an address with class D or E;
 *      any address on net 127;
 *      all addresses on net 0 (except 0.0.0.0, for a default route);
 *      any broadcast address.
 *
 * Any entry which specifies these addresses is ignored. Broadcast addresses
 * are detected using the netmask contained in the route update, if any.
 * Otherwise the subnetmask value from the receiving interface is used to
 * determine the host part of the address.
 */

int inet_checkhost(sin, ifp)
	struct sockaddr_in *sin;
	struct interface *ifp;
{

        /* 
         * The sockaddr_in structure actually overlays a RIP route entry,
         * so the "unused" sin_zero field contains valid data. Retrieve
         * the destination address and the subnet mask, if any. 
         */

	u_long i = ntohl(sin->sin_addr.s_addr);
        u_long mask;

        /*
         * Set the mask value to use the subnet of the receiving interface
         * if none is available in the route entry. The interface's subnet
         * value is equal to the class-based mask by default, but will
         * contain a value that produces a longer or shorter network prefix 
         * if subnetting or supernetting are used.
         */

        bcopy (sin->sin_zero, (char *)&mask, 4);

        /* 
         * Convert to host byte order if necessary. Otherwise use the
         * available subnet mask.
         */

        if (mask == 0)
            mask = ifp->int_subnetmask;
        else
            mask = ntohl (mask);

        /* Limited broadcast address: reject. */

        if (i == 0xffffffff)
            return (0);

        /* Accept the default address. */

        if (i == 0)
            return (1);

        /* Reject remaining entries with network number 0. */

        if ((i & mask) == 0)
            return (0);

        /* Check for other broadcast address formats. Reject if found. */

        mask = ~mask;

        if ( (i & mask) == mask)    /* Host part is all ones? */
            return (0);

        /* Reject any address in loopback network. */

        if ((i & 0x7f000000) == 0x7f000000)
            return (0);

        /* Reject any address in class D or class E. */
        
        if (i >= 0xe0000000)
            return (0);

        /* Accept all other addresses. */

	return (1);
}

int inet_canon(sin)
	struct sockaddr_in *sin;
{

	sin->sin_port = 0;
	sin->sin_len = sizeof(*sin);

        return (OK);

}


char *
inet_format(sin)
	struct sockaddr_in *sin;
{
	char *inet_ntoa();

	return (inet_ntoa(sin->sin_addr));
}
