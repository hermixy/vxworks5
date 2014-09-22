/* clnt_tcp.c - implements a TCP/IP based, client side RPC */

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
01t,05nov01,vvv  fixed compilation warning
01s,15oct01,rae  merge from truestack ver 01t, base 01r (NULL check)
01r,01aug96,dbt  close the socket if connection failed in clnttc_create
		 (SPR #3803).
		 Updated copyright.
01q,11aug93,jmm  Changed ioctl.h and socket.h to sys/ioctl.h and sys/socket.h
01p,17sep92,wmd  changed how assignment of ntohl(--(*msg_x_id)) to x_id is
		 written to avoid erroneous side-effect by i960 compiler.
01o,26may92,rrr  the tree shuffle
		  -changed includes to have absolute path from h/
01n,04oct91,rrr  passed through the ansification filter
		  -changed includes to have absolute path from h/
		  -changed VOID to void
		  -changed copyright notice
01m,01apr91,elh   removed clnt_tcpInit (this is included via clnt_generic).
01l,25oct90,dnw   removed include of utime.h.
01k,24oct90,dnw   changed clnt_tcpInit from void to void.
01j,10may90,dnw   changed taskModuleList->rpccreateerr to rpccreateerr macro
		  changed to use rpcErrnoGet instead of errnoGet
01i,19apr90,hjb   de-linted.
01h,27oct89,hjb   upgraded to 4.0
01g,22jun88,dnw   name tweaks.
01f,30may88,dnw   changed to v4 names.
01e,05apr88,gae   updated select() to BSD4.3, i.e. used "fd_set".
		  changed fprintf() to printErr().
01d,22feb88,jcf   made kernel independent.
01c,12feb88,rdc   added clnt_tcpInit.
		  changed tcpread and tcpwrite to clnt_tcpread, clnt_tcpwrite
		  to avoid clash with corresponding routines in svc_tcp.c.
01b,11nov87,jlf   added wrs copyright, title, mod history, etc.
01a,01nov87,rdc   first VxWorks version
*/

#ifndef lint
/* static char sccsid[] = "@(#)clnt_tcp.c 1.1 86/02/03 Copyr 1984 Sun Micro"; */
#endif

/*
 * clnt_tcp.c, Implements a TCP/IP based, client side RPC.
 *
 *
 * TCP based RPC supports 'batched calls'.
 * A sequence of calls may be batched-up in a send buffer.  The rpc call
 * return immediately to the client even though the call was not necessarily
 * sent.  The batching occurs iff the results' xdr routine is NULL (0) AND
 * the rpc timeout value is zero (see clnt.h, rpc).
 *
 * Clients should NOT casually batch calls that in fact return results; that is,
 * the server side should be aware that a call is batched and not produce any
 * return message.  Batched calls that produce many result messages can
 * deadlock (netlock) the client and the server....
 *
 * Now go hang yourself.
 */

#include "rpc/rpctypes.h"
#include "sys/socket.h"
#include "netinet/in.h"
#include "errno.h"
#include "rpc/xdr.h"
#include "rpc/auth.h"
#include "rpc/clnt.h"
#include "rpc/rpc_msg.h"
#include "rpc/pmap_clnt.h"
#include "vxWorks.h"
#include "rpc/rpcGbl.h"
#include "memLib.h"
#include "stdio.h"
#include "unistd.h"
#include "rpcLib.h"
#include "sockLib.h"
#include "tickLib.h"

#define MCALL_MSG_SIZE 24


LOCAL int	clnt_readtcp();
LOCAL int	clnt_writetcp();

LOCAL enum clnt_stat	clnttcp_call();				/* 4.0 */
LOCAL void		clnttcp_abort();			/* 4.0 */
LOCAL void		clnttcp_geterr();			/* 4.0 */
LOCAL bool_t		clnttcp_freeres();			/* 4.0 */
LOCAL bool_t		clnttcp_control();			/* 4.0 */
LOCAL void		clnttcp_destroy();			/* 4.0 */

static struct clnt_ops tcp_ops = {
	clnttcp_call,
	clnttcp_abort,
	clnttcp_geterr,
	clnttcp_freeres,
	clnttcp_destroy,
	clnttcp_control						/* 4.0 */
};

struct ct_data {
	int		ct_sock;
	bool_t		ct_closeit;	/* 4.0 */
	struct timeval	ct_wait;
	bool_t		ct_waitset;	/* 4.0 - wait bit set by clnt_control */
	struct sockaddr_in ct_addr;	/* 4.0 */
	struct rpc_err	ct_error;
	char		ct_mcall[MCALL_MSG_SIZE];	/* marshalled callmsg */
	u_int		ct_mpos;			/* pos after marshal */
	XDR		ct_xdrs;
};

IMPORT bool_t xdr_opaque_auth ();


/*
 * Create a client handle for a tcp/ip connection.
 * If *sockp<0, *sockp is set to a newly created TCP socket and it is
 * connected to raddr.  If *sockp non-negative then
 * raddr is ignored.  The rpc/tcp package does buffering
 * similar to stdio, so the client must pick send and receive buffer sizes,];
 * 0 => use the default.
 * If raddr->sin_port is 0, then a binder on the remote machine is
 * consulted for the right port number.
 * NB: *sockp is copied into a private area.
 * NB: It is the clients responsibility to close *sockp.
 * NB: The rpch->cl_auth is set null authentication.  Caller may wish to set this
 * something more useful.
 */
CLIENT *
clnttcp_create(raddr, prog, vers, sockp, sendsz, recvsz)
	struct sockaddr_in *raddr;
	u_long prog;
	u_long vers;
	register int *sockp;
	u_int sendsz;
	u_int recvsz;
{
	CLIENT *h;
	register struct ct_data *ct = NULL;
	/*struct timeval now;*/
	struct rpc_msg call_msg;

	h  = (CLIENT *)mem_alloc(sizeof(*h));
	if (h == NULL) {
		printErr ("clnttcp_create: out of memory\n");
		rpc_createerr.cf_stat = RPC_SYSTEMERROR;
 		rpc_createerr.cf_error.re_errno = rpcErrnoGet ();
		goto fooy;
	}
	ct = (struct ct_data *)mem_alloc(sizeof(*ct));
	if (ct == NULL) {
		printErr ("clnttcp_create: out of memory\n");
		rpc_createerr.cf_stat = RPC_SYSTEMERROR;
		rpc_createerr.cf_error.re_errno = rpcErrnoGet ();
		goto fooy;
	}

	/*
	 * If no port number given ask the pmap for one
	 */
	if (raddr->sin_port == 0) {
		u_short port;
		if ((port = pmap_getport(raddr, prog, vers, (u_long)IPPROTO_TCP)) == 0) {
			mem_free((caddr_t)ct, sizeof(struct ct_data));
			mem_free((caddr_t)h, sizeof(CLIENT));
			return ((CLIENT *)NULL);
		}
		raddr->sin_port = htons(port);
	}

	/*
	 * If no socket given, open one
	 */
	if (*sockp < 0) {
		if (((*sockp = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
		    || (connect(*sockp, (struct sockaddr *)raddr,
		    sizeof(*raddr)) < 0)) {
			if (*sockp >= 0)
			    (void) close (*sockp);
			rpc_createerr.cf_stat = RPC_SYSTEMERROR;
			rpc_createerr.cf_error.re_errno = rpcErrnoGet ();
			goto fooy;
		}
		ct->ct_closeit = TRUE;			/* 4.0 */
	} else {
		ct->ct_closeit = FALSE;			/* 4.0 */
	}

	/*
	 * Set up private data struct
	 */
	ct->ct_sock = *sockp;
	ct->ct_wait.tv_usec = 0;
	ct->ct_waitset = FALSE;			/* 4.0 */
	ct->ct_addr = *raddr;			/* 4.0 */

	/*
	 * Initialize call message
	 */
	/* XXX (void)gettimeofday(&now, (struct timezone *)0); */
	/* call_msg.rm_xid = getpid() ^  now.tv_sec ^ now.tv_usec; */
	call_msg.rm_xid = taskIdSelf () ^  tickGet ();
	call_msg.rm_direction = CALL;
	call_msg.rm_call.cb_rpcvers = RPC_MSG_VERSION;
	call_msg.rm_call.cb_prog = prog;
	call_msg.rm_call.cb_vers = vers;

	/*
	 * pre-serialize the staic part of the call msg and stash it away
	 */
	xdrmem_create(&(ct->ct_xdrs), ct->ct_mcall, MCALL_MSG_SIZE,
	    XDR_ENCODE);
	if (! xdr_callhdr(&(ct->ct_xdrs), &call_msg)) {
		if (ct->ct_closeit) 		/* 4.0 */
		    {
		    (void) close (*sockp);
		    }
		goto fooy;
	}
	ct->ct_mpos = XDR_GETPOS(&(ct->ct_xdrs));
	XDR_DESTROY(&(ct->ct_xdrs));

	/*
	 * Create a client handle which uses xdrrec for serialization
	 * and authnone for authentication.
	 */
	xdrrec_create(&(ct->ct_xdrs), sendsz, recvsz,
	    (caddr_t)ct, clnt_readtcp, clnt_writetcp);
	h->cl_ops = &tcp_ops;
	h->cl_private = (caddr_t) ct;
	h->cl_auth = authnone_create();
	return (h);

fooy:
	/*
	 * Something goofed, free stuff and barf
	 */

	if (ct != NULL)
	    mem_free((caddr_t)ct, sizeof(struct ct_data));
	if (h != NULL)
	    mem_free((caddr_t)h, sizeof(CLIENT));

/* XXX	(void)close(*sockp);    RPC 4.0 doesn't do this -- it checks ct_closeit
				instead */

	return ((CLIENT *)NULL);
}

LOCAL enum clnt_stat				/* 4.0 */
clnttcp_call(h, proc, xdr_args, args_ptr, xdr_results, results_ptr, timeout)
	register CLIENT *h;
	u_long proc;
	xdrproc_t xdr_args;
	caddr_t args_ptr;
	xdrproc_t xdr_results;
	caddr_t results_ptr;
	struct timeval timeout;
{
	register struct ct_data *ct = (struct ct_data *) h->cl_private;
	register XDR *xdrs = &(ct->ct_xdrs);
	struct rpc_msg reply_msg;
	u_long x_id;
	u_long *msg_x_id = (u_long *)(ct->ct_mcall);	/* yuk */
	register bool_t shipnow;
	int refreshes = 2;			/* 4.0 */

	if (!ct->ct_waitset)			/* 4.0 */
	    {					/* 4.0 */
	    ct->ct_wait = timeout;
	    }					/* 4.0 */

	shipnow =
	    (xdr_results == (xdrproc_t)0 && timeout.tv_sec == 0
	    && timeout.tv_usec == 0) ? FALSE : TRUE;

call_again:
	xdrs->x_op = XDR_ENCODE;
	ct->ct_error.re_status = RPC_SUCCESS;
	--(*msg_x_id);
	x_id = ntohl(*msg_x_id);
	if ((! XDR_PUTBYTES(xdrs, ct->ct_mcall, ct->ct_mpos)) ||
	    (! XDR_PUTLONG(xdrs, (long *)&proc)) ||
	    (! AUTH_MARSHALL(h->cl_auth, xdrs)) ||
	    (! (*xdr_args)(xdrs, args_ptr))) {
		if (ct->ct_error.re_status == RPC_SUCCESS)
			ct->ct_error.re_status = RPC_CANTENCODEARGS;
		(void)xdrrec_endofrecord(xdrs, TRUE);
		return (ct->ct_error.re_status);
	}
	if (! xdrrec_endofrecord(xdrs, shipnow))
		return (ct->ct_error.re_status = RPC_CANTSEND);
	if (! shipnow)
		return (RPC_SUCCESS);

	/*
	 *  Hack to provide rpc-based message passing
	 */
	 if (timeout.tv_sec == 0 && timeout.tv_usec == 0)	/* 4.0 */
	     {							/* 4.0 */
	     return (ct->ct_error.re_status = RPC_TIMEDOUT);	/* 4.0 */
	     }							/* 4.0 */

	xdrs->x_op = XDR_DECODE;

	/*
	 * Keep receiving until we get a valid transaction id
	 */
	while (TRUE) {
		reply_msg.acpted_rply.ar_verf = _null_auth;
		reply_msg.acpted_rply.ar_results.where = NULL;
		reply_msg.acpted_rply.ar_results.proc = xdr_void;
		if (! xdrrec_skiprecord(xdrs))
			return (ct->ct_error.re_status);
		/* now decode and validate the response header */
		if (! xdr_replymsg(xdrs, &reply_msg)) {
			if (ct->ct_error.re_status == RPC_SUCCESS)
				continue;
			return (ct->ct_error.re_status);
		}
		if (reply_msg.rm_xid == x_id)
			break;
	}

	/*
	 * process header
	 */
	_seterr_reply(&reply_msg, &(ct->ct_error));
	if (ct->ct_error.re_status == RPC_SUCCESS) {
		if (! AUTH_VALIDATE(h->cl_auth, &reply_msg.acpted_rply.ar_verf)) {
			ct->ct_error.re_status = RPC_AUTHERROR;
			ct->ct_error.re_why = AUTH_INVALIDRESP;
		} else if (! (*xdr_results)(xdrs, results_ptr)) {
			if (ct->ct_error.re_status == RPC_SUCCESS)
				ct->ct_error.re_status = RPC_CANTDECODERES;
		}
		/* free verifier ... */
		if (reply_msg.acpted_rply.ar_verf.oa_base != NULL) {
			xdrs->x_op = XDR_FREE;
			(void)xdr_opaque_auth(xdrs, &(reply_msg.acpted_rply.ar_verf));
		}
	}  /* end successful completion */
	else {
		/* maybe our credentials need to be refreshed ... */
		if (refreshes-- && AUTH_REFRESH(h->cl_auth))
			goto call_again;
	}  /* end of unsuccessful completion */
	return (ct->ct_error.re_status);
}

LOCAL void				/* 4.0 */
clnttcp_geterr(h, errp)
	CLIENT *h;
	struct rpc_err *errp;
{
	register struct ct_data *ct =
	    (struct ct_data *) h->cl_private;

	*errp = ct->ct_error;
}

LOCAL bool_t				/* 4.0 */
clnttcp_freeres(cl, xdr_res, res_ptr)
	CLIENT *cl;
	xdrproc_t xdr_res;
	caddr_t res_ptr;
{
	register struct ct_data *ct = (struct ct_data *)cl->cl_private;
	register XDR *xdrs = &(ct->ct_xdrs);

	xdrs->x_op = XDR_FREE;
	return ((*xdr_res)(xdrs, res_ptr));
}

LOCAL void				/* 4.0 */
clnttcp_abort()
{
}

LOCAL bool_t							/* 4.0 */
clnttcp_control (cl, request, info)				/* 4.0 */
    CLIENT *cl;							/* 4.0 */
    int request;						/* 4.0 */
    char *info;							/* 4.0 */
    {								/* 4.0 */
    register struct ct_data *ct = (struct ct_data *)cl->cl_private;  /* 4.0 */

    switch (request)						/* 4.0 */
	{							/* 4.0 */
	case CLSET_TIMEOUT:					/* 4.0 */
	    ct->ct_wait = *(struct timeval *) info;		/* 4.0 */
	    ct->ct_waitset = TRUE;				/* 4.0 */
	    break;						/* 4.0 */

	case CLGET_TIMEOUT:					/* 4.0 */
	    *(struct timeval *) info = ct->ct_wait;		/* 4.0 */
	    break;						/* 4.0 */

	case CLGET_SERVER_ADDR:					/* 4.0 */
	    *(struct sockaddr_in *) info = ct->ct_addr;		/* 4.0 */
	    break;						/* 4.0 */

	default:						/* 4.0 */
	    return (FALSE);					/* 4.0 */
	}							/* 4.0 */

    return (TRUE);						/* 4.0 */
    }								/* 4.0 */

LOCAL void				/* 4.0 */
clnttcp_destroy(h)
	CLIENT *h;
{
	register struct ct_data *ct =
	    (struct ct_data *) h->cl_private;

	if (ct->ct_closeit)			/* new in 4.0 */
	    (void) close (ct->ct_sock);

	XDR_DESTROY(&(ct->ct_xdrs));
	mem_free((caddr_t)ct, sizeof(struct ct_data));
	mem_free((caddr_t)h, sizeof(CLIENT));
}

/*
 * Interface between xdr serializer and tcp connection.
 * Behaves like the system calls, read & write, but keeps some error state
 * around for the rpc level.
 */
LOCAL int				/* 4.0 */
clnt_readtcp(ct, buf, len)
	register struct ct_data *ct;
	caddr_t buf;
	register int len;
{
	fd_set mask;				/* 4.0 */
	fd_set readFds;

	if (len == 0)
	    return (0);

	FD_ZERO (&mask);			/* 4.0 */
	FD_SET (ct->ct_sock, &mask);		/* 4.0 */

	while (TRUE) {
		readFds = mask;					    /* 4.0 */
		switch (select (FD_SETSIZE, &readFds, (fd_set *)NULL,  /* 4.0 */
				(fd_set *)NULL, &(ct->ct_wait))) {     /* 4.0 */

		case 0:
			ct->ct_error.re_status = RPC_TIMEDOUT;
			return (-1);

		case -1:
			if (rpcErrnoGet () == EINTR)
				continue;
			ct->ct_error.re_status = RPC_CANTRECV;
			ct->ct_error.re_errno = rpcErrnoGet ();
			return (-1);
		}
/* XXX 4.0	if (FD_ISSET(fd, &readFds))			 */
		break;
	}

	switch (len = read(ct->ct_sock, buf, len)) {

	case 0:
		/* premature eof */
		ct->ct_error.re_errno = ECONNRESET;
		ct->ct_error.re_status = RPC_CANTRECV;
		len = -1;  /* it's really an error */
		break;

	case -1:
		ct->ct_error.re_errno = rpcErrnoGet ();
		ct->ct_error.re_status = RPC_CANTRECV;
		break;
	}
	return (len);
}

LOCAL int				/* 4.0 */
clnt_writetcp(ct, buf, len)
	struct ct_data *ct;
	caddr_t buf;
	int len;
{
	register int i, cnt;

	for (cnt = len; cnt > 0; cnt -= i, buf += i) {
		if ((i = write(ct->ct_sock, buf, cnt)) == -1) {
			ct->ct_error.re_errno = rpcErrnoGet ();
			ct->ct_error.re_status = RPC_CANTSEND;
			return (-1);
		}
	}
	return (len);
}
