/* workQALib.s - i80x86 internal VxWorks kernel work queue assembly library */

/* Copyright 1984-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01i,20mar02,hdn  preserved previous state of the int enable bit (spr 74016)
01h,23aug01,hdn  added FUNC/FUNC_LABEL, replaced .align w .balign
01g,25feb99,nps  remove obsolete scrPad reference.
01f,17feb98,pr   deleted Windview code (no more needed).
01e,29jul96,sbs  Made windview conditionally compile.
01d,08aug94,hdn  added support for WindView.
01c,17nov93,hdn  fixed a bug: changed movb to movzbl.
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
#include "asm.h"
#include "private/workQLibP.h"
#include "private/eventP.h"


	.data
	.globl	FUNC(copyright_wind_river)
	.long	FUNC(copyright_wind_river)


#ifndef PORTABLE

	/* externals */

	.globl	FUNC(workQPanic)
	.globl	VAR(workQReadIx)
	.globl	VAR(workQWriteIx)
	.globl	VAR(workQIsEmpty)


	/* internals */

	.globl	GTEXT(workQAdd0)	/* add function to workQ */
	.globl	GTEXT(workQAdd1)	/* add function and 1 arg to workQ */
	.globl	GTEXT(workQAdd2)	/* add function and 2 args to workQ */
	.globl	GTEXT(workQDoWork)	/* do all queued work in a workQ */


	.text
	.balign 16

/*******************************************************************************
*
* workQOverflow - work queue has overflowed so call workQPanic ()
*
* NOMANUAL
*/  

workQOverflow:					/* leave interrupts locked */
	call	FUNC(workQPanic)		/* panic and never return */

/*******************************************************************************
*
* workQAdd0 - add a function with no argument to the work queue
*
* NOMANUAL

* void workQAdd0 (func)
*     FUNCPTR func;	/@ function to invoke @/

*/  

	.balign 16,0x90
FUNC_LABEL(workQAdd0)
	pushfl					/* save IF in EFLAGS */
	cli					/* LOCK INTERRUPTS */
	movzbl	FUNC(workQWriteIx),%eax		/* get write index into %eax */
	movl	%eax,%ecx
	addb	$4,%al				/* advance write index */
	cmpb	FUNC(workQReadIx),%al		/* overflow? */
	je	workQOverflow			/* panic if overflowed */
	movb	%al,FUNC(workQWriteIx)		/* update write index */
	popfl					/* UNLOCK INTERRUPTS */
	movl	$0,FUNC(workQIsEmpty)		/* work queue isn't empty */
	leal	FUNC(pJobPool)(,%ecx,4),%edx	/* get a start of an entry */
	movl	SP_ARG1(%esp),%eax		/* move it to pool */
	movl	%eax,JOB_FUNCPTR(%edx)
	ret					/* we're done */

/*******************************************************************************
*
* workQAdd1 - add a function with one argument to the work queue
*
* NOMANUAL

* void workQAdd1 (func, arg1)
*     FUNCPTR func;	/@ function to invoke 		@/
*     int arg1;		/@ parameter one to function	@/

*/  

	.balign 16,0x90
FUNC_LABEL(workQAdd1)
	pushfl					/* save IF in EFLAGS */
	cli					/* LOCK INTERRUPTS */
	movzbl	FUNC(workQWriteIx),%eax		/* get write index into %eax */
	movl	%eax,%ecx
	addb	$4,%al				/* advance write index */
	cmpb	FUNC(workQReadIx),%al		/* overflow? */
	je	workQOverflow			/* panic if overflowed */
	movb	%al,FUNC(workQWriteIx)		/* update write index */
	popfl					/* UNLOCK INTERRUPTS */
	movl	$0,FUNC(workQIsEmpty)		/* work queue isn't empty */
	leal	FUNC(pJobPool)(,%ecx,4),%edx	/* get the start of an entry */
	movl	SP_ARG1(%esp),%eax		/* move the function to pool */
	movl	%eax,JOB_FUNCPTR(%edx)
	movl	SP_ARG2(%esp),%eax		/* move the argument to pool */
	movl	%eax,JOB_ARG1(%edx)
	ret					/* we're done */

/*******************************************************************************
*
* workQAdd2 - add a function with two arguments to the work queue
*
* NOMANUAL

* void workQAdd2 (func, arg1, arg2)
*     FUNCPTR func;	/@ function to invoke 		@/
*     int arg1;		/@ parameter one to function	@/
*     int arg2;		/@ parameter two to function	@/

*/  

	.balign 16,0x90
FUNC_LABEL(workQAdd2)
	pushfl					/* save IF in EFLAGS */
	cli					/* LOCK INTERRUPTS */
	movzbl	FUNC(workQWriteIx),%eax		/* get write index into %eax */
	movl	%eax,%ecx
	addb	$4,%al				/* advance write index */
	cmpb	FUNC(workQReadIx),%al		/* overflow? */
	je	workQOverflow			/* panic if overflowed */
	movb	%al,FUNC(workQWriteIx)		/* update write index */
	popfl					/* UNLOCK INTERRUPTS */
	movl	$0,FUNC(workQIsEmpty)		/* work queue isn't empty */
	leal	FUNC(pJobPool)(,%ecx,4),%edx	/* get the start of an entry */
	movl	SP_ARG1(%esp),%eax		/* move the function to pool */
	movl	%eax,JOB_FUNCPTR(%edx)
	movl	SP_ARG2(%esp),%eax		/* move 1st argument to pool */
	movl	%eax,JOB_ARG1(%edx)
	movl	SP_ARG3(%esp),%eax		/* move 2nd argument to pool */
	movl	%eax,JOB_ARG2(%edx)
	ret					/* we're done */

    
/*******************************************************************************
*
* workQDoWork - perform all the work queued in the kernel work queue
*
* This routine empties all the deferred work in the work queue.  The global
* variable errno is saved restored, so the work will not clobber it.
* The work routines may be C code, and thus clobber the volatile registers
* %e[adc]x.  This routine avoids using these registers.
*
* NOMANUAL

* void workQDoWork ()

*/

	.balign 16,0x90
FUNC_LABEL(workQDoWork)
	pushl	FUNC(errno)			/* push _errno */
	movzbl	FUNC(workQReadIx),%eax		/* load read index */
	cmpb	FUNC(workQWriteIx),%al		/* if readIndex != writeIndex */
	je	workQNoMoreWork			/* more work to be done */

workQMoreWork:
	sall	$2,%eax
	leal	FUNC(pJobPool)(%eax),%edx
	addb	$4,FUNC(workQReadIx)		/* increment readIndex */
	pushl	JOB_ARG2(%edx)			/* push arg2 */
	pushl	JOB_ARG1(%edx)			/* push arg1 */
	movl	FUNC(pJobPool)(%eax),%eax
	call	*%eax				/* %eax the work routine */
	addl	$8,%esp				/* clean up stack */

	movl	$1,FUNC(workQIsEmpty)		/* set boolean before test! */
	movzbl	FUNC(workQReadIx),%eax		/* load the new read index */
	cmpb	FUNC(workQWriteIx),%al		/* if readIndex != writeIndex */
	jne	workQMoreWork			/* more work to be done */

workQNoMoreWork:
	pop	FUNC(errno)			/* pop _errno */
	ret					/* return to caller */

#endif /* !PORTABLE */
