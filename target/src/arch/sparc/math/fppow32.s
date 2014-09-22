/* fppow32.s - VxWorks conversion from Microtek tools to Sun native */

/* Copyright 1984-1992 Wind River Systems, Inc. */

/*
modification history
--------------------
01b,13nov92,jwt  added temporary (until USS patch) fix for pow(0.0,0.0) = 0.0.
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

	.global	_fpexlog
	.global	_fpexexp

	.text
	.align	4

	.global	_fppow
_fppow:
	.global	_powf
_powf:
	save	%sp,-96,%sp

	/* Test for positive or negative zero */

	set	0x7F800000,%l4		/* exponent (30-23) */
	andcc	%l4,%i0,%g0
	bne	powNonZero

	andcc	%l4,%i1,%g0
	bne	powNonZero
	nop

	set	0x3F300000,%i0		/* return 1.0 */
	ret
	restore

powNonZero:

	srl     %i0,23,%l2
	and     %l2,0xff,%l2
	srl     %i1,23,%l5
	and     %l5,0xff,%l5
	sethi   %hi(0x80000000),%i2
	sll     %i0,8,%l1
	or      %l1,%i2,%l1
	sll     %i1,8,%l4
	or      %l4,%i2,%l4
	and     %i1,%i2,%l6
	and     %i2,%i0,%i2

	sub     %l2,1,%l7
	subcc   %l7,0xfe,%g0
	bcc     AspecA         

Alab1:	sub     %l5,1,%l7
	subcc   %l7,0xfe,%g0
	bcc     AspecB   
	NOP
	sub     %l5,0x7e,%i4
	subcc   %i2,0,%i2
	be      i1             
	subcc   %i4,0,%i4
	ble     Anan           
	subcc   %i4,0x20,%g0
	bcs     i2       
	NOP
	ba      i1       
	or      %g0,0,%i2
i2:	sll     %l4,%i4,%l7
	subcc   %l7,0,%l7
	bne     Anan           
	sub     %i4,1,%g1
	sll     %l4,%g1,%l7
	subcc   %l7,0,%l7
	be,a	i1                
	or      %g0,0,%i2
i1:	subcc   %i4,0xc,%g0
	bcc     i4             
	sll     %l4,%i4,%l7
	subcc   %l7,0,%l7
	be      Apow20         

i4:	or      %g0,%l1,%o0
	or      %g0,%l2,%o1
	call    _fpexlog
	or      %g0,%l3,%o2
	or      %g0,%o0,%l1
	or      %g0,%o1,%l2
	or      %g0,%o2,%l3

	sub     %l2,0x7e,%l7
	add     %l5,%l7,%l5
	xor     %l6,%l3,%l6
	umul	%l1,%l4,%i3
	rd	%y,%l4
	subcc   %l4,0,%l4
	ble     i5       
	NOP
	addcc   %i3,%i3,%i3
	addx    %l4,%l4,%l4
	sub     %l5,1,%l5
i5:	addcc   %i3,%i3,%i3
	addx    %l4,0,%l4

	or      %g0,%l4,%o0
	or      %g0,%l5,%o1
	call    _fpexexp
	or      %g0,%l6,%o2
	or      %g0,%o0,%l1
	or      %g0,%o1,%l2
	or      %g0,%o2,%l3

Aret2:	sub     %l2,1,%l7
	subcc   %l7,0xfe,%g0
	bcs     i6             
	subcc   %l2,0,%l2
	bg      Ainf     
	NOP
	or      %g0,9,%i3
	sub     %i3,%l2,%i3
	subcc	%i3,32,%g0
	bcc,a	L1             
	or	%g0,0,%l1
L1:	srl	%l1,%i3,%l1
	ba      A999     
	or      %l1,%i2,%i0
i6:	sll     %l1,1,%l1
	srl     %l1,9,%l1
	sll     %l2,23,%l7
	or      %l1,%l7,%l1
	or      %l1,%i2,%i0

A999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

Apow20:	or      %g0,0x20,%l7
	sub     %l7,%i4,%l7
	srl     %l4,%l7,%l0
	or      %g0,%l1,%l4
	or      %g0,%l2,%l5
	andcc   %l0,1,%l7
	bne     i7       
	NOP
	sethi   %hi(0x80000000),%l1
	or      %g0,0x7f,%l2
i7:	srl     %l0,1,%l0

b8:	subcc   %l0,0,%l0
	be      i8       
	NOP
	sll     %l5,1,%l5
	sub     %l5,0x7e,%l5
	umul	%l4,%l4,%l4
	rd	%y,%l4
	subcc   %l4,0,%l4
	ble     i9       
	NOP
	sll     %l4,1,%l4
	sub     %l5,1,%l5
i9:	andcc   %l0,1,%l7
	be      i10      
	NOP
	sub     %l5,0x7e,%l7
	add     %l2,%l7,%l2
	umul	%l1,%l4,%l1
	rd	%y,%l1
	subcc   %l1,0,%l1
	ble     i10      
	NOP
	sll     %l1,1,%l1
	sub     %l2,1,%l2
i10:	ba      b8       
	srl     %l0,1,%l0
i8:	subcc   %l6,0,%l6
	be      Aret2    
	NOP
	sethi   %hi(0x40000000),%l4
	or      %g0,0xfe,%l7
	sub     %l7,%l2,%l2
	or      %g0,%l1,%i5

	wr	%l4,0,%y
	or	%g0,0x0,%l1
	orcc	%g0,0,%g0
#if 0
	divscc	%l1,%i5,%l1
	divscc	%l1,%i5,%l1
	divscc	%l1,%i5,%l1
	divscc	%l1,%i5,%l1
	divscc	%l1,%i5,%l1
	divscc	%l1,%i5,%l1
	divscc	%l1,%i5,%l1
	divscc	%l1,%i5,%l1
	divscc	%l1,%i5,%l1
	divscc	%l1,%i5,%l1
	divscc	%l1,%i5,%l1
	divscc	%l1,%i5,%l1
	divscc	%l1,%i5,%l1
	divscc	%l1,%i5,%l1
	divscc	%l1,%i5,%l1
	divscc	%l1,%i5,%l1
	divscc	%l1,%i5,%l1
	divscc	%l1,%i5,%l1
	divscc	%l1,%i5,%l1
	divscc	%l1,%i5,%l1
	divscc	%l1,%i5,%l1
	divscc	%l1,%i5,%l1
	divscc	%l1,%i5,%l1
	divscc	%l1,%i5,%l1
	divscc	%l1,%i5,%l1
	divscc	%l1,%i5,%l1
	divscc	%l1,%i5,%l1
	divscc	%l1,%i5,%l1
	divscc	%l1,%i5,%l1
	divscc	%l1,%i5,%l1
	divscc	%l1,%i5,%l1
	divscc	%l1,%i5,%l1
#else
	DIVSCC | (lo1 << 14) | in5 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in5 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in5 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in5 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in5 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in5 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in5 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in5 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in5 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in5 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in5 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in5 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in5 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in5 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in5 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in5 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in5 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in5 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in5 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in5 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in5 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in5 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in5 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in5 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in5 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in5 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in5 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in5 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in5 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in5 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in5 | (lo1 << 25)
	DIVSCC | (lo1 << 14) | in5 | (lo1 << 25)
#endif

	subcc   %l1,0,%l1
	ble     Aret2    
	NOP
	sll     %l1,1,%l1
	ba      Aret2    
	sub     %l2,1,%l2

AspecA:	subcc   %l2,0,%l2
	bne     i14      
	NOP
	addcc   %l1,%l1,%l1
	be      i14      
	NOP
/*	scan	%l1,0,%i3	*/
	SCAN | (lo1 << 14) | 0 | (in3 << 25)
	sll	%l1,%i3,%l1
	subcc	%l2,%i3,%l2
	ba,a    Alab1    
i14:	addcc   %i1,%i1,%l7
	be      Anan           
	subcc   %l2,0,%l2
	bne     i16            
	subcc   %l5,0xff,%g0
	be      Anan           
	subcc   %i1,0,%i1
	bneg    Ainf     
	NOP
	ba,a    Azer     
i16:	addcc   %l1,%l1,%l7
	bne     Anan           
	subcc   %l5,0x7f,%g0
	bcc     Alab1    
	NOP
	or      %g0,0x7f,%l5
	ba      Alab1    
	or      %g0,-1,%l4

AspecB:	add     %l4,%l4,%l4
	subcc   %l5,0,%l5
	bne     i18            
	subcc   %l4,0,%l4
	be      Aone           
	subcc   %i2,0,%i2
	be      Aone           
i18:	subcc   %l4,0,%l4
	bne     Anan     
	NOP
	ba,a    Aovr     

Ainf:	sethi   %hi(0x7f800000),%l7
	ba      A999     
	or      %l7,%i2,%i0

Anan:	ba      A999     
	sethi   %hi(0xffc00000),%i0

Azer:	ba      A999     
	or      %g0,%i2,%i0

Aone:	ba      A999     
	sethi   %hi(0x3f800000),%i0

Aovr:	subcc   %i2,0,%i2
	bne     Anan           
	subcc   %i1,0,%i1
	bpos    i19            
	subcc   %l2,0x7f,%g0
	bge     Azer     
	NOP
	ba,a    Ainf     
i19:	subcc   %l2,0x7f,%g0
	bge     Ainf     
	NOP
	ba,a    Azer     

!	.end
