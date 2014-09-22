/* get_myaddress.c - get client's IP address via ioctl */

/* Copyright 1989-2000 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/* @(#)get_myaddress.c	2.1 88/07/29 4.0 RPCSRC */
/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 *
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 *
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 *
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */
#if !defined(lint) && defined(SCCSIDS)
/* static char sccsid[] = "@(#)get_myaddress.c 1.4 87/08/11 Copyr 1984 Sun Micro"; */
#endif

/*
modification history
--------------------
01g,18apr00,ham  fixed compilation warnings.  
01f,11aug93,jmm  Changed ioctl.h and socket.h to sys/ioctl.h and sys/socket.h
01e,26may92,rrr  the tree shuffle
		  -changed includes to have absolute path from h/
01d,04oct91,rrr  passed through the ansification filter
		  -added include "rpc/xdr.h"
		  -changed includes to have absolute path from h/
		  -changed copyright notice
01c,18may91,hjb  included vxWorks.h
01b,19apr90,hjb  de-linted.
01a,11jul89,hjb  first VxWorks version for RPC 4.0 upgrade
*/

/*
 * get_myaddress.c
 *
 * Get client's IP address via ioctl.  This avoids using the yellowpages.
 * Copyright (C) 1984, Sun Microsystems, Inc.
 */

#include "vxWorks.h"
#include "stdio.h"
#include "ioLib.h"
#include "rpc/rpctypes.h"
#include "rpc/xdr.h"
#include "rpc/pmap_prot.h"
#include "sys/socket.h"
#include "sockLib.h"
#include "net/if.h"
#include "sys/ioctl.h"
#include "netinet/in.h"
#include "taskLib.h"

#ifdef BUFSIZ
#undef BUFSIZ
#endif
#define BUFSIZ 1024
#define SOCKSIZ 40

/*
 * don't use gethostbyname, which would invoke yellow pages
 */
void
get_myaddress(addr)
	struct sockaddr_in *addr;
{
	int s;
	char buf[BUFSIZ], ifreqBuf [SOCKSIZ];
	struct ifconf ifc;
	struct ifreq * ifr;
	int len;

	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
	    perror("get_myaddress: socket");
	    taskSuspend (0);
	}
	ifc.ifc_len = sizeof (buf);
	ifc.ifc_buf = buf;
	if (ioctl (s, SIOCGIFCONF, (int)&ifc) < 0) {
		perror("get_myaddress: ioctl (get interface configuration)");
		taskSuspend (0);
	}
	ifr = ifc.ifc_req;
	for (len = ifc.ifc_len; len; len -= (sizeof(ifr->ifr_name) + 
					     ifr->ifr_addr.sa_len))
	    {
	    bcopy ((caddr_t)ifr, ifreqBuf, (sizeof(ifr->ifr_name) + 
					    ifr->ifr_addr.sa_len)); 
		if (ioctl(s, SIOCGIFFLAGS, (int)ifreqBuf) < 0) {
			perror("get_myaddress: ioctl");
			taskSuspend (0);
		}
		if ((((struct ifreq *)ifreqBuf)->ifr_flags & IFF_UP) &&
		    ifr->ifr_addr.sa_family == AF_INET) {
			*addr = *((struct sockaddr_in *)&ifr->ifr_addr);
			addr->sin_port = htons((u_short) PMAPPORT);
			break;
		}
		ifr = (struct ifreq *)((char *)ifr + (sizeof(ifr->ifr_name) + 
				     ifr->ifr_addr.sa_len));
	    }
	(void) close(s);
}
