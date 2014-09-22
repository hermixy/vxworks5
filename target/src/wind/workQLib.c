/* workQLib.c - wind work queue library */

/* Copyright 1988-1998 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01u,09nov01,dee  add CPU_FAMILY != COLDFIRE
01t,03mar00,zl   merged SH support into T2
01u,18dec00,pes  Correct compiler warnings
01t,09oct00,pai  added Ping's (pfl) fix for scheduler problem, spr 28648
01s,29oct97,cth  removed references to scrPad for WV2.0 for real
01r,31jul97,nps  WindView 2.0 - remove reference to scratchPad.
01t,18feb98,cdp  undo 01s: put ARM back in list of optimised CPUs.
01s,21oct97,kkk  undo 01r, take out ARM from list of optimized CPUs.
01r,22apr97,jpd  added ARM to list of non-portable CPUs.
01q,24jun96,sbs  made windview instrumentation conditionally compiled
01n,21mar95,dvs  removed tron references.
01m,09jun93,hdn  added a support for I80X86
01p,03nov94,rdc  changed where interrupts get locked in workQDoWork.
01o,01sep94,rdc  fixed spr 3442 (scratch pad mismanagement in portable version)
01n,10dec93,smb  added instrumentation for windview
01l,04jul92,jcf  private header files.
01k,26may92,rrr  the tree shuffle
01j,15oct91,ajm  added MIPS to list of optimized CPU's.
01i,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed includes to have absolute path from h/
		  -fixed #else and #endif
		  -changed TINY and UTINY to INT8 and UINT8
		  -changed VOID to void
		  -changed copyright notice
01h,26sep91,hdn  added TRON to list of non-portable CPU's.
01g,24jul91,del  added I960 to list of non-portable CPU's.
01f,29dec90,gae  added forward declaration of workQPanic.
01e,28sep90,jcf  documentation.
01d,01aug90,jcf  added queue overflow panic.
01c,15jul90,jcf  made workQDoWork () callable from itself, for watchdogs
01b,11may90,jcf  fixed up PORTABLE definition.
01a,14jun89,jcf	 written.
*/

/*
DESCRIPTION
The VxWorks kernel provides tasking control services to an application.  The
libraries kernelLib, taskLib, semLib, tickLib, and wdLib comprise the kernel
functionality.  This library is internal to the VxWorks kernel and contains
no user callable routines.

INTERNAL
These routines manage the wind work queue.  The work queue is used to queue
kernel work that must be deferred because the kernel is already engaged by
another request.  Kernel work deferred to the work queue must originate
from an interrupt service routine.

The work queue is an example of a single reader/multiple writer queue.
The reader is always the first task or interrupt that enters the kernel, and
that reader is responsible for emptying the work queue before leaving.  The
work queue writers are from interrupt service routines so the interrupts must
be locked during work queue write manipulations, but need not be locked during
read operations.

The work queue is implemented is implemented as a 1K byte ring buffer.  Each
ring entry, or JOB, is 16 bytes in size.  So the ring has 64 entries.  The
selection of this size is based on the efficiency of indexing the work queue
with a single byte index.  To advance the index, all we have to do is add four
to the index, and if our arithmatic is 8 bit, the overflow will be discarded.
The efficiency lies in the fact that we have no conditionals to test for ring
wrap around.

This library should be optimized whenever possible, because the work queue
should scream.

CAVEATS
Two constraints make the future of this library somewhat cloudy.  The
limits of 64 jobs and 16 byte jobs are pretty much hard coded in.  So its
likely that this implementation will give way someday to meet future needs.
It remains the most efficient mechanism for the work queue today.

SEE ALSO: workALib, windLib, and windALib.

NOMANUAL
*/

#include "vxWorks.h"
#include "intLib.h"
#include "sysLib.h"
#include "errno.h"
#include "rebootLib.h"
#include "private/workQLibP.h"
#include "private/funcBindP.h"

/*
 * optimized version available for 680X0, I960, MIPS, I80X86, SH,
 * ARM (excluding Thumb)
 */

#if (defined(PORTABLE) || \
     ((CPU_FAMILY != MC680X0) && \
      (CPU_FAMILY != I960) && \
      (CPU_FAMILY != MIPS) && \
      (CPU_FAMILY != I80X86) && \
      (CPU_FAMILY != SH) && \
      (CPU_FAMILY != COLDFIRE) && \
      (CPU_FAMILY != ARM)) || \
     ((CPU_FAMILY == ARM) && ARM_THUMB))
#define workLib_PORTABLE
#endif

/* globals */

volatile UINT8 workQReadIx;		/* circular work queue read index */
volatile UINT8 workQWriteIx;		/* circular work queue read index */
volatile BOOL  workQIsEmpty;		/* TRUE if work queue is empty */
int   pJobPool [WIND_JOBS_MAX * 4];	/* pool of memory for jobs */


/*******************************************************************************
*
* workQInit - initialize the wind work queue
*
* This routine sets the read and write index to their initial values of zero.
*/

void workQInit (void)
    {
    workQReadIx  = workQWriteIx = 0;	/* initialize the indexes */
    workQIsEmpty = TRUE;		/* the work queue is empty */
    }

#ifdef workLib_PORTABLE

/*******************************************************************************
*
* workQAdd0 - add work with no parameters to the wind work queue
*
* When the kernel is interrupted, new kernel work must be queued to an internal
* work queue.  The work queue is emptied by whatever task or interrupt service
* routine that entered the kernel first.  The work is emptied as the last
* code of reschedule().
*
* INTERNAL
* The work queue is single reader, multiple writer, so we should have to lock
* out interrupts while the writer is copying into the ring, but because the
* reader can never interrupt a writer, interrupts need only be locked while
* we advance the write queue pointer.
*
* RETURNS: OK
*
* SEE ALSO: reschedule().
*
* NOMANUAL
*/

void workQAdd0
    (
    FUNCPTR func        /* function to invoke */
    )
    {
    int level = intLock ();		/* LOCK INTERRUPTS */
    FAST JOB *pJob = (JOB *) &pJobPool [workQWriteIx];

    workQWriteIx += 4;			/* advance write index */

    if (workQWriteIx == workQReadIx)
	workQPanic ();			/* leave interrupts locked */

    intUnlock (level);			/* UNLOCK INTERRUPTS */

    workQIsEmpty = FALSE;		/* we put something in it */

    pJob->function = func;		/* fill in function */
    }

/*******************************************************************************
*
* workQAdd1 - add work with one parameter to the wind work queue
*
* When the kernel is interrupted, new kernel work must be queued to an internal
* work queue.  The work queue is emptied by whatever task or interrupt service
* routine that entered the kernel first.  The work is emptied as the last
* code of reschedule().
*
* INTERNAL
* The work queue is single reader, multiple writer, so we should have to lock
* out interrupts while the writer is copying into the ring, but because the
* reader can never interrupt a writer, interrupts need only be locked while
* we advance the write queue pointer.
*
* RETURNS: OK
*
* SEE ALSO: reschedule()
*
* NOMANUAL
*/

void workQAdd1
    (
    FUNCPTR func,       /* function to invoke */
    int arg1            /* parameter one to function */
    )
    {
    int level = intLock ();		/* LOCK INTERRUPTS */
    FAST JOB *pJob = (JOB *) &pJobPool [workQWriteIx];

    workQWriteIx += 4;			/* advance write index */

    if (workQWriteIx == workQReadIx)
	workQPanic ();			/* leave interrupts locked */

    intUnlock (level);			/* UNLOCK INTERRUPTS */

    workQIsEmpty = FALSE;		/* we put something in it */

    pJob->function = func;		/* fill in function */
    pJob->arg1 = arg1;			/* fill in argument */
    }

/*******************************************************************************
*
* workQAdd2 - add work with two parameters to the wind work queue
*
* When the kernel is interrupted, new kernel work must be queued to an internal
* work queue.  The work queue is emptied by whatever task or interrupt service
* routine that entered the kernel first.  The work is emptied as the last
* code of reschedule().
*
* INTERNAL
* The work queue is single reader, multiple writer, so we should have to lock
* out interrupts while the writer is copying into the ring, but because the
* reader can never interrupt a writer, interrupts need only be locked while
* we advance the write queue pointer.
*
* RETURNS: OK
*
* SEE ALSO: reschedule().
*
* NOMANUAL
*/

void workQAdd2
    (
    FUNCPTR func,       /* function to invoke */
    int arg1,           /* parameter one to function */
    int arg2            /* parameter two to function */
    )
    {
    int level = intLock ();		/* LOCK INTERRUPTS */
    FAST JOB *pJob = (JOB *) &pJobPool [workQWriteIx];

    workQWriteIx += 4;			/* advance write index */

    if (workQWriteIx == workQReadIx)
	workQPanic ();			/* leave interrupts locked */

    intUnlock (level);			/* UNLOCK INTERRUPTS */

    workQIsEmpty = FALSE;		/* we put something in it */

    pJob->function = func;		/* fill in function */
    pJob->arg1 = arg1;			/* fill in arguments */
    pJob->arg2 = arg2;
    }

/*******************************************************************************
*
* workQDoWork - execute all the work in the work queue
*
* When the kernel is interrupted, new kernel work must be queued to an internal
* work queue.  The work queue is emptied by whatever task or interrupt service
* routine that entered the kernel first.  The work is emptied by calling this
* routine as the last code of reschedule().
*
* INTERNAL
* The work queue is single reader, multiple writer, so we should have to lock
* out interrupts while the writer is copying into the ring, but because the
* reader can never interrupt a writer, interrupts need only be locked while
* we advance the write queue pointer.
*
* SEE ALSO: reschedule().
*
* NOMANUAL
*/

void workQDoWork (void)
    {
    FAST JOB *pJob;
    int oldErrno = errno;			/* save errno */

    while (workQReadIx != workQWriteIx)
	{
        pJob = (JOB *) &pJobPool [workQReadIx];	/* get job */

	/* increment read index before calling function, because work function
	 * could be windTickAnnounce () that calls this routine as well.
	 */

	workQReadIx += 4;

        (FUNCPTR *)(pJob->function) (pJob->arg1, pJob->arg2);

	workQIsEmpty = TRUE;			/* leave loop with empty TRUE */
	}

    errno = oldErrno;				/* restore _errno */
    }

#endif	/* workLib_PORTABLE */

/*******************************************************************************
*
* workQPanic - work queue has overflowed so reboot system
*
* The application has really botched things up if we get here, so we reboot
* the system.  An overflow can be detected if the read index is one greater than
* the write index.
*
* NOMANUAL
*/

void workQPanic (void)
    {
    static char *pWorkQMsg = "\nworkQPanic: Kernel work queue overflow.\n";

    while ((*(sysExcMsg++) = *(pWorkQMsg++)) != EOS)
	;
    
    sysExcMsg --;

    reboot (BOOT_WARM_AUTOBOOT);
    }
