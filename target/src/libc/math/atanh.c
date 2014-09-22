/* atanh.c - math routines */

/* Copyright 1992 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,14oct92,jdi  made atanh() nomanual.
01b,20sep92,smb  documentation additions
01a,08jul92,smb  documentation.
*/

/*
DESCRIPTION
* Copyright (c) 1985 Regents of the University of California.
*  All rights reserved.
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
SEE ALSO: American National Standard X3.159-1989
NOMANUAL
*/

#include "vxWorks.h"
#include "math.h"
#if defined(vax)||defined(tahoe)
#include <errno.h>
#endif	/* defined(vax)||defined(tahoe) */

/*****************************************************************************
*
* atanh	-
*
* ATANH(X)
* RETURN THE HYPERBOLIC ARC TANGENT OF X
* DOUBLE PRECISION (VAX D format 56 bits, IEEE DOUBLE 53 BITS)
* CODED IN C BY K.C. NG, 1/8/85;
* REVISED BY K.C. NG on 2/7/85, 3/7/85, 8/18/85.
*
* Required kernel function:
* .CS
*	log1p(x) 	...return log(1+x)
* .CE
*
* Method :
* .CS
*	Return
*                          1              2x                          x
*		atanh(x) = --- * log(1 + -------) = 0.5 * log1p(2 * --------)
*                          2             1 - x                      1 - x
* .CE
*
* Special cases are if atanh(x) is NaN if |x| > 1 with signal; atanh(NaN) is 
* that NaN with no signal; or if atanh(+-1) is +-INF with signal.
*
* atanh(x) returns the exact hyperbolic arc tangent of x nearly rounded.
* In a test run with 512,000 random arguments on a VAX, the maximum
* observed error was 1.87 ulps (units in the last place) at
* x= -3.8962076028810414000e-03.
*
* NOMANUAL
*/

double atanh(x)
double x;
{
	double copysign(),log1p(),z;
	z = copysign(0.5,x);
	x = copysign(x,1.0);
#if defined(vax)||defined(tahoe)
	if (x == 1.0) {
	    extern double infnan();
	    return(copysign(1.0,z)*infnan(ERANGE));	/* sign(x)*INF */
	}
#endif	/* defined(vax)||defined(tahoe) */
	x = x/(1.0-x);
	return( z*log1p(x+x) );
}
