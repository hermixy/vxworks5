/* wdbTaskBpLib.c - Break point handing for the target server */

/* Copyright 1994-2001 Wind River Systems, Inc. */

/*
modification history
--------------------
01o,09nov01,jhw  Revert WDB_INFO to be inside WIND_TCB.
01n,17oct01,jhw  Access WDB_INFO through WIND_TCB pointer.
01m,21apr98,dbt  code cleanup.
01l,06nov97,dbt  merge with target shell debugger.
01k,15jul96,ms   fixed problem with trace mode
01j,26jun96,ms   2nd paramter to eventpoint call func is now a REG_SET *.
		 breakpointIgnore routine no longer checks for VX_UNBREAKABLE.
01i,04jun96,kkk  replace _sigCtxLoad() with WDB_CTX_LOAD().
01h,25apr96,ms   support generalized eventPoint handling
		 support multiple breakpoints at same address
		 lots of cleanup
01f,23jan96,tpr  added casts to compile with Diab Data tools.
01e,26oct95,ms   install switch hook in wdbTaskStep if needed (SPR 4920).
01d,21sep95,ms   fixed SPR 4935 - task BPs work with TEXT_PROTECT now.
01c,07jun95,ms	 no longer use the spare TCB fields
01b,01jun95,ms	 deactivate task BPs when in system mode + some cleanup.
01a,02nov94,rrr  written.
*/

/*
DESCPRIPTION

This library contains breakpoint handling routines when the agent runs
in task mode. Most of those routines are only interfaces for OS specific 
routines (dbgTaskLib.c).
*/

/* includes */

#include "vxWorks.h"
#include "regs.h"
#include "stddef.h"
#include "intLib.h"
#include "wdb/wdb.h"
#include "wdb/dll.h"
#include "wdb/wdbRegs.h"
#include "wdb/wdbBpLib.h"
#include "wdb/wdbLibP.h"
#include "wdb/wdbLib.h"
#include "wdb/wdbRtIfLib.h"
#include "wdb/wdbSvcLib.h"
#include "wdb/wdbEvtLib.h"
#include "wdb/wdbDbgLib.h"

/* defines */

#define WDB_INFO(p)	(&(((WIND_TCB *)(p))->wdbInfo))

#if     CPU_FAMILY==MC680X0
#undef reg_pc
#define reg_pc  regSet.pc
#undef reg_sp
#define reg_sp  regSet.addrReg[7]
#undef reg_fp
#define reg_fp  regSet.addrReg[6]
#endif  /* CPU_FAMILY==MC680X0 */

/* externals */

IMPORT FUNCPTR		_func_dbgHostNotify;

/* locals */

static WDB_EVT_NODE	bpEventNode;
static dll_t		bpEventList;

/* forward static declarations */

static void 	wdbTaskBpPost (UINT32 contextId, WDB_IU_REGS * pRegisters, 
					UINT32 addr);
static void	wdbBreakpointEventGet (void * pTcb, WDB_EVT_DATA * pEvtData);
static void	wdbBreakpointEventDeq (void * arg);
static UINT32	wdbTaskBpAdd (WDB_EVTPT_ADD_DESC * pEv);

/*******************************************************************************
*
* wdbTaskBpLibInit - Initialize the task break point library
*
* NOMANUAL
*
* RETURNS : N/A
*/

void wdbTaskBpLibInit (void)
    {
    _wdbTaskBpAdd	    = wdbTaskBpAdd;
    _wdbTaskStep	    = (UINT32 (*)()) dbgTaskStep;
    _wdbTaskCont	    = (UINT32 (*)()) dbgTaskCont;
#if	DBG_NO_SINGLE_STEP
    _wdbTaskBpTrap	    = (VOIDFUNCPTR) dbgTaskBpTrap;
#else	/* DBG_NO_SINGLE_STEP */
    _wdbTaskBpTrace	    = (VOIDFUNCPTR) dbgTaskBpTrace;
    _wdbTaskBpBreakpoint    = (VOIDFUNCPTR) dbgTaskBpBreakpoint;
#endif	/* DBG_NO_SINGLE_STEP */
    _func_dbgHostNotify	    = (FUNCPTR) wdbTaskBpPost;
  
    /* initialize breakpoint event list */

    dll_init (&bpEventList);
    wdbEventNodeInit (&bpEventNode, wdbBreakpointEventGet,
			    wdbBreakpointEventDeq, (void *) &bpEventNode);
    }

/******************************************************************************
*
* wdbTaskBpPost - post a breakpoint event to the host
* 
* This routine posts an event to the host when a task mode breakpoint
* is hit. It is called when the system is suspended.
* 
* RETURNS : N/A
*
* NOMANUAL
*/ 

static void wdbTaskBpPost
    (
    UINT32		contextId,	/* task ID */
    WDB_IU_REGS *	pRegisters,	/* task registers */
    UINT32		addr		/* breakpoint addr */
    )
    {
    int		level;
    dll_t *	pEvtList;	/* pointer on event list */ 

    level = intLock ();

    WDB_INFO(contextId)->taskPc		= (UINT32) pRegisters->reg_pc;
    WDB_INFO(contextId)->taskFp		= (UINT32) pRegisters->reg_fp; 
    WDB_INFO(contextId)->taskSp		= (UINT32) pRegisters->reg_sp; 
    WDB_INFO(contextId)->taskBpAddr	= addr;

    /* 
     * If the task is already queued in the event list, only overwrite 
     * breakpoint informations for this task. 
     * Otherwise add this breakpoint in the event list and post the event
     * to the host.
     * If the action associated with a specific breakpoint does not include
     * ACTION_STOP, it is possible that we send several breakpoint events
     * for the same task before the target server reads those events. In 
     * that case, we can loose some breakpoint events.
     */

    if (!(WDB_INFO(contextId)->wdbState & WDB_QUEUED))
	{
	pEvtList = (dll_t *)&WDB_INFO(contextId)->wdbEvtList;
	WDB_INFO(contextId)->wdbState |= WDB_QUEUED;
	dll_insert(pEvtList, &bpEventList);
	wdbEventPost (&bpEventNode);
	}

    intUnlock (level);
    }

/******************************************************************************
*
* wdbBreakpointEventGet - fill in the WDB_EVT_DATA for the host.
*
* Earlier, we posted an WDB_EVT_NODE to the event library.
* The event library is now calling this routine (bpEventNode->getEvent)
* in order to get the eventMsg associated with this bpEventNode.
*
* RETURNS : N/A
*
* NOMANUAL
*/

static void wdbBreakpointEventGet
    (
    void *		pNode,
    WDB_EVT_DATA *	pEvtData
    )
    {
    WDB_BP_INFO * 		pBpInfo = (WDB_BP_INFO *)&pEvtData->eventInfo;
    int 			level;
    WIND_TCB *			pTcb;

    level = intLock ();

    if (dll_empty (&bpEventList))
	{
	intUnlock (level);
	pEvtData->evtType = WDB_EVT_NONE;
	return;
	}

    /*
     * The return value of dll_tail is the address of WDB breakpoint
     * Event data structure in WDB_INFO. WDB_INFO is located above WIND_TCB
     * in the tasks memory partition. You can therefore calculate the
     * pTcb by subtracting the offset of wdbEvtList in WDB_INFO and the
     * size of the WIND_TCB structure from the return value of dll_tail.
     */

    pTcb = ((WIND_TCB *)((char *)(dll_tail (&bpEventList)) -
			 OFFSET (WIND_TCB, wdbInfo.wdbEvtList)));

    dll_remove ((dll_t *)&(WDB_INFO(pTcb)->wdbEvtList));

    /* 
     * If the program counter of the task is not the same that the address
     * of the breakpoint we have encountered, it must be a watch point 
     */

    if (WDB_INFO(pTcb)->taskPc != WDB_INFO(pTcb)->taskBpAddr)
	{
	/* assume it is a watch point */

	pEvtData->evtType	= WDB_EVT_WP;
	pBpInfo->numInts	= 6;
	pBpInfo->addr		= WDB_INFO(pTcb)->taskBpAddr;
	}
    else
	{
	/* assume it is a break point */

	pEvtData->evtType	= WDB_EVT_BP;
	pBpInfo->numInts	= 5;
	}

    pBpInfo->context.contextType	= WDB_CTX_TASK;
    pBpInfo->context.contextId		= (int) pTcb;
    pBpInfo->pc				= WDB_INFO(pTcb)->taskPc;
    pBpInfo->fp				= WDB_INFO(pTcb)->taskFp;
    pBpInfo->sp				= WDB_INFO(pTcb)->taskSp;

    WDB_INFO(pTcb)->wdbState &= ~WDB_QUEUED;
    intUnlock (level);
    }

/******************************************************************************
*
* wdbBreakpointEventDeq - dequeue an event node.
*
* The host has acknowledged the receipt of our eventMsg, so the event
* library is now calling this routine to dequeue the bpEventNode.
*
* RETURNS : N/A
*
* NOMANUAL
*/

static void wdbBreakpointEventDeq
    (
    void * pBreakpointNode
    )
    {
    if (!dll_empty(&bpEventList))
	wdbEventPost (&bpEventNode);
    }

/******************************************************************************
*
* wdbTaskBpAdd - initialize tasking breakpoints
*
* This routine initializes tasking breakpoint when the first 
* breakpoint is added.
*
* RETURNS : OK always
*
* NOMANUAL
*/

static UINT32 wdbTaskBpAdd
    (
    WDB_EVTPT_ADD_DESC *	pEv
    )
    {
    dbgTaskBpHooksInstall();
    return (WDB_OK);
    }
