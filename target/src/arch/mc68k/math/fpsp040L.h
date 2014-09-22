/* fpsp040L.h - Motorola 68040 FP definitions (LIB) */

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


	fpsp040L.h 1.1 3/27/91


		Copyright (C) Motorola, Inc. 1991
			All Rights Reserved

	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA
	The copyright notice above does not evidence any
	actual or intended publication of such source code.

	fpsp040L.h --- stack frame offsets for library version of FPSP

	This file is derived from fpsp040E.h.  All equates that refer
	to the fsave frame and it's bits are removed with the
	exception of ETEMP, WBTEMP, DTAG and STAG which are simulated
	in the library version.  Equates for the exception frame are
	also not needed.  Some of the equates that are only used in
	the kernel version of the FPSP are left in to minimize the
	differences between this file and the original.

	The library routines use the same source files as the regular
	kernel mode code so they expect the same setup.  That is, you
	must create enough space on the stack for all save areas and
	work variables that are needed, and save any registers that
	your compiler does not treat as scratch registers on return
	from function calls.

	The worst case setup is:

		link	a6,#-LOCAL_SIZE
		moveml	d0-d1/a0-a1,a6@(USER_DA)
		fmovemx fp0-fp3,a6@(USER_FP0)
		fmoveml fpsr/fpcr,a6@(USER_FPSR)

	After initialization, the stack looks like this:

	A7 --->	+-------------------------------+
		|				|
		|	FPSP Local Variables	|
		|	     including		|
		|	  saved registers	|
		|				|
		+-------------------------------+
	A6 --->	|	Saved A6		|
		+-------------------------------+
		|	Return PC		|
		+-------------------------------+
		|	Arguments to 		|
		|	an FPSP library		|
		|	package			|
		|				|

	Positive offsets from A6 refer to the input arguments.  Negative
	offsets refer to the Local Variable area.

	On exit, execute:

		moveml	a6@(USER_DA),d0-d1/a0-a1
		fmovemx	a6@(USER_FP0),fp0-fp3
		fmovel	a6@(USER_FPSR),fpsr/fpcr
		unlk	a6
		rts

	Many 68K C compilers treat a0/a1/d0/d1/fp0/fp1 as scratch so
	a simplified setup/exit is possible:

		link	a6,#-LOCAL_SIZE
		fmovemx fp2-fp3,a6@(USER_FP2)
		fmovel	fpsr/fpcr,a6@(USER_FPSR)

		[call appropriate emulation routine]

		fmovemx	a6@(USER_FP2),fp2-fp3
		fmovel	a6@(USER_FPSR),fpsr/fpcr
		unlk	a6
		rts

	Note that you must still save fp2/fp3 because the FPSP emulation
	routines expect fp0-fp3 as scratch registers.  For all monadic
	entry points, the caller should save the fpcr in d1 and zero the
	real fpcr before calling the emulation routine.  On return, the
	monadic emulation code will place the value supplied in d1 back
	into the fpcr and do a single floating point operation so that
	the final result will be correctly rounded and any specified
	exceptions will be generated.


NOMANUAL
*/


|----------------------------------------------------------------------
|
|	Local Variables on the stack
|
#define	LOCAL_SIZE		228		/* bytes req'd for local vars */
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
#define	BINDEC_FLG		LV+117		/* used in __l_bindec */
#define	DNRM_FLG		LV+118		/* used in __l_res_func */
#define	RES_FLG			LV+119		/* used in __l_res_func */
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
|--------------------------------------------------------------------------
|
#define	STAG			LV+192		/* source tag (1 byte) */
|
#define	DTAG			LV+193		/* dest tag (1 byte) */
|
#define	FPTEMP			LV+196		/* fptemp (12 bytes) */
#define	FPTEMP_EX		FPTEMP		/*  sign & exponent (2 bytes) */
#define	FPTEMP_HI		FPTEMP+4	/*  mantissa [63:32] (4 bytes)*/
#define	FPTEMP_LO		FPTEMP+8	/*  mantissa [31:00] (4 bytes)*/
|
#define	FPTEMP_SGN		FPTEMP+2	/* used to store sign */
|
#define	ETEMP			LV+208		/* etemp (12 bytes) */
#define	ETEMP_EX		ETEMP		/*  sign & exponent (2 bytes) */
#define	ETEMP_HI		ETEMP+4		/*  mantissa [63:32] (4 bytes)*/
#define	ETEMP_LO		ETEMP+8		/*  mantissa [31:00] (4 bytes)*/
|
#define	ETEMP_SGN		ETEMP+2		/* used to store sign */
|
|--------------------------------------------------------------------------
|
|	FPSR/fpcr bits
|
#define	neg_bit			3	/* negative result */
#define	z_bit			2	/* zero result */
#define	inf_bit			1	/* infinity result */
#define	nan_bit			0	/* not-a-number result */

#define	q_sn_bit		7	/* sign bit of quotient byte */

#define	__l_bsun_bit		7	/* branch on unordered */
#define	__l_snan_bit		6	/* signalling nan */
#define	__l_operr_bit		5	/* operand error */
#define	__l_ovfl_bit		4	/* overflow */
#define	__l_unfl_bit		3	/* underflow */
#define	__l_dz_bit		2	/* divide by zero */
#define	__l_inex2_bit		1	/* inexact result 2 */
#define	__l_inex1_bit		0	/* inexact result 1 */
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

#define	__l_bsun_mask		0x00008000
#define	__l_snan_mask		0x00004000
#define	__l_operr_mask		0x00002000
#define	__l_ovfl_mask		0x00001000
#define	__l_unfl_mask		0x00000800
#define	__l_dz_mask		0x00000400
#define	__l_inex2_mask		0x00000200
#define	__l_inex1_mask		0x00000100

#define	aiop_mask		0x00000080	/* accrued illegal operation */
#define	aovfl_mask		0x00000040	/* accrued overflow */
#define	aunfl_mask		0x00000020	/* accrued underflow */
#define	adz_mask		0x00000010	/* accrued divide by zero */
#define	ainex_mask		0x00000008	/* accrued inexact */
|
|	FPSR combinations used in the FPSP
|
#define	__l_dzinf_mask		inf_mask+__l_dz_mask+adz_mask
#define	opnan_mask		nan_mask+__l_operr_mask+aiop_mask
#define	nzi_mask		0x01ffffff 	/* clears N, Z, and I */
#define	unfinx_mask	__l_unfl_mask+__l_inex2_mask+aunfl_mask+ainex_mask
#define	unf2inx_mask		__l_unfl_mask+__l_inex2_mask+ainex_mask
#define	ovfinx_mask	__l_ovfl_mask+__l_inex2_mask+aovfl_mask+ainex_mask
#define	inx1a_mask		__l_inex1_mask+ainex_mask
#define	inx2a_mask		__l_inex2_mask+ainex_mask
#define	__l_snaniop_mask	nan_mask+__l_snan_mask+aiop_mask
#define	naniop_mask		nan_mask+aiop_mask
#define	neginf_mask		neg_mask+inf_mask
#define	infaiop_mask		inf_mask+aiop_mask
#define	negz_mask		neg_mask+z_mask
#define	opaop_mask		__l_operr_mask+aiop_mask
#define	__l_unfl_inx_mask	__l_unfl_mask+aunfl_mask+ainex_mask
#define	__l_ovfl_inx_mask	__l_ovfl_mask+aovfl_mask+ainex_mask
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

#define	signan_bit		6	/* signalling nan bit in mantissa */
#define	sign_bit		7

#define	rnd_stky_bit		29	/* round/sticky bit of mantissa
					 * this can only be used if in a
					 * data register */
#define	LOCAL_EX		0
#define	LOCAL_SGN		2
#define	LOCAL_HI		4
#define	LOCAL_LO		8
#define	LOCAL_GRS		12	/* valid ONLY for FP_SCR1, FP_SCR2 */


#define	__l_norm_tag		0x00	/* tag bits in {7:5} position */
#define	zero_tag		0x20
#define	inf_tag			0x40
#define	nan_tag			0x60
#define	dnrm_tag		0x80

#define	dbl_thresh		0x3C01
#define	sgl_thresh		0x3F81

