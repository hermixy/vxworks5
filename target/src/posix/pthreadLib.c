/* pthreadLib.c - POSIX 1003.1c thread library interfaces */

/* Copyright 1984-2001 Wind River Systems, Inc.  */
/* Copyright (c) 1995 by Dot4, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01i,03may02,gls  updated pthread_attr_setstackaddr documentaion (SPR #76769)
01h,22apr02,gls  removed references to AE (SPR #75799)
01g,05nov01,gls  merged in code from AE
01f,24oct01,jgn  correct scheduling policy inheritance (SPR #71125)
		 (docs update - main change in _pthreadLib.c)
01e,04apr01,pfl  doc update (SPR #63976)
01d,11sep00,jgn  split into user and system level components (SPR #33375)
01c,22aug00,jgn  remove dependency on TCB spare4 field (SPR #33815) +
		 fix some memory handling & memory leaks (SPR #33554) +
		 move readdir_r() to dirLib.c (SPR #34056)
01b,15aug00,jgn  add bug fixes from DOT-4
01a,17jul00,jgn  created from DOT-4 version 1.17
*/

/*
DESCRIPTION
This library provides an implementation of POSIX 1003.1c threads for VxWorks.
This provides an increased level of compatibility between VxWorks applications
and those written for other operating systems that support the POSIX threads
model (often called <pthreads>).

VxWorks is a task based operating system, rather than one implementing the
process model in the POSIX sense. As a result of this, there are a few
restrictions in the implementation, but in general, since tasks are roughly
equivalent to threads, the <pthreads> support maps well onto VxWorks.
The restrictions are explained in more detail in the following paragraphs.

CONFIGURATION
To add POSIX threads support to a system, the component INCLUDE_POSIX_PTHREADS
must be added.

Threads support also requires the POSIX scheduler to be included (see
schedPxLib for more detail). 

THREADS
A thread is essentially a VxWorks task, with some additional characteristics.
The first is detachability, where the creator of a thread can optionally block
until the thread exits. The second is cancelability, where one task or thread
can cause a thread to exit, possibly calling cleanup handlers. The next is
private data, where data private to a thread is created, accessed and deleted
via keys. Each thread has a unique ID. A thread's ID is different than it's
VxWorks task ID.

MUTEXES
Included with the POSIX threads facility is a mutual exclusion facility, or
<mutex>. These are functionally similar to the VxWorks mutex semaphores (see
'semMLib' for more detail), and in fact are implemented using a VxWorks
mutex semaphore. The advantage they offer, like all of the POSIX libraries,
is the ability to run software designed for POSIX platforms under VxWorks.

There are two types of locking protocols available, PTHREAD_PRIO_INHERIT and
PTHREAD_PRIO_PROTECT. PTHREAD_PRIO_INHERIT maps to a semaphore create with
SEM_PRIO_INHERIT set (see 'semMCreate' for more detail). A thread locking a
mutex created with its protocol attribute set to PTHREAD_PRIO_PROTECT has its
priority elevated to that of of the prioceiling attribute of the mutex. When
the mutex is unlocked, the priority of the calling thread is restored to its
previous value.

CONDITION VARIABLES
Condition variables are another synchronization mechanism that is included
in the POSIX threads library. A condition variable allows threads
to block until some condition is met. There are really only two basic
operations that a condition variable can be involved in: waiting and
signalling. Condition variables are always associated with a mutex.

A thread can wait for a condition to become true by taking the mutex and
then calling pthread_cond_wait(). That function will release the mutex and
wait for the condition to be signalled by another thread. When the condition
is signalled, the function will re-acquire the mutex and return to the caller.

Condition variable support two types of signalling: single thread wake-up using
pthread_cond_signal(), and multiple thread wake-up using
pthread_cond_broadcast(). The latter of these will unblock all threads that
were waiting on the specified condition variable.

It should be noted that condition variable signals are not related to POSIX
signals. In fact, they are implemented using VxWorks semaphores.

RESOURCE COMPETITION
All tasks, and therefore all POSIX threads, compete for CPU time together. For
that reason the contention scope thread attribute is always
'PTHREAD_SCOPE_SYSTEM'.

NO VXWORKS EQUIVALENT
Since there is no notion of a process (in the POSIX sense), there is no
notion of sharing of locks (mutexes) and condition variables between processes.
As a result, the POSIX symbol '_POSIX_THREAD_PROCESS_SHARED' is not defined in
this implementation, and the routines pthread_condattr_getpshared(),
pthread_condattr_setpshared(), pthread_mutexattr_getpshared() are not
implemented.

Also, since there are no processes in VxWorks, fork(), wait(), and
pthread_atfork() are unimplemented.

VxWorks does not have password, user, or group databases, therefore there are
no implementations of getlogin(), getgrgid(), getpwnam(), getpwuid(),
getlogin_r(), getgrgid_r(), getpwnam_r(),  and getpwuid_r().

SCHEDULING
The default scheduling policy for a created thread is inherited from the system
setting at the time of creation.

Scheduling policies under VxWorks are global; they are not set per-thread,
as the POSIX model describes. As a result, the <pthread> scheduling routines,
as well as the POSIX scheduling routines native to VxWorks, do not allow you
to change the scheduling policy. Under VxWorks you may set the scheduling
policy in a thread, but if it does not match the system's scheduling policy,
an error is returned.

The detailed explanation for why this error occurs is a bit convoluted:
technically the scheduling policy is an attribute of a thread (in that there
are pthread_attr_getschedpolicy() and pthread_attr_setschedpolicy() functions
that define what the thread's scheduling policy will be once it is created,
and not what any thread should do at the time they are called). A situation
arises where the scheduling policy in force at the time of a thread's
creation is not the same as set in its attributes. In this case
pthread_create() fails with an otherwise undocumented error 'ENOTTY'.

The bottom line is that under VxWorks, if you wish to specify the scheduling
policy of a thread, you must set the desired global scheduling policy to match.
Threads must then adhere to that scheduling policy, or use the
PTHREAD_INHERIT_SCHED mode to inherit the current mode and creator's priority.

CREATION AND CANCELLATION
Each time a thread is created, the <pthreads> library allocates resources on
behalf of it. Each time a VxWorks task (i.e. one not created by the
pthread_create() function) uses a POSIX threads feature such as thread
private data or pushes a cleanup handler, the <pthreads> library creates
resources on behalf of that task as well.

Asynchronous thread cancellation is accomplished by way of a signal. A
special signal, SIGCANCEL, has been set aside in this version of VxWorks for
this purpose. Applications should take care not to block or handle SIGCANCEL.

SUMMARY MATRIX

.TS
tab (|);
l l l.
'<pthread> function' | 'Implemented?' | 'Note(s)'
_
pthread_attr_destroy | Yes |
pthread_attr_getdetachstate | Yes |
pthread_attr_getinheritsched | Yes |
pthread_attr_getschedparam | Yes |
pthread_attr_getschedpolicy | Yes |
pthread_attr_getscope | Yes |
pthread_attr_getstackaddr | Yes |
pthread_attr_getstacksize | Yes |
pthread_attr_init | Yes |
pthread_attr_setdetachstate | Yes |
pthread_attr_setinheritsched | Yes |
pthread_attr_setschedparam | Yes |
pthread_attr_setschedpolicy | Yes |
pthread_attr_setscope | Yes | 2
pthread_attr_setstackaddr | Yes |
pthread_attr_setstacksize | Yes |
pthread_atfork | No | 1
pthread_cancel | Yes | 5
pthread_cleanup_pop | Yes |
pthread_cleanup_push | Yes |
pthread_condattr_destroy | Yes |
pthread_condattr_getpshared | No | 3
pthread_condattr_init | Yes |
pthread_condattr_setpshared | No | 3
pthread_cond_broadcast | Yes |
pthread_cond_destroy | Yes |
pthread_cond_init | Yes |
pthread_cond_signal | Yes |
pthread_cond_timedwait | Yes |
pthread_cond_wait | Yes |
pthread_create | Yes |
pthread_detach | Yes |
pthread_equal | Yes |
pthread_exit | Yes |
pthread_getschedparam | Yes | 4
pthread_getspecific | Yes |
pthread_join | Yes |
pthread_key_create | Yes |
pthread_key_delete | Yes |
pthread_kill | Yes |
pthread_once | Yes |
pthread_self | Yes |
pthread_setcancelstate | Yes |
pthread_setcanceltype | Yes |
pthread_setschedparam | Yes | 4
pthread_setspecific | Yes |
pthread_sigmask | Yes |
pthread_testcancel | Yes |
pthread_mutexattr_destroy | Yes |
pthread_mutexattr_getprioceiling | Yes |
pthread_mutexattr_getprotocol | Yes |
pthread_mutexattr_getpshared | No | 3
pthread_mutexattr_init | Yes |
pthread_mutexattr_setprioceiling | Yes |
pthread_mutexattr_setprotocol | Yes |
pthread_mutexattr_setpshared | No | 3
pthread_mutex_destroy | Yes |
pthread_mutex_getprioceiling | Yes |
pthread_mutex_init | Yes |
pthread_mutex_lock | Yes |
pthread_mutex_setprioceiling | Yes |
pthread_mutex_trylock | Yes |
pthread_mutex_unlock | Yes |
getlogin_r | No | 6
getgrgid_r | No | 6
getpwnam_r | No | 6
getpwuid_r | No | 6
.TE

NOTES
.iP 1 4
The pthread_atfork() function is not implemented since fork() is not
implemented in VxWorks.
.iP 2
The contention scope thread scheduling attribute is always
PTHREAD_SCOPE_SYSTEM, since threads (i.e. tasks) contend for resources
with all other threads in the system.
.iP 3
The routines pthread_condattr_getpshared(), pthread_attr_setpshared(),
pthread_mutexattr_getpshared() and pthread_mutexattr_setpshared() are not
supported, since these interfaces describe how condition variables and
mutexes relate to a process, and VxWorks does not implement a process model.
.iP 4
The default scheduling policy is inherited from the current system setting.
The POSIX model of per-thread scheduling policies is not supported, since a
basic tenet of the design of VxWorks is a system-wide scheduling policy.
.iP 5
Thread cancellation is supported in appropriate <pthread> routines and
those routines already supported by VxWorks. However, the complete list of
cancellation points specified by POSIX is not supported because routines
such as msync(), fcntl(), tcdrain(), and wait() are not implemented by
VxWorks.
.iP 6
The routines getlogin_r(), getgrgid_r(), getpwnam_r(), and getpwuid_r() are
not implemented.
.LP

INCLUDE FILES: pthread.h

SEE ALSO: taskLib, semMLib, semPxLib,
<VxWorks Programmer's Guide: Multitasking>
*/


/* includes */

#include "vxWorks.h"
#include "semLib.h"
#include "taskLib.h"
#include "timers.h"
#include "taskArchLib.h"
#include "private/timerLibP.h"
#include "types/vxTypesOld.h"
#include "private/taskLibP.h"
#include "kernelLib.h"
#include "taskHookLib.h"
#include "signal.h"
#include "stdio.h"
#include "string.h"
#include "errno.h"
#include "memLib.h"
#include "fcntl.h"
#include "ioLib.h"
#include "logLib.h"
#include "sched.h"
#include "private/schedP.h"

#ifndef _WRS_VXWORKS_5_X
#include "private/pdLibP.h"
#include "pdLib.h"
#endif

#define __PTHREAD_SRC
#include "pthread.h"

#undef PTHREADS_DEBUG

#ifdef PTHREADS_DEBUG
#undef LOCAL
#define LOCAL
#endif

/* defines */

#define INIT_MUTEX(pMutex)						\
    do									\
	{								\
	if (!pthreadLibInited)						\
	    return (EINVAL);						\
	if ((pMutex)->mutexInitted == FALSE)				\
	    {								\
	    if (vxTas(&(pMutex)->mutexIniting) == TRUE)			\
		{							\
		taskLock();  /* may not be necessary */			\
									\
		(pMutex)->mutexSemId = semMCreate(			\
		      (pMutex->mutexAttr.mutexAttrProtocol ==		\
		       PTHREAD_PRIO_INHERIT ? SEM_INVERSION_SAFE : 0) |	\
		       SEM_Q_PRIORITY);					\
									\
		if ((pMutex)->mutexSemId == NULL)			\
		    {							\
		    taskUnlock ();					\
		    return (EINVAL);	/* internal error */		\
		    }							\
									\
		(pMutex)->mutexInitted = TRUE;				\
					   				\
		taskUnlock();	/* may not be necessary */		\
		}							\
	    else							\
		{							\
	        while ((pMutex)->mutexInitted == FALSE)			\
		    ;    /* pound sand */				\
		}							\
	    }								\
	}								\
    while (0)

#define INIT_COND(pCond)						\
    do									\
	{								\
	if (!pthreadLibInited)						\
	    return (EINVAL);						\
	if ((pCond)->condInitted == FALSE)				\
	    {								\
	    if (vxTas(&(pCond)->condIniting) == TRUE)			\
		{							\
		taskLock();  /* may not be necessary */			\
									\
		(pCond)->condSemId = semBCreate(SEM_Q_PRIORITY,		\
						SEM_EMPTY);		\
									\
		if ((pCond)->condSemId == NULL)				\
		    {							\
		    taskUnlock ();					\
		    return (EINVAL); /* internal error */		\
		    }							\
									\
		(pCond)->condInitted = TRUE;				\
									\
		taskUnlock(); /* may not be necessary */		\
		}							\
	    else							\
		{							\
	    	while ((pCond)->condInitted == FALSE)			\
		    ;    /* pound sand */				\
		}							\
	    }								\
	}								\
    while (0)

#ifndef _WRS_VXWORKS_5_X
#define VALID_PTHREAD(pThread, pLcb) \
		((pThread) && ((pThread)->flags & VALID) && \
		((pLcb) = taskLcbGet((pThread)->taskId)) && \
		(pLcb)->pPthread && \
		(pLcb)->pPthread == (pThread))
#else
#define VALID_PTHREAD(pThread, pTcb) \
		((pThread) && ((pThread)->flags & VALID) && \
		((pTcb) = (WIND_TCB *)(pThread)->taskId) && \
		(pTcb)->pPthread && \
		(pTcb)->pPthread == (pThread))
#endif


#define MY_PTHREAD ((internalPthread *)(taskIdCurrent->pPthread))
#define LOCK_PTHREAD(pThread)   semTake((pThread)->mutexSemId, WAIT_FOREVER)
#define UNLOCK_PTHREAD(pThread) semGive((pThread)->mutexSemId)
#define CANCEL_LOCK(pThread)    semTake((pThread)->cancelSemId, WAIT_FOREVER)
#define CANCEL_UNLOCK(pThread)  semGive((pThread)->cancelSemId)

/* numeric boundary check for priority. no mention of meaning of value */
#define PRIO_LOWER_BOUND 0
#define PRIO_UPPER_BOUND 255
#define VALID_PRIORITY(pri)     (PRIO_LOWER_BOUND <= (pri) && \
				 (pri) <= PRIO_UPPER_BOUND)

#define DEF_PRIORITY	(PRIO_LOWER_BOUND + \
		      		(PRIO_UPPER_BOUND - PRIO_LOWER_BOUND) / 2)

/*
 * Make sure we free memory from the PD's heap, even when called from a kernel
 * task, or a delete hook...
 */

/* typedefs */

typedef struct
    {
    pthread_mutex_t    mutex;
    long               count;
    void               (*destructor)();
    } pthread_key;

/* globals */

/*
 * This is a common. DO NOT INITIALISE IT - doing so will break the scalability
 * that relies on its status as a common.
 */

FUNCPTR _pthread_setcanceltype;

/* locals */

LOCAL pthread_key	key_table[_POSIX_THREAD_KEYS_MAX];
LOCAL SEM_ID		key_mutex		= NULL;
LOCAL BOOL		pthreadLibInited	= FALSE;

/* The default mutex, condition variable and thread creation attribute objects
 * and initializers - either POSIX or implementation-defined.
 * Used by:
 *  pthread_mutex_init()
 *  pthread_cond_init()
 *  pthread_create()
 *
 * For mutexes and condition variables,
 * only the 'process-shared' attribute is defined.
 */

LOCAL pthread_mutexattr_t defaultMutexAttr =
    {
    PTHREAD_INITIALIZED, PTHREAD_PRIO_INHERIT, 0
    };

/* forward declarations */

LOCAL void self_become_pthread(void);
LOCAL void pthreadDeleteTask (internalPthread * pThread);
LOCAL void threadExitCleanup (internalPthread * pThread);
LOCAL void cleanupPrivateData (internalPthread * pThread);

/* externals */

IMPORT BOOL	vxTas (void *addr);

/*******************************************************************************
*
* pthreadLibInit - initialize POSIX threads support
*
* This routine initializes the POSIX threads (<pthreads>) support for
* VxWorks. It should be called before any POSIX threads functions are used;
* normally it will be called as part of the kernel's initialization sequence.
*
* RETURNS: N/A
*/

void pthreadLibInit (void)
    {
    if (_pthreadLibInit ((FUNCPTR) pthreadDeleteTask) != ERROR)
	{
	/* Create the thread specific data key */

	key_mutex = semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE);

	/*
	 * Initialise the function pointer for user level access to this
	 * feature.
	 */

	_pthread_setcanceltype = (FUNCPTR) pthread_setcanceltype;

	pthreadLibInited = TRUE;
	}
    }

/*
 *  Section 3 - Process Primitives
 */

/*******************************************************************************
*
* pthread_sigmask - change and/or examine calling thread's signal mask (POSIX)
*
* This routine changes the signal mask for the calling thread as described
* by the <how> and <set> arguments. If <oset> is not NULL, the previous
* signal mask is stored in the location pointed to by it.
*
* The value of <how> indicates the manner in which the set is changed and
* consists of one of the following defined in 'signal.h':
* .iP SIG_BLOCK 4
* The resulting set is the union of the current set and the signal set
* pointed to by <set>.
* .iP SIG_UNBLOCK
* The resulting set is the intersection of the current set and the complement
* of the signal set pointed to by <set>.
* .iP SIG_SETMASK
* The resulting set is the signal set pointed to by <oset>.
* .LP
*
* RETURNS: On success zero; on failure a non-zero error code is returned.
*
* ERRNOS: 'EINVAL'
*
* SEE ALSO: kill(), pthread_kill(), sigprocmask(), sigaction(),
* sigsuspend(), sigwait()
*/

int pthread_sigmask
    (
    int			how,		/* method for changing set	*/
    const sigset_t *	set,		/* new set of signals		*/
    sigset_t *		oset		/* old set of signals		*/
    )
    {
    if (sigprocmask(how, (sigset_t *) set, oset) == ERROR)
	return (errno);

    return (_RETURN_PTHREAD_SUCCESS);
    }

/*******************************************************************************
*
* pthread_kill - send a signal to a thread (POSIX)
*
* This routine sends signal number <sig> to the thread specified by <thread>.
* The signal is delivered and handled as described for the kill() function.
*
* RETURNS: On success zero; on failure a non-zero error code.
*
* ERRNOS: 'ESRCH', 'EINVAL'
*
* SEE ALSO: kill(), pthread_sigmask(), sigprocmask(), sigaction(),
* sigsuspend(), sigwait()
*/

int pthread_kill
    (
    pthread_t thread,			/* thread to signal */
    int sig				/* signal to send */
    )
    {
    internalPthread *	pThread = (internalPthread *)thread;
    WIND_TCB *		pTcb;

    if (!VALID_PTHREAD(pThread, pTcb))
	return (ESRCH);

    if (sig < 0 || sig >_NSIGS)
	return (EINVAL);

    if (sig == 0 || kill(((int)pThread->taskId), sig) == OK)
	return (_RETURN_PTHREAD_SUCCESS);
    else
	return (errno);
    }

/*
 * Section 11.3 - Mutexes
 */

/*******************************************************************************
*
* pthread_mutexattr_init - initialize mutex attributes object (POSIX)
*
* This routine initializes the mutex attribute object <pAttr> and fills it
* with  default values for the attributes as defined by the POSIX
* specification.
*
* RETURNS: On success zero; on failure a non-zero error code.
*
* ERRNOS: 'EINVAL'
*
* SEE ALSO:
* pthread_mutexattr_destroy(),
* pthread_mutexattr_getprioceiling(),
* pthread_mutexattr_getprotocol(),
* pthread_mutexattr_setprioceiling(),
* pthread_mutexattr_setprotocol(),
* pthread_mutex_init()
*/

int pthread_mutexattr_init
    (
    pthread_mutexattr_t *pAttr		/* mutex attributes */
    )
    {
    if (!pAttr)
	return (EINVAL);

    pAttr->mutexAttrStatus = PTHREAD_INITIALIZED;

    return (_RETURN_PTHREAD_SUCCESS);
    }

/*******************************************************************************
*
* pthread_mutexattr_destroy - destroy mutex attributes object (POSIX)
*
* This routine destroys a mutex attribute object. The mutex attribute object
* must not be reused until it is reinitialized.
*
* RETURNS: On success zero; on failure a non-zero error code.
*
* ERRNOS: 'EINVAL'
*
* SEE ALSO:
* pthread_mutexattr_getprioceiling(),
* pthread_mutexattr_getprotocol(),
* pthread_mutexattr_init(),
* pthread_mutexattr_setprioceiling(),
* pthread_mutexattr_setprotocol(),
* pthread_mutex_init()
*/

int pthread_mutexattr_destroy
    (
    pthread_mutexattr_t *pAttr		/* mutex attributes */
    )
    {
    if (!pAttr)
	return (EINVAL);

    pAttr->mutexAttrStatus = PTHREAD_DESTROYED;

    return (_RETURN_PTHREAD_SUCCESS);
    }

/*******************************************************************************
*
* pthread_mutexattr_setprotocol - set protocol attribute in mutex attribut object (POSIX)
*
* This function selects the locking protocol to be used when a mutex is created
* using this attributes object. The protocol to be selected is either
* PTHREAD_PRIO_INHERIT or PTHREAD_PRIO_PROTECT.
*
* RETURNS: On success zero; on failure a non-zero error code.
*
* ERRNOS: 'EINVAL', 'ENOTSUP'
*
* SEE ALSO:
* pthread_mutexattr_destroy(),
* pthread_mutexattr_getprioceiling(),
* pthread_mutexattr_getprotocol(),
* pthread_mutexattr_init(),
* pthread_mutexattr_setprioceiling(),
* pthread_mutex_init()
*/

int pthread_mutexattr_setprotocol
    (
    pthread_mutexattr_t *pAttr,		/* mutex attributes	*/
    int protocol			/* new protocol		*/
    )
    {
    if (!pAttr || pAttr->mutexAttrStatus != PTHREAD_INITIALIZED)
	return (EINVAL);

    if (protocol != PTHREAD_PRIO_NONE && protocol != PTHREAD_PRIO_INHERIT &&
        protocol != PTHREAD_PRIO_PROTECT)
	{
	return (ENOTSUP);
	}

    pAttr->mutexAttrProtocol = protocol;

    return (_RETURN_PTHREAD_SUCCESS);
    }

/*******************************************************************************
*
* pthread_mutexattr_getprotocol - get value of protocol in mutex attributes object (POSIX)
*
* This function gets the current value of the protocol attribute in a mutex
* attributes object.
*
* RETURNS: On success zero; on failure a non-zero error code.
*
* ERRNOS: 'EINVAL'
*
* SEE ALSO:
* pthread_mutexattr_destroy(),
* pthread_mutexattr_getprioceiling(),
* pthread_mutexattr_init(),
* pthread_mutexattr_setprioceiling(),
* pthread_mutexattr_setprotocol(),
* pthread_mutex_init()
*/

int pthread_mutexattr_getprotocol
    (
    pthread_mutexattr_t *pAttr,		/* mutex attributes		*/
    int *pProtocol			/* current protocol (out)	*/
    )
    {
    if (!pAttr || pAttr->mutexAttrStatus != PTHREAD_INITIALIZED)
	return (EINVAL);

    *pProtocol = pAttr->mutexAttrProtocol;

    return (_RETURN_PTHREAD_SUCCESS);
    }

/*******************************************************************************
*
* pthread_mutexattr_setprioceiling - set prioceiling attribute in mutex attributes object (POSIX)
*
* This function sets the value of the prioceiling attribute in a mutex
* attributes object. Unless the protocol attribute is set to
* PTHREAD_PRIO_PROTECT, this attribute is ignored.
*
* RETURNS: On success zero; on failure a non-zero error code.
*
* ERRNOS: 'EINVAL'
*
* SEE ALSO:
* pthread_mutexattr_destroy(),
* pthread_mutexattr_getprioceiling(),
* pthread_mutexattr_getprotocol(),
* pthread_mutexattr_init(),
* pthread_mutexattr_setprotocol(),
* pthread_mutex_init()
*/

int pthread_mutexattr_setprioceiling
    (
    pthread_mutexattr_t *pAttr,		/* mutex attributes	*/
    int prioceiling			/* new priority ceiling	*/
    )
    {
    if (!pAttr || pAttr->mutexAttrStatus != PTHREAD_INITIALIZED ||
	(prioceiling < PRIO_LOWER_BOUND || prioceiling > PRIO_UPPER_BOUND))
	{
	return (EINVAL);
	}

    pAttr->mutexAttrPrioceiling = prioceiling;

    return (_RETURN_PTHREAD_SUCCESS);
    }

/*******************************************************************************
*
* pthread_mutexattr_getprioceiling - get the current value of the prioceiling attribute in a mutex attributes object (POSIX)
*
* This function gets the current value of the prioceiling attribute in a mutex
* attributes object. Unless the value of the protocol attribute is
* PTHREAD_PRIO_PROTECT, this value is ignored.
*
* RETURNS: On success zero; on failure a non-zero error code.
*
* ERRNOS: 'EINVAL'
*
* SEE ALSO:
* pthread_mutexattr_destroy(),
* pthread_mutexattr_getprotocol(),
* pthread_mutexattr_init(),
* pthread_mutexattr_setprioceiling(),
* pthread_mutexattr_setprotocol(),
* pthread_mutex_init()
*/

int pthread_mutexattr_getprioceiling
    (
    pthread_mutexattr_t *pAttr,		/* mutex attributes		  */
    int *pPrioceiling			/* current priority ceiling (out) */
    )
    {
    if (!pAttr || pAttr->mutexAttrStatus != PTHREAD_INITIALIZED)
	return (EINVAL);

    *pPrioceiling = pAttr->mutexAttrPrioceiling;

    return (_RETURN_PTHREAD_SUCCESS);
    }

/*******************************************************************************
*
* pthread_mutex_getprioceiling - get the value of the prioceiling attribute of a mutex (POSIX)
*
* This function gets the current value of the prioceiling attribute of a mutex.
* Unless the mutex was created with a protocol attribute value of
* PTHREAD_PRIO_PROTECT, this value is meaningless.
*
* RETURNS: On success zero; on failure a non-zero error code.
*
* ERRNOS: 'EINVAL'
*
* SEE ALSO:
* pthread_mutex_setprioceiling(),
* pthread_mutexattr_getprioceiling(),
* pthread_mutexattr_setprioceiling()
*/

int pthread_mutex_getprioceiling
    (
    pthread_mutex_t *pMutex,		/* POSIX mutex			  */
    int *pPrioceiling			/* current priority ceiling (out) */
    )
    {
    if (!pMutex || pMutex->mutexValid != TRUE)
	return (EINVAL);

    INIT_MUTEX(pMutex);

    *pPrioceiling = pMutex->mutexAttr.mutexAttrPrioceiling;

    return (_RETURN_PTHREAD_SUCCESS);
    }

/*******************************************************************************
*
* pthread_mutex_setprioceiling - dynamically set the prioceiling attribute of a mutex (POSIX)
*
* This function dynamically sets the value of the prioceiling attribute of a
* mutex. Unless the mutex was created with a protocol value of
* PTHREAD_PRIO_PROTECT, this function does nothing.
*
* RETURNS: On success zero; on failure a non-zero error code.
*
* ERRNOS: 'EINVAL', 'EPERM', S_objLib_OBJ_ID_ERROR, S_semLib_NOT_ISR_CALLABLE
*
* SEE ALSO:
* pthread_mutex_getprioceiling(),
* pthread_mutexattr_getprioceiling(),
* pthread_mutexattr_setprioceiling()
*/

int pthread_mutex_setprioceiling
    (
    pthread_mutex_t *pMutex,		/* POSIX mutex			*/
    int prioceiling,			/* new priority ceiling		*/
    int *pOldPrioceiling		/* old priority ceiling (out)	*/
    )
    {
    if (!pMutex || pMutex->mutexValid != TRUE)
	return (EINVAL);

    INIT_MUTEX(pMutex);

    if (semTake(pMutex->mutexSemId, WAIT_FOREVER) == ERROR)
	return (EINVAL);			/* internal error */

    *pOldPrioceiling = pMutex->mutexAttr.mutexAttrPrioceiling;
    pMutex->mutexAttr.mutexAttrPrioceiling = prioceiling;

    /* Clear errno before trying to test it */

    errno = 0;

    if (semGive(pMutex->mutexSemId) == ERROR)
	{
	if (errno == S_semLib_INVALID_OPERATION)
	    return (EPERM);			/* not owner - can't happen */
	else
	    return (errno);			/* some other error */
	}

    return (_RETURN_PTHREAD_SUCCESS);
    }

/*******************************************************************************
*
* pthread_mutex_init - initialize mutex from attributes object (POSIX)
*
* This routine initializes the mutex object pointed to by <pMutex> according
* to the mutex attributes specified in <pAttr>.  If <pAttr> is NULL, default
* attributes are used as defined in the POSIX specification.
*
* RETURNS: On success zero; on failure a non-zero error code.
*
* ERRNOS: 'EINVAL', 'EBUSY'
*
* SEE ALSO:
* semLib, semMLib,
* pthread_mutex_destroy(),
* pthread_mutex_lock(),
* pthread_mutex_trylock(),
* pthread_mutex_unlock(),
* pthread_mutexattr_init(),
* semMCreate()
*/

int pthread_mutex_init
    (
    pthread_mutex_t *pMutex,		/* POSIX mutex			*/
    const pthread_mutexattr_t *pAttr	/* mutex attributes		*/
    )
    {
    pthread_mutexattr_t *pSource;

    if (!pMutex)
	return (EINVAL);

    if (!pthreadLibInited)
	return (EINVAL);

    if (pMutex->mutexValid == TRUE)
	return (EBUSY);

    pSource = pAttr ? (pthread_mutexattr_t *)pAttr : &defaultMutexAttr;

    bcopy((const char *)pSource, (char *)&pMutex->mutexAttr,
	  sizeof (pthread_mutexattr_t));

    pMutex->mutexValid		= TRUE;
    pMutex->mutexInitted	= FALSE;
    pMutex->mutexIniting	= 0;
    pMutex->mutexCondRefCount	= 0;

    return (_RETURN_PTHREAD_SUCCESS);
    }

/*******************************************************************************
*
* pthread_mutex_destroy - destroy a mutex (POSIX)
*
* This routine destroys a mutex object, freeing the resources it might hold.
* The mutex must be unlocked when this function is called, otherwise it will
* return an error ('EBUSY').
*
* RETURNS: On success zero; on failure a non-zero error code.
*
* ERRNOS: 'EINVAL', 'EBUSY'
*
* SEE ALSO:
* semLib, semMLib,
* pthread_mutex_init(),
* pthread_mutex_lock(),
* pthread_mutex_trylock(),
* pthread_mutex_unlock(),
* pthread_mutexattr_init(),
* semDelete()
*/

int pthread_mutex_destroy
    (
    pthread_mutex_t *pMutex		/* POSIX mutex		*/
    )
    {
    if (!pMutex || pMutex->mutexValid != TRUE)
	return (EINVAL);

    if (_pthreadSemOwnerGet (pMutex->mutexSemId) || pMutex->mutexCondRefCount)
	return (EBUSY);

    INIT_MUTEX(pMutex);

    pthread_mutexattr_destroy(&(pMutex->mutexAttr));

    if (semDelete(pMutex->mutexSemId) == ERROR)
	return (EINVAL);

    pMutex->mutexValid		= FALSE;

    return (_RETURN_PTHREAD_SUCCESS);
    }

/*******************************************************************************
*
* deadlock - simulate a deadlocked mutex
*
* Unlike mutex semaphores, POSIX mutexes deadlock if the owner tries to acquire
* a mutex more than once. This routine ensures that the calling thread will
* come to a grinding halt. This routine will only get called if there is an
* application programming error.
*
* NOMANUAL
*/

LOCAL void deadlock(void)
    {
    taskSuspend(0);
    }

/*******************************************************************************
*
* pthread_mutex_lock - lock a mutex (POSIX)
*
* This routine locks the mutex specified by <pMutex>. If the mutex is
* currently unlocked, it becomes locked, and is said to be owned by the
* calling thread. In this case pthread_mutex_lock() returns immediately.
*
* If the mutex is already locked by another thread, pthread_mutex_lock()
* blocks the calling thread until the mutex is unlocked by its current owner.
*
* If it is already locked by the calling thread, pthread_mutex_lock will
* deadlock on itself and the thread will block indefinitely.
*
* RETURNS: On success zero; on failure a non-zero error code.
*
* ERRNOS: 'EINVAL'
*
* SEE ALSO:
* semLib, semMLib,
* pthread_mutex_init(),
* pthread_mutex_lock(),
* pthread_mutex_trylock(),
* pthread_mutex_unlock(),
* pthread_mutexattr_init(),
* semTake()
*/

int pthread_mutex_lock
    (
    pthread_mutex_t *pMutex		/* POSIX mutex		*/
    )
    {
    WIND_TCB *	pTcb		= taskIdCurrent;
    int		priority;

    if (!pMutex || pMutex->mutexValid != TRUE)
	return (EINVAL);

    INIT_MUTEX(pMutex);

    taskPriorityGet((int)pTcb, &priority);

    if ((pMutex->mutexAttr.mutexAttrProtocol == PTHREAD_PRIO_PROTECT) &&
        (priority <
		PX_VX_PRIORITY_CONVERT(pMutex->mutexAttr.mutexAttrPrioceiling)))
	{
	return (EINVAL);		/* would violate priority ceiling */
	}

#if 0
    /* this works on an mv2700, but not a Dy4 SVME/DMV-179 */

    /* make sure that we're NOT a cancellation point */
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &savstate);
#endif

    /*
     * Plain old POSIX mutexes deadlock if the owner of a lock tries to
     * acquire it again. The Open Group's Unix98 standard as well as POSIX
     * P1003.1j Draft standard allow different lock types that at this point
     * would have different behavior - an error checking mutex would return
     * EDEADLK, and a recursive lock would bump a ref count and return
     * success.
     */

    if (_pthreadSemOwnerGet (pMutex->mutexSemId) == (int) taskIdCurrent)
        deadlock();

    if (semTake(pMutex->mutexSemId, WAIT_FOREVER) == ERROR)
	return (EINVAL);		/* internal error */

#if 0
    /* restore cancellation state */

    pthread_setcancelstate(savstate, NULL);
#endif

    if (pMutex->mutexAttr.mutexAttrProtocol == PTHREAD_PRIO_PROTECT)
	{
	pMutex->mutexSavPriority = priority;
	taskPrioritySet ((int)pTcb,
	      PX_VX_PRIORITY_CONVERT(pMutex->mutexAttr.mutexAttrPrioceiling));
	}

    return (_RETURN_PTHREAD_SUCCESS);
    }

/*******************************************************************************
*
* pthread_mutex_trylock - lock mutex if it is available (POSIX)
*
* This routine locks the mutex specified by <pMutex>. If the mutex is
* currently unlocked, it becomes locked and owned by the calling thread. In
* this case pthread_mutex_trylock() returns immediately.
*
* If the mutex is already locked by another thread, pthread_mutex_trylock()
* returns immediately with the error code 'EBUSY'.
*
* RETURNS: On success zero; on failure a non-zero error code.
*
* ERRNOS: 'EINVAL', 'EBUSY'
*
* SEE ALSO:
* semLib, semMLib,
* pthread_mutex_init(),
* pthread_mutex_lock(),
* pthread_mutex_trylock(),
* pthread_mutex_unlock(),
* pthread_mutexattr_init(),
* semTake()
*/

int pthread_mutex_trylock
    (
    pthread_mutex_t *pMutex		/* POSIX mutex		*/
    )
    {
    WIND_TCB *pTcb = taskIdCurrent;
    int priority;

    if (!pMutex || pMutex->mutexValid != TRUE)
	return (EINVAL);

    INIT_MUTEX(pMutex);

    taskPriorityGet((int)pTcb, &priority);

    if ((pMutex->mutexAttr.mutexAttrProtocol == PTHREAD_PRIO_PROTECT) &&
	(priority <
	       PX_VX_PRIORITY_CONVERT(pMutex->mutexAttr.mutexAttrPrioceiling)))
	{
	return (EINVAL);		/* would violate priority ceiling */
	}

    if (_pthreadSemOwnerGet (pMutex->mutexSemId) == (int) taskIdCurrent)
	return(EBUSY);

    if (semTake(pMutex->mutexSemId, NO_WAIT) == ERROR)
	return (EBUSY);			/* acquire failed */

    if (pMutex->mutexAttr.mutexAttrProtocol == PTHREAD_PRIO_PROTECT)
	{
	pMutex->mutexSavPriority = priority;
	taskPrioritySet ((int)pTcb,
		PX_VX_PRIORITY_CONVERT(pMutex->mutexAttr.mutexAttrPrioceiling));
	}

    return (_RETURN_PTHREAD_SUCCESS);
    }

/*******************************************************************************
*
* pthread_mutex_unlock - unlock a mutex (POSIX)
*
* This routine unlocks the mutex specified by <pMutex>. If the calling thread
* is not the current owner of the mutex, pthread_mutex_unlock() returns with
* the error code 'EPERM'.
*
* RETURNS: On success zero; on failure a non-zero error code.
*
* ERRNOS: 'EINVAL', 'EPERM', S_objLib_OBJ_ID_ERROR, S_semLib_NOT_ISR_CALLABLE
*
* SEE ALSO:
* semLib, semMLib,
* pthread_mutex_init(),
* pthread_mutex_lock(),
* pthread_mutex_trylock(),
* pthread_mutex_unlock(),
* pthread_mutexattr_init(),
* semGive()
*/

int pthread_mutex_unlock
    (
    pthread_mutex_t *pMutex
    )
    {
    WIND_TCB *pTcb = taskIdCurrent;

    if (!pMutex || pMutex->mutexValid != TRUE)
	return (EINVAL);

    INIT_MUTEX(pMutex);

    if (pMutex->mutexAttr.mutexAttrProtocol == PTHREAD_PRIO_PROTECT)
	taskPrioritySet((int)pTcb, pMutex->mutexSavPriority);

    errno = 0;

    if (semGive(pMutex->mutexSemId) == ERROR)
	{
	if (errno == S_semLib_INVALID_OPERATION)
	    return (EPERM);                                      /* not owner */
	else
	    return (errno);                               /* some other error */
	}

    return (_RETURN_PTHREAD_SUCCESS);
    }

/*
 * Section 11.4 - Condition Variables
 */

/*******************************************************************************
*
* pthread_condattr_init - initialize a condition attribute object (POSIX)
*
* This routine initializes the condition attribute object <pAttr> and fills
* it with default values for the attributes.
*
* RETURNS: On success zero; on failure a non-zero error code.
*
* ERRNOS: 'EINVAL'
*
* SEE ALSO:
* pthread_cond_init(),
* pthread_condattr_destroy()
*/

int pthread_condattr_init
    (
    pthread_condattr_t *pAttr		/* condition variable attributes */
    )
    {
    if (!pAttr)
	return (EINVAL);

    pAttr->condAttrStatus = PTHREAD_INITIALIZED;

    return (_RETURN_PTHREAD_SUCCESS);
    }

/*******************************************************************************
*
* pthread_condattr_destroy - destroy a condition attributes object (POSIX)
*
* This routine destroys the condition attribute object <pAttr>. It must
* not be reused until it is reinitialized.
*
* RETURNS: Always returns zero.
*
* ERRNOS: None.
*
* SEE ALSO:
* pthread_cond_init(),
* pthread_condattr_init()
*/

int pthread_condattr_destroy
    (
    pthread_condattr_t *pAttr		/* condition variable attributes */
    )
    {
    pAttr->condAttrStatus = PTHREAD_DESTROYED;

    return (_RETURN_PTHREAD_SUCCESS);
    }

/*******************************************************************************
*
* pthread_cond_init - initialize condition variable (POSIX)
*
* DESCRIPTION
*
* This function initializes a condition variable. A condition variable
* is a synchronization device that allows threads to block until some
* predicate on shared data is satisfied. The basic operations on conditions
* are to signal the condition (when the predicate becomes true), and wait
* for the condition, blocking the thread until another thread signals the
* condition.
*
* A condition variable must always be associated with a mutex to avoid a
* race condition between the wait and signal operations.
*
* If <pAttr> is NULL then the default attributes are used as specified by
* POSIX; if <pAttr> is non-NULL then it is assumed to point to a condition
* attributes object initialized by pthread_condattr_init(), and those are
* the attributes used to create the condition variable.
*
* RETURNS: On success zero; on failure a non-zero error code.
*
* ERRNOS: 'EINVAL', 'EBUSY'
*
* SEE ALSO:
* pthread_condattr_init(),
* pthread_condattr_destroy(),
* pthread_cond_broadcast(),
* pthread_cond_destroy(),
* pthread_cond_signal(),
* pthread_cond_timedwait(),
* pthread_cond_wait()
*/

int pthread_cond_init
    (
    pthread_cond_t *pCond,		/* condition variable		 */
    pthread_condattr_t *pAttr		/* condition variable attributes */
    )
    {
    if (!pCond)
	return (EINVAL);

    if (!pthreadLibInited)
	return (EINVAL);

    /* check for attempt to reinitialize cond - it's already intialized */

    if (pCond->condValid == TRUE)
	return (EBUSY);

    /* in VxWorks, there is nothing useful in a pthread_condattr_t */

    pCond->condValid	= TRUE;
    pCond->condInitted	= FALSE;
    pCond->condIniting	= 0;
    pCond->condMutex	= NULL;
    pCond->condRefCount	= 0;

    return (_RETURN_PTHREAD_SUCCESS);
    }

/*******************************************************************************
*
* pthread_cond_destroy - destroy a condition variable (POSIX)
*
* This routine destroys the condition variable pointed to by <pCond>. No
* threads can be waiting on the condition variable when this function is
* called. If there are threads waiting on the condition variable, then
* pthread_cond_destroy() returns 'EBUSY'.
*
* RETURNS: On success zero; on failure a non-zero error code.
*
* ERRNOS: 'EINVAL', 'EBUSY'
*
* SEE ALSO:
* pthread_condattr_init(),
* pthread_condattr_destroy(),
* pthread_cond_broadcast(),
* pthread_cond_init(),
* pthread_cond_signal(),
* pthread_cond_timedwait(),
* pthread_cond_wait()
*/

int pthread_cond_destroy
    (
    pthread_cond_t *pCond		/* condition variable */
    )
    {
    if (!pCond || pCond->condValid != TRUE)
	return (EINVAL);

    if (pCond->condRefCount != 0)
	return (EBUSY); /* someone else blocked on *pCond */

    INIT_COND(pCond);

    if (semDelete(pCond->condSemId) == ERROR)
	return (errno);

    return (_RETURN_PTHREAD_SUCCESS);
    }

/*******************************************************************************
*
* pthread_cond_signal - unblock a thread waiting on a condition (POSIX)
*
* This routine unblocks one thread waiting on the specified condition
* variable <pCond>. If no threads are waiting on the condition variable then
* this routine does nothing; if more than one thread is waiting, then one will
* be released, but it is not specified which one.
*
* RETURNS: On success zero; on failure a non-zero error code.
*
* ERRNOS: 'EINVAL'
*
* SEE ALSO:
* pthread_condattr_init(),
* pthread_condattr_destroy(),
* pthread_cond_broadcast(),
* pthread_cond_destroy(),
* pthread_cond_init(),
* pthread_cond_timedwait(),
* pthread_cond_wait()
*/

int pthread_cond_signal
    (
    pthread_cond_t *pCond
    )
    {
    if (!pCond || pCond->condValid != TRUE)
	return (EINVAL);

    INIT_COND(pCond);

    if (semGive(pCond->condSemId) == ERROR)
	return (EINVAL);                                   /* internal error */

    return (_RETURN_PTHREAD_SUCCESS);
    }

/*******************************************************************************
*
* pthread_cond_broadcast - unblock all threads waiting on a condition (POSIX)
*
* This function unblocks all threads blocked on the condition variable
* <pCond>. Nothing happens if no threads are waiting on the specified condition
* variable.
*
* RETURNS: On success zero; on failure a non-zero error code.
*
* ERRNOS: 'EINVAL'
*
* SEE ALSO:
* pthread_condattr_init(),
* pthread_condattr_destroy(),
* pthread_cond_destroy(),
* pthread_cond_init(),
* pthread_cond_signal(),
* pthread_cond_timedwait(),
* pthread_cond_wait()
*/

int pthread_cond_broadcast
    (
    pthread_cond_t *pCond
    )
    {
    int taskIdList[1];

    if (!pCond || pCond->condValid != TRUE)
	return (EINVAL);

    INIT_COND(pCond);

    /* get number of blocked tasks and give the semaphore for each */

    while (semInfo(pCond->condSemId, taskIdList, 1))
	{
	if (semGive(pCond->condSemId) == ERROR)
	    return (EINVAL);                             /* internal error */
	}

    return (_RETURN_PTHREAD_SUCCESS);
    }

/*******************************************************************************
*
* pthread_cond_wait - wait for a condition variable (POSIX)
*
* This function atomically releases the mutex <pMutex> and waits for the
* condition variable <pCond> to be signalled by another thread. The mutex
* must be locked by the calling thread when pthread_cond_wait() is called; if
* it is not then this function returns an error ('EINVAL').
*
* Before returning to the calling thread, pthread_cond_wait() re-acquires the
* mutex.
*
* RETURNS: On success zero; on failure a non-zero error code.
*
* ERRNOS: 'EINVAL'
*
* SEE ALSO:
* pthread_condattr_init(),
* pthread_condattr_destroy(),
* pthread_cond_broadcast(),
* pthread_cond_destroy(),
* pthread_cond_init(),
* pthread_cond_signal(),
* pthread_cond_timedwait()
*/

int pthread_cond_wait
    (
    pthread_cond_t *pCond,		/* condition variable	*/
    pthread_mutex_t *pMutex		/* POSIX mutex		*/
    )
    {
    WIND_TCB *mutexTcb;
    int savtype;

    if (!pCond || pCond->condValid != TRUE)
	return (EINVAL);

    INIT_COND(pCond);

    if (!pMutex)
	return (EINVAL);

    /*
     * make sure that if anyone else is blocked on this condition variable
     * that the same mutex was used to guard it. Also verify that mutex is
     * locked and that we are the owner
     */

    if (((pCond->condMutex != NULL) && (pCond->condMutex != pMutex)) ||
	(!(mutexTcb = (WIND_TCB *) _pthreadSemOwnerGet (pMutex->mutexSemId))) ||
	(mutexTcb != taskIdCurrent))
	{
	return (EINVAL);
	}
    else
	pCond->condMutex = pMutex;

    pCond->condRefCount++;
    pCond->condMutex->mutexCondRefCount++;

    if (semGive(pMutex->mutexSemId) == ERROR)
	{
	return (EINVAL);
	}

    if (MY_PTHREAD)
        MY_PTHREAD->cvcond = pCond;

    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &savtype);

    if (semTake(pCond->condSemId, WAIT_FOREVER) == ERROR)
	{
	pthread_setcanceltype(savtype, NULL);
	return (EINVAL);                                   /* internal error */
	}

    pthread_setcanceltype(savtype, NULL);

    if (MY_PTHREAD)
        MY_PTHREAD->cvcond = NULL;

    if (semTake(pMutex->mutexSemId, WAIT_FOREVER) == ERROR)
	return (EINVAL);

    pCond->condMutex->mutexCondRefCount--;

    if (--pCond->condRefCount == 0)
	pCond->condMutex = NULL;

    return (_RETURN_PTHREAD_SUCCESS);
    }

/*******************************************************************************
*
* pthread_cond_timedwait - wait for a condition variable with a timeout (POSIX)
*
* This function atomically releases the mutex <pMutex> and waits for another
* thread to signal the condition variable <pCond>. As with pthread_cond_wait(),
* the mutex must be locked by the calling thread when pthread_cond_timedwait()
* is called.
*
* If the condition variable is signalled before the system time reaches the
* time specified by <pAbsTime>, then the mutex is re-acquired and the calling
* thread unblocked.
*
* If the system time reaches or exceeds the time specified by <pAbsTime> before
* the condition is signalled, then the mutex is re-acquired, the thread
* unblocked and ETIMEDOUT returned.
*
* NOTE
* The timeout is specified as an absolute value of the system clock in a
* <timespec> structure (see clock_gettime() for more information). This is
* different from most VxWorks timeouts which are specified in ticks relative
* to the current time.
*
* RETURNS: On success zero; on failure a non-zero error code.
*
* ERRNOS: 'EINVAL', 'ETIMEDOUT'
*
* SEE ALSO:
* pthread_condattr_init(),
* pthread_condattr_destroy(),
* pthread_cond_broadcast(),
* pthread_cond_destroy(),
* pthread_cond_init(),
* pthread_cond_signal(),
* pthread_cond_wait()
*/

int pthread_cond_timedwait
    (
    pthread_cond_t *pCond,		/* condition variable	*/
    pthread_mutex_t *pMutex,		/* POSIX mutex		*/
    const struct timespec *pAbstime	/* timeout time		*/
    )
    {
    int			nTicks;
    struct timespec	tmp;
    struct timespec	now;
    WIND_TCB *		mutexTcb;
    int			savtype;

    if (!pCond || pCond->condValid != TRUE)
	return (EINVAL);

    INIT_COND(pCond);

    if (!pMutex || !pAbstime || !TV_VALID(*pAbstime))
	{
	return (EINVAL);
	}

    /*
     * make sure that if anyone else is blocked on this condition variable
     * that the same mutex was used to guard it. Also verify that mutex is
     * locked and that we are the owner
     */

    if (((pCond->condMutex != NULL) && (pCond->condMutex != pMutex)) ||
	(!(mutexTcb = (WIND_TCB *) _pthreadSemOwnerGet (pMutex->mutexSemId))) ||
	(mutexTcb != taskIdCurrent))
	{
	return (EINVAL);
	}
    else
	pCond->condMutex = pMutex;

    pCond->condMutex->mutexCondRefCount++;
    pCond->condRefCount++;

    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &savtype);

    /*
     * convert to number of ticks (relative time) or return if time has
     * already passed
     */

    if (clock_gettime(CLOCK_REALTIME, &now) != 0)
        {
	pthread_setcanceltype(savtype, NULL);
	return (errno);
        }

    TV_SET(tmp, *pAbstime);
    TV_SUB(tmp, now);

    if (TV_GT(tmp, *pAbstime))    /* overflow, e.g. too late */
	{
	/* drop and reacquire mutex as per POSIX */

	if (semGive(pMutex->mutexSemId) == ERROR)
	    {
	    pthread_setcanceltype(savtype, NULL);
	    return (EINVAL);        /* internal error */
	    }

	if (semTake(pMutex->mutexSemId, WAIT_FOREVER) == ERROR)
	    {
	    pthread_setcanceltype(savtype, NULL);
	    return (EINVAL);        /* internal error */
	    }

	pthread_setcanceltype(savtype, NULL);
	return (ETIMEDOUT);
	}

    TV_CONVERT_TO_TICK(nTicks, tmp);

    if (semGive(pMutex->mutexSemId) == ERROR)
	{
	pthread_setcanceltype(savtype, NULL);
	return (EINVAL);
	}

    if (MY_PTHREAD)
        MY_PTHREAD->cvcond = pCond;

    if (semTake(pCond->condSemId, nTicks) == ERROR)
	{
	if (MY_PTHREAD)
            MY_PTHREAD->cvcond = NULL;

	pthread_setcanceltype(savtype, NULL);

	if (semTake(pMutex->mutexSemId, WAIT_FOREVER) == ERROR)
	    {
	    return (EINVAL);
	    }

	pCond->condMutex->mutexCondRefCount--;

	if (--pCond->condRefCount == 0)
	    pCond->condMutex = NULL;

	return (ETIMEDOUT);
	}

    pthread_setcanceltype(savtype, NULL);

    if (semTake(pMutex->mutexSemId, WAIT_FOREVER) == ERROR)
	{
	return (EINVAL);
	}

    if (MY_PTHREAD)
        MY_PTHREAD->cvcond = NULL;

    pCond->condMutex->mutexCondRefCount--;

    if (--pCond->condRefCount == 0)
	pCond->condMutex = NULL;

    return (_RETURN_PTHREAD_SUCCESS);
    }

/*
 * Section 13.5 - Thread Scheduling
 */

/*******************************************************************************
*
* pthread_attr_setscope - set contention scope for thread attributes (POSIX)
*
* For VxWorks PTHREAD_SCOPE_SYSTEM is the only supported contention scope.
* Any other value passed to this function will result in 'EINVAL' being
* returned.
*
* RETURNS: On success zero; on failure a non-zero error code.
*
* ERRNOS: 'EINVAL'
*
* SEE ALSO:
* pthread_attr_getscope(),
* pthread_attr_init()
*/

int pthread_attr_setscope
    (
    pthread_attr_t *pAttr,		/* thread attributes object	*/
    int contentionScope			/* new contention scope		*/
    )
    {
    if (!pAttr || pAttr->threadAttrStatus != PTHREAD_INITIALIZED ||
	(contentionScope != PTHREAD_SCOPE_PROCESS &&
	contentionScope != PTHREAD_SCOPE_SYSTEM))
	{
	return (EINVAL);
	}

#if 0
    /*
     * Restriction on VxWorks: no multiprocessor, so all threads
     * compete for resources, PTHREAD_SCOPE_SYSTEM always in effect
     */

    pAttr->threadAttrContentionscope = contentionScope;
#endif

    return (_RETURN_PTHREAD_SUCCESS);
    }

/*******************************************************************************
*
* pthread_attr_getscope - get contention scope from thread attributes (POSIX)
*
* Reads the current contention scope setting from a thread attributes object.
* For VxWorks this is always PTHREAD_SCOPE_SYSTEM. If the thread attributes
* object is uninitialized then 'EINVAL' will be returned. The contention
* scope is returned in the location pointed to by <pContentionScope>.
*
* RETURNS: On success zero; on failure a non-zero error code.
*
* ERRNOS: 'EINVAL'
*
* SEE ALSO:
* pthread_attr_init(),
* pthread_attr_setscope()
*/

int pthread_attr_getscope
    (
    const pthread_attr_t *pAttr,	/* thread attributes object	*/
    int *pContentionScope		/* contention scope (out)	*/
    )
    {
    if (!pAttr || pAttr->threadAttrStatus != PTHREAD_INITIALIZED)
	return (EINVAL);

#if 0

  *pContentionScope = pAttr->threadAttrContentionscope;

#else

  /*
   * Restriction on VxWorks: no multiprocessor, so all threads
   * compete for resources, PTHREAD_SCOPE_SYSTEM always in effect
   */

  *pContentionScope = PTHREAD_SCOPE_SYSTEM;

#endif

  return (_RETURN_PTHREAD_SUCCESS);
}

/*******************************************************************************
*
* pthread_attr_setinheritsched - set inheritsched attribute in thread attribute object (POSIX)
*
* This routine sets the scheduling inheritance to be used when creating a
* thread with the thread attributes object specified by <pAttr>.
*
* Possible values are:
* .iP PTHREAD_INHERIT_SCHED 4
* Inherit scheduling parameters from parent thread.
* .iP PTHREAD_EXPLICIT_SCHED
* Use explicitly provided scheduling parameters (i.e. those specified in the
* thread attributes object).
* .LP
*
* RETURNS: On success zero; on failure a non-zero error code.
*
* ERRNOS: 'EINVAL'
*
* SEE ALSO:
* pthread_attr_getinheritsched(),
* pthread_attr_init(),
* pthread_attr_setschedparam(),
* pthread_attr_setschedpolicy()
*/

int pthread_attr_setinheritsched
    (
    pthread_attr_t *pAttr,		/* thread attributes object	*/
    int inheritsched			/* inheritance mode		*/
    )
    {
    if (!pAttr || pAttr->threadAttrStatus != PTHREAD_INITIALIZED ||
	(inheritsched != PTHREAD_INHERIT_SCHED &&
	inheritsched != PTHREAD_EXPLICIT_SCHED))
	{
	return (EINVAL);
	}

    pAttr->threadAttrInheritsched = inheritsched;

    return (_RETURN_PTHREAD_SUCCESS);
    }

/*******************************************************************************
*
* pthread_attr_getinheritsched - get current value if inheritsched attribute in thread attributes object (POSIX)
*
* This routine gets the scheduling inheritance value from the thread
* attributes object <pAttr>.
*
* Possible values are:
* .iP PTHREAD_INHERIT_SCHED 4
* Inherit scheduling parameters from parent thread.
* .iP PTHREAD_EXPLICIT_SCHED
* Use explicitly provided scheduling parameters (i.e. those specified in the
* thread attributes object).
* .LP
*
* RETURNS: On success zero; on failure a non-zero error code.
*
* ERRNOS: 'EINVAL'
*
* SEE ALSO:
* pthread_attr_init(),
* pthread_attr_getschedparam(),
* pthread_attr_getschedpolicy()
* pthread_attr_setinheritsched()
*/

int pthread_attr_getinheritsched
    (
    const pthread_attr_t *pAttr,	/* thread attributes object	*/
    int *pInheritsched			/* inheritance mode (out)	*/
    )
    {
    if (!pAttr || pAttr->threadAttrStatus != PTHREAD_INITIALIZED)
	return (EINVAL);

    *pInheritsched = pAttr->threadAttrInheritsched;

    return (_RETURN_PTHREAD_SUCCESS);
    }

/*******************************************************************************
*
* pthread_attr_setschedpolicy - set schedpolicy attribute in thread attributes object (POSIX)
*
* Select the thread scheduling policy. The default scheduling policy is
* to inherit the current system setting. Unlike the POSIX model,
* scheduling policies under VxWorks are global. If a scheduling policy is
* being set explicitly, the PTHREAD_EXPLICIT_SCHED mode must be set (see
* pthread_attr_setinheritsched() for information), and the selected scheduling
* policy must match the global scheduling policy in place at
* the time; failure to do so will result in pthread_create() failing with
* the non-POSIX error 'ENOTTY'.
*
* POSIX defines the following policies:
* .iP SCHED_RR 4
* Realtime, round-robin scheduling.
* .iP SCHED_FIFO
* Realtime, first-in first-out scheduling.
* .iP SCHED_OTHER
* Other, non-realtime scheduling.
* .LP
*
* VxWorks only supports SCHED_RR and SCHED_FIFO.
*
* RETURNS: On success zero; on failure a non-zero error code.
*
* ERRNOS: 'EINVAL'
*
* SEE ALSO:
* schedPxLib,
* pthread_attr_getschedpolicy(),
* pthread_attr_init(),
* pthread_attr_setinheritsched(),
* pthread_getschedparam(),
* pthread_setschedparam(),
* sched_setscheduler(),
* sched_getscheduler()
*/

int pthread_attr_setschedpolicy
    (
    pthread_attr_t *pAttr,		/* thread attributes	*/
    int policy				/* new policy		*/
    )
    {
    if (!pAttr || pAttr->threadAttrStatus != PTHREAD_INITIALIZED ||
	(policy != SCHED_OTHER &&
	policy != SCHED_FIFO &&
	policy != SCHED_RR))
	{
	return (EINVAL);
	}

    pAttr->threadAttrSchedpolicy = policy;

    return (_RETURN_PTHREAD_SUCCESS);
    }

/*******************************************************************************
*
* pthread_attr_getschedpolicy - get schedpolicy attribute from thread attributes object (POSIX)
*
* This routine returns, via the pointer <pPolicy>, the current scheduling
* policy in the thread attributes object specified by <pAttr>. Possible values
* for VxWorks systems are SCHED_RR and SCHED_FIFO; SCHED_OTHER is not
* supported.
*
* RETURNS: On success zero; on failure a non-zero error code.
*
* ERRNOS: 'EINVAL'
*
* SEE ALSO:
* schedPxLib,
* pthread_attr_init(),
* pthread_attr_setschedpolicy(),
* pthread_getschedparam(),
* pthread_setschedparam(),
* sched_setscheduler(),
* sched_getscheduler()
*/

int pthread_attr_getschedpolicy
    (
    const pthread_attr_t *pAttr,	/* thread attributes	*/
    int *pPolicy			/* current policy (out)	*/
    )
    {
    if (!pAttr || pAttr->threadAttrStatus != PTHREAD_INITIALIZED)
	return (EINVAL);

    *pPolicy = pAttr->threadAttrSchedpolicy;

    return (_RETURN_PTHREAD_SUCCESS);
    }

/*******************************************************************************
*
* pthread_attr_setschedparam - set schedparam attribute in thread attributes object (POSIX)
*
* Set the scheduling parameters in the thread attributes object <pAttr>.
* The scheduling parameters are essentially the thread's priority.
*
* RETURNS: On success zero; on failure a non-zero error code.
*
* ERRNOS: 'EINVAL'
*
* SEE ALSO:
* schedPxLib,
* pthread_attr_getschedparam(),
* pthread_attr_init(),
* pthread_getschedparam(),
* pthread_setschedparam(),
* sched_getparam(),
* sched_setparam()
*/

int pthread_attr_setschedparam
    (
    pthread_attr_t *pAttr,		/* thread attributes	*/
    const struct sched_param *pParam	/* new parameters	*/
    )
    {
    int policy;
    int priority;

    if (!pAttr || pAttr->threadAttrStatus != PTHREAD_INITIALIZED || !pParam)
	return (EINVAL);

    policy = pAttr->threadAttrSchedpolicy;

    if (!VALID_PRIORITY(pParam->sched_priority))
	return (EINVAL);
    else
	priority = pParam->sched_priority;

    pAttr->threadAttrSchedparam.sched_priority = priority;

    return (_RETURN_PTHREAD_SUCCESS);
    }

/*******************************************************************************
*
* pthread_attr_getschedparam - get value of schedparam attribute from thread attributes object (POSIX)
*
* Return, via the pointer <pParam>, the current scheduling parameters from the
* thread attributes object <pAttr>.
*
* RETURNS: On success zero; on failure a non-zero error code.
*
* ERRNOS: 'EINVAL'
*
* SEE ALSO:
* schedPxLib,
* pthread_attr_init(),
* pthread_attr_setschedparam(),
* pthread_getschedparam(),
* pthread_setschedparam(),
* sched_getparam(),
* sched_setparam()
*/

int pthread_attr_getschedparam
    (
    const pthread_attr_t *pAttr,	/* thread attributes		*/
    struct sched_param *pParam		/* current parameters (out)	*/
    )
    {
    if (!pAttr || pAttr->threadAttrStatus != PTHREAD_INITIALIZED || !pParam)
	return (EINVAL);

    pParam->sched_priority = pAttr->threadAttrSchedparam.sched_priority;

    return (_RETURN_PTHREAD_SUCCESS);
    }

/*******************************************************************************
*
* pthread_getschedparam - get value of schedparam attribute from a thread (POSIX)
*
* This routine reads the current scheduling parameters and policy of the thread
* specified by <thread>. The information is returned via <pPolicy> and
* <pParam>.
*
* RETURNS: On success zero; on failure a non-zero error code.
*
* ERRNOS: 'ESRCH'
*
* SEE ALSO:
* schedPxLib,
* pthread_attr_getschedparam()
* pthread_attr_getschedpolicy()
* pthread_attr_setschedparam()
* pthread_attr_setschedpolicy()
* pthread_setschedparam(),
* sched_getparam(),
* sched_setparam()
*/

int pthread_getschedparam
    (
    pthread_t thread,			/* thread			*/
    int *pPolicy,			/* current policy (out)		*/
    struct sched_param *pParam		/* current parameters (out)	*/
    )
    {
    internalPthread *	pThread = (internalPthread *)thread;
    WIND_TCB *		pTcb;
    BOOL		roundRobinOn;

    if (!VALID_PTHREAD(pThread, pTcb))
	return (ESRCH);

    roundRobinOn = _schedPxKernelIsTimeSlicing (NULL);

    if (roundRobinOn == TRUE)
	*pPolicy = SCHED_RR;
    else
	*pPolicy = SCHED_FIFO;

    return (sched_getparam((pid_t)pThread->taskId, pParam) != 0 ? errno : 0);
    }

/*******************************************************************************
*
* pthread_setschedparam - dynamically set schedparam attribute for a thread (POSIX)
*
* This routine will set the scheduling parameters (<pParam>) and policy
* (<policy>) for the thread specified by <thread>.
*
* In VxWorks the scheduling policy is global and not
* set on a per-thread basis; if the selected policy does not match the
* current global setting then this function will return an error ('EINVAL').
*
* RETURNS: On success zero; on failure a non-zero error code.
*
* ERRNOS: 'EINVAL', 'ESRCH'
*
* SEE ALSO:
* schedPxLib,
* pthread_attr_getschedparam()
* pthread_attr_getschedpolicy()
* pthread_attr_setschedparam()
* pthread_attr_setschedpolicy()
* pthread_getschedparam(),
* sched_getparam(),
* sched_setparam()
*/

int pthread_setschedparam
    (
    pthread_t thread,			/* thread		*/
    int policy,				/* new policy		*/
    const struct sched_param *pParam	/* new parameters	*/
    )
    {
    internalPthread *pThread = (internalPthread *)thread;
    WIND_TCB *pTcb;

    if (!VALID_PTHREAD(pThread, pTcb))
	return (ESRCH);

    return (sched_setscheduler((pid_t)pThread->taskId, policy, pParam) != 0 ?
	    errno : 0);
    }

/*
 * Section 16 - Thread Management
 */

/*******************************************************************************
*
* pthread_attr_init - initialize thread attributes object (POSIX)
*
* DESCRIPTION
*
* This routine initializes a thread attributes object. If <pAttr> is NULL
* then this function will return EINVAL.
*
* The attributes that are set by default are as follows:
* .iP "'Stack Address'" 4
* NULL - allow the system to allocate the stack.
* .iP "'Stack Size'"
* 0 - use the VxWorks taskLib default stack size.
* .iP "'Detach State'"
* PTHREAD_CREATE_JOINABLE
* .iP "'Contention Scope'"
* PTHREAD_SCOPE_SYSTEM
* .iP "'Scheduling Inheritance'"
* PTHREAD_INHERIT_SCHED
* .iP "'Scheduling Policy'"
* SCHED_RR
* .iP "'Scheduling Priority'"
* Use pthreadLib default priority
* .LP
*
* Note that the scheduling policy and priority values are only used if the
* scheduling inheritance mode is changed to PTHREAD_EXPLICIT_SCHED - see
* pthread_attr_setinheritsched() for information.
*
* RETURNS: On success zero; on failure a non-zero error code.
*
* ERRNOS: 'EINVAL'
*
* SEE ALSO:
* pthread_attr_destroy(),
* pthread_attr_getdetachstate(),
* pthread_attr_getinheritsched(),
* pthread_attr_getschedparam(),
* pthread_attr_getschedpolicy(),
* pthread_attr_getscope(),
* pthread_attr_getstackaddr(),
* pthread_attr_getstacksize(),
* pthread_attr_setdetachstate(),
* pthread_attr_setinheritsched(),
* pthread_attr_setschedparam(),
* pthread_attr_setschedpolicy(),
* pthread_attr_setscope(),
* pthread_attr_setstackaddr(),
* pthread_attr_setstacksize()
*/

int pthread_attr_init
    (
    pthread_attr_t *pAttr		/* thread attributes */
    )
    {
    if (!pAttr)
	return (EINVAL);    /* not a POSIX error return */

    pAttr->threadAttrStackaddr			= NULL;
    pAttr->threadAttrStacksize			= ((size_t) 0);
    pAttr->threadAttrDetachstate		= PTHREAD_CREATE_JOINABLE;
    pAttr->threadAttrContentionscope		= PTHREAD_SCOPE_SYSTEM;
    pAttr->threadAttrInheritsched		= PTHREAD_INHERIT_SCHED;
    pAttr->threadAttrSchedpolicy		= SCHED_RR;
    pAttr->threadAttrSchedparam.sched_priority	= DEF_PRIORITY;
    pAttr->threadAttrStatus			= PTHREAD_INITIALIZED;
    pAttr->threadAttrName                       = NULL;

    return (_RETURN_PTHREAD_SUCCESS);
    }

/*******************************************************************************
*
* pthread_attr_destroy - destroy a thread attributes object (POSIX)
*
* Destroy the thread attributes object <pAttr>. It should not be re-used until
* it has been reinitialized.
*
* RETURNS: On success zero; on failure a non-zero error code.
*
* ERRNOS: 'EINVAL'
*
* SEE ALSO:
* pthread_attr_init()
*/

int pthread_attr_destroy
    (
    pthread_attr_t *pAttr		/* thread attributes */
    )
    {
    if (!pAttr)
	return (EINVAL);    /* not a POSIX specified error return */

    pAttr->threadAttrStatus = PTHREAD_DESTROYED;

    return (_RETURN_PTHREAD_SUCCESS);
    }

/*******************************************************************************
*
* pthread_attr_setname - set name in thread attribute object
*
* This routine sets the name in the specified thread attributes object, <pAttr>.
*
* RETURNS: Always returns 
*
* ERRNOS: None.
*
* SEE ALSO:
* pthread_attr_getname(),
*/

int pthread_attr_setname
    (
    pthread_attr_t *pAttr,
    char *name
    )
    {
    pAttr->threadAttrName = name;

    return (_RETURN_PTHREAD_SUCCESS);
    }

/*******************************************************************************
*
* pthread_attr_getname - get name of thread attribute object
*
* This routine gets the name in the specified thread attributes object, <pAttr>.
*
* RETURNS: Always returns zero
*
* ERRNOS: None.
*
* SEE ALSO:
* pthread_attr_setname(),
*/

int pthread_attr_getname
    (
    pthread_attr_t *pAttr,
    char **name
    )
    {
    if (pAttr->threadAttrName == NULL)
	*name = "";
    else
	*name = pAttr->threadAttrName;
    
    return (_RETURN_PTHREAD_SUCCESS);
    }

/*******************************************************************************
*
* pthread_attr_setstacksize - set stacksize attribute in thread attributes
object (POSIX)
*
* This routine sets the thread stack size in the specified thread attributes
* object, <pAttr>.
*
* RETURNS: Always returns zero.
*
* ERRNOS: None.
*
* SEE ALSO:
* pthread_attr_getstacksize(),
* pthread_attr_init()
*/

int pthread_attr_setstacksize
    (
    pthread_attr_t *pAttr,		/* thread attributes	*/
    size_t stacksize			/* new stack size	*/
    )
    {
    pAttr->threadAttrStacksize = stacksize;

    return (_RETURN_PTHREAD_SUCCESS);
    }

/*******************************************************************************
*
* pthread_attr_getstacksize - get stack value of stacksize attribute from thread attributes object (POSIX)
*
* This routine gets the current stack size from the thread attributes object
* <pAttr> and places it in the location pointed to by <pStacksize>.
*
* RETURNS: Always returns zero.
*
* ERRNOS: None.
*
* SEE ALSO:
* pthread_attr_init(),
* pthread_attr_setstacksize()
*/

int pthread_attr_getstacksize
    (
    const pthread_attr_t *pAttr,	/* thread attributes		*/
    size_t *pStacksize			/* current stack size (out)	*/
    )
    {
    *pStacksize = pAttr->threadAttrStacksize;

    return (_RETURN_PTHREAD_SUCCESS);
    }

/*******************************************************************************
*
* pthread_attr_setstackaddr - set stackaddr attribute in thread attributes object (POSIX)
*
* This routine sets the stack address in the thread attributes object <pAttr>
* to be <pStackaddr>.
*
* It is important to note that the size of this stack must be large enough
* to also include the task's TCB.  The size of the TCB varies from architecture
* to architecture but can be determined by calling 'sizeof (WIND_TCB)'.
*
* Stack size is set using the routine pthread_attr_setstacksize();
*
* RETURNS: Always returns zero.
*
* ERRNOS: None.
*
* SEE ALSO:
* pthread_attr_getstacksize(),
* pthread_attr_setstacksize(),
* pthread_attr_init()
*/

int pthread_attr_setstackaddr
    (
    pthread_attr_t *pAttr,		/* thread attributes		*/
    void *pStackaddr			/* new stack address		*/
    )
    {
    pAttr->threadAttrStackaddr = pStackaddr;

    return (_RETURN_PTHREAD_SUCCESS);
    }

/*******************************************************************************
*
* pthread_attr_getstackaddr - get value of stackaddr attribute from thread attributes object (POSIX)
*
* This routine returns the stack address from the thread attributes object
* <pAttr> in the location pointed to by <ppStackaddr>.
*
* RETURNS: Always returns zero.
*
* ERRNOS: None.
*
* SEE ALSO:
* pthread_attr_init(),
* pthread_attr_setstacksize()
*/

int pthread_attr_getstackaddr
    (
    const pthread_attr_t *pAttr,	/* thread attributes		*/
    void **ppStackaddr			/* current stack address (out)	*/
    )
    {
    *ppStackaddr = pAttr->threadAttrStackaddr;

    return (_RETURN_PTHREAD_SUCCESS);
    }

/*******************************************************************************
*
* pthread_attr_setdetachstate - set detachstate attribute in thread attributes object (POSIX)
*
* This routine sets the detach state in the thread attributes object <pAttr>.
* The new detach state specified by <detachstate> must be one of
* PTHREAD_CREATE_DETACHED or PTHREAD_CREATE_JOINABLE. Any other values will
* cause an error to be returned ('EINVAL').
*
* RETURNS: On success zero; on failure a non-zero error code.
*
* ERRNOS: 'EINVAL'
*
* SEE ALSO:
* pthread_attr_getdetachstate(),
* pthread_attr_init()
*/

int pthread_attr_setdetachstate
    (
    pthread_attr_t *pAttr,		/* thread attributes	*/
    int detachstate			/* new detach state	*/
    )
    {
    if (detachstate != PTHREAD_CREATE_DETACHED &&
	detachstate != PTHREAD_CREATE_JOINABLE)
	{
	return (EINVAL);
	}

    pAttr->threadAttrDetachstate = detachstate;

    return (_RETURN_PTHREAD_SUCCESS);
    }

/*******************************************************************************
*
* pthread_attr_getdetachstate - get value of detachstate attribute from thread attributes object (POSIX)
*
* This routine returns the current detach state specified in the thread
* attributes object <pAttr>. The value is stored in the location pointed to
* by <pDetachstate>. Possible values for the detach state are:
* PTHREAD_CREATE_DETACHED and 'PTHREAD_CREATE_JOINABLE'.
*
* RETURNS: Always returns zero.
*
* ERRNOS: None.
*
* SEE ALSO:
* pthread_attr_init(),
* pthread_attr_setdetachstate()
*/

int pthread_attr_getdetachstate
    (
    const pthread_attr_t *pAttr,	/* thread attributes		*/
    int *pDetachstate			/* current detach state (out)	*/
    )
    {
    *pDetachstate = pAttr->threadAttrDetachstate;

    return (_RETURN_PTHREAD_SUCCESS);
    }

/*******************************************************************************
*
* __pthread_getcond -  (POSIX)
*
* DESCRIPTION
*
* RETURNS:
*
* NOMANUAL
*/

LOCAL pthread_cond_t *__pthread_getcond (void)
    {
    internalPthread *pThread = MY_PTHREAD;

    if (!pThread)
        return (NULL);
    return (pThread->cvcond);
    }

/*******************************************************************************
*
* sigCancelHandler - thread cancel signal handler function
*
* Handles thread cancel signals. It is attached as part of the thread creation.
*
* NOMANUAL
*/

LOCAL void sigCancelHandler()
    {
    pthread_cond_t *cvcond = __pthread_getcond();
 
    if (cvcond && cvcond->condMutex)
        {
        pthread_mutex_lock(cvcond->condMutex);
        --cvcond->condRefCount;
        }
 
    pthread_exit(PTHREAD_CANCELED);
    }

/*******************************************************************************
*
* wrapperFunc - thread entry wrapper function
*
* This function is the user level wrapper for the thread that manages the
* attachment of the thread cancel signal handler, and clean up if the thread
* entry function should just 'return' rather than calling pthread_exit().
*
* NOMANUAL
*/

LOCAL int wrapperFunc
    (
    int (*function)(int),
    int arg
    )
    {
    signal (SIGCANCEL, sigCancelHandler); 
 
    /* Call the function here, and pass its result to pthread_exit() */

    pthread_exit((void *)(function)(arg));
 
    /* NOTREACHED */
 
    return (0);
    }

/*******************************************************************************
*
* pthread_create - create a thread (POSIX)
*
* This routine creates a new thread and if successful writes its ID into the
* location pointed to by <pThread>. If <pAttr> is NULL then default attributes
* are used. The new thread executes <startRoutine> with <arg> as its argument.
*
* RETURNS: On success zero; on failure a non-zero error code.
*
* ERRNOS: 'EINVAL', 'EAGAIN'
*
* SEE ALSO:
* pthread_exit(),
* pthread_join()
*
* INTERNAL: This routine is in fact just a shell that calls the kernel to do
* the bulk of the work. The main reason for this arrangement though is to have
* the wrapper functions in user level where they need to be, but at the same
* time create the task that will represent the thread from within the kernel
* where we can directly manipulate things to set up the thread in special ways
* if the attributes require it.
*/

int pthread_create
    (
    pthread_t *	pThread,		/* Thread ID (out) */
    const pthread_attr_t * pAttr,	/* Thread attributes object */
    void * (*startRoutine)(void *),	/* Entry function */
    void * arg				/* Entry function argument */
    )
    {
    return (_pthreadCreate (pThread,
			    pAttr,
			    (void *) wrapperFunc,
			    startRoutine,
			    arg,
			    (int) posixPriorityNumbering));
    }

/*******************************************************************************
*
* pthread_detach - dynamically detach a thread (POSIX)
*
* This routine puts the thread <thread> into the detached state. This prevents
* other threads from synchronizing on the termination of the thread using
* pthread_join().
*
* RETURNS: On success zero; on failure a non-zero error code.
*
* ERRNOS: 'EINVAL', 'ESRCH'
*
* SEE ALSO: pthread_join()
*/

int pthread_detach
    (
    pthread_t thread		/* thread to detach */
    )
    {
    internalPthread *pThread = (internalPthread *) thread;
    WIND_TCB *pTcb;
    int ret;

    LOCK_PTHREAD(pThread);
    if (!VALID_PTHREAD(pThread, pTcb))
	ret = ESRCH;
    else
	{
	if (pThread->flags & JOINABLE)
	    {
	    pThread->flags &= ~JOINABLE;
	    ret = _RETURN_PTHREAD_SUCCESS;
	    }
	else
	    ret = EINVAL;
	}

    UNLOCK_PTHREAD(pThread);

    return (ret);
    }

/*******************************************************************************
*
* pthread_join - wait for a thread to terminate (POSIX)
*
* This routine will block the calling thread until the thread specified by
* <thread> terminates, or is canceled. The thread must be in the joinable
* state, i.e. it cannot have been detached by a call to pthread_detach(), or
* created in the detached state.
*
* If <ppStatus> is not NULL, when <thread> terminates its exit status will be
* stored in the specified location. The exit status will be either the value
* passed to pthread_exit(), or PTHREAD_CANCELED if the thread was canceled.
*
* Only one thread can wait for the termination of a given thread. If another
* thread is already waiting when this function is called an error will be
* returned ('EINVAL').
*
* If the calling thread passes its own ID in <thread>, the call will fail
* with the error 'EDEADLK'.
*
* NOTE
* All threads that remain <joinable> at the time they exit should ensure that
* pthread_join() is called on their behalf by another thread to reclaim the
* resources that they hold.
*
* RETURNS: On success zero; on failure a non-zero error code.
*
* ERRNOS: 'EINVAL', 'ESRCH', 'EDEADLK'
*
* SEE ALSO: pthread_detach(), pthread_exit()
*/

int pthread_join
    (
    pthread_t thread,		/* thread to wait for		*/
    void **ppStatus		/* exit status of thread (out)	*/
    )
    {
    internalPthread *	pThread	= (internalPthread *) thread;
    int 		myTid	= taskIdSelf();
    WIND_TCB *		pTcb;
    int			savtype;

    /* if task already exited, then return success */

    if (pThread)
	{
	if (pThread->flags & TASK_EXITED)
	    {
	    *ppStatus = pThread->exitStatus;
	    threadExitCleanup(pThread);
	    return (_RETURN_PTHREAD_SUCCESS);
	    }
	}

    /*
     * Check validity of thread. Can't happen earlier because if we did the
     * bit above the task had exited already.
     */

    if (!VALID_PTHREAD(pThread, pTcb))
	{
	return (ESRCH);
	}

    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &savtype);

    LOCK_PTHREAD(pThread);

    if (!(pThread->flags & JOINABLE) || pThread->flags & JOINER_WAITING)
	{
	UNLOCK_PTHREAD(pThread);
	pthread_setcanceltype(savtype, NULL);
	return (EINVAL);
	}

    if (pThread->taskId == myTid)
	{
	UNLOCK_PTHREAD(pThread);
	pthread_setcanceltype(savtype, NULL);
	return (EDEADLK);
	}

    pThread->flags |= JOINER_WAITING;

    UNLOCK_PTHREAD(pThread);

    while (pThread->flags & JOINER_WAITING)
	semTake(pThread->exitJoinSemId, WAIT_FOREVER);

    *ppStatus = pThread->exitStatus;

    threadExitCleanup(pThread);

    pthread_setcanceltype(savtype, NULL);

    return (_RETURN_PTHREAD_SUCCESS);
    }

/*******************************************************************************
*
* pthread_exit - terminate a thread (POSIX)
*
* This function terminates the calling thread. All cleanup handlers that have
* been set for the calling thread with pthread_cleanup_push() are executed
* in reverse order (the most recently added handler is executed first).
* Termination functions for thread-specific data are then called for all
* keys that have non-NULL values associated with them in the calling thread
* (see pthread_key_create() for more details). Finally, execution of the
* calling thread is stopped.
*
* The <status> argument is the return value of the thread and can be
* consulted from another thread using pthread_join() unless this thread was
* detached (i.e. a call to pthread_detach() had been made for it, or it was
* created in the detached state).
*
* All threads that remain <joinable> at the time they exit should ensure that
* pthread_join() is called on their behalf by another thread to reclaim the
* resources that they hold.
*
* RETURNS: Does not return.
*
* ERRNOS: N/A
*
* SEE ALSO:
* pthread_cleanup_push(),
* pthread_detach(),
* pthread_join(),
* pthread_key_create()
*/

void pthread_exit
    (
    void *status		/* exit status */
    )
    {
    internalPthread *	pThread		= MY_PTHREAD;
    cleanupHandler *	tmp;

    if (!pThread || !pThread->taskId)
	exit(EXIT_FAILURE);                            /* not a POSIX thread */

    while (pThread->handlerBase)
	{
	tmp = pThread->handlerBase;
	pThread->handlerBase->routine(pThread->handlerBase->arg);
	pThread->handlerBase = pThread->handlerBase->next;
	free(tmp);
	}

    cleanupPrivateData(pThread);

#ifndef _WRS_VXWORKS_5_X
    windLcbCurrent->pPthread = NULL;
#else
    taskIdCurrent->pPthread = NULL;
#endif

    if (!(pThread->flags & JOINABLE))
	{
	threadExitCleanup(pThread);
	pThread->flags |= TASK_EXITED;
	exit((int)status);
	}

    LOCK_PTHREAD(pThread);

    pThread->exitStatus = status;

    if (pThread->flags & JOINER_WAITING)
	{
	pThread->flags &= ~JOINER_WAITING;
	UNLOCK_PTHREAD(pThread);
	semGive(pThread->exitJoinSemId);
	}
    else
	UNLOCK_PTHREAD(pThread);

    pThread->flags |= TASK_EXITED;
    exit((int)status);
    }

/*******************************************************************************
*
* pthread_equal - compare thread IDs (POSIX)
*
* Tests the equality of the two threads <t1> and <t2>.
*
* RETURNS: Non-zero if <t1> and <t2> refer to the same thread, otherwise zero.
*/

int pthread_equal
    (
    pthread_t t1,			/* thread one */
    pthread_t t2			/* thread two */
    )
    {
    return (t1 == t2);
    }

/*******************************************************************************
*
* pthread_self - get the calling thread's ID (POSIX)
*
* This function returns the calling thread's ID.
*
* RETURNS: Calling thread's ID.
*/

pthread_t pthread_self (void)
    {
    self_become_pthread();

#ifndef _WRS_VXWORKS_5_X
    return ((pthread_t)windLcbCurrent->pPthread);
#else
    return ((pthread_t)taskIdCurrent->pPthread);
#endif
    }

/*******************************************************************************
*
* pthread_once - dynamic package initialization (POSIX)
*
* This routine provides a mechanism to ensure that one, and only one call
* to a user specified initialization function will occur. This allows all
* threads in a system to attempt initialization of some feature they need
* to use, without any need for the application to explicitly prevent multiple
* calls.
*
* When a thread makes a call to pthread_once(), the first thread to call it
* with the specified control variable, <onceControl>, will result in a call to
* <initFunc>, but subsequent calls will not. The <onceControl> parameter
* determines whether the associated initialization routine has been called.
* The <initFunc> function is complete when pthread_once() returns.
*
* The function pthread_once() is not a cancellation point; however, if the
* function <initFunc> is a cancellation point, and the thread is canceled
* while executing it, the effect on <onceControl> is the same as if
* pthread_once() had never been called.
*
* WARNING
* If <onceControl> has automatic storage duration or is not initialized to
* the value PTHREAD_ONCE_INIT, the behavior of pthread_once() is undefined.
*
* The constant PTHREAD_ONCE_INIT is defined in the 'pthread.h' header file.
*
* RETURNS: Always returns zero.
*
* ERRNOS: None.
*/

int pthread_once
    (
    pthread_once_t *onceControl,	/* once control location	*/
    void (*initFunc)(void)		/* function to call		*/
    )
    {
    int tasSuccess;

    if (onceControl->onceInitialized)
	return (_RETURN_PTHREAD_SUCCESS);

    tasSuccess = vxTas(&onceControl->onceMutex);

    if (tasSuccess == TRUE)
	{
	initFunc();
	onceControl->onceInitialized = 1;
	}

    return (_RETURN_PTHREAD_SUCCESS);
    }

/*
 * Section 17 - Thread-Specific Data
 */

/*******************************************************************************
*
* pthread_key_create - create a thread specific data key (POSIX)
*
* This routine allocates a new thread specific data key. The key is stored in
* the location pointed to by <key>. The value initially associated with the
* returned key is NULL in all currently executing threads. If the maximum
* number of keys are already allocated, the function returns an error
* ('EAGAIN').
*
* The <destructor> parameter specifies a destructor function associated with
* the key. When a thread terminates via pthread_exit(), or by cancellation,
* <destructor> is called with the value associated with the key in that
* thread as an argument. The destructor function is 'not' called if that value
* is NULL. The order in which destructor functions are called at thread
* termination time is unspecified.
*
* RETURNS: On success zero; on failure a non-zero error code.
*
* ERRNOS: 'EAGAIN'
*
* SEE ALSO:
* pthread_getspecific(),
* pthread_key_delete(),
* pthread_setspecific()
*/

int pthread_key_create
    (
    pthread_key_t *pKey,		/* thread specific data key	*/
    void (*destructor)(void *)		/* destructor function		*/
    )
    {

    /* If the mutex is not initialised, get out with an error */

    if (!key_mutex)
	return EAGAIN;

    self_become_pthread();

    semTake(key_mutex, WAIT_FOREVER);

    for ((*pKey) = 0; (*pKey) < _POSIX_THREAD_KEYS_MAX; (*pKey)++)
	{
	if (key_table[(*pKey)].count == 0)
	    {
	    key_table[(*pKey)].count = 1;
	    key_table[(*pKey)].destructor = destructor;
	    pthread_mutex_init(&(key_table[(*pKey)].mutex), NULL);
	    semGive(key_mutex);
	    return (_RETURN_PTHREAD_SUCCESS);
	    }
	}

    return(EAGAIN);
    }

/*******************************************************************************
*
* pthread_key_allocate_data - allocate key data (POSIX)
*
* Internal function to allocate the key storage internally for the thread.
*
* RETURNS: pointer to the new key data
*
* NOMANUAL
*/

static const void ** pthread_key_allocate_data(void)
    {
    const void ** new_data;

    if ((new_data =
	      (const void**)malloc(sizeof(void *) * _POSIX_THREAD_KEYS_MAX)))
	{
	memset((void *)new_data, 0, sizeof(void *) * _POSIX_THREAD_KEYS_MAX);
	}

    return(new_data);
    }

/*******************************************************************************
*
* pthread_setspecific - set thread specific data (POSIX)
*
* Sets the value of the thread specific data associated with <key> to <value>
* for the calling thread.
*
* RETURNS: On success zero; on failure a non-zero error code.
*
* ERRNOS: 'EINVAL', 'ENOMEM'
*
* SEE ALSO:
* pthread_getspecific(),
* pthread_key_create(),
* pthread_key_delete()
*/

int pthread_setspecific
    (
    pthread_key_t key,		/* thread specific data key	*/
    const void *value		/* new value			*/
    )
    {
    int			ret;
    internalPthread *	pThread;

    self_become_pthread();
    pThread = MY_PTHREAD;
    if ((pThread->privateData) ||
	(pThread->privateData = pthread_key_allocate_data()))
	{
	if ((key < _POSIX_THREAD_KEYS_MAX) && (key_table[key].count != 0))
	    {
	    pthread_mutex_lock(&(key_table[key].mutex));

	    if (pThread->privateData[key] == NULL)
		{
		if (value != NULL)
		    {
		    pThread->privateDataCount++;
		    }
		}
	    else
		{
		if (value == NULL)
		    {
		    pThread->privateDataCount--;
		    key_table[key].count = 0;
		    }
		}
	    pThread->privateData[key] = value;
	    ret = OK;

	    pthread_mutex_unlock(&(key_table[key].mutex));
	    }
	else
	    {
	    ret = EINVAL;
	    }
	}
    else
	{
	ret = ENOMEM;
	}

    return(ret);
    }

/*******************************************************************************
*
* pthread_getspecific - get thread specific data (POSIX)
*
* This routine returns the value associated with the thread specific data
* key <key> for the calling thread.
*
* RETURNS: The value associated with <key>, or NULL.
*
* ERRNOS: N/A
*
* SEE ALSO:
* pthread_key_create(),
* pthread_key_delete(),
* pthread_setspecific()
*/

void *pthread_getspecific
    (
    pthread_key_t key		/* thread specific data key */
    )
    {
    void *ret;
    internalPthread *pThread;

    self_become_pthread();
    pThread = MY_PTHREAD;
    if ((pThread->privateData) && (key < _POSIX_THREAD_KEYS_MAX))
	{
	pthread_mutex_lock(&(key_table[key].mutex));
	if (key_table[key].count)
	    {
	    ret = (void *)pThread->privateData[key];
	    }
	else
	    {
	    ret = NULL;
	    }
	pthread_mutex_unlock(&(key_table[key].mutex));
	}
    else
	{
	ret = NULL;
	}
    return(ret);
    }

/*******************************************************************************
*
* pthread_key_delete - delete a thread specific data key (POSIX)
*
* This routine deletes the thread specific data associated with <key>, and
* deallocates the key itself. It does not call any destructor associated
* with the key.
*
* RETURNS: On success zero; on failure a non-zero error code.
*
* ERRNOS: 'EINVAL'
*
* SEE ALSO:
* pthread_key_create()
*/

int pthread_key_delete
    (
    pthread_key_t key		/* thread specific data key to delete */
    )
    {
    if (!key_mutex)
	return EINVAL;

    if (key < _POSIX_THREAD_KEYS_MAX)
	{
	pthread_mutex_lock(&(key_table[key].mutex));
	switch (key_table[key].count)
	    {
	    case 1:
		/* mutex must be unlocked to destroy it */
		taskLock();
		pthread_mutex_unlock(&(key_table[key].mutex));
		pthread_mutex_destroy(&(key_table[key].mutex));
		key_table[key].destructor = NULL;
		key_table[key].count = 0;
		taskUnlock();
		return(_RETURN_PTHREAD_SUCCESS);
	    case 0:
		pthread_mutex_unlock(&(key_table[key].mutex));
		return(EINVAL);
	    default:
		pthread_mutex_unlock(&(key_table[key].mutex));
		return(EBUSY);
	    }
	pthread_mutex_unlock(&(key_table[key].mutex));
	}
    else
	{
	return(EINVAL);
	}
    }

/*
 * Section 18 - Thread Cancellation
 */

/*******************************************************************************
*
* pthread_cancel - cancel execution of a thread (POSIX)
*
* This routine sends a cancellation request to the thread specified by
* <thread>. Depending on the settings of that thread, it may ignore the
* request, terminate immediately or defer termination until it reaches a
* cancellation point.
*
* When the thread terminates it performs as if pthread_exit() had been called
* with the exit status 'PTHREAD_CANCELED'.
*
* IMPLEMENTATION NOTE
* In VxWorks, asynchronous thread cancellation is accomplished using a signal.
* The signal 'SIGCNCL' has been reserved for this purpose. Applications should
* take care not to block or handle this signal.
*
* RETURNS: On success zero; on failure a non-zero error code.
*
* ERRNOS: 'ESRCH'
*
* SEE ALSO:
* pthread_exit(),
* pthread_setcancelstate(),
* pthread_setcanceltype(),
* pthread_testcancel()
*/

int pthread_cancel
    (
    pthread_t thread			/* thread to cancel */
    )
    {
    internalPthread *	pThread	= (internalPthread *)thread;
    WIND_TCB *		pTcb;
    sigset_t		set;
    sigset_t *		pset	= &set;

    if (!VALID_PTHREAD(pThread, pTcb))
	return (ESRCH);

    sigemptyset(pset);
    sigaddset(pset, SIGCANCEL);
    sigprocmask(SIG_BLOCK, pset, NULL);
    CANCEL_LOCK(pThread);

    if (pThread->cancelstate == PTHREAD_CANCEL_ENABLE &&
	pThread->canceltype != PTHREAD_CANCEL_ASYNCHRONOUS)
	{
	pThread->cancelrequest = 1;
	CANCEL_UNLOCK(pThread);
	}
    else if (pThread->cancelstate == PTHREAD_CANCEL_ENABLE)
	{
	CANCEL_UNLOCK(pThread);
	pthread_kill((pthread_t)pThread, SIGCANCEL);
	}
    else
	CANCEL_UNLOCK(pThread);

    sigprocmask(SIG_UNBLOCK, pset, NULL);

    return (0);
    }

/*******************************************************************************
*
* pthread_setcancelstate - set cancellation state for calling thread (POSIX)
*
* This routine sets the cancellation state for the calling thread to <state>,
* and, if <oldstate> is not NULL, returns the old state in the location
* pointed to by <oldstate>.
*
* The state can be one of the following:
* .iP PTHREAD_CANCEL_ENABLE 4
* Enable thread cancellation.
* .iP PTHREAD_CANCEL_DISABLE
* Disable thread cancellation (i.e. thread cancellation requests are ignored).
* .LP
*
* RETURNS: On success zero; on failure a non-zero error code.
*
* ERRNOS: 'EINVAL'
*
* SEE ALSO:
* pthread_cancel(),
* pthread_setcanceltype(),
* pthread_testcancel()
*/

int pthread_setcancelstate
    (
    int state,			/* new state		*/
    int *oldstate		/* old state (out)	*/
    )
    {
    internalPthread *	pThread;
    sigset_t		set;
    sigset_t *		pset	= &set;

    self_become_pthread();

    pThread = MY_PTHREAD;
    if (state != PTHREAD_CANCEL_DISABLE && state != PTHREAD_CANCEL_ENABLE)
	return (EINVAL);

    sigemptyset(pset);
    sigaddset(pset, SIGCANCEL);
    sigprocmask(SIG_BLOCK, pset, NULL);
    CANCEL_LOCK(pThread);

    if (oldstate != NULL)
	*oldstate = pThread->cancelstate;

    pThread->cancelstate = state;

    CANCEL_UNLOCK(pThread);
    sigprocmask(SIG_UNBLOCK, pset, NULL);
    return (0);
    }

/*******************************************************************************
*
* pthread_setcanceltype - set cancellation type for calling thread (POSIX)
*
* This routine sets the cancellation type for the calling thread to <type>.
* If <oldtype> is not NULL, then the old cancellation type is stored in the
* location pointed to by <oldtype>.
*
* Possible values for <type> are:
* .iP PTHREAD_CANCEL_ASYNCHRONOUS 4
* Any cancellation request received by this thread will be acted upon as soon
* as it is received.
* .iP PTHREAD_CANCEL_DEFERRED
* Cancellation requests received by this thread will be deferred until the
* next cancellation point is reached.
* .LP
*
* RETURNS: On success zero; on failure a non-zero error code.
*
* ERRNOS: EINVAL
*
* SEE ALSO:
* pthread_cancel(),
* pthread_setcancelstate(),
* pthread_testcancel()
*/

int pthread_setcanceltype
    (
    int type,			/* new type		*/
    int *oldtype		/* old type (out)	*/
    )
    {
    return _pthreadSetCancelType (type, oldtype);
    }

/*******************************************************************************
*
* pthread_testcancel - create a cancellation point in the calling thread (POSIX)
*
* This routine creates a cancellation point in the calling thread. It has
* no effect if cancellation is disabled (i.e. the cancellation state has been
* set to PTHREAD_CANCEL_DISABLE using the pthread_setcancelstate() function).
*
* If cancellation is enabled, the cancellation type is PTHREAD_CANCEL_DEFERRED
* and a cancellation request has been received, then this routine will call
* pthread_exit() with the exit status set to 'PTHREAD_CANCELED'. If any of
* these conditions is not met, then the routine does nothing.
*
* RETURNS: N/A
*
* ERRNOS: N/A
*
* SEE ALSO:
* pthread_cancel(),
* pthread_setcancelstate(),
* pthread_setcanceltype()
*/

void pthread_testcancel (void)
    {
    internalPthread *pThread = MY_PTHREAD;

    if (!pThread)
	return;

    if (pThread->cancelstate == PTHREAD_CANCEL_ENABLE &&
	pThread->canceltype == PTHREAD_CANCEL_DEFERRED &&
	pThread->cancelrequest == 1)
	{
	pthread_exit(PTHREAD_CANCELED);
	}
    }

/*******************************************************************************
*
* pthread_cleanup_push - pushes a routine onto the cleanup stack (POSIX)
*
* This routine pushes the specified cancellation cleanup handler routine,
* <routine>, onto the cancellation cleanup stack of the calling thread. When
* a thread exits and its cancellation cleanup stack is not empty, the cleanup
* handlers are invoked with the argument <arg> in LIFO order from the
* cancellation cleanup stack.
*
* RETURNS: N/A
*
* ERRNOS: N/A
*
* SEE ALSO:
* pthread_cleanup_pop(),
* pthread_exit()
*/

void pthread_cleanup_push
    (
    void (*routine)(void *),		/* cleanup routine	*/
    void *arg				/* argument		*/
    )
    {
    internalPthread *pThread;
    cleanupHandler *pClean;

    self_become_pthread();
    pThread = MY_PTHREAD;
    pClean = malloc(sizeof (cleanupHandler));

    if (!pClean)
	return;

    pClean->routine	= routine;
    pClean->arg		= arg;
    pClean->next	= pThread->handlerBase;
    pThread->handlerBase= pClean;
    }

/*******************************************************************************
*
* pthread_cleanup_pop - pop a cleanup routine off the top of the stack (POSIX)
*
* This routine removes the cleanup handler routine at the top of the
* cancellation cleanup stack of the calling thread and executes it if
* <run> is non-zero. The routine should have been added using the
* pthread_cleanup_push() function.
*
* Once the routine is removed from the stack it will no longer be called when
* the thread exits.
*
* RETURNS: N/A
*
* ERRNOS: N/A
*
* SEE ALSO:
* pthread_cleanup_push(),
* pthread_exit()
*/

void pthread_cleanup_pop
    (
    int run			/* execute handler? */
    )
    {
    internalPthread *pThread = MY_PTHREAD;
    cleanupHandler *pClean;

    if (!pThread)
	return;

    pClean = pThread->handlerBase;
    if (pClean)
	{
	pThread->handlerBase = pClean->next;
	if (run != 0)
	    pClean->routine(pClean->arg);
	free(pClean);
	}
    }

/* support routines */

/*******************************************************************************
*
* cleanupPrivateData - release a thread's private data
*
* DESCRIPTION
*
* RETURNS:
*
* NOMANUAL
*/

LOCAL void cleanupPrivateData
    (
    internalPthread *	pThread
    )
    {
    void *		data;
    int			key;
    int			itr;

    if (!key_mutex || !pThread)
	return;

    semTake(key_mutex, WAIT_FOREVER);

    for (itr = 0; itr < _POSIX_THREAD_DESTRUCTOR_ITERATIONS; itr++)
	{
	for (key = 0; key < _POSIX_THREAD_KEYS_MAX; key++)
	    {
	    if (pThread->privateDataCount)
		{
		if (pThread->privateData[key])
		    {
		    data = (void *)pThread->privateData[key];
		    pThread->privateData[key] = NULL;
		    pThread->privateDataCount--;
		    if (key_table[key].destructor)
			{
			semGive(key_mutex);
			key_table[key].destructor(data);
			semTake(key_mutex, WAIT_FOREVER);
			}
		    pthread_mutex_destroy (&(key_table[key].mutex));
		    key_table[key].count	= 0;
		    key_table[key].destructor	= NULL;
		    }
		}
	    else
		{
		if (pThread->privateData != NULL)
		    {
		    free (pThread->privateData);
		    pThread->privateData = NULL;
		    }
		semGive(key_mutex);
		return;
		}
	    }
	}
    free (pThread->privateData);
    semGive(key_mutex);
    }

/*******************************************************************************
*
* pthreadDeleteTask - code used to perform use level cleanup from delete hook
*
* This is the function that is spawned as a task to clean up after a task
* that is exiting without calling the proper pthread_exit()  routine 
* (or is being deleted by some other abnormal means).
*
* NOMANUAL
*/

LOCAL void pthreadDeleteTask
    (
    internalPthread * pThread
    )
    {
    cleanupHandler *tmp;
 
    while (pThread->handlerBase)
	{
	tmp = pThread->handlerBase;
	pThread->handlerBase->routine(pThread->handlerBase->arg);
	pThread->handlerBase = pThread->handlerBase->next;
	free (tmp);
	}
    threadExitCleanup(pThread);
    }

/*******************************************************************************
*
* threadExitCleanup - exit cleanup routine
*
* Release pthread library resources associated with this thread.
*
* RETURNS:
*
* NOMANUAL
*/

LOCAL void threadExitCleanup
    (
    internalPthread *pThread
    )
    {
    pThread->taskId = 0;
    pThread->flags &= ~VALID;

    if (pThread->privateData)
	{
	cleanupPrivateData(pThread);
	}

    if (pThread->flags & JOINABLE)
	semDelete(pThread->exitJoinSemId);

    semDelete(pThread->mutexSemId);
    semDelete(pThread->cancelSemId);

    free (pThread);
    }

/*******************************************************************************
*
* self_become_pthread - convert a native task into a POSIX thread
*
* There are times when a main thread (e.g. one not created by pthread_create)
* may want to do pthread type things (create thread private data, establish
* cleanup handlers, etc. In these cases, we need to allocate an
* internalPthread struct for the task, and treat the thread as if it were
* detached.
*
* RETURNS:
*
* NOMANUAL
*/

LOCAL void self_become_pthread(void)
    {
    int myTid = taskIdSelf();
    internalPthread *pThread;

    if (MY_PTHREAD)
	return;
    if (!(pThread = malloc(sizeof (internalPthread))))
	return;

    bzero((char *)pThread, sizeof (internalPthread));
    pThread->privateData = NULL;
    pThread->handlerBase = NULL;
    pThread->taskId = myTid;
    if (!(pThread->mutexSemId = semMCreate(SEM_Q_PRIORITY|SEM_INVERSION_SAFE)))
	{
	free (pThread);
	return;
	}
    if (!(pThread->cancelSemId = semMCreate(SEM_Q_PRIORITY|SEM_INVERSION_SAFE)))
	{
	semDelete(pThread->mutexSemId);
	free (pThread);
	return;
	}
    taskPriorityGet(pThread->taskId, &pThread->priority);
    pThread->cancelstate = PTHREAD_CANCEL_ENABLE;
    pThread->canceltype = PTHREAD_CANCEL_DEFERRED;
    pThread->cancelrequest = 0;
    pThread->flags = VALID;

#if 0
    signal(SIGCANCEL, sigCancelHandler);
#endif

#ifndef _WRS_VXWORKS_5_X
    windLcbCurrent->pPthread = pThread;
#else
    taskIdCurrent->pPthread = pThread;
#endif

}
