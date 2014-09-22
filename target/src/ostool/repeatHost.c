/* repeatHost.c - target support of host side repeat command */

/* Copyright 1984-1998 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01c,04sep98,cdp  apply 01b for all ARM CPUs with ARM_THUMB==TRUE.
01b,11jul97,cdp  added Thumb (ARM7TDMI_T) support.
01a,02oct95,pad  created, copied from usrLib.c version 08j.
*/

/*
DESCRIPTION
This file consists of the support routine for the repeat command meant to be
executed from the WindSh shell.
*/

/* includes */

#include "vxWorks.h"

/*******************************************************************************
*
* repeatHost - call a function repeatedly
*
* This command calls a specified function <n> times, with up to eight of its
* arguments.  If <n> is 0, the routine is called endlessly.
*
* Normally, this routine is called only by repeat{}, which spawns it as a
* task.
*
* RETURNS: N/A
*
* SEE ALSO: repeat{}
*
* NOMANUAL
*/

void repeatHost
    (
    FAST int	 n,		/* no. of times to call func (0=forever) */
    FAST FUNCPTR func,		/* function to call repeatedly         */
    int		arg1,		/* first of eight args to pass to func */
    int		arg2,
    int		arg3,
    int		arg4,
    int		arg5,
    int		arg6,
    int		arg7,
    int		arg8
    )
    {
    FAST BOOL infinite = (n == 0);

#if ((CPU_FAMILY == ARM) && ARM_THUMB)
    func = (FUNCPTR)((UINT32)func | 1);
#endif

    while (infinite || (--n >= 0))
	(* func) (arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
    }
