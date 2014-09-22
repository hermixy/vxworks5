/* dosFsFmtLib.c - MS-DOS media-compatible file system formatting library */ 

/* Copyright 1984-2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
02b,29apr02,jkf  SPR#76013, dosFsVolFormat shall cbioUnlock all it cbioLock's
02a,12dec01,jkf  fixing diab warnings
01z,10dec01,jkf  SPR#72039, MSDOS FAT32 fsInfo sector now using hard coded 
                 offsets to properly support blkSize > 512bps.
                 increase check for bytesPerBlk minimum in dosFsFmtAutoParams.
01y,15nov01,jkf  SPR#71720, avoid unaligned pointer access.
01x,09nov01,jkf  SPR#71633, dont set errno when VolFormat suceeded.
01w,20sep01,jkf  SPR#69031, common code for both AE & 5.x.
01v,08dec00,jkf  SPR#34704,FAT12/FAT16 determination, SPR#62415 sig location.
01u,15sep00,jkf  cleaning warning
01t,29feb00,jkf  T3 changes, cleanup
01s,18oct99,jkf  avoiding div by zero if nHeads*blksPerTrack = 0, SPR#29508
01r,27oct99,jkf  Documentation correction for compatibility section.
01q,03oct99,jkf  setup a pseudo LCHS translation to mimic MSDOS more.
                 This helps VxLd on large disks formatted by us.
01p,03oct99,jkf  removed random signature, breaks vxsys booting.
                 Correction from vxsys investigation, removed random
                 signature, configure hidden sectors to sec per track,
                 change nClust calculation to match Microsoft.
01o,03oct99,jkf  added sysId check to dosFsFmtReadBootBlock, 
                 changed reserved sectors setting to one in 
                 dosFsVolFormat. Disabled the ret instructions
                 being written in dosFsFmtNonFsBootInit.
01n,15sep99,jkf  changes for new CBIO API.
01m,31jul99,jkf  FAT12/FAT16 calculation per NT, SPR#28274. 
                 improved media byte support, SPR#27282. 
		 added support FSTYPE (0x36) in boot sector, SPR#28273.
01l,16jun99,jkf  correctly set FS ID for FAT32, SPR#28275.
01k,12jul99,jkf  T2 merge, tidiness & spelling.
                 (allows Windows to mount our FAT32)
01j,30nov98,lrn  changed JMP instruction to be Win98 compatible (SPR#23442)
01i,07sep98,lrn  fixed formatting of 4 MB disks to be FAT16
01h,30jul98,wlf  partial doc cleanup
01g,14jul98,lrn  replaced perror() with printErr
01f,12jul98,lrn  fixed: option may force format to FAT16
01e,22jun98,lrn  vol desc name change
01d,03jun98,lrn  Libinit, integ, increased max root dirs for small disks
01c,03jun98,lrn  polished main function and interactive params control, doc
01b,14may98,lrn  added VxLong names 
01a,12may98,lrn  initial version
*/

/*
DESCRIPTION
This module is a scaleable companion module for dosFsLib, 
and is intended to facilitate high level formatting of 
disk volumes.

There are two ways to high level format a volume:
.IP "(1)"
Directly calling dosFsVolFormat() routine allows to have complete
control over the format used, parameters and allows to supply a hook
routine which for instance could interactively prompt the user to
modify disk parameters.
.IP "(2)"
Calling ioctl command FIODISKINIT will invoke the formatting routine
via dosFsLib. This uses the default volume format and parameters.

AVAILABILITY
This routine is an optional part of the MS-DOS file system,
and may be included in a target system if it is required to
be able to format new volumes.

In order to include this option, the following function needs
to be invoked during system initialization:
.CS
void dosFsFmtLibInit( void );
.CE

See reference page dosFsVolFormat() for complete description of
supported formats, options and arguments.

SEE ALSO
dosFsLib
*/

/* includes */
#include "vxWorks.h"
#include "private/dosFsVerP.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "ctype.h"
#include "errnoLib.h"
#include "memLib.h"
#include "tickLib.h"

#include "dosFsLib.h"
#include "private/dosFsLibP.h"
#include "private/dosDirLibP.h"

/* defines */

/* defines - FAT Boot Sector values */
#define DOS_BOOT_SEC_NUM        0       /* sector number of boot sector */
#define DOS_MIN_CLUST           2       /* lowest cluster number used */

/* most of these are old defaults which are now calculated */
#define DEFAULT_ROOT_ENTS       112     /* default # of root dir entries */
#define DEFAULT_MAX_ROOT_ENTS   512     /* default max # of root dir entries */
#define DEFAULT_SEC_PER_CLUST   2       /* default sectors per cluster */
#define DEFAULT_MEDIA_BYTE      0xF8    /* default media byte value */
#define DEFAULT_NFATS           2       /* default number FAT copies */
#define MAX_NFATS		16	/* maximum number FAT copies */
#define DEFAULT_NRESERVED       1       /* default # of reserved sec's (min=1)*/

#define DOS_FAT_12BIT_MAX       4085    /* max clusters, 12 bit FAT entries */

/* 
 * FIXME - these are not really -2, but -11, but it is ok because
 * we then should take into account the FAT sectors overhead when
 * calculating cluster size.
 */

#define DOS_FAT_16BIT_MAX   (0x10000-2) /* max clusters, 16 bit FAT */
#define DOS_FAT_32BIT_MAX (0x200000-2)  /* max clusters, 32-bit FAT entries */

#define	DOS32_INFO_SEC		1	/* FAT32 info sector location */
#define	DOS32_BACKUP_BOOT	6	/* FAT32 backup boot block location */


/*******************************************************************************
*
* dosFsFmtShow - print volume parameters to stdout
*
*/
LOCAL void dosFsFmtShow( DOS_VOL_CONFIG * pConf )
    {
    printf("Volume Parameters: FAT type: FAT%d, sectors per cluster %d\n",
	pConf->fatType, pConf->secPerClust);
    printf("  %d FAT copies, %ld clusters, %ld sectors per FAT\n",
	pConf->nFats, pConf->nClust, pConf->secPerFat );
    printf("  Sectors reserved %d, hidden %ld, FAT sectors %ld\n",
	pConf->nResrvd, pConf->nHidden, pConf->secPerFat* pConf->nFats);
    printf("  Root dir entries %d, sysId %-8s, serial number %lx\n",
	pConf->maxRootEnts, pConf->sysId, pConf->volSerial );
    printf("  Label:\"%-11s\" ...\n", pConf->volLabel );
    }

/*******************************************************************************
*
* dosFsFmtAutoParams - automatically calculate FAT formatting params
*
* This function attempts to imitate MSFT formulae for setting
* disk parameters for maximum compatibility.
* For fully automatic configuration, the configuration structure
* should be all zeroed out.
* If any of the modifiable fields are non-zero, this function will
* calculate the rest of the parameters, honoring the values filled
* in prior to the call.
*
* Note however that MSFT compatibility can not be always maintained
* unless fully automatic configuration is performed.
*
*/
LOCAL STATUS dosFsFmtAutoParams
    (
    DOS_VOL_CONFIG * pConf,	/* config params structure */
    ULONG nBlks,		/* # of secs on volume */
    int bytesPerBlk,		/* sector size */
    int opt			/* VxLongs */
    )
    {
    ULONG nClust, maxClust, minClust ;
    int rootSecs, fatBytes ;
    int dirEntrySize = DOS_DIRENT_STD_LEN;

    /* 
     * adjust the directory entry size if using vxLongnames which
     * use a unique directory entry size.
     */

    if( opt & DOS_OPT_VXLONGNAMES )
	dirEntrySize = DOS_VX_DIRENT_LEN;

    /* verify bytes per block supports the file system */

    if( (bytesPerBlk < (dirEntrySize *3))     || 
        (bytesPerBlk < 64 && nBlks > 0xfffe) )
	{
	errno = EINVAL;
	return ERROR;
	}
    
    /* setup some pConf field based on disk size */

    if( bytesPerBlk < 512 )
	{
	/* most likely a tiny RAMdisks */
	if(pConf->secPerClust == 0 )	
	    pConf->secPerClust = 1 ;
	if(pConf->mediaByte == 0 )	
	    pConf->mediaByte = 0xfd;  /* TODO, what to use for RAMDISK? */
	if(pConf->nFats == 0 )		
	    pConf->nFats = 1;
	rootSecs = (((DEFAULT_ROOT_ENTS * dirEntrySize) + (bytesPerBlk -1)) / 
			 bytesPerBlk);
	minClust = 1 ;
	}

    else if (nBlks <= 720) /* i.e. 360KB 5.25" */
	{
	/* temporary value, later scaled up as needed */
	if(pConf->secPerClust == 0)	
	    pConf->secPerClust = 1 ;
	if(pConf->mediaByte == 0)	
	    pConf->mediaByte = 0xfd; /* per NT Resource Kit */
	if(pConf->fatType == _AUTO)	
	    pConf->fatType = _FAT12; /* floppies use FAT12 */
	pConf->maxRootEnts = DEFAULT_ROOT_ENTS; /* always 112 for 360KB */
	rootSecs = (((DEFAULT_ROOT_ENTS * dirEntrySize) + (bytesPerBlk -1)) / 
			 bytesPerBlk);
	minClust = 2;
	}

    else if (nBlks <= 1440 ) /* i.e. 720KB 3.5" */
	{
	/* temporary value, later scaled up as needed */
	if( pConf->secPerClust == 0)	
	    pConf->secPerClust = 1 ;
	if( pConf->mediaByte == 0)	
	    pConf->mediaByte = 0xf9; /* per NT Resource Kit */ 
	if(pConf->fatType == _AUTO)	
	    pConf->fatType = _FAT12; /* floppies use FAT12 */
	pConf->maxRootEnts = DEFAULT_ROOT_ENTS; /* always 112 for 720KB */
	rootSecs = (((DEFAULT_ROOT_ENTS * dirEntrySize) + (bytesPerBlk -1)) / 
			 bytesPerBlk);
	minClust = 2;
	}
    else if (nBlks <= 2400 ) /* i.e. 1.2Mb 5.25" */
	{
	/* temporary value, later scaled up as needed */
	if(pConf->secPerClust == 0)	
	    pConf->secPerClust = 1 ;
	if(pConf->mediaByte == 0)	
	   pConf->mediaByte = 0xf9;/* per NT Resource Kit */
	if(pConf->fatType == _AUTO)	
	    pConf->fatType = _FAT12; /* floppies use FAT12 */
	pConf->maxRootEnts = DEFAULT_ROOT_ENTS;
	rootSecs = (((DEFAULT_ROOT_ENTS * dirEntrySize) + (bytesPerBlk -1)) / 
			 bytesPerBlk);
	minClust = 2;
	}
    else if( nBlks <= 2880 ) /* i.e. 1.44MB 3.5" floppy */
	{
	/* temporary value, later scaled up as needed */
	if(pConf->secPerClust == 0)	
	    pConf->secPerClust = 1 ;
	if(pConf->mediaByte == 0)	
	    pConf->mediaByte = 0xf0;/* Per NT Resource Kit */
	if(pConf->fatType == _AUTO)	
	    pConf->fatType = _FAT12; /* floppies use FAT12 */
	pConf->maxRootEnts = DEFAULT_ROOT_ENTS; 
	rootSecs = (((DEFAULT_ROOT_ENTS * dirEntrySize) + (bytesPerBlk -1)) / 
			 bytesPerBlk);
	minClust = 2;
	}
    else if( nBlks <= 4000 ) /* probally a hard disk or PCMCIA, FLASH etc.*/
	{
	/* temporary value, later scaled up as needed */
	if(pConf->secPerClust == 0)	
	    pConf->secPerClust = 1;
	if(pConf->mediaByte == 0)	
	    pConf->mediaByte = DEFAULT_MEDIA_BYTE;
	if(pConf->fatType == _AUTO)	
	    pConf->fatType = _FAT12; /* floppies use FAT12 */
	pConf->maxRootEnts = DEFAULT_ROOT_ENTS; 
	rootSecs = (((DEFAULT_ROOT_ENTS * dirEntrySize) + (bytesPerBlk -1)) / 
			 bytesPerBlk);
        minClust = 1 ;
	}
    else
	{
	/* temporary value, later scaled up as needed */
	if(pConf->secPerClust == 0)	
	    pConf->secPerClust = 2;
	if(pConf->mediaByte == 0)	
	    pConf->mediaByte = DEFAULT_MEDIA_BYTE;
	pConf->maxRootEnts = DEFAULT_MAX_ROOT_ENTS;
	rootSecs = (((DEFAULT_ROOT_ENTS * dirEntrySize) + (bytesPerBlk -1)) / 
			 bytesPerBlk);
	minClust = 2;
	}

    /* fixup root directory entries */
    if( pConf->maxRootEnts != 0 )
	{
	rootSecs = (((pConf->maxRootEnts * dirEntrySize) + (bytesPerBlk -1)) / 
		    bytesPerBlk);
	}

    if( pConf->nFats == 0 )		
	pConf->nFats = DEFAULT_NFATS;

    pConf->nFats = min( pConf->nFats, MAX_NFATS );

    /* temporary value, may be way too large */

    nClust = nBlks / pConf->secPerClust ;

    /* define FAT type first, many other params depend on it */
    if( pConf->fatType == _AUTO )
	{
	/* 
	 * Here we decide which FAT format to use.
	 */
	if(nClust < DOS_FAT_12BIT_MAX)
	    pConf->fatType = _FAT12 ;	/* <16MB volume, FAT12 */
	else if ( nBlks < 0x400000 )	
	    pConf->fatType = _FAT16 ;   /* 16MB+ <2GB, FAT16 */
	else			
	    pConf->fatType = _FAT32 ;	/* 2GB+ volume, FAT32 */
	}

    switch(pConf->fatType)
	{
	case _FAT32:
	    if( pConf->nResrvd == 0)		
		pConf->nResrvd = 32;
	    maxClust = DOS_FAT_32BIT_MAX;
	    minClust = 8;
	    break;
	default:
	case _FAT16:
	    if( pConf->nResrvd == 0)		
		pConf->nResrvd = 1;
	    maxClust = DOS_FAT_16BIT_MAX;
	    minClust = 2;
	    break;
	case _FAT12:
	    if( pConf->nResrvd == 0)		
		pConf->nResrvd = 1;
	    maxClust = DOS_FAT_12BIT_MAX;
	    minClust = 1;
	    break;
	}


    /* now, we know the FAT type, so we can really calculate secs per clust */

    while( (nClust > maxClust ) || ((ULONG)pConf->secPerClust < minClust) )
	{
	pConf->secPerClust <<= 1 ;	/* multiply secs/clust by 2 */
	nClust = nBlks / pConf->secPerClust ;	/* recalc */
	}

    /* max secs/clust is limited to 254 */

    pConf->secPerClust = min( pConf->secPerClust, 254 );
    nClust = nBlks / pConf->secPerClust ;	/* recalc */

    switch( pConf->fatType )
	{
	case _FAT32:
	    fatBytes = nClust * 4;
	    rootSecs = 0 ; /* root dir is a normal cluster */
	    if(pConf->sysId==NULL) pConf->sysId = "VX5DOS32";
	    break;
	default:
	case _FAT16:
	    fatBytes = nClust * 2;
	    if(pConf->sysId==NULL) pConf->sysId = "VXDOS16";
	    break;
	case _FAT12:
	    fatBytes = nClust * 3 / 2;
	    if(pConf->sysId==NULL) pConf->sysId = "VXDOS12";
	    break;
	}

    /* setup a special id for longnames */

    if( opt & DOS_OPT_VXLONGNAMES )
	{
	pConf->sysId = "VXEXT1.1";
	}

    /* calculate sectors per fat */

    pConf->secPerFat =  (fatBytes + bytesPerBlk - 1) / bytesPerBlk ;

    /* figure out root dir size */

    pConf->maxRootEnts = rootSecs * bytesPerBlk / dirEntrySize ;

    /* root entries are never bigger than 512 */

    if ((pConf->fatType != _FAT32) && (pConf->maxRootEnts > DEFAULT_MAX_ROOT_ENTS))
	{
        pConf->maxRootEnts = DEFAULT_MAX_ROOT_ENTS;
	}

    /* one last time, recalculate cluster nClust, knowing reserved */

    nClust = (nBlks - pConf->nResrvd - rootSecs - pConf->nHidden -
		(pConf->secPerFat * pConf->nFats ) ) /
		pConf->secPerClust;

    pConf->nClust = nClust;

    /* see if it all sums up */

    if((pConf->nHidden + pConf->nResrvd + rootSecs +
		(pConf->secPerFat * pConf->nFats ) +
		(pConf->nClust * pConf->secPerClust))  > nBlks )
	goto error;

    if( (pConf->secPerClust <= 255 ) )
	return OK ;
error:
    errno = EINVAL ;
    return ERROR;
    }

/*******************************************************************************
*
* dosFsFmtReadBootBlock - read existing boot block on the disk
*
* Read the existing boot block on the disk, to salvage some fields
* that should be preserved, if the volume is already formatted.
*
* RETURNS: results confidence level: 100 is the highest confidence, 
* a lower value means more probability for bogus values, or ERROR if 
* could not read block.
*/
LOCAL int dosFsFmtReadBootBlock
    (
    CBIO_DEV_ID		cbioDev,
    DOS_VOL_CONFIG *	pConf
    )
    {
    int confidence = 100 ;
    STATUS stat ;
    u_char bootBlockBuffer [ 512 ] ;  	/* must be at least 512 bytes */
    u_char * pBoot ;
    u_char c1 ;
    u_long i, work ;
    int bytesPerBlk;
    u_char tmpType [ DOS_BOOT_FSTYPE_LEN + 1] = { 0 };
    CBIO_PARAMS cbioParams;

     /* Below we request the (underlying CBIO) device parameters.  */

    if (ERROR == cbioParamsGet (cbioDev, &cbioParams))
	{
	return (ERROR);
	}

    bytesPerBlk = cbioParams.bytesPerBlk ;

    if (0 == bytesPerBlk)
	{
	return (ERROR);
	}

    if( bytesPerBlk > (int) sizeof(bootBlockBuffer) )
	{
	pBoot = KHEAP_ALLOC(bytesPerBlk);
	}
    else
	{
	pBoot = bootBlockBuffer ;
	}

    if (NULL == pBoot)
	{
	return (ERROR);
	}

    bzero( (char *) pBoot, bytesPerBlk);

    /* read the boot block */

    stat = cbioBlkRW( cbioDev, DOS_BOOT_SEC_NUM, 1,
		       (addr_t)pBoot, CBIO_READ, NULL );

    if( stat == ERROR )
	return (ERROR);

    /* 
     * inspect the boot block fields, and decrease confidence value
     * each time a field seems bogus, by something relative to
     * level of criticality of an appropriate field
     */

    /* check for either 80x80 JMP instruction opcodes */

    if( pBoot[ DOS_BOOT_JMP ]  != 0xe9 && pBoot[ DOS_BOOT_JMP ] != 0xeb )
	confidence -= 5;

    /* 
     * Many FAT documents mistakenly state that the 0x55aa signature word
     * "occupies the last two bytes of the boot sector".  That is only true 
     * when the bytes-per-sector value is 512 bytes.  Microsoft
     * defines the signature at offsets 510 and 511 (zero-based)
     * regardless of the sector size.  It is okay, however to have
     * the signature at the end of the sector.  We accept either.
     */

    /* First, check for a signature at the end of the sector. */

    if((pBoot[ bytesPerBlk - 2 ] != 0x55 ) || (pBoot[ bytesPerBlk-1] != 0xaa))
	{
        /* Didn't find signature, check the 510 and 511 offsets. */
        if ((pBoot[510] != 0x55) || (pBoot[511] != 0xaa))
	    {
	    confidence -= 5; /* no 0x55aa signature :-( */
	    }
	}

    if( DISK_TO_VX_16(pBoot+DOS_BOOT_BYTES_PER_SEC) != bytesPerBlk )
	confidence -= 5 ;

    if((pConf->secPerClust = pBoot[ DOS_BOOT_SEC_PER_CLUST ]) == 0)
	confidence -= 5 ;

    if((pConf->mediaByte = pBoot[ DOS_BOOT_MEDIA_BYTE ]) == 0)
	confidence -= 2;

    if((work = DISK_TO_VX_16( pBoot + DOS_BOOT_NRESRVD_SECS )) == 0 )
	confidence -= 5;
    else
	pConf->nResrvd = work ;

    if((pConf->nFats = pBoot[ DOS_BOOT_NFATS ]) == 0)
	confidence -= 5 ;

    if((work = DISK_TO_VX_16( pBoot + DOS_BOOT_MAX_ROOT_ENTS )) == 0 )
	confidence -= 5 ;
    else
	pConf->maxRootEnts = work ;

    if((work = DISK_TO_VX_16( pBoot + DOS_BOOT_NSECTORS )) == 0 )
        {
        if((work = DISK_TO_VX_32( pBoot + DOS_BOOT_LONG_NSECTORS )) == 0)
	    confidence -= 5;
        }

    /* a problem here may indicate a problem in cbio layer partitioning */

    if( work > cbioParams.nBlocks )
	confidence -= 5;

    /* setup the fat type and sectors per fat */

    if((work = DISK_TO_VX_16(pBoot + DOS_BOOT_SEC_PER_FAT)) == 0 )
	{
	/* when 16 bit sectors per FAT field is zero, we presume FAT32 */
	if((work = DISK_TO_VX_32( pBoot + DOS32_BOOT_SEC_PER_FAT )) == 0 )
	    confidence -= 5 ;

	bcopy((char *) pBoot + DOS32_BOOT_FS_TYPE, 
	      (char *) tmpType, DOS_SYS_ID_LEN );

	if(strcmp ((char *)tmpType, DOS_BOOT_SYSID_FAT32) != 0 )
	    confidence -= 5 ;

	pConf->fatType = _FAT32;
	pConf->secPerFat = work; 
	}
    else /* must be using either FAT12 or FAT16 */
	{
	/* 
	 * Not using FAT32, determine between FAT12 and FAT16,
	 * and test the cluster count against the string, if present. 
	 */

	pConf->secPerFat = work;

        bcopy ( (char *) pBoot + DOS_BOOT_FSTYPE_ID,
                (char *) tmpType, DOS_BOOT_FSTYPE_LEN);

        stat = dosFsVolIsFat12 (pBoot);
 
        if (ERROR == stat)
            {
	    confidence -= 5;
            }

	if (TRUE == stat) /* calc'd FAT12 */
	    {	
	    /* 
	     * Check the FSTYPE field in the BPB to ensure the string
	     * value matches our calculation.  If not, the we assume
	     * the formatter knew what they wanted, and we honor
	     * the string value. We look for "FAT12   " or "FAT16   ".
	     */

	    if ((strcmp ((char *)tmpType, DOS_BOOT_FSTYPE_FAT16)) == 0)
	        {
    	        pConf->fatType = _FAT16;
	        printf("dosFsFmtLib: %s indicated by BPB FSTYPE string, "
		       "cluster calculation was FAT12. Returning string.\n",
			tmpType);
	        }
	    else 
		{
    	        pConf->fatType = _FAT12;
	    	}
	    }
	else /* calc'd FAT16 */
	    {
	    /* 
	     * Check the FSTYPE field in the BPB to ensure the string
	     * value matches our calculation.  If not, the we assume
	     * the formatter knew what they wanted, and we honor
	     * the string value. We look for "FAT12   " or "FAT16   ".
	     */
	    if ((strcmp ((char *)tmpType, DOS_BOOT_FSTYPE_FAT12)) == 0)
	        {
    	        pConf->fatType = _FAT12;
	        printf("dosFsFmtLib: %s indicated by BPB FSTYPE string, "
		       "cluster calculation was FAT16. Returning string.\n",
			tmpType);
	        }
	    else
		{
    	        pConf->fatType = _FAT16;
	    	}
	    }
	}

    /* this is not the CHKDSK, no need to be absolutely perfect. */

    if( (pConf->nHeads = DISK_TO_VX_32( pBoot + DOS_BOOT_NHEADS))== 0)
	confidence -=5 ;

    if((pConf->secPerTrack = DISK_TO_VX_32(pBoot + DOS_BOOT_SEC_PER_TRACK))==0)
	confidence -=5 ;

    /* get volume serial number, this is what its all about, actually */

    if( pConf->fatType == _FAT32 )
	work = DISK_TO_VX_32( pBoot + DOS32_BOOT_VOL_ID ) ;
    else
	work = DISK_TO_VX_32( pBoot + DOS_BOOT_VOL_ID ) ;

    /* scrutinize the serial number */

    if( (work & 0xffff) == ((work >> 16) & 0xffff) || (work & 0xffff0000) == 0)
	{
	/* found suspicious serial number, so we mangle it XXX WHY? */
	confidence -= 5 ;
	work |= tickGet() << 16 ;
	}

    pConf->volSerial = work;

    /* get label, should be useful too */

    if( pConf->fatType == _FAT32 )
	i = DOS32_BOOT_VOL_LABEL;
    else
	i = DOS_BOOT_VOL_LABEL;

    bcopy( (char *)pBoot + i, pConf->volLabel, DOS_VOL_LABEL_LEN );

    /* replace any illegal character with a space */

    for(i = 0; i< DOS_VOL_LABEL_LEN; i++)
	{
	c1 = pConf->volLabel[i] ;
	if( ! isupper(c1) && ! isdigit(c1) && (c1 != '_') && (c1 != '$'))
	    pConf->volLabel[i] = ' ';
	}

    /* preserve hidden sectors */
    pConf->nHidden = DISK_TO_VX_32 (pBoot + DOS_BOOT_NHIDDEN_SECS);


    /* Done */

    if( pBoot != bootBlockBuffer )
	KHEAP_FREE(pBoot);
    if( confidence < 0 )
	confidence = 0 ;
    return( confidence  );
    }

/*******************************************************************************
*
* dosFsFmtNonFsBootInit - initialize non-fs fields in boot block
*
* Set areas such as JUMP command, boot code,  signature
* 
*/
LOCAL void dosFsFmtNonFsBootInit
    ( 
    u_char * pBoot,   /* comment for this arg */
    int bytesPerBlk   /* comment for this arg */
    )
    {

    /* 
     * 0xEB is a x86, JMP rel8, Jump short, relative, 
     * displacement relative to next instruction, 0x3e
     * The 0x90 is a NOP instruction and it is an alias 
     * mnemonic for the XCHG (E)AX, (E)AX instruction.   
     */

    pBoot[ 0 ] = 0xeb ;		/* JUMP */
    pBoot[ 1 ] = 0x3e ; 	/* program offset */
    pBoot[ 2 ] = 0x90 ; 	/* NOP */

    /* 
     * Many FAT documents mistakenly say that the 0x55AA signature word
     * "occupies the last two bytes of the boot sector".  That is only true 
     * when the bytes-per-sector value is 512 bytes.  Microsoft
     * defines the signature at offsets 510 and 511 (zero-based)
     * regardless of the sector size.  It is okay, however to have
     * the signature also at the end of the sector.  We write it to
     * both places to be safe.
     */

    pBoot[ bytesPerBlk-2 ] = 0x55 ; 
    pBoot[ bytesPerBlk-1 ] = 0xaa ;

    if (bytesPerBlk > 512)
	{
        pBoot[ 510 ] = 0x55 ; 
        pBoot[ 511 ] = 0xaa ;
	}
    }


/*******************************************************************************
* 
* dosFsFmtVolInit - perform the actual disk formatting
*
* Initialize all file system control structure on the disk, optionally
* preserving the boot block non-file system fields such as boot code
* and partition table.
*
*/
LOCAL int dosFsFmtVolInit
    (
    CBIO_DEV_ID		cbioDev,		/* cbio handle */
    DOS_VOL_CONFIG *	pConf,		/* volume parameters */
    BOOL		preserve,	/* =1 - preserve boot blocks fields */
    int			opt		/* VxLong names */
    )
    {
    STATUS stat = ERROR ;
    u_char bootBlockBuffer [512] ;    
    u_char * pBoot ;
    u_char dirEntrySize = DOS_DIRENT_STD_LEN;	
    int backupBootSec = NONE ;
    u_long i, work, rootStartSec ;
    int bytesPerBlk;
    CBIO_PARAMS cbioParams;

    /* 
     * Below we request the (underlying CBIO) device parameters.  
     * These parameters will be used to help calculate this dosFs volumes 
     * configuration.  The information will be used to help
     * determine the FAT type, the sectors per cluster, etc.
     * For example, the number of sectors (aka blocks) on the device returned 
     * by the cbioParamsGet call must reflect the partitions total block 
     * values, which is NOT necessarily the entire disk; else the calculations 
     * may be incorrect.
     */

    if (ERROR == cbioParamsGet (cbioDev, &cbioParams))
	{
	return (ERROR);
	}

    bytesPerBlk = cbioParams.bytesPerBlk ;

    if (0 == bytesPerBlk)
	{
	return (ERROR);
	}

    if( opt & DOS_OPT_VXLONGNAMES )
	dirEntrySize = DOS_VX_DIRENT_LEN;	/* Thanks Kent! */
    /* 
     * Need enough space for at least a sector, 
     * depending on sector size we may KHEAP_ALLOC it.
     */

    if( bytesPerBlk > (int)sizeof(bootBlockBuffer) )
	pBoot = KHEAP_ALLOC(bytesPerBlk);
    else
	pBoot = bootBlockBuffer ;

    if (NULL == pBoot)
	{
	return (ERROR);
	}

    bzero( (char *)pBoot, bytesPerBlk );

    /* clear out the entire FAT, all copies of it ! */

    for(i = 0; i < (pConf->secPerFat * pConf->nFats); i++ )
	{
	if(cbioBlkRW( cbioDev, i + DOS_BOOT_SEC_NUM + pConf->nResrvd, 1,
	             (addr_t)pBoot, CBIO_WRITE, NULL ) == ERROR )
	    {
	    goto _done_;
	    }
	}

    /* clear out the entire Root directory */

    /* size of Root Dir in sectors */

    if( pConf->fatType == _FAT32 )
	{
	work = pConf->secPerClust ;  
	}
    else /* FAT16/FAT12 */
	{
	/* SPR#34704 This must round up */
	work = (((pConf->maxRootEnts * dirEntrySize) + (bytesPerBlk - 1)) /
		bytesPerBlk);
	}

    /* this is where the Root dir starts */

    rootStartSec = DOS_BOOT_SEC_NUM + pConf->nResrvd +
			(pConf->secPerFat * pConf->nFats);

    /* clear it really now */

    for( i = 0; i < work; i++ )
	if( cbioBlkRW( cbioDev, i+rootStartSec,
		1, (addr_t)pBoot, CBIO_WRITE, NULL ) == ERROR )
	    goto _done_;

    /* 
     * now that we are done with zero's, we initialize the FAT's head
     * with the required beginning for the FAT type, this appears: 
     *
     * FAT12: f8 ff ff xx xx xx xx xx xx xx xx xx xx xx
     * FAT16: f8 ff ff ff xx xx xx xx xx xx xx xx xx xx
     * FAT32: f8 ff ff ff ff ff ff ff ff ff ff 0f xx xx 
     * f8 is the media byte.
     *  
     * Noticed that Win95 OSR2 FAT32 uses:
     *        f8 ff ff 0f ff ff ff 0f ff ff ff 0f xx xx 
     * Probally due to simplified formatting code, does not 
     * affect anything in MS ScanDisk or Norton Disk Doctor,
     * so we just leave it as is here, just noted.
     */

    if( pConf->fatType == _FAT32 )	/* how many bytes to init */
	{
	work = 11 ;			/* 0, 1, and 2 entry for root dir */
	pBoot[ 11 ] = 0x0f;		/* root dir EOF msb */
	}
    else if( pConf->fatType == _FAT16 )
	work = 4 ;
    else  /* FAT12 */
	work = 3 ;

    pBoot[ 0 ] = pConf->mediaByte ;		/* 0th byte always media */

    for(i=1; i < work ; i++ )
	pBoot[i] = 0xff ;			/* 1th to i-th bytes 0xff */

    for(i = DOS_BOOT_SEC_NUM + pConf->nResrvd ;
	i < DOS_BOOT_SEC_NUM + pConf->nResrvd 
	                     + pConf->secPerFat * pConf->nFats;
	i += pConf->secPerFat )

	if( cbioBlkRW( cbioDev, i , 1, (addr_t)pBoot, CBIO_WRITE, 
			NULL ) == ERROR )
	    goto _done_;

    /* create file system info sector for FAT32 */

    if( pConf->fatType == _FAT32 )
	{
	/* FS info sector  */

	bzero( (char *)pBoot, bytesPerBlk );

	/* FSI_LeadSig (magic signature) */

	pBoot[ 0 ] = 'R';			/* 0x52 */
	pBoot[ 1 ] = 'R';			/* 0x52 */
	pBoot[ 2 ] = 'a';			/* 0x61 */
	pBoot[ 3 ] = 'A';			/* 0x41 */

	/* FSI_StructSig */

	pBoot[ 484 ] = 'r';			/* 0x72 */
	pBoot[ 485 ] = 'r';			/* 0x72 */
	pBoot[ 486 ] = 'A';			/* 0x41 */
	pBoot[ 487 ] = 'a';			/* 0x61 */

        /* 
         * Many FAT documents mistakenly say that the 0x55AA signature word
         * "occupies the last two bytes of the boot sector".  That is only 
	 * true when the bytes-per-sector value is 512 bytes.  Microsoft
         * defines the signature at offsets 510 and 511 (zero-based)
         * regardless of the sector size.  It is okay, however to have the 
	 * signature also at the end of the sector.  We do both to be safe.
         */

	pBoot[ bytesPerBlk - 1 ] = 0xaa;	/* tail signature */
	pBoot[ bytesPerBlk - 2 ] = 0x55;	

        if (bytesPerBlk > 512)
	    {
	    pBoot[510] = 0x55;	
	    pBoot[511] = 0xaa;			/* tail signature */
	    }

	/* 
	 * The FAT32 `free count of clusters' field will be set 0xffffffff 
	 * This indicates to MSDOS to not consider the field valid.
         * Note dosFsLib does not update this field currently.
         * A possible setting is shown below in comments.
	 */

        pBoot[ 488 ] = 0xff;       /* &   (pConf->nClust - 1); */
        pBoot[ 489 ] = 0xff;       /* & ( (pConf->nClust - 1) >> 8); */
        pBoot[ 490 ] = 0xff;       /* & ( (pConf->nClust - 1) >> 16); */
        pBoot[ 491 ] = 0xff;       /* & ( (pConf->nClust - 1) >> 24); */

	/* 
	 * The FAT32 `next free cluster' field will be 0xffffffff 
	 * This indicates to MSDOS to not consider the field valid.
         * Note dosFsLib does not update this field currently.
	 */

        pBoot[ 492 ] = 0xff; 
        pBoot[ 493 ] = 0xff;
        pBoot[ 494 ] = 0xff;
        pBoot[ 495 ] = 0xff;

	/* now write this info sector to disk */

	if( cbioBlkRW( cbioDev, DOS32_INFO_SEC , 1, 
		(addr_t)pBoot, CBIO_WRITE, NULL ) == ERROR )
	    goto _done_;
	}

    /* create a boot sector image, and write it last */
    /* first, prepare the non-fs boot block data */

    if(TRUE == preserve)
	{
	if( cbioBlkRW( cbioDev, DOS_BOOT_SEC_NUM , 1, 
		(addr_t)pBoot, CBIO_READ, NULL ) == ERROR )
	    goto _done_;
	}
    else
	{
	bzero( (char *)pBoot, bytesPerBlk );
	dosFsFmtNonFsBootInit( pBoot, bytesPerBlk );
	}
	
    /* now write the fields which are relevant to our params */

			/* byte fields copied directly */
    pBoot [DOS_BOOT_SEC_PER_CLUST]	= pConf->secPerClust ;
    pBoot [DOS_BOOT_NFATS]             	= pConf->nFats ;
    pBoot [DOS_BOOT_MEDIA_BYTE]		= pConf->mediaByte ;

    bcopy( pConf->sysId,  (char *)pBoot+DOS_BOOT_SYS_ID, DOS_SYS_ID_LEN );

    /* 16-bit values */
    if( FALSE == preserve )
	{

	/* 
         * Small Hack with Big Comment. Mimic MSDOS n based translation.
         * This is just to appease MSDOS BIOS calls.
         * DOSFSII could care less really, but it helps MSDOS programs.
         * (such as our VxLd) Typical LCHS to CHS BIOS translation appears:
         *
         * +----------------+----------+------------+-------------+--------+
         * |Actual Cylinders|Actual Hds|Altered Cyl |Altered Heads|Max Size|
         * +----------------+----------+------------+-------------+--------+
         * |  1<C<=1024     | 1<H<=16  |     C=C    |    H=H      |528 MB  |
         * +----------------+----------+------------+-------------+--------+
         * | 1024<C<=2048   | 1<H<=16  |     C=C/2  |    H=H*2    |  1GB   |
         * +----------------+----------+------------+-------------+--------+
         * | 2048<C<=4096   | 1<H<=16  |     C=C/4  |    H=H*4    | 2.1GB  |
         * +----------------+----------+------------+-------------+--------+
         * | 4096<C<=8192   | 1<H<=16  |     C=C/8  |    H=H*8    | 4.2GB  |
         * +----------------+----------+------------+-------------+--------+
         * | 8192<C<=16384  | 1<H<=16  |     C=C/16 |    H=H*16   | 8.4GB  |
         * +----------------+----------+------------+-------------+--------+
         * | 16384<C<=32768 | 1<H<=8   |     C=C/32 |    H=H*32   | 8.4GB  |
         * +----------------+----------+------------+-------------+--------+
         * | 32768<C<=65536 | 1<H<=4   |     C=C/64 |    H=H*64   | 8.4GB  |
         * +----------------+----------+------------+-------------+--------+
         * 
         * The  MSDOS "logical CHS" (LCHS) format combined with the 
         * Physical IDE/ATA CHS interface (PCHS) gives MSDOS the 
         * following addressing limitation:
         * 
         *                   +------------+----------+----------------+   
         *                   | BIOS INT13 | IDE/ATA  | Combined Limit |
         * +-----------------+------------+----------+----------------+   
         * |Max Sectors/Track|  63        |    255   |     63         |
         * +-----------------+------------+----------+----------------+
         * |Max Heads        |  256       |     16   |     16         |
         * +-----------------+------------+----------+----------------+
         * |Max Cylinders    |  1024      |  65536   |    1024        |
         * +-----------------+------------+----------+----------------+
         * |Capacity         |  8.4GB     |  136.9GB |   528 MB       |
         * +-----------------+------------+----------+----------------+
         * 
         * 1024 x 16 x 63 X 512 bytes/sector = 528.48 MB limit.
         * 
         * Cylinder bits : 10 = 1024
         * Head bits     :  4 = 16
         * Sector bits   :  6 (-1) = 63 
         * (by convention, CHS sector numbering begins at
         *  #1, not #0; LBA begins at zero, which is CHS#1) 
         * To overcome the limitation. PC BIOS/OS vendors perform 
         * what is often referred to as geometric or "drive" CHS 
         * translation to address more than 528MB, and to "address" 
         * cylinders above 1024. So we just attempt to mimic here. 
         */

	if (((block_t)(cbioParams.nHeads) *
	     (block_t)(cbioParams.blocksPerTrack)) != (block_t) 0) 
	    {
            work = (block_t)(((block_t)(cbioParams.blockOffset) + 
                              (block_t)(cbioParams.nBlocks)) / 
                             ((block_t)(cbioParams.nHeads) * 
                              (block_t)(cbioParams.blocksPerTrack)));

            while ( (work > (block_t) 1023) && 
                    (((block_t)(cbioParams.nHeads) << (block_t) 2) < 
                     (block_t) 256) )
                {
	        cbioParams.nHeads <<= (block_t) 2;
                work >>= (block_t) 2;
	        }
	    }   

	/* values coming from driver, better not trust them if preserving */

	if( pConf->secPerTrack == 0 )
	    pConf->secPerTrack = cbioParams.blocksPerTrack ;
	if( pConf->nHeads == 0 )
	    pConf->nHeads = cbioParams.nHeads ;
	VX_TO_DISK_16( pConf->secPerTrack, pBoot + DOS_BOOT_SEC_PER_TRACK);
	VX_TO_DISK_16( pConf->nHeads, pBoot + DOS_BOOT_NHEADS);
	}

    VX_TO_DISK_16( pConf->nResrvd, pBoot + DOS_BOOT_NRESRVD_SECS );
    VX_TO_DISK_16( pConf->maxRootEnts, pBoot + DOS_BOOT_MAX_ROOT_ENTS );
    VX_TO_DISK_16( bytesPerBlk, pBoot + DOS_BOOT_BYTES_PER_SEC);


    /* total sector count */
    if( (cbioParams.nBlocks & ~0xffff ) == 0 )
	{
	VX_TO_DISK_16( cbioParams.nBlocks, 
			pBoot + DOS_BOOT_NSECTORS);
	}
    else
	{
	if( bytesPerBlk < 64 )	/* can not do with 32 byte sectors */
	    goto _done_;
	VX_TO_DISK_16( 0, pBoot + DOS_BOOT_NSECTORS);
	VX_TO_DISK_32( cbioParams.nBlocks, 
			pBoot + DOS_BOOT_LONG_NSECTORS);
	}

    if( pConf->nResrvd >= 32 )
	backupBootSec = 6 ;
    else if( pConf->nResrvd  >= 2 )
	backupBootSec = pConf->nResrvd -1 ;

    if( pConf->fatType == _FAT32 )
	{
	pBoot [DOS32_BOOT_SIGNATURE] 		= DOS_EXT_BOOT_SIG;
	VX_TO_DISK_16( 0, 		  pBoot + DOS_BOOT_SEC_PER_FAT );
	VX_TO_DISK_32( pConf->secPerFat,  pBoot + DOS32_BOOT_SEC_PER_FAT );
	VX_TO_DISK_16( 0, 		  pBoot + DOS32_BOOT_EXT_FLAGS );
	VX_TO_DISK_16( 0 /* XXX-? */,	  pBoot + DOS32_BOOT_FS_VERSION );
	VX_TO_DISK_32( 2 /* MUST */, 	  pBoot + DOS32_BOOT_ROOT_START_CLUST);
	VX_TO_DISK_16( 1 /* MUST */,	  pBoot + DOS32_BOOT_FS_INFO_SEC );
	VX_TO_DISK_16( backupBootSec,	  pBoot + DOS32_BOOT_BOOT_BKUP );
	VX_TO_DISK_32( pConf->volSerial,  pBoot + DOS32_BOOT_VOL_ID);

	bcopy( pConf->volLabel,	 (char *) pBoot + DOS32_BOOT_VOL_LABEL,
		DOS_VOL_LABEL_LEN );

	bcopy( DOS_BOOT_SYSID_FAT32, (char *)pBoot + DOS32_BOOT_FS_TYPE,
		DOS_SYS_ID_LEN ); /* "FAT32   " */

	if(FALSE == preserve)
	    {
	    pBoot [ DOS32_BOOT_BIOS_DRV_NUM ] = 0x80; /* assume hard */
	    }
	}
    else /* FAT 16 or FAT 12 */
	{
	/* from offset 0x24 in FAT12/16 have other meaning in FAT32 */
		    /* indicate extended boot sector fields in use */
	pBoot [DOS_BOOT_SIG_REC] 		= DOS_EXT_BOOT_SIG;

	VX_TO_DISK_32( pConf->nHidden, pBoot + DOS_BOOT_NHIDDEN_SECS);
	VX_TO_DISK_32( pConf->volSerial, pBoot + DOS_BOOT_VOL_ID);
	VX_TO_DISK_16( pConf->secPerFat, pBoot + DOS_BOOT_SEC_PER_FAT );

	bcopy( pConf->volLabel, (char *) pBoot + DOS_BOOT_VOL_LABEL,
		DOS_VOL_LABEL_LEN );

	/* Add file system type to offset 0x36, later DOS versions do this */
	/* FAT12 */
	if (pConf->fatType == _FAT12)
	    {
	    bcopy (DOS_BOOT_FSTYPE_FAT12, (char *) pBoot + DOS_BOOT_FSTYPE_ID,
		   DOS_BOOT_FSTYPE_LEN);
	    }

	/* FAT16 */
	if (pConf->fatType == _FAT16)
	    {
	    bcopy (DOS_BOOT_FSTYPE_FAT16, (char *) pBoot + DOS_BOOT_FSTYPE_ID, 
		   DOS_BOOT_FSTYPE_LEN);
	    }

	if(FALSE == preserve)
	    {
	    /* 
	     * jkf - Below is a hack. When writing this value out, 
	     * we really should  be writing 0xN for floppies where 
	     * N=0 for drive 0, 1 for drive 1, and 2 for drive 2, etc.  
	     * For hard disks we should be writing out 0x8N.
	     */
	    pBoot [ DOS_BOOT_DRIVE_NUM ] = 
		(pConf->fatType == _FAT12) ? 0x00: 0x80 ; /* floppy or hard */
	    }
	}

    /* write the backup boot block */

    if( backupBootSec != NONE )
	if( cbioBlkRW( cbioDev, backupBootSec , 1, (addr_t)pBoot, 
		CBIO_WRITE, NULL ) == ERROR )
	    goto _done_;

    /* now write the real boot block */
    if( cbioBlkRW( cbioDev, DOS_BOOT_SEC_NUM , 1, 
			(addr_t)pBoot, CBIO_WRITE, NULL ) == ERROR )
	goto _done_;

    /* Done */
    stat = OK ;
_done_:
    if( pBoot != bootBlockBuffer )
	KHEAP_FREE(pBoot);
    return( stat  );
    }
/*******************************************************************************
* 
* dosFsVolFormat - format an MS-DOS compatible volume
*
* This utility routine performs the initialization of file system data
* structures on a disk. It supports FAT12 for small disks, FAT16 for
* medium size and FAT32 for large volumes.
* The <device> argument may be either a device name known to the I/O
* system, or a dosFsLib Volume descriptor or a CBIO device handle.
*
* The <opt> argument is a bit-wise or'ed combination of options controlling the
* operation of this routine as follows:
*
* .IP DOS_OPT_DEFAULT	
* If the current volume boot block is reasonably intact, use existing
* parameters, else calculate parameters based only on disk size, possibly
* reusing only the volume label and serial number.
* 
* .IP DOS_OPT_PRESERVE
* Attempt to preserve the current volume parameters even if they seem to
* be somewhat unreliable.
* 
* .IP DOS_OPT_BLANK	
* Disregard the current volume parameters, and calculate new parameters
* based only on disk size.
*
* .IP DOS_OPT_QUIET
* Do not produce any diagnostic output during formatting.
*
* .IP DOS_OPT_FAT16
* Format the volume with FAT16 format even if the disk is larger then 
* 2 Gbytes, which would normally be formatted with FAT32.
*
* .IP DOS_OPT_FAT32
* Format the volume with FAT32, even if the disk is smaller then 
* 2 Gbytes, but is larger then 512 Mbytes.
*
* .IP DOS_OPT_VXLONGNAMES
* Format the volume to use Wind River proprietary case-sensitive Long
* File Names. Note that this format is incompatible with any other
* implementation of the MS-DOS file system.
*
* .LP
* The third argument, <pPromptFunc> is an optional pointer to a function
* that may interactively prompt the user to change any of the
* modifiable volume parameters before formatting:
*
* .CS
* void formatPromptFunc( DOS_VOL_CONFIG *pConfig );
* .CE
*
* The <*pConfig> structure upon entry to formatPromptFunc() will contain
* the initial volume parameters, some of which can be changed before it
* returns. <pPromptFunc> should be NULL if no interactive prompting is
* required.
*
* COMPATIBILITY
*
* Although this routine tries to format the disk to be
* compatibile with Microsoft implementations of the FAT and FAT32
* file systems, there may be differences which are not under WRS
* control.  For this reason, it is highly recommended that any
* disks which are expected to be interchanged between vxWorks and
* Windows should be formatted under Windows to provide the best
* interchangeability.  The WRS implementation is more flexible,
* and should be able to handle the differences when formatting is
* done on Windows, but Windows implementations may not be able to
* handle minor differences between their implementation and ours.
*
* AVAILABILITY
* This function is an optional part of the MS-DOS file system,
* and may be included in a target system if it is required to
* be able to format new volumes.
*
* RETURNS: OK or ERROR if was unable to format the disk.
*/
STATUS dosFsVolFormat
    (
    void * 	device,		/* device name or volume or CBIO pointer */
    int		opt,		/* bit-wise or'ed options */
    FUNCPTR	pPromptFunc	/* interactive parameter change callback */
    )
    {
    DOS_VOL_CONFIG cfg, cfgOld;
    CBIO_DEV_ID	   cbioDev = NULL;
    STATUS         stat    = ERROR;
    char *         pTail;
    BOOL           preserve;
    CBIO_PARAMS    cbioParams;

    /* 
     * Determine what type of device handle we were passed,
     * and extract the CBIO pointer.
     */

    /* NULL is presumed to be an invalid device */

    if (NULL == device)
        {
        errno = S_cbioLib_INVALID_CBIO_DEV_ID;
        return (ERROR);
        }

    /*
     * check for 'device' being a valid DOS_VOLUME_DESC_ID or a CBIO_DEV_ID
     * SPR#71720 : Ensure the pointer is aligned before attempting to access
     * any structure member from it.  Avoid exception on SH arch w/DIAB.
     * cbioDevVerify() will do an alignment check internally. The check
     * "((DOS_VOLUME_DESC_ID)device)->magic" needs _WRS_ALIGN_CHECK().
     */

    if (OK == cbioDevVerify ((CBIO_DEV_ID) device))
        {
        /* 'device' is a valid CBIO_DEV_ID */

        cbioDev = device;
        }
    else if (TRUE == _WRS_ALIGN_CHECK(device, DOS_VOLUME_DESC))
        {
        if (DOS_FS_MAGIC == ((DOS_VOLUME_DESC_ID)device)->magic)
            {
            /*
             * 'device' a valid DOS_VOLUME_DESC_ID, use its cbio ptr
             * Note that it will be cbioDevVerify()'d again below.
             */

            cbioDev = ((DOS_VOLUME_DESC_ID)device)->pCbio;
            }
        }

    if (NULL == cbioDev)
        {
        /*
         * didn't find anything valid above. Last try an iosDevFind()
         * to see if 'device' is a character string, an io device name.
         */

#ifdef _WRS_DOSFS2_VXWORKS_AE

        cbioDev = (CBIO_DEV_ID) iosDevFind((char *) device, 
                                           (const char **)&pTail );

#else

        cbioDev = (CBIO_DEV_ID) iosDevFind((char *) device, 
                                           (char **)&pTail );

#endif /* _WRS_DOSFS2_VXWORKS_AE */

        if( (NULL == cbioDev) || (pTail == device) )
            {
            /* 'device' appears invalid, so we bail out */
            errno = S_cbioLib_INVALID_CBIO_DEV_ID;
            return (ERROR);
            }

        if (TRUE == _WRS_ALIGN_CHECK(cbioDev, DOS_VOLUME_DESC))
            {
            if( ((DOS_VOLUME_DESC_ID)cbioDev)->magic != DOS_FS_MAGIC )
                {
                /* 'device' appears invalid, so we bail out */

                errno = S_cbioLib_INVALID_CBIO_DEV_ID;
                return (ERROR);
                }
            else
                {
                /* 
                 * 'cbioDev' is now a valid DOS_VOLUME_DESC_ID, use its CBIO
                 * PTR. Note that it will be cbioDevVerify()'d again below.
                 */

	        cbioDev = ((DOS_VOLUME_DESC_ID)cbioDev)->pCbio;
                }
            }
        }


    /* Again, verify that we have a valid CBIO handle */

    if( ERROR == cbioDevVerify(cbioDev))
	{
	return (ERROR);
	}
    else
	{
        /* 
         * SPR#71633: cbioDevVerify() may have set errno 
         * above, but if we made it here, then it doesn't 
         * matter to the user, so we clear any errno.
         */
        errno = 0;
	}

    /* clear the DOS_VOL_CONFIG structures */

    bzero( (caddr_t) &cfg, sizeof(cfg));
    bzero( (caddr_t) &cfgOld, sizeof(cfgOld));

    /* take ownership of the device */

    if( ERROR == cbioLock (cbioDev, WAIT_FOREVER))
	return ERROR;

    /* RESET the CBIO device */

    if( cbioIoctl( cbioDev, CBIO_RESET, 0) != OK )
	goto _done_;

    /* Ensure we can write to this CBIO device */

    if( O_RDONLY == cbioModeGet(cbioDev))
	{
	errno = S_dosFsLib_READ_ONLY ;
	goto _done_;
	}

    /* Get the underlying CBIO device parameters */

    if (ERROR == cbioParamsGet (cbioDev, &cbioParams))
	{
        stat = ERROR;
	goto _done_;
	}

    /* set hidden to sectors per track for hard disk */

    /*  
     * jkf The below is a hack. The boot sectors nHidden field should be 
     * set to the same value as the Relative Sector field in the MS 
     * partition table that defines the location of this volume.  This 
     * is not being handled here or in usrFdiskPartCreate.  Somehow 
     * usrFdiskPartLib.c and dpartCbio.c, should account for this someday.  
     * The value is also typically equal to the sectors per track value.  
     * Don't like the below code at all, it assumes too much.
     */

    if (36 <  cbioParams.blocksPerTrack) /* 36=2.88 floppy*/
	{
	if ((block_t)cbioParams.blockOffset <= 
            (block_t)cbioParams.blocksPerTrack) 
	    {
            cfgOld.nHidden = cbioParams.blockOffset;
            cfg.nHidden = cbioParams.blockOffset;
	    }
	else
	    {
            cfgOld.nHidden = cbioParams.blocksPerTrack;
            cfg.nHidden = cbioParams.blocksPerTrack;
	    }
	}
    else
	{
        cfgOld.nHidden = 0;
        cfg.nHidden = 0;
	}

    /* read the existing boot block from the disk. */

    stat = dosFsFmtReadBootBlock( cbioDev, &cfgOld );

    if((opt & DOS_OPT_QUIET) == 0)
	{
	printf("Retrieved old volume params with %%%d confidence:\n", stat );
	dosFsFmtShow(&cfgOld);
	}

   switch (opt & (DOS_OPT_BLANK | DOS_OPT_PRESERVE))
	{
	default:
	    /* preserve if well structured boot block uncovered */
	    if( stat < 90 )
		{
		cfg.volSerial = cfgOld.volSerial ;
		bcopy(cfgOld.volLabel, cfg.volLabel, sizeof(cfg.volLabel)) ;
		preserve = FALSE ;
		}
	    else
		{
		cfg = cfgOld ;
		preserve = TRUE ;
		}
	    break ;
	case DOS_OPT_PRESERVE: /* preserve if not totally corrupt */
	    if( stat < 70 )
		{
		cfg.volSerial = cfgOld.volSerial ;
		bcopy(cfgOld.volLabel, cfg.volLabel, sizeof(cfg.volLabel)) ;
		preserve = FALSE ;
		}
	    else
		{
		cfg = cfgOld ;
		preserve = TRUE ;
		}
	    break ;
	case DOS_OPT_BLANK: /* never preserve */
	    cfg.volSerial = cfgOld.volSerial ;
	    bcopy(cfg.volLabel, cfgOld.volLabel, sizeof(cfgOld.volLabel)) ;
	    preserve = FALSE ;
	    break ;
	    
	}

    /* forcing FAT32 or FAT16 */

    if( opt & DOS_OPT_FAT32 )
	cfg.fatType = _FAT32 ;
    else if( opt & DOS_OPT_FAT16 )
	cfg.fatType = _FAT16 ;

    /* calculate the BPB */
    stat = dosFsFmtAutoParams( &cfg,
	cbioParams.nBlocks, cbioParams.bytesPerBlk, opt );

    if( pPromptFunc != NULL )
	{
	/* interactive prompting */
	(void) (*pPromptFunc)( &cfg );
	stat = dosFsFmtAutoParams( &cfg,
	    cbioParams.nBlocks, cbioParams.bytesPerBlk, opt );
	}

    if((opt & DOS_OPT_QUIET) == 0)
	{
	printf("Disk with %ld sectors of %d bytes will be formatted with:\n",
	    cbioParams.nBlocks, (int) cbioParams.bytesPerBlk );
	dosFsFmtShow(&cfg);
	}

    if(stat != OK)
	{
	printErr("dosFsVolFormat: bad format parameters, errno = %#x\n",
		errno );
	goto _done_;
	}

    stat = dosFsFmtVolInit( cbioDev, &cfg, preserve, opt );

    /* flush and invalidate cache immediately */
    stat |= cbioIoctl( cbioDev, CBIO_CACHE_INVAL, 0) ;

    if(stat != OK)
	{
	printErr("dosFsVolFormat: format failed, errno=%#x\n", errno );
	}

    cbioRdyChgdSet (cbioDev, TRUE);	/* force re-mount */

_done_:
    cbioUnlock (cbioDev) ;
    return stat ;
    }

/*******************************************************************************
*
* dosFsFmtLibInit - initialize the MS-DOS formatting library
*
* This function is called to optionally enable the formatting
* functionality from dosFsLib.
*
* SEE ALSO: dosFsLib
*
* NOMANUAL
*/
void dosFsFmtLibInit( void )
    {
    dosFsVolFormatRtn = dosFsVolFormat;
    }


#ifdef	__unused__ 
/*******************************************************************************
*
* dosFsFmtTest - UNITEST CODE
*
* NOMANUAL
*/
void dosFsFmtTest( int size)
    {
    DOS_VOL_CONFIG cfg;
    ULONG i, k ;
    STATUS stat ;

    if( size != 0 )
	{
	i = size ;
	bzero((caddr_t) &cfg, sizeof(cfg));
	stat = dosFsFmtAutoParams( &cfg, i, 512, 0) ;
	dosFsFmtShow( &cfg );
	if( stat == ERROR )
	    perror("dosFsFmtAutoParams");
	return;
	}

    i = 100 ;

    for(k=0; k< 1000; k++ )
	{
	bzero((caddr_t) &cfg, sizeof(cfg));
	i += rand() ;
	stat = dosFsFmtAutoParams( &cfg, i, 512, 0) ;
	dosFsFmtShow( &cfg );
	if( stat == ERROR )
	    {
	    perror("dosFsFmtAutoParams");
	    return;
	    }
	}
    }

#endif	/* __unused__ */
