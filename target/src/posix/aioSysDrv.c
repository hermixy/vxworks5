/* aioSysDrv.c - AIO system driver */

/* Copyright 1984-1994 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
-------------------
1c,01feb94,dvs	documentation changes.
1b,12jan94,kdl  changed name to aioSysDrv.c; general cleanup.
1a,04apr93,elh  written.
*/

/* 
DESCRIPTION
This library is the AIO system driver.  The system driver implements
asynchronous I/O with system AIO tasks performing the AIO requests
in a synchronous manner.  It is installed as the default driver for AIO.

INTERNAL
Two types of tasks are used to implement the system driver: the aioIoTask 
and the aioWaitTask.  The aioIoTask services the work queue and performs 
the I/O.  It gets an AIO request from the work queue, performs the I/O 
on behalf of the task that initiated the I/O request, and then notifies 
the caller of the I/O completion.  The number of aioIoTasks gets specified
by the user in the call to aioSysInit.

The aioWaitTask services the wait queue and is used for AIO requests 
that can not be performed immediately due to blocking I/O.  The aioWaitTask 
waits for data (or events) to arrive that will allow waiting AIO requests 
to complete successfully. 

Therefore, if the aioIoTask gets an AIO request that will block, the 
aioIoTask will not execute the I/O command but instead put the request in 
"wait" state, add it to the wait queue and notify the aioWaitTask that the I/O
request is waiting on data.  

Once the data arrives, the aioWaitTask moves the request back to the 
work queue and notifies the aioIoTask the request can now be completed.

This library uses pipeDrv to communicate between the aioIoTasks and 
aioWaitTask.  Therefore pipeDrv must get dragged in to use the system 
driver.

SEE ALSO: POSIX 1003.1b document
*/

/* includes */

#include "vxWorks.h" 
#include "aioSysDrv.h"
#include "stdio.h"
#include "taskLib.h"
#include "private/iosLibP.h"
#include "logLib.h"
#include "selectLib.h"
#include "string.h"	
#include "pipeDrv.h"
#include "ioLib.h"


/* defines */

/* other defines */

#define AIO_TASK_BASE_NAME	"tAioIoTask"	/* task base name */
#define AIO_TASK_NAME_LEN	50		/* expected task name length */
#define AIO_WAIT_TASK_NAME	"tAioWait"	/* wait task name */
#define AIO_PIPE_NAME		"/aioPipe"	/* pipe name */
#define AIO_PIPE_MSG_MAX	50		/* max pipe messages */
#define READ_OP			0		/* read operation */
#define WRITE_OP		1		/* write operation */
#define IO_OP(op)		((op) == IO_READ ? READ_OP : WRITE_OP)

/* AIO system driver flags */

#define DRV_SELECT		0x4		/* select device */
#define	DRV_NOSELECT		0x8		/* not a select device */


/* typedefs */

typedef struct wait_msg				/* wait message */
    {
    int			op;			/* READ_OP || WRITE_OP */
    int			fd;			/* file descriptor */
    } WAIT_MSG;


/* globals */

FUNCPTR aioSysPrintRtn = NULL;			/* print routine */

struct						/* aioWaitTask wait fds */
    {
    fd_set	ioWait [2];	
    fd_set	io [2];	
    } ioFds;


/* locals */

LOCAL AIO_DEV	aioDev;				/* AIO device structure */
LOCAL SEMAPHORE aioIOSem;			/* I/O semaphore */
LOCAL SEMAPHORE	aioSysQSem;			/* Q semaphore (work & done) */
LOCAL SEMAPHORE	aioSysWorkSem;			/* work semaphore */
LOCAL BOOL	aioSysInitialized = FALSE;	/* library initialized */
LOCAL int	aioSysFd;			/* wait task control fd */


/* forward declarations */

void   		aioIoTask (AIO_DEV * pDev);
void   		aioWaitTask (void);

LOCAL STATUS 	aioSysInsert (int value, struct aiocb * pAiocb, int prio);
LOCAL STATUS 	aioSysIoctl (int value, int function, int arg);
LOCAL STATUS	aioSysRead (struct aiocb * pAiocb, int * pError);
LOCAL STATUS 	aioSysWrite (struct aiocb * pAiocb, int * pError);
LOCAL STATUS 	aioSysSyncReq (AIO_DEV * pDev, AIO_SYS * pReq);
LOCAL BOOL 	aioSysOpWillBlock (int fd, int op);
LOCAL BOOL 	aioSysBlockingDev (int fd);
LOCAL BOOL 	aioSysWaitFind (AIO_SYS * pReq, IO_Q * pQ, int bogus4);

/*******************************************************************************
*
* aioSysInit - initialize the AIO system driver
* 
* This routine initializes the AIO system driver.  It should be called
* once after the AIO library has been initialized.  It spawns
* <numTasks> system I/O tasks to be executed at <taskPrio> priority level,
* with a stack size of <taskStackSize>.  It also starts the wait task and sets 
* the system driver as the default driver for AIO. If <numTasks>, <taskPrio>,
* or <taskStackSize> is 0, a default value (AIO_IO_TASKS_DFLT, AIO_IO_PRIO_DFLT,
* or AIO_IO_STACK_DFLT, respectively) is used.
*
* RETURNS: OK if successful, otherwise ERROR.
*/

STATUS aioSysInit 
    (
    int			numTasks,		/* number of system tasks */
    int			taskPrio,		/* AIO task priority */
    int			taskStackSize		/* AIO task stack size */
    )

    {
    int			ix;			/* index */
    char 		taskName [AIO_TASK_NAME_LEN];		
						/* area to build task name */

    if (aioSysInitialized)
	return (OK);				/* already initialized */

    /* Set default parameter values */

    taskStackSize = (taskStackSize == 0) ? AIO_IO_STACK_DFLT : taskStackSize;
    numTasks	  = (numTasks == 0) ? AIO_IO_TASKS_DFLT : numTasks;
    taskPrio	  = (taskPrio == 0) ? AIO_IO_PRIO_DFLT : min (taskPrio, 254);

    /* Initialize the I/O queues (workQ and waitQ) */

    semMInit (&aioSysQSem, SEM_DELETE_SAFE | SEM_Q_PRIORITY);
    ioQInit (&aioDev.ioQ, ioQLockSem, ioQUnlockSem, (int) &aioSysQSem);

    /* Create IPC pipe for I/O tasks and wait task */

    pipeDrv ();
    if ((pipeDevCreate (AIO_PIPE_NAME, AIO_PIPE_MSG_MAX, 
			sizeof (WAIT_MSG)) == ERROR) ||
        (aioSysFd = open (AIO_PIPE_NAME, O_RDWR, 0666)) == ERROR)
	return (ERROR);				/* pipe driver needed */

    /* Start aioWaitTask.  It must have a lower priority than aioIoTasks. */

    if (taskSpawn (AIO_WAIT_TASK_NAME, taskPrio + 1, AIO_TASK_OPT, 
		   AIO_WAIT_STACK, (FUNCPTR) aioWaitTask, 
		   0, 0, 0, 0, 0, 0, 0, 0, 0, 0) == ERROR)
 	return (ERROR);

    /* Initialize the work and position semaphores */

    semCInit (&aioSysWorkSem, SEM_Q_FIFO, SEM_EMPTY);
    semMInit (&aioIOSem, SEM_DELETE_SAFE | SEM_Q_PRIORITY);

    /* Spawn aioIoTasks */

    while (numTasks-- > 0)
	{
	sprintf (taskName,"%s%d", AIO_TASK_BASE_NAME, numTasks); 
	if (aioSysPrintRtn != NULL)
	    (* aioSysPrintRtn) ("aioIoTask: %s starting\n", (int) taskName, 
				0, 0, 0, 0, 0);

	if (taskSpawn (taskName, taskPrio, AIO_TASK_OPT, taskStackSize, 
		       (FUNCPTR) aioIoTask, (int) &aioDev, 0, 0, 0, 0, 0, 0, 
		       0, 0, 0) == ERROR)
	    return (ERROR);
	}

    /* Install AIO system driver as the default driver */

    for (ix = 0 ; ix < maxDrivers; ix++)
	aioDrvInstall (ix, aioSysInsert, aioSysIoctl, 0);

    aioSysInitialized = TRUE;			/* mark as initialized */
    return (OK);
    }

/*****************************************************************************
*
* aioSysInsert - Insert an AIO request into the system work queue
*
* This routine accepts an AIO request into the driver.  If there are 
* other requests waiting on the fd (for the requested op) it adds 
* the AIO request to the wait queue and notifies the aioWaitTask.  
* Otherwise it adds the request to the work queue and signals the aioIoTask.
*
* RETURNS: OK if successfully inserted, ERROR otherwise.
*/

LOCAL STATUS aioSysInsert 
    (
    int 		value,			/* not used */
    struct aiocb *	pAiocb,			/* AIO control block */
    int			prio			/* priority */
    )
    {
    WAIT_MSG 		newFd;			/* new fd to wait on */
    IO_Q *		pQ = &aioDev.ioQ;	/* I/O queue */
    AIO_SYS *		pReq = &pAiocb->aio_sys;/* AIO request */

    if (aioSysPrintRtn != NULL)
	(* aioSysPrintRtn) ("aioSysInsert: aiocb (0%x) prio (%d) op %d\n", 
			    (int) pAiocb, prio, pReq->ioNode.op, 0, 0, 0);

    IOQ_LOCK (pQ);				/* lock access */
     
    if (pReq->state != AIO_READY)
   	{
        IOQ_UNLOCK (pQ);
	return (ERROR);				/* requests been mucked */ 
	}

    /* Check if the wait task is already waiting on the file descriptor
     * for the requested op.  
     */

    if (FD_ISSET (pAiocb->aio_fildes, &ioFds.ioWait [IO_OP(pReq->ioNode.op)]))
	{
        if (aioSysPrintRtn != NULL)
	    (* aioSysPrintRtn) ("aioSysInsert: will block - move wait queue\n");

	IOQ_WAIT_ADD (pQ, &pReq->ioNode, prio);		/* add to wait Q */
	pReq->state = AIO_WAIT;

    	IOQ_UNLOCK (pQ);
		
	newFd.op = IO_OP (pReq->ioNode.op);		/* notify waitTask */
	newFd.fd = pAiocb->aio_fildes;
	write (aioSysFd, (caddr_t) &newFd, sizeof (WAIT_MSG));
	}

    else
	{
        if (aioSysPrintRtn != NULL)
	    (* aioSysPrintRtn) ("aioSysInsert: move to work queue\n");

    	IOQ_WORK_ADD (pQ, &pReq->ioNode, prio);		/* add it to work Q */ 
    	pReq->state = AIO_QUEUED;
    	IOQ_UNLOCK (pQ);

    	semGive (&aioSysWorkSem);			/* notify I/O Task */
	}

    return (OK);
    }

/*****************************************************************************
*
* aioSysIoctl - control an AIO request
*
* This routine performs a control operation on a previously submitted AIO 
* request.
*
* RETURNS: OK if successful, otherwise ERROR.
*/

LOCAL STATUS aioSysIoctl 
    (
    int		value,				/* not used */
    int		function,			/* ioctl function */
    int		arg				/* argument */
    )
    {
    int		retVal = OK;			/* return value */

    switch (function)
	{
 	case FAIO_CANCEL : 			/* cancel request */
	    retVal = aioCancel (&aioDev, (struct aiocb *) arg); 
	    break;

	case FAIO_PUSH :			/* move to head */
	    retVal = aioPush (&aioDev, (struct aiocb *) arg);
	    break;

	default:				/* unknown function */ 
	    errno = S_ioLib_UNKNOWN_REQUEST;
	    retVal = ERROR;
	    break;
	}

    return (retVal);
    }

/*******************************************************************************
*
* aioIoTask - AIO I/O task
*
* This routine performs the I/O on behalf of the caller.  It gets a requests 
* from the work queue and if the operation will not block, performs the 
* requested operation.  If the request will block it moves the request to the
* wait queue and notifies the wait server of the new addition.
*
* RETURNS: N/A
* NOMANUAL
*/
 
void aioIoTask
    (
    AIO_DEV *           pDev			/* AIO device */
    )
 
    {
    AIO_SYS *		pReq;			/* AIO request */
    int			op;			/* operation */
    int			fd;			/* file descriptor */
    int                 retVal;			/* return value */
    int                 errorVal;           	/* error value */
    WAIT_MSG		waitMsg;		/* wait msg */ 
 
    FOREVER
        {
        semTake (&aioSysWorkSem, WAIT_FOREVER);     /* wait for work */

        if ((pReq = aioNext (pDev)) == NULL)	   /* Get I/O request */
            continue;

 	op = pReq->ioNode.op;
	fd = pReq->pAiocb->aio_fildes;

	if (aioSysPrintRtn != NULL)
	    (* aioSysPrintRtn) ("aioIoTask: (0x%x) op: %d fd: %d\n", 
				(int) pReq->pAiocb, op, fd);

    	semTake (&aioIOSem, WAIT_FOREVER);

	/* Sync function */

   	if (op == IO_SYNC) 
	    {
	    aioSync (pDev, pReq->pAiocb, aioSysSyncReq);
    	    semGive (&aioIOSem);
	    continue;
	    }

	if (aioSysBlockingDev (fd) && aioSysOpWillBlock (fd, op))
	    {

	    /* If the device is a blocking device and the requested
	     * operation will block, then move it from the work queue to 
	     * to the wait queue, and notify the aioWaitTask of new waiter.
	     */

	    semGive (&aioIOSem);

    	    IOQ_LOCK (&pDev->ioQ);
	    IOQ_WORK_DELETE (&pDev->ioQ, &pReq->ioNode);
	    IOQ_WAIT_ADD (&pDev->ioQ, &pReq->ioNode, pReq->ioNode.prio);
	    pReq->state = AIO_WAIT;			/* wait state */

    	    IOQ_UNLOCK (&pDev->ioQ);

	    waitMsg.op = IO_OP (op);
	    waitMsg.fd = fd;
	    write (aioSysFd, (char *) &waitMsg, sizeof (WAIT_MSG));
	    continue;
	    }

	/* Perform the requested I/O */

 	switch (op)
	    {
	    case IO_READ:
	    	retVal = aioSysRead (pReq->pAiocb, &errorVal); 
		break;

	    case IO_WRITE:
            	retVal = aioSysWrite (pReq->pAiocb, &errorVal);
		break;
	
	    default:
	    	retVal   = ERROR;
	    	errorVal = EINVAL;
		break;
	    }

	semGive (&aioIOSem);

	/* Mark request as completed and send the request back to aioLib */

    	IOQ_LOCK (&pDev->ioQ);

	AIO_DONE_SET (pReq->pAiocb, retVal, errorVal);
        IOQ_WORK_DELETE (&pDev->ioQ, &pReq->ioNode);

	IOQ_UNLOCK (&pDev->ioQ);

	ioQNodeDone (&pReq->ioNode);
        }
    }

/*****************************************************************************
*
* aioSysSyncReq - synchronize an AIO request 
*
* This routine attempts to synchronize the AIO request <pReq> to the 
* synchronized I/O completion state.
* 
* RETURNS: OK if successful, ERROR otherwise.
*/

LOCAL STATUS aioSysSyncReq
    (
    AIO_DEV *		pDev,			/* AIO device */
    AIO_SYS *		pReq			/* AIO request */
    )
    {
    int			errorVal;		/* error value */
    IO_Q *		pQ = &pDev->ioQ;	/* I/O queue */
    STATUS 		retVal = OK;		/* return value */

    if ((pReq->state == AIO_QUEUED) && (pReq->ioNode.op == IO_WRITE))
	{
	if (aioSysWrite (pReq->pAiocb, &errorVal) == ERROR)
	    retVal = ERROR;			/* write failed */

   	/* complete the AIO operation */

        AIO_DONE_SET (pReq->pAiocb, retVal, errorVal);

        IOQ_LOCK (pQ);
	IOQ_WORK_DELETE (pQ, &pReq->ioNode);
    	IOQ_UNLOCK (pQ);

	ioQNodeDone (&pReq->ioNode);
	}

    return (retVal);
    }


/*****************************************************************************
*
* aioSysWrite - AIO system driver write routine 
*
* This routine performs a write call for the AIO request <pAiocb>.
*
* RETURNS: return value for the write call. 
*
* INTERNAL
* POSIX says that writes can append to the file if O_APPEND is set
* for the file.  When we implement O_APPEND, the following code will 
* need to be changed.
*/
    
LOCAL STATUS aioSysWrite
    (
    struct aiocb *	pAiocb,			/* AIO control block */
    int *		pError
    )
    {
    int			retVal = ERROR;		/* return value */

    /* lseek protected by aioIOSem */

    lseek (pAiocb->aio_fildes, pAiocb->aio_offset, SEEK_SET); 

    retVal = write (pAiocb->aio_fildes, (char *) pAiocb->aio_buf, 
		    pAiocb->aio_nbytes); 

    if (aioSysPrintRtn != NULL)
	(* aioSysPrintRtn) 
	    ("aioSysWrite:fd (%d) wrote %d bytes buffer 0x%x loc %d \n", 
	     pAiocb->aio_fildes, retVal, pAiocb->aio_buf, pAiocb->aio_offset);


    *pError = (retVal == ERROR) ? errno : 0;

    return (retVal); 
    }

/*****************************************************************************
*
* aioSysRead - AIO system driver read routine
*
* This routine performs a read call for the AIO request <pAiocb>.
*
* RETURNS:  return value from the read call. 
*/

LOCAL STATUS aioSysRead 
    (
    struct aiocb *	pAiocb,			/* AIO control block */
    int *		pError
    )

    {
    int			retVal = ERROR;		/* return value */

    /* lseek protected by aioIOSem */

    lseek (pAiocb->aio_fildes, pAiocb->aio_offset, SEEK_SET); 

    retVal = read (pAiocb->aio_fildes, (char *) pAiocb->aio_buf, 
		   pAiocb->aio_nbytes);

    if (aioSysPrintRtn != NULL)
	(* aioSysPrintRtn) 
	    ("aioSysRead:fd (%d) read %d bytes buffer 0x%x loc %d \n", 
	     pAiocb->aio_fildes, retVal, pAiocb->aio_buf, pAiocb->aio_offset);

    *pError = (retVal == ERROR) ? errno : 0;

    return (retVal); 
    }


/*******************************************************************************
*
* aioWaitTask - AIO wait task
*
* The AIO wait task is responsible for managing the AIO requests that are
* are in AIO_WAIT state.  These are requests that can not be completed 
* immediately because they are waiting for I/O on blocking devices.  
* When data becomes available on a file descriptor, the aioWaitTask finds
* the waiting request and moves it back to the work queue to be executed
* by the aioIoTask.
*
* RETURNS: N/A
* NOMANUAL
*/

void aioWaitTask (void)
    {
    int			ndone;				/* num done */
    WAIT_MSG 		waitMsg;			/* control message */
    IO_Q *		pIoQ = &aioDev.ioQ;		/* I/O queue */

    /* clear out the file descriptors - set control fd */

    FD_ZERO (&ioFds.ioWait [READ_OP]);
    FD_ZERO (&ioFds.ioWait [WRITE_OP]);
    FD_SET (aioSysFd, &ioFds.ioWait [READ_OP]);

    FOREVER 
	{
	ioFds.io [READ_OP] = ioFds.ioWait [READ_OP];
	ioFds.io [WRITE_OP] = ioFds.ioWait [WRITE_OP];

	/* Wait for data and/or control messages */

	ndone = select (FD_SETSIZE, &ioFds.io [READ_OP], &ioFds.io [WRITE_OP], 
			NULL, NULL);

  	if (FD_ISSET (aioSysFd, &ioFds.io [READ_OP]))
	    {	    

	    /* Got a control message with new fd to wait on */

	    read (aioSysFd, (caddr_t) &waitMsg, sizeof (WAIT_MSG));

	    if (aioSysPrintRtn != NULL)
		(* aioSysPrintRtn) ("aioWaitTask: control op %s fd %d\n",  
			            (waitMsg.op == READ_OP) ? "read" : "write",
				    waitMsg.fd);
	    
	    /* mask in the new fd to wait on */

	    FD_SET (waitMsg.fd, &ioFds.ioWait [waitMsg.op]);
		    
	    if (--ndone == 0)
		continue;			/* only got control info */
	    }


	/* Data became available.  Find the request(s) this will satisfy */

    	IOQ_LOCK (pIoQ);
	FD_ZERO (&ioFds.ioWait [READ_OP]);
	FD_ZERO (&ioFds.ioWait [WRITE_OP]);
        ioQEach (&pIoQ->waitQ, aioSysWaitFind, (int) pIoQ, 0);
	IOQ_UNLOCK (pIoQ);

        FD_SET (aioSysFd, &ioFds.ioWait [READ_OP]);	/* add control fd */
	}
    }

/*****************************************************************************
*
* aioSysBlockingDev - check if a device is a blocking device
*
* This routine determines if the device associated with file descriptor
* <fd> is a blocking (select) device.
*
* RETURNS: TRUE if the device supports select, FALSE otherwise.
* NOMANUAL
*/

BOOL aioSysBlockingDev
    (
    int 		fd 			/* file descriptor */
    )
    {
    fd_set		readFds;		/* read fds */
    BOOL		selectDev;		/* device supports select */
    int			flags;			/* driver flags */
    struct timeval	timeOut = {0, 0};	/* timeout */

    if (aioDrvFlagsGet (fd, &flags) == ERROR)
	return (FALSE);

    /* check flags if we already know this information */

    if (flags & DRV_NOSELECT)
	return (FALSE);					/* no select */

    if (flags & DRV_SELECT)
	return (TRUE);					/* has select */

    /* Poll driver to see if select fails */

    FD_ZERO (&readFds);
    FD_SET (fd, &readFds);

    selectDev = select (FD_SETSIZE, &readFds, NULL, NULL, &timeOut) == ERROR ?
		FALSE : TRUE;

    aioDrvFlagsSet (fd, selectDev ? DRV_SELECT : DRV_NOSELECT);
    return (selectDev);
    }

/*******************************************************************************
*
* aioSysOpWillBlock - requested operation will block
*
* This routine determines if the the operation specified by <op> will block
* on the device associated with file descriptor <fd>.
*
* RETURNS: TRUE if the operation will block, FALSE otherwise.
*/

LOCAL BOOL aioSysOpWillBlock 
    (
    int			fd,			/* file descriptor */
    int			op			/* operation */
    )

    {
    fd_set		readFds;		/* read fds */
    fd_set		writeFds;		/* write fds */
    struct timeval	timeOut = {0, 0};	/* timeout */
    
    /* Poll the device to see if the requested operation will block */

    FD_ZERO (&readFds); 
    FD_ZERO (&writeFds);
    FD_SET (fd, (op == IO_READ) ? &readFds : &writeFds);
    return ((select (FD_SETSIZE, &readFds, &writeFds, NULL, &timeOut) == 0) ?
 	    TRUE : FALSE);
    }


/*******************************************************************************
*
* aioSysWaitFind - find waiting requests (each routine)
*
* This routine gets called once for each AIO request in the wait queue.
* It looks at the AIO request <pReq> to determine if the waiting request may 
* have been satisfied by the recent wake up.  It also reconstructs the 
* file descriptors for the aioWaitTask to select on.
*  
* RETURNS: TRUE
*/

LOCAL BOOL aioSysWaitFind
    (
    AIO_SYS *		pReq,			/* AIO request */
    IO_Q *		pQ,			/* I/O queue */
    int			bogus			/* not used */
    )
    {
    int			op;			/* operation */
    int			fd; 			/* file descriptor */

    op = IO_OP (pReq->ioNode.op);
    fd = pReq->pAiocb->aio_fildes;

    if (FD_ISSET (fd, &ioFds.io [op]))

	{
	/* Request may have been satisfied, move it from the 
	 * wait queue to the work queue and notify the I/O tasks 
	 * of new work. 
	 */

	IOQ_WAIT_DELETE (pQ, &pReq->ioNode);
    	IOQ_WORK_ADD (pQ, &pReq->ioNode, pReq->ioNode.prio);
    	pReq->pAiocb->aio_sys.state = AIO_QUEUED;
	FD_CLR (fd, &ioFds.io [op]);
        semGive (&aioSysWorkSem);			/* notify i/o task */
	}
    else

    	FD_SET (fd, &ioFds.ioWait [op]);

    return (TRUE);					/* do entire list */
    }
