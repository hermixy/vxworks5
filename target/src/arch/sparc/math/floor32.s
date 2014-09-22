/* floor32.s - VxWorks conversion from Microtek tools to Sun native */

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

	.global	_dpfloor
_dpfloor:
	.global	_floor
_floor:
	save	%sp,-96,%sp

	sethi   %hi(0x80000000),%l0
	and     %l0,%i0,%i2
	sll     %i0,1,%i3
	srl     %i3,21,%i3
	subcc   %i3,0x7ff,%g0
	be      Aspec    
	NOP
	or      %g0,0x433,%l0
	sub     %l0,%i3,%i3
	subcc   %i3,0x35,%g0
	bcs     i1             
	subcc   %i3,0,%i3
	bneg    Aret           
	subcc   %i2,0,%i2
	be      Azer           
	sll     %i0,1,%l0
	orcc    %l0,%i1,%l0
	bne     Amin1    
	NOP
	ba,a    Azer     
i1:	subcc   %i2,0,%i2
	be      i3       
	NOP
	subcc   %g0,%i1,%i1
	subx    %g0,%i0,%i0
i3:	and     %i3,0x1f,%l0
	or      %g0,1,%i5
	sll     %i5,%l0,%i5
	sub     %g0,%i5,%i5
	subcc   %i3,0x20,%g0
	bcs     i4       
	NOP
	or      %g0,0,%i1
	ba      e4       
	and     %i0,%i5,%i0
i4:	and     %i1,%i5,%i1
e4:	subcc   %i2,0,%i2
	be      Aret     
	NOP
	subcc   %g0,%i1,%i1
	subx    %g0,%i0,%i0

Aret:
A999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

Aspec:	sethi   %hi(0xfff00000),%l1
	andn    %i0,%l1,%l0
	orcc    %l0,%i1,%l0
	bne     Anan     
	NOP
	ba,a    Aret     

Amin1:	sethi   %hi(0xbff00000),%i0
	ba      Aret     
	or      %g0,0,%i1

Azer:	or      %g0,%i2,%i0
	ba      Aret     
	or      %g0,0,%i1

Anan:	sethi   %hi(0xfff80000),%i0
	ba      Aret     
	or      %g0,0,%i1

!	.end
