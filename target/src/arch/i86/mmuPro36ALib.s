/* mmuPro36ALib.s - MMU library for PentiumPro/2/3/4 36 bit mode */

/* Copyright 1984-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01e,27aug01,hdn  added FUNC/FUNC_LABEL, replaced .align with .balign
		 preserved IF bit in EFLAGS
01d,17sep98,hdn  renamed mmuEnabled to mmuPro36Enabled.
01c,13apr98,hdn  added support for PentiumPro's 36bit MMU.
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

	.globl	VAR(mmuPro36Enabled)
	.globl	VAR(sysProcessor)


	/* internals */

	.globl	GTEXT(mmuPro36Enable)
	.globl	GTEXT(mmuPro36On)
	.globl	GTEXT(mmuPro36Off)
	.globl	GTEXT(mmuPro36PdbrSet)
	.globl	GTEXT(mmuPro36PdbrGet)
	.globl	GTEXT(mmuPro36TLBFlush)


	.text
	.balign 16

 
/******************************************************************************
*
* mmuPro36Enable - turn mmu on or off
*
* RETURNS: OK

* STATUS mmuPro36Enable 
*    (
*    BOOL enable		/@ TRUE to enable, FALSE to disable MMU @/
*    )

*/

FUNC_LABEL(mmuPro36Enable)
	pushfl				/* save EFLAGS */
	cli				/* LOCK INTERRUPT */
	movl	SP_ARG1+4(%esp),%edx
        movl    %cr0,%eax
        movl    %edx,FUNC(mmuPro36Enabled)
	cmpl	$0,%edx
	je	mmuPro36Disable
	orl     $0x80010000,%eax	/* set PG, WP in CR0 */
	movl    %eax,%edx
        movl    %cr4,%eax
	orl	$0x000000a0,%eax	/* set PAE, PGE in CR4 */
	movl    %eax,%cr4
	movl    %edx,%eax
	movl    %eax,%cr0
	jmp     mmuPro36Enable1		/* flush prefetch queue */

mmuPro36Disable:
	andl    $0x7ffeffff,%eax	/* clear PG, WP in CR0 */
	movl    %eax,%edx
        movl    %cr4,%eax
	andl	$0xffffff5f,%eax	/* clear PAE, PGE in CR4 */
	movl    %eax,%cr4
	movl    %edx,%eax
	movl    %eax,%cr0
	jmp     mmuPro36Enable1		/* flush prefetch queue */

mmuPro36Enable1:
	movl	$0,%eax
	popfl				/* UNLOCK INTERRUPT */
	ret

/******************************************************************************
*
* mmuPro36On - turn MMU on 
*
* This routine assumes that interrupts are locked out.  It is called internally
* to enable the mmu after it has been disabled for a short period of time
* to access internal data structs.
*
* NOMANUAL

* void mmuPro36On (void)

*/

	.balign 16,0x90
FUNC_LABEL(mmuPro36On)
        movl    %cr0,%eax
	orl     $0x80010000,%eax	/* set PG and WP */
	movl    %eax,%edx
        movl    %cr4,%eax
	orl	$0x000000a0,%eax	/* set PAE, PGE in CR4 */
	movl    %eax,%cr4
	movl    %edx,%eax
	movl    %eax,%cr0
	jmp     mmuPro36On0		/* flush prefetch queue */
mmuPro36On0:
	ret

/******************************************************************************
*
* mmuPro36Off - turn MMU off 
*
* This routine assumes that interrupts are locked out.  It is called internally
* to disable the mmu for a short period of time
* to access internal data structs.
*
* NOMANUAL

* void mmuPro36Off (void)

*/

	.balign 16,0x90
FUNC_LABEL(mmuPro36Off)
	movl    %cr0,%eax
	andl    $0x7ffeffff,%eax	/* clear PG and WP */
	movl    %eax,%edx
        movl    %cr4,%eax
	andl	$0xffffff5f,%eax	/* clear PAE, PGE in CR4 */
	movl    %eax,%cr4
	movl    %edx,%eax
	movl    %eax,%cr0
	jmp     mmuPro36Off0		/* flush prefetch queue */
mmuPro36Off0:
	ret

/*******************************************************************************
*
* mmuPro36PdbrSet - Set Page Directory Base Register
*
* This routine Set Page Directory Base Register.
*
* NOMANUAL

* void mmuPro36PdbrSet 
*	(
*	void * transTbl;
*	)
 
*/

	.balign 16,0x90
FUNC_LABEL(mmuPro36PdbrSet)
	pushfl				/* save EFLAGS */
	cli				/* LOCK INTERRUPT */
	movl	SP_ARG1+4(%esp),%eax
	movl	(%eax),%eax
	movl	%cr3,%edx
	movl	$0xfffff000,%ecx	/* upper 20 bits */
	andl	$0x00000fff,%edx
	cmpl	$ X86CPU_386,FUNC(sysProcessor)
	je	mmuPro36PdbrSet1
	movl	$0xffffffe0,%ecx	/* upper 27 bits */
	andl	$0x00000007,%edx	/* PCD=0 PWT=0 */
mmuPro36PdbrSet1:
	andl	%ecx,%eax
	orl	%edx,%eax
	movl	%eax,%cr3
	jmp	mmuPro36PdbrSet0	/* flush prefetch queue */
mmuPro36PdbrSet0:
	popfl				/* UNLOCK INTERRUPT */
	ret

/*******************************************************************************
*
* mmuPro36PdbrGet - Get Page Directory Base Register
*
* This routine Get Page Directory Base Register.
*
* NOMANUAL

* MMU_TRANS_TBL * mmuPro36PdbrGet (void)
 
*/

	.balign 16,0x90
FUNC_LABEL(mmuPro36PdbrGet)
	movl	%cr3,%eax
	movl	$0xfffff000,%edx	/* upper 20 bits */
	cmpl	$ X86CPU_386,FUNC(sysProcessor)
	je	mmuPro36PdbrGet1
	movl	$0xffffffe0,%edx	/* upper 27 bits */
mmuPro36PdbrGet1:
	andl	%edx,%eax
	ret

/******************************************************************************
*
* mmuPro36TLBFlush - flush the Translation Lookaside Buffer.
*
* NOMANUAL

* void mmuPro36TLBFlush (void)

*/

	.balign 16,0x90
FUNC_LABEL(mmuPro36TLBFlush)
	pushfl				/* save EFLAGS */
	cli				/* LOCK INTERRUPT */
	movl	%cr3,%eax
	movl	%eax,%cr3
	jmp	mmuPro36TLBFlush0	/* flush prefetch queue */

mmuPro36TLBFlush0:
	popfl				/* UNLOCK INTERRUPT */
	ret

