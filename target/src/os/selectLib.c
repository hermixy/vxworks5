/* selectLib.c - UNIX BSD 4.3 select library */

/* Copyright 1984-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
02b,20may02,gls  Added semTake to select to prevent race condition (SPR #77032)
02a,30nov01,sbs  Added documentation about FD_SETSIZE (SPR 9377)
01z,12oct01,brk  added SELECT functionality to ptyLib (SPR 65498) 
01y,20sep01,aeg  removed dependence on FD_SETSIZE from select() (SPR #31319).
01x,10jul97,dgp  doc: fix SPR 6476, correct no. bits examined by select()
01w,09jul97,dgp  doc: add max number of file descripters (SPR 7695)
01v,13feb95,jdi  doc tweaks.
01u,03feb93,jdi  doc changes based on review by rdc.
01t,21jan93,jdi  documentation cleanup for 5.1.
01s,23aug92,jcf  added tyLib select stuff here to make tyLib standalone.
01r,18jul92,smb  Changed errno.h to errnoLib.h.
01q,04jul92,jcf  scalable/ANSI/cleanup effort.
01p,26may92,rrr  the tree shuffle
01o,10jan92,rdc  Fixed bug in selNodeAdd - release semaphore after malloc error.
01n,09jan92,rdc  integrated fix from 5.0.2 that fixed timeout bug in select
		 causing incorrect return fdsets.
01m,13dec91,gae  ANSI cleanup.
01l,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed includes to have absolute path from h/
		  -changed VOID to void
		  -changed copyright notice
01k,01aug91,yao  fixed to pass 6 args to excJobAdd() call.
01j,29apr91,hdn  changed how quitTime is calculated to optimize code
01i,05apr91,jdi	 documentation -- removed header parens and x-ref numbers;
		 doc review by rdc.
01h,24mar91,jaa  documentation cleanup.
01g,25oct90,dnw  deleted include of utime.h.
01f,01aug90,jcf  changed tcb selWakeupNode to pSelWakeupNode.
01e,13jul90,rdc  added mechanism to clean up after tasks that get deleted
		 while pended in select.
01d,26jun90,jcf  changed binary semaphore interface
01c,17may90,rdc  changed to use binary sem's and timeouts instead of WDOGS.
01b,14apr90,jcf  removed tcb extension dependencies.
01a,02jan90,rdc  written.
*/

/*
DESCRIPTION
This library provides a BSD 4.3 compatible \f2select\fP facility to wait
for activity on a set of file descriptors.  selectLib provides a mechanism
that gives a driver the ability to detect pended tasks that are awaiting
activity on the driver's device.  This allows a driver's interrupt service
routine to wake up such tasks directly, eliminating the need for polling.

Applications can use select() with pipes and serial devices, in addition
to sockets.  Also, select() examines \f2write\f1 file descriptors in addition
to \f2read\f1 file descriptors; however, exception file descriptors remain
unsupported.

Typically, application developers need concern themselves only with the
select() call.  However, driver developers should become familiar with the
other routines that may be used with select(), if they wish to support the
select() mechanism.

The select facility is included in a system when VxWorks is configured
with the INCLUDE_SELECT component.

INCLUDE FILES: selectLib.h

SEE ALSO:
.pG "I/O System"
*/


#include "vxWorks.h"
#include "ioLib.h"
#include "memLib.h"
#include "errnoLib.h"
#include "vwModNum.h"
#include "net/systm.h"
#include "semLib.h"
#include "logLib.h"
#include "string.h"
#include "sysLib.h"
#include "intLib.h"
#include "stdlib.h"
#include "taskHookLib.h"
#include "selectLib.h"
#include "tyLib.h"
#include "ptyDrv.h"
#include "private/funcBindP.h"
#include "private/selectLibP.h"
#include "private/iosLibP.h"		/* maxFiles */

/* global variables */

int mutexOptionsSelectLib = SEM_Q_FIFO | SEM_DELETE_SAFE;

/* forward declarations */

void selWakeupAll ();


/* forward static functions */

LOCAL void selTyAdd		(TY_DEV *pTyDev, int arg);
LOCAL void selTyDelete		(TY_DEV *pTyDev, int arg);
LOCAL void selPtyAdd            (PSEUDO_DEV *pPtyDev, int arg);
LOCAL void selPtyDelete         (PSEUDO_DEV *pPtyDev, int arg);
LOCAL void selTaskDeleteHook 	(WIND_TCB *pTcb);
LOCAL STATUS selTaskCreateHook	(WIND_TCB *pNewTcb);
LOCAL STATUS selDoIoctls	(fd_set *pFdSet, int fdSetWidth, int ioctlFunc,
				 SEL_WAKEUP_NODE *pWakeupNode, BOOL stopOnErr);


/*******************************************************************************
*
* selectInit - initialize the select facility
*
* This routine initializes the UNIX BSD 4.3 select facility.  It should be
* called only once, and typically is called from the root task, usrRoot(),
* in usrConfig.c.  It installs a task create hook such that a select context
* is initialized for each task.
*
* RETURNS: N/A
*
* INTERNAL
* The <numFiles> parameter is supplied so that the root task can have a
* properly defined select context area. This is needed for any init routines
* executed as part of the root task which call select, e.g. nfsMountAll().
*/

void selectInit 
    (
    int		numFiles	/* maximum number of open files */
    )
    {
    _func_selTyAdd		= (FUNCPTR) selTyAdd;
    _func_selTyDelete		= (FUNCPTR) selTyDelete;
    _func_selPtyAdd		= (FUNCPTR) selPtyAdd;
    _func_selPtyDelete		= (FUNCPTR) selPtyDelete; 
    _func_selWakeupAll		= (FUNCPTR) selWakeupAll;
    _func_selWakeupListInit	= (FUNCPTR) selWakeupListInit;
    _func_selWakeupListTerm	= (FUNCPTR) selWakeupListTerm;

    if (taskCreateHookAdd ((FUNCPTR) selTaskCreateHook) != OK)
	{
	if (_func_logMsg != NULL)
	    {
	    _func_logMsg ("selectInit: couldn't install task create hook!\n",
			   0, 0, 0, 0, 0, 0);
	    }
	return;
	}

    /*
     * Make sure maxFiles has the correct value in it. This will be overridden
     * later, but this will allow the root task to use select too.
     */

    if (maxFiles == 0)
	maxFiles = numFiles;

    /*
     * Create the select context for the calling task too.  This should be
     * the root task, but we don't check.  Instead we just make sure that
     * the task doesn't already have a select context.
     */

    if (taskIdCurrent->pSelectContext == NULL)
	{
	if (selTaskCreateHook (taskIdCurrent) == ERROR)
	    {
	    if (_func_logMsg != NULL)
		{
		_func_logMsg ("selectInit: couldn't install create hook for the caller! 0x%x\n",
			       errno, 0, 0, 0, 0, 0);
		}
	    return;
	    }
	}
    }

/*******************************************************************************
*
* selTaskDeleteHookAdd - initialize the select facility (part 2)
*
* This routine installs a select task delete hook so that tasks pended in
* select() can be cleaned up properly.  This routine should only be called
* once, and typically is called from the root task, usrRoot(), in usrConfig.c.
* It should be called after RPC initialization so that the select task delete
* hook is invoked before the RPC task delete hook whenever a task dies.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void selTaskDeleteHookAdd
    (
    void
    )
    {
    if (taskDeleteHookAdd ((FUNCPTR) selTaskDeleteHook) != OK)
	{
	if (_func_logMsg != NULL)
	    {
	    _func_logMsg ("selTaskDeleteHookAdd: couldn't install task delete hook!\n",
		     0, 0, 0, 0, 0, 0);
	    }
	}
    }

/*******************************************************************************
*
* select - pend on a set of file descriptors
*
* This routine permits a task to pend until one of a set of file descriptors
* becomes ready.  Three parameters -- <pReadFds>, <pWriteFds>, and
* <pExceptFds> -- point to file descriptor sets in which each bit
* corresponds to a particular file descriptor.   Bits set in the read file
* descriptor set (<pReadFds>) will cause select() to pend until data is
* available on any of the corresponding file descriptors, while bits set in
* the write file descriptor set (<pWriteFds>) will cause select() to pend
* until any of the corresponding file descriptors become writable.  (The
* <pExceptFds> parameter is currently unused, but is provided for UNIX call
* compatibility.)
*
* The following macros are available for setting the appropriate bits
* in the file descriptor set structure:
* .CS
*     FD_SET(fd, &fdset)
*     FD_CLR(fd, &fdset)
*     FD_ZERO(&fdset)
* .CE
*
* If either <pReadFds> or <pWriteFds> is NULL, they are ignored.  The
* <width> parameter defines how many bits will be examined in the file
* descriptor sets, and should be set to either the maximum file descriptor
* value in use plus one, or simply to FD_SETSIZE.  When select() returns, 
* it zeros out the file descriptor sets, and sets only the bits that 
* correspond to file descriptors that are ready.  The FD_ISSET macro may 
* be used to determine which bits are set.
*
* If <pTimeOut> is NULL, select() will block indefinitely.  If <pTimeOut> is
* not NULL, but points to a `timeval' structure with an effective time of
* zero, the file descriptors in the file descriptor sets will be polled and
* the results returned immediately.  If the effective time value is greater
* than zero, select() will return after the specified time has elapsed, even
* if none of the file descriptors are ready.
*
* Applications can use select() with pipes and serial devices, in addition
* to sockets.  Also, select() now examines write file descriptors in
* addition to read file descriptors; however, exception file descriptors
* remain unsupported.
*
* The value for the maximum number of file descriptors configured in the
* system (NUM_FILES) should be less than or equal to the value of 
* FD_SETSIZE (2048).
*
* Driver developers should consult the 
* .I "VxWorks Programmer's Guide: I/O System"
* for details on writing drivers that will use select().
*
* INTERNAL
* The select library can handle arbitrarily large fd_sets, i.e. up to
* the maximum number of file descriptors configured in the system
* (NUM_FILES).  To maintain this capability, the use of the macro
* FD_SETSIZE must be restricted.  For example, instead of using FD_ZERO,
* use bzero().
* 
* RETURNS:
* The number of file descriptors with activity, 0 if timed out, or
* ERROR if an error occurred when the driver's select() routine
* was invoked via ioctl().
*
* ERRNOS
* Possible errnos generated by this routine include:
* .iP 'S_selectLib_NO_SELECT_SUPPORT_IN_DRIVER'
* A driver associated with one or more fds does not support select().
* .iP 'S_selectLib_NO_SELECT_CONTEXT'
* The task's select context was not initialized at task creation time.
* .iP 'S_selectLib_WIDTH_OUT_OF_RANGE'
* The width parameter is greater than the maximum possible fd.
* .LP
*
* SEE ALSO:
* .pG "I/O System"
*/

int select
    (
    int             width,      /* number of bits to examine from 0 */
    FAST fd_set    *pReadFds,   /* read fds */
    FAST fd_set    *pWriteFds,  /* write fds */
    fd_set         *pExceptFds, /* exception fds (unsupported) */
    struct timeval *pTimeOut    /* max time to wait, NULL = forever */
    )
    {
    FAST int fd;
    FAST int partMask;
    SEL_WAKEUP_NODE wakeupNode;
    int widthInBytes;
    int status;
    int numFound = 0;
    int quitTime = 0;
    int clockRate;
    SEL_CONTEXT *pSelectContext = taskIdCurrent->pSelectContext;

    /* ensure that the task's select context was successfully initialized */

    if (pSelectContext == NULL)
	{
	errno = S_selectLib_NO_SELECT_CONTEXT;
	return (ERROR);
	}

    /* gracefully handle situation where width is larger than maxFiles */

    if ((width < 0) || ((width > maxFiles) && (width > FD_SETSIZE)))
	{
	errno = S_selectLib_WIDTH_OUT_OF_RANGE;
	return (ERROR);
	}

    /* truncate width to maxFiles for cases when FD_SETSIZE is specifed */

    if (width > maxFiles)
	width = maxFiles;

    widthInBytes = howmany(width, NFDBITS) * sizeof (fd_mask);

    /*
     * Make a copy of the original read/write fd_set for potential use
     * by the task delete hook. Note that FD_ZERO is not used since it
     * uses FD_SETSIZE.
     */

    if (pReadFds != NULL)
	bcopy ((char*) pReadFds, (char *) pSelectContext->pOrigReadFds,
		widthInBytes);	
    else
	bzero ((char *) pSelectContext->pOrigReadFds, widthInBytes);

    if (pWriteFds != NULL)
	bcopy ((char*) pWriteFds, (char *) pSelectContext->pOrigWriteFds,
		widthInBytes);
    else
	bzero ((char *) pSelectContext->pOrigWriteFds, widthInBytes);

    if (pTimeOut != NULL)
	{
	clockRate = sysClkRateGet ();

	/* convert to ticks */
	quitTime = (pTimeOut->tv_sec * clockRate) +
            ((((pTimeOut->tv_usec * clockRate) / 100)/100)/100);

	}

    /* initialize select context and wakeup node fields */

    pSelectContext->pReadFds = pReadFds;
    pSelectContext->pWriteFds = pWriteFds;

    /*
     * The reason we would want to take the semaphore here is that it
     * serves to prevent a race condition.  Without this semTake it is
     * possible for the call to select to be interrupted after acquiring
     * the semaphore but BEFORE the driver's FIOUNSELECT routine is called.
     * If another 'event' occurs on the driver's device during this time
     * it will be handled by the current call to select(), but not detected.
     * That is to say, the wakeup semaphore will remain available.  The next
     * call to select will take this semaphore but find that there is no
     * 'event' to handle and return this, which is interpreted as a
     * timeout.
     *
     * By taking the semaphore prior to calling the FIOSELECT routine of the
     * driver we will then properly wait for a valid 'event' in this case.  
     * If the semaphore is already empty we will pass through as NO_WAIT 
     * is specified.  
     */

    semTake(&pSelectContext->wakeupSem, NO_WAIT);

    wakeupNode.taskId = taskIdSelf ();

    /* clear out the read and write fd_sets in task's context */

    if (pReadFds != NULL)
	bzero ((char *)pReadFds, widthInBytes);

    if (pWriteFds != NULL)
	bzero ((char *)pWriteFds, widthInBytes);

    /* Clear out caller's copy of exception fds for completeness */

    if (pExceptFds != NULL)
	bzero ((char *)pExceptFds, widthInBytes);

    status = OK;

    /* do the read fd's */

    if (pReadFds != NULL)
	{
	wakeupNode.type = SELREAD;
	if (selDoIoctls (pSelectContext->pOrigReadFds, width,
			 FIOSELECT, &wakeupNode, TRUE) != OK)
	    {
	    status = ERROR;
	    }
	}

    /* do the write fd's */

    if (status != ERROR)
	if (pWriteFds != NULL)
	    {
	    wakeupNode.type = SELWRITE;
	    if (selDoIoctls (pSelectContext->pOrigWriteFds, width,
			     FIOSELECT, &wakeupNode, TRUE) != OK)
		{
		status = ERROR;
		}
	    }

    if (status != OK)
	{
	status = errnoGet ();

	wakeupNode.type = SELREAD;

	selDoIoctls (pSelectContext->pOrigReadFds, width, FIOUNSELECT, 
		     &wakeupNode, FALSE);

	wakeupNode.type = SELWRITE;

	selDoIoctls (pSelectContext->pOrigWriteFds, width, FIOUNSELECT,
		     &wakeupNode, FALSE);

	/* if no select support in driver, inform the naive user */

	if (status == S_ioLib_UNKNOWN_REQUEST)
	    errnoSet (S_selectLib_NO_SELECT_SUPPORT_IN_DRIVER);

	return (ERROR);
	}

    /* if the user specified a zero quittime, we don't pend.
     * a NULL pointer indicates wait for ever
     */

    if ((pTimeOut != NULL) && (quitTime == 0))
	quitTime = NO_WAIT;
    else
	if (pTimeOut == NULL)
	    quitTime = WAIT_FOREVER;


    /*
     * Indicate that the current task is pended in select().  In the
     * event the current task is deleted, the task delete hook will
     * be able to remove the various wakeup nodes from driver lists.
     */

    pSelectContext->width 	   = width;
    pSelectContext->pendedOnSelect = TRUE;

    semTake (&pSelectContext->wakeupSem, quitTime);

    /* clean up after ourselves */

    /* first the read fd's ... */

    status = OK;

    if (pReadFds != NULL)
	{
	wakeupNode.type = SELREAD;
	if (selDoIoctls (pSelectContext->pOrigReadFds, width,
			 FIOUNSELECT, &wakeupNode, FALSE) != OK)
	    {
	    status = ERROR;
	    }
	}

    /* ... now the write fd's. */

    if (pWriteFds != NULL)
	{
	wakeupNode.type = SELWRITE;
	if (selDoIoctls (pSelectContext->pOrigWriteFds, width,
			 FIOUNSELECT, &wakeupNode, FALSE) != OK)
	    {
	    status = ERROR;
	    }
	}


    /* indicate that current task is no longer pended in select() */

    pSelectContext->pendedOnSelect = FALSE;

    if (status == ERROR)
	return (ERROR);

    /* find out how many bits are set in the read and write fd sets */

    if (pReadFds != NULL)
	for (fd = 0; fd < width; fd++)
	    {
	    /* look at the fd_set a long word at a time */

	    partMask = pReadFds->fds_bits[((unsigned)fd) / NFDBITS];

	    if (partMask == 0)
		{
		fd += NFDBITS - 1;
		}
	    else if (partMask & (1 << (((unsigned) fd) % NFDBITS)))
		{
		numFound++;
		}
	    }

    if (pWriteFds != NULL)
	for (fd = 0; fd < width; fd++)
	    {
	    /* look at the fd_set a long word at a time */

	    partMask = pWriteFds->fds_bits[((unsigned)fd) / NFDBITS];

	    if (partMask == 0)
		fd += NFDBITS - 1;
	    else if (partMask & (1 << (((unsigned) fd) % NFDBITS)))
		{
		numFound++;
		}
	    }

    return (numFound);
    }

/******************************************************************************
*
* selDoIoctls - perform the given ioctl on all the fd's in a mask
*
* RETURNS: OK or ERROR if ioctl failed.
*/

LOCAL STATUS selDoIoctls
    (
    FAST fd_set *pFdSet,
    FAST int fdSetWidth,
    FAST int ioctlFunc,
    FAST SEL_WAKEUP_NODE *pWakeupNode,
    BOOL stopOnErr
    )
    {
    FAST int fd;
    FAST int partMask;
    int status = OK;

    for (fd = 0; fd < fdSetWidth; fd++)
	{
	/* look at the fd_set a long word at a time */

	partMask = pFdSet->fds_bits[((unsigned)fd)/NFDBITS];

	if (partMask == 0)
	    fd += NFDBITS - 1;
	else if (partMask & (1 << (((unsigned) fd) % NFDBITS)))
	    {
	    pWakeupNode->fd = fd;
	    if (ioctl (fd, ioctlFunc, (int)pWakeupNode) != OK)
		{
		status = ERROR;
		if (stopOnErr)
		    break;
		}
	    }
	}

    return (status);
    }

/* The following routines are available to drivers to manage wake-up lists */

/******************************************************************************
*
* selWakeup - wake up a task pended in select()
*
* This routine wakes up a task pended in select().  Once a driver's
* FIOSELECT function installs a wake-up node in a device's wake-up list
* (using selNodeAdd()) and checks to make sure the device is ready, this
* routine ensures that the select() call does not pend.
*
* RETURNS: N/A
*/

void selWakeup
    (
    SEL_WAKEUP_NODE *pWakeupNode        /* node to wake up */
    )
    {
    SEL_CONTEXT *pSelectContext;

    pSelectContext = ((WIND_TCB *)pWakeupNode->taskId)->pSelectContext;

    switch (pWakeupNode->type)
	{
	case SELREAD:
	    FD_SET(pWakeupNode->fd, pSelectContext->pReadFds);
	    break;

	case SELWRITE:
	    FD_SET(pWakeupNode->fd, pSelectContext->pWriteFds);
	    break;
	}

    semGive (&pSelectContext->wakeupSem);
    }

/******************************************************************************
*
* selWakeupAll - wake up all tasks in a select() wake-up list
*
* This routine wakes up all tasks pended in select() that are waiting for
* a device; it is called by a driver when the device becomes ready.  The
* <type> parameter specifies the task to be awakened, either reader tasks
* (SELREAD) or writer tasks (SELWRITE).
*
* RETURNS: N/A
*/

void selWakeupAll
    (
    SEL_WAKEUP_LIST *pWakeupList, /* list of tasks to wake up */
    FAST SELECT_TYPE type         /* readers (SELREAD) or writers (SELWRITE) */
    )
    {
    FAST SEL_WAKEUP_NODE *pWakeupNode;

    if (lstCount (&pWakeupList->wakeupList) == 0)
	return;

    if (intContext ())
	{
	excJobAdd (selWakeupAll, (int)pWakeupList, (int)type, 0, 0, 0, 0);
	return;
	}

    semTake (&pWakeupList->listMutex, WAIT_FOREVER);
    for (pWakeupNode = (SEL_WAKEUP_NODE *) lstFirst (&pWakeupList->wakeupList);
         pWakeupNode != NULL;
         pWakeupNode = (SEL_WAKEUP_NODE *) lstNext ((NODE *) pWakeupNode))
         {
         if (pWakeupNode->type == type)
            selWakeup (pWakeupNode);
         }

    semGive (&pWakeupList->listMutex);
    }

/******************************************************************************
*
* selNodeAdd - add a wake-up node to a select() wake-up list
*
* This routine adds a wake-up node to a device's wake-up list.  It is 
* typically called from a driver's FIOSELECT function.
*
* RETURNS: OK, or ERROR if memory is insufficient.
*/

STATUS selNodeAdd
    (
    SEL_WAKEUP_LIST *pWakeupList,	/* list of tasks to wake up */
    SEL_WAKEUP_NODE *pWakeupNode	/* node to add to list */
    )
    {
    SEL_WAKEUP_NODE *pCopyOfNode;
    BOOL dontFree;

    semTake (&pWakeupList->listMutex, WAIT_FOREVER);

    if (lstCount (&pWakeupList->wakeupList) == 0)
	{
	pCopyOfNode = &pWakeupList->firstNode;
	dontFree = TRUE;
	}
    else
	{
	pCopyOfNode = (SEL_WAKEUP_NODE *) malloc (sizeof (SEL_WAKEUP_NODE));
	dontFree = FALSE;
	}

    if (pCopyOfNode == NULL)
	{
	semGive (&pWakeupList->listMutex);
	return (ERROR);
	}

    *pCopyOfNode = *pWakeupNode;

    pCopyOfNode->dontFree = dontFree;

    lstAdd (&pWakeupList->wakeupList, (NODE *) pCopyOfNode);
    semGive (&pWakeupList->listMutex);
    return (OK);
    }

/******************************************************************************
*
* selNodeDelete - find and delete a node from a select() wake-up list
*
* This routine deletes a specified wake-up node from a specified wake-up
* list.  Typically, it is called by a driver's FIOUNSELECT function.
*
* RETURNS: OK, or ERROR if the node is not found in the wake-up list.
*/

STATUS selNodeDelete
    (
    SEL_WAKEUP_LIST *pWakeupList,	/* list of tasks to wake up */
    SEL_WAKEUP_NODE *pWakeupNode	/* node to delete from list */
    )
    {
    FAST SEL_WAKEUP_NODE *pWorkNode;

    semTake (&pWakeupList->listMutex, WAIT_FOREVER);	/* get exclusion */

    for (pWorkNode = (SEL_WAKEUP_NODE *) lstFirst (&pWakeupList->wakeupList);
         pWorkNode != NULL;
         pWorkNode = (SEL_WAKEUP_NODE *) lstNext ((NODE *) pWorkNode))
	{
	if ((pWorkNode->taskId == pWakeupNode->taskId) &&
	    (pWorkNode->type == pWakeupNode->type))
	    {
	    lstDelete (&pWakeupList->wakeupList, (NODE *) pWorkNode);

	    if (!(pWorkNode->dontFree))
		free ((char *) pWorkNode);		/* deallocate node */

	    semGive (&pWakeupList->listMutex);		/* release exclusion */

	    return (OK);
	    }
         }

    semGive (&pWakeupList->listMutex);			/* release exclusion */

    return (ERROR);
    }

/******************************************************************************
*
* selWakeupListInit - initialize a select() wake-up list
*
* This routine should be called in a device's create routine to
* initialize the SEL_WAKEUP_LIST structure.
*
* RETURNS: N/A
*
*/

void selWakeupListInit
    (
    SEL_WAKEUP_LIST *pWakeupList	/* wake-up list to initialize */
    )
    {
    lstInit (&pWakeupList->wakeupList);
    semMInit (&pWakeupList->listMutex, mutexOptionsSelectLib);
    }

/******************************************************************************
*
* selWakeupListTerm - terminate a select() wake-up list
*
* This routine should be called in a device's terminate routine to
* terminate the SEL_WAKEUP_LIST structure.
*
* RETURNS: N/A
*
*/

void selWakeupListTerm
    (
    SEL_WAKEUP_LIST *pWakeupList	/* wake-up list to terminate */
    )
    {
    semTerminate (&pWakeupList->listMutex);
    }

/******************************************************************************
*
* selWakeupListLen - get the number of nodes in a select() wake-up list
*
* This routine returns the number of nodes in a specified SEL_WAKEUP_LIST.
* It can be used by a driver to determine if any tasks are currently
* pended in select() on this device, and whether these tasks need to be
* activated with selWakeupAll().
*
* RETURNS:
* The number of nodes currently in a select() wake-up list, or ERROR.
*
*/

int selWakeupListLen
    (
    SEL_WAKEUP_LIST *pWakeupList	/* list of tasks to wake up */
    )
    {
    return (lstCount (&pWakeupList->wakeupList));
    }

/******************************************************************************
*
* selWakeupType - get the type of a select() wake-up node
*
* This routine returns the type of a specified SEL_WAKEUP_NODE.
* It is typically used in a device's FIOSELECT function to determine
* if the device is being selected for read or write operations.
*
* RETURNS: SELREAD (read operation) or SELWRITE (write operation).
*
*/

SELECT_TYPE selWakeupType
    (
    SEL_WAKEUP_NODE *pWakeupNode	/* node to get type of */
    )
    {
    return (pWakeupNode->type);
    }

/******************************************************************************
*
* selTaskCreateHook - select task create hook
*
* This routine creates a select context for the task.  The select context 
* for the root task is not created by this routine.  Instead, it is created 
* by the selectInit() routine.
*
* The memory for the context is allocated from the system heap.  The amount
* of memory allocated for the context is enough to support fd_sets of size
* maxFiles, instead of the compile time value of FD_SETSIZE.
*
* This routine is installed as a create hook by selectInit.
*/

LOCAL STATUS selTaskCreateHook
    (
    WIND_TCB *pNewTcb
    )
    {
    SEL_CONTEXT *pSelectContext;
    int		 fdSetBytes;

    /*
     * Allocate sufficient memory from the heap to contain the select
     * context.  This memory includes space for the various fd_sets to
     * handle file descriptors up to maxFiles, i.e. select() will not be
     * limited to FD_SETSIZE.
     */

     fdSetBytes = sizeof (fd_mask) * howmany (maxFiles, NFDBITS);

     pSelectContext = (SEL_CONTEXT *) malloc (sizeof (SEL_CONTEXT) + 
		       				   2 * fdSetBytes);

     if (pSelectContext == NULL)
	return (ERROR);

     /* assign various convenience fd_set pointers */

     pSelectContext->pOrigReadFds   = (fd_set *) ((int) pSelectContext +
						 sizeof (SEL_CONTEXT));

     pSelectContext->pOrigWriteFds  = (fd_set *)
				      ((int) pSelectContext->pOrigReadFds +
					     fdSetBytes);

     pSelectContext->pendedOnSelect = FALSE;   /* task is not pending */


     /* initialize the task's wakeup binary semaphore */

     if (semBInit (&pSelectContext->wakeupSem, SEM_Q_PRIORITY, 
		   SEM_EMPTY) != OK)
	 {
	 free ((char*) pSelectContext);
	 return (ERROR);
	 }

    /* initialization succeeded so set task's select context pointer */

    pNewTcb->pSelectContext = pSelectContext;

    return (OK);
    }

/******************************************************************************
*
* selTaskDeleteHook - select task delete hook
*
* If a task is deleted while it is pended in select, this delete hook
* tells all the relevent devices to delete the wake-up node associated
* with this task.  This routine is installed as a delete hook by 
* selTaskDeleteHookAdd().  RPC adds a delete hook to close the client and
* server handle sockets -- we insure that the select delete hook gets 
* called before the RPC delete hook (so that the socket device still exists)
* by installing it AFTER RPC initialization (see usrConfig.c).
*/

LOCAL void selTaskDeleteHook
    (
    WIND_TCB *pTcb
    )
    {
    SEL_CONTEXT *pSelectContext = pTcb->pSelectContext;

    /* 
     * A wakeup node is created on the deleting task's stack since
     * the work node on the stack of the task being deleting may
     * be inaccessible to the current task.
     */

    SEL_WAKEUP_NODE wakeupNode;

    if (pSelectContext != NULL)
	{
	/* task pended in select()? */

	if (pSelectContext->pendedOnSelect == TRUE)
	    {
	    wakeupNode.taskId = (int) pTcb;

	    /* unregister all the read fd's */

	    wakeupNode.type   = SELREAD;

	    selDoIoctls (pSelectContext->pOrigReadFds, pSelectContext->width,
			 FIOUNSELECT, &wakeupNode, FALSE);

	    /* unregister all the write fd's */

	    wakeupNode.type = SELWRITE;

	    selDoIoctls (pSelectContext->pOrigWriteFds, pSelectContext->width,
			 FIOUNSELECT, &wakeupNode, FALSE);
	    }

	semTerminate (&pSelectContext->wakeupSem);

	free ((char *) pSelectContext);
	}
    }
		
/*******************************************************************************
*
* selTyAdd - tyLib FIOSELECT ioctl
*
* This is located here to keep tyLib from dragging in selectLib.
*/

LOCAL void selTyAdd 
    (
    TY_DEV *pTyDev,
    int arg 
    )
    {
    selNodeAdd (&pTyDev->selWakeupList, (SEL_WAKEUP_NODE *) arg);

    if ((selWakeupType ((SEL_WAKEUP_NODE *) arg) == SELREAD) &&
		 (rngNBytes (pTyDev->rdBuf) > 0))
	{
	selWakeup ((SEL_WAKEUP_NODE *) arg);
	}

    if ((selWakeupType ((SEL_WAKEUP_NODE *) arg) == SELWRITE) &&
		 rngFreeBytes (pTyDev->wrtBuf) > 0)
	{
	selWakeup ((SEL_WAKEUP_NODE *) arg);
	}
    }

/*******************************************************************************
*
* selTyDelete - tyLib FIOUNSELECT ioctl
*
* This is located here to keep tyLib from dragging in selectLib.
*/

LOCAL void selTyDelete
    (
    TY_DEV *pTyDev,
    int arg 
    )
    {
    selNodeDelete (&pTyDev->selWakeupList, (SEL_WAKEUP_NODE *)arg);
    }

/*******************************************************************************
*
* selPtyAdd - ptyLib FIOSELECT ioctl
*
* This is located here to keep ptyLib from dragging in selectLib.
*/

LOCAL void selPtyAdd 
    (
    PSEUDO_DEV *pPtyDev,
    int arg 
    )
    {
    selNodeAdd (&pPtyDev->masterSelWakeupList, (SEL_WAKEUP_NODE *) arg);

    if ((selWakeupType ((SEL_WAKEUP_NODE *) arg) == SELREAD) &&
		  rngNBytes (pPtyDev->tyDev.wrtBuf) > 0)
		/* reversed read criteria for master */
	{
	selWakeup ((SEL_WAKEUP_NODE *) arg);
	}

    if ((selWakeupType ((SEL_WAKEUP_NODE *) arg) == SELWRITE) &&
		 rngFreeBytes (pPtyDev->tyDev.rdBuf) > 0 )
		/* reversed write criteria for master */
	{
	selWakeup ((SEL_WAKEUP_NODE *) arg);
	}
    }

/*******************************************************************************
*
* selPtyDelete - ptyLib FIOUNSELECT ioctl
*
* This is located here to keep ptyLib from dragging in selectLib.
*/

LOCAL void selPtyDelete
    (
    PSEUDO_DEV *pPtyDev,
    int arg 
    )
    {
    selNodeDelete (&pPtyDev->masterSelWakeupList, (SEL_WAKEUP_NODE *)arg);
    }
