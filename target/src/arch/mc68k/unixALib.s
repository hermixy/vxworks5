/* unixALib.s - network assembly language routines */

/* Copyright 1984-1994 Wind River Systems, Inc. */
	.data
	.globl	_copyright_wind_river
	.long	_copyright_wind_river

/*
modification history
--------------------
01v,31oct94,tmk  added MC68LC040 support
01u,30may94,tpr  added MC68060 cpu support.
01t,23aug92,jcf  changed bxxx to jxx.
01s,26may92,rrr  the tree shuffle
01r,04oct91,rrr  passed through the ansification filter
		  -fixed #else and #endif
		  -changed ASMLANGUAGE to _ASMLANGUAGE
		  -changed copyright notice
01q,25sep91,yao  added support for CPU32.
01p,19jun91,elh  changed jcc to jcc  to be more portable.
01o,29jan91,elh  fixed cksum to clear extended carry for odd previous sum.
01n,01oct90,dab  changed conditional compilation identifier from
		    HOST_SUN to AS_WORKS_WELL.
01m,12sep90,dab  changed jmp instruction in _cksum() to .word to make
	   +lpf    non-SUN hosts happy.
01l,14jul90,jcf  fixed cksum from clobbering non-volatile d3
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

/* optimized version available for 68020, 68040 and 68060 */

#if (defined(PORTABLE) || (CPU!=MC68020) && (CPU!=MC68040) && \
     (CPU!=MC68060) && (CPU!=MC68LC040))
    
#define unixALib_PORTABLE
#endif

#ifndef unixALib_PORTABLE
#define _ASMLANGUAGE

	/* internals */

	.globl	_cksum
	.globl  __insque
	.globl  __remque

	.text
	.even

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
_cksum:
	movl    d2,sp@-
	movl    d3,sp@-
	movl    sp@(12),d0   	/* sum */
	movl    sp@(16),a0	/* addr */
	movl    sp@(20),d1	/* len */
	btst	#0,sp@(24+3)	/* previously summed len */
	jeq	ckEvenSum
	clrl	d2		/* odd number of bytes summed previously */
	andb    #0xf,cc		/* Clear X (extended carry flag) */
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
/* The jump instruction contained in the following conditional
 * compilation executes a relative jump to one of the instructions
 * between the labels ckInLoop and ckStartLoop based on the number
 * of instructions between the labels.  Since the .word hardcodes
 * this difference for non-SUN hosts, any change to the number or
 * nature of instructions between the labels must be reflected
 * in a recalculation of the binary values defined by the .word.
 */
#if	defined(AS_WORKS_WELL)
	jmp     pc@(ckStartLoop-.-2:b,d2)   /* Jump into loop */
#else
	.word   0x4efb,0x2842		    /* Jump into loop */
#endif	/* AS_WORKS_WELL */
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
	jne     ckNextWord
	btst    #0,d3		/* another byte to add? */
	jne     ckNextByte
	jra     ckGetout
ckNextWord:
	movw    a0@+,d2
	addxw   d2,d0
	btst    #0,d3		/* another byte to add? */
	jeq     ckGetout
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
	movl    sp@+,d3
	movl    sp@+,d2
	rts

/****************************************************************************
*
* insque - insert a node into a linked list
*
*/
__insque:
	movel	sp@(4),a0		/* new */
	movel	sp@(8),a1		/* pred */
	movel	a1@,d0			/* succ */
	movel	d0,a0@
	movel	a1,a0@(4)
	movel	a0,a1@
	movel	d0,a1
	movel	a0,a1@(4)
	rts

/****************************************************************************
*
* remque - delete a node from a linked list
*
*/
__remque:
	movel	d0,d1
	movel	sp@(4),a0
	movel	a0@,a1
	movel	a0@(4),d0
	movel	d0,a1@(4)
	exg	d0,a0
	movel	a1,a0@
	rts
#endif	/* !unixALib_PORTABLE */
