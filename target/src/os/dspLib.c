/* dspLib.c - dsp support library */

/* Copyright 1998-2000 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01c,14sep00,hk   changed dspInit() to call dspArchInit() always for SH.
01b,12mar99,jmb  Moved code causing scalabilty problems to target/src/config.
01a,22jul98,mem  written.
*/

/*
DESCRIPTION
This library provides a general interface to the dsp.
To activate dsp support, dspInit() must be called before any
tasks using the dsp are spawned.  This is done automatically by
the root task, usrRoot(), in usrConfig.c when INCLUDE_DSP is defined in
configAll.h.

For information about architecture-dependent dsp routines, see
the manual entry for dspArchLib.

VX_DSP_TASK OPTION
Saving and restoring dsp registers adds to the context switch
time of a task.  Therefore, dsp registers are not saved
and restored for every task.  Only those tasks spawned with the task
option VX_DSP_TASK will have dsp registers saved and restored.

.RS 4 4
\%NOTE:  If a task does any dsp operations,
it must be spawned with VX_DSP_TASK.
.RE

INTERRUPT LEVEL
DSP registers are not saved and restored for interrupt
service routines connected with intConnect().  However, if necessary,
an interrupt service routine can save and restore dsp registers
by calling routines in dspArchLib.

INCLUDE FILES: dspLib.h

SEE ALSO: dspArchLib, dspShow, intConnect(),
.pG "Basic OS"
*/

#include "vxWorks.h"
#include "objLib.h"
#include "private/taskLibP.h"
#include "taskArchLib.h"
#include "taskHookLib.h"
#include "memLib.h"
#include "iv.h"
#include "regs.h"
#include "logLib.h"
#include "dspLib.h"

/* externals */

/* globals */

FUNCPTR	dspCreateHookRtn = NULL;  /* arch dependent create hook routine */
FUNCPTR	dspDisplayHookRtn = NULL; /* arch dependent display routine */

/* forward declarations */

LOCAL void dspCreateHook (WIND_TCB *pTcb);
LOCAL void dspSwapHook (WIND_TCB *pOldTcb, WIND_TCB *pNewTcb);


/******************************************************************************
*
* dspInit - initialize dsp support
*
* This routine initializes dsp support and must be
* called before using the dsp.  This is done
* automatically by the root task, usrRoot(), in usrConfig.c when INCLUDE_DSP
* is defined in configAll.h.
* 
* RETURNS: N/A
*/

void dspInit (void)
    {
#if (CPU_FAMILY == SH)
    dspArchInit ();	/* SH7729 (SH3-DSP) specific initialization */
#endif

    if (dspProbe() == OK)
	{
	taskCreateHookAdd ((FUNCPTR) dspCreateHook);
	taskSwapHookAdd ((FUNCPTR) dspSwapHook);
#if (CPU_FAMILY != SH)
        dspArchInit ();
#endif
	}
    }

/******************************************************************************
*
* dspCreateHook - initialize dsp support for task
*
* Carves a dsp context from the end of the stack.
*
* RETURNS: N/A
*
* NOMANUAL
*/

LOCAL void dspCreateHook
    (
    WIND_TCB *pTcb		/* newly create task tcb */
    )
    {
    /* check for option bit and presence of dsp support */

    if (pTcb->options & VX_DSP_TASK)
	{
	/* allocate space for saving context and registers */

	pTcb->pDspContext = (DSP_CONTEXT *)
	  taskStackAllot ((int) pTcb, sizeof (DSP_CONTEXT));

	if (pTcb->pDspContext == NULL)
	    return;

	taskSwapHookAttach ((FUNCPTR) dspSwapHook, (int) pTcb, TRUE, FALSE);

	taskLock ();
	dspArchTaskCreateInit (pTcb->pDspContext);
	taskUnlock ();

	if (dspCreateHookRtn != NULL)
	    (*dspCreateHookRtn) (pTcb);
	}
    }

/******************************************************************************
*
* dspSwapHook - swap in task dsp registers
*
* This routine is the task swap hook that implements the task dsp
* coprocessor registers facility.  It swaps the current and saved values of
* all the task coprocessor registers of the last dsp task and the
* in-coming dsp task.
*
* RETURNS: N/A
*
* NOMANUAL
*/

LOCAL void dspSwapHook
    (
    WIND_TCB *pOldTcb,		/* task tcb switching out */
    WIND_TCB *pNewTcb		/* task tcb switching in */
    )
    {
    if (pTaskLastDspTcb == pNewTcb)
	return;

    /* save task coprocessor registers into last dsp task */

    if (pTaskLastDspTcb != NULL) 
	dspSave (pTaskLastDspTcb->pDspContext);

    /* restore task coprocessor registers of incoming task */

    dspRestore (pNewTcb->pDspContext);

    pTaskLastDspTcb = pNewTcb;
    }
