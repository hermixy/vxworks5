/* abs.c - abs file for stdlib  */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,08feb93,jdi  documentation cleanup for 5.1.
01b,20sep92,smb  documentation additions.
01a,19jul92,smb  written and documented.
*/

/*
DESCRIPTION

INCLUDE FILES: stdlib.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "stdlib.h"

/*******************************************************************************
*
* abs - compute the absolute value of an integer (ANSI)
*
* This routine computes the absolute value of a specified integer.  If the
* result cannot be represented, the behavior is undefined.
*
* INCLUDE FILES: stdlib.h
*
* RETURNS: The absolute value of <i>.
*/

int abs 
    (
    int i          /* integer for which to return absolute value */
    )
    {
    return (i >= 0 ? i : -i);
    }
