/* x_snan.s - Motorola 68040 FP signalling NAN exception handler (EXC) */

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

	x_snansa 3.2 4/26/91

 fpsp_snan --- FPSP handler for signalling NAN exception

 SNAN for float -> integer conversions (integer conversion of
 an SNAN) is a non-maskable run-time exception.

 For trap disabled the 040 does the following:
 If the dest data format is s, d, or x, then the SNAN bit in the NAN
 is set to one and the resulting non-signaling NAN (truncated if
 necessary) is transferred to the dest.  If the dest format is b, w,
 or l, then garbage is written to the dest (actually the upper 32 bits
 of the mantissa are sent to the integer unit).

 For trap enabled the 040 does the following:
 If the inst is move_out, then the results are the same as for trap
 disabled with the exception posted.  If the instruction is not move_
 out, the dest. is not modified, and the exception is posted.


		Copyright (C) Motorola, Inc. 1990
			All Rights Reserved

	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA
	The copyright notice above does not evidence any
	actual or intended publication of such source code.

X_SNAN	idnt    2,1 Motorola 040 Floating Point Software Package

	section	8

NOMANUAL
*/

#include "fpsp040E.h"

|	xref	__x_get_fline
|	xref	__x_mem_write
|	xref	__x_real_snan
|	xref	__x_real_inex
|	xref	__x_fpsp_done
|	xref	__x_reg_dest
|	xref	__x_check_force


	.text

	.globl	__x_fpsp_snan
__x_fpsp_snan:
	link	a6,#-LOCAL_SIZE
	fsave	a7@-
	moveml	d0-d1/a0-a1,a6@(USER_DA)
	fmovemx	fp0-fp3,a6@(USER_FP0)
	fmoveml	fpcr/fpsr/fpi,a6@(USER_FPCR)

| At this point we need to look at the instructions and see if it is one of
| the force-precision ones (fsadd,fdadd,fssub,fdsub,fsmul,fdmul,fsdiv,fddiv,
| fssqrt,fdsqrt,fsmove,fdmove,fsabs,fdabs,fsneg,fdneg).  If it is then
| correct the USER_FPCR to the instruction rounding precision (s or d).
| Also, we need to check if the instruction is fsgldiv or fsglmul.  If it
| is then the USER_FPCR is set to extended rounding precision.  Otherwise
| leave the USER_FPCR alone.
|	bsrl		__x_check_force
|
| Check if trap enabled
|
	btst	#__x_snan_bit,a6@(fpcr_ENABLE)
	jne 	ena		| If enabled, then branch

	bsrl	move_out	| else SNAN disabled
|
| It is possible to have an inex1 exception with the
| snan.  If the inex enable bit is set in the fpcr, and either
| inex2 or inex1 occured, we must clean up and branch to the
| real inex handler.
|
ck_inex:
	moveb	a6@(fpcr_ENABLE),d0
	andb	a6@(FPSR_EXCEPT),d0
	andib	#0x3,d0
	jeq 	end_snan
|
| Inexact enabled and reported, and we must take an inexact exception.
|
take_inex:
	moveb	#INEX_VEC,a6@(EXC_VEC+1)
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP0),fp0-fp3
	fmoveml	a6@(USER_FPCR),fpcr/fpsr/fpi
	frestore a7@+
	unlk	a6
	jra 	__x_real_inex
|
| SNAN is enabled.  Check if inst is move_out.
| Make any corrections to the 040 output as necessary.
|
ena:
	btst	#5,a6@(CMDREG1B) | if set, inst is move out
	jeq 	not_out

	bsrl	move_out

report_snan:
	moveb	a7@,a6@(VER_TMP)
	cmpib	#VER_40,a7@	| test for orig unimp frame
	jne 	ck_rev
	moveql	#13,d0		| need to zero 14 lwords
	jra 	rep_con
ck_rev:
	moveql	#11,d0		| need to zero 12 lwords
rep_con:
	clrl	a7@
loop1:
	clrl	a7@-		| clear and dec a7
	dbra	d0,loop1
	moveb	a6@(VER_TMP),a7@ | format a busy frame
	moveb	#BUSY_SIZE-4,a7@(1)
	movel	a6@(USER_FPSR),a6@(FPSR_SHADOW)
	orl	#sx_mask,a6@(E_BYTE)
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP0),fp0-fp3
	fmoveml	a6@(USER_FPCR),fpcr/fpsr/fpi
	frestore a7@+
	unlk	a6
	jra 	__x_real_snan
|
| Exit snan handler by expanding the unimp frame into a busy frame
|
end_snan:
	bclr	#E1,a6@(E_BYTE)

	moveb	a7@,a6@(VER_TMP)
	cmpib	#VER_40,a7@	| test for orig unimp frame
	jne 	ck_rev2
	moveql	#13,d0		| need to zero 14 lwords
	jra 	rep_con2
ck_rev2:
	moveql	#11,d0		| need to zero 12 lwords
rep_con2:
	clrl	a7@
loop2:
	clrl	a7@-		| clear and dec a7
	dbra	d0,loop2
	moveb	a6@(VER_TMP),a7@ | format a busy frame
	moveb	#BUSY_SIZE-4,a7@(1) | write busy size
	movel	a6@(USER_FPSR),a6@(FPSR_SHADOW)
	orl	#sx_mask,a6@(E_BYTE)
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP0),fp0-fp3
	fmoveml	a6@(USER_FPCR),fpcr/fpsr/fpi
	frestore a7@+
	unlk	a6
	jra 	__x_fpsp_done

|
| Move_out
|
move_out:
	movel	a6@(EXC_EA),a0	| get <ea> from exc frame

	bfextu	a6@(CMDREG1B){#3:#3},d0 | move rx field to d0{2:0}
	cmpil	#0,d0		| check for long
	jeq 	sto_long	| branch if move_out long

	cmpil	#4,d0		| check for word
	jeq 	sto_word	| branch if move_out word

	cmpil	#6,d0		| check for byte
	jeq 	sto_byte	| branch if move_out byte

|
| Not byte, word or long
|
	rts
|
| Get the 32 most significant bits of etemp mantissa
|
sto_long:
	movel	a6@(ETEMP_HI),d1
	movel	#4,d0		| load byte count
|
| Set signalling nan bit
|
	bset	#30,d1
|
| Store to the users destination address
|
	tstl	a0		| check if <ea> is 0
	jeq 	wrt_dn		| destination is a data register

	movel	d1,a7@-	| move the snan onto the stack
	movel	a0,a1		| load dest addr into a1
	movel	a7,a0		| load src addr of snan into a0
	bsrl	__x_mem_write	| write snan to user memory
	movel	a7@+,d1	| clear off stack
	rts
|
| Get the 16 most significant bits of etemp mantissa
|
sto_word:
	movel	a6@(ETEMP_HI),d1
	movel	#2,d0		| load byte count
|
| Set signalling nan bit
|
	bset	#30,d1
|
| Store to the users destination address
|
	tstl	a0		| check if <ea> is 0
	jeq 	wrt_dn		| destination is a data register

	movel	d1,a7@-	| move the snan onto the stack
	movel	a0,a1		| load dest addr into a1
	movel	a7,a0		| point to low word
	bsrl	__x_mem_write	| write snan to user memory
	movel	a7@+,d1	| clear off stack
	rts
|
| Get the 8 most significant bits of etemp mantissa
|
sto_byte:
	movel	a6@(ETEMP_HI),d1
	movel	#1,d0		| load byte count
|
| Set signalling nan bit
|
	bset	#30,d1
|
| Store to the users destination address
|
	tstl	a0		| check if <ea> is 0
	jeq 	wrt_dn		| destination is a data register
	movel	d1,a7@-	| move the snan onto the stack
	movel	a0,a1		| load dest addr into a1
	movel	a7,a0		| point to source byte
	bsrl	__x_mem_write	| write snan to user memory
	movel	a7@+,d1	| clear off stack
	rts

|
|	wrt_dn --- write to a data register
|
|	We get here with D1 containing the data to write and D0 the
|	number of bytes to write: 1=byte,2=word,4=long.
|
wrt_dn:
	movel	d1,a6@(L_SCR1)	| data
	movel	d0,a7@-	| size
	bsrl	__x_get_fline	| returns fline word in d0
	movel	d0,d1
	andil	#0x7,d1		| d1 now holds register number
	movel	a7@+,d0	| get original size
	cmpil	#4,d0
	jeq 	wrt_long
	cmpil	#2,d0
	jne 	wrt_byte
wrt_word:
	orl	#0x8,d1
	jra 	__x_reg_dest
wrt_long:
	orl	#0x10,d1
	jra 	__x_reg_dest
wrt_byte:
	jra 	__x_reg_dest
|
| Check if it is a src nan or dst nan
|
not_out:
	movel	a6@(DTAG),d0
	bfextu	d0{#0:#3},d0	| isolate dtag in lsbs

	cmpib	#3,d0		| check for nan in destination
	jne 	issrc		| destination nan has priority
__x_dst_nan:
	btst	#6,a6@(FPTEMP_HI) | check if dest nan is an snan
	jne 	issrc		| no, so check source for snan
	movew	a6@(FPTEMP_EX),d0
	jra 	cont
issrc:
	movew	a6@(ETEMP_EX),d0
cont:
	btst	#15,d0		| test for sign of snan
	jeq 	clr_neg
	bset	#neg_bit,a6@(FPSR_CC)
	jra 	report_snan
clr_neg:
	bclr	#neg_bit,a6@(FPSR_CC)
	jra 	report_snan

|	end
