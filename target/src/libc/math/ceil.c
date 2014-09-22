/* ceil.c - ceil math routine */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01g,27oct98,yh   return itself if it is ZERO or INF.
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
	    return (v);

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
