/* l_entry.s - Motorola 68040 FP entry points (LIB) */

/* Copyright 1991-1993 Wind River Systems, Inc. */
	.data
	.globl	_copyright_wind_river
	.long	_copyright_wind_river

/*
modification history
--------------------
01j,28jan97,ms   fixed SPR 7822 (bug from 01i check-in - never part of product)
01i,31may96,ms   updated to mototorola version 2.3
01h,21jul93,kdl  added .text (SPR #2372).
01g,18sep92,kdl  restored title line clobbered in 01f checkin.
01f,18sep92,kdl  changed multi-reg (fpsr/fpcr) saves to use offset USER_FPCR
		 instead of USER_FPSR, to reflect actual save order (SPR #1455).
01e,23aug92,jcf  changed bxxx to jxx.
01d,20aug92,kdl  added changes from Motorola FPSP v2.2 to correctly save fpcr.
01c,26may92,rrr  the tree shuffle
01b,09jan92,kdl  added modification history; general cleanup.
01a,15aug91,kdl  original version, from Motorola FPSP v2.0.
*/

/*
DESCRIPTION

This file contains Motorola FPSP library interface entry points for
various floating point operations.

*/


|	section 8

#include "fpsp040L.h"

|	xref	__l_tag
|	xref	__l_szero
|	xref	__l_sinf
|	xref	__l_sopr_inf
|	xref	__l_sone
|	xref	__l_spi_2
|	xref	__l_szr_inf
|	xref	__l_src_nan
|	xref	__l_t_operr
|	xref	__l_t_dz2
|	xref	__l_snzrinx
|	xref	__l_ld_pone
|	xref	__l_ld_pinf
|	xref	__l_ld_ppi2
|	xref	__l_ssincosz
|	xref	__l_ssincosi
|	xref	__l_ssincosnan
|	xref	__l_setoxm1i
|
|	MONADIC.GEN 1.3 4/30/91
|
|	MONADIC.GEN --- generic MONADIC template
|
|	This version saves all registers that will be used by the emulation
|	routines and restores all but FP0 on exit.  The FPSR is
|	updated to reflect the result of the operation.  Return value
|	is placed in FP0 for single, double and extended results.
|
|	The package subroutines expect the incoming fpcr to be zeroed
|	since they need extended precision to work properly.  The
/* 	'final' fpcr is expected in d1 so that the calculated result */
|	can be properly sized and rounded.  Also, if the incoming fpcr
|	has enabled any exceptions, the exception will be taken on the
|	final fmovem in this template.
|
|	Customizations:
|		1. Remove the moveml at the entry and exit of
|		   each routine if your compiler treats those
|		   registers as scratch.
/* 		2. Likewise, don't save FP0/FP1 if they are scratch */
|		   registers.
|		3. Delete handling of the fpsr if you only care about
|		   the result.
|		4. Some (most?) C compilers convert all float arguments
|		   to double, and provide no support at all for extended
|		   precision so remove the __l_facoss and __l_facosx entry points.
|		5. Move the result to d0/d1 if the compiler is that old.

|		Copyright (C) Motorola, Inc. 1991
|			All Rights Reserved
|
|	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA
|	The copyright notice above does not evidence any
|	actual or intended publication of such source code.

|	xref	__l_tag
|	xref	__l_sacos
|	xref	__l_ld_ppi2
|	xref	__l_t_operr
|	xref	__l_mon_nan
|	xref	__l_sacosd


	.text

	.globl	__l_facoss
__l_facoss:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmoves	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_1012
	bsrl	__l_sacos		|  normalized (regular) number
	jra 	L_1016
L_1012:
	cmpb	#0x20,d0		|  zero?
	jne 	L_1013
	bsrl	__l_ld_ppi2
	jra 	L_1016
L_1013:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_1014
	bsrl	__l_t_operr
	jra 	L_1016
L_1014:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_1015
	bsrl	__l_mon_nan
	jra 	L_1016
L_1015:
	bsrl	__l_sacosd		|  assuming a denorm...

L_1016:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_facosd
__l_facosd:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmoved	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_1017
	bsrl	__l_sacos		|  normalized (regular) number
	jra 	L_101B
L_1017:
	cmpb	#0x20,d0		|  zero?
	jne 	L_1018
	bsrl	__l_ld_ppi2
	jra 	L_101B
L_1018:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_1019
	bsrl	__l_t_operr
	jra 	L_101B
L_1019:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_101A
	bsrl	__l_mon_nan
	jra 	L_101B
L_101A:
	bsrl	__l_sacosd		|  assuming a denorm...

L_101B:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_facosx
__l_facosx:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmovex	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_101C
	bsrl	__l_sacos		|  normalized (regular) number
	jra 	L_101G
L_101C:
	cmpb	#0x20,d0		|  zero?
	jne 	L_101D
	bsrl	__l_ld_ppi2
	jra 	L_101G
L_101D:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_101E
	bsrl	__l_t_operr
	jra 	L_101G
L_101E:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_101F
	bsrl	__l_mon_nan
	jra 	L_101G
L_101F:
	bsrl	__l_sacosd		|  assuming a denorm...

L_101G:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

|
|	MONADIC.GEN 1.3 4/30/91
|
|	MONADIC.GEN --- generic MONADIC template
|
|	This version saves all registers that will be used by the emulation
|	routines and restores all but FP0 on exit.  The FPSR is
|	updated to reflect the result of the operation.  Return value
|	is placed in FP0 for single, double and extended results.
|
|	The package subroutines expect the incoming fpcr to be zeroed
|	since they need extended precision to work properly.  The
/* |	'final' fpcr is expected in d1 so that the calculated result */
|	can be properly sized and rounded.  Also, if the incoming fpcr
|	has enabled any exceptions, the exception will be taken on the
|	final fmovem in this template.
|
|	Customizations:
|		1. Remove the moveml at the entry and exit of
|		   each routine if your compiler treats those
|		   registers as scratch.
/* |		2. Likewise, don't save FP0/FP1 if they are scratch */
|		   registers.
|		3. Delete handling of the fpsr if you only care about
|		   the result.
|		4. Some (most?) C compilers convert all float arguments
|		   to double, and provide no support at all for extended
|		   precision so remove the __l_fasins and __l_fasinx entry points.
|		5. Move the result to d0/d1 if the compiler is that old.

|		Copyright (C) Motorola, Inc. 1991
|			All Rights Reserved
|
|	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA
|	The copyright notice above does not evidence any
|	actual or intended publication of such source code.

|	xref	__l_tag
|	xref	__l_sasin
|	xref	__l_szero
|	xref	__l_t_operr
|	xref	__l_mon_nan
|	xref	__l_sasind

	.globl	__l_fasins
__l_fasins:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmoves	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_1022
	bsrl	__l_sasin		|  normalized (regular) number
	jra 	L_1026
L_1022:
	cmpb	#0x20,d0		|  zero?
	jne 	L_1023
	bsrl	__l_szero
	jra 	L_1026
L_1023:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_1024
	bsrl	__l_t_operr
	jra 	L_1026
L_1024:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_1025
	bsrl	__l_mon_nan
	jra 	L_1026
L_1025:
	bsrl	__l_sasind		|  assuming a denorm...

L_1026:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_fasind
__l_fasind:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmoved	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_1027
	bsrl	__l_sasin		|  normalized (regular) number
	jra 	L_102B
L_1027:
	cmpb	#0x20,d0		|  zero?
	jne 	L_1028
	bsrl	__l_szero
	jra 	L_102B
L_1028:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_1029
	bsrl	__l_t_operr
	jra 	L_102B
L_1029:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_102A
	bsrl	__l_mon_nan
	jra 	L_102B
L_102A:
	bsrl	__l_sasind		|  assuming a denorm...

L_102B:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_fasinx
__l_fasinx:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmovex	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_102C
	bsrl	__l_sasin		|  normalized (regular) number
	jra 	L_102G
L_102C:
	cmpb	#0x20,d0		|  zero?
	jne 	L_102D
	bsrl	__l_szero
	jra 	L_102G
L_102D:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_102E
	bsrl	__l_t_operr
	jra 	L_102G
L_102E:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_102F
	bsrl	__l_mon_nan
	jra 	L_102G
L_102F:
	bsrl	__l_sasind		|  assuming a denorm...

L_102G:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

|
|	MONADIC.GEN 1.3 4/30/91
|
|	MONADIC.GEN --- generic MONADIC template
|
|	This version saves all registers that will be used by the emulation
|	routines and restores all but FP0 on exit.  The FPSR is
|	updated to reflect the result of the operation.  Return value
|	is placed in FP0 for single, double and extended results.
|
|	The package subroutines expect the incoming fpcr to be zeroed
|	since they need extended precision to work properly.  The
/* |	'final' fpcr is expected in d1 so that the calculated result */
|	can be properly sized and rounded.  Also, if the incoming fpcr
|	has enabled any exceptions, the exception will be taken on the
|	final fmovem in this template.
|
|	Customizations:
|		1. Remove the moveml at the entry and exit of
|		   each routine if your compiler treats those
|		   registers as scratch.
/* |		2. Likewise, don't save FP0/FP1 if they are scratch */
|		   registers.
|		3. Delete handling of the fpsr if you only care about
|		   the result.
|		4. Some (most?) C compilers convert all float arguments
|		   to double, and provide no support at all for extended
|		   precision so remove the __l_fatans and __l_fatanx entry points.
|		5. Move the result to d0/d1 if the compiler is that old.

|		Copyright (C) Motorola, Inc. 1991
|			All Rights Reserved
|
|	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA
|	The copyright notice above does not evidence any
|	actual or intended publication of such source code.

|	xref	__l_tag
|	xref	__l_satan
|	xref	__l_szero
|	xref	__l_spi_2
|	xref	__l_mon_nan
|	xref	__l_satand

	.globl	__l_fatans
__l_fatans:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmoves	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_1032
	bsrl	__l_satan		|  normalized (regular) number
	jra 	L_1036
L_1032:
	cmpb	#0x20,d0		|  zero?
	jne 	L_1033
	bsrl	__l_szero
	jra 	L_1036
L_1033:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_1034
	bsrl	__l_spi_2
	jra 	L_1036
L_1034:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_1035
	bsrl	__l_mon_nan
	jra 	L_1036
L_1035:
	bsrl	__l_satand		|  assuming a denorm...

L_1036:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_fatand
__l_fatand:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmoved	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_1037
	bsrl	__l_satan		|  normalized (regular) number
	jra 	L_103B
L_1037:
	cmpb	#0x20,d0		|  zero?
	jne 	L_1038
	bsrl	__l_szero
	jra 	L_103B
L_1038:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_1039
	bsrl	__l_spi_2
	jra 	L_103B
L_1039:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_103A
	bsrl	__l_mon_nan
	jra 	L_103B
L_103A:
	bsrl	__l_satand		|  assuming a denorm...

L_103B:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_fatanx
__l_fatanx:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmovex	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_103C
	bsrl	__l_satan		|  normalized (regular) number
	jra 	L_103G
L_103C:
	cmpb	#0x20,d0		|  zero?
	jne 	L_103D
	bsrl	__l_szero
	jra 	L_103G
L_103D:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_103E
	bsrl	__l_spi_2
	jra 	L_103G
L_103E:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_103F
	bsrl	__l_mon_nan
	jra 	L_103G
L_103F:
	bsrl	__l_satand		|  assuming a denorm...

L_103G:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

|
|	MONADIC.GEN 1.3 4/30/91
|
|	MONADIC.GEN --- generic MONADIC template
|
|	This version saves all registers that will be used by the emulation
|	routines and restores all but FP0 on exit.  The FPSR is
|	updated to reflect the result of the operation.  Return value
|	is placed in FP0 for single, double and extended results.
|
|	The package subroutines expect the incoming fpcr to be zeroed
|	since they need extended precision to work properly.  The
/* |	'final' fpcr is expected in d1 so that the calculated result */
|	can be properly sized and rounded.  Also, if the incoming fpcr
|	has enabled any exceptions, the exception will be taken on the
|	final fmovem in this template.
|
|	Customizations:
|		1. Remove the moveml at the entry and exit of
|		   each routine if your compiler treats those
|		   registers as scratch.
/* |		2. Likewise, don't save FP0/FP1 if they are scratch */
|		   registers.
|		3. Delete handling of the fpsr if you only care about
|		   the result.
|		4. Some (most?) C compilers convert all float arguments
|		   to double, and provide no support at all for extended
|		   precision so remove the __l_fatanhs and __l_fatanhx entry points.
|		5. Move the result to d0/d1 if the compiler is that old.

|		Copyright (C) Motorola, Inc. 1991
|			All Rights Reserved
|
|	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA
|	The copyright notice above does not evidence any
|	actual or intended publication of such source code.

|	xref	__l_tag
|	xref	__l_satanh
|	xref	__l_szero
|	xref	__l_t_operr
|	xref	__l_mon_nan
|	xref	__l_satanhd

	.globl	__l_fatanhs
__l_fatanhs:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmoves	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_1042
	bsrl	__l_satanh		|  normalized (regular) number
	jra 	L_1046
L_1042:
	cmpb	#0x20,d0		|  zero?
	jne 	L_1043
	bsrl	__l_szero
	jra 	L_1046
L_1043:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_1044
	bsrl	__l_t_operr
	jra 	L_1046
L_1044:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_1045
	bsrl	__l_mon_nan
	jra 	L_1046
L_1045:
	bsrl	__l_satanhd		|  assuming a denorm...

L_1046:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_fatanhd
__l_fatanhd:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmoved	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_1047
	bsrl	__l_satanh		|  normalized (regular) number
	jra 	L_104B
L_1047:
	cmpb	#0x20,d0		|  zero?
	jne 	L_1048
	bsrl	__l_szero
	jra 	L_104B
L_1048:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_1049
	bsrl	__l_t_operr
	jra 	L_104B
L_1049:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_104A
	bsrl	__l_mon_nan
	jra 	L_104B
L_104A:
	bsrl	__l_satanhd		|  assuming a denorm...

L_104B:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_fatanhx
__l_fatanhx:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmovex	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_104C
	bsrl	__l_satanh		|  normalized (regular) number
	jra 	L_104G
L_104C:
	cmpb	#0x20,d0		|  zero?
	jne 	L_104D
	bsrl	__l_szero
	jra 	L_104G
L_104D:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_104E
	bsrl	__l_t_operr
	jra 	L_104G
L_104E:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_104F
	bsrl	__l_mon_nan
	jra 	L_104G
L_104F:
	bsrl	__l_satanhd		|  assuming a denorm...

L_104G:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

|
|	MONADIC.GEN 1.3 4/30/91
|
|	MONADIC.GEN --- generic MONADIC template
|
|	This version saves all registers that will be used by the emulation
|	routines and restores all but FP0 on exit.  The FPSR is
|	updated to reflect the result of the operation.  Return value
|	is placed in FP0 for single, double and extended results.
|
|	The package subroutines expect the incoming fpcr to be zeroed
|	since they need extended precision to work properly.  The
/* |	'final' fpcr is expected in d1 so that the calculated result */
|	can be properly sized and rounded.  Also, if the incoming fpcr
|	has enabled any exceptions, the exception will be taken on the
|	final fmovem in this template.
|
|	Customizations:
|		1. Remove the moveml at the entry and exit of
|		   each routine if your compiler treats those
|		   registers as scratch.
/* |		2. Likewise, don't save FP0/FP1 if they are scratch */
|		   registers.
|		3. Delete handling of the fpsr if you only care about
|		   the result.
|		4. Some (most?) C compilers convert all float arguments
|		   to double, and provide no support at all for extended
|		   precision so remove the __l_fcoss and __l_fcosx entry points.
|		5. Move the result to d0/d1 if the compiler is that old.

|		Copyright (C) Motorola, Inc. 1991
|			All Rights Reserved
|
|	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA
|	The copyright notice above does not evidence any
|	actual or intended publication of such source code.

|	xref	__l_tag
|	xref	__l_scos
|	xref	__l_ld_pone
|	xref	__l_t_operr
|	xref	__l_mon_nan
|	xref	__l_scosd

	.globl	__l_fcoss
__l_fcoss:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmoves	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_1052
	bsrl	__l_scos		|  normalized (regular) number
	jra 	L_1056
L_1052:
	cmpb	#0x20,d0		|  zero?
	jne 	L_1053
	bsrl	__l_ld_pone
	jra 	L_1056
L_1053:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_1054
	bsrl	__l_t_operr
	jra 	L_1056
L_1054:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_1055
	bsrl	__l_mon_nan
	jra 	L_1056
L_1055:
	bsrl	__l_scosd		|  assuming a denorm...

L_1056:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_fcosd
__l_fcosd:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmoved	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_1057
	bsrl	__l_scos		|  normalized (regular) number
	jra 	L_105B
L_1057:
	cmpb	#0x20,d0		|  zero?
	jne 	L_1058
	bsrl	__l_ld_pone
	jra 	L_105B
L_1058:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_1059
	bsrl	__l_t_operr
	jra 	L_105B
L_1059:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_105A
	bsrl	__l_mon_nan
	jra 	L_105B
L_105A:
	bsrl	__l_scosd		|  assuming a denorm...

L_105B:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_fcosx
__l_fcosx:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmovex	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_105C
	bsrl	__l_scos		|  normalized (regular) number
	jra 	L_105G
L_105C:
	cmpb	#0x20,d0		|  zero?
	jne 	L_105D
	bsrl	__l_ld_pone
	jra 	L_105G
L_105D:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_105E
	bsrl	__l_t_operr
	jra 	L_105G
L_105E:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_105F
	bsrl	__l_mon_nan
	jra 	L_105G
L_105F:
	bsrl	__l_scosd		|  assuming a denorm...

L_105G:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

|
|	MONADIC.GEN 1.3 4/30/91
|
|	MONADIC.GEN --- generic MONADIC template
|
|	This version saves all registers that will be used by the emulation
|	routines and restores all but FP0 on exit.  The FPSR is
|	updated to reflect the result of the operation.  Return value
|	is placed in FP0 for single, double and extended results.
|
|	The package subroutines expect the incoming fpcr to be zeroed
|	since they need extended precision to work properly.  The
/* |	'final' fpcr is expected in d1 so that the calculated result */
|	can be properly sized and rounded.  Also, if the incoming fpcr
|	has enabled any exceptions, the exception will be taken on the
|	final fmovem in this template.
|
|	Customizations:
|		1. Remove the moveml at the entry and exit of
|		   each routine if your compiler treats those
|		   registers as scratch.
/* |		2. Likewise, don't save FP0/FP1 if they are scratch */
|		   registers.
|		3. Delete handling of the fpsr if you only care about
|		   the result.
|		4. Some (most?) C compilers convert all float arguments
|		   to double, and provide no support at all for extended
|		   precision so remove the __l_fcoshs and __l_fcoshx entry points.
|		5. Move the result to d0/d1 if the compiler is that old.

|		Copyright (C) Motorola, Inc. 1991
|			All Rights Reserved
|
|	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA
|	The copyright notice above does not evidence any
|	actual or intended publication of such source code.

|	xref	__l_tag
|	xref	__l_scosh
|	xref	__l_ld_pone
|	xref	__l_ld_pinf
|	xref	__l_mon_nan
|	xref	__l_scoshd

	.globl	__l_fcoshs
__l_fcoshs:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmoves	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_1062
	bsrl	__l_scosh		|  normalized (regular) number
	jra 	L_1066
L_1062:
	cmpb	#0x20,d0		|  zero?
	jne 	L_1063
	bsrl	__l_ld_pone
	jra 	L_1066
L_1063:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_1064
	bsrl	__l_ld_pinf
	jra 	L_1066
L_1064:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_1065
	bsrl	__l_mon_nan
	jra 	L_1066
L_1065:
	bsrl	__l_scoshd		|  assuming a denorm...

L_1066:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_fcoshd
__l_fcoshd:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmoved	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_1067
	bsrl	__l_scosh		|  normalized (regular) number
	jra 	L_106B
L_1067:
	cmpb	#0x20,d0		|  zero?
	jne 	L_1068
	bsrl	__l_ld_pone
	jra 	L_106B
L_1068:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_1069
	bsrl	__l_ld_pinf
	jra 	L_106B
L_1069:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_106A
	bsrl	__l_mon_nan
	jra 	L_106B
L_106A:
	bsrl	__l_scoshd		|  assuming a denorm...

L_106B:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_fcoshx
__l_fcoshx:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmovex	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_106C
	bsrl	__l_scosh		|  normalized (regular) number
	jra 	L_106G
L_106C:
	cmpb	#0x20,d0		|  zero?
	jne 	L_106D
	bsrl	__l_ld_pone
	jra 	L_106G
L_106D:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_106E
	bsrl	__l_ld_pinf
	jra 	L_106G
L_106E:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_106F
	bsrl	__l_mon_nan
	jra 	L_106G
L_106F:
	bsrl	__l_scoshd		|  assuming a denorm...

L_106G:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

|
|	MONADIC.GEN 1.3 4/30/91
|
|	MONADIC.GEN --- generic MONADIC template
|
|	This version saves all registers that will be used by the emulation
|	routines and restores all but FP0 on exit.  The FPSR is
|	updated to reflect the result of the operation.  Return value
|	is placed in FP0 for single, double and extended results.
|
|	The package subroutines expect the incoming fpcr to be zeroed
|	since they need extended precision to work properly.  The
/* |	'final' fpcr is expected in d1 so that the calculated result */
|	can be properly sized and rounded.  Also, if the incoming fpcr
|	has enabled any exceptions, the exception will be taken on the
|	final fmovem in this template.
|
|	Customizations:
|		1. Remove the moveml at the entry and exit of
|		   each routine if your compiler treats those
|		   registers as scratch.
/* |		2. Likewise, don't save FP0/FP1 if they are scratch */
|		   registers.
|		3. Delete handling of the fpsr if you only care about
|		   the result.
|		4. Some (most?) C compilers convert all float arguments
|		   to double, and provide no support at all for extended
|		   precision so remove the __l_fetoxs and __l_fetoxx entry points.
|		5. Move the result to d0/d1 if the compiler is that old.

|		Copyright (C) Motorola, Inc. 1991
|			All Rights Reserved
|
|	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA
|	The copyright notice above does not evidence any
|	actual or intended publication of such source code.

|	xref	__l_tag
|	xref	__l_setox
|	xref	__l_ld_pone
|	xref	__l_szr_inf
|	xref	__l_mon_nan
|	xref	__l_setoxd

	.globl	__l_fetoxs
__l_fetoxs:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmoves	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_1072
	bsrl	__l_setox		|  normalized (regular) number
	jra 	L_1076
L_1072:
	cmpb	#0x20,d0		|  zero?
	jne 	L_1073
	bsrl	__l_ld_pone
	jra 	L_1076
L_1073:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_1074
	bsrl	__l_szr_inf
	jra 	L_1076
L_1074:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_1075
	bsrl	__l_mon_nan
	jra 	L_1076
L_1075:
	bsrl	__l_setoxd		|  assuming a denorm...

L_1076:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_fetoxd
__l_fetoxd:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmoved	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_1077
	bsrl	__l_setox		|  normalized (regular) number
	jra 	L_107B
L_1077:
	cmpb	#0x20,d0		|  zero?
	jne 	L_1078
	bsrl	__l_ld_pone
	jra 	L_107B
L_1078:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_1079
	bsrl	__l_szr_inf
	jra 	L_107B
L_1079:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_107A
	bsrl	__l_mon_nan
	jra 	L_107B
L_107A:
	bsrl	__l_setoxd		|  assuming a denorm...

L_107B:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_fetoxx
__l_fetoxx:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmovex	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_107C
	bsrl	__l_setox		|  normalized (regular) number
	jra 	L_107G
L_107C:
	cmpb	#0x20,d0		|  zero?
	jne 	L_107D
	bsrl	__l_ld_pone
	jra 	L_107G
L_107D:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_107E
	bsrl	__l_szr_inf
	jra 	L_107G
L_107E:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_107F
	bsrl	__l_mon_nan
	jra 	L_107G
L_107F:
	bsrl	__l_setoxd		|  assuming a denorm...

L_107G:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

|
|	MONADIC.GEN 1.3 4/30/91
|
|	MONADIC.GEN --- generic MONADIC template
|
|	This version saves all registers that will be used by the emulation
|	routines and restores all but FP0 on exit.  The FPSR is
|	updated to reflect the result of the operation.  Return value
|	is placed in FP0 for single, double and extended results.
|
|	The package subroutines expect the incoming fpcr to be zeroed
|	since they need extended precision to work properly.  The
/* |	'final' fpcr is expected in d1 so that the calculated result */
|	can be properly sized and rounded.  Also, if the incoming fpcr
|	has enabled any exceptions, the exception will be taken on the
|	final fmovem in this template.
|
|	Customizations:
|		1. Remove the moveml at the entry and exit of
|		   each routine if your compiler treats those
|		   registers as scratch.
/* |		2. Likewise, don't save FP0/FP1 if they are scratch */
|		   registers.
|		3. Delete handling of the fpsr if you only care about
|		   the result.
|		4. Some (most?) C compilers convert all float arguments
|		   to double, and provide no support at all for extended
|		   precision so remove the __l_fetoxm1s and __l_fetoxm1x entry points.
|		5. Move the result to d0/d1 if the compiler is that old.

|		Copyright (C) Motorola, Inc. 1991
|			All Rights Reserved
|
|	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA
|	The copyright notice above does not evidence any
|	actual or intended publication of such source code.

|	xref	__l_tag
|	xref	__l_setoxm1
|	xref	__l_szero
|	xref	__l_setoxm1i
|	xref	__l_mon_nan
|	xref	__l_setoxm1d

	.globl	__l_fetoxm1s
__l_fetoxm1s:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmoves	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_1082
	bsrl	__l_setoxm1		|  normalized (regular) number
	jra 	L_1086
L_1082:
	cmpb	#0x20,d0		|  zero?
	jne 	L_1083
	bsrl	__l_szero
	jra 	L_1086
L_1083:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_1084
	bsrl	__l_setoxm1i
	jra 	L_1086
L_1084:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_1085
	bsrl	__l_mon_nan
	jra 	L_1086
L_1085:
	bsrl	__l_setoxm1d		|  assuming a denorm...

L_1086:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_fetoxm1d
__l_fetoxm1d:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmoved	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_1087
	bsrl	__l_setoxm1		|  normalized (regular) number
	jra 	L_108B
L_1087:
	cmpb	#0x20,d0		|  zero?
	jne 	L_1088
	bsrl	__l_szero
	jra 	L_108B
L_1088:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_1089
	bsrl	__l_setoxm1i
	jra 	L_108B
L_1089:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_108A
	bsrl	__l_mon_nan
	jra 	L_108B
L_108A:
	bsrl	__l_setoxm1d		|  assuming a denorm...

L_108B:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_fetoxm1x
__l_fetoxm1x:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmovex	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_108C
	bsrl	__l_setoxm1		|  normalized (regular) number
	jra 	L_108G
L_108C:
	cmpb	#0x20,d0		|  zero?
	jne 	L_108D
	bsrl	__l_szero
	jra 	L_108G
L_108D:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_108E
	bsrl	__l_setoxm1i
	jra 	L_108G
L_108E:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_108F
	bsrl	__l_mon_nan
	jra 	L_108G
L_108F:
	bsrl	__l_setoxm1d		|  assuming a denorm...

L_108G:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

|
|	MONADIC.GEN 1.3 4/30/91
|
|	MONADIC.GEN --- generic MONADIC template
|
|	This version saves all registers that will be used by the emulation
|	routines and restores all but FP0 on exit.  The FPSR is
|	updated to reflect the result of the operation.  Return value
|	is placed in FP0 for single, double and extended results.
|
|	The package subroutines expect the incoming fpcr to be zeroed
|	since they need extended precision to work properly.  The
/* |	'final' fpcr is expected in d1 so that the calculated result */
|	can be properly sized and rounded.  Also, if the incoming fpcr
|	has enabled any exceptions, the exception will be taken on the
|	final fmovem in this template.
|
|	Customizations:
|		1. Remove the moveml at the entry and exit of
|		   each routine if your compiler treats those
|		   registers as scratch.
/* |		2. Likewise, don't save FP0/FP1 if they are scratch */
|		   registers.
|		3. Delete handling of the fpsr if you only care about
|		   the result.
|		4. Some (most?) C compilers convert all float arguments
|		   to double, and provide no support at all for extended
|		   precision so remove the __l_fgetexps and __l_fgetexpx entry points.
|		5. Move the result to d0/d1 if the compiler is that old.

|		Copyright (C) Motorola, Inc. 1991
|			All Rights Reserved
|
|	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA
|	The copyright notice above does not evidence any
|	actual or intended publication of such source code.

|	xref	__l_tag
|	xref	__l_sgetexp
|	xref	__l_szero
|	xref	__l_t_operr
|	xref	__l_mon_nan
|	xref	__l_sgetexpd

	.globl	__l_fgetexps
__l_fgetexps:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmoves	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_1092
	bsrl	__l_sgetexp		|  normalized (regular) number
	jra 	L_1096
L_1092:
	cmpb	#0x20,d0		|  zero?
	jne 	L_1093
	bsrl	__l_szero
	jra 	L_1096
L_1093:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_1094
	bsrl	__l_t_operr
	jra 	L_1096
L_1094:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_1095
	bsrl	__l_mon_nan
	jra 	L_1096
L_1095:
	bsrl	__l_sgetexpd		|  assuming a denorm...

L_1096:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_fgetexpd
__l_fgetexpd:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmoved	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_1097
	bsrl	__l_sgetexp		|  normalized (regular) number
	jra 	L_109B
L_1097:
	cmpb	#0x20,d0		|  zero?
	jne 	L_1098
	bsrl	__l_szero
	jra 	L_109B
L_1098:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_1099
	bsrl	__l_t_operr
	jra 	L_109B
L_1099:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_109A
	bsrl	__l_mon_nan
	jra 	L_109B
L_109A:
	bsrl	__l_sgetexpd		|  assuming a denorm...

L_109B:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_fgetexpx
__l_fgetexpx:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmovex	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_109C
	bsrl	__l_sgetexp		|  normalized (regular) number
	jra 	L_109G
L_109C:
	cmpb	#0x20,d0		|  zero?
	jne 	L_109D
	bsrl	__l_szero
	jra 	L_109G
L_109D:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_109E
	bsrl	__l_t_operr
	jra 	L_109G
L_109E:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_109F
	bsrl	__l_mon_nan
	jra 	L_109G
L_109F:
	bsrl	__l_sgetexpd		|  assuming a denorm...

L_109G:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

|
|	MONADIC.GEN 1.3 4/30/91
|
|	MONADIC.GEN --- generic MONADIC template
|
|	This version saves all registers that will be used by the emulation
|	routines and restores all but FP0 on exit.  The FPSR is
|	updated to reflect the result of the operation.  Return value
|	is placed in FP0 for single, double and extended results.
|
|	The package subroutines expect the incoming fpcr to be zeroed
|	since they need extended precision to work properly.  The
/* |	'final' fpcr is expected in d1 so that the calculated result */
|	can be properly sized and rounded.  Also, if the incoming fpcr
|	has enabled any exceptions, the exception will be taken on the
|	final fmovem in this template.
|
|	Customizations:
|		1. Remove the moveml at the entry and exit of
|		   each routine if your compiler treats those
|		   registers as scratch.
/* |		2. Likewise, don't save FP0/FP1 if they are scratch */
|		   registers.
|		3. Delete handling of the fpsr if you only care about
|		   the result.
|		4. Some (most?) C compilers convert all float arguments
|		   to double, and provide no support at all for extended
|		   precision so remove the __l_fsins and __l_fsinx entry points.
|		5. Move the result to d0/d1 if the compiler is that old.

|		Copyright (C) Motorola, Inc. 1991
|			All Rights Reserved
|
|	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA
|	The copyright notice above does not evidence any
|	actual or intended publication of such source code.

|	xref	__l_tag
|	xref	__l_ssin
|	xref	__l_szero
|	xref	__l_t_operr
|	xref	__l_mon_nan
|	xref	__l_ssind

	.globl	__l_fsins
__l_fsins:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmoves	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_1102
	bsrl	__l_ssin		|  normalized (regular) number
	jra 	L_1106
L_1102:
	cmpb	#0x20,d0		|  zero?
	jne 	L_1103
	bsrl	__l_szero
	jra 	L_1106
L_1103:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_1104
	bsrl	__l_t_operr
	jra 	L_1106
L_1104:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_1105
	bsrl	__l_mon_nan
	jra 	L_1106
L_1105:
	bsrl	__l_ssind		|  assuming a denorm...

L_1106:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_fsind
__l_fsind:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmoved	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_1107
	bsrl	__l_ssin		|  normalized (regular) number
	jra 	L_110B
L_1107:
	cmpb	#0x20,d0		|  zero?
	jne 	L_1108
	bsrl	__l_szero
	jra 	L_110B
L_1108:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_1109
	bsrl	__l_t_operr
	jra 	L_110B
L_1109:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_110A
	bsrl	__l_mon_nan
	jra 	L_110B
L_110A:
	bsrl	__l_ssind		|  assuming a denorm...

L_110B:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_fsinx
__l_fsinx:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmovex	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_110C
	bsrl	__l_ssin		|  normalized (regular) number
	jra 	L_110G
L_110C:
	cmpb	#0x20,d0		|  zero?
	jne 	L_110D
	bsrl	__l_szero
	jra 	L_110G
L_110D:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_110E
	bsrl	__l_t_operr
	jra 	L_110G
L_110E:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_110F
	bsrl	__l_mon_nan
	jra 	L_110G
L_110F:
	bsrl	__l_ssind		|  assuming a denorm...

L_110G:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

|
|	MONADIC.GEN 1.3 4/30/91
|
|	MONADIC.GEN --- generic MONADIC template
|
|	This version saves all registers that will be used by the emulation
|	routines and restores all but FP0 on exit.  The FPSR is
|	updated to reflect the result of the operation.  Return value
|	is placed in FP0 for single, double and extended results.
|
|	The package subroutines expect the incoming fpcr to be zeroed
|	since they need extended precision to work properly.  The
/* |	'final' fpcr is expected in d1 so that the calculated result */
|	can be properly sized and rounded.  Also, if the incoming fpcr
|	has enabled any exceptions, the exception will be taken on the
|	final fmovem in this template.
|
|	Customizations:
|		1. Remove the moveml at the entry and exit of
|		   each routine if your compiler treats those
|		   registers as scratch.
/* |		2. Likewise, don't save FP0/FP1 if they are scratch */
|		   registers.
|		3. Delete handling of the fpsr if you only care about
|		   the result.
|		4. Some (most?) C compilers convert all float arguments
|		   to double, and provide no support at all for extended
|		   precision so remove the __l_fsinhs and __l_fsinhx entry points.
|		5. Move the result to d0/d1 if the compiler is that old.

|		Copyright (C) Motorola, Inc. 1991
|			All Rights Reserved
|
|	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA
|	The copyright notice above does not evidence any
|	actual or intended publication of such source code.

|	xref	__l_tag
|	xref	__l_ssinh
|	xref	__l_szero
|	xref	__l_sinf
|	xref	__l_mon_nan
|	xref	__l_ssinhd

	.globl	__l_fsinhs
__l_fsinhs:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmoves	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_1112
	bsrl	__l_ssinh		|  normalized (regular) number
	jra 	L_1116
L_1112:
	cmpb	#0x20,d0		|  zero?
	jne 	L_1113
	bsrl	__l_szero
	jra 	L_1116
L_1113:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_1114
	bsrl	__l_sinf
	jra 	L_1116
L_1114:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_1115
	bsrl	__l_mon_nan
	jra 	L_1116
L_1115:
	bsrl	__l_ssinhd		|  assuming a denorm...

L_1116:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_fsinhd
__l_fsinhd:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmoved	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_1117
	bsrl	__l_ssinh		|  normalized (regular) number
	jra 	L_111B
L_1117:
	cmpb	#0x20,d0		|  zero?
	jne 	L_1118
	bsrl	__l_szero
	jra 	L_111B
L_1118:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_1119
	bsrl	__l_sinf
	jra 	L_111B
L_1119:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_111A
	bsrl	__l_mon_nan
	jra 	L_111B
L_111A:
	bsrl	__l_ssinhd		|  assuming a denorm...

L_111B:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_fsinhx
__l_fsinhx:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmovex	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_111C
	bsrl	__l_ssinh		|  normalized (regular) number
	jra 	L_111G
L_111C:
	cmpb	#0x20,d0		|  zero?
	jne 	L_111D
	bsrl	__l_szero
	jra 	L_111G
L_111D:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_111E
	bsrl	__l_sinf
	jra 	L_111G
L_111E:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_111F
	bsrl	__l_mon_nan
	jra 	L_111G
L_111F:
	bsrl	__l_ssinhd		|  assuming a denorm...

L_111G:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

|
|	MONADIC.GEN 1.3 4/30/91
|
|	MONADIC.GEN --- generic MONADIC template
|
|	This version saves all registers that will be used by the emulation
|	routines and restores all but FP0 on exit.  The FPSR is
|	updated to reflect the result of the operation.  Return value
|	is placed in FP0 for single, double and extended results.
|
|	The package subroutines expect the incoming fpcr to be zeroed
|	since they need extended precision to work properly.  The
/* |	'final' fpcr is expected in d1 so that the calculated result */
|	can be properly sized and rounded.  Also, if the incoming fpcr
|	has enabled any exceptions, the exception will be taken on the
|	final fmovem in this template.
|
|	Customizations:
|		1. Remove the moveml at the entry and exit of
|		   each routine if your compiler treats those
|		   registers as scratch.
/* |		2. Likewise, don't save FP0/FP1 if they are scratch */
|		   registers.
|		3. Delete handling of the fpsr if you only care about
|		   the result.
|		4. Some (most?) C compilers convert all float arguments
|		   to double, and provide no support at all for extended
|		   precision so remove the __l_ftans and __l_ftanx entry points.
|		5. Move the result to d0/d1 if the compiler is that old.

|		Copyright (C) Motorola, Inc. 1991
|			All Rights Reserved
|
|	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA
|	The copyright notice above does not evidence any
|	actual or intended publication of such source code.

|	xref	__l_tag
|	xref	__l_stan
|	xref	__l_szero
|	xref	__l_t_operr
|	xref	__l_mon_nan
|	xref	__l_stand

	.globl	__l_ftans
__l_ftans:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmoves	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_1122
	bsrl	__l_stan		|  normalized (regular) number
	jra 	L_1126
L_1122:
	cmpb	#0x20,d0		|  zero?
	jne 	L_1123
	bsrl	__l_szero
	jra 	L_1126
L_1123:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_1124
	bsrl	__l_t_operr
	jra 	L_1126
L_1124:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_1125
	bsrl	__l_mon_nan
	jra 	L_1126
L_1125:
	bsrl	__l_stand		|  assuming a denorm...

L_1126:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_ftand
__l_ftand:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmoved	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_1127
	bsrl	__l_stan		|  normalized (regular) number
	jra 	L_112B
L_1127:
	cmpb	#0x20,d0		|  zero?
	jne 	L_1128
	bsrl	__l_szero
	jra 	L_112B
L_1128:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_1129
	bsrl	__l_t_operr
	jra 	L_112B
L_1129:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_112A
	bsrl	__l_mon_nan
	jra 	L_112B
L_112A:
	bsrl	__l_stand		|  assuming a denorm...

L_112B:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_ftanx
__l_ftanx:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmovex	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_112C
	bsrl	__l_stan		|  normalized (regular) number
	jra 	L_112G
L_112C:
	cmpb	#0x20,d0		|  zero?
	jne 	L_112D
	bsrl	__l_szero
	jra 	L_112G
L_112D:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_112E
	bsrl	__l_t_operr
	jra 	L_112G
L_112E:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_112F
	bsrl	__l_mon_nan
	jra 	L_112G
L_112F:
	bsrl	__l_stand		|  assuming a denorm...

L_112G:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

|
|	MONADIC.GEN 1.3 4/30/91
|
|	MONADIC.GEN --- generic MONADIC template
|
|	This version saves all registers that will be used by the emulation
|	routines and restores all but FP0 on exit.  The FPSR is
|	updated to reflect the result of the operation.  Return value
|	is placed in FP0 for single, double and extended results.
|
|	The package subroutines expect the incoming fpcr to be zeroed
|	since they need extended precision to work properly.  The
/* |	'final' fpcr is expected in d1 so that the calculated result */
|	can be properly sized and rounded.  Also, if the incoming fpcr
|	has enabled any exceptions, the exception will be taken on the
|	final fmovem in this template.
|
|	Customizations:
|		1. Remove the moveml at the entry and exit of
|		   each routine if your compiler treats those
|		   registers as scratch.
/* |		2. Likewise, don't save FP0/FP1 if they are scratch */
|		   registers.
|		3. Delete handling of the fpsr if you only care about
|		   the result.
|		4. Some (most?) C compilers convert all float arguments
|		   to double, and provide no support at all for extended
|		   precision so remove the __l_ftanhs and __l_ftanhx entry points.
|		5. Move the result to d0/d1 if the compiler is that old.

|		Copyright (C) Motorola, Inc. 1991
|			All Rights Reserved
|
|	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA
|	The copyright notice above does not evidence any
|	actual or intended publication of such source code.

|	xref	__l_tag
|	xref	__l_stanh
|	xref	__l_szero
|	xref	__l_sone
|	xref	__l_mon_nan
|	xref	__l_stanhd

	.globl	__l_ftanhs
__l_ftanhs:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmoves	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_1132
	bsrl	__l_stanh		|  normalized (regular) number
	jra 	L_1136
L_1132:
	cmpb	#0x20,d0		|  zero?
	jne 	L_1133
	bsrl	__l_szero
	jra 	L_1136
L_1133:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_1134
	bsrl	__l_sone
	jra 	L_1136
L_1134:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_1135
	bsrl	__l_mon_nan
	jra 	L_1136
L_1135:
	bsrl	__l_stanhd		|  assuming a denorm...

L_1136:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_ftanhd
__l_ftanhd:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmoved	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_1137
	bsrl	__l_stanh		|  normalized (regular) number
	jra 	L_113B
L_1137:
	cmpb	#0x20,d0		|  zero?
	jne 	L_1138
	bsrl	__l_szero
	jra 	L_113B
L_1138:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_1139
	bsrl	__l_sone
	jra 	L_113B
L_1139:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_113A
	bsrl	__l_mon_nan
	jra 	L_113B
L_113A:
	bsrl	__l_stanhd		|  assuming a denorm...

L_113B:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_ftanhx
__l_ftanhx:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmovex	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_113C
	bsrl	__l_stanh		|  normalized (regular) number
	jra 	L_113G
L_113C:
	cmpb	#0x20,d0		|  zero?
	jne 	L_113D
	bsrl	__l_szero
	jra 	L_113G
L_113D:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_113E
	bsrl	__l_sone
	jra 	L_113G
L_113E:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_113F
	bsrl	__l_mon_nan
	jra 	L_113G
L_113F:
	bsrl	__l_stanhd		|  assuming a denorm...

L_113G:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

|
|	MONADIC.GEN 1.3 4/30/91
|
|	MONADIC.GEN --- generic MONADIC template
|
|	This version saves all registers that will be used by the emulation
|	routines and restores all but FP0 on exit.  The FPSR is
|	updated to reflect the result of the operation.  Return value
|	is placed in FP0 for single, double and extended results.
|
|	The package subroutines expect the incoming fpcr to be zeroed
|	since they need extended precision to work properly.  The
/* |	'final' fpcr is expected in d1 so that the calculated result */
|	can be properly sized and rounded.  Also, if the incoming fpcr
|	has enabled any exceptions, the exception will be taken on the
|	final fmovem in this template.
|
|	Customizations:
|		1. Remove the moveml at the entry and exit of
|		   each routine if your compiler treats those
|		   registers as scratch.
/* |		2. Likewise, don't save FP0/FP1 if they are scratch */
|		   registers.
|		3. Delete handling of the fpsr if you only care about
|		   the result.
|		4. Some (most?) C compilers convert all float arguments
|		   to double, and provide no support at all for extended
|		   precision so remove the __l_ftentoxs and __l_ftentoxx entry points.
|		5. Move the result to d0/d1 if the compiler is that old.

|		Copyright (C) Motorola, Inc. 1991
|			All Rights Reserved
|
|	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA
|	The copyright notice above does not evidence any
|	actual or intended publication of such source code.

|	xref	__l_tag
|	xref	__l_stentox
|	xref	__l_ld_pone
|	xref	__l_szr_inf
|	xref	__l_mon_nan
|	xref	__l_stentoxd

	.globl	__l_ftentoxs
__l_ftentoxs:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmoves	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_1142
	bsrl	__l_stentox		|  normalized (regular) number
	jra 	L_1146
L_1142:
	cmpb	#0x20,d0		|  zero?
	jne 	L_1143
	bsrl	__l_ld_pone
	jra 	L_1146
L_1143:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_1144
	bsrl	__l_szr_inf
	jra 	L_1146
L_1144:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_1145
	bsrl	__l_mon_nan
	jra 	L_1146
L_1145:
	bsrl	__l_stentoxd		|  assuming a denorm...

L_1146:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_ftentoxd
__l_ftentoxd:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmoved	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_1147
	bsrl	__l_stentox		|  normalized (regular) number
	jra 	L_114B
L_1147:
	cmpb	#0x20,d0		|  zero?
	jne 	L_1148
	bsrl	__l_ld_pone
	jra 	L_114B
L_1148:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_1149
	bsrl	__l_szr_inf
	jra 	L_114B
L_1149:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_114A
	bsrl	__l_mon_nan
	jra 	L_114B
L_114A:
	bsrl	__l_stentoxd		|  assuming a denorm...

L_114B:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_ftentoxx
__l_ftentoxx:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmovex	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_114C
	bsrl	__l_stentox		|  normalized (regular) number
	jra 	L_114G
L_114C:
	cmpb	#0x20,d0		|  zero?
	jne 	L_114D
	bsrl	__l_ld_pone
	jra 	L_114G
L_114D:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_114E
	bsrl	__l_szr_inf
	jra 	L_114G
L_114E:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_114F
	bsrl	__l_mon_nan
	jra 	L_114G
L_114F:
	bsrl	__l_stentoxd		|  assuming a denorm...

L_114G:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

|
|	MONADIC.GEN 1.3 4/30/91
|
|	MONADIC.GEN --- generic MONADIC template
|
|	This version saves all registers that will be used by the emulation
|	routines and restores all but FP0 on exit.  The FPSR is
|	updated to reflect the result of the operation.  Return value
|	is placed in FP0 for single, double and extended results.
|
|	The package subroutines expect the incoming fpcr to be zeroed
|	since they need extended precision to work properly.  The
/* |	'final' fpcr is expected in d1 so that the calculated result */
|	can be properly sized and rounded.  Also, if the incoming fpcr
|	has enabled any exceptions, the exception will be taken on the
|	final fmovem in this template.
|
|	Customizations:
|		1. Remove the moveml at the entry and exit of
|		   each routine if your compiler treats those
|		   registers as scratch.
/* |		2. Likewise, don't save FP0/FP1 if they are scratch */
|		   registers.
|		3. Delete handling of the fpsr if you only care about
|		   the result.
|		4. Some (most?) C compilers convert all float arguments
|		   to double, and provide no support at all for extended
|		   precision so remove the __l_ftwotoxs and __l_ftwotoxx entry points.
|		5. Move the result to d0/d1 if the compiler is that old.

|		Copyright (C) Motorola, Inc. 1991
|			All Rights Reserved
|
|	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA
|	The copyright notice above does not evidence any
|	actual or intended publication of such source code.

|	xref	__l_tag
|	xref	__l_stwotox
|	xref	__l_ld_pone
|	xref	__l_szr_inf
|	xref	__l_mon_nan
|	xref	__l_stwotoxd

	.globl	__l_ftwotoxs
__l_ftwotoxs:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmoves	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_1152
	bsrl	__l_stwotox		|  normalized (regular) number
	jra 	L_1156
L_1152:
	cmpb	#0x20,d0		|  zero?
	jne 	L_1153
	bsrl	__l_ld_pone
	jra 	L_1156
L_1153:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_1154
	bsrl	__l_szr_inf
	jra 	L_1156
L_1154:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_1155
	bsrl	__l_mon_nan
	jra 	L_1156
L_1155:
	bsrl	__l_stwotoxd		|  assuming a denorm...

L_1156:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_ftwotoxd
__l_ftwotoxd:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmoved	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_1157
	bsrl	__l_stwotox		|  normalized (regular) number
	jra 	L_115B
L_1157:
	cmpb	#0x20,d0		|  zero?
	jne 	L_1158
	bsrl	__l_ld_pone
	jra 	L_115B
L_1158:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_1159
	bsrl	__l_szr_inf
	jra 	L_115B
L_1159:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_115A
	bsrl	__l_mon_nan
	jra 	L_115B
L_115A:
	bsrl	__l_stwotoxd		|  assuming a denorm...

L_115B:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_ftwotoxx
__l_ftwotoxx:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmovex	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_115C
	bsrl	__l_stwotox		|  normalized (regular) number
	jra 	L_115G
L_115C:
	cmpb	#0x20,d0		|  zero?
	jne 	L_115D
	bsrl	__l_ld_pone
	jra 	L_115G
L_115D:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_115E
	bsrl	__l_szr_inf
	jra 	L_115G
L_115E:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_115F
	bsrl	__l_mon_nan
	jra 	L_115G
L_115F:
	bsrl	__l_stwotoxd		|  assuming a denorm...

L_115G:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

|
|	MONADIC.GEN 1.3 4/30/91
|
|	MONADIC.GEN --- generic MONADIC template
|
|	This version saves all registers that will be used by the emulation
|	routines and restores all but FP0 on exit.  The FPSR is
|	updated to reflect the result of the operation.  Return value
|	is placed in FP0 for single, double and extended results.
|
|	The package subroutines expect the incoming fpcr to be zeroed
|	since they need extended precision to work properly.  The
/* |	'final' fpcr is expected in d1 so that the calculated result */
|	can be properly sized and rounded.  Also, if the incoming fpcr
|	has enabled any exceptions, the exception will be taken on the
|	final fmovem in this template.
|
|	Customizations:
|		1. Remove the moveml at the entry and exit of
|		   each routine if your compiler treats those
|		   registers as scratch.
/* |		2. Likewise, don't save FP0/FP1 if they are scratch */
|		   registers.
|		3. Delete handling of the fpsr if you only care about
|		   the result.
|		4. Some (most?) C compilers convert all float arguments
|		   to double, and provide no support at all for extended
|		   precision so remove the __l_fgetmans and __l_fgetmanx entry points.
|		5. Move the result to d0/d1 if the compiler is that old.

|		Copyright (C) Motorola, Inc. 1991
|			All Rights Reserved
|
|	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA
|	The copyright notice above does not evidence any
|	actual or intended publication of such source code.

|	xref	__l_tag
|	xref	__l_sgetman
|	xref	__l_szero
|	xref	__l_t_operr
|	xref	__l_mon_nan
|	xref	__l_sgetmand

	.globl	__l_fgetmans
__l_fgetmans:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmoves	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_1162
	bsrl	__l_sgetman		|  normalized (regular) number
	jra 	L_1166
L_1162:
	cmpb	#0x20,d0		|  zero?
	jne 	L_1163
	bsrl	__l_szero
	jra 	L_1166
L_1163:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_1164
	bsrl	__l_t_operr
	jra 	L_1166
L_1164:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_1165
	bsrl	__l_mon_nan
	jra 	L_1166
L_1165:
	bsrl	__l_sgetmand		|  assuming a denorm...

L_1166:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_fgetmand
__l_fgetmand:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmoved	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_1167
	bsrl	__l_sgetman		|  normalized (regular) number
	jra 	L_116B
L_1167:
	cmpb	#0x20,d0		|  zero?
	jne 	L_1168
	bsrl	__l_szero
	jra 	L_116B
L_1168:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_1169
	bsrl	__l_t_operr
	jra 	L_116B
L_1169:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_116A
	bsrl	__l_mon_nan
	jra 	L_116B
L_116A:
	bsrl	__l_sgetmand		|  assuming a denorm...

L_116B:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_fgetmanx
__l_fgetmanx:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmovex	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_116C
	bsrl	__l_sgetman		|  normalized (regular) number
	jra 	L_116G
L_116C:
	cmpb	#0x20,d0		|  zero?
	jne 	L_116D
	bsrl	__l_szero
	jra 	L_116G
L_116D:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_116E
	bsrl	__l_t_operr
	jra 	L_116G
L_116E:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_116F
	bsrl	__l_mon_nan
	jra 	L_116G
L_116F:
	bsrl	__l_sgetmand		|  assuming a denorm...

L_116G:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

|
|	MONADIC.GEN 1.3 4/30/91
|
|	MONADIC.GEN --- generic MONADIC template
|
|	This version saves all registers that will be used by the emulation
|	routines and restores all but FP0 on exit.  The FPSR is
|	updated to reflect the result of the operation.  Return value
|	is placed in FP0 for single, double and extended results.
|
|	The package subroutines expect the incoming fpcr to be zeroed
|	since they need extended precision to work properly.  The
/* |	'final' fpcr is expected in d1 so that the calculated result */
|	can be properly sized and rounded.  Also, if the incoming fpcr
|	has enabled any exceptions, the exception will be taken on the
|	final fmovem in this template.
|
|	Customizations:
|		1. Remove the moveml at the entry and exit of
|		   each routine if your compiler treats those
|		   registers as scratch.
/* |		2. Likewise, don't save FP0/FP1 if they are scratch */
|		   registers.
|		3. Delete handling of the fpsr if you only care about
|		   the result.
|		4. Some (most?) C compilers convert all float arguments
|		   to double, and provide no support at all for extended
|		   precision so remove the __l_flogns and __l_flognx entry points.
|		5. Move the result to d0/d1 if the compiler is that old.

|		Copyright (C) Motorola, Inc. 1991
|			All Rights Reserved
|
|	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA
|	The copyright notice above does not evidence any
|	actual or intended publication of such source code.

|	xref	__l_tag
|	xref	__l_sslogn
|	xref	__l_t_dz2
|	xref	__l_sopr_inf
|	xref	__l_mon_nan
|	xref	__l_sslognd

	.globl	__l_flogns
__l_flogns:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmoves	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_1172
	bsrl	__l_sslogn		|  normalized (regular) number
	jra 	L_1176
L_1172:
	cmpb	#0x20,d0		|  zero?
	jne 	L_1173
	bsrl	__l_t_dz2
	jra 	L_1176
L_1173:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_1174
	bsrl	__l_sopr_inf
	jra 	L_1176
L_1174:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_1175
	bsrl	__l_mon_nan
	jra 	L_1176
L_1175:
	bsrl	__l_sslognd		|  assuming a denorm...

L_1176:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_flognd
__l_flognd:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmoved	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_1177
	bsrl	__l_sslogn		|  normalized (regular) number
	jra 	L_117B
L_1177:
	cmpb	#0x20,d0		|  zero?
	jne 	L_1178
	bsrl	__l_t_dz2
	jra 	L_117B
L_1178:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_1179
	bsrl	__l_sopr_inf
	jra 	L_117B
L_1179:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_117A
	bsrl	__l_mon_nan
	jra 	L_117B
L_117A:
	bsrl	__l_sslognd		|  assuming a denorm...

L_117B:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_flognx
__l_flognx:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmovex	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_117C
	bsrl	__l_sslogn		|  normalized (regular) number
	jra 	L_117G
L_117C:
	cmpb	#0x20,d0		|  zero?
	jne 	L_117D
	bsrl	__l_t_dz2
	jra 	L_117G
L_117D:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_117E
	bsrl	__l_sopr_inf
	jra 	L_117G
L_117E:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_117F
	bsrl	__l_mon_nan
	jra 	L_117G
L_117F:
	bsrl	__l_sslognd		|  assuming a denorm...

L_117G:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

|
|	MONADIC.GEN 1.3 4/30/91
|
|	MONADIC.GEN --- generic MONADIC template
|
|	This version saves all registers that will be used by the emulation
|	routines and restores all but FP0 on exit.  The FPSR is
|	updated to reflect the result of the operation.  Return value
|	is placed in FP0 for single, double and extended results.
|
|	The package subroutines expect the incoming fpcr to be zeroed
|	since they need extended precision to work properly.  The
/* |	'final' fpcr is expected in d1 so that the calculated result */
|	can be properly sized and rounded.  Also, if the incoming fpcr
|	has enabled any exceptions, the exception will be taken on the
|	final fmovem in this template.
|
|	Customizations:
|		1. Remove the moveml at the entry and exit of
|		   each routine if your compiler treats those
|		   registers as scratch.
/* |		2. Likewise, don't save FP0/FP1 if they are scratch */
|		   registers.
|		3. Delete handling of the fpsr if you only care about
|		   the result.
|		4. Some (most?) C compilers convert all float arguments
|		   to double, and provide no support at all for extended
|		   precision so remove the __l_flog2s and __l_flog2x entry points.
|		5. Move the result to d0/d1 if the compiler is that old.

|		Copyright (C) Motorola, Inc. 1991
|			All Rights Reserved
|
|	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA
|	The copyright notice above does not evidence any
|	actual or intended publication of such source code.

|	xref	__l_tag
|	xref	__l_sslog2
|	xref	__l_t_dz2
|	xref	__l_sopr_inf
|	xref	__l_mon_nan
|	xref	__l_sslog2d

	.globl	__l_flog2s
__l_flog2s:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmoves	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_1182
	bsrl	__l_sslog2		|  normalized (regular) number
	jra 	L_1186
L_1182:
	cmpb	#0x20,d0		|  zero?
	jne 	L_1183
	bsrl	__l_t_dz2
	jra 	L_1186
L_1183:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_1184
	bsrl	__l_sopr_inf
	jra 	L_1186
L_1184:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_1185
	bsrl	__l_mon_nan
	jra 	L_1186
L_1185:
	bsrl	__l_sslog2d		|  assuming a denorm...

L_1186:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_flog2d
__l_flog2d:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmoved	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_1187
	bsrl	__l_sslog2		|  normalized (regular) number
	jra 	L_118B
L_1187:
	cmpb	#0x20,d0		|  zero?
	jne 	L_1188
	bsrl	__l_t_dz2
	jra 	L_118B
L_1188:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_1189
	bsrl	__l_sopr_inf
	jra 	L_118B
L_1189:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_118A
	bsrl	__l_mon_nan
	jra 	L_118B
L_118A:
	bsrl	__l_sslog2d		|  assuming a denorm...

L_118B:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_flog2x
__l_flog2x:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmovex	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_118C
	bsrl	__l_sslog2		|  normalized (regular) number
	jra 	L_118G
L_118C:
	cmpb	#0x20,d0		|  zero?
	jne 	L_118D
	bsrl	__l_t_dz2
	jra 	L_118G
L_118D:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_118E
	bsrl	__l_sopr_inf
	jra 	L_118G
L_118E:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_118F
	bsrl	__l_mon_nan
	jra 	L_118G
L_118F:
	bsrl	__l_sslog2d		|  assuming a denorm...

L_118G:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

|
|	MONADIC.GEN 1.3 4/30/91
|
|	MONADIC.GEN --- generic MONADIC template
|
|	This version saves all registers that will be used by the emulation
|	routines and restores all but FP0 on exit.  The FPSR is
|	updated to reflect the result of the operation.  Return value
|	is placed in FP0 for single, double and extended results.
|
|	The package subroutines expect the incoming fpcr to be zeroed
|	since they need extended precision to work properly.  The
/* |	'final' fpcr is expected in d1 so that the calculated result */
|	can be properly sized and rounded.  Also, if the incoming fpcr
|	has enabled any exceptions, the exception will be taken on the
|	final fmovem in this template.
|
|	Customizations:
|		1. Remove the moveml at the entry and exit of
|		   each routine if your compiler treats those
|		   registers as scratch.
/* |		2. Likewise, don't save FP0/FP1 if they are scratch */
|		   registers.
|		3. Delete handling of the fpsr if you only care about
|		   the result.
|		4. Some (most?) C compilers convert all float arguments
|		   to double, and provide no support at all for extended
|		   precision so remove the __l_flog10s and __l_flog10x entry points.
|		5. Move the result to d0/d1 if the compiler is that old.

|		Copyright (C) Motorola, Inc. 1991
|			All Rights Reserved
|
|	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA
|	The copyright notice above does not evidence any
|	actual or intended publication of such source code.

|	xref	__l_tag
|	xref	__l_sslog10
|	xref	__l_t_dz2
|	xref	__l_sopr_inf
|	xref	__l_mon_nan
|	xref	__l_sslog10d

	.globl	__l_flog10s
__l_flog10s:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmoves	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_1192
	bsrl	__l_sslog10		|  normalized (regular) number
	jra 	L_1196
L_1192:
	cmpb	#0x20,d0		|  zero?
	jne 	L_1193
	bsrl	__l_t_dz2
	jra 	L_1196
L_1193:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_1194
	bsrl	__l_sopr_inf
	jra 	L_1196
L_1194:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_1195
	bsrl	__l_mon_nan
	jra 	L_1196
L_1195:
	bsrl	__l_sslog10d		|  assuming a denorm...

L_1196:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_flog10d
__l_flog10d:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmoved	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_1197
	bsrl	__l_sslog10		|  normalized (regular) number
	jra 	L_119B
L_1197:
	cmpb	#0x20,d0		|  zero?
	jne 	L_1198
	bsrl	__l_t_dz2
	jra 	L_119B
L_1198:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_1199
	bsrl	__l_sopr_inf
	jra 	L_119B
L_1199:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_119A
	bsrl	__l_mon_nan
	jra 	L_119B
L_119A:
	bsrl	__l_sslog10d		|  assuming a denorm...

L_119B:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_flog10x
__l_flog10x:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmovex	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_119C
	bsrl	__l_sslog10		|  normalized (regular) number
	jra 	L_119G
L_119C:
	cmpb	#0x20,d0		|  zero?
	jne 	L_119D
	bsrl	__l_t_dz2
	jra 	L_119G
L_119D:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_119E
	bsrl	__l_sopr_inf
	jra 	L_119G
L_119E:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_119F
	bsrl	__l_mon_nan
	jra 	L_119G
L_119F:
	bsrl	__l_sslog10d		|  assuming a denorm...

L_119G:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

|
|	MONADIC.GEN 1.3 4/30/91
|
|	MONADIC.GEN --- generic MONADIC template
|
|	This version saves all registers that will be used by the emulation
|	routines and restores all but FP0 on exit.  The FPSR is
|	updated to reflect the result of the operation.  Return value
|	is placed in FP0 for single, double and extended results.
|
|	The package subroutines expect the incoming fpcr to be zeroed
|	since they need extended precision to work properly.  The
/* |	'final' fpcr is expected in d1 so that the calculated result */
|	can be properly sized and rounded.  Also, if the incoming fpcr
|	has enabled any exceptions, the exception will be taken on the
|	final fmovem in this template.
|
|	Customizations:
|		1. Remove the moveml at the entry and exit of
|		   each routine if your compiler treats those
|		   registers as scratch.
/* |		2. Likewise, don't save FP0/FP1 if they are scratch */
|		   registers.
|		3. Delete handling of the fpsr if you only care about
|		   the result.
|		4. Some (most?) C compilers convert all float arguments
|		   to double, and provide no support at all for extended
|		   precision so remove the __l_flognp1s and __l_flognp1x entry points.
|		5. Move the result to d0/d1 if the compiler is that old.

|		Copyright (C) Motorola, Inc. 1991
|			All Rights Reserved
|
|	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA
|	The copyright notice above does not evidence any
|	actual or intended publication of such source code.

|	xref	__l_tag
|	xref	__l_sslognp1
|	xref	__l_szero
|	xref	__l_sopr_inf
|	xref	__l_mon_nan
|	xref	__l_slognp1d

	.globl	__l_flognp1s
__l_flognp1s:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmoves	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_1202
	bsrl	__l_sslognp1		|  normalized (regular) number
	jra 	L_1206
L_1202:
	cmpb	#0x20,d0		|  zero?
	jne 	L_1203
	bsrl	__l_szero
	jra 	L_1206
L_1203:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_1204
	bsrl	__l_sopr_inf
	jra 	L_1206
L_1204:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_1205
	bsrl	__l_mon_nan
	jra 	L_1206
L_1205:
	bsrl	__l_slognp1d		|  assuming a denorm...

L_1206:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_flognp1d
__l_flognp1d:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmoved	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_1207
	bsrl	__l_sslognp1		|  normalized (regular) number
	jra 	L_120B
L_1207:
	cmpb	#0x20,d0		|  zero?
	jne 	L_1208
	bsrl	__l_szero
	jra 	L_120B
L_1208:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_1209
	bsrl	__l_sopr_inf
	jra 	L_120B
L_1209:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_120A
	bsrl	__l_mon_nan
	jra 	L_120B
L_120A:
	bsrl	__l_slognp1d		|  assuming a denorm...

L_120B:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_flognp1x
__l_flognp1x:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmovex	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_120C
	bsrl	__l_sslognp1		|  normalized (regular) number
	jra 	L_120G
L_120C:
	cmpb	#0x20,d0		|  zero?
	jne 	L_120D
	bsrl	__l_szero
	jra 	L_120G
L_120D:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_120E
	bsrl	__l_sopr_inf
	jra 	L_120G
L_120E:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_120F
	bsrl	__l_mon_nan
	jra 	L_120G
L_120F:
	bsrl	__l_slognp1d		|  assuming a denorm...

L_120G:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

|
|	MONADIC.GEN 1.3 4/30/91
|
|	MONADIC.GEN --- generic MONADIC template
|
|	This version saves all registers that will be used by the emulation
|	routines and restores all but FP0 on exit.  The FPSR is
|	updated to reflect the result of the operation.  Return value
|	is placed in FP0 for single, double and extended results.
|
|	The package subroutines expect the incoming fpcr to be zeroed
|	since they need extended precision to work properly.  The
/* |	'final' fpcr is expected in d1 so that the calculated result */
|	can be properly sized and rounded.  Also, if the incoming fpcr
|	has enabled any exceptions, the exception will be taken on the
|	final fmovem in this template.
|
|	Customizations:
|		1. Remove the moveml at the entry and exit of
|		   each routine if your compiler treats those
|		   registers as scratch.
/* |		2. Likewise, don't save FP0/FP1 if they are scratch */
|		   registers.
|		3. Delete handling of the fpsr if you only care about
|		   the result.
|		4. Some (most?) C compilers convert all float arguments
|		   to double, and provide no support at all for extended
|		   precision so remove the __l_fints and __l_fintx entry points.
|		5. Move the result to d0/d1 if the compiler is that old.

|		Copyright (C) Motorola, Inc. 1991
|			All Rights Reserved
|
|	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA
|	The copyright notice above does not evidence any
|	actual or intended publication of such source code.

|	xref	__l_tag
|	xref	__l_l_sint
|	xref	__l_szero
|	xref	__l_sinf
|	xref	__l_mon_nan
|	xref	__l_l_sintd

	.globl	__l_fints
__l_fints:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmoves	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_1212
	bsrl	__l_l_sint		|  normalized (regular) number
	jra 	L_1216
L_1212:
	cmpb	#0x20,d0		|  zero?
	jne 	L_1213
	bsrl	__l_szero
	jra 	L_1216
L_1213:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_1214
	bsrl	__l_sinf
	jra 	L_1216
L_1214:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_1215
	bsrl	__l_mon_nan
	jra 	L_1216
L_1215:
	bsrl	__l_l_sintd		|  assuming a denorm...

L_1216:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_fintd
__l_fintd:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmoved	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_1217
	bsrl	__l_l_sint		|  normalized (regular) number
	jra 	L_121B
L_1217:
	cmpb	#0x20,d0		|  zero?
	jne 	L_1218
	bsrl	__l_szero
	jra 	L_121B
L_1218:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_1219
	bsrl	__l_sinf
	jra 	L_121B
L_1219:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_121A
	bsrl	__l_mon_nan
	jra 	L_121B
L_121A:
	bsrl	__l_l_sintd		|  assuming a denorm...

L_121B:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_fintx
__l_fintx:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmovex	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_121C
	bsrl	__l_l_sint		|  normalized (regular) number
	jra 	L_121G
L_121C:
	cmpb	#0x20,d0		|  zero?
	jne 	L_121D
	bsrl	__l_szero
	jra 	L_121G
L_121D:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_121E
	bsrl	__l_sinf
	jra 	L_121G
L_121E:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_121F
	bsrl	__l_mon_nan
	jra 	L_121G
L_121F:
	bsrl	__l_l_sintd		|  assuming a denorm...

L_121G:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

|
|	MONADIC.GEN 1.3 4/30/91
|
|	MONADIC.GEN --- generic MONADIC template
|
|	This version saves all registers that will be used by the emulation
|	routines and restores all but FP0 on exit.  The FPSR is
|	updated to reflect the result of the operation.  Return value
|	is placed in FP0 for single, double and extended results.
|
|	The package subroutines expect the incoming fpcr to be zeroed
|	since they need extended precision to work properly.  The
/* |	'final' fpcr is expected in d1 so that the calculated result */
|	can be properly sized and rounded.  Also, if the incoming fpcr
|	has enabled any exceptions, the exception will be taken on the
|	final fmovem in this template.
|
|	Customizations:
|		1. Remove the moveml at the entry and exit of
|		   each routine if your compiler treats those
|		   registers as scratch.
/* |		2. Likewise, don't save FP0/FP1 if they are scratch */
|		   registers.
|		3. Delete handling of the fpsr if you only care about
|		   the result.
|		4. Some (most?) C compilers convert all float arguments
|		   to double, and provide no support at all for extended
|		   precision so remove the __l_fintrzs and __l_fintrzx entry points.
|		5. Move the result to d0/d1 if the compiler is that old.

|		Copyright (C) Motorola, Inc. 1991
|			All Rights Reserved
|
|	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA
|	The copyright notice above does not evidence any
|	actual or intended publication of such source code.

|	xref	__l_tag
|	xref	__l_l_sintrz
|	xref	__l_szero
|	xref	__l_sinf
|	xref	__l_mon_nan
|	xref	__l_snzrinx

	.globl	__l_fintrzs
__l_fintrzs:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmoves	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_1222
	bsrl	__l_l_sintrz		|  normalized (regular) number
	jra 	L_1226
L_1222:
	cmpb	#0x20,d0		|  zero?
	jne 	L_1223
	bsrl	__l_szero
	jra 	L_1226
L_1223:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_1224
	bsrl	__l_sinf
	jra 	L_1226
L_1224:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_1225
	bsrl	__l_mon_nan
	jra 	L_1226
L_1225:
	bsrl	__l_snzrinx		|  assuming a denorm...

L_1226:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_fintrzd
__l_fintrzd:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmoved	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_1227
	bsrl	__l_l_sintrz		|  normalized (regular) number
	jra 	L_122B
L_1227:
	cmpb	#0x20,d0		|  zero?
	jne 	L_1228
	bsrl	__l_szero
	jra 	L_122B
L_1228:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_1229
	bsrl	__l_sinf
	jra 	L_122B
L_1229:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_122A
	bsrl	__l_mon_nan
	jra 	L_122B
L_122A:
	bsrl	__l_snzrinx		|  assuming a denorm...

L_122B:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_fintrzx
__l_fintrzx:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmovex	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_122C
	bsrl	__l_l_sintrz		|  normalized (regular) number
	jra 	L_122G
L_122C:
	cmpb	#0x20,d0		|  zero?
	jne 	L_122D
	bsrl	__l_szero
	jra 	L_122G
L_122D:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_122E
	bsrl	__l_sinf
	jra 	L_122G
L_122E:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_122F
	bsrl	__l_mon_nan
	jra 	L_122G
L_122F:
	bsrl	__l_snzrinx		|  assuming a denorm...

L_122G:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

|
|	DYADIC.GEN 1.2 4/30/91
|
|	DYADIC.GEN --- generic DYADIC template
|
|	This version saves all registers that will be used by the emulation
|	routines and restores all but FP0 on exit.  The FPSR is
|	updated to reflect the result of the operation.  Return value
|	is placed in FP0 for single, double and extended results.
|
|	The package subroutines expect the incoming fpcr to be zeroed
|	since they need extended precision to work properly.  The
/* |	'final' fpcr is expected in a6@(USER_FPCR) so that the calculated result */
|	can be properly sized and rounded.  Also, if the incoming fpcr
|	has enabled any exceptions, the exception will be taken on the
|	final fmovem in this template.
|
|	Customizations:
|		1. Remove the moveml at the entry and exit of
|		   each routine if your compiler treats those
|		   registers as scratch.
/* |		2. Likewise, don't save FP0/FP1 if they are scratch */
|		   registers.
|		3. Delete updating of the fpsr if you only care about
|		   the result.
|		4. Remove the __l_frems and __l_fremx entry points if your compiler
|		   treats everything as doubles.
|		5. Move the result to d0/d1 if the compiler is that old.
|

|		Copyright (C) Motorola, Inc. 1991
|			All Rights Reserved
|
|	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA
|	The copyright notice above does not evidence any
|	actual or intended publication of such source code.

|	xref	__l_prem
|	xref	__l_tag

	.globl	__l_frems
__l_frems:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmoveml fpsr/fpcr,a6@(USER_FPCR) /* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input arguments
|
	fmoves	a6@(8),fp0
	fmovex	fp0,a6@(FPTEMP)
	lea	a6@(FPTEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(DTAG)

	fmoves	a6@(12),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)

	bsrl	__l_prem

	fmovel	fpsr,d0		|  update status register
	orb	a6@(FPSR_AEXCEPT),d0	| add previously accrued exceptions
	swap	d0
	orb	a6@(FPSR_QBYTE),d0	|  pickup sign of quotient byte
	swap	d0
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_fremd
__l_fremd:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmoveml fpsr/fpcr,a6@(USER_FPCR) /* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input arguments
|
	fmoved	a6@(8),fp0
	fmovex	fp0,a6@(FPTEMP)
	lea	a6@(FPTEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(DTAG)

	fmoved	a6@(16),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)

	bsrl	__l_prem

	fmovel	fpsr,d0		|  update status register
	orb	a6@(FPSR_AEXCEPT),d0	| add previously accrued exceptions
	swap	d0
	orb	a6@(FPSR_QBYTE),d0	|  pickup sign of quotient byte
	swap	d0
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_fremx
__l_fremx:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmoveml fpsr/fpcr,a6@(USER_FPCR) /* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input arguments
|
	fmovex	a6@(8),fp0
	fmovex	fp0,a6@(FPTEMP)
	lea	a6@(FPTEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(DTAG)

	fmovex	a6@(20),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)

	bsrl	__l_prem

	fmovel	fpsr,d0		|  update status register
	orb	a6@(FPSR_AEXCEPT),d0	| add previously accrued exceptions
	swap	d0
	orb	a6@(FPSR_QBYTE),d0	|  pickup sign of quotient byte
	swap	d0
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

|
|	DYADIC.GEN 1.2 4/30/91
|
|	DYADIC.GEN --- generic DYADIC template
|
|	This version saves all registers that will be used by the emulation
|	routines and restores all but FP0 on exit.  The FPSR is
|	updated to reflect the result of the operation.  Return value
|	is placed in FP0 for single, double and extended results.
|
|	The package subroutines expect the incoming fpcr to be zeroed
|	since they need extended precision to work properly.  The
/* |	'final' fpcr is expected in a6@(USER_FPCR) so that the calculated result */
|	can be properly sized and rounded.  Also, if the incoming fpcr
|	has enabled any exceptions, the exception will be taken on the
|	final fmovem in this template.
|
|	Customizations:
|		1. Remove the moveml at the entry and exit of
|		   each routine if your compiler treats those
|		   registers as scratch.
/* |		2. Likewise, don't save FP0/FP1 if they are scratch */
|		   registers.
|		3. Delete updating of the fpsr if you only care about
|		   the result.
|		4. Remove the __l_fmods and __l_fmodx entry points if your compiler
|		   treats everything as doubles.
|		5. Move the result to d0/d1 if the compiler is that old.
|

|		Copyright (C) Motorola, Inc. 1991
|			All Rights Reserved
|
|	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA
|	The copyright notice above does not evidence any
|	actual or intended publication of such source code.

|	xref	__l_pmod
|	xref	__l_tag

	.globl	__l_fmods
__l_fmods:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmoveml fpsr/fpcr,a6@(USER_FPCR) /* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input arguments
|
	fmoves	a6@(8),fp0
	fmovex	fp0,a6@(FPTEMP)
	lea	a6@(FPTEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(DTAG)

	fmoves	a6@(12),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)

	bsrl	__l_pmod

	fmovel	fpsr,d0		|  update status register
	orb	a6@(FPSR_AEXCEPT),d0	| add previously accrued exceptions
	swap	d0
	orb	a6@(FPSR_QBYTE),d0	|  pickup sign of quotient byte
	swap	d0
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_fmodd
__l_fmodd:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmoveml fpsr/fpcr,a6@(USER_FPCR) /* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input arguments
|
	fmoved	a6@(8),fp0
	fmovex	fp0,a6@(FPTEMP)
	lea	a6@(FPTEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(DTAG)

	fmoved	a6@(16),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)

	bsrl	__l_pmod

	fmovel	fpsr,d0		|  update status register
	orb	a6@(FPSR_AEXCEPT),d0	| add previously accrued exceptions
	swap	d0
	orb	a6@(FPSR_QBYTE),d0	|  pickup sign of quotient byte
	swap	d0
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_fmodx
__l_fmodx:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmoveml fpsr/fpcr,a6@(USER_FPCR) /* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input arguments
|
	fmovex	a6@(8),fp0
	fmovex	fp0,a6@(FPTEMP)
	lea	a6@(FPTEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(DTAG)

	fmovex	a6@(20),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)

	bsrl	__l_pmod

	fmovel	fpsr,d0		|  update status register
	orb	a6@(FPSR_AEXCEPT),d0	| add previously accrued exceptions
	swap	d0
	orb	a6@(FPSR_QBYTE),d0	|  pickup sign of quotient byte
	swap	d0
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

|
|	DYADIC.GEN 1.2 4/30/91
|
|	DYADIC.GEN --- generic DYADIC template
|
|	This version saves all registers that will be used by the emulation
|	routines and restores all but FP0 on exit.  The FPSR is
|	updated to reflect the result of the operation.  Return value
|	is placed in FP0 for single, double and extended results.
|
|	The package subroutines expect the incoming fpcr to be zeroed
|	since they need extended precision to work properly.  The
/* |	'final' fpcr is expected in a6@(USER_FPCR) so that the calculated result */
|	can be properly sized and rounded.  Also, if the incoming fpcr
|	has enabled any exceptions, the exception will be taken on the
|	final fmovem in this template.
|
|	Customizations:
|		1. Remove the moveml at the entry and exit of
|		   each routine if your compiler treats those
|		   registers as scratch.
/* |		2. Likewise, don't save FP0/FP1 if they are scratch */
|		   registers.
|		3. Delete updating of the fpsr if you only care about
|		   the result.
|		4. Remove the __l_fscales and __l_fscalex entry points if your compiler
|		   treats everything as doubles.
|		5. Move the result to d0/d1 if the compiler is that old.
|

|		Copyright (C) Motorola, Inc. 1991
|			All Rights Reserved
|
|	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA
|	The copyright notice above does not evidence any
|	actual or intended publication of such source code.

|	xref	__l_pscale
|	xref	__l_tag

	.globl	__l_fscales
__l_fscales:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmoveml fpsr/fpcr,a6@(USER_FPCR) /* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input arguments
|
	fmoves	a6@(8),fp0
	fmovex	fp0,a6@(FPTEMP)
	lea	a6@(FPTEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(DTAG)

	fmoves	a6@(12),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)

	bsrl	__l_pscale

	fmovel	fpsr,d0		|  update status register
	orb	a6@(FPSR_AEXCEPT),d0	| add previously accrued exceptions
	swap	d0
	orb	a6@(FPSR_QBYTE),d0	|  pickup sign of quotient byte
	swap	d0
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_fscaled
__l_fscaled:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmoveml fpsr/fpcr,a6@(USER_FPCR) /* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input arguments
|
	fmoved	a6@(8),fp0
	fmovex	fp0,a6@(FPTEMP)
	lea	a6@(FPTEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(DTAG)

	fmoved	a6@(16),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)

	bsrl	__l_pscale

	fmovel	fpsr,d0		|  update status register
	orb	a6@(FPSR_AEXCEPT),d0	| add previously accrued exceptions
	swap	d0
	orb	a6@(FPSR_QBYTE),d0	|  pickup sign of quotient byte
	swap	d0
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_fscalex
__l_fscalex:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmoveml fpsr/fpcr,a6@(USER_FPCR) /* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input arguments
|
	fmovex	a6@(8),fp0
	fmovex	fp0,a6@(FPTEMP)
	lea	a6@(FPTEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(DTAG)

	fmovex	a6@(20),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)

	bsrl	__l_pscale

	fmovel	fpsr,d0		|  update status register
	orb	a6@(FPSR_AEXCEPT),d0	| add previously accrued exceptions
	swap	d0
	orb	a6@(FPSR_QBYTE),d0	|  pickup sign of quotient byte
	swap	d0
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

|
|	MONADIC.GEN 1.3 4/30/91
|
|	MONADIC.GEN --- generic MONADIC template
|
|	This version saves all registers that will be used by the emulation
|	routines and restores all but FP0 on exit.  The FPSR is
|	updated to reflect the result of the operation.  Return value
|	is placed in FP0 for single, double and extended results.
|
|	The package subroutines expect the incoming fpcr to be zeroed
|	since they need extended precision to work properly.  The
/* |	'final' fpcr is expected in d1 so that the calculated result */
|	can be properly sized and rounded.  Also, if the incoming fpcr
|	has enabled any exceptions, the exception will be taken on the
|	final fmovem in this template.
|
|	Customizations:
|		1. Remove the moveml at the entry and exit of
|		   each routine if your compiler treats those
|		   registers as scratch.
/* |		2. Likewise, don't save FP0/FP1 if they are scratch */
|		   registers.
|		3. Delete handling of the fpsr if you only care about
|		   the result.
|		4. Some (most?) C compilers convert all float arguments
|		   to double, and provide no support at all for extended
|		   precision so remove the __l_fabss and __l_fabsx entry points.
|		5. Move the result to d0/d1 if the compiler is that old.

|		Copyright (C) Motorola, Inc. 1991
|			All Rights Reserved
|
|	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA
|	The copyright notice above does not evidence any
|	actual or intended publication of such source code.

|	xref	__l_tag
|	xref	__l_sabs
|	xref	__l_sabs
|	xref	__l_sabs
|	xref	__l_sabs
|	xref	__l_sabs

	.globl	__l_fabss
__l_fabss:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmoves	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_1262
	bsrl	__l_sabs		|  normalized (regular) number
	jra 	L_1266
L_1262:
	cmpb	#0x20,d0		|  zero?
	jne 	L_1263
	bsrl	__l_sabs
	jra 	L_1266
L_1263:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_1264
	bsrl	__l_sabs
	jra 	L_1266
L_1264:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_1265
	bsrl	__l_sabs
	jra 	L_1266
L_1265:
	bsrl	__l_sabs		|  assuming a denorm...

L_1266:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_fabsd
__l_fabsd:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmoved	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_1267
	bsrl	__l_sabs		|  normalized (regular) number
	jra 	L_126B
L_1267:
	cmpb	#0x20,d0		|  zero?
	jne 	L_1268
	bsrl	__l_sabs
	jra 	L_126B
L_1268:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_1269
	bsrl	__l_sabs
	jra 	L_126B
L_1269:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_126A
	bsrl	__l_sabs
	jra 	L_126B
L_126A:
	bsrl	__l_sabs		|  assuming a denorm...

L_126B:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_fabsx
__l_fabsx:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmovex	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_126C
	bsrl	__l_sabs		|  normalized (regular) number
	jra 	L_126G
L_126C:
	cmpb	#0x20,d0		|  zero?
	jne 	L_126D
	bsrl	__l_sabs
	jra 	L_126G
L_126D:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_126E
	bsrl	__l_sabs
	jra 	L_126G
L_126E:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_126F
	bsrl	__l_sabs
	jra 	L_126G
L_126F:
	bsrl	__l_sabs		|  assuming a denorm...

L_126G:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

|
|	MONADIC.GEN 1.3 4/30/91
|
|	MONADIC.GEN --- generic MONADIC template
|
|	This version saves all registers that will be used by the emulation
|	routines and restores all but FP0 on exit.  The FPSR is
|	updated to reflect the result of the operation.  Return value
|	is placed in FP0 for single, double and extended results.
|
|	The package subroutines expect the incoming fpcr to be zeroed
|	since they need extended precision to work properly.  The
/* |	'final' fpcr is expected in d1 so that the calculated result */
|	can be properly sized and rounded.  Also, if the incoming fpcr
|	has enabled any exceptions, the exception will be taken on the
|	final fmovem in this template.
|
|	Customizations:
|		1. Remove the moveml at the entry and exit of
|		   each routine if your compiler treats those
|		   registers as scratch.
/* |		2. Likewise, don't save FP0/FP1 if they are scratch */
|		   registers.
|		3. Delete handling of the fpsr if you only care about
|		   the result.
|		4. Some (most?) C compilers convert all float arguments
|		   to double, and provide no support at all for extended
|		   precision so remove the __l_fnegs and __l_fnegx entry points.
|		5. Move the result to d0/d1 if the compiler is that old.

|		Copyright (C) Motorola, Inc. 1991
|			All Rights Reserved
|
|	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA
|	The copyright notice above does not evidence any
|	actual or intended publication of such source code.

|	xref	__l_tag
|	xref	__l_sneg
|	xref	__l_sneg
|	xref	__l_sneg
|	xref	__l_sneg
|	xref	__l_sneg

	.globl	__l_fnegs
__l_fnegs:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmoves	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_1272
	bsrl	__l_sneg		|  normalized (regular) number
	jra 	L_1276
L_1272:
	cmpb	#0x20,d0		|  zero?
	jne 	L_1273
	bsrl	__l_sneg
	jra 	L_1276
L_1273:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_1274
	bsrl	__l_sneg
	jra 	L_1276
L_1274:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_1275
	bsrl	__l_sneg
	jra 	L_1276
L_1275:
	bsrl	__l_sneg		|  assuming a denorm...

L_1276:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_fnegd
__l_fnegd:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmoved	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_1277
	bsrl	__l_sneg		|  normalized (regular) number
	jra 	L_127B
L_1277:
	cmpb	#0x20,d0		|  zero?
	jne 	L_1278
	bsrl	__l_sneg
	jra 	L_127B
L_1278:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_1279
	bsrl	__l_sneg
	jra 	L_127B
L_1279:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_127A
	bsrl	__l_sneg
	jra 	L_127B
L_127A:
	bsrl	__l_sneg		|  assuming a denorm...

L_127B:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_fnegx
__l_fnegx:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmovex	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_127C
	bsrl	__l_sneg		|  normalized (regular) number
	jra 	L_127G
L_127C:
	cmpb	#0x20,d0		|  zero?
	jne 	L_127D
	bsrl	__l_sneg
	jra 	L_127G
L_127D:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_127E
	bsrl	__l_sneg
	jra 	L_127G
L_127E:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_127F
	bsrl	__l_sneg
	jra 	L_127G
L_127F:
	bsrl	__l_sneg		|  assuming a denorm...

L_127G:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

|
|	MONADIC.GEN 1.3 4/30/91
|
|	MONADIC.GEN --- generic MONADIC template
|
|	This version saves all registers that will be used by the emulation
|	routines and restores all but FP0 on exit.  The FPSR is
|	updated to reflect the result of the operation.  Return value
|	is placed in FP0 for single, double and extended results.
|
|	The package subroutines expect the incoming fpcr to be zeroed
|	since they need extended precision to work properly.  The
/* |	'final' fpcr is expected in d1 so that the calculated result */
|	can be properly sized and rounded.  Also, if the incoming fpcr
|	has enabled any exceptions, the exception will be taken on the
|	final fmovem in this template.
|
|	Customizations:
|		1. Remove the moveml at the entry and exit of
|		   each routine if your compiler treats those
|		   registers as scratch.
/* |		2. Likewise, don't save FP0/FP1 if they are scratch */
|		   registers.
|		3. Delete handling of the fpsr if you only care about
|		   the result.
|		4. Some (most?) C compilers convert all float arguments
|		   to double, and provide no support at all for extended
|		   precision so remove the __l_fsqrts and __l_fsqrtx entry points.
|		5. Move the result to d0/d1 if the compiler is that old.

|		Copyright (C) Motorola, Inc. 1991
|			All Rights Reserved
|
|	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA
|	The copyright notice above does not evidence any
|	actual or intended publication of such source code.

|	xref	__l_tag
|	xref	__l_ssqrt
|	xref	__l_ssqrt
|	xref	__l_ssqrt
|	xref	__l_ssqrt
|	xref	__l_ssqrt

	.globl	__l_fsqrts
__l_fsqrts:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmoves	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_1282
	bsrl	__l_ssqrt		|  normalized (regular) number
	jra 	L_1286
L_1282:
	cmpb	#0x20,d0		|  zero?
	jne 	L_1283
	bsrl	__l_ssqrt
	jra 	L_1286
L_1283:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_1284
	bsrl	__l_ssqrt
	jra 	L_1286
L_1284:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_1285
	bsrl	__l_ssqrt
	jra 	L_1286
L_1285:
	bsrl	__l_ssqrt		|  assuming a denorm...

L_1286:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_fsqrtd
__l_fsqrtd:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmoved	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_1287
	bsrl	__l_ssqrt		|  normalized (regular) number
	jra 	L_128B
L_1287:
	cmpb	#0x20,d0		|  zero?
	jne 	L_1288
	bsrl	__l_ssqrt
	jra 	L_128B
L_1288:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_1289
	bsrl	__l_ssqrt
	jra 	L_128B
L_1289:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_128A
	bsrl	__l_ssqrt
	jra 	L_128B
L_128A:
	bsrl	__l_ssqrt		|  assuming a denorm...

L_128B:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_fsqrtx
__l_fsqrtx:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmovel	fpsr,a6@(USER_FPSR)
	fmovel	fpcr,a6@(USER_FPCR)
	fmovel	fpcr,d1		/* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input argument
|
	fmovex	a6@(8),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)
	tstb	d0
	jne 	L_128C
	bsrl	__l_ssqrt		|  normalized (regular) number
	jra 	L_128G
L_128C:
	cmpb	#0x20,d0		|  zero?
	jne 	L_128D
	bsrl	__l_ssqrt
	jra 	L_128G
L_128D:
	cmpb	#0x40,d0		|  infinity?
	jne 	L_128E
	bsrl	__l_ssqrt
	jra 	L_128G
L_128E:
	cmpb	#0x60,d0		|  NaN?
	jne 	L_128F
	bsrl	__l_ssqrt
	jra 	L_128G
L_128F:
	bsrl	__l_ssqrt		|  assuming a denorm...

L_128G:
	fmovel	fpsr,d0		|  update status register
	orb	a6@(USER_FPSR+3),d0	| add previously accrued exceptions
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

|
|	DYADIC.GEN 1.2 4/30/91
|
|	DYADIC.GEN --- generic DYADIC template
|
|	This version saves all registers that will be used by the emulation
|	routines and restores all but FP0 on exit.  The FPSR is
|	updated to reflect the result of the operation.  Return value
|	is placed in FP0 for single, double and extended results.
|
|	The package subroutines expect the incoming fpcr to be zeroed
|	since they need extended precision to work properly.  The
/* |	'final' fpcr is expected in a6@(USER_FPCR) so that the calculated result */
|	can be properly sized and rounded.  Also, if the incoming fpcr
|	has enabled any exceptions, the exception will be taken on the
|	final fmovem in this template.
|
|	Customizations:
|		1. Remove the moveml at the entry and exit of
|		   each routine if your compiler treats those
|		   registers as scratch.
/* |		2. Likewise, don't save FP0/FP1 if they are scratch */
|		   registers.
|		3. Delete updating of the fpsr if you only care about
|		   the result.
|		4. Remove the __l_fadds and __l_faddx entry points if your compiler
|		   treats everything as doubles.
|		5. Move the result to d0/d1 if the compiler is that old.
|

|		Copyright (C) Motorola, Inc. 1991
|			All Rights Reserved
|
|	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA
|	The copyright notice above does not evidence any
|	actual or intended publication of such source code.

|	xref	__l_sadd
|	xref	__l_tag

	.globl	__l_fadds
__l_fadds:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmoveml fpsr/fpcr,a6@(USER_FPCR) /* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input arguments
|
	fmoves	a6@(8),fp0
	fmovex	fp0,a6@(FPTEMP)
	lea	a6@(FPTEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(DTAG)

	fmoves	a6@(12),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)

	bsrl	__l_sadd

	fmovel	fpsr,d0		|  update status register
	orb	a6@(FPSR_AEXCEPT),d0	| add previously accrued exceptions
	swap	d0
	orb	a6@(FPSR_QBYTE),d0	|  pickup sign of quotient byte
	swap	d0
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_faddd
__l_faddd:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmoveml fpsr/fpcr,a6@(USER_FPCR) /* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input arguments
|
	fmoved	a6@(8),fp0
	fmovex	fp0,a6@(FPTEMP)
	lea	a6@(FPTEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(DTAG)

	fmoved	a6@(16),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)

	bsrl	__l_sadd

	fmovel	fpsr,d0		|  update status register
	orb	a6@(FPSR_AEXCEPT),d0	| add previously accrued exceptions
	swap	d0
	orb	a6@(FPSR_QBYTE),d0	|  pickup sign of quotient byte
	swap	d0
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_faddx
__l_faddx:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmoveml fpsr/fpcr,a6@(USER_FPCR) /* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input arguments
|
	fmovex	a6@(8),fp0
	fmovex	fp0,a6@(FPTEMP)
	lea	a6@(FPTEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(DTAG)

	fmovex	a6@(20),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)

	bsrl	__l_sadd

	fmovel	fpsr,d0		|  update status register
	orb	a6@(FPSR_AEXCEPT),d0	| add previously accrued exceptions
	swap	d0
	orb	a6@(FPSR_QBYTE),d0	|  pickup sign of quotient byte
	swap	d0
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

|
|	DYADIC.GEN 1.2 4/30/91
|
|	DYADIC.GEN --- generic DYADIC template
|
|	This version saves all registers that will be used by the emulation
|	routines and restores all but FP0 on exit.  The FPSR is
|	updated to reflect the result of the operation.  Return value
|	is placed in FP0 for single, double and extended results.
|
|	The package subroutines expect the incoming fpcr to be zeroed
|	since they need extended precision to work properly.  The
/* |	'final' fpcr is expected in a6@(USER_FPCR) so that the calculated result */
|	can be properly sized and rounded.  Also, if the incoming fpcr
|	has enabled any exceptions, the exception will be taken on the
|	final fmovem in this template.
|
|	Customizations:
|		1. Remove the moveml at the entry and exit of
|		   each routine if your compiler treats those
|		   registers as scratch.
/* |		2. Likewise, don't save FP0/FP1 if they are scratch */
|		   registers.
|		3. Delete updating of the fpsr if you only care about
|		   the result.
|		4. Remove the __l_fsubs and __l_fsubx entry points if your compiler
|		   treats everything as doubles.
|		5. Move the result to d0/d1 if the compiler is that old.
|

|		Copyright (C) Motorola, Inc. 1991
|			All Rights Reserved
|
|	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA
|	The copyright notice above does not evidence any
|	actual or intended publication of such source code.

|	xref	__l_ssub
|	xref	__l_tag

	.globl	__l_fsubs
__l_fsubs:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmoveml fpsr/fpcr,a6@(USER_FPCR) /* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input arguments
|
	fmoves	a6@(8),fp0
	fmovex	fp0,a6@(FPTEMP)
	lea	a6@(FPTEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(DTAG)

	fmoves	a6@(12),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)

	bsrl	__l_ssub

	fmovel	fpsr,d0		|  update status register
	orb	a6@(FPSR_AEXCEPT),d0	| add previously accrued exceptions
	swap	d0
	orb	a6@(FPSR_QBYTE),d0	|  pickup sign of quotient byte
	swap	d0
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_fsubd
__l_fsubd:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmoveml fpsr/fpcr,a6@(USER_FPCR) /* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input arguments
|
	fmoved	a6@(8),fp0
	fmovex	fp0,a6@(FPTEMP)
	lea	a6@(FPTEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(DTAG)

	fmoved	a6@(16),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)

	bsrl	__l_ssub

	fmovel	fpsr,d0		|  update status register
	orb	a6@(FPSR_AEXCEPT),d0	| add previously accrued exceptions
	swap	d0
	orb	a6@(FPSR_QBYTE),d0	|  pickup sign of quotient byte
	swap	d0
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_fsubx
__l_fsubx:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmoveml fpsr/fpcr,a6@(USER_FPCR) /* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input arguments
|
	fmovex	a6@(8),fp0
	fmovex	fp0,a6@(FPTEMP)
	lea	a6@(FPTEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(DTAG)

	fmovex	a6@(20),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)

	bsrl	__l_ssub

	fmovel	fpsr,d0		|  update status register
	orb	a6@(FPSR_AEXCEPT),d0	| add previously accrued exceptions
	swap	d0
	orb	a6@(FPSR_QBYTE),d0	|  pickup sign of quotient byte
	swap	d0
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

|
|	DYADIC.GEN 1.2 4/30/91
|
|	DYADIC.GEN --- generic DYADIC template
|
|	This version saves all registers that will be used by the emulation
|	routines and restores all but FP0 on exit.  The FPSR is
|	updated to reflect the result of the operation.  Return value
|	is placed in FP0 for single, double and extended results.
|
|	The package subroutines expect the incoming fpcr to be zeroed
|	since they need extended precision to work properly.  The
/* |	'final' fpcr is expected in a6@(USER_FPCR) so that the calculated result */
|	can be properly sized and rounded.  Also, if the incoming fpcr
|	has enabled any exceptions, the exception will be taken on the
|	final fmovem in this template.
|
|	Customizations:
|		1. Remove the moveml at the entry and exit of
|		   each routine if your compiler treats those
|		   registers as scratch.
/* |		2. Likewise, don't save FP0/FP1 if they are scratch */
|		   registers.
|		3. Delete updating of the fpsr if you only care about
|		   the result.
|		4. Remove the __l_fmuls and __l_fmulx entry points if your compiler
|		   treats everything as doubles.
|		5. Move the result to d0/d1 if the compiler is that old.
|

|		Copyright (C) Motorola, Inc. 1991
|			All Rights Reserved
|
|	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA
|	The copyright notice above does not evidence any
|	actual or intended publication of such source code.

|	xref	__l_smul
|	xref	__l_tag

	.globl	__l_fmuls
__l_fmuls:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmoveml fpsr/fpcr,a6@(USER_FPCR) /* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input arguments
|
	fmoves	a6@(8),fp0
	fmovex	fp0,a6@(FPTEMP)
	lea	a6@(FPTEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(DTAG)

	fmoves	a6@(12),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)

	bsrl	__l_smul

	fmovel	fpsr,d0		|  update status register
	orb	a6@(FPSR_AEXCEPT),d0	| add previously accrued exceptions
	swap	d0
	orb	a6@(FPSR_QBYTE),d0	|  pickup sign of quotient byte
	swap	d0
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_fmuld
__l_fmuld:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmoveml fpsr/fpcr,a6@(USER_FPCR) /* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input arguments
|
	fmoved	a6@(8),fp0
	fmovex	fp0,a6@(FPTEMP)
	lea	a6@(FPTEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(DTAG)

	fmoved	a6@(16),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)

	bsrl	__l_smul

	fmovel	fpsr,d0		|  update status register
	orb	a6@(FPSR_AEXCEPT),d0	| add previously accrued exceptions
	swap	d0
	orb	a6@(FPSR_QBYTE),d0	|  pickup sign of quotient byte
	swap	d0
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_fmulx
__l_fmulx:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmoveml fpsr/fpcr,a6@(USER_FPCR) /* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input arguments
|
	fmovex	a6@(8),fp0
	fmovex	fp0,a6@(FPTEMP)
	lea	a6@(FPTEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(DTAG)

	fmovex	a6@(20),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)

	bsrl	__l_smul

	fmovel	fpsr,d0		|  update status register
	orb	a6@(FPSR_AEXCEPT),d0	| add previously accrued exceptions
	swap	d0
	orb	a6@(FPSR_QBYTE),d0	|  pickup sign of quotient byte
	swap	d0
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

|
|	DYADIC.GEN 1.2 4/30/91
|
|	DYADIC.GEN --- generic DYADIC template
|
|	This version saves all registers that will be used by the emulation
|	routines and restores all but FP0 on exit.  The FPSR is
|	updated to reflect the result of the operation.  Return value
|	is placed in FP0 for single, double and extended results.
|
|	The package subroutines expect the incoming fpcr to be zeroed
|	since they need extended precision to work properly.  The
/* |	'final' fpcr is expected in a6@(USER_FPCR) so that the calculated result */
|	can be properly sized and rounded.  Also, if the incoming fpcr
|	has enabled any exceptions, the exception will be taken on the
|	final fmovem in this template.
|
|	Customizations:
|		1. Remove the moveml at the entry and exit of
|		   each routine if your compiler treats those
|		   registers as scratch.
/* |		2. Likewise, don't save FP0/FP1 if they are scratch */
|		   registers.
|		3. Delete updating of the fpsr if you only care about
|		   the result.
|		4. Remove the __l_fdivs and __l_fdivx entry points if your compiler
|		   treats everything as doubles.
|		5. Move the result to d0/d1 if the compiler is that old.
|

|		Copyright (C) Motorola, Inc. 1991
|			All Rights Reserved
|
|	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA
|	The copyright notice above does not evidence any
|	actual or intended publication of such source code.

|	xref	__l_sdiv
|	xref	__l_tag

	.globl	__l_fdivs
__l_fdivs:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmoveml fpsr/fpcr,a6@(USER_FPCR) /* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input arguments
|
	fmoves	a6@(8),fp0
	fmovex	fp0,a6@(FPTEMP)
	lea	a6@(FPTEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(DTAG)

	fmoves	a6@(12),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)

	bsrl	__l_sdiv

	fmovel	fpsr,d0		|  update status register
	orb	a6@(FPSR_AEXCEPT),d0	| add previously accrued exceptions
	swap	d0
	orb	a6@(FPSR_QBYTE),d0	|  pickup sign of quotient byte
	swap	d0
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_fdivd
__l_fdivd:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmoveml fpsr/fpcr,a6@(USER_FPCR) /* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input arguments
|
	fmoved	a6@(8),fp0
	fmovex	fp0,a6@(FPTEMP)
	lea	a6@(FPTEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(DTAG)

	fmoved	a6@(16),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)

	bsrl	__l_sdiv

	fmovel	fpsr,d0		|  update status register
	orb	a6@(FPSR_AEXCEPT),d0	| add previously accrued exceptions
	swap	d0
	orb	a6@(FPSR_QBYTE),d0	|  pickup sign of quotient byte
	swap	d0
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

	.globl	__l_fdivx
__l_fdivx:
	link	a6,#-LOCAL_SIZE
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx fp0-fp3,a6@(USER_FP0)
	fmoveml fpsr/fpcr,a6@(USER_FPCR) /* |  user's rounding mode/precision */
	fmovel	#0,fpcr		|  force rounding mode/prec to extended,rn
|
|	copy, convert and tag input arguments
|
	fmovex	a6@(8),fp0
	fmovex	fp0,a6@(FPTEMP)
	lea	a6@(FPTEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(DTAG)

	fmovex	a6@(20),fp0
	fmovex	fp0,a6@(ETEMP)
	lea	a6@(ETEMP),a0
	bsrl	__l_tag
	moveb	d0,a6@(STAG)

	bsrl	__l_sdiv

	fmovel	fpsr,d0		|  update status register
	orb	a6@(FPSR_AEXCEPT),d0	| add previously accrued exceptions
	swap	d0
	orb	a6@(FPSR_QBYTE),d0	|  pickup sign of quotient byte
	swap	d0
	fmovel	d0,fpsr
|
|	Result is now in FP0
|
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP1),fp1-fp3	|  note: fp0 not restored
	fmovel	a6@(USER_FPCR),fpcr	| fpcr restored
	unlk	a6
	rts

|	end
