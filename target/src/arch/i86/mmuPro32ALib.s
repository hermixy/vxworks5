/* mmuPro32ALib.s - MMU library for PentiumPro/2/3/4 32 bit mode */

/* Copyright 1984-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01c,27aug01,hdn  added FUNC/FUNC_LABEL, replaced .align with .balign
		 preserved IF bit in EFLAGS
01e,17sep98,hdn  renamed mmuEnabled to mmuPro32Enabled.
01d,21apr98,hdn  updated a comment in the header.
01c,13apr98,hdn  added support for PentiumPro's 32bit MMU.
01b,02nov94,hdn  added a support for PCD and PWT bit for 486 and Pentium
01a,26jul93,hdn  written.
*/

/*

*/

#define _ASMLANGUAGE
#include "vxWorks.h"
#include "asm.h"
#include "regs.h"


	/* externals */

	.globl	VAR(mmuPro32Enabled)
	.globl	VAR(sysProcessor)


	/* internals */

	.globl	GTEXT(mmuPro32Enable)
	.globl	GTEXT(mmuPro32On)
	.globl	GTEXT(mmuPro32Off)
	.globl	GTEXT(mmuPro32PdbrSet)
	.globl	GTEXT(mmuPro32PdbrGet)
	.globl	GTEXT(mmuPro32TLBFlush)


	.text
	.balign 16

 
/******************************************************************************
*
* mmuPro32Enable - turn mmu on or off
*
* RETURNS: OK

* STATUS mmuPro32Enable 
*    (
*    BOOL enable		/@ TRUE to enable, FALSE to disable MMU @/
*    )

*/

FUNC_LABEL(mmuPro32Enable)
	pushfl				/* save EFLAGS */
	cli				/* LOCK INTERRUPT */
	movl	SP_ARG1+4(%esp),%edx
        movl    %cr0,%eax
        movl    %edx,FUNC(mmuPro32Enabled)
	cmpl	$0,%edx
	je	mmuPro32Disable
	orl     $0x80010000,%eax	/* set PG and WP */
	jmp     mmuPro32Enable0

mmuPro32Disable:
	andl    $0x7ffeffff,%eax	/* clear PG and WP */

mmuPro32Enable0:
	movl    %eax,%cr0
	jmp     mmuPro32Enable1		/* flush prefetch queue */
mmuPro32Enable1:
	movl	$0,%eax
	popfl				/* UNLOCK INTERRUPT */
	ret

/******************************************************************************
*
* mmuPro32On - turn MMU on 
*
* This routine assumes that interrupts are locked out.  It is called internally
* to enable the mmu after it has been disabled for a short period of time
* to access internal data structs.
*
* NOMANUAL

* void mmuPro32On (void)

*/

	.balign 16,0x90
FUNC_LABEL(mmuPro32On)
        movl    %cr0,%eax
	orl     $0x80010000,%eax	/* set PG and WP */
	movl    %eax,%cr0
	jmp     mmuPro32On0		/* flush prefetch queue */
mmuPro32On0:
	ret

/******************************************************************************
*
* mmuPro32Off - turn MMU off 
*
* This routine assumes that interrupts are locked out.  It is called internally
* to disable the mmu for a short period of time
* to access internal data structs.
*
* NOMANUAL

* void mmuPro32Off (void)

*/

	.balign 16,0x90
FUNC_LABEL(mmuPro32Off)
	movl    %cr0,%eax
	andl    $0x7ffeffff,%eax	/* clear PG and WP */
	movl    %eax,%cr0
	jmp     mmuPro32Off0		/* flush prefetch queue */
mmuPro32Off0:
	ret

/*******************************************************************************
*
* mmuPro32PdbrSet - Set Page Directory Base Register
*
* This routine Set Page Directory Base Register.
*
* NOMANUAL

* void mmuPro32PdbrSet 
*	(
*	void * transTbl;
*	)
 
*/

	.balign 16,0x90
FUNC_LABEL(mmuPro32PdbrSet)
	pushfl				/* save EFLAGS */
	cli				/* LOCK INTERRUPT */
	movl	SP_ARG1+4(%esp),%eax
	movl	(%eax),%eax
	movl	%cr3,%edx
	movl	$0xfffff000,%ecx	/* upper 20 bits */
	andl	$0x00000fff,%edx
	cmpl	$ X86CPU_386,FUNC(sysProcessor)
	je	mmuPro32PdbrSet1
	movl	$0xffffffe0,%ecx	/* upper 27 bits */
	andl	$0x00000007,%edx	/* PCD=0 PWT=0 */
mmuPro32PdbrSet1:
	andl	%ecx,%eax
	orl	%edx,%eax
	movl	%eax,%cr3
	jmp	mmuPro32PdbrSet0	/* flush prefetch queue */
mmuPro32PdbrSet0:
	popfl				/* UNLOCK INTERRUPT */
	ret

/*******************************************************************************
*
* mmuPro32PdbrGet - Get Page Directory Base Register
*
* This routine Get Page Directory Base Register.
*
* NOMANUAL

* MMU_TRANS_TBL * mmuPro32PdbrGet (void)
 
*/

	.balign 16,0x90
FUNC_LABEL(mmuPro32PdbrGet)
	movl	%cr3,%eax
	movl	$0xfffff000,%edx	/* upper 20 bits */
	cmpl	$ X86CPU_386,FUNC(sysProcessor)
	je	mmuPro32PdbrGet1
	movl	$0xffffffe0,%edx	/* upper 27 bits */
mmuPro32PdbrGet1:
	andl	%edx,%eax
	ret

/******************************************************************************
*
* mmuPro32TLBFlush - flush the Translation Lookaside Buffer.
*
* NOMANUAL

* void mmuPro32TLBFlush (void)

*/

	.balign 16,0x90
FUNC_LABEL(mmuPro32TLBFlush)
	pushfl				/* save EFLAGS */
	cli				/* LOCK INTERRUPT */
	movl	%cr3,%eax
	movl	%eax,%cr3
	jmp	mmuPro32TLBFlush0	/* flush prefetch queue */
mmuPro32TLBFlush0:
	popfl				/* UNLOCK INTERRUPT */
	ret

