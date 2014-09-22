/* taskShow.c - task show routines */

/* Copyright 1984-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
02e,27mar02,pai  Added code to display Streaming SIMD Registers (SPR 74103).
02d,16oct01,jn   use symFindSymbol for symbol lookup (SPR #7453)
02c,19nov01,aeg  added display of VxWorks events information.
02b,09nov01,dee  add CPU_FAMILY==COLDFIRE
02a,11oct01,cjj  removed Am29k support
01z,13jul01,kab  Cleanup for merge to mainline
01y,03apr01,kab  Added _WRS_ALTIVEC_SUPPORT
01x,14mar01,pcs  Added code to recognize VX_ALTIVEC_TASK.
01w,18dec00,pes  Correct compiler warnings
01w,03mar00,zl   merged SH support into T2
01v,17mar99,jdi  doc: updated w/ info about proj facility (SPR 25727).
01u,29jul96,jmb  Merged patch from ease (mem) for HPSIM
01t,24jun96,sbs  made windview instrumentation conditionally compiled
01s,06oct95,jdi  changed Debugging .pG's to .tG "Shell".
01r,14mar95,jdi  doc tweak for taskShow().
01q,11feb95,jdi  format repairs.
01p,04feb94,cd   taskRegsShow calls taskArchRegsShow for CPU_FAMILY==MIPS
		 retrofitted 01o history
01o,27dec93,cd   taskShow calls taskArchRegsShow for CPU_FAMILY==MIPS
01o,02dec93,pad  added AM29XXX family support.
01q,20jul94,ms   added taskRegShow() hook.
01p,03may94,ms   fixed low order PC bits in taskShow for VxSim HP
01o,20aug93,gae  fixed pcReg definition for vxsim on hppa.
01o,20sep94,rhp  doc: describe fields in taskShow() TCB display,
                 result of taskStatusString() (SPR#2394).
01o,16jan94,c_s  taskShowInit () now initializes instrumented class.
01n,25feb93,jdi  doc: fixed example output of taskShow().
01m,04feb93,jdi  fixed mangen problem in taskShow().
01l,03feb93,jdi  amended library description.
01k,03feb93,jdi  changed INCLUDE_SHOW_RTNS to ...ROUTINES.
01j,02feb93,jdi  documentation tweak.
01i,25nov92,jdi  documentation cleanup.
01h,19aug92,smb  another tweak to the printf formatting.
01g,02aug92,jcf  added parameter to _func_excInfoShow.
01f,30jul92,smb  changed format for printf to avoid zero padding.
01e,28jul92,jcf  changed taskShowInit to call taskLibInit.
01d,27jul92,jcf  included errnoLib.h.
01c,12jul92,jcf  tuned register format string.
01b,08jul92,jwt  modified taskRegsFmt for CPU_FAMILY == SPARC.
01a,15jun92,jcf  extracted from v1k taskLib.c.
*/

/*
DESCRIPTION
This library provides routines to show task-related information,
such as register values, task status, etc.

The taskShowInit() routine links the task show facility into the VxWorks
system.  It is called automatically when this show facility is configured
into VxWorks using either of the following methods:
.iP
If you use the configuration header files, define
INCLUDE_SHOW_ROUTINES in config.h.
.iP
If you use the Tornado project facility, select INCLUDE_TASK_SHOW.
.LP

Task information is crucial as a debugging aid and user-interface
convenience during the development cycle of an application.  The routines
taskInfoGet(), taskShow(), taskRegsShow(), and taskStatusString() are used
to display task information.

The chief drawback of using task information is that tasks may
change their state between the time the information is gathered and the
time it is utilized.  Information provided by these routines should
therefore be viewed as a snapshot of the system, and not relied upon
unless the task is consigned to a known state, such as suspended.

Task management and control routines are provided by taskLib.  Programmatic
access to task information and debugging features is provided by taskInfo.

INCLUDE FILES: taskLib.h

SEE ALSO: taskLib, taskInfo, taskHookLib, taskVarLib, semLib, kernelLib,
.pG "Basic OS, Target Shell,"
.tG "Shell"
*/

#include "vxWorks.h"
#include "string.h"
#include "regs.h"
#include "stdio.h"
#include "a_out.h"
#include "sysSymTbl.h"
#include "errnoLib.h"
#include "taskArchLib.h"
#include "intLib.h"			/* intLock/intUnlock */
#include "private/funcBindP.h"
#include "private/taskLibP.h"
#include "private/kernelLibP.h"
#include "private/eventLibP.h"		/* eventTaskShow () */


#define MAX_DSP_TASKS	500		/* max tasks that can be displayed */

/* globals */

char *taskRegsFmt = "%-6s = %8x";
VOIDFUNCPTR _func_taskRegsShowRtn;

/* locals */

LOCAL char infoHdr [] = "\n\
  NAME        ENTRY       TID    PRI   STATUS      PC       SP     ERRNO  DELAY\n\
---------- ------------ -------- --- ---------- -------- -------- ------- -----\n";

/* forward declarations */

LOCAL void taskSummary (TASK_DESC *pTd);
#if (CPU_FAMILY==MIPS || CPU_FAMILY==COLDFIRE)
IMPORT void taskArchRegsShow(REG_SET *pRegSet);
#endif /* (CPU_FAMILY==MIPS) */


/******************************************************************************
*
* taskShowInit - initialize the task show routine facility
*
* This routine links the task show routines into the VxWorks system.
* It is called automatically when the task show facility is
* configured into VxWorks using either of the following methods:
* .iP
* If you use the configuration header files, define
* INCLUDE_SHOW_ROUTINES in config.h.
* .iP
* If you use the Tornado project facility, select INCLUDE_TASK_SHOW.
*
* RETURNS: N/A
*/

void taskShowInit (void)

    {
    if (taskLibInit () == OK)
	{ classShowConnect (taskClassId, (FUNCPTR)taskShow);
#ifdef WV_INSTRUMENTATION
	classShowConnect (taskInstClassId, (FUNCPTR)taskShow);
#endif
	}
    }

/*******************************************************************************
*
* taskInfoGet - get information about a task
*
* This routine fills in a specified task descriptor (TASK_DESC) for a
* specified task.  The information in the task descriptor is, for the most
* part, a copy of information kept in the task control block (WIND_TCB).
* The TASK_DESC structure is useful for common information and avoids
* dealing directly with the unwieldy WIND_TCB.
*
* NOTE
* Examination of WIND_TCBs should be restricted to debugging aids.
*
* RETURNS: OK, or ERROR if the task ID is invalid.
*/

STATUS taskInfoGet
    (
    int         tid,            /* ID of task for which to get info */
    TASK_DESC   *pTaskDesc      /* task descriptor to be filled in */
    )
    {
    int 	    key;
    FAST char	   *pStackHigh;
    FAST TASK_DESC *pTd  = pTaskDesc;
    WIND_TCB 	   *pTcb = taskTcb (tid);

    if (pTcb == NULL)				/* valid task ID? */
	return (ERROR);

    /* gather the information */

#if CPU_FAMILY==I960
    if (tid != taskIdSelf ())	/* showing our current sp is not possible */
	taskRegsStackToTcb (pTcb);
#endif	/* CPU_FAMILY==I960 */

    pTd->td_id		= (int) pTcb;			/* task ID */
    pTd->td_name	= pTcb->name;			/* name of task */
    pTd->td_priority	= (int) pTcb->priority; 	/* priority */
    pTd->td_status	= pTcb->status;			/* task status*/
    pTd->td_options	= pTcb->options;		/* task option bits */
    pTd->td_entry	= pTcb->entry;			/* entry of task */
    pTd->td_sp		= (char *)((int)pTcb->regs.spReg);	/* saved stack ptr */

    pTd->td_pStackLimit	= pTcb->pStackLimit;		/* limit of stack */
    pTd->td_pStackBase	= pTcb->pStackBase;		/* bottom of stack */
    pTd->td_pStackEnd	= pTcb->pStackEnd;		/* end of the stack */

#if (_STACK_DIR==_STACK_GROWS_DOWN)
    if (pTcb->options & VX_NO_STACK_FILL)
	pStackHigh = pTcb->pStackLimit;
    else
	for (pStackHigh = pTcb->pStackLimit;
	     *(UINT8 *)pStackHigh == 0xee; pStackHigh ++)
	    ;
#else 	/* _STACK_GROWS_UP */
    if (pTcb->options & VX_NO_STACK_FILL)
	pStackHigh = pTcb->pStackLimit - 1;
    else
	for (pStackHigh = pTcb->pStackLimit - 1;
	     *(UINT8 *)pStackHigh == 0xee; pStackHigh --)
	    ;
#endif 	/* _STACK_GROWS_UP */


    pTd->td_stackSize   = (int)(pTcb->pStackLimit - pTcb->pStackBase) *
			  _STACK_DIR;
    pTd->td_stackHigh	= (int)(pStackHigh - pTcb->pStackBase) * _STACK_DIR;
    pTd->td_stackMargin	= (int)(pTcb->pStackLimit - pStackHigh) * _STACK_DIR;
    pTd->td_stackCurrent= (int)(pTd->td_sp - pTcb->pStackBase) * _STACK_DIR;

    pTd->td_errorStatus	= errnoOfTaskGet (tid);		/* most recent error */

    /* if task is delayed, get the time to fire out of the task's tick node */

    if (pTcb->status & WIND_DELAY)
	pTd->td_delay = Q_KEY (&tickQHead, &pTcb->tickNode, 1);
    else
	pTd->td_delay = 0;			/* not delayed */

    /* copy the VxWorks events information */

    key = intLock ();
    pTd->td_events = pTcb->events;
    intUnlock (key);

    return (OK);
    }

/*******************************************************************************
*
* taskShow - display task information from TCBs
*
* This routine displays the contents of a task control block (TCB)
* for a specified task.  If <level> is 1, it also displays task options
* and registers.  If <level> is 2, it displays all tasks.
*
* The TCB display contains the following fields:
*
* .TS
* tab(|);
* lf3 lf3
* l l .
* Field  | Meaning
* _
* NAME   | Task name
* ENTRY  | Symbol name or address where task began execution
* TID    | Task ID
* PRI    | Priority
* STATUS | Task status, as formatted by taskStatusString()
* PC     | Program counter
* SP     | Stack pointer
* ERRNO  | Most recent error code for this task
* DELAY  | If task is delayed, number of clock ticks remaining in delay (0 otherwise)
* .TE
*
* EXAMPLE:
* The following example shows the TCB contents for the shell task:
* .CS
*   -> taskShow tShell, 1
* 
*     NAME        ENTRY    TID    PRI  STATUS      PC       SP    ERRNO  DELAY
*   ---------- --------- -------- --- --------- -------- -------- ------ -----
*   tShell     _shell     20efcac   1 READY      201dc90  20ef980      0     0
* 
*   stack: base 0x20efcac  end 0x20ed59c  size 9532   high 1452   margin 8080
* 
*   options: 0x1e
*   VX_UNBREAKABLE      VX_DEALLOC_STACK    VX_FP_TASK         VX_STDIO
*
*   VxWorks Events
*   --------------
*   Events Pended on    : Not Pended
*   Received Events     : 0x0
*   Options             : N/A
* 			
* 
*   D0 =       0   D4 =       0   A0 =       0   A4 =        0
*   D1 =       0   D5 =       0   A1 =       0   A5 =  203a084   SR =     3000
*   D2 =       0   D6 =       0   A2 =       0   A6 =  20ef9a0   PC =  2038614
*   D3 =       0   D7 =       0   A3 =       0   A7 =  20ef980
*   value = 34536868 = 0x20efda4
* .CE
*
* RETURNS: N/A
*
* SEE ALSO:
* taskStatusString(),
* .pG "Target Shell,"
* windsh,
* .tG "Shell"
*/

STATUS taskShow
    (
    int tid,		/* task ID */
    int level		/* 0 = summary, 1 = details, 2 = all tasks */
    )
    {
    FAST int	nTasks;			/* number of task */
    FAST int	ix;			/* index */
    TASK_DESC	td;			/* task descriptor for task info */
    WIND_TCB *	pTcb;			/* pointer to tasks tcb */
    int		idList[MAX_DSP_TASKS];	/* list of active IDs */
    char	optionsString[256];	/* task options string */

    tid = taskIdDefault (tid);				/* get default task */

    switch (level)
	{
	case 0 :					/* summarize a task */
	    {
	    if (taskInfoGet (tid, &td) != OK)
		{
		printErr ("Task not found.\n");
		return (ERROR);
		}

	    printf (infoHdr);
	    taskSummary (&td);
	    break;
	    }

	case 1 :					/* get task detail */
	    {
	    if (taskInfoGet (tid, &td) != OK)
		{
		printErr ("Task not found.\n");
		return (ERROR);
		}

	    taskOptionsString (tid, optionsString);	/* get options string */

	    /* Print the summary as in all_task_info, then all the regs. */

	    printf (infoHdr);				/* banner */
	    taskSummary (&td);

	    printf ("\nstack: base 0x%-6x  end 0x%-6x  size %-5d  ",
		    (int)td.td_pStackBase, (int)td.td_pStackEnd,
		    td.td_stackSize);

	    if (td.td_options & VX_NO_STACK_FILL)
		printf ("high %5s  margin %5s\n", "???", "???");
	    else
		printf ("high %-5d  margin %-5d\n", td.td_stackHigh,
			 td.td_stackMargin);

	    printf ("\noptions: 0x%x\n%s\n", td.td_options, optionsString);

	    /* display VxWorks events information */

	    eventTaskShow (&td.td_events);

	    if (tid != taskIdSelf ())			/* no self exam */
		{
		taskRegsShow (tid);
		if (_func_fppTaskRegsShow != NULL)	/* fp regs if attached*/
		    (* _func_fppTaskRegsShow) (tid);
		if (_func_dspTaskRegsShow != NULL)	/* dsp regs if attached*/
		    (* _func_dspTaskRegsShow) (tid);
#ifdef _WRS_ALTIVEC_SUPPORT
		if (_func_altivecTaskRegsShow != NULL)	/* altivec regs if attached*/
		    (* _func_altivecTaskRegsShow) (tid);
#endif /* _WRS_ALTIVEC_SUPPORT */

#if (CPU_FAMILY==I80X86)
		if (_func_sseTaskRegsShow != NULL)	/* SIMD regs if attached */
		    (* _func_sseTaskRegsShow) (tid);
#endif /* (CPU_FAMILY==I80X86) */
		}

	    /* print exception info if any */

	    if ((_func_excInfoShow != NULL) && ((pTcb = taskTcb (tid)) != NULL))
		(* _func_excInfoShow) (&pTcb->excInfo, FALSE);
	    break;
	    }

	case 2 :				/* summarize all tasks */
	default :
	    {
	    printf (infoHdr);

	    nTasks = taskIdListGet (idList, NELEMENTS (idList));
	    taskIdListSort (idList, nTasks);

	    for (ix = 0; ix < nTasks; ++ix)
		{
		if (taskInfoGet (idList [ix], &td) == OK)
		    taskSummary (&td);
		}
	    break;
	    }
	}

    return (OK);
    }

/*******************************************************************************
*
* taskSummary - print task summary line
*
* This routine is used by i() and ti() to print each task's summary line.
*
* NOMANUAL
*/

LOCAL void taskSummary
    (
    TASK_DESC *pTd		/* task descriptor to summarize */
    )
    {
    REG_SET   regSet;			/* get task's regs into here */
    char      statusString[10];		/* status string goes here   */
    SYMBOL_ID symbolId;                 /* symbol identifier         */
    char *    name;   /* ptr to sym tbl copy of name of main routine */
    void *    value = NULL;		/* symbol's actual value     */

    taskStatusString (pTd->td_id, statusString);

    /* Print the summary of the TCB */

    printf ("%-11.11s", pTd->td_name);	/* print the name of the task */

    /* 
     * Only check one symLib function pointer (for performance's sake). 
     * All symLib functions are provided by the same library, by convention.    
     */

    if ((_func_symFindSymbol != (FUNCPTR) NULL) && 
	(sysSymTbl != NULL) && 
	((* _func_symFindSymbol) (sysSymTbl, NULL, (char *)pTd->td_entry, 
				 N_EXT | N_TEXT, N_EXT | N_TEXT,
				 &symbolId) == OK))
	{
	(* _func_symNameGet) (symbolId, &name);
	(* _func_symValueGet) (symbolId, &value);
	}

    if (pTd->td_entry == (FUNCPTR) value)
	printf ("%-12.12s", name);
    else
	printf ("%-12x", (int)pTd->td_entry);

    /* get task's registers;  if the tcb being printed is the
     * calling task's tcb, then taskRegsGet will return garbage for pc,
     * so we fudge it a little so it won't look bad.
     */

    taskRegsGet (pTd->td_id, &regSet);

    printf (" %8x %3d %-10.10s %8x %8x %7x %5u\n",
	    pTd->td_id,
	    pTd->td_priority,
	    statusString,
	    ((taskIdSelf () == pTd->td_id) ? (int)taskSummary : (int)regSet.pc),

	    (int)regSet.spReg,
	    pTd->td_errorStatus,
	    pTd->td_delay);
    }

/******************************************************************************
*
* taskIdListSort - sort the ID list by priority
*
* This routine sorts the <idList> by task priority.
*
* NOMANUAL
*/

void taskIdListSort
    (
    int idList[],		/* id list to sort */
    int nTasks			/* number of tasks in id list */
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

/*******************************************************************************
*
* taskRegsShow - display the contents of a task's registers
*
* This routine displays the register contents of a specified task
* on standard output.
*
* EXAMPLE: The following example displays the register of the shell task 
* (68000 family):
* .CS 4
* -> taskRegsShow (taskNameToId ("tShell"))
*
* d0     =        0   d1     =        0    d2    =    578fe    d3     =        1
* d4     =   3e84e1   d5     =   3e8568    d6    =        0    d7     = ffffffff
* a0     =        0   a1     =        0    a2    =    4f06c    a3     =    578d0
* a4     =   3fffc4   a5     =        0    fp    =   3e844c    sp     =   3e842c
* sr     =     3000   pc     =    4f0f2
* value = 0 = 0x0
* .CE
*
* RETURNS: N/A
*/

void taskRegsShow
    (
    int tid             /* task ID */
    )
    {
#if ((CPU_FAMILY != MIPS) && (CPU_FAMILY != COLDFIRE))
    int		ix;
    int *	pReg;		/* points to register value */
#endif /* (CPU_FAMILY != MIPS) && (CPU_FAMILY != COLDFIRE) */
    REG_SET	regSet;		/* register set */

    if (_func_taskRegsShowRtn != NULL)
        {
        (_func_taskRegsShowRtn) (tid);
        return;
        }

    if (taskRegsGet (tid, &regSet) == ERROR)
	{
	printf ("taskRegsShow: invalid task id %#x\n", tid);
	return;
	}

#if (CPU_FAMILY==MIPS || CPU_FAMILY==COLDFIRE)
    taskArchRegsShow (&regSet);
#else
    /* print out registers */

    for (ix = 0; taskRegName[ix].regName != NULL; ix++)
	{
	if ((ix % 4) == 0)
	    printf ("\n");
	else
	    printf ("%3s","");

	if (taskRegName[ix].regName[0] != EOS)
	    {
	    pReg = (int *) ((int)&regSet + taskRegName[ix].regOff);
	    printf (taskRegsFmt, taskRegName[ix].regName, *pReg);
	    }
	else
	    printf ("%17s", "");
	}
    printf ("\n");
#endif
    }

/*******************************************************************************
*
* taskStatusString - get a task's status as a string
*
* This routine deciphers the WIND task status word in the TCB for a
* specified task, and copies the appropriate string to <pString>.
* 
* The formatted string is one of the following:
*
* .TS
* tab(|);
* lf3 lf3
* l l .
* String   | Meaning
* _
* READY    | Task is not waiting for any resource other than the CPU.
* PEND     | Task is blocked due to the unavailability of some resource.
* DELAY    | Task is asleep for some duration.
* SUSPEND  | Task is unavailable for execution (but not suspended, delayed, or pended).
* DELAY+S  | Task is both delayed and suspended.
* PEND+S   | Task is both pended and suspended.
* PEND+T   | Task is pended with a timeout.
* PEND+S+T | Task is pended with a timeout, and also suspended.
* \&...+I  | Task has inherited priority (+I may be appended to any string above).
* DEAD     | Task no longer exists.
* .TE
*
* EXAMPLE
* .CS
*     -> taskStatusString (taskNameToId ("tShell"), xx=malloc (10))
*     new symbol "xx" added to symbol table.
*     value = 0 = 0x0
*     -> printf ("shell status = <%s>\en", xx)
*     shell status = <READY>
*     value = 2 = 0x2
* .CE
*
* RETURNS: OK, or ERROR if the task ID is invalid.
*/

STATUS taskStatusString
    (
    int  tid,           /* task to get string for */
    char *pString       /* where to return string */
    )
    {
    WIND_TCB *pTcb = taskTcb (tid);

    if (pTcb == NULL)
	return (ERROR);

    switch (pTcb->status)
	{
	case WIND_READY:	strcpy (pString, "READY");  break;

	case WIND_DELAY:	strcpy (pString, "DELAY");  break;

	case WIND_DELAY |
	     WIND_SUSPEND:	strcpy (pString, "DELAY+S");  break;

	case WIND_PEND:		strcpy (pString, "PEND");   break;

	case WIND_PEND |
	     WIND_DELAY:	strcpy (pString, "PEND+T");   break;

	case WIND_PEND |
	     WIND_SUSPEND:	strcpy (pString, "PEND+S");   break;

	case WIND_PEND |
	     WIND_DELAY |
	     WIND_SUSPEND:	strcpy (pString, "PEND+S+T");   break;

	case WIND_SUSPEND:	strcpy (pString, "SUSPEND");  break;

	case WIND_DEAD:		strcpy (pString, "DEAD");   break;

	default:			/* unanticipated combination */
	    sprintf (pString, "0x%02x", pTcb->status);
	    return (ERROR);
	}

    if (pTcb->priority != pTcb->priNormal)
	strcat (pString, "+I");		/* task's priority inherited */

    return (OK);
    }

/*******************************************************************************
*
* taskOptionsString - get a task's options as a string
*
* This routine deciphers the WIND task options field in the TCB, for a
* specified task, and copies the appropriate string to <pString>.
*
* RETURNS: OK, or ERROR if the task ID is invalid.
*
* NOMANUAL
*/

STATUS taskOptionsString
    (
    int  tid,           /* task to get options string for */
    char *pString       /* where to return string of options */
    )
    {
    WIND_TCB *pTcb = taskTcb (tid);

    if (pTcb == NULL)
	return (ERROR);

    pString[0] = EOS;				/* null terminate string */

    if (pTcb->options & VX_SUPERVISOR_MODE)
	strcat (pString, "VX_SUPERVISOR_MODE  ");

    if (pTcb->options & VX_UNBREAKABLE)
	strcat (pString, "VX_UNBREAKABLE      ");

    if (pTcb->options & VX_DEALLOC_STACK)
	strcat (pString, "VX_DEALLOC_STACK    ");

    if (pTcb->options & VX_FP_TASK)
	strcat (pString, "VX_FP_TASK          ");

    if (pTcb->options & VX_DSP_TASK)
	strcat (pString, "VX_DSP_TASK         ");

#ifdef _WRS_ALTIVEC_SUPPORT
    if (pTcb->options & VX_ALTIVEC_TASK)
        strcat (pString, "VX_ALTIVEC_TASK     ");
#endif /* _WRS_ALTIVEC_SUPPORT */

    if (pTcb->options & VX_STDIO)
	strcat (pString, "VX_STDIO            ");

    if (pTcb->options & VX_ADA_DEBUG)
	strcat (pString, "VX_ADA_DEBUG        ");

    if (pTcb->options & VX_FORTRAN)
	strcat (pString, "VX_FORTRAN          ");

    if (pTcb->options & VX_PRIVATE_ENV)
	strcat (pString, "VX_PRIVATE_ENV      ");

    if (pTcb->options & VX_NO_STACK_FILL)
	strcat (pString, "VX_NO_STACK_FILL    ");

    return (OK);
    }

