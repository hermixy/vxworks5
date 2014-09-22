/* Copyright 1991-2001 Wind River Systems, Inc. */

/*
modification history
--------------------
01f,28aug01,hdn  replaced .align with .balign
		 replaced FUNC with GTEXT/GDATA macros for global symbols.
		 added FUNC / FUNC_LABEL	
		 assembly syntax has become more stringent for
		 indirect call instructions. '*' prefix now needed.
01e,12feb97,hdn  fixed a bug in fxchs. (SPR#7920)
01d,24oct96,yp   stuck in a # in USSC mailing addr so cpp will work 
01c,26sep95,hdn  implemented USSW's latest emumain.s.
01b,06sep95,hdn  changed .comm to .lcomm to make them contiguous.
01a,24jan95,hdn  original US Software version.
*/

#* * * * * * * * * *
#       Filename:   EMUMAIN.ASM
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
#       Version:        See VSNLOG.TXT
#       Released:       1 March 1991
#
#* * * * * * * * * *

#define _ASMLANGUAGE
#include "vxWorks.h"
#include "asm.h"

	.data
	.globl	FUNC(copyright_wind_river)
	.long	FUNC(copyright_wind_river)


	.include	"emuIncALib.s"
	.text

	.globl	emuret,exret
#        extrn   witoep:near,eptowi:near,sitoep:near,eptosi:near
#        extrn   litoep:near,eptoli:near,pdtoep:near,eptopd:near
#        extrn   fptoep:near,eptofp:near,dptoep:near,eptodp:near
#        extrn   eptoep:near
#        extrn   getqn:near
#        extrn   epadd:near,epsub:near,epsubr:near,epmul:near
#        extrn   epdiv:near,epdivr:near,eprem:near
#        extrn   epcomp:near,epcomu:near,epexam:near,eptown:near,epsqrt:near
#        extrn   rounde:near,roundx:near,exproc:near
#        extrn   xtract:near,scale:near
#        extrn   epf2xm1:near,epsin:near,epcos:near,eptan:near
#        extrn   eplog:near,eplog1:near,epatan:near
#        extrn   reduct:near
#        extrn   exir_no:abs
#        extrn   ENIRS:abs

#DGROUP  GROUP   hwseg
#CGROUP  GROUP   emuseg

#hwseg   segment RW page public 'DATA'
#        assume  ds:DGROUP, es:DGROUP, ss:DGROUP
	.globl	h_ctrl,h_stat,s_iptr,s_memp
	.lcomm	s_stk,256
	.lcomm	s_fpsp,4
	.lcomm	s_iptr,4
	.lcomm	s_memp,4
	.lcomm	h_ctrl,4
	.lcomm	h_stat,4
	.lcomm	h_tag,4
#hwseg   ends
				
#emuseg  segment public USE32 'CODE'
#        assume  cs:CGROUP

# address tables for floating-point instructions

instab:	.long	faddfp,fmulfp,fcomfp,fcompfp,fsubfp,fsubrfp,fdivfp,fdivrfp
	.long	fldfp,NONE_,fstf,fstpf,fldenv_,fldcwc,fstenv_,fstcwc
	.long	faddsi,fmulsi,fcomsi,fcompsi,fsubsi,fsubrsi,fdivsi,fdivrsi
	.long	fldsi,NONE_,fisti,fistpi,NONE_,flde,NONE_,fstpe
	.long	fadddp,fmuldp,fcomdp,fcompdp,fsubdp,fsubrdp,fdivdp,fdivrdp
	.long	flddp,NONE_,fstd,fstpd,frstor_,NONE_,fsave_,fstsw_
	.long	faddwi,fmulwi,fcomwi,fcompwi,fsubwi,fsubrwi,fdivwi,fdivrwi
	.long	fldwi,NONE_,fistw,fistpw,fbldp,fildl,fbstpp,fistpl

instas:	.long	fadds,fmuls,fcoms,fcomps,fsubs,fsubrs,fdivs,fdivrs
	.long	flds,fxchs,fnop_,fstps,is1,is8,is2,is3
	.long	NONE_,NONE_,NONE_,NONE_,NONE_,fucompps,NONE_,NONE_
	.long	NONE_,NONE_,NONE_,NONE_,is5,NONE_,NONE_,NONE_
	.long	faddx,fmulx,NONE_,NONE_,fsubx,fsubrx,fdivx,fdivrx
	.long	ffree_,NONE_,fsts,fstps,fucoms,fucomps,NONE_,NONE_
	.long	faddpx,fmulpx,NONE_,fcompps,fsubpx,fsubrpx,fdivrpx,fdivpx
	.long	NONE_,NONE_,NONE_,NONE_,fstswax_,NONE_,NONE_,NONE_

	.balign 16,0x90
	.globl	GTEXT(emuInit)
FUNC_LABEL(emuInit)
	fninit
	ret

	.balign 16,0x90
	.globl	GTEXT(emu387)
FUNC_LABEL(emu387)
#       test    byte ptr [esp+9],2 ;enable interrupts, unless they were
	.word	ENIRS
	.byte	0x24,0x09,0x02
	jz	emu3		#   disabled initially
	sti			

emu3:	pusha			#save all general registers

	subl	$s_len,%esp	#reserve a frame on the stack
	movl	%esp,%ebp	
	cld			

	addl	$12,s_sp(%ebp)	

	movl	s_ip(%ebp),%esi	#address of FP instruction -> esi

emu86:				
	.byte	0x2e #from cs: but assembler seems defective
	lodsw			#pick up the instruction
	cmpb	$0x0d8,%al	
	jc	emu84		

emu11:	movl	%eax,%edx	#build 6-bit instruction index -> edx
	shlb	$2,%dh		
	rolw	$3,%dx		
	andl	$0x3f,%edx	

	movb	%ah,%bl		#build R/M code
	andl	$7,%ebx		#   and leave in bl for the following

	cmpb	$0x0c0,%ah	#MOD 3 uses no memory addressing
	jnc	emu50		

	xorl	%edi,%edi	#handle possible s-i-b
	cmpb	$4,%bl		
	jnz	emu26		
	.byte	0x2e #from cs: 
	lodsb			
	movb	%al,%bl		
	andb	$7,%bl		
	shldl	$31,%eax,%edi	
	andl	$7*4,%edi	
	subl	$4*4,%edi	
	jz	emu26		
	movb	%al,%cl		#scaled index
	shrb	$6,%cl		
	negl	%edi		
	movl	s_di+28-16(%ebp,%edi),%edi	
	shll	%cl,%edi	

emu26:	addb	%ah,%ah		#take displacement -> ax
	js	emu24		#   MOD 01 reg+d8
	jc	emu25		#   MOD 10 reg+d32
	cmpb	$5,%bl		#   MOD 00 reg; reg5 = d32
	jnz	emu27		
	.byte	0x2e #from cs:
	lodsl			
	jmp	emu42		
emu27:	xorl	%eax,%eax	
	jmp	emu29		
emu25:				
	.byte	0x2e #from cs:
	lodsl			
	jmp	emu29		
emu24:				
	.byte	0x2e #from cs: 
	lodsb			
	movsbl	%al,%eax	

emu29:	negl	%ebx		#add the base register
	addl	s_di+28(%ebp,%ebx,4),%eax	

emu42:	addl	%edi,%eax	#possible scaled index

	xchgl	%esi,s_ip(%ebp)	#update return address

	call	*%cs:instab(,%edx,4)	#call instruction handler

emu51:	movl	s_ip(%ebp),%esi	#if next instruction is ESC, repeat
	.byte	0x2e #from cs: but assembler seems defective
	lodsw			
	xorb	$0x0d8,%al	
	cmpb	$8,%al		
	jc	emu11		

	addl	$s_len,%esp	#free the stack frame

	popa			#restore registers
emuret:	iret			

emu50:	xchgl	%esi,s_ip(%ebp)	#update return address
	call	*%cs:instas(,%edx,4)	#call instruction handler
	jmp	emu51		

exret:	addl	$s_len,%esp	#exception return

	popa			
	.byte	0x0cd,EXIRNO	#INT exception interrupt
	iret			

emu84:	decl	%esi		#illegal prefix, skip
	jmp	emu86		

#emu387  endp

intrn:

flddp:				#push from memory
	andb	$0x0fd,h_stat+1	
	call	dptoep		
	#STPUSHP
	movl	s_fpsp,%ebx	
	subb	$32,%bl		
	cmpb	$3,s_stk+10(%ebx)	
	jnz	stackof		
	movl	%ebx,s_fpsp	
	addl	$s_stk,%ebx	
	movl	%eax,(%ebx)	
	movl	%edx,4(%ebx)	
	movl	%ecx,8(%ebx)	
	ret			
fstd:				#fetch as double-precision
	#SAVEIX
	movl	%eax,s_memp	
	xchgl	%esi,%eax	
	movl	%eax,s_iptr	
	#STTOP   
	movl	s_fpsp,%edi	
	addl	$s_stk,%edi	
	#GETEP   
	movl	(%edi),%eax	
	movl	4(%edi),%edx	
	movl	8(%edi),%ecx	
	call	eptodp		
	movl	%eax,(%esi)	
	movl	%edx,4(%esi)	
	ret			
fstpd:				#fetch as double-precision, pop
	#SAVEIX
	movl	%eax,s_memp	
	xchgl	%esi,%eax	
	movl	%eax,s_iptr	
	#STTOP   
	movl	s_fpsp,%edi	
	addl	$s_stk,%edi	
	#GETEP   
	movl	(%edi),%eax	
	movl	4(%edi),%edx	
	movl	8(%edi),%ecx	
	call	eptodp		
	movl	%eax,(%esi)	
	movl	%edx,4(%esi)	
	#STPOPq
	movb	$3,10(%edi)	
	addb	$32,s_fpsp	
	ret			
fadddp:				#ST = ST + memory
	call	dptoep		
	#STTOP   
	movl	s_fpsp,%edi	
	addl	$s_stk,%edi	
	call	epadd		
	jmp	rounde		
fsubdp:				#ST = ST - memory
	call	dptoep		
	#STTOP   
	movl	s_fpsp,%edi	
	addl	$s_stk,%edi	
	call	epsubr		
	jmp	rounde		
fmuldp:				#ST = ST * memory
	call	dptoep		
	#STTOP   
	movl	s_fpsp,%edi	
	addl	$s_stk,%edi	
	call	epmul		
	jmp	rounde		
fdivdp:				#ST = ST / memory
	call	dptoep		
	#STTOP   
	movl	s_fpsp,%edi	
	addl	$s_stk,%edi	
	call	epdivr		
	jmp	rounde		
fsubrdp:			#ST = memory - ST
	call	dptoep		
	#STTOP   
	movl	s_fpsp,%edi	
	addl	$s_stk,%edi	
	call	epsub		
	jmp	rounde		
fdivrdp:			#ST = memory / ST
	call	dptoep		
	#STTOP   
	movl	s_fpsp,%edi	
	addl	$s_stk,%edi	
	call	epdiv		
	jmp	rounde		
fsqrt_:				#square-root of ST
	#STTOP   
	movl	s_fpsp,%edi	
	addl	$s_stk,%edi	
	#GETEP   
	movl	(%edi),%eax	
	movl	4(%edi),%edx	
	movl	8(%edi),%ecx	
	call	epsqrt		
	jmp	rounde		
fcomdp:				#ST - memory -> codes
	orb	$0x45,h_stat+1	
	call	dptoep		
	#STTOP   
	movl	s_fpsp,%edi	
	addl	$s_stk,%edi	
	jmp	epcomp		
fcompdp:			#ST - memory -> codes, pop
	orb	$0x45,h_stat+1	
	call	dptoep		
	#STTOP   
	movl	s_fpsp,%edi	
	addl	$s_stk,%edi	
	call	epcomp		
	#STPOPq
	movb	$3,10(%edi)	
	addb	$32,s_fpsp	
	ret			
faddfp:				#ST = ST + memory
	call	fptoep		
	#STTOP   
	movl	s_fpsp,%edi	
	addl	$s_stk,%edi	
	call	epadd		
	jmp	rounde		
faddwi:				#ST = ST + memory
	call	witoep		
	jmp	.+7		
faddsi:				#ST = ST + memory
	call	sitoep		
	#STTOP   
	movl	s_fpsp,%edi	
	addl	$s_stk,%edi	
	call	epadd		
	jmp	rounde		
fmulfp:				#ST = ST * memory
	call	fptoep		
	#STTOP   
	movl	s_fpsp,%edi	
	addl	$s_stk,%edi	
	call	epmul		
	jmp	rounde		
fmulwi:				#ST = ST * memory
	call	witoep		
	jmp	.+7		
fmulsi:				#ST = ST * memory
	call	sitoep		
	#STTOP   
	movl	s_fpsp,%edi	
	addl	$s_stk,%edi	
	call	epmul		
	jmp	rounde		
fcomfp:				#ST - memory -> codes
	orb	$0x45,h_stat+1	
	call	fptoep		
	#STTOP   
	movl	s_fpsp,%edi	
	addl	$s_stk,%edi	
	jmp	epcomp		
fcomsi:				#ST - memory -> codes
	call	sitoep		
	#STTOP   
	movl	s_fpsp,%edi	
	addl	$s_stk,%edi	
	jmp	epcomp		
fcomwi:				#ST - memory -> codes
	call	witoep		
	#STTOP   
	movl	s_fpsp,%edi	
	addl	$s_stk,%edi	
	jmp	epcomp		
fcompfp:			#ST - memory -> codes, pop
	orb	$0x45,h_stat+1	
	call	fptoep		
	#STTOP   
	movl	s_fpsp,%edi	
	addl	$s_stk,%edi	
	call	epcomp		
	#STPOPq
	movb	$3,10(%edi)	
	addb	$32,s_fpsp	
	ret			
fcompwi:			#ST - memory -> codes, pop
	call	witoep		
	jmp	.+7		
fcompsi:			#ST - memory -> codes, pop
	call	sitoep		
	#STTOP   
	movl	s_fpsp,%edi	
	addl	$s_stk,%edi	
	call	epcomp		
	#STPOPq
	movb	$3,10(%edi)	
	addb	$32,s_fpsp	
	ret			
fsubfp:				#ST = ST - memory
	call	fptoep		
	#STTOP   
	movl	s_fpsp,%edi	
	addl	$s_stk,%edi	
	call	epsubr		
	jmp	rounde		
fsubwi:				#ST = ST - memory
	call	witoep		
	jmp	.+7		
fsubsi:				#ST = ST - memory
	call	sitoep		
	#STTOP   
	movl	s_fpsp,%edi	
	addl	$s_stk,%edi	
	call	epsubr		
	jmp	rounde		
fsubrfp:			#ST = memory - ST
	call	fptoep		
	#STTOP   
	movl	s_fpsp,%edi	
	addl	$s_stk,%edi	
	call	epsub		
	jmp	rounde		
fsubrwi:			#ST = memory - ST
	call	witoep		
	jmp	.+7		
fsubrsi:			#ST = memory - ST
	call	sitoep		
	#STTOP   
	movl	s_fpsp,%edi	
	addl	$s_stk,%edi	
	call	epsub		
	jmp	rounde		
fdivfp:				#ST = ST / memory
	call	fptoep		
	#STTOP   
	movl	s_fpsp,%edi	
	addl	$s_stk,%edi	
	call	epdivr		
	jmp	rounde		
fdivwi:				#ST = ST / memory
	call	witoep		
	jmp	.+7		
fdivsi:				#ST = ST / memory
	call	sitoep		
	#STTOP   
	movl	s_fpsp,%edi	
	addl	$s_stk,%edi	
	call	epdivr		
	jmp	rounde		
fdivrfp:			#ST = memory / ST
	call	fptoep		
	#STTOP   
	movl	s_fpsp,%edi	
	addl	$s_stk,%edi	
	call	epdiv		
	jmp	rounde		
fdivrwi:			#ST = memory / ST
	call	witoep		
	jmp	.+7		
fdivrsi:			#ST = memory / ST
	call	sitoep		
	#STTOP   
	movl	s_fpsp,%edi	
	addl	$s_stk,%edi	
	call	epdiv		
	jmp	rounde		
fadds:				#ST = ST + ST(i)
	#SAVEIN
	movl	%esi,s_iptr	
	#GETEPS  ebx
	movl	s_fpsp,%eax	
	movl	%eax,%edi	
	addl	$s_stk,%edi	
	shll	$5,%ebx		
	addb	%bl,%al		
	addl	$s_stk,%eax	
	movl	8(%eax),%ecx	
	movl	4(%eax),%edx	
	movl	(%eax),%eax	
	call	epadd		
	jmp	rounde		
fmuls:				#ST = ST * ST(i)
	#SAVEIN
	movl	%esi,s_iptr	
	#GETEPS  ebx
	movl	s_fpsp,%eax	
	movl	%eax,%edi	
	addl	$s_stk,%edi	
	shll	$5,%ebx		
	addb	%bl,%al		
	addl	$s_stk,%eax	
	movl	8(%eax),%ecx	
	movl	4(%eax),%edx	
	movl	(%eax),%eax	
	call	epmul		
	jmp	rounde		
fcoms:				#ST - ST(i) -> codes
	#SAVEIN
	movl	%esi,s_iptr	
	#GETEPS  ebx
	movl	s_fpsp,%eax	
	movl	%eax,%edi	
	addl	$s_stk,%edi	
	shll	$5,%ebx		
	addb	%bl,%al		
	addl	$s_stk,%eax	
	movl	8(%eax),%ecx	
	movl	4(%eax),%edx	
	movl	(%eax),%eax	
	jmp	epcomp		
fcomps:				#ST - ST(i) -> codes, pop
	#SAVEIN
	movl	%esi,s_iptr	
	#GETEPS  ebx
	movl	s_fpsp,%eax	
	movl	%eax,%edi	
	addl	$s_stk,%edi	
	shll	$5,%ebx		
	addb	%bl,%al		
	addl	$s_stk,%eax	
	movl	8(%eax),%ecx	
	movl	4(%eax),%edx	
	movl	(%eax),%eax	
	call	epcomp		
	#STPOPq
	movb	$3,10(%edi)	
	addb	$32,s_fpsp	
	ret			
fsubs:				#ST = ST - ST(i)
	#SAVEIN
	movl	%esi,s_iptr	
	#GETEPS  ebx
	movl	s_fpsp,%eax	
	movl	%eax,%edi	
	addl	$s_stk,%edi	
	shll	$5,%ebx		
	addb	%bl,%al		
	addl	$s_stk,%eax	
	movl	8(%eax),%ecx	
	movl	4(%eax),%edx	
	movl	(%eax),%eax	
	call	epsubr		
	jmp	rounde		
fsubrs:				#ST = ST(i) - ST
	#SAVEIN
	movl	%esi,s_iptr	
	#GETEPS  ebx
	movl	s_fpsp,%eax	
	movl	%eax,%edi	
	addl	$s_stk,%edi	
	shll	$5,%ebx		
	addb	%bl,%al		
	addl	$s_stk,%eax	
	movl	8(%eax),%ecx	
	movl	4(%eax),%edx	
	movl	(%eax),%eax	
	call	epsub		
	jmp	rounde		
fdivs:				#ST = ST / ST(i)
	#SAVEIN
	movl	%esi,s_iptr	
	#GETEPS  ebx
	movl	s_fpsp,%eax	
	movl	%eax,%edi	
	addl	$s_stk,%edi	
	shll	$5,%ebx		
	addb	%bl,%al		
	addl	$s_stk,%eax	
	movl	8(%eax),%ecx	
	movl	4(%eax),%edx	
	movl	(%eax),%eax	
	call	epdivr		
	jmp	rounde		
fdivrs:				#ST = ST(i) / ST
	#SAVEIN
	movl	%esi,s_iptr	
	#GETEPS  ebx
	movl	s_fpsp,%eax	
	movl	%eax,%edi	
	addl	$s_stk,%edi	
	shll	$5,%ebx		
	addb	%bl,%al		
	addl	$s_stk,%eax	
	movl	8(%eax),%ecx	
	movl	4(%eax),%edx	
	movl	(%eax),%eax	
	call	epdiv		
	jmp	rounde		
fldfp:				#push from memory
	andb	$0x0fd,h_stat+1	
	call	fptoep		
	#STPUSHP
	movl	s_fpsp,%ebx	
	subb	$32,%bl		
	cmpb	$3,s_stk+10(%ebx)	
	jnz	stackof		
	movl	%ebx,s_fpsp	
	addl	$s_stk,%ebx	
	movl	%eax,(%ebx)	
	movl	%edx,4(%ebx)	
	movl	%ecx,8(%ebx)	
	ret			
fldsi:				#push from memory
	andb	$0x0fd,h_stat+1	
	call	sitoep		
	#STPUSHP
	movl	s_fpsp,%ebx	
	subb	$32,%bl		
	cmpb	$3,s_stk+10(%ebx)	
	jnz	stackof		
	movl	%ebx,s_fpsp	
	addl	$s_stk,%ebx	
	movl	%eax,(%ebx)	
	movl	%edx,4(%ebx)	
	movl	%ecx,8(%ebx)	
	ret			
fldwi:				#push from memory
	andb	$0x0fd,h_stat+1	
	call	witoep		
	#STPUSHP
	movl	s_fpsp,%ebx	
	subb	$32,%bl		
	cmpb	$3,s_stk+10(%ebx)	
	jnz	stackof		
	movl	%ebx,s_fpsp	
	addl	$s_stk,%ebx	
	movl	%eax,(%ebx)	
	movl	%edx,4(%ebx)	
	movl	%ecx,8(%ebx)	
	ret			
fstf:				#fetch as single-precision
	#SAVEIX
	movl	%eax,s_memp	
	xchgl	%esi,%eax	
	movl	%eax,s_iptr	
	#STTOP   
	movl	s_fpsp,%edi	
	addl	$s_stk,%edi	
	#GETEP   
	movl	(%edi),%eax	
	movl	4(%edi),%edx	
	movl	8(%edi),%ecx	
	call	eptofp		
	movl	%eax,(%esi)	
	ret			
fstpf:				#pop as single-precision 
	#SAVEIX
	movl	%eax,s_memp	
	xchgl	%esi,%eax	
	movl	%eax,s_iptr	
	#STTOP   
	movl	s_fpsp,%edi	
	addl	$s_stk,%edi	
	#GETEP   
	movl	(%edi),%eax	
	movl	4(%edi),%edx	
	movl	8(%edi),%ecx	
	call	eptofp		
	movl	%eax,(%esi)	
	#STPOPq
	movb	$3,10(%edi)	
	addb	$32,s_fpsp	
	ret			
fldcwc:				#load control word
	movw	(%eax),%ax	
	andw	$0x1f7f,%ax	
	orb	$0x40,%al	
	movw	%ax,h_ctrl	
	ret			
fstcwc:				#store control word
	movl	%eax,%edi	
	movw	h_ctrl,%ax	
	stosw			
	ret			
flds:				#ST = ST(i)
	#SAVEIN
	movl	%esi,s_iptr	
	andb	$0x0fd,h_stat+1	
	#STIX    ebx
	movl	s_fpsp,%eax	
	shll	$5,%ebx		
	addb	%bl,%al		
	addl	$s_stk,%eax	
	xchgl	%edi,%eax	
	#GETEP   
	movl	(%edi),%eax	
	movl	4(%edi),%edx	
	movl	8(%edi),%ecx	
	#STPUSHP
	movl	s_fpsp,%ebx	
	subb	$32,%bl		
	cmpb	$3,s_stk+10(%ebx)	
	jnz	stackof		
	movl	%ebx,s_fpsp	
	addl	$s_stk,%ebx	
	movl	%eax,(%ebx)	
	movl	%edx,4(%ebx)	
	movl	%ecx,8(%ebx)	
	ret			
fxchs:				#exchange ST and ST(i)
	#SAVEIN
	movl	%esi,s_iptr	
	andb	$0x0fd,h_stat+1	
	movl	s_fpsp,%eax	
	addl	$s_stk,%eax	
	movl	(%eax),%edi	
	movl	4(%eax),%edx	
	movl	8(%eax),%ecx	
	subl	$s_stk,%eax	
	shll	$5,%ebx
	addb	%bl,%al
	addl	$s_stk,%eax	
	xchgl	%edi,(%eax)	
	xchgl	%edx,4(%eax)	
	xchgl	%ecx,8(%eax)	
	subl	$s_stk,%eax	
	subb	%bl,%al		
	addl	$s_stk,%eax	
	movl	%edi,(%eax)	
	movl	%edx,4(%eax)	
	movl	%ecx,8(%eax)	
	ret			
fnop_:				#no operation
	#SAVEIN
	movl	%esi,s_iptr	
	ret			
fstps:				#ST(i) = ST and pop
	#SAVEIN
	movl	%esi,s_iptr	
	andb	$0x0fd,h_stat+1	
	movl	s_fpsp,%eax	
	addl	$s_stk,%eax	
	movl	%eax,%edi	
	movl	(%eax),%esi	
	movl	4(%eax),%edx	
	movl	8(%eax),%ecx	
	shll	$5,%ebx		
	subl	$s_stk,%eax	
	addb	%bl,%al		
	addl	$s_stk,%eax	
	movl	%esi,(%eax)	
	movl	%edx,4(%eax)	
	movl	%ecx,8(%eax)	
	#STPOPq
	movb	$3,10(%edi)	
	addb	$32,s_fpsp	
	ret			
fucoms:				#ST - ST(i) -> codes
	#SAVEIN
	movl	%esi,s_iptr	
	#GETEPS  ebx
	movl	s_fpsp,%eax	
	movl	%eax,%edi	
	addl	$s_stk,%edi	
	shll	$5,%ebx		
	addb	%bl,%al		
	addl	$s_stk,%eax	
	movl	8(%eax),%ecx	
	movl	4(%eax),%edx	
	movl	(%eax),%eax	
	jmp	epcomu		
fucomps:			#ST - ST(i) -> codes, pop
	#SAVEIN
	movl	%esi,s_iptr	
	#GETEPS  ebx
	movl	s_fpsp,%eax	
	movl	%eax,%edi	
	addl	$s_stk,%edi	
	shll	$5,%ebx		
	addb	%bl,%al		
	addl	$s_stk,%eax	
	movl	8(%eax),%ecx	
	movl	4(%eax),%edx	
	movl	(%eax),%eax	
	call	epcomu		
	#STPOPq
	movb	$3,10(%edi)	
	addb	$32,s_fpsp	
	ret			
is1t:	.long	fchs_,fabs_,NONE_,NONE_,ftst_,fxam_,NONE_,NONE_
is1:				
	#SAVEIN
	movl	%esi,s_iptr	
	jmp	*%cs:is1t(,%ebx,4)	
fchs_:				#ST = -ST
	andb	$0x0fd,h_stat+1	
	#STTOP   
	movl	s_fpsp,%edi	
	addl	$s_stk,%edi	
	xorb	$0x80,11(%edi)	
	ret			
fabs_:				#ST = |ST|
	andb	$0x0fd,h_stat+1	
	#STTOP   
	movl	s_fpsp,%edi	
	addl	$s_stk,%edi	
	andb	$0x7f,11(%edi)	
	ret			
ftst_:				#set codes according to ST
	#STTOP   
	movl	s_fpsp,%edi	
	addl	$s_stk,%edi	
	movl	$0x00010000,%ecx	
	xorl	%eax,%eax	
	xorl	%edx,%edx	
	jmp	epcomp		
fxam_:				#set codes according to ST
	andb	$0x0b8,h_stat+1	
	movl	s_fpsp,%edi	
	addl	$s_stk,%edi	
	cmpb	$3,10(%edi)	
	jz	fxa9		
	#GETEP   
	movl	(%edi),%eax	
	movl	4(%edi),%edx	
	movl	8(%edi),%ecx	
	call	epexam		
	orb	%al,h_stat+1	
fxa9:	ret			
is8t:	.long	ld1,ldl2t,ldl2e,ldpi,ldlg2,ldln2,ldz,NONE_ 
is8:				
	#SAVEIN
	movl	%esi,s_iptr	
	andb	$0x0fd,h_stat+1	
	call	*%cs:is8t(,%ebx,4)	
	#STPUSHP
	movl	s_fpsp,%ebx	
	subb	$32,%bl		
	cmpb	$3,s_stk+10(%ebx)	
	jnz	stackof		
	movl	%ebx,s_fpsp	
	addl	$s_stk,%ebx	
	movl	%eax,(%ebx)	
	movl	%edx,4(%ebx)	
	movl	%ecx,8(%ebx)	
	ret			
ld1:				#load 1
	xorl	%eax,%eax	
	movl	$0x80000000,%edx	
	movl	$0x00003fff,%ecx	
	ret			
ldl2t:				#load log2(10)
	movl	$0xcd1b8afe,%eax	
	movl	$0xd49a784b,%edx	
	movl	$0x00004000,%ecx	
	movb	h_ctrl+1,%bh	
	andb	$0x0c,%bh	
	cmpb	$0x08,%bh	
	jnz	ldc3		
	incl	%eax		
ldc3:	ret			
ldl2e:				#load log2(e)
	movl	$0x5c17f0bb,%eax	
	movl	$0xb8aa3b29,%edx	
	movl	$0x00003fff,%ecx	
	testb	$0x04,h_ctrl+1	
	jnz	ldc5		
	incl	%eax		
ldc5:	ret			
ldpi:				#load pi
	movl	$0x2168c234,%eax	
	movl	$0xc90fdaa2,%edx	
	movl	$0x00004000,%ecx	
	testb	$0x04,h_ctrl+1	
	jnz	ldp5		
	incl	%eax		
ldp5:	ret			
ldlg2:				#load log10(2)
	movl	$0xfbcff798,%eax	
	movl	$0x9a209a84,%edx	
	movl	$0x00003ffd,%ecx	
	testb	$0x04,h_ctrl+1	
	jnz	ldc9		
	incl	%eax		
ldc9:	ret			
ldln2:				#load ln(2)
	movl	$0xd1cf79ab,%eax	
	movl	$0xb17217f7,%edx	
	movl	$0x00003ffe,%ecx	
	testb	$0x04,h_ctrl+1	
	jnz	ldc11		
	incl	%eax		
ldc11:	ret			
ldz:				#load 0
	xorl	%eax,%eax	
	xorl	%edx,%edx	
	movl	$0x00010000,%ecx	
	ret			
is2t:	.long	f2xm1_,fyl2x_,fptan_,fpatan_,fxtract_,fprem_,fdecstp_,fincstp_ 
is2:				
	#SAVEIN
	movl	%esi,s_iptr	
	jmp	*%cs:is2t(,%ebx,4)	
f2xm1_:				#ST = (2^ST) - 1
	#STTOP   
	movl	s_fpsp,%edi	
	addl	$s_stk,%edi	
	#GETEP   
	movl	(%edi),%eax	
	movl	4(%edi),%edx	
	movl	8(%edi),%ecx	
	call	epf2xm1		
	#PUTEP   
	movl	%eax,(%edi)	
	movl	%edx,4(%edi)	
	movl	%ecx,8(%edi)	
	ret			
fyl2x_:				#calculate log2(ST) * ST(1), pop once
	#STTOP   
	movl	s_fpsp,%edi	
	addl	$s_stk,%edi	
	#GETEP   
	movl	(%edi),%eax	
	movl	4(%edi),%edx	
	movl	8(%edi),%ecx	
	call	eplog		
	#STPOPq
	movb	$3,10(%edi)	
	addb	$32,s_fpsp	
	#STTOP   
	movl	s_fpsp,%edi	
	addl	$s_stk,%edi	
	call	epmul		
	jmp	rounde		
fptan_:				#make ST into tan(ST)
	#STTOP   
	movl	s_fpsp,%edi	
	addl	$s_stk,%edi	
	#GETEP   
	movl	(%edi),%eax	
	movl	4(%edi),%edx	
	movl	8(%edi),%ecx	
	call	reduct		
	movl	%edi,%esi	
	#STPUSH
	xchgl	%edi,%ebx	
	movl	s_fpsp,%ebx	
	subb	$32,%bl		
	xchgl	%edi,%ebx	
	cmpb	$3,s_stk+10(%edi)	
	jz	LL1		
	call	stackof		
LL1:	movl	%edi,s_fpsp	
	addl	$s_stk,%edi	
	pushl	%edi		
	movl	%esi,%edi	
	testl	$0x30000,%ecx	
	jnz	tan11		
	call	eptan		
tan5:	#PUTEP   
	movl	%eax,(%edi)	
	movl	%edx,4(%edi)	
	movl	%ecx,8(%edi)	
	movl	$tBIAS,%ecx	
	movl	$0x80000000,%edx	
	xorl	%eax,%eax	
tan6:	popl	%edi		
	#PUTEP   
	movl	%eax,(%edi)	
	movl	%edx,4(%edi)	
	movl	%ecx,8(%edi)	
	ret			
tan11:	testl	$0x50000,%ecx	
	jnz	tan5		
	#PUTEP   
	movl	%eax,(%edi)	
	movl	%edx,4(%edi)	
	movl	%ecx,8(%edi)	
	call	getqn		
	jmp	tan6		
fpatan_:			#arctangent of ST(1)/ST, pop stack
	#STTOP   
	movl	s_fpsp,%edi	
	addl	$s_stk,%edi	
	#GETEP   
	movl	(%edi),%eax	
	movl	4(%edi),%edx	
	movl	8(%edi),%ecx	
	#STIX1
	movl	s_fpsp,%ebx	
	addb	$32,%bl		
	addl	$s_stk,%ebx	
	xchgl	%edi,%ebx	
	call	epatan		
	#PUTEP   
	movl	%eax,(%edi)	
	movl	%edx,4(%edi)	
	movl	%ecx,8(%edi)	
	#STPOP
	movl	s_fpsp,%eax	
	addl	$s_stk,%eax	
	movb	$3,10(%eax)	
	addb	$32,s_fpsp	
	ret			
fxtract_:			#extract exponent and mantissa
	andb	$0x0fd,h_stat+1	
	#STTOP   
	movl	s_fpsp,%edi	
	addl	$s_stk,%edi	
	#GETEP   
	movl	(%edi),%eax	
	movl	4(%edi),%edx	
	movl	8(%edi),%ecx	
	call	xtract		
	#STPUSHP
	movl	s_fpsp,%ebx	
	subb	$32,%bl		
	cmpb	$3,s_stk+10(%ebx)	
	jnz	stackof		
	movl	%ebx,s_fpsp	
	addl	$s_stk,%ebx	
	movl	%eax,(%ebx)	
	movl	%edx,4(%ebx)	
	movl	%ecx,8(%ebx)	
	ret			
fdecstp_:			#decrement stack pointer
	subb	$32,s_fpsp	
	ret			
fincstp_:			#increment stack pointer
	addb	$32,s_fpsp	
	ret			
is3t:	.long	fprem_,fyl2xp1_,fsqrt_,fsincos_,frndint_,fscale_,fsin_,fcos_ 
is3:				
	#SAVEIN
	movl	%esi,s_iptr	
	jmp	*%cs:is3t(,%ebx,4)	
fprem_:				# ST = ST / ST(1) Intel/IEEE remainder
	#STIX1q
	movl	s_fpsp,%eax	
	addb	$32,%al		
	addl	$s_stk,%eax	
	xchgl	%edi,%eax	
	#GETEP   
	movl	(%edi),%eax	
	movl	4(%edi),%edx	
	movl	8(%edi),%ecx	
	#STTOP   
	movl	s_fpsp,%edi	
	addl	$s_stk,%edi	
	call	eprem		
	#PUTEP   
	movl	%eax,(%edi)	
	movl	%edx,4(%edi)	
	movl	%ecx,8(%edi)	
	ret			
fyl2xp1_:			#calculate log2(ST+1) * ST(1), pop once
	#STTOP   
	movl	s_fpsp,%edi	
	addl	$s_stk,%edi	
	#GETEP   
	movl	(%edi),%eax	
	movl	4(%edi),%edx	
	movl	8(%edi),%ecx	
	call	eplog1		
	#STPOPq
	movb	$3,10(%edi)	
	addb	$32,s_fpsp	
	#STTOP   
	movl	s_fpsp,%edi	
	addl	$s_stk,%edi	
	call	epmul		
	jmp	rounde		
frndint_:			#round ST to integer
	#STTOP   
	movl	s_fpsp,%edi	
	addl	$s_stk,%edi	
	#GETEP   
	movl	(%edi),%eax	
	movl	4(%edi),%edx	
	movl	8(%edi),%ecx	
	call	eptown		
	#PUTEP   
	movl	%eax,(%edi)	
	movl	%edx,4(%edi)	
	movl	%ecx,8(%edi)	
	ret			
fscale_:			#add ST(1) to exponent of ST
	movl	s_fpsp,%eax	
	addl	$s_stk,%eax	
	movl	%eax,%edi	
	subl	$s_stk,%eax	
	addb	$32,%al		
	addl	$s_stk,%eax	
	movl	8(%eax),%ecx	
	movl	4(%eax),%edx	
	movl	(%eax),%eax	
	call	scale		
	jmp	roundx		
fucompps:			#ST - ST(1) -> codes, pop twice
	#SAVEIN
	movl	%esi,s_iptr	
	#STIX1q
	movl	s_fpsp,%eax	
	addb	$32,%al		
	addl	$s_stk,%eax	
	xchgl	%edi,%eax	
	#GETEP   
	movl	(%edi),%eax	
	movl	4(%edi),%edx	
	movl	8(%edi),%ecx	
	#STTOP   
	movl	s_fpsp,%edi	
	addl	$s_stk,%edi	
	call	epcomu		
	movl	%edi,%eax	
	movb	$3,10(%eax)	
	subl	$s_stk,%eax	
	addb	$32,%al		
	addl	$s_stk,%eax	
	movb	$3,10(%eax)	
	addb	$64,s_fpsp	
	ret			
fisti:				#fetch into 32-bit integer
	#SAVEIX
	movl	%eax,s_memp	
	xchgl	%esi,%eax	
	movl	%eax,s_iptr	
	#STTOP   
	movl	s_fpsp,%edi	
	addl	$s_stk,%edi	
	#GETEP   
	movl	(%edi),%eax	
	movl	4(%edi),%edx	
	movl	8(%edi),%ecx	
	call	eptosi		
	movl	%eax,(%esi)	
	ret			
fistpi:				#pop into 32-bit integer
	#SAVEIX
	movl	%eax,s_memp	
	xchgl	%esi,%eax	
	movl	%eax,s_iptr	
	#STTOP   
	movl	s_fpsp,%edi	
	addl	$s_stk,%edi	
	#GETEP   
	movl	(%edi),%eax	
	movl	4(%edi),%edx	
	movl	8(%edi),%ecx	
	call	eptosi		
	movl	%eax,(%esi)	
	#STPOPq
	movb	$3,10(%edi)	
	addb	$32,s_fpsp	
	ret			
flde:				#push EP from memory
	#SAVEII
	movl	%esi,s_iptr	
	movl	%eax,s_memp	
	andb	$0x0fd,h_stat+1	
	call	eptoep		
	#STPUSHP
	movl	s_fpsp,%ebx	
	subb	$32,%bl		
	cmpb	$3,s_stk+10(%ebx)	
	jnz	stackof		
	movl	%ebx,s_fpsp	
	addl	$s_stk,%ebx	
	movl	%eax,(%ebx)	
	movl	%edx,4(%ebx)	
	movl	%ecx,8(%ebx)	
	ret			
fstpe:				#fetch as extended-precision
	#SAVEIX
	movl	%eax,s_memp	
	xchgl	%esi,%eax	
	movl	%eax,s_iptr	
	andb	$0x0fd,h_stat+1	
	#STTOP   
	movl	s_fpsp,%edi	
	addl	$s_stk,%edi	
	#GETEP   
	movl	(%edi),%eax	
	movl	4(%edi),%edx	
	movl	8(%edi),%ecx	
	shldl	$16,%ecx,%ebx	
	cmpb	$3,%bl		
	jnz	stpe5		
	call	getqn		
stpe5:	movl	%eax,(%esi)	
	movl	%edx,4(%esi)	
	addl	%ecx,%ecx	
	rcrw	$1,%cx		
	movw	%cx,8(%esi)	
	#STPOPq
	movb	$3,10(%edi)	
	addb	$32,s_fpsp	
	ret			
is5t:	.long	NONE_,NONE_,fclex_,finit_,NONE_,NONE_,NONE_,NONE_ 
is5:	jmp	*%cs:is5t(,%ebx,4)	
fclex_:				#clear error status flags
	andw	$0x7f00,h_stat	
	ret			
faddx:				#ST(i) = ST + ST(i)
	#SAVEIN
	movl	%esi,s_iptr	
	#GETEPX  ebx
	movl	s_fpsp,%eax	
	addl	$s_stk,%eax	
	movl	(%eax),%edi	
	movl	4(%eax),%edx	
	movl	8(%eax),%ecx	
	subl	$s_stk,%eax	
	shll	$5,%ebx		
	addb	%bl,%al		
	addl	$s_stk,%eax	
	xchgl	%edi,%eax	
	call	epadd		
	jmp	rounde		
fmulx:				#ST(i) = ST * ST(i)
	#SAVEIN
	movl	%esi,s_iptr	
	#GETEPX  ebx
	movl	s_fpsp,%eax	
	addl	$s_stk,%eax	
	movl	(%eax),%edi	
	movl	4(%eax),%edx	
	movl	8(%eax),%ecx	
	subl	$s_stk,%eax	
	shll	$5,%ebx		
	addb	%bl,%al		
	addl	$s_stk,%eax	
	xchgl	%edi,%eax	
	call	epmul		
	jmp	rounde		
fsubx:				#ST(i) = ST - ST(i)
	#SAVEIN
	movl	%esi,s_iptr	
	#GETEPX  ebx
	movl	s_fpsp,%eax	
	addl	$s_stk,%eax	
	movl	(%eax),%edi	
	movl	4(%eax),%edx	
	movl	8(%eax),%ecx	
	subl	$s_stk,%eax	
	shll	$5,%ebx		
	addb	%bl,%al		
	addl	$s_stk,%eax	
	xchgl	%edi,%eax	
	call	epsub		
	jmp	rounde		
fsubrx:				#ST(i) = ST(i) - ST
	#SAVEIN
	movl	%esi,s_iptr	
	#GETEPX  ebx
	movl	s_fpsp,%eax	
	addl	$s_stk,%eax	
	movl	(%eax),%edi	
	movl	4(%eax),%edx	
	movl	8(%eax),%ecx	
	subl	$s_stk,%eax	
	shll	$5,%ebx		
	addb	%bl,%al		
	addl	$s_stk,%eax	
	xchgl	%edi,%eax	
	call	epsubr		
	jmp	rounde		
fdivx:				#ST(i) = ST / ST(i)
	#SAVEIN
	movl	%esi,s_iptr	
	#GETEPX  ebx
	movl	s_fpsp,%eax	
	addl	$s_stk,%eax	
	movl	(%eax),%edi	
	movl	4(%eax),%edx	
	movl	8(%eax),%ecx	
	subl	$s_stk,%eax	
	shll	$5,%ebx		
	addb	%bl,%al		
	addl	$s_stk,%eax	
	xchgl	%edi,%eax	
	call	epdiv		
	jmp	rounde		
fdivrx:				#ST(i) = ST(i) / ST
	#SAVEIN
	movl	%esi,s_iptr	
	#GETEPX  ebx
	movl	s_fpsp,%eax	
	addl	$s_stk,%eax	
	movl	(%eax),%edi	
	movl	4(%eax),%edx	
	movl	8(%eax),%ecx	
	subl	$s_stk,%eax	
	shll	$5,%ebx		
	addb	%bl,%al		
	addl	$s_stk,%eax	
	xchgl	%edi,%eax	
	call	epdivr		
	jmp	rounde		
fldenv_:			#load environment
frstor_:			#restore the HW status
	movl	%eax,%esi	
	movl	$h_ctrl,%edi	
	movl	$3,%ecx		
	.byte	0x36 #from ss: 
	repz			
	movsl			
	.byte	0x36 #from ss: 
	lodsl			#instruction pointer
	movl	%eax,s_iptr	
	addl	$4,%esi		
	.byte	0x36 #from ss: 
	lodsl			#operand pointer
	movl	%eax,s_memp	
	addl	$4,%esi		
	movb	h_stat+1,%al	
	shlb	$2,%al		
	andl	$0x0e0,%eax	
	movl	%eax,s_fpsp	
	cmpb	$0x2c,%dl	
	jnz	rst5		
	movl	s_fpsp,%edi	
	addl	$s_stk,%edi	
	movl	$8,%ecx		
lds2:	movl	%esi,%eax	
	pushl	%ecx		
	call	eptoep		
	#PUTEP
	movl	%eax,(%edi)	
	movl	%edx,4(%edi)	
	movl	%ecx,8(%edi)	
	popl	%ecx		
	addl	$10,%esi	
	movl	%edi,%eax	
	subl	$s_stk,%eax	
	addb	$32,%al		
	addl	$s_stk,%eax	
	movl	%eax,%edi	
	loop	lds2		
rst5:	movw	h_ctrl,%ax	
	notl	%eax		
	andw	h_stat,%ax	
	testb	$0x3f,%al	
	jz	rst7		
	orw	$0x8080,h_stat	
rst7:	movl	$8,%ecx		#prepare to loop over all 8 tags
	movw	h_tag,%ax	
	movl	$s_stk+0x0e0,%edi	
ltt3:	rolw	$2,%ax		
	movb	%al,%dl		
	andb	$3,%dl		
	movb	%dl,10(%edi)	
	subl	$32,%edi	
	loop	ltt3		
	ret			
fstenv_:			#store environment
fsave_:				#save HW block
	pushl	%es		#flat model assumes target is stack
	movw	%ss,%cx		
	movw	%cx,%es		
	pushl	%eax		
	movl	$8,%ecx		#prepare to loop over all 8 tags
	movl	$s_stk,%edi	
stt3:	movb	10(%edi),%bl	#concatenate tags       
	shrdw	$2,%bx,%ax	
	addl	$32,%edi	
	loop	stt3		
	movw	%ax,h_tag	#store new tags
	popl	%edi		
	call	stfpsp		
	movl	$h_ctrl,%esi	
	movsl			
	movsl			
	movsl			
	movl	s_iptr,%eax	#instruction pointer
	stosl			
	orl	%eax,%eax	
	jz	stp5		
	movw	%cs,%ax		
stp5:	stosl			
	movl	s_memp,%eax	#operand pointer
	stosl			
	orl	%eax,%eax	
	jz	stp7		
	movw	%es,%ax		
stp7:	stosl			
	popl	%es		
	cmpb	$0x2e,%dl	
	jnz	sts9		
	movl	$8,%ecx		
	movl	s_fpsp,%eax	
sts2:	addl	$s_stk,%eax	
	movl	(%eax),%esi	
	movl	4(%eax),%edx	
	movl	8(%eax),%ebx	
	subl	$s_stk,%eax	
	movl	%esi,(%edi)	
	movl	%edx,4(%edi)	
	addl	%ebx,%ebx	
	rcrw	$1,%bx		
	movw	%bx,8(%edi)	
	addl	$10,%edi	
	addb	$32,%al		
	loop	sts2		

finit_:				#initialize hardware block
	xorl	%eax,%eax	
	movl	$0x037f,h_ctrl	
	movl	%eax,h_stat	
	movl	$0xffff,h_tag	
	movl	%eax,s_iptr	
	movl	%eax,s_memp	
	movl	$s_stk,%eax	
	movl	$0,s_fpsp	
	movl	$8,%ecx		
fin4:	movb	$3,10(%eax)	
	addl	$32,%eax	
	loop	fin4		
sts9:	ret			
	.globl	fstsw_
fstsw_:				#fetch HW status word
	movl	%eax,%edi	
	call	stfpsp		
	movw	h_stat,%ax	
	stosw			
	ret			
ffree_:				#change tag to empty
	#SAVEIN
	movl	%esi,s_iptr	
	#STIX    ebx
	movl	s_fpsp,%eax	
	shll	$5,%ebx		
	addb	%bl,%al		
	addl	$s_stk,%eax	
	xchgl	%edi,%eax	
	movb	$3,10(%edi)	
	ret			
fsts:				#ST(i) = ST 
	#SAVEIN
	movl	%esi,s_iptr	
	#GETEPX  ebx
	movl	s_fpsp,%eax	
	addl	$s_stk,%eax	
	movl	(%eax),%edi	
	movl	4(%eax),%edx	
	movl	8(%eax),%ecx	
	subl	$s_stk,%eax	
	shll	$5,%ebx		
	addb	%bl,%al		
	addl	$s_stk,%eax	
	xchgl	%edi,%eax	
	#PUTEP   
	movl	%eax,(%edi)	
	movl	%edx,4(%edi)	
	movl	%ecx,8(%edi)	
	ret			
faddpx:				#ST(i) = ST + ST(i) and pop
	#SAVEIN
	movl	%esi,s_iptr	
	#GETEPX  ebx
	movl	s_fpsp,%eax	
	addl	$s_stk,%eax	
	movl	(%eax),%edi	
	movl	4(%eax),%edx	
	movl	8(%eax),%ecx	
	subl	$s_stk,%eax	
	shll	$5,%ebx		
	addb	%bl,%al		
	addl	$s_stk,%eax	
	xchgl	%edi,%eax	
	call	epadd		
	call	rounde		
	#STPOP
	movl	s_fpsp,%eax	
	addl	$s_stk,%eax	
	movb	$3,10(%eax)	
	addb	$32,s_fpsp	
	ret			
fmulpx:				#ST(i) = ST * ST(i) and pop
	#SAVEIN
	movl	%esi,s_iptr	
	#GETEPX  ebx
	movl	s_fpsp,%eax	
	addl	$s_stk,%eax	
	movl	(%eax),%edi	
	movl	4(%eax),%edx	
	movl	8(%eax),%ecx	
	subl	$s_stk,%eax	
	shll	$5,%ebx		
	addb	%bl,%al		
	addl	$s_stk,%eax	
	xchgl	%edi,%eax	
	call	epmul		
	call	rounde		
	#STPOP
	movl	s_fpsp,%eax	
	addl	$s_stk,%eax	
	movb	$3,10(%eax)	
	addb	$32,s_fpsp	
	ret			
fcompps:			#ST - ST(1) -> codes, pop twice
	#SAVEIN
	movl	%esi,s_iptr	
	#STIX1q
	movl	s_fpsp,%eax	
	addb	$32,%al		
	addl	$s_stk,%eax	
	xchgl	%edi,%eax	
	#GETEP   
	movl	(%edi),%eax	
	movl	4(%edi),%edx	
	movl	8(%edi),%ecx	
	#STTOP   
	movl	s_fpsp,%edi	
	addl	$s_stk,%edi	
	call	epcomp		
	movl	%edi,%eax	
	movb	$3,10(%eax)	
	subl	$s_stk,%eax	
	addb	$32,%al		
	addl	$s_stk,%eax	
	movb	$3,10(%eax)	
	addb	$64,s_fpsp	
	ret			
fsubpx:				#ST(i) = ST - ST(i) and pop
	#SAVEIN
	movl	%esi,s_iptr	
	#GETEPX  ebx
	movl	s_fpsp,%eax	
	addl	$s_stk,%eax	
	movl	(%eax),%edi	
	movl	4(%eax),%edx	
	movl	8(%eax),%ecx	
	subl	$s_stk,%eax	
	shll	$5,%ebx		
	addb	%bl,%al		
	addl	$s_stk,%eax	
	xchgl	%edi,%eax	
	call	epsub		
	call	rounde		
	#STPOP
	movl	s_fpsp,%eax	
	addl	$s_stk,%eax	
	movb	$3,10(%eax)	
	addb	$32,s_fpsp	
	ret			
fsubrpx:			#ST(i) = ST(i) - ST and pop
	#SAVEIN
	movl	%esi,s_iptr	
	#GETEPX  ebx
	movl	s_fpsp,%eax	
	addl	$s_stk,%eax	
	movl	(%eax),%edi	
	movl	4(%eax),%edx	
	movl	8(%eax),%ecx	
	subl	$s_stk,%eax	
	shll	$5,%ebx		
	addb	%bl,%al		
	addl	$s_stk,%eax	
	xchgl	%edi,%eax	
	call	epsubr		
	call	rounde		
	#STPOP
	movl	s_fpsp,%eax	
	addl	$s_stk,%eax	
	movb	$3,10(%eax)	
	addb	$32,s_fpsp	
	ret			
fdivpx:				#ST(i) = ST/ST(i) and pop
	#SAVEIN
	movl	%esi,s_iptr	
	#GETEPX  ebx
	movl	s_fpsp,%eax	
	addl	$s_stk,%eax	
	movl	(%eax),%edi	
	movl	4(%eax),%edx	
	movl	8(%eax),%ecx	
	subl	$s_stk,%eax	
	shll	$5,%ebx		
	addb	%bl,%al		
	addl	$s_stk,%eax	
	xchgl	%edi,%eax	
	call	epdivr		
	call	rounde		
	#STPOP
	movl	s_fpsp,%eax	
	addl	$s_stk,%eax	
	movb	$3,10(%eax)	
	addb	$32,s_fpsp	
	ret			
fdivrpx:			#ST(i) = ST(i)/ST and pop
	#SAVEIN
	movl	%esi,s_iptr	
	#GETEPX  ebx
	movl	s_fpsp,%eax	
	addl	$s_stk,%eax	
	movl	(%eax),%edi	
	movl	4(%eax),%edx	
	movl	8(%eax),%ecx	
	subl	$s_stk,%eax	
	shll	$5,%ebx		
	addb	%bl,%al		
	addl	$s_stk,%eax	
	xchgl	%edi,%eax	
	call	epdiv		
	call	rounde		
	#STPOP
	movl	s_fpsp,%eax	
	addl	$s_stk,%eax	
	movb	$3,10(%eax)	
	addb	$32,s_fpsp	
	ret			
fistw:				#fetch 16-bit integer
	#SAVEIX
	movl	%eax,s_memp	
	xchgl	%esi,%eax	
	movl	%eax,s_iptr	
	#STTOP   
	movl	s_fpsp,%edi	
	addl	$s_stk,%edi	
	#GETEP   
	movl	(%edi),%eax	
	movl	4(%edi),%edx	
	movl	8(%edi),%ecx	
	call	eptowi		
	movw	%ax,(%esi)	
	ret			
fistpw:				#fetch 16-bit integer, pop
	#SAVEIX
	movl	%eax,s_memp	
	xchgl	%esi,%eax	
	movl	%eax,s_iptr	
	#STTOP   
	movl	s_fpsp,%edi	
	addl	$s_stk,%edi	
	#GETEP   
	movl	(%edi),%eax	
	movl	4(%edi),%edx	
	movl	8(%edi),%ecx	
	call	eptowi		
	movw	%ax,(%esi)	
	#STPOPq
	movb	$3,10(%edi)	
	addb	$32,s_fpsp	
	ret			
fbldp:				#push 10-byte packed-decimal integer
	#SAVEII
	movl	%esi,s_iptr	
	movl	%eax,s_memp	
	andb	$0x0fd,h_stat+1	
	call	pdtoep		
	#STPUSHP
	movl	s_fpsp,%ebx	
	subb	$32,%bl		
	cmpb	$3,s_stk+10(%ebx)	
	jnz	stackof		
	movl	%ebx,s_fpsp	
	addl	$s_stk,%ebx	
	movl	%eax,(%ebx)	
	movl	%edx,4(%ebx)	
	movl	%ecx,8(%ebx)	
	ret			
fildl:				#push 64-bit integer
	andb	$0x0fd,h_stat+1	
	call	litoep		
	#STPUSHP
	movl	s_fpsp,%ebx	
	subb	$32,%bl		
	cmpb	$3,s_stk+10(%ebx)	
	jnz	stackof		
	movl	%ebx,s_fpsp	
	addl	$s_stk,%ebx	
	movl	%eax,(%ebx)	
	movl	%edx,4(%ebx)	
	movl	%ecx,8(%ebx)	
	ret			
fbstpp:				#pop into 10-byte packed-decimal integer
	#SAVEII
	movl	%esi,s_iptr	
	movl	%eax,s_memp	
	#STTOP   
	movl	s_fpsp,%edi	
	addl	$s_stk,%edi	
	#GETEP   
	movl	(%edi),%eax	
	movl	4(%edi),%edx	
	movl	8(%edi),%ecx	
	movl	s_memp,%edi	
	call	eptopd		
	#STPOP
	movl	s_fpsp,%eax	
	addl	$s_stk,%eax	
	movb	$3,10(%eax)	
	addb	$32,s_fpsp	
	ret			
fistpl:				#pop into 64-bit integer
	#SAVEIX
	movl	%eax,s_memp	
	xchgl	%esi,%eax	
	movl	%eax,s_iptr	
	#STTOP   
	movl	s_fpsp,%edi	
	addl	$s_stk,%edi	
	#GETEP   
	movl	(%edi),%eax	
	movl	4(%edi),%edx	
	movl	8(%edi),%ecx	
	call	eptoli		
	movl	%eax,(%esi)	
	movl	%edx,4(%esi)	
	#STPOPq
	movb	$3,10(%edi)	
	addb	$32,s_fpsp	
	ret			
fstswax_:			#status word -> ax
	call	stfpsp		
	movw	h_stat,%ax	
	movw	%ax,s_ax(%ebp)	
	ret			
fsincos_:			#make ST into sin(ST), push cos(ST)
	#STTOP   
	movl	s_fpsp,%edi	
	addl	$s_stk,%edi	
	#GETEP   
	movl	(%edi),%eax	
	movl	4(%edi),%edx	
	movl	8(%edi),%ecx	
	call	reduct		
	pushl	%edi		
	#STPUSH
	xchgl	%edi,%ebx	
	movl	s_fpsp,%ebx	
	subb	$32,%bl		
	xchgl	%edi,%ebx	
	cmpb	$3,s_stk+10(%edi)	
	jz	LL2		
	call	stackof		
LL2:	movl	%edi,s_fpsp	
	addl	$s_stk,%edi	
	#PUTEP   
	movl	%eax,(%edi)	
	movl	%edx,4(%edi)	
	movl	%ecx,8(%edi)	
	popl	%edi		
	testl	$0x30000,%ecx	
	jnz	sco11		
	pushl	%ebx		
	call	epsin		
	popl	%ebx		
	#PUTEP   
	movl	%eax,(%edi)	
	movl	%edx,4(%edi)	
	movl	%ecx,8(%edi)	
	#STTOP   
	movl	s_fpsp,%edi	
	addl	$s_stk,%edi	
	#GETEP   
	movl	(%edi),%eax	
	movl	4(%edi),%edx	
	movl	8(%edi),%ecx	
	call	epcos		
sco9:	#PUTEP   
	movl	%eax,(%edi)	
	movl	%edx,4(%edi)	
	movl	%ecx,8(%edi)	
	ret			
sco11:	testl	$0x50000,%ecx	
	jz	sco9		
	#PUTEP
	movl	%eax,(%edi)	
	movl	%edx,4(%edi)	
	movl	%ecx,8(%edi)	
	movl	$0x3fff,%ecx	
	movl	$0x80000000,%edx	
	xorl	%eax,%eax	
	#STTOP
	movl	s_fpsp,%edi	
	addl	$s_stk,%edi	
	jmp	sco9		
fsin_:				#make ST into sin(ST)
	#STTOP   
	movl	s_fpsp,%edi	
	addl	$s_stk,%edi	
	#GETEP   
	movl	(%edi),%eax	
	movl	4(%edi),%edx	
	movl	8(%edi),%ecx	
	call	reduct		
	testl	$0x30000,%ecx	
	jnz	sin9		
	call	epsin		
sin9:	#PUTEP   
	movl	%eax,(%edi)	
	movl	%edx,4(%edi)	
	movl	%ecx,8(%edi)	
	ret			
fcos_:				#make ST into cos(ST)
	#STTOP   
	movl	s_fpsp,%edi	
	addl	$s_stk,%edi	
	#GETEP   
	movl	(%edi),%eax	
	movl	4(%edi),%edx	
	movl	8(%edi),%ecx	
	call	reduct		
	testl	$0x30000,%ecx	
	jnz	cos11		
	call	epcos		
cos9:	#PUTEP   
	movl	%eax,(%edi)	
	movl	%edx,4(%edi)	
	movl	%ecx,8(%edi)	
	ret			
cos11:	testl	$0x50000,%ecx	
	jz	cos9		
	movl	$0x3fff,%ecx	
	movl	$0x80000000,%edx	
	xorl	%eax,%eax	
	jmp	cos9		
NONE_:				
	#SAVEIN
	movl	%esi,s_iptr	
	ret			

# stack overflow handler

stackof:			
	orw	$stacko,h_stat	#stack overflow
	call	exproc		
	orb	$2,h_stat+1	
	movl	$0x80027fff,%ecx	
	movl	$0xc0000000,%edx	
	xorl	%eax,%eax	
	subb	$32,s_fpsp	
	movl	s_fpsp,%ebx	
	addl	$s_stk,%ebx	
	movl	%eax,(%ebx)	
	movl	%edx,4(%ebx)	
	movl	%ecx,8(%ebx)	
	ret			

# store floating-point stack pointer into the status word

stfpsp:				
	movb	s_fpsp,%al	
	shrb	$2,%al		
	movb	h_stat+1,%ah	
	andb	$0x0c7,%ah	
	orb	%ah,%al		
	movb	%al,h_stat+1	
	ret			

#intrn   endp

#emuseg  ends

#        END
