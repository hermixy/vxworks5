/* memcpy.c - memory copy file for string */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01g,25feb93,jdi  documentation cleanup for 5.1.
01f,20sep92,smb  documentation additions
01e,14sep92,smb  memcpy again uses bcopy
01d,07sep92,smb  changed so that memcpy is seperate from bcopy.
01c,30jul92,smb  changed to use bcopy.
01b,12jul92,smb  changed post decrements to pre decrements.
01a,08jul92,smb  written and documented.
           +rrr
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
* memcpy - copy memory from one location to another (ANSI)
*
* This routine copies <size> characters from the object pointed
* to by <source> into the object pointed to by <destination>. If copying
* takes place between objects that overlap, the behavior is undefined.
*
* INCLUDE FILES: string.h
*
* RETURNS: A pointer to <destination>.
*/

void * memcpy
    (
    void *       destination,   /* destination of copy */
    const void * source,        /* source of copy */
    size_t       size           /* size of memory to copy */
    )
    {
    bcopy ((char *) source, (char *) destination, (size_t) size);
    return (destination);
    }

