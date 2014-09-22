/* fpexp32.s - VxWorks conversion from Microtek tools to Sun native */

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

expcof:	.word	0x7ff3
	.word	0x50c24
	.word	0x2bb100
	.word	0x13b2ab7
	.word	0x71ac236
	.word	0x1ebfbe00
	.word	0x58b90bfc
	.word	0x80000000

	.text
	.align	4

	.global	_fpexexp
_fpexexp:
	save	%sp,-96,%sp

	sra     %i2,31,%i4
	sethi   %hi(0xb8aa3b29),%i5
	or      %i5,0x329,%i5
	umul	%i5,%i0,%i5
	rd	%y,%l1
	subcc   %l1,0,%l1
	ble     i1       
	NOP
	addcc   %i5,%i5,%i5
	addx    %l1,%l1,%l1
	sub     %i1,1,%i1
i1:	or      %g0,0,%i3
	subcc   %i1,0x7d,%l0
	be      Alab3          
	subcc   %l0,0xf,%g0
	bcs     i2             
	subcc   %l0,0,%l0
	bg      Aovr     
	NOP
	sub     %g0,%l0,%i5
	subcc	%i5,32,%g0
	bcc,a	L1             
	or	%g0,0,%l1
L1:	srl	%l1,%i5,%l1
	ba,a    Alab3    
i2:	or      %g0,0x20,%l4
	sub     %l4,%l0,%l4
	srl     %l1,%l4,%i3
	sll     %l1,%l0,%l1

Alab3:	subcc   %l1,0,%l1
	bpos    i3       
	NOP
	sub     %g0,%l1,%l1
	xor     %i4,-1,%i4
	add     %i3,1,%i3
i3:	sethi   %hi(expcof),%l3
	or	%l3,%lo(expcof),%l3
	ld      [%l3],%i0
	add     %l3,4,%l3

b4:	xor     %i0,%i4,%l2
	umul	%l1,%i0,%i0
	rd	%y,%i0
	subcc   %l2,0,%l2
	bneg,a	i5              
	sub     %g0,%i0,%i0
i5:	ld      [%l3],%l0
	add     %l3,4,%l3
	add     %i0,%l0,%i0
	sethi   %hi(0x80000000),%l4
	subcc   %l0,%l4,%g0
	bne     b4       
	NOP
	sra     %i2,31,%i4
	xor     %i3,%i4,%i3
	sub     %i3,%i4,%i3
	add     %i3,0x7f,%i1
	subcc   %i0,0,%i0
	bneg    Aret     
	NOP
	sll     %i0,1,%i0
	sub     %i1,1,%i1

Aret:	or      %g0,0,%i2

A999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

Aovr:	sethi   %hi(0x80000000),%i0
	sethi   %hi(0xffff),%i1
	or      %i1,0x3ff,%i1
	subcc   %i2,0,%i2
	bne,a	Aret             
	xor     %i1,-1,%i1
	ba,a    Aret     

	.global	_fpexp
_fpexp:
	.global	_expf
_expf:
	save	%sp,-96,%sp

	srl     %i0,23,%i2
	and     %i2,0xff,%i2
	sethi   %hi(0x80000000),%i3
	sll     %i0,8,%i1
	or      %i1,%i3,%i1
	and     %i3,%i0,%i3
	sub     %i2,1,%l2
	subcc   %l2,0xfe,%g0
	bcc     Bspec          

	or      %g0,%i1,%o0
	or      %g0,%i2,%o1
	call    _fpexexp
	or      %g0,%i3,%o2
	or      %g0,%o0,%i4
	or      %g0,%o1,%i5
	or      %g0,%o2,%l0

	sub     %i5,1,%l2
	subcc   %l2,0xfe,%g0
	bcc     Bundove  
	NOP

Bret:	sll     %i4,1,%i0
	srl     %i0,9,%i0
	sll     %i5,23,%l2
	or      %i0,%l2,%i0

B999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

Bspec:	subcc   %i2,0,%i2
	bne     i8       
	NOP
	ba      B999     
	sethi   %hi(0x3f800000),%i0
i8:	sll     %i1,1,%l2
	subcc   %l2,0,%l2
	be      Bundove  
	NOP
	ba      B999     
	sethi   %hi(0xffc00000),%i0

Bundove:
	subcc   %i3,0,%i3
	be      i10      
	NOP
	or      %g0,9,%l1
	sub     %l1,%i5,%l1
	subcc	%l1,32,%g0
	bcc,a	L2             
	or	%g0,0,%i4
L2:	srl	%i4,%l1,%i4
	ba      B999     
	or      %g0,%i4,%i0
i10:	ba      B999     
	sethi   %hi(0x7f800000),%i0

!	.end
