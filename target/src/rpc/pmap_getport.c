/* pmap_getport.c - client interface to pmap rpc service */

/* Copyright 1984-1992 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
 * Copyright (c) 1987 Wind River Systems, Inc.
 * Copyright (C) 1984, Sun Microsystems, Inc.
 *
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

/*
modification history
--------------------
01j,11aug93,jmm  Changed ioctl.h and socket.h to sys/ioctl.h and sys/socket.h
01i,26may92,rrr  the tree shuffle
		  -changed includes to have absolute path from h/
01h,04oct91,rrr  passed through the ansification filter
		  -changed includes to have absolute path from h/
		  -changed copyright notice
01g,25oct90,dnw  removed include of utime.h.
01f,26jun90,hjb  removed sccsid[].
01e,10may90,dnw  changed taskModuleList->rpccreateerr to rpccreateerr macro
		 removed close() of socket which ALWAYS failed (see comment
		    in code).
01d,19apr90,hjb  de-linted.
01c,18mar90,jcf  fixed header dependency.
01b,11nov87,jlf  added wrs copyright, title, mod history, etc.
01a,01nov87,rdc  first VxWorks version
*/

#ifndef lint
/* static char sccsid[] = "@(#)pmap_getport.c 1.1 86/02/03 Copyr 1984 Sun Micro"; */
#endif

/*
 * pmap_getport.c
 * Client interface to pmap rpc service.
 *
 */

#include "rpc/rpctypes.h"
#include "netinet/in.h"
#include "rpc/xdr.h"
#include "rpc/auth.h"
#include "rpc/clnt.h"
#include "rpc/rpc_msg.h"
#include "rpc/pmap_prot.h"
#include "rpc/pmap_clnt.h"
#include "sys/socket.h"
#include "vxWorks.h"
#include "net/if.h"
#include "sys/ioctl.h"
#include "rpc/rpcGbl.h"
#define NAMELEN 255

static struct timeval timeout = { 5, 0 };
static struct timeval tottimeout = { 60, 0 };

/*
 * Find the mapped port for program,version.
 * Calls the pmap service remotely to do the lookup.
 * Returns 0 if no map exists.
 */
u_short
pmap_getport(address, program, version, protocol)
	struct sockaddr_in *address;
	u_long program;
	u_long version;
	u_long protocol;
{
	u_short port = 0;
	int socket = -1;
	register CLIENT *client;
	struct pmap parms;

	address->sin_port = htons((u_short) PMAPPORT);
	client = clntudp_bufcreate(address, PMAPPROG,
	    PMAPVERS, timeout, &socket, RPCSMALLMSGSIZE, RPCSMALLMSGSIZE);
	if (client != (CLIENT *)NULL) {
		parms.pm_prog = program;
		parms.pm_vers = version;
		parms.pm_prot = protocol;
		parms.pm_port = 0;  /* not needed or used */
		if (CLNT_CALL(client, PMAPPROC_GETPORT, xdr_pmap, &parms,
		    xdr_u_short, &port, tottimeout) != RPC_SUCCESS){
			rpc_createerr.cf_stat = RPC_PMAPFAILURE;
			clnt_geterr(client, &rpc_createerr.cf_error);
		} else if (port == 0) {
			rpc_createerr.cf_stat = RPC_PROGNOTREGISTERED;
		}
		CLNT_DESTROY(client);
	}
	/* XXX
	 * the following close ALWAYS fails because either the
	 * bufcreate failed, in which case no socket was opened,
	 * or the bufcreate succeeded, in which case CLNT_DESTROY
	 * was called, which closed the socket.
	 */
	/* (void)close(socket); */
	address->sin_port = 0;
	return (port);
}
