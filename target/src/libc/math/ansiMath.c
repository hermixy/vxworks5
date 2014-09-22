/* ansiMath.c - ANSI `math' documentation */

/* Copyright 1992-1995 Wind River Systems, Inc. */

/*
modification history
--------------------
01j,23may01,to   doc fix for frexp()
01j,03jan01,pes  Fix compiler warnings.
01i,04feb99,dgp  document errno values.
01h,19mar95,dvs  removed TRON reference - no longer supported.
01g,10feb95,rhp  update from subsidiary files
01f,16jan95,rhp  library man page: correct spelling of HUGE_VAL
01e,13mar93,jdi  doc tweak.
01d,05feb93,jdi  doc changes based on kdl review.
01c,02dec92,jdi  doc tweaks.
01b,28oct92,jdi  updated with new versions of .c files
01a,24oct92,smb  documentation
*/

/*
DESCRIPTION
The header math.h declares several mathematical functions and defines one
macro.  The functions take double arguments and return double values.

The macro defined is:
.iP `HUGE_VAL' 15
expands to a positive double expression, not necessarily representable
as a float.
.LP

The behavior of each of these functions is defined for all representable
values of their input arguments.  Each function executes as if it were a
single operation, without generating any externally visible exceptions.

For all functions, a domain error occurs if an input argument is outside
the domain over which the mathematical function is defined.  The
description of each function lists any applicable domain errors.  On a
domain error, the function returns an implementation-defined value; the
value EDOM is stored in `errno'.

Similarly, a range error occurs if the result of the function cannot be
represented as a double value.  If the result overflows (the magnitude of
the result is so large that it cannot be represented in an object of the
specified type), the function returns the value HUGE_VAL, with the same
sign (except for the tan() function) as the correct value of the function;
the value ERANGE is stored in `errno'.  If the result underflows (the
type), the function returns zero; whether the integer expression `errno'
acquires the value ERANGE is implementation defined.

INCLUDE FILES: math.h

SEE ALSO: mathALib, American National Standard X3.159-1989

INTERNAL
When generating man pages, the man pages from this library should be
built AFTER those from arch/mc68k/math/mathALib.  Thus, where there are
equivalent man pages in both ansiMath and mathALib, the ones in ansiMath
will overwrite those from mathALib, which is the correct behavior.
This ordering is set up in the overall makefile system.

INTERNAL
This module is built by appending the following files:
    asincos.c
    atan.c
    atan2.c
    ceil.c
    cosh.c
    exp.c
    fabs.c
    floor.c
    fmod.c
    frexp.c
    ldexp.c
    log.c
    log10.c
    modf.c
    pow.c
    sincos.c
    sinh.c
    sqrt.c
    tan.c
    tanh.c
*/


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
/* atan.c - math routines */

/* Copyright 1992-1994 Wind River Systems, Inc. */

/*
modification history
--------------------
01d,09dec94,rhp  fix man pages for inverse trig fns
01e,05feb93,jdi  doc changes based on kdl review.
01d,02dec92,jdi  doc tweaks.
01c,28oct92,jdi  documentation cleanup.
01b,20sep92,smb  documentation additions
01a,08jul92,smb  documentation.
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
* atan - compute an arc tangent (ANSI)
*
* This routine returns the principal value of the arc tangent of <x> in
* double precision (IEEE double, 53 bits).
* If <x> is the tangent of an angle <T>, this function returns <T> 
* (in radians).
*
* INCLUDE FILES: math.h
*
* RETURNS:
* The double-precision arc tangent of <x> in the range [-pi/2,pi/2] radians.
* Special case:	if <x> is NaN, atan() returns <x> itself.
*
* SEE ALSO: mathALib
*
* INTERNAL:
* Coded in C by K.C. Ng, 4/16/85, revised on 6/10/85.
*/

double atan
    (
    double x	/* tangent of an angle */
    )

    {
	double atan2(),one=1.0;
	return(atan2(x,one));
    }
/* atan2.c - math routines */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01e,05feb93,jdi  doc changes based on kdl review.
01d,02dec92,jdi  doc tweaks.
01c,28oct92,jdi  documentation cleanup.
01b,20sep92,smb  documentation additions
01a,08jul92,smb  documentation.
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
SEE ALSO: American National Standard X3.159-1989
NOMANUAL
*/

#include "vxWorks.h"
#include "math.h"

#if defined(vax)||defined(tahoe) 	/* VAX D format */
#ifdef vax
#define _0x(A,B)	0x/**/A/**/B
#else	/* vax */
#define _0x(A,B)	0x/**/B/**/A
#endif	/* vax */
/*static double */
/*athfhi =  4.6364760900080611433E-1  ,*//*Hex  2^ -1   *  .ED63382B0DDA7B */
/*athflo =  1.9338828231967579916E-19 ,*//*Hex  2^-62   *  .E450059CFE92C0 */
/*PIo4   =  7.8539816339744830676E-1  ,*//*Hex  2^  0   *  .C90FDAA22168C2 */
/*at1fhi =  9.8279372324732906796E-1  ,*//*Hex  2^  0   *  .FB985E940FB4D9 */
/*at1flo = -3.5540295636764633916E-18 ,*//*Hex  2^-57   * -.831EDC34D6EAEA */
/*PIo2   =  1.5707963267948966135E0   ,*//*Hex  2^  1   *  .C90FDAA22168C2 */
/*PI     =  3.1415926535897932270E0   ,*//*Hex  2^  2   *  .C90FDAA22168C2 */
/*a1     =  3.3333333333333473730E-1  ,*//*Hex  2^ -1   *  .AAAAAAAAAAAB75 */
/*a2     = -2.0000000000017730678E-1  ,*//*Hex  2^ -2   * -.CCCCCCCCCD946E */
/*a3     =  1.4285714286694640301E-1  ,*//*Hex  2^ -2   *  .92492492744262 */
/*a4     = -1.1111111135032672795E-1  ,*//*Hex  2^ -3   * -.E38E38EBC66292 */
/*a5     =  9.0909091380563043783E-2  ,*//*Hex  2^ -3   *  .BA2E8BB31BD70C */
/*a6     = -7.6922954286089459397E-2  ,*//*Hex  2^ -3   * -.9D89C827C37F18 */
/*a7     =  6.6663180891693915586E-2  ,*//*Hex  2^ -3   *  .8886B4AE379E58 */
/*a8     = -5.8772703698290408927E-2  ,*//*Hex  2^ -4   * -.F0BBA58481A942 */
/*a9     =  5.2170707402812969804E-2  ,*//*Hex  2^ -4   *  .D5B0F3A1AB13AB */
/*a10    = -4.4895863157820361210E-2  ,*//*Hex  2^ -4   * -.B7E4B97FD1048F */
/*a11    =  3.3006147437343875094E-2  ,*//*Hex  2^ -4   *  .8731743CF72D87 */
/*a12    = -1.4614844866464185439E-2  ;*//*Hex  2^ -6   * -.EF731A2F3476D9 */
static long athfhix[] = { _0x(6338,3fed), _0x(da7b,2b0d)};
#define athfhi	(*(double *)athfhix)
static long athflox[] = { _0x(5005,2164), _0x(92c0,9cfe)};
#define athflo	(*(double *)athflox)
static long   PIo4x[] = { _0x(0fda,4049), _0x(68c2,a221)};
#define   PIo4	(*(double *)PIo4x)
static long at1fhix[] = { _0x(985e,407b), _0x(b4d9,940f)};
#define at1fhi	(*(double *)at1fhix)
static long at1flox[] = { _0x(1edc,a383), _0x(eaea,34d6)};
#define at1flo	(*(double *)at1flox)
static long   PIo2x[] = { _0x(0fda,40c9), _0x(68c2,a221)};
#define   PIo2	(*(double *)PIo2x)
static long     PIx[] = { _0x(0fda,4149), _0x(68c2,a221)};
#define     PI	(*(double *)PIx)
static long     a1x[] = { _0x(aaaa,3faa), _0x(ab75,aaaa)};
#define     a1	(*(double *)a1x)
static long     a2x[] = { _0x(cccc,bf4c), _0x(946e,cccd)};
#define     a2	(*(double *)a2x)
static long     a3x[] = { _0x(4924,3f12), _0x(4262,9274)};
#define     a3	(*(double *)a3x)
static long     a4x[] = { _0x(8e38,bee3), _0x(6292,ebc6)};
#define     a4	(*(double *)a4x)
static long     a5x[] = { _0x(2e8b,3eba), _0x(d70c,b31b)};
#define     a5	(*(double *)a5x)
static long     a6x[] = { _0x(89c8,be9d), _0x(7f18,27c3)};
#define     a6	(*(double *)a6x)
static long     a7x[] = { _0x(86b4,3e88), _0x(9e58,ae37)};
#define     a7	(*(double *)a7x)
static long     a8x[] = { _0x(bba5,be70), _0x(a942,8481)};
#define     a8	(*(double *)a8x)
static long     a9x[] = { _0x(b0f3,3e55), _0x(13ab,a1ab)};
#define     a9	(*(double *)a9x)
static long    a10x[] = { _0x(e4b9,be37), _0x(048f,7fd1)};
#define    a10	(*(double *)a10x)
static long    a11x[] = { _0x(3174,3e07), _0x(2d87,3cf7)};
#define    a11	(*(double *)a11x)
static long    a12x[] = { _0x(731a,bd6f), _0x(76d9,2f34)};
#define    a12	(*(double *)a12x)
#else	/* defined(vax)||defined(tahoe) */
static double
athfhi =  4.6364760900080609352E-1    , /*Hex  2^ -2   *  1.DAC670561BB4F */
athflo =  4.6249969567426939759E-18   , /*Hex  2^-58   *  1.5543B8F253271 */
PIo4   =  7.8539816339744827900E-1    , /*Hex  2^ -1   *  1.921FB54442D18 */
at1fhi =  9.8279372324732905408E-1    , /*Hex  2^ -1   *  1.F730BD281F69B */
at1flo = -2.4407677060164810007E-17   , /*Hex  2^-56   * -1.C23DFEFEAE6B5 */
PIo2   =  1.5707963267948965580E0     , /*Hex  2^  0   *  1.921FB54442D18 */
PI     =  3.1415926535897931160E0     , /*Hex  2^  1   *  1.921FB54442D18 */
a1     =  3.3333333333333942106E-1    , /*Hex  2^ -2   *  1.55555555555C3 */
a2     = -1.9999999999979536924E-1    , /*Hex  2^ -3   * -1.9999999997CCD */
a3     =  1.4285714278004377209E-1    , /*Hex  2^ -3   *  1.24924921EC1D7 */
a4     = -1.1111110579344973814E-1    , /*Hex  2^ -4   * -1.C71C7059AF280 */
a5     =  9.0908906105474668324E-2    , /*Hex  2^ -4   *  1.745CE5AA35DB2 */
a6     = -7.6919217767468239799E-2    , /*Hex  2^ -4   * -1.3B0FA54BEC400 */
a7     =  6.6614695906082474486E-2    , /*Hex  2^ -4   *  1.10DA924597FFF */
a8     = -5.8358371008508623523E-2    , /*Hex  2^ -5   * -1.DE125FDDBD793 */
a9     =  4.9850617156082015213E-2    , /*Hex  2^ -5   *  1.9860524BDD807 */
a10    = -3.6700606902093604877E-2    , /*Hex  2^ -5   * -1.2CA6C04C6937A */
a11    =  1.6438029044759730479E-2    ; /*Hex  2^ -6   *  1.0D52174A1BB54 */
#endif	/* defined(vax)||defined(tahoe) */

/*******************************************************************************
*
* atan2 - compute the arc tangent of y/x (ANSI)
*
* This routine returns the principal value of the arc tangent of <y>/<x> in
* double precision (IEEE double, 53 bits).
* This routine uses the signs of both arguments to determine the quadrant of the
* return value.  A domain error may occur if both arguments are zero.
*
* INTERNAL:
* (1) Reduce <y> to positive by:
* 
*     atan2(y,x)=-atan2(-y,x)
* 
* (2) Reduce <x> to positive by (if <x> and <y> are unexceptional):
* 
*     ARG (x+iy) = arctan(y/x)   	 ... if x > 0
*     ARG (x+iy) = pi - arctan[y/(-x)]   ... if x < 0
*
* (3) According to the integer k=4t+0.25 truncated , t=y/x, the argument
*     is further reduced to one of the following intervals and the
*     arc tangent of y/x is evaluated by the corresponding formula:
*
*     [0,7/16]	      atan(y/x) = t - t^3*(a1+t^2*(a2+...(a10+t^2*a11)...)
*     [7/16,11/16]    atan(y/x) = atan(1/2) + atan( (y-x/2)/(x+y/2) )
*     [11/16.19/16]   atan(y/x) = atan( 1 ) + atan( (y-x)/(x+y) )
*     [19/16,39/16]   atan(y/x) = atan(3/2) + atan( (y-1.5x)/(x+1.5y) )
*     [39/16,INF]     atan(y/x) = atan(INF) + atan( -x/y )
*
* INCLUDE FILES: math.h
*
* RETURNS:
* The double-precision arc tangent of <y>/<x>, in the range [-pi,pi] radians.
*
* Special cases:
*     Notations: atan2(y,x) == ARG (x+iy) == ARG(x,y).
* 
* .TS
* tab(|);
* l0 c0 l.
*     ARG(NAN, (anything))                      | is | NaN
*     ARG((anything), NaN)                      | is | NaN
*     ARG(+(anything but NaN), +-0)             | is | +-0
*     ARG(-(anything but NaN), +-0)             | is | +-PI
*     ARG(0, +-(anything but 0 and NaN))        | is | +-PI/2
*     ARG(+INF, +-(anything but INF and NaN))   | is | +-0
*     ARG(-INF, +-(anything but INF and NaN))   | is | +-PI
*     ARG(+INF, +-INF)                          | is | +-PI/4
*     ARG(-INF, +-INF)                          | is | +-3PI/4
*     ARG((anything but 0, NaN, and INF),+-INF) | is | +-PI/2
* .TE
*
* SEE ALSO: mathALib
*
* INTERNAL:
* Coded in C by K.C. Ng, 1/8/85;
* Revised by K.C. Ng on 2/7/85, 2/13/85, 3/7/85, 3/30/85, 6/29/85.
*/

double atan2
    (
    double  y,	/* numerator   */
    double  x	/* denominator */
    )

    {
	static double zero=0, one=1, small=1.0E-9, big=1.0E18;
	double copysign(),logb(),scalb(),t,z,signy,signx,hi,lo;
	int finite(), k,m;

#if !defined(vax)&&!defined(tahoe)
    /* if x or y is NAN */
	if(x!=x) return(x); if(y!=y) return(y);
#endif	/* !defined(vax)&&!defined(tahoe) */

    /* copy down the sign of y and x */
	signy = copysign(one,y) ;
	signx = copysign(one,x) ;

    /* if x is 1.0, goto begin */
	if(x==1) { y=copysign(y,one); t=y; if(finite(t)) goto begin;}

    /* when y = 0 */
	if(y==zero) return((signx==one)?y:copysign(PI,signy));

    /* when x = 0 */
	if(x==zero) return(copysign(PIo2,signy));

    /* when x is INF */
	if(!finite(x))
	    if(!finite(y))
		return(copysign((signx==one)?PIo4:3*PIo4,signy));
	    else
		return(copysign((signx==one)?zero:PI,signy));

    /* when y is INF */
	if(!finite(y)) return(copysign(PIo2,signy));

    /* compute y/x */
	x=copysign(x,one);
	y=copysign(y,one);
	if((m=(k=logb(y))-logb(x)) > 60) t=big+big;
	    else if(m < -80 ) t=y/x;
	    else { t = y/x ; y = scalb(y,-k); x=scalb(x,-k); }

    /* begin argument reduction */
begin:
	if (t < 2.4375) {

	/* truncate 4(t+1/16) to integer for branching */
	    k = 4 * (t+0.0625);
	    switch (k) {

	    /* t is in [0,7/16] */
	    case 0:
	    case 1:
		if (t < small)
		    { big + small ;  /* raise inexact flag */
		      return (copysign((signx>zero)?t:PI-t,signy)); }

		hi = zero;  lo = zero;  break;

	    /* t is in [7/16,11/16] */
	    case 2:
		hi = athfhi; lo = athflo;
		z = x+x;
		t = ( (y+y) - x ) / ( z +  y ); break;

	    /* t is in [11/16,19/16] */
	    case 3:
	    case 4:
		hi = PIo4; lo = zero;
		t = ( y - x ) / ( x + y ); break;

	    /* t is in [19/16,39/16] */
	    default:
		hi = at1fhi; lo = at1flo;
		z = y-x; y=y+y+y; t = x+x;
		t = ( (z+z)-x ) / ( t + y ); break;
	    }
	}
	/* end of if (t < 2.4375) */

	else
	{
	    hi = PIo2; lo = zero;

	    /* t is in [2.4375, big] */
	    if (t <= big)  t = - x / y;

	    /* t is in [big, INF] */
	    else
	      { big+small;	/* raise inexact flag */
		t = zero; }
	}
    /* end of argument reduction */

    /* compute atan(t) for t in [-.4375, .4375] */
	z = t*t;
#if defined(vax)||defined(tahoe)
	z = t*(z*(a1+z*(a2+z*(a3+z*(a4+z*(a5+z*(a6+z*(a7+z*(a8+
			z*(a9+z*(a10+z*(a11+z*a12))))))))))));
#else	/* defined(vax)||defined(tahoe) */
	z = t*(z*(a1+z*(a2+z*(a3+z*(a4+z*(a5+z*(a6+z*(a7+z*(a8+
			z*(a9+z*(a10+z*a11)))))))))));
#endif	/* defined(vax)||defined(tahoe) */
	z = lo - z; z += t; z += hi;

	return(copysign((signx>zero)?z:PI-z,signy));
    }
/* ceil.c - ceil math routine */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01f,05feb93,jdi  doc changes based on kdl review.
01e,02dec92,jdi  doc tweaks.
01d,28oct92,jdi  documentation cleanup.
01c,20sep92,smb  documentation additions
01b,30jul92,kdl  changed _d_type() calls to fpTypeGet().
01a,08jul92,smb  documentation.
*/

/*
DESCRIPTION
SEE ALSO: American National Standard X3.159-1989
NOMANUAL
*/

#include "vxWorks.h"
#include "math.h"
#include "stddef.h"
#include "errno.h"
#include "private/mathP.h"


/*******************************************************************************
*
* ceil - compute the smallest integer greater than or equal to a specified value (ANSI)
*
* This routine returns the smallest integer greater than or equal to <v>,
* in double precision.
*
* INCLUDE FILES: math.h
*
* RETURNS: The smallest integral value greater than or equal to <v>, in
* double precision.
*
* SEE ALSO: mathALib
*/

double ceil
    (
    double v	/* value to find the ceiling of */
    )

    {
    double r;

    switch (fpTypeGet (v, &r))		/* get the type of v */
        {
    	case ZERO:	/* ZERO */
    	case INF:	/* INFINITY */
            return (0.0);

    	case NAN:	/* NOT A NUMBER */
            return (v);

    	case INTEGER:	/* INTEGER */
            return (v);

    	case REAL:	/* REAL */
            return (((v < 0.0) || (v == r)) ? r : r + 1.0);

	default:
	    return (0.0);		/* this should never happen */
        }
    }
/* cosh.c - hyperbolic routines */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01e,05feb93,jdi  doc changes based on kdl review.
01d,02dec92,jdi  doc tweaks.
01c,28oct92,jdi  documentation cleanup.
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
*
SEE ALSO: American National Standard X3.159-1989
NOMANUAL
*/

#include "vxWorks.h"
#include "math.h"

#if defined(vax)||defined(tahoe)
#ifdef vax
#define _0x(A,B)	0x/**/A/**/B
#else	/* vax */
#define _0x(A,B)	0x/**/B/**/A
#endif	/* vax */
/* static double  */
/* mln2hi =  8.8029691931113054792E1     , Hex  2^  7   *  .B00F33C7E22BDB */
/* mln2lo = -4.9650192275318476525E-16   , Hex  2^-50   * -.8F1B60279E582A */
/* lnovfl =  8.8029691931113053016E1     ; Hex  2^  7   *  .B00F33C7E22BDA */
static long    mln2hix[] = { _0x(0f33,43b0), _0x(2bdb,c7e2)};
static long    mln2lox[] = { _0x(1b60,a70f), _0x(582a,279e)};
static long    lnovflx[] = { _0x(0f33,43b0), _0x(2bda,c7e2)};
#define   mln2hi    (*(double*)mln2hix)
#define   mln2lo    (*(double*)mln2lox)
#define   lnovfl    (*(double*)lnovflx)
#else	/* defined(vax)||defined(tahoe) */
static double
mln2hi =  7.0978271289338397310E2     , /*Hex  2^ 10   *  1.62E42FEFA39EF */
mln2lo =  2.3747039373786107478E-14   , /*Hex  2^-45   *  1.ABC9E3B39803F */
lnovfl =  7.0978271289338397310E2     ; /*Hex  2^  9   *  1.62E42FEFA39EF */
#endif	/* defined(vax)||defined(tahoe) */

#if defined(vax)||defined(tahoe)
static max = 126                      ;
#else	/* defined(vax)||defined(tahoe) */
static max = 1023                     ;
#endif	/* defined(vax)||defined(tahoe) */

/*******************************************************************************
*
* cosh - compute a hyperbolic cosine (ANSI)
*
* This routine returns the hyperbolic cosine of <x> in
* double precision (IEEE double, 53 bits).
*
* A range error occurs if <x> is too large.
*
* INTERNAL:
* Method:
* (1) Replace <x> by |x|.
* (2)
*                                                 [ exp(x) - 1 ]^2
*     0        <= x <= 0.3465  :  cosh(x) := 1 + -------------------
*                                                    2*exp(x)
*
*                                            exp(x) +  1/exp(x)
*     0.3465   <= x <= 22      :  cosh(x) := -------------------
*                                                    2
*     22       <= x <= lnovfl  :  cosh(x) := exp(x)/2
*     lnovfl   <= x <= lnovfl+log(2)
*                              :  cosh(x) := exp(x)/2 (avoid overflow)
*     log(2)+lnovfl <  x <  INF:  overflow to INF
*
*         Note: .3465 is a number near one half of ln2.
*
* INCLUDE FILES: math.h
*
* RETURNS:
* The double-precision hyperbolic cosine of <x>.
*
* Special cases:
*     If <x> is +INF, -INF, or NaN, cosh() returns <x>.
*
* SEE ALSO: mathALib
*
* INTERNAL:
* Coded in C by K.C. Ng, 1/8/85;
* Revised by K.C. Ng on 2/8/85, 2/23/85, 3/7/85, 3/29/85, 4/16/85.
*/

double cosh
    (
    double x	/* value to compute the hyperbolic cosine of */
    )

    {
	static double half=1.0/2.0,one=1.0, small=1.0E-18; /* fl(1+small)==1 */
	double scalb(),copysign(),exp(),exp__E(),t;

#if !defined(vax)&&!defined(tahoe)
	if(x!=x) return(x);	/* x is NaN */
#endif	/* !defined(vax)&&!defined(tahoe) */
	if((x=copysign(x,one)) <= 22)
	    if(x<0.3465)
		if(x<small) return(one+x);
		else {t=x+exp__E(x,0.0);x=t+t; return(one+t*t/(2.0+x)); }

	    else /* for x lies in [0.3465,22] */
	        { t=exp(x); return((t+one/t)*half); }

	if( lnovfl <= x && x <= (lnovfl+0.7))
        /* for x lies in [lnovfl, lnovfl+ln2], decrease x by ln(2^(max+1))
         * and return 2^max*exp(x) to avoid unnecessary overflow
         */
	    return(scalb(exp((x-mln2hi)-mln2lo), max));

	else
	    return(exp(x)*half);	/* for large x,  cosh(x)=exp(x)/2 */
    }
/* exp.c - math routines */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01e,05feb93,jdi  doc changes based on kdl review.
01d,02dec92,jdi  doc tweaks.
01c,28oct92,jdi  documentation cleanup.
01b,20sep92,smb  documentation additions
01a,08jul92,smb  documentation.
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
/* ln2hi  =  6.9314718055829871446E-1    , Hex  2^  0   *  .B17217F7D00000 */
/* ln2lo  =  1.6465949582897081279E-12   , Hex  2^-39   *  .E7BCD5E4F1D9CC */
/* lnhuge =  9.4961163736712506989E1     , Hex  2^  7   *  .BDEC1DA73E9010 */
/* lntiny = -9.5654310917272452386E1     , Hex  2^  7   * -.BF4F01D72E33AF */
/* invln2 =  1.4426950408889634148E0     ; Hex  2^  1   *  .B8AA3B295C17F1 */
/* p1     =  1.6666666666666602251E-1    , Hex  2^-2    *  .AAAAAAAAAAA9F1 */
/* p2     = -2.7777777777015591216E-3    , Hex  2^-8    * -.B60B60B5F5EC94 */
/* p3     =  6.6137563214379341918E-5    , Hex  2^-13   *  .8AB355792EF15F */
/* p4     = -1.6533902205465250480E-6    , Hex  2^-19   * -.DDEA0E2E935F84 */
/* p5     =  4.1381367970572387085E-8    , Hex  2^-24   *  .B1BB4B95F52683 */
static long     ln2hix[] = { _0x(7217,4031), _0x(0000,f7d0)};
static long     ln2lox[] = { _0x(bcd5,2ce7), _0x(d9cc,e4f1)};
static long    lnhugex[] = { _0x(ec1d,43bd), _0x(9010,a73e)};
static long    lntinyx[] = { _0x(4f01,c3bf), _0x(33af,d72e)};
static long    invln2x[] = { _0x(aa3b,40b8), _0x(17f1,295c)};
static long        p1x[] = { _0x(aaaa,3f2a), _0x(a9f1,aaaa)};
static long        p2x[] = { _0x(0b60,bc36), _0x(ec94,b5f5)};
static long        p3x[] = { _0x(b355,398a), _0x(f15f,792e)};
static long        p4x[] = { _0x(ea0e,b6dd), _0x(5f84,2e93)};
static long        p5x[] = { _0x(bb4b,3431), _0x(2683,95f5)};
#define    ln2hi    (*(double*)ln2hix)
#define    ln2lo    (*(double*)ln2lox)
#define   lnhuge    (*(double*)lnhugex)
#define   lntiny    (*(double*)lntinyx)
#define   invln2    (*(double*)invln2x)
#define       p1    (*(double*)p1x)
#define       p2    (*(double*)p2x)
#define       p3    (*(double*)p3x)
#define       p4    (*(double*)p4x)
#define       p5    (*(double*)p5x)

#else	/* defined(vax)||defined(tahoe) */
static double
p1     =  1.6666666666666601904E-1    , /*Hex  2^-3    *  1.555555555553E */
p2     = -2.7777777777015593384E-3    , /*Hex  2^-9    * -1.6C16C16BEBD93 */
p3     =  6.6137563214379343612E-5    , /*Hex  2^-14   *  1.1566AAF25DE2C */
p4     = -1.6533902205465251539E-6    , /*Hex  2^-20   * -1.BBD41C5D26BF1 */
p5     =  4.1381367970572384604E-8    , /*Hex  2^-25   *  1.6376972BEA4D0 */
ln2hi  =  6.9314718036912381649E-1    , /*Hex  2^ -1   *  1.62E42FEE00000 */
ln2lo  =  1.9082149292705877000E-10   , /*Hex  2^-33   *  1.A39EF35793C76 */
lnhuge =  7.1602103751842355450E2     , /*Hex  2^  9   *  1.6602B15B7ECF2 */
lntiny = -7.5137154372698068983E2     , /*Hex  2^  9   * -1.77AF8EBEAE354 */
invln2 =  1.4426950408889633870E0     ; /*Hex  2^  0   *  1.71547652B82FE */
#endif	/* defined(vax)||defined(tahoe) */

/*****************************************************************************
*
* exp - compute an exponential value (ANSI)
*
* This routine returns the exponential value of <x> in
* double precision (IEEE double, 53 bits).
*
* A range error occurs if <x> is too large.
*
* INTERNAL:
* Method:
* (1) Argument Reduction: given the input <x>, find <r> and integer <k>
*     such that:
*
*         x = k*ln2 + r,  |r| <= 0.5*ln2
* 
*     <r> will be represented as r := z+c for better accuracy.
* 
* (2) Compute exp(r) by
*
*         exp(r) = 1 + r + r*R1/(2-R1)
*
*     where:
*
*         R1 = x - x^2*(p1+x^2*(p2+x^2*(p3+x^2*(p4+p5*x^2))))
*
* (3)     exp(x) = 2^k * exp(r)
*
* INCLUDE FILES: math.h
*
* RETURNS: The double-precision exponential value of <x>.
*
* Special cases:
*     If <x> is +INF or NaN, exp() returns <x>.
*     If <x> is -INF, it returns 0.
*
* SEE ALSO: mathALib
*
* INTERNAL:
* Coded in C by K.C. Ng, 1/19/85;
* Revised by K.C. Ng on 2/6/85, 2/15/85, 3/7/85, 3/24/85, 4/16/85, 6/14/86.
*/

double exp
    (
    double x	/* exponent */
    )

    {
	double scalb(), copysign(), z,hi,lo,c;
	int k,finite();

#if !defined(vax)&&!defined(tahoe)
	if(x!=x) return(x);	/* x is NaN */
#endif	/* !defined(vax)&&!defined(tahoe) */
	if( x <= lnhuge ) {
		if( x >= lntiny ) {

		    /* argument reduction : x --> x - k*ln2 */

			k=invln2*x+copysign(0.5,x);	/* k=NINT(x/ln2) */

		    /* express x-k*ln2 as hi-lo and let x=hi-lo rounded */

			hi=x-k*ln2hi;
			x=hi-(lo=k*ln2lo);

		    /* return 2^k*[1+x+x*c/(2+c)]  */
			z=x*x;
			c= x - z*(p1+z*(p2+z*(p3+z*(p4+z*p5))));
			return  scalb(1.0+(hi-(lo-(x*c)/(2.0-c))),k);

		}
		/* end of x > lntiny */

		else
		     /* exp(-big#) underflows to zero */
		     if(finite(x))  return(scalb(1.0,-5000));

		     /* exp(-INF) is zero */
		     else return(0.0);
	}
	/* end of x < lnhuge */

	else
	/* exp(INF) is INF, exp(+big#) overflows to INF */
	    return( finite(x) ?  scalb(1.0,5000)  : x);
    }
/* fabs.c - fabs math routine */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01f,05feb93,jdi  doc changes based on kdl review.
01e,02dec92,jdi  doc tweaks.
01d,28oct92,jdi  documentation cleanup.
01c,20sep92,smb  documentation additions
01b,30jul92,kdl  changed _d_type() calls to fpTypeGet().
01a,08jul92,smb  documentation.
*/

/*
DESCRIPTION
SEE ALSO: American National Standard X3.159-1989
NOMANUAL
*/

#include "vxWorks.h"
#include "math.h"
#include "stddef.h"
#include "errno.h"
#include "private/mathP.h"


/*******************************************************************************
*
* fabs - compute an absolute value (ANSI)
*
* This routine returns the absolute value of <v> in double precision.
*
* INCLUDE FILES: math.h
*
* RETURNS: The double-precision absolute value of <v>.
*
* ERRNO: EDOM, ERANGE
*
* SEE ALSO: mathALib
*/

double fabs
    (
    double v	/* number to return the absolute value of */
    )

    {
    switch (fpTypeGet (v, NULL))
	{
	case ZERO:			/* ZERO */
	    return (0.0);

	case NAN:			/* NOT  A NUMBER */
	    errno = EDOM;
	    return (v);

	case INF:			/* INFINITE */
	    errno = ERANGE;

	case INTEGER:			/* INTEGER */
	case REAL:			/* REAL */
	    return ((v < 0.0) ? - v : v);

	default:
	    return (0.0);
	}
    }
/* floor.c - floor math routine */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01f,05feb93,jdi  doc changes based on kdl review.
01e,02dec92,jdi  doc tweaks.
01d,28oct92,jdi  documentation cleanup.
01c,20sep92,smb  documentation additions
01b,30jul92,kdl  changed _d_type() calls to fpTypeGet().
01a,08jul92,smb  documentation.
*/

/*
DESCRIPTION
SEE ALSO: American National Standard X3.159-1989
NOMANUAL
*/

#include "vxWorks.h"
#include "math.h"
#include "stddef.h"
#include "errno.h"
#include "private/mathP.h"


/*******************************************************************************
*
* floor - compute the largest integer less than or equal to a specified value (ANSI)
*
* This routine returns the largest integer less than or equal to <v>, in
* double precision.
* 
* INCLUDE FILES: math.h
*
* RETURNS:
* The largest integral value less than or equal to <v>, in double precision.
*
* SEE ALSO: mathALib
*/

double floor
    (
    double v	/* value to find the floor of */
    )

    {
    double r;

    switch (fpTypeGet (v, &r))	/* find out the type of v */
	{
	case ZERO:
	case INF:	return (0);

	case NAN:	return (v);

	case INTEGER:	return (v);

	case REAL:	return (((v < 0.0) && (v != r)) ? (r - 1.0) : (r));

	default:	return (0);		/* this should never happen */
	}
    }
/* fmod.c - fmod math routine */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01f,05feb93,jdi  doc changes based on kdl review.
01e,02dec92,jdi  doc tweaks.
01d,28oct92,jdi  documentation cleanup.
01c,20sep92,smb  documentation additions
01b,30jul92,kdl  changed _d_type() calls to fpTypeGet().
01a,08jul92,smb  documentation.
*/

/*
DESCRIPTION
SEE ALSO: American National Standard X3.159-1989
NOMANUAL
*/

#include "vxWorks.h"
#include "math.h"
#include "stddef.h"
#include "errno.h"
#include "private/mathP.h"


/*******************************************************************************
*
* fmod - compute the remainder of x/y (ANSI)
*
* This routine returns the remainder of <x>/<y> with the sign of <x>,
* in double precision.
* 
* INCLUDE FILES: math.h
*
* RETURNS: The value <x> - <i> * <y>, for some integer <i>.  If <y> is
* non-zero, the result has the same sign as <x> and magnitude less than the
* magnitude of <y>.  If <y> is zero, fmod() returns zero.
*
* ERRNO: EDOM
*
* SEE ALSO: mathALib
*/

double fmod
    (
    double x,	/* numerator   */
    double y	/* denominator */
    )

    {
    double	t;
    short	negative = 0;
    int		errx = fpTypeGet (x, NULL);	/* determine number type */
    int		erry = fpTypeGet (y, NULL);	/* determine number type */

    if (errx == NAN || erry == NAN || errx == INF || erry == ZERO)
        {
        errno = EDOM;				/* Domain error */
        return ((errx == NAN) ? (x) : ((erry == NAN) ? (y) : (0)));
        }
    
    if (errx == ZERO || erry == INF)
	return (x);		

    /* make x and y absolute */

    if (y < 0.0)
	y = -y;

    if (x < 0.0)
	{
	x = -x;
	negative = 1;
	}

    /* loop substracting y from x until a value less than y remains */

    for (t = x; t > y; t -= y)
        ;

    return ((t == y) ? (0.0) : ((negative) ? (-t) : (t)));
    }
/* frexp.c - frexp math routine */

/* Copyright 1992-1996 Wind River Systems, Inc. */

/*
modification history
--------------------
01h,05aug96,dbt  frexp now returned a value >= 0.5 and strictly less than 1.0.
		 (SPR #4280).
		 Updated copyright.
01g,05feb93,jdi  doc changes based on kdl review.
01f,02dec92,jdi  doc tweaks.
01e,28oct92,jdi  documentation cleanup.
01d,21sep92,smb  replaced orignal versions.
01c,20sep92,smb  documentation additions
01b,30jul92,kdl  replaced routine contents with frexp() from floatLib.c.
01a,08jul92,smb  written, documentation.
*/

/*
DESCRIPTION

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "math.h"
#include "stddef.h"
#include "errno.h"
#include "private/mathP.h"

/*******************************************************************************
*
* frexp - break a floating-point number into a normalized fraction and power of 2 (ANSI)
*
* This routine breaks a double-precision number <value> into a normalized
* fraction and integral power of 2.  It stores the integer exponent in <pexp>.
*
* INCLUDE FILES: math.h
*
* RETURNS: 
* The double-precision value <x>, such that the magnitude of <x> is
* in the interval [1/2,1) or zero, and <value> equals <x> times
* 2 to the power of <pexp>. If <value> is zero, both parts of the result
* are zero.
*
* ERRNO: EDOM
*
*/
double frexp
    (
    double value,	/* number to be normalized */
    int *pexp   	/* pointer to the exponent */
    )

    {
    double r;

    *pexp = 0;

    /* determine number type */

    switch (fpTypeGet(value, &r))	
      	{
        case NAN:		/* NOT A NUMBER */
    	case INF:		/* INFINITE */
            errno = EDOM;
	    return (value);

    	case ZERO:		/* ZERO */
	    return (0.0);

	default:
	    /*
	     * return value must be strictly less than 1.0 and >=0.5 .
	     */
	    if ((r = fabs(value)) >= 1.0)
	        for(; (r >= 1.0); (*pexp)++, r /= 2.0);
	    else
	        for(; (r < 0.5); (*pexp)--, r *= 2.0);

	    return (value < 0 ? -r : r);
        }
    }
/* ldexp.c - ldexp math routine */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01g,05feb93,jdi  doc changes based on kdl review.
01f,02dec92,jdi  doc tweaks.
01e,28oct92,jdi  documentation cleanup.
01d,21sep92,smb  replaced original version.
01c,20sep92,smb  documentation additions
01b,30jul92,kdl  replaced routine contents with ldexp() from floatLib.c.
01a,08jul92,smb  written, documentation.
*/

/*
DESCRIPTION

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "math.h"
#include "stddef.h"
#include "errno.h"
#include "private/mathP.h"


/*******************************************************************************
*
* ldexp - multiply a number by an integral power of 2 (ANSI)
*
* This routine multiplies a floating-point number by an integral power of 2.
* A range error may occur.
*
* INCLUDE FILES: math.h
*
* RETURNS: The double-precision value of <v> times 2 to the power of <xexp>.
*
*/

double ldexp
    (
    double v,		/* a floating point number */
    int xexp		/* exponent                */
    )

    {
    double zero = 0.0;

    switch (fpTypeGet (v, NULL))	
       	{
       	case NAN:		/* NOT A NUMBER */
            errno = EDOM;
	    break;

    	case INF:		/* INFINITE */
	    errno = ERANGE;
	    break;

    	case ZERO:		/* ZERO */
	    return (zero);

	default:
	    if (xexp >= 0)
		for(; (xexp > 0); xexp--, v *= 2.0);
	    else
		for(; (xexp < 0); xexp++, v /= 2.0);

	    return (v);
    	}

    return (zero);
    }
/* log.c - math routines */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01f,05feb93,jdi  doc changes based on kdl review.
01e,02dec92,jdi  doc tweaks.
01d,28oct92,jdi  documentation cleanup.
01c,13oct92,jdi  mangen fixes.
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
*
SEE ALSO: American National Standard X3.159-1989
NOMANUAL
*/

#include "vxWorks.h"
#include "math.h"

#if defined(vax)||defined(tahoe)	/* VAX D format */
#include <errno.h>
#ifdef vax
#define _0x(A,B)	0x/**/A/**/B
#else	/* vax */
#define _0x(A,B)	0x/**/B/**/A
#endif	/* vax */
/* static double */
/* ln2hi  =  6.9314718055829871446E-1    , Hex  2^  0   *  .B17217F7D00000 */
/* ln2lo  =  1.6465949582897081279E-12   , Hex  2^-39   *  .E7BCD5E4F1D9CC */
/* sqrt2  =  1.4142135623730950622E0     ; Hex  2^  1   *  .B504F333F9DE65 */
static long     ln2hix[] = { _0x(7217,4031), _0x(0000,f7d0)};
static long     ln2lox[] = { _0x(bcd5,2ce7), _0x(d9cc,e4f1)};
static long     sqrt2x[] = { _0x(04f3,40b5), _0x(de65,33f9)};
#define    ln2hi    (*(double*)ln2hix)
#define    ln2lo    (*(double*)ln2lox)
#define    sqrt2    (*(double*)sqrt2x)
#else	/* defined(vax)||defined(tahoe) */
static double
ln2hi  =  6.9314718036912381649E-1    , /*Hex  2^ -1   *  1.62E42FEE00000 */
ln2lo  =  1.9082149292705877000E-10   , /*Hex  2^-33   *  1.A39EF35793C76 */
sqrt2  =  1.4142135623730951455E0     ; /*Hex  2^  0   *  1.6A09E667F3BCD */
#endif	/* defined(vax)||defined(tahoe) */

/*******************************************************************************
*
* log - compute a natural logarithm (ANSI)
*
* This routine returns the natural logarithm of <x>
* in double precision (IEEE double, 53 bits).
*
* A domain error occurs if the argument is negative.  A range error may occur
* if the argument is zero.
*
* INTERNAL:
* Method:
* (1) Argument Reduction: find <k> and <f> such that:
*
*         x = 2^k * (1+f)
*
*     where:
*
*         sqrt(2)/2 < 1+f < sqrt(2)
*
* (2) Let s = f/(2+f); based on:
*
*         log(1+f) = log(1+s) - log(1-s) = 2s + 2/3 s**3 + 2/5 s**5 + .....
*
*     log(1+f) is computed by:
*
*         log(1+f) = 2s + s*log__L(s*s)
*
*     where:
*
*        log__L(z) = z*(L1 + z*(L2 + z*(... (L6 + z*L7)...)))
*
* (3) Finally:
*
*         log(x) = k*ln2 + log(1+f)
*
*     (Here n*ln2 will be stored
*     in two floating-point numbers: n*ln2hi + n*ln2lo; n*ln2hi is exact
*     since the last 20 bits of ln2hi is 0.)
*
* INCLUDE FILES: math.h
*
* RETURNS: The double-precision natural logarithm of <x>.
*
* Special cases:
*     If <x> < 0 (including -INF), it returns NaN with signal.
*     If <x> is +INF, it returns <x> with no signal.
*     If <x> is 0, it returns -INF with signal.
*     If <x> is NaN it returns <x> with no signal.
*
* SEE ALSO: mathALib
*
* INTERNAL
* Coded in C by K.C. Ng, 1/19/85;
* Revised by K.C. Ng on 2/7/85, 3/7/85, 3/24/85, 4/16/85.
*/

double log
    (
    double x	/* value to compute the natural logarithm of */
    )

    {
	static double zero=0.0, negone= -1.0, half=1.0/2.0;
	double logb(),scalb(),copysign(),log__L(),s,z,t;
	int k,n,finite();

#if !defined(vax)&&!defined(tahoe)
	if(x!=x) return(x);	/* x is NaN */
#endif	/* !defined(vax)&&!defined(tahoe) */
	if(finite(x)) {
	   if( x > zero ) {

	   /* argument reduction */
	      k=logb(x);   x=scalb(x,-k);
	      if(k == -1022) /* subnormal no. */
		   {n=logb(x); x=scalb(x,-n); k+=n;}
	      if(x >= sqrt2 ) {k += 1; x *= half;}
	      x += negone ;

	   /* compute log(1+x)  */
              s=x/(2+x); t=x*x*half;
	      z=k*ln2lo+s*(t+log__L(s*s));
	      x += (z - t) ;

	      return(k*ln2hi+x);
	   }
	/* end of if (x > zero) */

	   else {
#if defined(vax)||defined(tahoe)
		extern double infnan();
		if ( x == zero )
		    return (infnan(-ERANGE));	/* -INF */
		else
		    return (infnan(EDOM));	/* NaN */
#else	/* defined(vax)||defined(tahoe) */
		/* zero argument, return -INF with signal */
		if ( x == zero )
		    return( negone/zero );

		/* negative argument, return NaN with signal */
		else
		    return ( zero / zero );
#endif	/* defined(vax)||defined(tahoe) */
	    }
	}
    /* end of if (finite(x)) */
    /* NOTREACHED if defined(vax)||defined(tahoe) */

    /* log(-INF) is NaN with signal */
	else if (x<0)
	    return(zero/zero);

    /* log(+INF) is +INF */
	else return(x);

    }
/* log10.c - math routine */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01e,05feb93,jdi  doc changes based on kdl review.
01d,02dec92,jdi  doc tweaks.
01c,28oct92,jdi  documentation cleanup.
01b,20sep92,smb  documentation additions
01a,08jul92,smb  documentation.
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
SEE ALSO: American National Standard X3.159-1989
NOMANUAL
*/

#include "vxWorks.h"
#include "math.h"

#if defined(vax)||defined(tahoe)	/* VAX D format (56 bits) */
#ifdef vax
#define _0x(A,B)	0x/**/A/**/B
#else	/* vax */
#define _0x(A,B)	0x/**/B/**/A
#endif	/* vax */
/* static double */
/* ln10hi =  2.3025850929940456790E0     ; Hex   2^  2   *  .935D8DDDAAA8AC */
static long    ln10hix[] = { _0x(5d8d,4113), _0x(a8ac,ddaa)};
#define   ln10hi    (*(double*)ln10hix)
#else	/* defined(vax)||defined(tahoe) */
static double
ivln10 =  4.3429448190325181667E-1    ; /*Hex   2^ -2   *  1.BCB7B1526E50E */
#endif	/* defined(vax)||defined(tahoe) */

/*******************************************************************************
*
* log10 - compute a base-10 logarithm (ANSI)
*
* This routine returns the base 10 logarithm of <x> in
* double precision (IEEE double, 53 bits).
*
* A domain error occurs if the argument is negative.  A range error may
* if the argument is zero.
*
* INTERNAL:
* Method:
*                  log(x)
*     log10(x) = ---------  or  [1/log(10)]*log(x)
*                 log(10)
*
*     [log(10)]   rounded to 56 bits has error  .0895  ulps,
*     [1/log(10)] rounded to 53 bits has error  .198   ulps;
*     therefore, for better accuracy, in VAX D format, we divide
*     log(x) by log(10), but in IEEE Double format, we multiply
*     log(x) by [1/log(10)].
*
* INCLUDE FILES: math.h
*
* RETURNS: The double-precision base-10 logarithm of <x>.
*
* Special cases:
*     If <x> < 0, log10() returns NaN with signal.
*     if <x> is +INF, it returns <x> with no signal.
*     if <x> is 0, it returns -INF with signal.
*     if <x> is NaN it returns <x> with no signal.
*
* SEE ALSO: mathALib
*
* INTERNAL:
* Coded in C by K.C. Ng, 1/20/85;
* Revised by K.C. Ng on 1/23/85, 3/7/85, 4/16/85.
*/

double log10
    (
    double x	/* value to compute the base-10 logarithm of */
    )

    {
	double log();

#if defined(vax)||defined(tahoe)
	return(log(x)/ln10hi);
#else	/* defined(vax)||defined(tahoe) */
	return(ivln10*log(x));
#endif	/* defined(vax)||defined(tahoe) */
    }
/* modf.c - separate floating-point number into integer and fraction parts */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01i,05feb93,jdi  doc changes based on kdl review.
01h,02dec92,jdi  doc tweaks.
01g,28oct92,jdi  documentation cleanup.
01f,20sep92,smb  documentation additions
01e,10sep92,wmd  changed dummy function for i960KB from printf to bcopy.
01d,04sep92,wmd  restored to rev 1b, instead modified mathP.h using  _BYTE_ORDER 
		 conditionals to flip the contents of struct DOUBLE.  Added a dummy
		 printf() in order to force compiler to compile code correctly for i960kb.
01c,03sep92,wmd  modified modf() for the i960, word order for DOUBLE is reversed.
01b,25jul92,kdl	 replaced modf() routine contents with version from floatLib.c;
		 deleted isnan(), iszero(), isinf(), _d_type().
01a,08jul92,smb  documentation.
*/

/*
DESCRIPTION

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "math.h"
#include "stddef.h"
#include "errno.h"
#include "private/mathP.h"



/*******************************************************************************
*
* modf - separate a floating-point number into integer and fraction parts (ANSI)
*
* This routine stores the integer portion of <value>
* in <pIntPart> and returns the fractional portion.
* Both parts are double precision and will have the same sign as <value>.
*
* INCLUDE FILES: math.h
*
* RETURNS: The double-precision fractional portion of <value>.
*
* SEE ALSO: frexp(), ldexp()
*/

double modf
    (
    double value,               /* value to split                  */
    double *pIntPart            /* where integer portion is stored */
    )
    {
    DOUBLE 	dat;
    FAST int 	exp;
    FAST double fracPart;
    FAST double intPart;
    int 	negflag = (value < 0);

    if (negflag)
        value = -value;		/* make it positive */

    dat.ddat = value;

    /* Separate the exponent, and change it from biased to 2's comp. */
    exp = ((dat.ldat.l1 & 0x7ff00000) >> 20) - 1023;

#if CPU==I960KB
    bcopy ((char *)&negflag, (char *)&negflag, 0);
#endif  /* CPU==I960KB - to force gcc960 to compile correct code for the i960kb */
    
    if (exp <= -1)
        {
	/* If exponent is negative, fracPart == |value|, and intPart == 0. */

        fracPart = value;
        intPart = 0.;
        }

    /* clear the fractional part in dat */

    else if (exp <= 20)
        {
        dat.ldat.l1 &= (-1 << (20 - exp));
        dat.ldat.l2 = 0;
        intPart = dat.ddat;
        fracPart = value - intPart;
        }

    else if (exp <= 52)
        {
        dat.ldat.l2 &= (-1 << (52 - exp));
        intPart = dat.ddat;
        fracPart = value - intPart;
	}

    else
        {
        fracPart = 0.;
        intPart = value;
        }

    *pIntPart = (negflag ? -intPart : intPart);
    return (negflag ? -fracPart : fracPart);
    }

/* pow.c - math routines */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01e,05feb93,jdi  doc changes based on kdl review.
01d,02dec92,jdi  doc tweaks.
01c,28oct92,jdi  documentation cleanup.
01b,20sep92,smb  documentation additions
01a,08jul92,smb  documentation.
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
SEE ALSO: American National Standard X3.159-1989
NOMANUAL
*/

#include "vxWorks.h"
#include "math.h"
#include "private/mathP.h"

#if defined(vax)||defined(tahoe)	/* VAX D format */
#include <errno.h>
extern double infnan();
#ifdef vax
#define _0x(A,B)	0x/**/A/**/B
#else	/* vax */
#define _0x(A,B)	0x/**/B/**/A
#endif	/* vax */
/* static double */
/* ln2hi  =  6.9314718055829871446E-1    , Hex  2^  0   *  .B17217F7D00000 */
/* ln2lo  =  1.6465949582897081279E-12   , Hex  2^-39   *  .E7BCD5E4F1D9CC */
/* invln2 =  1.4426950408889634148E0     , Hex  2^  1   *  .B8AA3B295C17F1 */
/* sqrt2  =  1.4142135623730950622E0     ; Hex  2^  1   *  .B504F333F9DE65 */
static long     ln2hix[] = { _0x(7217,4031), _0x(0000,f7d0)};
static long     ln2lox[] = { _0x(bcd5,2ce7), _0x(d9cc,e4f1)};
static long    invln2x[] = { _0x(aa3b,40b8), _0x(17f1,295c)};
static long     sqrt2x[] = { _0x(04f3,40b5), _0x(de65,33f9)};
#define    ln2hi    (*(double*)ln2hix)
#define    ln2lo    (*(double*)ln2lox)
#define   invln2    (*(double*)invln2x)
#define    sqrt2    (*(double*)sqrt2x)
#else	/* defined(vax)||defined(tahoe) */
static double
ln2hi  =  6.9314718036912381649E-1    , /*Hex  2^ -1   *  1.62E42FEE00000 */
ln2lo  =  1.9082149292705877000E-10   , /*Hex  2^-33   *  1.A39EF35793C76 */
invln2 =  1.4426950408889633870E0     , /*Hex  2^  0   *  1.71547652B82FE */
sqrt2  =  1.4142135623730951455E0     ; /*Hex  2^  0   *  1.6A09E667F3BCD */
#endif	/* defined(vax)||defined(tahoe) */

static double zero=0.0, half=1.0/2.0, one=1.0, two=2.0, negone= -1.0;

/*******************************************************************************
*
* pow - compute the value of a number raised to a specified power (ANSI)
*
* This routine returns <x> to the power of <y> in
* double precision (IEEE double, 53 bits).
*
* A domain error occurs if <x> is negative and <y> is not an integral value.
* A domain error occurs if the result cannot be represented when <x> is zero
* and <y> is less than or equal to zero.  A range error may occur.
*
* INTERNAL:
* Method:
* (1) Compute and return log(x) in three pieces:
*
*         log(x) = n*ln2 + hi + lo
*
*     where <n> is an integer.
*
* (2) Perform y*log(x) by simulating multi-precision arithmetic and
*     return the answer in three pieces:
*
*         y*log(x) = m*ln2 + hi + lo
*
*     where <m> is an integer.
*
* (3) Return:
*
*         x**y = exp(y*log(x)) = 2^m * ( exp(hi+lo) )
*
* INCLUDE FILES: math.h
*
* RETURNS: The double-precision value of <x> to the power of <y>.
*
* Special cases:
* .TS
* tab(|);
* l0 c0 l.
*     (anything) ** 0                       | is | 1
*     (anything) ** 1                       | is | itself
*     (anything) ** NaN                     | is | NaN
*     NaN ** (anything except 0)            | is | NaN
*     +-(anything > 1) ** +INF              | is | +INF
*     +-(anything > 1) ** -INF              | is | +0
*     +-(anything < 1) ** +INF              | is | +0
*     +-(anything < 1) ** -INF              | is | +INF
*     +-1 ** +-INF                          | is | NaN, signal INVALID
*     +0 ** +(anything non-0, NaN)          | is | +0
*     -0 ** +(anything non-0, NaN, odd int) | is | +0
*     +0 ** -(anything non-0, NaN)          | is | +INF, signal DIV-BY-ZERO
*     -0 ** -(anything non-0, NaN, odd int) | is | +INF with signal
*     -0 ** (odd integer)                   | =  | -(+0 ** (odd integer))
*     +INF ** +(anything except 0, NaN)     | is | +INF
*     +INF ** -(anything except 0, NaN)     | is | +0
*     -INF ** (odd integer)                 | =  | -(+INF ** (odd integer))
*     -INF ** (even integer)                | =  | (+INF ** (even integer))
*     -INF ** -(any non-integer, NaN)       | is | NaN with signal
*     -(x=anything) ** (k=integer)          | is | (-1)**k * (x ** k)
*     -(anything except 0) ** (non-integer) | is | NaN with signal
* .TE
*
* SEE ALSO: mathALib
*
* INTERNAL:
* Coded in C by K.C. Ng, 1/8/85;
* Revised by K.C. Ng on 7/10/85.
*/

double pow
    (
    double x,	/* operand  */
    double y	/* exponent */
    )

    {
	double drem(),pow_p(),copysign(),t;
	int finite();

	if     (y==zero)      return(one);
	else if(y==one
#if !defined(vax)&&!defined(tahoe)
		||x!=x
#endif	/* !defined(vax)&&!defined(tahoe) */
		) return( x );      /* if x is NaN or y=1 */
#if !defined(vax)&&!defined(tahoe)
	else if(y!=y)         return( y );      /* if y is NaN */
#endif	/* !defined(vax)&&!defined(tahoe) */
	else if(!finite(y))                     /* if y is INF */
	     if((t=copysign(x,one))==one) return(zero/zero);
	     else if(t>one) return((y>zero)?y:zero);
	     else return((y<zero)?-y:zero);
	else if(y==two)       return(x*x);
	else if(y==negone)    return(one/x);

    /* sign(x) = 1 */
	else if(copysign(one,x)==one) return(pow_p(x,y));

    /* sign(x)= -1 */
	/* if y is an even integer */
	else if ( (t=drem(y,two)) == zero)	return( pow_p(-x,y) );

	/* if y is an odd integer */
	else if (copysign(t,one) == one) return( -pow_p(-x,y) );

	/* Henceforth y is not an integer */
	else if(x==zero)	/* x is -0 */
	    return((y>zero)?-x:one/(-x));
	else {			/* return NaN */
#if defined(vax)||defined(tahoe)
	    return (infnan(EDOM));	/* NaN */
#else	/* defined(vax)||defined(tahoe) */
	    return(zero/zero);
#endif	/* defined(vax)||defined(tahoe) */
	}
    }

/****************************************************************************
*
* pow_p -
*
* pow_p(x,y) return x**y for x with sign=1 and finite y *
*
* RETURN:
* NOMANUAL
*/

double pow_p(x,y)
double x,y;
{
        double logb(),scalb(),copysign(),log__L(),exp__E();
        double c,s,t,z,tx,ty;
#ifdef tahoe
	double tahoe_tmp;
#endif	/* tahoe */
        float sx,sy;
	long k=0;
        int n,m;

	if(x==zero||!finite(x)) {           /* if x is +INF or +0 */
#if defined(vax)||defined(tahoe)
	     return((y>zero)?x:infnan(ERANGE));	/* if y<zero, return +INF */
#else	/* defined(vax)||defined(tahoe) */
	     return((y>zero)?x:one/x);
#endif	/* defined(vax)||defined(tahoe) */
	}
	if(x==1.0) return(x);	/* if x=1.0, return 1 since y is finite */

    /* reduce x to z in [sqrt(1/2)-1, sqrt(2)-1] */
        z=scalb(x,-(n=logb(x)));
#if !defined(vax)&&!defined(tahoe)	/* IEEE double; subnormal number */
        if(n <= -1022) {n += (m=logb(z)); z=scalb(z,-m);}
#endif	/* !defined(vax)&&!defined(tahoe) */
        if(z >= sqrt2 ) {n += 1; z *= half;}  z -= one ;

    /* log(x) = nlog2+log(1+z) ~ nlog2 + t + tx */
	s=z/(two+z); c=z*z*half; tx=s*(c+log__L(s*s));
	t= z-(c-tx); tx += (z-t)-c;

   /* if y*log(x) is neither too big nor too small */
	if((s=logb(y)+logb(n+t)) < 12.0)
	    if(s>-60.0) {

	/* compute y*log(x) ~ mlog2 + t + c */
        	s=y*(n+invln2*t);
                m=s+copysign(half,s);   /* m := nint(y*log(x)) */
		k=y;
		if((double)k==y) {	/* if y is an integer */
		    k = m-k*n;
		    sx=t; tx+=(t-sx); }
		else	{		/* if y is not an integer */
		    k =m;
	 	    tx+=n*ln2lo;
		    sx=(c=n*ln2hi)+t; tx+=(c-sx)+t; }
	   /* end of checking whether k==y */

                sy=y; ty=y-sy;          /* y ~ sy + ty */
#ifdef tahoe
		s = (tahoe_tmp = sx)*sy-k*ln2hi;
#else	/* tahoe */
		s=(double)sx*sy-k*ln2hi;        /* (sy+ty)*(sx+tx)-kln2 */
#endif	/* tahoe */
		z=(tx*ty-k*ln2lo);
		tx=tx*sy; ty=sx*ty;
		t=ty+z; t+=tx; t+=s;
		c= -((((t-s)-tx)-ty)-z);

	    /* return exp(y*log(x)) */
		t += exp__E(t,c); return(scalb(one+t,m));
	     }
	/* end of if log(y*log(x)) > -60.0 */

	    else
		/* exp(+- tiny) = 1 with inexact flag */
			{ln2hi+ln2lo; return(one);}
	    else if(copysign(one,y)*(n+invln2*t) <zero)
		/* exp(-(big#)) underflows to zero */
	        	return(scalb(one,-5000));
	    else
	        /* exp(+(big#)) overflows to INF */
	    		return(scalb(one, 5000));

}
/* sincos.c - math routines */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01f,05feb93,jdi  doc changes based on kdl review.
01e,02dec92,jdi  doc tweaks.
01d,28oct92,jdi  documentation cleanup.
01c,21sep92,smb  changed function headers for mg.
01b,20sep92,smb  documentation additions
01a,08jul92,smb  documentation.
*/

/*
DESCRIPTION
* Copyright (c) 1987 Regents of the University of California.
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
SEE ALSO: American National Standard X3.159-1989
NOMANUAL
*/

#include "vxWorks.h"
#include "math.h"
#include "private/trigP.h"

/*******************************************************************************
*
* sin - compute a sine (ANSI)
*
* This routine computes the sine of <x> in double precision.
* The angle <x> is expressed in radians.
*
* INCLUDE FILES: math.h
*
* RETURNS: The double-precision sine of <x>.
*
* SEE ALSO: mathALib
*/

double sin
    (
    double x	/* angle in radians */
    )

    {
	double a,c,z;

        if(!finite(x))		/* sin(NaN) and sin(INF) must be NaN */
		return x-x;
	x=drem(x,PI2);		/* reduce x into [-PI,PI] */
	a=copysign(x,one);
	if (a >= PIo4) {
		if(a >= PI3o4)		/* ... in [3PI/4,PI] */
			x = copysign((a = PI-a),x);
		else {			/* ... in [PI/4,3PI/4]  */
			a = PIo2-a;		/* rtn. sign(x)*C(PI/2-|x|) */
			z = a*a;
			c = cos__C(z);
			z *= half;
			a = (z >= thresh ? half-((z-half)-c) : one-(z-c));
			return copysign(a,x);
		}
	}

	if (a < small) {		/* rtn. S(x) */
		big+a;
		return x;
	}
	return x+x*sin__S(x*x);
    }

/*******************************************************************************
*
* cos - compute a cosine (ANSI)
*
* This routine computes the cosine of <x> in double precision.
* The angle <x> is expressed in radians.
*
* INCLUDE FILES: math.h
*
* RETURNS: The double-precision cosine of <x>.
*
* SEE ALSO: mathALib
*/

double cos
    (
    double x	/* angle in radians */
    )

    {
	double a,c,z,s = 1.0;

	if(!finite(x))		/* cos(NaN) and cos(INF) must be NaN */
		return x-x;
	x=drem(x,PI2);		/* reduce x into [-PI,PI] */
	a=copysign(x,one);
	if (a >= PIo4) {
		if (a >= PI3o4) {	/* ... in [3PI/4,PI] */
			a = PI-a;
			s = negone;
		}
		else {			/* ... in [PI/4,3PI/4] */
			a = PIo2-a;
			return a+a*sin__S(a*a);	/* rtn. S(PI/2-|x|) */
		}
	}
	if (a < small) {
		big+a;
		return s;		/* rtn. s*C(a) */
	}
	z = a*a;
	c = cos__C(z);
	z *= half;
	a = (z >= thresh ? half-((z-half)-c) : one-(z-c));
	return copysign(a,s);
    }
/* sinh.c - math routine */

/* Copyright 1992-1994 Wind River Systems, Inc. */

/*
modification history
--------------------
01f,09dec94,rhp  fix descriptions of hyperbolic fns.
01e,05feb93,jdi  doc changes based on kdl review.
01d,02dec92,jdi  doc tweaks.
01c,28oct92,jdi  documentation cleanup.
01b,20sep92,smb  documentation additions
01a,08jul92,smb  documentation.
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
SEE ALSO: American National Standard X3.159-1989
NOMANUAL
*/

#include "vxWorks.h"
#include "math.h"

#if defined(vax)||defined(tahoe)
#ifdef vax
#define _0x(A,B)	0x/**/A/**/B
#else	/* vax */
#define _0x(A,B)	0x/**/B/**/A
#endif	/* vax */
/* static double */
/* mln2hi =  8.8029691931113054792E1     , Hex  2^  7   *  .B00F33C7E22BDB */
/* mln2lo = -4.9650192275318476525E-16   , Hex  2^-50   * -.8F1B60279E582A */
/* lnovfl =  8.8029691931113053016E1     ; Hex  2^  7   *  .B00F33C7E22BDA */
static long    mln2hix[] = { _0x(0f33,43b0), _0x(2bdb,c7e2)};
static long    mln2lox[] = { _0x(1b60,a70f), _0x(582a,279e)};
static long    lnovflx[] = { _0x(0f33,43b0), _0x(2bda,c7e2)};
#define   mln2hi    (*(double*)mln2hix)
#define   mln2lo    (*(double*)mln2lox)
#define   lnovfl    (*(double*)lnovflx)
#else	/* defined(vax)||defined(tahoe) */
static double
mln2hi =  7.0978271289338397310E2     , /*Hex  2^ 10   *  1.62E42FEFA39EF */
mln2lo =  2.3747039373786107478E-14   , /*Hex  2^-45   *  1.ABC9E3B39803F */
lnovfl =  7.0978271289338397310E2     ; /*Hex  2^  9   *  1.62E42FEFA39EF */
#endif	/* defined(vax)||defined(tahoe) */

#if defined(vax)||defined(tahoe)
static max = 126                      ;
#else	/* defined(vax)||defined(tahoe) */
static max = 1023                     ;
#endif	/* defined(vax)||defined(tahoe) */

/*******************************************************************************
*
* sinh - compute a hyperbolic sine (ANSI)
*
* This routine returns the hyperbolic sine of <x> in
* double precision (IEEE double, 53 bits).
*
* A range error occurs if <x> is too large.
*
* INTERNAL:
* Method:
*
* (1) Reduce <x> to non-negative by sinh(-x) = - sinh(x).
*
* (2)
*    	                                     expm1(x) + expm1(x)/(expm1(x)+1)
*    	   0 <= x <= lnovfl     : sinh(x) := --------------------------------
*    			       		                      2
*         lnovfl <= x <= lnovfl+ln2 : sinh(x) := expm1(x)/2 (avoid overflow)
*     lnovfl+ln2 <  x <  INF        :  overflow to INF
*    
* INCLUDE FILES: math.h
*
* RETURNS:
* The double-precision hyperbolic sine of <x>.
*
* Special cases:
*     If <x> is +INF, -INF, or NaN, sinh() returns <x>.
*
* SEE ALSO: mathALib
*/

double sinh
    (
    double x	/* number whose hyperbolic sine is required */
    )

    {
	static double  one=1.0, half=1.0/2.0 ;
	double expm1(), t, scalb(), copysign(), sign;
#if !defined(vax)&&!defined(tahoe)
	if(x!=x) return(x);	/* x is NaN */
#endif	/* !defined(vax)&&!defined(tahoe) */
	sign=copysign(one,x);
	x=copysign(x,one);
	if(x<lnovfl)
	    {t=expm1(x); return(copysign((t+t/(one+t))*half,sign));}

	else if(x <= lnovfl+0.7)
		/* subtract x by ln(2^(max+1)) and return 2^max*exp(x)
	    		to avoid unnecessary overflow */
	    return(copysign(scalb(one+expm1((x-mln2hi)-mln2lo),max),sign));

	else  /* sinh(+-INF) = +-INF, sinh(+-big no.) overflow to +-INF */
	    return( expm1(x)*sign );
    }
/* sqrt.c - software version of sqare-root routine */

/* Copyright 1992-1994 Wind River Systems, Inc. */

/*
modification history
--------------------
01f,02sep93,jwt  moved sparcHardSqrt to src/arch/sparc/sparcLib.c.
01e,05feb93,jdi  doc changes based on kdl review.
01d,02dec92,jdi  doc tweaks.
01c,28oct92,jdi  documentation cleanup.
01b,13oct92,jdi  mangen fixes.
01a,23jun92,kdl  extracted from v.01d of support.c.
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
* Some IEEE standard 754 recommended functions and remainder and sqrt for
* supporting the C elementary functions.
* -------------------------------------------------------------------------
* WARNING:
*      These codes are developed (in double) to support the C elementary
* functions temporarily. They are not universal, and some of them are very
* slow (in particular, drem and sqrt is extremely inefficient). Each
* computer system should have its implementation of these functions using
* its own assembler.
* -------------------------------------------------------------------------
*
* IEEE 754 required operations:
*     drem(x,p)
*              returns  x REM y  =  x - [x/y]*y , where [x/y] is the integer
*              nearest x/y; in half way case, choose the even one.
*     sqrt(x)
*              returns the square root of x correctly rounded according to
*		the rounding mod.
*
* IEEE 754 recommended functions:
* (a) copysign(x,y)
*              returns x with the sign of y.
* (b) scalb(x,N)
*              returns  x * (2**N), for integer values N.
* (c) logb(x)
*              returns the unbiased exponent of x, a signed integer in
*              double precision, except that logb(0) is -INF, logb(INF)
*              is +INF, and logb(NAN) is that NAN.
* (d) finite(x)
*              returns the value TRUE if -INF < x < +INF and returns
*              FALSE otherwise.
*
*
* CODED IN C BY K.C. NG, 11/25/84;
* REVISED BY K.C. NG on 1/22/85, 2/13/85, 3/24/85.
*
* SEE ALSO: American National Standard X3.159-1989
* NOMANUAL
*/

#include "vxWorks.h"
#include "math.h"
#include "private/mathP.h"
#include "errno.h"

extern double	scalb();
extern double	logb();
extern int	finite();

/*******************************************************************************
*
* sqrt - compute a non-negative square root (ANSI)
*
* This routine computes the non-negative square root of <x> in double
* precision.  A domain error occurs if the argument is negative.
*
* INCLUDE FILES: math.h
*
* RETURNS: The double-precision square root of <x> or 0 if <x> is negative.
*
* ERRNO: EDOM
*
* SEE ALSO: mathALib
*/

double sqrt
    (
    double x	/* value to compute the square root of */
    )

    {
        double q,s,b,r;
        double t,zero=0.0;
        int m,n,i;

#if defined(vax)||defined(tahoe)
        int k=54;
#else	/* defined(vax)||defined(tahoe) */
        int k=51;
#endif	/* defined(vax)||defined(tahoe) */

#if	(CPU_FAMILY == SPARC)	/* Select hardware/software square root */
        extern BOOL sparcHardSqrt;

        if (sparcHardSqrt == TRUE)
	    {
	    double  sqrtHw();

	    return (sqrtHw (x));
	    }
#endif	/* (CPU_FAMILY == SPARC) */

    /* sqrt(NaN) is NaN, sqrt(+-0) = +-0 */
        if(x!=x||x==zero) return(x);

    /* sqrt(negative) is invalid */
        if(x<zero) {
#if defined(vax)||defined(tahoe)
		extern double infnan();
		return (infnan(EDOM));	/* NaN */
#else	/* defined(vax)||defined(tahoe) */
		errno = EDOM; 
		return(zero/zero);
#endif	/* defined(vax)||defined(tahoe) */
	}

    /* sqrt(INF) is INF */
        if(!finite(x)) return(x);

    /* scale x to [1,4) */
        n=logb(x);
        x=scalb(x,-n);
        if((m=logb(x))!=0) x=scalb(x,-m);       /* subnormal number */
        m += n;
        n = m/2;
        if((n+n)!=m) {x *= 2; m -=1; n=m/2;}

    /* generate sqrt(x) bit by bit (accumulating in q) */
            q=1.0; s=4.0; x -= 1.0; r=1;
            for(i=1;i<=k;i++) {
                t=s+1; x *= 4; r /= 2;
                if(t<=x) {
                    s=t+t+2, x -= t; q += r;}
                else
                    s *= 2;
                }

    /* generate the last bit and determine the final rounding */
            r/=2; x *= 4;
            if(x==zero) goto end; 100+r; /* trigger inexact flag */
            if(s<x) {
                q+=r; x -=s; s += 2; s *= 2; x *= 4;
                t = (x-s)-5;
                b=1.0+3*r/4; if(b==1.0) goto end; /* b==1 : Round-to-zero */
                b=1.0+r/4;   if(b>1.0) t=1;	/* b>1 : Round-to-(+INF) */
                if(t>=0) q+=r; }	      /* else: Round-to-nearest */
            else {
                s *= 2; x *= 4;
                t = (x-s)-1;
                b=1.0+3*r/4; if(b==1.0) goto end;
                b=1.0+r/4;   if(b>1.0) t=1;
                if(t>=0) q+=r; }

end:        return(scalb(q,n));
    }
/* tan.c - math routines */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01f,05feb93,jdi  doc changes based on kdl review.
01e,02dec92,jdi  doc tweaks.
01d,28oct92,jdi  documentation cleanup.
01c,21sep92,smb  changed function header for mg.
01b,20sep92,smb  documentation additions
01a,08jul92,smb  documentation.
*/

/*
DESCRIPTION
* Copyright (c) 1987 Regents of the University of California.
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
SEE ALSO: American National Standard X3.159-1989
NOMANUAL
*/

#include "vxWorks.h"
#include "math.h"
#include "private/trigP.h"

/*******************************************************************************
*
* tan - compute a tangent (ANSI)
*
* This routine computes the tangent of <x> in double precision.
* The angle <x> is expressed in radians.
* 
* INCLUDE FILES: math.h
*
* RETURNS: The double-precision tangent of <x>.
*
* SEE ALSO: mathALib
*/

double tan
    (
    double x	/* angle in radians */
    )

    {
	double a,z,ss,cc,c;
	int k;

	if(!finite(x))		/* tan(NaN) and tan(INF) must be NaN */
		return x-x;
	x = drem(x,PI);			/* reduce x into [-PI/2, PI/2] */
	a = copysign(x,one);		/* ... = abs(x) */
	if (a >= PIo4) {
		k = 1;
		x = copysign(PIo2-a,x);
	}
	else {
		k = 0;
		if (a < small) {
			big+a;
			return x;
		}
	}
	z = x*x;
	cc = cos__C(z);
	ss = sin__S(z);
	z *= half;			/* Next get c = cos(x) accurately */
	c = (z >= thresh ? half-((z-half)-cc) : one-(z-cc));
	if (k == 0)
		return x+(x*(z-(cc-ss)))/c;	/* ... sin/cos */
#ifdef national
	else if (x == zero)
		return copysign(fmax,x);	/* no inf on 32k */
#endif	/* national */
	else
		return c/(x+x*ss);		/* ... cos/sin */
    }
/* tanh.c - math routines */

/* Copyright 1992-1994 Wind River Systems, Inc. */

/*
modification history
--------------------
01f,09dec94,rhp  fix man pages for hyperbolic fns.
01e,05feb93,jdi  doc changes based on kdl review.
01d,02dec92,jdi  doc tweaks.
01c,28oct92,jdi  documentation cleanup.
01b,20sep92,smb  documentation additions
01a,08jul92,smb  documentation.
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
SEE ALSO: American National Standard X3.159-1989
NOMANUAL
*/

#include "vxWorks.h"
#include "math.h"

/*******************************************************************************
*
* tanh - compute a hyperbolic tangent (ANSI)
*
* This routine returns the hyperbolic tangent of <x> in
* double precision (IEEE double, 53 bits).
*
* INTERNAL:
* Method:
*
* (1) Reduce <x> to non-negative by:
*
*         tanh(-x) = - tanh(x)
*
* (2)
*         0      <  x <=  1.e-10 :  tanh(x) := x
*
*                                               -expm1(-2x)
*         1.e-10 <  x <=  1      :  tanh(x) := --------------
*                                              expm1(-2x) + 2
*
*                                                       2
*         1      <= x <=  22.0   :  tanh(x) := 1 -  ---------------
*                                                   expm1(2x) + 2
*         22.0   <  x <= INF     :  tanh(x) := 1.
*
*     Note: 22 was chosen so that fl(1.0+2/(expm1(2*22)+2)) == 1.
*
* INCLUDE FILES: math.h
*
* RETURNS:
* The double-precision hyperbolic tangent of <x>.
*
* Special cases:
*     If <x> is NaN, tanh() returns NaN.
*
* SEE ALSO: mathALib
*
* INTERNAL:
* Coded in C by K.C. Ng, 1/8/85;
* Revised by K.C. Ng on 2/8/85, 2/11/85, 3/7/85, 3/24/85.
*/

double tanh
    (
    double x	/* number whose hyperbolic tangent is required */
    )

    {
	static double one=1.0, two=2.0, small = 1.0e-10, big = 1.0e10;
	double expm1(), t, copysign(), sign;
	int finite();

#if !defined(vax)&&!defined(tahoe)
	if(x!=x) return(x);	/* x is NaN */
#endif	/* !defined(vax)&&!defined(tahoe) */

	sign=copysign(one,x);
	x=copysign(x,one);
	if(x < 22.0)
	    if( x > one )
		return(copysign(one-two/(expm1(x+x)+two),sign));
	    else if ( x > small )
		{t= -expm1(-(x+x)); return(copysign(t/(two-t),sign));}
	    else		/* raise the INEXACT flag for non-zero x */
		{big+x; return(copysign(x,sign));}
	else if(finite(x))
	    return (sign+1.0E-37); /* raise the INEXACT flag */
	else
	    return(sign);	/* x is +- INF */
    }
