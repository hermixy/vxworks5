/* tmpnam.c	- devise a temporary name. stdio.h */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/* 
modification history 
--------------------
01d,23sep93,jmm  made tmpnam()'s buffer static (spr 2525)
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
#include "string.h"

/******************************************************************************
*
* tmpnam - generate a temporary file name (ANSI)
*
* This routine generates a string that is a valid file name and not the same
* as the name of an existing file.  It generates a different string each
* time it is called, up to TMP_MAX times.
*
* If the argument is a null pointer, tmpnam() leaves its
* result in an internal static object and returns a pointer to that
* object.  Subsequent calls to tmpnam() may modify the same
* object.  If the argument is not a null pointer, it is assumed to
* point to an array of at least L_tmpnam chars; tmpnam() writes
* its result in that array and returns the argument as its value.
*
* INCLUDE FILES: stdio.h 
*
* RETURNS: A pointer to the file name.
*/

char * tmpnam
    (
    char *  s	/* name buffer */
    )
    {
    int             index;
    char *          pos;
    ushort_t        t;
    static char     buf [L_tmpnam];	/* internal buffer for name */
    static ushort_t seed = 0;		/* used to generate unique name */

    /* if parameter is NULL use internal buffer */
    if (s == NULL)
    	s = buf;

    /* generate unique name */

    strcpy (s, "tmp");

    /* fill up the name buffer from the last position */
    index = 5;
    pos = s + strlen (s) + index;
    *pos = '\0';

    seed++;
    for (t = seed; 0 <= --index; t >>= 3)
    	*--pos = '0' + (t & 07);

    /* return name buffer */
    return (s);
    }
