/* floor.c - floor math routine */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01g,17nov00,tkt  return itself if it is ZERO or INF.
01g,04may01,agf  fix return value of INF
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
	case ZERO:	return (0);

	case INF:
	case NAN:	return (v);

	case INTEGER:	return (v);

	case REAL:	return (((v < 0.0) && (v != r)) ? (r - 1.0) : (r));

	default:	return (0);		/* this should never happen */
	}
    }
