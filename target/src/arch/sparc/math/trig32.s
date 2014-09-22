/* trig32.s - VxWorks conversion from Microtek tools to Sun native */

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

	.global	_exmul
	.global	_exdiv
	.global	_sigmax

	.data
	.align	4

sincon:	.word	0xff946030,0xffffffff
	.word	0x5849184f,0x0
	.word	0x33753015,0xffffffca
	.word	0xe3a556c7,0x171d
	.word	0xf97f97f9,0xfff97f97
	.word	0x11111111,0x1111111
	.word	0xaaaaaaab,0xeaaaaaaa
	.word	0x0,0x80000000
coscon:	.word	0x6b9fd,0x0
	.word	0xf9b1a2d6,0xffffffff
	.word	0x7bb63bfe,0x4
	.word	0x360910ec,0xfffffdb0
	.word	0xd00d01,0xd00d
	.word	0xd27d27d2,0xffd27d27
	.word	0x55555555,0x5555555
	.word	0x0,0xc0000000
	.word	0x0,0x80000000
tancon:	.word	0xffde9d9a,0xffffffff
	.word	0xfeb68270,0xffffffff
	.word	0xf34c0f08,0xffffffff
	.word	0x82a09a5d,0xffffffff
	.word	0x2a9e38a7,0xfffffffb
	.word	0x4b63c10d,0xffffffd0
	.word	0x24d3b1f7,0xfffffe29
	.word	0xfa29bbda,0xffffedd7
	.word	0x86a0478a,0xffff4cab
	.word	0x1b32c43d,0xfff910a2
	.word	0xffbaa65,0xffbaa655
	.word	0x27d27d28,0xfd27d27d
	.word	0x55555555,0xd5555555
	.word	0x0,0x80000000

	.text
	.align	4

	.global	_reduct
_reduct:
	save	%sp,-96,%sp

	sll     %i0,1,%i4
	srl     %i4,21,%i4
	sll     %i0,11,%i2
	srl     %i1,21,%l5
	or      %i2,%l5,%i2
	sll     %i1,11,%i3
	sethi   %hi(0x80000000),%l5
	or      %i2,%l5,%i2
	subcc   %i4,0x3fe,%i5
	bneg    Aret2          
	subcc   %i5,0x40,%g0
	bg      Anan     
	NOP
	srl     %i3,1,%i3
	sll     %i2,31,%l5
	srl     %i2,1,%i2
	or      %i3,%l5,%i3
	sethi   %hi(0x60000000),%l4
	sethi   %hi(0x10b4611a),%l3
	or      %l3,0x11a,%l3
	sethi   %hi(0x6487ed51),%l2
	or      %l2,0x151,%l2
	or      %g0,-1,%l0
	or      %g0,0,%l1

Alab3:	subcc   %l1,%l4,%l1
	subxcc  %i3,%l3,%i3
	subxcc  %i2,%l2,%i2
	bcc     i1       
	NOP
	addcc   %l1,%l4,%l1
	addxcc  %i3,%l3,%i3
	addxcc  %i2,%l2,%i2
i1:	addx    %l0,%l0,%l0
	addcc   %l1,%l1,%l1
	addxcc  %i3,%i3,%i3
	addx    %i2,%i2,%i2
	subcc   %i5,0,%i5
	bne     Alab3    
	sub     %i5,1,%i5
	xor     %l0,-1,%l0
	andcc   %l0,1,%l5
	be      i2       
	NOP
	or      %g0,%l1,%i5
	sll     %l4,1,%l1
	subcc   %l1,%i5,%l1
	or      %g0,%i3,%i5
	sll     %l3,1,%i3
	subxcc  %i3,%i5,%i3
	or      %g0,%i2,%i5
	sll     %l2,1,%i2
	subx    %i2,%i5,%i2
i2:	or      %g0,0x3fe,%i4

b3:	subcc   %i2,0,%i2
	bneg    Aret     
	NOP
	addcc   %l1,%l1,%l1
	addxcc  %i3,%i3,%i3
	addx    %i2,%i2,%i2
	ba      b3       
	sub     %i4,1,%i4

Aret:	sll     %l0,29,%l5
	or      %i4,%l5,%i4

Aret2:	or      %g0,%i2,%i0
	or      %g0,%i3,%i1
	or      %g0,%i4,%i2

A999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

Anan:	or      %g0,-1,%i4
	sethi   %hi(0xc0000000),%i2
	ba      Aret2    
	or      %g0,0,%i3


	.global	_dpsin
_dpsin:
	.global	_sin
_sin:
	save	%sp,-96,%sp

	sethi   %hi(0x80000000),%l4
	and     %l4,%i0,%i2

	or      %g0,%i0,%o0
	call    _reduct
	or      %g0,%i1,%o1
	or      %g0,%o0,%i4
	or      %g0,%o1,%i5
	or      %g0,%o2,%l0

	subcc   %l0,-1,%g0
	be      Bret     
	NOP
	srl     %l0,29,%i3
	sll     %l0,3,%l0
	srl     %l0,3,%l0
	andcc   %i3,4,%l4
	be      i4       
	NOP
	sub     %i3,4,%i3
	sethi   %hi(0x80000000),%l4
	xor     %i2,%l4,%i2
i4:	sub     %i3,1,%l4
	subcc   %l4,2,%g0
	bcc     i5             

	or      %g0,%i4,%o0
	or      %g0,%i5,%o1
	or      %g0,%l0,%o2
	or      %g0,%i4,%o3
	or      %g0,%i5,%o4
	call    _exmul
	or      %g0,%l0,%o5

	or      %g0,0,%o3
	sethi   %hi(coscon),%o4
	call    _sigmax
	or	%o4,%lo(coscon),%o4
	or      %g0,%o0,%i4
	or      %g0,%o1,%i5
	ba      Bret     
	or      %g0,%o2,%l0

i5:	or      %g0,%i4,%l1
	or      %g0,%i5,%l2
	or      %g0,%l0,%l3

	or      %g0,%i4,%o0
	or      %g0,%i5,%o1
	or      %g0,%l0,%o2
	or      %g0,%i4,%o3
	or      %g0,%i5,%o4
	call    _exmul
	or      %g0,%l0,%o5

	or      %g0,0,%o3
	sethi   %hi(sincon),%o4
	call    _sigmax
	or	%o4,%lo(sincon),%o4

	or      %g0,%l1,%o3
	or      %g0,%l2,%o4
	call    _exmul
	or      %g0,%l3,%o5
	or      %g0,%o0,%i4
	or      %g0,%o1,%i5
	or      %g0,%o2,%l0

Bret:	srl     %i5,11,%i1
	sll     %i4,21,%l4
	or      %i1,%l4,%i1
	srl     %i4,11,%i0
	sethi   %hi(0xfffff),%l4
	or      %l4,0x3ff,%l4
	and     %i0,%l4,%i0
	sll     %l0,20,%l4
	or      %i0,%l4,%i0
	or      %i0,%i2,%i0

B999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0


	.global	_dpcos
_dpcos:
	.global	_cos
_cos:
	save	%sp,-96,%sp

	or      %g0,0,%i2

	or      %g0,%i0,%o0
	call    _reduct
	or      %g0,%i1,%o1
	or      %g0,%o0,%i4
	or      %g0,%o1,%i5
	or      %g0,%o2,%l0

	subcc   %l0,-1,%g0
	be      Cret     
	NOP
	srl     %l0,29,%i3
	sll     %l0,3,%l0
	srl     %l0,3,%l0
	andcc   %i3,4,%l4
	be      i6       
	NOP
	sub     %i3,4,%i3
	sethi   %hi(0x80000000),%l4
	xor     %i2,%l4,%i2
i6:	andcc   %i3,2,%l4
	be      i7       
	NOP
	sethi   %hi(0x80000000),%l4
	xor     %i2,%l4,%i2

i7:	sub     %i3,1,%l4
	subcc   %l4,2,%g0
	bcc     i8       
	NOP
	or      %g0,%i4,%l1
	or      %g0,%i5,%l2
	or      %g0,%l0,%l3

	or      %g0,%i4,%o0
	or      %g0,%i5,%o1
	or      %g0,%l0,%o2
	or      %g0,%i4,%o3
	or      %g0,%i5,%o4
	call    _exmul
	or      %g0,%l0,%o5

	or      %g0,0,%o3
	sethi   %hi(sincon),%o4
	call    _sigmax
	or	%o4,%lo(sincon),%o4

	or      %g0,%l1,%o3
	or      %g0,%l2,%o4
	call    _exmul
	or      %g0,%l3,%o5
	or      %g0,%o0,%i4
	or      %g0,%o1,%i5
	ba      Cret     
	or      %g0,%o2,%l0

i8:	or      %g0,%i4,%o0
	or      %g0,%i5,%o1
	or      %g0,%l0,%o2
	or      %g0,%i4,%o3
	or      %g0,%i5,%o4
	call    _exmul
	or      %g0,%l0,%o5

	or      %g0,0,%o3
	sethi   %hi(coscon),%o4
	call    _sigmax
	or	%o4,%lo(coscon),%o4
	or      %g0,%o0,%i4
	or      %g0,%o1,%i5
	or      %g0,%o2,%l0

Cret:	srl     %i5,11,%i1
	sll     %i4,21,%l4
	or      %i1,%l4,%i1
	srl     %i4,11,%i0
	sethi   %hi(0xfffff),%l4
	or      %l4,0x3ff,%l4
	and     %i0,%l4,%i0
	sll     %l0,20,%l4
	or      %i0,%l4,%i0
	or      %i0,%i2,%i0

C999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0


	.global	_dptan
_dptan:
	.global	_tan
_tan:
	save	%sp,-96,%sp

	sethi   %hi(0x80000000),%l5
	and     %l5,%i0,%i2

	or      %g0,%i0,%o0
	call    _reduct
	or      %g0,%i1,%o1
	or      %g0,%o0,%i5
	or      %g0,%o1,%l0
	or      %g0,%o2,%l1

	subcc   %l1,-1,%g0
	be      Dret     
	NOP
	srl     %l1,29,%i4
	sll     %l1,3,%l1
	srl     %l1,3,%l1
	and     %i4,0xfffffffb,%i4
	andcc   %i4,2,%l5
	be      i9       
	NOP
	sethi   %hi(0x80000000),%l5
	xor     %i2,%l5,%i2

i9:	or      %g0,%i5,%o0
	or      %g0,%l0,%o1
	or      %g0,%l1,%o2
	or      %g0,%i5,%o3
	or      %g0,%l0,%o4
	call    _exmul
	or      %g0,%l1,%o5

	or      %g0,0,%o3
	sethi   %hi(tancon),%o4
	call    _sigmax
	or	%o4,%lo(tancon),%o4
	or      %g0,%o0,%l2
	or      %g0,%o1,%l3
	or      %g0,%o2,%l4

	sub     %i4,1,%l5
	subcc   %l5,2,%g0
	bcc     i10            

	or      %g0,%l2,%o0
	or      %g0,%l3,%o1
	or      %g0,%l4,%o2
	or      %g0,%i5,%o3
	or      %g0,%l0,%o4
	call    _exdiv
	or      %g0,%l1,%o5
	or      %g0,%o0,%i5
	or      %g0,%o1,%l0
	ba      Dret     
	or      %g0,%o2,%l1

i10:	or      %g0,%i5,%o0
	or      %g0,%l0,%o1
	or      %g0,%l1,%o2
	or      %g0,%l2,%o3
	or      %g0,%l3,%o4
	call    _exdiv
	or      %g0,%l4,%o5
	or      %g0,%o0,%i5
	or      %g0,%o1,%l0
	or      %g0,%o2,%l1

Dret:	srl     %l0,11,%i1
	sll     %i5,21,%l5
	or      %i1,%l5,%i1
	srl     %i5,11,%i0
	sethi   %hi(0xfffff),%l5
	or      %l5,0x3ff,%l5
	and     %i0,%l5,%i0
	sll     %l1,20,%l5
	or      %i0,%l5,%i0
	or      %i0,%i2,%i0

D999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

!	.end
