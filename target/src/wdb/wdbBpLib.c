/* wdbBpLib.c - Break point handling for the target server */

/* Copyright 1994-2001 Wind River Systems, Inc. */

/*
modification history
--------------------
01u,14sep01,jhw  Fixed warnings from compiling with gnu -pedantic flag
01t,16nov98,cdp  enable Thumb support for all ARM CPUs with ARM_THUMB==TRUE.
01v,21apr99,dbt  removed TEXT_LOCK() and TEXT_UNLOCK() calls in wdbHwBpAdd()
		 routine (SPR #26927).
01t,19jan99,elg  Restore PC for MIPS (SPR 24356)
01s,10jul98,dbt  test if task mode agent is running before stepping or
                 continue a task.
01r,11may98,dbt  removed useless wdbTaskLock() and wdbTaskUnlock() calls in 
		 wdbSysBpLibInit().
01q,29apr98,dbt  code cleanup.
01p,21apr98,dbt  fixed Thumb (ARM7TDMI_T) support.
01o,11jul97,cdp	 added Thumb (ARM7TDMI_T) support.
01n,09mar98,dbt  fixed a problem with WDB_TASK_LOCK() and WDB_TASK_UNLOCK()
                 defines.
01m,13feb98,dbt  fixed typo in wdbBpLibInit().
01l,26jan98,dbt  replaced wdbEventClassConnect() with wdbEvtptClassConnect().
		 replaced WDB_EVT_CLASS with WDB_EVTPT_CLASS
01k,05dec97,dbt  merge with target shell debugger. Added support for hardware
		 breakpoints.
01j,28aug96,tam	 modified wdbCont() to fix SPR 7098.
01k,15jul96,ms   fixed problem with trace mode
01j,26jun96,ms   added target side code to support windview "e" command.
		 removed redundant call to wdbBpRemove().
		 2nd paramter to eventpoint call func is now a REG_SET *.
01i,03jun96,kkk  replaced _sigCtxLoad with WDB_CTX_LOAD.
		 replaced pRegister->pc with pRegister->reg_pc.		
01h,24apr96,ms   support generalized eventPoint handling.
		 support multiple breakpoints at same address.
		 lots of cleanup.
01g,23jan96,tpr  added cast to compile with DIAB DATA toolkit.
01f,21sep95,ms   unprotect mem before check for valid BP address (SPR 4935)
01e,17jul95,ms	 changed the boundry test on STEP_RANGE
01d,03jun95,ms	 deleting BP id = -1 means remove all breapoints.
01c,01jun95,ms	 check for WDB_CTX_ANY_TASK on BP add.
		 do a memProbe on BP add.
		 don't add task BP's in system mode.
		 deactivate system BP's when in task mode.
		 some cleanup.
01b,28mar95,ms	 wdbBpAdd now uses contextType to determine task vs system BP.
01a,02nov94,rrr	 written.
*/

/*
DESCPRIPTION

This library contains break point handling for system mode debugging.
It also contains code which is shared with wdbTaskBpLib.c.
*/

/* includes */

#include "vxWorks.h"
#include "string.h"
#include "regs.h"
#include "wdb/wdb.h"
#include "wdb/dll.h"
#include "wdb/wdbLibP.h"
#include "wdb/wdbLib.h"
#include "wdb/wdbBpLib.h"
#include "wdb/wdbSvcLib.h"
#include "wdb/wdbEvtLib.h"
#include "wdb/wdbEvtptLib.h"
#include "wdb/wdbArchIfLib.h"
#include "wdb/wdbDbgLib.h"

/* externals */

extern WDB_IU_REGS	wdbExternSystemRegs;
extern BOOL		wdbOneShot;

#if	DBG_NO_SINGLE_STEP
extern FUNCPTR 		_func_trap;
#else	/* DBG_NO_SINGLE_STEP */
extern FUNCPTR 		_func_breakpoint;
extern FUNCPTR 		_func_trace;
#endif	/* DBG_NO_SINGLE_STEP */
extern FUNCPTR 		_func_wdbIsNowExternal;

/* defines */

#if     CPU_FAMILY==MC680X0
#undef reg_pc
#define reg_pc	regSet.pc
#undef reg_sp
#define reg_sp	regSet.addrReg[7]
#undef reg_fp
#define reg_fp	regSet.addrReg[6]
#endif  /* CPU_FAMILY==MC680X0 */

/* globals */

UINT32  (*_wdbTaskBpAdd)	(WDB_EVTPT_ADD_DESC * pEv);
UINT32  (*_wdbTaskStep)		(UINT32 contextId, TGT_ADDR_T startAddr,
					TGT_ADDR_T endAddr);
UINT32  (*_wdbTaskCont)         (UINT32 contextId); 
#if	DBG_NO_SINGLE_STEP
void    (*_wdbTaskBpTrap)       (int level, INSTR * addr, void * pInfo, 
				    WDB_IU_REGS * pRegisters, void * pDbgRegs, 
				    BOOL hardware);
#else	/* DBG_NO_SINGLE_STEP */
void    (*_wdbTaskBpBreakpoint) (int level, void * pInfo, 
				    WDB_IU_REGS * pRegisters, void * pDbgRegs,
				    BOOL hardware);
void    (*_wdbTaskBpTrace)      (int level, void * pInfo, 
				    WDB_IU_REGS * pRegisters);
#endif	/* DBG_NO_SINGLE_STEP */

/* locals */

static int	wdbBpData;		/* store breakpoint informations */
static UINT32	wdbBpAddr;		/* store the address of the last */
					/* encountered breakpoint */
static int	wdbSysBpMode;		/* mode of the system context */
static INSTR *	wdbSysStepStart;	/* start address of step range */
static INSTR *	wdbSysStepEnd;		/* end address of step range */

#if	DBG_NO_SINGLE_STEP
static INSTR *	wdbSysNpc;
static INSTR	wdbSysNpcInstr;
#endif	/* DBG_NO_SINGLE_STEP */

static WDB_EVT_NODE	eventSysBpNode;	/* system mode breakpoint event node */
static WDB_EVTPT_CLASS	wdbEventClassBp;/* breakpoint event class */
#if	DBG_HARDWARE_BP
static WDB_EVTPT_CLASS	wdbEventClassHwBp;/* hardware breakpoint event class */
#endif	/* DBG_HARDWARE_BP */

/* forward static declarations */

static UINT32 wdbStep		(WDB_CTX_STEP_DESC * pCtxStep);
static UINT32 wdbCont		(WDB_CTX * ctx);
static UINT32 wdbBpAdd		(WDB_EVTPT_ADD_DESC * pBp, UINT32 * pId);
static UINT32 wdbBpDelete	(TGT_ADDR_T *pId);
static void wdbSysBpEventGet	(void * arg, WDB_EVT_DATA * pEventMsg);
static void wdbTrace		(int level, void * pInfo, 
				    WDB_IU_REGS * pRegisters);
static void wdbBreakpoint	(int level, void * pInfo, 
				    WDB_IU_REGS * pRegisters, void * pDbgRegs, 
				    BOOL hardware);
static void wdbSysBpPost	(int addr);
static void wdbTaskLock		(void);
static void wdbTaskUnlock	(void);
#if	DBG_HARDWARE_BP
static UINT32 wdbHwBpAdd	(WDB_EVTPT_ADD_DESC * pBp, UINT32 * pId);
#endif	/* DBG_HARDWARE_BP */
#if	DBG_NO_SINGLE_STEP
static void wdbTrap		(int level, INSTR * addr, void * pInfo, 
				    WDB_IU_REGS * pRegisters, void * pDbgRegs, 
				    BOOL hardware);
#endif	/* DBG_NO_SINGLE_STEP */

/******************************************************************************
*
* wdbSysBpLibInit - initialize the library.
*
* This routine initializes the agent debugger library : wdb services,
* breakpoint lists, event nodes and some pointers used by the debugger.
*
* RETURNS : N/A 
* 
* NOMANUAL
*/

void wdbSysBpLibInit
    (
    BRKPT *	pBps,	/* pointer on breakpoint structure */
    int		bpCnt	/* number of breakpoints */
    )
    {
    static int	wdbBpInstalled = FALSE;

    if (!wdbBpInstalled)
	{
	wdbSvcAdd (WDB_CONTEXT_STEP, wdbStep, xdr_WDB_CTX_STEP_DESC, xdr_void);
	wdbSvcAdd (WDB_CONTEXT_CONT, wdbCont, xdr_WDB_CTX, xdr_void);

	wdbEventClassBp.evtptType = WDB_EVT_BP;
	wdbEventClassBp.evtptAdd = wdbBpAdd;
	wdbEventClassBp.evtptDel = wdbBpDelete;
	wdbEvtptClassConnect (&wdbEventClassBp);

#if	DBG_HARDWARE_BP
	wdbEventClassHwBp.evtptType = WDB_EVT_HW_BP;
	wdbEventClassHwBp.evtptAdd = wdbHwBpAdd;
	wdbEventClassHwBp.evtptDel = wdbBpDelete;
	wdbEvtptClassConnect (&wdbEventClassHwBp);
#endif	/* DBG_HARDWARE_BP */

#if	DBG_NO_SINGLE_STEP
	_func_trap = (FUNCPTR) wdbTrap;
#else   /* DBG_NO_SINGLE_STEP */
	_func_breakpoint = (FUNCPTR) wdbBreakpoint;
	_func_trace = (FUNCPTR) wdbTrace;
#endif  /* DBG_NO_SINGLE_STEP */

	_func_wdbIsNowExternal = (FUNCPTR) wdbIsNowExternal;

	wdbDbgBpListInit();

	while (bpCnt)
	    {
	    dll_insert (&pBps->bp_chain, &bpFreeList);
	    ++pBps;
	    --bpCnt;
	    }

	wdbDbgArchInit();

	wdbEventNodeInit (&eventSysBpNode, wdbSysBpEventGet, NULL, 
					(void *) &eventSysBpNode);

	wdbBpInstalled = TRUE;
	}
    }

/******************************************************************************
*
* wdbBpInstall - install system mode breakpoints.
*
* Before the external agent transfers control back to the OS, this routine
* is called to reinsert the breakpoints.
* 
* RETURNS : N/A
*
* NOMANUAL
*/ 

void wdbBpInstall (void)
    {
    dll_t *	pDll;
#if	DBG_HARDWARE_BP
    DBG_REGS	dbgRegs;
#endif	/* DBG_HARDWARE_BP */

    /* don't install system BP's if we are not in system mode */

    if (wdbIsNowTasking())
	return;

#if	DBG_HARDWARE_BP
    memset (&dbgRegs, 0, sizeof (DBG_REGS));
    wdbDbgRegsClear ();		/* clean debug registers */
#endif	/* DBG_HARDWARE_BP */

    /* if stepping, just set trace mode */

    if (wdbSysBpMode != 0)
	{
#if	DBG_NO_SINGLE_STEP
	wdbSysNpc = wdbDbgGetNpc (&wdbExternSystemRegs);
	wdbSysNpcInstr = *wdbSysNpc;
	usrBreakpointSet (wdbSysNpc, DBG_BREAK_INST);
#endif	/* DBG_NO_SINGLE_STEP */
	wdbBpData = wdbDbgTraceModeSet ((REG_SET *) &wdbExternSystemRegs);
	}
    else	/* if not stepping, insert breakpoints */
	{
	for (pDll = dll_head(&bpList); pDll != dll_end(&bpList);
		pDll = dll_next(pDll))
	    {
	    if (BP_BASE(pDll)->bp_task ==  -1)
		{
		if ((BP_BASE(pDll)->bp_flags & BRK_HARDWARE) == 0)
		    usrBreakpointSet (BP_BASE(pDll)->bp_addr, DBG_BREAK_INST);
#if	DBG_HARDWARE_BP
		else
		    wdbDbgHwBpSet (&dbgRegs, 
				   BP_BASE(pDll)->bp_flags & BRK_HARDMASK, 
				   (UINT32) BP_BASE(pDll)->bp_addr);
#endif	/* DBG_HARDWARE_BP */
		BP_BASE(pDll)->bp_flags |= BP_INSTALLED;
		}
	    }

#if	DBG_HARDWARE_BP
	wdbDbgRegsSet (&dbgRegs);	/* set debug registers. */
#endif	/* DBG_HARDWARE_BP */
	}
    }

/******************************************************************************
*
* wdbSysBpEventGet - upload a breakpoint event to the host.
*
* This routine upload a system breakpoint event to the host.
*
* RETURNS : N/A
*
* NOMANUAL
*/ 

static void wdbSysBpEventGet
    (
    void *	   pRegs,
    WDB_EVT_DATA * pEvtData
    )
    {
    WDB_BP_INFO * pBpInfo = (WDB_BP_INFO *)&pEvtData->eventInfo;

    if ((TGT_ADDR_T) wdbExternSystemRegs.reg_pc != wdbBpAddr)
	{
	/* It should be a watch point */

	pEvtData->evtType	= WDB_EVT_WP;
	pBpInfo->numInts	= 6;
	pBpInfo->addr 		= wdbBpAddr;
	}
    else
	{
	/* It should be a break point */

	pEvtData->evtType   	= WDB_EVT_BP;
	pBpInfo->numInts	= 5;
	}

    /* fill WDB_BP_INFO structure */

    pBpInfo->context.contextType	= WDB_CTX_SYSTEM;
    pBpInfo->context.contextId 		= -1;
    pBpInfo->pc = (TGT_ADDR_T) wdbExternSystemRegs.reg_pc;
    pBpInfo->fp = (TGT_ADDR_T) wdbExternSystemRegs.reg_fp;
    pBpInfo->sp = (TGT_ADDR_T) wdbExternSystemRegs.reg_sp;
    }

/******************************************************************************
*
* wdbSysBpPost - post system mode breakpoint event  
*
* This routine posts an event to the host when a system mode breakpoint
* is hit. It is called when the system is suspended.
*
* RETURNS : N/A
*
* NOMANUAL
*/ 

static void wdbSysBpPost
    (
    int		addr		/* breakpoint addr */
    )
    {
    wdbEventPost (&eventSysBpNode);
    }

/*******************************************************************************
*
* wdbStep - Handle a step request
*
* The wdbStep() function is used to step the system or a task.
*
* NOMANUAL
*/

static UINT32 wdbStep
    (
    WDB_CTX_STEP_DESC * pCtxt
    )
    {
    switch (pCtxt->context.contextType)
	{
	case WDB_CTX_SYSTEM:
	    if (!wdbIsNowExternal())
		return (WDB_ERR_AGENT_MODE);

	    if ((pCtxt->startAddr == 0) && (pCtxt->endAddr == 0))
		wdbSysBpMode = WDB_STEP;
	    else
		{
		wdbSysBpMode	= WDB_STEP_RANGE;
		wdbSysStepStart	= (INSTR *) pCtxt->startAddr;
		wdbSysStepEnd	= (INSTR *) pCtxt->endAddr;
		}

	    wdbOneShot = TRUE;
	    return (WDB_OK);

	case WDB_CTX_TASK:
	    if (!wdbIsNowTasking())
		return (WDB_ERR_AGENT_MODE);

	    if (_wdbTaskStep != NULL)
		return ((*_wdbTaskStep) (pCtxt->context.contextId, 
					 pCtxt->startAddr, 
					 pCtxt->endAddr));

	    return (WDB_ERR_NO_RT_PROC);

	default:
	    return (WDB_ERR_INVALID_CONTEXT);
	}
    }

/*******************************************************************************
*
* wdbCont - Handle a continue request
*
* The wdbCont() function is used to continue the system or a task.
*
* NOMANUAL
*/

static UINT32 wdbCont
    (
    WDB_CTX *	pCtxt
    )
    {
    switch (pCtxt->contextType)
	{
	case WDB_CTX_SYSTEM:
	    if (!wdbIsNowExternal())
		return (WDB_OK);

	    /* 
	     * Check for breakpoint at extern system pc or wdbBpAddr 
	     * (last breakpoint address). This last test is usefull for
	     * data breakpoints.
	     */

	    if ((wdbDbgBpFind 
			((INSTR *) wdbExternSystemRegs.reg_pc, BP_SYS) == OK) ||
	         (wdbDbgBpFind ((INSTR *) wdbBpAddr, BP_SYS) == OK))
		{
	    	wdbSysBpMode = WDB_STEP_OVER;
		}

	    wdbOneShot = TRUE;
	    return (WDB_OK);

	case WDB_CTX_TASK:
	    if (!wdbIsNowTasking())
		return (WDB_ERR_AGENT_MODE);

	    if (_wdbTaskCont != NULL)
		return ((*_wdbTaskCont) (pCtxt->contextId));

	    return (WDB_ERR_NO_RT_PROC);

	default:
	    return (WDB_ERR_INVALID_CONTEXT);
	}
    }

/*******************************************************************************
*
* wdbBpAdd - Handle a break point add request
*
* The wdbBpAdd() function is used to add break points from the agent.
*
* NOMANUAL
*/

static UINT32 wdbBpAdd
    (
    WDB_EVTPT_ADD_DESC *	pBreakPoint,
    UINT32 *			pId
    )
    {
    INSTR	val;
    BRKPT *	pBp;
    INSTR *	addr;

    switch (pBreakPoint->numArgs)
	{
	default:
	case 1:
#if ((CPU_FAMILY == ARM) && ARM_THUMB)
	    addr = (INSTR *) ((UINT32)((pBreakPoint->args[0]) & ~1));
#else /* CPU_FAMILY == ARM */
	    addr = (INSTR *) pBreakPoint->args[0];
#endif /* CPU_FAMILY == ARM */
	    break;
	case 0:
	    return (WDB_ERR_INVALID_PARAMS);
	}

    /* check validity of breakpoint address */

    TEXT_UNLOCK(addr);

    if (((*pWdbRtIf->memProbe) ((char *)addr, VX_READ,
				sizeof(INSTR), (char *)&val) != OK) ||
        ((*pWdbRtIf->memProbe) ((char *)addr, VX_WRITE,
				sizeof(INSTR), (char *)&val) != OK))
	{
	TEXT_LOCK(addr);
        return (WDB_ERR_MEM_ACCES);
	}

    TEXT_LOCK(addr);

    /* check the agent mode */

    switch (pBreakPoint->context.contextType)
	{
	case WDB_CTX_SYSTEM:
	    if (!wdbIsNowExternal())
		return (WDB_ERR_AGENT_MODE);
	    break;
	default:
	    if (!wdbIsNowTasking())
		return (WDB_ERR_AGENT_MODE);
	}



    if (dll_empty (&bpFreeList))
	return (WDB_ERR_EVENTPOINT_TABLE_FULL);

    wdbTaskLock ();	/* disable task switching */
    pBp = BP_BASE(dll_tail (&bpFreeList));
    dll_remove (&pBp->bp_chain);
    wdbTaskUnlock ();	/* re-enable task switching */

    pBp->bp_flags = BP_HOST;
    pBp->bp_addr = addr;
    pBp->bp_action = pBreakPoint->action.actionType;
#if ((CPU_FAMILY == ARM) && ARM_THUMB)
    pBp->bp_callRtn = (void (*)())((UINT32)pBreakPoint->action.callRtn | 1);
#else /* CPU_FAMILY == ARM */
    pBp->bp_callRtn = (void (*)())pBreakPoint->action.callRtn;
#endif /* CPU_FAMILY == ARM */
    pBp->bp_callArg = pBreakPoint->action.callArg;
    pBp->bp_instr = *(INSTR *)addr;

    if (pBreakPoint->numArgs > 1)	/* second argument is count */
   	pBp->bp_count = pBreakPoint->args[1];
    else
   	pBp->bp_count = 0;

    /* XXX - hack because host tools pass wrong info */

    if ((pBp->bp_action == 0) || (pBp->bp_action == WDB_ACTION_STOP))
	pBp->bp_action = WDB_ACTION_STOP | WDB_ACTION_NOTIFY;

    /* set the context ID */

    switch (pBreakPoint->context.contextType)
        {
        case WDB_CTX_SYSTEM:
	    pBp->bp_task = BP_SYS;
            break;
        case WDB_CTX_ANY_TASK:
	    pBp->bp_task = BP_ANY_TASK;
            break;
        case WDB_CTX_TASK:
        default:
	    pBp->bp_task = pBreakPoint->context.contextId;
        }

    wdbTaskLock ();		/* disable task switching */
    dll_insert(&pBp->bp_chain, &bpList);
    wdbTaskUnlock ();		/* re-enable task switching */

    if (pBreakPoint->context.contextType != WDB_CTX_SYSTEM)
	if (_wdbTaskBpAdd != NULL)
	    _wdbTaskBpAdd (pBreakPoint);

    *pId = (UINT32)pBp;
    return (WDB_OK);
    }

#if	DBG_HARDWARE_BP
/*******************************************************************************
*
* wdbHwBpAdd - Handle a hardware break point add request
*
* The wdbBpAdd() function is used to add break points form the agent.
*
* NOMANUAL
*/

static UINT32 wdbHwBpAdd
    (
    WDB_EVTPT_ADD_DESC *	pBreakPoint,	/* breakpoint to add */
    UINT32 *			pId		/* breakpoint ID */
    )
    {
    BRKPT *	pBp;
    dll_t *	pDll;
    int		status;
    DBG_REGS	dbgRegs;		/* debug registers */
    int		contextId;		/* context ID */
    UINT32	addr;			/* breakpoint address */
    UINT32	count = 0;		/* breakpoint count */
    int		type = DEFAULT_HW_BP;	/* hardware type */

    switch (pBreakPoint->numArgs)
	{
	default:
	case 3:
	    type = pBreakPoint->args[2];
	    /* FALL THROUGH */
	case 2:
	    count = pBreakPoint->args[1];
	    /* FALL THROUGH */
	case 1:
	    addr = pBreakPoint->args[0];
	    break;
	case 0:
	    return (WDB_ERR_INVALID_PARAMS);
	}

    /* check validity of hardware breakpoint address */

    if (wdbDbgHwAddrCheck (addr, type, (FUNCPTR) pWdbRtIf->memProbe) != OK)
        return (WDB_ERR_MEM_ACCES);

    /* check the agent mode */

    switch (pBreakPoint->context.contextType)
	{
	case WDB_CTX_SYSTEM:
	    if (!wdbIsNowExternal())
		return (WDB_ERR_AGENT_MODE);
	    break;

	default:
	    if (!wdbIsNowTasking())
		return (WDB_ERR_AGENT_MODE);
	}

    /* set the context ID */

    switch (pBreakPoint->context.contextType)
        {
        case WDB_CTX_SYSTEM:
            contextId = BP_SYS;
            break;
        case WDB_CTX_ANY_TASK:
            contextId = BP_ANY_TASK;
            break;
        case WDB_CTX_TASK:
        default:
            contextId = pBreakPoint->context.contextId;
        }

    /* clean dbgRegs structure */

    memset (&dbgRegs, 0, sizeof (DBG_REGS));

    /* fill dbgRegs structure with all hardware breakpoints */

    wdbTaskLock ();	/* disable task switching */
    for (pDll = dll_head(&bpList); pDll != dll_end(&bpList);
		pDll = dll_next(pDll))
	{
	pBp = BP_BASE(pDll);

	/* check if found breakpoint is applicable to new breakpoint context */

	if (((contextId == BP_SYS) && (pBp->bp_task == BP_SYS)) ||
		((contextId == BP_ANY_TASK) && (pBp->bp_task != BP_SYS)) ||
		((contextId != BP_SYS) && (pBp->bp_task == BP_ANY_TASK)))
	    {
	    if (pBp->bp_flags & BRK_HARDWARE)
		{
		if ((status = wdbDbgHwBpSet (&dbgRegs, 
				pBp->bp_flags & BRK_HARDMASK, 
				(UINT32) pBp->bp_addr)) != OK)
		    {
		    wdbTaskUnlock ();	/* re-enable task switching */
		    return (status);
		    }
		}
	    }
	}
    wdbTaskUnlock ();	/* re-enable task switching */

    if ((status = wdbDbgHwBpSet (&dbgRegs, type, addr)) != OK)
	return (status);

    if (dll_empty (&bpFreeList))
	return (WDB_ERR_EVENTPOINT_TABLE_FULL);

    wdbTaskLock ();		/* disable task switching */
    pBp = BP_BASE(dll_tail (&bpFreeList));
    dll_remove (&pBp->bp_chain);
    wdbTaskUnlock ();	/* re-enable task switching */

    pBp->bp_flags = BP_HOST | type | BRK_HARDWARE;
    pBp->bp_addr = (INSTR *)addr;
    pBp->bp_action = pBreakPoint->action.actionType;
    pBp->bp_count = count;
    pBp->bp_callRtn = (void (*)())pBreakPoint->action.callRtn;
    pBp->bp_callArg = pBreakPoint->action.callArg;
    pBp->bp_task = contextId;

    /* 
     * XXX - MS hack because host tools pass wrong info.
     * XXX - DBT This has been corrected in tornado 2.0 host tools but we
     * must keep this hack for backward compatibility.
     */

    if ((pBp->bp_action == 0) || (pBp->bp_action == WDB_ACTION_STOP))
	pBp->bp_action = WDB_ACTION_STOP | WDB_ACTION_NOTIFY;

    wdbTaskLock ();		/* disable task switching */
    dll_insert(&pBp->bp_chain, &bpList);
    wdbTaskUnlock ();		/* re-enable task switching */

    if (pBreakPoint->context.contextType != WDB_CTX_SYSTEM)
	if (_wdbTaskBpAdd != NULL)
	    _wdbTaskBpAdd (pBreakPoint);

    *pId = (UINT32)pBp;
    return (WDB_OK);
    }
#endif	/* DBG_HARDWARE_BP */

/*******************************************************************************
*
* wdbBpDelete - Handle a break point delete request
*
* The wdbBpDelete() function is used to delete break points.
*
* NOMANUAL
*/

static UINT32 wdbBpDelete
    (
    TGT_ADDR_T *	pId
    )
    {
    dll_t *	pDll;
    dll_t *	pNextDll;
    BRKPT *	pBp;

    pBp = *(BRKPT **) pId;

    /* 
     * Breakpoint ID of -1 means remove all breakpoints. 
     * We can only remove breakpoints set by the host tools. Breakpoints
     * set from target shell can't be removed
     */

    wdbTaskLock ();	/* disable task switching */
    if ((int ) pBp == -1)
	{
	for (pDll = dll_head(&bpList);
	     pDll != dll_end(&bpList);
	     pDll = pNextDll)
	    {
	    pNextDll = dll_next(pDll);

	    if (BP_BASE(pDll)->bp_flags & BP_HOST)
		wdbDbgBpRemove (BP_BASE(pDll));
	    }

	wdbTaskUnlock ();		/* re-enable task switching */
	return (WDB_OK);
	}

    /* else just remove one breakpoint */

    for (pDll = dll_head(&bpList); pDll != dll_end(&bpList);
	    pDll = dll_next(pDll))
	{
	if (BP_BASE(pDll) == pBp)
	    {
	    wdbDbgBpRemove (pBp);
	    wdbTaskUnlock ();	/* re-enable task switching */
	    return (WDB_OK);
	    }
	}
    wdbTaskUnlock ();	/* re-enable task switching */

    return (WDB_ERR_INVALID_EVENTPOINT);
    }

 
#if	DBG_NO_SINGLE_STEP
/*******************************************************************************
*
* wdbTrap - handle hitting of breakpoint
*
* This routine handles the breakpoint trap.  It is called only from its 
* special stub routine (usually in assembly language) which is connected
* to the breakpoint trap. It is used by targets that have to multiplex
* tracing and breakpoint.
*
* NOMANUAL
*/

static void wdbTrap
    (
    int			level,
    INSTR *		addr,
    void *		pInfo,
    WDB_IU_REGS *	pRegisters,
    void *		pDbgRegs,
    BOOL		hardware
    )
    {
    /* check for task level breakpoint trap */

    if (wdbIsNowTasking())
	{
	_wdbTaskBpTrap (level, (INSTR *) pRegisters->reg_pc, pInfo, pRegisters, 
			    pDbgRegs, hardware);
	/* NOTREACHED */
	}

    /* first handle "step mode" (trace emulation) */

    if (wdbSysBpMode != 0)
	{
	wdbTrace (level, pInfo, pRegisters);
	/* NOTREACHED */
	}

    /* must be a system level breakpoint */

    wdbDbgBpRemoveAll ();	/* remove all breakpoints */
    wdbBreakpoint (level, pInfo, pRegisters, pDbgRegs, hardware);
    /* NOTREACHED */
    }
#endif	/* DBG_NO_SINGLE_STEP */

/*******************************************************************************
*
* wdbTrace - handle trace exception
*
* This routine handles the trace trap.  It is called only
* from its special assembly language stub routine which is connected
* to the trace trap.
*
* NOMANUAL
*/
static void wdbTrace
    (
    int			level,
    void *		pInfo,
    WDB_IU_REGS *	pRegisters
    )
    {
#if	!DBG_NO_SINGLE_STEP
    /* check for task level breakpoint trace */

    if (wdbIsNowTasking())
	{
	_wdbTaskBpTrace (level, pInfo, pRegisters);
	/* NOTREACHED */
	}
#else	/* !DBG_NO_SINGLE_STEP */
    usrBreakpointSet (wdbSysNpc, wdbSysNpcInstr);
#endif	/* DBG_NO_SINGLE_STEP */

    /* system mode step */

    wdbDbgTraceModeClear ((REG_SET *) pRegisters, wdbBpData);

    wdbBpAddr = (TGT_ADDR_T) pRegisters->reg_pc;	/* update wdbBpAddr */

    if (wdbSysBpMode == WDB_STEP_OVER)
	{
	wdbSysBpMode = 0;
	wdbBpInstall ();
	WDB_CTX_LOAD (pRegisters);
	/* NOTREACHED */
	}

    if ((wdbSysBpMode == WDB_STEP_RANGE) &&
	   (wdbSysStepStart <= (INSTR *) wdbBpAddr) &&
	       ((INSTR * ) wdbBpAddr < wdbSysStepEnd))
	{
#if	DBG_NO_SINGLE_STEP
	wdbSysNpc = wdbDbgGetNpc (pRegisters);
	wdbSysNpcInstr = *wdbSysNpc;
	usrBreakpointSet (wdbSysNpc, DBG_BREAK_INST);
#endif	/* DBG_NO_SINGLE_STEP */
	wdbBpData = wdbDbgTraceModeSet((REG_SET *) pRegisters);
	WDB_CTX_LOAD (pRegisters);
	/* NOTREACHED */
	}

    /* blindly assume WDB_STEP */

    wdbSysBpMode = 0;
    wdbSuspendSystem (pRegisters, wdbSysBpPost, 0);
    /* NOTREACHED */
    }

/*******************************************************************************
*
* wdbBreakpoint - handle hitting of breakpoint
*
* This routine handles the breakpoint trap.  It is called only from its 
* special stub routine (usually in assembly language) which is connected
* to the breakpoint trap.
*
* NOMANUAL
*/

static void wdbBreakpoint
    (
    int			level,
    void *		pInfo,
    WDB_IU_REGS *	pRegisters,
    void *		pDbgRegs,
    BOOL		hardware
    )
    {
    BRKPT	bpInfo;		/* breakpoint info structure */
    UINT32	type = 0;	/* breakpoint type */
    void 	(*callBack)() = NULL;

#if	!DBG_NO_SINGLE_STEP
    /* check for task level breakpoint trap */

    if (wdbIsNowTasking())
	{
	_wdbTaskBpBreakpoint (level, pInfo, pRegisters, pDbgRegs, hardware);
	/* NOTREACHED */
	}
#endif	/* !DBG_NO_SINGLE_STEP */

    wdbBpAddr = (UINT32) pRegisters->reg_pc;

#if	DBG_HARDWARE_BP
    /* If it is a hardware breakpoint, update type and addr variable. */

    if (hardware)
	wdbDbgHwBpFind (pDbgRegs, &type, &wdbBpAddr);
#endif  /* DBG_HARDWARE_BP */

    /* must be a system level breakpoint */

    wdbDbgBpGet ((INSTR *)wdbBpAddr, BP_SYS, type, &bpInfo);

    /* 
     * On some CPU (eg I960CX), the hardware breakpoint exception is sent only
     * after the instruction was executed. In that case, we consider that
     * the breakpoint address is the address where the processor is stopped,
     * not the real breakpoint address.
     */

#if	DBG_HARDWARE_BP && defined (BRK_INST)
    if (type == (BRK_INST | BRK_HARDWARE))
	wdbBpAddr = (UINT32) pRegisters->reg_pc;
#endif	/* DBG_HARDWARE_BP && defined (BRK_INST) */

    if (bpInfo.bp_action & WDB_ACTION_CALL)
	bpInfo.bp_callRtn (bpInfo.bp_callArg, pRegisters);

    if (bpInfo.bp_action & WDB_ACTION_NOTIFY)
	callBack = wdbSysBpPost;

#if     CPU_FAMILY == MIPS

    /*
     * On MIPS CPUs, when a breakpoint exception occurs in a branch delay slot,
     * the PC has been changed in the breakpoint handler to match with the
     * breakpoint address.
     * Once the matching has been made, the PC is modified to have its normal
     * value (the preceding jump instruction).
     */

    if (pRegisters->cause & CAUSE_BD)   /* Are we in a branch delay slot ? */
        pRegisters->reg_pc--;
#endif  /* CPU_FAMILY == MIPS */

    if (bpInfo.bp_action & (WDB_ACTION_NOTIFY | WDB_ACTION_STOP))
	{	
	if (!(bpInfo.bp_action & WDB_ACTION_STOP))
	    wdbOneShot = TRUE;

	wdbSuspendSystem (pRegisters, callBack, 0);
	/* NOTREACHED */
	}

    /* !WDB_ACTION_STOP means continue (e.g., step over this breakpoint) */

#if	DBG_NO_SINGLE_STEP
    wdbSysNpc = wdbDbgGetNpc (pRegisters);
    wdbSysNpcInstr = *wdbSysNpc;
    usrBreakpointSet (wdbSysNpc, DBG_BREAK_INST);
#endif	/* DBG_NO_SINGLE_STEP */
    wdbSysBpMode = WDB_STEP_OVER;
    wdbBpData = wdbDbgTraceModeSet((REG_SET *) pRegisters);
    WDB_CTX_LOAD(pRegisters);
    }

/******************************************************************************
*
* wdbE - target side code to support the windview "e" command
*/ 

void wdbE
    (
    int *		pData,
    WDB_IU_REGS *	pRegisters
    )
    {
    int (*conditionFunc)() = (int (*)())pData[0];
    int conditionArg	= pData[1];
    int eventNum	= pData[2];

    if ((conditionFunc == NULL) || (conditionFunc (conditionArg) == OK))
        EVT_CTX_BUF (eventNum, pRegisters->reg_pc, 0, NULL);
    }

/******************************************************************************
*
* wdbTaskLock - lock the current task if the agent is running in task mode
*
* This routine locks the current task is the agent is running in 
* task mode.
* 
* RETURNS : N/A
*
* NOMANUAL
*/ 

LOCAL void wdbTaskLock (void)
    {
    if (wdbIsNowTasking() && (pWdbRtIf->taskLock != NULL))
	(*pWdbRtIf->taskLock) ();
    }

/******************************************************************************
*
* wdbTaskUnlock - unlock the current task if the agent is running in task mode
*
* This routine unlocks the current task is the agent is running in 
* task mode.
* 
* RETURNS : N/A
*
* NOMANUAL
*/ 

LOCAL void wdbTaskUnlock (void)
    {
    if (wdbIsNowTasking() && (pWdbRtIf->taskUnlock != NULL))
	(*pWdbRtIf->taskUnlock) ();
    }
