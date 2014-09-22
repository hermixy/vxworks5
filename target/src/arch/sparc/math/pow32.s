/* pow32.s - VxWorks conversion from Microtek tools to Sun native */

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

	.global	_exlog
	.global	_exmul
	.global	_exexp
	.global	_exdiv

	.text
	.align	4

	.global	_dppow
_dppow:
	.global	_pow
_pow:
	save	%sp,-96,%sp

	/* Test for positive or negative zero */

	set	0x7FF00000,%l4		/* exponent (30-20) */
	andcc	%l4,%i0,%g0
	bne	powNonZero

	andcc	%l4,%i2,%g0
	bne	powNonZero
	nop

	set	0x3FF00000,%i0		/* return 1.0 */
	set	0,%i1
	ret
	restore

powNonZero:

	sll     %i0,1,%l4
	srl     %l4,21,%l4
	sll     %i0,11,%l2
	srl     %i1,21,%g1
	or      %l2,%g1,%l2
	sll     %i1,11,%l3
	sethi   %hi(0x80000000),%l0
	or      %l2,%l0,%l2
	sll     %i2,1,%l7
	srl     %l7,21,%l7
	sll     %i2,11,%l5
	srl     %i3,21,%g1
	or      %l5,%g1,%l5
	sll     %i3,11,%l6
	or      %l5,%l0,%l5
	and     %l0,%i0,%l0

	sub     %l4,1,%g1
	subcc   %g1,0x7fe,%g0
	bcc     AspecA         

Alab1:	sub     %l7,1,%g1
	subcc   %g1,0x7fe,%g0
	bcc     AspecB   
	NOP

Alab2:	sub     %l7,0x3fe,%i4
	subcc   %l0,0,%l0
	be      i1             
	subcc   %i4,0,%i4
	ble     Anan           
	subcc   %i4,0x40,%g0
	ble     i2       
	NOP
	or      %g0,0,%i5
	ba      e2       
	or      %g0,%i5,%l1
i2:	subcc   %i4,0x20,%g0
	ble     i3       
	NOP
	sub     %i4,0x21,%g1
	sll     %l6,%g1,%i5
	ba      e2       
	sll     %i5,1,%l1
i3:	sub     %i4,1,%g1
	sll     %l5,%g1,%i5
	sll     %i5,1,%l1
	or      %l1,%l6,%l1
e2:	subcc   %l1,0,%l1
	bne     Anan           
	subcc   %i5,0,%i5
	bpos,a	i1              
	or      %g0,0,%l0
i1:	subcc   %i4,0xf,%g0
	bcc     i5             
	sll     %l5,%i4,%g1
	orcc    %g1,%l6,%g1
	be      Apow20   
	NOP

i5:	sethi   %hi(0x80000000),%g1
	and     %g1,%i2,%i4
	subcc   %l4,0x3ff,%g0
	bge     i6       
	NOP
	sethi   %hi(0x80000000),%g1
	xor     %i4,%g1,%i4

i6:	or      %g0,%l2,%o0
	or      %g0,%l3,%o1
	call    _exlog
	or      %g0,%l4,%o2

	or      %g0,%l5,%o3
	or      %g0,%l6,%o4
	call    _exmul
	or      %g0,%l7,%o5

	call    _exexp
	or      %g0,%i4,%o3
	or      %g0,%o0,%l2
	or      %g0,%o1,%l3
	or      %g0,%o2,%l4

Aret2:	sub     %l4,1,%g1
	subcc   %g1,0x7fe,%g0
	bcc     Aundove  
	NOP
	srl     %l3,11,%i1
	sll     %l2,21,%g1
	or      %i1,%g1,%i1
	srl     %l2,11,%i0
	sethi   %hi(0xfffff),%g1
	or      %g1,0x3ff,%g1
	and     %i0,%g1,%i0
	sll     %l4,20,%g1
	or      %i0,%g1,%i0

Aret:	or      %i0,%l0,%i0

Aret3:
A999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

Apow20:	or      %g0,0x20,%g1
	sub     %g1,%i4,%g1
	srl     %l5,%g1,%l1
	or      %g0,%l2,%l5
	or      %g0,%l3,%l6
	or      %g0,%l4,%l7
	andcc   %l1,1,%g1
	bne     i7       
	NOP
	sethi   %hi(0x80000000),%l2
	or      %g0,0,%l3
	or      %g0,0x3ff,%l4
i7:	srl     %l1,1,%l1

b8:	subcc   %l1,0,%l1
	be      i8             

	or      %g0,%l5,%o0
	or      %g0,%l6,%o1
	or      %g0,%l7,%o2
	or      %g0,%l5,%o3
	or      %g0,%l6,%o4
	call    _exmul
	or      %g0,%l7,%o5
	or      %g0,%o0,%l5
	or      %g0,%o1,%l6
	or      %g0,%o2,%l7

	andcc   %l1,1,%g1
	be      i9             

	or      %g0,%l2,%o0
	or      %g0,%l3,%o1
	or      %g0,%l4,%o2
	or      %g0,%l5,%o3
	or      %g0,%l6,%o4
	call    _exmul
	or      %g0,%l7,%o5
	or      %g0,%o0,%l2
	or      %g0,%o1,%l3
	or      %g0,%o2,%l4

i9:	ba      b8       
	srl     %l1,1,%l1
i8:	subcc   %i2,0,%i2
	bpos    Aret2          

	sethi   %hi(0x80000000),%o0
	or      %g0,0,%o1
	or      %g0,0x3ff,%o2
	or      %g0,%l2,%o3
	or      %g0,%l3,%o4
	call    _exdiv
	or      %g0,%l4,%o5
	or      %g0,%o0,%l2
	or      %g0,%o1,%l3
	ba      Aret2    
	or      %g0,%o2,%l4

AspecA:	subcc   %l4,0,%l4
	bne     i11      
	NOP
	addcc   %l3,%l3,%l3
	addx    %l2,%l2,%l2
	orcc    %l2,%l3,%g1
	be      i11      
	NOP

/*	scan	%l2,0,%i4	*/
	SCAN | (lo2 << 14) | 0 | (in4 << 25)
	subcc	%i4,63,%g0
	bne	L1             
	NOP
	or	%l3,0,%l2
	or	%g0,0,%l3
	sub	%l4,32,%l4
/*	scan	%l2,0,%i4	*/
	SCAN | (lo2 << 14) | 0 | (in4 << 25)
	subcc	%i4,63,%g0
	be,a	L1             
	or	%i4,0,%l4
L1:	subcc	%g0,%i4,%i5
	be	Alab1          
	sub	%l4,%i4,%l4
	sll	%l2,%i4,%l2
	srl	%l3,%i5,%i5
	or	%i5,%l2,%l2
	sll	%l3,%i4,%l3

	ba,a    Alab1    
i11:	sll     %i2,1,%g1
	orcc    %g1,%i3,%g1
	be      Anan           
	subcc   %l4,0,%l4
	bne     i13            
	subcc   %l7,0x7ff,%g0
	be      Anan           
	subcc   %i2,0,%i2
	bneg    Ainf     
	NOP
	ba,a    Azer     
i13:	sll     %l2,1,%g1
	orcc    %g1,%l3,%g1
	bne     Anan           
	subcc   %l7,0x3ff,%g0
	bcc     Alab1    
	NOP
	or      %g0,0x3ff,%l7
	ba      Alab1    
	or      %g0,-1,%l5

AspecB:	subcc   %l7,0,%l7
	bne     i15            
	sll     %l5,1,%g1
	orcc    %g1,%l6,%g1
	be      Aone           
	subcc   %l0,0,%l0
	be      Aone           
i15:	sll     %l5,1,%g1
	orcc    %g1,%l6,%g1
	bne     Anan     
	NOP
	ba,a    Aovr     

Ainf:	sethi   %hi(0x7ff00000),%i0
	ba      Aret     
	or      %g0,0,%i1

Anan:	sethi   %hi(0xfff80000),%i0
	ba      Aret3    
	or      %g0,0,%i1

Azer:	or      %g0,0,%i0
	ba      Aret     
	or      %g0,0,%i1

Aone:	sethi   %hi(0x3ff00000),%i0
	ba      Aret3    
	or      %g0,0,%i1

Aovr:	subcc   %l0,0,%l0
	bne     Anan           
	subcc   %i2,0,%i2
	bpos    i16            
	subcc   %l4,0x3ff,%g0
	bge     Azer     
	NOP
	ba,a    Ainf     
i16:	subcc   %l4,0x3ff,%g0
	bge     Ainf     
	NOP
	ba,a    Azer     

Aundove:
	subcc   %l4,0,%l4
	bg      Ainf     
	NOP
	or      %g0,0xc,%i4
	sub     %i4,%l4,%i4

L3:	subcc	%i4,32,%g0
	bcs	L4             
	subcc	%g0,%i4,%i5
	sub	%i4,32,%i4
	orcc	%l2,0,%l3
	bne	L3             
	or	%g0,0,%l2
L4:	be	L5             
	sll	%l2,%i5,%i5
	srl	%l3,%i4,%l3
	srl	%l2,%i4,%l2
	or	%l3,%i5,%l3

L5:	or      %g0,%l2,%i0
	ba      Aret     
	or      %g0,%l3,%i1

!	.end
