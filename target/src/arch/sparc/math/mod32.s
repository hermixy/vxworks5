/* mod32.s - VxWorks conversion from Microtek tools to Sun native */

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

	.global	_exsub

	.text
	.align	4

	.global	_dpfmod
_dpfmod:
	.global	_fmod
_fmod:
	save	%sp,-96,%sp

	sll     %i0,1,%l0
	srl     %l0,21,%l0
	sll     %i0,11,%i4
	srl     %i1,21,%l7
	or      %i4,%l7,%i4
	sll     %i1,11,%i5
	sethi   %hi(0x80000000),%l4
	or      %i4,%l4,%i4
	sll     %i2,1,%l3
	srl     %l3,21,%l3
	sll     %i2,11,%l1
	srl     %i3,21,%l7
	or      %l1,%l7,%l1
	sll     %i3,11,%l2
	or      %l1,%l4,%l1
	and     %l4,%i0,%l4

	sub     %l0,1,%l7
	subcc   %l7,0x7fe,%g0
	bcc     AspecA         

Alab1:	sub     %l3,1,%l7
	subcc   %l7,0x7fe,%g0
	bcc     AspecB   
	NOP

Alab2:	subcc   %l0,%l3,%l5
	bneg    Aret2    
	NOP
	srl     %i5,1,%i5
	sll     %i4,31,%l7
	srl     %i4,1,%i4
	or      %i5,%l7,%i5
	srl     %l2,1,%l2
	sll     %l1,31,%l7
	srl     %l1,1,%l1
	or      %l2,%l7,%l2

b1:	subcc   %i5,%l2,%i5
	subxcc  %i4,%l1,%i4
	bcc     i2       
	NOP
	addcc   %i5,%l2,%i5
	addx    %i4,%l1,%i4
i2:	addcc   %i5,%i5,%i5
	addx    %i4,%i4,%i4
	subcc   %l5,0,%l5
	bne     b1       
	sub     %l5,1,%l5
	or      %g0,%l3,%l0

/*	scan	%i4,0,%l5	*/
	SCAN | (in4 << 14) | 0 | (lo5 << 25)
	subcc	%l5,63,%g0
	bne	L1             
	NOP
	or	%i5,0,%i4
	or	%g0,0,%i5
	sub	%l0,32,%l0
/*	scan	%i4,0,%l5	*/
	SCAN | (in4 << 14) | 0 | (lo5 << 25)
	subcc	%l5,63,%g0
	be,a	L1             
	or	%l5,0,%l0
L1:	subcc	%g0,%l5,%l6
	be	L2             
	sub	%l0,%l5,%l0
	sll	%i4,%l5,%i4
	srl	%i5,%l6,%l6
	or	%l6,%i4,%i4
	sll	%i5,%l5,%i5

L2:	subcc   %l0,0,%l0
	bg      i3       
	NOP
	or      %g0,1,%l5
	sub     %l5,%l0,%l5

L3:	subcc	%l5,32,%g0
	bcs	L4             
	subcc	%g0,%l5,%l6
	sub	%l5,32,%l5
	orcc	%i4,0,%i5
	bne	L3             
	or	%g0,0,%i4
L4:	be	L5             
	sll	%i4,%l6,%l6
	srl	%i5,%l5,%i5
	srl	%i4,%l5,%i4
	or	%i5,%l6,%i5

L5:	or      %g0,0,%l0
i3:	srl     %i5,11,%i1
	sll     %i4,21,%l7
	or      %i1,%l7,%i1
	srl     %i4,11,%i0
	sethi   %hi(0xfffff),%l7
	or      %l7,0x3ff,%l7
	and     %i0,%l7,%i0
	sll     %l0,20,%l7
	or      %i0,%l7,%i0
	or      %i0,%l4,%i0

Aret2:
A999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

AspecA:	subcc   %l0,0,%l0
	bne     Anan     
	NOP
	addcc   %i5,%i5,%i5
	addx    %i4,%i4,%i4
	orcc    %i4,%i5,%l7
	be      Alab1    
	NOP

/*	scan	%i4,0,%l5	*/
	SCAN | (in4 << 14) | 0 | (lo5 << 25)
	subcc	%l5,63,%g0
	bne	L6             
	NOP
	or	%i5,0,%i4
	or	%g0,0,%i5
	sub	%l0,32,%l0
/*	scan	%i4,0,%l5	*/
	SCAN | (in4 << 14) | 0 | (lo5 << 25)
	subcc	%l5,63,%g0
	be,a	L6             
	or	%l5,0,%l0
L6:	subcc	%g0,%l5,%l6
	be	Alab1          
	sub	%l0,%l5,%l0
	sll	%i4,%l5,%i4
	srl	%i5,%l6,%l6
	or	%l6,%i4,%i4
	sll	%i5,%l5,%i5

	ba,a    Alab1    

AspecB:	subcc   %l3,0,%l3
	bne     i5       
	NOP
	addcc   %l2,%l2,%l2
	addx    %l1,%l1,%l1
	orcc    %l1,%l2,%l7
	be      Anan     
	NOP

/*	scan	%l1,0,%l5	*/
	SCAN | (lo1 << 14) | 0 | (lo5 << 25)
	subcc	%l5,63,%g0
	bne	L8             
	NOP
	or	%l2,0,%l1
	or	%g0,0,%l2
	sub	%l3,32,%l3
/*	scan	%l1,0,%l5	*/
	SCAN | (lo1 << 14) | 0 | (lo5 << 25)
	subcc	%l5,63,%g0
	be,a	L8             
	or	%l5,0,%l3
L8:	subcc	%g0,%l5,%l6
	be	Alab2          
	sub	%l3,%l5,%l3
	sll	%l1,%l5,%l1
	srl	%l2,%l6,%l6
	or	%l6,%l1,%l1
	sll	%l2,%l5,%l2

	ba,a    Alab2    
i5:	sll     %l1,1,%l7
	orcc    %l7,%l2,%l7
	bne     Anan     
	NOP
	ba,a    Aret2    

Anan:	sethi   %hi(0xfff80000),%i0
	ba      Aret2    
	or      %g0,0,%i1

Azer:	or      %g0,%l4,%i0
	ba      Aret2    
	or      %g0,0,%i1

	.global	_dpfrexp
_dpfrexp:
	.global	_frexp
_frexp:
	save	%sp,-96,%sp

	sll     %i0,1,%i3
	srl     %i3,21,%i3
	sub     %i3,1,%i5
	subcc   %i5,0x7fe,%g0
	bcc     Bspec    
	NOP

Blab1:	sub     %i3,0x3fe,%i3
	sethi   %hi(0x800fffff),%i5
	or      %i5,0x3ff,%i5
	and     %i0,%i5,%i0
	sethi   %hi(0x3fe00000),%i5
	or      %i0,%i5,%i0

Bret:	st      %i3,[%i2]

B999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

Bspec:	subcc   %i3,0,%i3
	bne     i7       
	NOP
	sethi   %hi(0x80000000),%i5
	and     %i5,%i0,%i4
	sll     %i0,1,%i5
	orcc    %i5,%i1,%i5
	be      Bret     
	NOP
	add     %i3,1,%i3

Blab6:	sub     %i3,1,%i3
	addcc   %i1,%i1,%i1
	addx    %i0,%i0,%i0
	sethi   %hi(0x100000),%i5
	subcc   %i0,%i5,%g0
	bcs     Blab6    
	NOP
	ba      Blab1    
	or      %i0,%i4,%i0
i7:	sethi   %hi(0xfff80000),%i0
	or      %g0,0,%i1
	ba      Bret     
	or      %g0,%i1,%i3

	.global	_dpldexp
_dpldexp:
	.global	_ldexp
_ldexp:
	save	%sp,-96,%sp

	sll     %i0,1,%l0
	srl     %l0,21,%l0
	sub     %l0,1,%l3
	subcc   %l3,0x7fe,%g0
	bcc     Cspec    
	NOP
	add     %l0,%i2,%l0
	sub     %l0,1,%l3
	subcc   %l3,0x7fe,%g0
	bcc     Cspec2   
	NOP
	sethi   %hi(0x800fffff),%l3
	or      %l3,0x3ff,%l3
	and     %i0,%l3,%i0
	sll     %l0,20,%l3
	or      %i0,%l3,%i0

C999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

Cspec:	subcc   %l0,0x7ff,%g0
	bne     Cspec2         
	sethi   %hi(0xfff00000),%l4
	andn    %i0,%l4,%l3
	orcc    %l3,%i1,%l3
	be      Cret     
	NOP
	sethi   %hi(0xfff80000),%i0
	ba      Cret     
	or      %g0,0,%i1

Cspec2:	sll     %i0,1,%i5
	srl     %i5,21,%i5
	sll     %i0,11,%i3
	srl     %i1,21,%l3
	or      %i3,%l3,%i3
	sll     %i1,11,%i4
	sethi   %hi(0x80000000),%l1
	or      %i3,%l1,%i3
	and     %l1,%i0,%l1
	subcc   %i5,0,%i5
	bne     i11      
	NOP
	addcc   %i4,%i4,%i4
	addx    %i3,%i3,%i3
	orcc    %i3,%i4,%l3
	be      Cret     
	NOP

/*	scan	%i3,0,%l2	*/
	SCAN | (in3 << 14) | 0 | (lo2 << 25)
	subcc	%l2,63,%g0
	bne	L10             
	NOP
	or	%i4,0,%i3
	or	%g0,0,%i4
	sub	%i5,32,%i5
/*	scan	%i3,0,%l2	*/
	SCAN | (in3 << 14) | 0 | (lo2 << 25)
	subcc	%l2,63,%g0
	be,a	L10             
	or	%l2,0,%i5
L10:	subcc	%g0,%l2,%l0
	be	i11             
	sub	%i5,%l2,%i5
	sll	%i3,%l2,%i3
	srl	%i4,%l0,%l0
	or	%l0,%i3,%i3
	sll	%i4,%l2,%i4
	ba,a    i11      

i11:	add     %i5,%i2,%i5
	sub     %i5,1,%l3
	subcc   %l3,0x7fe,%g0
	bcs     Cret2          
	subcc   %i5,0,%i5
	ble     i13      
	NOP
	sethi   %hi(0x7ff00000),%i0
	ba      Cret1    
	or      %g0,0,%i1
i13:	or      %g0,1,%l2
	sub     %l2,%i5,%l2

L12:	subcc	%l2,32,%g0
	bcs	L13             
	orcc	%i4,0,%g0
	sub	%l2,32,%l2
	or	%i3,0,%i4
	or	%g0,0,%i3
	be	L14             
	orcc	%i4,0,%g0
	or	%i4,2,%i4
L14:	bne	L12             
	NOP
	ba,a	L15             
L13:	subcc	%g0,%l2,%i0
	be	L15             
	sll	%i4,%i0,%i1
	orcc	%i1,0,%g0
	sll	%i3,%i0,%i0
	srl	%i4,%l2,%i4
	srl	%i3,%l2,%i3
	or	%i4,%i0,%i4
	bne,a	L15             
	or	%i4,2,%i4

L15:	addcc   %i4,0x400,%i4
	addxcc  %i3,0,%i3
	addx    %i5,0,%i5
	srl     %i4,11,%l3
	and     %l3,1,%l3
	sub     %i4,%l3,%i4
	or      %g0,0,%i5

Cret2:	srl     %i4,11,%i1
	sll     %i3,21,%l3
	or      %i1,%l3,%i1
	srl     %i3,11,%i0
	sethi   %hi(0xfffff),%l3
	or      %l3,0x3ff,%l3
	and     %i0,%l3,%i0
	sll     %i5,20,%l3
	or      %i0,%l3,%i0

Cret1:	or      %i0,%l1,%i0

Cret:
	ba,a    C999     

	.global	_dpmodf
_dpmodf:
	.global	_modf
_modf:
	save	%sp,-96,%sp

	sll     %i0,1,%i5
	srl     %i5,21,%i5
	sll     %i0,11,%i3
	srl     %i1,21,%l6
	or      %i3,%l6,%i3
	sll     %i1,11,%i4
	sethi   %hi(0x80000000),%l4
	or      %i3,%l4,%i3
	and     %l4,%i0,%l4
	sub     %i5,1,%l6
	subcc   %l6,0x7fe,%g0
	bcc     Dspec    
	NOP
	or      %g0,%i3,%l0
	or      %g0,%i4,%l1
	or      %g0,%i5,%l2
	or      %g0,0x43e,%l3
	sub     %l3,%i5,%l3
	subcc   %l3,0,%l3
	ble     Dlab3          
	subcc   %l3,0x40,%g0
	bge     Dzer     
	NOP
	and     %l3,0x1f,%l6
	or      %g0,1,%l5
	sll     %l5,%l6,%l5
	sub     %g0,%l5,%l5
	subcc   %l3,0x20,%g0
	bl      i14      
	NOP
	or      %g0,0,%l1
	ba      Dlab3    
	and     %l0,%l5,%l0
i14:	and     %l1,%l5,%l1

Dlab3:	srl     %l1,11,%i1
	sll     %l0,21,%l6
	or      %i1,%l6,%i1
	srl     %l0,11,%i0
	sethi   %hi(0xfffff),%l6
	or      %l6,0x3ff,%l6
	and     %i0,%l6,%i0
	sll     %l2,20,%l6
	or      %i0,%l6,%i0
	or      %i0,%l4,%i0
	st      %i0,[%i2]
	st      %i1,[%i2+4]

	or      %g0,%i3,%o0
	or      %g0,%i4,%o1
	or      %g0,%i5,%o2
	or      %g0,%l0,%o3
	or      %g0,%l1,%o4
	call    _exsub
	or      %g0,%l2,%o5
	or      %g0,%o0,%i3
	or      %g0,%o1,%i4
	or      %g0,%o2,%i5

Dret:	srl     %i4,11,%i1
	sll     %i3,21,%l6
	or      %i1,%l6,%i1
	srl     %i3,11,%i0
	sethi   %hi(0xfffff),%l6
	or      %l6,0x3ff,%l6
	and     %i0,%l6,%i0
	sll     %i5,20,%l6
	or      %i0,%l6,%i0
	or      %i0,%l4,%i0

Dret2:
D999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

Dzer:	st      %g0,[%i2+4]
	ba      Dret2    
	st      %l4,[%i2]

Dspec:	subcc   %i5,0,%i5
	be      Dzer     
	NOP
	or      %g0,0,%i1
	st      %i1,[%i2+4]
	sethi   %hi(0xfff80000),%i0
	ba      Dret2    
	st      %i0,[%i2]

!	.end
