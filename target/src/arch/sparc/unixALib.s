/* unixALib.s - network assembly language routines */

/* Copyright 1984-1992 Wind River Systems, Inc. */
	.data
	.globl  _copyright_wind_river
	.long   _copyright_wind_river

/*
modification history
--------------------
01o,17jun92,jwt  using PORTABLE version because this code no longer works.
01n,26may92,rrr  the tree shuffle
01m,04oct91,rrr  passed through the ansification filter
		  -changed ASMLANGUAGE to _ASMLANGUAGE
		  -changed copyright notice
01l,07sep89,gae  cleanup.
01k,02sep88,ecs  SPARC version.
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

#define	unixALib_PORTABLE
#ifndef	unixALib_PORTABLE

#define _ASMLANGUAGE

	/* internals */

	.global	_cksum
	.global	__insque
	.global	__remque

	.seg	"text"
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

_cksum:	tst	%o2			/* length of string */
	be	ck9			/* zero length string, return */
	nop

	btst	1, %o3			/* sumlen odd? */
	bne	ck4			/* yes, goto ck4 */
	nop

	/* sum is word aligned, don't swap bytes */
	btst	1, %o1			/* address odd? */
	be	ck1			/* no, skip ahead */
	nop

	/* odd start address, do first byte, and then swap bytes */
	dec	%o2			/* --len */
	ldub	[%o1], %o4		/* get 1st byte */
	sll	%o4, 8, %o4		/* put it in upper byte */
	add	%o4, %o0, %o0		/* add to sum */
	ba	ck5
	inc	%o1			/* point at next byte */

	/* even address, sum aligned */
ck1:	mov	%o2, %o5
	ba	ck3
	nop

ck2:	lduh	[%o1], %o4		/* get half word from string */
	addx	%o4, %o0, %o0		/* add to sum */
ck3:	deccc	2, %o5			/* length -= 2 bytes */
	bne	ck2
	inc	2, %o1			/* point at next half word */

	btst	1, %o2			/* len param even? */
	be	ck8			/* yes, skip ahead */
	nop

	/* odd length, take care of last byte */
	ldub	[%o1], %o4		/* get last byte */
	sll	%o4, 8, %o4		/* put it in upper byte */
	addx	%o4, %o0, %o0		/* add to sum */
	ba	ck8
	nop

	/* sum is not word aligned, swap bytes */
ck4:	btst	1, %o1			/* address odd? */
	be	ck5			/* no, goto ck5 */
	nop

	/* odd start address, do first byte, and then don't swap bytes */
	dec	%o2			/* --len */
	ldub	[%o1], %o4		/* get 1st byte */
	add	%o4, %o0, %o0		/* add to sum */
	ba	ck1
	nop

	/* even address, sum misaligned, swap bytes */
ck5:	mov	%o2, %o5
	ba	ck7
	nop

ck6:	lduh	[%o1], %o4		/* get half word from string */
	srl	%o4, 8, %o3		/* lower byte of o3 gets upper byte */
	and	%o4, 0xff, %o4		/* mask off upper byte */
	sll	%o4, 8, %o4		/* lower byte now in upper byte */
	or	%o4, %o3, %o4		/* bytes are now switched */
	addx	%o4, %o0, %o0		/* add to sum */
ck7:	deccc	2, %o5			/* length -= 2 bytes */
	bne	ck6
	inc	2, %o1			/* point at next half word */

	btst	1, %o2			/* length param even? */
	be	ck8			/* yes, goto ck8 */
	nop

	/* odd length, take care of last byte */
	ldub	[%o1], %o4		/* get last byte */
	addx	%o4, %o0, %o0		/* add to sum */

	/* fold in last carry bit */
ck8:	srl	%o0, 16, %o4		/* get upper half word of sum */
	set	0xffff, %o5
	and	%o0, %o5, %o0		/* clear upper half word of sum */
	addx	%o4, %o0, %o0		/* add in upper half word overflow */
	addx	%g0, %o0, %o0		/* add in carry */

ck9:	retl
	nop

/*******************************************************************************
*
* insque - insert a node into a linked list
*
*/

__insque:
	ld	[%o1], %o2	/* o2 = pred->q_forw */
	st	%o2, [%o0]	/* elem->q_forw = pred->q_forw */
	st	%o0, [%o1]	/* pred->q_forw = elem */
	st	%o1, [%o0 + 4]	/* elem->q_back = pred */
	retl
	st	%o0, [%o2 + 4]	/* elem->q_forw->q_back = elem */

/*******************************************************************************
*
* remque - delete a node from a linked list
*
*/

__remque:
	ld	[%o0], %o1	/* o1 = elem->q_forw */
	ld	[%o0 + 4], %o2	/* o2 = elem->q_back */
	st	%o1, [%o2]	/* elem->q_back->q_forw = elem->q_forw */
	retl
	st	%o2, [%o1 + 4]	/* elem->q_forw->q_back = elem->q_back */

#endif	/* unixALib_PORTABLE */
