/* dbgTaskLib.c - Task breakpoint handling */

/* Copyright 1997-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01h,19apr02,jhw  Prevent breakpoint removal when running in System 
		 Mode (SPR 75987).
01g,09nov01,jhw  Revert WDB_INFO to be inside of WIND_TCB.
01f,17oct01,jhw  Access WDB_INFO from WIND_TCB pointer pWdbInfo.
01e,03feb99,cpd  Fix for SPR24102: Added Windview instrumentation when 
                 lockCnt is directly decremented, rather than taskUnlock.
01d,19jan99,elg  Restore PC for MIPS (SPR 24356)
01c,21apr98,dbt  code cleanup.
01b,13mar98,dbt  moved _func_wdbIsNowExternal to wdbDbgLib.c (scalability
                 problem).
01a,05dec97,dbt	 written based on wdbTaskBpLib.c.
*/

/*
DESCRIPTION

This library contains routines for task mode debugging. Those routines
are all OS specific and are used by both target shell debugger and 
WDB debugger in task mode.

The facilities provided by routines in this library are :
	- task deletion and switch handling.
	- breakpoint exceptions handling.
	- task continue and task step handling.

INTERNAL
We should avoid all direct calls to WDB routines without using a pointer.
This is necessary because target shell debugger and WDB debugger must be
independant.

Since target shell debugger is based on WDB debugger, it is normal to see
references to some WDB defines (WDB_CTX_LOAD for example) in this file.
*/

#include "vxWorks.h"
#include "taskLib.h"
#include "regs.h"
#include "stddef.h"
#include "sigLib.h"
#include "taskLib.h"
#include "taskHookLib.h"
#include "intLib.h"
#include "stdio.h"
#include "string.h"
#include "wdb/wdbDbgLib.h"
#include "private/taskLibP.h"
#include "private/kernelLibP.h"

/* defines */

#define DBG_INFO(p)	(&(((WIND_TCB *)(p))->wdbInfo))

/* externals */

extern FUNCPTR	_func_wdbIsNowExternal;	/* pointer on wdbIsNowExternal()
					   routine */
/* globals */

int	dbgLockUnbreakable;	/* taskLock()'ed tasks are unbreakable */
int	dbgSafeUnbreakable;	/* taskSafe()'ed tasks are unbreakable */
int	dbgUnbreakableOld;	/* unbreakable even if !WDB_ACTION_STOP */

FUNCPTR	_func_dbgHostNotify = NULL;	/* routine to call to notify the host
					   when a breakpoint is encountered */
FUNCPTR	_func_dbgTargetNotify = NULL;	/* routine to call to notify the target
					   when a breakpoint is encountered */

/* locals */

static int 	taskTraceData;
static int 	isrTraceData;

#if	DBG_NO_SINGLE_STEP
static INSTR *	taskNpc;	/* next pc of the task */
static INSTR 	taskNpcInstr;	/* next instruction of the task */
static INSTR *	isrNpc;		/* next pc at interrrupt level */
static INSTR 	isrNpcInstr;	/* next instruction at int. level */
#endif	/* DBG_NO_SINGLE_STEP */

/* forward static declarations */

LOCAL void	dbgTaskDoIgnore (REG_SET * pRegs);
LOCAL void	dbgTaskDoneIgnore (REG_SET * pRegs);
LOCAL void	dbgTaskSwitch (WIND_TCB * pOldTcb, WIND_TCB * pNewTcb);
LOCAL void	dbgTaskDeleteHook (WIND_TCB * pTcb);
LOCAL void	dbgTaskBpInstall (int tid);
LOCAL BOOL	dbgBrkIgnoreDefault (void);

/******************************************************************************
*
* dbgTaskBpHooksInstall - install tasking breakpoint hooks
*
* This routine installs breakpoint hooks. One for task switch, one for
* task deletion and one for modification of task flags.
*
* RETURNS : N/A.
*
* NOMANUAL
*/ 

void dbgTaskBpHooksInstall 
    (
    void
    )
    {
    static BOOL dbgTaskBpHooksInstalled = FALSE;	

    if (!dbgTaskBpHooksInstalled)
	{
	/* task switch hook */

	taskSwitchHookAdd ((FUNCPTR) dbgTaskSwitch);

	/* task delete hook */

	taskDeleteHookAdd ((FUNCPTR) dbgTaskDeleteHook);

	/* set break remove/install hook routine for taskOptionsSet */

	taskBpHookSet ((FUNCPTR) dbgTaskBpInstall);

	dbgTaskBpHooksInstalled = TRUE;
	}
    }

/*******************************************************************************
*
* dbgTaskCont - Handle a continue request
*
* The dbgTaskCont() function is used to continue a task.
*
* RETURNS : 
* WDB_ERR_INVALID_CONTEXT if the task is not suspended or 
* if we can't resume it. OK otherwise.
*
* NOMANUAL
*/

STATUS dbgTaskCont
    (
    UINT32	taskId		/* Id of task to continue */
    )
    {
    if (!taskIsSuspended(taskId))
	return (WDB_ERR_INVALID_CONTEXT);

    /* 
     * Check if there is a breakpoint at current pc or if the last 
     * encountered breakpoint still exist (needed for data access 
     * breakpoints).
     */

    taskLock ();	/* LOCK PREEMPTION */

    if ((wdbDbgBpFind ((INSTR *)(((WIND_TCB *)taskId)->regs.reg_pc),
		taskId) == OK) || 
	(wdbDbgBpFind ((INSTR *)(DBG_INFO(taskId)->bpAddr),
		taskId) == OK))
	{
	DBG_INFO(taskId)->wdbState |= WDB_STEP_OVER;
	}

    taskUnlock ();	/* UNLOCK PREEMPTION */

    if (taskResume (taskId) != OK)
	return (WDB_ERR_INVALID_CONTEXT);

    return (OK);
    }

/*******************************************************************************
*
* dbgTaskStep - Handle a step request
*
* The dbgTaskStep() function is used to step a task.
*
* RETURNS : 
* WDB_ERR_INVALID_CONTEXT if the task is not suspended or 
* if we can't resume it. OK otherwise.
*
* NOMANUAL
*/

STATUS dbgTaskStep
    (
    UINT32	contextId,	/* Id of task to step */
    UINT32	startAddr,	/* start address */
    UINT32	endAddr		/* end address */
    )
    {
    if (!taskIsSuspended(contextId))
	return (WDB_ERR_INVALID_CONTEXT);

    if (startAddr == 0 && endAddr == 0)
	DBG_INFO(contextId)->wdbState |= WDB_STEP;
    else
	{
	if (DBG_INFO(contextId)->wdbState & WDB_QUEUED)
	    return (WDB_ERR_INVALID_CONTEXT);

	DBG_INFO(contextId)->wdbState |= WDB_STEP_RANGE;
	DBG_INFO(contextId)->wdbEvtList.wdb1 = (void *) startAddr;
	DBG_INFO(contextId)->wdbEvtList.wdb2 = (void *) endAddr;
	}

    if (taskResume (contextId) != OK)
	return (WDB_ERR_INVALID_CONTEXT);

    return (OK);
    }

/******************************************************************************
*
* dbgTaskDeleteHook - remove task-specific BP's when a task exits
*
* This hook removes the task-specific breakpoints when a task exits.
*
* RETURNS : N/A
*
* NOMANUAL
*/ 

LOCAL void dbgTaskDeleteHook
    (
    WIND_TCB *	pTcb		/* TCB of the deleted task */
    )
    {
    dll_t *	pDll;
    dll_t *	pDllNext;
    int		level;

    /* remove events still queued in the task TCB */

    level = intLock ();

    if (DBG_INFO(pTcb)->wdbState & WDB_QUEUED)
	{
	DBG_INFO(taskIdCurrent)->wdbState &= ~WDB_QUEUED;
	dll_remove ((dll_t *)&(DBG_INFO(pTcb)->wdbEvtList));
	}

    intUnlock (level);

    /* delete task specific breakpoints */

    taskLock ();	/* LOCK PREEMPTION */

    for (pDll = dll_head(&bpList); pDll != dll_end(&bpList); pDll = pDllNext)
	{
	pDllNext = dll_next(pDll);

	/* We remove only breakpoints specific to deleted task. */

	if (BP_BASE(pDll)->bp_task == (int) pTcb)
	    wdbDbgBpRemove (BP_BASE(pDll));
	}
    
    taskUnlock ();	/* UNLOCK PREEMPTION */	
    }

/******************************************************************************
*
* dbgTaskBpInstall - install breakpoints for the current task
*
* This routine installs BP's for the new task and remove ones for 
* the old task.
*
* RETURNS : N/A.
*
* NOMANUAL
*/ 

LOCAL void dbgTaskBpInstall
    (
    int tid		/* id of the task */
    )
    {
    dll_t *	pDll;
    BOOL 	stepping;	/* task is stepping */
    BOOL	breakable;	/* task is breakable */
    BOOL	bpi;		/* breakpoint is installed */
    int		level;		/* interrupt lock level */
    BRKPT *	pBp;		
    WIND_TCB *	pTcb = (tid == 0 ? taskIdCurrent : (WIND_TCB *)tid);
#if	DBG_HARDWARE_BP
    DBG_REGS	dbgRegs;
#endif

    /* do not install or remove breakpoints when in system mode */

    if ((_func_wdbIsNowExternal != NULL) && _func_wdbIsNowExternal())
	return;

    /* clean local debug register buffer and the debug registers */

#if	DBG_HARDWARE_BP
    memset (&dbgRegs, 0, sizeof (DBG_REGS));
    wdbDbgRegsClear ();
#endif  /* DBG_HARDWARE_BP */

    /* set local flags */

    stepping = DBG_INFO(pTcb)->wdbState & WDB_STEPING;
    breakable = !(pTcb->options & VX_UNBREAKABLE);

    /*
     * Iterate through the breakpoint list and remove/install the 
     * breakpoints appropriate to the current task.
     */

    for (pDll = dll_head(&bpList); pDll != dll_end(&bpList);
	    pDll = dll_next(pDll))
	{
	pBp = BP_BASE(pDll);

	/* should the breakpoint be installed? */

	bpi = !(stepping ||
	    ((pBp->bp_task != BP_ANY_TASK) && (pBp->bp_task != (int)pTcb)) ||
	    ((!breakable) && (pBp->bp_action & WDB_ACTION_STOP)) ||
	    ((!breakable) && dbgUnbreakableOld));

	/* does the current BP state match the requested BP state? */

	if ((pBp->bp_flags & BP_INSTALLED) != bpi)
	    {
	    level = intLock ();

	    if (bpi)
		{
		pBp->bp_flags |= BP_INSTALLED;

		/* install software breakpoint only */

		if ((pBp->bp_flags & BRK_HARDWARE) == 0)
		    usrBreakpointSet (pBp->bp_addr, DBG_BREAK_INST);
		}
	    else
		{
		/* remove software breakpoint only */

		if ((pBp->bp_flags & BRK_HARDWARE) == 0)
		    usrBreakpointSet (BP_BASE(pDll)->bp_addr, 
					BP_BASE(pDll)->bp_instr);

		BP_BASE(pDll)->bp_flags &= ~BP_INSTALLED;
		}

	    intUnlock(level);
	    }

#if	DBG_HARDWARE_BP
	/* fill local dbgRegs structure with the HW breakpoint info */

	if (bpi && (pBp->bp_flags & BRK_HARDWARE))
	    wdbDbgHwBpSet (&dbgRegs, pBp->bp_flags & BRK_HARDMASK, 
						(UINT32) pBp->bp_addr);
#endif	/* DBG_HARDWARE_BP */
	}

#if	DBG_HARDWARE_BP
    /* 
     * Set CPU debug registers with the new info. 
     * This structure contains only hardware breakpoints for the new task.
     */

    wdbDbgRegsSet (&dbgRegs);	/* set debug registers. */
#endif	/* DBG_HARDWARE_BP */
    }

/*******************************************************************************
*
* dbgTaskSwitch - system task switch routine
*
* Tasking breakpoints are set during context switches using this routine.
* This allows breakpoints to not affect certain tasks.
*
* RETURNS : N/A.
*
* NOMANUAL
*/

LOCAL void dbgTaskSwitch 
    (
    WIND_TCB * pOldTcb,     /* pointer to tcb of switch-from task */
    WIND_TCB * pNewTcb      /* pointer to tcb of switch-to task */
    )
    {
    /* 
     * Don't do task BP switch hook if the agent is in system mode.
     * This test is only usefull if we use WDB breakpoints.
     */

    if ((_func_wdbIsNowExternal != NULL) && _func_wdbIsNowExternal())
	return;

    /* perform some cleanup on the old task */

    if (TASK_ID_VERIFY(pOldTcb) == OK)	/* suicide runs delete hook 1st */
	{
	if (DBG_INFO(pOldTcb)->wdbState & WDB_CLEANME)
	    {
	    taskSuspend((int) pOldTcb);
	    taskRegsSet((int) pOldTcb, DBG_INFO(pOldTcb)->wdbRegisters);
#if	CPU_FAMILY==MC680X0
	    pOldTcb->foroff = 0;
#endif	/* CPU_FAMILY==MC680X0 */
	    DBG_INFO(pOldTcb)->wdbState &= ~WDB_CLEANME;
	    }

	if (DBG_INFO(pOldTcb)->wdbState & WDB_STEPING)
	    {
#if	DBG_NO_SINGLE_STEP
	    usrBreakpointSet (taskNpc, taskNpcInstr);
#endif	/* DBG_NO_SINGLE_STEP */
	    wdbDbgTraceModeClear (&pOldTcb->regs, taskTraceData);
	    }
	}

    /*
     * If we are stepping, remove all BP's and set trace mode.
     *
     * else, if we are not stepping, remove breakpoints for the old task and
     * install them for new task.
     */

    if (DBG_INFO(pNewTcb)->wdbState & WDB_STEPING)
	{	
	/* remove all the breakpoints for this task */

	wdbDbgBpRemoveAll();

	/*
	 * if Single Step Mode (i.e. Trace Mode) doesn't exist on the CPU, 
	 * insert a breakpoint on the next instruction.
	 */

#if	DBG_NO_SINGLE_STEP
	taskNpc = wdbDbgGetNpc(&pNewTcb->regs);
	taskNpcInstr = *taskNpc;
	usrBreakpointSet (taskNpc, DBG_BREAK_INST);
#endif	/* DBG_NO_SINGLE_STEP */

	taskTraceData = wdbDbgTraceModeSet (&pNewTcb->regs);
	}
    else
	{
	/* remove breakpoints for old task and install them for new task */

	dbgTaskBpInstall((int)pNewTcb);
	}
    }

/*******************************************************************************
*
* dbgBrkIgnoreDefault - true if in situation to ignore breakpoint
*
* This is the default boolean function whose output determines whether the
* current task is in a situation where a breakpoint is to be ignored.  This
* routine is called indirectly via the global variable dbgBrkIgnore from the
* routine dbgBreakpoint.
*
* This routine specifies to ignore breakpoints if the breakpoint occurred in
* any of these situations:
*     (1) at interrupt level
*     (2) within kernel
*     (3) while task was unbreakable
*     (4) while preemption was disabled and dbgLockUnbreakable is TRUE
*     (5) while task was safe from deletion and dbgSafeUnbreakable is TRUE
*
* Note that (3) is actually an assertion because currently breakpoints are
* removed when a task is unbreakable.  But it might change someday.
*
* MS - removed check for (3) since I want to optionally allow non-breaking
* windview "eventpoints" in VX_UNBREAKABLE tasks (e.g., VX_UNBREAKABLE
* just means that the eventpoint can't cause the task to stop).
*
* RETURNS :
* TRUE if we should ignore the breakpoint FALSE otherwise
*
* NOMANUAL
*/

LOCAL BOOL dbgBrkIgnoreDefault 
    (
    void
    )
    {
    return ((intCnt > 0) ||
	    (kernelState == TRUE) ||
#if	FALSE
	    (taskIdCurrent->options & VX_UNBREAKABLE) ||
#endif	/* FALSE */
	    ((taskIdCurrent->lockCnt != 0) && (dbgLockUnbreakable)) ||
	    ((taskIdCurrent->safeCnt != 0) && (dbgSafeUnbreakable)));
    }


/******************************************************************************
*
* dbgTaskBpBreakpoint - generic task breakpoint handling
*
* This routines is the handler for breakpoints. It executes the actions
* binded to the breakpoint (notify the host, notify the target, call a 
* user's specified routine or stop the task).
*
* RETURNS : N/A.
*
* NOMANUAL
*/ 

void dbgTaskBpBreakpoint
    (
    int		level,		/* level of interupt lock */
    void *	pInfo,		/* pointer on info */
    REG_SET *	pRegisters,	/* task registers before breakpoint exception */
    void *	pDbgRegs,	/* pointer to debug registers */
    BOOL	hardware	/* indicates if it is a hardware breakpoint */
    )
    {
    BRKPT	bpInfo;		/* breakpoint info structure */
    UINT32	type = 0;	/* breakpoint type */
    UINT32	addr;		/* breakpoint address */

#if	!DBG_NO_SINGLE_STEP
    /* If the os wants us to ignore the break point, then do so */

    if (dbgBrkIgnoreDefault ())
	{
	dbgTaskDoIgnore (pRegisters);
	}
#endif	/* DBG_NO_SINGLE_STEP */

    taskLock();		/* LOCK PREEMPTION */
    intUnlock(level);

    /*
     * By default (software break point), the break point address is the
     * pc of the task.
     */

    addr = (UINT32) pRegisters->reg_pc;

#if	DBG_HARDWARE_BP
    /* If it is a hardware break point, update type and addr variable. */

    if (hardware)
	wdbDbgHwBpFind (pDbgRegs, &type, &addr);
#endif  /* DBG_HARDWARE_BP */

    /* 
     * Find in the break point list the one we have encountered and fill in the 
     * bpInfo structure. 
     */ 

    wdbDbgBpGet ((INSTR *) addr, (int)taskIdCurrent, type, &bpInfo);

    /* 
     * On some CPU (eg I960CX), the hardware breakpoint exception is sent only
     * after the instruction was executed. In that case, we consider that
     * the breakpoint address is the address where the processor is stopped.
     */

#if	DBG_HARDWARE_BP && defined (BRK_INST)
    if (type == (BRK_INST | BRK_HARDWARE))
	addr = (UINT32) pRegisters->reg_pc;
#endif	/* DBG_HARDWARE_BP && defined (BRK_INST) */

    /* 
     * Remove ACTION_STOP action if task is unbreakable. This is usefull
     * when two or more breakpoints are set at the same address, one with
     * action stop specified and one without.
     * XXX : this should be tested in wdbDbgBpGet() but in order to remove
     * all OS references in this routine used by the standalone agent debugger,
     * we can't check unbreakable flag in the task TCB.
     */

    if (taskIdCurrent->options & VX_UNBREAKABLE)
	bpInfo.bp_action &= ~WDB_ACTION_STOP;

    /* store the break point address */
    
    DBG_INFO(taskIdCurrent)->bpAddr = addr;

    /* check the action associated to the break point */

    if (bpInfo.bp_action & WDB_ACTION_CALL)
	bpInfo.bp_callRtn (bpInfo.bp_callArg, pRegisters);

    if (bpInfo.bp_action & WDB_ACTION_NOTIFY)
	{
	/* notify the host if the notify function pointer is initialised */

	if (_func_dbgHostNotify != NULL)
	    _func_dbgHostNotify (taskIdCurrent, pRegisters, addr);

	/* notify the target if the notify function pointer is initialised */

	if (_func_dbgTargetNotify != NULL)
	    _func_dbgTargetNotify (bpInfo.bp_flags, addr, pRegisters);
	}

#if	CPU_FAMILY == MIPS

    /*
     * On MIPS CPUs, when a breakpoint exception occurs in a branch delay slot,
     * the PC has been changed in the breakpoint handler to match with the
     * breakpoint address.
     * Once the matching has been made, the PC is modified to have its normal
     * value (the preceding jump instruction).
     */

    if (pRegisters->cause & CAUSE_BD)	/* Are we in a branch delay slot ? */
        pRegisters->reg_pc--;
#endif	/* CPU_FAMILY == MIPS */

    if (bpInfo.bp_action & WDB_ACTION_STOP)
	{
	DBG_INFO(taskIdCurrent)->wdbRegisters = pRegisters;
	DBG_INFO(taskIdCurrent)->wdbState |= WDB_CLEANME; 

	taskUnlock();		/* UNLOCK PREEMPTION */

	for (;;)
	    taskSuspend((int)taskIdCurrent);
	}

    /* !WDB_ACTION_STOP means continue (e.g., step over this break point) */

    DBG_INFO(taskIdCurrent)->wdbState |= WDB_STEP_OVER;
    wdbDbgBpRemoveAll();
#if	DBG_NO_SINGLE_STEP
    taskNpc = wdbDbgGetNpc (pRegisters);
    taskNpcInstr = *taskNpc;
    usrBreakpointSet (taskNpc, DBG_BREAK_INST);
#endif	/* DBG_NO_SINGLE_STEP */
    taskTraceData = wdbDbgTraceModeSet (pRegisters);
    intLock();

    /* Unlock task without rescheduling */

    taskIdCurrent->lockCnt--;
    
    /* Indicate change of taskLock state to WindView */
    
#ifdef WV_INSTRUMENTATION
    /* windview -level 3 event logging */
    EVT_CTX_0 (EVENT_TASKUNLOCK);
    /* windview -level 2 event logging */
    EVT_TASK_1 (EVENT_OBJ_TASK, taskIdCurrent);
#endif    
    
    WDB_CTX_LOAD (pRegisters);
    }

/******************************************************************************
*
* dbgTaskBpTrace - generic task trace (step) handling
*
* This routines is the handler for trace break point.
*
* RETURNS : N/A.
*
* NOMANUAL
*/ 

void dbgTaskBpTrace
    (
    int		level,			/* level of interupt lock */	
    void *	pInfo,			/* pointer to info saved on stack */
    REG_SET *	pRegisters		/* pointer to saved registers */	
    )
    {
    int 	wdbState;

#if	!DBG_NO_SINGLE_STEP
    if (isrTraceData != 0)
	dbgTaskDoneIgnore (pRegisters);
#else	/* DBG_NO_SINGLE_STEP */
    usrBreakpointSet (taskNpc, taskNpcInstr);
#endif	/* !DBG_NO_SINGLE_STEP */

    wdbDbgTraceModeClear (pRegisters, taskTraceData);

    DBG_INFO(taskIdCurrent)->bpAddr = (UINT32) pRegisters->reg_pc;

    if (DBG_INFO(taskIdCurrent)->wdbState & WDB_STEP_OVER)
	{
	DBG_INFO(taskIdCurrent)->wdbState &= ~WDB_STEP_OVER;
	dbgTaskBpInstall ((int)taskIdCurrent);
	WDB_CTX_LOAD (pRegisters);
	}

    if ((DBG_INFO(taskIdCurrent)->wdbState & WDB_STEP_RANGE) &&
	(DBG_INFO(taskIdCurrent)->wdbEvtList.wdb1 <= 
			    (void *) pRegisters->reg_pc) && 
	((void *) pRegisters->reg_pc < 
			    DBG_INFO(taskIdCurrent)->wdbEvtList.wdb2))
	{
#if	DBG_NO_SINGLE_STEP
	taskNpc = wdbDbgGetNpc(pRegisters);
	taskNpcInstr = *taskNpc;
	usrBreakpointSet (taskNpc, DBG_BREAK_INST);
#endif	/* DBG_NO_SINGLE_STEP */
	taskTraceData = wdbDbgTraceModeSet (pRegisters);
	WDB_CTX_LOAD (pRegisters);
	}

    taskLock();		/* LOCK PREEMPTION */
    intUnlock(level);

    /* blindly assume WDB_STEP or WDB_STEP_RANGE was set */

    wdbState = DBG_INFO(taskIdCurrent)->wdbState;

    DBG_INFO(taskIdCurrent)->wdbState &= ~(WDB_STEP_TARGET | WDB_STEP_RANGE);
    DBG_INFO(taskIdCurrent)->wdbRegisters = pRegisters;
    DBG_INFO(taskIdCurrent)->wdbState |= WDB_CLEANME;

    /* check if step was issued from target shell or WDB */

    if ((wdbState & WDB_STEP_TARGET) == WDB_STEP_TARGET)
	{
	if (_func_dbgTargetNotify != NULL)
	    _func_dbgTargetNotify (BP_STEP, pRegisters->reg_pc, pRegisters);
	}
    else
	{
	if (_func_dbgHostNotify != NULL)
	    _func_dbgHostNotify (taskIdCurrent, pRegisters, pRegisters->reg_pc);
	}

    taskUnlock();	/* UNLOCK PREEMPTION */

    for (;;)
	taskSuspend((int) taskIdCurrent);
    }

#if	DBG_NO_SINGLE_STEP
/******************************************************************************
*
* dbgTaskBpTrap - Trap handler.
*
* This routine handles bp's and trace's with one function if no trace mode
*
* RETURNS : N/A.
*
* NOMANUAL
*/ 

void dbgTaskBpTrap
    (
    int		level,		/* level of interupt lock */
    INSTR *	addr,		/* break point address */
    void *	pInfo,		/* pointer on info */
    REG_SET *	pRegisters,	/* task regs before break point exception */
    void *	pDbgRegs,	/* pointer to debug register set */
    BOOL	hardware	/* indicates if it is a hardware break point */
    )
    {
    /* remove all break point */

    wdbDbgBpRemoveAll ();

    /* step from a break point we were forced to ignore (e.g., in an ISR)? */

    if (isrTraceData != 0)
	{
	dbgTaskDoneIgnore (pRegisters);
	}

    /* do we have to ignore this break point (e.g., it's in an ISR)? */

    if (dbgBrkIgnoreDefault ())
	{
	dbgTaskDoIgnore (pRegisters);
	}

    if (DBG_INFO(taskIdCurrent)->wdbState & WDB_STEPING)
	dbgTaskBpTrace (level, pInfo, pRegisters);
    else
	dbgTaskBpBreakpoint (level, pInfo, pRegisters, pDbgRegs, hardware);

    /* NOTREACHED */
    }
#endif	/* DBG_NO_SINGLE_STEP */

/******************************************************************************
*
* dbgTaskDoIgnore - ignore current BP by stepping over it
*
* This routine assumes break point are uninstalled and interrupts
* are locked.
*
* RETURNS : N/A.
*
* NOMANUAL
*/ 

LOCAL void dbgTaskDoIgnore
    (
    REG_SET *	pRegs		/* pointer to saved registers */
    )
    {
#if	DBG_NO_SINGLE_STEP
    if (DBG_INFO(taskIdCurrent)->wdbState & WDB_STEPING)
	usrBreakpointSet (taskNpc, taskNpcInstr);
    isrNpc = wdbDbgGetNpc (pRegs);
    isrNpcInstr = *isrNpc;
    usrBreakpointSet (isrNpc, DBG_BREAK_INST);
#endif	/* DBG_NO_SINGLE_STEP */
    isrTraceData = wdbDbgTraceModeSet (pRegs);
    WDB_CTX_LOAD (pRegs);
    }

/******************************************************************************
*
* dbgTaskDoneIgnore - the step over from "dbgTaskDoIgnore" has completed
*
* The step over from "dbgTaskDoIgnore" has completed. We can now clear 
* trace mode and restore the task break point if we are not stepping.
*
* RETURNS : N/A.
*
* NOMANUAL
*/ 

LOCAL void dbgTaskDoneIgnore
    (
    REG_SET *	pRegs		/* task registers before exception */
    )
    {
    /* clear trace mode */

    wdbDbgTraceModeClear (pRegs, isrTraceData);

    /* we are no longer stepping from ISR */

    isrTraceData = 0;

#if	DBG_NO_SINGLE_STEP
    /* restore instructions */

    usrBreakpointSet (isrNpc, isrNpcInstr);

    if (DBG_INFO(taskIdCurrent)->wdbState & WDB_STEPING)
	{
	if (INT_CONTEXT())
	    usrBreakpointSet (taskNpc, DBG_BREAK_INST);
	else
	    DBG_INFO(taskIdCurrent)->wdbState &= ~WDB_STEPING;
	}
#endif	/* DBG_NO_SINGLE_STEP */

    /* If we are not stepping, install break point for the current task */

    if (!(DBG_INFO(taskIdCurrent)->wdbState & WDB_STEPING))
	dbgTaskBpInstall ((int)taskIdCurrent);

    /* reload task context */

    WDB_CTX_LOAD (pRegs);
    }
