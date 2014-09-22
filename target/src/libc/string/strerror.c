/* strerror.c - string error, string */

/* Copyright 1992-2001 Wind River Systems, Inc. */

/*
modification history
--------------------
01e,16oct01,jn   use symFindSymbol for symbol lookup (SPR #7453)
01d,25feb93,jdi  documentation cleanup for 5.1.
01c,30nov92,jdi  fixed doc for strerror() - SPR 1825.
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
#include "string.h"
#include "errno.h"
#include "symLib.h"
#include "limits.h"
#include "stdio.h"
#include "sysSymTbl.h"
#include "private/funcBindP.h"

/* forward declarations */

LOCAL STATUS strerrorIf (int errcode, char *buf);


/*******************************************************************************
*
* strerror_r - map an error number to an error string 
*
* This routine maps the error number in <errcode> to an error message string.
* It stores the error string in <buffer>.  The size of <buffer> should be 
* NAME_MAX + 1 - using a smaller buffer may lead to stack corruption.  NAME_MAX 
* is defined in limits.h.
*
* This routine is the reentrant version of strerror().
*
* INCLUDE FILES: string.h
*
* RETURNS: OK or ERROR.
*
* SEE ALSO: strerror()
*/

STATUS strerror_r 
    (
    int    errcode,	/* error code */
    char * buffer	/* string buffer */
    )
    {
    return (strerrorIf (errcode, buffer));
    }

/*******************************************************************************
*
* strerror - map an error number to an error string (ANSI)
*
* This routine maps the error number in <errcode> to an error message string.
* It returns a pointer to a static buffer that holds the error string.
*
* INCLUDE: string.h
*
* RETURNS: A pointer to the buffer that holds the error string.
*
* SEE ALSO: strerror_r()
*/

char * strerror
    (
    int errcode		/* error code */
    )
    {
    static char buffer [NAME_MAX+1]; /* NAME_MAX doesn't count the EOS. */

    (void) strerror_r (errcode, buffer);

    return (buffer);
    }

/*******************************************************************************
*
* strerrorIf - interface from libc to VxWorks for strerror_r
*
* RETURNS: OK, or ERROR if <buf> is null.
* NOMANUAL
*/

LOCAL STATUS strerrorIf 
    (
    int   errcode,		/* error code */
    char *buf			/* string buffer */
    )
    {
    void *	value;
    char *	statName;
    SYMBOL_ID   symId;

    if (buf == NULL)
	return (ERROR);

    if (errcode == 0)
	{
        strcpy (buf, "OK");
	return (OK);
	}

    /* 
     * Only check one symLib function pointer (for performance's sake). 
     * All symLib functions are provided by the same library, by convention.    
     */

    if ((_func_symFindSymbol != (FUNCPTR) NULL) && 
	(statSymTbl != NULL))
	{

	(* _func_symFindSymbol) (statSymTbl, NULL, (void *)errcode, 
				 SYM_MASK_NONE, SYM_MASK_NONE, &symId);
	(* _func_symNameGet) (symId, &statName);
	(* _func_symValueGet) (symId, &value);

	if (value == (void *)errcode)
	    {
	    strncpy (buf, statName, NAME_MAX+1);
	    return (OK);
	    }
	}

    sprintf (buf, "errno = %#x", errcode);

    return (OK);
    }
