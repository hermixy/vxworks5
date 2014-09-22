/* taskHookLib.c - task hook library */

/* Copyright 1984-1995 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01z,14mar99,jdi  doc: removed refs to config.h and/or configAll.h (SPR 25663).
01y,27may95,p_m  added _func_taskCreateHookAdd and _func_taskDeleteHookAdd
		 initialization in taskHookInit().
01x,11feb95,jdi  doc format repair in taskSwitchHookAdd().
01w,09dec94,rhp  add list of fns callable from task-switch hooks (SPR 2206)
01v,02feb93,jdi  documentation tweak for configuration.
01u,21jan93,jdi  documentation cleanup for 5.1.
01t,18jul92,smb  Changed errno.h to errnoLib.h.
01s,04jul92,jcf  scalable/ANSI/cleanup effort.
01r,26may92,rrr  the tree shuffle
01q,13dec91,gae  ANSI cleanup.
01p,19nov91,rrr  shut up some ansi warnings.
01o,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed VOID to void
		  -changed copyright notice
01n,05apr91,jdi	 documentation -- removed header parens and x-ref numbers;
		 doc review by dnw.
01m,24mar91,jdi  documentation cleanup.
01l,28sep90,jcf  documentation.
01k,10aug90,kdl  added forward declaration for taskHookShow.
01j,02aug90,jcf  documentation.
01i,10dec89,jcf  symbol table type now SYM_TYPE.
		 removed tcb extension dependencies.
		 added swap hook support.
01h,23jul89,gae  added task{Create,Switch,Delete}HookShow.
01g,08apr89,dnw  changed to store delete hooks in reverse order in the table.
01f,17aug88,gae  documentation.
01e,22jun88,dnw  name tweaks.
01d,05jun88,dnw  changed from hookLib to taskHookLib.
01c,30may88,dnw  changed to v4 names.
01b,28may88,dnw  removed hookAddRebootRtn to rebootLib.c
		 changed some status value names.
01a,25jan88,jcf  written by extracting from vxLib.c.
*/

/*
DESCRIPTION
This library provides routines for adding extensions to the VxWorks
tasking facility.  To allow task-related facilities to be added to the
system without modifying the kernel, the kernel provides call-outs every
time a task is created, switched, or deleted.  The call-outs allow additional
routines, or "hooks," to be invoked whenever these events occur.
The hook management routines below allow hooks to be dynamically added to
and deleted from the current lists of create, switch, and delete hooks:
.iP "taskCreateHookAdd() and taskCreateHookDelete()" 10
Add and delete routines to be called when a task is created.
.iP "taskSwitchHookAdd() and taskSwitchHookDelete()"
Add and delete routines to be called when a task is switched.
.iP "taskDeleteHookAdd() and taskDeleteHookDelete()"
Add and delete routines to be called when a task is deleted.
.LP
This facility is used by dbgLib to provide task-specific breakpoints
and single-stepping.  It is used by taskVarLib for the "task variable"
mechanism.  It is also used by fppLib for floating-point coprocessor
support.

NOTE
It is possible to have dependencies among task hook routines.  For
example, a delete hook may use facilities that are cleaned up and deleted
by another delete hook.  In such cases, the order in which the hooks run
is important.  VxWorks runs the create and switch hooks in the order in
which they were added, and runs the delete hooks in reverse of the order
in which they were added.  Thus, if the hooks are added in "hierarchical"
order, such that they rely only on facilities whose hook routines have
already been added, then the required facilities will be initialized
before any other facilities need them, and will be deleted after all
facilities are finished with them.

VxWorks facilities guarantee this by having each facility's initialization
routine first call any prerequisite facility's initialization routine
before adding its own hooks.  Thus, the hooks are always added in the
correct order.  Each initialization routine protects itself from multiple
invocations, allowing only the first invocation to have any effect.

INCLUDE FILES: taskHookLib.h

SEE ALSO: dbgLib, fppLib, taskLib, taskVarLib
.pG "Basic OS"
*/

#include "vxWorks.h"
#include "errnoLib.h"
#include "taskHookLib.h"
#include "private/funcBindP.h"
#include "private/taskLibP.h"


/* forward static functions */

static STATUS taskHookAdd (FUNCPTR hook, FUNCPTR table [], int maxEntries);
static STATUS taskHookDelete (FUNCPTR hook, FUNCPTR table [], int maxEntries);
static STATUS taskSwapMaskClear (int tid, int index, BOOL in, BOOL out);
static STATUS taskSwapMaskSet (int tid, int index, BOOL in, BOOL out);


/*******************************************************************************
*
* taskHookInit - initialize task hook facilities
*
* This routine is a NULL routine called to configure the task hook package
* into the system.  It is called automatically if the configuration macro
* INCLUDE_TASK_HOOKS is defined.
*
* RETURNS: N/A
*/

void taskHookInit (void)
    {
    _func_taskCreateHookAdd = (FUNCPTR) taskCreateHookAdd;
    _func_taskDeleteHookAdd = (FUNCPTR) taskDeleteHookAdd;
    }
/*******************************************************************************
*
* taskCreateHookAdd - add a routine to be called at every task create
*
* This routine adds a specified routine to a list of routines
* that will be called whenever a task is created.  The routine should be
* declared as follows:
* .CS
*     void createHook
*         (
*         WIND_TCB *pNewTcb	/@ pointer to new task's TCB @/
*         )
* .CE
*
* RETURNS: OK, or ERROR if the table of task create routines is full.
*
* SEE ALSO: taskCreateHookDelete()
*/

STATUS taskCreateHookAdd
    (
    FUNCPTR createHook  /* routine to be called when a task is created */
    )
    {
    return (taskHookAdd (createHook, taskCreateTable, VX_MAX_TASK_CREATE_RTNS));
    }
/*******************************************************************************
*
* taskCreateHookDelete - delete a previously added task create routine
*
* This routine removes a specified routine from the list of
* routines to be called at each task create.
*
* RETURNS: OK, or ERROR if the routine is not in the table of task create
* routines.
*
* SEE ALSO: taskCreateHookAdd()
*/

STATUS taskCreateHookDelete
    (
    FUNCPTR createHook          /* routine to be deleted from list */
    )
    {
    return (taskHookDelete (createHook, taskCreateTable,
			    VX_MAX_TASK_CREATE_RTNS));
    }
/*******************************************************************************
*
* taskSwitchHookAdd - add a routine to be called at every task switch
*
* This routine adds a specified routine to a list of routines
* that will be called at every task switch.  The routine should be
* declared as follows:
* .CS
*     void switchHook
*         (
*         WIND_TCB *pOldTcb,	/@ pointer to old task's WIND_TCB @/
*         WIND_TCB *pNewTcb	/@ pointer to new task's WIND_TCB @/
*         )
* .CE
*
* NOTE
* User-installed switch hooks are called within the kernel context.
* Therefore, switch hooks do not have access to all VxWorks
* facilities.  The following routines can be called from within a task
* switch hook:
* 
* .TS
* tab(|);
* lf3 lf3
* l   l .
* Library    |  Routines
* _
* bLib       |  All routines
* fppArchLib |  fppSave(), fppRestore()
* intLib     |  intContext(), intCount(), intVecSet(), intVecGet()
* lstLib     |  All routines
* mathALib   |  All routines, if fppSave()/fppRestore() are used
* rngLib     |  All routines except rngCreate()
* taskLib    |  taskIdVerify(), taskIdDefault(), taskIsReady(),
*            |  taskIsSuspended(), taskTcb()
* vxLib      |  vxTas()
* .TE
* 
* RETURNS: OK, or ERROR if the table of task switch routines is full.
*
* SEE ALSO: taskSwitchHookDelete()
*/

STATUS taskSwitchHookAdd
    (
    FUNCPTR switchHook  /* routine to be called at every task switch */
    )
    {
    return (taskHookAdd (switchHook, taskSwitchTable, VX_MAX_TASK_SWITCH_RTNS));
    }
/*******************************************************************************
*
* taskSwitchHookDelete - delete a previously added task switch routine
*
* This routine removes the specified routine from the list of
* routines to be called at each task switch.
*
* RETURNS: OK, or ERROR if the routine is not in the table of task switch
* routines.
*
* SEE ALSO: taskSwitchHookAdd()
*/

STATUS taskSwitchHookDelete
    (
    FUNCPTR switchHook          /* routine to be deleted from list */
    )
    {
    return (taskHookDelete (switchHook, taskSwitchTable,
			    VX_MAX_TASK_SWITCH_RTNS));
    }
/*******************************************************************************
*
* taskSwapHookAdd - add routine to be called at every task switch
*
* This routine adds the specified routine to a list of routines
* that get called at every task switch.  The routine should be
* declared as follows:
* .CS
*	void swapHook (pOldTcb, pNewTcb)
*	    WIND_TCB *pOldTcb;	/@ pointer to old task's WIND_TCB @/
*	    WIND_TCB *pNewTcb;	/@ pointer to new task's WIND_TCB @/
* .CE
*
* RETURNS: OK, or ERROR if table of task switch routines is full.
*
* SEE ALSO: taskSwapHookDelete()
*
* NOMANUAL
*/

STATUS taskSwapHookAdd
    (
    FUNCPTR swapHook    /* routine to be called at every task switch */
    )
    {
    FAST int ix;

    taskLock ();			/* disable task switching */

    /* find slot after last hook in table */

    for (ix = 0; ix < VX_MAX_TASK_SWAP_RTNS; ++ix)
	{
	if (taskSwapTable[ix] == NULL)
	    {
	    taskSwapTable[ix]     = swapHook;
	    taskSwapReference[ix] = 0;
	    taskUnlock ();		/* re-enable task switching */
	    return (OK);
	    }
	}

    /* no free slot found */

    taskUnlock ();		/* re-enable task switching */

    errnoSet (S_taskLib_TASK_HOOK_TABLE_FULL);
    return (ERROR);
    }
/*******************************************************************************
*
* taskSwapHookAttach - attach a task to a swap routine
*
* A swap hook is only called for a task that has attached to it.  If task
* attaches to the swap hook as <in>, then the hook will be called every time
* the task is swapped in.  If the task attaches to the swap hook as <out>, then
* the hook will be called every time the task is swapped out.
*
* RETURNS: OK or ERROR.
*
* NOMANUAL
*/

STATUS taskSwapHookAttach
    (
    FUNCPTR     swapHook,       /* swap hook for conection */
    int         tid,            /* task to connect to swap hook */
    BOOL        in,             /* conection for swap in */
    BOOL        out             /* conection for swap out */
    )
    {
    int ix;

    taskLock ();			/* disable task switching */

    /* find hook in hook table */

    for (ix = 0; ix < VX_MAX_TASK_SWAP_RTNS; ++ix)
	{
	if (taskSwapTable [ix] == swapHook)
	    {
	    taskSwapReference[ix] += (in)  ? 1 : 0; /* reference swap hook */
	    taskSwapReference[ix] += (out) ? 1 : 0; /* reference swap hook */

	    if (taskSwapMaskSet (tid, ix, in, out) != OK)
		{
		taskSwapReference[ix] -= (in)  ? 1 : 0; /* deref. swap hook */
		taskSwapReference[ix] -= (out) ? 1 : 0; /* deref. swap hook */
		taskUnlock ();			    /* reenable switching */
		return (ERROR);
		}
	    else
		{
		taskUnlock ();			    /* reenable switching */
		return (OK);
		}
	    }
	}

    /* hook not found in table */

    taskUnlock ();		/* re-enable task switching */

    errnoSet (S_taskLib_TASK_HOOK_NOT_FOUND);
    return (ERROR);
    }

/*******************************************************************************
*
* taskSwapHookDetach - detach a task from a swap routine
*
* A task may detach itself from a swap hook.  Once detached, the hook will not
* be called when the task is involved in a context switch.
*
* RETURNS: OK or ERROR.
*
* NOMANUAL
*/

STATUS taskSwapHookDetach
    (
    FUNCPTR     swapHook,       /* swap hook for disconection */
    int         tid,            /* task to disconnect from swap hook */
    BOOL        in,             /* conection for swap in */
    BOOL        out             /* conection for swap out */
    )
    {
    int ix;

    taskLock ();			/* disable task switching */

    /* find hook in hook table */

    for (ix = 0; ix < VX_MAX_TASK_SWAP_RTNS; ++ix)
	{
	if (taskSwapTable [ix] == swapHook)
	    {
	    if (taskSwapMaskClear (tid, ix, in, out) != OK)
		{
		taskUnlock ();			/* re-enable task switching */
		return (ERROR);			/* must of been a bum tid */
		}
	    else
		{
		taskSwapReference[ix] -= (in)  ? 1 : 0; /* deref. swap hook */
		taskSwapReference[ix] -= (out) ? 1 : 0; /* deref. swap hook */

		taskUnlock ();			/* re-enable task switching */
		return (OK);
		}
	    }
	}

    /* hook not found in table */

    taskUnlock ();		/* re-enable task switching */

    errnoSet (S_taskLib_TASK_HOOK_NOT_FOUND);
    return (ERROR);
    }
/*******************************************************************************
*
* taskSwapHookDelete - delete previously added task switch routine
*
* This routine removes the specified routine from the list of
* routines to be called by attached tasks during context switch.
*
* RETURNS: OK, or ERROR if the routine is not in the table of task delete
* routines.
*
* SEE ALSO: taskSwapHookAdd()
*
* NOMANUAL
*/

STATUS taskSwapHookDelete
    (
    FUNCPTR swapHook            /* routine to be deleted from list */
    )
    {
    int ix;

    taskLock ();			/* disable task switching */

    /* find hook in hook table */

    for (ix = 0; ix < VX_MAX_TASK_SWAP_RTNS; ++ix)
	{
	if (taskSwapTable [ix] == swapHook)
	    {
	    if (taskSwapReference [ix] != 0)	/* reference swap hook */
		{
		taskUnlock ();			/* re-enable task switching */
		errnoSet (S_taskLib_TASK_SWAP_HOOK_REFERENCED);
		return (ERROR);
		}
	    else
		{
		taskSwapTable [ix] = NULL; 	/* take out of table */
		taskUnlock ();			/* re-enable task switching */
		return (OK);
		}
	    }
	}

    /* hook not found in table */

    taskUnlock ();		/* re-enable task switching */

    errnoSet (S_taskLib_TASK_HOOK_NOT_FOUND);
    return (ERROR);
    }
/*******************************************************************************
*
* taskDeleteHookAdd - add a routine to be called at every task delete
*
* This routine adds a specified routine to a list of routines
* that will be called whenever a task is deleted.  The routine should be
* declared as follows:
* .CS
*     void deleteHook
*         (
*         WIND_TCB *pTcb	/@ pointer to deleted task's WIND_TCB @/
*         )
* .CE
*
* RETURNS: OK, or ERROR if the table of task delete routines is full.
*
* SEE ALSO: taskDeleteHookDelete()
*
* INTERNAL:
* Unlike the other "hook add" routines, this routine keeps the delete hooks in
* the table in REVERSE of the order in which they are added.  Thus the most
* recently added hook is the FIRST entry in the table.  This causes the delete
* hooks to be run in the reverse order when a task is deleted.  This is
* necessary since a task delete hook may depend on another facility that
* has a delete hook, e.g., stdio uses task variables.  If we ensure that the
* delete hooks are added in hierarchical order (i.e., any delete hook only
* uses facilities whose delete hooks have already been added), then running
* them in reverse order guarantees that facilities are not cleaned up
* prematurely.
*/

STATUS taskDeleteHookAdd
    (
    FUNCPTR deleteHook  /* routine to be called when a task is deleted */
    )
    {
    FAST int ix;
    STATUS status = OK;

    taskLock ();			/* disable task switching */

    if (taskDeleteTable [VX_MAX_TASK_DELETE_RTNS] != NULL)
	{
	/* no free slot found */

	errnoSet (S_taskLib_TASK_HOOK_TABLE_FULL);
	status = ERROR;
	}
    else
	{
	/* move all the hooks down one slot in the table */

	for (ix = VX_MAX_TASK_DELETE_RTNS - 2; ix >= 0; --ix)
	    taskDeleteTable [ix + 1] = taskDeleteTable [ix];

	taskDeleteTable [0] = deleteHook;
	}

    taskUnlock ();			/* re-enable task switching */

    return (status);
    }
/*******************************************************************************
*
* taskDeleteHookDelete - delete a previously added task delete routine
*
* This routine removes a specified routine from the list of
* routines to be called at each task delete.
*
* RETURNS: OK, or ERROR if the routine is not in the table of task delete
* routines.
*
* SEE ALSO: taskDeleteHookAdd()
*/

STATUS taskDeleteHookDelete
    (
    FUNCPTR deleteHook          /* routine to be deleted from list */
    )
    {
    return (taskHookDelete (deleteHook, taskDeleteTable,
			    VX_MAX_TASK_DELETE_RTNS));
    }

/*******************************************************************************
*
* taskHookAdd - add a hook routine to a hook table
*
* This routine does not guard against duplicate entries.
*
* RETURNS: OK, or ERROR if task hook table is full.
*/

LOCAL STATUS taskHookAdd
    (
    FUNCPTR hook,       /* routine to be added to table */
    FUNCPTR table[],    /* table to which to add */
    int maxEntries      /* max entries in table */
    )
    {
    FAST int ix;

    taskLock ();			/* disable task switching */

    /* find slot after last hook in table */

    for (ix = 0; ix < maxEntries; ++ix)
	{
	if (table[ix] == NULL)
	    {
	    table[ix] = hook;
	    taskUnlock ();	/* re-enable task switching */
	    return (OK);
	    }
	}

    /* no free slot found */

    taskUnlock ();		/* re-enable task switching */

    errnoSet (S_taskLib_TASK_HOOK_TABLE_FULL);
    return (ERROR);
    }
/*******************************************************************************
*
* taskHookDelete - delete a hook from a hook table
*
* RETURNS: OK, or ERROR if task hook could not be found.
*/

LOCAL STATUS taskHookDelete
    (
    FUNCPTR hook,       /* routine to be deleted from table */
    FUNCPTR table[],    /* table from which to delete */
    int maxEntries      /* max entries in table */
    )
    {
    FAST int ix;

    taskLock ();			/* disable task switching */

    /* find hook in hook table */

    for (ix = 0; ix < maxEntries; ++ix)
	{
	if (table [ix] == hook)
	    {
	    /* move all the remaining hooks up one slot in the table */

	    do
		table [ix] = table [ix + 1];
	    while (table [++ix] != NULL);

	    taskUnlock ();	/* re-enable task switching */
	    return (OK);
	    }
	}

    /* hook not found in table */

    taskUnlock ();		/* re-enable task switching */

    errnoSet (S_taskLib_TASK_HOOK_NOT_FOUND);
    return (ERROR);
    }

/*******************************************************************************
*
* taskSwapMaskSet - set a task's swap-in/swap-out mask
*
* This routine sets the bit for the specified index in the specified task's
* swap-in/swap-out mask.  The parameter index is the index into the
* taskSwapTable for the swap hook of interest.
*
* INTERNAL
* The most significant bit corresponds to the first element of the
* swap table.  This was done because optimized versions of the kernel
* utilize the fact that the most significant bit sets the carry flag on a
* shift left operation.
*
* RETURNS: OK, or ERROR if the swap mask could not be set.
*
* NOMANUAL
*/

LOCAL STATUS taskSwapMaskSet
    (
    int  tid,   /* task to change swap-in mask */
    int  index, /* index of task swap routine in taskSwapTable */
    BOOL in,    /* call swap routine when task is switched in */
    BOOL out    /* call swap routine when task is switched out */
    )
    {
    WIND_TCB *pTcb = taskTcb (tid);
    USHORT    ixBit = (1 << (15 - index));

    if (pTcb == NULL)				/* invalid task ID */
	return (ERROR);

    if (((in) && (pTcb->swapInMask & ixBit)) ||
        ((out) && (pTcb->swapOutMask & ixBit)))
	{
	errno = S_taskLib_TASK_SWAP_HOOK_SET;
	return (ERROR);
	}

    if (in)
	pTcb->swapInMask |= ixBit;		/* turn on swapInMask bit */

    if (out)
	pTcb->swapOutMask |= ixBit;		/* turn on swapOutMask bit */

    return (OK);
    }

/*******************************************************************************
*
* taskSwapMaskClear - clear a task's swap-in/swap-out mask
*
* This routine clears the bit for the specified index in the specified task's
* swap-in/swap-out mask.  The parameter index is the index into the
* taskSwapTable for the swap hook of interest.
*
* INTERNAL
* The most significant bit corresponds to the first element of the
* swap table.  This was done for kernel efficiency.
*
* RETURNS: OK, or ERROR if the swap mask could not be cleared.
*
* NOMANUAL
*/

LOCAL STATUS taskSwapMaskClear
    (
    int  tid,   /* task to change swap-in mask */
    int  index, /* index of task swap routine in taskSwapTable */
    BOOL in,    /* swap routine called when task is switched in */
    BOOL out    /* swap routine called when task is switched out */
    )
    {
    WIND_TCB *pTcb  = taskTcb (tid);
    USHORT    ixBit = (1 << (15 - index));

    if (pTcb == NULL)				/* invalid task ID */
	return (ERROR);


    if (((in) && !(pTcb->swapInMask & ixBit)) ||
        ((out) && !(pTcb->swapOutMask & ixBit)))
	{
	errno = S_taskLib_TASK_SWAP_HOOK_CLEAR;
	return (ERROR);
	}

    if (in)
	pTcb->swapInMask &= ~ixBit;		/* turn off swapInMask bit */

    if (out)
	pTcb->swapOutMask &= ~ixBit;		/* turn off swapOutMask bit */

    return (OK);
    }

