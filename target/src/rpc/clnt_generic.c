/* clnt_generic.c - generic client creation */

/* Copyright 1989-2000 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/* @(#)clnt_generic.c	2.1 87/11/04 3.9 RPCSRC */
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
/* static char sccsid[] = "@(#)clnt_generic.c 1.4 87/08/11 (C) 1987 SMI"; */
#endif
/*
 * Copyright (C) 1987, Sun Microsystems, Inc.
 */

/*
modification history
--------------------
01i,18apr00,ham  fixed compilation warnings.
01h,11aug93,jmm  Changed ioctl.h and socket.h to sys/ioctl.h and sys/socket.h
01g,26may92,rrr  the tree shuffle
01f,04oct91,rrr  passed through the ansification filter
		  -changed includes to have absolute path from h/
		  -changed copyright notice
01e,01apr91,elh   added clnt_genericInclude.
01d,25oct90,dnw   removed include of utime.h.
01c,10may90,dnw   changed taskModuleList->rpccreateerr to rpccreateerr macro
01b,19apr90,hjb   de-linted.
01a,11jul89,hjb   first VxWorks version for RPC 4.0 upgrade
*/

#include "rpc/rpc.h"
#include "sys/socket.h"
#include "vxWorks.h"
#include "string.h"
#include "memLib.h"
#include "rpc/rpcGbl.h"
#include "inetLib.h"
#include "hostLib.h"

#ifndef CLSET_TIMEOUT
#define CLSET_TIMEOUT 1
#endif

void clnt_genericInclude ()
    {
    }

/*
 * Generic client creation: takes (hostname, program-number, protocol) and
 * returns client handle. Default options are set, which the user can
 * change using the rpc equivalent of ioctl()'s.
 */
CLIENT *
clnt_create(hostname, prog, vers, proto)
	char *hostname;
	unsigned prog;
	unsigned vers;
	char *proto;
{
	int h;
	struct sockaddr_in sin;
	int sock;
	struct timeval tv;
	CLIENT *client;

	if (((h = hostGetByName (hostname)) == ERROR) &&
	    ((h = (int) inet_addr (hostname)) == ERROR))
	    {
	    rpc_createerr.cf_stat = RPC_UNKNOWNHOST;
	    return (NULL);
	    }
	sin.sin_family = AF_INET;
	sin.sin_port = 0;
	bzero(sin.sin_zero, sizeof(sin.sin_zero));
	sin.sin_addr.s_addr = h;
	sock = RPC_ANYSOCK;
	if (strcmp (proto, "udp") == 0)
	    {
	    tv.tv_sec = 5;
	    tv.tv_usec = 0;
	    client = clntudp_create(&sin, (u_long)prog, (u_long)vers,
				    tv, &sock);
	    if (client == NULL)
		    return (NULL);
	    }
	else if (strcmp (proto, "tcp") == 0)
	    {
	    client = clnttcp_create(&sin, (u_long)prog, (u_long)vers,
				    &sock, 0, 0);
	    if (client == NULL)
		    return (NULL);
	    }
	else
	    {
	    rpc_createerr.cf_stat = RPC_SYSTEMERROR;
	    return (NULL);
	    }
	return (client);
}
