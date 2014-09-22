/* pmap_getmaps.c - client interface to pmap rpc service */

/* Copyright 1984-2000 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
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
01j,18apr00,ham  fixed compilation warnings.
01i,11aug93,jmm  Changed ioctl.h and socket.h to sys/ioctl.h and sys/socket.h
01h,26may92,rrr  the tree shuffle
		  -changed includes to have absolute path from h/
01g,04oct91,rrr  passed through the ansification filter
		  -changed includes to have absolute path from h/
		  -changed copyright notice
01f,01apr91,elh   added pmap_getmapsInclude.
01e,25oct90,dnw   removed include of utime.h.
01d,19apr90,hjb   de-linted.
01c,27oct89,hjb   upgraded to 4.0
01b,11nov87,jlf   added wrs copyright, title, mod history, etc.
01a,01nov87,rdc   first VxWorks version
*/

#ifndef lint
/* static char sccsid[] = "@(#)pmap_getmaps.c 1.1 86/02/03 Copyr 1984 Sun Micro"; */
#endif

/*
 * pmap_getmap.c
 * Client interface to pmap rpc service.
 * contains pmap_getmaps, which is only tcp service involved
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
/* #include <netdb.h> */
/* #include <stdio.h> */
#include "errno.h"
#include "net/if.h"
#include "sys/ioctl.h"
#include "vxWorks.h"
#include "ioLib.h"

/* XXX #define NAMELEN 255					4.0 */
/* XXX#define MAX_BROADCAST_SIZE 1400				4.0 */

/* XXX extern int errno; */
/* XXX static struct sockaddr_in myaddress;			4.0 */

void pmap_getmapsInclude ()
    {
    }
/*
 * Get a copy of the current port maps.
 * Calls the pmap service remotely to do get the maps.
 */
struct pmaplist *
pmap_getmaps(address)
	 struct sockaddr_in *address;
{
	struct pmaplist *head = (struct pmaplist *)NULL;
	int socket = -1;
	struct timeval minutetimeout;
	register CLIENT *client;

	minutetimeout.tv_sec = 60;
	minutetimeout.tv_usec = 0;
	address->sin_port = htons((u_short) PMAPPORT);
	client = clnttcp_create(address, PMAPPROG,
	    PMAPVERS, &socket, 50, 500);
	if (client != (CLIENT *)NULL) {
		if (CLNT_CALL(client, PMAPPROC_DUMP, xdr_void, NULL, xdr_pmaplist,
		    &head, minutetimeout) != RPC_SUCCESS) {
			clnt_perror(client, "pmap_getmaps rpc problem");
		}
		CLNT_DESTROY(client);
	}
	(void)close(socket);
	address->sin_port = 0;
	return (head);
}
