/* input.c - routines for processing incoming RIP messages */

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
 *        @(#)input.c	8.3 (Berkeley) 4/28/95
 */

/*
modification history
--------------------
01s,22mar02,niq  Merged from Synth view, tor3_x.synth branch, ver 01z
01r,24jan02,niq  SPR 72415 - Added support for Route tags
01q,15oct01,rae  merge from truestack ver 01t, base 01n (SPRs 70188, 69983 etc.)
01p,21nov00,spm  fixed transmission of responses to wrong port (SPR #62532)
01o,10nov00,spm  merged from version 01p of tor3_x branch (SPR #33692 fix)
01n,16mar99,spm  recovered orphaned code from tor1_0_1.sens1_1 (SPR #25770)
01m,29sep98,spm  updated route modification time for IP group MIB (SPR #9374)
01l,11sep98,spm  general overhaul of input processing (SPR #22297); removed
                 all references to bloated trace commands (SPR #22350)
01k,01sep98,spm  changed test of next hop address to detect supernet 
                 matches (SPR #22220) and comply with RIP specification;
                 modified whitespace (for coding standards) and added comments
01j,26jun98,spm  corrected timer setting for poisoned routes; removed duplicate
                 tests from response processing; updated debug messages and
                 added typecast to remove compiler warning
01i,03oct97,gnn  added includes and declarations to remove warnings
01h,15may97,gnn  fixed errors relating to ripSplitPacket.
01g,08may97,gnn  fixed more ANVL related errors.
01f,17apr97,gnn  fixed errors pointed out by ANVL.       
01e,14apr97,gnn  added input packet authentication.
01d,07apr97,gnn  cleared up some of the more egregious warnings.
                 added MIB-II interface and option support.
01c,12mar97,gnn  added multicast support.
                 added time variables.
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
#include "tickLib.h"
#include "rip/defs.h"
#include "m2Lib.h"
#include "routeEnhLib.h"

#ifdef VIRTUAL_STACK
#include "netinet/vsLib.h"
#include "netinet/vsRip.h"
#else
IMPORT int routedDebug;
#endif

/* The random number generation limits the frequency of triggered updates. */

LOCAL u_long ripRandTimeSeed = 1;
LOCAL u_long ripRandTime (void);
LOCAL void ripRouteAgeUpdate(struct rt_entry *pRtEntry);

#define RANDOMDELAY()    (MIN_WAITTIME * 1000000 + \
               ripRandTime() % ((MAX_WAITTIME - MIN_WAITTIME) * 1000000))

#define osa(x) ((struct osockaddr *)(&(x)))

/* forward declarations */

void addrouteforif(register struct interface *ifp);
void ripSplitPacket (struct interface* pIfp, struct sockaddr_in *src,
                     struct sockaddr* orig, struct sockaddr* gateway, 
                     struct sockaddr* netmask, int* pTag );
void timevaladd (struct timeval *t1, struct timeval *t2);

IMPORT STATUS ripAuthCheck (char *, RIP_PKT *);
IMPORT void ripSockaddrPrint (struct sockaddr *);

/*
 * Process a newly received packet.
 */

void routedInput
    (
    struct sockaddr *from,
    register RIP_PKT *rip,
    int size
    )
    {
    register struct rt_entry *rt;
    register struct netinfo *n;
    register struct netinfo *pFirst;
    register struct interface *ifp;
    register struct interface *pErrorIfp;
    struct interface *ripIfWithDstAddr();
    int count, changes = 0;
    register struct afswitch *afp;
    static struct sockaddr badfrom, badfrom2;
    struct sockaddr gateway;
    struct sockaddr netmask;
    int tag;
    int origMetric;

#ifndef VIRTUAL_STACK
    IMPORT RIP ripState;
#endif

    ifp = 0;

    if (routedDebug > 2)
        logMsg ("Received RIP message.\n", 0, 0, 0, 0, 0, 0);

    if (from->sa_family >= AF_MAX ||
        (afp = &afswitch[from->sa_family])->af_hash == (int (*)())0)
        {
        if (routedDebug)
            logMsg ("Command %d received from unsupported address family %d.\n",
                    rip->rip_cmd, from->sa_family, 0, 0, 0, 0);
        return;
        }

    /* 
     * Determine the interface connected to the source of the 
     * RIP message. If no match is found, the message will be ignored.
     */

    pErrorIfp = ripIfLookup (from);
    if (pErrorIfp == NULL)
        {
        if (routedDebug)
            logMsg ("RIP message received from unreachable router %s.\n",
                    (int)(*afswitch[from->sa_family].af_format)(from),
                    0, 0, 0, 0, 0);
         return;
         }

    /* If the interface has been disabled, silently drop received packets. */

    if (pErrorIfp->ifConf.rip2IfConfStatus != M2_rip2IfConfStatus_valid)
        return;

    /* 
     * Test for valid version numbers and filter RIP messages based on the 
     * current setting of the interface's receive control switch. 
     */

    if (rip->rip_vers == 0)
        {
        if (routedDebug)
            logMsg ("RIP version 0 packet received from %s! (cmd %d)\n",
                    (int)(*afswitch[from->sa_family].af_format)(from), 
                    rip->rip_cmd, 0, 0, 0, 0);
        pErrorIfp->ifStat.rip2IfStatRcvBadPackets++;
        return;
        }

    if (rip->rip_vers == 1 && 
            pErrorIfp->ifConf.rip2IfConfReceive == M2_rip2IfConfReceive_rip2)
        {
        if (routedDebug)
            logMsg ("RIP-1 message rejected by receive control switch.\n",
                     0, 0, 0, 0, 0, 0);
        pErrorIfp->ifStat.rip2IfStatRcvBadPackets++;
        return;
        }

    if (rip->rip_vers == 2 && 
            pErrorIfp->ifConf.rip2IfConfReceive == M2_rip2IfConfReceive_rip1)
        {
        if (routedDebug)
            logMsg ("RIP-2 message rejected by receive control switch.\n",
                     0, 0, 0, 0, 0, 0);
        pErrorIfp->ifStat.rip2IfStatRcvBadPackets++;
        return;
        }

    /* 
     * Handle message authentication. If authentication is active, only 
     * messages validated by the authentication hook are accepted. That 
     * routine may allow RIP-1 messages if desired and must validate all 
     * RIP-2 messages.
     */

    if (pErrorIfp->ifConf.rip2IfConfAuthType ==
            M2_rip2IfConfAuthType_noAuthentication)
        {
        /*
         * Authentication is not enabled. Discard any RIP-2 messages 
         * containing authentication entries.
         */

        if (rip->rip_vers == 2)
            {
            pFirst = rip->rip_nets;
            if (ntohs(osa(pFirst->rip_dst)->sa_family) == RIP2_AUTH)
                {
                if (routedDebug)
                    logMsg ("Discarding message with unused authentication.\n",
                            0, 0, 0, 0, 0, 0);
                pErrorIfp->ifStat.rip2IfStatRcvBadPackets++;
                return;
                }
            }

        /* Get the first rip entry for processing */

        pFirst = n = rip->rip_nets;
        }
    else
        {
        /* 
         * Authenticate message. All RIP-2 messages without authentication
         * entries are rejected. RIP-2 messages with the current MD5 key
         * or simple password are accepted. Under MD5 authentication, all
         * other RIP-2 messages and all RIP-1 messages are rejected. Under
         * simple password authentication, those messages are still accepted
         * if the hook routine returns OK but are rejected if no hook routine
         * is installed.
         */

        if (rip->rip_vers == 2)
            {
            pFirst = rip->rip_nets;
            if (ntohs (osa (pFirst->rip_dst)->sa_family) != RIP2_AUTH)
                {
                if (routedDebug)
                    logMsg ("Discarding unauthenticated RIP-2 message.\n",
                            0, 0, 0, 0, 0, 0);
                pErrorIfp->ifStat.rip2IfStatRcvBadPackets++;
                return;
                }
            }

#ifdef RIP_MD5
        /*
         * When MD5 authentication is active, the validation routine discards
         * all RIP-1 messages and any RIP-2 messages without a matching key.
         */

        if (pErrorIfp->ifConf.rip2IfConfAuthType == M2_rip2IfConfAuthType_md5)
            {
            if (ripAuthKeyInMD5(pErrorIfp, rip, size) == ERROR)
                {
                if (routedDebug)
                    logMsg ("Unable to authenticate RIP message.\n",
                            0, 0, 0, 0, 0, 0);
                return;
                }

            /*
             * Reduce the reported size to ignore the entry which contains
             * the MD5 authentication trailer (20 bytes).
             */

            size -= sizeof(struct netinfo);
            }
        else if (pErrorIfp->ifConf.rip2IfConfAuthType ==
                 M2_rip2IfConfAuthType_simplePassword)
#else
        if (pErrorIfp->ifConf.rip2IfConfAuthType ==
            M2_rip2IfConfAuthType_simplePassword)
#endif /* RIP_MD5 */
            {
            /*
             * The ripAuthCheck() routine automatically accepts any RIP-2 
             * messages containing the correct simple password authentication 
             * key. All other messages are handled by the authentication hook,
             * or discarded if none is available.
             */

            if (ripAuthCheck (pErrorIfp->ifConf.rip2IfConfAuthKey,
                              rip) == ERROR)
                {
                if (pErrorIfp->authHook)
                    {
                    if (pErrorIfp->authHook(pErrorIfp->ifConf.
                                                       rip2IfConfAuthKey,
                                            rip) != OK)
                        {
                        pErrorIfp->ifStat.rip2IfStatRcvBadPackets++;
                        return;
                        }
                    }
                else
                    {
                    if (routedDebug)
                        logMsg("Discarding unvalidated RIP version %d message.\n",
                               rip->rip_vers, 0, 0, 0, 0, 0);
                    pErrorIfp->ifStat.rip2IfStatRcvBadPackets++;
                    return;
                    }
                }
            }

        else /* unknown auth type */
            {
            if (routedDebug)
                logMsg("Discarding unvalidated RIP version %d message (Unknown auth type).\n",
                       rip->rip_vers, 0, 0, 0, 0, 0);
            pErrorIfp->ifStat.rip2IfStatRcvBadPackets++;
            return;
            }

        /* Skip the first rip entry which was used for authentication */

        pFirst = n = rip->rip_nets + 1;
        }

    if (size > RIP_MAX_PACKET)
        {
        if (routedDebug)
            logMsg ("Packet too large from %s. (cmd %d)\n", 
                    (int)(*afswitch[from->sa_family].af_format)(from), 
                    rip->rip_cmd, 0, 0, 0, 0);
        pErrorIfp->ifStat.rip2IfStatRcvBadPackets++;
        return;
        }

    if (((size - 4) % 20) != 0)
        {
        if (routedDebug)
            logMsg("Badly formed packet from %s. (cmd %d)\n", 
                   (int)(*afswitch[from->sa_family].af_format)(from), 
                   rip->rip_cmd, 0, 0, 0, 0);
        pErrorIfp->ifStat.rip2IfStatRcvBadPackets++;
        return;
        }
    
    /* Check that the unused RIP header field contains 0 if required. */

    if (rip->rip_vers == 1)
        {
        if (rip->rip_domain[0] != 0 || rip->rip_domain[1] != 0)
            {
            pErrorIfp->ifStat.rip2IfStatRcvBadPackets++;
            return ;
            }
        }

    /*
     * Process the received message. RIP responses will exit the switch
     * statement so that triggered updates will be scheduled if necessary.
     * All other commands must execute return statements within the switch
     * to prevent that activity.
     */

    switch (rip->rip_cmd)
        {
        case RIPCMD_REQUEST:

            /*
             * Preserve port number before converting source address to
             * canonical form for testing against local interface addresses.
             */

            ((struct sockaddr_in *)&gateway)->sin_port = 
                ((struct sockaddr_in *)from)->sin_port;
            
            /* Ignore requests reflected from a local interface. */

            (*afp->af_canon) (from);
            ifp = ripIfWithAddr (from);
            if (ifp)
                return;

            /* Restore the port number for requests from valid interfaces. */

            ((struct sockaddr_in *)from)->sin_port = 
                ((struct sockaddr_in *)&gateway)->sin_port;

            /*
             * Determine number of entries in message. If there are no
             * entries, no response is given.
             */

            count = size - ((char *)n - (char *)rip);
            if (count < sizeof (struct netinfo))
                return;
            for (; count > 0; n++)
                {
                if (count < sizeof (struct netinfo))
                    break;
                count -= sizeof (struct netinfo);

                if (rip->rip_vers == 1)
                    {
                    if ( ((RIP2PKT *)n)->tag != 0)
                        {
                        pErrorIfp->ifStat.rip2IfStatRcvBadRoutes++;
                        continue;
                        }

                    if ( ((RIP2PKT *)n)->subnet != 0)
                        {
                        pErrorIfp->ifStat.rip2IfStatRcvBadRoutes++;
                        continue;
                        }

                    if ( ((RIP2PKT *)n)->gateway != 0)
                        {
                        pErrorIfp->ifStat.rip2IfStatRcvBadRoutes++;
                        continue;
                        }
                    }
                
                n->rip_dst.sa_family = ntohs (osa (n->rip_dst)->sa_family);
                n->rip_dst.sa_len = sizeof (n->rip_dst);
                n->rip_metric = ntohl (n->rip_metric);

                /* 
                 * Check for special case: the entire routing table is
                 * sent if the message contains a single entry with a 
                 * family identifier of 0 (meaning unspecified: AF_UNSPEC) 
                 * and an infinite metric.
                 */

                if (n->rip_dst.sa_family == AF_UNSPEC &&
                    (count != 0 || n != pFirst))
                    {

                    /*
                     * Ignore entry with family of 0 if the message 
                     * contains multiple route entries.
                     */

                    pErrorIfp->ifStat.rip2IfStatRcvBadRoutes++;
                    continue;
                    }

                if (n->rip_dst.sa_family == AF_UNSPEC &&
                    n->rip_metric != HOPCNT_INFINITY)
                    {
                    /* 
                     * Ignore message with single entry and family of 0 
                     * if metric is finite. 
                     */

                    pErrorIfp->ifStat.rip2IfStatRcvBadRoutes++;
                    return;
                    }

                if (n->rip_dst.sa_family == AF_UNSPEC)
                    {
                    /* 
                     * Send the entire routing table, except for entries
                     * learned from the interface which received the request.
                     * Silent RIP processes will only respond to a query which
                     * used a non-standard port. (RFC 1058, section 3.4.1).
                     */

                    if (ripState.supplier ||
                        (*afp->af_portmatch)(from) == 0)
                        {
                        supply (from, 0, pErrorIfp, 0, rip->rip_vers);

                        ripState.ripGlobal.rip2GlobalQueries++;
                        }
                    return;
                    }

                /* 
                 * Ignore entries with unsupported address families.
                 * Currently, only entries with a family of AF_INET 
                 * are accepted.
                 */

                if (n->rip_dst.sa_family != AF_INET)
                    {
                    pErrorIfp->ifStat.rip2IfStatRcvBadRoutes++;
                    continue;
                    }

                if (n->rip_dst.sa_family < AF_MAX &&
                    afswitch[n->rip_dst.sa_family].af_hash)
                    {
                    rt = rtlookup(&n->rip_dst);

                    /*
                     * If the interface for the route is down, the
                     * route is not reachable.
                     */

                    if (rt->rt_ifp && (rt->rt_ifp->int_flags & IFF_UP) == 0)
                        rt = 0;
                    }
                else
                    rt = 0;

                n->rip_metric = rt == 0 ? HOPCNT_INFINITY :
                    min(rt->rt_metric + 1, HOPCNT_INFINITY);
                osa(n->rip_dst)->sa_family =
                    htons(n->rip_dst.sa_family);
                n->rip_metric = htonl(n->rip_metric);
                changes++;
                }

            /* 
             * The RFC specification does not clearly define the desired
             * behavior for the case where all entries in a request use 
             * an unknown family. Although the individual entries must
             * be ignored, it seems as if a response containing unchanged 
             * metrics should be returned, but that behavior causes ANVL 
             * test 6.5 to fail. The following test avoids that problem.
             */

            if (changes)
                {
                /* Send a reply if at least one entry was recognized. */

                rip->rip_cmd = RIPCMD_RESPONSE;
                memmove(ripState.packet, rip, size);
                (*afp->af_output)(ripState.s, 0, from, size);
                ripState.ripGlobal.rip2GlobalQueries++;
                }
            return;
            
        case RIPCMD_TRACEON:
        case RIPCMD_TRACEOFF:
            /* Obsolete commands ignored by RIP. (RFC 1058, section 3.1) */

            if (routedDebug)
                logMsg ("Ignoring obsolete trace command from router %s.\n",
                        (int)(*afswitch[from->sa_family].af_format)(from),
                        0, 0, 0, 0, 0);
            pErrorIfp->ifStat.rip2IfStatRcvBadPackets++;
            return;
            
        case RIPCMD_RESPONSE:

            /* Verify that the message used the required port. */

            if ((*afp->af_portmatch)(from) == 0)
                {
                pErrorIfp->ifStat.rip2IfStatRcvBadPackets++;
                return;
                }

            /* 
             * Check for reflected updates from the router's own interface.
             * The test also eliminates updates from a directed broadcast
             * address which is one of the illegal source addresses forbidden
             * by the router requirements RFC.
             */

            (*afp->af_canon) (from);
            ifp = ripIfWithAddr (from);
            if (ifp)
                {
                /*
                 * When a router receives an invalid update, it uses it to 
                 * track the status of a directly connected interface, but 
                 * the message is otherwise ignored.
                 */

                if (ifp->int_flags & IFF_PASSIVE)
                    {
                    if (routedDebug)
                        logMsg ("bogus input (from passive interface, %s)\n",
                             (int)(*afswitch[from->sa_family].af_format)(from),
                                0, 0, 0, 0, 0);
                    ifp->ifStat.rip2IfStatRcvBadPackets++;
                    return;
                    }

                /* 
                 * Search for a route entry to the directly connected network
                 * and reset the expiration timer if found. If no such route
                 * is present or a non-interface route is detected, the 
                 * receiving interface was either recently added or restarted. 
                 * The addrouteforif routine will create a permanent interface 
                 * route and remove any overlapping (learned) route.
                 */

                rt = rtfind(from);
                if (rt == 0 || (((rt->rt_state & RTS_INTERFACE) == 0) &&
                    rt->rt_metric >= ifp->int_metric))
                    addrouteforif(ifp);
                else
                    {
                    rt->rt_timer = 0;

                    /* Update age for IP group MIB. */

                    if (rt->inKernel) 
                        ripRouteAgeUpdate (rt); 
                    }
                return;
                }

            /*
             * Update timer for interface route on which the message arrived.
             * If none is found and the sender is the other end of a 
             * point-to-point link that isn't registered in the routing 
             * tables, add or restore that route.
             */

            if ((rt = rtfind(from)) &&
                (rt->rt_state & (RTS_INTERFACE | RTS_REMOTE)))
                {
                rt->rt_timer = 0;
                if (rt->inKernel) 
                    ripRouteAgeUpdate (rt); /* Update age for IP group MIB. */
                }
            else if ((ifp = ripIfWithDstAddr(from, NULL)) &&
                     (rt == 0 || rt->rt_metric >= ifp->int_metric))
                addrouteforif(ifp);

            /* 
             * NOTE: The preceding rtfind() routine will not detect a 
             * match for updates from a router on the same supernet with 
             * a different class-based network number than the local
             * interfaces. (For instance, it will not reset the route 
             * timer for 192.168.254.0/23 when an update is received 
             * from 192.168.255.x/23 if the local interface has the 
             * address 192.168.254.x/23. This omission has no effect 
             * because the timer for an interface route is never 
             * incremented. The return value of 0 will not cause an
             * incorrect call to addrouteforif() because the search in
             *  ripIfWithDstAddr will always fail in this situation.
             */

            /*
             * Reset the pointer to validate the interface from which the
             * message arrived. Updates are accepted from routers directly 
             * connected via broadcast or point-to-point networks.
             */

            ifp = pErrorIfp;    /* Results of ripIfLookup on source address. */
            if ( (ifp->int_flags &
                     (IFF_BROADCAST | IFF_POINTOPOINT | IFF_REMOTE)) == 0 ||
                ifp->int_flags & IFF_PASSIVE)
                {
                /* Error: source does not use a directly-connected network. */

                if (memcmp(from, &badfrom, sizeof(badfrom)) != 0)
                    {
                    if (routedDebug)
                        logMsg ("packet from unknown router, %s\n",
                             (int)(*afswitch[from->sa_family].af_format)(from),
                                0, 0, 0, 0, 0);
                    badfrom = *from;
                    }
                pErrorIfp->ifStat.rip2IfStatRcvBadPackets++;
                return;
                }

            /*
             * At this point, the source address and port number of the 
             * response have been checked. It is from another host on a 
             * directly connected network and was sent to the correct port. 
             * Begin processing the datagram by ignoring the initial data 
             * (command, version, and two unused bytes) and leading 
             * authentication header, if any.
             */

            size = size - ((char *)n - (char *)rip);

            /* Now start processing actual route entries. */

            for (; size > 0; size -= sizeof (struct netinfo), n++)
                {
                if (size < sizeof (struct netinfo))
                    break;

                n->rip_dst.sa_family =
                    ntohs(osa(n->rip_dst)->sa_family);
                n->rip_dst.sa_len = sizeof(n->rip_dst);
                n->rip_metric = ntohl(n->rip_metric);

                /* Ignore any entry with an unknown address family. */

                if (n->rip_dst.sa_family >= AF_MAX ||
                    (afp = &afswitch[n->rip_dst.sa_family])->af_hash ==
                    (int (*)())0)
                    {
                    if (routedDebug)
                        logMsg("route in unsupported address family %d from %s"
                               "(af %d)\n", n->rip_dst.sa_family, 
                               (int)(*afswitch[from->sa_family].af_format)
                               (from), from->sa_family, 0, 0, 0);
                    pErrorIfp->ifStat.rip2IfStatRcvBadRoutes++;
                    continue;
                    }

                 /* 
                  * Ignore an entry if the unused fields are not zero.
                  * (These fields are only unused in version 1 updates).
                  */

                if (rip->rip_vers == 1)
                    {
                    if ( ((RIP2PKT *)n)->tag != 0)
                        {
                        pErrorIfp->ifStat.rip2IfStatRcvBadRoutes++;
                        continue;
                        }

                    if ( ((RIP2PKT *)n)->subnet != 0)
                        {
                        pErrorIfp->ifStat.rip2IfStatRcvBadRoutes++;
                        continue;
                        }

                    if ( ((RIP2PKT *)n)->gateway != 0)
                        {
                        pErrorIfp->ifStat.rip2IfStatRcvBadRoutes++;
                        continue;
                        }
                    }

                /* 
                 * Ignore any entry with an inappropriate address.
                 * The receiving interface is used to detect broadcast
                 * addresses if no netmask is present in the route update.
                 */

                if ( ((*afp->af_checkhost)(&n->rip_dst, ifp)) == 0)
                    {
                    if (routedDebug)
                        logMsg ("bad host in route from %s (af %d)\n",
                             (int)(*afswitch[from->sa_family].af_format)(from),
                                from->sa_family, 0, 0, 0, 0);
                    pErrorIfp->ifStat.rip2IfStatRcvBadRoutes++;
                    continue;
                    }

                if ((n->rip_metric == 0) ||
                    ((unsigned) n->rip_metric > HOPCNT_INFINITY))
                    {
                    if (memcmp(from, &badfrom2,
                               sizeof(badfrom2)) != 0)
                        {
                        if (routedDebug)
                            logMsg ("bad metric (%d) from %s\n", n->rip_metric,
                             (int)(*afswitch[from->sa_family].af_format)(from),
                                    0, 0, 0, 0);
                        badfrom2 = *from;
                        }
                    pErrorIfp->ifStat.rip2IfStatRcvBadRoutes++;
                    continue;
                    }

                rt = rtlookup (&n->rip_dst);

                /* 
                 * Do not allow a route learnt from other protocols to be
                 * replaced
                 */

                if (rt && ((rt->rt_state & RTS_OTHER) != 0))
                    continue;
                
                if (rt == 0 ||
                    (rt->rt_state & (RTS_INTERNAL|RTS_INTERFACE)) ==
                    (RTS_INTERNAL|RTS_INTERFACE))
                    {
                    /*
                     * If we're hearing a logical network route
                     * back from a peer to which we sent it,
                     * ignore it.
                     */
                    if (rt && rt->rt_state & RTS_SUBNET &&
                        (*afp->af_sendroute)(rt, from, ifp))
                        {
                        pErrorIfp->ifStat.rip2IfStatRcvBadRoutes++;
                        continue;
                        }

                    /*
                     * Adjust metric according to incoming interface.
                     */
                    if ((unsigned)n->rip_metric < HOPCNT_INFINITY)
                        {
                        /*
                         * Look for an equivalent route that
                         * includes this one before adding
                         * this route.
                         */

                        rt = rtfind(&n->rip_dst);
                        if ((rt && equal(from, &rt->rt_router)) &&
                            (rt->rt_metric <= n->rip_metric))
                            continue;

                        n->rip_metric += ifp->int_metric;

                        if (rip->rip_vers < 2)
                            {
                            rtadd (&n->rip_dst, from, n->rip_metric, 
                                   0, NULL, M2_ipRouteProto_rip, 0,
                                   ((struct sockaddr_in *)from)->
                                    sin_addr.s_addr, pErrorIfp);
                            }
                        else
                            {
                            ripSplitPacket (ifp, (struct sockaddr_in *)from, 
                                            &n->rip_dst, &gateway, &netmask,
                                            &tag);
                            if (((struct sockaddr_in *)&netmask)->
                                sin_addr.s_addr == -1)
                                {
                                pErrorIfp->ifStat.rip2IfStatRcvBadRoutes++;
                                continue;
                                }

                            /*
                             * RFC 1723, Section 3.4: check to see that the 
                             * gateway is directly reachable. If not, treat 
                             * that value as 0.0.0.0 by using the originator 
                             * as the gateway.
                             */

                            if ((((struct sockaddr_in *)&gateway)->
                                 sin_addr.s_addr & 
                                 htonl (ifp->int_subnetmask)) != 
                                (((struct sockaddr_in *)&ifp->int_addr)->
                                 sin_addr.s_addr & 
                                 htonl (ifp->int_subnetmask)))
                                {
                                ((struct sockaddr_in *)&gateway)->
                                    sin_addr.s_addr = 
                                    ((struct sockaddr_in *)from)->
                                    sin_addr.s_addr;
                                }

                            rtadd(&n->rip_dst, &gateway, n->rip_metric, 0,
                                  &netmask, M2_ipRouteProto_rip, tag,
                                  ((struct sockaddr_in *)from)->
                                  sin_addr.s_addr, pErrorIfp);
                            }
                        changes++;
                        }

                    continue;
                    }
                /*
                 * Update if from gateway and different,
                 * shorter, or equivalent but old route
                 * is getting stale.
                 */

                /*
                 * If this was a pre-existinng route then we can
                 * update it to be "infinite" and should.
                 * ANVL 7.13.
                 */

                origMetric = n->rip_metric;
                if ((unsigned) n->rip_metric < HOPCNT_INFINITY)
                    n->rip_metric += ifp->int_metric;

                if (n->rip_metric > HOPCNT_INFINITY)
                    {
                    pErrorIfp->ifStat.rip2IfStatRcvBadRoutes++;
                    continue;
                    }

                if (rip->rip_vers > 1)
                    {
                    ripSplitPacket(ifp, (struct sockaddr_in *)from, 
                                   &n->rip_dst, &gateway, &netmask,
                                   &tag);
                    if (((struct sockaddr_in *)&netmask)->
                        sin_addr.s_addr == -1)
                        {
                        pErrorIfp->ifStat.rip2IfStatRcvBadRoutes++;
                        continue;
                        }

                    /*
                     * RFC 1723, Section 3.4: check to see that the 
                     * gateway is directly reachable. If not, treat 
                     * that value as 0.0.0.0 by using the originator 
                     * as the gateway.
                     */
                    if ((((struct sockaddr_in *)&gateway)->sin_addr.s_addr & 
                         htonl (ifp->int_subnetmask)) != 
                        (((struct sockaddr_in *)&ifp->int_addr)->
                         sin_addr.s_addr & htonl (ifp->int_subnetmask)))
                        {
                        ((struct sockaddr_in *)&gateway)->sin_addr.s_addr = 
                            ((struct sockaddr_in *)from)->sin_addr.s_addr;
                        }
                    }

                /*
                 * We are making an assumption here that the address
                 * is an internet address (4 bytes long). Since we currently
                 * support only the AF_INET family its OK, but if that
                 * changes, the following check will need to be changed
                 * accordingly
                 */

                if (((struct sockaddr_in *)from)->sin_addr.s_addr ==
                    rt->rt_orgrouter)
                    {
                    /* 
                     * If the metric changed then we should change our 
                     * route. Also if this is a valid update with 
                     * metric of 15 and we have already deleted the 
                     * route from the routing database we should 
                     * revalidate the route in our table and add it 
                     * back to the system database 
                     */

                    if ((n->rip_metric != rt->rt_metric) || 
                        (n->rip_metric == HOPCNT_INFINITY)) 
                        {
                        /* 
                         * If this is a repeat advertisement of an 
                         * unreachable destination, ignore it. 
                         * Ignore it also if we have already expired 
                         * this route and are being informed that it 
                         * has expired. We have acted on it once already. 
                         */

                        if ((origMetric == HOPCNT_INFINITY) && 
                            (rt->rt_metric == HOPCNT_INFINITY) && 
                            !rt->inKernel && 
                            (rt->rt_timer >= ripState.expire)) 
                            break;

			/*
			 * If the advertised metric is (HOPCNT_INFINITY -1),
			 * a valid metric, check to see what we should
			 * do with it. If we already have the route
			 * in the system table, reset the timer,
			 * else trick rtchange() into adding the route back
			 */

			if (origMetric == (HOPCNT_INFINITY - 1))
			    {
			    if (rt->inKernel) 
                                {
                                rt->rt_timer = 0; 
                                /* Update age for IP group MIB. */ 
                                ripRouteAgeUpdate (rt);
                                break;
                                } 
                            else 
                                n->rip_metric++;
                            }

                        if (rip->rip_vers > 1)
                            {
                            rtchange(rt, &gateway, n->rip_metric,
                                     &netmask, tag, 0, NULL);
                            }
                        else
                            {
                            rtchange(rt, from, n->rip_metric, NULL, 0, 0, 
                                     NULL);
                            }
                        changes++;
                        rt->rt_timer = 0;
                        if (origMetric == HOPCNT_INFINITY)
                            rt->rt_timer = ripState.expire;
                        else if (rt->inKernel) 
                            {
                            /* Update age for IP group MIB. */ 

                            ripRouteAgeUpdate (rt);
                            }
                        }
                    else if (rt->rt_metric < HOPCNT_INFINITY)
                        {
                        rt->rt_timer = 0;
                        /* Update age for IP group MIB. */

                        if (rt->inKernel) 
                            ripRouteAgeUpdate (rt); 

                        /*
                         * Metric is same. Check if either the next hop
                         * or the tag changed, for a V2 packet. If it did,
                         * we need to record the change
                         */

                        if ((rip->rip_vers > 1) && 
                            ((((RIP2PKT *)n)->tag != rt->rt_tag) || 
                             (memcmp(&gateway, &(rt->rt_router), 
                                     sizeof(gateway)) != 0))) 
                            {
                            rtchange(rt, &gateway, n->rip_metric,
                                     &netmask, tag, 0, NULL);
                            }
                        }
                    }
                else if ((unsigned) n->rip_metric < rt->rt_metric ||
                         (rt->rt_metric == n->rip_metric &&
                          rt->rt_timer > (ripState.expire/2) &&
                          (unsigned) origMetric < HOPCNT_INFINITY))
                    {
                    if (rip->rip_vers > 1)
                        {
                        rtchange(rt, &gateway, n->rip_metric,
                                 &netmask, tag, 
                                 ((struct sockaddr_in *)from)->
                                 sin_addr.s_addr, NULL);
                        }
                    else
                        {
                        rtchange(rt, from, n->rip_metric, NULL, 0,
                                 ((struct sockaddr_in *)from)->
                                 sin_addr.s_addr, NULL);
                        }
                    changes++;
                    rt->rt_timer = 0;

                    /* Update age for IP group MIB. */

                    if (rt->inKernel) 
                        ripRouteAgeUpdate (rt); 
                    }
                }
            break;

        default:
            /* 
             * Ignore any unrecognized commands. Return from the routine
             * to prevent any possibility of scheduling a triggered update.
             */

            pErrorIfp->ifStat.rip2IfStatRcvBadPackets++;
            return;  
        }
    
    /*
     * This section of code schedules triggered updates whenever entries in
     * the routing table change. No updates are sent for silent RIP
     * configurations and are also suppressed if a regular update will
     * occur within the next MAX_WAITTIME seconds.
     */

    if (changes && ripState.supplier &&
        ripState.now.tv_sec - ripState.lastfullupdate.tv_sec < 
            ripState.supplyInterval - MAX_WAITTIME)
        {
        /*
         * No regular update is imminent. Check the elapsed time since
         * the previous (regular or triggered) update and the time limit
         * of the "nextbcast" quiet period which restricts the update
         * frequency. 
         */

        if (ripState.now.tv_sec - ripState.lastbcast.tv_sec >= MIN_WAITTIME
            && (ripState.nextbcast.tv_sec < ripState.now.tv_sec))
            {
            /*
             * All conditions have been met. Send a triggered update over the 
             * interfaces which did not receive the RIP response. The message 
             * sent over each interface only includes routes which changed 
             * since the last message sent.
             */

            if (routedDebug)
                logMsg ("send dynamic update\n", 0, 0, 0, 0, 0, 0);
            ifp->ifStat.rip2IfStatSentUpdates++;

            toall (supply, RTS_CHANGED, ifp);

            ripState.lastbcast = ripState.now;
            ripState.needupdate = 0;
            ripState.nextbcast.tv_sec = 0;
            }
        else
            {
            /*
             * The triggered update can't be sent because of frequency 
             * limitations imposed to prevent excessive network traffic.
             * Set the delayed update indicator so that the update will
             * be sent when possible.
             */

            ripState.needupdate++;
            }
        
        if (ripState.nextbcast.tv_sec == 0)
            {
            /*
             * Select the earliest possible time for the next triggered 
             * update. A random value is used to avoid periodic network
             * congestion from synchronized routers.
             */

            u_long delay = RANDOMDELAY();
            ripState.nextbcast.tv_sec = delay / 1000000;
            ripState.nextbcast.tv_usec = delay % 100000;
            timevaladd (&ripState.nextbcast, &ripState.now);

            /*
             * If the earliest allowable update occurs within MIN_WAITTIME 
             * seconds before the next regular update, force the delay past 
             * that point to avoid a redundant triggered update.
             */

            if (ripState.nextbcast.tv_sec >
                ripState.lastfullupdate.tv_sec +
                ripState.supplyInterval - MIN_WAITTIME)
                {
                ripState.nextbcast.tv_sec =
                    ripState.lastfullupdate.tv_sec +
                    ripState.supplyInterval + 1;
                }
            }
        }
    }

/* 
 * This pseudo-random number generator restricts the frequency of triggered
 * updates. It was plagiarized shamelessly from FreeBSD 2.1.7 where it was 
 * used for the same purpose by routed.
 */

LOCAL u_long ripRandTime (void)
    {
    register long x, hi, lo, t;

    /*
     * Compute x[n + 1] = (7^5 * x[n]) mod (2^31 - 1).
    * From "Random number generators: good ones are hard to find",
     * Park and Miller, Communications of the ACM, vol. 31, no. 10,
     * October 1988, p. 1195.
     */
    x = ripRandTimeSeed;
    hi = x / 127773;
    lo = x % 127773;
    t = 16807 * lo - 2836 * hi;
    if (t <= 0)
        t += 0x7fffffff;
    ripRandTimeSeed = t;
    return (t);
    }

/****************************************************************************
*
* ripRouteAgeUpdate - update the age for the RIP route
*
* This routine updates the age of the RIP route that is kept in the
* system Routing database.
* The parameter <pRtEntry> describes the RIP route that is kept in RIP's
* private database.
*
* This routine constructs a ROUTE DESCRIPTOR structure from the supplied
* parameter <pRtEntry> that describes the route whose age is to be updated
* and calls the routing extensions function routeAgeSet() to set the new
* age. tickGet() is used to get the current tick count and is used as the
* new age.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*/

LOCAL void ripRouteAgeUpdate 
    (
    struct rt_entry *	pRtEntry	/* Route entry describing the */
    					/* RIP route to update */
    )
    {
    struct sockaddr_in *	pDsin;
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
        logMsg ("ripRouteAgeUpdate: setting new age = %d for\n", 
                tickGet (), 0, 0, 0, 0, 0);
        ripSockaddrPrint ((struct sockaddr *)pDsin);
        ripSockaddrPrint ((struct sockaddr *)pNsin);
        ripSockaddrPrint ((struct sockaddr *)pGsin);
        }

    /* Now set the route age */

    if (routeAgeSet ((struct sockaddr *)pDsin, (struct sockaddr *)pNsin, 
                     M2_ipRouteProto_rip, tickGet ()) == ERROR) 

        {
        if (routedDebug) 
            logMsg ("Couldn't set age for rtEntry = %x.\n", (int)pRtEntry, 0, 
                    0, 0, 0, 0);
        }
    }
