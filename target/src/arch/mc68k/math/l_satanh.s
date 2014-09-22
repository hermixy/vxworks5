/* l_satanh.s - Motorola 68040 FP hyperbolic arc-tangent routines (LIB) */

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

	satanhsa 3.3 12/19/90

	The entry point __l_satanh computes the inverse
	hyperbolic tangent of
	an input argument|  __l_satanhd does the same except for denormalized
	input.

	Input: Double-extended number X in location pointed to
		by address register a0.

	Output: The value arctanh(X) returned in floating-point register Fp0.

	Accuracy and Monotonicity: The returned result is within 3 ulps in
		64 significant bit, i.e. within 0.5001 ulp to 53 bits if the
		result is subsequently rounded to double precision. The
		result is provably monotonic in double precision.

	Speed: The program __l_satanh takes approximately 270 cycles.

	Algorithm:

	ATANH
	1. If |X| >= 1, go to 3.

	2. (|X| < 1) Calculate atanh(X) by
		sgn := sign(X)
		y := |X|
		z := 2y/(1-y)
		atanh(X) := sgn * (1/2) * logp1(z)
		Exit.

	3. If |X| > 1, go to 5.

	4. (|X| = 1) Generate infinity with an appropriate sign and
		divide-by-zero by
		sgn := sign(X)
		atan(X) := sgn / (+0).
		Exit.

	5. (|X| > 1) Generate an invalid operation by 0 * infinity.
		Exit.


		Copyright (C) Motorola, Inc. 1990
			All Rights Reserved

	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA
	The copyright notice above does not evidence any
	actual or intended publication of such source code.

__l_satanh	IDNT	2,1 Motorola 040 Floating Point Software Package

	section	8

NOMANUAL
*/

|	xref	__l_t_dz
|	xref	__l_t_operr
|	xref	__l_t_frcinx
|	xref	__l_t_extdnrm
|	xref	__l_slognp1


	.text

	.globl	__l_satanhd
__l_satanhd:
|--ATANH(X) = X FOR DENORMALIZED X

	jra 		__l_t_extdnrm

	.globl	__l_satanh
__l_satanh:
	movel		a0@,d0
	movew		a0@(4),d0
	andil		#0x7FFFFFFF,d0
	cmpil		#0x3FFF8000,d0
	jge 		ATANHBIG

|--THIS IS THE USUAL CASE, |X| < 1
|--Y = |X|, Z = 2Y/(1-Y), ATANH(X) = SIGN(X) * (1/2) * LOG1P(Z).

	fabsx		a0@,fp0	|...Y = |X|
	fmovex		fp0,fp1
	fnegx		fp1		|...-Y
	faddx		fp0,fp0		|...2Y
/*	fadds	&0x3F800000,fp1 */	 .long 0xf23c44a2,0x3f800000
	fdivx		fp1,fp0		|...2Y/(1-Y)
	movel		a0@,d0
	andil		#0x80000000,d0
	oril		#0x3F000000,d0	|...SIGN(X)*HALF
	movel		d0,a7@-

	fmovemx	fp0-fp0,a0@	|...overwrite input
	movel		d1,a7@-
	clrl		d1
	bsrl		__l_slognp1		|...lOG1P(Z)
	fmovel		a7@+,fpcr
	fmuls		a7@+,fp0
	jra 		__l_t_frcinx

ATANHBIG:
	fabsx		a0@,fp0	|...|X|
/*	fcmps	&0x3F800000,fp0 */	 .long 0xf23c4438,0x3f800000
	fbgtl		__l_t_operr
	jra 		__l_t_dz

|	end
