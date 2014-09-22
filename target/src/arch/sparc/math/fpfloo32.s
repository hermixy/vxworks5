/* fpfloo32.s - VxWorks conversion from Microtek tools to Sun native */

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

	.global	_fpfloor
_fpfloor:
	.global	_floorf
_floorf:
	save	%sp,-96,%sp

	sethi   %hi(0x80000000),%i4
	and     %i4,%i0,%i1
	sll     %i0,1,%i3
	srl     %i3,24,%i3
	subcc   %i3,0xff,%g0
	be      Aspec    
	NOP
	or      %g0,0x96,%i4
	sub     %i4,%i3,%i3
	subcc   %i3,0x18,%g0
	bcs     i1             
	subcc   %i3,0,%i3
	bneg    Aret           
	sethi   %hi(0x80000000),%i4
	subcc   %i0,%i4,%g0
	bgu     Amin1    
	NOP
	ba,a    Azer     
i1:	subcc   %i1,0,%i1
	bne,a	i2               
	sub     %g0,%i0,%i0
i2:	or      %g0,1,%i2
	sll     %i2,%i3,%i2
	sub     %g0,%i2,%i2
	and     %i0,%i2,%i0
	subcc   %i1,0,%i1
	bne,a	Aret             
	sub     %g0,%i0,%i0

Aret:
A999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

Aspec:	sll     %i0,9,%i4
	subcc   %i4,0,%i4
	bne     Anan     
	NOP
	ba,a    A999     

Amin1:	ba      A999     
	sethi   %hi(0xbf800000),%i0

Azer:	ba      A999     
	or      %g0,%i1,%i0

Anan:	ba      A999     
	sethi   %hi(0xffc00000),%i0

!	.end
