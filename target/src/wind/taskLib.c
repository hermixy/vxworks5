/* taskLib.c - task management library */

/* Copyright 1984-2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
05o,15may02,pcm  added check for valid priority level in taskInit() (SPR 77368)
05n,04jan02,hbh  Increased default extra stack size for simulators.
05m,09nov01,jhw  Revert WDB_INFO to be inside WIND_TCB.
05l,05nov01,gls  added initialization of pPthread in taskInit
05k,17oct01,jhw  Move WDB_INFO outside of WIND_TCB in task mem partition.
05j,11oct01,cjj  removed Am29k support.  Added documentation to taskSpawn()
		 regarding the use of VX_FP_TASK.
05i,10oct01,pcm  added note to taskIdVerify () about exceptions (SPR 31496)
05h,09oct01,bwa  Added event field initialization.
05g,20sep01,aeg  added init of pSelectContext and pWdbInfo in taskInit().
05f,13jul01,kab  Cleanup for merge to mainline
05e,14mar01,pcs  Added code for ALTIVEC awareness.
05d,18dec00,pes  Correct compiler warnings
05d,03mar00,zl   merged SH support into T2
05c,12mar99,dbs  add COM task-local storage to TCB
05b,09mar98,cym  increasing stack size for SIMNT.
05a,05nov98,fle  doc : allowed link to taskInfo library by putting it in bold
04g,04sep98,cdp  force entry-point in TCB to have bit zero clear for all Thumb.
04f,25sep98,cym  removed 04f.
04e,06may98,cym  changed taskDestroy for SIMNT to use the calling context.
04d,22jul96,jmb  merged ease patch,  don't reinitialize excInfo on taskRestart.
04c,12jan98,dbt  modified for new debugger scheme.
04b,08jul97,dgp  doc: correct typo per SPR 7769
04b,22aug97,cdp  force entry point in TCB to have bit zero clear for Thumb.
04a,29oct96,dgp  doc: correct spelling of checkStack() in taskSpawn()
03z,16oct96,dgp  doc: add errnos for user-visible routines per SPR 6893
03y,09oct06,dgp  doc: correct taskInit() description of *pStackBase - SPR 6828
03x,08aug96,dbt  Modified taskRestart to handle task name>20 bytes (SPR #3445).
03w,24jul96,dbt  Initialized exception info for I960 in taskInit (SPR #3092).
03v,22jul96,jmb  merged ease patch,  don't reinitialize excInfo on taskRestart.
03u,27jun96,dbt  doc : added taskLock & taskUnlock not callable from
		 interrupt service routines (SPR #2568).
		 Updated copyright.
03t,24jun96,sbs  made windview instrumentation conditionally compiled
03u,13jun96,ism  increased stack size again
03t,13feb96,ism  bumped stack size for SIMSOLARIS created tasks
03s,25jan96,ism  cleanup for vxsim/solaris.
03r,23oct95,jdi  doc: added bit values for taskSpawn() options (SPR 4276).
03q,14oct95,jdi  doc: removed SEE ALSO to vxTaskEntry(), which is not published.
03p,26may95,ms	 initialize WDB fields in the TCB
	   + rrr
03p,30oct95,ism  added SIMSPARCSOLARIS support
03o,16jan95,rhp  added expl of taskID for taskInit(), taskActivate() (SPR#3923)
03n,10nov94,ms   bumped stack size for all spawned SIMHPPA tasks.
03m,28jul94,dvs  added reset of pTaskLastFpTcb in taskDestroy. (SPR #3033)
03l,02dec93,pme  added am29K family stack support.
03k,20jul94,ms   undid some of 03j for VxSim/HPPA. Bumped restartTaskStackSize.
03j,28jan94,gae  vxsim fixes for bumping stack on sysFlag & clearing excInfo.
03n,19oct94,rdc  bug in eventlogging in taskDestroy.
		 added lockCnt to EVT_CTX_TASKINFO.
03m,14apr94,smb  modified EVT_OBJ_* macros again
03l,15mar94,smb  modified EVT_OBJ_* macros
03k,24jan94,smb  added instrumentation macros
03j,10dec93,smb  added instrumentation
03i,16sep93,jmm  added check in taskPrioritySet() to ensure priority is 0-255
03h,01mar93,jdi  doc: reinstated VX_UNBREAKABLE as publishable option, per kdl;
		 addition to taskDelay() as per rrr.
03g,27feb93,jcf  fixed race in taskDestroy() with masking signals.
03f,15feb93,jdi  fixed taskSpawn() to mention fixed argument requirement.
03e,10feb93,jdi  doc review by jcf; corrected spelling of stdlib.h.
03d,04feb93,jdi  documentation cleanup; SPR#1365: added VX_DEALLOC_STACK
		 to doc for taskSpawn().
03c,01oct92,jcf  removed deadlock with deletion of deletion safe tasks.
03b,29sep92,pme  removed failure notification in taskDestroy.
03a,31aug92,rrr  fixed taskDelay to set errno to EINTR if a signal occurs.
02z,23aug92,jcf  moved info routines to taskInfo.c.  changed bzero to bfill.
02y,02aug92,jcf  initialized pExcRegSet (taskInit) utilized in taskRegSet().
02x,29jul92,jcf  taskDestroy suspends victim; padding to account for mem tags;
		 package initialization with taskInit; documentation.
02w,27jul92,jcf  cleanup.
02v,24jul92,yao  changed to use STACK_ROUND_UP instead MEM_ROUND_UP in 
		 taskStackAllot().  bzeroed dbgInfo in taskInit().
02u,23jul92,yao  removed initialization of pDbgState.
02t,23jul92,rrr  rounded the sizeof the TCB up to the requirements of rounding
                 of the stack.
02s,23jul92,ajm  moved _sig_timeout_recalc from sigLib to shrink proms
02r,21jul92,pme  changed shared TCB free management.
                 added smObjTaskDeleteFailRtn.
02q,19jul92,pme  added shared memory objects support.
02p,19jul92,smb  Added some ANSI documentation.
02o,09jul92,rrr  changed xsignal.h to private/sigLibP.h
02n,04jul92,jcf	 changed algorithm of taskDestroy/taskRestart to avoid excJobAdd
		 tid is now valid for delete hooks.
		 stack is now not filled optionally.
		 show/info routines removed.
02m,30jun92,yao  changed to init pDbgState for all architectures.
02l,26jun92,jwt  restored 02k changes; cleaned up compiler warnings.
02k,16jun92,jwt  made register display four across; fixed rrr version numbers.
02j,26may92,rrr  the tree shuffle
02i,30apr92,rrr  added signal restarting
02h,18mar92,yao  removed macro MEM_ROUND_UP.  removed conditional definition
		 STACK_GROWS_DOWN/UP.  abstracted taskStackAllot() from
		 taskArchLib.c.  removed unneccesary if conditionals for
		 calls to taskRegsInit() in taskInit().  changed taskRegsShow()
		 to be within 80 columns per line.
02g,12mar92,yao  added taskRegsShow().  changed copyright notice.
02f,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed includes to have absolute path from h/
		  -fixed #else and #endif
		  -changed VOID to void
		  -changed copyright notice
02e,11sep91,hdn  Init pDbgState for TRON.
02d,21aug91,ajm  made MIPS stacksize bounded on 8 bytes for varargs.
02c,14aug91,del  padded qInit and excJobAdd call's with 0's.
		 Init pDbgState for I960 only.
02b,10jun91,gae  fixed typo in taskOptionString to not say VX_FP_STACK.
02a,23may91,jwt  modifed roundup from 4 to 8 bytes for SPARC only.
01z,26apr91,hdn  added conditional checks for TRON architecture.
01y,20apr91,del  fixed bug in taskInfoGet() for checking the stack high mark.
		 For STACK_GROWS_UP (i960) version.
01x,05apr91,jdi	 documentation -- removed header parens and x-ref numbers;
		 doc review by dnw.
01w,31mar91,del  added I960 specifics.
01v,24mar91,jdi  documentation cleanup.
01u,25oct90,dnw  made taskTerminate() NOMANUAL.
01t,05oct90,dnw  added forward declarations.
01s,30sep90,jcf  added taskDeleteForce().
01r,31aug90,jcf  documentation.
01q,30jul90,jcf  changed task{Lock,Unlock,Safe,Unsafe} to return STATUS
01p,17jul90,dnw  changed to call to objAlloc() to objAllocExtra().
01o,13jul90,rdc  taskInit zeros environment var and select stuff in tcb.
01n,05jul90,jcf  added STACK_GROWS_XX to resolve stack growth direction issues.
		 renamed stack variables.
		 changed taskSuspend to disregard task deletion safety.
01m,26jun90,jcf  added taskStackAllot()
		 changed taskNames to use taskStackAllot()
		 removed taskFppHook.
		 general cleanup.
01l,15apr90,jcf  changed nameForNameless from task<%d> to t<%d>
01k,28aug89,jcf  modified to version 2.0 of wind.
01j,09aug89,gae  undid varargs stuff.
01i,28apr89,mcl  added some initializations in WIND_TCB (applies to
		   both SPARC & 68k); fixes alignment check in taskIdVerify.
01h,07apr89,mcl  SPARC floating point.
01g,12dec88,ecs  gave it some sparc.
		 added include of varargs.h.
01f,19nov88,jcf  taskDelay can not be called from interrupt level.
                   task ids of zero are translated to taskCurrentId here now.
01e,23sep88,gae  documentation touchup.
01d,07sep88,gae  documentation.
01c,23aug88,gae  documentation.
01b,20jul88,jcf  clean up.
01a,10mar88,jcf  written.
*/

/*
DESCRIPTION
This library provides the interface to the VxWorks task management facilities.
Task control services are provided by the VxWorks kernel, which is comprised
of kernelLib, taskLib, semLib, tickLib, msgQLib, and `wdLib'.  Programmatic
access to task information and debugging features is provided by `taskInfo'.
Higher-level task information display routines are provided by `taskShow'.

TASK CREATION
Tasks are created with the general-purpose routine taskSpawn().  Task
creation consists of the following:  allocation of memory for the stack
and task control block (WIND_TCB), initialization of the WIND_TCB, and
activation of the WIND_TCB.  Special needs may require the use of the
lower-level routines taskInit() and taskActivate(), which are the underlying
primitives of taskSpawn().

Tasks in VxWorks execute in the most privileged state of the underlying
architecture.  In a shared address space, processor privilege offers no
protection advantages and actually hinders performance.

There is no limit to the number of tasks created in VxWorks, as long as
sufficient memory is available to satisfy allocation requirements.

The routine sp() is provided in usrLib as a convenient abbreviation for
spawning tasks.  It calls taskSpawn() with default parameters.

TASK DELETION
If a task exits its "main" routine, specified during task creation, the
kernel implicitly calls exit() to delete the task.  Tasks can be
explicitly deleted with the taskDelete() or exit() routine.

Task deletion must be handled with extreme care, due to the inherent
difficulties of resource reclamation.  Deleting a task that owns a
critical resource can cripple the system, since the resource may no longer
be available.  Simply returning a resource to an available state is not a
viable solution, since the system can make no assumption as to the state
of a particular resource at the time a task is deleted.

The solution to the task deletion problem lies in deletion protection,
rather than overly complex deletion facilities.  Tasks may be protected
from unexpected deletion using taskSafe() and taskUnsafe().  While a task
is safe from deletion, deleters will block until it is safe to proceed.
Also, a task can protect itself from deletion by taking a mutual-exclusion
semaphore created with the SEM_DELETE_SAFE option, which enables an implicit
taskSafe() with each semTake(), and a taskUnsafe() with each semGive()
(see semMLib for more information).
Many VxWorks system resources are protected in this manner, and
application designers may wish to consider this facility where dynamic
task deletion is a possibility.

The sigLib facility may also be used to allow a task to
execute clean-up code before actually expiring.

TASK CONTROL
Tasks are manipulated by means of an ID that is returned when a task is
created.  VxWorks uses the convention that specifying a task ID of NULL
in a task control function signifies the calling task.

The following routines control task state:  taskResume(), taskSuspend(),
taskDelay(), taskRestart(), taskPrioritySet(), and taskRegsSet().

TASK SCHEDULING
VxWorks schedules tasks on the basis of priority.  Tasks may have
priorities ranging from 0, the highest priority, to 255, the lowest
priority.  The priority of a task in VxWorks is dynamic, and an existing
task's priority can be changed using taskPrioritySet().

INTERNAL:
	WINDVIEW INSTRUMENTATION
Level 1:
	taskInit() causes EVENT_TASKSPAWN
	taskDestroy() causes EVENT_TASKDESTROY
	taskSuspend() causes EVENT_TASKSUSPEND
	taskResume() causes EVENT_TASKRESUME
	taskPrioritySet() causes EVENT_TASKPRIORITYSET
	taskUnsafe() causes EVENT_TASKUNSAFE

Level 2:
	taskDestroy() causes EVENT_OBJ_TASK
	taskUnlock() causes EVENT_OBJ_TASK
	taskUnsafe() causes EVENT_OBJ_TASK

Level 3:
	taskInit() causes EVENT_TASKNAME
	taskUnlock() causes EVENT_TASKUNLOCK

INCLUDE FILES: taskLib.h

SEE ALSO: `taskInfo', taskShow, taskHookLib, taskVarLib, semLib, semMLib, 
kernelLib,
.pG "Basic OS"
*/

#include "vxWorks.h"
#include "errno.h"
#include "semLib.h"
#include "string.h"
#include "regs.h"
#include "intLib.h"
#include "taskArchLib.h"
#include "stdio.h"
#include "memLib.h"
#include "qFifoGLib.h"
#include "sysLib.h"		/* CPU==SIM* */
#include "private/eventLibP.h"
#include "private/sigLibP.h"
#include "private/classLibP.h"
#include "private/objLibP.h"
#include "private/smObjLibP.h"
#include "private/smFixBlkLibP.h"
#include "private/taskLibP.h"
#include "private/kernelLibP.h"
#include "private/workQLibP.h"
#include "private/windLibP.h"
#include "private/eventP.h"


/* locals */

LOCAL OBJ_CLASS	taskClass;			/* task object class */
LOCAL int	nameForNameless;		/* name for nameless tasks */
LOCAL BOOL	taskLibInstalled;		/* protect from double inits */

/* globals */

FUNCPTR		smObjTcbFreeRtn;		/* shared TCB free routine */
FUNCPTR		smObjTcbFreeFailRtn;		/* shared TCB free fail rtn */
FUNCPTR		smObjTaskDeleteFailRtn;		/* windDelete free fail rtn */
FUNCPTR		taskBpHook;			/* hook for VX_UNBREAKABLE */
FUNCPTR		taskCreateTable   [VX_MAX_TASK_CREATE_RTNS + 1];
FUNCPTR		taskSwitchTable   [VX_MAX_TASK_SWITCH_RTNS + 1];
FUNCPTR 	taskDeleteTable   [VX_MAX_TASK_DELETE_RTNS + 1];
FUNCPTR 	taskSwapTable     [VX_MAX_TASK_SWAP_RTNS   + 1];
int		taskSwapReference [VX_MAX_TASK_SWAP_RTNS   + 1];
WIND_TCB *	taskIdCurrent;				/* current task ID */
CLASS_ID	taskClassId   		= &taskClass;	/* task class ID */
char *		namelessPrefix		= "t";		/* nameless prefix */
char *		restartTaskName		= "tRestart";	/* restart name */
int		restartTaskPriority	= 0;		/* restart priority */
int		restartTaskStackSize	= 6000;		/* default stack size */
int		restartTaskOptions	= VX_NO_STACK_FILL | VX_UNBREAKABLE;
BOOL		taskPriRangeCheck	= TRUE; /* limit priorities to 0-255 */
WIND_TCB *	pTaskLastFpTcb          = NULL; /* pTcb for fppSwapHook */
WIND_TCB *	pTaskLastDspTcb         = NULL; /* pTcb for dspSwapHook */
#ifdef _WRS_ALTIVEC_SUPPORT
WIND_TCB *      pTaskLastAltivecTcb     = NULL; /* pTcb for altivecSwapHook */
#endif  /* _WRS_ALTIVEC_SUPPORT */

#ifdef WV_INSTRUMENTATION
/* instrumentation declarations */

LOCAL OBJ_CLASS taskInstClass;			/* task object class */
CLASS_ID 	taskInstClassId 	= &taskInstClass; /* instrumented task */
#endif

/* forward declarations */

int		taskCreat ();
WIND_TCB *	taskTcb ();			/* get pTcb from tid */


/*******************************************************************************
*
* taskLibInit - initialize kernel task library
*
* INTERNAL
* Tasks are a class of object in VxWorks.  This routine initializes the task
* task class object.  Once initialized, tasks can be created/deleted/etc via
* objLib.  No other task related routine may be executed prior to the call
* to this routine.  The sizeof the WIND_TCB is goosed up by 16 bytes
* bytes so we can protect the starting portion of a task's tcb during 
* deletion, in the case of STACK_GROWS_UP, or a portion of the top of the 
* task's top of stack in the case of STACK_GROWS_DOWN.  This portion is 
* clobbered with a FREE_BLOCK during objFree().
*
* NOMANUAL
*/

STATUS taskLibInit (void)
    {
    if ((!taskLibInstalled) &&
        (classInit (taskClassId, STACK_ROUND_UP(sizeof(WIND_TCB) + 16),
                    OFFSET(WIND_TCB, objCore), (FUNCPTR) taskCreat,
                    (FUNCPTR) taskInit, (FUNCPTR) taskDestroy) == OK))
        {
#ifdef WV_INSTRUMENTATION
	taskClassId->initRtn = taskInstClassId;
	classInstrument (taskClassId, taskInstClassId);
#endif
        taskLibInstalled = TRUE;
        }

    return ((taskLibInstalled) ? OK : ERROR);
    }

/*******************************************************************************
*
* taskSpawn - spawn a task
*
* This routine creates and activates a new task with a specified priority
* and options and returns a system-assigned ID.  See taskInit() and
* taskActivate() for the building blocks of this routine.
*
* A task may be assigned a name as a debugging aid.  This name will appear
* in displays generated by various system information facilities such as
* i().  The name may be of arbitrary length and content, but the current
* VxWorks convention is to limit task names to ten characters and prefix
* them with a "t".  If <name> is specified as NULL, an ASCII name will be
* assigned to the task of the form "t<n>" where <n> is an integer which
* increments as new tasks are spawned.
*
* The only resource allocated to a spawned task is a stack of a specified
* size <stackSize>, which is allocated from the system memory partition.
* Stack size should be an even integer.  A task control block (TCB) is
* carved from the stack, as well as any memory required by the task name.
* The remaining memory is the task's stack and every byte is filled with the
* value 0xEE for the checkStack() facility.  See the manual entry for
* checkStack() for stack-size checking aids.
*
* The entry address <entryPt> is the address of the "main" routine of the task.
* The routine will be called once the C environment has been set up.
* The specified routine will be called with the ten given arguments.
* Should the specified main routine return, a call to exit() will
* automatically be made.
*
* Note that ten (and only ten) arguments must be passed for the
* spawned function.
*
* Bits in the options argument may be set to run with the following modes:
* .iP "VX_FP_TASK  (0x0008)" 8
* execute with floating-point coprocessor support.  A task which performs
* floating point operations or calls any functions which either return
* or take a floating point value as arguments must be created with this 
* option.  Some routines perform floating point operations internally.
* The VxWorks documentation for these clearly state the need to use
* the VX_FP_TASK option.
* .iP "VX_PRIVATE_ENV  (0x0080)"
* include private environment support (see envLib).
* .iP "VX_NO_STACK_FILL  (0x0100)"
* do not fill the stack for use by checkStack().
* .iP "VX_UNBREAKABLE  (0x0002)"
* do not allow breakpoint debugging.
* .LP
* See the definitions in taskLib.h.
*
* RETURNS: The task ID, or ERROR if memory is insufficient or the task
* cannot be created.
*
* ERRNO: S_intLib_NOT_ISR_CALLABLE, S_objLib_OBJ_ID_ERROR,
*        S_smObjLib_NOT_INITIALIZED, S_memLib_NOT_ENOUGH_MEMORY,
*        S_memLib_BLOCK_ERROR, S_taskLib_ILLEGAL_PRIORITY
*
* SEE ALSO: taskInit(), taskActivate(), sp(),
* .pG "Basic OS"
*
* VARARGS5
*/

int taskSpawn
    (
    char          *name,        /* name of new task (stored at pStackBase) */
    int           priority,     /* priority of new task */
    int           options,      /* task option word */
    int           stackSize,    /* size (bytes) of stack needed plus name */
    FUNCPTR       entryPt,      /* entry point of new task */
    int           arg1,         /* 1st of 10 req'd task args to pass to func */
    int           arg2,
    int           arg3,
    int           arg4,
    int           arg5,
    int           arg6,
    int           arg7,
    int           arg8,
    int           arg9,
    int           arg10 
    )
    {
    int tid = taskCreat (name, priority, options, stackSize, entryPt, arg1,
                         arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10);

    if (tid == (int)NULL)			/* create failed */
	return (ERROR);

    taskActivate (tid);				/* activate task */

    return (tid);				/* return task ID */
    }

/*******************************************************************************
*
* taskCreat - allocate and initialize a task without activation
*
* This routine allocates and initializes a new task with the given priority,
* ID, and options.
*
* argument <name>
* A task may be given a name as a debugging aid.  This name will show up in
* various system information facilities suchas i().  The name may be of
* arbitrary length and content, but the current VxWorks convention is to limit
* task names to ten characters and prefix them with a `t'.  If the task name
* is specified as NULL, an ASCII name will be given to the task of the form
* `t<n>' where <n> is number which increments as tasks are spawned.
*
* argument <priority>
* VxWorks schedules tasks on the basis of priority.  Tasks may have priorities
* ranging from 0, the highest priority, to 255, the lowest priority.  VxWorks
* system tasks execute with priorities in the range 0 to 50.  The priority
* of a task in VxWorks in dynamic and one may change an existing task's priority
* with taskPrioritySet().
*
* Tasks in VxWorks always run in the most privileged mode of the CPU.  In
* a shared address space, processor privilege offers no protection advantages
* and actually hinders performance.
*
* argument <options>
* Bits in the options argument may be set to run with the following modes.
* .CS
*	VX_UNBREAKABLE		-  don't allow break point debugging
*	VX_FP_TASK		-  execute with coprocessor support
*	VX_DSP_TASK		-  execute with dsp support
*	VX_ALTIVEC_TASK		-  execute with Alitvec support
*	VX_STDIO		-  execute with standard I/O support
* .CE
* See definitions in taskLib.h.
*
* argument <stackSize>
* The only resource allocated to a spawned task is a stack of the specified
* size which is allocated from the memory pool.  Stack size should be even.
* A task control block (TCB) is carved from the stack, as well as a task
* debug structure WDB_INFO and any memory required by the task's name.
* The remaining memory is the task's stack. Every byte of the stack is 
* filled with the value 0xEE for the checkStack() facility. See checkStack()
* for stack size checking aids.
*
* The task stack memory map is: 
*
* For _STACK_GROWS_DOWN:
*
* .CS
*         - HIGH MEMORY -
*     ------------------------
*     |                      |
*     |       WIND_TCB       |
*     |                      |
*     ------------------------ <--- pStackBase, pTcb
*     |////// 16 bytes //////|	
*     |                      | 		(16 bytes clobbered during objFree()
*     |                      |		 in taskDestroy are not accounted for)
*     |                      |
*     |      TASK STACK      |
*     |                      |
*     |                      |
*     |                      |
*     ------------------------ <--- pTaskMem
*         - LOW  MEMORY -
* .CE
*
* For _STACK_GROWS_UP:
*
* .CS
*          - HIGH MEMORY -
*     ------------------------
*     |                      |
*     |                      |
*     |                      |
*     |      TASK STACK      |
*     |                      |
*     |                      |
*     |                      |
*     ------------------------ <--- pStackBase
*     |                      |
*     |       WIND_TCB       |
*     |                      |
*     ------------------------ <--- pTcb
*     |////// 16 bytes //////|	 (clobbered during objFree of taskDestroy)
*     ------------------------ <--- pTaskMem
*          - LOW  MEMORY -
* .CE
*
* argument <entryPt>
* The entry address is the address of the `main' routine of the task.  The
* routine will be called once the C environment has been set up by
* vxTaskEntry().  Should the specified main routine return, vxTaskEntry() will
* automatically call exit().  The specified routine will be called with the
* ten given arguments.
*
* See taskSpawn() for creating tasks with activation.
*
* See taskInit() for initializing tasks with pre-allocated stacks.
*
* RETURNS: Task ID, or NULL if out of memory or unable to create task.
*
* SEE ALSO: "Basic OS", taskInit(), taskActivate(), taskSpawn()
*
* VARARGS5
* NOMANUAL
*/

int taskCreat
    (
    char          *name,        /* name of new task (stored at pStackBase) */
    int           priority,     /* priority of new task */
    int           options,      /* task option word */
    int           stackSize,    /* size (bytes) of stack needed */
    FUNCPTR       entryPt,      /* entry point of new task */
    int           arg1,         /* task argument one */
    int           arg2,         /* task argument two */
    int           arg3,         /* task argument three */
    int           arg4,         /* task argument four */
    int           arg5,         /* task argument five */
    int           arg6,         /* task argument six */
    int           arg7,         /* task argument seven */
    int           arg8,         /* task argument eight */
    int           arg9,         /* task argument nine */
    int           arg10         /* task argument ten */
    )
    {
    char	newName[20];	/* place to keep name for nameless tasks */
    char *	pTaskMem;	/* pointer to task memory */
    char *	pStackBase;	/* bottom of stack (higher memory address) */
    WIND_TCB *	pTcb;		/* tcb pointer */
    char *	pBufStart;	/* points to temp name buffer start */
    char *	pBufEnd;	/* points to temp name buffer end */
    char	temp;		/* working character */
    int		value;		/* working value to convert to ascii */
    int		nPreBytes;	/* nameless prefix string length */
    int		nBytes	  = 0;	/* working nameless name string length */
    static char	digits [] = "0123456789";

    if (name == NULL)				/* name nameless w/o sprintf */
	{
	strcpy (newName, namelessPrefix);	/* copy in prefix */
	nBytes    = strlen (newName);		/* calculate prefix length */
	nPreBytes = nBytes;			/* remember prefix strlen() */
	value	  = ++ nameForNameless;		/* bump the nameless count */

	do					/* crank out digits backwards */
	    {
	    newName [nBytes++] = digits [value % 10];
	    value /= 10;			/* next digit */
	    }
	while (value != 0);			/* until no more digits */

	pBufStart = newName + nPreBytes;	/* start reverse after prefix */
    	pBufEnd	  = newName + nBytes - 1;	/* end reverse at EOS */

	while (pBufStart < pBufEnd)		/* reverse the digits */
	    {
	    temp	= *pBufStart;
	    *pBufStart	= *pBufEnd;
	    *pBufEnd	= temp;
	    pBufEnd--;
	    pBufStart++;
	    }
	newName[nBytes] = EOS;			/* EOS terminate string */
	name		= newName;		/* taskInit copies to task */
	}

#if	CPU==SIMSPARCSUNOS || CPU==SIMHPPA || CPU==SIMSPARCSOLARIS
    if (sysFlags & SYSFLG_DEBUG)
	stackSize *= 2;
#endif	/* CPU==SIMSPARCSUNOS || CPU==SIMHPPA || CPU==SIMSPARCSOLARIS */

#if     (CPU==SIMNT || CPU==SIMHPPA || CPU==SIMSPARCSOLARIS)
    /*
     * SIMHPPA and SIMSPARCSOLARIS take interrupts on the current 
     * task stack.
     * Moreover SIMNT and SIMSPARCSOLARIS use the current task stack for 
     * WINDOWS/UNIX system calls.
     * Bump all task stack sizes here to compensate, this way
     * when new vxWorks system tasks are added we won't need
     * to constantly readjust their default stack sizes.
     * Set this value to 12k which seems enough to prevent stackoverflow.
     */

    stackSize += 0x3000;
#endif  /* CPU == SIMNT || CPU==SIMHPPA || CPU==SIMSPARCSOLARIS */

    /* round stacksize up */

    stackSize	= STACK_ROUND_UP (stackSize);

    /* 
     * Allocate the WIND_TCB object, plus additional bytes for
     * the task stack from the task memory partition.
     * The 16 bytes of clobbered data is accounted for in the WIND_TCB object.
     */

    pTaskMem = (char *) objAllocExtra
    			    (
			    taskClassId,
			    (unsigned) (stackSize),
			    (void **) NULL
			    );
    if (pTaskMem == NULL)
	return ((int)NULL);			/* allocation failed */

#if	(_STACK_DIR == _STACK_GROWS_DOWN)
    /*
     * A portion of the very top of the stack is clobbered with a FREE_BLOCK
     * in the objFree() associated with taskDestroy().  There is no adverse
     * consequence of this, and is thus not accounted for.
     */

    pStackBase	= pTaskMem + stackSize;		/* grows down from WIND_TCB */
    pTcb	= (WIND_TCB *) pStackBase;

#else	/* _STACK_GROWS_UP */

    /*
     * To protect a portion of the WIND_TCB that is clobbered with a FREE_BLOCK
     * in the objFree() associated with taskDestroy(), we goose the base of
     * tcb by 16 bytes.
     */

    pTcb	= (WIND_TCB *) (pTaskMem + 16);
    pStackBase	= STACK_ROUND_UP (pTaskMem + 16 + sizeof(WIND_TCB));

#endif

    options    |= VX_DEALLOC_STACK;		/* set dealloc option bit */

    if (taskInit (pTcb, name, priority, options, pStackBase, stackSize, entryPt,
	          arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10)
	!= OK)
	{
	objFree (taskClassId, pTaskMem);
	return ((int)NULL);
	}

    return ((int) pTcb);			/* return task ID */
    }

/*******************************************************************************
*
* taskInit - initialize a task with a stack at a specified address
*
* This routine initializes user-specified regions of memory for a task stack
* and control block instead of allocating them from memory as taskSpawn()
* does.  This routine will utilize the specified pointers to the WIND_TCB
* and stack as the components of the task.  This allows, for example, the
* initialization of a static WIND_TCB variable.  It also allows for special
* stack positioning as a debugging aid.
*
* As in taskSpawn(), a task may be given a name.  While taskSpawn()
* automatically names unnamed tasks, taskInit() permits the existence of
* tasks without names.  The task ID required by other task routines
* is simply the address <pTcb>, cast to an integer.
*
* Note that the task stack may grow up or down from pStackBase, depending on
* the target architecture.
*
* Other arguments are the same as in taskSpawn().  Unlike taskSpawn(),
* taskInit() does not activate the task.  This must be done by calling
* taskActivate() after calling taskInit().
*
* Normally, tasks should be started using taskSpawn() rather than taskInit(),
* except when additional control is required for task memory allocation or
* a separate task activation is desired.
*
* RETURNS: OK, or ERROR if the task cannot be initialized.
*
* ERRNO: S_intLib_NOT_ISR_CALLABLE, S_objLib_OBJ_ID_ERROR,
*	 S_taskLib_ILLEGAL_PRIORITY
*
* SEE ALSO: taskActivate(), taskSpawn()
*
* VARARGS7
*/

STATUS taskInit
    (
    FAST WIND_TCB *pTcb,        /* address of new task's TCB */
    char          *name,        /* name of new task (stored at pStackBase) */
    int           priority,     /* priority of new task */
    int           options,      /* task option word */
    char          *pStackBase,  /* base of new task's stack */
    int           stackSize,    /* size (bytes) of stack needed */
    FUNCPTR       entryPt,      /* entry point of new task */
    int           arg1,         /* first of ten task args to pass to func */
    int           arg2,
    int           arg3,
    int           arg4,
    int           arg5,
    int           arg6,
    int           arg7,
    int           arg8,
    int           arg9,
    int           arg10 
    )
    {
    FAST int  	ix;			/* index for create hooks */
    int	      	pArgs [MAX_TASK_ARGS];	/* 10 arguments to new task */
    unsigned	taskNameLen;		/* string length of task name */
    char       *taskName = NULL;	/* assume nameless */

    if (INT_RESTRICT () != OK)			/* restrict ISR from calling */
	return (ERROR);

    if (taskPriRangeCheck)
	{
	if ((priority & 0xffffff00) != 0)	/* ! (0 <= x <= 255) */
	    {
	    errno = S_taskLib_ILLEGAL_PRIORITY;
	    return (ERROR);
	    }
	}

    if ((!taskLibInstalled) && (taskLibInit () != OK))
	return (ERROR);				/* package init problem */

    /* get arguments into indigenous variables */

    pArgs[0] = arg1; pArgs[1] = arg2; pArgs[2] = arg3; pArgs[3] = arg4;
    pArgs[4] = arg5; pArgs[5] = arg6; pArgs[6] = arg7;
    pArgs[7] = arg8; pArgs[8] = arg9;
    pArgs[9] = arg10;

    /* initialize task control block (in order) */

    pTcb->options       = options;		/* options */
    pTcb->status	= WIND_SUSPEND;		/* initially suspended */
    pTcb->priority	= priority;		/* set priority */
    pTcb->priNormal	= priority;		/* set standard priority */
    pTcb->priMutexCnt	= 0;			/* no mutex owned */
    pTcb->pPriMutex	= NULL;			/* not blocked on pri mutex */
    pTcb->lockCnt	= 0;	 		/* task w/o preemption lock */
    pTcb->tslice	= 0;			/* no round robin */
    pTcb->swapInMask	= 0;			/* no swap-in hooks */
    pTcb->swapOutMask   = 0;			/* no swap-out hooks */

    pTcb->pPendQ  	= NULL;			/* initialize ptr to pend q */

    pTcb->safeCnt	= 0;	 		/* initialize safe count */

    /* initialize safety q head */
    qInit (&pTcb->safetyQHead, Q_PRI_LIST, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

#if ((CPU_FAMILY == ARM) && ARM_THUMB)
    pTcb->entry         = (FUNCPTR)((UINT32)entryPt & ~1); /* entry address */
#else
    pTcb->entry         = entryPt;		/* entry address */
#endif

    pTcb->pStackBase	= pStackBase;		/* initialize stack base */
    pTcb->pStackLimit   = pStackBase + stackSize*_STACK_DIR;
    pTcb->pStackEnd	= pTcb->pStackLimit;

    if (!(options & VX_NO_STACK_FILL))		/* fill w/ 0xee for checkStack*/
#if	(_STACK_DIR == _STACK_GROWS_DOWN)
	bfill (pTcb->pStackLimit, stackSize, 0xee);
#else	/* (_STACK_DIR == _STACK_GROWS_UP) */
	bfill (pTcb->pStackBase, stackSize, 0xee);
#endif	/* (_STACK_DIR == _STACK_GROWS_UP) */

    taskRegsInit (pTcb, pStackBase);		/* initialize register set */

    pTcb->errorStatus   = OK;			/* errorStatus */
    pTcb->exitCode	= OK;			/* field for exit () */
    pTcb->pSignalInfo   = NULL;			/* ptr to signal context */
    pTcb->pSelectContext= NULL;			/* context not allocated yet */
    pTcb->ppEnviron	= NULL;			/* global environment vars */

    pTcb->taskTicks     = 0;			/* profiling for spyLib */
    pTcb->taskIncTicks  = 0;			/* profiling for spyLib */

#if CPU_FAMILY!=I960
    pTcb->excInfo.valid = 0;			/* exception info is invalid */
#else /* CPU_FAMILY == I960 */
    pTcb->excInfo.eiType = 0;
#endif	/* CPU_FAMILY!=I960 */

    pTcb->pTaskVar      = NULL;			/* list of task variables */
    pTcb->pRPCModList	= NULL;			/* rpc module statics context */
    pTcb->pFpContext    = NULL;			/* floating point context */
    pTcb->pDspContext   = NULL;			/* dsp context */
						
    for (ix = 0; ix < 3; ++ix)
	{
	pTcb->taskStd[ix]    = ix;		/* stdin/stdout/stderr fds */
	pTcb->taskStdFp[ix]  = NULL;		/* stdin/stdout/stderr fps */
	}

    pTcb->pSmObjTcb	= NULL;			/* no shared TCB for now */
    pTcb->windxLock	= (int)NULL;
    pTcb->pComLocal	= NULL;
    pTcb->pExcRegSet	= NULL;
    bzero ((char*)&(pTcb->events), sizeof(EVENTS));/* clear events structure */
    pTcb->reserved1	= (int)NULL;
    pTcb->reserved2	= (int)NULL;
    pTcb->spare1	= (int)NULL;
    pTcb->spare2	= (int)NULL;
    pTcb->spare3	= (int)NULL;
    pTcb->spare4	= (int)NULL;

    pTcb->pPthread	= NULL;
   
    pTcb->wdbInfo.wdbState	= 0;
    pTcb->wdbInfo.bpAddr	= 0;
    pTcb->wdbInfo.wdbExitHook	= NULL;

    taskArgsSet (pTcb, pStackBase, pArgs);	/* initialize task arguments */

#ifdef WV_INSTRUMENTATION
    /* windview instrumentation */
    /* windview - level 1 event logging
     * The objCore has not been initialised yet so this must be done
     * by hand.
     */
    EVT_OBJ_TASKSPAWN (EVENT_TASKSPAWN, (int) pTcb, priority,
                    	stackSize, (int) entryPt, options);

    /* windview - The parser must be able to link a name with a task id. */
    EVT_CTX_TASKINFO (EVENT_TASKNAME, WIND_SUSPEND,
               	       pTcb->priority, pTcb->lockCnt, (int) pTcb, name);
#endif

    kernelState = TRUE;				/* KERNEL ENTER */

    windSpawn (pTcb);				/* spawn the task */

#ifdef WV_INSTRUMENTATION
    /* windview - connect the instrumented class for level 1 event logging */
    if (wvObjIsEnabled)
        objCoreInit (&pTcb->objCore, taskInstClassId);
    else
#endif
	objCoreInit (&pTcb->objCore, taskClassId);	/* validate task ID */

    windExit ();				/* KERNEL EXIT */

    if (name != NULL)				/* copy name if not nameless */
	{
	taskNameLen = (unsigned) (strlen (name) + 1);
	taskName    = (char *) taskStackAllot ((int) pTcb, taskNameLen);
	strcpy (taskName, name);		/* copy the name onto stack */
	}

    pTcb->name = taskName;			/* name of this task */

    for (ix = 0; ix < VX_MAX_TASK_CREATE_RTNS; ++ix)
	{
	if (taskCreateTable[ix] != NULL)
	    (*taskCreateTable[ix]) (pTcb);
	}

    return (OK);				/* return task ID */
    }

/*******************************************************************************
*
* taskActivate - activate a task that has been initialized
*
* This routine activates tasks created by taskInit().  Without activation, a
* task is ineligible for CPU allocation by the scheduler.
* 
* The <tid> (task ID) argument is simply the address of the WIND_TCB
* for the task (the taskInit() <pTcb> argument), cast to an integer:
* .CS
* tid = (int) pTcb;
* .CE
*
* The taskSpawn() routine is built from taskActivate() and taskInit().
* Tasks created by taskSpawn() do not require explicit task activation.
*
* RETURNS: OK, or ERROR if the task cannot be activated.
*
* SEE ALSO: taskInit()
*/

STATUS taskActivate
    (
    int tid                     /* task ID of task to activate */
    )
    {
    return (taskResume (tid));			/* taskActivate == taskResume */
    }

/*******************************************************************************
*
* exit - exit a task  (ANSI)
*
* This routine is called by a task to cease to exist as a task.  It is
* called implicitly when the "main" routine of a spawned task is exited.
* The <code> parameter will be stored in the WIND_TCB for
* possible use by the delete hooks, or post-mortem debugging.
*
* ERRNO:  N/A
*
* SEE ALSO: taskDelete(),
* .I "American National Standard for Information Systems -"
* .I "Programming Language - C, ANSI X3.159-1989: Input/Output (stdlib.h),"
* .pG "Basic OS"
*/

void exit
    (
    int code            /* code stored in TCB for delete hooks */
    )
    {
    taskIdCurrent->exitCode = code;		/* store the exit code */

    taskLock ();				/* LOCK PREEMPTION */

    taskIdCurrent->options |= VX_UNBREAKABLE;	/* mark as unbreakable */

    if (taskBpHook != NULL)			/* call the debugger hook */
	(* taskBpHook) (taskIdCurrent);		/* to remove all breakpoints */

    taskUnlock ();				/* UNLOCK PREEMPTION */

    taskDestroy (0, TRUE, WAIT_FOREVER, FALSE);	/* self destruct */
    }

/*******************************************************************************
*
* taskDelete - delete a task
*
* This routine causes a specified task to cease to exist and deallocates the
* stack and WIND_TCB memory resources.  Upon deletion, all routines specified
* by taskDeleteHookAdd() will be called in the context of the deleting task.
* This routine is the companion routine to taskSpawn().
*
* RETURNS: OK, or ERROR if the task cannot be deleted.
*
* ERRNO: S_intLib_NOT_ISR_CALLABLE, S_objLib_OBJ_DELETED
*        S_objLib_OBJ_UNAVAILABLE, S_objLib_OBJ_ID_ERROR
*
* SEE ALSO: excLib, taskDeleteHookAdd(), taskSpawn(),
* .pG "Basic OS"
*/

STATUS taskDelete
    (
    int tid                     /* task ID of task to delete */
    )
    {
    return (taskDestroy (tid, TRUE, WAIT_FOREVER, FALSE));
    }

/*******************************************************************************
*
* taskDeleteForce - delete a task without restriction
*
* This routine deletes a task even if the task is protected from deletion.  
* It is similar to taskDelete().  Upon deletion, all routines
* specified by taskDeleteHookAdd() will be called in the context of the
* deleting task.
*
* CAVEATS
* This routine is intended as a debugging aid, and is generally inappropriate
* for applications.  Disregarding a task's deletion protection could leave the
* the system in an unstable state or lead to system deadlock.
*
* The system does not protect against simultaneous taskDeleteForce() calls.
* Such a situation could leave the system in an unstable state.
*
* RETURNS: OK, or ERROR if the task cannot be deleted.
*
* ERRNO: S_intLib_NOT_ISR_CALLABLE, S_objLib_OBJ_DELETED,
*        S_objLib_OBJ_UNAVAILABLE, S_objLib_OBJ_ID_ERROR
*
* SEE ALSO: taskDeleteHookAdd(), taskDelete()
*/

STATUS taskDeleteForce
    (
    int tid                     /* task ID of task to delete */
    )
    {
    return (taskDestroy (tid, TRUE, WAIT_FOREVER, TRUE));
    }


/*******************************************************************************
*
* taskTerminate - terminate a task
*
* This routine causes a task to cease to exist but does not deallocate the
* stack or WIND_TCB memory.  During termination, all routines specified by
* taskDeleteHookAdd() will be called in the context of the deleting task.
* This routine serves as the companion routine to taskInit().  To delete a
* task created by taskSpawn(), see taskDelete().
*
* If a task attempts to terminate itself, the termination and the delete hooks
* are performed in the context of the exception task.
*
* RETURNS: OK, or ERROR if task cannot be terminated.
*
* ERRNO: S_intLib_NOT_ISR_CALLABLE, S_objLib_OBJ_DELETED,
*        S_objLib_OBJ_UNAVAILABLE, S_objLib_OBJ_ID_ERROR
*
* SEE ALSO: excLib, taskDeleteHookAdd(), taskDelete(),
* .pG "Basic OS"
*
* NOMANUAL
*/

STATUS taskTerminate
    (
    int tid                     /* task ID of task to delete */
    )
    {
    return (taskDestroy (tid, FALSE, WAIT_FOREVER, FALSE));
    }

/*******************************************************************************
*
* taskDestroy - destroy a task
*
* This routine causes the specified task to cease to exist and, if successful,
* optionally frees the memory associated with the task's stack and
* WIND_TCB.  This call forms the foundation to the routines taskDelete() and
* taskTerminate().  Upon destruction, all routines specified by
* taskDeleteHookAdd() will be called in the context of the destroying task.
*
* If a task attempts to destroy itself, the destruction and the delete hooks
* are performed in the context of the exception task.
*
* RETURNS: OK, or ERROR if task cannot be destroyed.
*
* ERRNO: S_intLib_NOT_ISR_CALLABLE, S_objLib_OBJ_DELETED,
*        S_objLib_OBJ_UNAVAILABLE, S_objLib_OBJ_ID_ERROR
*
* SEE ALSO: excLib, taskDeleteHookAdd(), taskTerminate(), taskDelete()
*
* NOMANUAL
*/

STATUS taskDestroy
    (
    int  tid,                   /* task ID of task to delete */
    BOOL dealloc,               /* deallocate associated memory */
    int  timeout,               /* time willing to wait */
    BOOL forceDestroy           /* force deletion if protected */
    )
    {
    FAST int	  ix;		/* delete hook index */
    FAST WIND_TCB *pTcb;	/* convenient pointer to WIND_TCB */
    FAST int      lock;		/* to lock interrupts */
    int		  status;	/* windDelete return status */

    if (INT_RESTRICT () != OK)				/* no ISR use */
	return (ERROR);

    if (tid == 0)
	pTcb = taskIdCurrent;				/* suicide */
    else
	pTcb = (WIND_TCB *) tid;			/* convenient pointer */

#ifdef WV_INSTRUMENTATION
    /* windview - level 1 event logging */
    EVT_OBJ_2 (TASK, pTcb, taskClassId, EVENT_TASKDESTROY, pTcb, pTcb->safeCnt);
#endif


    if ((pTcb == taskIdCurrent) && (_func_excJobAdd != NULL))
	{
	/* If exception task is available, delete task from its context.
	 * While suicides are supported without an exception task, it seems
	 * safer to utilize another context for deletion.
	 */
	
	while (pTcb->safeCnt > 0)			/* make task unsafe */
	    TASK_UNSAFE ();

	_func_excJobAdd (taskDestroy, (int)pTcb, dealloc, NO_WAIT, FALSE);

	FOREVER
	    taskSuspend (0);				/* wait to die */
	}

again:
    lock = intLock ();					/* LOCK INTERRTUPTS */

    if (TASK_ID_VERIFY (pTcb) != OK)			/* valid task ID? */
	{
	intUnlock (lock);				/* UNLOCK INTERRUPTS */
	return (ERROR);
	}

    /*
     * Mask all signals of pTcb (may be suicide)
     * This is the same as calling sigfillset(&pTcb->pSignalInfo->sigt_blocked)
     * without the call to sigLib
     */

    if (pTcb->pSignalInfo != NULL)
	pTcb->pSignalInfo->sigt_blocked = 0xffffffff;

    while ((pTcb->safeCnt > 0) ||
	   ((pTcb->status == WIND_READY) && (pTcb->lockCnt > 0)))
	{
	kernelState = TRUE;				/* KERNEL ENTER */

	intUnlock (lock);				/* UNLOCK INTERRUPTS */

	if ((forceDestroy) || (pTcb == taskIdCurrent))
	    {

	    pTcb->safeCnt = 0;				/* unprotect */
	    pTcb->lockCnt = 0;				/* unlock */

	    if (Q_FIRST (&pTcb->safetyQHead) != NULL)	/* flush safe queue */
                {
#ifdef WV_INSTRUMENTATION
                /* windview - level 2 event logging */
                EVT_TASK_1 (EVENT_OBJ_TASK, pTcb);
#endif

                windPendQFlush (&pTcb->safetyQHead);
                }

	    windExit ();				/* KERNEL EXIT */
	    }
	else						/* wait to destroy */
	    {

#ifdef WV_INSTRUMENTATION
            /* windview - level 2 event logging */
            EVT_TASK_1 (EVENT_OBJ_TASK, pTcb);  /* log event */
#endif

	    if (windPendQPut (&pTcb->safetyQHead, timeout) != OK)
		{
		windExit ();				/* KERNEL EXIT */
		return (ERROR);
		}

	    switch (windExit())
		{
		case RESTART :
		    /* Always go back and reverify, this is because we have
		     * been running in a signal handler for who knows how long.
		     */
		    timeout = SIG_TIMEOUT_RECALC(timeout);
		    goto again;

		case ERROR :				/* timed out */
		    return (ERROR);

		default :				/* we were flushed */
		    break;
		}

	    /* All deleters of safe tasks block here.  When the safeCnt goes
	     * back to zero (or we taskUnlock) the deleters will be unblocked
	     * and the highest priority task among them will be elected to
	     * complete the deletion.  All unelected deleters will ultimately
	     * find the ID invalid, and return ERROR when they proceed from
	     * here.  The complete algorithm is summarized below.
	     */
	     }

	lock = intLock ();				/* LOCK INTERRTUPTS */

	if (TASK_ID_VERIFY (pTcb) != OK)
	    {
	    intUnlock (lock);				/* UNLOCK INTERRUPTS */
	    errno = S_objLib_OBJ_DELETED;
	    return (ERROR);
	    }
	}

    /* We can now assert that one and only one task has been elected to
     * perform the actual deletion.  The elected task may even be the task
     * to be deleted in the case of suicide.  To guarantee all other tasks
     * flushed from the safe queue receive an ERROR notification, the
     * elected task reprotects the victim from deletion.
     * 
     * A task flushed from the safe queue checks if the task ID is invalid,
     * which would mean the deletion is completed.  If, on the other hand,
     * the task ID is valid, one of two possibilities exist.  One outcome is
     * the flushed task performs the test condition in the while statement
     * above and finds the safe count equal to zero.  In this case the
     * flushed task is the elected deleter.
     * 
     * The second case is that the safe count is non-zero.  The only way the
     * safe count can be non zero after being flushed from the delete queue
     * is if the elected deleter blocked before completing the deletion or
     * the victim managed to legitimately taskSafe() itself in one way or
     * another.  A deleter can block because before performing the deletion
     * and hence task ID invalidation, the deleter must call the delete
     * hooks that possibly deallocate memory which involves taking a
     * semaphore.  So observe that nothing prevents the deleter from being
     * preempted by some other task which might also try a deletion on the
     * same victim.  We need not account for this case in any special way
     * because the task will atomically find the ID valid but the safe count
     * non zero and thus block on the safe queue.  It is therefore
     * impossible for two deletions to occur on the same task being killed
     * by one or more deleters.
     * 
     * We must also protect the deleter from being deleted by utilizing
     * taskSafe().  When a safe task is deleting itself the safe count is
     * set equal to zero, and other deleters are flushed from the safe
     * queue.  From this point on the algorithm remains the same.
     * 
     * The only special problem a suicide presents is deallocating the
     * memory associated with the task.  When we free the memory, we must
     * prevent any preemption from occuring, thus opening up an opportunity
     * for the memory to be allocated out from under us and corrupted.  We
     * lock preemption before the objFree() call.  The task may block
     * waiting for the partition, but once access is gained no further
     * preemption will occur.  An alternative to locking preemption is to
     * lock the partition by taking the partition's semaphore.  If the
     * partition utilizes mutex semaphores which permit recursive access, this
     * alternative seems attractive.  However, the memory manager will utilize
     * binary semaphores when scaled down.  With a fixed duration memPartFree()
     * algorithm, a taskLock() does not seem excessive compared to a more
     * intimate coupling with memPartLib.
     * 
     * One final complication exists before task invalidation and kernel
     * queue removal can be completed.  If we enter the kernel and
     * invalidate the task ID, there is a brief opportunity for an ISR to
     * add work to the kernel work queue referencing the soon to be defunct
     * task ID.  To prevent this we lock interrupts before invalidating the
     * task ID, and then enter the kernel.  Conclusion of the algorithm
     * consists of removing the task from the kernel queues, flushing the
     * unelected deleters to receive ERROR notification, exiting the kernel,
     * and finally restoring the deleter to its original state with
     * taskUnsafe(), and taskUnlock().
     */


    TASK_SAFE ();					/* protect deleter */

    pTcb->safeCnt ++;					/* reprotect victim */

    if (pTcb != taskIdCurrent)				/* if not a suicide */
	{
	kernelState = TRUE;				/* ENTER KERNEL */
	intUnlock (lock);				/* UNLOCK INTERRUPTS */
	windSuspend (pTcb);				/* suspend victim */
	windExit ();					/* EXIT KERNEL */
	}
    else
	intUnlock (lock);				/* UNLOCK INTERRUPTS */

    /* run the delete hooks in the context of the deleting task */

    for (ix = 0; ix < VX_MAX_TASK_DELETE_RTNS; ++ix)
	if (taskDeleteTable[ix] != NULL)
	    (*taskDeleteTable[ix]) (pTcb);

    TASK_LOCK ();					/* LOCK PREEMPTION */

    if ((dealloc) && (pTcb->options & VX_DEALLOC_STACK))
        {
	if (pTcb == (WIND_TCB *) rootTaskId)
	    memAddToPool (pRootMemStart, rootMemNBytes);/* add root into pool */
	else

#if 	(_STACK_DIR == _STACK_GROWS_DOWN)
	    /*
	     * A portion of the very top of the stack is clobbered with a
	     * FREE_BLOCK in the objFree() associated with taskDestroy(). 
	     * There is no adverse consequence of this, and is thus not 
	     * accounted for.
	     */

	    objFree (taskClassId, pTcb->pStackEnd);

#else	/* _STACK_GROWS_UP */

	    /*
	     * To protect a portion of the WIND_TCB that is clobbered with a
	     * FREE_BLOCK in this objFree() we previously goosed up the base of
	     * tcb by 16 bytes.
	     */

	    objFree (taskClassId, (char *) pTcb - 16);
#endif
	}

    lock = intLock ();					/* LOCK INTERRUPTS */

    objCoreTerminate (&pTcb->objCore);			/* INVALIDATE TASK */

    kernelState = TRUE;					/* KERNEL ENTER */

    intUnlock (lock);					/* UNLOCK INTERRUPTS */

    status = windDelete (pTcb);				/* delete task */

    /* 
     * If the task being deleted is the last Fp task from fppSwapHook then
     * reset pTaskLastFpTcb. 
     */

    if (pTcb == pTaskLastFpTcb)
    	pTaskLastFpTcb = NULL;				

    /*
     * If the task being deleted is the last DSP task from dspSwapHook then
     * reset pTaskLastDspTcb.
     */

    if (pTcb == pTaskLastDspTcb)
    	pTaskLastDspTcb = NULL;

#ifdef _WRS_ALTIVEC_SUPPORT
    /*
     * If the task being deleted is the last Altivec task from 
     * altivecSwapHook then reset pTaskLastAltivecTcb.
     */

    if (pTcb == pTaskLastAltivecTcb)
        pTaskLastAltivecTcb = NULL;
#endif /* _WRS_ALTIVEC_SUPPORT */

    /*
     * Now if the task has used shared memory objects the following 
     * can happen :
     *
     * 1) windDelete has return OK indicating that the task 
     * was not pending on a shared semaphore or was pending on a
     * shared semaphore but its shared TCB has been removed from the
     * shared semaphore pendQ.  In that case we simply give the 
     * shared TCB back to the shared TCB partition.
     * If an error occurs while giving back the shared TCB a warning
     * message in sent to the user saying the shared TCB is lost.
     *
     * 2) windDelete has return ALREADY_REMOVED indicating that the task
     * was pending on a shared semaphore but its shared TCB has already
     * been removed from the shared semaphore pendQ by another CPU.
     * Its shared TCB is now in this CPU event list but has not yet
     * shown-up.  In that case we don't free the shared TCB now since
     * it will screw up the CPU event list, the shared TCB will be freed
     * by smObjEventProcess when it will show-up.
     *
     * 3) This is the worst case, windDelete has return ERROR
     * indicating that the task was pending on a shared semaphore 
     * and qFifoRemove has failed when trying to get the lock to
     * the shared semaphore structure.
     * In that case the shared semaphore pendQ is in an inconsistant
     * state because it still contains a shared TCB of task which
     * no longer exist.  We send a message to the user saying
     * that access to a shared structure has failed.
     */

     /* no failure notification until we have a better solution */

    if (pTcb->pSmObjTcb != NULL)			/* sm tcb to free? */
	{
	if (status == OK)	
	    {						/* free sm tcb */
	    (*smObjTcbFreeRtn) (pTcb->pSmObjTcb);	
	    }
	}

    if (Q_FIRST (&pTcb->safetyQHead) != NULL)		/*flush any deleters */
        {

#ifdef WV_INSTRUMENTATION
        /* windview - level 2 event logging */
        EVT_TASK_1 (EVENT_OBJ_TASK, pTcb);      /* log event */
#endif

        windPendQFlush (&pTcb->safetyQHead);
        }

    windExit ();					/* KERNEL EXIT */

    /* we won't get here if we committed suicide */

    taskUnlock ();					/* UNLOCK PREEMPTION */
    taskUnsafe ();					/* TASK UNSAFE */

    return (OK);
    }

/*******************************************************************************
*
* taskSuspend - suspend a task
*
* This routine suspends a specified task.  A task ID of zero results in
* the suspension of the calling task.  Suspension is additive, thus tasks
* can be delayed and suspended, or pended and suspended.  Suspended, delayed
* tasks whose delays expire remain suspended.  Likewise, suspended,
* pended tasks that unblock remain suspended only.
*
* Care should be taken with asynchronous use of this facility.  The specified
* task is suspended regardless of its current state.  The task could, for
* instance, have mutual exclusion to some system resource, such as the network * or system memory partition.  If suspended during such a time, the facilities
* engaged are unavailable, and the situation often ends in deadlock.
*
* This routine is the basis of the debugging and exception handling packages.
* However, as a synchronization mechanism, this facility should be rejected 
* in favor of the more general semaphore facility.
*
* RETURNS: OK, or ERROR if the task cannot be suspended.
*
* ERRNO: S_objLib_OBJ_ID_ERROR
*
*/

STATUS taskSuspend
    (
    int tid             /* task ID of task to suspend */
    )
    {
#ifdef WV_INSTRUMENTATION
    int realTid = (tid == 0 ? (int) taskIdCurrent : tid);

    /* windview - level 1 event logging */
    EVT_OBJ_1 (TASK, (WIND_TCB *)realTid, taskClassId, EVENT_TASKSUSPEND, realTid);
#endif

    if (kernelState)				/* defer work if in kernel */
	{
	if ((tid == 0) || (TASK_ID_VERIFY ((void *)tid) != OK))
	    return (ERROR);

	workQAdd1 ((FUNCPTR)windSuspend, tid);	/* add work to kernel work q */
	return (OK);
	}

    if (tid == 0)
	tid = (int) taskIdCurrent;

    kernelState = TRUE;				/* KERNEL ENTER */

    if (TASK_ID_VERIFY ((void *)tid) != OK)	/* verify task */
	{
	windExit ();				/* KERNEL EXIT */
	return (ERROR);
	}

    windSuspend ((WIND_TCB *) tid);		/* suspend a task */

    windExit ();				/* KERNEL EXIT */

    return (OK);
    }

/*******************************************************************************
*
* taskResume - resume a task
*
* This routine resumes a specified task.  Suspension is cleared, and
* the task operates in the remaining state.
*
* RETURNS: OK, or ERROR if the task cannot be resumed.
*
* ERRNO: S_objLib_OBJ_ID_ERROR
*
*/

STATUS taskResume
    (
    int tid                     /* task ID of task to resume */
    )
    {

#ifdef WV_INSTRUMENTATION
    /* windview - level 1 event logging */
    EVT_OBJ_2 (TASK, (WIND_TCB *)tid, taskClassId,
	       EVENT_TASKRESUME, tid, ((WIND_TCB *)tid)->priority);
#endif

    if (kernelState)				/* defer work if in kernel */
	{
	if ((tid == 0) || (TASK_ID_VERIFY ((void *)tid) != OK))
	    return (ERROR);

#if	CPU==SIMSPARCSUNOS || CPU==SIMSPARCSOLARIS
	bzero ((char *) &((WIND_TCB *) tid)->excInfo, sizeof (EXC_INFO));
#endif	/* CPU==SIMSPARCSUNOS || CPU==SIMSPARCSOLARIS */

	workQAdd1 ((FUNCPTR)windResume, tid);	/* add work to kernel work q */
	return (OK);
	}

    if (tid == 0)				/* zero is calling task */
	return (OK);				/* presumably we're running */

    kernelState = TRUE;				/* KERNEL ENTER */

    if (TASK_ID_VERIFY ((void *)tid) != OK)	/* check for invalid ID */
	{
	windExit ();				/* KERNEL EXIT */
	return (ERROR);
	}

#if	CPU==SIMSPARCSUNOS || CPU==SIMSPARCSOLARIS
	bzero ((char *) &((WIND_TCB *) tid)->excInfo, sizeof (EXC_INFO));
#endif	/* CPU==SIMSPARCSUNOS || CPU==SIMSPARCSOLARIS */

    windResume ((WIND_TCB *) tid);		/* resume task */

    windExit ();				/* KERNEL EXIT */

    return (OK);
    }

/*******************************************************************************
*
* taskRestart - restart a task
*
* This routine "restarts" a task.  The task is first terminated, and then
* reinitialized with the same ID, priority, options, original entry point,
* stack size, and parameters it had when it was terminated.  Self-restarting
* of a calling task is performed by the exception task.  The shell utilizes
* this routine to restart itself when aborted.
*
* NOTE
* If the task has modified any of its start-up parameters, the restarted
* task will start with the changed values.
*
* RETURNS: OK, or ERROR if the task ID is invalid
* or the task could not be restarted.
*
* ERRNO: S_intLib_NOT_ISR_CALLABLE, S_objLib_OBJ_DELETED,
*        S_objLib_OBJ_UNAVAILABLE, S_objLib_OBJ_ID_ERROR,
*        S_smObjLib_NOT_INITIALIZED, S_memLib_NOT_ENOUGH_MEMORY,
*        S_memLib_BLOCK_ERROR, S_taskLib_ILLEGAL_PRIORITY
*
*/

STATUS taskRestart
    (
    int tid                     /* task ID of task to restart */
    )
    {
    char	*name;			/* task name */
    ULONG	priority;		/* priority of task */
    int		options;		/* task options */
    char	*pStackBase;		/* bottom of stack */
    int		stackSize;		/* stack size */
    FUNCPTR	entry;			/* entry point of task */
    int		pArgs[MAX_TASK_ARGS];	/* task's arguments */
    BOOL	error;			/* TRUE if ERROR */
    char	*rename = NULL;		/* place to keep rename */
    int		sizeName;		/* size of task name */
    WIND_TCB	*pTcb;			/* pointer to task control block */

    if (INT_RESTRICT () != OK)			/* no ISR use */
	return (ERROR);

    if ((tid == 0) || (tid == (int)taskIdCurrent))
	{
	/* self restart not possible because we have we have no context
	 * from which to sucessfully terminate or reinitialize ourself.
	 * We defer all self restarts to the exception task.
	 * To self restart within a critical section, we must TASK_UNSAFE ()
	 * until the safeCnt == 0.  Otherwise, the excTask would block
	 * forever trying to terminate us.
	 */

	while (taskIdCurrent->safeCnt > 0)	/* make task unsafe */
	    TASK_UNSAFE ();

#if	CPU==SIMSPARCSUNOS || CPU==SIMSPARCSOLARIS
	bzero ((char *) &taskIdCurrent->excInfo, sizeof (EXC_INFO));
#endif	/* CPU==SIMSPARCSUNOS || CPU==SIMSPARCSOLARIS */

	taskSpawn (restartTaskName, restartTaskPriority, restartTaskOptions,
		   restartTaskStackSize, taskRestart, (int) taskIdCurrent,
		   0, 0, 0, 0, 0, 0, 0, 0, 0);
	FOREVER
	    taskSuspend (0);			/* wait for restart */
	}

    /* the rest of this routine will only be executed by a task other than
     * the one being restarted.
     */

    if ((pTcb = taskTcb (tid)) == NULL)		/* valid task ID? */
	return (ERROR);

    priority    = pTcb->priNormal;		/* get the normal priority */
    options	= pTcb->options;		/* get the current options */
    entry	= pTcb->entry;			/* get the entry point */
    pStackBase	= pTcb->pStackBase;		/* get the bottom of stack */
    stackSize	= (pTcb->pStackEnd - pTcb->pStackBase)*_STACK_DIR;

    taskArgsGet (pTcb, pStackBase, pArgs);	/* get the arguments */

    /* If task was named, its name was alloted from the task stack.  So we
     * get a local copy from the stack so it won't be clobbered during task
     * initialization.
     */

    if ((name = pTcb->name) != NULL)
	{
	sizeName = strlen(name) + 1;
	rename = malloc(sizeName);
	if (rename != NULL)
	    strcpy (rename, name);		/* copy out name */
	name = rename;				/* name points at name copy */
	}

    TASK_SAFE ();				/* TASK SAFE */

    if (taskTerminate (tid) != OK)		/* terminate the task */
	{
	TASK_UNSAFE ();				/* TASK UNSAFE */
	return (ERROR);
	}

    /* Reinitialize task with original name, stack, priority, options,
     * arguments, and entry point.  Then activate it.
     */

#if	CPU==SIMSPARCSUNOS || CPU==SIMHPPA || CPU==SIMSPARCSOLARIS
    bzero ((char *) &pTcb->excInfo, sizeof (EXC_INFO));
#endif	/* CPU==SIMSPARCSUNOS || CPU==SIMHPPA || CPU==SIMSPARCSOLARIS */

    error = ((taskInit (pTcb, name, (int)priority, options, pStackBase,
			stackSize, entry, pArgs[0], pArgs[1], pArgs[2],
			pArgs[3], pArgs[4], pArgs[5], pArgs[6], pArgs[7],
			pArgs[8], pArgs[9]) != OK) ||
	     (taskActivate ((int) pTcb) != OK));

    TASK_UNSAFE ();				/* TASK UNSAFE */

    free (rename);

    return ((error) ? ERROR : OK);
    }

/*******************************************************************************
*
* taskPrioritySet - change the priority of a task
*
* This routine changes a task's priority to a specified priority.
* Priorities range from 0, the highest priority, to 255, the lowest priority.
*
* RETURNS: OK, or ERROR if the task ID is invalid.
*
* ERRNO: S_taskLib_ILLEGAL_PRIORITY, S_objLib_OBJ_ID_ERROR
*
* SEE ALSO: taskPriorityGet()
*/

STATUS taskPrioritySet
    (
    int tid,                    /* task ID */
    int newPriority             /* new priority */
    )
    {
#ifdef WV_INSTRUMENTATION
    int realTid = (tid == 0 ? (int) taskIdCurrent : tid);

    /* windview - level 1 event logging */
    EVT_OBJ_3 (TASK, (WIND_TCB *)realTid, taskClassId, EVENT_TASKPRIORITYSET,
	realTid, newPriority, ((WIND_TCB *)realTid)->priority);
#endif

    /* make sure priority is in range 0-255 */

    if (taskPriRangeCheck)
	{
        if ((newPriority & 0xffffff00) != 0)	/* ! (0 <= x <= 255) */
	    {
	    errno = S_taskLib_ILLEGAL_PRIORITY;
	    return (ERROR);
	    }
	}

    if (kernelState)					/* already in kernel? */
        {
	if ((tid == 0) ||
	    (TASK_ID_VERIFY ((void *)tid) != OK))	/* verify task ID */
	    return (ERROR);

        workQAdd2 ((FUNCPTR)windPriNormalSet, tid, newPriority);
	return (OK);
	}

    if (tid == 0)
	tid = (int) taskIdCurrent;

    kernelState = TRUE;					/* KERNEL ENTER */

    if (TASK_ID_VERIFY ((void *)tid) != OK)			/* verify task ID */
	{
	windExit ();					/* KERNEL EXIT */
	return (ERROR);
	}

    windPriNormalSet ((WIND_TCB *) tid, (UINT) newPriority);

    windExit ();					/* KERNEL EXIT */

    return (OK);
    }

/*******************************************************************************
*
* taskPriorityGet - examine the priority of a task
*
* This routine determines the current priority of a specified task.
* The current priority is copied to the integer pointed to by <pPriority>.
*
* RETURNS: OK, or ERROR if the task ID is invalid.
*
* ERRNO: S_objLib_OBJ_ID_ERROR
*
* SEE ALSO: taskPrioritySet()
*/

STATUS taskPriorityGet
    (
    int tid,                    /* task ID */
    int *pPriority              /* return priority here */
    )
    {
    WIND_TCB *pTcb = taskTcb (tid);

    if (pTcb == NULL)				/* invalid task ID */
	return (ERROR);

    *pPriority = (int) pTcb->priority;

    return (OK);
    }

/*******************************************************************************
*
* taskLock - disable task rescheduling
*
* This routine disables task context switching.  The task that calls this
* routine will be the only task that is allowed to execute, unless the task
* explicitly gives up the CPU by making itself no longer ready.  Typically
* this call is paired with taskUnlock(); together they surround a critical
* section of code.  These preemption locks are implemented with a counting
* variable that allows nested preemption locks.  Preemption will not be
* unlocked until taskUnlock() has been called as many times as taskLock().
*
* This routine does not lock out interrupts; use intLock() to lock out
* interrupts.
*
* A taskLock() is preferable to intLock() as a means of mutual exclusion,
* because interrupt lock-outs add interrupt latency to the system.
*
* A semTake() is preferable to taskLock() as a means of mutual exclusion,
* because preemption lock-outs add preemptive latency to the system.
*
* The taskLock() routine is not callable from interrupt service routines.
*
* RETURNS: OK or ERROR.
*
* ERRNO: S_objLib_OBJ_ID_ERROR, S_intLib_NOT_ISR_CALLABLE
*
* SEE ALSO: taskUnlock(), intLock(), taskSafe(), semTake()
*/

STATUS taskLock (void)
    {
    TASK_LOCK();					/* increment lock cnt */

    return (OK);
    }

/*******************************************************************************
*
* taskUnlock - enable task rescheduling
*
* This routine decrements the preemption lock count.  Typically this call is
* paired with taskLock() and concludes a critical section of code.
* Preemption will not be unlocked until taskUnlock() has been called as many
* times as taskLock().  When the lock count is decremented to zero, any tasks
* that were eligible to preempt the current task will execute.
*
* The taskUnlock() routine is not callable from interrupt service routines.
*
* RETURNS: OK or ERROR.
*
* ERRNO: S_intLib_NOT_ISR_CALLABLE
*
* SEE ALSO: taskLock()
*/

STATUS taskUnlock (void)
    {
    FAST WIND_TCB *pTcb = taskIdCurrent;

#ifdef WV_INSTRUMENTATION
    /* windview - level 3 event logging */
    EVT_CTX_0 (EVENT_TASKUNLOCK);
#endif

    if ((pTcb->lockCnt > 0) && ((-- pTcb->lockCnt) == 0)) /* unlocked? */
	{
	kernelState = TRUE;				/* KERNEL ENTER */

	if ((Q_FIRST (&pTcb->safetyQHead) != NULL) && (pTcb->safeCnt == 0))
            {

#ifdef WV_INSTRUMENTATION
	    /* windview - level 2 event logging */
	    EVT_TASK_1 (EVENT_OBJ_TASK, pTcb);		/* log event */
#endif

            windPendQFlush (&pTcb->safetyQHead);        /* flush safe queue */
            }

	windExit ();					/* KERNEL EXIT */
	}

    return (OK);
    }

/*******************************************************************************
*
* taskSafe - make the calling task safe from deletion
*
* This routine protects the calling task from deletion.  Tasks that attempt
* to delete a protected task will block until the task is made unsafe, using
* taskUnsafe().  When a task becomes unsafe, the deleter will be unblocked
* and allowed to delete the task.
*
* The taskSafe() primitive utilizes a count to keep track of nested
* calls for task protection.  When nesting occurs, the task
* becomes unsafe only after the outermost taskUnsafe() is executed.
*
* RETURNS: OK.
*
* SEE ALSO: taskUnsafe(),
* .pG "Basic OS"
*/

STATUS taskSafe (void)
    {
    TASK_SAFE ();					/* increment safe cnt */

    return (OK);
    }

/*******************************************************************************
*
* taskUnsafe - make the calling task unsafe from deletion
*
* This routine removes the calling task's protection from deletion.  Tasks
* that attempt to delete a protected task will block until the task is unsafe.
* When a task becomes unsafe, the deleter will be unblocked and allowed to
* delete the task.
*
* The taskUnsafe() primitive utilizes a count to keep track of nested
* calls for task protection.  When nesting occurs, the task
* becomes unsafe only after the outermost taskUnsafe() is executed.
*
* RETURNS: OK.
*
* SEE ALSO: taskSafe(),
* .pG "Basic OS"
*/

STATUS taskUnsafe (void)
    {
    FAST WIND_TCB *pTcb = taskIdCurrent;

#ifdef WV_INSTRUMENTATION
    /* windview - level 1 event logging */
    EVT_OBJ_2 (TASK, pTcb, taskClassId, EVENT_TASKUNSAFE, pTcb, pTcb->safeCnt);
#endif

    if ((pTcb->safeCnt > 0) && ((-- pTcb->safeCnt) == 0)) /* unsafe? */
	{
	kernelState = TRUE;				/* KERNEL ENTER */

        if (Q_FIRST (&pTcb->safetyQHead) != NULL)	/* deleter waiting? */
            {

#ifdef WV_INSTRUMENTATION
	    /* windview - level 2 event logging */
	    EVT_TASK_1 (EVENT_OBJ_TASK, pTcb);     /* log event */
#endif

            windPendQFlush (&pTcb->safetyQHead);
            }

	windExit ();					/* KERNEL EXIT */
	}

    return (OK);
    }

/*******************************************************************************
*
* taskDelay - delay a task from executing
*
* This routine causes the calling task to relinquish the CPU for the duration
* specified (in ticks).  This is commonly referred to as manual rescheduling,
* but it is also useful when waiting for some external condition that does not
* have an interrupt associated with it.
*
* If the calling task receives a signal that is not being blocked or ignored,
* taskDelay() returns ERROR and sets `errno' to EINTR after the signal
* handler is run.
*
* RETURNS: OK, or ERROR if called from interrupt level or if the calling task
* receives a signal that is not blocked or ignored.
*
* ERRNO: S_intLib_NOT_ISR_CALLABLE, EINTR
*
*/

STATUS taskDelay
    (
    int ticks           /* number of ticks to delay task */
    )
    {
    STATUS status;
   
#ifdef WV_INSTRUMENTATION
    int tid = (int) taskIdCurrent;
#endif

    if (INT_RESTRICT () != OK)			/* restrict ISR use */
	return (ERROR);

#ifdef WV_INSTRUMENTATION
    /* windview - level 1 event logging */
    EVT_OBJ_1 (TASK, ((WIND_TCB *)tid), taskClassId, EVENT_TASKDELAY, ticks);
#endif

    kernelState = TRUE;				/* KERNEL ENTER */

    if (ticks == NO_WAIT) 			/* NO_WAIT = resort in ready q*/
	{
	Q_REMOVE (&readyQHead, taskIdCurrent);	/* Q_RESORT incorrect here */
	Q_PUT (&readyQHead, taskIdCurrent, taskIdCurrent->priority);
	}
    else
	windDelay (ticks);			/* grab tick winks */

    if ((status = windExit()) == RESTART)       /* KERNEL EXIT */
	{
	status = ERROR;
	errno = EINTR;
	}

    return (status);
    }

/*******************************************************************************
*
* taskUndelay - wake up a sleeping task
*
* This routine removes the specified task from the delay queue.  The sleeping
* task will be returned ERROR.
*
* RETURNS: OK, or ERROR if the task ID is invalid or the task not delayed.
*
* NOMANUAL
*/

STATUS taskUndelay
    (
    int tid             /* task to wakeup */
    )
    {
    if (kernelState)
	{
	if (TASK_ID_VERIFY ((void *)tid) != OK)		/* verify task ID */
	    return (ERROR);

	workQAdd1 (windUndelay, tid);		/* defer undelay */
	return (OK);
	}

    kernelState = TRUE;				/* KERNEL ENTER */

    if (TASK_ID_VERIFY ((void *)tid) != OK)		/* check for bad ID */
	{
	windExit ();				/* KERNEL EXIT */
	return (ERROR);
	}

    windUndelay ((WIND_TCB *) tid);		/* wake up task */

    windExit ();				/* KERNEL EXIT */

    return (OK);
    }

/*******************************************************************************
*
* taskIdSelf - get the task ID of a running task
*
* This routine gets the task ID of the calling task.  The task ID will be
* invalid if called at interrupt level.
*
* RETURNS: The task ID of the calling task.
*/

int taskIdSelf (void)
    {
    return ((int)taskIdCurrent);
    }

/*******************************************************************************
*
* taskIdVerify - verify the existence of a task
*
* This routine verifies the existence of a specified task by validating the
* specified ID as a task ID.  Note that an exception occurs if the task ID
* parameter points to an address not located in physical memory.
*
* RETURNS: OK, or ERROR if the task ID is invalid.
*
* ERRNO: S_objLib_OBJ_ID_ERROR
*/

STATUS taskIdVerify
    (
    int tid     /* task ID */
    )
    {
    return (TASK_ID_VERIFY ((void *)tid));
    }

/*******************************************************************************
*
* taskTcb - get the task control block for a task ID
*
* This routine returns a pointer to the task control block (WIND_TCB) for a
* specified task.  Although all task state information is contained in the
* TCB, users must not modify it directly.  To change registers, for instance,
* use taskRegsSet() and taskRegsGet().
*
* RETURNS: A pointer to a WIND_TCB, or NULL if the task ID is invalid.
*
* ERRNO: S_objLib_OBJ_ID_ERROR
*/

WIND_TCB *taskTcb
    (
    int tid                     /* task ID */
    )
    {
    if (tid == 0)
	tid = (int)taskIdCurrent;		/* 0 tid means self */

    if (TASK_ID_VERIFY ((void *)tid) != OK)		/* verify task ID */
	return (NULL);

    return ((WIND_TCB *) tid);			/* tid == pTcb */
    }

/*******************************************************************************
*
* taskStackAllot - allot memory from caller's stack
*
* This routine allots the specified amount of memory from the end of the
* caller's stack.  This is a non-blocking operation.  This routine is utilized
* by task create hooks to allocate any additional memory they need.  The memory
* cannot be added back to the stack.  It will be reclaimed as part of the
* reclamation of the task stack when the task is deleted.
*
* Note that a stack crash will overwrite the allotments made from this routine
* because all portions are carved from the end of the stack.
*
* This routine will return NULL if requested size exceeds available stack
* memory.
*
* RETURNS: pointer to block, or NULL if unsuccessful.
*
* NOMANUAL
*/

void *taskStackAllot
    (
    int      tid,               /* task whose stack will be allotted from */
    unsigned nBytes             /* number of bytes to allot */
    )
    {
    char     *pStackPrevious;
    WIND_TCB *pTcb = taskTcb (tid);

    nBytes = STACK_ROUND_UP (nBytes);		/* round up request */

    if ((pTcb == NULL) ||
	(nBytes > (pTcb->pStackLimit - pTcb->pStackBase) * _STACK_DIR))
	{
	return (NULL);
	}

    pStackPrevious = pTcb->pStackLimit;		/* allot from end of stack */

#if	(_STACK_DIR == _STACK_GROWS_DOWN)

    pTcb->pStackLimit += nBytes;		/* move up end of stack */
    return ((void *)pStackPrevious);

#else	/* _STACK_GROWS_UP */

    pTcb->pStackLimit -= nBytes;		/* move down end of stack */
    return ((void *)pTcb->pStackLimit);

#endif	/* _STACK_GROWS_UP */

    }
