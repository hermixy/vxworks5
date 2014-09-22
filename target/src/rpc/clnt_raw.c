/* clnt_raw.c - memory based rpc for simple testing and timing */

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
01l,15oct01,rae  merge from truestack ver 01m, base 01k (AE /5_X)
01k,26may92,rrr  the tree shuffle
		  -changed includes to have absolute path from h/
01j,04oct91,rrr  passed through the ansification filter
		  -changed includes to have absolute path from h/
		  -changed VOID to void
		  -changed copyright notice
01i,01apr91,elh   added clnt_rawInclude.
01h,25oct90,dnw   removed include of utime.h.
01g,08oct90,hjb   de-linted.
01f,02oct90,hjb   made raw rpc cleanup after itself properly just for the
		    hell of it.
01e,10may90,dnw   removed _raw_buf back to rpcGbl: it must be shared w/svc_raw.
		  changed clnt_rawInit to alloc raw_buf if necessary
		  changed to call clnt_rawInit at start of every routine (not
		    in rpcTaskInit anymore)
01d,27oct89,hjb   upgraded to 4.0
01c,19apr89,gae   added clnt_rawExit to do tidy cleanup for tasks.
		  changed clnt_rawInit to return pointer to moduleStatics.
01b,11nov87,jlf   added wrs copyright, title, mod history, etc.
01a,01nov87,rdc   first VxWorks version
*/

#ifndef lint
/* static char sccsid[] = "@(#)clnt_raw.c 1.1 86/02/03 Copyr 1984 Sun Micro"; */
#endif

/*
 * clnt_raw.c
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 *
 * Memory based rpc for simple testing and timing.
 * Interface to create an rpc client and server in the same process.
 * This lets us similate rpc and get round trip overhead, without
 * any interference from the kernal.
 */

#include "rpc/rpctypes.h"
#include "netinet/in.h"
#include "rpc/xdr.h"
#include "rpc/auth.h"
#include "rpc/clnt.h"
#include "rpc/rpc_msg.h"
#include "vxWorks.h"
#include "memLib.h"
#include "rpc/rpcGbl.h"
#include "memPartLib.h"
#include "stdio.h"

#define MCALL_MSG_SIZE 24

struct moduleStatics
    {
    CLIENT client_object;
    XDR	xdr_stream;
    char mashl_callmsg[MCALL_MSG_SIZE];
    u_int mcnt;
    };

LOCAL void clnt_rawExit ();
LOCAL struct moduleStatics *clnt_rawInit ();

LOCAL enum clnt_stat	clntraw_call();			/* 4.0 */
LOCAL void		clntraw_abort();		/* 4.0 */
LOCAL void		clntraw_geterr();		/* 4.0 */
LOCAL bool_t		clntraw_freeres();		/* 4.0 */
LOCAL void		clntraw_destroy();		/* 4.0 */
LOCAL bool_t		clntraw_control();		/* 4.0 */

LOCAL struct clnt_ops client_ops = {			/* 4.0 */
	clntraw_call,
	clntraw_abort,
	clntraw_geterr,
	clntraw_freeres,
	clntraw_destroy,
	clntraw_control					/* 4.0 */
};

IMPORT bool_t xdr_opaque_auth ();

void	svc_getreq();

void clnt_rawInclude ()
     {
     }

LOCAL struct moduleStatics *clnt_rawInit ()

    {
    FAST struct moduleStatics *pClnt_raw;

    /* check if already initialized */

    if (taskRpcStatics->clnt_raw != NULL)
	return (taskRpcStatics->clnt_raw);


    /* allocate clnt/svc buffer if necessary */

    if (taskRpcStatics->_raw_buf == NULL)
	{
	taskRpcStatics->_raw_buf = (char *) KHEAP_ALLOC(UDPMSGSIZE);
	if (taskRpcStatics->_raw_buf == NULL)
	    {
	    printErr ("clnt_rawInit: out of memory!\n");
	    return (NULL);
	    }
	}


    /* allocate module statics */

    pClnt_raw =
	 (struct moduleStatics *) KHEAP_ALLOC(sizeof (struct moduleStatics));
    bzero ((char *)pClnt_raw, sizeof(struct moduleStatics));

    if (pClnt_raw == NULL)
	printErr ("clnt_rawInit: out of memory!\n");

    taskRpcStatics->clnt_raw = pClnt_raw;
    taskRpcStatics->clnt_rawExit = (void (*) ()) clnt_rawExit;

    return (pClnt_raw);
    }

LOCAL void clnt_rawExit ()

    {
    if (taskRpcStatics->_raw_buf != NULL)
	{
        KHEAP_FREE(taskRpcStatics->_raw_buf);
	taskRpcStatics->_raw_buf = NULL;
	}

    KHEAP_FREE((char *) taskRpcStatics->clnt_raw);
    }
/*
 * Create a client handle for memory based rpc.
 */
CLIENT *
clntraw_create(prog, vers)
	u_long prog;
	u_long vers;
{
	struct rpc_msg call_msg;
	FAST struct moduleStatics *ms = clnt_rawInit ();
	CLIENT	*client = &ms->client_object;
	XDR *xdrs = &ms->xdr_stream;

	/*
	 * pre-serialize the staic part of the call msg and stash it away
	 */
	call_msg.rm_direction = CALL;
	call_msg.rm_call.cb_rpcvers = RPC_MSG_VERSION;
	call_msg.rm_call.cb_prog = prog;
	call_msg.rm_call.cb_vers = vers;

	xdrmem_create(xdrs, ms->mashl_callmsg, MCALL_MSG_SIZE, XDR_ENCODE);
	if (! xdr_callhdr(xdrs, &call_msg)) {
		perror("clnt_raw.c - Fatal header serialization error.");
	}
	ms->mcnt = XDR_GETPOS(xdrs);
	XDR_DESTROY(xdrs);

	/*
	 * Set xdrmem for client/server shared buffer
	 */
	xdrmem_create(xdrs, taskRpcStatics->_raw_buf, UDPMSGSIZE, XDR_FREE);

	/*
	 * create client handle
	 */
	client->cl_ops = &client_ops;
	client->cl_auth = authnone_create();
	return (client);
}

LOCAL enum clnt_stat 				/* 4.0 */
clntraw_call(h, proc, xargs, argsp, xresults, resultsp /*, timeout LINT */)
	CLIENT *h;
	u_long proc;
	xdrproc_t xargs;
	caddr_t argsp;
	xdrproc_t xresults;
	caddr_t resultsp;
{
	struct rpc_msg msg;
	enum clnt_stat status;
	struct rpc_err error;
	FAST struct moduleStatics *ms = clnt_rawInit ();
	register XDR *xdrs = &ms->xdr_stream;

call_again:
	/*
	 * send request
	 */
	xdrs->x_op = XDR_ENCODE;
	XDR_SETPOS(xdrs, 0);
	((struct rpc_msg *)ms->mashl_callmsg)->rm_xid ++ ;
	if ((! XDR_PUTBYTES(xdrs, ms->mashl_callmsg, ms->mcnt)) ||
	    (! XDR_PUTLONG(xdrs, (long *)&proc)) ||
	    (! AUTH_MARSHALL(h->cl_auth, xdrs)) ||
	    (! (*xargs)(xdrs, argsp))) {
		return (RPC_CANTENCODEARGS);
	}
	(void)XDR_GETPOS(xdrs);  /* called just to cause overhead */

	/*
	 * We have to call server input routine here because this is
	 * all going on in one process. Yuk.
	 */
	svc_getreq(1);

	/*
	 * get results
	 */
	xdrs->x_op = XDR_DECODE;
	XDR_SETPOS(xdrs, 0);
	msg.acpted_rply.ar_verf = _null_auth;
	msg.acpted_rply.ar_results.where = resultsp;
	msg.acpted_rply.ar_results.proc = xresults;
	if (! xdr_replymsg(xdrs, &msg))
		return (RPC_CANTDECODERES);
	_seterr_reply(&msg, &error);
	status = error.re_status;

	if (status == RPC_SUCCESS) {
		if (! AUTH_VALIDATE(h->cl_auth, &msg.acpted_rply.ar_verf)) {
			status = RPC_AUTHERROR;
		}
	}  /* end successful completion */
	else {
		if (AUTH_REFRESH(h->cl_auth))
			goto call_again;
	}  /* end of unsuccessful completion */

	if (status == RPC_SUCCESS) {
		if (! AUTH_VALIDATE(h->cl_auth, &msg.acpted_rply.ar_verf)) {
			status = RPC_AUTHERROR;
		}
		if (msg.acpted_rply.ar_verf.oa_base != NULL) {
			xdrs->x_op = XDR_FREE;
			(void)xdr_opaque_auth(xdrs, &(msg.acpted_rply.ar_verf));
		}
	}

	return (status);
}

LOCAL void				/* 4.0 */
clntraw_geterr()
{
}

/* ARGSUSED */
LOCAL bool_t				/* 4.0 */
clntraw_freeres(cl, xdr_res, res_ptr)
	CLIENT *cl;
	xdrproc_t xdr_res;
	caddr_t res_ptr;
{
	FAST struct moduleStatics *ms = clnt_rawInit ();
	register XDR *xdrs = &ms->xdr_stream;

	xdrs->x_op = XDR_FREE;
	return ((*xdr_res)(xdrs, res_ptr));
}

LOCAL void				/* 4.0 */
clntraw_abort()
{
}

LOCAL bool_t				/* 4.0 */
clntraw_control()			/* 4.0 */
{					/* 4.0 */
	return (FALSE);			/* 4.0 */
}					/* 4.0 */

LOCAL void				/* 4.0 */
clntraw_destroy()
{
}
