/* wdbDbgLib.c - general breakpoint handling routines */

/* Copyright 1984-1998 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01c,08apr98,dbt  code cleanup.
01b,13mar98,dbt  added _func_wdbIsNowExternal definition.
01a,04dec97,dbt	 written.
*/

/*
DESCRIPTION
This library contains debugger routines. Those routines are used by both WDB
debugger and target shell debugger. Although this library is part of WDB,
we musn't have any reference to other WDB routines to avoid dependencies
between target shell debugger and WDB.

EXCEPTIONS
This library contains breakpoint handlers called by the special stub
routines (usually in assembly language). Those handlers choose between the
target shell handlers or the WDB handlers. If WDB is not included, the
target shell handler is used, otherwise we use the WDB handler.

WARNING
The target shell (native) debugger is based on the WDB core debugger. That's
why this library is used by both debuggers. The design was studied to keep
WDB and target shell independants. We must be very carefull when we modify
files shared by both debuggers in order to keep the independance.

Code that is share by both debuggers concerns task task handling (set
breakpoints during a task switch for example). Low level routines (handle
breakpoint exceptions, set breakpoint in memory, set registers for hardware
breakpoints ...)

The code specific to target shell debugger concerns only the interface 
between the user and the breakpoint handling routine (interface to set
a breakpoint, to remove a breakpoint or to display informations when a 
breakpoint is encountered).

Code specific to WDB core debugger concerns notification of host tools 
when a breakpoint is encountered, system mode handling and interface between
WDB routies and debugger routines.
*/

/* includes */

#include "vxWorks.h"
#include "cacheLib.h"
#include "intLib.h"
#include "wdb/wdbDbgLib.h"

/* globals */

dll_t	bpList;			/* breakpoint list */
dll_t   bpFreeList;		/* breakpoint free list */

#if	DBG_NO_SINGLE_STEP
FUNCPTR	_func_trap		= NULL;		/* trap handler */
#else	/* DBG_NO_SINGLE_STEP */
FUNCPTR	_func_breakpoint	= NULL;		/* breakpoint handler */
FUNCPTR	_func_trace		= NULL;		/* trace handler */
#endif	/* DBG_NO_SINGLE_STEP */
FUNCPTR	_func_wdbIsNowExternal	= NULL;		/* pointer on wdbIsNowExternal()
					   	   routine */

/******************************************************************************
*
* wdbDbgBpListInit - initialize the breakpoint lists
*
* This routine initializes the breakpoint list and the breakpoint free
* list.
* 
* RETURNS : N/A
*
* NOMANUAL
*/

void wdbDbgBpListInit
    (
    void
    )
    {
    static BOOL	bpListInitialized = FALSE;

    if (!bpListInitialized)
	{
	/* initialize breakpoint list */

	dll_init(&bpList);

	/* initialize breakpoint free list */

	dll_init(&bpFreeList);

	bpListInitialized = TRUE;
	}
    }

/******************************************************************************
*
* wdbDbgBpGet - find info about breakpoints at some address 
*
* In this routine we try to find info about breakpoints at a specified
* address.
* If there are breakpoints at the address, this routine returns info about
* the breakpoints by filling the fields of the breakpoint structure.
*
* IMPORTANT : This routine must be called with preemption locked.
*
* RETURNS: OK always.
*
* NOMANUAL
*/

STATUS wdbDbgBpGet
    (
    INSTR *	pc,		/* get info for bp at this address */
    int		context,	/* only get info for this context */
    int		type,		/* breakpoint type */
    BRKPT *	pBp		/* return info via this pointer */
    )
    {
    int 	action = 0;	/* action associated with breakpoint */
    int 	flags = 0;	/* breakpoint flag */
    dll_t *	pDll;

    for (pDll = dll_head(&bpList); pDll != dll_end(&bpList);
		pDll = dll_next(pDll))
	{
	/* 
	 * We check :
	 *	1 - breakpoint address.
	 *	2 - hardware breakpoint access type (if it is a hardware
	 *		breakpoint).
	 *	3 - the context ID.
	 */

	if ((BP_BASE(pDll)->bp_addr == pc) &&
	    ((type & (BRK_HARDWARE | BRK_HARDMASK)) ==
		(BP_BASE(pDll)->bp_flags & (BRK_HARDWARE | BRK_HARDMASK))) &&
	    ((BP_BASE(pDll)->bp_task == context) ||
	     ((BP_BASE(pDll)->bp_task == BP_ANY_TASK) &&
	      (context != BP_SYS))))
	    {
	    if (BP_BASE(pDll)->bp_count)	/* count != 0 */
		BP_BASE(pDll)->bp_count --;
	    else				/* count == 0 */
		{
		action |= BP_BASE(pDll)->bp_action;
		flags |= BP_BASE(pDll)->bp_flags;

		/* 
		 * If various breakpoints are set at the same address with 
		 * WDB_ACTION_CALL defined, only the latest call routine
		 * will be executed.
		 */

		if (BP_BASE(pDll)->bp_action & WDB_ACTION_CALL)
		    {
		    pBp->bp_callRtn = BP_BASE(pDll)->bp_callRtn;
		    pBp->bp_callArg = BP_BASE(pDll)->bp_callArg;
		    }
		}
	    }
	}
  
    /* fill action and flag fields in breakpoint structure */

    pBp->bp_action	= action;
    pBp->bp_flags	= flags;

    return (OK);
    }

/******************************************************************************
*
* wdbDbgBpFind - check if there is a breakpoints at some address
*
* In this routine we check if there is breakpoints at a specified
* address.
*
* IMPORTANT : This routine must be called with preemption locked.
*
* RETURNS: OK or ERROR if no breakpoint at that address.
*
* NOMANUAL
*/

STATUS wdbDbgBpFind
    (
    INSTR *		pc,		/* get info for bp at this address */
    int			context		/* only get info for this context */
    )
    {
    dll_t *	pDll;

    for (pDll = dll_head(&bpList); pDll != dll_end(&bpList);
		pDll = dll_next(pDll))
	{
	if ((BP_BASE(pDll)->bp_addr == pc) &&
		((BP_BASE(pDll)->bp_task == context) ||
		((BP_BASE(pDll)->bp_task == BP_ANY_TASK) &&
		 (context != BP_SYS))))
	    {
	    return (OK);
	    }
	}

    return (ERROR);
    }

/******************************************************************************
*
* wdbDbgBpRemoveAll - remove all breakpoints.
*
* Breakpoints are removed after a breakpoint is hit. This way
* the breakpoint handler  itself won't hit a breakpoint. They are 
* re-installed when the system is resumed.
* Also, during a context switch this routine is called to remove task
* specific breakpoints.
*
* IMPORTANT
* This routine must be called with preemption locked.
* 
* RETURNS : N/A.  
* 
* NOMANUAL 
*/

void wdbDbgBpRemoveAll 
    (
    void
    )
    {
    dll_t *	pDll;

#if	DBG_HARDWARE_BP
    /* clean debug registers */

    wdbDbgRegsClear ();
#endif	/* DBG_HARDWARE_BP */

    /* Remove all break points. */

    for (pDll = dll_head(&bpList); pDll != dll_end(&bpList);
	    pDll = dll_next(pDll))
	{
	if (BP_BASE(pDll)->bp_flags & BP_INSTALLED)
	    {
	    if ((BP_BASE(pDll)->bp_flags  & BRK_HARDWARE) == 0)
	    	usrBreakpointSet (BP_BASE(pDll)->bp_addr, 
					BP_BASE(pDll)->bp_instr);

	    BP_BASE(pDll)->bp_flags &= ~BP_INSTALLED;
	    }
	}
    }

/******************************************************************************
*
* wdbDbgBpRemove - remove a specific breakpoint.
*
* This routine removes a breakpoint. It first restore original instruction,
* then remove it from breakpoint list and add it in breakpoint free list.
*
* IMPORTANT : This routine must be called with preemption locked.
*
* RETURNS : OK always.
*
* NOMANUAL
*/

STATUS wdbDbgBpRemove 
    (
    BRKPT *	pBp	/* breakpoint to remove */
    )
    {
    dll_remove(&pBp->bp_chain);

    /* remove this breakpoint from memory if it is installed */

    if ((pBp->bp_flags & BP_INSTALLED) && ((pBp->bp_flags & BRK_HARDWARE) == 0))
	usrBreakpointSet (pBp->bp_addr, pBp->bp_instr);

    dll_insert (&pBp->bp_chain, &bpFreeList);

    return (OK);
    }

#if	DBG_NO_SINGLE_STEP
/*******************************************************************************
*
* wdbDbgTrap - handle hitting of breakpoint
*
* This routine handles the breakpoint trap.  It is called only from its
* special stub routine (usually in assembly language) which is connected
* to the breakpoint trap. It is used by targets that have to multiplex
* tracing and breakpoint.
*
* RETURNS : N/A
*
* NOMANUAL
*/

void wdbDbgTrap
    (
    INSTR *	addr,		/* pc value */
    REG_SET *	pRegisters,	/* task registers before breakpoint exception */
    void *	pInfo,		/* pointer on info */
    void *	pDbgRegSet,	/* pointer to debug register set */
    BOOL	hardware	/* indicates if it is a hardware breakpoint */
    )
    {
    int 	level;		/* level of interupt lock */

    level = intLock ();

    /* call the trap handler */

    if (_func_trap != NULL)
	_func_trap (level, addr, pInfo, pRegisters, pDbgRegSet, hardware);
    }

#else 	/* DBG_NO_SINGLE_STEP */
/*******************************************************************************
*
* wdbDbgTrace - handle trace exception
*
* This routine handles the trace trap.  It is called only
* from its special assembly language stub routine which is connected
* to the trace trap.
*
* RETURNS : N/A
*
* NOMANUAL
*/

void wdbDbgTrace
    (
    void *	pInfo,		/* pointer on info */
    REG_SET *	pRegisters	/* task registers before breakpoint exception */
    )
    {
    int		level;		/* level of interupt lock */

    level = intLock ();

    /* call the trace handler */

    if (_func_trace != NULL)
	_func_trace (level, pInfo, pRegisters);
    }

/*******************************************************************************
*
* wdbDbgBreakpoint - handle hitting of breakpoint
*
* This routine handles the breakpoint trap.  It is called only from its
* special stub routine (usually in assembly language) which is connected
* to the breakpoint trap.
*
* RETURNS : N/A
*
* NOMANUAL
*/

void wdbDbgBreakpoint
    (
    void *	pInfo,		/* pointer on info */
    REG_SET *	pRegisters,	/* pointer to register set */
    void *	pDbgRegSet,	/* pointer to debug registers */
    BOOL	hardware	/* indicates if it is a hardware breakpoint */
    )
    {
    int		level;		/* level of interrupt lock */
    
    level = intLock ();

    /* Remove all break points */

    wdbDbgBpRemoveAll();

    /* call the breakpoint handler */

    if (_func_breakpoint != NULL)
	_func_breakpoint (level, pInfo, pRegisters, pDbgRegSet, hardware);
    }
#endif	/* DBG_NO_SINGLE_STEP */
