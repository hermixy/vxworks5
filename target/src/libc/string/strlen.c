/* strlen.c - file for string */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,25feb93,jdi  documentation cleanup for 5.1.
01b,20sep92,smb  documentation additions
01a,08jul92,smb  written and documented.
*/

/*
DESCRIPTION

INCLUDE FILES: string.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "string.h"


/*******************************************************************************
*
* strlen - determine the length of a string (ANSI)
*
* This routine returns the number of characters in <s>, not including EOS.
*
* INCLUDE FILES: string.h
*
* RETURNS: The number of non-null characters in the string.
*/

size_t strlen
    (
    const char * s        /* string */
    )
    {
    const char *save = s + 1;

    while (*s++ != EOS)
	;

    return (s - save);
    }
