/* wdLib.c - watchdog timer library */

/* Copyright 1984-1995 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01t,24jun96,sbs  made windview instrumentation conditionally compiled
02s,13oct95,jdi  doc: removed SEE ALSO to .pG Cross-Dev.
02r,18jan95,rhp  doc: say explicitly no need to cancel expired timers,
                 and improve wdLib and wdSTart() man pages from bss comments.
02u,14apr94,smb  fixed class dereferencing for instrumentation macros
02t,15mar94,smb  modified instrumentation macros
02s,24jan94,smb  added instrumentation macros
02r,10dec93,smb  added instrumentation
02q,24feb93,jdi  doc tweaks from review by kdl.
02p,20jan93,jdi  documentation cleanup for 5.1.
02o,13nov92,jcf  package init called with with watchdog creation.
02n,29jul92,jcf  package init called with with object initialization.
02m,04jul92,jcf  private headers.
02l,26may92,rrr  the tree shuffle
02k,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed includes to have absolute path from h/
		  -changed VOID to void
		  -changed copyright notice
02j,30mar91,jdi  documentation cleanup; doc review by jcf.
02i,05oct90,dnw  made wdInit() and wdTerminate() be NOMANUAL.
02h,01oct90,jcf  fixed wdDestroy() to invalidate with ISR mutual exclusion.
02g,29aug90,jcf  documentation.
02f,10aug90,dnw  changed wdCancel() from void to STATUS.
02e,02aug90,jcf  documentation.
02d,17jul90,dnw  changed to new objAlloc() call.
02c,03jul90,jcf  documentation.
		 removed erroneous log message.
02b,26jun90,jcf  updated class structure, change over to objAlloc ()
02a,17apr90,jcf  integrated into wind 2.0.
01j,01sep88,gae  documentation.
01i,04nov87,ecs  documentation.
01h,25mar87,jlf  documentation
01g,21dec86,dnw  changed to not get include files from default directories.
01f,05sep86,jlf  minor documentation.
01e,14apr86,rdc  changed memAllocates to mallocs.
01d,07sep84,jlf  added copyright notice and comments.
01c,02aug84,dnw  changed calls to vxInt... to int...
01b,11jul84,ecs  changed calls to vxIntLock/vxIntUnlock to restore old level.
01a,22may84,dnw  written
*/

/*
DESCRIPTION
This library provides a general watchdog timer facility.  Any task may
create a watchdog timer and use it to run a specified routine in
the context of the system-clock ISR, after a specified delay.

Once a timer has been created with wdCreate(), it can be started with
wdStart().  The wdStart() routine specifies what routine to run, a
parameter for that routine, and the amount of time (in ticks) before
the routine is to be called.  (The timeout value is in ticks as
determined by the system clock; see sysClkRateSet() for more
information.)  After the specified delay ticks have elapsed (unless
wdCancel() is called first to cancel the timer) the timeout routine is
invoked with the parameter specified in the wdStart() call.  The
timeout routine is invoked whether the task which started the watchdog
is running, suspended, or deleted.

The timeout routine executes only once per wdStart() invocation; there
is no need to cancel a timer with wdCancel() after it has expired, or
in the expiration callback itself.

Note that the timeout routine is invoked at interrupt level, rather than
in the context of the task.  Thus, there are restrictions on what the
routine may do.  Watchdog routines are constrained to the same rules
as interrupt service routines.  For example, they may not take semaphores,
issue other calls that may block, or use I/O system routines like printf().

EXAMPLE
In the fragment below, if maybeSlowRoutine() takes more than 60 ticks,
logMsg() will be called with the string as a parameter, causing the message to
be printed on the console.  Normally, of course, more significant corrective
action would be taken.
.CS
    WDOG_ID wid = wdCreate ();
    wdStart (wid, 60, logMsg, "Help, I've timed out!");
    maybeSlowRoutine ();	/@ user-supplied routine @/
    wdCancel (wid);
.CE

INTERNAL:
	WINDVIEW INSTRUMENTATION
Level 1:
	wdCreate() causes EVENT_WDCREATE
	wdDestroy() causes EVENT_WDDELETE
	wdStart() causes EVENT_WDSTART
	wdCancel() causes EVENT_WDCANCEL

Level 2:
	N/A

Level 3: N/A

INCLUDE FILES: wdLib.h

SEE ALSO: logLib,
.pG "Basic OS"
*/

#include "vxWorks.h"
#include "taskLib.h"
#include "errno.h"
#include "intLib.h"
#include "private/classLibP.h"
#include "private/objLibP.h"
#include "private/wdLibP.h"
#include "private/windLibP.h"
#include "private/workQLibP.h"
#include "private/eventP.h"


/* locals */

LOCAL BOOL wdLibInstalled;
LOCAL OBJ_CLASS wdClass;		/* non-instrumented class */

/* globals */

CLASS_ID wdClassId = &wdClass;

/* windview definitions */

#ifdef WV_INSTRUMENTATION
LOCAL OBJ_CLASS wdInstClass;
CLASS_ID wdInstClassId = &wdInstClass;
#endif

/*******************************************************************************
*
* wdLibInit - initialize watchdog library
*
* This routine initializes the watchdog object class.  No watchdog operation
* will work until this is called.  This routine is called during system
* configuration within kernelInit().  This routine should only be called once.
*
* NOMANUAL
*/

STATUS wdLibInit (void)
    {
    if ((!wdLibInstalled) &&
	(classInit (wdClassId, sizeof (WDOG), OFFSET (WDOG, objCore),
	 (FUNCPTR) wdCreate, (FUNCPTR) wdInit, (FUNCPTR) wdDestroy) == OK))
	{

#ifdef WV_INSTRUMENTATION
	/* initialise the instrumented class for level 1 event logging */
	wdClassId -> initRtn = wdInstClassId;
        classInstrument (wdClassId, wdInstClassId);
        wdInstClassId->instRtn = (FUNCPTR) _func_evtLogO;
#endif
	wdLibInstalled = TRUE;
	}

    return ((wdLibInstalled) ? OK : ERROR);
    }

/*******************************************************************************
*
* wdCreate - create a watchdog timer
*
* This routine creates a watchdog timer by allocating a WDOG structure in
* memory.
*
* RETURNS: The ID for the watchdog created, or NULL if memory is insufficient.
*
* SEE ALSO: wdDelete()
*/

WDOG_ID wdCreate (void)
    {
    WDOG_ID wdId;
#ifdef WV_INSTRUMENTATION
    int level;
#endif

    if ((!wdLibInstalled) && (wdLibInit () != OK))
	return (NULL);				/* package init problem */

    wdId = (WDOG_ID) objAlloc (wdClassId);


    /* initialize allocated watchdog */

    if ((wdId != NULL) && (wdInit (wdId) != OK))
	{
	objFree (wdClassId, (char *) wdId);
	return (NULL);
	}

#ifdef WV_INSTRUMENTATION
    /* windview - level 1 event logging */
    level = intLock ();			
    EVT_OBJ_1 (OBJ, wdId, wdClassId, EVENT_WDCREATE, wdId);
    intUnlock (level);				
#endif

    return (wdId);
    }

/*******************************************************************************
*
* wdInit - initialize a watchdog timer
*
* This routine initializes a static watchdog or a watchdog embedded in a
* larger object.
*
* RETURNS: OK, or ERROR if the watchdog could not be initialized.
*
* NOMANUAL
*/

STATUS wdInit
    (
    WDOG *pWdog         /* pointer to watchdog to initialize */
    )
    {
    if ((!wdLibInstalled) && (wdLibInit () != OK))
	return (ERROR);				/* package init problem */

    pWdog->status        = WDOG_OUT_OF_Q;	/* initially out of q */
    pWdog->deferStartCnt = 0;			/* no pending starts */

#ifdef WV_INSTRUMENTATION
    /* windview - connect instrumented class for level 1 event logging */
    if (wvObjIsEnabled)
	objCoreInit (&pWdog->objCore, wdInstClassId); 
    else
#endif
        objCoreInit (&pWdog->objCore, wdClassId);	/* initialize core */

    return (OK);
    }

/*******************************************************************************
*
* wdDelete - delete a watchdog timer
*
* This routine de-allocates a watchdog timer.  The watchdog will be removed
* from the timer queue if it has been started.  This routine complements
* wdCreate().
*
* RETURNS: OK, or ERROR if the watchdog timer cannot be de-allocated.
*
* SEE ALSO: wdCreate()
*/

STATUS wdDelete
    (
    WDOG_ID wdId                /* ID of watchdog to delete */
    )
    {
    return (wdDestroy (wdId, TRUE));		/* delete watchdog */
    }

/*******************************************************************************
*
* wdTerminate - terminate a watchdog timer
*
* This routine terminates a watchdog timer.  The watchdog will be removed
* from the timer queue if it has been started.  This routine differs from
* wdDelete() in that associated memory is not de-allocated.  This routine
* complements wdInit().
*
* RETURNS: OK, or ERROR if the watchdog cannot be terminated.
*
* NOMANUAL
*/

STATUS wdTerminate
    (
    WDOG_ID wdId                /* ID of watchdog to terminate */
    )
    {
    return (wdDestroy (wdId, FALSE));		/* terminate watchdog */
    }

/*******************************************************************************
*
* wdDestroy - terminate a watchdog timer
*
* This routine terminates a watchdog timer and optionally de-allocates
* associated memory.  If the watchdog has been started, it will be removed
* from the timer queue.  This routine underlies wdDelete(), and
* wdTerminate().
*
* RETURNS: OK, or ERROR if the watchdog cannot be destroyed.
*
* NOMANUAL
*/

STATUS wdDestroy
    (
    WDOG_ID wdId,               /* ID of watchdog to terminate */
    BOOL    dealloc             /* dealloc associated memory */
    )
    {
    int level;

    if (INT_RESTRICT () != OK)			/* restrict isr use */
	return (ERROR);

    level = intLock ();				/* LOCK INTERRUPTS */

    if (OBJ_VERIFY (wdId, wdClassId) != OK)	/* validate watchdog ID */
	{
	intUnlock (level);			/* UNLOCK INTERRUPTS */
	return (ERROR);
	}

#ifdef WV_INSTRUMENTATION
    /* windview - level 1 event logging */
    EVT_OBJ_1 (OBJ, wdId, wdClassId, EVENT_WDDELETE, wdId);
#endif

    objCoreTerminate (&wdId->objCore);		/* invalidate watchdog */

    kernelState = TRUE;				/* KERNEL ENTER */

    intUnlock (level);				/* UNLOCK INTERRUPTS */

    windWdCancel (wdId);			/* cancel watchdog */

    wdId->status = WDOG_DEAD;			/* dead dog */

    TASK_SAFE ();				/* TASK SAFE */

    windExit ();				/* EXIT KERNEL */

    if (dealloc)
	objFree (wdClassId, (char *) wdId);	/* deallocate watchdog */

    TASK_UNSAFE ();				/* TASK UNSAFE */

    return (OK);
    }

/*******************************************************************************
*
* wdStart - start a watchdog timer
* 
* This routine adds a watchdog timer to the system tick queue.  The
* specified watchdog routine will be called from interrupt level after
* the specified number of ticks has elapsed.  Watchdog timers may be
* started from interrupt level.  
* 
* To replace either the timeout <delay> or the routine to be executed,
* call wdStart() again with the same <wdId>; only the most recent
* wdStart() on a given watchdog ID has any effect.  (If your
* application requires multiple watchdog routines, use wdCreate() to
* generate separate a watchdog ID for each.)  To cancel a watchdog
* timer before the specified tick count is reached, call wdCancel().
* 
* Watchdog timers execute only once, but some applications require
* periodically executing timers.  To achieve this effect, the timer
* routine itself must call wdStart() to restart the timer on each
* invocation.
* 
* WARNING: The watchdog routine runs in the context of the
* system-clock ISR; thus, it is subject to all ISR restrictions.
* 
* RETURNS: OK, or ERROR if the watchdog timer cannot be started.
*
* SEE ALSO: wdCancel()
*/

STATUS wdStart
    (
    WDOG_ID wdId,               /* watchdog ID */
    int delay,                  /* delay count, in ticks */
    FUNCPTR pRoutine,           /* routine to call on time-out */
    int parameter               /* parameter with which to call routine */
    )
    {
    int level = intLock ();			/* LOCK INTERRUPTS */

    if (OBJ_VERIFY (wdId, wdClassId) != OK)
	{
	intUnlock (level);			/* UNLOCK INTERRUPTS */
	return (ERROR);
	}

#ifdef WV_INSTRUMENTATION
    /* windview - level 1 event logging */
    EVT_OBJ_2 (OBJ, wdId, wdClassId, EVENT_WDSTART, wdId, delay);
#endif

    if (kernelState)				/* already in kernel? */
	{
	wdId->deferStartCnt ++;			/* bump the start count */
	wdId->wdParameter = parameter;		/* update w/ new parameter */
	wdId->wdRoutine   = pRoutine;		/* update w/ new routine */
	intUnlock (level);			/* UNLOCK INTERRUPTS */

	workQAdd2 (windWdStart, (int)wdId, delay);	/* defer the wdStart */
	}
    else
	{
	wdId->deferStartCnt  = 1;		/* initialize start count */
	wdId->wdParameter    = parameter;	/* update w/ new parameter */
	wdId->wdRoutine      = pRoutine;	/* update w/ new routine */
	kernelState	     = TRUE;		/* KERNEL ENTER */
	intUnlock (level);			/* UNLOCK INTERRUPTS */

	if (windWdStart (wdId, delay) != OK)	/* start the watchdog */
	    {
	    windExit ();			/* KERNEL EXIT */
	    return (ERROR);
	    }

	windExit ();				/* KERNEL EXIT */
	}

    return (OK);
    }

/*******************************************************************************
*
* wdCancel - cancel a currently counting watchdog
*
* This routine cancels a currently running watchdog timer by
* zeroing its delay count.  Watchdog timers may be canceled from interrupt
* level.
*
* RETURNS: OK, or ERROR if the watchdog timer cannot be canceled.
*
* SEE ALSO: wdStart()
*/

STATUS wdCancel
    (
    WDOG_ID wdId        /* ID of watchdog to cancel */
    )
    {
    int level = intLock ();			/* LOCK INTERRUPTS */

    if (OBJ_VERIFY (wdId, wdClassId) != OK)
	{
	intUnlock (level);			/* UNLOCK INTERRUPTS */
	return (ERROR);
	}

#ifdef WV_INSTRUMENTATION
    /* windview - level 1 event logging */
    EVT_OBJ_1 (OBJ, wdId, wdClassId, EVENT_WDCANCEL, wdId);
#endif

    if (kernelState)
	{
	intUnlock (level);			/* UNLOCK INTERRUPTS */
	workQAdd1 ((FUNCPTR)windWdCancel, (int)wdId);
	}
    else
	{
	kernelState = TRUE;			/* KERNEL_ENT */
	intUnlock (level);			/* UNLOCK INTERRUPTS */

	windWdCancel (wdId);			/* cancel watchdog */

	windExit ();				/* KERNEL EXIT */
	}

    return (OK);
    }

/*******************************************************************************
*
* wdTick - obsolete routine
*
* This routine is provided for backward compatibility and is simply a NOP
* if called.
*
* NOMANUAL
*/

void wdTick (void)
    {
    /* obsolete */
    }
