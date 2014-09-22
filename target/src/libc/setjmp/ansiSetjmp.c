/* ansiSetjmp.c - ANSI 'setjmp' documentation */

/* Copyright 1984-1995 Wind River Systems, Inc. */

/*
modification history
--------------------
01e,27oct95,jdi  doc: setjmp() no longer a macro (SPR 5300).
01d,27feb95,rhp  updated and reconciled with longjmp.c comments
01c,16feb95,jdi  doc style change.
01b,18jan95,rhp  describe ISR restrictions (SPR#3277)
01a,24oct92,smb  written.
*/

/*
DESCRIPTION
The header setjmp.h defines functions and
one type for bypassing the normal function call and return discipline.

The type declared is:
.iP `jmp_buf' 12
an array type suitable for holding the information needed to restore
a calling environment.
.LP

The ANSI C standard does not specify whether setjmp() is a subroutine
or a macro.

SEE ALSO: American National Standard X3.159-1989
*/

/*******************************************************************************
*
* setjmp - save the calling environment in a `jmp_buf' argument (ANSI)
* 
* This routine saves the calling environment in <env>, in order to permit
* a longjmp() call to restore that environment (thus performing a
* non-local goto).
*
* .SS "Constraints on Calling Environment"
* The setjmp() routine may only be used in the following
* contexts:
* .iP
* as the entire controlling expression of a selection or iteration statement;
* .iP
* as one operand of a relational or equality operator, in the
* controlling expression of a selection or iteration statement;
* .iP
* as the operand of a single-argument `!' operator, in the controlling
* expression of a selection or iteration statement; or
* .iP
* as a complete C statement statement containing nothing other than
* the setjmp() call (though the result may be cast to `void').
* 
* RETURNS
* From a direct invocation, setjmp() returns zero.  From a call
* to longjmp(), it returns a non-zero value specified as an argument
* to longjmp().
* 
* SEE ALSO
* longjmp()
*/

int setjmp
    (
    jmp_buf env
    )

    {
    }

/*******************************************************************************
*
* longjmp - perform non-local goto by restoring saved environment (ANSI) 
* 
* This routine restores the environment saved by the most recent
* invocation of the setjmp() routine that used the same `jmp_buf'
* specified in the argument <env>.  The restored environment includes
* the program counter, thus transferring control to the setjmp()
* caller.
* 
* If there was no corresponding setjmp() call, or if the function
* containing the corresponding setjmp() routine call has already
* returned, the behavior of longjmp() is unpredictable.
*  
* All accessible objects in memory retain their values as of the time
* longjmp() was called, with one exception: local objects on the C
* stack that are not declared `volatile', and have been changed
* between the setjmp() invocation and the longjmp() call, have
* unpredictable values.
*  
* The longjmp() function executes correctly in contexts of signal
* handlers and any of their associated functions (but not from interrupt
* handlers).
* 
* WARNING: Do not use longjmp() or setjmp() from an ISR.
*
* RETURNS:
* This routine does not return to its caller.
* Instead, it causes setjmp() to return <val>, unless <val> is 0; in
* that case setjmp() returns 1.
* 
* SEE ALSO
* setjmp()
*/

void longjmp
    (
    jmp_buf env,
    int val
    )

    {
    }

