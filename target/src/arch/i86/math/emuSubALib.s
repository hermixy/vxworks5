/* Copyright 1991-2001 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,14jul99,tdl  added FUNC / FUNC_LABEL
01b,24oct96,yp   stuck in a # in USSC mailing addr so cpp will work
01a,24jan95,hdn  original US Software version.
*/

#* * * * * * * * * *
#       Filename:   EMUSUB.ASM
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
				
#        extrn   exret:near

#DGROUP  GROUP   hwseg
#CGROUP  GROUP   emuseg

#hwseg   segment RW para public 'DATA'
#        assume  ds:DGROUP, es:DGROUP, ss:DGROUP
#        extrn   h_ctrl:word,h_stat:word
#hwseg   ends

#emuseg  segment public USE32 'CODE'
#        assume  cs:CGROUP

__lib:

# IEEE rounding for integers
# in    bl bit 7 = highest truncated bit
#          bit 6 = OR of all truncated bits
#       ecx bit 31 = sign bit
#       eax = low bits of number
# out   carry set if bump needed

	.globl	roundi
roundi:				
	andb	$0x0fd,h_stat+1	

	cmpb	$0,%bl		#no rounding if already exact
	jz	rndi99		

	movb	h_ctrl+1,%bh	
	andb	$0x0c,%bh	

	jnz	rndi3		#jump if not the usual "to nearest"
	btl	$0,%eax		
	adcb	$-1,%bl		
	jns	rndi9		

rndi8:	#PREEX   precis+rup
	orw	$precis+rup,h_stat	
	testb	$precis,h_ctrl	
	jnz	LL1		
	orw	$0x8080,h_stat	
	movl	$exret,-4(%ebp)	
LL1:				
	stc			
	ret			

rndi9:	#PREEXS  precis
	orb	$precis,h_stat	
	testb	$precis,h_ctrl	
	jnz	LL2		
	orw	$0x8080,h_stat	
	movl	$exret,-4(%ebp)	
LL2:				
rndi99:	ret			

rndi3:	cmpb	$0x0c,%bh	#analyze rounding bits
	jz	rndi9		
	rorl	$11,%ebx	
	xorl	%ecx,%ebx	
	js	rndi9		
	jmp	rndi8		

# IEEE rounding for EP number
# in    bl bit 7 = highest truncated bit
#          bit 6 = OR of all truncated bits
#       ecx:edx:eax = EP number
# out   number rounded

	.globl	rounde,roundx

roundx:				#entry that ignores precision
	andb	$0x0fd,h_stat+1	

	movb	h_ctrl+1,%bh	#get control bits
	jmp	rnde2		

rounde:				
	andb	$0x0fd,h_stat+1	

	movb	h_ctrl+1,%bh	#get control bits, jump
	testb	$1,%bh		#   if not 64-bit precision
	jz	rnde30		

rnde2:	cmpb	$0,%bl		#no rounding if mantissa is exact
	jz	rndx99		

	andb	$0x0c,%bh	#jump if not normal "to nearest"
	jnz	rnde7		
	btl	$0,%eax		
	adcb	$-1,%bl		
	jns	rnde9		

rnde8:	#PREEX   precis+rup      #this bumps the 64-bit mantissa
	orw	$precis+rup,h_stat	
	testb	$precis,h_ctrl	
	jnz	LL3		
	orw	$0x8080,h_stat	
	movl	$exret,-4(%ebp)	
LL3:				
	addl	$1,%eax		
rnde11:	adcl	$0,%edx		
rnde12:	jc	rnde18		
rnde99:	orw	%cx,%cx		
	jnz	rndx99		
	btsl	$16,%ecx	
	movl	%edx,%ebx	
	orl	%eax,%ebx	
	jz	rndx99		
	addl	$0x00010000,%ecx	
rndx99:	#PUTEP
	movl	%eax,(%edi)	
	movl	%edx,4(%edi)	
	movl	%ecx,8(%edi)	
	ret			
rnde18:	rcrl	$1,%edx		
	incl	%ecx		
	cmpw	$0x7fff,%cx	
	jnz	rnde99		
	btsl	$17,%ecx	
	jmp	rnde99		

rnde7:	cmpb	$0x0c,%bh	#analyze rounding option 
	jz	rnde9		
	rorl	$11,%ebx	
	xorl	%ecx,%ebx	
	jns	rnde8		

rnde9:	#PREEXS  precis          #rounding done here
	orb	$precis,h_stat	
	testb	$precis,h_ctrl	
	jnz	LL4		
	orw	$0x8080,h_stat	
	movl	$exret,-4(%ebp)	
LL4:				
	jmp	rndx99		

rnde30:	cmpw	$0x7fff,%cx	#NaN not rounded
	jz	rnde99		

	testb	$2,%bh		#jump if 24-bit precision
	jz	rnde40		

	shrb	$1,%bl		#calculate new rounding bits
	testb	$4,%ah		
	jz	rnde3a		
	orb	$0x80,%bl	
rnde3a:	testl	$0x3ff,%eax	
	jz	rnde3b		
	orb	$0x40,%bl	

rnde3b:	andw	$0xf800,%ax	#clear extra bits for 53-bit form

	cmpb	$0,%bl		#no rounding if already exact
	jz	rnde99		

	testw	$0x7fff,%cx	#we may have delayed masked underflow
	jnz	rnde36		
	orw	$underf+precis,h_stat	

rnde36:	andb	$0x0c,%bh	#rounding logic for 53-bits
	cmpb	$0x0c,%bh	
	jz	rnde9		
	cmpb	$0x00,%bh	
	jnz	rnde37		
	btl	$10,%eax	
	adcb	$-1,%bl		
	js	rnde38		
	jmp	rnde9		
rnde37:	rorl	$11,%ebx	
	xorl	%ecx,%ebx	
	js	rnde9		

rnde38:	#PREEX   precis+rup      #bump the 53-bit mantissa
	orw	$precis+rup,h_stat	
	testb	$precis,h_ctrl	
	jnz	LL5		
	orw	$0x8080,h_stat	
	movl	$exret,-4(%ebp)	
LL5:				
	addl	$0x800,%eax	
	jmp	rnde11		

rnde40:	addb	%dl,%dl		#calculate new rounding bits
	rcrb	$1,%bl		
	orb	%dl,%al		
	orl	%eax,%eax	
	jz	rnde4b		
	orb	$0x40,%bl	

rnde4b:	xorl	%eax,%eax	#clear extra bits for 24-bit form
	movb	$0,%dl		

	cmpb	$0,%bl		#no rounding if already exact
	jz	rnde99		

	testw	$0x7fff,%cx	#we may have delayed masked underflow
	jnz	rnde46		
	orw	$underf+precis,h_stat	

rnde46:	andb	$0x0c,%bh	#rounding logic for 24-bits
	cmpb	$0x0c,%bh	
	jz	rnde9		
	cmpb	$0x00,%bh	
	jnz	rnde47		
	btl	$8,%edx		
	adcb	$-1,%bl		
	js	rnde48		
	jmp	rnde9		
rnde47:	rorl	$11,%ebx	
	xorl	%ecx,%ebx	
	js	rnde9		

rnde48:	#PREEX   precis+rup      #bump the 24-bit mantissa
	orw	$precis+rup,h_stat	
	testb	$precis,h_ctrl	
	jnz	LL6		
	orw	$0x8080,h_stat	
	movl	$exret,-4(%ebp)	
LL6:				
	addl	$0x100,%edx	
	jmp	rnde12		

#multiply mantissa by 10
# in    edx:eax = mantissa
# out   edx:eax multiplied by 10

	.globl	mul10
mul10:				
	movl	%edx,%ebx	#save 1x mantissa
	movl	%eax,%esi	

	shldl	$2,%eax,%edx	#get 4x mantissa
	shll	$2,%eax		

	addl	%esi,%eax	#add 1x to get 5x mantissa
	adcl	%ebx,%edx	
				
	addl	%eax,%eax	#double to get 10x mantissa
	adcl	%edx,%edx	
	ret			

#renormalize unsigned EP number
# in    ecx:edx:eax:bl = exponent, mantissa
# out   ecx:edx:eax:bl standard normal or denormal

	.globl	renorm
renorm:				
	jc	renof		#carry set means overflow

	orw	%cx,%cx		#if exponent <= 0, we go normalize
	jle	ren30		

ren2:	orl	%edx,%edx	#here exponent OK, if mantissa normal
	js	ren8		#   we are done
	jnz	ren5		
	orb	%bl,%dl		
	orl	%eax,%edx	
	jz	ren8b		
	xorl	%edx,%edx	

ren5:	decw	%cx		#if exponent is 1 we drop it to 0
	jz	ren9a		#   else shift mantissa left
	addb	%bl,%bl		
	adcl	%eax,%eax	
	adcl	%edx,%edx	
	jns	ren5		

ren8:	cmpw	$0x7fff,%cx	#assume INF for maximum exponent
	jz	renof		
	ret			
ren8b:	andl	$0x80000000,%ecx	
	btsl	$16,%ecx	
	ret			

ren30a:	cmpw	$-64,%cx	#here exponent had underflowed
	jg	ren32		#   we try to return a denormal
	movw	$-64,%cx	
ren32:	shrl	$1,%edx		
	rcrl	$1,%eax		
	rcrb	$1,%bl		
	jnc	ren32a		
	orb	$0x20,%bl	
ren32a:	incw	%cx		
	jle	ren32		
	andl	$0x80000000,%ecx	
	btsl	$17,%ecx	

ren9:	orb	%bl,%bl		#back to caller
	jz	ren91		
	orw	$underf+precis,h_stat	
ren91:	rorl	$16,%ecx	
	movb	$2,%cl		
	orl	%edx,%edx	
	jnz	ren92		
	orl	%eax,%eax	
	jnz	ren92		
	movb	$1,%cl		
ren92:	rorl	$16,%ecx	
	ret			

ren9a:	testb	$underf,h_ctrl	
	jnz	ren9		

ren30:	testb	$underf,h_ctrl	
	jnz	ren30a		
	addb	$0x60,%ch	
	orw	$0x8080+underf,h_stat	
	movl	$exret,-4(%ebp)	
jren2:	jmp	ren2		
				
renof:	testb	$overf,h_ctrl	
	jnz	reninf		
	orw	$0x8080+overf,h_stat	
	movl	$exret,-4(%ebp)	
	subb	$0x60,%ch	
	jns	jren2		

reninf:	orb	$overf,h_stat	
	andl	$0x80000000,%ecx	
	movw	$0x7ffe,%cx	
	movl	$-1,%edx	
	movl	%edx,%eax	
	movb	%dl,%bl		
	ret			
				
#temporary normalization of EP number
# in    ecx:edx:eax = exponent, mantissa
# out   ecx:edx:eax with number normalized or zero

	.globl	tnormal
tnormal:			
	orl	%edx,%edx	#return if already normal
	js	tno11		#   or if all zero
	jnz	tno5		
	orl	%eax,%edx	
	jz	tno9		
	xorl	%edx,%edx	

tno5:	decw	%cx		#shift left untill high bit = 1
	addl	%eax,%eax	
	adcl	%edx,%edx	
	jns	tno5		
	jmp	tno12		

tno11:	orw	%cx,%cx		#handle pseudo-denormal monsters here
	jnz	tno9		
tno12:	incw	%cx		
	orb	$denorm,h_stat	
	call	exproc		
tno9:	btrl	$17,%ecx	
	ret			

# routine to return answer for a two-argument function when at least
# one argument is either NaN or an unsupported number
# see 387 manual page 3-11
# in    ecx:edx:eax = A
#       [edi]      = B
# out   if NaN present, loads answer into registers,
#       pops the stack, returns to one level up if
#	at least one argument is signalling NaN, set exception

#        extrn   xcmp:near

	.globl	retnan
retnan:				
	pushl	%ebx		#save registers
	pushl	%ecx		

	shldl	$16,%ecx,%ebx	
	cmpb	$3,%bl		
	movl	$0,%ebx		
	jnz	nan3		
	movb	$0x80,%bl	
	orb	$stacku,h_stat	
	call	exproc		
	jmp	nan11		

nan3:	orw	%cx,%cx		#set bl bits 7-6 if A is NaN
	jz	nan11		
	orl	%edx,%edx	
	jns	nan10		
	cmpw	$0x7fff,%cx	
	jnz	nan11		
	movl	%edx,%ecx	
	addl	%ecx,%ecx	
	orl	%eax,%ecx	
	jz	nan11		
	movl	%edx,%ebx	
	shrl	$24,%ebx	
	andb	$0x0c0,%bl	
	xorb	$0x40,%bl	
	orb	$0x80,%bl	
	jmp	nan11		

nan10:	orb	$0x20,%bl	#set bl bit 5 if A is unsupported
				
nan11:	cmpb	$3,10(%edi)	
	jnz	nan13		
	movb	$0x80,%bh	
	orb	$stacku,h_stat	
	call	exproc		
	jmp	nan30		

nan13:	movl	8(%edi),%ecx	#set bh bits 7-6 if B is NaN
	orw	%cx,%cx		
	jz	nan21		
	testb	$0x80,7(%edi)	
	jz	nan20		
	cmpw	$0x7fff,%cx	
	jnz	nan21		
	movl	4(%edi),%ecx	
	addl	%ecx,%ecx	
	orl	(%edi),%ecx	
	jz	nan21		
	movb	7(%edi),%bh	
	andb	$0x0c0,%bh	
	xorb	$0x40,%bh	
	orb	$0x80,%bh	
	jmp	nan21		

nan20:	orb	$0x20,%bh	#set bh bit 5 if A is unsupported
				
nan21:	orw	%bx,%bx		#if no specials, just return
	jnz	nan30		
	popl	%ecx		
	popl	%ebx		
	ret			

nan30:	cmpl	$xcmp,8(%esp)	#comp excepts all nans
	jnz	nan37		
	testb	$1,6(%esp)	
	jz	nan38		

nan37:	movw	%bx,%cx		#if signaling NaN or unsupported present,
	orb	%cl,%ch		#   set exception
	testb	$0x60,%ch	
	jz	nan31		
nan38:	orb	$invop,h_stat	
	call	exproc		

nan31:	movw	%bx,%cx		#if unsupported present, return standard NaN
	orb	%cl,%ch		
	testb	$0x20,%ch	
	jnz	nanrts		

	movl	%ebx,%ecx	#if just one argument is NaN,
	xorb	%cl,%ch		#   we return that made quiet
	jns	nan40		
	testb	$0x80,%bl	
	jnz	nanrta		
	jmp	nanrtb		

nan40:	testb	$0x40,%ch	#both are NaN, if just one is quiet,
	jz	nan50		#   we return that one
	testb	$0x40,%bl	
	jz	nanrta		
	jmp	nanrtb		

nan50:	pushl	%edx		#we have two quiet NaNs or two 
	pushl	%eax		#   signaling NaNs, we return the
				#   one with the larger mantissa 
				#   (this is a lot of trouble with dubious
	subl	(%edi),%eax	#   use, but that is what the book says)
	sbbl	4(%edi),%edx	
	popl	%eax		
	popl	%edx		
	jc	nanrtb		
				
nanrta:	btsl	$30,%edx	#this returns A as quiet
	popl	%ecx		
	jmp	nan99		

nanrts:	call	getqn		#this returns standard NaN
	jmp	nan98		

nanrtb:	#GETEP                   #this returns B as quiet
	movl	(%edi),%eax	
	movl	4(%edi),%edx	
	movl	8(%edi),%ecx	
	btsl	$30,%edx	
nan98:	addl	$4,%esp		

nan99:	addl	$4+ADDSIZ,%esp	#return skipping a level
	xorl	%ebx,%ebx	
	ret			

# routine to eliminate illegals from single argument

	.globl	chkarg
chkarg:				
	btl	$16,%ecx	
	jc	chk27		
	orw	%cx,%cx		
	jz	chk11		
	orl	%edx,%edx	
	jns	chk41		
	cmpw	$0x7fff,%cx	
	jz	chk21		
chk9:	clc			
	ret			
chk11:	orb	$denorm,h_stat	
	call	exproc		
	orl	%edx,%edx	
	jns	chk9		
	incl	%ecx		
	ret			
chk21:	movl	%edx,%ebx	
	addl	%ebx,%ebx	
	orl	%eax,%ebx	
	jz	chk9		
	btsl	$30,%edx	
	jnc	chkinv		
	ret			
chk27:	btl	$17,%ecx	
	jnc	chk9		
chk51:	call	getqn		
	orb	$stacku,h_stat	
	jmp	chk53		
chk41:	call	getqn		
chkinv:	orb	$invop,h_stat	
chk53:	call	exproc		
	stc			
	ret			

# shift the works right by cl bits
# in    EP mantissa in edx:eax
# out   EP mantissa shifted right by cl in edx:eax:bl (bl bit 6 sticky bit)

	.globl	shiftr
shiftr:				
	xorl	%ebx,%ebx	
	cmpw	$32,%cx		
	jnc	shf11		

shf3:	shrdl	%cl,%eax,%ebx	
	shrdl	%cl,%edx,%eax	
	shrl	%cl,%edx	

shf4:	addl	%ebx,%ebx	
	jz	shf8		
	movb	$0x80,%bl	
shf8:	rcrb	$1,%bl		
	ret			
				
shf11:	xchgl	%eax,%ebx	
	xchgl	%edx,%eax	
	subw	$32,%cx		
	jz	shf4		

shf12:	cmpl	$1,%ebx		
	cmc			
	rcrl	$1,%ebx		
	cmpw	$32,%cx		
	jc	shf3		

	shrl	$1,%ebx		
	orl	%eax,%ebx	
	xorl	%eax,%eax	
	subw	$32,%cx		
	jz	shf4		
	movw	$1,%cx		
	jmp	shf12		

# return standard NaN in registers

	.globl	getqn
getqn:				
	movl	$0x80027fff,%ecx	
	movl	$0xc0000000,%edx	
	xorl	%eax,%eax	
	movb	%al,%bl		
	ret			

# routine to process exception

	.globl	exproc, exprop
exproc:				
	pushl	%eax		
	movw	h_ctrl,%ax	
	notl	%eax		
	andw	h_stat,%ax	
	testb	$0x3f,%al	
	jz	xpp9		
	orw	$0x8080,h_stat	
	movl	%ebp,%esp	
	jmp	exret		

exprop:				
	pushl	%eax		
	movw	h_ctrl,%ax	
	notl	%eax		
	andw	h_stat,%ax	
	testb	$0x3f,%al	
	jz	xpp9		
	orw	$0x8080,h_stat	
	movl	$exret,-4(%ebp)	
xpp9:	popl	%eax		
	ret			

#__lib   endp

#emuseg  ends

#        end
