/* flushroute.c - DHCP library routing table access */

/* Copyright 1984 - 2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01a,12oct01,rae  made ______ -> ------ fix
01d,18apr97,spm  added conditional include DHCPC_DEBUG for displayed output
01c,07apr97,spm  code cleanup, modified comments
01b,27jan97,spm  brought into compliance with Wind River coding standards
01a,03oct96,spm  created by modifying WIDE Project DHCP Implementation
*/

/*
DESCRIPTION
This library contains the code which clears the routing table for the 
Wide project DHCP client, modified for vxWorks compatibility.

INCLUDE_FILES: None
*/

/*
 * Copyright (c) 1983, 1989 The Regents of the University of California.
 * All rights reserved.
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
 * Some part are modified by kei@cs.uec.ac.jp and tomy@sfc.wide.ad.jp.
 *  
 */

/* includes */

#include "vxWorks.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sockLib.h>
#include <ioLib.h>
#include <sys/ioctl.h>
#include <net/mbuf.h>
#include <net/route.h>
#include <netinet/in.h>
#include <rpc/types.h>

/*******************************************************************************
*
* flushroutes - clean the network routing table
*
* This routine removes all entries except the loopback entry from the routing 
* table for the network interface. Two variations are provided. The first
* uses the routing table as defined in the BSD 4.3 protocol stack and
* the second uses the BSD 4.4 protocol stack.
*
* RETURNS: 0 if successful, or -1 on error.
*
* ERRNO: N/A
*
* NOMANUAL
*/

#if BSD < 44
int flushroutes (void)
    {
    int s = 0;
    struct mbuf mb;
    register struct rtentry *rt = NULL;
    register struct mbuf *m = NULL;
    int rthashsize = 0, i = 0, status = 0;

    bzero ( (char *)&mb, sizeof (mb));

    if ( (s = socket(AF_INET, SOCK_RAW, 0)) < 0) 
        return(-1);

    rthashsize = RTHASHSIZ;

    /* Remove all host routing entries. */
    for (i = 0; i < rthashsize; i++) 
        for (m = rthost[i]; m != NULL; m = m->m_next)
            {
            rt = mtod (m, struct rtentry *);

            if ( ( (struct sockaddr_in *)&rt->rt_dst)->sin_addr.s_addr != 
                INADDR_LOOPBACK) 
	        status = ioctl(s, SIOCDELRT, (int)(caddr_t)rt);
            }

    for (i = 0; i < rthashsize; i++) 
        for (m = rtnet[i]; m != NULL; m = m->m_next)
            {
            rt = mtod (m, struct rtentry *);
            if ( ( (struct sockaddr_in *)&rt->rt_dst)->sin_addr.s_addr != 
                INADDR_LOOPBACK) 
	        status = ioctl(s, SIOCDELRT, (int)(caddr_t)rt);
            }

    /* The WIDE implementation ignores the IOCTL results, so we do too. */
    close (s);
    return (0);
    }
#else

/* externals */

IMPORT int sysctl_rtable(int *, int, caddr_t, size_t *, caddr_t *, size_t);

int flushroutes (void)
    {
    int s;
    size_t needed;
    int mib[3], rlen, seqno;
    char *buf, *next, *lim;
    register struct rt_msghdr *rtm;

    if ( (s = socket (PF_ROUTE, SOCK_RAW, 0)) < 0) 
        {
#ifdef DHCPC_DEBUG
        printf ("Warning: socket() error in flushroutes()");
#endif
        return (-1);
        }

    shutdown(s, 0);
    mib[0] = AF_INET;
    mib[1] = NET_RT_DUMP;
    mib[2] = 0;             /* no flags */

    /* Retrieve the size of the routing table and allocate needed memory. */

    if (sysctl_rtable (mib, 3, NULL, &needed, NULL, 0) < 0) 
        {
#ifdef DHCPC_DEBUG
        printf ("Warning: sysctl() error in flushroutes()");
#endif
        close (s);
        return (-1);
        }
    if ( (buf = calloc(1, needed)) == NULL) 
        {
#ifdef DHCPC_DEBUG
        printf ("Warning: calloc() error in flushroutes()");
#endif
        close (s);
        return(-1);
        }

    /* Copy the current routing table to the allocated buffer. */

    if (sysctl_rtable(mib, 3, buf, &needed, NULL, 0) < 0) 
        {
#ifdef DHCPC_DEBUG
        printf("Warning: sysctl() error in flushroutes()");
#endif
        free(buf);
        close(s);
        return(-1);
        }

    lim = buf + needed;
    seqno = 0;

    /* Delete each route contained in the routing tables. */

    for (next = buf; next < lim; next += rtm->rtm_msglen) 
         {
         rtm = (struct rt_msghdr *) next;

         if ((rtm->rtm_flags & (RTF_GATEWAY | RTF_LLINFO)) == 0)
             continue;
         rtm->rtm_type = RTM_DELETE;
         rtm->rtm_seq = seqno;
         rlen = write(s, next, rtm->rtm_msglen);
         if (rlen < (int)rtm->rtm_msglen) 
             break;
         seqno++;
         }

    free(buf);
    close(s);
    return(0);
    }
#endif    /* BSD 4.4 version */

