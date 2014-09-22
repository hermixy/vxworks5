/* x_ovfl.s - Motorola 68040 FP overflow exception handler (EXC) */

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

	x_ovflsa 3.4 4/26/91

	fpsp_ovfl --- FPSP handler for overflow exception

	Overflow occurs when a floating-point intermediate result is
	too large to be represented in a floating-point data register,
	or when storing to memory, the contents of a floating-point
	data register are too large to be represented in the
	destination format.

 Trap disabled results

 If the instruction is move_out, then garbage is stored in the
 destination.  If the instruction is not move_out, then the
 destination is not affected.  For 68881 compatibility, the
 following values should be stored at the destination, based
 on the current rounding mode:

  RN	Infinity with the sign of the intermediate result.
  RZ	Largest magnitude number, with the sign of the
	intermediate result.
  RM   For pos overflow, the largest pos number. For neg overflow,
	-infinity
  RP   For pos overflow, +infinity. For neg overflow, the largest
	neg number

 Trap enabled results
 All trap disabled code applies.  In addition the exceptional
 operand needs to be made available to the users exception handler
 with a bias of 0x6000 subtracted from the exponent.



		Copyright (C) Motorola, Inc. 1990
			All Rights Reserved

	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA
	The copyright notice above does not evidence any
	actual or intended publication of such source code.

X_OVFL	idnt    2,1 Motorola 040 Floating Point Software Package

	section	8

NOMANUAL
*/

#include "fpsp040E.h"

|	xref	__x_ovf_r_x2
|	xref	__x_ovf_r_x3
|	xref	__x_store
|	xref	__x_real_ovfl
|	xref	__x_real_inex
|	xref	__x_fpsp_done
|	xref	__x_g_opcls
|	xref	__x_b1238_fix
|	xref	__x_check_force


	.text

	.globl	__x_fpsp_ovfl
__x_fpsp_ovfl:
	link	a6,#-LOCAL_SIZE
	fsave	a7@-
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx	fp0-fp3,a6@(USER_FP0)
	fmoveml	fpcr/fpsr/fpi,a6@(USER_FPCR)

| At this point we need to look at the instructions and see if it is one of
| the force-precision ones (fsadd,fdadd,fssub,fdsub,fsmul,fdmul,fsdiv,fddiv,
| fssqrt,fdsqrt,fsmove,fdmove,fsabs,fdabs,fsneg,fdneg).  If it is then
| correct the USER_FPCR to the instruction rounding precision (s or d).
| Also, we need to check if the instruction is fsgldiv or fsglmul.  If it
| is then the USER_FPCR is set to extended rounding precision.  Otherwise
| leave the USER_FPCR alone.
|	bsrl		__x_check_force

|
/* 	The 040 doesn't set the AINEX bit in the FPSR, the following */
|	line temporarily rectifies this error.
|
	bset	#ainex_bit,a6@(FPSR_AEXCEPT)
|
	bsrl	ovf_adj		| denormalize, round # store interm op
|
|	if overflow traps not enabled check for inexact exception
|
	btst	#__x_ovfl_bit,a6@(fpcr_ENABLE)
	jeq 	ck_inex
|
	btst	#E3,a6@(E_BYTE)
	jeq 	no_e3_1
	bfextu	a6@(CMDREG3B){#6:#3},d0	| get dest reg no
	bclr	d0,a6@(FPR_DIRTY_BITS)	| clr dest dirty bit
	bsrl	__x_b1238_fix
	movel	a6@(USER_FPSR),a6@(FPSR_SHADOW)
	orl	#sx_mask,a6@(E_BYTE)
no_e3_1:
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP0),fp0-fp3
	fmoveml	a6@(USER_FPCR),fpcr/fpsr/fpi
	frestore a7@+
	unlk	a6
	jra 	__x_real_ovfl
|
| It is possible to have either inex2 or inex1 exceptions with the
| ovfl.  If the inex enable bit is set in the fpcr, and either
| inex2 or inex1 occured, we must clean up and branch to the
| real inex handler.
|
ck_inex:
|	moveb	a6@(fpcr_ENABLE),d0
|	andb	a6@(FPSR_EXCEPT),d0
|	andib	#0x3,d0
	btst	#__x_inex2_bit,a6@(fpcr_ENABLE)
	jeq 	__x_ovfl_exit
|
| Inexact enabled and reported, and we must take an __x_inexact exception.
|
take_inex:
	btst	#E3,a6@(E_BYTE)
	jeq 	no_e3_2
	bfextu	a6@(CMDREG3B){#6:#3},d0	| get dest reg no
	bclr	d0,a6@(FPR_DIRTY_BITS)	| clr dest dirty bit
	bsrl	__x_b1238_fix
	movel	a6@(USER_FPSR),a6@(FPSR_SHADOW)
	orl	#sx_mask,a6@(E_BYTE)
no_e3_2:
	moveb	#INEX_VEC,a6@(EXC_VEC+1)
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP0),fp0-fp3
	fmoveml	a6@(USER_FPCR),fpcr/fpsr/fpi
	frestore a7@+
	unlk	a6
	jra 	__x_real_inex

__x_ovfl_exit:
	bclr	#E3,a6@(E_BYTE)		| test and clear E3 bit
	jeq 	e1_set
|
| Clear dirty bit on dest resister in the frame before branching
| to __x_b1238_fix.
|
	bfextu	a6@(CMDREG3B){#6:#3},d0	| get dest reg no
	bclr	d0,a6@(FPR_DIRTY_BITS)	| clr dest dirty bit
	bsrl	__x_b1238_fix		| test for bug1238 case

	movel	a6@(USER_FPSR),a6@(FPSR_SHADOW)
	orl	#sx_mask,a6@(E_BYTE)
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP0),fp0-fp3
	fmoveml	a6@(USER_FPCR),fpcr/fpsr/fpi
	frestore a7@+
	unlk	a6
	jra 	__x_fpsp_done
e1_set:
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP0),fp0-fp3
	fmoveml	a6@(USER_FPCR),fpcr/fpsr/fpi
	unlk	a6
	jra 	__x_fpsp_done

|
|	ovf_adj
|
ovf_adj:
|
| Have a0 point to the correct operand.
|
	btst	#E3,a6@(E_BYTE)	| test E3 bit
	jeq 	ovf_e1

	lea	a6@(WBTEMP),a0
	jra 	ovf_com
ovf_e1:
	lea	a6@(ETEMP),a0

ovf_com:
	bclr	#sign_bit,a0@(LOCAL_EX)
	sne	a0@(LOCAL_SGN)

	bsrl	__x_g_opcls	| returns opclass in d0
	cmpiw	#3,d0		| check for opclass3
	jne 	not_opc011

|
| FPSR_CC is saved and restored because __x_ovf_r_x3 affects it. The
/* | CCs are defined to be 'not affected' for the opclass3 instruction. */
|
	moveb	a6@(FPSR_CC),a6@(L_SCR1)
 	bsrl	__x_ovf_r_x3	| returns a0 pointing to result
	moveb	a6@(L_SCR1),a6@(FPSR_CC)
	jra 	__x_store	| stores to memory or register

not_opc011:
	bsrl	__x_ovf_r_x2	| returns a0 pointing to result
	jra 	__x_store	| stores to memory or register

|	end
