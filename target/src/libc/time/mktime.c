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
