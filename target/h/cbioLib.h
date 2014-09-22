/* cbioLib.h - cached block I/O device (CBIO) header file */

/* Copyright 1999-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01j,21aug01,jkf  SPR#69031, common code for both AE & 5.x.
01i,14apr00,jkf  fixed S_cbioLib_INVALID_CBIO_DEV_ID definition
01h,29feb00,jkf  T3 changes
01g,31aug99,jkf  overhaul for new CBIO API.
01f,31jul99,jkf  T2 merge, tidiness & spelling.
01e,14oct98,lrn  added includes for required types
01d,07sep98,lrn  added prototype for cbioDevVerify 
01c,02jul98,lrn  ready for prerelease
01b,27apr98,lrn  added implementation dependent structure pointer, stats 
01a,15jan98,lrn  written
*/

#ifndef __INCcbioLibh
#define __INCcbioLibh

#ifdef __cplusplus
extern "C" {
#endif

/* includes */
#include "blkIo.h" 			/* for BLK_DEV type */

/* 
 * typedef's and enums 
 */

typedef	unsigned long block_t ;		/* block number */
typedef	unsigned long cookie_t ;	/* opaque cookie */
typedef struct cbioDev * CBIO_DEV_ID ;  /* CBIO device handle */

typedef enum                            /* CBIO_RW direction enum */
        {CBIO_READ, CBIO_WRITE}         /* data transfer direction */
        CBIO_RW;


typedef struct cbioParams	/* CBIO Device parameters */
    {
    BOOL	cbioRemovable;	/* removable medium flag */
    block_t 	nBlocks;	/* number of blocks on device */
    size_t 	bytesPerBlk;	/* number of bytes per block */
    size_t	blockOffset;	/* number of blocks from physical 0 */
    short	blocksPerTrack;	/* number of blocks per track */
    short	nHeads;		/* number of heads */
    short	retryCnt;	/* retry count for I/O errors -XXX */
    block_t 	lastErrBlk;	/* block # where last error occurred */
    int 	lastErrno;	/* error code of last error */
    } CBIO_PARAMS;


/* 
 * Cached Block I/O (CBIO) API functions 
 */

/* Library init routine */
IMPORT STATUS cbioLibInit
    (
    void
    );

/* transfer n blocks from/to memory */
IMPORT	STATUS cbioBlkRW		
    (
    CBIO_DEV_ID		dev,
    block_t		startBlock,
    block_t		numBlocks,
    addr_t		buffer,
    CBIO_RW		rw,
    cookie_t *		pCookie
    );


/* transfer n bytes from/to memory */
IMPORT STATUS cbioBytesRW		
    (
    CBIO_DEV_ID 	dev,
    block_t		startBlock,
    off_t		offset,
    addr_t		buffer,
    size_t		nBytes,
    CBIO_RW		rw,
    cookie_t *		pCookie
    );


/* Copy from block to block */
IMPORT STATUS cbioBlkCopy		
    (
    CBIO_DEV_ID 	dev,
    block_t		srcBlock,
    block_t		dstBlock,
    block_t		numBlocks
    );


/* I/O control operations */
IMPORT STATUS cbioIoctl			
    (
    CBIO_DEV_ID		dev,
    int			command,
    addr_t		arg
    );


/* get mode - O_RDONLY, O_WRONLY, O_RDWR */
IMPORT int cbioModeGet  
    (
    CBIO_DEV_ID dev
    );


/* set mode  - O_RDONLY, O_WRONLY, or O_RDWR */
IMPORT STATUS cbioModeSet  
    (
    CBIO_DEV_ID dev,
    int mode
    );


/* determine device ready status */
IMPORT int cbioRdyChgdGet 
    (
    CBIO_DEV_ID dev
    );


/* Force a change in ready status state */
IMPORT STATUS cbioRdyChgdSet  
    (
    CBIO_DEV_ID dev,
    BOOL status
    );


/* Obtain exclusive ownership of device */
IMPORT STATUS cbioLock  
    (
    CBIO_DEV_ID	dev,
    int		timeout	/* max time to pend (semTake) */
    );


/* Release exclusive ownership of device */
IMPORT STATUS cbioUnlock 
    (
    CBIO_DEV_ID	dev
    );


/* fill CBIO_PARAMS structure */
IMPORT STATUS cbioParamsGet		
    (
    CBIO_DEV_ID dev,
    CBIO_PARAMS * pCbioParams		
    );


/* display cbio information */
IMPORT STATUS cbioShow	
    (
    CBIO_DEV_ID dev
    );

/* Verify a CBIO device is valid */             
IMPORT STATUS cbioDevVerify                     
    (                                           
    CBIO_DEV_ID device                          
    );                                          
                                                
/* create a CBIO wapper atop a BLK_DEV */       
IMPORT CBIO_DEV_ID cbioWrapBlkDev               
    (                                           
    BLK_DEV * pDevice                            
    );                                          

/* 
 * CBIO I/O control codes for cbioIoctl()
 */

#define	CBIO_RESET		0xcb100000	/* Reset device */
#define	CBIO_STATUS_CHK		0xcb100001	/* check device status */
#define	CBIO_DEVICE_LOCK	0xcb100002	/* Prevent disk removal */
#define	CBIO_DEVICE_UNLOCK	0xcb100003	/* Allow disk removal */
#define	CBIO_DEVICE_EJECT	0xcb100004	/* Dismount device */
#define	CBIO_CACHE_FLUSH	0xcb100010	/* Flush dirty caches */
#define	CBIO_CACHE_INVAL	0xcb100030	/* Flush & Invalidate all */
#define	CBIO_CACHE_NEWBLK	0xcb100050	/* Allocate scratch block */

/*
 * cbioLib errno's 
 */

#define S_cbioLib_INVALID_CBIO_DEV_ID	(M_cbioLib | 1)

#ifdef __cplusplus
}
#endif

#endif /* __INCcbioLibh */
