/*
 * $Log:   V:/ssfdc.c_v  $
 *
 *    Rev 1.17   01 Mar 1998 12:59:30   amirban
 * Add parameter to mapSector
 *
 *    Rev 1.16   29 Sep 1997 14:17:28   danig
 * Call setBusy(OFF) from dismount function
 *
 *    Rev 1.15   18 Sep 1997 10:11:42   danig
 * Warnings
 *
 *    Rev 1.14   10 Sep 1997 16:21:16   danig
 * Got rid of generic names
 *
 *    Rev 1.13   10 Sep 1997 13:32:20   danig
 * Limit number of virtual units
 *
 *    Rev 1.12   04 Sep 1997 15:52:46   danig
 * Debug messages
 *
 *    Rev 1.11   04 Sep 1997 11:07:52   danig
 * Share buffer with MTD
 *
 *    Rev 1.10   28 Jul 1997 15:13:34   danig
 * volForCallback & moved standard typedefs to flbase.h
 *
 *    Rev 1.9   27 Jul 1997 14:04:18   danig
 * FAR -> FAR0
 *
 *    Rev 1.8   24 Jul 1997 15:14:18   danig
 * Fixed ECC on read
 *
 *    Rev 1.7   20 Jul 1997 11:01:52   danig
 * Ver 2.0
 *
 *    Rev 1.6   13 Jul 1997 10:51:58   danig
 * erasedSectors
 *
 *    Rev 1.5   01 Jul 1997 15:36:32   danig
 * Changes to isErasedSector & isErasedUnit
 *
 *    Rev 1.4   29 Jun 1997 16:13:02   danig
 * Comments
 *
 *    Rev 1.3   12 Jun 1997 17:41:54   danig
 * ECC on read, use second address area.
 *
 *    Rev 1.2   08 Jun 1997 19:05:24   danig
 * Delete sector bug & check for mount in setBusy()
 *
 *    Rev 1.1   01 Jun 1997 13:42:36   amirban
 * Use ftllite.h I/F
 *
 *    Rev 1.0   15 Apr 1997 17:56:26   danig
 * Initial revision.
 */

/************************************************************************/
/*                                                                      */
/*		FAT-FTL Lite Software Development Kit			*/
/*		Copyright (C) M-Systems Ltd. 1995-1997			*/
/*									*/
/************************************************************************/


#include "fltl.h"
#include "flflash.h"
#include "flbuffer.h"

/* VF: This macro is vital to supporting devices with 32 or more sectors per erase
       unit.  Data types and code coming originally from M-Systems only provided
       for up to 16 sectors per erase unit.  Some data types had bit fields that
       depended on the number of sectors per erase unit not exceeding 16.
       Several data type and code modifications have been made based on the value
       of this macro to accomodate larger erase unit sized devices.
*/
#define TFFS_SECTORS_PER_UNIT  32

typedef long int VirtualAddress;

/* VF: Updated PhysUnit storage type.  As used below in the Unit type,
       a PhysUnit only provided 8 bits of storage:  The upper 3 bits were
       used to bookkeep sector status, and the low 5 bits represented a count
       of sectors free in the unit.  5 bits only counts up to 31.  We need more
       bits for the count, hence the increase in size of this data type.
*/
#if (TFFS_SECTORS_PER_UNIT < 32)
typedef unsigned char PhysUnit;
#else
typedef unsigned short PhysUnit;
#endif

typedef unsigned short UnitNo;

#define FORMAT_PATTERN 1, 3, 0xd9, 1, 0xff, 0x18, 2, 0xdf, 1, 0x20

#define CIS_DATA 0x1, 0x3, 0xd9, 0x1, 0xff, 0x18, 0x2, 0xdf, 0x1, \
0x20, 0x4, 0x0, 0x0, 0x0, 0x0, 0x21, 0x2, \
0x4, 0x1, 0x22, 0x2, 0x1, 0x1, 0x22, 0x3, \
0x2, 0x4, 0x7, 0x1a, 0x5, 0x1, 0x3, 0x0, \
0x2, 0xf, 0x1b, 0x8, 0xc0, 0xc0, 0xa1, 0x1, \
0x55, 0x8, 0x0, 0x20, 0x1b, 0xa, 0xc1, 0x41, \
0x99, 0x1, 0x55, 0x64, 0xf0, 0xff, 0xff, 0x20, \
0x1b, 0xc, 0x82, 0x41, 0x18, 0xea, 0x61, 0xf0, \
0x1, 0x7, 0xf6, 0x3, 0x1, 0xee, 0x1b, 0xc, \
0x83, 0x41, 0x18, 0xea, 0x61, 0x70, 0x1, 0x7, \
0x76, 0x3, 0x1, 0xee, 0x15, 0x14, 0x5, 0x0, \
0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x0, \
0x20, 0x20, 0x20, 0x20, 0x0, 0x30, 0x2e, 0x30, \
0x0, 0xff, 0x14, 0x0, 0xff, 0x0, 0x0, 0x0, \
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, \
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, \
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, \
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, \
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, \
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, \
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, \
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, \
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, \
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, \
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, \
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, \
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, \
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, \
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, \
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, \
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, \
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1, \
0x3, 0xd9, 0x1, 0xff, 0x18, 0x2, 0xdf, 0x1, \
0x20, 0x4, 0x0, 0x0, 0x0, 0x0, 0x21, 0x2, \
0x4, 0x1, 0x22, 0x2, 0x1, 0x1, 0x22, 0x3, \
0x2, 0x4, 0x7, 0x1a, 0x5, 0x1, 0x3, 0x0, \
0x2, 0xf, 0x1b, 0x8, 0xc0, 0xc0, 0xa1, 0x1, \
0x55, 0x8, 0x0, 0x20, 0x1b, 0xa, 0xc1, 0x41, \
0x99, 0x1, 0x55, 0x64, 0xf0, 0xff, 0xff, 0x20, \
0x1b, 0xc, 0x82, 0x41, 0x18, 0xea, 0x61, 0xf0, \
0x1, 0x7, 0xf6, 0x3, 0x1, 0xee, 0x1b, 0xc, \
0x83, 0x41, 0x18, 0xea, 0x61, 0x70, 0x1, 0x7, \
0x76, 0x3, 0x1, 0xee, 0x15, 0x14, 0x5, 0x0, \
0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x0, \
0x20, 0x20, 0x20, 0x20, 0x0, 0x30, 0x2e, 0x30, \
0x0, 0xff, 0x14, 0x0, 0xff, 0x0, 0x0, 0x0, \
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, \
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, \
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, \
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, \
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, \
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, \
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, \
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, \
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, \
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, \
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, \
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, \
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, \
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, \
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, \
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, \
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, \
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0

#define SECTOR_ERASED		1
#define SECTOR_ALLOCATED        2
#define SECTOR_REPLACED		3
#define SECTOR_DELETED1		0x80
#define SECTOR_DELETED2		0xf0

#define SECTORS_PER_UNIT 16  /* default number of sectors per unit */

/*#define ECC_ON_READ*/

#define UNASSIGNED_ADDRESS 0xffffffffl

/* VF: Due to the increase in size of PhysUnit above from 8 to 16 bits, these macros
       must be adjusted accordingly to represent the upper 3 bits of a 16-bit quantity.
*/
#if (TFFS_SECTORS_PER_UNIT < 32)
#define UNIT_FREE	0xe0   /* unassigned unit */
#define	UNIT_AVAILABLE	0x80   /* assigned unit */
#define UNIT_ERASED	0xc0   /* unit is erased */
#define UNIT_BAD	0      /* unit is defective */
#define STATUS_MASK	0xe0   /* the unit status is in the 3 MSBs */
#else
#define UNIT_FREE	0xe000   /* unassigned unit */
#define	UNIT_AVAILABLE	0x8000   /* assigned unit */
#define UNIT_ERASED	0xc000   /* unit is erased */
#define UNIT_BAD	0x0000   /* unit is defective */
#define STATUS_MASK	0xe000   /* the unit status is in the 3 MSBs */
#endif


/* extra area of odd pages is read before extra area of even pages
   when page size is 256 */
#define STATUS_AREA_OFFSET      ((vol.flash.flags & BIG_PAGE) ? 4 : 12)
#define BLOCK_ADDRESS_OFFSET1   ((vol.flash.flags & BIG_PAGE) ? 6 : 14)
#define BLOCK_ADDRESS_OFFSET2   ((vol.flash.flags & BIG_PAGE) ? 11 : 3)
#define CIS_DATA_OFFSET		((vol.flash.flags & BIG_PAGE) ? 4 : 12)
#define ECC1			((vol.flash.flags & BIG_PAGE) ? 13 : 5)
#define ECC2			((vol.flash.flags & BIG_PAGE) ? 8 : 0)

#define OFFSET1 1
#define OFFSET2 2

#define INCORRECT_DATA	0
#define CORRECT_DATA	0xff

#define NO_UNIT 0xffff


typedef struct {
  PhysUnit unitStatus;             /* unit status in the 3 MSBs, unassigned */
				   /* sectors counter in the rest of the bits. */
/* VF: Updated next line.  This is a bit-field, so we need 32 bits to represent sectors: */
#if (TFFS_SECTORS_PER_UNIT < 32)
  unsigned short erasedSectors;    /* if bit i=1, sector i in the unit is erased. */
#else
  unsigned long erasedSectors;     /* if bit i=1, sector i in the unit is erased. */
#endif
} Unit;


typedef struct  {
  unsigned char sectorStatus;
  unsigned char unitStatus;
} StatusArea;


typedef enum {BAD_FORMAT, SSFDC_FORMAT, ANAND_FORMAT} FormatType;

#ifndef MALLOC_TFFS

#define HEAP_SIZE	(0x100000l / ASSUMED_NFTL_UNIT_SIZE) *       \
			(sizeof(UnitNo) + sizeof(Unit)) *  	\
			MAX_VOLUME_MBYTES + SECTORS_PER_UNIT

#endif

struct tTLrec {
  FLBoolean	    badFormat;		/* true if TFFS format is bad */

  UnitNo	    CISblock;           /* Unit no. of CIS block */
  unsigned int	    erasableBlockSizeBits;  /* log2 of erasable block size */
  UnitNo	    noOfVirtualUnits;
  UnitNo	    freeUnits;              /* number of unassigned units */
  unsigned long     unitOffsetMask;
  unsigned int	    sectorsPerUnit;
  UnitNo	    noOfUnits;
  unsigned int	    unitSizeBits;
  SectorNo	    virtualSectors;

  UnitNo            roverUnit;          /* Starting point for allocation search */
  UnitNo	    transferUnit;
  UnitNo	    replacedUnit;
  unsigned int	    sectorsDeleted;     /* number of sector deleted in the
					   replaced unit */
  Unit 		    *physicalUnits; 	/* unit table by physical no. */
  UnitNo	    *virtualUnits; 	/* unit table by logical no. */
  unsigned char	    *replacementSectors; /* a table of the sectors in the
					    replaced unit */
  SectorNo 	    mappedSectorNo;
  void FAR0         *mappedSector;
  CardAddress	    mappedSectorAddress;

  FLFlash	    flash;
  FLBuffer          *buffer;

#ifndef MALLOC_TFFS
  char		    heap[HEAP_SIZE];
#endif
};

#define ssfdcBuffer  vol.buffer->data

typedef TLrec SSFDC;

static SSFDC vols[DRIVES];


/********************** ECC\EDC part ******************************/

/*----------------------------------------------------------------------*/
/*		         c h a r P a r i t y				*/
/*									*/
/* Get odd parity of one byte.						*/
/*                                                                      */
/* Parameters:                                                          */
/*	c		: One byte of data.				*/
/*                                                                      */
/* Returns:                                                             */
/*	Odd parity.							*/
/*----------------------------------------------------------------------*/

static unsigned char charParity(unsigned char c)
{
  unsigned char parityBit = 1;

  for (; c; c >>= 1)
    parityBit ^= c & 0x01;

  return parityBit;
}


/*----------------------------------------------------------------------*/
/*		         c r e a t e E c c				*/
/*									*/
/* Calculate 22 bits Error correction \ detection code for 256 bytes    */
/* of data. 16 bits line parity, 6 bits column parity. ECC scheme 	*/
/* according to SSFDC specification.					*/
/*                                                                      */
/* Parameters:                                                          */
/*	buf		: 256 bytes of data.				*/
/*	code		: 3 bytes of ECC.				*/
/*                                                                      */
/*----------------------------------------------------------------------*/

static void createEcc(unsigned char FAR0 *buf, unsigned char FAR0 *code)
{
  unsigned char lineParity[16], columnParity[6], temp;
  int i, k;

  /* initialize to 1's, odd parity */
  for (i = 0; i < 16; i++)
    lineParity[i] = 0xff;
  for (i = 0; i < 6; i++)
    columnParity[i] = 1;

  /* xor the lines */
  for (i = 0; i < 256; i++)
    for (k = 0; k < 8; k++)
      if ((i / (1 << k)) % 2)
	lineParity[2 * k + 1] ^= buf[i];
      else
	lineParity[2 * k] ^= buf[i];

  temp = lineParity[0] ^ lineParity[1];  /* temp holds the xor of all the lines */

  for(i = 0; i < 3; i++)
    code[i] = 0;

  /* get line parity */
  for(i = 0; i < 16; i++)
    if (charParity(lineParity[i]))
      code[i / 8] |= (1 << (i % 8));

  /* xor the columns */
  for(i = 0; i < 8; i++)
    for (k = 0; k < 3; k++)
      if ((i / (1 << k)) % 2)
	columnParity[2 * k + 1] ^= (temp & (0xff >> (7 -  i))) >> i;
      else
	columnParity[2 * k] ^= (temp & (0xff >> (7 -  i))) >> i;

  code[2] |= 0x03;  /* bits 16 and 17 are always 1 */

  /* get column parity */
  for(i = 0; i < 6; i++)
    if (columnParity[i])
      code[2] |= 4 << i;
}

#ifdef ECC_ON_READ

/*----------------------------------------------------------------------*/
/*		         i s F i x a b l e				*/
/*									*/
/* We know there is an error, Check if it is fixable (one bit error in  */
/* 256 bytes of data is fixable).					*/
/*                                                                      */
/* Parameters:                                                          */
/*	diff		: The difference between the read and 		*/
/*			  calculated ECC.				*/
/*									*/
/* Returns:								*/
/*	TRUE if the error is fixable, otherwise FALSE.			*/
/*                                                                      */
/*----------------------------------------------------------------------*/

static FLBoolean isFixable (unsigned char FAR0 *diff)
{
  int i, j;
  unsigned char mask;

  /* data is fixable if for each pair of bits one is 0 and one is 1 */
  for (i = 0; i < 3; i++)
    for (j = 0, mask = 1; j < 4; j++, mask <<= 2) {
      if ((i == 2) && (j == 0))
	continue;     /* the two lowest bits in the 3rd byte are always 1 */
      /* xor two neighbouring bits */
      if (!(((diff[i] & mask) >> (2 * j)) ^
	    ((diff[i] & (mask << 1)) >> (2 * j + 1))))
	return FALSE;
    }

  return TRUE;
}


/*----------------------------------------------------------------------*/
/*		         c h e c k A n d F i x				*/
/*									*/
/* Compare the calculated ECC and the one read from the device. 	*/
/* If there is a difference, try to fix the error. If both codes	*/
/* are the same, data is correct.					*/
/*                                                                      */
/* Parameters:                                                          */
/*	ecc1		: Calculated ECC.				*/
/*	ecc2		: Read ECC.					*/
/*      sectorToFix	: The data to fix.				*/
/*									*/
/* Returns:								*/
/*	TRUE if data is correct or error fixed, otherwise FALSE.	*/
/*                                                                      */
/*----------------------------------------------------------------------*/

static FLBoolean checkAndFix(unsigned char FAR0 *ecc1, unsigned char FAR0 *ecc2,
			 unsigned char FAR0 *sectorToFix)
{
  if((ecc1[0] != ecc2[0]) ||
     (ecc1[1] != ecc2[1]) ||
     (ecc1[2] != ecc2[2])) {  /* ECC error, try to fix */
    unsigned char diff[3];
    int i;

    for (i = 0; i < 3; i++)
      diff[i] = ecc1[i] ^ ecc2[i];

    if (isFixable(diff)) {
      unsigned char line = 0, column = 0, mask;

      /* push the lower bits to the right */
      for (i = 0, mask = 2; i < 4; i++, mask <<= 2)
	line |= (diff[0] & mask) >> (i + 1);

      /* push the upper bits to the left */
      for (i = 0, mask = 0x80; i < 4; i++, mask >>= 2)
	line |= (diff[1] & mask) << i;

      /* push to the right */
      for (i = 0, mask = 8; i < 3; i++, mask <<= 2)
	column |= (diff[2] & mask) >> (i + 3);

      sectorToFix[line] ^= 1 << column;  /* fix */
      return TRUE;
    }
    else {
    #ifdef DEBUG_PRINT
      DEBUG_PRINT("Debug: ECC error in SSFDC.\n");
    #endif
      return FALSE;
    }
  }
  else
    return TRUE;
}


/*----------------------------------------------------------------------*/
/*		         e c c O n R e a d				*/
/*									*/
/* Use ECC\EDC for a sector of data. Do each half seperately.		*/
/* If there is a fixable error, fix it.					*/
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	sectorAddress	: Physical address of the sector to check.	*/
/*									*/
/* Returns:								*/
/* 	FALSE if there is an error that can't be fixed, otherwise 	*/
/*	return TRUE.							*/
/*                                                                      */
/*----------------------------------------------------------------------*/

static FLBoolean eccOnRead(SSFDC vol, CardAddress sectorAddress)
{
  unsigned char calcEcc[3], readEcc[3];
  unsigned char FAR0 *sectorToFix = (unsigned char FAR0 *)vol.mappedSector;

  createEcc(sectorToFix, calcEcc);
  vol.flash.read(&vol.flash, sectorAddress + ECC1, readEcc, sizeof readEcc, EXTRA);
  if (checkAndFix(calcEcc, readEcc, sectorToFix)) {
    /* success in the first half go for the second half */
    createEcc(sectorToFix + SECTOR_SIZE / 2, calcEcc);
    vol.flash.read(&vol.flash, sectorAddress + ECC2, readEcc, sizeof readEcc, EXTRA);
    if (checkAndFix(calcEcc, readEcc, sectorToFix + SECTOR_SIZE / 2))
      return TRUE;
  }

  return FALSE;
}

#endif /* ECC_ON_READ */

/******************* end of ECC\EDC part ********************************/

/*----------------------------------------------------------------------*/
/*		         u n i t B a s e A d d r e s s			*/
/*									*/
/* Returns the physical address of a unit.				*/
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	unitNo		: Physical unit number				*/
/*                                                                      */
/* Returns:                                                             */
/*	physical address of unitNo					*/
/*----------------------------------------------------------------------*/

static CardAddress unitBaseAddress(SSFDC vol, UnitNo unitNo)
{
  return (CardAddress)unitNo << vol.unitSizeBits;
}


/*----------------------------------------------------------------------*/
/*		         v i r t u a l 2 P h y s i c a l		*/
/*									*/
/* Translate virtual sector number to physical address.			*/
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	sectorNo	: Virtual sector number				*/
/*                                                                      */
/* Returns:                                                             */
/*	physical address of sectorNo					*/
/*----------------------------------------------------------------------*/

static CardAddress virtual2Physical(SSFDC vol, SectorNo sectorNo)
{
  unsigned unitOffset = (sectorNo % vol.sectorsPerUnit) << SECTOR_SIZE_BITS;
  UnitNo unitNo = vol.virtualUnits[sectorNo / vol.sectorsPerUnit];
  StatusArea statusArea;

  /* no physical unit is assigned to this virtual sector number */
  if (unitNo == NO_UNIT)
    return UNASSIGNED_ADDRESS;

  /* check if this sector was replaced */
  vol.flash.read(&vol.flash,
		 unitBaseAddress(&vol, unitNo) + unitOffset + STATUS_AREA_OFFSET,
		 &statusArea, sizeof statusArea, EXTRA);
  if (statusArea.sectorStatus != 0xff) {
    if (vol.replacedUnit == unitNo)
      if (vol.replacementSectors[sectorNo % vol.sectorsPerUnit] == SECTOR_REPLACED)
	return unitBaseAddress(&vol, vol.transferUnit) + unitOffset; /* this sector was replaced */
    return UNASSIGNED_ADDRESS;  /* this sector is bad or deleted */
  }

  return unitBaseAddress(&vol, unitNo) + unitOffset;
}


/*----------------------------------------------------------------------*/
/*		         p h y s i c a l 2 V i r t u a l		*/
/*									*/
/* Translate physical unit number to virtual unit number. Read virtual	*/
/* unit number from first or second address area according to the 	*/
/* parameter addressAreaNo.						*/
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	unitNo		: Physical unit number				*/
/*	addressAreaNo	: Specify which address area to use		*/
/*                                                                      */
/* Returns:                                                             */
/*	Virtual unit number of unitNo.					*/
/*----------------------------------------------------------------------*/

static UnitNo physical2Virtual(SSFDC vol, UnitNo unitNo, int addressAreaNo)
{
  unsigned char addressArea[2];
  UnitNo virtualUnitNo;
  int offset;

  offset = (addressAreaNo == OFFSET1 ? BLOCK_ADDRESS_OFFSET1 : BLOCK_ADDRESS_OFFSET2);

  /* get virtual unit no. from address area */
  vol.flash.read(&vol.flash, unitBaseAddress(&vol, unitNo) + offset, addressArea,
		 sizeof addressArea, EXTRA);

  /* trade places of byte 0 and byte 1 */
  virtualUnitNo = ((UnitNo)addressArea[0] << 8) | addressArea[1];
  /* virtual address is in bits 1 through 11 */
  virtualUnitNo <<= 4;
  virtualUnitNo >>= 5;

  return virtualUnitNo;
}


/*----------------------------------------------------------------------*/
/*      	        s e c t o r s I n V o l u m e			*/
/*									*/
/* Gets the total number of sectors in the volume			*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*                                                                      */
/* Returns:                                                             */
/*	Number of sectors in the volume					*/
/*----------------------------------------------------------------------*/

static SectorNo sectorsInVolume(SSFDC vol)
{
  return vol.virtualSectors;
}


/*----------------------------------------------------------------------*/
/*      	        I d e n t i f y F o r m a t			*/
/*									*/
/* There are two different formats for nand devices, this function 	*/
/* tries to identify one of them by reading its ID string. If format is */
/* identified, bootBlock holds the number of the unit where the boot 	*/
/* sector is. 								*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	bootBlock	: Receives the number of the block where the	*/
/*			  boot sector is.				*/
/*                                                                      */
/* Returns:                                                             */
/*	The type of the format idetified (SSFDC, NFTL, unknown)		*/
/*----------------------------------------------------------------------*/

static FormatType identifyFormat(SSFDC vol, UnitNo *bootBlock)
{
  int i;
  unsigned char bootRecordId[10], invalidDataFlag;
  unsigned char formatPattern[10] = {FORMAT_PATTERN};

  for (*bootBlock = 0; *bootBlock < vol.noOfUnits; (*bootBlock)++) {
    vol.flash.read(&vol.flash, unitBaseAddress(&vol, *bootBlock),
		   bootRecordId, sizeof bootRecordId, 0);
    if (tffscmp(bootRecordId, "ANAND", 6) == 0)
      return ANAND_FORMAT;
    for (i = 0; (unsigned)i < vol.sectorsPerUnit; i++) {
      vol.flash.read(&vol.flash, unitBaseAddress(&vol, *bootBlock) + (i << SECTOR_SIZE_BITS),
		     bootRecordId, sizeof bootRecordId, 0);
      if (tffscmp(bootRecordId, formatPattern, sizeof formatPattern) == 0) {
	/* check that the data is valid */
	vol.flash.read(&vol.flash,
		       unitBaseAddress(&vol, *bootBlock) +
		       (i << SECTOR_SIZE_BITS) + CIS_DATA_OFFSET,
		       &invalidDataFlag, sizeof invalidDataFlag, EXTRA);
	if (invalidDataFlag == 0xff)
	  return SSFDC_FORMAT;
      }
    }
  }

  return BAD_FORMAT;
}


/*----------------------------------------------------------------------*/
/*      	        i s E r a s e d s e c t o r 			*/
/*									*/
/* Check if a sector is erased.						*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/* 	unitNo		: The sector to check is in this unit.		*/
/*	offset		: Offset of the sector in the unit.		*/
/*                                                                      */
/* Returns:                                                             */
/*	TRUE if the sector is erased, otherwise FALSE			*/
/*----------------------------------------------------------------------*/

static FLBoolean isErasedSector(SSFDC vol, UnitNo unitNo, unsigned offset)
{
  unsigned char *buf;
  StatusArea statusArea;
  int i;

  buf = (unsigned char *)vol.flash.map(&vol.flash,
				       unitBaseAddress(&vol, unitNo) +
				       offset, SECTOR_SIZE);

  /* check data area */
  for (i = 0; i < SECTOR_SIZE; i++)
    if (buf[i] != 0xff)
      return FALSE;

  /* check status area */
  vol.flash.read(&vol.flash,
		 unitBaseAddress(&vol, unitNo) + offset + STATUS_AREA_OFFSET,
		 &statusArea, sizeof statusArea, EXTRA);
  if (statusArea.sectorStatus != 0xff)
    return FALSE;

  return TRUE;
}


/*----------------------------------------------------------------------*/
/*      	        w r i t e A n d C h e c k 			*/
/*									*/
/* Write one sector. If eccMode is TRUE, calculate and write ECC.	*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	address		: Physical address of the sector to write to	*/
/*	fromAddress	: Buffer of data to write			*/
/*	eccMode		: If TRUE, calculate and write ECC		*/
/*                                                                      */
/* Returns:                                                             */
/* 	FLStatus 	: 0 on success, failed otherwise.		*/
/*----------------------------------------------------------------------*/

static FLStatus writeAndCheck(SSFDC vol,
			      CardAddress address,
			      void FAR1 *fromAddress,
			      int eccMode)
{
  int sectorInUnit;
  FLStatus status = vol.flash.write(&vol.flash,address,fromAddress,SECTOR_SIZE,0);


  if (eccMode) {  /* calculate and write ECC */
    unsigned char ecc[3];

    createEcc((unsigned char FAR1 *)fromAddress, ecc);
    status = vol.flash.write(&vol.flash, address + ECC1, ecc, sizeof ecc, EXTRA);
    createEcc((unsigned char FAR1 *)fromAddress + SECTOR_SIZE / 2, ecc);
    status = vol.flash.write(&vol.flash, address + ECC2, ecc, sizeof ecc, EXTRA);
  }

  if (status == flWriteFault) {   /* write failed, sector doesn't hold valid data. */
    StatusArea statusArea = {0, 0xff};
    vol.flash.write(&vol.flash, address + STATUS_AREA_OFFSET, &statusArea,
		    sizeof statusArea,EXTRA);
  }

  /* mark sector as assigned and not erased */
  vol.physicalUnits[address >> vol.unitSizeBits].unitStatus--;
  sectorInUnit = (address % (1 << vol.unitSizeBits)) / SECTOR_SIZE;
  vol.physicalUnits[address >> vol.unitSizeBits].erasedSectors &= ~(1 << sectorInUnit);
  return status;
}


/*----------------------------------------------------------------------*/
/*      	             m a p S e c t o r				*/
/*									*/
/* Maps and returns location of a given sector no.			*/
/* NOTE: This function is used in place of a read-sector operation.	*/
/*									*/
/* A one-sector cache is maintained to save on map operations.		*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	sectorNo	: Sector no. to read				*/
/*	physAddress	: Optional pointer to receive sector address	*/
/*                                                                      */
/* Returns:                                                             */
/*	Pointer to physical sector location. NULL returned if sector	*/
/*	does not exist, or holds invalid data.				*/
/*----------------------------------------------------------------------*/

static const void FAR0 *mapSector(SSFDC vol, SectorNo sectorNo, CardAddress *physAddress)
{
  if (sectorNo != vol.mappedSectorNo || vol.flash.socket->remapped) {
    if (sectorNo >= vol.virtualSectors)
      vol.mappedSector = NULL;
    else {
      vol.mappedSectorAddress = virtual2Physical(&vol, sectorNo);

      if (vol.mappedSectorAddress == UNASSIGNED_ADDRESS)
	vol.mappedSector = NULL;	/* no such sector */
      else {
	vol.mappedSector = vol.flash.map(&vol.flash, vol.mappedSectorAddress, SECTOR_SIZE);
#ifdef ECC_ON_READ
	if (!eccOnRead(&vol, sectorAddress))  /* check and fix errors */
	  return NULL;			/* a non-fixable error  */
#endif
      }
    }
    vol.mappedSectorNo = sectorNo;
    vol.flash.socket->remapped = FALSE;
  }

  if (physAddress)
    *physAddress = vol.mappedSectorAddress;

  return vol.mappedSector;
}


/*----------------------------------------------------------------------*/
/*		          f o r m a t U n i t				*/
/*									*/
/* Format one unit. Erase the unit, and mark the physical units table.  */
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	unitNo		: Unit to format				*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus        : 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

static FLStatus formatUnit(SSFDC vol, UnitNo unitNo)
{
  if (!(vol.physicalUnits[unitNo].unitStatus & UNIT_AVAILABLE))
    return flWriteFault;

  if (((vol.physicalUnits[unitNo].unitStatus & STATUS_MASK) == UNIT_FREE) ||
      ((vol.physicalUnits[unitNo].unitStatus & STATUS_MASK) == UNIT_ERASED))
    vol.freeUnits--;

  vol.physicalUnits[unitNo].unitStatus &= ~UNIT_AVAILABLE; /* protect the unit */

  checkStatus(vol.flash.erase(&vol.flash,
			      unitNo << (vol.unitSizeBits - vol.erasableBlockSizeBits),
			      1 << (vol.unitSizeBits - vol.erasableBlockSizeBits)));

  /* mark unit erased */
  vol.physicalUnits[unitNo].unitStatus = UNIT_ERASED | vol.sectorsPerUnit;
/* VF: Updated next line: */
#if (TFFS_SECTORS_PER_UNIT < 32)  /* VF */
  vol.physicalUnits[unitNo].erasedSectors = 0xffff;  /* all sectors are erased */
#else
  vol.physicalUnits[unitNo].erasedSectors = 0xffffffff;  /* all sectors are erased */
#endif
  vol.freeUnits++;

  return flOK;
}


/*----------------------------------------------------------------------*/
/*		           a s s i g n U n i t				*/
/*									*/
/* Assigns a virtual unit no. to a unit					*/
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	unitNo		: Physical unit number				*/
/*	virtualUnitNo	: Virtual unit number to assign			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

static FLStatus assignUnit(SSFDC vol, UnitNo unitNo, UnitNo virtualUnitNo)
{
  unsigned char blockAddressArea[2], parityBit = 1; /* bit 12 is always 1 */
  FLStatus status;
  UnitNo un;
  unsigned sector;

  /* create the block address area */
  /* calculate parity bit */
  for (un = virtualUnitNo; un; un >>= 1)
    parityBit ^= un & 0x01;
  blockAddressArea[0] = (virtualUnitNo >> 7) | 0x10;
  blockAddressArea[1] = (virtualUnitNo << 1) | parityBit;


  /* write the block address to the block address areas in all the sectors
     of the unit.*/
  for (sector = 0; sector < vol.sectorsPerUnit; sector++) {
    CardAddress sectorAddress;

    sectorAddress = unitBaseAddress(&vol, unitNo) + (sector << SECTOR_SIZE_BITS);

    /* write to first address area */
    status = vol.flash.write(&vol.flash, sectorAddress + BLOCK_ADDRESS_OFFSET1,
			     &blockAddressArea, sizeof blockAddressArea, EXTRA);
    if (status == flOK)
      /* write to second address area */
      status = vol.flash.write(&vol.flash, sectorAddress + BLOCK_ADDRESS_OFFSET2,
			       &blockAddressArea, sizeof blockAddressArea, EXTRA);

    if (status != flOK) {  /* write failed, mark unit as bad */
      vol.physicalUnits[unitNo].unitStatus &= ~STATUS_MASK;
      vol.physicalUnits[unitNo].unitStatus |= UNIT_BAD;
      vol.freeUnits--;
      return status;
    }
  }

  /* mark unit assigned */
  vol.physicalUnits[unitNo].unitStatus &= ~STATUS_MASK;
  vol.physicalUnits[unitNo].unitStatus |= UNIT_AVAILABLE;
  vol.freeUnits--;
  vol.virtualUnits[virtualUnitNo] = unitNo;

  return flOK;
}


/*----------------------------------------------------------------------*/
/*		           f i n i s h U n i t T r a n s f e r		*/
/*									*/
/* Finish unit transfer from replaced unit to transfer unit.		*/
/* copy all the sectors that were not replaced from the replaced unit   */
/* to the transfer unit. This routine is called when either another	*/
/* unit needs replacement, or we finished the current Flie function.	*/
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

static FLStatus finishUnitTransfer(SSFDC vol)
{
  unsigned char ecc[3];
  UnitNo virtualUnitNo;
  unsigned freeSectors = 0, sector;  /* count the number of free sectors */

  if (vol.replacedUnit == NO_UNIT) /* no unit to fold */
    return flOK;

  for (sector = 0; sector < vol.sectorsPerUnit; sector++) {
    if (vol.replacementSectors[sector] == SECTOR_ALLOCATED) { /* copy only sectors
								 that holds valid data */
      CardAddress fromSectorAddress, toSectorAddress;

      fromSectorAddress = unitBaseAddress(&vol, vol.replacedUnit) +
			  (sector << SECTOR_SIZE_BITS);
      toSectorAddress = unitBaseAddress(&vol, vol.transferUnit) +
			(sector << SECTOR_SIZE_BITS);

      vol.flash.read(&vol.flash, fromSectorAddress, ssfdcBuffer,SECTOR_SIZE,0);
      vol.flash.socket->remapped = TRUE;
      checkStatus(writeAndCheck(&vol, toSectorAddress, ssfdcBuffer, 0));
      /* copy ecc fields instead of recalculating them */
      vol.flash.read(&vol.flash, fromSectorAddress + ECC1, ecc, sizeof ecc, EXTRA);
      checkStatus(vol.flash.write(&vol.flash, toSectorAddress + ECC1, ecc,
				  sizeof ecc, EXTRA));
      vol.flash.read(&vol.flash, fromSectorAddress + ECC2, ecc, sizeof ecc, EXTRA);
      checkStatus(vol.flash.write(&vol.flash, toSectorAddress + ECC2, ecc,
				  sizeof ecc, EXTRA));

    }
    if ((vol.replacementSectors[sector] == SECTOR_ERASED) ||
	(vol.replacementSectors[sector] == SECTOR_DELETED1))
      freeSectors++;
  }

  /* the old transfer unit gets the virtual number of the replaced unit */
  virtualUnitNo = physical2Virtual(&vol, vol.replacedUnit, OFFSET1);
  if (virtualUnitNo >= vol.noOfVirtualUnits) {
    /* first address area is not good try the second address area */
    virtualUnitNo = physical2Virtual(&vol, vol.replacedUnit, OFFSET2);
    if (virtualUnitNo >= vol.noOfVirtualUnits)
      return flGeneralFailure;
  }

  if (freeSectors < vol.sectorsPerUnit) {
    checkStatus(assignUnit(&vol, vol.transferUnit, virtualUnitNo));
    checkStatus(formatUnit(&vol, vol.replacedUnit));
    vol.transferUnit = vol.replacedUnit;  /* The old transfer unit is dead,
					     long live the new one */
  }

  vol.replacedUnit = NO_UNIT;

  return flOK;
}


/*----------------------------------------------------------------------*/
/*      	             t l s e t B u s y				*/
/*									*/
/* Notifies the start and end of a file-system operation.		*/
/* On exit, finish unit transfer (if necessary).			*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*      state		: TFFS_ON (1) = operation entry			*/
/*			  TFFS_OFF(0) = operation exit			*/
/*                                                                      */
/*----------------------------------------------------------------------*/

static void tlSetBusy(SSFDC vol, FLBoolean state)
{
  if (state == TFFS_ON)
    vol.replacedUnit = NO_UNIT;  /* in case we exit without doing anything */
  else
    finishUnitTransfer(&vol);  /* I ignore the returned status */
}


/*----------------------------------------------------------------------*/
/*      	             s e t R e p l a c e m e n t U n i t	*/
/*									*/
/* Set a replaced unit. From now on, if write inplace to this unit	*/
/* is impossible, write to the transfer unit instead. this routine	*/
/* reads the status of each sector in the unit and updates the replaced	*/
/* sectors table.							*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/* 	unitNo		: The new replaced unit				*/
/*                                                                      */
/*----------------------------------------------------------------------*/

static void setReplacementUnit(SSFDC vol, UnitNo unitNo)
{
  int iSector;
  StatusArea statusArea;

  vol.replacedUnit = unitNo;
  vol.sectorsDeleted = 0;
  for (iSector = 0; (unsigned)iSector < vol.sectorsPerUnit; iSector++) {
    vol.flash.read(&vol.flash, unitBaseAddress(&vol, unitNo) + (iSector << SECTOR_SIZE_BITS) +
		   STATUS_AREA_OFFSET, &statusArea, sizeof statusArea, EXTRA);
    if (statusArea.sectorStatus != 0xff) {
      vol.replacementSectors[iSector] = SECTOR_DELETED1;
      vol.sectorsDeleted++;
      continue;
    }

    if (vol.physicalUnits[unitNo].erasedSectors & (1 << iSector))
      vol.replacementSectors[iSector] = SECTOR_ERASED;
    else
      vol.replacementSectors[iSector] = SECTOR_ALLOCATED;
  }
}


/*----------------------------------------------------------------------*/
/*      	             m a r k S e c t o r D e l e t e d		*/
/*									*/
/* Mark a sector as deleted on the media, and update the unassigned	*/
/* sectors counter in the physical units table.				*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/* 	unitNo		: The sector is in this unit			*/
/*	unitOffset	: The offset of the sector in the unit		*/
/*                                                                      */
/* Returns:								*/
/*	FLStatus        : 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

static FLStatus markSectorDeleted(SSFDC vol, UnitNo unitNo, unsigned unitOffset)
{
  StatusArea statusArea;

  /* check if this sector is already deleted or erased */
  vol.flash.read(&vol.flash, unitBaseAddress(&vol, unitNo) +
			     unitOffset + STATUS_AREA_OFFSET,
		 &statusArea, sizeof statusArea, EXTRA);
  if ((statusArea.sectorStatus != 0xff) ||
      (vol.physicalUnits[unitNo].erasedSectors & (1 << (unitOffset / SECTOR_SIZE))))
    return flOK;    /* sector is erased or already deleted */

  statusArea.sectorStatus = 0;
  statusArea.unitStatus = 0xff;

  vol.physicalUnits[unitNo].unitStatus++;
  return vol.flash.write(&vol.flash, unitBaseAddress(&vol, unitNo) +
				     unitOffset + STATUS_AREA_OFFSET,
			 &statusArea, sizeof statusArea, EXTRA);
}


/*----------------------------------------------------------------------*/
/*      	             a l l o c a t e U n i t			*/
/*									*/
/* Find a free unit to allocate, erase it if necessary.			*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/* 	unitNo		: Receives the physical number of the allocated */
/*			  unit 						*/
/* Returns:								*/
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

static FLStatus allocateUnit(SSFDC vol, UnitNo *unitNo)
{
  UnitNo originalUnit = vol.roverUnit;

  if (vol.freeUnits > 1) {    /* the transfer unit is always free */
    do {
/* VF: Take 8- vs. 16-bit PhysUnit quantity into account: */
#if (TFFS_SECTORS_PER_UNIT < 32)
      unsigned char unitFlags;
#else
      unsigned short unitFlags;
#endif

      if (++vol.roverUnit == vol.transferUnit) /* we don't want to allocate this one */
	vol.roverUnit++;
      if (vol.roverUnit >= vol.noOfUnits)  /* we got to the end, wrap around */
	vol.roverUnit = vol.CISblock;

      unitFlags = vol.physicalUnits[vol.roverUnit].unitStatus;

      if (((unitFlags & STATUS_MASK) == UNIT_FREE) ||
	  ((unitFlags & STATUS_MASK) == UNIT_ERASED)) {
	if ((unitFlags & STATUS_MASK) == UNIT_FREE)
	  checkStatus(formatUnit(&vol, vol.roverUnit));

	*unitNo = vol.roverUnit;
	return flOK;
      }
    } while (vol.roverUnit != originalUnit);

    return flGeneralFailure;  /* Didn't find the free units */
  }

  return flNotEnoughMemory;
}


#if defined(DEFRAGMENT_VOLUME) || defined(SINGLE_BUFFER)

/*----------------------------------------------------------------------*/
/*      	            d e f r a g m e n t				*/
/*									*/
/* Performs unit allocations to arrange a minimum number of writable	*/
/* sectors.                                                             */
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	sectorsNeeded	: Minimum required sectors			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

static FLStatus defragment(SSFDC vol, long FAR2 *sectorsNeeded)
{
  UnitNo erasedUnits = 0, dummyUnit;

  while (erasedUnits * vol.sectorsPerUnit < *sectorsNeeded) {
    if (vol.badFormat)
      return flBadFormat;

    checkStatus(allocateUnit(&vol, &dummyUnit));
    erasedUnits++;
  }

  *sectorsNeeded = erasedUnits * vol.sectorsPerUnit;

  return flOK;
}

#endif


/*----------------------------------------------------------------------*/
/*      	     a l l o c a t e A n d W r i t e S e c t o r	*/
/*									*/
/* Write to virtual sectorNo. If sectorsNo is not assigned to any 	*/
/* physical sector, allocate a new one and write it there. If sectorNo  */
/* is assigned, try to write in place, if impossible, replace this      */
/* sector in the transfer unit (finish the previous unit transfer       */
/* first). 								*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	sectorNo	: Virtual sector no. to write			*/
/*	fromAddress	: Address of sector data. 			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

static FLStatus allocateAndWriteSector(SSFDC vol,
				       SectorNo sectorNo,
				       void FAR1 *fromAddress)
{
  UnitNo virtualUnitNo = sectorNo / vol.sectorsPerUnit;
  UnitNo unitNo = vol.virtualUnits[virtualUnitNo];
  unsigned sectorInUnit = sectorNo % vol.sectorsPerUnit;
  unsigned unitOffset = sectorInUnit << SECTOR_SIZE_BITS;


  /* Find a unit to write this sector */
  if ((unitNo != NO_UNIT) &&
      ((vol.physicalUnits[unitNo].unitStatus & STATUS_MASK) == UNIT_AVAILABLE)) {
    if (vol.replacedUnit == unitNo) {
      /* this unit is replaced by the transfer unit */
      if ((vol.replacementSectors[sectorInUnit] == SECTOR_REPLACED) ||
	  (vol.replacementSectors[sectorInUnit] == SECTOR_DELETED2)) {
	/* this sector was already replaced */
	UnitNo oldTransferUnit = vol.transferUnit;

	checkStatus(finishUnitTransfer(&vol));
	setReplacementUnit(&vol, oldTransferUnit);
      }

      if (vol.replacementSectors[sectorInUnit] != SECTOR_ERASED)  {
	/* write to the transfer unit, mark deleted in the replaced unit, and
	   mark replaced in the replaced sectors table. */
	checkStatus(writeAndCheck(&vol, unitBaseAddress(&vol, vol.transferUnit) +
					unitOffset,
				  fromAddress, 1))
	checkStatus(markSectorDeleted(&vol, vol.replacedUnit, unitOffset));
	vol.replacementSectors[sectorInUnit] = SECTOR_REPLACED;
      }
      else {   /* sector erased, write inplace */
	checkStatus(writeAndCheck(&vol, unitBaseAddress(&vol, vol.replacedUnit) +
					unitOffset,
				  fromAddress, 1));
	vol.replacementSectors[sectorInUnit] = SECTOR_ALLOCATED;
      }
    }
    else {
      if (vol.physicalUnits[unitNo].erasedSectors &
	  (1 << (unitOffset / SECTOR_SIZE))) {
	/* sector is erased, write inplace */
	checkStatus(writeAndCheck(&vol, unitBaseAddress(&vol, unitNo) + unitOffset,
				  fromAddress, 1));
      }
      else {
	/* finish the previous transfer first */
	checkStatus(finishUnitTransfer(&vol));
	setReplacementUnit(&vol, unitNo);
	/* write to the transfer unit, mark deleted in the replaced unit, and
	   mark replaced in the replaced sectors table. */
	checkStatus(writeAndCheck(&vol, unitBaseAddress(&vol, vol.transferUnit) +
					unitOffset,
				  fromAddress, 1))
	checkStatus(markSectorDeleted(&vol, vol.replacedUnit, unitOffset));
	vol.replacementSectors[sectorInUnit] = SECTOR_REPLACED;
      }
    }
  }
  else {
    /* this sector is unassigned, find a free unit and write the sector there */
    UnitNo toUnit;

    checkStatus(allocateUnit(&vol, &toUnit));

    checkStatus(writeAndCheck(&vol, unitBaseAddress(&vol, toUnit) + unitOffset,
			      fromAddress, 1));
    checkStatus(assignUnit(&vol, toUnit, virtualUnitNo));
  }

  return flOK;
}


/*----------------------------------------------------------------------*/
/*      	         d e l e t e S e c t o r			*/
/*									*/
/* Marks contiguous sectors as deleted.					*/
/* Update unassigned sectors counter in physical units table, and	*/
/* deleted replaced sectors table. If all the sectors in a unit are     */
/* deleted, mark it as free.						*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	sectorNo	: First sector no. to delete			*/
/*	noOfSectors	: No. of sectors to delete			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

static FLStatus deleteSector(SSFDC vol, SectorNo sectorNo, int noOfSectors)
{
  int iSector;

  if (vol.badFormat)
    return flBadFormat;
  if (sectorNo + noOfSectors > vol.virtualSectors)
    return flSectorNotFound;

  for (iSector = 0; iSector < noOfSectors; iSector++, sectorNo++) {
    unsigned sectorInUnit = sectorNo % vol.sectorsPerUnit;
    CardAddress sectorAddress = virtual2Physical(&vol, sectorNo);

    if (sectorAddress != UNASSIGNED_ADDRESS) {
      UnitNo virtualUnitNo = sectorNo / vol.sectorsPerUnit;
      UnitNo unitNo = vol.virtualUnits[virtualUnitNo];

      if (!(vol.physicalUnits[unitNo].unitStatus & UNIT_AVAILABLE) ||
	  (unitNo == NO_UNIT))
	return flSectorNotFound;

      if (unitNo == vol.replacedUnit) {
	/* this sector was replaced, mark it in the transfer unit, and
	   update the repleced sectors table. */
	if (vol.replacementSectors[sectorInUnit] == SECTOR_REPLACED) {
	  checkStatus(markSectorDeleted(&vol, vol.transferUnit,
					sectorInUnit << SECTOR_SIZE_BITS));
	  vol.replacementSectors[sectorInUnit] = SECTOR_DELETED2;
	}
	else {
	  checkStatus(markSectorDeleted(&vol, unitNo,
					sectorInUnit << SECTOR_SIZE_BITS));
	  vol.replacementSectors[sectorInUnit] = SECTOR_DELETED1;
	}
	if (++vol.sectorsDeleted >= vol.sectorsPerUnit) {
	  /* all the sectors in the replaced unit are deleted, no point
	     to keep their deleted copy in the transfer unit. */
	  checkStatus(formatUnit(&vol, vol.transferUnit));
	  vol.physicalUnits[vol.replacedUnit].unitStatus &= ~STATUS_MASK;
	  vol.physicalUnits[vol.replacedUnit].unitStatus |= UNIT_FREE;
	  vol.virtualUnits[virtualUnitNo] = NO_UNIT;
	  vol.replacedUnit = NO_UNIT;
	  vol.freeUnits++;
	}
      }
      else {  /* this sector is not in the replaced unit */
	checkStatus(markSectorDeleted(&vol, unitNo,
				      sectorInUnit << SECTOR_SIZE_BITS));
	if ((vol.physicalUnits[unitNo].unitStatus & ~STATUS_MASK) ==
	     (int)vol.sectorsPerUnit) {
	  /* all the sectors in this unit are unassigned, it can be marked
	     as free. */
	  vol.physicalUnits[unitNo].unitStatus &= ~STATUS_MASK;
	  vol.physicalUnits[unitNo].unitStatus |= UNIT_FREE;
	  vol.freeUnits++;
	  vol.virtualUnits[virtualUnitNo] = NO_UNIT;
	}
      }
    }
  }

  return flOK;
}


/*----------------------------------------------------------------------*/
/*      	          w r i t e S e c t o r				*/
/*									*/
/* Writes a sector.							*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	sectorNo	: Sector no. to write				*/
/*	fromAddress	: Data to write					*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus        : 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

static FLStatus writeSector(SSFDC vol,SectorNo sectorNo, void FAR1 *fromAddress)
{
  FLStatus status;
  int i;

  if (vol.badFormat)
    return flBadFormat;
  if (sectorNo > vol.virtualSectors)
    return flSectorNotFound;

  status = flWriteFault;
  for (i = 0; i < 4 && status == flWriteFault; i++) {
    if (vol.mappedSectorNo == sectorNo)
      vol.mappedSectorNo = UNASSIGNED_SECTOR;
    status = allocateAndWriteSector(&vol,sectorNo,fromAddress);
  }

  return status;
}


/*----------------------------------------------------------------------*/
/*		           m o u n t U n i t				*/
/*									*/
/* Mount one unit. Get the unit status (assigned, free or bad) and 	*/
/* update the tables.							*/
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	unitNo		: Unit to mount					*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

static FLStatus mountUnit(SSFDC vol, UnitNo unitNo)
{
  StatusArea statusArea;
  UnitNo virtualUnitNo;

  /* initialize unassigned sectors counter */
  vol.physicalUnits[unitNo].unitStatus &= STATUS_MASK;

  vol.flash.read(&vol.flash, unitBaseAddress(&vol, unitNo) + STATUS_AREA_OFFSET,
		 &statusArea, sizeof statusArea, EXTRA);

  if (statusArea.unitStatus != 0xff) {
    vol.physicalUnits[unitNo].unitStatus &= ~STATUS_MASK;
    vol.physicalUnits[unitNo].unitStatus |= UNIT_BAD;
    return flOK;
  }

  virtualUnitNo = physical2Virtual(&vol, unitNo, OFFSET1);

  if (virtualUnitNo >= vol.noOfVirtualUnits) {
    vol.physicalUnits[unitNo].unitStatus = UNIT_FREE | vol.sectorsPerUnit;
    vol.freeUnits++;
  }
  else {
    if (vol.virtualUnits[virtualUnitNo] != NO_UNIT) {
    /* there is another unit with this virtual address */
      vol.physicalUnits[unitNo].unitStatus = UNIT_FREE | vol.sectorsPerUnit;
      vol.freeUnits++;
    }
    else {  /* this unit is assigned */
      CardAddress unitBase;
      unsigned sector;

      vol.physicalUnits[unitNo].unitStatus &= ~STATUS_MASK;
      vol.physicalUnits[unitNo].unitStatus |= UNIT_AVAILABLE;
      vol.virtualUnits[virtualUnitNo] = unitNo;

      /* count the number of unassigned sectors and mark erased sectors */
      vol.physicalUnits[unitNo].erasedSectors = 0;
      unitBase = unitBaseAddress(&vol, unitNo);
      for (sector = 0; sector < vol.sectorsPerUnit; sector++) {
	vol.flash.read(&vol.flash, unitBase + (sector << SECTOR_SIZE_BITS) +
				   STATUS_AREA_OFFSET,
		       &statusArea, sizeof statusArea, EXTRA);
	if (statusArea.sectorStatus != 0xff)
	  vol.physicalUnits[unitNo].unitStatus++;
	else if (isErasedSector(&vol, unitNo, sector << SECTOR_SIZE_BITS)) {
	  vol.physicalUnits[unitNo].unitStatus++;
	  vol.physicalUnits[unitNo].erasedSectors |= (1 << sector);
	}
      }

      /* check if all the sectors are unassigned */
      if ((vol.physicalUnits[unitNo].unitStatus & ~STATUS_MASK) ==
	   (int)vol.sectorsPerUnit) {
	vol.physicalUnits[unitNo].unitStatus &= ~STATUS_MASK;
	vol.physicalUnits[unitNo].unitStatus |= UNIT_FREE;
	vol.virtualUnits[virtualUnitNo] = NO_UNIT;
	vol.freeUnits++;
      }
    }
  }


  /* prepare a transfer unit */
  if (((vol.physicalUnits[unitNo].unitStatus & STATUS_MASK) == UNIT_FREE) &&
      (vol.transferUnit == NO_UNIT)) {
    checkStatus (formatUnit(&vol, unitNo));
    vol.transferUnit = unitNo;
  }


  return flOK;
}


/*----------------------------------------------------------------------*/
/*      	                 i n i t S S F D C			*/
/*									*/
/* Initializes essential volume data as a preparation for mount or	*/
/* format.								*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

static FLStatus initSSFDC(SSFDC vol)
{
  long int size = 1;

  if (!(vol.flash.flags & NFTL_ENABLED)) {
  #ifdef DEBUG_PRINT
    DEBUG_PRINT("Debug: media is not fit for SSFDC format.\n");
  #endif
    return flUnknownMedia;
  }

  vol.physicalUnits = NULL;
  vol.virtualUnits = NULL;

  for (vol.erasableBlockSizeBits = 0; size < vol.flash.erasableBlockSize;
       vol.erasableBlockSizeBits++, size <<= 1);
  vol.unitSizeBits = vol.erasableBlockSizeBits;
  vol.noOfUnits = (vol.flash.noOfChips * vol.flash.chipSize) >> vol.unitSizeBits;

  vol.badFormat = TRUE;	/* until mount completes*/
  vol.mappedSectorNo = UNASSIGNED_SECTOR;
  /*get pointer to buffer (we assume SINGLE_BUFFER is not defined) */
  vol.buffer = flBufferOf(flSocketNoOf(vol.flash.socket));

  return flOK;
}


/*----------------------------------------------------------------------*/
/*      	            i n i t T a b l e s				*/
/*									*/
/* Allocates and initializes the dynamic volume table, including the	*/
/* unit tables and secondary virtual map.				*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

static FLStatus initTables(SSFDC vol)
{
  /* Allocate the conversion tables */
#ifdef MALLOC_TFFS
  vol.physicalUnits = (Unit *) MALLOC_TFFS(vol.noOfUnits * sizeof(Unit));
  vol.virtualUnits = (UnitNo *) MALLOC_TFFS(vol.noOfVirtualUnits * sizeof(UnitNo));
  vol.replacementSectors = (unsigned char *)
			   MALLOC_TFFS(vol.sectorsPerUnit * sizeof(unsigned char));

  if (vol.physicalUnits == NULL ||
      vol.virtualUnits == NULL ||
      vol.replacementSectors == NULL) {
  #ifdef DEBUG_PRINT
    DEBUG_PRINT("Debug: Failed allocating conversion tables for SSFDC.\n");
  #endif
    return flNotEnoughMemory;
  }
#else
  char *heapPtr;

  heapPtr = vol.heap;
  vol.physicalUnits = (Unit *) heapPtr;
  heapPtr += vol.noOfUnits * sizeof(Unit);
  vol.virtualUnits = (UnitNo *) heapPtr;
  heapPtr += vol.noOfVirtualUnits * sizeof(UnitNo);
  vol.replacementSectors = (unsigned char *) heapPtr;
  heapPtr += vol.sectorsPerUnit * sizeof(unsigned char);

  if (heapPtr > vol.heap + sizeof vol.heap) {
  #ifdef DEBUG_PRINT
    DEBUG_PRINT("Debug: Not enough memory for SSFDC conversion tables.\n");
  #endif
    return flNotEnoughMemory;
  }
#endif

  return flOK;
}

/*----------------------------------------------------------------------*/
/*      	         d i s m o u n t S S F D C			*/
/*									*/
/* Dismount SSFDC volume							*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*									*/
/*----------------------------------------------------------------------*/

static void dismountSSFDC(SSFDC vol)
{
  finishUnitTransfer(&vol);  /* copy replaced sectors if needed */
#ifdef MALLOC_TFFS
  FREE_TFFS(vol.physicalUnits);
  FREE_TFFS(vol.virtualUnits);
  FREE_TFFS(vol.replacementSectors);
#endif
}


/*----------------------------------------------------------------------*/
/*      	               m o u n t S S F D C			*/
/*									*/
/* Mount the volume							*/
/*									*/
/* Parameters:                                                          */
/*	flash		: Flash media to mount				*/
/*	tl		: Mounted translation layer on exit		*/
/*      volForCallback	: Pointer to FLFlash structure for power on	*/
/*			  callback routine.				*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

/*static*/ FLStatus mountSSFDC(FLFlash *flash, TL *tl, FLFlash **volForCallback)
{
  SSFDC vol = &vols[flSocketNoOf(flash->socket)];
  UnitNo iUnit;

#ifdef DEBUG_PRINT
  DEBUG_PRINT("Debug: starting SSFDC mount.\n");
#endif
  vol.replacedUnit = NO_UNIT; /* this is for finishUnitTransfer in case mount fails */
  vol.flash = *flash;
  *volForCallback = &vol.flash;
  checkStatus(initSSFDC(&vol));

  vol.sectorsPerUnit = 1 << (vol.unitSizeBits - SECTOR_SIZE_BITS);

  if (identifyFormat(&vol, &vol.CISblock) != SSFDC_FORMAT) {
  #ifdef DEBUG_PRINT
    DEBUG_PRINT("Debug: not SSFDC format.\n");
  #endif
    return flUnknownMedia;
  }

  vol.virtualSectors = vol.sectorsPerUnit;
  if (vol.noOfUnits < 500)
    vol.virtualSectors *= 250;         /* 1 MByte chip        */
  else if (vol.noOfUnits < 1000)
    vol.virtualSectors *= 500;         /* 2 or 4 MByte chip   */
  else
    vol.virtualSectors *= 1000;        /* 8 0r 16 MByte chip  */

  vol.unitOffsetMask = (1L << vol.unitSizeBits) - 1;

  vol.noOfVirtualUnits = (vol.virtualSectors + vol.sectorsPerUnit - 1) /
			 vol.sectorsPerUnit;

  if ((vol.virtualSectors >> (vol.unitSizeBits - SECTOR_SIZE_BITS)) >
      (unsigned long)(vol.noOfUnits - 1))
    return flBadFormat;

  checkStatus(initTables(&vol));

  vol.badFormat = FALSE;
  vol.transferUnit = NO_UNIT;

  for (iUnit = 0; iUnit < vol.noOfVirtualUnits; iUnit++)
    vol.virtualUnits[iUnit] = NO_UNIT;

  /* Mount all units */
  vol.freeUnits = 0;
  for (iUnit = vol.CISblock; iUnit < vol.noOfUnits; iUnit++) {
    if (iUnit == vol.CISblock)
      vol.physicalUnits[iUnit].unitStatus &= ~UNIT_AVAILABLE;
    else
      checkStatus(mountUnit(&vol, iUnit));
  }


  /* Initialize allocation rover */
  vol.roverUnit = vol.CISblock;

  tl->rec = &vol;
  tl->mapSector = mapSector;
  tl->writeSector = writeSector;
  tl->deleteSector = deleteSector;
#if defined(DEFRAGMENT_VOLUME) || defined(SINGLE_BUFFER)
  tl->defragment = defragment;
#endif
#ifdef FORMAT_VOLUME
  tl->sectorsInVolume = sectorsInVolume;
#endif
  tl->tlSetBusy = tlSetBusy;
  tl->dismount = dismountSSFDC;

#ifdef DEBUG_PRINT
  DEBUG_PRINT("Debug: finished SSFDC mount.\n");
#endif

  return flOK;
}


#ifdef FORMAT_VOLUME

/*----------------------------------------------------------------------*/
/*      	               i s E r a s e d U n i t			*/
/*									*/
/* Check if a unit is erased.                                           */
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*                                                                      */
/* Returns:                                                             */
/*	TRUE if unit is erased, FALSE otherwise				*/
/*----------------------------------------------------------------------*/

static FLBoolean isErasedUnit(SSFDC vol, UnitNo unitNo)
{
  CardAddress offset;

  for (offset = 0; (long)offset < (1L << vol.unitSizeBits); offset += SECTOR_SIZE)
    if (!(isErasedSector(&vol, unitNo, offset)))
      return FALSE;

  return TRUE;
}


/*----------------------------------------------------------------------*/
/*      	              f o r m a t S S F D C			*/
/*									*/
/* Perform SSFDC Format.						*/
/*									*/
/* Parameters:                                                          */
/*	flash		: Flash media to format				*/
/*	formatParams	: Address of FormatParams structure to use	*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

/*static*/ FLStatus formatSSFDC(FLFlash *flash, FormatParams FAR1 *formatParams)
{
  SSFDC vol = &vols[flSocketNoOf(flash->socket)];
  long int unitSize;
  UnitNo iUnit;
  unsigned char CISblock[SECTOR_SIZE] = {CIS_DATA}, CISextra[16], ecc[3];

#ifdef DEBUG_PRINT
  DEBUG_PRINT("Debug: Starting SSFDC format.\n");
#endif

  vol.replacedUnit = NO_UNIT; /* this is for finishUnitTransfer in case format fails */
  vol.flash = *flash;
  checkStatus(initSSFDC(&vol));

  vol.unitOffsetMask = (1L << vol.unitSizeBits) - 1;
  vol.sectorsPerUnit = 1 << (vol.unitSizeBits - SECTOR_SIZE_BITS);

  unitSize = 1L << vol.unitSizeBits;

  vol.virtualSectors = vol.sectorsPerUnit;
  if (vol.noOfUnits < 500)
    vol.virtualSectors *= 250;         /* 1 MByte chip        */
  else if (vol.noOfUnits < 1000)
    vol.virtualSectors *= 500;         /* 2 or 4 MByte chip   */
  else
    vol.virtualSectors *= 1000;        /* 8 0r 16 MByte chip  */

  vol.noOfVirtualUnits = (vol.virtualSectors + vol.sectorsPerUnit - 1) /
			 vol.sectorsPerUnit;

  checkStatus(initTables(&vol));

  for (iUnit = 0; iUnit < vol.noOfVirtualUnits; iUnit++)
    vol.virtualUnits[iUnit] = NO_UNIT;

  /* Find the medium boot record and identify the format */

  if (identifyFormat(&vol, &vol.CISblock) != SSFDC_FORMAT) {
	    /* not ssfdc format - scan it for bad blocks */

	    /* Generate the bad unit table and find a place for the CIS */
    vol.CISblock = vol.noOfUnits;
    for (iUnit = 0; iUnit < vol.noOfUnits; iUnit++) {
      if (isErasedUnit(&vol, iUnit)) {
	vol.physicalUnits[iUnit].unitStatus &= ~STATUS_MASK;
	vol.physicalUnits[iUnit].unitStatus |= UNIT_ERASED;
	if (vol.CISblock == vol.noOfUnits)
	  vol.CISblock = iUnit;
      }
      else {
	vol.physicalUnits[iUnit].unitStatus &= ~STATUS_MASK;
	vol.physicalUnits[iUnit].unitStatus |= UNIT_BAD;
      }
    }
    if (vol.CISblock == vol.noOfUnits) {
    #ifdef DEBUG_PRINT
      DEBUG_PRINT("Debug: format failed, too many bad blocks.\n");
    #endif
      return flGeneralFailure;
    }
  }
  else {  /* ssfdc format - read unit status from block status area */
    StatusArea statusArea;

    for (iUnit = 0; iUnit < vol.noOfUnits; iUnit++) {
      vol.flash.read(&vol.flash, unitBaseAddress(&vol, iUnit) + STATUS_AREA_OFFSET,
		     &statusArea, sizeof statusArea, EXTRA);
      vol.physicalUnits[iUnit].unitStatus &= ~STATUS_MASK;
      vol.physicalUnits[iUnit].unitStatus |= (statusArea.unitStatus == 0xff)
					    ? UNIT_FREE : UNIT_BAD;
    }
  }

  /* protect the CIS block */
  vol.physicalUnits[vol.CISblock].unitStatus &= ~UNIT_AVAILABLE;

  /* format all units */
  for (iUnit = vol.CISblock; iUnit < vol.noOfUnits; iUnit++) {
    FLStatus status = formatUnit(&vol, iUnit);
    if(status == flWriteFault) {
      if (iUnit != vol.CISblock)
	if ((vol.physicalUnits[iUnit].unitStatus & STATUS_MASK) != UNIT_BAD) {
	  vol.physicalUnits[iUnit].unitStatus &= ~STATUS_MASK;
	  vol.physicalUnits[iUnit].unitStatus |= UNIT_BAD;
	}
    }
    else if (status != flOK)
      return status;
  }


  /* Write the CIS block */
  vol.physicalUnits[vol.CISblock].unitStatus &= ~STATUS_MASK;
  vol.physicalUnits[vol.CISblock].unitStatus |= UNIT_FREE;     /* Unprotect it */

  checkStatus(formatUnit(&vol, vol.CISblock));
  checkStatus(vol.flash.write(&vol.flash, unitBaseAddress(&vol, vol.CISblock),
			   CISblock, sizeof CISblock, 0));

  /* write extra bytes */

  createEcc(CISblock, ecc);

  tffsset(CISextra, 0xff, sizeof CISextra);
  CISextra[CIS_DATA_OFFSET+2] = CISextra[CIS_DATA_OFFSET+3] = 0;
  CISextra[ECC1] = CISextra[ECC2] = ecc[0];
  CISextra[ECC1+1] = CISextra[ECC2+1] = ecc[1];
  CISextra[ECC1+2] = CISextra[ECC2+2] = ecc[2];
  CISextra[ECC1-2] = CISextra[ECC1-1] = 0;

  checkStatus(vol.flash.write(&vol.flash, unitBaseAddress(&vol, vol.CISblock),
			      CISextra, sizeof CISextra, EXTRA));

  /* Protect the CIS block */
  vol.physicalUnits[vol.CISblock].unitStatus &= ~UNIT_AVAILABLE;

#ifdef DEBUG_PRINT
  DEBUG_PRINT("Debug: finished SSFDC format.\n");
#endif
  return flOK;
}

#endif /* FORMAT_VOLUME */


#if FLASE
/*----------------------------------------------------------------------*/
/*      	         f l R e g i s t e r S S F D C			*/
/*									*/
/* Register this translation layer					*/
/*									*/
/* Parameters:                                                          */
/*	None								*/
/*                                                                      */
/* Returns:								*/
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

FLStatus flRegisterSSFDC(void)
{
  if (noOfTLs >= TLS)
    return flTooManyComponents;

  tlTable[noOfTLs].mountRoutine = mountSSFDC;
#ifdef FORMAT_VOLUME
  tlTable[noOfTLs].formatRoutine = formatSSFDC;
#endif
  noOfTLs++;

  return flOK;
}
#endif /* FLASE */

