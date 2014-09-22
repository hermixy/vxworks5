/* ansiStdarg.c - ANSI `stdarg' documentation */

/* Copyright 1984-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01b,16mar93,jdi  documentation cleanup for 5.1.
01a,24oct92,smb  written.
*/

/*
DESCRIPTION
The header stdarg.h declares a type and defines three macros for advancing
through a list of arguments whose number and types are not known to the
called function when it is translated.

A function may be called with a variable number of arguments of varying types.
The rightmost parameter plays a special role in the access mechanism, and
is designated <parmN> in this description.

The type declared is:
.iP `va_list' 12
a type suitable for holding information needed by the macros
va_start(), va_arg(), and va_end().
.LP

To access the varying arguments, the called function shall declare an object
having type `va_list'.  The object (referred to here as <ap>) may be
passed as an argument to another function; if that function invokes the
va_arg() macro with parameter <ap>, the value of <ap> in the calling
function is indeterminate and is passed to the va_end() macro prior
to any further reference to <ap>.

va_start() and va_arg() have been implemented as macros, not as
functions.  The va_start() and va_end() macros should be invoked in the
function accepting a varying number of arguments, if access to the varying
arguments is desired.

The use of these macros is documented here as if they were architecture-generic.
However, depending on the compilation environment, different macro versions are
included by vxWorks.h.

SEE ALSO: American National Standard X3.159-1989
*/   

/*******************************************************************************
*
* va_start - initialize a `va_list' object for use by va_arg() and va_end()
* 
* This macro initializes an object of type `va_list' (<ap>) for subsequent
* use by va_arg() and va_end().  The parameter <parmN> is the identifier of
* the rightmost parameter in the variable parameter list in the function
* definition (the one just before the , ...).  If <parmN> is declared with
* the register storage class with a function or array type, or with a type
* that is not compatible with the type that results after application of the
* default argument promotions, the behavior is undefined.
* 
* RETURNS: N/A
*/

void va_start
    (
    ap,		/* list of type va_list */
    parmN	/* rightmost parameter */
    )

    {
    /* dummy function for documenation purposes */
    }

/*******************************************************************************
*
* va_arg - expand to an expression having the type and value of the call's next argument
* 
* Each invocation of this macro modifies an object of type `va_list' (<ap>)
* so that the values of successive arguments are returned in turn.  The
* parameter <type> is a type name specified such that the type of a pointer
* to an object that has the specified type can be obtained simply by
* postfixing a * to <type>.  If there is no actual next argument, or if
* <type> is not compatible with the type of the actual next argument (as
* promoted according to the default argument promotions), the behavior is
* undefined.
* 
* RETURNS:
* The first invocation of va_arg() after va_start() returns the value of the
* argument after that specified by <parmN> (the rightmost parameter).
* Successive invocations return the value of the remaining arguments in
* succession.
*/

void va_arg
    (
    ap,		/* list of type va_list */
    type	/* type */
    )

    {
    /* dummy function for documenation purposes */
    }

/*******************************************************************************
*
* va_end - facilitate a normal return from a routine using a `va_list' object
*
* This macro facilitates a normal return from the function whose variable
* argument list was referred to by the expansion of va_start() that
* initialized the `va_list' object.
* 
* va_end() may modify the `va_list' object so that it is no longer
* usable (without an intervening invocation of va_start()).  If there is no
* corresponding invocation of the va_start() macro, or if the va_end() macro
* is not invoked before the return, the behavior is undefined.
* 
* RETURNS: N/A
*/

void va_end
    (
    ap		/* list of type va_list */
    )

    {
    /* dummy function for documenation purposes */
    }

