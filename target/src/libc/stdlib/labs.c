/* labs.c - labs file for stdlib  */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,08feb93,jdi  documentation cleanup for 5.1.
01b,20sep92,smb  documentation additions.
01a,19jul92,smb  written.
*/

/* 
DESCRIPTION 
 
INCLUDE FILE: stdlib.h
  
SEE ALSO: American National Standard X3.159-1989 

NOMANUAL
*/ 

#include "vxWorks.h"
#include "stdlib.h"

/*******************************************************************************
*
* labs - compute the absolute value of a `long' (ANSI)
*
* This routine computes the absolute value of a specified `long'.  If the
* result cannot be represented, the behavior is undefined.  This routine is
* equivalent to abs(), except that the argument and return value are all of
* type `long'.
*
* INCLUDE FILES: stdlib.h 
*
* RETURNS: The absolute value of <i>.
*/

long labs 
    (
    long i          /* long for which to return absolute value */
    )
    {
    return (i >= 0 ? i : -i);
    }

