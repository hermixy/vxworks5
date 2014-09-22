/* common_subr.c - DHCP library common code */

/* Copyright 1984 - 2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01l,22may02,wap  use tickGet() instead of vxTicks in dhcpTime()
01k,23apr02,wap  Implement dhcpTime() routine to use instead of time() (SPR
                 #68900)
01j,12oct01,rae  merge from truestack ver o1k, base o1f (VIRTUAL_STACK)
01i,04nov00,niq  Changing the MIB2 type names to conform to the RFC2233 names
01h,24oct00,spm  fixed modification history after tor3_x merge
01g,23oct00,niq  merged from version 01h of tor3_x branch (base version 01f)
01f,26aug97,spm  moved routines not shared by client and server
01e,10jun97,spm  isolated incoming messages in state machine from input hooks
01d,02jun97,spm  changed DHCP option tags to prevent name conflicts (SPR #8667)
                 and updated man pages
01c,07apr97,spm  added INCLUDE_FILES to module heading
01b,29jan97,spm  brought into compliance with Wind River coding standards
01a,03oct96,spm  created by modifying WIDE Project DHCP Implementation
*/

/*
DESCRIPTION
This library contains the code common to the WIDE Project implementations
of the DHCP client (runtime and boot-time versions) and the DHCP server,
modified for VxWorks compatibility, as well as some internal WRS code which
is also shared by those components.

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

/* includes */
#include "vxWorks.h"
#include "m2Lib.h"
#include "sysLib.h"
#include "tickLib.h"
#include "private/timerLibP.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/if_ether.h>

#include "dhcp/dhcp.h"

#ifdef VIRTUAL_STACK
#include "netinet/vsLib.h"
#endif /* VIRTUAL_STACK */

/* forward declarations */

u_short get_ipsum (struct ip *);
u_short get_udpsum (struct ip *, struct udphdr *);
u_short udp_cksum (struct ps_udph *, char *, int);

/*******************************************************************************
*
* dhcpConvert - convert the hardware type from the MUX
*
* This routine converts the hardware type value which the MUX interface
* provides into the encoding that DHCP uses. The MUX interface uses the
* RFC 1213 MIB values while DHCP uses the interface encodings from the ARP
* section of the assigned numbers RFC (RFC 1700).
*
* NOMANUAL
*
* RETURNS: RFC 1700 encoding of type value, or ERROR if none.
*
*/

int dhcpConvert
    (
    int muxType         /* RFC 1213 interface type value */
    )
    {
    int dhcpType = 0;

    switch (muxType)
        {
        default:
            return (ERROR);
            break;

        case M2_ifType_ethernet_csmacd:
            dhcpType = ETHER;
            break;

        case M2_ifType_ethernet3Mbit:
            dhcpType = EXPETHER;
            break;

        case M2_ifType_proteon10Mbit:     /* fall-through */
        case M2_ifType_proteon80Mbit:
            dhcpType = PRONET;
            break;

        case M2_ifType_iso88023_csmacd:    /* fall-through */
        case M2_ifType_iso88024_tokenBus:  /* fall-through */
        case M2_ifType_iso88025_tokenRing: /* fall-through */
        case M2_ifType_iso88026_man:
            dhcpType = IEEE802;
            break;

        case M2_ifType_hyperchannel:
            dhcpType = HYPERCH;
            break;

        case M2_ifType_starLan:
            dhcpType = LANSTAR;
            break;

        case M2_ifType_ultra:
            dhcpType = ULTRALINK;
            break;

        case M2_ifType_frameRelay:
            dhcpType = FRAMERELAY;
            break;

        case M2_ifType_propPointToPointSerial:    /* fall-through */
        case M2_ifType_ppp:                       /* fall-through */
        case M2_ifType_slip:
            dhcpType = SERIAL;
            break;
        }
    return (dhcpType);
    }

/*******************************************************************************
*
* pickup_opt - extract an option from a DHCP message
*
* This routine searches the fields in a DHCP message for the option specified
* by the <tag> parameter. If the file and sname message fields are overloaded 
* with options, they are searched as well. The search order is:
*
*    options field -> 'file' field -> 'sname' field
*
* RETURNS: Pointer to first occurrence of option, or NULL if not found.
*
* ERRNO:   N/A
*
* SEE ALSO: RFC 1533
*
* NOMANUAL
*/

char *	pickup_opt
    (
    struct dhcp *	msg,      /* Incoming message copied to structure */
    int			msglen,   /* Length of message */
    char		tag       /* RFC 1533 tag for desired option */
    )
    {
    BOOL 	sname_is_opt = FALSE;
    BOOL 	file_is_opt = FALSE;
    int 	i = 0;
    char *	opt = NULL;
    char *	found = NULL;

    /*  search option field. */

    opt = &msg->options[MAGIC_LEN];

    for (i = 0; i < msglen - DFLTDHCPLEN + DFLTOPTLEN - MAGIC_LEN; i++) 
        {
        if (*(opt + i) == tag) 
            {
            found = (opt + i);
            break;
            }
        else if (*(opt + i) == _DHCP_END_TAG) 
            break;
        else if (*(opt + i) == _DHCP_OPT_OVERLOAD_TAG) 
            {
            i += 2 ;
            if (*(opt + i) == 1)
                file_is_opt = TRUE;
            else if (*(opt + i) == 2)
                sname_is_opt = TRUE;
            else if (*(opt + i) == 3)
                file_is_opt = sname_is_opt = TRUE;
            continue;
            }
        else if (*(opt + i) == _DHCP_PAD_TAG) 
            continue;
        else 
            i += *(u_char *)(opt + i + 1) + 1;
        }

    if (found != NULL)
        return (found);

    /* If necessary, search file field. */

    if (file_is_opt)
        {
        opt = msg->file;
        for (i = 0; i < sizeof (msg->file); i++) 
             {
             if (*(opt + i) == _DHCP_PAD_TAG) 
                 continue;
             else if (*(opt + i) == _DHCP_END_TAG) 
                 break;
             else if (*(opt + i) == tag) 
                 {
                 found = (opt + i);
                 break;
                 }
             else 
                 i += *(u_char *)(opt + i + 1) + 1;
             }
        if (found != NULL)
            return (found);
        }

    /* If necessary, search sname field. */

    if (sname_is_opt) 
        {
        opt = msg->sname;
        for (i = 0; i < sizeof (msg->sname); i++) 
            {
            if (*(opt + i) == _DHCP_PAD_TAG) 
                continue;
            else if (*(opt + i) == _DHCP_END_TAG) 
                break;
            else if (*(opt + i) == tag) 
                {
                found = (opt + i);
                break;
                }
            else
                i += *(u_char *)(opt + i + 1) + 1;
            }
        if (found != NULL)
            return(found);
        }

    return(NULL);
    }

/*******************************************************************************
*
* udp_cksum - calculate the UDP header checksum
*
* This routine calculates the checksum for the given UDP pseudo-header and 
* additional bytes of data contained in the provided buffer, interpreted
* as unsigned shorts in network byte order.
*
* RETURNS: Value of calculated checksum, or 0xffff instead of 0.
*
* ERRNO:   N/A
*
* NOMANUAL
*/

u_short udp_cksum
    (
    struct ps_udph *pUdpPh,	/* UDP pseudo-header */
    char *buf,                  /* Additional data (received UDP header) */
    int n                       /* Length of provided buffer */
    )
    {
    u_long 		sum = 0;
    u_short *		tmp = NULL;
    u_short		result;
    FAST int 		i = 0;
    unsigned char 	pad[2];

    tmp = (u_short *) pUdpPh;
    for (i = 0; i < 6; i++) 
        {
        sum += *tmp++;
        }      

    tmp = (u_short *)buf;
    while (n > 1) 
        {
        sum += *tmp++;
        n -= sizeof (u_short);
        }

    if (n == 1)		/* n % 2 == 1, so padding is needed */
        {
        pad [0] = *(u_char *)tmp;
        pad [1] = 0;
        tmp = (u_short *)pad;
        sum += *tmp;
        }

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    result = (u_short) ~sum;
    if (result == 0)
        result = 0xffff;

    return (result);
    }

/*******************************************************************************
*
* dhcpTime - return system uptime
*
* This routine is similar to the POSIX time() routine, except it returns
* the time in terms of the number of seconds that the system has been
* running and is immune to changes in the system clock made by
* clock_settime(). Note however that it is _not_ immune to someone
* calling tickSet(). This is not the same thing as changing the
* calendar time though and should happen less frequently.
*
* RETURNS: Number of seconds since the system was booted.
*
* ERRNO:   N/A
*
* NOMANUAL
*/

time_t dhcpTime
    (
    time_t *timer
    )
    {
    ULONG      sysTicks;
 
    sysTicks = tickGet()/sysClkRateGet();

    if (timer != NULL)
        *timer = (time_t) sysTicks;
    return (time_t) (sysTicks);
    }
