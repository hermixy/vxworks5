/* dhcpr.c - DHCP relay agent library */

/* Copyright 1984 - 2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01h,23apr02,wap  Use BPF_WORDALIGN() when retreiving multiple messages from a
                 BPF buffer (SPR #74215)
01g,03dec01,vvv  fixed packet forwarding (SPR #71645)
01f,16feb01,rae  fixed modhist line
01e,14jun00,spm  upgraded to RFC 2131 and removed direct link-level access
01d,06oct97,spm  removed reference to deleted endDriver global; replaced with
                 support for dynamic driver type detection
01c,06may97,spm  changed memory access to align IP header on four byte boundary
01b,28apr97,spm  allowed user to change DHCP_MAX_HOPS setting
01a,07apr97,spm  created by modifying WIDE project DHCP implementation
*/

/*
DESCRIPTION
This library implements the relay agent of the Dynamic Host Configuration 
Protocol. It will transfer all DHCP or BOOTP messages arriving on the
client port of the specified network interfaces across subnet boundaries to
the IP addresses of other DHCP relay agents or DHCP servers.

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

#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include "net/bpf.h"

#include "logLib.h"
#include "sockLib.h"
#include "ioLib.h"
#include "muxLib.h"

#include "dhcprLib.h"
#include "dhcp/dhcp.h"
#include "dhcp/common.h"
#include "dhcp/common_subr.h"

/* globals */

IMPORT BOOL dhcprInputHook (struct ifnet*, char*, int);

struct msg dhcprMsgIn;
IMPORT struct if_info *dhcprIntfaceList;
IMPORT int dhcprBufSize;    /* size of receive buffer for each interface. */
IMPORT char * pDhcprSendBuf;

/* forward declarations */

void dhcpServerRelay (struct if_info *);
IMPORT void dhcpClientRelay (struct if_info *, int, char *);

/*******************************************************************************
*
* dhcprStart - monitor specified network interfaces
*
* This routine monitors the interfaces specified by the user for incoming
* DHCP or BOOTP messages. It is the entry point for the relay agent task and 
* should only be called internally.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*/

void dhcprStart (void)
    {
    struct if_info *ifp = NULL;          /* pointer to interface */
    struct if_info *pIf = NULL;          
    int n = 0;
    struct bpf_hdr * 	pMsgHdr;
    char * 		pMsgData;
    int 		msglen;
    int 		curlen;
    int 		totlen;         /* Amount of data in BPF buffer. */

  /****************************
   * Main loop                *
   * Process incoming message *
   ****************************/
  
    FOREVER
        {
        /* select and read from interfaces */

        ifp = read_interfaces (dhcprIntfaceList, &n, dhcprBufSize);
        if (ifp == NULL)
            continue;

        /* Divide each DHCP message in buffer into protocol sections. */

        msglen = curlen = 0;
        totlen = n;
        pMsgHdr = (struct bpf_hdr *)ifp->buf;
        pMsgData = ifp->buf;
        while (curlen < totlen)
            {
            msglen = BPF_WORDALIGN(pMsgHdr->bh_hdrlen + pMsgHdr->bh_caplen);
            curlen += msglen;

            /* Set the IP pointer to skip the BPF and link level headers. */

            dhcprMsgIn.ip = (struct ip *) (pMsgData + pMsgHdr->bh_hdrlen +
                                           pMsgHdr->bh_linklen);

            /* Check if message is addressed to us */

            pIf = dhcprIntfaceList;
	    while (pIf != NULL)
		{
		if (pIf->ipaddr.s_addr == dhcprMsgIn.ip->ip_dst.s_addr)
		    break;
		pIf = pIf->next;
		}

            if ((dhcprMsgIn.ip->ip_dst.s_addr == 0xffffffff || (pIf != NULL)) &&
                  check_ipsum (dhcprMsgIn.ip))
                dhcprMsgIn.udp = (struct udphdr *) ((char *) dhcprMsgIn.ip + 
						    (dhcprMsgIn.ip->ip_hl << 2));
            else
                {
                pMsgData = pMsgData + msglen;
                pMsgHdr = (struct bpf_hdr *)pMsgData;
                continue;
                }

            if (check_udpsum (dhcprMsgIn.ip, dhcprMsgIn.udp))
                dhcprMsgIn.dhcp = (struct dhcp *) ((char *) dhcprMsgIn.udp +
							    UDPHL);
            else
                {
                pMsgData = pMsgData + msglen;
                pMsgHdr = (struct bpf_hdr *)pMsgData;
                continue;
                }

            dhcpMsgIn.ip = dhcprMsgIn.ip;
            dhcpMsgIn.udp = dhcprMsgIn.udp;
            dhcpMsgIn.dhcp = dhcprMsgIn.dhcp;
            if (dhcpMsgIn.dhcp->op == BOOTREQUEST)
                dhcpServerRelay (ifp);     /* process the packet */
            else if (dhcpMsgIn.dhcp->op == BOOTREPLY)
                dhcpClientRelay (dhcprIntfaceList, DHCPLEN (dhcprMsgIn.udp),
                                 pDhcprSendBuf);

            pMsgData = pMsgData + msglen;
            pMsgHdr = (struct bpf_hdr *)pMsgData;
            }
        }
    }
