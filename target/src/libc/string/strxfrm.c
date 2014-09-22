/* strxfrm.c - file for string */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01d,25feb93,jdi  documentation cleanup for 5.1.
01c,20sep92,smb  documentation additions
01b,13jul92,smb  changed __cosave initialisation for MIPS.
01a,08jul92,smb  written and documented.
*/

/*
DESCRIPTION

INCLUDE FILES: string.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "limits.h"
#include "private/strxfrmP.h"


/*******************************************************************************
*
* strxfrm - transform up to <n> characters of <s2> into <s1> (ANSI)
*
* This routine transforms string <s2> and places the resulting string in <s1>.
* The transformation is such that if strcmp() is applied to two transformed
* strings, it returns a value greater than, equal to, or less than zero,
* corresponding to the result of the strcoll() function applied to the same
* two original strings.  No more than <n> characters are placed into the
* resulting <s1>, including the terminating null character.  If <n> is zero,
* <s1> is permitted to be a NULL pointer.  If copying takes place between
* objects that overlap, the behavior is undefined.
*
* INCLUDE FILES: string.h
*
* RETURNS:
* The length of the transformed string, not including the terminating null
* character.  If the value is <n> or more, the contents of <s1> are
* indeterminate.
*
* SEE ALSO: strcmp(), strcoll()
*/

size_t strxfrm
    (
    char *	 s1,		/* string out */
    const char * s2,		/* string in */
    size_t  	 n		/* size of buffer */
    )
    {
    size_t	   i;
    size_t	   nx = 0;
    const uchar_t *s = (const uchar_t *)s2;
    char	   buf[32];
    __cosave 	   state;

    /* stores state information */
    state.__state = EOS;
    state.__wchar = 0;

    while (nx < n)				/* translate and deliver */
    	{
    	i = __strxfrm (s1, &s, nx - n, &state);
    	s1 += i; 
	nx += i;

    	if ((i > 0) && (s1[-1] == EOS))
	    return (nx - 1);

    	if (*s == EOS)
	    s = (const uchar_t *) s2;
    	}

    FOREVER					/* translate the rest */ 
    	{
    	i = __strxfrm (buf, &s, sizeof (buf), &state);

    	nx += i;

    	if ((i > 0) && (buf [i - 1] == EOS))
	    return (nx - 1);

    	if (*s == EOS)
	    s = (const uchar_t *) s2;
    	}
    }

/*******************************************************************************
*
*  __strxfrm - translates string into an easier form for strxfrm() and strcoll()
*
* This routine performs the mapping as a finite state machine executing
* the table __wcstate defined in xstate.h.
*
* NOMANUAL
*/

size_t __strxfrm
    (
    char *	  	sout,		/* out string */
    const uchar_t **	ppsin,		/* pointer to character within string */
    size_t 		size,		/* size of string */
    __cosave *		ps		/* state information */
    )
    {
    const ushort_t *	stab;
    ushort_t	 	code;
    char 		state = ps->__state;	/* initial state */
    BOOL 		leave = FALSE;
    int 		limit = 0;
    int 		nout = 0;
    const uchar_t *	sin = *ppsin;		/* in string */
    ushort_t		wc = ps->__wchar;

    FOREVER					/* do state transformation */
    	{	
    	if ((_NSTATE <= state) ||
    	    ((stab = __costate.__table [state]) == NULL) ||
    	    ((code = stab [*sin]) == 0))
    	    break;		/* error */

    	state = (code & ST_STATE) >> ST_STOFF;

    	if ( code & ST_FOLD)
    		wc = wc & ~UCHAR_MAX | code & ST_CH;

    	if ( code & ST_ROTATE)
    		wc = wc >> CHAR_BIT & UCHAR_MAX | wc << CHAR_BIT;

    	if ((code & ST_OUTPUT) &&
    	    (((sout[nout++] = code & ST_CH ? code : wc) == EOS) ||
    	    (size <= nout)))
    		leave = TRUE;

    	if (code & ST_INPUT)
    	    if (*sin != EOS)
	        {
    	        ++sin; 
	        limit = 0;
	        }
    	    else
    	    	leave = TRUE;

    	if (leave)
    	    {		/* save state and return */
    	    *ppsin = sin;
    	    ps->__state = state;
    	    ps->__wchar = wc;
    	    return (nout);
    	    }
    	}

    sout[nout++] = EOS;		/* error */
    *ppsin = sin;
    ps->__state = _NSTATE;
    return (nout);
    }
