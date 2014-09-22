/* gmtime.c - gmtime file for time */

/* Copyright 1992-1996 Wind River Systems, Inc. */

/*
modification history
--------------------
01g,15oct96,dbt Used reentrant version of ldiv in getTime (fixed SPR #3795).
01f,12aug96,dbt modified __getTime to treat time_t as an unsigned (SPR #6178).  
01e,21jun96,dbt modified function __jullday (for leap year just test month > 1)
		modified call to jullday in __getTime in order to give it a 
		year as found in a tm structure, e.g. "96" (SPR #4251)
		Updated copyright.
01f,24aug94,ism	fixed problem with bus error in time conversion (SPR #3542)
			-fixed problem with negative time values
			-fixed problem with leap year in negative years (SPR #3576)
01e,24sep93,jmm  __julday() now checks for february 29th as a leap year 
01d,05feb93,jdi  documentation cleanup for 5.1.
01c,13nov92,dnw  changed slong_t decls to long
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

/****************************************************************************
*
* gmtime - convert calendar time into UTC broken-down time (ANSI)
*
* This routine converts the calendar time pointed to by <timer> into
* broken-down time, expressed as Coordinated Universal Time (UTC).
*
* INCLUDE FILES: time.h
*
* RETURNS:
* A pointer to a broken-down time structure (`tm'), or a null pointer
* if UTC is not available.
*/

struct tm *gmtime
    (
    const time_t *timer		/* calendar time in seconds */
    )
    {
    static struct tm timeBuffer;

    gmtime_r (timer, &timeBuffer);
    return (&timeBuffer);
    }

/****************************************************************************
*
* gmtime_r - convert calendar time into broken-down time (POSIX)
*
* This routine converts the calendar time pointed to by <timer> into
* broken-down time, expressed as Coordinated Universal Time (UTC).
* The broken-down time is stored in <timeBuffer>.
*
* This routine is the POSIX re-entrant version of gmtime().
*
* INCLUDE FILES: time.h
*
* RETURNS: OK.
*/

int gmtime_r
    (
    const time_t *timer,	/* calendar time in seconds */
    struct tm *   timeBuffer	/* buffer for broken down time */
    )
    {
    return (__getTime (*timer, timeBuffer));
    }

/************************************************************************
*
* __getTime - convert calendar time into broken-down time 
*
* internal routine.
*
* RETURNS: OK
* NOMANUAL
*/

int __getTime
    (
    const time_t timer,		/* time represented as seconds from epoch */
    struct tm *  tmp		/* pointer to broken-down time structure */
    )
    {
    long	days;
    long	timeOfDay;
    long	year;
    long	mon;
    ldiv_t	result; 

    /* Calulate number of days since epoch */

    days = timer / SECSPERDAY;
    timeOfDay = timer % SECSPERDAY;

    /* If time of day is negative, subtract one day, and add SECSPERDAY
     * to make it positive.
     */

    if(timeOfDay<0)
    	{
	timeOfDay+=SECSPERDAY;
	days-=1;
    	}

    /* Calulate number of years since epoch */

    year = days / DAYSPERYEAR;
    while ( __daysSinceEpoch (year, 0) > days )
    	year--;

    /* Calulate the day of the week */

    tmp->tm_wday = (days + EPOCH_WDAY) % DAYSPERWEEK;

	/*
	 * If there is a negative weekday, add DAYSPERWEEK to make it positive
	 */
	if(tmp->tm_wday<0)
		tmp->tm_wday+=DAYSPERWEEK;

    /* Find year and remaining days */

    days -= __daysSinceEpoch (year, 0);
    year += EPOCH_YEAR;

    /* Find month */
    /* __jullday needs years since TM_YEAR_BASE (SPR 4251) */

    for  ( mon = 0; 
         (days >= __julday (year - TM_YEAR_BASE, mon + 1, 0)) && (mon < 11); 
         mon++ )
	;

    /* Initialise tm structure */

    tmp->tm_year = year - TM_YEAR_BASE; /* years since 1900 */
    tmp->tm_mon  = mon;
    tmp->tm_mday = (days - __julday (tmp->tm_year, mon, 0)) + 1;
    tmp->tm_yday = __julday (tmp->tm_year, mon, tmp->tm_mday) - 1;
    tmp->tm_hour = timeOfDay / SECSPERHOUR;

    timeOfDay  %= SECSPERHOUR;
    ldiv_r (timeOfDay, SECSPERMIN, &result);
    tmp->tm_min = result.quot;
    tmp->tm_sec = result.rem;

    return(OK);
    }

/************************************************************************
*
*  daysSinceEpoch - calculate number days since ANSI C epoch                 
* 
*  The (year + 1)/4 term accounts for leap years, the     
*  first of which was 1972 & should be added starting '73 
* 
* RETURNS:
* NOMANUAL
*/

int __daysSinceEpoch
    ( 
    int year,	/* Years since epoch */
    int yday 	/* days since Jan 1  */
    )
    {
	if(year>=0) /* 1970 + */
    	return ( (365 * year) + (year + 1) / 4  + yday );
	else		/* 1969 - */
    	return ( (365 * year) + (year - 2) / 4  + yday );
    } 

/************************************************************************
*
* julday - calculate Julian Day given year, month, day            
*              Inputs      : year (years since 1900), month (0 - 11), 
*     			     day (1 - 31)  
*              Comment     : Returns Julian day in range 1:366.  
*			     Unix wants 0:365 
* RETURNS: Julian day                                            
* NOMANUAL
*/

int __julday
    ( 
    int yr, /* year */
    int mon, /* month */
    int day /* day */
    )
    {
    static jdays[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
    int leap = 0;

    if (isleap (yr + TM_YEAR_BASE))
	{
	/*
	 * If it is a leap year, leap only gets set if the day is
	 * after beginning of March (SPR #4251).
	 */
	if (mon > 1)
	    leap = 1;
	}

    return (jdays [mon] + day + leap );

    }
