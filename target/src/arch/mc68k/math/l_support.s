/* l_support.s - Motorola 68040 FP support routines (LIB) */

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
01a,15aug91,kdl  original version, from Motorola FPSP v2.0;
		 added missing comment symbols.
*/

/*
DESCRIPTION

	supportsa 1.2 5/1/91

		Copyright (C) Motorola, Inc. 1991
			All Rights Reserved

	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA
	The copyright notice above does not evidence any
	actual or intended publication of such source code.

L_SUPPORT    idnt    2,1 Motorola 040 Floating Point Software Package

	section    8

NOMANUAL
*/

mns_one:  .long 0xbfff0000,0x80000000,0x00000000
pls_one:  .long 0x3fff0000,0x80000000,0x00000000
pls_inf:  .long 0x7fff0000,0x00000000,0x00000000
pls_huge: .long 0x7ffe0000,0xffffffff,0xffffffff
mns_huge: .long 0xfffe0000,0xffffffff,0xffffffff
pls_tiny: .long 0x00000000,0x80000000,0x00000000
mns_tiny: .long 0x80000000,0x80000000,0x00000000
small:    .long 0x20000000,0x80000000,0x00000000
pls_zero: .long 0x00000000,0x00000000,0x00000000

#include "fpsp040L.h"

|
| 	__l_tag --- determine the type of an extended precision operand
|
|	The tag values returned match the way the 68040 would have
|	tagged them.
|
|	Input:	a0 points to operand
|
|	Output	d0b	= 0x00 norm
|			  0x20 zero
|			  0x40 inf
|			  0x60 nan
|			  0x80 denorm
|		All other registers are unchanged
|

	.text

	.globl	__l_tag
__l_tag:
	movew	a0@(LOCAL_EX),d0
	andiw	#0x7fff,d0
	jeq 	chk_zro
	cmpiw	#0x7fff,d0
	jeq 	chk_inf
__l_tag_nrm:
	clrb	d0
	rts
__l_tag_nan:
	moveb	#0x60,d0
	rts
__l_tag_dnrm:
	moveb	#0x80,d0
	rts
chk_zro:
	btst	#7,a0@(LOCAL_HI)	| # check if J-bit is set
	jne 	__l_tag_nrm
	tstl	a0@(LOCAL_HI)
	jne 	__l_tag_dnrm
	tstl	a0@(LOCAL_LO)
	jne 	__l_tag_dnrm
__l_tag_zero:
	moveb	#0x20,d0
	rts
chk_inf:
	tstl	a0@(LOCAL_HI)
	jne 	__l_tag_nan
	tstl	a0@(LOCAL_LO)
	jne 	__l_tag_nan
__l_tag_inf:
	moveb	#0x40,d0
	rts

|
|	__l_t_dz, __l_t_dz2 --- divide by zero exception
|
| __l_t_dz2 is used by monadic functions such as flogn (from __l_do_func).
| __l_t_dz is used by monadic functions such as __l_satanh (from the
| transcendental function).
|
	.globl    __l_t_dz2
__l_t_dz2:
	fmovemx	mns_one,fp0-fp0
	fmovel	d1,fpcr
	fdivx		pls_zero,fp0
	rts

	.globl	__l_t_dz
__l_t_dz:
	btst	#sign_bit,a6@(ETEMP_EX)	| check sign for neg or pos
	jeq 	p_inf			| branch if pos sign
m_inf:
	fmovemx mns_one,fp0-fp0
	fmovel	d1,fpcr
	fdivx		pls_zero,fp0
	rts
p_inf:
	fmovemx pls_one,fp0-fp0
	fmovel	d1,fpcr
	fdivx		pls_zero,fp0
	rts
|
|	__l_t_operr --- Operand Error exception
|
	.globl    __l_t_operr
__l_t_operr:
	fmovemx	pls_inf,fp0-fp0
	fmovel	d1,fpcr
	fmulx		pls_zero,fp0
	rts

|
|	__l_t_unfl --- UNFL exception
|
	.globl    __l_t_unfl
__l_t_unfl:
	btst	#sign_bit,a6@(ETEMP)
	jeq 	unf_pos
unf_neg:
	fmovemx	mns_tiny,fp0-fp0
	fmovel	d1,fpcr
	fmulx	pls_tiny,fp0
	rts

unf_pos:
	fmovemx	pls_tiny,fp0-fp0
	fmovel	d1,fpcr
	fmulx	fp0,fp0
	rts
|
|	__l_t_ovfl --- OVFL exception
|
|	__l_t_ovfl is called as an exit for monadic functions.  __l_t_ovfl2
|	is for dyadic exits.
|
	.globl   		__l_t_ovfl
__l_t_ovfl:
	.globl   		__l_t_ovfl2
	movel		d1,a6@(USER_FPCR)	/* |  user's control register */
	movel		#ovfinx_mask,d0
	jra 		t_work
__l_t_ovfl2:
	movel		#__l_ovfl_inx_mask,d0
t_work:
	btst		#sign_bit,a6@(ETEMP)
	jeq 		ovf_pos
ovf_neg:
	fmovemx	mns_huge,fp0-fp0
	fmovel		a6@(USER_FPCR),fpcr
	fmulx		pls_huge,fp0
	fmovel		fpsr,d1
	orl		d1,d0
	fmovel		d0,fpsr
	rts
ovf_pos:
	fmovemx	pls_huge,fp0-fp0
	fmovel		a6@(USER_FPCR),fpcr
	fmulx		pls_huge,fp0
	fmovel		fpsr,d1
	orl		d1,d0
	fmovel		d0,fpsr
	rts
|
|	__l_t_inx2 --- INEX2 exception (correct fpcr is in	a6@(USER_FPCR))
|
	.globl    __l_t_inx2
__l_t_inx2:
	fmovel		fpsr,a6@(USER_FPSR)	|  capture incoming fpsr
	fmovel		a6@(USER_FPCR),fpcr
|
| create an inex2 exception by adding two numbers with very different exponents
| do the add in fp1 so as to not disturb the result sitting in fp0
|
	fmovex		pls_one,fp1
	faddx		small,fp1
|
	orl	#inx2a_mask,a6@(USER_FPSR) | set INEX2, AINEX
	fmovel	a6@(USER_FPSR),fpsr
	rts
|
|	__l_t_frcinx --- Force Inex2 (for monadic functions)
|
	.globl	__l_t_frcinx
__l_t_frcinx:
	fmovel		fpsr,a6@(USER_FPSR)	|  capture incoming fpsr
	fmovel		d1,fpcr
|
| create an inex2 exception by adding two numbers with very different exponents
| do the add in fp1 so as to not disturb the result sitting in fp0
|
	fmovex		pls_one,fp1
	faddx		small,fp1
|
	orl	#inx2a_mask,a6@(USER_FPSR) | set INEX2, AINEX
	btst	#__l_unfl_bit,a6@(FPSR_EXCEPT) | test for unfl bit set
	jeq 	no_uacc1		| if clear, do not set aunfl
	bset	#aunfl_bit,a6@(FPSR_AEXCEPT)
no_uacc1:
	fmovel	a6@(USER_FPSR),fpsr
	rts
|
|	__l_dst_nan --- force result when destination is a NaN
|
	.globl	__l_dst_nan
__l_dst_nan:
	fmovel	a6@(USER_FPCR),fpcr
	fmovex	a6@(FPTEMP),fp0
	rts

|
|	__l_src_nan --- force result when source is a NaN
|
	.globl	__l_src_nan
__l_src_nan:
	fmovel	a6@(USER_FPCR),fpcr
	fmovex	a6@(ETEMP),fp0
	rts
|
|	__l_mon_nan --- force result when source is a NaN (monadic version)
|
/* |	This is the same as __l_src_nan except that the user's fpcr comes */
|	in via d1, not	a6@(USER_FPCR).
|
	.globl	__l_mon_nan
__l_mon_nan:
	fmovel	d1,fpcr
	fmovex	a6@(ETEMP),fp0
	rts
|
|	__l_t_extdnrm, __l_t_resdnrm --- generate results for denorm inputs
|
|	For all functions that have a denormalized input and that f(x)=x,
|	this is the entry point.
|
	.globl	__l_t_extdnrm
__l_t_extdnrm:
	fmovel	d1,fpcr
	fmovex	a0@(LOCAL_EX),fp0
	fmovel		fpsr,d0
	orl		#unfinx_mask,d0
	fmovel		d0,fpsr
	rts

	.globl	__l_t_resdnrm
__l_t_resdnrm:
	fmovel	a6@(USER_FPCR),fpcr
	fmovex	a0@(LOCAL_EX),fp0
	fmovel		fpsr,d0
	orl		#__l_unfl_mask,d0
	fmovel		d0,fpsr
	rts
|
|
|
	.globl	__l_t_avoid_unsupp
__l_t_avoid_unsupp:
	fmovex	fp0,fp0
	rts

	.globl	__l_sto_cos
__l_sto_cos:
	fmovemx	a0@(LOCAL_EX),fp1-fp1
	rts
|
|	Native instruction support
|
|	Some systems may need entry points even for 68040 native
|	instructions.  These routines are provided for
|	convenience.
|
	.globl	__l_sadd
__l_sadd:
	fmovemx	a6@(FPTEMP),fp0-fp0
	fmovel	a6@(USER_FPCR),fpcr
	faddx	a6@(ETEMP),fp0
	rts

	.globl	__l_ssub
__l_ssub:
	fmovemx	a6@(FPTEMP),fp0-fp0
	fmovel	a6@(USER_FPCR),fpcr
	fsubx	a6@(ETEMP),fp0
	rts

	.globl	__l_smul
__l_smul:
	fmovemx	a6@(FPTEMP),fp0-fp0
	fmovel	a6@(USER_FPCR),fpcr
	fmulx	a6@(ETEMP),fp0
	rts

	.globl	__l_sdiv
__l_sdiv:
	fmovemx	a6@(FPTEMP),fp0-fp0
	fmovel	a6@(USER_FPCR),fpcr
	fdivx	a6@(ETEMP),fp0
	rts

	.globl	__l_sabs
__l_sabs:
	fmovemx	a6@(ETEMP),fp0-fp0
	fmovel	d1,fpcr
	fabsx	fp0
	rts

	.globl	__l_sneg
__l_sneg:
	fmovemx	a6@(ETEMP),fp0-fp0
	fmovel	d1,fpcr
	fnegx	fp0
	rts

	.globl	__l_ssqrt
__l_ssqrt:
	fmovemx	a6@(ETEMP),fp0-fp0
	fmovel	d1,fpcr
	fsqrtx	fp0
	rts

|
|	__l_l_sint,__l_l_sintrz,__l_l_sintd --- wrapper for fint and fintrz
|
/* 	On entry, move the user's fpcr to USER_FPCR. */
|
|	On return from, we need to pickup the INEX2/AINEX bits
|	that are in USER_FPSR.
|
|	xref	__l_sint
|	xref	__l_sintrz
|	xref	__l_sintd

	.globl	__l_l_sint
__l_l_sint:
	movel	d1,a6@(USER_FPCR)
	jsr	__l_sint
	fmovel	fpsr,d0
	orl	a6@(USER_FPSR),d0
	fmovel	d0,fpsr
	rts

	.globl	__l_l_sintrz
__l_l_sintrz:
	movel	d1,a6@(USER_FPCR)
	jsr	__l_sintrz
	fmovel	fpsr,d0
	orl	a6@(USER_FPSR),d0
	fmovel	d0,fpsr
	rts

	.globl	__l_l_sintd
__l_l_sintd:
	movel	d1,a6@(USER_FPCR)
	jsr	__l_sintd
	fmovel	fpsr,d0
	orl	a6@(USER_FPSR),d0
	fmovel	d0,fpsr
	rts

|	end
