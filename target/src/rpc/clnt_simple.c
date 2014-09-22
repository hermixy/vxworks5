/* clnt_simple.c - simplified front end to rpc */

/* Copyright 1984-2001 Wind River Systems, Inc. */
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
01r,05nov01,vvv  fixed compilation warning
01q,18apr00,ham  fixed compilation warnings.
01p,11aug93,jmm  Changed ioctl.h and socket.h to sys/ioctl.h and sys/socket.h
01o,26may92,rrr  the tree shuffle
01n,04oct91,rrr  passed through the ansification filter
		  -changed includes to have absolute path from h/
		  -changed copyright notice
01m,01apr91,elh  added clnt_simpleInclude.
01l,25oct90,dnw  removed include of utime.h.
01k,26jun90,hjb  removed sccsid[].
01j,10may90,dnw  moved moduleStatics to rpcGbl; removed init/exit routines
		 changed taskModuleList to taskRpcStatics
		 changed taskModuleList->rpccreateerr to rpccreateerr macro
01i,11may90,yao  added missing modification history (01h) for the last checkin.
01h,09may90,yao  typecasted malloc to (char *).
01g,19apr90,hjb  de-linted.
01f,29apr89,gae  added clnt_simpleExit to do tidy cleanup for tasks.
		 changed clnt_simpleInit to return pointer to moduleStatics.
01d,30may88,dnw  changed to v4 names.
01c,29mar88,rdc   incorrect check of status returned by remGetHostByName
		  in callrpc fixed.
01b,11nov87,jlf   added wrs copyright, title, mod history, etc.
01a,01nov87,rdc   first VxWorks version
*/

#ifndef lint
/* static char sccsid[] = "@(#)clnt_simple.c 1.1 86/02/03 Copyr 1984 Sun Micro"; */
#endif

/*
 * clnt_simple.c
 * Simplified front end to rpc.
 *
 */

#include "rpc/rpc.h"
#include "sys/socket.h"
#include "vxWorks.h"
#include "memLib.h"
#include "string.h"
#include "rpc/rpcGbl.h"
#include "hostLib.h"
#include "unistd.h"

void clnt_simpleInclude ()
    {
    }

int
callrpc(host, prognum, versnum, procnum, inproc, in, outproc, out)
	char *host;
	xdrproc_t inproc, outproc;
	char *in, *out;
{
	struct sockaddr_in server_addr;
	enum clnt_stat clnt_stat;
	int hp;
	struct timeval timeout, tottimeout;
	FAST struct clnt_simple *ms = &taskRpcStatics->clnt_simple;

	if (ms->valid && ms->oldprognum == prognum && ms->oldversnum == versnum
		&& strcmp(ms->oldhost, host) == 0) {
		/* reuse old client */
	}
	else {
		ms->valid = 0;
		close(ms->socket);
		ms->socket = RPC_ANYSOCK;
		if (ms->client) {
			clnt_destroy(ms->client);
			ms->client = NULL;
		}
/* XXX		if ((hp = gethostbyname(host)) == NULL) */
		if ((hp = hostGetByName (host)) == ERROR)
			return ((int) RPC_UNKNOWNHOST);
		timeout.tv_usec = 0;
		timeout.tv_sec = 5;
/* XXX		bcopy(hp->h_addr, &server_addr.sin_addr, hp->h_length); */
		server_addr.sin_addr.s_addr = hp;
		server_addr.sin_family = AF_INET;
		server_addr.sin_port =  0;
		if ((ms->client = clntudp_create(&server_addr, (u_long)prognum,
		    (u_long)versnum, timeout, &ms->socket)) == NULL)
			return ((int) rpc_createerr.cf_stat);
		ms->valid = 1;
		ms->oldprognum = prognum;
		ms->oldversnum = versnum;
		strcpy(ms->oldhost, host);
	}
	tottimeout.tv_sec = 25;
	tottimeout.tv_usec = 0;
	clnt_stat = clnt_call(ms->client, procnum, inproc, in,
	    outproc, out, tottimeout);
	/*
	 * if call failed, empty cache
	 */
	if (clnt_stat != RPC_SUCCESS)
		ms->valid = 0;
	return ((int) clnt_stat);
}
