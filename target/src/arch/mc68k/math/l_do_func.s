/* l_do_func.s - Motorola 68040 FP routine jump table (LIB) */

/* Copyright 1991-1993 Wind River Systems, Inc. */
	.data
	.globl	_copyright_wind_river
	.long	_copyright_wind_river

/*
modification history
--------------------
01g,31may96,ms   updated to mototorola version 2.3
01f,14jun95,tpr  changed fbxx to fbxxl.
01e,21jul93,kdl  added .text (SPR #2372).
01d,23aug92,jcf  changed bxxx to jxx.
01c,26may92,rrr  the tree shuffle
01b,09jan92,kdl  added modification history; general cleanup.
01a,15aug91,kdl  original version, from Motorola FPSP v2.0.
/

/*
DESCRIPTION

	__l_do_funcsa 3.4 2/18/91

Do_func performs the unimplemented operation.  The operation
to be performed is determined from the lower 7 bits of the
extension word (except in the case of fmovecr and fsincos).
The opcode and tag bits form an index into a jump table in
tbldosa.  Cases of zero, infinity and NaN are handled in
__l_do_func by forcing the default result.  Normalized and
denormalized (there are no unnormalized numbers at this
point) are passed onto the emulation code.

CMDREG1B and STAG are extracted from the fsave frame
and combined to form the table index.  The function called
will start with a0 pointing to the ETEMP operand.  Dyadic
functions can find FPTEMP at	a0@(-12).

Called functions return their result in fp0.  Sincos returns
sin(x) in fp0 and cos(x) in fp1.


		Copyright (C) Motorola, Inc. 1990
			All Rights Reserved

	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA
	The copyright notice above does not evidence any
	actual or intended publication of such source code.

DO_FUNC	idnt    2,1 Motorola 040 Floating Point Software Package

	section	8

NOMANUAL
*/

#include "fpsp040L.h"

|	xref	__l_t_dz2
|	xref	__l_t_operr
|	xref	__l_t_inx2
|	xref 	__l_t_resdnrm
|	xref	__l_dst_nan
|	xref	__l_src_nan
|	xref	__l_nrm_set
|	xref	__l_sto_cos

|	xref	__l_slognp1,__l_slogn,__l_slog10,__l_slog2
|	xref	__l_slognd,__l_slog10d,__l_slog2d
|	xref	__l_smod,__l_srem
|	xref	__l_sscale

PONE:	.long	0x3fff0000,0x80000000,0x00000000	| +1
MONE:	.long	0xbfff0000,0x80000000,0x00000000	| -1
PZERO:	.long	0x00000000,0x00000000,0x00000000	| +0
MZERO:	.long	0x80000000,0x00000000,0x00000000	| -0
PINF:	.long	0x7fff0000,0x00000000,0x00000000	| +inf
MINF:	.long	0xffff0000,0x00000000,0x00000000	| -inf
QNAN:	.long	0x7fff0000,0xffffffff,0xffffffff	| non-signaling nan
PPIBY2:  .long	0x3FFF0000,0xC90FDAA2,0x2168C235	| +PI/2
MPIBY2:  .long	0xbFFF0000,0xC90FDAA2,0x2168C235	| -PI/2

|
| These routines load forced values into fp0.  They are called
| by index into tbldo.
|
| Load a signed zero to fp0 and set inex2/ainex
|

	.text

	.globl	__l_snzrinx
__l_snzrinx:
	btst	#sign_bit,a0@(LOCAL_EX)	| get sign of source operand
	jne 	ld_mzinx		| if negative, branch
	bsrl	__l_ld_pzero		| bsr so we can return and set inx
	jra 	__l_t_inx2		| now, set the inx for the next inst
ld_mzinx:
	bsrl	__l_ld_mzero		| if neg, load neg zero, return here
	jra 	__l_t_inx2		| now, set the inx for the next inst
|
| Load a signed zero to fp0|  do not set inex2/ainex
|
	.globl	__l_szero
__l_szero:
	btst	#sign_bit,a0@(LOCAL_EX) | get sign of source operand
	jne 	__l_ld_mzero		| if neg, load neg zero
	jra 	__l_ld_pzero		| load positive zero
|
| Load a signed infinity to fp0|  do not set inex2/ainex
|
	.globl	__l_sinf
__l_sinf:
	btst	#sign_bit,a0@(LOCAL_EX)	| get sign of source operand
	jne 	__l_ld_minf		| if negative branch
	jra 	__l_ld_pinf
|
| Load a signed one to fp0|  do not set inex2/ainex
|
	.globl	__l_sone
__l_sone:
	btst	#sign_bit,a0@(LOCAL_EX)	| check sign of source
	jne 	__l_ld_mone
	jra 	__l_ld_pone
|
| Load a signed pi/2 to fp0|  do not set inex2/ainex
|
	.globl	__l_spi_2
__l_spi_2:
	btst	#sign_bit,a0@(LOCAL_EX)	| check sign of source
	jne 	__l_ld_mpi2
	jra 	__l_ld_ppi2
|
| Load either a +0 or +inf for plus/minus operand
|
	.globl	__l_szr_inf
__l_szr_inf:
	btst	#sign_bit,a0@(LOCAL_EX)	| check sign of source
	jne 	__l_ld_pzero
	jra 	__l_ld_pinf
|
| Result is either an operr or +inf for plus/minus operand
| [Used by __l_slogn, __l_slognp1, __l_slog10, and __l_slog2]
|
	.globl	__l_sopr_inf
__l_sopr_inf:
	btst	#sign_bit,a0@(LOCAL_EX)	| check sign of source
	jne 	__l_t_operr
	jra 	__l_ld_pinf
|
|	FLOGNP1
|
	.globl	__l_sslognp1
__l_sslognp1:
	fmovemx	a0@,fp0-fp0
	fcmpb	#-1,fp0
	fbgtl	__l_slognp1
	fbeql	__l_t_dz2		| if = -1, divide by zero exception
	fmovel	#0,FPSR			| clr N flag
	jra 	__l_t_operr		| take care of operands < -1
|
|	FETOXM1
|
	.globl	__l_setoxm1i
__l_setoxm1i:
	btst	#sign_bit,a0@(LOCAL_EX)	| check sign of source
	jne 	__l_ld_mone
	jra 	__l_ld_pinf
|
|	FLOGN
|
| Test for 1.0 as an input argument, returning +zero.  Also check
| the sign and return operr if negative.
|
	.globl	__l_sslogn
__l_sslogn:
	btst	#sign_bit,a0@(LOCAL_EX)
	jne 	__l_t_operr		| take care of operands < 0
	cmpiw	#0x3fff,a0@(LOCAL_EX) 	| test for 1.0 input
	jne 	__l_slogn
	cmpil	#0x80000000,a0@(LOCAL_HI)
	jne 	__l_slogn
	tstl	a0@(LOCAL_LO)
	jne 	__l_slogn
	fmovex	PZERO,fp0
	rts

	.globl	__l_sslognd
__l_sslognd:
	btst	#sign_bit,a0@(LOCAL_EX)
	jeq 	__l_slognd
	jra 	__l_t_operr		| take care of operands < 0

|
|	FLOG10
|
	.globl	__l_sslog10
__l_sslog10:
	btst	#sign_bit,a0@(LOCAL_EX)
	jne 	__l_t_operr		| take care of operands < 0
	cmpiw	#0x3fff,a0@(LOCAL_EX) 	| test for 1.0 input
	jne 	__l_slog10
	cmpil	#0x80000000,a0@(LOCAL_HI)
	jne 	__l_slog10
	tstl	a0@(LOCAL_LO)
	jne 	__l_slog10
	fmovex	PZERO,fp0
	rts

	.globl	__l_sslog10d
__l_sslog10d:
	btst	#sign_bit,a0@(LOCAL_EX)
	jeq 	__l_slog10d
	jra 	__l_t_operr		| take care of operands < 0

|
|	FLOG2
|
	.globl	__l_sslog2
__l_sslog2:
	btst	#sign_bit,a0@(LOCAL_EX)
	jne 	__l_t_operr		| take care of operands < 0
	cmpiw	#0x3fff,a0@(LOCAL_EX) 	| test for 1.0 input
	jne 	__l_slog2
	cmpil	#0x80000000,a0@(LOCAL_HI)
	jne 	__l_slog2
	tstl	a0@(LOCAL_LO)
	jne 	__l_slog2
	fmovex	PZERO,fp0
	rts

	.globl	__l_sslog2d
__l_sslog2d:
	btst	#sign_bit,a0@(LOCAL_EX)
	jeq 	__l_slog2d
	jra 	__l_t_operr		| take care of operands < 0

|
|	FMOD
|
__l_pmodt:
|				| 0x21 fmod
|				| dtag,stag
	.long	__l_smod	|   00,00  norm,norm = normal
	.long	__l_smod_oper	|   00,01  norm,zero = nan with operr
	.long	__l_smod_fpn	|   00,10  norm,inf  = fpn
	.long	__l_smod_snan	|   00,11  norm,nan  = nan
	.long	__l_smod_zro	|   01,00  zero,norm = +-zero
	.long	__l_smod_oper	|   01,01  zero,zero = nan with operr
	.long	__l_smod_zro	|   01,10  zero,inf  = +-zero
	.long	__l_smod_snan	|   01,11  zero,nan  = nan
	.long	__l_smod_oper	|   10,00  inf,norm  = nan with operr
	.long	__l_smod_oper	|   10,01  inf,zero  = nan with operr
	.long	__l_smod_oper	|   10,10  inf,inf   = nan with operr
	.long	__l_smod_snan	|   10,11  inf,nan   = nan
	.long	__l_smod_dnan	|   11,00  nan,norm  = nan
	.long	__l_smod_dnan	|   11,01  nan,zero  = nan
	.long	__l_smod_dnan	|   11,10  nan,inf   = nan
	.long	__l_smod_dnan	|   11,11  nan,nan   = nan

	.globl	__l_pmod
__l_pmod:
	clr.b	FPSR_QBYTE(a6) 	    | clear quotient field
	bfextu	a6@(STAG){#0:#3},d0 | stag = d0
	bfextu	a6@(DTAG){#0:#3},d1 | dtag = d1

|
| Alias extended denorms to norms for the jump table.
|
	bclr	#2,d0
	bclr	#2,d1

	lslb	#2,d1
	orb	d0,d1		| d1{3:2} = dtag, d1{1:0} = stag
|				| Tag values:
|				| 00 = norm or denorm
|				| 01 = zero
|				| 10 = inf
|				| 11 = nan
	lea	__l_pmodt,a1
	movel	a1@(d1:w:4),a1
	jmp	a1@

__l_smod_snan:
	jra 	__l_src_nan
__l_smod_dnan:
	jra 	__l_dst_nan
__l_smod_oper:
	jra 	__l_t_operr
__l_smod_zro:
	moveb	a6@(ETEMP),d1	| get sign of src op
	moveb	a6@(FPTEMP),d0	| get sign of dst op
	eorb	d0,d1		| get exor of sign bits
	btst	#7,d1		| test for sign
	jeq 	__l_smod_zsn	| if clr, do not set sign big
	bset	#q_sn_bit,a6@(FPSR_QBYTE) | set q-byte sign bit
__l_smod_zsn:
	btst	#7,d0		| test if + or -
	jeq 	__l_ld_pzero	| if pos then load +0
	jra 	__l_ld_mzero	| else neg load -0

__l_smod_fpn:
	moveb	a6@(ETEMP),d1	| get sign of src op
	moveb	a6@(FPTEMP),d0	| get sign of dst op
	eorb	d0,d1		| get exor of sign bits
	btst	#7,d1		| test for sign
	jeq 	__l_smod_fsn	| if clr, do not set sign big
	bset	#q_sn_bit,a6@(FPSR_QBYTE) | set q-byte sign bit
__l_smod_fsn:
	tstb	a6@(DTAG)	| filter out denormal destination case
	jpl 	__l_smod_nrm	|
	lea	a6@(FPTEMP),a0	| a0<- addr(FPTEMP)
	jra 	__l_t_resdnrm	| force UNFL(but exact) result
__l_smod_nrm:
	fmovel	a6@(USER_FPCR),fpcr 	/* | use user's rmode and precision */
	fmovex	a6@(FPTEMP),fp0		| return dest to fp0
	rts

|
|	FREM
|
__l_premt:
|				| 0x25 frem
|				| dtag,stag
	.long	__l_srem	|   00,00  norm,norm = normal
	.long	__l_srem_oper	|   00,01  norm,zero = nan with operr
	.long	__l_srem_fpn	|   00,10  norm,inf  = fpn
	.long	__l_srem_snan	|   00,11  norm,nan  = nan
	.long	__l_srem_zro	|   01,00  zero,norm = +-zero
	.long	__l_srem_oper	|   01,01  zero,zero = nan with operr
	.long	__l_srem_zro	|   01,10  zero,inf  = +-zero
	.long	__l_srem_snan	|   01,11  zero,nan  = nan
	.long	__l_srem_oper	|   10,00  inf,norm  = nan with operr
	.long	__l_srem_oper	|   10,01  inf,zero  = nan with operr
	.long	__l_srem_oper	|   10,10  inf,inf   = nan with operr
	.long	__l_srem_snan	|   10,11  inf,nan   = nan
	.long	__l_srem_dnan	|   11,00  nan,norm  = nan
	.long	__l_srem_dnan	|   11,01  nan,zero  = nan
	.long	__l_srem_dnan	|   11,10  nan,inf   = nan
	.long	__l_srem_dnan	|   11,11  nan,nan   = nan

	.globl	__l_prem
__l_prem:
	clr.b	FPSR_QBYTE(a6) 	    | clear quotient field
	bfextu	a6@(STAG){#0:#3},d0 | stag = d0
	bfextu	a6@(DTAG){#0:#3},d1 | dtag = d1
|
| Alias extended denorms to norms for the jump table.
|
	bclr	#2,d0
	bclr	#2,d1

	lslb	#2,d1
	orb	d0,d1		| d1{3:2} = dtag, d1{1:0} = stag
|				| Tag values:
|				| 00 = norm or denorm
|				| 01 = zero
|				| 10 = inf
|				| 11 = nan
	lea	__l_premt,a1
	movel	a1@(d1:w:4),a1
	jmp	a1@

__l_srem_snan:
	jra 	__l_src_nan
__l_srem_dnan:
	jra 	__l_dst_nan
__l_srem_oper:
	jra 	__l_t_operr
__l_srem_zro:
	moveb	a6@(ETEMP),d1	| get sign of src op
	moveb	a6@(FPTEMP),d0	| get sign of dst op
	eorb	d0,d1		| get exor of sign bits
	btst	#7,d1		| test for sign
	jeq 	__l_srem_zsn	| if clr, do not set sign big
	bset	#q_sn_bit,a6@(FPSR_QBYTE) | set q-byte sign bit
__l_srem_zsn:
	btst	#7,d0		| test if + or -
	jeq 	__l_ld_pzero	| if pos then load +0
	jra 	__l_ld_mzero	| else neg load -0

__l_srem_fpn:
	moveb	a6@(ETEMP),d1	| get sign of src op
	moveb	a6@(FPTEMP),d0	| get sign of dst op
	eorb	d0,d1		| get exor of sign bits
	btst	#7,d1		| test for sign
	jeq 	__l_srem_fsn	| if clr, do not set sign big
	bset	#q_sn_bit,a6@(FPSR_QBYTE) | set q-byte sign bit
__l_srem_fsn:
	tstb	a6@(DTAG)	| filter out denormal destination case
	jpl 	__l_srem_nrm	|
	lea	a6@(FPTEMP),a0	| a0<- addr(FPTEMP)
	jra 	__l_t_resdnrm	| force UNFL(but exact) result
__l_srem_nrm:
	fmovel	a6@(USER_FPCR),fpcr 	/* | use user's rmode and precision */
	fmovex	a6@(FPTEMP),fp0		| return dest to fp0
	rts
|
|	FSCALE
|
__l_pscalet:
|				| 0x26 fscale
|				| dtag,stag
	.long	__l_sscale	|   00,00  norm,norm = result
	.long	__l_sscale	|   00,01  norm,zero = fpn
	.long	scl_opr		|   00,10  norm,inf  = nan with operr
	.long	scl_snan	|   00,11  norm,nan  = nan
	.long	scl_zro		|   01,00  zero,norm = +-zero
	.long	scl_zro		|   01,01  zero,zero = +-zero
	.long	scl_opr		|   01,10  zero,inf  = nan with operr
	.long	scl_snan	|   01,11  zero,nan  = nan
	.long	scl_inf		|   10,00  inf,norm  = +-inf
	.long	scl_inf		|   10,01  inf,zero  = +-inf
	.long	scl_opr		|   10,10  inf,inf   = nan with operr
 	.long	scl_snan	|   10,11  inf,nan   = nan
 	.long	scl_dnan	|   11,00  nan,norm  = nan
 	.long	scl_dnan	|   11,01  nan,zero  = nan
 	.long	scl_dnan	|   11,10  nan,inf   = nan
	.long	scl_dnan	|   11,11  nan,nan   = nan

	.globl	__l_pscale
__l_pscale:
	bfextu	a6@(STAG){#0:#3},d0 | stag in d0
	bfextu	a6@(DTAG){#0:#3},d1 | dtag in d1
	bclr	#2,d0		| alias  denorm into norm
	bclr	#2,d1		| alias  denorm into norm
	lslb	#2,d1
	orb	d0,d1		| d1{4:2} = dtag, d1{1:0} = stag
|				| dtag values     stag values:
|				| 000 = norm      00 = norm
|				| 001 = zero	 01 = zero
|				| 010 = inf	 10 = inf
|				| 011 = nan	 11 = nan
|				| 100 = dnrm
|
|
	lea	__l_pscalet,a1	| load start of jump table
	movel	a1@(d1:w:4),a1	| load a1 with label depending on tag
	jmp	a1@		| go to the routine

scl_opr:
	jra 	__l_t_operr

scl_dnan:
	jra 	__l_dst_nan

scl_zro:
	btst	#sign_bit,a6@(FPTEMP_EX)	| test if + or -
	jeq 	__l_ld_pzero			| if pos then load +0
	jra 	__l_ld_mzero			| if neg then load -0
scl_inf:
	btst	#sign_bit,a6@(FPTEMP_EX)	| test if + or -
	jeq 	__l_ld_pinf			| if pos then load +inf
	jra 	__l_ld_minf			| else neg load -inf
scl_snan:
	jra 	__l_src_nan
|
|	FSINCOS
|
	.globl	__l_ssincosz
__l_ssincosz:
	btst	#sign_bit,a6@(ETEMP)		| get sign
	jeq 	sincosp
	fmovex	MZERO,fp0
	jra 	sincoscom
sincosp:
	fmovex PZERO,fp0
sincoscom:
  	fmovemx PONE,fp1-fp1		| do not allow FPSR to be affected
	jra 	__l_sto_cos		| store cosine result

	.globl	__l_ssincosi
__l_ssincosi:
	fmovex QNAN,fp1	| load NAN
	bsrl	__l_sto_cos		| store cosine result
	fmovex QNAN,fp0	| load NAN
	jra 	__l_t_operr

	.globl	__l_ssincosnan
__l_ssincosnan:
	movel	a6@(ETEMP_EX),a6@(FP_SCR1)
	movel	a6@(ETEMP_HI),a6@(FP_SCR1+4)
	movel	a6@(ETEMP_LO),a6@(FP_SCR1+8)
	bset	#signan_bit,a6@(FP_SCR1+4)
	fmovemx	a6@(FP_SCR1),fp1-fp1
	bsrl	__l_sto_cos
	jra 	__l_src_nan
|
| This code forces default values for the zero, inf, and nan cases
| in the transcendentals code.  The CC bits must be set in the
| stacked FPSR to be correctly reported.
|
|**Returns +PI/2
	.globl	__l_ld_ppi2
__l_ld_ppi2:
	fmovex PPIBY2,fp0			| load +pi/2
	jra 	__l_t_inx2			| set inex2 exc

|**Returns -PI/2
	.globl	__l_ld_mpi2
__l_ld_mpi2:
	fmovex MPIBY2,fp0			| load -pi/2
	orl	#neg_mask,a6@(USER_FPSR)	| set N bit
	jra 	__l_t_inx2			| set inex2 exc

|**Returns +inf
	.globl	__l_ld_pinf
__l_ld_pinf:
	fmovex PINF,fp0				| load +inf
	orl	#inf_mask,a6@(USER_FPSR)	| set I bit
	rts

|**Returns -inf
	.globl	__l_ld_minf
__l_ld_minf:
	fmovex MINF,fp0					| load -inf
	orl	#neg_mask+inf_mask,a6@(USER_FPSR)	| set N and I bits
	rts

|**Returns +1
	.globl	__l_ld_pone
__l_ld_pone:
	fmovex PONE,fp0				| load +1
	rts

|**Returns -1
	.globl	__l_ld_mone
__l_ld_mone:
	fmovex MONE,fp0				| load -1
	orl	#neg_mask,a6@(USER_FPSR)	| set N bit
	rts

|**Returns +0
	.globl	__l_ld_pzero
__l_ld_pzero:
	fmovex PZERO,fp0			| load +0
	orl	#z_mask,a6@(USER_FPSR)		| set Z bit
	rts

|**Returns -0
	.globl	__l_ld_mzero
__l_ld_mzero:
	fmovex MZERO,fp0			| load -0
	orl	#neg_mask+z_mask,a6@(USER_FPSR)	| set N and Z bits
	rts

|	end
