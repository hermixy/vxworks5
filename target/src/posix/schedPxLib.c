/* schedPxLib.c - scheduling library (POSIX) */

/* Copyright 1984-1995 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01d,24jan95,rhp  doc: clarify relation between struct sched_param and priority
    19jan95,jdi  doc cleanup.
01c,01feb94,dvs  documentation cleanup.
01b,05jan94,kdl  changed include to private/schedP.h; general cleanup.
01a,04nov93,dvs  written
*/

/*
DESCRIPTION: 
This library provides POSIX-compliance scheduling routines.  The routines
in this library allow the user to get and set priorities and scheduling
schemes, get maximum and minimum priority values, and get the time slice
if round-robin scheduling is enabled.

The POSIX standard specifies a priority numbering scheme in which
higher priorities are indicated by larger numbers.  The VxWorks native
numbering scheme is the reverse of this, with higher priorities indicated
by smaller numbers.  For example, in the VxWorks native priority numbering
scheme, the highest priority task has a priority of 0.

In VxWorks, POSIX scheduling interfaces are implemented using the POSIX
priority numbering scheme.  This means that the priority numbers used by
this library \f2do not\f1 match those reported and used in all the other
VxWorks components.  It is possible to change the priority numbering
scheme used by this library by setting the global variable
`posixPriorityNumbering'.  If this variable is set to FALSE, the VxWorks
native numbering scheme (small number = high priority) is used, and
priority numbers used by this library will match those used by the other
portions of VxWorks.

The routines in this library are compliant with POSIX 1003.1b.  In
particular, task priorities are set and reported through the structure
`sched_setparam', which has a single member:

.CS
struct sched_param		/@ Scheduling parameter structure @/
    {
    int sched_priority;		/@ scheduling priority @/
    };
.CE

POSIX 1003.1b specifies this indirection to permit future extensions
through the same calling interface.  For example, because
sched_setparam() takes this structure as an argument (rather than
using the priority value directly) its type signature need not change
if future schedulers require other parameters.

INCLUDE FILES: sched.h

SEE ALSO: POSIX 1003.1b document, taskLib
*/

/* INCLUDES */
#include "vxWorks.h"
#include "private/schedP.h"		/* includes sched.h */
#include "timers.h"
#include "taskLib.h"
#include "drv/timer/timerDev.h"

/* globals */
BOOL posixPriorityNumbering = TRUE;	/* default use POSIX numbering */

/* externals */
extern BOOL	roundRobinOn;		/* BOOL for round-robin scheduling */
extern ULONG	roundRobinSlice;	/* time slice in ticks for RR sched */

/******************************************************************************
*
* sched_setparam - set a task's priority (POSIX)
*
* This routine sets the priority of a specified task, <tid>.  If <tid> is 0,
* it sets the priority of the calling task.  Valid priority numbers are 0
* through 255.
* 
* The <param> argument is a structure whose member `sched_priority' is
* the integer priority value.  For example, the following program fragment
* sets the calling task's priority to 13 using POSIX interfaces:
* 
* .CS
* #include "sched.h"
*  ...
* struct sched_param AppSchedPrio;
*  ...
* AppSchedPrio.sched_priority = 13;
* if ( sched_setparam (0, &AppSchedPrio) != OK )
*     {
*     ... /@ recovery attempt or abort message @/
*     }
*  ...
* .CE
*
* NOTE: If the global variable 'posixPriorityNumbering' is FALSE, the VxWorks
* native priority numbering scheme is used, in which higher priorities are
* indicated by smaller numbers.  This is different than the priority numbering 
* scheme specified by POSIX, in which higher priorities are indicated by
* larger numbers.
*
* RETURNS: 0 (OK) if successful, or -1 (ERROR) on error.
*
* ERRNO: 
*  EINVAL
*     - scheduling priority is outside valid range.
*  ESRCH
*     - task ID is invalid.
*
*/

int sched_setparam
    (
    pid_t 				tid,	/* task ID */
    const struct sched_param * 		param	/* scheduling parameter */
    )
    {
    if (taskPrioritySet (tid, PX_VX_PRIORITY_CONVERT (param->sched_priority)) 
	== ERROR)
	{
	/* map errno to that specified by POSIX */
	switch (errno)
	    {
	    case S_taskLib_ILLEGAL_PRIORITY:
		errno = EINVAL;
		break;
	    case S_objLib_OBJ_ID_ERROR:
		errno = ESRCH;
		break;
	    default:		
		break; 			/* No generic error to use */
	    }
	return (ERROR);
	}

    return (OK);
    }

/******************************************************************************
*
* sched_getparam - get the scheduling parameters for a specified task (POSIX)
*
* This routine gets the scheduling priority for a specified task, <tid>.
* If <tid> is 0, it gets the priority of the calling task.  The task's
* priority is copied to the `sched_param' structure pointed to by <param>.
*
* NOTE: If the global variable 'posixPriorityNumbering' is FALSE, the VxWorks
* native priority numbering scheme is used, in which higher priorities are
* indicated by smaller numbers.  This is different than the priority numbering 
* scheme specified by POSIX, in which higher priorities are indicated by
* larger numbers.
*
* RETURNS: 0 (OK) if successful, or -1 (ERROR) on error.
*
* ERRNO: 
*  ESRCH
*     - invalid task ID.
*
*/

int sched_getparam
    (
    pid_t 			tid,	/* task ID */
    struct sched_param *	param	/* scheduling param to store priority */
    )
    {
    if (taskPriorityGet (tid, &(param->sched_priority)) == ERROR)
	{
	errno = ESRCH;			/* invalid task ID */
	return (ERROR);
	}

    /* convert priority if necessary */
    param->sched_priority = PX_VX_PRIORITY_CONVERT (param->sched_priority);

    return (OK);
    }

/******************************************************************************
*
* sched_setscheduler - set scheduling policy and scheduling parameters (POSIX)
*
* This routine sets the scheduling policy and scheduling parameters for a
* specified task, <tid>.  If <tid> is 0, it sets the scheduling policy and
* scheduling parameters for the calling task.
*
* Because VxWorks does not set scheduling policies (e.g., round-robin scheduling) on a 
* task-by-task basis, setting a scheduling policy that conflicts with the 
* current system policy simply fails and errno is set to EINVAL.  If the 
* requested scheduling policy is the same as the current system policy, then 
* this routine acts just like sched_setparam().
*
* NOTE: If the global variable 'posixPriorityNumbering' is FALSE, the VxWorks
* native priority numbering scheme is used, in which higher priorities are
* indicated by smaller numbers.  This is different than the priority numbering 
* scheme specified by POSIX, in which higher priorities are indicated by
* larger numbers.
*
* RETURNS: The previous scheduling policy (SCHED_FIFO or SCHED_RR), or 
* -1 (ERROR) on error.
*
* ERRNO:
*  EINVAL
*     - scheduling priority is outside valid range, or it is impossible to set
*       the specified scheduling policy.
*  ESRCH
*     - invalid task ID.
*
*/

int sched_setscheduler
    (
    pid_t 			tid,	/* task ID */
    int 			policy,	/* scheduling policy requested */
    const struct sched_param *	param	/* scheduling parameters requested */
    )
    {
    /* currently do not support SCHED_OTHER */
    if (policy == SCHED_OTHER)
	{
	errno = EINVAL;
	return (ERROR);
	}

    /* 
     * is requested scheduling policy same as system policy? If not, fail 
     * request. 
     */
    if (((roundRobinOn == TRUE) && (policy != SCHED_RR)) ||
	((roundRobinOn == FALSE) && (policy != SCHED_FIFO)))
	{
	errno = EINVAL;
	return (ERROR);
	}

    /* set task priority; don't convert priority number - sched_setparam will */
    /* note that sched_setparam calls taskPrioritySet which validates the tid */

    if (sched_setparam (tid, param) == ERROR)
	return (ERROR);
    
    return (OK);

    }

/******************************************************************************
*
* sched_getscheduler - get the current scheduling policy (POSIX)
*
* This routine returns the currents scheduling policy (i.e., SCHED_FIFO 
* or SCHED_RR).  
*
* RETURNS: Current scheduling policy (SCHED_FIFO or SCHED_RR), or -1 (ERROR) 
* on error.
*
* ERRNO: 
*  ESRCH
*     - invalid task ID.
*
*/

int sched_getscheduler
    (
    pid_t tid		/* task ID */
    )
    {
    /* validate tid */
    if ((taskIdVerify (tid) == ERROR) && (tid != 0))
	{
	errno = ESRCH;
	return (ERROR);
	}

    return (roundRobinOn == TRUE ? SCHED_RR : SCHED_FIFO);
    }

/******************************************************************************
*
* sched_yield - relinquish the CPU (POSIX)
*
* This routine forces the running task to give up the CPU.
*
* RETURNS: 0 (OK) if successful, or -1 (ERROR) on error.
*
*/

int sched_yield (void)
    {
    return (taskDelay (0));
    }

/******************************************************************************
*
* sched_get_priority_max - get the maximum priority (POSIX)
*
* This routine returns the value of the highest possible task priority for a 
* specified scheduling policy (SCHED_FIFO or SCHED_RR).
*
* NOTE: If the global variable 'posixPriorityNumbering' is FALSE, the VxWorks
* native priority numbering scheme is used, in which higher priorities are
* indicated by smaller numbers.  This is different than the priority numbering 
* scheme specified by POSIX, in which higher priorities are indicated by
* larger numbers.
*
* RETURNS: Maximum priority value, or -1 (ERROR) on error.
*
* ERRNO: 
*  EINVAL
*     - invalid scheduling policy.
*
*/

int sched_get_priority_max
    (
    int policy		/* scheduling policy */
    )
    {
    /* check if policy is valid */
    if ((policy != SCHED_RR) && (policy != SCHED_FIFO))
	{
	errno = EINVAL;
	return (ERROR);
	}

    /* return highest possible priority */
    if (policy == SCHED_RR)
	return (PX_NUMBER_CONVERT (SCHED_RR_HIGH_PRI));
    else
	return (PX_NUMBER_CONVERT (SCHED_FIFO_HIGH_PRI));
    }    

/******************************************************************************
*
* sched_get_priority_min - get the minimum priority (POSIX)
*
* This routine returns the value of the lowest possible task priority for a 
* specified scheduling policy (SCHED_FIFO or SCHED_RR).
*
* NOTE: If the global variable 'posixPriorityNumbering' is FALSE, the VxWorks
* native priority numbering scheme is used, in which higher priorities are
* indicated by smaller numbers.  This is different than the priority numbering 
* scheme specified by POSIX, in which higher priorities are indicated by
* larger numbers.
*
* RETURNS: Minimum priority value, or -1 (ERROR) on error.
*
* ERRNO: 
*  EINVAL
*     - invalid scheduling policy.
*
*/

int sched_get_priority_min
    (
    int policy		/* scheduling policy */
    )
    {
    /* check if policy is valid */
    if ((policy != SCHED_RR) && ( policy != SCHED_FIFO))
	{
	errno = EINVAL;
	return (ERROR);
	}

    /* return lowest possible priority */
    if (policy == SCHED_RR)
	return (PX_NUMBER_CONVERT (SCHED_RR_LOW_PRI));
    else
	return (PX_NUMBER_CONVERT (SCHED_FIFO_LOW_PRI));
    }    

/******************************************************************************
*
* sched_rr_get_interval - get the current time slice (POSIX)
*
* This routine sets <interval> to the current time slice period if round-robin
* scheduling is currently enabled. 
*
* RETURNS: 0 (OK) if successful, -1 (ERROR) on error.
*
* ERRNO: 
*  EINVAL
*     - round-robin scheduling is not currently enabled.
*  ESRCH
*     - invalid task ID.
*
* INTERNAL: A slight race condition could exist between the time that
* we check that round-robin is on and when we get and report the time slice.
*
*/

int sched_rr_get_interval
    (
    pid_t		tid,		/* task ID */
    struct timespec * 	interval	/* struct to store time slice */
    )
    {
    time_t		rate;		/* clock rate in ticks */
    time_t		timeSlice;	/* time slice in ticks */

    /* validate tid - tid = 0 references current task */
    if ((taskIdVerify (tid) == ERROR) && (tid != 0))
	{
	errno = ESRCH;
	return (ERROR);
	}

    /* validate that round-robin is on */
    if (roundRobinOn != TRUE)
	{
	errno = EINVAL;
	return (ERROR);
	}

    /* get clock rate and time slice */
    rate = sysClkRateGet();
    timeSlice = roundRobinSlice;

    /* convert ticks to seconds/nanoseconds */
    interval->tv_sec = (time_t) (timeSlice / rate);
    interval->tv_nsec = (timeSlice % rate) * (1000000000 / rate);

    return (OK);
    }
