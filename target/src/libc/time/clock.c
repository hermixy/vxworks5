/* clock.c - clock file for time.h */

/* Copyright 1992-2001 Wind River Systems, Inc. */

/*
modification history
--------------------
01d,05oct01,dcb  Fix SPR 9814 and SPR 7736.  Change document to explain why
                 VxWorks always returns -1 for this function.
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

/******************************************************************************
*
* clock - determine the processor time in use (ANSI)
*
* This routine returns the implementation's best approximation of
* the processor time used by the program since the beginning of an
* implementation-defined era related only to the program invocation.
* To determine the time in seconds, the value returned by clock()
* should be divided by the value of the macro CLOCKS_PER_SEC.  If the
* processor time used is not available or its value cannot be
* represented, clock() returns -1.
*
* NOTE:
* This routine always returns -1 in VxWorks.  VxWorks does not track
* per-task time or system idle time.  There is no method of determining
* how long a task or the entire system has been doing work.  tickGet()
* can be used to query the number of system ticks since system start.
* clock_gettime() can be used to get the current clock time.
*
* INCLUDE FILES: time.h
*
* RETURNS: -1
*
* SEE ALSO: tickGet(), clock_gettime()
*/

clock_t clock (void)
    {
    return ((clock_t) -1);
    }
