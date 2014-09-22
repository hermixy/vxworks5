/* semALib.s - i80x86 internal VxWorks binary semaphore assembly library */

/* Copyright 1984-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01l,20mar02,hdn  preserved previous state of the int enable bit (spr 74016)
01k,09nov01,pcm  added VxWorks semaphore events
01j,23aug01,hdn  added FUNC/FUNC_LABEL, replaced .align with .balign
01i,30apr98,cjtc bug fixes in semaphore instrumentation
01h,28apr98,pr   fixed problem with registers in WV code.
01g,16apr98,pr   cleanup.
01f,17feb98,pr   added WindView 2.0 code.
01f,24jun97,sub  fixed semTakeGlobal and semGiveGlobal for VxMP - SPR # 8833
01e,29jul96,sbs  Made windview conditionally compile.
01d,08aug94,hdn  added support for WindView.
01c,02jun93,hdn  split into sem[CM]ALib.s to increase modularity.
		 added shared memory semaphores support.
		 added signal restart.
		 updated to 5.1
		  - fixed #else and #endif
		  - changed ASMLANGUAGE to _ASMLANGUAGE
		  - changed copyright notice
01b,13oct92,hdn  debugged.
01a,07apr92,hdn  written based on TRON version.
*/

/*
DESCRIPTION
This module contains internals to the VxWorks kernel.
These routines have been coded in assembler because they are optimized for
performance.
*/

#define _ASMLANGUAGE
#include "vxWorks.h"
#include "vwModNum.h"
#include "asm.h"
#include "eventLib.h"
#include "semLib.h"
#include "private/semLibP.h"
#include "private/classLibP.h"
#include "private/taskLibP.h"
#include "private/eventP.h"

	
	.data
	.globl	FUNC(copyright_wind_river)
	.long	FUNC(copyright_wind_river)


#ifndef PORTABLE

	/* externals */

	.globl	FUNC(semIntRestrict)
	.globl	FUNC(semInvalid)
	.globl	FUNC(semGiveDefer)
	.globl	FUNC(windExit)
	.globl	FUNC(windPendQPut)
	.globl	FUNC(windPendQGet)
	.globl	VAR(smObjPoolMinusOne)


	/* internals */

	.globl	GTEXT(semGive)		/* optimized semGive demultiplexer */
	.globl	GTEXT(semTake)		/* optimized semTake demultiplexer */
	.globl	GTEXT(semBGive)		/* optimized binary semaphore give */
	.globl	GTEXT(semBTake)		/* optimized binary semaphore take */
	.globl	GTEXT(semQGet)		/* semaphore queue get routine */
	.globl	GTEXT(semQPut)		/* semaphore queue put routine */
	.globl	GTEXT(semOTake)		/* optimized old semaphore take */
	.globl	GTEXT(semClear)		/* optimized old semaphore semClear */
	.globl	GTEXT(semEvRsrcSend)


	.text
	.balign 16

/*******************************************************************************
*
* semGiveKern - add give routine to work queue
*
*/

semGiveKern:
	jmp	FUNC(semGiveDefer)	/* let C rtn defer work and ret */

/*******************************************************************************
*
* semGive - give a semaphore
*
*

*STATUS semGive (semId)
*    SEM_ID semId;		/* semaphore id to give *

*/
	.balign 16, 0x90
semGiveReturn:			/* This small block is before the semGive */
	popfl			/* as the default branch for a conditional */ 
	ret			/* jump is jump back. */

	.balign 16,0x90
FUNC_LABEL(semGive)
	movl	SP_ARG1(%esp),%ecx	/* semId goes into %ecx */
	testl	$1,%ecx			/* is it a global semId */
	jne	semGiveGlobal		/* if LSB is 1, its a global sem */
	xorl	%edx, %edx		/* Clear edx */

#ifdef	WV_INSTRUMENTATION

	/* 
	 * windview instrumentation - BEGIN
	 * semGive: object status class
	 */

	cmpl	FUNC(evtAction), %edx	/*  is WindView on? */
	je	noSemGiveEvt


        cmpl    $FUNC(semClass),(%ecx)	/* check validity */
        je	objOkGive
        cmpl	$FUNC(semInstClass),(%ecx) /* check validity */
        jne     noSemGiveEvt		/* invalid semaphore */

objOkGive:
	movl	$ WV_CLASS_3_ON,%eax
        andl    FUNC(wvEvtClass),%eax	/* is event collection on? */
        cmpl    $ WV_CLASS_3_ON,%eax	/* is event collection on? */
	jne	trgCheckSemGive

	pushl	%eax
	pushl	%ecx			/* semId */
	pushl	%edx

	/* is this semaphore object instrumented? */

	movl	(%ecx),%eax		/* %ecx - semId */
	cmpl	$0,SEM_INST_RTN(%eax)	/* event routine attached? */
	je	trgNoSemGive

	/* log event for this object */

	pushl	%edx			/* $0 */
	pushl	%edx			/* $0 */
	movw	SEM_RECURSE(%ecx),%dx	/* recursively called */
	pushl	%edx	
	movl	SEM_STATE(%ecx),%edx	/* state/count/owner */
	pushl	%edx
	pushl	%ecx			/* semId */
	pushl	$ 3			/* number of paramters */
	pushl	$ EVENT_SEMGIVE		/* EVENT_SEMGIVE, event id */
	movl	SEM_INST_RTN(%eax),%edx	/* get logging routine */

	call	*%edx			/* call routine */

	addl	$28,%esp

trgNoSemGive:
	popl	%edx
	popl	%ecx			/* semId */
	popl	%eax

trgCheckSemGive:
	movl	$ TRG_CLASS_3,%eax     
	orl 	$ TRG_ON,%eax
        cmpl    FUNC(trgEvtClass),%eax	/* any trigger? */
	jne	noSemGiveEvt

        movl    FUNC(_func_trgCheck),%eax    /* triggering routine */
        cmpl    %edx, %eax		/* Note: %edx = 0 */
        je      noSemGiveEvt

	pushl	%ecx			/* semId */
	pushl	%edx

        pushl   %edx			/* $0 */
        pushl   %edx			/* $0 */
	movw	SEM_RECURSE(%ecx),%dx	/* recursively called */
	pushl	%edx	
	movl	SEM_STATE(%ecx),%edx	/* state/count/owner */
	pushl	%edx
	pushl	%ecx			/* semId */
	pushl	%ecx			/* objId */
        pushl   $ TRG_CLASS3_INDEX      /* TRG_CLASS3_INDEX */
	pushl	$ EVENT_SEMGIVE		/* EVENT_SEMGIVE, event id */

        call    *%eax                   /* call triggering routine */

        addl    $32,%esp
	popl	%edx
	popl	%ecx			/* semId */

noSemGiveEvt:

	/* windview instrumentation - END */

#endif  /* WV_INSTRUMENTATION */

	movl	FUNC(kernelState),%eax	/* are we in kernel state? */
	cmpl	%edx, %eax		/* %edx already 0 */
	jne	semGiveKern		/* %eax = 0 if we are not */
	movb	SEM_TYPE(%ecx),%dl	/* put the sem class into %edx */
	andb	$ SEM_TYPE_MASK,%dl	/* mask %edx to MAX_SEM_TYPE value */
	jne	semGiveNotBinary	/* optimize for BINARY if %edx == 0 */

		/* 
		 * BINARY SEMAPHORE OPTIMIZATION
		 * assumptions are:
		 *  - %ecx = semId (%ecx is a volatile register)
		 */

FUNC_LABEL(semBGive)
		pushfl				/* save IF in EFLAGS */
		cli				/* LOCK INTERRUPTS */
		cmpl    $FUNC(semClass),(%ecx)	/* check validity */

		/*
		 * FIXME-PR: if this is going to be part of the kernel 
		 *           permanently, it should be rearranged. It  
		 *           does an extra instruction that is not needed.
		 */

#ifdef	WV_INSTRUMENTATION

		je	objOkBGive		/* object is okay */

		/* windview - check the validity of instrumented class */

		cmpl    $FUNC(semInstClass),(%ecx) /* check validity */
		jne	    semIsInvalidUnlock	/* semaphore id error */

objOkBGive:

#else

		jne     semIsInvalidUnlock	/* invalid semaphore */

#endif  /* WV_INSTRUMENTATION */

		movl    SEM_Q_HEAD(%ecx),%eax
		movl    SEM_STATE(%ecx), %edx	/* %edx = previous semOwner */
		movl    %eax,SEM_STATE(%ecx)
		testl   %eax, %eax		/* is new semOwner NULL? */
		jne	    FUNC(semQGet)	/* if not empty, get from q */

		cmpl    SEM_EVENTS_TASKID (%ecx), %eax
		jz	    semGiveReturn	/* skip if taskId == NULL */

		testl   %edx, %edx		/* is prev semOwner NULL? */
		jnz     FUNC(semEvRsrcSend)	/* if not, jump */

		popfl				/* UNLOCK INTERRUPTS */
		ret				/* return OK */

	.balign 16,0x90
semGiveNotBinary:

        /*
	 * Call semGive indirectly via semGiveTbl.  Note that the index could
	 * equal zero after it is masked.  semBGive is the zeroeth element
	 * of the table, but for it to function correctly in the optimized
	 * version above, we must be certain not to clobber %ecx.  Note, also
	 * that old semaphores will also call semBGive above.
	 */

	movl	FUNC(semGiveTbl)(,%edx,4),%edx
	jmp	*%edx			    /* invoke give rtn, it will ret */

	.balign 16,0x90
semGiveGlobal:
	addl	FUNC(smObjPoolMinusOne),%ecx /* convert id to local addr */
	movb	7(%ecx),%dl		    /* get semaphore type in %dl */
				            /* offset 7 is used as the type
					     * is stored in  network order
					     */		
	andl	$ SEM_TYPE_MASK,%edx	    /* mask %edx to MAX_SEM_TYPE */
	movl	FUNC(semGiveTbl)(,%edx,4),%edx /* %edx is the give rtn. */
	pushl	%ecx			    /* push converted semId */
	call	*%edx			    /* call appropriate give rtn. */
	addl	$4,%esp			    /* clean up */
	ret

/*******************************************************************************
*
* semEvRsrcSend - send a semaphore event on semGive
*
* This sub-routine is only executed if a semaphore state change has occurred,
* and semId->events.taskId is not NULL.
*
* INTERNAL
* assumptions are:
*  - %ecx = semId (%ecx is a volatile register)
*  - %eax = 0
*/
	.balign	16, 0x90
FUNC_LABEL(semEvRsrcSend)
	movl	$1, FUNC(kernelState)	/* ENTER KERNEL */
	popfl				/* UNLOCK INTERRUPTS */

	movl	FUNC(errno), %eax
	pushl	%esi			/* Save %esi */
	pushl	%edi			/* Save %edi */
	pushl	%ebx			/* Save %ebx */
	pushl	%eax			/* Save errno */

	movl	%ecx, %esi		/* %esi = semId */

	/*
	 * Both semId->events.options and semId->options are being pushed
	 * onto the stack.  But where is semId->options?  SEM_TYPE is located
	 * at offset 0x04, and SEM_OPTIONS is located at offset 0x05.  As the 
	 * x86 series is little-endian, semId->options will be loaded into the
	 * %bh portion of the %ebx register.  It should also be noted that both
	 * semId->options and semId->events.options are a single byte.
	 */

	movl	SEM_EVENTS_OPTIONS (%ecx), %edi	/* Get semId->events.options */
	movl	SEM_TYPE (%ecx), %ebx		/* Get semId->options */

	movl	SEM_EVENTS_REGISTERED (%ecx), %edx
	movl	SEM_EVENTS_TASKID (%ecx), %eax
	pushl	%edx			/* semId->events.registered param */
	pushl	%eax			/* semId->events.taskId param */
	call	FUNC(eventRsrcSend)	/* eventRsrcSend (%eax, %edx) */
	addl	$8, %esp		/* Remove params from stack */

	/*
	 * This next block does the C code
	 * if ((evSendStatus != OK) && !(semOptions & EVENT_SEND_NOTIFY_ERROR))
	 * Two tests can be condensed into one since ...
	 * OK & x = 0, and ERROR & x = x
	 * Thus (evSendStatus & semOptions & EVENT_SEND_NOTIFY_ERROR) works.
	 * To avoid extra jumps (and flushing of the pipeline), the setXX
	 * commands are used.
	 */

	movl	%eax, %edx		/* edx = %eax (backup) */
	xorl	%ecx, %ecx		/* Clear <retStatus> */
	andl	%ebx, %eax		/* %eax = {0, semOptions << 8} */
	testl	$(SEM_EVENTSEND_ERR_NOTIFY << 8), %eax
	setz	%cl			/* 1 if will return OK, 0 if ERROR */

	/*
	 * "if ((evtOptions & EVENTS_SEND_ONCE) || (evSendStatus != OK))"
	 * combined into one test.  semId->events.taskId is cleared if the 
	 * if-statement was TRUE.  No branching--cool.
	 */

	xorl	%eax, %eax		/* Clear %eax */
	decl	%ecx			/* retStatus = {OK, ERROR} */
	orl	%edi, %edx		/* Combine two tests into one */
	incl	%eax			/* %eax  = $EVENTS_SEND_ONCE */

	movl	$S_eventLib_EVENTSEND_FAILED, %edi	/* new errno */
	andl	%edx, %eax		/* %eax = {1,0} */
	popl	%ebx			/* old <errno> */
	decl	%eax			/* %eax = {0x0, 0xffffffff} */
	subl	%ebx, %edi		/* new <errno> - old <errno> */
	andl	%eax, SEM_EVENTS_TASKID (%esi)	/* {0, unchanged} */
	andl	%ecx, %edi		/* %edi = {0, new errno - old errno} */
	movl	%ecx, %esi		/* Save <retStatus> */
	addl	%ebx, %edi		/* %edi = {old errno, new errno} */

	call	FUNC(windExit)		/* EXIT KERNEL */

	movl	%edi, FUNC(errno)	/* Restore errno */
	movl	%esi, %eax		/* Restore <retStatus> */

	popl	%ebx			/* Restore %ebx */
	popl	%edi			/* Restore %edi */
	popl	%esi			/* Restore %esi */

	ret

/*******************************************************************************
*
* semIsInvalid - unlock interupts and call semInvalid ().
*/

	.balign 16,0x90
semIsInvalidUnlock:
	popfl				/* UNLOCK INTERRUPTS */

semIsInvalid:
	jmp	FUNC(semInvalid)	/* let C rtn do work and ret */

/*******************************************************************************
*
* semTake - take a semaphore
*

* STATUS semTake
*	(
*	SEM_ID semId,		/@ semaphore ID to give @/
*	int  timeout 		/@ timeout in ticks @/
*	)

*/

	.balign 16,0x90
FUNC_LABEL(semTake)
	movl	SP_ARG1(%esp),%ecx	/* semId goes into %ecx */
	testl	$1,%ecx			/* is it a global semId */
	jne	semTakeGlobal		/* if LSB is 1, its a global sem */

#ifdef	WV_INSTRUMENTATION

        /* 
	 * windview instrumentation - BEGIN
         * semTake: object status class
         */

	cmpl	$0,FUNC(evtAction)	/* is WindView on? */
        je	noSemTakeEvt

        cmpl	$FUNC(semClass),(%ecx)	/* check validity */
        je	objOkTake
        cmpl	$FUNC(semInstClass),(%ecx) /* check validity */
        jne     noSemTakeEvt		/* invalid semaphore */

objOkTake:
	movl	$ WV_CLASS_3_ON,%eax
        andl    FUNC(wvEvtClass),%eax	/* is event collection on? */
        cmpl    $ WV_CLASS_3_ON,%eax	/* is event collection on? */
	jne	trgCheckSemTake

	pushl	%eax
	pushl	%ecx			/* semId */
	pushl	%edx

        /* is this semaphore object instrumented? */

        movl	(%ecx),%eax		/* %ecx - semId */
        cmpl	$0,SEM_INST_RTN(%eax)	/* event routine attached? */
	je	trgNoSemTake

        /* log event for this object */

        pushl	$0
        pushl	$0
        movl	$0,%edx
        movw	SEM_RECURSE(%ecx),%dx	/* recursively called */
        pushl	%edx
        movl	SEM_STATE(%ecx),%edx	/* state/count/owner */
        pushl	%edx
        pushl	%ecx			/* semId */
	pushl	$ 3			/* number of paramters */
	pushl	$ EVENT_SEMTAKE		/* EVENT_SEMTAKE, event id */
        movl	SEM_INST_RTN(%eax),%edx	/* get logging routine */

        call	*%edx			/* call routine */

        addl	$28,%esp

trgNoSemTake:
	popl	%edx
	popl	%ecx			/* semId */
	popl	%eax

trgCheckSemTake:
	movl	$ TRG_CLASS_3,%eax     
	orl 	$ TRG_ON,%eax
        cmpl    FUNC(trgEvtClass),%eax	/* any trigger? */
	jne	noSemTakeEvt

        movl    FUNC(_func_trgCheck),%eax /* triggering routine */
        cmpl    $0,%eax
        je      noSemTakeEvt

	pushl	%ecx			/* semId */
	pushl	%edx

        pushl   $ 0                     /* 0 */
        pushl   $ 0                     /* 0 */
        movl	$0,%edx
        movw	SEM_RECURSE(%ecx),%dx	/* recursively called */
        pushl	%edx
        movl	SEM_STATE(%ecx),%edx	/* state/count/owner */
        pushl	%edx
        pushl	%ecx			/* semId */
	pushl	%ecx			/* objId */
        pushl   $ TRG_CLASS3_INDEX      /* TRG_CLASS3_INDEX */
	pushl	$ EVENT_SEMTAKE		/* EVENT_SEMTAKE, event id */

        call    *%eax                   /* call triggering routine */

        addl    $32,%esp
	popl	%edx
	popl	%ecx			/* semId */

noSemTakeEvt:

        /* windview instrumentation - END */

#endif  /* WV_INSTRUMENTATION */

	movb	SEM_TYPE(%ecx),%dl	/* get semaphore class into %edx */
	andl	$ SEM_TYPE_MASK,%edx	/* mask %edx to sane value */
	jne	semTakeNotBinary	/* optimize binary semaphore %edx==0 */
	
		/* 
		 * BINARY SEMAPHORE OPTIMIZATION
		 * assumptions are:
	         *  - %ecx = semId (%ecx is a volatile register)
		 */

FUNC_LABEL(semBTake)
		cmpl	$0, FUNC(intCnt)	/* is it in ISR? */
		jne	FUNC(semIntRestrict)	/*   yes: let C do the work */

		pushfl				/* save IF in EFLAGS */
		cli				/* LOCK INTERRUPTS */
		cmpl	$FUNC(semClass),(%ecx)	/* check validity */

#ifdef	WV_INSTRUMENTATION

        	je	objOkBTake		/* object is okay */

		/* windview - check the validity of instrumented class */

        	cmpl	$FUNC(semInstClass),(%ecx) /* check validity */
        	jne	semIsInvalidUnlock	/* semaphore id error */

objOkBTake:

#else

                jne     semIsInvalidUnlock      /* invalid semaphore */

#endif  /* WV_INSTRUMENTATION */

		movl	SEM_STATE(%ecx),%eax	/* test for owner */
		testl	%eax,%eax		/* is the sem owned? */
		jne	FUNC(semQPut)		/* if sem is owned we block */
		movl	FUNC(taskIdCurrent),%edx
		movl	%edx,SEM_STATE(%ecx)	/* set the owner */
		popfl				/* UNLOCK INTERRUPTS */
		ret				/* %eax is still 0 for OK */

	.balign 16,0x90
semTakeNotBinary:
	movl	FUNC(semTakeTbl)(,%edx,4),%edx
	jmp	*%edx			/* invoke take rtn, it will ret */

	.balign 16,0x90
semTakeGlobal:
	addl	FUNC(smObjPoolMinusOne),%ecx /* convert id to local addr */
	movb	7(%ecx),%dl		/* get semaphore type in %dl */
				        /* offset 7 is used as the type
					 * is stored in  network order
					 */		
	andl	$ SEM_TYPE_MASK,%edx	/* mask %edx to MAX_SEM_TYPE */
	movl	FUNC(semTakeTbl)(,%edx,4),%edx /* %edx is the take rtn. */
	pushl	SP_ARG2(%esp)		/* push timeout */
	pushl	%ecx			/* push converted semId */
	call	*%edx			/* call appropriate take rtn. */
	addl	$8,%esp		  	/* clean up */
	ret

/*******************************************************************************
*
* semQGet - unblock a task from the semaphore queue head
*
* INTERNAL
* assumptions are:
*  - %ecx = semId (%ecx is a volatile register)
*  - (%esp + 0x0) = EFLAGS
*  - (%esp + 0x4) = retAddr
*  - (%esp + 0x8) = semId
*/

	.balign 16,0x90
FUNC_LABEL(semQGet)
	movl	$1,FUNC(kernelState)	/* KERNEL ENTER */

#ifdef	WV_INSTRUMENTATION

        /* 
	 * windview instrumentation - BEGIN
         * semGive: task transition state class
         */

	cmpl	$0,FUNC(evtAction)	/* is WindView on? */
	je	noSemQGetEvt

	movl	$ WV_CLASS_2_ON,%eax
        andl    FUNC(wvEvtClass),%eax	/* is event collection on? */
        cmpl    $ WV_CLASS_2_ON,%eax	/* is event collection on? */
	jne	trgCheckSemQGet

	movl	FUNC(_func_evtLogM1),%edx /* event log routine */
	cmpl	$0,%edx
	je	noSemQGetEvt
	pushl	%ecx			/* push semId */
	pushl	$ EVENT_OBJ_SEMGIVE	/* EVENT_OBJ_SEMGIVE */

	call	*%edx			/* call event log routine */		

	addl	$4,%esp			/* restore stack pointer */
        popl	%ecx        		/* pop semId */

trgCheckSemQGet:
	movl	$ TRG_CLASS_2,%eax     
	orl 	$ TRG_ON,%eax
        cmpl    FUNC(trgEvtClass),%eax	/* any trigger? */
	jne	noSemQGetEvt

        movl    FUNC(_func_trgCheck),%edx /* triggering routine */
        cmpl    $0,%edx
        je      noSemQGetEvt

	xorl	%eax, %eax		/* Clear %eax */
	pushl	%ecx			/* semId */
        pushl   %eax			/* 0 */
        pushl   %eax			/* 0 */
        pushl   %eax			/* 0 */
        pushl   %eax			/* 0 */
	pushl	%ecx			/* push semId */
	pushl	%ecx			/* objId */
        pushl   $ TRG_CLASS2_INDEX      /* TRG_CLASS3_INDEX */
	pushl	$ EVENT_OBJ_SEMGIVE	/* EVENT_OBJ_SEMGIVE */

	call	*%edx			/* call triggering routine */		

        addl    $32,%esp
	popl	%ecx			/* semId */

noSemQGetEvt:

        /* windview instrumentation - END */

#endif  /* WV_INSTRUMENTATION */

	popfl				/* UNLOCK INTERRUPTS */
	leal	SEM_Q_HEAD(%ecx),%edx
	pushl	%edx			/* push the pointer to qHead */
	call	FUNC(windPendQGet)	/* unblock someone */
	addl	$4,%esp			/* clean up */
	call	FUNC(windExit)		/* KERNEL EXIT */
	ret         			/* windExit sets %eax */

/*******************************************************************************
*
* semQPut - block current task on the semaphore queue head
*
* INTERNAL
* assumptions are:
*  - %ecx = semId (%ecx is a volatile register)
*  - (%esp + 0x0) = EFLAGS
*  - (%esp + 0x4) = retAddr
*  - (%esp + 0x8) = semId
*  - (%esp + 0xc) = timeout
*/

	.balign 16,0x90
FUNC_LABEL(semQPut)
	movl	$1,FUNC(kernelState)	/* KERNEL ENTER */

#ifdef	WV_INSTRUMENTATION

        /* 
	 * windview instrumentation - BEGIN
         * semTake: task transition state class
         */

	cmpl	$0,FUNC(evtAction)	/* is WindView on? */
	je	noSemQPutEvt

	movl	$ WV_CLASS_2_ON,%eax
        andl    FUNC(wvEvtClass),%eax	/* is event collection on? */
        cmpl    $ WV_CLASS_2_ON,%eax	/* is event collection on? */
	jne	trgCheckSemQPut

        movl	FUNC(_func_evtLogM1),%edx /* call event log routine */
	cmpl	$0,%edx
        je	trgCheckSemQPut

        pushl	%ecx			/* push semId */
        pushl	$ EVENT_OBJ_SEMTAKE	/* EVENT_OBJ_SEMTAKE */
        call	*%edx
        addl	$4,%esp			/* restore stack pointer */
        popl	%ecx			/* pop semId */

trgCheckSemQPut:
	movl	$ TRG_CLASS_2,%eax     
	orl 	$ TRG_ON,%eax
        cmpl    FUNC(trgEvtClass),%eax	/* any trigger? */
	jne	noSemQPutEvt

        movl    FUNC(_func_trgCheck),%edx /* triggering routine */
        cmpl    $0,%edx
        je      noSemQPutEvt

	pushl	%ecx			/* semId */
        pushl   $ 0                     /* 0 */
        pushl   $ 0                     /* 0 */
        pushl   $ 0                     /* 0 */
        pushl   $ 0                     /* 0 */
	pushl	%ecx			/* push semId */
	pushl	%ecx			/* objId */
        pushl   $ TRG_CLASS2_INDEX      /* TRG_CLASS3_INDEX */
	pushl	$ EVENT_OBJ_SEMTAKE	/* EVENT_OBJ_SEMTAKE */

	call	*%edx			/* call triggering routine */		

        addl    $32,%esp
	popl	%ecx			/* semId */

noSemQPutEvt:

        /* windview instrumentation - END */

#endif  /* WV_INSTRUMENTATION */

	popfl				/* UNLOCK INTERRUPTS */
	pushl	SP_ARG2(%esp)		/* push the timeout */
	leal	SEM_Q_HEAD(%ecx),%eax
	pushl	%eax			/* push the &semId->qHead */
	call	FUNC(windPendQPut)	/* block on the semaphore */
	addl	$8,%esp			/* tidy up */
	testl	%eax,%eax		/* if (windPendQPut() != OK) */
	jne	semQPutFail		/*   put failed */
	call	FUNC(windExit)		/* else KERNEL EXIT */
	testl	%eax,%eax		/* if (windExit() != OK) */
	jne	semWindExitNotOk	/*   RESTART */
	ret				/* done */

	.balign 16,0x90
semWindExitNotOk:
	cmpl 	$1,%eax			/* check for RESTART retVal */
	je	semRestart
	ret				/* EAX = ERROR */

	.balign 16,0x90
semQPutFail:
	call	FUNC(windExit)		/* KERNEL EXIT */
	movl	$-1,%eax		/* return ERROR */
	ret				/* return to sender */

	.balign 16,0x90
semRestart:
	pushl	SP_ARG2(%esp)		/* push the timeout */
	movl	FUNC(_func_sigTimeoutRecalc),%eax
	call	*%eax			/* recalc the timeout */
	addl	$4,%esp			/* clean up */
	movl	%eax,SP_ARG2(%esp)	/* and store it */
	jmp	FUNC(semTake)		/* start the whole thing over */

/*******************************************************************************
*
* semOTake - VxWorks 4.x semTake
*
* Optimized version of semOTake.  This inserts the necessary argument of
* WAIT_FOREVER for semBTake.
*/

	.balign 16,0x90
FUNC_LABEL(semOTake)
	pushl	$-1			/* push WAIT_FOREVER on stack */
	movl	SP_ARG1+4(%esp),%ecx	/* put semId in %ecx */
	pushl	%ecx			/* put semId on stack */
	call	FUNC(semBTake)		/* do semBTake */
	addl	$8,%esp			/* cleanup */
	ret
	
/*******************************************************************************
*
* semClear - VxWorks 4.x semClear
*
* Optimized version of semClear.  This inserts the necessary argument of
* NO_WAIT for semBTake().
*/

	.balign 16,0x90
FUNC_LABEL(semClear)
	pushl	$0			/* push NO_WAIT on stack */
	movl	SP_ARG1+4(%esp),%ecx	/* put semId in %ecx */
	pushl	%ecx			/* put semId on stack */
	call	FUNC(semBTake)		/* do semBTake */
	addl	$8,%esp			/* cleanup */
	ret

#endif	/* !PORTABLE */
