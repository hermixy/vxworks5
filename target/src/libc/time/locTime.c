/* locTime.c - time locale information */

/* Copyright 1992 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,20sep92,smb  documentation additions
01b,27jul92,smb	renamed from __timeLoc
01a,25jul92,smb	created.
*/

/*
DESCRIPTION

INCLUDE FILE: stdio.h, stdlib.h, assert.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "private/timeP.h"

static TIMELOCALE  __ctime = { 
	/* DAYS  */	    {"SUN","MON","TUE","WED","THU","FRI","SAT",
	                     "SUNDAY","MONDAY", "TUESDAY","WEDNESDAY",
			     "THURSDAY","FRIDAY", "SATURDAY"},
	/* MONTHS */	    {"JAN","FEB","MAR","APR","MAY","JUN","JUL", 
		 	     "AUG","SEP", "OCT", "NOV","DEC","JANUARY",
			     "FEBRUARY","MARCH", "APRIL","MAY", "JUNE",
			     "JULY","AUGUST","SEPTEMBER","OCTOBER", 
			     "NOVEMBER", "DECEMBER"},
	/* FORMAT */	    {"%b %d %H:%M:%S:%Y","%b %d %Y","%H:%M:%S"},
	/* AMPM */	    {"AM","PM"},
	/* ZONES */	    {"UTC","UTC",""},
	/* DST */	    {"",""}
			    };

TIMELOCALE  *__loctime = &__ctime; 

