/* atof.c - atof files for stdlib  */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,08feb93,jdi  documentation cleanup for 5.1.
01b,20sep92,smb  documentation additions.
01a,19jul92,smb  written and documentation.
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
* atof - convert a string to a `double' (ANSI)
*
* This routine converts the initial portion of the string <s> 
* to double-precision representation. 
*
* Its behavior is equivalent to:
* .CS
*     strtod (s, (char **)NULL);
* .CE
*
* INCLUDE FILES: stdlib.h
*
* RETURNS: The converted value in double-precision representation.
*/

double atof
    (
    const char * s	/* pointer to string */
    )
    {
    return (strtod (s, (char **) NULL));
    }
