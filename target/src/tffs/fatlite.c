/*
 * $Log:   P:/user/amir/lite/vcs/fatlite.c_v  $
 *
 *    Rev 1.46   06 Oct 1997 19:44:58   danig
 * COPY2
 *
 *    Rev 1.45   06 Oct 1997 10:28:54   danig
 * Check that drive handle < noOfDrives
 *
 *    Rev 1.44   29 Sep 1997 16:00:50   danig
 * Take the no. of hidden sectors from partition table in getBPB
 *
 *    Rev 1.43   29 Sep 1997 14:20:16   danig
 * flExitSocket & check findSector() return value
 *
 *    Rev 1.43   28 Sep 1997 19:08:02   danig
 * flExitSocket & check findSector return value
 *
 *    Rev 1.42   10 Sep 1997 17:51:40   danig
 * Got rid of generic names  &
 * big-endian + unaligned in replaceFATsector
 *
 *    Rev 1.41   04 Sep 1997 18:29:08   danig
 * Debug messages
 *
 *    Rev 1.40   02 Sep 1997 15:29:30   danig
 * Fixed flushBuffer for SINGLE_BUFFER
 *
 *    Rev 1.39   21 Aug 1997 14:06:20   unknown
 * Unaligned4
 *
 *    Rev 1.38   19 Aug 1997 20:05:38   danig
 * Andray's changes
 *
 *    Rev 1.37   17 Aug 1997 17:32:32   danig
 * Mutual exclusive mounting
 *
 *    Rev 1.37   17 Aug 1997 16:51:08   danig
 * Mutual exclusive mounting
 *
 *    Rev 1.36   14 Aug 1997 15:49:46   danig
 * Change in low level routines
 *
 *    Rev 1.35   04 Aug 1997 13:26:30   danig
 * Low level API & no irHandle in formatVolume
 *
 *    Rev 1.34   27 Jul 1997 14:35:58   danig
 * FAR -> FAR0 in split\joinFile
 *
 *    Rev 1.33   27 Jul 1997 11:59:24   amirban
 * Socket.volNo initialization
 *
 *    Rev 1.32   27 Jul 1997 10:12:32   amirban
 * replaceFATSector and name changes
 *
 *    Rev 1.30   21 Jul 1997 20:01:50   danig
 * Compilation issues
 *
 *    Rev 1.29   20 Jul 1997 18:44:10   danig
 *
 *    Rev 1.28   20 Jul 1997 18:34:50   danig
 * Fixed findFile with SET_ATTRIBUTES bug
 *
 *    Rev 1.27   20 Jul 1997 17:16:46   amirban
 * No watchDogTimer
 *
 *    Rev 1.26   07 Jul 1997 15:21:02   amirban
 * Ver 2.0
 *
 *    Rev 1.24   29 May 1997 14:59:00   amirban
 * Dismount closes files on one drive only
 *
 *    Rev 1.23   28 May 1997 18:51:28   amirban
 * setBusy(OFF) always
 *
 *    Rev 1.22   20 Apr 1997 10:48:50   danig
 * Fixed bug: fsFindFirstFile when subdirectories are undefined.
 *
 *    Rev 1.21   09 Apr 1997 17:35:42   amirban
 * Partition table redefined
 *
 *    Rev 1.20   31 Mar 1997 18:02:38   amirban
 * BPB redefined
 *
 v    Rev 1.19   03 Nov 1996 17:40:14   amirban
 * Correction for single-buffer
 *
 *    Rev 1.17   21 Oct 1996 18:10:20   amirban
 * Defragment I/F change
 *
 *    Rev 1.16   17 Oct 1996 18:56:36   danig
 * added big-endian to splitFile and joinFile
 *
 *    Rev 1.15   17 Oct 1996 16:21:46   danig
 * Audio features: splitFile, joinFile
 *
 *    Rev 1.14   17 Oct 1996 12:39:08   amirban
 * Buffer FAT sectors
 *
 *    Rev 1.13   03 Oct 1996 11:56:24   amirban
 * New Big-Endian
 *
 *    Rev 1.12   09 Sep 1996 11:38:52   amirban
 * Introduce sectorNotFound errors
 *
 *    Rev 1.11   29 Aug 1996 14:16:48   amirban
 * Warnings
 *
 *    Rev 1.10   20 Aug 1996 13:22:52   amirban
 * fsGetBPB
 *
 *    Rev 1.9   12 Aug 1996 15:46:26   amirban
 * Use setBusy instead of socketSetBusy
 *
 *    Rev 1.8   31 Jul 1996 14:29:58   amirban
 * Background stuff, and current file size in fsFindFile
 *
 *    Rev 1.7   14 Jul 1996 16:48:12   amirban
 * Format params
 *
 *    Rev 1.6   01 Jul 1996 15:41:26   amirban
 * new fsInit and modified fsGetDiskInfo
 *
 *    Rev 1.5   16 Jun 1996 14:04:06   amirban
 * Format does not need mounting
 *
 *    Rev 1.4   09 Jun 1996 18:15:42   amirban
 * Added fsExit, and VFAT rename mode
 *
 *    Rev 1.3   03 Jun 1996 16:26:24   amirban
 * Separated fsParsePath, and rename-file change
 *
 *    Rev 1.2   19 May 1996 19:31:46   amirban
 * Got rid of aliases in IOreq, and better fsParsePath
 *
 *    Rev 1.1   12 May 1996 20:05:26   amirban
 * Changes to findFile
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
#include "flsocket.h"
#include "dosformt.h"
#include "backgrnd.h"
#include "flbuffer.h"
#include "stdcomp.h"
#include "fatlite.h"

unsigned long flMsecCounter = 0;

/* Volume flag definitions */

#define VOLUME_LOW_LVL_MOUNTED  1       /* volume is mounted for low level operations */
#define	VOLUME_MOUNTED		2	/* Volume is mounted */
#define VOLUME_12BIT_FAT	4	/* Volume uses 12-bit FAT */
#define	VOLUME_ABS_MOUNTED	8	/* Volume is mounted for abs calls */

typedef struct {
  char		flags;			/* See description above */

  unsigned	sectorsPerCluster;	/* Cluster size in sectors */
  unsigned	maxCluster;		/* highest cluster no. */
  unsigned	bytesPerCluster;	/* Bytes per cluster */
  unsigned	bootSectorNo;		/* Sector no. of DOS boot sector */
  unsigned	firstFATSectorNo;	/* Sector no. of 1st FAT */
  unsigned	secondFATSectorNo;	/* Sector no. of 2nd FAT */
  unsigned	numberOfFATS;		/* number of FAT copies */
  unsigned	sectorsPerFAT;		/* Sectors per FAT copy */
  unsigned	rootDirectorySectorNo;	/* Sector no. of root directory */
  unsigned	sectorsInRootDirectory;	/* No. of sectors in root directory */
  unsigned	firstDataSectorNo;	/* 1st cluster sector no. */
  unsigned	allocationRover;	/* rover pointer for allocation */

#ifndef SINGLE_BUFFER
  #if FILES > 0
  FLBuffer	volBuffer;		/* Define a sector buffer */
  #endif
  FLMutex	volExecInProgress;
#endif

/* #ifdef LOW_LEVEL */
  FLFlash 	flash;			/* flash structure for low level operations */
/* #endif */

  TL		tl;			/* Translation layer methods */
  FLSocket	*socket;		/* Pointer to socket */
} Volume;


static FLBoolean initDone = FALSE;	/* Initialization already done */

static Volume 	vols[DRIVES];


typedef struct {
  long		currentPosition;	/* current byte offset in file */
#define		ownerDirCluster currentPosition	/* 1st cluster of owner directory */
  long 		fileSize;		/* file size in bytes */
  SectorNo	directorySector;	/* sector of directory containing file */
  unsigned      currentCluster;		/* cluster of current position */
  unsigned char	directoryIndex;		/* entry no. in directory sector */
  unsigned char	flags;			/* See description below */
  Volume *	fileVol;		/* Drive of file */
} File;

/* File flag definitions */

#define	FILE_MODIFIED		4	/* File was modified */
#define FILE_IS_OPEN		8	/* File entry is used */
#define	FILE_IS_DIRECTORY    0x10	/* File is a directory */
#define	FILE_IS_ROOT_DIR     0x20	/* File is root directory */
#define	FILE_READ_ONLY	     0x40	/* Writes not allowed */
#define	FILE_MUST_OPEN       0x80	/* Create file if not found */


#if FILES > 0
static File 	fileTable[FILES];	/* the file table */

#endif	/* FILES > 0 */

#ifdef SINGLE_BUFFER

FLBuffer buffer;
FLMutex execInProgress;

#else

#define buffer (vol.volBuffer)
#define execInProgress (vol.volExecInProgress)

#endif

#define directory ((DirectoryEntry *) buffer.data)



/*----------------------------------------------------------------------*/
/*      	           s e t B u s y				*/
/*									*/
/* Notifies the start and end of a file-system operation.		*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*      state		: TFFS_ON (1) = operation entry			*/
/*			  TFFS_OFF(0) = operation exit			*/
/*                                                                      */
/*----------------------------------------------------------------------*/

static FLStatus setBusy(Volume vol, FLBoolean state)
{
  if (state == TFFS_ON) {
    if (!flTakeMutex(&execInProgress,1))
      return flDriveNotAvailable;
    flSocketSetBusy(vol.socket,TFFS_ON);
    flNeedVcc(vol.socket);
    if (vol.flags & VOLUME_ABS_MOUNTED)
      vol.tl.tlSetBusy(vol.tl.rec,TFFS_ON);
  }
  else {
    if (vol.flags & VOLUME_ABS_MOUNTED)
      vol.tl.tlSetBusy(vol.tl.rec,TFFS_OFF);
    flDontNeedVcc(vol.socket);
    flSocketSetBusy(vol.socket,TFFS_OFF);
    flFreeMutex(&execInProgress);
  }

  return flOK;
}



/*----------------------------------------------------------------------*/
/*		          f i n d S e c t o r				*/
/*									*/
/* Locates a sector in the buffer or maps it				*/
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	sectorNo	: Sector no. to locate				*/
/*                                                                      */
/*----------------------------------------------------------------------*/


static const void FAR0 *findSector(Volume vol, SectorNo sectorNo)
{
CardAddress *physAddress =0;
  return
#if FILES > 0
	 (sectorNo == buffer.sectorNo && &vol == buffer.owner) ?
	 buffer.data :
#endif
	 vol.tl.mapSector(vol.tl.rec,sectorNo, physAddress);
}


#if FILES > 0
static FLStatus closeFile(File *file);	/* forward */
static FLStatus flushBuffer(Volume vol);	/* forward */
#endif

/*----------------------------------------------------------------------*/
/*		      d i s m o u n t V o l u m e			*/
/*									*/
/* Dismounts the volume, closing all files.				*/
/* This call is not normally necessary, unless it is known the volume   */
/* will soon be removed.						*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

static FLStatus dismountVolume(Volume vol)
{
#if FILES > 0
  int i;
#endif

  if (vol.flags & VOLUME_ABS_MOUNTED) {
    FLStatus status = flOK;
#ifndef FIXED_MEDIA
    status = flMediaCheck(vol.socket);
#endif
    if (status != flOK)
      vol.flags = 0;
#if FILES > 0
    else {
      checkStatus(flushBuffer(&vol));
    }

    /* Close or discard all files and make them available */
    for (i = 0; i < FILES; i++)
      if (fileTable[i].fileVol == &vol)
	if (vol.flags & VOLUME_MOUNTED)
	  closeFile(&fileTable[i]);
	else
	  fileTable[i].flags = 0;

    vol.tl.dismount(vol.tl.rec);

    buffer.sectorNo = UNASSIGNED_SECTOR;	/* Current sector no. (none) */
    buffer.dirty = buffer.checkPoint = FALSE;
#endif	/* FILES > 0 */
  }

  vol.flags = 0;	/* mark volume unmounted */

  return flOK;
}


/*----------------------------------------------------------------------*/
/*		        m o u n t V o l u m e				*/
/*									*/
/* Mounts the Flash volume.						*/
/*									*/
/* In case the inserted volume has changed, or on the first access to   */
/* the file system, it should be mounted before file operations can be  */
/* done on it.								*/
/* Mounting a volume has the effect of discarding all open files (the   */
/* files cannot be properly closed since the original volume is gone),  */
/* and turning off the media-change indication to allow file processing */
/* calls.								*/
/*									*/
/* The volume automatically becomes unmounted if it is removed or       */
/* changed.								*/
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

static FLStatus mountVolume(Volume vol)
{
  SectorNo noOfSectors;
  PartitionTable FAR0 *partitionTable;
  DOSBootSector FAR0 *bootSector;


  checkStatus(dismountVolume(&vol));

  checkStatus(flMount(&vol - vols,&vol.tl,&vol.flash)); /* Try to mount translation layer */

  /* Read in paritition table */
  partitionTable = (PartitionTable FAR0 *) findSector(&vol,0);
  if(partitionTable == NULL)
    return flSectorNotFound;

  /* *BUG* *BUG* Doesn't work if SECTOR_SIZE < 512 */

  if (LE2(partitionTable->signature) == PARTITION_SIGNATURE &&
      partitionTable->type != 0)
    vol.bootSectorNo =
	(unsigned) UNAL4(partitionTable->startingSectorOfPartition);
  else
    vol.bootSectorNo = 0;	/* If partition table is undetected,
				   assume sector 0 is DOS boot block */

  vol.firstFATSectorNo = vol.secondFATSectorNo = 0; /* Disable FAT monitoring */
  vol.flags |= VOLUME_ABS_MOUNTED;  /* Enough to do abs operations */

  bootSector = (DOSBootSector FAR0 *) findSector(&vol,vol.bootSectorNo);
  if(bootSector == NULL)
    return flSectorNotFound;

  /* Do the customary sanity checks */
  if (!(bootSector->bpb.jumpInstruction[0] == 0xe9 ||
	(bootSector->bpb.jumpInstruction[0] == 0xeb &&
	 bootSector->bpb.jumpInstruction[2] == 0x90))) {
  #ifdef DEBUG_PRINT
    DEBUG_PRINT("Debug: did not recognize format.\n");
  #endif
    return flNonFATformat;
  }

  /* See if we handle this sector size */
  if (UNAL2(bootSector->bpb.bytesPerSector) != SECTOR_SIZE)
    return flFormatNotSupported;

  vol.sectorsPerCluster = bootSector->bpb.sectorsPerCluster;
  vol.numberOfFATS = bootSector->bpb.noOfFATS;
  vol.sectorsPerFAT = LE2(bootSector->bpb.sectorsPerFAT);
  vol.firstFATSectorNo = vol.bootSectorNo +
			    LE2(bootSector->bpb.reservedSectors);
  vol.secondFATSectorNo = vol.firstFATSectorNo +
			     LE2(bootSector->bpb.sectorsPerFAT);
  vol.rootDirectorySectorNo = vol.firstFATSectorNo +
		   bootSector->bpb.noOfFATS * LE2(bootSector->bpb.sectorsPerFAT);
  vol.sectorsInRootDirectory =
	(UNAL2(bootSector->bpb.rootDirectoryEntries) * DIRECTORY_ENTRY_SIZE - 1) /
		SECTOR_SIZE + 1;
  vol.firstDataSectorNo = vol.rootDirectorySectorNo +
			     vol.sectorsInRootDirectory;

  noOfSectors = UNAL2(bootSector->bpb.totalSectorsInVolumeDOS3);
  if (noOfSectors == 0)
    noOfSectors = (SectorNo) LE4(bootSector->bpb.totalSectorsInVolume);

  vol.maxCluster = (unsigned) ((noOfSectors - vol.firstDataSectorNo) /
				vol.sectorsPerCluster) + 1;
  if (vol.maxCluster < 4085) {
#ifdef FAT_12BIT
    vol.flags |= VOLUME_12BIT_FAT;	/* 12-bit FAT */
#else
#ifdef DEBUG_PRINT
    DEBUG_PRINT("Debug: FAT_12BIT must be defined.\n");
#endif
    return flFormatNotSupported;
#endif
  }
  vol.bytesPerCluster = vol.sectorsPerCluster * SECTOR_SIZE;
  vol.allocationRover = 2;	/* Set rover at first cluster */
  vol.flags |= VOLUME_MOUNTED;	/* That's it */

  return flOK;
}


#if defined(DEFRAGMENT_VOLUME) || defined(SINGLE_BUFFER)

/*----------------------------------------------------------------------*/
/*		      	 d e f r a g m e n t V o l u m e		*/
/*									*/
/* Performs a general defragmentation and recycling of non-writable	*/
/* Flash areas, to achieve optimal write speed.				*/
/*                                                                      */
/* NOTE: The required number of sectors (in irLength) may be changed    */
/* (from another execution thread) while defragmentation is active. In  */
/* particular, the defragmentation may be cut short after it began by	*/
/* modifying the irLength field to 0.					*/
/*									*/
/* Parameters:                                                          */
/*	ioreq->irLength	: Minimum number of sectors to make available   */
/*			  for writes.					*/
/*                                                                      */
/* Returns:                                                             */
/*	ioreq->irLength	: Actual number of sectors available for writes	*/
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

static FLStatus defragmentVolume(Volume vol, IOreq FAR2 *ioreq)
{
  return vol.tl.defragment(vol.tl.rec,&ioreq->irLength);
}

#endif /* DEFRAGMENT_VOLUME */


#ifdef FORMAT_VOLUME

/*----------------------------------------------------------------------*/
/*		      f l F o r m a t V o l u m e			*/
/*									*/
/* Formats a volume, writing a new and empty file-system. All existing  */
/* data is destroyed. Optionally, a low-level FTL formatting is also    */
/* done.								*/
/* Formatting leaves the volume in the dismounted state, so that a	*/
/* flMountVolume call is necessary after it.				*/
/*                                                                      */
/* Parameters:                                                          */
/*	irHandle	: Drive number (0, 1, ...)			*/
/*	irFlags		: NO_FTL_FORMAT: Do FAT formatting only		*/
/*			  FTL_FORMAT: Do FTL & FAT formatting           */
/*			  FTL_FORMAT_IF_NEEDED: Do FTL formatting only	*/
/*				      if current format is invalid	*/
/*	irData		: Address of FormatParams structure to use	*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

static FLStatus formatVolume(Volume vol, IOreq FAR2 *ioreq)
{
  FormatParams FAR1 *formatParams = (FormatParams FAR1 *) ioreq->irData;

  checkStatus(dismountVolume(&vol));

  if (ioreq->irFlags == FTL_FORMAT) {
    checkStatus(flFormat(&vol - vols,formatParams));
  }
  else {
    FLStatus status = flMount(&vol - vols,&vol.tl,&vol.flash);

    if ((status == flUnknownMedia || status == flBadFormat) &&
	ioreq->irFlags == FTL_FORMAT_IF_NEEDED)
      status = flFormat(&vol - vols,formatParams);

    if (status != flOK)
      return status;
  }

  checkStatus(flMount(&vol - vols,&vol.tl,&vol.flash));
  checkStatus(flDosFormat(&vol.tl,formatParams));

  return flOK;
}

#endif /* FORMAT_VOLUME */


#ifdef ABS_READ_WRITE

/*----------------------------------------------------------------------*/
/*		             a b s R e a d 				*/
/*									*/
/* Reads absolute sectors by sector no.					*/
/*									*/
/* Parameters:                                                          */
/*	irHandle	: Drive number (0, 1, ...)			*/
/*      irData		: Address of user buffer to read into		*/
/*	irSectorNo	: First sector no. to read (sector 0 is the	*/
/*			  DOS boot sector).				*/
/*	irSectorCount	: Number of consectutive sectors to read	*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*	irSectorCount	: Number of sectors actually read		*/
/*----------------------------------------------------------------------*/

static FLStatus absRead(Volume vol, IOreq FAR2 *ioreq)
{
  char FAR1 *userBuffer = (char FAR1 *) ioreq->irData;
  SectorNo currSector = vol.bootSectorNo + ioreq->irSectorNo;
  unsigned sectorCount = ioreq->irSectorCount;

  for (ioreq->irSectorCount = 0;
       ioreq->irSectorCount < sectorCount;
       ioreq->irSectorCount++, currSector++, userBuffer += SECTOR_SIZE) {
    const void FAR0 *mappedSector = findSector(&vol,currSector);
    if (mappedSector)
      tffscpy(userBuffer,mappedSector,SECTOR_SIZE);
    else
      tffsset(userBuffer,0,SECTOR_SIZE);
  }

  return flOK;
}


/*----------------------------------------------------------------------*/
/*		      r e p l a c e F A T s e c t o r 			*/
/*									*/
/* Monitors sector deletions in the FAT.				*/
/*									*/
/* When a FAT block is about to be written by an absolute write, this   */
/* routine will first scan whether any sectors are being logically	*/
/* deleted by this FAT update, and if so, it will delete-sector them	*/
/* before the actual FAT update takes place.				*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	sectorNo	: FAT Sector no. about to be written		*/
/*      newFATsector	: Address of FAT sector about to be written	*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

FLStatus replaceFATsector(Volume vol,
			SectorNo sectorNo,
			const char FAR1 *newFATsector)
{
  const char FAR0 *oldFATsector = (const char FAR0 *) findSector(&vol,sectorNo);

#ifdef FAT_12BIT
  unsigned FAThalfBytes = vol.flags & VOLUME_12BIT_FAT ? 3 : 4;

  unsigned firstCluster =
	(FAThalfBytes == 3) ?
	(((unsigned) (sectorNo - vol.firstFATSectorNo) * (2 * SECTOR_SIZE) + 2) / 3) :
	((unsigned) (sectorNo - vol.firstFATSectorNo) * (SECTOR_SIZE / 2));
  SectorNo firstSector =
	((SectorNo) firstCluster - 2) * vol.sectorsPerCluster + vol.firstDataSectorNo;
  unsigned int halfByteOffset =
	(firstCluster * FAThalfBytes) & (2 * SECTOR_SIZE - 1);

  if (oldFATsector == NULL)
    return flOK;

  /* Find if any clusters were logically deleted, and if so, delete them */
  /* NOTE: We are skipping over 12-bit FAT entries which span more than  */
  /*       one sector. Nobody's perfect anyway.                          */
  for (; halfByteOffset < (2 * SECTOR_SIZE - 2);
       firstSector += vol.sectorsPerCluster, halfByteOffset += FAThalfBytes) {
    unsigned short oldFATentry, newFATentry;

#ifdef TFFS_BIG_ENDIAN
    oldFATentry = LE2(*(LEushort FAR0 *)(oldFATsector + (halfByteOffset / 2)));
    newFATentry = LE2(*(LEushort FAR1 *)(newFATsector + (halfByteOffset / 2)));
#else
    oldFATentry = UNAL2(*(Unaligned FAR0 *)(oldFATsector + (halfByteOffset / 2)));
    newFATentry = UNAL2(*(Unaligned FAR1 *)(newFATsector + (halfByteOffset / 2)));
#endif /* TFFS_BIG_ENDIAN */
    if (halfByteOffset & 1) {
      oldFATentry >>= 4;
      newFATentry >>= 4;
    }
    else if (FAThalfBytes == 3) {
      oldFATentry &= 0xfff;
      newFATentry &= 0xfff;
    }
#else
  unsigned firstCluster =
	((unsigned) (sectorNo - vol.firstFATSectorNo) * (SECTOR_SIZE / 2));
  SectorNo firstSector =
	((SectorNo) firstCluster - 2) * vol.sectorsPerCluster + vol.firstDataSectorNo;
  unsigned int byteOffset;

  if (oldFATsector == NULL)
    return flOK;

  /* Find if any clusters were logically deleted, and if so, delete them */
  for (byteOffset = 0; byteOffset < SECTOR_SIZE;
       firstSector += vol.sectorsPerCluster, byteOffset += 2) {
    unsigned short oldFATentry = LE2(*(LEushort FAR0 *)(oldFATsector + byteOffset));
    unsigned short newFATentry = LE2(*(LEushort FAR1 *)(newFATsector + byteOffset));
#endif

    if (oldFATentry != FAT_FREE && newFATentry == FAT_FREE)
      checkStatus(vol.tl.deleteSector(vol.tl.rec,firstSector,vol.sectorsPerCluster));

    /* make sure sector is still there */
    oldFATsector = (const char FAR0 *) findSector(&vol,sectorNo);
  }

  return flOK;
}

/*----------------------------------------------------------------------*/
/*                  d i s a b l e F A T m o n i t o r                   */
/*                                                                      */
/* Turns off FAT monitor.                                               */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*                                                                      */
/* Returns:                                                             */
/*      always flOK                                                     */
/*----------------------------------------------------------------------*/

static FLStatus disableFATmonitor(Volume vol)
{
  vol.firstFATSectorNo = vol.secondFATSectorNo = 0; /* Disable FAT monitoring */
  return flOK;
}

/*----------------------------------------------------------------------*/
/*		           a b s W r i t e 				*/
/*									*/
/* Writes absolute sectors by sector no.				*/
/*									*/
/* Parameters:                                                          */
/*	irHandle	: Drive number (0, 1, ...)			*/
/*      irData		: Address of user buffer to write from		*/
/*	irSectorNo	: First sector no. to write (sector 0 is the	*/
/*			  DOS boot sector).				*/
/*	irSectorCount	: Number of consectutive sectors to write	*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*	irSectorCount	: Number of sectors actually written		*/
/*----------------------------------------------------------------------*/

static FLStatus absWrite(Volume vol, IOreq FAR2 *ioreq)
{
  char FAR1 *userBuffer = (char FAR1 *) ioreq->irData;
  SectorNo currSector = vol.bootSectorNo + ioreq->irSectorNo;
  unsigned sectorCount = ioreq->irSectorCount;

  if (currSector < vol.secondFATSectorNo &&
      currSector + sectorCount > vol.firstFATSectorNo) {
    unsigned iSector;

    for (iSector = 0; iSector < sectorCount;
	 iSector++, currSector++, userBuffer += SECTOR_SIZE) {
      if (currSector >= vol.firstFATSectorNo &&
	  currSector < vol.secondFATSectorNo)
	replaceFATsector(&vol,currSector,userBuffer);
    }

    userBuffer = (char FAR1 *) ioreq->irData;
    currSector = vol.bootSectorNo + ioreq->irSectorNo;
  }

  for (ioreq->irSectorCount = 0;
       ioreq->irSectorCount < sectorCount;
       ioreq->irSectorCount++, currSector++, userBuffer += SECTOR_SIZE) {
    checkStatus(vol.tl.writeSector(vol.tl.rec,currSector,userBuffer));
  }

  return flOK;
}


/*----------------------------------------------------------------------*/
/*		          a b s D e l e t e 				*/
/*									*/
/* Marks absolute sectors by sector no. as deleted.			*/
/*									*/
/* Parameters:                                                          */
/*	irHandle	: Drive number (0, 1, ...)			*/
/*	irSectorNo	: First sector no. to write (sector 0 is the	*/
/*			  DOS boot sector).				*/
/*	irSectorCount	: Number of consectutive sectors to delete	*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

static FLStatus absDelete(Volume vol, IOreq FAR2 *ioreq)
{
  return vol.tl.deleteSector(vol.tl.rec,vol.bootSectorNo + ioreq->irSectorNo,ioreq->irSectorCount);
}


/*----------------------------------------------------------------------*/
/*		             g e t B P B 				*/
/*									*/
/* Reads the BIOS Parameter Block from the boot sector			*/
/*									*/
/* Parameters:                                                          */
/*	irHandle	: Drive number (0, 1, ...)			*/
/*      irData		: Address of user buffer to read BPB into	*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

static FLStatus getBPB(Volume vol, IOreq FAR2 *ioreq)
{
  BPB FAR1 *userBPB = (BPB FAR1 *) ioreq->irData;
  DOSBootSector FAR0 *bootSector;
  unsigned long noOfHiddenSectors;
  PartitionTable FAR0 *partitionTable;

  bootSector = (DOSBootSector FAR0 *) findSector(&vol,vol.bootSectorNo);
  if(bootSector == NULL)
    return flSectorNotFound;

  *userBPB = bootSector->bpb;
#if	FALSE
  tffscpy (userBPB, &bootSector->bpb, sizeof(BPB));
#endif	/* FALSE */

  /* take the number of hidden sectors from the partition table in case
     we don't have DOS format BPB					*/
  partitionTable = (PartitionTable FAR0 *) findSector(&vol,0);
  if(partitionTable == NULL)
    return flSectorNotFound;

  if (LE2(partitionTable->signature) == PARTITION_SIGNATURE &&
      partitionTable->type != 0) {
    noOfHiddenSectors = UNAL4(partitionTable->startingSectorOfPartition);
    toLE4(userBPB->noOfHiddenSectors, noOfHiddenSectors);
  }

  return flOK;
}

#endif /* ABS_READ_WRITE */

#ifdef LOW_LEVEL

/*----------------------------------------------------------------------*/
/*		             m o u n t L o w L e v e l 			*/
/*									*/
/* Mount a volume for low level operations. If a low level routine is   */
/* called and the volume is not mounted for low level operations, this	*/
/* routine is called atomatically. 					*/
/*									*/
/* Parameters:								*/
/*	vol		: Pointer identifying drive			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

static FLStatus mountLowLevel(Volume vol)
{
  checkStatus(flIdentifyFlash(vol.socket,&vol.flash));
  vol.flash.setPowerOnCallback(&vol.flash);
  vol.flags |= VOLUME_LOW_LVL_MOUNTED;

  return flOK;
}

/*----------------------------------------------------------------------*/
/*		             d i s m o u n t L o w L e v e l 		*/
/*									*/
/* Dismount the volume for low level operations.			*/
/*									*/
/* Parameters:								*/
/*	vol		: Pointer identifying drive			*/
/*                                                                      */
/*----------------------------------------------------------------------*/

static void dismountLowLevel(Volume vol)
{
  /* mark the volume as unmounted for low level operations, untouch other flags */
  vol.flags &= ~VOLUME_LOW_LVL_MOUNTED;
}

/*----------------------------------------------------------------------*/
/*		             g e t P h y s i c a l I n f o 		*/
/*									*/
/* Get physical information of the media. The information includes	*/
/* JEDEC ID, unit size and media size.					*/
/*									*/
/* Parameters:								*/
/*	vol		: Pointer identifying drive			*/
/*      irData		: Address of user buffer to read physical	*/
/*			  information into.				*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

static FLStatus getPhysicalInfo(Volume vol, IOreq FAR2 *ioreq)
{
  PhysicalInfo FAR2 *physicalInfo = (PhysicalInfo FAR2 *)ioreq->irData;

  physicalInfo->type = vol.flash.type;
  physicalInfo->unitSize = vol.flash.erasableBlockSize;
  physicalInfo->mediaSize = vol.flash.chipSize * vol.flash.noOfChips;

  return flOK;
}

/*----------------------------------------------------------------------*/
/*		             p h y s i c a l R e a d	 		*/
/*									*/
/* Read from a physical address.					*/
/*									*/
/* Parameters:								*/
/*	vol		: Pointer identifying drive			*/
/*	irAddress	: Physical address to read from.		*/
/*	irByteCount	: Number of bytes to read.			*/
/*      irData		: Address of user buffer to read into.		*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

static FLStatus physicalRead(Volume vol, IOreq FAR2 *ioreq)
{
  /* check that we are reading whithin the media boundaries */
  if (ioreq->irAddress + (long)ioreq->irByteCount > vol.flash.chipSize *
				    vol.flash.noOfChips)
    return flBadParameter;

  /* We don't read accross a unit boundary */
  if ((long)ioreq->irByteCount > vol.flash.erasableBlockSize -
			   (ioreq->irAddress % vol.flash.erasableBlockSize))
    return flBadParameter;

  vol.flash.read(&vol.flash, ioreq->irAddress, ioreq->irData,
		 ioreq->irByteCount, 0);
  return flOK;
}

/*----------------------------------------------------------------------*/
/*		             p h y s i c a l W r i t e	 		*/
/*									*/
/* Write to a physical address.						*/
/*									*/
/* Parameters:								*/
/*	vol		: Pointer identifying drive			*/
/*	irAddress	: Physical address to write to.			*/
/*	irByteCount	: Number of bytes to write.			*/
/*      irData		: Address of user buffer to write from.		*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

static FLStatus physicalWrite(Volume vol, IOreq FAR2 *ioreq)
{
  /* check that we are writing whithin the media boundaries */
  if (ioreq->irAddress + (long)ioreq->irByteCount > vol.flash.chipSize *
					       vol.flash.noOfChips)
    return flBadParameter;

  /* We don't write accross a unit boundary */
  if ((long)ioreq->irByteCount > vol.flash.erasableBlockSize -
			   (ioreq->irAddress % vol.flash.erasableBlockSize))
    return flBadParameter;

  checkStatus(vol.flash.write(&vol.flash, ioreq->irAddress, ioreq->irData,
			      ioreq->irByteCount, 0));
  return flOK;
}

/*----------------------------------------------------------------------*/
/*		             p h y s i c a l E r a s e	 		*/
/*									*/
/* Erase physical units.						*/
/*									*/
/* Parameters:								*/
/*	vol		: Pointer identifying drive			*/
/*	irUnitNo	: First unit to erase.				*/
/*	irUnitCount	: Number of units to erase.			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

static FLStatus physicalErase(Volume vol, IOreq FAR2 *ioreq)
{
  if (ioreq->irUnitNo + (long)ioreq->irUnitCount > vol.flash.chipSize * vol.flash.noOfChips /
					       vol.flash.erasableBlockSize)
    return flBadParameter;

  checkStatus(vol.flash.erase(&vol.flash, ioreq->irUnitNo, ioreq->irUnitCount));
  return flOK;
}

#endif /* LOW_LEVEL */

#if FILES > 0

/*----------------------------------------------------------------------*/
/*		         f l u s h B u f f e r				*/
/*									*/
/* Writes the buffer contents if it is dirty.                           */
/*									*/
/* If this is a FAT sector, all FAT copies are written.			*/
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

static FLStatus flushBuffer(Volume vol)
{
  if (buffer.dirty) {
    FLStatus status;
    unsigned i;

#ifdef SINGLE_BUFFER
    Volume *bufferOwner = (Volume *) buffer.owner;

    if (&vol != bufferOwner) {
      flFreeMutex(&execInProgress);
      checkStatus(setBusy(bufferOwner,TFFS_ON));
    }
#else
    Volume *bufferOwner = &vol;
#endif

    status = (*bufferOwner).tl.writeSector((*bufferOwner).tl.rec, buffer.sectorNo,
					   buffer.data);
    if (status == flOK) {
      if (buffer.sectorNo >= (*bufferOwner).firstFATSectorNo &&
	  buffer.sectorNo < (*bufferOwner).secondFATSectorNo)
	for (i = 1; i < (*bufferOwner).numberOfFATS; i++)
	  checkStatus((*bufferOwner).tl.writeSector((*bufferOwner).tl.rec,
						    buffer.sectorNo + i * (*bufferOwner).sectorsPerFAT,
						    buffer.data));
    }
    else
      buffer.sectorNo = UNASSIGNED_SECTOR;
#ifdef SINGLE_BUFFER
    if (&vol != bufferOwner) {
      setBusy(bufferOwner,TFFS_OFF);
      if (!flTakeMutex(&execInProgress,1))
	return flGeneralFailure;
    }
#endif
    buffer.dirty = buffer.checkPoint = FALSE;

    return status;
  }
  else
    return flOK;
}


/*----------------------------------------------------------------------*/
/*		        u p d a t e S e c t o r				*/
/*									*/
/* Prepares a sector for update in the buffer				*/
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	sectorNo	: Sector no. to read				*/
/*	read		: Whether to initialize buffer by reading, or	*/
/*                        clearing					*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

static FLStatus updateSector(Volume vol, SectorNo sectorNo, FLBoolean read)
{
  if (sectorNo != buffer.sectorNo || &vol != buffer.owner) {
    const void FAR0 *mappedSector;

    checkStatus(flushBuffer(&vol));
#ifdef SINGLE_BUFFER
  }

  if (!buffer.dirty) {
    long sectorsNeeded = 20;
    checkStatus(vol.tl.defragment(vol.tl.rec,&sectorsNeeded));
  }

  if (sectorNo != buffer.sectorNo || &vol != buffer.owner) {
    const void FAR0 *mappedSector;
#endif
    buffer.sectorNo = sectorNo;
    buffer.owner = &vol;
    if (read) {
      mappedSector = vol.tl.mapSector(vol.tl.rec,sectorNo);
      if (mappedSector)
	tffscpy(buffer.data,mappedSector,SECTOR_SIZE);
      else
	return flSectorNotFound;
    }
    else
      tffsset(buffer.data,0,SECTOR_SIZE);
  }

  buffer.dirty = TRUE;

  return flOK;
}


/*----------------------------------------------------------------------*/
/*		   f i r s t S e c t o r O f C l u s t e r		*/
/*									*/
/* Get sector no. corresponding to cluster no.				*/
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	cluster		: Cluster no.                                   */
/*                                                                      */
/* Returns:                                                             */
/*	first sector no. of cluster					*/
/*----------------------------------------------------------------------*/

static SectorNo firstSectorOfCluster(Volume vol, unsigned cluster)
{
  return (SectorNo) (cluster - 2) * vol.sectorsPerCluster +
	 vol.firstDataSectorNo;
}


/*----------------------------------------------------------------------*/
/*		       g e t D i r E n t r y    			*/
/*									*/
/* Get a read-only copy of a directory entry.				*/
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	file		: File belonging to directory entry		*/
/*                                                                      */
/* Returns:                                                             */
/*	dirEntry	: Pointer to directory entry			*/
/*----------------------------------------------------------------------*/

static const DirectoryEntry FAR0 *getDirEntry(File *file)
{
  return (DirectoryEntry FAR0 *) findSector(file->fileVol,file->directorySector) +
	 file->directoryIndex;
}


/*----------------------------------------------------------------------*/
/*		  g e t D i r E n t r y F o r U p d a t e		*/
/*									*/
/* Read a directory sector into the sector buffer and point to an	*/
/* entry, with the intention of modifying it.				*/
/* The buffer will be flushed on operation exit.			*/
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	file		: File belonging to directory entry		*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*	dirEntry	: Pointer to directory entry in buffer		*/
/*----------------------------------------------------------------------*/

static FLStatus getDirEntryForUpdate(File *file, DirectoryEntry * *dirEntry)
{
  Volume vol = file->fileVol;

  checkStatus(updateSector(file->fileVol,file->directorySector,TRUE));
  *dirEntry = directory + file->directoryIndex;
  buffer.checkPoint = TRUE;

  return flOK;
}


/*----------------------------------------------------------------------*/
/*		  s e t C u r r e n t D a t e T i m e                   */
/*									*/
/* Set current time/date in directory entry                             */
/*                                                                      */
/* Parameters:                                                          */
/*	dirEntry	: Pointer to directory entry                    */
/*                                                                      */
/*----------------------------------------------------------------------*/

static void setCurrentDateTime(DirectoryEntry *dirEntry)
{
  toLE2(dirEntry->updateTime,flCurrentTime());
  toLE2(dirEntry->updateDate,flCurrentDate());
}


/*----------------------------------------------------------------------*/
/*		        g e t F A T e n t r y				*/
/*									*/
/* Get an entry from the FAT. The 1st FAT copy is used.			*/
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	cluster		: Cluster no. of enrty.				*/
/*                                                                      */
/* Returns:                                                             */
/*	Value of FAT entry.						*/
/*----------------------------------------------------------------------*/

static unsigned getFATentry(Volume vol, unsigned cluster)
{
  LEushort FAR0 *fat16Sector;

  unsigned fatSectorNo = vol.firstFATSectorNo;
#ifdef FAT_12BIT
  if (vol.flags & VOLUME_12BIT_FAT)
    fatSectorNo += (cluster * 3) >> (SECTOR_SIZE_BITS + 1);
  else
#endif
    fatSectorNo += cluster >> (SECTOR_SIZE_BITS - 1);

#ifndef SINGLE_BUFFER
  if (!buffer.dirty) {
    /* If the buffer is free, use it to store this FAT sector */
    updateSector(&vol,fatSectorNo,TRUE);
    buffer.dirty = FALSE;
  }
#endif
  fat16Sector = (LEushort FAR0 *) findSector(&vol,fatSectorNo);

#ifdef FAT_12BIT
  if (vol.flags & VOLUME_12BIT_FAT) {
    unsigned char FAR0 *fat12Sector = (unsigned char FAR0 *) fat16Sector;
    unsigned halfByteOffset = (cluster * 3) & (SECTOR_SIZE * 2 - 1);
    unsigned char firstByte = fat12Sector[halfByteOffset / 2];
    halfByteOffset += 2;
    if (halfByteOffset >= SECTOR_SIZE * 2) {
      /* Entry continues on the next sector. What a mess */
      halfByteOffset -= SECTOR_SIZE * 2;
      fat12Sector = (unsigned char FAR0 *) findSector(&vol,fatSectorNo + 1);
    }
    if (halfByteOffset & 1)
      return ((firstByte & 0xf0) >> 4) + (fat12Sector[halfByteOffset / 2] << 4);
    else
      return firstByte + ((fat12Sector[halfByteOffset / 2] & 0xf) << 8);
  }
  else
#endif
    return LE2(fat16Sector[cluster & (SECTOR_SIZE / 2 - 1)]);
}


/*----------------------------------------------------------------------*/
/*		         s e t F A T e n t r y				*/
/*									*/
/* Writes a new value to a given FAT cluster entry.                     */
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	cluster		: Cluster no. of enrty.				*/
/*	entry		: New value of FAT entry.			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

static FLStatus setFATentry(Volume vol, unsigned cluster, unsigned entry)
{
  LEushort *fat16Sector;

  unsigned fatSectorNo = vol.firstFATSectorNo;
#ifdef FAT_12BIT
  if (vol.flags & VOLUME_12BIT_FAT)
    fatSectorNo += (cluster * 3) >> (SECTOR_SIZE_BITS + 1);
  else
#endif
    fatSectorNo += cluster >> (SECTOR_SIZE_BITS - 1);

  checkStatus(updateSector(&vol,fatSectorNo,TRUE));
  fat16Sector = (LEushort *) buffer.data;

#ifdef FAT_12BIT
  if (vol.flags & VOLUME_12BIT_FAT) {
    unsigned char *fat12Sector = (unsigned char *) buffer.data;
    unsigned halfByteOffset = (cluster * 3) & (SECTOR_SIZE * 2 - 1);
    if (halfByteOffset & 1) {
      fat12Sector[halfByteOffset / 2] &= 0xf;
      fat12Sector[halfByteOffset / 2] |= (entry & 0xf) << 4;
    }
    else
      fat12Sector[halfByteOffset / 2] = entry;
    halfByteOffset += 2;
    if (halfByteOffset >= SECTOR_SIZE * 2) {
      /* Entry continues on the next sector. What a mess */
      halfByteOffset -= SECTOR_SIZE * 2;
      fatSectorNo++;
      checkStatus(updateSector(&vol,fatSectorNo,TRUE));
    }
    if (halfByteOffset & 1)
      fat12Sector[halfByteOffset / 2] = entry >> 4;
    else {
      fat12Sector[halfByteOffset / 2] &= 0xf0;
      fat12Sector[halfByteOffset / 2] |= (entry & 0x0f00) >> 8;
    }
  }
  else
#endif
    toLE2(fat16Sector[cluster & (SECTOR_SIZE / 2 - 1)],entry);

  return flOK;
}


/*----------------------------------------------------------------------*/
/*		        a l l o c a t e C l u s t e r			*/
/*									*/
/* Allocates a new cluster for a file and adds it to a FAT chain or     */
/* marks it in a directory entry.					*/
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	file		: File to extend. It should be positioned at    */
/*			  end-of-file.					*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

static FLStatus allocateCluster(File *file)
{
  Volume vol = file->fileVol;
  unsigned originalRover;

  if (file->flags & FILE_READ_ONLY)
    return flNoWriteAccess;

  /* Look for a free cluster. Start at the allocation rover */
  originalRover = vol.allocationRover;

  do {
    vol.allocationRover++;
    if (vol.allocationRover > vol.maxCluster)
      vol.allocationRover = 2;	/* wraparound to start of volume */
    if (vol.allocationRover == originalRover)
      return flNoSpaceInVolume;
  } while (getFATentry(&vol,vol.allocationRover) != FAT_FREE);

  /* Found a free cluster. Mark it as an end of chain */
  checkStatus(setFATentry(&vol,vol.allocationRover,FAT_LAST_CLUSTER));

  /* Mark previous cluster or directory to point to it */
  if (file->currentCluster == 0) {
    DirectoryEntry *dirEntry;
    checkStatus(getDirEntryForUpdate(file,&dirEntry));

    toLE2(dirEntry->startingCluster,vol.allocationRover);
    setCurrentDateTime(dirEntry);
  }
  else
    checkStatus(setFATentry(&vol,file->currentCluster,vol.allocationRover));

  /* Set our new current cluster */
  file->currentCluster = vol.allocationRover;

  return flOK;
}


/*----------------------------------------------------------------------*/
/*		     g e t S e c t o r A n d O f f s e t		*/
/*									*/
/* Based on the current position of a file, gets a sector number and    */
/* offset in the sector that marks the file's current position.		*/
/* If the position is at the end-of-file, and the file is opened for    */
/* write, this routine will extend the file by allocating a new cluster.*/
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

static FLStatus getSectorAndOffset(File *file,
				 SectorNo *sectorNo,
				 unsigned *offsetInSector)
{
  Volume vol = file->fileVol;
  unsigned offsetInCluster =
	      (unsigned) file->currentPosition & (vol.bytesPerCluster - 1);
  if (offsetInCluster == 0) {	/* This cluster is finished. Get next */
    if (file->flags & FILE_IS_ROOT_DIR) {
      if (file->currentPosition >= file->fileSize)
	return flRootDirectoryFull;
    }
    else {
      if (file->currentPosition >= file->fileSize) {
	checkStatus(allocateCluster(file));
      }
      else {
	unsigned nextCluster;

	if (file->currentCluster == 0)
	  nextCluster = LE2(getDirEntry(file)->startingCluster);
	else
	  nextCluster = getFATentry(&vol,file->currentCluster);
	if (nextCluster < 2 || nextCluster > vol.maxCluster)
	  /* We have a bad file size, or the FAT is bad */
	  return flInvalidFATchain;
	file->currentCluster = nextCluster;
      }
    }
  }

  *offsetInSector = offsetInCluster & (SECTOR_SIZE - 1);
  if (file->flags & FILE_IS_ROOT_DIR)
    *sectorNo = vol.rootDirectorySectorNo +
		      (SectorNo) (file->currentPosition >> SECTOR_SIZE_BITS);
  else
    *sectorNo = firstSectorOfCluster(&vol,file->currentCluster) +
		      (SectorNo) (offsetInCluster >> SECTOR_SIZE_BITS);

  return flOK;
}


/*----------------------------------------------------------------------*/
/*		        c l o s e F i l e				*/
/*									*/
/* Closes an open file, records file size and dates in directory and    */
/* releases file handle.						*/
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	file		: File to close.		                */
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

static FLStatus closeFile(File *file)
{
  if ((file->flags & FILE_MODIFIED) && !(file->flags & FILE_IS_ROOT_DIR)) {
    DirectoryEntry *dirEntry;
    checkStatus(getDirEntryForUpdate(file,&dirEntry));

    dirEntry->attributes |= ATTR_ARCHIVE;
    if (!(file->flags & FILE_IS_DIRECTORY))
      toLE4(dirEntry->fileSize,file->fileSize);
    setCurrentDateTime(dirEntry);
  }

  file->flags = 0;		/* no longer open */

  return flOK;
}


#ifdef SUB_DIRECTORY

/*----------------------------------------------------------------------*/
/*		        e x t e n d D i r e c t o r y			*/
/*									*/
/* Extends a directory, writing empty entries and the mandatory '.' and */
/* '..' entries.							*/
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	file		: Directory file to extend. On entry,	        */
/*			  currentPosition == fileSize. On exit, fileSize*/
/*			  is updated.					*/
/*	ownerDir	: Cluster no. of owner directory		*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

static FLStatus extendDirectory(File *file, unsigned ownerDir)
{
  Volume vol = file->fileVol;
  unsigned i;
  SectorNo sectorOfDir;
  unsigned offsetInSector;

  /* Assuming the current position is at the end-of-file, this will     */
  /* extend the directory.						*/
  checkStatus(getSectorAndOffset(file,&sectorOfDir,&offsetInSector));

  for (i = 0; i < vol.sectorsPerCluster; i++) {
    /* Write binary zeroes to indicate never-used entries */
    checkStatus(updateSector(&vol,sectorOfDir + i,FALSE));
    buffer.checkPoint = TRUE;
    if (file->currentPosition == 0 && i == 0) {
      /* Add the mandatory . and .. entries */
      tffscpy(directory[0].name,".          ",sizeof directory[0].name);
      directory[0].attributes = ATTR_ARCHIVE | ATTR_DIRECTORY;
      toLE2(directory[0].startingCluster,file->currentCluster);
      toLE4(directory[0].fileSize,0);
      setCurrentDateTime(&directory[0]);
      tffscpy(&directory[1],&directory[0],sizeof directory[0]);
      directory[1].name[1] = '.';	/* change . to .. */
      toLE2(directory[1].startingCluster,ownerDir);
    }
    file->fileSize += SECTOR_SIZE;
  }
  /* Update the parent directory by closing the file */
  file->flags |= FILE_MODIFIED;
  return closeFile(file);
}

#endif	/* SUB_DIRECTORY */


/*----------------------------------------------------------------------*/
/*		        f i n d D i r E n t r y				*/
/*									*/
/* Finds a directory entry by path-name, or finds an available directory*/
/* entry if the file does not exist.					*/
/* Most fields necessary for opening a file are set by this routine.	*/
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	path		: path to find					*/
/*      file            : File in which to record directory information.*/
/*                        Specific fields on entry:			*/
/*			    flags: if FILE_MUST_OPEN = 1, directory 	*/
/*				   will be extended if necessary.	*/
/*			  on exit:					*/
/*			    flags: FILE_IS_DIRECTORY and            	*/
/*				   FILE_IS_ROOT_DIR set if true.	*/
/*			    fileSize: Set for non-directory files.  	*/
/*                          currentCluster: Set to 0 (unknown)		*/
/*			    ownerDirCluster: Set to 1st cluster of      */
/*				      owning directory.			*/
/*			    directorySector: Sector of directory. If 0  */
/*				      entry not found and directory full*/
/*			    directoryEntry: Entry # in directory sector */
/*			    currentPosition: not set by this routine.   */
/*									*/
/* Returns:                                                             */
/*	FLStatus	: 0 on success and file found			*/
/*			  flFileNotFound on success and file not found	*/
/*			  otherwise failed.				*/
/*----------------------------------------------------------------------*/

static FLStatus findDirEntry(Volume vol, FLSimplePath FAR1 *path, File *file)
{
  File scanFile;		/* Internal file of search */
  unsigned dirBackPointer = 0;	/* 1st cluster of previous directory */

  FLStatus status = flOK;		/* root directory exists */

  file->flags |= (FILE_IS_ROOT_DIR | FILE_IS_DIRECTORY);
  file->fileSize = (long) (vol.sectorsInRootDirectory) << SECTOR_SIZE_BITS;
  file->fileVol = &vol;

#ifdef SUB_DIRECTORY
  for (; path->name[0]; path++) /* while we have another path segment */
#else
  if (path->name[0])    /* search the root directory */
#endif
  {
    status = flFileNotFound;		/* not yet */
    if (!(file->flags & FILE_IS_DIRECTORY))
      return flPathNotFound;  /* if we don't have a directory,
			       we have no business here */

    scanFile = *file;	    /* the previous file found becomes the scan file */
    scanFile.currentPosition = 0;

    file->directorySector = 0;	/* indicate no entry found yet */
    file->flags &= ~(FILE_IS_ROOT_DIR | FILE_IS_DIRECTORY);
    file->ownerDirCluster = dirBackPointer;
    file->fileSize = 0;
    file->currentCluster = 0;

    /* Scan directory */
    while (scanFile.currentPosition < scanFile.fileSize) {
      int i;
      DirectoryEntry FAR0 *dirEntry;
      SectorNo sectorOfDir;
      unsigned offsetInSector;
      FLStatus readStatus = getSectorAndOffset(&scanFile,&sectorOfDir,&offsetInSector);
      if (readStatus == flInvalidFATchain) {
	scanFile.fileSize = scanFile.currentPosition;	/* now we know */
	break;		/* we ran into the end of the directory file */
      }
      else if (readStatus != flOK)
	return readStatus;

      dirEntry = (DirectoryEntry FAR0 *) findSector(&vol,sectorOfDir);
      if (dirEntry == NULL)
	return flSectorNotFound;

      scanFile.currentPosition += SECTOR_SIZE;

      for (i = 0; i < DIRECTORY_ENTRIES_PER_SECTOR; i++, dirEntry++) {
	if (tffscmp(path,dirEntry->name,sizeof dirEntry->name) == 0 &&
	    !(dirEntry->attributes & ATTR_VOL_LABEL)) {
	  /* Found a match */
	  file->directorySector = sectorOfDir;
	  file->directoryIndex = i;
	  file->fileSize = LE4(dirEntry->fileSize);
	  if (dirEntry->attributes & ATTR_DIRECTORY) {
	    file->flags |= FILE_IS_DIRECTORY;
	    file->fileSize = 0x7fffffffl;
	    /* Infinite. Directories don't have a recorded size */
	  }
	  if (dirEntry->attributes & ATTR_READ_ONLY)
	    file->flags |= FILE_READ_ONLY;
	  dirBackPointer = LE2(dirEntry->startingCluster);
	  status = flOK;
	  goto endOfPathSegment;
	}
	else if (dirEntry->name[0] == NEVER_USED_DIR_ENTRY ||
		 dirEntry->name[0] == DELETED_DIR_ENTRY) {
	  /* Found a free entry. Remember it in case we don't find a match */
	  if (file->directorySector == 0) {
	    file->directorySector = sectorOfDir;
	    file->directoryIndex = i;
	  }
	  if (dirEntry->name[0] == NEVER_USED_DIR_ENTRY)	/* end of dir */
	    goto endOfPathSegment;
	}
      }
    }

endOfPathSegment:
    ;
  }

  if (status == flFileNotFound && (file->flags & FILE_MUST_OPEN) &&
      file->directorySector == 0) {
    /* We did not find a place in the directory for this new entry. The */
    /* directory should be extended. 'scanFile' refers to the directory */
    /* to extend, and the current pointer is at its end			*/
#ifdef SUB_DIRECTORY
    checkStatus(extendDirectory(&scanFile,(unsigned) file->ownerDirCluster));
    file->directorySector = firstSectorOfCluster(&vol,scanFile.currentCluster);
    file->directoryIndex = 0;	      /* Has to be. This is a new cluster */
#else
    status = flRootDirectoryFull;
#endif
  }

  return status;
}


/*----------------------------------------------------------------------*/
/*		      	 r e a d F i l e				*/
/*									*/
/* Reads from the current position in the file to the user-buffer.	*/
/* Parameters:                                                          */
/*      file		: File to read.					*/
/*      ioreq->irData	: Address of user buffer			*/
/*	ioreq->irLength	: Number of bytes to read. If the read extends  */
/*			  beyond the end-of-file, the read is truncated */
/*			  at the end-of-file.				*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*	ioreq->irLength	: Actual number of bytes read			*/
/*----------------------------------------------------------------------*/

static FLStatus readFile(File *file,IOreq FAR2 *ioreq)
{
  Volume vol = file->fileVol;
  char FAR1 *userData = (char FAR1 *) ioreq->irData;   /* user buffer address */
  unsigned long stillToRead = ioreq->irLength;
  unsigned long remainingInFile = file->fileSize - file->currentPosition;
  ioreq->irLength = 0;		/* read so far */

  /* Should we return an end of file status ? */
  if (stillToRead > remainingInFile)
    stillToRead = (unsigned) remainingInFile;

  while (stillToRead > 0) {
    SectorNo sectorToRead;
    unsigned offsetInSector;
    unsigned readThisTime;
    const char FAR0 *sector;

    checkStatus(getSectorAndOffset(file,&sectorToRead,&offsetInSector));

    sector = (const char FAR0 *) findSector(&vol,sectorToRead);
    readThisTime = SECTOR_SIZE - offsetInSector;
    if (readThisTime > stillToRead)
      readThisTime = (unsigned) stillToRead;
    if (sector)
      tffscpy(userData,sector + offsetInSector,readThisTime);
    else
      return flSectorNotFound;		/* Sector does not exist */
    stillToRead -= readThisTime;
    ioreq->irLength += readThisTime;
    userData += readThisTime;
    file->currentPosition += readThisTime;
  }

  return flOK;
}


/*----------------------------------------------------------------------*/
/*		 f l F i n d N e x t F i l e				*/
/*                                                                      */
/* See the description of 'flFindFirstFile'.				*/
/*                                                                      */
/* Parameters:                                                          */
/*	irHandle	: File handle returned by flFindFirstFile.	*/
/*	irData		: Address of user buffer to receive a		*/
/*			  DirectoryEntry structure			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

static FLStatus findNextFile(File *file, IOreq FAR2 *ioreq)
{
  DirectoryEntry FAR1 *irDirEntry = (DirectoryEntry FAR1 *) ioreq->irData;

  /* Do we have a directory ? */
  if (!(file->flags & FILE_IS_DIRECTORY))
    return flNotADirectory;

  ioreq->irLength = DIRECTORY_ENTRY_SIZE;
  do {
    readFile(file,ioreq);
    if (ioreq->irLength != DIRECTORY_ENTRY_SIZE ||
	irDirEntry->name[0] == NEVER_USED_DIR_ENTRY) {
      checkStatus(closeFile(file));
      return flNoMoreFiles;
    }
  } while (irDirEntry->name[0] == DELETED_DIR_ENTRY ||
	   (irDirEntry->attributes & ATTR_VOL_LABEL));

  return flOK;
}


/*----------------------------------------------------------------------*/
/*		         d e l e t e F i l e				*/
/*									*/
/* Deletes a file or directory.                                         */
/*                                                                      */
/* Parameters:                                                          */
/*	ioreq->irPath	: path of file to delete			*/
/*	isDirectory	: 0 = delete file, other = delete directory	*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

static FLStatus deleteFile(Volume vol, IOreq FAR2 *ioreq, FLBoolean isDirectory)
{
  File file;		/* Our private file */
  DirectoryEntry *dirEntry;

  file.flags = 0;
  checkStatus(findDirEntry(&vol,ioreq->irPath,&file));

  if (file.flags & FILE_READ_ONLY)
    return flNoWriteAccess;

  if (isDirectory) {
    DirectoryEntry fileFindInfo;
    ioreq->irData = &fileFindInfo;

    if (!(file.flags & FILE_IS_DIRECTORY))
      return flNotADirectory;
    /* Verify that directory is empty */
    file.currentPosition = 0;
    for (;;) {
      FLStatus status = findNextFile(&file,ioreq);
      if (status == flNoMoreFiles)
	break;
      if (status != flOK)
	return status;
      if (!((fileFindInfo.attributes & ATTR_DIRECTORY) &&
	    (tffscmp(fileFindInfo.name,".          ",sizeof fileFindInfo.name) == 0 ||
	     tffscmp(fileFindInfo.name,"..         ",sizeof fileFindInfo.name) == 0)))
	return flDirectoryNotEmpty;
    }
  }
  else {
    /* Did we find a directory ? */
    if (file.flags & FILE_IS_DIRECTORY)
      return flFileIsADirectory;
  }

  /* Mark directory entry deleted */
  checkStatus(getDirEntryForUpdate(&file,&dirEntry));
  dirEntry->name[0] = DELETED_DIR_ENTRY;

  /* Delete FAT entries */
  file.currentPosition = 0;
  file.currentCluster = LE2(dirEntry->startingCluster);
  while (file.currentPosition < file.fileSize) {
    unsigned nextCluster;

    if (file.currentCluster < 2 || file.currentCluster > vol.maxCluster)
      /* We have a bad file size, or the FAT is bad */
      return isDirectory ? flOK : flInvalidFATchain;

    nextCluster = getFATentry(&vol,file.currentCluster);

    /* mark FAT free */
    checkStatus(setFATentry(&vol,file.currentCluster,FAT_FREE));
    buffer.checkPoint = TRUE;

    /* mark sectors free */
    checkStatus(vol.tl.deleteSector(vol.tl.rec,
				    firstSectorOfCluster(&vol,file.currentCluster),
				    vol.sectorsPerCluster));

    file.currentPosition += vol.bytesPerCluster;
    file.currentCluster = nextCluster;
  }

  return flOK;
}


/*----------------------------------------------------------------------*/
/*		     s e t N a m e I n D i r E n t r y			*/
/*									*/
/* Sets the file name in a directory entry from a path name             */
/*                                                                      */
/* Parameters:                                                          */
/*	dirEntry	: directory entry				*/
/*	path		: path the last segment of which is the name	*/
/*                                                                      */
/*----------------------------------------------------------------------*/

static void setNameInDirEntry(DirectoryEntry *dirEntry, FLSimplePath FAR1 *path)
{
  FLSimplePath FAR1 *lastSegment;

  for (lastSegment = path;		/* Find null terminator */
       lastSegment->name[0];
       lastSegment++);

  tffscpy(dirEntry->name,--lastSegment,sizeof dirEntry->name);
}


/*----------------------------------------------------------------------*/
/*		         o p e n F i l e				*/
/*									*/
/* Opens an existing file or creates a new file. Creates a file handle  */
/* for further file processing.						*/
/*                                                                      */
/* Parameters:                                                          */
/*	ioreq->irFlags	: Access and action options, defined below	*/
/*	ioreq->irPath	: path of file to open             		*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*	ioreq->irHandle	: New file handle for open file                 */
/*                                                                      */
/*----------------------------------------------------------------------*/


static FLStatus openFile(Volume vol, IOreq FAR2 *ioreq)
{
  int i;
  FLStatus status;

  /* Look for an available file */
  File *file = fileTable;
  for (i = 0; i < FILES && (file->flags & FILE_IS_OPEN); i++, file++);
  if (i >= FILES)
    return flTooManyOpenFiles;
  file->fileVol = &vol;
  ioreq->irHandle = i;		/* return file handle */

  /* Delete file if exists and required */
  if (ioreq->irFlags & ACCESS_CREATE) {
    FLStatus status = deleteFile(&vol,ioreq,FALSE);
    if (status != flOK && status != flFileNotFound)
      return status;
  }

  /* Find the path */
  if (ioreq->irFlags & ACCESS_CREATE)
    file->flags |= FILE_MUST_OPEN;
  status =  findDirEntry(file->fileVol,ioreq->irPath,file);
  if (status != flOK &&
      (status != flFileNotFound || !(ioreq->irFlags & ACCESS_CREATE)))
    return status;

  /* Did we find a directory ? */
  if (file->flags & FILE_IS_DIRECTORY)
    return flFileIsADirectory;

  /* Create file if required */
  if (ioreq->irFlags & ACCESS_CREATE) {
    DirectoryEntry *dirEntry;
    checkStatus(getDirEntryForUpdate(file,&dirEntry));

    setNameInDirEntry(dirEntry,ioreq->irPath);
    dirEntry->attributes = ATTR_ARCHIVE;
    toLE2(dirEntry->startingCluster,0);
    toLE4(dirEntry->fileSize,0);
    setCurrentDateTime(dirEntry);
  }

  if (!(ioreq->irFlags & ACCESS_READ_WRITE))
    file->flags |= FILE_READ_ONLY;

  file->currentPosition = 0;		/* located at file beginning */
  file->flags |= FILE_IS_OPEN;		/* this file now officially open */

  return flOK;
}


/*----------------------------------------------------------------------*/
/*		         w r i t e F i l e				*/
/*									*/
/* Writes from the current position in the file from the user-buffer.   */
/*                                                                      */
/* Parameters:                                                          */
/*	file		: File to write.				*/
/*      ioreq->irData	: Address of user buffer			*/
/*	ioreq->irLength	: Number of bytes to write.			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*	ioreq->irLength	: Actual number of bytes written		*/
/*----------------------------------------------------------------------*/

static FLStatus writeFile(File *file, IOreq FAR2 *ioreq)
{
  Volume vol = file->fileVol;
  char FAR1 *userData = (char FAR1 *) ioreq->irData;   /* user buffer address */
  unsigned long stillToWrite = ioreq->irLength;

  if (file->flags & FILE_READ_ONLY)
    return flNoWriteAccess;

  file->flags |= FILE_MODIFIED;

  ioreq->irLength = 0;		/* written so far */

  while (stillToWrite > 0) {
    SectorNo sectorToWrite;	/* She's got a sector to write, but she don't care */
    unsigned offsetInSector;
    unsigned writeThisTime;

    checkStatus(getSectorAndOffset(file,&sectorToWrite,&offsetInSector));

    if (stillToWrite < SECTOR_SIZE || offsetInSector > 0) {
      /* Not on full sector boundary */
      checkStatus(updateSector(&vol,sectorToWrite,file->currentPosition < file->fileSize));
      if (file->flags & FILE_IS_DIRECTORY)
	buffer.checkPoint = TRUE;
      writeThisTime = SECTOR_SIZE - offsetInSector;
      if (writeThisTime > stillToWrite)
	writeThisTime = (unsigned) stillToWrite;
      tffscpy(buffer.data + offsetInSector,userData,writeThisTime);
    }
    else {
      if (sectorToWrite == buffer.sectorNo && &vol == buffer.owner) {
	buffer.sectorNo = UNASSIGNED_SECTOR;		/* no longer valid */
	buffer.dirty = buffer.checkPoint = FALSE;
      }
#ifdef SINGLE_BUFFER
      if (buffer.dirty) {
	long int freeWritableSectors = 0;
	vol.tl.defragment(vol.tl.rec,&freeWritableSectors);
	/* No defragmentation, just tell us how much space left */
	if (freeWritableSectors < 20)
	  /* Prevent garbage collection while the buffer is dirty */
	  checkStatus(flushBuffer(&vol));
      }
#endif
      checkStatus(vol.tl.writeSector(vol.tl.rec,sectorToWrite,userData));
      writeThisTime = SECTOR_SIZE;
    }
    stillToWrite -= writeThisTime;
    ioreq->irLength += writeThisTime;
    userData += writeThisTime;
    file->currentPosition += writeThisTime;
    if (file->currentPosition > file->fileSize)
      file->fileSize = file->currentPosition;
  }

  return flOK;
}


/*----------------------------------------------------------------------*/
/*		          s e e k F i l e				*/
/*									*/
/* Sets the current position in the file, relative to file start, end or*/
/* current position.							*/
/* Note: This function will not move the file pointer beyond the	*/
/* beginning or end of file, so the actual file position may be		*/
/* different from the required. The actual position is indicated on     */
/* return.								*/
/*                                                                      */
/* Parameters:                                                          */
/*	file		: File to set position.                         */
/*      ioreq->irLength	: Offset to set position.			*/
/*	ioreq->irFlags	: Method code					*/
/*			  SEEK_START: absolute offset from start of file  */
/*			  SEEK_CURR:  signed offset from current position */
/*			  SEEK_END:   signed offset from end of file    */
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*	ioreq->irLength	: Actual absolute offset from start of file	*/
/*----------------------------------------------------------------------*/

static FLStatus seekFile(File *file, IOreq FAR2 *ioreq)
{
  Volume vol = file->fileVol;
  long int seekPosition = ioreq->irLength;

  switch (ioreq->irFlags) {

    case SEEK_START:
      break;

    case SEEK_CURR:
      seekPosition += file->currentPosition;
      break;

    case SEEK_END:
      seekPosition += file->fileSize;
      break;

    default:
      return flBadParameter;
  }

  if (seekPosition < 0)
    seekPosition = 0;
  if (seekPosition > file->fileSize)
    seekPosition = file->fileSize;

  /* now set the position ... */
  if (seekPosition < file->currentPosition) {
    file->currentCluster = 0;
    file->currentPosition = 0;
  }
  while (file->currentPosition < seekPosition) {
    SectorNo sectorNo;
    unsigned offsetInSector;
    checkStatus(getSectorAndOffset(file,&sectorNo,&offsetInSector));

    file->currentPosition += vol.bytesPerCluster;
    file->currentPosition &= - (long) (vol.bytesPerCluster);
  }
  ioreq->irLength = file->currentPosition = seekPosition;

  return flOK;
}


/*----------------------------------------------------------------------*/
/*		          f l F i n d F i l e				*/
/*                                                                      */
/* Finds a file entry in a directory, optionally modifying the file     */
/* time/date and/or attributes.                                         */
/* Files may be found by handle no. provided they are open, or by name. */
/* Only the Hidden, System or Read-only attributes may be modified.	*/
/* Entries may be found for any existing file or directory other than   */
/* the root. A DirectoryEntry structure describing the file is copied   */
/* to a user buffer.							*/
/*                                                                      */
/* The DirectoryEntry structure is defined in dosformt.h		*/
/*                                                                      */
/* Parameters:                                                          */
/*	irHandle	: If by name: Drive number (0, 1, ...)		*/
/*			  else      : Handle of open file		*/
/*	irPath		: If by name: Specifies a file or directory path*/
/*	irFlags		: Options flags					*/
/*			  FIND_BY_HANDLE: Find open file by handle. 	*/
/*					  Default is access by path.    */
/*                        SET_DATETIME:	Update time/date from buffer	*/
/*			  SET_ATTRIBUTES: Update attributes from buffer	*/
/*	irDirEntry	: Address of user buffer to receive a		*/
/*			  DirectoryEntry structure			*/
/*                                                                      */
/* Returns:                                                             */
/*	irLength	: Modified					*/
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

static FLStatus findFile(Volume vol, File *file, IOreq FAR2 *ioreq)
{
  File tFile;			/* temporary file for searches */

  if (ioreq->irFlags & FIND_BY_HANDLE)
    tFile = *file;
  else {
    tFile.flags = 0;
    checkStatus(findDirEntry(&vol,ioreq->irPath,&tFile));
  }

  if (tFile.flags & FILE_IS_ROOT_DIR)
    if (ioreq->irFlags & (SET_DATETIME | SET_ATTRIBUTES))
      return flPathIsRootDirectory;
    else {
      DirectoryEntry FAR1 *irDirEntry = (DirectoryEntry FAR1 *) ioreq->irData;

      tffsset(irDirEntry,0,sizeof(DirectoryEntry));
      irDirEntry->attributes = ATTR_DIRECTORY;
      return flOK;
    }

  if (ioreq->irFlags & (SET_DATETIME | SET_ATTRIBUTES)) {
    DirectoryEntry FAR1 *irDirEntry = (DirectoryEntry FAR1 *) ioreq->irData;
    DirectoryEntry *dirEntry;

    checkStatus(getDirEntryForUpdate(&tFile,&dirEntry));
    if (ioreq->irFlags & SET_DATETIME) {
      COPY2(dirEntry->updateDate,irDirEntry->updateDate);
      COPY2(dirEntry->updateTime,irDirEntry->updateTime);
    }
    if (ioreq->irFlags & SET_ATTRIBUTES)
      dirEntry->attributes = irDirEntry->attributes &
	     (ATTR_ARCHIVE | ATTR_HIDDEN | ATTR_READ_ONLY | ATTR_SYSTEM);
    tffscpy(irDirEntry, dirEntry, sizeof(DirectoryEntry));
  }
  else {
    tffscpy(ioreq->irData,getDirEntry(&tFile),sizeof(DirectoryEntry));
    if (ioreq->irFlags & FIND_BY_HANDLE)
      toLE4(((DirectoryEntry FAR1 *) (ioreq->irData))->fileSize, tFile.fileSize);
  }

  return flOK;
}


/*----------------------------------------------------------------------*/
/*		 f l F i n d F i r s t F i l e				*/
/*                                                                      */
/* Finds the first file entry in a directory.				*/
/* This function is used in combination with the flFindNextFile call,   */
/* which returns the remaining file entries in a directory sequentially.*/
/* Entries are returned according to the unsorted directory order.	*/
/* flFindFirstFile creates a file handle, which is returned by it. Calls*/
/* to flFindNextFile will provide this file handle. When flFindNextFile */
/* returns 'noMoreEntries', the file handle is automatically closed.    */
/* Alternatively the file handle can be closed by a 'closeFile' call    */
/* before actually reaching the end of directory.			*/
/* A DirectoryEntry structure is copied to the user buffer describing   */
/* each file found. This structure is defined in dosformt.h.		*/
/*                                                                      */
/* Parameters:                                                          */
/*	irHandle	: Drive number (0, 1, ...)			*/
/*	irPath		: Specifies a directory path			*/
/*	irData		: Address of user buffer to receive a		*/
/*			  DirectoryEntry structure			*/
/*                                                                      */
/* Returns:                                                             */
/*	irHandle	: File handle to use for subsequent operations. */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

static FLStatus findFirstFile(Volume vol, IOreq FAR2 *ioreq)
{
  int i;

  /* Look for an available file */
  File *file = fileTable;
  for (i = 0; i < FILES && (file->flags & FILE_IS_OPEN); i++, file++);
  if (i >= FILES)
    return flTooManyOpenFiles;
  file->fileVol = &vol;
  ioreq->irHandle = i;		/* return file handle */

  /* Find the path */
  checkStatus(findDirEntry(file->fileVol,ioreq->irPath,file));

  file->currentPosition = 0;		/* located at file beginning */
  file->flags |= FILE_IS_OPEN | FILE_READ_ONLY; /* this file now officially open */

  return findNextFile(file,ioreq);
}


/*----------------------------------------------------------------------*/
/*		         g e t D i s k I n f o				*/
/*									*/
/* Returns general allocation information.				*/
/*									*/
/* The bytes/sector, sector/cluster, total cluster and free cluster	*/
/* information are returned into a DiskInfo structure.			*/
/*                                                                      */
/* Parameters:                                                          */
/*	ioreq->irData	: Address of DiskInfo structure                 */
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

static FLStatus getDiskInfo(Volume vol, IOreq FAR2 *ioreq)
{
  unsigned i;

  DiskInfo FAR1 *diskInfo = (DiskInfo FAR1 *) ioreq->irData;

  diskInfo->bytesPerSector = SECTOR_SIZE;
  diskInfo->sectorsPerCluster = vol.sectorsPerCluster;
  diskInfo->totalClusters = vol.maxCluster - 1;
  diskInfo->freeClusters = 0;		/* let's count them */

  for (i = 2; i <= vol.maxCluster; i++)
    if (getFATentry(&vol,i) == 0)
      diskInfo->freeClusters++;

  return flOK;
}


#ifdef RENAME_FILE

/*----------------------------------------------------------------------*/
/*		        r e n a m e F i l e				*/
/*									*/
/* Renames a file to another name.					*/
/*									*/
/* Parameters:                                                          */
/*	ioreq->irPath	: path of existing file				*/
/*      ioreq->irData	: path of new name.				*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

static FLStatus renameFile(Volume vol, IOreq FAR2 *ioreq)
{
  File file, file2;		/* temporary files for searches */
  DirectoryEntry *dirEntry, *dirEntry2;
  FLStatus status;
  FLSimplePath FAR1 *irPath2 = (FLSimplePath FAR1 *) ioreq->irData;

  file.flags = 0;
  checkStatus(findDirEntry(&vol,ioreq->irPath,&file));

  file2.flags = FILE_MUST_OPEN;
  status = findDirEntry(file.fileVol,irPath2,&file2);
  if (status != flFileNotFound)
    return status == flOK ? flFileAlreadyExists : status;

#ifndef VFAT_COMPATIBILITY
  if (file.ownerDirCluster == file2.ownerDirCluster) {	/* Same directory */
    /* Change name in directory entry */
    checkStatus(getDirEntryForUpdate(&file,&dirEntry));
    setNameInDirEntry(dirEntry,irPath2);
  }
  else
#endif
  {	/* not same directory */
    /* Write new directory entry */
    checkStatus(getDirEntryForUpdate(&file2,&dirEntry2));
    *dirEntry2 = *getDirEntry(&file);
    setNameInDirEntry(dirEntry2,irPath2);

    /* Delete original entry */
    checkStatus(getDirEntryForUpdate(&file,&dirEntry));
    dirEntry->name[0] = DELETED_DIR_ENTRY;
  }

  return flOK;
}

#endif /* RENAME_FILE */


#ifdef SUB_DIRECTORY

/*----------------------------------------------------------------------*/
/*		             m a k e D i r				*/
/*									*/
/* Creates a new directory.						*/
/*                                                                      */
/* Parameters:                                                          */
/*	ioreq->irPath	: path of new directory.			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

static FLStatus makeDir(Volume vol, IOreq FAR2 *ioreq)
{
  File file;			/* temporary file for searches */
  unsigned dirBackPointer;
  DirectoryEntry *dirEntry;
  FLStatus status;

  file.flags = FILE_MUST_OPEN;
  status = findDirEntry(&vol,ioreq->irPath,&file);
  if (status != flFileNotFound)
    return status == flOK ? flFileAlreadyExists : status;

  /* Create the directory entry for the new dir */
  checkStatus(getDirEntryForUpdate(&file,&dirEntry));

  setNameInDirEntry(dirEntry,ioreq->irPath);
  dirEntry->attributes = ATTR_ARCHIVE | ATTR_DIRECTORY;
  toLE2(dirEntry->startingCluster,0);
  toLE4(dirEntry->fileSize,0);
  setCurrentDateTime(dirEntry);

  /* Remember the back pointer to owning directory for the ".." entry */
  dirBackPointer = (unsigned) file.ownerDirCluster;

  file.flags |= FILE_IS_DIRECTORY;
  file.currentPosition = 0;
  file.fileSize = 0;
  return extendDirectory(&file,dirBackPointer);
}


#endif /* SUB_DIRECTORY */

#ifdef SPLIT_JOIN_FILE

/*------------------------------------------------------------------------*/
/*		          j o i n F i l e                                 */
/*                                                                        */
/* joins two files. If the end of the first file is on a cluster          */
/* boundary, the files will be joined there. Otherwise, the data in       */
/* the second file from the beginning until the offset that is equal to   */
/* the offset in cluster of the end of the first file will be lost. The   */
/* rest of the second file will be joined to the first file at the end of */
/* the first file. On exit, the first file is the expanded file and the   */
/* second file is deleted.                                                */
/* Note: The second file will be open by this function, it is advised to  */
/*	 close it before calling this function in order to avoid          */
/*	 inconsistencies.                                                 */
/*                                                                        */
/* Parameters:                                                            */
/*	file            : file to join to.                                */
/*	irPath          : Path name of the file to be joined.             */
/*                                                                        */
/* Returns:                                                               */
/*	FLStatus        : 0 on success, otherwise failed.                 */
/*                                                                        */
/*------------------------------------------------------------------------*/

static FLStatus joinFile (File *file, IOreq FAR2 *ioreq)
{
  Volume vol = file->fileVol;
  File joinedFile;
  DirectoryEntry *joinedDirEntry;
  unsigned offsetInCluster = file->fileSize % vol.bytesPerCluster;

  if (file->flags & FILE_READ_ONLY)
    return flNoWriteAccess;

  if (file->flags & FILE_IS_DIRECTORY)
    return flFileIsADirectory;

  /* open the joined file. */
  joinedFile.flags = 0;
  checkStatus(findDirEntry(file->fileVol,ioreq->irPath,&joinedFile));
  joinedFile.currentPosition = 0;

  /* Check if the two files are the same file. */
  if (file->directorySector == joinedFile.directorySector &&
      file->directoryIndex == joinedFile.directoryIndex)
    return flBadFileHandle;

  file->flags |= FILE_MODIFIED;

  if (joinedFile.fileSize > offsetInCluster) { /* the joined file extends
						   beyond file's end of file.*/
    unsigned lastCluster, nextCluster, firstCluster;

    /* get the first cluster of the joined file. */
    firstCluster = LE2(getDirEntry(&joinedFile)->startingCluster);

    if (file->fileSize) {  /* the file is not empty.*/
      /* find the last cluster of file by following the FAT chain.*/
      if (file->currentCluster == 0)     /* start from the first cluster.*/
	nextCluster = LE2(getDirEntry(file)->startingCluster);
      else                               /* start from the current cluster.*/
	nextCluster = file->currentCluster;
      /* follow the FAT chain.*/
      while (nextCluster != FAT_LAST_CLUSTER) {
	if (nextCluster < 2 || nextCluster > vol.maxCluster)
	  return flInvalidFATchain;
	lastCluster = nextCluster;
	nextCluster = getFATentry(&vol,lastCluster);
      }
    }
    else                   /* the file is empty. */
      lastCluster = 0;

    if (offsetInCluster) {      /* join in the middle of a cluster.*/
      SectorNo sectorNo, joinedSectorNo, tempSectorNo;
      unsigned offset, joinedOffset, numOfSectors = 1, i;
      const char FAR0 *startCopy;

      /* get the sector and offset of the end of the file.*/
      file->currentPosition = file->fileSize;
      file->currentCluster = lastCluster;
      checkStatus(getSectorAndOffset(file, &sectorNo, &offset));

      /* load the sector of the end of the file to the buffer.*/
      checkStatus(updateSector(&vol, sectorNo, TRUE));

      /*  copy the second part of the first cluster of the joined file
	  to the end of the last cluster of the original file.*/
      /* first set the current position of the joined file.*/
      joinedFile.currentPosition = offsetInCluster;
      joinedFile.currentCluster = firstCluster;
      /* get the relevant sector in the joined file.*/
      checkStatus(getSectorAndOffset(&joinedFile, &joinedSectorNo, &joinedOffset));
      /* map sector and offset.*/
      startCopy = (const char FAR0 *) findSector(&vol,joinedSectorNo) + joinedOffset;
      if (startCopy == NULL)
	return flSectorNotFound;

      /* copy.*/
      tffscpy(buffer.data + offset, startCopy, SECTOR_SIZE - offset);
      checkStatus(flushBuffer(&vol));

      /* find how many sectors should still be copied (the number of sectors
	 until the end of the current cluster).*/
      tempSectorNo = firstSectorOfCluster(&vol,lastCluster);
      while(tempSectorNo != sectorNo) {
	tempSectorNo++;
	numOfSectors++;
      }

      /* copy the rest of the sectors in the current cluster.
	 this is done by loading a sector from the joined file to the buffer,
	 changing the sectoNo of the buffer to the relevant sector in file
	 and then flushing the buffer.*/
      sectorNo++;
      joinedSectorNo++;
      for(i = 0; i < vol.sectorsPerCluster - numOfSectors; i++) {
	checkStatus(updateSector(&vol,joinedSectorNo, TRUE));
	buffer.sectorNo = sectorNo;
	checkStatus(flushBuffer(&vol));
	sectorNo++;
	joinedSectorNo++;
      }

      /* adjust the FAT chain.*/
      checkStatus(setFATentry(&vol,
			      lastCluster,
			      getFATentry(&vol,firstCluster)));

      /* mark the first cluster of the joined file as free */
      checkStatus(setFATentry(&vol,firstCluster,FAT_FREE));
      buffer.checkPoint = TRUE;

      /* mark sectors free */
      checkStatus(vol.tl.deleteSector(vol.tl.rec,firstSectorOfCluster(&vol,firstCluster),
				      vol.sectorsPerCluster));
    }
    else {    /* join on a cluster boundary.*/
      if (lastCluster) {      /* file is not empty. */
	checkStatus(setFATentry(&vol,lastCluster, firstCluster));
      }
      else {                  /* file is empty.*/
	DirectoryEntry *dirEntry;

	checkStatus(getDirEntryForUpdate(file, &dirEntry));
	toLE2(dirEntry->startingCluster, firstCluster);
        setCurrentDateTime(dirEntry);
      }
    }
    /*adjust the size of the expanded file.*/
    file->fileSize += joinedFile.fileSize - offsetInCluster;
  }

  /* mark the directory entry of the joined file as deleted.*/
  checkStatus(getDirEntryForUpdate(&joinedFile, &joinedDirEntry));
  joinedDirEntry->name[0] = DELETED_DIR_ENTRY;

  return flOK;
}


/*------------------------------------------------------------------------*/
/*		      s p l i t F i l e                                   */
/*                                                                        */
/* Splits the file into two files. The original file contains the first   */
/* part, and a new file (which is created for that purpose) contains      */
/* the second part. If the current position is on a cluster               */
/* boundary, the file will be split at the current position. Otherwise,   */
/* the cluster of the current position is duplicated, one copy is the     */
/* first cluster of the new file, and the other is the last cluster of the*/
/* original file, which now ends at the current position.                 */
/*                                                                        */
/* Parameters:                                                            */
/*	file            : file to split.                                  */
/*	irPath          : Path name of the new file.                      */
/*                                                                        */
/* Returns:                                                               */
/*	irHandle        : handle of the new file.                         */
/*	FLStatus        : 0 on success, otherwise failed.                 */
/*                                                                        */
/*------------------------------------------------------------------------*/

static FLStatus splitFile (File *file, IOreq FAR2 *ioreq)
{
  Volume vol = file->fileVol;
  File *newFile, dummyFile;
  IOreq ioreq2;
  FLStatus status;

  if (file->flags & FILE_READ_ONLY)
    return flNoWriteAccess;

  if (file->flags & FILE_IS_DIRECTORY)
    return flFileIsADirectory;

  /* check if the path of the new file already exists.*/
  dummyFile.flags = 0;
  status = findDirEntry(&vol,ioreq->irPath,&dummyFile);
  if (status != flFileNotFound) {
    if (status == flOK)              /* there is a file with that path.*/
      return flFileAlreadyExists;
    else
      return status;
  }

  /* open the new file.*/

  ioreq2.irFlags = OPEN_FOR_WRITE;
  ioreq2.irPath = ioreq->irPath;
  checkStatus(openFile(&vol,&ioreq2));

  newFile = fileTable + ioreq2.irHandle;
  newFile->flags |= FILE_MODIFIED;
  file->flags |= FILE_MODIFIED;

  if (file->currentPosition % vol.bytesPerCluster) { /* not on a cluster boundary.*/
    SectorNo sectorNo, newSectorNo;
    int i;

    checkStatus(allocateCluster(newFile));

    sectorNo = firstSectorOfCluster(&vol,file->currentCluster);
    newSectorNo = firstSectorOfCluster(&vol,newFile->currentCluster);

    /* copy the current cluster of the original file to the first cluster
       of the new file, sector after sector.*/
    for(i = 0; i < vol.sectorsPerCluster; i++) {
      checkStatus(updateSector(&vol,sectorNo, TRUE));
      buffer.sectorNo = newSectorNo;
      checkStatus(flushBuffer(&vol));
      sectorNo++;
      newSectorNo++;
    }

    /* adjust the FAT chain of the new file.*/
    checkStatus(setFATentry(&vol,newFile->currentCluster,
		getFATentry(&vol,file->currentCluster)));

    /* mark current cluster 0 (as current position).*/
    newFile->currentCluster = 0;

  }
  else {                                  /* on a cluster boundary.*/
    DirectoryEntry *newDirEntry;

    /* adjust the directory entry of the new file.*/
    checkStatus(getDirEntryForUpdate(newFile,&newDirEntry));
    if (file->currentPosition) { /* split at the middle of the file.*/
      toLE2(newDirEntry->startingCluster, getFATentry(&vol,file->currentCluster));
      setCurrentDateTime(newDirEntry);
    }
    else {                     /* split at the beginning of the file.*/
      DirectoryEntry *dirEntry;

      /* first cluster of file becomes the first cluster of the new file.*/
      newDirEntry->startingCluster = getDirEntry(file)->startingCluster;
      setCurrentDateTime(newDirEntry);

      /* starting cluster of file becomes 0.*/
      checkStatus(getDirEntryForUpdate(file, &dirEntry));
      toLE2(dirEntry->startingCluster, 0);
      setCurrentDateTime(dirEntry);
    }
  }

  /* adjust the size of the new file.*/
  newFile->fileSize = file->fileSize - file->currentPosition +
		     (file->currentPosition % vol.bytesPerCluster);

  /* adjust the chain and size of the original file.*/
  if (file->currentPosition)    /* we didn't split at the beginning.*/
    checkStatus(setFATentry(&vol,file->currentCluster, FAT_LAST_CLUSTER));

  file->fileSize = file->currentPosition;

  /* return to the user the handle of the new file.*/
  ioreq->irHandle = ioreq2.irHandle;

  return flOK;
}

#endif /* SPLIT_JOIN_FILE */
#endif /* FILES > 0 */


/*----------------------------------------------------------------------*/
/*		           f l C a l l   				*/
/*									*/
/* Common entry-point to all file-system functions. Macros are          */
/* to call individual function, which are separately described below.	*/
/*                                                                      */
/* Parameters:                                                          */
/*	function	: file-system function code (listed below)	*/
/*	ioreq		: IOreq structure				*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

FLStatus flCall(FLFunctionNo functionNo, IOreq FAR2 *ioreq)
{
  Volume vol = NULL;
  FLStatus status;
#if FILES > 0
  File *file;
#endif

  if (!initDone)
    checkStatus(flInit());

#if FILES > 0
  /* Verify file handle if applicable */
  switch (functionNo) {
    case FL_FIND_FILE:
      if (!(ioreq->irFlags & FIND_BY_HANDLE))
	break;

#ifdef SPLIT_JOIN_FILE
    case FL_JOIN_FILE:
    case FL_SPLIT_FILE:
#endif

    case FL_CLOSE_FILE:
    case FL_READ_FILE:
    case FL_WRITE_FILE:
    case FL_SEEK_FILE:
    case FL_FIND_NEXT_FILE:
      if (ioreq->irHandle < FILES &&
	  (fileTable[ioreq->irHandle].flags & FILE_IS_OPEN)) {
	file = fileTable + ioreq->irHandle;
	pVol = file->fileVol;
      }
      else
	return flBadFileHandle;
  }
#endif

  if (pVol == NULL) {	/* irHandle is drive no. */
#if DRIVES > 1
    if (ioreq->irHandle >= noOfDrives)
      return flBadDriveHandle;
    pVol = &vols[ioreq->irHandle];
#else
    pVol = &vols[0];
#endif
  }

  checkStatus(setBusy(&vol,TFFS_ON));       /* Let everyone know we are here */

  /* Nag about mounting if necessary */
  switch (functionNo) {

    case FL_MOUNT_VOLUME:
    case FL_FORMAT_VOLUME:
#ifdef LOW_LEVEL
      if (vol.flags & VOLUME_LOW_LVL_MOUNTED)
	dismountLowLevel(&vol);  /* mutual exclusive mounting */
#endif
      break;

#ifdef LOW_LEVEL
    case FL_PHYSICAL_WRITE:
    case FL_PHYSICAL_ERASE:
      if (vol.flags & VOLUME_ABS_MOUNTED) {
	status = flGeneralFailure;  /* mutual exclusive mounting */
	goto flCallExit;
      }

      if (!(vol.flags & VOLUME_LOW_LVL_MOUNTED)) {
	status = mountLowLevel(&vol);  /* automatic low level mounting */
      }
      else {
	status = flOK;
#ifndef FIXED_MEDIA
	status = flMediaCheck(vol.socket);
	if (status == flDiskChange)
	  status = mountLowLevel(&vol); /* card was changed, remount */
#endif
      }

      if (status != flOK) {
	dismountLowLevel(&vol);
	goto flCallExit;
      }

      break;

    case FL_GET_PHYSICAL_INFO:
    case FL_PHYSICAL_READ:
      if (!(vol.flags & (VOLUME_LOW_LVL_MOUNTED | VOLUME_ABS_MOUNTED))) {
	status = mountLowLevel(&vol);  /* automatic low level mounting */
      }
      else {
	status = flOK;
#ifndef FIXED_MEDIA
	status = flMediaCheck(vol.socket);
	if (status == flDiskChange) {
          if (vol.flags & VOLUME_ABS_MOUNTED)
	    dismountVolume(&vol);

	  status = mountLowLevel(&vol); /* card was changed, remount */
	}
#endif
      }

      if (status != flOK) {
        if (vol.flags & VOLUME_ABS_MOUNTED)
	  dismountVolume(&vol);
        if (vol.flags & VOLUME_LOW_LVL_MOUNTED)
	  dismountLowLevel(&vol);
	goto flCallExit;
      }

      break;

#endif /* LOW_LEVEL */

    default:
      if (vol.flags & VOLUME_ABS_MOUNTED) {
	FLStatus status = flOK;
#ifndef FIXED_MEDIA
	status = flMediaCheck(vol.socket);
#endif
	if (status != flOK)
	  dismountVolume(&vol);
      }

      if (!(vol.flags & VOLUME_ABS_MOUNTED)) {
	status = flNotMounted;
	goto flCallExit;
      }

      switch (functionNo) {
	case FL_DISMOUNT_VOLUME:
	case FL_CHECK_VOLUME:
	case FL_DEFRAGMENT_VOLUME:
	case FL_ABS_READ:
	case FL_ABS_WRITE:
	case FL_ABS_DELETE:
	case FL_GET_BPB:
	  break;

	default:
	  if (!(vol.flags & VOLUME_MOUNTED)) {
	    status = flNotMounted;
	    goto flCallExit;
	  }
      }
  }


  /* Execute specific function */
  switch (functionNo) {

    case FL_MOUNT_VOLUME:
      status = mountVolume(&vol);
      break;

    case FL_DISMOUNT_VOLUME:
      status = dismountVolume(&vol);
      break;

    case FL_CHECK_VOLUME:
      status = flOK;		/* If we got this far */
      break;

#if defined(DEFRAGMENT_VOLUME) || defined(SINGLE_BUFFER)
    case FL_DEFRAGMENT_VOLUME:
      status = defragmentVolume(&vol,ioreq);
      break;
#endif

#ifdef FORMAT_VOLUME
    case FL_FORMAT_VOLUME:
      status = formatVolume(&vol,ioreq);
      break;
#endif

    case FL_DONT_MONITOR_FAT:
      status = disableFATmonitor(&vol);
      break;

#ifdef ABS_READ_WRITE
    case FL_ABS_READ:
      status = absRead(&vol,ioreq);
      break;

    case FL_ABS_WRITE:
      status = absWrite(&vol,ioreq);
      break;

    case FL_ABS_DELETE:
      status = absDelete(&vol,ioreq);
      break;

    case FL_GET_BPB:
      status = getBPB(&vol,ioreq);
      break;
#endif

#ifdef LOW_LEVEL
    case FL_GET_PHYSICAL_INFO:
      status = getPhysicalInfo(&vol, ioreq);
      break;

    case FL_PHYSICAL_READ:
      status = physicalRead(&vol, ioreq);
      break;

    case FL_PHYSICAL_WRITE:
      status = physicalWrite(&vol, ioreq);
      break;

    case FL_PHYSICAL_ERASE:
      status = physicalErase(&vol, ioreq);
      break;
#endif

#if FILES > 0
    case FL_OPEN_FILE:
      status = openFile(&vol,ioreq);
      break;

    case FL_CLOSE_FILE:
      status = closeFile(file);
      break;

#ifdef SPLIT_JOIN_FILE
    case FL_JOIN_FILE:
      status = joinFile(file, ioreq);
      break;

    case FL_SPLIT_FILE:
      status = splitFile(file, ioreq);
      break;
#endif

    case FL_READ_FILE:
      status = readFile(file,ioreq);
      break;

    case FL_WRITE_FILE:
      status = writeFile(file,ioreq);
      break;

    case FL_SEEK_FILE:
      status = seekFile(file,ioreq);
      break;

    case FL_FIND_FILE:
      status = findFile(&vol,file,ioreq);
      break;

    case FL_FIND_FIRST_FILE:
      status = findFirstFile(&vol,ioreq);
      break;

    case FL_FIND_NEXT_FILE:
      status = findNextFile(file,ioreq);
      break;

    case FL_GET_DISK_INFO:
      status = getDiskInfo(&vol,ioreq);
      break;

    case FL_DELETE_FILE:
      status = deleteFile(&vol,ioreq,FALSE);
      break;

#ifdef RENAME_FILE
    case FL_RENAME_FILE:
      status = renameFile(&vol,ioreq);
      break;
#endif

#ifdef SUB_DIRECTORY
    case FL_MAKE_DIR:
      status = makeDir(&vol,ioreq);
      break;

    case FL_REMOVE_DIR:
      status = deleteFile(&vol,ioreq,TRUE);
      break;
#endif

#endif	/* FILES > 0 */

    default:
      status = flBadFunction;
  }

#if FILES > 0
  if (buffer.checkPoint) {
    FLStatus st = flushBuffer(&vol);
    if (status == flOK)
      status = st;
  }
#endif

flCallExit:
  setBusy(&vol,TFFS_OFF);			/* We're leaving */

  return status;
}


#if POLLING_INTERVAL != 0

/*----------------------------------------------------------------------*/
/*      	   s o c k e t I n t e r v a l R o u t i n e		*/
/*									*/
/* Routine called by the interval timer to perform periodic socket      */
/* actions and handle the watch-dog timer.				*/
/*									*/
/* Parameters:                                                          */
/*      None								*/
/*                                                                      */
/*----------------------------------------------------------------------*/

/* Routine called at time intervals to poll sockets */
static void socketIntervalRoutine(void)
{
  unsigned volNo;
  Volume vol = vols;

  flMsecCounter += POLLING_INTERVAL;

  for (volNo = 0; volNo < noOfDrives; volNo++, pVol++)
#ifdef BACKGROUND
    if (flTakeMutex(&execInProgress,0)) {
#endif
      if (vol.flags & VOLUME_ABS_MOUNTED)
#ifdef BACKGROUND
	/* Allow background operation to proceed */
	vol.tl.tlSetBusy(vol.tl.rec,TFFS_OFF);
#endif
    if (flTakeMutex(&execInProgress,0)) {
      flIntervalRoutine(vol.socket);
      flFreeMutex(&execInProgress);
      }
#ifdef BACKGROUND
      flFreeMutex(&execInProgress);
    }
#endif
}

#endif

/*----------------------------------------------------------------------*/
/*		            f l I n i t					*/
/*									*/
/* Initializes the FLite system, sockets and timers.			*/
/*									*/
/* Calling this function is optional. If it is not called,		*/
/* initialization will be done automatically on the first FLite call.	*/
/* This function is provided for those applications who want to		*/
/* explicitly initialize the system and get an initialization status.	*/
/*									*/
/* Calling flInit after initialization was done has no effect.		*/
/*									*/
/* Parameters:                                                          */
/*	None								*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

FLStatus flInit(void)
{

  if (!initDone) {
    unsigned volNo;
    Volume vol = vols;

#ifdef SINGLE_BUFFER
    /* create mutex protecting FLite shared buffer */
    if (flCreateMutex(&execInProgress) != flOK)
      return flGeneralFailure;
#endif

    for (volNo = 0; volNo < DRIVES; volNo++, pVol++) {
      vol.socket = flSocketOf(volNo);
      vol.socket->volNo = volNo;
      vol.flags = 0;
#ifndef SINGLE_BUFFER
      /* create mutex protecting FLite volume access */
      if (flCreateMutex(&execInProgress) != flOK)
	return flGeneralFailure;
#endif
    }

    initDone = TRUE;

    flSysfunInit();

  #ifdef BACKGROUND
    flCreateBackground();
  #endif

    flRegisterComponents();

    checkStatus(flInitSockets());

  #if POLLING_INTERVAL > 0
    checkStatus(flInstallTimer(socketIntervalRoutine,POLLING_INTERVAL));
  #endif
  }

  return flOK;
}

#ifdef EXIT

/*----------------------------------------------------------------------*/
/*		            f l E x i t					*/
/*									*/
/* If the application ever exits, flExit should be called before exit.  */
/* flExit flushes all buffers, closes all open files, powers down the   */
/* sockets and removes the interval timer.				*/
/*                                                                      */
/* Parameters:                                                          */
/*	None								*/
/*                                                                      */
/* Returns:                                                             */
/*	Nothing								*/
/*----------------------------------------------------------------------*/

void flExit(void)
{
  unsigned volNo;
  Volume vol = vols;

  for (volNo = 0; volNo < noOfDrives; volNo++, pVol++) {
    if (setBusy(&vol,TFFS_ON) == flOK) {
      dismountVolume(&vol);
#ifdef LOW_LEVEL
      dismountLowLevel(&vol);
#endif
      flExitSocket(vol.socket);
      flFreeMutex(&execInProgress);  /* free the mutex that was taken in setBusy(TFFS_ON) */
#ifndef SINGLE_BUFFER
      /* delete mutex protecting FLite volume access */
      flDeleteMutex(&execInProgress);
#endif

    }
  }

#if POLLING_INTERVAL != 0
  flRemoveTimer();
#endif

#ifdef SINGLE_BUFFER
  /* delete mutex protecting FLite shared buffer */
  flDeleteMutex(&execInProgress);
#endif

  initDone = FALSE;
}

#ifdef __BORLANDC__

#pragma exit flExit

#include <dos.h>

static int cdecl flBreakExit(void)
{
  flExit();

  return 0;
}

static void setCBreak(void)
{
  ctrlbrk(flBreakExit);
}

#pragma startup setCBreak

#endif

#endif

