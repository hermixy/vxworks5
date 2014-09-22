/* stanh.s - Motorola 68040 FP hyperbolic tangent routines (EXC) */

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

	stanhsa 3.1 12/10/90

	The entry point sTanh computes the hyperbolic tangent of
	an input argument|  sTanhd does the same except for denormalized
	input.

	Input: Double-extended number X in location pointed to
		by address register a0.

	Output: The value tanh(X) returned in floating-point register Fp0.

	Accuracy and Monotonicity: The returned result is within 3 ulps in
		64 significant bit, i.e. within 0.5001 ulp to 53 bits if the
		result is subsequently rounded to double precision. The
		result is provably monotonic in double precision.

	Speed: The program __x_stanh takes approximately 270 cycles.

	Algorithm:

	TANH
	1. If |X| >= (5/2) log2 or |X| <= 2**(-40), go to 3.

	2. (2**(-40) < |X| < (5/2) log2) Calculate tanh(X) by
		sgn := sign(X), y := 2|X|, z := expm1(Y), and
		tanh(X) = sgn*( z/(2+z) ).
		Exit.

	3. (|X| <= 2**(-40) or |X| >= (5/2) log2). If |X| < 1,
		go to 7.

	4. (|X| >= (5/2) log2) If |X| >= 50 log2, go to 6.

	5. ((5/2) log2 <= |X| < 50 log2) Calculate tanh(X) by
		sgn := sign(X), y := 2|X|, z := exp(Y),
		tanh(X) = sgn - [ sgn*2/(1+z) ].
		Exit.

	6. (|X| >= 50 log2) Tanh(X) = +-1 (round to nearest). Thus, we
		calculate Tanh(X) by
		sgn := sign(X), Tiny := 2**(-126),
		tanh(X) := sgn - sgn*Tiny.
		Exit.

	7. (|X| < 2**(-40)). Tanh(X) = X.	Exit.


		Copyright (C) Motorola, Inc. 1990
			All Rights Reserved

	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA
	The copyright notice above does not evidence any
	actual or intended publication of such source code.

STANH	idnt	2,1 Motorola 040 Floating Point Software Package

	section	8

NOMANUAL
*/

#include "fpsp040E.h"

#define	X		FP_SCR5
#define	XDCARE		X+2
#define	XFRAC		X+4

#define	SGN		L_SCR3

#define	V		FP_SCR6

BOUNDS1:	.long 0x3FD78000,0x3FFFDDCE |... 2^(-40), (5/2)LOG2

|	xref	__x_t_frcinx
|	xref	__x_t_extdnrm
|	xref	__x_setox
|	xref	__x_setoxm1


	.text

	.globl	__x_stanhd
__x_stanhd:
|--TANH(X) = X FOR DENORMALIZED X

	jra 		__x_t_extdnrm

	.globl	__x_stanh
__x_stanh:
	fmovex		a0@,fp0	|...lOAD INPUT

	fmovex		fp0,a6@(X)
	movel		a0@,d0
	movew		a0@(4),d0
	movel		d0,a6@(X)
	andl		#0x7FFFFFFF,d0
	cmp2l		pc@(BOUNDS1),d0	|...2**(-40) < |X| < (5/2)LOG2 ?
	jcs 		TANHBORS

|--THIS IS THE USUAL CASE
|--Y = 2|X|, Z = EXPM1(Y), TANH(X) = SIGN(X) * Z / (Z+2).

	movel		a6@(X),d0
	movel		d0,a6@(SGN)
	andl		#0x7FFF0000,d0
	addl		#0x00010000,d0	|...EXPONENT OF 2|X|
	movel		d0,a6@(X)
	andl		#0x80000000,a6@(SGN)
	fmovex		a6@(X),fp0	|...FP0 IS Y = 2|X|

	movel		d1,a7@-
	clrl		d1
	fmovemx	fp0-fp0,a0@
	bsrl		__x_setoxm1 	|...FP0 IS Z = EXPM1(Y)
	movel		a7@+,d1

	fmovex		fp0,fp1
	.long 0xf23c44a2,0x40000000	/*  fadds  &0x40000000,fp1 */
	movel		a6@(SGN),d0
	fmovex		fp1,a6@(V)
	eorl		d0,a6@(V)

	fmovel		d1,fpcr		| restore users exceptions
	fdivx		a6@(V),fp0
	jra 		__x_t_frcinx

TANHBORS:
	cmpl		#0x3FFF8000,d0
	jlt 		TANHSM

	cmpl		#0x40048AA1,d0
	jgt 		TANHHUGE

|-- (5/2) LOG2 < |X| < 50 LOG2,
|--TANH(X) = 1 - (2/[EXP(2X)+1]). LET Y = 2|X|, SGN = SIGN(X),
|--TANH(X) = SGN -	SGN*2/[EXP(Y)+1].

	movel		a6@(X),d0
	movel		d0,a6@(SGN)
	andl		#0x7FFF0000,d0
	addl		#0x00010000,d0	|...EXPO OF 2|X|
	movel		d0,a6@(X)		|...Y = 2|X|
	andl		#0x80000000,a6@(SGN)
	movel		a6@(SGN),d0
	fmovex		a6@(X),fp0		|...Y = 2|X|

	movel		d1,a7@-
	clrl		d1
	fmovemx	fp0-fp0,a0@
	bsrl		__x_setox		|...FP0 IS EXP(Y)
	movel		a7@+,d1
	movel		a6@(SGN),d0
	.long 0xf23c4422,0x3f800000	/*  fadds  &0x3F800000,fp0 */

	eorl		#0xC0000000,d0	|...-SIGN(X)*2
	fmoves		d0,fp1		|...-SIGN(X)*2 IN SGL FMT
	fdivx		fp0,fp1	 	|...-SIGN(X)2 / [EXP(Y)+1 ]

	movel		a6@(SGN),d0
	orl		#0x3F800000,d0	|...SGN
	fmoves		d0,fp0		|...SGN IN SGL FMT

	fmovel		d1,fpcr		| restore users exceptions
	faddx		fp1,fp0

	jra 		__x_t_frcinx

TANHSM:
	movew		#0x0000,a6@(XDCARE)

	fmovel		d1,fpcr		| restore users exceptions
	fmovex		a6@(X),fp0		| last inst - possible exception set

	jra 		__x_t_frcinx

TANHHUGE:
|---RETURN SGN(X) - SGN(X)EPS
	movel		a6@(X),d0
	andl		#0x80000000,d0
	orl		#0x3F800000,d0
	fmoves		d0,fp0
	andl		#0x80000000,d0
	eorl		#0x80800000,d0	|...-SIGN(X)*EPS

	fmovel		d1,fpcr		| restore users exceptions
	fadds		d0,fp0

	jra 		__x_t_frcinx

|	end
