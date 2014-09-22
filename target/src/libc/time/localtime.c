/* localtime.c - localtime routine for the ANSI time library */

/* Copyright 1992-1996 Wind River Systems, Inc. */

/*
modification history
--------------------
01e,17nov97,dgp  doc: fix SPR 9415: localtime returns a static structure
01d,20jun96,dbt  corrected localtime_r (SPR #2521).
		 timeBuffer must be initialized before a call of __getDstInfo.
		 Updated copyright.
01c,05feb93,jdi  documentation cleanup for 5.1.
01b,20sep92,smb  documentation additions
01a,25jul92,smb  written.
*/

/*  
DESCRIPTION  
  
INCLUDE FILE: time.h, stdlib.h
   
SEE ALSO: American National Standard X3.159-1989 

NOMANUAL
*/

#include "vxWorks.h"
#include "time.h"
#include "stdlib.h"
#include "private/timeP.h"

extern TIMELOCALE *__loctime;	/* time locale information */

/****************************************************************************
*
* localtime - convert calendar time into broken-down time (ANSI)
*
* This routine converts the calendar time pointed to by <timer> into
* broken-down time, expressed as local time.
*
* INCLUDE FILES: time.h
*
* RETURNS: 
* A pointer to the static structure `tm' containing the local broken-down time.
*/

struct tm *localtime
    (
    const time_t * timer 	/* calendar time in seconds */
    )
    {
    static struct tm timeBuffer;

    localtime_r (timer, &timeBuffer);
    return (&timeBuffer);
    }

/****************************************************************************
*
* localtime_r - convert calendar time into broken-down time (POSIX)
*
* This routine converts the calendar time pointed to by <timer> into
* broken-down time, expressed as local time.  The broken-down time is
* stored in <timeBuffer>.
*
* This routine is the POSIX re-entrant version of localtime().
*
* INCLUDE FILES: time.h
*
* RETURNS: OK.
*/

int localtime_r
    (
    const time_t * timer, 	/* calendar time in seconds */
    struct tm *    timeBuffer	/* buffer for the broken-down time */
    )
    {
    char zoneBuf [sizeof (ZONEBUFFER)];
    int  dstOffset;

    /* First get the zone info */
    __getZoneInfo(zoneBuf, TIMEOFFSET, __loctime);

    /* Generate a broken-down time structure */
    __getTime (*timer - ((atoi (zoneBuf)) * SECSPERMIN), timeBuffer);

    /* is Daylight Saving Time in effect ? */
    dstOffset  = __getDstInfo (timeBuffer,__loctime);
    timeBuffer->tm_isdst = dstOffset;

    /* Correct the broken-down time structure if necessary */
    if (dstOffset)
	__getTime ((*timer - ((atoi (zoneBuf)) * SECSPERMIN))
				+ (dstOffset * SECSPERHOUR), timeBuffer);

    return (OK);                 /* __getTime always returns OK */
    }
