/* semMALib.s - i80x86 internal VxWorks mutex semaphore assembly library */

/* Copyright 1984-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01f,20mar02,hdn  preserved previous state of the int enable bit (spr 74016)
01e,09nov01,pcm  added VxWorks semaphore events
01d,22aug01,hdn  added FUNC/FUNC_LABEL, replaced .align with .balign
01c,29jul96,sbs  Made windview conditionally compile.
01b,08aug94,hdn  added support for WindView.
01a,02jun93,hdn  extracted from semALib.s
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
	.globl	FUNC(semMGiveKern)
	.globl	FUNC(semMPendQPut)
	.globl	FUNC(semTake)
	.globl	FUNC(windExit)
	.globl	VAR(semMGiveKernWork)


	/* internals */

	.globl	GTEXT(semMGive)		/* optimized mutex semaphore give */
	.globl	GTEXT(semMTake)		/* optimized mutex semaphore take */


	.text
	.balign 16

/*******************************************************************************
*
* semIsInvalidUnlock - unlock interupts and call semInvalid ().
*/

semIsInvalidUnlock:
	popfl				/* UNLOCK INTERRUPTS */
	jmp	FUNC(semInvalid)	/* let C rtn do work and ret */

/*******************************************************************************
*
* semMGive - optimized give of a mutex semaphore
*

* STATUS semMGive
*    (
*    SEM_ID semId		/@ semaphore id to give @/
*    )

* INTERNAL
* assumptions are:
*  - %ecx = semId
*  - %eax = 0
*/

	.balign 16,0x90
semMRecurse:
	decw	SEM_RECURSE(%ecx)		/* decrement recurse count */
	popfl					/* UNLOCK INTERRUPTS */
	ret

	.balign 16,0x90
FUNC_LABEL(semMGive)
	cmpl	FUNC(intCnt), %eax		/* is it in ISR? */
	jne	FUNC(semIntRestrict)		/*   yes: let C do the work */

	movl	FUNC(taskIdCurrent),%edx	/* taskIdCurrent into %edx */
	pushfl					/* save IF in EFLAGS */
	cli					/* LOCK INTERRUPTS */
	cmpl	$FUNC(semClass),(%ecx)		/* check validity */

#ifdef	WV_INSTRUMENTATION

        je	objOkMGive			/* object is okay */

	/* windview - check the validity of instrumented class */

        cmpl	$FUNC(semInstClass),(%ecx)	/* check validity */
	jne 	semIsInvalidUnlock		/* semaphore id error */

objOkMGive:

#else 

        jne     semIsInvalidUnlock		/* semaphore id error */

#endif  /* WV_INSTRUMENTATION */

	cmpl	SEM_STATE(%ecx),%edx		/* taskIdCurrent is owner? */
	jne	semIsInvalidUnlock		/* SEM_INVALID_OPERATION */
	cmpw	SEM_RECURSE(%ecx), %ax		/* if recurse count > 0 */
	jne	semMRecurse			/* handle recursion */

semMInvCheck:
	testb	$0x8,SEM_OPTIONS(%ecx)		/* SEM_INVERSION_SAFE? */
	je	semMStateSet			/* if not, test semQ */
	decl	WIND_TCB_MUTEX_CNT(%edx)	/* decrement mutex count */
	jne	semMStateSet			/* if nonzero, test semQ */
	movl	WIND_TCB_PRIORITY(%edx),%eax	/* put priority in %eax */
	subl	WIND_TCB_PRI_NORMAL(%edx),%eax	/* subtract normal priority */
	je	semMStateSet			/* if same, test semQ */
	movl	$4,%eax				/* or in PRIORITY_RESORT */

semMStateSet:
	pushl	SEM_Q_HEAD(%ecx)		/* update semaphore state */
	popl	SEM_STATE(%ecx)
	cmpl	$0,SEM_STATE(%ecx)
	je	semMEventRsrcSend		/* Jump if state change */
	incl	%eax				/* Else, OR in Q_GET */
	jmp	semMDelSafe			/* Jump to semMDelSafe */

	.balign	16, 0x90
semMEventRsrcSend:
	cmpl	$0, SEM_EVENTS_TASKID (%ecx)	
	je	semMDelSafe			/* Jump if taskId is NULL */
	addl	$SEM_M_SEND_EVENTS, %eax	/* Else, OR in SEND_EVENTS */

semMDelSafe:
	testb	$0x4,SEM_OPTIONS(%ecx)		/* SEM_DELETE_SAFE? */
	je	semMShortCut			/* check for short cut */
	decl	WIND_TCB_SAFE_CNT(%edx)		/* decrement safety count */
	jne	semMShortCut			/* check for short cut */
	cmpl	$0,WIND_TCB_SAFETY_Q_HEAD(%edx) /* check for pended deleters */
	je	semMShortCut			/* check for short cut */
	addl	$2,%eax				/* OR in SAFETY_Q_FLUSH */

semMShortCut:
	testl	%eax,%eax			/* any work for kernel level? */
	jne	semMKernWork			/* enter kernel if any work */
	popfl					/* UNLOCK INTERRUPTS */
	ret					/* %eax is still 0 for OK */

	.balign 16,0x90
semMKernWork:
	movl	$1,FUNC(kernelState)		/* KERNEL ENTER */
	popfl					/* UNLOCK INTERRUPTS */
	movl	%eax,FUNC(semMGiveKernWork)	/* setup work for semMGiveKern*/
	jmp	FUNC(semMGiveKern)		/* finish semMGive in C */

/*******************************************************************************
*
* semMTake - optimized take of a mutex semaphore
*

* STATUS semMTake
*    (
*    SEM_ID semId,		/@ semaphore id to give @/
*    int  timeout		/@ timeout in ticks @/
*    )

* INTERNAL
* assumptions are:
*  - %ecx = semId
*/

	.balign 16,0x90
FUNC_LABEL(semMTake)
	cmpl	$0, FUNC(intCnt)		/* is it in ISR? */
	jne	FUNC(semIntRestrict)		/*   yes: let C do the work */

	movl	FUNC(taskIdCurrent),%edx	/* taskIdCurrent into %edx */
	pushfl					/* save IF in EFLAGS */
	cli					/* LOCK INTERRUPTS */
	cmpl	$FUNC(semClass),(%ecx)		/* check validity */

#ifdef	WV_INSTRUMENTATION

        je      objOkMTake			/* object is okay */

	/* windview - check the validity of instrumented class */

        cmpl    $FUNC(semInstClass),(%ecx)	/* check validity */
        jne     semIsInvalidUnlock		/* invalid semaphore */

objOkMTake:

#else

        jne     semIsInvalidUnlock              /* invalid semaphore */

#endif  /* WV_INSTRUMENTATION */

	movl	SEM_STATE(%ecx),%eax		/* test for owner */
	testl	%eax,%eax			/* is the sem empty? */
	jne	semMEmpty			/* sem is owned, is it ours? */
	movl	%edx,SEM_STATE(%ecx)		/* we now own semaphore */
	testb	$0x4,SEM_OPTIONS(%ecx)		/* SEM_DELETE_SAFE? */
	je	semMPriCheck			/* semMPriCheck */
	incl	WIND_TCB_SAFE_CNT(%edx)		/* bump safety count */

semMPriCheck:
	testb	$0x8,SEM_OPTIONS(%ecx)		/* SEM_INVERSION_SAFE? */
	je	semMDone			/* if not, skip increment */
	incl	WIND_TCB_MUTEX_CNT(%edx)	/* bump priority mutex count */

semMDone:
	popfl					/* UNLOCK INTERRUPTS */
	ret					/* %eax is still 0 for OK */

	.balign 16,0x90
semMEmpty:
	subl	%edx,%eax			/* recursive take */
	jne	semMQUnlockPut			/* if not, block */
	incw	SEM_RECURSE(%ecx)		/* increment recurse count */
	popfl					/* UNLOCK INTERRUPTS */
	ret					/* %eax is still 0 for OK */

	.balign 16,0x90
semMQUnlockPut:
	movl	$1,FUNC(kernelState)		/* KERNEL ENTER */
	popfl					/* UNLOCK INTERRUPTS */
	pushl	SP_ARG2(%esp)			/* push the timeout */
	pushl	%ecx				/* push the semId */
	call	FUNC(semMPendQPut)		/* do as much in C as possible*/
	addl	$8,%esp				/* clean up */
	testl	%eax,%eax			/* test return value */
	jne	semMFail			/* if !OK, exit kernel, ERROR */
	call	FUNC(windExit)			/* KERNEL EXIT */
	testl 	%eax,%eax                 	/* windExit() == OK -> return */
	jne	semMWindExitNotOk
	ret					/* EAX = OK */

	.balign 16,0x90
semMWindExitNotOk:
	cmpl    $1,%eax                 	/* check for RESTART retVal */
	je      semMRestart
	ret 					/* EAX = ERROR */

	.balign 16,0x90
semMRestart:
	pushl	SP_ARG2(%esp)			/* push the timeout */
	movl	FUNC(_func_sigTimeoutRecalc),%eax /* addr of recalc routine */
	call	*%eax				/* recalc the timeout */
	addl	$4,%esp				/* clean up */
	movl	%eax,SP_ARG2(%esp)		/* restore the timeout */
	jmp	FUNC(semTake)			/* start the whole thing over */

	.balign 16,0x90
semMFail:
	call	FUNC(windExit)			/* KERNEL EXIT */
	movl	$-1,%eax			/* return ERROR */
	ret					/* failed */

#endif	/* !PORTABLE */
