/* unixALib.s - assembly language routines for tcpip */

/* Copyright 1984-2001 Wind River Systems, Inc. */
	.data
	.globl	copyright_wind_river

/*
 * This file has been developed or significantly modified by the
 * MIPS Center of Excellence Dedicated Engineering Staff.
 * This notice is as per the MIPS Center of Excellence Master Partner
 * Agreement, do not remove this notice without checking first with
 * WR/Platforms MIPS Center of Excellence engineering management.
 */

/*
modification history
--------------------
02g,02aug01,mem  Diab integration
02f,16jul01,ros  add CofE comment
02e,19oct93,cd   forced cksum() to return 0, MIPS uses portable in_cksum().
02d,26may92,rrr  the tree shuffle
02c,04oct91,rrr  passed through the ansification filter
		  -changed ASMLANGUAGE to _ASMLANGUAGE
		  -changed copyright notice
02b,08may90,rtp  modified remque; fixed addu error in cksum; added
		 documentation.
02a,01may90,rtp  converted to MIPS assembly language and added comments
		 to cksum.
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
#include "vxWorks.h"
#include "asm.h"

#define  CARRY_MASK	0x10000		/* for carry on half-word operations */

	/* internals */

	.globl	cksum
	.globl  insque
	.globl  remque


	.text
	.set	reorder

/**************************************************************************
*
* cksum - compute checksum
*
* Optimized checksum routine not implemented for MIPS.
*
* RETURNS: zero.
*
* int cksum (sum, string, len, sumlen);
*
* INTERNAL
* The portable checksum routine, in unixLib.c, is used for MIPS.
*/
	.ent	cksum
cksum:
	move	v0,zero
	j	ra
	.end	cksum

/****************************************************************************
*
* insque - insert a node into a linked list
*
*  	int insque ( q, p )
*		caddr_t	q;
*		caddr_t p;
*
*/
	.ent insque
insque:
	move	t0, a0			/* new  */
	move	t1, a1			/* pred */
	lw	t2, (t1)		/* succ */
	sw	t2, (t0)
	sw	t1, 4(t0)
	sw	t0, (t1)
	move	t1, t2
	sw	t0, 4(t1)
	j	ra
	.end insque


/****************************************************************************
*
* remque - delete a node from a (doubly) linked list
*
*	int remque ( node )
*		caddr_t node;
*/
	.ent  remque
remque:
	move	t0,a0		/* current = node to be deleted  */
	lw	t1,0(t0)	/* next = curent->next           */
	lw	t2,4(t0)	/* previous = current->prev      */
	sw	t2,4(t1)	/* next->prev = previous         */
	lw	v0,0(t0)	/* return value = current node   */
	sw	t1,0(t2)	/* prev->next = next             */
	j 	ra		/* return                        */
	.end  remque
