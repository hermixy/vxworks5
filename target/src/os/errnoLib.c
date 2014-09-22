/* errnoLib.c - error status library */

/* Copyright 1984-1999 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
04n,12mar99,p_m  Fixed SPR# 10002 by documenting makeStatTbl usage on Windows.
04m,14oct95,jdi  doc: revised pathnames for Tornado.
04l,05feb93,jdi  documentation; updated location of statTbl.
04k,21jan93,jdi  documentation cleanup for 5.1.
04j,14oct92,jdi  made __errno() NOMANUAL.
04i,09jul92,smb  added __errno() for ANSI merge.
		 removed errno.h include.
04h,04jul92,jcf  scalable/ANSI/cleanup effort.
04g,26may92,rrr  the tree shuffle
04f,25nov91,llk  ansi stuff.
		 added strerror().
04e,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed includes to have absolute path from h/
		  -changed copyright notice
04d,05apr91,jdi	 documentation -- removed header parens and x-ref numbers;
		 doc review by dnw.
04c,11feb91,jaa	 documentation cleanup.
04b,28oct90,jcf  documentation.
04a,14mar90,jcf  errno is now a global variable updated on context switch/intr.
		 removed tcb extension dependencies.
03f,07mar90,jdi  documentation cleanup.
03e,19aug88,gae  documentation.
03d,05jun88,dnw  changed from stsLib to errnoLib.
		 documentation.
03c,30may88,dnw  changed to v4 names.
03b,22apr88,gae  made [sg]etStatus() work at interrupt level.
03a,27jan88,jcf  made kernel independent.
02n,04nov87,ecs  documentation.
02m,22apr87,llk  moved printStatus() to usrLib.c.
02l,24mar87,jlf  documentation.
02k,20dec86,dnw  changed printStatus() to print numeric value if no
		   symbolic value.
		 changed to not get include files from default directories.
02j,04sep86,jlf  documentation.
02i,31jul86,llk  documentation.
02h,01jul86,jlf  documentation.
02g,08apr86,dnw  changed printStatus to not use symLib.
		 deleted makeStatSymTbl() since status symbol table is now
		   pre-built.
02f,11feb86,dnw  changed references to old TASK_STACK_HEADER to new
		   tcb extension.
02e,09nov85,jlf  fixed bug in makeStatSymTbl which was caused by an extra
		 level of indirection.
02d,11oct85,dnw  changed to new calling sequence of symCreate.
		 de-linted.
02c,27aug85,rdc  added makeStatSymTbl and printStatus.
02b,21jul85,jlf  documentation.
02a,08apr85,rdc  modifications for vrtx vers. 3: setTaskStatus
	      	 uses TASK_STACK_HEADER.
01d,19sep84,jlf  worked on comments a little.
01c,08sep84,jlf  added copyright and comments. Fixed setTaskStatus to not
		 try to set a status is tid is not legitimate.
01b,24aug84,dnw  added setTaskStatus.
		 added test for intContext in setStatus so that calls
		   at interrupt level won't clobber arbitrary task.
01a,30jul84,ecs  written
*/

/*
DESCRIPTION
This library contains routines for setting and examining the error status
values of tasks and interrupts.  Most VxWorks functions return ERROR when
they detect an error, or NULL in the case of functions returning pointers.
In addition, they set an error status that elaborates the nature of the
error.

This facility is compatible with the UNIX error status mechanism in which
error status values are set in the global variable `errno'.  However, in
VxWorks there are many task and interrupt contexts that share common memory
space and therefore conflict in their use of this global variable.
VxWorks resolves this in two ways:
.IP (1) 4
For tasks, VxWorks maintains the `errno' value for each context
separately, and saves and restores the value of `errno' with every context
switch.  The value of `errno' for a non-executing task is stored in the
task's TCB.  Thus, regardless of task context, code can always reference
or modify `errno' directly.
.IP (2)
For interrupt service routines, VxWorks saves and restores `errno' on
the interrupt stack as part of the interrupt enter and exit code provided
automatically with the intConnect() facility.   Thus, interrupt service
routines can also reference or modify `errno' directly.
.LP

The `errno' facility is used throughout VxWorks for error reporting.  In
situations where a lower-level routine has generated an error, by convention,
higher-level routines propagate the same error status, leaving `errno'
with the value set at the deepest level.  Developers are encouraged to use
the same mechanism for application modules where appropriate.

ERROR STATUS VALUES
An error status is a 4-byte integer.  By convention, the most significant
two bytes are the module number, which indicates the module in which the
error occurred.  The lower two bytes indicate the specific error within
that module.  Module number 0 is reserved for UNIX error numbers so that
values from the UNIX errno.h header file can be set and tested without
modification.  Module numbers 1-500 decimal are reserved for VxWorks
modules.  These are defined in vwModNum.h.  All other module numbers are
available to applications.

PRINTING ERROR STATUS VALUES
VxWorks can include a special symbol table called `statSymTbl'
which printErrno() uses to print human-readable error messages.

This table is created with the tool makeStatTbl, found
in \f3host/<hostOs>/bin\f1.
This tool reads all the .h files in a specified directory and generates a
C-language file, which generates a symbol table when compiled.  Each symbol
consists of an error status value and its definition, which was obtained
from the header file.

For example, suppose the header file target/h/myFile.h contains the line:
.CS
    #define S_myFile_ERROR_TOO_MANY_COOKS	0x230003
.CE
The table `statSymTbl' is created by first running:

On Unix:
.CS
    makeStatTbl target/h > statTbl.c
.CE
On Windows:
.CS
    makeStatTbl target/h
.CE

This creates a file \f3statTbl.c\fP in the current directory, which, when
compiled, generates `statSymTbl'.  The table is then linked in with
VxWorks.  Normally, these steps are performed automatically by the makefile
in \f3target/src/usr\f1.

If the user now types from the VxWorks shell:
.CS
    -> printErrno 0x230003
.CE
The printErrno() routine would respond:
.CS
    S_myFile_ERROR_TOO_MANY_COOKS
.CE
The makeStatTbl tool looks for error status lines of the form:
.CS
    #define S_xxx  <n>
.CE
where <xxx> is any string, and <n> is any number.
All VxWorks status lines are of the form:
.CS
    #define S_thisFile_MEANINGFUL_ERROR_MESSAGE   0xnnnn
.CE
where <thisFile> is the name of the module.

This facility is available to the user by adding header files with
status lines of the appropriate forms and remaking VxWorks.

INCLUDE FILES
The file vwModNum.h contains the module numbers for every VxWorks module.
The include file for each module contains the error numbers which that module
can generate.

SEE ALSO: printErrno(), makeStatTbl,
.pG "Basic OS"
*/

#include "vxWorks.h"
#include "taskLib.h"
#include "symLib.h"
#include "errnoLib.h"
#include "sysSymTbl.h"

#undef errno	/* undefine errno so we can define the actual errno variable */

/* global variables */

int errno;	/* updated on each context switch and interrupt by kernel */


/*******************************************************************************
* __errno - returns the address of errno.
*
* INCLUDE: errno.h
*
* RETURNS: pointer to errno
*
* NOMANUAL
*/

int *__errno (void)
    {
    return (&errno); 
    }

/*******************************************************************************
*
* errnoGet - get the error status value of the calling task
*
* This routine gets the error status stored in `errno'.
* It is provided for compatibility with previous versions of VxWorks and
* simply accesses `errno' directly.
*
* RETURNS:
* The error status value contained in `errno'.
*
* SEE ALSO: errnoSet(), errnoOfTaskGet()
*/

int errnoGet (void)
    
    {
    return (errno);
    }

/*******************************************************************************
*
* errnoOfTaskGet - get the error status value of a specified task
*
* This routine gets the error status most recently set for a specified task.
* If <taskId> is zero, the calling task is assumed, and the value currently
* in `errno' is returned.
*
* This routine is provided primarily for debugging purposes.  Normally,
* tasks access `errno' directly to set and get their own error status values.
*
* RETURNS: The error status of the specified task, or ERROR if the task does
* not exist.
*
* SEE ALSO: errnoSet(), errnoGet()
*/

int errnoOfTaskGet
    (
    int taskId                  /* task ID, 0 means current task */
    )
    {
    WIND_TCB *pTcb;

    /* if getting errno of self just return errno */

    if ((taskId == 0) || (taskId == taskIdSelf ()))
	return (errno);

    /* get pointer to current task's tcb, be sure it exists */

    if ((pTcb = taskTcb (taskId)) == NULL)
	return (ERROR);

    /* return the error status from the tcb extension */

    return (pTcb->errorStatus);
    }

/*******************************************************************************
*
* errnoSet - set the error status value of the calling task
*
* This routine sets the `errno' variable with a specified error status.
* It is provided for compatibility with previous versions of VxWorks and
* simply accesses `errno' directly.
*
* RETURNS:
* OK, or ERROR if the interrupt nest level is too deep.
*
* SEE ALSO: errnoGet(), errnoOfTaskSet()
*/

STATUS errnoSet
    (
    int errorValue              /* error status value to set */
    )
    {
    errno = errorValue;

    return (OK);
    }

/*******************************************************************************
*
* errnoOfTaskSet - set the error status value of a specified task
*
* This routine sets the error status for a specified task.
* If <taskId> is zero, the calling task is assumed, and `errno' is set with
* the specified error status.
*
* This routine is provided primarily for debugging purposes.  Normally,
* tasks access `errno' directly to set and get their own error status values.
*
* RETURNS: OK, or ERROR if the task does not exist.
*
* SEE ALSO: errnoSet(), errnoOfTaskGet()
*/

STATUS errnoOfTaskSet
    (
    int taskId,                 /* task ID, 0 means current task */
    int errorValue              /* error status value */
    )
    {
    WIND_TCB *pTcb;		/* this gets pointer to current task's tcb */

    /* if setting errno of self just set errno */

    if ((taskId == 0) || (taskId == taskIdSelf ()))
	{
	errno = errorValue;
	return (OK);
	}

    /* get pointer to specified task's tcb */

    if ((pTcb = taskTcb (taskId)) == NULL)
	return (ERROR);

    /* stuff errorValue in tcb extension */

    pTcb->errorStatus = errorValue;
    return (OK);
    }
