/* xdr_mem.c - XDR implementation using memory buffers */

/* Copyright 1984-1999 Wind River Systems, Inc. */
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
01g,27jul99,elg  Add xdrmem_putlongs() and xdrmem_putwords() routines.
01f,26may92,rrr  the tree shuffle
		  -changed includes to have absolute path from h/
01e,04oct91,rrr  passed through the ansification filter
		  -changed includes to have absolute path from h/
		  -changed copyright notice
01d,19apr90,hjb   de-linted.
01c,27oct89,hjb   upgraded to 4.0
01b,11nov87,jlf   added wrs copyright, title, mod history, etc.
01a,01nov87,rdc   first VxWorks version
*/

#ifndef lint
/* static char sccsid[] = "@(#)xdr_mem.c 1.1 86/02/03 Copyr 1984 Sun Micro"; */
#endif

/*
 * xdr_mem.c, XDR implementation using memory buffers.
 *
 * If you have some data to be interpreted as external data representation
 * or to be converted to external data representation in a memory buffer,
 * then this is the package for you.
 *
 */

#include "rpc/rpctypes.h"
#include "rpc/xdr.h"
#include "netinet/in.h"
#include "vxWorks.h"

LOCAL bool_t	xdrmem_getlong();
LOCAL bool_t	xdrmem_putlong();
LOCAL bool_t	xdrmem_getbytes();
LOCAL bool_t	xdrmem_putbytes();
LOCAL bool_t	xdrmem_putwords();
LOCAL bool_t	xdrmem_putlongs();
LOCAL u_int	xdrmem_getpos();
LOCAL bool_t	xdrmem_setpos();
LOCAL long *	xdrmem_inline();
LOCAL void	xdrmem_destroy();

static struct	xdr_ops xdrmem_ops = {
	xdrmem_getlong,
	xdrmem_putlong,
	xdrmem_getbytes,
	xdrmem_putbytes,
	xdrmem_putwords,
	xdrmem_putlongs,
	xdrmem_getpos,
	xdrmem_setpos,
	xdrmem_inline,
	xdrmem_destroy
};

/*
 * The procedure xdrmem_create initializes a stream descriptor for a
 * memory buffer.
 */
void
xdrmem_create(xdrs, addr, size, op)
	register XDR *xdrs;
	caddr_t addr;
	u_int size;
	enum xdr_op op;
{

	xdrs->x_op = op;
	xdrs->x_ops = &xdrmem_ops;
	xdrs->x_private = xdrs->x_base = addr;
	xdrs->x_handy = size;
}

LOCAL void						/* 4.0 */
xdrmem_destroy(/*xdrs*/)
	/*XDR *xdrs;*/
{
}

LOCAL bool_t						/* 4.0 */
xdrmem_getlong(xdrs, lp)
	register XDR *xdrs;
	long *lp;
{

	if ((xdrs->x_handy -= sizeof(long)) < 0)
		return (FALSE);
	*lp = ntohl(*((u_long *)(xdrs->x_private)));
	xdrs->x_private += sizeof(long);
	return (TRUE);
}

LOCAL bool_t						/* 4.0 */
xdrmem_putlong(xdrs, lp)
	register XDR *xdrs;
	long *lp;
{

	if ((xdrs->x_handy -= sizeof(long)) < 0)
		return (FALSE);
	*(u_long *)xdrs->x_private = htonl((u_long) *lp);
	xdrs->x_private += sizeof(long);
	return (TRUE);
}

LOCAL bool_t						/* 4.0 */
xdrmem_getbytes(xdrs, addr, len)
	register XDR *xdrs;
	caddr_t addr;
	register u_int len;
{

	if ((xdrs->x_handy -= len) < 0)
		return (FALSE);
	bcopy(xdrs->x_private, addr, (int) len);
	xdrs->x_private += len;
	return (TRUE);
}

LOCAL bool_t						/* 4.0 */
xdrmem_putbytes(xdrs, addr, len)
	register XDR *xdrs;
	caddr_t addr;
	register u_int len;
{

	if ((xdrs->x_handy -= len) < 0)
		return (FALSE);
	bcopy(addr, xdrs->x_private, (int) len);
	xdrs->x_private += len;
	return (TRUE);
}

LOCAL bool_t						/* 4.0 */
xdrmem_putwords (xdrs, addr, len)
	register XDR *	xdrs;
	caddr_t 	addr;
	register u_int 	len;
{

	if ((xdrs->x_handy -= len * 2) < 0)
		return (FALSE);
	bcopyWords (addr, xdrs->x_private, (int) len);
	xdrs->x_private += len * 2;
	return (TRUE);
}

LOCAL bool_t						/* 4.0 */
xdrmem_putlongs (xdrs, addr, len)
	register XDR *	xdrs;
	caddr_t 	addr;
	register u_int 	len;
{

	if ((xdrs->x_handy -= len * 4) < 0)
		return (FALSE);
	bcopyLongs (addr, xdrs->x_private, (int) len);
	xdrs->x_private += len * 4;
	return (TRUE);
}

LOCAL u_int						/* 4.0 */
xdrmem_getpos(xdrs)
	register XDR *xdrs;
{

	return ((u_int)xdrs->x_private - (u_int)xdrs->x_base);
}

LOCAL bool_t						/* 4.0 */
xdrmem_setpos(xdrs, pos)
	register XDR *xdrs;
	u_int pos;
{
	register caddr_t newaddr = xdrs->x_base + pos;
	register caddr_t lastaddr = xdrs->x_private + xdrs->x_handy;

	if ((long)newaddr > (long)lastaddr)
		return (FALSE);
	xdrs->x_private = newaddr;
	xdrs->x_handy = (int)lastaddr - (int)newaddr;
	return (TRUE);
}

LOCAL long *						/* 4.0 */
xdrmem_inline(xdrs, len)
	register XDR *xdrs;
	int len;
{
	long *buf = 0;

	if (xdrs->x_handy >= len) {
		xdrs->x_handy -= len;
		buf = (long *) xdrs->x_private;
		xdrs->x_private += len;
	}
	return (buf);
}
