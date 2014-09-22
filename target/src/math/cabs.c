/* cabs.c - math routines */

/* Copyright 1992 Wind River Systems, Inc. */

/*
modification history
--------------------
01d,15feb93,jdi  made hypot() NOMANUAL.
01c,14oct92,jdi  made cabs() NOMANUAL.
01b,13oct92,jdi  mangen fixes.
01a,30jul92,smb  documentation.
*/

/*
DESCRIPTION
* Copyright (c) 1985 Regents of the University of California.
* All rights reserved.
*
* Redistribution and use in source and binary forms are permitted
* provided that the above copyright notice and this paragraph are
* duplicated in all such forms and that any documentation,
* advertising materials, and other materials related to such
* distribution and use acknowledge that the software was developed
* by the University of California, Berkeley.  The name of the
* University may not be used to endorse or promote products derived
* from this software without specific prior written permission.
* THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
* IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
* WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
*
* All recipients should regard themselves as participants in an ongoing
* research project and hence should feel obligated to report their
* experiences (good or bad) with these elementary function codes, using
* the sendbug(8) program, to the authors.
*
#ifndef lint
static char sccsid[] = "@(#)cabs.c	5.3 (Berkeley) 6/30/88";
#endif	* not lint *

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/


#include "vxWorks.h"
#include "math.h"

#if defined(vax)||defined(tahoe)	/* VAX D format */
#ifdef vax
#define _0x(A,B)	0x/**/A/**/B
#else	/* vax */
#define _0x(A,B)	0x/**/B/**/A
#endif	/* vax */
/* static double */
/* r2p1hi =  2.4142135623730950345E0     , Hex  2^  2   *  .9A827999FCEF32 */
/* r2p1lo =  1.4349369327986523769E-17   , Hex  2^-55   *  .84597D89B3754B */
/* sqrt2  =  1.4142135623730950622E0     ; Hex  2^  1   *  .B504F333F9DE65 */
static long    r2p1hix[] = { _0x(8279,411a), _0x(ef32,99fc)};
static long    r2p1lox[] = { _0x(597d,2484), _0x(754b,89b3)};
static long     sqrt2x[] = { _0x(04f3,40b5), _0x(de65,33f9)};
#define   r2p1hi    (*(double*)r2p1hix)
#define   r2p1lo    (*(double*)r2p1lox)
#define    sqrt2    (*(double*)sqrt2x)
#else	/* defined(vax)||defined(tahoe) */
static double
r2p1hi =  2.4142135623730949234E0     , /*Hex  2^1     *  1.3504F333F9DE6 */
r2p1lo =  1.2537167179050217666E-16   , /*Hex  2^-53   *  1.21165F626CDD5 */
sqrt2  =  1.4142135623730951455E0     ; /*Hex  2^  0   *  1.6A09E667F3BCD */
#endif	/* defined(vax)||defined(tahoe) */

/******************************************************************************
*
* hypot - return the square root of X^2 + Y^2 where Z = X + iY
*
* This routine returns the square root of X^2 + Y^2 where Z=X+iY
* in double precision (VAX D format 56 bits, IEEE double 53 bits).
*
* Required system support functions: copysign(), finite(), scalb(), sqrt().
*
* Method:
* 1. replace x by |x| and y by |y|, and swap x and
*    y if y > x (hence x is never smaller than y).
* 2. Hypot(x,y) is computed by:
* .CS
*   Case I, x/y > 2
*
*			       y
*	hypot = x + -----------------------------
*				    2
*		    sqrt ( 1 + [x/y]  )  +  x/y
*
*   Case II, x/y <= 2
*					   y
*	hypot = x + --------------------------------------------------
*						     2
*						[x/y]   -  2
*		   (sqrt(2)+1) + (x-y)/y + -----------------------------
*							  2
*					  sqrt ( 1 + [x/y]  )  + sqrt(2)
* .CE
*
* Special cases:
* .CS
*	hypot(x,y) is INF if x or y is +INF or -INF; else
*	hypot(x,y) is NAN if x or y is NAN.
* .CE
*
* ACCURACY:
* hypot() returns the sqrt(x^2+y^2) with error less than 1 ulps (units
* in the last place).  See Kahan's "Interval Arithmetic Options in the
* Proposed IEEE Floating Point Arithmetic Standard", Interval Mathematics
* 1980, Edited by Karl L.E. Nickel, pp 99-128.  (A faster but less accurate
* code follows in comments.)  In a test run with 500,000 random arguments
* on a VAX, the maximum observed error was 0.959 ulps.
*
* CONSTANTS:
* The hexadecimal values are the intended ones for the following constants.
* The decimal values may be used, provided that the compiler will convert
* from decimal to binary accurately enough to produce the hexadecimal values
* shown.
*
* RETURNS: The square root of X^2 + Y^2 where Z = X + iY.
*
* INTERNAL:
* Coded in C by K.C. Ng, 11/28/84;
* Revised by K.C. Ng, 7/12/85.
*/

double hypot(x,y)
    double x, y;
{
	static double zero=0, one=1,
		      small=1.0E-18;	/* fl(1+small)==1 */
	static ibig=30;	/* fl(1+2**(2*ibig))==1 */
	double copysign(),scalb(),logb(),sqrt(),t,r;
	int finite(), exp;

	if(finite(x))
	    if(finite(y))
	    {
		x=copysign(x,one);
		y=copysign(y,one);
		if(y > x)
		    { t=x; x=y; y=t; }
		if(x == zero) return(zero);
		if(y == zero) return(x);
		exp= logb(x);
		if(exp-(int)logb(y) > ibig )
			/* raise inexact flag and return |x| */
		   { one+small; return(x); }

	    /* start computing sqrt(x^2 + y^2) */
		r=x-y;
		if(r>y) { 	/* x/y > 2 */
		    r=x/y;
		    r=r+sqrt(one+r*r); }
		else {		/* 1 <= x/y <= 2 */
		    r/=y; t=r*(r+2.0);
		    r+=t/(sqrt2+sqrt(2.0+t));
		    r+=r2p1lo; r+=r2p1hi; }

		r=y/r;
		return(x+r);

	    }

	    else if(y==y)   	   /* y is +-INF */
		     return(copysign(y,one));
	    else
		     return(y);	   /* y is NaN and x is finite */

	else if(x==x) 		   /* x is +-INF */
	         return (copysign(x,one));
	else if(finite(y))
	         return(x);		   /* x is NaN, y is finite */
#if !defined(vax)&&!defined(tahoe)
	else if(y!=y) return(y);  /* x and y is NaN */
#endif	/* !defined(vax)&&!defined(tahoe) */
	else return(copysign(y,one));   /* y is INF */
}

/*****************************************************************************
*
* cabs - return the absolute value of the complex number Z = X + iY
*
* This routine returns the absolute value of the complex number Z = X + iY,
* in double precision (VAX D format 56 bits, IEEE double 53 bits).
*
* Required kernel function:  hypot()
*
* Method :
* .CS
*	cabs(z) = hypot(x,y) .
* .CE
*
* RETURNS: The absolute value of Z = X + iY.
*
* INTERNAL:
* Coded in C by K.C. Ng, 11/28/84.
* Revised by K.C. Ng, 7/12/85.
*
* NOMANUAL
*/

double cabs(z)
    struct { double x, y;} z;
{
	return hypot(z.x,z.y);
}

double
z_abs(z)
struct { double x,y;} *z;
{
	return hypot(z->x,z->y);
}
