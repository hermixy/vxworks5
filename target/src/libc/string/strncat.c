/* strncat.c - file for string */

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
* strncat - concatenate characters from one string to another (ANSI)
*
* This routine appends up to <n> characters from string <src> to the
* end of string <dst>.
*
* INCLUDE FILES: string.h
*
* RETURNS: A pointer to the null-terminated string <s1>.
*/

char * strncat
    (
    char *	 dst,  	/* string to append to */
    const char * src,   /* string to append */
    size_t	 n     	/* max no. of characters to append */
    )
    {
    if (n != 0)
	{
	char *d = dst;

	while (*d++ != EOS)			/* find end of string */
	    ;

	d--;					/* rewind back of EOS */

	while (((*d++ = *src++) != EOS) && (--n > 0))
	    ;

	if (n == 0)
	    *d = EOS;				/* NULL terminate string */
	}

    return (dst);
    }
