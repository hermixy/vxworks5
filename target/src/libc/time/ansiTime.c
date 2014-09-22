/* ansiTime.c - ANSI `time' documentation */

/* Copyright 1992-2001 Wind River Systems, Inc. */

/*
modification history
--------------------
01p,05oct01,dcb  Fix SPR 9814, SPR 7736, SPR 62831 and SPR 7191.
01o,12mar99,p_m  Fixed SPR 8225 by putting asctime output in uppercase.
01n,11jul97,dgp  doc: SPR 7651 need list of non-reentrant functions
01m,20jan97,dbt  modified comment for tm_sec (SPR #4436).
01l,15oct96,dbt  Used reentrant version of ldiv in getTime (fixed SPR #3795).
01k,04oct96,dbt  reworked the fix for SPR #7277.
01j,03oct96,dbt  use memcpy with 'strlen + 1' instead of 'strlen' (SPR #7277)
01i,12aug96,dbt  modified __getTime to treat time_t as an unsigned (SPR #6178).
01h,30jul96,dbt  In function mktime, if tm_isdst flag is true substract one 
		 hour from timeIs (SPR #6954). 
		 Updated copyright.
01g,25jul96,dbt  modified function __jullday (for leap year just test month > 1)
	       	 modified call to jullday in __getTime in order to give it a
	         year as found in a tm structure, e.g. "96" (SPR #4251)
		 corrected localtime_r (SPR #2521).
                 timeBuffer must be initialized before a call of __getDstInfo.
 		 corrected __getDstInfo() (spr 2521)
		 Updated copyright.
01h,25jul96,dbt  fixed warnings in mktime.c.
01g,16feb95,rhp  doc tweaks, and synch with subsidiary files
01f,15sep94,rhp  fixed TIMEZONE example in comment (related to SPR #3490);
		 reduced duplication between man page and comments
01e,17aug93,dvs  changed TIME to TIMEO to fix conflicting defines (SPR #2249)
01d,13mar93,jdi  doc tweak.
01c,05feb93,jdi  doc: clarified TIMEZONE string - SPR 1977;
		 clarified that TIMEZONE is environment variable - SPR 1974.
01b,13nov92,dnw  changed slong_t decls to long
01a,24oct92,smb  written.
*/

/*  
DESCRIPTION  
The header time.h defines two macros and declares four types and several
functions for manipulating time.  Many functions deal with a `calendar time'
that represents the current date (according to the Gregorian calendar)
and time.  Some functions deal with `local time', which is the calendar
time expressed for some specific time zone, and with Daylight Saving Time,
which is a temporary change in the algorithm for determining local time.
The local time zone and Daylight Saving Time are implementation-defined.

.SS Macros
The macros defined are NULL and:
.iP `CLOCKS_PER_SEC' 12
the number of ticks per second.
.LP

.SS Types
The types declared are `size_t' and:
.iP "`clock_t', `time_t'" 12
arithmetic types capable of representing times.
.iP "`struct tm'"
holds the components of a calendar time in what is known as "broken-down time." 
The structure contains at least the following members, in any order.
The semantics of the members and their normal ranges are expressed in the
comments.

.TS
tab(|);
l1p9f3 l1 l.
int tm_sec;   | seconds after the minute | - [0, 59]
int tm_min;   | minutes after the hour   | - [0, 59]
int tm_hour;  | hours after midnight     | - [0, 23]
int tm_mday;  | day of the month         | - [1, 31]
int tm_mon;   | months since January     | - [0, 11]
int tm_year;  | years since 1900
int tm_wday;  | days since Sunday        | - [0, 6]
int tm_yday;  | days since January 1     | - [0, 365] 
int tm_isdst; | Daylight Saving Time flag
.TE

The value of `tm_isdst' is positive if Daylight Saving Time is in effect, zero
if Daylight Saving Time is not in effect, and negative if the information
is not available.
.LP

If the environment variable TIMEZONE is set, the information is retrieved from
this variable, otherwise from the locale information.
TIMEZONE is of the form:

.ft CB
    <name_of_zone>:<(unused)>:<time_in_minutes_from_UTC>:<daylight_start>:<daylight_end>
.ft 1

To calculate local time, the value of <time_in_minutes_from_UTC> is subtracted
from UTC; <time_in_minutes_from_UTC> must be positive.

Daylight information is expressed as mmddhh (month-day-hour), for
example:
.CS
    UTC::0:040102:100102
.CE

REENTRANCY
Where there is a pair of routines, such as div() and div_r(), only the routine
xxx_r() is reentrant.  The xxx() routine is not reentrant.
 
INCLUDE FILES: time.h

SEE ALSO: ansiLocale, American National Standard X3.159-1989 
*/

/*
Documentation for the ANSI C time library.
==========================================
 
Locale Information
------------------
 
The time locale information is stored in the file __timeloc.c. This
structure consists of
        + The days of the week in abbreviated form
        + The days of the week
        + Months of the year in abbreviated form
        + Months of the year
        + date and time representation for this locale
        + representation for AM.PM
        + Time zone information (discussed below )
        + Day light saving information (discussed below )

If you want to, for example, use the french translation for the days
of the week, then this file could be copied into another file
e.g. __frtimeloc.c and the days and months representation in french. A
full compile is neccessary.  This means that when using strftime or
asctime the days and months will be in French!

Maintaining a table of the possible locale variations would mean large
linked lists of tables and is not appropriate for this system.
Maybe later we can provide a mechanism for replacing this table by a
table defined by the user at system initialisation.

The zone and daylight information may also be set to represent the
local environment.

ZONE & Daylight Saving
----------------------

The first three fields of TIMEZONE are represented in the locale structure
by the time zone information area and the day light saving area of the
locale structure is of the same form as the last two fields.

The TIMEZONE environment variable can be set by the user to reflect the locale
environment.

SEE ALSO: American National Standard X3.159-1989 
NOMANUAL

INTERNAL
This documentation module is built by appending the following files:

    asctime.c
    clock.c
    ctime.c
    difftime.c
    gmtime.c
    localtime.c
    mktime.c
    strftime.c
    time.c
*/


/* asctime.c - asctime file for time.h */

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
* This routine is not reentrant.  For a reentrant version, see asctime_r().
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
/* clock.c - clock file for time.h */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01d,05oct01,dcb  Fix SPR 9814 and SPR 7736.
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
* This routine is not reentrant.  For a reentrant version, see ctime_r().
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
* This routine is not reentrant.  For a reentrant version, see gmtime_r().
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

/* localtime.c - localtime routine for the ANSI time library */

/* Copyright 1992-1996 Wind River Systems, Inc. */

/*
modification history
--------------------
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
* This routine is not reentrant.  For a reentrant version, see localtime_r().
*
* INCLUDE FILES: time.h
*
* RETURNS: 
* A pointer to a `tm' structure containing the local broken-down time.
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

/* mktime.c - mktime file for time  */

/* Copyright 1992-1996 Wind River Systems, Inc. */

/*
modification history
--------------------
01f,30jul96,dbt  In function mktime, if tm_isdst flag is true substract one 
		 hour from timeIs (SPR #6954). 
		 Updated copyright.
01e,25jul96,dbt  fixed warnings in mktime.c.
01d,24sep93,jmm  _tmValidate() calls _julday() with the tm_mday parameter
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

extern TIMELOCALE *__loctime; /* locale time structure */

/* LOCAL */
LOCAL void __tmNormalize (int *,int *,int);
LOCAL void __tmValidate (struct tm *); 

/*******************************************************************************
*
* mktime - convert broken-down time into calendar time (ANSI)
*
* This routine converts the broken-down time, expressed as local time, in
* the structure pointed to by <timeptr> into a calendar time value with the
* same encoding as that of the values returned by the time() function.  The
* original values of the `tm_wday' and `tm_yday' components of the `tm'
* structure are ignored, and the original values of the other components are
* not restricted to the ranges indicated in time.h.  On successful completion,
* the values of `tm_wday' and `tm_yday' are set appropriately, and the other
* components are set to represent the specified calendar time, but with
* their values forced to the ranges indicated in time.h; the final value of
* `tm_mday' is not set until `tm_mon' and `tm_year' are determined.
*
* INCLUDE FILES: time.h
*
* RETURNS:
* The calendar time in seconds, or ERROR (-1)
* if calendar time cannot be calculated.
*/

time_t mktime
    (
    struct tm * timeptr	/* pointer to broken-down structure */
    )
    {
    time_t timeIs = 0;
    int    days   = 0;
    char   zoneBuf [sizeof (ZONEBUFFER)];

    /* Validate tm structure */
    __tmValidate (timeptr);

    /* Calulate time_t value */
    /* time */
    timeIs += (timeptr->tm_sec +
    	      (timeptr->tm_min * SECSPERMIN) +
    	      (timeptr->tm_hour * SECSPERHOUR));

    /* date */
    days += __julday (timeptr->tm_year, timeptr->tm_mon, timeptr->tm_mday);

    timeptr->tm_yday = (days - 1);

    if ((timeptr->tm_year + TM_YEAR_BASE) < EPOCH_YEAR )
    	return ((time_t) ERROR);

    /* days in previous years */
    days = __daysSinceEpoch (timeptr->tm_year - (EPOCH_YEAR - TM_YEAR_BASE),
    		             timeptr->tm_yday );

    timeptr->tm_wday = (days + EPOCH_WDAY) % DAYSPERWEEK;

    timeIs += (days * SECSPERDAY);

    /* correct for day light saving */
    /* validate again for the extra DST hour */
    if ((timeptr->tm_isdst = __getDstInfo (timeptr, __loctime)))
    	{
    	timeIs -= SECSPERHOUR;
    	__tmValidate (timeptr);
    	}

    /* correct for zone offset from UTC */
    __getZoneInfo (zoneBuf, TIMEOFFSET, __loctime);
    timeIs += (atoi (zoneBuf) * SECSPERMIN);

    return(timeIs);
    }
	
/*******************************************************************************
*
* __tmValidate - validate the broken-down structure, tmptr.
*
* RETURNS: the validated structure.
* NOMANUAL
*/
LOCAL void __tmValidate
    (
    struct tm * tmptr	/* pointer to broken-down structure */
    )
    {
    struct tm tmStruct;
    int       jday;
    int       mon;

    /* Adjust timeptr to reflect a legal time
     * Is it within range 1970-2038?
     */
		   
    tmStruct = *tmptr;

    __tmNormalize (&tmStruct.tm_min, &tmStruct.tm_sec, SECSPERMIN);
    __tmNormalize (&tmStruct.tm_hour, &tmStruct.tm_min, MINSPERHOUR);
    __tmNormalize (&tmStruct.tm_mday, &tmStruct.tm_hour, HOURSPERDAY);
    __tmNormalize (&tmStruct.tm_year, &tmStruct.tm_mon, MONSPERYEAR);

    /* tm_mday may not be in the correct range - check */

    jday = __julday (tmStruct.tm_year, tmStruct.tm_mon , tmStruct.tm_mday);

    if (jday < 0) 
    	{
    	tmStruct.tm_year--;
    	jday += DAYSPERYEAR;
    	}

    /* Calulate month and day */
    for (mon = 0; 
         (jday > __julday (tmStruct.tm_year, mon+1, 0)) && (mon < 11); 
         mon++ )
	;

    tmStruct.tm_mon  = mon;
    tmStruct.tm_mday = jday - __julday (tmStruct.tm_year, mon, 0);
    tmStruct.tm_wday = 0;
    tmStruct.tm_yday = 0;

    *tmptr = tmStruct;
    }

/*******************************************************************************
*
* __tmNormalize - This function is used to reduce units to a range [0,base]
*		  tens is used to store the number of times units is divisable
*		  by base.
*
* 	total = (tens * base) + units
*
* RETURNS: no value
* NOMANUAL
*/

LOCAL void __tmNormalize
    (
    int * tens,		/* tens */
    int * units,	/* units */
    int   base		/* base */
    )
    {
    *tens += *units / base;
    *units %= base;

    if ((*units % base ) < 0)
    	{
    	(*tens)--;
    	*units += base;
    	}
    }

/* strftime.c - strftime file for time  */

/* Copyright 1991-1996 Wind River Systems, Inc. */

/*
modification history
--------------------
01i,20jan97,dbt  modified comment concerning seconds (SPR #4436).
01h,04oct96,dbt  reworked the fix for SPR #7277.
01g,03oct96,dbt  use memcpy with 'strlen + 1' instead of 'strlen' (SPR #7277)
01f,27jun96,dbt  corrected __getDstInfo() (spr 2521)
	 	 Updated copyright.
01g,10feb95,rhp  internal doc tweak from ansiTime.c
01f,15sep94,rhp  fixed TIMEZONE example in comment (related to SPR #3490)
01e,17aug93,dvs  changed TIME to TIMEO to fix conflicting defines (SPR #2249)
01d,05feb93,jdi  documentation cleanup for 5.1.
01c,20sep92,smb  documentation additions
01b,26jul92,rrr  fixed decl of __weekOfYear to compile on mips.
01a,25jul92,smb  written.
*/

/*  
DESCRIPTION  
  
INCLUDE FILE: time.h, stdlib.h, string.h, locale.h
   
SEE ALSO: American National Standard X3.159-1989 

NOMANUAL
*/

#include "vxWorks.h"
#include "string.h"
#include "stdlib.h"
#include "locale.h"
#include "private/timeP.h"

extern TIMELOCALE * __loctime;		/* time locale information */

/* LOCAL */

LOCAL size_t strftime_r (char *, size_t, const char *,
			 const struct tm *, TIMELOCALE *);
LOCAL void   __generateTime (char *, const struct tm *,
		             TIMELOCALE *, int *, const char *);
LOCAL void   __getDay (char *, int, int, TIMELOCALE *, int *);
LOCAL void   __getMonth (char *, int, int, TIMELOCALE *, int *);
LOCAL void   __getLocale (char *, int, const struct tm *, TIMELOCALE *, int *);
LOCAL void   __intToStr (char *, int, int);
LOCAL int    __weekOfYear (int, int, int);


/*******************************************************************************
*
* strftime - convert broken-down time into a formatted string (ANSI)
*
* This routine formats the broken-down time in <tptr> based on the conversion
* specified in the string <format>, and places the result in the string <s>.
*
* The format is a multibyte character sequence, beginning and ending in its
* initial state.  The <format> string consists of zero or more conversion
* specifiers and ordinary multibyte characters.  A conversion specifier
* consists of a % character followed by a character that determines the
* behavior of the conversion.  All ordinary multibyte characters (including
* the terminating NULL character) are copied unchanged to the array.  If
* copying takes place between objects that overlap, the behavior is
* undefined.  No more than <n> characters are placed into the array.
*
* Each conversion specifier is replaced by appropriate characters as 
* described in the following list.  The appropriate characters are determined 
* by the LC_TIME category of the current locale and by the values contained 
* in the structure pointed to by <tptr>.
*
* .iP %a
* the locale's abbreviated weekday name.
* .iP %A
* the locale's full weekday name.
* .iP %b
* the locale's abbreviated month name.
* .iP %B
* the locale's full month name.
* .iP %c
* the locale's appropriate date and time representation.
* .iP %d
* the day of the month as decimal number (01-31).
* .iP %H
* the hour (24-hour clock) as a decimal number (00-23).
* .iP %I
* the hour (12-hour clock) as a decimal number (01-12).
* .iP %j
* the day of the year as decimal number (001-366).
* .iP %m
* the month as a decimal number (01-12).
* .iP %M
* the minute as a decimal number (00-59).
* .iP %P
* the locale's equivalent of the AM/PM 
* designations associated with a 12-hour clock.
* .iP %S
* the second as a decimal number (00-59).
* .iP %U
* the week number of the year (first Sunday
* as the first day of week 1) as a decimal number (00-53).
* .iP %w
* the weekday as a decimal number (0-6), where Sunday is 0.
* .iP %W
* the week number of the year (the first Monday
* as the first day of week 1) as a decimal number (00-53).
* .iP %x
* the locale's appropriate date representation.
* .iP %X
* the locale's appropriate time representation.
* .iP %y
* the year without century as a decimal number (00-99).
* .iP %Y
* the year with century as a decimal number.
* .iP %Z
* the time zone name or abbreviation, or by no
* characters if no time zone is determinable.
* .iP %%
* %.
* .LP
*
* For any other conversion specifier, the behavior is undefined.
*
* INCLUDE FILES: time.h
*
* RETURNS:
* The number of characters in <s>, not including the terminating null
* character -- or zero if the number of characters in <s>, including the null
* character, is more than <n> (in which case the contents of <s> are
* indeterminate).
*/

size_t strftime
    (
    char *            s,		/* string array */
    size_t            n,		/* maximum size of array */
    const char *      format,		/* format of output string */
    const struct tm * tptr		/* broken-down time */
    )
    {
    return (strftime_r (s, n, format, tptr, __loctime));
    }

/*******************************************************************************
*
* strftime_r - format time into a string (POSIX)
*
* Re-entrant version of strftime().
*
* RETURNS:
*/

LOCAL size_t strftime_r
    (
    char *            bufHd,		/* string array */
    size_t            bufMaxSz,		/* maximum size of array */
    const char *      fmtHd,		/* format of output string */
    const struct tm * tmptr, 		/* broken-down time */
    TIMELOCALE *      timeInfo		/* locale information */
    )
    {
    const char *      fmt = fmtHd;
    char *            buffer = bufHd;
    char              addOn[MaxBufferSize];
    int               bufLen = 0;
    int               bufszNow = 0;
    
    FOREVER
       	{
       	while ((*fmt != '%') && (bufszNow != bufMaxSz) && (*fmt != EOS))
	    {
    	    bufszNow++; 
	    *buffer++ = *fmt++;
	    }

       	if (bufszNow == bufMaxSz) 
	    break;

       	if (*fmt++ != EOS)
	    {
	    __generateTime (addOn, tmptr, timeInfo, &bufLen, fmt);
	    if (bufLen >= 0)
	        {
	        if (bufMaxSz > (bufszNow + bufLen))
	            {
	            memcpy (buffer, addOn, bufLen);
	 	    bufszNow += bufLen;
		    buffer += bufLen;
		    fmt++;
		    } /* endif */
	        else 
		    {
		    memcpy (buffer, addOn, bufMaxSz - bufszNow);
		    buffer += (bufMaxSz - bufszNow);
		    bufszNow = bufMaxSz;
		    break;
		    }
		}	
	    else 
	        { /* process format strings */
		*(addOn + abs (bufLen)) = EOS; 

		/* This is recursive - but should recurse ONCE only */
	        bufLen = (int) strftime_r (buffer, 
					   bufMaxSz - bufszNow, 
					   addOn, 
					   tmptr, 
					   timeInfo);
	        buffer += bufLen;
	        bufszNow += bufLen;
	        fmt++;
	        } /* endif */	
	     }
	 else break;
         } /* end forever */
    *buffer = EOS;
    return (bufszNow);
    }

/*******************************************************************************
*
* __generateTime - generate a string representing the format indicator.
*
* Internal routine
*
* RETURNS:
* NOMANUAL
*/

LOCAL void __generateTime
    (
    char *            addOn,	/* string buffer */
    const struct tm * tmptr, 	/* broken-down time */
    TIMELOCALE *      timeInfo,	/* locale information */
    int *             pnum,	/* position number for strftime string */
    const char *      fmt	/* format to be decoded */
    )
    {
    switch (*fmt) 
    	{
        case 'a':	/* day */
    	    __getDay (addOn, tmptr->tm_wday, ABBR, timeInfo, pnum); 
    	    break;
        case 'A':	/* day */
    	    __getDay (addOn, tmptr->tm_wday, FULL, timeInfo, pnum); 
    	    break;
        case 'b':	/* month */
    	    __getMonth (addOn, tmptr->tm_mon, ABBR, timeInfo, pnum); 
    	    break;
        case 'B':	/* month */
    	    __getMonth (addOn, tmptr->tm_mon, FULL, timeInfo, pnum);
    	    break;
        case 'c': 	/* date and time */
    	    __getLocale (addOn, DATETIME, tmptr, timeInfo, pnum);
    	    *pnum = -*pnum;
    	    break;
        case 'd': 	/* day of month */
    	    __intToStr (addOn, tmptr->tm_mday, *pnum = 2); 
    	    break;
        case 'H':	/* hour */
    	    __intToStr (addOn, tmptr->tm_hour, *pnum = 2); 
    	    break;
        case 'I':	/* hour */
    	    __intToStr (addOn, tmptr->tm_hour % 12, *pnum = 2); 
    	    break;
        case 'j':	/* day of year */
    	    __intToStr (addOn, tmptr->tm_yday + 1, *pnum = 3); 
    	    break;
        case 'm':	/* month */
    	    __intToStr (addOn, tmptr->tm_mon + 1, *pnum = 2); 
    	    break;
        case 'M':	/* minute */
    	    __intToStr (addOn, tmptr->tm_min, *pnum = 2); 
    	    break;
        case 'p':	/* AP/PM */
    	    __getLocale (addOn, AMPM, tmptr, timeInfo, pnum);
    	    break;
        case 'S':	/* second */
    	    __intToStr (addOn, tmptr->tm_sec, *pnum = 2); 
       	    break;
        case 'U':	/* week number */
            __intToStr (addOn, 
			__weekOfYear(TM_SUNDAY, tmptr->tm_wday, tmptr->tm_yday),
			*pnum = 2);
    	    break;
        case 'w':	/* weekday */
    	    __intToStr (addOn, tmptr->tm_wday, *pnum = 1); 
    	    break;
        case 'W':	/* week number */
    	    __intToStr (addOn, 
    		        __weekOfYear(TM_MONDAY, tmptr->tm_wday, tmptr->tm_yday),
     	                *pnum = 2);
    	    break;
        case 'x':	/* date */
    	    __getLocale (addOn, DATE, tmptr, timeInfo, pnum);
    	    *pnum = -*pnum;
    	    break;
        case 'X':	/* time */
    	    __getLocale (addOn, TIMEO, tmptr, timeInfo, pnum);
       	    *pnum = -*pnum;
    	    break;
        case 'y':	/* year */
    	    __intToStr (addOn, (tmptr->tm_year % CENTURY), *pnum = 2);
    	    break;
        case 'Y':	/* year */
    	    __intToStr (addOn, (tmptr->tm_year + TM_YEAR_BASE), *pnum = 4);
    	    break;
        case 'Z':	/* zone */
    	    __getLocale (addOn, ZONE, tmptr, timeInfo, pnum);
    	    break;
        case '%':	/* % */
    	    memcpy (addOn, CHAR_FROM_CONST ("%"), 1); 
	    *pnum = 1; 
    	    break;
        default:
    	    *pnum = 0;
    	    break;
    	}
    }

/*******************************************************************************
*
* __weekOfYear - calulate week of year given julian day and week day.
*
* Internal routine
* The <start> determins whether the week starts on Sunday or Monday.
*
* RETURNS:	week of year 
* NOMANUAL
*/

LOCAL int __weekOfYear
    (
    int start,  /* either TM_SUNDAY or TM_MONDAY */
    int wday,   /* days since sunday */
    int yday    /* days since Jan 1 */
    )
    {
    wday = (wday - start + DAYSPERWEEK) % DAYSPERWEEK;
    return ((yday - wday + 12) / DAYSPERWEEK - 1);
    }

/*******************************************************************************
*
* __getLocale - determine locale information given an indicator or the
*		   type of information needed.
*
* Internal routine
*
* RETURNS: void
* NOMANUAL
*/

LOCAL void __getLocale
    (
    char *	      buffer,		/* buffer for the string */
    int               desc, 		/* descriptor */
    const struct tm * tmptr, 		/* broken-down time */
    TIMELOCALE *      timeInfo,		/* locale information */
    int *             size		/* size of array returned */
    )
    {
    char *	      p = NULL;
    char 	      zoneBuf [sizeof (ZONEBUFFER)];

    switch(desc)
    	{
        case DATETIME:
    	    p = timeInfo->_Format [DATETIME]; 
            break;
        case DATE:
    	    p = timeInfo->_Format [DATE]; 
            break;
        case TIMEO:
    	    p = timeInfo->_Format [TIMEO]; 
            break;
        case AMPM:
    	    p = timeInfo->_Ampm [(tmptr->tm_hour <= 12) ? 0 : 1];
            break;
        case ZONE:
    	    (void) __getZoneInfo (zoneBuf, NAME, timeInfo);
	    p = zoneBuf;
            break;
    	}

    *size = strlen (p);
    strcpy(buffer, p);
    }

/*******************************************************************************
*
* __intToStr - convert an integer into a string of <sz> size with leading 
*		  zeroes if neccessary.
*
* Internal routine
*
* RETURNS: void
* NOMANUAL
*/

LOCAL void __intToStr
    (
    char * buffer,	/* buffer for return string */
    int    number,	/* the number to be converted */
    int    size		/* size of the string, maximum length of 4 */
    )
    {
    if (number < 0) 
	number = 0;

    for (buffer += size; 0 <= --size; number /= 10)
	{
	*--buffer = number % 10 + '0';
	}
    }

/*******************************************************************************
*
* __getDay - determine the locale representation of the day of the week
*
* RETURNS: void
* NOMANUAL
*/

LOCAL void __getDay
    (
    char *       buffer,	/* buffer for return string */
    int          index, 	/* integer representation of day of the week */
    int          abbr,		/* abbrievation or full spelling */
    TIMELOCALE * timeInfo,	/* locale information */
    int *        size		/* size of the string returned */
    )
    {
    char *       dayStr;

    index += (abbr == ABBR) ? 0 : DAYSPERWEEK;
    *size = strlen (dayStr = timeInfo->_Days [index]);

    strcpy (buffer, dayStr);
    }

/*******************************************************************************
*
* __getMonth - determine the locale representation of the month of the year 
*
* RETURNS: void
* NOMANUAL
*/

LOCAL void __getMonth
    (
    char *       buffer,	/* buffer for return string */
    int          index, 	/* integer representation of month of year */
    int          abbr,		/* abbrievation or full spelling */
    TIMELOCALE * timeInfo,	/* locale information */
    int *        size		/* size of the string returned */
    )
    {
    char *       monStr;

    index += (abbr == ABBR) ? 0 : MONSPERYEAR;
    *size = strlen (monStr = timeInfo->_Months [index]);

    strcpy (buffer, monStr);
    }

/******************************************************************************
*
*  __getDstInfo - determins whether day light savings is in effect.
*
* TIMEZONE is of the form 
*     <name of zone>::<time in minutes from UTC>:<daylight start>:<daylight end>
*
*	daylight information is expressed as mmddhh ie. month:day:hour
*
*	e.g. UTC::0:040102:100102
*
* RETURNS: FALSE if not on, TRUE otherwise.
* NOMANUAL
*/

int __getDstInfo	
    (
    struct tm *  timeNow, 
    TIMELOCALE * timeInfo
    )
    {
    char *       start = NULL;
    char *       end = NULL;
    char *       dummy = NULL;
    char *       envInfo = NULL;
    char *       last = "";
    int          monStart = 0;
    int          monEnd   = 0;
    int          dayStart = 0;
    int          dayEnd   = 0;
    int          hrStart  = 0;
    int          hrEnd    = 0;
    char         numstr [2];
    char         buffer [sizeof (ZONEBUFFER)];

    /* Is daylight saving in effect? '0' NO; '>0' YES; '<0' don't know */
    /* commented out (SPR #2521)
    if (timeNow->tm_isdst != -1) 
        return (FALSE);
    */

    /* Is dst information stored in environmental variables */
    if ((envInfo = getenv("TIMEZONE")) != NULL) 
    	{
	strcpy (buffer, envInfo);
    	dummy = strtok_r (buffer, ":", &last); 			/* next */
    	dummy = strtok_r (NULL, ":", &last);   			/* next */
    	/* commented out (SPR #2521) */
/*     	dummy = strtok_r (NULL, ":", &last);   			/@ next */
    	start = strtok_r (NULL, ":", &last); 			/* start time */
    	end = strtok_r (NULL, ":", &last);   			/* end time */
    	}
    else
    	{
    	/* Is dst information stored in the locale tables */
    	start = timeInfo->_Isdst[DSTON];
    	end = timeInfo->_Isdst[DSTOFF];
    	if ((strcmp (start,"") == 0) || (strcmp (end,"") == 0))
    	    return (FALSE);
    	}

    if ((start == NULL) || (end == NULL)) 
        return (FALSE);

    /* analyze the dst information of the form 'mmddhh' */

    monStart = (atoi (strncpy (numstr, start, 2))) - 1;       	/*start month */
    monEnd = atoi (strncpy (numstr, end, 2)) - 1;           	/* end month */
    if ((timeNow->tm_mon < monStart) || (timeNow->tm_mon > monEnd)) 
    	return (FALSE);

    if ((timeNow->tm_mon == monStart) || (timeNow->tm_mon == monEnd))
    	{
    	dayStart = atoi (strncpy (numstr, start+2, 2)); 	/* start day */
    	dayEnd = atoi (strncpy (numstr, end+2, 2));     	/* end day */
    	if (((timeNow->tm_mon == monStart) && (timeNow->tm_mday < dayStart)) || 
    	    ((timeNow->tm_mon == monEnd) && (timeNow->tm_mday > dayEnd))) 
    	    return (FALSE);

    	if (((timeNow->tm_mon == monStart) && (timeNow->tm_mday == dayStart)) ||
    	    ((timeNow->tm_mon == monEnd) && (timeNow->tm_mday == dayEnd))) 
    	    {
    	    hrStart = atoi (strncpy (numstr, start+4, 2)); 	/* hour */
    	    hrEnd = atoi (strncpy (numstr, end+4, 2));     	/* hour */
    	    return ((((timeNow->tm_mon==monStart) &&
    		     (timeNow->tm_mday==dayStart) &&
    		     (timeNow->tm_hour < hrStart)) ||
    		    ((timeNow->tm_mon == monEnd) &&
    		     (timeNow->tm_mday==dayEnd) &&
    		     (timeNow->tm_hour > hrEnd)))
    		   ? FALSE : TRUE);
    	    }
    	}
    return (TRUE);
    }

/******************************************************************************
*
*  __getZoneInfo - determins in minutes the time difference from UTC.
*
* If the environment variable TIMEZONE is set then the information is
* retrieved from this variable, otherwise from the locale information.
*
* RETURNS: time in minutes from UTC.
* NOMANUAL
*/

void __getZoneInfo
    (
    char *       buffer,
    int          option, 	/* determine which part of zone information */
    TIMELOCALE * timeInfo	/* locale information */
    )
    {
    char *       envInfo;
    char *       limitStartPtr;
    char *       limitEndPtr;

    /* Time zone information from the environment variable */
    envInfo = getenv ("TIMEZONE");

     if ((envInfo != NULL) && (strcmp (envInfo, "") != 0)) 
    	{
	limitEndPtr = strpbrk (envInfo, ":");
	if (option == NAME)
	    {
	    strcpy (buffer, envInfo);
	    *(buffer + (limitEndPtr - envInfo)) = EOS;
	    return;
	    }

	limitStartPtr = limitEndPtr + 1;
	limitEndPtr = strpbrk (limitStartPtr, ":"); 
	if (option == NAME2)
	    {
	    strcpy (buffer, limitStartPtr);
	    *(buffer + (limitEndPtr - limitStartPtr)) = EOS;
	    return;
	    }

	limitStartPtr = limitEndPtr + 1;
	limitEndPtr = strpbrk (limitStartPtr, ":"); 
	if (option == TIMEOFFSET)
	    {
	    strcpy (buffer, limitStartPtr);
	    if (limitEndPtr != NULL)
	        *(buffer + (limitEndPtr - limitStartPtr)) = EOS;
	    return;
	    }
    	}
    else
	{
        /* Time zone information from the locale table */
        if (strcmp (timeInfo->_Zone [option], "") != 0)
	    strcpy(buffer,timeInfo->_Zone [option]);
        else	 
            *buffer = EOS;
        }
    }
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
