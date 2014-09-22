/* pmap_rmt.c - client interface to pmap rpc service */

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
01r,15oct01,rae  merge from truestack ver 01m, base 01h(p) (SPR #70325, etc.)
01q,23oct00,cn   replaced inet_makeaddr with inet_makeaddr_b (SPR #8797).
01h,20feb97,jank removed comment in comment L:239
01o,11aug93,jmm  Changed ioctl.h and socket.h to sys/ioctl.h and sys/socket.h
01n,26may92,rrr  the tree shuffle
		  -changed includes to have absolute path from h/
01m,04oct91,rrr  passed through the ansification filter
		  -changed includes to have absolute path from h/
		  -fixed #else and #endif
		  -changed copyright notice
01l,25oct90,dnw   removed include of utime.h.
01k,10may90,dnw   changed to use rpcErrnoGet instead of errnoGet
01j,11may90,yao   added missing modification history (01i) for the last checkin.
01i,09may90,yao   typecasted malloc to (char *).
01h,19apr90,hjb   de-linted.
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
/* static char sccsid[] = "@(#)pmap_rmt.c 1.1 86/02/03 Copyr 1984 Sun Micro"; */
#endif

/*
 * pmap_rmt.c
 * Client interface to pmap rpc service.
 * remote call and broadcast service
 *
 */

#include "vxWorks.h"
#include "stdio.h"
#include "rpc/rpctypes.h"
#include "netinet/in.h"
#include "rpc/xdr.h"
#include "rpc/auth.h"
#include "rpc/clnt.h"
#include "rpc/rpc_msg.h"
#include "rpc/pmap_prot.h"
#include "rpc/pmap_clnt.h"
#include "rpc/pmap_rmt.h"					/* 4.0 */
#include "sys/socket.h"
#include "sockLib.h"
#include "taskLib.h"
#include "errno.h"
#include "net/if.h"
#include "sys/ioctl.h"
#include "net/inet.h"
#include "memLib.h"
#include "ioLib.h"
#include "tickLib.h"
#include "rpcLib.h"
#include "inetLib.h"
#include "memPartLib.h"

#define MAX_BROADCAST_SIZE 1400
#define SOCKSIZ		40

static struct timeval timeout = { 3, 0 };

/* struct rmtcallargs and struct rmtcallres are defined in pmap_rmt.h - 4.0 */

/*
 * pmapper remote-call-service interface.
 * This routine is used to call the pmapper remote call service
 * which will look up a service program in the port maps, and then
 * remotely call that routine with the given parameters.  This allows
 * programs to do a lookup and call in one step.
 */
enum clnt_stat
pmap_rmtcall(addr, prog, vers, proc, xdrargs, argsp, xdrres, resp, tout, port_ptr)
	struct sockaddr_in *addr;
	u_long prog, vers, proc;
	xdrproc_t xdrargs, xdrres;
	caddr_t argsp, resp;
	struct timeval tout;
	u_long *port_ptr;
{
	int socket = -1;
	register CLIENT *client;
	struct rmtcallargs a;
	struct rmtcallres r;
	enum clnt_stat stat;

	addr->sin_port = htons((u_short) PMAPPORT);
	client = clntudp_create(addr, PMAPPROG, PMAPVERS, timeout, &socket);
	if (client != (CLIENT *)NULL) {
		a.prog = prog;
		a.vers = vers;
		a.proc = proc;
		a.args_ptr = argsp;
		a.xdr_args = xdrargs;
		r.port_ptr = port_ptr;
		r.results_ptr = resp;
		r.xdr_results = xdrres;
		stat = CLNT_CALL(client, PMAPPROC_CALLIT, xdr_rmtcall_args, &a,
		    xdr_rmtcallres, &r, tout);
		CLNT_DESTROY(client);
	} else {
		stat = RPC_FAILED;
	}
	(void)close(socket);
	addr->sin_port = 0;
	return (stat);
}

/*
 * XDR remote call arguments
 * written for XDR_ENCODE direction only
 */
bool_t
xdr_rmtcall_args(xdrs, cap)
	register XDR *xdrs;
	register struct rmtcallargs *cap;
{
	u_int lenposition, argposition, position;

	if (xdr_u_long(xdrs, &(cap->prog)) &&
	    xdr_u_long(xdrs, &(cap->vers)) &&
	    xdr_u_long(xdrs, &(cap->proc))) {
		lenposition = XDR_GETPOS(xdrs);
		if (! xdr_u_long(xdrs, &(cap->arglen)))
		    return (FALSE);
		argposition = XDR_GETPOS(xdrs);
		if (! (*(cap->xdr_args))(xdrs, cap->args_ptr))
		    return (FALSE);
		position = XDR_GETPOS(xdrs);
		cap->arglen = (u_long)position - (u_long)argposition;
		XDR_SETPOS(xdrs, lenposition);
		if (! xdr_u_long(xdrs, &(cap->arglen)))
		    return (FALSE);
		XDR_SETPOS(xdrs, position);
		return (TRUE);
	}
	return (FALSE);
}

/*
 * XDR remote call results
 * written for XDR_DECODE direction only
 */
bool_t
xdr_rmtcallres(xdrs, crp)
	register XDR *xdrs;
	register struct rmtcallres *crp;
{
	caddr_t port_ptr;					/* 4.0 */

	port_ptr = (caddr_t) crp->port_ptr;			/* 4.0 */
	if (xdr_reference(xdrs, &port_ptr, sizeof (u_long), 	/* 4.0 */
		xdr_u_long) && xdr_u_long(xdrs, &crp->resultslen)) { /* 4.0 */
		crp->port_ptr = (u_long *) port_ptr;		/* 4.0 */
		return ((*(crp->xdr_results))(xdrs, crp->results_ptr));
	}
	return (FALSE);
}

/*
 * The following is kludged-up support for simple rpc broadcasts.
 * Someday a large, complicated system will replace these trivial
 * routines which only support udp/ip .
 */

LOCAL int							/* 4.0 */
getbroadcastnets(addrs, sock, buf)
	struct in_addr *addrs;
	int sock;  /* any valid socket will do */
	char *buf;  /* why allocxate more when we can use existing... */
{
	char ifreqBuf [SOCKSIZ]; 
	struct ifconf ifc;
        struct ifreq  *ifr;
	struct sockaddr_in *sin;
        int i, len;

	ifc.ifc_len = UDPMSGSIZE;  /* changed from MAX_BROADCAST_SIZE - 4.0 */
        ifc.ifc_buf = buf;
        if (ioctl(sock, SIOCGIFCONF, (int)&ifc) < 0) 
	    {
	    perror("broadcast: ioctl (get interface configuration)");
	    return (0);
	    }
        ifr = ifc.ifc_req;
        for (i = 0, len = ifc.ifc_len; len; len -= (sizeof(ifr->ifr_name) + 
						    ifr->ifr_addr.sa_len))
	    {
	    bcopy ((caddr_t)ifr, ifreqBuf, (sizeof(ifr->ifr_name) + 
					    ifr->ifr_addr.sa_len)); 
	    if (ioctl(sock, SIOCGIFFLAGS, (int)ifreqBuf) < 0) 
		{
		perror("broadcast: ioctl (get interface flags)");
		continue;
		}
	    if ((((struct ifreq *)ifreqBuf)->ifr_flags & IFF_BROADCAST) &&
		(((struct ifreq *)ifreqBuf)->ifr_flags & IFF_UP) &&
		ifr->ifr_addr.sa_family == AF_INET) 
		{
		sin = (struct sockaddr_in *)&ifr->ifr_addr;
		
#ifdef SIOCGIFBRDADDR	/* 4.3 BSD */			/* 4.0 */
		
/* 4.0 */		if (ioctl (sock, SIOCGIFBRDADDR, (int)ifreqBuf) < 0) {
/* 4.0 */			inet_makeaddr_b (inet_netof
/* 4.0 */					(sin->sin_addr),
/* 4.0 */					(int) INADDR_ANY,
						&addrs [i]);
				i++;
/* 4.0 */		} else {
/* 4.0 */			addrs [ i++] =
				    ((struct sockaddr_in *)
				     &((struct ifreq *)ifreqBuf)->ifr_addr)
				    ->sin_addr;
/* 4.0 */		}

#else	/* 4.2 BSD */ 				/* 4.0 */

inet_makeaddr_b (inet_netof (sin->sin_addr.s_addr), 
		 INADDR_ANY,
		 &addrs [i]);
i++;

#endif	/* SIOCGIFBRDADDR */

		}
	    ifr = (struct ifreq *)((char *)ifr + (sizeof(ifr->ifr_name) + 
						  ifr->ifr_addr.sa_len));
	    }
	return (i);
}

typedef bool_t (*resultproc_t)();

enum clnt_stat
clnt_broadcast(prog, vers, proc, xargs, argsp, xresults, resultsp, eachresult)
	u_long		prog;		/* program number */
	u_long		vers;		/* version number */
	u_long		proc;		/* procedure number */
	xdrproc_t	xargs;		/* xdr routine for args */
	caddr_t		argsp;		/* pointer to args */
	xdrproc_t	xresults;	/* xdr routine for results */
	caddr_t		resultsp;	/* pointer to results */
	resultproc_t	eachresult;	/* call with each result obtained */
{
	enum clnt_stat stat;
	AUTH *unix_auth = authunix_create_default();
	XDR xdr_stream;
	register XDR *xdrs = &xdr_stream;
	int outlen, inlen, fromlen, nets;
	fd_set mask;					/* 4.0 */
	fd_set readFds;					/* 4.0 */
	int on = 1;					/* 4.0 */
/* XXX	int fd;						   4.0 */
	register int sock = 0, i;
	bool_t done = FALSE;
	register u_long xid;
	u_long port;
	struct in_addr addrs[20];
	struct sockaddr_in baddr, raddr; /* broadcast and response addresses */
	struct rmtcallargs a;
	struct rmtcallres r;
	struct rpc_msg msg;
	struct timeval t;
	/* XXX char outbuf[MAX_BROADCAST_SIZE], inbuf[MAX_BROADCAST_SIZE]; */
	/*
	 * XXX Hmmmm... is this done to save local stack space?
	 * Interesting...
	 */
	char *outbuf = (char *) KHEAP_ALLOC(MAX_BROADCAST_SIZE);
	char *inbuf =  (char *) KHEAP_ALLOC(MAX_BROADCAST_SIZE);

	if ((outbuf == NULL) || (inbuf == NULL))
	    {
	    panic ("rpc:clnt_broadcast: out of memory!\n");
	    stat = RPC_CANTSEND;
	    goto done_broad;
	    }

        if (unix_auth == NULL)
	    {
	    stat = RPC_CANTSEND;
	    goto done_broad;
	    }

	/*
	 * initialization: create a socket, a broadcast address, and
	 * preserialize the arguments into a send buffer.
	 */
	if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		perror("Cannot create socket for broadcast rpc");
		stat = RPC_CANTSEND;
		goto done_broad;
	}

#ifdef SO_BROADCAST				/* 4.0 */
/* 4.0*/ if (setsockopt (sock, SOL_SOCKET, SO_BROADCAST, (char *) &on,
/* 4.0 */	sizeof (on)) < 0)
/* 4.0 */    {
/* 4.0 */    perror ("Cannot set socket option SO_BROADCAST");
/* 4.0 */    stat = RPC_CANTSEND;
/* 4.0 */    goto done_broad;
/* 4.0 */    }
#endif	/* SO_BROADCAST */					/* 4.0 */

	FD_ZERO (&mask);					/* 4.0 */
	FD_SET (sock, &mask);					/* 4.0 */

	nets = getbroadcastnets(addrs, sock, inbuf);
	bzero((char *) &baddr, sizeof (baddr));
	baddr.sin_family = AF_INET;
	baddr.sin_port = htons((u_short) PMAPPORT);
	baddr.sin_addr.s_addr = htonl(INADDR_ANY);
	/* XXX (void)gettimeofday(&t, (struct timezone *)0); */
	/* msg.rm_xid = xid = getpid() ^ t.tv_sec ^ t.tv_usec; */
	msg.rm_xid = xid = taskIdSelf () ^ tickGet ();
	t.tv_usec = 0;
	msg.rm_direction = CALL;
	msg.rm_call.cb_rpcvers = RPC_MSG_VERSION;
	msg.rm_call.cb_prog = PMAPPROG;
	msg.rm_call.cb_vers = PMAPVERS;
	msg.rm_call.cb_proc = PMAPPROC_CALLIT;
	msg.rm_call.cb_cred = unix_auth->ah_cred;
	msg.rm_call.cb_verf = unix_auth->ah_verf;
	a.prog = prog;
	a.vers = vers;
	a.proc = proc;
	a.xdr_args = xargs;
	a.args_ptr = argsp;
	r.port_ptr = &port;
	r.xdr_results = xresults;
	r.results_ptr = resultsp;
	xdrmem_create(xdrs, outbuf, MAX_BROADCAST_SIZE, XDR_ENCODE);
	if ((! xdr_callmsg(xdrs, &msg)) || (! xdr_rmtcall_args(xdrs, &a))) {
		stat = RPC_CANTENCODEARGS;
		goto done_broad;
	}
	outlen = (int)xdr_getpos(xdrs);
	xdr_destroy(xdrs);
	/*
	 * Basic loop: broadcast a packet and wait a while for response(s).
	 * The response timeout grows larger per iteration.
	 */
	for (t.tv_sec = 4; t.tv_sec <= 14; t.tv_sec += 2) {
		for (i = 0; i < nets; i++) {
			baddr.sin_addr = addrs[i];
			if (sendto(sock, outbuf, outlen, 0,
				(struct sockaddr *)&baddr,
				sizeof (struct sockaddr)) != outlen) {
				perror("Cannot send broadcast packet");
				stat = RPC_CANTSEND;
				goto done_broad;
			}
		}

		if (eachresult == NULL) {		/* 4.0 */
			stat = RPC_SUCCESS;		/* 4.0 */
			goto done_broad;		/* 4.0 */
		}					/* 4.0 */

	recv_again:
		msg.acpted_rply.ar_verf = _null_auth;
		msg.acpted_rply.ar_results.where = (caddr_t)&r;
                msg.acpted_rply.ar_results.proc = xdr_rmtcallres;
		readFds = mask;				/* 4.0 */

		/* XXX why keep resetting? */
		/* XXX FD_ZERO (&readFds);		   4.0 */
		/* XXX FD_SET (fd, &readFds);		   4.0 */

		/* 4.0 RPC uses getdtablesize () instead of FD_SETSIZE */
		switch (select (FD_SETSIZE, &readFds, (fd_set *)NULL,
			(fd_set *)NULL, &t))
		{

		case 0:  /* timed out */
			stat = RPC_TIMEDOUT;
			continue;

		case -1:  /* some kind of error */
			if (rpcErrnoGet () == EINTR)
				goto recv_again;
			perror("Broadcast select problem");
			stat = RPC_CANTRECV;
			goto done_broad;

		}  /* end of select results switch */

		/* XXX if (!FD_ISSET (fd, &readFds))		4.0 */
		/* XXX	goto recv_again;			4.0 */

	try_again:
		fromlen = sizeof(struct sockaddr);
		inlen = recvfrom(sock, inbuf, MAX_BROADCAST_SIZE, 0,
			(struct sockaddr *)&raddr, &fromlen);
		if (inlen < 0) {
			if (rpcErrnoGet () == EINTR)
				goto try_again;
			perror("Cannot receive reply to broadcast");
			stat = RPC_CANTRECV;
			goto done_broad;
		}
		if (inlen < sizeof(u_long))
			goto recv_again;
		/*
		 * see if reply transaction id matches sent id.
		 * If so, decode the results.
		 */
		xdrmem_create(xdrs, inbuf, (u_int) inlen, XDR_DECODE);
		if (xdr_replymsg(xdrs, &msg)) {
			if ((msg.rm_xid == xid) &&
				(msg.rm_reply.rp_stat == MSG_ACCEPTED) &&
				(msg.acpted_rply.ar_stat == SUCCESS)) {
				raddr.sin_port = htons((u_short)port);
				done = (*eachresult)(resultsp, &raddr);
			}
			/* otherwise, we just ignore the errors ... */
		} else {
#ifdef notdef
			/* some kind of deserialization problem ... */
			if (msg.rm_xid == xid)
				printErr("Broadcast deserialization problem");
			/* otherwise, just random garbage */
#endif
		}
		xdrs->x_op = XDR_FREE;
		msg.acpted_rply.ar_results.proc = xdr_void;
		(void)xdr_replymsg(xdrs, &msg);
		(void)(*xresults)(xdrs, resultsp);
		xdr_destroy(xdrs);
		if (done) {
			stat = RPC_SUCCESS;
			goto done_broad;
		} else {
			goto recv_again;
		}
	}
done_broad:
        if (sock > 0)
	    (void)close(sock);
        if (unix_auth != NULL)
	    AUTH_DESTROY(unix_auth);
	if (inbuf != NULL)
	    KHEAP_FREE(inbuf);
	if (outbuf != NULL)
	    KHEAP_FREE(outbuf);
	return (stat);
}
