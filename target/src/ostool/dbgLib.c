/* dbgLib.c - debugging facilities */

/* Copyright 1984-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01y,26mar02,pai  Added code to display Streaming SIMD Registers (SPR 74103).
01x,09nov01,jhw  Revert WDB_INFO to be inside of WIND_TCB.
01w,05nov01,jn   use symFindSymbol for symbol lookup (SPR #7453)
01v,17oct01,jhw  Access WDB_INFO from WIND_TCB pointer pWdbInfo.
01u,03apr01,kab  Added _WRS_ALTIVEC_SUPPORT
01t,14mar01,pcs  Added code to display ALTIVEC Registers.
01s,08nov00,zl   added dsp show routine call.
01r,03mar00,zl   merged SH support from T1
01q,15mar99,elg  change documentation of tt() (SPR 20892).
01p,14mar99,jdi  doc: removed refs to config.h and/or configAll.h (SPR 25663).
01o,18jan99,elg  Authorize breakpoints in branch delay slot (SPR 24356).
01n,31aug98,ms   dbgTlSnglStep only conditionally prints fpp regs (SPR 9198).
01m,23apr98,dbt  code cleanup
01l,16mar98,dbt  documentation and code cleanup.
01k,19nov97,dbt  modified to merge with WDB debugger
10a,14jul97,dgp  doc: add windsh x-ref to cret() & so(), change ^C to CTRL-C
09z,21feb97,tam  moved _dbgDsmInstRtn declaration to funcBind.c
09z,28nov96,cdp  added ARM support (make things line up in dbgPrintCall).
09y,23jan96,tpr  turned off the Trace capability for the PPC403ga.
09x,06oct95,jdi  changed Debugging .pG's to .tG "Shell".
09w,31aug95,jdi  doc tweak to b(); changed <passcount> to <count> -- SPR 4603.
09v,20mar95,jdi  doc tweak to l().
09u,01feb95,jdi  added special synopsis to bh() for i386/i486.
09u,01feb95,rhp  doc: simplify integration with standard VxWorks docn by
                 limiting impact of eventpoints to EVENTPOINTS para and 
                 e() routine; also propagate recent doc changes to 
                 standard bh() man page.
...deleted pre 95 history
*/

/*
DESCRIPTION
This library contains VxWorks's primary interactive debugging routines, which 
provide the following facilities:

    - task breakpoints
    - task single-stepping
    - symbolic disassembly
    - symbolic task stack tracing

In addition, dbgLib provides the facilities necessary for enhanced use
of other VxWorks functions, including:

    - enhanced shell abort and exception handling (via tyLib and excLib)

The facilities of excLib are used by dbgLib to support breakpoints,
single-stepping, and additional exception handling functions.

INITIALIZATION
The debugging facilities provided by this module are optional.  In the
standard VxWorks development configuration as distributed, the debugging
package is included.  The configuration macro is INCLUDE_DEBUG.
When defined, it enables the call to dbgInit() in the task usrRoot()
in usrConfig.c.  The dbgInit() routine initializes dbgLib and must be made
before any other routines in the module are called.

BREAKPOINTS
Use the routine b() or bh() to set breakpoints.  Breakpoints can be set to 
be hit by a specific task or all tasks.  Multiple breakpoints for different 
tasks can be set at the same address.  Clear breakpoints with bd() and bdall().

When a task hits a breakpoint, the task is suspended and a message is
displayed on the console.  At this point, the task can be examined,
traced, deleted, its variables changed, etc.  If you examine the task at
this point (using the i() routine), you will see that it is in a suspended
state.  The instruction at the breakpoint address has not yet been executed.

To continue executing the task, use the c() routine.  The breakpoint
remains until it is explicitly removed.

EVENTPOINTS (WINDVIEW)
When WindView is installed, dbgLib supports eventpoints.  Use the routine
e() to set eventpoints.  Eventpoints can be set to be hit by a specific
task or all tasks.  Multiple eventpoints for different tasks can be set at
the same address.

When a task hits an eventpoint, an event is logged and is displayed by
VxWorks kernel instrumentation.

You can manage eventpoints with the same facilities that manage
breakpoints:  for example, unbreakable tasks (discussed below) ignore
eventpoints, and the b() command (without arguments) displays
eventpoints as well as breakpoints.  As with breakpoints, you can clear
eventpoints with bd() and bdall().

UNBREAKABLE TASKS
An \f2unbreakable\fP task ignores all breakpoints.  Tasks can be spawned
unbreakable by specifying the task option VX_UNBREAKABLE.  Tasks can
subsequently be set unbreakable or breakable by resetting VX_UNBREAKABLE
with taskOptionsSet().  Several VxWorks tasks are spawned unbreakable,
such as the shell, the exception support task excTask(), and several
network-related tasks.

DISASSEMBLER AND STACK TRACER
The l() routine provides a symbolic disassembler.  The tt() routine
provides a symbolic stack tracer.

SHELL ABORT AND EXCEPTION HANDLING
This package includes enhanced support for the shell in a debugging
environment.  The terminal abort function, which restarts the shell, is
invoked with the abort key if the OPT_ABORT option has been set.  By
default, the abort key is CTRL-C.  For more information, see the manual
entries for tyAbortSet() and tyAbortFuncSet().

THE DEFAULT TASK AND TASK REFERENCING
Many routines in this module take an optional task name or ID as an
argument.  If this argument is omitted or zero, the "current" task is
used.  The current task (or "default" task) is the last task referenced.
The dbgLib library uses taskIdDefault() to set and get the last-referenced
task ID, as do many other VxWorks routines.

All VxWorks shell expressions can reference a task by either ID or name.
The shell attempts to resolve a task argument to a task ID; if no match is
found in the system symbol table, it searches for the argument in the list
of active tasks.  When it finds a match, it substitutes the task name with
its matching task ID.  In symbol lookup, symbol names take precedence over
task names.

INTERNAL
Architecture:
The VxWorks native debugger is structured to be portable across different
architectures. Six files make up the debugger:  dbgLib.c, dbgTaskLib.c,
wdbDbgLib.c, dbgArchLib.c, wdbDbgALib.s and wdbDbgArchLib.c. The three
first files contain the architecture-independent functions, the others
contains the architecture-dependent support functions.

CAVEAT
When a task is continued, c() and s() routines do not yet distinguish
between a suspended task or a task suspended by the debugger.  Therefore, 
use of these routines should be restricted to only those tasks being
debugged.

INCLUDE FILES: dbgLib.h

SEE ALSO
excLib, tyLib, taskIdDefault(), taskOptionsSet(), tyAbortSet(),
tyAbortFuncSet(),
.pG "Target Shell,"
windsh,
.tG "Shell"
*/

/* LINTLIBRARY */

#include "vxWorks.h"

#include "iv.h"
#include "esf.h"
#include "intLib.h"
#include "ioLib.h"
#include "memLib.h"
#include "dbgLib.h"
#include "dsmLib.h"
#include "regs.h"
#include "shellLib.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "trcLib.h"
#include "tyLib.h"
#include "usrLib.h"
#include "vxLib.h"
#include "private/cplusLibP.h"
#include "private/windLibP.h"
#include "private/taskLibP.h"
#include "private/funcBindP.h"
#include "private/dbgLibP.h"
#include "wdb/wdbDbgLib.h"

/* defines */

#define DBG_INFO(p)      (&(((WIND_TCB *)(p))->wdbInfo))

#define DBG_DEMANGLE_PRINT_LEN 256  /* Num chars of demangled names to print */

/* error messages */

#define DBG_TASK_NOT_FOUND	"Task not found\n"
#define DBG_ILL_BKPT_ADDR	"Illegal Breakpoint Address: 0x%08x\n"
#define DBG_BKPT_TBL_FULL	"Breakpoint table is full\n"
#define DBG_ILL_EVTPT_ADDR	"Illegal Eventpoint Address: 0x%08x\n"
#define DBG_EVTPT_TBL_FULL	"Eventpoint table is full\n"
#define DBG_HW_BKPT_REGS_FULL	"Cannot set hardware breakpoint. All hardware breakpoint registers are in use\n"
#define DBG_INV_HW_BKPT_TYPE	"Invalid hardware breakpoint type\n"
#define DBG_NO_BKPT_AT_ADDR	"No breakpoint at 0x%08x"
#define DBG_TASK_IX_NOT_FOUND	"Task %#x not found\n"
#define DBG_CRET_NOT_SUPP	"cret not supported\n"
#define DBG_NO_BKPTS		"No breakpoints\n"

/* typedefs */

typedef struct                  /* EVT_CALL_ARGS */
    {
    FUNCPTR	evtRtn;		/* event routine */
    int 	evtRtnArg;	/* event routine argument */
    event_t	eventId;	/* event ID */
    } EVT_CALL_ARGS;

/* globals */

BOOL		dbgPrintFpp;
BOOL		dbgPrintDsp;
#ifdef _WRS_ALTIVEC_SUPPORT
BOOL            dbgPrintAltivec;
#endif /* _WRS_ALTIVEC_SUPPORT */
BOOL            dbgPrintSimd;

/* extern */

IMPORT FUNCPTR	_func_breakpoint;
IMPORT FUNCPTR	_func_trace;
IMPORT FUNCPTR	_func_trap;
IMPORT FUNCPTR	_func_dbgTargetNotify;
IMPORT BOOL	dbgUnbreakableOld;

/* locals */

static INSTR *	nextToDsm	= NULL;	/* instruction to disassemble from */
static int	dfltCount	= 10;	/* # of instructions to disassemble */

/* forward declarations */

LOCAL void 	dbgTargetNotify (UINT32 flag, UINT32 addr, 
				    REG_SET * pRegisters);
LOCAL BRKPT *	dbgBrkAdd (INSTR * addr, int task, unsigned flags, 
				    unsigned action, unsigned count, 
				    FUNCPTR callRtn, int callArg);
LOCAL STATUS	dbgCheckBreakable (int task);
LOCAL void	dbgTlSnglStep (int task);
LOCAL STATUS	dbgInstall (void);
LOCAL void	dbgTyAbort (void);
LOCAL void	dbgTlTyAbort (void);
LOCAL void 	dbgBrkDisplay (void);
LOCAL void	dbgPrintAdrs (int address);
LOCAL INSTR * 	dbgList (INSTR * addr, int count);
LOCAL void 	dbgBrkPrint (INSTR * addr);
LOCAL void	dbgE (EVT_CALL_ARGS * pEvtCall, REG_SET * pRegisters);

/*******************************************************************************
*
* dbgHelp - display debugging help menu
*
* This routine displays a summary of dbgLib utilities with a
* short description of each, similar to the following:
*
* .CS
*     dbgHelp                         Print this list
*     dbgInit                         Install debug facilities
*     b                               Display breakpoints
*     b         addr[,task[,count]]   Set breakpoint
*     e         addr[,eventNo[,task[,func[,arg]]]]] Set eventpoint (WindView)
*     bd        addr[,task]           Delete breakpoint
*     bdall     [task]                Delete all breakpoints
*     c         [task[,addr[,addr1]]] Continue from breakpoint
*     cret      [task]                Continue to subroutine return
*     s         [task[,addr[,addr1]]] Single step
*     so        [task]                Single step/step over subroutine
*     l         [adr[,nInst]]         List disassembled memory
*     tt        [task]                Do stack trace on task
*     bh        addr[,access[,task[,count[,quiet]]]] set hardware breakpoint
*                                     (if supported by the architecture)
* .CE
*
* RETURNS: N/A
*
* SEE ALSO:
* .pG "Target Shell"
*/

void dbgHelp (void)
    {
    static char * help_msg  = "\n\
dbgHelp                         Print this list\n\
dbgInit                         Install debug facilities\n\
b                               Display breakpoints and eventpoints\n\
b         addr[,task[,count]]   Set breakpoint\n\
e         addr[,eventNo[,task[,func[,arg]]]]] Set eventpoint\n\
bd        addr[,task]           Delete breakpoint\n\
bdall     [task]                Delete all breakpoints and eventpoints\n\
c         [task[,addr[,addr1]]] Continue from breakpoint\n\
cret      [task]                Continue to subroutine return\n\
s         [task[,addr[,addr1]]] Single step\n\
so        [task]                Single step/step over subroutine\n\
l         [adr[,nInst]]         List disassembled memory\n\
tt        [task]                Do stack trace on task\n";

    printf ("%s", help_msg);

    printf ("%s", _archHelp_msg);
    }

/*******************************************************************************
*
* dbgInit - initialize the local debugging package
*
* This routine initializes the local debugging package and enables the basic
* breakpoint and single-step functions.
*
* This routine also enables the shell abort function, CTRL-C.
*
* NOTE
* The debugging package should be initialized before any debugging routines
* are used.  If the configuration macro INCLUDE_DEBUG is defined, dbgInit()
* is called by the root task, usrRoot(), in usrConfig.c.
*
* RETURNS: OK, always.
*
* SEE ALSO:
* .pG "Target Shell"
*/

STATUS dbgInit (void)
    {
    _dbgArchInit ();	/* initialise disassembler ... */

    tyAbortFuncSet ((FUNCPTR) dbgTyAbort);	/* set abort routine */

    _func_bdall = bdall;

    return (OK);
    }

/*******************************************************************************
*
* dbgInstall - install the debugging facility
*
* This routine installs the debug package.
* It performs the following actions:
*
*   - Initialize breakpoint entry lists
*
*   - Sets the interrupt vectors of the breakpoint
*     and trace hardware interrupts to the appropriate
*     debug interrupt handlers.
*
*   - Adds the debug task switch routine to the kernel
*     context switch call out.
*
* This routine is called automatically by all breakpoint related routines
* in this library.  Note that dbgInstall should not be called until
* the following items have been initialized:
*
*   - the pipe driver
*   - the ty driver used for logging
*   - the exception handler library (excLib)
*   - the message logging library (logLib)
*
* RETURNS: OK, or ERROR if unable to install task switch routine.
*
* NOMANUAL
*/

LOCAL STATUS dbgInstall (void)
    {
    static BOOL dbgInstalled = FALSE;	   /* TRUE = facility installed */

    /* if debugging facility is not already installed, then install it
     * by initializing lists and installing switch hook.
     */

    if (!dbgInstalled)
	{
	/* initialize breakpoint lists */

	wdbDbgBpListInit ();

	/* connect various handlers */

	dbgTaskBpHooksInstall ();

	/* routine to be called when a breakpoint is encountered */

	_func_dbgTargetNotify = (FUNCPTR) dbgTargetNotify;

#if DBG_NO_SINGLE_STEP
	/* 
	 * If _func_trap was not already initialized by the WDB agent, 
	 * initialize it.
	 */

	if (_func_trap == NULL)
	    _func_trap = (FUNCPTR) dbgTaskBpTrap;
#else	/* DBG_NO_SINGLE_STEP */ 
	/*
	 * If _func_trace was not already initialized by the WDB agent,
	 * initialize it.
	 */

	if (_func_trace == NULL)
	    _func_trace = (FUNCPTR) dbgTaskBpTrace;

	/* 
	 * if _func_breakpoint was not already initialized by the WDB agent, 
	 * initialize it. 
	 */

	if (_func_breakpoint == NULL)
	    _func_breakpoint = (FUNCPTR) dbgTaskBpBreakpoint;
#endif	/* DBG_NO_SINGLE_STEP */

	/* Insert the break and trace vectors, set breakpoint instruction */

	wdbDbgArchInit ();

	dbgInstalled = TRUE;
	}

    return (OK);
    }

/*******************************************************************************
*
* b - set or display breakpoints
*
* This routine sets or displays breakpoints.  To display the list of
* currently active breakpoints, call b() without arguments:
* .CS
*     -> b
* .CE
* The list shows the address, task, and pass count of each breakpoint.
* Temporary breakpoints inserted by so() and cret() are also indicated.
*
* To set a breakpoint with b(), include the address, which can be
* specified numerically or symbolically with an optional offset.
* The other arguments are optional:
* .CS
*     -> b addr[,task[,count[,quiet]]]
* .CE
* If <task> is zero or omitted, the breakpoint will apply to all breakable
* tasks.  If <count> is zero or omitted, the breakpoint will occur every
* time it is hit.  If <count> is specified, the break will not occur
* until the <count> +1th time an eligible task hits the breakpoint
* (i.e., the breakpoint is ignored the first <count> times it is hit).
*
* If <quiet> is specified, debugging information destined for the console
* will be suppressed when the breakpoint is hit.  This option is included
* for use by external source code debuggers that handle the breakpoint
* user interface themselves.
*
* Individual tasks can be unbreakable, in which case breakpoints that
* otherwise would apply to a task are ignored.  Tasks can be spawned
* unbreakable by specifying the task option VX_UNBREAKABLE.
* Tasks can also be set unbreakable or breakable by resetting
* VX_UNBREAKABLE with the routine taskOptionsSet().
*
* RETURNS
* OK, or ERROR if <addr> is illegal or the breakpoint table is full.
*
* SEE ALSO: bd(), taskOptionsSet(),
* .pG "Target Shell,"
* windsh,
* .tG "Shell"
*/

STATUS b
    (
    INSTR *	addr,		/* where to set breakpoint, or	 */
				/* 0 = display all breakpoints	 */
    int	 	task,		/* task for which to set breakboint,   */
				/* 0 = set all tasks		   */
    int		count,		/* number of passes before hit	 */
    BOOL	quiet		/* TRUE = don't print debugging info,  */
				/* FALSE = print debugging info	*/
    )
    {
    int		tid = 0;	/* task ID */
    int		val;		/* temporary buffer */
    int		action;		/* breakpoint action */
    int 	retVal = OK;	/* return value */

    if (dbgInstall () != OK)
	return (ERROR);

    if (task != 0)
	tid = taskIdFigure (task);	/* convert to task id */

    if (tid == ERROR)
	{
	printErr (DBG_TASK_NOT_FOUND);
	return (ERROR);
	}

    /* NULL address means show all breakpoints */

    if (addr == NULL) 
	{
	dbgBrkDisplay ();
	return (OK);
	}

    /* if text segment protection turned on, first write enable the page */

    TEXT_UNLOCK(addr);

    /* check validity of breakpoint address */

    if (!ALIGNED ((int) addr, DBG_INST_ALIGN) ||
	(vxMemProbe ((char *)addr, VX_READ,
				sizeof(INSTR), (char *)&val) != OK) ||
	(vxMemProbe ((char *)addr, VX_WRITE,
				sizeof(INSTR), (char *)&val) != OK))
	{
	printErr (DBG_ILL_BKPT_ADDR, (int)addr);
	retVal = ERROR;
	goto done;
	}

    if (quiet)
	action = WDB_ACTION_STOP;
    else
	action = WDB_ACTION_STOP | WDB_ACTION_NOTIFY;

    /* add new breakpoint */

    if (dbgBrkAdd (addr, tid, 0, action, count, NULL, 0) == NULL)
	{
	printErr (DBG_BKPT_TBL_FULL);
	retVal = ERROR;
	goto done;
	}
done:
    TEXT_LOCK(addr);
    return (retVal);
    }

/******************************************************************************
*
* e - set or display eventpoints (WindView)
*
* This routine sets "eventpoints"--that is, breakpoint-like instrumentation
* markers that can be inserted in code to generate and log an event for use
* with WindView.  Event logging must be enabled with wvEvtLogEnable() for
* the eventpoint to be logged.
*
* <eventId> selects the evenpoint number that will be logged: it is in 
* the user event ID range (0-25536).
*
* If <addr> is NULL, then all eventpoints and breakpoints are displayed.
* If <taskNameOrId> is 0, then this event is logged in all tasks.
* The <evtRtn> routine is called when this eventpoint is hit.  If <evtRtn>
* returns OK, then the eventpoint is logged; otherwise, it is ignored.
* If <evtRtn> is a NULL pointer, then the eventpoint is always logged.
*
* Eventpoints are exactly like breakpoints (which are set with the b()
* command) except in how the system responds when the eventpoint is hit.  An
* eventpoint typically records an event and continues immediately (if
* <evtRtn> is supplied, this behavior may be different).  Eventpoints cannot
* be used at interrupt level.
*
* To delete an eventpoint, use bd().
*
* RETURNS
* OK, or ERROR if <addr> is odd or nonexistent in memory, or if the
* breakpoint table is full.
*
* SEE ALSO: wvEvent()
*/

STATUS e
    (
    INSTR *	addr,		/* where to set eventpoint, or		*/
				/* 0 means display all eventpoints	*/
    event_t	eventId,	/* event ID				*/
    int		taskNameOrId,	/* task affected; 0 means all tasks	*/
    FUNCPTR	evtRtn,		/* function to be invoked;		*/
				/* NULL means no function is invoked	*/
    int		arg		/* argument to be passed to <evtRtn>	*/
    )
    {
    int			tid = 0;	/* task ID */
    int			val;		/* temporary buffer */
    int 		retVal = OK;	/* return value */
    EVT_CALL_ARGS * 	pEvtCall;	/* structure to pass arguments */

    if (dbgInstall () != OK)
	return (ERROR);

    if (taskNameOrId != 0)
	tid = taskIdFigure (taskNameOrId);	/* convert to task id */

    if (tid == ERROR)
	{
	printErr (DBG_TASK_NOT_FOUND);
	return (ERROR);
	}

    /* NULL address means show all breakpoints */

    if (addr == NULL) 
	{
	dbgBrkDisplay ();
	return (OK);
	}

    /* if text segment protection turned on, first write enable the page */

    TEXT_UNLOCK(addr);

    /* check validity of breakpoint address */

    if (!ALIGNED ((int) addr, DBG_INST_ALIGN) ||
	(vxMemProbe ((char *)addr, VX_READ,
				sizeof(INSTR), (char *)&val) != OK) ||
	(vxMemProbe ((char *)addr, VX_WRITE,
				sizeof(INSTR), (char *)&val) != OK))
	{
	printErr (DBG_ILL_EVTPT_ADDR, (int)addr);
	retVal = ERROR;
	goto done;
	}

    /*
     * The runtime callout, dbgE, requires three parameters (an event number,
     * and a conditioning routine and argument). However the breakpoints
     * only support passing one argument to the callout. So here we allocate
     * a structure and pass it's address to the callout.
     * XXX - this creates a memory leak of 12 bytes.
     * We need to store the address of the structure with the eventpoint,
     * and have the breakpoint delete routines check and free the memory.
     */

    if ((pEvtCall = (EVT_CALL_ARGS *) malloc (sizeof (EVT_CALL_ARGS))) == NULL)
	{
	retVal = ERROR;
	goto done;
	}

    pEvtCall->evtRtn = evtRtn;
    pEvtCall->evtRtnArg = arg;
    pEvtCall->eventId = eventId;

    /* add new breakpoint */

    if (dbgBrkAdd (addr, tid, BP_EVENT, WDB_ACTION_CALL, 0, (FUNCPTR) dbgE, 
			(int) pEvtCall) == NULL)
	{
	printErr (DBG_EVTPT_TBL_FULL);
	retVal = ERROR;
	goto done;
	}

done:
    TEXT_LOCK(addr);
    return (retVal);
    }

#if	DBG_HARDWARE_BP
/******************************************************************************
*
* bh - set a hardware breakpoint
*
* This routine is used to set a hardware breakpoint.   If the architecture
* allows it, this function will add the breakpoint to the list of breakpoints
* and set the hardware breakpoint register(s).  For more information, see the
* manual entry for b().
*
* NOTE
* The types of hardware breakpoints vary with the architectures.  Generally,
* a hardware breakpoint can be a data breakpoint or an instruction breakpoint.
*
* RETURNS
* OK, or ERROR if <addr> is illegal or the hardware breakpoint table is
* full.
*
* SEE ALSO: b(),
* .pG "Target Shell"
*/

STATUS bh
    (
    INSTR *	addr,		/* where to set breakpoint, or */
				/* 0 = display all breakpoints */
    int		access,		/* access type (arch dependant) */
    int		task,		/* task for which to set breakboint,	*/
				/* 0 = set all tasks			*/
    int		count,		/* number of passes before hit		*/
    BOOL	quiet		/* TRUE = don't print debugging info,  */
				/* FALSE = print debugging info	*/
    )
    {
    int		tid = 0;
    BRKPT *	pBp;
    DBG_REGS	dbgRegs;
    dll_t *	pDll;
    int		action;
    int		status;

    if (dbgInstall () != OK)
	return (ERROR);

    if (task != 0)
	tid = taskIdFigure (task);	/* convert to task id */

    if (tid == ERROR)
	{
	printErr (DBG_TASK_NOT_FOUND);
	return (ERROR);
	}

    /* 0 address means show all breakpoints */

    if (addr == NULL) 
	{
	dbgBrkDisplay ();
	return (OK);
	}

    /* check validity of hardware breakpoint address */

    if (wdbDbgHwAddrCheck ((UINT32) addr, access, (FUNCPTR) vxMemProbe) != OK)
	{
	printErr (DBG_ILL_BKPT_ADDR, (int)addr);
	return (ERROR);
	}

    /* clean dbgRegs structure */

    memset (&dbgRegs, 0, sizeof (DBG_REGS));

    /* fill dbgRegs structure */
   
    taskLock ();	/* LOCK PREEMPTION */

    for (pDll = dll_head(&bpList); pDll != dll_end(&bpList);
		pDll = dll_next(pDll))
	{
	pBp = BP_BASE(pDll);

	/* check if found breakpoint is applicable to current task */

	if ((pBp->bp_task == ALL) || (task == ALL) || (pBp->bp_task == task))
	    {
	    if (pBp->bp_flags & BRK_HARDWARE)
		{
		if (wdbDbgHwBpSet (&dbgRegs, pBp->bp_flags & BRK_HARDMASK, 
					(UINT32) pBp->bp_addr) != OK)
		    {
		    printErr (DBG_HW_BKPT_REGS_FULL);
		    taskUnlock ();	/* UNLOCK PREEMPTION */
		    return (ERROR);
		    }
		}
	    }
	}

    taskUnlock ();	/* UNLOCK PREEMPTION */

    if (quiet)
	action = WDB_ACTION_STOP;
    else
	action = WDB_ACTION_STOP | WDB_ACTION_NOTIFY;

    status = wdbDbgHwBpSet (&dbgRegs, access, (UINT32) addr);

    switch (status)
	{
	case WDB_ERR_HW_REGS_EXHAUSTED:
	    printErr (DBG_HW_BKPT_REGS_FULL);
	    return (ERROR);
	case WDB_ERR_INVALID_HW_BP:
	    printErr (DBG_INV_HW_BKPT_TYPE);
	    return (ERROR);
	case OK:
	    break;
	default:
	    return (ERROR);
	}

    /* add breakpoint to breakpoint table */

    if ((pBp = dbgBrkAdd (addr, tid, access | BRK_HARDWARE, action, 
						count, NULL, 0)) == NULL)
	{
	printErr (DBG_BKPT_TBL_FULL);
	return (ERROR);
	}

    return (OK);
    }
#endif	/* DBG_HARDWARE_BP */

/*******************************************************************************
*
* bd - delete a breakpoint
*
* This routine deletes a specified breakpoint.
*
* To execute, enter:
* .CS
*     -> bd addr [,task]
* .CE
* If <task> is omitted or zero, the breakpoint will be removed for all tasks.
* If the breakpoint applies to all tasks, removing it for only a single
* task will be ineffective.  It must be removed for all tasks and then set
* for just those tasks desired.  Temporary breakpoints inserted by the
* routines so() or cret() can also be deleted.
*
* RETURNS
* OK, or ERROR if there is no breakpoint at the specified address.
*
* SEE ALSO: b(),
* .pG "Target Shell,"
* windsh,
* .tG "Shell"
*/

STATUS bd
    (
    INSTR *	addr,		/* address of breakpoint to delete      */
    int		task		/* task for which to delete breakpoint, */
				/* 0 = delete for all tasks	     */
    )
    {
    BRKPT *		pBp;
    int 		tid = 0;
    dll_t *		pDll;
    dll_t *		pDllNext;

    if (dbgInstall () != OK)
	return (ERROR);

    if (task != 0)
	tid = taskIdFigure (task);	/* convert to task id */

    if (tid == ERROR)
	{
	printErr (DBG_TASK_NOT_FOUND);
	return (ERROR);
	}

    /* search breakpoint table for entry with correct address & task */

    taskLock ();	/* LOCK PREEMPTION */

    for (pDll = dll_head(&bpList); pDll != dll_end(&bpList); pDll = pDllNext)
	{
	pDllNext = dll_next(pDll);

	pBp = BP_BASE(pDll);

	if (pBp->bp_addr == addr && 
		(pBp->bp_task == tid || tid == ALL) &&
		!(pBp->bp_flags & BP_HOST))
	    {
	    wdbDbgBpRemove (pBp);
	    taskUnlock ();	/* UNLOCK PREEMPTION */
	    return (OK);
	    }
	}

    taskUnlock ();	/* UNLOCK PREEMPTION */

    /*
     * print an error message if we have not found the breakpoint to remove
     */

    printErr (DBG_NO_BKPT_AT_ADDR, (int) addr);

    if (tid == ALL)
	printErr ("\n");
    else
	printErr (" for task %#x (%s)\n", tid, taskName (tid));

    return (ERROR);
    }

/*******************************************************************************
*
* bdall - delete all breakpoints
*
* This routine removes all breakpoints.
*
* To execute, enter:
* .CS
*     -> bdall [task]
* .CE
* If <task> is specified, all breakpoints that apply to that task are
* removed.  If <task> is omitted,  all breakpoints for all tasks are
* removed.  Temporary breakpoints inserted by so() or cret() are not
* deleted; use bd() instead.
*
* RETURNS: OK, always.
*
* SEE ALSO: bd(),
* .pG "Target Shell,"
* windsh,
* .tG "Shell"
*/

STATUS bdall
    (
    int		task		/* task for which to delete breakpoints, */
				/* 0 = delete for all tasks	      */
    )
    {
    int 	tid = 0;
    dll_t *	pDll;
    dll_t *	pDllNext;
    BRKPT *	pBp;

    if (dbgInstall () != OK)
	return (ERROR);

    if (task != 0)
	tid = taskIdFigure (task);	/* convert to task id */

    if (tid == ERROR)
	{
	printErr (DBG_TASK_NOT_FOUND);
	return (ERROR);
	}

    taskLock ();	/* LOCK PREEMPTION */

    for (pDll = dll_head(&bpList); pDll != dll_end(&bpList); pDll = pDllNext)
	{
	pDllNext = dll_next(pDll);
	pBp = BP_BASE(pDll);

	/* we can't remove a breakpoint set by a host tool */

	if ((pBp->bp_task == tid || tid == ALL) && !(pBp->bp_flags & BP_HOST))
	    wdbDbgBpRemove (pBp);
	}

    taskUnlock ();	/* UNLOCK PREEMPTION */

    return (OK);
    }

/*******************************************************************************
*
* c - continue from a breakpoint
*
* This routine continues the execution of a task that has stopped at a
* breakpoint.  
*
* To execute, enter:
* .CS
*     -> c [task [,addr[,addr1]]]
* .CE
* If <task> is omitted or zero, the last task referenced is assumed.
* If <addr> is non-zero, the program counter is changed to <addr>;
* if <addr1> is non-zero, the next program counter is changed to <addr1>,
* and the task is continued.
*
* CAVEAT
* When a task is continued, c() does not distinguish between a suspended
* task or a task suspended by the debugger.  Therefore, its use should be
* restricted to only those tasks being debugged.
* 
* NOTE
* The next program counter, <addr1>, is currently supported only by SPARC.
*
* RETURNS:
* OK, or ERROR if the specified task does not exist.
*
* SEE ALSO: tr(),
* .pG "Target Shell,"
* windsh,
* .tG "Shell"
*/

STATUS c
    (
    int		task,	/* task that should proceed from breakpoint     */
    INSTR *	addr,	/* address to continue at; 0 = next instruction */
    INSTR *	addr1	/* address for npc; 0 = instruction next to pc */
    )
    {
    int 	tid = 0;
    WIND_TCB *	pTcb;

    if (dbgInstall () != OK)
	return (ERROR);

    if ((tid = taskIdFigure (task)) == ERROR)   /* convert to task id */
	{
	printErr (DBG_TASK_NOT_FOUND);
	return (ERROR);
	}

    tid = taskIdDefault (tid);	/* set default task id */

    if (dbgCheckBreakable (tid) == ERROR)
	return (ERROR);

    pTcb = taskTcb(tid);

    if (pTcb == NULL)
	return (ERROR);

    if (!taskIsSuspended (tid))
	return (ERROR);

    /* adjust pc & continue the task */

    if (addr != 0)
	_dbgTaskPCSet (tid, addr, addr1);

    if (dbgTaskCont (tid) != OK)
	{
	printErr (DBG_TASK_IX_NOT_FOUND, tid);
	return (ERROR);
	}

    return (OK);
    }

/*******************************************************************************
*
* cret - continue until the current subroutine returns
*
* This routine places a breakpoint at the return address of the current
* subroutine of a specified task, then continues execution of that task.
*
* To execute, enter:
* .CS
*     -> cret [task]
* .CE
* If <task> is omitted or zero, the last task referenced is assumed.
*
* When the breakpoint is hit, information about the task will be printed in
* the same format as in single-stepping.  The breakpoint is automatically
* removed when hit, or if the task hits another breakpoint first.
*
* RETURNS:
* OK, or ERROR if there is no such task or the breakpoint table is
* full.
*
* SEE ALSO: so(),
* .pG "Target Shell",
* windsh,
* .tG "Shell"
*/

STATUS cret
    (
    int task		/* task to continue, 0 = default */
    )
    {
#if 	(DBG_CRET == TRUE)
    int 	tid = 0;	/* task ID */
    REG_SET	regSet;		/* task's register set */
    WIND_TCB *	pTcb;		/* pointer on task TCB */
    INSTR *	retAdrs;	/* return address */

    if (dbgInstall () != OK)
	return (ERROR);

    if ((tid = taskIdFigure (task)) == ERROR)	/* convert to task id */
	{
	printErr (DBG_TASK_NOT_FOUND);
	return (ERROR);
	}

    tid = taskIdDefault (tid);	  /* set default task id */

    /* check if task is breakable */

    if (dbgCheckBreakable (tid) == ERROR)
	return (ERROR);

    if (taskRegsGet (tid, &regSet) != OK)
	{
	printErr (DBG_TASK_IX_NOT_FOUND, tid);
	return (ERROR);
	}

    pTcb = taskTcb (tid);

    if (pTcb == NULL)
	return (ERROR);

    if (!taskIsSuspended (tid))
	return (ERROR);

    retAdrs = _dbgRetAdrsGet (&regSet);

    /* add breakpoint to breakpoint table */

    if (dbgBrkAdd (retAdrs, tid, BP_SO, WDB_ACTION_STOP | WDB_ACTION_NOTIFY, 
			0, NULL, 0) == NULL)
	{
	printErr (DBG_BKPT_TBL_FULL);
	return (ERROR);
	}

    if (dbgTaskCont (tid) != OK)
	{
	printErr (DBG_TASK_IX_NOT_FOUND, tid);
	return (ERROR);
	}

    return (OK);
#else	/* (DBG_CRET == FALSE) */
    printErr (DBG_CRET_NOT_SUPP);
    return (ERROR);
#endif	/* (DBG_CRET == TRUE) */
    }

/*******************************************************************************
*
* s - single-step a task
*
* This routine single-steps a task that is stopped at a breakpoint.  
*
* To execute, enter:
* .CS
*     -> s [task[,addr[,addr1]]]
* .CE
* If <task> is omitted or zero, the last task referenced is assumed.
* If <addr> is non-zero, then the program counter is changed to <addr>;
* if <addr1> is non-zero, the next program counter is changed to <addr1>,
* and the task is stepped.
*
* CAVEAT
* When a task is continued, s() does not distinguish between a suspended
* task or a task suspended by the debugger.  Therefore, its use should be
* restricted to only those tasks being debugged.
*
* NOTE
* The next program counter, <addr1>, is currently supported only by SPARC.
*
* RETURNS: OK, or ERROR if the debugging package is not installed, the task
* cannot be found, or the task is not suspended.
*
* SEE ALSO:
* .pG "Target Shell,"
* windsh,
* .tG "Shell"
*/

STATUS s
    (
    int		taskNameOrId,	/* task to step; 0 = use default	    */
    INSTR *	addr,		/* address to step to; 0 = next instruction */
    INSTR *	addr1		/* address for npc, 0 = next instruction */
    )
    {
    int		tid;
    WIND_TCB * 	pTcb;

    if (dbgInstall () != OK)
	return (ERROR);

    if ((tid = taskIdFigure (taskNameOrId)) == ERROR)   /* convert to task id */
	{
	printErr (DBG_TASK_NOT_FOUND);
	return (ERROR);
	}

    tid = taskIdDefault (tid);			/* set default task id */

    if (dbgCheckBreakable (tid) == ERROR)
	return (ERROR);

    pTcb = taskTcb (tid);

    if (pTcb == NULL)
	return (ERROR);

    if (!taskIsSuspended (tid))
	return (ERROR);

    /* adjust pc & resume the task */

    if (addr != 0)
	_dbgTaskPCSet (tid, addr, addr1);

    /* mark task as single-stepping */

    DBG_INFO(tid)->wdbState |= WDB_STEP_TARGET;

    if (taskResume (tid) != OK)
	return (ERROR);

    return (OK);
    }

/*******************************************************************************
*
* so - single-step, but step over a subroutine
*
* This routine single-steps a task that is stopped at a breakpoint.
* However, if the next instruction is a JSR or BSR, so() breaks at the
* instruction following the subroutine call instead.
*
* To execute, enter:
* .CS
*     -> so [task]
* .CE
* If <task> is omitted or zero, the last task referenced is assumed.
*
* SEE ALSO:
* .pG "Target Shell",
* windsh,
* .tG "Shell"
*/

STATUS so
    (
    int		task	/* task to step; 0 = use default */
    )
    {
    INSTR *	pc;
    int		tid;
    WIND_TCB *	pTcb;

    if (dbgInstall () != OK)
	return (ERROR);

    if ((tid = taskIdFigure (task)) == ERROR)	   /* convert to task id */
	{
	printErr (DBG_TASK_NOT_FOUND);
	return (ERROR);
	}

    tid = taskIdDefault (tid);		/* set default task id */

    if (dbgCheckBreakable (tid) == ERROR)
	return (ERROR);

    pTcb = taskTcb (tid);

    if (pTcb == NULL)
	return (ERROR);

    if (!taskIsSuspended (tid))
	return (ERROR);

    pc = _dbgTaskPCGet (tid);

    if (_dbgFuncCallCheck (pc))  /* check if function call instruction */
	{
	/* add breakpoint to breakpoint table */

	if (dbgBrkAdd (pc + _dbgInstSizeGet (pc), tid, BP_SO, 
			WDB_ACTION_STOP | WDB_ACTION_NOTIFY, 
			0, NULL, 0) == NULL)
	    {
	    printErr (DBG_BKPT_TBL_FULL);
	    return (ERROR);
	    }

	if (dbgTaskCont (tid) == ERROR)
	    {
	    printErr (DBG_TASK_IX_NOT_FOUND, tid);
	    return (ERROR);
	    }
	}
    else
	{
	/* mark task as single-stepping */

	DBG_INFO(tid)->wdbState |= WDB_STEP_TARGET;

	/* resume the task */

	if (taskResume (tid) != OK)
	    {
	    printErr (DBG_TASK_IX_NOT_FOUND, tid);
	    return (ERROR);
	    }
	}

    return (OK);
    }

/*******************************************************************************
*
* dbgBrkAdd - add a breakpoint to the breakpoint table
*
* RETURNS: 
* Breakpoint structure address or NULL if we can't add the breakpoint.
*
* NOMANUAL
*/

LOCAL BRKPT * dbgBrkAdd
    (
    INSTR *	addr,		/* where to set breakpoint		*/
    int		task,		/* task for which to set breakboint	*/
    unsigned	flags,		/* breakpoint flags			*/
    unsigned    action,		/* breakpoint action			*/
    unsigned    count,		/* breakpoint count			*/
    FUNCPTR	callRtn,	/* routine to call			*/
    int		callArg		/* routine argument			*/
    )
    {
    BRKPT *	pBp;

    /* get a free breakpoint entry */

    taskLock ();		/* LOCK PREEMPTION */

    if (dll_empty (&bpFreeList))
	pBp = NULL;
    else
	{
	pBp = BP_BASE(dll_tail (&bpFreeList));
	dll_remove (&pBp->bp_chain);
	}

    taskUnlock ();		/* UNLOCK PREEMPTION */

    if (pBp == NULL)
	{
	pBp = (BRKPT *) malloc (sizeof (BRKPT));

	if (pBp == NULL)
	    return (NULL);
	}

    /* initialize breakpoint and add to list */

    pBp->bp_flags = flags;
    pBp->bp_addr = addr;
    pBp->bp_task = task;
    pBp->bp_action = action;
    pBp->bp_count = count;
    pBp->bp_callRtn = (void (*)()) callRtn;
    pBp->bp_callArg = callArg;

    if ((pBp->bp_flags  & BRK_HARDWARE) == 0)
	pBp->bp_instr = *addr;

    taskLock ();				/* LOCK PREEMPTION */
    dll_insert(&pBp->bp_chain, &bpList);	/* add bp to active list */
    taskUnlock ();				/* UNLOCK PREEMPTION */

    return (pBp);
    }

/*******************************************************************************
*
* dbgBrkDisplay - display breakpoints
*
* NOMANUAL
*/

LOCAL void dbgBrkDisplay (void)
    {
    dll_t *	pDll;
    BRKPT *	pBp;
    BOOL	nobreaks = TRUE;	/* flag that no breakpoints are set */

    for (pDll = dll_head(&bpList); pDll != dll_end(&bpList);
	    pDll = dll_next(pDll))
	{
	/* 
	 * taskLock() and taskUnlock() are useless because of the use of
	 * printf(). So, one or more breakpoints could be deleted by
	 * another task while we are reading the breakpoint list.
	 * If it happens, we can enter an infinite loop.
	 */

	pBp = BP_BASE(pDll);

	nobreaks = FALSE;

	dbgBrkPrint (pBp->bp_addr);

	if (pBp->bp_task == ALL)
	    printf ("   Task: %9s   Count: %2d", "all", pBp->bp_count);
	else
	    printf ("   Task: %#9x   Count: %2d", pBp->bp_task, pBp->bp_count);

#if	DBG_HARDWARE_BP
	/* print informations on hardware breakpoints */

	_dbgBrkDisplayHard (pBp); 
#endif  /* DBG_HARDWARE_BP */

	if (pBp->bp_flags & BP_HOST )
	    printf ("   (tgtsvr)");
	else
	    {
	    if (pBp->bp_flags & BP_EVENT)
		printf ("   (event)");
	    else if (pBp->bp_flags & BP_SO )
		printf ("   (temporary)");
	    else if (!(pBp->bp_action & WDB_ACTION_NOTIFY ))
		printf ("   (quiet)");
	    }
	printf ("\n");
	}

    if (nobreaks)
	printErr (DBG_NO_BKPTS);
    }

/*******************************************************************************
*
* dbgBrkPrint - print breakpoint address
*
* NOMANUAL
*/

LOCAL void dbgBrkPrint 
    (
    INSTR *	addr		    /* address of breakpoint */
    )
    {
    char *    label;	    /* pointer to name from symbol table */
    void *    actVal;		        /* actual value of label */
    SYMBOL_ID symId;                        /* symbol identifier */
    int       displacement;
    char      demangled [DBG_DEMANGLE_PRINT_LEN+1];
    char *    labelToPrint;

    printf ("0x%08x: ", (UINT32) addr);		/* print breakpoint address */

    /* print symbolic address and displacement, if any */

    if ((symFindSymbol (sysSymTbl, NULL, (void *)addr, 
			SYM_MASK_NONE, SYM_MASK_NONE, &symId) == OK) &&
	(symNameGet (symId, &label) == OK) &&
	(symValueGet (symId, &actVal) == OK))
	{
	labelToPrint = cplusDemangle (label, demangled, sizeof (demangled));
	printf ("%-15s", labelToPrint);
	if ((displacement = (int)addr - (int)actVal) != 0)
	    printf ("+0x%-4x", displacement);
	else
	    printf ("%6s", "");		/* no displacement */
	}
    else
	printf ("%21s", "");		/* no symbolic address */
    }

/*******************************************************************************
*
* dbgCheckBreakable - make sure task is breakable
*
* Prints an error message if task is unbreakable.
*
* RETURNS: OK or ERROR.
*
* NOMANUAL
*/

LOCAL STATUS dbgCheckBreakable
    (
    int task			/* task id */
    )
    {
    WIND_TCB *	pTcb;	/* pointer on task TCB */

    if (task == ERROR)
	return (ERROR);

    pTcb = taskTcb (task);

    if (pTcb == NULL)
	{
	printErr (DBG_TASK_IX_NOT_FOUND, task);
	return (ERROR);
	}

    if ((pTcb->options & VX_UNBREAKABLE) != 0)
	{
	int tid = task == 0 ? taskIdSelf () : task;

	printErr ("Task %#x (%s) is unbreakable.\n", tid, taskName (tid));

	return (ERROR);
	}

    return (OK);
    }

/*******************************************************************************
*
* dbgTlBreak - print info about a breakpoint 
*
* This routine prints info about a breakpoint we have encountered.
*
* RETURNS : NA
*
* NOMANUAL
*/

LOCAL void dbgTlBreak
    (
    int		task,		/* taskId */
    INSTR *	pc,		/* task's pc */
    INSTR *	addr		/* breakpoint's address */
    )
    {
    WIND_TCB * pTcb;	/* pointer on task TCB */

    taskIdDefault (task);

    pTcb = taskTcb(task);

    if (pTcb == NULL)
	return;

    /* print breakpoint info */

    printErr ("\nBreak at ");
    dbgBrkPrint (addr);		/* print the address */
    printErr ("   Task: %#x (%s)\n", task, taskName (task));
    nextToDsm = pc;		/* update l's default list-from */

    /* 
     * Reset the flag dbgUnbreakableOld. This flag was set to prevent
     * encontering breakpoints in this routine (even breakpoint with 
     * stop action not defined).
     */

    dbgUnbreakableOld = FALSE;
    }

/*******************************************************************************
*
* dbgTlSnglStep - print info about a single-stepping task
*
* NOMANUAL
*/

LOCAL void dbgTlSnglStep
    (
    int		task		/* task id */
    )
    {
    int		pc;
    WIND_TCB *	pTcb;
    dll_t *	pDll;
    dll_t *	pNextDll;
    BRKPT *	pBp;

    /* get pc of the task */

    pc = (int) _dbgTaskPCGet (task);

    /* remove (if any) temporary breakpoint set by so or cret instruction */

    taskLock ();	/* LOCK PREEMPTION */
    for (pDll = dll_head(&bpList); pDll != dll_end(&bpList);
		pDll = pNextDll)
	{
	pNextDll = dll_next(pDll);

	pBp = BP_BASE(pDll);

	if ((pBp->bp_task == (UINT32) task) && 
	    (pBp->bp_addr == (INSTR *) pc) &&
	    (pBp->bp_flags == BP_SO))
	    {
	    wdbDbgBpRemove (pBp);
	    }
	}
    taskUnlock ();	/* UNLOCK PREEMPTION */

    taskIdDefault (task);	/* update default task */

    pTcb = taskTcb (task);

    if (pTcb == NULL)
	return;

    /* print the registers and disassemble next instruction */

    taskRegsShow (task);

    if ((pTcb->options & VX_FP_TASK) && dbgPrintFpp)
	{
	/* 
	 * On the other hand fppTaskRegsShow() may modify the FP registers
	 * (this depends of the architecture & compiler: it does happen for
	 * PPC, SH): therefore we need to restore the FP context to what it 
	 * was before fppTaskRegsShow() so that the fppSwapHook() will save
	 * the correct value in the TCB when the next FP task is swapped in.
	 * (see also SPR #7983). This is needed because this routine is
	 * executed in the tExcTask context. tExcTask is spawned without the
	 * VX_FP_TASK option.
	 */
	fppSave(pTcb->pFpContext);
	fppTaskRegsShow (task);
	fppRestore(pTcb->pFpContext);
	}

    if ((pTcb->options & VX_DSP_TASK) && dbgPrintDsp && 
        (_func_dspTaskRegsShow != NULL))
	{
	(* _func_dspTaskRegsShow) (task);
	}

#if (CPU_FAMILY==I80X86)
    if ((pTcb->options & VX_FP_TASK) && dbgPrintSimd &&
        (_func_sseTaskRegsShow != NULL))
        {
        (* _func_sseTaskRegsShow) (task);
        }
#endif /* (CPU_FAMILY==I80X86) */

#ifdef _WRS_ALTIVEC_SUPPORT
    if ((pTcb->options & VX_ALTIVEC_TASK) && dbgPrintAltivec)
        altivecTaskRegsShow (task);
#endif /* _WRS_ALTIVEC_SUPPORT */

    (void) dbgList ((INSTR *) pc, 1);
    nextToDsm = (INSTR *) pc;

    /* 
     * Reset the flag dbgUnbreakableOld. This flag was set to prevent
     * encountering breakpoints (for example if ACTION_STOP was not
     * specified) in this unbreakable routine.
     */

    dbgUnbreakableOld = FALSE;
    }

/*******************************************************************************
*
* dbgTyAbort - abort the shell from interrupt level
*
* NOMANUAL
*/

LOCAL void dbgTyAbort (void)
    {
    excJobAdd ((VOIDFUNCPTR)dbgTlTyAbort, 0,0,0,0,0,0);
    }

/*******************************************************************************
*
* dbgTlTyAbort - task level portion of shell abort
*
* NOMANUAL
*/

LOCAL void dbgTlTyAbort (void)
    {
    tyAbortFuncSet ((FUNCPTR) NULL);	/* cancel abort routine */

    /* reset the standard input and output channels in case they are hung up */

    ioctl (STD_IN,  FIOFLUSH, 0);
    ioctl (STD_OUT, FIOFLUSH, 0);
    ioctl (STD_ERR, FIOFLUSH, 0);

    tt (shellTaskId);

    if ((taskRestart (shellTaskId)) != ERROR)
	printErr ("%s restarted.\n", taskName (shellTaskId));
    else
	{
	printErr ("spawning new shell.\n");

	if (shellInit (0, TRUE) == ERROR)
	    printErr ("shell spawn failed!\n");
	}

    tyAbortFuncSet ((FUNCPTR) dbgTyAbort);		/* set abort routine */
    }

/*******************************************************************************
*
* l - disassemble and display a specified number of instructions
*
* This routine disassembles a specified number of instructions and displays
* them on standard output.  If the address of an instruction is entered in
* the system symbol table, the symbol will be displayed as a label for that
* instruction.  Also, addresses in the opcode field of instructions will be
* displayed symbolically.
*
* To execute, enter:
* .CS
*     -> l [address [,count]]
* .CE
* If <address> is omitted or zero, disassembly continues from the previous
* address.  If <count> is omitted or zero, the last specified count is used
* (initially 10).  As with all values entered via the shell, the address may
* be typed symbolically.
*
* RETURNS: N/A
*
* SEE ALSO: 
* .pG "Target Shell,"
* windsh,
* .tG "Shell"
*/

void l 
    (
    INSTR * addr,       /* address of first instruction to disassemble */
			/* if 0, continue from the last instruction */
			/* disassembled on the last call to l */
    int    count	/* number of instruction to disassemble */
			/* if 0, use the same as the last call to l */
    )
    {
    addr = (INSTR *) ROUND_DOWN (addr, DBG_INST_ALIGN);	/* force alignment */

    /* set new start address and instruction count if specified */

    if ((int) addr != 0)
	nextToDsm = addr;

    if (count != 0)
	dfltCount = count;

    nextToDsm = dbgList (nextToDsm, dfltCount);	/* disassemble n instructions */
    }

/*******************************************************************************
*
* dbgList - disassemble and list n instructions
*
* This routine disassembles some number of instructions, using dsmLib.
* The normal user interface is through the routine 'l', rather than through
* this routine.
*
* RETURNS: Pointer to instruction following last instruction disassembled.
*
* NOMANUAL
*/

LOCAL INSTR * dbgList 
    (
    INSTR *	addr,	/* address of first instruction to disassemble */
    int		count 	/* number of intructions to dissamble */
    )
    {
    char *    label;   /* pointer to symbol table copy of symbol name string */
    void *    actVal;  /* actual address of symbol found */
    SYMBOL_ID symId;   /* symbol identifier */
    int       ix;
    char      demangled[DBG_DEMANGLE_PRINT_LEN + 1];
    char *    labelToPrint;

    for (ix = 0; ix < count; ix++)
	{
	if ((symFindSymbol (sysSymTbl, NULL, (void *)addr, 
			    SYM_MASK_NONE, SYM_MASK_NONE, &symId) == OK) &&
	    (symNameGet (symId, &label) == OK) &&
	    (symValueGet (symId, &actVal) == OK) &&
	    (actVal == (void *)addr))
	    {
	    labelToPrint = cplusDemangle(label, demangled, sizeof (demangled));
#if (CPU_FAMILY == SH)
	    printf ("               %s:\n", labelToPrint);
#else
	    printf ("			%s:\n", labelToPrint);
#endif
	    }

	if (_dbgDsmInstRtn == (FUNCPTR) NULL)
	    printErr ("dbgList: no disassembler.\n");
	else
	    addr += (*_dbgDsmInstRtn) (addr, (int)addr, dbgPrintAdrs);
	}

    return (addr);
    }

/*******************************************************************************
*
* dbgPrintAdrs - prints addresses as symbols, if an exact match can be found
*
* NOMANUAL
*/

LOCAL void dbgPrintAdrs 
    (
    int address 	/* address to print */
    )
    {
    char *    label;  /* pointer to symbol table copy of symbol name string */
    void *    actVal; /* actual address of symbol found */
    SYMBOL_ID symId;  /* symbol identifier */
    char      demangled[DBG_DEMANGLE_PRINT_LEN + 1];
    char *    labelToPrint;

    if ((symFindSymbol (sysSymTbl, NULL, (void *)address, 
			SYM_MASK_NONE, SYM_MASK_NONE, &symId) == OK) &&
	(symNameGet (symId, &label) == OK) &&
	(symValueGet (symId, &actVal) == OK) &&
	(actVal == (void *)address))
	{
	labelToPrint = cplusDemangle(label, demangled, sizeof (demangled));
	printf ("%s", labelToPrint);
	}
    else
	printf ("0x%08x", address);
    }

/*******************************************************************************
*
* tt - display a stack trace of a task
*
* This routine displays a list of the nested routine calls that the specified
* task is in.  Each routine call and its parameters are shown.
*
* If <taskNameOrId> is not specified or zero, the last task referenced is
* assumed.  The tt() routine can only trace the stack of a task other than
* itself.  For instance, when tt() is called from the shell, it cannot trace
* the shell's stack.
*
* EXAMPLE
* .CS
*     -> tt "logTask"
*      3ab92 _vxTaskEntry   +10 : _logTask (0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
*       ee6e _logTask       +12 : _read (5, 3f8a10, 20)
*       d460 _read          +10 : _iosRead (5, 3f8a10, 20)
*       e234 _iosRead       +9c : _pipeRead (3fce1c, 3f8a10, 20)
*      23978 _pipeRead      +24 : _semTake (3f8b78)
*     value = 0 = 0x0
* .CE
* This indicates that logTask() is currently in semTake() (with
* one parameter) and was called by pipeRead() (with three parameters),
* which was called by iosRead() (with three parameters), and so on.
*
* INTERNAL
* This higher-level symbolic stack trace is built on top of the
* lower-level routines provided by trcLib.
*
* CAVEAT
* In order to do the trace, some assumptions are made.  In general, the
* trace will work for all C language routines and for assembly language
* routines that start with a LINK instruction.  Some C compilers require
* specific flags to generate the LINK first.  Most VxWorks assembly language
* routines include LINK instructions for this reason.  The trace facility
* may produce inaccurate results or fail completely if the routine is
* written in a language other than C, the routine's entry point is
* non-standard, or the task's stack is corrupted.  Also, all parameters are
* assumed to be 32-bit quantities, so structures passed as parameters will
* be displayed as \f2long\fP integers.
*
* RETURNS:
* OK, or ERROR if the task does not exist.
*
* SEE ALSO:
* .pG "Target Shell,"
* windsh,
* .tG "Shell"
*/

STATUS tt
    (
    int		taskNameOrId	/* task name or task ID */
    )
    {
    REG_SET regSet;
    BOOL resumeIt = FALSE;	/* flag to remember if resuming is necessary */
    int tid = taskIdFigure (taskNameOrId);	/* convert to task id */

    if (tid == ERROR)
	{
	printErr (DBG_TASK_NOT_FOUND);
	return (ERROR);
	}

    tid = taskIdDefault (tid);		/* set default task id */

    /* get caller task's id and make sure it is not the task to be traced */

    if (tid == taskIdSelf () || tid == 0)
	{
	printErr ("Sorry, traces of my own stack begin at tt ().\n");
	return (ERROR);
	}

    /* make sure the task exists */

    if (taskIdVerify (tid) != OK)
	{
	printErr ("Can't trace task %#x: invalid task id.\n", tid);
	return (ERROR);
	}

    /* if the task is not already suspended, suspend it while we trace it */

    if (!taskIsSuspended (tid))
	{
	resumeIt = TRUE;		/* we want to resume it later */
	taskSuspend (tid);		/* suspend the task if need be */
	}

    /* trace the stack */

    taskRegsGet (tid, &regSet);
    trcStack (&regSet, (FUNCPTR) dbgPrintCall, tid);

    if (resumeIt)
	taskResume (tid);		/* resume task if we suspended it */

    return (OK);
    }
    
/*******************************************************************************
*
* dbgPrintCall - print a stack frame
*
* This routine is called by trcStack to print each level in turn.
* If nargs is specified as 0, then a default number of args (trcDefaultArgs)
* is printed in brackets ("[..]"), since this often indicates that the
* number of args is unknown.
*
* NOMANUAL	- also used by shellSigHandler
*/

void dbgPrintCall
    (
    INSTR *	callAdrs,	/* address from which function was called */
    int		funcAdrs,	/* address of function called */
    int		nargs,		/* number of arguments in function call */
    UINT32 *	args		/* pointer to function args */
    )
    {
    int       ix;
    SYMBOL_ID symId;                                     /* symbol identifier */
    char *    name;  /* pointer to symbol tbl copy of function name goes here */
    void *    val;  	  		     /* address of named fn goes here */
    BOOL      doingDefault = FALSE;
    char      demangled [DBG_DEMANGLE_PRINT_LEN + 1];
    char *    nameToPrint;

    /* print call address and name of calling function plus offset */

    printErr ("%6x ", callAdrs);	/* print address from which called */

    if ((symFindSymbol (sysSymTbl, NULL, (void *)callAdrs, 
			SYM_MASK_NONE, SYM_MASK_NONE, &symId) == OK) &&
	(symNameGet (symId, &name) == OK) &&
	(symValueGet (symId, &val) == OK))
	{
	nameToPrint = cplusDemangle (name, demangled, sizeof (demangled));
	printErr ("%-15s+%-3x: ", nameToPrint, (int)callAdrs - (int)val);
	}
    else
#if CPU_FAMILY == ARM
	/* make things line up properly */
        printErr ("                   : ");
#else
	printErr ("                     : ");
#endif


    /* print function address/name */

    if ((symFindSymbol (sysSymTbl, NULL, (void *)funcAdrs, 
			SYM_MASK_NONE, SYM_MASK_NONE, &symId) == OK) &&
	(symNameGet (symId, &name) == OK) &&
	(symValueGet (symId, &val) == OK) &&
	(val == (void *)funcAdrs))
	{
	nameToPrint = cplusDemangle (name, demangled, sizeof (demangled));
	printErr ("%s (", nameToPrint);		/* print function name */
	}
    else
	printErr ("%x (", funcAdrs);		/* print function address */

    /* if no args are specified, print out default number (see doc at top) */

    if ((nargs == 0) && (trcDefaultArgs != 0))
	{
	doingDefault = TRUE;
	nargs = trcDefaultArgs;
	printErr ("[");
	}

    /* print args */

    for (ix = 0; ix < nargs; ix++)
	{
	if (ix > 0)
	    printErr (", ");

	if (args [ix] == 0)
	    printErr ("0");

	else if ((symFindSymbol (sysSymTbl, NULL, (void *)args[ix], 
				 SYM_MASK_NONE, SYM_MASK_NONE, &symId) == OK) &&
		 (symNameGet (symId, &name) == OK) &&
		 (symValueGet (symId, &val) == OK) &&
		 (val == (void *)args [ix]))
	    {
	    nameToPrint = cplusDemangle (name, demangled, sizeof (demangled));
	    printErr ("&%s", nameToPrint);	/* print argument name */
	    }
	else
	    printErr ("%x", args [ix]);
	}

    if (doingDefault)
	printErr ("]");

    printErr (")\n");
    }

/******************************************************************************
*
* dbgE - code to support the windview "e" command
* 
* This routine is called each time a windview breakpoint is encountered
* This is a temporary solution. This routine should be moved in windview 
* code.
*
* RETURNS : NA
*
* NOMANUAL
*/ 

LOCAL void dbgE
    (
    EVT_CALL_ARGS *	pEvtCallArgs,
    REG_SET *		pRegisters
    )
    {
    int (*conditionFunc)() = (int (*)())pEvtCallArgs->evtRtn;

    if (conditionFunc == NULL || conditionFunc (pEvtCallArgs->evtRtnArg) == OK)
        EVT_CTX_BUF (pEvtCallArgs->eventId, pRegisters->reg_pc, 0, NULL);
    }

/*******************************************************************************
*
* dbgTargetNotify - notify the target when a breakpoint is encountered 
*
* This routine is called to notify the shell user through the console
* when a breakpoint is encountered. Depending on the type of breakpoint, 
* the action is different.
*
* RETURNS: N/A.
*
* NOMANUAL
*/

LOCAL void dbgTargetNotify
    (
    UINT32	flag,		/* type of breakpoint (step over, step ...) */
    UINT32	addr,		/* breakpoint addr */
    REG_SET * 	pRegisters	/* task registers before exception */
    )
    {
    /* 
     * Prevent excTask to encounter a breakpoint event if there is not
     * ACTION_STOP. This is necessary to avoid recursive loop if a 
     * NOTIFY breakpoint could be encountered by dbgTlSnglStep or
     * dbgTlBreak. dbgUnbreakableOld should be restored to FALSE state
     * in dbgTlSnglStep and dbgTlBreak routines.
     */

    dbgUnbreakableOld = TRUE;

    /* check the breakpoint flag in order to determine the action to perform */

    if (flag & (BP_SO | BP_STEP))
	excJobAdd (dbgTlSnglStep, (UINT32) taskIdCurrent, 0, 0, 0, 0, 0);
    else
	excJobAdd (dbgTlBreak, (UINT32) taskIdCurrent, (UINT32) pRegisters->pc, 
					addr, 0, 0, 0);
    }
