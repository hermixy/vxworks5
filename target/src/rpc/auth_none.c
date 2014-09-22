/* auth_none.c -  client authentication for remote systems */

/* Copyright 1984-2000 Wind River Systems, Inc. */
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
01h,18apr00,ham  fixed compilation warnings.
01g,26may92,rrr  the tree shuffle
01f,04oct91,rrr  passed through the ansification filter
		  -changed includes to have absolute path from h/
		  -changed copyright notice
01e,10may90,dnw   moved moduleStatics to rpcGbl; removed init/exit routines
		  changed taskModuleList to taskRpcStatics
01d,27oct89,hjb   upgraded to 4.0
01c,19apr89,gae   added auth_noneExit to do tidy cleanup of tasks.
		  changed auth_noneInit to return pointer to moduleStatics.
01b,11nov87,jlf   added wrs copyright, title, mod history, etc.
01a,01nov87,rdc   first VxWorks version
*/

/*
 * auth_none.c
 * Creates a client authentication handle for passing "null"
 * credentials and verifiers to remote systems.
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 */

#include "rpc/rpctypes.h"
#include "rpc/xdr.h"
#include "rpc/auth.h"
#include "vxWorks.h"
#include "memLib.h"
#include "rpc/rpcGbl.h"

extern bool_t xdr_opaque_auth (XDR *xdrs, struct opaque_auth *ap);

/* XXX #define NULL ((caddr_t)0) */
/* #define MAX_MARSHEL_SIZE 20 XXX moved to rpcGbl */

/*
 * Authenticator operations routines
 */
LOCAL void	authnone_verf();			/* 4.0 */
LOCAL void	authnone_destroy();			/* 4.0 */
LOCAL bool_t	authnone_marshal();			/* 4.0 */
LOCAL bool_t	authnone_validate();			/* 4.0 */
LOCAL bool_t	authnone_refresh();			/* 4.0 */

static struct auth_ops ops = {
	authnone_verf,
	authnone_marshal,
	authnone_validate,
	authnone_refresh,
	authnone_destroy
};

AUTH *
authnone_create()
{
	XDR xdr_stream;
	register XDR *xdrs;
	FAST struct auth_none *ms = &taskRpcStatics->auth_none;

	if (! ms->mcnt) {
		ms->no_client.ah_cred = ms->no_client.ah_verf = _null_auth;
		ms->no_client.ah_ops = &ops;
		xdrs = &xdr_stream;
		xdrmem_create(xdrs, ms->marshalled_client,
		    (u_int)MAX_MARSHEL_SIZE, XDR_ENCODE);
		(void)xdr_opaque_auth(xdrs, &ms->no_client.ah_cred);
		(void)xdr_opaque_auth(xdrs, &ms->no_client.ah_verf);
		ms->mcnt = XDR_GETPOS(xdrs);
		XDR_DESTROY(xdrs);
	}
	return (&ms->no_client);
}

/*ARGSUSED*/
LOCAL bool_t			/* 4.0 */
authnone_marshal(client, xdrs)
	AUTH *client;
	XDR *xdrs;
{
	FAST struct auth_none *ms = &taskRpcStatics->auth_none;

	return ((*xdrs->x_ops->x_putbytes)(xdrs, ms->marshalled_client,
					   ms->mcnt));
}

LOCAL void 			/* 4.0 */
authnone_verf()
{
}

LOCAL bool_t			/* 4.0 */
authnone_validate()
{

	return (TRUE);
}

LOCAL bool_t			/* 4.0 */
authnone_refresh()
{

	return (FALSE);
}

LOCAL void			/* 4.0 */
authnone_destroy()
{
}
