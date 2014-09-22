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
