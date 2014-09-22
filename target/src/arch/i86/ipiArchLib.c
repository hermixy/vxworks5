/* ipiArchLib.c - I86 Inter Processor Interrupt (IPI) handling facilities */

/* Copyright 1984-2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01d,28feb02,hdn  initialized ipiHandlerTbl[] with NULL
		 moved IPI_MAX_XXX macros to ipiI86Lib.h
01c,27feb02,hdn  moved BSP part of shutdown code to sysShutdownSup()
01b,26feb02,hdn  replaced loApicIpiIntNumGet() to ipiIntNum
01a,20feb02,hdn  written
*/

/*
This module contains I80X86 architecture dependent portions of the
Inter Processor Interrupt (IPI) handling facilities.  See ipiALib for 
the companion assembly routines.

SEE ALSO: ipiALib
*/

#include "vxWorks.h"
#include "esf.h"
#include "iv.h"
#include "sysLib.h"
#include "intLib.h"
#include "taskLib.h"
#include "qLib.h"
#include "errno.h"
#include "string.h"
#include "vxLib.h"
#include "logLib.h"
#include "arch/i86/pentiumLib.h"
#include "arch/i86/ipiI86Lib.h"
#include "drv/intrCtl/loApic.h"


/* defines */

#undef	IPI_DBG
#ifdef	IPI_DBG
#   define IPI_DBG_MSG(STR,VALUE)	printf(STR,VALUE);
#else
#   define IPI_DBG_MSG(STR,VALUE)
#endif	/* IPI_DEBUG */


/* externals */

IMPORT int	sysCsExc;		/* CS for IPI */
IMPORT volatile UINT32 * sysNipi;	/* IPI counter */
IMPORT void 	sysShutdownSup ();	/* BSP shutdown routine */


/* globals */

VOIDFUNCPTR	ipiHandlerTbl [IPI_MAX_HANDLERS] = {NULL}; /* IPI handlers */


/* locals */

LOCAL UINT32	ipiIntNum;		/* IPI base intNum */


/* forward static functions */



/*******************************************************************************
*
* ipiVecInit - initialize the IPI vectors
*
* This routine sets all IPI vectors in IDT and assigns the default handlers.
* IPI_MAX_HANDLERS vectors are initialized starting from the IPI base vector.
*
* RETURNS: OK (always).
*/

void ipiVecInit 
    (
    UINT32 intNum		/* IPI base intNum */
    )
    {
    INT32 ix;

    ipiIntNum = intNum;		/* remember it */

    /* make all IPI vectors point to generic IPI handler */

    for (ix = 0; ix < IPI_MAX_HANDLERS; ix++, intNum++)
	{
	intVecSet2 ((FUNCPTR *)INUM_TO_IVEC (intNum),
		    (FUNCPTR) &ipiCallTbl[ix << 3],
		    IDT_INT_GATE, sysCsExc);
	}
    }

/*******************************************************************************
*
* ipiConnect - connect a C routine to the Inter Processor Interrupt
*
* This routine connects a specified C routine to a specified Inter Processor 
* Interrupt vector.  The address of <routine> is stored in ipiHandlerTbl[] 
* that is called by ipiHandler(index) when the IPI occurs.  The connected 
* routine is invoked in supervisor mode in the interrupt level. 
*
* The routine can be any normal C code, except that it must not invoke
* certain operating system functions that may block or perform I/O
* operations.
*
* NOMANUAL
*/

STATUS ipiConnect
    (
    UINT32	intNum,		/* IPI interrupt number */
    VOIDFUNCPTR	routine		/* routine to be called */
    )
    {
    /* sanity check */
  
    if ((intNum < ipiIntNum) || 
        (intNum >= (ipiIntNum + IPI_MAX_HANDLERS)))
        {
	return (ERROR);
	}

    /* register the routine */

    ipiHandlerTbl[intNum - ipiIntNum] = routine;

    return (OK);
    }

/*******************************************************************************
*
* ipiHandler - interrupt level IPI handling routine
*
* This routine handles Inter Processor Interrupt (IPI).  It is never to be 
* called except from the special assembly language interrupt stub routine.
* Since it runs in the interrupt level, the task level work may be deferred 
* by excJobAdd()/logMsg(). 
*
* RETURNS: N/A
*
* NOMANUAL
*/

void ipiHandler
    (
    UINT32	index		/* index in the ipiHandlerTbl[] */
    )
    {

    /* send EOI */

    *(int *)(loApicBase + LOAPIC_EOI) = 0;

    /* sanity check */
  
    if (index >= IPI_MAX_HANDLERS)
	return;

    /* increment the counter */

    sysNipi[index]++;

    /* call the registered IPI handler */

    if (ipiHandlerTbl[index] != NULL)
        (*(ipiHandlerTbl[index])) ();
    }

/*******************************************************************************
*
* ipiHandlerShutdown - interrupt level Shutdown IPI handling routine
*
* This routine handles Shutdown Inter Processor Interrupt (IPI).
*
* Note that this routine runs in the context of the task that got the IPI.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void ipiHandlerShutdown (void)
    {

    /* 
     * do it in the interrupt level or task level?
     * doing it in the interrupt level 
     * - is more reliable (works even if the kernel is misbehaving)
     * - doesn't require context switch (to excTask) working
     * - happens immediatly (may be nested interrupt)
     * doing it in the task level 
     * - is more graceful
     * - require context switch (to excTask) working
     * - servicing interrupts will be completed
     * What about having two handlers, one for each?
     */
     
    sysShutdownSup ();			/* shutdown the included components */

    loApicEnable (FALSE);		/* disable the LOAPIC */

    intLock ();				/* LOCK INTERRUPTS */

    ipiShutdownSup ();			/* never come back */
    }

/*******************************************************************************
*
* ipiStartup - start up the specified Application Processor (AP)
*
* This routine starts up the specified Application Processor (AP).
*
* RETURNS: N/A
*
* NOMANUAL
*/

STATUS ipiStartup
    (
    UINT32	apicId,		/* AP's local APIC ID */
    UINT32	vector,		/* entry point address */
    UINT32	nTimes		/* send SIPI nTimes */
    )
    {
    INT32 ix;

    /* BSP sends AP an INIT IPI */

    if (loApicIpi (apicId, 0, 0, 1, 0, 5, 0) != OK)
	{
	IPI_DBG_MSG ("failed - 1st INIT %d\n",0)
	return (ERROR);
	}
    if (loApicIpi (apicId, 0, 1, 0, 0, 5, 0) != OK)
	{
	IPI_DBG_MSG ("failed - 2nd INIT %d\n",0)
	return (ERROR);
	}

    /* BSP delay 10msec */

    for (ix = 0; ix < 15000; ix++)	/* 15000*720 ~= 10.8 msec */
         sysDelay ();			/* 720ns */

    while (nTimes-- > 0)
	{
        /* BSP sends AP a STARTUP IPI again */

        if (loApicIpi (apicId, 0, 0, 1, 0, 6, (vector >> 12)) != OK)
	    {
	    IPI_DBG_MSG ("failed - SIPI %d\n", nTimes)
	    return (ERROR);
	    }

        /* BSP delays 200usec again */
    
        for (ix = 0; ix < 300; ix++)	/* 300*720 ~= 216usec */
             sysDelay ();		/* 720ns */
	}

    return (OK);
    }

/*******************************************************************************
*
* ipiShutdown - shutdown the specified Application Processor (AP)
*
* This routine shutdowns the specified Application Processor (AP).
*
* NOMANUAL
*/

STATUS ipiShutdown
    (
    UINT32	apicId,		/* AP's local APIC ID */
    UINT32	intNum		/* intNum of IPI_SHUTDOWN */
    )
    {
    STATUS status = OK;
    INT32 retry = 0;

    /* BP sends AP a SHUTDOWN IPI */

    while (loApicIpi (apicId, 0, 0, 1, 0, 0, intNum) != OK)
	{
        taskDelay (1);
	if (retry++ > IPI_MAX_RETRIES)
	    {
            status = ERROR;
	    break;
	    }
	}

    return (status);
    }

#if	FALSE	/* XXX will be done soon */

/*******************************************************************************
*
* ipiFixedDest - send IPI to the specified Application Processor
*
* This routine sends IPI to the specified Application Processor
*
* NOMANUAL
*/

STATUS ipiFixedDest
    (
    UINT32	apicId,		/* AP's local APIC ID */
    UINT32	intNum		/* intNum to send */
    )
    {
    }

/*******************************************************************************
*
* ipiFixedSelf - send IPI to myself
*
* This routine sends IPI to myself
*
* NOMANUAL
*/

STATUS ipiFixedSelf
    (
    UINT32	intNum		/* intNum to send */
    )
    {
    }

/*******************************************************************************
*
* ipiFixedAlli - send IPI to all processors including self
*
* This routine sends IPI to all processors including self
*
* NOMANUAL
*/

STATUS ipiFixedAlli
    (
    UINT32	intNum		/* intNum to send */
    )
    {
    }

/*******************************************************************************
*
* ipiFixedAlle - send IPI to all processors excluding self
*
* This routine sends IPI to all processors excluding self
*
* NOMANUAL
*/

STATUS ipiFixedAlle
    (
    UINT32	intNum		/* intNum to send */
    )
    {
    }

/*******************************************************************************
*
* ipiNmiDest - send NMI to the specified Application Processor
*
* This routine sends NMI to the specified Application Processor
*
* NOMANUAL
*/

STATUS ipiNmiDest
    (
    UINT32	apicId		/* AP's local APIC ID */
    )
    {
    }

/*******************************************************************************
*
* ipiNmiSelf - send NMI to myself
*
* This routine sends NMI to myself
*
* NOMANUAL
*/

STATUS ipiNmiSelf (void)
    {
    }

/*******************************************************************************
*
* ipiNmiAlli - send NMI to all processors including self
*
* This routine sends NMI to all processors including self
*
* NOMANUAL
*/

STATUS ipiNmiAlli (void)
    {
    }

/*******************************************************************************
*
* ipiNmiAlle - send NMI to all processors excluding self
*
* This routine sends NMI to all processors excluding self
*
* NOMANUAL
*/

STATUS ipiNmiAlle (void)
    {
    }

/*******************************************************************************
*
* ipiLowestAlli - send IPI to the lowest priority processor including self
*
* This routine sends IPI to the lowest priority processor including self
*
* NOMANUAL
*/

STATUS ipiLowestAlli
    (
    UINT32	intNum		/* intNum to send */
    )
    {
    }

/*******************************************************************************
*
* ipiLowestAlle - send IPI to the lowest priority processor excluding self
*
* This routine sends IPI to the lowest priority processor excluding self
*
* NOMANUAL
*/

STATUS ipiLowestAlle
    (
    UINT32	intNum		/* intNum to send */
    )
    {
    }

#endif
