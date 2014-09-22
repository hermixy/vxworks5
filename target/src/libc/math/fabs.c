/* fabs.c - fabs math routine */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01g,05feb99,dgp  document errno values
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
