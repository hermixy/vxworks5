/* interface.c - DHCP server and relay agent network interface library */

/* Copyright 1984 - 2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01f,10oct01,rae  fixed modhist line
01e,06oct97,spm  split interface name into device name and unit number
01d,26aug97,spm  moved routines from common_subr.c not used by DHCP client
01c,06may97,spm  changed memory access to align IP header on four byte
                 boundary; removed unused timeout variable
01b,18apr97,spm  added conditional include DHCPS_DEBUG for displayed output
01a,07apr97,spm  created by modifying WIDE project DHCP implementation
*/

/*
DESCRIPTION
This library contains the code used by the DHCP server and relay agents to
monitor one or more network interfaces for incoming DHCP messages. The monitor
routine is triggered on a semaphore given by an Ethernet input hook. The
corresponding message, read from a pre-allocated ring buffer, identifies the
originating network interface and the amount of received data. The data itself
is then copied from another ring buffer into a temporary interface-specific
buffer.

INCLUDE_FILES: None
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

#include "vxWorks.h"
#include "ioLib.h" 	/* ioctl() declaration */
#include "vxLib.h" 	/* checksum() declaration */
#include "sockLib.h"
#include "logLib.h"
#include "rngLib.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>

#include "dhcp/dhcp.h"
#include "dhcp/common.h"
#include "dhcp/common_subr.h"

/*******************************************************************************
*
* open_if - initialize per-interface data structure
*
* This routine sets the IP address and subnet mask entries for each
* interface monitored by the DHCP server or relay agent.
*
* RETURNS: 0 if successful, or -1 on error.
*
* ERRNO: N/A
*
* NOMANUAL
*/

int open_if
    (
    struct if_info *ifinfo 	/* pointer to interface descriptor */
    )
    {
    int n;
    struct ifreq ifreq;

    bzero ( (char *)&ifreq, sizeof(ifreq));
    sprintf (ifreq.ifr_name, "%s%d", ifinfo->name, ifinfo->unit);

    /*
     * Initialize the interface information (subnet and IP address).
     */
    n = socket (AF_INET, SOCK_DGRAM, 0);
    if (n < 0) 
        return (-1);

    if (ioctl (n, SIOCGIFNETMASK, (int)&ifreq) < 0) 
        {
#ifdef DHCPS_DEBUG
        logMsg("Error: Can't retrieve netmask in open_if().\n", 
                0, 0, 0, 0, 0, 0);
#endif
        close (n);
        return (-1);
        }
    ifinfo->subnetmask.s_addr =
                    ( (struct sockaddr_in *)&ifreq.ifr_addr)->sin_addr.s_addr;

    if (ioctl(n, SIOCGIFADDR, (int)&ifreq) < 0) 
        {
#ifdef DHCPS_DEBUG
        logMsg("Error: can't retrieve address in open_if().\n", 
                0, 0, 0, 0, 0, 0);
#endif
        close (n);
        return (-1);
        }

    ifinfo->ipaddr.s_addr = 
                    ( (struct sockaddr_in *)&ifreq.ifr_addr)->sin_addr.s_addr;
    close(n);

    /*
     * Store size of preallocated buffer. Offset used to align IP header on 
     * 4-byte boundary for Sun BSP's. 
     */

    ifinfo->buf_size = DHCP_MSG_SIZE + DHCPS_OFF;

    return (0);
    }

/*******************************************************************************
*
* read_interfaces - extract arriving messages from shared buffers
*
* This function waits for the arrival of a DHCP message (extracted by
* a BPF device) and copies the message into the private buffer
* associated with the receiving interface.
*
* RETURNS: Pointer to data structure of receiving interface, or NULL on error.
*
* ERRNO: N/A
*
* NOMANUAL
*/

struct if_info * read_interfaces
    (
    struct if_info *iflist, 	/* pointer to interface descriptors */
    int *n, 			/* buffer for actual length of message */
    int maxSize 		/* length of (internal) buffer for message */
    )
    {
    struct if_info *ifp = NULL; 	/* descriptor of receiver's interface */
    int status; 			/* message extraction results */

    fd_set readFds;
    int fileNum = 0;
    int maxFileNum = 0;

    /* Wait for an arriving DHCP message. */

    FD_ZERO (&readFds);

    ifp = iflist;
    while (ifp != NULL)
        {
        fileNum = ifp->bpfDev;

        FD_SET (fileNum, &readFds);

        if (fileNum > maxFileNum)
            maxFileNum = fileNum;

        ifp = ifp->next;
        }
    maxFileNum++;    /* Adjust maximum for select() routine. */

#ifdef DHCPS_DEBUG
    logMsg ("DHCP server: waiting for data.\n", 0, 0, 0, 0, 0, 0);
#endif

    status = select (maxFileNum, &readFds, NULL, NULL, NULL);
    if (status <= 0)
        return (NULL);

#ifdef DHCPS_DEBUG
   logMsg ("DHCP server: read_interfaces() received new message.\n", 
           0, 0, 0, 0, 0, 0);
#endif

    /* Find the interface descriptor for the first available DHCP message. */

    ifp = iflist;
    while (ifp != NULL)
        {
        fileNum = ifp->bpfDev;
        if (FD_ISSET (fileNum, &readFds))
            break;
        ifp = ifp->next;
        }

   if (ifp == NULL) 
       {
#ifdef DHCPS_DEBUG
       logMsg ("Warning: invalid network device in read_interfaces()\n", 
               0, 0, 0, 0, 0, 0);
#endif
       return (NULL);
       }

   status = read (fileNum, ifp->buf, maxSize);
   if (status <= 0)
       {
#ifdef DHCPS_DEBUG
       logMsg ("Warning: error reading DHCP message.\n", 0, 0, 0, 0, 0, 0);
#endif
       return (NULL);
       }
   *n = status;

   return (ifp);
   }

/*******************************************************************************
*
* get_ipsum - retrieve the IP header checksum
*
* This routine fetches the checksum for the given IP header.
*
* RETURNS: Value of checksum in network byte order.
*
* ERRNO:   N/A
*
* NOMANUAL
*/

u_short get_ipsum
    (
    struct ip * 	pIph 	/* IP header */
    )
    {
    pIph->ip_sum = 0;

#if BSD<44
    return (checksum ( (u_short *)pIph, (pIph->ip_v_hl & 0xf) << 2));
#else
    return (checksum ( (u_short *)pIph, pIph->ip_hl << 2));
#endif
    }

/*******************************************************************************
*
* check_ipsum - verify the IP header checksum
*
* This routine retrieves the checksum for the given IP header and compares
* it to the received checksum.
*
* RETURNS: TRUE if checksums match, or FALSE otherwise.
*
* ERRNO:   N/A
*
* NOMANUAL
*/

BOOL check_ipsum
    (
    struct ip * 	pIph 	/* Received IP header */
    )
    {
    u_short ripcksum;   /* received IP checksum */
    ripcksum = pIph->ip_sum;

    return (ripcksum == get_ipsum (pIph));
    }

/*******************************************************************************
*
* get_udpsum - retrieve the UDP header checksum
*
* This routine fetches the checksum for a UDP header.
*
* RETURNS: Value of checksum in network byte order.
*
* ERRNO:   N/A
*
* NOMANUAL
*/

u_short get_udpsum
    (
    struct ip * 	pIph, 	/* IP header */
    struct udphdr * 	pUdph 	/* UDP header */
    )
    {
    struct ps_udph UdpPh;       /* UDP pseudo-header */

    bzero ( (char *)&UdpPh, sizeof (UdpPh));

    UdpPh.srcip.s_addr = pIph->ip_src.s_addr;
    UdpPh.dstip.s_addr = pIph->ip_dst.s_addr;
    UdpPh.zero = 0;
    UdpPh.prto = IPPROTO_UDP;
    UdpPh.ulen = pUdph->uh_ulen;
    pUdph->uh_sum = 0;

    return (udp_cksum (&UdpPh, (char *)pUdph, ntohs (UdpPh.ulen)));
    }

/*******************************************************************************
*
* check_udpsum - verify the IP header checksum
*
* This routine retrieves the checksum for a UDP header and compares it
* to the received checksum.
*
* RETURNS: TRUE if checksums match, or FALSE otherwise.
*
* ERRNO:   N/A
*
* NOMANUAL
*/

int check_udpsum
    (
    struct ip * 	pIph, 	/* Received IP header */
    struct udphdr * 	pUdph 	/* Received UDP header */
    )
    {
    u_short rudpcksum;  /* received UDP checksum */

    if (pUdph->uh_sum == 0)
        return(TRUE);
    rudpcksum = pUdph->uh_sum;

    return (rudpcksum == get_udpsum (pIph, pUdph));
    }
