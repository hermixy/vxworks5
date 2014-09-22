/* fppArchLib.c - architecture-dependent floating-point coprocessor support */

/* Copyright 1984-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01j,05jun02,wsl  remove reference to SPARC and i960
01i,28mar02,hdn  updated x86 specific sections
01h,20nov01,hdn  updated x86 specific sections
01g,13nov01,hbh  Updated for simulators.
01f,29oct01,zl   added SH-specific documentation
01e,27feb97,jpd  added ARM-specific documentation.
01d,25nov95,jdi  removed 29k stuff.
01c,31jan95,rhp  update for MIPS R4000, AM29K, i386/i486.
            jdi  changed 80960 to i960.
01b,20jan93,jdi  documentation cleanup.
01a,23sep92,jdi	 written, based on fppALib.s and fppArchLib.c for
		 mc68k, sparc, i960, mips.
*/

/*
DESCRIPTION
This library contains architecture-dependent routines to support the 
floating-point coprocessor.  The routines fppSave() and fppRestore() save
and restore all the task floating-point context information.  The routine
fppProbe() checks for the presence of the floating-point coprocessor.  The
routines fppTaskRegsSet() and fppTaskRegsGet() inspect and set coprocessor
registers on a per-task basis.

With the exception of fppProbe(), the higher-level facilities in dbgLib
and usrLib should be used instead of these routines.  For information about
architecture-independent access mechanisms, see the manual entry for fppLib.

INITIALIZATION
To activate floating-point support, fppInit() must be called before any
tasks using the coprocessor are spawned.  This is done by the root task,
usrRoot(), in usrConfig.c.  See the manual entry for fppLib.

NOTE X86:
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

NOTE X86 INITIALIZATION:
To activate floating-point support, fppInit() must be called before any
tasks using the coprocessor are spawned.  If INCLUDE_FLOATING_POINT is
defined in configAll.h, this is done by the root task, usrRoot(), in
usrConfig.c.

NOTE X86 VX FP TASK OPTION: 
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

NOTE X86 MIXING MMX AND FPU INSTRUCTIONS:
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

NOTE X86 MIXING SSE SSE2 FPU AND MMX INSTRUCTIONS:
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

NOTE X86 INTERRUPT LEVEL:
Floating-point registers are \f2not\fP saved and restored for interrupt
service routines connected with intConnect().  However, if necessary,
an interrupt service routine can save and restore floating-point registers
by calling routines in fppALib.  See the manual entry for intConnect() for
more information.

NOTE X86 EXCEPTIONS:
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

NOTE ARM
This architecture does not currently support floating-point coprocessors.


INCLUDE FILES: fppLib.h

SEE ALSO: fppLib, intConnect(),
.nf
.IR "Motorola MC68881/882 Floating-Point Coprocessor User's Manual" ,
.IR "Intel 387 DX User's Manual" ,
.IR "Intel Architecture Software Developer's Manual" ,
.IR "Hitachi SH7750 Hardware Manual" ,
.fi
Gerry Kane and Joe Heinrich:
.I "MIPS RISC Architecture Manual"
*/


/*******************************************************************************
*
* fppSave - save the floating-point coprocessor context
*
* This routine saves the floating-point coprocessor context.
* The context saved is:
*
* \&`MC680x0':
*	- registers `fpcr', `fpsr', and `fpiar'
*	- registers `f0' - `f7'
*	- internal state frame (if NULL, the other registers are not saved.)
*
* \"\&`SPARC':
* \"	- registers `fsr' and `fpq'
* \"	- registers `f0' - `f31'
* \"
* \"\&`i960':
* \"	- registers `fp0' - `fp3'
* \"
* \&`MIPS':
*	- register `fpcsr'
*	- registers `fp0' - `fp31'
*
* \&`SH-4':
*	- registers `fpcsr' and `fpul'
*	- registers `fr0' - `fr15'
*	- registers `xf0' - `xf15'
*
* \&`x86':
*       108 byte old context with fsave and frstor instruction
*       - control word, status word, tag word, 
*       - instruction pointer,
*       - instruction pointer selector,
*       - last FP instruction op code,
*       - data pointer,
*       - data pointer selector,
*       - registers `st/mm0' - `st/mm7' (10 bytes * 8)
*       512 byte new context with fxsave and fxrstor instruction
*       - control word, status word, tag word, 
*       - last FP instruction op code,
*       - instruction pointer,
*       - instruction pointer selector,
*       - data pointer,
*       - data pointer selector,
*       - registers `st/mm0' - `st/mm7' (10 bytes * 8)
*       - registers `xmm0' - `xmm7' (16 bytes * 8)
*
* \&`ARM':
*       - currently, on this architecture, this routine does nothing.
*
* \&`SimSolaris':
*	- register `fsr'
*	- registers `f0' - `f31'
*
* \&`SimNT':
*	- this routine does nothing on Windows simulator. Floating point
*	  registers are saved by Windows.
*
* RETURNS: N/A
*
* SEE ALSO: fppRestore()
*/

void fppSave
    (
    FP_CONTEXT *  pFpContext	/* where to save context */
    )

    {
    ...
    }



/*******************************************************************************
*
* fppRestore - restore the floating-point coprocessor context
*
* This routine restores the floating-point coprocessor context.
* The context restored is:
*
* \&`MC680x0':
*	- registers `fpcr', `fpsr', and `fpiar'
*	- registers `f0' - `f7'
*	- internal state frame (if NULL, the other registers are not saved.)
*
* \"\&`SPARC':
* \"	- registers `fsr' and `fpq'
* \"	- registers `f0' - `f31'
* \"
* \"\&`i960':
* \"	- registers `fp0' - `fp3'
* \"
* \&`MIPS':
*	- register `fpcsr'
*	- registers `fp0' - `fp31'
*
* \&`SH-4':
*	- registers `fpcsr' and `fpul'
*	- registers `fr0' - `fr15'
*	- registers `xf0' - `xf15'
*
* \&`x86':
*       108 byte old context with fsave and frstor instruction
*       - control word, status word, tag word, 
*       - instruction pointer,
*       - instruction pointer selector,
*       - last FP instruction op code,
*       - data pointer,
*       - data pointer selector,
*       - registers `st/mm0' - `st/mm7' (10 bytes * 8)
*       512 byte new context with fxsave and fxrstor instruction
*       - control word, status word, tag word, 
*       - last FP instruction op code,
*       - instruction pointer,
*       - instruction pointer selector,
*       - data pointer,
*       - data pointer selector,
*       - registers `st/mm0' - `st/mm7' (10 bytes * 8)
*       - registers `xmm0' - `xmm7' (16 bytes * 8)
*
* \&`ARM':
*       - currently, on this architecture, this routine does nothing.
*
* \&`SimSolaris':
*	- register `fsr'
*	- registers `f0' - `f31'
*
* \&`SimNT':
*	- this routine does nothing on Windows simulator. 
*
* RETURNS: N/A
*
* SEE ALSO: fppSave()
*/

void fppRestore
    (
    FP_CONTEXT *  pFpContext	/* where to restore context from */
    )

    {
    ...
    }

/*******************************************************************************
*
* fppProbe - probe for the presence of a floating-point coprocessor
*
* This routine determines whether there is a
* floating-point coprocessor in the system.
*
* The implementation of this routine is architecture-dependent:
*
* .IP "`MC680x0', `x86', `SH-4':"
* This routine sets the illegal coprocessor opcode trap vector and executes
* a coprocessor instruction.  If the instruction causes an exception,
* fppProbe() returns ERROR.  Note that this routine saves and restores
* the illegal coprocessor opcode trap vector that was there prior to this
* call.
*
* The probe is only performed the first time this routine is called.
* The result is stored in a static and returned on subsequent
* calls without actually probing.
*
* \".IP `i960':
* \"This routine merely indicates whether VxWorks was compiled with
* \"the flag `-DCPU=I960KB'.
* \"
* .IP `MIPS':
* This routine simply reads the R-Series status register and reports
* the bit that indicates whether coprocessor 1 is usable.  This bit
* must be correctly initialized in the BSP.
*
* .IP `ARM':
* This routine currently returns ERROR to indicate no floating-point 
* coprocessor support.
* 
* .IP "`SimNT', `SimSolaris':"
* This routine currently returns OK.
*
* INTERNAL
* The STAR board will always have an on-board floating-point unit.
* What about others?
*
* RETURNS:
* OK, or ERROR if there is no floating-point coprocessor.
*/

STATUS fppProbe (void)

    {
    ...
    }

/*******************************************************************************
*
* fppTaskRegsGet - get the floating-point registers from a task TCB
*
* This routine copies a task's floating-point registers and/or status
* registers to the locations whose pointers are passed as
* parameters.  The floating-point registers are copied into
* an array containing all the registers.
*
* NOTE
* This routine only works well if <task> is not the calling task.
* If a task tries to discover its own registers, the values will be stale
* (that is, left over from the last task switch).
*
* RETURNS: OK, or ERROR if there is no floating-point
* support or there is an invalid state.
*
* SEE ALSO: fppTaskRegsSet()
*/

STATUS fppTaskRegsGet
    (
    int		 task,       	/* task to get info about */
    FPREG_SET *  pFpRegSet	/* ptr to floating-point register set */
    )

    {
    ...
    }

/*******************************************************************************
*
* fppTaskRegsSet - set the floating-point registers of a task
*
* This routine loads the specified values into the TCB of a specified task.
* The register values are copied from the array at <pFpRegSet>.
*
* RETURNS: OK, or ERROR if there is no floating-point
* support or there is an invalid state.
*
* SEE ALSO: fppTaskRegsGet()
*/

STATUS fppTaskRegsSet
    (
    int		 task,          /* task to set registers for */
    FPREG_SET *	 pFpRegSet	/* ptr to floating-point register set */
    )

    {
    ...
    }
