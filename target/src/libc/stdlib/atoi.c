/* atoi.c - atoi files for stdlib  */

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

/******************************************************************************
*
* atoi - convert a string to an `int' (ANSI)
*
* This routine converts the initial portion of the string
* <s> to `int' representation.
*
* Its behavior is equivalent to:
* .CS
*     (int) strtol (s, (char **) NULL, 10);
* .CE
*
* INCLUDE FILES: stdlib.h
*
* RETURNS: The converted value represented as an `int'.
*/

int atoi
    (
    const char * s		/* pointer to string */
    )
    {
    return (int) strtol (s, (char **) NULL, 10);
    }
