/* fdmul32.s - VxWorks conversion from Microtek tools to Sun native */

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

	.text
	.align	4

	.global	_fdmul
_fdmul:
	save	%sp,-96,%sp

	sra     %i0,23,%l3
	and     %l3,0xff,%l3
	sra     %i1,23,%i3
	and     %i3,0xff,%i3
	sethi   %hi(0x80000000),%i4
	sll     %i0,8,%l1
	or      %l1,%i4,%l1
	sll     %i1,8,%i2
	or      %i2,%i4,%i2
	xor     %i0,%i1,%l4
	and     %i4,%l4,%i4

	sub     %l3,1,%l4
	subcc   %l4,0xfe,%g0
	bcc     AspecA         
	sub     %i3,1,%l4
	subcc   %l4,0xfe,%g0
	bcc     AspecB   
	NOP
	umul	%l1,%i2,%l2
	rd	%y,%l1
	subcc   %l1,0,%l1
	ble     i1       
	NOP
	addcc   %l2,%l2,%l2
	addx    %l1,%l1,%l1
	sub     %l3,1,%l3
i1:	add     %i3,0x302,%l4
	add     %l3,%l4,%l3
	subcc   %l3,0,%l3
	ble     Azer     
	NOP

	addcc   %l2,0x400,%l2
	addxcc  %l1,0,%l1
	addx    %l3,0,%l3
	andcc   %l2,0x7ff,%l4
	be,a	i2                
	and     %l2,0xfffff7ff,%l2
i2:	srl     %l2,11,%l0
	sll     %l1,21,%l4
	or      %l0,%l4,%l0
	srl     %l1,11,%i5
	sethi   %hi(0xfffff),%l4
	or      %l4,0x3ff,%l4
	and     %i5,%l4,%i5
	sll     %l3,20,%l4
	or      %i5,%l4,%i5
	or      %i5,%i4,%i5

Aret:	or      %g0,%i5,%i0
	or      %g0,%l0,%i1

A999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

AspecA:	subcc   %l3,0,%l3
	bne     i3             
	subcc   %i3,0xff,%g0
	be      Anan     
	NOP
	ba,a    Azer     
i3:	addcc   %l1,%l1,%l4
	bne     Anan           
	subcc   %i3,0,%i3
	be      Anan           
	subcc   %i3,0xff,%g0
	bne     Ainf           
	addcc   %i2,%i2,%l4
	bne     Anan     
	NOP
	ba,a    Ainf     

AspecB:	subcc   %i3,0,%i3
	be      Azer           
	addcc   %i2,%i2,%l4
	bne     Anan     
	NOP

Ainf:	sethi   %hi(0x7ff00000),%i5
	ba      Aret     
	or      %g0,0,%l0

Azer:	or      %g0,0,%l0
	ba      Aret     
	or      %g0,%i4,%i5

Anan:	sethi   %hi(0xfff80000),%i5
	ba      Aret     
	or      %g0,0,%l0

!	.end
