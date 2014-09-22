/* dp32.s - VxWorks conversion from Microtek tools to Sun native */

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

	.text
	.align	4

	.global	_dpsub
_dpsub:
	.global	__dsub
__dsub:

	sethi   %hi(0x80000000),%g1
	xor     %o2,%g1,%o2

	.global	_dpadd
_dpadd:
	.global	__dadd
__dadd:
	save	%sp,-96,%sp

	sll     %i0,1,%l4
	srl     %l4,21,%l4
	sll     %i0,11,%l2
	srl     %i1,21,%g1
	or      %l2,%g1,%l2
	sll     %i1,11,%l3
	sethi   %hi(0x80000000),%l1
	or      %l2,%l1,%l2
	sll     %i2,1,%l7
	srl     %l7,21,%l7
	sll     %i2,11,%l5
	srl     %i3,21,%g1
	or      %l5,%g1,%l5
	sll     %i3,11,%l6
	or      %l5,%l1,%l5
	and     %l1,%i0,%l1

	sub     %l4,1,%g1
	subcc   %g1,0x7fe,%g0
	bcc     BspecA         

Blab1:	sub     %l7,1,%g1
	subcc   %g1,0x7fe,%g0
	bcc     BspecB   
	NOP

Blab2:	subcc   %l4,%l7,%i4
	be      Badd3          
	subcc   %i4,0,%i4
	ble     i1       
	NOP

L1:	subcc	%i4,32,%g0
	bcs	L2             
	orcc	%l6,0,%g0
	sub	%i4,32,%i4
	or	%l5,0,%l6
	or	%g0,0,%l5
	be	L3             
	orcc	%l6,0,%g0
	or	%l6,2,%l6
L3:	bne	L1             
	NOP
	ba,a	Badd3          
L2:	subcc	%g0,%i4,%i5
	be	Badd3          
	sll	%l6,%i5,%l0
	orcc	%l0,0,%g0
	sll	%l5,%i5,%i5
	srl	%l6,%i4,%l6
	srl	%l5,%i4,%l5
	or	%l6,%i5,%l6
	bne,a	Badd3          
	or	%l6,2,%l6
	ba,a    Badd3    

i1:	or      %g0,%l7,%l4
	sub     %g0,%i4,%i4

L5:	subcc	%i4,32,%g0
	bcs	L6             
	orcc	%l3,0,%g0
	sub	%i4,32,%i4
	or	%l2,0,%l3
	or	%g0,0,%l2
	be	L7             
	orcc	%l3,0,%g0
	or	%l3,2,%l3
L7:	bne	L5             
	NOP
	ba,a	Badd3          
L6:	subcc	%g0,%i4,%i5
	be	Badd3          
	sll	%l3,%i5,%l0
	orcc	%l0,0,%g0
	sll	%l2,%i5,%i5
	srl	%l3,%i4,%l3
	srl	%l2,%i4,%l2
	or	%l3,%i5,%l3
	bne,a	Badd3          
	or	%l3,2,%l3

Badd3:	xorcc   %i0,%i2,%g1
	bneg    Bsub1    
	NOP
	addcc   %l3,%l6,%l3
	addxcc  %l2,%l5,%l2
	bcc     Bres           
	andcc   %l3,1,%g1
	bne,a	i3               
	or      %l3,2,%l3
i3:	srl     %l3,1,%l3
	sll     %l2,31,%g1
	srl     %l2,1,%l2
	or      %l3,%g1,%l3
	sethi   %hi(0x80000000),%g1
	or      %l2,%g1,%l2
	add     %l4,1,%l4
	subcc   %l4,0x7ff,%g0
	be      Binf     
	NOP
	ba,a    Bres     

Bsub1:	subcc   %l3,%l6,%l3
	subxcc  %l2,%l5,%l2
	bcc     i4       
	NOP
	sethi   %hi(0x80000000),%g1
	xor     %l1,%g1,%l1
	subcc   %g0,%l3,%l3
	subx    %g0,%l2,%l2

i4:
/*	scan	%l2,0,%i4	*/
	SCAN | (lo2 << 14) | 0 | (in4 << 25)
	subcc	%i4,63,%g0
	bne	L9             
	NOP
	or	%l3,0,%l2
	or	%g0,0,%l3
	sub	%l4,32,%l4
/*	scan	%l2,0,%i4	*/
	SCAN | (lo2 << 14) | 0 | (in4 << 25)
	subcc	%i4,63,%g0
	be,a	L9             
	or	%i4,0,%l4
L9:	subcc	%g0,%i4,%i5
	be	Bres            
	sub	%l4,%i4,%l4
	sll	%l2,%i4,%l2
	srl	%l3,%i5,%i5
	or	%i5,%l2,%l2
	sll	%l3,%i4,%l3

Bres:	subcc   %l4,0,%l4
	ble     Bund     
	NOP

Blab8:	addcc   %l3,0x400,%l3
	addxcc  %l2,0,%l2
	addx    %l4,0,%l4
	srl     %l3,11,%g1
	and     %g1,1,%g1
	sub     %l3,%g1,%l3
	srl     %l3,11,%i1
	sll     %l2,21,%g1
	or      %i1,%g1,%i1
	srl     %l2,11,%i0
	sethi   %hi(0xfffff),%g1
	or      %g1,0x3ff,%g1
	and     %i0,%g1,%i0
	sll     %l4,20,%g1
	or      %i0,%g1,%i0

Bret:	or      %i0,%l1,%i0

B999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

BspecA:	subcc   %l4,0,%l4
	bne     i5       
	NOP
	addcc   %l3,%l3,%l3
	addx    %l2,%l2,%l2
	orcc    %l2,%l3,%g1
	be      i6       
	NOP

/*	scan	%l2,0,%i4	*/
	SCAN | (lo2 << 14) | 0 | (in4 << 25)
	subcc	%i4,63,%g0
	bne	L11             
	NOP
	or	%l3,0,%l2
	or	%g0,0,%l3
	sub	%l4,32,%l4
/*	scan	%l2,0,%i4	*/
	SCAN | (lo2 << 14) | 0 | (in4 << 25)
	subcc	%i4,63,%g0
	be,a	L11             
	or	%i4,0,%l4
L11:	subcc	%g0,%i4,%i5
	be	Blab1           
	sub	%l4,%i4,%l4
	sll	%l2,%i4,%l2
	srl	%l3,%i5,%i5
	or	%i5,%l2,%l2
	sll	%l3,%i4,%l3

	ba,a    Blab1    
i6:	subcc   %l7,0x7ff,%g0
	be      BspecB   
	NOP
	ba,a    BretB    
i5:	sll     %l2,1,%g1
	orcc    %g1,%l3,%g1
	bne     Bnan           
	subcc   %l7,0x7ff,%g0
	bne     BretA          
	xorcc   %i0,%i2,%g1
	bneg    Bnan     
	NOP
	ba,a    BretB    

BspecB:	subcc   %l7,0,%l7
	bne     i8       
	NOP
	addcc   %l6,%l6,%l6
	addx    %l5,%l5,%l5
	orcc    %l5,%l6,%g1
	be      BretA    
	NOP

/*	scan	%l5,0,%i4	*/
	SCAN | (lo5 << 14) | 0 | (in4 << 25)
	subcc	%i4,63,%g0
	bne	L13             
	NOP
	or	%l6,0,%l5
	or	%g0,0,%l6
	sub	%l7,32,%l7
/*	scan	%l5,0,%i4	*/
	SCAN | (lo5 << 14) | 0 | (in4 << 25)
	subcc	%i4,63,%g0
	be,a	L13             
	or	%i4,0,%l7
L13:	subcc	%g0,%i4,%i5
	be	Blab2           
	sub	%l7,%i4,%l7
	sll	%l5,%i4,%l5
	srl	%l6,%i5,%i5
	or	%i5,%l5,%l5
	sll	%l6,%i4,%l6

	ba,a    Blab2    
i8:	sll     %l5,1,%g1
	orcc    %g1,%l6,%g1
	bne     Bnan     
	NOP

BretB:	or      %g0,%i2,%i0
	ba      B999     
	or      %g0,%i3,%i1

BretA:
	ba,a    B999     

Bnan:	sethi   %hi(0xfff80000),%i0
	ba      Bret     
	or      %g0,0,%i1

Bund:	or      %g0,1,%i4
	sub     %i4,%l4,%i4

L15:	subcc	%i4,32,%g0
	bcs	L16             
	orcc	%l3,0,%g0
	sub	%i4,32,%i4
	or	%l2,0,%l3
	or	%g0,0,%l2
	be	L17             
	orcc	%l3,0,%g0
	or	%l3,2,%l3
L17:	bne	L15             
	NOP
	ba,a	L18             
L16:	subcc	%g0,%i4,%i5
	be	L18             
	sll	%l3,%i5,%l0
	orcc	%l0,0,%g0
	sll	%l2,%i5,%i5
	srl	%l3,%i4,%l3
	srl	%l2,%i4,%l2
	or	%l3,%i5,%l3
	bne,a	L18             
	or	%l3,2,%l3

L18:	ba      Blab8    
	or      %g0,0,%l4

Binf:	sethi   %hi(0x7ff00000),%i0
	ba      Bret     
	or      %g0,0,%i1

	.global	_dpmul
_dpmul:
	.global	__dmul
__dmul:
	save	%sp,-96,%sp

	sll     %i0,1,%l0
	srl     %l0,21,%l0
	sll     %i0,11,%i4
	srl     %i1,21,%g1
	or      %i4,%g1,%i4
	sll     %i1,11,%i5
	sethi   %hi(0x80000000),%l5
	or      %i4,%l5,%i4
	sll     %i2,1,%l3
	srl     %l3,21,%l3
	sll     %i2,11,%l1
	srl     %i3,21,%g1
	or      %l1,%g1,%l1
	sll     %i3,11,%l2
	or      %l1,%l5,%l1
	xor     %i0,%i2,%g1
	and     %l5,%g1,%l5

	sub     %l0,1,%g1
	subcc   %g1,0x7fe,%g0
	bcc     CspecA         

Clab1:	sub     %l3,1,%g1
	subcc   %g1,0x7fe,%g0
	bcc     CspecB   
	NOP

Clab2:	umul	%i4,%l1,%i2	
	rd	%y,%l6
	umul	%i5,%l2,%l4	
	rd	%y,%i3
	umul	%i4,%l2,%l2	
	rd	%y,%l7
	addcc   %i3,%l2,%i3
	addxcc  %i2,%l7,%i2
	addx    %l6,0,%l6
	umul	%i5,%l1,%i5	
	rd	%y,%l7
	addcc   %i3,%i5,%i3
	addxcc  %i2,%l7,%i2
	addx    %l6,0,%l6
	orcc    %l4,%i3,%g1
	bne,a	i10              
	or      %i2,1,%i2
i10:	orcc    %g0,%l6,%i4
	bneg    i11      
	or      %g0,%i2,%i5
	addcc   %i5,%i5,%i5
	addx    %i4,%i4,%i4
	sub     %l0,1,%l0
i11:	sub     %l3,0x3fe,%g1
	add     %l0,%g1,%l0
	sub     %l0,1,%g1
	subcc   %g1,0x7fe,%g0
	bcc     Coveund  
	NOP

Clab8:	addcc   %i5,0x400,%i5
	addxcc  %i4,0,%i4
	addx    %l0,0,%l0
	srl     %i5,11,%g1
	and     %g1,1,%g1
	sub     %i5,%g1,%i5
	srl     %i5,11,%i1
	sll     %i4,21,%g1
	or      %i1,%g1,%i1
	srl     %i4,11,%i0
	sethi   %hi(0xfffff),%g1
	or      %g1,0x3ff,%g1
	and     %i0,%g1,%i0
	sll     %l0,20,%g1
	or      %i0,%g1,%i0

Cret:	or      %i0,%l5,%i0

Cret2:
C999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

CspecA:	subcc   %l0,0,%l0
	bne     i12      
	NOP
	addcc   %i5,%i5,%i5
	addx    %i4,%i4,%i4
	orcc    %i4,%i5,%g1
	be      i12      
	NOP

/*	scan	%i4,0,%l6	*/
	SCAN | (in4 << 14) | 0 | (lo6 << 25)
	subcc	%l6,63,%g0
	bne	L19             
	NOP
	or	%i5,0,%i4
	or	%g0,0,%i5
	sub	%l0,32,%l0
/*	scan	%i4,0,%l6	*/
	SCAN | (in4 << 14) | 0 | (lo6 << 25)
	subcc	%l6,63,%g0
	be,a	L19             
	or	%l6,0,%l0
L19:	subcc	%g0,%l6,%i2
	be	Clab1           
	sub	%l0,%l6,%l0
	sll	%i4,%l6,%i4
	srl	%i5,%i2,%i2
	or	%i2,%i4,%i4
	sll	%i5,%l6,%i5

	ba,a    Clab1    
i12:	add     %l0,%l3,%g1
	subcc   %g1,0x7ff,%g0
	be      Cnan           
	subcc   %l0,0,%l0
	be      Czer           
	sll     %i4,1,%g1
	orcc    %g1,%i5,%g1
	bne     Cnan           
	subcc   %l3,0x7ff,%g0
	bne     Cinf           

CspecB:	subcc   %l3,0,%l3
	bne     i14      
	NOP
	addcc   %l2,%l2,%l2
	addx    %l1,%l1,%l1
	orcc    %l1,%l2,%g1
	be      Czer     
	NOP

/*	scan	%l1,0,%l7	*/
	SCAN | (lo1 << 14) | 0 | (lo7 << 25)
	subcc	%l7,63,%g0
	bne	L21             
	NOP
	or	%l2,0,%l1
	or	%g0,0,%l2
	sub	%l3,32,%l3
/*	scan	%l1,0,%l7	*/
	SCAN | (lo1 << 14) | 0 | (lo7 << 25)
	subcc	%l7,63,%g0
	be,a	L21             
	or	%l7,0,%l3
L21:	subcc	%g0,%l7,%l6
	be	Clab2           
	sub	%l3,%l7,%l3
	sll	%l1,%l7,%l1
	srl	%l2,%l6,%l6
	or	%l6,%l1,%l1
	sll	%l2,%l7,%l2

	ba,a    Clab2    
i14:	sll     %l1,1,%g1
	orcc    %g1,%l2,%g1
	bne     Cnan     
	NOP

Cinf:	sethi   %hi(0x7ff00000),%i0
	ba      Cret     
	or      %g0,0,%i1

Cnan:	sethi   %hi(0xfff80000),%i0
	ba      Cret     
	or      %g0,0,%i1

Coveund:
	subcc   %l0,0,%l0
	bg      Cinf     
	NOP
	or      %g0,1,%l6
	sub     %l6,%l0,%l6

L23:	subcc	%l6,32,%g0
	bcs	L24             
	orcc	%i5,0,%g0
	sub	%l6,32,%l6
	or	%i4,0,%i5
	or	%g0,0,%i4
	be	L25             
	orcc	%i5,0,%g0
	or	%i5,2,%i5
L25:	bne	L23             
	NOP
	ba,a	L26             
L24:	subcc	%g0,%l6,%i2
	be	L26             
	sll	%i5,%i2,%i3
	orcc	%i3,0,%g0
	sll	%i4,%i2,%i2
	srl	%i5,%l6,%i5
	srl	%i4,%l6,%i4
	or	%i5,%i2,%i5
	bne,a	L26             
	or	%i5,2,%i5

L26:	ba      Clab8    
	or      %g0,0,%l0

Czer:	or      %g0,0,%i0
	ba      Cret     
	or      %g0,%i0,%i1

	.global	_dpdiv
_dpdiv:
	.global	__ddiv
__ddiv:
	save	%sp,-96,%sp

	sll     %i0,1,%l0
	srl     %l0,21,%l0
	sll     %i0,11,%i4
	srl     %i1,21,%g1
	or      %i4,%g1,%i4
	sll     %i1,11,%i5
	sethi   %hi(0x80000000),%l4
	or      %i4,%l4,%i4
	sll     %i2,1,%l3
	srl     %l3,21,%l3
	sll     %i2,11,%l1
	srl     %i3,21,%g1
	or      %l1,%g1,%l1
	sll     %i3,11,%l2
	or      %l1,%l4,%l1
	xor     %i0,%i2,%g1
	and     %l4,%g1,%l4

	sub     %l0,1,%g1
	subcc   %g1,0x7fe,%g0
	bcc     DspecA         

Dlab1:	sub     %l3,1,%g1
	subcc   %g1,0x7fe,%g0
	bcc     DspecB   
	NOP

Dlab2:	srl     %i5,1,%i5
	sll     %i4,31,%g1
	srl     %i4,1,%i4
	or      %i5,%g1,%i5

	wr	%i4,0,%y
	or	%g0,%i5,%l5
	orcc	%g0,0,%g0
#if 0
	divscc	%l5,%l1,%l5
	divscc	%l5,%l1,%l5
	divscc	%l5,%l1,%l5
	divscc	%l5,%l1,%l5
	divscc	%l5,%l1,%l5
	divscc	%l5,%l1,%l5
	divscc	%l5,%l1,%l5
	divscc	%l5,%l1,%l5
	divscc	%l5,%l1,%l5
	divscc	%l5,%l1,%l5
	divscc	%l5,%l1,%l5
	divscc	%l5,%l1,%l5
	divscc	%l5,%l1,%l5
	divscc	%l5,%l1,%l5
	divscc	%l5,%l1,%l5
	divscc	%l5,%l1,%l5
	divscc	%l5,%l1,%l5
	divscc	%l5,%l1,%l5
	divscc	%l5,%l1,%l5
	divscc	%l5,%l1,%l5
	divscc	%l5,%l1,%l5
	divscc	%l5,%l1,%l5
	divscc	%l5,%l1,%l5
	divscc	%l5,%l1,%l5
	divscc	%l5,%l1,%l5
	divscc	%l5,%l1,%l5
	divscc	%l5,%l1,%l5
	divscc	%l5,%l1,%l5
	divscc	%l5,%l1,%l5
	divscc	%l5,%l1,%l5
	divscc	%l5,%l1,%l5
	divscc	%l5,%l1,%l5
#else
	DIVSCC | (lo5 << 14) | lo1 | (lo5 << 25)
	DIVSCC | (lo5 << 14) | lo1 | (lo5 << 25)
	DIVSCC | (lo5 << 14) | lo1 | (lo5 << 25)
	DIVSCC | (lo5 << 14) | lo1 | (lo5 << 25)
	DIVSCC | (lo5 << 14) | lo1 | (lo5 << 25)
	DIVSCC | (lo5 << 14) | lo1 | (lo5 << 25)
	DIVSCC | (lo5 << 14) | lo1 | (lo5 << 25)
	DIVSCC | (lo5 << 14) | lo1 | (lo5 << 25)
	DIVSCC | (lo5 << 14) | lo1 | (lo5 << 25)
	DIVSCC | (lo5 << 14) | lo1 | (lo5 << 25)
	DIVSCC | (lo5 << 14) | lo1 | (lo5 << 25)
	DIVSCC | (lo5 << 14) | lo1 | (lo5 << 25)
	DIVSCC | (lo5 << 14) | lo1 | (lo5 << 25)
	DIVSCC | (lo5 << 14) | lo1 | (lo5 << 25)
	DIVSCC | (lo5 << 14) | lo1 | (lo5 << 25)
	DIVSCC | (lo5 << 14) | lo1 | (lo5 << 25)
	DIVSCC | (lo5 << 14) | lo1 | (lo5 << 25)
	DIVSCC | (lo5 << 14) | lo1 | (lo5 << 25)
	DIVSCC | (lo5 << 14) | lo1 | (lo5 << 25)
	DIVSCC | (lo5 << 14) | lo1 | (lo5 << 25)
	DIVSCC | (lo5 << 14) | lo1 | (lo5 << 25)
	DIVSCC | (lo5 << 14) | lo1 | (lo5 << 25)
	DIVSCC | (lo5 << 14) | lo1 | (lo5 << 25)
	DIVSCC | (lo5 << 14) | lo1 | (lo5 << 25)
	DIVSCC | (lo5 << 14) | lo1 | (lo5 << 25)
	DIVSCC | (lo5 << 14) | lo1 | (lo5 << 25)
	DIVSCC | (lo5 << 14) | lo1 | (lo5 << 25)
	DIVSCC | (lo5 << 14) | lo1 | (lo5 << 25)
	DIVSCC | (lo5 << 14) | lo1 | (lo5 << 25)
	DIVSCC | (lo5 << 14) | lo1 | (lo5 << 25)
	DIVSCC | (lo5 << 14) | lo1 | (lo5 << 25)
	DIVSCC | (lo5 << 14) | lo1 | (lo5 << 25)
#endif
	bge	L27       
	 rd	%y,%i4
	addcc	%l1,%i4,%i4
L27:	umul	%l5,%l2,%i2	
	rd	%y,%l7
	or      %g0,0,%i5
	subcc   %i5,%i2,%i5
	subxcc  %i4,%l7,%i4
	bcc     i16      
	NOP

Dlab3:	sub     %l5,1,%l5
	addcc   %i5,%l2,%i5
	addxcc  %i4,%l1,%i4
	bcc     Dlab3          
i16:	subcc   %i4,%l1,%g0
	bne     i17      
	NOP
	or      %g0,%l2,%l7
	or      %g0,%i5,%i4
	or      %g0,0,%l6
	ba      e17      
	or      %g0,%l6,%i2

i17:	wr	%i4,0,%y
	or	%g0,%i5,%l6
	orcc	%g0,0,%g0
#if 0
	divscc	%l6,%l1,%l6
	divscc	%l6,%l1,%l6
	divscc	%l6,%l1,%l6
	divscc	%l6,%l1,%l6
	divscc	%l6,%l1,%l6
	divscc	%l6,%l1,%l6
	divscc	%l6,%l1,%l6
	divscc	%l6,%l1,%l6
	divscc	%l6,%l1,%l6
	divscc	%l6,%l1,%l6
	divscc	%l6,%l1,%l6
	divscc	%l6,%l1,%l6
	divscc	%l6,%l1,%l6
	divscc	%l6,%l1,%l6
	divscc	%l6,%l1,%l6
	divscc	%l6,%l1,%l6
	divscc	%l6,%l1,%l6
	divscc	%l6,%l1,%l6
	divscc	%l6,%l1,%l6
	divscc	%l6,%l1,%l6
	divscc	%l6,%l1,%l6
	divscc	%l6,%l1,%l6
	divscc	%l6,%l1,%l6
	divscc	%l6,%l1,%l6
	divscc	%l6,%l1,%l6
	divscc	%l6,%l1,%l6
	divscc	%l6,%l1,%l6
	divscc	%l6,%l1,%l6
	divscc	%l6,%l1,%l6
	divscc	%l6,%l1,%l6
	divscc	%l6,%l1,%l6
	divscc	%l6,%l1,%l6
#else
	DIVSCC | (lo6 << 14) | lo1 | (lo6 << 25)
	DIVSCC | (lo6 << 14) | lo1 | (lo6 << 25)
	DIVSCC | (lo6 << 14) | lo1 | (lo6 << 25)
	DIVSCC | (lo6 << 14) | lo1 | (lo6 << 25)
	DIVSCC | (lo6 << 14) | lo1 | (lo6 << 25)
	DIVSCC | (lo6 << 14) | lo1 | (lo6 << 25)
	DIVSCC | (lo6 << 14) | lo1 | (lo6 << 25)
	DIVSCC | (lo6 << 14) | lo1 | (lo6 << 25)
	DIVSCC | (lo6 << 14) | lo1 | (lo6 << 25)
	DIVSCC | (lo6 << 14) | lo1 | (lo6 << 25)
	DIVSCC | (lo6 << 14) | lo1 | (lo6 << 25)
	DIVSCC | (lo6 << 14) | lo1 | (lo6 << 25)
	DIVSCC | (lo6 << 14) | lo1 | (lo6 << 25)
	DIVSCC | (lo6 << 14) | lo1 | (lo6 << 25)
	DIVSCC | (lo6 << 14) | lo1 | (lo6 << 25)
	DIVSCC | (lo6 << 14) | lo1 | (lo6 << 25)
	DIVSCC | (lo6 << 14) | lo1 | (lo6 << 25)
	DIVSCC | (lo6 << 14) | lo1 | (lo6 << 25)
	DIVSCC | (lo6 << 14) | lo1 | (lo6 << 25)
	DIVSCC | (lo6 << 14) | lo1 | (lo6 << 25)
	DIVSCC | (lo6 << 14) | lo1 | (lo6 << 25)
	DIVSCC | (lo6 << 14) | lo1 | (lo6 << 25)
	DIVSCC | (lo6 << 14) | lo1 | (lo6 << 25)
	DIVSCC | (lo6 << 14) | lo1 | (lo6 << 25)
	DIVSCC | (lo6 << 14) | lo1 | (lo6 << 25)
	DIVSCC | (lo6 << 14) | lo1 | (lo6 << 25)
	DIVSCC | (lo6 << 14) | lo1 | (lo6 << 25)
	DIVSCC | (lo6 << 14) | lo1 | (lo6 << 25)
	DIVSCC | (lo6 << 14) | lo1 | (lo6 << 25)
	DIVSCC | (lo6 << 14) | lo1 | (lo6 << 25)
	DIVSCC | (lo6 << 14) | lo1 | (lo6 << 25)
	DIVSCC | (lo6 << 14) | lo1 | (lo6 << 25)
#endif
	bge	L28       
	 rd	%y,%i4
	addcc	%l1,%i4,%i4
L28:	umul	%l6,%l2,%i2	
	rd	%y,%l7
e17:	or      %g0,0,%i5
	subcc   %i5,%i2,%i5
	subxcc  %i4,%l7,%i4
	bcc     i18      
	NOP

Dlab4:	sub     %l6,1,%l6
	addcc   %i5,%l2,%i5
	addxcc  %i4,%l1,%i4
	bcc     Dlab4    
	NOP
i18:	sub     %l0,%l3,%l0
	add     %l0,0x3ff,%l0
	subcc   %l5,0,%l5
	bneg    i19      
	NOP
	addcc   %l6,%l6,%l6
	addx    %l5,%l5,%l5
	sub     %l0,1,%l0
i19:	orcc    %i4,%i5,%g1
	bne,a	i20              
	or      %l6,1,%l6
i20:	or      %g0,%l5,%i4
	or      %g0,%l6,%i5
	sub     %l0,1,%g1
	subcc   %g1,0x7fe,%g0
	bcc     Doveund  
	NOP

Dlab8:	addcc   %i5,0x400,%i5
	addxcc  %i4,0,%i4
	addx    %l0,0,%l0
	srl     %i5,11,%g1
	and     %g1,1,%g1
	sub     %i5,%g1,%i5
	srl     %i5,11,%i1
	sll     %i4,21,%g1
	or      %i1,%g1,%i1
	srl     %i4,11,%i0
	sethi   %hi(0xfffff),%g1
	or      %g1,0x3ff,%g1
	and     %i0,%g1,%i0
	sll     %l0,20,%g1
	or      %i0,%g1,%i0

Dret:	or      %i0,%l4,%i0

D999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

Doveund:
	subcc   %l0,0,%l0
	bg      Dinf     
	NOP
	or      %g0,1,%l5
	sub     %l5,%l0,%l5

L29:	subcc	%l5,32,%g0
	bcs	L30             
	orcc	%i5,0,%g0
	sub	%l5,32,%l5
	or	%i4,0,%i5
	or	%g0,0,%i4
	be	L31             
	orcc	%i5,0,%g0
	or	%i5,2,%i5
L31:	bne	L29             
	NOP
	ba,a	L32             
L30:	subcc	%g0,%l5,%l6
	be	L32             
	sll	%i5,%l6,%l7
	orcc	%l7,0,%g0
	sll	%i4,%l6,%l6
	srl	%i5,%l5,%i5
	srl	%i4,%l5,%i4
	or	%i5,%l6,%i5
	bne,a	L32             
	or	%i5,2,%i5

L32:	ba      Dlab8    
	or      %g0,0,%l0

DspecA:	subcc   %l0,0,%l0
	bne     i21      
	NOP
	addcc   %i5,%i5,%i5
	addx    %i4,%i4,%i4
	orcc    %i4,%i5,%g1
	be      i21      
	NOP

/*	scan	%i4,0,%l5	*/
	SCAN | (in4 << 14) | 0 | (lo5 << 25)
	subcc	%l5,63,%g0
	bne	L33             
	NOP
	or	%i5,0,%i4
	or	%g0,0,%i5
	sub	%l0,32,%l0
/*	scan	%i4,0,%l5	*/
	SCAN | (in4 << 14) | 0 | (lo5 << 25)
	subcc	%l5,63,%g0
	be,a	L33             
	or	%l5,0,%l0
L33:	subcc	%g0,%l5,%l6
	be	Dlab1           
	sub	%l0,%l5,%l0
	sll	%i4,%l5,%i4
	srl	%i5,%l6,%l6
	or	%l6,%i4,%i4
	sll	%i5,%l5,%i5

	ba,a    Dlab1    
i21:	subcc   %l0,%l3,%g0
	be      Dnan           
	subcc   %l3,0x7ff,%g0
	be      DspecB         
	subcc   %l0,0,%l0
	be      Dzer           
	sll     %i4,1,%g1
	orcc    %g1,%i5,%g1
	bne     Dnan     
	NOP

Dinf:	sethi   %hi(0x7ff00000),%i0
	ba      Dret     
	or      %g0,0,%i1

DspecB:	subcc   %l3,0,%l3
	bne     i23      
	NOP
	addcc   %l2,%l2,%l2
	addx    %l1,%l1,%l1
	orcc    %l1,%l2,%g1
	be      Dinf     
	NOP

/*	scan	%l1,0,%l5	*/
	SCAN | (lo1 << 14) | 0 | (lo5 << 25)
	subcc	%l5,63,%g0
	bne	L35             
	NOP
	or	%l2,0,%l1
	or	%g0,0,%l2
	sub	%l3,32,%l3
/*	scan	%l1,0,%l5	*/
	SCAN | (lo1 << 14) | 0 | (lo5 << 25)
	subcc	%l5,63,%g0
	be,a	L35             
	or	%l5,0,%l3
L35:	subcc	%g0,%l5,%l6
	be	Dlab2           
	sub	%l3,%l5,%l3
	sll	%l1,%l5,%l1
	srl	%l2,%l6,%l6
	or	%l6,%l1,%l1
	sll	%l2,%l5,%l2

	ba,a    Dlab2    
i23:	sll     %l1,1,%g1
	orcc    %g1,%l2,%g1
	bne     Dnan     
	NOP

Dzer:	or      %g0,0,%i1
	ba      Dret     
	or      %g0,%i1,%i0

Dnan:	sethi   %hi(0xfff80000),%i0
	ba      Dret     
	or      %g0,0,%i1


	.global	__dcmp
	.global	_dpcmp
! ! ! ! !
!  __dcmp:	dp floating point comparison
!  On entry:	o0:o1 = argument A
!		o2:o3 = argument B
!  On exit:	condition codes set
!		o0 =	-1  A < B
!			 0  A == B
!			 1  A > B
! ! ! ! !
__dcmp:				! C ifc
_dpcmp:				! USSW ifc
	or	%o2,%o0,%o4	! two zeroes of any kind are equal
	add	%o4,%o4,%o4
	or	%o4,%o1,%o4
	orcc	%o4,%o3,%o4
	be	cmp98
!
	 andcc	%o0,%o2,%g0
	bneg	cmp10		! j/ both negative
!
!	A & B are both positive or of different sign
!
	 subcc	%o0,%o2,%g0
	bl,a	cmp99		! j/ A < B
	 or	%g0,-1,%o0
	bg,a	cmp99		! j/ A > B
	 or	%g0,1,%o0
	subcc	%o1,%o3,%g0
	bcs,a	cmp99		! j/ A < B
	 or	%g0,-1,%o0
	bgu,a	cmp99		! j/ A > B
	 or	%g0,1,%o0
cmp98:	or	%g0,0,%o0	! A == B
cmp99:	jmpl	%o7+8,%g0	! return
	 orcc	%g0,%o0,%g0	! setting cc
!
!	A & B are both negative
!
cmp10:	 subcc	%o0,%o2,%g0
	bl,a	cmp99		! j/ A > B
	 or	%g0,1,%o0
	bg,a	cmp99		! j/ A < B
	 or	%g0,-1,%o0
	subcc	%o1,%o3,%o0
	bcs,a	cmp99		! j/ A > B
	 or	%g0,1,%o0
	bgu,a	cmp99		! j/ A < B
	 or	%g0,-1,%o0
	jmpl	%o7+8,%g0	! return
	 orcc	%g0,%o0,%g0	! setting cc


	.global	__dtol
	.global	_dptoli
! ! ! ! !
!  __dtol:	dp floating point to long
!  On entry:	o0:o1 = argument A
!  On exit:	o0 =	long
!  Note:	>>>> This routine truncates DOWN <<<<
! ! ! ! !
__dtol:				! C ifc
_dptoli:			! USSW ifc
	sll	%o0,1,%o2	! o2 = exp:mant left justified
	sethi	%hi(0xffe00000),%g1
	subcc	%o2,%g1,%g0
	bgu	tol22		! j/ NaN
	 srl	%o2,21,%o2	! o2 = exp right justified
!
	sll	%o0,11,%o3	! %o3 = mantissa left justified
	sethi	%hi(0x80000000),%g1
	or	%o3,%g1,%o3
	srl	%o1,21,%g1
	or	%g1,%o3,%o3
!
!	convert mantissa to integer
!
	subcc	%o2,1054,%o2 	! check unbiased exponent
	bge	tol22		! j/ exp too large
	 sub	%g0,%o2,%o2
	subcc	%o2,31,%g0
	bg	tol20		! j/ exp too small
	 orcc	%o0,0,%g0
	bge	tol99		! j/ positive number
	 srl	%o3,%o2,%o0	! shift mantissa to long answer
!
	sub	%g0,%o0,%o0	! negative answer
!
tol99:	jmpl	%o7+8,%g0	! return
	 nop
!
!	exponent too small, return 0
!
tol20:	ba	tol99		! exp too small, return 0
	 or	%g0,%g0,%o0
!
!	NaN or exponent too large, return IEEE invalid 0x80000000
!
tol22:	ba	tol99
	sethi	%hi(0x80000000),%o0


	.global	_dptoul
_dptoul:
	save	%sp,-96,%sp

	sll	%i0,11,%i3
	srl	%i1,21,%i5
	sll	%i1,11,%i4
	or	%i3,%i5,%i3
	sethi	%hi(0x80000000),%i5
	or	%i3,%i5,%i3
	sll	%i0,1,%i5
	srl	%i5,21,%i5

	or	%g0,%i3,%l0

	subcc	%i5,0x41f,%g0
	bcc	Lnaninf
	NOP

	or	%g0,0x41e,%i2
	subcc	%i2,%i5,%i2

	subcc	%i2,32,%g0
	bcc,a	L4
	or	%g0,0,%l0
L4:	srl	%l0,%i2,%l0

	subcc	%i0,0x0,%g0
	bge	i28      
	NOP

	subcc	%g0,%l0,%l0

i28:	or	%g0,%l0,%i0

L999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

Lnaninf:
	sethi	%hi(0x80000000),%i0
	ba	L999
	NOP


	.global	__ltod
	.global	__ultod
	.global	_litodp
	.global	_ultodp
! ! ! ! !
!  __ultod:	unsigned long to dp floating point
!  __ltod:	long to dp floating point
!  On entry:	o0 =	argument A
!  On exit:	o0:o1 =	dp floating point
! ! ! ! !
__ltod:				! C ifc
_litodp:			! USSW ifc
	orcc	%o0,0,%g0
	bl	tod10		! j/ negative

__ultod:			! C ifc
_ultodp:			! USSW ifc
	 or	%g0,31+0x3ff,%o4	! o4 = anticipated exp
!
!	normalize mantissa to o2:o3
!
tod01:
/*	scan	%o0,0,%g1	! count zero bits	*/
	SCAN | (out0 << 14) | 0 | (gl1 << 25)
	subcc	%g1,63,%g0
	be	tod22		! j/ zero
	 sll	%o0,%g1,%o2	! o2 = long left justified
	sll	%o2,1,%o2	! toss hidden bit
	sll	%o2,20,%o3	! o3 = low order bits
	srl	%o2,12,%o2	! room for sign & mant
!
	sub	%o4,%g1,%o4	! adjust exp
	sll	%o4,20,%o4	! position exp
	or	%o2,%o4,%o2	! combine exp & mant
!
!	o0:o1 = answer
!
tod99:	or	%g0,%o2,%o0	! o0:o1 = answer
	jmpl	%o7+8,%g0	! return
	 or	%g0,%o3,%o1
!
tod22:	or	%g0,0,%o0	! return 0
	jmpl	%o7+8,%g0
	 or	%g0,0,%o1
!
!	negate long
!	normalize mantissa to o2:o3
!
tod10:	sub	%g0,%o0,%o0	! negate long
/*	scan	%o0,0,%g1	! count zero bits	*/
	SCAN | (out0 << 14) | 0 | (gl1 << 25)
	sll	%o0,%g1,%o2	! o2 = long left justified
	sll	%o2,1,%o2	! toss hidden bit
	sll	%o2,20,%o3	! o3 = low order bits
	srl	%o2,12,%o2	! room for sign & mant
!
	sub	%o4,%g1,%o4	! adjust exp
	or	%o4,0x800,%o4	! combine sign & exp
	sll	%o4,20,%o4	! position sign & exp
	ba	tod99
	 or	%o2,%o4,%o2	! combine sign & exp & mant


	.global	_fptodp
_fptodp:
	.global	__ftod
__ftod:
	save	%sp,-96,%sp

	srl     %i0,23,%i3
	and     %i3,0xff,%i3
	sethi   %hi(0x80000000),%i4
	and     %i0,%i4,%i1
	sub     %i3,1,%i5
	subcc   %i5,0xfe,%g0
	bcc     Gspec    
	NOP

Glab1:	add     %i3,0x380,%i3
	sll     %i0,9,%i5
	srl     %i5,12,%i5
	or      %i1,%i5,%i1
	sll     %i0,29,%i2

Gret:	sll     %i3,20,%i5
	or      %i1,%i5,%i1

Gret2:	or      %g0,%i1,%i0
	or      %g0,%i2,%i1

G999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

Gspec:	or      %g0,0,%i2
	subcc   %i3,0,%i3
	bne     j28      
	NOP
	addcc   %i0,%i0,%i0
	be      Gret2    
	NOP
/*	scan	%i0,0,%i4	*/
	SCAN | (in0 << 14) | 0 | (in4 << 25)
	sll	%i0,%i4,%i0
	subcc	%i3,%i4,%i3
	srl     %i0,8,%i0
	ba      Glab1    
	add     %i3,8,%i3
j28:	or      %g0,0x7ff,%i3
	sethi   %hi(0xff800000),%l0
	andncc  %i0,%l0,%i5
	bne,a	Gret             
	sethi   %hi(0xfff80000),%i1
	ba,a    Gret     

	.global	_dptofp
_dptofp:
	.global	__dtof
__dtof:
	save	%sp,-96,%sp

	sll     %i0,1,%i4
	srl     %i4,21,%i4
	sll     %i0,11,%i2
	srl     %i1,21,%l1
	or      %i2,%l1,%i2
	sll     %i1,11,%i3
	sethi   %hi(0x80000000),%i5
	or      %i2,%i5,%i2
	and     %i5,%i0,%i5
	sub     %i4,0x380,%i4
	sub     %i4,1,%l1
	subcc   %l1,0xfe,%g0
	bcc     Hspec          

Hlab1:	subcc   %i3,0,%i3
	bne,a	i30              
	or      %i2,1,%i2
i30:	addcc   %i2,0x80,%i2
	addx    %i4,0,%i4
	srl     %i2,8,%l1
	and     %l1,1,%l1
	sub     %i2,%l1,%i2
	sll     %i2,1,%i2
	srl     %i2,9,%i2
	sll     %i4,23,%l1
	or      %i2,%l1,%i2
	or      %i2,%i5,%i0

H999:	jmpl	%i7+8,%g0
	restore	%g0,0,%g0

Hspec:	subcc   %i4,0x47f,%g0
	bne     i31            
	sll     %i2,1,%l1
	orcc    %l1,%i3,%l1
	be      i31      
	NOP
	sethi   %hi(0x7fc00000),%l1
	ba      H999     
	or      %l1,%i5,%i0
i31:	subcc   %i4,0xff,%g0
	bl      i33      
	NOP
	sethi   %hi(0x7f800000),%l1
	ba      H999     
	or      %l1,%i5,%i0
i33:	or      %g0,1,%l0
	sub     %l0,%i4,%l0
	subcc   %l0,0x20,%g0
	bcs     i34      
	NOP
	or      %i3,%i2,%i3
	ba      e34      
	or      %g0,0,%i2
i34:	or      %g0,0x20,%l2
	sub     %l2,%l0,%l2
	sll     %i2,%l2,%l1
	or      %i3,%l1,%i3
	srl     %i2,%l0,%i2
e34:	ba      Hlab1    
	or      %g0,0,%i4

	.global	_dpfabs
_dpfabs:
	sethi	%hi(0x80000000),%g1
	jmpl	%o7+8,%g0
	andn	%o0,%g1,%o0

!	.end
