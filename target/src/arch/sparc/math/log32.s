/* log32.s - VxWorks conversion from Microtek tools to Sun native */

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

	.global	_exadd
	.global	_exmul
	.global	_exdiv
	.global	_sigmax

	.data
	.align	4

logcon:	.word	0x88888889,0x8888888
	.word	0xd89d89d9,0x9d89d89
	.word	0x2e8ba2e9,0xba2e8ba
	.word	0x38e38e39,0xe38e38e
	.word	0x49249249,0x12492492
	.word	0x9999999a,0x19999999
	.word	0xaaaaaaab,0x2aaaaaaa
	.word	0x0,0x80000000
bastab:	.word	0xb17217f7,0xd1cf79ac,0x3fe
	.word	0x934b1089,0xa6dc93c2,0x3fe
	.word	0xf0a450d1,0x39366ca7,0x3fd
	.word	0xbfd7d1de,0xc0a8df6f,0x3fd
	.word	0x934b1089,0xa6dc93c2,0x3fd
	.word	0xd49f69e4,0x56cf1b7a,0x3fc
	.word	0x88bc7411,0x3f23def2,0x3fc
	.word	0x842cc5ac,0xf1d03445,0x3fb
	.word	0x0,0x0,0x0
	.word	0xf1383b71,0x57972f50,0x3fb
	.word	0xe47fbe3c,0xd4d10d61,0x3fc
	.word	0xa30c5e10,0xe2f613e8,0x3fd
	.word	0xcf991f65,0xfcc25f96,0x3fd
	.word	0xf8947afd,0x7837659b,0x3fd
	.word	0x8f42faf3,0x820681f0,0x3fe
	.word	0xa0ec7f42,0x33957323,0x3fe

	.text
	.align	4

	.global	_exlog
_exlog:
	save	%sp,-96,%sp

	sethi   %hi(0xf0000000),%l7
	and     %l7,%i0,%i5
	or      %g0,%i1,%l2
	or      %g0,%l2,%l5
	srl     %i0,28,%i4
	subcc   %i2,0x3ff,%g0
	bl      i1       
	NOP
	or      %g0,0x3ff,%l3
	sub     %i2,%l3,%i3
	add     %i0,%i5,%l4
	srl     %l5,1,%l5
	sll     %l4,31,%l7
	srl     %l4,1,%l4
	or      %l5,%l7,%l5
	sethi   %hi(0x80000000),%l7
	or      %l4,%l7,%l4
	or      %g0,0x400,%l6
	ba      e1       
	sub     %i0,%i5,%l1
i1:	or      %g0,0x3fe,%l3
	sub     %l3,%i2,%i3
	sub     %i4,7,%i4
	sethi   %hi(0x10000000),%l7
	add     %i5,%l7,%i5
	add     %i0,%i5,%l4
	srl     %l5,1,%l5
	sll     %l4,31,%l7
	srl     %l4,1,%l4
	or      %l5,%l7,%l5
	sethi   %hi(0x80000000),%l7
	or      %l4,%l7,%l4
	or      %g0,0x3ff,%l6
	sub     %i0,%i5,%l1
	subcc   %g0,%l2,%l2
	subx    %g0,%l1,%l1

e1:
/*	scan	%l1,0,%i5	*/
	SCAN | (lo1 << 14) | 0 | (in5 << 25)
	subcc	%i5,63,%g0
	bne	L1             
	NOP
	or	%l2,0,%l1
	or	%g0,0,%l2
	sub	%l3,32,%l3
/*	scan	%l1,0,%i5	*/
	SCAN | (lo1 << 14) | 0 | (in5 << 25)
	subcc	%i5,63,%g0
	be,a	L1             
	or	%i5,0,%l3
L1:	subcc	%g0,%i5,%l0
	be	L2             
	sub	%l3,%i5,%l3
	sll	%l1,%i5,%l1
	srl	%l2,%l0,%l0
	or	%l0,%l1,%l1
	sll	%l2,%i5,%l2

L2:	or      %g0,%l1,%o0
	or      %g0,%l2,%o1
	or      %g0,%l3,%o2
	or      %g0,%l4,%o3
	or      %g0,%l5,%o4
	call    _exdiv
	or      %g0,%l6,%o5
	or      %g0,%o0,%l1
	or      %g0,%o1,%l2
	or      %g0,%o2,%l3

	or      %g0,%o0,%o3
	or      %g0,%o1,%o4
	call    _exmul
	or      %g0,%o2,%o5

	or      %g0,0,%o3
	sethi   %hi(logcon),%o4
	call    _sigmax
	or	%o4,%lo(logcon),%o4
	or      %g0,%o0,%l4
	or      %g0,%o1,%l5
	or      %g0,%o2,%l6

	add     %l6,1,%l6

	or      %g0,%l1,%o0
	or      %g0,%l2,%o1
	or      %g0,%l3,%o2
	or      %g0,%l4,%o3
	or      %g0,%l5,%o4
	call    _exmul
	or      %g0,%l6,%o5

	sethi   %hi(bastab),%g1
	or	%g1,%lo(bastab),%g1
	sll     %i4,2,%l7
	add     %g1,%l7,%g1
	add     %g1,%l7,%g1
	add     %g1,%l7,%g1
	ld      [%g1],%o3
	ld      [%g1+4],%o4
	call    _exadd
	ld      [%g1+8],%o5
	or      %g0,%o0,%l1
	or      %g0,%o1,%l2
	or      %g0,%o2,%l3

	subcc   %i3,0,%i3
	be      Aret     
	NOP
	or      %g0,0x41e,%l6
/*	scan	%i3,0,%i5	*/
	SCAN | (in3 << 14) | 0 | (in5 << 25)
	sll	%i3,%i5,%i3
	subcc	%l6,%i5,%l6
	sethi   %hi(0xb17217f7),%i5
	or      %i5,0x3f7,%i5
	umul	%i3,%i5,%l5
	rd	%y,%l4
	sethi   %hi(0xd1cf79ac),%i5
	or      %i5,0x1ac,%i5
	umul	%i3,%i5,%l0
	rd	%y,%i5
	addcc   %l5,%i5,%l5
	addxcc  %l4,0,%l4
	bneg    i2       
	NOP
	addcc   %l0,%l0,%l0
	addxcc  %l5,%l5,%l5
	addx    %l4,%l4,%l4
	sub     %l6,1,%l6
i2:	addcc   %l0,%l0,%l0
	addxcc  %l5,0,%l5
	addx    %l4,0,%l4

	or      %g0,%l1,%o0
	or      %g0,%l2,%o1
	or      %g0,%l3,%o2
	or      %g0,%l4,%o3
	or      %g0,%l5,%o4
	call    _exadd
	or      %g0,%l6,%o5
	or      %g0,%o0,%l1
	or      %g0,%o1,%l2
	or      %g0,%o2,%l3

Aret:	or      %g0,%l1,%i0
	or      %g0,%l2,%i1
	or      %g0,%l3,%i2

A999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

	.global	_dplog
_dplog:
	.global	_log
_log:
	save	%sp,-96,%sp

	srl     %i0,20,%i4
	sll     %i0,11,%i2
	srl     %i1,21,%l4
	or      %i2,%l4,%i2
	sll     %i1,11,%i3
	sethi   %hi(0x80000000),%l4
	or      %i2,%l4,%i2
	sub     %i4,1,%l4
	subcc   %l4,0x7fe,%g0
	bcc     Bspec          

Blog5:	or      %g0,%i2,%o0
	or      %g0,%i3,%o1
	call    _exlog
	or      %g0,%i4,%o2
	or      %g0,%o0,%i5
	or      %g0,%o1,%l0
	or      %g0,%o2,%l1

	srl     %l0,11,%i1
	sll     %i5,21,%l4
	or      %i1,%l4,%i1
	srl     %i5,11,%i0
	sethi   %hi(0xfffff),%l4
	or      %l4,0x3ff,%l4
	and     %i0,%l4,%i0
	sll     %l1,20,%l4
	or      %i0,%l4,%i0

Bret2:	subcc   %i4,0x3ff,%g0
	bge     Bret     
	NOP
	sethi   %hi(0x80000000),%l4
	or      %i0,%l4,%i0

Bret:
B999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

Bspec:	subcc   %i0,0,%i0
	bneg    Bnan           
	subcc   %i4,0,%i4
	bne     i4       
	NOP
	addcc   %i3,%i3,%i3
	addx    %i2,%i2,%i2
	orcc    %i2,%i3,%l4
	be      e4       
	NOP

/*	scan	%i2,0,%l2	*/
	SCAN | (in2 << 14) | 0 | (lo2 << 25)
	subcc	%l2,63,%g0
	bne	L3             
	NOP
	or	%i3,0,%i2
	or	%g0,0,%i3
	sub	%i4,32,%i4
/*	scan	%i2,0,%l2	*/
	SCAN | (in2 << 14) | 0 | (lo2 << 25)
	subcc	%l2,63,%g0
	be,a	L3             
	or	%l2,0,%i4
L3:	subcc	%g0,%l2,%l3
	be	Blog5          
	sub	%i4,%l2,%i4
	sll	%i2,%l2,%i2
	srl	%i3,%l3,%l3
	or	%l3,%i2,%i2
	sll	%i3,%l2,%i3

	ba,a    Blog5    
i4:	sll     %i2,1,%l4
	orcc    %l4,%i3,%l4
	bne     Bnan     
	NOP
e4:	sethi   %hi(0x7ff00000),%i0
	ba      Bret2    
	or      %g0,0,%i1

Bnan:	sethi   %hi(0xfff80000),%i0
	ba      Bret     
	or      %g0,0,%i1

	.global	_dplog10
_dplog10:
	.global	_log10
_log10:
	save	%sp,-96,%sp

	srl     %i0,20,%i4
	sll     %i0,11,%i2
	srl     %i1,21,%l4
	or      %i2,%l4,%i2
	sll     %i1,11,%i3
	sethi   %hi(0x80000000),%l4
	or      %i2,%l4,%i2
	sub     %i4,1,%l4
	subcc   %l4,0x7fe,%g0
	bcc     Cspec          

Clog5:	or      %g0,%i2,%o0
	or      %g0,%i3,%o1
	call    _exlog
	or      %g0,%i4,%o2

	sethi   %hi(0xde5bd8a9),%o3
	or      %o3,0xa9,%o3
	sethi   %hi(0x37287195),%o4
	or      %o4,0x195,%o4
	call    _exmul
	or      %g0,0x3fd,%o5
	or      %g0,%o0,%i5
	or      %g0,%o1,%l0
	or      %g0,%o2,%l1

	srl     %l0,11,%i1
	sll     %i5,21,%l4
	or      %i1,%l4,%i1
	srl     %i5,11,%i0
	sethi   %hi(0xfffff),%l4
	or      %l4,0x3ff,%l4
	and     %i0,%l4,%i0
	sll     %l1,20,%l4
	or      %i0,%l4,%i0

Cret2:	subcc   %i4,0x3ff,%g0
	bge     Cret     
	NOP
	sethi   %hi(0x80000000),%l4
	or      %i0,%l4,%i0

Cret:
C999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

Cspec:	subcc   %i0,0,%i0
	bneg    Cnan           
	subcc   %i4,0,%i4
	bne     i7       
	NOP
	addcc   %i3,%i3,%i3
	addx    %i2,%i2,%i2
	orcc    %i2,%i3,%l4
	be      e7       
	NOP

/*	scan	%i2,0,%l2	*/
	SCAN | (in2 << 14) | 0 | (lo2 << 25)
	subcc	%l2,63,%g0
	bne	L5             
	NOP
	or	%i3,0,%i2
	or	%g0,0,%i3
	sub	%i4,32,%i4
/*	scan	%i2,0,%l2	*/
	SCAN | (in2 << 14) | 0 | (lo2 << 25)
	subcc	%l2,63,%g0
	be,a	L5             
	or	%l2,0,%i4
L5:	subcc	%g0,%l2,%l3
	be	Clog5          
	sub	%i4,%l2,%i4
	sll	%i2,%l2,%i2
	srl	%i3,%l3,%l3
	or	%l3,%i2,%i2
	sll	%i3,%l2,%i3

	ba,a    Clog5    
i7:	sll     %i2,1,%l4
	orcc    %l4,%i3,%l4
	bne     Cnan     
	NOP
e7:	sethi   %hi(0x7ff00000),%i0
	ba      Cret2    
	or      %g0,0,%i1

Cnan:	sethi   %hi(0xfff80000),%i0
	ba      Cret     
	or      %g0,0,%i1

!	.end
