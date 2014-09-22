/* intLib.c - architecture-independent interrupt subroutine library */

/* Copyright 1984-1994 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01m,17oct94,rhp  doc: remove obsolete reference to intALib (SPR#3712).
01l,20sep94,rhp  doc: do not mention exceptions in intLib man page (SPR#1494).
01k,21jan93,jdi  documentation cleanup for 5.1.
01j,07jul92,yao  moved from arch/mips.  moved intConnect(), areWeNested
		 back to arch/mips/intMipsLib.c.  cleaned up.
01i,04jul92,jcf  scalable/ANSI/cleanup effort.
01h,05jun92,ajm  5.0.5 merge, note mod history changes
01f,26may92,rrr  the tree shuffle
01e,14jan92,jdi  documentation cleanup.
01d,05nov91,ajm  added areWeNested for interrupt nesting
01c,16oct91,ajm  documentation
01b,04oct91,rrr  passed through the ansification filter
                  -changed includes to have absolute path from h/
                  -changed copyright notice
01a,26mar91,ajm  made arch independant.  derived from 03b 68K version.
*/

/*
DESCRIPTION
This library provides generic routines for interrupts.  Any C language
routine can be connected to any interrupt (trap) by calling
intConnect(), which resides in intArchLib.  The intCount() and intContext()
routines are used to determine whether the CPU is running in an
interrupt context or in a normal task context.  For information about
architecture-dependent interrupt handling, see the manual entry for
intArchLib.

INCLUDE FILES: intLib.h

SEE ALSO: intArchLib,
.pG "Basic OS"
*/

/* LINTLIBRARY */

#include "vxWorks.h"
#include "errno.h"
#include "intLib.h"

/* globals */

int intCnt = 0;		/* Counter to tell if we are at interrupt (non-task) */
			/* level.  Bumped and de-bumped by kernel */

/*******************************************************************************
*
* intRestrict - restrict interrupt context from using a routine
*
* This routine returns OK if and only if we are executing in a
* task's context and ERROR if called within an interrupt context.
*
* SEE ALSO: INT_RESTRICT() macro in intLib.h.
*
* RETURNS: TRUE or FALSE
*
* NOMANUAL
*/

STATUS intRestrict (void)

    {
    return ((intContext ()) ? errno = S_intLib_NOT_ISR_CALLABLE, ERROR : OK);
    }

/*******************************************************************************
*
* intContext - determine if the current state is in interrupt or task context
*
* This routine returns TRUE only if the current execution state is in
* interrupt context and not in a meaningful task context.
*
* RETURNS: TRUE or FALSE.
*/

BOOL intContext (void)

    {
    return (intCnt > 0);
    }

/*******************************************************************************
*
* intCount - get the current interrupt nesting depth
*
* This routine returns the number of interrupts that are currently nested.
*
* RETURNS: The number of nested interrupts.
*/

int intCount (void)

    {
    return (intCnt);
    }
