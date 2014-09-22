/* semOLib.c - release 4.x binary semaphore library */

/* Copyright 1984-1998 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01v,09nov01,dee  add CPU_FAMILY != COLDFIRE in portable test
01u,04sep98,cdp  make ARM CPUs with ARM_THUMB==TRUE use portable routines.
01u,03mar00,zl   merged SH support into T2
01t,17feb98,cdp  added ARM to list of optimised CPUs.
01s,19mar95,dvs  removed tron references.
01r,09jun93,hdn  added a support for I80X86
01q,20jan93,jdi  documentation cleanup for 5.1.
01p,28jul92,jcf  semO{Create,Init} call semOLibInit for robustness.
01o,06jul92,kdl  fixed semOLibInit() to use correct table offset (SEM_TYPE_OLD).
01n,04jul92,jcf  added semOLibInit() to fill in semLib tables.
01m,15jun92,jcf  scalability addressed.
01l,26may92,rrr  the tree shuffle
01k,15sep91,ajm  added MIPS to list of optimized CPU's
01j,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -fixed #else and #endif
		  -changed copyright notice
01i,26sep91,hdn  added conditional flag for TRON optimized code.
01h,05apr91,jdi	 documentation -- removed header parens and x-ref numbers;
		 doc review by jcf.
01g,24mar91,jdi  documentation cleanup.
01f,28sep90,jcf  documentation.
01e,03aug90,jcf  documentation.
01d,05jul90,jcf  optimized version now available.
01c,26jun90,jcf  merged into one semaphore class.
		 fixed stack usagae error in semClear.
01b,10may90,jcf  fixed optimized version of semClear.
01a,20oct89,jcf  written based on v1g of semLib.
*/

/*
DESCRIPTION
This library is provided for backward compatibility with VxWorks 
4.x semaphores.  The semaphores are identical to 5.0 
binary semaphores, except that timeouts -- missing or specified -- 
are ignored.

For backward compatibility, semCreate() operates as before, allocating and
initializing a 4.x-style semaphore.  Likewise, semClear() has been implemented
as a semTake(), with a timeout of NO_WAIT.

For more information on of the behavior of binary semaphores, see
the manual entry for semBLib.

INCLUDE FILES: semLib.h

SEE ALSO: semLib, semBLib,
.pG "Basic OS"
*/

#include "vxWorks.h"
#include "private/objLibP.h"
#include "private/semLibP.h"
#include "private/windLibP.h"

/* optimized version available for 680X0, MIPS, i86, SH, */
/* COLDFIRE and ARM (excluding Thumb) */

#if (defined(PORTABLE) || \
     ((CPU_FAMILY != MC680X0) && \
      (CPU_FAMILY != MIPS) && \
      (CPU_FAMILY != I80X86) && \
      (CPU_FAMILY != SH) && \
      (CPU_FAMILY != COLDFIRE) && \
      (CPU_FAMILY != ARM)) || \
     ((CPU_FAMILY == ARM) && ARM_THUMB))
#define semOLib_PORTABLE
#endif

/* locals */

LOCAL BOOL	semOLibInstalled;	/* protect from muliple inits */


/*******************************************************************************
*
* semOLibInit - initialize the old sytle semaphore management package
* 
* SEE ALSO: semLibInit(1).
* NOMANUAL
*/

STATUS semOLibInit (void)

    {
    if (!semOLibInstalled)
	{
	semGiveTbl [SEM_TYPE_OLD]		= (FUNCPTR) semBGive;
	semTakeTbl [SEM_TYPE_OLD]		= (FUNCPTR) semOTake;
	semFlushTbl [SEM_TYPE_OLD]		= (FUNCPTR) semQFlush;
	semGiveDeferTbl [SEM_TYPE_OLD]		= (FUNCPTR) semBGiveDefer;
	semFlushDeferTbl [SEM_TYPE_OLD]		= (FUNCPTR) semQFlushDefer;

	if (semLibInit () == OK)
	    semOLibInstalled = TRUE;
	}

    return ((semOLibInstalled) ? OK : ERROR);
    }

/*******************************************************************************
*
* semCreate - create and initialize a release 4.x binary semaphore
*
* This routine allocates a VxWorks 4.x binary semaphore.  The semaphore is
* initialized to empty.  After initialization, it must be given before it
* can be taken.
*
* RETURNS: The semaphore ID, or NULL if memory cannot be allocated.
*
* SEE ALSO: semInit()
*/

SEM_ID semCreate (void)
    {
    SEM_ID semId; 

    if ((!semOLibInstalled) && (semOLibInit () != OK))	/* initialize package */
	return (NULL);

    if ((semId = semBCreate (SEM_Q_PRIORITY, SEM_EMPTY)) != NULL)
	semId->semType = SEM_TYPE_OLD;		/* change type to OLD */

    return (semId);
    }

/******************************************************************************
*
* semInit - initialize a static binary semaphore
*
* This routine initializes static VxWorks 4.x semaphores.  In some
* instances, a semaphore cannot be created with semCreate() but is a static
* object.
*
* RETURNS: OK, or ERROR if the semaphore cannot be initialized.
*
* SEE ALSO: semCreate()
*/

STATUS semInit
    (
    SEMAPHORE *pSemaphore       /* 4.x semaphore to initialize */
    )
    {
    if ((!semOLibInstalled) && (semOLibInit () != OK))	/* initialize package */
	return (ERROR);

    if (semBInit (pSemaphore, SEM_Q_PRIORITY, SEM_EMPTY) != OK)
	return (ERROR);

    pSemaphore->semType = SEM_TYPE_OLD;

    return (OK);
    }

#ifdef semOLib_PORTABLE

/*******************************************************************************
*
* semOTake - take semaphore
*
* Takes the semaphore.  If the semaphore is empty (it has not been given
* since the last semTake or semInit), this task will become pended until
* the semaphore becomes available by some other task doing a semGive()
* of it.  If the semaphore is already available, this call will empty
* the semaphore, so that no other task can take it until this task gives
* it back, and this task will continue running.
*
* WARNING
* This routine may not be used from interrupt level.
*
* NOMANUAL
*/

STATUS semOTake
    (
    SEM_ID semId        /* semaphore ID to take */
    )
    {
    return (semBTake (semId, WAIT_FOREVER));
    }

/******************************************************************************
*
* semClear - take a release 4.x semaphore, if the semaphore is available
*
* This routine takes a VxWorks 4.x semaphore if it is available (full), 
* otherwise no action is taken except to return ERROR.  This routine never 
* preempts the caller.
*
* RETURNS: OK, or ERROR if the semaphore is unavailable.
*/

STATUS semClear
    (
    SEM_ID semId        /* semaphore ID to empty */
    )
    {
    return (semBTake (semId, NO_WAIT));
    }

#endif	/* semOLib_PORTABLE */
