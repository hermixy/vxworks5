/* usrFdiskPartLib.c - FDISK-style partition handler */

/* Copyright 1997-2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
02p,10dec01,jkf  fixed typo in 02n checkin that broke 4 partition systems.
02o,09nov01,jkf  SPR#71633, dont set errno when DevCreate is called w/BLK_DEV
02n,04oct01,rip  SPR#66973, computation overflows with large capacity disks
                 SPR#30850, CBIO_RESET may be needed on device's first reading
02m,20sep01,jkf  SPR#69031, common code for both AE & 5.x.
02l,03mar01,jkf  SPR#62415, use sector size from BLK_DEV.
02k,26dec99,jkf  T3 KHEAP_ALLOC, fixed usrFdiskPartCreate to work w/BLK_DEV
02j,31aug99,jkf  changes for new CBIO API.
02i,31jul99,jkf  changed usrFdiskPartCreate. SPR#28285
02h,31jul99,jkf  reentrancy overhaul. SPR#28278
02g,31jul99,jkf  fixed blind resetting cbio in show routine. SPR#28279
02f,31jul99,jkf  fixed bugs introduced in 02a adaptation: 
                 useFdiskPartParse offset calculation fixed, SPR#28280
02e,31jul99,jkf  T2 merge, tidiness & spelling.
02d,07dec98,lrn  partition table creation routine (SPR#21977), terse Show
02c,15sep98,lrn  enabled SHOW by default, assume non-partitioned disk 
		 if all partitions are nil
02b,02jul98,lrn  DosFs 2.0 pre-release
02a,21jun98,lrn  adopted from rev 01c by jkf.
01a,01sep97,jkf  adapted from dosPartLibAta.c.
*/


/*
DESCRIPTION:
This module is provided is source code to accommodate various
customizations of partition table handling, resulting from
variations in the partition table format in a particular 
configuration.
It is intended for use with dpartCbio partition manager.

This code supports both mounting MSDOS file systems and
displaying partition tables written by MSDOS FDISK.exe or
by any other MSDOS FDISK.exe compatible partitioning software.  

The first partition table is contained within a hard drives
Master Boot Record (MBR) sector, which is defined as sector
one, cylinder zero, head zero or logical block address zero.

The mounting and displaying routines within this code will 
first parse the MBR partition tables entries (defined below)
and also recursively parse any "extended" partition tables,
which may reside within another sector further into the hard 
disk.   MSDOS file systems within extended partitions are known
to those familiar with the MSDOS FDISK.exe utility as "Logical 
drives within the extended partition".

Here is a picture showing the layout of a single disk containing
multiple MSDOS file systems:

.CS
  +---------------------------------------------------------+
  |<---------------------The entire disk------------------->|
  |M                                                        |
  |B<---C:--->                                              |
  |R           /---- First extended partition--------------\|
  |           E<---D:---><-Rest of the ext part------------>|
  |P          x                                             |
  |A          t          E<---E:--->E<Rest of the ext part->|
  |R                     x          x                       |
  |T                     t          t<---------F:---------->|
  +---------------------------------------------------------+
  (Ext == extended partition sector)
  C: is a primary partiion
  D:, E:, and F: are logical drives within the extended partition.
.CE

A MS-DOS partition table resides within one sector on a hard
disk.  There is always one in the first sector of a hard disk
partitioned with FDISK.exe.  There first partition table may
contain references to "extended" partition tables residing on
other sectors if there are multiple partitions.  The first 
sector of the disk is the starting point.  Partition tables
are of the format:

.CS
Offset from     
the beginning 
of the sector          Description
-------------          -------------------------
   0x1be               Partition 1 table entry  (16 bytes)
   0x1ce               Partition 2 table entry  (16 bytes)
   0x1de               Partition 3 table entry  (16 bytes)
   0x1ee               Partition 4 table entry  (16 bytes)
   0x1fe               Signature  (0x55aa, 2 bytes)
.CE

Individual MSDOS partition table entries are of the format:
.CS
Offset   Size      Description
------   ----      ------------------------------
 0x0     8 bits    boot type
 0x1     8 bits    beginning sector head value
 0x2     8 bits    beginning sector (2 high bits of cylinder#)
 0x3     8 bits    beginning cylinder# (low order bits of cylinder#)
 0x4     8 bits    system indicator
 0x5     8 bits    ending sector head value
 0x6     8 bits    ending sector (2 high bits of cylinder#)
 0x7     8 bits    ending cylinder# (low order bits of cylinder#)
 0x8    32 bits    number of sectors preceding the partition
 0xc    32 bits    number of sectors in the partition
.CE

The Cylinder, Head and Sector values herein are not used,
instead the 32-bit partition offset and size (also known as LBA
addresses) are used exclusively to determine partition geometry.

If a non-partitioned disk is detected, in which case the 0'th block
is a DosFs boot block rather then an MBR, the entire disk will be
configured as partition 0, so that disks formatted with VxWorks and
disks formatted on MS-DOS or Windows can be accepted interchangeably.

The usrFdiskPartCreate() will create a partition table with up to four
partitions, which can be later used with usrFdiskPartRead() and
dpartCbio to manage a partitioned disk on VxWorks.

However, it can not be guaranteed that this partition table can be used
on another system due to several BIOS specific paramaters in the boot
area.  If interchangeability via removable disks is a requirement, 
partition tables should be created and volumes should be formatted 
on the other system with which the data is to be interchanged/

CAUTION
The partition decode function is recursive, up to the maximum
number of partitions expected, which is no more then 24.

Sufficient stack space needs to be provided via taskSpawn() to 
accommodate the recursion level.

SEE ALSO: dpartCbio
*/


/* includes */

#include "vxWorks.h"
#include "private/dosFsVerP.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "tickLib.h"
#include "dosFsLib.h"
#include "usrFdiskPartLib.h"
#include "private/dosFsLibP.h"	/* for byte swapping macros */
#include "dpartCbio.h"
#include "cbioLib.h"

/* defines */

/* may be undefine the following to conserve memory space */

#define	INCLUDE_PART_SHOW	/* include the show function by default */

/* declarations */

/* locals */

LOCAL STATUS useFdiskPartParse
    (
    CBIO_DEV_ID dev,        	/* device from which to read blocks */
    CBIO_PARAMS * pCbioParams,
    PART_TABLE_ENTRY *pPartTab, /* table where to fill results */
    int nPart,               	/* # of entries in <pPartTable> */
    ULONG startBlock,		/* where to expect part table */
    ULONG extStartBlock		/* Offset to extended partition */
    );

#ifdef INCLUDE_PART_SHOW

STATUS usrFdiskPartShow
    (
    CBIO_DEV_ID cbioDev,     /* device CBIO handle */
    block_t extPartOffset,    /* user should pass zero */
    block_t currentOffset,    /* user should pass zero */
    int extPartLevel          /* user should pass zero */
    );

LOCAL const struct partType /* Some partition type values & names. */
    {                       /* Only MSDOS are used in this code.   */
    const UINT8 partTypeNum;      
    const char *partTypeName;    
    } partNames[] = 
        {
        {0x00, "Empty (NULL) Partition"},
        {0x01, "MSDOS Partition 12-bit FAT"},   
        {0x02, "XENIX / (slash) Partition"},        
        {0x03, "XENIX /usr Partition"}, 
        {0x04, "MSDOS 16-bit FAT <32M Partition"},  
        {0x05, "MSDOS Extended Partition"}, 
        {0x06, "MSDOS 16-bit FAT >=32M Partition"},
        {0x07, "HPFS / NTFS Partition"},
        {0x08, "AIX boot or SplitDrive Partition"},
        {0x09, "AIX data or Coherent Partition"},
        {0x0a, "OS/2 Boot Manager Partition"},
        {0x0b, "Win95 FAT32 Partition"},
        {0x0c, "Win95 FAT32 (LBA) Partition"},
        {0x0e, "Win95 FAT16 (LBA) Partition"},
        {0x0f, "Win95 Extended (LBA) Partition"},
        {0x10, "OPUS Partition"},
        {0x11, "Hidden DOS FAT12 Partition"},
        {0x12, "Compaq diagnostics Partition"},
        {0x14, "Hidden DOS FAT16 Partition"},
        {0x16, "Hidden DOS FAT16 (big) Partition"},
        {0x17, "Hidden HPFS/NTFS Partition"},
        {0x18, "AST Windows swapfile Partition"},
        {0x24, "NEC DOS Partition"},
        {0x3c, "PartitionMagic recovery Partition"},
        {0x40, "Venix 80286 Partition"},
        {0x41, "Linux/MINIX (shared with DRDOS) Partition"},
        {0x42, "SFS or Linux swap part (shared with DRDOS)"},
        {0x43, "Linux native (shared with DRDOS) Partition"},
        {0x50, "DM (disk manager) Partition"},
        {0x51, "DM6 Aux1 (or Novell) Partition"},
        {0x52, "CP/M or Microport SysV/AT Partition"},
        {0x53, "DM6 Aux3 Partition"},
        {0x54, "DM6 Partition"},
        {0x55, "EZ-Drive (disk manager) Partition"},
        {0x56, "Golden Bow (disk manager) Partition"},
        {0x5c, "Priam Edisk (disk manager) Partition"},
        {0x61, "SpeedStor Partition"},
        {0x63, "GNU HURD or Mach or Sys V/386 (ISC UNIX)"},
        {0x64, "Novell Netware 286 Partition"},
        {0x65, "Novell Netware 386 Partition"},
        {0x70, "DiskSecure Multi-Boot Partition"},
        {0x75, "PC/IX Partition"},
        {0x77, "QNX4.x Partition"},
        {0x78, "QNX4.x 2nd part Partition"},
        {0x79, "QNX4.x 3rd part Partition"},
        {0x80, "MINIX until 1.4a Partition"},
        {0x81, "MINIX / old Linux Partition"},
        {0x82, "Linux swap Partition"},
        {0x83, "Linux native Partition"},
        {0x84, "OS/2 hidden C: drive Partition"},
        {0x85, "Linux extended Partition"},
        {0x86, "NTFS volume set Partition"},
        {0x87, "NTFS volume set Partition"},
        {0x93, "Amoeba Partition"},
        {0x94, "Amoeba BBT Partition"}, 
        {0xa0, "IBM Thinkpad hibernation Partition"},
        {0xa5, "BSD/386 Partition"},
        {0xa7, "NeXTSTEP 486 Partition"},
        {0xb7, "BSDI fs Partition"},
        {0xb8, "BSDI swap Partition"},
        {0xc1, "DRDOS/sec (FAT-12) Partition"},
        {0xc4, "DRDOS/sec (FAT-16, < 32M) Partition"},
        {0xc6, "DRDOS/sec (FAT-16, >= 32M) Partition"},
        {0xc7, "Syrinx Partition"},
        {0xdb, "CP/M-Concurrent CP/M-Concurrent DOS-CTOS"},
        {0xe1, "DOS access-SpeedStor 12-bit FAT ext."},
        {0xe3, "DOS R/O or SpeedStor Partition"},
        {0xe4, "SpeedStor 16-bit FAT Ext Part. < 1024 cyl."},
        {0xf1, "SpeedStor Partition"},
        {0xf2, "DOS 3.3+ secondary Partition"},
        {0xf4, "SpeedStor large partition Partition"},
        {0xfe, "SpeedStor >1024 cyl. or LANstep Partition"},
        {0xff, "Xenix Bad Block Table Partition"},
     };
#endif /* INCLUDE_PART_SHOW */


/*****************************************************************************
*
* useFdiskPartParse - parse partitions on given disk
*
* This routine is not intended to be user callable.
* 
* This routine parses all existing partition tables on a disk.
* It adds partition node data entries to a table which it has been 
* passed  for any partition which contains a
* file system mountable by dosFsLib().  The size (in sectors)
* of the partition and the absolute offset (in sectors) from the
* start of the drive are also stored, since that is what VxWorks
* CBIO device create routines need to overlay partitions. 
* 
* The partition table must appear to be valid when checked with
* 0x55aa signature.   The partition table in the Master Boot Record
* will be parsed first.  If there are any extended partitions found,
* a (recursive) call to itself is made to parse the extended
* partition(s) in the new sector(s).  Recursive functions may use a lot
* of stack space. Developer should beware of stack overflow.
*
* RETURNS: ERROR or a (positive) number of partitions decoded and filled.
*
*/
LOCAL STATUS useFdiskPartParse
    (
    CBIO_DEV_ID dev,        	/* device from which to read blocks */
    CBIO_PARAMS * pCbioParams, 	/* ptr to CBIO device parameters */
    PART_TABLE_ENTRY *pPartTab, /* table where to fill results */
    int nPart,               	/* # of entries in <pPartTable> */
    ULONG startBlock,		/* where to expect part table, use zero */
    ULONG extStartBlock		/* Offset to extended partition, use zero */
    )
    {
    u_char * secBuf;		/* sector data we work on */
    int i;                      /* used for loop through parts  */
    int partOffset;		/* offset of part tab entry in block */
    int tableIndex = 0;		/* where in table to write partition geo */
    u_char partBootType, partSysType ;
    STATUS stat;

    /* allocate a local secBuf for the read sectors MBR/Part data */

    if  ((secBuf = KHEAP_ALLOC(pCbioParams->bytesPerBlk)) == NULL)
        {
        printErr ("usrFdiskPartParse: Error allocating sector buffer.\n");
        return (ERROR);
        }

    bzero( (char *)secBuf, pCbioParams->bytesPerBlk);

    /* Get current sector containing part table into buffer */

    stat = cbioBlkRW( dev, startBlock, 1, (addr_t)secBuf,
				CBIO_READ, NULL );

   /* 
    * SPR#30850.  Certain devices when accessed for the first time report 
    * Error erroneously.  Once the first read fails, the device might then
    * work as expected after receiving a RESET.  This fix checks for
    * that condition by forcing a RESET and then retrying the read.
    * If it passes after the RESET, then it continues as requested.  If
    * it fails again, however, then it assumes that the Error condition will
    * continue in perpetuity and returns the error code as before.
    */

    if( stat == ERROR ) 
        {
        if(cbioIoctl(dev, CBIO_RESET, 0) == ERROR)
            {
            printErr ("usrFdiskPartParse: error issuing "
                      "CBIO_RESET cmd to device %x, errno %x\n",
                      dev, errno);
            printErr ("usrFdiskPartParse: device is not ready\n");
            KHEAP_FREE(secBuf);
            return (ERROR);
            }
        }
        /* The reset terminated correctly.  Now try the access again. */
        stat = cbioBlkRW( dev, startBlock, 1, (addr_t)secBuf,
				CBIO_READ, NULL );

        /* If still returning error, then assume not going to work. */
        if( stat == ERROR )
            {
            printErr ("usrFdiskPartParse: error reading "
                      "block %ld, errno %x\n",
                      startBlock, errno);
            KHEAP_FREE(secBuf);
            return (ERROR);
            }

    /* Check the validity of partition data with 0x55aa signature */

    if ((secBuf[pCbioParams->bytesPerBlk - 2] != PART_SIG_MSB) || 
        (secBuf[pCbioParams->bytesPerBlk - 1] != PART_SIG_LSB))
	{
	if ((secBuf[PART_SIG_ADRS]     != PART_SIG_MSB) || 
            (secBuf[PART_SIG_ADRS + 1] != PART_SIG_LSB))
            {
	    if( startBlock == 0)
	        {
	        printErr ("usrFdiskPartLib warning: " 
		      "disk partition table is not initialized\n");
	        printErr ("usrFdiskPartLib warning: "
		     "use usrFdiskPartCreate( %#x, 1) to initialize the "
		     "disk as a single partition\n", dev );

	        pPartTab [ 0 ].nBlocks = pCbioParams->nBlocks;
	        pPartTab [ 0 ].offset = 0;
	        pPartTab [ 0 ].flags = 0xeeee ;
	        KHEAP_FREE (secBuf);
	        return(1);
	        }

            printErr ("usrFdiskPartLib error: " 
                  "Sector %d contains an invalid "
                  "MSDOS partition signature 0x55aa.\n", startBlock);
	    KHEAP_FREE (secBuf);
            return (ERROR);
            }
	}

    /*
     * Loop through all parts add mountable parts to table, will call
     * itself recursively to parse extended partitions & logical
     * drives in extended parts.  Recursive functions may use a lot
     * of stack space. Developer should beware of stack overflow.
     */

    for (i=0 ; i<4 ; i++)
        {
	/* partition offset relative to DOS_BOOT_PART_TBL */
	partOffset = DOS_BOOT_PART_TBL + i * 16 ;

        /* if it is a bootable & non-extended partition, add first */
	partBootType = secBuf[ partOffset + BOOT_TYPE_OFFSET] ;
	partSysType  = secBuf[ partOffset + SYSTYPE_OFFSET] ;

	/* XXX - for MS-DOS consistency, Bootable partitions should be first */

        /* continue to next loop iteration if an empty partition */

        if (partSysType == 0)
            {
            continue;
            }

	/* XXX - make this more logical, if not extended and if valid FS */

        if (((partSysType != PART_TYPE_DOSEXT)      &&
             (partSysType != PART_TYPE_WIN95_EXT))  &&      
            ((partSysType == PART_TYPE_DOS4)     ||
             (partSysType == PART_TYPE_WIN95_D4) ||
             (partSysType == PART_TYPE_DOS3)     ||
             (partSysType == PART_TYPE_DOS32)    ||
             (partSysType == PART_TYPE_DOS32X)   ||
             (partSysType == PART_TYPE_DOS12)))
            {
            /* Fill the node with the current partition tables data */ 

	    pPartTab [ tableIndex ].nBlocks =
		DISK_TO_VX_32( &secBuf[ partOffset + NSECTORS_TOTAL]) ;

	    /* offset in part table is relative to table location */

	    pPartTab [ tableIndex ].offset =
		startBlock +
		DISK_TO_VX_32( &secBuf[ partOffset + NSECTORS_OFFSET]) ;

	    /* type of partition */

	    pPartTab [ tableIndex ].flags =
		partSysType | (partBootType << 8 ) ;

	    tableIndex ++ ; /* next partition in next entry */
            }

        else if ((partSysType == PART_TYPE_DOSEXT) ||
                 (partSysType == PART_TYPE_WIN95_EXT))
            {
	    /* found an extended partition, call ourselves to parse.
	     * For primary partitions, the Relative Sectors 
	     * field represents the offset from the beginning 
	     * of the disk to the beginning of the partition, 
	     * counting by sectors. The Number of Sectors field 
	     * represents the total number of sectors in the partition. 
	     * Extended partition have the System ID byte in the 
	     * Partition Table entry is set to 5.  Within the extended 
	     * partition, you can create any number of logical drives. 
	     * The primary partition points to an extended which can have
	     * multiple "logical drives".
	     * When you have an extended partition on the hard disk, 
	     * the entry for that partition in the Partition Table 
	     * (at the end of the Master Boot Record) points to the 
	     * first disk sector in the extended partition.  
	     * The first sector of each logical drive in an extended 
	     * partition also has a Partition Table, with four entries 
	     * (just like the others).  The first entry is for the 
	     * current logical drive.  The second entry contains 
	     * information about the next logical drive in the 
	     * extended partition. Entries three and four are all 
	     * zeroes.  This format repeats for every logical drive. 
	     * The last logical drive has only its own partition entry 
	     * listed. The entries for partitions 2-4 are all zeroes.  
	     * The Partition Table entry is the only information 
	     * on the first side of the first cylinder of each 
	     * logical drive in the extended partition. The entry 
	     * for partition 1 in each Partition Table contains the 
	     * starting address for data on the current logical drive. 
	     * And the entry for partition 2 is the address of the 
	     * sector that contains the Partition Table for the next 
	     * logical drive. 
	     * 
	     * The use of the Relative Sector and Total Sectors 
	     * fields for logical drives in an extended partition 
	     * is different than for primary partitions. 
	     * 
	     * For the partition 1 entry of each logical drive, the 
	     * Relative Sectors field is the sector from the beginning 
	     * of the logical drive that contains the Partition Boot 
	     * Sector. The Total Sectors field is the number of sectors 
	     * from the Partition Boot Sector to the end of the logical 
	     * drive.   
	     * 
	     * For the partition 2 entry, the Relative Sectors field is 
	     * the offset from the beginning of the extended partition 
	     * to the sector containing the Partition Table for the logical 
	     * drive defined in the Partition 2 entry. The Total Sectors 
	     * field is the total size of the logical drive defined in 
	     * the Partition 2 entry.
	     */

	    if (startBlock == 0) 	/* In the MBR, logical sector zero */
		{  
		extStartBlock = DISK_TO_VX_32
			(&secBuf[ partOffset + NSECTORS_OFFSET]);	
		startBlock = extStartBlock; /* startBlock = extended start */
		}
	    else		/* In extended partition/logical drive */
		{
		startBlock = DISK_TO_VX_32
			(&secBuf[ partOffset + NSECTORS_OFFSET]);	
		startBlock += extStartBlock;
		}

            /* Call ourselves to parse extended (sub) partition table */

            stat = useFdiskPartParse (dev,	/* same device */
			pCbioParams,		/* CBIO device parameters */
			pPartTab + tableIndex,	/* next tab entry to fill */
			nPart - tableIndex,	/* # left entries */
			startBlock, 		/* where ext starts */
			extStartBlock		/* where main ext starts */
			);

	    if( stat == ERROR )
		{
		KHEAP_FREE(secBuf);
		return ERROR ;
		}

	    tableIndex += stat ;
            }
	else
	    {
	    printErr("usrFdiskPartLib: warning: "
		    "invalid partition entry encountered "
		    "block %d, entry %d, sys type %x, Skipped\n",
		    startBlock, i, partSysType );
	    }

	/* check for max # of partitions */

	if( tableIndex >= nPart )
	    {
      	    KHEAP_FREE(secBuf);
	    return tableIndex ;
            }

        /* loop here to next part or end looping */
        }

    /*
     * if signature is ok, but no partitions where found
     * on the first level, assume the entire disk is
     * one big file system
     */

    if (tableIndex == 0 && startBlock == 0)
	{
	pPartTab [ tableIndex ].nBlocks = pCbioParams->nBlocks;
	pPartTab [ tableIndex ].offset = 0;
	pPartTab [ tableIndex ].flags = 0x55aa ;
	tableIndex ++;
	}

    KHEAP_FREE(secBuf);
    return (tableIndex);
    }

/*****************************************************************************
*
* usrFdiskPartRead - read an FDISK-style partition table
*
* This function will read and decode a PC formatted partition table
* on a disk, and fill the appropriate partition table array with
* the resulting geometry, which should be used by the dpartCbio
* partition manager to access a partitioned disk with a shared disk
* cache.
*
* EXAMPLE:
* The following example shows how a hard disk which is expected to have
* up to two partitions might be configured, assuming the physical
* level initialization resulted in the <blkIoDevId> handle:
*
* .CS
* devCbio = dcacheDevCreate( blkIoDevId, 0, 0x20000, "Hard Disk");
* mainDevId = dpartDevCreate( devCbio, 2, usrFdiskPartRead )
* dosFsDevCreate(  "/disk0a", dpartPartGet (mainDevId, 0), 0,0,0);
* dosFsDevCreate(  "/disk0b", dpartPartGet (mainDevId, 1), 0,0,0);
* .CE
* 
* RETURNS: OK or ERROR if partition table is corrupt
*
*/

STATUS usrFdiskPartRead
    (
    CBIO_DEV_ID cDev,        /* device from which to read blocks */
    PART_TABLE_ENTRY *pPartTab, /* table where to fill results */
    int nPart               /* # of entries in <pPartTable> */
    )
    {
    CBIO_DEV_ID dev = cDev;
    CBIO_PARAMS cbioParams;

    /* Verify the device handle, possibly create wrapper */

    if (ERROR == cbioDevVerify( dev ))                 
        {                                                 
        /* attempt to handle BLK_DEV subDev */            
        dev = cbioWrapBlkDev ((BLK_DEV *) cDev);

        if( NULL != dev )
            {
            /* SPR#71633, clear the errno set in cbioDevVerify() */
            errno = 0;
            }
        }                                                 

    if (dev == NULL)
	{
	return (ERROR);
	}

    /* The main device has already been RESET before this is called */

    if (TRUE == cbioRdyChgdGet(dev))
	{
	return (ERROR);
	}

    /* Get CBIO device parameters */

    if (ERROR == cbioParamsGet (dev, &cbioParams))
	{
	return (ERROR);
	}

    /* parse the boot partition and fill table with valid partitions */

    if  ((useFdiskPartParse(dev, &cbioParams, pPartTab, nPart, 0, 0)) == ERROR)
	{
	printErr ("useFdiskPartParse error\n");
	return (ERROR);
	}
    
    return (OK);
    }

/*****************************************************************************
*
* usrFdiskPartCreate - create an FDISK-like partition table on a disk
*
* This function may be used to create a basic PC partition table.
* Such partition table however is not intended to be compatible with
* other operating systems, it is intended for disks connected to a
* VxWorks target, but without the access to a PC which may be used to
* create the partition table.
*
* This function is capable of creating only one partition table - the
* MBR, and will not create any Bootable or Extended partitions.
* Therefore, 4 partitions are supported.
*
* <dev> is a CBIO device handle for an entire disk, e.g. a handle
* returned by dcacheDevCreate(), or if dpartCbio is used, it can be either
* the Master partition manager handle, or the one of the 0th partition
* if the disk does not contain a partition table at all.
*
* The <nPart> argument contains the number of partitions to create. If
* <nPart> is 0 or 1, then a single partition covering the entire disk is
* created.
* If <nPart> is between 2 and 4, then the arguments <size1>, <size2>
* and <size3> contain the 
* .I percentage
* of disk space to be assigned to the 2nd, 3rd, and 4th partitions
* respectively. The first partition (partition 0) will be assigned the
* remainder of space left (space hog).
*
* Partition sizes will be round down to be multiple of whole tracks
* so that partition Cylinder/Head/Track fields will be initialized 
* as well as the LBA fields. Although the CHS fields are written they
* are not used in VxWorks, and can not be guaranteed to work correctly
* on other systems.
*
* RETURNS: OK or ERROR writing a partition table to disk
*
*/
STATUS usrFdiskPartCreate
    (
    CBIO_DEV_ID cDev, 	/* device representing the entire disk */
    int		nPart,	/* how many partitions needed, default=1, max=4 */
    int		size1,	/* space percentage for second partition */
    int		size2,	/* space percentage for third partition */
    int		size3	/* space percentage for fourth partition */
    )
    {
    PART_TABLE_ENTRY partTbl[4];
    UINT32 totalSecs, trackSecs, cylSecs, totalTracks, i;
    caddr_t secBuf = NULL ;
    const char dummyString[] =
             "Wind River Systems Inc., DosFs 2.0 Partition Table";
    STATUS stat = OK;

    CBIO_PARAMS cbioParams;
    CBIO_DEV_ID dev = cDev;

    bzero((caddr_t) partTbl, sizeof(partTbl));

    /* Verify the device handle, possibly create wrapper */

    if (ERROR == cbioDevVerify( dev ))
        {
        /* attempt to handle BLK_DEV subDev */
        dev = cbioWrapBlkDev ((BLK_DEV *) cDev);

        if( NULL != dev )
            {
            /* SPR#71633, clear the errno set in cbioDevVerify() */
            errno = 0;
            }
        }
    else
	{
	dev = cDev;
	}

    if (dev == NULL)
        {
        printErr ("usrFdiskPartCreate error: bad handle\n");
        return (ERROR);
        }

    /* if first time usage, a RESET may be needed on the device */

    if( cbioIoctl( dev, CBIO_RESET, 0) == ERROR )
        {
        printErr ("usrFdiskPartCreate error: device is not ready\n");
        return (ERROR);
	}

    /* Get CBIO device parameters */

    if (ERROR == cbioParamsGet (dev, &cbioParams))
	{
        printErr ("usrFdiskPartCreate error: cbioParamsGet returned error\n");
	return (ERROR);
	}

    if( cbioParams.blockOffset != 0 )
	{
	printErr ("usrFdiskPartCreate error: disk is already partitioned\n");
	errno = EINVAL;
	return (ERROR);
	}

    totalSecs = cbioParams.nBlocks ;
    trackSecs = cbioParams.blocksPerTrack;

    if( trackSecs < 1)
	trackSecs = 1;

    cylSecs = trackSecs * cbioParams.nHeads ;

    if( totalSecs < trackSecs * 4 )
	{
	printErr ("usrFdiskPartCreate error: disk too small %d blocks\n",
		    totalSecs);
	errno = EINVAL;
	return (ERROR);
	}

    /* also, part table CHS fields have certain limitations for CHS values */

    if( trackSecs < 1 || trackSecs > 63 )
	{
	trackSecs = 64 ;
	cylSecs = trackSecs * cbioParams.nHeads ;
	}

    if( cylSecs < 1 )
	cylSecs = trackSecs ;

    while((totalSecs/cylSecs) > 1023 )
	cylSecs = cylSecs << 1 ;

    /* rest of calculation made in tracks, round, less chance of overflowing */

    totalTracks = totalSecs / trackSecs ;

#ifdef	DEBUG
    printErr( "  totalTracks %d, trackSecs %d, cylSecs %d\n",
	totalTracks, trackSecs, cylSecs );
#endif

    /* reserve one track for MBR */

    i = totalTracks = totalTracks - 1 ;


    /* 
     * SPR#66973.  The lines inside case statements were of the form
     * partTbl[3].spare = (i * size3)/100 ;
     * which led to overflows on signed 32 bit machines when i and size3
     * were of a certain size (70+Gb harddrives, for example).  'i' is 
     * totalTracks - 1, sizeN represents the percentage portion of that.  
     * sizeN is an integer value < 100, not a float value < 1.
     * Since i is the larger of the two, the reduction in absolute terms 
     * by dividing it (instead of sizeN) will be greater.
     * Other option is to use 64bit math temporarily, which you could do 
     * by simply casting, forcing the compiler to promote it:
     *
     *           partTbl[3].spare = ((long long)i * size3)/100 ;  
     *
     * but that takes extra instructions, cycles, etc, also the receptacle
     * (partTbl[N].spare) is declared an int so the demotion back to a 32
     * bits might cause trouble depending on the compiler implementation.
     *
     */

    switch (nPart)
	{
	case 4:
	    partTbl[3].spare = size3 * (i/100) ; /* SPR 66973 */
	    totalTracks -= partTbl[3].spare ;

	    /*FALLTHROUGH*/
	case 3:
	    partTbl[2].spare = size2 * (i/100) ; /* SPR 66973 */
	    totalTracks -= partTbl[2].spare ;

	    /*FALLTHROUGH*/
	case 2:
	    partTbl[1].spare = size1 * (i/100) ; /* SPR 66973 */
	    totalTracks -= partTbl[1].spare ;

	    /*FALLTHROUGH*/
	case 0:
	case 1:
	    if( totalTracks <= 0 )
		{
		/* partition sizes dont sum up */
		/* printErr("  EINVAL:  totalTracks %d\n", totalTracks ); */
		errno = EINVAL;
		return (ERROR);
		}
	    partTbl[0].spare = totalTracks ;
	    break ;
	default:
	    errno = EINVAL;
	    return (ERROR);
	}

    /* normalize the entire partition table, calc offset etc.*/

    for(i=0, totalTracks = 1; i<4; i++)
	{
	if( partTbl[i].spare == 0 )
	    continue ;

	partTbl[i].offset = totalTracks * trackSecs ;
	partTbl[i].nBlocks = partTbl[i].spare * trackSecs ;
	totalTracks += partTbl[i].spare ;

	/*
	 * If the partition is greater than or equal to 2GB,
	 * use FAT32x partition type.  Else if the partition 
	 * is greater than or equal to 65536, use BIGDOS FAT 
	 * 16bit FAT, 32Bit sector number.  Else if the partition 
	 * is and greater or equal to 32680 use SMALLDOS FAT, 
	 * 16bit FAT, 16bit sector num.  Else use FAT12 for 
	 * anything smaller than 32680.  Note: some systems 
 	 * may want to change this to use the Windows95 partiton 
	 * types that support (LBA) INT 13 extensions, since the 
	 * only thing VxWorks can truely ensure is the LBA fields, 
	 * mostly since vxWorks does not use the BIOS (PC BIOS is 
	 * NOT deterministic, bad for a RTOS, plus they tend not 
         * to be present on non-x86 targets.) and cannot ensure
	 * the BIOS translation of CHS.  Of course, the 32bit VxWorks
	 * RTOS would never need such a hack as CHS translation.  
	 * The reason we don't use the LBA field now is that 
	 * NT 3.51 and NT 4.0 will not recognize the new partition 
	 * types (0xb-0xf).   That is one reason this is shipped 
	 * in source.
	 * 
	 * TODO: Reconsider using partition types 0xb-0xf when 
	 * MS gets their trip together.
	 */

	if(partTbl[i].nBlocks >= 0x400000) 
	    partTbl[i].flags = PART_TYPE_DOS32X;
	else if (partTbl[i].nBlocks >= 65536)  
	    partTbl[i].flags = PART_TYPE_DOS4; 
	else if (partTbl[i].nBlocks >= 32680) 
	    partTbl[i].flags = PART_TYPE_DOS3; 
	else
	    partTbl[i].flags = PART_TYPE_DOS12;
	}

    /* allocate a local secBuf for the read sectors MBR/Part data */

    if  ((secBuf = KHEAP_ALLOC(cbioParams.bytesPerBlk)) == NULL)
        {
        printErr ("usrFdiskPartCreate: Error allocating sector buffer.\n");
        return (ERROR);
        }

    /* start filling the MBR buffer */

    bzero( secBuf, cbioParams.bytesPerBlk);

    /* fill the top with a silly RET sequence, not JMP */

    secBuf[0] = 0x90 ; /* NOP */
    secBuf[1] = 0x90 ; /* NOP */
    secBuf[2] = 0xc3 ; /* RET */

    bcopy( dummyString, secBuf+3, sizeof(dummyString));

    /* write bottom 0x55aa signature */

    secBuf[ PART_SIG_ADRS ]     = PART_SIG_MSB; /* 0x55 */
    secBuf[ PART_SIG_ADRS + 1 ] = PART_SIG_LSB; /* 0xaa */

    /* 
     * When the sector size is larger than 512 bytesPerSector we write the 
     * signature to the end of the sector as well as at offset 510/511.
     */

    if (512 < cbioParams.bytesPerBlk)
	{
	secBuf[ cbioParams.bytesPerBlk - 2 ] = PART_SIG_MSB; /* 0x55 */
	secBuf[ cbioParams.bytesPerBlk - 1 ] = PART_SIG_LSB; /* 0xAA */
	}

    /* Now, fill the 4 partition entries, careful with byte ordering */

    for(i = 0; i < 4; i ++ )
	{
	int cyl, head, tsec, s1 ;

	/* calculate offset of current partition table entry */

	int partOffset = DOS_BOOT_PART_TBL + i * 16 ;

	/* fill in fields */

	secBuf[ partOffset + BOOT_TYPE_OFFSET]  = 
			(i) ? PART_NOT_BOOTABLE : PART_IS_BOOTABLE;

	secBuf[ partOffset + SYSTYPE_OFFSET]  = partTbl[i].flags ;

	/* LBA number of sectors */

	VX_TO_DISK_32( partTbl [ i ].nBlocks,
			&secBuf[ partOffset + NSECTORS_TOTAL]);
	/* LBA offset */

	VX_TO_DISK_32( partTbl [ i ].offset,
			&secBuf[ partOffset + NSECTORS_OFFSET]);

	/* beginning of partition in CHS */

	if( partTbl [ i ].nBlocks > 0)
	    {
	    s1 = partTbl [ i ].offset ;
	    cyl = s1 / cylSecs ;
	    head = (s1 - (cyl * cylSecs)) / trackSecs ;
	    tsec = 1 + s1 - (cyl * cylSecs) - (head*trackSecs);
	    }
	else
	    {
	    cyl = head = tsec = 0 ;	/* unused table entry */
	    }

#ifdef	DEBUG
	printErr("  start cyl %d hd %d s %d\n", cyl, head, tsec );
#endif
        secBuf[ partOffset + STARTSEC_CYL_OFFSET ] = cyl & 0xff ;
        secBuf[ partOffset + STARTSEC_SEC_OFFSET ] = ((cyl>>2) & 0xc0) | tsec ;
        secBuf[ partOffset + STARTSEC_HD_OFFSET ] = head ;

	/* end of partition in CHS */

	if( partTbl [ i ].nBlocks > 0)
	    {
	    s1 = partTbl [ i ].offset + partTbl [ i ].nBlocks - 1  ;
	    cyl = s1 / cylSecs ;
	    head = (s1 - (cyl * cylSecs)) / trackSecs ;
	    tsec = 1 + s1 - (cyl * cylSecs) - (head*trackSecs);
	    }
	else
	    {
	    cyl = head = tsec = 0 ;	/* unused table entry */
	    }

#ifdef	DEBUG
	printErr("  end   cyl %d hd %d s %d\n", cyl, head, tsec );
#endif
        secBuf[ partOffset + ENDSEC_CYL_OFFSET ] = cyl & 0xff ;
        secBuf[ partOffset + ENDSEC_SEC_OFFSET ] = ((cyl>>2) & 0xc0) | tsec ;
        secBuf[ partOffset + ENDSEC_HD_OFFSET ] = head ;
	}

    (void)  cbioIoctl( dev, CBIO_DEVICE_LOCK, 0) ;

    /* Last but not least, write the MBR to disk */

    stat = cbioBlkRW( dev, 0 , 1, secBuf, CBIO_WRITE, NULL ) ;

    /* flush and invalidate cache immediately */

    stat |= cbioIoctl( dev, CBIO_CACHE_INVAL, 0) ;

    cbioRdyChgdSet (dev, TRUE) ;	/* force re-mount */

    (void)  cbioIoctl( dev, CBIO_DEVICE_UNLOCK, 0) ;

    KHEAP_FREE(secBuf);
    return stat;
    }


#if defined(INCLUDE_PART_SHOW)
/*****************************************************************************
*
* usrFdiskPartShow - parse and display partition data
*
* This routine is intended to be user callable.
*
* A device dependent partition table show routine.  This 
* routine outputs formatted data for all partition table 
* fields for every partition table found on a given disk,
* starting with the MBR sectors partition table.  This code can be 
* removed to reduce code size by undefining: INCLUDE_PART_SHOW
* and rebuilding this library and linking to the new library.
* 
* This routine takes three arguments. First, a CBIO pointer 
* (assigned for the entire physical disk) usually obtained 
* from dcacheDevCreate().  It also takes two block_t type 
* arguments and one signed int, the user shall pass zero 
* in these paramaters.
* 
* For example:
* .CS
* sp usrFdiskPartShow (pCbio,0,0,0)
* .CE
*
* Developers may use size<arch> to view code size.
* 
* NOTE
*
* RETURNS: OK or ERROR
*
* INTERNAL
* This function was adopted from jkf with minimal changes
*/

STATUS usrFdiskPartShow
    (
    CBIO_DEV_ID cbio,         /* device CBIO handle */
    block_t extPartOffset,    /* user should pass zero */
    block_t currentOffset,    /* user should pass zero */
    int extPartLevel          /* user should pass zero */
    )
    {
    block_t tmpVal;    
    UCHAR * secBuf;             /* a secBuf for the sector data */
    size_t numNames;      	/* partition name array entries */
    UINT8 dosSec;               /* for temp use. Sector         */
    UINT8 dosCyl;               /* for temp use. Cylinder       */
    STATUS stat;		/* for return status of called  */
    int i;                      /* used for loop through parts  */
    size_t ix;                  /* used for loop through parts  */
    int partOffset;             /* offset from partition tbl ent*/
    CBIO_PARAMS cbioParams;
    CBIO_DEV_ID cbioDev = cbio;

    /* Verify the device handle, possibly create wrapper */

    if (ERROR == cbioDevVerify( cbio ))
        {
        /* attempt to handle BLK_DEV subDev */
        cbioDev = cbioWrapBlkDev ((BLK_DEV *) cbio);

        if( NULL != cbioDev )
            {
            /* SPR#71633, clear the errno set in cbioDevVerify() */
            errno = 0;
            }
        }
    else
	{
	cbioDev = cbio;
	}

    if (cbioDev == NULL)
        {
        return (ERROR);
        }

    if (ERROR == cbioParamsGet (cbioDev, &cbioParams))
	{
	return (ERROR);
	}

    /* get number of partition type entries */

    numNames = ((sizeof (partNames)) / (sizeof (struct partType)));


    /* allocate a local secBuf for the read sectors MBR/Part data */

    if  ((secBuf = KHEAP_ALLOC(cbioParams.bytesPerBlk)) == NULL)
        {
        printErr ("usrFdiskPartShow: Error allocating sector buffer.\n");
        return (ERROR);
        }


    /* cbio reads the partition sector into a secBuf */

    stat = cbioBlkRW( cbioDev, currentOffset, 1, (addr_t)secBuf,
				CBIO_READ, NULL );

    if (stat == ERROR)
        {
        /* if first time reading, a RESET may be needed on the device */

        if( cbioIoctl( cbioDev, CBIO_RESET, 0) == ERROR )
            {
            printErr ("usrFdiskPartShow: error reading"
                      " sector %ld, errno %x\n", currentOffset, errno);
            printErr ("usrFdiskPartShow: device is not ready\n");
            return (ERROR);
	    }
	else /* Reset OK, try read again */
	    {
	    stat = cbioBlkRW( cbioDev, currentOffset, 1, (addr_t)secBuf,
				CBIO_READ, NULL );
	    if (stat == ERROR)
		{
                printErr ("usrFdiskPartShow: error reading "
			  "sector %ld, errno %x\n", currentOffset, errno);
		return (ERROR);
		}
	    }
        } 

    /* Check the validity of partition data with 0x55aa signature */

    if ((secBuf[cbioParams.bytesPerBlk - 2] != PART_SIG_MSB) || 
        (secBuf[cbioParams.bytesPerBlk - 1] != PART_SIG_LSB))
	{
	if ((secBuf[PART_SIG_ADRS]     != PART_SIG_MSB) || 
            (secBuf[PART_SIG_ADRS + 1] != PART_SIG_LSB))
            {
            printErr ("usrFdiskPartShow: "
                  "Sector %ld contains an invalid signature 0x55aa.\n",
		  currentOffset);
            return (ERROR);
            }
	}

    /* Loop through and display data for all partitions in the list */

    for (i=0 ; i<4 ; i++)
        {
	/* setup an offset relative to DOS_BOOT_PART_TBL to entry */

	partOffset = i * 16;

        /* Display extended or MBR */

        if (currentOffset == 0x0)
            {
            printf ("\n");
            printf ("Master Boot Record - Partition Table \n");
            printf ("--------------------------------------\n");
            }

        else 
            {
            printf ("\n");
            printf ("Extended Partition Table %02d \n", extPartLevel);
            printf ("--------------------------------------\n");
            }
        

        /* display partition entry and offset */

        printf ("  Partition Entry number %02d    ", i);


        switch (i)
            {
            case 3:
                printf (" Partition Entry offset 0x1ee\n");
                break;

            case 2:
                printf (" Partition Entry offset 0x1de\n");
                break;

            case 1:
                printf (" Partition Entry offset 0x1ce\n");
                break;

            case 0:
                printf (" Partition Entry offset 0x1be\n");
                break; 
            }


        printf ("  Status field = 0x%02x          ",
                secBuf[DOS_BOOT_PART_TBL + partOffset + 
                       BOOT_TYPE_OFFSET]);
        
        if ((secBuf[DOS_BOOT_PART_TBL + partOffset + 
             BOOT_TYPE_OFFSET]) == (PART_IS_BOOTABLE))
            {
            printf (" Primary (bootable) Partition \n");
            }

        else
            {
            printf (" Non-bootable Partition       \n");
            }

        /* print partition type name from array */

        for (ix=0; ix < numNames; ix++)
            {
            if ((partNames[ix].partTypeNum) == 
                (secBuf[DOS_BOOT_PART_TBL + partOffset + 
                        SYSTYPE_OFFSET]))
                {
                printf ("  Type 0x%02x: ",secBuf[DOS_BOOT_PART_TBL
                                                 + partOffset
                                                 + SYSTYPE_OFFSET]);
                printf ("%s\n",partNames[ix].partTypeName);

                break;  /*  found type, so fall out of loop. */
                }
	    }

        /* Skip the rest of a NULL/empty partition */

        if (secBuf[DOS_BOOT_PART_TBL + partOffset + SYSTYPE_OFFSET] 
            == 0x0)
            {
            printf ("\n");
            continue;
            }


        /* Print out the CHS information */

        dosSec = (secBuf[DOS_BOOT_PART_TBL + partOffset
                  + STARTSEC_SEC_OFFSET] & 0x3f);
        dosCyl =  ((((secBuf[DOS_BOOT_PART_TBL + partOffset
                      + STARTSEC_SEC_OFFSET]) & 0xc0) << 2)|
                   (((secBuf[DOS_BOOT_PART_TBL + partOffset + 
                      STARTSEC_CYL_OFFSET]) & 0xff) >> 8));

        printf ("  Partition start LCHS: Cylinder %04d, Head %03d,",
                 dosCyl, secBuf[DOS_BOOT_PART_TBL + partOffset + 
                 STARTSEC_HD_OFFSET]);
        printf (" Sector %02d   \n", dosSec);

        /*
         * Print out the ending sector CHS information for partition
         * calculate the actual DOS CHS sector and cylinder values.
         */

        dosSec = (secBuf[DOS_BOOT_PART_TBL + partOffset
                  + ENDSEC_SEC_OFFSET] & 0x3f);

        dosCyl =  ((((secBuf[DOS_BOOT_PART_TBL + partOffset 
                             + ENDSEC_SEC_OFFSET]) & 0xc0) << 2) | 
                   (((secBuf[DOS_BOOT_PART_TBL + partOffset
                             + ENDSEC_CYL_OFFSET]) & 0xff) ));

        printf ("  Partition end   LCHS: Cylinder %04d, Head %03d,",
                dosCyl, secBuf[DOS_BOOT_PART_TBL + partOffset
                               + ENDSEC_HD_OFFSET]);

        printf (" Sector %02d   \n", dosSec);


        /* Print offsets */

	tmpVal = DISK_TO_VX_32( &secBuf[DOS_BOOT_PART_TBL + partOffset
                 + NSECTORS_OFFSET]) ;

        if (extPartLevel == 0x0)
            {
            printf ("  Sectors offset from MBR partition 0x%08lx\n",tmpVal);
            }
        else 
            {
            printf ("  Sectors offset from the extended partition 0x%08lx\n",
			tmpVal);
            }

	tmpVal = DISK_TO_VX_32( &secBuf[DOS_BOOT_PART_TBL + partOffset 
                 + NSECTORS_TOTAL]) ;

        printf ("  Number of sectors in partition 0x%08lx\n", tmpVal);

	tmpVal = DISK_TO_VX_32( &secBuf[DOS_BOOT_PART_TBL + partOffset 
                 + NSECTORS_OFFSET]) ;

	printf ("  Sectors offset from start of disk 0x%08lx\n",
		(tmpVal + currentOffset));

        }


    /* Loop through all partitions in the list to find extended */

    for (i=0 ; i<4 ; i++)
        {
	/* setup an offset relative to DOS_BOOT_PART_TBL to entry */
	partOffset = i * 16;

        if ((secBuf[DOS_BOOT_PART_TBL + partOffset + SYSTYPE_OFFSET]
             == PART_TYPE_DOSEXT) ||
            (secBuf[DOS_BOOT_PART_TBL + partOffset + SYSTYPE_OFFSET]
             == PART_TYPE_WIN95_EXT))
            {
	    /* found an extended partition, call ourselves to parse.
	     * For primary partitions, the Relative Sectors 
	     * field represents the offset from the beginning 
	     * of the disk to the beginning of the partition, 
	     * counting by sectors. The Number of Sectors field 
	     * represents the total number of sectors in the partition. 
	     * Extended partition have the System ID byte in the 
	     * Partition Table entry is set to 5.  Within the extended 
	     * partition, you can create any number of logical drives. 
	     * The primary partition points to an extended which can have
	     * multiple "logical drives".
	     * When you have an extended partition on the hard disk, 
	     * the entry for that partition in the Partition Table 
	     * (at the end of the Master Boot Record) points to the 
	     * first disk sector in the extended partition.  
	     * The first sector of each logical drive in an extended 
	     * partition also has a Partition Table, with four entries 
	     * (just like the others).  The first entry is for the 
	     * current logical drive.  The second entry contains 
	     * information about the next logical drive in the 
	     * extended partition. Entries three and four are all 
	     * zeroes.  This format repeats for every logical drive. 
	     * The last logical drive has only its own partition entry 
	     * listed. The entries for partitions 2-4 are all zeroes.  
	     * The Partition Table entry is the only information 
	     * on the first side of the first cylinder of each 
	     * logical drive in the extended partition. The entry 
	     * for partition 1 in each Partition Table contains the 
	     * starting address for data on the current logical drive. 
	     * And the entry for partition 2 is the address of the 
	     * sector that contains the Partition Table for the next 
	     * logical drive. 
	     * 
	     * The use of the Relative Sector and Total Sectors 
	     * fields for logical drives in an extended partition 
	     * is different than for primary partitions. 
	     * 
	     * For the partition 1 entry of each logical drive, the 
	     * Relative Sectors field is the sector from the beginning 
	     * of the logical drive that contains the Partition Boot 
	     * Sector. The Total Sectors field is the number of sectors 
	     * from the Partition Boot Sector to the end of the logical 
	     * drive.   
	     * 
	     * For the partition 2 entry, the Relative Sectors field is 
	     * the offset from the beginning of the extended partition 
	     * to the sector containing the Partition Table for the logical 
	     * drive defined in the Partition 2 entry. The Total Sectors 
	     * field is the total size of the logical drive defined in 
	     * the Partition 2 entry.
	     */

	    if (currentOffset == 0) 	/* In the MBR, logical sector zero */
		{  
		extPartOffset = DISK_TO_VX_32
			(&secBuf[DOS_BOOT_PART_TBL + partOffset 
			         + NSECTORS_OFFSET]);	
		currentOffset = extPartOffset; /* startBlock = ext. start */
		}
	    else		/* In extended partition/logical drive */
		{
		currentOffset = DISK_TO_VX_32
			(&secBuf[DOS_BOOT_PART_TBL + partOffset 
			         + NSECTORS_OFFSET]);	
		currentOffset += extPartOffset;
		}

            /* Update extPartLevel and parse found extended partition */

            extPartLevel++;
            usrFdiskPartShow (cbioDev, currentOffset, extPartOffset, 
			      extPartLevel);
            extPartLevel--;
            }
        /* next part or end looping */
        }

    /* Clean up */

    KHEAP_FREE(secBuf);  
    return (OK);
    }
#endif /* INCLUDE_DOS_PART_SHOW */

