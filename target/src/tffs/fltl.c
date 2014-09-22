
/*
 * $Log:   P:/user/amir/lite/vcs/fltl.c_v  $
 *
 *    Rev 1.2   28 Jul 1997 14:48:06   danig
 * Call to setPowerOnCallback in flMount
 *
 *    Rev 1.1   20 Jul 1997 17:14:54   amirban
 * Format change
 *
 *    Rev 1.0   07 Jul 1997 15:23:10   amirban
 * Initial revision.
 */

/************************************************************************/
/*                                                                      */
/*		FAT-FTL Lite Software Development Kit			*/
/*		Copyright (C) M-Systems Ltd. 1995-1996			*/
/*									*/
/************************************************************************/


#include "flflash.h"
#include "fltl.h"


extern int noOfTLs;	/* No. of translation layers actually registered */

extern TLentry tlTable[];


/*----------------------------------------------------------------------*/
/*      	             m o u n t 					*/
/*									*/
/* Mount a translation layer						*/
/*									*/
/* Parameters:                                                          */
/*	volNo		: Volume no.					*/
/*	tl		: Where to store translation layer methods	*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

FLStatus flMount(unsigned volNo, TL *tl, FLFlash *flash)
{
  FLFlash *volForCallback;
  FLSocket *socket = flSocketOf(volNo);
  FLStatus status = flUnknownMedia;
  int iTL;

  checkStatus(flIdentifyFlash(socket,flash));

  for (iTL = 0; iTL < noOfTLs && status != flOK; iTL++)
    status = tlTable[iTL].mountRoutine(flash,tl, &volForCallback);

  volForCallback->setPowerOnCallback(volForCallback);

  return status;
}


#ifdef FORMAT_VOLUME

/*----------------------------------------------------------------------*/
/*      	             f o r m a t 				*/
/*									*/
/* Formats the Flash volume						*/
/*									*/
/* Parameters:                                                          */
/*	volNo		: Volume no.					*/
/*	formatParams	: Address of FormatParams structure to use	*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

FLStatus flFormat(unsigned volNo, FormatParams FAR1 *formatParams)
{
  FLFlash flash;
  FLSocket *socket = flSocketOf(volNo);
  FLStatus status = flUnknownMedia;
  int iTL;

  checkStatus(flIdentifyFlash(socket,&flash));

  for (iTL = 0; iTL < noOfTLs && status == flUnknownMedia; iTL++)
    status = tlTable[iTL].formatRoutine(&flash,formatParams);

  return status;
}

#endif


