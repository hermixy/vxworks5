/* Copyright 1991-1992 Wind River Systems, Inc. */
	.data
	.globl	_copyright_wind_river
	.long	_copyright_wind_river

/*
modification history
--------------------
01f,04jun96,ms   fixed SPR 6583 - tracing during fpp emulation trashes a0
01e,23aug92,jcf  changed bxxx to jxx.
01d,26may92,rrr  the tree shuffle
01c,17dec91,kdl  added changes from Motorola "skeleton.sa" v3.2:
		 don't do fsave unless frame format id is 40.
01b,04dec91,kdl  fixed register saving in __x_fpsp_ill_inst.
01a,12aug91,jcf  adapted from Motorola version 2.1.
*/

/*
DESCRIPTION

skeleton.s 3.1 12/10/90,  2.1 Motorola 040 Floating Point Software Package

The exception handler entry points for the FPSP are as follows:

    ill_inst	-> __x_fpsp_ill_inst
    fline	-> __x_fpsp_fline
    bsun	-> __x_fpsp_bsun
    inex	-> __x_fpsp_inex
    dz		-> __x_fpsp_dz
    unfl	-> __x_fpsp_unfl
    operr	-> __x_fpsp_operr
    ovfl	-> __x_fpsp_ovfl
    snan	-> __x_fpsp_snan
    unsupp	-> __x_fpsp_unsupp

If the FPSP determines that the exception is one that must be handled by
the operating system then there will be a return from the package by a 'jmp
real_xxxx' where xxxx corresponds to an exception outlined above.  At that
point the machine state will be identical to the state before the FPSP was
entered.  In particular, whatever condition that caused the exception will
still be pending when the FPSP package returns.  This will enable system
specific code, contained in excALib()/excLib() to handle the exception.

If the exception was completely handled by the package, then the return
will be via a 'jmp fpsp_done'.  Unless there is OS specific work to be done
(such as handling a context switch or interrupt) the user program can be
resumed via 'rte'.

In the following skeleton code, some typical 'real_xxxx' handling code is
shown.  This code may need to be moved to an appropriate place in the
target system, or rewritten.

Copyright (C) Motorola, Inc. 1990
All Rights Reserved

THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA
The copyright notice above does not evidence any
actual or intended publication of such source code.

Original VxWorks implementation by Jim Foris, GE Medical Systems.

NOMANUAL
*/

#include "fpsp040E.h"

/* imports */

	.globl	_excStub
	.globl	__x_b1238_fix
	.globl	__x_do_func
	.globl	__x_gen_except
	.globl	__x_get_op
	.globl	__x_sto_res

/* globals */

	.globl	__x_real_bsun	/* called from __x_fpsp_bsun for OS handling */
	.globl	__x_real_dz	/* called from __x_fpsp_dz for OS handling */
	.globl	__x_real_operr	/* called from __x_fpsp_operr for OS handling */
	.globl	__x_real_ovfl	/* called from __x_fpsp_ovfl for OS handling */
	.globl	__x_real_snan	/* called from __x_fpsp_snan for OS handling */
	.globl	__x_real_trace	/* called from __x_fpsp_trace for OS handling */
	.globl	__x_real_unfl	/* called from __x_fpsp_unfl for OS handling */
	.globl	__x_real_unsupp	/* called from __x_fpsp_unsupp for OS handling*/
	.globl	__x_real_inex	/* called from __x_fpsp_inex for OS handling */
	.globl	__x_fpsp_dz	/* divide by zero exception handler */
	.globl	__x_fpsp_inex	/* inexact exception handler */
	.globl	__x_fpsp_fline	/* fline exception handler */
	.globl	__x_fpsp_ill_inst  /* illegal instruction exception handler */
	.globl	__x_fpsp_fmt_error /* called if format not supported */
	.globl	__x_fpsp_done	/* called from handlers to return to user */
	.globl	__x_mem_read	/* utility routine to read memory */
	.globl	__x_mem_write	/* utility routine to write memory */

	.text

/*******************************************************************************
*
* BSUN exception
*
* This sample handler simply clears the nan bit in the FPSR.
*/

__x_real_bsun:
	link		a6,#-192
	fsave		a7@-
	bclr		#E1,a6@(E_BYTE)	/* bsun is always an E1 exception */
	fmovel		FPSR,a7@-
	bclr		#nan_bit,a7@
	fmovel		a7@+,FPSR
	frestore	a7@+
	unlk		a6
	jmp		_excStub	/* start VxWorks exception handling */

/*******************************************************************************
*
* Divide by Zero exception
*
*/

__x_fpsp_dz:
__x_real_dz:
	link		a6,#-192
	fsave		a7@-
	bclr		#E1,a6@(E_BYTE)
	frestore	a7@+
	unlk		a6
	jmp		_excStub	/* start VxWorks exception handling */

/*******************************************************************************
*
* Operand Error exception
*/

__x_real_operr:
	link		a6,#-192
	fsave		a7@-
	bclr		#E1,a6@(E_BYTE)	/* operr is always an E1 exception */
	frestore	a7@+
	unlk		a6
	jmp		_excStub	/* start VxWorks exception handling */

/*******************************************************************************
*
* Overflow exception
*/

__x_real_ovfl:
	link		a6,#-192
	fsave		a7@-
	bclr		#E3,a6@(E_BYTE)	/* clear and test E3 flag */
	jne 		ovfl_done
	bclr		#E1,a6@(E_BYTE)
ovfl_done:
	frestore	a7@+
	unlk		a6
	jmp		_excStub	/* start VxWorks exception handling */

/*******************************************************************************
*
* Signalling NAN exception
*/

__x_real_snan:
	link		a6,#-192
	fsave		a7@-
	bclr		#E1,a6@(E_BYTE)	/* snan is always an E1 exception */
	frestore	a7@+
	unlk		a6
	jmp		_excStub	/* start VxWorks exception handling */

/*******************************************************************************
*
* Trace Exception
*/

__x_real_trace:
	movel		a0, sp@-	/* save a0 to the stack */
	movec		vbr,a0		/* compute IV_TRACE vector address */
	movel		a0@(0x24),a0
	movel		a0, sp@-	/* and push it on the stack */
	movel		sp@(0x4), a0	/* restore a0 */
	rtd		#4		/* jump to trace vector */

/*******************************************************************************
*
* Underflow exception
*/

__x_real_unfl:
	link		a6,#-192
	fsave		a7@-
	bclr		#E3,a6@(E_BYTE)	/* clear and test E3 flag */
	jne 		unfl_done
	bclr		#E1,a6@(E_BYTE)
unfl_done:
	frestore	a7@+
	unlk		a6
	jmp		_excStub	/* start VxWorks exception handling */

/*******************************************************************************
*
* Unsupported data type exception
*/

__x_real_unsupp:
	link		a6,#-192
	fsave		a7@-
	bclr		#E1,a6@(E_BYTE)	/* unsupp is always an E1 exception */
	frestore	a7@+
	unlk		a6
	jmp		_excStub	/* start VxWorks exception handling */

/*******************************************************************************
*
* Inexact exception
*
* All inexact exceptions are real, but the 'real' handler
* will probably want to clear the pending exception.
* The provided code will clear the E3 exception (if pending),
* otherwise clear the E1 exception.  The frestore is not really
* necessary for E1 exceptions.
*
* Code following the 'inex' label is to handle bug #1232.  In this
* bug, if an E1 snan, ovfl, or unfl occured, and the process was
* swapped out before taking the exception, the exception taken on
* return was inex, rather than the correct exception.  The snan, ovfl,
* and unfl exception to be taken must not have been enabled.  The
* fix is to check for E1, and the existence of one of snan, ovfl,
* or unfl bits set in the fpsr.  If any of these are set, branch
* to the appropriate  handler for the exception in the fpsr.  Note
* that this fix is only for d43b parts, and is skipped if the
* version number is not 0x40.
*/

__x_fpsp_inex:
	link		a6,#-LOCAL_SIZE
	fsave		a7@-
	cmpib		#VER_40,a7@		| test version number
	jne 		not_fmt40
	fmovel		fpsr,a7@-
	btst		#E1,a6@(E_BYTE)		| test for E1 set
	jeq 		not_b1232
	btst		#__x_snan_bit,a7@(2)	| test for snan
beq		__x_inex_ckofl
	addl		#4,sp
	frestore	a7@+
	unlk		a6
	jra		__x_fpsp_snan
__x_inex_ckofl:
	btst		#__x_ovfl_bit,a7@(2)	| test for ovfl
	jeq		__x_inex_ckufl
	addl		#4,sp
	frestore	a7@+
	unlk		a6
	jra		__x_fpsp_ovfl
__x_inex_ckufl:
	btst		#__x_unfl_bit,a7@(2)	| test for unfl
	jeq		not_b1232
	addl		#4,sp
	frestore	a7@+
	unlk		a6
	jra		__x_fpsp_unfl

|
| We do not have the bug 1232 case.  Clean up the stack and call
| __x_real_inex.
|
not_b1232:
	addl		#4,sp
	frestore	a7@+
	unlk		a6

__x_real_inex:
	link		a6,#-LOCAL_SIZE
	fsave		a7@-
not_fmt40:
	bclr		#E3,a6@(E_BYTE)		| clear and test E3 flag
	jeq 		__x_inex_cke1
|
| Clear dirty bit on dest resister in the frame before branching
| to __x_b1238_fix.
|
	moveml		d0/d1,a6@(USER_DA)
	bfextu		a6@(CMDREG1B){#6:#3},d0	| get dest reg no
	bclr		d0,a6@(FPR_DIRTY_BITS)	| clr dest dirty bit
	bsrl		__x_b1238_fix		| test for bug1238 case
	moveml		a6@(USER_DA),d0/d1
	jra 		__x_inex_done
__x_inex_cke1:
	bclr		#E1,a6@(E_BYTE)
__x_inex_done:
	frestore	a7@+
	unlk		a6
	jmp		_excStub	/* start VxWorks exception handling */

/*******************************************************************************
*
* fpsp_ill_inst - illegal instruction handler (from Mot. FPSP Version 1.0)
*
* ERRATA E34 fix: F-line illegal instruction through vector 4
*/

__x_fpsp_ill_inst:
	movl		a0,a7@-		| save a0
	movl		a7@(6),a0	| copy PC into a0
	cmpw		#0xffff,a0@	| check if it was really a F-line
	jeq 		fpsp_ill1	| skip if true
	movl		a7@+,a0		| else, restore a0
	jmp		_excStub	| and jump into common vxWorks handler*

fpsp_ill1:				| really a F-line exception
	movl		a7@+,a0		| so restore a0
	jra 		__x_fpsp_fline	| and jump into F-line handler

/*******************************************************************************
*
* fpsp_fline --- FPSP handler for fline exception (x_flinesa 3.3 1/10/91)
*
* A 'real' F-line exception is one that the FPSP isn't supposed to
* handle. E.g. an instruction with a co-processor ID that is not 1.
* First determine if the exception is one of the unimplemented floating point
* instructions.  If so, let fpsp_unimp handle it.  Next, determine if the
* instruction is an fmovecr with a non-zero <ea> field.  If so, handle here
* and return.  Otherwise, it must be a real F-line exception.
*/

__x_fpsp_fline:

/* Check for unimplemented vector first.  Use EXC_VEC-4 because
 * the equate is valid only after a 'link a6' has pushed one more
 * .long onto the stack.
 */
	cmpw	#UNIMP_VEC,a7@(EXC_VEC-4)
	jeq 	__x_fpsp_unimp
				| fmovecr with non-zero <ea> handling here
	subl	#4,a7		| 4 accounts for 2-word difference
				| between six word frame (unimp) and
				| four word frame
	link	a6,#-LOCAL_SIZE
	fsave	a7@-
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	moveal	a6@(EXC_PC+4),a0 | get address of __x_fline instruction
	lea	a6@(L_SCR1),a1	| use L_SCR1 as scratch
	movel	#4,d0
	addl	#4,a6		| to offset the subl #4,a7 above so that
				| a6 can point correctly to the stack frame
				| before branching to __x_mem_read
	bsrl	__x_mem_read
	subl	#4,a6
	movel	a6@(L_SCR1),d0	| d0 contains the __x_fline and command word
	bfextu	d0{#4:#3},d1	| extract coprocessor id
	cmpib	#1,d1		| check if cpid=1
	jne	not_mvcr	| exit if not
	bfextu	d0{#16:#6},d1
	cmpib	#0x17,d1	| check if it is an FMOVECR encoding
	jne	not_mvcr
				| if an FMOVECR instruction, fix stack
				| and go to FPSP_UNIMP
fix_stack:
	cmpib	#VER_40,a7@	| test for orig unimp frame
	jne 	ck_rev
	subl	#UNIMP_40_SIZE-4,a7 | emulate an orig fsave
	moveb	#VER_40,a7@
	moveb	#UNIMP_40_SIZE-4,a7@(1)
	clrw	a7@(2)
	jra 	fix_con
ck_rev:
	cmpib	#VER_41,a7@		| test for rev unimp frame
	jne 	__x_fpsp_fmt_error	| if not 0x40 or 0x41, exit with error
	subl	#UNIMP_41_SIZE-4,a7 	| emulate a rev fsave
	moveb	#VER_41,a7@
	moveb	#UNIMP_41_SIZE-4,a7@(1)
	clrw	a7@(2)
fix_con:
	movew	a6@(EXC_SR+4),a6@(EXC_SR) | move stacked sr to new position
	movel	a6@(EXC_PC+4),a6@(EXC_PC) | move stacked pc to new position
	fmovel	a6@(EXC_PC),fpi 	| point fpi to __x_fline inst
	movel	#4,d1
	addl	d1,a6@(EXC_PC)		| increment stacked pc val to next inst
	movew	#0x202c,a6@(EXC_VEC) 	| reformat vector to unimp
	clrl	a6@(EXC_EA)		| clear the EXC_EA field
	movew	d0,a6@(CMDREG1B) 	| move the lower word into CMDREG1B
	clrl	a6@(E_BYTE)
	bset	#UFLAG,a6@(T_BYTE)
	moveml	a6@(USER_DA),d0-d1/a0-a1 | restore data registers
	jra 	__x_uni_2

not_mvcr:
	moveml	a6@(USER_DA),d0-d1/a0-a1 | restore data registers
	frestore a7@+
	unlk	a6
	addl	#4,a7

__x_real_fline:
	jmp		_excStub	/* start VxWorks exception handling */

/*******************************************************************************
*
* fpsp_unimp --- FPSP handler for unimp inst exception (x_unimpsa 3.2 4/26/91)
*
* Invoked when the user program encounters a floating-point op-code that
* hardware does not support.  Trap vector# 11 (See table 8-1 MC68030 User's
* Manual).
*
* Note: An fsave for an unimplemented inst. will create a short
* fsave stack.
*
*  Input: 1. Six word stack frame for unimplemented inst, four word
*            for illegal. (See table 8-7 MC68030 User's Manual).
*         2. Unimp (short) fsave state frame created here by fsave
*            instruction.
*/

__x_fpsp_unimp:
	link		a6,#-LOCAL_SIZE
	fsave		a7@-
__x_uni_2:
	moveml		d0-d1/a0-a1,a6@(USER_DA)
	fmovemx		fp0-fp3,a6@(USER_FP0)
	fmoveml		fpcr/fpsr/fpi,a6@(USER_FPCR)

	moveb		a7@,d0		| test for valid version num
	andib		#0xf0,d0	| test for 0x4x
	cmpib		#VER_4,d0	| must be 0x4x or exit
	jne 		__x_fpsp_fmt_error

/* Temporary D25B Fix - The following lines are used to ensure that the FPSR
 * exception byte and condition codes are clear before proceeding
 */
	movel		a6@(USER_FPSR),d0
	andl		#0xFF,d0	| clear all but accrued exceptions
	movel		d0,a6@(USER_FPSR)
	fmovel		#0,FPSR 	| clear all user bits
	fmovel		#0,fpcr		| clear all user exceptions for FPSP
	clrb		a6@(UFLG_TMP)	| clr flag for __x_unsupp data
	bsrl		__x_get_op	| go get operand(s)
	clrb		a6@(STORE_FLG)
	bsrl		__x_do_func	| do the function
	fsave		a7@-		| capture possible exc state
	tstb		a6@(STORE_FLG)
	jne 		no_store	| if STORE_FLG is set, no __x_store
	bsrl		__x_sto_res	| store the result in user space
no_store:
	jra 		__x_gen_except	| post any exceptions and return

/*******************************************************************************
*
* __x_fpsp_fmt_error --- exit point for frame format error
*
* The fpu stack frame does not match the frames existing
* or planned at the time of this writing.  The fpsp is
* unable to handle frame sizes not in the following
* version:size pairs:
*
* {4060, 4160} - busy frame
* {4028, 4130} - unimp frame
* {4000, 4100} - idle frame
*
* This entry point simply holds an f-line illegal value.
* Replace this with a call to your kernel panic code or
* code to handle future revisions of the fpu.
*/

__x_fpsp_fmt_error:
	.long	0xf27f0000	| f-line illegal

/*******************************************************************************
*
* __x_fpsp_done --- FPSP exit point
*
* The exception has been handled by the package and we are ready
* to return to user mode, but there may be OS specific code
* to execute before we do.  If there is, do it now.
*/

__x_fpsp_done:
	rte

/*******************************************************************************
*
* __x_mem_write --- write to user or supervisor address space
*
* Writes to memory while in supervisor mode.  For systems with MMU this must
* be rewritten to utilize a 'moves' instruction.
*
* Input:
*     a0 - supervisor source address
*     a1 - user destination address
*     d0 - number of bytes to write (maximum count is 12)
*/

__x_mem_write:
	movb		a0@+,a1@+
	subql		#1,d0
	jne 		__x_mem_write
	rts

/*******************************************************************************
*
* __x_mem_read --- read from user or supervisor address space
*
* Reads from memory while in supervisor mode.  For systems with MMU this must
* be rewritten to utilize a 'moves' instruction.
*
* The FPSP calls mem_read to read the original F-line instruction in order
* to extract the data register number when the 'Dn' addressing mode is
* used.
*
* Input:
*     a0 - user source address
*     a1 - supervisor destination address
*     d0 - number of bytes to read (maximum count is 12)
*/

__x_mem_read:
	movb		a0@+,a1@+
	subql		#1,d0
	jne 		__x_mem_read
	rts

#if 0
/*******************************************************************************
*
* excFppEmulate - emulate subset of MC68881/MC68882 unimplemented instructions
*
* This routine emulates the MC68881/MC68882 instructions that the GNU toolchain
* generates.  Namely, fmovecr and fintrz.  The notion is that complete
* emulation of the MC68881/MC68882 is only required in situations where
* part of the application is a floating point assembly package.
* For everyone else (the majority?) it is 32K of dead weight.  So we use
* a hook to connect the rest of the support.
*/

__x_excFppEmulate:
	clrb	a6@(CU_ONLY)

/* Check for fmovecr.  It does not follow the format of fp gen unimplemented
 * instructions.  The test is on the upper 6 bits if they are 0x17, the inst
 * is fmovecr.  Call entry smovcr directly.
 */

	bfextu	a6@(CMDREG1B){#0:#6},d0 | get opclass and src fields
	cmpil	#0x17,d0		| if op class and size fields are 0x17,
	jne 	not_fmovecr
	jmp	smovcr			| fmovecr, jmp directly to emulation

not_fmovecr:
	st	a6@(STORE_FLG)		| return ERROR for now
	rts
#endif
