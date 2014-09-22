/* xdr_float.c - generic XDR routines implementation */

/* Copyright 1984-1997 Wind River Systems, Inc. */
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
02f,22oct01,dee  Add CPU coldfire
02e,07mar00,zl   added Hitachi SH support.
02d,06may98,cym  added SIMNT to portable list.
02c,23jan97,cdp  make ARM use big-endian word order in xdr_double.
02b,28nov96,cdp  added ARM support.
02a,27may94,yao  added PPC support.
01s,31aug95,ms   fixed xdr_double for little endian machines (SPR #4205)
01s,28nov95,ism  updated for vxsim/solaris
01r,19mar95,dvs  removed tron references.
01q,02dec93,pme  added Am29K support
01p,11aug93,rrr  vxsim hppa.
01o,23jun93,rrr  vxsim.
01n,09jun93,hdn  added a support for I80X86
01m,26may92,rrr  the tree shuffle
01l,04oct91,rrr  passed through the ansification filter
		  -changed includes to have absolute path from h/
		  -fixed #else and #endif
		  -changed copyright notice
01k,06aug91,ajm   added proper definitions of "mipsr3k".
01j,29apr91,hdn   added defines and macros for TRON architecture.
01i,01apr91,elh   added xdr_floatInclude.
01h,25mar91,del   modified CPU_FAMILY checking for floating point type.
01g,17oct90,gae   changed #elif to ordinary #if for the sake of some compilers.
01f,08oct90,hjb   added proper definitions of "mc68000" and "sparc".
01e,30apr90,gae   fixed compiler dependent definitions.
01d,19apr90,hjb   de-linted.
01c,27oct89,hjb   upgraded to 4.0
01b,11nov87,jlf   added wrs copyright, title, mod history, etc.
01a,01nov87,rdc   first VxWorks version
*/

#ifndef lint
/* static char sccsid[] = "@(#)xdr_float.c 1.1 86/02/03 Copyr 1984 Sun Micro"; */
#endif

/*
 * xdr_float.c, Generic XDR routines impelmentation.
 *
 * These are the "floating point" xdr routines used to (de)serialize
 * most common data items.  See xdr.h for more info on the interface to
 * xdr.
 */

#include "rpc/rpctypes.h"
#include "rpc/xdr.h"
#include "vxWorks.h"

#if ((CPU_FAMILY == MC680X0) || (CPU_FAMILY == I960) || (CPU_FAMILY==SPARC) || \
     (CPU_FAMILY == MIPS) || (CPU_FAMILY == SIMSPARCSUNOS) || \
     (CPU_FAMILY == SIMHPPA) || (CPU_FAMILY == AM29XXX) || \
     (CPU_FAMILY == I80X86) || (CPU_FAMILY == SIMSPARCSOLARIS) || \
     (CPU_FAMILY == SIMNT) || (CPU_FAMILY == ARM) || (CPU_FAMILY == PPC) || \
     (CPU_FAMILY == SH) || (CPU_FAMILY == COLDFIRE))
#define	FLOAT_NORM
#endif	/* (CPU_FAMILY) */

/*
 * NB: Not portable.
 * This routine works on Suns (Sky / 68000's) and Vaxen.
 */

/* What IEEE single precision floating point looks like on a Vax */
struct  ieee_single {
	unsigned int	mantissa: 23;
	unsigned int	exp     : 8;
	unsigned int	sign    : 1;
};

/* Vax single precision floating point */
struct  vax_single {
	unsigned int	mantissa1 : 7;
	unsigned int	exp       : 8;
	unsigned int	sign      : 1;
	unsigned int	mantissa2 : 16;

};

#define VAX_SNG_BIAS    0x81
#define IEEE_SNG_BIAS   0x7f

#if !defined(FLOAT_NORM)		/* 4.0 */
static struct sgl_limits {
	struct vax_single s;
	struct ieee_single ieee;
} sgl_limits[2] = {
	{{ 0x3f, 0xff, 0x0, 0xffff },	/* Max Vax */
	{ 0x0, 0xff, 0x0 }},		/* Max IEEE */
	{{ 0x0, 0x0, 0x0, 0x0 },	/* Min Vax */
	{ 0x0, 0x0, 0x0 }}		/* Min IEEE */
};
#endif

void xdr_floatInclude ()
    {
    }

bool_t
xdr_float(xdrs, fp)
	register XDR *xdrs;
	register float *fp;
{
#if !defined(FLOAT_NORM)
	struct ieee_single is;
	struct vax_single vs, *vsp;
	struct sgl_limits *lim;
	int i;
#endif

	switch (xdrs->x_op) {

	case XDR_ENCODE:
#if defined(FLOAT_NORM)
		return (XDR_PUTLONG(xdrs, (long *)fp));
#else
		vs = *((struct vax_single *)fp);
		for (i = 0, lim = sgl_limits;
			i < sizeof(sgl_limits)/sizeof(struct sgl_limits);
			i++, lim++) {
			if ((vs.mantissa2 == lim->s.mantissa2) &&
				(vs.exp == lim->s.exp) &&
				(vs.mantissa1 == lim->s.mantissa1)) {
				is = lim->ieee;
				goto shipit;
			}
		}
		is.exp = vs.exp - VAX_SNG_BIAS + IEEE_SNG_BIAS;
		is.mantissa = (vs.mantissa1 << 16) | vs.mantissa2;
	shipit:
		is.sign = vs.sign;
		return (XDR_PUTLONG(xdrs, (long *)&is));
#endif

	case XDR_DECODE:
#if defined(FLOAT_NORM)				/* 4.0 */
		return (XDR_GETLONG(xdrs, (long *)fp));
#else
		vsp = (struct vax_single *)fp;
		if (!XDR_GETLONG(xdrs, (long *)&is))
			return (FALSE);
		for (i = 0, lim = sgl_limits;
			i < sizeof(sgl_limits)/sizeof(struct sgl_limits);
			i++, lim++) {
			if ((is.exp == lim->ieee.exp) &&
				(is.mantissa = lim->ieee.mantissa)) {
				*vsp = lim->s;
				goto doneit;
			}
		}
		vsp->exp = is.exp - IEEE_SNG_BIAS + VAX_SNG_BIAS;
		vsp->mantissa2 = is.mantissa;
		vsp->mantissa1 = (is.mantissa >> 16);
	doneit:
		vsp->sign = is.sign;
		return (TRUE);
#endif

	case XDR_FREE:
		return (TRUE);
	}
	return (FALSE);
}

/*
 * This routine works on Suns (Sky / 68000's) and Vaxen.
 */

#ifdef vax		/* 4.0 */
/* What IEEE double precision floating point looks like on a Vax */
struct  ieee_double {
	unsigned int	mantissa1 : 20;
	unsigned int	exp	  : 11;
	unsigned int	sign	  : 1;
	unsigned int	mantissa2 : 32;
};

/* Vax double precision floating point */
struct  vax_double {
	unsigned int	mantissa1 : 7;
	unsigned int	exp       : 8;
	unsigned int	sign      : 1;
	unsigned int	mantissa2 : 16;
	unsigned int	mantissa3 : 16;
	unsigned int	mantissa4 : 16;
};

#define VAX_DBL_BIAS    0x81
#define IEEE_DBL_BIAS   0x3ff
#define MASK(nbits)     ((1 << nbits) - 1)

static struct dbl_limits {
	struct  vax_double d;
	struct  ieee_double ieee;
} dbl_limits[2] = {
	{{ 0x7f, 0xff, 0x0, 0xffff, 0xffff, 0xffff },	/* Max Vax */
	{ 0x0, 0x7ff, 0x0, 0x0 }},			/* Max IEEE */
	{{ 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},		/* Min Vax */
	{ 0x0, 0x0, 0x0, 0x0 }}				/* Min IEEE */
};

#endif	/* vax */		/* 4.0 */

bool_t
xdr_double(xdrs, dp)
	register XDR *xdrs;
	double *dp;
{
	register long *lp;
#if !defined(FLOAT_NORM)				/* 4.0 */
	struct  ieee_double id;
	struct  vax_double vd;
	register struct dbl_limits *lim;
	int i;
#endif

	switch (xdrs->x_op) {

	case XDR_ENCODE:
#if defined(FLOAT_NORM)					/* 4.0 */
		lp = (long *)dp;
#else
		vd = *((struct  vax_double *)dp);
		for (i = 0, lim = dbl_limits;
			i < sizeof(dbl_limits)/sizeof(struct dbl_limits);
			i++, lim++) {
			if ((vd.mantissa4 == lim->d.mantissa4) &&
				(vd.mantissa3 == lim->d.mantissa3) &&
				(vd.mantissa2 == lim->d.mantissa2) &&
				(vd.mantissa1 == lim->d.mantissa1) &&
				(vd.exp == lim->d.exp)) {
				id = lim->ieee;
				goto shipit;
			}
		}
		id.exp = vd.exp - VAX_DBL_BIAS + IEEE_DBL_BIAS;
		id.mantissa1 = (vd.mantissa1 << 13) | (vd.mantissa2 >> 3);
		id.mantissa2 = ((vd.mantissa2 & MASK(3)) << 29) |
				(vd.mantissa3 << 13) |
				((vd.mantissa4 >> 3) & MASK(13));
	shipit:
		id.sign = vd.sign;
		lp = (long *)&id;
#endif
#if	(_BYTE_ORDER == _BIG_ENDIAN) || (CPU_FAMILY == ARM)
		return (XDR_PUTLONG(xdrs, lp++) && XDR_PUTLONG(xdrs, lp));
#else	/* _BYTE_ORDER == _LITTLE_ENDIAN */
		return (XDR_PUTLONG(xdrs, lp+1) && XDR_PUTLONG(xdrs, lp));
#endif	/* _BYTE_ORDER == _BIG_ENDIAN */

	case XDR_DECODE:
#if defined(FLOAT_NORM)					/* 4.0 */
		lp = (long *)dp;
#if	(_BYTE_ORDER == _BIG_ENDIAN) || (CPU_FAMILY == ARM)
		return (XDR_GETLONG(xdrs, lp++) && XDR_GETLONG(xdrs, lp));
#else	/* _BYTE_ORDER == _LITTLE_ENDIAN */
		return (XDR_GETLONG(xdrs, lp+1) && XDR_GETLONG(xdrs, lp));
#endif	/* _BYTE_ORDER == _BIG_ENDIAN */
#else
		lp = (long *)&id;
		if (!XDR_GETLONG(xdrs, lp++) || !XDR_GETLONG(xdrs, lp))
			return (FALSE);
		for (i = 0, lim = dbl_limits;
			i < sizeof(dbl_limits)/sizeof(struct dbl_limits);
			i++, lim++) {
			if ((id.mantissa2 == lim->ieee.mantissa2) &&
				(id.mantissa1 == lim->ieee.mantissa1) &&
				(id.exp == lim->ieee.exp)) {
				vd = lim->d;
				goto doneit;
			}
		}
		vd.exp = id.exp - IEEE_DBL_BIAS + VAX_DBL_BIAS;
		vd.mantissa1 = (id.mantissa1 >> 13);
		vd.mantissa2 = ((id.mantissa1 & MASK(13)) << 3) |
				(id.mantissa2 >> 29);
		vd.mantissa3 = (id.mantissa2 >> 13);
		vd.mantissa4 = (id.mantissa2 << 3);
	doneit:
		vd.sign = id.sign;
		*dp = *((double *)&vd);
		return (TRUE);
#endif

	case XDR_FREE:
		return (TRUE);
	}
	return (FALSE);
}
