/* memmove.c - memory move file for string */

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
* memmove - copy memory from one location to another (ANSI)
*
* This routine copies <size> characters from the memory location <source> to
* the location <destination>.  It ensures that the memory is not corrupted
* even if <source> and <destination> overlap.
*
* INCLUDE FILES: string.h
*
* RETURNS: A pointer to <destination>.
*/

void * memmove
    (
    void *	 destination,	/* destination of copy */
    const void * source,	/* source of copy */
    size_t 	 size		/* size of memory to copy */
    )
    {
    char *	dest;
    const char *src;

    dest = destination;
    src = source;

    if ((src < dest) && (dest < (src + size)))
	{
	for (dest += size, src += size; size > 0; --size)
	    *--dest = *--src;
        }
    else 
	{
	while (size > 0)
	    {
	    size--;
	    *dest++ = *src++;
	    }
        }
    return (destination);
    }
