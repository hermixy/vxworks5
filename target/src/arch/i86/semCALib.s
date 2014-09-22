/* semCALib.s - i80x86 internal VxWorks counting semaphore assembly library */

/* Copyright 1984-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01f,20mar02,hdn  preserved previous state of the int enable bit (spr 74016)
01e,09nov01,pcm  added VxWorks semaphore events
01d,22aug01,hdn  added FUNC/FUNC_LABEL, replaced .align with .balign
01c,29jul96,sbs  Made windview conditionally compile.
01b,08aug94,hdn  added support for WindView.
01a,02jun93,hdn  extracted from semALib.s.
*/

/*
DESCRIPTION
This module contains internals to the VxWorks kernel.
These routines have been coded in assembler because they are optimized for
performance.
*/

#define _ASMLANGUAGE
#include "vxWorks.h"
#include "asm.h"
#include "private/taskLibP.h"
#include "private/semLibP.h"


	.data
	.globl	FUNC(copyright_wind_river)
	.long	FUNC(copyright_wind_river)


#ifndef PORTABLE

	/* externals */

	.globl	FUNC(semIntRestrict)
	.globl	FUNC(semInvalid)
	.globl	FUNC(semQGet)
	.globl	FUNC(semQPut)
	.globl	FUNC(semEvRsrcSend)


	/* internals */

	.globl	GTEXT(semCGive)		/* optimized counting semaphore give */
	.globl	GTEXT(semCTake)		/* optimized counting semaphore take */


	.text
	.balign 16

/*******************************************************************************
*
* semIsInvalid - unlock interrupts and call semInvalid ().
*/

semIsInvalidUnlock:
	popfl				/* UNLOCK INTERRUPTS */

semIsInvalid:
	jmp	FUNC(semInvalid)	/* let C rtn do work and ret */

/*******************************************************************************
*
* semCGive - optimized give of a counting semaphore
*

* STATUS semCGive
*    (
*    SEM_ID semId		/@ semaphore id to give @/
*    )

* INTERNAL
* assumptions are:
*  - %ecx = semId
*  - %eax = 0
*/
	.balign 16,0x90
FUNC_LABEL(semCGive)
	pushfl				/* save IF in EFLAGS */
	cli				/* LOCK INTERRUPTS */
	cmpl	$FUNC(semClass),(%ecx)	/* check validity */

#ifdef	WV_INSTRUMENTATION

        je	objOkCGive		/* object is okay */

	/* windview - check the validity of instrumented class */

        cmpl    $FUNC(semInstClass),(%ecx) /* check validity */
        jne     semIsInvalidUnlock      /* invalid semaphore */

objOkCGive:

#else

        jne     semIsInvalidUnlock      /* invalid semaphore */

#endif  /* WV_INSTRUMENTATION */

	movl	SEM_Q_HEAD(%ecx),%edx	/* test semaphore queue head */
	testl	%edx, %edx		/* is the sem empty? */
	jne	FUNC(semQGet)		/* if not empty, get from q */
	incl	SEM_STATE(%ecx)		/* increment count */

	cmpl	SEM_EVENTS_TASKID (%ecx), %edx /* %edx is 0 here */
	jnz	FUNC(semEvRsrcSend)	/* Event Code */

	popfl				/* UNLOCK INTERRUPTS */
	ret				/* return OK */

/*******************************************************************************
*
* semCTake - optimized take of a counting semaphore
*

* STATUS semCTake (semId)
*    (
*    SEM_ID semId,		/@ semaphore id to give @/
*    int  timeout		/@ timeout in ticks @/
*    )

* INTERNAL
* assumptions are:
*  - %ecx = semId
*/

	.balign 16,0x90
FUNC_LABEL(semCTake)
	cmpl	$0, FUNC(intCnt)	/* is it in ISR? */
	jne	FUNC(semIntRestrict)	/*   yes: let C do the work */

	pushfl				/* save IF in EFLAGS */
	cli				/* LOCK INTERRUPTS */
	cmpl	$FUNC(semClass),(%ecx)	/* check validity */

#ifdef	WV_INSTRUMENTATION

        je	objOkCTake		/* object is okay */

	/* windview - check the validity of instrumented class */

        cmpl    $FUNC(semInstClass),(%ecx) /* check validity */
        jne     semIsInvalidUnlock      /* invalid semaphore */

objOkCTake:

#else

        jne     semIsInvalidUnlock      /* invalid semaphore */

#endif  /* WV_INSTRUMENTATION */

	cmpl	$0,SEM_STATE(%ecx)	/* test count */
	je	FUNC(semQPut)		/* if sem is owned we block */
	decl	SEM_STATE(%ecx)		/* decrement count */
	popfl				/* UNLOCK INTERRUPTS */
	xorl	%eax,%eax		/* return OK */
	ret

#endif	/* !PORTABLE */
