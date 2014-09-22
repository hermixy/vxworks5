/* dspShow.c - dsp show routines */

/* Copyright 1984-1998 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01a,22jul98,mem  written.
*/

/*
DESCRIPTION
This library provides the routines necessary to show a task's optional 
dsp context.  To use this facility, it must first be
installed using dspShowInit().  The facility is included
automatically when INCLUDE_SHOW_ROUTINES and INCLUDE_DSP are defined
in configAll.h. 

This library enhances task information routines, such as ti(), to display
the dsp context.

INCLUDE FILES: dspLib.h 

SEE ALSO: dspLib
*/

#include "vxWorks.h"
#include "stdio.h"
#include "regs.h"
#include "dspLib.h"
#include "private/funcBindP.h"

/* global variables */

char *dspTaskRegsFmt = "%-6.6s = %8x";


/******************************************************************************
*
* dspShowInit - initialize the dsp show facility
*
* This routine links the dsp show facility into the VxWorks system.
* The facility is included automatically when \%INCLUDE_SHOW_ROUTINES and
* \%INCLUDE_DSP are defined
* in configAll.h.
*
* RETURNS: N/A
*/

void dspShowInit (void)
    {
    /* avoid direct coupling with dspShow with this global variable */

    _func_dspTaskRegsShow = (FUNCPTR) dspTaskRegsShow;
    }

/*******************************************************************************
*
* dspTaskRegsShow - print the contents of a task's dsp registers
*
* This routine prints to standard output the contents of a task's
* dsp registers.
*
* RETURNS: N/A
*/

void dspTaskRegsShow
    (
    int task		/* task to display dsp registers for */
    )
    {
    int		ix;
    ULONG *	dspTmp;
    ULONG *	dspCtlTmp;
    DSPREG_SET	dspRegSet;

    if ((dspProbe() != OK) || (dspTaskRegsGet (task, &dspRegSet) == ERROR))
	return;

    /* Some architectures organize floats sequentially, but to
     * display them as doubles the floats need to be word swapped.
     */

    if (dspDisplayHookRtn != NULL)
        (* dspDisplayHookRtn) (&dspRegSet);

    /* print dsp control registers */

    for (ix = 0; dspCtlRegName[ix].regName != (char *) NULL; ix++)
	{
	if ((ix % 4) == 0)
	    printf ("\n");
	else
	    printf ("%3s","");

	dspCtlTmp = (ULONG *) ((int)&dspRegSet + dspCtlRegName[ix].regOff);
	printf (dspTaskRegsFmt, dspCtlRegName[ix].regName, *dspCtlTmp);
	}

    /* print dsp data registers */

    for (ix = 0; dspRegName[ix].regName != (char *) NULL; ix++)
	{
	if ((ix % 4) == 0)
	    printf ("\n");
	else
	    printf ("%3s","");
	
	dspTmp = (ULONG *) ((int)&dspRegSet + dspRegName[ix].regOff);
	printf (dspTaskRegsFmt, dspRegName[ix].regName, *dspTmp);
	}
    printf ("\n");
    }
