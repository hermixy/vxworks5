/* longjmp.c - non local goto function */

/* Copyright 1984-1995 Wind River Systems, Inc. */

/*
modification history
--------------------
01f,27dec95,mem  restored use of longjmp for VxSim hppa.
01e,27oct95,jdi  doc: setjmp() no longer a macro (SPR 5300).
01d,27feb95,rhp  reconcile comments with ansiSetjmp.c documentation
01c,12may94,ms   removed longjmp for VxSIm hppa (use macro instead).
01b,20sep92,smb  documentation additions
01a,31aug92,rrr  written.
*/

/*
DESCRIPTION

INCLUDE FILE: setjmp.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "setjmp.h"
#include "private/sigLibP.h"
#include "taskLib.h"

#define JMP_DATA(env)	((int *) \
			 (((char *)env) + sizeof(jmp_buf) - 2*sizeof(int)))

/*******************************************************************************
*
* _setjmpSetup - portable part of setjmp
*
* RETURNS: Nothing.
*
* NOMANUAL
*/
void _setjmpSetup
    (
    jmp_buf env,
    int val
    )
    {
    int *p;

    p = JMP_DATA(env);

    p[0] = (int)taskIdCurrent;

    if (val)
	{
	p[0] |= 1;
	p[1] = (taskIdCurrent->pSignalInfo == NULL) ?
		0 : taskIdCurrent->pSignalInfo->sigt_blocked;
	}
    }

/*******************************************************************************
*
* longjmp - perform non-local goto	(ANSI)
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
*
* SEE ALSO: setjmp()
*/
void longjmp
    (
    jmp_buf env,
    int val
    )
    {
    int *p;

    p = JMP_DATA(env);

    if ((p[0] & ~1) != (int)taskIdCurrent)
	taskSuspend(0);

    if ((p[0] & 1) && (_func_sigprocmask != NULL))
	_func_sigprocmask(SIG_SETMASK, &p[1], 0);

    _sigCtxRtnValSet((REG_SET *)env, val == 0 ? 1 : val);
    _sigCtxLoad((REG_SET *)env);
    }
