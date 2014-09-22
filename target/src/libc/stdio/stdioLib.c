/* stdioLib.c - standard I/O library */

/* Copyright 1984-1995 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
02j,10feb97,tam  reclaimed resources from the standard file pointers (SPR #7915)
02i,11feb95,jdi  doc tweak.
02h,05mar93,jdi  documentation cleanup for 5.1.
02g,13nov92,dnw  added __std{in,out,err} (SPR #1770)
		 made stdInitStd() be LOCAL.
		 changed stdioFp() create FILE if it doesn't already exist.
02f,20sep92,smb  documentation additions
02e,29jul92,smb  added stdioFp().
		 Modified the documentation for the new stdio library.
02d,29jul92,jcf  taken from stdioLib.c 
02c,26may92,rrr  the tree shuffle
02b,02apr92,jmm  added free() of memory if bad options passed to fdopen()
		 SPR # 1396
02a,27mar92,jmm  changed fopen() to free memory if the open() fails, SPR #1115
01z,25nov91,rrr  cleanup of some ansi warnings.
01y,12nov91,rrr  removed VARARG_OK, no longer needed with ansi c.
01x,07oct91,rrr  junk for r3000 braindamage.
01w,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed includes to have absolute path from h/
		  -fixed #else and #endif
		  -changed VOID to void
		  -changed copyright notice
01v,18may91,gae  fixed varargs for 960 with conditional VARARG_OK,
		 namely: fscanf, fprintf, and scanf.
01u,04apr91,jdi  documentation cleanup; doc review by dnw.
01t,10aug90,dnw  added forward declaration of stdioExitStd ().
01s,08aug90,dnw  changed incorrect forward declaration for stdioCreateHook().
01r,10jun90,dnw  changed to call fioFormatV and fioScanV directly with
		   appropriate handling of varargs (removed doscan and doprnt)
		 moved all routine implementations of stdio macros to end of
		   module so that macros would be used in rest of code
		 spr 640: fprintf returns number of chars printed instead of OK
		 spr 754: vararg routines no longer limited to 16 args
		 fixed coercions to allow void to be defined as void one day.
01q,26jun90,jcf  lint.
01p,12mar90,jcf  changed std{in,out,err} to macros to fps in tcbx.
01o,16feb90,dab  fixed bug in doscan() that wouldn't match characters between
		 the input stream and the format specification.
01n,03aug89,dab  removed call to creat() in fopen().
01m,08apr89,dnw  changed stdioInit() to call taskVarInit().
01l,23mar89,dab  changed numerical constants to appropriate defines.
		 fixed fseek() to return only OK or ERROR.
01k,15nov88,dnw  documentation touchup.
01j,23sep88,gae  documentation touchup.
01i,13sep88,gae  removed ifdef'd sprintf which got into documentation.
01h,06sep88,gae  adjusted some argument declarations to please f2cgen.
01g,20aug88,gae  documentation.
01f,07jul88,jcf  changed malloc to match new declaration.
01e,29jun88,gae  documentation.  Added error messages in stioInit().
01d,22jun88,dnw  name tweaks.
01c,30may88,dnw  changed to v4 names.
01b,28may88,dnw  removed routines that had been excluded with "#if FALSE".
		 made stdioFlushBuf LOCAL.
		 cleaned up stdio{Init,Exit}Task; improved error msgs.
01a,28mar88,gae  created.
*/

/*
DESCRIPTION
This library provides a complete UNIX compatible standard I/O buffering
scheme.  It is beyond the scope of this manual entry to describe all aspects
of the buffering -- see the
.I "VxWorks Programmer's Guide:  I/O System"
and the Kernighan & Ritchie C manual.  This manual entry primarily highlights
the differences between the UNIX and VxWorks standard I/O.

FILE POINTERS
The routine fopen() creates a file pointer.  Use of the file pointer follows
conventional UNIX usage.  In a shared address space, however, and perhaps more
critically, with the VxWorks system symbol table, tasks may not use each
others' file pointers, at least not without some interlocking mechanism.  If it
is necessary to use the same name for a file pointer but have incarnations for
each task, then use task variables; see the manual entry for taskVarLib.

FIOLIB
Several routines normally considered part of standard I/O -- printf(),
sscanf(), and sprintf() -- are not implemented in stdio; they are
instead implemented in fioLib.  They do not use the standard I/O buffering
scheme.  They are self-contained, formatted, but unbuffered I/O
functions.  This allows a limited amount of formatted I/O to be achieved
without the overhead of the stdio library.

TASK TERMINATION
When a task exits, unlike in UNIX, it is the responsibility of the task to
fclose() its file pointers, except `stdin', `stdout', and `stderr'.  If a
task is to be terminated asynchronously, use kill() and arrange for a
signal handler to clean up.

INCLUDE FILES
stdio.h, taskLib.h

All the macros defined in stdio.h are also implemented as real functions so
that they are available from the VxWorks shell.

SEE ALSO
fioLib, ioLib, taskVarLib, sigLib, Kernighan & Ritchie C manual,
.pG "I/O System"

*/

#include "vxWorks.h"
#include "stdio.h"
#include "sys/types.h"
#include "ctype.h"
#include "ioLib.h"
#include "stdlib.h"
#include "taskLib.h"
#include "taskHookLib.h"
#include "stdarg.h"
#include "logLib.h"
#include "fcntl.h"
#include "unistd.h"
#include "errnoLib.h"
#include "string.h"
#include "fioLib.h"
#include "classLib.h"
#include "private/objLibP.h"
#include "private/stdioP.h"
#include "private/funcBindP.h"

/* locals */

LOCAL OBJ_CLASS fpClass;			/* file object class */
LOCAL BOOL	stdioInitialized = FALSE;
LOCAL BOOL      stdioFpCleanupHookDone = FALSE;

/* global variables */

CLASS_ID fpClassId = &fpClass;			/* file class id */


/*******************************************************************************
*
* stdioInit - initialize standard I/O support
*
* This routine installs standard I/O support.  It must be called before
* using `stdio' buffering.  If INCLUDE_STDIO is defined in configAll.h, it
* is called automatically by the root task usrRoot() in usrConfig.c.
*
* RETURNS:
* OK, or ERROR if the standard I/O facilities cannot be installed.
*/

STATUS stdioInit (void)
    {
    if ((!stdioInitialized) &&
	(classInit (fpClassId, sizeof (FILE), OFFSET (FILE, objCore),
		    (FUNCPTR) NULL, (FUNCPTR) NULL, (FUNCPTR) NULL) == OK))
	{
	_func_fclose	 = fclose;	/* attach fclose vfunc to taskLib */
	stdioInitialized = TRUE;	/* we've finished the initialization */
	}

    return (OK);
    }

/*******************************************************************************
*
* stdioFpCreate - allocate a new FILE structure
*
* RETURNS:
* The pointer to newly created file, or NULL if out of memory.
*
* NOMANUAL
*/

FILE *stdioFpCreate (void)
    {
    FAST FILE *fp = NULL;

    if ((stdioInit () == OK) &&
        ((fp = (FILE *)objAlloc (fpClassId)) != NULL))
	{
	fp->_p		= NULL;			/* no current pointer */
	fp->_r		= 0;
	fp->_w		= 0;			/* nothing to read or write */
	fp->_flags	= 1;			/* caller sets real flags */
	fp->_file	= -1;			/* no file */
	fp->_bf._base	= NULL;			/* no buffer */
	fp->_bf._size	= 0;
	fp->_lbfsize	= 0;			/* not line buffered */
	fp->_ub._base	= NULL;			/* no ungetc buffer */
	fp->_ub._size	= 0;
	fp->_lb._base	= NULL;			/* no line buffer */
	fp->_lb._size	= 0;
	fp->_blksize	= 0;
	fp->_offset	= 0;
	fp->taskId	= (int) taskIdCurrent;	/* task id might be useful */

	objCoreInit (&fp->objCore, fpClassId);	/* validate file object */
	}

    return (fp);
    }

/*******************************************************************************
*
* stdioFpDestroy - destroy and reclaim resources of specified file pointer
*
* RETURNS:
* OK, or ERROR if file pointer could not be destroyed.
*
* NOMANUAL
*/

STATUS stdioFpDestroy
    (
    FILE *fp
    )
    {
    /* fclose() deallocates any buffers associated with the file pointer */

    objCoreTerminate (&fp->objCore);		/* invalidate file pointer */

    return (objFree (fpClassId, (char *) fp));	/* deallocate file pointer */
    }

/*******************************************************************************
*
* stdioStdfpCleanup - reclaim resources from the standard file pointers. 
*
* RETURNS: N/A
*
* NOMANUAL
*/

LOCAL void stdioStdfpCleanup 
    (
    WIND_TCB *pTcb      /* address of task's TCB */
    )
    {
    int ix;

    /* close standard file pointers (stdin, stdout, stderr) if present */

    for (ix = 0; ix < 3; ++ix)
        if (pTcb->taskStdFp[ix] != NULL)
            fclose (pTcb->taskStdFp[ix]);
    }

/*******************************************************************************
*
* stdioInitStd - initialize use of a standard file
*/

LOCAL STATUS stdioInitStd
    (
    int stdFd		/* standard file descriptor to initialize (0,1,2) */
    )
    {
    FILE *fp;

    if ((fp = stdioFpCreate ()) == NULL)
	return (ERROR);

    switch (stdFd)
	{
	case STD_IN:  fp->_flags = __SRD;    break;	/* read only */
	case STD_OUT: fp->_flags = __SWR;    break;	/* write only */
	case STD_ERR: fp->_flags = __SWRNBF; break;	/* write only unbuf'd */
	}

    fp->_file = stdFd;				/* standard fd */

    taskIdCurrent->taskStdFp[stdFd] = fp;	/* init private file pointer */

    /* 
     * need to deallocated stdout, stdin and stderr FILE structures and 
     * ressources when a task exits or is deleted. Ressources are reclaimed
     * via the taskDeleteHook facility.
     */

    if (!stdioFpCleanupHookDone &&
        ((fp == stdout) || (fp == stdin) || (fp == stderr)))
        {
        /* initialize  task hook facility if necessary */

        if (_func_taskDeleteHookAdd == NULL)
            taskHookInit ();
        
        taskDeleteHookAdd ((FUNCPTR) stdioStdfpCleanup);
        stdioFpCleanupHookDone = TRUE;
        }

    return (OK);
    }

/******************************************************************************
*
* stdioFp - return the standard input/output/error FILE of the current task
* 
* This routine returns the specified standard FILE structure address of the
* current task.  It is provided primarily to give access to standard input,
* standard output, and standard error from the shell, where the usual
* `stdin', `stdout', `stderr' macros cannot be used.
*
* INCLUDE FILES: stdio.h 
*
* RETURNS: The standard FILE structure address of the specified file
* descriptor, for the current task.
*/

FILE * stdioFp 
    (
    int stdFd		/* fd of standard FILE to return (0,1,2) */
    )
    {
    if (taskIdCurrent->taskStdFp [stdFd] == NULL)
	stdioInitStd (stdFd);

    return (taskIdCurrent->taskStdFp [stdFd]);
    }

/*******************************************************************************
*
* __stdin - get pointer to current task's stdin
*
* This function returns a pointer to the current task's stdin.  If the
* current task does not have a stdin then one is created.
*
* NOMANUAL
*/

FILE ** __stdin (void)
    {
    if (taskIdCurrent->taskStdFp [STD_IN] == NULL)
	stdioInitStd (STD_IN);

    return (&taskIdCurrent->taskStdFp [STD_IN]);
    }

/*******************************************************************************
*
* __stdout - get pointer to current task's stdout
*
* This function returns a pointer to the current task's stdout.  If the
* current task does not have a stdout then one is created.
*
* NOMANUAL
*/

FILE ** __stdout (void)
    {
    if (taskIdCurrent->taskStdFp [STD_OUT] == NULL)
	stdioInitStd (STD_OUT);

    return (&taskIdCurrent->taskStdFp [STD_OUT]);
    }

/*******************************************************************************
*
* __stderr - get pointer to current task's stderr
*
* This function returns a pointer to the current task's stderr.  If the
* current task does not have a stderr then one is created.
*
* NOMANUAL
*/

FILE ** __stderr (void)
    {
    if (taskIdCurrent->taskStdFp [STD_ERR] == NULL)
	stdioInitStd (STD_ERR);

    return (&taskIdCurrent->taskStdFp [STD_ERR]);
    }

