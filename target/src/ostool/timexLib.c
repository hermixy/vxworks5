/* timexLib.c - execution timer facilities */

/* Copyright 1984-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
03x,16oct01,jn   use symFindSymbol for symbol lookup (SPR #7453)
03w,14mar99,jdi  doc: removed refs to config.h and/or configAll.h (SPR 25663).
03v,14feb97,dvs  added error checking in timexShowCalls (SPR #6876).
03v,14feb97,dvs  timexShowCalls now prints func addr if lookup in sym table 
		 fails (SPR #6876).
03u,05may93,caf  tweaked for ansi.
03t,13feb93,kdl  changed cplusLib.h to private/cplusLibP.h (SPR #1917).
03s,20jan93,jdi  documentation cleanup for 5.1.
03r,01aug92,srh  added C++ demangling idiom to timexShowCalls
03q,26may92,rrr  the tree shuffle
03p,01dec91,gae  more ANSI cleanup.
03o,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed VOID to void
		  -changed copyright notice
03n,05apr91,jdi	 documentation -- removed header parens and x-ref numbers;
		 doc review by dnw.
03m,24mar91,jdi  documentation cleanup.
03l,10aug90,dnw  added include of timexLib.h.
03k,10aug90,kdl  added forward declarations for functions returning void.
03j,10jul90,dnw  changed declaration of timexNull from void to int to allow
		   void to be void one day
03i,12mar90,jcf  symbol table type now SYM_TYPE.
03h,07mar90,jdi  documentation cleanup.
03g,03apr89,ecs  made optional arguments explicit in timexFunc, timex, timexN,
	   +gae	    timexPost, timexPre.
03f,20aug88,gae  documentation.
03e,07jul88,jcf  lint.
03d,05jun88,dnw  changed name to timexLib.
03c,30may88,dnw  changed to v4 names.
03b,01may88,gae  added null routine timLib().
03a,27jan88,jcf  made kernel independent.
02s,18nov87,ecs  documentation.
02r,04nov87,ecs  documentation.
02q,08jun87,llk  fixed initialization of tiPreCalls, tiTimeCalls, tiPostCalls.
02p,23mar87,jlf  documentation.
02o,12jan87,llk  corrected information in tihelp.
02n,21dec86,dnw  changed to not get include files from default directories.
02m,02dec86,llk  changed tiHelp to tihelp.
02l,04sep86,jlf  documentation.
02k,27jul86,llk  prints error messages to standard error (uses printErr)
02j,05jun86,dnw  changed sstLib calls to symLib.
02i,24mar86,dnw  de-linted.
02h,19mar86,dnw  fixed calibration to include pushing of args and calling
		   routine.
		 corrected documentation.
		 changed timeN to report how many reps it made.
02g,11oct85,dnw  de-linted.
02f,37aug85,rdc  changed MAX_SYM_LEN to MAX_SYS_SYM_LEN.
02e,21jul85,jlf  documentation.
02d,01jun85,rdc  updated documentation.
02c,14aug84,jlf  changed calls to clkGetRate to sysClkGetRate.
02b,16jul84,ecs  changed to use unix-style format codes.
02a,24jun84,dnw  rewritten
*/


/*
This library contains routines for timing the execution of programs,
individual functions, and groups of functions.  The VxWorks system clock
is used as a time base.  Functions that have a short execution time
relative to this time base can be called repeatedly to establish an
average execution time with an acceptable percentage of error.

Up to four functions can be specified to be timed as a group.
Additionally, sets of up to four functions can be specified as pre- or
post-timing functions, to be executed before and after the timed functions.
The routines timexPre() and timexPost() are used to specify the pre- and
post-timing functions, while timexFunc() specifies the functions to
be timed.

The routine timex() is used to time a single execution of a function or
group of functions.  If called with no arguments, timex() uses the
functions in the lists created by calls to timexPre(), timexPost(), and
timexFunc().  If called with arguments, timex() times the function
specified, instead of the previous list.  The routine timexN() works in
the same manner as timex() except that it iterates the function calls to
be timed.

EXAMPLES
The routine timex() can be used to obtain the execution time of a single
routine:
.CS
    -> timex myFunc, myArg1, myArg2, ...
.CE
The routine timexN() calls a function repeatedly until a 2% or better
tolerance is obtained:
.CS
    -> timexN myFunc, myArg1, myArg2, ...
.CE
The routines timexPre(), timexPost(), and timexFunc() are used to specify
a list of functions to be executed as a group:
.CS
    -> timexPre 0, myPreFunc1, preArg1, preArg2, ...
    -> timexPre 1, myPreFunc2, preArg1, preArg2, ...

    -> timexFunc 0, myFunc1, myArg1, myArg2, ...
    -> timexFunc 1, myFunc2, myArg1, myArg2, ...
    -> timexFunc 2, myFunc3, myArg1, myArg2, ...

    -> timexPost 0, myPostFunc, postArg1, postArg2, ...
.CE
The list is executed by calling timex() or timexN() without arguments:
.CS
    -> timex
.ft 1
or
.ft P
    -> timexN
.CE
In this example, <myPreFunc1> and <myPreFunc2> are called with their
respective arguments.  <myFunc1>, <myFunc2>, and <myFunc3> are then
called in sequence and timed.  If timexN() was used, the sequence is
called repeatedly until a 2% or better error tolerance is achieved.
Finally, <myPostFunc> is called with its arguments.  The timing results
are reported after all post-timing functions are called.

NOTE
The timings measure the execution time of the routine body, without the
usual subroutine entry and exit code (usually LINK, UNLINK, and RTS
instructions).  Also, the time required to set up the arguments and
call the routines is not included in the reported times.  This is
because these timing routines automatically calibrate themselves by
timing the invocation of a null routine, and thereafter subtracting
that constant overhead.

INCLUDE FILES: timexLib.h

SEE ALSO: spyLib
*/

#include "vxWorks.h"
#include "symLib.h"
#include "stdio.h"
#include "string.h"
#include "sysSymTbl.h"
#include "sysLib.h"
#include "tickLib.h"
#include "timexLib.h"
#include "private/cplusLibP.h"

#define MAX_CALLS	4		/* max functions in each category */
#define MAX_ARGS	8		/* max args to each function */

#define MAX_PERCENT	2		/* max percent error for auto-timing */
#define MAX_REPS	20000		/* max reps for auto-timing */

#define TIMEX_DEMANGLE_PRINT_LEN 256

typedef struct			/* CALL */
    {
    VOIDFUNCPTR func;			/* function to call */
    int arg [MAX_ARGS];			/* args to function */
    } CALL;

typedef CALL CALL_ARRAY [MAX_CALLS];

LOCAL char *timexScaleText [] =
    {
    "secs",			/* scale = 0 */
    "millisecs",		/* scale = 1 */
    "microsecs",		/* scale = 2 */
    };


static void timexNull (void);		/* forward declaration */

LOCAL CALL_ARRAY timexPreCalls =	/* calls to be made just before timing*/
    {
    { timexNull },
    { timexNull },
    { timexNull },
    { timexNull },
    };

LOCAL CALL_ARRAY timexTimeCalls =	/* calls to be timed */
    {
    { timexNull },
    { timexNull },
    { timexNull },
    { timexNull },
    };

LOCAL CALL_ARRAY timexPostCalls =	/* calls to be made just after timing */
    {
    { timexNull },
    { timexNull },
    { timexNull },
    { timexNull },
    };

LOCAL CALL_ARRAY timexNullCalls =
    {
    { timexNull },
    { timexNull },
    { timexNull },
    { timexNull },
    };

LOCAL int overhead;		/* usecs of overhead per rep in timing test */


/* forward static functions */

static void timexAddCall (CALL *callArray, int i, FUNCPTR func, int arg1, int
		arg2, int arg3, int arg4, int arg5, int arg6, int arg7,
		int arg8);
static void timexAutoTime (CALL_ARRAY preCalls, CALL_ARRAY timeCalls,
		CALL_ARRAY postCalls, int *pNreps, int *pScale, int *pTime,
		int *pError, int *pPercent);
static void timexCal (void);
static void timexClrArrays (void);
static void timexMakeCalls (CALL_ARRAY calls);
static void timexScale (int ticks, int reps, int *pScale, int *pTime, int
		*pError, int *pPercent);
static void timexShowCalls (CALL_ARRAY calls);
static void timexTime (int reps, CALL_ARRAY preCalls, CALL_ARRAY timeCalls,
		CALL_ARRAY postCalls, int *pScale, int *pTime, int *pError,
		int *pPercent);


/*******************************************************************************
*
* timexInit - include the execution timer library
*
* This null routine is provided so that timexLib can be linked into the system.
* If the configuration macro INCLUDE_TIMEX is defined, it is called by the
* root task, usrRoot(), in usrConfig.c.
*
* RETURNS: N/A
*/

void timexInit (void)
    {
    }
/*******************************************************************************
*
* timexClear - clear the list of function calls to be timed
*
* This routine clears the current list of functions to be timed.
*
* RETURNS: N/A
*/

void timexClear (void)
    {
    timexClrArrays ();
    timexShow ();
    }
/*******************************************************************************
*
* timexFunc - specify functions to be timed
*
* This routine adds or deletes functions in the list of functions
* to be timed as a group by calls to timex() or timexN().  Up to four
* functions can be included in the list.  The argument <i> specifies the
* function's position in the sequence of execution (0, 1, 2, or 3).
* A function is deleted by specifying its sequence number <i> and NULL for the
* function argument <func>.
*
* RETURNS: N/A
*
* SEE ALSO: timex(), timexN()
*/

void timexFunc
    (
    int i,        /* function number in list (0..3)               */
    FUNCPTR func, /* function to be added (NULL if to be deleted) */
    int arg1,     /* first of up to 8 args to call function with */ 
    int arg2,
    int arg3,
    int arg4,
    int arg5,
    int arg6,
    int arg7,
    int arg8
    )
    {
    timexAddCall (timexTimeCalls, i, func, arg1, arg2, arg3, arg4, arg5, arg6,
		  arg7, arg8);
    timexShow ();
    }
/*******************************************************************************
*
* timexHelp - display synopsis of execution timer facilities
*
* This routine displays the following summary of the available
* execution timer functions:
* .CS
*     timexHelp                      Print this list.
*     timex       [func,[args...]]   Time a single execution.
*     timexN      [func,[args...]]   Time repeated executions.
*     timexClear                     Clear all functions.
*     timexFunc   i,func,[args...]   Add timed function number i (0,1,2,3).
*     timexPre    i,func,[args...]   Add pre-timing function number i.
*     timexPost   i,func,[args...]   Add post-timing function number i.
*     timexShow                      Show all functions to be called.
*
*     Notes:
*       1) timexN() will repeat calls enough times to get
*          timing accuracy to approximately 2%.
*       2) A single function can be specified with timex() and timexN();
*          or, multiple functions can be pre-set with timexFunc().
*       3) Up to 4 functions can be pre-set with timexFunc(),
*          timexPre(), and timexPost(), i.e., i in the range 0 - 3.
*       4) timexPre() and timexPost() allow locking/unlocking, or
*          raising/lowering priority before/after timing.
* .CE
*
* RETURNS: N/A
*/

void timexHelp (void)
    {
    static char *helpMsg [] =
    {
    "timexHelp                      Print this list.",
    "timex       [func,[args...]]   Time a single execution.",
    "timexN      [func,[args...]]   Time repeated executions.",
    "timexClear                     Clear all functions.",
    "timexFunc   i,func,[args...]   Add timed function number i (0,1,2,3).",
    "timexPre    i,func,[args...]   Add pre-timing function number i.",
    "timexPost   i,func,[args...]   Add post-timing function number i.",
    "timexShow                      Show all functions to be called.\n",
    "Notes:",
    "  1) timexN() will repeat calls enough times to get",
    "     timing accuracy to approximately 2%.",
    "  2) A single function can be specified with timex() and timexN();",
    "     or, multiple functions can be pre-set with timexFunc().",
    "  3) Up to 4 functions can be pre-set with timexFunc(),",
    "     timexPre(), and timexPost(), i.e., i in the range 0 - 3.",
    "  4) timexPre() and timexPost() allow locking/unlocking, or",
    "     raising/lowering priority before/after timing.",
    NULL,	/* list terminator */
    };

    FAST char **ppMsg;

    printf ("\n");
    for (ppMsg = helpMsg; *ppMsg != NULL; ++ppMsg)
	printf ("%s\n", *ppMsg);
    }
/*******************************************************************************
*
* timex - time a single execution of a function or functions
*
* This routine times a single execution of a specified function with up to
* eight of the function's arguments.  If no function is specified, it times
* the execution of the current list of functions to be timed,
* which is created using timexFunc(), timexPre(), and timexPost().
* If timex() is executed with a function argument, the entire current list
* is replaced with the single specified function.
*
* When execution is complete, timex() displays the execution time.
* If the execution was so fast relative to the clock rate that the time is
* meaningless (error > 50%), a warning message is printed instead.  In such
* cases, use timexN().
*
* RETURNS: N/A
*
* SEE ALSO: timexFunc(), timexPre(), timexPost(), timexN()
*/

void timex
    (
    FUNCPTR func,   /* function to time (optional)                 */
    int arg1,       /* first of up to 8 args to call function with (optional) */
    int arg2,
    int arg3,
    int arg4,
    int arg5,
    int arg6,
    int arg7,
    int arg8
    )
    {
    int scale;
    int time;
    int error;
    int percent;

    /* if function specified, clear any existing functions and add this one */

    if (func != NULL)
	{
	timexClrArrays ();
	timexAddCall (timexTimeCalls, 0, func, arg1, arg2, arg3, arg4, arg5,
		      arg6, arg7, arg8);
	}


    /* calibrate if necessary */

    if (overhead == 0)
	timexCal ();


    /* time calls */

    timexTime (1, timexPreCalls, timexTimeCalls, timexPostCalls,
	    &scale, &time, &error, &percent);

    if (percent > 50)
	{
	printErr (
	    "timex: execution time too short to be measured meaningfully\n");
	printErr ("       in a single execution.\n");
	printErr ("       Type \"timexN\" to time repeated execution.\n");
	printErr ("       Type \"timexHelp\" for more information.\n");
	}
    else
	printErr ("timex: time of execution = %d +/- %d (%d%%) %s\n",
		  time, error, percent, timexScaleText [scale]);
    }
/*******************************************************************************
*
* timexN - time repeated executions of a function or group of functions
*
* This routine times the execution of the current list of functions to be
* timed in the same manner as timex(); however, the list of functions
* is called a variable number of times until sufficient resolution
* is achieved to establish the time with an error less than 2%. (Since each
* iteration of the list may be measured to a resolution of +/- 1 clock
* tick, repetitive timings decrease this error to 1/N ticks, where N is
* the number of repetitions.)
*
* RETURNS: N/A
*
* SEE ALSO: timexFunc(), timex()
*/

void timexN
    (
    FUNCPTR func,  /* function to time (optional)                   */
    int arg1,      /* first of up to 8 args to call function with */
    int arg2,
    int arg3,
    int arg4,
    int arg5,
    int arg6,
    int arg7,
    int arg8
    )
    {
    int scale;
    int time;
    int error;
    int nreps;
    int percent;

    /* if function specified, clear any existing functions and add this one */

    if (func != NULL)
	{
	timexClrArrays ();
	timexAddCall (timexTimeCalls, 0, func, arg1, arg2, arg3, arg4, arg5,
		      arg6, arg7, arg8);
	}


    /* calibrate if necessary then time calls */

    if (overhead == 0)
	timexCal ();

    timexAutoTime (timexPreCalls, timexTimeCalls, timexPostCalls,
		   &nreps, &scale, &time, &error, &percent);

    printErr ("timex: %d reps, time per rep = %d +/- %d (%d%%) %s\n",
	      nreps, time, error, percent, timexScaleText [scale]);
    }
/*******************************************************************************
*
* timexPost - specify functions to be called after timing
*
* This routine adds or deletes functions in the list of functions to be
* called immediately following the timed functions.  A maximum of four
* functions may be included.  Up to eight arguments may be passed to each
* function.
*
* RETURNS: N/A
*/

void timexPost
    (
    int i,        /* function number in list (0..3)                */
    FUNCPTR func, /* function to be added (NULL if to be deleted)  */
    int arg1,     /* first of up to 8 args to call function with */
    int arg2,
    int arg3,
    int arg4,
    int arg5,
    int arg6,
    int arg7,
    int arg8
    )
    {
    timexAddCall (timexPostCalls, i, func, arg1, arg2, arg3, arg4, arg5, arg6,
		  arg7, arg8);
    timexShow ();
    }
/*******************************************************************************
*
* timexPre - specify functions to be called prior to timing
*
* This routine adds or deletes functions in the list of functions to be
* called immediately prior to the timed functions.  A maximum of four
* functions may be included.  Up to eight arguments may be passed to each
* function.
*
* RETURNS: N/A
*/

void timexPre
    (
    int i,        /* function number in list (0..3)                */
    FUNCPTR func, /* function to be added (NULL if to be deleted)  */
    int arg1,     /* first of up to 8 args to call function with */
    int arg2,
    int arg3,
    int arg4,
    int arg5,
    int arg6,
    int arg7,
    int arg8
    )
    {
    timexAddCall (timexPreCalls, i, func, arg1, arg2, arg3, arg4, arg5, arg6,
		  arg7, arg8);
    timexShow ();
    }
/*******************************************************************************
*
* timexShow - display the list of function calls to be timed
*
* This routine displays the current list of function calls to be timed.
* These lists are created by calls to timexPre(), timexFunc(), and
* timexPost().
*
* RETURNS: N/A
*
* SEE ALSO: timexPre(), timexFunc(), timexPost()
*/

void timexShow (void)
    {
    printf ("\ntimex:");
    printf ("\n    pre-calls:\n");
    timexShowCalls (timexPreCalls);
    printf ("\n    timed calls:\n");
    timexShowCalls (timexTimeCalls);
    printf ("\n    post-calls:\n");
    timexShowCalls (timexPostCalls);
    }
/*******************************************************************************
*
* timexAddCall - enter a call in a call array
*
* RETURNS: N/A.
*/

LOCAL void timexAddCall
    (
    CALL *callArray,    /* array to be altered                     */
    int i,        /* function number in list (0..3)                */
    FUNCPTR func, /* function to be added (NULL if to be deleted)  */
    int arg1,     /* first of up to 8 params to call function with */
    int arg2,
    int arg3,
    int arg4,
    int arg5,
    int arg6,
    int arg7,
    int arg8
    )
    {
    if (func == NULL)
	func = (FUNCPTR) timexNull;	/* func should be declared FUNCPTR? */

    if (i >= 0 && i < MAX_CALLS)
	{
	callArray[i].func   = (VOIDFUNCPTR) func;	/* func should be declared FUNCPTR? */
	callArray[i].arg[0] = arg1;
	callArray[i].arg[1] = arg2;
	callArray[i].arg[2] = arg3;
	callArray[i].arg[3] = arg4;
	callArray[i].arg[4] = arg5;
	callArray[i].arg[5] = arg6;
	callArray[i].arg[6] = arg7;
	callArray[i].arg[7] = arg8;
	}
    else
	printf ("timex: call number must be in range 0..%d\n", MAX_CALLS - 1);
    }
/*******************************************************************************
*
* timexAutoTime - time specified function calls with automatic scaling
*
* This routine performs the specified timing, dynamically increasing
* the number of reps until the desired accuracy is achieved.
*
* RETURNS: N/A.
*/

LOCAL void timexAutoTime
    (
    CALL_ARRAY preCalls,        /* list of functions to call before timing */
    FAST CALL_ARRAY timeCalls,  /* list of functions to time */
    CALL_ARRAY postCalls,       /* list of functions to call after timing */
    int *pNreps,        /* ptr where to return number of times called */
    int *pScale,        /* ptr where to return scale:
                         *   0 = secs, 1 = millisecs, 2 = microsecs */
    int *pTime,         /* ptr where to return time per rep in above units */
    int *pError,        /* ptr where to return error margin in above units */
    int *pPercent       /* ptr where to return percent error (0..100) */
    )
    {
    FAST int reps;

    /* start with one rep then increase reps until it takes a long
     * enough interval to provide sufficient resolution */

    reps = 1;

    timexTime (reps, preCalls, timeCalls, postCalls,
    	       pScale, pTime, pError, pPercent);

    while ((*pPercent > MAX_PERCENT) && ((reps < MAX_REPS) || (*pTime > 0)))
	{
	reps = reps * (*pPercent) / MAX_PERCENT;

	timexTime (reps, preCalls, timeCalls, postCalls,
		   pScale, pTime, pError, pPercent);
	}

    *pNreps = reps;
    }
/*******************************************************************************
*
* timexCal - calibrate timex by timing null functions
*
* This routine establishes the constant per rep overhead in the timing
* function.
*
* RETURNS: N/A.
*/

LOCAL void timexCal (void)
    {
    int scale;
    int time;
    int error;
    int percent;
    int nreps;

    overhead = 0;

    timexAutoTime (timexNullCalls, timexNullCalls, timexNullCalls,
		   &nreps, &scale, &time, &error, &percent);

    overhead = time;
    }
/*******************************************************************************
*
* timexClrArrays - clear out function arrays
*
* RETURNS: N/A.
*/

LOCAL void timexClrArrays (void)
    {
    bcopy ((char *) timexNullCalls, (char *) timexPreCalls,
	   sizeof (timexNullCalls));
    bcopy ((char *) timexNullCalls, (char *) timexTimeCalls,
	   sizeof (timexNullCalls));
    bcopy ((char *) timexNullCalls, (char *) timexPostCalls,
	   sizeof (timexNullCalls));
    }
/*******************************************************************************
*
* timexMakeCalls - make function calls in specified call array
*
* RETURNS: N/A.
*/

LOCAL void timexMakeCalls
    (
    CALL_ARRAY calls    /* list of functions to call */
    )
    {
    FAST int ix;
    FAST CALL *pCall = &calls[0];

    for (ix = 0; ix < MAX_CALLS; ix++, pCall++)
	{
	(* pCall->func) (pCall->arg[0], pCall->arg[1], pCall->arg[2],
			 pCall->arg[3], pCall->arg[4], pCall->arg[5],
			 pCall->arg[6], pCall->arg[7]);
	}
    }
/*******************************************************************************
*
* timexNull - null routine
*
* This routine is used as a place holder for null routines in the
* timing function arrays.  It is used to guarantee a constant calling
* overhead in each iteration.
*
* RETURNS: N/A.
*/

LOCAL void timexNull (void)
    {
    }
/*******************************************************************************
*
* timexScale - scale raw timing data
*
* RETURNS: N/A.
*/

LOCAL void timexScale
    (
    int ticks,          /* total ticks required for all reps */
    int reps,           /* number of reps performed */
    FAST int *pScale,   /* ptr where to return scale:
                         *   0 = secs, 1 = millisecs, 2 = microsecs */
    FAST int *pTime,    /* ptr where to return time per rep in above units */
    int *pError,        /* ptr where to return error margin in above units */
    int *pPercent       /* ptr where to return percent error (0..100) */
    )
    {
    FAST int scalePerSec;

    /* calculate time per rep in best scale */

    *pScale = 0;		/* start with "seconds" scale */
    scalePerSec = 1;

    *pTime = ticks * scalePerSec / sysClkRateGet () / reps;

    while ((*pScale < 2) && (*pTime < 100))
	{
	(*pScale)++;
	scalePerSec = scalePerSec * 1000;
	*pTime = ticks * scalePerSec / sysClkRateGet () / reps;
	}


    /* adjust for overhead if in microseconds scale */

    if (*pScale == 2)
	{
	*pTime -= overhead;
	if (*pTime < 0)
	    *pTime = 0;
	}


    /* calculate error */

    *pError = scalePerSec / sysClkRateGet () / reps;

    if (*pTime == 0)
	*pPercent = 100;
    else
	*pPercent = 100 * *pError / *pTime;
    }
/*******************************************************************************
*
* timexShowCalls - print specified call array
*
* RETURNS: N/A.
*/

LOCAL void timexShowCalls
    (
    CALL_ARRAY calls            /* list of functions to be displayed */
    )
    {
    char      *name;            /* pointer to sym table's copy of name string */
    char      demangled [TIMEX_DEMANGLE_PRINT_LEN + 1];
    char      *nameToPrint;
    void      *value;
    SYMBOL_ID symId;            /* symbol identifier */
    int       offset;
    int       i;
    int       j;
    int       arg;
    int       ncalls = 0;

    for (i = 0; i < MAX_CALLS; i++)
	{
	if (calls[i].func != timexNull)
	    {
	    ncalls++;

	    if ((symFindSymbol (sysSymTbl, NULL, (void *)calls[i].func, 
				SYM_MASK_NONE, SYM_MASK_NONE, 
				&symId) != OK) ||
		(symNameGet (symId, &name) != OK) ||
		(symValueGet (symId, &value) != OK))
		printf ("        %d: 0x%x (", i, (int)calls[i].func);
	    else
		{
	        offset = (int) calls[i].func - (int) value;
	        nameToPrint = cplusDemangle (name, demangled, 
					     sizeof (demangled));
	        if (offset == 0)
		    printf ("        %d: %s (", i, nameToPrint);
	        else
		    printf ("        %d: %s+%x (", i, nameToPrint, offset);
		}

	    for (j = 0; j < MAX_ARGS; j++)
		{
		if (j != 0)
		    printf (", ");

		arg = calls[i].arg[j];
		if ((-9 <= arg) && (arg <= 9))
		    printf ("%d", arg);
		else
		    printf ("0x%x", arg);
		}
	    printf (")\n");
	    }
	}

    if (ncalls == 0)
	printf ("        (none)\n");
    }
/*******************************************************************************
*
* timexTime - time a specified number of reps
*
* RETURNS: N/A.
*/

LOCAL void timexTime
    (
    FAST int reps,              /* number of reps to perform */
    CALL_ARRAY preCalls,        /* list of functions to call before timing */
    FAST CALL_ARRAY timeCalls,  /* list of functions to be timed */
    CALL_ARRAY postCalls,       /* list of functions to call after timing */
    int *pScale,        /* ptr where to return scale:
                         *   0 = secs, 1 = millisecs, 2 = microsecs */
    int *pTime,         /* ptr where to return time per rep in above units */
    int *pError,        /* ptr where to return error margin in above units */
    int *pPercent       /* ptr where to return percent error (0..100) */
    )
    {
    int start;
    int end;
    FAST int i;

    timexMakeCalls (preCalls);

    start = (int) tickGet ();

    for (i = 0; i < reps; i++)
	timexMakeCalls (timeCalls);

    end = (int) tickGet ();

    timexMakeCalls (postCalls);

    timexScale (end - start, reps, pScale, pTime, pError, pPercent);
    }
