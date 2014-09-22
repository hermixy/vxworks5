/* sint.s - Motorola 68040 FP integer routines (EXC) */

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

	sintsa 3.1 12/10/90

	The entry point sINT computes the rounded integer
	equivalent of the input argument, sINTRZ computes
	the integer rounded to zero of the input argument.

	Entry points __x_sint and __x_sintrz are called from __x_do_func
	to emulate the fint and fintrz unimplemented instructions,
	respectively.  Entry point __x_sintdo is used by __x_bindec.

	Input: (Entry points __x_sint and __x_sintrz) Double-extended
		number X in the ETEMP space in the floating-point
		save stack.
	       (Entry point __x_sintdo) Double-extended number X in
		location pointed to by the address register a0.
	       (Entry point __x_sintd) Double-extended denormalized
		number X in the ETEMP space in the floating-point
		save stack.

	Output: The function returns int(X) or intrz(X) in fp0.

	Modifies: fp0.

	Algorithm: (sint and __x_sintrz)

	1. If exp(X) >= 63, return X.
	   If exp(X) < 0, return +/- 0 or +/- 1, according to
	   the rounding mode.

	2. (X is in range) set rsc = 63 - exp(X). Unnormalize the
	   result to the exponent 0x403e.

	3. Round the result in the mode given in USER_FPCR. For
	   __x_sintrz, force round-to-zero mode.

	4. Normalize the rounded result|  store in fp0.

	For the denormalized cases, force the correct result
	for the given sign and rounding mode.

		        Sign(X)
		RMODE   +    -
		-----  --------
		 RN    +0   -0
		 RZ    +0   -0
		 RM    +0   -1
		 RP    +1   -0


		Copyright (C) Motorola, Inc. 1990
			All Rights Reserved

	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA
	The copyright notice above does not evidence any
	actual or intended publication of such source code.

SINT    idnt    2,1 Motorola 040 Floating Point Software Package

	section	8

NOMANUAL
*/

#include "fpsp040E.h"

|	xref	__x_dnrm_lp
|	xref	__x_nrm_set
|	xref	__x_round
|	xref	__x_t_inx2
|	xref	__x_ld_pone
|	xref	__x_ld_mone
|	xref	__x_ld_pzero
|	xref	__x_ld_mzero
|	xref	__x_snzrinx

|
|	FINT
|

	.text

	.globl	__x_sint
__x_sint:
	bfextu	a6@(fpcr_MODE){#2:#2},d1	/* | use user's mode for rounding */
|					| implicity has extend precision
|					| in upper word.
	movel	d1,a6@(L_SCR1)		| save mode bits
	jra 	__x_sintexc

|
|	FINT with extended denorm inputs.
|
	.globl	__x_sintd
__x_sintd:
	btst	#5,a6@(fpcr_MODE)
	jeq 	__x_snzrinx		| if round nearest or round zero, +/- 0
	btst	#4,a6@(fpcr_MODE)
	jeq 	rnd_mns
rnd_pls:
	btst	#sign_bit,a0@(LOCAL_EX)
	jne 	__x_sintmz
	bsrl	__x_ld_pone		| if round plus inf & pos, answer is +1
	jra 	__x_t_inx2
rnd_mns:
	btst	#sign_bit,a0@(LOCAL_EX)
	jeq 	__x_sintpz
	bsrl	__x_ld_mone		| if round mns inf and neg, answer is -1
	jra 	__x_t_inx2
__x_sintpz:
	bsrl	__x_ld_pzero
	jra 	__x_t_inx2
__x_sintmz:
	bsrl	__x_ld_mzero
	jra 	__x_t_inx2

|
|	FINTRZ
|
	.globl	__x_sintrz
__x_sintrz:
	movel	#1,a6@(L_SCR1)		| use rz mode for rounding
|					| implicity has extend precision
|					| in upper word.
	jra 	__x_sintexc
|
|	SINTDO
|
|	Input:	a0 points to an IEEE extended format operand
| 	Output:	fp0 has the result
|
| Exeptions:
|
| If the subroutine results in an inexact operation, the inx2 and
| ainx bits in the USER_FPSR are set.
|
|
	.globl	__x_sintdo
__x_sintdo:
	bfextu	a6@(fpcr_MODE){#2:#2},d1	/* | use user's mode for rounding */
|					| implicitly has ext precision
|					| in upper word.
	movel	d1,a6@(L_SCR1)		| save mode bits
|
| Real work of __x_sint is in __x_sintexc
|
__x_sintexc:
	bclr	#sign_bit,a0@(LOCAL_EX)	| convert to internal extended
|					| format
	sne	a0@(LOCAL_SGN)
	cmpw	#0x403e,a0@(LOCAL_EX)	| check if (unbiased) exp > 63
	jgt 	out_rnge			| branch if exp < 63
	cmpw	#0x3ffd,a0@(LOCAL_EX)	| check if (unbiased) exp < 0
	jgt 	in_rnge			| if 63 >= exp > 0, do calc
|
| Input is less than zero.  Restore sign, and check for directed
| rounding modes.  L_SCR1 contains the rmode in the lower byte.
|
un_rnge:
	btst	#1,a6@(L_SCR1+3)		| check for rn and rz
	jeq 	un_rnrz
	tstb	a0@(LOCAL_SGN)		| check for sign
	jne 	un_rmrp_neg
|
| Sign is +.  If rp, load +1.0, if rm, load +0.0
|
	cmpib	#3,a6@(L_SCR1+3)		| check for rp
	jeq 	un_ldpone		| if rp, load +1.0
	bsrl	__x_ld_pzero		| if rm, load +0.0
	jra 	__x_t_inx2
un_ldpone:
	bsrl	__x_ld_pone
	jra 	__x_t_inx2
|
| Sign is -.  If rm, load -1.0, if rp, load -0.0
|
un_rmrp_neg:
	cmpib	#2,a6@(L_SCR1+3)		| check for rm
	jeq 	un_ldmone		| if rm, load -1.0
	bsrl	__x_ld_mzero		| if rp, load -0.0
	jra 	__x_t_inx2
un_ldmone:
	bsrl	__x_ld_mone
	jra 	__x_t_inx2
|
| Rmode is rn or rz|  return signed zero
|
un_rnrz:
	tstb	a0@(LOCAL_SGN)		| check for sign
	jne 	un_rnrz_neg
	bsrl	__x_ld_pzero
	jra 	__x_t_inx2
un_rnrz_neg:
	bsrl	__x_ld_mzero
	jra 	__x_t_inx2

|
| Input is greater than 2^63.  All bits are significant.  Return
| the input.
|
out_rnge:
	bfclr	a0@(LOCAL_SGN){#0:#8}	| change back to IEEE ext format
	jeq 	intps
	bset	#sign_bit,a0@(LOCAL_EX)
intps:
	fmovel	fpcr,a7@-
	fmovel	#0,fpcr
	fmovex	a0@(LOCAL_EX),fp0	| if exp > 63
|					| then return X to the user
|					| there are no fraction bits
	fmovel	a7@+,fpcr
	rts

in_rnge:
| 					| shift off fraction bits
	clrl	d0			| clear d0 - initial g,r,s for
|					| dnrm_lp
	movel	#0x403e,d1		| set threshold for __x_dnrm_lp
|					| assumes a0 points to operand
	bsrl	__x_dnrm_lp
|					| returns unnormalized number
|					| pointed by a0
|					| output d0 supplies g,r,s
|					| used by round
	movel	a6@(L_SCR1),d1		| use selected rounding mode
|
|
	bsrl	__x_round			| round the unnorm based on users
|					| input	a0 ptr to ext X
|					| 	d0 g,r,s bits
|					| 	d1 PREC/MODE info
|					| output a0 ptr to rounded result
|					| inexact flag set in USER_FPSR
|					| if initial grs set
|
| normalize the rounded result and store value in fp0
|
	bsrl	__x_nrm_set			| normalize the unnorm
|					| Input: a0 points to operand to
|					| be normalized
|					| Output: a0 points to normalized
|					| result
	bfclr	a0@(LOCAL_SGN){#0:#8}
	jeq 	nrmrndp
	bset	#sign_bit,a0@(LOCAL_EX)	| return to IEEE extended format
nrmrndp:
	fmovel	fpcr,a7@-
	fmovel	#0,fpcr
	fmovex	a0@(LOCAL_EX),fp0	| move result to fp0
	fmovel	a7@+,fpcr
	rts

|	end
