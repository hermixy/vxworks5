/* strrchr.c - string search, string */

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


/******************************************************************************
*
* strrchr - find the last occurrence of a character in a string (ANSI)
*
* This routine locates the last occurrence of <c> in the string pointed
* to by <s>.  The terminating null is considered to be part of the string.
*
* INCLUDE FILES: string.h
*
* RETURNS:
* A pointer to the last occurrence of the character, or
* NULL if the character is not found.
*/

char * strrchr
    (
    const char * s,         /* string to search */
    int          c          /* character to look for */
    )
    {
    char *save = NULL;

    do					/* loop, searching for character */
	{
	if (*s == (char) c)
	    save = CHAR_FROM_CONST (s);
        } while (*s++ != EOS);

    return (CHAR_FROM_CONST(save));
    }
