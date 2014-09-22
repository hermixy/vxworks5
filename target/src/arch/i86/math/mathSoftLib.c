/* mathSoftLib.c - high-level floating-point emulation library */

/* Copyright 1984-1995 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01a,07apr95,hdn	 written.
*/

/*
DESCRIPTION
This library initializes software emulation library which is generally
for use in systems that lack a floating-point coprocessor.

SEE ALSO: fppArchLib

*/


#include "vxWorks.h"


/* externals */

IMPORT void emu387 (void);	/* FPP exception handler */
IMPORT void emuInit (void);	/* FPP emulater initializer */
IMPORT VOIDFUNCPTR emu387Func;	/* function pointer to trap handler */
IMPORT VOIDFUNCPTR emuInitFunc;	/* function pointer to initializer */


/******************************************************************************
*
* mathSoftInit - initialize software floating-point math support
*
* This routine set two global pointers, and is called from usrConfig.c
* if INCLUDE_SW_FP is defined.  This definition causes the linker to
* include the floating-point emulation library.
*
* If the system is to use some combination of emulated as well as hardware
* coprocessor floating points, then this routine should be called before calling
* mathHardInit().
*
* RETURNS: N/A
*
* SEE ALSO: mathHardInit()
*
*/

void mathSoftInit (void)

    {

    emu387Func	= emu387;
    emuInitFunc	= emuInit;
    }
