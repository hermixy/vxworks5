/* setlocale.c - ANSI locale */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01f,30aug93,jmm  fixed null pointer dereference in setlocale() (spr 2490)
01e,07feb93,jdi  documentation cleanup for 5.1.
01d,28sep92,smb  added ANSI to function description
01c,20sep92,smb  documentation additions
01b,14sep92,smb  added some minor error checking.
01a,08jul92,smb  written
*/

/*
DESCRIPTION

INCLUDE FILE: locale.h string.h stdlib.h 

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "locale.h"
#include "string.h"
#include "stdlib.h"
#include "ctype.h"
#include "private/localeP.h"

LOCAL char *currentName = "C";		/* current locale name */
__linfo	    __clocale = 
    {
    "C"
    };


/*******************************************************************************
*
* setlocale - set the appropriate locale (ANSI)
*
* This function selects the appropriate portion of the program's locale as
* specified by the <category> and <localeName> arguments.  This routine can
* be used to change or query the program's entire current locale or portions
* thereof.
*
* Values for <category> affect the locale as follows:
* .iP LC_ALL
* specifies the program's entire locale.
* .iP LC_COLLATE
* affects the behavior of the strcoll() and strxfrm() functions.
* .iP LC_CTYPE
* affects the behavior of the character-handling functions and the multi-byte
* functions.
* .iP LC_MONETARY
* affects the monetary-formatting information returned by localeconv().
* .iP LC_NUMERIC
* affects the decimal-point character for the formatted input/output
* functions and the string-conversion functions, as well as the
* nonmonetary-formatting information returned by localeconv().
* .iP LC_TIME
* affects the behavior of the strftime() function.
* .LP
*
* A value of "C" for <localeName> specifies the minimal environment for C
* translation; a value of "" specifies the implementation-defined native
* environment.  Other implementation-defined strings may be passed as the
* second argument.
*
* At program start-up, the equivalent of the following is executed:
* .CS
*     setlocale (LC_ALL, "C");
* .CE
*
* The implementation behaves as if no library function calls setlocale().
*
* If <localeName> is a pointer to a string and the selection can be
* honored, setlocale() returns a pointer to the string associated with the
* specified category for the new locale.  If the selection cannot be
* honored, it returns a null pointer and the program's locale is unchanged.
*
* If <localeName> is null pointer, setlocale() returns a pointer to the
* string associated with the category for the program's current locale; the
* program's locale is unchanged.
*
* The string pointer returned by setlocale() is such that a subsequent call
* with that string value and its associated category will restore that part
* of the program's locale.  The string is not modified by the program, but
* may be overwritten by a subsequent call to setlocale().
*
* Currently, only the "C" locale is implemented in this library.
*
* INCLUDE FILES: locale.h, string.h, stdlib.h 
*
* RETURNS: A pointer to the string "C".
*/

char *setlocale 
    (
    int		category,	/* category to change */
    const char *localeName	/* locale name */
    )
    {
    if (localeName != NULL &&
	(strcmp (localeName, currentName) != 0) && 
	(strcmp (localeName, "") != 0))	
        return (NULL);

    return (CHAR_FROM_CONST(currentName));
    }

