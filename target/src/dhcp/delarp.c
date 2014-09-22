/* delarp.c - DHCP server ARP deletion code */

/* Copyright 1984 - 2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01b,09oct01,vvv  fixed mod hist
01a,07apr97,spm  created by modifying WIDE project DHCP implementation
*/

/*
DESCRIPTION
This library contains the code used by the DHCP server to remove
entries from the ARP cache for offered network addresses. This
allows accurate ICMP checks by the server to determine if the address 
is in use.

INCLUDE_FILES: None
*/

/*
 * Modified by Akihiro Tominaga (tomy@sfc.wide.ad.jp)
 */
/*
 * Copyright (c) 1984, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Sun Microsystems, Inc.
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
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
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

#include <sys/socket.h>
#include <sys/ioctl.h>

#include <net/if.h>

#include <net/route.h>

#include <netinet/in.h>
#include <netinet/if_ether.h>

#include <unistd.h>
#include <string.h>

#include "ioLib.h" 	/* ioctl() declaration */
#include "logLib.h"
#include "sockLib.h"
#include "taskLib.h"

/*******************************************************************************
*
* delarp - delete an ARP entry
*
* This routine deletes the ARP entry associated with the given IP address
* prior to checking its availability by issuing an ICMP request.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*/

void delarp
    (
    struct in_addr *target,
    int sockfd
    )
    {
    struct arpreq arq;
    struct sockaddr_in *sin = NULL;

    bzero ( (char *)&arq, sizeof (arq));
    arq.arp_pa.sa_family = AF_INET;
    sin = (struct sockaddr_in *)&arq.arp_pa;
    sin->sin_family = AF_INET;
    sin->sin_addr.s_addr = target->s_addr;
    ioctl (sockfd, SIOCDARP, (int)&arq);

    return;
    }
