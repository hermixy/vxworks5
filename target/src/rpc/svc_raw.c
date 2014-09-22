/* svc_raw.c - a toy for simple testing and timing rpc */

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
01k,15oct01,rae  merge from truestack ver 01l, base 01j (AE / 5_X)
01j,26may92,rrr  the tree shuffle
		  -changed includes to have absolute path from h/
01i,04oct91,rrr  passed through the ansification filter
		  -changed includes to have absolute path from h/
		  -changed VOID to void
		  -changed copyright notice
01h,01apr91,elh   svc_rawInclude added.
01g,08oct90,hjb   de-linted.
01f,02oct90,hjb   made raw rpc cleanup after itself properly.
01e,10may90,dnw   removed _raw_buf back to rpcGbl: it must be shared w/clnt_raw.
		  changed svc_rawInit to alloc raw_buf if necessary
		  changed to call svc_rawInit at start of every routine (not
		    in rpcTaskInit anymore)
01d,27oct89,hjb   upgraded to 4.0
01c,19apr89,gae   added svcExit to do tidy cleanup for tasks.
		  changed svc_rawInit to return pointer to moduleStatics.
01b,11nov87,jlf   added wrs copyright, title, mod history, etc.
01a,01nov87,rdc   first VxWorks version
*/

#ifndef lint
/* static char sccsid[] = "@(#)svc_raw.c 1.1 86/02/03 Copyr 1984 Sun Micro"; */
#endif

/*
 * svc_raw.c,   This a toy for simple testing and timing.
 * Interface to create an rpc client and server in the same UNIX process.
 * This lets us similate rpc and get rpc (round trip) overhead, without
 * any interference from the kernal.
 *
 */

#include "rpc/rpctypes.h"
#include "netinet/in.h"
#include "rpc/xdr.h"
#include "rpc/auth.h"
#include "rpc/clnt.h"
#include "rpc/rpc_msg.h"
#include "rpc/svc.h"
#include "vxWorks.h"
#include "memLib.h"
#include "rpc/rpcGbl.h"
#include "memPartLib.h"
#include "stdio.h"

/*
 * This is the "network" that we will be moving data over
 */


LOCAL void svc_rawExit ();
LOCAL struct moduleStatics *svc_rawInit ();

/* pointer to this external buffer is now located in rpcGbl */
/* extern char _raw_buf[UDPMSGSIZE]; */

LOCAL bool_t		svcraw_recv();
LOCAL enum xprt_stat 	svcraw_stat();
LOCAL bool_t		svcraw_getargs();
LOCAL bool_t		svcraw_reply();
LOCAL bool_t		svcraw_freeargs();
LOCAL void		svcraw_destroy();

LOCAL struct xp_ops server_ops = {
	svcraw_recv,
	svcraw_stat,
	svcraw_getargs,
	svcraw_reply,
	svcraw_freeargs,
	svcraw_destroy
};

struct moduleStatics
    {
    SVCXPRT 	server;
    XDR 	xdr_stream;
    char 	verf_body[MAX_AUTH_BYTES];
    };

void svc_rawInclude ()
    {
    }

LOCAL struct moduleStatics *svc_rawInit ()

    {
    FAST struct moduleStatics *pSvc_raw;

    /* check if already initialized */

    if (taskRpcStatics->svc_raw != NULL)
	return (taskRpcStatics->svc_raw);


    /* allocate clnt/svc buffer if necessary */

    if (taskRpcStatics->_raw_buf == NULL)
	{
	taskRpcStatics->_raw_buf = (char *) KHEAP_ALLOC(UDPMSGSIZE);
	if (taskRpcStatics->_raw_buf == NULL)
	    {
	    printErr ("svc_rawInit: out of memory!\n");
	    return (NULL);
	    }
	}

    /* allocate module statics */

    pSvc_raw = (struct moduleStatics *)
		    KHEAP_ALLOC(sizeof (struct moduleStatics));

    if (pSvc_raw == NULL)
	printErr ("svc_rawInit: out of memory!\n");

    bzero ((char *)pSvc_raw, sizeof(struct moduleStatics));
    taskRpcStatics->svc_raw = pSvc_raw;
    taskRpcStatics->svc_rawExit = (void (*) ()) svc_rawExit;

    return (pSvc_raw);
    }

LOCAL void svc_rawExit ()

    {
    if (taskRpcStatics->_raw_buf != NULL)
	{
        KHEAP_FREE(taskRpcStatics->_raw_buf);
	taskRpcStatics->_raw_buf = NULL;
	}

    KHEAP_FREE((char *) taskRpcStatics->svc_raw);
    }

SVCXPRT *
svcraw_create()
{

	FAST struct moduleStatics *ms = svc_rawInit ();

	ms->server.xp_sock = 0;
	ms->server.xp_port = 0;
	ms->server.xp_ops = &server_ops;
	ms->server.xp_verf.oa_base = ms->verf_body;
	xdrmem_create(&ms->xdr_stream, taskRpcStatics->_raw_buf, UDPMSGSIZE,
		      XDR_FREE);
	return (&ms->server);
}

LOCAL enum xprt_stat
svcraw_stat()
{

	return (XPRT_IDLE);
}
/* ARGSUSED */
LOCAL bool_t
svcraw_recv(xprt, msg)
	SVCXPRT *xprt;
	struct rpc_msg *msg;
{
	FAST struct moduleStatics *ms = svc_rawInit ();
	FAST XDR *xdrs = &ms->xdr_stream;

	xdrs->x_op = XDR_DECODE;
	XDR_SETPOS(xdrs, 0);
	if (! xdr_callmsg(xdrs, msg))
	       return (FALSE);
	return (TRUE);
}

/* ARGSUSED */
LOCAL bool_t					/* 4.0 */
svcraw_reply(xprt, msg)
	SVCXPRT *xprt;
	struct rpc_msg *msg;
{
	FAST struct moduleStatics *ms = svc_rawInit ();
	FAST XDR *xdrs = &ms->xdr_stream;

	xdrs->x_op = XDR_ENCODE;
	XDR_SETPOS(xdrs, 0);
	if (! xdr_replymsg(xdrs, msg))
	       return (FALSE);
	(void)XDR_GETPOS(xdrs);  /* called just for overhead */
	return (TRUE);
}

/* ARGSUSED */
LOCAL bool_t					/* 4.0 */
svcraw_getargs(xprt, xdr_args, args_ptr)
	SVCXPRT *xprt;
	xdrproc_t xdr_args;
	caddr_t args_ptr;
{

	FAST struct moduleStatics *ms = svc_rawInit ();

	return ((*xdr_args)(&ms->xdr_stream, args_ptr));
}

/* ARGSUSED */
LOCAL bool_t					/* 4.0 */
svcraw_freeargs(xprt, xdr_args, args_ptr)
	SVCXPRT *xprt;
	xdrproc_t xdr_args;
	caddr_t args_ptr;
{
	FAST struct moduleStatics *ms = svc_rawInit ();
	FAST XDR *xdrs = &ms->xdr_stream;

	xdrs->x_op = XDR_FREE;
	return ((*xdr_args)(xdrs, args_ptr));
}

LOCAL void					/* 4.0 */
svcraw_destroy()
{
}
