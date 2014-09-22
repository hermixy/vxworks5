/* x_bsun.s - Motorola 68040 FP branch/set unordered exception handler (EXC) */

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


	x_bsunsa 3.2 4/26/91

	fpsp_bsun --- FPSP handler for branch/set on unordered exception

	Copy the PC to fpi to maintain 881/882 compatability

	The __x_real_bsun handler will need to perform further corrective
	measures as outlined in the 040 User's Manual on pages
	9-41f, section 9.8.3.


		Copyright (C) Motorola, Inc. 1990
			All Rights Reserved

	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA
	The copyright notice above does not evidence any
	actual or intended publication of such source code.

X_BSUN	idnt    2,1 Motorola 040 Floating Point Software Package

	section	8

NOMANUAL
*/

#include "fpsp040E.h"

|	xref	__x_real_bsun
|	xref	__x_check_force


	.text

	.globl	__x_fpsp_bsun
__x_fpsp_bsun:
|
	link		a6,#-LOCAL_SIZE
	fsave		a7@-
	moveml		d0-d1/a0-a1,a6@(USER_DA)
	fmovemx		fp0-fp3,a6@(USER_FP0)
	fmoveml		fpcr/fpsr/fpi,a6@(USER_FPCR)

| At this point we need to look at the instructions and see if it is one of
| the force-precision ones (fsadd,fdadd,fssub,fdsub,fsmul,fdmul,fsdiv,fddiv,
| fssqrt,fdsqrt,fsmove,fdmove,fsabs,fdabs,fsneg,fdneg).  If it is then
| correct the USER_FPCR to the instruction rounding precision (s or d).
| Also, we need to check if the instruction is fsgldiv or fsglmul.  If it
| is then the USER_FPCR is set to extended rounding precision.  Otherwise
| leave the USER_FPCR alone.
|	bsrl		__x_check_force
|
	movel		a6@(EXC_PC),a6@(USER_fpi)
|
	moveml		a6@(USER_DA),d0-d1/a0-a1
	fmovemx		a6@(USER_FP0),fp0-fp3
	fmoveml		a6@(USER_FPCR),fpcr/fpsr/fpi
	frestore	a7@+
	unlk		a6
	jra 		__x_real_bsun
|
|	end
