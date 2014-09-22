/* exp32.s - VxWorks conversion from Microtek tools to Sun native */

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
	.global	_sigmax

	.data
	.align	4
expcon:	.word	0xe1deb28,0x0
	.word	0xf465639b,0x0
	.word	0x267a8ac6,0xf
	.word	0x929e9caf,0xda
	.word	0x111d2e4,0xb16
	.word	0xff1622c3,0x7ff2
	.word	0x4be1b1e2,0x50c24
	.word	0xcf14ce62,0x2bb0ff
	.word	0xfba4e773,0x13b2ab6
	.word	0xc1282fe3,0x71ac235
	.word	0x82c58ea9,0x1ebfbdff
	.word	0xe8e7bcd6,0x58b90bfb
	.word	0x0,0x80000000

	.text
	.align	4

	.global	_exexp
_exexp:
	save	%sp,-96,%sp

	sra     %i3,31,%i4

	sethi   %hi(0xb8aa3b29),%o0
	or      %o0,0x329,%o0
	sethi   %hi(0x5c17f0bc),%o1
	or      %o1,0xbc,%o1
	or      %g0,0x3ff,%o2
	or      %g0,%i0,%o3
	or      %g0,%i1,%o4
	call    _exmul
	or      %g0,%i2,%o5
	or      %g0,%o0,%l2
	or      %g0,%o1,%l3
	or      %g0,%o2,%l4

	or      %g0,0,%i5
	sub     %l4,0x3fe,%l0
	subcc   %l0,0xf,%g0
	bcs     b2             
	subcc   %l0,0,%l0
	bg      Aovr     
	NOP
	ba,a    Alab3    

b2:	subcc   %l0,0,%l0
	be      i2       
	sub     %l0,1,%l0
	addcc   %l3,%l3,%l3
	addxcc  %l2,%l2,%l2
	addx    %i5,%i5,%i5
	ba      b2       
	sub     %l4,1,%l4
i2:	subcc   %l2,0,%l2
	bpos    Alab3    
	NOP
	subcc   %g0,%l3,%l3
	subx    %g0,%l2,%l2
	sethi   %hi(0x80000000),%l5
	xor     %i3,%l5,%i3
	add     %i5,1,%i5

Alab3:	or      %g0,%l2,%o0
	or      %g0,%l3,%o1
	or      %g0,%l4,%o2
	or      %g0,%i3,%o3
	sethi   %hi(expcon),%o4
	call    _sigmax
	or	%o4,%lo(expcon),%o4
	or      %g0,%o0,%l2
	or      %g0,%o1,%l3
	or      %g0,%o2,%l4

	xor     %i5,%i4,%i5
	sub     %i5,%i4,%i5
	add     %l4,%i5,%l4

Aret:	or      %g0,%l2,%i0
	or      %g0,%l3,%i1
	or      %g0,%l4,%i2

A999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

Aovr:	or      %g0,0,%l3
	subcc   %i3,0,%i3
	be      i4       
	NOP
	or      %g0,0,%l4
	ba      Aret     
	or      %g0,%l4,%l2
i4:	sethi   %hi(0x80000000),%l2
	sethi   %hi(0x7fff),%l4
	ba      Aret     
	or      %l4,0x3ff,%l4

	.global	_dpexp
_dpexp:
	.global	_exp
_exp:
	save	%sp,-96,%sp

	sll     %i0,1,%i4
	srl     %i4,21,%i4
	sll     %i0,11,%i2
	srl     %i1,21,%l0
	or      %i2,%l0,%i2
	sll     %i1,11,%i3
	sethi   %hi(0x80000000),%i5
	or      %i2,%i5,%i2
	sub     %i4,1,%l0
	subcc   %l0,0x7fe,%g0
	bcc     Bspec          

	or      %g0,%i2,%o0
	or      %g0,%i3,%o1
	or      %g0,%i4,%o2
	call    _exexp
	and     %i0,%i5,%o3
	or      %g0,%o0,%i2
	or      %g0,%o1,%i3
	or      %g0,%o2,%i4

	sub     %i4,1,%l0
	subcc   %l0,0x7fe,%g0
	bcc     Bundove  
	NOP
	srl     %i3,11,%i1
	sll     %i2,21,%l0
	or      %i1,%l0,%i1
	srl     %i2,11,%i0
	sethi   %hi(0xfffff),%l0
	or      %l0,0x3ff,%l0
	and     %i0,%l0,%i0
	sll     %i4,20,%l0
	or      %i0,%l0,%i0

Bret:
B999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

Bspec:	subcc   %i4,0,%i4
	be      Bone           
	sll     %i2,1,%l0
	orcc    %l0,%i3,%l0
	bne     Bnan           
	subcc   %i0,0,%i0
	bpos    Binf     
	NOP
	or      %g0,0,%i1
	ba      Bret     
	or      %g0,%i1,%i0

Binf:	sethi   %hi(0x7ff00000),%i0
	ba      Bret     
	or      %g0,0,%i1

Bnan:	sethi   %hi(0xfff80000),%i0
	ba      Bret     
	or      %g0,0,%i1

Bone:	sethi   %hi(0x3ff00000),%i0
	ba      Bret     
	or      %g0,0,%i1

Bundove:
	subcc   %i4,0,%i4
	bg      Binf     
	NOP
	or      %g0,0xc,%i5
	sub     %i5,%i4,%i5

L1:	subcc	%i5,32,%g0
	bcs	L2             
	subcc	%g0,%i5,%i4
	sub	%i5,32,%i5
	orcc	%i2,0,%i3
	bne	L1             
	or	%g0,0,%i2
L2:	be	L3             
	sll	%i2,%i4,%i4
	srl	%i3,%i5,%i3
	srl	%i2,%i5,%i2
	or	%i3,%i4,%i3

L3:	or      %g0,%i2,%i0
	ba      Bret     
	or      %g0,%i3,%i1

!	.end
