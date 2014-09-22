/* fpsqrt32.s - VxWorks conversion from Microtek tools to Sun native */

/* Copyright 1984-1992 Wind River Systems, Inc. */

/*
modification history
--------------------
01b,13nov92,jwt  added workaround for sqrt(-0.0) until USS patch.
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

	.text
	.align	4

	.global	_fpexsqrt
_fpexsqrt:
	save	%sp,-96,%sp

	subcc   %i0,0,%i0
	be      Azer           
	andcc   %i1,1,%l3
	be      i1       
	NOP
	sethi   %hi(0x67f21800),%i5
	umul	%i0,%i5,%i5
	rd	%y,%i5
	sethi   %hi(0x4d134800),%l3
	add     %i5,%l3,%i5
	srl     %i0,1,%i0
	ba      e1       
	add     %i1,1,%i1
i1:	sethi   %hi(0x93000000),%i5
	umul	%i0,%i5,%i5
	rd	%y,%i5
	sethi   %hi(0x6cfffc00),%l3
	add     %i5,%l3,%i5
e1:	sra     %i1,1,%i3
	add     %i3,0x3f,%i3

	srl     %i0,1,%i0

	wr	%i0,0,%y
	or	%g0,0x0,%l0
	orcc	%g0,0,%g0
#if 0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
#else
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
#endif

	srl     %i5,1,%i5
	addcc   %i5,%l0,%i5
	subx    %i5,0,%i5

	wr	%i0,0,%y
	or	%g0,0x0,%l0
	orcc	%g0,0,%g0
#if 0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
	divscc	%l0,%i5,%l0
#else
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in5 | (lo0 << 25)
#endif

	srl     %i5,1,%i5
	addcc   %i5,%l0,%i5
	subx    %i5,0,%i5

	or      %g0,%i5,%i2

Aret:	or      %g0,%i2,%i0
	or      %g0,%i3,%i1
	or      %g0,%i4,%i2

A999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

Azer:	or      %g0,0,%i3
	ba      Aret     
	or      %g0,%i3,%i2

	.global	_fpsqrt
_fpsqrt:
	.global	_sqrtf
_sqrtf:
	save	%sp,-96,%sp

	/* Test for negative zero */

	set	0xFF800000,%l0		/* sign (31) and exponent (30-23) */
	and	%l0,%i0,%l0
	set	0x80000000,%l1		/* negative zero */
	cmp	%l0,%l1
	bne	sqrtNonZero
	nop

	ret				/* return value = -0.0 */
	restore

sqrtNonZero:

	srl     %i0,23,%i4
	sll     %i0,8,%i3
	sethi   %hi(0x80000000),%l3
	or      %i3,%l3,%i3
	sub     %i4,1,%l3
	subcc   %l3,0xfe,%g0
	bcc     Bspec          

Blab1:	or      %g0,%i3,%o0
	call    _fpexsqrt
	or      %g0,%i4,%o1
	or      %g0,%o0,%l0
	or      %g0,%o1,%l1
	or      %g0,%o2,%l2

	andcc   %i4,1,%l3
	bne,a	i2               
	srl     %i3,1,%i3
i2:
	umul	%l0,%l0,%i2
	rd	%y,%i1
	sub     %i1,%i3,%i1
	orcc    %i1,%i2,%l3
	be      Bret           
	subcc   %i1,0,%i1
	bneg    Bloo1    
	NOP

Bloo2:	sub     %l0,1,%l0
	addcc   %i2,1,%i2
	addx    %i1,0,%i1
	subcc   %i2,%l0,%i2
	subx    %i1,0,%i1
	subcc   %i2,%l0,%i2
	subxcc  %i1,0,%i1
	bpos    Bloo2    
	NOP
	ba,a    Bret     

Bloo1:	addcc   %l0,1,%l0
	bcs     Blab3    
	NOP
	addcc   %i2,%l0,%i2
	addx    %i1,0,%i1
	addcc   %i2,%l0,%i2
	addx    %i1,0,%i1
	addcc   %i2,1,%i2
	addxcc  %i1,0,%i1
	bneg    Bloo1    
	NOP

Blab3:	sub     %l0,1,%l0

Bret:	addcc   %l0,0x80,%l0
	addx    %l1,0,%l1
	sll     %l0,1,%i0
	srl     %i0,9,%i0
	sll     %l1,23,%l3
	or      %i0,%l3,%i0

B999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

Bspec:	subcc   %i0,0,%i0
	bneg    Bnan           
	subcc   %i4,0,%i4
	bne     i4       
	NOP
	addcc   %i3,%i3,%i3
	be      i5       
	NOP
/*	scan	%i3,0,%i1	*/
	SCAN | (in3 << 14) | 0 | (in1 << 25)
	sll	%i3,%i1,%i3
	subcc	%i4,%i1,%i4
	ba,a    Blab1    
i5:	ba      B999     
	or      %g0,0,%i0
i4:	addcc   %i3,%i3,%l3
	bne     Bnan     
	NOP
	ba,a    B999     

Bnan:	ba      B999     
	sethi   %hi(0xffc00000),%i0

!	.end
