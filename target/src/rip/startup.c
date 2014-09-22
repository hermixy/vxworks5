/* startup.c - RIP routines for creating initial data structures */

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
 * @(#)startup.c	8.2 (Berkeley) 4/28/95
 */

/*
modification history
--------------------
01l,22mar02,niq  Merged from Synth view, tor3_x.synth branch, ver 01q
01k,24jan02,niq  SPR 68672 - Add route to the local end of the PTP link with
                 the correct netmask
                 SPR 72415 - Added support for Route tags
01j,15oct01,rae  merge from truestack ver 01m, base 01h (SPR #69983 etc.)
01i,10nov00,spm  merged from version 01j of tor3_x branch (SPR #29099 fix)
01h,11sep98,spm  provided fatal error handling in place of quit routine,
                 replaced ripMakeAddr with optimized results (SPR #22350)
01g,01sep98,spm  modified addrouteforif to correctly support interfaces 
                 with classless netmasks (SPR #22220); changed spacing
                 for coding standards
01f,26jul98,spm  moved assignment from conditional to avoid compiler warnings
01e,06oct97,gnn  fixed SPR 9211 and added sendHook functionality
01d,16may97,gnn  added code to initialize hooks to NULL.
01c,07apr97,gnn  cleared up some of the more egregious warnings.
                 added MIB-II interfaces and options.
01b,24feb97,gnn  added rip version 2 functionality.
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
#include "net/if.h"
#include "net/if_dl.h"
#include "netinet/in.h"
#include "stdlib.h"
#include "logLib.h"
#include "m2Lib.h"
#include "sockLib.h"
#ifdef VIRTUAL_STACK
#include "netinet/vsLib.h"
#include "netinet/vsRip.h"
#endif /* VIRTUAL_STACK */

#define same(a1, a2) \
        (memcmp((a1)->sa_data, (a2)->sa_data, 14) == 0)

/* globals */

#ifndef VIRTUAL_STACK
IMPORT int		routedDebug;
IMPORT RIP		ripState;
struct interface *	ripIfNet;
struct interface **	ifnext = &ripIfNet;
#endif /* VIRTUAL_STACK */

/* locals */

#ifndef VIRTUAL_STACK
LOCAL int		foundloopback;		/* valid flag for loopaddr */
LOCAL struct sockaddr 	loopaddr;		/* our address on loopback */
LOCAL struct rt_addrinfo	info;
#endif /* VIRTUAL_STACK */

/* forward declarations */

IMPORT void _ripAddrsXtract (caddr_t, caddr_t, struct rt_addrinfo *); 
IMPORT int sysctl_rtable (int *, int, caddr_t, size_t *, caddr_t *, size_t);
IMPORT void _ripIfShow (struct interface *);
void addrouteforif(register struct interface *);
LOCAL int add_ptopt_localrt (register struct interface *, int);


/* defines */

/* Sleazy use of local variables throughout file, warning!!!! */

#define NETMASK	info.rti_info[RTAX_NETMASK]
#define IFAADDR	info.rti_info[RTAX_IFA]
#define BRDADDR	info.rti_info[RTAX_BRD]


/*****************************************************************************
*
* routedIfInit - initialize interface information
*
* This routine behaves differently based on the <ifindex> parameter.
* If <ifIndex> is 0, this routine searches the kernel's "ifnet" interface
* list for all interfaces belonging to the AF_INET family and copies the
* interface data into RIP private interface table list if that interface is UP.
* If <ifIndex> is non-zero then only the specified interface is queried
* and its data copied if the interface is UP.
*
* Interfaces that are down are ignored.
*
* For every interface retrieved from the kernel, this routine checks if
* a route for the interface address already exists in RIP's table. If it does
* this signifies that RIP already knows about this interface address; hence it
* skips over that entry. If a route doesn't exist, then RIP adds a route
* to that interface in its own table as well as in the system Routing database.
* The above procedure is repeated for each address entry present for an
* interface.
*
* If an interface has multiple IP addresses (aliases), this routine creates
* a different interface entry (virtual interface) for each address. 
* For example an interface "fei0" in the system with address "100.10.1.1"
* and "144.1.1.1" will result in two interfaces in RIP's interface list - 
* 	1) "fei0", 100.10.1.1
*	2) "fei0", 144.1.1.1

* The <resetFlag> parameter distinguishes the initial execution of this
* routine from later executions which add new interfaces to the list. 
* It is used to initialize RIP's interface list.
*
* If the sytem runs out of memory during uring the addition of an interface
* to RIP's internal list, or if is encounters some incomplete entries, 
* the "lookforinterfaces" flag is set to automatically repeat the execution
* of this routine at the next timer interval. 
*
* The initial execution of this routine (during RIP startup) must succeed
* or the session will not begin, but the session will continue regardless
* of the results of later executions.
*
* RETURNS: OK, or ERROR if unable to process interface table.
*
* NOMANUAL
*
* INTERNAL
* After the RIP session has started, the routine can be called again
* within the context of either the RIP timer task or the RIP input task. 
* The caller should take the ripLockSem semaphore to provide mutual
* exclusion so that no message processing happens while the 
* interface list is changed and any corresponding route entries are
* created.
*/

STATUS routedIfInit
    (
    BOOL 	resetFlag, 	/* build entire new list or add entries? */
    long		ifIndex		/* particular interface to search for, 0-all */
    )
    {
    struct interface ifs, *ifp;
    size_t needed;
    int mib[6], no_ipaddr = 0, flags = 0;
    char *buf, *cplim, *cp;
    register struct if_msghdr *ifm;
    register struct ifa_msghdr *ifam;
    struct sockaddr_dl *sdl = NULL;
    struct sockaddr_in *sin;
    struct ip_mreq ipMreq; 	/* Structure for joining multicast groups. */
    u_long i;

    if (resetFlag)
        {
        /* Reset globals in case of restart after shutdown. */

        foundloopback = 0;
        ripIfNet = NULL;
        ifnext = &ripIfNet;
        }

    ripState.lookforinterfaces = FALSE;

    ipMreq.imr_multiaddr.s_addr = htonl (RIP_MCAST_ADDR);

    /* 
     * The sysctl_rtable routine provides access to internal networking data 
     * according to the specified operations. The NET_RT_IFLIST operator
     * traverses the entire internal "ifnet" linked list created when the
     * network devices are initialized. The third argument, when non-zero,
     * restricts the search to a particular unit number. In this case, all
     * devices are examined. The first argument, when non-zero, limits the
     * addresses to the specified address family. 
     */

    mib[0] = AF_INET;
    mib[1] = NET_RT_IFLIST;
    mib[2] = ifIndex;

    /* 
     * The first call to the routine determines the amount of space needed
     * for the results. No data is actually copied.
     */

    if (sysctl_rtable(mib, 3, NULL, &needed, NULL, 0) < 0)
        {
        if (routedDebug)
            logMsg ("Error %x estimating size of interface buffer.\n", errno,
                    0, 0, 0, 0, 0);
        return (ERROR);
        }

    if (routedDebug)
        logMsg ("Executing routedInit for iIndex = %d.\n", ifIndex, 0, 0, 0, 0, 0);

    /* 
     * Allocate the required data, and repeat the system call to copy the
     * actual values.
     */

    buf = malloc (needed);
    if (buf == NULL)
        {
        if (routedDebug)
            logMsg ("Error %x allocating interface buffer.\n", errno,
                    0, 0, 0, 0, 0);
        return (ERROR);
        }

    if (sysctl_rtable(mib, 3, buf, &needed, NULL, 0) < 0)
        {
        if (routedDebug)
            logMsg ("Error %x retrieving interface table.\n", errno,
                    0, 0, 0, 0, 0);
        free (buf);
        return (ERROR);
        }

    /* End of VxWorks specific stuff. */

    /* 
     * Analyze the retrieved data. The provided buffer now contains a 
     * structure of type if_msghdr for each interface followed by zero 
     * or more structures of type ifa_msghdr for each address for that
     * interface. The if_msghdr structures have type fields of
     * RTM_IFINFO and the address structures have type fields of 
     * RTM_NEWADDR.
     */

    cplim = buf + needed;
    for (cp = buf; cp < cplim; cp += ifm->ifm_msglen) 
        {
        ifm = (struct if_msghdr *)cp;

        if (ifm->ifm_type == RTM_IFINFO) 
            {
            /* Parse the generic interface information. */

            memset(&ifs, 0, sizeof(ifs));
            ifs.int_flags = flags = (0xffff & ifm->ifm_flags) | IFF_INTERFACE;

            /* 
             * The sockaddr_dl structure containing the link-level address
             * immediately follows the if_msghdr structure.
             */

            sdl = (struct sockaddr_dl *) (ifm + 1);
            sdl->sdl_data[sdl->sdl_nlen] = 0;
            no_ipaddr = 1;
            continue;
            }

        /* 
         * Only address entries should be present at this point.
         * Exit if an unknown type is detected.
         */

        if (ifm->ifm_type != RTM_NEWADDR)
            {
            if (routedDebug)
                logMsg ("Unexpected entry in interface table.\n", 
                        0, 0, 0, 0, 0, 0);
            free (buf);
            return (ERROR);
            }

        /* If RIP is not desired on this interface, then so be it */

        if (ripIfExcludeListCheck (sdl->sdl_data) == OK)
            continue;

        /* 
         * Ignore the address information if the interface initialization
         * is incomplete. Set flag to repeat the search at the next timer 
         * interval.
         */

        if ( (flags & IFF_UP) == 0)
            {
            continue;
            }

        /* Access the address information through the ifa_msghdr structure. */

        ifam = (struct ifa_msghdr *)ifm;

        /* Reset the pointers to access the address pointers. */

        info.rti_addrs = ifam->ifam_addrs;
        _ripAddrsXtract ((char *)(ifam + 1), cp + ifam->ifam_msglen, &info);
        if (IFAADDR == 0) 
            {
            if (routedDebug)
                logMsg ("No address for %s.\n", (int)sdl->sdl_data,
                        0, 0, 0, 0, 0);
            continue;
            }

        /* Copy and process the actual address values. */

        ifs.int_addr = *IFAADDR;
        if (ifs.int_addr.sa_family != AF_INET)
            continue;
        no_ipaddr = 0;

        if (ifs.int_flags & IFF_POINTOPOINT)
            {
            if (BRDADDR == 0)    /* No remote address specified? */
                {
                if (routedDebug)
                    logMsg ("No dstaddr for %s.\n", (int)sdl->sdl_data, 
                            0, 0, 0, 0, 0);
                continue;
                }

            if (BRDADDR->sa_family == AF_UNSPEC)
                {
                ripState.lookforinterfaces = TRUE;
                continue;
                }

            /* Store the remote address for the link in the correct place. */

            ifs.int_dstaddr = *BRDADDR;
            }

        /*
         * Skip interfaces we already know about. But if there are
         * other interfaces that are on the same network as the ones
         * we already know about, we want to know about them as well.
         */

        if (ifs.int_flags & IFF_POINTOPOINT)
            {
            if (ripIfWithDstAddrAndIndex (&ifs.int_dstaddr, NULL, 
                                          ifam->ifam_index))
                continue;
            }
        else if (ripIfWithAddrAndIndex (&ifs.int_addr, ifam->ifam_index))
            continue;

        if (ifs.int_flags & IFF_LOOPBACK)
            {
            ifs.int_flags |= IFF_PASSIVE;
            foundloopback = 1;
            loopaddr = ifs.int_addr;
            for (ifp = ripIfNet; ifp; ifp = ifp->int_next)
                 if (ifp->int_flags & IFF_POINTOPOINT)
                     add_ptopt_localrt(ifp, 0);
            }

        /* Assign the broadcast address for the interface, if any. */

        if (ifs.int_flags & IFF_BROADCAST)
            {
            if (BRDADDR == 0)
                {
                if (routedDebug)
                    logMsg ("No broadcast address for %s.\n",
                            (int)sdl->sdl_data, 0, 0, 0, 0, 0);
                continue;
                }
            ifs.int_dstaddr = *BRDADDR;
            }

         /*
          * Use a minimum metric of one; treat the interface metric 
          * (default 0) as an increment to the hop count of one.
          */

        ifs.int_metric = ifam->ifam_metric + 1;

        /* Assign the network and subnet values. */

        if (NETMASK == 0) 
            {
            if (routedDebug)
                logMsg ("No netmask for %s.\n", (int)sdl->sdl_data,
                        0, 0, 0, 0, 0);
            continue;
            }

        /* 
         * For AF_INET addresses, the int_subnetmask field copied here
         * defaults to the class-based value for the associated int_addr 
         * field, but will contain some other value if explicity specified.
         */

        sin = (struct sockaddr_in *)NETMASK;
        ifs.int_subnetmask = ntohl (sin->sin_addr.s_addr);

        /*
         * The network number stored here may differ from the original
         * value accessed through the ifnet linked list. The original value
         * is equal to the class-based setting by default, but is reset
         * to the same value as the int_subnetmask field if a less
         * specific (i.e. supernetted) value is assigned. This value is
         * always set equal to the class-based value, even if a supernet
         * is specified.
         */

        sin = (struct sockaddr_in *)&ifs.int_addr;
        i = ntohl(sin->sin_addr.s_addr);
        if (IN_CLASSA(i))
            ifs.int_netmask = IN_CLASSA_NET;
        else if (IN_CLASSB(i))
            ifs.int_netmask = IN_CLASSB_NET;
        else
            ifs.int_netmask = IN_CLASSC_NET;
        ifs.int_net = i & ifs.int_netmask;

        ifs.int_subnet = i & ifs.int_subnetmask;

        /* 
         * The IFF_SUBNET flag indicates that the interface does not use the
         * class-based mask stored in the int_netmask field: it may be more 
         * or less specific than that value.
         */

        if (ifs.int_subnetmask != ifs.int_netmask)
            ifs.int_flags |= IFF_SUBNET;

        ifp = (struct interface *)malloc (sdl->sdl_nlen + 1 + sizeof(ifs));
        if (ifp == 0)
            {
            if (routedDebug)
                logMsg ("routedIfInit: out of memory.\n", 0, 0, 0, 0, 0, 0);
            ripState.lookforinterfaces = TRUE;
            break;
            }
        *ifp = ifs;

        /*
         * Count the # of directly connected networks
         * and point to point links which aren't looped
         * back to ourself.  This is used below to
         * decide if we should be a routing ``supplier''.
         */

        if ( (ifs.int_flags & IFF_LOOPBACK) == 0 &&
            ( (ifs.int_flags & IFF_POINTOPOINT) == 0 ||
             ripIfWithAddr (&ifs.int_dstaddr) == 0))
            ripState.externalinterfaces++;

        /*
         * If we have a point-to-point link, we want to act
         * as a supplier even if it's our only interface,
         * as that's the only way our peer on the other end
         * can tell that the link is up.
         */

        if ( (ifs.int_flags & IFF_POINTOPOINT) && ripState.supplier < 0)
            ripState.supplier = 1;

        /* 
         * Copy the interface name specified in the link-level address. 
         * into the space provided.
         */

        ifp->int_name = (char *)(ifp + 1);
        strcpy (ifp->int_name, sdl->sdl_data);

        /* Set the index of the interface */

        ifp->int_index = ifam->ifam_index;

        /* Add the given entry to the ripIfNet linked list. */

        *ifnext = ifp;
        ifnext = &ifp->int_next;

        /* Create a route to the network reachable with the new address. */

        addrouteforif (ifp);

        /* Set the MIB variables for the configuration on this interface. */

        bzero ( (char *)&ifp->ifStat, sizeof(ifp->ifStat));
        bzero ( (char *)&ifp->ifConf, sizeof(ifp->ifConf));

        sin = (struct sockaddr_in *)&ifs.int_addr;
        i = sin->sin_addr.s_addr;
        ifp->ifStat.rip2IfStatAddress = i;
        ifp->ifConf.rip2IfConfAddress = i;
        ifp->ifConf.rip2IfConfAuthType = ripState.ripConf.rip2IfConfAuthType;
        bcopy (ripState.ripConf.rip2IfConfAuthKey,
               ifp->ifConf.rip2IfConfAuthKey, 16);
        ifp->ifConf.rip2IfConfSend = ripState.ripConf.rip2IfConfSend;
        ifp->ifConf.rip2IfConfReceive = ripState.ripConf.rip2IfConfReceive;
        ifp->ifConf.rip2IfConfDefaultMetric = 
                                      ripState.ripConf.rip2IfConfDefaultMetric;
        ifp->ifConf.rip2IfConfStatus = ripState.ripConf.rip2IfConfStatus;
        ifp->authHook = NULL;
        ifp->leakHook = NULL;
        ifp->sendHook = NULL;

        /*
         * If multicasting is enabled and the interface supports it, 
         * attempt to join the RIP multicast group.
         */

        if (ripState.multicast && (ifs.int_flags & IFF_MULTICAST))
            {
            ipMreq.imr_interface.s_addr = i;

            if (setsockopt (ripState.s, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                            (char *)&ipMreq, sizeof (ipMreq)) < 0)
                {
                if (routedDebug)
                    logMsg ("setsockopt IP_ADD_MEMBERSHIP error:\n",
                            0, 0, 0, 0, 0, 0);
                }
            }
        }

    if (ripState.externalinterfaces > 1 && ripState.supplier < 0)
        ripState.supplier = 1;
    free (buf);


    return (OK);
    }

/*
 * Add route for interface if not currently installed.
 * Create route to other end if a point-to-point link,
 * otherwise a route to this (sub)network.
 * INTERNET SPECIFIC.
 */
void addrouteforif(ifp)
	register struct interface *ifp;
{
	struct sockaddr_in net;
	struct sockaddr *dst;
	int state;
	register struct rt_entry *rt;
        int refcnt_save = 0;
        int subnets_cnt_save = 0;

        /* Clear the fact there there are no routes for this interface */
        ifp->int_flags &= ~IFF_ROUTES_DELETED;

        /*
         * The algorithm explained....
         * We are going to add  route(s) for the interface.
         * The following cases exist:
         *	1) The interface is not PPP and  not-subnetted 
         *	   e.g, 164.10.1.1/16
         *         In this case we just want to add the following route
         *            164.10.0.0 255.255.0.0 --> 164.10.1.1
         *         If there already exists another route for that same network,
         *            if that route is not an internally generated route
         *               increment rt_refcnt 
         *            else
         *               Delete the existing route and save its ref counts
         *               Add route through this interface.
         *               Set the reference counts from the previous route.
         *               increment rt_refcnt and rt_subnets_cnt
         *         else
         *            Add the route through the inetrface
         *            Set rt_refcnt and rt_subnets_cnt to zero
         *
         *	2) The interface is not PPP and is subnetted 
         *	   e.g, 164.10.2.1/24
         *         In this case we want to add the following routes
         *            164.10.2.0 255.255.255.0 --> 164.10.2.1
         *            164.10.0.0 255.255.0.0   --> 164.10.2.1 (Internal route)
         *         The internal route is used for border gateway filtering.
         *
         *         If there already exists another route for that same 
         *          (sub)network,
         *            if that route is not an internally generated route
         *               increment rt_refcnt 
         *            else
         *               Delete the existing route and save its ref counts
         *               Add route through this interface.
         *               Set the reference counts from the previous route.
         *               increment rt_refcnt and rt_subnets_cnt
         *         else
         *            Add the route through the inetrface
         *            Set rt_refcnt and rt_subnets_cnt to zero
         *         If there exists a route for the class based network
         *            increment the rt_refcnt and rt_subnets_cnt for the route
         *         else
         *            Add the internal class based route. 
         *            Set both the ref counts to zero.
         *
         *	3) The interface is PPP
         *	   e.g, 164.10.1.1/32 --> 124.10.2.2
         *         In this case we just want to add the following routes
         *            124.10.2.2   255.255.255.255  --> 164.10.1.1
         *            164.10.1.1   255.255.255.255  --> 127.0.0.1
         *         If there already exists another route for the destination
         *            increment rt_refcnt 
         *         else
         *            Add the route through the inetrface
         *            Set rt_refcnt and rt_subnets_cnt to zero
         *         Add a route for the local address of the link thru the 
         *          loopback interface.
         */


        /* Check if there already exists a route for the network (remote end)*/
	if (ifp->int_flags & IFF_POINTOPOINT)
		dst = &ifp->int_dstaddr;
	else {
		memset(&net, 0, sizeof (net));
                net.sin_len = sizeof (net);
		net.sin_family = AF_INET;
		net.sin_addr.s_addr = htonl (ifp->int_subnet);
		dst = (struct sockaddr *)&net;
	}
        if ((rt = rtlookup(dst)) == NULL) 
            rt = rtfind(dst);

	if (rt)
            {
            /* 
             * If we have an indirect route to the destination, 
             * delete it as we now have a direct route
             */
            if ((rt->rt_state & RTS_INTERFACE) == 0)
                rtdelete (rt);
            else
                {
                if (ifp->int_flags & IFF_POINTOPOINT)
                    {
                    rt->rt_refcnt++;
                    if (ifp->int_flags & IFF_POINTOPOINT && foundloopback) 
                        add_ptopt_localrt(ifp, 0);
                    return;
                    }

                if ((rt->rt_state & (RTS_INTERFACE | RTS_INTERNAL))
                        == RTS_INTERFACE)
                    {
                    /* The route is not an internal route */
                    if (rt->rt_ifp->int_subnetmask == ifp->int_subnetmask)
                        {
                        /* 
                         * This interface is on the same (sub)network as the
                         * retrieved route. Increment the reference count.
                         */
                        rt->rt_refcnt++;

                        /*
                         * If the interface uses classless addressing, 
                         * an additional entry is stored in the routing table
                         * which can be correctly interpreted by RIP-1 and 
                         * RIP-2 routers that are not directly connected to
                         * the interface's network. The appropriate entry is
                         * selected by the border gateway filtering performed
                         * when updates are generated.
                         */

                        if (ifp->int_flags & IFF_SUBNET) 
                            {
                            net.sin_addr.s_addr = htonl (ifp->int_net);
                            if ((rt = rtlookup(dst)) == NULL) 
                                rt = rtfind(dst);
                            if (rt == 0)
                                {
                                if (routedDebug) 
                                    logMsg ("Error! Didn't find classful route"
                                            " for %x, interface %s.\n", 
                                            (int)ifp->int_net, 
                                            (int)ifp->int_name, 0, 0, 0, 0);
                                }
                            else 
                                {
                                rt->rt_refcnt++;
                                rt->rt_subnets_cnt++;
                                if ((rt->rt_state & (RTS_INTERNAL|RTS_SUBNET))
                                    == (RTS_INTERNAL|RTS_SUBNET) && 
                                    (ifp->int_metric < rt->rt_metric))
                                    rtchange (rt, &ifp->int_addr, 
                                              ifp->int_metric, NULL, 0, 0, 
                                              ifp);
                                }
                            }
                        return;
                        }
                    }
                else
                    {
                    /*
                     * If this was a classful route installed by an earlier
                     * subnetted interface, save the reference count values
                     * as we'll need to restore them on the route that will
                     * be installed later
                     */
                    refcnt_save = rt->rt_refcnt + 1; 
                    subnets_cnt_save = rt->rt_subnets_cnt + 1;
                    rtdelete (rt);
                    }
                }
            }

	/*
	 * If the interface uses classless addressing, an additional entry
         * is stored in the routing table which can be correctly interpreted
         * by RIP-1 and RIP-2 routers that are not directly connected to
         * the interface's network. The appropriate entry is selected by the
         * border gateway filtering performed when updates are generated.
	 */

	if ((ifp->int_flags & (IFF_SUBNET|IFF_POINTOPOINT)) == IFF_SUBNET) {
		struct in_addr subnet;

		subnet = net.sin_addr;
		net.sin_addr.s_addr = htonl (ifp->int_net);
		if ((rt = rtlookup(dst)) == NULL) 
		    rt = rtfind(dst);
		if (rt == 0) 
                    rt = rtadd(dst, &ifp->int_addr, ifp->int_metric,
                               ((ifp->int_flags & (IFF_INTERFACE|IFF_REMOTE)) |
                                RTS_PASSIVE | RTS_INTERNAL | RTS_SUBNET), 
                               NULL, M2_ipRouteProto_rip, 0, 0, ifp);
		else 
                    {
                    rt->rt_refcnt++; 
                    rt->rt_subnets_cnt++; 
                    if ((rt->rt_state & (RTS_INTERNAL|RTS_SUBNET)) == 
                        (RTS_INTERNAL|RTS_SUBNET) && 
                        (ifp->int_metric < rt->rt_metric))
                        rtchange(rt, &ifp->int_addr, ifp->int_metric, NULL, 
                                 0, 0, ifp);
                    }
		net.sin_addr = subnet;
	}
	if (ifp->int_transitions++ > 0)
            if (routedDebug)
		logMsg ("Warning! Re-installing route for interface %s.\n", 
                        (int)ifp->int_name, 0, 0, 0, 0, 0);
	state = ifp->int_flags &
	    (IFF_INTERFACE | IFF_PASSIVE | IFF_REMOTE | IFF_SUBNET);
	if (ifp->int_flags & IFF_POINTOPOINT && 
            (ntohl(((struct sockaddr_in *)&ifp->int_dstaddr)->sin_addr.s_addr)
             & ifp->int_netmask) != ifp->int_net)
		state &= ~RTS_SUBNET;
	if (ifp->int_flags & IFF_LOOPBACK)
		state |= RTS_EXTERNAL;
	rt = rtadd(dst, &ifp->int_addr, ifp->int_metric, state, NULL, 
                   M2_ipRouteProto_rip, 0, 0, ifp);
        /*
         * Now set the reference counts.
         * This might be a network route that replaced a previous
         * classful route. If we had any references on that earlier
         * route we need to transfer them here
         */
        rt->rt_refcnt = refcnt_save;
        rt->rt_subnets_cnt = subnets_cnt_save;

	if (ifp->int_flags & IFF_POINTOPOINT && foundloopback)
                    add_ptopt_localrt(ifp, 0);
    }

/*
 * Add route to local end of point-to-point using loopback.
 * If a route to this network is being sent to neighbors on other nets,
 * mark this route as subnet so we don't have to propagate it too.
 */
int add_ptopt_localrt(ifp, refcnt)
    register struct interface *ifp;
    int refcnt;
    {
    struct rt_entry *rt;
    struct sockaddr *dst;
    struct sockaddr_in net;
    int state;
    struct sockaddr_in hostMask = {
    sizeof (struct sockaddr_in),
    AF_INET, 0, {INADDR_BROADCAST} };

    state = RTS_INTERFACE | RTS_PASSIVE;

    /* look for route to logical network */
    memset(&net, 0, sizeof (net));
    net.sin_len = sizeof (net);
    net.sin_family = AF_INET;
    net.sin_addr.s_addr = htonl (ifp->int_net);
    dst = (struct sockaddr *)&net;
    if ((rt = rtlookup(dst)) == NULL) 
        rt = rtfind(dst);
    if (rt && rt->rt_state & RTS_INTERNAL)
        state |= RTS_SUBNET;

    dst = &ifp->int_addr;
    rt = rtfind (dst);
    if (rt) 
        {
        if (rt->rt_state & RTS_INTERFACE)
            {
            /*
             * We are piggybacking onto somebody else' route. So we
             * should increment the ref count
             */
            if (refcnt == 0)
                refcnt = 1;

            rt->rt_refcnt += refcnt; 
            return (ERROR);
            }
        rtdelete(rt);
        }

    rt = rtadd(dst, &loopaddr, 1, state, (struct sockaddr *)&hostMask,
               M2_ipRouteProto_rip, 0, 0, NULL);
    if (rt)
        rt->rt_refcnt = refcnt;

    return (OK);
    }

/*
 * Add route for interface or an internal network route for
 * a subnetted interface
 * This routine is called from the rtdelete() routine when an
 * interface route is deleted and there exists other references to that
 * route. This routine will add the route through another interface
 * and adjust the reference count accordingly.
 * The internalFlag is set when an internally generated classful route
 * should be added. If the flag is not set, then a regular interface
 * route is added.
 * In the simplest of cases, wherein the route being deleted was referenced
 * by another interface on the same network, a new route is added through that
 * other interface.
 *
 * In other cases, the network route being deleted might be referenced by
 * point to point interfaces on the same network: e.g
 *   164.10.0.0/16  -->  164.10.1.1   (being deleted)
 *   is referenced by the following PPP interface
 *      src = 164.10.22.1, dst=164.10.22.2
 * In the above case the rt_refcnt field of the route (prior to rtdelete)
 * would be 2 and rt_subnets_cnt would be 0. On call to this routine,
 * refCnt would be 1 and subnetsCnt would be 0
 * This routine would need to add the following routes in response to the
 * deletion of the 164.10.0.0/16  -->  164.10.1.1 route:
 *    164.10.22.2/32 --> 164.10.22.1
 *    164.10.22.1/32 --> 127.0.0.1
 * Both the above routes should have rt_refcnt and rt_subnets_cnt set to zero
 *
 * There are many other cases where just the local or the remote end of the PPP
 * interface may be referencing the route being deleted, or there may be
 * multiple such interfaces. 
 *     
 * INPUT
 *    pAddr     -  Destination of the interface route being deleted
 *    refCnt    -  How many more references to this route exist
 *    subnetsCnt-  How many subnetted interfaces exist for this network
 *    internalFlag-Should an internal or a regular interface route be added
 *    subnetMask-  Mask associated with the route being deleted
 *
 * NOMANUAL
 *
 * NOTE: INTERNET SPECIFIC.
 */
void ifRouteAdd
    (
    struct sockaddr *	pAddr,
    int			refCnt,
    int			subnetsCnt,
    BOOL		internalFlag,
    u_long		subnetMask
    )
    {
    register struct interface *ifp = 0;
    register struct interface *pPPPIfp = 0;
    register struct interface *pNetIfp = 0;
    register struct interface *pSubnetIfp = 0;
    register struct interface *pIfMax;
    register int af = pAddr->sa_family;
    register u_long dstaddr;
    int addLocalRoute = 0;
    int addDstRoute = 0;
    int state; 
    struct rt_entry *rt;

    if (af >= AF_MAX) 
        return;

    pIfMax = 0;

    dstaddr = ntohl (((struct sockaddr_in *)pAddr)->sin_addr.s_addr);

    /*
     * If we need to add an "internal" route (a classful route for a
     * subnetted network - to be used for border gateway filtering)
     * Look for a list of subnetted interfaces that are on the same network
     * as the current route, and add a class based network route through
     * the interface that has the least metric
     */
    if (internalFlag)
        {
        for (ifp = ripIfNet; ifp; ifp = ifp->int_next) 
            {
            if (ifp->int_flags & IFF_REMOTE || !(ifp->int_flags & IFF_UP) || 
                ifp->int_flags & IFF_POINTOPOINT)
                continue;
            if (af != ifp->int_addr.sa_family) 
                continue;

            if ((ifp->int_netmask != ifp->int_subnetmask) && 
                (dstaddr == ifp->int_net)) 
                {
                if (!pSubnetIfp || pSubnetIfp->int_metric > ifp->int_metric)
                    pSubnetIfp = ifp;

                if (routedDebug > 2)
                    {
                    logMsg ("internalFlag: found ifp = %x\n", (int)pSubnetIfp, 0,
                            0, 0, 0, 0);
                    _ripIfShow (pSubnetIfp);
                    }
                }
            }
        ifp = pSubnetIfp;
        }
    else
        {
        for (ifp = ripIfNet; ifp; ifp = ifp->int_next) 
            {
            if (ifp->int_flags & IFF_REMOTE || !(ifp->int_flags & IFF_UP))
                continue;
            if (af != ifp->int_addr.sa_family) 
                continue;

            /* See if the address matches any end of the PPP links */
            if ((ifp->int_flags & IFF_POINTOPOINT)) 
                {
                if (same (&ifp->int_dstaddr, pAddr) || 
                    same (&ifp->int_addr, pAddr)) 
                    {
                    if (!pPPPIfp || pPPPIfp->int_metric > ifp->int_metric)
                        pPPPIfp = ifp; 
                    if (routedDebug > 2)
                        {
                        logMsg ("PPP: found ifp = %x\n", (int)pPPPIfp, 0,
                                0, 0, 0, 0);
                        _ripIfShow (pPPPIfp);
                        }
                    }
                }

            /*
             * It would seem that the two else if cases below could be
             * replaced with a single if (dstaddr == ifp->int_subnet)
             * but the above case will not be able to distinguish between
             * 164.10.0.0/16 and 164.10.0.0/24. Hence we need to
             * compare separately
             */
            else if ((ifp->int_netmask == ifp->int_subnetmask) &&
                     (dstaddr == ifp->int_net)) 
                {
                if (!pNetIfp || pNetIfp->int_metric > ifp->int_metric)
                    pNetIfp = ifp;
                if (routedDebug > 2)
                    {
                    logMsg ("NET: found ifp = %x\n", (int)pNetIfp, 0,
                            0, 0, 0, 0);
                    _ripIfShow (pNetIfp);
                    }
                }
            else if ((ifp->int_netmask != ifp->int_subnetmask) &&
                     (dstaddr == ifp->int_subnet)) 
                {
                if (!pSubnetIfp || pSubnetIfp->int_metric > ifp->int_metric)
                    pSubnetIfp = ifp;
                if (routedDebug > 2)
                    {
                    logMsg ("SUBNET: found ifp = %x\n", (int)pSubnetIfp, 0,
                            0, 0, 0, 0);
                    _ripIfShow (pSubnetIfp);
                    }
                }
            }

        ifp = pPPPIfp ? pPPPIfp : (pNetIfp ? pNetIfp : pSubnetIfp);
        if (ifp == NULL)
            {
            /*
             * We did not find any interface that matched the address of the
             * route being deleted. This must be the following case:
             *   164.10.0.0/16  -->  164.10.1.1   (being deleted)
             *   is referenced by the following PPP interface
             *      src = 164.10.22.1, dst=164.10.22.2
             * So now, search for PPP interfaces that lie on the same
             * network as the route being deleted. We do that by applying the
             * netmask of the route to the PPP interface address. 
             * If we have both ends of the link on the same network, then we
             * will have to add two routes:
             *   one for the remote end.
             *   one for the local end
             */
            pAddr = NULL;
            for (ifp = ripIfNet; ifp; ifp = ifp->int_next) 
                {
                if (ifp->int_flags & IFF_REMOTE || !(ifp->int_flags & IFF_UP))
                    continue;
                if (af != ifp->int_addr.sa_family) 
                    continue;

                if ((ifp->int_flags & IFF_POINTOPOINT)) 
                    {
                    if ((ntohl(((struct sockaddr_in *)
                                &ifp->int_dstaddr)->sin_addr.s_addr) & 
                         subnetMask) == dstaddr)
                        {
                        addDstRoute = 1;
                        if (routedDebug > 2)
                            {
                            logMsg ("Add PPP dest route for %x.\n", (int)ifp, 0,
                                    0, 0, 0, 0);
                            _ripIfShow (ifp);
                            }
                        if (!pPPPIfp || pPPPIfp->int_metric > ifp->int_metric)
                            pPPPIfp = ifp; 
                        pAddr = &ifp->int_dstaddr;
                        }
                    if ((ntohl(((struct sockaddr_in *)
                                &ifp->int_addr)->sin_addr.s_addr) & 
                         subnetMask) == dstaddr)
                        {
                        addLocalRoute = 1;
                        if (routedDebug > 2)
                            {
                            logMsg ("Add PPP local route for %x.\n", (int)ifp, 0,
                                    0, 0, 0, 0);
                            _ripIfShow (ifp);
                            }
                        if (!pPPPIfp || pPPPIfp->int_metric > ifp->int_metric)
                            pPPPIfp = ifp; 
                        break;
                        }
                    }
                }
            ifp = pPPPIfp;
            }
        }

    if (ifp == NULL)
        {
        if (routedDebug > 2)
            logMsg ("No ifp found\n", 0, 0, 0, 0, 0, 0);
        return;
        }

    /*
     * If both the local and remote end were on the same network, we had
     * incremented the refcount by 2 when we added the route. So account
     * for that. rtdelete() had already decremented the refcount by 1
     */
    if (addDstRoute && addLocalRoute)
        refCnt--;

    /*
     * pAddr will be set to NULL if we just need to add a route to the
     * local end of the PPP link. In all other cases, add the route
     */
    if (pAddr)
        {
        state = ifp->int_flags &
            (IFF_INTERFACE | IFF_PASSIVE | IFF_REMOTE | IFF_SUBNET);
        if (ifp->int_flags & IFF_POINTOPOINT && 
            (same (&ifp->int_dstaddr, pAddr)) &&
            (ntohl(((struct sockaddr_in *)&ifp->int_dstaddr)->sin_addr.s_addr) 
             & ifp->int_netmask) != ifp->int_net)
                state &= ~RTS_SUBNET;

        if (internalFlag)
            rt = rtadd(pAddr, &ifp->int_addr, ifp->int_metric,
                       ((ifp->int_flags & (IFF_INTERFACE|IFF_REMOTE)) |
                        RTS_PASSIVE | RTS_INTERNAL | RTS_SUBNET), 
                       NULL, M2_ipRouteProto_rip, 0, 0, ifp);
        else
            rt = rtadd(pAddr, &ifp->int_addr, ifp->int_metric, state, NULL, 
                       M2_ipRouteProto_rip, 0, 0, ifp);
        /*
         * Now set the reference counts.
         */
        if (rt)
            {
            rt->rt_refcnt = refCnt;
            rt->rt_subnets_cnt = subnetsCnt;
            }
        }

    /* Now add route to the local end of PPP link, if needed */
    if (addLocalRoute) 
        add_ptopt_localrt(ifp, refCnt);
        
    }
