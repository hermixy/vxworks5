/* strcoll.c - string collate, string */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01d,26jun02,gls  fixed infinite loop in strcoll() (SPR #78357)
01c,25feb93,jdi  documentation cleanup for 5.1.
01b,20sep92,smb  documentation additions
01a,08jul92,smb  written and documented.
*/

/*
DESCRIPTION

INCLUDE FILES: string.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "private/strxfrmP.h"

/* The __sctl type describes a data object that holds the information 
 * needed to process each source string. The internal function getxfrm
 * calls __strxfrm to update an sctl data object.
 */
typedef struct 
    {
    char 	   buf[32];
    const uchar_t *s1;
    const uchar_t *s2;
    const uchar_t *sout;
    __cosave       state;
    } __sct1;


/***************************************************************************
*
* getxfrm - get transformed characters
*
* A conparison loop within strcoll calls getxfrm for each source string that
* has no mapped characters in its sctl buffer. This ensures that each source
* string is represented by at least one mapped character, if any such
* character remains to be generated.
*
* RETURNS: the size of the transformed string
* NOMANUAL
*/

LOCAL size_t getxfrm
    (
    __sct1 *p	/* information needed to process each source string */
    )
    {
    size_t sz;

    /* loop until chars delivered */
    do  
	{
        p->sout = (const uchar_t *) p->buf;

        sz = __strxfrm (p->buf, &p->s1, sizeof (p->buf), &p->state);

        if ((sz > 0) && (p->buf [sz - 1] == EOS))
    	    return (sz - 1);

	/* only reset the scan if the __strxfrm() call returned ERROR. */

        if ((*p->s1 == EOS) && (p->state.__state == _NSTATE))
	    {
	     p->s1 = p->s2;		/* rescan */
	    }

        } while (sz == 0);

    return (sz);
    }
	
/***************************************************************************
*
* strcoll - compare two strings as appropriate to LC_COLLATE  (ANSI)
*
* This routine compares two strings, both interpreted as appropriate to the
* LC_COLLATE category of the current locale.
*
* INCLUDE FILES: string.h
*
* RETURNS:
* An integer greater than, equal to, or less than zero, according to whether
* string <s1> is greater than, equal to, or less than string <s2> when both
* are interpreted as appropriate to the current locale.
*/

int strcoll
    (
    const char * s1,	/* string 1 */
    const char * s2	/* string 2 */
    )
    {
    size_t n1 = 0;		/* size of string 1 */
    size_t n2 = 0;		/* size of string 2 */
    __sct1 st1;			/* transform structure for string 1 */
    __sct1 st2;		 	/* transform structure for string 2 */
    static const __cosave initial = 
	{
	0
	};

    /* compare s1[], s2[] using locale-dependant rules */

    st1.s1	= (const uchar_t *)s1;	/* string transformation 1 */
    st1.s2	= (const uchar_t *)s1;
    st1.state	= initial;

    st2.s1	= (const uchar_t *)s2;	/* string transformation 2 */
    st2.s2	= (const uchar_t *)s2;
    st2.state	= initial;

    FOREVER				/* compare transformed characters */
    	{
    	int ans;
    	size_t sz;

    	if (n1 == 0)
    	    n1 = getxfrm (&st1);	/* string 1 */

    	if (n2 == 0)
    	    n2 = getxfrm (&st2);	/* string 2 */

    	sz = (n1 < n2) ? n1 : n2;

    	if (sz == 0)
	    {
    	    if (n1 == n2) 
		return (0); 

	    if (n2 > 0) 
		return (-1);

	    return (1);
            }

    	if ((ans = memcmp (st1.sout, st2.sout, sz)) != 0)
    	    return (ans);

    	st1.sout += sz;
    	st2.sout += sz;
    	n1 	 -= sz;
    	n2 	 -= sz;
    	}
    }
