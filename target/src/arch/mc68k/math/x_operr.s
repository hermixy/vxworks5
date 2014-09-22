/* x_operr.s - Motorola 68040 FP operand error exception handler (EXC) */

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


	x_operrsa 3.4 4/26/91

	__x_fpsp_operr --- FPSP handler for operand error exception

	See 68040 User's Manual pp. 9-44f

 Note 1: For trap disabled 040 does the following:
 If the dest is a fp reg, then an extended precision non_signaling
 NAN is stored in the dest reg.  If the dest format is b, w, or l and
 the source op is a NAN, then garbage is stored as the result (actually
 the upper 32 bits of the mantissa are sent to the integer unit). If
 the dest format is integer (b, w, l) and the operr is caused by
 integer overflow, or the source op is inf, then the result stored is
 garbage.
 There are three cases in which operr is incorrectly signaled on the
 040.  This occurs for move_out of format b, w, or l for the largest
 negative integer (-2^7 for b, -2^15 for w, -2^31 for l).

	  On opclass = 011 fmove.(b,w,l) that causes a conversion
	  overflow -> OPERR, the exponent in wbte (and fpte) is:
		.byte    56 - (62 - exp)
		.word    48 - (62 - exp)
		.long    32 - (62 - exp)

			where exp = (true exp) - 1

  So, wbtemp and fptemp will contain the following on erroneoulsy
	  signalled operr:
			fpts = 1
			fpte = 0x4000  (15 bit externally)
		.byte	fptm = 0xffffffff ffffff80
		.word	fptm = 0xffffffff ffff8000
		.long	fptm = 0xffffffff 80000000

 Note 2: For trap enabled 040 does the following:
 If the inst is move_out, then same as Note 1.
 If the inst is not move_out, the dest is not modified.
 The exceptional operand is not defined for integer overflow
 during a move_out.


		Copyright (C) Motorola, Inc. 1990
			All Rights Reserved

	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA
	The copyright notice above does not evidence any
	actual or intended publication of such source code.

X_OPERR	idnt    2,1 Motorola 040 Floating Point Software Package

	section	8

NOMANUAL
*/

#include "fpsp040E.h"

|	xref	__x_mem_write
|	xref	__x_real_operr
|	xref	__x_real_inex
|	xref	__x_get_fline
|	xref	__x_fpsp_done
|	xref	__x_reg_dest
|	xref 	__x_check_force


	.text

	.globl	__x_fpsp_operr
__x_fpsp_operr:
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
| Check if this is an opclass 3 instruction.
|  If so, fall through, else branch to __x_operr_end
|
	btst	#TFLAG,a6@(T_BYTE)
	jeq 	__x_operr_end

|
| If the destination size is B,W,or L, the operr must be
| handled here.
|
	movel	a6@(CMDREG1B),d0
	bfextu	d0{#3:#3},d0	| 0=long, 4=word, 6=byte
	cmpib	#0,d0		| determine size|  check long
	jeq 	__x_operr_long
	cmpib	#4,d0		| check word
	jeq 	__x_operr_word
	cmpib	#6,d0		| check byte
	jeq 	__x_operr_byte

|
| The size is not B,W,or L, so the operr is handled by the
| kernel handler.  Set the operr bits and clean up, leaving
| only the integer exception frame on the stack, and the
| fpu in the original exceptional state.
|
__x_operr_end:
	bset		#__x_operr_bit,a6@(FPSR_EXCEPT)
	bset		#aiop_bit,a6@(FPSR_AEXCEPT)

	moveml		a6@(USER_DA),d0-d1/a0-a1
	fmovemx		a6@(USER_FP0),fp0-fp3
	fmoveml		a6@(USER_FPCR),fpcr/fpsr/fpi
	frestore	a7@+
	unlk		a6
	jra 		__x_real_operr

__x_operr_long:
	moveql	#4,d1			| write size to d1
	moveb	a6@(STAG),d0		| test stag for nan
	andib	#0xe0,d0		| clr all but tag
	cmpib	#0x60,d0		| check for nan
	jeq 	__x_operr_nan
	cmpil	#0x80000000,a6@(FPTEMP_LO) | test if ls lword is special
	jne 	chklerr			| if not equal, chk for incorrect operr
	bsrl	check_upper		| check if exp and ms mant are special
	tstl	d0
	jne 	chklerr			| if d0 is true, chk for incorrect operr
	movel	#0x80000000,d0		| store special case result
	bsrl	__x_operr_store
	jra 	not_enabled		| clean and exit
|
|	CHECK FOR INCORRECTLY GENERATED OPERR EXCEPTION HERE
|
chklerr:
	movew	a6@(FPTEMP_EX),d0
	andw	#0x7FFF,d0	| ignore sign bit
	cmpw	#0x3FFE,d0	| this is the only possible exponent value
	jne 	chklerr2
fixlong:
	movel	a6@(FPTEMP_LO),d0
	bsrl	__x_operr_store
	jra 	not_enabled
chklerr2:
	movew	a6@(FPTEMP_EX),d0
	andw	#0x7FFF,d0	| ignore sign bit
	cmpw	#0x4000,d0
	jhi 	__x_store_max	| exponent out of range

	movel	a6@(FPTEMP_LO),d0
	andl	#0x7FFF0000,d0	/* | look for all 1's on bits 30-16 */
	cmpl	#0x7FFF0000,d0
	jeq 	fixlong

	tstl	a6@(FPTEMP_LO)
	jpl 	chklepos
	cmpl	#0xFFFFFFFF,a6@(FPTEMP_HI)
	jeq 	fixlong
	jra 	__x_store_max
chklepos:
	tstl	a6@(FPTEMP_HI)
	jeq 	fixlong
	jra 	__x_store_max

__x_operr_word:
	moveql	#2,d1			| write size to d1
	moveb	a6@(STAG),d0		| test stag for nan
	andib	#0xe0,d0		| clr all but tag
	cmpib	#0x60,d0		| check for nan
	jeq 	__x_operr_nan
	cmpil	#0xffff8000,a6@(FPTEMP_LO) | test if ls lword is special
	jne 	chkwerr		| if not equal, check for incorrect operr
	bsrl	check_upper	| check if exp and ms mant are special
	tstl	d0
	jne 	chkwerr		| if d0 is true, check for incorrect operr
	movel	#0x80000000,d0	| store special case result
	bsrl	__x_operr_store
	jra 	not_enabled	| clean and exit
|
|	CHECK FOR INCORRECTLY GENERATED OPERR EXCEPTION HERE
|
chkwerr:
	movew	a6@(FPTEMP_EX),d0
	andw	#0x7FFF,d0	| ignore sign bit
	cmpw	#0x3FFE,d0	| this is the only possible exponent value
	jne 	__x_store_max
	movel	a6@(FPTEMP_LO),d0
	swap	d0
	bsrl	__x_operr_store
	jra 	not_enabled

__x_operr_byte:
	moveql	#1,d1			| write size to d1
	moveb	a6@(STAG),d0		| test stag for nan
	andib	#0xe0,d0		| clr all but tag
	cmpib	#0x60,d0		| check for nan
	jeq 	__x_operr_nan
	cmpil	#0xffffff80,a6@(FPTEMP_LO) | test if ls lword is special
	jne 	chkberr		| if not equal, check for incorrect operr
	bsrl	check_upper	| check if exp and ms mant are special
	tstl	d0
	jne 	chkberr		| if d0 is true, check for incorrect operr
	movel	#0x80000000,d0	| store special case result
	bsrl	__x_operr_store
	jra 	not_enabled	| clean and exit
|
|	CHECK FOR INCORRECTLY GENERATED OPERR EXCEPTION HERE
|
chkberr:
	movew	a6@(FPTEMP_EX),d0
	andw	#0x7FFF,d0	| ignore sign bit
	cmpw	#0x3FFE,d0	| this is the only possible exponent value
	jne 	__x_store_max
	movel	a6@(FPTEMP_LO),d0
	asll	#8,d0
	swap	d0
	bsrl	__x_operr_store
	jra 	not_enabled

|
| This operr condition is not of the special case.  Set operr
| and aiop and write the portion of the nan to memory for the
| given size.
|
__x_operr_nan:
	orl	#opaop_mask,a6@(USER_FPSR) | set operr # aiop

	movel	a6@(ETEMP_HI),d0	| output will be from upper 32 bits
	bsrl	__x_operr_store
	jra 	end_operr
|
| Store_max loads the max pos or negative for the size, sets
| the operr and aiop bits, and clears inex and ainex, incorrectly
| set by the 040.
|
__x_store_max:
	orl	#opaop_mask,a6@(USER_FPSR) | set operr # aiop
	bclr	#__x_inex2_bit,a6@(FPSR_EXCEPT)
	bclr	#ainex_bit,a6@(FPSR_AEXCEPT)
	fmovel	#0,FPSR

	tstw	a6@(FPTEMP_EX)		| check sign
	jlt 	load_neg
	movel	#0x7fffffff,d0
	bsrl	__x_operr_store
	jra 	end_operr
load_neg:
	movel	#0x80000000,d0
	bsrl	__x_operr_store
	jra 	end_operr

|
| This routine stores the data in d0, for the given size in d1,
| to memory or data register as required.  A read of the fline
| is required to determine the destination.
|
__x_operr_store:
	movel	d0,a6@(L_SCR1)	| move write data to L_SCR1
	movel	d1,a7@-		| save register size
	bsrl	__x_get_fline	| fline returned in d0
	movel	a7@+,d1
	bftst	d0{#26:#3}	| if mode is zero, dest is Dn
	jne 	dest_mem
|
| Destination is Dn.  Get register number from d0. Data is on
| the stack at	a7@. D1 has size: 1=byte,2=word,4=long/single
|
	andil	#7,d0		| isolate register number
	cmpil	#4,d1
	jeq 	op_long		| the most frequent case
	cmpil	#2,d1
	jne 	op_con
	orl	#8,d0
	jra 	op_con
op_long:
	orl	#0x10,d0
op_con:
	movel	d0,d1		| format size:reg for __x_reg_dest
	jra 	__x_reg_dest	| call to __x_reg_dest returns to caller
|				| of __x_operr_store
|
| Destination is memory.  Get <ea> from integer exception frame
| and call __x_mem_write.
|
dest_mem:
	lea	a6@(L_SCR1),a0	| put ptr to write data in a0
	movel	a6@(EXC_EA),a1	| put user destination address in a1
	movel	d1,d0		| put size in d0
	bsrl	__x_mem_write
	rts
|
| Check the exponent for 0xc000 and the upper 32 bits of the
| mantissa for 0xffffffff.  If both are true, return d0 clr
| and store the lower n bits of the least lword of FPTEMP
| to d0 for write out.  If not, it is a real operr, and set d0.
|
check_upper:
	cmpil	#0xffffffff,a6@(FPTEMP_HI) /* | check if first byte is all 1's */
	jne 	true_operr		/* if not all 1's then was true operr */
	cmpiw	#0xc000,a6@(FPTEMP_EX) 	| check if incorrectly signalled
	jeq 	not_true_operr		| branch if not true operr
	cmpiw	#0xbfff,a6@(FPTEMP_EX) 	| check if incorrectly signalled
	jeq 	not_true_operr		| branch if not true operr
true_operr:
	movel	#1,d0			| signal real operr
	rts
not_true_operr:
	clrl	d0			| signal no real operr
	rts

|
| End_operr tests for operr enabled.  If not, it cleans up the stack
| and does an rte.  If enabled, it cleans up the stack and branches
| to the kernel operr handler with only the integer exception
| frame on the stack and the fpu in the original exceptional state
| with correct data written to the destination.
|
end_operr:
	btst		#__x_operr_bit,a6@(fpcr_ENABLE)
	jeq 		not_enabled
enabled:
	moveml		a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP0),fp0-fp3
	fmoveml	a6@(USER_FPCR),fpcr/fpsr/fpi
	frestore	a7@+
	unlk		a6
	jra 		__x_real_operr

not_enabled:
|
| It is possible to have either inex2 or inex1 exceptions with the
| operr.  If the inex enable bit is set in the fpcr, and either
| inex2 or inex1 occured, we must clean up and branch to the
| real inex handler.
|
ck_inex:
	moveb	a6@(fpcr_ENABLE),d0
	andb	a6@(FPSR_EXCEPT),d0
	andib	#0x3,d0
	jeq 	__x_operr_exit
|
| Inexact enabled and reported, and we must take an inexact exception.
|
take_inex:
	moveb		#INEX_VEC,a6@(EXC_VEC+1)
	movel		a6@(USER_FPSR),a6@(FPSR_SHADOW)
	orl		#sx_mask,a6@(E_BYTE)
	moveml		a6@(USER_DA),d0-d1/a0-a1
	fmovemx		a6@(USER_FP0),fp0-fp3
	fmoveml		a6@(USER_FPCR),fpcr/fpsr/fpi
	frestore	a7@+
	unlk		a6
	jra 		__x_real_inex
|
| Since operr is only an E1 exception, there is no need to frestore
| any state back to the fpu.
|
__x_operr_exit:
	moveml		a6@(USER_DA),d0-d1/a0-a1
	fmovemx		a6@(USER_FP0),fp0-fp3
	fmoveml		a6@(USER_FPCR),fpcr/fpsr/fpi
	unlk		a6
	jra 		__x_fpsp_done

|	end
