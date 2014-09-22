/* ossLib.h - O/S-independent services */

/* Copyright 2000 Wind River Systems, Inc. */

/*
Modification history
--------------------
01g,18sep01,wef  merge from wrs.tor2_0.usb1_1-f for veloce
01f,07may01,wef  changed module number to be (module sub num << 8) | 
		 M_usbHostLib
01e,02may01,wef  changed module number to be M_<module> + M_usbHostLib
01d,16mar01,wef  changed priority levels of OSS_PRIORITY_HIGH and 
		 OSS_PRIORITY_INTERRUPT to be 5 and 10 respectivly
01d,05dec00,wef  moved Module number defs to vwModNum.h - add this
                 to #includes
01c,07mar00,rcb  Change MUTEX_HANDLE, SEM_HANDLE, and THREAD_HANDLE from
		 UINT32 to pVOID to allow comparison with NULL.
01b,13jul99,rcb  Removed queue functions to usbQueueLib.h.
01a,07jun99,rcb  First.
*/

/*
DESCRIPTION

Defines constants, structures, and functions provided by the ossLib.
These functions include:

o   Multithreading services (thread management and thread synchronization).
o   Shared memory allocation (for passing memory globally).
o   Time services.

Other system-unique services may be provided in the future.

IMPORTANT: Code using ossLib may be ported to preemptive and
non-preemptive multitasking environments.  Therefore, code must observe the
following rules:

1. Code must not assume that the scheduler is preemptive.  Therefore, no
task is permitted to "hog" the processor indefinitely.	All threads should 
regularly relinquish the processor, either through calls which may block in 
ossLib functions - which automatically relinquish, or through explicit 
calls to the threadSleep() function.

2. Code must not assume that the scheduler is NOT preemptive.  Therefore,
critical sections must be protected from reentrancy.  This protection is 
typically managed through the use of mutexes.
*/


#ifndef __INCossLibh
#define __INCossLibh

#ifdef	__cplusplus
extern "C" {
#endif


/* includes */

#include "vwModNum.h" 		/* USB Module Number Def's */

/* Defines */

/* ossLib return codes. */

/* 
 * USB errnos are defined as being part of the USB host Module, as are all
 * vxWorks module numbers, but the USB Module number is further divided into 
 * sub-modules.  Each sub-module has upto 255 values for its own error codes
 */
 
#define USB_OSS_SUB_MODULE  1

#define M_ossLib    ( (USB_OSS_SUB_MODULE  << 8) | M_usbHostLib )

#define ossErr(x)   (M_ossLib | (x))

#define S_ossLib_BAD_HANDLE	    ossErr (1)
#define S_ossLib_BAD_PARAMETER	    ossErr (2)
#define S_ossLib_OUT_OF_MEMORY	    ossErr (3)
#define S_ossLib_OUT_OF_RESOURCES   ossErr (4)
#define S_ossLib_INTERNAL_FAULT     ossErr (5)
#define S_ossLib_TIMEOUT	    ossErr (6)
#define S_ossLib_GENERAL_FAULT	    ossErr (7)
#define S_ossLib_DESTROY_FAILED     ossErr (8)
#define S_ossLib_QUEUE_FULL	    ossErr (9)
#define S_ossLib_QUEUE_EMPTY	    ossErr (10)
#define S_ossLib_NO_SIGNAL	    ossErr (11)
#define S_ossLib_TAKE_MUTEX	    ossErr (12)
#define S_ossLib_RELEASE_MUTEX	    ossErr (13)
#define S_ossLib_NOT_IMPLEMENTED    ossErr (14)


/* Blocking/non-blocking parameters for functions which can block. */

#define OSS_BLOCK	0xffffffff  /* Function will block indefinitely */
#define OSS_DONT_BLOCK	0x0	    /* Function will not block */
				    /* NOTE: All other values are interpreted */
				    /* as a count of milliseconds to block */


/* Thread priority parameters for ossThreadCreate */

#define OSS_PRIORITY_INHERIT	0xffff	/* inherit caller's priority */
#define OSS_PRIORITY_LOW	255
#define OSS_PRIORITY_TYPICAL	127
#define OSS_PRIORITY_HIGH	10
#define OSS_PRIORITY_INTERRUPT	5


/* Maximum length of a thread name string. */

#define MAX_OSS_NAME_LEN	32


/* Structure definitions. */

/* Handles. */

typedef pVOID THREAD_HANDLE;	    /* Thread handle */
typedef THREAD_HANDLE *pTHREAD_HANDLE;

typedef pVOID SEM_HANDLE;	    /* Semaphore handle */
typedef SEM_HANDLE *pSEM_HANDLE;    

typedef pVOID MUTEX_HANDLE;	    /* Mutex handle */
typedef MUTEX_HANDLE *pMUTEX_HANDLE;


/* Structure of a thread entry point */

typedef VOID (*THREAD_PROTOTYPE) (pVOID param);


/* function prototypes */

STATUS ossStatus 
    (
    int status
    );


STATUS ossInitialize (void);


STATUS ossShutdown (void);


STATUS ossThreadCreate 
    (
    THREAD_PROTOTYPE func,	    /* function to spawn as new thread */
    pVOID param,		    /* Parameter to be passed to new thread */
    UINT16 priority,		    /* OSS_PRIORITY_xxxx */
    pCHAR name, 		    /* thread name or NULL */
    pTHREAD_HANDLE pThreadHandle    /* Handle of newly spawned thread */
    );

#define OSS_THREAD_CREATE(func, param, priority, name, pThreadHandle) \
    ossThreadCreate (func, param, priority, name, pThreadHandle)


STATUS ossThreadDestroy 
    (
    THREAD_HANDLE threadHandle	    /* handle of thread to be destroyed */
    );

#define OSS_THREAD_DESTROY(threadHandle) ossThreadDestroy (threadHandle)


STATUS ossThreadSleep
    (
    UINT32 msec 		    /* Number of msec to sleep */
    );

#define OSS_THREAD_SLEEP(msec) ossThreadSleep (msec)


STATUS ossSemCreate 
    (
    UINT32 maxCount,		    /* Max count allowed for semaphore */
    UINT32 curCount,		    /* initial count for semaphore */
    pSEM_HANDLE pSemHandle	    /* newly created semaphore handle */
    );

#define OSS_SEM_CREATE(maxCount, curCount, pSemHandle) \
    ossSemCreate (maxCount, curCount, pSemHandle)


STATUS ossSemDestroy 
    (
    SEM_HANDLE semHandle	    /* Handle of semaphore to destroy */
    );

#define OSS_SEM_DESTROY(semHandle) ossSemDestroy (semHandle)


STATUS ossSemGive 
    (
    SEM_HANDLE semHandle	    /* semaphore to signal */
    );

#define OSS_SEM_GIVE(semHandle) ossSemGive (semHandle)


STATUS ossSemTake 
    (
    SEM_HANDLE semHandle,	    /* semaphore to take */
    UINT32 blockFlag		    /* specifies blocking action */
    );

#define OSS_SEM_TAKE(semHandle, blockFlag) ossSemTake (semHandle, blockFlag)


STATUS ossMutexCreate 
    (
    pMUTEX_HANDLE pMutexHandle	    /* Handle of newly created mutex */
    );

#define OSS_MUTEX_CREATE(pMutexHandle) ossMutexCreate (pMutexHandle)


STATUS ossMutexDestroy 
    (
    MUTEX_HANDLE mutexHandle	    /* Handle of mutex to destroy */
    );

#define OSS_MUTEX_DESTROY(mutexHandle) ossMutexDestroy (mutexHandle)


STATUS ossMutexTake 
    (
    MUTEX_HANDLE mutexHandle,	    /* Mutex to take */
    UINT32 blockFlag		    /* specifies blocking action */
    );

#define OSS_MUTEX_TAKE(mutexHandle, blockFlag) \
    ossMutexTake (mutexHandle, blockFlag)


STATUS ossMutexRelease 
    (
    MUTEX_HANDLE mutexHandle	    /* Mutex to be released */
    );

#define OSS_MUTEX_RELEASE(mutexHandle) ossMutexRelease(mutexHandle)


pVOID ossMalloc 
    (
    UINT32 numBytes		    /* Size of buffer to allocate */
    );

#define OSS_MALLOC(numBytes) ossMalloc(numBytes)


pVOID ossCalloc
    (
    UINT32 numBytes		    /* size of buffer to allocate */
    );

#define OSS_CALLOC(numBytes) ossCalloc(numBytes)


pVOID ossMallocAlign
    (
    UINT32 numBytes,		    /* size of buffer to allocate */
    UINT32 alignment,		    /* alignment, must be power of 2 */
    BOOL zero			    /* indicates if memory to be zeroed */
    );

#define OSS_MALLOC_ALIGN(numBytes, alignment, zero) \
    ossMallocAlign (numBytes, alignment, zero)


VOID ossFree 
    (
    pVOID bfr
    );

#define OSS_FREE(bfr) ossFree (bfr)


UINT32 ossTime (void);

#define OSS_TIME() ossTime ()


#ifdef	__cplusplus
}
#endif

#endif	/* __INCossLibh */


/* End of file. */

