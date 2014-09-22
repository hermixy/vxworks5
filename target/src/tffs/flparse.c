
/*
 * $Log:   P:/user/amir/lite/vcs/flparse.c_v  $
 * 
 *    Rev 1.4   10 Sep 1997 16:29:18   danig
 * Got rid of generic names
 * 
 *    Rev 1.3   20 Jul 1997 17:16:38   amirban
 * Get rid of warnings
 * 
 *    Rev 1.2   07 Jul 1997 15:21:42   amirban
 * Ver 2.0
 * 
 *    Rev 1.1   18 Aug 1996 13:48:16   amirban
 * Comments
 * 
 *    Rev 1.0   03 Jun 1996 16:21:46   amirban
 * Initial revision.
 */

/************************************************************************/
/*                                                                      */
/*		FAT-FTL Lite Software Development Kit			*/
/*		Copyright (C) M-Systems Ltd. 1995-1996			*/
/*									*/
/************************************************************************/



#include "fatlite.h"

#ifdef PARSE_PATH


/*----------------------------------------------------------------------*/
/*		         f l P a r s e P a t h				*/
/*									*/
/* Converts a DOS-like path string to a simple-path array.		*/
/*                                                                      */
/* Parameters:                                                          */
/*	ioreq->irData	: address of path string to convert		*/
/*	ioreq->irPath	: address of array to receive parsed-path	*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

FLStatus flParsePath(IOreq FAR2 *ioreq)
{
  char FAR1 *dosPath;

  FLSimplePath FAR1 *sPath = ioreq->irPath;

  unsigned offset = 0, dots = 0, chars = 0;
  FLBoolean isExt = FALSE;
  for (dosPath = (char FAR1 *) ioreq->irData; ; dosPath++) {
    if (*dosPath == '\\' || *dosPath == 0) {
      if (offset != 0) {
	while (offset < sizeof(FLSimplePath))
	  sPath->name[offset++] = ' ';
	if (chars == 0) {
	  if (dots == 2)
	    sPath--;
	}
	else
	  sPath++;
	offset = dots = chars = 0;
	isExt = FALSE;
      }
      sPath->name[offset] = 0;
      if (*dosPath == 0)
	break;
    }
    else if (*dosPath == '.') {
      dots++;
      while (offset < sizeof sPath->name)
	sPath->name[offset++] = ' ';
      isExt = TRUE;
    }
    else if (offset < sizeof(FLSimplePath) &&
	     (isExt || offset < sizeof sPath->name)) {
      chars++;
      if (*dosPath == '*') {
	while (offset < (isExt ? sizeof(FLSimplePath) : sizeof sPath->name))
	  sPath->name[offset++] = '?';
      }
      else if (*dosPath >= 'a' && *dosPath <= 'z')
	sPath->name[offset++] = *dosPath - ('a' - 'A');
      else
	sPath->name[offset++] = *dosPath;
    }
  }

  return flOK;
}

#endif /* PARSE_PATH */


