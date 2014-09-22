/* fphyp32.s - VxWorks conversion from Microtek tools to Sun native */

/* Copyright 1984-1992 Wind River Systems, Inc. */

/*
modification history
--------------------
01a,06aug92,jwt  converted.
*/

/*
! ! ! ! !
!	SPARClite		Floating Point Library
!
!	Copyright (C) 1992 By
!	United States Software Corporation
!	14215 N.W. Science Park Drive
!	Portland, Oregon 97229
!
!	This software is furnished under a license and may be used
!	and copied only in accordance with the terms of such license
!	and with the inclusion of the above copyright notice.
!	This software or any other copies thereof may not be provided
!	or otherwise made available to any other person.  No title to
!	and ownership of the software is hereby transferred.
!
!	The information in this software is subject to change without 
!	notice and should not be construed as a commitment by United
!	States Software Corporation.
!
!	First Release:		V1.02	March 26 1992
! ! ! ! !

	.macro	scan,p1,p2,p3
	.word	0x81602000 | (&p3<<25) | (&p1<<14) | p2
	.endm
	.macro	umul,p1,p2,p3
	.word	0x80500000 | (&p3<<25) | (&p1<<14) | &p2
	.endm
	.macro	divscc,p1,p2,p3
	.word	0x80e80000 | (&p3<<25) | (&p1<<14) | &p2
	.endm
*/

#include "arch/sparc/ussSun4.h"

	.global	_fpexexp

	.data
	.align	4

sihcof:	.word	0x171e
	.word	0x68068
	.word	0x1111111
	.word	0x15555555
	.word	0x80000000
tahcof:	.word	0x1d7
	.word	0xffffedd8
	.word	0xb354
	.word	0xfff910a2
	.word	0x4559ab
	.word	0xfd27d27d
	.word	0x2aaaaaab
	.word	0x80000000

	.text
	.align	4

	.global	_fpcosh
_fpcosh:
	.global	_coshf
_coshf:
	save	%sp,-96,%sp

	sra     %i0,23,%i2
	and     %i2,0xff,%i2
	sll     %i0,8,%i1
	sethi   %hi(0x80000000),%l5
	or      %i1,%l5,%i1
	or      %g0,0,%i3
	subcc   %i2,0xff,%g0
	be      Aspec          

	or      %g0,%i1,%o0
	or      %g0,%i2,%o1
	call    _fpexexp
	or      %g0,%i3,%o2
	orcc    %g0,%o0,%i1
	or      %g0,%o1,%i2

	be      Ainf     
	or      %g0,%o2,%i3
	sethi   %hi(0x40000000),%i4
	add     %i2,%i2,%l1
	sub     %l1,0xfe,%l1

	wr	%i4,0,%y
	or	%g0,0x0,%i4
	orcc	%g0,0,%g0
#if 0
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
#else
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
#endif
	subcc	%l1,32,%g0
	bcc,a	L1             
	or	%g0,0,%i4
L1:	srl	%i4,%l1,%i4
	addcc   %i1,%i4,%i1
	bcc     i1       
	NOP
	srl     %i1,1,%i1
	add     %i2,1,%i2
i1:	sub     %i2,1,%i2
	subcc   %i2,0xff,%g0
	bcc     Ainf     
	NOP
	sll     %i1,1,%i1
	srl     %i1,9,%i1
	sll     %i2,23,%l5
	or      %i1,%l5,%i1
	or      %g0,%i1,%i0

A999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

Aspec:	addcc   %i1,%i1,%l5
	be      Ainf     
	NOP
	ba      A999     
	sethi   %hi(0xffc00000),%i0

Ainf:	ba      A999     
	sethi   %hi(0x7f800000),%i0

	.global	_fpsinh
_fpsinh:
	.global	_sinhf
_sinhf:
	save	%sp,-96,%sp

	sra     %i0,23,%i2
	and     %i2,0xff,%i2
	sethi   %hi(0x80000000),%l2
	sll     %i0,8,%i1
	or      %i1,%l2,%i1
	or      %g0,0,%i3
	and     %l2,%i0,%l2
	subcc   %i2,0xff,%g0
	be      Bspec          
	subcc   %i2,0x7e,%g0
	bcc     i3       
	NOP
	umul	%i1,%i1,%i4
	rd	%y,%i4
	sethi   %hi(sihcof),%l1
	or	%l1,%lo(sihcof),%l1
	or      %g0,0x7e,%l3
	sub     %l3,%i2,%l3
	sll     %l3,1,%l3
	subcc	%l3,32,%g0
	bcc,a	L2             
	or	%g0,0,%i4
L2:	srl	%i4,%l3,%i4
	ld      [%l1],%l5
	add     %l1,4,%l1

b4:
	umul	%i4,%l5,%l5
	rd	%y,%l5
	ld      [%l1],%l4
	add     %l1,4,%l1
	addcc   %l5,%l4,%l5
	bpos    b4       
	NOP
	add     %i2,1,%i2
	umul	%i1,%l5,%i1
	rd	%y,%i1
	subcc   %i1,0,%i1
	ble     e3       
	NOP
	sll     %i1,1,%i1
	ba      e3       
	sub     %i2,1,%i2

i3:	or      %g0,%i1,%o0
	or      %g0,%i2,%o1
	call    _fpexexp
	or      %g0,%i3,%o2
	orcc    %g0,%o0,%i1
	or      %g0,%o1,%i2

	be      Binf     
	or      %g0,%o2,%i3
	sethi   %hi(0x40000000),%i4
	add     %i2,%i2,%l3
	sub     %l3,0xfe,%l3

	wr	%i4,0,%y
	or	%g0,0x0,%i4
	orcc	%g0,0,%g0
#if 0
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
#else
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
#endif
	subcc	%l3,32,%g0
	bcc,a	L3             
	or	%g0,0,%i4
L3:	srl	%i4,%l3,%i4
	subcc   %i1,%i4,%i1
/*	scan	%i1,0,%l3	*/
	SCAN | (in1 << 14) | 0 | (lo3 << 25)
	sll	%i1,%l3,%i1
	subcc	%i2,%l3,%i2
	sub     %i2,1,%i2
	subcc   %i2,0xff,%g0
	bcc     Binf     
	NOP
e3:	sll     %i1,1,%i1
	srl     %i1,9,%i1
	sll     %i2,23,%l7
	or      %i1,%l7,%i1
	or      %i1,%l2,%i0

B999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

Bspec:	addcc   %i1,%i1,%l7
	be      Binf     
	NOP
	ba      B999     
	sethi   %hi(0xffc00000),%i0

Binf:	sethi   %hi(0x7f800000),%l7
	ba      B999     
	or      %l7,%l2,%i0

	.global	_fptanh
_fptanh:
	.global	_tanhf
_tanhf:
	save	%sp,-96,%sp

	sra     %i0,23,%i2
	and     %i2,0xff,%i2
	sethi   %hi(0x80000000),%l1
	sll     %i0,8,%i1
	or      %i1,%l1,%i1
	or      %g0,0,%i3
	and     %l1,%i0,%l1
	subcc   %i2,0xff,%g0
	be      Cspec          
	subcc   %i2,0x7e,%g0
	bcc     i7       
	NOP
	umul	%i1,%i1,%l4
	rd	%y,%l4
	sethi   %hi(tahcof),%l6
	or	%l6,%lo(tahcof),%l6
	or      %g0,0x7e,%l2
	sub     %l2,%i2,%l2
	sll     %l2,1,%l2
	subcc	%l2,32,%g0
	bcc,a	L4             
	or	%g0,0,%l4
L4:	srl	%l4,%l2,%l4
	ld      [%l6],%i4
	add     %l6,4,%l6

b8:	sra     %i4,31,%l3
	xor     %i4,%l3,%i4
	sub     %i4,%l3,%i4
	umul	%l4,%i4,%i4
	rd	%y,%i4
	xor     %i4,%l3,%i4
	sub     %i4,%l3,%i4
	ld      [%l6],%l3
	add     %l6,4,%l6
	add     %i4,%l3,%i4
	addcc   %l3,%l3,%l7
	bne     b8             
	subcc   %i4,0,%i4
	ble     i9       
	NOP
	sll     %i4,1,%i4
	add     %i2,1,%i2
i9:	ba      e7       
	srl     %i1,1,%l2
i7:	subcc   %i2,0x88,%g0
	bgu     Cone           

	or      %g0,%i1,%o0
	or      %g0,%i2,%o1
	call    _fpexexp
	or      %g0,%i3,%o2
	or      %g0,%o0,%i1
	or      %g0,%o1,%i2
	or      %g0,%o2,%i3

	sethi   %hi(0x40000000),%i4
	add     %i2,%i2,%l2
	sub     %l2,0xfe,%l2

	wr	%i4,0,%y
	or	%g0,0x0,%i4
	orcc	%g0,0,%g0
#if 0
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
	divscc	%i4,%i1,%i4
#else
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
	DIVSCC | (in4 << 14) | in1 | (in4 << 25)
#endif
	subcc	%l2,32,%g0
	bcc,a	L5             
	or	%g0,0,%i4
L5:	srl	%i4,%l2,%i4
	or      %g0,0x7e,%i2
	subcc   %i1,%i4,%l2
	bpos    i10      
	NOP
	srl     %l2,1,%l2
	add     %i2,1,%i2
i10:	addcc   %i4,%i1,%i4
	bcc     e7       
	NOP
	srl     %i4,1,%i4
	sethi   %hi(0x80000000),%l7
	or      %i4,%l7,%i4
	sub     %i2,1,%i2

e7:	wr	%l2,0,%y
	or	%g0,0x0,%i1
	orcc	%g0,0,%g0
#if 0
	divscc	%i1,%i4,%i1
	divscc	%i1,%i4,%i1
	divscc	%i1,%i4,%i1
	divscc	%i1,%i4,%i1
	divscc	%i1,%i4,%i1
	divscc	%i1,%i4,%i1
	divscc	%i1,%i4,%i1
	divscc	%i1,%i4,%i1
	divscc	%i1,%i4,%i1
	divscc	%i1,%i4,%i1
	divscc	%i1,%i4,%i1
	divscc	%i1,%i4,%i1
	divscc	%i1,%i4,%i1
	divscc	%i1,%i4,%i1
	divscc	%i1,%i4,%i1
	divscc	%i1,%i4,%i1
	divscc	%i1,%i4,%i1
	divscc	%i1,%i4,%i1
	divscc	%i1,%i4,%i1
	divscc	%i1,%i4,%i1
	divscc	%i1,%i4,%i1
	divscc	%i1,%i4,%i1
	divscc	%i1,%i4,%i1
	divscc	%i1,%i4,%i1
	divscc	%i1,%i4,%i1
	divscc	%i1,%i4,%i1
	divscc	%i1,%i4,%i1
	divscc	%i1,%i4,%i1
	divscc	%i1,%i4,%i1
	divscc	%i1,%i4,%i1
	divscc	%i1,%i4,%i1
	divscc	%i1,%i4,%i1
#else
	DIVSCC | (in1 << 14) | in4 | (in1 << 25)
	DIVSCC | (in1 << 14) | in4 | (in1 << 25)
	DIVSCC | (in1 << 14) | in4 | (in1 << 25)
	DIVSCC | (in1 << 14) | in4 | (in1 << 25)
	DIVSCC | (in1 << 14) | in4 | (in1 << 25)
	DIVSCC | (in1 << 14) | in4 | (in1 << 25)
	DIVSCC | (in1 << 14) | in4 | (in1 << 25)
	DIVSCC | (in1 << 14) | in4 | (in1 << 25)
	DIVSCC | (in1 << 14) | in4 | (in1 << 25)
	DIVSCC | (in1 << 14) | in4 | (in1 << 25)
	DIVSCC | (in1 << 14) | in4 | (in1 << 25)
	DIVSCC | (in1 << 14) | in4 | (in1 << 25)
	DIVSCC | (in1 << 14) | in4 | (in1 << 25)
	DIVSCC | (in1 << 14) | in4 | (in1 << 25)
	DIVSCC | (in1 << 14) | in4 | (in1 << 25)
	DIVSCC | (in1 << 14) | in4 | (in1 << 25)
	DIVSCC | (in1 << 14) | in4 | (in1 << 25)
	DIVSCC | (in1 << 14) | in4 | (in1 << 25)
	DIVSCC | (in1 << 14) | in4 | (in1 << 25)
	DIVSCC | (in1 << 14) | in4 | (in1 << 25)
	DIVSCC | (in1 << 14) | in4 | (in1 << 25)
	DIVSCC | (in1 << 14) | in4 | (in1 << 25)
	DIVSCC | (in1 << 14) | in4 | (in1 << 25)
	DIVSCC | (in1 << 14) | in4 | (in1 << 25)
	DIVSCC | (in1 << 14) | in4 | (in1 << 25)
	DIVSCC | (in1 << 14) | in4 | (in1 << 25)
	DIVSCC | (in1 << 14) | in4 | (in1 << 25)
	DIVSCC | (in1 << 14) | in4 | (in1 << 25)
	DIVSCC | (in1 << 14) | in4 | (in1 << 25)
	DIVSCC | (in1 << 14) | in4 | (in1 << 25)
	DIVSCC | (in1 << 14) | in4 | (in1 << 25)
	DIVSCC | (in1 << 14) | in4 | (in1 << 25)
#endif

	subcc   %i1,0,%i1
	ble     i12      
	NOP
	sll     %i1,1,%i1
	sub     %i2,1,%i2
i12:	sll     %i1,1,%i1
	srl     %i1,9,%i1
	sll     %i2,23,%l7
	or      %i1,%l7,%i1
	or      %i1,%l1,%i0

C999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

Cspec:	addcc   %i1,%i1,%l7
	be      Cone     
	NOP
	ba      C999     
	sethi   %hi(0xffc00000),%i0

Cone:	sethi   %hi(0x3f800000),%l7
	ba      C999     
	or      %l7,%l1,%i0

!	.end
