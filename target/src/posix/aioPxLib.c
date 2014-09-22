/* aioPxLib.c - asynchronous I/O (AIO) library (POSIX) */

/* Copyright 1984-2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01k,17jul00,jgn  merge DOT-4 pthreads changes
01j,26sep01,jgn  disable manual generation for aio_fsync (SPR #34282)
01i,04feb99,vcm  adding errno code for aio_return()
01h,15jan95,rhp  fix typo.
            jdi  doc cleanup.
01g,08apr94,kdl  changed aio_show() reference to aioShow().
01f,01feb94,dvs  documentation changes.
01e,26jan94,kdl	 commented out aio_fsync().
01d,21jan94,kdl	 minor doc cleanup; made driver-related routines NOMANUAL.
01c,12jan94,kdl	 changed aioInit() to aioPxLibInit(); added include of 
		 private/schedP.h; general cleanup.
01b,06dec93,dvs  minor configuration related changes; changed S_aioLib* to 
		 S_aioPxLib*
01a,04apr93,elh  written.
*/

/* 
DESCRIPTION
This library implements asynchronous I/O (AIO) according to the definition 
given by the POSIX standard 1003.1b (formerly 1003.4, Draft 14).  AIO 
provides the ability to overlap application processing and I/O operations 
initiated by the application.  With AIO, a task can perform I/O simultaneously 
to a single file multiple times or to multiple files.

After an AIO operation has been initiated, the AIO proceeds in 
logical parallel with the processing done by the application.
The effect of issuing an asynchronous I/O request is as if a separate
thread of execution were performing the requested I/O.
 

AIO LIBRARY
The AIO library is initialized by calling aioPxLibInit(), which
should be called once (typically at system start-up) after the I/O system 
has already been initialized.


AIO COMMANDS 
The file to be accessed asynchronously is opened via the standard open 
call.  Open returns a file descriptor which is used in subsequent AIO 
calls.
 
The caller initiates asynchronous I/O via one of the following routines: 
.IP aio_read() 15
initiates an asynchronous read
.IP aio_write()
initiates an asynchronous write
.IP lio_listio()
initiates a list of asynchronous I/O requests
.LP
Each of these routines has a return value and error value 
associated with it; however, these values indicate only whether the AIO 
request was successfully submitted (queued), not the ultimate success or
failure of the AIO operation itself.  

There are separate return and error values associated with the success or
failure of the AIO operation itself.  The error status can be retrieved
using aio_error(); however, until the AIO operation completes, the error
status will be EINPROGRESS.  After the AIO operation completes, the return
status can be retrieved with aio_return().

The aio_cancel() call cancels a previously submitted AIO request.  The
aio_suspend() call waits for an AIO operation to complete.

Finally, the aioShow() call (not a standard POSIX function) displays
outstanding AIO requests.


AIO CONTROL BLOCK
Each of the calls described above takes an AIO control block (`aiocb') as
an argument.  The calling routine must allocate space for the `aiocb', and
this space must remain available for the duration of the AIO operation.
(Thus the `aiocb' must not be created on the task's stack unless the
calling routine will not return until after the AIO operation is complete
and aio_return() has been called.)  Each `aiocb' describes a single AIO
operation.  Therefore, simultaneous asynchronous I/O operations using the
same `aiocb' are not valid and produce undefined results.

The `aiocb' structure and the data buffers referenced by it are used 
by the system to perform the AIO request.  Therefore, once the `aiocb' has
been submitted to the system, the application must not modify the `aiocb' 
structure until after a subsequent call to aio_return().  The aio_return() 
call retrieves the previously submitted AIO data structures from the system.
After the aio_return() call, the calling application can modify the
`aiocb', free the memory it occupies, or reuse it for another AIO call. 

As a result, if space for the `aiocb' is allocated off the stack 
the task should not be deleted (or complete running) until the `aiocb' 
has been retrieved from the system via an aio_return().

The `aiocb' is defined in aio.h.  It has the following elements:
.CS 
	struct
    	    {
    	    int                 aio_fildes;  
    	    off_t               aio_offset; 
    	    volatile void *     aio_buf; 
    	    size_t              aio_nbytes;
    	    int                 aio_reqprio;
    	    struct sigevent     aio_sigevent;
    	    int                 aio_lio_opcode;
    	    AIO_SYS             aio_sys; 
    	    } aiocb
.CE 
.IP `aio_fildes'
file descriptor for I/O.
.IP `aio_offset'
offset from the beginning of the file where the
AIO takes place.  Note that performing AIO on the file does not 
cause the offset location to automatically increase as in read and write;
the caller must therefore keep track of the location of reads and writes made 
to the file (see POSIX COMPLIANCE below). 
.IP `aio_buf'
address of the buffer from/to which AIO is requested.
.IP `aio_nbytes'
number of bytes to read or write.
.IP `aio_reqprio'
amount by which to lower the priority of an AIO request.  Each
AIO request is assigned a priority; this priority, based on the calling
task's priority, indicates the desired order of execution relative to
other AIO requests for the file.  The `aio_reqprio' member allows the
caller to lower (but not raise) the AIO operation priority by the
specified value.  Valid values for `aio_reqprio' are in the range of zero
through AIO_PRIO_DELTA_MAX.  If the value specified by `aio_req_prio'
results in a priority lower than the lowest possible task priority, the
lowest valid task priority is used.
.IP `aio_sigevent'
(optional) if nonzero, the signal to return on completion of an operation.
.IP `aio_lio_opcode'
operation to be performed by a lio_listio() call; valid entries include
LIO_READ, LIO_WRITE, and LIO_NOP.  
.IP `aio_sys'
a Wind River Systems addition to the `aiocb' structure; it is
used internally by the system and must not be modified by the user.
.LP

EXAMPLES
A writer could be implemented as follows:
.CS
    if ((pAioWrite = calloc (1, sizeof (struct aiocb))) == NULL)
        {
        printf ("calloc failed\en");
        return (ERROR);
        }

    pAioWrite->aio_fildes = fd;
    pAioWrite->aio_buf = buffer;
    pAioWrite->aio_offset = 0;
    strcpy (pAioWrite->aio_buf, "test string");
    pAioWrite->aio_nbytes  = strlen ("test string");
    pAioWrite->aio_sigevent.sigev_notify = SIGEV_NONE;

    aio_write (pAioWrite);

    /@  .
	.

	do other work
	.
	.
    @/

    /@ now wait until I/O finishes @/

    while (aio_error (pAioWrite) == EINPROGRESS)
        taskDelay (1);

    aio_return (pAioWrite);
    free (pAioWrite);
.CE
   
A reader could be implemented as follows:

.CS
    /@ initialize signal handler @/
 
    action1.sa_sigaction = sigHandler;
    action1.sa_flags   = SA_SIGINFO;
    sigemptyset(&action1.sa_mask);
    sigaction (TEST_RT_SIG1, &action1, NULL);

    if ((pAioRead = calloc (1, sizeof (struct aiocb))) == NULL)
        {
        printf ("calloc failed\en");
        return (ERROR);
        }
 
    pAioRead->aio_fildes = fd;
    pAioRead->aio_buf = buffer;
    pAioRead->aio_nbytes = BUF_SIZE;
    pAioRead->aio_sigevent.sigev_signo = TEST_RT_SIG1;
    pAioRead->aio_sigevent.sigev_notify = SIGEV_SIGNAL;
    pAioRead->aio_sigevent.sigev_value.sival_ptr = (void *)pAioRead;
 
    aio_read (pAioRead);

    /@
        .
        .

        do other work 
	.
	.
    @/
.CE

The signal handler might look like the following:

.CS
void sigHandler
    (
    int                 sig,
    struct siginfo      info,
    void *              pContext
    )
    {
    struct aiocb *      pAioDone;
    pAioDone = (struct aiocb *) info.si_value.sival_ptr;
    aio_return (pAioDone);
    free (pAioDone);
    }
.CE

POSIX COMPLIANCE
Currently VxWorks does not support the O_APPEND flag in the open call.  
Therefore, the user must keep track of the offset in the file that 
the asynchronous writes occur (as in the case of reads).  The `aio_offset'
field is used to specify that file position.

In addition, VxWorks does not currently support synchronized I/O.

INCLUDE FILES: aio.h

SEE ALSO: POSIX 1003.1b document


INTERNAL

As it currently stands, aioLib could be better integrated into the I/O 
system.  aioDrvTable and aioFdTable could be integrated with drvTable and 
fdTable respectively.  Also, the hook routines aioFdNew and aioFdFree
could be eliminated.  However we choose not to do this (for now) to
avoid increasing the size of the I/O structures (thus causing the user
to have to recompile their applications).  This should be changed  
later.

Although the interface between aioLib and AIO drivers has been defined, 
currently there is only support to use it with the AIO system driver.  Work
needs to be done (as well as modifications to the xxxFsLib and scsiLib) 
before it can be implemented in our block devices.
*/ 

/* includes */ 

#include "vxWorks.h" 
#include "aio.h"
#include "private/iosLibP.h" 
#include "string.h"
#include "qFifoLib.h"
#include "timers.h"
#include "intLib.h"
#include "taskLib.h"
#include "stdlib.h"
#include "tickLib.h"
#include "private/mutexPxLibP.h"
#include "private/schedP.h" 
#define __PTHREAD_SRC
#include "pthread.h"

/* defines */

#define AIO_LOCK()		(semTake (&aioSem, WAIT_FOREVER))
#define AIO_UNLOCK()		(semGive (&aioSem))

/* typedefs */

typedef struct 					/* each parameter */
    {
    AIO_DEV * 	pDev;				/* device */
    int		retVal;				/* return value */
    int		errorVal;			/* error value */
    FUNCPTR	syncRtn;			/* sync routine */
    } ARGS;


/* globals */

FUNCPTR			aioPrintRtn = NULL;

/* locals */

LOCAL AIO_DRV_ENTRY *	aioDrvTable = NULL;		/* driver table */
LOCAL AIO_FD_ENTRY *	aioFdTable = NULL;		/* fd table */
LOCAL AIO_CLUST *	aioClustTable = NULL;		/* cluster table */
LOCAL int		aioClustMax;			/* max clusters */
LOCAL SEMAPHORE		aioSem;				/* AIO semaphore */


/* forward declarations */

LOCAL STATUS  aioSubmit (struct aiocb * pAiocb, int op, AIO_CLUST * pClust);
LOCAL int     aioListEach (struct aiocb * list[], int num, FUNCPTR routine, 
			   int arg1, int arg2);
LOCAL BOOL    aioListSubmit (struct aiocb * pAiocb, AIO_CLUST * pClust, 
			     BOOL * pFailed);
LOCAL BOOL    aioListClust (struct aiocb * pAiocb, AIO_CLUST * pClust, 
			    int bogus);
LOCAL BOOL    aioListError (struct aiocb * pAiocb, int * pError, int bogus);
LOCAL BOOL    aioListWait (struct aiocb * pAiocb, AIO_WAIT_ID * pAioId, 
			    int bogus);
LOCAL BOOL    aioListUnwait (struct aiocb * pAiocb, AIO_WAIT_ID * pAioId, 
			     int bogus);
LOCAL STATUS  aioSyncNode (DL_NODE * pFdNode, ARGS * pArgs);
LOCAL void    aioClustRet (struct aiocb * pAiocb, AIO_CLUST * pClust);

LOCAL AIO_CLUST *    aioClustGet (void);

/* I/O hook routines */

LOCAL void    aioFdFree (int fd);		
LOCAL void    aioFdNew (int fd);

extern BOOL		iosLibInitialized;


/*******************************************************************************
* 
* aioPxLibInit - initialize the asynchronous I/O (AIO) library
*
* This routine initializes the AIO library.  It should be called only
* once after the I/O system has been initialized.  <lioMax> specifies the 
* maximum number of outstanding lio_listio() calls at one time.  If <lioMax>
* is zero, the default value of AIO_CLUST_MAX is used.
*
* RETURNS: OK if successful, otherwise ERROR.
*
* ERRNO: S_aioPxLib_IOS_NOT_INITIALIZED
*
* INTERNAL:  This library relies on the fact that the IOS library 
* has been installed because it uses the variables maxDrivers and maxFiles. 
* This can be better integrated into the I/O system but for now we do this 
* to avoid adding entries to the drvTable and fdTable.
*/

STATUS aioPxLibInit 
    (
    int			lioMax 			/* max outstanding lio calls */
    )
    {
    int			ix;			/* index */

    if (!iosLibInitialized)
	{
	errno = S_aioPxLib_IOS_NOT_INITIALIZED;
	return (ERROR);				/* ios not initialized */
	}

    if (aioDrvTable != NULL)
	return (OK);				/* already initialized */


    /* Allocate memory for the AIO driver table */

    if ((aioDrvTable = calloc (maxDrivers, sizeof (AIO_DRV_ENTRY))) == NULL)
	return (ERROR);


    /* Allocate memory for the AIO fd table */

    if ((aioFdTable = calloc (maxFiles, sizeof (AIO_FD_ENTRY))) == NULL)
	return (ERROR);

    for (ix = 0; ix < maxFiles; ix++)
	{
	AIO_FD_ENTRY *	pEntry = &aioFdTable [ix];

        pEntry->pFdEntry = &fdTable [ix];
        semMInit (&pEntry->ioQSem, SEM_DELETE_SAFE | SEM_Q_PRIORITY);
	}

    /* Allocate and initialize the cluster table */

    aioClustMax = (lioMax == 0) ? AIO_CLUST_MAX : lioMax;

    if ((aioClustTable = calloc (aioClustMax, sizeof (AIO_CLUST))) == NULL)
	return (ERROR);				/* memory problem */ 

    for (ix = 0 ; ix < aioClustMax; ix++)
	aioClustTable [ix].inuse = FALSE;

    /* Initialize semaphore that protects data structures created above */

    semMInit (&aioSem, SEM_DELETE_SAFE | SEM_Q_PRIORITY);


    /* Hook into the ios system */ 

    iosFdNewHookRtn  = aioFdNew;
    iosFdFreeHookRtn = aioFdFree;

    return (OK);
    }


/* POSIX specified routines */

/*******************************************************************************
*
* aio_read - initiate an asynchronous read (POSIX)
*
* This routine asynchronously reads data based on the following parameters
* specified by members of the AIO control structure <pAiocb>.  It reads
* `aio_nbytes' bytes of data from the file `aio_fildes' into the buffer
* `aio_buf'.
*
* The requested operation takes place at the absolute position in the file
* as specified by `aio_offset'.
*
* `aio_reqprio' can be used to lower the priority of the AIO request; if
* this parameter is nonzero, the priority of the AIO request is
* `aio_reqprio' lower than the calling task priority.
*
* The call returns when the read request has been initiated or queued to the
* device.  aio_error() can be used to determine the error status and of the
* AIO operation.  On completion, aio_return() can be used to determine the
* return status.
*
* `aio_sigevent' defines the signal to be generated on completion 
* of the read request.  If this value is zero, no signal is generated. 
*
* RETURNS: OK if the read queued successfully, otherwise ERROR.
*
* ERRNO: EBADF, EINVAL
*
* INCLUDE FILES: aio.h
*
* SEE ALSO: aio_error(), aio_return(), read()
*
*/

int aio_read 
    (
    struct aiocb * 	pAiocb			/* AIO control block */
    )

    {
    return (aioSubmit (pAiocb, IO_READ, NULL));
    }
    
/*******************************************************************************
*
* aio_write - initiate an asynchronous write (POSIX)
*
* This routine asynchronously writes data based on the following parameters
* specified by members of the AIO control structure <pAiocb>.  It writes
* `aio_nbytes' of data to the file `aio_fildes' from the buffer `aio_buf'.
*
* The requested operation takes place at the absolute position in the file
* as specified by `aio_offset'.
*
* `aio_reqprio' can be used to lower the priority of the AIO request; if
* this parameter is nonzero, the priority of the AIO request is
* `aio_reqprio' lower than the calling task priority.
*
* The call returns when the write request has been initiated or queued to
* the device.  aio_error() can be used to determine the error status and of
* the AIO operation.  On completion, aio_return() can be used to determine
* the return status.
*
* `aio_sigevent' defines the signal to be generated on completion 
* of the write request.  If this value is zero, no signal is generated. 
*
* RETURNS: OK if write queued successfully, otherwise ERROR.
*
* ERRNO: EBADF, EINVAL
*
* INCLUDE FILES: aio.h
*
* SEE ALSO: aio_error(), aio_return(), write()
*
*/

int aio_write 
    (
    struct aiocb * 	pAiocb 			/* AIO control block */
    )

    {
    return (aioSubmit (pAiocb, IO_WRITE, NULL));
    }

/*******************************************************************************
*
* lio_listio - initiate a list of asynchronous I/O requests (POSIX)
*
* This routine submits a number of I/O operations (up to AIO_LISTIO_MAX)
* to be performed asynchronously.  <list> is a pointer to an array of 
* `aiocb' structures that specify the AIO operations to be performed.  
* The array is of size <nEnt>.   
*
* The `aio_lio_opcode' field of the `aiocb' structure specifies the AIO
* operation to be performed.  Valid entries include LIO_READ, LIO_WRITE, and
* LIO_NOP.  LIO_READ corresponds to a call to aio_read(), LIO_WRITE
* corresponds to a call to aio_write(), and LIO_NOP is ignored.
*
* The <mode> argument can be either LIO_WAIT or LIO_NOWAIT.  If <mode> is 
* LIO_WAIT, lio_listio() does not return until all the AIO operations 
* complete and the <pSig> argument is ignored.  If <mode> is LIO_NOWAIT, the 
* lio_listio() returns as soon as the operations are queued.  In this case, 
* if <pSig> is not NULL and the signal number indicated by
* `pSig->sigev_signo' is not zero, the signal `pSig->sigev_signo' is
* delivered when all requests have completed.
*
* RETURNS: OK if requests queued successfully, otherwise ERROR.
*
* ERRNO: EINVAL, EAGAIN, EIO
*
* INCLUDE FILES: aio.h
*
* SEE ALSO: aio_read(), aio_write(), aio_error(), aio_return().
*/

int lio_listio
    (
    int 		mode,			/* LIO_WAIT or LIO_NOWAIT */
    struct aiocb * 	list[],			/* list of operations */
    int 		nEnt,			/* size of list */
    struct sigevent * 	pSig			/* signal on completion */
    )
    {
    AIO_CLUST *		pClust;			/* cluster */
    int			errorVal = 0;		/* error value */
    STATUS		retVal = OK;		/* return value */
    BOOL 		submitFailed = FALSE;	/* submit failed */

    
    /* Validate parameters */

    if (((mode != LIO_WAIT) && (mode != LIO_NOWAIT)) || (nEnt > AIO_LISTIO_MAX))
	{
	errno = EINVAL;
	return (ERROR);				/* invalid parameter */
	}

    if ((pSig != NULL) && (pSig->sigev_notify == SIGEV_SIGNAL) &&
	((pSig->sigev_signo < 1) || (pSig->sigev_signo > _NSIGS)))
	{
	errno = EINVAL;
 	return (ERROR);				/* safety check */
	}

    /* Obtain a free cluster */

    if ((pClust = aioClustGet ()) == NULL)
	return (ERROR); 			/* no clusters available  */

    if ((mode == LIO_NOWAIT) && (pSig != NULL) &&
	(pSig->sigev_notify == SIGEV_SIGNAL))
	{
	sigPendInit (&pClust->sigpend);
	pClust->sigpend.sigp_info.si_signo = pSig->sigev_signo;
	pClust->sigpend.sigp_info.si_value = pSig->sigev_value;
	pClust->sigpend.sigp_info.si_code = SI_ASYNCIO;
	}
    else
	pClust->sigpend.sigp_info.si_signo = 0;

    /* Link up cluster and submit AIO requests */

    aioListEach (list, nEnt, aioListClust, (int) pClust, 0);
    aioListEach (list, nEnt, aioListSubmit, (int) pClust, (int) &submitFailed);

    if (mode == LIO_NOWAIT)	
	{
    	if (submitFailed)
	    {
	    errno = EIO;
	    return (ERROR);			/* submit failed */
	    }
	else
	    return (OK);				/* don't wait for I/O */
	}

    /* Wait for I/O completion */

    mutex_lock (&pClust->lock);

    retVal = OK;

    while ((pClust->refCnt > 0) && (retVal == OK)) 
	retVal = cond_timedwait (&pClust->wake, &pClust->lock, NULL);

    mutex_unlock (&pClust->lock);

    if (retVal != OK)
	{
	errno = retVal;
    	return (ERROR);				/* signal */
	}

    /* Check for I/O errors */

    aioListEach (list, nEnt, aioListError, (int) &errorVal, 0);

    if (errorVal)
       {
       errno = EIO;
       return (ERROR);				/* an AIO request failed */
       }

    return (OK);				/* I/O completed successfully */
    }

/*******************************************************************************
*
* aio_suspend - wait for asynchronous I/O request(s)  (POSIX)
*
* This routine suspends the caller until one of the following occurs:
* .iP
* at least one of the previously submitted asynchronous I/O operations
* referenced by <list> has completed,
* .iP
* a signal interrupts the function, or
* .iP
* the time interval specified by <timeout> has passed
* (if <timeout> is not NULL).
* .LP
* 
* RETURNS: OK if an AIO request completes, otherwise ERROR.
*
* ERRNO: EAGAIN, EINTR  
*
* INCLUDE FILES: aio.h
*
* INTERNAL:  We make the task safe from deletion so the (stack) variables 
* that were added to the wait list don't get hosed.  This could have 
* been implemented with taskDeleteHooks.
*/

int aio_suspend 
    (
    const struct aiocb * 	list[],		/* AIO requests */
    int 			nEnt,		/* number of requests */
    const struct timespec * 	timeout		/* wait timeout */
    )

    {
    AIO_WAIT_ID			aioId;		/* AIO Id */
    int				numDone;	/* num completed */
    STATUS			retVal = OK;	/* return value */
    int				savtype;	/* saved cancellation type */

    /* Initialize the wait structure */

    bzero ((caddr_t) &aioId, sizeof (AIO_WAIT_ID));
    aioId.done   = FALSE;
    
    /* Link into pthreads support code */

    if (_func_pthread_setcanceltype != NULL)
	{
	_func_pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &savtype);
	}
    mutex_init (&aioId.lock, NULL);
    cond_init (&aioId.wake, NULL);

    taskSafe ();				/* TASK_SAFE */
    
    /* Submit wait requests */

    numDone = aioListEach ((struct aiocb **) list, nEnt, aioListWait, 
			   (int) &aioId, 0);

    if (numDone == nEnt)			
	{
	
	/* All wait requests got submitted - wait for one to complete */

        mutex_lock (&aioId.lock);

	retVal = OK;

    	while ((!aioId.done) && (retVal == OK))
	    retVal = cond_timedwait (&aioId.wake, &aioId.lock, timeout); 

        mutex_unlock (&aioId.lock);
	}
    else					
	{
	/* Wait requests didn't all get submited */

	if (errno != AIO_COMPLETED)		/* not completed successfully */
	    retVal = errno;

	numDone = nEnt;				/* cancel to the one failed */
   	}

    /* Cancel outstanding wait requests */

    aioListEach ((struct aiocb **) list, numDone, aioListUnwait, (int) &aioId, 
		 0);

    taskUnsafe ();				/* TASK_UNSAFE */

    /* Link into pthreads support code */

    if (_func_pthread_setcanceltype != NULL)
        {
        _func_pthread_setcanceltype(savtype, NULL);
        }

    if (retVal != OK)
	{
	errno = retVal;
	return (ERROR);				/* error occurred */
	}

    return (OK);
    }

/******************************************************************************* *
*
* aio_cancel - cancel an asynchronous I/O request (POSIX)
*
* This routine attempts to cancel one or more asynchronous I/O request(s) 
* currently outstanding against the file descriptor <fildes>.  <pAiocb> 
* points to the asynchronous I/O control block for a particular request to
* be cancelled.  If <pAiocb> is NULL, all outstanding cancelable 
* asynchronous I/O requests associated with <fildes> are cancelled.
* 
* Normal signal delivery occurs for AIO operations that are successfully 
* cancelled.  If there are requests that cannot be cancelled, then the normal 
* asynchronous completion process takes place for those requests when they 
* complete.
*
* Operations that are cancelled successfully have a return status of -1
* and an error status of ECANCELED.
*
* RETURNS:
*  AIO_CANCELED if requested operations were cancelled,
*  AIO_NOTCANCELED if at least one operation could not be cancelled,
*  AIO_ALLDONE if all operations have already completed, or
*  ERROR if an error occurred.
*
* ERRNO: EBADF
*
* INCLUDE FILES: aio.h
*
* SEE ALSO: aio_return(), aio_error()
*/

int aio_cancel
    (
    int			fildes,		/* file descriptor */
    struct aiocb * 	pAiocb		/* AIO control block */
    )

    {
    FUNCPTR		ioctlRtn;	/* ioctl routine */
    AIO_FD_ENTRY *	pEntry;		/* fd entry */
    DL_NODE *		pNode;		/* fd node */
    AIO_SYS *		pReq;		/* AIO request */
    int			value;		/* driver parameter */ 
    int			cancelled = 0;	/* num cancelled */
    int			retVal = OK;	/* return value */

    if ((pEntry = aioEntryFind (fildes)) == NULL)
	return (ERROR);

    ioctlRtn = aioDrvTable [pEntry->drv_num].ioctl;
    value = pEntry->drv_value;

    if (pAiocb != NULL)				/* cancel a single request */
	{
	if (pAiocb->aio_state == AIO_COMPLETED)
	    return (AIO_ALLDONE);

        if (((* ioctlRtn) (value, FAIO_CANCEL, pAiocb)) == OK)
	    return (AIO_CANCELED);

	return (AIO_NOTCANCELED);
	}


    /* Cancel all requests on the file descriptor */

    semTake (&pEntry->ioQSem, WAIT_FOREVER);

    for (pNode = DLL_FIRST (&pEntry->ioQ); pNode != NULL; 
	 pNode = DLL_NEXT (pNode))
	{
	pReq = FD_NODE_TO_SYS (pNode);

	if (pReq->state != AIO_COMPLETED)
	    {
	    if ((* ioctlRtn) (value, FAIO_CANCEL, pReq->pAiocb) == ERROR)
		retVal = AIO_NOTCANCELED;
	    else
	    	cancelled ++;
	    }
	}

    semGive (&pEntry->ioQSem);

    if (retVal == AIO_NOTCANCELED)
	return (AIO_NOTCANCELED);		/* couldn't cancel */

    if (cancelled)
	return (AIO_CANCELED);			/* cancelled */

    return (AIO_ALLDONE);
    }

#if FALSE	/* not ready for primetime - kdl 1/26/94 */
/*******************************************************************************
*
* aio_fsync - asynchronous file synchronization (POSIX)
*
* This routine asynchronously forces all I/O operations associated 
* with the file, indicated by `aio_fildes', queued at the time aio_fsync() is 
* called to the synchronized I/O completion state.  aio_fsync() returns 
* when the synchronization request has be initiated or queued to the file or 
* device.
* 
* The value of <op> is ignored.  It currently has no meaning in VxWorks.
*
* If the call fails, the outstanding I/O operations are not guaranteed to
* have completed.  If it succeeds, only the I/O that was queued at the time
* of the call is guaranteed to the relevant completion state.
* 
* The `aio_sigevent' member of the <pAiocb> defines an optional signal to be 
* generated on completion of aio_fsync().
*
* RETURNS: OK if queued successfully, otherwise ERROR.
*
* ERRNO: EINVAL, EBADF
*
* INCLUDE FILES: aio.h
*
* SEE ALSO: aio_error(), aio_return() 
*
* INTERNAL
* Asynchronously synchronize a file??????  Excuse me?  Sounds like an 
* oxymoron (or at least moronic).  At any rate this is hard to implement
* correctly. 
*
* NOMANUAL
*/

int aio_fsync
    (
    int			op,			/* operation */
    struct aiocb * 	pAiocb			/* AIO control block */
    )
    {
    FUNCPTR		ioctlRtn;		/* ioctl routine */
    AIO_FD_ENTRY *	pEntry;			/* fd entry */

    if (pAiocb == NULL)
	return (OK);				/* not sure what POSIX means */

    if ((pEntry = aioEntryFind (pAiocb->aio_fildes)) == NULL)
	return (ERROR);

    /* Submit sync request */

    if (aioSubmit (pAiocb, IO_SYNC, NULL) == ERROR)
	return (ERROR);
       
    /* Push request to head of work queue */ 

    ioctlRtn = aioDrvTable [pEntry->drv_num].ioctl;
    (* ioctlRtn) (pEntry->drv_value, FAIO_PUSH, pAiocb);    

    return (OK);
    }
#endif /* FALSE */

/*******************************************************************************
*
* aio_error - retrieve error status of asynchronous I/O operation (POSIX)
*
* This routine returns the error status associated with the I/O operation 
* specified by <pAiocb>.  If the operation is not yet completed, the error 
* status will be EINPROGRESS.
*
* RETURNS:
*  EINPROGRESS if the AIO operation has not yet completed,
*  OK if the AIO operation completed successfully,
*  the error status if the AIO operation failed, 
*  otherwise ERROR.
*
* ERRNO: EINVAL
*
* INCLUDE FILES: aio.h
*
*/

int aio_error 
    (
    const struct aiocb * pAiocb			/* AIO control block */
    )
    {
    int 		state; 			/* AIO request state */

    if (pAiocb != NULL)
	{
	state = pAiocb->aio_state;

	if (state == AIO_COMPLETED)
	    return (pAiocb->aio_errorVal);	/* return error value */

    	if ((state >= AIO_READY) && (state <= AIO_RUNNING))
	    return (EINPROGRESS);		/* still running */
	}

    errno = EINVAL;
    return (ERROR);				/* unsure about pAiocb */
    }

/*******************************************************************************
*
* aio_return - retrieve return status of asynchronous I/O operation (POSIX)
*
* This routine returns the return status associated with the I/O operation
* specified by <pAiocb>.  The return status for an AIO operation is the 
* value that would be returned by the corresponding read(), write(), or 
* fsync() call.  aio_return() may be called only after the AIO operation
* has completed (aio_error() returns a valid error code--not EINPROGRESS).
* Furthermore, aio_return() may be called only once; subsequent 
* calls will fail.
*
* RETURNS: The return status of the completed AIO request, or ERROR. 
*
* ERRNO:  EINVAL, EINPROGRESS
*
* INCLUDE FILES: aio.h
*/

size_t aio_return 
    (
    struct aiocb * 	pAiocb			/* AIO control block */
    )
    {
    int			retVal;			/* return value */
    AIO_FD_ENTRY *	pEntry;			/* fd entry */
    FAST AIO_SYS *	pReq; 			/* AIO request */

    if (pAiocb == NULL)
	{
	errno = EINVAL;
 	return (ERROR);				/* invalid parameter */
	}

    if ((pEntry = aioEntryFind (pAiocb->aio_fildes)) == NULL)
	return (ERROR);

    pReq = &pAiocb->aio_sys;

    if (pReq->state != AIO_COMPLETED)
	{
    	errno = EINPROGRESS;
	return (ERROR);				/* not done yet */
	}

    /* Remove the aiocb from the fd list if the aiocb was 
     * submitted correctly. 
     */

    semTake (&pEntry->ioQSem, WAIT_FOREVER);

    if (pReq->pAiocb != NULL)	
        dllRemove (&pEntry->ioQ, &pReq->fdNode);

    semGive (&pEntry->ioQSem);

    /* Wait here until all call to suspend (on this aiocb) 
     * wake up and are deleted from wait list. aioDone wakes up 
     * each individual suspend.  
     * aio_suspend() removes them from the wait list.
     */

    mutex_lock (&pReq->lock);	
    while (lstCount (&pReq->wait) > 0)
	cond_timedwait (&pReq->wake, &pReq->lock, NULL);
    mutex_unlock (&pReq->lock);

    sigPendDestroy (&pReq->sigpend);
    retVal = pAiocb->aio_retVal;
    bzero ((char *) pReq, sizeof (AIO_SYS));

    return (retVal);
    }

/******************************************************************************
*
* aioSubmit - submit an AIO request 
* 
* This routine fills in the AIO_SYS portion, of the AIO control block (pAiocb)
* then passes the request to the AIO driver by calling the driver supplied 
* insert routine.
*
* RETURNS: OK if successful, ERROR otherwise
*
* ERRNO: EINVAL
* 
* INTERNAL
* The state leaving this routine is AIO_READY.  The driver provided insert 
* routine must mark the state as AIO_QUEUED after inserting it onto the 
* workQ.
*/

LOCAL STATUS aioSubmit
    (
    struct aiocb *	pAiocb,			/* AIO control block */
    int			op,			/* operation */
    AIO_CLUST *		pClust			/* cluster */
    )
    {
    FUNCPTR		insertRtn;		/* insert routine */
    AIO_FD_ENTRY *	pEntry;			/* fd entry */
    AIO_SYS *		pReq;			/* AIO request */
    int			prio = 0;		/* priority */

    if (pAiocb == NULL)
	{
	errno = EINVAL;
 	return (ERROR);				/* safety check */
	}

    if ((pAiocb->aio_sigevent.sigev_notify == SIGEV_SIGNAL) &&
	((pAiocb->aio_sigevent.sigev_signo < 1) ||
	 (pAiocb->aio_sigevent.sigev_signo > _NSIGS)))
	{
	errno = EINVAL;
 	return (ERROR);				/* safety check */
	}

    pReq = &pAiocb->aio_sys;			/* clear AIO_SYS */
    bzero ((char *) pReq, sizeof (AIO_SYS));	

    if ((pEntry = aioEntryFind (pAiocb->aio_fildes)) == NULL)
	return (ERROR);

    /* #ifdef _POSIX_PRIORITIZED_IO (says POSIX) but implementing 
     * this here is easy.  Let the driver decide what to do with it.
     */

    if ((pAiocb->aio_reqprio < 0) || 
	(pAiocb->aio_reqprio > AIO_PRIO_DELTA_MAX))
	{
	errno = EINVAL;
	return (ERROR);				/* invalid priority */
	}
    					
    /* Determine the effective priority - use VxWorks numbering */

    taskPriorityGet (taskIdSelf (), &prio);	
    prio = min ((pAiocb->aio_reqprio + prio), VXWORKS_LOW_PRI);

    /* initialize the AIO control block */

    pReq->ioNode.op	 = op;			/* fill in the ioNode */
    pReq->ioNode.prio	 = prio;
    pReq->ioNode.doneRtn = aioDone;		
    pReq->ioNode.taskId  = taskIdSelf ();
    pReq->ioNode.errorVal= EINPROGRESS;	
    pReq->ioNode.retVal  = OK;

    pReq->pAiocb         = pAiocb;		/* back pointer */
    pReq->pClust	 = pClust;		/* cluster */

    lstInit (&pReq->wait);			/* initailize wait */
    mutex_init (&pReq->lock, NULL);
    cond_init (&pReq->wake, NULL);

    pReq->state 	 = AIO_READY;		/* mark it ready */

    semTake (&pEntry->ioQSem, WAIT_FOREVER);

    if (!pEntry->ioQActive)
	{
    	semGive (&pEntry->ioQSem);
	errno = EBADF;
	return (ERROR);				/* was deleted */
	}

    dllAdd (&pEntry->ioQ, &pReq->fdNode);	/* add it to fd list */

    semGive (&pEntry->ioQSem);

    insertRtn = aioDrvTable [pEntry->drv_num].insert;
    return ((* insertRtn) (pEntry->drv_value, pAiocb, prio));
    }

/*****************************************************************************
*
* aioListEach - call routine for each element in list
*
* This routine calls the passed routine <routine> with arguments <arg1>
* and <arg2> for each element in <list> which is non null.  <list>
* has <num> elements.  aioListEach calls the passed function <routine> 
* until <routine> returns a value of FALSE.  
*
* RETURNS:  the index of in <list> which caused aioList() to stop (return FALSE)
* 	    or <num> if all members of the list element returned TRUE.
*/

LOCAL int aioListEach
    (
    struct aiocb * 	list[],			/* list of operations */
    int			num,			/* size of list */
    FUNCPTR		routine,		/* routine to call */
    int			arg1,			/* argument */
    int			arg2			/* argument */
    )
    {
    int 		ix;			/* index */

    for (ix = 0; ix < num; ix++)
	{
	if (list [ix] != NULL) 
	    {
	    if (((* routine) (list [ix], arg1, arg2)) == FALSE)
		return (ix);
	    }
	} 
    return (num);
    } 

/*****************************************************************************
*
* aioListSubmit - submit a list of AIO requests (aioListEach routine)
*
* This routine is called from lio_listio() to submit the list of AIO
* requests.  aioListSubmit submits each element of the list, even of
* on fails.
* 
* RETURNS: TRUE
*/

LOCAL BOOL aioListSubmit
    (
    struct aiocb *	pAiocb, 		/* AIO control block */
    AIO_CLUST * 	pClust,			/* cluster */
    BOOL *		pFailed 		/* set if submit failed */
    )
    {
    int			op = pAiocb->aio_lio_opcode;

    switch (op)
	{
	case LIO_READ  : op = IO_READ; break;
	case LIO_WRITE : op = IO_WRITE; break;
	case LIO_NOP   : return (TRUE); 	/* NOP is dumb, but valid */
	}

    if (aioSubmit (pAiocb, op, pClust) == ERROR)
	{
	AIO_DONE_SET (pAiocb, ERROR, errno);
	*pFailed = TRUE;			/* notify caller failure */ 
	aioClustRet (pAiocb, pClust);
	}

    return (TRUE);
    }

/*****************************************************************************
*
* aioListClust - cluster AIO requests together (aioListEach routine)
*
* This routine is called from lio_listio() to cluster the list of AIO
* requests together.
* 
* RETURNS: TRUE
*/

LOCAL BOOL aioListClust
    (
    struct aiocb *	pAiocb, 		/* AIO control block */
    AIO_CLUST *		pClust,			/* cluster */
    int			bogus1			/* not used */
    )
    {
    if (pAiocb->aio_lio_opcode != LIO_NOP)
	pClust->refCnt++;			/* count number of aiocbs */

    return (TRUE);
    }

/*****************************************************************************
*
* aioListError - check for errors in list (aioListEach routine)
*
* This routine is called from lio_listio() to check if any of the completed
* AIO requests in the list failed.
*
* RETURNS: FALSE if any request failed, TRUE otherwise.
*/

LOCAL BOOL aioListError 
    (
    struct aiocb *	pAiocb,			/* AIO control block */
    int *		pError,			/* error */
    int			bogus			/* not used */
    )
    {
    if ((pAiocb->aio_lio_opcode != LIO_NOP) && 
	(pAiocb->aio_state == AIO_COMPLETED) && 
	(pAiocb->aio_retVal == ERROR))
	{
	*pError = pAiocb->aio_errorVal;
	return (FALSE);	
	}
    return (TRUE);
    }
    
/*****************************************************************************
*
* aioListWait - add wait request to AIO control block (aioListEach routine).
*
* This routine is called from aio_suspend() to add the <pAioId> to the wait 
* list associated with <pAiocb>.
*
* RETURNS: FALSE if an error occurred or any request had completed
*	   TRUE otherwise
*/

LOCAL BOOL aioListWait
    (
    struct aiocb *	pAiocb, 		/* AIO control block */
    AIO_WAIT_ID *	pAioId,			/* AIO id */
    int			bogus			/* not used */
    )

    {
    FAST AIO_FD_ENTRY *	pEntry;			/* fd entry */
    STATUS		retVal = TRUE;		/* return value */
    AIO_SYS *		pReq   = &pAiocb->aio_sys;

    if ((pEntry = aioEntryFind (pAiocb->aio_fildes)) == NULL)
	return (FALSE);				/* invalid file */

    mutex_lock (&pReq->lock);

    if ((pReq->state >= AIO_READY) && (pReq->state <= AIO_RUNNING))
	lstAdd (&pReq->wait, &pAioId->node);
    else
	{
	errno = (pReq->state == AIO_COMPLETED) ? AIO_COMPLETED : EINVAL;
	retVal = FALSE;
	}

    mutex_unlock (&pReq->lock);
    return (retVal);
    }

/*****************************************************************************
*
* aioListUnwait - delete previous wait request (aioListEach routine).
*
* This routine cancels a previous wait request.  It deletes the <pAioId>
* from the wait list associated with <pAiocb>.
*
* RETURNS: TRUE
*/

LOCAL BOOL aioListUnwait
    (
    struct aiocb *	pAiocb, 		/* AIO control block */
    AIO_WAIT_ID *	pAioId,			/* AIO id */
    int			bogus			/* not used */
    )
    {
    mutex_lock (&pAiocb->aio_sys.lock);

    lstDelete (&pAiocb->aio_sys.wait, &pAioId->node);

    mutex_unlock (&pAiocb->aio_sys.lock);
    return (TRUE);
    }

/*******************************************************************************
* 
* aioClustGet - get a free cluster 
*
* RETURNS: A pointer to a cluster structure or NULL.
*/

LOCAL AIO_CLUST * aioClustGet (void)
    {
    int 		ix;			/* index */
    AIO_CLUST *		pClust;			/* AIO cluster */

    AIO_LOCK ();				/* lock access */

    for (ix = 0; ix < aioClustMax; ix++)
	{
        if (!aioClustTable [ix].inuse)
	    {
	    aioClustTable [ix].inuse = TRUE;
	    break;				/* got free cluster */
	    }
	}

    AIO_UNLOCK ();				/* unlock access */

    if (ix >= aioClustMax)
	{
        errno = EAGAIN;
	return (NULL);				/* none available */
	}

    /* Initialize cluster */

    pClust = &aioClustTable [ix];
    pClust->refCnt = 0;
    mutex_init (&pClust->lock, NULL);
    cond_init  (&pClust->wake, NULL);

    return (pClust);
    }

/*******************************************************************************
* 
* aioDone - AIO completed 
*
* This routine gets called on completion of an AIO operation.  It notifies
* any waiting tasks of the AIO completion.  When aioDone is called the 
* AIO request should not be associated with any driver workQ and the 
* state should have been set to AIO_COMPLETED and the return and error 
* values set.  aioDone shouldnt't be called from interrupt service 
* routine (if so, it gets deffered to excTask).
*
* RETURNS: N/A
*
* NOMANUAL
*/

void aioDone 
    (
    AIO_SYS * 		pReq			/* AIO request */
    )

    {
    AIO_WAIT_ID *	pWaitId;		/* wait request id */
    struct aiocb *	pAiocb = pReq->pAiocb;	/* AIO control block */

    if (INT_CONTEXT ())	
	{
	excJobAdd ((VOIDFUNCPTR) aioDone, (int) pReq, 0, 0, 0, 0, 0);
	return;
	}

    if (aioPrintRtn != NULL)
	(* aioPrintRtn) ("aioDone: aiocb (0x%x) retVal %d error 0x%x\n", 
			 (int) pAiocb, pAiocb->aio_retVal, 
			 pAiocb->aio_errorVal, 0, 0, 0);

    /* Wake tasks waiting in aio_suspend */

    mutex_lock (&pReq->lock);
    						
    for (pWaitId = (AIO_WAIT_ID *) lstFirst (&pReq->wait); (pWaitId != NULL); 
	 pWaitId = (AIO_WAIT_ID *) lstNext (&pWaitId->node))
	{
	mutex_lock (&pWaitId->lock);
	   
	if (!pWaitId->done)
	    pWaitId->done = TRUE;

	cond_signal (&pWaitId->wake);
	mutex_unlock (&pWaitId->lock);
	}

    mutex_unlock (&pReq->lock);

    /* We lock out the task because even though we signaled it
     * we don't want it to run until we marked it completed.
     */
     
    taskLock ();				

    if (pReq->pClust != NULL)
	aioClustRet (pAiocb, pReq->pClust);

    /* Send a caller specified signal */

    if (pAiocb->aio_sigevent.sigev_notify == SIGEV_SIGNAL)
	{
	if (aioPrintRtn != NULL)
	    (* aioPrintRtn) ("aioDone: signal %d\n",
			     pAiocb->aio_sigevent.sigev_signo, 0, 0, 0, 0, 0);

	sigPendInit (&pReq->sigpend);
	pReq->sigpend.sigp_info.si_signo = pAiocb->aio_sigevent.sigev_signo;
	pReq->sigpend.sigp_info.si_code  = SI_ASYNCIO;
	pReq->sigpend.sigp_info.si_value = pAiocb->aio_sigevent.sigev_value;
	sigPendKill (pReq->ioNode.taskId, &pReq->sigpend);
	}

    pReq->state = AIO_COMPLETED;
    taskUnlock ();
    }

/*******************************************************************************
* 
* aioClustRet - return a cluster
*
* This routine frees <pClust> and  signals/wakes up the initiating task if
* <pAiocb> is the last AIO request in the cluster.
*
* RETURN: N/A
*
* NOMANUAL
*/

LOCAL void aioClustRet
    (
    struct aiocb  *	pAiocb,			/* AIO control block */
    AIO_CLUST *		pClust			/* AIO cluster */
    )
    {
    BOOL		returnClust;		/* cluster goes to table */ 
    int			signo;			/* signal number */
    int			taskId;			/* lio_listio() task */

    if (pClust == NULL)
	return; 

    mutex_lock (&pClust->lock);
    returnClust = ((pClust->refCnt > 0) && (--pClust->refCnt == 0));
    mutex_unlock (&pClust->lock);

    /* If we are the last one in the cluster.  Signal/wake up the caller
     * and return the cluster.
     */

    if (returnClust)
	{
	signo = pClust->sigpend.sigp_info.si_signo;
	taskId  = pAiocb->aio_sys.ioNode.taskId;

	if (aioPrintRtn != NULL)
	    (* aioPrintRtn) ("clust done signal (%d) to task %x\n", signo, 
			     taskId, 0, 0, 0, 0);
	
	/* send the user defined signal */

	if (signo != 0)
	    {
	    sigPendKill (taskId, &pClust->sigpend);
	    }
	else
	    {
	    /* wake up waiting process */

	    cond_signal (&pClust->wake);
	    }

	pClust->inuse = FALSE;		/* free cluster */
	}
    }
 

/* DRIVER INTERFACE ROUTINES */

/*******************************************************************************
* 
* aioDrvInstall - install an AIO driver 
*
* This routine is called by an I/O driver to specify it supports AIO and 
* to insert it's AIO routines into the AIO driver table.  <drvnum> is 
* the driver number to install.   <pInsert> is the AIO insert routine.
* <pIoctl> the the AIO ioctl routine.  <flags> gets filled into the 
* driver specific flags member of the AIO_DRV_ENTRY structure.  
*
* RETURNS: OK if successful, otherwise ERROR.
*
* ERRNO: S_aioPxLib_DRV_NUM_INVALID
*
* INTERNAL:  The aioDrvTable is similar to the drvTable in iosLib.  
* aioDrvTable (and this routine) might go away if aioLib is more 
* integrated into iosLib.   Don't man this page until driver interface
* is documented and supported.
*
* NOMANUAL
*/

STATUS aioDrvInstall 
    (
    int		drvnum,				/* driver number */
    FUNCPTR	pInsert,			/* insert routine */
    FUNCPTR	pIoctl,				/* ioctl routine */
    int		flags				/* driver flags */
    )
    {
    AIO_LOCK ();

    if ((drvnum < 0) || (drvnum >= maxDrivers) || aioDrvTable [drvnum].inuse)
	{
	errno = S_aioPxLib_DRV_NUM_INVALID;
	return (ERROR);				/* invalid driver number */
	}

    aioDrvTable [drvnum].insert = pInsert;	/* add to driver table */
    aioDrvTable [drvnum].ioctl  = pIoctl;
    aioDrvTable [drvnum].flags  = flags;

    AIO_UNLOCK ();
    return (OK);
    }

/*******************************************************************************
* 
* aioDrvFlagsGet - get AIO driver flags
*
* RETURNS: OK if successful, otherwise ERROR.
*
* NOMANUAL
*/

STATUS aioDrvFlagsGet 
    (
    int			fd, 			/* file descriptor */
    int *		pFlags			/* return flags */
    )
    {
    AIO_FD_ENTRY * 	pEntry; 		/* fd entry */

    if ((pEntry = aioEntryFind (fd)) == NULL)
	return (ERROR); 			/* error condition */

    *pFlags = aioDrvTable [pEntry->drv_num].flags;
    return (OK);
    }

/*******************************************************************************
* 
* aioDrvFlagsSet - set AIO driver flags
*
* RETURNS: OK if successful, otherwise ERROR.
*
* NOMANUAL
*/

STATUS aioDrvFlagsSet
    (
    int			fd, 			/* file descriptor */
    int			flags			/* flags to set */
    )
    {
    AIO_FD_ENTRY * 	pEntry; 		/* fd entry */

    if ((pEntry = aioEntryFind (fd)) == NULL)
	return (ERROR); 			/* error condition */

    aioDrvTable [pEntry->drv_num].flags |= flags;
    return (OK); 
    }

/*******************************************************************************
* 
* aioNext - obtain next AIO request to initiate
*
* This routine obtains the next available AIO request in the work queue. 
* It finds the first request where the state is AIO_QUEUED. 
*
* RETURNS: pointer to next queued AIO control block or NULL
*
* NOMANUAL
*/

AIO_SYS * aioNext
    (
    AIO_DEV *		pDev				/* AIO device */
    )
    {
    AIO_SYS *		pReq = NULL;			/* AIO request */
    LIST *		pWorkQ = &pDev->ioQ.workQ;	/* work queue */

    IOQ_LOCK (&pDev->ioQ);

    for (pReq = (AIO_SYS *) lstFirst (pWorkQ); (pReq != NULL); 
	 pReq = (AIO_SYS *) lstNext (&pReq->ioNode.node))
	{
	if (pReq->state == AIO_QUEUED) 
	    {
	    pReq->state = AIO_RUNNING;
	    break;
	    }
	}

    IOQ_UNLOCK (&pDev->ioQ);

    return (pReq);
    }

/*****************************************************************************
*
* aioCancel - cancel an AIO request 
*
* This routine tries to cancel an AIO request.  If the state of the AIO
* request is not running, the request is deleted from the work queue 
* <pDev->workQ> and completed with a return value of -1 and an error 
* value of ECANCELED.
*
* RETURNS: ERROR if request is RUNNING, otherwise OK.
*
* NOMANUAL
*/

STATUS aioCancel
    (
    AIO_DEV *		pDev,			/* AIO device */
    struct aiocb *	pAiocb			/* AIO control block */
    )
    {
    int			state;			/* state */
    IO_Q *		pQ = &pDev->ioQ;	/* queue */
    AIO_SYS *		pReq = &pAiocb->aio_sys;/* AIO request */

    IOQ_LOCK (pQ);				/* lock the ioQ */

    state = pReq->state;

    if (state == AIO_RUNNING)
	{
	IOQ_UNLOCK (pQ);
        return (ERROR);				/* can't cancel running */
	}

    if (state == AIO_QUEUED) 
	IOQ_WORK_DELETE (pQ, &pReq->ioNode);
    else
	{
	if (state == AIO_WAIT)
	    IOQ_WAIT_DELETE (pQ, &pReq->ioNode);
	}

    AIO_DONE_SET (pAiocb, ERROR, ECANCELED);	/* mark it as cancelled */

    IOQ_UNLOCK (pQ);				/* unlock ioQ */

    ioQNodeDone (&pReq->ioNode);
    return (OK);
    }

/*****************************************************************************
*
* aioPush - push a queued request to head of work queue.
*
* RETURNS: OK if successful, otherwise ERROR.
*
* NOMANUAL
*/

STATUS aioPush 
    (
    AIO_DEV *		pDev,			/* AIO device */
    struct aiocb *	pAiocb			/* AIO request */
    )
    {
    IO_Q *		pQ    = &pDev->ioQ;
    IO_NODE *		pNode = &pAiocb->aio_sys.ioNode;

    IOQ_LOCK (pQ);

    if (pAiocb->aio_state == AIO_QUEUED)
	{
	IOQ_WORK_DELETE (pQ, pNode);
	IOQ_WORK_ADD_HEAD (pQ, pNode);
	}
    
    IOQ_UNLOCK (pQ);
    return (OK);
    }

/*****************************************************************************
*
* aioSync - synchronize
*
* This routine synchronizes all AIO requests assoicated with file descriptor
* <pAiocb->aio_fildes>.  <syncReqRtn> specifies a driver provided 
* synchronization routine that gets called for each request associated with 
* the above specified file.  After each AIO request has been synchronized,
* it is returned to the library via aioDone.
*
* RETURNS: OK, or ERROR if couldn't find entry.
*
* NOMANUAL
*/

STATUS aioSync 
    (
    AIO_DEV *		pDev,			/* AIO device */
    struct aiocb * 	pAiocb,			/* aio control block */
    FUNCPTR		syncReqRtn		/* routine to sync a request */
    )
    {
    FAST AIO_FD_ENTRY * pEntry;			/* fd entry */
    ARGS 		args;
    int			fd = pAiocb->aio_fildes;

    if ((pEntry = aioEntryFind (fd)) == NULL)
	return (ERROR);

    args.pDev     = pDev;			/* device */
    args.retVal   = OK;				/* return */
    args.errorVal = 0;				/* errno */
    args.syncRtn  = syncReqRtn;

    semTake (&pEntry->ioQSem, WAIT_FOREVER);

    dllEach (&pEntry->ioQ, aioSyncNode, (int) &args);

    semGive (&pEntry->ioQSem);

    if (ioctl (fd, FIOSYNC, 0) == ERROR)
    	ioctl (fd, FIOFLUSH, 0);

    /* one for the  aio_fsync() request */

    IOQ_LOCK (&pDev->ioQ);

    AIO_DONE_SET (pAiocb, args.retVal, args.errorVal);
    IOQ_WORK_DELETE (&pDev->ioQ, &pAiocb->aio_sys.ioNode);

    IOQ_UNLOCK (&pDev->ioQ);

    ioQNodeDone (&pAiocb->aio_sys.ioNode);
    return (OK);
    }

/*****************************************************************************
*
* aioSyncNode - synchronize an AIO request
* 
* This routine gets called from aioSync for each AIO request on the fd list.
* It calls the driver provided sync routine for each request whose 
* state is not completed. 
*/

LOCAL STATUS aioSyncNode
    (
    DL_NODE *		pFdNode,		/* node on fd list */
    ARGS *		pArgs			/* arguments */
    )

    {
    AIO_SYS *		pReq;			/* AIO request */
    BOOL		retVal = TRUE;		/* return value */

    pReq = FD_NODE_TO_SYS (pFdNode); 

    if (pReq->state != AIO_COMPLETED)
	{
    	if (((* pArgs->syncRtn) (pArgs->pDev, pReq)) == ERROR)
	    {
	    pArgs->retVal = ERROR;
	    pArgs->errorVal = errno;
  	    retVal = FALSE;
  	    }
	}

    return (retVal);
    }


/* The following routines are support routines for iosLib.  They wont 
 * be needed if fully integerated into the I/O system.
 */

/*****************************************************************************
*
* aioFdNew - initialize an AIO file descriptor
*
* This routine gets called from iosFdNew when allocating a new file
* descriptor. 
*
* RETURNS: N/A
*/

LOCAL void aioFdNew
    (
    int			fd			/* file descriptor */
    )
    {
    AIO_FD_ENTRY *	pEntry 	 = &aioFdTable [STD_UNFIX(fd)];

    semTake (&pEntry->ioQSem, WAIT_FOREVER);
    dllInit (&pEntry->ioQ);
    pEntry->ioQActive = TRUE;
    semGive (&pEntry->ioQSem);
    }

/*****************************************************************************
*
* aioFdFree - free a file descriptor
*
* This routine gets called from iosFdFree after closing a file descriptor. 
* It cancels (or waits for, if it can't cancel) all the outstanding AIO
* submitted to file descriptor <fd>.
*
* RETURNS: N/A
*/

LOCAL void aioFdFree
    (
    int			fd			/* file descriptor */
    )

    {
    AIO_SYS *		pReq;			/* AIO request */
    DL_NODE *		pNode;			/* fd node */
    FAST AIO_FD_ENTRY *	pEntry = &aioFdTable [STD_UNFIX(fd)];

    semTake (&pEntry->ioQSem, WAIT_FOREVER);

    FOREVER 
	{
	if ((pNode = DLL_FIRST (&pEntry->ioQ)) == NULL)
	    break;				/* list empty */
        
	pReq = FD_NODE_TO_SYS (pNode);

	/* Try to cancel it.  If can't, then wait for it */

	if (aio_cancel (fd, pReq->pAiocb) == AIO_NOTCANCELED)
	    aio_suspend ((const struct aiocb **) &pReq->pAiocb, 1, 
			 (const struct timespec *) NULL);

	dllRemove (&pEntry->ioQ, pNode);	/* remove it */
	}
    
    pEntry->ioQActive = FALSE;
    semGive (&pEntry->ioQSem);
    }

/*****************************************************************************
*
* aioEntryFind - find the aio fd entry
*
* aiEntryFind validates <fd> and finds the AIO_FD_ENTRY associated with
* <fd>. 
*
* RETURNS: a pointer to the AIO_FD_ENTRY or NULL if error.
*
* NOMANUAL
*/

AIO_FD_ENTRY * aioEntryFind 
    (
    int 		fd			/* file descriptor */
    )
    {
    int			xfd = STD_MAP (fd);	/* mapped fd */

    if (FD_CHECK (xfd) == NULL)
	{
	errno = EBADF;
	return (NULL);				/* file descriptor invalid */
	}
    return (&aioFdTable [xfd]);	
    }
