/* scale.s - Motorola 68040 FP scaling routines (EXC) */

/* Copyright 1991-1993 Wind River Systems, Inc. */
	.data
	.globl	_copyright_wind_river
	.long	_copyright_wind_river

/*
modification history
--------------------
01f,21jul93,kdl  added .text (SPR #2372).
01e,23aug92,jcf  changed bxxx to jxx.
01d,26may92,rrr  the tree shuffle
01c,10jan92,kdl  added modification history; general cleanup.
01b,30dec91,kdl  put in changes from Motorola v3.3 (FPSP v2.1):
	 	 clear the exceptional operand for zero underflow.
01a,15aug91,kdl  original version, from Motorola FPSP v2.0.
*/

/*
DESCRIPTION

	scalesa 3.2 2/18/91

	The entry point sSCALE computes the destination operand
	scaled by the source operand.  If the absoulute value of
	the source operand is (>= 2^14) an overflow or underflow
	is returned.

	The entry point __x_sscale is called from __x_do_func to emulate
	the fscale unimplemented instruction.

	Input: Double-extended destination operand in FPTEMP,
		double-extended source operand in ETEMP.

	Output: The function returns scale(X,Y) to fp0.

	Modifies: fp0.

	Algorithm:

		Copyright (C) Motorola, Inc. 1990
			All Rights Reserved

	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA
	The copyright notice above does not evidence any
	actual or intended publication of such source code.

SCALE    idnt    2,1 Motorola 040 Floating Point Software Package

	section	8

NOMANUAL
*/

#include "fpsp040E.h"

|	xref	__x_t_ovfl2
|	xref	__x_t_unfl
|	xref	__x_round
|	xref	__x_t_resdnrm

SRC_BNDS: .word	0x3fff,0x400c

|
| This entry point is used by the unimplemented instruction exception
| handler.
|
|
|
|	FSCALE
|

	.text

	.globl	__x_sscale
__x_sscale:
	fmovel		#0,fpcr		| clr user enabled exc
	clrl		d1
	movew		a6@(FPTEMP),d1	| get dest exponent
	smi		a6@(L_SCR1)	| use L_SCR1 to hold sign
	andil		#0x7fff,d1	| strip sign
	movew		a6@(ETEMP),d0	| check src bounds
	andiw		#0x7fff,d0	| clr sign bit
	cmp2w		SRC_BNDS,d0
	jcc 		src_in
	cmpiw		#0x400c,d0	| test for too large
	jge 		src_out
|
| The source input is below 1, so we check for denormalized numbers
| and set unfl.
|
src_small:
	moveb		a6@(DTAG),d0
	andib		#0xe0,d0
	tstb		d0
	jeq 		no_denorm
	st		a6@(STORE_FLG)	| dest already contains result
	orl		#__x_unfl_mask,a6@(USER_FPSR) | set UNFL
den_done:
	lea		a6@(FPTEMP),a0
	jra 		__x_t_resdnrm
no_denorm:
	fmovel		a6@(USER_FPCR),fpcr
	fmovex		a6@(FPTEMP),fp0	| simply return dest
	rts


|
| Source is within 2^14 range.  To perform the int operation,
| move it to d0.
|
src_in:
	fmovex		a6@(ETEMP),fp0	| move in src for int
	fmovel		#rz_mode,fpcr	| force rz for src conversion
	fmovel		fp0,d0		| int src to d0
	fmovel		#0,FPSR		| clr status from above
	tstw		a6@(ETEMP)	| check src sign
	jlt 		src_neg
|
| Source is positive.  Add the src to the dest exponent.
| The result can be denormalized, if src = 0, or overflow,
| if the result of the add sets a bit in the upper word.
|
src_pos:
	tstw		d1		| check for denorm
	jeq 		dst_dnrm
	addl		d0,d1		| add src to dest exp
	jeq 		__x_denorm	| if zero, result is denorm
	cmpil		#0x7fff,d1	| test for overflow
	jge 		__x_ovfl
	tstb		a6@(L_SCR1)
	jeq 		spos_pos
	orw		#0x8000,d1
spos_pos:
	movew		d1,a6@(FPTEMP)	| result in FPTEMP
	fmovel		a6@(USER_FPCR),fpcr
	fmovex		a6@(FPTEMP),fp0	| write result to fp0
	rts
__x_ovfl:
	tstb		a6@(L_SCR1)
	jeq 		sovl_pos
	orw		#0x8000,d1
sovl_pos:
	movew		a6@(FPTEMP),a6@(ETEMP)	| result in ETEMP
	movel		a6@(FPTEMP_HI),a6@(ETEMP_HI)
	movel		a6@(FPTEMP_LO),a6@(ETEMP_LO)
	jra 		__x_t_ovfl2

__x_denorm:
	tstb		a6@(L_SCR1)
	jeq 		den_pos
	orw		#0x8000,d1
den_pos:
	tstl		a6@(FPTEMP_HI)	| check j bit
	jlt 		nden_exit	| if set, not denorm
	movew		d1,a6@(ETEMP)	| input expected in ETEMP
	movel		a6@(FPTEMP_HI),a6@(ETEMP_HI)
	movel		a6@(FPTEMP_LO),a6@(ETEMP_LO)
	orl		#__x_unfl_bit,a6@(USER_FPSR)	| set unfl
	lea		a6@(ETEMP),a0
	jra 		__x_t_resdnrm
nden_exit:
	movew		d1,a6@(FPTEMP)	| result in FPTEMP
	fmovel		a6@(USER_FPCR),fpcr
	fmovex		a6@(FPTEMP),fp0	| write result to fp0
	rts

|
| Source is negative.  Add the src to the dest exponent.
| (The result exponent will be reduced).  The result can be
| denormalized.
|
src_neg:
	addl		d0,d1		| add src to dest
	jeq 		__x_denorm	| if zero, result is denorm
	jlt 		fix_dnrm	| if negative, result is
|					| needing denormalization
	tstb		a6@(L_SCR1)
	jeq 		__x_sneg_pos
	orw		#0x8000,d1
__x_sneg_pos:
	movew		d1,a6@(FPTEMP)	| result in FPTEMP
	fmovel		a6@(USER_FPCR),fpcr
	fmovex		a6@(FPTEMP),fp0	| write result to fp0
	rts


|
| The result exponent is below denorm value.  Test for catastrophic
| underflow and force zero if true.  If not, try to shift the
| mantissa right until a zero exponent exists.
|
fix_dnrm:
	cmpiw		#0xffc0,d1	| lower bound for normalization
	jlt 		fix_unfl	| if lower, catastrophic unfl
	movew		d1,d0		| use d0 for exp
	movel		d2,a7@-	| free d2 for norm
	movel		a6@(FPTEMP_HI),d1
	movel		a6@(FPTEMP_LO),d2
	clrl		a6@(L_SCR2)
fix_loop:
	addw		#1,d0		| drive d0 to 0
	lsrl		#1,d1		| while shifting the
	roxrl		#1,d2		| mantissa to the right
	jcc 		no_carry
	st		a6@(L_SCR2)	| use L_SCR2 to capture inex
no_carry:
	tstw		d0		| it is finished when
	jlt 		fix_loop	| d0 is zero or the mantissa
	tstb		a6@(L_SCR2)
	jeq 		tst_zero
	orl		#__x_unfl_inx_mask,a6@(USER_FPSR)
|					| set unfl, aunfl, ainex
|
| Test for zero. If zero, simply use fmovel to return +/- zero
| to the fpu.
|
tst_zero:
	clrw		a6@(FPTEMP_EX)
	tstb		a6@(L_SCR1)	| test for sign
	jeq 		tst_con
	orw		#0x8000,a6@(FPTEMP_EX) | set sign bit
tst_con:
	movel		d1,a6@(FPTEMP_HI)
	movel		d2,a6@(FPTEMP_LO)
	movel		a7@+,d2
	tstl		d1
	jne 		not_zero
	tstl		a6@(FPTEMP_LO)
	jne 		not_zero
|
| Result is zero.  Check for rounding mode to set lsb.  If the
| mode is rp, and the zero is positive, return smallest denorm.
| If the mode is rm, and the zero is negative, return smallest
| negative denorm.
|
	btst		#5,a6@(fpcr_MODE) | test if rm or rp
	jeq 		no_dir
	btst		#4,a6@(fpcr_MODE) | check which one
	jeq 		zer_rm
zer_rp:
	tstb		a6@(L_SCR1)	| check sign
	jne 		no_dir		| if set, neg op, no inc
	movel		#1,a6@(FPTEMP_LO) | set lsb
	jra 		sm_dnrm
zer_rm:
	tstb		a6@(L_SCR1)	| check sign
	jeq 		no_dir		| if clr, neg op, no inc
	movel		#1,a6@(FPTEMP_LO) | set lsb
	orl		#neg_mask,a6@(USER_FPSR) | set N
	jra 		sm_dnrm
no_dir:
	fmovel		a6@(USER_FPCR),fpcr
	fmovex		a6@(FPTEMP),fp0	/* | use fmovel to set cc's */
	rts

|
| The rounding mode changed the zero to a smallest denorm. Call
| __x_t_resdnrm with exceptional operand in ETEMP.
|
sm_dnrm:
	movel		a6@(FPTEMP_EX),a6@(ETEMP_EX)
	movel		a6@(FPTEMP_HI),a6@(ETEMP_HI)
	movel		a6@(FPTEMP_LO),a6@(ETEMP_LO)
	lea		a6@(ETEMP),a0
	jra 		__x_t_resdnrm

|
| Result is still denormalized.
|
not_zero:
	orl		#__x_unfl_mask,a6@(USER_FPSR) | set unfl
	tstb		a6@(L_SCR1)	| check for sign
	jeq 		fix_exit
	orl		#neg_mask,a6@(USER_FPSR) | set N
fix_exit:
	jra 		sm_dnrm


|
| The result has underflowed to zero. Return zero and set
| unfl, aunfl, and ainex.
|
fix_unfl:
	orl		#__x_unfl_inx_mask,a6@(USER_FPSR)
	btst		#5,a6@(fpcr_MODE) | test if rm or rp
	jeq 		no_dir2
	btst		#4,a6@(fpcr_MODE) | check which one
	jeq 		zer_rm2
zer_rp2:
	tstb		a6@(L_SCR1)	| check sign
	jne 		no_dir2		| if set, neg op, no inc
	clrl		a6@(FPTEMP_EX)
	clrl		a6@(FPTEMP_HI)
	movel		#1,a6@(FPTEMP_LO) | set lsb
	jra 		sm_dnrm		| return smallest denorm
zer_rm2:
	tstb		a6@(L_SCR1)	| check sign
	jeq 		no_dir2		| if clr, neg op, no inc
	movew		#0x8000,a6@(FPTEMP_EX)
	clrl		a6@(FPTEMP_HI)
	movel		#1,a6@(FPTEMP_LO) | set lsb
	orl		#neg_mask,a6@(USER_FPSR) | set N
	jra 		sm_dnrm		| return smallest denorm

no_dir2:
	tstb		a6@(L_SCR1)
	jge 		pos_zero
neg_zero:
	clrl		a6@(FP_SCR1)	| clear the exceptional operand
	clrl		a6@(FP_SCR1+4)	| for __x_gen_except.
	clrl		a6@(FP_SCR1+8)
	.long 0xf23c4400,0x80000000	/*  fmoves  &0x80000000,fp0 */
	rts
pos_zero:
	clrl		a6@(FP_SCR1)	| clear the exceptional operand
	clrl		a6@(FP_SCR1+4)	| for __x_gen_except.
	clrl		a6@(FP_SCR1+8)
	.long 0xf23c4400,0x00000000	/*  fmoves  &0x00000000,fp0 */
	rts

|
| The destination is a denormalized number.  It must be handled
| by first shifting the bits in the mantissa until it is normalized,
| then adding the remainder of the source to the exponent.
|
dst_dnrm:
	moveml		d2/d3,a7@-
	movew		a6@(FPTEMP_EX),d1
	movel		a6@(FPTEMP_HI),d2
	movel		a6@(FPTEMP_LO),d3
dst_loop:
	tstl		d2		| test for normalized result
	jlt 		dst_norm	| exit loop if so
	tstl		d0		| otherwise, test shift count
	jeq 		dst_fin		| if zero, shifting is done
	subil		#1,d0		| dec src
	lsll		#1,d3
	roxll		#1,d2
	jra 		dst_loop
|
| Destination became normalized.  Simply add the remaining
| portion of the src to the exponent.
|
dst_norm:
	addw		d0,d1		| dst is normalized|  add src
	tstb		a6@(L_SCR1)
	jeq 		dnrm_pos
	orl		#0x8000,d1
dnrm_pos:
	movemw		d1,a6@(FPTEMP_EX)
	moveml		d2,a6@(FPTEMP_HI)
	moveml		d3,a6@(FPTEMP_LO)
	fmovel		a6@(USER_FPCR),fpcr
	fmovex		a6@(FPTEMP),fp0
	moveml		a7@+,d2/d3
	rts

|
| Destination remained denormalized.  Call t_excdnrm with
| exceptional operand in ETEMP.
|
dst_fin:
	tstb		a6@(L_SCR1)	| check for sign
	jeq 		dst_exit
	orl		#neg_mask,a6@(USER_FPSR) | set N
	orl		#0x8000,d1
dst_exit:
	movemw		d1,a6@(ETEMP_EX)
	moveml		d2,a6@(ETEMP_HI)
	moveml		d3,a6@(ETEMP_LO)
	orl		#__x_unfl_mask,a6@(USER_FPSR) | set unfl
	moveml		a7@+,d2/d3
	lea		a6@(ETEMP),a0
	jra 		__x_t_resdnrm

|
| Source is outside of 2^14 range.  Test the sign and branch
| to the appropriate exception handler.
|
src_out:
	tstb		a6@(L_SCR1)
	jeq 		scro_pos
	orl		#0x8000,d1
scro_pos:
	movel		a6@(FPTEMP_HI),a6@(ETEMP_HI)
	movel		a6@(FPTEMP_LO),a6@(ETEMP_LO)
	tstw		a6@(ETEMP)
	jlt 		res_neg
res_pos:
	movew		d1,a6@(ETEMP)	| result in ETEMP
	jra 		__x_t_ovfl2
res_neg:
	movew		d1,a6@(ETEMP)	| result in ETEMP
	lea		a6@(ETEMP),a0
	jra 		__x_t_unfl
|	end
