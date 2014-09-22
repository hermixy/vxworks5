/* kernelLib.c - VxWorks kernel library */

/* Copyright 1984-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
02n,13may02,cyr  update round-robin scheduling documentation section
02m,09nov01,jhw  Revert WDB_INFO to be inside WIND_TCB.
02l,27oct01,jhw  Set pTcb->pWdbInfo to NULL for the root task.
02k,11oct01,cjj  updated kernel version to 2.6.  Removed Am29k support.
02j,10apr00,hk   put SH interrupt stack in same space to VBR. use the original
                 vxIntStackBase and removed sysIntStackSpace reference for SH.
02i,03mar00,zl   merged SH support into T2
02h,24feb98,dbt  initialize activeQHead to be able to dectect from a host tool
                 if the OS is multitasking
02h,28nov96,cdp  added ARM support.
02g,02oct96,p_m  set version number to 2.5.
02f,07jul94,tpr  added MC68060 cpu support.
02g,26apr94,hdn  added a int stack alloc to make checkStack() work for I80X86.
02f,09jun93,hdn  added a support for I80X86
02f,02dec93,pme  added Am29K family support with 512 bytes aligned 
		 interrupt stack.
02e,20jan93,jdi  documentation cleanup for 5.1.
02d,23aug92,jcf  changed bzero to bfill.
02c,12jul92,jcf  cleaned up padding around root task's tcb.
02b,07jul92,rrr  added pad around the root task's tcb and stack so that
                 memAddToPool (called in taskDestroy) woundn't clobber the tcb.
02a,04jul92,jcf  moved most of kernelInit to usrInit.
		 changed the way root task is initialized to avoid excJobAdd.
		 removed kernelRoot().
01w,26may92,rrr  the tree shuffle
01v,21may92,yao  added bzero of initTcb for all architectures (spr 1272).
01u,18mar92,yao  removed conditional definition for STACK_GROWS_DOWN/UP.
		 removed macro MEM_ROUND_UP.  removed unused sentinel[].
		 changed copyright notice.  removed bzero of initTcb for
		 I960.  changed not to carve interrupt stack for MC680{0,1}0.
01t,23oct91,wmd  fixed bug in kernelInit() to compensate for pumped up stack
		 size for i960.
01s,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -fixed #else and #endif
		  -changed VOID to void
		  -changed copyright notice
01r,26sep91,jwt  added code to 8-byte align input address parameters.
01q,21aug91,ajm  added MIPS support.
01p,14aug91,del  padded excJobAdd, qInit, and taskInit calls with 0's.
01o,17jun91,del  added bzero of initTcb for I960.
01n,23may91,jwt  forced 8-byte alignment of stack for SPARC and other RISCs.
01m,26apr91,hdn  added conditional checks for TRON family of processors.
01l,05apr91,jdi	 documentation -- removed header parens and x-ref numbers;
		 doc review by jcf.
01k,25mar91,del  I960 needs init stack base aligned.
01j,08feb91,jaa	 documentation cleanup.
01i,30sep90,jcf  added kernelRootCleanup, to reclaim root task memory safely.
01h,28aug90,jcf  documentation.
01g,09jul90,dnw  added initialization of msg queue class
		 deleted include of qPriHeapLib.h
01f,05jul90,jcf  added STACK_GROWS_XXX to resolve stack growth direction issues.
		 reworked object initialization.
		 timer queue is now a simple linked list.
		 added room for MEM_TAG at beginning of root task memory.
01e,26jun90,jcf  changed semaphore object initialization.
		 changed topOfStack to endOfStack.
01d,28aug89,jcf  modified to wind version 2.0.
01c,21apr89,jcf  added kernelType.
01b,23aug88,gae  documentation.
01a,19jan88,jcf  written.
*/

/*
DESCRIPTION
The VxWorks kernel provides tasking control services to an application.  The
libraries kernelLib, taskLib, semLib, tickLib, and wdLib comprise the kernel
functionality.  This library is the interface to the VxWorks kernel
initialization, revision information, and scheduling control.

KERNEL INITIALIZATION
The kernel must be initialized before any other kernel operation is
performed.  Normally kernel initialization is taken care of by the system
configuration code in usrInit() in usrConfig.c.

Kernel initialization consists of the following:
.IP "(1)" 4
Defining the starting address and size of the system memory partition.
The malloc() routine uses this partition to satisfy memory allocation
requests of other facilities in VxWorks.
.IP "(2)"
Allocating the specified memory size for an interrupt stack.  Interrupt
service routines will use this stack unless the underlying architecture
does not support a separate interrupt stack, in which case the service
routine will use the stack of the interrupted task.
.IP "(3)"
Specifying the interrupt lock-out level.  VxWorks will not exceed the
specified level during any operation.  The lock-out level is normally
defined to mask the highest priority possible.  However, in situations
where extremely low interrupt latency is required, the lock-out level may
be set to ensure timely response to the interrupt in question.  Interrupt
service routines handling interrupts of priority greater than the
interrupt lock-out level may not call any VxWorks routine.
.LP

Once the kernel initialization is complete, a root task is spawned with
the specified entry point and stack size.  The root entry point is normally
usrRoot() of the usrConfig.c module.  The remaining VxWorks initialization
takes place in usrRoot().

ROUND-ROBIN SCHEDULING
Round-robin scheduling allows the processor to be shared fairly by all tasks
of the same priority.  Without round-robin scheduling, when multiple tasks of
equal priority must share the processor, a single non-blocking task can usurp
the processor until preempted by a task of higher priority, thus never giving
the other equal-priority tasks a chance to run.

Round-robin scheduling is disabled by default.  It can be enabled or
disabled with the routine kernelTimeSlice(), which takes a parameter for
the "time slice" (or interval) that each task will be allowed to run
before relinquishing the processor to another equal-priority task.  If the
parameter is zero, round-robin scheduling is turned off.  If round-robin
scheduling is enabled and preemption is enabled for the executing task,
the system tick handler will increment the task's time-slice count.
When the specified time-slice interval is completed, the system tick
handler clears the counter and the task is placed at the tail of the list
of tasks at its priority.  New tasks joining a given priority group are
placed at the tail of the group with a run-time counter initialized to zero.

Enabling round-robin scheduling does not affect the performance of task
context switches, nor is additional memory allocated.

If a task blocks or is preempted by a higher priority task during its
interval, it's time-slice count is saved and then restored when the task
is eligible for execution.  In the case of preemption, the task will
resume execution once the higher priority task completes, assuming no
other task of a higher priority is ready to run.  For the case when the
task blocks, it is placed at the tail of the list of tasks at its priority.
If preemption is disabled during round-robin scheduling, the time-slice
count of the executing task is not incremented.

Time-slice counts are accrued against the task that is executing when a system
tick occurs regardless of whether the task has executed for the entire tick
interval.  Due to preemption by higher priority tasks or ISRs stealing CPU
time from the task, scenarios exist where a task can execute for less or more
total CPU time than it's allotted time slice.


INCLUDE FILES: kernelLib.h

SEE ALSO: taskLib, intLib,
.pG "Basic OS"
*/

#include "vxWorks.h"
#include "tickLib.h"
#include "memLib.h"
#include "qLib.h"
#include "string.h"
#include "intLib.h"
#include "qPriBMapLib.h"
#include "classLib.h"
#include "semLib.h"
#include "wdLib.h"
#include "msgQLib.h"
#include "private/kernelLibP.h"
#include "private/windLibP.h"
#include "private/workQLibP.h"
#include "private/taskLibP.h"
#include "private/memPartLibP.h"
#if (CPU==SH7750 || CPU==SH7729 || CPU==SH7700)
#include "cacheLib.h"
#endif


/* macros */

#define WIND_TCB_SIZE		((unsigned) STACK_ROUND_UP (sizeof(WIND_TCB)))
#define MEM_BLOCK_HDR_SIZE	((unsigned) STACK_ROUND_UP (sizeof(BLOCK_HDR)))
#define MEM_FREE_BLOCK_SIZE	((unsigned) STACK_ROUND_UP (sizeof(FREE_BLOCK)))
#define MEM_BASE_BLOCK_SIZE	(MEM_BLOCK_HDR_SIZE + MEM_FREE_BLOCK_SIZE)
#define MEM_END_BLOCK_SIZE	(MEM_BLOCK_HDR_SIZE)
#define MEM_TOT_BLOCK_SIZE	((2 * MEM_BLOCK_HDR_SIZE) + MEM_FREE_BLOCK_SIZE)


/* global variables */

char *    vxIntStackEnd;		/* end of interrupt stack */
char *	  vxIntStackBase;		/* base of interrupt stack */
char *	  pRootMemStart;		/* bottom of root task's stack */
unsigned  rootMemNBytes;		/* memory for TCB and root stack */
int 	  rootTaskId;			/* root task ID */
BOOL	  roundRobinOn;			/* boolean reflecting round robin mode*/
ULONG	  roundRobinSlice;		/* round robin slice in ticks */
Q_HEAD	  activeQHead = {NULL,0,0,NULL};/* multi-way active queue head */
Q_HEAD	  tickQHead;			/* multi-way tick queue head */
Q_HEAD	  readyQHead;			/* multi-way ready queue head */

#if     (CPU_FAMILY == ARM)
char *    vxSvcIntStackBase;            /* base of SVC-mode interrupt stack */
char *    vxSvcIntStackEnd;             /* end of SVC-mode interrupt stack */
char *    vxIrqIntStackBase;            /* base of IRQ-mode interrupt stack */
char *    vxIrqIntStackEnd;             /* end of IRQ-mode interrupt stack */
VOIDFUNCPTR _func_armIntStackSplit = NULL;      /* ptr to fn to split stack */
#endif  /* (CPU_FAMILY == ARM) */

/*******************************************************************************
*
* kernelInit - initialize the kernel
*
* This routine initializes and starts the kernel.  It should be called only
* once.  The parameter <rootRtn> specifies the entry point of the user's
* start-up code that subsequently initializes system facilities (i.e., the
* I/O system, network).  Typically, <rootRtn> is set to usrRoot().
*
* Interrupts are enabled for the first time after kernelInit() exits.
* VxWorks will not exceed the specified interrupt lock-out level during any
* of its brief uses of interrupt locking as a means of mutual exclusion.
*
* The system memory partition is initialized by kernelInit() with the size
* set by <pMemPoolStart> and <pMemPoolEnd>.  Architectures that support a
* separate interrupt stack allocate a portion of memory for this
* purpose, of <intStackSize> bytes starting at <pMemPoolStart>.
*
* NOTE SH77XX:
* The interrupt stack is emulated by software, and it has to be located in
* a fixed physical address space (P1 or P2) if the on-chip MMU is enabled.
* If <pMemPoolStart> is in a logical address space (P0 or P3), the interrupt
* stack area is reserved on the same logical address space.  The actual
* interrupt stack is relocated to a fixed physical space pointed by VBR.
*
* INTERNAL
* The routine kernelRoot() is called before the user's root routine so that
* memory management can be initialized.  The memory setup is as follows:
*
* For _STACK_GROWS_DOWN:
*
* .CS
*	  - HIGH MEMORY -
*     ------------------------ <--- pMemPoolEnd
*     |                      |  We have to leave room for this block headers
*     |     1 BLOCK_HDR      |	so we can add the root task memory to the pool.
*     |                      |
*     ------------------------
*     |                      |   
*     |       WIND_TCB       |
*     |                      |
*     ------------------------ <--- pRootStackBase;
*     |                      |
*     |      ROOT STACK      |
*     |                      |
*     ------------------------
*     |                      |  We have to leave room for these block headers
*     |     1 FREE_BLOCK     |	so we can add the root task memory to the pool.
*     |     1 BLOCK_HDR      |
*     |                      |
*     ------------------------ <--- pRootMemStart;
*     ------------------------
*     |                      |
*     ~   FREE MEMORY POOL   ~		pool initialized in kernelRoot()
*     |                      |
*     ------------------------ <--- pMemPoolStart + intStackSize; vxIntStackBase
*     |                      |
*     |   INTERRUPT STACK    |
*     |                      |
*     ------------------------ <--- pMemPoolStart; vxIntStackEnd
*         - LOW  MEMORY -
* .CE
*
* For _STACK_GROWS_UP:
*
* .CS
*	   - HIGH MEMORY -
*     ------------------------ <--- pMemPoolEnd;
*     |                      |  We have to leave room for this block header
*     |     1 BLOCK_HDR      |	so we can add the root task memory to the pool.
*     |                      |
*     ------------------------ <--- pRootStackEnd;
*     |                      |
*     |      ROOT STACK      |
*     |                      |
*     ------------------------ <--- pRootStackBase;
*     |                      |
*     |       WIND_TCB       |
*     |                      |
*     ------------------------
*     |                      |  We have to leave room for these block headers
*     |     1 FREE_BLOCK     |	so we can add the root task memory to the pool.
*     |     1 BLOCK_HDR      |
*     |                      |
*     ------------------------ <--- pRootMemStart;
*     ------------------------
*     |                      |
*     ~   FREE MEMORY POOL   ~		pool initialized in kernelRoot()
*     |                      |
*     ------------------------ <--- pMemPoolStart + intStackSize; vxIntStackEnd
*     |                      |
*     |   INTERRUPT STACK    |
*     |                      |
*     ------------------------ <--- pMemPoolStart; vxIntStackBase
*	   - LOW  MEMORY -
* .CE
*
* RETURNS: N/A
*
* SEE ALSO: intLockLevelSet()
*/

void kernelInit
    (
    FUNCPTR	rootRtn,	/* user start-up routine */
    unsigned	rootMemSize,	/* memory for TCB and root stack */
    char *	pMemPoolStart,	/* beginning of memory pool */
    char *	pMemPoolEnd,	/* end of memory pool */
    unsigned	intStackSize,	/* interrupt stack size */
    int		lockOutLevel	/* interrupt lock-out level (1-7) */
    )
    {
    union
	{
	double   align8;        /* 8-byte alignment dummy */
	WIND_TCB initTcb;       /* context from which to activate root */
	} tcbAligned;
    WIND_TCB *  pTcb;           /* pTcb for root task */

    unsigned	rootStackSize;	/* actual stacksize of root task */
    unsigned	memPoolSize;	/* initial size of mem pool */
    char *	pRootStackBase;	/* base of root task's stack */

    /* align input size and address parameters */

    rootMemNBytes = STACK_ROUND_UP(rootMemSize);
    pMemPoolStart = (char *) STACK_ROUND_UP(pMemPoolStart);
    pMemPoolEnd   = (char *) STACK_ROUND_DOWN(pMemPoolEnd);
    intStackSize  = STACK_ROUND_UP(intStackSize);

    /* initialize VxWorks interrupt lock-out level */

    intLockLevelSet (lockOutLevel);

    /* round-robin mode is disabled by default */

    roundRobinOn = FALSE;

    /* initialize the time to zero */

    vxTicks = 0;				/* good morning */

    /* If the architecture supports a separate interrupt stack,
     * carve the interrupt stack from the beginning of the memory pool
     * and fill it with 0xee for checkStack ().  The MC680[016]0, I960, and
     * I80X86 not do support a separate interrupt stack.  I80X86, however, 
     * allocate the stack for checkStack () which is not used.
     */

#if 	(_STACK_DIR == _STACK_GROWS_DOWN)
#if 	(CPU != MC68000 && CPU != MC68010 && CPU != MC68060)
    vxIntStackBase = pMemPoolStart + intStackSize;
    vxIntStackEnd  = pMemPoolStart;
    bfill (vxIntStackEnd, (int) intStackSize, 0xee);

#if	(CPU != SH7750 && CPU != SH7729 && CPU != SH7700)
    windIntStackSet (vxIntStackBase);
    pMemPoolStart = vxIntStackBase;

#else	/* CPU == SH7750 || CPU == SH7729 || CPU == SH7700 */
    /* If mmu is enabled, emulated SH7700 interrupt stack needs to be relocated
     * on a fixed physical address space (P1/P2).  If mmu is disabled, it is
     * also possible to put the interrupt stack on copy-back cache (P0/P3).
     * Note that cache flush is necessary, since above bfill() might be done
     * on copy-back cache and we may use the area from its behind.
     */
    {
    pMemPoolStart = vxIntStackBase;

    /* push out 0xee's on copy-back cache to memory (nop if write-through) */

    CACHE_DRV_FLUSH (&cacheLib, vxIntStackEnd, (int) intStackSize);

    /* relocate interrupt stack to same address space to vector base */

    vxIntStackBase = (char *)(((UINT32)vxIntStackBase  & 0x1fffffff)
			    | ((UINT32)intVecBaseGet() & 0xe0000000));

    vxIntStackEnd  = (char *)(((UINT32)vxIntStackEnd   & 0x1fffffff)
			    | ((UINT32)intVecBaseGet() & 0xe0000000));

    /* load vxIntStackBase to P1/P2 */

    windIntStackSet (vxIntStackBase);
    }
#endif	/* CPU == SH7750 || CPU == SH7729 || CPU == SH7700 */


#if     (CPU_FAMILY == ARM)
    /*
     * The ARM family uses 3 interrupt stacks. The ratio of the sizes of
     * these stacks is dependent on the interrupt structure of the board
     * and so is handled in the BSP code. Note that FIQ is now external to
     * VxWorks.
     */
    if (_func_armIntStackSplit != NULL)
        (_func_armIntStackSplit)(vxIntStackBase, intStackSize);
#endif  /* (CPU_FAMILY == ARM) */

#endif 	/* (CPU != MC68000 && CPU != MC68010 && CPU != MC68060) */
#else	/* _STACK_DIR == _STACK_GROWS_UP */
#if	CPU_FAMILY != I960
    vxIntStackBase = pMemPoolStart;
    vxIntStackEnd  = pMemPoolStart + intStackSize;
    bfill (vxIntStackBase, intStackSize, 0xee);

    windIntStackSet (vxIntStackBase);
    pMemPoolStart = vxIntStackEnd;
#endif	/* CPU_FAMILY != I960 */
#endif 	/* (_STACK_DIR == _STACK_GROWS_UP) */

    /* Carve the root stack and tcb from the end of the memory pool.  We have
     * to leave room at the very top and bottom of the root task memory for
     * the memory block headers that are put at the end and beginning of a
     * free memory block by memLib's memAddToPool() routine.  The root stack
     * is added to the memory pool with memAddToPool as the root task's
     * dieing breath.
     */

    rootStackSize  = rootMemNBytes - WIND_TCB_SIZE - MEM_TOT_BLOCK_SIZE;
    pRootMemStart  = pMemPoolEnd - rootMemNBytes;

#if	(_STACK_DIR == _STACK_GROWS_DOWN)
    pRootStackBase = pRootMemStart + rootStackSize + MEM_BASE_BLOCK_SIZE;
    pTcb           = (WIND_TCB *) pRootStackBase;
#else	/* _STACK_GROWS_UP */
    pTcb           = (WIND_TCB *) (pRootMemStart   + MEM_BASE_BLOCK_SIZE);
    pRootStackBase = pRootMemStart + WIND_TCB_SIZE + MEM_BASE_BLOCK_SIZE;
#endif	/* _STACK_GROWS_UP */

    /* We initialize the root task with taskIdCurrent == 0.  This only works
     * because when taskInit calls windExit (), the ready queue will be
     * empty and thus the comparison of taskIdCurrent to the head of the
     * ready queue will result in equivalence.  In this case windExit ()
     * just returns to the caller, without changing anybody's context.
     */

    taskIdCurrent = (WIND_TCB *) NULL;	/* initialize taskIdCurrent */

    bfill ((char *) &tcbAligned.initTcb, sizeof (WIND_TCB), 0);

    memPoolSize = (unsigned) ((int) pRootMemStart - (int) pMemPoolStart);

    taskInit (pTcb, "tRootTask", 0, VX_UNBREAKABLE | VX_DEALLOC_STACK,
	      pRootStackBase, (int) rootStackSize, (FUNCPTR) rootRtn,
	      (int) pMemPoolStart, (int)memPoolSize, 0, 0, 0, 0, 0, 0, 0, 0);

    rootTaskId = (int) pTcb;			/* fill in the root task ID */

    /* Now taskIdCurrent needs to point at a context so when we switch into
     * the root task, we have some place for windExit () to store the old
     * context.  We just use a local stack variable to save memory.
     */

    taskIdCurrent = &tcbAligned.initTcb;        /* update taskIdCurrent */


    taskActivate ((int) pTcb);			/* activate root task */

    }

/*******************************************************************************
*
* kernelVersion - return the kernel revision string
*
* This routine returns a string which contains the current revision of the
* kernel.  The string is of the form "WIND version x.y", where "x"
* corresponds to the kernel major revision, and "y" corresponds to the
* kernel minor revision.
*
* RETURNS: A pointer to a string of format "WIND version x.y".
*/

char *kernelVersion (void)
    {
    return ("WIND version 2.6");
    }

/*******************************************************************************
*
* kernelTimeSlice - enable round-robin selection
*
* This routine enables round-robin selection among tasks of same priority
* and sets the system time-slice to <ticks>.  Round-robin scheduling is
* disabled by default.  A time-slice of zero ticks disables round-robin
* scheduling.
*
* For more information about round-robin scheduling, see the manual entry
* for kernelLib.
*
* RETURNS: OK, always.
*/

STATUS kernelTimeSlice
    (
    int ticks		/* time-slice in ticks or 0 to disable round-robin */
    )
    {
    if (ticks == 0)			/* 0 means turn off round-robin mode */
	roundRobinOn = FALSE;
    else
	{
	roundRobinSlice	= ticks;	/* set new time-slice */
	roundRobinOn	= TRUE;		/* enable round-robin scheduling mode */
	}

    return (OK);
    }
