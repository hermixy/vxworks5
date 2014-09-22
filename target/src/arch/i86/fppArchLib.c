/* fppArchLib.c - floating-point coprocessor support library */

/* Copyright 1984-2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01j,25mar02,hdn  added two APIs to detect illegal FPU usage (spr 70187)
01i,20nov01,hdn  doc clean up for 5.5
01h,21aug01,hdn  imported SSE support from T31 ver 01k
01g,21apr99,hdn  changed selector of the emulator exception handler.
01f,14jun95,hdn  added support for fpp emulation library from USS.
01e,25may95,ms   added fppRegsToCtx and fppCtxToRegs.
01d,10aug93,hdn  added two global variables for fppProbeSup().
01c,03jun93,hdn  updated to 5.1
		  - moved fppInit(), fppCreateHook(), fppSwapHook(),
		    fppTaskRegsShow() to src/all/fppLib.c.  
		  - changed to pointer to floating-point register to 
		    fppTasksRegs{S,G}et().
		  - added arrays fpRegIndex[], fpCtlRegIndex[].
		  - added fppArchTastInit().  
		  - included regs.h, intLib.h.
                  - changed functions to ansi style
		  - changed VOID to void
01b,29sep92,hdn  fixed a bug.
01a,07apr92,hdn  written based on TRON version.
*/

/*
DESCRIPTION
This library provides a low-level interface to the Intel Architecture
Floating-Point Unit (FPU).  Routines fppTaskRegsShow(), fppTaskRegsSet(),
and fppTaskRegsGet() inspect and set coprocessor registers on a per task
basis.  The routine fppProbe() checks for the presence of the
Intel Architecture FPU.  With the exception of fppProbe(), the higher
level facilities in dbgLib and usrLib should be used instead of these
routines.
There are two kind of floating-point contexts and set of routines for 
each kind.  One is 108 bytes for older FPU (i80387, i80487, Pentium) 
and older MMX technology and fppSave(), fppRestore(), fppRegsToCtx(), 
and fppCtxToRegs() are used to save and restore the context, convert to 
or from the FPPREG_SET.
The other is 512 bytes for newer FPU, newer MMX technology and streaming 
SIMD technology (PentiumII, III, 4) and fppXsave(), fppXrestore(), 
fppXregsToCtx(), and fppXctxToRegs() are used to save and restore 
the context, convert to or from the FPPREG_SET.  
Which to use is automatically detected by checking CPUID information in 
fppArchInit().  And fppTaskRegsSet() and fppTaskRegsGet() access the 
appropriate floating-point context.  The bit interrogated for the 
automatic detection is the "Fast Save and Restore" feature flag.

INITIALIZATION
To activate floating-point support, fppInit() must be called before any
tasks using the coprocessor are spawned.  If INCLUDE_FLOATING_POINT is
defined in configAll.h, this is done by the root task, usrRoot(), in
usrConfig.c.

VX_FP_TASK OPTION 
Saving and restoring floating-point registers adds to the context switch
time of a task.
Therefore, floating-point registers are \f2not\fP saved and restored for 
\f2every\fP task.  Only those tasks spawned with the task option VX_FP_TASK
will have floating-point state, MMX technology state, and streaming SIMD 
state saved and restored.  

\f3NOTE:\fP  If a task does any floating-point operations, MMX operations,
and streaming SIMD operation, it must be spawned with VX_FP_TASK.
It is deadly to execute any floating-point operations in a task spawned 
without VX_FP_TASK option, and very difficult to find.  To detect that
illegal/unintentional/accidental floating-point operations, a new API and
mechanism is added.  The mechanism is to enable or disable the FPU by 
toggling the TS flag in the CR0 in the new task switch hook routine -
fppArchSwitchHook() - respecting the VX_FP_TASK option.  If VX_FP_TASK
option is not set in the switching-in task, the FPU is disabled.  Thus
the device-not-available exception will be raised if that task does any 
floating-point operations.  This mechanism is disabled in the default.
To enable, call the enabler - fppArchSwitchHookEnable() - with a 
parameter TRUE(1).  A parameter FALSE(0) disables the mechanism.

MIXING MMX AND FPU INSTRUCTIONS
A task with VX_FP_TASK option saves and restores the FPU and MMX state
when performing a context switch.  Therefore, the application does not
have to save or restore the FPU and MMX state if the FPU and MMX 
instructions are not mixed within a task.  Because the MMX registers
are aliased to the FPU registers, care must be taken when making
transitions between FPU instructions and MMX instructions to prevent
the loss of data in the FPU and MMX registers and to prevent incoherent
or unexpected result.  When mixing MMX and FPU instructions within a 
task, follow these guidelines from Intel:
    - Keep the code in separate modules, procedures, or routines.
    - Do not rely on register contents across transitions between FPU
      and MMX code modules.
    - When transitioning between MMX code and FPU code, save the MMX
      register state (if it will be needed in the future) and execute
      an EMMS instruction to empty the MMX state.
    - When transitioning between FPU and MMX code, save the FPU state,
      if it will be needed in the future.

MIXING SSE/SSE2 AND FPU/MMX INSTRUCTIONS
The XMM registers and the FPU/MMX registers represent separate 
execution environments, which has certain ramifications when executing
SSE, SSE2, MMX and FPU instructions in the same task context:
    - Those SSE and SSE2 instruction that operate only on the XMM 
      registers (such as the packed and scalar floating-point
      instructions and the 128-bit SIMD integer instructions) can be
      executed in the same instruction stream with 64-bit SIMD integer
      or FPU instructions without any restrictions.  For example, an
      application can perform the majority of its floating-point 
      computations in the XMM registers, using the packed and scalar
      floating-point instructions, and at the same time use the FPU
      to perform trigonometric and other transcendental computations.
      Likewise, an application can perform packed 64-bit and 128-bit
      SIMD integer operations can be executed together without
      restrictions.
    - Those SSE and SSE2 instructions that operate on MMX registers
      (such as the CVTPS2PI, CVTTPS2PI, CVTPI2PS, CVTPD2PI, CVTTPD2PI,
      CVTPI2PD, MOVDQ2Q, MOVQ2DQ, PADDQ, and PSUBQ instructions) can
      also be executed in the same instruction stream as 64-bit SIMD
      integer or FPU instructions, however, here they subject to the
      restrictions on the simultaneous use of MMX and FPU instructions,
      which mentioned in the previous paragraph.

INTERRUPT LEVEL
Floating-point registers are \f2not\fP saved and restored for interrupt
service routines connected with intConnect().  However, if necessary,
an interrupt service routine can save and restore floating-point registers
by calling routines in fppALib.  See the manual entry for intConnect() for
more information.

EXCEPTIONS
There are six FPU exceptions that can send an exception to the CPU.  They 
are controlled by Exception Mask bits of the Control Word register.  VxWorks 
disables them in the default configuration.  They
are:
    - Precision
    - Overflow
    - Underflow
    - Division by zero
    - Denormalized operand
    - Invalid Operation
  
SEE ALSO: fppALib, intConnect(), Intel Architecture Software Developer's Manual
*/

#include "vxWorks.h"
#include "objLib.h"
#include "taskLib.h"
#include "taskArchLib.h"
#include "memLib.h"
#include "string.h"
#include "iv.h"
#include "intLib.h"
#include "regs.h"
#include "fppLib.h"
#include "vxLib.h"
#include "taskHookLib.h"


/* externals */

IMPORT CPUID	sysCpuId;
IMPORT int	sysCsExc;
IMPORT void	intVecSet2 (FUNCPTR * vec, FUNCPTR func, int idtGate, \
			    int idtSelector);


/* globals */

REG_INDEX fpRegName [] =	/* FP non-control register name and offset */
    {
    {"st/mm0",	FPREG_FPX(0)},
    {"st/mm1",	FPREG_FPX(1)},
    {"st/mm2",	FPREG_FPX(2)},
    {"st/mm3",	FPREG_FPX(3)},
    {"st/mm4",	FPREG_FPX(4)},
    {"st/mm5",	FPREG_FPX(5)},
    {"st/mm6",	FPREG_FPX(6)},
    {"st/mm7",	FPREG_FPX(7)},
    {NULL, 0},
    };

REG_INDEX fpCtlRegName [] =	/* FP control register name and offset */
    {
    {"fpcr",	FPREG_FPCR},
    {"fpsr",	FPREG_FPSR},
    {"fptag",	FPREG_FPTAG},
    {"op",	FPREG_OP},
    {"ip",	FPREG_IP},
    {"cs",	FPREG_CS},
    {"dp",	FPREG_DP},
    {"ds",	FPREG_DS},
    {NULL, 0},
    };

int fppFsw = 0;
int fppFcw = 0;

VOIDFUNCPTR emu387Func	= NULL;		/* func ptr to trap handler */
VOIDFUNCPTR emuInitFunc	= NULL;		/* func ptr to initializer */
VOIDFUNCPTR _func_fppSaveRtn = NULL;	/* func ptr to fppSave/fppXsave */
VOIDFUNCPTR _func_fppRestoreRtn = NULL; /* func ptr to fppRestore/fppXrestore */


/* locals */

LOCAL FP_CONTEXT * pFppInitContext;	/* pointer to initial FP context */


/*******************************************************************************
*
* fppArchInit - initialize floating-point coprocessor support
*
* This routine must be called before using the floating-point coprocessor.
* It is typically called from fppInit().  
*
* NOMANUAL
*/

void fppArchInit (void)
    {
    
    /* initialize the function pointer and the CR4 OSFXSR bit */

    if (sysCpuId.featuresEdx & CPUID_FXSR)
	{
	_func_fppSaveRtn = fppXsave;		/* new FP save routine */
	_func_fppRestoreRtn = fppXrestore;	/* new FP restore routine */
	vxCr4Set (vxCr4Get () | CR4_OSFXSR);	/* set OSFXSR bit */
	}
    else
	{
	_func_fppSaveRtn = fppSave;		/* old FP save routine */
	_func_fppRestoreRtn = fppRestore;	/* old FP restore routine */
	vxCr4Set (vxCr4Get () & ~CR4_OSFXSR);	/* clear OSFXSR bit */
	}

    fppCreateHookRtn = (FUNCPTR) NULL;

    /* allocate and initialize the initial FP_CONTEXT */

    pFppInitContext = (FP_CONTEXT *) memalign \
				     (_CACHE_ALIGN_SIZE, sizeof (FP_CONTEXT));

    (*_func_fppSaveRtn) (pFppInitContext);
    }

/*******************************************************************************
*
* fppArchTaskCreateInit - initialize floating-point coprocessor support for task
*
* INTERNAL
* It might seem odd that we use a bcopy() to initialize the floating point
* context when a fppSave() of an idle frame would accomplish the same thing
* without the cost of a 108/512 byte data structure.  The problem is that we
* are not guaranteed to have an idle frame.  Consider the following scenario:
* a floating point task is midway through a floating point instruction when
* it is preempted by a *non-floting point* task.  In this case the swap-in
* hook does not save the coprocessor state as an optimization.  Now if we
* create a floating point task the initial coprocessor frame would not be
* idle but rather mid-instruction.  To make matters worse when get around
* to saving the original fp task's floating point context frame, it would be
* incorrectly saved as idle!  One solution would be to fppSave() once to
* the original fp task's context, then fppSave() an idle frame to the new task,
* and finally restore the old fp task's context (in case we return to it
* before another fp task).  The problem with this approach is that it is
* *slow* and considering the fact that preemption is locked, the 108/512 bytes
* don't look so bad anymore.  Indeed, when this approach is adopted by all
* architectures we will not need to lock out preemption anymore.
*
* NOMANUAL
*/

void fppArchTaskCreateInit
    (
    FP_CONTEXT * pFpContext		/* pointer to FP_CONTEXT */
    )
    {
    if (sysCpuId.featuresEdx & CPUID_FXSR)
	{
	bcopyLongs ((char *) pFppInitContext, (char *)pFpContext,
	            (sizeof (FPX_CONTEXT) >> 2));	/* copy 512>>2 longs */
	}
    else
	{
	bcopyLongs ((char *) pFppInitContext, (char *)pFpContext,
	            (sizeof (FPO_CONTEXT) >> 2));	/* copy 108>>2 longs */
	}
    }

/******************************************************************************
*
* fppRegsToCtx - convert FPREG_SET to FPO_CONTEXT.
*/ 

void fppRegsToCtx
    (
    FPREG_SET *  pFpRegSet,             /* input -  fpp reg set */
    FP_CONTEXT * pFpContext             /* output - fpp context */
    )
    {
    int ix;

    for (ix = 0; ix < FP_NUM_REGS; ix++)
        fppDtoDx ((DOUBLEX *)&pFpContext->u.o.fpx[ix], 
                  (double *)&pFpRegSet->fpx[ix]);

    pFpContext->u.o.fpcr  = pFpRegSet->fpcr;
    pFpContext->u.o.fpsr  = pFpRegSet->fpsr;
    pFpContext->u.o.fptag = pFpRegSet->fptag;
    pFpContext->u.o.op    = pFpRegSet->op;
    pFpContext->u.o.ip    = pFpRegSet->ip;
    pFpContext->u.o.cs    = pFpRegSet->cs;
    pFpContext->u.o.dp    = pFpRegSet->dp;
    pFpContext->u.o.ds    = pFpRegSet->ds;
    }

/******************************************************************************
*
* fppCtxToRegs - convert FPO_CONTEXT to FPREG_SET.
*/ 

void fppCtxToRegs
    (
    FP_CONTEXT * pFpContext,            /* input -  fpp context */
    FPREG_SET *  pFpRegSet              /* output - fpp register set */
    )
    {
    int ix;

    for (ix = 0; ix < FP_NUM_REGS; ix++)
        fppDxtoD ((double *)&pFpRegSet->fpx[ix], 
                  (DOUBLEX *)&pFpContext->u.o.fpx[ix]);

    pFpRegSet->fpcr  = pFpContext->u.o.fpcr;
    pFpRegSet->fpsr  = pFpContext->u.o.fpsr;
    pFpRegSet->fptag = pFpContext->u.o.fptag;
    pFpRegSet->op    = pFpContext->u.o.op;
    pFpRegSet->ip    = pFpContext->u.o.ip;
    pFpRegSet->cs    = pFpContext->u.o.cs;
    pFpRegSet->dp    = pFpContext->u.o.dp;
    pFpRegSet->ds    = pFpContext->u.o.ds;
    }

/******************************************************************************
*
* fppXregsToCtx - convert FPREG_SET to FPX_CONTEXT.
*/ 

void fppXregsToCtx
    (
    FPREG_SET *  pFpRegSet,             /* input -  fpp reg set */
    FP_CONTEXT * pFpContext             /* output - fpp context */
    )
    {
    int ix;

    for (ix = 0; ix < FP_NUM_REGS; ix++)
        fppDtoDx ((DOUBLEX *)&pFpContext->u.x.fpx[ix], 
                  (double *)&pFpRegSet->fpx[ix]);

    pFpContext->u.x.fpcr  = pFpRegSet->fpcr;
    pFpContext->u.x.fpsr  = pFpRegSet->fpsr;
    pFpContext->u.x.fptag = pFpRegSet->fptag;
    pFpContext->u.x.op    = pFpRegSet->op;
    pFpContext->u.x.ip    = pFpRegSet->ip;
    pFpContext->u.x.cs    = pFpRegSet->cs;
    pFpContext->u.x.dp    = pFpRegSet->dp;
    pFpContext->u.x.ds    = pFpRegSet->ds;
    }

/******************************************************************************
*
* fppXctxToRegs - convert FPX_CONTEXT to FPREG_SET.
*/ 

void fppXctxToRegs
    (
    FP_CONTEXT * pFpContext,            /* input -  fpp context */
    FPREG_SET *  pFpRegSet              /* output - fpp register set */
    )
    {
    int ix;

    for (ix = 0; ix < FP_NUM_REGS; ix++)
        fppDxtoD ((double *)&pFpRegSet->fpx[ix], 
                  (DOUBLEX *)&pFpContext->u.x.fpx[ix]);

    pFpRegSet->fpcr  = pFpContext->u.x.fpcr;
    pFpRegSet->fpsr  = pFpContext->u.x.fpsr;
    pFpRegSet->fptag = pFpContext->u.x.fptag;
    pFpRegSet->op    = pFpContext->u.x.op;
    pFpRegSet->ip    = pFpContext->u.x.ip;
    pFpRegSet->cs    = pFpContext->u.x.cs;
    pFpRegSet->dp    = pFpContext->u.x.dp;
    pFpRegSet->ds    = pFpContext->u.x.ds;
    }

/*******************************************************************************
*
* fppTaskRegsGet - get the floating-point registers from a task TCB
*
* This routine copies the floating-point registers of a task
* (control, status, and tag) to the locations whose pointers are passed as
* parameters.  The floating-point registers are copied in
* an array containing the 8 registers.
*
* NOTE
* This routine only works well if <task> is not the calling task.  
* If a task tries to get its own registers, the values will be stale 
* (i.e., left over from the last task switch).
*
* RETURNS: OK, or ERROR if there is no floating-point 
* support or there is an invalid state.
*
* SEE ALSO: fppTaskRegsSet()
*/

STATUS fppTaskRegsGet
    (
    int task,			/* task to get info about          */
    FPREG_SET * pFpRegSet	/* pointer to floating-point register set */
    )
    {
    WIND_TCB * pTcb = taskTcb (task);
    FP_CONTEXT * pFpContext;

    if ((pTcb == NULL) ||
        ((pFpContext = pTcb->pFpContext) == (FP_CONTEXT *)NULL))
	return (ERROR);				/* no coprocessor support */

    if (sysCpuId.featuresEdx & CPUID_FXSR)
	fppXctxToRegs (pFpContext, pFpRegSet);
    else
	fppCtxToRegs (pFpContext, pFpRegSet);

    return (OK);
    }

/*******************************************************************************
*
* fppTaskRegsSet - set the floating-point registers of a task 
*
* This routine loads the specified values into the specified task TCB.
* The 8 registers st/mm0 - st/mm7 are copied to the array <fpregs>.
*
* RETURNS: OK, or ERROR if there is no floating-point
* support or there is an invalid state.
*
* SEE ALSO: fppTaskRegsGet()
*/

STATUS fppTaskRegsSet
    (
    int task,			/* task whose registers are to be set */
    FPREG_SET * pFpRegSet	/* pointer to floating-point register set */
    )
    {
    WIND_TCB * pTcb = taskTcb (task);
    FP_CONTEXT * pFpContext;

    if ((pTcb == NULL) ||
        ((pFpContext = pTcb->pFpContext) == (FP_CONTEXT *)NULL))
	return (ERROR);				/* no coprocessor support */

    if (sysCpuId.featuresEdx & CPUID_FXSR)
	fppXregsToCtx (pFpRegSet, pFpContext);
    else
	fppRegsToCtx (pFpRegSet, pFpContext);

    return (OK);
    }

/*******************************************************************************
*
* fppProbe - probe for the presence of a floating-point coprocessor
*
* This routine determines whether there is an Intel Architecture 
* Floating-Point Unit (FPU) in the system.
*
* IMPLEMENTATION
* This routine sets the illegal coprocessor op-code trap vector and executes
* a coprocessor instruction.  If the instruction causes an exception,
* fppProbe() will return ERROR.  Note that this routine saves and restores
* the illegal coprocessor op-code trap vector that was present prior to this
* call.
*
* The probe is only performed the first time this routine is called.
* The result is stored in a static and returned on subsequent
* calls without actually probing.
*
* RETURNS:
* OK if the floating-point coprocessor is present, otherwise ERROR.
*/

STATUS fppProbe (void)
    {
    static int fppProbed = -2;		/* -2 = not done, -1 = ERROR, 0 = OK */

    if (fppProbed == -2)
	{
	fppProbed = fppProbeSup ();	/* check ET bit in %cr0 */
        if (fppProbed == ERROR)
            {
            if ((emuInitFunc != NULL) && (emu387Func != NULL))
                {
                intVecSet2 ((FUNCPTR *)IV_NO_DEVICE, (FUNCPTR)emu387Func,
			    IDT_TRAP_GATE, sysCsExc);
                (* emuInitFunc) ();             /* initialize the emulator */
                fppProbed = OK;                 /* pretend as we have 80387 */
                }
            }
	}

    return (fppProbed);
    }

/*******************************************************************************
*
* fppArchSwitchHook - switch hook routine to set or clear the TS flag in CR0
*
* This routine is a switch hook routine to set or clear the TS flag in CR0.
* The purpose of toggling the TS bit is to detect an illegal FPU/MMX 
* instruction usage without the task option VX_FP_TASK.  The TS flag is set 
* when the new task's VX_FP_TASK bit is set, and cleared otherwise.
* Setting the TS flag generates the device-not-available exception prior 
* to the execution of a FPU/MMX instruction.  Since the MP flag in CR0 is 
* cleared in the default, the execution of WAIT/FWAIT instruction does not 
* raise the exception.  The swap hook routines or switch hook routines that 
* are executed prior to this routine may need to clear the TS bit by itself 
* before using the FPU/MMX instructions.
*
* RETURNS: N/A
*/

void fppArchSwitchHook
    (
    WIND_TCB * pOldTcb,		/* pointer to old task's WIND_TCB */
    WIND_TCB * pNewTcb		/* pointer to new task's WIND_TCB */
    )
    {
    INT32 tempReg; 

    /* enable(clear the TS flag) or disbale(set the TS flag) FPU */

    if (pNewTcb->options & VX_FP_TASK)
	{
	WRS_ASM ("clts");				/* clear TS */
	}
    else
	{
	WRS_ASM ("movl %%cr0,%0; orl $0x8,%0; movl %0,%%cr0;" \
		 : "=r" (tempReg) : : "memory");	/* set TS */
	}
    }

/*******************************************************************************
*
* fppArchSwitchHookEnable - enable or disable the switch hook routine 
*
* This routine is enables or disables the architecture specific FPU switch
* hook routine that detect the illegal FPU/MMX usage.  
* Ths usage of this mechanism is optional, and disabled in the default.
*
* RETURNS: OK, or ERROR if the associated Add/Delete routine failed.
*/

STATUS fppArchSwitchHookEnable
    (
    BOOL enable			/* TRUE to enable, FALSE to disable */
    )
    {
    static INT32 oldCr0 = 0;	/* default: TS flag cleared */
    INT32 tempReg; 

    if (enable)
	{
	if (taskSwitchHookAdd ((FUNCPTR) fppArchSwitchHook) != OK)
	    return (ERROR);

	/* save the TS flag */

        oldCr0 = vxCr0Get ();	

	/* enable(clear the TS flag) or disbale(set the TS flag) FPU */

	if (taskIdCurrent->options & VX_FP_TASK)
	    {
	    WRS_ASM ("clts");				/* clear TS */
	    }
	else
	    {
	    WRS_ASM ("movl %%cr0,%0; orl $0x8,%0; movl %0,%%cr0;" \
		     : "=r" (tempReg) : : "memory");	/* set TS */
	    }
	}
    else
	{
	if (taskSwitchHookDelete ((FUNCPTR) fppArchSwitchHook) != OK)
	    return (ERROR);

	/* restore the TS flag */

	if (oldCr0 & CR0_TS)
	    {
	    WRS_ASM ("movl %%cr0,%0; orl $0x8,%0; movl %0,%%cr0;" \
		     : "=r" (tempReg) : : "memory");	/* set TS */
	    }
	else
	    {
	    WRS_ASM ("clts");				/* clear TS */
	    }
	}

    return (OK);
    }

