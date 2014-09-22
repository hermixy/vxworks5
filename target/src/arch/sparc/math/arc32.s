/* arc32.s - VxWorks conversion from Microtek tools to Sun native */

/* Copyright 1984-1992 Wind River Systems, Inc. */

/*
modification history
--------------------
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
*/

#include "arch/sparc/ussSun4.h"

	.global	_exadd
	.global	_exsub
	.global	_exmul
	.global	_exdiv
	.global	_exsqrt
	.global	_sigmax

	.data
	.align	4
atncon:	.word	0x38e38e39,0xe38e38e
	.word	0xb6db6db7,0xedb6db6d
	.word	0x9999999a,0x19999999
	.word	0x55555555,0xd5555555
	.word	0x0,0x80000000
atntab:	.word	0x0,0x0,0x0
	.word	0xffeaaddd,0x4bb12542,0x3f9
	.word	0xffaaddb9,0x67ef4e37,0x3fa
	.word	0xbf70c130,0x17887460,0x3fb
	.word	0xfeadd4d5,0x617b6e33,0x3fb
	.word	0x9eb77746,0x331362c3,0x3fc
	.word	0xbdcbda5e,0x72d81134,0x3fc
	.word	0xdc86ba94,0x93051023,0x3fc
	.word	0xfadbafc9,0x6406eb15,0x3fc
	.word	0x8c5fad18,0x5f8bc131,0x3fd
	.word	0x9b13b9b8,0x3f5e5e6a,0x3fd
	.word	0xa9856cca,0x8e6a4eda,0x3fd
	.word	0xb7b0ca0f,0x26f78474,0x3fd
	.word	0xc59269ca,0x50d92b6e,0x3fd
	.word	0xd327761e,0x611fe5b6,0x3fd
	.word	0xe06da64a,0x764f7c68,0x3fd
	.word	0xed63382b,0xdda7b46,0x3fd
	.word	0xfa06e85a,0xa0a0be5c,0x3fd
	.word	0x832bf4a6,0xd9867e2a,0x3fe
	.word	0x892aecdf,0xde9547b5,0x3fe
	.word	0x8f005d5e,0xf7f59f9b,0x3fe
	.word	0x94ac72c9,0x847186f6,0x3fe
	.word	0x9a2f80e6,0x71bdda20,0x3fe
	.word	0x9f89fdc4,0xf4b7a1ed,0x3fe
	.word	0xa4bc7d19,0x34f70924,0x3fe
	.word	0xa9c7abdc,0x4830f5c8,0x3fe
	.word	0xaeac4c38,0xb4d8c080,0x3fe
	.word	0xb36b31c9,0x1f043691,0x3fe
	.word	0xb8053e2b,0xc2319e74,0x3fe
	.word	0xbc7b5dea,0xe98af281,0x3fe
	.word	0xc0ce85b8,0xac526640,0x3fe
	.word	0xc4ffaffa,0xbf8fbd54,0x3fe

	.text
	.align	4

	.global	_exatan
_exatan:
	save	%sp,-96,%sp

	or      %g0,0,%i3
	subcc   %i2,0x3ff,%g0
	bl      i1             

	sethi   %hi(0x80000000),%o0
	or      %g0,0,%o1
	or      %g0,0x3ff,%o2
	or      %g0,%i0,%o3
	or      %g0,%i1,%o4
	call    _exdiv
	or      %g0,%i2,%o5
	or      %g0,%o0,%i0
	or      %g0,%o1,%i1
	or      %g0,%o2,%i2

	or      %g0,1,%i3
	subcc   %i2,0x3ff,%g0
	bne     i1       
	NOP
	or      %g0,0x3fe,%i2
	or      %g0,-1,%i0
	or      %g0,%i0,%i1
i1:	or      %g0,%i0,%l2
	or      %g0,%i1,%l3
	or      %g0,%i2,%l4
	or      %g0,0x401,%l0
	sub     %l0,%i2,%l0
	subcc   %l0,8,%g0
	bcc,a	i3               
	or      %g0,8,%l0
i3:	srl     %i0,24,%l1
	srl     %l1,%l0,%l1
	sll     %l1,27,%i4
	or      %g0,8,%l6
	sub     %l6,%l0,%l6
	or      %g0,-1,%l5
	srl     %l5,%l6,%l5
	and     %i0,%l5,%i0

/*	scan	%i0,0,%i5	*/
	SCAN | (in0 << 14) | 0 | (in5 << 25)
	subcc	%i5,63,%g0
	bne	L1             
	NOP
	or	%i1,0,%i0
	or	%g0,0,%i1
	sub	%i2,32,%i2
/*	scan	%i0,0,%i5	*/
	SCAN | (in0 << 14) | 0 | (in5 << 25)
	subcc	%i5,63,%g0
	be,a	L1             
	or	%i5,0,%i2
L1:	subcc	%g0,%i5,%l0
	be	L2             
	sub	%i2,%i5,%i2
	sll	%i0,%i5,%i0
	srl	%i1,%l0,%l0
	or	%l0,%i0,%i0
	sll	%i1,%i5,%i1
L2:	umul	%i4,%l3,%i5	
	rd	%y,%i5
	umul	%i4,%l2,%l3	
	rd	%y,%l2
	addcc   %l3,%i5,%l3
	addx    %l2,0,%l2
	or      %g0,0x3ff,%i4
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

L5:	or      %g0,0x3ff,%l4
	sethi   %hi(0x80000000),%l5
	or      %l2,%l5,%l2

	or      %g0,%i0,%o0
	or      %g0,%i1,%o1
	or      %g0,%i2,%o2
	or      %g0,%l2,%o3
	or      %g0,%l3,%o4
	call    _exdiv
	or      %g0,%l4,%o5
	or      %g0,%o0,%i0
	or      %g0,%o1,%i1
	or      %g0,%o2,%i2

	or      %g0,%o0,%o3
	or      %g0,%o1,%o4
	call    _exmul
	or      %g0,%o2,%o5

	or      %g0,0,%o3
	sethi   %hi(atncon),%o4
	call    _sigmax
	or	%o4,%lo(atncon),%o4
	or      %g0,%o0,%l2
	or      %g0,%o1,%l3
	or      %g0,%o2,%l4

	or      %g0,%i0,%o0
	or      %g0,%i1,%o1
	or      %g0,%i2,%o2
	or      %g0,%l2,%o3
	or      %g0,%l3,%o4
	call    _exmul
	or      %g0,%l4,%o5

	sethi   %hi(atntab),%l6
	or	%l6,%lo(atntab),%l6
	sll     %l1,2,%l5
	add     %l6,%l5,%l6
	add     %l6,%l5,%l6
	add     %l6,%l5,%l6
	ld      [%l6],%o3
	ld      [%l6+4],%o4
	call    _exadd
	ld      [%l6+8],%o5
	or      %g0,%o0,%i0
	or      %g0,%o1,%i1
	or      %g0,%o2,%i2

	subcc   %i3,0,%i3
	be      i4             

	sethi   %hi(0xc90fdaa2),%o0
	or      %o0,0x2a2,%o0
	sethi   %hi(0x2168c235),%o1
	or      %o1,0x235,%o1
	or      %g0,0x3ff,%o2
	or      %g0,%i0,%o3
	or      %g0,%i1,%o4
	call    _exsub
	or      %g0,%i2,%o5
	or      %g0,%o0,%i0
	or      %g0,%o1,%i1
	or      %g0,%o2,%i2

i4:
A999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

	.global	_dpatan
_dpatan:
	.global	_atan
_atan:
	save	%sp,-96,%sp

	sll     %i0,1,%i4
	srl     %i4,21,%i4
	sll     %i0,11,%i2
	srl     %i1,21,%l0
	or      %i2,%l0,%i2
	sll     %i1,11,%i3
	sethi   %hi(0x80000000),%i5
	or      %i2,%i5,%i2
	and     %i5,%i0,%i5

	subcc   %i4,0x7ff,%g0
	bne     i5             
	sll     %i2,1,%l0
	orcc    %l0,%i3,%l0
	bne     Bnan           
i5:	subcc   %i4,0,%i4
	be      Bret           

	or      %g0,%i2,%o0
	or      %g0,%i3,%o1
	call    _exatan
	or      %g0,%i4,%o2
	or      %g0,%o0,%i2
	or      %g0,%o1,%i3
	or      %g0,%o2,%i4

	srl     %i3,11,%i1
	sll     %i2,21,%l0
	or      %i1,%l0,%i1
	srl     %i2,11,%i0
	sethi   %hi(0xfffff),%l0
	or      %l0,0x3ff,%l0
	and     %i0,%l0,%i0
	sll     %i4,20,%l0
	or      %i0,%l0,%i0
	or      %i0,%i5,%i0

Bret:
B999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

Bnan:	sethi   %hi(0xfff80000),%i0
	ba      Bret     
	or      %g0,0,%i1

	.global	_dpatan2
_dpatan2:
	.global	_atan2
_atan2:
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
	bcc     CspecA         

Clab1:	sub     %l3,1,%l7
	subcc   %l7,0x7fe,%g0
	bcc     CspecB         

Clab2:	or      %g0,%i4,%o0
	or      %g0,%i5,%o1
	or      %g0,%l0,%o2
	or      %g0,%l1,%o3
	or      %g0,%l2,%o4
	call    _exdiv
	or      %g0,%l3,%o5

	call    _exatan
	NOP
	or      %g0,%o0,%i4
	or      %g0,%o1,%i5
	or      %g0,%o2,%l0

	subcc   %i2,0,%i2
	bpos    i6             

	sethi   %hi(0xc90fdaa2),%o0
	or      %o0,0x2a2,%o0
	sethi   %hi(0x2168c235),%o1
	or      %o1,0x235,%o1
	or      %g0,0x400,%o2
	or      %g0,%i4,%o3
	or      %g0,%i5,%o4
	call    _exsub
	or      %g0,%l0,%o5
	or      %g0,%o0,%i4
	or      %g0,%o1,%i5
	or      %g0,%o2,%l0

i6:	subcc   %l0,0,%l0
	bg      i7       
	NOP
	or      %g0,1,%l5
	sub     %l5,%l0,%l5

L6:	subcc	%l5,32,%g0
	bcs	L7             
	subcc	%g0,%l5,%l0
	sub	%l5,32,%l5
	orcc	%i4,0,%i5
	bne	L6             
	or	%g0,0,%i4
L7:	be	L8             
	sll	%i4,%l0,%l0
	srl	%i5,%l5,%i5
	srl	%i4,%l5,%i4
	or	%i5,%l0,%i5

L8:	or      %g0,0,%l0
i7:	srl     %i5,11,%i1
	sll     %i4,21,%l7
	or      %i1,%l7,%i1
	srl     %i4,11,%i0
	sethi   %hi(0xfffff),%l7
	or      %l7,0x3ff,%l7
	and     %i0,%l7,%i0
	sll     %l0,20,%l7
	or      %i0,%l7,%i0
	or      %i0,%l4,%i0

Cret:
C999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

CspecA:	subcc   %l0,0,%l0
	be      i8             
	sll     %i4,1,%l7
	orcc    %l7,%i5,%l7
	bne     Cnan     
	NOP
	ba      Clab1    
	sethi   %hi(0x1000),%l0
i8:	addcc   %i5,%i5,%i5
	addx    %i4,%i4,%i4

/*	scan	%i4,0,%l5	*/
	SCAN | (in4 << 14) | 0 | (lo5 << 25)
	subcc	%l5,63,%g0
	bne	L9             
	NOP
	or	%i5,0,%i4
	or	%g0,0,%i5
	sub	%l0,32,%l0
/*	scan	%i4,0,%l5	*/
	SCAN | (in4 << 14) | 0 | (lo5 << 25)
	subcc	%l5,63,%g0
	be,a	L9             
	or	%l5,0,%l0
L9:	subcc	%g0,%l5,%l6
	be	Clab1           
	sub	%l0,%l5,%l0
	sll	%i4,%l5,%i4
	srl	%i5,%l6,%l6
	or	%l6,%i4,%i4
	ba      Clab1    
	sll	%i5,%l5,%i5

CspecB:	subcc   %l3,0,%l3
	be      i9             
	sll     %l1,1,%l7
	orcc    %l7,%l2,%l7
	bne     Cnan     
	NOP
	sethi   %hi(0x1000),%l3
	subcc   %l0,%l3,%g0
	be      Cnan     
	NOP
	ba,a    Clab2    
i9:	addcc   %l2,%l2,%l2
	addx    %l1,%l1,%l1

/*	scan	%l1,0,%l5	*/
	SCAN | (lo1 << 14) | 0 | (lo5 << 25)
	subcc	%l5,63,%g0
	bne	L11             
	NOP
	or	%l2,0,%l1
	or	%g0,0,%l2
	sub	%l3,32,%l3
/*	scan	%l1,0,%l5	*/
	SCAN | (lo1 << 14) | 0 | (lo5 << 25)
	subcc	%l5,63,%g0
	be,a	L11             
	or	%l5,0,%l3
L11:	subcc	%g0,%l5,%l6
	be	Clab2           
	sub	%l3,%l5,%l3
	sll	%l1,%l5,%l1
	srl	%l2,%l6,%l6
	or	%l6,%l1,%l1
	ba      Clab2    
	sll	%l2,%l5,%l2

Cnan:	sethi   %hi(0xfff80000),%i0
	ba      Cret     
	or      %g0,0,%i1

	.global	_dpasin
_dpasin:
	.global	_asin
_asin:
	save	%sp,-96,%sp

	sll     %i0,1,%i4
	srl     %i4,21,%i4
	sll     %i0,11,%i2
	srl     %i1,21,%l3
	or      %i2,%l3,%i2
	sll     %i1,11,%i3
	sethi   %hi(0x80000000),%l2
	or      %i2,%l2,%i2
	and     %l2,%i0,%l2

	subcc   %i4,0x3ff,%g0
	bcs     i10            
	sll     %i2,1,%l3
	orcc    %l3,%i3,%l3
	bne     Dnan           
	subcc   %i4,0x3ff,%g0
	bgu     Dnan           
i10:	subcc   %i4,0,%i4
	be      Dret           

	or      %g0,%i2,%o0
	or      %g0,%i3,%o1
	or      %g0,%i4,%o2
	or      %g0,%i2,%o3
	or      %g0,%i3,%o4
	call    _exmul
	or      %g0,%i4,%o5
	or      %g0,%o0,%i5
	or      %g0,%o1,%l0
	or      %g0,%o2,%l1

	sethi   %hi(0x80000000),%o0
	or      %g0,0,%o1
	or      %g0,0x3ff,%o2
	or      %g0,%i5,%o3
	or      %g0,%l0,%o4
	call    _exsub
	or      %g0,%l1,%o5

	call    _exsqrt
	NOP
	or      %g0,%o0,%i5
	or      %g0,%o1,%l0
	or      %g0,%o2,%l1

	or      %g0,%i2,%o0
	or      %g0,%i3,%o1
	or      %g0,%i4,%o2
	or      %g0,%i5,%o3
	or      %g0,%l0,%o4
	call    _exdiv
	or      %g0,%l1,%o5

	call    _exatan
	NOP
	or      %g0,%o0,%i2
	or      %g0,%o1,%i3
	or      %g0,%o2,%i4

	srl     %i3,11,%i1
	sll     %i2,21,%l3
	or      %i1,%l3,%i1
	srl     %i2,11,%i0
	sethi   %hi(0xfffff),%l3
	or      %l3,0x3ff,%l3
	and     %i0,%l3,%i0
	sll     %i4,20,%l3
	or      %i0,%l3,%i0
	or      %i0,%l2,%i0

Dret:
D999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

Dnan:	sethi   %hi(0xfff80000),%i0
	ba      Dret     
	or      %g0,0,%i1

	.global	_dpacos
_dpacos:
	.global	_acos
_acos:
	save	%sp,-96,%sp

	sll     %i0,1,%i4
	srl     %i4,21,%i4
	sll     %i0,11,%i2
	srl     %i1,21,%l2
	or      %i2,%l2,%i2
	sll     %i1,11,%i3
	sethi   %hi(0x80000000),%l2
	or      %i2,%l2,%i2

	subcc   %i4,0x3ff,%g0
	bcs     i11            
	sll     %i2,1,%l2
	orcc    %l2,%i3,%l2
	bne     Enan           
	subcc   %i4,0x3ff,%g0
	bgu     Enan           

i11:	or      %g0,%i2,%o0
	or      %g0,%i3,%o1
	or      %g0,%i4,%o2
	or      %g0,%i2,%o3
	or      %g0,%i3,%o4
	call    _exmul
	or      %g0,%i4,%o5
	or      %g0,%o0,%i5
	or      %g0,%o1,%l0
	or      %g0,%o2,%l1

	sethi   %hi(0x80000000),%o0
	or      %g0,0,%o1
	or      %g0,0x3ff,%o2
	or      %g0,%i5,%o3
	or      %g0,%l0,%o4
	call    _exsub
	or      %g0,%l1,%o5

	call    _exsqrt
	NOP
	or      %g0,%o0,%i5
	or      %g0,%o1,%l0
	or      %g0,%o2,%l1

	or      %g0,%i2,%o3
	or      %g0,%i3,%o4
	call    _exdiv
	or      %g0,%i4,%o5

	call    _exatan
	NOP
	or      %g0,%o0,%i2
	or      %g0,%o1,%i3
	or      %g0,%o2,%i4

	subcc   %i0,0,%i0
	bpos    i12            

	sethi   %hi(0xc90fdaa2),%o0
	or      %o0,0x2a2,%o0
	sethi   %hi(0x2168c235),%o1
	or      %o1,0x235,%o1
	or      %g0,0x400,%o2
	or      %g0,%i2,%o3
	or      %g0,%i3,%o4
	call    _exsub
	or      %g0,%i4,%o5
	or      %g0,%o0,%i2
	or      %g0,%o1,%i3
	or      %g0,%o2,%i4

i12:	srl     %i3,11,%i1
	sll     %i2,21,%l2
	or      %i1,%l2,%i1
	srl     %i2,11,%i0
	sethi   %hi(0xfffff),%l2
	or      %l2,0x3ff,%l2
	and     %i0,%l2,%i0
	sll     %i4,20,%l2
	or      %i0,%l2,%i0

Eret:
E999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

Enan:	sethi   %hi(0xfff80000),%i0
	ba      Eret     
	or      %g0,0,%i1

!	.end
