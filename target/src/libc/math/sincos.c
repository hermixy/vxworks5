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
