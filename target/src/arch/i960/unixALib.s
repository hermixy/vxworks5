/* unixALib.s - network assembly language routines */

/* Copyright 1984-1992 Wind River Systems, Inc. */
	.data
	.globl	_copyright_wind_river
	.long	_copyright_wind_river

/*
modification history
--------------------
01n,26may92,rrr  the tree shuffle
01m,04oct91,rrr  passed through the ansification filter
		  -fixed #else and #endif
		  -changed ASMLANGUAGE to _ASMLANGUAGE
		  -changed copyright notice
01l,23apr91,del  made __insque and __remque work w/i960.
01k,31oct89,hjb  recoded cksum() according to Van Jacobson's algorithm
		 described in RFC1071.
01j,22jun88,dnw  removed unnecessary intLock()s in insque() and remque().
		 removed setjmp()/longjmp() to cALib.s.
01i,13feb87,dnw  changed setjmp and longjmp register lists to be processable
		   by Motorola assemblers.
		 added .data before copyright.
01h,13nov87,rdc  added setjmp and longjmp (temporary).
01g,24oct87,dnw  removed erroneous ';' at end of copyright.
01f,03apr87,ecs  added copyright.
01e,02apr87,jlf  removed .globl of tas, left behind after 01d.
01d,27mar86,rdc  moved tas() to vxALib.s for non-network systems.
01c,22dec86,dnw  more changes for assembler compatibility:
		   changed jeq/jne/jra to beq/bne/bra.
		   added .globl declarations of intLock(), intUnlock().
01b,30nov86,dnw  changed to be acceptable to various assemblers:
		   changed ".align 1" to ".even".
		   changed "mov" to "move"
		   replaced <n>[fb] labels with real labels.
01a,06aug86,rdc  written
*/

#define _ASMLANGUAGE

	/* internals */

	.globl	_cksum
	.globl  __insque
	.globl  __remque

	.text
	.align	4

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
	.align	4
_cksum:
#if FALSE
	movl    d2,sp@-
	movl    sp@(8),d0   	/* sum */
	movl    sp@(12),a0	/* addr */
	movl    sp@(16),d1	/* len */
	btst	#0,sp@(20+3)	/* previously summed len */
	beq	ckEvenSum
	clrl	d2		/* odd number of bytes summed previously */
	movb	a0@+,d2		/* add the first byte into our sum */
	addxl	d2,d0		/* before others */
	subl	#1,d1
ckEvenSum:
	movl    d1,d2
	movl    d1,d3
	andl    #0x3,d3		/* count % 4 */
	lsrl    #6,d1		/* count/64 = # loop traversals */
	andl    #0x3c,d2	/* Then find fractions of a chunk */
	negl    d2
	andb    #0xf,cc		/* Clear X (extended carry flag) */
	jmp     pc@(ckStartLoop-.-2:b,d2)   /* Jump into loop */
ckInLoop:				   /* Begin inner loop... */
	movl    a0@+,d2		/*  Fetch 32-bit word */
	addxl   d2,d0		/*    Add word + previous carry */
	movl    a0@+,d2
	addxl   d2,d0
	movl    a0@+,d2
	addxl   d2,d0
	movl    a0@+,d2
	addxl   d2,d0
	movl    a0@+,d2
	addxl   d2,d0
	movl    a0@+,d2
	addxl   d2,d0
	movl    a0@+,d2
	addxl   d2,d0
	movl    a0@+,d2
	addxl   d2,d0
	movl    a0@+,d2
	addxl   d2,d0
	movl    a0@+,d2
	addxl   d2,d0
	movl    a0@+,d2
	addxl   d2,d0
	movl    a0@+,d2
	addxl   d2,d0
	movl    a0@+,d2
	addxl   d2,d0
	movl    a0@+,d2
	addxl   d2,d0
	movl    a0@+,d2
	addxl   d2,d0
	movl    a0@+,d2
	addxl   d2,d0
ckStartLoop:
	dbra    d1,ckInLoop	/* dbra doesn't affect X */
	btst    #1,d3		/* another word to add? */
	bne     ckNextWord
	btst    #0,d3		/* another byte to add? */
	bne     ckNextByte
	bra     ckGetout
ckNextWord:
	movw    a0@+,d2
	addxw   d2,d0
	btst    #0,d3		/* another byte to add? */
	beq     ckGetout
ckNextByte:
	movw    a0@+,d2
	andw    #0xff00,d2
	addxw   d2,d0
ckGetout:
	movl    d0,d1		/* Fold 32 bit sum to 16 bits */
	swap    d1		/* swap doesn't affect X */
	addxw   d1,d0
	jcc     ckNoCarry
	addw    #1,d0
ckNoCarry:
	andl    #0xffff,d0
	movl    sp@+,d2
	rts
#endif	/* FALSE */

/****************************************************************************
*
* insque - insert a node into a linked list
*
*/
	.align	4
__insque:
	ld	(g1), r3
	st	r3, (g0)
	st	g1, 0x04(g0)
	st	g0, (g1)
	st	g0, 0x04(r3)
	ret


/****************************************************************************
*
* remque - delete a node from a linked list
*
*/
	.align	4
__remque:
	ld	(g0), r4
	ld	0x04(g0), r3
	st	r3, 0x04(r4)
	st	r4, (r3)
	ret
