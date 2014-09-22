/* fpmod32.s - VxWorks conversion from Microtek tools to Sun native */

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

	.global	_fpfmod
_fpfmod:
	.global	_fmodf
_fmodf:
	save	%sp,-96,%sp

	srl     %i0,23,%i3
	and     %i3,0xff,%i3
	srl     %i1,23,%l0
	and     %l0,0xff,%l0
	sethi   %hi(0x80000000),%l2
	sll     %i0,8,%i2
	or      %i2,%l2,%i2
	sll     %i1,8,%i5
	or      %i5,%l2,%i5
	and     %i0,%l2,%i4

	sub     %i3,1,%l3
	subcc   %l3,0xfe,%g0
	bcc     AspecA         

Alab1:	sub     %l0,1,%l3
	subcc   %l3,0xfe,%g0
	bcc     AspecB   
	NOP

Alab2:	subcc   %i3,%l0,%l2
	bpos    i1       
	NOP
	ba,a    A999     
i1:	srl     %i2,1,%i2
	srl     %i5,1,%i5

b2:	subcc   %i2,%i5,%i2
	bcs,a	i3               
	addcc   %i2,%i5,%i2
i3:	addcc   %i2,%i2,%i2
	subcc   %l2,0,%l2
	bne     b2       
	sub     %l2,1,%l2
	subcc   %i2,0,%i2
	be      Azer     
	NOP
/*	scan	%i2,0,%l2	*/
	SCAN | (in2 << 14) | 0 | (lo2 << 25)
	sll	%i2,%l2,%i2
	subcc	%l0,%l2,%l0
	subcc   %l0,0,%l0
	bg      i4       
	NOP
	or      %g0,9,%l2
	sub     %l2,%l0,%l2
	subcc	%l2,32,%g0
	bcc,a	L1             
	or	%g0,0,%i2
L1:	srl	%i2,%l2,%i2
	ba,a    Aret     
i4:	sll     %i2,1,%i2
	srl     %i2,9,%i2
	sll     %l0,23,%l3
	or      %i2,%l3,%i2

Aret:	or      %i2,%i4,%i0

A999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

AspecA:	subcc   %i3,0,%i3
	bne     Anan     
	NOP
	addcc   %i2,%i2,%i2
	be      Alab1    
	NOP
/*	scan	%i2,0,%l2	*/
	SCAN | (in2 << 14) | 0 | (lo2 << 25)
	sll	%i2,%l2,%i2
	subcc	%i3,%l2,%i3
	ba,a    Alab1    

AspecB:	add     %i5,%i5,%i5
	subcc   %l0,0,%l0
	bne     i6             
	subcc   %i5,0,%i5
	be      Anan     
	NOP
/*	scan	%i5,0,%l2	*/
	SCAN | (in5 << 14) | 0 | (lo2 << 25)
	sll	%i5,%l2,%i5
	subcc	%l0,%l2,%l0
	ba,a    Alab2    
i6:	subcc   %i5,0,%i5
	bne     Anan     
	NOP
	ba,a    A999     

Anan:	ba      A999     
	sethi   %hi(0xffc00000),%i0

Azer:	ba      A999     
	or      %g0,%i4,%i0

	.global	_fpfrexp
_fpfrexp:
	.global	_frexpf
_frexpf:
	save	%sp,-96,%sp

	srl     %i0,23,%i2
	and     %i2,0xff,%i2
	sub     %i2,1,%i5
	subcc   %i5,0xfe,%g0
	bcc     Bspec    
	NOP

Blab1:	sub     %i2,0x7e,%i2
	sethi   %hi(0x807fffff),%i5
	or      %i5,0x3ff,%i5
	and     %i0,%i5,%i0
	sethi   %hi(0x3f000000),%i5
	or      %i0,%i5,%i0

Bret:	st      %i2,[%i1]

B999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

Bspec:	subcc   %i2,0,%i2
	bne     i8             
	addcc   %i0,%i0,%i5
	be      Bret     
	NOP
	sll     %i0,9,%i3
/*	scan	%i3,0,%i4	*/
	SCAN | (in3 << 14) | 0 | (in4 << 25)
	sll	%i3,%i4,%i3
	subcc	%i2,%i4,%i2
	sethi   %hi(0x80000000),%i5
	and     %i5,%i0,%i0
	srl     %i3,8,%i5
	ba      Blab1    
	or      %i0,%i5,%i0
i8:	sethi   %hi(0xffc00000),%i0
	ba      Bret     
	or      %g0,0,%i2

	.global	_fpldexp
_fpldexp:
	.global	_ldexpf
_ldexpf:
	save	%sp,-96,%sp

	srl     %i0,23,%l0
	and     %l0,0xff,%l0
	sub     %l0,1,%l2
	subcc   %l2,0xfe,%g0
	bcc     Cspec    
	NOP
	add     %l0,%i1,%l0
	sub     %l0,1,%l2
	subcc   %l2,0xfe,%g0
	bcc     Cspec2   
	NOP
	sethi   %hi(0x807fffff),%l2
	or      %l2,0x3ff,%l2
	and     %i0,%l2,%i0
	sll     %l0,23,%l2
	or      %i0,%l2,%i0

C999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

Cspec:	subcc   %l0,0xff,%g0
	bne     Cspec2         
	sethi   %hi(0xff800000),%l3
	andncc  %i0,%l3,%l2
	bne,a	i10              
	sethi   %hi(0xffc00000),%i0
i10:
	ba,a    C999     

Cspec2:	srl     %i0,23,%i3
	andcc   %i3,0xff,%i3
	sethi   %hi(0x80000000),%i4
	sll     %i0,8,%i2
	or      %i2,%i4,%i2
	bne     i11      
	and     %i4,%i0,%i4
	addcc   %i2,%i2,%i2
	be      i12      
	NOP
/*	scan	%i2,0,%l1	*/
	SCAN | (in2 << 14) | 0 | (lo1 << 25)
	sll	%i2,%l1,%i2
	subcc	%i3,%l1,%i3
	ba,a    i11      
i12:
	ba,a    C999     
i11:	add     %i3,%i1,%i3
	sub     %i3,1,%l2
	subcc   %l2,0xfe,%g0
	bcs     Cret2          
	subcc   %i3,0,%i3
	ble     i13      
	NOP
	ba      Cret1    
	sethi   %hi(0x7f800000),%i0
i13:	or      %g0,1,%l1
	sub     %l1,%i3,%l1
	subcc   %l1,0x20,%g0
	bcs     i14      
	NOP
	ba      e14      
	or      %g0,2,%i2
i14:	or      %g0,%i2,%l0
	srl     %i2,%l1,%i2
	sll     %i2,%l1,%l2
	subcc   %l2,%l0,%g0
	bne,a	e14              
	or      %i2,2,%i2
e14:	addcc   %i2,0x80,%i2
	addx    %i3,0,%i3
	srl     %i2,8,%l2
	and     %l2,1,%l2
	sub     %i2,%l2,%i2
	or      %g0,0,%i3

Cret2:	sll     %i2,1,%i0
	srl     %i0,9,%i0
	sll     %i3,23,%l2
	or      %i0,%l2,%i0

Cret1:	ba      C999     
	or      %i0,%i4,%i0

	.global	_fpmodf
_fpmodf:
	.global	_modff
_modff:
	save	%sp,-96,%sp

	srl     %i0,23,%i3
	and     %i3,0xff,%i3
	sethi   %hi(0x80000000),%i4
	sll     %i0,8,%i2
	or      %i2,%i4,%i2
	and     %i4,%i0,%i4
	subcc   %i3,0xff,%g0
	be      Dinfnan  
	NOP
	or      %g0,%i2,%i5
	or      %g0,%i3,%l0
	or      %g0,%i4,%l1
	or      %g0,0x9e,%l2
	sub     %l2,%i3,%l2
	subcc   %l2,0,%l2
	ble     Dlab3          
	subcc   %l2,0x20,%g0
	bge     Dzer     
	NOP
	srl     %i5,%l2,%i5
	sll     %i5,%l2,%i5

Dlab3:	sll     %i5,1,%l2
	srl     %l2,9,%l2
	sll     %l0,23,%l3
	or      %l2,%l3,%l2
	or      %l2,%l1,%l3
	st      %l3,[%i1]
	sub     %i3,%l0,%l2
	subcc	%l2,32,%g0
	bcc,a	L2             
	or	%g0,0,%i5
L2:	srl	%i5,%l2,%i5
	subcc   %i2,%i5,%i2
	be      Dret2    
	NOP
/*	scan	%i2,0,%l2	*/
	SCAN | (in2 << 14) | 0 | (lo2 << 25)
	sll	%i2,%l2,%i2
	subcc	%i3,%l2,%i3

Dret:	sll     %i2,1,%i2
	srl     %i2,9,%i2
	sll     %i3,23,%l3
	or      %i2,%l3,%i2

Dret2:	or      %i2,%i4,%i0

D999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

Dzer:	ba      Dret     
	st      %i4,[%i1]

Dinfnan:
	sethi   %hi(0xffc00000),%i0
	ba      D999     
	st      %i0,[%i1]

!	.end
