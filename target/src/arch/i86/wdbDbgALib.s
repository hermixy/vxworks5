/* wdbDbgALib.s - i80x86 debugging aids assembly interface */

/* Copyright 1984-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01f,30may02,hdn  added WDB_CTX_LOAD that is _wdbDbgCtxLoad() (spr 75694)
01e,23aug01,hdn  added FUNC/FUNC_LABEL, replaced .align w .balign
		 fixed stack adjustment bug in wdbDbgBpStub.
01d,08jan98,dbt  modified for new breakpoint scheme. Added hardware
		 breakpoints support
01c,01jun93,hdn  updated to 5.1.
		  - fixed #else and #endif
		  - changed VOID to void
		  - changed ASMLANGUAGE to _ASMLANGUAGE
		  - changed copyright notice
01b,13oct92,hdn  debugged.
01a,08jul92,hdn  written based on TRON version.
*/

/*
DESCRIPTION
This module contains assembly language routines needed for the debug
package and the i80x86 exception vectors. 

.ne 24
PICTURE OF STACK GROWING
.CS
  
							 sp_5 ---------
								sp_1(pointer to ESF0)
							 sp_4 ---------
								sp_2(pointer to REGS)
							---------
							0 or 1
			  fp/sp_3 ---------		 ---------
				     fp			 fp
			     sp_2 ---------		 ---------
				db0			db0
				 |			  |
				db7			db7
				    edi			edi
				     |			  |
				    eax			eax
  sp_1 ---------		 ---------		 ---------
	 EIP			EIP			EIP
	---------		 ---------		 ---------
	 CS			 CS			 CS
	---------		 ---------		 ---------
	 EFLAGS		    EFLAGS		    EFLAGS
	---------		 ---------		 ---------
  
     fig.1		     fig.2		     fig.3
.CE 
  
SEE ALSO: dbgLib(1), "Debugging"
*/

#define _ASMLANGUAGE
#include "vxWorks.h"
#include "asm.h"
#include "wdb/wdbDbgLib.h"


	.data
	.globl	FUNC(copyright_wind_river)
	.long	FUNC(copyright_wind_river)
	

	/* externals */

	.globl	FUNC(wdbDbgPreBreakpoint) /* breakpoint processing routine */
	.globl	FUNC(wdbDbgPreTrace)	/* trace processing routine */
	.globl  VAR(wdbDbgCtxPc)	/* saved PC */
	.globl  VAR(wdbDbgCtxCs)	/* saved CS */
	.globl  VAR(wdbDbgCtxEsp)	/* saved ESP */
	.globl  VAR(sysCsSuper)		/* task level CS */


	/* internals */

	.globl	GTEXT(wdbDbgBpStub)	/* breakpoint exceptions handler */
	.globl	GTEXT(wdbDbgTraceStub)	/* trace exceptions handler */
	.globl	GTEXT(wdbDbgRegsSet)	/* set Debug Registers */
	.globl	GTEXT(_wdbDbgCtxLoad)	/* load the new context */


	.text
	.balign 16

/****************************************************************************
*
* wdbDbgBpStub - software breakpoint handling
*
* This routine is attached to the breakpoint trap (default int $3).  It
* saves the entire task context on the stack and calls wdbDbgBreakpoint () to 
* handle the event.
*
* NOMANUAL
*/

FUNC_LABEL(wdbDbgBpStub)		/* sp_1: breakpoint trap driver */
	pushal				/* sp_2: save regs */
	movl	%db7,%eax
	pushl	%eax
	movl	%db6,%eax
	pushl	%eax
	movl	%db3,%eax
	pushl	%eax
	movl	%db2,%eax
	pushl	%eax
	movl	%db1,%eax
	pushl	%eax
	movl	%db0,%eax
	pushl	%eax

	movl	%esp,%eax		/*     : sp points to saved regs */
	pushl	%ebp			/*     : save a frame-pointer */
	movl	%esp,%ebp		/* sp_3: make a frame-pointer */
	decl	SAVED_REGS+0x4(%ebp)	/*     : adjust saved program counter */
	pushl	$0			/*     : push FALSE is hardware break */
	pushl	%eax			/* sp_4: push pointer to saved regs */
	leal	SAVED_REGS+0x4(%ebp),%eax
	pushl	%eax			/* sp_5: push pointer to saved info */
	call	FUNC(wdbDbgPreBreakpoint) /*     : do breakpoint handling */

	/* we only return if the breakpoint was hit at interrupt level */

	addl	$0x10,%esp		/* sp_2: pop the params,frame-pointer */
	addl	$ SAVED_DBGREGS,%esp	/*     : pop saved debug registers */
	popal				/* sp_1: restore regs */
	iret

/**************************************************************************
*
* wdbDbgTraceStub - hardware breakpoint and trace exception handling
*
* This routine is attached to the i80x86 trace exception vector.  It saves the
* entire task context on the stack and calls wdbDbgTrace () to handle the event.
* This handler is for any of these situations:
*     (1) Single step.				Trap	BS=1
*     (2) Hardware instruction breakpoint.	Falt	Bn=1
*     (3) Hardware data breakpoint.		Trap	Bn=1
*
* NOMANUAL
*/

	.balign 16,0x90
FUNC_LABEL(wdbDbgTraceStub)		/* sp_1: trace trap driver */
	andl	$0xfffffeff,0x8(%esp)	/* clear TraceFlag in saved EFLAGS */
	pushal				/* sp_2: save regs */
	movl	%db7,%eax
	pushl	%eax
	movl	%db6,%eax
	pushl	%eax
	movl	%db3,%eax
	pushl	%eax
	movl	%db2,%eax
	pushl	%eax
	movl	%db1,%eax
	pushl	%eax
	movl	%db0,%eax
	pushl	%eax

	movl	%esp,%eax		/*     : sp points to saved regs */
	pushl	%ebp			/*     : save a frame-pointer */
	movl	%esp,%ebp		/* sp_3: make a frame-pointer */
	pushl	$1			/*     : push TRUE is hardware break */
	pushl	%eax			/* sp_4: push pointer to saved regs */
	leal	SAVED_REGS+4(%ebp),%eax
	pushl	%eax			/* sp_5: push pointer to saved info */

	movl	%db6,%eax		/* get DR6 debug status reg */

	movl	%db6,%eax		/* get DR6 debug status reg */
	xorl	%edx,%edx
	movl	%edx,%db7		/* clear DR7 debug control reg */
	movl	%edx,%db6		/* clear DR6 debug status reg */
	movl	%edx,%db3		/* clear DR3 debug reg */
	movl	%edx,%db2		/* clear DR2 debug reg */
	movl	%edx,%db1		/* clear DR1 debug reg */
	movl	%edx,%db0		/* clear DR0 debug reg */

	btl	$14,%eax		/* is it a trace-exception ? */
	jc	wdbDbgTrace0		/* if yes, jump wdbDbgTrace0 */
	orl	$0x00010000,SAVED_REGS+12(%ebp) /* set ResumeFlag */
	call	FUNC(wdbDbgPreBreakpoint) /* do breakpoint handling */
	jmp	wdbDbgTrace1

	.balign 16,0x90
wdbDbgTrace0:
	call	FUNC(wdbDbgPreTrace)	/* do trace handling */

wdbDbgTrace1:
	/* we returned from wdbDbgTrace if this was a CONTINUE or the break
	 * was hit at interrupt level
	 */

	addl	$0x10,%esp		/* sp_2: pop the params,frame-pointer */
	popl	%eax
	movl	%eax,%db0
	popl	%eax
	movl	%eax,%db1
	popl	%eax
	movl	%eax,%db2
	popl	%eax
	movl	%eax,%db3
	popl	%eax
	xorl	%eax,%eax
	movl	%eax,%db6		/* clear status bits in DR6 */
	popl	%eax
	movl	%eax,%db7
	popal				/* sp_1: restore regs */
	iret

/**************************************************************************
*
* wdbDbgRegsSet - set the debug registers
*
* This routine set hardware breakpoint registers.
*
* SEE ALSO: "i80x86 32-Bit Microprocessor User's Manual"

* void wdbDbgRegsSet (pReg)
*     int *pReg;		/* pointer to saved registers *

*/

	.balign 16,0x90
FUNC_LABEL(wdbDbgRegsSet)

	pushl	%ebp
	movl	%esp,%ebp

	movl	ARG1(%ebp),%edx

	movl	0(%edx),%eax
	movl	%eax,%db0
	movl	4(%edx),%eax
	movl	%eax,%db1
	movl	8(%edx),%eax
	movl	%eax,%db2
	movl	12(%edx),%eax
	movl	%eax,%db3
	movl	16(%edx),%eax
	movl	%eax,%db6
	movl	20(%edx),%eax
	movl	%eax,%db7

	leave
	ret

/*******************************************************************************
*
* _wdbDbgCtxLoad - Load a new context in the current task
*
* This is just like longjmp, but every register must be loaded.
* You could also look at this as half a context switch.
*
* RETURNS: Never returns

* void _wdbDbgCtxLoad
*     (
*     REG_SET * pRegs		/@ Context to load @/
*     )

*/

	.balign 16,0x90
FUNC_LABEL(_wdbDbgCtxLoad)
	movl	SP_ARG1(%esp),%eax

	movl	0x00(%eax),%edi		/* load 5 registers */
	movl	0x04(%eax),%esi
	movl	0x08(%eax),%ebp
	movl	0x10(%eax),%ebx
	movl	0x18(%eax),%ecx

	cli				/* LOCK INTERRUPTS */

	movl	0x0c(%eax),%esp		/* load the stack pointer */

	pushl	0x20(%eax)		/* push EFLAGS */

	/*
	 * Restoring the previous CS(code selector) is required
	 * to go back to the int-level context.  This happens
	 * when we switch back to the system from the external
	 * agent.  To do this, the CS should be included in the
	 * REG_SET.  In the mean time, the CS in the breakpoint 
	 * and trace exception stack frame is saved in the 
	 * handler and restored here.  To find the right REG_SET, 
	 * the saved PC(program counter) and ESP(stack pointer) 
	 * are compared to ones in REG_SET beforehand.
	 */

	movl	0x24(%eax),%edx		/* get the PC in REG_SET */
	cmpl	FUNC(wdbDbgCtxPc),%edx	/* compare the PC */
	jne	wdbDbgCtxLoad0		/* jump if different */
	movl	0x0c(%eax),%edx		/* get the ESP in REG_SET */
	cmpl	FUNC(wdbDbgCtxEsp),%edx	/* compare the ESP */
	jne	wdbDbgCtxLoad0		/* jump if different */

	pushl	FUNC(wdbDbgCtxCs)	/* push the previous CS */
	jmp	wdbDbgCtxLoad1

wdbDbgCtxLoad0:
	pushl	FUNC(sysCsSuper)	/* push the task level CS */

wdbDbgCtxLoad1:
	pushl	0x24(%eax)		/* push PC */

	movl	0x14(%eax),%edx		/* load remaining 2 registers */
	movl	0x1c(%eax),%eax

	iret				/* UNLOCK INTERRUPTS */

