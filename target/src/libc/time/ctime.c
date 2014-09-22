/* ctime.c - ctime file for time */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
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

/*******************************************************************************
*
* ctime - convert time in seconds into a string (ANSI)
*
* This routine converts the calendar time pointed to by <timer> into local
* time in the form of a string.  It is equivalent to:
* .CS
*     asctime (localtime (timer));
* .CE
*
* INCLUDE FILES: time.h
*
* RETURNS:
* The pointer returned by asctime() with local broken-down time as the
* argument.
*
* SEE ALSO: asctime(), localtime()
*/

char * ctime
    ( 
    const time_t *timer 	/* calendar time in seconds */
    )
    {
    size_t len = sizeof (ASCBUF);
    static char asctimeBuf [sizeof (ASCBUF)];

    return (ctime_r (timer, asctimeBuf, &len));
    }

/*******************************************************************************
*
* ctime_r - convert time in seconds into a string (POSIX)
*
* This routine converts the calendar time pointed to by <timer> into local
* time in the form of a string.  It is equivalent to:
* .CS
*     asctime (localtime (timer));
* .CE
*
* This routine is the POSIX re-entrant version of ctime().
*
* INCLUDE FILES: time.h
*
* RETURNS:
* The pointer returned by asctime() with local broken-down time as the
* argument.
*
* SEE ALSO: asctime(), localtime()
*/

char * ctime_r
    ( 
    const time_t * timer,		/* calendar time in seconds */
    char *         asctimeBuf,		/* buffer to contain the string */
    size_t *       buflen		/* size of the buffer */
    )
    {
    asctime_r (localtime (timer), asctimeBuf, buflen);

    return (asctimeBuf);
    }
