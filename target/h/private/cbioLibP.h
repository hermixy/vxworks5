/* cbioLibP.h - cached block I/O device (CBIO) private header file */

/* Copyright 1999-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01e,30jul01,jkf  SPR#69031, common code for both AE & 5.x.
01d,13jun01,jyo  SPR#67729: Added three members to the CBIO_DEV structure also
                 included blkIo.h
01c,29feb00,jkf  T3 changes
01b,07dec99,jkf  changed OBJ_CORE to HANDLE, SEMAPHORE to SEM_ID. 
01a,31aug99,jkf  written - CBIO API changes.
*/



#ifndef __INCcbioLibPh
#define __INCcbioLibPh

#ifdef __cplusplus
extern "C" {
#endif

/*
 * This module is for CBIO modules internal use only. 
 * File system level code (ie users of CBIO device modules)
 * should not include this module but rather should use 
 * the public CBIO API, cbioLib.h.
 *
 * This WRS private header file is subject to change by WRS
 * without notice.  
 */


/* include's */
#include "vxWorks.h"
#include "private/dosFsVerP.h" 	/* determine DOSFS2 OS version */
#include "cbioLib.h"  		/* pull in public CBIO API */
#include "semLib.h"		/* for SEM_ID type */
#include "blkIo.h"		/* for BLK_DEV type */

/* macro's */

#define CBIO_READYCHANGED(x) 	((x)->readyChanged)
#define CBIO_REMOVABLE(x) 	((x)->cbioParams.cbioRemovable)
#define CBIO_MODE(x) 		((x)->cbioMode)

#ifndef CBIO_DEV_EXTRA 
#define CBIO_DEV_EXTRA void	/* module specific field */
#endif

/* typedef's */

typedef struct cbioFuncs		/* CBIO modules method functions */
    {
    STATUS	(* cbioDevBlkRW)		/* Read/Write blocks */
	(CBIO_DEV_ID	dev,
	 block_t	startBlock,
	 block_t	numBlocks,
	 addr_t		buffer,
	 CBIO_RW	rw,
	 cookie_t *     pCookie);
    STATUS	(* cbioDevBytesRW)	/* Read/Write bytes */
	(CBIO_DEV_ID 	dev,
 	 block_t	startBlock,
	 off_t		offset,
	 addr_t		buffer,
	 size_t		nBytes,
	 CBIO_RW	rw,
	 cookie_t * 	pCookie);
    STATUS	(* cbioDevBlkCopy)	/* Copy sectors */
	(CBIO_DEV_ID 	dev,
	 block_t	srcBlock,
	 block_t	dstBlock,
	 block_t	numBlocks);
    STATUS	(* cbioDevIoctl)		/* control operations */
	(CBIO_DEV_ID	dev,
	 int		command,
	 addr_t		arg);
    } CBIO_FUNCS;

typedef struct	cbioDev		/* CBIO_DEV */
    {
#ifdef _WRS_DOSFS2_VXWORKS_AE
    HANDLE	cbioHandle; 		/* VxWorks AE handle management */
#else
    OBJ_CORE	objCore; 		/* VxWorks 5.x objCore */
#endif /* _WRS_DOSFS2_VXWORKS_AE */

    /* Embedded objects */

    SEM_ID	cbioMutex;		/* mutex semaphore */

    /* Functions (methods) */

    struct cbioFuncs * pFuncs; 		/* cbioFuncs functions */

    /* Public attributes */

    char *	cbioDesc;	/* printable descriptive string */
    short	cbioMode;		/* O_RDONLY |O_WRONLY| O_RDWR */
    BOOL        readyChanged;      /* Device READY status indicator */
    /* ** Physical device attributes */

    CBIO_PARAMS cbioParams;		/* Physical parameters, cbioLib.h */

    /* *** Implementation defined attributes */

    caddr_t	cbioMemBase;		/* base addr of memory pool */
    size_t	cbioMemSize;		/* size of memory pool used */
    u_long	cbioPriv0;		/* Implementation defined */
    u_long	cbioPriv1;		/* Implementation defined */
    u_long	cbioPriv2;		/* Implementation defined */
    u_long	cbioPriv3;		/* Implementation defined */
    u_long	cbioPriv4;		/* Implementation defined */
    u_long	cbioPriv5;		/* Implementation defined */
    u_long	cbioPriv6;		/* Implementation defined */
    u_long	cbioPriv7;		/* Implementation defined */
    CBIO_DEV_EXTRA * pDc ;		/* Implementation defined structure */
    CBIO_DEV_ID cbioSubDev;	       /* Stores the pointer to lower CBIO.  */
    BLK_DEV   * blkSubDev;	       /* Stores the pointer to lower BLKDEV. */
    BOOL        isDriver;	       /* This variable is used to verify */
				       /* the nature of current layer.     */
    } CBIO_DEV;

/* externals */

IMPORT CLASS_ID	cbioClassId ;

/* private functions */

CBIO_DEV_ID cbioDevCreate 	/* used by CBIO modules only */
    (
    caddr_t  	ramAddr, 	/* where it is in memory (0 = malloc) */
    size_t	ramSize		/* pool size */
    );

#ifdef __cplusplus
}
#endif

#endif /* __INCcbioLibPh */
