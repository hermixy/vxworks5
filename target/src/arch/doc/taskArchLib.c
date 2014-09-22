/* taskArchLib.c - architecture-specific task management routines */

/* Copyright 1984-2001 Wind River Systems, Inc. */

/*
modification history
--------------------
01j,05jun02,wsl  remove reference to SPARC and i960
01i,20nov01,hdn  updated x86 specific sections
01h,14nov01,hbh  Updated for simulators.
01g,03mar00,zl   merged SH support into T2
01f,12mar99,p_m  Fixed SPR 20355 by documenting taskSRInit() for MIPS.
01e,25nov95,jdi  removed 29k stuff.
01d,10feb95,rhp  Update for R4000, 29K, i386/i486.
            jdi  changed 80960 to i960.
01c,15feb93,jdi  made NOMANUAL: taskACWGet(), taskACWSet(), taskPCWGet(),
		 taskPCWSet(), taskSRInit(), taskTCWGet(), taskTCWSet().
01b,20jan93,jdi  documentation cleanup.
01a,23sep92,jdi  written, based on taskArchLib.c for
		 mc68k, sparc, i960, mips.
*/

/*
DESCRIPTION
This library provides architecture-specific task management routines that set
and examine architecture-dependent registers.  For information about
architecture-independent task management facilities, see the manual entry
for taskLib.

NOTE: There are no application-level routines in taskArchLib for
SimSolaris, SimNT or SH.

INCLUDE FILES: regs.h, taskArchLib.h

SEE ALSO: taskLib
*/


/*******************************************************************************
*
* taskSRSet - set the task status register (MC680x0, MIPS, x86)
*
* This routine sets the status register of a task that is not running
* (i.e., the TCB must not be that of the calling task).
* Debugging facilities use this routine to set the trace bit in the 
* status register of a task that is being single-stepped.
*
* .IP `x86':
* The second parameter represents EFLAGS register and the size is 32 bit.
*
* RETURNS: OK, or ERROR if the task ID is invalid.
*/

STATUS taskSRSet
    (
    int     tid,         /* task ID */
    UINT16  sr           /* new SR  */
    )

    {
    ...
    }

/******************************************************************************
*
* taskPCWGet - get the task processor control word (i960)
*
* This routine gets the task processor control word (PCW).
*
* RETURNS: The PCW of the specified task.
*
* NOMANUAL
*/

int taskPCWGet
    (
    INT32	tid		/* task ID */
    )

    {
    ...
    }

/*******************************************************************************
*
* taskPCWSet - set the task processor control word (i960)
*
* This routine sets the processor control word of a non-executing task
* (i.e., the TCB must not be that of the calling task).
*
* RETURNS: OK, or ERROR if the task ID is invalid.
*
* NOMANUAL
*/

STATUS taskPCWSet
    (
    INT32 	tid, 		/* task ID */
    UINT32 	pcw		/* new PCW */
    )

    {
    ...
    }

/******************************************************************************
*
* taskACWGet - get the task arithmetic control word (i960)
*
* This routine gets the task arithmetic control word (ACW).
*
* RETURNS: The ACW of a specified task.
*
* NOMANUAL
*/

int taskACWGet
    (
    INT32	tid	/* task ID */
    )

    {
    ...
    }

/*******************************************************************************
*
* taskACWSet - set the task arithmetic control word (i960)
*
* This routine sets the arithmetic control word of a non-executing task
* (i.e., the TCB must not be that of the calling task).
*
* RETURNS: OK, or ERROR if the task ID is invalid.
*
* NOMANUAL
*/

STATUS taskACWSet
    (
    INT32 	tid, 	/* task ID */
    UINT32 	acw	/* new ACW */
    )

    {
    ...
    }

/******************************************************************************
*
* taskTCWGet - get the task trace control word (i960)
*
* This routine gets the task trace control word (TCW).
*
* RETURNS: The TCW of a specified task.
*
* NOMANUAL
*/

int taskTCWGet
    (
    INT32 tid		/* task ID */
    )

    {
    ...
    }

/*******************************************************************************
*
* taskTCWSet - set the task trace control word (i960)
*
* This routine sets the trace control word (TCW) of a non-executing 
* task (i.e., the TCB must not be that of the calling 
* task).  It is used by the debugging facilities to set the trace mode 
* in the TCW of a task being debugged.
*
* RETURNS: OK, or ERROR if the task ID is invalid.
*
* NOMANUAL
*/

STATUS taskTCWSet
    (
    INT32 	tid, 		/* task ID */
    UINT32 	tcw		/* new TCW */
    )

    {
    ...
    }

/*******************************************************************************
*
* taskSRInit - initialize the default task status register (MIPS)
*
* This routine sets the default status register for system-wide tasks.
* All tasks will be spawned with the status register set to this value; thus, 
* it must be called before kernelInit().
*
* RETURNS: The previous value of the default status register.
*/

ULONG taskSRInit
    (
    ULONG  newSRValue 	/* new default task status register  */
    )

    {
    ...
    }

