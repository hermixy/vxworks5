/* unixALib.s - i80x86 network assembly routines */

/* Copyright 1984-2001 Wind River Systems, Inc. */

/*
modification history
--------------------
01f,22aug01,hdn  added FUNC/FUNC_LABEL, replaced .align with .balign
01e,15sep93,hdn  deleted 486 specific functions ntohl() etc.
01d,01jun93,hdn  updated to 5.1.
		  - fixed #else and #endif
		  - changed VOID to void
		  - changed ASMLANGUAGE to _ASMLANGUAGE
		  - changed copyright notice
01c,26mar93,hdn  added ntohl for 486's new instruction bswap.
01b,18nov92,hdn  fixed a bug in cksum.
01a,07apr92,hdn  written based on TRON version.
*/ 

#define _ASMLANGUAGE
#include "vxWorks.h"
#include "asm.h"


	.data
	.globl	FUNC(copyright_wind_river)
	.long	FUNC(copyright_wind_river)
	

#ifndef	PORTABLE

	/* internals */

	.globl	GTEXT(cksum)
	.globl  GTEXT(_insque)
	.globl  GTEXT(_remque)


	.text
	.balign 16

/**************************************************************************
* 
* cksum - compute check sum
*
* return 16bit one's complement sum of 'sum' and the 16bit one's 
* complement sum of the string 'string' of byte length 'len'.
* Complicated by 'len' or 'sumlen' not being even.
*
* RETURNS  int
*
* int cksum (sum, string, len, sumlen);
*/ 

FUNC_LABEL(cksum)
	pushl	%ebx
	pushl	%esi
	movl	SP_ARG1+8(%esp),%eax	/* sum */
	movl	SP_ARG2+8(%esp),%esi	/* addr */
	movl	SP_ARG3+8(%esp),%edx	/* len */
	testl	$1,SP_ARG4+8(%esp)	/* previously summed len */
	je	ckEvenSum
	xorl	%ecx,%ecx
	movb	(%esi),%ch		/* move the first byte into %ch */
	clc				/* clear C flag */
	adcl	%ecx,%eax		/* add the 1st byte before others */
	incl	%esi			/* addr+=1 */
	decl	%edx			/* len-=1 */
ckEvenSum:
	movl	%edx,%ecx
	movl	%edx,%ebx
	andl	$3,%ebx			/* %ebx = count % 4 */
	shrl	$2,%ecx			/* %ecx = count / 4 */
	clc				/* clear C flag */
	je	ckEndLoop		/* jump if Loop Counter is 0 */
ckInLoop:
	adcl	(%esi),%eax		/* add word + previous carry */
	leal	4(%esi),%esi		/* addr+=4, no flags have changed */
	decl	%ecx			/* C flag doesn't change */
	jnz	ckInLoop
ckEndLoop:
	movl	%eax,%ecx
	lahf
	testl	$2,%ebx			/* another word to add ? */
	jne	ckNextWord
	testl	$1,%ebx			/* another byte to add ? */
	jne	ckNextByte
	jmp	ckGetOut

	.balign 16,0x90
ckNextWord:
	sahf
	adcw	(%esi),%cx
	lahf	
	leal	2(%esi),%esi
	testl	$1,%ebx			/* another byte to add ? */
	je	ckGetOut
ckNextByte:
	movzbl	(%esi),%edx
	sahf
	adcw	%dx,%cx
	lahf
ckGetOut:
	movl	%ecx,%edx		/* fold 32 bits sum to 16 bits */
	shrl	$16,%edx		/* swap high-half and low-half */
	sahf
	adcw	%dx,%cx			/* add high-half and low-half */
	jnc	ckNoCarry
	addw	$1,%cx			/* add C flag, if it is set */
ckNoCarry:
	movl	%ecx,%eax
	andl	$0xffff,%eax
	popl	%esi
	popl	%ebx
	ret


/****************************************************************************
* 
* insque - insert a node into a linked list
*
.ne 11
.CS
*  BEFORE                          AFTER
*  %ecx            %eax            %ecx            %edx(new)        %eax
*  _______         _______         _______         _______         _______
* |       |       |       |       |       |       |       |       |       |
* | pNext | ----> |       |       | pNext | ----> | pNext | ----> |       |
* |_______|       |_______| ====> |_______|       |_______|       |_______|
* |       |       |       |       |       |       |       |       |       |
* |       | <---- | pPrev |       |       | <---- | pPrev | <---- | pPrev |
* |_______|       |_______|       |_______|       |_______|       |_______|
.CE
*
*/

	.balign 16,0x90
FUNC_LABEL(_insque)
	movl	SP_ARG1(%esp),%edx
	movl	SP_ARG2(%esp),%ecx
	movl	(%ecx),%eax
	movl	%eax,(%edx)
	movl	%ecx,4(%edx)
	movl	%edx,(%ecx)
	movl	%edx,4(%eax)
	ret

/****************************************************************************
* 
* remque - delete a node from a linked list
*
.ne 11
.CS
*  BEFORE                                          AFTER
*  %eax            %edx            %ecx            %eax            %ecx
*  _______         _______         _______         _______         _______
* |       |       |       |       |       |       |       |       |       |
* | pNext | ----> | pNext | ----> |       |       | pNext | ----> |       |
* |_______|       |_______|       |_______| ====> |_______|       |_______|
* |       |       |       |       |       |       |       |       |       |
* |       | <---- | pPrev | <---- | pPrev |       |       | <---- | pPrev |
* |_______|       |_______|       |_______|       |_______|       |_______|
.CE
*
*/

	.balign 16,0x90
FUNC_LABEL(_remque)
	movl	SP_ARG1(%esp),%edx
	movl	4(%edx),%eax
	movl	(%edx),%ecx
	movl	%eax,4(%ecx)
	movl	%ecx,(%eax)
	ret

#endif	/* !PORTABLE */
