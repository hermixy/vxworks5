/* tickLib.c - clock tick support library */

/* Copyright 1984-1994 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01q,22may02,jgn  add tick64Get() interface - SPR #70255
01p,24jun96,sbs  made windview instrumentation conditionally compiled
01o,24jan94,smb  added instrumentation macros
01n,10dec93,smb  added instrumentation
01m,20jan93,jdi  documentation cleanup for 5.1.
01l,04jul92,jcf  private header files.
01k,26may92,rrr  the tree shuffle
01j,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed VOID to void
		  -changed copyright notice
01i,30mar91,jdi  documentation cleanup; doc review by jcf.
01h,29aug90,jcf  documentation.
01g,10jul90,jcf  changed to support 64 bit internal tick count.
01f,26jun90,jcf  removed queue overflow stuff.
01e,28aug89,jcf  modified to support version 2.0 of wind.
01d,23sep88,gae  documentation touchup.
01c,23aug88,gae  documentation.
01b,12aug88,jcf  clean up.
01a,19jan87,jcf  written.
*/

/*
DESCRIPTION
This library is the interface to the VxWorks kernel routines that announce a
clock tick to the kernel, get the current time in ticks, and set the
current time in ticks.

Kernel facilities that rely on clock ticks include taskDelay(), wdStart(),
kernelTimeslice(), and semaphore timeouts.  In each case, the specified
timeout is relative to the current time, also referred to as "time to fire."
Relative timeouts are not affected by calls to tickSet(), which only changes
absolute time.  The routines tickSet() and tickGet() keep track of absolute
time in isolation from the rest of the kernel.

Time-of-day clocks or other auxiliary time bases are preferable for lengthy
timeouts of days or more.  The accuracy of such time bases is greater, and
some external time bases even calibrate themselves periodically.

INTERNAL:
	WINDVIEW INSTRUMENTATION
Level 1:
	N/A

Level 2:
	N/A

Level 3:
	tickAnnounce() causes EVENT_TICKANNOUNCE

INCLUDE FILES: tickLib.h

SEE ALSO: kernelLib, taskLib, semLib, wdLib,
.pG "Basic OS"
*/

#include "vxWorks.h"
#include "tickLib.h"
#include "taskLib.h"
#include "private/windLibP.h"
#include "private/workQLibP.h"
#include "private/eventP.h"

/* globals */

ULONG	vxTicks;		/* current time in ticks */
UINT64	vxAbsTicks;		/* absolute time since power on in ticks */

/*******************************************************************************
*
* tickAnnounce - announce a clock tick to the kernel
*
* This routine informs the kernel of the passing of time.  It should be
* called from an interrupt service routine that is connected to the system
* clock.  The most common frequencies are 60Hz or 100Hz.  Frequencies in
* excess of 600Hz are an inefficient use of processor power because the
* system will spend most of its time advancing the clock.  By default,
* this routine is called by usrClock() in usrConfig.c.
*
* RETURNS: N/A
*
* SEE ALSO: kernelLib, taskLib, semLib, wdLib,
* .pG "Basic OS"
*/

void tickAnnounce (void)
    {

#ifdef WV_INSTRUMENTATION
    /* windview - level 3 event */
    EVT_CTX_0 (EVENT_TICKANNOUNCE);
#endif

    if (kernelState)			/* defer work if in kernel */
	workQAdd0 ((FUNCPTR)windTickAnnounce);
    else
	{
	kernelState = TRUE;		/* KERNEL_ENT */

	windTickAnnounce ();		/* advance tick queue */

	windExit ();			/* KERNEL_EXIT */
	}
    }

/*******************************************************************************
*
* tickSet - set the value of the kernel's tick counter
*
* This routine sets the internal tick counter to a specified value in
* ticks.  The new count will be reflected by tickGet(), but will not change
* any delay fields or timeouts selected for any tasks.  For example, if a
* task is delayed for ten ticks, and this routine is called to advance time,
* the delayed task will still be delayed until ten tickAnnounce() calls have
* been made.
*
* RETURNS: N/A
*
* SEE ALSO: tickGet(), tickAnnounce()
*
* INTERNAL
* The routine locks interrupts while obtaining the copy of the 64 value to
* prevent an interrupt from splitting what is unlikely to be an atomic operation
* on a 32 bit CPU.
*/

void tickSet
    (
    ULONG ticks         /* new time in ticks */
    )
    {
    int key = intLock();		/* INTERRUPTS LOCKED */

    vxAbsTicks = ticks;			/* effectively zero the upper bits */

    intUnlock(key);			/* INTERRUPTS UNLOCKED */
    }

/*******************************************************************************
*
* tickGet - get the value of the kernel's tick counter
*
* This routine returns the current value of the tick counter.
* This value is set to zero at startup, incremented by tickAnnounce(),
* and can be changed using tickSet().
*
* RETURNS: The most recent tickSet() value, plus all tickAnnounce() calls since.
*
* SEE ALSO: tickSet(), tickAnnounce()
*
* INTERNAL
* There should be no need to lock interrupts on this one; the compiler should
* only generate a read from one half of the UINT64 which means that the read
* can never be interrupted.
*/

ULONG tickGet (void)
    {
    return (ULONG) (vxAbsTicks & 0xFFFFFFFFull);
    }

/*******************************************************************************
*
* tick64Set - set the value of the kernel's tick counter in 64 bits
*
* This routine sets the internal tick counter to a specified value in
* ticks.  The new count will be reflected by tick64Get() and tickGet() (only
* the lower 32 bits), but will not change any delay fields or timeouts selected
* for any tasks.  For example, if a task is delayed for ten ticks, and this
* routine is called to advance time, the delayed task will still be delayed
* until ten tickAnnounce() calls have been made.
*
* RETURNS: N/A
*
* SEE ALSO: tick64Get(), tickGet(), tickSet(), tickAnnounce()
*
* INTERNAL
* The routine locks interrupts while obtaining the copy of the 64 value to
* prevent an interrupt from splitting what is unlikely to be an atomic operation
* on a 32 bit CPU.
*
* NOMANUAL
*/

void tick64Set
    (
    UINT64 ticks         /* new time in ticks */
    )
    {
    int key = intLock();			/* INTERRUPTS LOCKED */

    vxAbsTicks = ticks;

    intUnlock (key);				/* INTERRUPTS UNLOCKED */
    }

/*******************************************************************************
*
* tick64Get - get the value of the kernel's tick counter as a 64 bit value
*
* This routine returns the current value of the 64 bit absolute tick counter.
* This value is set to zero at startup, incremented by tickAnnounce(),
* and can be changed using tickSet() or tick64Set().
*
* RETURNS: The most recent tickSet()/tick64Set() value, plus all
* tickAnnounce() calls since.
*
* SEE ALSO: tickGet(), tick64Set(), tickSet(), tickAnnounce()
*
* INTERNAL
* The routine locks interrupts while obtaining the copy of the 64 value to
* prevent an interrupt from splitting what is unlikely to be an atomic operation
* on a 32 bit CPU.
*
* NOMANUAL
*/

UINT64 tick64Get (void)
    {
    UINT64	ret;
    int		key	= intLock();		/* INTERRUPTS LOCKED */

    ret = vxAbsTicks;

    intUnlock (key);				/* INTERRUPTS UNLOCKED */

    return ret;
    }
