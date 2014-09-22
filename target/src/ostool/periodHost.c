/* periodHost.c - target support of host side period command */

/* Copyright 1984-1998 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01c,04sep98,cdp  apply 01b for all ARM CPUs with ARM_THUMB==TRUE.
01b,11jul97,cdp  added (Thumb) ARM7TDMI_T support.
01a,02oct95,pad  created, copied from usrLib.c version 08j.
*/

/*
DESCRIPTION
This file consists of the support routine for the repeat command meant to be
executed from the WindSh shell.
*/

/* includes */

#include "vxWorks.h"
#include "taskLib.h"

/* externals */

IMPORT int sysClkRateGet (void);	/* Get the system clock rate */

/*******************************************************************************
*
* periodHost - call a function periodically
*
* This command repeatedly calls a specified function, with up to eight of its
* arguments, delaying the specified number of seconds between calls.
*
* Normally, this routine is called only by period{}, which spawns
* it as a task.
*
* RETURNS: N/A
*
* SEE ALSO: period{}
*/

void periodHost
    (
    int		secs,		/* no. of seconds to delay between calls */
    FUNCPTR	func,		/* function to call repeatedly */
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
#if ((CPU_FAMILY == ARM) && ARM_THUMB)
    func = (FUNCPTR)((UINT32)func | 1);
#endif

    FOREVER
	{
	(* func) (arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);

	taskDelay (secs * sysClkRateGet ());
	}
    }
