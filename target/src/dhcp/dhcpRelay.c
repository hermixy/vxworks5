/* dhcpRelay.c - DHCP server and relay agent shared code library */

/* Copyright 1984 - 2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01e,07may02,wap  Put debug messages under DHCPR_DEBUG (SPR #76495)
01d,27mar02,wap  call ip_mloopback() rather than calling looutput() directly
                 (SPR #72246)
01c,22mar02,wap  avoid division by zero in dhcpServerRelay() (SPR #74456)
01b,06dec01,wap  Add NPT support
01a,12oct01,rae  merge from truestack ver 01n, base 01f (SPR #69547)
01i,17nov00,spm  added support for BSD Ethernet devices
01h,24oct00,spm  fixed merge from tor3_x branch and updated mod history
01g,23oct00,niq  merged from version 01h of tor3_x branch (base version 01f)
01f,04dec97,spm  added code review modifications
01e,06oct97,spm  split interface name into device name and unit number; fixed
                 errors in debugging output
01d,02jun97,spm  changed DHCP option tags to prevent name conflicts (SPR #8667)
                 and updated man pages
01c,28apr97,spm  limited maximum number of hops to 16 for RFC compliance
01b,18apr97,spm  added conditional include DHCPR_DEBUG for displayed output
01a,07apr97,spm  created by modifying WIDE project DHCP implementation
*/

/*
DESCRIPTION
This library contains the code used by the DHCP relay agent to transfer 
packets between DHCP or BOOTP clients and DHCP servers. The DHCP server
will also use this code if configured to act as a relay agent. The
library also includes the shared transmit routine which sends unicast
packets to a DHCP client without performing address resolution, since
the destination IP address is not yet resolvable.

INCLUDE_FILES: dhcprLib.h
*/

/*
 * WIDE Project DHCP Implementation
 * Copyright (c) 1995 Akihiro Tominaga
 * Copyright (c) 1995 WIDE Project
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided only with the following
 * conditions are satisfied:
 *
 * 1. Both the copyright notice and this permission notice appear in
 *    all copies of the software, derivative works or modified versions,
 *    and any portions thereof, and that both notices appear in
 *    supporting documentation.
 * 2. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by WIDE Project and
 *      its contributors.
 * 3. Neither the name of WIDE Project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE DEVELOPER ``AS IS'' AND WIDE
 * PROJECT DISCLAIMS ANY LIABILITY OF ANY KIND FOR ANY DAMAGES
 * WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE. ALSO, THERE
 * IS NO WARRANTY IMPLIED OR OTHERWISE, NOR IS SUPPORT PROVIDED.
 *
 * Feedback of the results generated from any improvements or
 * extensions made to this software would be much appreciated.
 * Any such feedback should be sent to:
 * 
 *  Akihiro Tominaga
 *  WIDE Project
 *  Keio University, Endo 5322, Kanagawa, Japan
 *  (E-mail: dhcp-dist@wide.ad.jp)
 *
 * WIDE project has the rights to redistribute these changes.
 */

/* includes */

#include "vxWorks.h"
#include "vxLib.h"             /* checksum() declaration. */

#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include "logLib.h"
#include "sockLib.h"
#include "ioLib.h"
#include "ipProto.h" 		/* IP_DRV_CTRL definition. */
#include "muxLib.h"
#include "tickLib.h" 		/* tickGet() declaration. */

#ifdef DHCPR_DEBUG
#include "inetLib.h"
#endif

#include "dhcprLib.h"
#include "dhcp/dhcp.h"
#include "dhcp/common.h"
#include "dhcp/common_subr.h"

/* globals */

#ifndef VIRTUAL_STACK
DHCP_TARGET_DESC *pDhcpRelayTargetTbl;
struct server *pDhcpTargetList = NULL;
int dhcpNumTargets = 0;
struct msg dhcpMsgIn;
struct msg dhcprMsgOut;
u_short dhcps_port;
u_short dhcpc_port;
#else
#include "netinet/vsLib.h"
#include "netinet/vsDhcps.h"
#endif /* VIRTUAL_STACK */

/* forward declarations */

IMPORT void ip_mloopback(struct ifnet *, struct mbuf *, struct sockaddr_in *,
                    struct rtentry *rt);

void dhcpServerRelay (struct if_info *);
void dhcpClientRelay (struct if_info *, int, char *);
LOCAL STATUS forwarding();

/*******************************************************************************
*
* dhcpsSend - transmit an outgoing DHCP message
*
* This routine uses the MUX interface to send an IP packet containing a
* DHCP message independently of the link level type. It is derived from
* the ipOutput() routine in the ipProto.c module, but uses the available
* hardware address of the original sender as the destination link-level
* address instead of performing address resolution.
*
* Bypassing the normal address resolution process is required since the
* destination IP address has not yet been assigned so (for instance)
* ARP requests would fail, preventing delivery of the reply.
*
* RETURNS: OK, or ERROR if send fails.
*
* ERRNO: N/A
*
* NOMANUAL
*/

STATUS dhcpsSend
    (
    struct ifnet * 	pIf,      /* interface for sending message */
    char * 		pHwAddr,  /* destination link-level address */
    int 		hlen,     /* size of link-level address, in bytes */
    struct sockaddr_in * pDest,   /* destination IP address for message */
    char * 		pData,    /* IP packet containing DHCP message */
    int 		size,     /* amount of data to send */
    BOOL 		broadcastFlag   /* broadcast packet? */
    )
    {
    IP_DRV_CTRL * pDrvCtrl;     /* Protocol-specific information. */
    void *        pIpCookie;    /* Result of muxBind() routine for device */
    struct mbuf * pMbuf;
    u_short etype;
    int s;

    if ((pIf->if_flags & (IFF_UP|IFF_RUNNING)) != (IFF_UP|IFF_RUNNING))
        return (ERROR);    /* ENETDOWN */

    pDrvCtrl = (IP_DRV_CTRL *)pIf->pCookie;
    if (pDrvCtrl == NULL)    /* Undefined error in original transmit code. */
        return (ERROR);

    pIpCookie = pDrvCtrl->pIpCookie;
    if (pIpCookie == NULL)
        return (ERROR);    /* EINVAL */

    pMbuf = bcopy_to_mbufs (pData, size, 0, pIf, NONE);
    if (pMbuf == NULL)
        return (ERROR);    /* ENOBUFS */

    s = splnet ();

    pIf->if_lastchange = tickGet ();

    /* Setup existing mBlk structures and build link-level header. */

    etype = htons (ETHERTYPE_IP);

    pDrvCtrl->pSrc->m_data = (char *)&((struct arpcom *)pIf)->ac_enaddr;
    bcopy (pHwAddr, (char *)pDrvCtrl->pDstAddr, hlen);

    pDrvCtrl->pDst->mBlkHdr.reserved = etype;
    pDrvCtrl->pSrc->mBlkHdr.reserved = etype;

    if (broadcastFlag)
        {
        /*
         * Use the registered resolve routine to get the link-level broadcast.
         */

        pMbuf->mBlkHdr.mFlags |= M_BCAST;

        /* Loopback a copy of the data if using a simplex interface. */

        if (pIf->if_flags & IFF_SIMPLEX)
            {
            ip_mloopback (pIf, pMbuf, (struct sockaddr_in *)pDest, NULL);
            }
        }

    if (pDrvCtrl->nptFlag)
        {
        if (pIf->if_addrlen)
            {
            M_PREPEND(pMbuf, pIf->if_addrlen, M_DONTWAIT);
            if (pMbuf == NULL)
                {
                netMblkClChainFree (pMbuf);
                return(ERROR);
                }
            ((M_BLK_ID)pMbuf)->mBlkPktHdr.rcvif = 0;

            /* Store the destination address. */

            bcopy (pDrvCtrl->pDstAddr, pMbuf->m_data, pIf->if_addrlen);
            }

        /* Save the network protocol type. */

        ((M_BLK_ID)pMbuf)->mBlkHdr.reserved = etype;
        }
    else
        {
        pMbuf = muxLinkHeaderCreate (pDrvCtrl->pIpCookie, pMbuf,
                                     pDrvCtrl->pSrc,
                                     pDrvCtrl->pDst, broadcastFlag);
        if (pMbuf == NULL)
            {
            netMblkClChainFree (pMbuf);
            return (ERROR);    /* ENOBUFS */
            }
        }

    /*
     * Queue message on interface, and start output if interface
     * not yet active.
     */

    if (IF_QFULL(&pIf->if_snd))
        {
        IF_DROP(&pIf->if_snd);
        splx(s);

        netMblkClChainFree (pMbuf);
        return (ERROR);    /* ENOBUFS */
        }

    IF_ENQUEUE(&pIf->if_snd, pMbuf);

    pIf->if_start (pIf);

    pIf->if_obytes += pMbuf->mBlkHdr.mLen;
    if (pMbuf->m_flags & M_MCAST || pMbuf->m_flags & M_BCAST)
        pIf->if_omcasts++;

    splx(s);

    return (OK);
    }

/*******************************************************************************
*
* dhcpServerRelay - send incoming DHCP/BOOTP message to client port
*
* This routine relays a DHCP or BOOTP message to the client port of 
* every DHCP server or relay agent whose IP address is contained in the 
* circular linked list of targets. That list is constructed during startup of 
* the relay agent (or server, if configured to relay messages). The routine
* accesses global pointers already set to indicate the outgoing message. 
* All messages are discarded after the hops field exceeds 16 to comply with
* the relay agent behavior specified in RFC 1542. The relay agent normally
* discards such messages before this routine is called. They will only
* be received by this routine if the user ignores the instructions in the
* manual and sets the value of DHCP_MAX_HOPS higher than 16.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*/

void dhcpServerRelay
    (
    struct if_info *ifp
    )
    {
    unsigned int hash;
    int i;
    struct server *srvp;

    dhcpMsgIn.dhcp->hops++;
    if (dhcpMsgIn.dhcp->hops >= 17)    /* RFC limits to 16 hops - ignore. */
        return;

    /*
     * If the target table is empty, bail, otherwise
     * trying to do the hash computation below will yield a
     * division by zero.
     */

    if (dhcpNumTargets == 0)
        return;

    if (dhcpMsgIn.dhcp->giaddr.s_addr == 0)
        dhcpMsgIn.dhcp->giaddr.s_addr = ifp->ipaddr.s_addr;

    /* Quick and dirty load balancing - pick starting point in circular list. */

    hash = (unsigned)checksum( (u_short *)dhcpMsgIn.dhcp->chaddr, 
                               (MAX_HLEN / 2));

    hash %= dhcpNumTargets;

    srvp = pDhcpTargetList;
    for (i = 0; i < hash; i++)
        srvp = srvp->next;

    forwarding (srvp);

    return;
    }

/*******************************************************************************
*
* forwarding - send DHCP/BOOTP message to target address
*
* This routine relays a DHCP or BOOTP message to the specified target address.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*/

static STATUS forwarding
    (
    struct server *srvp
    )
    {
    int i, n;
    int sockfd = -1;
    int msgsize = DHCPLEN (dhcpMsgIn.udp);
    struct sockaddr_in my_addr, serv_addr;
    int result;
#ifdef DHCPR_DEBUG
    char output [INET_ADDR_LEN];
#endif

    if ( (sockfd = socket (AF_INET, SOCK_DGRAM, 0)) < 0) 
        {
#ifdef DHCPR_DEBUG
        logMsg ("socket() error in forwarding()\n", 0, 0, 0, 0, 0, 0);
#endif
        return (ERROR);
        }
    bzero ((char *)&my_addr, sizeof (my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = htonl (INADDR_ANY);
    my_addr.sin_port = dhcpc_port;  /* Servers expect client port. */
    if (bind (sockfd, (struct sockaddr *) &my_addr, sizeof (my_addr)) < 0) 
        {
#ifdef DHCPR_DEBUG
        logMsg("bind() error in forwarding()\n", 0, 0, 0, 0, 0, 0);
#endif
        close (sockfd);
        return (ERROR);
        }

    dhcprMsgOut.dhcp = dhcpMsgIn.dhcp;
    result = setsockopt (sockfd, SOL_SOCKET, SO_SNDBUF, 
                         (char *)&msgsize, sizeof (msgsize));
    if (result < 0) 
        {
#ifdef DHCPR_DEBUG
        logMsg ("Warning: can't set send buffer in forwarding().\n",
                 0, 0, 0, 0, 0, 0);
#endif
        close (sockfd);
        return (ERROR);
        }
  
    n = sizeof(serv_addr);
    for (i = 0; i < dhcpNumTargets; i++) 
        {
        bzero ((char *)&serv_addr, n);
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = srvp->ip.s_addr;
        serv_addr.sin_port = dhcps_port;
#ifdef DHCPR_DEBUG
        inet_ntoa_b (srvp->ip, output);
        printf ("Server relay: sending %d bytes to %s.\n", 
                msgsize, output);
#endif
        sendto (sockfd, (char *)dhcprMsgOut.dhcp, msgsize, 0, 
                  (struct sockaddr *)&serv_addr, n);
        srvp = srvp->next;
        }
    close (sockfd);
    return (OK);
    }

/*******************************************************************************
*
* dhcpClientRelay - send DHCP/BOOTP replies to client
*
* This routine relays a DHCP or BOOTP reply to the client which generated
* the initial request. The routine accesses global pointers already set to 
* indicate the reply. All messages are discarded after the hops field exceeds 
* 16 to comply with the relay agent behavior specified in RFC 1542. The relay 
* agent normally discards such messages before this routine is called. They 
* will only be received by this routine if the user ignores the instructions 
* in the manual and sets the value of DHCP_MAX_HOPS higher than 16.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*/

void dhcpClientRelay
    (
    struct if_info *list,
    int dhcplen,
    char * pSendBuf    /* Transmit buffer for fragmented message. */
    )
    {
    int total;
    char devName [10];
    struct if_info *ifp;
    struct sockaddr_in dest;
    BOOL bcastFlag;
    int msgtype = 0;
    char *option = NULL;
    struct ifnet *pIf;
    STATUS result;

#ifdef DHCPR_DEBUG
    char srcAddr [INET_ADDR_LEN];
    char dstAddr [INET_ADDR_LEN];
#endif

    if (dhcpMsgIn.dhcp->hops >= 17)    /* RFC limits to 16 hops - ignore. */
        return;

    ifp = list;
    while (ifp != NULL) 
        {
        if (ifp->ipaddr.s_addr == dhcpMsgIn.dhcp->giaddr.s_addr)
            break;
        ifp = ifp->next;
        }

    if (ifp == NULL) 
        { 
#ifdef DHCPR_DEBUG
        inet_ntoa_b (dhcpMsgIn.ip->ip_src, srcAddr);

        logMsg ("Warning: DHCP message from server(%s) has no interface.\n",
	         srcAddr, 0, 0, 0, 0, 0);
#endif
        return;
        }

    sprintf (devName, "%s%d", ifp->name, ifp->unit);

    pIf = ifunit (devName);
    if (pIf == NULL)
        {
#ifdef DHCPR_DEBUG
        logMsg ("Warning: couldn't access network interface %s.\n", 
                 (int)ifp->name, 0, 0, 0, 0, 0);
#endif
        return;
        }


    if ( (option = pickup_opt (dhcpMsgIn.dhcp, dhcplen, _DHCP_MSGTYPE_TAG)) 
          != NULL) 
        msgtype = *OPTBODY(option);

     /* There is already space for the IP and UDP headers. */

    dhcprMsgOut.dhcp = dhcpMsgIn.dhcp;
    dhcprMsgOut.udp = dhcpMsgIn.udp;
    dhcprMsgOut.ip = dhcpMsgIn.ip;

#ifdef DHCPR_DEBUG
    logMsg ("Client relay: Headers found.\n", 0, 0, 0, 0, 0, 0);
#endif

#ifdef DHCPR_DEBUG
    logMsg ("Setting parameters.\n", 0, 0, 0, 0, 0, 0);
#endif

    bzero ( (char *)&dest, sizeof (struct sockaddr_in));

    dest.sin_len = sizeof (struct sockaddr_in);
    dest.sin_family = AF_INET;

    if (ISBRDCST (dhcpMsgIn.dhcp->flags)) 
        {
#ifdef DHCPR_DEBUG
        logMsg ("Broadcasting reply.\n", 0, 0, 0, 0, 0, 0);
#endif
        dest.sin_addr.s_addr = dhcprMsgOut.ip->ip_dst.s_addr = 0xffffffff;
        dhcprMsgOut.ip->ip_src.s_addr = ifp->ipaddr.s_addr;
        bcastFlag = TRUE;
        } 
    else 
        {
        if (msgtype == DHCPNAK)
            {
            dest.sin_addr.s_addr = dhcprMsgOut.ip->ip_dst.s_addr = 0xffffffff;
            bcastFlag = TRUE;
            }
        else
            {
            dhcprMsgOut.ip->ip_dst.s_addr = dhcpMsgIn.dhcp->yiaddr.s_addr;
            dest.sin_addr.s_addr = dhcpMsgIn.dhcp->yiaddr.s_addr;
            bcastFlag = FALSE;
            }
        dhcprMsgOut.ip->ip_src.s_addr = ifp->ipaddr.s_addr;
#ifdef DHCPR_DEBUG
        inet_ntoa_b (dhcprMsgOut.ip->ip_src, srcAddr);
        inet_ntoa_b (dhcprMsgOut.ip->ip_dst, dstAddr);
        logMsg ("Sending reply from %s to %s.\n", srcAddr, dstAddr,
                 0, 0, 0, 0);
#endif
        }
    dhcprMsgOut.udp->uh_sport = dhcps_port;
    dhcprMsgOut.udp->uh_dport = dhcpc_port;
    dhcprMsgOut.udp->uh_ulen = htons (dhcplen + UDPHL);
    dhcprMsgOut.udp->uh_sum = get_udpsum (dhcprMsgOut.ip, dhcprMsgOut.udp);
    dhcprMsgOut.ip->ip_v = IPVERSION;
    dhcprMsgOut.ip->ip_hl = IPHL >> 2;
    dhcprMsgOut.ip->ip_tos = 0;
    dhcprMsgOut.ip->ip_len = htons (dhcplen + UDPHL + IPHL);
    dhcprMsgOut.ip->ip_id = dhcprMsgOut.udp->uh_sum;
    dhcprMsgOut.ip->ip_ttl = 0x20;                       /* XXX */
    dhcprMsgOut.ip->ip_p = IPPROTO_UDP;

    total = dhcplen + IPHL + UDPHL;
    if (total <= pIf->if_mtu) 
        {
        /*
         * The message fits within the MTU size of the outgoing interface.
         * Send it "in place", altering the UDP and IP headers of the
         * original (incoming) message's buffer.
         */

        dhcprMsgOut.ip->ip_off = 0;
        dhcprMsgOut.ip->ip_sum = get_ipsum (dhcprMsgOut.ip);
#ifdef DHCPR_DEBUG
        logMsg ("Writing %d bytes.\n", total, 0, 0, 0, 0, 0);
#endif
        result = dhcpsSend (pIf,
                            dhcprMsgOut.dhcp->chaddr, dhcprMsgOut.dhcp->hlen,
                            &dest, (char *)dhcprMsgOut.ip, total, bcastFlag);
#ifdef DHCPR_DEBUG
       if (result != OK)
            logMsg ("Error %d relaying message to client.\n", errno, 
                    0, 0, 0, 0, 0);
#endif
        } 
    else 
        {
        /*
         * The message exceeds the MTU size of the outgoing interface.
         * Split it into fragments and send them using the provided
         * transmit buffer.
         */

        char *n, *end, *begin;

        begin = (char *) dhcprMsgOut.udp;
        end = &begin [total];

        total = pIf->if_mtu - IPHL;
        for (n = begin; n < end; n += total) 
            {
            if ((end - n) >= total) 
                {
                dhcprMsgOut.ip->ip_len = htons (pIf->if_mtu);
                dhcprMsgOut.ip->ip_off = htons(IP_MF | ((((n - begin) / 8)) & 0x1fff));
                dhcprMsgOut.ip->ip_sum = get_ipsum (dhcprMsgOut.ip);
                bcopy ( (char *)dhcprMsgOut.ip, pSendBuf, IPHL);
                bcopy ( (char *)n, &pSendBuf [IPHL], total); 
                } 
            else 
                {
                /* Last fragment of message. */

                dhcprMsgOut.ip->ip_len = htons (IPHL + end - n);
                dhcprMsgOut.ip->ip_off = htons(((n - begin) / 8) & 0x1fff);
                dhcprMsgOut.ip->ip_sum = get_ipsum(dhcprMsgOut.ip);
                bcopy ( (char *)dhcprMsgOut.ip, pSendBuf, IPHL);
                bcopy ( (char *)n, &pSendBuf [IPHL], end - n); 
                }
            dhcpsSend (pIf,
                       dhcprMsgOut.dhcp->chaddr, dhcprMsgOut.dhcp->hlen,
                       &dest, pSendBuf, ntohs (dhcprMsgOut.ip->ip_len),
                       bcastFlag);
            }

        /* Send complete: clean the transmit buffer before later use. */

        bzero (pSendBuf, total + IPHL);
        }
    return;
    }

/*******************************************************************************
*
* read_server_db - read IP addresses to receive relayed packets
*
* This routine extracts the IP address entries hard-coded into usrNetwork.c as 
* dhcpTargetTbl[] and stores them in a circular linked list. Each of these 
* address entries must be on a different subnet from the calling server
* or relay agent. Each entry in the list will receive a copy of every DHCP or
* BOOTP message arriving at the client port.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*/

void read_server_db (int dbsize)
    {
    char targetIP [sizeof ("255.255.255.255")];
    struct server *srvp = NULL;
    struct server *lastp = NULL;
    int loop;

    /* Read IP addresses of target servers or relay agents. */

    for (loop = 0; (loop < dbsize) &&
	 (pDhcpRelayTargetTbl [loop].pAddress); loop++)
        {
        sprintf (targetIP, "%s", pDhcpRelayTargetTbl [loop].pAddress);

        srvp = (struct server *)calloc (1, sizeof (struct server));
        if (srvp == NULL) 
            {
#ifdef DHCPR_DEBUG
            logMsg ("Memory allocation error reading relay target database.\n", 
                     0, 0, 0, 0, 0, 0);
#endif
            break;
            }
   
        srvp->ip.s_addr = inet_addr (targetIP);
        if (srvp->ip.s_addr == -1) 
            {
#ifdef DHCPR_DEBUG
            logMsg ("Conversion error reading relay target database.\n",
                     0, 0, 0, 0, 0, 0);
#endif
            free (srvp);
            continue;
            }
        dhcpNumTargets++;
        srvp->next = pDhcpTargetList;
        if (pDhcpTargetList == NULL)
            lastp = srvp;
        pDhcpTargetList = srvp;
        }

    if (lastp == NULL)                /* No target DHCP servers. */
        return;

    /* Create circular list of DHCP server addresses. */

    lastp->next = pDhcpTargetList;

#ifdef DHCPR_DEBUG
    logMsg ("read %d entries from relay target database", dhcpNumTargets, 
             0, 0, 0, 0, 0);
#endif

    return;
    }

