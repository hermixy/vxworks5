/* atol.c - atol files for stdlib  */

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

INCLUDE FILES: stdlib.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "stdlib.h"

/*****************************************************************************
*
* atol - convert a string to a `long' (ANSI)
*
* This routine converts the initial portion of the string <s> 
* to long integer representation.
*
* Its behavior is equivalent to:
* .CS
*     strtol (s, (char **)NULL, 10);
* .CE
*
* INCLUDE FILES: stdlib.h
*
* RETURNS: The converted value represented as a `long'.
*/

long atol
    (
    const register char * s		/* pointer to string */
    )
    {
    return strtol (s, (char **) NULL, 10);
    }
