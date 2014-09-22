/* pipeDrv.c - pipe I/O driver */

/* Copyright 1995-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
02t,17oct01,dcb  Added include for memPartLib.h to pick up the KHEAP_ macros.
02s,05oct01,dcb  Fix SPR 9434.  Add pipeDevDelete() call.  Code merged back
                 from the AE tree (created for AE under SPR 26204).
02r,14mar99,jdi  doc: removed refs to config.h and/or configAll.h (SPR 25663).
02q,12jun96,dbt  fixed SPR 5834 in pipeOpen. 
		 Update Copyright.
02p,22jan93,jdi  documentation cleanup for 5.1.
02o,21oct92,jdi  removed mangen SECTION designation.
02n,21sep92,jmm  fixed problem with select checking for writeable fd's (spr1095)
02m,23aug92,jcf  fixed race with select and pipeWrite.
02l,18jul92,smb  Changed errno.h to errnoLib.h.
02k,04jul92,jcf  scalable/ANSI/cleanup effort.
02j,26may92,rrr  the tree shuffle
02i,16dec91,gae  added includes for ANSI.
02h,25nov91,rrr  cleanup of some ansi warnings.
02g,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed VOID to void
		  -changed copyright notice
02f,05apr91,jdi	 documentation -- removed header parens and x-ref numbers;
		 doc review by dnw.
02e,27feb91,jaa	 documentation cleanup.
02d,05oct90,dnw  changed to new message queue interface.
		 finished documentation.
02c,04oct90,shl  made pipe{Open,Read,Write,Ioctl} be LOCAL.
02b,17jul90,dnw  lint
02a,10jun90,dnw  rewritten to use msgQ
01h,15mar90,rdc  added select support.
01g,08aug89,dab  fixed bug in pipeDevCreate() that set the wrong value for
                   the size of a pipe.
01f,21nov88,jcf  lint.
01e,01oct88,gae  restored FIONMSGS and FIOFLUSH lost in new version 01a.
01d,26aug88,gae  documentation.
01c,30may88,dnw  changed to v4 names.
01b,04may88,jcf  changed semaphores for new semLib.
... old versions deleted - see RCS
*/

/*
DESCRIPTION
The pipe driver provides a mechanism that lets tasks communicate with each 
other through the standard I/O interface.  Pipes can be read and written with
normal read() and write() calls.  The pipe driver is initialized with
pipeDrv().  Pipe devices are created with pipeDevCreate().

The pipe driver uses the VxWorks message queue facility to do the actual
buffering and delivering of messages.  The pipe driver simply provides
access to the message queue facility through the I/O system.  The main
differences between using pipes and using message queues directly are:
.iP "" 4
pipes are named (with I/O device names).
.iP
pipes use the standard I/O functions -- open(), close(), read(),
write() -- while message queues use the functions msgQSend() and msgQReceive().
.iP
pipes respond to standard ioctl() functions.
.iP
pipes can be used in a select() call.
.iP
message queues have more flexible options for timeouts and message
priorities.
.iP
pipes are less efficient than message queues because of the additional
overhead of the I/O system.
.LP

INSTALLING THE DRIVER
Before using the driver, it must be initialized and installed by calling
pipeDrv().  This routine must be called before any pipes are created.
It is called automatically by the root task, usrRoot(), in usrConfig.c when
the configuration macro INCLUDE_PIPES is defined.

CREATING PIPES
Before a pipe can be used, it must be created with pipeDevCreate().
For example, to create a device pipe "/pipe/demo" with up to 10 messages
of size 100 bytes, the proper call is:
.CS
    pipeDevCreate ("/pipe/demo", 10, 100);
.CE

USING PIPES
Once a pipe has been created it can be opened, closed, read, and written
just like any other I/O device.  Often the data that is read and written
to a pipe is a structure of some type.  Thus, the following example writes
to a pipe and reads back the same data:
.ne 5
.CS
    {
    int fd;
    struct msg outMsg;
    struct msg inMsg;
    int len;

    fd = open ("/pipe/demo", O_RDWR);

    write (fd, &outMsg, sizeof (struct msg));
    len = read (fd, &inMsg, sizeof (struct msg));

    close (fd);
    }
.CE
The data written to a pipe is kept as a single message and will be
read all at once in a single read.  If read() is called with a buffer
that is smaller than the message being read, the remainder of the message
will be discarded.  Thus, pipe I/O is "message oriented" rather than
"stream oriented."  In this respect, VxWorks pipes differ significantly 
from UNIX pipes which are stream oriented and do not preserve message 
boundaries.

WRITING TO PIPES FROM INTERRUPT SERVICE ROUTINES
Interrupt service routines (ISR) can write to pipes, providing one of several
ways in which ISRs can communicate with tasks.  For example, an interrupt
service routine may handle the time-critical interrupt response and then
send a message on a pipe to a task that will continue with the less
critical aspects.  However, the use of pipes to communicate from an ISR to
a task is now discouraged in favor of the direct message queue facility,
which offers lower overhead (see the manual entry for msgQLib for more
information).

SELECT CALLS
An important feature of pipes is their ability to be used in a select()
call.  The select() routine allows a task to wait for input from any of a
selected set of I/O devices.  A task can use select() to wait for input
from any combination of pipes, sockets, or serial devices.  See the manual
entry for select().

IOCTL FUNCTIONS
Pipe devices respond to the following ioctl() functions.
These functions are defined in the header file ioLib.h.

.iP "FIOGETNAME" 18 3
Gets the file name of fd and copies it to the buffer referenced by <nameBuf>:
.CS
    status = ioctl (fd, FIOGETNAME, &nameBuf);
.CE
.iP "FIONREAD"
Copies to <nBytesUnread> the number of bytes remaining in the first message
in the pipe:
.CS
    status = ioctl (fd, FIONREAD, &nBytesUnread);
.CE
.iP "FIONMSGS"
Copies to <nMessages> the number of discrete messages remaining in the pipe:
.CS
    status = ioctl (fd, FIONMSGS, &nMessages);
.CE
.iP "FIOFLUSH"
Discards all messages in the pipe and releases the memory block that contained
them:
.CS
    status = ioctl (fd, FIOFLUSH, 0);
.CE

INCLUDE FILES: ioLib.h, pipeDrv.h

SEE ALSO: select(), msgQLib,
.pG "I/O System"
*/

#include "vxWorks.h"
#include "ioLib.h"
#include "iosLib.h"
#include "stdlib.h"
#include "lstLib.h"
#include "selectLib.h"
#include "semLib.h"
#include "intLib.h"
#include "taskLib.h"
#include "errnoLib.h"
#include "string.h"
#include "memPartLib.h"
#include "private/msgQLibP.h"


typedef struct			/* PIPE_DEV */
    {
    DEV_HDR		devHdr;		/* pipe device header */
    MSG_Q		msgQ;		/* underlying message queue */
    SEL_WAKEUP_LIST	selWakeupList;	/* list of tasks pended in select */
    UINT		numOpens;	/* number of pipe terminals open */
    } PIPE_DEV;


/* globals */

int pipeMsgQOptions = MSG_Q_FIFO;	/* options with which msg queues are
					 * created */

/* locals */

LOCAL int pipeDrvNum = ERROR;		/* driver number of pipe driver */


/* forward static functions */

static int pipeOpen (PIPE_DEV *pPipeDev, char *name, int flags, int mode);
static int pipeClose (PIPE_DEV *pPipeDev);
static int pipeRead (PIPE_DEV *pPipeDev, char *buffer, unsigned int maxbytes);
static int pipeWrite (PIPE_DEV *pPipeDev, char *buffer, int nbytes);
static STATUS pipeIoctl (PIPE_DEV *pPipeDev, int request, int *argptr);


/*******************************************************************************
*
* pipeDrv - initialize the pipe driver
*
* This routine initializes and installs the driver.  It must be called
* before any pipes are created.  It is called automatically by the root
* task, usrRoot(), in usrConfig.c when the configuration macro INCLUDE_PIPES
* is defined.
*
* RETURNS: OK, or ERROR if the driver installation fails.
*/

STATUS pipeDrv (void)
    {
    /* check if driver already installed */

    if (pipeDrvNum == ERROR)
	{
	pipeDrvNum = iosDrvInstall ((FUNCPTR) NULL, (FUNCPTR) NULL, pipeOpen,
				    pipeClose, pipeRead, pipeWrite, pipeIoctl);
	}

    return (pipeDrvNum == ERROR ? ERROR : OK);
    }

/*******************************************************************************
*
* pipeDevCreate - create a pipe device
*
* This routine creates a pipe device.  It cannot be called from an interrupt
* service routine.
* It allocates memory for the necessary structures and initializes the device.
* The pipe device will have a maximum of <nMessages> messages of up to
* <nBytes> each in the pipe at once.  When the pipe is full, a task attempting
* to write to the pipe will be suspended until a message has been read.
* Messages are lost if written to a full pipe at interrupt level.
*
* RETURNS: OK, or ERROR if the call fails.
*
* ERRNO
* S_ioLib_NO_DRIVER - driver not initialized
* S_intLib_NOT_ISR_CALLABLE - cannot be called from an ISR
*/

STATUS pipeDevCreate
    (
    char *name,         /* name of pipe to be created      */
    int nMessages,      /* max. number of messages in pipe */
    int nBytes          /* size of each message            */
    )
    {
    FAST PIPE_DEV *pPipeDev;

    /* can't be called from ISR */

    if (INT_RESTRICT () != OK)
        {
        return (ERROR);
        }

    if (pipeDrvNum == ERROR)
        {
        errnoSet (S_ioLib_NO_DRIVER);
        return (ERROR);
        }

    pPipeDev = (PIPE_DEV *) KHEAP_ALLOC ((unsigned) sizeof (PIPE_DEV) +
					 (nMessages * MSG_NODE_SIZE (nBytes)));
    if (pPipeDev == NULL)
	return (ERROR);

    /* initialize open link counter, message queue, and select list */

    pPipeDev->numOpens = 0;
    if (msgQInit (&pPipeDev->msgQ, nMessages, nBytes, pipeMsgQOptions,
		  (void *) (((char *) pPipeDev) + sizeof (PIPE_DEV))) != OK)
	{
	KHEAP_FREE ((char *) pPipeDev);
	return (ERROR);
	}

    selWakeupListInit (&pPipeDev->selWakeupList);

    /* I/O device to system */

    if (iosDevAdd (&pPipeDev->devHdr, name, pipeDrvNum) != OK)
	{
	msgQTerminate (&pPipeDev->msgQ);
	KHEAP_FREE ((char *)pPipeDev);
	return (ERROR);
	}

    return (OK);
    }

/*******************************************************************************
*
* pipeDevDelete - delete a pipe device
*
* This routine deletes a pipe device of a given name.  The name must match
* that passed to pipeDevCreate() else ERROR will be returned.  This routine
* frees memory for the necessary structures and deletes the device.  It cannot
* be called from an interrupt service routine.
*
* A pipe device cannot be deleted until its number of open requests has been
* reduced to zero by an equal number of close requests and there are no tasks
* pending in its select list.  If the optional force flag is asserted, the
* above restrictions are ignored, resulting in forced deletion of any select
* list and freeing of pipe resources.
*
* CAVEAT: Forced pipe deletion can have catastrophic results if used
* indescriminately.  Use only as a last resort.
*
* RETURNS: OK, or ERROR if the call fails.
*
* ERRNO
* S_ioLib_NO_DRIVER         - driver not initialized
* S_intLib_NOT_ISR_CALLABLE - cannot be called from an ISR
* EMFILE                    - pipe still has other openings
* EBUSY                     - pipe is selected by at least one pending task
*/

STATUS pipeDevDelete
    (
    char * name,	/* name of pipe to be deleted */
    BOOL   force	/* if TRUE, force pipe deletion */
    )
    {
    FAST PIPE_DEV *   pPipeDev;
    char *            pTail = NULL;
    SEL_WAKEUP_NODE * pNode = NULL;

    /* can't be called from ISR */

    if (INT_RESTRICT () != OK)
        {
        return (ERROR);
        }

    /* driver must be initialized */

    if (pipeDrvNum == ERROR)
        {
        errno = S_ioLib_NO_DRIVER;
        return (ERROR);
        }

    /* get pointer to pipe device descriptor */

    if ((pPipeDev = (PIPE_DEV *) iosDevFind (name, &pTail)) == NULL)
        {
	return (ERROR);
        }

    /* if not forced, check for other opens and non-empty select list */

    if (!force)
        {
        if (pPipeDev->numOpens != 0)
            {
            errno = EMFILE;
            return (ERROR);
            }

        if (selWakeupListLen (&pPipeDev->selWakeupList) != 0)
            {
            errno = EBUSY;
            return (ERROR);
            }
        }

    /* I/O device no longer in system */

    iosDevDelete (&pPipeDev->devHdr);

    /* force clearing of any select list */

    if(force && (selWakeupListLen (&pPipeDev->selWakeupList) != 0))
        {
        pNode = (SEL_WAKEUP_NODE *)lstFirst ((LIST *)&pPipeDev->selWakeupList);
        do
            {
            selNodeDelete (&pPipeDev->selWakeupList, pNode);
            } while ((pNode = (SEL_WAKEUP_NODE *) lstNext ((NODE *)pNode))
                           != NULL);
        lstFree ((LIST *)&pPipeDev->selWakeupList);
        selWakeupListTerm (&pPipeDev->selWakeupList);
        }

    /* terminate message queue */

    msgQTerminate (&pPipeDev->msgQ);

    /* free pipe memory */

    KHEAP_FREE ((char *)pPipeDev);

    return (OK);
    }

/* routines supplied to I/O system */

/*******************************************************************************
*
* pipeOpen - open a pipe file
*
* This routine is called to open a pipe.  It returns a pointer to the
* device.  This routine is normally reached only via the I/O system.
*
* RETURNS  pPipeDev or ERROR if pipe has not been created by pipeDevCreate().
*/

LOCAL int pipeOpen
    (
    PIPE_DEV * pPipeDev,	/* pipe descriptor */
    char *     name,
    int        flags,
    int        mode
    )
    {
    if ((name != NULL) && (strlen (name) > 0))
	{
	/* Only the first part of the name match with the driver's name */ 

	errnoSet (S_ioLib_NO_DEVICE_NAME_IN_PATH);
	return (ERROR);
	}
    else
	{
	/* track number of openings to pipe */

	++pPipeDev->numOpens;
    	return ((int) pPipeDev);
	}
    }

/*******************************************************************************
*
* pipeClose - close a pipe file
*
* This routine is called to close a pipe.  This routine is normally reached
* only via the I/O system.
*
* RETURNS  OK or ERROR if NULL pipe device pointer.
*/

LOCAL int pipeClose
    (
    PIPE_DEV * pPipeDev		/* pipe descriptor */
    )
    {
    if (pPipeDev != NULL)
	{
	/* decrement the open counter, but not past zero */ 

	if (pPipeDev->numOpens > 0)
	  --pPipeDev->numOpens;
	return (OK);
	}
    else
	{
    	return (ERROR);
	}
    }

/*******************************************************************************
*
* pipeRead - read bytes from a pipe
*
* This routine reads up to maxbytes bytes of the next message in the pipe.
* If the message is too long, the additional bytes are just discarded.
*
* RETURNS:
*  number of bytes actually read;
*  will be between 1 and maxbytes, or ERROR
*/

LOCAL int pipeRead
    (
    FAST PIPE_DEV * pPipeDev,	/* pointer to pipe descriptor */
    char *          buffer,	/* buffer to receive bytes */
    unsigned int    maxbytes	/* max number of bytes to copy into buffer */
    )
    {
    int nbytes;

    /* wait for something to be in pipe */

    nbytes = msgQReceive (&pPipeDev->msgQ, buffer, maxbytes, WAIT_FOREVER);

    if (nbytes == ERROR)
	return (ERROR);

    /* wake up any select-blocked writers */

    selWakeupAll (&pPipeDev->selWakeupList, SELWRITE);

    return (nbytes);
    }

/*******************************************************************************
*
* pipeWrite - write bytes to a pipe
*
* This routine writes a message of `nbytes' to the pipe.
*
* RETURNS: number of bytes written or ERROR
*/

LOCAL int pipeWrite
    (
    FAST PIPE_DEV * pPipeDev,	/* pointer to pipe descriptor */
    char *          buffer,	/* buffer from which to copy bytes */
    int             nbytes	/* number of bytes to copy from buffer */
    )
    {
    if (!INT_CONTEXT ())
	TASK_LOCK ();				/* LOCK PREEMPTION */

    /* 
     * We lock preemption so after we send the message we can guarantee that
     * we get to the selWakeupAll() call before unblocking any readers.  This
     * is to avoid a race in which a higher priority reader of the pipe is
     * unblocked by the msgQSend() below and subsequently enters and blocks
     * in a call to select(), only to be inadvertently awakened when we return
     * here and call selWakeupAll().  To minimize preemption latency we
     * release the preemption lock after we obtain the selWakeupList mutual
     * exclusion semaphore.  This semaphore is a mutual exclusion semaphore
     * which allows recursive takes.  Avoiding a preemption lock by utilizing
     * the selWakeupList semaphore as the only means of mutual exclusion is
     * not viable because deadlock can occur by virtue of the fact that
     * msgQSend() can block if the the message queue is full at which time a
     * call to select() could block waiting for the listMutex instead of
     * returning that a read is OK.  A problem this approach does not account
     * for is the possibility that the selWakeupList semaphore is unavailable
     * when the semTake() is attempted below.  If this were the case, the
     * task could be preempted and therefore be vulnerable to the same
     * scenario outlined above.  
     */

    if (msgQSend (&pPipeDev->msgQ, buffer, (UINT) nbytes,
		  INT_CONTEXT() ? NO_WAIT : WAIT_FOREVER, MSG_PRI_NORMAL) != OK)
	{
	if (!INT_CONTEXT ())
	    TASK_UNLOCK ();			/* UNLOCK PREEMPTION */
	return (ERROR);
	}

    if (!INT_CONTEXT ())
	{
	semTake (&pPipeDev->selWakeupList.listMutex, WAIT_FOREVER);
	TASK_UNLOCK ();				/* UNLOCK PREEMPTION */
	}

    /* wake up any select-blocked readers */

    selWakeupAll (&pPipeDev->selWakeupList, SELREAD);

    if (!INT_CONTEXT ())
	semGive (&pPipeDev->selWakeupList.listMutex);

    return (nbytes);
    }

/*******************************************************************************
*
* pipeIoctl - do device specific control function
*
* The ioctl requests recognized are FIONREAD, FIONMSGS, and FIOFLUSH.
*
* RETURNS:
*  OK and `argptr' gets number of bytes in pipe, or
*  ERROR if request is not FIONREAD, FIONMSGS, or FIOFLUSH.
*/

LOCAL STATUS pipeIoctl
    (
    FAST PIPE_DEV *pPipeDev,	/* pointer to pipe descriptor */
    int           request,	/* ioctl code */
    int           *argptr	/* where to send answer */
    )
    {
    STATUS	status = OK;
    MSG_Q_INFO	msgQInfo;
    SEL_WAKEUP_NODE * wakeNode = (SEL_WAKEUP_NODE *) argptr;

    switch (request)
	{
	case FIONREAD:
	    /* number of bytes in 1st message in the queue */

	    bzero ((char *) &msgQInfo, sizeof (msgQInfo));
	    msgQInfo.msgListMax	= 1;
	    msgQInfo.msgLenList	= argptr;
	    *argptr = 0;
	    msgQInfoGet (&pPipeDev->msgQ, &msgQInfo);
	    break;

	case FIONMSGS:
	    /* number of messages in pipe */

	    *argptr = msgQNumMsgs (&pPipeDev->msgQ);
	    break;

	case FIOFLUSH:
	    /* discard all outstanding messages */

	    taskLock ();
	    while (msgQReceive (&pPipeDev->msgQ, (char *) NULL, 0, NO_WAIT) !=
									ERROR)
		;
	    taskUnlock ();
	    break;

	case FIOSELECT:
	    selNodeAdd (&pPipeDev->selWakeupList, (SEL_WAKEUP_NODE *) argptr);

	    switch (wakeNode->type)
		{
		case SELREAD:
		    if (msgQNumMsgs (&pPipeDev->msgQ) > 0)
		        selWakeup ((SEL_WAKEUP_NODE *) argptr);
		    break;
		    
		case SELWRITE:
		    if (pPipeDev->msgQ.maxMsgs >
			msgQNumMsgs (&pPipeDev->msgQ))
		        selWakeup ((SEL_WAKEUP_NODE *) argptr);
                    break;
		}
            break;

        case FIOUNSELECT:
	    selNodeDelete (&pPipeDev->selWakeupList, (SEL_WAKEUP_NODE *)argptr);
            break;

	default:
	    status = ERROR;
	    errnoSet (S_ioLib_UNKNOWN_REQUEST);
	    break;
	}

    return (status);
    }
