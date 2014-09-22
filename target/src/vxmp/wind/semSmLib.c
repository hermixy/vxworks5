/* semSmLib.c - shared memory semaphore library (VxMP Option) */

/* Copyright 1984-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01n,06may02,mas  cache flush and volatile fix (SPR 68334); bridge flush fix
		 (SPR 68844)
01m,24oct01,mas  doc update (SPR 71149)
01l,14mar99,jdi  doc: removed refs to config.h and/or configAll.h (SPR 25663).
01k,22feb99,wsl  doc: fixed typo in errno code
01j,22mar94,pme  added WindView level event logging.
01i,24feb93,jdi  doc tweaks.
01h,08feb93,jdi  documentation cleanup.
01g,29jan93,pme  added little endian support.
01f,23nov92,jdi  documentation cleanup.
01e,13nov92,dnw  added include of smObjLib.h
01d,02oct92,pme  added SPARC support. documentation cleanup.
01c,29sep92,pme  changed semSm[BC]Create to sem[BC]SmCreate
		 comments cleanup
01b,30jul92,pme  changed semSm[BC]Init to call qFifoGInit.
		 added signal restarting.
		 semSm{Create,Init} call semSmLibInit for robustness.
		 documentation cleanup.
01a,19jul92,pme  added semSmLibInit() to reduce coupling.
                 reduced interrupt latency in semSmGive and semSmTake.
                 merged shared counting and binary semaphores.
                 code review clean up.
                 written.
*/

/*
DESCRIPTION
This library provides the interface to VxWorks shared memory binary and
counting semaphores.  Once a shared memory semaphore is created, the
generic semaphore-handling routines provided in semLib are used to
manipulate it.  Shared memory binary semaphores are created using
semBSmCreate().  Shared memory counting semaphores are created using
semCSmCreate().

Shared memory binary semaphores are used to:  (1) control mutually
exclusive access to multiprocessor-shared data structures, or (2)
synchronize multiple tasks running in a multiprocessor system.  For
general information about binary semaphores, see the manual entry
semBLib.

Shared memory counting semaphores are used for guarding multiple instances
of a resource used by multiple CPUs.  For general information about shared
counting semaphores, see the manual entry for semCLib.

For information about the generic semaphore-handling routines, see the
manual entry for semLib.

MEMORY REQUIREMENTS
The semaphore structure is allocated from a dedicated shared memory partition.

The shared semaphore dedicated shared memory partition is initialized by
the shared memory objects master CPU.  The size of this partition is
defined by the maximum number of shared semaphores, set in the configuration
parameter SM_OBJ_MAX_SEM .

This memory partition is common to shared binary and counting semaphores,
thus SM_OBJ_MAX_SEM must be set to the sum total of binary and counting
semaphores to be used in the system.

RESTRICTIONS
Shared memory semaphores differ from local semaphores in the following ways:

\is
\i `Interrupt Use:'
Shared semaphores may not be given, taken, or flushed at interrupt level.

\i `Deletion:'
There is no way to delete a shared semaphore and free its associated
shared memory.  Attempts to delete a shared semaphore return ERROR and
set `errno' to S_smObjLib_NO_OBJECT_DESTROY .

\i `Queuing Style:'
The shared semaphore queuing style specified when the semaphore is created 
must be FIFO.
\ie

INTERRUPT LATENCY
Internally, interrupts are locked while manipulating shared semaphore 
data structures, thus increasing local CPU interrupt latency.

CONFIGURATION
Before routines in this library can be called, the shared memory object
facility must be initialized by calling usrSmObjInit().  This is done
automatically during VxWorks initialization when the component INCLUDE_SM_OBJ
is included.

AVAILABILITY
This module is distributed as a component of the unbundled shared memory
support option, VxMP.

INTERNAL
To achieve full transparency, semBSmCreate() and semCSmCreate() returns 
a semaphore of type SEM_ID but the effective structure pointed to by 
this ID is a SM_SEMAPHORE.

When a task blocks on a shared semaphore a data structure called shared
memory object task control block (smObjTcb) is added to the shared 
semaphore pend queue.  This smObjTcb is allocated from a dedicated 
fixed block size shared memory partition the first time the task
blocks on a shared semaphore or after each restart signal or timeout
occuring while taking a shared semaphore.

In order to use the standard wind kernel routines we use a pseudo multi-way
queue to manipulate the shared queues and nodes (actually shared TCBs). 
We have defined a new class of queue called qFifoG for which the queue head
contains :

       - a pointer to qFifoGClass.
       - a pointer to the Fifo pending queue associated with the
         shared object where task is pending on.
       - a pointer to the shared memory object pend queue lock.  This
         pointer is initialized to NULL if we already have the lock.
         This pointer is necessary because when a timeout expires,
         when a signal is sent to the task, or when a task is deleted 
	 Q_REMOVE is called to remove the shared TCB from the semaphore 
	 pending list without having previously acquired semaphore 
	 lock access.  Thus only qFifoGRemove(), which is called by Q_REMOVE ,
         need to check if this pointer is NULL before manipulating the queue.

- Shared Semaphore Flush

The issue here is to keep a per CPU semaphore flush indivisibility while
keeping a reasonable interrupt latency.  Note that a system wide flush
indivisibility is impossible to achieve.
This implies that only one notification is made to any CPU where there's
one or more tasks pending on the flushed semaphore.

Since no modifications are made to the semaphore status, semaphore flush
is the same for binary and counting semaphores and is called semSmFlush().

- Optional Facility:

Since this facility is optional, 'flush', 'give' and 'take' routines are
called via function pointers in semFlush(), semGive(), and semTake(),
respectively, to avoid this code being imported into VxWorks when the VxMP
option is not available.
Functions pointers are initialized by semSmLibInit() which is called
in smObjInit().

routine structure:

 semBSmCreate       semCSmCreate
 |     |            |      |
(o) semSmBInit     (o) semSmCInit
         |                 |
         \                 /
          ------   --------
                \ /
             qFifoGInit
                 |
             smDllInit     



         semGive           semFlush                 semTake
            |                 |        Interrupt       | 
        semSmGive         semSmFlush   /            semSmTake
        /  |  |           /  | |      /              | |   \
       /   *  |   --------   * |     /               | *    \
      /       |  /             |    /                |       \
      |\ smObjEventSend  smObjEventProcess       smObjTcbInit/|
      | \    /   \             |       |   \         |      / |
      |  \  *     -----  windReadyQPut |    *       (i)    /  |
      |   -----------  \               |                  /   |
 windReadyQPut       \  -------------- |    windReadyQRemove  |
                      |               \|                      |
                 qFifoGRemove          |                  qFifoGPut
                      |      \         |                      |
                  smDllRemove *    smDllConcat             smDllPut



        *                      *               (o)           (i)
	|                      |                |             |
   SM_OBJ_LOCK_TAKE    SM_OBJ_LOCK_GIVE   smMemPartAlloc smFixBlkPartAlloc	

INCLUDE FILES: semSmLib.h

SEE ALSO: semLib, semBLib, semCLib, smObjLib, semShow, usrSmObjInit(),
\tb VxWorks Programmer's Guide: Shared Memory Objects, 
\tb VxWorks Programmer's Guide: Basic OS
*/

/* LINTLIBRARY */

#include "vxWorks.h"
#include "errnoLib.h"
#include "objLib.h"
#include "classLib.h"
#include "qFifoGLib.h"
#include "semLib.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "taskLib.h"
#include "cacheLib.h"
#include "intLib.h"
#include "smObjLib.h"
#include "netinet/in.h"
#include "private/semSmLibP.h"
#include "private/windLibP.h"
#include "private/smObjLibP.h"
#include "private/smMemLibP.h"
#include "private/sigLibP.h"
#include "private/eventP.h"

/* locals */

LOCAL BOOL  semSmLibInstalled;	/* protect from muliple inits */

/*****************************************************************************
*
* semSmLibInit - initialize the shared semaphore management package
*
* This routine initializes the shared memory semaphore library by installing
* the addresses of its give, take, and flush routines for both binary and
* counting semaphores in the kernel's semaphore function dispatch table.
*
* SEE ALSO: smObjLibInit().
*
* RETURNS: N/A
*
* NOMANUAL
*/

void semSmLibInit (void)
    {
    if ((!semSmLibInstalled) & (semLibInit () == OK))
	{
    	/* fill the semaphore function tables */

    	semGiveTbl  [SEM_TYPE_SM_BINARY]	= (FUNCPTR) semSmGive;
    	semTakeTbl  [SEM_TYPE_SM_BINARY]	= (FUNCPTR) semSmTake;
    	semFlushTbl [SEM_TYPE_SM_BINARY]	= (FUNCPTR) semSmFlush;
    	semGiveTbl  [SEM_TYPE_SM_COUNTING]	= (FUNCPTR) semSmGive;
    	semTakeTbl  [SEM_TYPE_SM_COUNTING]	= (FUNCPTR) semSmTake;
    	semFlushTbl [SEM_TYPE_SM_COUNTING]	= (FUNCPTR) semSmFlush;

	semSmLibInstalled = TRUE;
	}
    }

/*****************************************************************************
*
* semBSmCreate - create and initialize a shared memory binary semaphore (VxMP Option)
*
* This routine allocates and initializes a shared memory binary semaphore.
* The semaphore is initialized to an <initialState> of either
* SEM_FULL (available) or SEM_EMPTY (not available).  The shared semaphore
* structure is allocated from the shared semaphore dedicated memory
* partition.
*
* The semaphore ID returned by this routine can be used directly by the
* generic semaphore-handling routines in semLib -- semGive(), semTake(), and
* semFlush() -- and the show routines, such as show() and semShow().
*
* The queuing style for blocked tasks is set by <options>; the only
* supported queuing style for shared memory semaphores is first-in-first-out,
* selected by SEM_Q_FIFO .
*
* Before this routine can be called, the shared memory objects facility must
* be initialized (see semSmLib).
*
* The maximum number of shared memory semaphores (binary plus counting) that
* can be created is SM_OBJ_MAX_SEM , a configurable parameter.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled shared memory
* support option, VxMP.
* 
* RETURNS: The semaphore ID, or NULL if memory cannot be allocated 
* from the shared semaphore dedicated memory partition.
* 
* ERRNO: S_memLib_NOT_ENOUGH_MEMORY, S_semLib_INVALID_QUEUE_TYPE,
* S_semLib_INVALID_STATE, S_smObjLib_LOCK_TIMEOUT
*
* SEE ALSO: semLib, semBLib, smObjLib, semShow,
* \tb VxWorks Programmer's Guide: Basic OS
*
* INTERNAL
* The least significant bit of the semaphore id is set to 1 in order to
* differentiate shared and local semaphores.
*/

SEM_ID semBSmCreate
    (
    int		options,	/* semaphore options */
    SEM_B_STATE	initialState	/* initial semaphore state */
    )
    {
    SM_SEM_ID   smSemId;
    int         temp;           /* temp storage */

    /* 
     * Allocate semaphore structure from shared semaphores
     * dedicated shared memory partition. 
     */
  
    smSemId = (SM_SEM_ID) smMemPartAlloc ((SM_PART_ID) smSemPartId,
                                          sizeof (SM_SEMAPHORE));

    if (smSemId == NULL)
        {
	return (NULL);
        }

    /* clear shared semaphore structure */

    bzero ((char *) smSemId, sizeof (SM_SEMAPHORE));

    /* initialize allocated semaphore */

    if (semSmBInit ((SM_SEMAPHORE *) (smSemId), options, initialState) != OK)
    	{
    	smMemPartFree ((SM_PART_ID) smSemPartId, (char *) smSemId);
    	return (NULL);
    	}

    /* update shared memory objects statistics */

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    temp = pSmObjHdr->curNumSemB;               /* PCI bridge bug [SPR 68844]*/

    pSmObjHdr->curNumSemB = htonl (ntohl (pSmObjHdr->curNumSemB) + 1);

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    temp = pSmObjHdr->curNumSemB;               /* BRIDGE FLUSH  [SPR 68334] */

    return ((SEM_ID) (SM_OBJ_ADRS_TO_ID (smSemId)));
    }

/*****************************************************************************
*
* semSmBInit - initialize a declared shared binary semaphore
*
* The initialization of a static shared binary semaphore, or a shared binary
* semaphore embedded in some larger shared object need not deal with
* allocation.  This routine may be called to initialize such a semaphore.  The
* semaphore is initialized to the specified initial state of either SEM_FULL
* or SEM_EMPTY .
*
* Binary semaphore options include the queuing style for blocked tasks.
* For now, the only available shared semaphore queueing style is
* first-in-first-out, type SEM_Q_FIFO defined as 0 in semLib.h.
*
* The semaphore address parameter is the local address of a shared semaphore
* structure. 
*
* RETURNS: OK, or ERROR if queue type or initial state is invalid.
* 
* SEE ALSO: semBSmCreate
*
* ERRNO: S_semLib_INVALID_QUEUE_TYPE, S_semLib_INVALID_STATE   
*
* NOMANUAL
*/

STATUS semSmBInit
    (
    SM_SEMAPHORE * pSem,                /* pointer to semaphore to initialize */
    int            options,             /* semaphore options */
    SEM_B_STATE    initialState         /* initial semaphore state */
    )
    {
    Q_FIFO_G_HEAD           pseudoPendQ; /* pseudo pendQ to init sem pend Q */
    SM_SEMAPHORE volatile * pSemv = (SM_SEMAPHORE volatile *) pSem;
    int                     temp;        /* temp storage */

    if (!semSmLibInstalled)
        {
	semSmLibInit ();		/* initialize package */
        }

    if (options != SEM_Q_FIFO)		/* only FIFO queuing for now */
	{
	errno = S_semLib_INVALID_QUEUE_TYPE;
	return (ERROR);
	}

    pSemv->objType = htonl (SEM_TYPE_SM_BINARY); /* semaphore type is binary */
    pSemv->lock    = 0;				/* lock available */

    /* initialize pseudo multi way queue */

    pseudoPendQ.pLock   = NULL;			/* we already have the lock */
    pseudoPendQ.pFifoQ  = &pSem->smPendQ;	/* address of actual queue */
    pseudoPendQ.pQClass = qFifoGClassId;	/* global fifo multi way Q */

    qFifoGInit (&pseudoPendQ);         		/* initialize sem pend Q */

    /* fill state flag according to initial state */

    switch (initialState)
        {
        case SEM_EMPTY:
        case SEM_FULL:
            pSemv->state.flag = htonl (initialState);   
            break;
        default:
            errno = S_semLib_INVALID_STATE;
            return (ERROR);
        }

    /* verify field must contain the global address of semaphore */

    pSemv->verify = (UINT32) htonl (LOC_TO_GLOB_ADRS (pSem));	

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    temp = pSemv->lock;                         /* BRIDGE FLUSH  [SPR 68334] */

    return (OK);
    }

/*****************************************************************************
*
* semCSmCreate - create and initialize a shared memory counting semaphore (VxMP Option)
*
* This routine allocates and initializes a shared memory counting
* semaphore.  The initial count value of the semaphore is specified
* by <initialCount>.
*
* The semaphore ID returned by this routine can be used directly by the
* generic semaphore-handling routines in semLib -- semGive(), semTake() and
* semFlush() -- and the show routines, such as show() and semShow().
*
* The queuing style for blocked tasks is set by <options>; the only
* supported queuing style for shared memory semaphores is first-in-first-out,
* selected by SEM_Q_FIFO .
*
* Before this routine can be called, the shared memory objects facility must
* be initialized (see semSmLib).
*
* The maximum number of shared memory semaphores (binary plus counting) that
* can be created is SM_OBJ_MAX_SEM , a configurable paramter.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled shared memory
* support option, VxMP.
* 
* RETURNS: The semaphore ID, or NULL if memory cannot be allocated
* from the shared semaphore dedicated memory partition.
*
* ERRNO: S_memLib_NOT_ENOUGH_MEMORY, S_semLib_INVALID_QUEUE_TYPE,
* S_smObjLib_LOCK_TIMEOUT
*
* SEE ALSO: semLib, semCLib, smObjLib, semShow,
* \tb VxWorks Programmer's Guide: Basic OS
*
* INTERNAL
* The least significant bit of the semaphore ID is set to 1 in order to
* differentiate shared and local semaphores.
*/

SEM_ID semCSmCreate
    (
    int options,	/* semaphore options */
    int initialCount	/* initial semaphore count */
    )
    {
    SM_SEM_ID smSemId;
    int       temp;     /* temp storage */

    /* allocate semaphore structure from shared semaphore dedicated pool */

    smSemId = (SM_SEM_ID) smMemPartAlloc ((SM_PART_ID) smSemPartId,
                                          sizeof (SM_SEMAPHORE));

    if (smSemId == NULL)
        {
	return (NULL);
        }

    bzero ((char *) smSemId, sizeof(SM_SEMAPHORE));

    /* initialize allocated semaphore */

    if (semSmCInit ((SM_SEMAPHORE *) (smSemId), options, initialCount) != OK)
        {
        smMemPartFree ((SM_PART_ID) smSemPartId, (char *) smSemId);
        return (NULL);
        }

    /* update shared memory objects statistics */

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    temp = pSmObjHdr->curNumSemC;               /* PCI bridge bug [SPR 68844]*/

    pSmObjHdr->curNumSemC = htonl (ntohl (pSmObjHdr->curNumSemC) + 1);

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    temp = pSmObjHdr->curNumSemC;               /* BRIDGE FLUSH  [SPR 68334] */

    return ((SEM_ID) (SM_OBJ_ADRS_TO_ID (smSemId)));
    }

/*****************************************************************************
*
* semSmCInit - initialize a declared shared counting semaphore
*
* The initialization of a static counting semaphore, or a shared 
* counting semaphore embedded in some larger object need not deal 
* with allocation.
* This routine may be called to initialize such a semaphore.  The semaphore
* <pSem> is initialized to the specified <initialCount>.
*
* Counting semaphore <options> include the queuing style for blocked tasks.
* The only available pending queue type is first-in-first-out, type
* SEM_Q_FIFO defined as 0 in semLib.h.
*
* The semaphore address parameter <pSem> is the local address of a
* shared semaphore data structure.
*
* RETURNS: OK or ERROR if queue type invalid.
*
* ERRNO: S_semLib_INVALID_QUEUE_TYPE
*
* SEE ALSO: semCSmCreate()
*
* NOMANUAL
*/

STATUS semSmCInit
    (
    SM_SEMAPHORE * pSem,                /* pointer to semaphore to initialize */
    int            options,             /* semaphore options */
    int            initialCount         /* initial semaphore count */
    )
    {
    Q_FIFO_G_HEAD           pseudoPendQ; /* pseudo pendQ to init sem pend Q */
    SM_SEMAPHORE volatile * pSemv = (SM_SEMAPHORE volatile *) pSem;
    int                     temp;        /* temp storage */

    if (!semSmLibInstalled)
        {
        semSmLibInit ();                /* initialize package */
        }

    if (options != SEM_Q_FIFO)          /* only FIFO queuing for now */
        {
        errno = S_semLib_INVALID_QUEUE_TYPE;
        return (ERROR);
        }

    pSemv->objType = htonl(SEM_TYPE_SM_COUNTING);/* fill semaphore type */
    pSemv->lock    = 0;				/* lock available */

    /* initialize pseudo multi way queue */

    pseudoPendQ.pLock   = NULL;			/* we already have the lock */
    pseudoPendQ.pFifoQ  = &pSem->smPendQ;	/* address of actual queue */
    pseudoPendQ.pQClass = qFifoGClassId;	/* global fifo multi way Q */

    qFifoGInit (&pseudoPendQ);         		/* initialize sem pend Q */

    pSemv->state.count = htonl (initialCount); 	/* set initial count */

    /* verify field must contain the global address of semaphore */

    pSemv->verify = (UINT32) htonl (LOC_TO_GLOB_ADRS (pSem));

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    temp = pSemv->lock;                         /* BRIDGE FLUSH  [SPR 68334] */

    return (OK);
    }

/*****************************************************************************
*
* semSmGive - give shared memory binary or counting semaphore
*
* Gives the semaphore.  If a higher priority task has already taken
* the semaphore (so that it is now pended waiting for it), that task
* will now become ready to run.  If that task is local to this CPU and is of
* higher priority than the task that does the semGive, it will preempt this
* task.  If the first pended task is located on a remote processor, the
* availability of the semaphore is made known to the remote processor via
* an internal notification mecanism.
* 
* For a shared binary semaphore, if the semaphore is already full (it has
* been given but not taken), this call is essentially a no-op.
*
* For a shared counting semaphore, if the semaphore count is already
* greater than 0 (semaphore was given but not taken), this call is
* essentially a no-op.
*
* The <smSemId> passed to this routine must be the local semaphore address.
*
* This routine is usually called by semGive() which first converts the
* semaphore ID to the shared semaphore local address.
*
* WARNING
* This routine may not be used from interrupt level.
*
* RETURNS: OK or ERROR.
*
* ERRNO: S_intLib_NOT_ISR_CALLABLE, S_objLib_OBJ_ID_ERROR,
*        S_smObjLib_LOCK_TIMEOUT
*
* NOMANUAL
*/

STATUS semSmGive
    (
    SM_SEM_ID smSemId           /* global semaphore ID to give */
    )
    {
    Q_FIFO_G_HEAD pendQ;        /* temporary pendQ to unpend task */
    int           level;	/* processor specific inLock return value */
    SM_DL_LIST    eventList;	/* list of events used to notify give */
    SM_OBJ_TCB volatile *  firstSmTcb;   /* first pending shared memory tcb */
    SM_SEM_ID volatile     smSemIdv = (SM_SEM_ID volatile) smSemId;
    int                    temp;         /* temp storage */
    
    if (INT_RESTRICT () != OK)		/* not ISR callable */
        {
	return (ERROR);
        }

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    temp = smSemIdv->verify;                    /* PCI bridge bug [SPR 68844]*/

    if (SM_OBJ_VERIFY (smSemIdv) != OK)	/* check semaphore id */
	{
	return (ERROR);
	}

    /* ENTER LOCKED SECTION */

    if (SM_OBJ_LOCK_TAKE (&smSemId->lock, &level) != OK) 
	{
	smObjTimeoutLogMsg ("semGive", (char *) &smSemId->lock);
	return (ERROR);                         /* can't take lock */
	}

    /* get first pending shared TCB */

    firstSmTcb = (SM_OBJ_TCB volatile *) SM_DL_FIRST (&smSemId->smPendQ);

    /* pendQ is empty */

    if (firstSmTcb == (SM_OBJ_TCB volatile *) LOC_NULL)
	{
        /* binary semaphore: sem available */

	if (ntohl (smSemIdv->objType) == SEM_TYPE_SM_BINARY) 
	    {
	    smSemIdv->state.flag = htonl (SEM_FULL);
	    }

        /* counting semaphore: increment sem count */

        else
	    {
	    smSemIdv->state.count = htonl (ntohl (smSemIdv->state.count) + 1);
	    }

        /* EXIT LOCKED SECTION */

	SM_OBJ_LOCK_GIVE (&smSemId->lock, level);

	return (OK);
	}

    /* pendQ is not empty, initialize pseudo multi way queue */

    pendQ.pLock   = NULL;			/* we already have the lock */
    pendQ.pFifoQ  = &smSemId->smPendQ;	 	/* address of actual queue */
    pendQ.pQClass = qFifoGClassId;		/* global fifo multi way Q */

    /* 
     * Do now the necessary manipulations in shared memory
     * in order to be able to release lock access ASAP
     * thus reducing interrupt latency.
     */

    /* ENTER KERNEL */

    kernelState = TRUE;

    /* remove from pending list */

    Q_REMOVE (&pendQ, (SM_DL_NODE *) firstSmTcb);

    /* EXIT LOCKED SECTION */

    SM_OBJ_LOCK_GIVE (&smSemId->lock, level);

    /* pending task is local */

    if (ntohl (firstSmTcb->ownerCpu) == smObjProcNum)
	{
	/* 
	 * The shared TCB has been removed from the shared semaphore
	 * pendQ by qFifoGRemove(), during that call removedByGive is
	 * set to TRUE to avoid non-null notification time race.
	 * Then, if the task that has taken the semaphore is remote,
	 * an event notification is done and removedByGive is reset
	 * by smObjEventProcess() on the remote processor.
	 * If the pended task is local removedByGive must be reset
	 * here to avoid the shared TCB being screwed up.
	 */ 

	firstSmTcb->removedByGive = htonl (FALSE);

	/* windview - level 2 event logging */

	EVT_TASK_1 (EVENT_OBJ_SEMGIVE, smSemId);

        CACHE_PIPE_FLUSH ();                    /* CACHE FLUSH   [SPR 68334] */
        temp = (int) firstSmTcb->localTcb;      /* PCI bridge bug [SPR 68844]*/

	windReadyQPut ((WIND_TCB *) ntohl ((int) firstSmTcb->localTcb));
	}

    /* pending task is remote */

    else
	{
 	/* 
	 * Add the shared TCB that was pended on the semaphore
	 * pend queue to the list of events to send to the
	 * CPU where the pended task is located.
	 */

	eventList.head = (SM_DL_NODE *) htonl (LOC_TO_GLOB_ADRS (firstSmTcb));
	eventList.tail = eventList.head;

	/* 
	 * Unlink the shared TCB from other shared TCBs of the semaphore
	 * pend queue.
	 */

	firstSmTcb->qNode.next = NULL;

 	/* notify remote CPU */

        CACHE_PIPE_FLUSH ();                    /* CACHE FLUSH   [SPR 68334] */
        temp = firstSmTcb->ownerCpu;            /* PCI bridge bug [SPR 68844]*/

	if (smObjEventSend (&eventList, ntohl (firstSmTcb->ownerCpu)) != OK) 
	    {
    	    windExit ();				/* EXIT KERNEL */
	    return (ERROR);
	    }
	}

    windExit ();					/* EXIT KERNEL */

    return (OK);
    }

/*****************************************************************************
*
* semSmTake - take shared memory binary or counting semaphore
*
* Takes a shared semaphore.  If the semaphore is empty, i.e., it has not been
* given since the last semTake() or semInit(), this task will become pended
* until the semaphore becomes available by some other local or remote task
* doing a semGive() of it or if the optional <timeout> value is not NO_WAIT,
* in which case the task will pend for no more than <timeout> system clock
* ticks.  If <timeout> is WAIT_FOREVER , the task may never return if a remote
* task has the semaphore and crashes.
*
* For binary semaphores, if the semaphore is already available, this
* call will empty the semaphore, so that no other task can take it
* until this task gives it back, and this task will continue running.
*
* For counting semaphores, if the semaphore count is already greater
* than 0, this call will decrement the semaphore count, and this task
* will continue running.
*
* The <smSemId> passed to this routine must be the local semaphore address.
* 
* This routine is usually called by semTake() which first converts the
* semaphore ID to the shared semaphore local address.
*
* WARNING
* This routine may not be used from interrupt level.
*
* RETURNS: OK or ERROR.
*
* ERRNO: S_objLib_OBJ_UNAVAILABLE, S_intLib_NOT_ISR_CALLABLE,
*        S_objLib_OBJ_ID_ERROR, S_objLib_OBJ_TIMEOUT, S_smObjLib_LOCK_TIMEOUT,
*        S_smMemLib_NOT_ENOUGH_MEMORY
*
* NOMANUAL
*
* INTERNAL
* When semSmTake is called for the first time or after a timeout or a
* RESTART signal occuring while this task was blocked on a shared
* semaphore, a shared TCB is created in shared memory.
* This is done by allocating a shared TCB structure
* from a shared partition dedicated to shared TCBs.  A pointer to this
* shared TCB is then initialized in the local Task Control Block.
*/

STATUS semSmTake 
    (
    SM_SEM_ID smSemId,     /* global semaphore ID to take */
    int       timeout      /* timeout in ticks */
    )
    {
    Q_FIFO_G_HEAD      pendQ;	/* global FIFO to pend on */
    int                level;   /* processor specific inLock return value */
    int                status;  /* returned status */
    WIND_TCB *         pTcb;	/* current task tcb pointer */
    SM_SEM_ID volatile smSemIdv = (SM_SEM_ID volatile) smSemId;
    int                temp;    /* temp storage */


    if (INT_RESTRICT () != OK)		/* not ISR callable */
        {
	return (ERROR);
        }

again:
    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    temp = smSemIdv->verify;                    /* PCI bridge bug [SPR 68844]*/

    if (SM_OBJ_VERIFY (smSemIdv) != OK)	/* check semaphore */
	{
	return (ERROR);
	}

    /* 
     * If it's the first call to semSmBTake, or if a timeout or a
     * RESTART signal has occured while this task was blocked on a shared
     * semaphore, the pSmObjTcb field of the TCB is NULL, we allocate
     * and initialize a shared TCB.
     */
    
    pTcb = (WIND_TCB *) taskIdCurrent;
    if (pTcb->pSmObjTcb == NULL)
	{
	if (smObjTcbInit () != OK)
	    {
	    return  (ERROR);
	    }
	}

    /* ENTER LOCKED SECTION */

    if (SM_OBJ_LOCK_TAKE (&smSemId->lock, &level) != OK)
	{
	smObjTimeoutLogMsg ("semTake", (char *) &smSemId->lock);
	return (ERROR);                         /* can't take lock */
	}
    
    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    temp = smSemIdv->objType;                   /* PCI bridge bug [SPR 68844]*/

    if (ntohl (smSemIdv->objType) == SEM_TYPE_SM_BINARY) /* binary semaphore */
	{
    	if (ntohl (smSemIdv->state.flag) == SEM_FULL)
	    {
	    smSemIdv->state.flag = htonl (SEM_EMPTY); /* set sem unavailable */

	    SM_OBJ_LOCK_GIVE (&smSemId->lock, level); /* EXIT LOCKED SECTION */
	    return (OK);
	    }
	}
    else 					/* counting samaphore */
	{
    	if (ntohl (smSemId->state.count) > 0)
            {
	    /* decrement semaphore count */

            smSemId->state.count = htonl (ntohl (smSemId->state.count) - 1);

            SM_OBJ_LOCK_GIVE(&smSemId->lock, level);  /* EXIT LOCKED SECTION */
            return (OK);
            }
   	} 

    kernelState = TRUE;  			/* ENTER KERNEL */

    if (timeout == NO_WAIT)                           /* NO_WAIT = no block */
        {
        SM_OBJ_LOCK_GIVE (&smSemId->lock, level); /* EXIT LOCKED SECTION */

        errno = S_objLib_OBJ_UNAVAILABLE;             /* resource gone */

        windExit ();                                  /* KERNEL EXIT */
        return (ERROR);
        }

    /* 
     * We get here if the semaphore is not available and we must wait.
     * The calling task must be added to the shared semaphore
     * queue using standard wind kernel functions.  For that we
     * initialize a pseudo multi-way queue header that points to
     * the shared semaphore pend queue and which will be used
     * by the multi-way queue manipulation macros.
     */

    /* 
     * We optimize interrupt latency by doing only the Q_PUT on the
     * shared semaphore pend Q while interrupts are locked.
     * The remaining part of blocking the task is done in
     * windReadyQRemove() with interrupts unlocked.
     */

    pendQ.pLock   = NULL;			/* we already have the lock */
    pendQ.pFifoQ  = &smSemId->smPendQ;	 	/* address of actual queue */
    pendQ.pQClass = qFifoGClassId;		/* global fifo multi way Q */

    Q_PUT (&pendQ, taskIdCurrent, taskIdCurrent->priority);

    SM_OBJ_LOCK_GIVE (&smSemId->lock, level); 	/* EXIT LOCKED SECTION */

    pendQ.pLock  = &smSemId->lock;		/* now we don't have lock */

    /* windview - level 2 event logging */

    EVT_TASK_1 (EVENT_OBJ_SEMTAKE, smSemId);

    windReadyQRemove ((Q_HEAD *) &pendQ, timeout);	/* block task */

    if ((status = windExit ()) == RESTART)      /* KERNEL EXIT */
        {
        timeout = SIG_TIMEOUT_RECALC (timeout);
        goto again;
        }

    return (status);
    }

/*****************************************************************************
*
* semSmFlush - flush shared binary or counting semaphore
*
* Flush the shared semaphore.  If one or more tasks have already taken
* the semaphore (so that it is now pended waiting for it), those tasks
* will now become ready to run.  If a local higher priority task was
* pending on the semaphore it will preempt the task that does the semFlush().
* If the semaphore pend queue is empty, this call is essentially a no-op.
* The smSemId passed to this routine must be the local semaphore address.
*
* This routine is usually called by semFlush() which first converts the
* semaphore ID to the shared semaphore local address.
*
* WARNING
* This routine may not be used from interrupt level.
*
* RETURNS: OK or ERROR.
*
* ERRNO: S_intLib_NOT_ISR_CALLABLE, S_objLib_OBJ_ID_ERROR,
*        S_smObjLib_LOCK_TIMEOUT
*
* NOMANUAL
*/

STATUS semSmFlush
    (
    SM_SEM_ID smSemId        	    /* global semaphore ID to flush */
    )
    {
    SM_OBJ_TCB volatile * firstSmTcb;   /* first pending shared memory TCB */
    SM_OBJ_TCB volatile * pSmObjTcb;    /* useful shared TCB pointer */ 
    SM_OBJ_TCB volatile * pSmObjTcbTmp; /* useful shared TCB pointer */   
    int                   level;        /* CPU specific inLock return value */
    int                   cpuNum;       /* loop counter for remote flush */
    SM_DL_LIST            flushQ [SM_OBJ_MAX_CPU];  /* per CPU flush Q */
    SM_SEM_ID volatile    smSemIdv = (SM_SEM_ID volatile) smSemId;
    int                   temp;         /* temp storage */
    
    if (INT_RESTRICT () != OK)			/* not ISR callable */
        {
	return (ERROR);
        }

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    temp = smSemIdv->verify;                    /* PCI bridge bug [SPR 68844]*/

    if (SM_OBJ_VERIFY (smSemIdv) != OK)		/* check semaphore */
	{
	return (ERROR);
	}

    /* ENTER LOCKED SECTION */

    if (SM_OBJ_LOCK_TAKE (&smSemId->lock, &level) != OK)
	{
	smObjTimeoutLogMsg ("semFlush", (char *) &smSemId->lock);
	return (ERROR);                         /* can't take lock */
	}

    /* 
     * In order to reduce interrupt latency we get the first
     * shared TCB from the shared semaphore pend queue, then
     * we empty the pend queue by clearing the pend queue head.
     * All the previously pended shared TCBs are still linked
     * to the first pending TCB and we can give back lock access
     * to the shared semaphore.
     */

    firstSmTcb = (SM_OBJ_TCB volatile *) SM_DL_FIRST (&smSemId->smPendQ);

    if (firstSmTcb == (SM_OBJ_TCB volatile *) LOC_NULL)  /* pendQ is empty */
	{
	SM_OBJ_LOCK_GIVE (&smSemId->lock, level);/* EXIT LOCKED SECTION */
	return (OK);
	}

    smSemIdv->smPendQ.head = NULL;	    /* empty semaphore pend queue */
    smSemIdv->smPendQ.tail = NULL;

    SM_OBJ_LOCK_GIVE (&smSemId->lock, level);	/* EXIT LOCKED SECTION */

    /* create a list of pending task per cpu */

    bzero ((char *) flushQ, sizeof (flushQ)); 	/* clear flush Q table */
    pSmObjTcb = firstSmTcb;
    do
	{
	/* get next shared TCB behind current shared TCB */	

        pSmObjTcbTmp = (SM_OBJ_TCB volatile *) SM_DL_NEXT (pSmObjTcb);

	/* add the shared TCB to its CPU flush Queue */

	smDllAdd (&flushQ [ntohl (pSmObjTcb->ownerCpu)], 
		  (SM_DL_NODE *) pSmObjTcb);

        /* make current shared TCB be next shared TCB */

	pSmObjTcb = pSmObjTcbTmp;

	} while (pSmObjTcbTmp != LOC_NULL);

    /* notify remote flush */

    for (cpuNum = 0; cpuNum < SM_OBJ_MAX_CPU; cpuNum ++)
	{
	if ((SM_DL_FIRST (&flushQ [cpuNum]) != (int) LOC_NULL) && 
	    (cpuNum != smObjProcNum))
	    {
	    /* 
	     * There is one or more task running on cpuNum to unblock and
	     * cpuNum is not the local CPU so notify cpuNum.
	     */

	    if (smObjEventSend (&flushQ [cpuNum], cpuNum) != OK)
	        {
                return (ERROR);
	        }
	    }
	}

    /* perform local flush if needed */

    if (SM_DL_FIRST (&flushQ [smObjProcNum]) != (int) LOC_NULL)
	{
	kernelState = TRUE;				/* ENTER KERNEL */

	/*
	 * Since all tasks have been removed from the shared
	 * semaphore pend queue, unblocking them now consists
	 * of putting all the tasks for which the shared TCBs
	 * are in the flush Queue in the ready Queue.
	 * This is exacly what smObjEventProcess() does on the remote
	 * side so we just call it.
	 */

	smObjEventProcess (&flushQ [smObjProcNum]);

	windExit ();					/* EXIT KERNEL */
	}

    return (OK);
    }

