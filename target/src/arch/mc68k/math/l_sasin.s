/* sasin.s - Motorola 68040 FP routines (LIB) */

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

	sasinsa 3.3 12/19/90

	Description: The entry point sAsin computes the inverse sine of
		an input argument|  sAsind does the same except for denormalized
		input.

	Input: Double-extended number X in location pointed to
		by address register a0.

	Output: The value arcsin(X) returned in floating-point register Fp0.

	Accuracy and Monotonicity: The returned result is within 3 ulps in
		64 significant bit, i.e. within 0.5001 ulp to 53 bits if the
		result is subsequently rounded to double precision. The
		result is provably monotonic in double precision.

	Speed: The program sASIN takes approximately 310 cycles.

	Algorithm:

	ASIN
	1. If |X| >= 1, go to 3.

	2. (|X| < 1) Calculate asin(X) by
		z := sqrt( [1-X][1+X] )
		asin(X) = atan( x / z ).
		Exit.

	3. If |X| > 1, go to 5.

	4. (|X| = 1) sgn := sign(X), return asin(X) := sgn * Pi/2. Exit.

	5. (|X| > 1) Generate an invalid operation by 0 * infinity.
		Exit.


		Copyright (C) Motorola, Inc. 1990
			All Rights Reserved

	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA
	The copyright notice above does not evidence any
	actual or intended publication of such source code.

SASIN	idnt	2,1 Motorola 040 Floating Point Software Package

	section	8

NOMANUAL
*/

PIBY2:	.long 0x3FFF0000,0xC90FDAA2,0x2168C235,0x00000000

|	xref	__l_t_operr
|	xref	__l_t_frcinx
|	xref	__l_t_extdnrm
|	xref	__l_satan


	.text

	.globl	__l_sasind
__l_sasind:
|--ASIN(X) = X FOR DENORMALIZED X

	jra 		__l_t_extdnrm

	.globl	__l_sasin
__l_sasin:
	fmovex		a0@,fp0	|...lOAD INPUT

	movel		a0@,d0
	movew		a0@(4),d0
	andil		#0x7FFFFFFF,d0
	cmpil		#0x3FFF8000,d0
	jge 		asinbig

|--THIS IS THE USUAL CASE, |X| < 1
|--ASIN(X) = ATAN( X / SQRT( (1-X)(1+X) ) )

/*	fmoves	&0x3F800000,fp1 */	 .long 0xf23c4480,0x3f800000
	fsubx		fp0,fp1		|...1-X
	fmovemx	fp2-fp2,a7@-
/*	fmoves	&0x3F800000,fp2 */	 .long 0xf23c4500,0x3f800000
	faddx		fp0,fp2		|...1+X
	fmulx		fp2,fp1		|...(1+X)(1-X)
	fmovemx	a7@+,fp2-fp2
	fsqrtx		fp1		|...SQRT([1-X][1+X])
	fdivx		fp1,fp0	 	|...X/SQRT([1-X][1+X])
	fmovemx	fp0-fp0,a0@
	bsrl		__l_satan
	jra 		__l_t_frcinx

asinbig:
	fabsx		fp0,fp0	 |...|X|
/*	fcmps	&0x3F800000,fp0 */	 .long 0xf23c4438,0x3f800000
	fbgtl		__l_t_operr		| cause an operr exception

|--|X| = 1, ASIN(X) = +- PI/2.

	fmovex		PIBY2,fp0
	movel		a0@,d0
	andil		#0x80000000,d0	|...SIGN BIT OF X
	oril		#0x3F800000,d0	|...+-1 IN SGL FORMAT
	movel		d0,a7@-	|...push SIGN(X) IN SGL-FMT
	fmovel		d1,fpcr
	fmuls		a7@+,fp0
	jra 		__l_t_frcinx

|	end
