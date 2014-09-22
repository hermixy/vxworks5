/* mutexPxLib.c - kernel mutexs and condition variables library */

/* Copyright 1993-1994 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01d,24jan94,smb  added instrumentation macros
01c,12jan94,kdl  added includes of intLib.h, sysLib.h; general cleanup.
01b,13dec93,dvs  made NOMANUAL
01a,18feb93,rrr  written.
*/

/*
DESCRIPTION
This library provides the interface to kernel mutexs and condition variables.
Mutexs provide mutually exclusive access to resources.  Condition variables,
when used with mutexes, provide synchronization between threads.

The example below illustrates the use of a mutex.  The first step is to
initialize the mutex (see mutex_init() for more details).
.CS
.ne 4
    mutex_t mutex;

    mutex_init (&mutex, ...);
.CE
Then guard a critical section or resource by taking the mutex with
mutex_lock(), and exit the section or release the resource by giving the
mutex with a mutex_unlock().  For example:
.CS
.ne 4
    mutex_lock (&mutex);
	... /@ critical region, only accessible by a single thread at a time @/
    mutex_unlock (&mutex);
.CE
A condition variable works in conjunction with a mutex to provide
synchronization between multiple threads. A thread will block at a
call to cond_wait() and remain blocked until another thread signals
the condition with a call to cond_signal().  A condition variable is
considered a resource and must be protected with a mutex.
See cond_wait() and cond_signal() for more details.

The following example is for a one byte pipe.  The writer will sit on the
pipe until it can write the data.  The reader will abort the read if a
signal occurs.
.CS
.ne 4
    struct pipe
        {
        mutex_t p_mutex;
        cond_t  p_writer;
        cond_t  p_reader;
        int     p_empty;
        char    p_data;
        };

    pipe_init (struct pipe *pPipe)
        {
        mutex_init (&pPipe->p_mutex, MUTEX_THREAD);
        cond_init (&pPipe->p_reader, &pPipe->p_mutex);
        cond_init (&pPipe->p_writer, &pPipe->p_mutex);
        pPipe->p_empty = TRUE;
        }

    pipe_write (struct pipe *pPipe, char data)
        {
        mutex_lock (&pPipe->p_mutex);
        while (pPipe->p_empty != TRUE)
	    cond_wait (&pPipe->p_writer, &pPipe->p_mutex, 0, "pipe write", 0);
        pPipe->p_empty = FALSE;
        pPipe->p_data = data;
	cond_signal (&pPipe->p_reader, &pPipe->p_mutex);
        mutex_unlock (&pPipe->p_mutex);
        }

    pipe_read (struct pipe *pPipe)
        {
        char data;
        int error;

        mutex_lock (&pPipe->p_mutex);
        while (pPipe->p_empty == TRUE)
	    {
	    error = cond_wait (&pPipe->p_reader, &pPipe->p_mutex,
				SIGCATCH, "pipe read", 0);
            if (error != 0)
                {
		mutex_unlock (&pPipe->p_mutex);
                errno = error;
                return -1;
                }
	    }
        pPipe->p_empty = TRUE;
        data = pPipe->p_data;
	cond_signal (&pPipe->p_writer, &pPipe->p_mutex);
        mutex_unlock (&pPipe->p_mutex);

	return data;
        }
.CE
NOMANUAL
*/

#include "vxWorks.h"
#include "private/sigLibP.h"
#include "private/mutexPxLibP.h"
#include "private/windLibP.h"
#include "private/eventP.h"
#include "timers.h"
#include "errno.h"
#include "intLib.h"
#include "sysLib.h"

/* locals */

/* globals */

/*******************************************************************************
*
* mutex_init - initialize a kernel mutex
*
* This routine initializes a kernel mutex.
*
* NOMANUAL
*/

void mutex_init
    (
    mutex_t     *pMutex,
    void	*dummy
    )
    {
    pMutex->m_owner = 0;
    qInit (&pMutex->m_queue, Q_PRI_LIST, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    }

/*******************************************************************************
*
* mutex_destroy - Destroy a kernel mutex
*
* This routine destroys a kernel mutex.
*
* NOMANUAL
*/

void mutex_destroy
    (
    mutex_t	*pMutex
    )
    {
    kernelState = TRUE;			/* ENTER KERNEL */

    if (Q_FIRST (&pMutex->m_queue) != NULL)
	{
        /* windview - level 2 event logging */
        EVT_TASK_1 (EVENT_OBJ_SEMFLUSH, pMutex);

	windPendQFlush (&pMutex->m_queue);
	}

    windExit();				/* EXIT KERNEL */
    }

/*******************************************************************************
*
* cond_init - initialize a kernel condition variable
*
* This routine initializes a kernel condition variable.
*
* NOMANUAL
*/

void cond_init
    (
    cond_t	*pCond,
    void	*pDummy
    )
    {
    pCond->c_mutex = NULL;
    qInit (&pCond->c_queue, Q_PRI_LIST, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    }

/*******************************************************************************
*
* cond_destroy - Destroy a kernel condition variable
*
* This routine destroys a kernel condition variable.
*
* NOMANUAL
*/

void cond_destroy
    (
    cond_t	*pCond
    )
    {
    kernelState = TRUE;			/* ENTER KERNEL */

    if (Q_FIRST (&pCond->c_queue) != NULL)
	{
        /* windview - level 2 event logging */
        EVT_TASK_1 (EVENT_OBJ_SEMFLUSH, pCond);

	windPendQFlush (&pCond->c_queue);
	}

    windExit();				/* EXIT KERNEL */
    }

/*******************************************************************************
*
* mutex_lock - Take a kernel mutex
*
* This routine takes a kernel mutex.
*
* NOMANUAL
*/

void mutex_lock
    (
    mutex_t	*pMutex
    )
    {
    int level;

    level = intLock ();			/* LOCK INTERRUPTS */

    if (pMutex->m_owner == NULL)
	{
	pMutex->m_owner = (int) taskIdCurrent;

	intUnlock (level);		/* UNLOCK INTERRUPTS */
	return;
	}

    kernelState = TRUE;			/* ENTER KERNEL */
    intUnlock (level);			/* UNLOCK INTERRUPTS */

    /* windview - level 2 event logging */
    EVT_TASK_1 (EVENT_OBJ_SEMTAKE, pMutex);

    windPendQPut (&pMutex->m_queue, WAIT_FOREVER);
    windExit ();			/* EXIT KERNEL */
    }

/*******************************************************************************
*
* mutex_unlock - Release a kernel mutex
*
* This routine releases a kernel mutex.
*
* NOMANUAL
*/

void mutex_unlock
    (
    mutex_t	*pMutex
    )
    {
    int level;

    if (pMutex->m_owner != (int) taskIdCurrent)
	return;

    level = intLock ();			/* LOCK INTERRUPTS */

    if ((pMutex->m_owner = (int) Q_FIRST (&pMutex->m_queue)) != NULL)
	{
	kernelState = TRUE;		/* ENTER KERNEL */
	intUnlock (level);		/* UNLOCK INTERRRUPTS */

        /* windview - level 2 event logging */
        EVT_TASK_1 (EVENT_OBJ_SEMGIVE, pMutex);

	windPendQGet (&pMutex->m_queue);
	windExit ();			/* EXIT KERNEL */
	}
    else
	intUnlock (level);		/* UNLOCK INTERRUPTS */
    }

/*******************************************************************************
*
* cond_signal - Signal a condition variable
*
* This routine signals a condition variable.  That is it will resume a
* single thread that was suspended on a cond_wait().
*
* NOMANUAL
*/

void cond_signal
    (
    cond_t	*pCond
    )
    {
    WIND_TCB *pTcb;

    kernelState = TRUE;			/* ENTER KERNEL */

    if (Q_FIRST (&pCond->c_queue) != NULL)
	{
	if (pCond->c_mutex->m_owner == NULL)
	    {
	    pCond->c_mutex->m_owner = (int) Q_FIRST(&pCond->c_queue); 

            /* windview - level 2 event logging */
            EVT_TASK_1 (EVENT_OBJ_SEMGIVE, pCond);

	    windPendQGet (&pCond->c_queue);
	    }
	else
	    {
	    pTcb = (WIND_TCB *) Q_GET (&pCond->c_queue);
	    Q_PUT (&pCond->c_mutex->m_queue, pTcb, pTcb->priority);
	    }

	if (Q_FIRST (&pCond->c_queue) == NULL)
	    pCond->c_mutex = NULL;
	}

    windExit();				/* EXIT KERNEL */
    }

/*******************************************************************************
*
* cond_timedwait - Wait on a condition variable
*
* This routine suspends the calling thread on the condition variable
* pointed to by pCond.  The thread will resume when another thread signals
* the condition variable using the function cond_signal().  The mutex
* pointed to by pMutex must be owned by the calling thread.  While the
* thread is suspended, the mutex will be given back.  When the thread
* resumes, the mutex will be taken back.
*
* The argument pTimeout is optional.  If it is NULL, then cond_timedwait will
* wait indefinitely.  If it is not NULL, then it points to a timespec
* structure with the minimum time cond_timedwait should suspend the thread.
* If the minimum time has been reached, then cond_timedwait() return EAGAIN.
*
* RETURNS
* Cond_wait() returns 0 if another thread resumed it using cond_signal().
* Otherwise it returns EINTR if a signal occured during the wait or EAGAIN
* if the time expired.  In all cases the mutex pMutex is given during the
* time suspended are taken back when the function returns.
*
* NOMANUAL
*/

int cond_timedwait
    (
    cond_t			*pCond,		/* Cond to wait on */
    mutex_t			*pMutex,	/* Mutex to give */
    const struct timespec	*pTimeout	/* max time to wait */
    )
    {
    int tickRate;
    int wait;
    int retval;

    if (pTimeout != 0)
	{
	tickRate = sysClkRateGet();
	wait = pTimeout->tv_sec * tickRate +
		pTimeout->tv_nsec / (1000000000 / tickRate);
	}
    else
	wait = WAIT_FOREVER;


    kernelState = TRUE;			/* ENTER KERNEL */

    if (pCond->c_mutex != 0 && pCond->c_mutex != pMutex)
	{
	windExit();			/* EXIT KERNEL */
	return (EINVAL);
	}

    pCond->c_mutex = pMutex;

    /* windview - level 2 event logging */
    EVT_TASK_1 (EVENT_OBJ_SEMTAKE, pCond);

    windPendQPut (&pCond->c_queue, wait);

    if ((pMutex->m_owner = (int) Q_FIRST (&pMutex->m_queue)) != NULL)
	{
        /* windview - level 2 event logging */
        EVT_TASK_1 (EVENT_OBJ_SEMGIVE, pMutex);

	windPendQGet (&pMutex->m_queue);
	}

    if ((retval = windExit ()) != 0)	/* EXIT KERNEL */
	{
	mutex_lock(pMutex);
	return (retval == RESTART) ? EINTR : EAGAIN;
	}

    return (0);
    }
