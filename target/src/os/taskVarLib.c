/* taskVarLib.c - task variables support library */

/* Copyright 1984-1993 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01z,21jan93,jdi  documentation cleanup for 5.1.
01y,02oct92,jcf  added task validity check to switch hook.
01x,18jul92,smb  changed errno.h to errnoLib.h.
01w,07jul92,ajm  removed unnecesary cacheClear calls for 040
01v,04jul92,jcf  scalable/ANSI/cleanup effort.
01u,03jul92,jwt  converted cacheClearEntry() calls to cacheClear() for 5.1.
01t,26may92,rrr  the tree shuffle
01s,21dec91,gae  added includes for ANSI.
01r,19nov91,rrr  shut up some ansi warnings.
01q,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -fixed #else and #endif
		  -changed VOID to void
		  -changed copyright notice
01p,27aug91,shl  added cache coherency calls for MC68040 support.
01o,220may1,jdi	 documentation tweak.
01n,05apr91,jdi	 documentation -- removed header parens and x-ref numbers;
		 doc review by dnw.
01m,24mar91,jdi  documentation cleanup.
01l,01oct90,jcf  added taskVarInfo().
01k,01aug90,jcf  changed tcb taskVar to pTaskVar.
		 added include of taskVarLib.h.
01j,19jul90,dnw  mangen fix
01i,14apr90,jcf  removed tcb extension dependencies.
01h,14mar90,jdi  documentation cleanup.
01g,08apr89,dnw  added taskVarInit().
01f,17aug88,gae  documentation.
01d,22jun88,dnw  name tweaks.
01c,30may88,dnw  changed to v4 names.
01b,08apr88,gae  added taskId parm. to taskVar{Add,Del}();
		 made taskVar{Get,Set}() work with active task.
		 Fixed fatal bug in taskVarDel() of not replacing bg value.
		 Added taskVarDeleteHook() to cleanup after tasks.
		 Lint. Documentation.
01a,25jan88,jcf  written by extracting from vxLib.c.
*/

/*
DESCRIPTION
VxWorks provides a facility called "task variables," which allows
4-byte variables to be added to a task's context, and the
variables' values to be switched each time a task switch occurs to or
from the calling task.  Typically, several tasks declare the same
variable (4-byte memory location) as a task variable and treat that
memory location as their own private variable.  For example, this
facility can be used when a routine must be spawned more than once as
several simultaneous tasks.

The routines taskVarAdd() and taskVarDelete() are used to add or delete
a task variable.  The routines taskVarGet() and taskVarSet() are used to get
or set the value of a task variable.

NOTE
If you are using task variables in a task delete hook
(see taskHookLib), refer to the manual entry for taskVarInit()
for warnings on proper usage.

INCLUDE FILES: taskVarLib.h

SEE ALSO: taskHookLib,
.pG "Basic OS"
*/

/* LINTLIBRARY */

#include "vxWorks.h"
#include "stdlib.h"
#include "taskHookLib.h"
#include "taskVarLib.h"
#include "taskLib.h"
#include "errnoLib.h"

/* forward static functions */

static void taskVarDeleteHook (WIND_TCB *pTcb);
static void taskVarSwitchHook (WIND_TCB *pOldTcb, WIND_TCB *pNewTcb);


/*******************************************************************************
*
* taskVarInit - initialize the task variables facility
*
* This routine initializes the task variables facility.  It installs task
* switch and delete hooks used for implementing task variables.
* If taskVarInit() is not called explicitly, taskVarAdd() will call it
* automatically when the first task variable is added.
*
* After the first invocation of this routine, subsequent invocations
* have no effect.
*
* WARNING
* Order dependencies in task delete hooks often involve
* task variables.  If a facility uses task variables and has a
* task delete hook that expects to use those task variables,
* the facility's delete hook must run before the task
* variables' delete hook.  Otherwise, the task variables
* will be deleted by the time the facility's delete hook runs.
*
* VxWorks is careful to run the delete hooks in reverse of the order in
* which they were installed.  Any facility that has a delete hook that will
* use task variables can guarantee proper ordering by calling taskVarInit()
* before adding its own delete hook.
*
* Note that this is not an issue in normal use of task variables.  The issue
* only arises when adding another task delete hook that uses task variables.
*
* Caution should also be taken when adding task variables from within
* create hooks.  If the task variable package has not been installed via
* taskVarInit(), the create hook attempts to create a create hook, and that
* may cause system failure.  To avoid this situation, taskVarInit() should
* be called during system initialization from the root task, usrRoot(), in
* usrConfig.c.
*
* RETURNS: OK, or ERROR if the task switch/delete hooks could not be installed.
*/

STATUS taskVarInit (void)
    {
    static BOOL taskVarInstalled = FALSE;	/* TRUE = facility installed */

    /* if task variables facility is not already installed, then install it
     * by adding the switch and delete hooks
     */

    if (!taskVarInstalled)
	{
	if ((taskSwitchHookAdd ((FUNCPTR)taskVarSwitchHook) != OK) ||
	    (taskDeleteHookAdd ((FUNCPTR)taskVarDeleteHook) != OK))
	    {
	    return (ERROR);
	    }

	taskVarInstalled = TRUE;
	}

    return (OK);
    }

/*******************************************************************************
*
* taskVarDeleteHook - delete task variables of exiting tasks
*
* This routine is the task delete routine that deletes all task
* variables of an exiting task.
*/

LOCAL void taskVarDeleteHook
    (
    WIND_TCB *pTcb
    )
    {
    FAST TASK_VAR *pTaskVar;
    FAST TASK_VAR *pTaskVarNext;

    for (pTaskVar = pTcb->pTaskVar;
	 pTaskVar != NULL;
	 pTaskVar = pTaskVarNext)
	{
	pTaskVarNext = pTaskVar->next;
	free ((char *)pTaskVar);	/* free storage of deleted cell */
	}
    }
/*******************************************************************************
*
* taskVarSwitchHook - switch task variables of switching tasks
*
* This routine is the task switch routine that implements the task variable
* facility.  It swaps the current and saved values of all the task variables
* of the out-going and in-coming tasks.
*/

LOCAL void taskVarSwitchHook
    (
    WIND_TCB *pOldTcb,
    WIND_TCB *pNewTcb
    )
    {
    FAST TASK_VAR *pTaskVar;
    FAST int temp;

    /* swap task variables of old task */

    if (TASK_ID_VERIFY(pOldTcb) == OK)	/* suicide runs delete hook 1st */
	{
	for (pTaskVar = pOldTcb->pTaskVar;
	     pTaskVar != NULL;
	     pTaskVar = pTaskVar->next)
	    {
	    /* swap current and save value of task variable */

	    temp = pTaskVar->value;
	    pTaskVar->value = *(pTaskVar->address);
	    *(pTaskVar->address) = temp;
	    }
	}

    /* swap task variables of new task */

    for (pTaskVar = pNewTcb->pTaskVar;
	 pTaskVar != NULL;
	 pTaskVar = pTaskVar->next)
	{
	/* swap current and save value of task variable */

	temp = pTaskVar->value;
	pTaskVar->value = *(pTaskVar->address);
	*(pTaskVar->address) = temp;
	}
    }
/*******************************************************************************
*
* taskVarAdd - add a task variable to a task
*
* This routine adds a specified variable <pVar> (4-byte memory location) to a
* specified task's context.  After calling this routine, the variable will
* be private to the task.  The task can access and modify the variable, but
* the modifications will not appear to other tasks, and other tasks'
* modifications to that variable will not affect the value seen by the
* task.  This is accomplished by saving and restoring the variable's initial
* value each time a task switch occurs to or from the calling task.
*
* This facility can be used when a routine is to be spawned repeatedly as
* several independent tasks.  Although each task will have its own stack,
* and thus separate stack variables, they will all share the same static and
* global variables.  To make a variable \f2not\fP shareable, the routine can
* call taskVarAdd() to make a separate copy of the variable for each task, but
* all at the same physical address.
*
* Note that task variables increase the task switch time to and from the
* tasks that own them.  Therefore, it is desirable to limit the number of
* task variables that a task uses.  One efficient way to use task variables 
* is to have a single task variable that is a pointer to a dynamically 
* allocated structure containing the task's private data.
*
* EXAMPLE:
* Assume that three identical tasks were spawned with a routine called
* \f2operator()\f1.  All three use the structure OP_GLOBAL for all variables
* that are specific to a particular incarnation of the task.  The following
* code fragment shows how this is set up:
*
* .CS
* OP_GLOBAL *opGlobal;  /@ ptr to operator task's global variables @/
*
* void operator
*     (
*     int opNum         /@ number of this operator task @/
*     )
*     {
*     if (taskVarAdd (0, (int *)&opGlobal) != OK)
*         {
*         printErr ("operator%d: can't taskVarAdd opGlobal\en", opNum);
*         taskSuspend (0);
*         }
*
*     if ((opGlobal = (OP_GLOBAL *) malloc (sizeof (OP_GLOBAL))) == NULL)
*         {
*         printErr ("operator%d: can't malloc opGlobal\en", opNum);
*         taskSuspend (0);
*         }
*     ...
*     }
* .CE
*
* RETURNS:
* OK, or ERROR if memory is insufficient for the task variable descriptor.
*
* INTERNAL
* The first time this routine is called the task switch and delete
* routines, taskVarSwitchHook() and taskVarDeleteHook(), are installed.
*
* SEE ALSO: taskVarDelete(), taskVarGet(), taskVarSet()
*/

STATUS taskVarAdd
    (
    int tid,    /* ID of task to have new variable */
    int *pVar   /* pointer to variable to be switched for task */
    )
    {
    FAST WIND_TCB *pTcb = taskTcb (tid);
    FAST TASK_VAR *pTaskVar;

    /* make sure task variable facility is installed */

    if (taskVarInit () != OK)
	return (ERROR);


    /* allocate descriptor for new task variable */

    pTaskVar = (TASK_VAR *) malloc (sizeof (TASK_VAR));

    if (pTaskVar == NULL)
	return (ERROR);

    pTaskVar->address = pVar;
    pTaskVar->value   = *pVar;


    /* link new task variable into list in stack header */

    pTaskVar->next = pTcb->pTaskVar;
    pTcb->pTaskVar = pTaskVar;

    return (OK);
    }
/*******************************************************************************
*
* taskVarDelete - remove a task variable from a task
*
* This routine removes a specified task variable, <pVar>, from the specified
* task's context.  The private value of that variable is lost.
*
* RETURNS
* OK, or
* ERROR if the task variable does not exist for the specified task.
*
* SEE ALSO: taskVarAdd(), taskVarGet(), taskVarSet()
*/

STATUS taskVarDelete
    (
    int tid,    /* ID of task whose variable is to be removed */
    int *pVar   /* pointer to task variable to be removed */
    )
    {
    FAST TASK_VAR **ppTaskVar;		/* ptr to ptr to next node */
    FAST TASK_VAR *pTaskVar;
    WIND_TCB *pTcb = taskTcb (tid);

    if (pTcb == NULL)			/* check that task is valid */
	return (ERROR);

    /* find descriptor for specified task variable */

    for (ppTaskVar = &pTcb->pTaskVar;
	 *ppTaskVar != NULL;
	 ppTaskVar = &((*ppTaskVar)->next))
	{
	pTaskVar = *ppTaskVar;

	if (pTaskVar->address == pVar)
	    {
	    /* if active task, replace background value */

	    if (taskIdCurrent == pTcb)
		*pVar = pTaskVar->value;

	    *ppTaskVar = pTaskVar->next;/* delete variable from list */

	    free ((char *)pTaskVar);	/* free storage of deleted cell */

	    return (OK);
	    }
	}


    /* specified address is not a task variable for specified task */

    errnoSet (S_taskLib_TASK_VAR_NOT_FOUND);
    return (ERROR);
    }
/*******************************************************************************
*
* taskVarGet - get the value of a task variable
*
* This routine returns the private value of a task variable for a
* specified task.  The specified task is usually not the calling task,
* which can get its private value by directly accessing the variable.
* This routine is provided primarily for debugging purposes.
*
* RETURNS:
* The private value of the task variable, or
* ERROR if the task is not found or it
* does not own the task variable.
*
* SEE ALSO: taskVarAdd(), taskVarDelete(), taskVarSet()
*/

int taskVarGet
    (
    int tid,            /* ID of task whose task variable is to be retrieved */
    int *pVar           /* pointer to task variable */
    )
    {
    FAST TASK_VAR *pTaskVar;		/* ptr to next node */
    WIND_TCB *pTcb = taskTcb (tid);

    if (pTcb == NULL)
	return (ERROR);

    /* find descriptor for specified task variable */

    for (pTaskVar = pTcb->pTaskVar;
	 pTaskVar != NULL;
	 pTaskVar = pTaskVar->next)
	{
	if (pTaskVar->address == pVar)
	    return (taskIdCurrent == pTcb ? *pVar : pTaskVar->value);
	}


    /* specified address is not a task variable for specified task */

    errnoSet (S_taskLib_TASK_VAR_NOT_FOUND);
    return (ERROR);
    }
/*******************************************************************************
*
* taskVarSet - set the value of a task variable
*
* This routine sets the private value of the task variable for a specified
* task.  The specified task is usually not the calling task, which can set
* its private value by directly modifying the variable.  This routine is
* provided primarily for debugging purposes.
*
* RETURNS:
* OK, or
* ERROR if the task is not found or it
* does not own the task variable.
*
* SEE ALSO: taskVarAdd(), taskVarDelete(), taskVarGet()
*/

STATUS taskVarSet
    (
    int tid,    /* ID of task whose task variable is to be set */
    int *pVar,  /* pointer to task variable to be set for this task */
    int value   /* new value of task variable */
    )
    {
    FAST TASK_VAR *pTaskVar;	/* ptr to next node */
    WIND_TCB *pTcb = taskTcb (tid);

    if (pTcb == NULL)
	return (ERROR);

    /* find descriptor for specified task variable */

    for (pTaskVar = pTcb->pTaskVar;
	 pTaskVar != NULL;
	 pTaskVar = pTaskVar->next)
	{
	if (pTaskVar->address == pVar)
	    {
	    if (taskIdCurrent == pTcb)
		{
		*pVar = value;
		}
	    else
		pTaskVar->value = value;
	    return (OK);
	    }
	}

    /* specified address is not a task variable for specified task */

    errnoSet (S_taskLib_TASK_VAR_NOT_FOUND);
    return (ERROR);
    }
/********************************************************************************
* taskVarInfo - get a list of task variables of a task
*
* This routine provides the calling task with a list of all of the task
* variables of a specified task.  The unsorted array of task variables is
* copied to <varList>.
*
* CAVEATS
* Kernel rescheduling is disabled with taskLock() while task variables are
* looked up.  There is no guarantee that all the task variables are still
* valid or that new task variables have not been created by the time this
* routine returns.
*
* RETURNS: The number of task variables in the list.
*/

int taskVarInfo
    (
    int tid,            /* ID of task whose task variable is to be set */
    TASK_VAR varList[], /* array to hold task variable addresses */
    int maxVars         /* maximum variables varList can accommodate */
    )
    {
    FAST TASK_VAR *pTaskVar;			/* ptr to next node */
    WIND_TCB *	   pTcb	  = taskTcb (tid);
    int		   active = 0;

    if (pTcb == NULL)
	return (ERROR);

    taskLock ();				/* LOCK PREEMPTION */

    for (pTaskVar = pTcb->pTaskVar;
	 (pTaskVar != NULL) && (active < maxVars);
	 pTaskVar = pTaskVar->next, active ++)
	{
	varList[active] = *pTaskVar;
	}

    taskUnlock ();				/* UNLOCK PREEMPTION */

    return (active);
    }
