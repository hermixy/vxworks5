/* clnt_udp.c - implements a UPD/IP based, client side RPC */

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
01p,18apr00,ham  fixed compilation warnings.
01o,28jul96,sgv  fix for spr #1477, Increased the send window size and
		 receive window size to 8000.
01n,22apr93,caf  ansification: added cast to ioctl() parameter.
01m,11aug93,jmm  Changed ioctl.h and socket.h to sys/ioctl.h and sys/socket.h
01l,26may92,rrr  the tree shuffle
		  -changed includes to have absolute path from h/
01k,04oct91,rrr  passed through the ansification filter
		  -changed includes to have absolute path from h/
		  -changed copyright notice
01j,25oct90,dnw   removed include of utime.h.
01i,10may90,dnw   changed taskModuleList->rpccreateerr to rpccreateerr macro
		  changed to use rpcErrnoGet instead of errnoGet
01h,18mar90,jcf   fixed header dependency.
01g,27oct89,hjb   upgraded to 4.0
01f,22jun88,dnw   name tweaks.
01e,30may88,dnw   changed to v4 names.
01d,05apr88,gae   updated select() to BSD4.3, i.e. used "fd_set".
		  changed fprintf() to printErr().
01c,22feb88,jcf   made kernel independent.
01b,11nov87,jlf   added wrs copyright, title, mod history, etc.
01a,01nov87,rdc   first VxWorks version
*/

#ifndef lint
/* static char sccsid[] = "@(#)clnt_udp.c 1.1 86/02/03 Copyr 1984 Sun Micro"; */
#endif

/*
 * clnt_udp.c, Implements a UPD/IP based, client side RPC.
 *
 */

#include "rpc/rpctypes.h"
#include "sys/socket.h"
#include "sys/ioctl.h"
#include "netinet/in.h"
#include "errno.h"
#include "rpc/xdr.h"
#include "rpc/auth.h"
#include "rpc/clnt.h"
#include "rpc/rpc_msg.h"
#include "rpc/pmap_clnt.h"
#include "vxWorks.h"
#include "rpc/rpcGbl.h"
#include "ioLib.h"
#include "memLib.h"
#include "stdio.h"
#include "rpcLib.h"
#include "tickLib.h"
#include "sockLib.h"
#include "remLib.h"

/*
 * UDP bases client side rpc operations
 */
LOCAL enum clnt_stat	clntudp_call();				/* 4.0 */
LOCAL void		clntudp_abort();			/* 4.0 */
LOCAL void		clntudp_geterr();			/* 4.0 */
LOCAL bool_t		clntudp_control();			/* 4.0 */
LOCAL void		clntudp_destroy();			/* 4.0 */

/*
 * clntudp_freeres () is used by nfsLib.c so it cannot be declared as LOCAL.
 */
bool_t			clntudp_freeres();

static struct clnt_ops udp_ops = {
	clntudp_call,
	clntudp_abort,
	clntudp_geterr,
	clntudp_freeres,
	clntudp_destroy,
	clntudp_control				/* 4.0 */
};

/*
 * Private data kept per client handle
 */
struct cu_data {
	int		   cu_sock;
	bool_t		   cu_closeit;		/* 4.0 */
	struct sockaddr_in cu_raddr;
	int		   cu_rlen;
	struct timeval	   cu_wait;
	struct timeval	   cu_total;		/* 4.0 */
	struct rpc_err	   cu_error;
	XDR		   cu_outxdrs;
	u_int		   cu_xdrpos;
	u_int		   cu_sendsz;
	char		   *cu_outbuf;
	u_int		   cu_recvsz;
	char		   cu_inbuf[1];
};

IMPORT bool_t xdr_opaque_auth ();

/*
 * Create a UDP based client handle.
 * If *sockp<0, *sockp is set to a newly created UPD socket.
 * If raddr->sin_port is 0 a binder on the remote machine
 * is consulted for the correct port number.
 * NB: It is the clients responsibility to close *sockp.
 * NB: The rpch->cl_auth is initialized to null authentication.
 *     Caller may wish to set this something more useful.
 *
 * wait is the amount of time used between retransmitting a call if
 * no response has been heard;  retransmition occurs until the actual
 * rpc call times out.
 *
 * sendsz and recvsz are the maximum allowable packet sizes that can be
 * sent and received.
 */
CLIENT *
clntudp_bufcreate(raddr, program, version, wait, sockp, sendsz, recvsz)
	struct sockaddr_in *raddr;
	u_long program;
	u_long version;
	struct timeval wait;
	register int *sockp;
	u_int sendsz;
	u_int recvsz;
{
	CLIENT *cl = NULL;
	register struct cu_data *cu = NULL;
	/* XXX struct timeval now;*/
	struct rpc_msg call_msg;

	cl = (CLIENT *)mem_alloc(sizeof(CLIENT));
	if (cl == NULL) {
		printErr ("clntudp_create: out of memory\n");
		rpc_createerr.cf_stat = RPC_SYSTEMERROR;
		rpc_createerr.cf_error.re_errno = rpcErrnoGet ();
		goto fooy;
	}
	sendsz = ((sendsz + 3) / 4) * 4;
	recvsz = ((recvsz + 3) / 4) * 4;
	cu = (struct cu_data *)mem_alloc(sizeof(*cu) + sendsz + recvsz);
	if (cu == NULL) {
		printErr ("clntudp_create: out of memory\n");
		rpc_createerr.cf_stat = RPC_SYSTEMERROR;
		rpc_createerr.cf_error.re_errno = rpcErrnoGet ();
		goto fooy;
	}
	cu->cu_outbuf = &cu->cu_inbuf[recvsz];

	/* XXX (void)gettimeofday(&now, (struct timezone *)0); */
	if (raddr->sin_port == 0) {
		u_short port;
		if ((port =
		    pmap_getport(raddr, program, version, (u_long)IPPROTO_UDP)) == 0) {
			goto fooy;
		}
		raddr->sin_port = htons(port);
	}
	cl->cl_ops = &udp_ops;
	cl->cl_private = (caddr_t)cu;
	cu->cu_raddr = *raddr;
	cu->cu_rlen = sizeof (cu->cu_raddr);
	cu->cu_wait = wait;
	cu->cu_total.tv_sec = -1;
	cu->cu_total.tv_usec = -1;
	cu->cu_sendsz = sendsz;
	cu->cu_recvsz = recvsz;
	/* XXX call_msg.rm_xid = getpid() ^ now.tv_sec ^ now.tv_usec; */
	call_msg.rm_xid = taskIdSelf () ^ tickGet ();
	call_msg.rm_direction = CALL;
	call_msg.rm_call.cb_rpcvers = RPC_MSG_VERSION;
	call_msg.rm_call.cb_prog = program;
	call_msg.rm_call.cb_vers = version;
	xdrmem_create(&(cu->cu_outxdrs), cu->cu_outbuf,
	    sendsz, XDR_ENCODE);
	if (! xdr_callhdr(&(cu->cu_outxdrs), &call_msg)) {
		goto fooy;
	}
	cu->cu_xdrpos = XDR_GETPOS(&(cu->cu_outxdrs));
	if (*sockp < 0) {
		int dontblock = 1;

		*sockp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (*sockp < 0) {
			rpc_createerr.cf_stat = RPC_SYSTEMERROR;
			rpc_createerr.cf_error.re_errno = rpcErrnoGet ();
			goto fooy;
		}

		setsockopt (*sockp, SOL_SOCKET, SO_SNDBUF, (char *)&(sendsz),
                            sizeof(long));
    		setsockopt (*sockp, SOL_SOCKET, SO_RCVBUF, (char *)&(recvsz),
                            sizeof(long));


		/* attemp to bind to private port */
		(void) bindresvport (*sockp, (struct sockaddr_in *) 0);

		/* the sockets rpc controls are non-blocking */
		(void)ioctl(*sockp, FIONBIO, (int) &dontblock);
		cu->cu_closeit = TRUE;
	} else {
		cu->cu_closeit = FALSE;
	}

	cu->cu_sock = *sockp;
	cl->cl_auth = authnone_create();
	return (cl);
fooy:
	if (cu)
		mem_free((caddr_t)cu, sizeof(*cu) + sendsz + recvsz);
	if (cl)
		mem_free((caddr_t)cl, sizeof(CLIENT));
	return ((CLIENT *)NULL);
}

CLIENT *
clntudp_create(raddr, program, version, wait, sockp)
	struct sockaddr_in *raddr;
	u_long program;
	u_long version;
	struct timeval wait;
	register int *sockp;
{

	return(clntudp_bufcreate(raddr, program, version, wait, sockp,
	    UDPMSGSIZE, UDPMSGSIZE));
}

LOCAL enum clnt_stat 						/* 4.0 */
clntudp_call(cl, proc, xargs, argsp, xresults, resultsp, utimeout)
	register CLIENT	*cl;		/* client handle */
	u_long		proc;		/* procedure number */
	xdrproc_t	xargs;		/* xdr routine for args */
	caddr_t		argsp;		/* pointer to args */
	xdrproc_t	xresults;	/* xdr routine for results */
	caddr_t		resultsp;	/* pointer to results */
	struct timeval	utimeout;   /* seconds to wait before giving up - 4.0 */
{
	register struct cu_data *cu = (struct cu_data *)cl->cl_private;
	register XDR *xdrs;
	register int outlen;
	register int inlen;
	int fromlen;
	fd_set readFds;
	fd_set mask;				/* 4.0 */
	struct sockaddr_in from;
	struct rpc_msg reply_msg;
	XDR reply_xdrs;
	struct timeval time_waited;
	bool_t ok;
	int nrefreshes = 2;	/* number of times to refresh cred   4.0 */
	struct timeval timeout;

	if (cu->cu_total.tv_usec == -1) {			/* 4.0 */
		timeout = utimeout;	/* use supplied timeout -- 4.0 */
	} else {
		timeout = cu->cu_total;	/* use default timeout  -- 4.0 */
	}

	time_waited.tv_sec = 0;
	time_waited.tv_usec = 0;

call_again:						/* 4.0 */
	xdrs = &(cu->cu_outxdrs);
	xdrs->x_op = XDR_ENCODE;
	XDR_SETPOS(xdrs, cu->cu_xdrpos);

	/*
	 * the transaction is the first thing in the out buffer
	 */
	(*(u_short *)(cu->cu_outbuf))++;
	if ((! XDR_PUTLONG(xdrs, (long *)&proc)) ||
	    (! AUTH_MARSHALL(cl->cl_auth, xdrs)) ||
	    (! (*xargs)(xdrs, argsp)))
	    {
	    return (cu->cu_error.re_status = RPC_CANTENCODEARGS);
	    }

	outlen = (int)XDR_GETPOS(xdrs);

send_again:					/* 4.0 */

	if (sendto(cu->cu_sock, cu->cu_outbuf, outlen, 0,
	    (struct sockaddr *)&(cu->cu_raddr), cu->cu_rlen)
	    != outlen) {
		cu->cu_error.re_errno = rpcErrnoGet ();
		return (cu->cu_error.re_status = RPC_CANTSEND);
	}

	/*
	 * Hack to provide rpc-based message passing
	 */
	if (timeout.tv_sec == 0 && timeout.tv_usec == 0) {	/* 4.0 */
		return (cu->cu_error.re_status = RPC_TIMEDOUT);	/* 4.0 */
	}	/* 4.0 */

	/*
	 * sub-optimal code appears inside the loop because we have
	 * some clock time to spare while the packets are in flight.
	 * (We assume that this is actually only executed once.)
	 */
	reply_msg.acpted_rply.ar_verf = _null_auth;
	reply_msg.acpted_rply.ar_results.where = resultsp;
	reply_msg.acpted_rply.ar_results.proc = xresults;

	FD_ZERO (&mask);			/* 4.0 */
	FD_SET (cu->cu_sock, &mask);		/* 4.0 */

	for (;;) {				/* 4.0 */
		readFds = mask;			/* 4.0 */
		switch (select (FD_SETSIZE, &readFds, (fd_set *)NULL,
				(fd_set *)NULL, &(cu->cu_wait)))
		{

		case 0:
			time_waited.tv_sec += cu->cu_wait.tv_sec;
			time_waited.tv_usec += cu->cu_wait.tv_usec;
			while (time_waited.tv_usec >= 1000000) {
				time_waited.tv_sec++;
				time_waited.tv_usec -= 1000000;
			}
			if ((time_waited.tv_sec < timeout.tv_sec) ||
				((time_waited.tv_sec == timeout.tv_sec) &&
				(time_waited.tv_usec < timeout.tv_usec)))
				goto send_again;
			return (cu->cu_error.re_status = RPC_TIMEDOUT);

		/*
		 * buggy in other cases because time_waited is not being
		 * updated
		 */
		case -1:
			if (rpcErrnoGet () == EINTR)
				continue;		/* 4.0 */
			cu->cu_error.re_errno = rpcErrnoGet ();
			return (cu->cu_error.re_status = RPC_CANTRECV);
		}

		do {
			fromlen = sizeof (struct sockaddr);
			inlen = recvfrom (cu->cu_sock, cu->cu_inbuf,
					  (int) cu->cu_recvsz, 0,
					  (struct sockaddr *) &from, &fromlen);
		} while (inlen < 0 && (rpcErrnoGet () == EINTR));

		if (inlen < 0) {
			if (rpcErrnoGet () == EWOULDBLOCK)
				continue;			/* 4.0 */
			cu->cu_error.re_errno = rpcErrnoGet ();
			return (cu->cu_error.re_status = RPC_CANTRECV);
		}
		if (inlen < sizeof(u_long))
			continue;
		/* see if reply transaction id matches sent id */
		if (*((u_long *)(cu->cu_inbuf)) != *((u_long *)(cu->cu_outbuf)))
			continue;			/* 4.0 */
		/* we now assume we have the proper reply */
		break;
	}

	/*
	 * now decode and validate the response
	 */
	xdrmem_create(&reply_xdrs, cu->cu_inbuf, (u_int)inlen, XDR_DECODE);
	ok = xdr_replymsg(&reply_xdrs, &reply_msg);
	/* XDR_DESTROY(&reply_xdrs);  save a few cycles on noop destroy */
	if (ok) {
		_seterr_reply(&reply_msg, &(cu->cu_error));
		if (cu->cu_error.re_status == RPC_SUCCESS) {
			if (! AUTH_VALIDATE(cl->cl_auth,
				&reply_msg.acpted_rply.ar_verf)) {
				cu->cu_error.re_status = RPC_AUTHERROR;
				cu->cu_error.re_why = AUTH_INVALIDRESP;
			}
			if (reply_msg.acpted_rply.ar_verf.oa_base != NULL) {
				xdrs->x_op = XDR_FREE;
				(void)xdr_opaque_auth(xdrs,
				    &(reply_msg.acpted_rply.ar_verf));
			}
		}  /* end successful completion */
		else {
			/* maybe our credentials need to be refreshed ... */
			if (nrefreshes > 0 && 			/* 4.0 */
			    AUTH_REFRESH(cl->cl_auth))
				goto call_again;
		}  /* end of unsuccessful completion */
	}  /* end of valid reply message */
	else {
		cu->cu_error.re_status = RPC_CANTDECODERES;
	}
	return (cu->cu_error.re_status);
}

LOCAL void
clntudp_geterr(cl, errp)
	CLIENT *cl;
	struct rpc_err *errp;
{
	register struct cu_data *cu = (struct cu_data *)cl->cl_private;

	*errp = cu->cu_error;
}


bool_t							/* 4.0 */
clntudp_freeres(cl, xdr_res, res_ptr)
	CLIENT *cl;
	xdrproc_t xdr_res;
	caddr_t res_ptr;
{
	register struct cu_data *cu = (struct cu_data *)cl->cl_private;
	register XDR *xdrs = &(cu->cu_outxdrs);

	xdrs->x_op = XDR_FREE;
	return ((*xdr_res)(xdrs, res_ptr));
}

LOCAL void 						/* 4.0 */
clntudp_abort(/*h*/)
	/*CLIENT *h;*/
{
}

LOCAL bool_t						/* 4.0 */
clntudp_control (cl, request, info)
    CLIENT *cl;
    int request;
    char *info;
    {
    register struct cu_data *cu = (struct cu_data *) cl->cl_private;

    switch (request)
	{
	case CLSET_TIMEOUT:
		cu->cu_total = *(struct timeval *) info;
		break;
	case CLGET_TIMEOUT:
		*(struct timeval *) info = cu->cu_total;
		break;
	case CLSET_RETRY_TIMEOUT:
		cu->cu_wait = *(struct timeval *)info;
		break;
	case CLGET_RETRY_TIMEOUT:
		*(struct timeval *) info = cu->cu_wait;
		break;
	case CLGET_SERVER_ADDR:
		*(struct sockaddr_in *) info = cu->cu_raddr;
		break;
	default:
		return (FALSE);
	}
    return (TRUE);
    }

LOCAL void						/* 4.0 */
clntudp_destroy(cl)
	CLIENT *cl;
{
	register struct cu_data *cu = (struct cu_data *)cl->cl_private;

	if (cu->cu_closeit) {				/* 4.0 */
		(void) close (cu->cu_sock);		/* 4.0 */
	}						/* 4.0 */

	XDR_DESTROY(&(cu->cu_outxdrs));
	mem_free((caddr_t)cu, (sizeof(*cu) + cu->cu_sendsz + cu->cu_recvsz));
	mem_free((caddr_t)cl, sizeof(CLIENT));
}
