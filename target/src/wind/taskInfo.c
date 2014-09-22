/* taskInfo.c - task information library */

/* Copyright 1984-1993 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01j,18dec00,pes  Correct compiler warnings
01i,29may98,cym  removed all ifdefs for SIMNT since the register set is now
		current.
01h,04mar98,jmb  add dec'l
01g,06jan98,cym  changed taskRegsGet for SIMNT to update TCB from 
		 windows context.
01g,10nov97,dbt  modified taskBpHook() routine interface.
01f,29oct96,jdi  doc: taskName() returns empty string of the task has no name.
01e,06oct95,jdi  changed Debugging .pG's to .tG "Shell".
01d,25feb93,jdi  doc: reinstated VX_UNBREAKABLE as publishable option, per kdl.
01c,04feb93,jdi  documentation cleanup for 5.1.
01b,04jan04,wmd  added check of calling task id before calling 
		 taskRegsStackToTcb() for the i960 in taskRegsGet().
		 added predeclarations for i960 to get rid of warnings.
01a,23aug92,jcf  extracted from v02y of taskLib.c.
*/

/*
DESCRIPTION
This library provides a programmatic interface for obtaining task information.

Task information is crucial as a debugging aid and user-interface 
convenience during the development cycle of an application.  
The routines taskOptionsGet(), taskRegsGet(), taskName(), taskNameToId(), 
taskIsReady(), taskIsSuspended(), and taskIdListGet() are used to obtain
task information.  Three routines -- taskOptionsSet(), taskRegsSet(), and
taskIdDefault() -- provide programmatic access to debugging features.

The chief drawback of using task information is that tasks may
change their state between the time the information is gathered and the
time it is utilized.  Information provided by these routines should
therefore be viewed as a snapshot of the system, and not relied upon
unless the task is consigned to a known state, such as suspended.

Task management and control routines are provided by taskLib.  Higher-level
task information display routines are provided by taskShow.

INCLUDE FILES: taskLib.h

SEE ALSO: taskLib, taskShow, taskHookLib, taskVarLib, semLib, kernelLib,
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
#include "private/sigLibP.h"
#include "private/classLibP.h"
#include "private/objLibP.h"
#include "private/smObjLibP.h"
#include "private/smFixBlkLibP.h"
#include "private/taskLibP.h"
#include "private/kernelLibP.h"
#include "private/workQLibP.h"
#include "private/windLibP.h"


/* external function declarations */
#if CPU_FAMILY==I960
void taskRegsStackToTcb (WIND_TCB *pTcb);
void taskRegsTcbToStack (WIND_TCB *pTcb);
#endif

/* forward static functions */

static BOOL taskNameNoMatch (Q_NODE *pNode, char *name);


/*******************************************************************************
*
* taskOptionsSet - change task options
*
* This routine changes the execution options of a task.
* The only option that can be changed after a task has been created is:
* .iP VX_UNBREAKABLE 21
* do not allow breakpoint debugging.
* .LP
* For definitions, see taskLib.h.
*
* RETURNS: OK, or ERROR if the task ID is invalid.
*
* SEE ALSO: taskOptionsGet()
*/

STATUS taskOptionsSet
    (
    int tid,                    /* task ID */
    int mask,                   /* bit mask of option bits to unset */
    int newOptions              /* bit mask of option bits to set */
    )
    {
    FAST WIND_TCB *pTcb;

    if (INT_RESTRICT () != OK)			/* restrict interrupt use */
	return (ERROR);

    taskLock ();				/* LOCK PREEMPTION */

    if ((pTcb = taskTcb (tid)) == NULL)		/* check task ID validity */
	{
	taskUnlock ();				/* UNLOCK PREEMPTION */
	return (ERROR);
	}

    /* update the task options */

    pTcb->options = (pTcb->options & ~mask) | newOptions;

    /* 
     * If we are setting/resetting unbreakable option for current task,
     * then call breakpoint callout.
     */

    if (((mask & VX_UNBREAKABLE) || (newOptions & VX_UNBREAKABLE)) &&
	((tid == 0) || (tid == (int)taskIdCurrent)) &&
	(taskBpHook != NULL))
	{
	(* taskBpHook) (tid);
	}

    taskUnlock ();				/* UNLOCK PREEMPTION */

    return (OK);
    }

/*******************************************************************************
*
* taskOptionsGet - examine task options
*
* This routine gets the current execution options of the specified task.
* The option bits returned by this routine indicate the following modes:
* .iP VX_FP_TASK 22
* execute with floating-point coprocessor support.
* .iP VX_PRIVATE_ENV
* include private environment support (see envLib).
* .iP VX_NO_STACK_FILL
* do not fill the stack for use by checkstack().
* .iP VX_UNBREAKABLE
* do not allow breakpoint debugging.
* .LP
* For definitions, see taskLib.h.
*
* RETURNS: OK, or ERROR if the task ID is invalid.
*
* SEE ALSO: taskOptionsSet()
*/

STATUS taskOptionsGet
    (
    int tid,                    /* task ID */
    int *pOptions               /* task's options */
    )
    {
    WIND_TCB *pTcb = taskTcb (tid);		/* get pointer to tcb */

    if (pTcb == NULL)				/* invalid task ID */
	return (ERROR);

    *pOptions = pTcb->options;			/* fill in the options */

    return (OK);
    }

/*******************************************************************************
*
* taskBpHookSet - set breakpoint hook for dbgLib
*
* This routine allows dbgLib to install its break-point install/remove routine
* used by taskOptionsSet.  It should only be called by dbgInit().
*
* NOMANUAL
*/

void taskBpHookSet
    (
    FUNCPTR bpHook
    )
    {
    taskBpHook = bpHook;
    }

/*******************************************************************************
*
* taskRegsGet - get a task's registers from the TCB
*
* This routine gathers task information kept in the TCB.  It copies the
* contents of the task's registers to the register structure <pRegs>.
*
* NOTE
* This routine only works well if the task is known to be in a stable,
* non-executing state.  Self-examination, for instance, is not advisable,
* as results are unpredictable.
*
* RETURNS: OK, or ERROR if the task ID is invalid.
*
* SEE ALSO: taskSuspend(), taskRegsSet()
*/

STATUS taskRegsGet
    (
    int         tid,    /* task ID */
    REG_SET     *pRegs  /* put register contents here */
    )
    {
    FAST WIND_TCB *pTcb = taskTcb (tid);

    if (pTcb == NULL)
	return (ERROR);

#if CPU_FAMILY==I960
    if (tid != taskIdSelf ())
    	taskRegsStackToTcb (pTcb);
#endif	/* CPU_FAMILY==I960 */

    if (pTcb->pExcRegSet != NULL)
	{
	bcopy ((char *)pTcb->pExcRegSet, (char *)&pTcb->regs, sizeof (REG_SET));
        pTcb->pExcRegSet = NULL;
	}

    bcopy ((char *) &pTcb->regs, (char *) pRegs, sizeof (REG_SET));

    return (OK);
    }

/*******************************************************************************
*
* taskRegsSet - set a task's registers
*
* This routine loads a specified register set <pRegs> into a specified
* task's TCB.
*
* NOTE
* This routine only works well if the task is known not to be in the ready
* state.  Suspending the task before changing the register set is
* recommended.
*
* RETURNS: OK, or ERROR if the task ID is invalid.
*
* SEE ALSO: taskSuspend(), taskRegsGet()
*/

STATUS taskRegsSet
    (
    int         tid,    /* task ID */
    REG_SET     *pRegs  /* get register contents from here */
    )
    {
    FAST WIND_TCB *pTcb = taskTcb (tid);

    if (pTcb == NULL)
	return (ERROR);

    bcopy ((char *) pRegs, (char *) &pTcb->regs, sizeof (REG_SET));

#if CPU_FAMILY==I960
    taskRegsTcbToStack (pTcb);
#endif	/* CPU_FAMILY==I960 */

    return (OK);
    }
/*******************************************************************************
*
* taskName - get the name associated with a task ID
*
* This routine returns a pointer to the name of a task of a specified ID, if
* the task has a name.  If the task has no name, it returns an empty string. 
*
* RETURNS: A pointer to the task name, or NULL if the task ID is invalid.
*/

char *taskName
    (
    int tid             /* ID of task whose name is to be found */
    )
    {
    WIND_TCB *pTcb = taskTcb (tid);

    if (pTcb ==  NULL)
	return ((char *) NULL);

    if (pTcb->name == NULL)
	return ("\0");
    else
	return (pTcb->name);
    }

/*******************************************************************************
*
* taskNameToId - look up the task ID associated with a task name
*
* This routine returns the ID of the task matching a specified name.
* Referencing a task in this way is inefficient, since it involves a search
* of the task list.
*
* RETURNS: The task ID, or ERROR if the task is not found.
*
* ERRNO: S_taskLib_NAME_NOT_FOUND
*
*/

int taskNameToId
    (
    char *name          /* task name to look up */
    )
    {
    int tid = (int) Q_EACH (&activeQHead, taskNameNoMatch, name);

    if (tid == (int)NULL)				/* no match found */
	{
	errno = S_taskLib_NAME_NOT_FOUND;
	return (ERROR);
	}

    return (tid - OFFSET (WIND_TCB, activeNode));
    }

/*******************************************************************************
*
* taskNameNoMatch - boolean function checks that a node's name and name differ
*
* This local routine is utilized by taskNameToId() as the qEach() boolean
* function to test for a node matching some name.
*
* RETURNS: TRUE if node's name and specified name differ, FALSE otherwise.
*
* NOMANUAL
*/

LOCAL BOOL taskNameNoMatch
    (
    Q_NODE *pNode,      /* active node to compare name with */
    char   *name        /* name to compare to */
    )
    {
    WIND_TCB *pTcb = (WIND_TCB *) ((int)pNode - OFFSET (WIND_TCB, activeNode));

    return ((pTcb->name == NULL) || (strcmp (pTcb->name, name) != 0));
    }

/*******************************************************************************
*
* taskIdDefault - set the default task ID
*
* This routine maintains a global default task ID.  This ID is used by
* libraries that want to allow a task ID argument to take on a default value
* if the user did not explicitly supply one.
*
* If <tid> is not zero (i.e., the user did specify a task ID), the default
* ID is set to that value, and that value is returned.  If <tid> is zero
* (i.e., the user did not specify a task ID), the default ID is not changed
* and its value is returned.  Thus the value returned is always the last
* task ID the user specified.
*
* RETURNS: The most recent non-zero task ID.
*
* SEE ALSO: dbgLib,
* .pG "Target Shell,"
* windsh,
* .tG "Shell"
*/

int taskIdDefault
    (
    int tid             /* user supplied task ID; if 0, return default */
    )
    {
    static int defaultTaskId;	/* current default task ID */

    if (tid != 0)
	defaultTaskId = tid;		/* update default */

    return (defaultTaskId);
    }

/*******************************************************************************
*
* taskIsReady - check if a task is ready to run
*
* This routine tests the status field of a task to determine
* if it is ready to run.
*
* RETURNS: TRUE if the task is ready, otherwise FALSE.
*/

BOOL taskIsReady
    (
    int tid     /* task ID */
    )
    {
    FAST WIND_TCB *pTcb = taskTcb (tid);

    return ((pTcb != NULL) && (pTcb->status == WIND_READY));
    }

/*******************************************************************************
*
* taskIsSuspended - check if a task is suspended
*
* This routine tests the status field of a task to determine
* if it is suspended.
*
* RETURNS: TRUE if the task is suspended, otherwise FALSE.
*/

BOOL taskIsSuspended
    (
    int tid     /* task ID */
    )
    {
    WIND_TCB *pTcb = taskTcb (tid);

    return ((pTcb != NULL) && (pTcb->status & WIND_SUSPEND));
    }

/*******************************************************************************
*
* taskIdListGet - get a list of active task IDs
*
* This routine provides the calling task with a list of all active
* tasks.  An unsorted list of task IDs for no more than <maxTasks> tasks is
* put into <idList>.
*
* WARNING:
* Kernel rescheduling is disabled with taskLock() while tasks are filled
* into the <idList>.  There is no guarantee that all the tasks are valid
* or that new tasks have not been created by the time this routine returns.
*
* RETURNS: The number of tasks put into the ID list.
*/

int taskIdListGet
    (
    int idList[],               /* array of task IDs to be filled in */
    int maxTasks                /* max tasks <idList> can accommodate */
    )
    {
    FAST int ix;
    int      active;

    taskLock ();				/* LOCK PREEMPTION */

    active = Q_INFO (&activeQHead, idList, maxTasks);

    taskUnlock ();				/* UNLOCK PREEMPTION */

    /* fix up the ID list by lopping off the the offset to the activeNode for
     * each element in the idList.
     */

    for (ix = 0; ix < active; ix ++)
	idList [ix] -= OFFSET (WIND_TCB, activeNode);

    return (active);				/* return idList count */
    }
