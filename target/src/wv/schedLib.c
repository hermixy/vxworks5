/* schedLib.c - internal VxWorks kernel scheduler library */

/* Copyright 1984-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
02p,25mar02,kab  SPR 74651: PPC & SH can idle w/ work queued
02o,09nov01,dee  add CPU_FAMILY is/not COLDFIRE
02n,11oct01,cjj  removed Am29k support and removed #include asm.h
02m,07sep01,h_k  added _func_vxIdleLoopHook for power control support for the
                 SH processors (SPR #69838).
02l,19apr01,jmp  fixed scheduling bug on SIMNT, workQDoWork() must be performed
		 after the simulator enter in idle mode, not before (SPR#64900).
02l,03mar00,zl   merged SH support into T2
02k,31mar98,cjtc exclude portable scheduler for I960
02j,22jan98,pr   replaced EVT_CTX_SCHED with EVT_CTX_IDLE. Cleanup.
02i,02jan98,jmb  name change for Win32 externals.
02h,27oct97,pr   replaced WV code with macros
		 added trgLibP.h
02g,21oct97,cym  added intUnlock() to the idle loop for SIMNT.
02f,18jul97,pr   replaced evtLogTIsOn with evtInstMode
02e,28nov96,cdp  added ARM support.
02d,30aug96,ism  added floating point save code for SIMSPARCSOLARIS
02c,23oct96,tam  added call to vxPowerDown() in the iddle loop for PowerPC. 
		 added #include of vxLib.h for PowerPC only.
02b,24jun96,sbs  made windview instrumentation conditionally compiled
02a,04nov94,yao  added PPC support.
01x,25jan96,ism  cleanup from vxsim
01w,03nov95,ism  vxsim solaris
01v,09mar94,caf  use PORTABLE version for R3000 (SPR #2523).
01u,09jun93,hdn  added a support for I80X86
01s,12may94,ms   fixed vxsim hppa version of reschedule to save errno.
01r,11aug93,gae  vxsim hppa.
01q,22jun93,gae  vxsim.
01v,09nov94,rdc  locked interrupts while calling _func_evtLogTSched.
01u,03jun94,smb  merged with VxWorks for SPARC (marc shepard's version of 
		 reschedule)
01t,19may94,pad  merged with VxWorks for Am29K.
                 added evtLogTIsOn test.
	    pme  added Am29K family support and #include "asm.h"
		 added evetsched() prototype
01s,05may94,smb  WindView porting
01r,24jan94,smb  added instrumentation for priority inheritance
01q,10dec93,smb  added instrumentation for windview
01p,16mar93,jcf  removed NULL assignment of taskIdCurrent from while loop.
01o,06jul92,jwt  fixed reschedule() (wouldn't compile) for SPARC kernel.
01n,04jul92,jcf  private header files.
01m,04jun92,ajm  now uses global taskSrDefault instead of macro
01l,26may92,rrr  the tree shuffle
01k,22jan92,shl  fixed typo introduced in 01j.
01j,14jan92,jwt  merged 5.0.2b SPARC release; copyright message to 1992.
01i,15oct91,ajm  mips version is now optimized
01h,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
                  -fixed #else and #endif
                  -changed VOID to void
                  -changed copyright notice
01g,06Sep91,wmd  move line clearing kernelIsIdle.
01f,19aug91,ajm  imported kernelIsIdle for idle state consistency.
                 made kernel idle loop always use default SR just as
                 optimized version does.
01e,15feb91,jwt  passed taskIdCurrent as a parameter to reschedule().
           +hvh  set/cleared kernelIsIdle flag in reschedule().
01d,29aug90,jcf  documentation.
01c,26jun90,jcf  fixed up, fixed up PORTABLE version.
01b,11may90,jcf  fixed up PORTABLE definition.
01a,17jun89,jcf	 written.
*/

/*
DESCRIPTION
The VxWorks kernel provides tasking control services to an application.  The
libraries kernelLib, taskLib, semLib, tickLib, and wdLib comprise the kernel
functionality.  This library is internal to the VxWorks kernel and contains
no user callable routines.

INTERNAL
This module contains the portable version of the rescheduler of the VxWorks
kernel.  An optimized version is desirable and usually found in windALib.

At the time reschedule() is called, taskIdCurrent must contain the current
executing task.  That task's context has been saved as part of the execution of
windExit().  Execution continues to be in kernel space so mutual exclusion to
all queues is guaranteed.

We must simply make taskIdCurrent equal to whatever task is first in the ready
queue and load its context.  But nothing is simple.  The first complicating
factor is we must call the switch and swap hooks, if any.  Also, we must
remember to empty the kernel work queue before loading the context of the new
taskIdCurrent.  The testing of more work to do and the loading of the context
must be executed with interrupts locked out.  Otherwise interrupt service
routines may add work that gets forgotten.  An optimized version of this
routine should load as much of the context as possible before locking
interrupts, thus reducing interrupt latency.

Finally we consider the case of going idle, which will occur when the
ready queue is empty.  In such a case we loop waiting for work to be added
to the work queue, because an ISR waking some task up is the only way the
machine can stop idling.

.CS
windview INSTRUMENTATION
-------------------------

 level 3 events
	EVENT_WIND_EXIT_DISPATCH
	EVENT_WIND_EXIT_NODISPATCH
	EVENT_WIND_EXIT_DISPATCH_PI
	EVENT_WIND_EXIT_NODISPATCH_PI
	EVENT_WIND_EXIT_IDLE
.CE

SEE ALSO: "Basic OS", windALib

NOMANUAL
*/

#include "vxWorks.h"
#include "taskLib.h"
#include "private/workQLibP.h"
#include "private/windLibP.h"
#include "private/taskLibP.h"
#include "private/funcBindP.h"
#include "private/trgLibP.h"
#include "intLib.h"
#include "errno.h"
#if     (CPU_FAMILY == PPC) || (CPU_FAMILY == SH)
#include "vxLib.h"
#endif  /* (CPU_FAMILY == PPC) */

IMPORT	ULONG	taskSrDefault;
IMPORT	Q_HEAD	readyQHead;
IMPORT	void	windLoadContext (void);

#if (CPU_FAMILY == SIMNT)
#include "win_Lib.h"
IMPORT  WIN_HANDLE simIntIdleSem;
#endif

#if     (CPU_FAMILY == PPC)
IMPORT  _RType  taskMsrDefault;
#endif  /* (CPU_FAMILY == PPC) */

/* optimized version available for 680X0, MIPS, I80X86, and SH */

#if (defined(PORTABLE) || ((CPU_FAMILY != MC680X0) && (CPU_FAMILY != MIPS) && \
	(CPU_FAMILY != I80X86) && (CPU_FAMILY != ARM) && \
	(CPU_FAMILY != COLDFIRE) && \
	(CPU_FAMILY != I960) && (CPU_FAMILY != SH)))
#define schedLib_PORTABLE
#endif

#ifdef schedLib_PORTABLE

/*******************************************************************************
*
* reschedule - portable version of the scheduler
*
* This routine determines the appropriate task to execute, then calls any
* switch and swap hooks, then loads its context.  The complicating factor
* is that the kernel work queue must be checked before leaving.  In the
* portable version the checking of the work queue and the loading of the
* task's context is done at interrupt lock out level to avoid races with
* ISRs.  An optimized version ought to load as much of the context as is
* possible before locking interrupts and checking the work queue.  This will
* reduce interrupt latency considerably.
*
* If no task is ready, the machine will idle looking for more work in the
* work queue.  The idle state is not in a task context and therefore
* no switch or swap hooks are called when the machine goes idle.  Upon leaving
* the idle state, switch and swap hooks will be called if the new task is
* different than the task that was executing when the machine went idle.
*
* NOMANUAL
*/

void reschedule (void)

    {
    FAST WIND_TCB *taskIdPrevious;
    FAST int       ix;
    FAST UINT16    mask;
    int		   oldLevel;

unlucky:

    taskIdPrevious = taskIdCurrent;		/* remember old task */

    workQDoWork ();				/* execute all queued work */

    /* idle here until someone is ready to execute */

    kernelIsIdle = TRUE;                        /* idle flag for spy */

    /* windview conditions */
#ifdef WV_INSTRUMENTATION
    EVT_CTX_IDLE(EVENT_WIND_EXIT_IDLE, &readyQHead);
#endif

    while (((WIND_TCB *) Q_FIRST (&readyQHead)) == NULL)
        {
#if	CPU_FAMILY==SIMSPARCSUNOS || CPU_FAMILY==SIMHPPA
	{
	int mask=0;
        taskIdCurrent->errorStatus = errno;
	u_sigsuspend (&mask);
        errno = taskIdCurrent->errorStatus;
	}
#endif	/* CPU_FAMILY==SIMSPARCSUNOS || CPU_FAMILY==SIMHPPA */

#if CPU_FAMILY==SIMSPARCSOLARIS
	{
	int mask[4]={0, 0, 0, 0};
	extern void u_sigsuspend ();
	extern WIND_TCB *pTaskLastFpTcb;
	extern void fppSave();

	/*
	 * u_sigsuspend() hoses the fpp registers, so save them now if they
	 * are active.  Then set the Last FP TCB to NULL, so we won't try to
	 * save them again.
	 */
	if (pTaskLastFpTcb!=(WIND_TCB *)NULL)
		{
		fppSave (pTaskLastFpTcb->pFpContext);
		pTaskLastFpTcb = (WIND_TCB *)NULL;
		}

        taskIdCurrent->errorStatus = errno;
	u_sigsuspend (&mask[0]);
        errno = taskIdCurrent->errorStatus;
	}
#endif /* CPU_FAMILY==SIMSPARCSOLARIS */

#if 	CPU_FAMILY == SIMNT
	intUnlock(0);
    /* Don't busy wait, let Windoze run.  This semaphore is given when 
     * a message is received by the process, aka an interrupt shows up.
     */

	win_SemTake(simIntIdleSem);
#endif /* CPU_FAMILY == SIMNT */

	workQDoWork ();

#if     CPU_FAMILY==MIPS
    /* Let the idle loop look like a real task by turning all ints on.
     * Without this if a task locks interrupts and suspends itself, and
     * there are no ready tasks, we will lockup.
     */

	intSRSet (taskSrDefault);
#endif  /* CPU_FAMILY == MIPS */

#if     (CPU_FAMILY == PPC)
        intUnlock (taskMsrDefault);
	/* 
	 * SPR 74651: if the previous workQDoWork() had added jobs,
	 * vxPowerDown still ran until the *next* interrupt.
	 */
	if (((WIND_TCB *) Q_FIRST (&readyQHead)) != NULL)
	    break;

	vxPowerDown ();
#endif  /* (CPU_FAMILY == PPC) */

#if	(CPU_FAMILY == SH)			/* Unlock interrupts */
	intLevelSet (0);
	/* 
	 * SPR 74651: if the previous workQDoWork() had added jobs,
	 * the idleLoopHook was still run until the *next* interrupt.
	 */
	if (((WIND_TCB *) Q_FIRST (&readyQHead)) != NULL)
	    break;

	if (_func_vxIdleLoopHook != NULL)
	    (* _func_vxIdleLoopHook) ();
#endif

#if	(CPU_FAMILY == SPARC)
	intLevelSet (0);                        /* Unlock interrupts */
#endif

#if     (CPU_FAMILY == ARM)
        intUnlock(0);                           /* Unlock interrupts */
#endif
	}

    taskIdCurrent = (WIND_TCB *) Q_FIRST (&readyQHead);

    kernelIsIdle = FALSE;                       /* idle flag for spy */

    /* taskIdCurrent now has some task to run.  If it is different from
     * taskIdPrevious we execute the switch and swap hooks.
     */

    if (taskIdCurrent != taskIdPrevious)	/* switching in a new task? */
	{
	/* do swap hooks */

	mask = taskIdCurrent->swapInMask | taskIdPrevious->swapOutMask;

	for (ix = 0; mask != 0; ix++, mask = mask << 1)
	    if (mask & 0x8000)
		(* taskSwapTable[ix]) (taskIdPrevious, taskIdCurrent);

	/* do switch hooks */

	for (ix = 0;
	     (ix < VX_MAX_TASK_SWITCH_RTNS) && (taskSwitchTable[ix] != NULL);
	     ++ix)
	    {
	    (* taskSwitchTable[ix]) (taskIdPrevious, taskIdCurrent);
	    }
	}

    oldLevel = intLock ();			/* LOCK INTERRUPTS */

    if (!workQIsEmpty)
	{
	intUnlock (oldLevel);			/* UNLOCK INTERRUPTS */
	goto unlucky;				/* take it from the top... */
	}
    else
	{

#ifdef WV_INSTRUMENTATION
        /* log the dispatch event */
        EVT_CTX_DISP (EVENT_WIND_EXIT_DISPATCH_PI, (int) taskIdCurrent, 
	              taskIdCurrent->priority, taskIdCurrent->priNormal);
#endif

	kernelState = FALSE;			/* KERNEL EXIT */

	/* Now we load context and schedule the selected task in.  Note that
	 * interrupts are locked out until the task is switched in.  An
	 * optimized version of this routine should load the context just
	 * before locking interrupts above, then after locking interrupts,
	 * check the work queue to see if we missed any deferred.
	 */

	windLoadContext ();			/* dispatch the selected task */
	}
    }

#endif	/* schedLib_PORTABLE */
