/* taskArchLib.c - I80x86 architecture-specific task management routines */

/* Copyright 1984-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01c,20nov01,hdn  doc update for 5.5
01b,02jun93,hdn  updated to 5.1
		  - changed functions to ansi style
		  - changed VOID to void
		  - changed copyright notice
01a,28feb92,hdn  written based on TRON, 68k version.
*/

/*
DESCRIPTION
This library provides an interface to I80x86-specific task management 
routines.

SEE ALSO: taskLib
*/

/* LINTLIBRARY */

#include "vxWorks.h"
#include "taskArchLib.h"
#include "regs.h"
#include "private/taskLibP.h"
#include "private/windLibP.h"


/* globals */

REG_INDEX taskRegName[] =
    {
    {"edi", G_REG_OFFSET(0)},
    {"esi", G_REG_OFFSET(1)},
    {"ebp", G_REG_OFFSET(2)},
    {"esp", G_REG_OFFSET(3)},
    {"ebx", G_REG_OFFSET(4)},
    {"edx", G_REG_OFFSET(5)},
    {"ecx", G_REG_OFFSET(6)},
    {"eax", G_REG_OFFSET(7)},
    {"eflags", SR_OFFSET},
    {"pc", PC_OFFSET},
    {NULL, 0},
    };

UINT	taskATenable = 0x00000000;		/* AT bit in EFLAGS */


/*******************************************************************************
*
* taskRegsInit - initialize a task's registers
*
* During task initialization this routine is called to initialize the specified
* task's registers to the default values.
* 
* NOMANUAL
*/

void taskRegsInit
    (
    WIND_TCB	*pTcb,		/* pointer TCB to initialize */
    char	*pStackBase	/* bottom of task's stack */
    )
    {

    pTcb->regs.eflags = EFLAGS_BRANDNEW | taskATenable; /* set status reg */
    pTcb->regs.pc = (INSTR *)vxTaskEntry;	/* set entry point */

    pTcb->regs.edi = 0;				/* initialize 7 regs */
    pTcb->regs.esi = 0;
    pTcb->regs.ebp = 0;
    pTcb->regs.ebx = 0;
    pTcb->regs.edx = 0;
    pTcb->regs.ecx = 0;
    pTcb->regs.eax = 0;

    /* initial stack pointer is just after MAX_TASK_ARGS task arguments */

    pTcb->regs.spReg = (int) (pStackBase - (MAX_TASK_ARGS * sizeof (int)));
    }

/*******************************************************************************
*
* taskArgsSet - set a task's arguments
*
* During task initialization this routine is called to push the specified
* arguments onto the task's stack.
*
* NOMANUAL
*/

void taskArgsSet
    (
    WIND_TCB	*pTcb,		/* pointer TCB to initialize */
    char	*pStackBase,	/* bottom of task's stack */
    int		pArgs[]		/* array of startup arguments */
    )
    {
    FAST int ix;
    FAST int *sp;

    /* push args on the stack */

    sp = (int *) pStackBase;			/* start at bottom of stack */

    for (ix = MAX_TASK_ARGS - 1; ix >= 0; --ix)
	*--sp = pArgs[ix];			/* put arguments onto stack */
    }

/*******************************************************************************
*
* taskRtnValueSet - set a task's subroutine return value
*
* This routine sets register EAX, the return code, to the specified value.  It
* may only be called for tasks other than the executing task.
*
* NOMANUAL
*/

void taskRtnValueSet
    (
    WIND_TCB	*pTcb,		/* pointer TCB for return value */
    int		returnValue	/* return value to fill into WIND_TCB */
    )
    {
    pTcb->regs.eax = returnValue;
    }

/*******************************************************************************
*
* taskArgsGet - get a task's arguments
*
* This routine is utilized during task restart to recover the original task
* arguments.
*
* NOMANUAL
*/

void taskArgsGet
    (
    WIND_TCB *pTcb,		/* pointer TCB to initialize */
    char *pStackBase,		/* bottom of task's stack */
    int  pArgs[]		/* array of arguments to fill */
    )
    {
    FAST int ix;
    FAST int *sp;

    /* pop args from the stack */

    sp = (int *) pStackBase;			/* start at bottom of stack */

    for (ix = MAX_TASK_ARGS - 1; ix >= 0; --ix)
	pArgs[ix] = *--sp;			/* fill arguments from stack */
    }

/*******************************************************************************
*
* taskSRSet - set task status register
*
* This routine sets the status register of a task that is not running
* (i.e., the TCB must not be that of the calling task).  The debugging
* facilities use this routine to set the trace bit in the EFLAGS of a
* task being single-stepped.
*
* RETURNS: OK, or ERROR if the task ID is invalid.
*/

STATUS taskSRSet
    (
    int    tid,	 		/* task ID */
    UINT   eflags		/* new EFLAGS */
    )
    {
    FAST WIND_TCB *pTcb = taskTcb (tid);

    if (pTcb == NULL)		/* task non-existent */
	return (ERROR);

    pTcb->regs.eflags = eflags;

    return (OK);
    }
