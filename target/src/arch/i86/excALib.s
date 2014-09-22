/* excALib.s - exception handling I80x86 assembly routines */

/* Copyright 1984-2001 Wind River Systems, Inc. */

/*
modification history
--------------------
01f,25sep01,hdn  called intEnt() in excIntStub() to support the interrupt stack
01e,23aug01,hdn  added FUNC/FUNC_LABEL, replaced .align with .balign
		 added "error" parameter for exc{Exc,Int}Handle.
01d,26sep95,hdn  fixed a bug by incrementing _intCnt in excIntStub.
01c,04jun93,hdn  updated to 5.1
		  -overhauled
		  -fixed #else and #endif
		  -changed ASMLANGUAGE to _ASMLANGUAGE
		  -changed copyright notice
01b,13oct92,hdn  debugged.
01a,28feb92,hdn  written based on TRON, 68k version.
*/

/*
DESCRIPTION
This module contains the assembly language exception handling stub.
It is connected directly to the 80x86 exception vectors.
It sets up an appropriate environment and then calls a routine
in excLib(1).

.ne 26
EXCEPTION STACK FRAME GROWTH
.CS
	   Exception/Trap               Interrupt
                                     -----------------------
                                      vector-number
  sp_5 -----------------------       -----------------------
	vector-number                 sp_1(pointer to ESFn) 
  sp_4 -----------------------       -----------------------
        sp_1(pointer to ESFn)         sp_2(pointer to REGS)
  sp_3 -----------------------       -----------------------
        sp_2(pointer to REGS)         errno
  sp_2 -----------------------       -----------------------
        edi                           edi
         |                             |
        eax                           eax
	eflags                        eflags
	pc / return-address           pc / return-address
  sp_1 -----------------------       -----------------------
        EIP        ERROR              EIP
  0x04 ---------  ---------          -----------------------
        CS         EIP                CS
  0x08 ---------  ---------          ---------
        EFLAGS     CS                 EFLAGS
  0x0c ---------  ---------          ---------
                   EFLAGS  
  0x10            ---------

        ESF0   or   ESF1              ESF0
.CE

SEE ALSO: excLib(1)
*/

#define _ASMLANGUAGE
#include "vxWorks.h"
#include "asm.h"
#include "esf.h"
#include "iv.h"


	.data
	.globl	FUNC(copyright_wind_river)
	.long	FUNC(copyright_wind_river)


       	/* externals */

	.globl	FUNC(windExit)	    /* kernel exit routine */
	.globl	FUNC(intEnt)	    /* interrupt enter routine */
	.globl	FUNC(intExit)	    /* interrupt exit routine */


	/* internals */

	.globl	GTEXT(excCallTbl)   /* call-table */
	.globl	GTEXT(excStub)	    /* uninitialized exception handler */
	.globl	GTEXT(excIntStub)   /* uninitialized interrupt handler */


	.text
	.balign 16

/**************************************************************************
*
* excCallTbl - table of Calls
*
* NOMANUAL
*/

FUNC_LABEL(excCallTbl)
	call	FUNC(excStub)		/* 0x00 */ /* divide error */
	call	FUNC(excStub)		/* debug */
	call	FUNC(excStub)		/* non-maskable interrupt */
	call	FUNC(excStub)		/* breakpoint */
	call	FUNC(excStub)		/* overflow */
	call	FUNC(excStub)		/* bound */
	call	FUNC(excStub)		/* invalid opcode */
	call	FUNC(excStub)		/* device not available */
	call	FUNC(excStub)		/* double fault */
	call	FUNC(excStub)		/* co-processor overrun */
	call	FUNC(excStub)		/* invalid TSS */
	call	FUNC(excStub)		/* segment not present */
	call	FUNC(excStub)		/* stack fault */
	call	FUNC(excStub)		/* general protection fault */
	call	FUNC(excStub)		/* page fault */
	call	FUNC(excStub)		/* reserved */
	call	FUNC(excStub)		/* 0x10 */ /* co-processor error */
	call	FUNC(excStub) 		/* alignment check */
	call	FUNC(excStub) 		/* machine check */
	call	FUNC(excStub) 		/* streaming SIMD */
	call	FUNC(excStub) 		/* unassigned reserved */
	call	FUNC(excStub) 		/* unassigned reserved */
	call	FUNC(excStub) 		/* unassigned reserved */
	call	FUNC(excStub) 		/* unassigned reserved */
	call	FUNC(excStub)		/* unassigned reserved */
	call	FUNC(excStub)		/* unassigned reserved */
	call	FUNC(excStub)		/* unassigned reserved */
	call	FUNC(excStub)		/* unassigned reserved */
	call	FUNC(excStub)		/* unassigned reserved */
	call	FUNC(excStub)		/* unassigned reserved */
	call	FUNC(excStub)		/* unassigned reserved */
	call	FUNC(excStub)		/* unassigned reserved */
	call	FUNC(excIntStub)	/* 0x20 */ /* User Interrupts */
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)	/* 0x30 */
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)	/* 0x40 */
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)	/* 0x50 */
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)	/* 0x60 */
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)	/* 0x70 */
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)	/* 0x80 */
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)	/* 0x90 */
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)	/* 0xa0 */
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)	/* 0xb0 */
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)	/* 0xc0 */
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)	/* 0xd0 */
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)	/* 0xe0 */
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)	/* 0xf0 */
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)
	call	FUNC(excIntStub)

/*********************************************************************
*
* excStub - exception handler
*
* NOMANUAL
*/

	.balign 16,0x90
FUNC_LABEL(excStub)
	pushfl				/* save eflags */
	pushal				/* save regs */
	movl	%esp, %ebx		/* save pointer to regs */

	/* compute vector offset from return address to Call in table */

	movl	0x24(%esp), %eax	/* get Call return adrs */
	subl	$4, %eax		/* adjust return adrs to be Call adrs */
	subl	$FUNC(excCallTbl), %eax	/* get offset from start of Call table
					 * (= vector offset) */
	movl	$5, %ecx		/* turn vector offset into excep num */
	cltd
	idivl	%ecx			/* %eax has exception num */

	/* check whether the exception stack frame is ESF0 or ESF1 */

	cmpl	$ IN_DOUBLE_FAULT, %eax
	jl	excStub1		/* vecNum <   8 , it is ESF0 */
	cmpl	$ IN_CP_OVERRUN, %eax
	je	excStub1		/* vecNum ==  9 , it is ESF0 */
	cmpl	$ IN_RESERVED, %eax
	je	excStub1		/* vecNum == 15 , it is ESF0 */
	cmpl	$ IN_CP_ERROR, %eax
	je	excStub1		/* vecNum == 16 , it is ESF0 */
	cmpl	$ IN_MACHINE_CHECK, %eax
	je	excStub1		/* vecNum == 18 , it is ESF0 */
	cmpl	$ IN_SIMD, %eax
	je	excStub1		/* vecNum == 19 , it is ESF0 */

	/* exception stack frame is ESF1 which has error-code */

	movl	0x2c(%esp), %edx	/* get pc from ESF */
	movl	%edx, 0x24(%esp)	/* replace a return addr by the pc */

	pushl	$ TRUE			/* push flag (ERROR code) */
	pushl	%ebx			/* push pointer to REG_SET */
	addl	$0x28, %ebx
	pushl	%ebx			/* push pointer to ESF */
	pushl	%eax			/* push exception number */
	call	FUNC(excExcHandle)
	addl	$16, %esp		/* clean up pushed arguments */

	popal				/* restore regs */
	addl	$12, %esp		/* get pointer to ESF */
	iret				/* retry the instruction */


	.balign 16,0x90
excStub1:
	/* exception stack frame is ESF0 */

	movl	0x28(%esp), %edx	/* get pc from ESF */
	movl	%edx, 0x24(%esp)	/* replace a return addr by the pc */

	pushl	$ FALSE			/* push flag (no ERROR code) */
	pushl	%ebx			/* push pointer to REG_SET */
	addl	$0x28, %ebx
	pushl	%ebx			/* push pointer to ESF */
	pushl	%eax			/* push exception number */
	call	FUNC(excExcHandle)	/* do exception processing */
	addl	$16, %esp		/* clean up pushed arguments */

	popal				/* restore regs */
	addl	$8, %esp		/* get pointer to ESF */
	iret				/* retry the instruction */


/*********************************************************************
*
* excIntStub - uninitialized interrupt handler
*
* NOMANUAL
*/

	.balign 16,0x90
FUNC_LABEL(excIntStub)
	popl    %eax                    /* save the return address */
	call    FUNC(intEnt)            /* call intEnt */
	pushl   %eax                    /* push PC (return address) */

	/* create REG_SET on the stack for the Show routine */

	pushfl				/* save EFLAGS */
	pushal				/* save regs */
	movl	%esp, %ebx		/* save pointer to REG_SET */

	/* compute vector offset from return address to Call in table */

	movl	0x24(%esp), %eax	/* get Call return adrs */
	subl	$4, %eax		/* adjust return adrs to be Call adrs */
	subl	$FUNC(excCallTbl), %eax	/* get offset from start of Call table
					 * (= vector offset) */
	movl	$5, %ecx		/* turn vector offset into excep num */
	cltd
	idivl	%ecx			/* %eax has exception num */

	/* exception stack frame is ESF0. offset = REG_SET(0x28) + errno(4) */

	movl	0x28+4(%esp), %edx	/* get pc from ESF */
	movl	%edx, 0x24(%esp)	/* replace a return addr by the pc */

	pushl	$ FALSE			/* push flag (no ERROR code) */
	pushl	%ebx			/* push pointer to REG_SET */
	addl	$0x28+4, %ebx		/* get addr of ESF by adding the offset */
	pushl	%ebx			/* push pointer to ESF */
	pushl	%eax			/* push exception number */
	call	FUNC(excIntHandle)	/* do exception processing */
	addl	$16, %esp		/* clean up pushed arguments */

	popal				/* restore regs */
	addl	$8, %esp		/* skip EFLAGS, PC to get addr of errno */
	jmp	FUNC(intExit)

