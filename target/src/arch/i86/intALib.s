/* intALib.s - I80x86 interrupt library assembly routines */

/* Copyright 1984-2001 Wind River Systems, Inc. */

/*
modification history
--------------------
01h,20nov01,hdn  doc clean up for 5.5
01g,23aug01,hdn  added FUNC/FUNC_LABEL, replaced .align with .balign
01f,04jun98,hdn  fixed intBoiExit()'s stack pop count.
01e,28may98,hdn  added intBoiExit() for spurious/phantom int support.
01d,26jan95,rhp  doc tweaks from architecture-independent man pages	
01c,01jun93,hdn  updated to 5.1.
		  - fixed #else and #endif
		  - changed VOID to void
		  - changed ASMLANGUAGE to _ASMLANGUAGE
		  - changed copyright notice
01c,18nov92,hdn  changed intLock and intUnlock.
01b,13oct92,hdn  debugged.
01a,28feb92,hdn  written based on TRON, 68k version.
*/

/*
DESCRIPTION
This library supports various functions associated with
interrupts from C.  

SEE ALSO
intLib, intArchLib

INTERNAL
Some routines in this module set up the "C" frame pointer (ebp) 
although they don't use it in any way!  This is only for the benefit of
the stacktrace facility to allow it to properly trace a stack of the 
task executing within these routines.
*/

#define _ASMLANGUAGE
#include "vxWorks.h"
#include "asm.h"
#include "regs.h"


	.data
	.globl	FUNC(copyright_wind_river)
	.long	FUNC(copyright_wind_river)
	

	/* internals */

	.globl	GTEXT(intLevelSet)
	.globl	GTEXT(intLock)
	.globl	GTEXT(intUnlock)
	.globl	GTEXT(intVBRSet)
	.globl	GTEXT(intBoiExit)


	.text
	.balign 16

/*******************************************************************************
*
* intLevelSet - set the interrupt level
*
* An I80x86 doesn't have an interrupt level (unlike MC680x0, for example); 
* thus this routine is a no-op.	
*
* RETURNS: 0.

* int intLevelSet (level)
*     int level;	/* new interrupt level mask *

*/

FUNC_LABEL(intLevelSet)
	xorl	%eax, %eax
	ret

/*******************************************************************************
*
* intLock - lock out interrupts
*
* This routine disables interrupts.
* 
* For x86 architectures, interrupts are disabled at the level set by 
* intLockLevelSet().  The default lock-out level is the highest interrupt 
* level (1).
* 
* The routine returns the interrupt enable flag (IF) bit from the
* EFLAGS register as the lock-out key for the interrupt level prior to
* the call, and this should be passed to intUnlock() to re-enable
* interrupts.
* 
* WARNINGS
* Do not call VxWorks system routines with interrupts locked.
* Violating this rule may re-enable interrupts unpredictably.
*
* The routine intLock() can be called from either interrupt or task level.
* When called from a task context, the interrupt lock level is part of the
* task context.  Locking out interrupts does not prevent rescheduling.
* Thus, if a task locks out interrupts and invokes kernel services that
* cause the task to block (e.g., taskSuspend() or taskDelay()) or that cause a
* higher priority task to be ready (e.g., semGive() or taskResume()), then
* rescheduling will occur and interrupts will be unlocked while other tasks
* run.  Rescheduling may be explicitly disabled with taskLock().
* Traps must be enabled when calling this routine.
* 
* EXAMPLE
* .CS
*     lockKey = intLock ();
*
*         ...    /* work with interrupts locked out *
*
*     intUnlock (lockKey);
* .CE
* 
*
* To lock out interrupts and task scheduling as well (see WARNING above):
* .CS
*     if (taskLock() == OK)
*         {
*         lockKey = intLock ();
*
*         ... (critical section)
*
*         intUnlock (lockKey);
*         taskUnlock();
*         }
*      else
*         {
*         ... (error message or recovery attempt)
*         }
* .CE
*
* RETURNS
* The interrupt enable flag (IF) bit from the EFLAGS register, as the
* lock-out key for the lock-out level prior to the call.
* 
* SEE ALSO: intUnlock(), taskLock()

* int intLock ()

*/

	.balign 16,0x90
FUNC_LABEL(intLock)
	pushf				/* push EFLAGS on stack */
	popl	%eax			/* get EFLAGS in EAX */
	andl	$ EFLAGS_IF,%eax	/* mask it with IF bit */
	cli				/* LOCK INTERRUPTS */
	ret

/*******************************************************************************
*
* intUnlock - cancel interrupt locks
* 
* This routine re-enables interrupts that have been disabled by intLock().
* Use the lock-out key obtained from the preceding intLock() call.
*
* RETURNS: N/A
*
* SEE ALSO: intLock()

* void intUnlock (oldSR)
*    int oldSR;

*/

	.balign 16,0x90
FUNC_LABEL(intUnlock)
	movl	SP_ARG1(%esp),%eax	/* get oldSR in EAX */
	andl	$ EFLAGS_IF,%eax	/* is IF bit set in oldSR? */
	jz	intUnlock0		/*   no: skip next instruction */
	sti				/* UNLOCK INTERRUPTS */

intUnlock0:
	ret

/*******************************************************************************
*
* intVBRSet - set the interrupt table descriptor register
*
* This routine should only be called in supervisor mode.
*
* NOMANUAL

* void intVBRSet (baseAddr)
*      char *baseAddr;	/* vector base address *

*/

	.balign 16,0x90
FUNC_LABEL(intVBRSet)
	movl	SP_ARG1(%esp),%eax
	lidt	(%eax)
	ret

/*******************************************************************************
*
* intBoiExit - exit the interrupt handler stub without sending EOI
*
* This routine exits the interrupt handler stub without sending EOI.
* It should be call from BOI routine attached by intConnect().
* EOI should not be sent if it is the fantom/spurious interrupt,
* such as IRQ7 or IRQ15 in PIC.
*
* NOMANUAL

* void intBoiExit (void)

*/

	.balign 16,0x90
FUNC_LABEL(intBoiExit)
	addl	$4, %esp		/* pop param */
	popl	%ecx			/* restore regs */
	popl	%edx
	popl	%eax
	jmp	FUNC(intExit)		/* exit via kernel */
