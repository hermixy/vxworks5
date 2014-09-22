/* strtod.c function for stdlib  */

/* Copyright 1992-1995 Wind River Systems, Inc. */

/*
modification history
--------------------
01e,11feb95,jdi  doc fix.
01d,16aug93,dvs  fixed value of endptr for strings that start with d, D, e, or E
		 (SPR #2229)
01c,08feb93,jdi  documentation cleanup for 5.1.
01b,20sep92,smb  documentation additions.
01a,19jul92,smb  written  and documented
*/

/*
DESCRIPTION

INCLUDE FILES: stdlib.h, math.h, assert.h, arrno.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "stdlib.h"
#include "math.h"
#include "errno.h"

#define Ise(c)          ((c == 'e') || (c == 'E') || (c == 'd') || (c == 'D'))
#define Isdigit(c)      ((c <= '9') && (c >= '0'))
#define Isspace(c)      ((c == ' ') || (c == '\t') || (c=='\n') || (c=='\v') \
			 || (c == '\r') || (c == '\f'))
#define Issign(c)       ((c == '-') || (c == '+'))
#define Val(c)          ((c - '0'))

#define MAXE  308
#define MINE  (-308)

static double powtab[] = {1.0,
		          10.0,
			  100.0,
			  1000.0,
			  10000.0};

/* flags */
#define SIGN    0x01
#define ESIGN   0x02
#define DECP    0x04

/* locals */

int             __ten_mul (double *acc, int digit);
double          __adjust (double *acc, int dexp, int sign);
double          __exp10 (uint_t x);

/*****************************************************************************
*
* strtod - convert the initial portion of a string to a double (ANSI) 
*
* This routine converts the initial portion of a specified string <s> to a
* double.  First, it decomposes the input string into three parts:  an initial, 
* possibly empty, sequence of white-space characters (as specified by the 
* isspace() function); a subject sequence resembling a floating-point
* constant; and a final string of one or more unrecognized characters, 
* including the terminating null character of the input string.  Then, it
* attempts to convert the subject sequence to a floating-point number, and 
* returns the result.
*
* The expected form of the subject sequence is an optional plus or minus
* decimal-point character, then an optional exponent part but no floating
* suffix.  The subject sequence is defined as the longest initial
* subsequence of the input string, starting with the first non-white-space
* character, that is of the expected form.  The subject sequence contains
* no characters if the input string is empty or consists entirely of 
* white space, or if the first non-white-space character is other than a 
* sign, a digit, or a decimal-point character.
*
* If the subject sequence has the expected form, the sequence of characters
* starting with the first digit or the decimal-point character (whichever
* occurs first) is interpreted as a floating constant, except that the 
* decimal-point character is used in place of a period, and that if neither
* an exponent part nor a decimal-point character appears, a decimal point is
* assumed to follow the last digit in the string.  If the subject sequence
* begins with a minus sign, the value resulting form the conversion is negated.
* A pointer to the final string is stored in the object pointed to by <endptr>,
* provided that <endptr> is not a null pointer.
*
* In other than the "C" locale, additional implementation-defined subject
* sequence forms may be accepted.  VxWorks supports only the "C" locale.
*
* If the subject sequence is empty or does not have the expected form, no
* conversion is performed; the value of <s> is stored in the object pointed
* to by <endptr>, provided that <endptr> is not a null pointer.
*
* INCLUDE FILES: stdlib.h 
*
* RETURNS:
* The converted value, if any.  If no conversion could be performed, it
* returns zero.  If the correct value is outside the range of representable
* values, it returns plus or minus HUGE_VAL (according to the sign of the
* value), and stores the value of the macro ERANGE in `errno'.  If the
* correct value would cause underflow, it returns zero and stores the value
* of the macro ERANGE in `errno'.
*
*/

double strtod 
    (
    const char * s,	/* string to convert */
    char **      endptr	/* ptr to final string */
    )
    {
    /* Note that the point positioning is locale dependant */
    const char *start     = s;
    double      accum     = 0.0;
    int         flags     = 0;
    int         texp      = 0;
    int         e         = 0;
    BOOL        conv_done = FALSE;

    while (Isspace (*s)) s++;
    if (*s == EOS)
	{   /* just leading spaces */
	if (endptr != NULL) 
	    *endptr = CHAR_FROM_CONST (start);

	return (0.0);
	}

    if (Issign (*s))
	{
	if (*s == '-') flags = SIGN;

	if (*++s == EOS)
	    {   /* "+|-" : should be an error ? */
	    if (endptr != NULL) 
		*endptr = CHAR_FROM_CONST (start);

	    return (0.0);
	    }
	}

    /* added code to fix problem with leading e, E, d, or D */

    if ( !Isdigit (*s) && (*s != '.'))
	{
	if (endptr != NULL)
	    *endptr = CHAR_FROM_CONST (start);

	return (0.0);
	}


    for ( ; (Isdigit (*s) || (*s == '.')); s++)
	{
	conv_done = TRUE;

	if (*s == '.')
	    flags |= DECP;
	else
	    {
	    if ( __ten_mul (&accum, Val(*s)) ) 
		texp++;
	    if (flags & DECP) 
		texp--;
	    }
	}

    if (Ise (*s))
	{
	conv_done = TRUE;

	if (*++s != EOS)             /* skip e|E|d|D */
	    {                         /* ! ([s]xxx[.[yyy]]e)  */

	    while (Isspace (*s)) s++; /* Ansi allows spaces after e */
	    if (*s != EOS)
		{                     /*  ! ([s]xxx[.[yyy]]e[space])  */
		if (Issign (*s))
		    if (*s++ == '-') flags |= ESIGN;

		if (*s != EOS)
                                      /*  ! ([s]xxx[.[yyy]]e[s])  -- error?? */
		    {                
		    for(; Isdigit (*s); s++)
                                      /* prevent from grossly overflowing */
			if (e < MAXE) 
			    e = e*10 + Val (*s);

		                      /* dont care what comes after this */
		    if (flags & ESIGN)
			texp -= e;
		    else
			texp += e;
		    }
		}
	    }
	}

    if (endptr != NULL)
	*endptr = CHAR_FROM_CONST ((conv_done) ? s : start);

    return (__adjust (&accum, (int) texp, (int) (flags & SIGN)));
    }

/*******************************************************************************
*
* __ten_mul -
*
* multiply 64 bit accumulator by 10 and add digit.
* The KA/CA way to do this should be to use
* a 64-bit integer internally and use "adjust" to
* convert it to float at the end of processing.
*
* AUTHOR:
* Taken from cygnus.
*
* RETURNS:
* NOMANUAL
*/

LOCAL int __ten_mul 
    (
    double *acc,
    int     digit
    )
    {
    *acc *= 10;
    *acc += digit;

    return (0);     /* no overflow */
    }

/*******************************************************************************
*
* __adjust -
*
* return (*acc) scaled by 10**dexp.
*
* AUTHOR:
* Taken from cygnus.
*
* RETURNS:
* NOMANUAL
*/

LOCAL double __adjust 
    (
    double *acc,	/* *acc    the 64 bit accumulator */
    int     dexp,   	/* dexp    decimal exponent       */
    int     sign	/* sign    sign flag              */
    )
    {
    double  r;

    if (dexp > MAXE)
	{
	errno = ERANGE;
	return (sign) ? -HUGE_VAL : HUGE_VAL;
	}
    else if (dexp < MINE)
	{
	errno = ERANGE;
	return 0.0;
	}

    r = *acc;
    if (sign)
	r = -r;
    if (dexp==0)
	return r;

    if (dexp < 0)
	return r / __exp10 (abs (dexp));
    else
	return r * __exp10 (dexp);
    }

/*******************************************************************************
*
* __exp10 -
*
*  compute 10**x by successive squaring.
*
* AUTHOR:
* Taken from cygnus.
*
* RETURNS:
* NOMANUAL
*/

double __exp10 
    (
    uint_t x
    )
    {
    if (x < (sizeof (powtab) / sizeof (double)))
	return (powtab [x]);
    else if (x & 1)
	return (10.0 * __exp10 (x-1));
    else
	return (__exp10 (x/2) * __exp10 (x/2));
    }
