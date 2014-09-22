/* arm_call_via.s - _call_via_rX routines for Thumb mode */

/* Copyright 2001-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,30apr02,to   add .text
01b,22apr02,sn   SPR 76106 - added marker symbol
01a,05dec01,to   written
*/

#define _ASMLANGUAGE
#include "vxWorks.h"
#include "asm.h"

	.data
	.globl __arm_call_via_o
__arm_call_via_o:
	.long 0x0
	
#define	CALL_VIA(reg) \
	.globl	_call_via_##reg	;\
	.thumb_func		;\
_call_via_##reg##:		;\
	bx	reg

#define	CALL_VIA2(reg1, reg2) \
	.globl	_call_via_##reg1;\
	.globl	_call_via_##reg2;\
	.thumb_func		;\
_call_via_##reg1##:		;\
_call_via_##reg2##:		;\
	bx	reg1

#if (ARM_THUMB == TRUE)
	.text
	.code 16
	.balign 2
	CALL_VIA(r0)
	CALL_VIA(r1)
	CALL_VIA(r2)
	CALL_VIA(r3)
	CALL_VIA(r4)
	CALL_VIA(r5)
	CALL_VIA(r6)
	CALL_VIA(r7)
	CALL_VIA(r8)
	CALL_VIA(r9)
	CALL_VIA2(r10, sl)
	CALL_VIA2(r11, fp)
	CALL_VIA2(r12, ip)
	CALL_VIA(sp)
	CALL_VIA(lr)
#endif /* (ARM_THUMB == TRUE) */

	.end
