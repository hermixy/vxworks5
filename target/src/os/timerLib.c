/* timerLib.c - timer library (POSIX) */

/* Copyright 1991-2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
02w,09may02,wsl  add definition of timespec to nanosleep comments; SPR 26086
02v,08aug00,jgn  move alarm from sigLib to timerLib
02u,17jul00,jgn  merge DOT-4 pthreads code
02t,19oct01,dcb  Fix the routine title line to match the coding conventions.
02s,21sep99,cno  check return from wdDelete (SPR20611)
02r,15sep99,pfl  fixed compile error
02q,09jul99,cno  timer_settime returns timer_gettime remaining time (SPR27189)
02p,21feb99,jdi  doc: listed errnos.
02o,05feb96,dbt  added a test in timer_settime to detect if the time has
		 already passed (SPR #7463).
02n,30oct96,dgp  doc: change INTERNAL comment to external for timer_settime
		  per SPR #6525
02m,09oct06,dgp  doc: correct timer_settime() reference per SPR 7323 & 3612
02l,28mar95,kdl  return timer reload value during timer_gettime() (SPR #3299).
02k,20jan95,jdi  doc cleanup, including new number for POSIX standard.
02j,08apr94,dvs  added check for disarmed timer in timer_gettime() (SPR #3100).
02i,08dec93,dvs  added fix of timer_gettime() for interval timer as well.
02h,06dec93,dvs  timer_gettime() now returns amt of time remaining on the timer
		 (SPR #2673)
02g,29nov93,dvs  brought up to POSIX draft 14 specification
02f,14mar93,jdi  fixed typo in code example for timer_connect().
02e,11feb93,gae  clockLibInit() installed before using timers (#1834).
		 made sure wdStart() not invoked with 0 delay when
		 interval less than clock resolution (#2006).
02d,02oct92,jdi  documentation cleanup.
02c,23sep92,kdl  changed include to private/timerLibP.h; deleted signal
		 function prototypes, now in sigLibP.h.
02b,19aug92,gae  fixed nanosleep() to return correct amount of underrun.
02a,22jul92,gae  Draft 12 revision.  fixed timer_getoverrun(), nanosleep().
		 Removed timerLibInit() and timer_object_create().
		 Got rid of objects.
01d,25jul92,smb  changed time.h to timers.h
01d,26may92,rrr  the tree shuffle
01c,30apr92,rrr  some preparation for posix signals.
01b,04feb92,gae  fixed copyright include; revised according to DEC review.
		     added ability to delete timers from exiting tasks;
		     timer_create() tests for valid signal number;
		     timer_getoverrun() always returns -1 as no support;
		     timer_settime() no longer arms timer with 0 delay.
		     nanosleep() no longer returns +1 on "success";
		     documentation touchup.
01a,16oct91,gae  written.
*/

/*
DESCRIPTION
This library provides a timer interface, as defined in the IEEE standard,
POSIX 1003.1b.

Timers are mechanisms by which tasks signal themselves after a designated
interval.  Timers are built on top of the clock and signal facilities.  The
clock facility provides an absolute time-base.  Standard timer functions
simply consist of creation, deletion and setting of a timer.  When a timer
expires, sigaction() (see sigLib) must be in place in order for the user
to handle the event.  The "high resolution sleep" facility, nanosleep(),
allows sub-second sleeping to the resolution of the clock.

The clockLib library should be installed and clock_settime() set
before the use of any timer routines.

ADDITIONS
Two non-POSIX functions are provided for user convenience:
   
    timer_cancel() quickly disables a timer by calling timer_settime().
    timer_connect() easily hooks up a user routine by calling sigaction().

CLARIFICATIONS
The task creating a timer with timer_create() will receive the
signal no matter which task actually arms the timer.

When a timer expires and the task has previously exited, logMsg()
indicates the expected task is not present.  Similarly, logMsg() indicates
when a task arms a timer without installing a signal handler.  Timers may
be armed but not created or deleted at interrupt level.

IMPLEMENTATION
The actual clock resolution is hardware-specific and in many cases is
1/60th of a second.  This is less than _POSIX_CLOCKRES_MIN, which is
defined as 20 milliseconds (1/50th of a second).

INTERNAL
1. The <timespec> key calculation is limited by the amount of time
represented (years?) and the resolution of the clock (1/1000's?).
2. Request for option of timer deletion when task is deleted.
However, the task that created the timer is not necessarily the
task that is going to receive it (set in timer_settime()).
This could be cleared up, if the task calling timer_create()
was the task to receive the signal.

INCLUDE FILES: timers.h

SEE ALSO: clockLib, sigaction(), POSIX 1003.1b documentation,
.pG "Basic\ OS"
*/

#include "vxWorks.h"
#include "errno.h"
#include "memLib.h"
#include "logLib.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "taskLib.h"
#include "wdLib.h"
#include "intLib.h"
#include "time.h"
#include "tickLib.h"
#include "private/sigLibP.h"
#include "private/timerLibP.h"
#define __PTHREAD_SRC
#include "pthread.h"

extern int clockLibInit ();

int timerLibLog = TRUE;	/* log warning messages from wd handler */

#define	TIMER_RELTIME	(~TIMER_ABSTIME)	/* relative time */


/*******************************************************************************
*
* timer_cancel - cancel a timer
*
* This routine is a shorthand method of invoking timer_settime(), which stops
* a timer.
*
* NOTE
* Non-POSIX.
*
* RETURNS: 0 (OK), or -1 (ERROR) if <timerid> is invalid.
*
* ERRNO: EINVAL
*/

int timer_cancel
    (
    timer_t timerid	/* timer ID */
    )
    {
    struct itimerspec value;

    bzero ((char*)&value, sizeof (value));

    return (timer_settime (timerid, TIMER_RELTIME, &value, NULL));
    }

/*******************************************************************************
*
* timerConHandler - default 'connect' timer handler
*
* User routine wrapper called from timerWdHandler().
*
* RETURNS: N/A
*/

LOCAL void timerConHandler
    (
    int     sig,		/* caught signal */
    timer_t timerid,		/* "int code" */
    struct sigcontext *pContext	/* unused */
    )
    {
    if (timerid == NULL)
	{
	/* sometimes caused by a pended signal ('code' lost when unblocked) */
	if (timerLibLog)
	    {
	    logMsg ("timerConHandler: bad timer %#x, signal %d\n",
		    (int)timerid, sig, 0, 0, 0, 0);
	    }
	}
    else if (timerid->routine == NULL)
	{
	if (timerLibLog)
	    {
	    logMsg ("timerConHandler: expired %#x, signal %d\n",
		    (int)timerid, sig, 0, 0, 0, 0);
	    }
	}
    else
	{
	(*timerid->routine) (timerid, timerid->arg);
	}
    }
/*******************************************************************************
*
* timer_connect - connect a user routine to the timer signal
*
* This routine sets the specified <routine> to be invoked with <arg> when
* fielding a signal indicated by the timer's <evp> signal number, or
* if <evp> is NULL, when fielding the default signal (SIGALRM).
*
* The signal handling routine should be declared as:
* .ne 5
* .CS
*     void my_handler
*         (
*         timer_t timerid,	/@ expired timer ID @/
*         int arg               /@ user argument    @/
*         )
* .CE
*
* NOTE
* Non-POSIX.
*
* RETURNS: 0 (OK), or -1 (ERROR) if the timer is invalid or cannot bind the 
* signal handler.
*
* ERRNO: EINVAL
*/

int timer_connect
    (
    timer_t     timerid,	/* timer ID      */
    VOIDFUNCPTR routine,	/* user routine  */
    int         arg		/* user argument */
    )
    {
    static struct sigaction timerSig;

    if (timerSig.sa_handler == 0)
	{
	/* just the first time */
	timerSig.sa_handler = (void (*)(int))timerConHandler;
	(void) sigemptyset (&timerSig.sa_mask);
	timerSig.sa_flags = 0;	/* !SA_SIGINFO: cause timerid to be passed */
	}

    if (intContext ())
	return (ERROR);

    if (timerid == NULL)
	{
	errno = EINVAL;
	return (ERROR);
	}

    timerid->routine = routine;
    timerid->arg     = arg;

    timerid->sevent.sigev_signo = SIGALRM;
    timerid->sevent.sigev_value.sival_ptr = timerid;	/* !SA_SIGINFO */

    if (sigaction (SIGALRM, &timerSig, NULL) == ERROR)
	return (ERROR);
    else
	return (OK);
    }
/*******************************************************************************
*
* timer_create - allocate a timer using the specified clock for a timing base (POSIX)
*
* This routine returns a value in <pTimer> that identifies the timer 
* in subsequent timer requests.  The <evp> argument, if non-NULL, points to 
* a `sigevent' structure, which is allocated by the application and defines 
* the signal number and application-specific data to be sent to the task when 
* the timer expires.  If <evp> is NULL, a default signal (SIGALRM) is queued 
* to the task, and the signal data is set to the timer ID.  Initially, the 
* timer is disarmed.
*
* RETURNS:
* 0 (OK), or -1 (ERROR) if too many timers already are allocated or the signal 
* number is invalid.
*
* ERRNO: EMTIMERS, EINVAL, ENOSYS, EAGAIN, S_memLib_NOT_ENOUGH_MEMORY
*
* SEE ALSO: timer_delete()
*/

int timer_create
    (
    clockid_t clock_id,		/* clock ID (always CLOCK_REALTIME) */
    struct sigevent *evp,	/* user event handler               */
    timer_t * pTimer		/* ptr to return value 		    */
    )
    {
    timer_t timerid;
    struct sigevent sevp;

    if (intContext ())
	return (ERROR);

    (void)clockLibInit (); /* make sure clock "running" */

    timerid = (timer_t) calloc (1, sizeof(*timerid));

    if (timerid == NULL)
	{
	/* errno = EMTIMERS;  * S_memLib_NOT_ENOUGH_MEMORY will be lost */
	return (ERROR);
	}

    if ((timerid->wdog = wdCreate ()) == NULL)
        {
	free ((char *) timerid);
	return (ERROR);
        }

    timerid->active = FALSE;
    timerid->taskId = taskIdSelf ();

    timerid->clock_id = clock_id;	/* should check for known clock_id? */

    if (evp == NULL)
	{
	sevp.sigev_signo = SIGALRM;
	/* remember value possibly set in timer_connect() */
	/* XXX
	sevp.sigev_value.sival_int = timerid->sevent.sigev_value.sival_int;
	*/
	sevp.sigev_value.sival_ptr = timerid;
	}
    else
	{
	if (evp->sigev_signo < 1 || evp->sigev_signo > _NSIGS)
	    {
	    errno = EINVAL;
	    return (ERROR);
	    }
	sevp = *evp;
	}

    timerid->sevent = sevp;
    timerid->sigpend.sigp_info.si_signo = sevp.sigev_signo;
    timerid->sigpend.sigp_info.si_code  = SI_TIMER;
    timerid->sigpend.sigp_info.si_value = sevp.sigev_value;

    sigPendInit (&timerid->sigpend);

    TV_ZERO(timerid->exp.it_interval);
    TV_ZERO(timerid->exp.it_value);

    *pTimer = timerid;
    return (OK);
    }
/*******************************************************************************
*
* timer_delete - remove a previously created timer (POSIX)
*
* This routine removes a timer.
*
* RETURNS: 0 (OK), or -1 (ERROR) if <timerid> is invalid.
*
* ERRNO: EINVAL
*
* SEE ALSO: timer_create()
*/

int timer_delete
    (
    timer_t timerid	/* timer ID */
    )
    {
    if (intContext ())
	return (ERROR);

    if (timerid == NULL)
	{
	errno = EINVAL;
	return (ERROR);
	}

    if (timer_cancel (timerid) != 0)
	return (ERROR);

    if (wdDelete (timerid->wdog) != OK)
    return (ERROR);

    (void)sigPendDestroy (&timerid->sigpend);

    free ((char *) timerid);

    return (OK);
    }
/*******************************************************************************
*
* timer_gettime - get the remaining time before expiration and the reload value (POSIX)
*
* This routine gets the remaining time and reload value of a specified timer.
* Both values are copied to the <value> structure.
*
* RETURNS: 0 (OK), or -1 (ERROR) if <timerid> is invalid.
*
* ERRNO: EINVAL
*/

int timer_gettime
    (
    timer_t           timerid,	 /* timer ID                       */
    struct itimerspec *value	 /* where to return remaining time */
    )
    {
    struct timespec now; 	 /* current time */ 
    struct timespec timerExpire; /* absolute time that timer will go off */

    if (timerid == NULL || value == NULL)
	{
	errno = EINVAL;
	return (ERROR);
	}

    if (timerid->clock_id != CLOCK_REALTIME)
	{
	errno = EINVAL;
	return (ERROR);
	}

    /* if timer is disarmed simply set value to 0 */
    if (timerid->active == FALSE)
        {
        value->it_value.tv_sec = 0;
        value->it_value.tv_nsec = 0;
        value->it_interval.tv_sec = 0;
        value->it_interval.tv_nsec = 0;
        return (OK);
        }

    /* get current time */
    if (clock_gettime (CLOCK_REALTIME, &now) == ERROR)
	return (ERROR);;

    /* use time stamp and get absolute time that timer will go off */
    timerExpire.tv_sec = timerid->timeStamp.tv_sec + timerid->exp.it_value.tv_sec;
    timerExpire.tv_nsec = timerid->timeStamp.tv_nsec + timerid->exp.it_value.tv_nsec;
    TV_NORMALIZE (timerExpire);

    /* compute difference using current time */
    value->it_value.tv_sec = timerExpire.tv_sec - now.tv_sec;
    value->it_value.tv_nsec = timerExpire.tv_nsec - now.tv_nsec;
    TV_NORMALIZE (value->it_value);

    /* get reload value */
    value->it_interval.tv_sec = timerid->exp.it_interval.tv_sec;
    value->it_interval.tv_nsec = timerid->exp.it_interval.tv_nsec;
    TV_NORMALIZE (value->it_interval);

    return (OK);
    }
/*******************************************************************************
*
* timer_getoverrun - return the timer expiration overrun (POSIX)
*
* This routine returns the timer expiration overrun count for <timerid>,
* when called from a timer expiration signal catcher.  The overrun count is
* the number of extra timer expirations that have occurred, up to the
* implementation-defined maximum _POSIX_DELAYTIMER_MAX.  If the count is
* greater than the maximum, it returns the maximum.
*
* RETURNS:
* The number of overruns, or _POSIX_DELAYTIMER_MAX if the count equals or is
* greater than _POSIX_DELAYTIMER_MAX, or -1 (ERROR) if <timerid> is invalid.
*
* ERRNO: EINVAL, ENOSYS
*/

int timer_getoverrun
    (
    timer_t timerid	/* timer ID */
    )
    {
    if (timerid == NULL)
	{
	errno = EINVAL;
	return (ERROR);
	}

    return (timerid->sigpend.sigp_overruns);
    }
/*******************************************************************************
*
* timerWdHandler - wd handler used by timer_settime
*
* RETURNS: 
*/

LOCAL void timerWdHandler
    (
    timer_t timerid
    )
    {
    ULONG delayTicks;
    int status;

    if (timerid == NULL)
	{
	if (timerLibLog)
	    logMsg ("timerWdHandler: NULL timerid!\n", 0, 0, 0, 0, 0, 0);
	return;
	}

    status = sigPendKill (timerid->taskId, &timerid->sigpend);

    if (status != OK && timerLibLog)
	{
	logMsg ("timerWdHandler: kill failed (timer=%#x, tid=%#x, errno=%#x)\n",
		(int)timerid, timerid->taskId, errno, 0, 0, 0);
	}

    if (TV_ISZERO(timerid->exp.it_interval))
	{
	timerid->active = FALSE;
	}
    else
	{
	/* interval timer needs reloading */

	TV_CONVERT_TO_TICK (delayTicks, timerid->exp.it_interval);

	if (delayTicks < 1)
	    delayTicks = 1;	/* delay of 0 will cause recursion! */

	/* time stamp when we arm interval timer */
	(void) clock_gettime (CLOCK_REALTIME, &(timerid->timeStamp));

	wdStart (timerid->wdog, (int)delayTicks, (FUNCPTR)timerWdHandler,
		(int)timerid);
	}
    }
/*******************************************************************************
*
* timer_settime - set the time until the next expiration and arm timer (POSIX)
*
* This routine sets the next expiration of the timer, using the `.it_value'
* of <value>, thus arming the timer.  If the timer is already armed, this
* call resets the time until the next expiration.  If `.it_value' is zero,
* the timer is disarmed.
*
* If <flags> is not equal to TIMER_ABSTIME, the interval is relative to the
* current time, the interval being the `.it_value' of the <value> parameter.
* If <flags> is equal to TIMER_ABSTIME, the expiration is set to
* the difference between the absolute time of `.it_value' and the current
* value of the clock associated with <timerid>.  If the time has already
* passed, then the timer expiration notification is made immediately.
* The task that sets the timer receives the signal; in other words, the taskId
* is noted.  If a timer is set by an ISR, the signal is delivered to the
* task that created the timer.
*
* The reload value of the timer is set to the value specified by
* the `.it_interval' field of <value>.  When a timer is
* armed with a nonzero `.it_interval' a periodic timer is set up.
*
* Time values that are between two consecutive non-negative integer
* multiples of the resolution of the specified timer are rounded up to
* the larger multiple of the resolution.
*
* If <ovalue> is non-NULL, the routine stores a value representing the
* previous amount of time before the timer would have expired.  Or if the
* timer is disarmed, the routine stores zero, together with the previous
* timer reload value.  The <ovalue> parameter is the same value as that 
* returned by timer_gettime() and is subject to the timer resolution.
*
* WARNING
* If clock_settime() is called to reset the absolute clock time after a timer
* has been set with timer_settime(), and if <flags> is equal to TIMER_ABSTIME, 
* then the timer will behave unpredictably.  If you must reset the absolute
* clock time after setting a timer, do not use <flags> equal to TIMER_ABSTIME.
*
* RETURNS:
* 0 (OK), or -1 (ERROR) if <timerid> is invalid, the number of nanoseconds 
* specified by <value> is less than 0 or greater than or equal to 
* 1,000,000,000, or the time specified by <value> exceeds the maximum 
* allowed by the timer.
*
* ERRNO: EINVAL
*/

int timer_settime
    (
    timer_t                 timerid,	/* timer ID                           */
    int                     flags,	/* absolute or relative               */
    const struct itimerspec *value,	/* time to be set                     */
    struct itimerspec       *ovalue	/* previous time set (NULL=no result) */
    )
    {
    struct timespec now;
    struct timespec timerExpire; /* absolute time that timer will go off */
    ULONG delayTicks;

    if (timerid == NULL)
	{
	errno = EINVAL;
	return (ERROR);
	}

    if (timerid->clock_id != CLOCK_REALTIME || !TV_VALID(value->it_value))
	{
	errno = EINVAL;
	return (ERROR);
	}

    if (ovalue != NULL)
	{

    /* if timer is disarmed, simply set value to 0 */
        if (timerid->active == FALSE)
        {
            ovalue->it_value.tv_sec = 0;
            ovalue->it_value.tv_nsec = 0;
        }
        else
        {

    /* get current time */
            if (clock_gettime (CLOCK_REALTIME, &now) == ERROR)
                return (ERROR);;

    /* use time stamp and get absolute time that timer will go off */
            timerExpire.tv_sec = timerid->timeStamp.tv_sec + timerid->exp.it_value.tv_sec;
            timerExpire.tv_nsec = timerid->timeStamp.tv_nsec + timerid->exp.it_value.tv_nsec;
            TV_NORMALIZE (timerExpire);

    /* compute difference using current time */
            ovalue->it_value.tv_sec = timerExpire.tv_sec - now.tv_sec;
            ovalue->it_value.tv_nsec = timerExpire.tv_nsec - now.tv_nsec;
            TV_NORMALIZE (ovalue->it_value);
        }

    /* get reload value */
        ovalue->it_interval.tv_sec = timerid->exp.it_interval.tv_sec;
        ovalue->it_interval.tv_nsec = timerid->exp.it_interval.tv_nsec;
        TV_NORMALIZE (ovalue->it_interval);

    }

    if (TV_ISZERO(value->it_value))
	{
	if (timerid->active)
	    {
	    timerid->active = FALSE;
	    wdCancel (timerid->wdog);
	    }
	return (OK);
	}

    TV_SET(timerid->exp.it_interval, value->it_interval);
    TV_SET(timerid->exp.it_value, value->it_value);

    if (flags == TIMER_ABSTIME)
	{
	/* convert current to relative time */
	(void) clock_gettime (CLOCK_REALTIME, &now);

	/* could be in the past */

	if (TV_GT(now, timerid->exp.it_value))
	    {
	    TV_ZERO(timerid->exp.it_value);
	    }
	else
	    {
    	    TV_SUB(timerid->exp.it_value, now);
	    }

	/* time stamp when timer is armed */
	TV_SET (timerid->timeStamp, now);
	}
    else
	/* time stamp when timer is armed */
	(void) clock_gettime (CLOCK_REALTIME, &(timerid->timeStamp));

    TV_CONVERT_TO_TICK (delayTicks, timerid->exp.it_value);

    if (timerid->active)
	wdCancel (timerid->wdog);
    else
	timerid->active = TRUE;

    wdStart (timerid->wdog, delayTicks, (FUNCPTR)timerWdHandler, (int)timerid);

    return (OK);
    }
/*******************************************************************************
*
* timer_show - show information on a specified timer
*
* WARNING
* Non-POSIX.
*
* RETURNS: 0 (OK), or -1 (ERROR) if <timerid> is invalid, or the context is 
* invalid.
*
* ERRNO: EINVAL
*
* NOMANUAL
*/

int timer_show
    (
    timer_t timerid	/* timer ID */
    )
    {
    static char *title1 = "task       timerid    evp        routine\n";
    static char *title2 = "---------- ---------- ---------- ----------\n";

    if (intContext ())
	return (ERROR);

    if (timerid == NULL)
	return (OK);

    printf (title1);
    printf (title2);

    printf ("%#10x %#10x %#10x %#10x\n", timerid->taskId, (unsigned int)timerid,
		(unsigned int)&timerid->sevent, (unsigned int)timerid->routine);

    return (OK);
    }
/*******************************************************************************
*
* nanosleep - suspend the current task until the time interval elapses (POSIX)
*
* This routine suspends the current task for a specified time <rqtp>
* or until a signal or event notification is made.
*
* The suspension may be longer than requested due to the rounding up of the
* request to the timer's resolution or to other scheduling activities (e.g.,
* a higher priority task intervenes).
*
* The `timespec' structure is defined as follows:
*
* \cs
* struct timespec
*     {
*                                /@ interval = tv_sec*10**9 + tv_nsec @/
*      time_t tv_sec;            /@ seconds @/
*      long tv_nsec;             /@ nanoseconds (0 - 1,000,000,000) @/
*      };
* \ce
*
* If <rmtp> is non-NULL, the `timespec' structure is updated to contain the
* amount of time remaining.  If <rmtp> is NULL, the remaining time is not
* returned.  The <rqtp> parameter is greater than 0 or less than or equal to
* 1,000,000,000.
*
* RETURNS:
* 0 (OK), or -1 (ERROR) if the routine is interrupted by a signal or an 
* asynchronous event notification, or <rqtp> is invalid.
*
* ERRNO: EINVAL, EINTR
*
* SEE ALSO: sleep(), taskDelay()
*/

int nanosleep
    (
    const struct timespec *rqtp,	/* time to delay                     */
    struct timespec       *rmtp		/* premature wakeup (NULL=no result) */
    )
    {
    int status;
    /* int oldErrno; */
    ULONG delayTicks;
    struct timespec then;
    struct timespec now;
    int returnStatus;
    int savtype;

    if (rqtp == NULL || !TV_VALID(*rqtp))
	{
	errno = EINVAL;
	return (ERROR);
	}

    if (TV_ISZERO(*rqtp))
	return (OK);

    if (_func_pthread_setcanceltype != NULL)
        {
        _func_pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &savtype);
        }

    (void)clockLibInit (); /* make sure clock "running" */

    (void)clock_gettime (CLOCK_REALTIME, &then);

    TV_CONVERT_TO_TICK (delayTicks, *rqtp);

    /* return's 1 (RESTART) if interrupted sleep */

    status = taskDelay (delayTicks);

    if (status == 0)
	returnStatus = 0;
    else
	returnStatus = -1;

    if (rmtp != NULL)
	{
	(void)clock_gettime (CLOCK_REALTIME, &now);

	TV_SUB (now, then);	/* make time relative to start */

	if (TV_LT(now, *rqtp))
	    {
	    TV_SET(*rmtp, *rqtp);
	    TV_SUB(*rmtp, now);
	    }
	else
	    TV_ZERO((*rmtp));
	}

    if (_func_pthread_setcanceltype != NULL)
        {
        _func_pthread_setcanceltype(savtype, NULL);
        }

    return (returnStatus);
    }

/*******************************************************************************
*
* sleep - delay for a specified amount of time
*
* This routine causes the calling task to be blocked for <secs> seconds.
*
* The time the task is blocked for may be longer than requested due to the
* rounding up of the request to the timer's resolution or to other scheduling
* activities (e.g., a higher priority task intervenes).
*
* RETURNS: Zero if the requested time has elapsed, or the number of seconds
* remaining if it was interrupted.
*
* ERRNO: EINVAL, EINTR
*
* SEE ALSO: nanosleep(), taskDelay()
*/
 
unsigned int sleep
    (
    unsigned int secs
    )
    {
    struct timespec ntp, otp;
 
    ntp.tv_sec = secs;
    ntp.tv_nsec = 0;
 
    nanosleep(&ntp, &otp);
 
    return(otp.tv_sec);
    }

/*******************************************************************************
*
* alarm - set an alarm clock for delivery of a signal
*
* This routine arranges for a 'SIGALRM' signal to be delivered to the
* calling task after <secs> seconds.
*
* If <secs> is zero, no new alarm is scheduled. In all cases, any previously
* set alarm is cancelled.
*
* RETURNS: Time remaining until a previously scheduled alarm was due to be
* delivered, zero if there was no previous alarm, or ERROR in case of an
* error.
*/
 
unsigned int alarm
    (
    unsigned int secs
    )
    {
    static timer_t timer_id = NULL;
    struct itimerspec tspec, tremain;
 
    /* if first time, create a timer */
 
    if (!timer_id)
        {
        if (timer_create(CLOCK_REALTIME, NULL, &timer_id) == ERROR)
            return(ERROR);
        }
 
    /* set new time*/
 
    tspec.it_interval.tv_sec = 0;
    tspec.it_interval.tv_nsec = 0;
    tspec.it_value.tv_sec = secs;
    tspec.it_value.tv_nsec = 0;
 
    /* save off timer remaining from previous for return */
 
    timer_gettime(timer_id, &tremain);
    timer_settime(timer_id, CLOCK_REALTIME, &tspec, NULL);
 
    return((unsigned int)tremain.it_value.tv_sec);
    }
