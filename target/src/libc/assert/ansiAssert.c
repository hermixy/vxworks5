/* ansiAssert.c - ANSI 'assert' documentation */

/* Copyright 1992 Wind River Systems, Inc. */

/*
modification history
--------------------
01a,24oct92,smb  written.
*/

/*
DESCRIPTION
The header assert.h defines the assert() macro and refers to another
macro, NDEBUG, which is not defined by assert.h.  If NDEBUG is defined
as a macro at the point in the source file where assert.h is included,
the assert() macro is defined simply as:
.CS
    #define assert(ignore) ((void)0)
.CE
ANSI specifies that assert() should be implemented as a macro, not as a
routine.  If the macro definition is suppressed in order to access an
actual routine, the behavior is undefined.

INCLUDE FILES: stdio.h, stdlib.h, assert.h

SEE ALSO: American National Standard X3.159-1989
*/

/******************************************************************************
*
* assert - put diagnostics into programs (ANSI)
* 
* If an expression is false (that is, equal to zero), the assert() macro
* writes information about the failed call to standard error in an
* implementation-defined format.  It then calls abort().
* The diagnostic information includes:
*     - the text of the argument
*     - the name of the source file (value of preprocessor macro __FILE__)
*     - the source line number (value of preprocessor macro __LINE__)
*
* INCLUDE: stdio.h, stdlib.h, assert.h
* 
* RETURNS: N/A
* 
*/  

void assert
    (
    int a
    )

    {
    /* This is a dummy for documentation purposes */
    }
