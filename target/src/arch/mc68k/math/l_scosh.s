/* l_scosh - Motorola 68040 FP hyperbolic cosine routines (LIB) */

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
01b,09jan92,kdl  added modification history; general cleanup.
01a,15aug91,kdl  original version, from Motorola FPSP v2.0.
*/

/*
DESCRIPTION


	scoshsa 3.1 12/10/90

	The entry point sCosh computes the hyperbolic cosine of
	an input argument|  sCoshd does the same except for denormalized
	input.

	Input: Double-extended number X in location pointed to
		by address register a0.

	Output: The value cosh(X) returned in floating-point register Fp0.

	Accuracy and Monotonicity: The returned result is within 3 ulps in
		64 significant bit, i.e. within 0.5001 ulp to 53 bits if the
		result is subsequently rounded to double precision. The
		result is provably monotonic in double precision.

	Speed: The program sCOSH takes approximately 250 cycles.

	Algorithm:

	COSH
	1. If |X| > 16380 log2, go to 3.

	2. (|X| <= 16380 log2) Cosh(X) is obtained by the formulae
		y = |X|, z = exp(Y), and
		cosh(X) = (1/2)*( z + 1/z ).
		Exit.

	3. (|X| > 16380 log2). If |X| > 16480 log2, go to 5.

	4. (16380 log2 < |X| <= 16480 log2)
		cosh(X) = sign(X) * exp(|X|)/2.
		However, invoking exp(|X|) may cause premature overflow.
		Thus, we calculate sinh(X) as follows:
		Y	:= |X|
		Fact	:=	2**(16380)
		Y'	:= Y - 16381 log2
		cosh(X) := Fact * exp(Y').
		Exit.

	5. (|X| > 16480 log2) sinh(X) must overflow. Return
		Huge*Huge to generate overflow and an infinity with
		the appropriate sign. Huge is the largest finite number in
		extended format. Exit.



		Copyright (C) Motorola, Inc. 1990
			All Rights Reserved

	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA
	The copyright notice above does not evidence any
	actual or intended publication of such source code.

SCOSH	idnt	2,1 Motorola 040 Floating Point Software Package

	section	8

NOMANUAL
*/

|	xref	__l_t_ovfl
|	xref	__l_t_frcinx
|	xref	__l_setox

T1:	.long 0x40C62D38,0xD3D64634 |... 16381 LOG2 LEAD
T2:	.long 0x3D6F90AE,0xB1E75CC7 |... 16381 LOG2 TRAIL

TWO16380: .long 0x7FFB0000,0x80000000,0x00000000,0x00000000


	.text

	.globl	__l_scoshd
__l_scoshd:
|--COSH(X) = 1 FOR DENORMALIZED X

	.long 0xf23c4400,0x3f800000	/*	fmoves	&0x3F800000,fp0 */

	fmovel		d1,fpcr
	.long 0xf23c4422,0x00800000	/*	fadds	&0x00800000,fp0 */
	jra 		__l_t_frcinx

	.globl	__l_scosh
__l_scosh:
	fmovex		a0@,fp0	|...lOAD INPUT

	movel		a0@,d0
	movew		a0@(4),d0
	andil		#0x7FFFFFFF,d0
	cmpil		#0x400CB167,d0
	jgt 		COSHBIG

|--THIS IS THE USUAL CASE, |X| < 16380 LOG2
|--COSH(X) = (1/2) * ( EXP(X) + 1/EXP(X) )

	fabsx		fp0,fp0		|...|X|

	movel		d1,a7@-
	clrl		d1
	fmovemx	fp0-fp0,a0@	| pass parameter to __l_setox
	bsrl		__l_setox		|...FP0 IS EXP(|X|)
	.long 0xf23c4423,0x3f000000	/*	fmuls	&0x3F000000,fp0 */
	movel		a7@+,d1

	.long 0xf23c4480,0x3e800000	/*	fmoves	&0x3E800000,fp1 */
	fdivx		fp0,fp1	 	|...1/(2 EXP(|X|))

	fmovel		d1,fpcr
	faddx		fp1,fp0

	jra 		__l_t_frcinx

COSHBIG:
	cmpil		#0x400CB2B3,d0
	jgt 		COSHHUGE

	fabsx		fp0,fp0
	fsubd		pc@(T1),fp0		|...(|X|-16381LOG2_LEAD)
	fsubd		pc@(T2),fp0		|...|X| - 16381 LOG2, ACCURATE

	movel		d1,a7@-
	clrl		d1
	fmovemx	fp0-fp0,a0@
	bsrl		__l_setox
	fmovel		a7@+,fpcr

	fmulx		pc@(TWO16380),fp0
	jra 		__l_t_frcinx

COSHHUGE:
	fmovel		#0,fpsr		| clr N bit if set by source
	bclr		#7,a0@		| always return positive value
	fmovemx	a0@,fp0-fp0
	jra 		__l_t_ovfl

|	end
