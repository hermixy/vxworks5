/* excArchShow.c - I80X86 exception show facilities */

/* Copyright 1984-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01d,28aug01,hdn  added new SSE SIMD exception 
		 added esp, ss, esp0, cr[23], esp0[07] to EXC_INFO.
01c,09apr98,hdn  added support for Pentium, PentiumPro.
01b,29may94,hdn  removed I80486 conditional.
01a,08jun93,hdn  extracted from excI86Lib.c.
*/

/*
This module contains I80X86 architecture dependent portions of the
exception handling facilities.  See excLib for the portions that are
architecture independent.

SEE ALSO: dbgLib, sigLib, intLib, "Debugging"
*/

#include "vxWorks.h"
#include "esf.h"
#include "iv.h"
#include "taskLib.h"
#include "errno.h"
#include "string.h"
#include "logLib.h"
#include "stdio.h"
#include "fioLib.h"
#include "intLib.h"
#include "qLib.h"
#include "private/kernelLibP.h"
#include "private/funcBindP.h"


/* globals */

FUNCPTR excMcaInfoShow = NULL;


/* locals */

/* 
 * Exception error messages.  These are used by the exception printing routine.
 * Exception numbers are the same as used by the CPU.
 */

LOCAL char *excMsgs [] =
    {
    "Divide Error",			/*  0 */
    "Debug",				/*  1 */
    "Nonmaskable Interrupt",		/*  2 */
    "Breakpoint",			/*  3 */
    "Overflow",				/*  4 */
    "Bound",				/*  5 */
    "Invalid Opcode",			/*  6 */
    "Device Not Available",		/*  7 */
    "Double Fault",			/*  8 */
    "Coprocessor Overrun",		/*  9 */
    "Invalid TSS",			/* 10 */
    "Segment Not Present",		/* 11 */
    "Stack Fault",			/* 12 */
    "General Protection Fault",		/* 13 */
    "Page Fault",			/* 14 */
    "Intel Reserved",			/* 15 */
    "Coprocessor Error",		/* 16 */
    "Alignment Check",			/* 17 */
    "Machine Check",			/* 18 */
    "Streaming SIMD",			/* 19 */
    };

LOCAL char *excIntInfoFmt = "\n\
Uninitialized Interrupt!\n\
Vector number %d (0-255). %s\n\
Supervisor ESP : 0x%08x\n\
Program Counter: 0x%08x\n\
Code Selector  : 0x%08x\n\
Eflags Register: 0x%08x\n";


/* forward declarations */

LOCAL void excInfoShow	  (EXC_INFO * pExcInfo, BOOL doBell);
LOCAL void excIntInfoShow (int vecNum, ESF0 * pEsf, REG_SET * pRegs,
			   EXC_INFO * pExcInfo);
LOCAL void excPanicShow	  (int vecNum, ESF0 * pEsf, REG_SET * pRegs,
			   EXC_INFO * pExcInfo);


/*******************************************************************************
*
* excShowInit - initialize exception show facility
*
* NOMANUAL
*/

STATUS excShowInit (void)
    {
    _func_excInfoShow	= (FUNCPTR) excInfoShow;
    _func_excIntHook	= (FUNCPTR) excIntInfoShow;
    _func_excPanicHook	= (FUNCPTR) excPanicShow;

    return (OK);
    }

/*******************************************************************************
*
* excInfoShow - print exception info
*
* NOMANUAL
*/

LOCAL void excInfoShow
    (
    EXC_INFO *	pExcInfo,	/* exception information to summarize */
    BOOL	doBell		/* print task id and ring warning bell */
    )
    {
    int valid	= pExcInfo->valid;
    int vecNum	= pExcInfo->vecNum;
    char * extraExcMsg = "";	/* additional message for exec/rd/wr
				 * access to oool unmapped area, or 
				 * stack gard pages 
				 */

    if (valid & EXC_VEC_NUM)
	{
	if ((vecNum < NELEMENTS (excMsgs)) && (excMsgs [vecNum] != NULL))
	    printExc ("\n%s\n", (int) excMsgs [vecNum], 0, 0, 0, 0);
	else
	    printExc ("\nTrap to uninitialized vector number %d (0-255).\n",
		      vecNum, 0, 0, 0, 0);
	printExc ("Page Dir Base   : 0x%08x\n", pExcInfo->cr3, 0, 0, 0, 0);
        printExc ("Esp0 0x%08x : 0x%08x, 0x%08x, 0x%08x, 0x%08x\n",
		  pExcInfo->esp0, pExcInfo->esp00, pExcInfo->esp01,
		  pExcInfo->esp02, pExcInfo->esp03);
        printExc ("Esp0 0x%08x : 0x%08x, 0x%08x, 0x%08x, 0x%08x\n",
		  pExcInfo->esp0+16, pExcInfo->esp04, pExcInfo->esp05,
		  pExcInfo->esp06, pExcInfo->esp07);
	printExc ("Program Counter : 0x%08x\n", (int)pExcInfo->pc, 0, 0, 0, 0);
	printExc ("Code Selector   : 0x%08x\n", pExcInfo->cs, 0, 0, 0, 0);
	printExc ("Eflags Register : 0x%08x\n", pExcInfo->eflags, 0, 0, 0, 0);
	}

    if (valid & EXC_ERROR_CODE)
	printExc ("Error Code      : 0x%08x\n", pExcInfo->errCode, 0, 0, 0, 0);

    if (valid & EXC_CR2)
	{
	printExc ("Page Fault Addr : 0x%08x %s\n", pExcInfo->cr2, 
		  (int)extraExcMsg, 0, 0, 0);
	}

    if (doBell)
	printExc ("Task: %#x \"%s\"\007\n", (int)taskIdCurrent, 
		  (int)taskName ((int)taskIdCurrent), 0, 0, 0);

    if ((vecNum == 18) && (excMcaInfoShow != NULL))
	(* excMcaInfoShow) ();

    }

/*******************************************************************************
*
* excIntInfoShow - print out uninitialized interrupt info
*/

LOCAL void excIntInfoShow
    (
    int		vecNum,		/* exception vector number */
    ESF0 *	pEsf,		/* pointer to exception stack frame */
    REG_SET *	pRegs,		/* pointer to register info on stack */
    EXC_INFO *	pExcInfo	/* parsed exception information */
    )
    {
    char * vecName = "";
    int valid      = pExcInfo->valid;

    if ((vecNum < NELEMENTS (excMsgs)) && (excMsgs [vecNum] != NULL))
	vecName = excMsgs [vecNum];

    if (Q_FIRST (&activeQHead) == NULL)			/* pre kernel */
	{
        if (valid & EXC_VEC_NUM)
	    {
	    printExc ("\nUninitialized Interrupt!\nVector No %d (0-255). %s\n",
		      vecNum, (int)vecName, 0, 0, 0);
            printExc ("Supervisor ESP : 0x%08x\n", pExcInfo->esp0, 0, 0, 0, 0);
	    printExc ("Program Counter: 0x%08x\n", (int)pExcInfo->pc, 0,0,0,0);
	    printExc ("Code Selector  : 0x%08x\n", pExcInfo->cs, 0,0,0,0);
	    printExc ("Eflags         : 0x%08x\n", pExcInfo->eflags, 0,0,0,0);
	    }
	}
    else
	{
	logMsg (excIntInfoFmt, vecNum, (int)vecName, (int)pExcInfo->esp0,
		(int)pExcInfo->pc, (int)pExcInfo->cs, (int)pExcInfo->eflags);
	}
    }

/*******************************************************************************
*
* excPanicShow - exception at interrupt level
*
* This routine is called if an exception is caused at interrupt
* level.  We can't handle it in the usual way.  Instead, we save info in
* sysExcMsg and trap to rom monitor.
*/

LOCAL void excPanicShow
    (
    int		vecNum,		/* exception vector number */
    ESF0 *	pEsf,		/* pointer to exception stack frame */
    REG_SET *	pRegs,		/* pointer to register info on stack */
    EXC_INFO *	pExcInfo	/* parsed exception information */
    )
    {
    if (INT_CONTEXT ())
	printExc (" \nException at interrupt level:\n", 0, 0, 0, 0, 0);

    if (kernelState != FALSE)
	printExc (" \nException in kernel state:\n", 0, 0, 0, 0, 0);

    if (Q_FIRST (&activeQHead) == NULL)
	printExc ("Exception before kernel initialized:\n", 0, 0, 0, 0, 0);

    excInfoShow (pExcInfo, FALSE);	/* print the message into sysExcMsg */

    printExc ("Regs at 0x%x\n", (int) pRegs, 0, 0, 0, 0);
    }


