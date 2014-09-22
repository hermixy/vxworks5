/* wdbLib.c - WDB agent context management library */

/* Copyright 1994-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01r,28feb02,jhw  Add entry point to tWdbTask. (SPR 73238).
01q,09feb99,fle  doc : put the code examples between .CS and .CE markups
01p,24nov98,cym  disabling clock during system mode on simnt.
01o,05oct98,jmp  doc: fixed DESCRIPTION section.
01n,27aug98,fle  doc : documented undocumented routines headers
01m,25mar98,dbt  added wdbSystemSuspend() routine.
01l,17dec97,dbt  Set driver in interrupt mode in wdbResumeSystem() routine only
                 if wdb agent can run in task mode (SPR #5630).
01k,02oct96,elp  added casts due to TGT_ADDR_T type change in wdb.h.
01j,09jul96,ms   wdbModeSet to task mode automatically resumes the system.
01i,03jun96,kkk  replace _sigCtxLoad() with WDB_CTX_LOAD() and _sigCtxSave()
                 with WDB_CTX_SAVE().
01h,29aug95,ms   fixed SPRs #4785 and 4499 for external mode agent I/O
01g,21jun95,ms	 added global variable wdbTaskId.
01f,15jun95,ms	 use intRegsLock instead of _sigCtxIntLock
01e,03jun95,ms	 don't notify host if not connected.
01d,25may95,ms	 added fpp reg support for system mode agent.
01c,07feb95,ms	 pass RPC transport handles and commIf's to both agents.
01b,18jan94,rrr  made wdbExternSystemResg global.
01a,06oct94,ms   written.
*/

/*
DESCRIPTION
This library provides a routine to transfer control from the run time 
system to the WDB agent running in external mode. This agent in external
mode allows a system-wide control, including ISR debugging, from a host tool
(eg: Crosswind, WindSh ...) through the target server and the WDB
communcation link.

INTERNAL
This library contains all the routines that manipulate
the WDB agents context, including the agents main command loop.

INCLUDE FILES: wdb/wdbLib.h

SEE ALSO:
.I "API Guide: WTX Protocol",
.tG "Overview"
*/

#include "wdb/wdb.h"
#include "wdb/wdbLib.h"
#include "wdb/wdbLibP.h"
#include "wdb/wdbRpcLib.h"
#include "wdb/wdbSvcLib.h"
#include "wdb/wdbCommIfLib.h"
#include "wdb/wdbRpcLib.h"
#include "wdb/wdbRtIfLib.h"
#include "wdb/wdbArchIfLib.h"
#include "wdb/wdbEvtLib.h"
#include "string.h"

/* Agent variables */

WDB_RT_IF *     pWdbRtIf;		/* interface to runtime system */
static u_int	wdbState;		/* WDB_STATE_EXTERN_RUNNING? */
static u_int	wdbMode;		/* current agent mode. For version 1,
					 * only one agent active at a time. */
static u_int	wdbAvailModes;		/* available agent modes */
static struct timeval wdbTv = {3, 0};	/* 3 second timeout on host NOTIFY */

/* task agent variables */

static WDB_COMM_IF *	pWdbTaskCommIf;		/* communication interface */
static void *		pWdbTaskXport;		/* RPC transport handle */
int			wdbTaskId;		/* agents task ID */

/* system agent variables */

static WDB_IU_REGS	wdbExternAgentRegs;
WDB_IU_REGS		wdbExternSystemRegs;
static void		(*wdbSuspendCallbackRtn)();	/* callback routine */
static int		wdbSuspendCallbackArg;	/* callback argument */
BOOL			wdbOneShot = FALSE;	/* resume system after cmd */
static WDB_COMM_IF *	pWdbExternCommIf;	/* communication interface */
static void *		pWdbExternXport;	/* RPC transport handle */
static dll_t		wdbRegSetList;		/* register set list */

/* forward declarations */

static void   wdbCmdLoop	(void);
static void   wdbCmdOnce	(void);

/******************************************************************************
*
* wdbInstallRtIf - install the runtime interface functions.
*
* The WDB agent is independent of the underlying runtime system or OS.
* This routine installs OS callouts for the agent to use.
*
* NOMANUAL
*/

void wdbInstallRtIf
    (
    WDB_RT_IF *	pRtIf				/* Must be a static structure */
    )
    {
    pWdbRtIf   = pRtIf;
    }

/******************************************************************************
*
* wdbInstallCommIf - install the communication interface.
*
* NOMANUAL
*/ 

void wdbInstallCommIf
    (
    WDB_COMM_IF *	pCommIf,		/* communication functions */
    WDB_XPORT *		pXport			/* RPC xport handle */
    )
    {
    pWdbTaskCommIf	= pCommIf;
    pWdbTaskXport	= pXport;
    pWdbExternCommIf	= pCommIf;
    pWdbExternXport	= pXport;
    }

/******************************************************************************
*
* wdbInfoGet - get info on the WDB agent.
*
* NOMANUAL
*/

void wdbInfoGet
    (
    WDB_AGENT_INFO * pInfo
    )
    {
    pInfo->agentVersion	= WDB_VERSION_STR;
    pInfo->mtu		= wdbCommMtu;
    pInfo->mode		= wdbAvailModes;
    }

/******************************************************************************
*
* wdbTask - WDB task entry point
*
* This is the WDB task entry point. It simply calls wdbCmdLoop() but it has
* been added to have a "real" entry point to WDB task when one issues
* a i() command.
*
* NOMANUAL
*/

void wdbTask (void)
    {
    wdbCmdLoop ();
    }
	     
/******************************************************************************
*
* wdbTaskInit - initialize the task mode agent.
*
* This routine creates a task debug agent by making callouts to
* the runtime system. If the runtime system supports task creation,
* then this routine should succeed.
*
* RETURNS: OK, or ERROR if unable to create the task agent.
*
* NOMANUAL
*/

STATUS wdbTaskInit
    (
    int           wdbTaskPriority,	/* agent task priority */
    int           wdbTaskOptions,	/* agent task options */
    caddr_t       wdbTaskStackBase,	/* agent task stack base (or NULL) */
    int           wdbTaskStackSize	/* agent task stack size */
    )
    {
    WDB_CTX	context;
    int		args[10];

    /* create the agent's task context via callouts to the OS */

    if ((pWdbRtIf->taskCreate == NULL) ||
	(pWdbRtIf->taskResume == NULL) ||
	(pWdbTaskXport == NULL))
	return (ERROR);

    context.contextId = (*pWdbRtIf->taskCreate) ("tWdbTask", wdbTaskPriority,
		wdbTaskOptions, NULL, wdbTaskStackSize, (char *)wdbTask,
		args, 0, 0, 0);

    if (context.contextId == ERROR)
	return (ERROR);

    context.contextType = WDB_CTX_TASK;
    if ((*pWdbRtIf->taskResume)(&context) == ERROR)
	return (ERROR);

    /* record the fact that the agent can run as a task */

    wdbAvailModes |= WDB_MODE_TASK;

    /* store away the task ID in a global variable for all to see */

    wdbTaskId = context.contextId;

    return (OK);
    }

/******************************************************************************
*
* wdbExternInit - initialize the external mode agent.
*
* This routine sets up the external agents context (stack pointer and
* entry address). It does not start the external agents command loop.
*
* RETURNS: OK always.
*
* NOMANUAL
*/

STATUS wdbExternInit
    (
    void *	  stackBase		/* stack to use */
    )
    {
    /* set up the external agents context */

    int pArgs[10];
    pArgs[0] = 1;

    if (pWdbExternXport == NULL)
	return (ERROR);

    _sigCtxSetup (&wdbExternAgentRegs, stackBase, wdbCmdLoop, pArgs);
    intRegsLock (&wdbExternAgentRegs);

    /* initialize the linked list of register objects to save/restore */

    dll_init (&wdbRegSetList);

    /* mark the agent as external */

    wdbAvailModes |= WDB_MODE_EXTERN;

    /* install the external agent's interrupt on first packet hook */

    if ((pWdbExternCommIf->hookAdd != NULL))
	(*pWdbExternCommIf->hookAdd) (pWdbExternCommIf->commId, wdbCmdOnce, 0);

    return (OK);
    }

/******************************************************************************
*
* wdbExternRegSetObjAdd - tell the external agent to manipulate a class of regs.
*
* NOMANUAL
*/ 

void wdbExternRegSetObjAdd
    (
    WDB_REG_SET_OBJ *	pRegSet			/* reg class object */
    )
    {
    dll_insert (&pRegSet->node, &wdbRegSetList);
    }

/******************************************************************************
*
* wdbExternRegsSet -
*
* NOMANUAL
*/ 

STATUS wdbExternRegsSet
    (
    WDB_REG_SET_TYPE	type,
    char *		pRegs
    )
    {
    dll_t *		pThisNode;
    WDB_REG_SET_OBJ *	pRegSet;

    if (type == WDB_REG_SET_IU)
	{
	bcopy (pRegs, (char *)&wdbExternSystemRegs, sizeof (wdbExternSystemRegs));
	return (OK);
	}

    for (pThisNode = dll_head (&wdbRegSetList);
	 pThisNode != dll_end  (&wdbRegSetList);
	 pThisNode  = dll_next (pThisNode))
	{
	pRegSet = (WDB_REG_SET_OBJ *)pThisNode;
	if (pRegSet->regSetType == type)
	    {
	    (*pRegSet->set) (pRegs);
	    return (OK);
	    }
	}

    return (ERROR);
    }

/******************************************************************************
*
* wdbExternRegsGet - gets external registers
*
* NOMANUAL
*/ 

STATUS wdbExternRegsGet
    (
    WDB_REG_SET_TYPE	type,
    char **		ppRegs
    )
    {
    dll_t *		pThisNode;
    WDB_REG_SET_OBJ *	pRegSet;

    if (type == WDB_REG_SET_IU)
	{
	*ppRegs = (char *)&wdbExternSystemRegs;
	return (OK);
	}

    for (pThisNode  = dll_head (&wdbRegSetList);
	 pThisNode != dll_end  (&wdbRegSetList);
	 pThisNode  = dll_next (pThisNode))
	{
	pRegSet = (WDB_REG_SET_OBJ *)pThisNode;
	if (pRegSet->regSetType == type)
	    {
	    (*pRegSet->get) (ppRegs);
	    return (OK);
	    }
	}

    return (ERROR);
    }


/******************************************************************************
*
* wdbModeSet - set the agent mode.
*
* This routine activates one of the agents and deactivates the other.
*
* RETURNS: OK if the requested mode is supported, else ERROR.
*
* NOMANUAL
*/

STATUS	wdbModeSet
    (
    int	newMode			/* agent mode */
    )
    {
    /* check if newMode is OK to set */

    if ((newMode != WDB_MODE_TASK) && (newMode != WDB_MODE_EXTERN))
	return (ERROR);			/* requested mode is invalid */

    if (! (newMode & wdbAvailModes))
	return (ERROR);			/* requested mode is not available */

    /*
     * if both agents are available, deactivate the other agent
     * since both agents use the same communication port, all we
     * need to do is add or remove the external agents interrupt hook.
     */

    if (wdbAvailModes == WDB_MODE_BI)
	{
	if (newMode == WDB_MODE_TASK)
	    {
	    if ((pWdbExternCommIf->hookAdd != NULL))
		(*pWdbExternCommIf->hookAdd) (pWdbExternCommIf->commId,
					      NULL, 0);
	    if (wdbMode == WDB_MODE_EXTERN)
		wdbOneShot = TRUE;
	    }
	else
	    {
	    if ((pWdbExternCommIf->hookAdd != NULL))
		(*pWdbExternCommIf->hookAdd) (pWdbExternCommIf->commId,
					      wdbCmdOnce, 0);
	    }
	}

    wdbMode = newMode;

    return (OK);
    }

/******************************************************************************
*
* wdbRunsExternal - check if the agent runs externally.
*
* NOMANUAL
*/

BOOL  wdbRunsExternal (void)
    {
    return ((wdbAvailModes & WDB_MODE_EXTERN) != 0);
    }

/******************************************************************************
*
* wdbIsNowExternal - Check if the agent is now in external mode.
*
* NOMANUAL
*/

BOOL	wdbIsNowExternal (void)
    {
    return (wdbMode & WDB_MODE_EXTERN);
    }

/******************************************************************************
*
* wdbRunsTasking - check if the agent runs as a task.
*
* NOMANUAL
*/

BOOL  wdbRunsTasking (void)
    {
    return ((wdbAvailModes & WDB_MODE_TASK) != 0);
    }

/******************************************************************************
*
* wdbIsNowTasking - Check if the agent is now in tasking mode.
*
* NOMANUAL
*/

BOOL  wdbIsNowTasking (void)
    {
    return (!wdbIsNowExternal());
    }

/******************************************************************************
*
* wdbCmdLoop - agent command loop.
*
* This is the command loop used by both agents.
*
* NOMANUAL
*/

static void wdbCmdLoop (void)
    {
    WDB_XPORT * pXport;

    /* get the RPC transport handle */

    if (wdbIsNowExternal())
	pXport = pWdbExternXport;
    else
	pXport = pWdbTaskXport;

    /* check if the external agent needs to perform a callback */

    if (wdbIsNowExternal() && (wdbSuspendCallbackRtn != NULL))
	{
	(*wdbSuspendCallbackRtn)(wdbSuspendCallbackArg);
	wdbSuspendCallbackRtn = NULL;
	}

    /* the main command loop */

    while (1)
        {
	/* try to process an RPC command (with timeout) */

	if (!wdbRpcRcv (pXport, (wdbEventListIsEmpty() ? NULL : &wdbTv)))
	    {
	    /* on time out, see if we need to notify the host of events */

	    if (!wdbEventListIsEmpty())
		wdbRpcNotifyHost(pXport);
	    }

	if ((wdbState & WDB_STATE_EXTERN_RUNNING) && wdbOneShot && wdbEventListIsEmpty())
	    wdbResumeSystem();
	}
    }

/******************************************************************************
*
* wdbCmdOnce - Process one external agent command, then return to the system.
*
* This routine is installed as an interrupt on first packet hook to
* notify the external agent.
*
* XXX - should really change the way the hook is done. E.g., examine the
* packet, and if it is for the agent use it, else let the comm layer
* keep it. Right now we assume that any packet must be for the agent,
* and the agent just discards packets that are not really for it.
*
* NOMANUAL
*/

static void wdbCmdOnce (void)
    {
    wdbOneShot = TRUE;
    wdbSuspendSystemHere (NULL, 0);
    }

/******************************************************************************
*
* wdbSuspendSystem - suspend the run-time system.
* 
* This routine transfers control from the runtime system to the extern agent.
* The integer-unit context of the run-time sstem is passed to us (which we
* save). We are also passed a callback to execute after the system has been
* suspended.
* This routine is called by the system breakpoint library.
*
* INTERNAL
* Do not confuse with wdbSystemSuspend() routine. This routine is not part
* of the API and should not be called by the user.
*
* NOMANUAL
*/

void wdbSuspendSystem
    (
    WDB_IU_REGS * pRegs,			/* runtime context to save */
    void	  (*callBack)(),		/* callback after system is stopped */
    int		  arg			/* callback argument */
    )
    {
    dll_t *		pThisNode;
    WDB_REG_SET_OBJ *	pRegSet;

    intLock();

    wdbSuspendCallbackRtn = callBack;		/* install the callback */
    wdbSuspendCallbackArg = arg;		/* and the callback argument */
    wdbState |= WDB_STATE_EXTERN_RUNNING;	/* mark extern agent running */
    wdbExternEnterHook();			/* call extern enter hook */
    (*pWdbExternCommIf->modeSet)		/* reset communication stack */
		(pWdbExternCommIf->commId,
		WDB_COMM_MODE_POLL);
    bcopy ((caddr_t)pRegs,			/* save inferior context */
	   (caddr_t)&wdbExternSystemRegs,
	   sizeof (WDB_IU_REGS));
    for (pThisNode  = dll_head (&wdbRegSetList);
         pThisNode != dll_end  (&wdbRegSetList);
         pThisNode  = dll_next (pThisNode))
        {
        pRegSet = (WDB_REG_SET_OBJ *)pThisNode;
	(*pRegSet->save)();
	}

#if	CPU_FAMILY==MC680X0
    wdbExternAgentRegs.regSet.sr &= 0xefff;
    wdbExternAgentRegs.regSet.sr |= (wdbExternSystemRegs.regSet.sr & 0x1000);
#endif

    WDB_CTX_LOAD (&wdbExternAgentRegs); /* run external agent */

    /*NOTREACHED*/
    }

/******************************************************************************
*
* wdbResumeSystem - resume the runtime system.
*
* This routine transfers control from the external agent back to the runtime
* system.
* This routine is called by the system breakpoint library to continue or step.
*
* NOMANUAL
*/

void wdbResumeSystem (void)
    {
    dll_t *		pThisNode;
    WDB_REG_SET_OBJ *	pRegSet;

    wdbState &= (~WDB_STATE_EXTERN_RUNNING);	/* mark extern agent done */

    /* 
     * Set driver in interrupt mode only if task mode is supported by the
     * agent. Otherwise, it is not usefull.
     */

    if (wdbRunsTasking ())
	{
	(*pWdbExternCommIf->modeSet)		/* reset communication stack */
                (pWdbExternCommIf->commId,
		    WDB_COMM_MODE_INT);
	}

    wdbOneShot = FALSE;				/* ... */
    wdbExternExitHook();			/* call extern exit hook */

    /* restore inferior context */

    for (pThisNode  = dll_head (&wdbRegSetList);
         pThisNode != dll_end  (&wdbRegSetList);
         pThisNode  = dll_next (pThisNode))
        {
        pRegSet = (WDB_REG_SET_OBJ *)pThisNode;
	(*pRegSet->load)();
	}
    WDB_CTX_LOAD (&wdbExternSystemRegs);

    /*NOTREACHED*/
    }

/******************************************************************************
*
* wdbNotifyCallback - callback to notify host of target events
*
* NOMANUAL
*/ 

static void wdbNotifyCallback (void)
    {
    wdbRpcNotifyHost (pWdbExternXport);
    wdbResumeSystem();
    }

/******************************************************************************
*
* wdbNotifyHost - notify the host that an event has occured on the target.
*
* This routine is not called directly by anyone. It is called indirectly
* by wdbEventPost().
*
* NOMANUAL
*/

void wdbNotifyHost (void)
    {
    /* don't notify the host if we are not connected */

    if (!wdbTargetIsConnected())
	return;

    /* if the task agent is currently active, let it do the notify */

    if (wdbIsNowTasking())
	{
	(*pWdbTaskCommIf->cancel)(pWdbTaskCommIf->commId);
	}

    /* if the external agent is running right now, do the notify now */

    else if (wdbState & WDB_STATE_EXTERN_RUNNING)
	{
	wdbRpcNotifyHost(pWdbExternXport);
	}

    /*
     * else the external agent is active, but not currently running.
     * In this case, we suspend the system and have the external agent
     * do the notify as a callback.
     */

     else
	{
	wdbSuspendSystemHere (wdbNotifyCallback, 0);
	}
    }

/******************************************************************************
*
* wdbSuspendSystemHere - suspend the system.
*
* This routine transfers control from the run time system to the external
* agent. This routine is just like wdbSuspendSystem, except we don't pass
* it a register set to save (it uses setjmp to get the register set).
*
* NOMANUAL
*/

void wdbSuspendSystemHere
    (
    void  (*callback)(),
    int   arg
    )
    {
    u_int	lockKey;
    WDB_IU_REGS	regSet;

    lockKey = intLock();

#if CPU==SIMNT
    sysClkDisable();
#endif
    if (WDB_CTX_SAVE (&regSet) == 0)
        {
        _sigCtxRtnValSet (&regSet, 1);
        wdbSuspendSystem (&regSet, callback, arg);
        }
#if CPU==SIMNT
    sysClkEnable();
#endif

    intUnlock (lockKey);
    }

/******************************************************************************
*
* wdbSystemSuspend - suspend the system.
*
* This routine transfers control from the run time system to the WDB 
* agent running in external mode. In order to give back the control to
* the system it must be resumed by the the external WDB agent.
*
* EXAMPLE
*
* The code below, called in a vxWorks application, suspends the system :
* 	
* .CS
*   if (wdbSystemSuspend != OK)
*       printf ("External mode is not supported by the WDB agent.\n");
* .CE
*
* From a host tool, we can detect that the system is suspended.
*
* First, attach to the target server :
*
* .CS
*   wtxtcl> wtxToolAttach EP960CX
*   EP960CX_ps@sevre
* .CE
*
*Then, you can get the agent mode :
*
* .CS
*   wtxtcl> wtxAgentModeGet
*   AGENT_MODE_EXTERN
* .CE
*
* To get the status of the system context, execute :
*
* .CS
*   wtxtcl> wtxContextStatusGet CONTEXT_SYSTEM 0
*   CONTEXT_SUSPENDED
* .CE
*
* In order to resume the system, simply execute :
*
* .CS
*   wtxtcl>  wtxContextResume CONTEXT_SYSTEM 0
*   0
* .CE
*
* You will see that the system is now running :
*
* .CS
*   wtxtcl> wtxContextStatusGet CONTEXT_SYSTEM 0
*   CONTEXT_RUNNING
* .CE
*
* INTERNAL
* This routine is only a user interface for wdbSuspendSystemHere() routine.
* Its name is derived from wdbSuspendSystem but has been modified to
* handle vxWorks API specifications.
*
* RETURNS:
* OK upon successful completion, ERROR if external mode is not supported
* by the WDB agent.
*/

STATUS wdbSystemSuspend (void)
    {
    if (wdbIsNowTasking ())	/* set the agent mode if necessary */
	{
	if (wdbModeSet (WDB_MODE_EXTERN) != OK)
	    return (ERROR);
	}

    wdbSuspendSystemHere (NULL, 0);	/* suspend the system */
    return (OK);
    }
