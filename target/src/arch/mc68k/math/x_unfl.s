/* x_unfl.s - Motorola 68040 FP underflow exception handler (EXC) */

/* Copyright 1991-1993 Wind River Systems, Inc. */
	.data
	.globl	_copyright_wind_river
	.long	_copyright_wind_river

/*
modification history
--------------------
01e,21jul93,kdl  added .text (SPR #2372).
01d,23aug92,jcf  changed bxxx to jxx.
01c,26may92,rrr  the tree shuffle
01b,10jan92,kdl  added modification history; general cleanup.
01a,15aug91,kdl  original version, from Motorola FPSP v2.0.
*/

/*
DESCRIPTION


	x_unflsa 3.3 4/26/91

	__x_fpsp_unfl --- FPSP handler for underflow exception

Trap disabled results
	For 881/2 compatibility, sw must denormalize the intermediate
result, then store the result.  Denormalization is accomplished
by taking the intermediate result (which is always normalized) and
shifting the mantissa right while incrementing the exponent until
it is equal to the denormalized exponent for the destination
format.  After denormalizatoin, the result is rounded to the
destination format.

Trap enabled results
	All trap disabled code applies.	In addition the exceptional
operand needs to made available to the user with a bias of 0x6000
added to the exponent.


		Copyright (C) Motorola, Inc. 1990
			All Rights Reserved

	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA
	The copyright notice above does not evidence any
	actual or intended publication of such source code.

X_UNFL	idnt    2,1 Motorola 040 Floating Point Software Package

	section	8

NOMANUAL
*/

#include "fpsp040E.h"

|	xref	__x_denorm
|	xref	__x_round
|	xref	__x_store
|	xref	__x_g_rndpr
|	xref	__x_g_opcls
|	xref	__x_g_dfmtou
|	xref	__x_real_unfl
|	xref	__x_real_inex
|	xref	__x_fpsp_done
|	xref	__x_b1238_fix
|	xref	__x_check_force


	.text

	.globl	__x_fpsp_unfl
__x_fpsp_unfl:
	link		a6,#-LOCAL_SIZE
	fsave		a7@-
	moveml		d0-d1/a0-a1,a6@(USER_DA)
	fmovemx	fp0-fp3,a6@(USER_FP0)
	fmoveml	fpcr/fpsr/fpi,a6@(USER_FPCR)

| At this point we need to look at the instructions and see if it is one of
| the force-precision ones (fsadd,fdadd,fssub,fdsub,fsmul,fdmul,fsdiv,fddiv,
| fssqrt,fdsqrt,fsmove,fdmove,fsabs,fdabs,fsneg,fdneg).  If it is then
/* | correct the USER_FPCR to the instruction's rounding precision (s or d). */
| Also, we need to check if the instruction is fsgldiv or fsglmul.  If it
| is then the USER_FPCR is set to extended rounding precision.  Otherwise
| leave the USER_FPCR alone.
|	bsrl		__x_check_force
|
	bsrl		unf_res	| denormalize, round # store interm op
|
| If underflow exceptions are not enabled, check for inexact
| exception
|
	btst		#__x_unfl_bit,a6@(fpcr_ENABLE)
	jeq 		ck_inex

	btst		#E3,a6@(E_BYTE)
	jeq 		no_e3_1
|
| Clear dirty bit on dest resister in the frame before branching
| to __x_b1238_fix.
|
	bfextu		a6@(CMDREG3B){#6:#3},d0	| get dest reg no
	bclr		d0,a6@(FPR_DIRTY_BITS)	| clr dest dirty bit
	bsrl		__x_b1238_fix		| test for bug1238 case
	movel		a6@(USER_FPSR),a6@(FPSR_SHADOW)
	orl		#sx_mask,a6@(E_BYTE)
no_e3_1:
	moveml		a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP0),fp0-fp3
	fmoveml	a6@(USER_FPCR),fpcr/fpsr/fpi
	frestore	a7@+
	unlk		a6
	jra 		__x_real_unfl
|
| It is possible to have either inex2 or inex1 exceptions with the
| unfl.  If the inex enable bit is set in the fpcr, and either
| inex2 or inex1 occured, we must clean up and branch to the
| real inex handler.
|
ck_inex:
	moveb		a6@(fpcr_ENABLE),d0
	andb		a6@(FPSR_EXCEPT),d0
	andib		#0x3,d0
	jeq 		__x_unfl_done

|
| Inexact enabled and reported, and we must take an inexact exception
|
take_inex:
	btst		#E3,a6@(E_BYTE)
	jeq 		no_e3_2
|
| Clear dirty bit on dest resister in the frame before branching
| to __x_b1238_fix.
|
	bfextu		a6@(CMDREG3B){#6:#3},d0	| get dest reg no
	bclr		d0,a6@(FPR_DIRTY_BITS)	| clr dest dirty bit
	bsrl		__x_b1238_fix		| test for bug1238 case
	movel		a6@(USER_FPSR),a6@(FPSR_SHADOW)
	orl		#sx_mask,a6@(E_BYTE)
no_e3_2:
	moveb		#INEX_VEC,a6@(EXC_VEC+1)
	moveml        	a6@(USER_DA),d0-d1/a0-a1
	fmovemx       	a6@(USER_FP0),fp0-fp3
	fmoveml       	a6@(USER_FPCR),fpcr/fpsr/fpi
	frestore       	a7@+
	unlk            a6
	jra 		__x_real_inex

__x_unfl_done:
	bclr		#E3,a6@(E_BYTE)
	jeq 		e1_set		| if set then branch
|
| Clear dirty bit on dest resister in the frame before branching
| to __x_b1238_fix.
|
	bfextu		a6@(CMDREG3B){#6:#3},d0		| get dest reg no
	bclr		d0,a6@(FPR_DIRTY_BITS)	| clr dest dirty bit
	bsrl		__x_b1238_fix		| test for bug1238 case
	movel		a6@(USER_FPSR),a6@(FPSR_SHADOW)
	orl		#sx_mask,a6@(E_BYTE)
	moveml		a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP0),fp0-fp3
	fmoveml	a6@(USER_FPCR),fpcr/fpsr/fpi
	frestore	a7@+
	unlk		a6
	jra 		__x_fpsp_done
e1_set:
	moveml		a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP0),fp0-fp3
	fmoveml	a6@(USER_FPCR),fpcr/fpsr/fpi
	unlk		a6
	jra 		__x_fpsp_done
|
|	unf_res --- underflow result calculation
|
unf_res:
	bsrl		__x_g_rndpr		| returns RND_PREC in d0 0=ext,
|					| 1=sgl, 2=dbl
|					| we need the RND_PREC in the
|					| upper word for __x_round
	movew		#0,a7@-
	movew		d0,a7@-	| copy RND_PREC to stack
|
|
| If the exception bit set is E3, the exceptional operand from the
| fpu is in WBTEMP|  else it is in FPTEMP.
|
	btst		#E3,a6@(E_BYTE)
	jeq 		unf_E1
unf_E3:
	lea		a6@(WBTEMP),a0	| a0 now points to operand
|
| Test for fsgldiv and fsglmul.  If the inst was one of these, then
| force the precision to extended for the __x_denorm routine.  Use
/* | the user's precision for the __x_round routine. */
|
	movew		a6@(CMDREG3B),d1	| check for fsgldiv or fsglmul
	andiw		#0x7f,d1
	cmpiw		#0x30,d1		| check for sgldiv
	jeq 		unf_sgl
	cmpiw		#0x33,d1		| check for sglmul
	jne 		unf_cont	| if not, use fpcr prec in __x_round
unf_sgl:
	clrl		d0
	movew		#0x1,a7@	| override __x_g_rndpr precision
|					| force single
	jra 		unf_cont
unf_E1:
	lea		a6@(FPTEMP),a0	| a0 now points to operand
unf_cont:
	bclr		#sign_bit,a0@(LOCAL_EX)	| clear sign bit
	sne		a0@(LOCAL_SGN)		| store sign

	bsrl		__x_denorm		| returns denorm, a0 points to it
|
| WARNING:
|				| d0 has guard,round sticky bit
|				| make sure that it is not corrupted
|				| before it reaches the __x_round subroutine
/* |				| also ensure that a0 isn't corrupted */

|
| Set up d1 for __x_round subroutine d1 contains the PREC/MODE
| information respectively on upper/lower register halves.
|
	bfextu		a6@(fpcr_MODE){#2:#2},d1	| get mode from fpcr
|						| mode in lower d1
	addl		a7@+,d1		| merge PREC/MODE
|
| WARNING: a0 and d0 are assumed to be intact between the __x_denorm and
| __x_round subroutines. All code between these two subroutines
| must not corrupt a0 and d0.
|
|
| Perform Round
|	Input:		a0 points to input operand
|			d0{31:29} has guard, round, sticky
|			d1{01:00} has rounding mode
|			d1{17:16} has rounding precision
|	Output:		a0 points to rounded operand
|

	bsrl		__x_round		| returns rounded denorm at	a0@
|
| Differentiate between store to memory vs. store to register
|
unf_store:
	bsrl		__x_g_opcls		| returns opclass in d0{2:0}
	cmpib		#0x3,d0
	jne 		not_opc011
|
| At this point, a store to memory is pending
|
opc011:
	bsrl		__x_g_dfmtou
	tstb		d0
	jeq 		ext_opc011	| If extended, do not subtract
| 					| If destination format is sgl/dbl,
	tstb		a0@(LOCAL_HI)	/* If rounded result is normal,don't */
|					|  subtract
	jmi 		ext_opc011
	subqw		#1,a0@(LOCAL_EX)	| account for denorm bias vs.
|					| normalized bias
|					|           normalized   denormalized
|					| single       0x7f           0x7e
|					| double       0x3ff          0x3fe
|
ext_opc011:
	bsrl		__x_store	| stores to memory
	jra 		unf_done	| finish up

|
| At this point, a store to a float register is pending
|
not_opc011:
	bsrl		__x_store	| stores to float register
|					| a0 is not corrupted on a store to a
|					| float register.
|
| Set the condition codes according to result
|
	tstl		a0@(LOCAL_HI)	| check upper mantissa
	jne 		ck_sgn
	tstl		a0@(LOCAL_LO)	| check lower mantissa
	jne 		ck_sgn
	bset		#z_bit,a6@(FPSR_CC) | set condition codes if zero
ck_sgn:
	btst 		#sign_bit,a0@(LOCAL_EX)	| check the sign bit
	jeq 		unf_done
	bset		#neg_bit,a6@(FPSR_CC)

|
| Finish.
|
unf_done:
	btst		#__x_inex2_bit,a6@(FPSR_EXCEPT)
	jeq 		no_aunfl
	bset		#aunfl_bit,a6@(FPSR_AEXCEPT)
no_aunfl:
	rts

|	end
