/* get_op.s - Motorola 68040 FP get opclass routine (EXC) */

/* Copyright 1991-1993 Wind River Systems, Inc. */
	.data
	.globl	_copyright_wind_river
	.long	_copyright_wind_river

/*
modification history
--------------------
01f,31may96,ms   updated to mototorola version 2.3
01e,21jul93,kdl  added .text (SPR #2372).
01d,23aug92,jcf  changed bxxx to jxx.
01c,26may92,rrr  the tree shuffle
01b,10jan92,kdl  added modification history; general cleanup.
01a,15aug91,kdl  original version, from Motorola FPSP v2.0.
*/

/*
DESCRIPTION


	get_opsa 3.5 4/26/91

 Description: This routine is called by the unsupported format/data
type exception handler ('unsupp' - vector 55) and the unimplemented
instruction exception handler ('unimp' - vector 11).  'get_op'
determines the opclass (0, 2, or 3) and branches to the
opclass handler routine.  See 68881/2 User's Manual table 4-11
for a description of the opclasses.

For UNSUPPORTED data/format (exception vector 55) and for
UNIMPLEMENTED instructions (exception vector 11) the following
applies:

- For unnormormalized numbers (opclass 0, 2, or 3) the
number(s) is normalized and the operand type tag is updated.

- For a packed number (opclass 2) the number is unpacked and the
operand type tag is updated.

- For denormalized numbers (opclass 0 or 2) the number(s) is not
changed but passed to the next module.  The next module for
unimp is __x_do_func, the next module for unsupp is __x_res_func.

For UNSUPPORTED data/format (exception vector 55) only the
following applies:

- If there is a move out with a packed number (opclass 3) the
number is packed and written to user memory.  For the other
opclasses the number(s) are written back to the fsave stack
and the instruction is then restored back into the '040.  The
'040 is then able to complete the instruction.

For example:
faddx fpm,fpn where the fpm contains an unnormalized number.
The '040 takes an unsupported data trap and gets to this
routine.  The number is normalized, put back on the stack and
then an frestore is done to restore the instruction back into
the '040.  The '040 then re-executes the faddx fpm,fpn with
a normalized number in the source and the instruction is
successful.

Next consider if in the process of normalizing the un-
normalized number it becomes a denormalized number.  The
routine which converts the unnorm to a norm (called mk_norm)
detects this and tags the number as a denorm.  The routine
__x_res_func sees the denorm tag and converts the denorm to a
norm.  The instruction is then restored back into the '040
which re_executess the instruction.


		Copyright (C) Motorola, Inc. 1990
			All Rights Reserved

	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA
	The copyright notice above does not evidence any
	actual or intended publication of such source code.

GET_OP    idnt    2,1 Motorola 040 Floating Point Software Package

	section	8

NOMANUAL
*/

#include "fpsp040E.h"

	.globl	__x_PIRN,__x_PIRZRM,__x_PIRP
	.globl	__x_SMALRN,__x_SMALRZRM,__x_SMALRP
	.globl	__x_BIGRN,__x_BIGRZRM,__x_BIGRP

__x_PIRN:
	.long 0x40000000,0xc90fdaa2,0x2168c235    | pi
__x_PIRZRM:
	.long 0x40000000,0xc90fdaa2,0x2168c234    | pi
__x_PIRP:
	.long 0x40000000,0xc90fdaa2,0x2168c235    | pi

|round to nearest
__x_SMALRN:
	.long 0x3ffd0000,0x9a209a84,0xfbcff798    | log10(2)
	.long 0x40000000,0xadf85458,0xa2bb4a9a    | e
	.long 0x3fff0000,0xb8aa3b29,0x5c17f0bc    | log2(e)
	.long 0x3ffd0000,0xde5bd8a9,0x37287195    | log10(e)
	.long 0x00000000,0x00000000,0x00000000    | 0.0
| round to zero; round to negative infinity
__x_SMALRZRM:
	.long 0x3ffd0000,0x9a209a84,0xfbcff798    | log10(2)
	.long 0x40000000,0xadf85458,0xa2bb4a9a    | e
	.long 0x3fff0000,0xb8aa3b29,0x5c17f0bb    | log2(e)
	.long 0x3ffd0000,0xde5bd8a9,0x37287195    | log10(e)
	.long 0x00000000,0x00000000,0x00000000    | 0.0
| round to positive infinity
__x_SMALRP:
	.long 0x3ffd0000,0x9a209a84,0xfbcff799    | log10(2)
	.long 0x40000000,0xadf85458,0xa2bb4a9b    | e
	.long 0x3fff0000,0xb8aa3b29,0x5c17f0bc    | log2(e)
	.long 0x3ffd0000,0xde5bd8a9,0x37287195    | log10(e)
	.long 0x00000000,0x00000000,0x00000000    | 0.0

|round to nearest
__x_BIGRN:
	.long 0x3ffe0000,0xb17217f7,0xd1cf79ac    | ln(2)
	.long 0x40000000,0x935d8ddd,0xaaa8ac17    | ln(10)
	.long 0x3fff0000,0x80000000,0x00000000    | 10 ^ 0

	.globl	__x_PTENRN
__x_PTENRN:
	.long 0x40020000,0xA0000000,0x00000000    | 10 ^ 1
	.long 0x40050000,0xC8000000,0x00000000    | 10 ^ 2
	.long 0x400C0000,0x9C400000,0x00000000    | 10 ^ 4
	.long 0x40190000,0xBEBC2000,0x00000000    | 10 ^ 8
	.long 0x40340000,0x8E1BC9BF,0x04000000    | 10 ^ 16
	.long 0x40690000,0x9DC5ADA8,0x2B70B59E    | 10 ^ 32
	.long 0x40D30000,0xC2781F49,0xFFCFA6D5    | 10 ^ 64
	.long 0x41A80000,0x93BA47C9,0x80E98CE0    | 10 ^ 128
	.long 0x43510000,0xAA7EEBFB,0x9DF9DE8E    | 10 ^ 256
	.long 0x46A30000,0xE319A0AE,0xA60E91C7    | 10 ^ 512
	.long 0x4D480000,0xC9767586,0x81750C17    | 10 ^ 1024
	.long 0x5A920000,0x9E8B3B5D,0xC53D5DE5    | 10 ^ 2048
	.long 0x75250000,0xC4605202,0x8A20979B    | 10 ^ 4096
|round to minus infinity
__x_BIGRZRM:
	.long 0x3ffe0000,0xb17217f7,0xd1cf79ab    | ln(2)
	.long 0x40000000,0x935d8ddd,0xaaa8ac16    | ln(10)
	.long 0x3fff0000,0x80000000,0x00000000    | 10 ^ 0

	.globl	__x_PTENRM
__x_PTENRM:
	.long 0x40020000,0xA0000000,0x00000000    | 10 ^ 1
	.long 0x40050000,0xC8000000,0x00000000    | 10 ^ 2
	.long 0x400C0000,0x9C400000,0x00000000    | 10 ^ 4
	.long 0x40190000,0xBEBC2000,0x00000000    | 10 ^ 8
	.long 0x40340000,0x8E1BC9BF,0x04000000    | 10 ^ 16
	.long 0x40690000,0x9DC5ADA8,0x2B70B59D    | 10 ^ 32
	.long 0x40D30000,0xC2781F49,0xFFCFA6D5    | 10 ^ 64
	.long 0x41A80000,0x93BA47C9,0x80E98CDF    | 10 ^ 128
	.long 0x43510000,0xAA7EEBFB,0x9DF9DE8D    | 10 ^ 256
	.long 0x46A30000,0xE319A0AE,0xA60E91C6    | 10 ^ 512
	.long 0x4D480000,0xC9767586,0x81750C17    | 10 ^ 1024
	.long 0x5A920000,0x9E8B3B5D,0xC53D5DE5    | 10 ^ 2048
	.long 0x75250000,0xC4605202,0x8A20979A    | 10 ^ 4096
|round to positive infinity
__x_BIGRP:
	.long 0x3ffe0000,0xb17217f7,0xd1cf79ac    | ln(2)
	.long 0x40000000,0x935d8ddd,0xaaa8ac17    | ln(10)
	.long 0x3fff0000,0x80000000,0x00000000    | 10 ^ 0

	.globl	__x_PTENRP
__x_PTENRP:
	.long 0x40020000,0xA0000000,0x00000000    | 10 ^ 1
	.long 0x40050000,0xC8000000,0x00000000    | 10 ^ 2
	.long 0x400C0000,0x9C400000,0x00000000    | 10 ^ 4
	.long 0x40190000,0xBEBC2000,0x00000000    | 10 ^ 8
	.long 0x40340000,0x8E1BC9BF,0x04000000    | 10 ^ 16
	.long 0x40690000,0x9DC5ADA8,0x2B70B59E    | 10 ^ 32
	.long 0x40D30000,0xC2781F49,0xFFCFA6D6    | 10 ^ 64
	.long 0x41A80000,0x93BA47C9,0x80E98CE0    | 10 ^ 128
	.long 0x43510000,0xAA7EEBFB,0x9DF9DE8E    | 10 ^ 256
	.long 0x46A30000,0xE319A0AE,0xA60E91C7    | 10 ^ 512
	.long 0x4D480000,0xC9767586,0x81750C18    | 10 ^ 1024
	.long 0x5A920000,0x9E8B3B5D,0xC53D5DE6    | 10 ^ 2048
	.long 0x75250000,0xC4605202,0x8A20979B    | 10 ^ 4096

|	xref	__x_nrm_zero
|	xref	__x_decbin
|	xref	__x_round

	.globl    __x_get_op
	.globl    __x_uns_getop
	.globl    __x_uni_getop

	.text

__x_get_op:
	clrb	a6@(DY_MO_FLG)
	tstb	a6@(UFLG_TMP)	| test flag for unsupp/unimp state
	jeq 	__x_uni_getop

__x_uns_getop:
	btst	#direction_bit,a6@(CMDREG1B)
	jne 	opclass3	| branch if a fmovel out (any kind)
	btst	#6,a6@(CMDREG1B)
	jeq 	uns_notpacked

	bfextu	a6@(CMDREG1B){#3:#3},d0
	cmpb	#3,d0
	jeq 	pack_source	| check for a packed src op, branch if so
uns_notpacked:
	bsrl	chk_dy_mo	| set the dyadic/monadic flag
	tstb	a6@(DY_MO_FLG)
	jeq 	src_op_ck	| if monadic, go check src op
|				| else, check dst op (fall through)

	btst	#7,a6@(DTAG)
	jeq 	src_op_ck	| if dst op is norm, check src op
	jra 	dst_ex_dnrm	| else, handle destination unnorm/dnrm

__x_uni_getop:
	bfextu	a6@(CMDREG1B){#0:#6},d0 | get opclass and src fields
	cmpil	#0x17,d0		| if op class and size fields are 0x17,
|				| it is FMOVECR|  if not, continue
|
| If the instruction is fmovecr, exit __x_get_op.  It is handled
| in __x_do_func and smovecrsa.
|
	jne 	not_fmovecr	| handle fmovecr as an unimplemented inst
	rts

not_fmovecr:
	btst	#E1,a6@(E_BYTE)	| if set, there is a packed operand
	jne 	pack_source	| check for packed src op, branch if so

| The following lines of are coded to optimize on normalized operands
	moveb	a6@(STAG),d0
	orb	a6@(DTAG),d0	| check if either of STAG/DTAG msb set
	jmi 	dest_op_ck	| if so, some op needs to be fixed
	rts

dest_op_ck:
	btst	#7,a6@(DTAG)	| check for unsupported data types in
	jeq 	src_op_ck	| the destination, if not, check src op
	bsrl	chk_dy_mo	| set dyadic/monadic flag
	tstb	a6@(DY_MO_FLG)	|
	jeq 	src_op_ck	| if monadic, check src op
|
| At this point, destination has an extended denorm or unnorm.
|
dst_ex_dnrm:
	movew	a6@(FPTEMP_EX),d0 | get destination exponent
	andiw	#0x7fff,d0	| mask sign, check if exp = 0000
	jeq 	src_op_ck	| if denorm then check source op.
|				| denorms are taken care of in __x_res_func
|				| (unsupp) or __x_do_func (unimp)
|				| else unnorm fall through
	lea	a6@(FPTEMP),a0	| point a0 to dop - used in mk_norm
	bsrl	mk_norm		| go normalize - mk_norm returns:
|				| L_SCR1{7:5} = operand tag
|				| 	(000 = norm, 100 = denorm)
|				| L_SCR1{4} = fpte15 or ete15
|				| 	0 = exp >  0x3fff
|				| 	1 = exp <= 0x3fff
|				| and puts the normalized num back
|				| on the fsave stack
|
	moveb	a6@(L_SCR1),a6@(DTAG) | write the new tag # fpte15
|				| to the fsave stack and fall
|				| through to check source operand
|
src_op_ck:
	btst	#7,a6@(STAG)
	jeq 	end_getop	| check for unsupported data types on the
|				| source operand
	btst	#5,a6@(STAG)
	jne 	src_sd_dnrm	| if bit 5 set, handle sgl/dbl denorms
|
| At this point only unnorms or extended denorms are possible.
|
src_ex_dnrm:
	movew	a6@(ETEMP_EX),d0 | get source exponent
	andiw	#0x7fff,d0	| mask sign, check if exp = 0000
	jeq 	end_getop	| if denorm then exit, denorms are
|				| handled in __x_do_func
	lea	a6@(ETEMP),a0	| point a0 to sop - used in mk_norm
	bsrl	mk_norm		| go normalize - mk_norm returns:
|				| L_SCR1{7:5} = operand tag
|				| 	(000 = norm, 100 = denorm)
|				| L_SCR1{4} = fpte15 or ete15
|				| 	0 = exp >  0x3fff
|				| 	1 = exp <= 0x3fff
|				| and puts the normalized num back
|				| on the fsave stack
|
	moveb	a6@(L_SCR1),a6@(STAG) | write the new tag # ete15
	rts			| end_getop

|
| At this point, only single or double denorms are possible.
| If the inst is not fmove, normalize the source.  If it is,
| do nothing to the input.
|
src_sd_dnrm:
	btst	#4,a6@(CMDREG1B)	| differentiate between sgl/dbl denorm
	jne 	is_double
is_single:
	movew	#0x3f81,d1	| write bias for sgl denorm
	jra 	common		| goto the common code
is_double:
	movew	#0x3c01,d1	| write the bias for a dbl denorm
common:
	btst	#sign_bit,a6@(ETEMP_EX) | grab sign bit of mantissa
	jeq 	pos
	bset	#15,d1		| set sign bit because it is negative
pos:
	movew	d1,a6@(ETEMP_EX)
|				| put exponent on stack

	movew	a6@(CMDREG1B),d1
	andw	#0xe3ff,d1	| clear out source specifier
	orw	#0x0800,d1	| set source specifier to extended prec
	movew	d1,a6@(CMDREG1B)	| write back to the command word in stack
|				| this is needed to fix unsupp data stack
	lea	a6@(ETEMP),a0	| point a0 to sop

	bsrl	mk_norm		| convert sgl/dbl denorm to norm
	moveb	a6@(L_SCR1),a6@(STAG) | put tag into source tag reg - d0
	rts			| end_getop
|
| At this point, the source is definitely packed, whether
| instruction is dyadic or monadic is still unknown
|
pack_source:
	movel	a6@(FPTEMP_LO),a6@(ETEMP)	| write ms part of packed
|				| number to etemp slot
	bsrl	chk_dy_mo	| set dyadic/monadic flag
	bsrl	unpack

	tstb	a6@(DY_MO_FLG)
	jeq 	end_getop	| if monadic, exit
|				| else, fix FPTEMP
pack_dya:
	bfextu	a6@(CMDREG1B){#6:#3},d0 | extract dest fp reg
	movel	#7,d1
	subl	d0,d1
	clrl	d0
	bset	d1,d0		| set up d0 as a dynamic register mask
	fmovemx d0,a6@(FPTEMP)	| write to FPTEMP

	btst	#7,a6@(DTAG)	| check dest tag for unnorm or denorm
	jne 	dst_ex_dnrm	| else, handle the unnorm or ext denorm
|
| Dest is not denormalized.  Check for norm, and set fpte15
| accordingly.
|
	moveb	a6@(DTAG),d0
	andib	#0xf0,d0		| strip to only dtag:fpte15
	tstb	d0		| check for normalized value
	jne 	end_getop	| if inf/nan/zero leave __x_get_op
	movew	a6@(FPTEMP_EX),d0
	andiw	#0x7fff,d0
	cmpiw	#0x3fff,d0	| check if fpte15 needs setting
	jge 	end_getop	| if >= 0x3fff, leave fpte15=0
	orb	#0x10,a6@(DTAG)
	jra 	end_getop

|
| At this point, it is either an fmoveout packed, unnorm or denorm
|
opclass3:
	clrb	a6@(DY_MO_FLG)	| set dyadic/monadic flag to monadic
	bfextu	a6@(CMDREG1B){#4:#2},d0
	cmpib	#3,d0
	jne 	src_ex_dnrm	| if not equal, must be unnorm or denorm
|				| else it is a packed move out
|				| exit
end_getop:
	rts

|
| Sets the DY_MO_FLG correctly. This is used only on if it is an
| unuspported data type exception.  Set if dyadic.
|
chk_dy_mo:
	movew	a6@(CMDREG1B),d0
	btst	#5,d0		| testing extension command word
	jeq 	set_mon		| if bit 5 = 0 then monadic
	btst	#4,d0		| know that bit 5 = 1
	jeq 	set_dya		| if bit 4 = 0 then dyadic
	andiw	#0x007f,d0	| get rid of all but extension bits {6:0}
	cmpiw 	#0x0038,d0	| if extension = 0x38 then fcmp (dyadic)
	jne 	set_mon
set_dya:
	st	a6@(DY_MO_FLG)	| set the inst flag type to dyadic
	rts
set_mon:
	clrb	a6@(DY_MO_FLG)	| set the inst flag type to monadic
	rts
|
|	MK_NORM
|
| Normalizes unnormalized numbers, sets tag to norm or denorm, sets unfl
| exception if denorm.
|
| CASE opclass 0x0 unsupp
|	mk_norm till msb set
|	set tag = norm
|
| CASE opclass 0x0 unimp
|	mk_norm till msb set or exp = 0
|	if integer bit = 0
|	   tag = denorm
|	else
|	   tag = norm
|
| CASE opclass 011 unsupp
|	mk_norm till msb set or exp = 0
|	if integer bit = 0
|	   tag = denorm
|	   set unfl_nmcexe = 1
|	else
|	   tag = norm
|
| if exp <= 0x3fff
|   set ete15 or fpte15 = 1
| else set ete15 or fpte15 = 0

| input:
|	a0 = points to operand to be normalized
| output:
|	L_SCR1{7:5} = operand tag (000 = norm, 100 = denorm)
|	L_SCR1{4}   = fpte15 or ete15 (0 = exp > 0x3fff, 1 = exp <=0x3fff)
|	the normalized operand is placed back on the fsave stack
mk_norm:
	clrl	a6@(L_SCR1)
	bclr	#sign_bit,a0@(LOCAL_EX)
	sne	a0@(LOCAL_SGN)	| transform into internal extended format

	cmpib	#0x2c,a6@(1+EXC_VEC) | check if unimp
	jne 	uns_data	| branch if unsupp
	bsrl	uni_inst	| call if unimp (opclass 0x0)
	jra 	reload
uns_data:
	btst	#direction_bit,a6@(CMDREG1B) | check transfer direction
	jne 	bit_set		| branch if set (opclass 011)
	bsrl	uns_opx		| call if opclass 0x0
	jra 	reload
bit_set:
	bsrl	uns_op3		| opclass 011
reload:
	cmpw	#0x3fff,a0@(LOCAL_EX) | if exp > 0x3fff
	jgt 	end_mk		|    fpte15/ete15 already set to 0
	bset	#4,a6@(L_SCR1)	| else set fpte15/ete15 to 1
|				| calling routine actually sets the
|				| value on the stack (along with the
/* |				| tag), since this routine doesn't  */
|				| know if it should set ete15 or fpte15
/* |				| ie, it doesn't know if this is the  */
|				| src op or dest op.
end_mk:
	bfclr	a0@(LOCAL_SGN){#0:#8}
	jeq 	end_mk_pos
	bset	#sign_bit,a0@(LOCAL_EX) | convert back to IEEE format
end_mk_pos:
	rts
|
|     CASE opclass 011 unsupp
|
uns_op3:
	bsrl	__x_nrm_zero	| normalize till msb = 1 or exp = zero
	btst	#7,a0@(LOCAL_HI)	| if msb = 1
	jne 	no_unfl		| then branch
set_unfl:
	orw	#dnrm_tag,a6@(L_SCR1) | set denorm tag
	bset	#__x_unfl_bit,a6@(FPSR_EXCEPT) | set unfl exception bit
no_unfl:
	rts
|
|     CASE opclass 0x0 unsupp
|
uns_opx:
	bsrl	__x_nrm_zero	| normalize the number
	btst	#7,a0@(LOCAL_HI)	| check if integer bit (j-bit) is set
	jeq 	uns_den		| if clear then now have a denorm
uns_nrm:
	orb	#__x_norm_tag,a6@(L_SCR1) | set tag to norm
	rts
uns_den:
	orb	#dnrm_tag,a6@(L_SCR1) | set tag to denorm
	rts
|
|     CASE opclass 0x0 unimp
|
uni_inst:
	bsrl	__x_nrm_zero
	btst	#7,a0@(LOCAL_HI)	| check if integer bit (j-bit) is set
	jeq 	uni_den		| if clear then now have a denorm
uni_nrm:
	orb	#__x_norm_tag,a6@(L_SCR1) | set tag to norm
	rts
uni_den:
	orb	#dnrm_tag,a6@(L_SCR1) | set tag to denorm
	rts

|
|	Decimal to binary conversion
|
| Special cases of inf and NaNs are completed outside of __x_decbin.
| If the input is an snan, the snan bit is not set.
|
| input:
|	a6@(ETEMP)	- points to packed decimal string in memory
| output:
|	fp0	- contains packed string converted to extended precision
|	ETEMP	- same as fp0
unpack:
	movew	a6@(CMDREG1B),d0	/* | examine command word, looking for fmove's */
	andw	#0x3b,d0
	jeq 	move_unpack	| special handling for fmove: must set FPSR_CC

	movew	a6@(ETEMP),d0	| get word with inf information
	bfextu	d0{#20:#12},d1	| get exponent into d1
	cmpiw	#0x0fff,d1	| test for inf or NaN
	jne 	try_zero	| if not equal, it is not special
	bfextu	d0{#17:#3},d1	| get SE and y bits into d1
	cmpiw	#7,d1		| SE and y bits must be on for special
	jne 	try_zero	| if not on, it is not special
|input is of the special cases of inf and NaN
	tstl	a6@(ETEMP_HI)	| check ms mantissa
	jne 	fix_nan		| if non-zero, it is a NaN
	tstl	a6@(ETEMP_LO)	| check ls mantissa
	jne 	fix_nan		| if non-zero, it is a NaN
	jra 	finish		| special already on stack
fix_nan:
	btst	#signan_bit,a6@(ETEMP_HI) | test for snan
	jne 	finish
	orl	#__x_snaniop_mask,a6@(USER_FPSR) | always set snan if it is so
	jra 	finish
try_zero:
	movew	a6@(ETEMP_EX+2),d0 | get word 4
	andiw	#0x000f,d0	| clear all but last ni(y)bble
	tstw	d0		| check for zero.
	jne 	not_spec
	tstl	a6@(ETEMP_HI)	| check words 3 and 2
	jne 	not_spec
	tstl	a6@(ETEMP_LO)	| check words 1 and 0
	jne 	not_spec
	tstl	a6@(ETEMP)	| test sign of the zero
	jge 	pos_zero
	movel	#0x80000000,a6@(ETEMP) | write neg zero to etemp
	clrl	a6@(ETEMP_HI)
	clrl	a6@(ETEMP_LO)
	jra 	finish
pos_zero:
	clrl	a6@(ETEMP)
	clrl	a6@(ETEMP_HI)
	clrl	a6@(ETEMP_LO)
	jra 	finish

not_spec:
	fmovemx fp0-fp1,a7@-	| save fp0 - __x_decbin returns in it
	bsrl	__x_decbin
	fmovex fp0,a6@(ETEMP)	| put the unpacked sop in the fsave stack
	fmovemx	a7@+,fp0-fp1
	fmovel	#0,FPSR		| clr fpsr from __x_decbin
	jra 	finish

|
| Special handling for packed move in:  Same results as all other
| packed cases, but we must set the FPSR condition codes properly.
|
move_unpack:
	movew	a6@(ETEMP),d0	| get word with inf information
	bfextu	d0{#20:#12},d1	| get exponent into d1
	cmpiw	#0x0fff,d1	| test for inf or NaN
	jne 	mtry_zero	| if not equal, it is not special
	bfextu	d0{#17:#3},d1	| get SE and y bits into d1
	cmpiw	#7,d1		| SE and y bits must be on for special
	jne 	mtry_zero	| if not on, it is not special
|input is of the special cases of inf and NaN
	tstl	a6@(ETEMP_HI)	| check ms mantissa
	jne 	mfix_nan		| if non-zero, it is a NaN
	tstl	a6@(ETEMP_LO)	| check ls mantissa
	jne 	mfix_nan		| if non-zero, it is a NaN
|input is inf
	orl	#inf_mask,a6@(USER_FPSR) | set I bit
	tstl	a6@(ETEMP)	| check sign
	jge 	finish
	orl	#neg_mask,a6@(USER_FPSR) | set N bit
	jra 	finish		| special already on stack
mfix_nan:
	orl	#nan_mask,a6@(USER_FPSR) | set NaN bit
	moveb	#nan_tag,a6@(STAG)	| set stag to NaN
	btst	#signan_bit,a6@(ETEMP_HI) | test for snan
	jne 	mn_snan
	orl	#__x_snaniop_mask,a6@(USER_FPSR) | set snan bit
	btst	#__x_snan_bit,a6@(fpcr_ENABLE) | test for snan enabled
	jne 	mn_snan
	bset	#signan_bit,a6@(ETEMP_HI) | force snans to qnans
mn_snan:
	tstl	a6@(ETEMP)	| check for sign
	jge 	finish		| if clr, go on
	orl	#neg_mask,a6@(USER_FPSR) | set N bit
	jra 	finish

mtry_zero:
	movew	a6@(ETEMP_EX+2),d0 | get word 4
	andiw	#0x000f,d0	| clear all but last ni(y)bble
	tstw	d0		| check for zero.
	jne 	mnot_spec
	tstl	a6@(ETEMP_HI)	| check words 3 and 2
	jne 	mnot_spec
	tstl	a6@(ETEMP_LO)	| check words 1 and 0
	jne 	mnot_spec
	tstl	a6@(ETEMP)	| test sign of the zero
	jge 	mpos_zero
	orl	#neg_mask+z_mask,a6@(USER_FPSR) | set N and Z
	movel	#0x80000000,a6@(ETEMP) | write neg zero to etemp
	clrl	a6@(ETEMP_HI)
	clrl	a6@(ETEMP_LO)
	jra 	finish
mpos_zero:
	orl	#z_mask,a6@(USER_FPSR) | set Z
	clrl	a6@(ETEMP)
	clrl	a6@(ETEMP_HI)
	clrl	a6@(ETEMP_LO)
	jra 	finish

mnot_spec:
	fmovemx fp0-fp1,a7@-	| save fp0 - __x_decbin returns in it
	bsrl	__x_decbin
	fmovex fp0,a6@(ETEMP)
|				| put the unpacked sop in the fsave stack
	fmovemx	a7@+,fp0-fp1

finish:
	movew	a6@(CMDREG1B),d0	| get the command word
	andw	#0xfbff,d0	| change the source specifier field to
|				| extended (was packed).
	movew	d0,a6@(CMDREG1B)	| write command word back to fsave stack
|				| we need to do this so the 040 will
|				| re-execute the inst. without taking
|				| another packed trap.

fix_stag:
|Converted result is now in etemp on fsave stack, now set the source
|tag (stag)
|	if (ete =0x7fff) then INF or NAN
|		if (etemp = $x.0----0) then
|			stag = INF
|		else
|			stag = NAN
|	else
|		if (ete = 0x0000) then
|			stag = ZERO
|		else
|			stag = NORM
|
| Note also that the etemp_15 bit (just right of the stag) must
| be set accordingly.
|
	movew		a6@(ETEMP_EX),d1
	andiw		#0x7fff,d1   | strip sign
	cmpw  		#0x7fff,d1
	jne   		z_or_nrm
	movel		a6@(ETEMP_HI),d1
	jne 		is_nan
	movel		a6@(ETEMP_LO),d1
	jne 		is_nan
is_inf:
	moveb		#0x40,a6@(STAG)
	movel		#0x40,d0
	rts
is_nan:
	moveb		#0x60,a6@(STAG)
	movel		#0x60,d0
	rts
z_or_nrm:
	tstw		d1
	jne 		is_nrm
is_zro:
| For a zero, set etemp_15
	moveb		#0x30,a6@(STAG)
	movel		#0x20,d0
	rts
is_nrm:
| For a norm, check if the exp <= 0x3fff|  if so, set etemp_15
	cmpiw		#0x3fff,d1
	jle 		set_bit15
	moveb		#0,a6@(STAG)
	jra 		end_is_nrm
set_bit15:
	moveb		#0x10,a6@(STAG)
end_is_nrm:
	movel		#0,d0
end_fix:
	rts

end_get:
	rts
|	end
