/* fpsp040E.h - Motorola 68040 FP definitions (EXC) */

/* Copyright 1991-1992 Wind River Systems, Inc. */
	.data
	.globl	_copyright_wind_river
	.long	_copyright_wind_river

/*
modification history
--------------------
01c,26may92,rrr  the tree shuffle
01b,10jan92,kdl  added modification history; general cleanup.
01a,15aug91,kdl  original version, from Motorola FPSP v2.0.
*/

/*
DESCRIPTION


	fpsp040E.h 3.3 3.3


		Copyright (C) Motorola, Inc. 1990
			All Rights Reserved

	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA
	The copyright notice above does not evidence any
	actual or intended publication of such source code.

	fpsp040E.h --- stack frame offsets during FPSP exception handling

	These equates are used to access the exception frame, the fsave
	frame and any local variables needed by the FPSP package.

	All FPSP handlers begin by executing:

		link	a6,#-LOCAL_SIZE
		fsave	a7@-
		moveml	d0-d1/a0-a1,a6@(USER_DA)
		fmovemx fp0-fp3,a6@(USER_FP0)
		fmovel	fpsr/fpcr/fpi,a6@(USER_FPSR)

	After initialization, the stack looks like this:

	A7 --->	+-------------------------------+
		|				|
		|	FPU fsave area		|
		|				|
		+-------------------------------+
		|				|
		|	FPSP Local Variables	|
		|	     including		|
		|	  saved registers	|
		|				|
		+-------------------------------+
	A6 --->	|	Saved A6		|
		+-------------------------------+
		|				|
		|	Exception Frame		|
		|				|
		|				|

	Positive offsets from A6 refer to the exception frame.  Negative
	offsets refer to the Local Variable area and the fsave area.
	The fsave frame is also accessible 'from the top' via A7.

	On exit, the handlers execute:

		moveml	a6@(USER_DA),d0-d1/a0-a1
		fmovemx	a6@(USER_FP0),fp0-fp3
		fmovel	a6@(USER_FPSR),fpsr/fpcr/fpi
		frestore	a7@+
		unlk	a6

	and then either 'bra __x_fpsp_done' if the exception was completely
	handled	by the package, or 'bra real_xxxx' which is an external
	label to a routine that will process a real exception of the
	type that was generated.  Some handlers may omit the 'frestore'
	if the FPU state after the exception is idle.

	Sometimes the exception handler will transform the fsave area
	because it needs to report an exception back to the user.  This
	can happen if the package is entered for an unimplemented float
	instruction that generates (say) an underflow.  Alternatively,
	a second fsave frame can be pushed onto the stack and the
	handler	exit code will reload the new frame and discard the old.

	The registers d0, d1, a0, a1 and fp0-fp3 are always saved and
	restored from the 'local variable' area and can be used as
	temporaries.  If a routine needs to change any
	of these registers, it should modify the saved copy and let
	the handler exit code restore the value.


NOMANUAL
*/


|----------------------------------------------------------------------
|
|	Local Variables on the stack
|
#define	LOCAL_SIZE		192		/* bytes req'd for local vars */
#define	LV			-LOCAL_SIZE	/* convenient base value */

#define	USER_DA			LV+0		/* save space for d0-d1,a0-a1 */
#define	USER_D0			LV+0		/* saved user d0 */
#define	USER_D1			LV+4		/* saved user d1 */
#define	USER_A0			LV+8		/* saved user a0 */
#define	USER_A1			LV+12		/* saved user a1 */
#define	USER_FP0		LV+16		/* saved user fp0 */
#define	USER_FP1		LV+28		/* saved user fp1 */
#define	USER_FP2		LV+40		/* saved user fp2 */
#define	USER_FP3		LV+52		/* saved user fp3 */
#define	USER_FPCR		LV+64		/* saved user fpcr */
#define	fpcr_ENABLE		USER_FPCR+2	/* fpcr exception enable  */
#define	fpcr_MODE		USER_FPCR+3	/* fpcr rounding mode control */
#define	USER_FPSR		LV+68		/* saved user FPSR */
#define	FPSR_CC			USER_FPSR+0	/* FPSR condition code */
#define	FPSR_QBYTE		USER_FPSR+1	/* FPSR quotient */
#define	FPSR_EXCEPT		USER_FPSR+2	/* FPSR exception */
#define	FPSR_AEXCEPT		USER_FPSR+3	/* FPSR accrued exception */
#define	USER_fpi		LV+72		/* saved user fpi */
#define	FP_SCR1			LV+76		/* room for temp float value */
#define	FP_SCR2			LV+92		/* room for temp float value */
#define	L_SCR1			LV+108		/* room for temp long value */
#define	L_SCR2			LV+112		/* room for temp long value */
#define	STORE_FLG		LV+116
#define	BINDEC_FLG		LV+117		/* used in __x_bindec */
#define	DNRM_FLG		LV+118		/* used in __x_res_func */
#define	RES_FLG			LV+119		/* used in __x_res_func */
#define	DY_MO_FLG		LV+120		/* dyadic/monadic flag */
#define	UFLG_TMP		LV+121		/* temporary for uflag errata */
#define	CU_ONLY			LV+122		/* cu-only flag */
#define	VER_TMP			LV+123		/* temp holding for version no*/
#define	L_SCR3			LV+124		/* room for temp long value */
#define	FP_SCR3			LV+128		/* room for temp float value */
#define	FP_SCR4			LV+144		/* room for temp float value */
#define	FP_SCR5			LV+160		/* room for temp float value */
#define	FP_SCR6			LV+176
|
|NEXT		equ	LV+192		| need to increase LOCAL_SIZE
|
|--------------------------------------------------------------------------
|
|	fsave offsets and bit definitions
|
|	Offsets are defined from the end of an fsave because the last 10
|	.words of a busy frame are the same as the unimplemented frame.
|
#define	CU_SAVEPC		LV-92		/* micro-pc for CU (1 byte) */
#define	FPR_DIRTY_BITS		LV-91		/* fpr dirty bits */

#define	WBTEMP			LV-76		/* write back temp (12 bytes) */
#define	WBTEMP_EX		WBTEMP		/*  sign & exponent (2 bytes) */
#define	WBTEMP_HI		WBTEMP+4	/*  mantissa [63:32] (4 bytes)*/
#define	WBTEMP_LO		WBTEMP+8	/*  mantissa [31:00] (4 bytes)*/

#define	WBTEMP_SGN		WBTEMP+2	/* used to store sign */

#define	FPSR_SHADOW		LV-64		/* fpsr shadow reg */

#define	fpiCU			LV-60		/* Instr. addr. reg. for CU
						 * (4 bytes) */

#define	CMDREG2B		LV-52		/* cmd reg for machine 2 */
#define	CMDREG3B		LV-48		/* cmd reg for E3 exceptions
						 * (2 bytes) */

#define	NMNEXC			LV-44		/* NMNEXC (unsup,snan bits) */
#define	nmn_unsup_bit		1
#define	nmn_snan_bit		0

#define	NMCEXC			LV-43		/* NMNEXC # NMCEXC */
#define	nmn_operr_bit		7
#define	nmn_ovfl_bit		6
#define	nmn_unfl_bit		5
#define	nmc_unsup_bit		4
#define	nmc_snan_bit		3
#define	nmc_operr_bit		2
#define	nmc_ovfl_bit		1
#define	nmc_unfl_bit		0
|
#define	STAG			LV-40		/* source tag (1 byte) */
#define	WBTEMP_GRS		LV-40		/* alias wbtemp guard, round, sticky */
#define	guard_bit		1		/* guard bit is bit number 1 */
#define	__x_round_bit		0		/* round bit is bit number 0 */
#define	stag_mask		0xE0		/* upper 3 bits are source tag
						 * type */
#define	__x_denorm_bit		7		/* bit determins if denorm or
						 * unnorm */
#define	etemp15_bit		4		/* etemp exponent bit #15 */
#define	wbtemp66_bit		2		/* wbtemp mantissa bit #66 */
#define	wbtemp1_bit		1		/* wbtemp mantissa bit #1 */
#define	wbtemp0_bit		0		/* wbtemp mantissa bit #0 */
|
#define	STICKY			LV-39		/* holds sticky bit */
#define	sticky_bit		7
|
#define	CMDREG1B		LV-36		/* cmd reg for E1 exceptions
						 * (2 bytes) */
#define	kfact_bit		12		/* distinguishes static/dynamic
						 * k-factor on packed move out.
						 * NOTE: this equate only works
						 * when CMDREG1B is in a reg. */

#define	CMDWORD			LV-35		/* command word in cmd1b */
#define	direction_bit		5		/* bit 0 in opclass */
#define	size_bit2		12		/* bit 2 in size field */
|
#define	DTAG			LV-32		/* dest tag (1 byte) */
#define	dtag_mask		0xE0		/* upper 3 bits are dest type */
#define	fptemp15_bit		4		/* fptemp exponent bit #15 */
|
#define	WB_BYTE			LV-31		/* holds WBTE15 bit (1 byte) */
#define	wbtemp15_bit		4		/* wbtemp exponent bit #15 */

#define	E_BYTE			LV-28		/* holds E1 & E3 bits (1 byte)*/
#define	E1			2		/* which bit is E1 flag */
#define	E3			1		/* which bit is E3 flag */
#define	SFLAG			0		/* which bit is S flag */

#define	T_BYTE			LV-27		/* holds T and U bits (1 byte)*/
#define	XFLAG			7		/* which bit is X flag */
#define	UFLAG			5		/* which bit is U flag */
#define	TFLAG			4		/* which bit is T flag */
|
#define	FPTEMP			LV-24		/* fptemp (12 bytes) */
#define	FPTEMP_EX		FPTEMP		/*  sign & exponent (2 bytes) */
#define	FPTEMP_HI		FPTEMP+4	/*  mantissa [63:32] (4 bytes)*/
#define	FPTEMP_LO		FPTEMP+8	/*  mantissa [31:00] (4 bytes)*/

#define	FPTEMP_SGN		FPTEMP+2	/* used to store sign */

#define	ETEMP			LV-12		/* etemp (12 bytes) */
#define	ETEMP_EX		ETEMP		/*  sign & exponent (2 bytes) */
#define	ETEMP_HI		ETEMP+4		/*  mantissa [63:32] (4 bytes)*/
#define	ETEMP_LO		ETEMP+8		/*  mantissa [31:00] (4 bytes)*/

#define	ETEMP_SGN		ETEMP+2		/* used to store sign */

#define	EXC_SR			4		/* exc frame status register */
#define	EXC_PC			6		/* exc frame program counter */
#define	EXC_VEC			10		/* exc frame vector
						 * (format+vector#) */
#define	EXC_EA			12		/* exc frame effective addr */

|--------------------------------------------------------------------------
|
|	FPSR/fpcr bits
|
#define	neg_bit			3	/* negative result */
#define	z_bit			2	/* zero result */
#define	inf_bit			1	/* infinity result */
#define	nan_bit			0	/* not-a-number result */
|
#define	q_sn_bit		7	/* sign bit of quotient byte */
|
#define	__x_bsun_bit		7	/* branch on unordered */
#define	__x_snan_bit		6	/* signalling nan */
#define	__x_operr_bit		5	/* operand error */
#define	__x_ovfl_bit		4	/* overflow */
#define	__x_unfl_bit		3	/* underflow */
#define	__x_dz_bit		2	/* divide by zero */
#define	__x_inex2_bit		1	/* inexact result 2 */
#define	__x_inex1_bit		0	/* inexact result 1 */
|
#define	aiop_bit		7	/* accrued illegal operation */
#define	aovfl_bit		6	/* accrued overflow */
#define	aunfl_bit		5	/* accrued underflow */
#define	adz_bit			4	/* accrued divide by zero */
#define	ainex_bit		3	/* accrued inexact */
|
|	FPSR individual bit masks
|
#define	neg_mask		0x08000000
#define	z_mask			0x04000000
#define	inf_mask		0x02000000
#define	nan_mask		0x01000000
|
#define	__x_bsun_mask		0x00008000
#define	__x_snan_mask		0x00004000
#define	__x_operr_mask		0x00002000
#define	__x_ovfl_mask		0x00001000
#define	__x_unfl_mask		0x00000800
#define	__x_dz_mask		0x00000400
#define	__x_inex2_mask		0x00000200
#define	__x_inex1_mask		0x00000100
|
#define	aiop_mask		0x00000080	/* accrued illegal operation */
#define	aovfl_mask		0x00000040	/* accrued overflow */
#define	aunfl_mask		0x00000020	/* accrued underflow */
#define	adz_mask		0x00000010	/* accrued divide by zero */
#define	ainex_mask		0x00000008	/* accrued inexact */
|
|	FPSR combinations used in the FPSP
|
#define	__x_dzinf_mask		inf_mask+__x_dz_mask+adz_mask
#define	opnan_mask		nan_mask+__x_operr_mask+aiop_mask
#define	nzi_mask		0x01ffffff 	/* clears N, Z, and I */
#define	unfinx_mask	__x_unfl_mask+__x_inex2_mask+aunfl_mask+ainex_mask
#define	unf2inx_mask		__x_unfl_mask+__x_inex2_mask+ainex_mask
#define	ovfinx_mask		__x_ovfl_mask+__x_inex2_mask+aovfl_mask+ainex_mask
#define	inx1a_mask		__x_inex1_mask+ainex_mask
#define	inx2a_mask		__x_inex2_mask+ainex_mask
#define	__x_snaniop_mask	nan_mask+__x_snan_mask+aiop_mask
#define	naniop_mask		nan_mask+aiop_mask
#define	neginf_mask		neg_mask+inf_mask
#define	infaiop_mask		inf_mask+aiop_mask
#define	negz_mask		neg_mask+z_mask
#define	opaop_mask		__x_operr_mask+aiop_mask
#define	__x_unfl_inx_mask	__x_unfl_mask+aunfl_mask+ainex_mask
#define	__x_ovfl_inx_mask	__x_ovfl_mask+aovfl_mask+ainex_mask
|
|--------------------------------------------------------------------------
|
|	fpcr rounding modes
|
#define	x_mode			0x00	/* round to extended */
#define	s_mode			0x40	/* round to single */
#define	d_mode			0x80	/* round to double */

#define	rn_mode			0x00	/* round nearest */
#define	rz_mode			0x10	/* round to zero */
#define	rm_mode			0x20	/* round to minus infinity */
#define	rp_mode			0x30	/* round to plus infinity */
|
|--------------------------------------------------------------------------
|
|	Miscellaneous equates
|
#define	signan_bit		6	/* signalling nan bit in mantissa */
#define	sign_bit		7
|
#define	rnd_stky_bit		29	/* round/sticky bit of mantissa
					 * this can only be used if in a
					 * data register */
#define	sx_mask			0x01800000 /* set s and x bits in word 0x48 */

#define	LOCAL_EX		0
#define	LOCAL_SGN		2
#define	LOCAL_HI		4
#define	LOCAL_LO		8
#define	LOCAL_GRS		12	/* valid ONLY for FP_SCR1, FP_SCR2 */


#define	__x_norm_tag		0x00	/* tag bits in {7:5} position */
#define	zero_tag		0x20
#define	inf_tag			0x40
#define	nan_tag			0x60
#define	dnrm_tag		0x80
|
|	fsave sizes and formats
|
#define	VER_4			0x40		/* fpsp compatible version
					 	 * numbers are in the 0x40s
						 * {0x40-0x4f} */

#define	VER_40			0x40		/* original version number */
#define	VER_41			0x41		/* revision version number */

#define	BUSY_SIZE		100		/* size of busy frame */
#define	BUSY_FRAME		LV-BUSY_SIZE	/* start of busy frame */

#define	UNIMP_40_SIZE		44		/* size of orig unimp frame */
#define	UNIMP_41_SIZE		52		/* size of rev unimp frame */

#define	IDLE_SIZE		4		/* size of idle frame */
#define	IDLE_FRAME		LV-IDLE_SIZE	/* start of idle frame */
|
|	exception vectors
|
#define	TRACE_VEC		0x2024		/* trace trap */
#define	FLINE_VEC		0x002C		/* 'real' F-line */
#define	UNIMP_VEC		0x202C		/* unimplemented */
#define	INEX_VEC		0x00C4

#define	dbl_thresh		0x3C01
#define	sgl_thresh		0x3F81
|
