/* asctime.c - asctime file for time.h */

/* Copyright 1992-1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01d,12mar99,p_m  Fixed SPR 8225 by putting asctime output in uppercase.
01c,05feb93,jdi  documentation cleanup for 5.1.
01b,20sep92,smb  documentation additions
01a,25jul92,smb  written.
*/

/* 
DESCRIPTION 
 
INCLUDE FILE: time.h
  
SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/ 

#include "vxWorks.h"
#include "time.h"
#include "private/timeP.h"

/****************************************************************************
*
* asctime - convert broken-down time into a string (ANSI)
*
* This routine converts the broken-down time pointed to by <timeptr> into a
* string of the form:
* .CS
*	SUN SEP 16 01:03:52 1973\en\e0
* .CE
*
* INCLUDE FILES: time.h
*
* RETURNS: A pointer to the created string.
*/

char * asctime
    (
    const struct tm *timeptr	/* broken-down time */
    )
    {
    size_t           len = sizeof (ASCBUF);
    static char      asctimeBuf [sizeof (ASCBUF)];

    asctime_r (timeptr, asctimeBuf, &len);

    return (asctimeBuf);
    }

/****************************************************************************
*
* asctime_r - convert broken-down time into a string (POSIX)
*
* This routine converts the broken-down time pointed to by <timeptr> into a
* string of the form:
* .CS
*	SUN SEP 16 01:03:52 1973\en\e0
* .CE
* The string is copied to <asctimeBuf>.
*
* This routine is the POSIX re-entrant version of asctime().
*
* INCLUDE FILES: time.h
*
* RETURNS: The size of the created string.
*/

int asctime_r
    (
    const struct tm *timeptr,		/* broken-down time */
    char *           asctimeBuf,	/* buffer to contain string */
    size_t *         buflen		/* size of buffer */
    )
    {
    size_t           size;

    size = strftime (asctimeBuf, 
             	     *buflen,
             	     "%a %b %d %H:%M:%S %Y\n\0", 
             	     timeptr);

    return ((int) size);
    }
