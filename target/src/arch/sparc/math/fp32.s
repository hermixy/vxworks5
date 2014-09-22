/* fp32.s - VxWorks conversion from Microtek tools to Sun native */

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

	.global	__finc
! ! ! ! !
!  Routine:	__finc	sp increment
!  On entry:	o0 = argument A
!  On exit:	o0 = A + 1
!  Notes:	Uses _fpadd after setting second argument
! ! ! ! !
__finc:
	ba	_fpadd			! j/ add
	 sethi	%hi(0x3f800000),%o1	! o1 = 1

	.global	__fdec
! ! ! ! !
!  Routine:	__fdec	sp decrement
!  On entry:	o0 = argument A
!  On exit:	o0 = A - 1
!  Notes:	Uses _fpadd after setting second argument
! ! ! ! !
__fdec:
	ba	_fpadd			! j/ add
	 sethi	%hi(0xbf800000),%o1	! o1 = -1

	.global	_fpsub
_fpsub:
	.global	__fsub
__fsub:

	sethi   %hi(0x80000000),%g1
	xor     %o1,%g1,%o1

	.global	_fpadd
_fpadd:
	.global	__fadd
__fadd:
	save	%sp,-96,%sp

	sra     %i0,23,%i5
	and     %i5,0xff,%i5
	sra     %i1,23,%l1
	and     %l1,0xff,%l1
	sethi   %hi(0x80000000),%i3
	sll     %i0,8,%i4
	or      %i4,%i3,%i4
	sll     %i1,8,%l0
	or      %l0,%i3,%l0

	sub     %i5,1,%l3
	subcc   %l3,0xfe,%g0
	bcc     BspecA         

Blab1:	sub     %l1,1,%l3
	subcc   %l3,0xfe,%g0
	bcc     BspecB   

Blab2:	subcc   %i5,%l1,%l2
	bneg    i1             
	subcc   %l2,0x20,%g0
	bl      i2       
	NOP
	ba      e1       
	or      %g0,2,%l0
i2:	or      %g0,%l0,%i2
	srl     %l0,%l2,%l0
	sll     %l0,%l2,%l3
	subcc   %l3,%i2,%g0
	bne,a	e1               
	or      %l0,2,%l0
	ba,a    e1       
i1:	or      %g0,%l1,%i5
	sub     %g0,%l2,%l2
	subcc   %l2,0x20,%g0
	bl      i4       
	NOP
	ba      e1       
	or      %g0,2,%i4
i4:	or      %g0,%i4,%i2
	srl     %i4,%l2,%i4
	sll     %i4,%l2,%l3
	subcc   %l3,%i2,%g0
	bne,a	e1               
	or      %i4,2,%i4

e1:	and     %i0,%i3,%l2
	xorcc   %l2,%i1,%l3
	bneg    Bsub1    
	NOP
	addcc   %i4,%l0,%i4
	bcc     Bres           
	andcc   %i4,1,%l3
	bne,a	i7               
	or      %i4,2,%i4
i7:	srl     %i4,1,%i4
	add     %i5,1,%i5
	subcc   %i5,0xff,%g0
	be,a	Bres              
	or      %g0,0,%i4
	ba,a    Bres     

Bsub1:	subcc   %i4,%l0,%i4
	be      Bzer     
	NOP
	bcc     i9       
	NOP
	sub     %g0,%i4,%i4
	xor     %l2,%i3,%l2
i9:
/*	scan	%i4,0,%i2	*/
	SCAN | (in4 << 14) | 0 | (in2 << 25)
	sll	%i4,%i2,%i4
	subcc	%i5,%i2,%i5

Bres:	subcc   %i5,0,%i5
	ble     Bund     
	NOP

Blab12:	addcc   %i4,0x80,%i4
	addx    %i5,0,%i5
	srl     %i4,8,%l3
	and     %l3,1,%l3
	sub     %i4,%l3,%i4
	sll     %i4,1,%i4
	srl     %i4,9,%i4
	sll     %i5,23,%l3
	or      %i4,%l3,%i4
	or      %i4,%l2,%i0

B999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

Bund:	or      %g0,1,%i2
	sub     %i2,%i5,%i2
	subcc   %i2,0x20,%g0
	bcc     Bzer     
	sethi   %hi(0x80000000),%l3
	or      %i4,%l3,%i4
	or      %g0,0x20,%l3
	sub     %l3,%i2,%l3
	sll     %i4,%l3,%i5
	srl     %i4,%i2,%i4
	subcc   %i5,0,%i5
	bne,a	i10              
	or      %i4,1,%i4
i10:	ba      Blab12   
	or      %g0,0,%i5

BspecA:	subcc   %i5,0,%i5
	bne     i11      
	NOP
	addcc   %i4,%i4,%i4
	be      i12      
	NOP
/*	scan	%i4,0,%l2	*/
	SCAN | (in4 << 14) | 0 | (lo2 << 25)
	sll	%i4,%l2,%i4
	subcc	%i5,%l2,%i5
	ba,a    Blab1    
i12:	subcc   %l1,0xff,%g0
	be      BspecB   
	NOP
	ba,a    BretB    
i11:	addcc   %i4,%i4,%l3
	bne     Bnan           
	subcc   %l1,0xff,%g0
	bne     BretA          
	xorcc   %i0,%i1,%l3
	bneg    Bnan     
	NOP
	ba,a    BretB    

BspecB:	subcc   %l1,0,%l1
	bne     i14      
	NOP
	addcc   %l0,%l0,%l0
	be      BretA    
	NOP
/*	scan	%l0,0,%l2	*/
	SCAN | (lo0 << 14) | 0 | (lo2 << 25)
	sll	%l0,%l2,%l0
	subcc	%l1,%l2,%l1
	ba,a    Blab2    
i14:	addcc   %l0,%l0,%l3
	bne     Bnan     
	NOP

BretB:	ba      B999     
	or      %g0,%i1,%i0

BretA:
	ba,a    B999     

Bzer:	ba      B999     
	or      %g0,0,%i0

Bnan:	ba      B999     
	sethi   %hi(0xffc00000),%i0

	.global	_fpmul
_fpmul:
	.global	__fmul
__fmul:
	save	%sp,-96,%sp

	sra     %i0,23,%i3
	and     %i3,0xff,%i3
	sra     %i1,23,%i5
	and     %i5,0xff,%i5
	sethi   %hi(0x80000000),%l0
	sll     %i0,8,%i2
	or      %i2,%l0,%i2
	sll     %i1,8,%i4
	or      %i4,%l0,%i4
	xor     %i0,%i1,%l2
	and     %l0,%l2,%l0

	sub     %i3,1,%l2
	subcc   %l2,0xfe,%g0
	bcc     CspecA         

Clab1:	sub     %i5,1,%l2
	subcc   %l2,0xfe,%g0
	bcc     CspecB   

Clab2:
	umul	%i2,%i4,%l1
	rd	%y,%i2
	subcc   %l1,0,%l1
	bne,a	i16              
	or      %i2,1,%i2

i16:	subcc   %i2,0,%i2
	bneg    i17      
	NOP
	sll     %i2,1,%i2
	sub     %i3,1,%i3
i17:	sub     %i5,0x7e,%l2
	add     %i3,%l2,%i3
	sub     %i3,1,%l2
	subcc   %l2,0xfe,%g0
	bcc     Coveund  
	NOP

Clab8:	addcc   %i2,0x80,%i2
	addx    %i3,0,%i3
	srl     %i2,8,%l2
	and     %l2,1,%l2
	sub     %i2,%l2,%i2
	sll     %i2,1,%i2
	srl     %i2,9,%i2
	sll     %i3,23,%l2
	or      %i2,%l2,%i2
	or      %i2,%l0,%i0

C999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

CspecA:	subcc   %i3,0,%i3
	bne     i18      
	NOP
	addcc   %i2,%i2,%i2
	be      i18      
	NOP
/*	scan	%i2,0,%l1	*/
	SCAN | (in2 << 14) | 0 | (lo1 << 25)
	sll	%i2,%l1,%i2
	subcc	%i3,%l1,%i3
	ba,a    Clab1    
i18:	add     %i3,%i5,%l2
	subcc   %l2,0xff,%g0
	be      Cnan           
	subcc   %i3,0,%i3
	be      Czer           
	addcc   %i2,%i2,%l2
	bne     Cnan           
	subcc   %i5,0xff,%g0
	bne     Cinf           

CspecB:	subcc   %i5,0,%i5
	bne     i20      
	NOP
	addcc   %i4,%i4,%i4
	be      Czer     
	NOP
/*	scan	%i4,0,%l1	*/
	SCAN | (in4 << 14) | 0 | (lo1 << 25)
	sll	%i4,%l1,%i4
	subcc	%i5,%l1,%i5
	ba,a    Clab2    

Czer:	ba      C999     
	or      %g0,%l0,%i0
i20:	addcc   %i4,%i4,%l2
	bne     Cnan     
	NOP

Cinf:	sethi   %hi(0x7f800000),%l2
	ba      C999     
	or      %l2,%l0,%i0

Cnan:	ba      C999     
	sethi   %hi(0xffc00000),%i0

Coveund:
	subcc   %i3,0,%i3
	bg      Cinf     
	NOP
	or      %g0,1,%l1
	sub     %l1,%i3,%l1
	subcc   %l1,0x20,%g0
	bcc     Czer     
	NOP
	or      %g0,0x20,%l2
	sub     %l2,%l1,%l2
	sll     %i2,%l2,%i3
	srl     %i2,%l1,%i2
	subcc   %i3,0,%i3
	bne,a	i23              
	or      %i2,2,%i2
i23:	ba      Clab8    
	or      %g0,0,%i3

	.global	_fpdiv
_fpdiv:
	.global	__fdiv
__fdiv:
	save	%sp,-96,%sp

	sethi   %hi(0x80000000),%l1
	sra     %i0,23,%i4
	and     %i4,0xff,%i4
	sra     %i1,23,%l0
	and     %l0,0xff,%l0
	sll     %i0,8,%i3
	or      %i3,%l1,%i3
	sll     %i1,8,%i5
	or      %i5,%l1,%i5
	xor     %i0,%i1,%l6
	and     %l1,%l6,%l1

	sub     %i4,1,%l6
	subcc   %l6,0xfe,%g0
	bcc     DspecA         

Dlab1:	sub     %l0,1,%l6
	subcc   %l6,0xfe,%g0
	bcc     DspecB         

Dlab2:	subcc   %i3,%i5,%g0
	bcs     i24      
	NOP
	srl     %i3,1,%i3
	add     %i4,1,%i4

i24:	wr	%i3,0,%y
	or	%g0,0x0,%i3
	orcc	%g0,0,%g0
#if 0
	divscc	%i3,%i5,%i3
	divscc	%i3,%i5,%i3
	divscc	%i3,%i5,%i3
	divscc	%i3,%i5,%i3
	divscc	%i3,%i5,%i3
	divscc	%i3,%i5,%i3
	divscc	%i3,%i5,%i3
	divscc	%i3,%i5,%i3
	divscc	%i3,%i5,%i3
	divscc	%i3,%i5,%i3
	divscc	%i3,%i5,%i3
	divscc	%i3,%i5,%i3
	divscc	%i3,%i5,%i3
	divscc	%i3,%i5,%i3
	divscc	%i3,%i5,%i3
	divscc	%i3,%i5,%i3
	divscc	%i3,%i5,%i3
	divscc	%i3,%i5,%i3
	divscc	%i3,%i5,%i3
	divscc	%i3,%i5,%i3
	divscc	%i3,%i5,%i3
	divscc	%i3,%i5,%i3
	divscc	%i3,%i5,%i3
	divscc	%i3,%i5,%i3
	divscc	%i3,%i5,%i3
	divscc	%i3,%i5,%i3
	divscc	%i3,%i5,%i3
	divscc	%i3,%i5,%i3
	divscc	%i3,%i5,%i3
	divscc	%i3,%i5,%i3
	divscc	%i3,%i5,%i3
	divscc	%i3,%i5,%i3
#else
	DIVSCC | (in3 << 14) | in5 | (in3 << 25)
	DIVSCC | (in3 << 14) | in5 | (in3 << 25)
	DIVSCC | (in3 << 14) | in5 | (in3 << 25)
	DIVSCC | (in3 << 14) | in5 | (in3 << 25)
	DIVSCC | (in3 << 14) | in5 | (in3 << 25)
	DIVSCC | (in3 << 14) | in5 | (in3 << 25)
	DIVSCC | (in3 << 14) | in5 | (in3 << 25)
	DIVSCC | (in3 << 14) | in5 | (in3 << 25)
	DIVSCC | (in3 << 14) | in5 | (in3 << 25)
	DIVSCC | (in3 << 14) | in5 | (in3 << 25)
	DIVSCC | (in3 << 14) | in5 | (in3 << 25)
	DIVSCC | (in3 << 14) | in5 | (in3 << 25)
	DIVSCC | (in3 << 14) | in5 | (in3 << 25)
	DIVSCC | (in3 << 14) | in5 | (in3 << 25)
	DIVSCC | (in3 << 14) | in5 | (in3 << 25)
	DIVSCC | (in3 << 14) | in5 | (in3 << 25)
	DIVSCC | (in3 << 14) | in5 | (in3 << 25)
	DIVSCC | (in3 << 14) | in5 | (in3 << 25)
	DIVSCC | (in3 << 14) | in5 | (in3 << 25)
	DIVSCC | (in3 << 14) | in5 | (in3 << 25)
	DIVSCC | (in3 << 14) | in5 | (in3 << 25)
	DIVSCC | (in3 << 14) | in5 | (in3 << 25)
	DIVSCC | (in3 << 14) | in5 | (in3 << 25)
	DIVSCC | (in3 << 14) | in5 | (in3 << 25)
	DIVSCC | (in3 << 14) | in5 | (in3 << 25)
	DIVSCC | (in3 << 14) | in5 | (in3 << 25)
	DIVSCC | (in3 << 14) | in5 | (in3 << 25)
	DIVSCC | (in3 << 14) | in5 | (in3 << 25)
	DIVSCC | (in3 << 14) | in5 | (in3 << 25)
	DIVSCC | (in3 << 14) | in5 | (in3 << 25)
	DIVSCC | (in3 << 14) | in5 | (in3 << 25)
	DIVSCC | (in3 << 14) | in5 | (in3 << 25)
#endif
	bge	L1       
	 rd	%y,%i2
	addcc	%i5,%i2,%i2

L1:	subcc   %i2,0,%i2
	bne,a	i25              
	or      %i3,1,%i3
i25:	or      %g0,0x7e,%l6
	sub     %l6,%l0,%l6
	add     %i4,%l6,%i4
	sub     %i4,1,%l6
	subcc   %l6,0xfe,%g0
	bcc     Doveund  
	NOP

Dlab8:	addcc   %i3,0x80,%i3
	addx    %i4,0,%i4
	srl     %i3,8,%l6
	and     %l6,1,%l6
	sub     %i3,%l6,%i3
	sll     %i3,1,%i3
	srl     %i3,9,%i3
	sll     %i4,23,%l6
	or      %i3,%l6,%i3
	or      %i3,%l1,%i0

D999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

DspecA:	subcc   %i4,0,%i4
	bne     i26      
	NOP
	addcc   %i3,%i3,%i3
	be      i26      
	NOP
/*	scan	%i3,0,%l2	*/
	SCAN | (in3 << 14) | 0 | (lo2 << 25)
	sll	%i3,%l2,%i3
	subcc	%i4,%l2,%i4
	ba,a    Dlab1    
i26:	subcc   %l0,%i4,%g0
	be      Dnan           
	subcc   %l0,0xff,%g0
	be      DspecB         
	subcc   %i4,0,%i4
	be      Dzer           
	addcc   %i3,%i3,%l6
	bne     Dnan     
	NOP
	ba,a    Dinf     

DspecB:	subcc   %l0,0,%l0
	bne     i28      
	NOP
	addcc   %i5,%i5,%i5
	be      Dinf     
	NOP
/*	scan	%i5,0,%l2	*/
	SCAN | (in5 << 14) | 0 | (lo2 << 25)
	sll	%i5,%l2,%i5
	subcc	%l0,%l2,%l0
	ba,a    Dlab2    
i28:	addcc   %i5,%i5,%l6
	bne     Dnan     
	NOP

Dzer:	ba      D999     
	or      %g0,%l1,%i0

Doveund:
	subcc   %i4,0,%i4
	bg      Dinf     
	NOP
	or      %g0,1,%l2
	sub     %l2,%i4,%l2
	subcc   %l2,0x20,%g0
	bcc     Dzer     
	NOP
	or      %g0,0x20,%l6
	sub     %l6,%l2,%l6
	sll     %i3,%l6,%i4
	srl     %i3,%l2,%i3
	subcc   %i4,0,%i4
	bne,a	i31              
	or      %i3,2,%i3
i31:	ba      Dlab8    
	or      %g0,0,%i4

Dinf:	sethi   %hi(0x7f800000),%l6
	ba      D999     
	or      %l6,%l1,%i0

Dnan:	ba      D999     
	sethi   %hi(0xffc00000),%i0


	.global	__ltof
	.global	_sitofp
! ! ! ! !
!  _sitofp:	signed int to sp floating point
!  On entry:	o0 =	argument A
!  On exit:	o0 =	sp floating point
! ! ! ! !
__ltof:					! C ifc
_sitofp:				! USSW ifc
	sethi	%hi(0x80000000),%o3
	subcc	%o0,0x0,%g0
	be	j22      
	 andcc	%o3,%o0,%o2
	bl,a	j23
	 subcc	%g0,%o0,%o0

j23:	or	%g0,0x9e,%o1

b24:	sub	%o1,0x1,%o1
	subcc	%o0,0x0,%g0
	bg	b24      
	 sll	%o0,0x1,%o0

	add	%o1,0x1,%o1

	addcc	%o0,0x100,%o0
	addxcc	%o1,0x0,%o1
	andcc	%o0,0x1ff,%o3
	be,a	j24
	 andcc	%o0,0xfffffdff,%o0

j24:	srl	%o0,0x9,%o0
	sll	%o1,0x17,%o3
	orcc	%o0,%o3,%o0

j22:	orcc	%o0,%o2,%o0

E999:	jmpl	%o7+8,%g0
	 NOP


	.global	__ultof
	.global	_uitofp
! ! ! ! !
!  _uitofp:	unsigned int to sp floating point
!  On entry:	o0 =	argument A
!  On exit:	o0 =	sp floating point
! ! ! ! !
__ultof:				! C ifc
_uitofp:				! USSW ifc
	subcc	%o0,0x0,%g0
	be	F999     
	 or	%g0,0x9e,%o1

b26:	sub	%o1,0x1,%o1
	subcc	%o0,0x0,%g0
	bg	b26      
	 sll	%o0,0x1,%o0

	add	%o1,0x1,%o1

	addcc	%o0,0x100,%o0
	addxcc	%o1,0x0,%o1
	andcc	%o0,0x1ff,%o3
	be,a	j26
	 andcc	%o0,0xfffffdff,%o0

j26:	srl	%o0,0x9,%o0
	sll	%o1,0x17,%o2
	orcc	%o0,%o2,%o0

F999:	jmpl	%o7+8,%g0
	 NOP


	.global	__ftol
	.global	_fptosi
! ! ! ! !
!  _fptosi:	sp floating point to int
!  On entry:	o0 =	argument A
!  On exit:	o0 =	int
! ! ! ! !
__ftol:					! C ifc
_fptosi:				! USSW ifc
	srl	%o0,0x17,%o2
	andcc	%o2,0xff,%o2

	sll	%o0,0x8,%o1
	sethi	%hi(0x80000000),%o3
	orcc	%o1,%o3,%o1

	subcc	%o2,0x9e,%g0
	bge,a	G999
	 sethi	%hi(0x80000000),%o0

j27:	or	%g0,0x9e,%o3
	subcc	%o3,%o2,%o2

	subcc	%o2,0x20,%g0
	bge,a	e28
	 or	%g0,0,%o1

j28:	srl	%o1,%o2,%o1

e28:	subcc	%o0,0x0,%g0
	bl,a	j29
	 subcc	%g0,%o1,%o1

j29:	or	%g0,%o1,%o0
G999:	jmpl	%o7+8,%g0
	 NOP


	.global	_fptoui
! ! ! ! !
!  _fptoui:	sp floating point to unsigned int
!  On entry:	o0 =	argument A
!  On exit:	o0 =	unsigned int
! ! ! ! !
_fptoui:
	srl	%o0,0x17,%o2
	andcc	%o2,0xff,%o2

	sll	%o0,0x8,%o1
	sethi	%hi(0x80000000),%o3
	orcc	%o1,%o3,%o1

	subcc	%o2,0x9f,%g0
	bge,a	I999
	 sethi	%hi(0x80000000),%o0

i30:	or	%g0,0x9e,%o3
	subcc	%o3,%o2,%o2

	subcc	%o2,0x20,%g0
	bge,a	e31
	 or	%g0,0,%o1

	srl	%o1,%o2,%o1

e31:	subcc	%o0,0x0,%g0
	bl,a	i32
	 subcc	%g0,%o1,%o1

i32:	or	%g0,%o1,%o0
I999:	jmpl	%o7+8,%g0
	 NOP


	.global	__fcmp
	.global	_fpcmp
! ! ! ! !
!  _fpcmp:	sp floating point compare
!  On entry:	o0 =	argument A
!		o1 =	argument B
!  On exit:	condition codes set
!		o0 =	-1  A < B
!			 0  A == B
!			 1  A > B
! ! ! ! !
__fcmp:					! C ifc
_fpcmp:					! USSW ifc
	orcc	%o0,%o1,%o3
	addcc	%o3,%o3,%o3
	be	Jequ

	 sra	%o0,0x1f,%o2

	sethi	%hi(0x80000000),%o3
	andncc	%o0,%o3,%o0

	xorcc	%o0,%o2,%o0

	subcc	%o0,%o2,%o0

	sra	%o1,0x1f,%o2

	sethi	%hi(0x80000000),%o3
	andncc	%o1,%o3,%o1

	xorcc	%o1,%o2,%o1

	subcc	%o1,%o2,%o1

	subcc	%o0,%o1,%g0
	bl,a	J999
	 or	%g0,0xffffffff,%o0

i33:	subcc	%o0,%o1,%g0
	bg,a	J999
	 or	%g0,0x1,%o0

Jequ:	or	%g0,0,%o0

J999:	jmpl	%o7+8,%g0
	 orcc	%o0,%g0,%o0


	.global	_fabsf
	.global	_fpfabs
! ! ! ! !
!  _fpabs:	sp floating point absolute
!  On entry:	o0 =	argument A
!  On exit:	o0 =	absolute value
! ! ! ! !
_fabsf:					! C ifc
_fpfabs:				! USSW ifc
	sethi	%hi(0x80000000),%o1
K999:	jmpl	%o7+8,%g0
	 andncc	%o0,%o1,%o0

!	.end
