/* Copyright 1991-2001 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,14jul99,tdl  added FUNC / FUNC_LABEL
01b,24oct96,yp   stuck in a # in USSC mailing addr so cpp will work
01a,24jan95,hdn  original US Software version.
*/

#* * * * * * * * * *
#       Filename:   EMUOPER.ASM
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
#* * * * * * * * * *

#define _ASMLANGUAGE
#include "vxWorks.h"
#include "asm.h"

	.data
	.globl  FUNC(copyright_wind_river)
	.long   FUNC(copyright_wind_river)

	.include	"emuIncALib.s"
	.text


#        extrn   roundi:near,rounde:near,chkarg:near
#        extrn   retnan:near,tnormal:near,renorm:near,getqn:near
#        extrn   mul10:near,shiftr:near,exproc:near,exprop:near
#        extrn   exret:near

#DGROUP  GROUP   hwseg
#CGROUP  GROUP   emuseg

#hwseg   segment RW para public 'DATA'
#        assume  ds:DGROUP, es:DGROUP, ss:DGROUP
#        extrn   h_ctrl:word,h_stat:word,s_iptr:dword,s_memp:dword
#hwseg   ends

#emuseg  segment public USE32 'CODE'
#        assume  cs:CGROUP

__lib:

#        subttl  Convert To Internal Floating-Point Format
#        page

# convert 16-bit or 32-bit integer to extended floating-point
# input:        *eax = signed integer
# output:       EP in ecx:edx:eax

	.globl	witoep,wiep
witoep:				
	movswl	(%eax),%edx	
	jmp	wiep2		

	.globl	sitoep
sitoep:				
	movl	(%eax),%edx	#pick up number

wiep2:	#SAVEII
	movl	%esi,s_iptr	
	movl	%eax,s_memp	

wiep:	movl	$31,%ecx	#initial sign -> cx
	xorl	%eax,%eax	

	orl	%edx,%edx	#weed out zero
	jz	liep20		

	jns	siep5		#if negative, set sign, negate
	orl	$0x80000000,%ecx	
	negl	%edx		

siep5:	bsrl	%edx,%esi	
	subl	%esi,%ecx	
	shll	%cl,%edx	
	negw	%cx		
	addw	$tBIAS+31,%cx	
	ret			

# convert 64-bit integer to extended floating-point
# input:        *eax = signed integer
# output:       EP in ecx:edx:eax

	.globl	litoep
litoep:				
	#SAVEII
	movl	%esi,s_iptr	
	movl	%eax,s_memp	

	movl	4(%eax),%edx	#pick up number
	movl	(%eax),%eax	

	movl	%eax,%ecx	#jump if all zero
	orl	%edx,%ecx	
	jz	liep20		

	movl	$tBIAS+63,%ecx	#initial exponent -> cx

	orl	%edx,%edx	#if negative, set sign, negate
	jns	liep5		
	orl	$0x80000000,%ecx	
	notl	%edx		
	negl	%eax		
	sbbl	$-1,%edx	
	js	liep7		

liep5:	decl	%ecx		#shift left until high bit = 1
	addl	%eax,%eax	
	adcl	%edx,%edx	
	jns	liep5		

liep7:	ret			

liep20:	movl	$0x00010000,%ecx	#zero
	ret			

# convert 10-byte packed decimal integer to extended floating-point
# input:        *eax = signed integer
# output:       EP in ecx:edx:eax

	.globl	pdtoep
pdtoep:				
	movl	$9,%ecx		#set up loop counter and pointer
	lea	9(%eax),%edi	

	xorl	%edx,%edx	#initialize mantissa
	xorl	%eax,%eax	
				
pdep3:	call	mul10		#multiply mantissa by 10

	decl	%edi		#pick up next 2-digit
	movb	(%edi),%bl	

	pushl	%ebx		#add low digit to mantissa
	shrb	$4,%bl		
	andl	$0x0f,%ebx	
	addl	%ebx,%eax	
	adcl	$0,%edx		

	call	mul10		#again multiply mantissa by 10

	popl	%ebx		#add high digit to mantissa
	andl	$0x0f,%ebx	
	addl	%ebx,%eax	
	adcl	$0,%edx		

	loop	pdep3		#loop for all digits

	movl	%eax,%ecx	#jump if all zero
	orl	%edx,%ecx	
	jz	pdep20		

	movl	$tBIAS+63,%ecx	#initial exponent -> cx

pdep5:	decl	%ecx		#shift left until high bit = 1
	addl	%eax,%eax	
	adcl	%edx,%edx	
	jns	pdep5		

pdep7:	movb	9(%edi),%bl	#set the sign bit
	andb	$0x80,%bl	
	shll	$24,%ebx	
	orl	%ebx,%ecx	
	ret			

pdep20:	movl	$0x00010000,%ecx	#zero
	jmp	pdep7		

# convert single-precision to extended floating-point
# input:        *eax = single-precision
# output:       EP in ecx:edx:eax

	.globl	fptoep
fptoep:				
	#SAVEII
	movl	%esi,s_iptr	
	movl	%eax,s_memp	

	movl	(%eax),%edx	#pick up number

	movl	%edx,%ecx	#sign, exponent to ecx
	sarl	$23,%ecx	
	andl	$0x800000ff,%ecx	

	shll	$9,%edx		#mantissa -> edx:eax
	xorl	%eax,%eax	

	cmpb	$0,%cl		#go handle zero and denormal
	jz	fpep20		

	cmpb	$-1,%cl		#go handle INF and NaN
	jz	dpep30		
	rcrl	$1,%edx		

fpep7:	addw	$tBIAS-sBIAS,%cx	#adjust bias
	ret			

fpep20:	shrl	$1,%edx		#this is either zero or denormal
	jz	dpep29		#   zero if mantissa = 0
	orb	$denorm,h_stat	
	call	exprop		
	incl	%ecx		
fpep21:	decw	%cx		
	addl	%edx,%edx	
	jns	fpep21		
	jmp	fpep7		

# convert double-precision to extended floating-point
# input:        eax -> double-precision
# output:       EP in ecx:edx:eax

	.globl	dptoep
dptoep:				
	#SAVEII
	movl	%esi,s_iptr	
	movl	%eax,s_memp	

	movl	4(%eax),%edx	#pick up number
	movl	(%eax),%eax	

	movl	%edx,%ecx	#sign, exponent to ecx
	sarl	$20,%ecx	
	andl	$0x800007ff,%ecx	

	shldl	$12,%eax,%edx	#mantissa -> edx:eax
	shll	$11,%eax	
	orw	%cx,%cx		
	jz	dpep20		
	cmpw	$0x7ff,%cx	
	jz	dpep30		
	rcrl	$1,%edx		

dpep7:	addw	$tBIAS-lBIAS,%cx	#adjust exponent bias
dpep9:	ret			

dpep30:	orl	$0x00027fff,%ecx	#this is INF or NaN
	movl	%edx,%ebx	#   INF if mantissa 800..00
	shrdl	$1,%ecx,%edx	
	orl	%eax,%ebx	
	jz	dpep9		
nanret:	btl	$30,%edx	
	jc	dpep32		
	orw	$invop,h_stat	
	call	exproc		
	btsl	$30,%edx	
dpep32:	xorl	%ebx,%ebx	
	ret			

dpep20:	shrl	$1,%edx		#this is either zero or denormal
	movl	%edx,%ebx	#   zero if mantissa = 0
	orl	%eax,%ebx	
	jz	dpep29		
	orb	$denorm,h_stat	
	call	exprop		
	incl	%ecx		
dpep21:	decw	%cx		
	addl	%eax,%eax	
	adcl	%edx,%edx	
	jns	dpep21		
	jmp	dpep7		

dpep29:	btsl	$16,%ecx	#zero
	ret			

# convert extended floating-point number to internal formal
# input:        EP in *ss:eax
# output:       EP in ecx:edx:eax

	.globl	eptoep
eptoep:				
	movl	4(%eax),%edx	#pick up the number
	movzwl	8(%eax),%ecx	
	movl	(%eax),%eax	
	addw	%cx,%cx		
	rcrl	$1,%ecx		

	jz	epe11		#tags: 10 = INF, denormal, illegal
	cmpw	$0x7fff,%cx	#      01 = zero
	jz	epe6		
	orl	%edx,%edx	
	jns	epe6		
	movl	%edx,%ebx	
	orl	%eax,%ebx	
	jz	epe6		
	ret			

epe6:	btsl	$17,%ecx	#illegal etc
	ret			

epe11:	movl	%edx,%ebx	#this is zero or denormal
	orl	%eax,%ebx	
	jnz	epe6		
	btsl	$16,%ecx	
	ret			
				
#        subttl  Convert From Internal Floating-Point Format
#        page

# convert extended floating-point to 16 or 32 bit integer
# input:        EP in ecx:edx:eax
# output:       eax = signed integer

	.globl	eptowi
eptowi:				
	movl	$15,%ebx	#integer size
	jmp	epsi1		

	.globl	eptosi
eptosi:				
	movl	$31,%ebx	#integer size

epsi1:	testl	$0x00020000,%ecx	#weed out specials
	jnz	epsi20		

epsi3:	subw	$tBIAS,%cx	#subtract the bias

	cmpw	%bx,%cx		#if exponent too large, return INF
	jge	epsi32		

	subw	$63,%cx		#here shift right
	negw	%cx		
	call	shiftr		

	call	roundi		#IEEE rounding
	adcl	$0,%eax		

	orl	%ecx,%ecx	#if negative take 2s complement
	jns	epsi11		
	negl	%eax		

epsi11:	ret			#return

epsi20:	rorl	$8,%ecx		#check for stack underflow
	cmpb	$3,%ch		
	roll	$8,%ecx		
	jz	epsi80		

	orw	%cx,%cx		#specials, but denormals go back
	jz	epsi3		
	jmp	epsi30		

epsi32:	jg	epsi30		#overflow
	addl	%edx,%edx	
	orl	%eax,%edx	
	jz	epsi34		

epsi30:	orb	$invop,h_stat	#either +INF or -INF
	call	exproc		
epsi34:	movl	$1,%eax		
	movb	%bl,%cl		
	shll	%cl,%eax	
	ret			

epsi80:	orb	$stacku,h_stat	#stack underflow
	call	exproc		
	jmp	epsi34		

# convert extended floating-point to 64-bit integer
# input:        EP in ecx:edx:eax
# output:       edx:eax = signed integer

	.globl	eptoli
eptoli:				
	testl	$0x00020000,%ecx	#weed out specials
	jnz	epli20		

epli3:	subw	$tBIAS,%cx	#subtract the bias

	cmpw	$63,%cx		#if exponent too large, return INF
	je	epli32		
	jg	epli30		

	subw	$63,%cx		#here shift right
	negw	%cx		
	call	shiftr		

	call	roundi		#do IEEE rounding
	adcl	$0,%eax		
	adcl	$0,%edx		

	orl	%ecx,%ecx	#if negative take 2s complement
	jns	epli11		
	notl	%edx		
	negl	%eax		
	sbbl	$-1,%edx	

epli11:	ret			#return

epli20:	shldl	$16,%ecx,%ebx	#check for stack underflow
	cmpb	$3,%bl		
	jz	epli80		

	orw	%cx,%cx		#specials, but denormals go back
	jz	epli3		
	jmp	epli30		

epli32:	addl	%edx,%edx	#treat 0x80000000 as special case
	orl	%eax,%edx	
	jz	epli34		

epli30:	orb	$invop,h_stat	#either +INF or -INF
	call	exproc		
epli34:	movl	$0x80000000,%edx	
	xorl	%eax,%eax	
	jmp	epli11		

epli80:	orb	$stacku,h_stat	#stack underflow
	call	exproc		
	jmp	epli34		

# convert EP to 10-byte packed-decimal integer
# input:        EP in ecx:edx:eax
#               edi pointer to result area
# output:       signed integer in result area

	.globl	eptopd

eppd20:	shldl	$16,%ecx,%ebx	#check for stack underflow
	cmpb	$3,%bl		
	jz	eppd80		

	orw	%cx,%cx		#specials, but denormals go back
	jz	eppd3		

eppd30:	orb	$invop,h_stat	#either +INF or -INF
	call	exproc		
eppd34:	movl	$0xffff8000,%eax	
	movl	%eax,6(%edi)	
	movl	$6,%ecx		
	repz			
	stosb			
	ret			

eppd80:	orb	$stacku,h_stat	#stack underflow
	call	exproc		
	jmp	eppd34		

eppd33:	subl	$9,%edi		#number too big
	jmp	eppd30		

eptopd:				
	movl	%ecx,6(%edi)	#store the sign

	btl	$17,%ecx	#weed out specials
	jc	eppd20		

eppd3:	subw	$tBIAS+63,%cx	#subtract the bias
	jge	eppd30		
	negw	%cx		
	call	shiftr		

	call	roundi		#do IEEE rounding
	adcl	$0,%eax		
	adcl	$0,%edx		

	movl	%edx,%esi	#get the digits by dividing with 10
	movl	$9,%ecx		
	pushl	%ebp		
	movl	$10,%ebp	

eppd11:	xchgl	%esi,%eax	
	xorl	%edx,%edx	
	divl	%ebp		
	xchgl	%esi,%eax	
	divl	%ebp		
	movb	%dl,%bl		
	xchgl	%esi,%eax	
	xorl	%edx,%edx	
	divl	%ebp		
	xchgl	%esi,%eax	
	divl	%ebp		
	shlb	$4,%dl		
	orb	%bl,%dl		
	movb	%dl,(%edi)	
	incl	%edi		
	loop	eppd11		

	popl	%ebp		
	orl	%eax,%eax	
	jnz	eppd33		
	ret			

# convert extended floating-point to single-precision floating-point
# input:        EP in ecx:edx:eax
# output:       eax = floating-point

	.globl	eptofp

epfp2:	shldl	$16,%ecx,%ebx	#check for stack underflow
	cmpb	$3,%bl		
	jz	epfp38		

	btsl	$16,%ecx	#check zero, denormal
	jc	epfp9		
	orw	%cx,%cx		
	jz	epfp3		
	orl	%edx,%edx	
	jns	epfp36		
	jmp	epfp30		

eptofp:				
	testl	$0x00030000,%ecx	#jump if special value
	jnz	epfp2		

epfp3:	addw	$sBIAS-tBIAS,%cx	#adjust the bias

	cmpw	$0,%cx		#weed out underflow and overflow
	jle	epfp20		
	cmpw	$0x0ff,%cx	
	jnc	epfp32		

epfp6:	addl	%edx,%edx	#shift left to delete hidden bit
				
epfp7:	movl	%edx,%ebx	#set rounding bits
	shll	$23,%ebx	
	addl	$-1,%eax	
	adcl	%ebx,%ebx	
	jz	epfp71		
	movb	$0x80,%bl	
epfp71:	rcrb	$1,%bl		

epfp8:	shldl	$9,%ecx,%eax	#combine sign, exponent, mantissa
	movb	%cl,%al		
	shldl	$23,%edx,%eax	

	orb	%cl,%cl		#set underflow if necessary
	jnz	epfp11		
	testb	$underf,h_ctrl	
	jz	epfp13		
	orb	%bl,%bl		
	jz	epfp11		
epfp13:	orb	$underf,h_stat	
	call	exproc		

epfp11:	call	roundi		#IEEE rounding
	adcl	$0,%eax		
	ret			

epfp9:	addl	%ecx,%ecx	#return signed zero
	rcrl	$1,%eax		
	ret			

epfp20:	negw	%cx		#underflow, we return zero or denormal
	jz	epfp7		
	cmpw	$25,%cx		
	jc	epfp23		
	movw	$25,%cx		
epfp23:	xorl	%ebx,%ebx	
	shrdl	%cl,%edx,%ebx	
	shrl	%cl,%edx	
	xorw	%cx,%cx		
	orl	%ebx,%ebx	
	jz	epfp7		
	orb	$1,%dl		
	jmp	epfp7		

epfp30:	btcl	$31,%edx	#this is NaN or INF
	orl	%edx,%eax	
	jnz	epfp34		
	movw	$2*sBIAS+1,%cx	
	jmp	epfp7		

epfp32:	orb	$overf,h_stat	#this is overflow
	call	exproc		
	movw	$2*sBIAS,%cx	
	movl	$-1,%edx	
	jmp	epfp7		

epfp38:	call	getqn		
	orb	$stacku,h_stat	
	jmp	epfp39		
epfp36:	call	getqn		#this is NaN
	jmp	epfp37		
epfp34:	btl	$30,%edx	
	jc	epfp35		
epfp37:	orb	$invop,h_stat	
epfp39:	call	exproc		
	btsl	$30,%edx	
epfp35:	movw	$2*sBIAS+1,%cx	
	addl	%edx,%edx	
	xorl	%ebx,%ebx	
	jmp	epfp8		

# convert extended floating-point to double-precision floating-point
# input:        EP in ecx:edx:eax
# output:       edx:eax = double-precision

	.globl	eptodp

epdp32:	orb	$overf,h_stat	#this is overflow
	call	exproc		
	movw	$0x7fe,%cx	
	movl	$-1,%edx	
	movl	%edx,%eax	
	jmp	epdp6		

epdp2:	shldl	$16,%ecx,%ebx	#check for stack underflow
	cmpb	$3,%bl		
	jz	epdp38		

	btsl	$16,%ecx	#check zero, denormal
	jc	epdp3		
	orw	%cx,%cx		
	jz	epdp4		
	orl	%edx,%edx	
	jns	epdp36		
	jmp	epdp30		

epdp3:	movl	%ecx,%edx	#return zero
	andl	$0x80000000,%edx	
	ret			

eptodp:				
	testl	$0x00030000,%ecx	#jump if special value
	jnz	epdp2		

epdp4:	addw	$lBIAS-tBIAS,%cx	#adjust the bias
	jle	epdp20		#   weed out underflow and overflow
	cmpw	$0x07ff,%cx	
	jnc	epdp32		

epdp6:	addl	%eax,%eax	#shift left mantissa to delete hidden bit
	adcl	%edx,%edx	

	xorl	%ebx,%ebx	#discarded bits into ebx
	shrdl	$12,%eax,%ebx	

	shrdl	$12,%edx,%eax	#shift right everything 12 bits
	shrdl	$11,%ecx,%edx	
	btl	$31,%ecx	
	rcrl	$1,%edx		

epdp8:	orl	%ebx,%ebx	#exact = no rounding
	jz	epdp19		

	testb	$0x0c,h_ctrl+1	#quick round
	jnz	epdp14		
	btl	$0,%eax		
	adcl	$-1,%ebx	
	jns	epdp17		
epdp18:	#PREEX   precis+rup
	orw	$precis+rup,h_stat	
	testb	$precis,h_ctrl	
	jnz	LL1		
	orw	$0x8080,h_stat	
	movl	$exret,-4(%ebp)	
LL1:				
	addl	$1,%eax		
	adcl	$0,%edx		
	ret			
epdp17:	#PREEXS  precis
	orb	$precis,h_stat	
	testb	$precis,h_ctrl	
	jnz	LL2		
	orw	$0x8080,h_stat	
	movl	$exret,-4(%ebp)	
LL2:				
epdp19:	andb	$0x0fd,h_stat+1	
	ret			

epdp14:	roll	$8,%ebx		#IEEE rounding
	cmpl	$0x0ff,%ebx	
	jna	epdp11		
	orb	$0x40,%bl	
epdp11:	call	roundi		
	adcl	$0,%eax		
	adcl	$0,%edx		
	ret			

epdp20:	negw	%cx		#this is zero or denormal
	jz	epdp7		
	cmpw	$53,%cx		
	jc	epdp21		
	movw	$53,%cx		
epdp21:	shrl	$1,%edx		
	rcrl	$1,%eax		
	jnc	epdp22		
	orl	$1,%eax		
epdp22:	decw	%cx		
	jnz	epdp21		

epdp7:	xorl	%ebx,%ebx	#discarded bits into ebx
	shrdl	$12,%eax,%ebx	

	shrdl	$12,%edx,%eax	#shift right everything 12 bits
	shrdl	$11,%ecx,%edx	
	btl	$31,%ecx	
	rcrl	$1,%edx		

	testb	$underf,h_ctrl	#set underflow if necessary
	jz	epdp13		
	orl	%ebx,%ebx	
	jz	epdp19		
epdp13:	orb	$underf,h_stat	
	call	exproc		
	jmp	epdp8		

epdp30:	movl	%edx,%ebx	#this is INF or NaN
	addl	%ebx,%ebx	
	orl	%eax,%ebx	
	jnz	epdp34		
	movw	$0x7ff,%cx	
	jmp	epdp6		

epdp38:	call	getqn		
	orb	$stacku,h_stat	
	jmp	epdp39		
epdp36:	call	getqn		#this is NaN
	jmp	epdp37		
epdp34:	btl	$30,%edx	
	jc	epdp35		
epdp37:	orb	$invop,h_stat	
epdp39:	call	exproc		
	btsl	$30,%edx	
epdp35:	movw	$2*lBIAS+1,%cx	
	andw	$0xf800,%ax	
	jmp	epdp6		

# round extended floating-point to whole floating-point
# input:        EP in ecx:edx:eax
# output:       EP in ecx:edx:eax no decimals

	.globl	eptown

epwn2:	call	chkarg		#check for stack underflow, NaN
	jc	epwn82		

	btl	$16,%ecx	#check zero, denormal
	jc	epwn82		
	orw	%cx,%cx		
	jz	epwn85		
	cmpw	$1,%cx		
	jz	epwn43		
	orl	%edx,%edx	
	jns	stdnan		

epwn82:	ret			#INF

epwn85:	orw	$denorm+precis,h_stat	
	call	exproc		
epwn43:	orb	$1,%bl		

epwn40:	movb	$0,%bl		#here answer is 0 or 1 or -1
	jnz	epwn41		
	shldl	$8,%edx,%ebx	
	addl	%edx,%edx	
epwn41:	orl	%eax,%edx	
	jz	epwn42		
	orb	$0x40,%bl	
epwn42:	xorl	%edx,%edx	
	xorl	%eax,%eax	
	andl	$0x80000000,%ecx	
	movw	$0x3fff,%cx	
	call	roundi		
	jc	epwn16		
	addl	$0xc001,%ecx	
	ret			

eptown:				
	testl	$0x00030000,%ecx	#jump if special value
	jnz	epwn2		

epwn3:	movl	%ecx,%esi	#number of bits to clear
	movw	$tBIAS+63,%si	
	subw	%cx,%si		
	jle	epwn8		

	cmpw	$64,%si		#over not needed
	jnc	epwn40		

	xchgl	%esi,%ecx	
	xorl	%ebx,%ebx	

	cmpb	$32,%cl		#clear decimal bits
	jc	epwn21		
	xchgl	%ebx,%eax	
	xchgl	%eax,%edx	
	jz	epwn22		
	cmpl	%ebx,%edx	
	rcrl	$1,%ebx		
epwn21:	shrdl	%cl,%eax,%ebx	
	shrdl	%cl,%edx,%eax	
	shrl	%cl,%edx	
epwn22:	roll	$8,%ebx		
	cmpl	$0x100,%ebx	
	jc	epwn23		
	orb	$0x20,%bl	
epwn23:	call	roundi		
	adcl	$0,%eax		
	adcl	$0,%edx		
	cmpb	$32,%cl		
	jc	epwn24		
	xchgl	%eax,%edx	
	shrl	$1,%eax		
	adcl	$0,%esi		
epwn24:	shldl	%cl,%eax,%edx	
	adcl	$0,%esi		
	shll	%cl,%eax	

	xchgl	%esi,%ecx	
				
epwn16:	btsl	$31,%edx	#set high mantissa bit
epwn8:	ret			

# extract exponent and mantissa from extended floating-point
# input:        EP in ecx:edx:eax
# output:       mantissa as EP in ecx:edx:eax
#               [edi] exponent as EP

	.globl	xtract

xtr2:	btl	$16,%ecx	#check zero, denormal
	jc	xtr80		
	orw	%cx,%cx		
	jz	xtr23		
	orl	%edx,%edx	
	jns	xtr92		
	jmp	xtr90		

xtr23:	call	tnormal		#normalize
	jmp	xtr3		

xtract:				
	movl	%ecx,%ebx	#save original sign
	andl	$0x80000000,%ebx	

	testl	$0x00030000,%ecx	#jump if special value
	jnz	xtr2		

xtr3:	pushl	%edx		#save the argument
	pushl	%eax		

	movswl	%cx,%edx	#convert real exponent to EP floating-point
	subl	$tBIAS,%edx	
	call	wiep		
	orw	%cx,%cx		
	jnz	xtr7		
	orl	%ebx,%ecx	
xtr7:	#PUTEP   
	movl	%eax,(%edi)	
	movl	%edx,4(%edi)	
	movl	%ecx,8(%edi)	

	popl	%eax		#restore original argument
	popl	%edx		

	movl	$0x3fff,%ecx	#make exponent 1
	orl	%ebx,%ecx	
	ret			

xtr80:	orb	$zerod,h_stat	#argument was 0, store -INF
	call	exproc		
	movl	%eax,(%edi)	
	movl	$0x80000000,4(%edi)	
	movl	$0x80007fff,8(%edi)	
	ret			

xtr92:	call	getqn		#invalid
	jmp	xtr97		

xtr90:	movl	%edx,%ebx	#either NaN or INF
	addl	%ebx,%ebx	
	orl	%eax,%ebx	
	jnz	xtr95		

	movl	%eax,(%edi)	#argument was INF, store INF
	movl	$0x7fff,8(%edi)	
	movl	$0x80000000,%edx	
	movl	%edx,4(%edi)	
	ret			

xtr95:	btl	$30,%edx	#NaN, make into quiet
	jc	xtr96		
xtr97:	orb	$invop,h_stat	
	call	exproc		
	btsl	$30,%edx	
xtr96:	#PUTEP   
	movl	%eax,(%edi)	
	movl	%edx,4(%edi)	
	movl	%ecx,8(%edi)	
	ret			

# add integer to exponent or EP
# input:        integer EP in ecx:edx:eax
#               [edi] EP
# output:       EP scaled in ecx:edx:eax

	.globl	scale

scaw:	call	retnan		#remove NaNs and invalids
	movb	$0,%bl		

	movl	%ecx,%esi	#set denormal exception if either
	addl	%esi,%esi	#   argument denormal
	cmpl	$0x00040000,%esi	
	jz	sca24		
	movl	8(%edi),%esi	
	addl	%esi,%esi	
	cmpl	$0x00040000,%esi	
	jnz	sca26		
	movb	$1,8(%edi)	

sca24:	orb	$denorm,h_stat	
	call	exproc		

sca26:	testb	$0x01,10(%edi)	#cases x = 0
	jz	sca28		
	cmpl	$0x00027fff,%ecx	
	jz	stdnan		

scazer:	xorl	%edx,%edx	#return zero 
	xorl	%eax,%eax	
	movl	$0x00010000,%ecx	
	jmp	sca17		

sca28:	cmpw	$0x7fff,8(%edi)	#cases x = INF
	jnz	sca30		
	cmpl	$0x80027fff,%ecx	
	jz	stdnan		

	#GETEP                   #return x
	movl	(%edi),%eax	
	movl	4(%edi),%edx	
	movl	8(%edi),%ecx	
	ret			

sca30:	cmpl	$0x80027fff,%ecx	#cases n = INF
	jz	scazer		
	cmpl	$0x00027fff,%ecx	
	jz	sca17		
	jmp	sca4		

sca40:	movl	$-2,%ebx	#this is special cases for integer
	jmp	sca9		
sca44:	xorl	%ebx,%ebx	
	jmp	sca9		

scale:				
	andb	$0x0fd,h_stat+1	

	shldl	$16,%ecx,%ebx	#weed out special arguments
	orb	10(%edi),%bl	
	jnz	scaw		

sca4:	movl	%ecx,%esi	#save the sign

	subw	$tBIAS+15,%cx	#calculate shift count
	ja	sca40		
	negw	%cx		
	cmpw	$16,%cx		
	jnc	sca44		
	addb	$16,%cl		

	shrl	%cl,%edx	#integer -> bx
	movl	%edx,%ebx	

sca9:	#GETEP                   #load the EP
	movl	(%edi),%eax	
	movl	4(%edi),%edx	
	movl	8(%edi),%ecx	

	andl	$0xffff,%ebx	#we need 32-bit integers
	andl	$0xffff,%ecx	

	sarl	$31,%esi	#make twos complement of integer
	xorl	%esi,%ebx	
	subl	%esi,%ebx	

	addl	%ebx,%ecx	#add integer to exponent
	jle	sca70		
	cmpl	$0x7fff,%ecx	
	jge	sca74		

	orl	%edx,%edx	#also check for denormal
	jns	sca78		

	movb	$0,%bl		#no extra bits here

sca17:	roll	$16,%ecx	#put sign in place
	movb	11(%edi),%ch	
	roll	$16,%ecx	
	ret			

sca70:	cmpl	$0xffff8000,%ecx	#renormalization needed
	jge	sca71		
	movw	$0x8000,%cx	
sca71:	clc			
	jmp	sca78		
sca74:	stc			
sca78:	movb	$0,%bl		
	call	renorm		
	jmp	sca17		

itsstu:	orb	$stacku,h_stat	
	jmp	nan3		
itsnan:	btl	$30,%edx	
	jc	nan9		
stdnan:	orb	$invop,h_stat	
nan3:	call	getqn		
	call	exproc		
	btsl	$30,%edx	
nan9:	xorl	%ebx,%ebx	
	ret			

#        subttl  CCALL Arithmetic Operations
#        page

# examine an EP operand
# input:        ecx:edx:eax
# output:       condition bits in al

	.globl	epexam

epexam:				
	xorl	%ebx,%ebx	#first set sign bit
	shldl	$2,%ecx,%ebx	

	testl	$0x00030000,%ecx	#check for specials
	jnz	exa10		

exa8:	orb	$4,%bl		#regular bit

exa9:	movb	%bl,%al		#return bits
	ret			
				
exa10:	btl	$16,%ecx	#check for zero, NaN, unsupported
	jc	exa12		
	orw	%cx,%cx		
	jz	exa14		
	orl	%edx,%edx	
	jns	exa9		

exa20:	orb	$0x01,%bl	#INF or NaN
	addl	%edx,%edx	
	orl	%eax,%edx	
	jnz	exa9		
	jmp	exa8		

exa14:	orb	$0x4,%bl	#denormal case
exa12:	orb	$0x40,%bl	#zero case
	jmp	exa9		

# compare two EP operands
# input:        cx:edx:eax
#               in epcomu quiet NaN will not cause exception
# output:       ST - input -> condition bits in al

	.globl	epcomp, epcomu, xcmp

epcomu:				
	movb	$1,%bl		#unordered entry
	jmp	cmp1		

epcomp:				
	movb	$0,%bl		#Intel entry

cmp1:	movl	8(%edi),%esi	#sign, exponent of B

	shldl	$16,%ecx,%ebx	#weed out special arguments
	orb	10(%edi),%bl	
	jnz	cmpw		

cmp7:	movl	%ecx,%ebx	#set bit in bh if both negative
	andl	%esi,%ebx	
	shrl	$23,%ebx	

	subl	(%edi),%eax	#compare hook line and sinker
	sbbl	4(%edi),%edx	
	sbbl	%esi,%ecx	
	jz	cmp10		
	setnl	%al		

cmp8:	xorb	%bh,%al		#if both negative, complement answer

cmp9:	andb	$0x0b8,h_stat+1	
	orb	%al,h_stat+1	
	ret			

cmp10:	orl	%eax,%edx	#possibly equal
	movb	$1,%al		
	jnz	cmp8		
cmpeq:	movb	$0x40,%al	
	jmp	cmp9		
				
cmpw:	andb	$0x0b8,h_stat+1	
	orb	$0x45,h_stat+1	
	call	retnan		

xcmp:	shldl	$16,%ecx,%ebx	#two zeroes are equal regardless of sign
	andb	10(%edi),%bl	
	testb	$1,%bl		
	jnz	cmpeq		

	movl	%ecx,%ebx	#set denormal exception if either
	addl	%ebx,%ebx	#   argument denormal
	cmpl	$0x00040000,%ebx	
	jz	cmpw2		
	movl	%esi,%ebx	
	addl	%ebx,%ebx	
	cmpl	$0x00040000,%ebx	
	jnz	cmpw4		
cmpw2:	orb	$denorm,h_stat	
	testb	$denorm,h_ctrl	
	jnz	cmpw4		
	orw	$0x8080,h_stat	
	movl	$exret,-4(%ebp)	

cmpw4:	orw	%cx,%cx		#adjust exponent of pseudo-denormal
	jnz	cmpw5		
	orl	%edx,%edx	
	jns	cmpw5		
	incl	%ecx		
cmpw5:	orw	%si,%si		
	jnz	cmpw6		
	testb	$0x80,7(%edi)	
	jz	cmpw6		
	incl	%esi		

cmpw6:	andl	$0x8000ffff,%ecx	
	andl	$0x8000ffff,%esi	
	jmp	cmp7		

# add/subtract from registers to stack
# input:        ecx:edx:eax = EP number
# result:       registers = ST + number

	.globl	epadd,epadd1,epsub,epsubr

epsub:				
	shldl	$16,%ecx,%ebx	#weed out special arguments
	orb	10(%edi),%bl	
	jnz	sub2		
	movl	8(%edi),%esi	
	xorl	$0x80000000,%esi	
	jmp	epadd2		

sub2:	call	retnan		#handle special arguments
	xorb	$0x80,11(%edi)	
	jmp	addw		

epsubr:				
	shldl	$16,%ecx,%ebx	#weed out special arguments
	orb	10(%edi),%bl	
	jnz	subr2		
	xorl	$0x80000000,%ecx	
	jmp	epadd1		

subr2:	call	retnan		#handle special arguments
	xorl	$0x80000000,%ecx	

addw:	testb	$0x02,%bl	#zero is handled normally
	jz	epadd1		

	call	retnan		#first remove NaNs and invalids

	cmpw	$0x7fff,%cx	#weed out A = INF

	jnz	addw3		
	cmpw	%cx,8(%edi)	
	jnz	addrta		
	movl	%ecx,%esi	
	xorl	8(%edi),%esi	
	jns	addrta		
	jmp	stdnan		

addrtb:	#GETEP                   #return B or A as such
	movl	(%edi),%eax	
	movl	4(%edi),%edx	
	movl	8(%edi),%ecx	
addrta:	movb	$0,%bl		
	ret			

addw3:	cmpw	$0x7fff,8(%edi)	#weed out B = INF
	jz	addrtb		

	call	tnormal		#normalize arguments
	xchgl	%eax,(%edi)	
	xchgl	%edx,4(%edi)	
	xchgl	%ecx,8(%edi)	
	call	tnormal		
	jmp	epadd1		

epadd:				
	shldl	$16,%ecx,%ebx	#weed out special arguments
	orb	10(%edi),%bl	
	jnz	addw		

epadd1:	movl	8(%edi),%esi	#the other exponent

epadd2:	cmpw	%si,%cx		#if exponent of A > exponent of B,
	jl	add7		#   we swap arguments
	movl	$0,%ebx		
	je	add34		
	xchgl	%eax,(%edi)	
	xchgl	%edx,4(%edi)	
	xchgl	%ecx,%esi	
add7:	subw	%si,%cx		
	negw	%cx		

	call	shiftr		#shift mantissa right by cl

	movw	%si,%cx		#load the exponent

add34:	andl	$0x8000ffff,%ecx	#clear special bits

	xorl	%ecx,%esi	#go subtract if different signs
	js	add20		

	addl	(%edi),%eax	#add mantissas
	adcl	4(%edi),%edx	
	jnc	add15		
	rcrl	$1,%edx		
	rcrl	$1,%eax		
	rcrb	$1,%bl		
	incl	%ecx		
	cmpw	$0x7fff,%cx	
	je	add19		

add15:	orw	%cx,%cx		#renormalize if needed
	jle	add28		
	ret			

add20:	sbbl	(%edi),%eax	#subtract mantissas
	sbbl	4(%edi),%edx	
	jnc	add24		
	xorl	$0x80000000,%ecx	
	notl	%edx		
	notl	%eax		
	negb	%bl		
	sbbl	$-1,%eax	
	sbbl	$-1,%edx	

add24:	jns	add25		#weed out not-normal, possible all-zero
	orw	%cx,%cx		
	jle	add19		
	ret			
				
add25:	je	add27		#here needs normalizing, or is zero             
add26:	decw	%cx		
	addb	%bl,%bl		
	adcl	%eax,%eax	
	adcl	%edx,%edx	
	jns	add26		
	orw	%cx,%cx		
	jle	add19		
	ret			

add28:	orl	%edx,%edx	#this may be 0+0
	jnz	add19		
	movb	%bl,%dl		
	orl	%eax,%edx	
	jnz	add22		
	btsl	$16,%ecx	
	ret			

add27:	orb	%bl,%dl		#here may be all zero
	orl	%eax,%edx	
	jz	addzer		
add22:	xorl	%edx,%edx	

add19:	call	renorm		#normalize
	ret			

addzer:	movl	$0x00010000,%ecx	#result went all the way to zero
	movb	h_ctrl+1,%bh	
	andb	$0x0c,%bh	
	cmpb	$0x04,%bh	
	jnz	add18		
	btsl	$31,%ecx	
add18:	ret			

# multiply registers to stack
# input:        cx:edx:eax = EP number
# result:       registers = ST * number

	.globl	epmul,epmul1

mulzer:	shrl	$16,%ecx	
	movb	$1,%cl		
	shll	$16,%ecx	
	xorl	%edx,%edx	
	jmp	mulin2		

mulinf:	orl	$0x00027fff,%ecx	#return INF
	movl	$0x80000000,%edx	
mulin2:	xorl	%eax,%eax	
	xorb	%bl,%bl		
	jmp	mul21		

mulw:	call	retnan		#weed out NaN arguments

	movl	%ecx,%esi	#weed out special cases
	addl	8(%edi),%esi	
	btrl	$31,%esi	
	cmpl	$0x37fff,%esi	
	jz	stdnan		
	testb	$1,%bl		
	jnz	mulzer		
	cmpw	$0x7fff,%si	
	jnc	mulinf		
				
	call	tnormal		#normalize arguments if needed
	xchgl	%eax,(%edi)	
	xchgl	%edx,4(%edi)	
	xchgl	%ecx,8(%edi)	
	call	tnormal		
	jmp	epmul1		

epmul:				
	shldl	$16,%ecx,%ebx	#weed out special arguments
	orb	10(%edi),%bl	
	jnz	mulw		

epmul1:	pushl	%ebp		#save exponent, sign, FP stack pointer
	pushl	%ecx		

	movl	%eax,%esi	#save A0123
	movl	%edx,%ecx	

	mull	(%edi)		# A23 * B23
	xchgl	%eax,%esi	
	movl	%edx,%ebx	

	mull	4(%edi)		# A23 * B01
	addl	%eax,%ebx	
	adcl	$0,%edx		
	movl	%edx,%ebp	

	movl	(%edi),%eax	# A01 * B23
	mull	%ecx		
	addl	%eax,%ebx	
	adcl	%edx,%ebp	
	movl	$0,%eax		
	adcl	$0,%eax		
	xchgl	%ecx,%eax	

	mull	4(%edi)		# A01 * B01
	addl	%ebp,%eax	
	adcl	%ecx,%edx	

	popl	%ecx		#exponent

	js	mul42		#possibly normalize
	addl	%ebx,%ebx	
	adcl	%eax,%eax	
	adcl	%edx,%edx	
	decl	%ecx		

mul42:	addl	$-1,%esi	#set rounding bits
	adcl	%ebx,%ebx	
	jz	mul43		
	movb	$0x80,%bl	
mul43:	rcrb	$1,%bl		

	popl	%ebp		#restore base pointer

	subw	$tBIAS-2,%cx	#calculate new exponent
	addw	8(%edi),%cx	
	jo	mul44		
	decw	%cx		
	jle	mul45		

mul21:	roll	$8,%ecx		#put in sign
	xorb	11(%edi),%cl	
	rorl	$8,%ecx		
	ret			#return

mul44:	stc			#here re-normalize
	jmp	mul46		
mul45:	clc			
mul46:	call	renorm		
	jmp	mul21		

# divide registers to stack
# input:        cx:edx:eax = EP number
# result:       registers = ST / number

	.globl	epdiv,epdivr,epdiv1

divw61:	orb	$zerod,h_stat	
	call	exproc		
	jmp	mulinf		

divw:	call	retnan		#weed out NaN arguments

	cmpw	$0x7fff,%cx	#weed out A = INF
	jnz	divw3		
	cmpw	%cx,8(%edi)	
	jz	stdnan		
	jmp	mulinf		

divw3:	cmpw	$0x7fff,8(%edi)	#weed out B = INF
	jz	mulzer		

	btl	$16,%ecx	#if A = 0 and B = 0 return NaN
	jnc	divw7		
	testb	$1,10(%edi)	
	jnz	stdnan		
	jmp	mulzer		

divw7:	testb	$1,10(%edi)	#if B = 0 "zero divide"
	jnz	divw61		

	call	tnormal		#normalize arguments
	xchgl	%eax,(%edi)	
	xchgl	%edx,4(%edi)	
	xchgl	%ecx,8(%edi)	
	call	tnormal		
	xchgl	%eax,(%edi)	
	xchgl	%edx,4(%edi)	
	xchgl	%ecx,8(%edi)	
	jmp	epdiv1		

epdivr:				
	xchgl	(%edi),%eax	
	xchgl	4(%edi),%edx	
	xchgl	8(%edi),%ecx	

epdiv:				
	shldl	$16,%ecx,%ebx	#weed out special arguments
	orb	10(%edi),%bl	
	jnz	divw		

epdiv1:	pushl	%ebp		#save exponent, sign
	pushl	%ecx		

	movl	$1,%ebp		#first bit = 0

	cmpl	4(%edi),%edx	#jump if x < y
	jc	div50		

	subl	0(%edi),%eax	#get first bit
	sbbl	4(%edi),%edx	
	jnc	div49		

	movl	%eax,%edx	#prevent overflow
	xorl	%eax,%eax	
	movl	%eax,%ecx	
	jmp	div31		

div49:	decl	%ebp		#first bit = 1

div50:	divl	4(%edi)		# q = A0123 / B01 -> ebx
	movl	%eax,%ecx	#     r -> esi
	movl	%edx,%esi	

	mull	0(%edi)		# x * B23 -> edx:eax

	xchgl	%edx,%esi	#subtract that from qA23
	negl	%eax		
	sbbl	%esi,%edx	
	jnc	div41		
div31:	decl	%ecx		
	addl	0(%edi),%eax	
	adcl	4(%edi),%edx	
	jnc	div31		

div41:	cmpl	4(%edi),%edx	#prevent overflow
	jnz	div51		
	movl	%eax,%esi	
	xorl	%eax,%eax	
	movl	%eax,%ebx	
	subl	0(%edi),%esi	
	jmp	div32		

div51:	divl	4(%edi)		# x = A0123 / B01 -> bx
	movl	%eax,%ebx	
	movl	%edx,%esi	

	mull	0(%edi)		#then q * B23 -> edx:eax

	negl	%eax		#subtract that from qA23
	sbbl	%edx,%esi	
	jnc	div42		
div32:	decl	%ebx		
	addl	0(%edi),%eax	
	adcl	4(%edi),%esi	
	jnc	div32		

div42:	movl	%esi,%edx	#set rounding bit, first bit flag
	orl	%eax,%edx	
	xchgl	%ebx,%edx	
	jz	div54		
	movb	$0x80,%bl	
	addl	%eax,%eax	
	adcl	%esi,%esi	
	jc	div43		
	subl	0(%edi),%eax	
	sbbl	4(%edi),%esi	
	cmc			
div43:	rcrb	$1,%bl		

div54:	movl	%edx,%eax	#load result to edx:eax
	movl	%ecx,%edx	

	popl	%ecx		#restore registers

	cmpl	$1,%ebp		#handle first bit
	jnc	div57		
	rcrl	$1,%edx		
	rcrl	$1,%eax		
	rcrb	$1,%bl		
	incw	%cx		

div57:	popl	%ebp		

	subw	8(%edi),%cx	#calculate new exponent
	addw	$tBIAS-0,%cx	
	jo	div23		
	decw	%cx		
	jle	div24		

div21:	roll	$8,%ecx		#put in sign
	xorb	11(%edi),%cl	
	rorl	$8,%ecx		
	ret			

div23:	stc			#renormalize if needed
	jmp	div26		
div24:	clc			
div26:	call	renorm		
	jmp	div21		

# partial remainder, Intel version
# input:        cx:edx:eax = EP number
# result:       registers = ST % number

	.globl	eprem

res88:	cmpw	$-1,%cx		#here exponent of A < exponent of B
	jnz	rem83		#   return A unless A > B/2
	pushl	%eax		#   in which case return A-B
	pushl	%edx		
	shll	$1,(%edi)	
	rcll	$1,4(%edi)	
	btrl	$31,%edx	
	jmp	res17		

rem87:	btl	$16,%ebx	#branch if IEEE remainder
	jc	res88		
rem83:	jmp	rem16		

remw:	andb	$0x0fb,h_stat+1	#clear C2
	call	retnan		#remove NaNs and invalids

	andb	$0x0b8,h_stat+1	#clear condition codes

	btl	$16,%ecx	#if B = 0 or A = INF return NaN
	jc	stdnan		
	cmpw	$0x7fff,%si	
	jz	stdnan		

	call	tnormal		#normalize arguments
	xchgl	%eax,(%edi)	
	xchgl	%edx,4(%edi)	
	xchgl	%ecx,%esi	
	cmpw	$0x7fff,%si	
	jz	rem17		
	btl	$16,%ecx	
	jc	rem19		
	call	tnormal		
	jmp	rem3		

eprem:				
	movl	8(%edi),%esi	

	shldl	$16,%ecx,%ebx	#weed out special arguments
	orb	10(%edi),%bl	
	jnz	remw		

	andb	$0x0b8,h_stat+1	#clear condition codes

	xchgl	(%edi),%eax	#swap registers
	xchgl	4(%edi),%edx	
	xchgl	%esi,%ecx	

rem3:	movb	$0,%bl		#initialize quotient

	subw	%si,%cx		#get the shift count
	js	rem87		

	cmpw	$64,%cx		#maximum shift count 64
	jl	rem4		
	addw	%cx,%si		
	orb	$32,%cl		
	andw	$63,%cx		
	subw	%cx,%si		
	orb	$0x04,h_stat+1	
rem4:	movw	%si,8(%edi)	

	pushl	%ebp		#save registers
	movl	(%edi),%ebp	
	movl	4(%edi),%esi	

rem5:	subl	%ebp,%eax	#divide using shift-subtract
	sbbl	%esi,%edx	
	jnc	rem9		
	addl	%ebp,%eax	
	adcl	%esi,%edx	
rem9:	cmc			
rem11:	adcb	%bl,%bl		
	decb	%cl		
	js	rem15		
	addl	%eax,%eax	
	adcl	%edx,%edx	
	jnc	rem5		
	subl	%ebp,%eax	
	sbbl	%esi,%edx	
	jmp	rem11		

rem15:	popl	%ebp		#restore registers

	testb	$0x04,h_stat+1	#if C2 is set (incomplete)
	jnz	rem16		#   leave Q zero

	testl	$0x10000,%ebx	#branch if IEEE remainder
	jnz	res18		

rem41:	andl	$7,%ebx		#massage quotient for the required form
	rorw	$1,%bx		#   bits are discontinuous and reversed
	shrb	$4,%bh		
	rorw	$1,%bx		
	shrb	$2,%bh		
	shlb	$7,%bl		
	shll	$1,%ebx		
	orb	%bh,h_stat+1	

rem16:	movw	8(%edi),%cx	#normalize
rem17:	xorb	%bl,%bl		
	call	renorm		
rem19:	ret			

res18:	pushl	%eax		#save remainder
	pushl	%edx		

res17:	addl	%eax,%eax	#if next quotent bit would be 0, we are
	adcl	%edx,%edx	#   done
	setc	%bh		
	subl	(%edi),%eax	
	sbbl	4(%edi),%edx	
	sbbb	$0,%bh		
	jc	res42		

	movl	%ebx,%esi	#next bit is one, if all the rest are 0,
	andl	$1,%esi		#   and Q is even, we are likewise done
	orl	%edx,%esi	
	orl	%eax,%esi	
	jz	res42		

	incl	%ebx		#here we round up the quotient
	popl	%edx		
	popl	%eax		
	subl	(%edi),%eax	
	sbbl	4(%edi),%edx	
	notl	%edx		
	negl	%eax		
	sbbl	$-1,%edx	
				
	btcl	$31,%ecx	
	jmp	rem41		

res42:	popl	%edx		
	popl	%eax		
	jmp	rem41		

# square-root
# input:        ecx:edx:eax = EP number
# result:       ST = sqrt(number)

	.globl	epsqrt

sqr89:	ret			
sqrw:	xorl	%ebx,%ebx	
	btl	$16,%ecx	#zero returns as such
	jc	sqr89		

	orw	%cx,%cx		#check for unsupported
	jz	sqrw5		
	orl	%edx,%edx	
	jns	stdnan		

	cmpw	$0x7fff,%cx	#check for NaN
	jnz	sqrw5		
	movl	%edx,%ebx	
	addl	%ebx,%ebx	
	orl	%eax,%ebx	
	jnz	nanret		

sqrw5:	orl	%ecx,%ecx	#negative is illegal
	js	stdnan		

	cmpw	$0x7fff,%cx	#+INF returns as such
	jz	sqr89		

	call	tnormal		#normalize
	movl	%ecx,8(%edi)	
	jmp	sqr2		

epsqrt:				
	testl	$0x80030000,%ecx	#jump if special value
	jnz	sqrw		

sqr2:				
	pushl	%ebp		#save registers
	pushl	%edi		

	xorl	%ebx,%ebx	#save x, possibly shifted right by 1
	movl	%eax,%esi	
	andb	$1,%cl		
	shrdl	%cl,%esi,%ebx	
	shrdl	%cl,%edx,%esi	
	pushl	%esi		
	pushl	%ebx		

	incl	%ecx		#load high 30 or 31 bits -> esi:edi
	shrdl	%cl,%edx,%eax	
	shrl	%cl,%edx	
	movl	%edx,%esi	
	movl	%eax,%edi	

	testb	$1,%cl		#get first estimate with a*x+b
	movl	$0x4d12cfe4,%ebx	
	jz	sqr3		
	movl	$0x6cff9300,%ebx	
sqr3:	shldl	$17,%esi,%eax	
	mulw	%bx		
	shrl	$16,%ebx	
	addw	%dx,%bx		

	movl	%esi,%eax	#16-bit iteration y = (x/y + y)/2
	shldl	$16,%esi,%edx	
	divw	%bx		
	shrw	$1,%bx		
	addw	%ax,%bx		
	sbbw	$0,%bx		
	shll	$16,%ebx	

	movl	%esi,%edx	#32-bit iteration y = (x/y + y)/2
	movl	%edi,%eax	
	divl	%ebx		
	shrl	$1,%ebx		
	addl	%eax,%ebx	
	sbbl	$0,%ebx		

	movl	%esi,%edx	#64-bit iteration y = (x/y + y)/2
	movl	%edi,%eax	
	divl	%ebx		
	movl	%eax,%ecx	
	xorl	%eax,%eax	
	divl	%ebx		
	xorl	%edx,%edx	
	shrl	$1,%ebx		
	rcrl	$1,%edx		
	addl	%edx,%eax	
	adcl	%ebx,%ecx	

	movl	%eax,%ebx	#mantissa of y -> ecx:ebx

	mull	%eax		#calculate y^2 -> (none):si:edi:ebp
	movl	%eax,%ebp	
	movl	%edx,%edi	
	movl	%ecx,%eax	
	mull	%eax		
	movl	%eax,%esi	
	movl	%ebx,%eax	
	mull	%ecx		
	addl	%eax,%edi	
	adcl	%edx,%esi	
	addl	%eax,%edi	
	adcl	%edx,%esi	

	movl	%ebx,%eax	#mantissa of y -> ecx:eax

	popl	%ebx		#now y^2 - x into si:edi:ebp
	subl	%ebx,%edi	
	popl	%ebx		
	sbbl	%ebx,%esi	
	js	sqr37		
	jz	sqr35		

sqr31:	subl	$1,%eax		#try out next smallest y 
	sbbl	$0,%ecx		

	stc			#new residue = old - 2y + 1
	sbbl	%eax,%ebp	
	sbbl	%ecx,%edi	
	sbbl	$0,%esi		
	subl	%eax,%ebp	
	sbbl	%ecx,%edi	
	sbbl	$0,%esi		

	jns	sqr31		#more if we are still above real sqrt(x)
	cmc			

sqr32:	sbbb	%bl,%bl		#1/2 bit here, never exactly half
	orb	$0x40,%bl	

sqr33:	movl	%ecx,%edx	

	popl	%edi		#restore registers
	popl	%ebp		
				
	movl	8(%edi),%ecx	#calculate new exponent
	shrw	$1,%cx		
	adcw	$0x1fff,%cx	
	ret			

sqr37:	stc			#try next largest x
	adcl	%eax,%ebp	
	adcl	%ecx,%edi	
	adcl	$0,%esi		
	addl	%eax,%ebp	
	adcl	%ecx,%edi	
	adcl	$0,%esi		

	jns	sqr32		#quit if differential > 0
	addl	$1,%eax		
	adcl	$0,%ecx		
	jmp	sqr37		

sqr35:	movl	%edi,%ebx	#see if we have an exact answer
	orl	%ebp,%ebx	
	jz	sqr33		
	jmp	sqr31		

#__lib   endp

#emuseg  ends

#        end
