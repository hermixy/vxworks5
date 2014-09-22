/* logLib.c - message logging library */

/* Copyright 1984-1995 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
03l,13may02,cyr  fix SPR 32358 documentation clarification on logMsg
03k,29oct01,cyr  doc:doc: correct SPR 7113 logMsg
03j,30oct95,ism  added simsolaris support
03i,09nov94,ms   undid 03h. Bumped stack size for SIMSPARC.
03h,17jul94,ms   jacked up the stack size for VxSim/HPPA.
03g,24aug93,dvs  added reset of logFdFromRlogin.
03f,23aug93,dvs  fixed logFdSet() for rlogin sessions (SPR #2212).
03e,15feb93,jdi  fixed doc for logMsg() to mention fixed number of args.
03d,21jan93,jdi  documentation cleanup for 5.1.
03c,13nov92,jcf  added _func_logMsg initialization.  added include semLibP.h.
03b,17jul92,gae  reverted to 02x -- eliminating stdargs change.
		 Documentation sprs #600 and #1148 fixed.  Checked
		 for null format specifier.
03a,18jul92,smb  Changed errno.h to errnoLib.h.
02z,26may92,rrr  the tree shuffle
02y,04mar92,gae  Used stdargs for logMsg, no longer fixed at 6 parameters.
		 Allowed messages to be dropped via global var. logMsgTimeout.
		 Reduced stack size from 5000 to 3000.
		 Checked for null format specifier.
		 Fixed spr #600 (documentation of logMsg).
		 Fixed spr #1138 (printing of floating point numbers).
		 Fixed spr #1148 (document use of message queues)
02x,10dec91,gae  added includes for ANSI.
02w,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed VOID to void
		  -changed copyright notice
02v,01aug91,yao  changed to check for dead task in logTask(). added missing
		 arg to lprintf() call.
02u,10jun91,del  changed MAX_ARGS to MAX_LOGARGS to silence redef warning.
02t,05apr91,jdi	 documentation -- removed header parens and x-ref numbers;
		 doc review by dnw.
02s,11feb91,jaa	 documentation cleanup.
02r,08oct90,dnw  lint
02q,05oct90,dnw  changed to new msgQ interface.
02p,10aug90,dnw  changed declaration for logMsg from void to int.
		 added include of logLib.h.
02o,10aug90,kdl  added forward declaration for lprintf.
02n,19jul90,dnw  changed to use message queue instead of pipe
		 changed logTask stack from 10k to 5k
		 spr 641: changed logMsg to return number of bytes written to
			msg q or EOF if error writing
		 improved module and logMsg doc, esp about volitile args
02m,10jul90,dnw  logTask no longer dies with bus error when printing msg from
		   interrupt level (fixed to not call taskName(-1))
		 changed to use INT_CONTEXT() instead of intContext()
02l,26jun90,jcf  changed logTask semaphore to mutex.
02k,15apr90,jcf  changed logTask name to tLogTask.
02j,14mar90,jdi  documentation cleanup.
02i,16sep88,ecs  bumped logTaskStackSize from 2000 to 10000.
		 removed architectural assumption from logMsg.
02h,14jun89,gae  changed the format of the logMsg's to be more intelligible.
02g,07jul88,jcf  made logShow global but NOMANUAL, so lint would let up.
02f,22jun88,dnw  name tweaks.
02e,06jun88,dnw  changed taskSpawn/taskCreate args.
02d,30may88,dnw  changed to v4 names.
02c,23apr88,jcf  changed semaphores for new semLib.
02b,05apr88,gae  changed fprintf() to fdprintf().  Added debug rtn logShow().
02a,26jan88,jcf  made kernel independent.
01q,14dec87,gae  changed logMsg() to not have multiple argument decl's for
		   parsing by the C interface generator.
01p,16nov87,ecs  documentation.
01o,03nov87,dnw  changed to let system pick logTask id;
		   now available in logTaskId.
01n,14oct87,gae  added log{Add,Del}Fd(), allowing multiple logging fds.
		 removed NOT_GENERIC stuff.
01m,24mar87,jlf  documentation
01l,21dec86,dnw  changed to not get include files from default directories.
01k,31jul86,llk  uses new spawn
01j,09jul86,dnw  restored NOT_GENERIC stuff that got lost in v01i.
01i,01jul86,jlf  minor documentation
01h,09apr86,dnw  added call to vxSetTaskBreakable to make logTask unbreakable.
		 fixed documentation of logMsg().
01g,07apr86,dnw  increased logTsk stack from 1000 to 2000.
		 corrected errors in documentation.
		 added logSetFd().
01f,11mar86,jlf  changed GENERIC to NOT_GENERIC
01e,20jul85,jlf  documentation.
01d,04jan85,ecs  added an ARGSUSED to logMsg, got rid of 'housetime'.
01c,19sep84,jlf  cleaned up comments a little
01b,07sep84,jlf  added copyright notice and comments.
01a,28aug84,dnw  culled from fioLib and enhanced.
*/

/*
DESCRIPTION
This library handles message logging.  It is usually used to display error
messages on the system console, but such messages can also be sent to a
disk file or printer.

The routines logMsg() and logTask() are the basic components of the
logging system.  The logMsg() routine has the same calling sequence as
printf(), but instead of formatting and outputting the message directly,
it sends the format string and arguments to a message queue.  The task
logTask() waits for messages on this message queue.  It formats each
message according to the format string and arguments in the message,
prepends the ID of the sender, and writes it on one or more file
descriptors that have been specified as logging output streams (by
logInit() or subsequently set by logFdSet() or logFdAdd()).

USE IN INTERRUPT SERVICE ROUTINES
Because logMsg() does not directly cause output to I/O devices, but
instead simply writes to a message queue, it can be called from an
interrupt service routine as well as from tasks.  Normal I/O, such as
printf() output to a serial port, cannot be done from an interrupt service
routine.

DEFERRED LOGGING
Print formatting is performed within the context of logTask(), rather than
the context of the task calling logMsg().  Since formatting can require
considerable stack space, this can reduce stack sizes for tasks that only
need to do I/O for error output.

However, this also means that the arguments to logMsg() are not interpreted
at the time of the call to logMsg(), but rather are interpreted at some
later time by logTask().  This means that the arguments to logMsg() should
not be pointers to volatile entities.  For example, pointers to dynamic or
changing strings and buffers should not be passed as arguments to be formatted.
Thus the following would not give the desired results:
.ne 8
.CS
    doLog (which)
	{
	char string [100];

	strcpy (string, which ? "hello" : "goodbye");
	...
	logMsg (string);
	}
.CE

By the time logTask() formats the message, the stack frame of the caller may
no longer exist and the pointer <string> may no longer be valid.
On the other hand, the following is correct since the string pointer passed
to the logTask() always points to a static string:
.CS
    doLog (which)
	{
	char *string;

	string = which ? "hello" : "goodbye";
	...
	logMsg (string);
	}
.CE

INITIALIZATION
To initialize the message logging facilities, the routine logInit() must
be called before calling any other routine in this module.  This is done
by the root task, usrRoot(), in usrConfig.c.

INCLUDE FILES: logLib.h

SEE ALSO: msgQLib,
.pG "I/O System"
*/

/* LINTLIBRARY */

#include "vxWorks.h"
#include "errnoLib.h"
#include "logLib.h"
#include "ioLib.h"
#include "taskLib.h"
#include "semLib.h"
#include "intLib.h"
#include "msgQLib.h"
#include "stdio.h"
#include "private/semLibP.h"
#include "private/funcBindP.h"

#define MAX_LOGARGS	6	/* max args to log message */
#define MAX_LOGFDS	5	/* max log fds */

typedef struct				/* LOG_MSG */
    {
    int		id;			/* ID of sending task */
    char *	fmt;			/* pointer to format string */
    int		arg [MAX_LOGARGS];	/* args for format string */
    } LOG_MSG;


/* logTask parameters */

int logTaskId		= 0;
int logTaskPriority	= 0;
int logTaskOptions	= VX_UNBREAKABLE;
#if    CPU_FAMILY == SIMSPARCSUNOS || CPU_FAMILY == SIMSPARCSOLARIS
int logTaskStackSize    = 8000;
#else  /* CPU_FAMILY != SIMSPARCSUNOS */
int logTaskStackSize	= 5000;
#endif /* CPU_FAMILY == SIMSPARCSUNOS || CPU_FAMILY == SIMSPARCSOLARIS */

int mutexOptionsLogLib = SEM_Q_FIFO | SEM_DELETE_SAFE; /* mutex options */

int logFdFromRlogin	= NONE;		/* fd of pty for rlogin */


/* local variables */

LOCAL SEMAPHORE	logFdSem;		/* semaphore for accessing logFd's */
LOCAL MSG_Q_ID	logMsgQId = 0;		/* ID of misg q to log task */
LOCAL int	logFd [MAX_LOGFDS];	/* output fd's used for logging */
LOCAL int	numLogFds = 0;		/* number of active logging fd's */
LOCAL int	logMsgsLost = 0;	/* count of log messages lost */

/* forward static functions */

static void lprintf (char *fmt, int arg1, int arg2, int arg3, int arg4,
		     int arg5, int arg6);


/*******************************************************************************
*
* logInit - initialize message logging library
*
* This routine specifies the file descriptor to be used as the logging
* device and the number of messages that can be in the logging queue.  If
* more than <maxMsgs> are in the queue, they will be discarded.  A message
* is printed to indicate lost messages.
*
* This routine spawns logTask(), the task-level portion of error logging.
*
* This routine must be called before any other routine in logLib.
* This is done by the root task, usrRoot(), in usrConfig.c.
*
* RETURNS
* OK, or ERROR if a message queue could not be created
* or logTask() could not be spawned.
*/

STATUS logInit
    (
    int fd,             /* file descriptor to use as logging device */
    int maxMsgs         /* max. number of messages allowed in log queue */
    )
    {
    if (logTaskId != 0)
	return (ERROR);         /* already called */

    logMsgQId = msgQCreate (maxMsgs, sizeof (LOG_MSG), MSG_Q_FIFO);

    if (logMsgQId == NULL)
	return (ERROR);

    logTaskId = taskSpawn ("tLogTask", logTaskPriority,
			   logTaskOptions, logTaskStackSize,
			   (FUNCPTR)logTask, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    if (logTaskId == ERROR)
	{
	/* XXX free message q? */
	return (ERROR);
	}

   _func_logMsg = (FUNCPTR) logMsg;

    semMInit (&logFdSem, mutexOptionsLogLib);

    logFdSet (fd);

    return (OK);
    }
/*******************************************************************************
*
* logMsg - log a formatted error message
*
* This routine logs a specified message via the logging task.  This
* routine's syntax is similar to printf() -- a format string is followed
* by arguments to format.  However, the logMsg() routine takes a char *
* rather than a const char * and requires a fixed number of arguments
* (6).
*
* The task ID of the caller is prepended to the specified message.
*
* SPECIAL CONSIDERATIONS
* Because logMsg() does not actually perform the output directly to the
* logging streams, but instead queues the message to the logging task,
* logMsg() can be called from interrupt service routines.
*
* However, since the arguments are interpreted by the logTask() at the
* time of actual logging, instead of at the moment when logMsg() is called,
* arguments to logMsg() should not be pointers to volatile entities
* (e.g., dynamic strings on the caller stack).
*
* logMsg() checks to see whether or not it is running in interupt context. 
* If it is, it will not block.  However, if invoked from a task, it can 
* cause the task to block.  
*
*
* For more detailed information about the use of logMsg(), see the manual
* entry for logLib.
*
* EXAMPLE
* If the following code were executed by task 20:
* .CS
*     {
*     name = "GRONK";
*     num = 123;
*
*     logMsg ("ERROR - name = %s, num = %d.\en", name, num, 0, 0, 0, 0);
*     }
* .CE
* the following error message would appear on the system log:
* .CS
*     0x180400 (t20): ERROR - name = GRONK, num = 123.
* .CE
*
* RETURNS: The number of bytes written to the log queue,
* or EOF if the routine is unable to write a message.
*
* SEE ALSO: printf(), logTask()
*/

int logMsg
    (
    char *fmt,  /* format string for print */
    int arg1,   /* first of six required args for fmt */
    int arg2,
    int arg3,
    int arg4,
    int arg5,
    int arg6
    )
    {
    int timeout;
    LOG_MSG msg;

    if (INT_CONTEXT ())
	{
	msg.id = -1;
	timeout = NO_WAIT;
	}
    else
	{
	msg.id = taskIdSelf ();
	timeout = WAIT_FOREVER;
	}

    msg.fmt    = fmt;
    msg.arg[0] = arg1;
    msg.arg[1] = arg2;
    msg.arg[2] = arg3;
    msg.arg[3] = arg4;
    msg.arg[4] = arg5;
    msg.arg[5] = arg6;

    if (msgQSend (logMsgQId, (char *) &msg, sizeof (msg), timeout,
		  MSG_PRI_NORMAL) != OK)
	{
	++logMsgsLost;
	return (EOF);
	}

    return (sizeof (msg));
    }
/*******************************************************************************
*
* logFdSet - set the primary logging file descriptor
*
* This routine changes the file descriptor where messages from logMsg() 
* are written, allowing the log device to be changed from the default
* specified by logInit().  It first removes the old file descriptor (if 
* one had been previously set) from the log file descriptor list, then 
* adds the new <fd>.
*
* The old logging file descriptor is not closed or affected by this call; 
* it is simply no longer used by the logging facilities.
*
* RETURNS: N/A
*
* SEE ALSO: logFdAdd(), logFdDelete()
*/

void logFdSet
    (
    int fd      /* file descriptor to use as logging device */
    )
    {
    static int oldLogFd = NONE;

    if (oldLogFd != NONE)
	logFdDelete (oldLogFd);

    /* if we are called from an rlogin session, remove pty fd from log fd list */

    if (logFdFromRlogin != NONE)
	{
	logFdDelete (logFdFromRlogin);
	logFdFromRlogin = NONE;		/* reset since its deleted from list */
	}

    if (logFdAdd (fd) == OK)
	oldLogFd = fd;
    else
	oldLogFd = NONE;
    }
/*******************************************************************************
*
* logFdAdd - add a logging file descriptor
*
* This routine adds to the log file descriptor list another file descriptor
* <fd> to which messages will be logged.  The file descriptor must be a 
* valid open file descriptor.
*
* RETURNS:
* OK, or ERROR if the allowable number of additional logging file descriptors
* (5) is exceeded.
*
* SEE ALSO: logFdDelete()
*/

STATUS logFdAdd
    (
    int fd      /* file descriptor for additional logging device */
    )
    {
    semTake (&logFdSem, WAIT_FOREVER);

    if ((numLogFds + 1) > MAX_LOGFDS)
	{
	semGive (&logFdSem);

	/* XXX errnoSet (S_logLib_TOO_MANY_LOGGING_FDS); */
	return (ERROR);
	}

    logFd [numLogFds++] = fd;

    semGive (&logFdSem);

    return (OK);
    }
/*******************************************************************************
*
* logFdDelete - delete a logging file descriptor
*
* This routine removes from the log file descriptor list a logging file 
* descriptor added by logFdAdd().  The file descriptor is not closed; but is
* no longer used by the logging facilities.
*
* RETURNS:
* OK, or ERROR if the file descriptor was not added with logFdAdd().
*
* SEE ALSO: logFdAdd()
*/

STATUS logFdDelete
    (
    int fd      /* file descriptor to stop using as logging device */
    )
    {
    FAST int ix;
    FAST int jx;

    semTake (&logFdSem, WAIT_FOREVER);

    for (ix = jx = 0; ix < numLogFds; ix++, jx++)
	{
	/* shift array of logFd's after deleting unwanted fd */

	if (((logFd [jx] = logFd [ix]) == fd) && ix == jx)
	    jx--;
	}

    if (ix == jx)
	{
	semGive (&logFdSem);
	return (ERROR);		/* didn't find specified fd */
	}

    numLogFds--;

    semGive (&logFdSem);

    return (OK);
    }
/*******************************************************************************
*
* logTask - message-logging support task
*
* This routine prints the messages logged with logMsg().  It waits on a
* message queue and prints the messages as they arrive on the file descriptor
* specified by logInit() (or a subsequent call to logFdSet() or logFdAdd()).
*
* This task is spawned by logInit().
*
* RETURNS: N/A
*
* SEE ALSO: logMsg()
*/

void logTask (void)
    {
    static int oldMsgsLost;

    int newMsgsLost;	/* used in case logMsgsLost is changed during use */
    LOG_MSG msg;
    char *checkName;

    FOREVER
	{
	if (msgQReceive (logMsgQId, (char *) &msg, sizeof (msg),
			 WAIT_FOREVER) != sizeof (msg))
	    lprintf ("logTask: error reading log messages.\n", 0, 0, 0, 0, 0,0);
	else
	    {
	    /* print task ID followed by caller's message */

	    /* print task ID */

	    if (msg.id == -1)
		{
		lprintf ("interrupt: ", 0, 0, 0, 0, 0, 0);
		}
	    else
		{
		if ((checkName = taskName (msg.id)) == NULL)
		    lprintf ("%#x (): task dead", msg.id, 0, 0, 0, 0, 0);
		else
		    lprintf ("%#x (%s): ", msg.id, (int)checkName, 0, 0, 0, 0);
		}

	    if (msg.fmt == NULL)
		lprintf ("<null \"fmt\" parameter>\n", 0, 0, 0, 0, 0, 0);
	    else
		{
		lprintf (msg.fmt, msg.arg[0], msg.arg[1], msg.arg[2],
				  msg.arg[3], msg.arg[4], msg.arg[5]);
		}
	    }

	/* check for any more messages lost */

	newMsgsLost = logMsgsLost;

	if (newMsgsLost != oldMsgsLost)
	    {
	    lprintf ("logTask: %d log messages lost.\n",
		     newMsgsLost - oldMsgsLost, 0, 0, 0, 0, 0);

	    oldMsgsLost = newMsgsLost;
	    }
	}
    }
/*******************************************************************************
*
* lprintf - log printf
*
* Performs an fdprintf on all logFds.
*/

LOCAL void lprintf
    (
    char *fmt,	/* format string for print */
    int arg1,	/* optional arguments to fmt */
    int arg2,
    int arg3,
    int arg4,
    int arg5,
    int arg6 
    )
    {
    FAST int ix;

    semTake (&logFdSem, WAIT_FOREVER);

    for (ix = 0; ix < numLogFds; ix++)
	fdprintf (logFd [ix], fmt, arg1, arg2, arg3, arg4, arg5, arg6);

    semGive (&logFdSem);
    }
/*******************************************************************************
*
* logShow - show active logging fd's (debug only)
*
* NOMANUAL
*/

void logShow (void)
    {
    FAST int ix;

    printf ("%3s %3s\n", "num", "fd");
    printf ("%3s %3s\n", "---", "--");

    for (ix = 0; ix < numLogFds; ix++)
	printf ("%3d %3d\n", ix, logFd [ix]);

    /* XXX timeout, message size, ...
    msgQShow (logMsgQId, 1);
    */
    }
