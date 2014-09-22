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
