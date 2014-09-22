/* ipiALib.s - Inter Processor Interrupt (IPI) handling I80x86 assembly routines */

/* Copyright 1984-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01b,27feb02,hdn  renamed ipiStubShutdown() to ipiShutdownSup()
01a,20feb02,hdn  written
*/

/*
DESCRIPTION
This module contains the assembly language Inter Processor Interrupt (IPI)
handling stub.  It is connected directly to the 80x86 IPI vectors.
It sets up an appropriate environment and then calls a routine
in ipiArchLib(1).

SEE ALSO: ipiArchLib(1), loApicIntr.c
*/

#define _ASMLANGUAGE
#include "vxWorks.h"
#include "asm.h"
#include "regs.h"
#include "esf.h"
#include "iv.h"
#include "excLib.h"
#include "private/taskLibP.h"


/* defines */


	.data
	.globl	FUNC(copyright_wind_river)
	.long	FUNC(copyright_wind_river)


       	/* externals */

	.globl	FUNC(ipiHandler)	/* IPI handler */
	.globl	VAR(sysNcpu)		/* number of CPUs */
	.globl	VAR(sysLockSem)		/* MP Lock Semaphore */


	/* internals */

	.globl	GTEXT(ipiCallTbl)	/* IPI call-table */
	.globl	GTEXT(ipiStub)		/* IPI handler stub */
	.globl	GTEXT(ipiShutdownSup)	/* IPI handler stub: Shutdown */
	.globl	GTEXT(ipiHandlerTlbFlush) /* IPI handler: TLB flush */
	.globl	GTEXT(ipiHandlerTscReset) /* IPI handler: TSC reset */


	.text
	.balign 16

/**************************************************************************
*
* ipiCallTbl - table of IPI calls
*
* NOMANUAL
*/

FUNC_LABEL(ipiCallTbl)
	call	FUNC(ipiStub); nop; nop; nop;	/* */
	call	FUNC(ipiStub); nop; nop; nop;	/* */
	call	FUNC(ipiStub); nop; nop; nop;	/* */
	call	FUNC(ipiStub); nop; nop; nop;	/* */
	call	FUNC(ipiStub); nop; nop; nop;	/* */
	call	FUNC(ipiStub); nop; nop; nop;	/* */
	call	FUNC(ipiStub); nop; nop; nop;	/* */
	call	FUNC(ipiStub); nop; nop; nop;	/* */

/*********************************************************************
*
* ipiStub - IPI handler 
*
* This routine is the default IPI handler. 
*
* NOMANUAL
*/

	.balign 16,0x90
FUNC_LABEL(ipiStub)
	
	/* create REG_SET on the stack */
					/* return address is the PC */
	pushfl				/* push EFLAGS */
	pushal				/* push general registers */
	movl	%esp, %ebp		/* save pointer to REG_SET to %ebp */

	/* compute vector offset from return address to call-table */

	movl	REG_PC(%ebp), %eax	/* get call return addr */
	subl	$5, %eax		/* adjust ret addr to be call addr */
	subl	$FUNC(ipiCallTbl), %eax	/* get offset from start of call-table */
	shrl	$3, %eax		/* turn vector offset into index */

	/* call ipiHandler */

	pushl	%eax			/* push the index */
	call	FUNC(ipiHandler)	/* do IPI processing */
	addl	$4, %esp		/* clean up pushed argument */

	/* restore general registers and return */

	popal				/* pop general registers */
	addl	$8, %esp		/* skip EFLAGS, PC */
	iret				/* retry the instruction */

/*********************************************************************
*
* ipiHandlerTlbFlush - IPI handler to flush the TLB
*
* NOMANUAL
*/

	.balign 16,0x90
FUNC_LABEL(ipiHandlerTlbFlush)
	movl	%cr3, %eax
	movl	%eax, %cr3
	ret

/*********************************************************************
*
* ipiHandlerTscReset - IPI handler to reset the TSC
*
* NOMANUAL
*/

	.balign 16,0x90
FUNC_LABEL(ipiHandlerTscReset)
        xorl    %eax,%eax               /* zero low-order 32 bits */
        xorl    %edx,%edx               /* zero high-order 32 bits */
        movl    $ MSR_TSC,%ecx          /* specify MSR_TSC */
        wrmsr                           /* write %edx:%eax to TSC */
        ret

/*********************************************************************
*
* ipiShutdownSup - IPI handler to shutdown the CPU immediately
*
* NOMANUAL
*/

	.balign 16,0x90
FUNC_LABEL(ipiShutdownSup)

	/* decrement the CPU counter */

	movl	FUNC(sysNcpu), %eax
	lock
	decl	(%eax)

	/* write-back & invalidate the cache */

	wbinvd

	/* lock interrupts & halt */

	cli			/* LOCK INTERRUTPS */

ipiShutdownSup1:
	hlt
	jmp	ipiShutdownSup1

