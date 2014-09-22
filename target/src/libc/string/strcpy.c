/* strcpy.c - string copy, string */

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
* strcpy - copy one string to another (ANSI)
*
* This routine copies string <s2> (including EOS) to string <s1>.
*
* INCLUDE FILES: string.h
*
* RETURNS: A pointer to <s1>.
*/

char * strcpy
    (
    char *       s1,	/* string to copy to */
    const char * s2	/* string to copy from */
    )
    {
    char *save = s1;

    while ((*s1++ = *s2++) != EOS)
	;

    return (save);
    }
