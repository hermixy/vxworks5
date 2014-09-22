/* svc_simple.c - simplified front end to rpc */

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
01p,15oct01,rae  merge from truestack ver 01q, base 01o (AE / 5_X)
01o,20feb97,jank removed comment in comment L:158
01n,15sep93,kdl  changed svcudp_create() call to only pass one param (SPR #2427)
01m,11aug93,jmm  Changed ioctl.h and socket.h to sys/ioctl.h and sys/socket.h
01l,26may92,rrr  the tree shuffle
01k,04oct91,rrr  passed through the ansification filter
		  -changed includes to have absolute path from h/
		  -changed copyright notice
01j,01apr91,elh	  added svc_simpleInclude.
01i,25oct90,dnw   removed include of utime.h.
01h,10may90,dnw   moved moduleStatics to rpcGbl; removed init/exit routines
		  changed taskModuleList to taskRpcStatics
01g,19apr90,hjb   de-linted.
01f,27oct89,hjb   upgraded to 4.0
01e,04may89,dnw   fixed bug in universal() of not freeing args.
01d,19apr89,gae   added svc_simpleExit to do tidy cleanup for tasks.
		  changed svc_simpleInit to return pointer to moduleStatics.
01c,05apr88,gae   changed fprintf() to printErr().
01b,11nov87,jlf   added wrs copyright, title, mod history, etc.
01a,01nov87,rdc   first VxWorks version
*/

#ifndef lint
/* static char sccsid[] = "@(#)svc_simple.c 1.1 86/02/03 Copyr 1984 Sun Micro"; */
#endif

/*
 * svc_simple.c
 * Simplified front end to rpc.
 *
 */

#include "rpc/rpc.h"
#include "sys/socket.h"
#include "vxWorks.h"
#include "memLib.h"
#include "rpc/rpcGbl.h"
#include "memPartLib.h"
#include "stdio.h"
#include "rpc/pmap_clnt.h"

LOCAL void universal();				/* 4.0 */

void svc_simpleInclude ()
    {
    }

int
registerrpc(prognum, versnum, procnum, progname, inproc, outproc)
	char *(*progname)();
	xdrproc_t inproc, outproc;
{
	FAST struct svc_simple *ms = &taskRpcStatics->svc_simple;

	if (procnum == NULLPROC) {
		printErr ("can't reassign procedure number %d\n", NULLPROC);
		return (-1);
	}
	if (!ms->madetransp) {
		ms->madetransp = 1;
		ms->transp = svcudp_create(RPC_ANYSOCK);
		if (ms->transp == NULL) {
			printErr ("couldn't create an rpc server\n");
			return (-1);
		}
	}
	pmap_unset((u_long) prognum, (u_long) versnum);
	if (!svc_register(ms->transp, (u_long) prognum, (u_long) versnum,
			  universal, IPPROTO_UDP)) {
	    	printErr ("couldn't register prog %d vers %d\n",
			  prognum, versnum);
		return (-1);
	}
	ms->pl = (struct proglst *) KHEAP_ALLOC(sizeof(struct proglst));
	if (ms->pl == NULL) {
		printErr ("registerrpc: out of memory\n");
		return (-1);
	}
	ms->pl->p_progname = progname;
	ms->pl->p_prognum = prognum;
	ms->pl->p_procnum = procnum;
	ms->pl->p_inproc = inproc;
	ms->pl->p_outproc = outproc;
	ms->pl->p_nxt = ms->proglst;
	ms->proglst = ms->pl;
	return (0);
}


LOCAL void universal(rqstp, transp)				/* 4.0 */
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	int prog, proc;
	char *outdata;
	/* XXX char xdrbuf[UDPMSGSIZE]; */
	char *xdrbuf = (char *) KHEAP_ALLOC(UDPMSGSIZE);
	struct proglst *pl;
	FAST struct svc_simple *ms = &taskRpcStatics->svc_simple;

	if (xdrbuf == NULL)
	    {
	    panic ("rpc: universal: out of memory\n");
	    return;
	    }

	/*
	 * enforce "procnum 0 is echo" convention
	 */
	if (rqstp->rq_proc == NULLPROC) {
		if (svc_sendreply(transp, xdr_void, (char *) 0) == FALSE) {
			printErr ("xxx\n");
			KHEAP_FREE(xdrbuf);
			taskSuspend (0);
		}
		KHEAP_FREE(xdrbuf);
		return;
	}
	prog = rqstp->rq_prog;
	proc = rqstp->rq_proc;
	for (pl = ms->proglst; pl != NULL; pl = pl->p_nxt)
		if (pl->p_prognum == prog && pl->p_procnum == proc) {
			/* decode arguments into a CLEAN buffer */
			/*XXX bzero(xdrbuf, sizeof(xdrbuf)); required ! */
			bzero(xdrbuf, UDPMSGSIZE); /* required ! */
			if (!svc_getargs(transp, pl->p_inproc, xdrbuf)) {
				svcerr_decode(transp);
				KHEAP_FREE(xdrbuf);
				return;
			}
			outdata = (*(pl->p_progname))(xdrbuf);
			if (outdata == NULL && pl->p_outproc != xdr_void)
				{
				/* there was an error */
				KHEAP_FREE(xdrbuf);
				return;
				}
			if (!svc_sendreply(transp, pl->p_outproc, outdata)) {
				printErr ("trouble replying to prog %d\n",
				    pl->p_prognum);
				KHEAP_FREE(xdrbuf);
				taskSuspend (0);
			}
			/* free the decoded arguments */
			(void)svc_freeargs(transp, pl->p_inproc, xdrbuf);
			KHEAP_FREE(xdrbuf);
			return;
		}
	printErr ("never registered prog %d\n", prog);
	KHEAP_FREE(xdrbuf);
	taskSuspend (0);
}
