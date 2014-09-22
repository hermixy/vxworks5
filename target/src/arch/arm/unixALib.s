/* unixALib.s - Assembler optimised UNIX kernel compatability library */

/* Copyright 1996-2001 Wind River Systems, Inc. */

/*
modification history
--------------------
01h,17oct01,t_m  convert to FUNC_LABEL:
01g,11oct01,jb  Enabling removal of pre-pended underscores for new compilers
                 (Diab/Gnu elf)
01f,21feb01,cdp  fix cksum loss of carry into high-byte (SPR #29703). Comment.
01e,08jul98,cdp  added preliminary big-endian support.
01d,10mar98,jpd  layout tidying.
01c,27oct97,kkk  took out "***EOF***" line from end of file.
01b,23may97,jpd  Amalgamated into VxWorks.
01a,16jul96,ams  written.
*/

/*
DESCRIPTION
This library provides routines that simulate or replace Unix kernel functions
that are used in the network code.
*/

#define _ASMLANGUAGE
#include "vxWorks.h"
#include "asm.h"

	.data
	.globl	FUNC(copyright_wind_river)
	.long	FUNC(copyright_wind_river)

#if (defined(PORTABLE))
#define unixALib_PORTABLE
#endif

#ifndef unixALib_PORTABLE

/* Exports */

	.global	FUNC(cksum)		/* checksum routine */
	.global	FUNC(_insque)	/* insert node in queue after specified node */
	.global	FUNC(_remque)	/* remove specified node */

	.text
	.balign	4

/*******************************************************************************
*
* cksum - compute check sum
*
* return 16bit sum of 'sum' and the 16bit sum of the string 'string' of byte
* length 'len'. Complicated by 'len' or 'sumlen' not being even.
* Note, the one's complement is handled by the caller.
*
* RETURNS: the checksum caclulated
*
* NOMANUAL
*
* int cksum
*	(
*	int	sum;
*	char *	string;
*	int	len;
*	int	sumlen;
*	)
*/

FUNC_LABEL(cksum)

#ifdef STACK_FRAMES
	mov	ip, sp
	stmdb	sp!, {fp, ip, lr, pc}
	sub	fp, ip, #4
#endif /* STACK_FRAMES */

	cmp	r2, #0

#ifdef STACK_FRAMES
	ldmeqdb	fp, {fp, sp, pc}
#else /* !STACK_FRAMES */
	moveq	pc, lr			/* Capture 0 */

	stmdb	sp!, {lr}		/* Keep return address */
#endif /* STACK_FRAMES */

	tst	r3, #1			/* Test sumlen for odd count */
	beq	L$_Even_Byte

	ldrb	r3, [r1], #1		/* Byte left over from last call */
#if (_BYTE_ORDER == _BIG_ENDIAN)
	add	r0, r0, r3
#else
	add	r0, r0, r3, lsl #8	/* Add upper byte */
#endif
	subs	r2, r2, #1		/* One less byte */
	beq	L$_All_Done

L$_Even_Byte:
	/* Now try to align to a word boundary */

	tst	r1, #3			/* Aligned ? */
	moveq	lr, #0			/* No offset */
	beq	L$_Word_Align

	ldr	r3, [r1, #0]		/* Load word */

	/*
	 * The following may help with understanding of the next bit.
	 * If the bytes in memory from address a contain 11 22 33 44
	 * LDR from a+0 => 44332211 (little-endian) 11223344 (big-endian)
	 *          a+1 => 11443322                 44112233
	 *          a+2 => 22114433                 33441122
	 *          a+3 => 33221144                 22334411
	 */

	and	ip, r1, #3		/* Get no of bytes we need from r3 */
	rsb	ip, ip, #4		/* The number of bytes we can get */
	cmp	r2, ip			/* Have == Got */
	movlt	ip, r2			/* Got == Have */

#if (_BYTE_ORDER == _BIG_ENDIAN)
	tsts	r1, #1
	movne	r3, r3, ror #16		/* move addressed byte to b31..24 */
	cmps	ip, #2
	mov	lr, #0xff000000		/* At least one byte */
	orrge	lr, lr, lr, lsr #8	/* The 2nd byte */
	orrgt	lr, lr, lr, lsr #8	/* The third byte */
	and	r3, r3, lr		/* Mask out bytes we do not want */

	/*
	 * r3 contains up to 3 bytes to add, in highest 3 bytes of register
	 * This can cause carry (e.g 0xffff + 0xffffff00).
	 */

	adds	r0, r0, r3		/* Add bytes */
	adc	r0, r0, #0		/* add in any possible carry */
#else
	cmp	ip, #2
	mov	lr, #0xff		/* At least one byte */
	orrge	lr, lr, lr, lsl #8	/* The 2nd byte */
	orrgt	lr, lr, lr, lsl #8	/* The third byte */
	and	r3, r3, lr		/* Mask out bytes we do not want */

	/*
	 * r3 contains up to 3 bytes to add, in lowest 3 bytes of register
	 * This cannot cause carry.
	 */

	add	r0, r0, r3		/* Add bytes */
#endif

	/*
	 * Check if rotation of checksum required now so that we do the
	 * addition of all the next words the right way round. If we do rotate,
	 * save the rotation in lr so we can undo it at the end.
	 */

	tst	r1, #1			/* Odd alignment */
	moveq	lr, #0			/* No offset */
	movne	lr, #8			/* Offset from different base */

	/*
	 * SPR #29703 (T3 SPR #64293): above addition was of a one, two or
	 * three byte number to a 16-bit sum and may have caused overflow into
	 * the fourth (highest) byte. Original (little-endian) code did an LSL
	 * here instead of ROR, incorrectly discarding such overflow. New code
	 * is same as big-endian code: it preserves the high byte. (Unimportant
	 * whether we shift 8 or 24 bits.)
	 */

	movne	r0, r0, ror #8		/* Offset, work from a different base */

	subs	r2, r2, ip		/* Less bytes */
	add	r1, r1, ip		/* Move the pointer */
	beq	L$_Boundary_Done	/* We know we will shift */

	/* Now we are word aligned */
L$_Word_Align:
	movs	ip, r2, lsr #2		/* How many words can we add */
	beq	L$_Left_123		/* No complete words left */
	sub	r2, r2, ip, lsl #2	/* Take number of words off total */
	adds	r0, r0, #0		/* Clear carry */

	ands	r3, ip, #0x0f		/* Last 16 ? (Does not clear carry) */


	/* Compute offset to start with in the loop */

	rsbne	r3, r3, #16		/* Where to start */
	addne	r3, pc, r3, lsl #3	/* Where to jump */
	movne	pc, r3			/* And jump */
0:
	ldr	r3, [r1], #4		/* 1 */
	adds	r0, r0, r3		/* Carry already added */
	ldr	r3, [r1], #4		/* 2 */
	adcs	r0, r0, r3
	ldr	r3, [r1], #4		/* 3 */
	adcs	r0, r0, r3
	ldr	r3, [r1], #4		/* 4 */
	adcs	r0, r0, r3
	ldr	r3, [r1], #4		/* 5 */
	adcs	r0, r0, r3
	ldr	r3, [r1], #4		/* 6 */
	adcs	r0, r0, r3
	ldr	r3, [r1], #4		/* 7 */
	adcs	r0, r0, r3
	ldr	r3, [r1], #4		/* 8 */
	adcs	r0, r0, r3
	ldr	r3, [r1], #4		/* 9 */
	adcs	r0, r0, r3
	ldr	r3, [r1], #4		/* 10 */
	adcs	r0, r0, r3
	ldr	r3, [r1], #4		/* 11 */
	adcs	r0, r0, r3
	ldr	r3, [r1], #4		/* 12 */
	adcs	r0, r0, r3
	ldr	r3, [r1], #4		/* 13 */
	adcs	r0, r0, r3
	ldr	r3, [r1], #4		/* 14 */
	adcs	r0, r0, r3
	ldr	r3, [r1], #4		/* 15 */
	adcs	r0, r0, r3
	ldr	r3, [r1], #4		/* 16 */
	adcs	r0, r0, r3

	adc	r0, r0, #0		/* Add Carry */
	subs	ip, ip, #16		/* 16 words less ! */
	bgt	0b


	/* Main loop complete, less than 4 bytes left */

	cmp	r2, #0
	beq	L$_Boundary_Done	/* Completed on a word boundary */


	/* 1, 2 or 3 bytes left */

L$_Left_123:

	/* Completed on an odd boundary, do we shift the sum back ? */

	mov	r0, r0, ror lr		/* Shift total */

	/* Extract the last byte/s */

#if (_BYTE_ORDER == _BIG_ENDIAN)
	cmp	r2, #2
	mov	r2, #0xff000000		/* At least one byte */
	orrge	r2, r2, r2, lsr #8	/* The 2nd byte */
	orrgt	r2, r2, r2, lsr #8	/* The third byte */
	ldr	r3, [r1, #0]		/* Load word */
	and	r3, r3, r2		/* Mask out bytes we do not want */
	adds	r0, r0, r3, lsr lr	/* Add bytes */
#else
	cmp	r2, #2
	mov	r2, #0xff		/* At least one byte */
	orrge	r2, r2, r2, lsl #8	/* The 2nd byte */
	orrgt	r2, r2, r2, lsl #8	/* The third byte */
	ldr	r3, [r1, #0]		/* Load word */
	and	r3, r3, r2		/* Mask out bytes we do not want */
	adds	r0, r0, r3, lsl lr	/* Add bytes */
#endif
	adc	r0, r0, #0		/* Add carry */

	adds	r0, r0, r0, lsl #16	/* Result in upper 16 bits + C */
	mov	r0, r0, lsr #16		/* Get upper 16 bits */
	adc	r0, r0, #0		/* Add carry */

#ifdef STACK_FRAMES
	ldmdb	fp, {fp, sp, pc}	/* Return (sum is in r0) */
#else /* !STACK_FRAMES */
	ldmia	sp!, {pc}		/* Return (sum is in r0) */
#endif /* STACK_FRAMES */


	/* Completed on a boundary */

L$_Boundary_Done:
	mov	r0, r0, ror lr		/* Shift total back 8 */

L$_All_Done:
	adds	r0, r0, r0, lsl #16	/* Result in upper 16 bits + C */
	mov	r0, r0, lsr #16		/* Get upper 16 bits */
	adc	r0, r0, #0		/* Add carry */

#ifdef STACK_FRAMES
	ldmdb	fp, {fp, sp, pc}	/* Return (sum is in r0) */
#else /* !STACK_FRAMES */
	ldmia	sp!, {pc}		/* Return (sum is in r0) */
#endif /* STACK_FRAMES */

/*******************************************************************************
*
* _insque - insert node in list after specified node.
*
* RETURNS: N/A
*
* NOMANUAL
*
* void _insque
*	(
*	NODE *	pNode,
*	NODE *	pPrev
*	)
*/

FUNC_LABEL(_insque)

#ifdef STACK_FRAMES
	mov	ip, sp
	stmdb	sp!, {fp, ip, lr, pc}
	sub	fp, ip, #4
#endif /* STACK_FRAMES */

	ldr	r3, [r1, #0]		/* r3 = pPrev->next */
	str	r0, [r1, #0]		/* pPrev->next = pNode */
	str	r0, [r3, #4]		/* r3->previous = pNode */
	str	r3, [r0, #0]		/* pNode->next = r3 */
	str	r1, [r0, #4]		/* pNode->previous = pPrev */

	/* Done */
#ifdef STACK_FRAMES
	ldmdb	fp, {fp, sp, pc}
#else /* !STACK_FRAMES */
	mov	pc, lr
#endif /* STACK_FRAMES */


/*******************************************************************************
*
* _remque - remove specified node in list.
*
* RETURNS: N/A
*
* NOMANUAL
*
* void _remque
*	(
*	NODE *	pNode
*	)
*/

FUNC_LABEL(_remque)

#ifdef STACK_FRAMES
	mov	ip, sp
	stmdb	sp!, {fp, ip, lr, pc}
	sub	fp, ip, #4
#endif /* STACK_FRAMES */

	/* Setup */

	ldr	r2, [r0, #4]		/* r2 = pNode->previous */
	ldr	r3, [r0, #0]		/* r3 = pNode->next */

	/*  pNode->previous->next = pNode->next */

	str	r3, [r2, #0]		/* r2->next = r3 */

	/* pNode->next->previous = pNode->previous */

	str	r2, [r3, #4]


#ifdef STACK_FRAMES
	ldmdb	fp, {fp, sp, pc}
#else /* !STACK_FRAMES */
	mov	pc, lr
#endif /* STACK_FRAMES */

#endif /* ! unixALib_PORTABLE */
