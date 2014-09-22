/* util.s - Motorola 68040 FP utility routines (EXC) */

/* Copyright 1991-1993 Wind River Systems, Inc. */
	.data
	.globl	_copyright_wind_river
	.long	_copyright_wind_river

/*
modification history
--------------------
01f,21jul93,kdl  added .text (SPR #2372).
01e,23aug92,jcf  changed bxxx to jxx.
01d,26may92,rrr  the tree shuffle
01c,10jan92,kdl  general cleanup.
01b,17dec91,kdl	 put in changes from Motorola v3.7 (FPSP v2.1):
		 handle implied precision in __x_ovf_r_k;
		 comment-out __x_check_force routine.
01a,15aug91,kdl  original version, from Motorola FPSP v2.0;
		 changed {byte,word,long}_d{0-7} labels to avoid
		 confusion when converting to mit format.
*/

/*
DESCRIPTION


	utilsa 3.3 4/26/91

	This file contains routines used by other programs.

	__x_ovf_res: used by overflow to force the correct
		     result. __x_ovf_r_k, __x_ovf_r_x2, __x_ovf_r_x3 are
		     derivatives of this routine.
	__x_get_fline: get user's opcode word
	__x_g_dfmtou: returns the destination format.
	__x_g_opcls: returns the opclass of the float instruction.
	__x_g_rndpr: returns the rounding precision.
	__x_reg_dest: write byte, word, or long data to Dn


		Copyright (C) Motorola, Inc. 1990
			All Rights Reserved

	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA
	The copyright notice above does not evidence any
	actual or intended publication of such source code.

UTIL	idnt    2,1 Motorola 040 Floating Point Software Package

	section	8

NOMANUAL
*/

#include "fpsp040E.h"

|	xref	__x_mem_read

	.globl	__x_g_dfmtou
	.globl	__x_g_opcls
	.globl	__x_g_rndpr
	.globl	__x_get_fline
	.globl	__x_reg_dest

|
| Final result table for __x_ovf_res. Note that the negative counterparts
| are unnecessary as __x_ovf_res always returns the sign separately from
| the exponent.
|					| +inf
EXT_PINF:	.long	0x7fff0000,0x00000000,0x00000000,0x00000000
|					| largest +ext
EXT_PLRG:	.long	0x7ffe0000,0xffffffff,0xffffffff,0x00000000
|					| largest magnitude +sgl in ext
SGL_PLRG:	.long	0x407e0000,0xffffff00,0x00000000,0x00000000
|					| largest magnitude +dbl in ext
DBL_PLRG:	.long	0x43fe0000,0xffffffff,0xfffff800,0x00000000
|					| largest -ext

tblovfl:
	.long	EXT_RN
	.long	EXT_RZ
	.long	EXT_RM
	.long	EXT_RP
	.long	SGL_RN
	.long	SGL_RZ
	.long	SGL_RM
	.long	SGL_RP
	.long	DBL_RN
	.long	DBL_RZ
	.long	DBL_RM
	.long	DBL_RP
	.long	error
	.long	error
	.long	error
	.long	error

|	.globl	__x_check_force
|
| 	__x_check_force --- check the rounding precision of the instruction
|
| This entry point is used by the exception handlers.
|
| Check the CMDREG1B or CMDREG3B command word to decide the rounding
| precision of the instruction.
|
| Input:	rounding precision in	a6@(USER_FPCR).
| Output:	a6@(USER_FPCR) is possibly modified to reflect the
|		rounding precision of the instruction.
| Register Usage:	d0 is scratch register.

|__x_check_force:
|	btst	#E3,a6@(E_BYTE)		| check for nu exception
|	jeq 	e1_exc			| it is cu exception
|e3_exc:
|	movew	a6@(CMDREG3B),d0	| get the command word
|	andiw	#0x00000060,d0		| clear all bits except 6 and 5
|	cmpil	#0x00000040,d0
|	jeq 	f_sgl			| force precision is single
|	cmpil	#0x00000060,d0
|	jeq 	f_dbl			| force precision is double
|	movew	a6@(CMDREG3B),d0	| get the command word again
|	andil	#0x7f,d0		| clear all except operation
|	cmpil	#0x33,d0
|	jeq 	f_sglmd			| fsglmul or fsgldiv
|	cmpil	#0x30,d0
|	jeq 	f_sglmd
|	rts
|e1_exc:
|	movew	a6@(CMDREG1B),d0	| get command word
|	andil	#0x00000044,d0		| clear all bits except 6 and 2
|	cmpil	#0x00000040,d0
|	jeq 	f_sgl			| the instruction is force single
|	cmpil	#0x00000044,d0
|	jeq 	f_dbl			| the instruction is force double
|	movew	a6@(CMDREG1B),d0		| again get the command word
|	andil	#0x0000007f,d0		| clear all except the op code
|	cmpil	#0x00000027,d0
|	jeq 	f_sglmd			| fsglmul
|	cmpil 	#0x00000024,d0
|	jeq 	f_sglmd			| fsgldiv
|	rts
|f_sgl:
|	andib	#0x3f,a6@(fpcr_MODE)	| single precision
|	orib	#0x40,a6@(fpcr_MODE)
|	rts
|f_dbl:
|	andib	#0x3f,a6@(fpcr_MODE)
|	orib	#0x80,a6@(fpcr_MODE)
|	rts
|f_sglmd:
|	andib	#0x3f,a6@(fpcr_MODE)
|	rts

|
|	__x_ovf_r_k --- overflow result calculation
|
| This entry point is used by kernel_ex.
|
| This forces the destination precision to be extended
|
| Input:	operand in ETEMP
| Output:	a result is in ETEMP (internal extended format)
|

	.text

	.globl	__x_ovf_r_k
__x_ovf_r_k:
	lea	a6@(ETEMP),a0		| a0 points to source operand
	bclr	#sign_bit,a6@(ETEMP_EX)
	sne	a6@(ETEMP_SGN)		| convert to internal IEEE format

|
|	__x_ovf_r_x2 --- overflow result calculation
|
| This entry point used by x_ovfl.  (opclass 0 and 2)
|
| Input		a0  points to an operand in the internal extended format
| Output	a0  points to the result in the internal extended format
|
| This sets the round precision according to the user fpcr unless the
| instruction is fsgldiv or fsglmul or fsadd, fdadd, fsub, fdsub, fsmul,
| fdmul, fsdiv, fddiv, fssqrt, fsmove, fdmove, fsabs, fdabs, fsneg, fdneg.
| If the instruction is fsgldiv of fsglmul, the rounding precision must be
| extended.  If the instruction is not fsgldiv or fsglmul but a force-
| precision instruction, the rounding precision is then set to the force
| precision.

	.globl	__x_ovf_r_x2
__x_ovf_r_x2:
	btst	#E3,a6@(E_BYTE)		| check for nu exception
	jeq 	ovf_e1_exc		| it is cu exception
ovf_e3_exc:
	movew	a6@(CMDREG3B),d0	| get the command word
	andiw	#0x00000060,d0		| clear all bits except 6 and 5
	cmpil	#0x00000040,d0
	jeq 	ovff_sgl		| force precision is single
	cmpil	#0x00000060,d0
	jeq 	ovff_dbl		| force precision is double
	movew	a6@(CMDREG3B),d0	| get the command word again
	andil	#0x7f,d0		| clear all except operation
	cmpil	#0x33,d0
	jeq 	ovf_fsgl		| fsglmul or fsgldiv
	cmpil	#0x30,d0
	jeq 	ovf_fsgl
	jra 	ovf_fpcr		| instruction is none of the above
|					| use fpcr
ovf_e1_exc:
	movew	a6@(CMDREG1B),d0	| get command word
	andil	#0x00000044,d0		| clear all bits except 6 and 2
	cmpil	#0x00000040,d0
	jeq 	ovff_sgl		| the instruction is force single
	cmpil	#0x00000044,d0
	jeq 	ovff_dbl		| the instruction is force double
	movew	a6@(CMDREG1B),d0	| again get the command word
	andil	#0x0000007f,d0		| clear all except the op code
	cmpil	#0x00000027,d0
	jeq 	ovf_fsgl		| fsglmul
	cmpil 	#0x00000024,d0
	jeq 	ovf_fsgl		| fsgldiv
	jra 	ovf_fpcr		| none of the above, use fpcr
|
|
| Inst is either fsgldiv or fsglmul.  Force extended precision.
|
ovf_fsgl:
	clrl	d0
	jra 	__x_ovf_res

ovff_sgl:
	movel	#0x00000001,d0		| set single
	jra 	__x_ovf_res
ovff_dbl:
	movel	#0x00000002,d0		| set double
	jra 	__x_ovf_res
|
| The precision is in the fpcr.
|
ovf_fpcr:
	bfextu	a6@(fpcr_MODE){#0:#2},d0 | set round precision
	jra 	__x_ovf_res

|
|
|	__x_ovf_r_x3 --- overflow result calculation
|
| This entry point used by x_ovfl. (opclass 3 only)
|
| Input		a0  points to an operand in the internal extended format
| Output	a0  points to the result in the internal extended format
|
| This sets the round precision according to the destination size.
|
	.globl	__x_ovf_r_x3
__x_ovf_r_x3:
	bsrl	__x_g_dfmtou	| get dest fmt in d0{1:0}
|				| for fmovout, the destination format
|				| is the rounding precision

|
|	__x_ovf_res --- overflow result calculation
|
| Input:
|	a0 	points to operand in internal extended format
| Output:
|	a0 	points to result in internal extended format
|
	.globl	__x_ovf_res
__x_ovf_res:
	lsll	#2,d0			| move round precision to d0{3:2}
	bfextu	a6@(fpcr_MODE){#2:#2},d1 | set round mode
	orl	d1,d0			| index is fmt:mode in d0{3:0}
	lea	tblovfl,a1		| load a1 with table address
	movel	a1@(d0:w:4),a1		| use d0 as index to the table
	jmp	a1@			| go to the correct routine
|
|case DEST_FMT = EXT
|
EXT_RN:
	lea	EXT_PINF,a1		| answer is +/- infinity
	bset	#inf_bit,a6@(FPSR_CC)
	jra 	set_sign		| now go set the sign
EXT_RZ:
	lea	EXT_PLRG,a1		| answer is +/- large number
	jra 	set_sign		| now go set the sign
EXT_RM:
	tstb	a0@(LOCAL_SGN)		| if negative overflow
	jeq 	e_rm_pos
e_rm_neg:
	lea	EXT_PINF,a1		| answer is negative infinity
	orl	#neginf_mask,a6@(USER_FPSR)
	jra 	end_ovfr
e_rm_pos:
	lea	EXT_PLRG,a1		| answer is large positive number
	jra 	end_ovfr
EXT_RP:
	tstb	a0@(LOCAL_SGN)		| if negative overflow
	jeq 	e_rp_pos
e_rp_neg:
	lea	EXT_PLRG,a1		| answer is large negative number
	bset	#neg_bit,a6@(FPSR_CC)
	jra 	end_ovfr
e_rp_pos:
	lea	EXT_PINF,a1		| answer is positive infinity
	bset	#inf_bit,a6@(FPSR_CC)
	jra 	end_ovfr
|
|case DEST_FMT = DBL
|
DBL_RN:
	lea	EXT_PINF,a1		| answer is +/- infinity
	bset	#inf_bit,a6@(FPSR_CC)
	jra 	set_sign
DBL_RZ:
	lea	DBL_PLRG,a1		| answer is +/- large number
	jra 	set_sign		| now go set the sign
DBL_RM:
	tstb	a0@(LOCAL_SGN)		| if negative overflow
	jeq 	d_rm_pos
d_rm_neg:
	lea	EXT_PINF,a1		| answer is negative infinity
	orl	#neginf_mask,a6@(USER_FPSR)
	jra 	end_ovfr	| inf is same for all precisions (ext,dbl,sgl)
d_rm_pos:
	lea	DBL_PLRG,a1		| answer is large positive number
	jra 	end_ovfr
DBL_RP:
	tstb	a0@(LOCAL_SGN)		| if negative overflow
	jeq 	d_rp_pos
d_rp_neg:
	lea	DBL_PLRG,a1		| answer is large negative number
	bset	#neg_bit,a6@(FPSR_CC)
	jra 	end_ovfr
d_rp_pos:
	lea	EXT_PINF,a1		| answer is positive infinity
	bset	#inf_bit,a6@(FPSR_CC)
	jra 	end_ovfr
|
|case DEST_FMT = SGL
|
SGL_RN:
	lea	EXT_PINF,a1		| answer is +/-  infinity
	bset	#inf_bit,a6@(FPSR_CC)
	jra 	set_sign
SGL_RZ:
	lea	SGL_PLRG,a1		| anwer is +/- large number
	jra 	set_sign
SGL_RM:
	tstb	a0@(LOCAL_SGN)		| if negative overflow
	jeq 	s_rm_pos
s_rm_neg:
	lea	EXT_PINF,a1		| answer is negative infinity
	orl	#neginf_mask,a6@(USER_FPSR)
	jra 	end_ovfr
s_rm_pos:
	lea	SGL_PLRG,a1		| answer is large positive number
	jra 	end_ovfr
SGL_RP:
	tstb	a0@(LOCAL_SGN)		| if negative overflow
	jeq 	s_rp_pos
s_rp_neg:
	lea	SGL_PLRG,a1		| answer is large negative number
	bset	#neg_bit,a6@(FPSR_CC)
	jra 	end_ovfr
s_rp_pos:
	lea	EXT_PINF,a1		| answer is postive infinity
	bset	#inf_bit,a6@(FPSR_CC)
	jra 	end_ovfr

set_sign:
	tstb	a0@(LOCAL_SGN)		| if negative overflow
	jeq 	end_ovfr
neg_sign:
	bset	#neg_bit,a6@(FPSR_CC)

end_ovfr:
	movew	a1@(LOCAL_EX),a0@(LOCAL_EX) | do not overwrite sign
	movel	a1@(LOCAL_HI),a0@(LOCAL_HI)
	movel	a1@(LOCAL_LO),a0@(LOCAL_LO)
	rts


|
|	ERROR
|
error:
	rts
|
|	__x_get_fline --- get f-line opcode of interrupted instruction
|
|	Returns opcode in the low word of d0.
|
__x_get_fline:
	movel	a6@(USER_fpi),a0	| opcode address
	movel	#0,a7@-			| reserve a word on the stack
	lea	a7@(2),a1		| point to low word of temporary
	movel	#2,d0			| count
	bsrl	__x_mem_read
	movel	a7@+,d0
	rts
|
| 	__x_g_rndpr --- put rounding precision in d0{1:0}
|
|	valid return codes are:
|		00 - extended
|		01 - single
|		10 - double
|
| begin
| get rounding precision (cmdreg3b{6:5})
| begin
|  case	opclass = 011 (move out)
|	get destination format - this is the also the rounding precision
|
|  case	opclass = 0x0
|	if E3
|	    *case RndPr(from cmdreg3b{6:5} = 11  then RND_PREC = DBL
|	    *case RndPr(from cmdreg3b{6:5} = 10  then RND_PREC = SGL
|	     case RndPr(from cmdreg3b{6:5} = 00 | 01
|		use precision from fpcr{7:6}
|			case 00 then RND_PREC = EXT
|			case 01 then RND_PREC = SGL
|			case 10 then RND_PREC = DBL
|	else E1
|	     use precision in fpcr{7:6}
|	     case 00 then RND_PREC = EXT
|	     case 01 then RND_PREC = SGL
|	     case 10 then RND_PREC = DBL
| end
|
__x_g_rndpr:
	bsrl	__x_g_opcls		| get opclass in d0{2:0}
	cmpw	#0x0003,d0		| check for opclass 011
	jne 	op_0x0

|
| For move out instructions (opclass 011) the destination format
| is the same as the rounding precision.  Pass results from __x_g_dfmtou.
|
	bsrl 	__x_g_dfmtou
	rts
op_0x0:
	btst	#E3,a6@(E_BYTE)
	jeq 	unf_e1_exc		| branch to e1 underflow
unf_e3_exc:
	movel	a6@(CMDREG3B),d0	| rounding precision in d0{10:9}
	bfextu	d0{#9:#2},d0		| move the rounding prec bits to d0{1:0}
	cmpil	#0x2,d0
	jeq 	unff_sgl		| force precision is single
	cmpil	#0x3,d0			| force precision is double
	jeq 	unff_dbl
	movew	a6@(CMDREG3B),d0	| get the command word again
	andil	#0x7f,d0		| clear all except operation
	cmpil	#0x33,d0
	jeq 	unf_fsgl		| fsglmul or fsgldiv
	cmpil	#0x30,d0
	jeq 	unf_fsgl		| fsgldiv or fsglmul
	jra 	unf_fpcr
unf_e1_exc:
	movel	a6@(CMDREG1B),d0	| get 32 bits off the stack, 1st 16 bits
|					| are the command word
	andil	#0x00440000,d0		| clear all bits except bits 6 and 2
	cmpil	#0x00400000,d0
	jeq 	unff_sgl		| force single
	cmpil	#0x00440000,d0		| force double
	jeq 	unff_dbl
	movel	a6@(CMDREG1B),d0	| get the command word again
	andil	#0x007f0000,d0		| clear all bits except the operation
	cmpil	#0x00270000,d0
	jeq 	unf_fsgl		| fsglmul
	cmpil	#0x00240000,d0
	jeq 	unf_fsgl		| fsgldiv
	jra 	unf_fpcr

|
| Convert to return format.  The values from cmdreg3b and the return
| values are:
|	cmdreg3b	return	     precision
|	--------	------	     ---------
|	  00,01		  0		ext
|	   10		  1		sgl
|	   11		  2		dbl
| Force single
|
unff_sgl:
	movel	#1,d0		| return 1
	rts
|
| Force double
|
unff_dbl:
	movel	#2,d0			| return 2
	rts
|
| Force extended
|
unf_fsgl:
	movel	#0,d0
	rts
|
| Get rounding precision set in fpcr{7:6}.
|
unf_fpcr:
	movel	a6@(USER_FPCR),d0 	| rounding precision bits in d0{7:6}
	bfextu	d0{#24:#2},d0		| move the rounding prec bits to d0{1:0}
	rts
|
|	__x_g_opcls --- put opclass in d0{2:0}
|
__x_g_opcls:
	btst	#E3,a6@(E_BYTE)
	jeq 	opc_1b			| if set, go to cmdreg1b
opc_3b:
	clrl	d0			| if E3, only opclass 0x0 is possible
	rts
opc_1b:
	movel	a6@(CMDREG1B),d0
	bfextu	d0{#0:#3},d0	| shift opclass bits d0{31:29} to d0{2:0}
	rts
|
|	__x_g_dfmtou --- put destination format in d0{1:0}
|
|	If E1, the format is from cmdreg1b{12:10}
|	If E3, the format is extended.
|
|	Dest. Fmt.
|		extended  010 -> 00
|		single    001 -> 01
|		double    101 -> 10
|
__x_g_dfmtou:
	btst	#E3,a6@(E_BYTE)
	jeq 	op011
	clrl	d0			| if E1, size is always ext
	rts
op011:
	movel	a6@(CMDREG1B),d0
	bfextu	d0{#3:#3},d0		| dest fmt from cmdreg1b{12:10}
	cmpb	#1,d0			| check for single
	jne 	not_sgl
	movel	#1,d0
	rts
not_sgl:
	cmpb	#5,d0			| check for double
	jne 	not_dbl
	movel	#2,d0
	rts
not_dbl:
	clrl	d0			| must be extended
	rts

|
|
| Final result table for __x_unf_sub. Note that the negative counterparts
| are unnecessary as __x_unf_sub always returns the sign separately from
| the exponent.
|					| +zero
EXT_PZRO:	.long	0x00000000,0x00000000,0x00000000,0x00000000
|					| +zero
SGL_PZRO:	.long	0x3f810000,0x00000000,0x00000000,0x00000000
|					| +zero
DBL_PZRO:	.long	0x3c010000,0x00000000,0x00000000,0x00000000
|					| smallest +ext denorm
EXT_PSML:	.long	0x00000000,0x00000000,0x00000001,0x00000000
|					| smallest +sgl denorm
SGL_PSML:	.long	0x3f810000,0x00000100,0x00000000,0x00000000
|					| smallest +dbl denorm
DBL_PSML:	.long	0x3c010000,0x00000000,0x00000800,0x00000000
|
|	UNF_SUB --- underflow result calculation
|
| Input:
|	d0 	contains round precision
|	a0	points to input operand in the internal extended format
|
| Output:
|	a0 	points to correct internal extended precision result.
|

tblunf:
	.long	uEXT_RN
	.long	uEXT_RZ
	.long	uEXT_RM
	.long	uEXT_RP
	.long	uSGL_RN
	.long	uSGL_RZ
	.long	uSGL_RM
	.long	uSGL_RP
	.long	uDBL_RN
	.long	uDBL_RZ
	.long	uDBL_RM
	.long	uDBL_RP
	.long	uDBL_RN
	.long	uDBL_RZ
	.long	uDBL_RM
	.long	uDBL_RP

	.globl	__x_unf_sub
__x_unf_sub:
	lsll	#2,d0			| move round precision to d0{3:2}
	bfextu	a6@(fpcr_MODE){#2:#2},d1 | set round mode
	orl	d1,d0			| index is fmt:mode in d0{3:0}
	lea	tblunf,a1		| load a1 with table address
	movel	a1@(d0:w:4),a1		| use d0 as index to the table
	jmp	a1@			| go to the correct routine
|
|case DEST_FMT = EXT
|
uEXT_RN:
	lea	EXT_PZRO,a1		| answer is +/- zero
	bset	#z_bit,a6@(FPSR_CC)
	jra 	uset_sign		| now go set the sign
uEXT_RZ:
	lea	EXT_PZRO,a1		| answer is +/- zero
	bset	#z_bit,a6@(FPSR_CC)
	jra 	uset_sign		| now go set the sign
uEXT_RM:
	tstb	a0@(LOCAL_SGN)		| if negative underflow
	jeq 	ue_rm_pos
ue_rm_neg:
	lea	EXT_PSML,a1		| answer is negative smallest denorm
	bset	#neg_bit,a6@(FPSR_CC)
	jra 	end_unfr
ue_rm_pos:
	lea	EXT_PZRO,a1		| answer is positive zero
	bset	#z_bit,a6@(FPSR_CC)
	jra 	end_unfr
uEXT_RP:
	tstb	a0@(LOCAL_SGN)		| if negative underflow
	jeq 	ue_rp_pos
ue_rp_neg:
	lea	EXT_PZRO,a1		| answer is negative zero
	oril	#negz_mask,a6@(USER_FPSR)
	jra 	end_unfr
ue_rp_pos:
	lea	EXT_PSML,a1		| answer is positive smallest denorm
	jra 	end_unfr
|
|case DEST_FMT = DBL
|
uDBL_RN:
	lea	DBL_PZRO,a1		| answer is +/- zero
	bset	#z_bit,a6@(FPSR_CC)
	jra 	uset_sign
uDBL_RZ:
	lea	DBL_PZRO,a1		| answer is +/- zero
	bset	#z_bit,a6@(FPSR_CC)
	jra 	uset_sign		| now go set the sign
uDBL_RM:
	tstb	a0@(LOCAL_SGN)		| if negative overflow
	jeq 	ud_rm_pos
ud_rm_neg:
	lea	DBL_PSML,a1		| answer is smallest denorm negative
	bset	#neg_bit,a6@(FPSR_CC)
	jra 	end_unfr
ud_rm_pos:
	lea	DBL_PZRO,a1		| answer is positive zero
	bset	#z_bit,a6@(FPSR_CC)
	jra 	end_unfr
uDBL_RP:
	tstb	a0@(LOCAL_SGN)		| if negative overflow
	jeq 	ud_rp_pos
ud_rp_neg:
	lea	DBL_PZRO,a1		| answer is negative zero
	oril	#negz_mask,a6@(USER_FPSR)
	jra 	end_unfr
ud_rp_pos:
	lea	DBL_PSML,a1		| answer is smallest denorm negative
	jra 	end_unfr
|
|case DEST_FMT = SGL
|
uSGL_RN:
	lea	SGL_PZRO,a1		| answer is +/- zero
	bset	#z_bit,a6@(FPSR_CC)
	jra 	uset_sign
uSGL_RZ:
	lea	SGL_PZRO,a1		| answer is +/- zero
	bset	#z_bit,a6@(FPSR_CC)
	jra 	uset_sign
uSGL_RM:
	tstb	a0@(LOCAL_SGN)		| if negative overflow
	jeq 	us_rm_pos
us_rm_neg:
	lea	SGL_PSML,a1		| answer is smallest denorm negative
	bset	#neg_bit,a6@(FPSR_CC)
	jra 	end_unfr
us_rm_pos:
	lea	SGL_PZRO,a1		| answer is positive zero
	bset	#z_bit,a6@(FPSR_CC)
	jra 	end_unfr
uSGL_RP:
	tstb	a0@(LOCAL_SGN)		| if negative overflow
	jeq 	us_rp_pos
us_rp_neg:
	lea	SGL_PZRO,a1		| answer is negative zero
	oril	#negz_mask,a6@(USER_FPSR)
	jra 	end_unfr
us_rp_pos:
	lea	SGL_PSML,a1		| answer is smallest denorm positive
	jra 	end_unfr

uset_sign:
	tstb	a0@(LOCAL_SGN)		| if negative overflow
	jeq 	end_unfr
uneg_sign:
	bset	#neg_bit,a6@(FPSR_CC)

end_unfr:
	movew	a1@(LOCAL_EX),a0@(LOCAL_EX) | be careful not to overwrite sign
	movel	a1@(LOCAL_HI),a0@(LOCAL_HI)
	movel	a1@(LOCAL_LO),a0@(LOCAL_LO)
	rts
|
|	__x_reg_dest --- write byte, word, or long data to Dn
|
|
| Input:
|	L_SCR1: Data
|	d1:     data size and dest register number formatted as:
|
|	32		5    4     3     2     1     0
|       -----------------------------------------------
|       |        0        |    Size   |  Dest Reg #   |
|       -----------------------------------------------
|
|	Size is:
|		0 - Byte
|		1 - Word
|		2 - Long/Single
|
pregdst:
	.long	__byte_d0
	.long	__byte_d1
	.long	__byte_d2
	.long	__byte_d3
	.long	__byte_d4
	.long	__byte_d5
	.long	__byte_d6
	.long	__byte_d7
	.long	__word_d0
	.long	__word_d1
	.long	__word_d2
	.long	__word_d3
	.long	__word_d4
	.long	__word_d5
	.long	__word_d6
	.long	__word_d7
	.long	__long_d0
	.long	__long_d1
	.long	__long_d2
	.long	__long_d3
	.long	__long_d4
	.long	__long_d5
	.long	__long_d6
	.long	__long_d7

__x_reg_dest:
	lea	pregdst,a0
	movel	a0@(d1:w:4),a0
	jmp	a0@

__byte_d0:
	moveb	a6@(L_SCR1),a6@(USER_D0+3)
	rts
__byte_d1:
	moveb	a6@(L_SCR1),a6@(USER_D1+3)
	rts
__byte_d2:
	moveb	a6@(L_SCR1),d2
	rts
__byte_d3:
	moveb	a6@(L_SCR1),d3
	rts
__byte_d4:
	moveb	a6@(L_SCR1),d4
	rts
__byte_d5:
	moveb	a6@(L_SCR1),d5
	rts
__byte_d6:
	moveb	a6@(L_SCR1),d6
	rts
__byte_d7:
	moveb	a6@(L_SCR1),d7
	rts
__word_d0:
	movew	a6@(L_SCR1),a6@(USER_D0+2)
	rts
__word_d1:
	movew	a6@(L_SCR1),a6@(USER_D1+2)
	rts
__word_d2:
	movew	a6@(L_SCR1),d2
	rts
__word_d3:
	movew	a6@(L_SCR1),d3
	rts
__word_d4:
	movew	a6@(L_SCR1),d4
	rts
__word_d5:
	movew	a6@(L_SCR1),d5
	rts
__word_d6:
	movew	a6@(L_SCR1),d6
	rts
__word_d7:
	movew	a6@(L_SCR1),d7
	rts
__long_d0:
	movel	a6@(L_SCR1),a6@(USER_D0)
	rts
__long_d1:
	movel	a6@(L_SCR1),a6@(USER_D1)
	rts
__long_d2:
	movel	a6@(L_SCR1),d2
	rts
__long_d3:
	movel	a6@(L_SCR1),d3
	rts
__long_d4:
	movel	a6@(L_SCR1),d4
	rts
__long_d5:
	movel	a6@(L_SCR1),d5
	rts
__long_d6:
	movel	a6@(L_SCR1),d6
	rts
__long_d7:
	movel	a6@(L_SCR1),d7
	rts
|	end
