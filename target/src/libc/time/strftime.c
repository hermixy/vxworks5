/* strftime.c - strftime file for time  */

/* Copyright 1991-1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01l,05aug99,cno  atoi calls need byte for null termination (SPR28245)
01k,23jul00,jrp  SPR 27606 End of DST miscalculated.
01j,08jun99,map  fixed 12hr reporting (SPR# 7499)
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
            __intToStr (addOn, ((tmptr->tm_hour % 12) ?
                                (tmptr->tm_hour % 12) : 12), *pnum = 2);
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
    	    p = timeInfo->_Ampm [(tmptr->tm_hour < 12) ? 0 : 1];
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
    char         numstr [4];   /* SPR 28245: changed from 2 to 4 chars */
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

    bzero (numstr, sizeof (numstr));  /* clear numstr before null terminating */
    monStart = (atoi (strncpy (numstr, start, 2))) - 1;       	/*start month */
    bzero (numstr, sizeof (numstr));  /* clear numstr before null terminating */
    monEnd = atoi (strncpy (numstr, end, 2)) - 1;           	/* end month */
    if ((timeNow->tm_mon < monStart) || (timeNow->tm_mon > monEnd)) 
    	return (FALSE);

    if ((timeNow->tm_mon == monStart) || (timeNow->tm_mon == monEnd))
    	{
        bzero (numstr, sizeof (numstr));                    /* clear numstr */
    	dayStart = atoi (strncpy (numstr, start+2, 2)); 	/* start day */
        bzero (numstr, sizeof (numstr));                    /* clear numstr */
    	dayEnd = atoi (strncpy (numstr, end+2, 2));     	/* end day */
    	if (((timeNow->tm_mon == monStart) && (timeNow->tm_mday < dayStart)) || 
    	    ((timeNow->tm_mon == monEnd) && (timeNow->tm_mday > dayEnd))) 
    	    return (FALSE);

    	if (((timeNow->tm_mon == monStart) && (timeNow->tm_mday == dayStart)) ||
    	    ((timeNow->tm_mon == monEnd) && (timeNow->tm_mday == dayEnd))) 
    	    {
            bzero (numstr, sizeof (numstr));                /* clear numstr */
    	    hrStart = atoi (strncpy (numstr, start+4, 2)); 	/* hour */
            bzero (numstr, sizeof (numstr));                /* clear numstr */
    	    hrEnd = atoi (strncpy (numstr, end+4, 2));     	/* hour */
        /*
         * SPR 27606.  DST requires a fallback when the local DST reaches
         * the ending hour on the required date.  The timeNow structure
         * holds the non-DST corrected time. Hence for the end of DST check
         * we assume a DST corrected time (tm_hour+1) and test to see if the
         * hour is greater than or *equal* to the ending hour
         */
    	    return ((((timeNow->tm_mon==monStart) &&
    		     (timeNow->tm_mday==dayStart) &&
    		     (timeNow->tm_hour < hrStart)) ||
    		    ((timeNow->tm_mon == monEnd) &&
    		     (timeNow->tm_mday==dayEnd) &&
                 ((timeNow->tm_hour+1) >= hrEnd)))  /* SPR 27606 */
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
