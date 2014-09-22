/* ossLib.c - O/S-independent services for vxWorks */

/* Copyright 2000-2002 Wind River Systems, Inc. */

/*
Modification history
--------------------
01f,22jan02,wef  All USB memory now comes from a cacheDmaMalloc'd partition.  
		 Instrument routines to get partition information.
01e,25oct01,wef  changed ossMalloc to call memalign instead of malloc - it now
		 returns buffers that are a multiple of a cache line size 
		 aligned on a cache line boundary
01d,18sep01,wef  merge from wrs.tor2_0.usb1_1-f for veloce
01c,01aug01,rcb  fixed manpage generation in comments
01b,07mar00,rcb  Cast handles as necessary to reflect change in handle
		 definition from UINT32 to pVOID.
01a,07jun99,rcb  First.
*/

/*
DESCRIPTION

Implements functions defined by ossLib.h.  See ossLib.h for
a complete description of these functions.
*/


/* includes */

#include "vxWorks.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "errnoLib.h"		/* errno */
#include "sysLib.h"		/* sysClkRateGet(), etc. */
#include "taskLib.h"		/* taskSpawn(), etc. */
#include "semLib.h"		/* semCreate(), etc. */
#include "objLib.h"		/* S_objLib_OBJ_xxxx errors */
#include "tickLib.h"		/* tickGet(), etc. */
#include "cacheLib.h"		/* cacheDmaMalloc, Free */

#include "usb/usbPlatform.h"
#include "usb/ossLib.h" 	/* Our API */


/* Defines */

/* #define  FILL_MEMORY */	    /* for debugging */


/* Definitions related to the creation of new threads. */

#define DEFAULT_OPTIONS     0	    /* task options */
#define DEFAULT_STACK	    0x4000  /* Default stack size */


/* 
 * TIMEOUT() calculates the number of ticks for a timeout from a blockFlag
 * parameter to one of the ossLib functions.
 */

#define TIMEOUT(bf) (((bf) == OSS_BLOCK) ? \
    WAIT_FOREVER : ((bf) + msecsPerTick - 1) / msecsPerTick)

#define USB_MEM_PART_DEF_SIZE 0x10000	/* 0x10000  Default 
					 * partition size - 64k 
					 */

/* typedefs */

/* globals */

/* Default to using the partition method of malloc / free. */

FUNCPTR ossMallocFuncPtr;
FUNCPTR ossFreeFuncPtr;

/* locals */

LOCAL int msecsPerTick = 55;	    /* milliseconds per system tick */
				    /* value updated by ossInitialize() */

LOCAL BOOL 	ossPartInitFlag = TRUE;

LOCAL char *  	pUsbMemSpace; 

LOCAL PART_ID 	usbMemPartId;

LOCAL UINT32 	usbMemPartSize = USB_MEM_PART_DEF_SIZE;

LOCAL UINT32 	usbMemCount = 0;

LOCAL UINT32 	ossOldInstallFlag = FALSE;

/* Functions */

/***************************************************************************
*
* translateError - translates vxWorks error to S_ossLib_xxxx
*
* Translates certain vxWorks errno values to an appropriate S_ossLib_xxxx.
* This function should only be invoked when the caller has already
* ascertained that a vxWorks function has returned an error.
*
* RETURNS: S_ossLib_xxxx
*
* ERRNO:
*  S_ossLib_BAD_HANDLE
*  S_ossLib_TIMEOUT
*  S_ossLib_GENERAL_FAULT
*/

LOCAL int translateError (void)
    {
    switch (errno)
	{
	case S_objLib_OBJ_ID_ERROR:	return S_ossLib_BAD_HANDLE;
	case S_objLib_OBJ_TIMEOUT:	return S_ossLib_TIMEOUT;
	default:			return S_ossLib_GENERAL_FAULT;
	}
    }


/***************************************************************************
*
* translatePriority - translates OSS_PRIORITY_xxx to vxWorks priority
*
* RETURNS: vxWorks task priority
*/

LOCAL int translatePriority
    (
    UINT16 priority
    )

    {
    int curPriority;

    switch (priority)
	{
	case OSS_PRIORITY_LOW:
	case OSS_PRIORITY_TYPICAL:
	case OSS_PRIORITY_HIGH:
	case OSS_PRIORITY_INTERRUPT:

	    return priority;

	case OSS_PRIORITY_INHERIT:
	default:

	    taskPriorityGet (0, &curPriority);
	    return curPriority;
	}
    }


/***************************************************************************
*
* threadHead - vxWorks taskSpawn() compatible thread entry point
*
* ossThreadCreate() uses this intermediary function to spawn threads.  The
* vxWorks taskSpawn() function passes 10 int parameters to the task entry
* point.  By convention, ossThreadCreate passes the function's real entry
* point in <arg1> and the thread's <param> in <arg2>..
*/

LOCAL int threadHead
    (
    int arg1,			    /* <func> in this parameter */
    int arg2,			    /* <param> in this parameter */
    int arg3,
    int arg4,
    int arg5,
    int arg6,
    int arg7,
    int arg8,
    int arg9,
    int arg10
    )
    
    {
    THREAD_PROTOTYPE func = (THREAD_PROTOTYPE) arg1;
    pVOID param = (pVOID) arg2;

    (*func) (param);

    return 0;
    }


/***************************************************************************
*
* ossStatus - Returns OK or ERROR and sets errno based on status
*
* If <status> & 0xffff is not equal to zero, then sets errno to the
* indicated <status> and returns ERROR.  Otherwise, does not set errno
* and returns OK.
*
* RETURNS: OK or ERROR
*/

STATUS ossStatus 
    (
    int status
    )

    {
    if ((status & 0xffff) != 0)
	{
	errnoSet (status);
	return ERROR;
	}
    
    return OK;
    }



/***************************************************************************
*
* ossShutdown - Shuts down ossLib
*
* This function should be called once at system shutdown if an only if
* the corresponding call to ossInitialize() was successful.
*
* RETURNS: OK or ERROR
*/

STATUS ossShutdown (void)
    {
    return OK;
    }


/***************************************************************************
*
* ossThreadCreate - Spawns a new thread
*
* The ossThreadCreate() function creates a new thread which begins execution 
* with the specified <func>.  The <param> argument will be passed to <func>.  
* The ossThreadCreate() function creates the new thread with a stack of a 
* default size and with no security restrictions - that is, there are no 
* restrictions on the use of the returned <pThreadHandle> by other threads.  
* The newly created thread will execute in the same address space as the 
* calling thread.  <priority> specifies the thread's desired priority - in
* systems which implement thread priorities, as OSS_PRIORITY_xxxx.  
*
* RETURNS: OK or ERROR
*
* ERRNO:
*  S_ossLib_BAD_PARAMETER
*  S_ossLib_GENERAL_FAULT
*/

STATUS ossThreadCreate 
    (
    THREAD_PROTOTYPE func,	    /* function to spawn as new thread */
    pVOID param,		    /* Parameter to be passed to new thread */
    UINT16 priority,		    /* OSS_PRIORITY_xxxx */
    pCHAR name, 		    /* thread name or NULL */
    pTHREAD_HANDLE pThreadHandle    /* Handle of newly spawned thread */
    )

    {
    /* validate params */

    if (pThreadHandle == NULL)
	return ossStatus (S_ossLib_BAD_PARAMETER);


    /* Use vxWorks to spawn a new thread (task in vxWorks parlance). */

    if ((*pThreadHandle = (THREAD_HANDLE) taskSpawn (name, 
	translatePriority (priority), DEFAULT_OPTIONS, DEFAULT_STACK, 
	threadHead, (int) func, (int) param, 0, 0, 0, 0, 0, 0, 0, 0)) == 
	(THREAD_HANDLE) ERROR)
	return ossStatus (S_ossLib_GENERAL_FAULT);

    return OK;
    }


/***************************************************************************
*
* ossThreadDestroy - Attempts to destroy a thread
*
* This function attempts to destroy the thread specified by <threadHandle>.
*
* NOTE: Generally, this function should be called only after the given
* thread has terminated normally.  Destroying a running thread may result
* in a failure to release resources allocated by the thread.
*
* RETURNS: OK or ERROR
*
* ERRNO:
*  S_ossLib_GENERAL_FAULT
*/

STATUS ossThreadDestroy 
    (
    THREAD_HANDLE threadHandle	    /* handle of thread to be destroyed */
    )

    {
    if (taskDelete ((int) threadHandle) == ERROR)
	return ossStatus (S_ossLib_GENERAL_FAULT);

    return OK;
    }
    

/***************************************************************************
*
* ossThreadSleep - Voluntarily relinquishes the CPU
*
* Threads may call ossThreadSleeph() to voluntarily release the CPU to
* another thread/process.  If the <msec> argument is 0, then the thread will
* be reschuled for execution as soon as possible.  If the <msec> argument is
* greater than 0, then the current thread will sleep for at least the number
* of milliseconds specified.
*
* RETURNS: OK or ERROR
*/

STATUS ossThreadSleep
    (
    UINT32 msec 		    /* Number of msec to sleep */
    )

    {
    /* Delay for a number of ticks at least as long as the number of
    milliseconds requested by the caller. */

    taskDelay ((msec == 0) ? 0 : ((msec + msecsPerTick - 1) / msecsPerTick) + 1);

    return OK;
    }


/***************************************************************************
*
* ossSemCreate - Creates a new semaphore
*
* This function creates a new semaphore and returns the handle of that
* semaphore in <pSemHandle>.  The semaphore's initial count is set to
* <curCount> and has a maximum count as specified by <maxCount>.
*
* RETURNS: OK or ERROR
* 
* ERRNO:
*  S_ossLib_BAD_PARAMETER
*  S_ossLib_GENERAL_FAULT
*/

STATUS ossSemCreate 
    (
    UINT32 maxCount,		    /* Max count allowed for semaphore */
    UINT32 curCount,		    /* initial count for semaphore */
    pSEM_HANDLE pSemHandle	    /* newly created semaphore handle */
    )

    {
    /* Validate parameters */

    if (pSemHandle == NULL) 
	return ossStatus (S_ossLib_BAD_PARAMETER);

    if ((*pSemHandle = (SEM_HANDLE) semCCreate (SEM_Q_FIFO, curCount)) == NULL)
	return ossStatus (S_ossLib_GENERAL_FAULT);

    return OK;
    }


/***************************************************************************
*
* ossSemDestroy - Destroys a semaphore
*
* Destroys the semaphore <semHandle> created by ossSemCreate().
*
* RETURNS: OK or ERROR
*
* ERRNO:
*  S_ossLib_GENERAL_FAULT
*/

STATUS ossSemDestroy 
    (
    SEM_HANDLE semHandle	    /* Handle of semaphore to destroy */
    )

    {
    if (semDelete ((SEM_ID) semHandle) != OK)
	return ossStatus (S_ossLib_GENERAL_FAULT);

    return OK;
    }


/***************************************************************************
*
* ossSemGive - Signals a semaphore
*
* This function signals the sepcified semaphore.  A semaphore may have more
* than one outstanding signal, as specified by the maxCount parameter when
* the semaphore was created by ossSemCreate().	While the semaphore is at its
* maximum count, additional calls to ossSemSignal for that semaphore have no
* effect.
*
* RETURNS: OK or ERROR
*
* ERRNO:
*  S_ossLib_BAD_HANDLE
*/

STATUS ossSemGive
    (
    SEM_HANDLE semHandle	    /* semaphore to signal */
    )

    {
    if (semGive ((SEM_ID) semHandle) != OK)
	return ossStatus (S_ossLib_BAD_HANDLE);

    return OK;
    }



/***************************************************************************
*
* ossSemTake - Attempts to take a semaphore
*
* ossSemTake() attempts to "take" the semaphore specified by <semHandle>.
* <blockFlag> specifies the blocking behavior.	OSS_BLOCK blocks indefinitely
* waiting for the semaphore to be signalled.  OSS_DONT_BLOCK does not block
* and returns an error if the semaphore is not in the signalled state.	Other
* values of <blockFlag> are interpreted as a count of milliseconds to wait
* for the semaphore to enter the signalled state before declaring an error.
*
* RETURNS: OK or ERROR
*/

STATUS ossSemTake 
    (
    SEM_HANDLE semHandle,	    /* semaphore to take */
    UINT32 blockFlag		    /* specifies blocking action */
    )

    {
    if (semTake ((SEM_ID) semHandle, TIMEOUT (blockFlag)) != OK)
	return ossStatus (translateError ());

    return OK;
    }


/***************************************************************************
*
* ossMutexCreate - Creates a new mutex
*
* This function creates a new mutex and returns the handle of that
* mutex in <pMutexHandle>.  The mutex is created in the "untaken" state.
*
* RETURNS: OK or STATUS
*
* ERRNO:
*  S_ossLib_BAD_PARAMETER
*  S_ossLib_GENERAL_FAULT
*/

STATUS ossMutexCreate 
    (
    pMUTEX_HANDLE pMutexHandle	    /* Handle of newly created mutex */
    )

    {
    
    /* Validate parameters */

    if (pMutexHandle == NULL) 
	return ossStatus (S_ossLib_BAD_PARAMETER);

    if ((*pMutexHandle = (MUTEX_HANDLE) semMCreate (SEM_Q_FIFO)) == NULL)
	return ossStatus (S_ossLib_GENERAL_FAULT);

    return OK;
    }


/***************************************************************************
*
* ossMutexDestroy - Destroys a mutex
*
* Destroys the mutex <mutexHandle> created by ossMutexCreate().
*
* RETURNS: OK or ERROR
*
* ERRNO:
*  S_ossLib_GENERAL_FAULT
*/

STATUS ossMutexDestroy 
    (
    MUTEX_HANDLE mutexHandle	    /* Handle of mutex to destroy */
    )

    {
    if (semDelete ((SEM_ID) mutexHandle) != OK)
	return ossStatus (S_ossLib_GENERAL_FAULT);

    return OK;
    }


/***************************************************************************
*
* ossMutexTake - Attempts to take a mutex
*
* ossMutexTake() attempts to "take" the specified mutex.  The attempt will
* succeed if the mutex is not owned by any other threads.  If a thread
* attempts to take a mutex which it already owns, the attempt will succeed.
* <blockFlag> specifies the blocking behavior.	OSS_BLOCK blocks indefinitely
* waiting for the mutex to be released.  OSS_DONT_BLOCK does not block and
* returns an error if the mutex is not in the released state.  Other values
* of <blockFlag> are interpreted as a count of milliseconds to wait for the
* mutex to be released before declaring an error.
*
* RETURNS: OK or ERROR
*/

STATUS ossMutexTake 
    (
    MUTEX_HANDLE mutexHandle,	    /* Mutex to take */
    UINT32 blockFlag		    /* specifies blocking action */
    )

    {
    if (semTake ((SEM_ID) mutexHandle, TIMEOUT (blockFlag)) != OK)
	return ossStatus (translateError ());

    return OK;
    }


/***************************************************************************
*
* ossMutexRelease - Releases (gives) a mutex
*
* Release the mutex specified by <mutexHandle>.  This function will fail
* if the calling thread is not the owner of the mutex.
*
* RETURNS: OK or ERROR
*
* ERRNO:
*  S_ossLib_BAD_HANDLE
*/

STATUS ossMutexRelease 
    (
    MUTEX_HANDLE mutexHandle	    /* Mutex to be released */
    )

    {
    if (semGive ((SEM_ID) mutexHandle) != OK)
	return ossStatus (S_ossLib_BAD_HANDLE);

    return OK;
    }

/***************************************************************************
*
* ossPartSizeGet - Retrieves the size of the USB memory partition.
*
* Returns the size of the USB memory partition.  
*
* RETURNS: Size of partition.
*/

UINT32 ossPartSizeGet (void)
    {

    return usbMemPartSize;

    }

/***************************************************************************
*
* ossPartSizeSet - Sets the the initial size of the USB memory partition.
*
* Sets the size of the USB memory partition.  This must be called prior to 
* the first call to ossMalloc.  This will set the size that ossMalloc will
* use to do its allocation. Once ossMalloc has been called, the partition 
* size has been already allocated.  To add more memory to the USB partition, 
* you must retrieve the USB partition ID and add more memory via the 
* memPartLib routines.
*
* RETURNS: OK or ERROR
*
* SEE ALSO: memPartLib
*
*/

STATUS ossPartSizeSet 
    (
    UINT32 numBytes
    )
    {

    /* ossMalloc has already initialized the USB memory partition */

    if (ossPartInitFlag == FALSE)
	return ERROR;

    /* ossMalloc has not been called yet, so set the partition size.  */

    else
	usbMemPartSize = numBytes;

    return usbMemPartSize;

    }

/***************************************************************************
*
* ossPartIdGet - Retrieves the partition ID of USB memory partition.
*
* Returns the partition ID of the USB memory partition.  
*
* RETURNS: The partition ID.
*/

PART_ID ossPartIdGet (void)
    {

    return usbMemPartId;

    }

/***************************************************************************
*
* ossMemUsedGet - Retrieves amount of memory currently in use by USB.
*
* Returns the amount, in bytes, currently being used by USB.
*
* RETURNS: Number of bytes of memory in use.
*/

UINT32 ossMemUsedGet (void)
    {

    return usbMemCount;

    }


/***************************************************************************
*
* ossMalloc - Master USB memory allocation routine.
*
* ossMalloc() calls the malloc routine installed in the global variable
* <ossMallocFuncPtr>.  These default to ossPartMalloc(), but can be changed
* by the user to their own defined malloc routine or to a non-partition
* method of malloc / free by calling ossOldInstall().
*
* RETURNS: Pointer to allocated buffer, or NULL 
*
*/

void * ossMalloc 
    (
    UINT32 numBytes
    )
    {
    return (void *) ((*ossMallocFuncPtr) (numBytes));
    }

/***************************************************************************
*
* ossPartMalloc - USB memory allocation.
*
* ossPartMalloc() allocates cache-safe buffer of size <numBytes> out of the USB 
* partition and returns a pointer to this buffer.  The buffer is allocated 
* from a local USB partition.  The size of this partition is defaulted to 
* 64k but can be modified to suit the users needs.  This partition will 
* dynamically grow based on additional need.  Memory allocated by this 
* function must be freed by calling ossFree().
*
* RETURNS: Pointer to allocated buffer, or NULL 
*
* INTERNAL:
*
* ossPartMalloc() is responsible for creating the USB partition.  This is 
* to allow full user control over the USB memory system.  The decision to
* put the partition creation here as opposed to ossInitialize() was because
* ossInitialize() is called from usbdInitialize().  Implementations of both
* these routines are not shipped in source.  Therefore the order in which
* ossInitialize() is called cannot be modified by users who do not have 
* source code.   Putting the partition creation in ossPartMalloc() allows
* the user to set the malloc / free pointers with a call to ossOldInstall()
* such that the partition is not created if need be.
*
*/

void * ossPartMalloc 
    (
    UINT32 numBytes		    /* Size of buffer to allocate */
    )
    {
    void * pBfr;
    void * pMoreMemory;
    UINT32 growSize;

    /*  Create the USB memory partition from cache safe memory */ 

    if (ossPartInitFlag == TRUE)
	{

	/* on the first pass create 64k chunk of cache safe memory for usb */

	pUsbMemSpace = (char *) cacheDmaMalloc (usbMemPartSize);

	if (pUsbMemSpace == NULL)
	    {
	    printf ("ossLib.c: unable to allocate USB memory space.\n");
	    return NULL;
	    } 
	
	/* make this chunk a partition */
	
	usbMemPartId = memPartCreate (pUsbMemSpace, usbMemPartSize);
	
	if (usbMemPartId == NULL)
	    {
	    printf ("ossLib.c: unable to create USB memory partition.\n");
	    return NULL;
	    } 

	ossPartInitFlag = FALSE;

    	}

    /* 
     * If this call to ossMalloc is going to allocate more memory than is 
     * available in the partition, then grow the partition by 8k, or if
     * numBytes is larger than 4k, then grow the partition by numBytes + 4k.
     */

    if ((usbMemCount + numBytes) > (usbMemPartSize - 0x1000))
	{

	growSize = 0x2000;

	if (numBytes > 0x1000)
	    growSize += numBytes;

	pMoreMemory = cacheDmaMalloc (growSize);
	
	if (pMoreMemory == NULL)
	   {
	   printf ("ossLib.c: ossPartMalloc could not cacheDmaMalloc() new" \
		   " memory.\n");
	   return NULL;
	   }

	if (memPartAddToPool (usbMemPartId, pMoreMemory, growSize) == ERROR)
	   {
	   printf ("ossLib.c: ossPartMalloc could not add new memory to" \
		   " pool.\n");
	   return NULL;
	   }
	
	usbMemPartSize += growSize;	
	}

    /* From now on, use this partition for all USB mallocs */

    pBfr = memPartAlignedAlloc (usbMemPartId, 
				ROUND_UP(numBytes, _CACHE_ALIGN_SIZE), 
				_CACHE_ALIGN_SIZE);
	
    if (pBfr == NULL)
	{
	printf ("ossLib.c: unable to malloc USB memory.\n");
	return NULL;
	} 

    /* Update the amount of memory USB is currently using. */

    usbMemCount += ROUND_UP(numBytes, _CACHE_ALIGN_SIZE);

    return pBfr;

    }

/***************************************************************************
*
* ossOldMalloc - Global memory allocation
*
* ossOldMalloc() allocates a buffer of <numBytes> in length and returns a
* pointer to the allocated buffer.  The buffer i allocated from a global
* pool which can be made visible to all processes or drivers in the
* system.  Memory allocated by this function must be freed by calling
* ossFree().
*
* RETURNS: Pointer to allocated buffer, or NULL
*
*/
 
void * ossOldMalloc
    (
    UINT32 numBytes                 /* Size of buffer to allocate */
    )
 
    {
#ifdef  FILL_MEMORY
    pVOID pBfr = memalign (_CACHE_ALIGN_SIZE,
                           ROUND_UP(numBytes, _CACHE_ALIGN_SIZE));
    memset (pBfr, 0xdd, numBytes);
    return pBfr;
#else
    return memalign (_CACHE_ALIGN_SIZE,
                     ROUND_UP(numBytes, _CACHE_ALIGN_SIZE));
#endif
    }

/***************************************************************************
*
* ossCalloc - Allocates memory initialized to zeros
*
* ossCalloc() uses ossMalloc() to allocate a block of memory and then
* initializes it to zeros.  Memory allocated using this function should
* be freed using ossFree().
*
* RETURNS: Pointer to allocated buffer, or NULL. 
*
*/

pVOID ossCalloc
    (
    UINT32 numBytes		    /* size of buffer to allocate */
    )

    {
    pVOID mem;

    if ((mem = ossMalloc (numBytes)) == NULL)
	return NULL;

    memset (mem, 0, numBytes);
    return mem;
    }


/***************************************************************************
*
* ossFree - Master USB memory free routine.
*
* ossFree() calls the free routine installed in the global variable
* <ossFreeFuncPtr>.  This defaults to ossPartFree(), but can be changed
* by the user to their own defined free routine or to a non-partition
* method of malloc / free by calling ossOldInstall().
*
* RETURNS: N/A
*/

void ossFree 
    (
    pVOID bfr
    )

    {

    (*ossFreeFuncPtr) (bfr);

    }

/***************************************************************************
*
* ossPartFree - Frees globally allocated memory
*
* ossPartFree() frees memory allocated by ossMalloc().
*
* RETURNS: N/A
*/

void ossPartFree 
    (
    pVOID bfr
    )

    {
    if (bfr != NULL)
	{
	if (usbMemPartId == NULL)
	    {
	    printf ("ossFree: usbMemPartId is NULL.\n");
	    return;
	    }

	memPartFree (usbMemPartId, bfr);

	/* Update the amount of USB memory in use. */

	usbMemCount -= sizeof (bfr);

	}
    }

/***************************************************************************
*
* ossOldFree - Frees globally allocated memory
*
* ossOldFree() frees memory allocated by ossMalloc().
*
* RETURNS: N/A
*/
 
void ossOldFree
    (
    void * bfr
    )
 
    {
    if (bfr != NULL)
        free (bfr);
    }

/***************************************************************************
*
* ossOldInstall - Installs old method of USB malloc and free. 
*
* Installs old method of USB malloc and free.  This must be called before
* the call to usbdInitialize().
*
* RETURNS: N/A
*/
 
void ossOldInstall (void)
    {

    ossMallocFuncPtr = (FUNCPTR) &ossOldMalloc;
    ossFreeFuncPtr = (FUNCPTR) &ossOldFree;

    ossOldInstallFlag = TRUE;

    }

/***************************************************************************
*
* ossTime - Returns relative system time in msec
*
* Returns a count of milliseconds relative to the time the system was
* started.
*
* NOTE: The time will wrap approximately every 49 days, so time calucations
* should always be based on the difference between two time values.
*/

UINT32 ossTime (void)
    {
    return tickGet () * msecsPerTick;
    }


/***************************************************************************
*
* ossInitialize - Initializes ossLib
*
* This function should be called once at initialization in order
* to initialize the ossLib.  Calls to this function may be
* nested.  This permits multiple, indpendent libraries to use this library
* without need to coordinate the use of ossInitialize() and ossShutdown()
* across the libraries.
*
* RETURNS: OK or ERROR
*/

STATUS ossInitialize (void)
    {

    /* Get the system clock rate...used by ossThreadSleep */

    msecsPerTick = 1000 / sysClkRateGet();
   
    /*  Default to using the partition method of malloc / free. */

    if (!ossOldInstallFlag)
   	{
	ossMallocFuncPtr = (FUNCPTR) &ossPartMalloc;
        ossFreeFuncPtr = (FUNCPTR) &ossPartFree;
	}

    return OK;
    }

/* End of file. */

