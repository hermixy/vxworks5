/* ramDiskCbio.c - RAM Disk Cached Block Driver */

/* Copyright 1998-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01j,12dec01,jkf  fixing diab build warnings
01i,09dec01,jkf  SPR#71637, fix for SPR#68387 caused ready changed bugs.
01h,30jul01,jkf  SPR#69031, common code for both AE & 5.x.
01g,14jun01,jyo  SPR#67729: Updating blkSubDev, cbioSubDev, isDriver in
                 ramDiskDevCreate().
01f,19apr00,dat  doc fixes
01e,29feb00,jkf  T3 changes
01d,31aug99,jkf  CBIO API changes
01c,31jul99,jkf  T2 merge, tidiness & spelling.
01b,07jun98,lrn  re-integration, doc
01a,15jan98,lrn  written, preliminary
*/

/*
DESCRIPTION
This module implements a RAM-disk driver with a CBIO interface
which can be directly utilized by dosFsLib without the use of 
the Disk Cache module dcacheCbio.  This results in an
ultra-compact RAM footprint.  This module is implemented 
using the CBIO API (see cbioLib())

This module is delivered in source as a functional example of 
a basic CBIO module.

CAVEAT
This module may be used for SRAM or other non-volatile RAM cards to 
store a file system, but that configuration will be susceptible
to data corruption in events of system failure which are not normally
observed with magnetic disks, i.e. using this driver with an SRAM card
can not guard against interruptions in midst of updating a particular
sector, resulting in that sector become internally inconsistent.

SEE ALSO
dosFsLib, cbioLib
*/

/* includes */
#include "vxWorks.h"
#include "stdlib.h"
#include "semLib.h"
#include "ioLib.h"
#include "string.h"
#include "errno.h"
#include "assert.h"

#include "private/dosFsVerP.h"
#include "private/cbioLibP.h" /* only CBIO modules may include this file */

#include "ramDiskCbio.h"

/* Implementation dependent fields */
#define	cbioBlkShift	cbioPriv0

extern void logMsg( const char *fmt, ... );

LOCAL STATUS ramDiskBlkRW
    (
    CBIO_DEV_ID		dev,
    block_t		startBlock,
    block_t		numBlocks,
    addr_t		buffer,
    CBIO_RW		rw,
    cookie_t		*pCookie
    );

LOCAL STATUS ramDiskBytesRW
    (
    CBIO_DEV_ID 	dev,
    block_t		startBlock,
    off_t		offset,
    addr_t		buffer,
    size_t		nBytes,
    CBIO_RW		rw,
    cookie_t		*pCookie
    );

LOCAL STATUS ramDiskBlkCopy
    (
    CBIO_DEV_ID 	dev,
    block_t		srcBlock,
    block_t		dstBlock,
    block_t		numBlocks
    );

LOCAL STATUS ramDiskIoctl
    (
    CBIO_DEV_ID	dev,
    UINT32	command,
    addr_t	arg
    );


/* CBIO_FUNCS, one per cbio driver */

LOCAL CBIO_FUNCS cbioFuncs = {(FUNCPTR) ramDiskBlkRW,
			      (FUNCPTR) ramDiskBytesRW,
			      (FUNCPTR) ramDiskBlkCopy,
			      (FUNCPTR) ramDiskIoctl};


int ramDiskDebug = 0;

#define	INFO_MSG	logMsg
#define	DEBUG_MSG	if(ramDiskDebug) logMsg


/*******************************************************************************
*
* ramDiskBlkRW - Read/Write blocks
* 
* This routine transfers between a user buffer and the lower layer hardware
* It is optimized for block transfers.  
*
* dev - the CBIO handle of the device being accessed (from creation routine)
* 
* startBlock - the starting block of the transfer operation
* 
* numBlocks - the total number of blocks to transfer
* 
* buffer - address of the memory buffer used for the transfer
* 
* rw - indicates the direction of transfer up or down (READ/WRITE)
* 
* *pCookie - pointer to cookie used by upper layer such as dosFsLib(),
* it should be preserved.
*
* RETURNS OK or ERROR and may otherwise set errno.
*
*/
LOCAL STATUS ramDiskBlkRW
    (
    CBIO_DEV_ID		dev,
    block_t		startBlock,
    block_t		numBlocks,
    addr_t		buffer,
    CBIO_RW		rw,
    cookie_t		*pCookie
    )
    {
    CBIO_DEV *	pDev = (void *) dev ;
    caddr_t 	pStart;
    size_t 	nBytes ;

    if( TRUE == CBIO_READYCHANGED (dev) )
	{
	errno = S_ioLib_DISK_NOT_PRESENT ;
	return ERROR;
	}

    if( (startBlock) > pDev->cbioParams.nBlocks ||
    	(startBlock+numBlocks) > pDev->cbioParams.nBlocks )
	return ERROR;

    startBlock += pDev->cbioParams.blockOffset;
    pStart = pDev->cbioMemBase + ( startBlock << pDev->cbioBlkShift ) ;
    nBytes = numBlocks << pDev->cbioBlkShift ;

    DEBUG_MSG("ramDiskBlkRW: blk %d # %d -> addr %x, %x bytes\n",
	startBlock, numBlocks, pStart, nBytes );

    if( semTake( pDev->cbioMutex, WAIT_FOREVER) == ERROR )
	return ERROR;

    switch( rw )
	{
	case CBIO_READ:
	    bcopy( pStart, buffer, nBytes );
	    break;
	case CBIO_WRITE:
	    bcopy( buffer, pStart,  nBytes );
	    break;
	}

    semGive(pDev->cbioMutex);

    return OK;
    }

/*******************************************************************************
*
* ramDiskBytesRW - Read/Write bytes
*
* This routine transfers between a user buffer and the lower layer hardware
* It is optimized for byte transfers.  
*
* dev - the CBIO handle of the device being accessed (from creation routine)
* 
* startBlock - the starting block of the transfer operation
* 
* offset - offset in bytes from the beginning of the starting block
* 
* buffer - address of the memory buffer used for the transfer
* 
* nBytes - number of bytes to transfer
* 
* rw - indicates the direction of transfer up or down (READ/WRITE)
* 
* *pCookie - pointer to cookie used by upper layer such as dosFsLib(),
* it should be preserved.
* 
* RETURNS OK or ERROR and may otherwise set errno.
*/
LOCAL STATUS ramDiskBytesRW
    (
    CBIO_DEV_ID 	dev,
    block_t		startBlock,
    off_t		offset,
    addr_t		buffer,
    size_t		nBytes,
    CBIO_RW		rw,
    cookie_t		*pCookie
    )
    {
    CBIO_DEV * 	pDev = dev ;
    caddr_t 	pStart;

    if( TRUE == CBIO_READYCHANGED (dev) )
	{
	errno = S_ioLib_DISK_NOT_PRESENT ;
	return ERROR;
	}

    if( startBlock >= pDev->cbioParams.nBlocks )
	return ERROR;

    /* verify that all bytes are within one block range */
    if (((offset + nBytes) > pDev->cbioParams.bytesPerBlk ) ||
	(offset <0) || (nBytes <=0))
	return ERROR;


    /* calculate actual memory address of data */
    startBlock += pDev->cbioParams.blockOffset;
    pStart = pDev->cbioMemBase + ( startBlock << pDev->cbioBlkShift ) ;
    pStart += offset ;

    DEBUG_MSG("ramDiskBytesRW: blk %d + %d # %d bytes -> addr %x, %x bytes\n",
	startBlock, offset, nBytes, pStart, nBytes );

    if( semTake( pDev->cbioMutex, WAIT_FOREVER) == ERROR )
	return ERROR;

    switch( rw )
	{
	case CBIO_READ:
	    bcopy( pStart, buffer, nBytes );
	    break;
	case CBIO_WRITE:
	    bcopy( buffer, pStart,  nBytes );
	    break;
	}

    semGive(pDev->cbioMutex);

    return OK;
    }

/*******************************************************************************
*
* ramDiskBlkCopy - Copy sectors 
* 
* This routine makes copies of one or more blocks on the lower layer hardware.
* It is optimized for block copies on the subordinate layer.  
* 
* dev - the CBIO handle of the device being accessed (from creation routine)
* 
* srcBlock - source start block of the copy
* 
* dstBlock - destination start block of the copy
* 
* num_block - number of blocks to copy
*
* RETURNS OK or ERROR and may otherwise set errno.
*/
LOCAL STATUS ramDiskBlkCopy
    (
    CBIO_DEV_ID 	dev,
    block_t		srcBlock,
    block_t		dstBlock,
    block_t		numBlocks
    )
    {
    CBIO_DEV	*pDev = (void *) dev ;
    caddr_t pSrc, pDst;
    size_t nBytes ;

    if( TRUE == CBIO_READYCHANGED (dev) )
	{
	errno = S_ioLib_DISK_NOT_PRESENT ;
	return ERROR;
	}

    if( (srcBlock) > pDev->cbioParams.nBlocks ||
        (dstBlock) > pDev->cbioParams.nBlocks )
	return ERROR;

    if( (srcBlock+numBlocks) > pDev->cbioParams.nBlocks ||
        (dstBlock+numBlocks) > pDev->cbioParams.nBlocks )
	return ERROR;

    srcBlock += pDev->cbioParams.blockOffset;
    dstBlock += pDev->cbioParams.blockOffset;
    pSrc = pDev->cbioMemBase + ( srcBlock << pDev->cbioBlkShift ) ;
    pDst = pDev->cbioMemBase + ( dstBlock << pDev->cbioBlkShift ) ;
    nBytes = numBlocks << pDev->cbioBlkShift ;

    DEBUG_MSG("ramDiskBlkCopy: blk %d to %d # %d -> addr %x,to %x %x bytes\n",
	srcBlock, dstBlock, numBlocks, pSrc, pDst, nBytes );

    if( semTake( pDev->cbioMutex, WAIT_FOREVER) == ERROR )
	return ERROR;

    /* Do the actual copying */
    bcopy( pSrc, pDst, nBytes );

    semGive(pDev->cbioMutex);

    return OK;
    }

/*******************************************************************************
*
* ramDiskIoctl - Misc control operations 
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
* dev - the CBIO handle of the device being accessed (from creation routine)
* 
* command - ioctl() command being issued
* 
* arg - specific to the particular ioctl() function requested or un-used.
*
* RETURNS OK or ERROR and may otherwise set errno.
*/
LOCAL STATUS ramDiskIoctl
    (
    CBIO_DEV_ID	dev,
    UINT32	command,
    addr_t	arg
    )
    {
    switch ( command )
	{
	case CBIO_RESET :
		/* HELP is this really okay for a driver to do? */
		CBIO_READYCHANGED (dev) = FALSE; 
		return (OK);
	case CBIO_STATUS_CHK : /* These are no-ops for the RAM Disk : */
	case CBIO_DEVICE_LOCK :
	case CBIO_DEVICE_UNLOCK :
	case CBIO_DEVICE_EJECT :
	case CBIO_CACHE_FLUSH :
	case CBIO_CACHE_INVAL :
	    if( TRUE == CBIO_READYCHANGED (dev) )
		{
		errno = S_ioLib_DISK_NOT_PRESENT ;
		return ERROR;
		}
	    return OK;

	case CBIO_CACHE_NEWBLK:
	    if( TRUE == CBIO_READYCHANGED (dev) )
		{
		errno = S_ioLib_DISK_NOT_PRESENT ;
		return ERROR;
		}
	    else
		{
		/* zero out the new block */
		caddr_t pBlk ;
		block_t blk = (long) arg ;
		int nBytes = 1 << dev->cbioBlkShift ;
		if( blk >= dev->cbioParams.nBlocks )
		    return ERROR;
		blk += dev->cbioParams.blockOffset ;
		pBlk = dev->cbioMemBase + ( blk << dev->cbioBlkShift ) ;
		bzero( pBlk, nBytes );
		return OK;
		}

	default:
	    errno = EINVAL;
	    return (ERROR);
	}
    }


/*******************************************************************************
*
* shiftCalc - calculate how many shift bits
*
* How many shifts <n< are needed such that <mask> == 1 << <N>
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
* ramDiskDevCreate - Initialize a RAM Disk device
*
* This function creates a compact RAM-Disk device that can be directly
* utilized by dosFsLib, without the intermediate disk cache.
* It can be used for non-volatile RAM as well as volatile RAM disks.
*
* The RAM size is specified in terms of total number of blocks in the
* device and the block size in bytes. The minimal block size is 32 bytes.
* If <pRamAddr> is NULL, space will be allocated from the default memory
* pool.
*
* RETURNS: a CBIO handle that can be directly used by dosFsDevCreate()
* or NULL if the requested amount of RAM is not available.
*
* CAVEAT: When used with NV-RAM, this module can not eliminate mid-block
* write interruption, which may cause file system corruption not
* existent in common disk drives.
*
* SEE ALSO: dosFsDevCreate().
*/

CBIO_DEV_ID ramDiskDevCreate
    (
    char  *pRamAddr,     /* where it is in memory (0 = malloc)     */
    int   bytesPerBlk,   /* number of bytes per block              */
    int   blksPerTrack,  /* number of blocks per track             */
    int   nBlocks,       /* number of blocks on this device        */
    int   blkOffset      /* no. of blks to skip at start of device */
    )
    {
    CBIO_DEV_ID pDev = NULL ;
    size_t	diskSize;
    char	shift;

    /* Fill in defaults for args with 0 value */
    if( bytesPerBlk == 0)
	bytesPerBlk = 64 ;

    if( nBlocks == 0)
	nBlocks = 2048 ;

    if( blksPerTrack == 0)
	blksPerTrack = 16 ;

    /* first check that bytesPerBlk is 2^n */
    shift = shiftCalc( bytesPerBlk );

    if( bytesPerBlk != ( 1 << shift ) )
	{
	errno = EINVAL;
	return( NULL );
	}

    /* calculate memory pool size in bytes */
    diskSize = nBlocks << shift ;

    cbioLibInit();	/* just in case */

    /* Create CBIO device - generic (cbioDevCreate, cbioLib.c) */
    pDev = (CBIO_DEV_ID) cbioDevCreate ( pRamAddr, diskSize );

    if( pDev == NULL )
	return( NULL );

    /* If we got here, then generic device is allocated */
    pDev->cbioDesc	= "RAM Disk Driver";
    pDev->cbioParams.cbioRemovable	= FALSE ;
    CBIO_READYCHANGED (pDev)     	= FALSE ;
    pDev->cbioParams.nBlocks		= nBlocks ;
    pDev->cbioParams.bytesPerBlk	= bytesPerBlk ;
    pDev->cbioParams.blocksPerTrack	= blksPerTrack ;
    pDev->cbioParams.nHeads		= 1 ;
    pDev->cbioParams.blockOffset		= blkOffset ;
    pDev->cbioMode		= O_RDWR ;
    pDev->cbioParams.lastErrBlk	= NONE ;
    pDev->cbioParams.lastErrno	= 0 ;

    pDev->cbioBlkShift		= shift ;

    /* cbioFuncs is staticly allocated in each driver. */

    pDev->pFuncs = &cbioFuncs;

    /* 
     * SPR#67729: Fill in the members blksubDev, cbioSubDev and isDriver 
     * appropriately 
     */

    pDev->blkSubDev = NULL;  /* This layer does not have a BLKDEV below it */
    pDev->cbioSubDev = pDev; /* Stores a pointer to itself   */
                             /* since this is the last layer */
    pDev->isDriver = TRUE;   /* For ramDiskCbio we are the lowest layer */

    DEBUG_MSG("ramDiskDevCreate: dev %x, size %x\n", pDev, diskSize );

    /* return device handle */
    return( pDev );
    }

/*******************************************************************************
*
* ramDiskShow - show current parameters of the RAM disk device
*
* This function is not implemented.
* 
* NOMANUAL
*/
STATUS ramDiskShow( CBIO_DEV_ID dev, int verb )
    {
    return OK;
    }


