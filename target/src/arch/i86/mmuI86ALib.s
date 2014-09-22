/* mmuI86ALib.s - MMU library for i86 */

/* Copyright 1984-2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01d,07mar02,hdn  removed mmuI86Lock/Unlock declaration (spr 73358)
01c,23aug01,hdn  added FUNC/FUNC_LABEL, replaced .align with .balign
		 renamed from mmu* to mmuI86*, preserved IF bit in EFLAGS
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

	.globl	VAR(mmuI86Enabled)
	.globl	VAR(sysProcessor)


	/* internals */

	.globl	GTEXT(mmuI86Enable)
	.globl	GTEXT(mmuI86On)
	.globl	GTEXT(mmuI86Off)
	.globl	GTEXT(mmuI86PdbrSet)
	.globl	GTEXT(mmuI86PdbrGet)
	.globl	GTEXT(mmuI86TLBFlush)


	.text
	.balign 16
 
/******************************************************************************
*
* mmuI86Enable - turn mmu on or off
*
* RETURNS: OK

* STATUS mmuI86Enable 
*    (
*    BOOL enable		/@ TRUE to enable, FALSE to disable MMU @/
*    )

*/

FUNC_LABEL(mmuI86Enable)
	pushfl				/* save EFLAGS */
	cli				/* LOCK INTERRUPTS */
	movl	SP_ARG1+4(%esp),%edx
        movl    %cr0,%eax
        movl    %edx,FUNC(mmuI86Enabled)
	cmpl	$0,%edx
	je	mmuI86Disable
	orl     $0x80010000,%eax	/* set PG and WP */
	jmp     mmuI86Enable0

mmuI86Disable:
	andl    $0x7ffeffff,%eax	/* clear PG and WP */

mmuI86Enable0:
	movl    %eax,%cr0
	jmp     mmuI86Enable1		/* flush prefetch queue */
mmuI86Enable1:
	movl	$0,%eax
	popfl				/* UNLOCK INTERRUPTS */
	ret

/******************************************************************************
*
* mmuI86On - turn MMU on 
*
* This routine assumes that interrupts are locked out.  It is called internally
* to enable the mmu after it has been disabled for a short period of time
* to access internal data structs.
*
* NOMANUAL

* void mmuI86On (void)

*/

	.balign 16,0x90
FUNC_LABEL(mmuI86On)
        movl    %cr0,%eax
	orl     $0x80010000,%eax	/* set PG and WP */
	movl    %eax,%cr0
	jmp     mmuI86On0		/* flush prefetch queue */
mmuI86On0:
	ret

/******************************************************************************
*
* mmuI86Off - turn MMU off 
*
* This routine assumes that interrupts are locked out.  It is called internally
* to disable the mmu for a short period of time
* to access internal data structs.
*
* NOMANUAL

* void mmuI86Off (void)

*/

	.balign 16,0x90
FUNC_LABEL(mmuI86Off)
	movl    %cr0,%eax
	andl    $0x7ffeffff,%eax	/* clear PG and WP */
	movl    %eax,%cr0
	jmp     mmuI86Off0		/* flush prefetch queue */
mmuI86Off0:
	ret

/*******************************************************************************
*
* mmuI86PdbrSet - Set Page Directory Base Register
*
* This routine Set Page Directory Base Register.
*
* NOMANUAL

* void mmuI86PdbrSet 
*	(
*	MMU_TRANS_TBL * transTbl;
*	)
 
*/

	.balign 16,0x90
FUNC_LABEL(mmuI86PdbrSet)
	pushfl				/* save EFLAGS */
	cli				/* LOCK INTERRUPTS */
	movl	SP_ARG1+4(%esp),%eax
	movl	(%eax),%eax
	andl	$0xfffff000,%eax
	movl	%cr3,%edx
	andl	$0x00000fff,%edx
	cmpl	$ X86CPU_386,FUNC(sysProcessor)
	je	mmuI86PdbrSet1
	andl	$0x00000fe7,%edx
mmuI86PdbrSet1:
	orl	%edx,%eax
	movl	%eax,%cr3
	jmp	mmuI86PdbrSet0		/* flush prefetch queue */
mmuI86PdbrSet0:
	popfl				/* UNLOCK INTERRUPTS */
	ret

/*******************************************************************************
*
* mmuI86PdbrGet - Get Page Directory Base Register
*
* This routine Get Page Directory Base Register.
*
* NOMANUAL

* MMU_TRANS_TBL * mmuI86PdbrGet (void)
 
*/

	.balign 16,0x90
FUNC_LABEL(mmuI86PdbrGet)
	movl	%cr3,%eax
	andl	$0xfffff000,%eax
	ret

/******************************************************************************
*
* mmuI86TLBFlush - flush the Translation Lookaside Buffer.
*
* NOMANUAL

* void mmuI86TLBFlush (void)

*/

	.balign 16,0x90
FUNC_LABEL(mmuI86TLBFlush)
	pushfl				/* save EFLAGS */
	cli				/* LOCK INTERRUPTS */
	movl	%cr3,%eax
	movl	%eax,%cr3
	jmp	mmuI86TLBFlush0		/* flush prefetch queue */
mmuI86TLBFlush0:
	popfl				/* UNLOCK INTERRUPTS */
	ret

