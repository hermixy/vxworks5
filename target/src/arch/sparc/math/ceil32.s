/* ceil32.s - VxWorks conversion from Microtek tools to Sun native */

/* Copyright 1984-1992 Wind River Systems, Inc. */

/*
modification history
--------------------
01b,10nov92,jwt  added temporary (until USS patch) fix for ceil(+0.0) = 1.0.
01a,05aug92,jwt  converted.
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
	.text
*/

#include "arch/sparc/ussSun4.h"

	.global	_dpceil
_dpceil:
	.global	_ceil
_ceil:
	save	%sp,-96,%sp

	/* Test for positive zero */

	set	0xFFF00000,%l1		/* sign (31) and exponent (30-20) */
	andcc	%l1,%i0,%l1
	bne	ceilNonZero
	nop

	clr	%i0			/* return value */
	clr	%i1
	ret
	restore

ceilNonZero:

	sethi   %hi(0x80000000),%l1
	and     %l1,%i0,%i4
	sll     %i0,1,%i3
	srl     %i3,21,%i3
	subcc   %i3,0x7ff,%g0
	be      Aspec    
	NOP
	or      %g0,0x433,%l1
	sub     %l1,%i3,%i3
	subcc   %i3,0,%i3
	ble     Aret           
	subcc   %i3,0x35,%g0
	bl      i1             
	subcc   %i4,0,%i4
	bne     Azer     
	NOP
	ba,a    Aone     
i1:	subcc   %i4,0,%i4
	bne     i2       
	NOP
	subcc   %g0,%i1,%i1
	subx    %g0,%i0,%i0
i2:	and     %i3,0x1f,%l1
	or      %g0,1,%l0
	sll     %l0,%l1,%l0
	sub     %g0,%l0,%l0
	subcc   %i3,0x20,%g0
	bl      i3       
	NOP
	or      %g0,0,%i1
	ba      e3       
	and     %i0,%l0,%i0
i3:	and     %i1,%l0,%i1

e3:	subcc   %i4,0,%i4
	bne     Aret     
	NOP
	subcc   %g0,%i1,%i1
	subx    %g0,%i0,%i0

Aret:
A999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

Aspec:	sethi   %hi(0xfff00000),%l2
	andn    %i0,%l2,%l1
	orcc    %l1,%i1,%l1
	bne     Anan     
	NOP
	ba,a    Aret     

Aone:	sethi   %hi(0x3ff00000),%i0
	ba      Aret     
	or      %g0,0,%i1

Azer:	or      %g0,%i4,%i0
	ba      Aret     
	or      %g0,0,%i1

Anan:	sethi   %hi(0xfff80000),%i0
	ba      Aret     
	or      %g0,0,%i1

!	.end
