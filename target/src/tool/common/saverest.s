/* saverest.s - Power PC save and restore EABI functions

/* Copyright 1984-2001 Wind River Systems, Inc. */

/*
modification history 
--------------------
01b,22apr02,sn   SPR 76106 - added marker symbol
01a,03dec01,sn   wrote
*/

#define _ASMLANGUAGE
#include "vxWorks.h"
#include "asm.h"

		.data
		.globl __saverest_o
__saverest_o:	
		.long 0x0
		
_WRS_TEXT_SEG_START
	
#if	((CPU != PPC403) && (CPU != PPC405) && (CPU != PPC440))

/* restfpr.s */

		FUNC_EXPORT(_restfpr_14_l)
		FUNC_EXPORT(_restfpr_15_l)
		FUNC_EXPORT(_restfpr_16_l)
		FUNC_EXPORT(_restfpr_17_l)
		FUNC_EXPORT(_restfpr_18_l)
		FUNC_EXPORT(_restfpr_19_l)
		FUNC_EXPORT(_restfpr_20_l)
		FUNC_EXPORT(_restfpr_21_l)
		FUNC_EXPORT(_restfpr_22_l)
		FUNC_EXPORT(_restfpr_23_l)
		FUNC_EXPORT(_restfpr_24_l)
		FUNC_EXPORT(_restfpr_25_l)
		FUNC_EXPORT(_restfpr_26_l)
		FUNC_EXPORT(_restfpr_27_l)
		FUNC_EXPORT(_restfpr_28_l)
		FUNC_EXPORT(_restfpr_29_l)
		FUNC_EXPORT(_restfpr_30_l)
		FUNC_EXPORT(_restfpr_31_l)
		.long	0x00600000
_restfpr_14_l:	lfd	f14,-144(r11)
_restfpr_15_l:	lfd	f15,-136(r11)
_restfpr_16_l:	lfd	f16,-128(r11)
_restfpr_17_l:	lfd	f17,-120(r11)
_restfpr_18_l:	lfd	f18,-112(r11)
_restfpr_19_l:	lfd	f19,-104(r11)
_restfpr_20_l:	lfd	f20,-96(r11)
_restfpr_21_l:	lfd	f21,-88(r11)
_restfpr_22_l:	lfd	f22,-80(r11)
_restfpr_23_l:	lfd	f23,-72(r11)
_restfpr_24_l:	lfd	f24,-64(r11)
_restfpr_25_l:	lfd	f25,-56(r11)
_restfpr_26_l:	lfd	f26,-48(r11)
_restfpr_27_l:	lfd	f27,-40(r11)
_restfpr_28_l:	lwz	r0,4(r11)
		lfd	f28,-32(r11)
		mtspr	8,r0
		lfd	f29,-24(r11)
		lfd	f30,-16(r11)
		lfd	f31,-8(r11)
		ori	r1,r11,0
		blr

_restfpr_29_l:	lfd	f29,-24(r11)
_restfpr_30_l:	lfd	f30,-16(r11)
_restfpr_31_l:	lwz	r0,4(r11)
		lfd	f31,-8(r11)
		mtspr	8,r0
		ori	r1,r11,0
		blr

/* savefpr.s */

		FUNC_EXPORT(_savefpr_14_l)
		FUNC_EXPORT(_savefpr_15_l)
		FUNC_EXPORT(_savefpr_16_l)
		FUNC_EXPORT(_savefpr_17_l)
		FUNC_EXPORT(_savefpr_18_l)
		FUNC_EXPORT(_savefpr_19_l)
		FUNC_EXPORT(_savefpr_20_l)
		FUNC_EXPORT(_savefpr_21_l)
		FUNC_EXPORT(_savefpr_22_l)
		FUNC_EXPORT(_savefpr_23_l)
		FUNC_EXPORT(_savefpr_24_l)
		FUNC_EXPORT(_savefpr_25_l)
		FUNC_EXPORT(_savefpr_26_l)
		FUNC_EXPORT(_savefpr_27_l)
		FUNC_EXPORT(_savefpr_28_l)
		FUNC_EXPORT(_savefpr_29_l)
		FUNC_EXPORT(_savefpr_30_l)
		FUNC_EXPORT(_savefpr_31_l)
		.long	0x00400000
_savefpr_14_l:	stfd	f14,-144(r11)
_savefpr_15_l:	stfd	f15,-136(r11)
_savefpr_16_l:	stfd	f16,-128(r11)
_savefpr_17_l:	stfd	f17,-120(r11)
_savefpr_18_l:	stfd	f18,-112(r11)
_savefpr_19_l:	stfd	f19,-104(r11)
_savefpr_20_l:	stfd	f20,-96(r11)
_savefpr_21_l:	stfd	f21,-88(r11)
_savefpr_22_l:	stfd	f22,-80(r11)
_savefpr_23_l:	stfd	f23,-72(r11)
_savefpr_24_l:	stfd	f24,-64(r11)
_savefpr_25_l:	stfd	f25,-56(r11)
_savefpr_26_l:	stfd	f26,-48(r11)
_savefpr_27_l:	stfd	f27,-40(r11)
_savefpr_28_l:	stfd	f28,-32(r11)
_savefpr_29_l:	stfd	f29,-24(r11)
_savefpr_30_l:	stfd	f30,-16(r11)
_savefpr_31_l:	stfd	f31,-8(r11)
		stw	r0,4(r11)
		blr

#endif	/* (CPU != PPC403) && (CPU != PPC405) && (CPU != PPC440) */

/* restgpr.s */

		FUNC_EXPORT(_restgpr_14)
		FUNC_EXPORT(_restgpr_15)
		FUNC_EXPORT(_restgpr_16)
		FUNC_EXPORT(_restgpr_17)
		FUNC_EXPORT(_restgpr_18)
		FUNC_EXPORT(_restgpr_19)
		FUNC_EXPORT(_restgpr_20)
		FUNC_EXPORT(_restgpr_21)
		FUNC_EXPORT(_restgpr_22)
		FUNC_EXPORT(_restgpr_23)
		FUNC_EXPORT(_restgpr_24)
		FUNC_EXPORT(_restgpr_25)
		FUNC_EXPORT(_restgpr_26)
		FUNC_EXPORT(_restgpr_27)
		FUNC_EXPORT(_restgpr_28)
		FUNC_EXPORT(_restgpr_29)
		FUNC_EXPORT(_restgpr_30)
		FUNC_EXPORT(_restgpr_31)
		.long	0x00600000
_restgpr_14:		lwz	r14,-72(r11)
_restgpr_15:		lwz	r15,-68(r11)
_restgpr_16:		lwz	r16,-64(r11)
_restgpr_17:		lwz	r17,-60(r11)
_restgpr_18:		lwz	r18,-56(r11)
_restgpr_19:		lwz	r19,-52(r11)
_restgpr_20:		lwz	r20,-48(r11)
_restgpr_21:		lwz	r21,-44(r11)
_restgpr_22:		lwz	r22,-40(r11)
_restgpr_23:		lwz	r23,-36(r11)
_restgpr_24:		lwz	r24,-32(r11)
_restgpr_25:		lwz	r25,-28(r11)
_restgpr_26:		lwz	r26,-24(r11)
_restgpr_27:		lwz	r27,-20(r11)
_restgpr_28:		lwz	r28,-16(r11)
_restgpr_29:		lwz	r29,-12(r11)
_restgpr_30:		lwz	r30,-8(r11)
_restgpr_31:		lwz	r31,-4(r11)
		blr

/* savegpr.s */

		FUNC_EXPORT(_savegpr_14)
		FUNC_EXPORT(_savegpr_15)
		FUNC_EXPORT(_savegpr_16)
		FUNC_EXPORT(_savegpr_17)
		FUNC_EXPORT(_savegpr_18)
		FUNC_EXPORT(_savegpr_19)
		FUNC_EXPORT(_savegpr_20)
		FUNC_EXPORT(_savegpr_21)
		FUNC_EXPORT(_savegpr_22)
		FUNC_EXPORT(_savegpr_23)
		FUNC_EXPORT(_savegpr_24)
		FUNC_EXPORT(_savegpr_25)
		FUNC_EXPORT(_savegpr_26)
		FUNC_EXPORT(_savegpr_27)
		FUNC_EXPORT(_savegpr_28)
		FUNC_EXPORT(_savegpr_29)
		FUNC_EXPORT(_savegpr_30)
		FUNC_EXPORT(_savegpr_31)
		.long	0x00400000
_savegpr_14:		stw	r14,-72(r11)
_savegpr_15:		stw	r15,-68(r11)
_savegpr_16:		stw	r16,-64(r11)
_savegpr_17:		stw	r17,-60(r11)
_savegpr_18:		stw	r18,-56(r11)
_savegpr_19:		stw	r19,-52(r11)
_savegpr_20:		stw	r20,-48(r11)
_savegpr_21:		stw	r21,-44(r11)
_savegpr_22:		stw	r22,-40(r11)
_savegpr_23:		stw	r23,-36(r11)
_savegpr_24:		stw	r24,-32(r11)
_savegpr_25:		stw	r25,-28(r11)
_savegpr_26:		stw	r26,-24(r11)
_savegpr_27:		stw	r27,-20(r11)
_savegpr_28:		stw	r28,-16(r11)
_savegpr_29:		stw	r29,-12(r11)
_savegpr_30:		stw	r30,-8(r11)
_savegpr_31:		stw	r31,-4(r11)
		blr

/* restgprl.s */

		FUNC_EXPORT(_restgpr_14_l)
		FUNC_EXPORT(_restgpr_15_l)
		FUNC_EXPORT(_restgpr_16_l)
		FUNC_EXPORT(_restgpr_17_l)
		FUNC_EXPORT(_restgpr_18_l)
		FUNC_EXPORT(_restgpr_19_l)
		FUNC_EXPORT(_restgpr_20_l)
		FUNC_EXPORT(_restgpr_21_l)
		FUNC_EXPORT(_restgpr_22_l)
		FUNC_EXPORT(_restgpr_23_l)
		FUNC_EXPORT(_restgpr_24_l)
		FUNC_EXPORT(_restgpr_25_l)
		FUNC_EXPORT(_restgpr_26_l)
		FUNC_EXPORT(_restgpr_27_l)
		FUNC_EXPORT(_restgpr_28_l)
		FUNC_EXPORT(_restgpr_29_l)
		FUNC_EXPORT(_restgpr_30_l)
		FUNC_EXPORT(_restgpr_31_l)
		.long	0x00600000
_restgpr_14_l:	lwz	r14,-72(r11)
_restgpr_15_l:	lwz	r15,-68(r11)
_restgpr_16_l:	lwz	r16,-64(r11)
_restgpr_17_l:	lwz	r17,-60(r11)
_restgpr_18_l:	lwz	r18,-56(r11)
_restgpr_19_l:	lwz	r19,-52(r11)
_restgpr_20_l:	lwz	r20,-48(r11)
_restgpr_21_l:	lwz	r21,-44(r11)
_restgpr_22_l:	lwz	r22,-40(r11)
_restgpr_23_l:	lwz	r23,-36(r11)
_restgpr_24_l:	lwz	r24,-32(r11)
_restgpr_25_l:	lwz	r25,-28(r11)
_restgpr_26_l:	lwz	r26,-24(r11)
_restgpr_27_l:	lwz	r0,4(r11)
		lwz	r27,-20(r11)
		mtspr	8,r0
		lwz	r28,-16(r11)
		lwz	r29,-12(r11)
		lwz	r30,-8(r11)
		lwz	r31,-4(r11)
		ori	r1,r11,0
		blr

_restgpr_28_l:	lwz	r28,-16(r11)
_restgpr_29_l:	lwz	r29,-12(r11)
_restgpr_30_l:	lwz	r30,-8(r11)
_restgpr_31_l:	lwz	r0,4(r11)
		lwz	r31,-4(r11)
		mtspr	8,r0
		ori	r1,r11,0
		blr


/* savegprl.s */

		FUNC_EXPORT(_savegpr_14_l)
		FUNC_EXPORT(_savegpr_15_l)
		FUNC_EXPORT(_savegpr_16_l)
		FUNC_EXPORT(_savegpr_17_l)
		FUNC_EXPORT(_savegpr_18_l)
		FUNC_EXPORT(_savegpr_19_l)
		FUNC_EXPORT(_savegpr_20_l)
		FUNC_EXPORT(_savegpr_21_l)
		FUNC_EXPORT(_savegpr_22_l)
		FUNC_EXPORT(_savegpr_23_l)
		FUNC_EXPORT(_savegpr_24_l)
		FUNC_EXPORT(_savegpr_25_l)
		FUNC_EXPORT(_savegpr_26_l)
		FUNC_EXPORT(_savegpr_27_l)
		FUNC_EXPORT(_savegpr_28_l)
		FUNC_EXPORT(_savegpr_29_l)
		FUNC_EXPORT(_savegpr_30_l)
		FUNC_EXPORT(_savegpr_31_l)
		.long	0x00400000
_savegpr_14_l:	stw	r14,-72(r11)
_savegpr_15_l:	stw	r15,-68(r11)
_savegpr_16_l:	stw	r16,-64(r11)
_savegpr_17_l:	stw	r17,-60(r11)
_savegpr_18_l:	stw	r18,-56(r11)
_savegpr_19_l:	stw	r19,-52(r11)
_savegpr_20_l:	stw	r20,-48(r11)
_savegpr_21_l:	stw	r21,-44(r11)
_savegpr_22_l:	stw	r22,-40(r11)
_savegpr_23_l:	stw	r23,-36(r11)
_savegpr_24_l:	stw	r24,-32(r11)
_savegpr_25_l:	stw	r25,-28(r11)
_savegpr_26_l:	stw	r26,-24(r11)
_savegpr_27_l:	stw	r27,-20(r11)
_savegpr_28_l:	stw	r28,-16(r11)
_savegpr_29_l:	stw	r29,-12(r11)
_savegpr_30_l:	stw	r30,-8(r11)
_savegpr_31_l:	stw	r31,-4(r11)
		stw	r0,4(r11)
		blr
