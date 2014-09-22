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
