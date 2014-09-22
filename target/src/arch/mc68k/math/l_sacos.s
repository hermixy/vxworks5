/* l_sacos.s - Motorola 68040 FP arc-cosine routines (LIB) */

/* Copyright 1991-1993 Wind River Systems, Inc. */
	.data
	.globl	_copyright_wind_river
	.long	_copyright_wind_river

/*
modification history
--------------------
01f,14jun95,tpr  changed fbxx to fbxxl.
01e,21jul93,kdl  added .text (SPR #2372).
01d,23aug92,jcf  changed bxxx to jxx.
01c,26may92,rrr  the tree shuffle
01b,09jan92,kdl  added modification history; general cleanup.
01a,15aug91,kdl  original version, from Motorola FPSP v2.0.
*/

/*
DESCRIPTION


	sacossa 3.3 12/19/90

	Description: The entry point sAcos computes the inverse cosine of
		an input argument|  sAcosd does the same except for denormalized
		input.

	Input: Double-extended number X in location pointed to
		by address register a0.

	Output: The value arccos(X) returned in floating-point register Fp0.

	Accuracy and Monotonicity: The returned result is within 3 ulps in
		64 significant bit, i.e. within 0.5001 ulp to 53 bits if the
		result is subsequently rounded to double precision. The
		result is provably monotonic in double precision.

	Speed: The program sCOS takes approximately 310 cycles.

	Algorithm:

	ACOS
	1. If |X| >= 1, go to 3.

	2. (|X| < 1) Calculate acos(X) by
		z := (1-X) / (1+X)
		acos(X) = 2 * atan( sqrt(z) ).
		Exit.

	3. If |X| > 1, go to 5.

	4. (|X| = 1) If X > 0, return 0. Otherwise, return Pi. Exit.

	5. (|X| > 1) Generate an invalid operation by 0 * infinity.
		Exit.


		Copyright (C) Motorola, Inc. 1990
			All Rights Reserved

	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA
	The copyright notice above does not evidence any
	actual or intended publication of such source code.

SACOS	idnt	2,1 Motorola 040 Floating Point Software Package

	section	8

NOMANUAL
*/

PI:	.long 0x40000000,0xC90FDAA2,0x2168C235,0x00000000
PIBY2:	.long 0x3FFF0000,0xC90FDAA2,0x2168C235,0x00000000

|	xref	__l_t_operr
|	xref	__l_t_frcinx
|	xref	__l_satan


	.text

	.globl	__l_sacosd
__l_sacosd:
|--ACOS(X) = PI/2 FOR DENORMALIZED X
	fmovel		d1,fpcr		/* |...load user's rounding mode/precision */
	fmovex		PIBY2,fp0
	jra 		__l_t_frcinx

	.globl	__l_sacos
__l_sacos:
	fmovex		a0@,fp0	|...lOAD INPUT

	movel		a0@,d0		|...pack exponent with upper 16 fraction
	movew		a0@(4),d0
	andil		#0x7FFFFFFF,d0
	cmpil		#0x3FFF8000,d0
	jge 		ACOSBIG

|--THIS IS THE USUAL CASE, |X| < 1
|--ACOS(X) = 2 * ATAN(	SQRT( (1-X)/(1+X) )	)

/*	fmoves	&0x3F800000,fp1 */	 .long 0xf23c4480,0x3f800000
	faddx		fp0,fp1	 	|...1+X
	fnegx		fp0	 	|... -X
/*	fadds	&0x3F800000,fp0 */	 .long 0xf23c4422,0x3f800000
	fdivx		fp1,fp0	 	|...(1-X)/(1+X)
	fsqrtx		fp0		|...SQRT((1-X)/(1+X))
	fmovemx	fp0-fp0,a0@	|...overwrite input
	movel		d1,a7@-	| save original users fpcr
	clrl		d1
	bsrl		__l_satan		|...ATAN(SQRT([1-X]/[1+X]))
	fmovel		a7@+,fpcr	| restore users exceptions
	faddx		fp0,fp0	 	|...2 * ATAN( STUFF )
	jra 		__l_t_frcinx

ACOSBIG:
	fabsx		fp0,fp0
/*	fcmps	&0x3F800000,fp0 */	 .long 0xf23c4438,0x3f800000
	fbgtl		__l_t_operr		| cause an operr exception

|--|X| = 1, ACOS(X) = 0 OR PI
	movel		a0@,d0		|...pack exponent with upper 16 fraction
	movew		a0@(4),d0
	cmpl		#0,d0		| D0 has original exponent+fraction
	jgt 		ACOSP1

|--X = -1
|Returns PI and inexact exception
	fmovex		PI,fp0
	fmovel		d1,fpcr
/*	fadds	&0x00800000,fp0 */	 .long 0xf23c4422,0x00800000
					| cause an inexact exception to be put
|					| into the 040 - will not trap until next
|					| fp inst.
	jra 		__l_t_frcinx

ACOSP1:
	fmovel		d1,fpcr
/*	fmoves	&0x00000000,fp0 */	 .long 0xf23c4400,0x00000000
	rts				| Facos of +1 is exact

|	end
