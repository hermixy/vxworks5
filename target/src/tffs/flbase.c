/*
 * $Log:   P:/user/amir/lite/vcs/flbase.c_v  $
 * 
 *    Rev 1.2   06 Oct 1997 19:42:18   danig
 * Changed LEushort\long & Unaligned\4 to arrays
 * 
 *    Rev 1.1   10 Sep 1997 16:32:08   danig
 * Got rid of generic names
 * 
 *    Rev 1.0   28 Aug 1997 16:42:00   danig
 * Initial revision.
 */

/************************************************************************/
/*                                                                      */
/*		FAT-FTL Lite Software Development Kit			*/
/*		Copyright (C) M-Systems Ltd. 1995-1997			*/
/*									*/
/************************************************************************/

#include "flbase.h"

#ifdef TFFS_BIG_ENDIAN

/*----------------------------------------------------------------------*/
/*         Little / Big - Endian Conversion Routines			*/
/*----------------------------------------------------------------------*/

void toLEushort(unsigned char FAR0 *le, unsigned n)
{
  le[1] = (unsigned char)(n >> 8);
  le[0] = (unsigned char)n;
}


unsigned short fromLEushort(unsigned char const FAR0 *le)
{
  return ((short)le[1] << 8) + le[0];
}


void toLEulong(unsigned char FAR0 *le, unsigned long n)
{
  le[3] = (unsigned char)(n >> 24);
  le[2] = (unsigned char)(n >> 16);
  le[1] = (unsigned char)(n >> 8);
  le[0] = (unsigned char)n;
}


unsigned long fromLEulong(unsigned char const FAR0 *le)
{
  return ((long)le[3] << 24) +
	 ((long)le[2] << 16) +
	 ((long)le[1] << 8) +
	 le[0];
}

extern void copyShort(unsigned char FAR0 *to, unsigned char const FAR0 *from)
{
  to[0] = from[0];
  to[1] = from[1];
}

extern void copyLong(unsigned char FAR0 *to, unsigned char const FAR0 *from)
{
  to[0] = from[0];
  to[1] = from[1];
  to[2] = from[2];
  to[3] = from[3];
}


#else

void toUNAL(unsigned char FAR0 *unal, unsigned n)
{
  unal[1] = (unsigned char)(n >> 8);
  unal[0] = (unsigned char)n;
}


unsigned short fromUNAL(unsigned char const FAR0 *unal)
{
  return ((short)unal[1] << 8) + unal[0];
}


void toUNALLONG(Unaligned FAR0 *unal, unsigned long n)
{
  toUNAL(unal[0],(unsigned short) n);
  toUNAL(unal[1],(unsigned short) (n >> 16));
}


unsigned long fromUNALLONG(Unaligned const FAR0 *unal)
{
  return fromUNAL(unal[0]) +
	 ((unsigned long) fromUNAL(unal[1]) << 16);
}

#endif /* TFFS_BIG_ENDIAN */

int     tffscmpWords
        (
        void *buf1,                     /* first buffer to compare */
        void *buf2,                     /* second buffer */
        int   nbytes                    /* length in bytes */
        )
        {
        short *b1;
        short *b2;

        b1 = buf1;
        b2 = buf2;
        
        for (; nbytes > 0; nbytes -= 2)
            if (*(b1)++ != *(b2)++)
                return nbytes;
        return  0;
        }

