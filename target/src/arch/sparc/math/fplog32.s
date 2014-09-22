/* fplog32.s - VxWorks conversion from Microtek tools to Sun native */

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

	.data
	.align	4

logtaf:	.word	0xb17217f8,0x7e
	.word	0x934b108a,0x7e
	.word	0xf0a450d1,0x7d
	.word	0xbfd7d1df,0x7d
	.word	0x934b108a,0x7d
	.word	0xd49f69e4,0x7c
	.word	0x88bc7411,0x7c
	.word	0x842cc5ad,0x7b
	.word	0x0,0x0
	.word	0xf1383b71,0x7b
	.word	0xe47fbe3d,0x7c
	.word	0xa30c5e11,0x7d
	.word	0xcf991f66,0x7d
	.word	0xf8947afd,0x7d
	.word	0x8f42faf4,0x7e
	.word	0xa0ec7f42,0x7e

	.text
	.align	4

	.global	_fpexlog
_fpexlog:
	save	%sp,-96,%sp

	srl     %i0,27,%i4
	and     %i4,0x1e,%i4
	sethi   %hi(0xf0000000),%l6
	and     %l6,%i0,%i5
	subcc   %i1,0x7f,%g0
	bl      i1       
	NOP
	or      %g0,0,%i2
	sub     %i1,0x7f,%i3
	add     %i5,%i0,%l3
	sethi   %hi(0xfffffff),%l6
	or      %l6,0x3ff,%l6
	ba      e1       
	and     %i0,%l6,%i0
i1:	sethi   %hi(0x80000000),%i2
	or      %g0,0x7e,%i3
	sub     %i3,%i1,%i3
	sub     %i4,0xe,%i4
	sethi   %hi(0x10000000),%l6
	add     %i5,%l6,%i5
	add     %i5,%i0,%l3
	sub     %i5,%i0,%i0
e1:	srl     %l3,1,%l3
	sethi   %hi(0x80000000),%l6
	or      %l3,%l6,%l3
	or      %g0,0x7f,%i1
/*	scan	%i0,0,%i5	*/
	SCAN | (in0 << 14) | 0 | (in5 << 25)
	sll	%i0,%i5,%i0
	subcc	%i1,%i5,%i1
	subcc   %i0,%l3,%g0
	bcs     i2       
	NOP
	srl     %i0,1,%i0
	add     %i1,1,%i1
i2:	srl     %l3,1,%i5

	wr	%i0,0,%y
	or	%g0,%i5,%i0
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
	umul	%i0,%i0,%l3
	rd	%y,%l3

	or      %g0,0x80,%i5
	sub     %i5,%i1,%i5
	sll     %i5,1,%i5
	subcc	%i5,32,%g0
	bcc,a	L1             
	or	%g0,0,%l3
L1:	srl	%l3,%i5,%l3
	sethi   %hi(0x12492492),%l1
	or      %l1,0x92,%l1
	umul	%l3,%l1,%l1
	rd	%y,%l1
	sethi   %hi(0x1999999a),%l6
	or      %l6,0x19a,%l6
	add     %l1,%l6,%l1
	umul	%l3,%l1,%l1
	rd	%y,%l1
	sethi   %hi(0x2aaaaaab),%l6
	or      %l6,0x2ab,%l6
	add     %l1,%l6,%l1
	umul	%l3,%l1,%l1
	rd	%y,%l1
	sethi   %hi(0x80000000),%l6
	add     %l1,%l6,%l1
	umul	%i0,%l1,%i5
	rd	%y,%i0
	subcc   %i0,0,%i0
	ble     i3       
	NOP
	addcc   %i5,%i5,%i5
	addx    %i0,%i0,%i0
	sub     %i1,1,%i1

i3:	sethi   %hi(logtaf),%l6
	or	%l6,%lo(logtaf),%l6
	add     %i4,1,%l7
	sll     %l7,2,%l7
	ld      [%l6+%l7],%i5
	subcc   %i5,0,%i5
	be      i4       
	NOP
	sub     %i5,%i1,%l0
	subcc	%l0,32,%g0
	bcc,a	L2             
	or	%g0,0,%i0
L2:	srl	%i0,%l0,%i0
	or      %g0,%i5,%i1
	sethi   %hi(logtaf),%l6
	or	%l6,%lo(logtaf),%l6
	sll     %i4,2,%l7
	ld      [%l6+%l7],%i5
	addcc   %i0,%i5,%i0
	bcc     i4       
	NOP
	srl     %i0,1,%i0
	sethi   %hi(0x80000000),%l6
	or      %i0,%l6,%i0
	add     %i1,1,%i1
i4:	subcc   %i3,0,%i3
	be      i6       
	NOP
	or      %g0,0x9e,%l4
/*	scan	%i3,0,%i5	*/
	SCAN | (in3 << 14) | 0 | (in5 << 25)
	sll	%i3,%i5,%i3
	subcc	%l4,%i5,%l4
	sethi   %hi(0xb17217f8),%i5
	or      %i5,0x3f8,%i5
	umul	%i3,%i5,%i5
	rd	%y,%l3
	subcc   %l3,0,%l3
	ble     i7       
	NOP
	addcc   %i5,%i5,%i5
	addx    %l3,%l3,%l3
	sub     %l4,1,%l4
i7:	sub     %l4,%i1,%i5
	subcc	%i5,32,%g0
	bcc,a	L3             
	or	%g0,0,%i0
L3:	srl	%i0,%i5,%i0
	addcc   %i0,%l3,%i0
	bcc     i8       
	NOP
	srl     %i0,1,%i0
	sethi   %hi(0x80000000),%l6
	or      %i0,%l6,%i0
	add     %l4,1,%l4
i8:	or      %g0,%l4,%i1
i6:	subcc   %i0,0,%i0
	be,a	i9                
	or      %g0,0,%i1

i9:
A999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

	.global	_fplog
_fplog:
	.global	_logf
_logf:
	save	%sp,-96,%sp

	srl     %i0,23,%i2
	sll     %i0,8,%i1
	sethi   %hi(0x80000000),%i5
	or      %i1,%i5,%i1
	sub     %i2,1,%i5
	subcc   %i5,0xfe,%g0
	bcc     Bspec          

Blog5:	or      %g0,%i1,%o0
	or      %g0,%i2,%o1
	call    _fpexlog
	or      %g0,%i3,%o2
	or      %g0,%o0,%i1
	or      %g0,%o1,%i2
	or      %g0,%o2,%i3

	sll     %i1,1,%i1
	srl     %i1,9,%i1
	sll     %i2,23,%i5
	or      %i1,%i5,%i1
	or      %i1,%i3,%i0

B999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

Bspec:	subcc   %i0,0,%i0
	bneg    Bnan           
	subcc   %i2,0,%i2
	bne     i10      
	NOP
	addcc   %i1,%i1,%i1
	bne     i11      
	NOP
	ba      B999     
	sethi   %hi(0xff800000),%i0
i11:
/*	scan	%i1,0,%i4	*/
	SCAN | (in1 << 14) | 0 | (in4 << 25)
	sll	%i1,%i4,%i1
	subcc	%i2,%i4,%i2
	ba,a    Blog5    
i10:	addcc   %i1,%i1,%i5
	bne     Bnan     
	NOP
	ba      B999     
	sethi   %hi(0x7f800000),%i0

Bnan:	ba      B999     
	sethi   %hi(0xffc00000),%i0

	.global	_fplog10
_fplog10:
	.global	_log10f
_log10f:
	save	%sp,-96,%sp

	srl     %i0,23,%i2
	sll     %i0,8,%i1
	sethi   %hi(0x80000000),%i5
	or      %i1,%i5,%i1
	sub     %i2,1,%i5
	subcc   %i5,0xfe,%g0
	bcc     Cspec          

Clog5:	or      %g0,%i1,%o0
	or      %g0,%i2,%o1
	call    _fpexlog
	or      %g0,%i3,%o2
	or      %g0,%o0,%i1
	or      %g0,%o1,%i2
	or      %g0,%o2,%i3

	sub     %i2,1,%i2
	sethi   %hi(0xde5bd8a9),%i4
	or      %i4,0xa9,%i4
	umul	%i1,%i4,%i1
	rd	%y,%i1
	subcc   %i1,0,%i1
	ble     i13      
	NOP
	sll     %i1,1,%i1
	sub     %i2,1,%i2
i13:	sll     %i1,1,%i1
	srl     %i1,9,%i1
	sll     %i2,23,%i5
	or      %i1,%i5,%i1
	or      %i1,%i3,%i0

C999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

Cspec:	subcc   %i0,0,%i0
	bneg    Cnan           
	subcc   %i2,0,%i2
	bne     i14      
	NOP
	addcc   %i1,%i1,%i1
	bne     i15      
	NOP
	ba      C999     
	sethi   %hi(0xff800000),%i0
i15:
/*	scan	%i1,0,%i4	*/
	SCAN | (in1 << 14) | 0 | (in4 << 25)
	sll	%i1,%i4,%i1
	subcc	%i2,%i4,%i2
	ba,a    Clog5    
i14:	addcc   %i1,%i1,%i5
	bne     Cnan     
	NOP
	ba      C999     
	sethi   %hi(0x7f800000),%i0

Cnan:	ba      C999     
	sethi   %hi(0xffc00000),%i0

!	.end
