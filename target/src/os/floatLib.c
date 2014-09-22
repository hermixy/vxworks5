/* floatLib.c - floating-point formatting and scanning library */

/* Copyright 1984-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
02f,19oct01,dcb  Fix the routine title line to match the coding conventions.
02e,14mar99,jdi  doc: removed refs to config.h and/or configAll.h (SPR 25663).
02d,30jul96,dbt  fixed floatScan to avoid loose of precision (SPR #3829).
 		 Updated copyright.
02c,01dec94,ism  fixed alt 'g' format bug.  Added in-line docs.
02b,06oct94,ism  fixed %.XXg outputting more than XX sig figs (SPR#3695)
02a,27jan93,jdi  documentation cleanup for 5.1; fixed SPR 1399.
01z,30jul92,kdl  Restored 01w changes (clobbered by 01x); removed fpTypeGet()
		 and isInf, isNan, & isZero macros.
01y,30jul92,rrr  Removed decl of DOUBLE (now in mathLibP.h)
01x,30jul92,smb  (Accidental checkin.)
01w,30jul92,kdl  Significantly reworked for new ansi fio; moved frexp(), 
	   +jcf	 ldexp(), modf() to src/libc/math; moved DOUBLE union 
		 definition to mathP.h; removed special handling for i960.
01v,18jul92,smb  Changed errno.h to errnoLib.h.
01u,13jul92,smb  frexp, ldexp, modf now in libc for the SPARC (temporary change)
01t,26may92,rrr  the tree shuffle
01s,04mar92,wmd  placed declaration of atof() outside of 960 conditional,
		 included string.h.
01r,10dec91,gae  added includes for ANSI.
01q,28oct91,wmd  added in type casting for *pCh in floatScan as per Intel mods.
01p,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed includes to have absolute path from h/
		  -fixed #else and #endif
		  -changed VOID to void
		  -changed copyright notice
01o,09jun91,del  integration of mods by intel for interfacing to gnu960
		 libraries and fix bugs from 01m.
01n,05apr91,jdi	 documentation -- removed header parens and x-ref numbers;
		 doc review by dnw.
01m,25mar91,del  interfaces to gnu960 flt. pt. libraries for I960CA
01l,07feb91,jaa	 documentation cleanup.
01k,23aug90,elr  Documentation
01j,01aug90,dnw  fixed bug in floatScan scanning <nn>e<nn> format numbers.
		 changed floatFormat to take pVaList instead of vaList.
01i,04jun90,dnw  changed floatFormat to take a vararg va_list instead of a
		   double arg, so that fioLib can be free of all flt pt refs.
		 replaced floatAtoF with new routine floatScan, written
		   from scratch, with our own algorithm for scanning flt pt
		   numbers, designed to work with scanField() in fioLib.
		 fixed bug incorrectly scanning large numbers that are too big.
		 fixed documentation.
		 make ecvtb, fcvtb, gcvtb be "no manual".
01h,07mar90,jdi  documentation cleanup.
01g,20apr89,dab  fixed precision bug in cvtb().
		 documentation touchup in fcvtb() and cvtb().
01f,05jun88,dnw  changed name from fltLib to floatLib.
01e,30may88,dnw  changed to v4 names.
01d,28may88,dnw  changed atof() to fltAtoF().
01c,17mar88,gae  added 'E' and 'G' format specifiers.
01b,05nov87,jlf  documentation
01a,01aug87,gae  written/extracted from fioLib.c.
*/

/*
DESCRIPTION
This library provides the floating-point I/O formatting and scanning
support routines.

The floating-point formatting and scanning support routines are not
directly callable; they are connected to call-outs in the printf()/scanf()
family of functions in fioLib.  This is done dynamically by the routine
floatInit(), which is called by the root task, usrRoot(), in usrConfig.c
when the configuration macro INCLUDE_FLOATING_POINT is defined.
If this option is omitted (i.e., floatInit() is not called), floating-point
format specifications in printf() and sscanf() are not supported.

INCLUDE FILES: math.h

SEE ALSO: fioLib
*/

#include "vxWorks.h"
#include "vwModNum.h"
#include "fioLib.h"
#include "string.h"
#include "stdlib.h"
#include "ctype.h"
#include "string.h"
#include "errnoLib.h"
#include "math.h"
#include "limits.h"
#include "stdarg.h"
#include "fioLib.h"
#include "private/floatioP.h"
#include "private/mathP.h"


/* macros */

#define	to_digit(c)	((c) - '0')
#define is_digit(c)	((unsigned)to_digit(c) <= 9)
#define	to_char(n)	((n) + '0')


/* forward static functions */

LOCAL int 	floatFormat (va_list *pVaList, int precision, BOOL doAlt, 
		             int fmtSym, BOOL *pDoSign, char *pBufStart, 
		             char *pBufEnd);
LOCAL BOOL 	floatScan (int *pReturn, int returnSpec, int fieldwidth,
		           FUNCPTR getRtn, int getArg, int *pCh, int *pNchars);
LOCAL int 	cvt (double number, int prec, BOOL doAlt, int fmtch,
	       	     BOOL *pDoSign, char *startp, char *endp);
LOCAL char *	exponentCvt (char *p, int exp, int fmtch);
LOCAL char *	roundCvt (double fract, int *exp, char *start, char *end, 
			  char ch, BOOL *pDoSign);
LOCAL int	floatFpTypeGet ( double v, double * r);


/*******************************************************************************
*
* floatInit - initialize floating-point I/O support
*
* This routine must be called if floating-point format specifications are
* to be supported by the printf()/scanf() family of routines.
* If the configuration macro INCLUDE_FLOATING_POINT is defined, it is called
* by the root task, usrRoot(), in usrConfig.c.
*
* RETURNS: N/A
*/

void floatInit (void)
    {
    fioFltInstall ((FUNCPTR)floatFormat, (FUNCPTR)floatScan);
    }

/*******************************************************************************
*
* floatFormat - format arg for output
*
* This routine converts from a floating-point value to an ASCII representation.
* The <fmtSym> parameter indicates the format type desired; the actual work
* is done in cvt.
*
* RETURNS: number of characters placed in buffer
*/

LOCAL int floatFormat 
    (
    va_list *	pVaList,	/* vararg list */
    int		precision,	/* precision */
    BOOL	doAlt, 		/* doAlt boolean */
    int		fmtSym,		/* format symbol */
    BOOL *	pDoSign, 	/* where to fill in doSign result */
    char *	pBufStart,	/* buffer start */
    char *	pBufEnd		/* buffer end */
    )
    {
    double dbl = (double) va_arg (*pVaList, double);

    *pDoSign = FALSE;				/* assume no sign needed */

    if (isInf(dbl)) 				/* infinite? */
	{
	strcpy (pBufStart, "Inf");		/* fill in the string */
	if (dbl < 0.0)				/* less than 0.0 */
	    *pDoSign = TRUE;			/* we need a sign */
	return (-3);				/* length will be three */
	}

    if (isNan(dbl)) 				/* not a number? */
	{
	strcpy (pBufStart, "NaN");		/* fill in the string */
	return (-3);				/* length will be three */
	}

    return (cvt (dbl, precision, doAlt, fmtSym, pDoSign, pBufStart, pBufEnd));
    }

/******************************************************************************
* 
* cvt - helper function for floatFormat
* 
* RETURNS: 
*/

LOCAL int cvt
    (
    double	number,
    FAST int	prec,
    BOOL	doAlt,
    int		fmtch,
    BOOL *	pDoSign,
    char *	startp,
    char *	endp
    )
    {
    FAST char *	p;
    FAST char *	t;
    FAST double fract;
    int 	dotrim;
    int 	expcnt;
    int 	gformat;
	int		nonZeroInt=FALSE;
    double 	integer;
    double	tmp;

    dotrim = expcnt = gformat = 0;

    if (number < 0) 
	{
	number = -number;
	*pDoSign = TRUE;
	}
    else
	*pDoSign = FALSE;

    fract = modf(number, &integer);

	if(integer!=(double)0.0)
		nonZeroInt=TRUE;

    t = ++startp;			/* get an extra slot for rounding. */

    /* get integer portion of number; put into the end of the buffer; the
     * .01 is added for modf(356.0 / 10, &integer) returning .59999999...
     */

    for (p = endp - 1; integer; ++expcnt) 
	{
	tmp  = modf(integer / 10, &integer);
	*p-- = to_char((int)((tmp + .01) * 10));
	}

    switch (fmtch) 
	{
	case 'f':
		/* reverse integer into beginning of buffer */
		if (expcnt)
		    for (; ++p < endp; *t++ = *p);
		else
		    *t++ = '0';

		/* if precision required or alternate flag set, add in a
		 * decimal point.
		 */

		if (prec || doAlt)
		    *t++ = '.';

		/* if requires more precision and some fraction left */

		if (fract) 
		    {
		    if (prec)
			do 
			    {
			    fract = modf(fract * 10, &tmp);
			    *t++  = to_char((int)tmp);
			    } while (--prec && fract);

		    if (fract)
			startp = roundCvt (fract, (int *)NULL, startp, t - 1,
				       (char)0, pDoSign);
		    }

		for (; prec--; *t++ = '0')
		    ;
		break;

	case 'e':
	case 'E':
eformat:	if (expcnt) 
		    {
		    *t++ = *++p;

		    if (prec || doAlt)
			*t++ = '.';

		    /* if requires more precision and some integer left */

		    for (; prec && ++p < endp; --prec)
			*t++ = *p;

		    /* if done precision and more of the integer component,
		     * round using it; adjust fract so we don't re-round
		     * later.
		     */

		    if (!prec && ++p < endp)
			{
			fract  = 0;
			startp = roundCvt((double)0, &expcnt, startp, t - 1, *p,
				       pDoSign);
			}

		    --expcnt;	/* adjust expcnt for dig. in front of decimal */
		    }

		/* until first fractional digit, decrement exponent */

		else if (fract) 
		    {
		    /* adjust expcnt for digit in front of decimal */

		    for (expcnt = -1;; --expcnt) 
			{
			fract = modf(fract * 10, &tmp);
			if (tmp)
			    break;
			}

		    *t++ = to_char((int)tmp);

		    if (prec || doAlt)
			*t++ = '.';
		    }
		else 
		    {
		    *t++ = '0';
		    if (prec || doAlt)
			*t++ = '.';
		    }

		/* if requires more precision and some fraction left */
		if (fract) 
		    {
		    if (prec)
			do 
			    {
			    fract = modf(fract * 10, &tmp);
			    *t++ = to_char((int)tmp);
			    } while (--prec && fract);

		    if (fract)
			startp = roundCvt(fract, &expcnt, startp, t - 1,(char)0,
				          pDoSign);
		    }

		for (; prec--; *t++ = '0')	/* if requires more precision */
		    ;

		/* unless alternate flag, trim any g/G format trailing 0's */

		if (gformat && !doAlt) 
		    {
		    while (t > startp && *--t == '0')
			;

		    if (*t == '.')
			--t;
		    ++t;
		    }

		t = exponentCvt (t, expcnt, fmtch);
		break;

	case 'g':
	case 'G':

		/* a precision of 0 is treated as a precision of 1. */

		if (!prec)
		    ++prec;

		/*
		 * ``The style used depends on the value converted; style e
		 * will be used only if the exponent resulting from the
		 * conversion is less than -4 or greater than the precision.''
		 *	-- ANSI X3J11
		 */

		if (expcnt > prec || !expcnt && fract && fract < .0001) 
		    {
		    /*
		     * g/G format counts "significant digits, not digits of
		     * precision; for the e/E format, this just causes an
		     * off-by-one problem, i.e. g/G considers the digit
		     * before the decimal point significant and e/E doesn't
		     * count it as precision.
		     */
		    --prec;
		    fmtch  -= 2;		/* G->E, g->e */
		    gformat = 1;
		    goto eformat;
		    }

		/*
		 * reverse integer into beginning of buffer,
		 * note, decrement precision
		 */

		if (expcnt)
		    for (; ++p < endp; *t++ = *p, --prec)
			;
		else
		    *t++ = '0';

		/*
		 * if precision required or alternate flag set, add in a
		 * decimal point.  If no digits yet, add in leading 0.
		 */

		if (prec || doAlt) 
		    {
		    dotrim = 1;
		    *t++ = '.';
		    }
		else
		    dotrim = 0;

		/* if requires more precision and some fraction left */

		if (fract) 
		    {
		    if (prec) 
			{

			/*
			 * If there is a zero integer portion, so we can't count any
             * of the leading zeros as significant.  Roll 'em on out until
			 * we get to the first non-zero one.
			 */
			if (!nonZeroInt)
				{
				do 
			    	{
			    	fract = modf(fract * 10, &tmp);
			    	*t++ = to_char((int)tmp);
			    	} while(!tmp);
				prec--;
				}

			/*
             * Now add on a number of digits equal to our precision.
             */
			while (prec && fract) 
			    {
			    fract = modf(fract * 10, &tmp);
			    *t++ = to_char((int)tmp);
				prec--;
			    }
			}
		    if (fract)
			startp = roundCvt (fract, (int *)NULL, startp, t - 1,
				           (char)0, pDoSign);
		    }

		/* alternate format, adds 0's for precision, else trim 0's */

		if (doAlt)
	    	for (; prec--; *t++ = '0')
	        	;
		else if (dotrim) 
		    {
		    while (t > startp && *--t == '0')
			;

		    if (*t != '.')
			++t;
		    }

	default :
		break;
	}

    return (t - startp);
    }

/******************************************************************************
*
* roundCvt - helper function for floatFormat
* 
* RETURNS: 
*/

LOCAL char *roundCvt
    (
    double	fract,
    int *	exp,
    FAST char *	start,
    FAST char *	end,
    char 	ch,
    BOOL *	pDoSign
    )
    {
    double tmp;

    if (fract)
	(void)modf(fract * 10, &tmp);
    else
	tmp = to_digit(ch);
    if (tmp > 4)
	{
	for (;; --end) 
	    {
	    if (*end == '.')
		    --end;
	    if (++*end <= '9')
		    break;
	    *end = '0';
	    if (end == start) 
		{
		if (exp) 
		    {				/* e/E; increment exponent */
		    *end = '1';
		    ++*exp;
		    }
		else 
		    {				/* f; add extra digit */
		    *--end = '1';
		    --start;
		    }
		break;
		}
	    }
	}

    /* ``"%.3f", (double)-0.0004'' gives you a negative 0. */

    else if (*pDoSign)
	for (;; --end) 
	    {
	    if (*end == '.')
		--end;

	    if (*end != '0')
		break;

	    if (end == start)
		*pDoSign = FALSE;
	    }

    return (start);
    }

/******************************************************************************
*
* exponentCvt - helper function for floatFormat
* 
* RETURNS: 
*/

LOCAL char *exponentCvt
    (
    FAST char *	p,
    FAST int 	exp,
    int 	fmtch
    )
    {
    FAST char *t;
    char expbuf[MAXEXP];

    *p++ = fmtch;
    if (exp < 0) 
	{
	exp = -exp;
	*p++ = '-';
	}
    else
	*p++ = '+';

    t = expbuf + MAXEXP;

    if (exp > 9) 
	{
	do 
	    {
	    *--t = to_char(exp % 10);
	    } while ((exp /= 10) > 9);

	*--t = to_char(exp);

	for (; t < expbuf + MAXEXP; *p++ = *t++)
	    ;
	}
    else 
	{
	*p++ = '0';
	*p++ = to_char(exp);
	}

    return (p);
    }

/*******************************************************************************
*
* floatScan - scan ASCII input to a floating-point number
*
* This routine scans the ASCII input into a floating-point number.
*
* [+-]<digs>.<digs>e[+-]<digs>
*
* RETURNS: TRUE if successful, FALSE if unsuccessful
*/

LOCAL BOOL floatScan
    (
    FAST int *  pReturn,
    int         returnSpec,     /* 0, 'l', 'L' */
    int         fieldwidth,
    FAST FUNCPTR getRtn,
    FAST int    getArg,
    int *       pCh,
    int *       pNchars
    )
    {
#define DBL_DIG			16			/* float.h */

#define GET_CHAR(ch, ix)	((ix)++, (ch) = getRtn (getArg))

    static double power10 [] =
	    {1e0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8};
    static double posExpPower10 [] =
	    {1e1, 1e2, 1e4, 1e8, 1e16, 1e32, 1e64, 1e128, 1e256, 0};

    FAST int	ch	  = *pCh;	/* current char */
    FAST int	ix	  = 0;		/* number of chars consumed */
    FAST long	long1	  = 0;		/* 1st part of integer representation */
    FAST long	long2	  = 0;		/* 2nd part of integer representation */
    int		ndigs1	  = 0;		/* num of digits in long1 */
    int		ndigs2	  = 0;		/* num of digits in long2 */
    int		ndigsPrec = 0;		/* num of digits of precision total */
    BOOL	fracPart  = FALSE;	/* TRUE = doing digits after '.' */
    int		ndigsFrac = 0;		/* number of digits after '.' */
    int		exp	  = 0;		/* exponent */
    double	num;			/* double representation of number */
    BOOL	negnum = FALSE;		/* number is negative */
    BOOL	negexp = FALSE;		/* exponent is negative */
    double *	pExpPower10;		/* current exponent bit multiplier */

    /* check for sign */

    if (ix < fieldwidth)
	{
	if ((negnum = ((char)ch == '-')) || ((char)ch == '+'))
	    GET_CHAR (ch, ix);
	}

    /* scan integer and fraction parts */

    for (; (ch != EOF) && (ix < fieldwidth); GET_CHAR (ch, ix))
	{
	if (!fracPart && ch == '.')
	    {
	    fracPart = TRUE;
	    continue;
	    }

	if ((ch < '0') || (ch > '9'))
	    break;

	ch -= '0';		/* turn ch into digit */

	if (fracPart)		/* count digits after '.' */
	    ndigsFrac++;

	if ((ndigsPrec != 0) || (ch != 0))	/* skip leading 0s */
	    {
	    ndigsPrec++;

	    /* check if another digit will fit in long1 */

	    if (long1 < ((INT_MAX - 9) / 10))
		{
		long1 = 10 * long1 + ch;
		ndigs1++;
		}
	    else if (ndigsPrec <= DBL_DIG)
		{
		long2 = 10 * long2 + ch;
		ndigs2++;
		}
	    }
	}


    /* scan for explicit exponent */

    if ((ix < fieldwidth) && (ch == 'e' || ch == 'E'))
	{
	GET_CHAR (ch, ix);

	/* skip over sign */

	if (ix < fieldwidth)
	    {
	    if ((negexp = (ch == '-')) || (ch == '+'))
		GET_CHAR (ch, ix);
	    }

	for (; (ch != EOF) && (ix < fieldwidth); GET_CHAR (ch, ix))
	    {
	    if ((ch < '0') || (ch > '9'))
		break;

	    exp = 10 * exp + (ch - '0');
	    }
	}


    /* check that we scanned at least one character */

    if (ix == 0)
	return (FALSE);

    /* put significant digits into double representation */

    num = long1;
    if (ndigs2 != 0)
	num = num * power10 [ndigs2] + long2;


    /* apply exponent to number; total effective exponent =
     *   + number of digits of precision that were scanned but not
     *     represented in the number (ndigsPrec - ndigs1 - ndigs2)
     *   - number of digits scanned after the '.' (ndigsFrac)
     *   + the scanned explicit exponent
     */

    exp = (ndigsPrec - ndigs1 - ndigs2) - ndigsFrac + (negexp ? -exp : exp);

    if ((negexp = (exp < 0)))
	exp = -exp;

    pExpPower10 = posExpPower10;

    while ((exp != 0) && (*pExpPower10 != 0))
	{
	if (exp & 1)
	    {
	    if (negexp) 
	    	num /= *pExpPower10;
	    else
	    	num *= *pExpPower10;
	    }
	exp >>= 1;
	pExpPower10++;
	}

    if (exp != 0)
	{
	/* exponent too big */

	if (negexp)
	    num = 0;
	else
	    {
	    /* Infinity */
	    ((DOUBLE *) &num)->ldat.l1 = 0x7ff00000;
	    ((DOUBLE *) &num)->ldat.l2 = 0;
	    }
	}


    /* return value to caller */

    if (negnum)
	num = -num;

    if (pReturn != NULL)
	{
	switch (returnSpec)
	    {
	    case 'l': * (double *) pReturn = num;	break;
	    default:  * (float *) pReturn = num;	break;
	    }
	}

    *pCh = ch;
    *pNchars += ix;

    return (ix != 0);
    }

