/* sigCtxALib.s - software signal architecture support library */

/* Copyright 1984-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01i,30may02,hdn  added locking interrupt in _sigCtxLoad and
		 stopped corrupting current stack (spr 75694)
01h,23aug01,hdn  added FUNC/FUNC_LABEL, replaced .align with .balign
		 replaced sysCodeSelector with sysCsSuper.
01g,06apr98,hdn  fixed a bug in __sigCtxLoad (SPR-20903).
01f,17jun96,hdn  changed CODE_SELECTOR to sysCodeSelector.
01e,16may95,ms   made __sigCtxSave() save the interrupt mask.
01d,24oct94,hdn  deleted cli in __sigCtxLoad.
01c,17oct94,hdn  fixed a bug in __sigCtxLoad.
01b,13sep93,hdn  added adding 4 bytes to sp before it is saved.
01a,15jun93,hdn  written based on mc68k version.
*/

/*
This library provides the architecture specific support needed by
software signals.
*/

#define _ASMLANGUAGE
#include "vxWorks.h"
#include "asm.h"
#include "regs.h"


	.text
	.globl	FUNC(copyright_wind_river)
	.long	FUNC(copyright_wind_river)
	

	/* extetnals */


	/* internals */

	.globl	GTEXT(_sigCtxLoad)
	.globl	GTEXT(_sigCtxSave)
	.globl	GTEXT(sigsetjmp)
	.globl	GTEXT(setjmp)


	.text
	.balign 16

/*******************************************************************************
*
* sigsetjmp - set non-local goto with option to save signal mask
*
* This routine saves the current task context and program counter in <env>
* for later use by siglongjmp().   It returns 0 when called.  However, when
* program execution returns to the point at which sigsetjmp() was called and the
* task context is restored by siglongjmp(), sigsetjmp() will then return the
* value <val>, as specified in siglongjmp().
*
* If the value of <savemask> argument is not zero, the sigsetjmp() function
* shall save the current signal mask of the task as part of the calling
* environment.
*
* RETURNS: 0 or <val> if return is via siglongjmp().
*
* SEE ALSO: longjmp()

* int setjmp
*    (
*    jmp_buf env,       /@ where to save stack environment @/
*    int     savemask	/@ whether or not to save the current signal mask @/
*    )

*/

FUNC_LABEL(sigsetjmp)
	pushl	SP_ARG2(%esp)
	pushl	SP_ARG1+4(%esp)
	call	FUNC(_setjmpSetup)
	addl	$8,%esp
	jmp	FUNC(_sigCtxSave)


/*******************************************************************************
*
* setjmp - set non-local goto
*
* This routine saves the current task context and program counter in <env>
* for later use by longjmp().   It returns 0 when called.  However, when
* program execution returns to the point at which setjmp() was called and the
* task context is restored by longjmp(), setjmp() will then return the value
* <val>, as specified in longjmp().
*
* RETURNS: 0 or <val> if return is via longjmp().
*
* SEE ALSO: longjmp()

* int setjmp
*    (
*    jmp_buf env        /@ where to save stack environment @/
*    )

*/
	.balign 16,0x90
FUNC_LABEL(setjmp)
	pushl	$1
	pushl	8(%esp)
	call	FUNC(_setjmpSetup)
	addl	$8,%esp

	/* FALL THROUGH */

/*******************************************************************************
*
* _sigCtxSave - Save the current context of the current task
*
* This is just like setjmp except it doesn't worry about saving any sigmask.
* It must also always return 0.
*
* RETURNS: 0

* int _sigCtxSave
*     (
*     REG_SET *pRegs		/@ Location to save current context @/
*     )

*/

	.balign 16,0x90
FUNC_LABEL(_sigCtxSave)
	movl	SP_ARG1(%esp),%eax

	movl	(%esp),%edx
	movl	%edx,0x24(%eax)		/* save pc */
	pushfl
	popl	0x20(%eax)		/* save eflags, set IF */

	movl	%edi,0x00(%eax)		/* save all registers */
	movl	%esi,0x04(%eax)
	movl	%ebp,0x08(%eax)
	movl	%ebx,0x10(%eax)
	movl	%edx,0x14(%eax)
	movl	%ecx,0x18(%eax)
	movl	%eax,0x1c(%eax)
	movl	%esp,%edx
	addl	$4,%edx
	movl	%edx,0x0c(%eax)

	xorl	%eax,%eax		/* make return value 0 */
	ret

/*******************************************************************************
*
* _sigCtxLoad - Load a new context in the current task
*
* This is just like longjmp, but every register must be loaded.
* You could also look at this as half a context switch.
*
* Restoring the previous CS(code selector) is required to go back 
* to the int-level context.  This is not supported in this routine.
* Going back to the task-level context is assumed with the task-level
* CS sysCsSuper.  For the WDB's switching back and forth the two 
* contexts - the system (that might be interrupt or task level) and 
* the external agent, _wdbDbgCtxLoad() is provide in wdbDbgALib.s.
*
* RETURNS: Never returns

* void _sigCtxLoad
*     (
*     REG_SET *pRegs		/@ Context to load @/
*     )

*/

	.balign 16,0x90
FUNC_LABEL(_sigCtxLoad)
	movl	SP_ARG1(%esp),%eax

	movl	0x00(%eax),%edi		/* load 5 registers */
	movl	0x04(%eax),%esi
	movl	0x08(%eax),%ebp
	movl	0x10(%eax),%ebx
	movl	0x18(%eax),%ecx

	cli				/* LOCK INTERRUPTS */

	movl	0x0c(%eax),%esp		/* load the stack pointer */

	pushl	0x20(%eax)		/* push EFLAGS */
	pushl	FUNC(sysCsSuper)	/* push the task level CS */
	pushl	0x24(%eax)		/* push PC */

	movl	0x14(%eax),%edx		/* load remaining 2 registers */
	movl	0x1c(%eax),%eax

	iret				/* UNLOCK INTERRUPTS */

