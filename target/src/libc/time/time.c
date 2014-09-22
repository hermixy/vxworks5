/* time.c - time file for time */

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
#include "timers.h"

/****************************************************************************
*
* time - determine the current calendar time (ANSI)
*
* This routine returns the implementation's best approximation of current
* calendar time in seconds.  If <timer> is non-NULL, the return value is
* also copied to the location <timer> points to.
*
* INTERNAL
* Uses the POSIX clockLib functions.
* Does this return the number of seconds since the BOARD was booted?
*
* INCLUDE FILES: time.h
*
* RETURNS:
* The current calendar time in seconds, or ERROR (-1) if the calendar time
* is not available.
*
* SEE ALSO: clock_gettime()
*/
 
time_t time
    (
    time_t *timer	/* calendar time in seconds */
    )
    {
    struct timespec tp;

    if (clock_gettime (CLOCK_REALTIME, &tp) == 0)
	{
	if (timer != NULL)
	    *timer = (time_t) tp.tv_sec;
	return (time_t) (tp.tv_sec);
	}
    else
	return (time_t) (ERROR);
    }
