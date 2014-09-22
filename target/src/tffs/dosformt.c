/*
 * $Log:   V:/dosformt.c_v  $
 * 
 *    Rev 1.16   05 May 1998 15:02:00   yogu
 * renamed MIN_CLUSTER_SIZE to flMinClusterSize and made it
 * a global int so it's customizable
 *
 *    Rev 1.16   25 Mar 1998 15:02:00   ANDRY, Hdei
 * Added bootCode[] table.
 *
 *    Rev 1.16   11 Nov 1997 15:27:14   ANDRY
 * () in complex expressions to get rid of compiler warnings
 *
 *    Rev 1.15   28 Aug 1997 16:40:32   danig
 * Moved big-endian to flbase.c
 *
 *    Rev 1.14   21 Aug 1997 14:06:34   unknown
 * Unaligned4
 *
 *    Rev 1.13   19 Aug 1997 20:08:58   danig
 * Andray's changes
 *
 *    Rev 1.12   14 Aug 1997 16:06:34   danig
 * Moved MIN_CLUSTER_SIZE to flcustom.h
 *
 *    Rev 1.11   24 Jul 1997 17:53:42   amirban
 * FAR -> FAR0, UNALIGNED -> Unaligned
 *
 *    Rev 1.10   20 Jul 1997 17:16:20   amirban
 * Get rid of warnings
 *
 *    Rev 1.9   07 Jul 1997 15:21:00   amirban
 * Ver 2.0
 *
 *    Rev 1.8   03 Jun 1997 17:18:56   amirban
 * Min cluster size
 *
 *    Rev 1.7   13 May 1997 14:04:40   amirban
 * Big-endian bug fix (reverse order)
 *
 *    Rev 1.6   09 Apr 1997 17:35:36   amirban
 * Partition table redefined
 *
 *    Rev 1.5   02 Apr 1997 16:55:34   amirban
 * BBP redefined
 *
 *    Rev 1.4   16 Oct 1996 16:01:40   danig
 * Big-Endian bug
 *
 *    Rev 1.3   03 Oct 1996 14:37:28   amirban
 * New Big-Endian
 *
 *    Rev 1.3   03 Oct 1996 11:56:36   amirban
 * New Big-Endian
 *
 *    Rev 1.2   18 Aug 1996 13:48:34   amirban
 * Comments
 *
 *    Rev 1.1   14 Jul 1996 16:48:24   amirban
 * format params
 *
 *    Rev 1.0   20 Mar 1996 13:33:06   amirban
 * Initial revision.
 */

/************************************************************************/
/*                                                                      */
/*		FAT-FTL Lite Software Development Kit			*/
/*		Copyright (C) M-Systems Ltd. 1995-1996			*/
/*									*/
/************************************************************************/

#include "fltl.h"

#ifdef FORMAT_VOLUME

#include "dosformt.h"

#define FAT12bit  (LE4(bpb->totalSectorsInVolume) < 	\
		   4086LU * bpb->sectorsPerCluster)

int flMinClusterSize = 4;

/*----------------------------------------------------------------------*/
/*      	      g e t D r i v e G e o m e t r y			*/
/*									*/
/* Calculates the geometry parameters for BIOS/DOS media		*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*                                                                      */
/* Returns:                                                             */
/*	bpb		: volume BIOS parameter block			*/
/*	cylinders	: Number of "cylinders" in volume		*/
/*	noOfFATs	: Number of FAT copies				*/
/*----------------------------------------------------------------------*/

static void getDriveGeometry(TL vol,
			     BPB *bpb,
			     unsigned int *cylinders,
			     unsigned noOfFATs)
{
  unsigned long heads, sectors, temp;
  long int sizeInSectors, noOfClusters;
  int directorySectors;

  SectorNo capacity = vol.sectorsInVolume(vol.rec); /* Volume size in sectors */
  *cylinders = 1024;                 /* Set number of cylinders to max value */

  heads = 16L;                      /* Max out number of heads */
  temp = *cylinders * heads;        /* Compute divisor for heads */
  sectors = capacity / temp;        /* Compute value for sectors per track */
  if (capacity % temp) { 	    /* If no remainder, done! */
    sectors++;                      /* Else, increment number of sectors */
    temp = *cylinders * sectors;    /* Compute divisor for heads */
    heads = capacity / temp;        /* Compute value for heads */
    if (capacity % temp) {          /* If no remainder, done! */
      heads++;                      /* Else, increment number of heads */
      temp = heads * sectors;       /* Compute divisor for cylinders */
      *cylinders = (unsigned) (capacity / temp);
    }
  }

  toLE2(bpb->sectorsPerTrack,(unsigned short) sectors);
  toLE2(bpb->noOfHeads,(unsigned short) heads);
  toUNAL2(bpb->bytesPerSector,SECTOR_SIZE);
  toLE2(bpb->reservedSectors,1);
  bpb->noOfFATS = noOfFATs;
  bpb->mediaDescriptor = 0xf8;	/* hard disk */
  toLE4(bpb->noOfHiddenSectors,sectors);

  sizeInSectors = (long) (*cylinders) * heads * sectors - sectors;

  toLE4(bpb->totalSectorsInVolume,sizeInSectors);
  toUNAL2(bpb->totalSectorsInVolumeDOS3,sizeInSectors > 65535l ? 0 : (unsigned short) sizeInSectors);

  noOfClusters = sizeInSectors / flMinClusterSize;
  for (bpb->sectorsPerCluster = flMinClusterSize;
       noOfClusters > (bpb->sectorsPerCluster < 8 ? 32766l : 65534l);
       bpb->sectorsPerCluster <<= 1, noOfClusters >>= 1);

  if (FAT12bit)
    toLE2(bpb->sectorsPerFAT,
	  (unsigned short) ((((noOfClusters + 2L) * 3 + 1) / 2 - 1) / SECTOR_SIZE + 1));
  else
    toLE2(bpb->sectorsPerFAT,
	  (unsigned short) (((noOfClusters + 2L) * 2 - 1) / SECTOR_SIZE + 1));

  directorySectors = capacity / 200;
  if (directorySectors < 1) directorySectors = 1;
  if (directorySectors > 15) directorySectors = 15;
  toUNAL2(bpb->rootDirectoryEntries,
	 directorySectors * (SECTOR_SIZE / sizeof(DirectoryEntry)));
}


/*----------------------------------------------------------------------*/
/*      	 c r e a t e M a s t e r B o o t R e c o r d		*/
/*									*/
/* Creates the Master Boot Record (Sector 0)				*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	bpb		: volume BIOS parameter block			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*	cylinders	: Number of "cylinders" in volume		*/
/*----------------------------------------------------------------------*/

static FLStatus createMasterBootRecord(TL vol,
				     BPB *bpb,
				     unsigned cylinders)
{
  /* create partition table */
  PartitionTable partitionTable;
  static unsigned char bootCode[] = {
    0xFA, 0x33, 0xC0, 0x8E, 0xD0, 0xBC, 0x00, 0x7C,
    0x8B, 0xF4, 0x50, 0x07, 0x50, 0x1F, 0xFB, 0xFC,
    0xBF, 0x00, 0x06, 0xB9, 0x00, 0x01, 0xF2, 0xA5,
    0xEA, 0x1D, 0x06, 0x00, 0x00, 0xBE, 0xBE, 0x07,
    0xB3, 0x04, 0x80, 0x3C, 0x80, 0x74, 0x0E, 0x80,
    0x3C, 0x00, 0x75, 0x1C, 0x83, 0xC6, 0x10, 0xFE,
    0xCB, 0x75, 0xEF, 0xCD, 0x18, 0x8B, 0x14, 0x8B,
    0x4C, 0x02, 0x8B, 0xEE, 0x83, 0xC6, 0x10, 0xFE,
    0xCB, 0x74, 0x1A, 0x80, 0x3C, 0x00, 0x74, 0xF4,
    0xBE, 0x8B, 0x06, 0xAC, 0x3C, 0x00, 0x74, 0x0B,
    0x56, 0xBB, 0x07, 0x00, 0xB4, 0x0E, 0xCD, 0x10,
    0x5E, 0xEB, 0xF0, 0xEB, 0xFE, 0xBF, 0x05, 0x00,
    0xBB, 0x00, 0x7C, 0xB8, 0x01, 0x02, 0x57, 0xCD,
    0x13, 0x5F, 0x73, 0x0C, 0x33, 0xC0, 0xCD, 0x13,
    0x4F, 0x75, 0xED, 0xBE, 0xA3, 0x06, 0xEB, 0xD3,
    0xBE, 0xC2, 0x06, 0xBF, 0xFE, 0x7D, 0x81, 0x3D,
    0x55, 0xAA, 0x75, 0xC7, 0x8B, 0xF5, 0xEA, 0x00,
    0x7C, 0x00, 0x00, 0x49, 0x6E, 0x76, 0x61, 0x6C,
    0x69, 0x64, 0x20, 0x70, 0x61, 0x72, 0x74, 0x69,
    0x74, 0x69, 0x6F, 0x6E, 0x20, 0x74, 0x61, 0x62,
    0x6C, 0x65, 0x00, 0x45, 0x72, 0x72, 0x6F, 0x72,
    0x20, 0x6C, 0x6F, 0x61, 0x64, 0x69, 0x6E, 0x67,
    0x20, 0x6F, 0x70, 0x65, 0x72, 0x61, 0x74, 0x69,
    0x6E, 0x67, 0x20, 0x73, 0x79, 0x73, 0x74, 0x65,
    0x6D, 0x00, 0x4D, 0x69, 0x73, 0x73, 0x69, 0x6E,
    0x67, 0x20, 0x6F, 0x70, 0x65, 0x72, 0x61, 0x74,
    0x69, 0x6E, 0x67, 0x20, 0x73, 0x79, 0x73, 0x74,
    0x65, 0x6D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x01,
    0x01, 0x00, 0x06, 0x07, 0xE3, 0x66, 0x23, 0x00,
    0x00, 0x00, 0x85, 0xB8, 0x03, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x55, 0xAA};

  tffscpy(&partitionTable,bootCode,sizeof partitionTable);

  partitionTable.activeFlag = 0x80;	/* bootable */
  if (LE2(bpb->noOfHeads) > 1) {
    partitionTable.startingHead = 1;
    toLE2(partitionTable.startingCylinderSector,CYLINDER_SECTOR(0,1));
  }
  else {
    partitionTable.startingHead = 0;
    toLE2(partitionTable.startingCylinderSector,CYLINDER_SECTOR(1,1));
  }
  partitionTable.type = FAT12bit ? 1 : 4;
  partitionTable.endingHead = LE2(bpb->noOfHeads) - 1;
  toLE2(partitionTable.endingCylinderSector,
		CYLINDER_SECTOR((cylinders - 1),LE2(bpb->sectorsPerTrack)));
  toUNAL4(partitionTable.startingSectorOfPartition,LE2(bpb->sectorsPerTrack));
  toUNAL4(partitionTable.sectorsInPartition,LE4(bpb->totalSectorsInVolume));

  toLE2(partitionTable.signature,PARTITION_SIGNATURE);

  return vol.writeSector(vol.rec,0,&partitionTable);
}

/*----------------------------------------------------------------------*/
/*      	     c r e a t e D O S B o o t S e c t o r		*/
/*									*/
/* Creates the DOS boot sector						*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	bpb		: volume BIOS parameter block			*/
/*	volumeId	: 32-bit volume id				*/
/*	volumeLabel	: volume label					*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

static FLStatus createDOSbootSector(TL vol,
				  BPB *bpb,
				  const char FAR1 *volumeId,
				  const char FAR1 *volumeLabel)
{
  DOSBootSector bootSector;

  tffsset(&bootSector,0,sizeof bootSector);
  bootSector.physicalDriveNo = 0x80;
  bootSector.extendedBootSignature = 0x29;
  tffscpy(bootSector.volumeId,volumeId,sizeof bootSector.volumeId);
  tffsset(bootSector.volumeLabel,' ',sizeof bootSector.volumeLabel);
  if (volumeLabel)
    tffscpy(bootSector.volumeLabel,volumeLabel,sizeof bootSector.volumeLabel);
  tffscpy(bootSector.systemId,
	  FAT12bit ? "FAT12   " : "FAT16   ",
	  sizeof bootSector.systemId);

  bootSector.bpb = *bpb;
#if	FALSE
  tffscpy (&bootSector.bpb, bpb, sizeof(BPB));
#endif	/* FALSE */

  bootSector.bpb.jumpInstruction[0] = 0xe9;
  tffscpy(bootSector.bpb.OEMname,"MSystems",sizeof bootSector.bpb.OEMname);
  toLE2(bootSector.signature,PARTITION_SIGNATURE);

  return vol.writeSector(vol.rec,(SectorNo) LE4(bpb->noOfHiddenSectors),&bootSector);
}


/*----------------------------------------------------------------------*/
/*      	          c r e a t e F A T s				*/
/*									*/
/* Creates the FAT's							*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	bpb		: volume BIOS parameter block			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

static FLStatus createFATs(TL vol, BPB *bpb)
{
  int iFAT;

  SectorNo sectorNo = (SectorNo) (LE4(bpb->noOfHiddenSectors) +
				  LE2(bpb->reservedSectors));

  /* create the FATs */
  for (iFAT = 0; iFAT < bpb->noOfFATS; iFAT++) {
    int iSector;
    unsigned char FATEntry[SECTOR_SIZE];

    for (iSector = 0; iSector < LE2(bpb->sectorsPerFAT); iSector++) {
      tffsset(FATEntry,0,SECTOR_SIZE);
      if (iSector == 0) {		/* write the reserved FAT entries */
	FATEntry[0] = bpb->mediaDescriptor;
	FATEntry[1] = 0xff;
	FATEntry[2] = 0xff;
	if (!FAT12bit)
	  FATEntry[3] = 0xff;
      }
      checkStatus(vol.writeSector(vol.rec,sectorNo++,FATEntry));
    }
  }

  return flOK;
}


/*----------------------------------------------------------------------*/
/*      	     c r e a t e R o o t D i r e c t o r y		*/
/*									*/
/* Creates the root directory						*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	bpb		: volume BIOS parameter block			*/
/*	volumeLabel	: volume label					*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

static FLStatus createRootDirectory(TL vol,
				  BPB *bpb,
				  const char FAR1 *volumeLabel)
{
  int iEntry;

  SectorNo sectorNo = (SectorNo) (LE4(bpb->noOfHiddenSectors) +
				  LE2(bpb->reservedSectors) +
				  bpb->noOfFATS * LE2(bpb->sectorsPerFAT));

  /* create the root directory */
  for (iEntry = 0; iEntry < UNAL2(bpb->rootDirectoryEntries);
       iEntry += (SECTOR_SIZE / sizeof(DirectoryEntry))) {
    DirectoryEntry rootDirectorySector[SECTOR_SIZE / sizeof(DirectoryEntry)];

    tffsset(rootDirectorySector,0,SECTOR_SIZE);
    if (iEntry == 0 && volumeLabel) {
      tffsset(rootDirectorySector[0].name,' ',sizeof rootDirectorySector[0].name);
      tffscpy(rootDirectorySector[0].name,volumeLabel,sizeof rootDirectorySector[0].name);
      rootDirectorySector[0].attributes = 0x28;	/* VOL + ARC */
      toLE2(rootDirectorySector[0].updateTime,0);
      toLE2(rootDirectorySector[0].updateDate,0x21);	/* 1/1/80 */
    }
    checkStatus(vol.writeSector(vol.rec,sectorNo++,rootDirectorySector));
  }

  return flOK;
}


/*----------------------------------------------------------------------*/
/*      	          f l D o s F o r m a t				*/
/*									*/
/* Writes a DOS-FAT file system on the Flash volume			*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	formatParams	: Address of FormatParams structure to use	*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

FLStatus flDosFormat(TL vol, FormatParams FAR1 *formatParams)
{
  unsigned int cylinders;
  BPB bpb;

  getDriveGeometry(&vol,&bpb,&cylinders,formatParams->noOfFATcopies);

  checkStatus(createMasterBootRecord(&vol,&bpb,cylinders));

  checkStatus(createDOSbootSector(&vol,&bpb,formatParams->volumeId,formatParams->volumeLabel));

  checkStatus(createFATs(&vol,&bpb));

  checkStatus(createRootDirectory(&vol,&bpb,formatParams->volumeLabel));

  return flOK;
}

#endif

