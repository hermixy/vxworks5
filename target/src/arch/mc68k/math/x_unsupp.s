/* x_unsupp.s - Motorola 68040 FP unsupported data type exc handler (EXC) */

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


	x_unsuppsa 3.2 4/26/91

	__x_fpsp_unsupp --- FPSP handler for unsupported data type exception

Trap vector #55	(See table 8-1 Mc68030 User's manual).
Invoked when the user program encounters a data format (packed) that
hardware does not support or a data type (denormalized numbers or un-
normalized numbers).
Normalizes denorms and unnorms, unpacks packed numbers then stores
them back into the machine to let the 040 finish the operation.

Unsupp calls two routines:
	1. __x_get_op -  gets the operand(s)
	2. __x_res_func - restore the function back into the 040 or
			if fmovep fpm,<ea> then pack source (fpm)
			and store in users memory <ea>.

Input: Long fsave stack frame



		Copyright (C) Motorola, Inc. 1990
			All Rights Reserved

	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA
	The copyright notice above does not evidence any
	actual or intended publication of such source code.

X_UNSUPP	idnt    2,1 Motorola 040 Floating Point Software Package

	section	8

NOMANUAL
*/

#include "fpsp040E.h"

|	xref	__x_get_op
|	xref	__x_res_func
|	xref	__x_gen_except
|	xref	__x_fpsp_fmt_error
|	xref	__x_check_force


	.text

	.globl	__x_fpsp_unsupp
__x_fpsp_unsupp:
|
	link		a6,#-LOCAL_SIZE
	fsave		a7@-
	moveml		d0-d1/a0-a1,a6@(USER_DA)
	fmovemx	fp0-fp3,a6@(USER_FP0)
	fmoveml	fpcr/fpsr/fpi,a6@(USER_FPCR)

/*
| At this point we need to look at the instructions and see if it is one of
| the force-precision ones (fsadd,fdadd,fssub,fdsub,fsmul,fdmul,fsdiv,fddiv,
| fssqrt,fdsqrt,fsmove,fdmove,fsabs,fdabs,fsneg,fdneg).  If it is then
| correct the USER_FPCR to the instruction's rounding precision (s or d).
| Also, we need to check if the instruction is fsgldiv or fsglmul.  If it
| is then the USER_FPCR is set to extended rounding precision.  Otherwise
| leave the USER_FPCR alone.
*/

|	bsrl		__x_check_force

	moveb		a7@,a6@(VER_TMP) | save version number
	moveb		a7@,d0		| test for valid version num
	andib		#0xf0,d0		| test for 0x4x
	cmpib		#VER_4,d0	| must be 0x4x or exit
	jne 		__x_fpsp_fmt_error

	fmovel		#0,FPSR		| clear all user status bits
	fmovel		#0,fpcr		| clear all user control bits
|
|	The following lines are used to ensure that the FPSR
|	exception byte and condition codes are clear before proceeding,
/* |	except in the case of fmove, which leaves the cc's intact. */
|
__x_unsupp_con:
	movel		a6@(USER_FPSR),d1
	btst		#5,a6@(CMDREG1B)	| looking for fmovel out
	jne 		fmove_con
	andl		#0xFF00FF,d1	| clear all but aexcs and qbyte
	jra 		end_fix
fmove_con:
	andl		#0x0FFF40FF,d1	/* clear all but cc's, snan bit,
					 * aexcs, and qbyte */
end_fix:
	movel		d1,a6@(USER_FPSR)

	st		a6@(UFLG_TMP)	| set flag for unsupp data

	bsrl		__x_get_op		| everything okay, go get operand(s)
	bsrl		__x_res_func	| fix up stack frame so can restore it
	clrl		a7@-
	moveb		a6@(VER_TMP),a7@ | move idle fmt word to top of stack
	jra 		__x_gen_except
|
|	end
