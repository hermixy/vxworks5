/* excLib.c - generic exception handling facilities */

/* Copyright 1984-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01s,13nov01,yvp  Included private/excLibP.h
01r,13sep01,pcm  moved excShowInit() from excInit() to usrConfig.c (SPR 7333)
01q,04sep98,cdp  apply 01o for all ARM CPUs with ARM_THUMB==TRUE.
01p,25feb98,jgn  make logMsg calls indirect for scalability (SPR #20625)
01o,03dec97,cdp  force excTask to call func in Thumb state (ARM7TDMI_T).
01n,09oct06,dgp  doc: correct excHookAdd() reference to ESF0 per SPR 7013
01m,19jun96,dgp  doc: change excHookAdd() description (SPR #6684)
01l,06oct95,jdi  removed .pG "Debugging".
01k,21jan93,jdi  documentation cleanup for 5.1.
01j,23aug92,jcf  added _func_excJobAdd for scalability.
01i,02aug92,jcf  moved printExc() to fioLib.c.
01h,30jul92,rrr  backed out 01g (now back to 01f)
01g,30jul92,kdl  backed out 01f changes pending rest of exc handling.
01f,29jul92,rrr  removed excDeliverHook and excDeliverSignal now in signals
                 added hooks for exc<Arch>Lib
01e,28jul92,rdc  made printExc write enable sysExcMsg memory.
01d,27jul92,rrr  added excDeliverHook for signal delivery
01c,04jul92,jcf  scalable/ANSI/cleanup effort.
01b,26may92,rrr  the tree shuffle
01a,09jan92,yao  written from 960/excLib.c version 03f.  fixed document,
		 spr#1148.
*/

/*
This library provides generic initialization facilities for handling
exceptions.  It safely traps and reports exceptions caused by program
errors in VxWorks tasks, and it reports occurrences of interrupts that are
explicitly connected to other handlers.  For information about
architecture-dependent exception handling facilities, see the manual entry
for excArchLib.

INITIALIZATION
Initialization of excLib facilities occurs in two steps.  First, the routine
excVecInit() is called to set all vectors to the default handlers for an
architecture provided by the corresponding architecture exception handling
library.  Since this does not involve VxWorks' kernel facilities, it is
usually done early in the system start-up routine usrInit() in the library
usrConfig.c with interrupts disabled.

The rest of this package is initialized by calling excInit(), which spawns
the exception support task, excTask(), and creates the message queues used to
communicate with it.

Exceptions or uninitialized interrupts that occur after the vectors
have been initialized by excVecInit(), but before excInit() is called,
cause a trap to the ROM monitor.

NORMAL EXCEPTION HANDLING
When a program error generates an exception (such as divide by zero, or a
bus or address error), the task that was executing when the error occurred
is suspended, and a description of the exception is displayed on standard
output.  The VxWorks kernel and other system tasks continue uninterrupted.
The suspended task can be examined with the usual VxWorks routines,
including ti() for task information and tt() for a stack trace.  It may
be possible to fix the task and resume execution with tr().  However, tasks
aborted in this way are often unsalvageable and can be deleted with td().

When an interrupt that is not connected to a handler occurs, the default
handler provided by the architecture-specific module displays a
description of the interrupt on standard output.

ADDITIONAL EXCEPTION HANDLING ROUTINE
The excHookAdd() routine adds a routine that will be called when a hardware
exception occurs.  This routine is called at the end of normal exception
handling.

TASK-LEVEL SUPPORT
The excInit() routine spawns excTask(), which performs special exception
handling functions that need to be done at task level.  Do not suspend,
delete, or change the priority of this task.

DBGLIB
The facilities of excLib, including excTask(), are used by dbgLib to support
breakpoints, single-stepping, and additional exception handling functions.

SIGLIB
A higher-level, UNIX-compatible interface for hardware and software
exceptions is provided by sigLib.  If sigvec() is used to initialize
the appropriate hardware exception/interrupt (e.g., BUS ERROR == SIGSEGV),
excLib will use the signal mechanism instead.

INCLUDE FILES: excLib.h

SEE ALSO: dbgLib, sigLib, intLib
*/

/* LINTLIBRARY */

#include "vxWorks.h"
#include "esf.h"
#include "iv.h"
#include "intLib.h"
#include "msgQLib.h"
#include "signal.h"
#include "taskLib.h"
#include "errno.h"
#include "stdarg.h"
#include "logLib.h"
#include "stdio.h"
#include "private/excLibP.h"
#include "private/funcBindP.h"

/* global variables */

FUNCPTR excExcepHook;	/* add'l rtn to call when exceptions occur */

MSG_Q_ID excMsgQId;	/* ID of msgQ to excTask */

/* excTask parameters */

int excTaskId;
int excTaskPriority	= 0;
int excTaskOptions	= VX_SUPERVISOR_MODE | VX_UNBREAKABLE;
int excTaskStackSize	= 8000;


#define EXC_MAX_ARGS	6		/* max args to task level call */
#define EXC_MAX_MSGS	10		/* max number of exception msgs */

typedef struct				/* EXC_MSG */
    {
    VOIDFUNCPTR	func;			/* pointer to function to invoke */
    int		arg [EXC_MAX_ARGS];	/* args for function */
    } EXC_MSG;


/* local variables */

LOCAL int excMsgsLost;			/* count of messages to excTask lost */


/*******************************************************************************
*
* excInit - initialize the exception handling package
*
* This routine installs the exception handling facilities and spawns excTask(),
* which performs special exception handling functions that need to be done at
* task level.  It also creates the message queue used to communicate with
* excTask().
*
* NOTE:
* The exception handling facilities should be installed as early as
* possible during system initialization in the root task, usrRoot(), in
* usrConfig.c.
*
* RETURNS:
* OK, or ERROR if a message queue cannot be created or excTask() cannot be
* spawned.
*
* SEE ALSO: excTask()
*/

STATUS excInit ()

    {
    _func_excJobAdd = (FUNCPTR) excJobAdd;

    excMsgQId = msgQCreate (EXC_MAX_MSGS, sizeof (EXC_MSG), MSG_Q_FIFO);

    if (excMsgQId == NULL)
	return (ERROR);

    excTaskId = taskSpawn ("tExcTask", excTaskPriority,
			   excTaskOptions, excTaskStackSize,
			   (FUNCPTR) excTask, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    return (excTaskId == ERROR ? ERROR : OK);
    }
/*******************************************************************************
*
* excHookAdd - specify a routine to be called with exceptions
*
* This routine specifies a routine that will be called when hardware
* exceptions occur.  The specified routine is called after normal exception
* handling, which includes displaying information about the error.  Upon return
* from the specified routine, the task that incurred the error is suspended.
*
* The exception handling routine should be declared as:
* .tS
*     void myHandler
*         (
*         int      task,    /@ ID of offending task             @/
*         int      vecNum,  /@ exception vector number          @/
*         <ESFxx>  *pEsf    /@ pointer to exception stack frame @/
*         )
* .tE
* where <task> is the ID of the task that was running when the exception
* occurred. <ESFxx> is architecture-specific and can be found by examining
* `/target/h/arch/<arch>/esf<arch>.h'; for example, the PowerPC uses ESFPPC.
*
* This facility is normally used by dbgLib() to activate its exception
* handling mechanism.  If an application provides its own exception handler,
* it will supersede the dbgLib mechanism.
*
* RETURNS: N/A
*
* SEE ALSO: excTask()
*/

void excHookAdd
    (
    FUNCPTR excepHook	/* routine to call when exceptions occur */
    )

    {
    excExcepHook = excepHook;
    }
/*******************************************************************************
*
* excJobAdd - request a task-level function call from interrupt level
*
* This routine allows interrupt level code to request a function call
* to be made by excTask at task-level.
*
* NOMANUAL
*/

STATUS excJobAdd (func, arg1, arg2, arg3, arg4, arg5, arg6)
    VOIDFUNCPTR func;
    int arg1;
    int arg2;
    int arg3;
    int arg4;
    int arg5;
    int arg6;

    {
    EXC_MSG msg;

    msg.func = func;
    msg.arg[0] = arg1;
    msg.arg[1] = arg2;
    msg.arg[2] = arg3;
    msg.arg[3] = arg4;
    msg.arg[4] = arg5;
    msg.arg[5] = arg6;

    if (msgQSend (excMsgQId, (char *) &msg, sizeof (msg),
		  INT_CONTEXT() ? NO_WAIT : WAIT_FOREVER, MSG_PRI_NORMAL) != OK)
        {
        ++excMsgsLost;
        return (ERROR);
        }

    return (OK);
    }
/*******************************************************************************
*
* excTask - handle task-level exceptions
*
* This routine is spawned as a task by excInit() to perform functions
* that cannot be performed at interrupt or trap level.  It has a priority of 0.
* Do not suspend, delete, or change the priority of this task.
*
* RETURNS: N/A
*
* SEE ALSO: excInit()
*/

void excTask ()

    {
    static int oldMsgsLost = 0;

    int newMsgsLost;
    EXC_MSG msg;

    FOREVER
	{
	if (msgQReceive (excMsgQId, (char *) &msg, sizeof (msg),
			 WAIT_FOREVER) != sizeof (msg))
            {
            if (_func_logMsg != NULL)
		_func_logMsg ("excTask: error receiving msg, status = %#x.\n",
		              errno, 0, 0, 0, 0, 0);
            }
        else
#if	((CPU_FAMILY == ARM) && ARM_THUMB)
	    /* force call in Thumb state */

	    (* (VOIDFUNCPTR)((UINT32)(msg.func) | 1)) (msg.arg[0], msg.arg[1],
						       msg.arg[2], msg.arg[3],
						       msg.arg[4], msg.arg[5]);
#else
            (* msg.func) (msg.arg[0], msg.arg[1], msg.arg[2],
			  msg.arg[3], msg.arg[4], msg.arg[5]);
#endif	/* CPU_FAMILY == ARM */

	/* check to see if interrupt level lost any more calls */

	if ((newMsgsLost = excMsgsLost) != oldMsgsLost)
	    {
	    if (_func_logMsg != NULL)
		_func_logMsg ("%d messages from interrupt level lost.\n",
		              newMsgsLost - oldMsgsLost, 0, 0, 0, 0, 0);

	    oldMsgsLost = newMsgsLost;
	    }
	}
    }
