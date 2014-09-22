/* fppALib.s - floating-point coprocessor support assembly language routines */

/* Copyright 1984-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01j,25mar02,hdn  added clearing TS bit in fppSave/fppXsave (spr 70187)
01i,20nov01,hdn  doc clean up for 5.5
01h,21aug01,hdn  imported SSE support from T31 ver 01k
01g,04apr98,hdn  added X86FPU_387, X86FPU_487 macros.
01f,21sep95,hdn  added support for NS486
01e,01nov94,hdn  added a check of sysProcessor for Pentium
01d,10aug93,hdn  changed fppProbeSup().
01c,01jun93,hdn  updated to 5.1.
		  - fixed #else and #endif
		  - changed VOID to void
		  - changed ASMLANGUAGE to _ASMLANGUAGE
		  - changed copyright notice
01b,29sep92,hdn  debugged. fixed bugs.
01a,07apr92,hdn  written based on TRON version.
*/

/*
DESCRIPTION
This library contains routines to support the Intel Architecture Floating
Point Unit (FPU).  The routines fppSave() and fppRestore() save and restore
all the floating-point context and MMX technology state, which is 108 bytes
and consists of the 16 extended double precision registers and three control
registers.  
The routines fppXsave() and fppXrestore() save and restore the newer 
floating-point context, newer MMX technology state, and streaming SIMD state
that is 512 bytes, with fxsave and fxrstor instruction.
Higher-level access mechanisms are found in fppLib.

SEE ALSO: fppLib, Intel Architecture Floating-Point Unit User's Manual
*/

#define _ASMLANGUAGE
#include "vxWorks.h"
#include "fppLib.h"
#include "asm.h"
#include "regs.h"
#include "arch/i86/vxI86Lib.h"


	.data
	.globl	FUNC(copyright_wind_river)
	.long	FUNC(copyright_wind_river)
	

	/* externals */

	.globl VAR(fppFsw)
	.globl VAR(fppFcw)
	.globl VAR(sysCoprocessor)


	/* internals */

	.globl GTEXT(fppSave)
	.globl GTEXT(fppRestore)
	.globl GTEXT(fppXsave)
	.globl GTEXT(fppXrestore)
	.globl GTEXT(fppDtoDx)
	.globl GTEXT(fppDxtoD)
	.globl GTEXT(fppProbeSup)


	.text
	.balign 16

/*******************************************************************************
*
* fppSave - save the floating-pointing unit context with fnsave
*
* This routine saves the floating-point unit context and MMX technology state
* with fnsave.
* The context saved is 108 bytes and contains:
*
*       - control word			4
*	- status word			4
*	- tag word			4
*	- instruction pointer		4
*	- instruction pointer selector	2
*	- last FP instruction op code	2
*	- data pointer			4
*	- data pointer selector		4
*	- FR/MM[0-7]			8 * 10
*
* RETURNS: N/A
*
* SEE ALSO: fppRestore(), Intel Architecture Floating-Point Unit 
* User's Manual

* void fppSave (pFpContext)
*     FP_CONTEXT * pFpContext;	/* where to save context *

*/

FUNC_LABEL(fppSave)
	movl	SP_ARG1(%esp), %eax	/* where to save registers */
	clts				/* clear the TS bit in CR0 */
	fnsave	(%eax)
	ret

/*******************************************************************************
*
* fppRestore - restore the floating-point unit context with frstor
*
* This routine restores the floating-point unit context and MMX technology
* state with frstor.
* The context restored is:
*
*       - control word			4
*	- status word			4
*	- tag word			4
*	- instruction pointer		4
*	- instruction pointer selector	2
*	- last FP instruction op code	2
*	- data pointer			4
*	- data pointer selector		4
*	- FR/MM[0-7]			8 * 10
*
* RETURNS: N/A
*
* SEE ALSO: fppSave(), Intel Architecture Floating-Point Unit User's Manual

* void fppRestore (pFpContext)
*    FP_CONTEXT * pFpContext;	/* from where to restore context *

*/

	.balign 16,0x90
FUNC_LABEL(fppRestore)
	movl	SP_ARG1(%esp), %eax	/* from where to restore registers */
	frstor	(%eax)
	ret

/*******************************************************************************
*
* fppXsave - save the floating-pointing unit context with fxsave
*
* This routine saves the floating-point unit context, newer MMX technology
* state, and streaming SIMD state with fxsave.
* The context saved is 512 bytes and contains:
*
*       - control word			2
*	- status word			2
*	- tag word			2
*	- last FP instruction op code	2
*	- instruction pointer 		4
*	- instruction pointer selector	4
*	- data pointer			4
*	- data pointer selector		4
*	- FR/MM[0-7]			8 * 16
*	- XMM[0-7]			8 * 16
*
* RETURNS: N/A
*
* SEE ALSO: fppXrestore(), Intel Architecture Software Developer's Manual 

* void fppXsave (pFpContext)
*     FP_CONTEXT * pFpContext;	/* where to save context *

*/

	.balign 16,0x90
FUNC_LABEL(fppXsave)
	movl	SP_ARG1(%esp), %eax	/* where to save registers */
	clts				/* clear the TS bit in CR0 */
	fxsave	(%eax)
	ret

/*******************************************************************************
*
* fppXrestore - restore the floating-point unit context with fxrstor
*
* This routine restores the floating-point unit context, newer MMX technology
* state, and streaming SIMD state with fxrstor.
* The context restored is:
*
*       - control word			2
*	- status word			2
*	- tag word			2
*	- last FP instruction op code	2
*	- instruction pointer 		4
*	- instruction pointer selector	4
*	- data pointer			4
*	- data pointer selector		4
*	- FR/MM[0-7]			8 * 16
*	- XMM[0-7]			8 * 16
*
* RETURNS: N/A
*
* SEE ALSO: fppXsave(), Intel Architecture Software Developer's Manual 

* void fppXrestore (pFpContext)
*    FP_CONTEXT * pFpContext;	/* from where to restore context *

*/

	.balign 16,0x90
FUNC_LABEL(fppXrestore)
	movl	SP_ARG1(%esp), %eax	/* from where to restore registers */
	fxrstor	(%eax)
	ret

/*******************************************************************************
*
* fppDtoDx - convert double to extended double precision
*
* The FPU uses a special extended double precision format
* (10 bytes as opposed to 8 bytes) for internal operations.
* The routines fppSave and fppRestore must preserve this precision.
*
* NOMANUAL

* void fppDtoDx (pDx, pDouble)
*     DOUBLEX *pDx;	 /* where to save result    *
*     double *pDouble;	 /* ptr to value to convert *

*/

	.balign 16,0x90
FUNC_LABEL(fppDtoDx)
	movl	SP_ARG1(%esp), %edx	/* to Dx */
	movl	SP_ARG2(%esp), %eax	/* from D */

	subl	$16, %esp
	fstpt	(%esp)			/* save %st */
	fldl	(%eax)
	fstpt	(%edx)
	fldt	(%esp)			/* restore %st */
	addl	$16, %esp

	ret

/*******************************************************************************
*
* fppDxtoD - convert extended double precisoion to double
*
* The FPU uses a special extended double precision format
* (10 bytes as opposed to 8 bytes) for internal operations.
* The routines fppSave and fppRestore must preserve this precision.
*
* NOMANUAL

* void fppDxtoD (pDouble, pDx)
*     double *pDouble;		/* where to save result    *
*     DOUBLEX *pDx;		/* ptr to value to convert *

*/

	.balign 16,0x90
FUNC_LABEL(fppDxtoD)
	movl	SP_ARG1(%esp), %edx	/* to D */
	movl	SP_ARG2(%esp), %eax	/* from Dx */

	subl	$16, %esp
	fstpt	(%esp)			/* save %st */
	fldt	(%eax)
	fstpl	(%edx)
	fldt	(%esp)			/* restore %st */
	addl	$16, %esp

	ret

/*******************************************************************************
*
* fppProbeSup - fppProbe support routine
*
* This routine executes some floating-point unit instruction which will cause a
* bus error if a floating-point unit is not present.  A handler, viz. 
* fppProbeTrap, should be installed at that vector.  If the floating-point
* unit is present this routine returns OK.
*
* SEE ALSO: Intel Architecture Floating-Point Unit User's Manual
*
* NOMANUAL

* STATUS fppProbeSup ()
*/

	.balign 16,0x90
FUNC_LABEL(fppProbeSup)
	cmpl    $ X86CPU_386,FUNC(sysProcessor)	/* is it 386 ? */
	jne	fppProbe487

	/* does it have 387 ? */

	fninit
	movl	$FUNC(fppFsw),%edx
	movl	$FUNC(fppFcw),%ecx
	fnstsw	(%edx)
	cmpb	$0,(%edx)
	jne	fppProbeNo387
	fnstcw	(%ecx)
	movw	(%ecx),%ax
	cmpw	$0x37f,%ax
	jne	fppProbeNo387
	movl	%cr0,%eax
	andl	$0xfffffff9,%eax
	orl	$0x00000002,%eax
	movl	%eax,%cr0			/* EM=0, MP=1 */
	movl    $ X86FPU_387,FUNC(sysCoprocessor)	/* it has 80387 */
	xorl	%eax,%eax			/* set status to OK */
	jmp	fppProbeDone

fppProbeNo387:
	movl	%cr0,%eax
	andl	$0xfffffff9,%eax
	orl	$0x00000004,%eax 		/* EM=1, MP=0 */
	movl	%eax,%cr0
	movl	$ ERROR,%eax			/* set status to ERROR */
	jmp	fppProbeDone


fppProbe487:
	cmpl    $ X86CPU_NS486,FUNC(sysProcessor)	/* is it NS486 ? */
	je	fppProbeNo487

	/* does it have 487 ? */

	fninit
	movl	$FUNC(fppFcw),%edx
	fnstcw	(%edx)
	movw	(%edx),%ax
	cmpw	$0x37f,%ax
	jne	fppProbeNo487
	movl	%cr0,%eax
	andl	$0xffffffd9,%eax
	orl	$0x00000002,%eax
	movl	%eax,%cr0			/* NE=0, EM=0, MP=1 */
	movl    $ X86FPU_487,FUNC(sysCoprocessor)	/* it has 80487 */
	xorl	%eax,%eax			/* set status to OK */
	jmp	fppProbeDone

fppProbeNo487:
	movl	%cr0,%eax
	andl	$0xffffffd9,%eax
	orl	$0x00000004,%eax
	movl	%eax,%cr0			/* NE=0, EM=1, MP=0 */
	movl	$ ERROR,%eax			/* set status to ERROR */
	jmp	fppProbeDone

fppProbeNS486:
	movl	$ ERROR,%eax			/* set status to ERROR */

fppProbeDone:
	ret

