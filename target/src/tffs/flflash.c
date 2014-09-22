/*
 * $Log:   P:/user/amir/lite/vcs/flflash.c_v  $
 *
 *    Rev 1.14   08 Aug 2000 11:22:00   chackney
 * Changed second arameter for ISRAM_WRITE to &flashPtr[0]
 *
 *    Rev 1.13   03 Nov 1997 16:11:32   danig
 * amdCmdRoutine receives FlashPTR
 *
 *    Rev 1.12   10 Sep 1997 16:28:48   danig
 * Got rid of generic names
 *
 *    Rev 1.11   04 Sep 1997 18:36:24   danig
 * Debug messages
 *
 *    Rev 1.10   18 Aug 1997 11:46:26   danig
 * MTDidentifyRoutine already defined in header file
 *
 *    Rev 1.9   28 Jul 1997 14:43:30   danig
 * setPowerOnCallback
 *
 *    Rev 1.8   24 Jul 1997 17:54:28   amirban
 * FAR to FAR0
 *
 *    Rev 1.7   07 Jul 1997 15:21:38   amirban
 * Ver 2.0
 *
 *    Rev 1.6   08 Jun 1997 17:03:28   amirban
 * power on callback
 *
 *    Rev 1.5   20 May 1997 13:53:34   danig
 * Defined write/read mode
 *
 *    Rev 1.4   18 Aug 1996 13:47:24   amirban
 * Comments
 *
 *    Rev 1.3   31 Jul 1996 14:30:36   amirban
 * New flash.erase definition
 *
 *    Rev 1.2   04 Jul 1996 18:20:02   amirban
 * New flag field
 *
 *    Rev 1.1   16 Jun 1996 14:02:20   amirban
 * Corrected reset method in intelIdentify
 *
 *    Rev 1.0   20 Mar 1996 13:33:08   amirban
 * Initial revision.
 */

/************************************************************************/
/*                                                                      */
/*		FAT-FTL Lite Software Development Kit			*/
/*		Copyright (C) M-Systems Ltd. 1995-1996			*/
/*									*/
/************************************************************************/


#include "flflash.h"

#define	READ_ID			0x90
#define	INTEL_READ_ARRAY        0xff
#define	AMD_READ_ARRAY		0xf0

#if	FALSE
/* MTD registration information */

extern int noOfMTDs;

extern MTDidentifyRoutine mtdTable[];
#endif	/* FALSE */


/*----------------------------------------------------------------------*/
/*      	            f l a s h M a p				*/
/*									*/
/* Default flash map method: Map through socket window.			*/
/* This method is applicable for all NOR Flash				*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*      address		: Card address to map				*/
/*	length		: Length to map (irrelevant here)		*/
/*                                                                      */
/* Returns:                                                             */
/*	Pointer to required card address				*/
/*----------------------------------------------------------------------*/

static void FAR0 *flashMap(FLFlash vol, CardAddress address, int length)
{
  return flMap(vol.socket,address);
}


/*----------------------------------------------------------------------*/
/*      	            f l a s h R e a d				*/
/*									*/
/* Default flash read method: Read by copying from mapped address	*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*      address		: Card address to read				*/
/*	buffer		: Area to read into				*/
/*	length		: Length to read				*/
/*                                                                      */
/*----------------------------------------------------------------------*/

static FLStatus flashRead(FLFlash vol,
			CardAddress address,
			void FAR1 *buffer,
			int length,
			int mode)
{
  tffscpy(buffer,vol.map(&vol,address,length),length);

  return flOK;
}



/*----------------------------------------------------------------------*/
/*      	         f l a s h N o W r i t e			*/
/*									*/
/* Default flash write method: Write not allowed (read-only mode)	*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*      address		: Card address to write				*/
/*	buffer		: Area to write from				*/
/*	length		: Length to write				*/
/*                                                                      */
/* Returns:                                                             */
/*	Write-protect error						*/
/*----------------------------------------------------------------------*/

static FLStatus flashNoWrite(FLFlash vol,
			   CardAddress address,
			   const void FAR1 *from,
			   int length,
			   int mode)
{
  return flWriteProtect;
}


/*----------------------------------------------------------------------*/
/*      	         f l a s h N o E r a s e			*/
/*									*/
/* Default flash erase method: Erase not allowed (read-only mode)	*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*      firstBlock	: No. of first erase block			*/
/*	noOfBlocks	: No. of contiguous blocks to erase		*/
/*                                                                      */
/* Returns:                                                             */
/*	Write-protect error						*/
/*----------------------------------------------------------------------*/

static FLStatus flashNoErase(FLFlash vol,
			   int firstBlock,
			   int noOfBlocks)
{
  return flWriteProtect;
}

/*----------------------------------------------------------------------*/
/*      	         s e t N o C a l l b a c k			*/
/*									*/
/* Register power on callback routine. Default: no routine is 		*/
/* registered.								*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*                                                                      */
/*----------------------------------------------------------------------*/

static void setNoCallback(FLFlash vol)
{
  flSetPowerOnCallback(vol.socket,NULL,NULL);
}

/*----------------------------------------------------------------------*/
/*      	         f l I n t e l I d e n t i f y			*/
/*									*/
/* Identify the Flash type and interleaving for Intel-style Flash.	*/
/* Sets the value of vol.type (JEDEC id) & vol.interleaving.	        */
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	amdCmdRoutine	: Routine to read-id AMD/Fujitsu style at	*/
/*			  a specific location. If null, Intel procedure	*/
/*			  is used.                                      */
/*      idOffset	: Chip offset to use for identification		*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 = OK, otherwise failed (invalid Flash array)*/
/*----------------------------------------------------------------------*/

void flIntelIdentify(FLFlash vol,
		     void (*amdCmdRoutine)(FLFlash vol, CardAddress,
					   unsigned char, FlashPTR),
		     CardAddress idOffset)
{
  int inlv;
  FlashPTR flashPtr;
  unsigned char vendorId = 0;
  unsigned char firstByte = 0;
  unsigned char resetCmd = amdCmdRoutine ? AMD_READ_ARRAY : INTEL_READ_ARRAY;

  flNeedVpp (vol.socket);		/* ADDED, first thing to do */
  flashPtr = (FlashPTR) flMap(vol.socket,idOffset);

  for (inlv = 0; inlv < 15; inlv++) {	/* Increase interleaving until failure */
    flashPtr[inlv] = resetCmd;	/* Reset the chip */
    flashPtr[inlv] = resetCmd;	/* Once again for luck */
    if (inlv == 0)
      firstByte = flashPtr[0]; 	/* Remember byte on 1st chip */
    if (amdCmdRoutine)	/* AMD: use unlock sequence */
      amdCmdRoutine(&vol,idOffset + inlv, READ_ID, flashPtr);
    else
      flashPtr[inlv] = READ_ID;	/* Read chip id */
    flDelayLoop(2);  /* HOOK for VME-177 */
    if (inlv == 0)
      vendorId = flashPtr[0];	/* Assume first chip responded */
    else if (flashPtr[inlv] != vendorId || firstByte != flashPtr[0]) {
      /* All chips should respond in the same way. We know interleaving = n */
      /* when writing to chip n affects chip 0.				    */

      /* Get full JEDEC id signature */
      vol.type = (FlashType) ((vendorId << 8) | flashPtr[inlv]);
      flashPtr[inlv] = resetCmd;
      break;
    }
    flashPtr[inlv] = resetCmd;
  }

  if (inlv & (inlv - 1))
    vol.type = NOT_FLASH;		/* not a power of 2, no way ! */
  else
    vol.interleaving = inlv;

  flDontNeedVpp (vol.socket);		/* ADDED, last thing to do */
}


/*----------------------------------------------------------------------*/
/*      	            i n t e l S i z e				*/
/*									*/
/* Identify the card size for Intel-style Flash.			*/
/* Sets the value of vol.noOfChips.					*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	amdCmdRoutine	: Routine to read-id AMD/Fujitsu style at	*/
/*			  a specific location. If null, Intel procedure	*/
/*			  is used.                                      */
/*      idOffset	: Chip offset to use for identification		*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 = OK, otherwise failed (invalid Flash array)*/
/*----------------------------------------------------------------------*/

FLStatus flIntelSize(FLFlash vol,
		     void (*amdCmdRoutine)(FLFlash vol, CardAddress,
					   unsigned char, FlashPTR),
		     CardAddress idOffset)
{
  unsigned char resetCmd = amdCmdRoutine ? AMD_READ_ARRAY : INTEL_READ_ARRAY;
  FlashPTR flashPtr;

  flNeedVpp (vol.socket);		/* ADDED, first thing to do */
  flashPtr = (FlashPTR) vol.map(&vol,idOffset,0);

  if (amdCmdRoutine)	/* AMD: use unlock sequence */
    amdCmdRoutine(&vol,0,READ_ID, flashPtr);
  else
    flashPtr[0] = READ_ID;
  /* We leave the first chip in Read ID mode, so that we can		*/
  /* discover an address wraparound.					*/

  for (vol.noOfChips = 0;	/* Scan the chips */
       vol.noOfChips < 2000;  /* Big enough ? */
       vol.noOfChips += vol.interleaving) {
    int i;

    flashPtr = (FlashPTR) vol.map(&vol,vol.noOfChips * vol.chipSize + idOffset,0);

    /* Check for address wraparound to the first chip */
    if (vol.noOfChips > 0 &&
	(FlashType) ((flashPtr[0] << 8) | flashPtr[vol.interleaving]) == vol.type)
      goto noMoreChips;	   /* wraparound */

    /* Check if chip displays the same JEDEC id and interleaving */
    for (i = (vol.noOfChips ? 0 : 1); i < vol.interleaving; i++) {
      if (amdCmdRoutine)	/* AMD: use unlock sequence */
	amdCmdRoutine(&vol,vol.noOfChips * vol.chipSize + idOffset + i,
		      READ_ID, flashPtr);
      else
	flashPtr[i] = READ_ID;
      if ((FlashType) ((flashPtr[i] << 8) | flashPtr[i + vol.interleaving]) !=
	  vol.type)
	goto noMoreChips;  /* This "chip" doesn't respond correctly, so we're done */

      flashPtr[i] = resetCmd;
    }
  }

noMoreChips:
  flashPtr = (FlashPTR) vol.map(&vol,idOffset,0);
  flashPtr[0] = resetCmd;		/* reset the original chip */

  flDontNeedVpp (vol.socket);		/* ADDED, last thing to do */
  return (vol.noOfChips == 0) ? flUnknownMedia : flOK;
}


/*----------------------------------------------------------------------*/
/*      	                 i s R A M				*/
/*									*/
/* Checks if the card memory behaves like RAM				*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*                                                                      */
/* Returns:                                                             */
/*	0 = not RAM-like, other = memory is apparently RAM		*/
/*----------------------------------------------------------------------*/

static FLBoolean isRAM(FLFlash vol)
{
  volatile unsigned int FAR0 * flashIPtr;
  unsigned int firstInt;
  unsigned int writeInt;

  flNeedVpp (vol.socket);		/* ADDED, first thing to do */
  flashIPtr = (volatile unsigned int FAR0 *) flMap(vol.socket,0);

  firstInt = flashIPtr[0];
  writeInt = (firstInt != 0x0) ? 0x0 : (unsigned int) -1;

  ISRAM_WRITE(writeInt, &flashIPtr[0]);	/* write something else safe */
  flDelayLoop(2);  /* HOOK for VME-177 */
  if (flashIPtr[0] == writeInt) {	/* Was it written ? */
    flashIPtr[0] = firstInt;		/* must be RAM, undo the damage */
    flDontNeedVpp (vol.socket);		/* ADDED, last thing to do */

#ifdef DEBUG_PRINT
  DEBUG_PRINT("Debug: error, socket window looks like RAM.\n");
#endif

    return TRUE;
  }

  flDontNeedVpp (vol.socket);		/* ADDED, last thing to do */
  return FALSE;
}
/*----------------------------------------------------------------------*/
/*      	        f l I d e n t i f y F l a s h			*/
/*									*/
/* Identify the current Flash medium and select an MTD for it		*/
/*									*/
/* Parameters:                                                          */
/*	socket		: Socket of flash				*/
/*	vol		: New volume pointer				*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 = Flash was identified			*/
/*			  other = identification failed                 */
/*----------------------------------------------------------------------*/

FLStatus flIdentifyFlash(FLSocket *socket, FLFlash vol)
{
  FLStatus status = flUnknownMedia;
  int iMTD;

  vol.socket = socket;

#ifndef FIXED_MEDIA
  /* Check that we have a media */
  flResetCardChanged(vol.socket);		/* we're mounting anyway */
  checkStatus(flMediaCheck(vol.socket));
#endif

  if (isRAM(&vol))
    return flUnknownMedia;	/* if it looks like RAM, leave immediately */

  /* Install default methods */
  vol.type = NOT_FLASH;
  vol.flags = 0;
  vol.map = flashMap;
  vol.read = flashRead;
  vol.write = flashNoWrite;
  vol.erase = flashNoErase;
  vol.setPowerOnCallback = setNoCallback;

  /* Attempt all MTD's */
  for (iMTD = 0; iMTD < noOfMTDs && status != flOK; iMTD++)
    status = mtdTable[iMTD](&vol);

  if (status != flOK) {	/* No MTD recognition */
    /* Setup arbitrary parameters for read-only mount */
    vol.chipSize = 0x100000L;
    vol.erasableBlockSize = 0x1000;
    vol.noOfChips = 1;
    vol.interleaving = 1;

  #ifdef DEBUG_PRINT
    DEBUG_PRINT("Debug: did not identify flash media.\n");
  #endif
    return flUnknownMedia;
  }

  return flOK;
}
