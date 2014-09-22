/* fptrig32.s - VxWorks conversion from Microtek tools to Sun native */

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

	.data
	.align	4

sincof:	.word	0x171e
	.word	0xfff97f98
	.word	0x1111111
	.word	0xeaaaaaab
	.word	0x80000000
coscof:	.word	0xd00d
	.word	0xffd27d28
	.word	0x5555555
	.word	0xc0000000
	.word	0x80000000
tancof:	.word	0x1228
	.word	0xb354
	.word	0x6ef5e
	.word	0x4559ab
	.word	0x2d82d83
	.word	0x2aaaaaab

	.text
	.align	4

	.global	_fpreduct
_fpreduct:
	save	%sp,-96,%sp

	sra     %i0,23,%i2
	and     %i2,0xff,%i2
	sll     %i0,8,%i1
	sethi   %hi(0x80000000),%l2
	or      %i1,%l2,%i1
	or      %g0,0,%i3
	subcc   %i2,0x7e,%i4
	bneg    Aret           
	subcc   %i4,0x40,%g0
	bg      Anan     
	NOP
	srl     %i1,1,%i1
	or      %g0,0,%i5
	or      %g0,-1,%i3
	sethi   %hi(0x10b4611a),%l1
	or      %l1,0x11a,%l1
	sethi   %hi(0x6487ed51),%l0
	or      %l0,0x151,%l0

b1:	subcc   %i5,%l1,%i5
	subxcc  %i1,%l0,%i1
	bcc     i2       
	NOP
	addcc   %i5,%l1,%i5
	addxcc  %i1,%l0,%i1
i2:	addx    %i3,%i3,%i3
	addcc   %i5,%i5,%i5
	addx    %i1,%i1,%i1
	subcc   %i4,0,%i4
	bne     b1       
	sub     %i4,1,%i4

	xor     %i3,7,%i3
	and     %i3,7,%i3
	andcc   %i3,1,%l2
	be      i3       
	NOP
	or      %g0,%i5,%i4
	sll     %l1,1,%i5
	subcc   %i5,%i4,%i5
	or      %g0,%i1,%i4
	sll     %l0,1,%i1
	subx    %i1,%i4,%i1
i3:	or      %g0,0x7e,%i2

b4:	subcc   %i1,0,%i1
	bneg    Aret     
	NOP
	addcc   %i5,%i5,%i5
	addx    %i1,%i1,%i1
	ba      b4       
	sub     %i2,1,%i2

Aret:	or      %g0,%i1,%i0
	or      %g0,%i2,%i1
	or      %g0,%i3,%i2

A999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

Anan:	sethi   %hi(0x80000000),%i3
	or      %g0,0xff,%i2
	ba      Aret     
	sethi   %hi(0xc0000000),%i1


	.global	_fpsin
_fpsin:
	.global	_sinf
_sinf:
	save	%sp,-96,%sp

	or      %g0,%i0,%o0
	call    _fpreduct
	NOP
	or      %g0,%o0,%l0
	or      %g0,%o1,%l1
	orcc    %g0,%o2,%l2

	bpos    i5       
	NOP
	ba      B999     
	sethi   %hi(0xffc00000),%i0
i5:	sethi   %hi(0x80000000),%l7
	and     %l7,%i0,%i1
	andcc   %l2,4,%l7
	be      i6       
	NOP
	sub     %l2,4,%l2
	sethi   %hi(0x80000000),%l7
	xor     %i1,%l7,%i1
i6:	sub     %l2,1,%l7
	subcc   %l7,2,%g0
	bcc     i7       
	NOP
	umul	%l0,%l0,%i5
	rd	%y,%i5
	sethi   %hi(coscof),%l6
	or	%l6,%lo(coscof),%l6
	or      %g0,0x7e,%i3
	sub     %i3,%l1,%i3
	sll     %i3,1,%i3
	subcc	%i3,32,%g0
	bcc,a	L1             
	or	%g0,0,%i5
L1:	srl	%i5,%i3,%i5
	ld      [%l6],%l0
	add     %l6,4,%l6

b8:	sra     %l0,31,%i4
	xor     %l0,%i4,%l0
	sub     %l0,%i4,%l0
	umul	%i5,%l0,%l0
	rd	%y,%l0
	xor     %l0,%i4,%l0
	sub     %l0,%i4,%l0
	ld      [%l6],%i4
	add     %l6,4,%l6
	add     %l0,%i4,%l0
	addcc   %i4,%i4,%l7
	bne     b8       
	NOP
	ba      e7       
	or      %g0,0x7f,%l1
i7:	or      %g0,%l0,%l3
	umul	%l0,%l0,%i5
	rd	%y,%i5
	sethi   %hi(sincof),%l6
	or	%l6,%lo(sincof),%l6
	or      %g0,0x7e,%i3
	sub     %i3,%l1,%i3
	sll     %i3,1,%i3
	subcc	%i3,32,%g0
	bcc,a	L2             
	or	%g0,0,%i5
L2:	srl	%i5,%i3,%i5
	ld      [%l6],%l0
	add     %l6,4,%l6

b9:	sra     %l0,31,%i4
	xor     %l0,%i4,%l0
	sub     %l0,%i4,%l0
	umul	%i5,%l0,%l0
	rd	%y,%l0
	xor     %l0,%i4,%l0
	sub     %l0,%i4,%l0
	ld      [%l6],%i4
	add     %l6,4,%l6
	add     %l0,%i4,%l0
	addcc   %i4,%i4,%l7
	bne     b9             
	subcc   %l0,0,%l0
	ble     i10      
	NOP
	sll     %l0,1,%l0
	sub     %l1,1,%l1
i10:	add     %l1,1,%l1
	umul	%l0,%l3,%l0
	rd	%y,%l0
e7:	subcc   %l0,0,%l0
	ble     i11      
	NOP
	sll     %l0,1,%l0
	sub     %l1,1,%l1
i11:	sll     %l0,1,%l0
	srl     %l0,9,%l0
	sll     %l1,23,%l7
	or      %l0,%l7,%l0
	or      %l0,%i1,%i0

B999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

	.global	_fpcos
_fpcos:
	.global	_cosf
_cosf:
	save	%sp,-96,%sp

	or      %g0,%i0,%o0
	call    _fpreduct
	NOP
	or      %g0,%o0,%i5
	or      %g0,%o1,%l0
	orcc    %g0,%o2,%l1

	bneg    Cnan     
	NOP
	or      %g0,0,%i1
	andcc   %l1,4,%l6
	be      i12      
	NOP
	sub     %l1,4,%l1
	sethi   %hi(0x80000000),%l6
	xor     %i1,%l6,%i1
i12:	andcc   %l1,2,%l6
	be      i13      
	NOP
	sethi   %hi(0x80000000),%l6
	xor     %i1,%l6,%i1

i13:	sub     %l1,1,%l6
	subcc   %l6,2,%g0
	bcc     i14      
	NOP
	or      %g0,%i5,%l2
	umul	%i5,%i5,%i4
	rd	%y,%i4
	sethi   %hi(sincof),%l5
	or	%l5,%lo(sincof),%l5
	or      %g0,0x7e,%i2
	sub     %i2,%l0,%i2
	sll     %i2,1,%i2
	subcc	%i2,32,%g0
	bcc,a	L3             
	or	%g0,0,%i4
L3:	srl	%i4,%i2,%i4
	ld      [%l5],%i5
	add     %l5,4,%l5

b15:	sra     %i5,31,%i3
	xor     %i5,%i3,%i5
	sub     %i5,%i3,%i5
	umul	%i4,%i5,%i5
	rd	%y,%i5
	xor     %i5,%i3,%i5
	sub     %i5,%i3,%i5
	ld      [%l5],%i3
	add     %l5,4,%l5
	add     %i5,%i3,%i5
	addcc   %i3,%i3,%l6
	bne     b15            
	subcc   %i5,0,%i5
	ble     i16      
	NOP
	sll     %i5,1,%i5
	sub     %l0,1,%l0
i16:	add     %l0,1,%l0
	umul	%i5,%l2,%i5
	rd	%y,%i5
	ba,a    e14      
i14:
	umul	%i5,%i5,%i4
	rd	%y,%i4
	sethi   %hi(coscof),%l5
	or	%l5,%lo(coscof),%l5
	or      %g0,0x7e,%i2
	sub     %i2,%l0,%i2
	sll     %i2,1,%i2
	subcc	%i2,32,%g0
	bcc,a	L4             
	or	%g0,0,%i4
L4:	srl	%i4,%i2,%i4
	ld      [%l5],%i5
	add     %l5,4,%l5

b17:	sra     %i5,31,%i3
	xor     %i5,%i3,%i5
	sub     %i5,%i3,%i5
	umul	%i4,%i5,%i5
	rd	%y,%i5
	xor     %i5,%i3,%i5
	sub     %i5,%i3,%i5
	ld      [%l5],%i3
	add     %l5,4,%l5
	add     %i5,%i3,%i5
	addcc   %i3,%i3,%l6
	bne     b17      
	NOP
	or      %g0,0x7f,%l0
e14:	subcc   %i5,0,%i5
	ble     i18      
	NOP
	sll     %i5,1,%i5
	sub     %l0,1,%l0
i18:	sll     %i5,1,%i5
	srl     %i5,9,%i5
	sll     %l0,23,%l6
	or      %i5,%l6,%i5
	or      %i5,%i1,%i0

C999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

Cnan:	ba      C999     
	sethi   %hi(0xffc00000),%i0

	.global	_fptan
_fptan:
	.global	_tanf
_tanf:
	save	%sp,-96,%sp

	or      %g0,%i0,%o0
	call    _fpreduct
	NOP
	or      %g0,%o0,%l0
	or      %g0,%o1,%l1
	orcc    %g0,%o2,%l2

	bneg    Dnan     
	NOP
	sethi   %hi(0x80000000),%l7
	and     %l7,%i0,%i1
	and     %l2,0xfffffffb,%l2
	andcc   %l2,2,%l7
	be      i19      
	NOP
	sethi   %hi(0x80000000),%l7
	xor     %i1,%l7,%i1
i19:	or      %g0,%l1,%l4
	umul	%l0,%l0,%l3
	rd	%y,%l3
	sethi   %hi(tancof),%l6
	or	%l6,%lo(tancof),%l6
	or      %g0,0x7e,%i2
	sub     %i2,%l1,%i2
	sll     %i2,1,%i2
	subcc	%i2,32,%g0
	bcc,a	L5             
	or	%g0,0,%l3
L5:	srl	%l3,%i2,%l3
	or      %g0,0,%i4

b20:	ld      [%l6],%i3
	add     %l6,4,%l6
	add     %i4,%i3,%i4
	umul	%l3,%i4,%i4
	rd	%y,%i4
	sethi   %hi(0x20000000),%l7
	subcc   %i3,%l7,%g0
	bcs     b20      
	NOP
	or      %g0,0x7f,%l1
	sethi   %hi(0x7fffffff),%l7
	or      %l7,0x3ff,%l7
	subcc   %l7,%i4,%i4
	bneg    i21      
	NOP
	sll     %i4,1,%i4
	sub     %l1,1,%l1
i21:	sub     %l2,1,%l7
	subcc   %l7,2,%g0
	bcc     i22      
	NOP
	add     %l1,0x7f,%l1
	sub     %l1,%l4,%l1
	srl     %i4,1,%i4
	or      %g0,%l0,%i5

	wr	%i4,0,%y
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
	ba,a    e22      

i22:	sub     %l4,%l1,%l1
	add     %l1,0x7f,%l1
	srl     %l0,1,%l0

	wr	%l0,0,%y
	or	%g0,0x0,%l0
	orcc	%g0,0,%g0
#if 0
	divscc	%l0,%i4,%l0
	divscc	%l0,%i4,%l0
	divscc	%l0,%i4,%l0
	divscc	%l0,%i4,%l0
	divscc	%l0,%i4,%l0
	divscc	%l0,%i4,%l0
	divscc	%l0,%i4,%l0
	divscc	%l0,%i4,%l0
	divscc	%l0,%i4,%l0
	divscc	%l0,%i4,%l0
	divscc	%l0,%i4,%l0
	divscc	%l0,%i4,%l0
	divscc	%l0,%i4,%l0
	divscc	%l0,%i4,%l0
	divscc	%l0,%i4,%l0
	divscc	%l0,%i4,%l0
	divscc	%l0,%i4,%l0
	divscc	%l0,%i4,%l0
	divscc	%l0,%i4,%l0
	divscc	%l0,%i4,%l0
	divscc	%l0,%i4,%l0
	divscc	%l0,%i4,%l0
	divscc	%l0,%i4,%l0
	divscc	%l0,%i4,%l0
	divscc	%l0,%i4,%l0
	divscc	%l0,%i4,%l0
	divscc	%l0,%i4,%l0
	divscc	%l0,%i4,%l0
	divscc	%l0,%i4,%l0
	divscc	%l0,%i4,%l0
	divscc	%l0,%i4,%l0
	divscc	%l0,%i4,%l0
#else
	DIVSCC | (lo0 << 14) | in4 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in4 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in4 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in4 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in4 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in4 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in4 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in4 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in4 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in4 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in4 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in4 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in4 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in4 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in4 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in4 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in4 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in4 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in4 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in4 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in4 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in4 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in4 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in4 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in4 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in4 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in4 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in4 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in4 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in4 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in4 | (lo0 << 25)
	DIVSCC | (lo0 << 14) | in4 | (lo0 << 25)
#endif

e22:	subcc   %l0,0,%l0
	ble     i23      
	NOP
	sll     %l0,1,%l0
	sub     %l1,1,%l1
i23:	sll     %l0,1,%l0
	srl     %l0,9,%l0
	sll     %l1,23,%l7
	or      %l0,%l7,%l0
	or      %l0,%i1,%i0

D999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

Dnan:	ba      D999     
	sethi   %hi(0xffc00000),%i0

!	.end
