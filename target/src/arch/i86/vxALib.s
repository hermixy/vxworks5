/* vxALib.s - i80x86 miscellaneous assembly routines */

/* Copyright 1984-2001 Wind River Systems, Inc. */

/*
modification history 
--------------------
01f,20nov01,hdn  doc clean up for 5.5
01e,01nov01,hdn  added vxDr[SG]et(), vxTss[GS]et(), vx[GIL]dtGet()
01d,21aug01,hdn  added FUNC/FUNC_LABEL, replaced .align with .balign
01c,01jun93,hdn  updated to 5.1.
		  - fixed #else and #endif
		  - changed VOID to void
		  - changed ASMLANGUAGE to _ASMLANGUAGE
		  - changed copyright notice
01a,03mar92,hdn  written based on TRON, 68k version.
*/

/*
DESCRIPTION
This module contains miscellaneous VxWorks support routines for the
I80x86 family of processors.

SEE ALSO: vxLib
*/

#define _ASMLANGUAGE

#include "vxWorks.h"
#include "asm.h"


	.data
	.globl	FUNC(copyright_wind_river)
	.long	FUNC(copyright_wind_river)


	/* externals */


	/* internals */

#if (CPU != SIMNT)
	.globl GTEXT(vxMemProbeSup)
	.globl GTEXT(vxMemProbeTrap)
	.globl GTEXT(vxTas)		
	.globl GTEXT(vxCr0Get)		
	.globl GTEXT(vxCr0Set)		
	.globl GTEXT(vxCr2Get)		
	.globl GTEXT(vxCr2Set)		
	.globl GTEXT(vxCr3Get)		
	.globl GTEXT(vxCr3Set)		
	.globl GTEXT(vxCr4Get)		
	.globl GTEXT(vxCr4Set)		
	.globl GTEXT(vxEflagsGet)		
	.globl GTEXT(vxEflagsSet)		
	.globl GTEXT(vxDrGet)		
	.globl GTEXT(vxDrSet)		
	.globl GTEXT(vxTssGet)		
	.globl GTEXT(vxTssSet)		
	.globl GTEXT(vxGdtrGet)		
	.globl GTEXT(vxIdtrGet)		
	.globl GTEXT(vxLdtrGet)		
#endif /* CPU != SIMNT */


	.text
	.balign 16

#if (CPU != SIMNT)
/*******************************************************************************
*
* vxMemProbeSup - vxMemProbe support routine
*
* This routine is called to try to read byte, word, or long, as specified
* by length, from the specified source to the specified destination.
*
* NOMANUAL

* STATUS vxMemProbeSup (length, src, dest)
*     int length;	/* length of cell to test (1, 2, 4) *
*     char *src;	/* address to read *
*     char *dest;	/* address to write *

*/

FUNC_LABEL(vxMemProbeSup)
	pushl	%ebp
	movl	%esp,%ebp

	pushl	%ebx		/* save non-volatile registers */
	pushl	%esi
	pushl	%edi

	movl	ARG2(%ebp),%esi	/* get source address */
	movl	ARG3(%ebp),%edi	/* get destination address */

	xorl	%eax,%eax	/* preset status = OK */

	movl	ARG1(%ebp),%edx	/* get length */
	cmpl	$1,%edx
	jne	vmp10
	movb	(%esi),%bl	/* read  byte */
	movb	%bl,(%edi)	/* write byte */
	jmp	vmpRtn

	.balign 16,0x90
vmp10:
	cmp	$2,%edx
	jne	vmp20
	movw	(%esi),%bx	/* read  word */
	movw	%bx,(%edi)	/* write word */
	jmp	vmpRtn

	.balign 16,0x90
vmp20:
	movl	(%esi),%ebx	/* read  long */
	movl	%ebx,(%edi)	/* write long */

	/* NOTE: vmpRtn is known by vxMemProbTrap to stop retry */
vmpRtn:
	popl	%edi		/* restore non-volatile registers */
	popl	%esi
	popl	%ebx

	leave
	ret

/*******************************************************************************
*
* vxMemProbeTrap - vxMemProbe support routine
*
* This entry point is momentarily attached to the bus error exception vector.
* It simply sets %eax to ERROR to indicate that
* the general protection fault did occur, and returns from the interrupt.
*
* NOTE:
* The instruction that caused the general protection fault must not be run 
* again so we have to set some special bits in the exception stack frame.
*
* NOMANUAL
*
* void vxMemProbeTrap()
*/

	.balign 16,0x90
FUNC_LABEL(vxMemProbeTrap)	/* we get here via the general prot. fault */

	addl	$8,%esp
	pushl	$vmpRtn		/* patch return address, EIP */
	movl	$-1,%eax	/* set status to ERROR */
	iret			/* return to the subroutine */

/*******************************************************************************
* 
* vxTas - C-callable atomic test-and-set primitive
*
* This routine provides a C-callable interface to the test-and-set
* instruction.  The LOCK-BTS instruction is executed on the specified
* address.
*
* RETURNS:
* TRUE if value had been not set, but is now,
* FALSE if the value was set already.

* BOOL vxTas (address)
*     char *address;		/* address to be tested *

*/

	.balign 16,0x90
FUNC_LABEL(vxTas)
	movl	SP_ARG1(%esp),%edx /* get address */
	xorl	%eax,%eax	/* set status to FALSE */
	lock			/* lock the Bus during the next inst */
	bts	$0,(%edx)	/* XXX set MSB with bus-lock */
	jc	vxTas1
	incl	%eax		/* set status to TRUE */
vxTas1:
	ret

/*******************************************************************************
*
* vxCr0Get - get a content of the Control Register 0
*
* This routine gets a content of the Control Register 0. 
*
* RETURNS: a value of the Control Register 0

* int vxCr0Get (void)
 
*/

        .balign 16,0x90
FUNC_LABEL(vxCr0Get)
	movl	%cr0, %eax		/* get CR0 */
	ret

/*******************************************************************************
*
* vxCr0Set - set a value to the Control Register 0
*
* This routine sets a value to the Control Register 0.
*
* RETURNS: N/A

* void vxCr0Set (value)
*       int value;			/@ CR0 value @/
 
*/

        .balign 16,0x90
FUNC_LABEL(vxCr0Set)
	movl	SP_ARG1(%esp), %eax
	movl	%eax, %cr0		/* set CR0 */
	ret

/*******************************************************************************
*
* vxCr2Get - get a content of the Control Register 2
*
* This routine gets a content of the Control Register 2. 
*
* RETURNS: a value of the Control Register 2

* int vxCr2Get (void)
 
*/

        .balign 16,0x90
FUNC_LABEL(vxCr2Get)
	movl	%cr2, %eax		/* get CR2 */
	ret

/*******************************************************************************
*
* vxCr2Set - set a value to the Control Register 2
*
* This routine sets a value to the Control Register 2.
*
* RETURNS: N/A

* void vxCr2Set (value)
*       int value;			/@ CR2 value @/
 
*/

        .balign 16,0x90
FUNC_LABEL(vxCr2Set)
	movl	SP_ARG1(%esp), %eax
	movl	%eax, %cr2		/* set CR2 */
	ret

/*******************************************************************************
*
* vxCr3Get - get a content of the Control Register 3
*
* This routine gets a content of the Control Register 3. 
*
* RETURNS: a value of the Control Register 3

* int vxCr3Get (void)
 
*/

        .balign 16,0x90
FUNC_LABEL(vxCr3Get)
	movl	%cr3, %eax		/* get CR3 */
	ret

/*******************************************************************************
*
* vxCr3Set - set a value to the Control Register 3
*
* This routine sets a value to the Control Register 3.
*
* RETURNS: N/A

* void vxCr3Set (value)
*       int value;			/@ CR3 value @/
 
*/

        .balign 16,0x90
FUNC_LABEL(vxCr3Set)
	movl	SP_ARG1(%esp), %eax
	movl	%eax, %cr3		/* set CR3 */
	ret

/*******************************************************************************
*
* vxCr4Get - get a content of the Control Register 4
*
* This routine gets a content of the Control Register 4. 
*
* RETURNS: a value of the Control Register 4

* int vxCr4Get (void)
 
*/

        .balign 16,0x90
FUNC_LABEL(vxCr4Get)
	movl	%cr4, %eax		/* get CR4 */
	ret

/*******************************************************************************
*
* vxCr4Set - set a value to the Control Register 4
*
* This routine sets a value to the Control Register 4.
*
* RETURNS: N/A

* void vxCr4Set (value)
*       int value;			/@ CR4 value @/
 
*/

        .balign 16,0x90
FUNC_LABEL(vxCr4Set)
	movl	SP_ARG1(%esp), %eax
	movl	%eax, %cr4		/* set CR4 */
	ret

/*******************************************************************************
*
* vxEflagsGet - get a content of the EFLAGS register
*
* This routine gets a content of the EFLAGS register
*
* RETURNS: a value of the EFLAGS register

* int vxEflagsGet (void)
 
*/

        .balign 16,0x90
FUNC_LABEL(vxEflagsGet)
	pushfl	
	popl	%eax			/* get EFLAGS */
	ret

/*******************************************************************************
*
* vxEflagsSet - set a value to the EFLAGS register
*
* This routine sets a value to the EFLAGS register
*
* RETURNS: N/A

* void vxEflagsSet (value)
*       int value;			/@ EFLAGS value @/
 
*/

        .balign 16,0x90
FUNC_LABEL(vxEflagsSet)
	pushl	SP_ARG1(%esp)
	popfl				/* set EFLAGS */
	ret

/*******************************************************************************
*
* vxDrGet - get a content of the Debug Register 0 to 7
*
* SYNOPSIS
* \ss
* void vxDrGet (pDr0, pDr1, pDr2, pDr3, pDr4, pDr5, pDr6, pDr7)
*       int * pDr0;			/@ DR0 @/
*       int * pDr1;			/@ DR1 @/
*       int * pDr2;			/@ DR2 @/
*       int * pDr3;			/@ DR3 @/
*       int * pDr4;			/@ DR4 @/
*       int * pDr5;			/@ DR5 @/
*       int * pDr6;			/@ DR6 @/
*       int * pDr7;			/@ DR7 @/
* \se
*
* This routine gets a content of the Debug Register 0 to 7. 
*
* RETURNS: N/A
*/

        .balign 16,0x90
FUNC_LABEL(vxDrGet)
	movl	SP_ARG1(%esp), %edx
	movl	%dr0, %eax
	movl	%eax, (%edx)
	movl	SP_ARG2(%esp), %edx
	movl	%dr1, %eax
	movl	%eax, (%edx)
	movl	SP_ARG3(%esp), %edx
	movl	%dr2, %eax
	movl	%eax, (%edx)
	movl	SP_ARG4(%esp), %edx
	movl	%dr3, %eax
	movl	%eax, (%edx)
	movl	SP_ARG5(%esp), %edx
	movl	%dr4, %eax
	movl	%eax, (%edx)
	movl	SP_ARG6(%esp), %edx
	movl	%dr5, %eax
	movl	%eax, (%edx)
	movl	SP_ARG7(%esp), %edx
	movl	%dr6, %eax
	movl	%eax, (%edx)
	movl	SP_ARG8(%esp), %edx
	movl	%dr7, %eax
	movl	%eax, (%edx)
	ret

/*******************************************************************************
*
* vxDrSet - set a value to the Debug Register 0 to 7
*
* SYNOPSIS
* \ss
* void vxDrSet (dr0, dr1, dr2, dr3, dr4, dr5, dr6, dr7)
*       int dr0;			/@ DR0 @/
*       int dr1;			/@ DR1 @/
*       int dr2;			/@ DR2 @/
*       int dr3;			/@ DR3 @/
*       int dr4;			/@ DR4 @/
*       int dr5;			/@ DR5 @/
*       int dr6;			/@ DR6 @/
*       int dr7;			/@ DR7 @/
* \se
*
* This routine sets a value to the Debug Register 0 to 7. 
*
* RETURNS: N/A
*/

        .balign 16,0x90
FUNC_LABEL(vxDrSet)
	movl	SP_ARG1(%esp), %eax
	movl	%eax, %dr0
	movl	SP_ARG2(%esp), %eax
	movl	%eax, %dr1
	movl	SP_ARG3(%esp), %eax
	movl	%eax, %dr2
	movl	SP_ARG4(%esp), %eax
	movl	%eax, %dr3
	movl	SP_ARG5(%esp), %eax
	movl	%eax, %dr4
	movl	SP_ARG6(%esp), %eax
	movl	%eax, %dr5
	movl	SP_ARG7(%esp), %eax
	movl	%eax, %dr6
	movl	SP_ARG8(%esp), %eax
	movl	%eax, %dr7
	ret

/*******************************************************************************
*
* vxTssGet - get a content of the TASK register
*
* This routine gets a content of the TASK register
*
* RETURNS: a value of the TASK register

* int vxTssGet (void)
 
*/

        .balign 16,0x90
FUNC_LABEL(vxTssGet)
	xorl	%eax, %eax
	str	%ax			/* get a value of the TASK register */
	ret

/*******************************************************************************
*
* vxTssSet - set a value to the TASK register
*
* This routine sets a value to the TASK register
*
* RETURNS: N/A

* void vxTssSet (value)
*       int value;			/@ TASK register value @/
 
*/

        .balign 16,0x90
FUNC_LABEL(vxTssSet)
	movl	SP_ARG1(%esp), %eax
	ltr	%ax			/* set it to TASK register */
	ret

/*******************************************************************************
*
* vxGdtrGet - get a content of the Global Descriptor Table Register
*
* This routine gets a content of the Global Descriptor Table Register
*
* RETURNS: N/A

* void vxGdtrGet (pGdtr)
*     long long int * pGdtr;		/@ memory to store GDTR @/
 
*/

        .balign 16,0x90
FUNC_LABEL(vxGdtrGet)
	movl	SP_ARG1(%esp), %eax
	sgdt	(%eax)			/* get a value of the GDTR */
	ret

/*******************************************************************************
*
* vxIdtrGet - get a content of the Interrupt Descriptor Table Register
*
* This routine gets a content of the Interrupt Descriptor Table Register
*
* RETURNS: N/A

* void vxIdtrGet (pIdtr)
*     long long int * pIdtr;		/@ memory to store IDTR @/
 
*/

        .balign 16,0x90
FUNC_LABEL(vxIdtrGet)
	movl	SP_ARG1(%esp), %eax
	sidt	(%eax)			/* get a value of the IDTR */
	ret

/*******************************************************************************
*
* vxLdtrGet - get a content of the Local Descriptor Table Register
*
* This routine gets a content of the Local Descriptor Table Register
*
* RETURNS: N/A

* void vxLdtrGet (pLdtr)
*     long long int * pLdtr;		/@ memory to store LDTR @/
 
*/

        .balign 16,0x90
FUNC_LABEL(vxLdtrGet)
	movl	SP_ARG1(%esp), %eax
	sldt	(%eax)			/* get a value of the LDTR */
	ret

#endif /* CPU != SIMNT */

