/* dcacheCbio.c - Disk Cache Driver */

/* Copyright 1999-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01z,03mar02,jkf  SPR#32277, adding dcacheDevEnable and Disable(), orig by chn
01y,21dec01,chn  SPRs 30130, 22463, 21975 (partial). Disabled defaulting
                 tuneable parameters after they are explicitly set by user.
01x,12dec01,jkf  fixing diab build warnings.
01w,09dec01,jkf  SPR#71637, fix for SPR#68387 caused ready changed bugs.
01v,09nov01,jkf  SPR#71633, dont set errno when DevCreate is called w/BLK_DEV
                 SPR#65431, fixing typo in man page
01u,29aug01,jkf  SPR#69031, common code for both AE & 5.x.
01t,01aug01,jyo  Fixed SPR#68387: readyChanged bit is not correctly checked to
                 verify the change in the media, SPR#69411: Change in media's
                 readyChanged bit is not being propogated appropriately to the
                 layers above.
01s,13jun01,jyo  SPR#67729: Updating blkSubDev, cbioSubDev and isDriver in
                 dcacheDevCreate().
01r,19apr00,dat  doc fixup
01q,09mar00,jkf  removed taskUndelay & taskPrioritySet from CBIO_FLUSH
                 now forcing ioctl flushes inline.
01p,29feb00,jkf  T3 changes
01o,31aug99,jkf  changes for new CBIO API.
01n,31jul99,jkf  T2 merge, tidiness & spelling.
01i,17nov98,lrn  zero-fill blocks allocated with CBIO_CACHE_NEWBLK
01h,29oct98,lrn  pass along 3rd arg on CBIO_RESET, removed old mod history
01g,20oct98,lrn  fixed SPR#22553, SPR#22731
01f,14sep98,lrn  refined error handling, made updTask unconditional
01e,08sep98,lrn  add hash for speed (SPR#21972), size change (SPR#21975)
01d,06sep98,lrn  modify to work on top of CBIO (SPR#21974), and use
                 wrapper for block devices b.c.
01c,30jul98,wlf  partial doc cleanup
01b,01jul98,lrn  written.
01a,28jan98,lrn  written, preliminary
*/

/*
DESCRIPTION

This module implements a disk cache mechanism via the CBIO API.
This is intended for use by the VxWorks DOS file system, to store
frequently used disk blocks in memory.  The disk cache is unaware 
of the particular file system format on the disk, and handles the
disk as a collection of blocks of a fixed size, typically the sector
size of 512 bytes.  

The disk cache may be used with SCSI, IDE, ATA, Floppy or any other
type of disk controllers.  The underlying device driver may be either
comply with the CBIO API or with the older block device API.  

This library interfaces to device drivers implementing the block 
device API via the basic CBIO BLK_DEV wrapper provided by cbioLib.

Because the disk cache complies with the CBIO programming interface on
both its upper and lower layers, it is both an optional and a stackable
module.   It can be used or omitted depending on resources available and 
performance required.

The disk cache module implements the CBIO API, which is used by the file
system module to access the disk blocks, or to access bytes within a 
particular disk block.  This allows the file system to use the disk cache
to store file data as well as Directory and File Allocation Table blocks, 
on a Most Recently Used basis, thus keeping a controllable subset of these
disk structures in memory.  This results in minimized memory requirements 
for the file system, while avoiding any significant performance degradation.

The size of the disk cache, and thus the memory consumption of the disk
subsystem, is configured at the time of initialization (see 
dcacheDevCreate()), allowing the user to trade-off memory consumption
versus performance.  Additional performance tuning capabilities are
available through dcacheDevTune().

Briefly, here are the main techniques deployed by the disk cache:
.IP
Least Recently Used block re-use policy
.IP
Read-ahead
.IP
Write-behind with sorting and grouping
.IP
Hidden writes
.IP
Disk cache bypass for large requests
.IP
Background disk updating (flushing changes to disk) with an adjustable
update period (ioctl flushes occur without delay.)
.LP

Some of these techniques are discussed in more detail below; others 
are described in varrious professional and academic publications.

DISK CACHE ALGORITHM
The disk cache is composed internally of a number cache blocks, of
the same size as the disk physical block (sector). These cache blocks
are maintained in a list in "Most Recently Used" order, that is, blocks
which are used are moved to the top of this list. When a block needs to
be relinquished, and made available to contain a new disk block, the
Least Recently Used block will be used for this purpose.

In addition to the regular cache blocks, some of the memory allocated
for cache is set aside for a "big buffer", which may range from 1/4 of
the overall cache size up to 64KB.  This buffer is used for:

.IP
Combining cache blocks with adjacent disk block numbers, in order to
write them to disk in groups, and save on latency and overhead 
.IP
Reading ahead a group of blocks, and then converting them to normal
cache blocks.
.LP

Because there is significant overhead involved in accessing the disk
drive, read-ahead improves performance significantly by reading groups
of blocks at once.

TUNABLE PARAMETERS
There are certain operational parameters that control the disk cache
operation which are tunable. A number of
.I preset
parameter sets is provided, dependent on the size of the cache. These
should suffice for most purposes, but under certain types of workload,
it may be desirable to tune these parameters to better suite the
particular workload patterns.

See dcacheDevTune() for description of the tunable parameters. It is
recommended to call dcacheShow() after calling dcacheTune() in order 
to verify that the parameters where set as requested, and to inspect 
the cache statistics which may change dramatically. Note that the hit 
ratio is a principal indicator of cache efficiency, and should be inspected
during such tuning.

BACKGROUND UPDATING
A dedicated task will be created to take care of updating the disk with
blocks that have been modified in cache. The time period between updates
is controlled with the tunable parameter syncInterval. Its priority
should be set above the priority of any CPU-bound tasks so as to assure
it can wake up frequently enough to keep the disk synchronized with the
cache.   There is only one such task for all cache devices configured.  
The task name is tDcacheUpd

The updating task also has the responsibility to invalidate disk cache
blocks for removable devices which have not been used for 2 seconds or more.

There are a few global variables which control the parameters of this
task, namely:

.IP <dcacheUpdTaskPriority>
controls the default priority of the update task, and is set by default to 250.

.IP <dcacheUpdTaskStack>
is used to set the update task stack size.

.IP <dcacheUpdTaskOptions>
controls the task options for the update task.
.LP
All the above global parameters must be set prior to calling
dcacheDevCreate() for the first time, with the exception of
dcacheUpdTaskPriority, which may be modified in run-time, and takes
effect almost immediately. It should be noted that this priority is not
entirely fixed, at times when critical disk operations are performed,
and FIOFLUSH ioctl is called, the caller task will temporarily
.I loan
its priority to the update task, to insure the completion of the flushing
operation.

REMOVABLE DEVICES
For removable devices, disk cache provides these additional features:

.IP "disk updating"
is performed such that modified blocks will be written to disk within
one second, so as to minimize the risk of losing data in case of a
failure or disk removal.
.IP "error handling"
includes a test for disk removal, so that if a disk is removed from the
drive while an I/O operation is in progress, the disk removal event will
be set immediately.
.IP "disk signature"
which is a checksum of the disk's boot block, is maintained by the cache
control structure, and it will be verified against the disk if it was
idle for 2 seconds or more. Hence if during that idle time a disk was
replaced, the change will be detected on the next disk access, and the
condition will be flagged to the file system.

.IP NOTE
It is very important that removable disks should all have a unique
volume label, or volume serial number, which are stored in the disk's
boot sector during formatting. Changing disks which have an identical
boot sector may result in failure to detect the change, resulting in
unpredictable behavior, possible file system corruption.
.LP


CACHE IMPLEMENTATION

Most Recently Used (MRU) disk blocks are stored in a collection of memory
buffers called the disk cache.  The purpose of the disk cache is to reduce 
the number of disk accesses and to accelerate disk read and write operations, 
by means of the following techniques:

.IP
Most Recently Used blocks are stored in RAM, which results in the most
frequently accessed data being retrieved from memory rather than from disk.
.IP
Reading data from disk is performed in large units, relying on the read-ahead
feature, one of the disk cache£s tunable parameters.
.IP
Write operations are optimized because they occur to memory first. Then
updating the disk happens in an orderly manner, by delayed write, another
tunable parameter.
.LP

Overall, the main performance advantage arises from a dramatic
reduction in the amount of time spent by the disk drive seeking, 
thus maximizing the time available for the disk to read and write
actual data. In other words, you get efficient use of the disk 
drive£s available throughput.  The disk cache offers a number of 
operational parameters that can be tuned by the user to suit a 
particular file system workload pattern, for example, delayed write, 
read ahead, and bypass threshold.

The technique of delaying writes to disk means that if the
system is turned off unexpectedly, updates that have not yet 
been written to the disk are lost.  To minimize the effect of 
a possible crash, the disk cache periodically updates the disk.
Modified blocks of data are not kept in memory more then a specified
period of time.  By specifying a small update period, the possible
worst-case loss of data from a crash is the sum of changes possible
during that specified period. For example, it is assumed that an 
update period of 2 seconds is sufficiently large to effectively 
optimize disk writes, yet small enough to make the potential 
loss of data a reasonably minor concern. It is possible to 
set the update period to 0, in which case, all updates are
flushed to disk immediately.   This is essentially the
equivalent of using the DOS_OPT_AUTOSYNC option in earlier 
dosFsLib implementations.  The disk cache allows you to negotiate 
between disk performance and memory consumption: The more memory 
allocated to the disk cache, the higher the "hit ratio" observed,
which means increasingly better performance of file system operations.
Another tunable parameter is the bypass threshold, which defines
how much data constitutes a request large enough to justify bypassing
the disk cache.  When significantly large read or write requests 
are made by the application, the disk cache is circumvented and 
there is a direct transfer of data between the disk controller and 
the user data buffer. The use of bypassing, in conjunction with support 
for contiguous file allocation and access (via the FIOCONTIG ioctl()
command and the DOS_O_CONTIG open() flag), should provide performance
equivalent to that offered by the raw file system (rawFs).  

PARTITION INTERACTION:

The dcache CBIO layer is intended to operate atop an entire fixed
disk device.  When using the dcache layer with the dpart CBIO partition
layer, it is important to place the dcache layer below the partition layer.

For example:

.CS
+----------+
| dosFsLib |
+----------+
     |
+----------+
|   dpart  |
+----------+
     |
+----------+
| dcache   |
+----------+
     |
+----------+
| blkIoDev |
+----------+
.CE

ENABLE/DISABLE THE DISK CACHE

The function dcacheDevEnable is used to enable the disk cache.
The function dcacheDevDisable is used to disable the disk cache.
When the disk cache is disabled, all IO will bypass the cache
layer. 

SEE ALSO: dosFsLib, cbioLib, dpartCbio

INTERNAL

State Machine
-------------
Each cache block can be at one of the five different states at any time, while
the state transitions may occur only when the mutex is taken.
The three basic states are:

.IP EMPTY
a block does not contain any disk data
.IP CLEAN
a block contains an unmodified copy of a certain disk block
.IP DIRTY
a block contains a disk block which has been modified in memory.

There is also a UNSTABLE state which is used between mutex locks,
which is used to indicate that a block is being modified in memory
and its data is not valid. This state is never used after mutex is 
released.

Removable Device Support Details

It is worth noting that we dont trust the block driver's ability
to set its readyChanged flag correctly. Some drivers set it without
need, others fail to set it when indeed a disk is replaced.
Hence we devised an independent approach to this issue - we are
assuming that while the device is active and a disc is replaced,
we will get an error, and we also assume it takes at least 2 seconds
to replace a disk. Hence, if the disk has been idle for more then 2 seconds,
we check the checksum of its boot block, against a previously registered
signature.

Issues to revisit or implement:
 + boot block number is hardcoded.
 + separate removable detection into a separate CBIO module below dcache
*/

/* includes */
#include "vxWorks.h"
#include "stdlib.h"
#include "stdio.h"
#include "semLib.h"
#include "ioLib.h"
#include "taskLib.h"
#include "tickLib.h"
#include "string.h"
#include "errno.h"
#include "dllLib.h"
#include "wdLib.h"
#include "logLib.h"
#include "sysLib.h"

/* START - CBIO private header */
#define CBIO_DEV_EXTRA  struct dcacheCtrl
#include "private/cbioLibP.h"
/* END - CBIO private header */

#include "dcacheCbio.h"

#ifndef DCACHE_MAX_DEVS
#define DCACHE_MAX_DEVS         16
#endif

/* Cache block states */
typedef enum {
    CB_STATE_EMPTY,             /* no valid block is assigned */
    CB_STATE_CLEAN,             /* contains a valid block, unmodified */
    CB_STATE_DIRTY,             /* contains a valid but modified in memory */
    CB_STATE_UNSTABLE           /* block data is undefined or being modified */
    } CB_STATE ;

/* Cache descriptor block */
typedef struct dcacheDesc {
    DL_NODE     lruList;        /* element in LRU list */
    block_t     block;          /* block number contained */
    caddr_t     data;           /* actual data pointer */
    struct dcacheDesc *
                pHashNext;      /* offset of next desc in hash slot */
    CB_STATE    state:4;        /* current state */
    unsigned    busy:1;         /* descriptor busy (unused) */
} DCACHE_DESC ;

struct dcacheCtrl {
        CBIO_DEV_ID     cbioDev ;       /* main device handle */
        CBIO_DEV_ID     dcSubDev ;      /* subordinate CBIO device */
        DL_LIST         dcLru ;         /* LRU list head */
        char *          pDcDesc ;       /* description */
        u_long          dcNumCacheBlocks ;
        caddr_t         dcBigBufPtr;
        u_long          dcBigBufSize;
        block_t         dcLastAccBlock ;
        u_long          dcUpdTick ;
        u_long          dcActTick ;
        u_long          dcDiskSignature;
        /* Hash table params */
        DCACHE_DESC **  ppDcHashBase;
        u_long          dcHashSize;
        u_long          dcHashHits;
        u_long          dcHashMisses;
        /* Tunable Parameters */
        u_long          dcBypassCount;
        u_long          dcOldBypassCount;
        u_long          dcDirtyMax;
        u_long          dcReadAhead;
        u_long          dcSyncInterval;
        u_long          dcCylinderSize;
        BOOL            dcIsTuned;      /* Set when cache is tuned */
        /* Statistic Counters */
        u_long          dcDirtyCount;
        u_long          dcCookieHits ;
        u_long          dcCookieMisses ;
        u_long          dcLruHits ;
        u_long          dcLruMisses ;
        u_long          dcWritesForeground ;
        u_long          dcWritesBackground ;
        u_long          dcWritesHidden ;
        u_long          dcWritesForced ;
} DCACHE_CTRL ;

LOCAL const struct tunablePresets {
        u_long          dcNumCacheBlocks ;
        u_long          dcBypassCount;
        u_long          dcDirtyMax;
        u_long          dcReadAhead;
        u_long          dcHashSize;
        u_long          dcSyncInterval;
} dcacheTunablePresets[] = {
/* <= nblks     bypass  dirty   read-ahead      hash-sz sync */
{   16,         4,      7,      1,              0,      0       },
{   32,         8,      15,     4,              37,     0       },
{   64,         8,      25,     7,              67,     0       },
{   128,        16,     50,     10,             131,    1       },
{   256,        24,     100,    22,             269,    1       },
{   512,        32,     200,    28,             547,    2       },
{  1024,        64,     500,    32,             1033,   5       },
{  NONE,        128,    1000,   64,             2221,   15      },
};

#undef DEBUG

#ifdef DEBUG
#warning dcacheCbio.c: "DEBUG" macro has been defined
#endif

#ifdef  DEBUG
int dcacheCbStateSize = sizeof(CB_STATE);
int dcacheDescSize = sizeof( DCACHE_DESC);
int dcacheDebug = 1;
#endif


/* forward declarations */
LOCAL STATUS dacacheDevInit ( CBIO_DEV_ID dev );
void dcacheUpd ( void ) ;
LOCAL void dcacheQuickFlush (void);

LOCAL DCACHE_DESC * dcacheBlockLocate( CBIO_DEV_ID dev, block_t block );
LOCAL DCACHE_DESC * dcacheBlockGet( CBIO_DEV_ID dev, block_t block,
        cookie_t * pCookie, BOOL readData );

LOCAL STATUS dcacheBlkRW 
    ( 
    CBIO_DEV_ID         dev,
    block_t             startBlock,
    block_t             numBlocks,
    addr_t              buffer,
    CBIO_RW             rw,
    cookie_t *          pCookie);

LOCAL STATUS dcacheBytesRW 
    ( 
    CBIO_DEV_ID         dev,
    block_t             startBlock,
    off_t               offset,
    addr_t              buffer,
    size_t              nBytes,
    CBIO_RW             rw,
    cookie_t *          pCookie);
    
LOCAL STATUS dcacheBlkCopy 
    ( 
    CBIO_DEV_ID         dev,
    block_t             srcBlock,
    block_t             dstBlock,
    block_t             numBlocks);

LOCAL STATUS dcacheIoctl 
    ( 
    CBIO_DEV_ID         dev,
    UINT32              command,
    addr_t              arg);


struct dcacheCtrl dcacheCtrl[ DCACHE_MAX_DEVS ] ;

int dcacheUpdTaskId = 0;           /* updater task, one for all devices */
int dcacheUpdTaskPriority = 250;   /* tunable by user */
int dcacheUpdTaskStack = 5000 ;    /* tuneable by user to save space */

/* CBIO_FUNCS, one per cbio driver */

LOCAL CBIO_FUNCS cbioFuncs = {(FUNCPTR) dcacheBlkRW,
                              (FUNCPTR) dcacheBytesRW,
                              (FUNCPTR) dcacheBlkCopy,
                              (FUNCPTR) dcacheIoctl};

#define DCACHE_UPD_TASK_GRANULARITY     2       /* 1/4 sec granularity */
#define DCACHE_BOOT_BLOCK_NUM           0       /* sec # where signature is */
#define DCACHE_IDLE_SECS                2       /* after 2 secs idle checksum */

#ifdef  DEBUG
#undef  NDEBUG
#define DEBUG_MSG(fmt,a1,a2,a3,a4,a5,a6)        \
        { if(dcacheDebug) logMsg(fmt,a1,a2,a3,a4,a5,a6); }
int dcacheUpdTaskOptions = VX_SUPERVISOR_MODE | VX_FP_TASK ;
#else
#define NDEBUG
#define DEBUG_MSG(fmt,a1,a2,a3,a4,a5,a6)        {}
int dcacheUpdTaskOptions = VX_FP_TASK | VX_SUPERVISOR_MODE | VX_UNBREAKABLE ;
#endif  /* DEBUG */

#define strdup(s)       (strcpy(KHEAP_ALLOC((strlen(s)+1)),(s)))

#define INFO_MSG(fmt,a1,a2,a3,a4,a5,a6) \
        { logMsg(fmt,a1,a2,a3,a4,a5,a6); }

#include "assert.h"

/*******************************************************************************
*
* dcacheDevCreate - Create a disk cache
*
* This routine creates a CBIO layer disk data cache instance. 
* The disk cache unit accesses the disk through the subordinate CBIO
* device driver, provided with the <subDev> argument.
*
* A valid block device BLK_DEV handle may be provided instead of a CBIO
* handle, in which case it will be automatically converted into
* a CBIO device by using the wrapper functionality from cbioLib.
*
* Memory which will be used for caching disk data may be provided by the
* caller with <pRamAddr>, or it will be allocated by dcacheDevCreate() from
* the common system memory pool, if <memAddr> is passed as NULL. <memSize> is
* the amount of memory to use for disk caching, if 0 is passed, then a
* certain default value will be calculated, based on available memory.
* <pDesc> is a string describing the device, used later by
* dcacheShow(), and is useful when there are many cached disk devices.
*
* A maximum of 16 disk cache devices are supported at this time.
* 
* RETURNS: disk cache device handle, or NULL if there is not enough
* memory to satisfy the request, or the <blkDev> handle is invalid.
*
* INTERNAL
* There is not an appropriate Destroy function at this time.
*/
CBIO_DEV_ID dcacheDevCreate
    (
    CBIO_DEV_ID  subDev,  /* block device handle */
    char  *   pRamAddr,   /* where it is in memory (NULL = KHEAP_ALLOC)  */
    int       memSize,    /* amount of memory to use */
    char *    pDesc       /* device description string */
    )
    {
    CBIO_DEV_ID pDev = NULL;
    CBIO_DEV_ID pSubDev = NULL;
    int i ;

    cbioLibInit();      /* just in case */

    if (ERROR == cbioDevVerify( subDev ))
        {
        /* attempt to handle BLK_DEV subDev */
        pSubDev = cbioWrapBlkDev ((BLK_DEV *) subDev);

        if( NULL != pSubDev )
            {
            /* SPR#71633, clear the errno set in cbioDevVerify() */
            errno = 0;
            }
        }
   else
       {
       pSubDev = subDev;
       }

    if (pSubDev == NULL)
        {
        return NULL;    /* failed */
        }

    /* Fill in defaults for args with 0 value */
    if( memSize == 0)
        {
        memSize = min( memFindMax()/16, 188*1024 ) ;
        }


    pDev = (CBIO_DEV_ID) cbioDevCreate ( pRamAddr, memSize );

    if( pDev == NULL )
        {
        return( NULL ); /* failed */
        }

    /* SPR#67729: Fill in blkSubDev, cbioSubDev and isDriver appropriately */

    pDev->blkSubDev = NULL;    /* Since lower layer is not BLOCK DEVICE  */
    pDev->cbioSubDev = pSubDev;   /* Reference to lower CBIO device */
    pDev->isDriver = FALSE;   /* = FALSE since this is a CBIO to CBIO layer*/

    pDev->pDc                   = NULL ;

    /* cbioFuncs is staticly allocated in each driver. */
    pDev->pFuncs = &cbioFuncs;

    /* mutex is not created yet */
    taskLock();

    /* allocate extension control structure */
    for(i = (int) 0; i < (int) NELEMENTS(dcacheCtrl); i++)
        {

        /* Initially cache is not tuned */
        dcacheCtrl[i].dcIsTuned = FALSE;

        /* Initially there is no stored old bypass count */
        dcacheCtrl[i].dcOldBypassCount = 0;

        if( dcacheCtrl[i].cbioDev == NULL )
            {
            pDev->pDc = & dcacheCtrl[i] ;
            break;
            }
        }

    if( pDev->pDc == NULL )
        {
        taskUnlock();           /* FIXME - if too many devs, mem leak */
        errno = ENODEV;
        return NULL;
        }

    bzero( (caddr_t) pDev->pDc, sizeof(struct dcacheCtrl));

    /* backward connect the cbio device */
    pDev->pDc->cbioDev = pDev ;

    taskUnlock();

    /* connect the underlying block driver */
    pDev->pDc->dcSubDev = pSubDev ;

    if( pDesc != NULL )
        {
        pDev->pDc->pDcDesc      = strdup(pDesc) ;
        }
    else
        {
        pDev->pDc->pDcDesc      = "" ;
        }

    dacacheDevInit( pDev );

    /* Create the Updater task if not already created */
    if( 0 == dcacheUpdTaskId )
        {
        dcacheUpdTaskId = taskSpawn(
                "tDcacheUpd",
                dcacheUpdTaskPriority,
                dcacheUpdTaskOptions,
                dcacheUpdTaskStack,
                (FUNCPTR) dcacheUpd,
                0,0,0,0,0,0,0,0,0,0) ;
        }

    /* return device handle */
    return( pDev );
    }

/*******************************************************************************
* 
* dcacheErrorHandler - handle I/O errors and device changes
*
* INTERNAL
*
* Error handling here are performed with the assumption that
* any possible recovery was already performed by the driver, i.e. retries,
* corrections, maybe even reassignments.
* This means there is not much we can do at this level.
* Still, most errors are probably due to device change, which
* is not limited to strictly removable devices, as hard drives
* that appear Fixed are in fact hot swappable in some environments.
*
* Other issues such as mapping a sectors as BAD in the FAT, or Bad
* Block Reassignment (SCSI) will be left to the Utilities to handle,
* e.g. Surface Scan is the way to do such mappings.
*
* RETURNS: the return value will mean if the disk cache should
* continue operation, or abort the current operation and return
* an ERROR to the file system.
*
*/
LOCAL STATUS dcacheErrorHandler
    (
    CBIO_DEV_ID dev,
    DCACHE_DESC * pDesc, 
    CBIO_RW     rw
    )
    {
    STATUS stat = ERROR ; /* OK means cache may continue, ERROR - abort */
    static char errorMsgTemplate[] =
        "disk cache error: device %x block %d errno %x, %s\n" ;

    /* record this errno */
    dev->cbioParams.lastErrno = errno ;

    /* record the block # where error occurred */
    if( pDesc != NULL )
        dev->cbioParams.lastErrBlk = pDesc->block ;
    else
        dev->cbioParams.lastErrBlk = NONE ;

    DEBUG_MSG( "disk cache error: errno=%x, block=%x, removable=%s, "
            "readyChanged=%s\n",
            dev->cbioParams.lastErrno,
            dev->cbioParams.lastErrBlk,
            (int)((dev->pDc->dcSubDev->cbioParams.cbioRemovable)?"Yes":"No"),
            (int)(cbioRdyChgdGet(dev->pDc->dcSubDev)?"Yes":"No"), 0, 0
            );

    /* now shall we see if the error was due to device removal */

    /* first, let us assume the driver has set the flag */
    if( TRUE == cbioRdyChgdGet (dev->pDc->dcSubDev))
        {
        stat = ERROR ;
        CBIO_READYCHANGED (dev) = TRUE;
        }
    /* next, we will double-check if the device is ready */
    else 
        {
        stat = dev->pDc->dcSubDev->pFuncs->cbioDevIoctl(
                        dev->pDc->dcSubDev, CBIO_STATUS_CHK, 0);
        if( stat == ERROR )
            CBIO_READYCHANGED(dev) =  TRUE;
        }

    /* so if the reason was device removal, stop right here */
    if( TRUE == cbioRdyChgdGet (dev))
        {
        /* override errno for removed disk */
        errno = S_ioLib_DISK_NOT_PRESENT ;

        /* dont issue warning for read errors on removal, only for writes */
        if( rw == CBIO_WRITE )
            {
            /* write error, issue warning to the console */
            INFO_MSG(errorMsgTemplate,
                    (int) dev,
                    dev->cbioParams.lastErrBlk,
                    dev->cbioParams.lastErrno,
                    (int) "disk removed while writing data, possible data loss",
                    0,0
                    );
            }
        return(ERROR);
        }

    /* at this point, it is a certain block which is causing this error */

    /* override errno if not set by driver */
    if( errno == 0 )
        errno = S_ioLib_DEVICE_ERROR ;

    if( rw == CBIO_READ )
        {
        INFO_MSG(errorMsgTemplate,
                (int) dev,
                dev->cbioParams.lastErrBlk,
                dev->cbioParams.lastErrno,
                (int) "disk read failed",
                0,0
                );
        return(ERROR);
        }
    else
        {
        /*
         * this policy will be questioned for some time to come
         */
        INFO_MSG(errorMsgTemplate,
                (int) dev,
                dev->cbioParams.lastErrBlk,
                dev->cbioParams.lastErrno,
                (int) "disk write failed, data loss may have occurred",
                0,0
                );
        return(ERROR);
        }
    /*NOTREACH*/
    }

/*******************************************************************************
* 
* dcacheBlockCksum - calculate a checksum signature for a cache block
*
* calculate a 32-bit signature for a block, using a permuted-shift checksum
*/
LOCAL u_long dcacheBlockCksum( CBIO_DEV_ID dev, DCACHE_DESC *pDesc )
    {
    FAST u_long cksum = 0 ;
    FAST int i ;
    FAST u_short * p ;

    p = (ushort *) pDesc->data ;
    i = dev->cbioParams.bytesPerBlk >> 1 ;

    for( ; i > 0 ; i -- )
        cksum += *p++ << (i & 0x3) ;

    return( cksum );
    }

/*******************************************************************************
* 
* dcacheHashRemove - remove a block desc from hash table
*
*/
LOCAL void dcacheHashRemove ( CBIO_DEV_ID dev, DCACHE_DESC *pDesc )
    {
    FAST DCACHE_DESC *pTmp = NULL ;
    FAST DCACHE_DESC ** ppHashSlot ;
    FAST struct dcacheCtrl * pDc = dev->pDc ;

    if( pDesc->state != CB_STATE_EMPTY && pDc->dcHashSize != 0 )
        {
        /* empty blocks are not expected to be in hash, or no hash table */

        ppHashSlot = &(pDc->ppDcHashBase [
                        pDesc->block % pDc->dcHashSize ]);

        pTmp = *ppHashSlot ;

        if( pTmp == pDesc )
            {
            *ppHashSlot = pTmp->pHashNext ;
            }
        else 
            {
            while( pTmp != NULL )
                {
                if( pTmp->pHashNext == pDesc )
                    {
                    pTmp->pHashNext = pDesc->pHashNext;
                    break;
                    }
                pTmp = pTmp->pHashNext ;
                }
            }
        }
    /* assert that we did not fail to find in hash unless has is void */
    assert( (pTmp != NULL) == (pDc->dcHashSize > 0) );

    pDesc->busy = 0;
    pDesc->block = NONE ;
    pDesc->state = CB_STATE_EMPTY ;
    pDesc->pHashNext = NULL ;
    }

/*******************************************************************************
* 
* dcacheChangeDetect - detect a possible disk change
*
* This will return ERROR and set readyChanged to TRUE
* if a change in the boot block's checksum is detected.
*/
LOCAL STATUS dcacheChangeDetect ( CBIO_DEV_ID dev, BOOL invalidate )
    {
    DCACHE_DESC * pDesc ;
    block_t bootBlockNum = DCACHE_BOOT_BLOCK_NUM ;
    FAST u_long cksum ;

    DEBUG_MSG("change detect, idle was %d ms\n",
        1000 * (tickGet() - dev->pDc->dcActTick)/sysClkRateGet(),
        0,0,0,0,0);

    dev->pDc->dcActTick = tickGet() ;
    
    pDesc = dcacheBlockGet(dev, bootBlockNum, NULL, /*readData=*/ TRUE);

    if( pDesc == NULL )
        return ERROR ;
    if( pDesc->state != CB_STATE_CLEAN )
        {
        return OK;
        }
    else if ((pDesc->state == CB_STATE_CLEAN) && invalidate )
        {
        /* invalidate the boot block to read it from disk */
        dcacheHashRemove(dev, pDesc);
        }

    cksum = dcacheBlockCksum( dev, pDesc );

    if ( dev->pDc->dcDiskSignature != cksum )
        {
        DEBUG_MSG("disk changed, signature old %x, new %x\n",
                dev->pDc->dcDiskSignature, cksum, 0,0,0,0 );
        CBIO_READYCHANGED (dev)  = TRUE; 
        dev->pDc->dcDiskSignature = cksum ;
        errno = S_ioLib_DISK_NOT_PRESENT ;
        return (ERROR);
        }

    return (OK);
    }

/*******************************************************************************
* 
* dcacheListAddSort - add descriptor to a sorted list
*
*/
LOCAL void dcacheListAddSort( DL_LIST * pList, DCACHE_DESC * pDesc )
    {
    FAST DCACHE_DESC * pTmp = NULL ;

    if( DLL_EMPTY( pList ))
        {
        dllAdd( pList, &pDesc->lruList );
        return;
        }

    /* start at the tail */
    for( pTmp = (DCACHE_DESC *) DLL_LAST(pList) ; pTmp != NULL ;
                pTmp = (DCACHE_DESC *) DLL_PREVIOUS( pTmp ) )
        {
        if( pTmp->block < pDesc->block )
            {
            /* insert before pTmp */
            dllInsert( pList,  &pTmp->lruList,  &pDesc->lruList );
            return;
            }
        }

    /* else it should be first */
    dllInsert( pList, NULL,  &pDesc->lruList );
    return;
    }


/*******************************************************************************
* 
* dcacheFlushBatch - flush a batch of sorted blocks to disk 
*
*/
LOCAL STATUS dcacheFlushBatch
    (
    CBIO_DEV_ID dev,
    DL_LIST * pList,
    BOOL doInvalidate
    )
    {
    FAST DCACHE_DESC * pTmp, * pContig  ;
    FAST struct dcacheCtrl * pDc = dev->pDc ;
    STATUS ret, retstat = OK ;
    u_long burstCount, ix ;
    caddr_t data ;

    /* 
     * If the device is removeable and the time since the last activity 
     * is greater than the idle threshold check to see if the disk changed 
     */
    if ( dev->cbioParams.cbioRemovable 
         && (pDc->dcActTick < ( tickGet() - (sysClkRateGet() * DCACHE_IDLE_SECS) ) )
       )
        {
        /* If the device changed then we can't flush */
        if( dcacheChangeDetect( dev, FALSE ) == ERROR )
            return ERROR;
        }

    for( pTmp = (DCACHE_DESC *) DLL_FIRST(pList) ; pTmp != NULL ;
                pTmp = (DCACHE_DESC *) DLL_FIRST(pList) )
        {
        pContig = pTmp ;

        for(burstCount = 1 ; burstCount <= pDc->dcBigBufSize; burstCount++)
            {
            pContig =  (DCACHE_DESC *) DLL_NEXT( pContig );
            if( pContig == NULL )
                break ;
            if( pContig->block != (pTmp->block + burstCount) )
                break ;
            } /*for burstCount */

        if( burstCount == 1 )
            {
            /* non-contiguous block, write directly from cache block */
            ret = pDc->dcSubDev->pFuncs->cbioDevBlkRW(
                    pDc->dcSubDev,
                    pTmp->block, 1,
                    pTmp->data, CBIO_WRITE, NULL );
            pDc->dcLastAccBlock = pTmp->block + 1 ;
            }
        else
            {
            /* contiguous burst write */

            /* in case we overcounted */
            burstCount = min(burstCount, pDc->dcBigBufSize);

            data = pDc->dcBigBufPtr ;
            pContig = pTmp ;

            for(ix = 0 ; ix < burstCount ; ix++)
                {
                bcopy(pContig->data, data, dev->cbioParams.bytesPerBlk );
                data += dev->cbioParams.bytesPerBlk;
                pContig =  (DCACHE_DESC *) DLL_NEXT( pContig );
                }

            ret = pDc->dcSubDev->pFuncs->cbioDevBlkRW(
                    pDc->dcSubDev,
                    pTmp->block, burstCount,
                    pDc->dcBigBufPtr, CBIO_WRITE, NULL );

            pDc->dcLastAccBlock = pTmp->block + burstCount ;
            } /* else - burst write */

        /* write error processing */
        if(ret == ERROR)
            {
            retstat |= dcacheErrorHandler( dev, pTmp, CBIO_WRITE );

            /* 
             * NOTE - returning here wont get all blocks back on the
             * list, so we continue as usual, returning error at end
             * of function.
             * we start invalidating from this point on, because cache
             * coherency is questionable after a write error
             */
            doInvalidate = TRUE ;
            }

        for(ix = 0; ix< burstCount ; ix++ )
            {
            pDc->dcDirtyCount -- ;
            if( doInvalidate )
                {
                dcacheHashRemove(dev, pTmp);
                }
            else
                {
                pTmp->state = CB_STATE_CLEAN;
                }

            pContig =  (DCACHE_DESC *) DLL_NEXT( pTmp );
            /* Makes this block LRU now */
            dllRemove( pList, &pTmp->lruList );
            dllAdd( & pDc->dcLru,  &pTmp->lruList );
            pTmp = pContig ;
            }
        } /* for */

    pDc->dcActTick = tickGet() ;
    return( retstat );
    }

/*******************************************************************************
* 
* dcacheManyFlushInval - Flush and/or Invalidate many blocks at once
*
* Walk through the entire block list, and perform the requested action
* for each of the blocks that fall within the specified range.
* All of these operations are done without releasing the mutex, so as to avoid
* anyone filling or modifying any of the blocks or otherwise rearranging of the
* LRU list while it is being traversed.
*
*/
LOCAL STATUS dcacheManyFlushInval
    (
    CBIO_DEV_ID dev,            /* device handle */
    block_t     startBlock,     /* range start - inclusive */
    block_t     end_block,      /* range end - inclusive */
    BOOL        doFlush,        /* TRUE : flush DIRTY blocks */
    BOOL        doInvalidate,   /* TRUE : Invalidate blocks */
    u_long *    pWriteCounter   /* where to count # of writes */
    )
    {
    DCACHE_DESC * pDesc, * pNext ;
    STATUS stat = OK ;
    DL_LIST     flushList ;
    u_long      writeCounter = 0 ;


    /* init the list in which we store all blocks to be flushed */
    dllInit( & flushList );

    /* start with Least Recently Used block, from tail of list */
    pDesc = (DCACHE_DESC *) DLL_LAST( & dev->pDc->dcLru );

    /* walk through the list towards the top */
    while ( (pDesc != NULL) && (stat == OK ) )
        {
        /* next, please ... */
        pNext = (DCACHE_DESC *) DLL_PREVIOUS(pDesc);

        if( (pDesc->block < startBlock) || (pDesc->block > end_block) )
            goto next;

        switch( pDesc->state )
            {
            case CB_STATE_EMPTY:/* no valid block is assigned */
            case CB_STATE_UNSTABLE:
                break;          /* ditto */

            case CB_STATE_DIRTY:/* contains a valid but modified in memory */
                if( doFlush )
                    {
                    /* remove from LRU list, add to flush batch */
                    dllRemove( & dev->pDc->dcLru,  &pDesc->lruList );
                    dcacheListAddSort( &flushList, pDesc );
                    writeCounter ++ ;
                    goto next ;
                    /* because of batch flush, these are invalidated later */
                    }

            case CB_STATE_CLEAN:/* contains a valid block, unmodified */
                if( doInvalidate )
                    {
                    dcacheHashRemove(dev, pDesc);
                    }
                break ;

            default:
                assert(pDesc->state == CB_STATE_DIRTY);
                break ;
            }
next:
        pDesc = pNext ;
        }

    if( ! DLL_EMPTY( &flushList ) )
        {
        stat = dcacheFlushBatch( dev, &flushList, doInvalidate ) ;
        if( pWriteCounter != NULL )
            *pWriteCounter += writeCounter ;
        }
    
    return (stat);
    }

/*******************************************************************************
*
* dcacheBlockAllocate - allocate a cache block with disk data
*
* Allocate a cache block to be associated with a certain disk block.
* At this point we assume the block IS NOT already on the list
*
* NOTE: The cbioMutex must already be taken when entering this function.
*/
LOCAL DCACHE_DESC * dcacheBlockAllocate( CBIO_DEV_ID dev, block_t block )
    {
    FAST DCACHE_DESC * pDesc  ;
    FAST DCACHE_DESC ** ppHashSlot ;
    FAST struct dcacheCtrl * pDc = dev->pDc ;
    int retry = 10 ;

again:
    /* start with Least Recently Used block, from tail of list */
    pDesc = (DCACHE_DESC *) DLL_LAST( & pDc->dcLru );

    if( retry -- <= 0)
        {
        errno = EAGAIN;
        return( NULL );
        }

    /* never reuse blocks which are Dirty or Unstable */
    while ( (pDesc->state == CB_STATE_DIRTY) ||
            (pDesc->state == CB_STATE_UNSTABLE))
        {
        pDesc = (DCACHE_DESC *) DLL_PREVIOUS(pDesc);
        if( pDesc == NULL)
            {
            break;
            }
        }

    if( pDesc == NULL )
        {
        STATUS stat ;

        stat = dcacheManyFlushInval( dev, 0, NONE, TRUE, FALSE,
                        & pDc->dcWritesForeground );

        if( stat == ERROR)
            return (NULL);

        goto again;
        }

    assert( (pDesc->state == CB_STATE_EMPTY) ||
            (pDesc->state == CB_STATE_CLEAN));

    /* in case this block contained some valid block remove it from hash */
    if( pDesc->state != CB_STATE_EMPTY )
        dcacheHashRemove(dev, pDesc);

    /* mark block fields */
    pDesc->state = CB_STATE_UNSTABLE ;
    pDesc->block = block ;

    assert( dev == pDc->cbioDev );

    if( pDc->dcHashSize > 0)
        {
        /* insert the block into hash table too */
        ppHashSlot = &(pDc->ppDcHashBase [ block % pDc->dcHashSize ]);

        if( *ppHashSlot != NULL )
            assert( (block % pDc->dcHashSize) ==
                    ((*ppHashSlot)->block % pDc->dcHashSize));

        pDesc->pHashNext = *ppHashSlot ;
        *ppHashSlot =  pDesc ;
        pDesc->busy = 1;
        }

    return (pDesc );
    }

/*******************************************************************************
*
* dcacheBlockFill - fill a cache block with disk data
*
* Reading can be costly if done a block at a time, hence
* the use of the BigBuf to read ahead as much blocks as configured
* fit into BigBuf, and spread them into cache buffers.
*/
LOCAL STATUS dcacheBlockFill( CBIO_DEV_ID dev, DCACHE_DESC * pDesc )
    {
    FAST struct dcacheCtrl * pDc = dev->pDc ;
    STATUS stat = OK;
    int off ;
    int numBlks ;
    block_t block ;
    block_t startBlock ;

    if ( dev->cbioParams.cbioRemovable && 
        (pDesc->block != DCACHE_BOOT_BLOCK_NUM ) &&
        (pDc->dcActTick < tickGet() - sysClkRateGet() * DCACHE_IDLE_SECS ))
        {
        stat = dcacheChangeDetect( dev, FALSE );

        if(stat == ERROR)
            return (stat);
        }

    /* if read ahead is possible ... */

    if( (pDc->dcBigBufSize > 1 ) && (pDc->dcReadAhead > 1))
        {
        block = startBlock = pDesc->block ;

        /* actually this is read-ahead size we are dealing with */
        numBlks = min( pDc->dcBigBufSize, pDc->dcReadAhead ); 

        /* and dont use-up all clean blocks we got right now */
        numBlks = min( (u_long) numBlks,
            (u_long)(pDc->dcNumCacheBlocks - pDc->dcDirtyCount)/2 );

        /* and if we're at the end of the device */
        numBlks = min( (u_long)numBlks, 
                        (u_long)dev->cbioParams.nBlocks - block );

        /* BEGIN - Hidden write handling */
        /* make sure the jump is Forward and Significantly large */
        if(pDc->dcLastAccBlock <
                (pDesc->block - pDc->dcCylinderSize))
            {
            stat = dcacheManyFlushInval( dev,
                        pDc->dcLastAccBlock,
                        startBlock + numBlks,
                        TRUE, FALSE,
                        & pDc->dcWritesHidden );
            }

        if( stat == ERROR)      /* write errors count be ignored */
            return( stat );
        /* END   - Hidden write handling */

        stat = pDc->dcSubDev->pFuncs->cbioDevBlkRW(
                    pDc->dcSubDev,
                    startBlock, numBlks,
                    pDc->dcBigBufPtr,
                    CBIO_READ, NULL );

        if( stat == ERROR )
            goto read_single ;

        pDc->dcLastAccBlock = startBlock + numBlks;

        /* dump each prefetched block into its own block buffer */
        do 
            {
            /* calculate how far into the BBlk we need to go */
            off = (pDesc->block - startBlock) * dev->cbioParams.bytesPerBlk ;

            /* perform the sweep: move from bigBuf to normal blocks */
            pDesc->state = CB_STATE_UNSTABLE ;
            bcopy( pDc->dcBigBufPtr + off, 
                   pDesc->data , dev->cbioParams.bytesPerBlk );
            /* make this block MRU */
            dllRemove( & pDc->dcLru, &pDesc->lruList );
            dllInsert( & pDc->dcLru, NULL, &pDesc->lruList );
            pDesc->state = CB_STATE_CLEAN ;
            
            /* another one is done */
            numBlks -- ; block ++ ; pDesc = NULL ;

            /* get us more them blocks for read-ahead data */
            if( numBlks > 0)
                {
                /* locate block in LRU list, although it shouldn't be there */
                if( dcacheBlockLocate(dev, block) != NULL )
                    {
                    break;      /* if found, terminate read-ahead */
                    }

                /* now really, allocate one */
                pDesc = dcacheBlockAllocate(dev, block);

                /* to clean blocks, stop here */
                if (pDesc == NULL )
                    {
                    break ; /* from while loop */
                    }

                } /* if numBlks > 0 */

            } while (numBlks > 0);

        /* return here if read ahead was successful */
        pDc->dcActTick = tickGet() ;
        return (OK);
        } /* if read ahead is possible ... */

        /* if read-ahead was not possible, read single block */

read_single:

    /* get the actual data from disk */
    stat = pDc->dcSubDev->pFuncs->cbioDevBlkRW(
            pDc->dcSubDev,
            pDesc->block, 1,
            pDesc->data,
            CBIO_READ, NULL );

    if(stat == ERROR)
        {
        stat = dcacheErrorHandler( dev, pDesc, CBIO_READ );
        }
    else
        {
        pDesc->state = CB_STATE_CLEAN ;
        }

    pDc->dcLastAccBlock = pDesc->block;

    pDc->dcActTick = tickGet() ;

    return (stat);
    }

/*******************************************************************************
*
* dcacheBlkBypassRW - read or write operation bypassing the disk cache
*
* Just call the underlying block driver Read or Write function.
* This should be called when we own the mutex, to avoid conflict
* in case some other task messes with the same blocks we use,
* and to maintain predictable I/O performance.
*/
LOCAL STATUS dcacheBlkBypassRW
    (
    CBIO_DEV_ID         dev, 
    block_t             startBlock,
    block_t             numBlocks,
    addr_t              buffer,
    CBIO_RW             rw
    )
    {
    STATUS stat = OK ;
    FAST struct dcacheCtrl * pDc = dev->pDc ;

    /* NOTE: no need to do change detect here, its been done by ManyInval */

    stat = pDc->dcSubDev->pFuncs->cbioDevBlkRW(
            pDc->dcSubDev,
            startBlock, numBlocks, buffer, 
            rw, NULL );

    if(stat == ERROR)
        stat = dcacheErrorHandler( dev, NULL, rw );

    pDc->dcLastAccBlock = startBlock + numBlocks ;
    pDc->dcActTick = tickGet() ;

    return (stat);
    }

/*******************************************************************************
*
* dcacheBlockLocate - locate a block in the cache list
*
* First, search the block in the has table, if that is not
* possible, find the block in the LRU list.
*/
LOCAL DCACHE_DESC * dcacheBlockLocate( CBIO_DEV_ID dev, block_t block )
    {
    FAST DCACHE_DESC *pDesc = NULL;
    FAST struct dcacheCtrl * pDc = dev->pDc ;

    if( pDc->dcHashSize > 0)
        {
        /* First, search in hash table */
        pDesc = pDc->ppDcHashBase [ block % pDc->dcHashSize ] ;

        pDc->dcHashHits ++ ;            /* think positively */

        while( pDesc != NULL )
            {
            /* verify all blocks are in correct hash slot */
            assert( (block % pDc->dcHashSize) == 
                    (pDesc->block % pDc->dcHashSize) );
            if( pDesc->block == block )
                return (pDesc);
            else
                pDesc = pDesc->pHashNext ;
            }

        /* hash miss, block not in cache */
        pDc->dcHashHits -- ; pDc->dcHashMisses ++ ;
        return NULL;
        }

    /* Now search the LRU list - linear search */
    pDesc = (DCACHE_DESC *) DLL_FIRST( & pDc->dcLru );
    while( pDesc != NULL )
        {
        if( pDesc->block == block )
            {
            goto found ;
            }
        pDesc = (DCACHE_DESC *) DLL_NEXT(& pDesc->lruList );
        }

    return NULL;

found:
    return (pDesc );
    }

/*******************************************************************************
*
* dcacheDevDisable - Disable the disk cache for this device
*
* This function disables the cache by setting the bypass count to zero and 
* storing the old value, if there is already an old value then we won't 
* repeat the process though.
*
* RETURNS OK if cache is sucessfully disabled or ERROR. 
*/
STATUS dcacheDevDisable
    (
    CBIO_DEV_ID dev   /* CBIO device handle */
    )
    {
    CBIO_DEV    *pDev = (void *) dev; /* tmp holder */

    if (OK != cbioDevVerify (dev))
        return (ERROR);
    
    /* Get semaphore */

    if( ERROR == semTake( pDev->cbioMutex, WAIT_FOREVER) )
        {
        return (ERROR);
        }
    
    /* Flush cache */

    if (ERROR == dcacheManyFlushInval ( dev, 0, NONE, TRUE, FALSE,
                                & pDev->pDc->dcWritesForced ) )
        {
        /* 
         * Something wrong if we can't flush the cache, so
         * give the semaphore back and exit with error status 
         */
        semGive (pDev->cbioMutex);
        return (ERROR);
        }

    /* Establish writethrough. If the bypass count isn't already zero */

    if (pDev->pDc->dcBypassCount != 0)
        {
        /* Save the current setting */

        pDev->pDc->dcOldBypassCount = pDev->pDc->dcBypassCount;
        pDev->pDc->dcBypassCount = 0;
        }                                                         
    else
        {
        /* It wasn't enabled, return ERROR */

        semGive (pDev->cbioMutex);
        return (ERROR);                      
        }

    /* Release semaphore */

    semGive (pDev->cbioMutex);

    return (OK);
    }

/*******************************************************************************
*
* dcacheDevEnable - Reenable the disk cache
*
* This function re-enables the cache if we disabled it. If we did not disable 
* it, then we cannot re-enable it.
*
* RETURNS OK if cache is sucessfully enabled or ERROR. 
*/
STATUS dcacheDevEnable
    (
    CBIO_DEV_ID dev  /* CBIO device handle */
    )
    {
    CBIO_DEV    *pDev = (void *) dev;
   
    if (OK != cbioDevVerify (dev))
        return (ERROR);

    /* 
     * Get semaphore first, otherwise another task could call
     * disable after we are part way through, they shouldn't
     * but guess who's fault it would be if they did 
     */

    if( ERROR == semTake( pDev->cbioMutex, WAIT_FOREVER) )
        {
        return (ERROR);
        }
        
    /* 
     * If there is no stored bypass count then we didn't disable
     * it, so return ERROR                                       
     */

    if (0 == pDev->pDc->dcOldBypassCount)
        {
        /* Return the semaphore before exit */
        semGive(pDev->cbioMutex);

        return (ERROR);
        }
    
    /* Restore the bypass count and the caching will start again */

    pDev->pDc->dcBypassCount = pDev->pDc->dcOldBypassCount;
    
    /* Zero out the old count so we know it isn't disabled */

    pDev->pDc->dcOldBypassCount = 0;
    
    /* Release semaphore */

    semGive(pDev->cbioMutex);

    return (OK);
    }

/*******************************************************************************
*
* dcacheBlockGet - get a disc block in a cache block
*
* This function is code which is common for reading and writing bytewise
* as well as reading blockwise.
* NOTE: The cbioMutex must already be taken when entering this function.
*/
LOCAL DCACHE_DESC * dcacheBlockGet( CBIO_DEV_ID dev, block_t block,
        cookie_t * pCookie, BOOL readData )
    {
    DCACHE_DESC * pDesc = NULL ;
    BOOL allocated = FALSE ;
    STATUS stat;

    /* First, verify that pCookie is valid, otherwise destroy it */
    if( pCookie != NULL )
        if( *pCookie != 0)
            {
            /* these sanity tests are not needed after debugging is complete */
            assert( ((caddr_t) *pCookie >= dev->cbioMemBase) &&
                  ((caddr_t) *pCookie < (dev->cbioMemBase+dev->cbioMemSize)));
            }

    /* try to locate block by pCookie if valid */
    if( pCookie != NULL )
        if( *pCookie != 0)
            {
            pDesc = (void *) *pCookie ;
            if( pDesc->block != block )
                {
                dev->pDc->dcCookieMisses ++ ;
                pDesc = NULL;
                }
            else
                {
                dev->pDc->dcCookieHits ++ ;
                return(pDesc);
                /* 
                 * we omitted here making this block MRU,
                 * pCookie references should be very fast,
                 * at a minimal risk of loosing this block
                 * earlier then it normally would happened.
                 */
                }
            }

    /* locate block in LRU list */
    if( pDesc == NULL)
        pDesc = dcacheBlockLocate(dev, block);

    /* block not found, allocate one */
    if( pDesc == NULL)
        {
        dev->pDc->dcLruMisses ++ ;
        allocated = TRUE ;
        pDesc = dcacheBlockAllocate(dev, block);

        /* if that failed, something is basically wrong here */
        if (pDesc == NULL )
            {
            return NULL ;
            }
        }
    else
        {
        dev->pDc->dcLruHits ++ ;
        }

    /* if this is a write-only operation,  -> DIRTY shortcut */
    if( ! readData )
        {
        if( pDesc->state != CB_STATE_DIRTY )
            dev->pDc->dcDirtyCount ++ ;
        pDesc->state = CB_STATE_DIRTY ;
        }

    if( pCookie != NULL )
        {
        *((void * *)pCookie) = (void *) pDesc ;
        }

    /* fill block with data from disk if needed */
    if ( readData && allocated )
        {
        stat = dcacheBlockFill( dev, pDesc );

        if( stat == ERROR)
            {
            dcacheHashRemove(dev, pDesc);
            /* the disk may have been removed */
            return NULL ;
            }

        assert(pDesc->state == CB_STATE_CLEAN);
        }

    /* make this block MRU */
    dllRemove( & dev->pDc->dcLru, &pDesc->lruList );
    dllInsert( & dev->pDc->dcLru, NULL, &pDesc->lruList );

    if( (pDesc->block == block) )
        {
        return (pDesc);
        }
    else
        {
        /* this should never happen, unless tunable params are out of sanity */
        INFO_MSG("dcacheCbio: dev=%#x cache internal error - starvation %d\n",
                (u_int) dev, pDesc->block, 0, 0, 0, 0);
        errno = EAGAIN;
        return(NULL);
        }
    }


/*******************************************************************************
*
* dcacheBlkRW - Read/Write blocks
*
* This routine transfers between a user buffer and the lower layer CBIO
* It is optimized for block transfers.  
*
* RETURNS OK or ERROR and may otherwise set errno.
*/

LOCAL STATUS dcacheBlkRW
    (
    CBIO_DEV_ID         dev,
    block_t             startBlock,
    block_t             numBlocks,
    addr_t              buffer,
    CBIO_RW             rw,
    cookie_t            *pCookie
    )
    {
    CBIO_DEV *  pDev = (void *) dev ;
    DCACHE_DESC *pDesc;
    STATUS stat = OK ;

    if( TRUE == cbioRdyChgdGet (dev))
        {
        errno = S_ioLib_DISK_NOT_PRESENT ;
        return ERROR;
        }

    if( (startBlock) > pDev->cbioParams.nBlocks ||
        (startBlock+numBlocks) > pDev->cbioParams.nBlocks )
        {
        errno = EINVAL;
        return ERROR;
        }

    startBlock += pDev->cbioParams.blockOffset;

    DEBUG_MSG("dcacheBlkRW: blk %d num blocks %d\n",
        startBlock, numBlocks, 0,0,0,0 );

    if( semTake( pDev->cbioMutex, WAIT_FOREVER) == ERROR )
        return ERROR;

    /* by-pass cache for large transfers */
    if( numBlocks > pDev->pDc->dcBypassCount ) 
        {
        BOOL doFlush = (rw == CBIO_READ)? TRUE : FALSE ;
        BOOL doInval = (rw == CBIO_WRITE)? TRUE : FALSE ;

        /* BEGIN - Hidden write handling */
        if( dev->pDc->dcLastAccBlock < startBlock )
            {
            stat = dcacheManyFlushInval( dev,
                        dev->pDc->dcLastAccBlock,
                        startBlock,
                        TRUE, FALSE,
                        & dev->pDc->dcWritesHidden );
            if( ERROR == stat )
                goto read_error;
            }
        /* END   - Hidden write handling */

        stat = dcacheManyFlushInval( dev, startBlock,
                        startBlock + numBlocks -1 ,
                        doFlush, doInval, & pDev->pDc->dcWritesHidden );


        if( OK == stat )
            {
            stat = dcacheBlkBypassRW( dev, startBlock, numBlocks, buffer, rw);
            semGive(pDev->cbioMutex);
            return( stat ) ;
            }
        }

    for( ; numBlocks > 0; numBlocks-- ) /* For each block req'ed */
        {
        /* skip reading back the data if writing entire blocks */
        BOOL readData = (rw == CBIO_READ)? TRUE : FALSE ;
        CB_STATE saveState ;

        /* get that block */
        pDesc = dcacheBlockGet(pDev, startBlock, pCookie, readData);

        if( pDesc == NULL )
            goto read_error ;

        /* Debug: make sure the block still has got the data */
        assert(  (pDesc->state == CB_STATE_CLEAN) ||
                 (pDesc->state == CB_STATE_DIRTY) );

        /* move data between the cached block and caller's buffer */
        switch( rw )
            {
            case CBIO_READ:
                bcopy( pDesc->data, buffer, pDev->cbioParams.bytesPerBlk );
                break;
            case CBIO_WRITE:
                saveState = pDesc->state ;
                pDesc->state = CB_STATE_UNSTABLE ;
                bcopy( buffer, pDesc->data, pDev->cbioParams.bytesPerBlk );
                /* in case it was already CLEAN, we mark it DIRTY,
                 * so that it will be written to disk again.
                 */
                if( saveState != CB_STATE_DIRTY )
                    dev->pDc->dcDirtyCount ++ ;
                pDesc->state = CB_STATE_DIRTY ;
                break;
            } /* switch */

        startBlock ++ ; /* prepare for next */
        buffer += pDev->cbioParams.bytesPerBlk ;
        }       /* end of: For each block req'ed */

    if( dev->pDc->dcDirtyCount > dev->pDc->dcDirtyMax )
        stat = dcacheManyFlushInval( pDev, 0, NONE, TRUE, FALSE,
                        & pDev->pDc->dcWritesForeground );

    semGive(pDev->cbioMutex);
    return stat;
read_error:
    semGive(pDev->cbioMutex);
    return ERROR;
    }

/*******************************************************************************
*
* dcacheBytesRW - Read/Write bytes
*
* This routine transfers between a user buffer and the lower layer hardware
* It is optimized for byte transfers.  
*
* RETURNS OK or ERROR and may otherwise set errno.
*/
LOCAL STATUS dcacheBytesRW
    (
    CBIO_DEV_ID         dev,
    block_t             startBlock,
    off_t               offset,
    addr_t              buffer,
    size_t              nBytes,
    CBIO_RW             rw,
    cookie_t            *pCookie
    )
    {
    FAST CBIO_DEV *     pDev = (void *) dev ;
    FAST struct dcacheCtrl * pDc = dev->pDc ;
    FAST DCACHE_DESC *  pDesc;
    BOOL readData = TRUE;       /* read data from disk, always */
    STATUS stat = OK ;
    CB_STATE saveState ;

    if( TRUE == cbioRdyChgdGet (pDev))
        {
        errno = S_ioLib_DISK_NOT_PRESENT ;
        return ERROR;
        }

    if( (startBlock) > pDev->cbioParams.nBlocks )
        {
        errno = EINVAL;
        return ERROR;
        }

    /* verify that all bytes are within one block range */
    if (((offset + nBytes) > pDev->cbioParams.bytesPerBlk ) ||
        (offset <0) || (nBytes <=0))
        {
        errno = EINVAL;
        return ERROR;
        }

    startBlock += pDev->cbioParams.blockOffset;

    DEBUG_MSG("dcacheBytesRW: blk %d num bytes %d, offset %d\n",
        startBlock, nBytes, offset, 0,0,0 );

    if( semTake( pDev->cbioMutex, WAIT_FOREVER) == ERROR )
        return ERROR;

    /* get that block */
    pDesc = dcacheBlockGet(pDev, startBlock, pCookie, readData);

    if( pDesc == NULL )
        goto _error ;

    /* Debug: make sure the block still has got the data */
    assert(  (pDesc->state == CB_STATE_CLEAN) ||
             (pDesc->state == CB_STATE_DIRTY) );

    /* move data between the cached block and caller's buffer */
    switch( rw )
        {
        case CBIO_READ:
            bcopy( pDesc->data+offset, buffer, nBytes);
            break;
        case CBIO_WRITE:
            saveState = pDesc->state ;
            pDesc->state = CB_STATE_UNSTABLE ;
            bcopy( buffer, pDesc->data+offset, nBytes);
            if( saveState != CB_STATE_DIRTY )
                pDc->dcDirtyCount ++ ;
            pDesc->state = CB_STATE_DIRTY;
            break;
        } /* switch */

    if( pDc->dcDirtyCount > pDc->dcDirtyMax )
        stat = dcacheManyFlushInval( pDev, 0, NONE, TRUE, FALSE,
                & pDev->pDc->dcWritesForeground );

    semGive(pDev->cbioMutex);
    return stat;
_error:
    semGive(pDev->cbioMutex);
    return ERROR;
    }

/*******************************************************************************
*
* dcacheBlkCopy - Copy sectors 
*
* This function will be mainly used to write backup copies of FAT,
* hence we may assume the the source blocks are in the cache anyway,
* and we assume the requests are never too big.
*
* This routine makes copies of one or more blocks on the lower layer CBIO
* It is optimized for block copies on the subordinate layer.  
* dev - the CBIO handle of the device being accessed (from creation routine)
* 
* RETURNS OK or ERROR and may otherwise set errno.
*/
LOCAL STATUS dcacheBlkCopy
    (
    CBIO_DEV_ID         dev,
    block_t             srcBlock,
    block_t             dstBlock,
    block_t             numBlocks
    )
    {
    CBIO_DEV    *pDev = (void *) dev ;
    DCACHE_DESC *pDescSrc, *pDescDst ;
    CB_STATE saveState ;

    if( TRUE == cbioRdyChgdGet (dev))
        {
        errno = S_ioLib_DISK_NOT_PRESENT ;
        return ERROR;
        }

    if( (srcBlock) > pDev->cbioParams.nBlocks ||
        (dstBlock) > pDev->cbioParams.nBlocks )
        {
        errno = EINVAL;
        return ERROR;
        }

    if( (srcBlock+numBlocks) > pDev->cbioParams.nBlocks ||
        (dstBlock+numBlocks) > pDev->cbioParams.nBlocks )
        {
        errno = EINVAL;
        return ERROR;
        }

    srcBlock += pDev->cbioParams.blockOffset;
    dstBlock += pDev->cbioParams.blockOffset;

    DEBUG_MSG("dcacheBlkCopy: blk %d to %d # %d blocks\n",
        srcBlock, dstBlock, numBlocks, 0, 0,0 );

    if( semTake( pDev->cbioMutex, WAIT_FOREVER) == ERROR )
        return ERROR;

    for( ; numBlocks > 0; numBlocks-- ) /* For each block req'ed */
        {
        if( dev->pDc->dcDirtyCount > dev->pDc->dcDirtyMax )
            if( dcacheManyFlushInval( pDev, 0, NONE, TRUE, FALSE,
                                    & pDev->pDc->dcWritesForeground) == ERROR )
                goto _error ;

        /* get a source  block , with data in it */
        pDescSrc = dcacheBlockGet(pDev, srcBlock, NULL, TRUE);

        if( pDescSrc == NULL )
            goto _error ;

        /* Debug: make sure the block still has got the data */
        assert(  (pDescSrc->state == CB_STATE_CLEAN) ||
                 (pDescSrc->state == CB_STATE_DIRTY) );

        /* get a destination  block , with or without data */
        pDescDst = dcacheBlockGet(pDev, dstBlock, NULL, FALSE);

        if( pDescDst == NULL )
            goto _error ;

        /* Debug: make sure the block still has got the data */
        assert(  (pDescDst->state == CB_STATE_CLEAN) ||
                 (pDescDst->state == CB_STATE_DIRTY) );

        /* move data between the cached blocks */
        saveState = pDescDst->state ;
        pDescDst->state = CB_STATE_UNSTABLE ;
        bcopy( pDescSrc->data, pDescDst->data, pDev->cbioParams.bytesPerBlk );
        if( saveState != CB_STATE_DIRTY )
            dev->pDc->dcDirtyCount ++ ;
        pDescDst->state = CB_STATE_DIRTY ;

        srcBlock ++ ;   /* prepare for next */
        dstBlock ++ ;   
        }       /* end of: For each block req'ed */

    semGive(pDev->cbioMutex);
    return OK;
_error:
    semGive(pDev->cbioMutex);
    return ERROR;
    }

/*******************************************************************************
*
* dcacheIoctl - Misc control operations 
*
* This performs the requested ioctl() operation.
* 
* CBIO modules can expect the following ioctl() codes from cbioLib.h:
* CBIO_RESET - reset the CBIO device and the lower layer
* CBIO_STATUS_CHK - check device status of CBIO device and lower layer
* CBIO_DEVICE_LOCK - Prevent disk removal 
* CBIO_DEVICE_UNLOCK - Allow disk removal
* CBIO_DEVICE_EJECT - Unmount and eject device
* CBIO_CACHE_FLUSH - Flush any dirty cached data
* CBIO_CACHE_INVAL - Flush & Invalidate all cached data
* CBIO_CACHE_NEWBLK - Allocate scratch block
*
* RETURNS OK or ERROR and may otherwise set errno.
*/
LOCAL STATUS dcacheIoctl
    (
    CBIO_DEV_ID dev,
    UINT32      command,
    addr_t      arg
    )
    {
    STATUS stat = OK ;
    FAST struct dcacheCtrl * pDc = dev->pDc ;
    DCACHE_DESC *pDesc = NULL ;

    if( OK != cbioDevVerify(dev))
        {
        DEBUG_MSG("dcacheIoctl: invalid handle\n",0,0,0,0,0,0);
        return ERROR;
        }

    if( (TRUE == cbioRdyChgdGet (dev))  && ( command != (int) CBIO_RESET ))
        {
        errno = S_ioLib_DISK_NOT_PRESENT ;
        return ERROR;
        }

    /* avoid writes upon known O_RDONLY devices */

    switch ( command )
        {
        case CBIO_CACHE_FLUSH:
        case CBIO_CACHE_INVAL:
        case CBIO_CACHE_NEWBLK:
            if (O_RDONLY == cbioModeGet(dev))
                {
                errno = S_ioLib_WRITE_PROTECTED;
                return (ERROR);
                }
            break;
        default:
            break;
        }

    if( semTake( dev->cbioMutex, WAIT_FOREVER) == ERROR )
        return ERROR;

    switch ( command )
        {
        case CBIO_RESET :
            /* reset subordinate device, pass along 3rd argument */

            pDc->dcSubDev->pFuncs->cbioDevIoctl(pDc->dcSubDev, command, arg);

            /* if device present, subordinate has its readyChanged FALSE */

            CBIO_READYCHANGED (dev) = cbioRdyChgdGet (pDc->dcSubDev);

            /* since the disk geometry may have changed, we must re-init */

            if(FALSE == cbioRdyChgdGet (dev))
                {
                stat = dacacheDevInit(dev);
                /* read new signature */
                (void) dcacheChangeDetect( dev, TRUE );
                CBIO_READYCHANGED (dev)  = FALSE;
                }

            if( stat == ERROR && errno == OK )
                errno = S_ioLib_DISK_NOT_PRESENT ;
            break;

        case CBIO_CACHE_FLUSH :
                {
                if( ( arg  == (addr_t)-1 ) || (pDc->dcSyncInterval == 0) ||
                    (dcacheUpdTaskId == 0))
                    {
                    /* if forced, or if no updater task, FLUSH inline */
                    stat = dcacheManyFlushInval( dev, 0, NONE, TRUE, FALSE,
                                & pDc->dcWritesForced );
                    }
                else if (arg == 0)
                    {
                    dcacheQuickFlush(); /* TODO HELP!  review this design */
                    }
                else
                    {
                    /* lazy flush request */
                    pDc->dcUpdTick = (int) arg * sysClkRateGet() + tickGet();
                    }
                }
                break;


        case CBIO_CACHE_INVAL :
                stat = dcacheManyFlushInval( dev, 0, NONE, TRUE, TRUE,
                                & pDc->dcWritesForced );
                break;


        case CBIO_STATUS_CHK :
            /* avoid checking too frequently */
            if ( pDc->dcActTick <
                        tickGet() - sysClkRateGet() * DCACHE_IDLE_SECS )
                {
                stat = pDc->dcSubDev->pFuncs->cbioDevIoctl(
                        pDc->dcSubDev, CBIO_STATUS_CHK, 0);
                /*    
                 * SPR#68387: readyChanged bit of the BLK_DEV
                 * should be checked to verify the change
                 * in the media. If there is a change in the
                 * media then initialize the cache layer's datastructures.
                 */              

                if( TRUE == cbioRdyChgdGet(dev))
                    { 
                    CBIO_READYCHANGED (dev) = FALSE;
                    dacacheDevInit(dev);
                    }

                /* disk seems ready, but is it the same one ? */
                if( stat != ERROR )
                    {
                    stat = dcacheChangeDetect( dev, TRUE );
                    pDc->dcActTick = tickGet() ;
                    }
                }
                break;


        case CBIO_CACHE_NEWBLK :        /* Allocate scratch block */
            arg += dev->cbioParams.blockOffset ;
            /* get that block */
            pDesc = dcacheBlockGet(dev, (block_t) arg, NULL, FALSE);
            /* zero-fill the block so that unwritten data reads 0s */
            bzero( pDesc->data, dev->cbioParams.bytesPerBlk );
            break ;

        case CBIO_DEVICE_LOCK :         /* these belong to low-level */
        case CBIO_DEVICE_UNLOCK :
        case CBIO_DEVICE_EJECT :
        default:
            stat = pDc->dcSubDev->pFuncs->cbioDevIoctl(
                        pDc->dcSubDev, command, arg);
        }

    semGive(dev->cbioMutex);

    return (stat);
    }


/*******************************************************************************
*
* shiftCalc - calculate how many shift bits
*
* How many shifts <n> are needed such that <mask> == 1 << <N>
* This is very useful for replacing multiplication with shifts,
* where it is known a priori that the multiplier is 2^k.
*
* RETURNS: Number of shifts.
*/

LOCAL int shiftCalc
    (
    u_long mask
    )
    {
    int i;

    for (i=0; i<32; i++)
        {
        if (mask & 1)
            break ;
        mask = mask >> 1 ;
        }
    return( i );
    }

/*******************************************************************************
*
* dcacheTunableVerify - verify the tunable parameters for sanity
*
*/
LOCAL void dcacheTunableVerify( CBIO_DEV_ID dev )
    {
    FAST struct dcacheCtrl * pDc = dev->pDc ;

    /* read-ahead - shouldn't be less then 2 */
    pDc->dcReadAhead = max( 2, pDc->dcReadAhead );

    /* read-ahead - shouldn't be more then BigBuf size */
    pDc->dcReadAhead = min( pDc->dcBigBufSize, 
        pDc->dcReadAhead );

    /* dirty max - should be at least as large as BugBuf*2 to be worth */
    pDc->dcDirtyMax = max(pDc->dcBigBufSize*2,
        pDc->dcDirtyMax);

    /* dirty max - must not be more then say 80% of total cache */
    pDc->dcDirtyMax = min((pDc->dcNumCacheBlocks*4)/5,
        pDc->dcDirtyMax);

    /* bypass count could be anything, but no less then two */
    pDc->dcBypassCount = max( 2, pDc->dcBypassCount );

    /* Only modify if not hand tuned */
    if (pDc->dcIsTuned == FALSE )
        {
        /* don't allow long sync period for removable devices */
        if( dev->cbioParams.cbioRemovable )
            {
            pDc->dcSyncInterval = min( 1, pDc->dcSyncInterval );
            }
        }


    /* cylinder size is used to rationalize hidden write attempts */
    pDc->dcCylinderSize = max( (u_long) pDc->dcDirtyMax, 
        (u_long) dev->cbioParams.blocksPerTrack * dev->cbioParams.nHeads );
    }

/*******************************************************************************
*
* dcacheMemInit - initialize disk cache memory structure
*
*
*/
LOCAL STATUS dcacheMemInit( CBIO_DEV_ID dev )
    {
    FAST struct dcacheCtrl * pDc = dev->pDc ;
    DCACHE_DESC *pDesc ;
    caddr_t     data ;
    int         size ;
    block_t     nBlk ;
    u_long      n ;
    int         sizeBBlk = 0;

    /* Init LRU list to be empty */
    dllInit( & pDc->dcLru );

    /* calculate how many blocks we can afford to keep in our cache */
    size = dev->cbioMemSize ;

    sizeBBlk = size / dev->cbioParams.bytesPerBlk / 4 ;

    if( sizeBBlk < 2 )
        sizeBBlk = 0; /* FIXME - avoid using it if it isn't there */

    /* too big burst is counter-productive, so limit it to typically 63K */
    sizeBBlk = min(sizeBBlk, 127);

    size -= sizeBBlk * dev->cbioParams.bytesPerBlk ;

    nBlk = size / ( dev->cbioParams.bytesPerBlk + sizeof(DCACHE_DESC) );

    pDc->dcNumCacheBlocks               = nBlk ;

    /* Lookup an appropriate entry in the presets table */
    for(n = 0; n < (NELEMENTS( dcacheTunablePresets )-1); n++ )
        {
        if(nBlk <= dcacheTunablePresets[n].dcNumCacheBlocks )
            break;
        }

    /* having found the right entry in the presets table, use it */

    /* If the device is already tuned then leave it alone SPR#30130 */
    if ( pDc->dcIsTuned == FALSE )
      {
      pDc->dcDirtyMax     = dcacheTunablePresets[n].dcDirtyMax;
      pDc->dcBypassCount  = dcacheTunablePresets[n].dcBypassCount;
      pDc->dcReadAhead    = dcacheTunablePresets[n].dcReadAhead;
      pDc->dcSyncInterval = dcacheTunablePresets[n].dcSyncInterval;
      }
    pDc->dcHashSize     = dcacheTunablePresets[n].dcHashSize;

    /* re-calculate the nBlk now accounting for hash table size too */
    size -= pDc->dcHashSize * sizeof(caddr_t) ;
    nBlk = size / ( dev->cbioParams.bytesPerBlk + sizeof(DCACHE_DESC) );

#ifdef  DEBUG
    if( pDc->dcNumCacheBlocks != nBlk )
        {
        INFO_MSG("dcacheMemInit: # of blocks reduced, %d to %d due to hash\n",
                pDc->dcNumCacheBlocks, nBlk, 0,0,0,0);
        }
#endif  /*DEBUG*/

    pDc->dcNumCacheBlocks               = nBlk ;

    /* set address for first descriptor and data block */
    pDesc = (void *) dev->cbioMemBase ;

    data = dev->cbioMemBase + (nBlk * sizeof(DCACHE_DESC));
    DEBUG_MSG("dcacheMemInit: cache size %d bytes, curved %d blocks, at %#x\n",
        dev->cbioMemSize, nBlk, (int) dev->cbioMemBase, 0,0,0 );

    for( n = nBlk ; n > 0; n -- )
        {
        /* init fields */
        pDesc->block = NONE;
        pDesc->state = CB_STATE_EMPTY ;
        pDesc->data  = data ;
        pDesc->busy = 0;
        pDesc->pHashNext = NULL;

        /* add node to list */
        dllAdd( & pDc->dcLru, &pDesc->lruList );

        /* advance pointers */
        pDesc ++ ;
        data += dev->cbioParams.bytesPerBlk ;
        }

    /* record size and location of burst block */
    pDc->dcBigBufSize = sizeBBlk ;
    if( sizeBBlk > 0)
        pDc->dcBigBufPtr = data ;
    else
        pDc->dcBigBufPtr = NULL ;

    data += sizeBBlk * dev->cbioParams.bytesPerBlk ;            /* forw mark */

    /* reset of memory is used for hash table */
    pDc->ppDcHashBase = (DCACHE_DESC **) data ;
    data += pDc->dcHashSize * sizeof(caddr_t) ;
    bzero( (caddr_t) pDc->ppDcHashBase,
                pDc->dcHashSize * sizeof(caddr_t) );

    dcacheTunableVerify( dev );

    DEBUG_MSG("dcacheMemInit: pDesc %x, data %x, sizeBBlk %d nBlk %d\n",
        (int) pDesc, (int) data, sizeBBlk, nBlk, 0, 0);

    /* self-test memory allocations */
    assert( (int) pDesc <=
        (int) (dev->cbioMemBase + (nBlk * sizeof(DCACHE_DESC)))) ;
    assert( data <= (dev->cbioMemBase + dev->cbioMemSize));

    return OK;
    }

/*******************************************************************************
*
* dcacheDevTune - modify tunable disk cache parameters
*
* This function allows the user to tune some disk cache parameters
* to obtain better performance for a given application or workload pattern.
* These parameters are checked for sanity before being used, hence it is
* recommended to verify the actual parameters being set with dcacheShow().
*
* Following is the description of each tunable parameter:
*
* .IP <bypassCount>
* In order to achieve maximum performance, Disk Cache is bypassed for very
* large requests. This parameter sets the threshold number of blocks for
* bypassing the cache, resulting usually in the data being transferred by
* the low level driver directly to/from application data buffers (also
* known as cut-through DMA). Passing the value of 0 in this argument
* preserves the previous value of the associated parameter.
*
* .IP <syncInterval>
* The Disk Cache provides a low priority task that will update all
* modified blocks onto the disk periodically. This parameters controls the
* time between these updates in seconds. The longer this period, the
* better throughput is likely to be achieved, while risking to loose more
* data in the event of a failure. For removable devices this interval is
* fixed at 1 second. Setting this parameter to 0 results in immediate
* writes to disk when requested, resulting in minimal data loss risk at
* the cost of somewhat degraded performance.
*
* .IP <readAhead>
* In order to avoid accessing the disk in small units, the Disk Cache
* will read many contiguous blocks once a block which is absent from the
* cache is needed. Increasing this value increases read performance, but a
* value which is too large may cause blocks which are frequently used to
* be removed from the cache, resulting in a low Hit Ratio, and increasing
* the number of Seeks, slowing down performance dramatically. Passing the
* value of 0 in this argument preserves the pervious value of the
* associated parameter.
*
* .IP <dirtyMax>
* Routinely the Disk Cache will keep modified blocks in memory until it is
* specifically instructed to update these blocks to the disk, or until the
* specified time interval between disk updates has elapsed, or until the
* number of modified blocks is large enough to justify an update. Because
* the disk is updated in an ordered manner, and the blocks are written in
* groups when adjacent blocks have been modified, a larger dirtyMax
* parameter will minimize the number of Seek operation, but a value which
* is too large may decrease the Hit Ratio, thus degrading performance.
* Passing the value of 0 in this argument preserves the pervious value of
* the associated parameter.
* 
* RETURNS: OK or  ERROR if device handle is invalid.
* Parameter value which is out of range will be silently corrected.
*
* SEE ALSO: dcacheShow()
*/

STATUS dcacheDevTune
    (
    CBIO_DEV_ID dev,            /* device handle */
    int         dirtyMax,       /* max # of dirty cache blocks allowed */
    int         bypassCount,    /* request size for bypassing cache */
    int         readAhead,      /* how many blocks to read ahead */
    int         syncInterval    /* how many seconds between disk updates */
    )
    {
    FAST struct dcacheCtrl * pDc = dev->pDc ;

    if( OK != cbioDevVerify(dev))
        {
        DEBUG_MSG("dcacheDevTune: invalid handle\n",0,0,0,0,0,0);
        return ERROR;
        }

    if( semTake( dev->cbioMutex, WAIT_FOREVER) == ERROR )
        return ERROR;

    /* Set tuneable parameters, 0 means use current */
    if( dirtyMax > 0 )
        pDc->dcDirtyMax = dirtyMax;

    if( bypassCount > 0 )
        pDc->dcBypassCount      = bypassCount;

    if( readAhead > 0 )
        pDc->dcReadAhead        = readAhead;

    /* Set sync interval, note 0 can be set here */
    if( syncInterval >= 0 )
        pDc->dcSyncInterval     = syncInterval;

    /* Remember we have tuned the cache, used to switch off defaulting */
    pDc->dcIsTuned = TRUE;

    dcacheTunableVerify( dev ); /* check and correct for sanity */


    semGive(dev->cbioMutex);
    return OK ;
    }

/*******************************************************************************
*
* dcacheDevMemResize - set a new size to a disk cache device
*
* This routine is used to resize the dcache layer.  This routine is also 
* useful after a disk change event, for example a PCMCIA disk swap.   
* The routine pccardDosDevCreate() in pccardLib.c uses this routine for that
* function.   This should be invoked each time a new disk is inserted on 
* media where the device geometry could possibly change.  This function 
* will re-read all device geometry data from the block driver, carve out
* and initialize all cache descriptors and blocks.
*
* RETURNS OK or ERROR if the device is invalid or if the device geometry is 
* invalid (EINVAL) or if there is not enough memory to perform the operation.
*/
STATUS dcacheDevMemResize
    (
    CBIO_DEV_ID dev,            /* device handle */
    size_t      newSize         /* new cache size in bytes */
    )
    {
    FAST struct dcacheCtrl * pDc = dev->pDc ;
    STATUS stat = OK ;
    caddr_t pNewMem = NULL ;

    if( OK != cbioDevVerify (dev))
        {
        DEBUG_MSG("dcacheDevMemResize: invalid handle\n",0,0,0,0,0,0);
        return ERROR;
        }

    if( semTake( dev->cbioMutex, WAIT_FOREVER) == ERROR )
        return ERROR;

    /* first, flush and invalidate all cache blocks */
    stat = dcacheManyFlushInval( dev, 0, NONE, TRUE, TRUE,
                                & pDc->dcWritesForced );

    if( stat == OK)
        pNewMem = KHEAP_REALLOC( dev->cbioMemBase, newSize );

    if( pNewMem == NULL)
        stat = ERROR ;

    if( stat == OK )
        {
        dev->cbioMemBase = pNewMem ;
        dev->cbioMemSize = newSize ;
        stat = dacacheDevInit(dev);
        }

    semGive(dev->cbioMutex);
    return stat ;
    }


/*******************************************************************************
*
* dacacheDevInit - initialize the disk cache and control
*
* This initialization should be repeated each time a new disk
* is inserted, because block size may have changed.
* Re-read all geometry data from block driver, cut and initialize
* all cache descriptors and blocks.
*/
LOCAL STATUS dacacheDevInit ( CBIO_DEV_ID dev )
    {
    char        shift;
    int         bytesPerBlk;

    bytesPerBlk = dev->pDc->dcSubDev->cbioParams.bytesPerBlk ;
    
    /* first check that bytesPerBlk is 2^n */
    shift = shiftCalc( bytesPerBlk );

    if( bytesPerBlk != ( 1 << shift ) )
        {
        errno = EINVAL;
        return( ERROR );
        }

    dev->cbioDesc       = "CBIO Disk Data Cache - LRU";
    dev->cbioParams.bytesPerBlk = bytesPerBlk ;
    dev->cbioParams.blockOffset = 0;
    dev->cbioParams.lastErrBlk  = NONE ;

    CBIO_READYCHANGED(dev)      = FALSE;

    dev->cbioParams.lastErrno   = 0 ;
    dev->cbioParams.nBlocks     = dev->pDc->dcSubDev->cbioParams.nBlocks ;
    dev->cbioParams.nHeads      = dev->pDc->dcSubDev->cbioParams.nHeads ;
    dev->cbioMode               = dev->pDc->dcSubDev->cbioMode ;

    dev->pDc->dcDirtyCount      = 0 ;
    dev->pDc->dcLastAccBlock    = NONE ;
    dev->cbioParams.cbioRemovable       = 
                dev->pDc->dcSubDev->cbioParams.cbioRemovable ;
    dev->cbioParams.blocksPerTrack      = 
                dev->pDc->dcSubDev->cbioParams.blocksPerTrack ;

    dcacheMemInit(dev) ;

    return (OK);
    }

/*******************************************************************************
*
* dcacheQuickFlush - Called from ioctl, instead of rescheduling the lazy writer
*
* NOMANUAL
*/

LOCAL void dcacheQuickFlush (void)
    {
    FAST int i ;
    CBIO_DEV_ID dev ;

    for(i = 0; i < (int) NELEMENTS( dcacheCtrl); i ++ )
        {
        dev = dcacheCtrl[i].cbioDev ;

        if(NULL == dev)
            {
            continue ;
            }

        /* skip the device if its busy now */
        if( semTake( dev->cbioMutex, NO_WAIT) == ERROR )
            {
            continue;
            }

        if( dcacheCtrl[i].dcDirtyCount > 0 )
            {
            (void) dcacheManyFlushInval( dev, 0, NONE, TRUE, FALSE,
                       & dev->pDc->dcWritesForced );
            }
        else if (dev->cbioParams.cbioRemovable == TRUE)
            {
            /* because the device could been idle for some time, */
            (void) dcacheManyFlushInval( dev, 0, NONE, TRUE, TRUE,
                   & dev->pDc->dcWritesForced );
            DEBUG_MSG("cache blocks invalidated\n", 0,0,0,0,0,0);
            }

        dcacheCtrl[i].dcUpdTick = dcacheCtrl[i].dcSyncInterval * 
                                  sysClkRateGet() + tickGet();

        semGive( dev->cbioMutex );
        }
    return;
    }



/*******************************************************************************
*
* dcacheUpd - disk-cache updater task (lazy writer)
*
* NOMANUAL
*/

void dcacheUpd (void)
    {
    FAST int i ;
    FAST u_long tick ;
    CBIO_DEV_ID dev ;

    FOREVER
        {
        /* establish our priority */
        taskPrioritySet(0, dcacheUpdTaskPriority);

        /* all update intervals are in seconds, pause a second every time */
        taskDelay(sysClkRateGet() >> DCACHE_UPD_TASK_GRANULARITY  );

        tick = tickGet();

        for(i = 0; i < (int)NELEMENTS( dcacheCtrl); i ++ )
            {
            dev = dcacheCtrl[i].cbioDev ;

            if(NULL == dev)
                {
                continue ;
                }

            /* our time has not come yet or readyChanged is set */
            if( (dcacheCtrl[i].dcUpdTick > tick ) || 
                (TRUE == cbioRdyChgdGet (dev)))
                {
                continue ;
                }

            /* skip the device if its busy now */
            if( semTake( dev->cbioMutex, NO_WAIT) == ERROR )
                {
                continue ;
                }

            /* calc syncInterval preset */
            dcacheCtrl[i].dcUpdTick = 
                dcacheCtrl[i].dcSyncInterval * sysClkRateGet() + tick ;

            if( dcacheCtrl[i].dcDirtyCount > 0 )
                {
                (void) dcacheManyFlushInval( dev, 0, NONE, TRUE, FALSE,
                        & dev->pDc->dcWritesBackground );
                }
            else if (dev->cbioParams.cbioRemovable &&(dcacheCtrl[i].dcActTick <
                                tick - sysClkRateGet() * DCACHE_IDLE_SECS ))
                {
                /* because the device has been idle for some time, */
                (void) dcacheManyFlushInval( dev, 0, NONE, TRUE, TRUE,
                        & dev->pDc->dcWritesBackground );
                DEBUG_MSG("cache blocks invalidated\n",
                        0,0,0,0,0,0); 
                }
            semGive( dev->cbioMutex );
            } /* for i */
        } /* FOREVER */
    /*UNREACH*/
    }


/*******************************************************************************
*
* dcacheShow - print information about disk cache
*
* This routine displays various information regarding a disk cache, namely
* current disk parameters, cache size, tunable parameters and performance
* statistics. The information is displayed on the standard output.
*
* The <dev> argument is the device handle, if it is NULL,
* all disk caches are displayed.
*
* RETURNS: N/A
*/
void dcacheShow
    (
    CBIO_DEV_ID dev,    /* device handle */
    int verbose         /* 1 - display state of each cache block */
    )
    {
    FAST DCACHE_DESC * pTmp ;
    BOOL all = FALSE ;
    int index = 0 ;
    int count = 0  ;
    char state ;

    if( dev == NULL )
        {
        all = TRUE ;
        }

loop_all:

    if( all )
        {
        if( index >= (int)NELEMENTS( dcacheCtrl ) )
            return ;

        if((dev = dcacheCtrl[index++].cbioDev) == NULL )
            goto loop_all ;
        }

    printf("Disk Cache, %s:\n", dev->pDc->pDcDesc );

    cbioShow(dev);

    if( verbose) 
        {
        printf("Cached block numbers:\n");

        for( pTmp = (DCACHE_DESC *) DLL_FIRST( & dev->pDc->dcLru ) ;
             pTmp != NULL ;
             pTmp = (DCACHE_DESC *) DLL_NEXT( pTmp ) )
            {
            count ++;
            switch( pTmp->state)
                {
                case CB_STATE_EMPTY:/* no valid block is assigned */
                    state = ' ';
                    break;
                case CB_STATE_DIRTY:/* valid block but modified in memory */
                    state = 'D';
                    break;
                case CB_STATE_CLEAN:/* contains a valid block, unmodified */
                    state = 'C';
                    break ;
                default:
                    state = '?';
                }

            if( verbose && state != ' ' )
                printf(" blk %ld %c,", pTmp->block, state );
            if( verbose && ((count % 8) == 7))
                printf("\n");
            } /* for*/

        printf("\n");
        }
    else
        count = dev->pDc->dcNumCacheBlocks ;

    printf("Total cache Blocks %d, %ld blocks (%%%ld) dirty\n",
            count, dev->pDc->dcDirtyCount,
            100 * dev->pDc->dcDirtyCount / count );

    printf("Tunable Params:\n");
    printf("   Bypass Threshold %ld, Max Dirty %ld, "
            "Read Ahead %ld blocks, Sync interval %ld sec\n",
            dev->pDc->dcBypassCount,
            dev->pDc->dcDirtyMax,
            dev->pDc->dcReadAhead,
            dev->pDc->dcSyncInterval
            );

    printf("Hit Stats: Cookie Hits %ld Miss %ld, ",
            dev->pDc->dcCookieHits,
            dev->pDc->dcCookieMisses) ;
    printf("Hash size %ld Hits %ld Miss %ld\n",
            dev->pDc->dcHashSize,
            dev->pDc->dcHashHits,
            dev->pDc->dcHashMisses);
    printf("   LRU Hits %ld, Misses %ld, Hit Ratio %%%ld\n",
        dev->pDc->dcLruHits,
        dev->pDc->dcLruMisses,
        (100 * dev->pDc->dcLruHits) /
                (1 + dev->pDc->dcLruHits + dev->pDc->dcLruMisses)
        ) ;
    printf("Write Sstats: Foreground %ld, Background %ld, "
           "Hidden %ld, Forced %ld\n",
        dev->pDc->dcWritesForeground,
        dev->pDc->dcWritesBackground,
        dev->pDc->dcWritesHidden,
        dev->pDc->dcWritesForced);
    printf("\n");       /* done */


    printf("Subordinate Device Start:\n");
    cbioShow( dev->pDc->dcSubDev);
    printf("Subordinate Device End:\n");

    if( all )
            goto loop_all ;
    }

#ifdef  DEBUG_XXX
/*******************************************************************************
*
* dcacheHashTest - test hash table integrity
*
*/
void dcacheHashTest( CBIO_DEV_ID dev )
    {
    FAST DCACHE_DESC * pTmp ;
    int count = 0 ;

    for( pTmp = (DCACHE_DESC *) DLL_FIRST( & dev->pDc->dcLru ) ;
         pTmp != NULL ;
         pTmp = (DCACHE_DESC *) DLL_NEXT( pTmp ) )
        {
        count ++;
        switch( pTmp->state)
            {
            case CB_STATE_EMPTY:/* no valid block is assigned */
                break;
            case CB_STATE_DIRTY:/* contains a valid but modified in memory */
            case CB_STATE_CLEAN:/* contains a valid block, unmodified */
            default:
                if( pTmp != dcacheBlockLocate( dev, pTmp->block) )
                    INFO_MSG("dcacheHashTest: mismatch\n");
            }

        } /* for*/
    }
#endif  /*DEBUG*/
/* End of File */

