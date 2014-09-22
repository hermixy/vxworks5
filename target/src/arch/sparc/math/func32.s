/* func32.s - VxWorks conversion from Microtek tools to Sun native */

/* Copyright 1984-1992 Wind River Systems, Inc. */

/*
modification history
--------------------
01b,10nov92,jwt  added two missing DIVSCC instructions in exdiv().
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

	.global	_sigmax
_sigmax:
	save	%sp,-96,%sp

	or      %g0,0x3fe,%l4
	sub     %l4,%i2,%l4

L1:	subcc	%l4,32,%g0
	bcs	L2             
	subcc	%g0,%l4,%l5
	sub	%l4,32,%l4
	orcc	%i0,0,%i1
	bne	L1             
	or	%g0,0,%i0
L2:	be	L3             
	sll	%i0,%l5,%l5
	srl	%i1,%l4,%i1
	srl	%i0,%l4,%i0
	or	%i1,%l5,%i1

L3:	ld      [%i4],%l3
	add     %i4,4,%i4
	ld      [%i4],%l2
	add     %i4,4,%i4

b1:	xor     %l2,%i3,%l6
	subcc   %l2,0,%l2
	bpos    i2       
	NOP
	subcc   %g0,%l3,%l3
	subx    %g0,%l2,%l2
i2:
	umul	%i0,%l3,%l3
	rd	%y,%l3
	umul	%i0,%l2,%l5
	rd	%y,%l0
	addcc   %l3,%l5,%l3
	addx    %l0,0,%l0
	umul	%i1,%l2,%l4
	rd	%y,%l4
	addcc   %l3,%l4,%l3
	addx    %l0,0,%l0
	or      %g0,%l0,%l2
	subcc   %l6,0,%l6
	bpos    i3       
	NOP
	subcc   %g0,%l3,%l3
	subx    %g0,%l2,%l2
i3:	ld      [%i4],%l1
	add     %i4,4,%i4
	ld      [%i4],%l0
	add     %i4,4,%i4
	addcc   %l3,%l1,%l3
	addx    %l2,%l0,%l2
	sethi   %hi(0x80000000),%l7
	subcc   %l0,%l7,%g0
	bne     b1       
	NOP

	or      %g0,0x3ff,%i2
	subcc   %l2,0,%l2
	bneg    i4       
	NOP
	addcc   %l3,%l3,%l3
	addx    %l2,%l2,%l2
	sub     %i2,1,%i2
i4:	or      %g0,%l3,%i1
	or      %g0,%l2,%i0

A999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

	.global	_exmul
_exmul:
	save	%sp,-96,%sp
	umul	%i0,%i3,%l1
	rd	%y,%l0
	umul	%i0,%i4,%l3
	rd	%y,%l2
	addcc   %l1,%l2,%l1
	addx    %l0,0,%l0
	umul	%i1,%i3,%l4
	rd	%y,%l2
	addcc   %l4,%l3,%l4
	addxcc  %l1,%l2,%l1
	addxcc  %l0,0,%l0
	bneg    i5       
	NOP
	be      Bzer     
	NOP
	addcc   %l4,%l4,%l4
	addxcc  %l1,%l1,%l1
	addx    %l0,%l0,%l0
	sub     %i2,1,%i2
i5:	addcc   %l4,%l4,%l4
	addxcc  %l1,0,%l1
	addx    %l0,0,%l0
	add     %i2,%i5,%i2
	sub     %i2,0x3fe,%i2
	or      %g0,%l1,%i1
	or      %g0,%l0,%i0

Bret:
B999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

Bzer:	or      %g0,0,%i2
	or      %g0,%i2,%i0
	ba      Bret     
	or      %g0,%i2,%i1


	.global	_exadd
_exadd:
	save	%sp,-96,%sp

	sub     %i2,%i5,%l0
	subcc   %l0,0,%l0
	bneg    i6       
	NOP

L4:	subcc	%l0,32,%g0
	bcs	L5             
	subcc	%g0,%l0,%l1
	sub	%l0,32,%l0
	orcc	%i3,0,%i4
	bne	L4             
	or	%g0,0,%i3
L5:	be	e6             
	sll	%i3,%l1,%l1
	srl	%i4,%l0,%i4
	srl	%i3,%l0,%i3
	or	%i4,%l1,%i4
	ba,a    e6       

i6:	or      %g0,%i5,%i2
	sub     %g0,%l0,%l0

L7:	subcc	%l0,32,%g0
	bcs	L8             
	subcc	%g0,%l0,%l1
	sub	%l0,32,%l0
	orcc	%i0,0,%i1
	bne	L7             
	or	%g0,0,%i0
L8:	be	e6             
	sll	%i0,%l1,%l1
	srl	%i1,%l0,%i1
	srl	%i0,%l0,%i0
	or	%i1,%l1,%i1

e6:	addcc   %i1,%i4,%i1
	addxcc  %i0,%i3,%i0
	bcc     i7       
	NOP
	srl     %i1,1,%i1
	sll     %i0,31,%l2
	srl     %i0,1,%i0
	or      %i1,%l2,%i1
	sethi   %hi(0x80000000),%l2
	or      %i0,%l2,%i0
	add     %i2,1,%i2

i7:
C999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0


	.global	_exsub
_exsub:
	save	%sp,-96,%sp

	sub     %i2,%i5,%l0

L10:	subcc	%l0,32,%g0
	bcs	L11             
	subcc	%g0,%l0,%l1
	sub	%l0,32,%l0
	orcc	%i3,0,%i4
	bne	L10             
	or	%g0,0,%i3
L11:	be	L12             
	sll	%i3,%l1,%l1
	srl	%i4,%l0,%i4
	srl	%i3,%l0,%i3
	or	%i4,%l1,%i4

L12:	subcc   %i1,%i4,%i1
	subx    %i0,%i3,%i0

/*	scan	%i0,0,%l0	*/
	SCAN | (in0 << 14) | 0 | (lo0 << 25)
	subcc	%l0,63,%g0
	bne	L13             
	NOP
	or	%i1,0,%i0
	or	%g0,0,%i1
	sub	%i2,32,%i2
/*	scan	%i0,0,%l0	*/
	SCAN | (in0 << 14) | 0 | (lo0 << 25)
	subcc	%l0,63,%g0
	be,a	L13             
	or	%l0,0,%i2
L13:	subcc	%g0,%l0,%l1
	be	L14             
	sub	%i2,%l0,%i2
	sll	%i0,%l0,%i0
	srl	%i1,%l1,%l1
	or	%l1,%i0,%i0
	sll	%i1,%l0,%i1

L14:
D999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

	.global	_exdiv
_exdiv:
	save	%sp,-96,%sp

	subcc   %i3,0,%i3
	be      Einf     
	NOP
	srl     %i1,1,%i1
	sll     %i0,31,%l5
	srl     %i0,1,%i0
	or      %i1,%l5,%i1

	wr	%i0,0,%y
	or	%g0,%i1,%l0
	orcc	%g0,0,%g0
#if 0
	divscc	%l0,%i3,%l0
	divscc	%l0,%i3,%l0
	divscc	%l0,%i3,%l0
	divscc	%l0,%i3,%l0
	divscc	%l0,%i3,%l0
	divscc	%l0,%i3,%l0
	divscc	%l0,%i3,%l0
	divscc	%l0,%i3,%l0
	divscc	%l0,%i3,%l0
	divscc	%l0,%i3,%l0
	divscc	%l0,%i3,%l0
	divscc	%l0,%i3,%l0
	divscc	%l0,%i3,%l0
	divscc	%l0,%i3,%l0
	divscc	%l0,%i3,%l0
	divscc	%l0,%i3,%l0
	divscc	%l0,%i3,%l0
	divscc	%l0,%i3,%l0
	divscc	%l0,%i3,%l0
	divscc	%l0,%i3,%l0
	divscc	%l0,%i3,%l0
	divscc	%l0,%i3,%l0
	divscc	%l0,%i3,%l0
	divscc	%l0,%i3,%l0
	divscc	%l0,%i3,%l0
	divscc	%l0,%i3,%l0
	divscc	%l0,%i3,%l0
	divscc	%l0,%i3,%l0
	divscc	%l0,%i3,%l0
	divscc	%l0,%i3,%l0
	divscc	%l0,%i3,%l0
	divscc	%l0,%i3,%l0
#else
	DIVSCC | (lo0 << 14) | in3 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in3 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in3 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in3 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in3 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in3 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in3 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in3 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in3 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in3 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in3 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in3 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in3 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in3 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in3 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in3 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in3 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in3 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in3 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in3 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in3 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in3 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in3 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in3 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in3 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in3 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in3 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in3 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in3 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in3 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in3 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in3 | (lo0 << 25)
#endif
	bge	L15       
	 rd	%y,%i0
	addcc	%i3,%i0,%i0
L15:
	umul	%l0,%i4,%l3
	rd	%y,%l2
	or      %g0,0,%i1
	subcc   %i1,%l3,%i1
	subxcc  %i0,%l2,%i0
	bcc     i8       
	NOP

Elab3:	sub     %l0,1,%l0
	addcc   %i1,%i4,%i1
	addxcc  %i0,%i3,%i0
	bcc     Elab3          
i8:	subcc   %i0,%i3,%g0
	bne     i9       
	NOP
	or      %g0,%i4,%l2
	or      %g0,%i1,%i0
	or      %g0,0,%l1
	ba      e9       
	or      %g0,%l1,%l3

i9:	wr	%i0,0,%y
	or	%g0,%i1,%l1
	orcc	%g0,0,%g0
#if 0
	divscc	%l1,%i3,%l1
	divscc	%l1,%i3,%l1
	divscc	%l1,%i3,%l1
	divscc	%l1,%i3,%l1
	divscc	%l1,%i3,%l1
	divscc	%l1,%i3,%l1
	divscc	%l1,%i3,%l1
	divscc	%l1,%i3,%l1
	divscc	%l1,%i3,%l1
	divscc	%l1,%i3,%l1
	divscc	%l1,%i3,%l1
	divscc	%l1,%i3,%l1
	divscc	%l1,%i3,%l1
	divscc	%l1,%i3,%l1
	divscc	%l1,%i3,%l1
	divscc	%l1,%i3,%l1
	divscc	%l1,%i3,%l1
	divscc	%l1,%i3,%l1
	divscc	%l1,%i3,%l1
	divscc	%l1,%i3,%l1
	divscc	%l1,%i3,%l1
	divscc	%l1,%i3,%l1
	divscc	%l1,%i3,%l1
	divscc	%l1,%i3,%l1
	divscc	%l1,%i3,%l1
	divscc	%l1,%i3,%l1
	divscc	%l1,%i3,%l1
	divscc	%l1,%i3,%l1
	divscc	%l1,%i3,%l1
	divscc	%l1,%i3,%l1
	divscc	%l1,%i3,%l1
	divscc	%l1,%i3,%l1
#else
	DIVSCC | (lo1 << 14) | in3 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in3 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in3 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in3 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in3 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in3 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in3 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in3 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in3 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in3 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in3 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in3 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in3 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in3 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in3 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in3 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in3 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in3 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in3 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in3 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in3 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in3 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in3 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in3 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in3 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in3 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in3 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in3 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in3 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in3 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in3 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in3 | (lo1 << 25)
#endif
	bge	L16       
	 rd	%y,%i0
	addcc	%i3,%i0,%i0
L16:
	umul	%l1,%i4,%l3
	rd	%y,%l2
e9:	or      %g0,0,%i1
	subcc   %i1,%l3,%i1
	subxcc  %i0,%l2,%i0
	bcc     i10      
	NOP

Elab4:	sub     %l1,1,%l1
	addcc   %i1,%i4,%i1
	addxcc  %i0,%i3,%i0
	bcc     Elab4    
	NOP
i10:	sub     %i2,%i5,%i2
	add     %i2,0x3ff,%i2
	subcc   %l0,0,%l0
	bneg    i11      
	NOP
	addcc   %l1,%l1,%l1
	addx    %l0,%l0,%l0
	sub     %i2,1,%i2
i11:	or      %g0,%l0,%i0
	or      %g0,%l1,%i1

Eret:
E999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

Einf:	or      %g0,0x7ff,%i2
	sethi   %hi(0x80000000),%i0
	ba      Eret     
	or      %g0,0,%i1

!	.end
