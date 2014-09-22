/* difftime.c - difftime file for time */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01d,04oct01,dcb  Fix SPR 7191.  Cast time1 and time2 before subtracting.
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

/*******************************************************************************
*
* difftime - compute the difference between two calendar times (ANSI)
*
* This routine computes the  difference between two calendar times: 
* <time1> - <time0>.
*
* INCLUDE FILES: time.h
*
* RETURNS: The time difference in seconds, expressed as a double.
*/
  
double difftime
    (
    time_t time1,		/* later time, in seconds */
    time_t time0		/* earlier time, in seconds */
    )
    {
    /* This function assumes that sizeof(time_t) is <= sizeof(double). */
    
    return (double)time1 - (double)time0;
    }
