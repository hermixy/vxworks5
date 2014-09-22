/* dosFsFatP.h - DOS file system FAT module header file*/
 
/* Copyright 1999 Wind River Systems, Inc. */
 
/* 
modification history 
-------------------- 
01f,20sep01,jkf  SPR#69031, common code for both AE & 5.x.
01e,29feb00,jkf  T3 changes
01d,31jul99,jkf  T2 merge, tidiness & spelling.
01c,23sep98,vld  field <syncEnabled> added to structure MS_FAT_DESC
01b,20sep98,mjc  descriptor structure name changed from DOS_FAT16_DESC
		 to MS_FAT_DESC
01a,23feb98,mjc  written, preliminary 
*/

#ifndef __INCdosFath
#define __INCdosFath

#ifdef __cplusplus
extern "C" {
#endif

#include "private/dosFsLibP.h"


/* defines */

/* conversions of sector <-> cluster numbers */

#define CLUST_TO_SEC( pVolDesc, clustNum )                                  \
                ( ((clustNum) - DOS_MIN_CLUST) * (pVolDesc)->secPerClust +  \
                  (pVolDesc)->dataStartSec )

#define SEC_TO_CLUST( pVolDesc, secNum )                                    \
                                ( ((secNum) -  (pVolDesc)->dataStartSec) /  \
                                  (pVolDesc)->secPerClust + DOS_MIN_CLUST )


/* typedefs */

typedef enum fat_rw	{FAT_READ, FAT_WRITE_GROUP, FAT_WRITE_CLUST} FAT_RW;

typedef struct
    {
    DOS_FAT_DESC	dosFatDesc;	/* generic DOS FAT descriptor */

    uint32_t		(*entryRead)
				(DOS_FILE_DESC_ID pFd, uint32_t copyNum,
				uint32_t entry);
    STATUS		(*entryWrite)
				(DOS_FILE_DESC_ID pFd, uint32_t copyNum,
				uint32_t entry,
				uint32_t value);
    uint32_t		fatStartSec;	/* FAT starting sector number */
    uint32_t		nFatEnts;	/* number of entries in FAT */
    uint32_t		fatEntFreeCnt;	/* count of free clusters */
    uint32_t		dos_fat_avail;	/* cluster is available: 0x0000 */
    uint32_t            dos_fat_reserv;	/* reserved cluster marker: 0xff(f)0 */
    uint32_t		dos_fat_bad;	/* bad cluster marker: 0xff(f)7 */
    uint32_t		dos_fat_eof;	/* end-of-file marker: 0xff(f)f */
    uint32_t		fatGroupSize;	/* size clusters */
    uint32_t		groupAllocStart;/* initialize to DOS_MIN_CLUST */
    uint32_t		clustAllocStart;/* initialize to DOS_MIN_CLUST */
    BOOL		syncEnabled;	/* FAT copies mirroring enabling */
    SEM_ID		allocSem;	/* semaphore to guard allocations */
    } MS_FAT_DESC;

typedef MS_FAT_DESC * MS_FAT_DESC_ID;


/* defines */


#ifdef __cplusplus
}
#endif

#endif /* __INCdosFath */
