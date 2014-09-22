/* gen_except.s - Motorola 68040 FP exception detection (EXC) */

/* Copyright 1991-1993 Wind River Systems, Inc. */
	.data
	.globl	_copyright_wind_river
	.long	_copyright_wind_river

/*
modification history
--------------------
01g,21jul93,kdl  added .text (SPR #2372).
01f,23aug92,jcf  changed bxxx to jxx.
01e,20aug92,kdl	 put in changes from Motorola v3.7 (from FPSP 2.2):
		 added workaround for hardware bug #1384.
01d,26may92,rrr  the tree shuffle
01c,10jan92,kdl  added modification history; general cleanup.
01b,16dec91,kdl	 put in changes from Motorola v3.6 (from FPSP 2.1):
		 add translation of CMDREG1B to CMDREG3B; add check
		 for new version number in unimp frame.
01a,15aug91,kdl  original version, from Motorola FPSP v2.0; added
		 missing comment symbols.
*/

/*
DESCRIPTION


	gen_exceptsa 3.4 4/26/91


	__x_gen_except --- FPSP routine to detect reportable exceptions

	This routine compares the exception enable byte of the
	user_fpcr on the stack with the exception status byte
	of the user_fpsr.

	Any routine which may report an exceptions must load
	the stack frame in memory with the exceptional operand(s).

	Priority for exceptions is:

	Highest:	bsun
			snan
			operr
			ovfl
			unfl
			dz
			inex2
	Lowest:		inex1

	Note: The IEEE standard specifies that inex2 is to be
	reported if ovfl occurs and the ovfl enable bit is not
	set but the inex2 enable bit is.


		Copyright (C) Motorola, Inc. 1990
			All Rights Reserved

	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA
	The copyright notice above does not evidence any
	actual or intended publication of such source code.

GEN_EXCEPT    idnt    2,1 Motorola 040 Floating Point Software Package

	section 8

NOMANUAL
*/

#include "fpsp040E.h"

|	xref	__x_real_trace
|	xref	__x_fpsp_done
|	xref	__x_fpsp_fmt_error

exc_tbl:
	.long	__x_bsun_exc
	.long	commonE1
	.long	commonE1
	.long	__x_ovfl_unfl
	.long	__x_ovfl_unfl
	.long	commonE1
	.long	commonE3
	.long	commonE3
	.long	no_match

	.globl	__x_gen_except

	.text

__x_gen_except:
	cmpib	#IDLE_SIZE-4,a7@(1)	| test for idle frame
	jeq 	do_check		| go handle idle frame
	cmpib	#UNIMP_40_SIZE-4,a7@(1)	| test for orig unimp frame
	jeq 	unimp_x			| go handle unimp frame
	cmpib	#UNIMP_41_SIZE-4,a7@(1)	| test for rev unimp frame
	jeq 	unimp_x			| go handle unimp frame
	cmpib	#BUSY_SIZE-4,a7@(1)	| if size <> 0x60, fmt error
	jne 	__x_fpsp_fmt_error
	lea	a7@(BUSY_SIZE+LOCAL_SIZE),a1 | init a1 so fpsp040E.h
|					| equates will work
| Fix up the new busy frame with entries from the unimp frame
|
	movel	a6@(ETEMP_EX),a1@(ETEMP_EX) 	| copy etemp from unimp
	movel	a6@(ETEMP_HI),a1@(ETEMP_HI) 	| frame to busy frame
	movel	a6@(ETEMP_LO),a1@(ETEMP_LO)
	movel	a6@(CMDREG1B),a1@(CMDREG1B) 	| set inst in frame to unimp
	movel	a6@(CMDREG1B),d0		| fix cmd1b to make it
	andl	#0x03c30000,d0			| work for cmd3b
	bfextu	a6@(CMDREG1B){#13:#1},d1	| extract bit 2
	lsll	#5,d1
	swap	d1
	orl	d1,d0				| put it in the right place
	bfextu	a6@(CMDREG1B){#10:#3},d1	| extract bit 3,4,5
	lsll	#2,d1
	swap	d1
	orl	d1,d0				| put them in the right place
	movel	d0,a1@(CMDREG3B)		| in the busy frame
|
| Or in the FPSR from the emulation with the USER_FPSR on the stack.
|
	fmovel	FPSR,d0
	orl	d0,a6@(USER_FPSR)
	movel	a6@(USER_FPSR),a1@(FPSR_SHADOW) | set exc bits
	orl	#sx_mask,a1@(E_BYTE)
	jra 	do_clean

|
| Frame is an unimp frame possible resulting from an fmovel <ea>,fp0
| that caused an exception
|
| a1 is modified to point into the new frame allowing fpsp equates
| to be valid.
|
unimp_x:
	cmpib	#UNIMP_40_SIZE-4,a7@(1)		| test for orig unimp frame
	jne 	test_rev
	lea	a7@(UNIMP_40_SIZE+LOCAL_SIZE),a1
	jra 	unimp_con
test_rev:
	cmpib	#UNIMP_41_SIZE-4,a7@(1)		| test for rev unimp frame
	jne 	__x_fpsp_fmt_error		| if not 0x28 or 0x30
	lea	a7@(UNIMP_41_SIZE+LOCAL_SIZE),a1

unimp_con:
|
| Fix up the new unimp frame with entries from the old unimp frame
|
	movel	a6@(CMDREG1B),a1@(CMDREG1B) 	| set inst in frame to unimp
|
| Or in the FPSR from the emulation with the USER_FPSR on the stack.
|
	fmovel	FPSR,d0
	orl	d0,a6@(USER_FPSR)
	jra 	do_clean

|
| Frame is idle, so check for exceptions reported through
| USER_FPSR and set the unimp frame accordingly.
| A7 must be incremented to the point before the
| idle fsave vector to the unimp vector.
|

do_check:
	addl	#4,a7				| point a7 back to unimp frame
|
| Or in the FPSR from the emulation with the USER_FPSR on the stack.
|
	fmovel	FPSR,d0
	orl	d0,a6@(USER_FPSR)
|
| On a busy frame, we must clear the nmnexc bits.
|
	cmpib	#BUSY_SIZE-4,a7@(1)		| check frame type
	jne 	check_fr			| if busy, clr nmnexc
	clrw	a6@(NMNEXC)			| clr nmnexc # nmcexc
	btst	#5,a6@(CMDREG1B)		| test for fmovel out
	jne 	frame_com
	movel	a6@(USER_FPSR),a6@(FPSR_SHADOW) | set exc bits
	orl	#sx_mask,a6@(E_BYTE)
	jra 	frame_com

check_fr:
	cmpb	#UNIMP_40_SIZE-4,a7@(1)
	jeq 	frame_com
	clrw	a6@(NMNEXC)
frame_com:
	moveb	a6@(fpcr_ENABLE),d0		| get fpcr enable byte
	andb	a6@(FPSR_EXCEPT),d0		| and in the fpsr exc byte
	bfffo	d0{#24:#8},d1			| test for first set bit
	lea	exc_tbl,a0			| load jmp table address
	subib	#24,d1				| normalize bit offset to 0-8
	movel	a0@(d1:w:4),a0			| load routine address based
|						| based on first enabled exc
	jmp	a0@				| jump to routine
|
| Bsun is not possible in unimp or unsupp
|
__x_bsun_exc:
	jra 	do_clean
|
| The typical work to be done to the unimp frame to report an
| exception is to set the E1/E3 byte and clr the U flag.
| commonE1 does this for E1 exceptions, which are snan,
| operr, and dz.  commonE3 does this for E3 exceptions, which
| are inex2 and inex1, and also clears the E1 exception bit
| left over from the unimp exception.
|
commonE1:
	bset	#E1,a6@(E_BYTE)			| set E1 flag
	jra 	commonE				| go clean and exit

commonE3:
	tstb	a6@(UFLG_TMP)		| test flag for unsup/unimp state
	jne 	unsE3
uniE3:
	bset	#E3,a6@(E_BYTE)		| set E3 flag
	bclr	#E1,a6@(E_BYTE)		| clr E1 from unimp
	jra 	commonE

unsE3:
	tstb	a6@(RES_FLG)
	jne 	unsE3_0
unsE3_1:
	bset	#E3,a6@(E_BYTE)			| set E3 flag
unsE3_0:
	bclr	#E1,a6@(E_BYTE)			| clr E1 flag
	movel	a6@(CMDREG1B),d0
	andl	#0x03c30000,d0			| work for cmd3b
	bfextu	a6@(CMDREG1B){#13:#1},d1	| extract bit 2
	lsll	#5,d1
	swap	d1
	orl	d1,d0				| put it in the right place
	bfextu	a6@(CMDREG1B){#10:#3},d1	| extract bit 3,4,5
	lsll	#2,d1
	swap	d1
	orl	d1,d0				| put them in the right place
	movel	d0,a6@(CMDREG3B)		| in the busy frame

commonE:
	bclr	#UFLAG,a6@(T_BYTE)		| clr U flag from unimp
	jra 	do_clean			| go clean and exit
|
| No bits in the enable byte match existing exceptions.  Check for
| the case of the ovfl exc without the ovfl enabled, but with
| inex2 enabled.
|
no_match:
	btst	#__x_inex2_bit,a6@(fpcr_ENABLE) | check for ovfl/inex2 case
	jeq 	no_exc				| if clear, exit
	btst	#__x_ovfl_bit,a6@(FPSR_EXCEPT) 	| now check ovfl
	jeq 	no_exc				| if clear, exit
	jra 	__x_ovfl_unfl		| go to __x_unfl_ovfl to determine if
|					| it is an unsupp or unimp exc

| No exceptions are to be reported.  If the instruction was
| unimplemented, no FPU restore is necessary.  If it was
| unsupported, we must perform the restore.
no_exc:
	tstb	a6@(UFLG_TMP)		| test flag for unsupp/unimp state
	jeq 	uni_no_exc
uns_no_exc:
	tstb	a6@(RES_FLG)		| check if frestore is needed
	jne 	do_clean 		| if clear, no frestore needed
uni_no_exc:
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP0),fp0-fp3
	fmoveml	a6@(USER_FPCR),fpcr/fpsr/fpi
	unlk	a6
	jra 	finish_up
|
| Unsupported Data Type Handler:
| Ovfl:
|   An fmoveout that results in an overflow is reported this way.
| Unfl:
|   An fmoveout that results in an underflow is reported this way.
|
| Unimplemented Instruction Handler:
| Ovfl:
|   Only scosh, setox, ssinh, stwotox, and scale can set overflow in
|   this manner.
| Unfl:
|   Stwotox, setox, and scale can set underflow in this manner.
|   Any of the other Library Routines such that f(x)=x in which
|   x is an extended denorm can report an underflow exception.
|   It is the responsibility of the exception-causing exception
|   to make sure that WBTEMP is correct.
|
|   The exceptional operand is in FP_SCR1.
|
__x_ovfl_unfl:
	tstb	a6@(UFLG_TMP)	| test flag for unsupp/unimp state
	jeq 	ofuf_con
|
| The caller was from an unsupported data type trap.  Test if the
| caller set CU_ONLY.  If so, the exceptional operand is expected in
| FPTEMP, rather than WBTEMP.
|
	tstb	a6@(CU_ONLY)		| test if inst is cu-only
	jeq 	unsE3
|	movew	#0xfe,a6@(CU_SAVEPC)
	clrb	a6@(CU_SAVEPC)
	bset	#E1,a6@(E_BYTE)		| set E1 exception flag
	movew	a6@(ETEMP_EX),a6@(FPTEMP_EX)
	movel	a6@(ETEMP_HI),a6@(FPTEMP_HI)
	movel	a6@(ETEMP_LO),a6@(FPTEMP_LO)
	bset	#fptemp15_bit,a6@(DTAG)	| set fpte15
	bclr	#UFLAG,a6@(T_BYTE)	| clr U flag from unimp
	jra 	do_clean		| go clean and exit

ofuf_con:
	moveb	a7@,a6@(VER_TMP)	| save version number
	cmpib	#BUSY_SIZE-4,a7@(1)	| check for busy frame
	jeq 	busy_fr			| if unimp, grow to busy
	cmpib	#VER_40,a7@		| test for orig unimp frame
	jne 	try_41			| if not, test for rev frame
	moveql	#13,d0			| need to zero 14 lwords
	jra 	ofuf_fin
try_41:
	cmpib	#VER_41,a7@		| test for rev unimp frame
	jne 	__x_fpsp_fmt_error	| if neither, exit with error
	moveql	#11,d0			| need to zero 12 lwords

ofuf_fin:
	clrl	a7@
loop1:
	clrl	a7@-				| clear and dec a7
	dbra	d0,loop1
	moveb	a6@(VER_TMP),a7@
	moveb	#BUSY_SIZE-4,a7@(1)		| write busy fmt word.
busy_fr:
	movel	a6@(FP_SCR1),a6@(WBTEMP_EX)	| write
	movel	a6@(FP_SCR1+4),a6@(WBTEMP_HI)	| execptional op to
	movel	a6@(FP_SCR1+8),a6@(WBTEMP_LO)	| wbtemp
	bset	#E3,a6@(E_BYTE)			| set E3 flag
	bclr	#E1,a6@(E_BYTE)			| make sure E1 is clear
	bclr	#UFLAG,a6@(T_BYTE)		| clr U flag
	movel	a6@(USER_FPSR),a6@(FPSR_SHADOW)
	orl	#sx_mask,a6@(E_BYTE)
	movel	a6@(CMDREG1B),d0		| fix cmd1b to make it
	andl	#0x03c30000,d0			| work for cmd3b
	bfextu	a6@(CMDREG1B){#13:#1},d1	| extract bit 2
	lsll	#5,d1
	swap	d1
	orl	d1,d0				| put it in the right place
	bfextu	a6@(CMDREG1B){#10:#3},d1	| extract bit 3,4,5
	lsll	#2,d1
	swap	d1
	orl	d1,d0				| put them in the right place
	movel	d0,a6@(CMDREG3B)		| in the busy frame

|
| Check if the frame to be restored is busy or unimp.
|** NOTE *** Bug fix for errata (0d43b #3)
| If the frame is unimp, we must create a busy frame to
| fix the bug with the nmnexc bits in cases in which they
| are set by a previous instruction and not cleared by
| the save. The frame will be unimp only if the final
| instruction in an emulation routine caused the exception
| by doing an fmovel <ea>,fp0.  The exception operand, in
| internal format, is in fptemp.
|
do_clean:
	cmpib	#UNIMP_40_SIZE-4,a7@(1)
	jne 	do_con
	moveql	#13,d0			| in orig, need to zero 14 lwords
	jra 	do_build
do_con:
	cmpib	#UNIMP_41_SIZE-4,a7@(1)
	jne 	do_restore		| frame must be busy
	moveql	#11,d0			| in rev, need to zero 12 lwords

do_build:
	moveb	a7@,a6@(VER_TMP)
	clrl	a7@
loop2:
	clrl	a7@-			| clear and dec a7
	dbra	d0,loop2
|
| Use a1 as pointer into new frame.  a6 is not correct if an unimp or
| busy frame was created as the result of an exception on the final
| instruction of an emulation routine.
|
| We need to set the nmcexc bits if the exception is E1. Otherwise,
| the exc taken will be inex2.
|
	lea	a7@(BUSY_SIZE+LOCAL_SIZE),a1	| init a1 for new frame
	moveb	a6@(VER_TMP),a7@		| write busy fmt word
	moveb	#BUSY_SIZE-4,a7@(1)
	movel	a6@(FP_SCR1),a1@(WBTEMP_EX) 	| write
	movel	a6@(FP_SCR1+4),a1@(WBTEMP_HI)	| exceptional op to
	movel	a6@(FP_SCR1+8),a1@(WBTEMP_LO)	| wbtemp
|	btst	#E1,a1@(E_BYTE)
|	jeq 	do_restore
	bfextu	a6@(USER_FPSR){#17:#4},d0	| get snan/operr/ovfl/unfl bits
	bfins	d0,a1@(NMCEXC){#4:#4}		| and insert them in nmcexc
	movel	a6@(USER_FPSR),a1@(FPSR_SHADOW) | set exc bits
	orl	#sx_mask,a1@(E_BYTE)

do_restore:
	moveml	a6@(USER_DA),d0-d1/a0-a1
	fmovemx	a6@(USER_FP0),fp0-fp3
	fmoveml	a6@(USER_FPCR),fpcr/fpsr/fpi
	frestore	a7@+
	tstb	a6@(RES_FLG)	| RES_FLG indicates a "continuation" frame
	jeq 	cont
	bsrl	bug1384
cont:
	unlk	a6
|
| If trace mode enabled, then go to trace handler.  This handler
/* | cannot have any fp instructions.  If there are fp inst's and an  */
| exception has been restored into the machine then the exception
| will occur upon execution of the fp inst.  This is not desirable
| in the kernel (supervisor mode).  See MC68040 manual Section 9.3.8.
|
finish_up:
	btst	#7,a7@		| test T1 in SR
	jne 	g_trace
	btst	#6,a7@		| test T0 in SR
	jne 	g_trace
	jra 	__x_fpsp_done
|
| Change integer stack to look like trace stack
| The address of the instruction that caused the
| exception is already in the integer stack (is
| the same as the saved friar)
|
| If the current frame is already a 6-word stack then all
| that needs to be done is to change the vector# to TRACE.
| If the frame is only a 4-word stack (meaning we got here
| on an Unsupported data type exception), then we need to grow
| the stack an extra 2 words and get the fpi from the FPU.
|
g_trace:
	bftst	a7@(EXC_VEC-4){#0:#4}
	jne 	g_easy

	subw	#4,sp		|  make room
	movel	a7@(4),a7@
	movel	a7@(8),a7@(4)
	subw	#BUSY_SIZE,sp
	fsave	a7@
	fmovel	fpi,a7@(BUSY_SIZE+EXC_EA-4)
	frestore	a7@
	addw	#BUSY_SIZE,sp

g_easy:
	movew	#TRACE_VEC,a7@(EXC_VEC-4)
	jra 	__x_real_trace
|
|
|  This is a work-around for hardware bug 1384.
|
bug1384:
	link	a5,#0
	fsave	a7@-
	cmpib	#0x41,a7@	|  check for correct frame
	jeq 	frame_41
	jgt 	nofix		|  if more advanced mask, do nada

frame_40:
	tstb	a7@(1)		|  check to see if idle
	jne 	notidle
idle40:
	clrl	a7@		|  get rid of old fsave frame
        movel  d1,a6@(USER_D1)  |  save d1
	movew	#8,d1		|  place unimp frame instead
loop40:	clrl	a7@-
	dbra	d1,loop40
        movel 	a6@(USER_D1),d1  |  restore d1
	movel	#0x40280000,a7@-
	frestore	a7@+
	unlk  	a5	
	rts

frame_41:
	tstb	a7@(1)		|  check to see if idle
	jne 	notidle	
idle41:
	clrl	a7@		|  get rid of old fsave frame
        movel  d1,a6@(USER_D1)  |  save d1
	movew	#10,d1		|  place unimp frame instead
loop41:	clrl	a7@-
	dbra	d1,loop41
        movel 	a6@(USER_D1),d1  |  restore d1
	movel	#0x41300000,a7@-
	frestore	a7@+
	unlk	a5	
	rts

notidle:
	bclr	#etemp15_bit,a5@(-40) 
	frestore	a7@+
	unlk	a5	
	rts

nofix:
	frestore	a7@+
	unlk	a5	
	rts

|	end
