/* Copyright 1991-2001 Wind River Systems, Inc. */

/*
modification history
--------------------
01e,28aug01,hdn  added FUNC/FUNC_LABEL,	replaced .align with .balign
01d,16apr98,hdn  fixed comments which cause assembler error.
01c,03feb98,hdn  fixed sin/cos problem (SPR7764) w USSW v.311
01b,24oct96,yp   stuck in a # in USSC mailing addr so cpp will work
01a,24jan95,hdn  original US Software version.
*/

#
#       Filename:   EMUFNC.ASM
#
#       Copyright (C) 1990,1991 By
#       # United States Software Corporation
#       # 14215 N.W. Science Park Drive
#       # Portland, Oregon 97229
#
#       This software is furnished under a license and may be used
#       and copied only in accordance with the terms of such license
#       and with the inclusion of the above copyright notice.
#       This software or any other copies thereof may not be provided
#       or otherwise made available to any other person.  No title to
#       and ownership of the software is hereby transferred.
#
#       The information in this software is subject to change without
#       notice and should not be construed as a commitment by United
#       States Software Corporation.
#
#       Version:        V3.09  01/19/95 GOFAST  WRS GNU version of GF-PROT
#       Released:       1 March 1991
#
#

#define _ASMLANGUAGE
#include "vxWorks.h"
#include "asm.h"
	
	.data
	.globl	FUNC(copyright_wind_river)
	.long	FUNC(copyright_wind_river)

	.include	"emuIncALib.s"
	.text

#        extrn   wiep:near
#        extrn   tnormal:near,shiftr:near,exproc:near,exprop:near
#        extrn   epmul:near
#        extrn   epadd1:near,epmul1:near,epdiv1:near
#        extrn   retnan:near,getqn:near,chkarg:near,exret:near

#DGROUP  GROUP   hwseg
#CGROUP  GROUP   emuseg

#hwseg   segment RW para public 'DATA'
#        assume  ds:DGROUP, es:DGROUP, ss:DGROUP
#        extrn   h_ctrl:word,h_stat:word
#hwseg   ends

#emuseg  segment public USE32 'CODE'
#        assume  cs:CGROUP

__lib:

#        subttl  (2 to power x) - 1
#        page

# raise 2 to power x and subtract 1 (x between -1 and 1)
# input:        EP in cx:edx:eax
# output:       result in cx:edx:eax

	.globl	epf2xm1

	.balign	16
expcon:	.word	0,0xf717,0x0001,0x0000,0x0000,0x0000 # ln2^18/18!
	.word	0,0x0889,0x0033,0x0000,0x0000,0x0000 # ln2^17/17!
	.word	0,0xa26b,0x04e3,0x0000,0x0000,0x0000 # ln2^16/16!
	.word	0,0xa10e,0x70db,0x0000,0x0000,0x0000 # ln2^15/15!
	.word	0,0x26ac,0x8a4b,0x0009,0x0000,0x0000 # ln2^14/14!
	.word	0,0x8b36,0xb0c9,0x00c0,0x0000,0x0000 # ln2^13/13!
	.word	0,0x7e14,0xeb28,0x0e1d,0x0000,0x0000 # ln2^12/12!
	.word	0,0x8dd9,0x639a,0xf465,0x0000,0x0000 # ln2^11/11!
	.word	0,0xc764,0x8ac5,0x267a,0x000f,0x0000 # ln2^10/10!
	.word	0,0x3e1e,0x9caf,0x929e,0x00da,0x0000 # ln2^9/9!
	.word	0,0x11fe,0xd2e4,0x0111,0x0b16,0x0000 # ln2^8/8!
	.word	0,0x1a1b,0x22c3,0xff16,0x7ff2,0x0000 # ln2^7/7!
	.word	0,0xdbd2,0xb1e1,0x4be1,0x0c24,0x0005 # ln2^6/6!
	.word	0,0x20e2,0xce62,0xcf14,0xb0ff,0x002b # ln2^5/5!
	.word	0,0x9ccb,0xe772,0xfba4,0x2ab6,0x013b # ln2^4/4!
	.word	0,0xcce9,0x2fe2,0xc128,0xc235,0x071a # ln2^3/3!
	.word	0,0x6f16,0x8ea8,0x82c5,0xbdff,0x1ebf # ln2^2/2!
	.word	0,0xe4f1,0xbcd5,0xe8e7,0x0bfb,0x58b9 # ln2^1/1!
	.word	-1

exp87:	orl	%ecx,%ecx	#argument + or - 1
	jns	exp89		#   +1 returns +1
	decl	%ecx		#   -1 returns -1/2
exp89:	ret			

expw:	btl	$16,%ecx	#zero returns as such
	jc	exp89		

	call	chkarg		#weed out illegals
	jc	exp89		

	cmpw	$0x7fff,%cx	#-INF returns -1, +INF as such
	jnz	exp2		
	orl	%ecx,%ecx	
	jns	exp89		
	movl	$0x80003fff,%ecx	
	ret			

epf2xm1:			
	testl	$0x00030000,%ecx	#jump if special value
	jnz	expw		

exp2:	#PREEXS  precis          #set precision exception
	orb	$precis,h_stat	
	testb	$precis,h_ctrl	
	jnz	LL1		
	orw	$0x8080,h_stat	
	movl	$exret,-4(%ebp)	
LL1:				

	cmpw	$0x3fff,%cx	#handle limit cases +1 and -1
	jz	exp87		

	movl	$expcon,%ebx	#call the polynomial routine
	call	sigmax		
	addw	$0x3ffe,%cx	

	call	epmul		#then multiply with x
	ret			

# sin: trigonometric sine of x
# cos: trigonometric cosine of x
# tan: trigonometric tangent of x
# input:        x in ecx:edx:eax and ST
# return:       answer in ecx:edx:eax

	.globl	epsin,epcos,eptan

	.balign	16
sincon:				#coefficients for sin(x)
	.word	0,0x1dc0,0x654b,0x0000,0x0000,0x0000
	.word	0,0x6332,0x6030,0xff94,0xffff,0xffff
	.word	0,0xa1b4,0x184e,0x5849,0x0000,0x0000
	.word	0,0x763a,0x3015,0x3375,0xffca,0xffff
	.word	0,0x338e,0x56c7,0xe3a5,0x171d,0x0000
	.word	0,0x7f98,0x97f9,0xf97f,0x7f97,0xfff9
	.word	0,0x1111,0x1111,0x1111,0x1111,0x0111
	.word	0,0xaaab,0xaaaa,0xaaaa,0xaaaa,0xeaaa
	.word	0,0x0000,0x0000,0x0000,0x0000,0x8000
	.word	-1

	.balign	16
coscon:				#coefficients for cos(x)
	.word	0,0x61b8,0xfa5f,0xffff,0xffff,0xffff
	.word	0,0xf9cc,0xb9fc,0x0006,0x0000,0x0000
	.word	0,0xcfe2,0xa2d5,0xf9b1,0xffff,0xffff
	.word	0,0x3624,0x3bfe,0x7bb6,0x0004,0x0000
	.word	0,0x1472,0x10ec,0x3609,0xfdb0,0xffff
	.word	0,0xd00d,0x0d00,0x00d0,0xd00d,0x0000
	.word	0,0x7d28,0x27d2,0xd27d,0x7d27,0xffd2
	.word	0,0x5555,0x5555,0x5555,0x5555,0x0555
	.word	0,0x0000,0x0000,0x0000,0x0000,0xc000
	.word	0,0x0000,0x0000,0x0000,0x0000,0x8000
	.word	-1

	.balign	16
tancon:				#coefficients for tan(x)
	.word	0,0x49fa,0xa843,0xffff,0xffff,0xffff
	.word	0,0xd55c,0x9e11,0xfffc,0xffff,0xffff
	.word	0,0x5074,0x9d9a,0xffde,0xffff,0xffff
	.word	0,0xcc5e,0x827f,0xfeb6,0xffff,0xffff
	.word	0,0xb1e6,0x0f07,0xf34c,0xffff,0xffff
	.word	0,0xd54e,0x9a5c,0x82a0,0xffff,0xffff
	.word	0,0xb45a,0x38a6,0x2a9e,0xfffb,0xffff
	.word	0,0xd548,0xc10c,0x4b63,0xffd0,0xffff
	.word	0,0xe9e2,0xb1f6,0x24d3,0xfe29,0xffff
	.word	0,0x810a,0xbbda,0xfa29,0xedd7,0xffff
	.word	0,0x12ec,0x478a,0x86a0,0x4cab,0xffff
	.word	0,0x54e8,0xc43d,0x1b32,0x10a2,0xfff9
	.word	0,0x5100,0xaa65,0x0ffb,0xa655,0xffba
	.word	0,0xd27d,0x7d27,0x27d2,0xd27d,0xfd27
	.word	0,0x5555,0x5555,0x5555,0x5555,0xd555
	.word	0,0x0000,0x0000,0x0000,0x0000,0x8000
	.word	-1

epsin:				
	#PREEXS  precis          #not precise
	orb	$precis,h_stat	
	testb	$precis,h_ctrl	
	jnz	LL2		
	orw	$0x8080,h_stat	
	movl	$exret,-4(%ebp)	
LL2:				

	cmpb	$4,%bl		#sin(x+pi) = -sinx
	jc	sin2		#   so if octant > 3 we flip the sign
	xorb	$0x80,11(%edi)	#   and subtract 4 from octant number
	andb	$-5,%bl		

sin2:	jpo	cos3		#if octant is 1 or 2 we take cosx

sin3:	movl	%eax,(%edi)	#store reduced value, leave sign
	movl	%edx,4(%edi)	
	movw	%cx,8(%edi)	

	call	square		#we use x^2 in the polynomial

	lea	sincon,%ebx	#do the polynomial
	call	sigmax		
	addw	$0x3ffe,%cx	

	call	epmul1		#multiply with x
	ret			

epcos:				
	#PREEXS  precis          #not precise
	orb	$precis,h_stat	
	testb	$precis,h_ctrl	
	jnz	LL3		
	orw	$0x8080,h_stat	
	movl	$exret,-4(%ebp)	
LL3:				

	andl	$0xffff,%ecx	#store for possible use, sign cleared
	movl	%ecx,8(%edi)	

	testb	$2,%bl		#we flip the sign in octants 2 and 3
	jz	cos2		
	xorb	$0x80,11(%edi)	

cos2:	cmpb	$4,%bl		#cos(x+pi) = -cosx
	jc	cos2b		#   so if octant > 3 we flip the sign
	xorb	$0x80,11(%edi)	#   and subtract 4 from octant number
	andb	$-5,%bl		

cos2b:	jpo	sin3		#if octant is 1 or 2 we take sinx

cos3:	call	square		#we use x^2 in the polynomial

	lea	coscon,%ebx	#do the polynomial
	call	sigmax		
	addw	$0x3ffe,%cx	

	movw	$0,8(%edi)	#put in the sign
	orl	8(%edi),%ecx	
	ret			

eptan:				
	#PREEXS  precis          #not precise
	orb	$precis,h_stat	
	testb	$precis,h_ctrl	
	jnz	LL4		
	orw	$0x8080,h_stat	
	movl	$exret,-4(%ebp)	
LL4:				
	movl	%eax,(%edi)	
	movl	%edx,4(%edi)	
	movw	%cx,8(%edi)	

	andb	$-5,%bl		#tan(x+pi) = tan(x) 

	testb	$2,%bl		#if octant is 2 or 3 flip sign
	jz	tan3		
	xorb	$0x80,11(%edi)	

tan3:	call	square		#we use x^2 in the polynomial

	pushl	%ebx		#save the octant

	lea	tancon,%ebx	#do the polynomial
	call	sigmax		
	addw	$0x3ffe,%cx	

	popl	%ebx		#tan(x) = x/SUM
	orb	%bl,%bl		#   if octant is 1 or 2 we take 1/tan(x)
	jpo	tan7		
	xchgw	8(%edi),%cx	
	xchgl	4(%edi),%edx	
	xchgl	0(%edi),%eax	
tan7:	call	epdiv1		

	ret			#return

#        subttl   base 2 logarithm of x
#        page

# take base 2 logarithm of x
# input:        x in cx:edx:eax and ST
# output:       log2(x) in cx:edx:eax

	.globl	eplog,eplog1

	.balign	16
logcon:	.word	0,0x26b2,0xff99,0xd343,0x0f07,0x04dc #log2(e)/19
	.word	0,0xd0e5,0xc350,0xdd0f,0x6b26,0x056e #log2(e)/17
	.word	0,0xb98d,0xff7d,0xa533,0xcec5,0x0627 #log2(e)/15
	.word	0,0x388f,0xd807,0x34c5,0x3d5a,0x071a #log2(e)/13
	.word	0,0x2b91,0x1694,0xca01,0xd424,0x0864 #log2(e)/11
	.word	0,0x3540,0x54c7,0xbe01,0x589e,0x0a42 #log2(e)/9
	.word	0,0xb22e,0x6c9f,0x3d6f,0xbb15,0x0d30 #log2(e)/7
	.word	0,0x2ca7,0xfe79,0xef9b,0x6c50,0x1277 #log2(e)/5
	.word	0,0x9fc1,0xfd74,0x3a03,0x09dc,0x1ec7 #log2(e)/3
	.word	0,0xdf44,0xf85d,0xae0b,0x1d94,0x5c55 #log2(e)
	.word	-1

	.balign	16
flnlog:	.word	0x0000,0x0000,0x0000,0x8000,0x3fff,0x8000 #log2(0.5) 
	.word	0xf0c1,0x0852,0xcb8c,0xd47f,0x3ffe,0x8000 #log2(0.5625)
	.word	0xd407,0xcb91,0x1ed0,0xad96,0x3ffe,0x8000 #log2(0.625)
	.word	0xc407,0x3457,0xb07f,0x8a62,0x3ffe,0x8000 #log2(0.6875)
	.word	0xf0c1,0x0852,0xcb8c,0xd47f,0x3ffd,0x8000 #log2(0.75)
	.word	0x432d,0x8773,0xf71b,0x995f,0x3ffd,0x8000 #log2(0.8125)
	.word	0x9333,0xfde9,0xc055,0xc544,0x3ffc,0x8000 #log2(0.875)
	.word	0x633a,0x7dda,0x24b6,0xbeb0,0x3ffb,0x8000 #log2(0.9375)
	.word	0x0000,0x0000,0x0000,0x0000,0x0000,1 #log2(1)
	.word	0x3cfd,0xdeb4,0xd1cf,0xae00,0x3ffc,0 #log2(1.125)
	.word	0x57f2,0x68dc,0xc25e,0xa4d3,0x3ffd,0 #log2(1.25)
	.word	0x77f2,0x9750,0x9f01,0xeb3a,0x3ffd,0 #log2(1.375)
	.word	0x87a0,0xfbd6,0x1a39,0x95c0,0x3ffe,0 #log2(1.5)
	.word	0x5e69,0x3c46,0x0472,0xb350,0x3ffe,0 #log2(1.625)
	.word	0x9b33,0x8085,0xcfea,0xceae,0x3ffe,0 #log2(1.75)
	.word	0xb399,0x3044,0xfb69,0xe829,0x3ffe,0 #log2(1.875)

logzer:	orb	$zerod,h_stat	#return -INF
	call	exproc		
	btsl	$31,%edx	
	jmp	log88		

lognan:	orb	$invop,h_stat	#return NaN
	call	exproc		
	movl	$0xc0000000,%edx	
log88:	movl	$0x80027fff,%ecx	
	xorl	%eax,%eax	
log89:	ret			

logw:	btl	$16,%ecx	#zero returns -INF
	jc	logzer		

	call	chkarg		#weed out illegals
	jc	log89		

	orl	%ecx,%ecx	#negative illegal
	js	lognan		

	cmpw	$0x7fff,%cx	#+INF returns as such
	jz	log89		

	call	tnormal		#handle denormal
	#PUTEP   
	movl	%eax,(%edi)	
	movl	%edx,4(%edi)	
	movl	%ecx,8(%edi)	

eplog:				
	testl	$0x80030000,%ecx	#jump if special value
	jnz	logw		

log2:	movl	%edx,%ebx	#precision exception unless power of 2
	addl	%ebx,%ebx	
	orl	%eax,%ebx	
	jz	log3		
	#PREEXS  precis
	orb	$precis,h_stat	
	testb	$precis,h_ctrl	
	jnz	LL5		
	orw	$0x8080,h_stat	
	movl	$exret,-4(%ebp)	
LL5:				

log3:	movl	%ecx,%ebx	#we make the exponent 0 or -1
	cmpw	$0x3fff,%cx	#   depending on if x > 1 or < 1
	movw	$0x3fff,%cx	#   push the needed adjustment
	jge	log31		
	decl	%ecx		
log31:	movw	%cx,8(%edi)	
	subl	%ecx,%ebx	
	movswl	%bx,%ebx	
	pushl	%ebx		

	xorl	%ebx,%ebx	#calculate area number (0-15) 
	shldl	$4,%edx,%ebx	
	testb	$1,%cl		
	jnz	log5		
	andb	$7,%bl		

log5:	cmpb	$7,%bl		#handle 0.9375 < x < specially
	jnz	log21		
	incl	%ebx		
	pushl	%ebx		
	stc			
	rcrl	$1,%edx		
	rcrl	$1,%eax		
	incw	%cx		
	xchgl	(%edi),%eax	
	xchgl	4(%edi),%edx	
	xchgw	8(%edi),%cx	
	notl	%edx		
	negl	%eax		
	sbbl	$-1,%edx	
	xorl	$0x80000000,%ecx	
	jmp	log51		

log21:	pushl	%ebx		#interpolation area number

	movl	%edx,%ebx	#registers = x+C
	andl	$0xf0000000,%ebx	
	addl	%ebx,%edx	
	rcrl	$1,%edx		
	rcrl	$1,%eax		
	incw	%cx		

	xchgl	(%edi),%eax	#swap arguments
	xchgl	4(%edi),%edx	
	xchgw	8(%edi),%cx	

	andl	$0x0fffffff,%edx	#registers = x-C
log54:	jnz	log51		
	orl	%eax,%eax	
	jnz	log51		
	xorl	%ecx,%ecx	
	jmp	log57		
log51:	decw	%cx		
	addl	%eax,%eax	
	adcl	%edx,%edx	
	jns	log51		
				
log52:	call	epdiv1		#calculate d = (M-C)/(M+C)
	addb	%bl,%bl		
	adcl	$0,%eax		
	adcl	$0,%edx		
	#PUTEP   
	movl	%eax,(%edi)	
	movl	%edx,4(%edi)	
	movl	%ecx,8(%edi)	

	call	square		#square that

	lea	logcon,%ebx	#calculate polynomial for d^2
	call	sigmax		
	addb	$0x40,%ch	

	call	epmul1		#then multiply by d
	addb	%bl,%bl		
	adcl	$0,%eax		
	adcl	$0,%edx		

log57:	popl	%ebx		#add base log2(C)
	shll	$2,%ebx		
	lea	flnlog(%ebx,%ebx,2),%ebx	
	movl	%cs:(%ebx),%esi	
	movl	%esi,(%edi)	
	movl	%cs:4(%ebx),%esi	
	movl	%esi,4(%edi)	
	movl	%cs:8(%ebx),%esi	
	movl	%esi,8(%edi)	
	call	epadd1		
	addb	%bl,%bl		
	adcl	$0,%eax		
	adcl	$0,%edx		
	#PUTEP   
	movl	%eax,(%edi)	
	movl	%edx,4(%edi)	
	movl	%ecx,8(%edi)	

	popl	%edx		#get exponent adjustment
	call	wiep		#   convert to EP, add to log2(d)
	call	epadd1		
	addb	%bl,%bl		
	adcl	$0,%eax		
	adcl	$0,%edx		
log9:	ret			

lgpw61:	orl	%ecx,%ecx	#-INF returns NaN
	js	lognan		
	ret			

lgpw:	btl	$16,%ecx	#zero returns 0
	jc	log9		

	call	chkarg		#weed out illegals
	jc	log9		

	cmpw	$0x7fff,%cx	#INF jumps
	jz	lgpw61		

lgpw8:	#PREEXS  precis          #very small multiplied by log2(e)
	orb	$precis,h_stat	
	testb	$precis,h_ctrl	
	jnz	LL6		
	orw	$0x8080,h_stat	
	movl	$exret,-4(%ebp)	
LL6:				
	movl	$0x5C17F0BC,0(%edi)	
	movl	$0xB8AA3B29,4(%edi)	
	movl	$0x3fff,8(%edi)	
	call	epmul		
	ret			

eplog1:				
	testl	$0x00030000,%ecx	#jump if special value
	jnz	lgpw		

	cmpb	$0x10,%ch	#very small skip some code
	jc	lgpw8		

	#PREEXS  precis
	orb	$precis,h_stat	
	testb	$precis,h_ctrl	
	jnz	LL7		
	orw	$0x8080,h_stat	
	movl	$exret,-4(%ebp)	
LL7:				

	xorl	%ebx,%ebx	#push zero exponent adjustment
	pushl	%ebx		

	orl	%ecx,%ecx	#go handle x < 0 separately
	js	lgp30		

	subw	$tBIAS-4,%cx	#calculate area number (8-15) 
	cmpw	$9,%cx		
	jnc	lgp3		
	shldl	%cl,%edx,%ebx	
lgp3:	addb	$8,%bl		
	pushl	%ebx		

	movw	$0x4000,%cx	#calculate x+1+C
	subw	8(%edi),%cx	
	call	shiftr		
	btsl	$30,%edx	
	movl	%edx,%ebx	
	andl	$0x78000000,%ebx	
	addl	%ebx,%edx	
	movl	$0x4000,%ecx	

	xchgl	(%edi),%eax	#swap arguments
	xchgl	4(%edi),%edx	
	xchgl	8(%edi),%ecx	

	movl	%ecx,%ebx	#registers = x-C
	addw	$4-tBIAS,%bx	
	jle	log52		
	xchgl	%ecx,%ebx	
	shll	%cl,%edx	
	shrl	%cl,%edx	
	xchgl	%ecx,%ebx	
	jmp	log54		
				
lgp30:	subw	$tBIAS-5,%cx	#calculate area number (0-7) 
	cmpw	$9,%cx		
	jnc	lgp33		
	shldl	%cl,%edx,%ebx	
	negl	%ebx		
lgp33:	addl	$8,%ebx		
	pushl	%ebx		

	movw	$0x3fff,%cx	#calculate x+1+C
	subw	8(%edi),%cx	
	call	shiftr		
	movl	%edx,%esi	
	andl	$0x38000000,%esi	
	addl	%esi,%edx	
	addb	%bl,%bl		
	cmc			
	notl	%eax		
	adcl	$0,%eax		
	notl	%edx		
	adcl	$0,%edx		
	movl	$0x3fff,%ecx	
	adcl	$0,%ecx		
	orl	$0x80000000,%edx	

	xchgl	(%edi),%eax	#swap arguments
	xchgl	4(%edi),%edx	
	xchgl	8(%edi),%ecx	

	movl	%ecx,%ebx	#registers = x-C
	addw	$0xc006,%bx	
	jle	log52		
	xchgl	%ecx,%ebx	
	shll	%cl,%edx	
	shrl	%cl,%edx	
	xchgl	%ecx,%ebx	
	jmp	log54		

#        subttl   arctangent of x
#        page

# take arctangent of x
# input:        x in cx:edx:eax and ST
# output:       atan(x) in cx:edx:eax

	.globl	epatan

	.balign	16
atncon:				#coefficients for polynomial
	.word	0,0x45d1,0x5d17,0xd174,0x1745,0xf45d
	.word	0,0xe38e,0x8e38,0x38e3,0xe38e,0x0e38
	.word	0,0xdb6e,0x6db6,0xb6db,0xdb6d,0xedb6
	.word	0,0x999a,0x9999,0x9999,0x9999,0x1999
	.word	0,0x5555,0x5555,0x5555,0x5555,0xd555
	.word	0,0x0000,0x0000,0x0000,0x0000,0x8000
	.word	-1

	.balign	16
attab:				#interpolation table for n/32
	.word	0x0000,0x0000,0x0000,0x0000,0x0000,1 # atan(0.000000)
	.word	0x2542,0x4bb1,0xaddd,0xffea,0x3ff9,0 # atan(0.031250)
	.word	0x4e37,0x67ef,0xddb9,0xffaa,0x3ffa,0 # atan(0.062500)
	.word	0x7460,0x1788,0xc130,0xbf70,0x3ffb,0 # atan(0.093750)
	.word	0x6e33,0x617b,0xd4d5,0xfead,0x3ffb,0 # atan(0.125000)
	.word	0x62c4,0x3313,0x7746,0x9eb7,0x3ffc,0 # atan(0.156250)
	.word	0x1135,0x72d8,0xda5e,0xbdcb,0x3ffc,0 # atan(0.187500)
	.word	0x1023,0x9305,0xba94,0xdc86,0x3ffc,0 # atan(0.218750)
	.word	0xeb16,0x6406,0xafc9,0xfadb,0x3ffc,0 # atan(0.250000)
	.word	0xc130,0x5f8b,0xad18,0x8c5f,0x3ffd,0 # atan(0.281250)
	.word	0x5e6a,0x3f5e,0xb9b8,0x9b13,0x3ffd,0 # atan(0.312500)
	.word	0x4eda,0x8e6a,0x6cca,0xa985,0x3ffd,0 # atan(0.343750)
	.word	0x8473,0x26f7,0xca0f,0xb7b0,0x3ffd,0 # atan(0.375000)
	.word	0x2b6d,0x50d9,0x69ca,0xc592,0x3ffd,0 # atan(0.406250)
	.word	0xe5b6,0x611f,0x761e,0xd327,0x3ffd,0 # atan(0.437500)
	.word	0x7c68,0x764f,0xa64a,0xe06d,0x3ffd,0 # atan(0.468750)
	.word	0x7b46,0x0dda,0x382b,0xed63,0x3ffd,0 # atan(0.500000)
	.word	0xbe5c,0xa0a0,0xe85a,0xfa06,0x3ffd,0 # atan(0.531250)
	.word	0x7e2a,0xd986,0xf4a6,0x832b,0x3ffe,0 # atan(0.562500)
	.word	0x47b5,0xde95,0xecdf,0x892a,0x3ffe,0 # atan(0.593750)
	.word	0x9f9c,0xf7f5,0x5d5e,0x8f00,0x3ffe,0 # atan(0.625000)
	.word	0x86f6,0x8471,0x72c9,0x94ac,0x3ffe,0 # atan(0.656250)
	.word	0xda20,0x71bd,0x80e6,0x9a2f,0x3ffe,0 # atan(0.687500)
	.word	0xa1ed,0xf4b7,0xfdc4,0x9f89,0x3ffe,0 # atan(0.718750)
	.word	0x0924,0x34f7,0x7d19,0xa4bc,0x3ffe,0 # atan(0.750000)
	.word	0xf5c8,0x4830,0xabdc,0xa9c7,0x3ffe,0 # atan(0.781250)
	.word	0xc080,0xb4d8,0x4c38,0xaeac,0x3ffe,0 # atan(0.812500)
	.word	0x3691,0x1f04,0x31c9,0xb36b,0x3ffe,0 # atan(0.843750)
	.word	0x9e74,0xc231,0x3e2b,0xb805,0x3ffe,0 # atan(0.875000)
	.word	0xf281,0xe98a,0x5dea,0xbc7b,0x3ffe,0 # atan(0.906250)
	.word	0x6640,0xac52,0x85b8,0xc0ce,0x3ffe,0 # atan(0.937500)
	.word	0xbd54,0xbf8f,0xaffa,0xc4ff,0x3ffe,0 # atan(0.968750)

atnw8:	movl	8(%edi),%ecx	#return 3pi/4
	andl	$0x80000000,%ecx	
	movw	$0x4000,%cx	
	movl	$0x96cbe3f9,%edx	
	movl	$0x990e91a7,%eax	
	orb	$precis,h_stat	
	call	exprop		
	ret			

atnzer:	movb	11(%edi),%ch	#return signed zero
	movb	$1,%cl		
	shll	$16,%ecx	
	xorl	%edx,%edx	
	xorl	%eax,%eax	
	ret			

atnw11:	orl	%ecx,%ecx	# INF / INF
	js	atnw8		
	orb	$precis,h_stat	
	call	exprop		
	jmp	atn90		

atnw3:	cmpw	$0x7fff,%cx	# INF / y
	jz	atnw11		
atnw9:	orb	$precis,h_stat	
	call	exprop		
atn82:	movl	$0x3fff,%ecx	#INF returns +- pi/2
	jmp	atn91		

atnw5:	orl	%ecx,%ecx	# y / INF or 0 / x
	jns	atnzer		
	orb	$precis,h_stat	
	call	exprop		
atn84:	movl	$0x4000,%ecx	#here return +-pi
	jmp	atn91		

atnw:				
	call	retnan		#proper return for illegal arguments

	testb	$1,10(%edi)	#weed out x = 0
	jnz	atnw5		

	btl	$16,%ecx	#weed out y = 0
	jc	atnw9		

	cmpw	$0x7fff,8(%edi)	#weed out x = INF
	jz	atnw3		

	cmpw	$0x7fff,%cx	#weed out y = INF
	jz	atnw5		

	call	tnormal		#normalize arguments
	xchgl	(%edi),%eax	
	xchgl	4(%edi),%edx	
	xchgl	8(%edi),%ecx	
	call	tnormal		
	xchgl	(%edi),%eax	
	xchgl	4(%edi),%edx	
	xchgl	8(%edi),%ecx	
	jmp	atn2		

atn90:	orl	%ecx,%ecx	
	js	atnw8		
	movl	$0x3ffe,%ecx	#atan(1) return +-pi/4
atn91:	movl	$0xc90fdaa2,%edx	
	movl	$0x2168c235,%eax	
	jmp	atn23		

atn89:	movb	s_X+5(%ebp),%bl	
	testb	$1,%bl		
	jz	atn82		
	orb	%bl,%bl		
	js	atn84		
	call	epdiv1		
	jmp	atn23		

epatan:				
	shldl	$16,%ecx,%ebx	#save sign bits
	movb	11(%edi),%bl	
	movw	%bx,s_X+4(%ebp)	

	shldl	$16,%ecx,%ebx	#weed out special arguments
	orb	10(%edi),%bl	
	jnz	atnw		

atn2:	#PREEXS  precis          #set precision exception
	orb	$precis,h_stat	
	testb	$precis,h_ctrl	
	jnz	LL8		
	orw	$0x8080,h_stat	
	movl	$exret,-4(%ebp)	
LL8:				

	cmpw	8(%edi),%cx	#compare x and y
	jl	atn5		
	jnz	atn7		
	cmpl	4(%edi),%edx	
	jc	atn5		
	jnz	atn7		
	cmpl	(%edi),%eax	
	jz	atn90		
	jc	atn5		
				
atn7:	xchgl	(%edi),%eax	#switch arguments, set flag
	xchgl	4(%edi),%edx	
	xchgl	8(%edi),%ecx	
	incb	s_X+5(%ebp)	

atn5:	movl	%ecx,%ebx	#underflow here means either
	addw	$tBIAS,%bx	#   atn(INF) = +-pi/2 or
	subw	8(%edi),%bx	#   atn(0) = 0 or pi
	cmpw	$0x3f40,%bx	
	jl	atn89		

atn6:	call	epdiv1		#divide x / y
	andl	$0x7fffffff,%ecx	
	#PUTEP   
	movl	%eax,(%edi)	
	movl	%edx,4(%edi)	
	movl	%ecx,8(%edi)	

	xorl	%ebx,%ebx	#get interpolation interval, 0-31
	subw	$tBIAS-6,%cx	
	cmpw	$9,%cx		
	jc	atn4		
	movw	$0,%cx		
atn4:	shldl	%cl,%edx,%ebx	
	pushl	%ebx		

	shll	%cl,%edx	#calculate x - C = d
	shrl	%cl,%edx	
	movw	8(%edi),%cx	
	js	atn55		
	jnz	atn51		
	orl	%eax,%eax	
	jnz	atn51		
	xorl	%ecx,%ecx	
	jmp	atn54		
atn51:	decw	%cx		
	addl	%eax,%eax	
	adcl	%edx,%edx	
	jns	atn51		

atn55:	pushl	%eax		#push d
	pushl	%edx		
	pushl	%ecx		
				
	rorl	$5,%ebx		#calculate x * C
	movl	4(%edi),%eax	
	mull	%ebx		
	movl	%eax,%ecx	
	xchgl	%ebx,%edx	
	movl	(%edi),%eax	
	mull	%edx		
	addl	%edx,%ecx	
	adcl	$0,%ebx		

	movl	%ebx,%edx	#get x * C into right registers
	xchgl	%ecx,%eax	
	xorl	%ebx,%ebx	
	movl	$0x3fff,%ecx	
	subw	8(%edi),%cx	

atn11:	shrl	$1,%edx		#normalize x * C to exponent 3fff
	rcrl	$1,%eax		
	loop	atn11		

	orl	$0x80000000,%edx	#calculate 1 + x*C
	movw	$0x3fff,%cx	
	#PUTEP   
	movl	%eax,(%edi)	
	movl	%edx,4(%edi)	
	movl	%ecx,8(%edi)	

	popl	%ecx		#calculate d/(1 + x*C)
	popl	%edx		
	popl	%eax		
	call	epdiv1		
	addb	%bl,%bl		
	adcl	$0,%eax		
	adcl	$0,%edx		
	#PUTEP   
	movl	%eax,(%edi)	
	movl	%edx,4(%edi)	
	movl	%ecx,8(%edi)	

	call	square		#polynomial uses x^2

	movl	$atncon,%ebx	#do the polynomial
	call	sigmax		
	addw	$0x3ffe,%cx	

	call	epmul1		#multiply with x

atn54:	popl	%ebx		#then add atan(C)       
	shll	$2,%ebx		
	lea	attab(%ebx,%ebx,2),%ebx	
	movl	%cs:(%ebx),%esi	
	movl	%esi,(%edi)	
	movl	%cs:4(%ebx),%esi	
	movl	%esi,4(%edi)	
	movl	%cs:8(%ebx),%esi	
	movl	%esi,8(%edi)	
	call	epadd1		

	movb	s_X+5(%ebp),%bl	#adjust for actual quadrant
	cmpb	$0x01,%bl	
	jz	atn23		
	cmpb	$0x80,%bl	
	jz	atn25		
	orl	$0x80000000,%ecx	
atn25:	movw	$0x4000,8(%edi)	
	cmpb	$0x81,%bl	
	jz	atn26		
	movw	$0x3fff,8(%edi)	
atn26:	movl	$0xc90fdaa2,4(%edi)	
	movl	$0x2168c235,0(%edi)	
	call	epadd1		

atn23:	roll	$8,%ecx		#put in sign
	movb	s_X+4(%ebp),%cl	
	rorl	$8,%ecx		
	ret			

# routine to compute a Taylor polynomial
# for speed and accuracy, we perform fixed-point scaled calculations
# in    ebx = pointer to coefficient table
#       ecx:edx:eax = EP argument
# out   ecx:edx:eax = EP result

shf22:	movl	%edx,%eax	#big shift here
	xorl	%edx,%edx	
	subw	$32,%cx		
	cmpw	$32,%cx		
	jc	shf9		
	xorl	%eax,%eax	
	jmp	shf19		

sigmax:				
	pushl	%edi		#save registers
	movl	%ecx,%esi	

	subw	$0x3ffe,%cx	#get the scaling factor
	negl	%ecx		
	cmpw	$32,%cx		
	jnc	shf22		

shf9:	shrdl	%cl,%edx,%eax	#shift x right to scale
	shrl	%cl,%edx	

shf19:	movl	%eax,s_A(%ebp)	#store x 
	movl	%edx,s_A+4(%ebp)	

	movw	%cs:2(%ebx),%ax	#initial value of sum = first coefficient
	movl	%cs:4(%ebx),%ecx	
	movl	%cs:8(%ebx),%edx	
	addl	$12,%ebx	

sig5:	pushl	%ebx		#keep constant pointer on the stack
	movl	%edx,%edi	

	mulw	s_A+6(%ebp)	#multiply with x01
	movw	%dx,%si		#   put result in ebx:ecx:si
	movl	%edi,%eax	
	mull	s_A+4(%ebp)	
	movl	%edx,%ebx	
	xchgl	%ecx,%eax	
	orl	%edi,%edi	
	jns	sig42		
	subl	s_A(%ebp),%ecx	
	sbbl	s_A+4(%ebp),%ebx	
sig42:	pushl	%eax		
	mull	s_A+4(%ebp)	
	shrl	$16,%eax	
	addw	%ax,%si		
	adcl	%edx,%ecx	
	adcl	$0,%ebx		

	popl	%eax		#multiply with x23
	shrl	$16,%eax	
	mulw	s_A+2(%ebp)	
	addw	%dx,%si		
	adcl	$0,%ecx		
	adcl	$0,%ebx		
	movl	%edi,%eax	
	mull	s_A(%ebp)	
	shrl	$16,%eax	
	addw	%si,%ax		
	adcl	%edx,%ecx	
	adcl	$0,%ebx		

	movl	%ebx,%edx	#sum now in edx:ecx:ax

	popl	%ebx		#address of constant table in bx

	orl	%esi,%esi	#we may have to complement the sum
	jns	sig31		
	notl	%edx		
	notl	%ecx		
	negw	%ax		
	sbbl	$-1,%ecx	
	sbbl	$-1,%edx	

sig31:	addw	%cs:2(%ebx),%ax	#add next coefficient to sum
	adcl	%cs:4(%ebx),%ecx	
	adcl	%cs:8(%ebx),%edx	

	addl	$12,%ebx	#proceed to next element
	cmpb	$-1,(%ebx)	
	jnz	sig5		

	movb	%ah,%bl		#restore registers
	movl	%ecx,%eax	
	popl	%edi		

	movl	$1,%ecx		#normalize number
	orl	%edx,%edx	
	js	sig18		
	decl	%ecx		
	addb	%bl,%bl		
	adcl	%eax,%eax	
	adcl	%edx,%edx	
sig18:	ret			

#routine to raise number to second power
# in    x in ecx:edx:eax
# out   x^2 in same

squ80:	ret			#return unchanged

square:				
	orw	%cx,%cx		#may be called with zero
	jz	squ80		

	pushl	%edi		#preserve registers
	pushl	%ebx		
	pushl	%ebp		
	pushl	%ecx		

	movl	%edx,%esi	#move mantissa
	movl	%eax,%edi	

	shrl	$16,%eax	#x23 * x23
	mulw	%ax		
	movb	%dh,%bl		

	movl	%esi,%eax	#x01 * x01 -> bp:cx:bl
	mull	%eax		
	movl	%eax,%ecx	
	movl	%edx,%ebp	

	movl	%esi,%eax	#double x01 * x23
	mull	%edi		
	shrl	$16,%eax	
	addb	%al,%bl		
	adcl	%edx,%ecx	
	adcl	$0,%ebp		
	addb	%al,%bl		
	adcl	%edx,%ecx	
	adcl	$0,%ebp		

	movl	%ecx,%eax	#move mantissa to proper place
	movl	%ebp,%edx	

	popl	%ecx		#double exponent
	addl	%ecx,%ecx	
	subw	$tBIAS-1,%cx	

	addb	%bl,%bl		#round
	adcl	$0,%eax		
	adcl	$0,%edx		

	popl	%ebp		#restore registers
	popl	%ebx		
	popl	%edi		
	ret			

# routine to reduce the trigonometric argument to between 0 and pi/4
# returns octant number (0-7) in cl

	.globl	reduct

red80:	orb	$4,h_stat+1	#argument too big
	addl	$4,%esp		
	ret			

red85:	call	getqn		#here INF
	orb	$invop,h_stat	
	call	exproc		
red99:	xorb	%bl,%bl		
	ret			
red96:	andl	$0x8000ffff,%ecx	
	jmp	red99		

redw:	call	chkarg		#weed out illegals
	jc	red99		

	btl	$16,%ecx	#weed out INF, zero
	jc	red99		
	cmpw	$1,%cx		
	jz	red96		
	orw	%cx,%cx		
	jnz	red85		
	btsl	$18,%ecx	
	cmpb	$7,rmcode(%ebp)	
	jz	red99		
	orb	$underf,h_stat	
	call	exproc		
	jmp	red99		

reduct:				
	andb	$0x0fb,h_stat+1	#clear reduction bit

	testl	$0x00030000,%ecx	#jump if special value
	jnz	redw		

	cmpw	$tBIAS-1,%cx	#small x, do nothing
	jc	red99		

	subw	$tBIAS-2,%cx	#get shift count -> cx
	cmpw	$64,%cx		#   reduce only up to 63
	jg	red80		

	pushl	%edi		#preserve register

	movl	$0x2168c234,%edi	
	movl	$0xc90fdaa2,%esi	

	xorl	%ebx,%ebx	#divide by subtraction
red61:	subb	$0x0c0,%bh	#   remainder -> edx:eax:bh
	sbbl	%edi,%eax	
	sbbl	%esi,%edx	
	sbbb	$0,%ch		
	jnc	red63		
	xorb	%ch,%ch		
	addb	$0x0c0,%bh	
	adcl	%edi,%eax	
	adcl	%esi,%edx	
red63:	cmc			
	adcb	%bl,%bl		
	decb	%cl		
	jle	red65		
	addb	%bh,%bh		
	adcl	%eax,%eax	
	adcl	%edx,%edx	
	adcb	$0,%ch		
	jmp	red61		

red65:	andb	$7,%bl		#octant number -> bh

	testb	$1,%bl		#if octant 1 or 3 return pi/4 - x
	jz	red41		
	negb	%bh		
	addb	$0x0c0,%bh	
	notl	%eax		
	adcl	%edi,%eax	
	notl	%edx		
	adcl	%esi,%edx	

red41:	movl	$0x3ffe,%ecx	#exponent of pi/4
	popl	%edi		

red37:	orl	%edx,%edx	#normalize so bit 31 = 1
	js	red19		
	jnz	red25		
	xchgl	%eax,%edx	
	xchgb	%bh,%al		
	shll	$16,%eax	#fix 10/28/97
#	shll	$16,%ebx	
	subl	$32,%ecx	
	jmp	red37		
red25:	decl	%ecx		
	addb	%bh,%bh		
	adcl	%eax,%eax	
	adcl	%edx,%edx	
	jns	red25		

red19:	ret			

#__lib   endp

#emuseg  ends

#        end
