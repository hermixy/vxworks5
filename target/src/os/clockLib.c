/* clockLib.c - clock library (POSIX) */

/* Copyright 1991-2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
02n,22may02,gls  changed to use 64 bit ticks (SPR #70255)
02m,25sep01,gls  changed clock_gettime() to load sysTicks from tickGet() 
                 to prevent the compiler from optimizing it out (SPR #64367)
02l,15mar99,elg  add informations on clock_gettime() parameters (SPR 5412).
02k,21feb99,jdi  doc: listed errnos.
02j,16oct95,jdi  doc: clarified action of clock_setres() (SPR 4283).
02i,28mar95,kdl  made clock_gettime() keep local copy of vxTicks (SPR #3595).
02h,07feb95,jdi  doc: changed clock_setres() to clarify the action.
02g,23jan95,jdi  doc cleanup.
02f,08apr94,kdl  fixed alignment in clock_show() display.
02e,23nov93,dvs  updated for POSIX 1003.4 draft 14 compliance.
02d,02oct92,jdi  documentation cleanup.
02c,23sep92,kdl  changed include to private/timerLibP.h.
02b,09sep92,gae  fixed clock_show() format.
02a,23jun92,gae  Draft 12 revision. Deleted clock_getdrift() & clock_setdrift().
		 Added non-POSIX clock_setres().  Arranged that clockLibInit()
		 needn't be explicitly called.
01d,25jul92,smb  changed time.h to timers.h
01c,26may92,rrr  the tree shuffle
01b,04feb92,gae  fixed copyright include; revised according to DEC review.
		   clockLibInit() tests for initial system clock rate > 0;
		   clock_gettime() returns EFAULT for NULL time pointer;
		   clock_settime() checks nanosecond range properly.
01a,16oct91,gae  written.
*/

/*
DESCRIPTION
This library provides a clock interface, as defined in the IEEE standard,
POSIX 1003.1b.

A clock is a software construct that keeps time in seconds and
nanoseconds.  The clock has a simple interface with three routines:
clock_settime(), clock_gettime(), and clock_getres().  The non-POSIX routine
clock_setres() that was provided so that clockLib could be informed if there
were changes in the system clock rate is no longer necessary. This routine
is still present for backward compatibility, but does nothing.

Times used in these routines are stored in the timespec structure:
.CS
struct timespec
    {
    time_t	tv_sec;		/@ seconds @/
    long	tv_nsec;	/@ nanoseconds (0 -1,000,000,000) @/
    };
.CE

IMPLEMENTATION
Only one <clock_id> is supported, the required CLOCK_REALTIME.
Conceivably, additional "virtual" clocks could be supported, or support
for additional auxiliary clock hardware (if available) could be added.

INCLUDE FILES: timers.h

SEE ALSO: IEEE 
.pG "Basic OS,"
POSIX 1003.1b documentation

INTERNAL
1. Access to clockRealtime should be intLock'ed during task usage?
2. What is, is not, accessable at interrupt level?
*/

#include "vxWorks.h"
#include "errno.h"
#include "memLib.h"
#include "tickLib.h"
#include "stdio.h"
#include "string.h"
#include "sysLib.h"
#include "timers.h"
#include "private/timerLibP.h"
#include "private/windLibP.h"

struct clock _clockRealtime;


/*******************************************************************************
*
* clockLibInit - initialize clock facility
*
* This routine initializes the POSIX timer/clock facilities.
* It should be called by usrRoot() in usrConfig.c before any other
* routines in this module.
*
* WARNING
* Non-POSIX.
*
* RETURNS:
* 0 (OK), or -1 (ERROR) on failure or already initialized or clock rate 
* less than 1 Hz.
*
* NOMANUAL
*/

int clockLibInit (void)
    {
    static BOOL libInstalled = FALSE;

    if (libInstalled)
	return (ERROR);

    if (sysClkRateGet () < 1)
	return (ERROR);

    libInstalled = TRUE;

    bzero ((char*)&_clockRealtime, sizeof (_clockRealtime));

    _clockRealtime.tickBase	= tick64Get();

    return (OK);
    }
/*******************************************************************************
*
* clock_getres - get the clock resolution (POSIX)
*
* This routine gets the clock resolution, in nanoseconds, based on the
* rate returned by sysClkRateGet().  If <res> is non-NULL, the resolution is
* stored in the location pointed to.
*
* INTERNAL
* Resolution is always assumed to be less than 1 second -- only
* the tv_nsec field is filled in.
*
* RETURNS:
* 0 (OK), or -1 (ERROR) if <clock_id> is invalid.
*
* ERRNO: EINVAL
*
* SEE ALSO: clock_settime(), sysClkRateGet(), clock_setres()
*/

int clock_getres
    (
    clockid_t clock_id,		/* clock ID (always CLOCK_REALTIME) */
    struct timespec * res	/* where to store resolution */
    )
    {
    (void)clockLibInit ();

    if (clock_id != CLOCK_REALTIME)
	{
	errno = EINVAL;
	return (ERROR);
	}

    if (res != NULL)
	{
	/* assume that clock resolution is <= 1 second */
	res->tv_sec  = 0;
	res->tv_nsec = (BILLION / sysClkRateGet());
	}

    return (OK);
    }

/*******************************************************************************
*
* clock_setres - set the clock resolution
*
* This routine is obsolete. It will always return OK.
*
* NOTE
* Non-POSIX.
*
* INTERNAL
* This routine is no longer needed; the underlying system clock resolution
* will always be used directly.
*
* RETURNS:
* OK always.
*
* ERRNO: EINVAL
*
* SEE ALSO: clock_getres(), sysClkRateSet()
*/

int clock_setres
    (
    clockid_t clock_id,		/* clock ID (always CLOCK_REALTIME) */
    struct timespec * res	/* resolution to be set */
    )
    {
    return (OK);
    }

/*******************************************************************************
*
* clock_gettime - get the current time of the clock (POSIX)
*
* This routine gets the current value <tp> for the clock.
*
* INTERNAL
* The standard doesn't indicate when <tp> is NULL (an invalid address)
* whether errno should be EINVAL or EFAULT.
*
* RETURNS: 0 (OK), or -1 (ERROR) if <clock_id> is invalid or <tp> is NULL.
*
* ERRNO: EINVAL, EFAULT
*/

int clock_gettime
    (
    clockid_t clock_id,		/* clock ID (always CLOCK_REALTIME) */
    struct timespec * tp	/* where to store current time */
    )
    {
    UINT64     diffTicks;	/* system clock tick count */

    (void)clockLibInit ();

    if (clock_id != CLOCK_REALTIME)
	{
	errno = EINVAL;
	return (ERROR);
	}

    if (tp == NULL)
	{
	errno = EFAULT;
	return (ERROR);
	}

    diffTicks = tick64Get() - _clockRealtime.tickBase;

    TV_CONVERT_TO_SEC(*tp, diffTicks);
    TV_ADD (*tp, _clockRealtime.timeBase);

    return (OK);
    }
/*******************************************************************************
*
* clock_settime - set the clock to a specified time (POSIX)
*
* This routine sets the clock to the value <tp>, which should be a multiple
* of the clock resolution.  If <tp> is not a multiple of the resolution, it
* is truncated to the next smallest multiple of the resolution.
*
* RETURNS:
* 0 (OK), or -1 (ERROR) if <clock_id> is invalid, <tp> is outside the supported 
* range, or the <tp> nanosecond value is less than 0 or equal to or greater than
* 1,000,000,000.
*
* ERRNO: EINVAL
*
* SEE ALSO: clock_getres()
*/

int clock_settime
    (
    clockid_t clock_id,		/* clock ID (always CLOCK_REALTIME) */
    const struct timespec * tp	/* time to set */
    )
    {
    (void)clockLibInit ();

    if (clock_id != CLOCK_REALTIME)
	{
	errno = EINVAL;
	return (ERROR);
	}

    if (tp == NULL ||
	tp->tv_nsec < 0 || tp->tv_nsec >= BILLION)
	{
	errno = EINVAL;
	return (ERROR);
	}

    /* convert timespec to vxTicks XXX use new kernel time */

    _clockRealtime.tickBase = tick64Get();
    _clockRealtime.timeBase = *tp;

    return (OK);
    }

/*******************************************************************************
*
* clock_show - show info on a clock
*
* WARNING
* Non-POSIX.
*
* RETURNS:
* 0 (OK), or -1 (ERROR) if <clock_id> is invalid
*
* ERRNO: EINVAL
*
* NOMANUAL
*/

int clock_show
    (
    clockid_t clock_id		/* clock ID (always CLOCK_REALTIME) */
    )
    {
    static char *title1 = "  seconds    nanosecs  freq (hz)  resolution\n";
    static char *title2 = "---------- ----------- ---------- ----------\n";  
    static char *title3 = "\n  base ticks          base secs  base nsecs\n";
    static char *title4 = "--------------------  ---------- ----------\n";

    struct timespec tp;

    (void)clockLibInit ();

    if (clock_id != CLOCK_REALTIME)
	{
	errno = EINVAL;
	return (ERROR);
	}

    (void) clock_gettime (clock_id, &tp);

    printf (title1);
    printf (title2);

    printf (" %8.8ld   %9.9ld   %8.8d   %8.8d\n",
	    tp.tv_sec, tp.tv_nsec, sysClkRateGet(), BILLION / sysClkRateGet());

    printf (title3);
    printf (title4);

    printf(" 0x%08x%08x    %8.8ld   %9.9ld\n",
	   (UINT) (_clockRealtime.tickBase >> 32),
	   (UINT)  _clockRealtime.tickBase, 
	   _clockRealtime.timeBase.tv_sec, 
	   _clockRealtime.timeBase.tv_nsec);

    return (OK);
    }
