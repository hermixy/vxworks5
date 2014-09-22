/* ansiStdlib.c - ANSI 'stdlib' documentation */

/* Copyright 1992-1995 Wind River Systems, Inc. */

/*
modification history
--------------------
01d,11jul97,dgp  doc: SPR 7651 and 7677, specify non-reentrant functions
01c,10feb95,jdi  doc tweaks for div_r(), strtod(), strtol().
01b,08feb93,jdi  documentation cleanup for 5.1.
01a,24oct92,smb  written and documented.
*/

/*
DESCRIPTION
This library includes several standard ANSI routines.  Note that where 
there is a pair of routines, such as div() and div_r(), only the routine
xxx_r() is reentrant.  The xxx() routine is not reentrant.

The header stdlib.h declares four types and several functions of general
utility, and defines several macros.

.SS Types
The types declared are `size_t', `wchar_t', and:
.iP `div_t' 12
is the structure type of the value returned by the div().
.iP `ldiv_t'
is the structure type of the value returned by the ldiv_t().

.SS Macros
The macros defined are NULL and:
.iP "`EXIT_FAILURE', `EXIT_SUCCESS'" 12
expand to integral constant expressions that may be used as the
argument to exit() to return unsuccessful or successful termination
status, respectively, to the host environment.
.iP `RAND_MAX'
expands to a positive integer expression whose value is the maximum
number of bytes on a multibyte character for the extended character set
specified by the current locale, and whose value is never greater
than MB_LEN_MAX.
.LP

INCLUDE FILES: stdlib.h

SEE ALSO: American National Standard X3.159-1989

INTERNAL
This module is built by appending the following files:
    abort.c
    abs.c
    atexit.c
    atof.c
    atoi.c
    atol.c
    bsearch.c
    div.c
    labs.c
    ldiv.c
    multibyte.c
    qsort.c
    rand.c
    strtod.c
    strtol.c
    strtoul.c
    system.c
*/

/* abort.c - abort file for stdlib  */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01d,08feb93,jdi  documentation cleanup for 5.1.
01c,20sep92,smb  documentation additions.
01b,27jul92,smb  abort now raises an abort signal.
01a,19jul92,smb  Phase 1 of ANSI merge.
*/

/*
DESCRIPTION

INCLUDE FILES: stdlib.h, signal.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "stdlib.h"
#include "signal.h"

/******************************************************************************
*
* abort - cause abnormal program termination (ANSI)
*
* This routine causes abnormal program termination, unless the signal
* SIGABRT is being caught and the signal handler does not return.  VxWorks
* does not flush output streams, close open streams, or remove temporary
* files.  abort() returns unsuccessful status termination to the host
* environment by calling:
* .CS
*     raise (SIGABRT);
* .CE
*
* INCLUDE FILES: stdlib.h
*
* RETURNS: This routine cannot return to the caller.
*/

void abort (void)
	{
	raise (SIGABRT);
	exit (EXIT_FAILURE);
	}
/* abs.c - abs file for stdlib  */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,08feb93,jdi  documentation cleanup for 5.1.
01b,20sep92,smb  documentation additions.
01a,19jul92,smb  written and documented.
*/

/*
DESCRIPTION

INCLUDE FILES: stdlib.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "stdlib.h"

/*******************************************************************************
*
* abs - compute the absolute value of an integer (ANSI)
*
* This routine computes the absolute value of a specified integer.  If the
* result cannot be represented, the behavior is undefined.
*
* INCLUDE FILES: stdlib.h
*
* RETURNS: The absolute value of <i>.
*/

int abs 
    (
    int i          /* integer for which to return absolute value */
    )
    {
    return (i >= 0 ? i : -i);
    }
/* atexit.c - atexit file for stdlib  */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,08feb93,jdi  documentation cleanup for 5.1.
01b,20sep92,smb  documentation additions.
01a,19jul92,smb  written.
*/

/*
DESCRIPTION

INCLUDE FILES: stdlib.h, signal.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "stdlib.h"

/*******************************************************************************
*
* atexit - call a function at program termination (Unimplemented) (ANSI)
*
* This routine is unimplemented.  VxWorks task exit hooks
* provide this functionality.
*
* INCLUDE FILES: stdlib.h
*
* RETURNS: ERROR, always.
*
* SEE ALSO: taskHookLib
*/

int atexit 
    (
    void (*__func)(void)	/* pointer to a function */
    ) 
    {
    return (ERROR); 
    }

/* atof.c - atof files for stdlib  */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,08feb93,jdi  documentation cleanup for 5.1.
01b,20sep92,smb  documentation additions.
01a,19jul92,smb  written and documentation.
*/

/*
DESCRIPTION

INCLUDE FILES: stdlib.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "stdlib.h"

/*******************************************************************************
*
* atof - convert a string to a `double' (ANSI)
*
* This routine converts the initial portion of the string <s> 
* to double-precision representation. 
*
* Its behavior is equivalent to:
* .CS
*     strtod (s, (char **)NULL);
* .CE
*
* INCLUDE FILES: stdlib.h
*
* RETURNS: The converted value in double-precision representation.
*/

double atof
    (
    const char * s	/* pointer to string */
    )
    {
    return (strtod (s, (char **) NULL));
    }
/* atoi.c - atoi files for stdlib  */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,08feb93,jdi  documentation cleanup for 5.1.
01b,20sep92,smb  documentation additions.
01a,19jul92,smb  written and documented.
*/

/*
DESCRIPTION

INCLUDE FILES: stdlib.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "stdlib.h"

/******************************************************************************
*
* atoi - convert a string to an `int' (ANSI)
*
* This routine converts the initial portion of the string
* <s> to `int' representation.
*
* Its behavior is equivalent to:
* .CS
*     (int) strtol (s, (char **) NULL, 10);
* .CE
*
* INCLUDE FILES: stdlib.h
*
* RETURNS: The converted value represented as an `int'.
*/

int atoi
    (
    const char * s		/* pointer to string */
    )
    {
    return (int) strtol (s, (char **) NULL, 10);
    }
/* atol.c - atol files for stdlib  */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,08feb93,jdi  documentation cleanup for 5.1.
01b,20sep92,smb  documentation additions.
01a,19jul92,smb  written.
*/

/*
DESCRIPTION

INCLUDE FILES: stdlib.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "stdlib.h"

/*****************************************************************************
*
* atol - convert a string to a `long' (ANSI)
*
* This routine converts the initial portion of the string <s> 
* to long integer representation.
*
* Its behavior is equivalent to:
* .CS
*     strtol (s, (char **)NULL, 10);
* .CE
*
* INCLUDE FILES: stdlib.h
*
* RETURNS: The converted value represented as a `long'.
*/

long atol
    (
    const register char * s		/* pointer to string */
    )
    {
    return strtol (s, (char **) NULL, 10);
    }
/* bsearch.c	- bsearch routine for the stdlib ANSI library */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,08feb93,jdi  documentation cleanup for 5.1.
01b,20sep92,smb  documentation additions.
01a,19jul92,smb  written and documented
*/

/*
DESCRIPTION

INCLUDE FILE: stdlib.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "stdlib.h"

/******************************************************************************
*
* bsearch - perform a binary search (ANSI)
*
* This routine searches an array of <nmemb> objects, the initial element of
* which is pointed to by <base0>, for an element that matches the object
* pointed to by <key>.  The <size> of each element of the array is specified
* by <size>.
*
* The comparison function pointed to by <compar> is called with two arguments
* that point to the <key> object and to an array element, in that order.  The
* function shall return an integer less than, equal to, or greater than zero if
* the <key> object is considered, respectively, to be less than, to match, or
* to be greater than the array element.  The array shall consist of all the
* elements that compare greater than the <key> object, in that order.
*
* INCLUDE FILES: stdlib.h
*
* RETURNS:
* A pointer to a matching element of the array, or a NULL pointer
* if no match is found.  If two elements compare as equal, which element 
* is matched is unspecified.
*/

void * bsearch
    (
    FAST const void *	key,		/* element to match */
    const void *	base0,		/* initial element in array */
    size_t		nmemb,		/* array to search */
    FAST size_t		size,		/* size of array element */
    FAST int (*compar) (const void *, const void *)  /* comparison function */
    )
    {
    FAST const char *     base = base0;
    FAST const void *     p;
    FAST int              lim;
    FAST int              cmp;

    for (lim = nmemb; lim != 0; lim >>= 1) 
	{
    	p = base + (lim >> 1) * size;
    	cmp = (*compar)(key, p);

    	if (cmp == 0)
    	    return (CHAR_FROM_CONST (p));

    	if (cmp > 0) 
	    {				/* key > p: move right */
    	    base = (CHAR_FROM_CONST (p) + size);
    	    lim--;
    	    } 		
        }

    return (NULL);
    }
/* div.c - div file for stdlib  */

/* Copyright 1992-1995 Wind River Systems, Inc. */

/*
modification history
--------------------
01f,10feb95,jdi  doc format tweak.
01e,08feb93,jdi  documentation cleanup for 5.1.
01d,20sep92,smb  documentation additions.
01c,24jul92,smb  corrected parameter ordering.
01b,24jul92,smb  added reentrant version for div()
01a,19jul92,smb  written and documented.
*/

/*
DESCRIPTION

INCLUDE FILES: stdlib.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "stdlib.h"

/*******************************************************************************
*
* div - compute a quotient and remainder (ANSI)
*
* This routine computes the quotient and remainder of <numer>/<denom>.
* If the division is inexact, the resulting quotient is the integer of lesser
* magnitude that is the nearest to the algebraic quotient.  If the result cannot
* be represented, the behavior is undefined; otherwise, `quot' * <denom> + `rem'
* equals <numer>.
*
* This routine is not reentrant.  For a reentrant version, see div_r().
*
* INCLUDE FILES: stdlib.h 
*
* RETURNS:
* A structure of type `div_t', containing both the quotient and the 
* remainder. 
*
* INTERNAL
* The structure shall contain the following members, in either order:
*		int quot; * quotient *
*		int rem;  * remainder *
*/

div_t div 
    (
    int numer, 		/* numerator */
    int denom		/* denominator */
    ) 
    {
    static div_t divStruct;	/* div_t structure */

    div_r (numer, denom, &divStruct);
    return (divStruct);
    }

/*******************************************************************************
*
* div_r - compute a quotient and remainder (reentrant)
*
* This routine computes the quotient and remainder of <numer>/<denom>.
* The quotient and remainder are stored in the `div_t' structure
* pointed to by <divStructPtr>.
*
* This routine is the reentrant version of div().
*
* INCLUDE FILES: stdlib.h 
*
* RETURNS: N/A
*/

void div_r
    (
    int     numer,          /* numerator */
    int     denom,          /* denominator */
    div_t * divStructPtr    /* div_t structure */
    )
    {
    /* calculate quotient */
    divStructPtr->quot = numer / denom;

    /* calculate remainder */
    divStructPtr->rem = numer - (denom * divStructPtr->quot);

    /* check for negative quotient */
    if ((divStructPtr->quot < 0) && (divStructPtr->rem > 0))
        {
        divStructPtr->quot++;
        divStructPtr->rem -= denom;
        }
    }
/* labs.c - labs file for stdlib  */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,08feb93,jdi  documentation cleanup for 5.1.
01b,20sep92,smb  documentation additions.
01a,19jul92,smb  written.
*/

/* 
DESCRIPTION 
 
INCLUDE FILE: stdlib.h
  
SEE ALSO: American National Standard X3.159-1989 

NOMANUAL
*/ 

#include "vxWorks.h"
#include "stdlib.h"

/*******************************************************************************
*
* labs - compute the absolute value of a `long' (ANSI)
*
* This routine computes the absolute value of a specified `long'.  If the
* result cannot be represented, the behavior is undefined.  This routine is
* equivalent to abs(), except that the argument and return value are all of
* type `long'.
*
* INCLUDE FILES: stdlib.h 
*
* RETURNS: The absolute value of <i>.
*/

long labs 
    (
    long i          /* long for which to return absolute value */
    )
    {
    return (i >= 0 ? i : -i);
    }

/* ldiv.c - ldiv file for stdlib  */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01e,08feb93,jdi  documentation cleanup for 5.1.
01d,20sep92,smb  documentation additions.
01c,24jul92,smb  corrected parameter ordering.
01b,24jul92,smb  added reentrant version of ldiv()
01a,19jul92,smb  written.
*/

/*
DESCRIPTION

INCLUDE FILES: stdlib.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "stdlib.h"

/*******************************************************************************
*
* ldiv - compute the quotient and remainder of the division (ANSI)
*
* This routine computes the quotient and remainder of <numer>/<denom>.
* This routine is similar to div(), except that the arguments and the
* elements of the returned structure are all of type `long'.
*
* This routine is not reentrant.  For a reentrant version, see ldiv_r().
*
* INCLUDE FILES: stdlib.h 
*
* RETURNS:
* A structure of type `ldiv_t', containing both the quotient and the 
* remainder. 
*/

ldiv_t ldiv 
    (
    long numer, 	/* numerator */
    long denom		/* denominator */
    ) 
    {
    static ldiv_t divStruct;		

    ldiv_r (numer, denom, &divStruct); 
    return (divStruct);
    }
 
/*******************************************************************************
*
* ldiv_r - compute a quotient and remainder (reentrant)
*
* This routine computes the quotient and remainder of <numer>/<denom>.
* The quotient and remainder are stored in the `ldiv_t' structure
* `divStructPtr'.
*
* This routine is the reentrant version of ldiv().
*
* INCLUDE FILES: stdlib.h 
*
* RETURNS: N/A
*/

void ldiv_r
    (
    long     numer,          /* numerator */
    long     denom,          /* denominator */
    ldiv_t * divStructPtr    /* ldiv_t structure */
    )
    {
    /* calculate quotient */
    divStructPtr->quot = numer / denom;
 
    /* calculate remainder */
    divStructPtr->rem = numer - (denom * divStructPtr->quot);
 
    /* check for negative quotient */
    if ((divStructPtr->quot < 0) && (divStructPtr->rem > 0))
        {
        divStructPtr->quot++;
        divStructPtr->rem -= denom;
        }
    }   
/* multibyte.c - multibyte file for stdlib  */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,08feb93,jdi  documentation cleanup for 5.1.
01b,20sep92,smb  documentation additions.
01a,19jul92,smb  written and documented.
*/

/*
DESCRIPTION
These ignore the current fixed ("C") locale and
always indicate that no multibyte characters are supported.

INCLUDE FILES: stdlib.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "stdlib.h"
#include "string.h"

/******************************************************************************
*
* mblen - calculate the length of a multibyte character (Unimplemented) (ANSI)
* 
* This multibyte character function is unimplemented in VxWorks.
*
* INCLUDE FILES: stdlib.h 
*
* RETURNS: OK, or ERROR if the parameters are invalid.
*/

int mblen
    (
    const char * s,
    size_t       n
    )
    {
    if ((strcmp (s,NULL) == 0) && (n == 0) && (*s == EOS))
    	return (ERROR);

    return (OK);
    }

/******************************************************************************
*
* mbtowc - convert a multibyte character to a wide character (Unimplemented) (ANSI)
* 
* This multibyte character function is unimplemented in VxWorks.
*
* INCLUDE FILES: stdlib.h 
*
* RETURNS: OK, or ERROR if the parameters are invalid.
*/

int mbtowc
    (
    wchar_t *    pwc,
    const char * s,
    size_t       n
    )
    {
    if ((strcmp (s,NULL) == 0) && (n == 0) && (*s == EOS))
    	return (ERROR);

    return (OK);
    }

/******************************************************************************
*
* wctomb - convert a wide character to a multibyte character (Unimplemented) (ANSI)
* 
* This multibyte character function is unimplemented in VxWorks.
*
* INCLUDE FILES: stdlib.h 
*
* RETURNS: OK, or ERROR if the parameters are invalid.
*/

int wctomb
    (
    char *  s, 
    wchar_t wchar
    )
    {
    if (strcmp (s,NULL) == 0)
    	return (ERROR);

    return (OK);
    }

/******************************************************************************
*
* mbstowcs - convert a series of multibyte char's to wide char's (Unimplemented) (ANSI)
* 
* This multibyte character function is unimplemented in VxWorks.
*
* INCLUDE FILES: stdlib.h 
*
* RETURNS: OK, or ERROR if the parameters are invalid.
*/

size_t mbstowcs
    (
    wchar_t *	 pwcs,
    const char * s,
    size_t       n
    )
    {
    if ((strcmp (s,NULL) == 0) && (n == 0) && (*s == EOS))
    	return (ERROR);

    return (OK);
    }

/******************************************************************************
*
* wcstombs - convert a series of wide char's to multibyte char's (Unimplemented) (ANSI)
* 
* This multibyte character function is unimplemented in VxWorks.
*
* INCLUDE FILES: stdlib.h 
*
* RETURNS: OK, or ERROR if the parameters are invalid.
*/

size_t wcstombs
    (
    char *          s,
    const wchar_t * pwcs,
    size_t          n
    )
    {
    if ((strcmp (pwcs,NULL) == 0) && (n == 0) && (*pwcs == (wchar_t) EOS))
    	return (ERROR);

    return (OK);
    }
/* qsort.c - qsort file for stdlib  */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01d,01jul93,jmm  fixed parameter order of quick_sort and insertion_sort (spr 2202)
01c,08feb93,jdi  documentation cleanup for 5.1.
01b,20sep92,smb  documentation additions.
01a,19jul92,smb  written and documented.
*/

/*
DESCRIPTION
 * Copyright (c) 1980, 1983, 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
INCLUDE FILES: stdlib.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "stdlib.h"

/*
 * MTHRESH is the smallest partition for which we compare for a median
 * value instead of using the middle value.
 */
#define	MTHRESH	6

/*
 * THRESH is the minimum number of entries in a partition for continued
 * partitioning.
 */
#define	THRESH	4

static void insertion_sort(), quick_sort(); 


/*
 * Swap two areas of size number of bytes.  Although qsort(3) permits random
 * blocks of memory to be sorted, sorting pointers is almost certainly the
 * common case (and, were it not, could easily be made so).  Regardless, it
 * isn't worth optimizing; the SWAP's get sped up by the cache, and pointer
 * arithmetic gets lost in the time required for comparison function calls.
 */
#define	SWAP(a, b) 						\
    {						 		\
    cnt = size; 						\
    do 								\
	{ 							\
    	ch = *a; 						\
    	*a++ = *b; 						\
    	*b++ = ch; 						\
        } while (--cnt); 					\
    }

/*
 * Knuth, Vol. 3, page 116, Algorithm Q, step b, argues that a single pass
 * of straight insertion sort after partitioning is complete is better than
 * sorting each small partition as it is created.  This isn't correct in this
 * implementation because comparisons require at least one (and often two)
 * function calls and are likely to be the dominating expense of the sort.
 * Doing a final insertion sort does more comparisons than are necessary
 * because it compares the "edges" and medians of the partitions which are
 * known to be already sorted.
 *
 * This is also the reasoning behind selecting a small THRESH value (see
 * Knuth, page 122, equation 26), since the quicksort algorithm does less
 * comparisons than the insertion sort.
 */
#define	SORT(bot, n) 						\
    { 								\
    if (n > 1) 							\
    	if (n == 2) 						\
	    { 							\
    	    t1 = bot + size; 					\
    	    if (compar (t1, bot) < 0) 				\
    	    	SWAP (t1, bot); 				\
    	    } 							\
	else 							\
    	    insertion_sort(bot, n, size, compar); 		\
    }

/******************************************************************************
*
* qsort - sort an array of objects (ANSI)
*
* This routine sorts an array of <nmemb> objects, the initial element of
* which is pointed to by <bot>.  The size of each object is specified by
* <size>.
*
* The contents of the array are sorted into ascending order according to a
* comparison function pointed to by <compar>, which is called with two
* arguments that point to the objects being compared.  The function shall
* return an integer less than, equal to, or greater than zero if the first
* argument is considered to be respectively less than, equal to, or greater
* than the second.
*
* If two elements compare as equal, their order in the sorted array is
* unspecified.
*
* INCLUDE FILES: stdlib.h
*
* RETURNS: N/A
*/
void qsort
    (
    void *	bot,		/* initial element in array */
    size_t	nmemb,		/* no. of objects in array */
    size_t	size,		/* size of array element */
    int	(*compar) (const void *, const void *)  /* comparison function */
    )
    {
    /* static void insertion_sort(), quick_sort(); */

    if (nmemb <= 1)
    	return;

    if (nmemb >= THRESH)
    	quick_sort (bot, nmemb, size, compar);
    else
    	insertion_sort (bot, nmemb, size, compar);
    }

/******************************************************************************
*
* quick_sort - sort an array of objects.
*
* RETURNS: no value.
* NOMANUAL
*/

static void quick_sort
    (
    FAST char *  bot,
    int          nmemb,
    FAST int     size,
    int          (*compar)()
    )
    {
    FAST int 	  cnt;
    FAST uint_t	  ch;
    FAST char *   top;
    FAST char *   mid;
    FAST char *   t1;
    FAST char *   t2;
    FAST int      n1;
    FAST int      n2;
    char *        bsv;

    /* bot and nmemb must already be set. */

partition:

    /* find mid and top elements */
    mid = bot + size * (nmemb >> 1);
    top = bot + (nmemb - 1) * size;

    /*
     * Find the median of the first, last and middle element (see Knuth,
     * Vol. 3, page 123, Eq. 28).  This test order gets the equalities
     * right.
     */
    if (nmemb >= MTHRESH) 
	{
    	n1 = compar (bot, mid);
    	n2 = compar (mid, top);

    	if ((n1 < 0) && (n2 > 0))
    		t1 = compar (bot, top) < 0 ? top : bot;
    	else 
	    if ((n1 > 0) && (n2 < 0))
    		t1 = compar (bot, top) > 0 ? top : bot;
    	else
    		t1 = mid;

        /* if mid element not selected, swap selection there */
        if (t1 != mid) 
            {
            SWAP (t1, mid);
            mid -= size;
            }
        }

    /* Standard quicksort, Knuth, Vol. 3, page 116, Algorithm Q. */
#define	didswap	n1
#define	newbot	t1
#define	replace	t2

    didswap = 0;
    bsv = bot;

    FOREVER
	{
    	while ((bot < mid) && (compar (bot, mid) <= 0))
	    {
	    bot += size;
	    }

    	while (top > mid) 
	    {
    	    if (compar (mid, top) <= 0) 
		{
    	    	top -= size;
    	    	continue;
    	        }
    	    newbot = bot + size;	/* value of bot after swap */

    	    if (bot == mid)		/* top <-> mid, mid == top */
    	    	replace = mid = top;
    	    else 
		{			/* bot <-> top */
    	    	replace = top;
    	    	top -= size;
    	        }
    	    goto swap;
    	    }

    	if (bot == mid)
    		break;

    	/* bot <-> mid, mid == bot */
    	replace = mid;
    	newbot = mid = bot;		/* value of bot after swap */
    	top -= size;

swap:	SWAP(bot, replace);

    	bot = newbot;
    	didswap = 1;
        }

    /*
     * Quicksort behaves badly in the presence of data which is already
     * sorted (see Knuth, Vol. 3, page 119) going from O N lg N to O N^2.
     * To avoid this worst case behavior, if a re-partitioning occurs
     * without swapping any elements, it is not further partitioned and
     * is insert sorted.  This wins big with almost sorted data sets and
     * only loses if the data set is very strangely partitioned.  A fix
     * for those data sets would be to return prematurely if the insertion
     * sort routine is forced to make an excessive number of swaps, and
     * continue the partitioning.
     */
    if (!didswap) 
	{
    	insertion_sort (bsv, nmemb, size, compar);
    	return;
        }

    /*
     * Re-partition or sort as necessary.  Note that the mid element
     * itself is correctly positioned and can be ignored.
     */
#define	nlower	n1
#define	nupper	n2

    bot = bsv;
    nlower = (mid - bot) / size;	/* size of lower partition */
    mid += size;
    nupper = nmemb - nlower - 1;	/* size of upper partition */

    /*
     * If must call recursively, do it on the smaller partition; this
     * bounds the stack to lg N entries.
     */
    if (nlower > nupper) 
	{
    	if (nupper >= THRESH)
    	    quick_sort (mid, nupper, size, compar);
    	else 
	    {
    	    SORT (mid, nupper);
    	    if (nlower < THRESH) 
		{
    	    	SORT (bot, nlower);
    	    	return;
    	        }
    	    }
    	nmemb = nlower;
        } 
    else 
	{
    	if (nlower >= THRESH)
    	    quick_sort (bot, nlower, size, compar);
    	else 
	    {
    	    SORT (bot, nlower);
    	    if (nupper < THRESH) 
		{
    		SORT (mid, nupper);
    		return;
    		}
    	    }
    	bot = mid;
    	nmemb = nupper;
        }
    goto partition;
    /* NOTREACHED */
    }

/******************************************************************************
*
* insertion_sort - internal routine
*
* RETURNS: no value.
* NOMANUAL
*/

static void insertion_sort
    (
    char *       bot,
    int          nmemb,
    FAST int     size,
    int          (*compar)()
    )
    {
    FAST int     cnt;
    FAST uint_t  ch;
    FAST char *  s1;
    FAST char *  s2;
    FAST char *  t1;
    FAST char *  t2;
    FAST char *  top;

    /*
     * A simple insertion sort (see Knuth, Vol. 3, page 81, Algorithm
     * S).  Insertion sort has the same worst case as most simple sorts
     * (O N^2).  It gets used here because it is (O N) in the case of
     * sorted data.
     */
    top = bot + nmemb * size;

    for (t1 = bot + size; t1 < top;) 
	{
    	for (t2 = t1; ((t2 -= size) >= bot) && (compar (t1, t2) < 0);)
	    ;
    	if (t1 != (t2 += size)) 
	    {
    	    /* Bubble bytes up through each element. */
    	    for (cnt = size; cnt--; ++t1) 
		{
    	    	ch = *t1;
    	    	for (s1 = s2 = t1; (s2 -= size) >= t2; s1 = s2)
		    {
    	    	    *s1 = *s2;
		    }
    	    	*s1 = ch;
    	        }
    	    } 
        else
    	    t1 += size;
        }
    }
/* rand.c - rand, srand functions for stdlib  */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,08feb93,jdi  documentation cleanup for 5.1.
01b,20sep92,smb  documentation additions.
01a,19jul92,smb  written and documented.
*/

/*
DESCRIPTION

INCLUDE FILES: stdlib.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "stdlib.h"

ulong_t _Randseed = 1;

/*******************************************************************************
*
* rand - generate a pseudo-random integer between 0 and RAND_MAX  (ANSI)
*
* This routine generates a pseudo-random integer between 0 and RAND_MAX.
* The seed value for rand() can be reset with srand().
*
* INCLUDE FILES: stdlib.h 
*
* RETURNS: A pseudo-random integer.
*
* SEE ALSO: srand()
*/

int rand (void) 
    {
    _Randseed = _Randseed * 1103515245 + 12345;
    return (uint_t) (_Randseed/65536) % (RAND_MAX + 1);
    }

/*******************************************************************************
*
* srand - reset the value of the seed used to generate random numbers (ANSI)
*
* This routine resets the seed value used by rand().  If srand() is then
* called with the same seed value, the sequence of pseudo-random numbers is
* repeated.  If rand() is called before any calls to srand() have been made,
* the same sequence shall be generated as when srand() is first called with
* the seed value of 1.
*
* INCLUDE FILES: stdlib.h 
*
* RETURNS: N/A
*
* SEE ALSO: rand()
*/

void * srand 
    (
    uint_t seed		/* random number seed */
    ) 
    {
    _Randseed = seed;
    return (void *)0;
    }
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
/* strtol.c - strtol file for stdlib  */

/* Copyright 1992-1995 Wind River Systems, Inc. */

/*
modification history
--------------------
01f,11feb95,jdi  doc fix.
01e,13oct93,caf  ansi fix: added cast when assigning from a const.
01d,08feb93,jdi  documentation cleanup for 5.1.
01c,21sep92,smb  tweaks for mg
01b,20sep92,smb  documentation additions.
01a,10jul92,smb  written and documented.
*/

/*
DESCRIPTION
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.

INCLUDE FILES: stdlib.h, limits.h, ctype.h, errno.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "stdlib.h"
#include "limits.h"
#include "ctype.h"
#include "errno.h"

/*****************************************************************************
*
* strtol - convert a string to a long integer (ANSI)
*
* This routine converts the initial portion of a string <nptr> to `long int'
* representation.  First, it decomposes the input string into three parts:
* an initial, possibly empty, sequence of white-space characters (as
* specified by isspace()); a subject sequence resembling an integer
* represented in some radix determined by the value of <base>; and a final
* string of one or more unrecognized characters, including the terminating
* NULL character of the input string.  Then, it attempts to convert the
* subject sequence to an integer number, and returns the result.
*
* If the value of <base> is zero, the expected form of the subject sequence
* is that of an integer constant, optionally preceded by a plus or minus
* sign, but not including an integer suffix.  If the value of <base> is
* between 2 and 36, the expected form of the subject sequence is a sequence
* of letters and digits representing an integer with the radix specified by
* <base> optionally preceded by a plus or minus sign, but not including an
* integer suffix.  The letters from a (or A) through to z (or Z) are
* ascribed the values 10 to 35; only letters whose ascribed values are less
* than <base> are premitted.  If the value of <base> is 16, the characters
* 0x or 0X may optionally precede the sequence of letters and digits,
* following the sign if present.
*
* The subject sequence is defined as the longest initial subsequence of the 
* input string, starting with the first non-white-space character, that is of 
* the expected form.  The subject sequence contains no characters if the 
* input string is empty or consists entirely of white space, or if the first
* non-white-space character is other than a sign or a permissible letter or
* digit.
*
* If the subject sequence has the expected form and the value of <base> is
* zero, the sequence of characters starting with the first digit is
* interpreted as an integer constant.  If the subject sequence has the
* expected form and the value of <base> is between 2 and 36, it is used as
* the <base> for conversion, ascribing to each latter its value as given
* above.  If the subject sequence begins with a minus sign, the value
* resulting from the conversion is negated.  A pointer to the final string
* is stored in the object pointed to by <endptr>, provided that <endptr> is
* not a NULL pointer.
*
* In other than the "C" locale, additional implementation-defined subject
* sequence forms may be accepted.  VxWorks supports only the "C" locale;
* it assumes that the upper- and lower-case alphabets and digits are
* each contiguous.
*
* If the subject sequence is empty or does not have the expected form, no
* conversion is performed; the value of <nptr> is stored in the object
* pointed to by <endptr>, provided that <endptr> is not a NULL pointer.
*
* INCLUDE FILES: stdlib.h 
*
* RETURNS:
* The converted value, if any.  If no conversion could be performed, it
* returns zero.  If the correct value is outside the range of representable
* values, it returns LONG_MAX or LONG_MIN (according to the sign of the
* value), and stores the value of the macro ERANGE in `errno'.
*/

long strtol
    (
    const char * nptr,		/* string to convert */
    char **      endptr,	/* ptr to final string */
    FAST int     base		/* radix */
    )
    {
    FAST const   char *s = nptr;
    FAST ulong_t acc;
    FAST int 	 c;
    FAST ulong_t cutoff;
    FAST int     neg = 0;
    FAST int     any;
    FAST int     cutlim;

    /*
     * Skip white space and pick up leading +/- sign if any.
     * If base is 0, allow 0x for hex and 0 for octal, else
     * assume decimal; if base is already 16, allow 0x.
     */
    do 
        {
    	c = *s++;
        } while (isspace (c));

    if (c == '-') 
        {
    	neg = 1;
    	c = *s++;
        } 
    else if (c == '+')
    	c = *s++;

    if (((base == 0) || (base == 16)) &&
        (c == '0') && 
        ((*s == 'x') || (*s == 'X'))) 
        {
    	c = s[1];
    	s += 2;
    	base = 16;
        }

    if (base == 0)
    	base = (c == '0' ? 8 : 10);

    /*
     * Compute the cutoff value between legal numbers and illegal
     * numbers.  That is the largest legal value, divided by the
     * base.  An input number that is greater than this value, if
     * followed by a legal input character, is too big.  One that
     * is equal to this value may be valid or not; the limit
     * between valid and invalid numbers is then based on the last
     * digit.  For instance, if the range for longs is
     * [-2147483648..2147483647] and the input base is 10,
     * cutoff will be set to 214748364 and cutlim to either
     * 7 (neg==0) or 8 (neg==1), meaning that if we have accumulated
     * a value > 214748364, or equal but the next digit is > 7 (or 8),
     * the number is too big, and we will return a range error.
     *
     * Set any if any `digits' consumed; make it negative to indicate
     * overflow.
     */

    cutoff = (neg ? -(ulong_t) LONG_MIN : LONG_MAX);
    cutlim = cutoff % (ulong_t) base;
    cutoff /= (ulong_t) base;

    for (acc = 0, any = 0;; c = *s++) 
        {
    	if (isdigit (c))
    	    c -= '0';
    	else if (isalpha (c))
    	    c -= (isupper(c) ? 'A' - 10 : 'a' - 10);
    	else
    	    break;

    	if (c >= base)
    	    break;

    	if ((any < 0) || (acc > cutoff) || (acc == cutoff) && (c > cutlim))
    	    any = -1;
    	else 
            {
    	    any = 1;
    	    acc *= base;
    	    acc += c;
    	    }
        }

    if (any < 0) 
        {
    	acc = (neg ? LONG_MIN : LONG_MAX);
    	errno = ERANGE;
        } 
    else if (neg)
    	acc = -acc;

    if (endptr != 0)
    	*endptr = (any ? (char *) (s - 1) : (char *) nptr);

    return (acc);
    }
/* strtoul.c - strtoul file for stdlib  */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01d,08feb93,jdi  documentation cleanup for 5.1.
01c,21sep92,smb  tweaks for mg
01b,20sep92,smb  documentation additions.
01a,10jul92,smb  documented.
*/

/*
DESCRIPTION
 * Copyright (c) 1990 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.

INCLUDE FILES: stdlib.h, limits.h, ctype.h, errno.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "limits.h"
#include "ctype.h"
#include "errno.h"
#include "stdlib.h"

/*****************************************************************************
*
* strtoul - convert a string to an unsigned long integer (ANSI)
*
* This routine converts the initial portion of a string <nptr> to
* `unsigned long int' representation.  First, it decomposes the input
* string into three parts:  an initial, possibly empty, sequence of
* white-space characters (as specified by isspace()); a subject sequence
* resembling an unsigned integer represented in some radix determined by
* the value <base>; and a final string of one or more unrecognized
* characters, including the terminating null character of the input string.
* Then, it attempts to convert the subject sequence to an unsigned integer,
* and returns the result.
*
* If the value of <base> is zero, the expected form of the subject sequence
* is that of an integer constant, optionally preceded by a plus or minus
* sign, but not including an integer suffix.  If the value of <base> is
* between 2 and 36, the expected form of the subject sequence is a sequence
* of letters and digits representing an integer with the radix specified by
* letters from a (or A) through z (or Z) which are ascribed the values 10 to
* 35; only letters whose ascribed values are less than <base> are
* premitted.  If the value of <base> is 16, the characters 0x or 0X may
* optionally precede the sequence of letters and digits, following the sign
* if present.
*
* The subject sequence is defined as the longest initial subsequence of the
* input string, starting with the first non-white-space character, that is
* of the expected form.  The subject sequence contains no characters if the
* input string is empty or consists entirely of white space, or if the first
* non-white-space character is other than a sign or a permissible letter or
* digit.
*
* If the subject sequence has the expected form and the value of <base> is
* zero, the sequence of characters starting with the first digit is
* interpreted as an integer constant.  If the subject sequence has the
* expected form and the value of <base> is between 2 and 36, it is used as
* the <base> for conversion, ascribing to each letter its value as given
* above.  If the subject sequence begins with a minus sign, the value
* resulting from the conversion is negated.  A pointer to the final string
* is stored in the object pointed to by <endptr>, provided that <endptr> is
* not a null pointer.
*
* In other than the "C" locale, additional implementation-defined subject
* sequence forms may be accepted.  VxWorks supports only the "C" locale;
* it assumes that the upper- and lower-case alphabets and digits are
* each contiguous.
*
* If the subject sequence is empty or does not have the expected form, no
* conversion is performed; the value of <nptr> is stored in the object
* pointed to by <endptr>, provided that <endptr> is not a null pointer.
*
* INCLUDE FILES: stdlib.h 
*
* RETURNS:
* The converted value, if any.  If no conversion could be performed it
* returns zero.  If the correct value is outside the range of representable
* values, it returns ULONG_MAX, and stores the value of the macro ERANGE in
* <errno>.
*/

ulong_t strtoul
    (
    const char *     nptr,	/* string to convert */
    char **          endptr,	/* ptr to final string */
    FAST int         base	/* radix */
    )
    {
    FAST const char *s = nptr;
    FAST ulong_t     acc;
    FAST int         c;
    FAST ulong_t     cutoff;
    FAST int         neg = 0;
    FAST int 	     any;
    FAST int 	     cutlim;

    /*
     * See strtol for comments as to the logic used.
     */
    do 
        {
    	c = *s++;
        } while (isspace (c));

    if (c == '-') 
        {
    	neg = 1;
    	c = *s++;
        } 
    else if (c == '+')
    	c = *s++;

    if (((base == 0) || (base == 16)) &&
        (c == '0') && 
        ((*s == 'x') || (*s == 'X'))) 
        {
    	c = s[1];
    	s += 2;
    	base = 16;
        }

    if (base == 0)
    	base = (c == '0' ? 8 : 10);

    cutoff = (ulong_t)ULONG_MAX / (ulong_t)base;
    cutlim = (ulong_t)ULONG_MAX % (ulong_t)base;

    for (acc = 0, any = 0;; c = *s++) 
        {
    	if (isdigit (c))
    	    c -= '0';
    	else if (isalpha (c))
    	    c -= (isupper (c) ? 'A' - 10 : 'a' - 10);
    	else
    	    break;

    	if (c >= base)
    	    break;

    	if ((any < 0) || (acc > cutoff) || (acc == cutoff) && (c > cutlim))
    	    any = -1;
    	else 
            {
    	    any = 1;
    	    acc *= base;
    	    acc += c;
    	    }
        }

    if (any < 0) 
        {
    	acc = ULONG_MAX;
    	errno = ERANGE;
        } 
    else if (neg)
    	acc = -acc;

    if (endptr != 0)
    	*endptr = (any ? CHAR_FROM_CONST (s - 1) : CHAR_FROM_CONST (nptr));

    return (acc);
    }
/* system.c - system file for stdlib  */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,08feb93,jdi  documentation cleanup for 5.1.
01b,20sep92,smb  documentation additions.
01a,25mar92,smb  written.
*/

/*
DESCRIPTION

INCLUDE FILES: stdlib.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "stdlib.h"

/*****************************************************************************
*
* system - pass a string to a command processor (Unimplemented) (ANSI)
*
* This function is not applicable to VxWorks.
*
* INCLUDE FILES: stdlib.h 
*
* RETURNS: OK, always.
*/
int system 
    (
    const char * string		/* pointer to string */
    ) 
    {
    return (OK);
    }
