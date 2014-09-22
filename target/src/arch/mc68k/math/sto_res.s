/* sto_res.s - Motorola 68040 FP result storage routines (EXC) */

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

	sto_ressa 3.1 12/10/90

	Takes the result and puts it in where the user expects it.
	Library functions return result in fp0.	If fp0 is not the
	users destination register then fp0 is moved to the the
	correct floating-point destination register.  fp0 and fp1
	are then restored to the original contents.

	Input:	result in fp0,fp1

		d2 # a0 should be kept unmodified

	Output:	moves the result to the true destination reg or mem

	Modifies: destination floating point register


		Copyright (C) Motorola, Inc. 1990
			All Rights Reserved

	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA
	The copyright notice above does not evidence any
	actual or intended publication of such source code.

STO_RES	idnt	2,1 Motorola 040 Floating Point Software Package


	section	8

NOMANUAL
*/

#include "fpsp040E.h"


	.text

	.globl	__x_sto_cos
__x_sto_cos:
	bfextu		a6@(CMDREG1B){#13:#3},d0	| extract cos destination
	cmpib		#3,d0		| check for fp0/fp1 cases
	jle 		c_fp0123
	fmovemx	fp1-fp1,a7@-
	moveql		#7,d1
	subl		d0,d1		| d1 = 7- (dest. reg. no.)
	clrl		d0
	bset		d1,d0		| d0 is dynamic register mask
	fmovemx	a7@+,d0
	rts
c_fp0123:
	cmpib		#0,d0
	jeq 		c_is_fp0
	cmpib		#1,d0
	jeq 		c_is_fp1
	cmpib		#2,d0
	jeq 		c_is_fp2
c_is_fp3:
	fmovemx	fp1-fp1,a6@(USER_FP3)
	rts
c_is_fp2:
	fmovemx	fp1-fp1,a6@(USER_FP2)
	rts
c_is_fp1:
	fmovemx	fp1-fp1,a6@(USER_FP1)
	rts
c_is_fp0:
	fmovemx	fp1-fp1,a6@(USER_FP0)
	rts


	.globl	__x_sto_res
__x_sto_res:
	bfextu		a6@(CMDREG1B){#6:#3},d0	| extract destination register
	cmpib		#3,d0		| check for fp0/fp1 cases
	jle 		fp0123
	fmovemx	fp0-fp0,a7@-
	moveql		#7,d1
	subl		d0,d1		| d1 = 7- (dest. reg. no.)
	clrl		d0
	bset		d1,d0		| d0 is dynamic register mask
	fmovemx	a7@+,d0
	rts
fp0123:
	cmpib		#0,d0
	jeq 		is_fp0
	cmpib		#1,d0
	jeq 		is_fp1
	cmpib		#2,d0
	jeq 		is_fp2
is_fp3:
	fmovemx	fp0-fp0,a6@(USER_FP3)
	rts
is_fp2:
	fmovemx	fp0-fp0,a6@(USER_FP2)
	rts
is_fp1:
	fmovemx	fp0-fp0,a6@(USER_FP1)
	rts
is_fp0:
	fmovemx	fp0-fp0,a6@(USER_FP0)
	rts

|	end
