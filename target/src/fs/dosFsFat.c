/* dosFsFat.c - DOS file system File Allocation Table Handler */

/* Copyright 1987-2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01q,10jan02,chn  Renamed some functions to clear up sector/cluster confusion
01p,10dec01,jkf  SPR#72039, various fixes from Mr. T. Johnson.
01o,12oct01,jkf  adapted debugging code to be more portable. 
01n,02oct01,jkf  replaced diab changes lost in 01m.
01m,20sep01,jkf  SPR#69031, common code for both AE & 5.x.
01l,08oct99,jkf  added ifdef for HP native tools to ignore GNUish debug macro's
01k,31aug99,jkf  changes for new CBIO API.
01j,31jul99,jkf  T2 merge, tidiness & spelling.
01i,21dec98,mjc  fixed bug in fat16Seek(), changed name of <contigEnd> 
                 field in structure <DOS_FILE_HDL> to <contigEndPlus1>
01h,23nov98,mjc  avoided disk access for seek within contiguous file area;
01j,23sep98,vld  added FAT mirroring disabling;
		 added routine fat16SyncToggle()
01f,20sep98,mjc  descriptor structure name changed from DOS_FAT16_DESC
                 to MS_FAT_DESC
01e,17sep98,vld  added argument "number of FAT copy" to cluster read/write
                 routines;
cluster writing routines extended with a"raw" data
		 write to FAT copies other, then active one;
    	    	 routine fat16ClustValueSet() extended to support
                 FAT copy initialization;
01d,12jul98,mjc  cluster group allocation factor changed from
                 constant to global variable fatClugFac.
                 Assert header files including was moved 
                 to be after #ifdef DEBUG statement to allow
                 NDEBUG definition  in case when DEBUG is undefined.
                 fat16ContigAlloc() algorithm was changed to 1st fit.
01c,01jul98,lrn  doc reviewed
01b,29jun98,mjc  added FAT32, tested
01a,23feb98,mjc  initially written written.
*/

/*
DESCRIPTION
This module is part of dosFsLib and is designed to manage the three
different File Allocation Table formats: FAT12, FAT16 and FAT32.
These formats are similar thus are implemented in a single module
without the ability to scale out one of these formats for systems which
do not intend to use them.

IMPLEMENTATION
This FAT handler does not keep any part of the File Allocation Table in
memory, depending entirely on the underlying CBIO module, typically the
Disk Cache for the ability to access any FAT entry of any size, i.e.
byte-wise access to any particular disk sector.

INITIALIZATION
*/

/* includes */

#include "vxWorks.h"
#include "private/dosFsVerP.h"
#include "semLib.h"
#include "errnoLib.h"
#include "logLib.h"
#include "string.h"
#include "memLib.h"
#include "stdlib.h"
#include "stdio.h"
#include "taskLib.h"

#include "private/print64Lib.h"
#include "private/dosFsLibP.h"
#include "private/dosFsFatP.h"

/* macros */

#define _TO_STR(z) _TO_TMP(z)
#define _TO_TMP(z) #z

#undef ERR_MSG
#undef DBG_MSG
#undef ERR_PRINT

#ifdef DEBUG

#   undef  LOCAL
#   define LOCAL

#   define ERR_PRINT	/* include errnoSetOut() */
#   define errnoSet( err ) errnoSetOut( __FILE__, __LINE__, #err, (err) )

#   define DBG_MSG(lvl, fmt, a1, a2, a3, a4, a5, a6)                       \
        { if ((lvl) <= fat16Debug)                                         \
            printErr (__FILE__" : "_TO_STR(__LINE__)" : "fmt, a1, a2,      \
                      a3, a4, a5, a6); }

#   define ERR_MSG(lvl, fmt, a1, a2, a3, a4, a5, a6)			   \
        { logMsg (__FILE__" : "_TO_STR(__LINE__)" : "fmt, (int)(a1),       \
                  (int)(a2), (int)(a3), (int)(a4), (int)(a5), (int)(a6)); }

#else   /* NO DEBUG */

#   undef  NDEBUG
#   define NDEBUG


/* fixme: quick dirty hack for native HP native tools build, HP simulator */
#   if (CPU == SIMHPPA)
    /* Allow TOOL=hp to build, will cause no effect warnings when TOOL=gnu */
#        define DBG_MSG
#        define ERR_MSG
#   else
#        define DBG_MSG(lvl, fmt, a1, a2, a3, a4, a5, a6) {}
#        define ERR_MSG(lvl, fmt, a1, a2, a3, a4, a5, a6)                    \
             { if ((lvl) <= fat16Debug)                                      \
                 logMsg (__FILE__" : "_TO_STR(__LINE__)" : %s : "fmt,        \
                 (int)(a1), (int)(a2), (int)(a3), (int)(a4), (int)(a5),      \
		 (int)(a6)); }
#   endif /* (CPU == SIMHPPA) */
#endif /* DEBUG */
                                                                             
#include "assert.h"


/* defines */

/* File System Information Sector number */

#define FSINFO_SEC_NUM		1

/* Offset of free clusters count field in the File System Information Sector */

#define FSINFO_SIGN		484
#define FSINFO_FREE_CLUSTS	488
#define FSINFO_NEXT_FREE        492

/* Read FAT entry error code */

#define FAT_CBIO_ERR		1

/* Mutex semaphore options */

#define ALLOC_SEM_OPTIONS	SEM_Q_PRIORITY | SEM_DELETE_SAFE | \
				SEM_INVERSION_SAFE

#define ENTRY_READ(pFd, copy, entry)		\
		pFatDesc->entryRead ((pFd), (copy), (entry))

#define ENTRY_WRITE(pFd, copy, entry, value)	\
		pFatDesc->entryWrite ((pFd), (copy), (entry), (value))


/* typedefs */


/* globals */

int     fat16Debug = 0;	/* debug level */

int	fatClugFac = 10000;	/* cluster allocation group size factor */


/* locals */



#ifdef ERR_PRINT
/*******************************************************************************
* errnoSetOut - put error message
*
* This routine is called instead errnoSet during debugging.
*/
LOCAL VOID errnoSetOut(char * pFile, int line, const char * pStr, int errCode )
    {
    /* If it is not errno clearing or setting to previously stored value 
     * (in <errnoBuf> variable), print the error information.
     */

    if( errCode != OK  && strcmp( pStr, "errnoBuf" ) != 0 )
        printf( "ERROR from %s : line %d : %s = %p, task %p\n",
                pFile, line, pStr, (void *)errCode, (void *)taskIdSelf() );
    errno = errCode;
    }
#endif  /* ERR_PRINT */

/*******************************************************************************
*
* fat16MirrorSect - mirror FAT sector
*
* This routine copies last modified sector of Primary FAT copy to appropriate 
* sectors of another FAT copies.
*
* RETURNS: OK or ERROR.
*/

LOCAL STATUS fat16MirrorSect
    (
    FAST DOS_FILE_DESC_ID	pFd		/* pointer to file descriptor */
    )
    {
    FAST DOS_VOLUME_DESC_ID	pVolDesc = pFd->pVolDesc;
					/* pointer to volume descriptor */
    FAST MS_FAT_DESC_ID	pFatDesc = (void *) pVolDesc->pFatDesc;
					/* pointer to FAT descriptor */
    FAST CBIO_DEV_ID		pCbio = pVolDesc->pCbio;
					/* pointer to CBIO device */
    FAST uint32_t		srcSecNum = pFd->pFileHdl->fatSector;
    FAST uint32_t		dstSecNum;
    FAST int			fatNum;		/* FAT copy number */
         uint8_t		fsinfoBuf [8];

    if (srcSecNum == 0)
        return OK;

    /* FAT copies synchronization might be disabled */

    if ( ! pFatDesc->syncEnabled)
    	return OK;

    /* 
     * TBD: mirroring disabled (FAT32)
     * ? Maybe it is worth adding callbacks to process CBIO write errors ?
     */

    for (fatNum = 1, dstSecNum = srcSecNum + pVolDesc->secPerFat; 
         fatNum < pVolDesc->nFats; 
         fatNum++, dstSecNum += pVolDesc->secPerFat)
        {
        if (cbioBlkCopy (pCbio, srcSecNum, dstSecNum, 1) != OK)
            return ERROR;
        /* ...??? */
        }

    if (pVolDesc->fatType == FAT32)
        {
        VX_TO_DISK_32 (pFatDesc->fatEntFreeCnt, &fsinfoBuf[0]);
        VX_TO_DISK_32 (pFatDesc->groupAllocStart, &fsinfoBuf[4]);

        if (OK != cbioBytesRW (pCbio, FSINFO_SEC_NUM, FSINFO_FREE_CLUSTS, 
		(addr_t)fsinfoBuf, sizeof (fsinfoBuf), CBIO_WRITE, NULL))
            return ERROR;
        }

    return OK;
    } /* fat16MirrorSect */

/*******************************************************************************
*
* fat16SyncToggle - toggle FAT copies mirroring.
* 
* This routine enables/disables FAT copes mirroring. When mirroring
* is enabled all reserved FAT copes are synchronized with active one.
* 
* RETURNES: N/A.
*/
LOCAL void fat16SyncToggle
    (
    DOS_VOLUME_DESC_ID	pVolDesc,
    BOOL	syncEnable
    )
    {
    MS_FAT_DESC_ID	pFatDesc = (MS_FAT_DESC_ID)pVolDesc->pFatDesc;
    block_t		srcSec, dstSec, secNum;
    uint8_t		copyNum;
    
    pFatDesc->syncEnabled = syncEnable;
    
    if (! syncEnable)
    	return;
    
    /* synchronize FAT copies */
    
    /* copy by one sector. It permits always one read - multi write */

    srcSec = pFatDesc->fatStartSec +
             pVolDesc->pFatDesc->activeCopyNum * pVolDesc->secPerFat;
    for (secNum = 0; secNum < pVolDesc->secPerFat; secNum ++, srcSec ++ )
    	{
    	for (dstSec = pFatDesc->fatStartSec + secNum, copyNum = 0;
	     copyNum < pVolDesc->nFats;
	     dstSec += pVolDesc->secPerFat, copyNum ++ )
    	    {
    	    if (copyNum == pVolDesc->pFatDesc->activeCopyNum)
    	    	continue;
    	    
    	    cbioBlkCopy (pVolDesc->pCbio, srcSec, dstSec, 1);
    	    }
    	}
    } /* fat16SyncToggle() */

/*******************************************************************************
*
* fat12EntRead - read FAT entry from disk
* 
* This routine reads the file allocation table (FAT) entry from a dosFs
* volume.
*
* This routine only reads from the first copy of the FAT on disk, even if
* other copies exist.
* 
* RETURNS: Contents of the entry, or FAT_CBIO_ERR if error accessing disk.
*/

LOCAL uint32_t fat12EntRead
    (
    FAST DOS_FILE_DESC_ID	pFd,	/* pointer to file descriptor */
    FAST uint32_t		copyNum,/* fat copy number */
    FAST uint32_t		cluster	/* entry number */
    )
    {
    FAST DOS_VOLUME_DESC_ID	pVolDesc = pFd->pVolDesc;
					/* pointer to volume descriptor */
    FAST CBIO_DEV_ID		pCbio = pVolDesc->pCbio;
					/* pointer to CBIO device */
    FAST MS_FAT_DESC_ID	pFatDesc = (void *) pVolDesc->pFatDesc;
					/* pointer to FAT descriptor */
    FAST uint32_t		fatEntry;/* FAT entry */
    FAST uint32_t		fatOff;	/* offset in the FAT */
    FAST block_t		secNum;	/* sector number to read/write */
    FAST off_t			secOff;	/* offset in the sector */
    FAST uint32_t		nBytes;	/* number of bytes */
         uint8_t		valBuf [2];
					/* buffer for value in the entry */

    assert ( copyNum < pVolDesc->nFats);
    assert ( (cluster >= DOS_MIN_CLUST) && (cluster < pFatDesc->nFatEnts) );

    /* Offset of the entry in the FAT in bytes */

    fatOff = cluster + (cluster >> 1);	/* index = cluster * 1.5 */

    /* Number of sector containing the entry */

    secNum = pFatDesc->fatStartSec + copyNum * pVolDesc->secPerFat +
            (fatOff >> pVolDesc->secSizeShift);

    /* Offset of the entry in the sector */

    secOff = fatOff & (pVolDesc->bytesPerSec - 1);

    /* Number of bytes to read */

    nBytes = pVolDesc->bytesPerSec - secOff;
    if (nBytes > 2)
        nBytes = 2;

    /* Read entry from disk */

    if (cbioBytesRW (pCbio, secNum, secOff, (addr_t)valBuf, nBytes, CBIO_READ, 
				&pFd->fatHdl.cbioCookie) != OK)
        {
        pFd->fatHdl.errCode = FAT_CBIO_ERR;
        return FAT_CBIO_ERR;				/* read error */
        }

    if (nBytes == 1)
        if (cbioBytesRW (pCbio, secNum + 1, 0, (addr_t)(&valBuf[1]), 1, 
				CBIO_READ, &pFd->fatHdl.cbioCookie) != OK)
            {
            pFd->fatHdl.errCode = FAT_CBIO_ERR;
            return FAT_CBIO_ERR;			/* read error */
            }

    fatEntry = valBuf [0] | (valBuf [1] << 8);
					/* copy word, swapping bytes */
    if (cluster & 0x1)				/* if cluster number is ODD */
        fatEntry = (fatEntry >> 4) & 0xfff;	/*  keep high order 12 bits */
    else					/* if cluster number is EVEN */
        fatEntry &= 0xfff;			/*  keep low order 12 bits */

    return fatEntry;
    } /* fat12EntRead */

/*******************************************************************************
*
* fat12EntWrite - write FAT entry to disk
* 
* This routine writes the file allocation table (FAT) entry to a dosFs
* volume.
*
* This routine reads the file allocation table (FAT) entry for the
* specified cluster and returns it to the caller.
*
* If the value to be written is too large to fit in a FAT entry
* (12 bits), it is truncated (high order bits are discarded).
*
* This routine only writes to the first copy of the FAT on disk, even if
* other copies exist.
* 
* RETURNS: OK, or ERROR if error accessing disk.
*/

LOCAL STATUS fat12EntWrite
    (
    FAST DOS_FILE_DESC_ID	pFd,	/* pointer to file descriptor */
    FAST uint32_t		copyNum,/* fat copy number */
    FAST uint32_t		cluster,/* entry number */
    FAST uint32_t		value	/* value to write */
    )
    {
    FAST DOS_VOLUME_DESC_ID	pVolDesc = pFd->pVolDesc;
					/* pointer to volume descriptor */
    FAST CBIO_DEV_ID		pCbio = pVolDesc->pCbio;
					/* pointer to CBIO device */
    FAST MS_FAT_DESC_ID	pFatDesc = (void *) pVolDesc->pFatDesc;
					/* pointer to FAT descriptor */
    FAST uint32_t		fatOff;	/* offset in the FAT */
    FAST block_t		secNum;	/* sector number to read/write */
    FAST off_t			secOff;	/* offset in the sector */
    FAST uint32_t		nBytes;	/* number of bytes */
         cookie_t    		cookie;	/* CBIO dev. cookie */
         uint8_t		valBuf [2];
					/* buffer for value in the entry */

    assert ( copyNum < pVolDesc->nFats);
    assert ( (cluster >= DOS_MIN_CLUST) && (cluster < pFatDesc->nFatEnts) );
    assert ( copyNum != pVolDesc->pFatDesc->activeCopyNum ||
             ( (value == pFatDesc->dos_fat_avail) || 
               ((value >= DOS_MIN_CLUST) && (value < pFatDesc->nFatEnts)) || 
               (value == pFatDesc->dos_fat_eof) ) );

    cookie = pFd->fatHdl.cbioCookie;

    /* Offset of the entry in the FAT in bytes */

    fatOff = cluster + (cluster >> 1);	/* index = cluster * 1.5 */

    /* Number of sector containing the entry */

    secNum = pFatDesc->fatStartSec + copyNum * pVolDesc->secPerFat +
            (fatOff >> pVolDesc->secSizeShift);

    /* Mirror last modified FAT sector if entry is situated in other sector */

    if (pFd->pFileHdl->fatSector != secNum &&
        copyNum == pVolDesc->pFatDesc->activeCopyNum)
        {
        fat16MirrorSect (pFd);
        pFd->pFileHdl->fatSector = secNum;
        }

    /* Offset of the entry in the sector */

    secOff = fatOff & (pVolDesc->bytesPerSec - 1);

    /* Number of bytes to write */

    nBytes = pVolDesc->bytesPerSec - secOff;
    if (nBytes > 2)
        nBytes = 2;

    /* Read previous entry value */

    if (cbioBytesRW (pCbio, secNum, secOff,(addr_t) valBuf, nBytes, 
			CBIO_READ, &cookie) != OK)
        return ERROR;				/* read error */

    if (nBytes == 1)
        if (cbioBytesRW (pCbio, secNum + 1, 0,(addr_t) (&valBuf[1]), 1, 
				CBIO_READ, &pFd->fatHdl.cbioCookie) != OK)
            return ERROR;			/* read error */

    if (cluster & 0x1)				/* if cluster number is ODD */
        {
        valBuf [0] &= 0x0f;			/* clear high order 4 bits */
        valBuf [0] |= (value << 4);		/* put in low bits of value */
        valBuf [1]  = (value >> 4);		/* next byte is high bits */
        }
    else					/* if cluster number is EVEN */
        {
        valBuf [0]  = value;			/* this byte gets low bits */
        valBuf [1] &= 0xf0;			/* clear low bits next byte */
        valBuf [1] |= (value >> 8) & 0x0f;	/* put in high bits */
        }

    /* Write entry to disk */

    if (cbioBytesRW (pCbio, secNum, secOff,(addr_t)valBuf, nBytes, CBIO_WRITE, 
				&cookie) != OK)
        return ERROR;

    if (nBytes == 1)
        if (cbioBytesRW (pCbio, secNum + 1, 0, (addr_t)(&valBuf [1]), 1, 
				CBIO_WRITE, &pFd->fatHdl.cbioCookie) != OK)
        return ERROR;

    return OK;
    } /* fat12EntWrite */

/*******************************************************************************
* 
* fat16EntRead - read FAT entry from disk
* 
* This routine reads the file allocation table (FAT) entry from a dosFs
* volume.
*
* This routine only reads from the first copy of the FAT on disk, even if
* other copies exist.
* 
* RETURNS: Contents of the entry, or FAT_CBIO_ERR if error accessing disk.
*/

LOCAL uint32_t fat16EntRead
    (
    FAST DOS_FILE_DESC_ID	pFd,	/* pointer to file descriptor */
    FAST uint32_t		copyNum,/* fat copy number */
    FAST uint32_t		entry	/* entry number */
    )
    {
    FAST DOS_VOLUME_DESC_ID	pVolDesc = pFd->pVolDesc;
					/* pointer to volume descriptor */
    FAST CBIO_DEV_ID		pCbio = pVolDesc->pCbio;
					/* pointer to CBIO device */
    FAST MS_FAT_DESC_ID	pFatDesc = (void *) pVolDesc->pFatDesc;
					/* pointer to FAT descriptor */
    FAST block_t		secNum;	/* sector number to read/write */
    FAST off_t			secOff;	/* offset in the sector */
         uint8_t		valBuf [2];
					/* buffer for value in the entry */

    assert ( copyNum < pVolDesc->nFats);
    assert ( (entry >= DOS_MIN_CLUST) && (entry < pFatDesc->nFatEnts) );

    /* Offset of the entry in the FAT in bytes */

    entry <<= 1;

    /* Number of sector containing the entry */

    secNum = pFatDesc->fatStartSec + copyNum * pVolDesc->secPerFat +
            (entry >> pVolDesc->secSizeShift);

    /* Offset of the entry in the sector */

    secOff = entry & (pVolDesc->bytesPerSec - 1);

    /* Read entry from disk */

    if (cbioBytesRW (pCbio, secNum, secOff, (addr_t)valBuf, 2, CBIO_READ, 
				&pFd->fatHdl.cbioCookie) != OK)
        {
        pFd->fatHdl.errCode = FAT_CBIO_ERR;
        return FAT_CBIO_ERR;				/* read error */
        }

    return (valBuf [0] | (valBuf [1] << 8));
					/* copy word, swapping bytes */
    } /* fat16EntRead */

/*******************************************************************************
* 
* fat16EntWrite - write FAT entry to disk
* 
* This routine writes the file allocation table (FAT) entry to a dosFs
* volume.
*
* This routine only writes to the first copy of the FAT on disk, even if
* other copies exist.
* 
* RETURNS: OK, or ERROR if error accessing disk.
*/

LOCAL STATUS fat16EntWrite
    (
    FAST DOS_FILE_DESC_ID	pFd,	/* pointer to file descriptor */
    FAST uint32_t		copyNum,/* fat copy number */
    FAST uint32_t		entry,	/* entry number */
    FAST uint32_t		value	/* value to write */
    )
    {
    FAST DOS_VOLUME_DESC_ID	pVolDesc = pFd->pVolDesc;
					/* pointer to volume descriptor */
    FAST CBIO_DEV_ID		pCbio = pVolDesc->pCbio;
					/* pointer to CBIO device */
    FAST MS_FAT_DESC_ID	pFatDesc = (void *) pVolDesc->pFatDesc;
					/* pointer to FAT descriptor */
    FAST block_t		secNum;	/* sector number to read/write */
    FAST off_t			secOff;	/* offset in the sector */
         uint8_t		valBuf [2];
					/* buffer for value in the entry */

    assert ( copyNum < pVolDesc->nFats);
    assert ( (entry >= DOS_MIN_CLUST) && (entry < pFatDesc->nFatEnts) );
    assert ( copyNum != pVolDesc->pFatDesc->activeCopyNum ||
             ( (value == pFatDesc->dos_fat_avail) || 
               ((value >= DOS_MIN_CLUST) && (value < pFatDesc->nFatEnts)) || 
               (value == pFatDesc->dos_fat_eof) ) );

    /* Offset of the entry in the FAT in bytes */

    entry <<= 1;

    /* Number of sector containing the entry */

    secNum = pFatDesc->fatStartSec + copyNum * pVolDesc->secPerFat +
            (entry >> pVolDesc->secSizeShift);

    /* Mirror last modified FAT sector if entry is situated in other sector */

    if (pFd->pFileHdl->fatSector != secNum &&
        copyNum == pVolDesc->pFatDesc->activeCopyNum)
        {
        fat16MirrorSect (pFd);
        pFd->pFileHdl->fatSector = secNum;
        }

    /* Offset of the entry in the sector */

    secOff = entry & (pVolDesc->bytesPerSec - 1);

    valBuf [0] = value        & 0xff;	/* this byte gets low bits */
    valBuf [1] = (value >> 8) & 0xff;	/* next byte gets high bits */

    /* Write entry to disk */

    return cbioBytesRW (pCbio, secNum, secOff, (addr_t)valBuf, 2, CBIO_WRITE, 
				&pFd->fatHdl.cbioCookie);
    } /* fat16EntWrite */

/*******************************************************************************
* 
* fat32EntRead - read FAT entry from disk
* 
* This routine reads the file allocation table (FAT) entry from a dosFs
* volume.
*
* This routine only reads from the first copy of the FAT on disk, even if
* other copies exist.
* 
* RETURNS: Contents of the entry, or FAT_CBIO_ERR if error accessing disk.
*/

LOCAL uint32_t fat32EntRead
    (
    FAST DOS_FILE_DESC_ID	pFd,	/* pointer to file descriptor */
    FAST uint32_t		copyNum,/* fat copy number */
    FAST uint32_t		entry	/* entry number */
    )
    {
    FAST DOS_VOLUME_DESC_ID	pVolDesc = pFd->pVolDesc;
					/* pointer to volume descriptor */
    FAST CBIO_DEV_ID		pCbio = pVolDesc->pCbio;
					/* pointer to CBIO device */
    FAST MS_FAT_DESC_ID	pFatDesc = (void *) pVolDesc->pFatDesc;
					/* pointer to FAT descriptor */
    FAST block_t		secNum;	/* sector number to read/write */
    FAST off_t			secOff;	/* offset in the sector */
         uint8_t		valBuf [4];
         uint8_t		highByteMask = 0x0f;

    /* do not mask high bits when non active copy is being written */

    if (copyNum != pVolDesc->pFatDesc->activeCopyNum)
    	highByteMask = 0xff;
					/* buffer for value in the entry */

    assert ( copyNum < pVolDesc->nFats);
    assert ( (entry >= DOS_MIN_CLUST) && (entry < pFatDesc->nFatEnts) );

    /* Offset of the entry in the FAT in bytes */

    entry <<= 2;

    /* Number of sector containing the entry */

    secNum = pFatDesc->fatStartSec + copyNum * pVolDesc->secPerFat +
            (entry >> pVolDesc->secSizeShift);

    /* Offset of the entry in the sector */

    secOff = entry & (pVolDesc->bytesPerSec - 1);

    /* Read entry from disk */

    if (cbioBytesRW (pCbio, secNum, secOff, (addr_t)valBuf, 4, CBIO_READ, 
			     &pFd->fatHdl.cbioCookie) != OK)
        {
        pFd->fatHdl.errCode = FAT_CBIO_ERR;
        return FAT_CBIO_ERR;				/* read error */
        }

    return (valBuf [0] | (valBuf [1] << 8) | (valBuf [2] << 16) | 
            ((valBuf [3] & highByteMask) << 24));/* mask highest 4 bits */
    } /* fat32EntRead */

/*******************************************************************************
* 
* fat32EntWrite - write FAT entry to disk
* 
* This routine writes the file allocation table (FAT) entry to a dosFs
* volume.
*
* This routine only writes to the first copy of the FAT on disk, even if
* other copies exist.
* 
* RETURNS: OK, or ERROR if error accessing disk.
*/

LOCAL STATUS fat32EntWrite
    (
    FAST DOS_FILE_DESC_ID	pFd,	/* pointer to file descriptor */
    FAST uint32_t		copyNum,/* fat copy number */
    FAST uint32_t		entry,	/* entry number */
    FAST uint32_t		value	/* value to write */
    )
    {
    FAST DOS_VOLUME_DESC_ID	pVolDesc = pFd->pVolDesc;
					/* pointer to volume descriptor */
    FAST CBIO_DEV_ID		pCbio = pVolDesc->pCbio;
					/* pointer to CBIO device */
    FAST MS_FAT_DESC_ID	pFatDesc = (void *) pVolDesc->pFatDesc;
					/* pointer to FAT descriptor */
    FAST block_t		secNum;	/* sector number to read/write */
    FAST off_t			secOff;	/* offset in the sector */
         uint8_t		valBuf [4];
					/* buffer for value in the entry */
         uint8_t		highByteMask = 0x0f;

    /* do not mask high bits when non active copy is being written */

    if (copyNum != pVolDesc->pFatDesc->activeCopyNum)
    	highByteMask = 0xff;

    assert ( copyNum < pVolDesc->nFats);
    assert ( (entry >= DOS_MIN_CLUST) && (entry < pFatDesc->nFatEnts) );
    assert ( highByteMask == 0xff ||
             ( (value == pFatDesc->dos_fat_avail) || 
               ((value >= DOS_MIN_CLUST) && (value < pFatDesc->nFatEnts)) || 
               (value == pFatDesc->dos_fat_eof) ) );

    /* Offset of the entry in the FAT in bytes */

    entry <<= 2;

    /* Number of sector containing the entry */

    secNum = pFatDesc->fatStartSec + copyNum * pVolDesc->secPerFat +
            (entry >> pVolDesc->secSizeShift);

    /* Mirror last modified FAT sector if entry is situated in other sector */

    if (pFd->pFileHdl->fatSector != secNum && highByteMask != 0xff )
        {
        fat16MirrorSect (pFd);
        pFd->pFileHdl->fatSector = secNum;
        }

    /* Offset of the entry in the sector */

    secOff = entry & (pVolDesc->bytesPerSec - 1);

    valBuf [0] = value         & 0xff;	/*  */
    valBuf [1] = (value >> 8)  & 0xff;	/*  */
    valBuf [2] = (value >> 16) & 0xff;	/*  */
    valBuf [3] = (value >> 24) & highByteMask;	/*  */

    /* Write entry to disk */

    return cbioBytesRW (pCbio, secNum, secOff, (addr_t)valBuf, 4, CBIO_WRITE, 
				&pFd->fatHdl.cbioCookie);
    } /* fat32EntWrite */

/*******************************************************************************
*
* fat16ContigGet - get next section of contiguous clusters in file chain
* 
* This routine reads FAT entries starting from the passed cluster number, and 
* returns number of contiguous clusters in the file chain starting from the 
* passed one up to number of clusters requested.
*
* If starting cluster number is out of legal cluster number range, a 0 is 
* returned, except of the following 2 cases:
*
* 1) If start cluster number is 0 .
*
* 2) If start cluster number is end of file .
*
* This routine checks the File Allocation Table (FAT) to determine the
* length, in clusters, of a contiguous section of a file.  The starting
* cluster to check is passed as an input parameter.  The number of
* contiguous clusters beginning with that cluster (including the starting
* cluster itself) is returned.  The minimum returned value is 1.
*
* RETURNS: Number of contiguous clusters in chain starting from the passed one,
*          or 0 if starting cluster number is illegal or disk access error.
*/

LOCAL STATUS fat16ContigGet
    (
    FAST DOS_FILE_DESC_ID pFd,		/* pointer to file descriptor */
    FAST uint32_t	  numClusts	/* num. of clusters to follow */
    )
    {
    FAST DOS_FILE_HDL_ID	pFileHdl = pFd->pFileHdl;
					/* pointer to file handle */
    FAST DOS_FAT_HDL_ID		pFatHdl = &(pFd->fatHdl);
					/* pointer to FAT handle */
    FAST DOS_VOLUME_DESC_ID	pVolDesc = pFd->pVolDesc;
					/* pointer to volume descriptor */
    FAST MS_FAT_DESC_ID	pFatDesc = (void *) pVolDesc->pFatDesc;
				/* pointer to FAT handler descriptor */
    FAST uint32_t		startClust;	/* starting cluster number */
    FAST uint32_t		nextClust;	/* next cluster number */
    FAST uint32_t		cluster;	/* cluster number */
    FAST uint32_t		maxClust;	/* maximum cluster number */

    startClust = nextClust = pFatHdl->nextClust;
    cluster    = pFatHdl->lastClust;

    if ((startClust < DOS_MIN_CLUST) || (startClust >= pFatDesc->nFatEnts))
        {
        if ((startClust == 0) && (cluster == 0))	/* file just open */
            startClust = pFileHdl->startClust;

        if (startClust > pFatDesc->dos_fat_bad)		/* end of file chain */
    	    {
            startClust = ENTRY_READ (pFd, pFatDesc->dosFatDesc.activeCopyNum,
                                     cluster);
    	    }

        if ((startClust < DOS_MIN_CLUST) || (startClust >= pFatDesc->nFatEnts))
            {
            assert ((startClust == 0) || (startClust > pFatDesc->dos_fat_bad));
            pFatHdl->nextClust = startClust;
            return ERROR;
            }
        }

    if ( (startClust < pFileHdl->contigEndPlus1) && 
         (startClust >= pFileHdl->startClust) )
        {	/* in contiguous area */
        cluster = startClust + numClusts;	/* lastClust + 1 */
		/* numClusts can be avoided in the function calls and changed 
		 * to pFatDesc->fatGroupSize inside the function 
		 */

        if (cluster >= pFileHdl->contigEndPlus1)
            {
            cluster = pFileHdl->contigEndPlus1;	/* lastClust + 1 */
            nextClust = pFatDesc->dos_fat_eof;
            }
        else
            nextClust = cluster;

        DBG_MSG (2, "Get from contiguous area.\n", 1,2,3,4,5,6);
        }
    else
        {	/* out of contiguous area */

        /* Count number of contiguous clusters starting from <startClust> */

        maxClust = startClust + numClusts;
        if (maxClust > pFatDesc->nFatEnts)
            maxClust = pFatDesc->nFatEnts;
        cluster = startClust;			/* initialize cluster number */

        while (cluster < maxClust)
            {
            nextClust = ENTRY_READ (pFd, pFatDesc->dosFatDesc.activeCopyNum,
                                    cluster);	/* follow chain */

            if (nextClust != ++cluster)
                break;				/* end of contiguous area */
            }

        if (pFatHdl->errCode == FAT_CBIO_ERR)
            return ERROR;
        }
#ifdef	__unused
    assert ( ((nextClust >= DOS_MIN_CLUST)&&
              (nextClust < pFatDesc->nFatEnts)) ||
             ((nextClust > pFatDesc->dos_fat_bad) && 
              (nextClust <= pFatDesc->dos_fat_eof)) );
#endif
    /* Store contents of last entry in contiguous section */

    pFatHdl->nextClust = nextClust;

    pFatHdl->lastClust = cluster - 1;

    pFd->curSec = CLUST_TO_SEC (pVolDesc, startClust);

    pFd->nSec   = (cluster - startClust) * pVolDesc->secPerClust;

    DBG_MSG (2, "Get %ld clusters starting from cluster %ld\n", 
             cluster - startClust, startClust,3,4,5,6);

    return OK;
    } /* fat16ContigGet */

/*******************************************************************************
*
* fat16MarkAlloc - allocate a contiguous set of clusters
*
*
* RETURNS: ERROR if was unable to allocate requested amount of space
*/

LOCAL STATUS fat16MarkAlloc
    (
    FAST DOS_FILE_DESC_ID	pFd,		/* pointer to file descriptor */
    FAST uint32_t		firstClust,	/* initial cluster to search */
    FAST uint32_t		numClusts	/* number of clusters needed */
    )
    {
    FAST MS_FAT_DESC_ID	pFatDesc = (void *) pFd->pVolDesc->pFatDesc;
						/* pointer to FAT descriptor */
    FAST uint32_t		curClust;	/* current cluster number */

    assert ((firstClust >= DOS_MIN_CLUST) && 
            (firstClust < pFatDesc->nFatEnts));
    assert (numClusts <= (pFatDesc->nFatEnts - DOS_MIN_CLUST));

    /* Build cluster chain in FAT */

    for (curClust = firstClust; 
         curClust < (firstClust + numClusts - 1); 
         curClust++)
        {

        /* Each entry = next clust. num. */

        if (ENTRY_WRITE (pFd, pFatDesc->dosFatDesc.activeCopyNum,
                         curClust, curClust + 1) != OK)
            return ERROR;	/* disk access error */

        /* Update free clusters counter */

        pFatDesc->fatEntFreeCnt--;
        }

    /* Mark last entry as end of file cluster chain */

    if (ENTRY_WRITE (pFd, pFatDesc->dosFatDesc.activeCopyNum, curClust,
                     pFatDesc->dos_fat_eof) != OK)
        return ERROR;		/* disk access error */

    pFatDesc->fatEntFreeCnt--;

    return OK;
    } /* fat16MarkAlloc */

/*******************************************************************************
*
* fat16GetNext - get/allocate next cluster for file
*
* This routine searches the File Allocation Table (FAT) for a sequence
* of contiguous free clusters, and allocates clusters to extend the
* current chain if requested to do so.
*
* RETURNS: resulting chain of sectors in the File Descriptor structure.
*/

LOCAL STATUS fat16GetNext
    (
    FAST DOS_FILE_DESC_ID pFd,		/* pointer to file descriptor */
    FAST uint_t		  allocPolicy	/* allocation policy */
    )
    {
    FAST DOS_FILE_HDL_ID	pFileHdl = pFd->pFileHdl;
						/* pointer to file handle */
    FAST DOS_FAT_HDL_ID		pFatHdl = &pFd->fatHdl;
						/* pointer to FAT handle */
    FAST DOS_VOLUME_DESC_ID	pVolDesc = pFd->pVolDesc;
					/* pointer to volume descriptor */
    FAST MS_FAT_DESC_ID	pFatDesc = (void *) pVolDesc->pFatDesc;
						/* pointer to FAT descriptor */
    FAST uint32_t		startClust;	/*  */
    FAST uint32_t		numClusts;	/*  */
    FAST uint32_t		prevClust;	/*  */
    FAST uint32_t		firstClust;	/*  */
    FAST uint32_t		contigCount;	/* count of fit clusters */
    FAST uint32_t		curClust;	/* current cluster number */
    FAST uint32_t		fatEntry;	/* FAT entry */
    FAST uint32_t		maxClust;	/* maximum cluster number */
    FAST uint32_t		pass;		/*  */

    /* Try to follow file chain */

    if (fat16ContigGet (pFd, pFatDesc->fatGroupSize) == OK)
        return OK;

    if (pFatHdl->errCode == FAT_CBIO_ERR)
        return ERROR;

    /* Can't follow file chain */

    if ((allocPolicy & FAT_ALLOC) == 0)		/* not alloc */
        return ERROR;

    firstClust = pFatHdl->nextClust;
    prevClust  = pFatHdl->lastClust;
    startClust = 0;

    /* Set number of clusters to allocate.
     */

    if (pFatDesc->groupAllocStart == 0)	/* no free groups in prev. alloc. */
        allocPolicy = FAT_ALLOC_ONE;

    numClusts = (allocPolicy == (uint_t)FAT_ALLOC_ONE) ? 
					1 : pFatDesc->fatGroupSize;

    /* Set initial cluster number to try allocation from.
     */

    if (firstClust > pFatDesc->dos_fat_bad)		/* end of file chain */
        startClust = prevClust;

    else if ((firstClust == 0) && (prevClust == 0))	/* it is a new file */
        startClust = (allocPolicy == (uint_t)FAT_ALLOC_ONE) ?
                     pFatDesc->clustAllocStart : pFatDesc->groupAllocStart;

    if (startClust == 0)
        {
        errnoSet (S_dosFsLib_ILLEGAL_CLUSTER_NUMBER);
        return ERROR;
        }

    /* Try finding a set of clusters starting at or after the initial one.
     *   Continue searching upwards until end of FAT.
     */

    maxClust    = pFatDesc->nFatEnts - 1;
    curClust    = startClust;		/* start from initial cluster number */
    firstClust  = 0;
    contigCount = 0;

    semTake (pFatDesc->allocSem, WAIT_FOREVER);

    for (pass = 0; pass < 2; pass++)
        {
        while (curClust <= maxClust)
            {
            fatEntry = ENTRY_READ (pFd, pFatDesc->dosFatDesc.activeCopyNum,
                                   curClust);

            if ((fatEntry == FAT_CBIO_ERR)&&(pFatHdl->errCode == FAT_CBIO_ERR))
                goto group_alloc_error;

            if (fatEntry == pFatDesc->dos_fat_avail)
                {	/* free space */
                if (contigCount == 0)
                    firstClust = curClust;/* this one will be new start */

                if (++contigCount == numClusts)	/* one more found */
                    goto group_alloc;		/* quit if enough found */
                }
            else	/* allocated space */
                {
                contigCount = 0;
                }

            curClust++;
            } /* while */

        /* 
	 * Try finding a contiguous area starting before the current cluster
         * Note that the new contiguous area could include the initial cluster
         */

        maxClust    = startClust - 1;
        curClust    = DOS_MIN_CLUST;	/* start from lowest cluster number */
        contigCount = 0;
        } /* for */

    if (firstClust == 0)
        {
        errnoSet (S_dosFsLib_DISK_FULL);
        ERR_MSG (1, "!!! DISK FULL !!!\n", 1,2,3,4,5,6);
        goto group_alloc_error;	/* could not find space */
        }

    pFatDesc->groupAllocStart = 0;

    numClusts = 1;

group_alloc:	/* Allocate the found chain */

    if (fat16MarkAlloc (pFd, firstClust, numClusts) != OK)
        goto group_alloc_error;

    semGive (pFatDesc->allocSem);
    DBG_MSG (1, "Allocated %ld clusters starting from cluster %ld\n", 
             contigCount, firstClust,3,4,5,6);

    /*  */

    if (startClust == prevClust)
        {
        /* Add just allocated contiguous section to the file chain */

        if (ENTRY_WRITE (pFd, pFatDesc->dosFatDesc.activeCopyNum,
                        prevClust, firstClust) != OK)
            return ERROR;

        if (firstClust == pFileHdl->contigEndPlus1)
            pFileHdl->contigEndPlus1 = firstClust + numClusts;

        DBG_MSG (1, " ----- Old end %ld -----\n", prevClust,2,3,4,5,6);
        }
    else
        {
        /* Advance start allocation cluster number */

        if (allocPolicy == (uint_t)FAT_ALLOC_ONE)
            pFatDesc->clustAllocStart = firstClust + 1;
        else
            if (pFatDesc->groupAllocStart != 0)
                pFatDesc->groupAllocStart = firstClust + numClusts;

        DBG_MSG (1, " ----- Old start %ld -----\n", startClust,2,3,4,5,6);
        DBG_MSG (1, " ----- New start %ld -----\n", firstClust,2,3,4,5,6);

        pFileHdl->startClust = firstClust;
        pFileHdl->contigEndPlus1  = firstClust + numClusts;
        }

    pFatHdl->nextClust = pFatDesc->dos_fat_eof;

    pFatHdl->lastClust = firstClust + numClusts - 1;

    pFd->curSec = CLUST_TO_SEC (pVolDesc, firstClust);

    pFd->nSec   = numClusts * pVolDesc->secPerClust;

    return OK;

group_alloc_error:
    semGive (pFatDesc->allocSem);
    return ERROR;
    } /* fat16GetNext */

/*******************************************************************************
*
* fat16Truncate - truncate chain starting from cluster
*
* This routine is used when removing files and directories as well as
* when truncating files. It will follow
* a chain of cluster entries in the file allocation table (FAT), freeing each
* as it goes.
*
* RETURNS: OK or ERROR if invalid cluster encountered in chain
*/

LOCAL STATUS fat16Truncate
    (
    FAST DOS_FILE_DESC_ID	pFd,	/* pointer to file descriptor */
    FAST uint32_t		sector,		/* sector to truncate from */
    FAST uint32_t		flag		/* FH_INCLUDE or FH_EXCLUDE */
    )
    {
    FAST DOS_FILE_HDL_ID	pFileHdl = pFd->pFileHdl;
					/* pointer to file handle */
    FAST DOS_VOLUME_DESC_ID	pVolDesc = pFd->pVolDesc;
					/* pointer to volume descriptor */
    FAST MS_FAT_DESC_ID	pFatDesc = (void *) pVolDesc->pFatDesc;
						/* pointer to FAT descriptor */
    FAST uint32_t		curClust;	/* current cluster */
    FAST uint32_t		nextClust;	/* next cluster in chain */
    FAST uint32_t		cluster;	/* cluster to truncate from */

    if (sector == FH_FILE_START)
        {
        cluster = pFileHdl->startClust;
        pFileHdl->contigEndPlus1 = 0;

        if (cluster < DOS_MIN_CLUST)
            {
            errnoSet (S_dosFsLib_INVALID_PARAMETER);	/* ??? */
            return ERROR;
            }
        }
    else
        {
        if (sector < pVolDesc->dataStartSec)
            {
            errnoSet (S_dosFsLib_INVALID_PARAMETER);
            return ERROR;
            }

        cluster = SEC_TO_CLUST (pVolDesc, sector);
        }

    if (cluster >= pFatDesc->nFatEnts)
        {
        errnoSet (S_dosFsLib_INVALID_PARAMETER);	/* ??? */
        return ERROR;
        }

    switch (flag )
        {
        case FH_INCLUDE:
            if ((sector == FH_FILE_START) || 
               (((sector - pVolDesc->dataStartSec) % 
                    pVolDesc->secPerClust) == 0))
                {
                curClust = cluster;
                break;
                }

        case FH_EXCLUDE:
            /* Read cluster to truncate from, including this one */
    
            curClust = ENTRY_READ (pFd, pFatDesc->dosFatDesc.activeCopyNum,
                                   cluster);
    
            if (curClust > pFatDesc->dos_fat_bad)
                return OK;	/* end of file */

            if ((curClust < DOS_MIN_CLUST) || (curClust >= pFatDesc->nFatEnts))
                return ERROR;	/*  */
    
            /* Mark passed cluster as end of file */
    
            if (ENTRY_WRITE (pFd, pFatDesc->dosFatDesc.activeCopyNum,
                             cluster, pFatDesc->dos_fat_eof) != OK)
                return ERROR;	/* disk access error */

            break;

        default:
            {
            errnoSet (S_dosFsLib_INVALID_PARAMETER);
            return ERROR;
            }
        }

    if ( (curClust < pFileHdl->contigEndPlus1) && 
         (curClust >= pFileHdl->startClust) )
        pFileHdl->contigEndPlus1 = curClust;

    if (pFatDesc->groupAllocStart == 0)
        pFatDesc->groupAllocStart = curClust;

   /* Adjust single cluster allocation start point to start of the disk */

    if (pFatDesc->clustAllocStart > curClust)
        pFatDesc->clustAllocStart = curClust;

    /* Free a chain of clusters */

    while ((curClust >= DOS_MIN_CLUST) && (curClust < pFatDesc->nFatEnts))
        {
        /* get next cluster in chain */

        nextClust = ENTRY_READ (pFd, pFatDesc->dosFatDesc.activeCopyNum,
                                curClust);

        /* free current cluster (mark cluster in FAT as available) */

        if (ENTRY_WRITE (pFd, pFatDesc->dosFatDesc.activeCopyNum,
                         curClust, pFatDesc->dos_fat_avail) != OK)
            return ERROR;

        /* Update free clusters counter */

        pFatDesc->fatEntFreeCnt++;

        curClust = nextClust;			/* do next cluster */
        }

    if (curClust <= pFatDesc->dos_fat_bad)	/* If not end of file */
        return ERROR;

    return OK;
    } /* fat16Truncate */

/*******************************************************************************
*
* fat16Seek - seek cluster within file chain
*
* Returns a particular cluster relative to the cluster chain beginning
* which is noted in the file descriptor structure.
* It is used to seek in to a file.
*
* RETURNS: requested position is returned in the file descriptor structure
*/

LOCAL STATUS fat16Seek
    (
    FAST DOS_FILE_DESC_ID pFd,		/* pointer to file descriptor */
    FAST uint32_t		sector,		/* sector to seek from */
    FAST uint32_t		sectOff		/* sector offset within file */
    )
    {
    FAST DOS_VOLUME_DESC_ID	pVolDesc = pFd->pVolDesc;
					/* pointer to volume descriptor */
    FAST MS_FAT_DESC_ID	pFatDesc = (void *) pVolDesc->pFatDesc;
						/* pointer to FAT descriptor */
    FAST uint32_t		nextClust;	/* next cluster number */
    FAST uint32_t		count;		/* count of clusters */
    FAST uint32_t		cluster;	/* cluster to seek from */
    FAST uint32_t		clustOff;	/*  */

    if (sector == FH_FILE_START)
        {
        pFd->fatHdl.nextClust  = 0;
        pFd->fatHdl.lastClust  = 0;
        pFd->fatHdl.cbioCookie = 0;

        pFd->curSec = 0;
        pFd->nSec   = 0;

        cluster = pFd->pFileHdl->startClust;

        if (cluster == 0)
            return OK;

        if (cluster < DOS_MIN_CLUST)
            return ERROR;
        }
    else
        {
        if (sector < pVolDesc->dataStartSec)
            return ERROR;

        /* cluster to seek from */

        cluster = SEC_TO_CLUST (pVolDesc, sector);

        /* offset from start of cluster */

        sectOff += (sector - pVolDesc->dataStartSec) % pVolDesc->secPerClust;
        }

    if (cluster >= pFatDesc->nFatEnts)
        return ERROR;

    clustOff = sectOff / pVolDesc->secPerClust;

    sectOff %= pVolDesc->secPerClust;

    /* Skip contiguous area */

    if ( (cluster < pFd->pFileHdl->contigEndPlus1) && 
         (cluster >= pFd->pFileHdl->startClust) )
        {
        count = min (clustOff, pFd->pFileHdl->contigEndPlus1 - cluster - 1);
        cluster += count;
        clustOff -= count;
        }

    for (count = 0; count < clustOff; count++)
        {
        nextClust = ENTRY_READ (pFd, pFatDesc->dosFatDesc.activeCopyNum,
                                cluster);	/* follow chain */

        /* If end of cluster chain, bad cluster number or disk access error */

        if ((nextClust < DOS_MIN_CLUST) || (nextClust >= pFatDesc->nFatEnts))
            return ERROR;

        cluster = nextClust;			/* do next cluster */
        }

    pFd->fatHdl.nextClust = cluster;

    if (fat16ContigGet (pFd, pFatDesc->fatGroupSize) != OK)
        return ERROR;

    pFd->curSec += sectOff;

    pFd->nSec   -= sectOff;

    return OK;
    } /* fat16Seek */

/*******************************************************************************
*
* fat16NFree - count number of free bytes on disk
*
*
* RETURNS: amount of free space is returned in file descriptor handle
*/

LOCAL fsize_t fat16NFree
    (
    FAST DOS_FILE_DESC_ID	pFd	/* pointer to file descriptor */
    )
    {
    FAST DOS_VOLUME_DESC_ID	pVolDesc = pFd->pVolDesc;
					/* pointer to volume descriptor */
    FAST MS_FAT_DESC_ID	pFatDesc = (void *) pVolDesc->pFatDesc;
						/* pointer to FAT descriptor */
    FAST uint32_t		clustNum;	/* cluster number */
    FAST uint32_t		fatEntry;	/*  */
    FAST uint32_t		freeCount;	/*  */

    freeCount = pFatDesc->fatEntFreeCnt;

    if(freeCount == 0xffffffff)
        {
        freeCount = 0;

        /* Read file allocation table (FAT) to find unused clusters */

        for (clustNum=DOS_MIN_CLUST; clustNum < pFatDesc->nFatEnts; clustNum++)
            {
            fatEntry = ENTRY_READ (pFd, pFatDesc->dosFatDesc.activeCopyNum,
                                   clustNum);

            if ((fatEntry == FAT_CBIO_ERR)&&
                (pFd->fatHdl.errCode == FAT_CBIO_ERR))
                return ERROR;

            if (fatEntry == pFatDesc->dos_fat_avail)
                freeCount++;
            }

        pFatDesc->fatEntFreeCnt = freeCount;
        }

    return ( (((fsize_t)freeCount) * pVolDesc->secPerClust) << 
             pVolDesc->secSizeShift );
    } /* fat16NFree */

/*******************************************************************************
*
* fat16ContigChk - check file chain for contiguity
*
* RETURNS: ERROR if an illegal cluster number is encountered
*/

LOCAL STATUS fat16ContigChk
    (
    FAST DOS_FILE_DESC_ID	pFd	/* pointer to file descriptor */
    )
    {
    FAST DOS_FILE_HDL_ID	pFileHdl = pFd->pFileHdl;
					/* pointer to file handle */
    FAST MS_FAT_DESC_ID	pFatDesc = (void *) pFd->pVolDesc->pFatDesc;
					/* pointer to FAT handler descriptor */
    FAST uint32_t		nextClust;	/* next cluster number */
    FAST uint32_t		cluster;	/* cluster number */

    cluster = pFileHdl->startClust;

    if (cluster == 0)
        return OK;

    if ((cluster < DOS_MIN_CLUST) || (cluster >= pFatDesc->nFatEnts))
        {
        errnoSet (S_dosFsLib_ILLEGAL_CLUSTER_NUMBER);
        return ERROR;
        }

    FOREVER
        {
        nextClust = ENTRY_READ (pFd, pFatDesc->dosFatDesc.activeCopyNum,
                                cluster);	/* follow chain */

        if (nextClust != ++cluster)
            break;				/* end of contiguous area */
        }

    if (pFd->fatHdl.errCode == FAT_CBIO_ERR)
        return ERROR;

    pFileHdl->contigEndPlus1 = cluster;

    if (nextClust > pFatDesc->dos_fat_bad)	/* end of file chain */
        return OK;

    if ((nextClust < DOS_MIN_CLUST) || (nextClust >= pFatDesc->nFatEnts))
	{
        errnoSet (S_dosFsLib_ILLEGAL_CLUSTER_NUMBER);
	}

    return ERROR;
    } /* fat16ContigChk */

/*******************************************************************************
*
* fat16MaxContigClustersGet - return size of largest contiguous space on disk
* 
* RETURNS: max contiguous space in clusters
*/ 
 
LOCAL size_t fat16MaxContigClustersGet
    (
    FAST DOS_FILE_DESC_ID	pFd,		/* pointer to file descriptor */
    FAST uint32_t *		pMaxStart	/* were to return start clust */
    )
    {
    FAST MS_FAT_DESC_ID	pFatDesc = (void *) pFd->pVolDesc->pFatDesc;
					/* pointer to FAT handler descriptor */
    FAST uint32_t		fatEntry;	/*  */
    FAST uint32_t		curClust;	/*  */
    FAST uint32_t		firstContig;	/* 1st clust. of contig area */
    FAST uint32_t		contigCount;	/* count of fit clusters */
    FAST uint32_t		maxStart;	/*  */
    FAST uint32_t		maxCount;	/*  */

    firstContig = DOS_MIN_CLUST;
    contigCount = 0;
    maxStart    = 0;
    maxCount    = 0;

    for (curClust = DOS_MIN_CLUST; curClust < pFatDesc->nFatEnts; curClust++)
        {
        fatEntry = ENTRY_READ (pFd, pFatDesc->dosFatDesc.activeCopyNum,
                               curClust);

        if ((fatEntry == FAT_CBIO_ERR)&&(pFd->fatHdl.errCode == FAT_CBIO_ERR))
            return 0;

        if (fatEntry == pFatDesc->dos_fat_avail)
            {	/* free space */
            contigCount++;	/* one more found */
            }
        else
            {
            /* Check for maximum */

            if (contigCount > maxCount)
                {
                maxStart = firstContig;
                maxCount = contigCount;
                }

            firstContig = curClust + 1;	/* next one will be new start */

            contigCount = 0;
            }
        } /* for */

    if (contigCount > maxCount)
        {
        maxStart = firstContig;
        maxCount = contigCount;
        }

    *pMaxStart = maxStart;
    return (maxCount);
    } /* fat16MaxContigClustersGet */

/*******************************************************************************
*
* fat16ContigAlloc - allocate <numSect> contiguous chain
*
* This function employs the Best Fit algorithm to locate a
* contiguous fragment of space for the requested size
*
* RETURNS: ERROR if requested space is not available
*/

LOCAL STATUS fat16ContigAlloc
    (
    FAST DOS_FILE_DESC_ID pFd,		/* pointer to file descriptor */
    FAST uint32_t		number		/* number of sectors needed */
    )
    {
    FAST DOS_FILE_HDL_ID	pFileHdl = pFd->pFileHdl;
					/* pointer to file handle */
    FAST DOS_VOLUME_DESC_ID	pVolDesc = pFd->pVolDesc;
					/* pointer to volume descriptor */
    FAST MS_FAT_DESC_ID	pFatDesc = (void *) pVolDesc->pFatDesc;
					/* pointer to FAT handler descriptor */
    FAST uint32_t		fatEntry;	/*  */
    FAST uint32_t		curClust;	/*  */
    FAST uint32_t		firstContig;	/* 1st clust. of contig area */
    FAST uint32_t		contigCount;	/* count of fit clusters */
    FAST uint32_t		bestCount;	/*  */
         uint32_t		bestStart;	/*  */

    if (number == (uint32_t)CONTIG_MAX)
        {
        semTake (pFatDesc->allocSem, WAIT_FOREVER);

        number = fat16MaxContigClustersGet (pFd, &bestStart);
        if (number == 0)
            {
            errnoSet (S_dosFsLib_DISK_FULL);
            goto contig_alloc_error;
            }

        goto contig_alloc;
        }

    /* Convert number of sectors needed to number of clusters */

    number = (number + pVolDesc->secPerClust - 1) / pVolDesc->secPerClust;

    firstContig = DOS_MIN_CLUST;
    contigCount = 0;
    bestStart   = 0;
    bestCount   = pFatDesc->nFatEnts;

    semTake (pFatDesc->allocSem, WAIT_FOREVER);

    for (curClust = DOS_MIN_CLUST; 
    /* firstContig <= (pFatDesc->nFatEnts - number); ??? for 1st fit ??? */
         curClust < (pFatDesc->nFatEnts - number);	
         curClust++)
        {
        fatEntry = ENTRY_READ (pFd, pFatDesc->dosFatDesc.activeCopyNum,
                               curClust);

        if ((fatEntry == FAT_CBIO_ERR)&&(pFd->fatHdl.errCode == FAT_CBIO_ERR))
            goto contig_alloc_error;

#if FALSE	/* best fit */

        if (fatEntry == pFatDesc->dos_fat_avail)
            {	/* free space */
            contigCount++;	/* one more found */
            }
        else
            {
            /* Check for best fit */

            if ((contigCount >= number) && (contigCount < bestCount))
                {		/* most close from above */
                bestStart = firstContig;
                bestCount = contigCount;

                if (contigCount == number)
                    break;	/* fit exactly */
                }

            firstContig = curClust + 1;	/* next one will be new start */

            contigCount = 0;
            }

#else	/* 1st fit */

        if (fatEntry == pFatDesc->dos_fat_avail)
            {	/* free space */
            if (++contigCount == number)	/* one more found */
                {
                bestStart = firstContig;	/* ??? temporary solution */
                bestCount = contigCount;	/* ??? temporary solution */
                goto contig_alloc;		/* quit if enough found */
                }
            }
        else	/* allocated space */
            {
            firstContig = curClust + 1;	/* next one will be new start */
            contigCount = 0;
            }
#endif /* FALSE */
        } /* for */

    if (bestStart == 0)
        {
        if (contigCount < number)
            {
            errnoSet (S_dosFsLib_NO_CONTIG_SPACE);
            goto contig_alloc_error;
            }

        bestStart = firstContig;
        }

contig_alloc:

    if (fat16MarkAlloc (pFd, bestStart, number) != OK)
        goto contig_alloc_error;

    semGive (pFatDesc->allocSem);

    DBG_MSG (1, "Allocated %ld clusters starting from cluster %ld\n", 
             number, bestStart,3,4,5,6);

    pFileHdl->startClust = bestStart;
    pFileHdl->contigEndPlus1 = bestStart + number;

    return OK;

contig_alloc_error:
    semGive (pFatDesc->allocSem);
    return ERROR;
    } /* fat16ContigAlloc */

/*******************************************************************************
*
* fat16MaxContigSectors - max free contiguous chain length in sectors
* 
* RETURNS: size of the max contiguous free space in sectors
*/ 
 
static size_t fat16MaxContigSectors
    (
    FAST DOS_FILE_DESC_ID	pFd	/* pointer to file descriptor */
    )
    {
    uint32_t	maxStart;	

    return (fat16MaxContigClustersGet (pFd, &maxStart) * pFd->pVolDesc->secPerClust);
    } /* fat16MaxContigSectors */

/*******************************************************************************
*
* fat16Show - display handler specific data
*
* RETURNS: N/A.
*/

LOCAL void fat16Show 
    (
    DOS_VOLUME_DESC_ID	pVolDesc	/* pointer to volume descriptor */
    )
    {
    MS_FAT_DESC_ID	pFatDesc = (MS_FAT_DESC_ID) pVolDesc->pFatDesc;
    DOS_FILE_DESC	fileDesc = { 0 }; /* pointer to file descriptor */

    fileDesc.pVolDesc = pVolDesc;

    printf ("FAT handler information:\n");
    printf ("------------------------\n");

    printf      (" - allocation group size:	%ld clusters\n", 
	pFatDesc->fatGroupSize);

    print64Fine (" - free space on volume:	", fat16NFree (&fileDesc), 
                " bytes\n", 10);
    } /* fat16Show */

/*******************************************************************************
*
* fat16ClustValueSet - set value to a particular FAT entry
*
* RETURNS: OK or ERROR
*/

STATUS fat16ClustValueSet
    (
    DOS_FILE_DESC_ID	pFd,		/* pointer to file descriptor */
    uint32_t		copyNum,	/* fat copy number */
    uint32_t		clustNum,	/* cluster number to check */
    uint32_t		value,
    uint32_t		nextClust
    )
    {
    DOS_VOLUME_DESC_ID	pVolDesc = pFd->pVolDesc;
    MS_FAT_DESC_ID	pFatDesc = (void *) pVolDesc->pFatDesc;
    
    switch (value)
        {
        case DOS_FAT_AVAIL:
            value = pFatDesc->dos_fat_avail;
            break;

        case DOS_FAT_EOF:
            value = pFatDesc->dos_fat_eof;
            break;

        case DOS_FAT_BAD:
            assert (FALSE);
            value = pFatDesc->dos_fat_bad;
            break;

        case DOS_FAT_RESERV:
            assert (FALSE);
            value = pFatDesc->dos_fat_reserv;
            break;

        case DOS_FAT_ALLOC:
            assert ((nextClust >= DOS_MIN_CLUST) &&
                    (nextClust < pFd->pVolDesc->nFatEnts));
            value = nextClust;
            break;
    	
     	case DOS_FAT_RAW:
    	    value = nextClust;
    	    break;

     	case DOS_FAT_SET_ALL:
    	    {
    	    CBIO_DEV_ID	pCbio = pVolDesc->pCbio;
    	    cookie_t	cookie = (cookie_t) NULL;
    	    uint8_t	wBuf [64];

    	    /* fill one sector (<value> = sector number) */

    	    value = pFatDesc->fatStartSec + (copyNum * pVolDesc->secPerFat);

    	    bfill ((char *)wBuf, sizeof (wBuf), nextClust );

    	    for ( clustNum = 0;
		  clustNum < pFd->pVolDesc->bytesPerSec;
                  clustNum += sizeof (wBuf) )
    	    	{
    	    	if (cbioBytesRW (pCbio, value, clustNum, (addr_t)wBuf,
                                         sizeof (wBuf), CBIO_WRITE,
                                         &cookie) != OK)
    	    	    {
    	    	    return ERROR;
    	    	    }
    	    	}

    	    /* copy first sector to other FAT sectors */

    	    for ( nextClust = value++;
                  value < nextClust + pVolDesc->secPerFat;
    	    	  value ++ )
    	    	{
    	    	if (cbioBlkCopy (pCbio, nextClust, value, 1) != OK)
    	    	    return ERROR;
    	    	}

    	    return (OK);
    	    }

        case DOS_FAT_INVAL:
        default:
            assert (FALSE);
        }

    return ENTRY_WRITE (pFd, copyNum, clustNum, value);
    } /* fat16ClustValueSet */

/*******************************************************************************
*
* fat16ClustValueGet - get value corresponding to this cluster number in FAT
*
* Get the value of a particular FAT entry, resulting in the
* number of the next cluster in chain, or one of the following:
*
* DOS_FAT_AVAIL  - available cluster
* DOS_FAT_EOF    - end of file's cluster chain
* DOS_FAT_BAD    - bad cluster (damaged media)
* DOS_FAT_ALLOC  - allocated cluster
* DOS_FAT_INVAL  - invalid cluster (illegal value)
* DOS_FAT_RESERV - reserved cluster
*
* RETURNS: cluster number or special value above
*/

uint32_t fat16ClustValueGet 
    (
    DOS_FILE_DESC_ID	pFd,		/* pointer to file descriptor */
    uint32_t		copyNum,	/* fat copy number */
    uint32_t		clustNum,	/* cluster number to check */
    uint32_t *		pNextClust	/*  */
    )
    {
    MS_FAT_DESC_ID	pFatDesc = (void *) pFd->pVolDesc->pFatDesc;

    clustNum &= 0x0fffffff;	/* high 4 bits in FAT32 are reserved */

    if (clustNum == 0)
        return DOS_FAT_AVAIL;

    if (clustNum > pFatDesc->dos_fat_bad)
        return DOS_FAT_EOF;

    if (clustNum == pFatDesc->dos_fat_bad)
        return DOS_FAT_BAD;

    if (clustNum >= pFatDesc->dos_fat_reserv)
        return DOS_FAT_RESERV;

    if ((clustNum < DOS_MIN_CLUST) || (clustNum >= pFd->pVolDesc->nFatEnts))
        return DOS_FAT_INVAL;

    *pNextClust = ENTRY_READ (pFd, copyNum, clustNum);

    if (pFd->fatHdl.errCode == FAT_CBIO_ERR)
        return DOS_FAT_ERROR;

    return DOS_FAT_ALLOC;
    } /* fat16ClustValueGet */

/*******************************************************************************
*
* fat16VolUnmount - unmount volume
*
* This routine frees all resources, that were allocated for the volume.
*
* RETURNS: N/A
*/

LOCAL void fat16VolUnmount 
    (
    DOS_VOLUME_DESC_ID	pVolDesc	/*  */
    )
    {
    return;
    } /* fat16VolUnmount */

/*******************************************************************************
*
* fat16VolMount - mount volume
*
* Initialize a new volume
*
* RETURNS: ERROR if FAT is illegal
*/

STATUS fat16VolMount 
    (
    DOS_VOLUME_DESC_ID	pVolDesc,	/*  */
    void *		arg		/* unused */
    )
    {
    FAST CBIO_DEV_ID	pCbio = pVolDesc->pCbio;
						/* pointer to CBIO device */
    MS_FAT_DESC_ID	pFat16Desc;
    DOS_FILE_DESC	fileDesc = { 0 };	/* file descriptor */
    uint8_t		fsinfoBuf [8];		/* FSINFO sector data */

    /*
     * if previous volume had alternative FAT structure,
     * unmount previous FAT handler and
     * allocate FAT handler descriptor
     */

    if (pVolDesc->pFatDesc != NULL &&
        pVolDesc->pFatDesc->volUnmount != fat16VolUnmount &&
        pVolDesc->pFatDesc->volUnmount != NULL)
        {
        /* Unmount previous FAT handler */

        pVolDesc->pFatDesc->volUnmount( pVolDesc );
        }

    /* Allocate FAT handler descriptor */

    pVolDesc->pFatDesc = 
	KHEAP_REALLOC((char *)pVolDesc->pFatDesc, sizeof( *pFat16Desc));

    if (pVolDesc->pFatDesc == NULL)
        return ERROR;

    pFat16Desc = (void *)pVolDesc->pFatDesc;
    bzero( (void *)pFat16Desc, sizeof( *pFat16Desc) ); 

    fileDesc.pVolDesc = pVolDesc;	/* for fat16NFree() */
 
    /* Number of FAT entries = count of clusters available for files + 2 */

    pVolDesc->nFatEnts = pFat16Desc->nFatEnts = 
			( ((pVolDesc->totalSec - pVolDesc->dataStartSec) / 
				pVolDesc->secPerClust) + DOS_MIN_CLUST );

    if (pVolDesc->fatType == FAT32)
        {
        /* activeFatStart ??? */

        if (cbioBytesRW (pCbio, FSINFO_SEC_NUM, FSINFO_FREE_CLUSTS, 
		(addr_t)fsinfoBuf, sizeof (fsinfoBuf), CBIO_READ, NULL) != OK)
            goto mount_error;

        /* Fill free FAT entries count */

        pFat16Desc->fatEntFreeCnt   = DISK_TO_VX_32 (&fsinfoBuf[0]);

        /* Fill start cluster for cluster groups allocation */

        pFat16Desc->groupAllocStart = DISK_TO_VX_32 (&fsinfoBuf[4]);

        if ( (pFat16Desc->groupAllocStart < DOS_MIN_CLUST) ||
             (pFat16Desc->groupAllocStart >= pFat16Desc->nFatEnts) )
            pFat16Desc->groupAllocStart = DOS_MIN_CLUST;

        pFat16Desc->entryRead       = fat32EntRead;
        pFat16Desc->entryWrite      = fat32EntWrite;
        pFat16Desc->dos_fat_reserv  = 0x0ffffff0;
        pFat16Desc->dos_fat_bad     = 0x0ffffff7;
        pFat16Desc->dos_fat_eof     = 0x0fffffff;
        }
    else
        {
        /* activeFatStart ??? */

        /* -1 in fatEntFreeCnt cause fat16NFree() to fill this field */

        pFat16Desc->fatEntFreeCnt = -1;

        /* Fill cluster groups allocation start cluster */

        pFat16Desc->groupAllocStart = DOS_MIN_CLUST;
 
        if (pVolDesc->fatType == FAT16)
            {
            pFat16Desc->entryRead       = fat16EntRead;
            pFat16Desc->entryWrite      = fat16EntWrite;
            pFat16Desc->dos_fat_reserv  = 0xfff0;
            pFat16Desc->dos_fat_bad     = 0xfff7;
            pFat16Desc->dos_fat_eof     = 0xffff;
            }
        else if (pVolDesc->fatType == FAT12)
            {
            pFat16Desc->entryRead       = fat12EntRead;
            pFat16Desc->entryWrite      = fat12EntWrite;
            pFat16Desc->dos_fat_reserv  = 0xff0;
            pFat16Desc->dos_fat_bad     = 0xff7;
            pFat16Desc->dos_fat_eof     = 0xfff;
            }
        else
            goto mount_error;
        }

    pFat16Desc->dosFatDesc.volUnmount  = fat16VolUnmount;
    pFat16Desc->dosFatDesc.getNext     = fat16GetNext;
    pFat16Desc->dosFatDesc.contigChk   = fat16ContigChk;
    pFat16Desc->dosFatDesc.show        = fat16Show;
    pFat16Desc->dosFatDesc.truncate    = fat16Truncate;   /*FIOTRUNC*/
    pFat16Desc->dosFatDesc.seek        = fat16Seek;       /*FIOSEEK*/
    pFat16Desc->dosFatDesc.contigAlloc = fat16ContigAlloc;/*FIOCONTIG*/
    pFat16Desc->dosFatDesc.maxContig   
                                 = fat16MaxContigSectors; /*FIONCONTIG*/
    pFat16Desc->dosFatDesc.nFree       = fat16NFree;      /*FIONFREE*/
    pFat16Desc->dosFatDesc.flush       = fat16MirrorSect; /*FIOFLUSH*/
    pFat16Desc->dosFatDesc.syncToggle  = fat16SyncToggle;
    pFat16Desc->dosFatDesc.clustValueSet = fat16ClustValueSet;
    pFat16Desc->dosFatDesc.clustValueGet = fat16ClustValueGet;

    pFat16Desc->fatStartSec     = pVolDesc->nReservedSecs;

    /* current version uses only fat copy 0 as active */
    
    pFat16Desc->dosFatDesc.activeCopyNum	= 0;

    /* Number of clusters in allocation group = ??? 
     * must be calculated in future
     */

    pFat16Desc->fatGroupSize    = pFat16Desc->nFatEnts / fatClugFac + 1;

    pFat16Desc->dos_fat_avail   = 0x00000000;

    pFat16Desc->syncEnabled = TRUE;	/* enable FAT copies mirroring */

    pFat16Desc->allocSem = semMCreate (ALLOC_SEM_OPTIONS);

    if (NULL == pFat16Desc->allocSem)
	{
        goto mount_error;
	}

    pFat16Desc->clustAllocStart = DOS_MIN_CLUST;
    
    fat16NFree (&fileDesc);	/* fill 'fatEntFreeCnt' if it equals -1 */

    return OK;

mount_error:
    fat16VolUnmount (pVolDesc);
    return ERROR;
    } /* fat16VolMount */

/*****************************************************************************
*
* dosFsFatInit - init handler and install it into dosFsLib
*
* This function must be called to install the FAT handler,
* which is mandatory, once during system initialization.
*
* RETURNS: OK or ERROR if failed to install
*/
STATUS dosFsFatInit ( void )
    {
    DOS_HDLR_DESC	hdlr;

    hdlr.id       = DOS_FATALL_HDLR_ID;
    hdlr.mountRtn = fat16VolMount;
    hdlr.arg      = NULL;

    return dosFsHdlrInstall (dosFatHdlrsList, &hdlr);
    } /* dosFsFatInit() */
