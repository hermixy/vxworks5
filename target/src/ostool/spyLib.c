/* spyLib.c - spy CPU activity library */

/* Copyright 1984-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
03n,16oct01,jn   use symFindSymbol for symbol lookup (SPR #7453)
03m,14mar99,jdi  doc: removed refs to config.h and/or configAll.h (SPR 25663).
03l,18sep97,kkk	 fixed typo in spyClkInt() (SPR# 8886), updated copyright.
03k,22oct95,jdi  doc: removed references to dbxtool/dbxTask (SPR 4378).
03j,12jun95,p_m  updated man pages.
03i,27may95,p_m  implemented decoupling of the result display to allow
		 host browser spy to run silently.
		 Moved spyHelp to usrLib.c.
03h,01nov94,kdl  merge cleanup - fixed spyStackSize definition.
03g,02dec93,tpr  increased spy stack size for Am29K.
03g,02may94,ms   increased stacksize for the simulator.
03f,13feb93,kdl  changed cplusLib.h to private/cplusLibP.h (SPR #1917).
03e,20jan93,jdi  documentation cleanup for 5.1.
03d,31oct92,jcf  increased spy stack size for 68k.
03c,31oct92,ccc  increased spy stack size for I960.
03b,04aug92,ajm  fixed define of nameToPrint to be char *
03a,01aug92,srh  added C++ demangling idiom to spyReport
02z,30jul92,smb  changed format for printf to avoid zero padding.
02y,04jul92,jcf  scalable/ANSI/cleanup effort.
02x,26may92,rrr  the tree shuffle
02w,22apr92,jwt  converted CPU==SPARC to CPU_FAMILY==SPARC; copyright.
02v,23mar92,jmm  changed spyClkStart and spyReport to check for null ptrs
                 SPR #1384
02u,19nov91,rrr  shut up some ansi warnings.
02t,15oct91,ajm  modified spy stack size for MIPS too
02s,15oct91,jwt  02q merge never did initialize auxClkTickPerSecond to 60.
02r,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed VOID to void
		  -changed copyright notice
02q,23may91,jwt  modified spy stack size, init auxClkTickPerSecond for SPARC.
02p,05apr91,jdi	 documentation -- removed header parens and x-ref numbers;
		 doc review by shl.
02o,24mar91,jdi  documentation cleanup.
02n,10aug90,dnw  added forward declaration of spyTaskIdListSort().
02m,28jun90,shl  cleared sysKernelTicks and sysKernelIncTicks in sysSpyStart().
02l,21jun90,shl	 added task names and priorities to spy report,
		 display of tasks now ordered by priority.
		 added spyTaskIdListSort().
	         set auxClkRate back to its original value after spyStop().
		 replaced sysAuxClkDisconnect() by sysAuxClkDisable().
02k,14apr90,jcf  remove tcbx dependencies
		 added kernel state time.
		 changed name of spyTask to tSpyTask.
02j,19mar90,jdi  documentation cleanup.
02i,20aug88,gae  documentation.
02h,22jun88,dnw  name tweaks.
		 clean-up.
02g,07jun88,gae  added call to sysClkEnable ().
02f,06jun88,dnw  changed taskSpawn/taskCreate args.
02e,04jun88,gae  changed spyStart() to spyClkStart().
		 added spyClkStop().
02c,30may88,dnw  changed to v4 names.
02b,24mar88,gae  fixed spyReport() formatting, % calculations, prints
		   out tasks that have no ticks & newly created tasks.
		 fixed bugs allowing multiple spy's & extra delete hooks.
02a,07mar88,gae  made it work with generic kernel -- does not use
		   static array for tasks.
		 Renamed module and all routines to "spy".
		 Added spy{Create,Delete}Task() to correctly handle
		   variable number of tasks between reports.
01i,04nov87,ecs  documentation.
01h,02apr87,ecs  eased lint's mind in taReport.
01g,23mar87,jlf  documentation.
01f,20feb87,jlf  made taStart check the parameter returned by
		   sysAuxClkConnect, to determine whether the board has
		   an aux clock on it.
01e,21dec86,dnw  changed to not get include files from default directories.
01d,19nov86,llk  Added spyTask.
01c,06nov86,jlf  Changed to use new CPU-independent routines, sysAuxClk...
01b,24oct85,dnw  changed taSpy to explicity spawn periodRun since default
		   parameters supplied by period, weren't adequate.
01a,22oct85,jlf  written.
*/

/*
DESCRIPTION
This library provides a facility to monitor tasks' use of the CPU.
The primary interface routine, spy(), periodically calls spyReport() to
display the amount of CPU time utilized by each task, the amount of time
spent at interrupt level, the amount of time spent in the kernel, and the
amount of idle time.  It also displays the total usage since the start of
spy() (or the last call to spyClkStart()), and the change in usage since
the last spyReport().

CPU usage can also be monitored manually by calling spyClkStart() and
spyReport(), instead of spy().  In this case, spyReport() provides a one-time
report of the same information provided by spy().

Data is gathered by an interrupt-level routine that is connected by
spyClkStart() to the auxiliary clock.  Currently, this facility cannot be
used with CPUs that have no auxiliary clock.  Interrupts that are at a
higher level than the auxiliary clock's interrupt level cannot be
monitored.

All user interface routine except spyLibInit() are available through usrLib.

EXAMPLE
The following call:
.CS
    -> spy 10, 200
.CE
will generate a report in the following format every 10 seconds,
gathering data at the rate of 200 times per second.
.CS
.ne 5
\&NAME          ENTRY       TID   PRI  total % (ticks)  delta % (ticks)
\&--------     --------    -----  ---  ---------------  ---------------
\&tExcTask     _excTask    fbb58    0    0% (       0)    0% (       0)
\&tLogTask     _logTask    fa6e0    0    0% (       0)    0% (       0)
\&tShell       _shell      e28a8    1    0% (       4)    0% (       0)
\&tRlogind     _rlogind    f08dc    2    0% (       0)    0% (       0)
\&tRlogOutTask _rlogOutTa  e93e0    2    2% (     173)    2% (      46)
\&tRlogInTask  _rlogInTas  e7f10    2    0% (       0)    0% (       0)
\&tSpyTask     _spyTask    ffe9c    5    1% (     116)    1% (      28)
\&tNetTask     _netTask    f3e2c   50    0% (       4)    0% (       1)
\&tPortmapd    _portmapd   ef240  100    0% (       0)    0% (       0)
\&KERNEL                                 1% (     105)    0% (      10)
\&INTERRUPT                              0% (       0)    0% (       0)
\&IDLE                                  95% (    7990)   95% (    1998)
\&TOTAL                                 99% (    8337)   98% (    2083)
.CE
The "total" column reflects CPU activity since the initial call to spy()
or the last call to spyClkStart().  The "delta" column reflects activity
since the previous report.  A call to spyReport() will produce a single
report; however, the initial auxiliary clock interrupts and data collection
must first be started using spyClkStart().

Data collection/clock interrupts and periodic reporting are stopped by
calling:
.CS
    -> spyStop
.CE

INCLUDE FILES: spyLib.h

SEE ALSO: usrLib
*/

#include "vxWorks.h"
#include "sysSymTbl.h"
#include "sysLib.h"
#include "taskHookLib.h"
#include "logLib.h"
#include "stdio.h"
#include "intLib.h"
#include "spyLib.h"
#include "private/cplusLibP.h"
#include "private/windLibP.h"


#define MAX_SPY_TASKS	200		/* max tasks that can be spy'd */

/* spyTask parameters */

int spyTaskId		= ERROR;	/* ERROR = spy task not active */
int spyTaskOptions	= VX_UNBREAKABLE;
int spyTaskPriority	= 5;

#if	((CPU_FAMILY==SPARC) || (CPU_FAMILY==MIPS) || (CPU_FAMILY==I960) || \
	 (CPU_FAMILY== AM29XXX)  ||  (CPU_FAMILY== SIMSPARCSUNOS) || \
	 (CPU_FAMILY==SIMHPPA))

int spyTaskStackSize	= 10000;

#else

int spyTaskStackSize	= 6000;

#endif	/* CPU_FAMILY==SPARC || CPU_FAMILY==MIPS || CPU_FAMILY==I960 ... */

/* global variables */

/* 
 * The variables below must remain declared in the order they are declared 
 * here since they are read by host tools using gopher.
 */

UINT spyTotalTicks;			/* all ticks since start */

UINT spyIncTicks;			/* all ticks since last report */

UINT spyInterruptTicks;			/* int ticks since start */
UINT spyInterruptIncTicks;		/* int ticks since last report */

UINT spyKernelTicks;			/* int ticks since start */
UINT spyKernelIncTicks;			/* int ticks since last report */

UINT spyIdleTicks;			/* idle ticks since start */
UINT spyIdleIncTicks;			/* idle ticks since last report*/

/* local variables */

LOCAL BOOL spyClkRunning;		/* TRUE = spyClkStart'ed */
LOCAL BOOL spyInitialized;		/* TRUE = hooks installed */
LOCAL int  spyCreateCount;		/* num tasks created since report */
LOCAL int  spyDeleteCount;		/* num tasks deleted since report */

#if	CPU_FAMILY==SPARC
LOCAL int  auxClkTicksPerSecond = 60;
#else	/* CPU_FAMILY==SPARC */
LOCAL int  auxClkTicksPerSecond;
#endif	/* CPU_FAMILY==SPARC */

/* display formats */

LOCAL char *spyFmt1 = "%-12.12s %-10.10s %8x  %3d   %3d%% (%8d)  %3d%% (%8d)\n";
LOCAL char *spyFmt2 = "%-12.12s %-10.10s %8s  %3s   %3d%% (%8d)  %3d%% (%8d)\n";


/* forward static functions */

static void spyCreateHook (WIND_TCB *pTcb);
static void spyDeleteHook (WIND_TCB *pTcb);
static void spyClkInt (void);
static void spyTaskIdListSort (int idList [ ], int nTasks);

/*******************************************************************************
*
* spyLibInit - initialize task cpu utilization tool package
*
* This routine initializes the task cpu utilization tool package.  
* If the configuration macro INCLUDE_SPY is defined, it is called by the root 
* task, usrRoot(), in usrConfig.c.
*
* RETURNS: N/A
*
* SEE ALSO: usrLib
*/

void spyLibInit (void)
    {
    _func_spy		= (FUNCPTR) spyCommon;
    _func_spyStop	= (FUNCPTR) spyStopCommon;
    _func_spyClkStart 	= (FUNCPTR) spyClkStartCommon;
    _func_spyClkStop 	= (FUNCPTR) spyClkStopCommon;
    _func_spyTask 	= (FUNCPTR) spyComTask;
    _func_spyReport	= (FUNCPTR) spyReportCommon;
    }

/*******************************************************************************
*
* spyCreateHook - initialize task tick counters
*
* This routine is installed as a task create hook so that the task tick
* counters can be initialized and spyReport() can indicate when new tasks
* appear between reports.
*
* RETURNS: N/A
*/

LOCAL void spyCreateHook
    (
    WIND_TCB *pTcb      /* WIND_TCB of new task */
    )
    {
    pTcb->taskTicks    = 0;
    pTcb->taskIncTicks = 0;
    spyCreateCount++;
    }
/*******************************************************************************
*
* spyDeleteHook - notify spyLib of task deletion
*
* This routine is installed as a task delete hook so that spyReport()
* can indicate when tasks disappear between reports.
*
* RETURNS: N/A
*
* ARGSUSED
*/

LOCAL void spyDeleteHook
    (
    WIND_TCB *pTcb      /* WIND_TCB of new task */
    )
    {
    spyDeleteCount++;
    }
/*******************************************************************************
*
* spyClkInt - spyLib interrupt service routine
*
* This routine is called at each tick of the auxiliary clock.  When at
* interrupt level the interrupt tick count is incremented.  If there is no
* active task then the idle tick count is incremented, otherwise increment
* the active task's tick counter.
*
* RETURNS: N/A
*/

LOCAL void spyClkInt (void)
    {
    if (intCount () > 1)		/* we interrupted an interrupt */
	spyInterruptIncTicks++;
    else if (kernelIsIdle)		/* we interrupted idle state */
	spyIdleIncTicks++;
    else if (kernelState)		/* we interrupted the kernel */
	spyKernelIncTicks++;
    else
	{
	if (taskIsReady ((int) taskIdCurrent))
	    taskIdCurrent->taskIncTicks++;
	else
	    logMsg ("spyClkInt: taskIdCurrent not ready.\n",0,0,0,0,0,0);
	}

    spyIncTicks++;
    }
/*******************************************************************************
*
* spyClkStartCommon - start collecting task activity data
*
* This routine begins data collection by enabling the auxiliary clock
* interrupts at a frequency of <intsPerSec> interrupts per second.  If
* <intsPerSec> is omitted or zero, the frequency will be 100.  Data from
* previous collections is cleared.
*
* RETURNS:
* OK, or ERROR if the CPU has no auxiliary clock, or if task create and
* delete hooks cannot be installed.
*
* SEE ALSO: sysAuxClkConnect()
*
* NOMANUAL
*/

STATUS spyClkStartCommon
    (
    int		intsPerSec,	/* timer interrupt freq, 0 = default of 100 */
    FUNCPTR	printRtn	/* routine to display result */
    )
    {
    FAST int ix;
    FAST int nTasks;
    FAST WIND_TCB *pTcb;
    int idList [MAX_SPY_TASKS];

    if (spyClkRunning)
	return (ERROR);		/* We're already going */

    if ((_func_taskCreateHookAdd != NULL) && (_func_taskDeleteHookAdd != NULL))
	{
	if (!spyInitialized &&
	    ((* _func_taskCreateHookAdd) ((FUNCPTR)spyCreateHook) == ERROR ||
	     (* _func_taskDeleteHookAdd) ((FUNCPTR)spyDeleteHook) == ERROR))
	    {
	    if (printRtn != NULL)
		(* printRtn) ("Unable to add create/delete hooks.\n");

	    return (ERROR);
	    }
	}

    spyInitialized = TRUE;

    if (intsPerSec == 0)
	intsPerSec = 100;

    spyDeleteCount  = 0;
    spyCreateCount  = 0;
    spyTotalTicks = spyIdleTicks  = spyKernelTicks    = spyInterruptTicks    =0;
    spyIncTicks = spyIdleIncTicks = spyKernelIncTicks = spyInterruptIncTicks =0;

    /* initialize tick counters of tasks already running */

    nTasks = taskIdListGet (idList, NELEMENTS (idList));

    for (ix = 0; ix < nTasks; ++ix)
	{
	pTcb = taskTcb (idList [ix]);

	/*
	 * Need to make sure pTcb is a valid pointer
	 */

	if (pTcb == NULL)
	    continue;

	pTcb->taskIncTicks = pTcb->taskTicks = 0;
	}

    if (sysAuxClkConnect ((FUNCPTR)spyClkInt, 0) != OK)
	{
	if (printRtn != NULL)
	    (* printRtn) ("No auxiliary clock on CPU.\n");
	return (ERROR);
	}

    auxClkTicksPerSecond = sysAuxClkRateGet ();

    sysAuxClkRateSet (intsPerSec);
    sysAuxClkEnable ();

    spyClkRunning = TRUE;

    return (OK);
    }
/*******************************************************************************
*
* spyClkStopCommon - stop collecting task activity data
*
* This routine disables the auxiliary clock interrupts.
* Data collected remains valid until the next spyClkStart() call.
*
* RETURNS: N/A
*
* SEE ALSO: spyClkStart()
*
* NOMANUAL
*/

void spyClkStopCommon (void)
    {
    sysAuxClkRateSet (auxClkTicksPerSecond);
    sysAuxClkDisable ();
    spyClkRunning  = FALSE;
    }
/*******************************************************************************
*
* spyReportCommon - display task activity data
*
* This routine reports on data gathered at interrupt level for the amount of
* CPU time utilized by each task, the amount of time spent at interrupt level,
* the amount of time spent in the kernel, and the amount of idle time.  Time
* is displayed in ticks and as a percentage, and the data is shown since both
* the last call to spyClkStart() and the last spyReport().  If no interrupts
* have occurred since the last spyReport(), nothing is displayed.
*
* RETURNS: N/A
*
* SEE ALSO: spyClkStart()
*
* NOMANUAL
*/

void spyReportCommon 
    (
    FUNCPTR printRtn		/* routine to display result */
    )
    {
    FAST WIND_TCB *pTcb;
    FAST int 	   ix;
    SYMBOL_ID      symId;
    FUNCPTR 	   symbolAddress = 0;
    char 	   *name;       /* pointer to symbol table copy of string */
    int 	   taskPriority;
    char	   demangled [MAX_SYS_SYM_LEN + 1];
    char	   *nameToPrint;

    int 	   idList [MAX_SPY_TASKS];	/* task specific statistics */
    int 	   taskIncTicks [MAX_SPY_TASKS];
    int 	   taskTotalTicks [MAX_SPY_TASKS];
    FAST int 	   nTasks;

    int 	   tmpIncTicks;			/* incremental snap shot */
    int 	   tmpIdleIncTicks;
    int 	   tmpKernelIncTicks;
    int 	   tmpInterruptIncTicks;

    int 	   totalPerCent;
    int 	   incPerCent;
    int 	   sumTotalPerCent = 0;
    int 	   sumIncPerCent   = 0;

    /* if there have been no ticks, there is nothing to report */

    if (spyIncTicks == 0)
	return;

    /* snap shot and clear task statistics */

    nTasks = taskIdListGet (idList, NELEMENTS (idList));

    spyTaskIdListSort (idList, nTasks);

    for (ix = 0; ix < nTasks; ++ix)
	{
	pTcb = taskTcb (idList [ix]);

	/*
	 * Need to make sure pTcb is a valid pointer
	 */

	if (pTcb == NULL)
	    continue;


	/* order is important: save and clear incremental, then update total */

	taskIncTicks [ix]    = pTcb->taskIncTicks;
	pTcb->taskIncTicks  = 0;

	pTcb->taskTicks    += taskIncTicks [ix];
	taskTotalTicks [ix]  = pTcb->taskTicks;
	}


    /* save and clear incremental counts and accumulate totals */

    tmpIncTicks          = spyIncTicks;
    tmpIdleIncTicks      = spyIdleIncTicks;
    tmpKernelIncTicks    = spyKernelIncTicks;
    tmpInterruptIncTicks = spyInterruptIncTicks;

    spyIncTicks = spyIdleIncTicks = spyKernelIncTicks = spyInterruptIncTicks =0;

    spyTotalTicks       += tmpIncTicks;
    spyInterruptTicks   += tmpInterruptIncTicks;
    spyKernelTicks      += tmpKernelIncTicks;
    spyIdleTicks        += tmpIdleIncTicks;


    if (printRtn == NULL)	/* for host browser don't display result */
	return;

    /* print info */

    (* printRtn) ("\n");
    (* printRtn) (
    "NAME          ENTRY         TID   PRI   total %% (ticks)  delta %% (ticks)\n");
    (* printRtn) (
    "--------     --------      -----  ---   ---------------  ---------------\n");


    for (ix = 0; ix < nTasks; ++ix)
	{
	/* find name in symbol table */

	pTcb = taskTcb (idList [ix]);

	/*
	 * Need to make sure pTcb is a valid pointer
	 */

	if (pTcb == NULL)
	    continue;

        /* 
	 * Only check one symLib function pointer (for performance's sake). All
	 * symLib functions are provided by the same library, by convention.    
	 */

	if ((_func_symFindSymbol !=(FUNCPTR) NULL) &&
	    (sysSymTbl != NULL))
	    {
	    if ((* _func_symFindSymbol) (sysSymTbl,  NULL, 
					 (void *)pTcb->entry, 
					 SYM_MASK_NONE, SYM_MASK_NONE, 
					 &symId) == OK)
	        {
		(* _func_symNameGet) (symId, &name);
		(* _func_symValueGet) (symId, (void **) &symbolAddress); 
		}
	    }

	if (symbolAddress != pTcb->entry)
	    name = "\0";	         /* no matching symbol */
	    
        taskPriorityGet (idList [ix], &taskPriority);

	/* print line for this task */

	totalPerCent     = (taskTotalTicks [ix] * 100) / spyTotalTicks;
	incPerCent       = (taskIncTicks [ix] * 100) / tmpIncTicks;
	sumTotalPerCent += totalPerCent;
	sumIncPerCent   += incPerCent;

	nameToPrint = cplusDemangle (name, demangled, sizeof (demangled));
	(* printRtn) (spyFmt1, pTcb->name, nameToPrint, idList [ix], 
		      taskPriority, totalPerCent, taskTotalTicks [ix],
		      incPerCent, taskIncTicks [ix]);
	}

    totalPerCent     = (spyKernelTicks * 100) / spyTotalTicks;
    incPerCent       = (tmpKernelIncTicks * 100) / tmpIncTicks;
    sumTotalPerCent += totalPerCent;
    sumIncPerCent   += incPerCent;

    (* printRtn) (spyFmt2, "KERNEL", "", "", "", totalPerCent, spyKernelTicks,
				      incPerCent, tmpKernelIncTicks);


    totalPerCent     = (spyInterruptTicks * 100) / spyTotalTicks;
    incPerCent       = (tmpInterruptIncTicks * 100) / tmpIncTicks;
    sumTotalPerCent += totalPerCent;
    sumIncPerCent   += incPerCent;

    (* printRtn) (spyFmt2, "INTERRUPT", "", "", "", totalPerCent, spyInterruptTicks,
				      incPerCent, tmpInterruptIncTicks);

    totalPerCent     = (spyIdleTicks * 100) / spyTotalTicks;
    incPerCent       = (tmpIdleIncTicks * 100) / tmpIncTicks;
    sumTotalPerCent += totalPerCent;
    sumIncPerCent   += incPerCent;

    (* printRtn) (spyFmt2, "IDLE", "", "", "", totalPerCent, spyIdleTicks,
				 incPerCent, tmpIdleIncTicks);

    (* printRtn) (spyFmt2, "TOTAL", "", "", "", sumTotalPerCent, spyTotalTicks,
				  sumIncPerCent, tmpIncTicks);

    (* printRtn) ("\n");

    if (spyCreateCount > 0)
	{
	(* printRtn) ("%d task%s created.\n", spyCreateCount,
		spyCreateCount == 1 ? " was" : "s were");
	spyCreateCount = 0;
	}

    if (spyDeleteCount > 0)
	{
	(* printRtn) ("%d task%s deleted.\n", spyDeleteCount,
		spyDeleteCount == 1 ? " was" : "s were");
	spyDeleteCount = 0;
	}
    }
/*******************************************************************************
*
* spyComTask - run periodic task activity reports
*
* This routine is spawned as a task by spy() to provide periodic task
* activity reports.  It prints a report, delays for the specified number of
* seconds, and repeats.
*
* RETURNS: N/A
*
* SEE ALSO: spy()
*
* NOMANUAL
*/

void spyComTask
    (
    int		freq,		/* reporting frequency, in seconds */
    FUNCPTR	printRtn	/* routine to display results */
    )
    {
    int delay = freq * sysClkRateGet ();

    while (TRUE)
	{
	spyReportCommon (printRtn);
	taskDelay (delay);
	}
    }
/*******************************************************************************
*
* spyStopCommon - stop spying and reporting
*
* This routine calls spyClkStop().  Any periodic reporting by spyTask()
* is terminated.
*
* RETURNS: N/A
*
* SEE ALSO: spyClkStop(), spyTask()
*
* NOMANUAL
*/

void spyStopCommon (void)
    {
    spyClkStopCommon ();
    if (spyTaskId != ERROR)
	{
	taskDelete (spyTaskId);
	spyTaskId = ERROR;
	}
    }

/*******************************************************************************
*
* spyCommon - begin periodic task activity reports
*
* This routine collects task activity data and periodically runs spyReport().
* Data is gathered <ticksPerSec> times per second, and a report is made every
* <freq> seconds.  If <freq> is zero, it defaults to 5 seconds.  If
* <ticksPerSec> is omitted or zero, it defaults to 100.
*
* This routine spawns spyTask() to do the actual reporting.
*
* It is not necessary to call spyClkStart() before running spy().
*
* RETURNS: N/A
*
* SEE ALSO: spyClkStart(), spyTask()
*
* NOMANUAL
*/

void spyCommon
    (
    int		freq,		/* reporting freq in sec, 0 = default of 5 */
    int		ticksPerSec,	/* interrupt clock freq, 0 = default of 100 */
    FUNCPTR	printRtn	/* routine to use to display results */
    )
    {
    if (freq == 0)
	freq = 5;	/* default frequency is 5 secs */

    if (spyClkStartCommon (ticksPerSec, printRtn) == OK)
	{
	spyTaskId = taskSpawn ("tSpyTask", spyTaskPriority,
			       spyTaskOptions, spyTaskStackSize,
			       (FUNCPTR)spyComTask, freq, (int) printRtn, 
			       0, 0, 0, 0, 0, 0, 0, 0);

	if ((spyTaskId == ERROR)&& (printRtn != NULL))
	    (* printRtn) ("Unable to spawn spyTask.\n");
	}
    }

/*******************************************************************************
*
* spyTaskIdListSort - sort the ID list by priority
*
* This routine sorts the task ID list <idList> by task priority.
*
* RETURNS: N/A
*/

LOCAL void spyTaskIdListSort
    (
    int idList[],
    int nTasks
    )
    {
    FAST int temp;
    int prevPri;
    int curPri;
    FAST int *pCurId;
    BOOL change = TRUE;
    FAST int *pEndId = &idList [nTasks];

    if (nTasks == 0)
        return;

    while (change)
        {
        change = FALSE;

        taskPriorityGet (idList[0], &prevPri);

        for (pCurId = &idList[1]; pCurId < pEndId; ++pCurId, prevPri = curPri)
            {
            taskPriorityGet (*pCurId, &curPri);

            if (prevPri > curPri)
                {
                temp = *pCurId;
                *pCurId = *(pCurId - 1);
                *(pCurId - 1) = temp;
                change = TRUE;
                }
            }
        }
    }
