/* ttHostLib.c - Host based stack trace library */

/* Copyright 1995 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01b,13may95,p_m  store the parameters even when their number is unknown.
01a,02may95,p_m  written.
*/

/*
This module provides the Target side of the Tornado shell stack tracing. 
It should be use in conjonction with the TCL based tt procedure.  It relies
on the VxWorks trcStack routine to provide the stack trace information
and store them at a well known location (ttCallStack) where the host
will be able to read them.
*/

#include "vxWorks.h"
#include "taskLib.h"
#include "string.h"
#include "trcLib.h"
#include "regs.h"

/* defines */

#define TT_STACK_DEPTH_MAX	40
#define TT_DEFAULT_ARGS		6

/* globals */

UINT32	ttCallStack [TT_STACK_DEPTH_MAX][13];	/* call stack storage */

/* 
 * Current trace level needed by ttStoreCall. This make this code 
 * non-reentrant.
 */

UINT32  ttCallLevel;			

/* forward declaration */

static void ttStoreCall (INSTR * callAdrs, int funcAdrs, int nargs, 
    			 UINT32 * args);

/*******************************************************************************
*
* ttHost - print a stack trace of a task
*
* This routine prints a list of the nested routine calls that the specified
* task is in.  Each routine call and its parameters are shown.
*
* If <task> is not specified or zero, the last task referenced is
* assumed.  The tt() routine can only trace the stack of a task other than
* itself.  For instance, when tt() is called from the shell, it cannot trace
* the shell's stack.
*
* EXAMPLE
* .CS
*     -> tt "logTask"
*      3ab92 _vxTaskEntry   +10 : _logTask (0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
*       ee6e _logTask       +12 : _read (5, 3f8a10, 20)
*       d460 _read          +10 : _iosRead (5, 3f8a10, 20)
*       e234 _iosRead       +9c : _pipeRead (3fce1c, 3f8a10, 20)
*      23978 _pipeRead      +24 : _semTake (3f8b78)
*     value = 0 = 0x0
* .CE
* This indicates that logTask() is currently in semTake() (with
* one parameter) and was called by pipeRead() (with three parameters),
* which was called by iosRead() (with three parameters), and so on.
*
* INTERNAL
* This higher-level symbolic stack trace is built on top of the
* lower-level routines provided by trcLib.
*
* CAVEAT
* In order to do the trace, some assumptions are made.  In general, the
* trace will work for all C language routines and for assembly language
* routines that start with a LINK instruction.  Some C compilers require
* specific flags to generate the LINK first.  Most VxWorks assembly language
* routines include LINK instructions for this reason.  The trace facility
* may produce inaccurate results or fail completely if the routine is
* written in a language other than C, the routine's entry point is
* non-standard, or the task's stack is corrupted.  Also, all parameters are
* assumed to be 32-bit quantities, so structures passed as parameters will
* be displayed as \f2long\fP integers.
*
* RETURNS:
* OK, or ERROR if the task does not exist.
*
* SEE ALSO:
* .pG "Debugging"
*/

STATUS ttHost 
    (
    int task            /* task whose stack is to be traced */
    )
    {
    REG_SET	regSet;
    BOOL	resumeIt = FALSE;		/* flag to remember if */
    int 	tid;
						/* resuming is necessary */
    /* clear trace table */

    memset ((void *) ttCallStack, 0 , sizeof (ttCallStack));

    tid = taskIdDefault (task);			/* set default task id */

    /* get caller task's id and make sure it is not the task to be traced */

    if (tid == taskIdSelf () || tid == 0)
	{
	return (ERROR);
	}

    /* make sure the task exists */

    if (taskIdVerify (tid) != OK)
	{
	return (ERROR);
	}

    /* if the task is not already suspended, suspend it while we trace it */

    if (!taskIsSuspended (tid))
	{
	resumeIt = TRUE;		/* we want to resume it later */
	taskSuspend (tid);		/* suspend the task if need be */
	}

    /* trace the stack */

    ttCallLevel  = 0;

    taskRegsGet (tid, &regSet);
    trcStack (&regSet, (FUNCPTR) ttStoreCall, tid);

    if (resumeIt)
	taskResume (tid);		/* resume task if we suspended it */

    return (OK);
    }

/*******************************************************************************
*
* ttStoreCall - print a stack frame
*
* This routine is called by trcStack to store each stack level entry 
* information in the table ttCallStack[] where the host tool will be able
* to get them.
*/

void ttStoreCall 
    (
    INSTR *	callAdrs,       /* address from which function was called */
    int 	funcAdrs,       /* address of function called */
    int 	nargs,          /* number of arguments in function call */
    UINT32 *	args		/* pointer to function args */
    )
    {
    int ix;

    /* store call address and name of calling function plus offset */

    ttCallStack [ttCallLevel][0] = (UINT32) callAdrs;	
    ttCallStack [ttCallLevel][1] = (UINT32) funcAdrs;	
    ttCallStack [ttCallLevel][2] = (UINT32) nargs;	

    /* set number of arguments to default if unknown */

    if (nargs == 0)
	nargs = TT_DEFAULT_ARGS;

    /* store args */

    for (ix = 0; ix < nargs; ix++)
	{
	ttCallStack [ttCallLevel][ix+3] = args [ix];
	}

    /* next call will be for next call level */

    ttCallLevel++;
    }
