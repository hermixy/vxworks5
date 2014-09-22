/* l_ssinh.s - Motorola 68040 FP hyperbolic sine routines (LIB) */

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
01b,10jan92,kdl  general cleanup.
01a,15aug91,kdl  original version, from Motorola FPSP v2.0;
		 added missing comment symbols.
*/

/*
DESCRIPTION


	ssinhsa 3.1 12/10/90

       The entry point sSinh computes the hyperbolic sine of
       an input argument;  sSinhd does the same except for denormalized
       input.

       Input: Double-extended number X in location pointed to
		by address register a0.

       Output: The value sinh(X) returned in floating-point register Fp0.

       Accuracy and Monotonicity: The returned result is within 3 ulps in
               64 significant bit, i.e. within 0.5001 ulp to 53 bits if the
               result is subsequently rounded to double precision. The
               result is provably monotonic in double precision.

       Speed: The program sSINH takes approximately 280 cycles.

       Algorithm:

       SINH
       1. If |X| > 16380 log2, go to 3.

       2. (|X| <= 16380 log2) Sinh(X) is obtained by the formulae
               y = |X|, sgn = sign(X), and z = expm1(Y),
               sinh(X) = sgn*(1/2)*( z + z/(1+z) ).
          Exit.

       3. If |X| > 16480 log2, go to 5.

       4. (16380 log2 < |X| <= 16480 log2)
               sinh(X) = sign(X) * exp(|X|)/2.
          However, invoking exp(|X|) may cause premature overflow.
          Thus, we calculate sinh(X) as follows:
             Y       := |X|
             sgn     := sign(X)
             sgnFact := sgn * 2**(16380)
             Y'      := Y - 16381 log2
             sinh(X) := sgnFact * exp(Y').
          Exit.

       5. (|X| > 16480 log2) sinh(X) must overflow. Return
          sign(X)*Huge*Huge to generate overflow and an infinity with
          the appropriate sign. Huge is the largest finite number in
          extended format. Exit.

		Copyright (C) Motorola, Inc. 1990
			All Rights Reserved

	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA
	The copyright notice above does not evidence any
	actual or intended publication of such source code.

SSINH	idnt	2,1 Motorola 040 Floating Point Software Package

	section	8

NOMANUAL
*/

T1:	.long 0x40C62D38,0xD3D64634 |... 16381 LOG2 LEAD
T2:	.long 0x3D6F90AE,0xB1E75CC7 |... 16381 LOG2 TRAIL

|	xref	__l_t_frcinx
|	xref	__l_t_ovfl
|	xref	__l_t_extdnrm
|	xref	__l_setox
|	xref	__l_setoxm1


	.text

	.globl	__l_ssinhd
__l_ssinhd:
|--SINH(X) = X FOR DENORMALIZED X

	jra 	__l_t_extdnrm

	.globl	__l_ssinh
__l_ssinh:
	fmovex	a0@,fp0	|...lOAD INPUT

	movel	a0@,d0
	movew	a0@(4),d0
	movel	d0,a1		|  save a copy of original (compacted) operand
	andl	#0x7FFFFFFF,d0
	cmpl	#0x400CB167,d0
	jgt 	SINHBIG

|--THIS IS THE USUAL CASE, |X| < 16380 LOG2
|--Y = |X|, Z = EXPM1(Y), SINH(X) = SIGN(X)*(1/2)*( Z + Z/(1+Z) )

	fabsx	fp0		|...Y = |X|

	moveml	a1/d1,a7@-
	fmovemx fp0-fp0,a0@
	clrl	d1
	bsrl	__l_setoxm1	 	|...FP0 IS Z = EXPM1(Y)
	fmovel	#0,fpcr
	moveml	a7@+,a1/d1

	fmovex	fp0,fp1
	.long 0xf23c44a2,0x3f800000	/*	fadds	&0x3F800000,fp1 */
	fmovex	fp0,a7@-
	fdivx	fp1,fp0		|...Z/(1+Z)
	movel	a1,d0
	andl	#0x80000000,d0
	orl	#0x3F000000,d0
	faddx	a7@+,fp0
	movel	d0,a7@-

	fmovel	d1,fpcr
	fmuls	a7@+,fp0	| last fp inst - possible exceptions set

	jra 	__l_t_frcinx

SINHBIG:
	cmpl	#0x400CB2B3,d0
	jgt 	__l_t_ovfl
	fabsx	fp0
	fsubd	pc@(T1),fp0	|...(|X|-16381LOG2_LEAD)
	movel	#0,a7@-
	movel	#0x80000000,a7@-
	movel	a1,d0
	andl	#0x80000000,d0
	orl	#0x7FFB0000,d0
	movel	d0,a7@-		|...EXTENDED FMT
	fsubd	pc@(T2),fp0	|...|X| - 16381 LOG2, ACCURATE

	movel	d1,a7@-
	clrl	d1
	fmovemx fp0-fp0,a0@
	bsrl	__l_setox
	fmovel	a7@+,fpcr

	fmulx	a7@+,fp0	| possible exception
	jra 	__l_t_frcinx

|	end
