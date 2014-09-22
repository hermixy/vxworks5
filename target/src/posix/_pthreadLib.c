/* _pthreadLib.c - POSIX 1003.1c thread library kernel support */

/* Copyright 1984-2002 Wind River Systems, Inc.  */
/* Copyright (c) 1995 by Dot4, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01f,03may02,gls  fixed memory leak when user specifies stack (SPR #76769)
01e,22apr02,gls  removed references to AE in comments (SPR #75799)
01d,26nov01,gls  changed stackAddr to char * for DIAB support
01c,31oct01,jgn  fix rounding of stack size (SPR #71350)
01b,22oct01,jgn  correct scheduling policy inheritance (SPR #71125)
01a,10sep00,jgn  created from pthreadLib.c version 01c (SPR #33375) +
		 ensure TCB pdId set always (SPR #33711)
*/

/*
DESCRIPTION
Kernel support routines for the VxWorks pthreads implementation. 

NOMANUAL
*/


/* includes */

#include "vxWorks.h"
#include "intLib.h"
#include "kernelLib.h"
#include "private/schedP.h"
#include "private/taskLibP.h"
#include "pthread.h"
#include "semLib.h"
#include "string.h"
#include "taskLib.h"
#include "taskArchLib.h"
#include "taskHookLib.h"
#include "limits.h"

#undef PTHREADS_DEBUG

#ifdef PTHREADS_DEBUG
#undef LOCAL
#define LOCAL
#endif

/* defines */

#define DEF_STACK_SIZE  (20 * 1024)

#define MY_PTHREAD ((internalPthread *)(taskIdCurrent->pPthread))
#define CANCEL_LOCK(pThread)    semTake((pThread)->cancelSemId, WAIT_FOREVER)
#define CANCEL_UNLOCK(pThread)  semGive((pThread)->cancelSemId)

/*
 * Need copies of these since we are going to have to pass in the current
 * PD's numbering scheme to _pthreadCreate() and can't use the existing
 * global variable that the original macros use...
 */

#define PX_VX_PRI_CONV(mode,pri) (mode ? (POSIX_HIGH_PRI - pri) : pri)

/* locals */

LOCAL BOOL	pthreadLibInited	= FALSE;
LOCAL FUNCPTR	pthreadDeleteTaskEntry	= NULL;

LOCAL pthread_attr_t defaultPthreadAttr =                               \
    {                                                                   \
    PTHREAD_INITIALIZED,        /* object status      */                \
    ((size_t) 0),               /* stacksize          */                \
    NULL,                       /* stackaddr          */                \
    PTHREAD_CREATE_JOINABLE,    /* detachstate        */                \
    PTHREAD_SCOPE_SYSTEM,       /* contentionscope    */                \
    PTHREAD_INHERIT_SCHED,      /* inheritsched       */                \
    SCHED_RR,                   /* schedpolicy        */                \
    NULL,                       /* name               */                \
    {0},                        /* struct sched_param */                \
    };

/* externals */

IMPORT BOOL	vxTas (void *addr);
IMPORT BOOL	roundRobinOn;
IMPORT ULONG	roundRobinSlice;

/* forward references */

LOCAL void deleteHook (WIND_TCB *pTcb);

/*******************************************************************************
*
* _pthreadLibInit - initialize POSIX threads support
*
* This routine initializes the POSIX threads (<pthreads>) support for
* VxWorks. It should only be called via pthreadLibInit() since it needs to
* register the delete hook's user level function.
*
* Multiple attempts to initialise are ignored, but the delete hook function will
* be changed. This is to ensure that if the user level code is replaced then
* the new function pointer will be available.
*
* RETURNS: N/A
*
* NOMANUAL
*/

STATUS _pthreadLibInit
    (
    FUNCPTR	deleteTask		/* user level code for delete hook */
    )
    {
    /* Always store this in case it is simply being changed */

    pthreadDeleteTaskEntry = deleteTask;

    taskLock();

    if (pthreadLibInited)
	{
	taskUnlock();
	return OK;
	}

    if (taskDeleteHookAdd((FUNCPTR) deleteHook) == ERROR)
	{
	if (_func_logMsg != NULL)
	    _func_logMsg ("taskDeleteHookAdd of pthread deleteHook failed!\n",
			  0, 0, 0, 0, 0, 0);
	}
    else
	{
	pthreadLibInited = TRUE;
	_func_pthread_setcanceltype = _pthreadSetCancelType;
	}

    taskUnlock();

    return ((pthreadLibInited == TRUE) ? OK : ERROR);
    }

/*
 * Section 16 - Thread Management
 */

/*******************************************************************************
*
* _pthreadCreate - create a thread (POSIX)
*
* This is the actual thread creation function. It is called by the user level
* pthread_create() function.
*
* NOMANUAL
*/

int _pthreadCreate
    (
    pthread_t *			pThread,
    const pthread_attr_t *	pAttr,
    void *			(*wrapperFunc)(void *),
    void *			(*start_routine)(void *),
    void *			arg,
    int				priNumMode
    )
    {
    char name[NAME_MAX];

    char *      pBufStart;      /* points to temp name buffer start */
    char *      pBufEnd;        /* points to temp name buffer end */
    char        temp;           /* working character */
    int         value;          /* working value to convert to ascii */
    int         nPreBytes;      /* nameless prefix string length */
    int         nBytes    = 0;  /* working nameless name string length */

    static int		namecntr	= {0};
    static char *	prefix		= "pthr";
    static char *digits = "0123456789";

    WIND_TCB *		pTcb;
    internalPthread *	pMyThread;
    char *		stackaddr	= NULL;
    int			stacksize;

    if (!pThread)
	return (EINVAL);

    if (!pthreadLibInited)
	return (EINVAL);

    if (pAttr && pAttr->threadAttrStatus != PTHREAD_INITIALIZED)
	return (EINVAL);
    else if (!pAttr)
	pAttr = &defaultPthreadAttr;

    /*
     * Make sure that sched policy is supported, valid, and matches global
     * policy. Only perform this test if the inheritsched attribute is set
     * to PTHREAD_EXPLICIT_SCHED though; if we are inheriting then we'll get
     * it right by definition (no need to worry about finding out the current
     * policy since for VxWorks it is system-wide).
     */

    if (pAttr->threadAttrInheritsched == PTHREAD_EXPLICIT_SCHED)
	{
	if ((pAttr->threadAttrSchedpolicy == SCHED_OTHER)		||
	    ((roundRobinOn == TRUE) &&
			  (pAttr->threadAttrSchedpolicy != SCHED_RR))	||
	    ((roundRobinOn == FALSE) &&
			  (pAttr->threadAttrSchedpolicy != SCHED_FIFO)))
	    {
	    errno = ENOTTY;
	    return (ENOTTY);
	    }
	}

    if (!(pMyThread = malloc(sizeof (internalPthread))))
	return (EAGAIN);

    bzero((char *)pMyThread, sizeof (internalPthread));

    /*
     * If zero is specified, use the default stack size, otherwise use the
     * specified size, rounded up to PTHREAD_STACK_MIN if necessary.
     */

    if (pAttr->threadAttrStacksize == 0)
	{
	stacksize = DEF_STACK_SIZE;
	}
    else
	{
	stacksize = max(pAttr->threadAttrStacksize, PTHREAD_STACK_MIN);
	}

    if (pAttr->threadAttrStackaddr)
	{
	pMyThread->flags |= STACK_PASSED_IN;
	stackaddr = pAttr->threadAttrStackaddr;
	}

    if (pAttr->threadAttrDetachstate == PTHREAD_CREATE_JOINABLE)
	{
	pMyThread->flags |= JOINABLE;
	if ((pMyThread->exitJoinSemId = semBCreate(SEM_Q_PRIORITY,
						   SEM_EMPTY)) == NULL)
	    {
	    free(pMyThread);
	    return (EAGAIN);
	    }
	}

    if ((pMyThread->mutexSemId =
			semMCreate(SEM_Q_PRIORITY|SEM_INVERSION_SAFE)) == NULL)
	{
	semDelete(pMyThread->exitJoinSemId);
	free(pMyThread);
	return (EAGAIN);
	}

    if ((pMyThread->cancelSemId =
			semMCreate(SEM_Q_PRIORITY|SEM_INVERSION_SAFE)) == NULL)
	{
	semDelete(pMyThread->mutexSemId);
	semDelete(pMyThread->exitJoinSemId);
	free(pMyThread);
	return (EAGAIN);
	}

    /* create name for the task */

    if (pAttr && pAttr->threadAttrName == NULL) 
      {
      strcpy (name, prefix);                  /* copy in prefix */
      nBytes = strlen (name);                 /* calculate prefix length */
      nPreBytes = nBytes;                     /* remember prefix strlen() */
      value  = ++ namecntr;                   /* bump the nameless count */
 
      do                                      /* crank out digits backwards */
	  {
	  name [nBytes++] = digits [value % 10];
	  value /= 10;                              /* next digit */
	  }
      while (value != 0);                           /* until no more digits */
 
      pBufStart = name + nPreBytes;        /* start reverse after prefix */
      pBufEnd   = name + nBytes - 1;       /* end reverse at EOS */
 
      while (pBufStart < pBufEnd)          /* reverse the digits */
	  {
	  temp        = *pBufStart;
	  *pBufStart  = *pBufEnd;
	  *pBufEnd    = temp;
	  pBufEnd--;
	  pBufStart++;
	  }

      name[nBytes] = EOS;                      /* EOS terminate string */
      } 
  
  else 
      {
      strncpy(name, pAttr->threadAttrName, NAME_MAX - 1);
      name[NAME_MAX - 1] = EOS;
      }

    /*
     * Determine the priority. This is also based on the inheritsched
     * attribute; we handled the scheduling policy inheritance above.
     */

    if (pAttr->threadAttrInheritsched == PTHREAD_INHERIT_SCHED)
	{
	taskPriorityGet(taskIdSelf(), &pMyThread->priority);

	/* Convert to POSIX format */

	pMyThread->priority = PX_VX_PRI_CONV (priNumMode, pMyThread->priority);
	}
    else
	pMyThread->priority = pAttr->threadAttrSchedparam.sched_priority;

    if (!(pMyThread->flags & STACK_PASSED_IN))
	{
	if (!(pTcb = (WIND_TCB *)taskCreat(name, 
		         PX_VX_PRI_CONV(priNumMode, pMyThread->priority),
			 VX_FP_TASK,
			 stacksize, 
			 (FUNCPTR) wrapperFunc, 
			 (int)start_routine, 
			 (int)arg,
			 0, 0, 0, 0, 0, 0, 0, 0)))

	    {
	    if (pMyThread->flags & JOINABLE)
		semDelete(pMyThread->exitJoinSemId);

	    semDelete(pMyThread->mutexSemId);
	    free(pMyThread);
	    return (EAGAIN);
	    }
	}
    else
	{
	/*
	 * Since we have a passed in stack pointer, we need to do what
	 * taskCreat() does, and put the TCB on the stack.
	 */

#if     (_STACK_DIR == _STACK_GROWS_DOWN)

	/*
	 * A portion of the very top of the stack is clobbered with a
	 * FREE_BLOCK in the objFree() associated with taskDestroy().
	 * There is no adverse consequence of this, and is thus not 
	 * accounted for.
	 *
	 * Given we set the stacksize above there is no need to check that
	 * there is enough room on the stack for the tcb.  There will be
	 */

	stackaddr = (char *) STACK_ROUND_DOWN (stackaddr +  stacksize - 
					       sizeof(WIND_TCB));

	pTcb = (WIND_TCB *) stackaddr;

	/* 
	 * We must adjust the stacksize as well.  However, because of the
	 * STACK_ROUND_DOWN the cleanest way to calculate this is to compare
	 * the original stack address to the new stack address.
	 */

	stacksize = ((UINT) stackaddr - (UINT) pAttr->threadAttrStackaddr);

#else	/* _STACK_GROWS_UP */

#warning "Exception stack growing up not tested"

	/*
	 * To protect a portion of the WIND_TCB that is clobbered with a
	 * FREE_BLOCK in the objFree() associated with taskDestroy(),
	 * we goose the base of tcb by 16 bytes.
	 *
	 * Given we set the stacksize above there is no need to check that
	 * there is enough room on the stack for the tcb.  There will be
	 */

	pTcb = (WIND_TCB *) (stackaddr + 16);
	stackaddr = (char *) STACK_ROUND_UP (stackaddr + 16 + 
					      sizeof(WIND_TCB));

	/* 
	 * We must adjust the stacksize as well.  However, because of the
	 * STACK_ROUND_UP the cleanest way to calculate this is to compare
	 * the original stack address to the new stack address before 
	 * subtracting from the stacksize.
	 */

	stacksize = (stacksize - ((UINT) stackaddr - 
				  (UINT) pAttr->threadAttrStackaddr));

#endif

	if (taskInit(pTcb, name,
		     PX_VX_PRI_CONV(priNumMode, pMyThread->priority),
		     (VX_FP_TASK | VX_NO_STACK_FILL), stackaddr, stacksize,
		     (FUNCPTR) wrapperFunc, (int)start_routine, (int)arg,
		     0, 0, 0, 0, 0, 0, 0, 0) == ERROR)
	    {
	    if (pMyThread->flags & JOINABLE)
		semDelete(pMyThread->exitJoinSemId);

	    semDelete(pMyThread->mutexSemId);
	    free(pMyThread);
	    return (EAGAIN);
	    }
	}

    pTcb->pPthread = pMyThread;

    pMyThread->privateData	= NULL;
    pMyThread->handlerBase	= NULL;
    pMyThread->cancelstate	= PTHREAD_CANCEL_ENABLE;
    pMyThread->canceltype	= PTHREAD_CANCEL_DEFERRED;
    pMyThread->cancelrequest	= 0;
    pMyThread->taskId		= (int)pTcb;
    pMyThread->flags		|= VALID;

    taskActivate((int)pTcb);

    *pThread = (pthread_t)pMyThread;

    return (_RETURN_PTHREAD_SUCCESS);
    }

/*
 * Section 18 - Thread Cancellation
 */

/*******************************************************************************
*
* _pthreadSetCancelType - set cancellation type for calling thread
*
* This is the kernel support routine for pthread_setcanceltype(). It is here
* so that kernel libraries (such as the I/O system code) can make calls to it.
*
* The public API, pthread_setcanceltype(), is simply a call to this routine.
*
* RETURNS: On success zero; on failure a non-zero error code.
*
* ERRNOS: EINVAL
*
* SEE ALSO: pthread_setcanceltype(),
*/

int _pthreadSetCancelType
    (
    int type,                   /* new type             */
    int *oldtype                /* old type (out)       */
    )
    {
    internalPthread *   pThread = MY_PTHREAD;
    sigset_t            set;
    sigset_t *          pset    = &set;

    if (!pthreadLibInited || !pThread)
        return (0);

#if 0
    self_become_pthread();
#endif
    if (type != PTHREAD_CANCEL_ASYNCHRONOUS && type != PTHREAD_CANCEL_DEFERRED)
        {
        return (EINVAL);
        }

    sigemptyset(pset);
    sigaddset(pset, SIGCANCEL);
    sigprocmask(SIG_BLOCK, pset, NULL);
    CANCEL_LOCK(pThread);
 
    /* If the oldtype is required, save it now */
 
    if (oldtype != NULL)
        *oldtype = pThread->canceltype;
 
    pThread->canceltype = type;
 
    CANCEL_UNLOCK(pThread);
    sigprocmask(SIG_UNBLOCK, pset, NULL);
    return (0);
    }

/*******************************************************************************
*
* deleteHook -
*
* Catch all for abnormally terminating pthreads, and those threads that
* don't call pthread_exit to terminate.
*
* RETURNS:
*
* NOMANUAL
*/

LOCAL void deleteHook
    (
    WIND_TCB *pTcb
    )
    {
    if ((pthreadDeleteTaskEntry == NULL) ||	/* not initialised */
	(pTcb->pPthread == NULL))		/* task not a thread */
	return;

    pthreadDeleteTaskEntry (pTcb->pPthread);
    }

/*******************************************************************************
*
* _pthreadSemOwnerGet - return the owner of a WIND semaphore
*
* Returns the owner task ID of a semaphore. This is only valid for mutex and
* binary semaphores, but no attempt is made by the routine to check this. It is
* the caller's responsibility to ensure that it is only called for correct
* type of semaphore.
*
* RETURNS: task ID of owner, NULL if not owned, or ERROR if the semaphore ID
* is invalid.
*
* SEE ALSO: _pthreadSemStateGet()
*
* NOMANUAL
*/
 
int _pthreadSemOwnerGet
    (
    SEM_ID      semId
    )
    {
    int owner;
    int level = intLock ();                     /* LOCK INTERRUPTS */
 
    if (OBJ_VERIFY (semId, semClassId) != OK)
        {
        intUnlock (level);                      /* UNLOCK INTERRUPTS */
        return (ERROR);
        }
 
    owner = (int) semId->semOwner;
 
    intUnlock (level);
    return owner;
    }
 
/*******************************************************************************
*
* _pthreadSemStateGet - return the state of a semaphore
*
* Returns the state of a semaphore. This will be zero or non-zero for mutex and
* binary semaphores, meaning empty/not owned and full/owned respectively. For
* counting semaphores the current count will be returned.
*
* RETURNS: current state of semaphore
*
* SEE ALSO: _pthreadSemOwnerGet()
*
* NOMANUAL
*/
 
int _pthreadSemStateGet
    (
    SEM_ID      semId
    )
    {
    int state;
    int level = intLock ();                     /* LOCK INTERRUPTS */
 
    if (OBJ_VERIFY (semId, semClassId) != OK)
        {
        intUnlock (level);                      /* UNLOCK INTERRUPTS */
        return (ERROR);
        }
 
    if ((semId->semType == (UINT8) SEM_TYPE_BINARY) ||
        (semId->semType == (UINT8) SEM_TYPE_MUTEX))
        {
        state = (semId->semOwner == NULL) ? 0 : 1;
        }
    else if (semId->semType == (UINT8) SEM_TYPE_COUNTING)
        {
        state = semId->state.count;
        }
    else
        {
        /* "Old" semaphore, or just invalid */
 
        state = ERROR;
        }
 
    intUnlock (level);
    return state;
    }
