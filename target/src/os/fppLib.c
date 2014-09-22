/* fppLib.c - floating-point coprocessor support library */

/* Copyright 1984-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
03v,16oct01,hdn  made pTcb->pFpContext aligned at cache line boundary for SSE.
03u,21aug01,hdn  added PENTIUM2/3/4 support
03t,14mar99,jdi  doc: removed refs to config.h and/or configAll.h (SPR 25663).
03s,17mar90,jmb  merge Mark Mason patch of 24oct96:  for SIMHPPA, save and
                 restore fpp regs on VX_FP_TASK on both swap in and swap out.
03r,10nov94,dvs  removed reference to fppLastTcb and changed name to 
		 pLastLastFpTcb to deal with merge conflicts between 
		 SPRs 1060 and 3033.
03q,14oct94,ism  merged customer support fix for SPR#1060
03p,28jul94,dvs  moved pLastFpTcb (now called pTaskLastFpTcb) to taskLib and
		 changed include of taskLib.h to private/taskLibP.h. (SPR #3033)
03o,22oct93,jcf  eliminated MC68882 coprocessor violations.
03n,02feb93,jdi  documentation cleanup for 5.1.
03m,13nov92,jcf  removed extraneous logMsg's.
03l,04jul92,jcf  extracted fppShow.
03k,26jun92,jwt  cleaned up fppTaskRegsShow() display format.
03j,05jun92,ajm  added fppDisplayHookRtn routine for mips arch, init hook
		  routines to NULL
03i,26may92,rrr  the tree shuffle
03h,30mar92,yao  made fppRegsShow() in 80 columns per line.
03g,20feb92,yao  added fppTaskRegsShow ().  added global variable
		 pFppTaskIdPrevious. changed copyright notice. documentation.
03f,04oct91,rrr  passed through the ansification filter
		  -changed VOID to void
		  -changed copyright notice
03e,24oct90,jcf  lint.
03d,10aug90,dnw  corrected forward declaration of fppCreateHook().
03c,01aug90,jcf  Changed tcb fpContext to pFpContext.
03b,26jun90,jcf  remove fppHook because taskOptionsSet of f-pt. not allowed.
		 changed fppCreateHook to use taskStackAllot instead of malloc
		 removed fppDeleteHook.
03a,12mar90,jcf  changed fppSwitchHook into fppSwapHook.
		 removed many (FP_CONTEXT *) casts.
		 removed tcb extension dependencies.
02l,07mar90,jdi  documentation cleanup.
02k,09aug89,gae  changed iv68k.h to generic header.
02j,14nov88,dnw  documentation.
02i,29aug88,gae  documentation.
02h,06jul88,jcf  fixed bug in fppTaskRegsSet introduced v02a.
02g,22jun88,dnw  name tweaks.
		 changed to add task switch hook when fppInit() is called,
		   instead of first time fppCreateHook() is called.
02f,30may88,dnw  changed to v4 names.
02e,28may88,dnw  cleaned-up fppProbe.
02d,29apr88,jcf  removed unnecessary intLock () within fppTaskSwitch().
02c,31mar88,gae  fppProbe() now done in install routine fppInit() and
		   hooks only added if true.
02b,18mar88,gae  now supports both MC68881 & MC68882.
02a,25jan88,jcf  make kernel independent.
01e,20feb88,dnw  lint
01d,05nov87,jlf  documentation
01c,22oct87,gae  changed fppInit to use task create/delete facilities.
	   +jcf  made fppExitTask ... use pTcb not pTcbX
01b,28sep87,gae  removed extraneous logMsg's.
01a,06aug87,gae  written/extracted from vxLib.c
*/

/*
DESCRIPTION
This library provides a general interface to the floating-point coprocessor.
To activate floating-point support, fppInit() must be called before any
tasks using the coprocessor are spawned.  This is done automatically by
the root task, usrRoot(), in usrConfig.c when the configuration macro
INCLUDE_HW_FP is defined.

For information about architecture-dependent floating-point routines, see
the manual entry for fppArchLib.

The fppShow() routine displays coprocessor registers on a per-task basis.
For information on this facility, see the manual entries for fppShow and
fppShow().

VX_FP_TASK OPTION
Saving and restoring floating-point registers adds to the context switch
time of a task.  Therefore, floating-point registers are not saved
and restored for every task.  Only those tasks spawned with the task
option VX_FP_TASK will have floating-point registers saved and restored.

.RS 4 4
\%NOTE:  If a task does any floating-point operations,
it must be spawned with VX_FP_TASK.
.RE

INTERRUPT LEVEL
Floating-point registers are not saved and restored for interrupt
service routines connected with intConnect().  However, if necessary,
an interrupt service routine can save and restore floating-point registers
by calling routines in fppArchLib.

INCLUDE FILES: fppLib.h

SEE ALSO: fppArchLib, fppShow, intConnect(),
.pG "Basic OS"
*/

#include "vxWorks.h"
#include "objLib.h"
#include "private/taskLibP.h"
#include "taskArchLib.h"
#include "taskHookLib.h"
#include "memLib.h"
#include "stdio.h"
#include "iv.h"
#include "regs.h"
#include "logLib.h"
#include "fppLib.h"

/* globals */

WIND_TCB *	pFppTaskIdPrevious;	/* Task ID for deferred exceptions */
FUNCPTR		fppCreateHookRtn;	/* arch dependent create hook routine */
FUNCPTR		fppDisplayHookRtn;	/* arch dependent display routine */

/* locals */

#if	(CPU_FAMILY == I80X86)
LOCAL FP_CONTEXT WRS_DATA_ALIGN_BYTES(_CACHE_ALIGN_SIZE) fppDummyContext =
		 {{{0}}};		/* it must be in data section for now */
#else
LOCAL FP_CONTEXT fppDummyContext;
#endif	/* (CPU_FAMILY == I80X86) */

/* forward declarations */

LOCAL void fppCreateHook (WIND_TCB *pTcb);
LOCAL void fppSwapHook (WIND_TCB *pOldTcb, WIND_TCB *pNewTcb);


/*******************************************************************************
*
* fppInit - initialize floating-point coprocessor support
*
* This routine initializes floating-point coprocessor support and must be
* called before using the floating-point coprocessor.  This is done
* automatically by the root task, usrRoot(), in usrConfig.c when the
* configuration macro INCLUDE_HW_FP is defined.
* 
* RETURNS: N/A
*/

void fppInit (void)

    {
    if (fppProbe() == OK)
	{
	taskCreateHookAdd ((FUNCPTR) fppCreateHook);
	taskSwapHookAdd ((FUNCPTR) fppSwapHook);
        fppArchInit ();
	}
    }
/*******************************************************************************
*
* fppCreateHook - initialize floating-point coprocessor support for task
*
* Carves a floating-point coprocessor context from the end of the stack.
*/

LOCAL void fppCreateHook
    (
    FAST WIND_TCB *pTcb		/* newly create task tcb */
    )
    {
    /* check for option bit and presence of floating-point coprocessor */

    if (pTcb->options & VX_FP_TASK)
	{
	/* allocate space for saving context and registers */

#if	(CPU_FAMILY == I80X86)

	/* SSE/SSE2 requires it aligned at 16 byte */

	pTcb->pFpContext = (FP_CONTEXT *)
	    taskStackAllot ((int) pTcb, sizeof (FP_CONTEXT) + _CACHE_ALIGN_SIZE);

	if (pTcb->pFpContext == NULL)
	    return;

	pTcb->pFpContext = (FP_CONTEXT *)
	    (((UINT32)pTcb->pFpContext + (_CACHE_ALIGN_SIZE - 1)) & 
	     ~(_CACHE_ALIGN_SIZE - 1));

#else

	pTcb->pFpContext = (FP_CONTEXT *)
	    taskStackAllot ((int) pTcb, sizeof (FP_CONTEXT));

	if (pTcb->pFpContext == NULL)
	    return;

#endif	/* (CPU_FAMILY == I80X86) */

#if (CPU==SIMHPPA)
	taskSwapHookAttach ((FUNCPTR) fppSwapHook, (int) pTcb, TRUE, TRUE);
#else	/* (CPU==SIMHPPA) */
	taskSwapHookAttach ((FUNCPTR) fppSwapHook, (int) pTcb, TRUE, FALSE);
#endif	/* (CPU==SIMHPPA) */

	taskLock ();
	fppArchTaskCreateInit (pTcb->pFpContext);
	taskUnlock ();

	if (fppCreateHookRtn != NULL)
	    (*fppCreateHookRtn) (pTcb);
	}
    }
/*******************************************************************************
*
* fppSwapHook - swap in task floating-point coprocessor registers
*
* This routine is the task swap hook that implements the task floating-point
* coprocessor registers facility.  It swaps the current and saved values of
* all the task coprocessor registers of the last floating point task and the
* in-coming floating point task.
*/

LOCAL void fppSwapHook
    (
    WIND_TCB *pOldTcb,      /* task tcb switching out */
    FAST WIND_TCB *pNewTcb  /* task tcb switching in */
    )
    {
#if (CPU==SIMHPPA)

    /* save/restore fpp regs on both task swap in and swap out. */
    if (pOldTcb->pFpContext)
	{
	pFppTaskIdPrevious = pOldTcb;
	fppSave (pOldTcb->pFpContext);
	}
    else
	fppSave (&fppDummyContext);     /* to avoid protocol errors */
    if (pNewTcb->pFpContext)
	{
	pTaskLastFpTcb = pNewTcb;
	fppRestore (pNewTcb->pFpContext);
	}

#elif	((CPU == PENTIUM) || (CPU == PENTIUM2) || (CPU == PENTIUM3) || \
	 (CPU == PENTIUM4))

    if (pTaskLastFpTcb == pNewTcb)
	return;

    /* save task coprocessor registers into last floating point task */

    if (pTaskLastFpTcb != NULL)
	{
	pFppTaskIdPrevious = pTaskLastFpTcb;
	(*_func_fppSaveRtn) (pTaskLastFpTcb->pFpContext);
	}
    else
	(*_func_fppSaveRtn) (&fppDummyContext);	/* to avoid protocol errors */

    /* restore task coprocessor registers of incoming task */

    (*_func_fppRestoreRtn) (pNewTcb->pFpContext);

    pTaskLastFpTcb = pNewTcb;

#else	/* (CPU==SIMHPPA) */

    if (pTaskLastFpTcb == pNewTcb)
	return;

    /* save task coprocessor registers into last floating point task */

    if (pTaskLastFpTcb != NULL)
	{
	pFppTaskIdPrevious = pTaskLastFpTcb;
	fppSave (pTaskLastFpTcb->pFpContext);
	}
    else
	fppSave (&fppDummyContext);     /* to avoid protocol errors */

    /* restore task coprocessor registers of incoming task */

    fppRestore (pNewTcb->pFpContext);

    pTaskLastFpTcb = pNewTcb;

#endif /* (CPU==SIMHPPA) */
    }
