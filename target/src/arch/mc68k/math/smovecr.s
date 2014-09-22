/* smovecr.s - Motorola 68040 FP constant routines (EXC) */

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

	smovecrsa 3.1 12/10/90

	The entry point sMOVECR returns the constant at the
	offset given in the instruction field.

	Input: An offset in the instruction word.

	Output:	The constant rounded to the user's rounding
		mode unchecked for overflow.

	Modified: fp0.


		Copyright (C) Motorola, Inc. 1990
			All Rights Reserved

	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA
	The copyright notice above does not evidence any
	actual or intended publication of such source code.

SMOVECR	idnt	2,1 Motorola 040 Floating Point Software Package

	section 8

NOMANUAL
*/

#include "fpsp040E.h"

|	xref	__x_nrm_set
|	xref	__x_round
|	xref	__x_PIRN
|	xref	__x_PIRZRM
|	xref	__x_PIRP
|	xref	__x_SMALRN
|	xref	__x_SMALRZRM
|	xref	__x_SMALRP
|	xref	__x_BIGRN
|	xref	__x_BIGRZRM
|	xref	__x_BIGRP

FZERO:	.long	00000000
|
|	FMOVECR
|

	.text

	.globl	__x_smovcr
__x_smovcr:
	bfextu	a6@(CMDREG1B){#9:#7},d0 | get offset
	bfextu	a6@(USER_FPCR){#26:#2},d1 | get rmode
|
| check range of offset
|
	tstb	d0			| if zero, offset is to pi
	jeq 	PI_TBL			| it is pi
	cmpib	#0x0a,d0		| check range 0x01 - 0x0a
	jle 	Z_VAL			| if in this range, return zero
	cmpib	#0x0e,d0		| check range 0x0b - 0x0e
	jle 	SM_TBL			| valid constants in this range
	cmpib	#0x2f,d0		| check range 0x10 - 0x2f
	jle 	Z_VAL			| if in this range, return zero
	cmpib	#0x3f,d0		| check range 0x30 - 0x3f
	jle   	BG_TBL			| valid constants in this range
Z_VAL:
	fmoves	FZERO,fp0
	rts
PI_TBL:
	tstb	d1			| offset is zero, check for rmode
	jeq 	PI_RN			| if zero, rn mode
	cmpib	#0x3,d1			| check for rp
	jeq 	PI_RP			| if 3, rp mode
PI_RZRM:
	lea	__x_PIRZRM,a0	| rmode is rz or rm, load __x_PIRZRM in a0
	jra 	set_finx
PI_RN:
	lea	__x_PIRN,a0	| rmode is rn, load __x_PIRN in a0
	jra 	set_finx
PI_RP:
	lea	__x_PIRP,a0	| rmode is rp, load __x_PIRP in a0
	jra 	set_finx
SM_TBL:
	subil	#0xb,d0		| make offset in 0 - 4 range
	tstb	d1		| check for rmode
	jeq 	SM_RN		| if zero, rn mode
	cmpib	#0x3,d1		| check for rp
	jeq 	SM_RP		| if 3, rp mode
SM_RZRM:
	lea	__x_SMALRZRM,a0	| rmode is rz or rm, load SMRZRM in a0
	cmpib	#0x2,d0		| check if result is inex
	jle 	set_finx	| if 0 - 2, it is inexact
	jra 	no_finx		| if 3, it is exact
SM_RN:
	lea	__x_SMALRN,a0	| rmode is rn, load SMRN in a0
	cmpib	#0x2,d0		| check if result is inex
	jle 	set_finx	| if 0 - 2, it is inexact
	jra 	no_finx		| if 3, it is exact
SM_RP:
	lea	__x_SMALRP,a0	| rmode is rp, load SMRP in a0
	cmpib	#0x2,d0		| check if result is inex
	jle 	set_finx	| if 0 - 2, it is inexact
	jra 	no_finx		| if 3, it is exact
BG_TBL:
	subil	#0x30,d0	| make offset in 0 - f range
	tstb	d1		| check for rmode
	jeq 	BG_RN		| if zero, rn mode
	cmpib	#0x3,d1		| check for rp
	jeq 	BG_RP		| if 3, rp mode
BG_RZRM:
	lea	__x_BIGRZRM,a0	| rmode is rz or rm, load BGRZRM in a0
	cmpib	#0x1,d0		| check if result is inex
	jle 	set_finx	| if 0 - 1, it is inexact
	cmpib	#0x7,d0		| second check
	jle 	no_finx		| if 0 - 7, it is exact
	jra 	set_finx	| if 8 - f, it is inexact
BG_RN:
	lea	__x_BIGRN,a0	| rmode is rn, load BGRN in a0
	cmpib	#0x1,d0		| check if result is inex
	jle 	set_finx	| if 0 - 1, it is inexact
	cmpib	#0x7,d0		| second check
	jle 	no_finx		| if 0 - 7, it is exact
	jra 	set_finx	| if 8 - f, it is inexact
BG_RP:
	lea	__x_BIGRP,a0	| rmode is rp, load SMRP in a0
	cmpib	#0x1,d0		| check if result is inex
	jle 	set_finx	| if 0 - 1, it is inexact
	cmpib	#0x7,d0		| second check
	jle 	no_finx		| if 0 - 7, it is exact
|	jra 	set_finx	| if 8 - f, it is inexact
set_finx:
	orl	#inx2a_mask,a6@(USER_FPSR) | set inex2/ainex
no_finx:
	mulul	#12,d0			| use offset to point into tables
	movel	d1,a6@(L_SCR1)		| load mode for __x_round call
	bfextu	a6@(USER_FPCR){#24:#2},d1	| get precision
	tstl	d1			| check if extended precision
|
| Precision is extended
|
	jne 	not_ext			| if extended, do not call __x_round
	fmovemx a0@(d0),fp0-fp0		| return result in fp0
	rts
|
| Precision is single or double
|
not_ext:
	swap	d1			| rnd prec in upper word of d1
	addl	a6@(L_SCR1),d1		| merge rmode in low word of d1
	movel	a0@(d0),a6@(FP_SCR1)	| load first word to temp storage
	movel	a0@(4,d0),a6@(FP_SCR1+4)	| load second word
	movel	a0@(8,d0),a6@(FP_SCR1+8)	| load third word
	clrl	d0			| clear g,r,s
	lea	a6@(FP_SCR1),a0
	btst	#sign_bit,a0@(LOCAL_EX)
	sne	a0@(LOCAL_SGN)		| convert to internal ext. format

	bsrl	__x_round		| go round the mantissa

	bfclr	a0@(LOCAL_SGN){#0:#8}	| convert back to IEEE ext format
	jeq 	fin_fcr
	bset	#sign_bit,a0@(LOCAL_EX)
fin_fcr:
	fmovemx	a0@,fp0-fp0
	rts

|	end
