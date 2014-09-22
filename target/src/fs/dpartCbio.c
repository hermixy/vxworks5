/* dpartCbio.c - generic disk partition manager */

/* Copyright 1999-2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01s,14jan02,jkf  SPR#72533,dpartDevCreate failed with BLK_DEV, & doc edits.
01r,12dec01,jkf  fixing diab build warnings.
01q,09dec01,jkf  SPR#71637, fix for SPR#68387 caused ready changed bugs.
01p,09nov01,jkf  SPR#71633, dont set errno when DevCreate is called w/BLK_DEV
01o,20sep01,jkf  SPR#69031, common code for both AE & 5.x.
01n,01aug01,jyo  Fixed SPR#69411: Change in media's readyChanged bit is not
                 being propogated appropriately to the layers above.
01m,14jun01,jyo  SPR#67729: Updating blkSubDev, cbioSubDev and isDriver in
                 dpartDevCreate().
01l,19apr00,dat  doc fixup
01k,29feb00,jkf  T3 changes, cleanup.
01j,15sep99,jkf  changes for new CBIO API.
01i,24aug99,jkf  changed docs
01h,31jul99,jkf  increased max partitions to 24 (c-z) in header, 
                 changed docs, defaulted the debug global to 0. SPR#28277.
01g,31jul99,jkf  T2 merge, tidiness & spelling.
01f,07dec98,lrn  minor fixes for partition table creation
01e,25oct98,lrn  fixed re-reading subDev geometry on RESET
01d,15sep98,lrn  some doc cleanups
01c,10sep98,lrn  added cbioDevVerify for cache-less configs
01b,02jul98,lrn  tested, doc review
01a,15jun98,lrn  written, preliminary
*/

/*
DESCRIPTION

This module implements a generic partition manager using the CBIO
API (see cbioLib)  It supports creating a separate file system 
device for each of its partitions.

This partition manager depends upon an external library to decode a
particular disk partition table format, and report the resulting
partition layout information back to this module.  This module is
responsible for maintaining the partition logic during operation.

When using this module with the dcacheCbio module, it is recommened
this module be the master CBIO device.   This module should be above 
the cache CBIO module layer.   This is because the cache layer is 
optimized to fuction efficently atop a single physical disk drive.
One should call dcacheDevCreate before dpartDevCreate.

An implementation of the de-facto standard partition table format
which is created by the MSDOS FDISK program is provided with the
usrFdiskPartLib module, which should be used to handle PC-style
partitioned hard or removable drives.

EXAMPLE
The following code will initialize a disk which is expected to have up
to 4 partitions:
.CS
  usrPartDiskFsInit( BLK_DEV * blkDevId )
    {
    const char * devNames[] = { "/sd0a", "/sd0b", "/sd0c", "/sd0d" };
    CBIO_DEV_ID cbioCache;
    CBIO_DEV_ID cbioParts;

    /@ create a disk cache atop the entire BLK_DEV @/

    cbioCache = dcacheDevCreate ( blkDevId, NULL, 0, "/sd0" );

    if (NULL == cbioCache)
        {
        return (ERROR);
        }

    /@ create a partition manager with a FDISK style decoder @/

    cbioParts = dpartDevCreate( cbioCache, 4, usrFdiskPartRead );

    if (NULL == cbioParts)
        {
        return (ERROR);
        }

    /@ create file systems atop each partition @/

    dosFsDevCreate( devNames[0], dpartPartGet(cbioParts,0), 0x10, NONE);
    dosFsDevCreate( devNames[1], dpartPartGet(cbioParts,1), 0x10, NONE);
    dosFsDevCreate( devNames[2], dpartPartGet(cbioParts,2), 0x10, NONE);
    dosFsDevCreate( devNames[3], dpartPartGet(cbioParts,3), 0x10, NONE);
    }
.CE

Because this module complies with the CBIO programming interface on both 
its upper and lower layers, it is both an optional and a stackable module.

SEE ALSO:
dcacheLib, dosFsLib, usrFdiskPartLib

INTERNAL
*/

/* includes */
#include "vxWorks.h"
#include "private/dosFsVerP.h"
#include "stdlib.h"
#include "semLib.h"
#include "ioLib.h"
#include "string.h"
#include "stdio.h"
#include "errno.h"
#include "assert.h"


/* START - CBIO private header */
#define	CBIO_DEV_EXTRA	struct dpartCtrl
#include "private/cbioLibP.h"
/* END - CBIO private header */

#include "dpartCbio.h"

/* Implementation dependent data structures */

typedef struct dpartCtrl
    {
    CBIO_DEV_ID	subDev;		/* lower level CBIO device handle */
    int		nPart ;		/* Actual # of partitions on the disk */
    FUNCPTR	pPartDecodeFunc;/* Extern. partition decoder */

    PART_TABLE_ENTRY		/* partition table */
		table [ PART_MAX_ENTRIES ];
    
    CBIO_DEV			/* per-partition virtual devices */
		vDev [ PART_MAX_ENTRIES ];
    CBIO_DEV_ID	masterDev;	/* master handle CBIO */
    } DPART_CTRL ;

/* custom use of CBIO fields */

#define	cbioPartIndx	cbioPriv0

extern void logMsg( const char *fmt, ... );

int dpartDebug = 0;
int dpartSemOptions = (SEM_Q_PRIORITY | SEM_INVERSION_SAFE | SEM_DELETE_SAFE) ;

#define	INFO_MSG	logMsg
#define	DEBUG_MSG	if(dpartDebug) logMsg


/* declarations */

LOCAL STATUS dpartBlkRW
    (
    CBIO_DEV_ID		dev,
    block_t		startBlock,
    block_t		numBlocks,
    addr_t		buffer,
    CBIO_RW		rw,
    cookie_t		*pCookie
    );

LOCAL STATUS dpartBytesRW
    (
    CBIO_DEV_ID 	dev,
    block_t		startBlock,
    off_t		offset,
    addr_t		buffer,
    size_t		nBytes,
    CBIO_RW		rw,
    cookie_t		*pCookie
    );

LOCAL STATUS dpartBlkCopy
    (
    CBIO_DEV_ID 	dev,
    block_t		srcBlock,
    block_t		dstBlock,
    block_t		numBlocks
    );

LOCAL STATUS dpartIoctl
    (
    CBIO_DEV_ID	dev,
    UINT32 command,
    addr_t	arg
    );


/* CBIO_FUNCS, one per cbio driver */

LOCAL CBIO_FUNCS cbioFuncs = {(FUNCPTR) dpartBlkRW,
			      (FUNCPTR) dpartBytesRW,
			      (FUNCPTR) dpartBlkCopy,
			      (FUNCPTR) dpartIoctl};

/*******************************************************************************
*
* dpartPartTableFill - Fill in partition geometry
*
* Call external function to decode partitions, and fill the partition
* geometry table accordingly.
*
* RETURNS: OK or ERROR if the partition decode function failed for any reason
*/
LOCAL STATUS dpartPartTableFill( DPART_CTRL * pDc )
    {
    STATUS ret;
    int pn ;
    CBIO_DEV_ID  masterDev, subDev ;

    masterDev = pDc->masterDev;
    subDev = pDc->subDev;

    if((OK != cbioDevVerify( masterDev )) ||
       (OK != cbioDevVerify( subDev )))
	{
	return ERROR;
	}

    bzero( (char *) &pDc->table, sizeof( pDc->table ) );

    /* First, call out the external decode function */
    ret = pDc->pPartDecodeFunc( pDc->subDev, &pDc->table, pDc->nPart ) ;

    if( ret == ERROR )
	return ERROR ;

    /*
     * re-configure device geometry in case it has been replaced
     */

    masterDev->cbioParams.nBlocks	= subDev->cbioParams.nBlocks ;
    masterDev->cbioParams.bytesPerBlk	= subDev->cbioParams.bytesPerBlk ;
    masterDev->cbioParams.blocksPerTrack= subDev->cbioParams.blocksPerTrack ;
    masterDev->cbioParams.nHeads	= subDev->cbioParams.nHeads ;
    masterDev->cbioMode			= subDev->cbioMode ;

    /*
     * check sanity of the resulting partition table:
     */

    for( pn = 0; pn < pDc->nPart; pn ++ )
	{
	if( pDc->table[pn].spare != 0 || 
	    (pDc->table[pn].offset + pDc->table[pn].nBlocks) >
		pDc->subDev->cbioParams.nBlocks )
		{
		DEBUG_MSG("dpartCbio: error: "
			"partition spills over disk size: %d blocks\n",
			pDc->table[pn].offset + pDc->table[pn].nBlocks -
			pDc->subDev->cbioParams.nBlocks );
		return ERROR;
		}
	}
    DEBUG_MSG("dpartCbio: partition table decoded:\n");

    for( pn = 0; pn < pDc->nPart; pn ++ )
	{
	DEBUG_MSG("  part %d: offset %d nBlocks %d (next free block %d) \n",
		pn, pDc->table[pn].offset, pDc->table[pn].nBlocks,
		pDc->table[pn].offset + pDc->table[pn].nBlocks);

	pDc->vDev[ pn ].cbioParams.nBlocks = pDc->table[pn].nBlocks ;
	pDc->vDev[ pn ].cbioParams.blockOffset  = pDc->table[pn].offset ;

    	pDc->vDev[ pn ].cbioParams.bytesPerBlk	= 
				subDev->cbioParams.bytesPerBlk ;
    	pDc->vDev[ pn ].cbioParams.blocksPerTrack = 
				subDev->cbioParams.blocksPerTrack ;
    	pDc->vDev[ pn ].cbioParams.nHeads	= 
				subDev->cbioParams.nHeads ;
    	pDc->vDev[ pn ].cbioMode		= subDev->cbioMode ;

	CBIO_READYCHANGED (&(pDc->vDev[ pn ])) = TRUE;
	}

    DEBUG_MSG("dpartCbio: end of partitions, total device blocks %d\n",
		pDc->subDev->cbioParams.nBlocks );

    return OK ;
    }

/*******************************************************************************
*
* dpartBlkRW - Read/Write blocks
*
* This routine transfers between a user buffer and the lower layer CBIO
* It is optimized for block transfers.  
*
* RETURNS OK or ERROR and may otherwise set errno.
*/
LOCAL STATUS dpartBlkRW
    (
    CBIO_DEV_ID		dev,		/* CBIO handle */
    block_t		startBlock,	/* starting block */
    block_t		numBlocks,	/* nbr of blocks */
    addr_t		buffer,		/* data buffer */
    CBIO_RW		rw,		/* data direction */
    cookie_t		*pCookie	/* passed thru */
    )
    {
    CBIO_DEV *	pDev = (void *) dev ;
    STATUS retStat ;

    if(TRUE == cbioRdyChgdGet (dev))
	{
	errno = S_ioLib_DISK_NOT_PRESENT ;
	return ERROR;
	}

    if( (startBlock) > pDev->cbioParams.nBlocks ||
    	(startBlock+numBlocks) > pDev->cbioParams.nBlocks )
	return ERROR;

    startBlock += pDev->cbioParams.blockOffset;

    if( semTake( pDev->cbioMutex, WAIT_FOREVER) == ERROR )
	return ERROR;

    retStat = pDev->pDc->subDev->pFuncs->cbioDevBlkRW(
		pDev->pDc->subDev,
		startBlock,
		numBlocks,
		buffer,
		rw,
		pCookie );

    if( retStat == ERROR )
	if( TRUE == cbioRdyChgdGet (dev->pDc->subDev))
	    CBIO_READYCHANGED (dev) = TRUE;

    semGive( pDev->cbioMutex );

    return (retStat);

    }
/*******************************************************************************
*
* dpartBytesRW - Read/Write bytes
*
* This routine transfers between a user buffer and the lower layer CBIO
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
LOCAL STATUS dpartBytesRW
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
    STATUS retStat ;

    if(TRUE == cbioRdyChgdGet (dev))
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


    /* apply offset */
    startBlock += pDev->cbioParams.blockOffset;

    if( semTake( pDev->cbioMutex, WAIT_FOREVER) == ERROR )
	return ERROR;

    retStat = pDev->pDc->subDev->pFuncs->cbioDevBytesRW(
		pDev->pDc->subDev,
		startBlock,
		offset,
		buffer,
		nBytes,
		rw,
		pCookie );

    if( retStat == ERROR )
	if( TRUE == cbioRdyChgdGet (dev->pDc->subDev))
	    CBIO_READYCHANGED (dev) = TRUE;

    semGive( pDev->cbioMutex );

    return ( retStat );
    }

/*******************************************************************************
*
* dpartBlkCopy - Copy sectors 
*
* This routine makes copies of one or more blocks on the lower layer CBIO.
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
LOCAL STATUS dpartBlkCopy
    (
    CBIO_DEV_ID 	dev,
    block_t		srcBlock,
    block_t		dstBlock,
    block_t		numBlocks
    )
    {
    CBIO_DEV	*pDev = (void *) dev ;
    STATUS retStat ;

    if(TRUE == cbioRdyChgdGet (dev))
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

    if( semTake( pDev->cbioMutex, WAIT_FOREVER) == ERROR )
	return ERROR;

    retStat = pDev->pDc->subDev->pFuncs->cbioDevBlkCopy(
		pDev->pDc->subDev,
		srcBlock,
		dstBlock,
		numBlocks
		);

    if( retStat == ERROR )
	if( TRUE == cbioRdyChgdGet (dev->pDc->subDev))
	    CBIO_READYCHANGED (dev) = TRUE;

    semGive( pDev->cbioMutex );

    return ( retStat);
    }

/*******************************************************************************
*
* dpartIoctl - Misc control operations 
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
* 
* command - ioctl() command being issued
* 
* arg - specific to the particular ioctl() function requested or un-used.
*
* RETURNS OK or ERROR and may otherwise set errno.
*/
LOCAL STATUS dpartIoctl
    (
    CBIO_DEV_ID	dev,
    UINT32	command,
    addr_t	arg
    )
    {
    STATUS retStat = ERROR ;

    if(OK != cbioDevVerify (dev))
	{
	INFO_MSG("dpartIoctl: invalid handle\n");
	return ERROR;
	}

    if( semTake( dev->cbioMutex, WAIT_FOREVER) == ERROR )
	return ERROR;

    switch ( command )
	{
	case CBIO_RESET :
		/*
		 * if this is the first partition to issue RESET
		 * we need to RESET the lower level device too,
		 * otherwise, RESET only this partition.
		 */
		if((TRUE == cbioRdyChgdGet (dev->pDc->subDev)) ||
		   (TRUE == cbioRdyChgdGet (dev->pDc->masterDev)))
		    {
		    DEBUG_MSG("dpartCbio: resetting subordinate\n");
		    retStat = dev->pDc->subDev->pFuncs->cbioDevIoctl(
				dev->pDc->subDev,
				command,
				arg );

		    /* and also fill  the partitions geometry now */
		    if( retStat == OK )
			{
			retStat = dpartPartTableFill( dev->pDc );
			}

		    if( retStat == OK )
			{
			CBIO_READYCHANGED (dev->pDc->masterDev) = FALSE;
			}
		    }
		else
		    {
		    /*
		     * if masterDev does not have readyChanged
		     * there is no need to reset subDev, its been
		     * already reset
		     */
		    retStat = OK;
		    }

		/* non-existent partitions will remain "removed" */
		if (retStat == OK && dev->cbioParams.nBlocks == 0)
		    {
		    errno = S_ioLib_DISK_NOT_PRESENT ;
		    retStat = ERROR ;
		    }

		if( retStat == OK )
		    CBIO_READYCHANGED(dev) = FALSE;
		break;

	/* NEWBLK is the only ioctl where block# is coded in arg */

	case CBIO_CACHE_NEWBLK:
	    arg += dev->cbioParams.blockOffset ;

	    /*FALLTHROUGH*/

	/* all other commands should be executed by the lower layer */
	case CBIO_STATUS_CHK : 
	case CBIO_DEVICE_LOCK :
	case CBIO_DEVICE_UNLOCK :
	case CBIO_DEVICE_EJECT :

	    /* check if master has readyChanged */

	    if((TRUE == cbioRdyChgdGet (dev->pDc->subDev)) ||
	       (TRUE == cbioRdyChgdGet (dev->pDc->masterDev)))
		{
		CBIO_READYCHANGED (dev) = TRUE;
		}

	    /*FALLTHROUGH*/

	case CBIO_CACHE_FLUSH :
	case CBIO_CACHE_INVAL :
	default:
	    if(TRUE == cbioRdyChgdGet (dev))
		{
		errno = S_ioLib_DISK_NOT_PRESENT ;
		retStat = ERROR ;
		}
	    else
		{
		if( command == (int)CBIO_DEVICE_LOCK )
		    semTake( dev->pDc->subDev->cbioMutex, WAIT_FOREVER) ;

		retStat = dev->pDc->subDev->pFuncs->cbioDevIoctl(
			    dev->pDc->subDev,
			    command,
			    arg );

		if( command == (int)CBIO_DEVICE_UNLOCK )
		    semGive( dev->pDc->subDev->cbioMutex) ;
		}
	} /* end of switch */

    if( retStat == ERROR )
	if( TRUE == cbioRdyChgdGet (dev->pDc->subDev))
	    CBIO_READYCHANGED (dev) = TRUE;

    semGive( dev->cbioMutex );

    return( retStat );
    }

/*******************************************************************************
*
* dpartDevCreate - Initialize a partitioned disk
*
* To handle a partitioned disk, this function should be called,
* with <subDev> as the handle returned from dcacheDevCreate(),
* It is recommended that for efficient operation a single disk cache
* be allocated for the entire disk and shared by its partitions.
*
* <nPart> is the maximum number of partitions which are expected
* for the particular disk drive. Up to 24 (C-Z) partitions per disk 
* are supported.
*
* PARTITION DECODE FUNCTION
* An external partition table decode function is provided via the
* <pPartDecodeFunc> argument, which implements a particular style and
* format of partition tables, and fill in the results into a table
* defined as Pn array of PART_TABLE_ENTRY types. See dpartCbio.h
* for definition of PART_TABLE_ENTRY.
* The prototype for this function is as follows:
* 
* .CS
*     STATUS parDecodeFunc
* 	(
* 	CBIO_DEV_ID dev,	/@ device from which to read blocks @/
* 	PART_TABLE_ENTRY *pPartTab, /@ table where to fill results @/
* 	int nPart		/@ # of entries in <pPartTable> @/
* 	)
* .CE
*
* RETURNS: CBIO_DEV_ID or NULL if error creating CBIO device.
* 
* SEE ALSO: dosFsDevCreate().
* INTERNAL
* during create, readyChanged bit is TRUE, so no accesses are allowed
* until after a CBIO_RESET, at which time the actual partition table
* will be brought in and applied.
*/

CBIO_DEV_ID dpartDevCreate
    (
    CBIO_DEV_ID	subDev,		/* lower level CBIO device */
    int		nPart,		/* # of partitions */
    FUNCPTR	pPartDecodeFunc	/* function to decode partition table */
    )
    {
    CBIO_DEV_ID	cbio = subDev ;
    CBIO_DEV_ID	pDev = NULL ;
    DPART_CTRL * pDc ;
    int p ;

    if( nPart > PART_MAX_ENTRIES || pPartDecodeFunc == NULL )
	{
	printErr ("%d is too many partitions, max limit %d\n",
		  nPart, PART_MAX_ENTRIES);
	errno = EINVAL;
	return NULL;
	}

    cbioLibInit();	/* just in case */

    /* if this is a BLK_DEV, create a CBIO_DEV */

    if (OK != cbioDevVerify( subDev ))
        {
        /* attempt to handle BLK_DEV subDev */
        cbio = cbioWrapBlkDev ((BLK_DEV *) subDev);

        if( NULL != cbio )
            {
            /* SPR#71633, clear the errno set in cbioDevVerify() */
            errno = 0;
            }
        }
    else
	{
	cbio = subDev;
	}

    if( NULL == cbio )
	{
	DEBUG_MSG("dpartDevCreate: invalid handle\n");
	return NULL;
	}


    /* allocate our main DPART_CTRL structure */

    pDc = KHEAP_ALLOC((sizeof(DPART_CTRL)));

    if( pDc == NULL )
	{
	return( NULL );
	}

    bzero( (char *) pDc, sizeof( DPART_CTRL) );

    /* create the primary CBIO handle */

    pDev = cbioDevCreate ( NULL, 0 );

    if( pDev == NULL )
	{
	return( NULL );
	}

    /* the primary CBIO handle is not for any actual I/O, just for show */

    pDev->cbioDesc	= "dpartCbio Partition Manager" ;
    CBIO_READYCHANGED (pDev)		= CBIO_READYCHANGED (cbio);
    pDev->cbioParams.cbioRemovable	= cbio->cbioParams.cbioRemovable ;
    pDev->cbioParams.nBlocks		= cbio->cbioParams.nBlocks ;
    pDev->cbioParams.bytesPerBlk	= cbio->cbioParams.bytesPerBlk ;
    pDev->cbioParams.blocksPerTrack	= cbio->cbioParams.blocksPerTrack ;
    pDev->cbioParams.nHeads		= cbio->cbioParams.nHeads ;
    pDev->cbioMode			= cbio->cbioMode ;
    pDev->cbioParams.blockOffset	= 0 ;
    pDev->cbioParams.lastErrBlk		= NONE ;
    pDev->cbioParams.lastErrno		= 0 ;
    pDev->cbioPartIndx			= NONE ; /* master device */

    /* SPR#67729: Fill in the members subDev and isDriver appropriately */

    pDev->blkSubDev = NULL;    /* Since lower layer is not BLKDEV  */
    pDev->cbioSubDev = cbio;    /* Storing the reference to the sub Device  */
    pDev->isDriver = FALSE;   /* == FALSE since this is a CBIO TO CBIO layer */
    pDev->pDc				= pDc ;  /* DPART_CTRL */
    pDev->pFuncs 			= &cbioFuncs;

    /* setup main fields */

    pDc->subDev				= cbio ;
    pDc->pPartDecodeFunc		= pPartDecodeFunc ;
    pDc->nPart				= nPart ;
    pDc->masterDev			= pDev ;

    /* Now initialize the partition virtual devs */

    for(p = 0; p < PART_MAX_ENTRIES; p++ )
	{
	CBIO_DEV_ID tmpDev = & ( pDc->vDev[ p ] );

	/* inherit most fields from the primary handle */

	*tmpDev = *pDev ;

	tmpDev->cbioDesc        = "dpartCbio Partition Slave Device";
	tmpDev->cbioPartIndx    = p;   /* partition # */

	CBIO_READYCHANGED (tmpDev) = TRUE;

	/* semaphore can be just copied, it has to be init'ed */

        tmpDev->cbioMutex = semMCreate (dpartSemOptions);

        if (NULL == tmpDev->cbioMutex)
	    {
	    DEBUG_MSG("Error Creating the device semaphore\n");
	    pDev = NULL;
	    break;
	    }

#ifdef _WRS_DOSFS2_VXWORKS_AE	
	/* init the handle */
	handleInit (&tmpDev->cbioHandle, handleTypeCbioHdl);
#endif  /* _WRS_DOSFS2_VXWORKS_AE */

	/* these will be filled later from actual geometry */
	tmpDev->cbioParams.nBlocks	= 0 ;
	tmpDev->cbioParams.blockOffset	= 0 ;
	}

    DEBUG_MSG("dpartDevCreate: handle dev %x subDev %x\n", pDev, cbio );

    /* return device handle */
    return( pDev );
    }

/*******************************************************************************
*
* dpartPartGet - retrieve handle for a partition
*
* This function retrieves a CBIO handle into a particular partition
* of a partitioned device. This handle is intended to be used with
* dosFsDevCreate().
*
* RETURNS: CBIO_DEV_ID or NULL if partition is out of range, or
* <masterHandle> is invalid.
*
* SEE ALSO: dosFsDevCreate()
*/
CBIO_DEV_ID dpartPartGet
    (
    CBIO_DEV_ID masterHandle,	/* CBIO handle of the master partition */
    int partNum			/* partition number from 0 to nPart */
    )
    {
    if(OK != cbioDevVerify( masterHandle))
	{
	return NULL;
	}

    if( partNum >= masterHandle->pDc->nPart )
	{
	errno = EINVAL;
	return NULL;
	}

    return ( & masterHandle->pDc->vDev[ partNum ] );
    }

/*******************************************************************************
*
* dpartShow - show current parameters of the RAM disk device
*
* NOMANUAL
*/
STATUS dpartShow( CBIO_DEV_ID dev, int verb )
    {
    DPART_CTRL * pDc ;
    int pn ;

    if( (OK != cbioDevVerify (dev)))
	{
	DEBUG_MSG("dpartShow: invalid handle\n");
	return ERROR;
	}

    pDc = dev->pDc ;

    if( TRUE == cbioRdyChgdGet (dev))
	dev->pFuncs->cbioDevIoctl( dev, CBIO_RESET, 0);

    if( dev->cbioPartIndx == (u_long)NONE )
	printf("  Device %#lx is Master handle, Partition Table:\n",
		(u_long) dev);
    else
	printf("  Device %#lx is partition %ld, Partition Table:\n",
		(u_long) dev, dev->cbioPartIndx );

    for( pn = 0; pn < pDc->nPart ; pn ++ )
	{
	printf("  #%ld: from %ld to %ld (%ld blocks)\n",
		pDc->vDev[ pn ].cbioPartIndx,
		(block_t) pDc->vDev[ pn ].cbioParams.blockOffset,
		pDc->vDev[ pn ].cbioParams.nBlocks + 
			pDc->vDev[ pn ].cbioParams.blockOffset,
		pDc->vDev[ pn ].cbioParams.nBlocks );
	}

    printf("  master dev %#lx, subordinate dev %#lx, with total %ld blocks\n",
	(u_long) pDc->masterDev, (u_long) pDc->subDev,
	pDc->subDev->cbioParams.nBlocks );

    return OK;
    }
/* End of File */
