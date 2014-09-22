/* getw.c     - get a word. stdio.h */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,05mar93,jdi  documentation cleanup for 5.1.
01b,20sep92,smb  documentation additions
01a,29jul92,smb  written.
*/

/*
DESCRIPTION

INCLUDE FILE: stdio.h, string.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "stdio.h"

/******************************************************************************
*
* getw - read the next word (32-bit integer) from a stream
*
* This routine reads the next 32-bit quantity from a specified stream.
* It returns EOF on end-of-file or an error; however, this is also a
* valid integer, thus feof() and ferror() must be used to check for
* a true end-of-file.
*
* This routine is provided for compatibility with earlier VxWorks releases.
*
* INCLUDE FILES: stdio.h
*
* RETURN: A 32-bit number from the stream, or EOF on either end-of-file
* or an error.
*
* SEE ALSO: putw()
*/

int getw
    (
    FILE *	fp	/* stream to read from */
    )
    {
    int x;

    return (fread ((void *)&x, sizeof (x), 1, fp) == 1 ? x : EOF);
    }
