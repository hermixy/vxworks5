/* asincos.c - inverse sine and cosine math routines */

/* Copyright 1992-1994 Wind River Systems, Inc. */

/*
modification history
--------------------
01h,09dec94,rhp  fix man pages for inverse trig fns
01g,05feb93,jdi  doc changes based on kdl review.
01f,02dec92,jdi  doc tweaks.
01e,28oct92,jdi  documentation cleanup.
01d,13oct92,jdi  mangen fixes.
01c,21sep92,smb  corrected file name in first line of file.
01b,20sep92,smb  documentation additions
01a,08jul92,smb  documentation
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

SEE ALSO: American National Standard X3.159-1989
NOMANUAL
*/

#include "vxWorks.h"
#include "math.h"

/*******************************************************************************
*
* asin - compute an arc sine (ANSI)
*
* This routine returns the principal value of the arc sine of <x>
* in double precision (IEEE double, 53 bits).
* If <x> is the sine of an angle <T>, this function returns <T>.
*
* A domain error occurs for arguments not in the range [-1,+1].
*
* INTERNAL
* Method:
*     asin(x) = atan2(x,sqrt(1-x*x))
* For better accuracy, 1-x*x is computed as follows:
*     1-x*x                     if x <  0.5,
*     2*(1-|x|)-(1-|x|)*(1-|x|) if x >= 0.5.
*
* INCLUDE FILES: math.h
*
* RETURNS:
* The double-precision arc sine of <x> in the range [-pi/2,pi/2] radians.
*
* Special cases:
*     If <x> is NaN, asin() returns <x>.
*     If |x|>1, it returns NaN.
*
* SEE ALSO: mathALib
*
* INTERNAL
* Coded in C by K.C. Ng, 4/16/85, REVISED ON 6/10/85.
*/

double asin
    (
    double x	/* number between -1 and 1 */
    )

    {
	double s,t,copysign(),atan2(),sqrt(),one=1.0;
#if !defined(vax)&&!defined(tahoe)
	if(x!=x) return(x);	/* x is NaN */
#endif	/* !defined(vax)&&!defined(tahoe) */
	s=copysign(x,one);
	if(s <= 0.5)
	    return(atan2(x,sqrt(one-x*x)));
	else
	    { t=one-s; s=t+t; return(atan2(x,sqrt(s-t*t))); }

    }

/*******************************************************************************
*
* acos - compute an arc cosine (ANSI)
*
* This routine returns principal value of the arc cosine of <x>
* in double precision (IEEE double, 53 bits).
* If <x> is the cosine of an angle <T>, this function returns <T>.
*
* A domain error occurs for arguments not in the range [-1,+1].
*
* INTERNAL
* Method:
*			      ________
*                           / 1 - x
*	acos(x) = 2*atan2( / -------- , 1 ) .
*                        \/   1 + x
*
* INCLUDE FILES: math.h
*
* RETURNS:
* The double-precision arc cosine of <x> in the range [0,pi] radians.
*
* Special cases:
*     If <x> is NaN, acos() returns <x>.
*     If |x|>1, it returns NaN.
*
* SEE ALSO: mathALib
*
* INTERNAL
* Coded in C by K.C. Ng, 4/16/85, revised on 6/10/85.
*/

double acos
    (
    double x	/* number between -1 and 1 */
    )

    {
	double t,copysign(),atan2(),sqrt(),one=1.0;
#if !defined(vax)&&!defined(tahoe)
	if(x!=x) return(x);
#endif	/* !defined(vax)&&!defined(tahoe) */
	if( x != -1.0)
	    t=atan2(sqrt((one-x)/(one+x)),one);
	else
	    t=atan2(one,0.0);	/* t = PI/2 */
	return(t+t);
    }
