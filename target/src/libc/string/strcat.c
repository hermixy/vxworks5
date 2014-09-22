/* strcat.c - concatenate one string to another, string */

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
* strcat - concatenate one string to another (ANSI)
*
* This routine appends a copy of string <append> to the end of string 
* <destination>.  The resulting string is null-terminated.
*
* INCLUDE FILES: string.h
*
* RETURNS: A pointer to <destination>.
*/

char * strcat
    (
    char *       destination, /* string to be appended to */
    const char * append       /* string to append to <destination> */
    )
    {
    char *save = destination;

    while (*destination++ != '\0')		/* find end of string */
        ;

    destination--;

    while ((*destination++ = *append++) != '\0')
	;

    return (save);
    }
