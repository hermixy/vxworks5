/* memset.c - set a block of memory, string */

/* Copyright 1992-1995 Wind River Systems, Inc. */

/*
modification history
--------------------
01g,11feb95,jdi  fixed size parameter name in doc.
01f,25feb93,jdi  documentation cleanup for 5.1.
01e,20sep92,smb  documentation additions
01d,14sep92,smb  changes back to use bfill.
01c,07sep92,smb  changed so that memset is seperate from bfill
01b,30jul92,smb  changes to use bfill.
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
* memset - set a block of memory (ANSI)
*
* This routine stores <c> converted to an `unsigned char' in each of the
* elements of the array of `unsigned char' beginning at <m>, with size <size>.
*
* INCLUDE FILES: string.h
*
* RETURNS: A pointer to <m>.
*/

void * memset
    (
    void * m,                   /* block of memory */
    int    c,                   /* character to store */
    size_t size                 /* size of memory */
    )
    {
    bfill ((char *) m, (int) size, c);
    return (m);
    }
