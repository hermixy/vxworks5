/* windALib.s - I80x86 internal VxWorks kernel assembly library */

/* Copyright 1984-2001 Wind River Systems, Inc. */

/*
modification history
--------------------
01q,05dec01,hdn  added PAUSE instruction for Pentium4
01p,13nov01,ahm  Added AutoHalt mode for idling kenel (SPR#32599)
01o,26sep01,hdn  added the interrupt stack switch support
01n,23aug01,hdn  added FUNC/FUNC_LABEL, replaced .align with .balign
		 replaced sysCodeSelector with sysCsSuper
01m,28aug98,hdn  replaced sysIntLevel() with "movl INT29(%edi),%eax"
01l,10aug98,pr   replaced evtsched with function pointer _func_evtLogTSched
01k,29apr98,cjtc fix WV instrumentation in idle
01j,16apr98,pr   cleanup.
01i,17feb98,pr   added WindView 2.0 code.
01h,29jul96,sbs  Made windview conditionally compile.
01g,14jun95,hdn  changed CODE_SELECTOR to sysCodeSelector.
01f,08aug94,hdn  added support for WindView.
01e,02jun93,hdn  updated to 5.1.
		  - fixed #else and #endif
		  - changed VOID to void
		  - changed ASMLANGUAGE to _ASMLANGUAGE
		  - changed copyright notice
01d,15oct92,hdn  supported nested interrupt.
01c,13oct92,hdn  debugged.
01b,07apr92,hdn  written optimized codes.
01a,28feb92,hdn  written based on TRON, 68k version.
*/

/*
DESCRIPTION
This module contains internals to the VxWorks kernel.
These routines have been coded in assembler because they are either
specific to this processor, or they have been optimized for performance.
*/

#define _ASMLANGUAGE
#include "vxWorks.h"
#include "asm.h"
#include "regs.h"
#include "esf.h"
#include "private/eventP.h"
#include "private/trgLibP.h"
#include "private/taskLibP.h"
#include "private/semLibP.h"
#include "private/workQLibP.h"

/* defines */

#define	INT_STACK_USE
#define X86_POWER_MANAGEMENT


	.data
	.globl	FUNC(copyright_wind_river)
	.long	FUNC(copyright_wind_river)


	/* internals */

	.globl	GTEXT(windExit)		/* routine to exit mutual exclusion */
	.globl	GTEXT(windIntStackSet)	/* interrupt stack set routine */
	.globl	GTEXT(vxTaskEntry)	/* task entry wrapper */
	.globl	GTEXT(intEnt)		/* interrupt entrance routine */
	.globl	GTEXT(intExit)		/* interrupt exit routine */
	.globl	GTEXT(intStackEnable)	/* interrupt stack usage enabler */

#ifdef X86_POWER_MANAGEMENT
        .globl	GTEXT(vxIdleAutoHalt)	/* put cpu in AutoHalt when idle */
#endif /* X86_POWER_MANAGEMENT */

#ifdef PORTABLE
	.globl	GTEXT(windLoadContext)	/* needed by portable reschedule () */
#else
	.globl	GTEXT(reschedule)	/* optimized reschedule () routine */
#endif	/* PORTABLE */

	.globl	GDATA(vxIntStackPtr)	/* interrupt stack pointer */
	.globl	GDATA(vxIntStackEnabled) /* interrupt stack enabled */

FUNC_LABEL(vxIntStackPtr)		/* interrupt stack pointer */
	.long	0x00000000
FUNC_LABEL(vxIntStackEnabled)		/* TRUE if interrupt stack is enabled */
	.long	0x00000000
FUNC_LABEL(intNest)			/* interrupt stack nest counter */
	.long	0x00000000


	.text
	.balign 16

/*******************************************************************************
*
* windExitInt - exit kernel routine from interrupt level
*
* windExit branches here if exiting kernel routine from int level
* No rescheduling is necessary because the ISR will exit via intExit, and
* intExit does the necessary rescheduling.
*/

windExitIntWork:
	popfl				/* pop original level */
	call	FUNC(workQDoWork)	/* empty the work queue */

windExitInt:
	pushfl				/* push interrupt level to stack */
	cli				/* LOCK INTERRUPTS */
	cmpl	$0,FUNC(workQIsEmpty)	/* test for work to do */
	je	windExitIntWork		/* workQ is not empty */

#ifdef WV_INSTRUMENTATION

	/* windview instrumentation - BEGIN
	 * exit windExit with no dispatch; point 1 in the windExit diagram.
	 */
        cmpl    $0,FUNC(evtAction)	/* is WindView on? */
        je      noInst1

	movl	$ WV_CLASS_1_ON,%eax
        andl    FUNC(wvEvtClass),%eax	/* is event collection on? */
        cmpl    $ WV_CLASS_1_ON,%eax	/* is event collection on? */
        jne     trgCheckInst1

	movl	FUNC(_func_evtLogTSched),%edx	/* event log routine */
	cmpl	$0,%edx
        je      trgCheckInst1

        movl	FUNC(taskIdCurrent),%eax	/* current task */
        movl	WIND_TCB_PRIORITY(%eax),%ecx

        pushl	%ecx	                        /* WIND_TCB_PRIORITY */
        pushl	%eax	                        /* taskIdCurrent */
        /* Here we try to determine if the task is running at an
         * inherited priority, if so a different event is generated.
         */
        cmpl	WIND_TCB_PRI_NORMAL(%eax),%ecx
        jge     noInst1Inheritance		/* no inheritance */
        pushl	$ EVENT_WIND_EXIT_NODISPATCH_PI
        jmp     inst1Inheritance		/* no inheritance */
noInst1Inheritance:
        pushl	$ EVENT_WIND_EXIT_NODISPATCH	/* event id */
inst1Inheritance:
        call    *%edx                           /* call evtsched routine */

	addl    $12,%esp
trgCheckInst1:
	movl	$ TRG_CLASS_1,%eax     
	orl 	$ TRG_ON,%eax
        cmpl    FUNC(trgEvtClass),%eax		/* any trigger? */
        jne     noInst1

        movl    FUNC(_func_trgCheck),%edx	/* triggering routine */
	cmpl	$0,%edx
        je      noInst1

        movl	FUNC(taskIdCurrent),%eax	/* current task */
        movl	WIND_TCB_PRIORITY(%eax),%ecx

        pushl   $ 0                     /* 0 */
        pushl   $ 0                     /* 0 */
        pushl   $ 0                     /* 0 */
        pushl	%ecx	                /* WIND_TCB_PRIORITY */
        pushl	%eax	                /* taskIdCurrent */
        pushl   $ 0                     /* obj */
        pushl   $ 0                     /* TRG_CLASS1_INDEX */

        /* Here we try to determine if the task is running at an
         * inherited priority, if so a different event is generated.
         */
        cmpl	WIND_TCB_PRI_NORMAL(%eax),%ecx
        jge     noTrgInst1Inheritance		/* no inheritance */
        pushl	$ EVENT_WIND_EXIT_NODISPATCH_PI
        jmp     trgInst1Inheritance		/* no inheritance */
noTrgInst1Inheritance:
        pushl	$ EVENT_WIND_EXIT_NODISPATCH    /* event id */
trgInst1Inheritance:
        call    *%edx                           /* call triggering routine */

	addl    $32,%esp

noInst1:
	/* windview instrumentation - END */

#endif  /* WV_INSTRUMENTATION */

	movl	$0,FUNC(kernelState)	/* release mutual exclusion to kernel */
	popfl				/* pop original level */
	xorl	%eax,%eax		/* return OK */
	ret				/* intExit will do rescheduling */

/*******************************************************************************
*
* checkTaskReady - check that taskIdCurrent is ready to run
*
* This code branched to by windExit when it finds preemption is disabled.
* It is possible that even though preemption is disabled, a context switch
* must occur.  This situation arrises when a task block during a preemption
* lock.  So this routine checks if taskIdCurrent is ready to run, if not it
* branches to save the context of taskIdCurrent, otherwise it falls thru to
* check the work queue for any pending work.
*/

	.balign 16,0x90
checkTaskReady:
	cmpl	$0,WIND_TCB_STATUS(%edx) /* is task ready to run */
	jne	saveTaskContext		/* if no, we blocked with preempt off */

	/* FALL THRU TO CHECK WORK QUEUE */

/*******************************************************************************
*
* checkWorkQ -	check the work queue for any work to do
*
* This code is branched to by windExit.  Currently taskIdCurrent is highest
* priority ready task, but before we can return to it we must check the work
* queue.  If there is work we empty it via doWorkPreSave, otherwise we unlock
* interrupts, clear d0, and return to taskIdCurrent.
*/

checkWorkQ:
	cli				/* LOCK INTERRUPTS */
	cmpl	$0,FUNC(workQIsEmpty)	/* test for work to do */
	je	doWorkPreSave		/* workQueue is not empty */

#ifdef WV_INSTRUMENTATION

	/* windview instrumentation - BEGIN
	 * exit windExit with no dispatch; point 4 in the windExit diagram.
	 */
        cmpl    $0,FUNC(evtAction)	/* is WindView on? */
        je      noInst4

	movl	$ WV_CLASS_1_ON,%eax
        andl    FUNC(wvEvtClass),%eax	/* is event collection on? */
        cmpl    $ WV_CLASS_1_ON,%eax	/* is event collection on? */
        jne     trgCheckInst4

	movl	FUNC(_func_evtLogTSched),%edx	/* event log routine */
	cmpl	$0,%edx
        je      trgCheckInst4

        movl	FUNC(taskIdCurrent),%eax	/* current task */
        movl	WIND_TCB_PRIORITY(%eax),%ecx

        pushl	%ecx	                        /* WIND_TCB_PRIORITY */
        pushl	%eax	                        /* taskIdCurrent */

	/* Here we try to determine if the task is running at an
	 * inherited priority, if so a different event is generated.
	 */

        cmpl	WIND_TCB_PRI_NORMAL(%eax),%ecx
        jge     noInst4Inheritance		/* no inheritance */
        pushl	$ EVENT_WIND_EXIT_NODISPATCH_PI
        jmp     inst4Inheritance		/* no inheritance */
noInst4Inheritance:
        pushl	$ EVENT_WIND_EXIT_NODISPATCH	/* event id */
inst4Inheritance:
        call    *%edx                           /* call evtsched routine */

	addl    $12,%esp

trgCheckInst4:
	movl	$ TRG_CLASS_1,%eax     
	orl 	$ TRG_ON,%eax
        cmpl    FUNC(trgEvtClass),%eax		/* any trigger? */
        jne     noInst4

        movl    FUNC(_func_trgCheck),%edx	/* triggering routine */
	cmpl	$0,%edx
        je      noInst4

        movl	FUNC(taskIdCurrent),%eax	/* current task */
        movl	WIND_TCB_PRIORITY(%eax),%ecx

        pushl   $ 0                     /* 0 */
        pushl   $ 0                     /* 0 */
        pushl   $ 0                     /* 0 */
        pushl	%ecx	                /* WIND_TCB_PRIORITY */
        pushl	%eax	                /* taskIdCurrent */
        pushl   $ 0                     /* obj */
        pushl   $ 0                     /* TRG_CLASS1_INDEX */

	/* Here we try to determine if the task is running at an
	 * inherited priority, if so a different event is generated.
	 */

        cmpl	WIND_TCB_PRI_NORMAL(%eax),%ecx
        jge     noTrgInst4Inheritance		/* no inheritance */
        pushl	$ EVENT_WIND_EXIT_NODISPATCH_PI
        jmp     trgInst4Inheritance		/* no inheritance */
noTrgInst4Inheritance:
        pushl	$ EVENT_WIND_EXIT_NODISPATCH	/* event id */
trgInst4Inheritance:

        call    *%edx                   /* call triggering routine */

	addl    $32,%esp

noInst4:
	/* windview instrumentation - END */

#endif  /* WV_INSTRUMENTATION */

	movl	$0,FUNC(kernelState)	/* else release exclusion */
	sti				/* UNLOCK INTERRUPTS */
	xorl	%eax,%eax		/* return OK */
	ret				/* back to calling task */

/*******************************************************************************
*
* doWorkPreSave - empty the work queue with current context not saved
*
* We try to empty the work queue here, rather than let reschedule
* perform the work because there is a strong chance that the
* work we do will not preempt the calling task.  If this is the case, then
* saving the entire context just to restore it in reschedule is a waste of
* time.  Once the work has been emptied, the ready queue must be checked to
* see if reschedule must be called, the check of the ready queue is done by
* branching back up to checkTaskCode.
*/

	.balign 16,0x90
doWorkPreSave:
	sti				/* UNLOCK INTERRUPTS */
	call	FUNC(workQDoWork)	/* empty the work queue */
	jmp	checkTaskSwitch		/* back up to test if tasks switched */


/******************************************************************************
*
* windExit - task level exit from kernel
*
* Release kernel mutual exclusion (kernelState) and dispatch any new task if
* necessary.  If a higher priority task than the current task has been made
* ready, then we invoke the rescheduler.  Before releasing mutual exclusion,
* the work queue is checked and emptied if necessary.
*
* If rescheduling is necessary, the context of the calling task is saved in its
* associated TCB with the PC pointing at the next instruction after the jsr to
* this routine.  The SP in the tcb is modified to ignore the return address
* on the stack.  Thus the context saved is as if this routine was never called.
*
* Only the volatile registers e[adc]x are safe to use until the context
* is saved in saveTaskContext.
*
* At the call to reschedule the value of taskIdCurrent must be in edx.
*
* RETURNS: OK or
*	   ERROR if semaphore timeout occurs.
*
* NOMANUAL

* STATUS windExit ()

*/

	.balign 16,0x90
FUNC_LABEL(windExit)

	cmpl	$0,FUNC(intCnt)		/* if intCnt == 0 we're from task */
	jne	windExitInt		/* else we're exiting interrupt code */

	/* FALL THRU TO CHECK THAT CURRENT TASK IS STILL HIGHEST */

/*******************************************************************************
*
* checkTaskSwitch - check to see if taskIdCurrent is still highest task
*
* We arrive at this code either as the result of falling thru from windExit,
* or if we have finished emptying the work queue.  We compare taskIdCurrent
* with the highest ready task on the ready queue.  If they are same we
* go to a routine to check the work queue.  If they are different and preemption
* is allowed we branch to a routine to make sure that taskIdCurrent is really
* ready (it may have blocked with preemption disabled).  If they are different
* we save the context of taskIdCurrent and fall thru to reschedule.
*/

checkTaskSwitch:
	movl	FUNC(taskIdCurrent),%edx	/* move taskIdCurrent to edx */
	cmpl	FUNC(readyQHead),%edx		/* compare highest ready task */
	je	checkWorkQ			/* if same then time to leave */

	cmpl	$0,WIND_TCB_LOCK_CNT(%edx)	/* is task preemption allowed */
	jne	checkTaskReady			/* if no, check task is ready */

saveTaskContext:
	movl	(%esp),%eax			/* save return address as PC */
	movl	%eax,WIND_TCB_PC(%edx)
	pushfl					/* save a eflags */
	popl	WIND_TCB_EFLAGS(%edx)	
	bts	$9,WIND_TCB_EFLAGS(%edx)	/* set IF to enable INT */

	movl	%ebx,WIND_TCB_EBX(%edx)		/* e[adc]x are volatile */
	movl	%esi,WIND_TCB_ESI(%edx)
	movl	%edi,WIND_TCB_EDI(%edx)
	movl	%ebp,WIND_TCB_EBP(%edx)
	movl	%esp,WIND_TCB_ESP(%edx)
	movl	$0,WIND_TCB_EAX(%edx)		/* clear saved eax for return */

	addl	$4,WIND_TCB_ESP(%edx)		/* fix up SP for no ret adrs */

	pushl	FUNC(errno)			/* save errno */
	popl	WIND_TCB_ERRNO(%edx)

#ifdef PORTABLE
	call	FUNC(reschedule)
#else
	/* FALL THRU TO RESCHEDULE */

/*******************************************************************************
*
* reschedule - rescheduler for VxWorks kernel
*
* This routine is called when either intExit, or windExit, thinks the
* context might change.  All of the contexts of all of the tasks are
* accurately stored in the task control blocks when entering this function.
* The status register is 0x800f0000. (Supervisor, Stack0, Interrupts UNLOCKED)
*
* The register %edx must contain the value of _taskIdCurrent at the entrance to
* this routine.
*
* At the conclusion of this routine, taskIdCurrent will equal the highest
* priority task eligible to run, and the kernel work queue will be empty.
* If a context switch to a different task is to occur, then the installed
* switch hooks are called.
*
* NOMANUAL

* void reschedule ()

*/

	.balign 16,0x90
FUNC_LABEL(reschedule)
	movl	FUNC(readyQHead),%eax		/* get highest task to %eax */
	cmpl	$0,%eax
	je	idle				/* idle if nobody ready */

switchTasks:
	movl	%eax,FUNC(taskIdCurrent)	/* update taskIdCurrent */

 	movw	WIND_TCB_SWAP_IN(%eax),%bx	/* swap hook mask into %bx */
 	orw	WIND_TCB_SWAP_OUT(%edx),%bx	/* or in swap out hook mask */
 	jne	doSwapHooks			/* any swap hooks to do */
 	cmpl	$0,FUNC(taskSwitchTable)	/* any global switch hooks? */
	jne	doSwitchHooks			/* any switch hooks to do */

dispatch:
	movl	WIND_TCB_ERRNO(%eax),%ecx	/* retore errno */
	movl	%ecx,FUNC(errno)
	movl	WIND_TCB_ESP(%eax),%esp		/* push dummy except */
	pushl	WIND_TCB_EFLAGS(%eax)		/* push EFLAGS */
	pushl	FUNC(sysCsSuper)		/* push CS */
	pushl	WIND_TCB_PC(%eax) 		/* push PC */

	movl	WIND_TCB_EDX(%eax),%edx		/* restore registers */
	movl	WIND_TCB_ECX(%eax),%ecx
	movl	WIND_TCB_EBX(%eax),%ebx
	movl	WIND_TCB_ESI(%eax),%esi
	movl	WIND_TCB_EDI(%eax),%edi
	movl	WIND_TCB_EBP(%eax),%ebp
	movl	WIND_TCB_EAX(%eax),%eax

	cli					/* LOCK INTERRUPTS */
	cmpl	$0,FUNC(workQIsEmpty)		/* if work q is not empty */
	je	doWorkUnlock			/* then unlock and do work */

#ifdef WV_INSTRUMENTATION

	/* windview instrumentation - BEGIN
	 * exit windExit with dispatch;
	 */

        cmpl    $0,FUNC(evtAction)		/* is WindView on? */
        je      noInst3

	pushl	%eax				/* save regs */
	pushl	%edx
	pushl	%ecx

	movl	$ WV_CLASS_1_ON,%eax
        andl    FUNC(wvEvtClass),%eax		/* is event collection on? */
        cmpl    $ WV_CLASS_1_ON,%eax		/* is event collection on? */
        jne     trgCheckInst3

	movl	FUNC(_func_evtLogTSched),%edx	/* event log routine */
	cmpl	$0,%edx
        je      trgCheckInst3

        movl	FUNC(taskIdCurrent),%eax	/* current task */
        movl	WIND_TCB_PRIORITY(%eax),%ecx

        pushl	%ecx	                        /* WIND_TCB_PRIORITY */
        pushl	%eax	                        /* taskIdCurrent */
        /* Here we try to determine if the task is running at an
         * inherited priority, if so a different event is generated.
         */
        cmpl	WIND_TCB_PRI_NORMAL(%eax),%ecx
        jge     noInst3Inheritance		/* no inheritance */
        pushl	$ EVENT_WIND_EXIT_DISPATCH_PI
        jmp     inst3Inheritance		/* no inheritance */
noInst3Inheritance:
        pushl	$ EVENT_WIND_EXIT_DISPATCH	/* event id */
inst3Inheritance:
        call    *%edx                           /* call evtsched routine */

	addl    $12,%esp

trgCheckInst3:
	movl	$ TRG_CLASS_1,%eax     
	orl 	$ TRG_ON,%eax
        cmpl    FUNC(trgEvtClass),%eax		/* any trigger? */
        jne      inst3Clean

        movl    FUNC(_func_trgCheck),%edx	/* triggering routine */
	cmpl	$0,%edx
        je      inst3Clean

        movl	FUNC(taskIdCurrent),%eax	/* current task */
        movl	WIND_TCB_PRIORITY(%eax),%ecx

        pushl   $ 0                     /* 0 */
        pushl   $ 0                     /* 0 */
        pushl   $ 0                     /* 0 */
        pushl	%ecx	                /* WIND_TCB_PRIORITY */
        pushl	%eax	                /* taskIdCurrent */
        pushl   $ 0                     /* obj */
        pushl   $ 0                     /* TRG_CLASS1_INDEX */

        /* Here we try to determine if the task is running at an
         * inherited priority, if so a different event is generated.
         */
        cmpl	WIND_TCB_PRI_NORMAL(%eax),%ecx
        jge     noTrgInst3Inheritance		/* no inheritance */
        pushl	$ EVENT_WIND_EXIT_DISPATCH_PI
        jmp     trgInst3Inheritance		/* no inheritance */
noTrgInst3Inheritance:
        pushl	$ EVENT_WIND_EXIT_DISPATCH	/* event id */
trgInst3Inheritance:

        call    *%edx                   /* call triggering routine */

	addl    $32,%esp

inst3Clean:
	popl	%ecx			/* restore regs */
	popl	%edx
	popl	%eax
noInst3:


	/* windview instrumentation - END */

#endif  /* WV_INSTRUMENTATION */

	movl	$0,FUNC(kernelState)		/* release kernel mutex */
	iret					/* UNLOCK INTERRUPTS */

/*******************************************************************************
*
* idle - spin here until there is more work to do
*
* When the kernel is idle, we spin here continually checking for work to do.
*/

	.balign 16,0x90
idle:

#ifdef WV_INSTRUMENTATION

        /* windview instrumentation - BEGIN
         * enter idle state
         *
         * NOTE: I am making the assumption here that it is okay to
         * modify the %eax,%edx,%ecx registers. They are not being saved.
         */


        cmpl    $0,FUNC(evtAction)	/* is WindView on? */
        je      noInstIdle

	pushal
	cli				/* LOCK INTERRUPTS */

	movl	$ WV_CLASS_1_ON,%eax
        andl    FUNC(wvEvtClass),%eax	/* is event collection on? */
        cmpl    $ WV_CLASS_1_ON,%eax	/* is event collection on? */
        jne     trgCheckInstIdle

	movl	FUNC(_func_evtLogT0),%edx /* event log routine */
	cmpl	$0,%edx
        je      trgCheckInstIdle

        pushl	$ EVENT_WIND_EXIT_IDLE  /* event id */

	call	*%edx			/* call event log routine */	

	addl    $4,%esp

trgCheckInstIdle:
	movl	$ TRG_CLASS_1,%eax     
	orl 	$ TRG_ON,%eax
        cmpl    FUNC(trgEvtClass),%eax		/* any trigger? */
        jne     instIdleClean

        movl    FUNC(_func_trgCheck),%edx	/* triggering routine */
	cmpl	$0,%edx
	je	instIdleClean

        pushl   $ 0                     /* 0 */
        pushl   $ 0                     /* 0 */
        pushl   $ 0                     /* 0 */
        pushl   $ 0                     /* 0 */
        pushl   $ 0                     /* 0 */
        pushl   $ 0                     /* obj */
        pushl   $ 0                     /* TRG_CLASS1_INDEX */
        pushl   $ EVENT_WIND_EXIT_IDLE	/* EVENT_WIND_EXIT_IDLE */

        call    *%edx                   /* call triggering routine */

	addl 	$32,%esp 


instIdleClean:
	popal

noInstIdle:
	/* windview instrumentation - END */
#endif  /* WV_INSTRUMENTATION */

	sti				/* UNLOCK INTERRUPTS (just in case) */
	movl	$1,FUNC(kernelIsIdle)	/* set idle flag for spyLib */
idleLoop:
        cmpl    $0,FUNC(workQIsEmpty)   /* if work queue has work to do */
        je      goDoWork		/* no more idling */

	/* PAUSE instruction operates like a NOP in earlier IA-32 processors */

        .byte   0xf3, 0x90              /* PAUSE should be in the loop */

#ifdef X86_POWER_MANAGEMENT

	cmp     $0, FUNC(vxIdleRtn)	/* is Power Management enabled ? */
	je	idleLoop		/* no: stay in loop */
        call    *FUNC(vxIdleRtn)        /* call routine set by vxPowerModeSet */
#else
        jmp     idleLoop                /* keep hanging around */

#endif /* X86_POWER_MANAGEMENT */

goDoWork:	/* there is some work to do, can't remain idle now */
        movl    $0,FUNC(kernelIsIdle)   /* unset idle flag for spyLib */
        jmp     doWork                  /* go do the work */


#ifdef X86_POWER_MANAGEMENT
/*******************************************************************************
*
* vxIdleAutoHalt - place the processor in AutoHalt mode when nothing to do
*/

        .balign 16,0x90
FUNC_LABEL(vxIdleAutoHalt)
        sti                             /* make sure interrupts are enabled */
        nop                             /* delay a bit */
        cmpl    $0,FUNC(workQIsEmpty)   /* if work queue is still empty */
        je      vpdRet                  /* there is work to do - return */
        hlt                             /* nothing to do - go in AutoHalt */
vpdRet:
        ret

#endif /* X86_POWER_MANAGEMENT */


/*******************************************************************************
*
* doSwapHooks - execute the tasks' swap hooks
*/

	.balign 16,0x90
doSwapHooks:
	pushl	%eax			/* push pointer to new tcb */
	pushl	%edx			/* push pointer to old tcb */
	leal	FUNC(taskSwapTable),%edi /* get adrs of task switch rtn list */
	movl	$-4,%esi		/* start index at -1, heh heh */
	jmp	doSwapShift		/* jump into the loop */

	.balign 16,0x90
doSwapHook:
	movl	(%esi,%edi,1),%ecx	/* call task switch rtn into r3+4*n */
	call	*%ecx

doSwapShift:
	addl	$4,%esi			/* bump swap table index */
	shlw	$1,%bx			/* shift swapMask bit pattern left */
	jc	doSwapHook		/* if carry bit set then do ix hook */
	jne	doSwapShift		/* any bits still set */
					/* no need to clean stack */
	movl	FUNC(taskIdCurrent),%eax /* restore %eax with taskIdCurrent */
	cmpl	$0,FUNC(taskSwitchTable) /* any global switch hooks? */
	je	dispatch		/* if no then dispatch taskIdCurrent */
	jmp	doSwitchFromSwap	/* do switch routines from swap */

/*******************************************************************************
*
* doSwitchHooks - execute the global switch hooks
*/

	.balign 16,0x90
doSwitchHooks:
	pushl	%eax			/* push pointer to new tcb */
	pushl	%edx			/* push pointer to old tcb */

doSwitchFromSwap:
	leal	FUNC(taskSwitchTable),%edi /* get adrs of task switch rtn list */
	movl	(%edi),%esi		/* get task switch rtn into %esi */

doSwitchHook:
	call	*%esi			/* call routine */
	addl	$4,%edi			/* bump to next task switch routine */
	movl	(%edi),%esi		/* get next task switch rtn */
	cmpl	$0,%esi
	jne	doSwitchHook		/* check for end of table (NULL) */
					/* no need to clean stack */
	movl	FUNC(taskIdCurrent),%eax /* restore %eax with taskIdCurrent */
	jmp	dispatch		/* dispatch task */

/*******************************************************************************
*
* doWork - empty the work queue
* doWorkUnlock - unlock interrupts and empty the work queue
*/

	.balign 16,0x90
doWorkUnlock:
	sti				/* UNLOCK INTERRUPTS */
doWork:
	call	FUNC(workQDoWork)	/* empty the work queue */
	movl	FUNC(taskIdCurrent),%edx /* %edx = taskIdCurrent */
	movl	FUNC(readyQHead),%eax	/* %eax = highest task */
	cmpl	$0,%eax
	je	idle			/* nobody is ready so spin */
	cmpl	%edx,%eax		/* compare to last task */
	je	dispatch		/* if the same dispatch */
	jmp	switchTasks		/* not same, do switch */

#endif	/* PORTABLE */

#ifdef PORTABLE
/*******************************************************************************
*
* windLoadContext - load the register context from the control block
*
* The registers of the current executing task, (the one reschedule chose),
* are restored from the control block.  Then the appropriate exception frame
* for the architecture being used is constructed.  To unlock interrupts and
* enter the new context we simply use the instruction rte.
*
* NOMANUAL

* void windLoadContext ()

*/

	.balign 16,0x90
FUNC_LABEL(windLoadContext)
	movl	FUNC(taskIdCurrent),%eax	/* current tid */
	movl	WIND_TCB_ERRNO(%eax),%ecx	/* save errno */
	movl	%ecx,FUNC(errno)
	movl	WIND_TCB_ESP(%eax),%esp		/* push dummy except. */

	pushl	WIND_TCB_EFLAGS(%eax)		/* push eflags */
	pushl	FUNC(sysCsSuper)		/* push CS */
	pushl	WIND_TCB_PC(%eax)		/* push pc */

	movl	WIND_TCB_EDX(%eax),%edx		/* restore registers */
	movl	WIND_TCB_ECX(%eax),%ecx
	movl	WIND_TCB_EBX(%eax),%ebx
	movl	WIND_TCB_ESI(%eax),%esi
	movl	WIND_TCB_EDI(%eax),%edi
	movl	WIND_TCB_EBP(%eax),%ebp
	movl	WIND_TCB_EAX(%eax),%eax

	iret					/* enter task's context. */

#endif	/* PORTABLE */

/*******************************************************************************
*
* intEnt - enter an interrupt service routine
*
* intEnt must be called at the entrance of an interrupt service routine.
* This normally happens automatically, from the stub built by intConnect (2).
* This routine should NEVER be called from C.
*
* SEE ALSO: intConnect(2)

* void intEnt ()

*/

	.balign 16,0x90
FUNC_LABEL(intEnt)
	cli				/* LOCK INTERRUPTS */
	pushl	(%esp)			/* bump return address up a notch */
	pushl	%eax
	movl	FUNC(errno),%eax	/* save errno where return adress was */
	movl	%eax,8(%esp)
	incl	FUNC(intCnt)		/* increment the counter */
	incl	FUNC(intNest)		/* increment the private counter */

#ifdef	INT_STACK_USE
	/*
	 * switch to the interrupt stack from the supervisor stack 
	 * used by a task running in the supervisor mode
	 *
	 * if we are already in the interrupt stack, no stack switch will
	 * happen.  It happens in the following cases:
	 *   int - exc(break/trace) - int
	 *   int - int - exc(break/trace) - int
	 * To detect if the interrupt stack is already in use, the
	 * private counter intNest is used.  Checking the CS in ESF does
	 * not work in the above cases.  But the checking the CS in ESF
	 * is necessary to detect the nesting interrupt before and after
	 * the counter is manipulated.
         *
         * stack growth in the 1st interrupt
	 *   (supervisor stack)            (interrupt stack)
	 *   ----------------------------------------------
	 *     :                 +-----     supervisor SP
	 *     ESF(12 bytes)	 |          ESF(12 bytes)
	 *     errno       <-----+	    errno
	 *     return addr       	    : return addr
	 *     %eax			    : %eax
	 *     : %ecx			    V
	 *     : %esi			    
	 *     : %edi			    
	 *     :
	 *     V
	 *
         * stack growth in the 2nd interrupt (nested interrupt)
	 *   (supervisor stack)            (interrupt stack)
	 *   ----------------------------------------------
	 *     :                 +-----     supervisor SP
	 *     ESF(12 bytes)	 |          ESF(12 bytes)    --- 1st int
	 *     errno       <-----+	    errno            --- 1st int
	 *     return addr       	    :
	 *     %eax			    ESF(12 bytes)    --- 2nd int
	 *     : %ecx			    errno            --- 2nd int
	 *     : %esi			    : return addr
	 *     : %edi			    : %eax
	 *     :			    V
	 *     V
	 *
	 */

	cmpl	$0,FUNC(vxIntStackEnabled) /* if vxIntStackEnabled == 0 then */
	je	intEnt0			/* skip the interrupt stack switch */

	movl	ESF0_CS+12(%esp), %eax	/* get CS in ESF0 */
	cmpw	FUNC(sysCsInt), %ax	/* is it nested interrupt ? */
	je	intEnt0			/*   yes: skip followings */

	cmpl	$1,FUNC(intNest)	/* already in the interrupt stack? */
	jne	intEnt0			/* skip the interrupt stack switch */

	/* copy the supervisor stack to the interrupt stack. */

intEntStackSwitch:
	pushl	%ecx			/* save %ecx */
	pushl	%esi			/* save %esi */
	pushl	%edi			/* save %edi */

	/* copy ESF0(12 bytes), errno, return addr, %eax */

	subl	$ ESF0_NBYTES+12+4, FUNC(vxIntStackPtr) /* alloc */
	movl	FUNC(vxIntStackPtr), %eax  /* get int-stack ptr */
	leal	20(%esp), %ecx		   /* get addr of errno */
	movl	%ecx, ESF0_NBYTES+12(%eax) /* save the original ESP */

	leal	12(%esp), %esi		/* set the source addr */
	movl	%eax, %edi		/* set the destination addr */
	movl	$ ESF0_NLONGS+3, %ecx	/* set number of longs to copy */
	cld				/* set direction ascending order */
	rep				/* repeat next inst */
	movsl				/* copy ESF0_NLONGS + 3 longs */

	popl	%edi			/* restore %edi */
	popl	%esi			/* restore %esi */
	popl	%ecx			/* restore %ecx */

	movl	%eax, %esp		/* switch to the interrupt stack */

	/* now, we are in the interrupt stack */

#endif	/* INT_STACK_USE */

intEnt0:
	pushl	ESF0_EFLAGS+12(%esp)	/* push the saved EFLAGS */
	popfl				/* UNLOCK INTERRUPT */
	popl	%eax			/* restore %eax */

#ifdef WV_INSTRUMENTATION

	/* windview instrumentation - BEGIN
	 * enter an interrupt handler. 
	 *
	 * ALL registers must be saved.  
	 */

        cmpl    $0,FUNC(evtAction)		/* is WindView on? */
        je	noIntEnt
	pushal                          	/* save regs */
	pushfl					/* save EFLAGS */
	cli					/* LOCK INTERRUPTS */

	movl	32+4(%esp),%edi		/* use %edi to store the return address */

	movl	$ WV_CLASS_1_ON,%eax
        andl    FUNC(wvEvtClass),%eax		/* is event collection on? */
        cmpl    $ WV_CLASS_1_ON,%eax		/* is event collection on? */
        jne     trgCheckIntEnt

	movl	INT_CONNECT_CODE29(%edi), %eax	/* get IRQ number */
	addl    $ MIN_INT_ID,%eax 		/* get event ID */

	movl	FUNC(_func_evtLogT0),%edx	/* event log routine */
	cmpl	$0,%edx
	je	trgCheckIntEnt

	pushl   %eax

	call	*%edx				/* call event log routine */	

	addl    $4,%esp

trgCheckIntEnt:
	movl	$ TRG_CLASS_1,%eax     
	orl 	$ TRG_ON,%eax
        cmpl    FUNC(trgEvtClass),%eax		/* any trigger? */
        jne     intEntClean

	movl	INT_CONNECT_CODE29(%edi), %eax	/* get IRQ number */
	addl    $ MIN_INT_ID,%eax 		/* get event ID */

        movl    FUNC(_func_trgCheck),%edx	/* triggering routine */
	cmpl	$0,%edx
        je      intEntClean

        pushl   $ 0                     	/* 0 */
        pushl   $ 0                     	/* 0 */
        pushl   $ 0                     	/* 0 */
        pushl   $ 0                     	/* 0 */
        pushl   $ 0                     	/* 0 */
        pushl   $ 0                     	/* obj */
        pushl   $ 0                     	/* TRG_CLASS1_INDEX */
        pushl   %eax                    	/* push event ID */

        call    *%edx                   	/* call triggering routine */

	addl    $32,%esp

intEntClean:

	popfl					/* restore EFLAGS */
	popal					/* restore regs */

noIntEnt:
	/* windview instrumentation - END */

#endif  /* WV_INSTRUMENTATION */

	ret

/*******************************************************************************
*
* intExit - exit an interrupt service routine
*
* Check the kernel ready queue to determine if resheduling is necessary.  If
* no higher priority task has been readied, and no kernel work has been queued,
* then we return to the interrupted task.
*
* If rescheduling is necessary, the context of the interrupted task is saved
* in its associated TCB with the PC, EFLAGS and EFLAGS retrieved from the 
* exception frame on the master stack.
*
* This routine must be branched to when exiting an interrupt service routine.
* This normally happens automatically, from the stub built by intConnect (2).
*
* This routine can NEVER be called from C.
*
* It can only be jumped to because a jsr will push a return address on the
* stack.
*
* SEE ALSO: intConnect(2)

* void intExit ()

* INTERNAL
* This routine must preserve all registers up until the context is saved,
* so any registers that are used to check the queues must first be saved on
* the stack.
*
* At the call to reschedule the value of taskIdCurrent must be in edx.
*/

	.balign 16,0x90
FUNC_LABEL(intExit)
	popl	FUNC(errno)		/* restore errno */
	pushl	%eax			/* push %eax onto the stack */

#ifdef WV_INSTRUMENTATION

        /* windview instrumentation - BEGIN
         * log event if work has been done in the interrupt handler.
         * NOTE: a0 is still on the stack
         */

        cmpl    $0,FUNC(evtAction)		/* is WindView on? */
        je	noIntExit
        pushl	%edx
        pushl	%ecx
	pushfl					/* save EFLAGS */
	cli					/* LOCK INTERRUPTS */

	movl	$ WV_CLASS_1_ON,%eax
        andl    FUNC(wvEvtClass),%eax		/* is event collection on? */
        cmpl    $ WV_CLASS_1_ON,%eax		/* is event collection on? */
        jne     trgCheckIntExit

	movl	FUNC(_func_evtLogT0),%edx	/* event log routine */
	cmpl	$0,%edx
	je	trgCheckIntExit

        cmpl	$0,FUNC(workQIsEmpty)		/* work in work queue? */
        jne     intExitEvent
        pushl	$ EVENT_INT_EXIT_K		/* event id */
        jmp     intExitCont
intExitEvent:
        pushl	$ EVENT_INT_EXIT		/* event id */
intExitCont:

	call	*%edx				/* call event log routine */	

	addl 	$4,%esp 
trgCheckIntExit:
	movl	$ TRG_CLASS_1,%eax     
	orl 	$ TRG_ON,%eax
        cmpl    FUNC(trgEvtClass),%eax		/* any trigger? */
        jne      intExitClean

        movl    FUNC(_func_trgCheck),%edx	/* triggering routine */
	cmpl	$0,%edx
        je      intExitClean

        pushl   $ 0                     	/* 0 */
        pushl   $ 0                     	/* 0 */
        pushl   $ 0                     	/* 0 */
        pushl   $ 0                     	/* 0 */
        pushl   $ 0                     	/* 0 */
        pushl   $ 0                     	/* obj */
        pushl   $ 0                     	/* TRG_CLASS1_INDEX */

        cmpl	$0,FUNC(workQIsEmpty)		/* work in work queue? */
        jne     trgIntExitEvent
        pushl	$ EVENT_INT_EXIT_K		/* event id */
        jmp     trgIntExitCont
trgIntExitEvent:
        pushl	$ EVENT_INT_EXIT		/* event id */
trgIntExitCont:
        call    *%edx                   	/* call triggering routine */

	addl    $32,%esp

intExitClean:
	popfl					/* UNLOCK INTERRUPTS */
        popl	%ecx				/* restore regs */
        popl	%edx
noIntExit:
        /* windview instrumentation - END */

#endif  /* WV_INSTRUMENTATION */

	cli					/* LOCK INTERRUPTS */
	decl	FUNC(intCnt)			/* decrement intCnt */
	decl	FUNC(intNest)			/* decrement the private counter */

        movl	ESF0_CS+4(%esp),%eax		/* if CS on stack is sysCsInt, */
        cmpw	FUNC(sysCsInt),%ax		/*  then we were in an ISR */
        je	intRte				/*  so just clean up/rte */

#ifdef	INT_STACK_USE
	/*
	 * switch to the supervisor stack from the interrupt stack
	 *
         * stack growth in the 1st interrupt
	 *   (supervisor stack)            (interrupt stack)
	 *   ----------------------------------------------
	 *     :                 +-----     supervisor SP
	 *     ESF(12 bytes)	 |          ESF(12 bytes)    --- 1st int
	 *     errno       <-----+	    : errno / %eax   --- 1st int
	 *     :                            : %eax
	 *     :                            :
	 *     V                            V
	 *
         * stack growth in the 2nd interrupt
	 *   (supervisor stack)            (interrupt stack)
	 *   ----------------------------------------------
	 *     :                 +-----     supervisor SP
	 *     ESF(12 bytes)	 |          ESF(12 bytes)    --- 1st int
	 *     errno       <-----+	    errno            --- 1st int
	 *     :                            :
	 *     :                            ESF(12 bytes)    --- 2nd int
	 *     :                            : errno / %eax   --- 2nd int
	 *     V                            : %eax
	 *                                  :
	 *                                  V
	 *
	 */

	cmpl	$0,FUNC(vxIntStackEnabled) /* if vxIntStackEnabled == 0 then */
	je	intExit0		/* skip the interrupt stack switch */

	cmpl    $0, FUNC(intNest)	/* is it nested interrupt ? */
	jne     intExit0		/*   yes: goto intRteInt */

	popl	%eax			/* restore %eax */
	addl	$ ESF0_NBYTES+12+4, FUNC(vxIntStackPtr) /* free */
	movl	ESF0_NBYTES(%esp), %esp /* switch to supervisor stack */
	popl	FUNC(errno)		/* restore errno */
	pushl	%eax			/* save %eax */

#endif	/* INT_STACK_USE */

intExit0:
	cmpl	$0,FUNC(kernelState)	/* if kernelState == TRUE then */
	jne	intRte			/*  just clean up and rte */

	movl	FUNC(taskIdCurrent),%eax /* put current task in %eax */
	cmpl	FUNC(readyQHead),%eax 	/* compare to highest ready task */
	je	intRte			/* if same then don't reschedule */

	cmpl	$0,WIND_TCB_LOCK_CNT(%eax) /* is task preemption allowed */
	je	saveIntContext		/* if yes, then save context */
	cmpl	$0,WIND_TCB_STATUS(%eax) /* is task ready to run */
	jne	saveIntContext		/* if no, then save context */

intRte:
	popl	%eax			/* restore %eax */
	iret				/* UNLOCK INTERRUPTS */

/* We are here if we have decided that rescheduling is a distinct possibility.
 * The context must be gathered and stored in the current task's tcb.
 * The stored stack pointers must be modified to clean up the stacks (SP).
 */

	.balign 16,0x90
saveIntContext:
	/* interrupts are still locked out */
	movl	$1,FUNC(kernelState)		/* kernelState = TRUE; */
	movl	FUNC(taskIdCurrent),%eax	/* tcb to be fixed up */
	popl	WIND_TCB_EAX(%eax)		/* store %eax in tcb */
	popl	WIND_TCB_PC(%eax)		/* save pc in tcb */
	leal	4(%esp),%esp			/* do not save %cs in tcb */
	popl	WIND_TCB_EFLAGS(%eax)		/* save eflags in tcb */
	sti					/* UNLOCK INTERRUPTS */

	/* interrupts unlocked and using master stack*/
	movl	%edx,WIND_TCB_EDX(%eax)		/* save %edx */
	movl	%ecx,WIND_TCB_ECX(%eax)		/* save %ecx */
	movl	%ebx,WIND_TCB_EBX(%eax)		/* save %ebx */
	movl	%esi,WIND_TCB_ESI(%eax)		/* save %esi */
	movl	%edi,WIND_TCB_EDI(%eax)		/* save %edi */
	movl	%ebp,WIND_TCB_EBP(%eax)		/* save %ebp */
	movl	%esp,WIND_TCB_ESP(%eax)		/* save %esp */

	movl	FUNC(errno),%edx		/* save errno */
	movl	%edx,WIND_TCB_ERRNO(%eax)
	movl	%eax,%edx			/* taskIdCurrent into %edx */
	jmp	FUNC(reschedule)		/* goto rescheduler */

/*******************************************************************************
*
* vxTaskEntry - task startup code following spawn
*
* This hunk of code is the initial entry point to every task created via
* the "spawn" routines.  taskCreate(2) has put the true entry point of the
* task into the tcb extension before creating the task,
* and then pushed exactly ten arguments (although the task may use
* fewer) onto the stack.  This code picks up the real entry point and calls it.
* Upon return, the 10 task args are popped, and the result of the main
* routine is passed to "exit" which terminates the task.
* This way of doing things has several purposes.  First a task is easily
* "restartable" via the routine taskRestart(2) since the real
* entry point is available in the tcb extension.  Second, the call to the main
* routine is a normal call including the usual stack clean-up afterwards,
* which means that debugging stack trace facilities will handle the call of
* the main routine properly.
*
* NOMANUAL

* void vxTaskEntry ()

*/

	.balign 16,0x90
FUNC_LABEL(vxTaskEntry)
	xorl	%ebp,%ebp		/* make sure frame pointer is 0 */
	movl	FUNC(taskIdCurrent),%eax /* get current task id */
	movl	WIND_TCB_ENTRY(%eax),%eax /* entry point for task is in tcb */
	call	*%eax			/* call main routine */
	addl	$40,%esp		/* pop args to main routine */
	pushl	%eax			/* pass result to exit */
	call	FUNC(exit)		/* gone for good */

/*******************************************************************************
*
* windIntStackSet - set the interrupt stack pointer
*
* This routine sets the interrupt stack pointer to the specified address.
* It is only valid on architectures with an interrupt stack pointer.
* For I80X86, the switch to/from the interrupt stack is done by software.
*
* NOMANUAL

* void windIntStackSet (pBotStack)
*     char *pBotStack;	/* pointer to bottom of interrupt stack *

*/

	.balign 16,0x90
FUNC_LABEL(windIntStackSet)
	movl	SP_ARG1(%esp), %eax	/* get pBotStack */
	movl	%eax, FUNC(vxIntStackPtr) /* set it to vxIntStackPtr */
	ret

/*******************************************************************************
*
* intStackEnable - enable the interrupt stack usage
*
* This routine enables the interrupt stack usage.
* This routine is only callable from the task level, returns ERROR otherwise.
* The interrupt stack usage is disabled in the default configuration 
* for the backward compatibility.
*
* RETURNS: OK, or ERROR if it is not in the task level.

* STATUS intStackEnable 
*    (
*    BOOL enable		/@ TRUE to enable, FALSE to disable @/
*    )

*/

	.balign 16,0x90
FUNC_LABEL(intStackEnable)
	movl	%cs, %eax
	cmpl	FUNC(sysCsSuper), %eax	/* is the CS for the task level? */
	jne	intStackEnableError	/*   no: return ERROR */

	xorl	%eax, %eax		/* zero for OK */
	movl	SP_ARG1(%esp), %edx	/* get & set the parameter */
	
	/* next "movl" instruction is atomic, so no intLock/Unlock is needed */

	movl	%edx, FUNC(vxIntStackEnabled)

	ret

intStackEnableError:
	movl	$ ERROR, %eax
	ret

