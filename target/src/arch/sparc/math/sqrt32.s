/* sqrt32.s - VxWorks conversion from Microtek tools to Sun native */

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

	.global	_exsqrt
_exsqrt:
	save	%sp,-96,%sp

	subcc   %i0,0,%i0
	be      Aret           
	andcc   %i2,1,%l5
	be      i1       
	NOP
	sethi   %hi(0x67f21800),%l0
	umul	%i0,%l0,%l0
	rd	%y,%l0
	sethi   %hi(0x4d134800),%l5
	add     %l0,%l5,%l0
	srl     %i1,1,%i1
	sll     %i0,31,%l5
	srl     %i0,1,%i0
	or      %i1,%l5,%i1
	ba      e1       
	add     %i2,1,%i2
i1:	sethi   %hi(0x93000000),%l0
	umul	%i0,%l0,%l0
	rd	%y,%l0
	sethi   %hi(0x6cfffc00),%l5
	add     %l0,%l5,%l0
e1:	sra     %i2,1,%i2
	add     %i2,0x1ff,%i2
	srl     %i1,1,%i1
	sll     %i0,31,%l5
	srl     %i0,1,%i0
	or      %i1,%l5,%i1

	wr	%i0,0,%y
	or	%g0,%i1,%i3
	orcc	%g0,0,%g0
#if 0
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
#else
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
#endif

	srl     %l0,1,%l0
	addcc   %l0,%i3,%l0
	subx    %l0,0,%l0

	wr	%i0,0,%y
	or	%g0,%i1,%i3
	orcc	%g0,0,%g0
#if 0
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
#else
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
#endif

	srl     %l0,1,%l0
	addcc   %l0,%i3,%l0
	subx    %l0,0,%l0

	wr	%i0,0,%y
	or	%g0,%i1,%i3
	orcc	%g0,0,%g0
#if 0
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
	divscc	%i3,%l0,%i3
#else
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
	DIVSCC | (in3 << 14) | lo0 | (in3 << 25)
#endif
	bge	L1       
	 rd	%y,%l4
	addcc	%l0,%l4,%l4

L1:	wr	%l4,0,%y
	or	%g0,0x0,%i4
	orcc	%g0,0,%g0
#if 0
	divscc	%i4,%l0,%i4
	divscc	%i4,%l0,%i4
	divscc	%i4,%l0,%i4
	divscc	%i4,%l0,%i4
	divscc	%i4,%l0,%i4
	divscc	%i4,%l0,%i4
	divscc	%i4,%l0,%i4
	divscc	%i4,%l0,%i4
	divscc	%i4,%l0,%i4
	divscc	%i4,%l0,%i4
	divscc	%i4,%l0,%i4
	divscc	%i4,%l0,%i4
	divscc	%i4,%l0,%i4
	divscc	%i4,%l0,%i4
	divscc	%i4,%l0,%i4
	divscc	%i4,%l0,%i4
	divscc	%i4,%l0,%i4
	divscc	%i4,%l0,%i4
	divscc	%i4,%l0,%i4
	divscc	%i4,%l0,%i4
	divscc	%i4,%l0,%i4
	divscc	%i4,%l0,%i4
	divscc	%i4,%l0,%i4
	divscc	%i4,%l0,%i4
	divscc	%i4,%l0,%i4
	divscc	%i4,%l0,%i4
	divscc	%i4,%l0,%i4
	divscc	%i4,%l0,%i4
	divscc	%i4,%l0,%i4
	divscc	%i4,%l0,%i4
	divscc	%i4,%l0,%i4
	divscc	%i4,%l0,%i4
#else
	DIVSCC | (in4 << 14) | lo0 | (in4 << 25)
	DIVSCC | (in4 << 14) | lo0 | (in4 << 25)
	DIVSCC | (in4 << 14) | lo0 | (in4 << 25)
	DIVSCC | (in4 << 14) | lo0 | (in4 << 25)
	DIVSCC | (in4 << 14) | lo0 | (in4 << 25)
	DIVSCC | (in4 << 14) | lo0 | (in4 << 25)
	DIVSCC | (in4 << 14) | lo0 | (in4 << 25)
	DIVSCC | (in4 << 14) | lo0 | (in4 << 25)
	DIVSCC | (in4 << 14) | lo0 | (in4 << 25)
	DIVSCC | (in4 << 14) | lo0 | (in4 << 25)
	DIVSCC | (in4 << 14) | lo0 | (in4 << 25)
	DIVSCC | (in4 << 14) | lo0 | (in4 << 25)
	DIVSCC | (in4 << 14) | lo0 | (in4 << 25)
	DIVSCC | (in4 << 14) | lo0 | (in4 << 25)
	DIVSCC | (in4 << 14) | lo0 | (in4 << 25)
	DIVSCC | (in4 << 14) | lo0 | (in4 << 25)
	DIVSCC | (in4 << 14) | lo0 | (in4 << 25)
	DIVSCC | (in4 << 14) | lo0 | (in4 << 25)
	DIVSCC | (in4 << 14) | lo0 | (in4 << 25)
	DIVSCC | (in4 << 14) | lo0 | (in4 << 25)
	DIVSCC | (in4 << 14) | lo0 | (in4 << 25)
	DIVSCC | (in4 << 14) | lo0 | (in4 << 25)
	DIVSCC | (in4 << 14) | lo0 | (in4 << 25)
	DIVSCC | (in4 << 14) | lo0 | (in4 << 25)
	DIVSCC | (in4 << 14) | lo0 | (in4 << 25)
	DIVSCC | (in4 << 14) | lo0 | (in4 << 25)
	DIVSCC | (in4 << 14) | lo0 | (in4 << 25)
	DIVSCC | (in4 << 14) | lo0 | (in4 << 25)
	DIVSCC | (in4 << 14) | lo0 | (in4 << 25)
	DIVSCC | (in4 << 14) | lo0 | (in4 << 25)
	DIVSCC | (in4 << 14) | lo0 | (in4 << 25)
	DIVSCC | (in4 << 14) | lo0 | (in4 << 25)
#endif

	sll     %l0,31,%l1
	srl     %l0,1,%l0
	addcc   %i4,%l1,%i4
	addx    %i3,%l0,%i3

	or      %g0,%i3,%i0
	or      %g0,%i4,%i1

Aret:
A999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

	.global	_dpsqrt
_dpsqrt:
	.global	_sqrt
_sqrt:
	save	%sp,-96,%sp

	/* Test for negative zero */

	set	0xFFF00000,%l0		/* sign (31) and exponent (30-20) */
	and	%l0,%i0,%l0
	set	0x80000000,%l1		/* negative zero */
	cmp	%l0,%l1
	bne	sqrtNonZero
	nop

	ret				/* return value = -0.0 */
	restore

sqrtNonZero:

	srl     %i0,20,%i4
	sll     %i0,11,%i2
	srl     %i1,21,%l6
	or      %i2,%l6,%i2
	sll     %i1,11,%i3
	sethi   %hi(0x80000000),%l6
	or      %i2,%l6,%i2
	sub     %i4,1,%l6
	subcc   %l6,0x7fe,%g0
	bcc     Bspec          

Blab1:	or      %g0,%i2,%o0
	or      %g0,%i3,%o1
	call    _exsqrt
	or      %g0,%i4,%o2
	or      %g0,%o0,%i5
	or      %g0,%o1,%l0
	or      %g0,%o2,%l1

	andcc   %i4,1,%l6
	be      i2       
	NOP
	srl     %i3,1,%i3
	sll     %i2,31,%l6
	srl     %i2,1,%i2
	or      %i3,%l6,%i3
i2:
	umul	%i5,%l0,%l3
	rd	%y,%l2
	addcc   %l3,%l3,%l3
	addx    %l2,%l2,%l2
	umul	%i5,%i5,%l5
	rd	%y,%l4
	add     %l2,%l5,%l2
	umul	%l0,%l0,%l4
	rd	%y,%l5
	addcc   %l3,%l5,%l3
	addx    %l2,0,%l2
	sub     %l2,%i3,%l2
	or      %l2,%l3,%l6
	orcc    %l6,%l4,%l6
	be      Bret           
	subcc   %l2,0,%l2
	bneg    Bloo1    
	NOP

Bloo2:	subcc   %l0,1,%l0
	subx    %i5,0,%i5
	addcc   %l4,1,%l4
	addxcc  %l3,0,%l3
	addx    %l2,0,%l2
	subcc   %l4,%l0,%l4
	subxcc  %l3,%i5,%l3
	subx    %l2,0,%l2
	subcc   %l4,%l0,%l4
	subxcc  %l3,%i5,%l3
	subxcc  %l2,0,%l2
	bpos    Bloo2    
	NOP
	ba,a    Bret     

Bloo1:	addcc   %l0,1,%l0
	addxcc  %i5,0,%i5
	bcs     Blab3    
	NOP
	addcc   %l4,%l0,%l4
	addxcc  %l3,%i5,%l3
	addx    %l2,0,%l2
	addcc   %l4,%l0,%l4
	addxcc  %l3,%i5,%l3
	addx    %l2,0,%l2
	addcc   %l4,1,%l4
	addxcc  %l3,0,%l3
	addxcc  %l2,0,%l2
	bneg    Bloo1    
	NOP

Blab3:	subcc   %l0,1,%l0
	subx    %i5,0,%i5

Bret:	addcc   %l0,0x400,%l0
	addxcc  %i5,0,%i5
	addx    %l1,0,%l1
	srl     %l0,11,%i1
	sll     %i5,21,%l6
	or      %i1,%l6,%i1
	srl     %i5,11,%i0
	sethi   %hi(0xfffff),%l6
	or      %l6,0x3ff,%l6
	and     %i0,%l6,%i0
	sll     %l1,20,%l6
	or      %i0,%l6,%i0

B999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

Bspec:	subcc   %i0,0,%i0
	bneg    Bnan           
	subcc   %i4,0,%i4
	bne     i4       
	NOP
	addcc   %i3,%i3,%i3
	addx    %i2,%i2,%i2
	orcc    %i2,%i3,%l6
	be      Bzer     
	NOP

/*	scan	%i2,0,%l2	*/
	SCAN | (in2 << 14) | 0 | (lo2 << 25)
	subcc	%l2,63,%g0
	bne	L2             
	NOP
	or	%i3,0,%i2
	or	%g0,0,%i3
	sub	%i4,32,%i4
/*	scan	%i2,0,%l2	*/
	SCAN | (in2 << 14) | 0 | (lo2 << 25)
	subcc	%l2,63,%g0
	be,a	L2             
	or	%l2,0,%i4
L2:	subcc	%g0,%l2,%l3
	be	Blab1          
	sub	%i4,%l2,%i4
	sll	%i2,%l2,%i2
	srl	%i3,%l3,%l3
	or	%l3,%i2,%i2
	sll	%i3,%l2,%i3

	ba,a    Blab1    
i4:	sll     %i2,1,%l6
	orcc    %l6,%i3,%l6
	bne     Bnan     
	NOP
	ba,a    B999     

Bnan:	sethi   %hi(0xfff80000),%i0
	ba      B999     
	or      %g0,0,%i1

Bzer:	or      %g0,0,%i1
	ba      B999     
	or      %g0,%i1,%i0

!	.end
