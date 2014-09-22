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
