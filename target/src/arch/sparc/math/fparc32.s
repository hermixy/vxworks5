/* fparc32.s - VxWorks conversion from Microtek tools to Sun native */

/* Copyright 1984-1992 Wind River Systems, Inc. */

/*
modification history
--------------------
01b,10nov92,jwt  fixed register use error in fpexatan().
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

	.global	_fpexsqrt

	.data
	.align	4

atntaf:	.word	0x0,0x0
	.word	0xffeaaddd,0x79
	.word	0xffaaddb9,0x7a
	.word	0xbf70c130,0x7b
	.word	0xfeadd4d5,0x7b
	.word	0x9eb77746,0x7c
	.word	0xbdcbda5e,0x7c
	.word	0xdc86ba95,0x7c
	.word	0xfadbafc9,0x7c
	.word	0x8c5fad18,0x7d
	.word	0x9b13b9b8,0x7d
	.word	0xa9856ccb,0x7d
	.word	0xb7b0ca0f,0x7d
	.word	0xc59269ca,0x7d
	.word	0xd327761e,0x7d
	.word	0xe06da64a,0x7d
	.word	0xed63382b,0x7d
	.word	0xfa06e85b,0x7d
	.word	0x832bf4a7,0x7e
	.word	0x892aece0,0x7e
	.word	0x8f005d5f,0x7e
	.word	0x94ac72c9,0x7e
	.word	0x9a2f80e6,0x7e
	.word	0x9f89fdc5,0x7e
	.word	0xa4bc7d19,0x7e
	.word	0xa9c7abdc,0x7e
	.word	0xaeac4c39,0x7e
	.word	0xb36b31c9,0x7e
	.word	0xb8053e2c,0x7e
	.word	0xbc7b5deb,0x7e
	.word	0xc0ce85b9,0x7e
	.word	0xc4ffaffb,0x7e

	.text
	.align	4

	.global	_fpexatan
_fpexatan:
	save	%sp,-96,%sp

	subcc   %i1,0x7f,%g0
	bl      i1             
	subcc   %i1,0,%i1
	bneg    Azer     
	NOP
	sub     %g0,%i1,%i1
	add     %i1,0xfd,%i1
	sethi   %hi(0x80000000),%l3
	subcc   %i0,%l3,%g0
	bne     i2             
	subcc   %i1,0x7e,%g0
	bne     i3       
	NOP
	ba      e2       
	or      %g0,-1,%i0
i3:	ba      e2       
	add     %i1,1,%i1
i2:	or      %g0,%i0,%i3

	wr	%l3,0,%y
	or	%g0,0x0,%i0
	orcc	%g0,0,%g0
#if 0
	divscc	%i0,%i3,%i0
	divscc	%i0,%i3,%i0
	divscc	%i0,%i3,%i0
	divscc	%i0,%i3,%i0
	divscc	%i0,%i3,%i0
	divscc	%i0,%i3,%i0
	divscc	%i0,%i3,%i0
	divscc	%i0,%i3,%i0
	divscc	%i0,%i3,%i0
	divscc	%i0,%i3,%i0
	divscc	%i0,%i3,%i0
	divscc	%i0,%i3,%i0
	divscc	%i0,%i3,%i0
	divscc	%i0,%i3,%i0
	divscc	%i0,%i3,%i0
	divscc	%i0,%i3,%i0
	divscc	%i0,%i3,%i0
	divscc	%i0,%i3,%i0
	divscc	%i0,%i3,%i0
	divscc	%i0,%i3,%i0
	divscc	%i0,%i3,%i0
	divscc	%i0,%i3,%i0
	divscc	%i0,%i3,%i0
	divscc	%i0,%i3,%i0
	divscc	%i0,%i3,%i0
	divscc	%i0,%i3,%i0
	divscc	%i0,%i3,%i0
	divscc	%i0,%i3,%i0
	divscc	%i0,%i3,%i0
	divscc	%i0,%i3,%i0
	divscc	%i0,%i3,%i0
	divscc	%i0,%i3,%i0
#else
	DIVSCC | (in0 << 14) | in3 | (in0 << 25)
	DIVSCC | (in0 << 14) | in3 | (in0 << 25)
	DIVSCC | (in0 << 14) | in3 | (in0 << 25)
	DIVSCC | (in0 << 14) | in3 | (in0 << 25)
	DIVSCC | (in0 << 14) | in3 | (in0 << 25)
	DIVSCC | (in0 << 14) | in3 | (in0 << 25)
	DIVSCC | (in0 << 14) | in3 | (in0 << 25)
	DIVSCC | (in0 << 14) | in3 | (in0 << 25)
	DIVSCC | (in0 << 14) | in3 | (in0 << 25)
	DIVSCC | (in0 << 14) | in3 | (in0 << 25)
	DIVSCC | (in0 << 14) | in3 | (in0 << 25)
	DIVSCC | (in0 << 14) | in3 | (in0 << 25)
	DIVSCC | (in0 << 14) | in3 | (in0 << 25)
	DIVSCC | (in0 << 14) | in3 | (in0 << 25)
	DIVSCC | (in0 << 14) | in3 | (in0 << 25)
	DIVSCC | (in0 << 14) | in3 | (in0 << 25)
	DIVSCC | (in0 << 14) | in3 | (in0 << 25)
	DIVSCC | (in0 << 14) | in3 | (in0 << 25)
	DIVSCC | (in0 << 14) | in3 | (in0 << 25)
	DIVSCC | (in0 << 14) | in3 | (in0 << 25)
	DIVSCC | (in0 << 14) | in3 | (in0 << 25)
	DIVSCC | (in0 << 14) | in3 | (in0 << 25)
	DIVSCC | (in0 << 14) | in3 | (in0 << 25)
	DIVSCC | (in0 << 14) | in3 | (in0 << 25)
	DIVSCC | (in0 << 14) | in3 | (in0 << 25)
	DIVSCC | (in0 << 14) | in3 | (in0 << 25)
	DIVSCC | (in0 << 14) | in3 | (in0 << 25)
	DIVSCC | (in0 << 14) | in3 | (in0 << 25)
	DIVSCC | (in0 << 14) | in3 | (in0 << 25)
	DIVSCC | (in0 << 14) | in3 | (in0 << 25)
	DIVSCC | (in0 << 14) | in3 | (in0 << 25)
	DIVSCC | (in0 << 14) | in3 | (in0 << 25)
#endif

e2:	or      %i2,1,%i2
i1:	or      %g0,0x81,%i3
	sub     %i3,%i1,%i3
	subcc   %i3,8,%g0
	bge,a	i4               
	or      %g0,8,%i3
i4:	srl     %i0,24,%l1
	srl     %l1,%i3,%l1
	sll     %l1,1,%l1
	sll     %l1,26,%l3
	umul	%l3,%i0,%l3
	rd	%y,%l3
	or      %g0,0x7f,%i5
	sub     %i5,%i1,%i5
	subcc	%i5,32,%g0
	bcc,a	L1             
	or	%g0,0,%l3
L1:	srl	%l3,%i5,%l3
	sethi   %hi(0x80000000),%l6
	or      %l3,%l6,%l3
	or      %g0,8,%l7
	sub     %l7,%i3,%l7
	or      %g0,-1,%l6
	srl     %l6,%l7,%l6
	andcc   %i0,%l6,%i0
	be      Alab5    
	NOP
/*	scan	%i0,0,%i3	*/
	SCAN | (in0 << 14) | 0 | (in3 << 25)
	sll	%i0,%i3,%i0
	subcc	%i1,%i3,%i1
	srl     %i0,1,%i0

	wr	%i0,0,%y
	or	%g0,0x0,%i0
	orcc	%g0,0,%g0
#if 0
	divscc	%i0,%l3,%i0
	divscc	%i0,%l3,%i0
	divscc	%i0,%l3,%i0
	divscc	%i0,%l3,%i0
	divscc	%i0,%l3,%i0
	divscc	%i0,%l3,%i0
	divscc	%i0,%l3,%i0
	divscc	%i0,%l3,%i0
	divscc	%i0,%l3,%i0
	divscc	%i0,%l3,%i0
	divscc	%i0,%l3,%i0
	divscc	%i0,%l3,%i0
	divscc	%i0,%l3,%i0
	divscc	%i0,%l3,%i0
	divscc	%i0,%l3,%i0
	divscc	%i0,%l3,%i0
	divscc	%i0,%l3,%i0
	divscc	%i0,%l3,%i0
	divscc	%i0,%l3,%i0
	divscc	%i0,%l3,%i0
	divscc	%i0,%l3,%i0
	divscc	%i0,%l3,%i0
	divscc	%i0,%l3,%i0
	divscc	%i0,%l3,%i0
	divscc	%i0,%l3,%i0
	divscc	%i0,%l3,%i0
	divscc	%i0,%l3,%i0
	divscc	%i0,%l3,%i0
	divscc	%i0,%l3,%i0
	divscc	%i0,%l3,%i0
	divscc	%i0,%l3,%i0
	divscc	%i0,%l3,%i0
#else
	DIVSCC | (in0 << 14) | lo3 | (in0 << 25)
	DIVSCC | (in0 << 14) | lo3 | (in0 << 25)
	DIVSCC | (in0 << 14) | lo3 | (in0 << 25)
	DIVSCC | (in0 << 14) | lo3 | (in0 << 25)
	DIVSCC | (in0 << 14) | lo3 | (in0 << 25)
	DIVSCC | (in0 << 14) | lo3 | (in0 << 25)
	DIVSCC | (in0 << 14) | lo3 | (in0 << 25)
	DIVSCC | (in0 << 14) | lo3 | (in0 << 25)
	DIVSCC | (in0 << 14) | lo3 | (in0 << 25)
	DIVSCC | (in0 << 14) | lo3 | (in0 << 25)
	DIVSCC | (in0 << 14) | lo3 | (in0 << 25)
	DIVSCC | (in0 << 14) | lo3 | (in0 << 25)
	DIVSCC | (in0 << 14) | lo3 | (in0 << 25)
	DIVSCC | (in0 << 14) | lo3 | (in0 << 25)
	DIVSCC | (in0 << 14) | lo3 | (in0 << 25)
	DIVSCC | (in0 << 14) | lo3 | (in0 << 25)
	DIVSCC | (in0 << 14) | lo3 | (in0 << 25)
	DIVSCC | (in0 << 14) | lo3 | (in0 << 25)
	DIVSCC | (in0 << 14) | lo3 | (in0 << 25)
	DIVSCC | (in0 << 14) | lo3 | (in0 << 25)
	DIVSCC | (in0 << 14) | lo3 | (in0 << 25)
	DIVSCC | (in0 << 14) | lo3 | (in0 << 25)
	DIVSCC | (in0 << 14) | lo3 | (in0 << 25)
	DIVSCC | (in0 << 14) | lo3 | (in0 << 25)
	DIVSCC | (in0 << 14) | lo3 | (in0 << 25)
	DIVSCC | (in0 << 14) | lo3 | (in0 << 25)
	DIVSCC | (in0 << 14) | lo3 | (in0 << 25)
	DIVSCC | (in0 << 14) | lo3 | (in0 << 25)
	DIVSCC | (in0 << 14) | lo3 | (in0 << 25)
	DIVSCC | (in0 << 14) | lo3 | (in0 << 25)
	DIVSCC | (in0 << 14) | lo3 | (in0 << 25)
	DIVSCC | (in0 << 14) | lo3 | (in0 << 25)
#endif

	subcc   %i0,0,%i0
	ble     i5       
	NOP
	sll     %i0,1,%i0
	sub     %i1,1,%i1

i5:
	umul	%i0,%i0,%l3
	rd	%y,%l3
	or      %g0,0xfc,%i3
	sub     %i3,%i1,%i3
	sub     %i3,%i1,%i3
	subcc	%i3,32,%g0
	bcc,a	L2             
	or	%g0,0,%l3
L2:	srl	%l3,%i3,%l3
	sethi   %hi(0x1999999a),%i5
	or      %i5,0x19a,%i5
	umul	%l3,%i5,%i5
	rd	%y,%i5
	sethi   %hi(0x2aaaaaab),%l6
	or      %l6,0x2ab,%l6
	sub     %l6,%i5,%i5
	umul	%l3,%i5,%i5
	rd	%y,%i5
	sethi   %hi(0x80000000),%l6
	subcc   %l6,%i5,%i5
	bneg    i6       
	NOP
	ba      e6       
	sll     %i5,1,%i5
i6:	add     %i1,1,%i1

e6:
	umul	%i0,%i5,%i0
	rd	%y,%i0
	subcc   %i0,0,%i0
	bneg    i7       
	NOP
	sll     %i0,1,%i0
	sub     %i1,1,%i1
i7:	subcc   %l1,0,%l1
	be      Alab7    
	NOP

Alab5:	sethi   %hi(atntaf),%l6
	or	%l6,%lo(atntaf),%l6
	add     %l1,1,%l7
	sll     %l7,2,%l7
	ld      [%l6+%l7],%i3
	sub     %i3,%i1,%i4
	subcc	%i4,32,%g0
	bcc,a	L3             
	or	%g0,0,%i0
L3:	srl	%i0,%i4,%i0
	or      %g0,%i3,%i1
	sethi   %hi(atntaf),%l6
	or	%l6,%lo(atntaf),%l6
	sll     %l1,2,%l7
	ld      [%l6+%l7],%i3
	addcc   %i0,%i3,%i0
	bcc     Alab7    
	NOP
	srl     %i0,1,%i0
	sethi   %hi(0x80000000),%l6
	or      %i0,%l6,%i0
	add     %i1,1,%i1

Alab7:	andcc   %i2,1,%l6
	be      Alab8    
	NOP
	and     %i2,0xfffffffe,%i2
	or      %g0,0x7f,%i3
	sub     %i3,%i1,%i3
	subcc	%i3,32,%g0
	bcc,a	L4             
	or	%g0,0,%i0
L4:	srl	%i0,%i3,%i0
	or      %g0,0x7f,%i1
	sethi   %hi(0xc90fdaa2),%l6
	or      %l6,0x2a2,%l6
	subcc   %l6,%i0,%i0
	bneg    Alab8    
	NOP
	sll     %i0,1,%i0
	sub     %i1,1,%i1

Alab8:
A999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

Azer:	or      %g0,0,%i1
	ba      Alab8    
	or      %g0,%i1,%i0

	.global	_fpatan
_fpatan:
	.global	_atanf
_atanf:
	save	%sp,-96,%sp

	srl     %i0,23,%i2
	and     %i2,0xff,%i2
	sethi   %hi(0x80000000),%i3
	sll     %i0,8,%i1
	or      %i1,%i3,%i1
	and     %i3,%i0,%i3
	subcc   %i2,0xff,%g0
	bne     i11            
	sll     %i1,1,%l1
	subcc   %l1,0,%l1
	bne     Bnan           

i11:	or      %g0,%i1,%o0
	or      %g0,%i2,%o1
	call    _fpexatan
	or      %g0,%i3,%o2
	or      %g0,%o0,%i4
	or      %g0,%o1,%i5
	or      %g0,%o2,%l0

	sll     %i4,1,%i1
	srl     %i1,9,%i1
	sll     %i5,23,%l1
	or      %i1,%l1,%i1
	or      %i1,%i3,%i0

B999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

Bnan:	ba      B999     
	sethi   %hi(0xffc00000),%i0

	.global	_fpatan2
_fpatan2:
	.global	_atan2f
_atan2f:
	save	%sp,-96,%sp

	srl     %i0,23,%i3
	and     %i3,0xff,%i3
	sethi   %hi(0x80000000),%l2
	sll     %i0,8,%i2
	or      %i2,%l2,%i2
	and     %i0,%l2,%i4
	srl     %i1,23,%l0
	and     %l0,0xff,%l0
	sll     %i1,8,%i5
	or      %i5,%l2,%i5

	sub     %i3,1,%l6
	subcc   %l6,0xfe,%g0
	bcc     CspecA         

Clab1:	sub     %l0,1,%l6
	subcc   %l6,0xfe,%g0
	bcc     CspecB   
	NOP

Clab2:	add     %i3,0x7f,%i3
	sub     %i3,%l0,%i3
	srl     %i2,1,%i2

	wr	%i2,0,%y
	or	%g0,0x0,%i2
	orcc	%g0,0,%g0
#if 0
	divscc	%i2,%i5,%i2
	divscc	%i2,%i5,%i2
	divscc	%i2,%i5,%i2
	divscc	%i2,%i5,%i2
	divscc	%i2,%i5,%i2
	divscc	%i2,%i5,%i2
	divscc	%i2,%i5,%i2
	divscc	%i2,%i5,%i2
	divscc	%i2,%i5,%i2
	divscc	%i2,%i5,%i2
	divscc	%i2,%i5,%i2
	divscc	%i2,%i5,%i2
	divscc	%i2,%i5,%i2
	divscc	%i2,%i5,%i2
	divscc	%i2,%i5,%i2
	divscc	%i2,%i5,%i2
	divscc	%i2,%i5,%i2
	divscc	%i2,%i5,%i2
	divscc	%i2,%i5,%i2
	divscc	%i2,%i5,%i2
	divscc	%i2,%i5,%i2
	divscc	%i2,%i5,%i2
	divscc	%i2,%i5,%i2
	divscc	%i2,%i5,%i2
	divscc	%i2,%i5,%i2
	divscc	%i2,%i5,%i2
	divscc	%i2,%i5,%i2
	divscc	%i2,%i5,%i2
	divscc	%i2,%i5,%i2
	divscc	%i2,%i5,%i2
	divscc	%i2,%i5,%i2
	divscc	%i2,%i5,%i2
#else
	DIVSCC | (in2 << 14) | in5 | (in2 << 25)
	DIVSCC | (in2 << 14) | in5 | (in2 << 25)
	DIVSCC | (in2 << 14) | in5 | (in2 << 25)
	DIVSCC | (in2 << 14) | in5 | (in2 << 25)
	DIVSCC | (in2 << 14) | in5 | (in2 << 25)
	DIVSCC | (in2 << 14) | in5 | (in2 << 25)
	DIVSCC | (in2 << 14) | in5 | (in2 << 25)
	DIVSCC | (in2 << 14) | in5 | (in2 << 25)
	DIVSCC | (in2 << 14) | in5 | (in2 << 25)
	DIVSCC | (in2 << 14) | in5 | (in2 << 25)
	DIVSCC | (in2 << 14) | in5 | (in2 << 25)
	DIVSCC | (in2 << 14) | in5 | (in2 << 25)
	DIVSCC | (in2 << 14) | in5 | (in2 << 25)
	DIVSCC | (in2 << 14) | in5 | (in2 << 25)
	DIVSCC | (in2 << 14) | in5 | (in2 << 25)
	DIVSCC | (in2 << 14) | in5 | (in2 << 25)
	DIVSCC | (in2 << 14) | in5 | (in2 << 25)
	DIVSCC | (in2 << 14) | in5 | (in2 << 25)
	DIVSCC | (in2 << 14) | in5 | (in2 << 25)
	DIVSCC | (in2 << 14) | in5 | (in2 << 25)
	DIVSCC | (in2 << 14) | in5 | (in2 << 25)
	DIVSCC | (in2 << 14) | in5 | (in2 << 25)
	DIVSCC | (in2 << 14) | in5 | (in2 << 25)
	DIVSCC | (in2 << 14) | in5 | (in2 << 25)
	DIVSCC | (in2 << 14) | in5 | (in2 << 25)
	DIVSCC | (in2 << 14) | in5 | (in2 << 25)
	DIVSCC | (in2 << 14) | in5 | (in2 << 25)
	DIVSCC | (in2 << 14) | in5 | (in2 << 25)
	DIVSCC | (in2 << 14) | in5 | (in2 << 25)
	DIVSCC | (in2 << 14) | in5 | (in2 << 25)
	DIVSCC | (in2 << 14) | in5 | (in2 << 25)
	DIVSCC | (in2 << 14) | in5 | (in2 << 25)
#endif

	subcc   %i2,0,%i2
	ble     i12      
	NOP
	sll     %i2,1,%i2
	sub     %i3,1,%i3

i12:	or      %g0,%i2,%o0
	or      %g0,%i3,%o1
	call    _fpexatan
	or      %g0,%i4,%o2
	or      %g0,%o0,%i5
	or      %g0,%o1,%l0
	or      %g0,%o2,%l1

	subcc   %i1,0,%i1
	bpos    i13      
	NOP
	or      %g0,0x80,%l2
	sub     %l2,%l0,%l2
	or      %g0,0x80,%l0
	subcc	%l2,32,%g0
	bcc,a	L5             
	or	%g0,0,%i5
L5:	srl	%i5,%l2,%i5
	sethi   %hi(0xc90fdaa2),%l6
	or      %l6,0x2a2,%l6
	sub     %l6,%i5,%i5
/*	scan	%i5,0,%l2	*/
	SCAN | (in5 << 14) | 0 | (lo2 << 25)
	sll	%i5,%l2,%i5
	subcc	%l0,%l2,%l0
i13:	subcc   %l0,0,%l0
	bg      i14      
	NOP
	or      %g0,1,%l2
	sub     %l2,%l0,%l2
	subcc	%l2,32,%g0
	bcc,a	L6             
	or	%g0,0,%i5
L6:	srl	%i5,%l2,%i5
	or      %g0,0,%l0
i14:	sll     %i5,1,%i2
	srl     %i2,9,%i2
	sll     %l0,23,%l6
	or      %i2,%l6,%i2

Cret:	or      %i2,%i4,%i0

C999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

CspecA:	subcc   %i3,0,%i3
	be      i15            
	addcc   %i2,%i2,%l6
	bne     Cnan     
	NOP
	ba      Clab1    
	or      %g0,0x100,%i3
i15:	addcc   %i2,%i2,%i2
	be      Clab1    
	NOP
/*	scan	%i2,0,%l2	*/
	SCAN | (in2 << 14) | 0 | (lo2 << 25)
	sll	%i2,%l2,%i2
	subcc	%i3,%l2,%i3
	ba,a    Clab1    

CspecB:	subcc   %l0,0,%l0
	be      i17            
	addcc   %i5,%i5,%l6
	bne     Cnan     
	NOP
	or      %g0,0x100,%l0
	subcc   %i3,%l0,%g0
	be      Cnan     
	NOP
	ba,a    Clab2    
i17:	addcc   %i5,%i5,%i5
	be      Clab2    
	NOP
/*	scan	%i5,0,%l2	*/
	SCAN | (in5 << 14) | 0 | (lo2 << 25)
	sll	%i5,%l2,%i5
	subcc	%l0,%l2,%l0
	ba,a    Clab2    

Cnan:	ba      C999     
	sethi   %hi(0xffc00000),%i0

	.global	_fpasin
_fpasin:
	.global	_asinf
_asinf:
	save	%sp,-96,%sp

	srl     %i0,23,%l0
	and     %l0,0xff,%l0
	sethi   %hi(0x80000000),%l1
	sll     %i0,8,%i5
	or      %i5,%l1,%i5
	and     %l1,%i0,%l1
	umul	%i5,%i5,%l2
	rd	%y,%l2
	add     %l0,%l0,%l3
	sub     %l3,0x7e,%l3
	subcc   %l2,0,%l2
	bneg    i19      
	NOP
	sll     %l2,1,%l2
	sub     %l3,1,%l3
i19:	or      %g0,0x7f,%i1
	subcc   %i1,%l3,%i1
	bneg    Dnan     
	NOP
	or      %g0,0x7f,%l3
	subcc	%i1,32,%g0
	bcc,a	L7             
	or	%g0,0,%l2
L7:	srl	%l2,%i1,%l2
	sethi   %hi(0x80000000),%i1
	subcc   %l2,%i1,%g0
	bgu     Dnan     
	NOP
	subcc   %i1,%l2,%l2
	bne     i20      
	NOP
	ba      Dlab4    
	or      %g0,0xff,%l0
i20:
/*	scan	%l2,0,%i1	*/
	SCAN | (lo2 << 14) | 0 | (in1 << 25)
	sll	%l2,%i1,%l2
	subcc	%l3,%i1,%l3

	or      %g0,%l2,%o0
	call    _fpexsqrt
	or      %g0,%l3,%o1
	orcc    %g0,%o0,%l2
	or      %g0,%o1,%l3

	bne     i21      
	or      %g0,%o2,%l4
	ba,a    D999     
i21:	add     %l0,0x7f,%l0
	sub     %l0,%l3,%l0
	srl     %i5,1,%i5

	wr	%i5,0,%y
	or	%g0,0x0,%i5
	orcc	%g0,0,%g0
#if 0
	divscc	%i5,%l2,%i5
	divscc	%i5,%l2,%i5
	divscc	%i5,%l2,%i5
	divscc	%i5,%l2,%i5
	divscc	%i5,%l2,%i5
	divscc	%i5,%l2,%i5
	divscc	%i5,%l2,%i5
	divscc	%i5,%l2,%i5
	divscc	%i5,%l2,%i5
	divscc	%i5,%l2,%i5
	divscc	%i5,%l2,%i5
	divscc	%i5,%l2,%i5
	divscc	%i5,%l2,%i5
	divscc	%i5,%l2,%i5
	divscc	%i5,%l2,%i5
	divscc	%i5,%l2,%i5
	divscc	%i5,%l2,%i5
	divscc	%i5,%l2,%i5
	divscc	%i5,%l2,%i5
	divscc	%i5,%l2,%i5
	divscc	%i5,%l2,%i5
	divscc	%i5,%l2,%i5
	divscc	%i5,%l2,%i5
	divscc	%i5,%l2,%i5
	divscc	%i5,%l2,%i5
	divscc	%i5,%l2,%i5
	divscc	%i5,%l2,%i5
	divscc	%i5,%l2,%i5
	divscc	%i5,%l2,%i5
	divscc	%i5,%l2,%i5
	divscc	%i5,%l2,%i5
	divscc	%i5,%l2,%i5
#else
	DIVSCC | (in5 << 14) | lo2 | (in5 << 25)
	DIVSCC | (in5 << 14) | lo2 | (in5 << 25)
	DIVSCC | (in5 << 14) | lo2 | (in5 << 25)
	DIVSCC | (in5 << 14) | lo2 | (in5 << 25)
	DIVSCC | (in5 << 14) | lo2 | (in5 << 25)
	DIVSCC | (in5 << 14) | lo2 | (in5 << 25)
	DIVSCC | (in5 << 14) | lo2 | (in5 << 25)
	DIVSCC | (in5 << 14) | lo2 | (in5 << 25)
	DIVSCC | (in5 << 14) | lo2 | (in5 << 25)
	DIVSCC | (in5 << 14) | lo2 | (in5 << 25)
	DIVSCC | (in5 << 14) | lo2 | (in5 << 25)
	DIVSCC | (in5 << 14) | lo2 | (in5 << 25)
	DIVSCC | (in5 << 14) | lo2 | (in5 << 25)
	DIVSCC | (in5 << 14) | lo2 | (in5 << 25)
	DIVSCC | (in5 << 14) | lo2 | (in5 << 25)
	DIVSCC | (in5 << 14) | lo2 | (in5 << 25)
	DIVSCC | (in5 << 14) | lo2 | (in5 << 25)
	DIVSCC | (in5 << 14) | lo2 | (in5 << 25)
	DIVSCC | (in5 << 14) | lo2 | (in5 << 25)
	DIVSCC | (in5 << 14) | lo2 | (in5 << 25)
	DIVSCC | (in5 << 14) | lo2 | (in5 << 25)
	DIVSCC | (in5 << 14) | lo2 | (in5 << 25)
	DIVSCC | (in5 << 14) | lo2 | (in5 << 25)
	DIVSCC | (in5 << 14) | lo2 | (in5 << 25)
	DIVSCC | (in5 << 14) | lo2 | (in5 << 25)
	DIVSCC | (in5 << 14) | lo2 | (in5 << 25)
	DIVSCC | (in5 << 14) | lo2 | (in5 << 25)
	DIVSCC | (in5 << 14) | lo2 | (in5 << 25)
	DIVSCC | (in5 << 14) | lo2 | (in5 << 25)
	DIVSCC | (in5 << 14) | lo2 | (in5 << 25)
	DIVSCC | (in5 << 14) | lo2 | (in5 << 25)
	DIVSCC | (in5 << 14) | lo2 | (in5 << 25)
#endif

	subcc   %i5,0,%i5
	ble     Dlab4    
	NOP
	sll     %i5,1,%i5
	sub     %l0,1,%l0

Dlab4:	or      %g0,%i5,%o0
	or      %g0,%l0,%o1
	call    _fpexatan
	or      %g0,%l1,%o2
	or      %g0,%o0,%l2
	or      %g0,%o1,%l3
	or      %g0,%o2,%l4

	sll     %l2,1,%i5
	srl     %i5,9,%i5
	sll     %l3,23,%l5
	or      %i5,%l5,%i5
	or      %i5,%l1,%i0

D999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

Dnan:	ba      D999     
	sethi   %hi(0xffc00000),%i0

	.global	_fpacos
_fpacos:
	.global	_acosf
_acosf:
	save	%sp,-96,%sp

	srl     %i0,23,%l0
	and     %l0,0xff,%l0
	sethi   %hi(0x80000000),%l1
	sll     %i0,8,%i5
	or      %i5,%l1,%i5
	and     %l1,%i0,%l1
	umul	%i5,%i5,%l2
	rd	%y,%l2
	add     %l0,%l0,%l3
	sub     %l3,0x7e,%l3
	subcc   %l2,0,%l2
	bneg    i23      
	NOP
	sll     %l2,1,%l2
	sub     %l3,1,%l3
i23:	or      %g0,0x7f,%i1
	subcc   %i1,%l3,%i1
	bneg    Enan     
	NOP
	or      %g0,0x7f,%l3
	subcc	%i1,32,%g0
	bcc,a	L8             
	or	%g0,0,%l2
L8:	srl	%l2,%i1,%l2
	sethi   %hi(0x80000000),%i1
	subcc   %l2,%i1,%g0
	bgu     Enan     
	NOP
	sub     %i1,%l2,%l2
/*	scan	%l2,0,%i1	*/
	SCAN | (lo2 << 14) | 0 | (in1 << 25)
	sll	%l2,%i1,%l2
	subcc	%l3,%i1,%l3

	or      %g0,%l2,%o0
	call    _fpexsqrt
	or      %g0,%l3,%o1
	or      %g0,%o0,%l2
	or      %g0,%o1,%l3
	or      %g0,%o2,%l4

	sub     %l3,%l0,%l0
	add     %l0,0x7f,%l0
	srl     %l2,1,%l2
	or      %g0,%i5,%i2

	wr	%l2,0,%y
	or	%g0,0x0,%i5
	orcc	%g0,0,%g0
#if 0
	divscc	%i5,%i2,%i5
	divscc	%i5,%i2,%i5
	divscc	%i5,%i2,%i5
	divscc	%i5,%i2,%i5
	divscc	%i5,%i2,%i5
	divscc	%i5,%i2,%i5
	divscc	%i5,%i2,%i5
	divscc	%i5,%i2,%i5
	divscc	%i5,%i2,%i5
	divscc	%i5,%i2,%i5
	divscc	%i5,%i2,%i5
	divscc	%i5,%i2,%i5
	divscc	%i5,%i2,%i5
	divscc	%i5,%i2,%i5
	divscc	%i5,%i2,%i5
	divscc	%i5,%i2,%i5
	divscc	%i5,%i2,%i5
	divscc	%i5,%i2,%i5
	divscc	%i5,%i2,%i5
	divscc	%i5,%i2,%i5
	divscc	%i5,%i2,%i5
	divscc	%i5,%i2,%i5
	divscc	%i5,%i2,%i5
	divscc	%i5,%i2,%i5
	divscc	%i5,%i2,%i5
	divscc	%i5,%i2,%i5
	divscc	%i5,%i2,%i5
	divscc	%i5,%i2,%i5
	divscc	%i5,%i2,%i5
	divscc	%i5,%i2,%i5
	divscc	%i5,%i2,%i5
	divscc	%i5,%i2,%i5
#else
	DIVSCC | (in5 << 14) | in2 | (in5 << 25)
	DIVSCC | (in5 << 14) | in2 | (in5 << 25)
	DIVSCC | (in5 << 14) | in2 | (in5 << 25)
	DIVSCC | (in5 << 14) | in2 | (in5 << 25)
	DIVSCC | (in5 << 14) | in2 | (in5 << 25)
	DIVSCC | (in5 << 14) | in2 | (in5 << 25)
	DIVSCC | (in5 << 14) | in2 | (in5 << 25)
	DIVSCC | (in5 << 14) | in2 | (in5 << 25)
	DIVSCC | (in5 << 14) | in2 | (in5 << 25)
	DIVSCC | (in5 << 14) | in2 | (in5 << 25)
	DIVSCC | (in5 << 14) | in2 | (in5 << 25)
	DIVSCC | (in5 << 14) | in2 | (in5 << 25)
	DIVSCC | (in5 << 14) | in2 | (in5 << 25)
	DIVSCC | (in5 << 14) | in2 | (in5 << 25)
	DIVSCC | (in5 << 14) | in2 | (in5 << 25)
	DIVSCC | (in5 << 14) | in2 | (in5 << 25)
	DIVSCC | (in5 << 14) | in2 | (in5 << 25)
	DIVSCC | (in5 << 14) | in2 | (in5 << 25)
	DIVSCC | (in5 << 14) | in2 | (in5 << 25)
	DIVSCC | (in5 << 14) | in2 | (in5 << 25)
	DIVSCC | (in5 << 14) | in2 | (in5 << 25)
	DIVSCC | (in5 << 14) | in2 | (in5 << 25)
	DIVSCC | (in5 << 14) | in2 | (in5 << 25)
	DIVSCC | (in5 << 14) | in2 | (in5 << 25)
	DIVSCC | (in5 << 14) | in2 | (in5 << 25)
	DIVSCC | (in5 << 14) | in2 | (in5 << 25)
	DIVSCC | (in5 << 14) | in2 | (in5 << 25)
	DIVSCC | (in5 << 14) | in2 | (in5 << 25)
	DIVSCC | (in5 << 14) | in2 | (in5 << 25)
	DIVSCC | (in5 << 14) | in2 | (in5 << 25)
	DIVSCC | (in5 << 14) | in2 | (in5 << 25)
	DIVSCC | (in5 << 14) | in2 | (in5 << 25)
#endif

	subcc   %i5,0,%i5
	ble     i24      
	NOP
	sll     %i5,1,%i5
	sub     %l0,1,%l0

i24:	or      %g0,%i5,%o0
	or      %g0,%l0,%o1
	call    _fpexatan
	or      %g0,%l1,%o2
	or      %g0,%o0,%l2
	or      %g0,%o1,%l3
	or      %g0,%o2,%l4

	subcc   %l1,0,%l1
	be      i25      
	NOP
	or      %g0,0x80,%i1
	sub     %i1,%l3,%i1
	or      %g0,0x80,%l3
	subcc	%i1,32,%g0
	bcc,a	L9             
	or	%g0,0,%l2
L9:	srl	%l2,%i1,%l2
	sethi   %hi(0xc90fdaa2),%l5
	or      %l5,0x2a2,%l5
	subcc   %l5,%l2,%l2
	be,a	i26               
	or      %g0,0,%l3
i26:
/*	scan	%l2,0,%i1	*/
	SCAN | (lo2 << 14) | 0 | (in1 << 25)
	sll	%l2,%i1,%l2
	subcc	%l3,%i1,%l3
i25:	sll     %l2,1,%i5
	srl     %i5,9,%i5
	sll     %l3,23,%l5
	or      %i5,%l5,%i5
	or      %g0,%i5,%i0

E999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

Enan:	ba      E999     
	sethi   %hi(0xffc00000),%i0

!	.end
