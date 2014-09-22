/* hyp32.s - VxWorks conversion from Microtek tools to Sun native */

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

	.global	_exadd
	.global	_exsub
	.global	_exmul
	.global	_exdiv
	.global	_exexp
	.global	_sigmax

	.data
	.align	4

sihcon:	.word	0x654b,0x0
	.word	0x6b9fd0,0x0
	.word	0x5849184f,0x0
	.word	0xcc8acfeb,0x35
	.word	0xe3a556c7,0x171d
	.word	0x6806807,0x68068
	.word	0x11111111,0x1111111
	.word	0x55555555,0x15555555
	.word	0x0,0x80000000
tahcon:	.word	0xfeb68270,0xffffffff
	.word	0xcb3f0f8,0x0
	.word	0x82a09a5d,0xffffffff
	.word	0xd561c759,0x4
	.word	0x4b63c10d,0xffffffd0
	.word	0xdb2c4e09,0x1d6
	.word	0xfa29bbda,0xffffedd7
	.word	0x795fb876,0xb354
	.word	0x1b32c43d,0xfff910a2
	.word	0xf004559b,0x4559aa
	.word	0x27d27d28,0xfd27d27d
	.word	0xaaaaaaab,0x2aaaaaaa
	.word	0x0,0x80000000

	.text
	.align	4

	.global	_dpcosh
_dpcosh:
	.global	_cosh
_cosh:
	save	%sp,-96,%sp

	sll     %i0,1,%i4
	srl     %i4,21,%i4
	sll     %i0,11,%i2
	srl     %i1,21,%l2
	or      %i2,%l2,%i2
	sll     %i1,11,%i3
	sethi   %hi(0x80000000),%l2
	or      %i2,%l2,%i2
	subcc   %i4,0x7ff,%g0
	be      Aspec          

	or      %g0,%i2,%o0
	or      %g0,%i3,%o1
	or      %g0,%i4,%o2
	call    _exexp
	or      %g0,0,%o3
	or      %g0,%o0,%i2
	or      %g0,%o1,%i3
	or      %g0,%o2,%i4

	sethi   %hi(0x80000000),%o0
	or      %g0,0,%o1
	or      %g0,0x3ff,%o2
	or      %g0,%i2,%o3
	or      %g0,%i3,%o4
	call    _exdiv
	or      %g0,%i4,%o5
	or      %g0,%o0,%i5
	or      %g0,%o1,%l0
	or      %g0,%o2,%l1

	or      %g0,%i2,%o0
	or      %g0,%i3,%o1
	or      %g0,%i4,%o2
	or      %g0,%i5,%o3
	or      %g0,%l0,%o4
	call    _exadd
	or      %g0,%l1,%o5
	or      %g0,%o0,%i2
	or      %g0,%o1,%i3
	or      %g0,%o2,%i4

	sub     %i4,1,%i4
	subcc   %i4,0x7ff,%g0
	bcc     Ainf     
	NOP
	srl     %i3,11,%i1
	sll     %i2,21,%l2
	or      %i1,%l2,%i1
	srl     %i2,11,%i0
	sethi   %hi(0xfffff),%l2
	or      %l2,0x3ff,%l2
	and     %i0,%l2,%i0
	sll     %i4,20,%l2
	or      %i0,%l2,%i0

Aret:
A999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

Aspec:	add     %i2,%i2,%l2
	orcc    %l2,%i3,%l2
	be      Ainf     
	NOP
	or      %g0,0,%i1
	ba      Aret     
	sethi   %hi(0xfff80000),%i0

Ainf:	or      %g0,0,%i1
	ba      Aret     
	sethi   %hi(0x7ff00000),%i0

	.global	_dpsinh
_dpsinh:
	.global	_sinh
_sinh:
	save	%sp,-96,%sp

	sll     %i0,1,%i4
	srl     %i4,21,%i4
	sll     %i0,11,%i2
	srl     %i1,21,%l3
	or      %i2,%l3,%i2
	sll     %i1,11,%i3
	sethi   %hi(0x80000000),%l2
	or      %i2,%l2,%i2
	and     %l2,%i0,%l2
	subcc   %i4,0x7ff,%g0
	be      Bspec          
	subcc   %i4,0x3fe,%g0
	bcc     i2             

	or      %g0,%i2,%o0
	or      %g0,%i3,%o1
	or      %g0,%i4,%o2
	or      %g0,%i2,%o3
	or      %g0,%i3,%o4
	call    _exmul
	or      %g0,%i4,%o5

	or      %g0,0,%o3
	sethi   %hi(sihcon),%o4
	call    _sigmax
	or	%o4,%lo(sihcon),%o4
	or      %g0,%o0,%i5
	or      %g0,%o1,%l0
	or      %g0,%o2,%l1

	or      %g0,%i2,%o3
	or      %g0,%i3,%o4
	call    _exmul
	or      %g0,%i4,%o5
	or      %g0,%o0,%i2
	or      %g0,%o1,%i3
	ba      e2       
	or      %g0,%o2,%i4

i2:	or      %g0,%i2,%o0
	or      %g0,%i3,%o1
	or      %g0,%i4,%o2
	call    _exexp
	or      %g0,0,%o3
	or      %g0,%o0,%i2
	or      %g0,%o1,%i3
	or      %g0,%o2,%i4

	sethi   %hi(0x80000000),%o0
	or      %g0,0,%o1
	or      %g0,0x3ff,%o2
	or      %g0,%i2,%o3
	or      %g0,%i3,%o4
	call    _exdiv
	or      %g0,%i4,%o5
	or      %g0,%o0,%i5
	or      %g0,%o1,%l0
	or      %g0,%o2,%l1

	or      %g0,%i2,%o0
	or      %g0,%i3,%o1
	or      %g0,%i4,%o2
	or      %g0,%i5,%o3
	or      %g0,%l0,%o4
	call    _exsub
	or      %g0,%l1,%o5
	or      %g0,%o0,%i2
	or      %g0,%o1,%i3
	or      %g0,%o2,%i4

	sub     %i4,1,%i4
	subcc   %i4,0x7ff,%g0
	bcc     Bovr     
	NOP
e2:	srl     %i3,11,%i1
	sll     %i2,21,%l3
	or      %i1,%l3,%i1
	srl     %i2,11,%i0
	sethi   %hi(0xfffff),%l3
	or      %l3,0x3ff,%l3
	and     %i0,%l3,%i0
	sll     %i4,20,%l3
	or      %i0,%l3,%i0

Bret:	or      %i0,%l2,%i0

B999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

Bspec:	sll     %i2,1,%l3
	orcc    %l3,%i3,%l3
	be      Bret     
	NOP
	or      %g0,0,%i1
	ba      Bret     
	sethi   %hi(0xfff80000),%i0

Bovr:	or      %g0,0,%i1
	ba      Bret     
	sethi   %hi(0x7ff00000),%i0

	.global	_dptanh
_dptanh:
	.global	_tanh
_tanh:
	save	%sp,-96,%sp

	sll     %i0,1,%i4
	srl     %i4,21,%i4
	sll     %i0,11,%i2
	srl     %i1,21,%l6
	or      %i2,%l6,%i2
	sll     %i1,11,%i3
	sethi   %hi(0x80000000),%l5
	or      %i2,%l5,%i2
	and     %l5,%i0,%l5
	sub     %i4,1,%l6
	subcc   %l6,0x7fe,%g0
	bcc     Cspec          
	subcc   %i4,0x3fe,%g0
	bcc     i4             

	or      %g0,%i2,%o0
	or      %g0,%i3,%o1
	or      %g0,%i4,%o2
	or      %g0,%i2,%o3
	or      %g0,%i3,%o4
	call    _exmul
	or      %g0,%i4,%o5
	or      %g0,%o0,%l2
	or      %g0,%o1,%l3
	or      %g0,%o2,%l4

	or      %g0,0,%o3
	sethi   %hi(tahcon),%o4
	call    _sigmax
	or	%o4,%lo(tahcon),%o4
	or      %g0,%o0,%i5
	or      %g0,%o1,%l0
	ba      e4       
	or      %g0,%o2,%l1

i4:	or      %g0,%i2,%o0
	or      %g0,%i3,%o1
	or      %g0,%i4,%o2
	call    _exexp
	or      %g0,0,%o3
	or      %g0,%o0,%i5
	or      %g0,%o1,%l0
	or      %g0,%o2,%l1

	sethi   %hi(0x80000000),%o0
	or      %g0,0,%o1
	or      %g0,0x3ff,%o2
	or      %g0,%i5,%o3
	or      %g0,%l0,%o4
	call    _exdiv
	or      %g0,%l1,%o5
	or      %g0,%o0,%l2
	or      %g0,%o1,%l3
	or      %g0,%o2,%l4

	or      %g0,%i5,%o0
	or      %g0,%l0,%o1
	or      %g0,%l1,%o2
	or      %g0,%l2,%o3
	or      %g0,%l3,%o4
	call    _exsub
	or      %g0,%l4,%o5
	or      %g0,%o0,%i2
	or      %g0,%o1,%i3
	or      %g0,%o2,%i4

	or      %g0,%i5,%o0
	or      %g0,%l0,%o1
	or      %g0,%l1,%o2
	or      %g0,%l2,%o3
	or      %g0,%l3,%o4
	call    _exadd
	or      %g0,%l4,%o5
	or      %g0,%o0,%i5
	or      %g0,%o1,%l0
	or      %g0,%o2,%l1

e4:	or      %g0,%i2,%o0
	or      %g0,%i3,%o1
	or      %g0,%i4,%o2
	or      %g0,%i5,%o3
	or      %g0,%l0,%o4
	call    _exdiv
	or      %g0,%l1,%o5
	or      %g0,%o0,%i2
	or      %g0,%o1,%i3
	or      %g0,%o2,%i4

	srl     %i3,11,%i1
	sll     %i2,21,%l6
	or      %i1,%l6,%i1
	srl     %i2,11,%i0
	sethi   %hi(0xfffff),%l6
	or      %l6,0x3ff,%l6
	and     %i0,%l6,%i0
	sll     %i4,20,%l6
	or      %i0,%l6,%i0

Cret:	or      %i0,%l5,%i0

C999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

Cspec:	subcc   %i4,0,%i4
	be      Cret     
	NOP
	sethi   %hi(0x3ff00000),%i0
	sll     %i2,1,%l6
	orcc    %l6,%i3,%l6
	be      Cret     
	NOP
	or      %g0,0,%i1
	ba      Cret     
	sethi   %hi(0xfff80000),%i0

!	.end
