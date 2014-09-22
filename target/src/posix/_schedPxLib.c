/* _schedPxLib.c - kernel support for the POSIX scheduling library */

/* Copyright 1984-2000 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01a,12sep00,jgn  written
*/

/*
DESCRIPTION: 
Provides support routines for the POSIX scheduler library that can be accessed
from outside the kernel (to cope with deficiencies in the core kernel API).
*/

/* INCLUDES */
#include "vxWorks.h"

/* externs */

extern BOOL	roundRobinOn;		/* round robin enabled? */
extern ULONG	roundRobinSlice;	/* period if it is */

/*******************************************************************************
*
* _schedPxKernelIsTimeSlicing - return current state of timeslicing & period
*
* This routine allows tasks to determine the current state of time slicing in
* the system, including the current period if time slicing is enabled. The
* parameter <pPeriod> may be passed as NULL if this information is not
* required. The value stored in <pPeriod> is only valid if the function
* returns TRUE.
*
* RETURNS: TRUE if time slicing enabled, FALSE otherwise. Also returns current
* time slice period via <pPeriod> if it is non-NULL and time slicing is enabled.
*
* SEE ALSO: kernelTimeSlice()
*
* NOMANUAL
*/
 
BOOL _schedPxKernelIsTimeSlicing
    (
    ULONG *	pPeriod                 /* place to store period, or NULL */
    )
    {
    if ((roundRobinOn) && (pPeriod != NULL))
        {
        *pPeriod = roundRobinSlice;
        }
 
    return roundRobinOn;
    }
