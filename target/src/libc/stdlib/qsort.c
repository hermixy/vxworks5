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
