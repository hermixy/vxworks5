/* smObjLib.c - shared memory objects library (VxMP Option) */

/* Copyright 1984-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01s,03may02,mas  cache flush and volatile fix (SPR 68334); bridge flush fix
		 (SPR 68844)
01r,24oct01,mas  fixed diab warnings (SPR 71120); doc update (SPR 71149)
01q,20sep01,jws  doc change for SPR68418
01p,16mar99,dat  fixed declaration of smUtilTasClearRtn. (SPR 25804)
01o,14mar99,jdi  doc: removed refs to config.h and/or configAll.h (SPR 25663).
01n,22feb99,wsl  DOC: removed all reference to spurious error code.
01l,05may93,caf  tweaked for ansi.
01l,17mar94,pme  Added WindView level 2 event logging.
		 Added comments about acknowledge of BUS interrupts (SPR# 2560).
01k,13mar93,jdi  code style fix for mangen.
01j,14feb93,jdi  documentation cleanup for 5.1.
01i,29jan93,pme  added little endian support.
		 added smObjLibInit() manual. Made smObjLibInit() return
		 ERROR if already initialized.
		 suppressed ntohl() macros in smObjCpuInfoGet().
		 aligned partition addresses.
01h,08dec92,jdi  documentation cleanup.
01g,02oct92,pme  added SPARC support. documentation cleanup.
01f,29sep92,pme  fixed bug in smObjInit (objCpuTbl was not converted to
		 local value in pSmObjHdr).
		 Added version number. comments cleanup.
		 set PART_OVH to 32 bytes.
01e,23aug92,jcf  cleanup.
01d,30jul92,pme  added SM_OBJ_SIZE(), cleaned up smObjSetup.
                 changed parameter to smObjBeat.
		 added smObjTasClearRoutine for use with smLockGive().
		 added shared memory layout chart.
01c,24jul92,elh  changed parameters to smIsAlive.
01b,24jul92,gae  fixed number of parameters to logMsg().
01a,07feb92,pme  written.
*/

/*
DESCRIPTION
This library contains miscellaneous functions used by the shared memory
objects facility (VxMP).  Shared memory objects provide high-speed
synchronization and communication among tasks running on separate CPUs that
have access to a common shared memory.  Shared memory objects are system
objects (e.g., semaphores and message queues) that can be used across
processors.

The main uses of shared memory objects are interprocessor synchronization,
mutual exclusion on multiprocessor shared data structures, and high-speed
data exchange.

Routines for displaying shared memory objects statistics are provided  
by the smObjShow module.

SHARED MEMORY MASTER CPU
One CPU node acts as the shared memory objects master.  This CPU
initializes the shared memory area and sets up the shared memory anchor.
These steps are performed by the master calling smObjSetup().  This
routine should be called only once by the master CPU.  Usually smObjSetup()
is called from usrSmObjInit().  (See "Configuration" below.)

Once smObjSetup() has completed successfully, there is little functional
difference between the master CPU and other CPUs using shared memory objects,
except that the master is responsible for maintaining the heartbeat in
the shared memory objects header.

ATTACHING TO SHARED MEMORY
Each CPU, master or non-master, that will use shared memory objects
must attach itself to the shared memory objects facility, which
must already be initialized.

Before it can attach to a shared memory region, each CPU must allocate
and initialize a shared memory descriptor (SM_DESC), which describes the
individual CPU's attachment to the shared memory objects facility.  Since
the shared memory descriptor is used only by the local CPU, it is not
necessary for the descriptor itself to be located in shared memory.  In
fact, it is preferable for the descriptor to be allocated from the CPU's
local memory, since local memory is usually more efficiently accessed.

The shared memory descriptor is initialized by calling smObjInit().  This
routine takes a number of parameters which specify the characteristics of
the calling CPU and its access to shared memory.

Once the shared memory descriptor has been initialized, the CPU can
attach itself to the shared memory region.  This is done by calling
smObjAttach().

When smObjAttach() is called, it verifies that the shared memory anchor
contains the value SM_READY and that the heartbeat located in the shared
memory objects header is incrementing.  If either of these conditions is
not met, the routine will check periodically until either SM_READY or an
incrementing heartbeat is recognized or a time limit is reached.  The
limit is expressed in seconds, and 600 seconds (10 minutes) is the
default.  If the time limit is reached before SM_READY or a heartbeat is
found, ERROR is returned and `errno' is set to S_smLib_DOWN .

ADDRESS CONVERSION
This library also provides routines for converting between local and
global shared memory addresses, smObjLocalToGlobal() and
smObjGlobalToLocal().  A local shared memory address is the address
required by the local CPU to reach a location in shared memory.  A global
shared memory address is a value common to all CPUs in the system used to
reference a shared memory location.  A global shared memory address is
always an offset from the shared memory anchor.

SPIN-LOCK MECHANISM
The shared memory objects facilities use a spin-lock mechanism based on an
indivisible read-modify-write (RMW) operation on a shared memory location
which acts as a low-level mutual exclusion device.  The spin-lock mechanism
is called with a system-wide configuration parameter,
SM_OBJ_MAX_TRIES, which specifies the maximum number of RMW tries on a
spin-lock location.

Care must be taken that the number of RMW tries on a spin-lock on a
particular CPU never reaches SM_OBJ_MAX_TRIES, otherwise system
behavior becomes unpredictable.  The default value should be sufficient
for reliable operation.

The routine smObjTimeoutLogEnable() can be used to enable or disable the
printing of a message should a shared memory object call fail while trying
to take a spin-lock.

RELATION TO BACKPLANE DRIVER
Shared memory objects and the shared memory network (backplane) driver use 
common underlying shared memory utilities.  They also use the same anchor, 
the same shared memory header, and the same interrupt when they are used at
the same time.

LIMITATIONS
A maximum of twenty CPUs can be used concurrently with shared memory
objects.  Each CPU in the system must have a hardware test-and-set (TAS)
mechanism, which is called via the system-dependent routine sysBusTas().

The use of shared memory objects raises interrupt latency, because internal
mechanisms lock interrupts while manipulating critical shared data
structures.  Interrupt latency does not depend on the number of objects or
CPUs used.

GETTING STATUS INFORMATION
The routine smObjShow() displays useful information regarding the current
status of shared memory objects, including the number of tasks using
shared objects, shared semaphores, and shared message queues, the number
of names in the database, and also the maximum number of tries to get
spin-lock access for the calling CPU.

INTERNAL

Routine structure

    smObjSetup smObjLocalToGlobal smObjGlobalToLocal   smObjAttach   smObjInit
     /    \                                            /    |            |
 smSetup   ------------------------                smAttach |          smInit
          /                        \                        |
   smMemPartInit               smNameInit                   |    
                                                            |
                                                            |
                                                            |
  smObjEventSend-*             smObjNotifyHandler-*         | 
   /   |      |                 /      |                    |
  /    \ smObjCpuInfoGet       /  smObjEventProcess         |     smObjTcbInit  
  |     -------------  --------                             |
smUtilIntGen         \/                              smUtilIntConnect
                      |
                  smDllConcat

        *                      *                     (o)
        |                      |                      |
   SM_OBJ_LOCK_TAKE    SM_OBJ_LOCK_GIVE         smMemPartAlloc


Memory layout

 Shared Memory         Shared Memory           Shared Memory
    Anchor              Descriptor             CPU Descriptors
 (SM_ANCHOR)             (SM_HDR)              (SM_CPU_DESC)
--------------   ----->--------------   ------>--------------
|readyValue  |   |     | tasType    |   |      | status     |
|    .       |   |     | maxCpus    |   |      | intType    |
|smHeader    |----     | cpuTable   |----      | intArg1    | CPU 0 
|    .       |         |     .      |          | intArg2    | descriptor
|    .       |         |     .      |          | intArg3    |
|smPktHeader |----     |     .      |          |------------|
|    .       |   |     |     .      |          |    .       | CPU 1
|smObjHeader |-- |     --------------          |------------|
|    .       | | |                             |            |
|    .       | | |                             ~            ~
|    .       | | |                             |            | CPU n
-------------- | |                             --------------
               | |   Shared Memory Packet
               | |     Memory Header 
               | |    (SM_PKT_MEM_HDR)
               | ----->--------------                             Free Packets
               |       | heartBeat  |                        ---->-------------
               |       | freeList   |------------------------|    |           |
               |       | pktCpuTbl  |----                         |           |
               |       | maxPktBytes|   |                         |           |
               |       |     .      |   |                         |           |
               |       --------------   |                         |           |
               |                        |                         |           |
               |                        |          per CPU        |           |
               |                        |     Packet Descriptor   |           |
               |                        |      (SM_PKT_CPU_DESC)  -------------
               |                        ------>----------------
               |                               |  status      | CPU 0
               |                               |  inputList   | Packet Desc
               |                               |  freeList    |
               |                               |--------------|
               |                               |      .       | CPU 1
               |                               |      .       |
               |                               |--------------|
               |                               |              |
               |                               ~              ~
               |                               |              | CPU n
               |                               |              |
               |                               ----------------
               |       Shared memory               per CPU
               |       objects header           sm Objects Desc
               |      (SM_OBJ_MEM_HDR)         (SM_OBJ_CPU_DESC)
               ------->--------------   ------>----------------
                       | heartBeat  |   |      |  status      | CPU 0 
                       | initDone   |   |      |  smObjEventQ | Object Desc 
                       | Partitions |   |      |--------------| 
                       |    .       |   |      |      .       | CPU 1 
                       | Name DTB   |   |      |      .       | 
                       | objCpuTbl  |----      |--------------| 
                       |    .       |          |              | 
                       | max values |          |              | 
                       |    .       |          ~              ~ 
                       | statistics |          |              | CPU n
                       --------------          ---------------- 

CONFIGURATION
When the component INCLUDE_SM_OBJ is included, the init and setup
routines in this library are called automatically during VxWorks
initialization.

AVAILABILITY
This module is distributed as a component of the unbundled shared memory
objects support option, VxMP.

INCLUDE FILES: smObjLib.h

SEE ALSO: smObjShow, semSmLib, msgQSmLib, smMemLib, smNameLib, usrSmObjInit(),
\tb VxWorks Programmer's Guide: Shared Memory Objects
*/

/* includes */

#include "vxWorks.h"
#include "errno.h"
#include "intLib.h"
#include "logLib.h"
#include "cacheLib.h"
#include "qFifoGLib.h"
#include "smDllLib.h"
#include "smLib.h"
#include "smUtilLib.h"
#include "semLib.h"
#include "string.h"
#include "stdlib.h"
#include "sysLib.h"
#include "wdLib.h"
#include "private/smNameLibP.h"
#include "private/smMemLibP.h"
#include "private/smObjLibP.h"
#include "private/smFixBlkLibP.h"
#include "private/msgQSmLibP.h"
#include "private/windLibP.h"
#include "private/workQLibP.h"
#include "private/funcBindP.h"
#include "private/eventP.h"

/* defines */

#define PART_OVH      32	/* bytes overhead per partition */

#define SM_OBJ_VERSION 1	/* version number */

/* macro to calculate the size of a shared memory allocated object */

#define SM_OBJ_SIZE(type) (ROUND_UP (sizeof (type), SM_ALIGN_BOUNDARY) \
			   + sizeof (SM_BLOCK_HDR))

/* typedefs */

/* cached CPU Information Structure */

typedef struct sm_obj_cpu_info  /* SM_OBJ_CPU_INFO */
    {
    BOOL             cached;    /* TRUE if info for this CPU is cached */
    SM_CPU_DESC      smCpuDesc;	/* cpu notification info */
    SM_OBJ_EVENT_Q * pCpuEventQ;/* pointer to cpu event queue */
    } SM_OBJ_CPU_INFO;

/* globals */

SM_HDR volatile *         pSmHdr;    /* pointer to shared memory header */
SM_OBJ_MEM_HDR volatile * pSmObjHdr; /* pointer to shared memory obj header */
SM_OBJ_DESC               smObjDesc; /* shared memory object descriptor */

FUNCPTR	smObjTasRoutine;        /* test and set routine */
FUNCPTR smObjTasClearRoutine; 	/* clear routine */
int     smObjSpinTries;         /* maximum # of tries to obtain lock access */
int     smObjProcNum;           /* processor number */
BOOL    smObjTimeoutLog;	/* TRUE : print message if lock take fails */

int smObjInitialized = FALSE;	/* smObj facility not initialized */

SM_FIX_BLK_PART_ID smTcbPartId;	/* shared TCB partition id */

/* locals */

LOCAL SM_DL_LIST       eventList;	/* event list to be processed */
LOCAL SM_OBJ_EVENT_Q * pLocalEventQ;	/* cached local event Q pointer */
LOCAL SM_OBJ_CPU_INFO  smObjCpuInfoTbl [SM_OBJ_MAX_CPU];
					/* smObj cpu info table */ 
LOCAL int              smObjHeartBeatRate; 
					/* heartbeat incrementing rate */
LOCAL WDOG_ID          smObjWdog;	/* watch dog for heartbeat */

/* forward */

LOCAL STATUS  smObjNotifyHandler (void);
LOCAL STATUS  smObjCpuInfoGet (SM_OBJ_DESC * pSmObjDesc, int cpuNum, 
                               SM_OBJ_CPU_INFO * pSmObjCpuInfo);
LOCAL void    smObjBeat (UINT * pHeartBeat); 

extern VOIDFUNCPTR smUtilTasClearRtn;


/******************************************************************************
*
* smObjLibInit - install the shared memory objects facility (VxMP Option)
*
* This routine installs the shared memory objects facility.  It is called
* automatically when the component INCLUDE_SM_OBJ is included.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled shared memory
* objects support option, VxMP.
* 
* RETURNS: OK, or ERROR if the shared memory objects facility has already been
* installed. 
*/

STATUS smObjLibInit (void)

   {
   if (!smObjInitialized)
	{
   	semSmLibInit ();	/* initialize shared semaphore facility */
   	msgQSmLibInit ();	/* initialize shared message queue facility */
   	smMemPartLibInit ();	/* initialize shared memory manager facility */

	smObjInitialized = TRUE;
	return (OK);
	}

   return (ERROR);
   }

/******************************************************************************
*
* smObjSetup - initialize the shared memory objects facility (VxMP Option)
*
* This routine initializes the shared memory objects facility by filling the
* shared memory header.  It must be called only once by the shared memory
* master CPU.  It is called automatically only by the master CPU, when the
* component INCLUDE_SM_OBJ is included.
*
* Any CPU on the system backplane can use the shared memory objects facility;
* however, the facility must first be initialized on the master CPU.  Then
* before other CPUs are attached to the shared memory area by smObjAttach(), 
* each must initialize its own shared memory objects descriptor using
* smObjInit().  This mechanism is similar to the one used by the shared memory
* network driver.
*
* The <smObjParams> parameter is a pointer to a structure containing the
* values used to describe the shared memory objects setup.  This 
* structure is defined as follows in smObjLib.h:
* \cs
* typedef struct sm_obj_params    /@ setup parameters @/
*     {
*     BOOL        allocatedPool;  /@ TRUE if shared memory pool is malloced @/
*     SM_ANCHOR * pAnchor;        /@ shared memory anchor                   @/
*     char *      smObjFreeAdrs;  /@ start address of shared memory pool    @/
*     int         smObjMemSize;   /@ memory size reserved for shared memory @/
*     int         maxCpus;        /@ max number of CPUs in the system       @/
*     int         maxTasks;       /@ max number of tasks using smObj        @/
*     int         maxSems;        /@ max number of shared semaphores        @/
*     int         maxMsgQueues;   /@ max number of shared message queues    @/
*     int         maxMemParts;    /@ max number of shared memory partitions @/
*     int         maxNames;       /@ max number of names of shared objects  @/
*     } SM_OBJ_PARAMS;
* \ce
*
* INTERNAL
* 
*   During initialization, five partitions dedicated to each shared
*   memory object type are created to avoid runtime memory fragmentation.
*
*      - smTcbPartId : shared TCBs partition.
*
*        This partition is a fixed block size partition protected 
*        against concurrent access by a spinlock mechanism and 
*        taskSafe()/taskUnsafe().
*        The size of this partition is defined by the maximum number of
*        tasks using shared memory objects facility as defined in the
*        configuration parameter SM_OBJ_MAX_TASK.
*        We use a fixed block size partition for the shared TCBs because 
*        it is less time consuming than the variable block size memory 
*        manager.  Interrupt lattency is thus reduced.	 
*        
*      - smSemPartId : shared semaphores partition.
*
*        This partition is protected against concurrent access
*        by a shared semaphore and taskSafe()/taskUnsafe().
*        The size of this partition is defined by the maximum number of
*        shared semaphores as defined in the configuration parameter
*        SM_OBJ_MAX_SEM.
*
*      - smNamePartId : shared name database elements partition.
*
*        This partition is protected against concurrent access
*        by a shared semaphore and taskSafe()/taskUnsafe().
*        The size of this partition is defined by the maximum number of
*        names to be entered in the database as defined in the configuration
*        parameter SM_OBJ_MAX_NAME.
*
*      - smMsgQPartId : shared message queues partition.
*
*        This partition is protected against concurrent access
*        by a shared semaphore and taskSafe()/taskUnsafe().
*        The size of this partition is defined by the maximum number of
*        shared message queues as defined in the configuration parameter
*        SM_OBJ_MAX_MSG_Q.
*
*      - smPartPartId : user created shared partitions partition.
*
*        This partition is protected against concurrent access
*        by a shared semaphore and taskSafe()/taskUnsafe().
*        The size of this partition is defined by the maximum number of
*        user created shared partitions as defined in the configuration
*        parameter SM_OBJ_MAX_PART.
*
*      The remaining memory is dedicated to the default system
*      shared partition : smSystemPart
*
*        This partition is used by smMemMalloc(), smMemFree() ... system calls.
*        This partition is protected against concurrent access
*        by a shared semaphore and taskSafe()/taskUnsafe().
*        The size of this partition is the remaining part of the declared
*        shared memory pool.
*
*   All partition headers are stored in the shared memory object
*   descriptor reachable by all CPUs via the shared memory anchor.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled shared memory
* objects support option, VxMP.
* 
* RETURNS: OK, or ERROR if the shared memory pool cannot hold all the requested
* objects or the number of CPUs exceeds the maximum.
* 
* ERRNO:
*  S_smObjLib_TOO_MANY_CPU  
*  S_smObjLib_SHARED_MEM_TOO_SMALL
*
* SEE ALSO: smObjInit(), smObjAttach() 
*/

STATUS smObjSetup 
    (
    SM_OBJ_PARAMS *	smObjParams	/* setup parameters */
    )
    {
    int		reqMemSize;	/* required memory pool size */ 
    int	        partSize;	/* size of each partition */
    char *      partAdrs;	/* free address for each partition */
    int		bytesUsed;	/* bytes already used by smSetup */
    int		internalPartsSize; /* sum of bytes used by all partitions */
    char *      smObjFreeAdrs;  /* start address of shared memory pool */
    int         smObjMemSize;   /* memory size reserved for shared memory */
    SM_ANCHOR volatile * pAnchor;        /* shared memory anchor */
    int         tmp;            /* temp storage */

    /* check that only master CPU is calling us */

    if (sysProcNumGet () != SM_MASTER)
        {
	return (ERROR);
        }

    /* 
     * Check that we don't want to use more than absolute maximum 
     * number of CPUs.
     */

    if (smObjParams->maxCpus > SM_OBJ_MAX_CPU)
	{
	errno = S_smObjLib_TOO_MANY_CPU;
	return (ERROR);
	}

    smObjFreeAdrs = smObjParams->smObjFreeAdrs;
    smObjMemSize  = smObjParams->smObjMemSize;
    pAnchor       = (SM_ANCHOR volatile *)smObjParams->pAnchor;

    /* 
     * Common shared memory setup.
     * If smSetup has already been called by the bp driver bytesUsed
     * will be 0 indicating that all the shared memory can be used
     * by shared memory objects.
     */

    if (smSetup ((SM_ANCHOR *)pAnchor, smObjFreeAdrs, SM_TAS_HARD,
		 smObjParams->maxCpus, &bytesUsed) == ERROR)
        {
	return (ERROR);
        }

    /*
     * Move address pool after already used memory and set the
     * new size of the pool.  Already used memory can be the anchor 
     * when an external memory is used and/or memory dedicated
     * to the packet based backplane driver.
     */

    smObjMemSize  -= bytesUsed;
    smObjFreeAdrs += bytesUsed;

    /* check that shared memory pool is big enough */

    reqMemSize = sizeof (SM_OBJ_MEM_HDR) +
     (sizeof (SM_OBJ_CPU_DESC) * SM_OBJ_MAX_CPU) +
     (smObjParams->maxTasks * SM_OBJ_SIZE (SM_OBJ_TCB)) +
     (smObjParams->maxSems * SM_OBJ_SIZE (SM_SEMAPHORE)) +
     (smObjParams->maxNames * SM_OBJ_SIZE (SM_OBJ_NAME)) +
     (smObjParams->maxMsgQueues * SM_OBJ_SIZE (SM_MSG_Q)) +
     (smObjParams->maxMemParts * SM_OBJ_SIZE (SM_PARTITION)) +
     4 * PART_OVH + 5 * SM_ALIGN_BOUNDARY;

    if (smObjMemSize < reqMemSize)
	{
	errno = S_smObjLib_SHARED_MEM_TOO_SMALL;
	return (ERROR);
	}

    /* clear shared memory */

    bzero (smObjFreeAdrs, smObjMemSize);

    /* store shared memory objects header at the top of shared memory */

    pSmObjHdr = (SM_OBJ_MEM_HDR volatile *) ((int) smObjFreeAdrs);

    /* set global values needed while initializing */

    localToGlobalOffset = (int) pAnchor;
    smObjPoolMinusOne   = localToGlobalOffset - 1;

    /* set test and set routine pointer */

    smObjTasRoutine      =  (FUNCPTR) sysBusTas;		
    smObjTasClearRoutine =  (FUNCPTR) smUtilTasClearRtn;		
 
    /* set shared memory objects header in the anchor */

    pAnchor->smObjHeader = htonl (LOC_TO_GLOB_ADRS (pSmObjHdr));

    /* initialize shared TCB partition id */

    smTcbPartId = (SM_FIX_BLK_PART_ID) 
		  (LOC_TO_GLOB_ADRS ((&pSmObjHdr->smTcbPart)));
     
    /* free memory is now behind shared memory objects header */

    smObjFreeAdrs += sizeof (SM_OBJ_MEM_HDR);
    smObjMemSize  -= sizeof (SM_OBJ_MEM_HDR);

    /* store the objects CPU table behind the shared mem obj header */

    pSmObjHdr->objCpuTbl = htonl (LOC_TO_GLOB_ADRS ((int) pSmObjHdr + 
			    sizeof (SM_OBJ_MEM_HDR)));

    /* free memory is now behind the smObj CPU table */

    smObjFreeAdrs += sizeof (SM_OBJ_CPU_DESC) * SM_OBJ_MAX_CPU;
    smObjMemSize  -= sizeof (SM_OBJ_CPU_DESC) * SM_OBJ_MAX_CPU;

    internalPartsSize = 0;

    /* initialize shared TCBs partition */

    partAdrs = smObjFreeAdrs;
    partSize = SM_ALIGN_BOUNDARY + smObjParams->maxTasks * sizeof (SM_OBJ_TCB);
    internalPartsSize += partSize;
    smFixBlkPartInit (smTcbPartId, (char *) LOC_TO_GLOB_ADRS (partAdrs), 
		      partSize, sizeof (SM_OBJ_TCB));

    /* initialize shared semaphores partition */

    /* 
     * align partition address otherwise partition size can 
     * be reduced in smMemPartInit() 
     */

    partAdrs = (char *) ROUND_UP ((partAdrs + partSize), SM_ALIGN_BOUNDARY);
    partSize = PART_OVH + (smObjParams->maxSems * SM_OBJ_SIZE (SM_SEMAPHORE));
    internalPartsSize += partSize;
    smMemPartInit ((SM_PART_ID volatile) smSemPartId,
                   (char *) LOC_TO_GLOB_ADRS(partAdrs), partSize);

    /* initialize shared name database partition */

    partAdrs = (char *) ROUND_UP ((partAdrs + partSize), SM_ALIGN_BOUNDARY);
    partSize = PART_OVH + (smObjParams->maxNames * SM_OBJ_SIZE (SM_OBJ_NAME));
    internalPartsSize += partSize;
    smMemPartInit ((SM_PART_ID volatile) smNamePartId,
                   (char *) LOC_TO_GLOB_ADRS(partAdrs), partSize);

    /* initialize shared message queues partition */

    partAdrs = (char *) ROUND_UP ((partAdrs + partSize), SM_ALIGN_BOUNDARY);
    partSize = PART_OVH + (smObjParams->maxMsgQueues * SM_OBJ_SIZE (SM_MSG_Q)); 
    internalPartsSize += partSize;
    smMemPartInit ((SM_PART_ID volatile) smMsgQPartId,
                   (char *) LOC_TO_GLOB_ADRS(partAdrs), partSize);

    /* initialize shared partitions headers partition */

    partAdrs = (char *) ROUND_UP ((partAdrs + partSize), SM_ALIGN_BOUNDARY);
    partSize = PART_OVH +(smObjParams->maxMemParts * SM_OBJ_SIZE(SM_PARTITION));
    internalPartsSize += partSize;
    smMemPartInit ((SM_PART_ID volatile) smPartPartId,
                   (char *) LOC_TO_GLOB_ADRS(partAdrs), partSize);

    /* initialize system shared partition with remaining memory */

    partAdrs = (char *) (partAdrs + partSize);
    partSize = smObjMemSize - internalPartsSize;
    smMemPartInit ((SM_PART_ID volatile) smSystemPartId,
                   (char *) LOC_TO_GLOB_ADRS(partAdrs), partSize);

    /* put version number */

    pSmObjHdr->version = htonl (SM_OBJ_VERSION);

    /* fills information fields */

    pSmObjHdr->maxSems      = htonl (smObjParams->maxSems);
    pSmObjHdr->maxMsgQueues = htonl (smObjParams->maxMsgQueues);
    pSmObjHdr->maxTasks     = htonl (smObjParams->maxTasks);
    pSmObjHdr->maxMemParts  = htonl (smObjParams->maxMemParts);
    pSmObjHdr->maxNames     = htonl (smObjParams->maxNames);

    /* start smObj heartbeat */
    
    smObjHeartBeatRate	= sysClkRateGet ();
    smObjWdog		= wdCreate ();

    smObjBeat ((UINT *) &pSmObjHdr->heartBeat);

    pSmObjHdr->initDone = htonl (TRUE);	/* indicate initialization done */

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    tmp = pSmObjHdr->initDone;                  /* BRIDGE FLUSH  [SPR 68334] */

    smNameInit (smObjParams->maxNames);	/* initialize shared name database */

    return (OK);
    }

/******************************************************************************
*
* smObjInit - initialize a shared memory objects descriptor (VxMP Option)
*
* This routine initializes a shared memory descriptor.  The descriptor must
* already be allocated in the CPU's local memory.  Once the descriptor has
* been initialized by this routine, the CPU may attach itself to the shared  
* memory area by calling smObjAttach().
*
* This routine is called automatically when the component INCLUDE_SM_OBJ is
* included.
* 
* Only the shared memory descriptor itself is modified by this routine.
* No structures in shared memory are affected.
*
* Parameters:
* \is
* \i `<pSmObjDesc>'
* The address of the shared memory descriptor to be initialized; this
* structure must be allocated before smObjInit() is called.
* \i `<anchorLocalAdrs>'
* The memory address by which the local CPU may access the shared memory
* anchor.  This address may vary among CPUs in the system because of address
* offsets (particularly if the anchor is located in one CPU's dual-ported
* memory).
* \i `<ticksPerBeat>'
* Specifies the frequency of the shared memory anchor's heartbeat.  The
* frequency is expressed in terms of how many CPU ticks on the local CPU
* correspond to one heartbeat period.
* \i `<smObjMaxTries>'
* Specifies the maximum number of tries to obtain access to an internal
* mutually exclusive data structure.
* \i `<intType>, <intArg1>, <intArg2>, <intArg3>'
* Allow a CPU to announce the method by which it is to be notified of shared
* memory events.  See the manual entry for if_sm for a discussion about
* interrupt types and their associated parameters.
* \ie
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled shared memory
* objects support option, VxMP.
* 
* RETURNS: N/A
*
* SEE ALSO: smObjSetup(), smObjAttach()
*/

void smObjInit
    (
    SM_OBJ_DESC * pSmObjDesc,	   /* ptr to shared memory descriptor */
    SM_ANCHOR *	  anchorLocalAdrs, /* shared memory anchor local adrs */
    int		  ticksPerBeat,	   /* cpu ticks per heartbeat */
    int		  smObjMaxTries,   /* max no. of tries to obtain spinLock */
    int		  intType,	   /* interrupt method */
    int		  intArg1,	   /* interrupt argument #1 */
    int		  intArg2,	   /* interrupt argument #2 */
    int		  intArg3	   /* interrupt argument #3 */
    )
    {
    if (pSmObjDesc == NULL)
        {
        return;                 /* don't use null ptr */
        }

    /* generic shared memory descriptor initialization */

    smInit (&pSmObjDesc->smDesc, anchorLocalAdrs, ticksPerBeat, intType, 
            intArg1, intArg2, intArg3); 
  
    /* set global processor number */

    smObjProcNum = pSmObjDesc->smDesc.cpuNum;		

    /* set test and set routine pointer */

    smObjTasRoutine      =  (FUNCPTR) sysBusTas;	
    smObjTasClearRoutine =  (FUNCPTR) smUtilTasClearRtn;		
 
    /* allow message to be printed if lock take fails */

    smObjTimeoutLogEnable (TRUE);

    /* set system wide maximum number of tries to obtain lock */

    smObjSpinTries = smObjMaxTries;

    /* install show routines */
    /* XXXmas these should be turned into one or more components in a folder */

    smObjShowInit ();	
    smNameShowInit ();
    smMemShowInit ();
    semSmShowInit ();
    msgQSmShowInit ();	

    /* generic show routine for shared memory objects called by show() */

    _func_smObjObjShow = (FUNCPTR) smObjObjShow;
    }

/******************************************************************************
*
* smObjAttach - attach the calling CPU to the shared memory objects facility (VxMP Option)
*
* This routine "attaches" the calling CPU to the shared memory objects
* facility.  The shared memory area is identified by the shared memory
* descriptor with an address specified by <pSmObjDesc>.  The descriptor must
* already have been initialized by calling smObjInit().
*
* This routine is called automatically when the component INCLUDE_SM_OBJ is
* included.
*
* This routine will complete the attach process only if and when the shared
* memory has been initialized by the master CPU.  If the shared memory is
* not recognized as active within the timeout period (10 minutes), this
* routine returns ERROR.
*
* The smObjAttach() routine connects the shared memory objects handler to the
* shared memory interrupt.  Note that this interrupt may be shared between
* the shared memory network driver and the shared memory objects facility
* when both are used at the same time.
*
* WARNING:
* Once a CPU has attached itself to the shared memory objects facility, 
* it cannot be detached.  Since the shared memory network driver and 
* the shared memory objects facility use the same low-level attaching
* mechanism, a CPU cannot be detached from a shared memory network driver
* if the CPU also uses shared memory objects.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled shared memory
* objects support option, VxMP.
* 
* RETURNS:
* OK, or ERROR if the shared memory objects facility is not active
* or the number of CPUs exceeds the maximum.
*
* ERRNO:
*  S_smLib_INVALID_CPU_NUMBER
*
* SEE ALSO: smObjSetup(), smObjInit()
*/

STATUS smObjAttach
    (
    SM_OBJ_DESC * pSmObjDesc	/* pointer to shared memory descriptor */
    )
    {
    SM_OBJ_CPU_INFO * pSmObjCpuInfo; /* cpu info structure to fill */
    SM_OBJ_CPU_DESC * pObjDesc;      /* pointer to smObj CPU descriptor */
    SM_ANCHOR *       pAnchor;       /* pointer to shared memory anchor */
    int               cpuNum;        /* this CPU number */
    int               beatsToWait;   /* heart beat period */
    int               tmp;           /* temp storage */

    cpuNum  = pSmObjDesc->smDesc.cpuNum;
    pAnchor = pSmObjDesc->smDesc.anchorLocalAdrs;

    /* Check that shared memory is initialized and running */

    beatsToWait = (cpuNum == SM_MASTER) ?
                  DEFAULT_BEATS_TO_WAIT : DEFAULT_ALIVE_TIMEOUT;

    if (smIsAlive (pAnchor, &(pAnchor->smObjHeader), pSmObjDesc->smDesc.base,
		   beatsToWait, pSmObjDesc->smDesc.ticksPerBeat) == FALSE)
        {
        return (ERROR);                         /* sh memory not active */
        }

    /* generic shared memory attach */

    if (smAttach (&pSmObjDesc->smDesc) == ERROR)
        {
        return (ERROR);
        }
 
    /* set offset to access shared objects */

    localToGlobalOffset = (int) pSmObjDesc->smDesc.anchorLocalAdrs;
			  
    smObjPoolMinusOne = localToGlobalOffset - 1;

    /* get local address of shared memory header */

    pSmObjHdr = (SM_OBJ_MEM_HDR volatile *) GLOB_TO_LOC_ADRS(\
                                                  ntohl(pAnchor->smObjHeader));

    pSmObjDesc->hdrLocalAdrs = (SM_OBJ_MEM_HDR *) pSmObjHdr;
    pSmObjDesc->cpuLocalAdrs = (SM_OBJ_CPU_DESC *) 
                               GLOB_TO_LOC_ADRS (ntohl (pSmObjHdr->objCpuTbl));

    /* initialize shared TCB partition id and free routines pointer */
     
    smTcbPartId = (SM_FIX_BLK_PART_ID)
		   (LOC_TO_GLOB_ADRS((&pSmObjHdr->smTcbPart)));

    smObjTcbFreeRtn        = (FUNCPTR) smObjTcbFree;
    smObjTcbFreeFailRtn    = (FUNCPTR) smObjTcbFreeLogMsg;

    /* set global name database pointer */

    pSmNameDb = &pSmObjHdr->nameDtb;   /* get name database header */

    /* initialize windDelete failure notification routine pointer */

    smObjTaskDeleteFailRtn = (FUNCPTR) smObjTimeoutLogMsg;

    /* get local event Q pointer */

    pSmObjCpuInfo = (SM_OBJ_CPU_INFO *) malloc (sizeof (SM_OBJ_CPU_INFO));
    bzero ((char *) pSmObjCpuInfo, sizeof (SM_OBJ_CPU_INFO));

    smObjCpuInfoGet (pSmObjDesc, NONE, pSmObjCpuInfo);
    pLocalEventQ = pSmObjCpuInfo->pCpuEventQ;

    /* clear cpu notification info cache table */

    bzero ((char *) smObjCpuInfoTbl, 
           sizeof (SM_OBJ_CPU_INFO) * SM_OBJ_MAX_CPU);

    /* connect the shared memory objects interrupt handler */
   
    if (smUtilIntConnect (HIGH_PRIORITY, (FUNCPTR) smObjNotifyHandler, 0, 
			  pSmObjDesc->smDesc.intType, 
                          pSmObjDesc->smDesc.intArg1, 
                          pSmObjDesc->smDesc.intArg2, 
			  pSmObjDesc->smDesc.intArg3) == ERROR)
        {
	return (ERROR);
        }

    /* calculate addr of cpu desc in global table */

    pObjDesc = &((pSmObjDesc->cpuLocalAdrs) [smObjProcNum]);

    pObjDesc->status = htonl (SM_CPU_ATTACHED); /* mark this cpu as attached */
    pSmObjDesc->status = SM_CPU_ATTACHED;       /* also mark sh mem descr */

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    tmp = pObjDesc->status;                     /* BRIDGE FLUSH  [SPR 68334] */

    return (OK);
    }

/******************************************************************************
*
* smObjDetach - detach CPU from shared memory objects facility
*
* This routine ends the "attachment" between the calling CPU and
* the shared memory objects facility.  
*
* RETURNS: OK, or ERROR.
*
* INTERNAL
* Detaching a CPU that uses shared memory objects is not possible
* as smObj are implemented now.  Indeed, if a CPU has a task pended
* on a shared semaphore, there is a shared TCB attached to the semaphore
*
* NOMANUAL
*/

STATUS smObjDetach
    (
    SM_DESC * pSmObjDesc       /* ptr to shared mem object descriptor */
    )
    {
    if (pSmObjDesc == NULL)
        {
        return (ERROR);
        }

    /* PME for now no detachment from shared memory objects available */

    return (ERROR);
    }


/******************************************************************************
*
* smObjEventSend - send one or more events to remote CPU
*
* This routine notifies one or more shared memory events to a remote CPU.
* Notification informations are stored in the shared memory objects task
* control block added to <pEventList> list.  The last parameter <destCpu>
* contains the processor number to which events are sent.
*
* Events are shared TCB of tasks to be added to the ready queue.
*
* A spin-lock mechanism is used to prevent more than one processor to add 
* events to the same processor event queue at the same time.
*
* In order to reduce bus traffic and interrupt number, the destination CPU 
* is interrupted only if its event queue was previously empty. 
*
* This routine uses smMemIntGenFunc() to interrupt the destination CPU.
*
* RETURNS: OK, or ERROR if cannot obtain access to CPU event queue.
*
* ERRNO:
*   S_smObjLib_LOCK_TIMEOUT
*
* NOMANUAL
*/

STATUS smObjEventSend
    (
    SM_DL_LIST * pEventList,	/* list of events */
    UINT32       destCpu
    )
    {
    SM_OBJ_EVENT_Q *	pDestCpuEventQ;   /* remote CPU event queue */
    BOOL		destEventQWasEmpty;	
    int			level;

    /*
     * Cache eventQ and notification informations if this is
     * the first time events are send to destCpu.
     */

    if (!smObjCpuInfoTbl [destCpu].cached)
	{
	smObjCpuInfoGet (&smObjDesc, destCpu, &smObjCpuInfoTbl [destCpu]);
	smObjCpuInfoTbl[destCpu].cached = TRUE;
        }

    pDestCpuEventQ = smObjCpuInfoTbl [destCpu].pCpuEventQ;

    /* ENTER LOCKED SECTION */

    if (SM_OBJ_LOCK_TAKE (&pDestCpuEventQ->lock, &level) != OK)
        {
	smObjTimeoutLogMsg ("smObjEventSend", (char *) &pDestCpuEventQ->lock);
        return (ERROR);                         /* can't take lock */
        }

    destEventQWasEmpty = SM_DL_EMPTY (&pDestCpuEventQ->eventList);

    /* add shared TCBs to event queue */

    smDllConcat (&pDestCpuEventQ->eventList, pEventList);

    /* EXIT LOCKED SECTION */

    SM_OBJ_LOCK_GIVE (&pDestCpuEventQ->lock, level);

    /* 
     * now interrupt destination CPU only event queue was previously empty
     */

    if (destEventQWasEmpty)
	{
    	if (smUtilIntGen (&(smObjCpuInfoTbl[destCpu].smCpuDesc), destCpu) !=OK)
    	    {
	    return (ERROR);                    /* int gen routine error */
    	    }
	}
	
    return (OK);
    }

/******************************************************************************
*
* smObjEventProcess - shared memory objects event processing
*
* This routine is called at interrupt level to handle the remote CPU part
* of a global semaphore give or flush.  It is called on the CPU that made the
* semTake.
*
* NOTE :
* When the pending node is a remote node, the smObjTcb is removed from
* the waiting list on the semGive or semFlush side and the TCB is put
* on the ready queue on the semTake() side, otherwise since the notification
* time is not null another semGive() or semFlush() could find the smObjTcb() in
* the waiting list before the remote node have removed it.  During
* the notification time the task is not in the shared semaphore
* waiting list and not in the remote CPU ready queue.
*
* NOMANUAL
*/

void smObjEventProcess
    (
    SM_DL_LIST * pEventList      /* list of event to process */
    )
    {
    SM_OBJ_TCB * pSmObjTcb;
    WIND_TCB   * pTcb;
    int 	 level;

    /*
     * We are going to process each event stored in a local copy of 
     * the event list obtain by smObjNotifyHandler.
     */

    while (!(SM_DL_EMPTY (pEventList)))
        {
	/* 
	 * We must lock out interrupts while removing shared TCBs from 
	 * event list, because we can run here as deferred work and another
	 * shared TCB event may be added to the global eventList 
	 * we use to empty the CPU event queue in smObjNotifyHandler.
	 */
        
	level = intLock ();

        pSmObjTcb = (SM_OBJ_TCB *) smDllGet (pEventList);

	intUnlock (level);

	/* get the local TCB */

	pTcb = (WIND_TCB *) ntohl ((int) pSmObjTcb->localTcb);

	/* 
	 * At this time the following can happen, a task delete, a timeout
	 * or a signal can have change the status of the previously
	 * pending task.  In that the incoming shared TCB must be dropped.
	 * The fact that the task was deleted, timeout or signaled is
	 * indicated by a NULL value in the localTcb pointer. 
	 * See qFifoGRemove for more details.
	 */

	if (pTcb != NULL)
	    {
	    /* 
	     * The task is still pending on the shared semaphore
	     * we are giving.  Put it on the ready Queue.
	     * When it comes here the shared TCB has its removedByGive
	     * field set to TRUE.  We have to reset removedByGive
	     * to indicate that this shared TCB can block again
	     * on a shared semaphore otherwise a future call to
	     * qFifoGRemove will get lost.
	     */

	    pSmObjTcb->removedByGive = htonl (FALSE);

            /* don't disturb cache while in ISR, just trigger FIFO flush */

            level = pSmObjTcb->removedByGive;   /* BRIDGE FLUSH  [SPR 68334] */

            /* windview - level 2 event logging */

	    EVT_TASK_1 (EVENT_OBJ_SEMGIVE, NULL);

            windReadyQPut (pTcb);
	    }

	else
	    {
	    /* 
	     * The task was deleted, was signaled or get a timeout,
	     * we free the shared TCB.  The next call to semTake
	     * on a shared semaphore for this task will allocate 
	     * a new shared TCB since the pSmObjTcb field of the local
	     * TCB has been set to NULL in qFifoGRemove.
	     */

	    smObjTcbFree (pSmObjTcb);
	    }
        }
    }


/******************************************************************************
*
* smObjNotifyHandler - handle shared memory events
*
* This routine processes all events currently stored in local CPU event queue. 
* Events are shared TCBs of tasks which were previously blocked
* on a shared semaphore and which were unblock by another CPU doing
* a semGive.
*
* This routine must be connected to the shared memory interrupt, mailbox 
* or polling task. 
*
* The CPU event queue is protected against concurrent access by a spin-lock.
* To reduce interrupt lattency, access to the event queue is only
* prevented while events are removed from it, not when they are processed.
*
* When the shared memory backplane driver is used, this handler is connected to
* the interrupt, mailbox, or polling task used by the backplane driver.
*
* RETURNS: OK, or ERROR if cannot obtain access to CPU event queue.
*
* ERRNO:
*   S_smObjLib_LOCK_TIMEOUT
*
* NOMANUAL
*/

LOCAL STATUS smObjNotifyHandler (void)
    {
    int                level;

    /* 
     * What we have to do here is remove all the shared TCB from
     * the CPU event queue and put them on a local eventList
     * and process them. 
     * Note that there is no need of a while loop to check again for 
     * CPU event queue emptiness, since if a CPU adds events 
     * to it, it will generate an interrupt because we left
     * the CPU event queue empty after smDllConcat.
     */

    /*
     * We must check for event queue emptiness because
     * we are connected to the same interrupt as the backplane driver.
     */

    if (!(SM_DL_EMPTY (&pLocalEventQ->eventList)))
	{
	/* ENTER LOCKED SECTION */

        if (SM_OBJ_LOCK_TAKE (&pLocalEventQ->lock, &level) != OK)
	    {
	    smObjTimeoutLogMsg ("smObjNotifyHandler", 
				(char *) &pLocalEventQ->lock);
       	    return (ERROR);              		/* can't take lock */
       	    }

	/* 
	 * Now, in order to minimize interrupt lattency, we remove all 
	 * the events from event queue at the same time and put them in 
	 * a local event list.
	 */
	
	smDllConcat (&eventList, &pLocalEventQ->eventList);  

	/* EXIT LOCKED SECTION */

	SM_OBJ_LOCK_GIVE (&pLocalEventQ->lock, level);

	/* 
	 * Now process list of events out of the locked section,
	 * if we are already in kernel state, defer the work.
	 */

	/* 
	 * We can come here with an empty list in the case where
	 * shared memory objects and sm backplane driver are used
	 * at the same time and the interrupt was for an sm packet
	 * not for a smObj event.  In that case smObjEventProcess
	 * will essentially be a no-op.
	 */

	if (kernelState)
	    {
	    workQAdd1 ((FUNCPTR) smObjEventProcess, (int) &eventList);
	    }
	else
	    {
	    kernelState = TRUE;				/* ENTER KERNEL */
	    smObjEventProcess (&eventList);		/* unblock tasks */
	    windExit (); 				/* EXIT KERNEL */
	    }

        /* 
         * If we were notified by a bus interrupt we must acknowledge it.
         * This is not done here since it is not possible to acknowledge a bus
         * interrupt more than once on most i960 boards.  Under heavy load
         * on both the backplane driver and shared memory objects the backplane
         * driver will also acknowledge the interrupt and cause the CPU to
         * reboot.  The only solution is to handle the acknowledge in the
         * smUtilIntRoutine() but this is only possible with a rework of both
         * VxMP and the shared memory network code.
         */
	}	

    return (OK);
    }

/******************************************************************************
*
* smObjLocalToGlobal - convert a local address to a global address (VxMP Option)
*
* This routine converts a local shared memory address <localAdrs> to its
* corresponding global value.  This routine does not verify that
* <localAdrs> is really a valid local shared memory address.
*
* All addresses stored in shared memory are global.  Any access made to shared
* memory by the local CPU must be done using local addresses.  This routine
* and smObjGlobalToLocal() are used to convert between these address types.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled shared memory
* objects support option, VxMP.
* 
* RETURNS: The global shared memory address pointed to by <localAdrs>.
*
* SEE ALSO: smObjGlobalToLocal()
*/

void * smObjLocalToGlobal 
    (
    void * localAdrs		/* local address to convert */
    )
    {
    /* test memory bounds PME */

    return ((void *) LOC_TO_GLOB_ADRS (localAdrs));
    }

/******************************************************************************
*
* smObjGlobalToLocal - convert a global address to a local address (VxMP Option)
*
* This routine converts a global shared memory address <globalAdrs> to its
* corresponding local value.  This routine does not verify that <globalAdrs>
* is really a valid global shared memory address.
*
* All addresses stored in shared memory are global.  Any access made to shared
* memory by the local CPU must be done using local addresses.  This routine
* and smObjLocalToGlobal() are used to convert between these address types.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled shared memory
* objects support option, VxMP.
* 
* RETURNS: The local shared memory address pointed to by <globalAdrs>.
*
* SEE ALSO: smObjLocalToGlobal()
*/

void * smObjGlobalToLocal 
    (
    void * globalAdrs		/* global address to convert */
    )
    {
    /*
     * PME test memory bounds not in the range 
     * SM_OBJ_FREE_ADRS - (SM_OBJ_FREE_ADRS + SM_OBJ_FREE_SIZE)
     */

    return ((void *) GLOB_TO_LOC_ADRS (globalAdrs));
    }

/******************************************************************************
*
* smObjTcbInit - initialize shared Task Control Block
*
* This routine allocates a shared task control block from the shared memory 
* TCB dedicated pool (smTcbPartId) and initializes it.  The shared TCB pointer
* is stored in the pSmObjTcb field of the task control block.
*
* This function is called the first time a task block on a shared semaphore
* (ie in semTake).
*
* The shared Task Control Block is de-allocated and the associated
* memory is given back to the shared TCB dedicated pool when the task
* exits or is deleted.
*
* RETURNS: OK, or ERROR if cannot allocate shared TCB.
*          
* ERRNO:
*  S_smMemLib_NOT_INITIALIZED
*  S_smMemLib_NOT_ENOUGH_MEMORY
*  S_smObjLib_LOCK_TIMEOUT
*
* NOMANUAL
*/

STATUS smObjTcbInit (void)
    {
    WIND_TCB * pTcb;

    pTcb = (WIND_TCB *) taskIdCurrent;		/* get current tcb */

    /* allocate shared TCB from shared TCBs partition */

    pTcb->pSmObjTcb = (SM_OBJ_TCB *) smFixBlkPartAlloc (smTcbPartId);

    if (pTcb->pSmObjTcb == NULL)
        {
        return (ERROR);
        }

    /* initialize shared TCB */

    bzero ((char *) pTcb->pSmObjTcb, sizeof (SM_OBJ_TCB));
    pTcb->pSmObjTcb->ownerCpu = htonl (smObjProcNum);   /* our proc number */
						/* local TCB we belong to */
    pTcb->pSmObjTcb->localTcb = (WIND_TCB *) htonl ((int) pTcb); 

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */

    return (OK);
    }

/******************************************************************************
*
* smObjTcbFree - free shared Task Control Block
*
* This routine gives back a shared task control block to the shared memory
* TCB dedicated pool (smTcbPartId).
*
* The shared Task Control Block is de-allocated and the associated
* memory is given back to the shared TCB dedicated pool when the task
* exits or is deleted.
*
* RETURNS: OK, or ERROR if cannot free shared TCB.
*
* ERRNO:
*  S_smMemLib_NOT_INITIALIZED
*  S_smObjLib_LOCK_TIMEOUT
*
* NOMANUAL
*/

STATUS smObjTcbFree 
    (
    SM_OBJ_TCB * pSmObjTcb	/* pointer to shared TCB to be freed */
    )
    {
    return (smFixBlkPartFree (smTcbPartId, (char *) pSmObjTcb));
    }

/******************************************************************************
*
* smObjCpuInfoGet - get information about a single CPU using shared memory
*
* This routine obtains a variety of information describing the CPU specified
* by <cpuNum>.  If <cpuNum> is NONE (-1), this routine returns information
* about the local (calling) CPU.  The information is returned in a special
* structure, SM_CPU_INFO, whose address is specified by <pCpuInfo>.  (The
* structure must have been allocated before this routine is called.)
*
* RETURNS: OK, or ERROR.
*
* NOMANUAL
*/

LOCAL STATUS smObjCpuInfoGet
    (
    SM_OBJ_DESC *     pSmObjDesc,    /* ptr to shared memory descriptor */
    int               cpuNum,        /* number of cpu to get info about */
    SM_OBJ_CPU_INFO * pSmObjCpuInfo  /* cpu info structure to fill */
    )
    {
    SM_CPU_DESC volatile * pCpuDesc; /* ptr to cpu descriptor */
    int                    tmp;      /* temp storage */

    /* Report on local cpu if NONE specified */

    if (cpuNum == NONE)
        {
        cpuNum = pSmObjDesc->smDesc.cpuNum;
        }

    /* Get info from cpu descriptor */

    if (cpuNum < 0  ||  cpuNum >= pSmObjDesc->smDesc.maxCpus)
        {
        errno = S_smLib_INVALID_CPU_NUMBER;
        return (ERROR);                 /* cpu number out of range */
        }

    /* get address of cpu descr */

    pCpuDesc = (SM_CPU_DESC volatile *) &(pSmObjDesc->smDesc.cpuTblLocalAdrs \
                                                                     [cpuNum]);

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    tmp = pCpuDesc->status;                     /* PCI bridge bug [SPR 68844]*/

    pSmObjCpuInfo->smCpuDesc.status  = pCpuDesc->status; 
					/* attached or not attached */
    pSmObjCpuInfo->smCpuDesc.intType = pCpuDesc->intType;
					/* interrupt type */
    pSmObjCpuInfo->smCpuDesc.intArg1 = pCpuDesc->intArg1;
					/* interrupt argument #1 */
    pSmObjCpuInfo->smCpuDesc.intArg2 = pCpuDesc->intArg2;
					/* interrupt argument #2 */
    pSmObjCpuInfo->smCpuDesc.intArg3 = pCpuDesc->intArg3;
					/* interrupt argument #3 */

    /* get event Q pointer */

    pSmObjCpuInfo->pCpuEventQ = &(pSmObjDesc->cpuLocalAdrs[cpuNum].smObjEventQ);

    return (OK);
    }

/******************************************************************************
*
* smObjTimeoutLogMsg - notify spin-lock take timeout
*
* This routine prints a message on the standard output when an attempt
* to get lock access on a shared ressource failed if the global
* boolean smObjTimeoutLog is set to TRUE, otherwise only the error
* status is set to S_smObjLib_LOCK_TIMEOUT.
*
* <routineName> is a string containing the name of the function where
* attempt to take lock has failed.
*
* <lockLocalAdrs> is the local address of the shared memory spin-lock.
*
* NOMANUAL
*/

void smObjTimeoutLogMsg
    (
    char * routineName, 
    char * lockLocalAdrs
    ) 
    {
    if (smObjTimeoutLog)
        {
	logMsg ("ERROR : %s cannot take lock at address %#x.\n", 
		(int) routineName, (int) lockLocalAdrs, 0, 0, 0, 0);
        }

    errno = S_smObjLib_LOCK_TIMEOUT;
    }

/******************************************************************************
*
* smObjTimeoutLogEnable - control logging of failed attempts to take a spin-lock (VxMP Option)
*
* This routine enables or disables the printing of a message when
* an attempt to take a shared memory spin-lock fails.
*
* By default, message logging is enabled.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled shared memory
* objects support option, VxMP.
* 
* RETURNS: N/A
*/

void smObjTimeoutLogEnable
    (
    BOOL timeoutLogEnable	/* TRUE to enable, FALSE to disable */
    ) 
    {
    smObjTimeoutLog = timeoutLogEnable;
    }

/******************************************************************************
*
* smObjBeat - increment shared memory objects hearbeat
*
* This routine continuously restart itself to increment the shared 
* memory objects heartbeat.
*
* RETURNS: N/A
*
* NOMANUAL
*/

LOCAL void smObjBeat 
    (
    UINT * pHeartBeat
    ) 
    {
    int    tmp;           /* temp storage */

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    tmp = *pHeartBeat;                          /* PCI bridge bug [SPR 68844]*/

    *pHeartBeat = htonl (ntohl (*pHeartBeat) + 1); /* increment heartbeat */

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    tmp = *pHeartBeat;                          /* BRIDGE FLUSH  [SPR 68334] */

    /* start a watchdog to call ourself every smObjHeartBeatRate */

    wdStart (smObjWdog, smObjHeartBeatRate, (FUNCPTR) smObjBeat, 
	     (int) pHeartBeat);
    }

/******************************************************************************
*
* smObjTcbFreeLogMsg - notify shared TCB free failure
*
* This routine prints a message on the standard output when 
* a shared TCB have not been given back to the shared TCB partition 
* because taskDestroy was unable to take the shared TCB partition spin-lock.
*
* NOMANUAL
*/

void smObjTcbFreeLogMsg (void)
    {
    static int failNum;

    failNum++;

    if (smObjTimeoutLog)
	{
	logMsg ("WARNING: shared TCB not freed.\n", 0, 0, 0, 0, 0, 0); 
	}
    }

