/* usrFdiskPartLib.h - FDISK partition support header */

/* Copyright 2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01a,20sep01,jkf   written
*/

#ifndef __INCusrFdiskPartLibh
#define __INCusrFdiskPartLibh

#ifdef __cplusplus
extern "C" {
#endif

#include "cbioLib.h"       /* for CBIO_DEV_ID */
#include "dpartCbio.h"     /* for PART_TABLE_ENTRY */

/* function declarations */

#if defined(__STDC__) || defined(__cplusplus)

extern STATUS usrFdiskPartRead
    (
    CBIO_DEV_ID cDev,            /* device from which to read blocks */
    PART_TABLE_ENTRY *pPartTab,  /* table where to fill results */
    int nPart                    /* # of entries in <pPartTable> */
    );

extern STATUS usrFdiskPartCreate
    (
    CBIO_DEV_ID cDev, 	/* device representing the entire disk */
    int		nPart,	/* how many partitions needed, default=1, max=4 */
    int		size1,	/* space percentage for second partition */
    int		size2,	/* space percentage for third partition */
    int		size3	/* space percentage for fourth partition */
    );

#else

STATUS usrFdiskPartRead ();
STATUS usrFdiskPartCreate ();

#endif	/* __STDC__ */

/* macro's */

/* partition table structure offsets */

#define PART_SIG_ADRS           0x1fe   /* dos partition signature  */
#define PART_SIG_MSB            0x55    /* msb of the partition sig */
#define PART_SIG_LSB            0xaa    /* lsb of the partition sig */
#define PART_IS_BOOTABLE        0x80    /* a dos bootable partition */
#define PART_NOT_BOOTABLE       0x00    /* not a bootable partition */
#define PART_TYPE_DOS4          0x06    /* dos 16b FAT, 32b secnum  */
#define PART_TYPE_DOSEXT        0x05    /* msdos extended partition */
#define PART_TYPE_DOS3          0x04    /* dos 16b FAT, 16b secnum  */
#define PART_TYPE_DOS12         0x01    /* dos 12b FAT, 32b secnum  */
#define PART_TYPE_DOS32         0x0b    /* dos 32b FAT, 32b secnum  */
#define PART_TYPE_DOS32X        0x0c    /* dos 32b FAT, 32b secnum  */
#define PART_TYPE_WIN95_D4      0x0e    /* Win95 dosfs  16bf 32bs   */
#define PART_TYPE_WIN95_EXT     0x0f    /* Win95 extended partition */

#define BOOT_TYPE_OFFSET    0x0   /* boot type                      */
#define STARTSEC_HD_OFFSET  0x1   /* beginning sector head value    */
#define STARTSEC_SEC_OFFSET 0x2   /* beginning sector               */
#define STARTSEC_CYL_OFFSET 0x3   /* beginning cylinder             */
#define SYSTYPE_OFFSET      0x4   /* system indicator               */
#define ENDSEC_HD_OFFSET    0x5   /* ending sector head value       */
#define ENDSEC_SEC_OFFSET   0x6   /* ending sector                  */
#define ENDSEC_CYL_OFFSET   0x7   /* ending cylinder                */
#define NSECTORS_OFFSET     0x8   /* sector offset from reference   */
#define NSECTORS_TOTAL      0xc   /* number of sectors in part      */

#ifdef __cplusplus
}
#endif

#endif /* __INCusrFdiskPartLibh */
