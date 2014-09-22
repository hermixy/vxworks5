/* wvTmrLib.c - timer library (WindView) */

/* Copyright 1984-1998 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01i,05oct98,jmp  doc: fixed SEE ALSO section.
01h,28may98,dgp  clean up man pages for WV 2.0 beta release
01g,15apr98,cjtc WV2.0 i960 port
01f,03feb95,rhp  docn improvements from last printed man pages
01e,01feb95,rhp  library man page: add ref to User's Guide
01d,05apr94,smb  and more documentation changes
01c,07mar94,smb  more documentation changes
01b,17jan94,smb  documentation changes
01a,10dec93,smb  created
*/

/*
DESCRIPTION
This library allows a WindView timestamp timer to be registered.  When this
timer is enabled, events are tagged with a timestamp as they are logged.

Seven routines are required: a timestamp routine, a timestamp routine that
guarantees interrupt lockout, a routine that enables the timer driver, a
routine that disables the timer driver, a routine that specifies the
routine to run when the timer hits a rollover, a routine that returns the
period of the timer, and a routine that returns the frequency of the
timer.

INCLUDE FILES:

SEE ALSO: wvLib,
.I WindView User's Guide
*/

/* includes */

#include "vxWorks.h"
#include "wvTmrLib.h"
#include "private/eventP.h"


#if CPU_FAMILY == I960

/* locals */

LOCAL FUNCPTR _func_tmrFreqRaw;		/* original timestamp freq routine */


/* forward declarations */

LOCAL void 	wvI960TmrSelect (void);
LOCAL UINT32 	wvI960TmrFreq (void);

#endif


/*******************************************************************************
*
* wvTmrRegister - register a timestamp timer (WindView)
*
* This routine registers a timestamp routine for each of the following: 
* .iP <wvTmrRtn>
* a timestamp routine, which returns a timestamp when called (must be called
* with interrupts locked).
* .iP <wvTmrLockRtn>
* a timestamp routine, which returns a timestamp when called (locks interrupts).
* .iP <wvTmrEnable>
* an enable-timer routine, which enables the timestamp timer.
* .iP <wvTmrDisable>
* a disable-timer routine, which disables the timestamp timer.
* .iP <wvTmrConnect>
* a connect-to-timer routine, which connects a handler to be run when the timer
* rolls over; this routine should return NULL if the system clock tick is to be
* used.
* .iP <wvTmrPeriod>
* a period-of-timer routine, which returns the period of the timer.
* .iP <wvTmrFreq>
* a frequency-of-timer routine, which returns the frequency of the timer.
* .LP
*
* If any of these routines is set to NULL, the behavior of instrumented code 
* is undefined.
*
* RETURNS: N/A
*/

void wvTmrRegister 
    (
    UINTFUNCPTR wvTmrRtn,		/* timestamp routine */
    UINTFUNCPTR wvTmrLockRtn, 		/* locked timestamp routine */
    FUNCPTR wvTmrEnable, 		/* enable timer routine */
    FUNCPTR wvTmrDisable,		/* disable timer routine */
    FUNCPTR wvTmrConnect,		/* connect to timer routine */
    UINTFUNCPTR wvTmrPeriod,		/* period of timer routine */
    UINTFUNCPTR wvTmrFreq		/* frequency of timer routine */
    )
    {
    /* initialize function pointers for windview timer */
    _func_tmrStamp =     (FUNCPTR) wvTmrRtn;
    _func_tmrStampLock = (FUNCPTR) wvTmrLockRtn;
    _func_tmrEnable =    wvTmrEnable;
    _func_tmrDisable =   wvTmrDisable;
    _func_tmrConnect =   wvTmrConnect;
    _func_tmrFreq =      (FUNCPTR) wvTmrFreq;
    _func_tmrPeriod =    (FUNCPTR) wvTmrPeriod;

#   if CPU_FAMILY==I960
	wvI960TmrSelect ();
#   endif
    }

#if CPU_FAMILY==I960

/*******************************************************************************
*
* wvi960I960TmrSelect - select a timestamp frequency function (windView)
*
* This routine is included for I960 targets only.
* Some I960 timestamp drivers only have the ability to return the timestamp
* operating frequency if the timestamp is enabled. Attempting to read the
* timestamp frequency whilst the timestamp is disabled yields an invalid
* result.
*
* In order to determine which type the linked timestamp driver is, this routine
* calls it twice, once disabled, then enabled and compared the results. If the
* results are the same, the timestamp driver can be called as normal. If the 
* results differ, this routine links the pointer _func_tmrFreq to another
* routine which will enable the timestamp before reading its frequency.
*
* RETURNS: N/A
*/

LOCAL void wvI960TmrSelect (void)
    {
    UINT32 wvTimestampFreqDisabled;
    UINT32 wvTimestampFreqEnabled;

    (* _func_tmrDisable) ();
    wvTimestampFreqDisabled = (* _func_tmrFreq) ();
    (*_func_tmrEnable) ();
    wvTimestampFreqEnabled = (* _func_tmrFreq) ();
    (* _func_tmrDisable) ();

    if (wvTimestampFreqDisabled != wvTimestampFreqEnabled)
	{
	/* save old function pointer to link to */

	_func_tmrFreqRaw = _func_tmrFreq;
	_func_tmrFreq = (FUNCPTR) wvI960TmrFreq;
	}
    }

/*******************************************************************************
*
* wvI960TmrFreq - obtain timestamp frequency for i960 targets (windView)
*
* This routine is included for I960 targets only.
* It is linked to by wvI960TmrSelect for i960 targets in which the timestamp
* driver is unable to return a valid timestamp frequency if the timestamp is
* disabled. It will briefly enable the timestamp, collect the frequency then
* disable the timestamp again.
*
* RETURNS: an unsigned integer representing the timestamp frequency in Hz.
*/

LOCAL UINT32 wvI960TmrFreq (void)
    {
    UINT32 timestamp;

    (* _func_tmrEnable) ();
    timestamp = (* _func_tmrFreqRaw) ();
    (* _func_tmrDisable) ();
    return (timestamp);
    }

#endif
