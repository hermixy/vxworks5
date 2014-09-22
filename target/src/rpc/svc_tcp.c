/* svc_tcp.c - server side for TCP/IP based RPC */

/* Copyright 1984-2002 Wind River Systems, Inc. */
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
01o,11jan02,vvv  fixed infinite loop in readtcp when select times out 
		 (SPR #67857)
01n,05nov01,vvv  fixed compilation warnings
01m,18apr00,ham  fixed compilation warnings.
01l,11aug93,jmm  Changed ioctl.h and socket.h to sys/ioctl.h and sys/socket.h
01k,26may92,rrr  the tree shuffle
		  -changed includes to have absolute path from h/
01j,04oct91,rrr  passed through the ansification filter
		  -changed includes to have absolute path from h/
		  -changed VOID to void
		  -changed copyright notice
01i,25oct90,dnw   removed include of utime.h.
01h,07oct90,hjb   deleted declaration of mem_alloc() due to problems with
		  ISI cpp complaining about macro argument.  Added include
		  of "memLib.h" instead.
01g,19jul90,dnw   changed declaration of mem_alloc() from char* to void*
01f,10may90,dnw   changed to use rpcErrnoGet instead of errnoGet
01e,19apr90,hjb   de-linted.
01d,27oct89,hjb   upgraded to 4.0
01d,30may88,dnw   changed to v4 names.
01c,05apr88,gae   updated select() to BSD4.3, i.e. used "fd_set".
		  changed fprintf() to printErr().
01b,11nov87,jlf   added wrs copyright, title, mod history, etc.
01a,01nov87,rdc   first VxWorks version
*/

#ifndef lint
/* static char sccsid[] = "@(#)svc_tcp.c 1.1 86/02/03 Copyr 1984 Sun Micro"; */
#endif

/*
 * svc_tcp.c, Server side for TCP/IP based RPC.
 *
 * Actually implements two flavors of transporter -
 * a tcp rendezvouser (a listner and connection establisher)
 * and a record/tcp stream.
 */

#include "rpc/rpctypes.h"
#include "netinet/in.h"
#include "sys/socket.h"
#include "errno.h"
#include "errnoLib.h"
#include "rpc/xdr.h"
#include "rpc/auth.h"
#include "rpc/clnt.h"
#include "rpc/rpc_msg.h"
#include "rpc/svc.h"
#include "vxWorks.h"
#include "memLib.h"
#include "stdio.h"
#include "sockLib.h"
#include "ioLib.h"
#include "rpcLib.h"
#include "remLib.h"

extern bool_t abort();

/*
 * Including stdlib.h causes conflict with abort() declaration, so adding
 * externs for malloc and free
 */

extern void *malloc ();
extern void free ();


/*
 * Ops vector for TCP/IP based rpc service handle
 */
LOCAL bool_t		svctcp_recv();
LOCAL enum xprt_stat	svctcp_stat();
LOCAL bool_t		svctcp_getargs();
LOCAL bool_t		svctcp_reply();
LOCAL bool_t		svctcp_freeargs();
LOCAL void		svctcp_destroy();

static struct xp_ops svctcp_op = {
	svctcp_recv,
	svctcp_stat,
	svctcp_getargs,
	svctcp_reply,
	svctcp_freeargs,
	svctcp_destroy
};

/*
 * Ops vector for TCP/IP rendezvous handler
 */
LOCAL bool_t		rendezvous_request();
LOCAL enum xprt_stat	rendezvous_stat();

static struct xp_ops svctcp_rendezvous_op = {
	rendezvous_request,
	rendezvous_stat,
	abort,
	abort,
	abort,
	svctcp_destroy
};

LOCAL int	readtcp();
LOCAL int	writetcp();
LOCAL SVCXPRT * makefd_xprt();

struct tcp_rendezvous { /* kept in xprt->xp_p1 */
	u_int sendsize;
	u_int recvsize;
};

struct tcp_conn {  /* kept in xprt->xp_p1 */
	enum xprt_stat strm_stat;
	u_long x_id;
	XDR xdrs;
	char verf_body[MAX_AUTH_BYTES];
};

/*
 * Usage:
 *	xprt = svctcp_create(sock, send_buf_size, recv_buf_size);
 *
 * Creates, registers, and returns a (rpc) tcp based transporter.
 * Once *xprt is initialized, it is registered as a transporter
 * see (svc.h, xprt_register).  This routine returns
 * a NULL if a problem occurred.
 *
 * If sock<0 then a socket is created, else sock is used.
 * If the socket, sock is not bound to a port then svctcp_create
 * binds it to an arbitrary port.  The routine then starts a tcp
 * listener on the socket's associated port.  In any (successful) case,
 * xprt->xp_sock is the registered socket number and xprt->xp_port is the
 * associated port number.
 *
 * Since tcp streams do buffered io similar to stdio, the caller can specify
 * how big the send and receive buffers are via the second and third parms;
 * 0 => use the system default.
 */
SVCXPRT *
svctcp_create(sock, sendsize, recvsize)
	register int sock;
	u_int sendsize;
	u_int recvsize;
{
	bool_t madesock = FALSE;
	register SVCXPRT *xprt;
	register struct tcp_rendezvous *r;
	struct sockaddr_in addr;
	int len = sizeof(struct sockaddr_in);

	if (sock == RPC_ANYSOCK) {
		if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
			perror("svctcp_.c - udp socket creation problem");
			return ((SVCXPRT *)NULL);
		}
		madesock = TRUE;
	}

	bzero ((char *) &addr, sizeof (addr));			/* 4.0 */
	addr.sin_family = AF_INET;
	if (bindresvport (sock, &addr)) 			/* 4.0 */
	    {							/* 4.0 */
	    addr.sin_port = 0;					/* 4.0 */
	    (void) bind (sock, (struct sockaddr *)&addr, len);	/* 4.0 */
	    }							/* 4.0 */

	if ((getsockname(sock, (struct sockaddr *)&addr, &len) != 0)  ||
	    (listen(sock, 2) != 0)) {
		perror("svctcp_.c - cannot getsockname or listen");
		if (madesock)
		       (void)close(sock);
		return ((SVCXPRT *)NULL);
	}
	r = (struct tcp_rendezvous *)mem_alloc(sizeof(*r));
	if (r == NULL) {
		printErr ("svctcp_create: out of memory\n");
		return (NULL);
	}
	r->sendsize = sendsize;
	r->recvsize = recvsize;
	xprt = (SVCXPRT *)mem_alloc(sizeof(SVCXPRT));
	if (xprt == NULL) {
		printErr ("svctcp_create: out of memory\n");
		return (NULL);
	}
	xprt->xp_p2 = NULL;
	xprt->xp_p1 = (caddr_t)r;
	xprt->xp_verf = _null_auth;
	xprt->xp_ops = &svctcp_rendezvous_op;
	xprt->xp_port = ntohs(addr.sin_port);
	xprt->xp_sock = sock;
	xprt_register(xprt);
	return (xprt);
}

/*
 * Like svtcp_create(), except the routine takes any *open* UNIX file
 * descriptor as its first input.
 */
SVCXPRT *
svcfd_create(fd, sendsize, recvsize)
	int fd;
	u_int sendsize;
	u_int recvsize;
{

	return (makefd_xprt(fd, sendsize, recvsize));
}

LOCAL SVCXPRT *						/* 4.0 */
makefd_xprt(fd, sendsize, recvsize)
	int fd;
	u_int sendsize;
	u_int recvsize;
{
	register SVCXPRT *xprt;
	register struct tcp_conn *cd;

	xprt = (SVCXPRT *)mem_alloc(sizeof(SVCXPRT));
	if (xprt == (SVCXPRT *)NULL) {
		printErr ("svc_tcp: makefd_xprt: out of memory\n");
		goto done;
	}
	cd = (struct tcp_conn *)mem_alloc(sizeof(struct tcp_conn));
	if (cd == (struct tcp_conn *)NULL) {
		printErr ("svc_tcp: makefd_xprt: out of memory\n");
		mem_free((char *) xprt, sizeof(SVCXPRT));
		xprt = (SVCXPRT *)NULL;
		goto done;
	}
	cd->strm_stat = XPRT_IDLE;
	xdrrec_create(&(cd->xdrs), sendsize, recvsize,
	    (caddr_t)xprt, readtcp, writetcp);
	xprt->xp_p2 = NULL;
	xprt->xp_p1 = (caddr_t)cd;
	xprt->xp_verf.oa_base = cd->verf_body;
	xprt->xp_addrlen = 0;
	xprt->xp_ops = &svctcp_op;  /* truely deals with calls */
	xprt->xp_port = 0;  /* this is a connection, not a rendezvouser */
	xprt->xp_sock = fd;
	xprt_register(xprt);
    done:
	return (xprt);
}

LOCAL bool_t						/* 4.0 */
rendezvous_request(xprt)
	register SVCXPRT *xprt;
{
	int sock;
	struct tcp_rendezvous *r;
	struct sockaddr_in addr;
	int len;

	r = (struct tcp_rendezvous *)xprt->xp_p1;
    again:
	len = sizeof(struct sockaddr_in);
	if ((sock = accept(xprt->xp_sock, (struct sockaddr *)&addr,
	    &len)) < 0) {
		if (rpcErrnoGet () == EINTR)
			goto again;
	       return (FALSE);
	}
	/*
	 * make a new transporter (re-uses xprt)
	 */
	xprt = makefd_xprt(sock, r->sendsize, r->recvsize);
	xprt->xp_raddr = addr;
	xprt->xp_addrlen = len;
	return (FALSE); /* there is never an rpc msg to be processed */
}

LOCAL enum xprt_stat					/* 4.0 */
rendezvous_stat()
{

	return (XPRT_IDLE);
}

LOCAL void						/* 4.0 */
svctcp_destroy(xprt)
	register SVCXPRT *xprt;
{
	register struct tcp_conn *cd = (struct tcp_conn *)xprt->xp_p1;

	xprt_unregister(xprt);
	(void)close(xprt->xp_sock);
	if (xprt->xp_port != 0) {
		/* a rendezvouser socket */
		xprt->xp_port = 0;
	} else {
		/* an actual connection socket */
		XDR_DESTROY(&(cd->xdrs));
	}
	mem_free((caddr_t)cd, sizeof(struct tcp_conn));
	mem_free((caddr_t)xprt, sizeof(SVCXPRT));
}

/*
 * All read operations timeout after 35 seconds.
 * A timeout is fatal for the connection.
 */
static struct timeval wait_per_try = { 35, 0 };

/*
 * reads data from the tcp conection.
 * any error is fatal and the connection is closed.
 * (And a read of zero bytes is a half closed stream => error.)
 */
LOCAL int						/* 4.0 */
readtcp(xprt, buf, len)
	register SVCXPRT *xprt;
	caddr_t buf;
	register int len;
{
	register int sock = xprt->xp_sock;
	fd_set mask;					/* 4.0 */
	fd_set readFds;

	FD_ZERO (&mask);				/* 4.0 */
	FD_SET (sock, &mask);				/* 4.0 */

	do {
		readFds = mask;				/* 4.0 */
		if (select (FD_SETSIZE, &readFds, (fd_set *)NULL,
			    (fd_set *)NULL, &wait_per_try) <= 0) {
			if (errnoGet () == EINTR)
			    continue;
			goto fatal_err;
		}
	} while (!FD_ISSET (sock, &readFds));

	if ((len = read(sock, buf, len)) > 0)
		return (len);
fatal_err:
	((struct tcp_conn *)(xprt->xp_p1))->strm_stat = XPRT_DIED;
	return (-1);
}

/*
 * writes data to the tcp connection.
 * Any error is fatal and the connection is closed.
 */
LOCAL int						/* 4.0 */
writetcp(xprt, buf, len)
	register SVCXPRT *xprt;
	caddr_t buf;
	int len;
{
	register int i, cnt;

	for (cnt = len; cnt > 0; cnt -= i, buf += i) {
		if ((i = write(xprt->xp_sock, buf, cnt)) < 0) {
			((struct tcp_conn *)(xprt->xp_p1))->strm_stat =
			    XPRT_DIED;
			return (-1);
		}
	}
	return (len);
}

LOCAL enum xprt_stat						/* 4.0 */
svctcp_stat(xprt)
	SVCXPRT *xprt;
{
	register struct tcp_conn *cd =
	    (struct tcp_conn *)(xprt->xp_p1);

	if (cd->strm_stat == XPRT_DIED)
		return (XPRT_DIED);
	if (! xdrrec_eof(&(cd->xdrs)))
		return (XPRT_MOREREQS);
	return (XPRT_IDLE);
}

LOCAL bool_t					/* 4.0 */
svctcp_recv(xprt, msg)
	SVCXPRT *xprt;
	register struct rpc_msg *msg;
{
	register struct tcp_conn *cd =
	    (struct tcp_conn *)(xprt->xp_p1);
	register XDR *xdrs = &(cd->xdrs);

	xdrs->x_op = XDR_DECODE;
	(void)xdrrec_skiprecord(xdrs);
	if (xdr_callmsg(xdrs, msg)) {
		cd->x_id = msg->rm_xid;
		return (TRUE);
	}
	return (FALSE);
}

LOCAL bool_t					/* 4.0 */
svctcp_getargs(xprt, xdr_args, args_ptr)
	SVCXPRT *xprt;
	xdrproc_t xdr_args;
	caddr_t args_ptr;
{

	return ((*xdr_args)(&(((struct tcp_conn *)(xprt->xp_p1))->xdrs), args_ptr));
}

LOCAL bool_t					/* 4.0 */
svctcp_freeargs(xprt, xdr_args, args_ptr)
	SVCXPRT *xprt;
	xdrproc_t xdr_args;
	caddr_t args_ptr;
{
	register XDR *xdrs =
	    &(((struct tcp_conn *)(xprt->xp_p1))->xdrs);

	xdrs->x_op = XDR_FREE;
	return ((*xdr_args)(xdrs, args_ptr));
}

LOCAL bool_t					/* 4.0 */
svctcp_reply(xprt, msg)
	SVCXPRT *xprt;
	register struct rpc_msg *msg;
{
	register struct tcp_conn *cd =
	    (struct tcp_conn *)(xprt->xp_p1);
	register XDR *xdrs = &(cd->xdrs);
	register bool_t stat;

	xdrs->x_op = XDR_ENCODE;
	msg->rm_xid = cd->x_id;
	stat = xdr_replymsg(xdrs, msg);
	(void)xdrrec_endofrecord(xdrs, TRUE);
	return (stat);
}
