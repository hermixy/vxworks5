/* fppShow.c - floating-point show routines */

/* Copyright 1984-1998 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01j,21jun02,h_k  changed showing FPU reg val to float from double for SH4 (SPR
                 #78976).
01i,24apr01,mem  Fix reg swapping for MIPS32/MIPS64.
01h,03mar00,zl   merged SH support into T2
01h,20dec00,agf  Adapt to MIPS32/MIPS64 CPU architectures
01g,17mar99,jdi  doc: updated w/ info about proj facility (SPR 25727).
01f,10dec96,tam  added code to word swap FP reg. and to display FP reg. in 
		 single precision for MIPS (spr #7628).
01e,03feb93,jdi  changed INCLUDE_SHOW_RTNS to ...ROUTINES.
01d,21jan93,jdi  documentation cleanup for 5.1.
01c,30jul92,smb  changed format for printf to avoid zero padding.
01b,12jul92,jcf  tuned floating point register show format.
01a,04jul92,jcf  written/extracted from v3k fppLib.
*/

/*
DESCRIPTION
This library provides the routines necessary to show a task's optional 
floating-point context.  To use this facility, it must first be
installed using fppShowInit(), which is called automatically
when the floating-point show facility is configured into VxWorks
using either of the following methods:
.iP
If you use the configuration header files, define
INCLUDE_SHOW_ROUTINES in config.h.
.iP
If you use the Tornado project facility, select INCLUDE_HW_FP_SHOW.
.LP

This library enhances task information routines, such as ti(), to display
the floating-point context.

INCLUDE FILES: fppLib.h 

SEE ALSO: fppLib
*/

#include "vxWorks.h"
#include "stdio.h"
#include "regs.h"
#include "fppLib.h"
#include "private/funcBindP.h"

/* global variables */

char *fppTaskRegsCFmt = "%-6.6s = %8x";
char *fppTaskRegsDFmt = "%-6.6s = %8g";


/******************************************************************************
*
* fppShowInit - initialize the floating-point show facility
*
* This routine links the floating-point show facility into the VxWorks system.
* It is called automatically when the floating-point show facility is
* configured into VxWorks using either of the following methods:
* .iP
* If you use the configuration header files, define
* INCLUDE_SHOW_ROUTINES in config.h.
* .iP
* If you use the Tornado project facility, select INCLUDE_HW_FP_SHOW.
*
* RETURNS: N/A
*/

void fppShowInit (void)
    {
    /* avoid direct coupling with fppShow with this global variable */

    _func_fppTaskRegsShow = (FUNCPTR) fppTaskRegsShow;
    }

/*******************************************************************************
*
* fppTaskRegsShow - print the contents of a task's floating-point registers
*
* This routine prints to standard output the contents of a task's
* floating-point registers.
*
* RETURNS: N/A
*/

void fppTaskRegsShow
    (
    int task		/* task to display floating point registers for */
    )
    {
    int		ix;
#if (CPU_FAMILY == SH)
    float *	fpTmp;
#else
    double *	fpTmp;
#endif
    int *	fpCtlTmp;
    FPREG_SET	fpRegSet;
#if	(CPU == MIPS32)
    double	doubleTmp;
    double *	fpDoubleTmp = &doubleTmp;
#endif	/* CPU == MIPS32 */

    if ((fppProbe() != OK) || (fppTaskRegsGet (task, &fpRegSet) == ERROR))
	return;

    /* Some architectures organize floats sequentially, but to
     * display them as doubles the floats need to be word swapped.
     */

    if (fppDisplayHookRtn != NULL)
        (* fppDisplayHookRtn) (&fpRegSet);

    /* print floating point control registers */

    for (ix = 0; fpCtlRegName[ix].regName != (char *) NULL; ix++)
	{
	if ((ix % 4) == 0)
	    printf ("\n");
	else
	    printf ("%3s","");

	fpCtlTmp = (int *) ((int)&fpRegSet + fpCtlRegName[ix].regOff);
	printf (fppTaskRegsCFmt, fpCtlRegName[ix].regName, *fpCtlTmp);
	}

    /* print floating point data registers */

    for (ix = 0; fpRegName[ix].regName != (char *) NULL; ix++)
	{
	if ((ix % 4) == 0)
	    printf ("\n");
	else
	    printf ("%3s","");
	
#if	(CPU == MIPS32)
	/* 
	 * word swap the 32 bits FP Reg. pair before printing double
	 * value - R4000 and R3000.
	 */

	*(UINT32 *) fpDoubleTmp = *(UINT32 *) (((UINT32)&fpRegSet + 
						fpRegName[ix].regOff + 
						sizeof(UINT32)));
	*(UINT32 *) ((UINT32) fpDoubleTmp + sizeof(UINT32)) = 
		*(UINT32 *) (((UINT32)&fpRegSet + fpRegName[ix].regOff));

	fpTmp = fpDoubleTmp;
#elif	(CPU_FAMILY == SH)
	fpTmp = (float *) ((int)&fpRegSet + fpRegName[ix].regOff);
#else	/* CPU */
	fpTmp = (double *) ((int)&fpRegSet + fpRegName[ix].regOff);
#endif	/* CPU */
	printf (fppTaskRegsDFmt, fpRegName[ix].regName, *fpTmp);
	}

    printf ("\n");
    }
