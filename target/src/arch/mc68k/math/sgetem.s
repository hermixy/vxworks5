/* sgetem.s - Motorola 68040 FP exponent and mantissa routines (EXC) */

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


	sgetemsa 3.1 12/10/90

	The entry point sGETEXP returns the exponent portion
	of the input argument.  The exponent bias is removed
	and the exponent value is returned as an extended
	precision number in fp0.  sGETEXPD handles denormalized
	numbers.

	The entry point sGETMAN extracts the mantissa of the
	input argument.  The mantissa is converted to an
	extended precision number and returned in fp0.  The
	range of the result is [1.0 - 2.0).


	Input:  Double-extended number X in the ETEMP space in
		the floating-point save stack.

	Output:	The functions return exp(X) or man(X) in fp0.

	Modified: fp0.


		Copyright (C) Motorola, Inc. 1990
			All Rights Reserved

	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA
	The copyright notice above does not evidence any
	actual or intended publication of such source code.

SGETEM	idnt	2,1 Motorola 040 Floating Point Software Package

	section 8

NOMANUAL
*/

#include "fpsp040E.h"

|	xref	__x_nrm_set

|
| This entry point is used by the unimplemented instruction exception
| handler.  It points a0 to the input operand.
|
|
|
|	SGETEXP
|


	.text

	.globl	__x_sgetexp
__x_sgetexp:
	movew	a0@(LOCAL_EX),d0	| get the exponent
	bclr	#15,d0			| clear the sign bit
	subw	#0x3fff,d0		| subtract off the bias
	fmovew  d0,fp0			| move the exp to fp0
	rts

	.globl	__x_sgetexpd
__x_sgetexpd:
	bclr	#sign_bit,a0@(LOCAL_EX)
	bsrl	__x_nrm_set		| normalize (exp will go negative)
	movew	a0@(LOCAL_EX),d0	| load resulting exponent into d0
	subw	#0x3fff,d0		| subtract off the bias
	fmovew	d0,fp0			| move the exp to fp0
	rts
|
|
| This entry point is used by the unimplemented instruction exception
| handler.  It points a0 to the input operand.
|
|
|
|	SGETMAN
|
|
| For normalized numbers, leave the mantissa alone, simply load
| with an exponent of +/- 0x3fff.
|
	.globl	__x_sgetman
__x_sgetman:
	movel	a6@(USER_FPCR),d0
	andil	#0xffffff00,d0		| clear rounding precision and mode
	fmovel	d0,fpcr			| this fpcr setting is used by the 882
	movew	a0@(LOCAL_EX),d0	| get the exp (really just want sign bit)
	orw	#0x7fff,d0		| clear old exp
	bclr	#14,d0	 		| make it the new exp +-3fff
	movew	d0,a0@(LOCAL_EX)	| move the sign # exp back to fsave stack
	fmovex	a0@,fp0	| put new value back in fp0
	rts

|
| For denormalized numbers, shift the mantissa until the j-bit = 1,
| then load the exponent with +/1 0x3fff.
|
	.globl	__x_sgetmand
__x_sgetmand:
	movel	a0@(LOCAL_HI),d0	| load ms mant in d0
	movel	a0@(LOCAL_LO),d1	| load ls mant in d1
	bsrl	shft			| shift mantissa bits till msbit is set
	movel	d0,a0@(LOCAL_HI)	| put ms mant back on stack
	movel	d1,a0@(LOCAL_LO)	| put ls mant back on stack
	jra 	__x_sgetman

|
|	SHFT
|
|	Shifts the mantissa bits until msbit is set.
|	input:
|		ms mantissa part in d0
|		ls mantissa part in d1
|	output:
|		shifted bits in d0 and d1
shft:
	tstl	d0			| if any bits set in ms mant
	jne 	upper			| then branch
|					| else no bits set in ms mant
	tstl	d1			| test if any bits set in ls mant
	jne 	cont			| if set then continue
	jra 	shft_end		| else return
cont:
	movel	d3,a7@-			| save d3
	exg	d0,d1			| shift ls mant to ms mant
	bfffo	d0{#0:#32},d3		| find first 1 in ls mant to d0
	lsll	d3,d0			| shift first 1 to int bit in ms mant
	movel	a7@+,d3			| restore d3
	jra 	shft_end
upper:

	moveml	d3/d5/d6,a7@-		| save registers
	bfffo	d0{#0:#32},d3		| find first 1 in ls mant to d0
	lsll	d3,d0			| shift ms mant until j-bit is set
	movel	d1,d6			| save ls mant in d6
	lsll	d3,d1			| shift ls mant by count
	movel	#32,d5
	subl	d3,d5			| sub 32 from shift for ls mant
	lsrl	d5,d6			| shift off all bits but those that will
|					| be shifted into ms mant
	orl	d6,d0			| shift the ls mant bits into ms mant
	moveml	a7@+,d3/d5/d6		| restore registers
shft_end:
	rts

|	end
