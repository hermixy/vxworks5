/* sigCtxLib.c - software signal architecture support library */

/* Copyright 1984-1993 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01a,15jun93,hdn  written based on mc68k version.
*/

/*
This library provides the architecture specific support needed by
software signals.
*/

#include "vxWorks.h"
#include "private/sigLibP.h"
#include "string.h"

struct sigfaulttable _sigfaulttable [] =
    {
    {0,  SIGILL},
    {1,  SIGEMT},
    {2,  SIGILL},
    {3,  SIGEMT},
    {4,  SIGILL},
    {5,  SIGILL},
    {6,  SIGILL},
    {7,  SIGFPE},
    {8,  SIGILL},
    {9,  SIGFPE},
    {10, SIGILL},
    {11, SIGBUS},
    {12, SIGBUS},
    {13, SIGILL},
    {14, SIGBUS},
    {15, SIGILL},
    {16, SIGFPE},
    {17, SIGBUS},
    {0,  0},
    };


/*******************************************************************************
*
* _sigCtxRtnValSet - set the return value of a context
*
* Set the return value of a context.
* This routine should be almost the same as taskRtnValueSet in taskArchLib.c
*/

void _sigCtxRtnValSet
    (
    REG_SET		*pRegs,
    int			val
    )
    {

    pRegs->eax = val;
    }


/*******************************************************************************
*
* _sigCtxStackEnd - get the end of the stack for a context
*
* Get the end of the stack for a context, the context will not be running.
* If during a context switch, stuff is pushed onto the stack, room must
* be left for that (on the 386 the pc, cs, and eflags are pushed just before
* a ctx switch)
*/

void *_sigCtxStackEnd
    (
    const REG_SET	*pRegs
    )
    {

    /*
     * The 12 is pad for the pc, cs, and eflags which are pushed onto the stack.
     */
    return (void *)(pRegs->esp - 12);
    }

/*******************************************************************************
*
* _sigCtxSetup - Setup of a context
*
* This routine will set up a context that can be context switched in.
* <pStackBase> points beyond the end of the stack. The first element of
* <pArgs> is the number of args to call <taskEntry> with.
* When the task gets swapped in, it should start as if called like
*
* (*taskEntry) (pArgs[1], pArgs[2], ..., pArgs[pArgs[0]])
*
* This routine is a blend of taskRegsInit and taskArgsSet.
*
* Currently (for signals) pArgs[0] always equals 1, thus the task should
* start as if called like
* (*taskEntry) (pArgs[1]);
*
* Furthermore (for signals), the function taskEntry will never return.
* For the 386 case, we push vxTaskEntry() onto the stack so a stacktrace
* looks good.
*/

void _sigCtxSetup
    (
    REG_SET		*pRegs,
    void		*pStackBase,
    void		(*taskEntry)(),
    int			*pArgs
    )
    {
    extern void vxTaskEntry();
    int i;
    union
	{
	void		*pv;
	int		*pi;
	void		(**ppfv)();
	int		i;
	} pu;

    bzero((void *)pRegs, sizeof(*pRegs));
    pu.pv = (void *)((int)pStackBase & ~3);

    for (i = pArgs[0]; i > 0; --i)
        *--pu.pi = pArgs[i];

    *--pu.ppfv = vxTaskEntry;
    pRegs->esp = pu.i;
    pRegs->pc = (INSTR *)taskEntry;
    pRegs->eflags = EFLAGS_BRANDNEW;
    }
