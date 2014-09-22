/* ffsLib.c - find first bit set library */

/* Copyright 1984-1998 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01m,13nov01,dee  add CPU_FAMILY==COLDFIRE to portable test
01l,13nov98,cdp  use portable routines for ARM CPUs with ARM_THUMB==TRUE.
01l,03mar00,zl   merged SH support into T2
01k,04may98,cym  added SIMNT to PROTABLE list.
01j,22apr97,jpd  added optimised version for ARM.
01i,28nov96,cdp  added ARM support.
01h,03apr96,ism  vxsim/solaris
01g,02dec93,pme  added Am29K family support.
01h,11aug93,gae  vxsim hppa from rrr.
01g,12jun93,rrr  vxsim.
01f,08jun92,ajm  made portable for mips, added ffsLsb routine
01e,26may92,rrr  the tree shuffle
01d,22apr92,jwt  optimized version for SPARClite; copyright to 1992.
01c,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -fixed #else and #endif
		  -changed copyright notice
01b,25sep91,yao   added suport for CPU32.
01a,21jan91,jcf   written.
*/

/*
DESCRIPTION
This library contains routines to find the first bit set in a 32 bit field.
It is utilized by bit mapped priority queues and hashing functions.
*/

#include "vxWorks.h"


/* optimized version available for MC680xx, COLDFIRE and ARM families */


#if (defined(PORTABLE) || \
     (CPU_FAMILY == SIMNT) || (CPU_FAMILY == SPARC) || (CPU_FAMILY == MIPS) || \
     (CPU_FAMILY == SIMSPARCSUNOS) || (CPU_FAMILY == SIMHPPA) || \
     (CPU_FAMILY == AM29XXX) || (CPU_FAMILY == SIMSPARCSOLARIS) || \
     ((CPU_FAMILY == ARM) && ARM_THUMB))
#define ffsLib_PORTABLE
#endif

#if ((CPU == SPARClite) || (CPU_FAMILY == SH))
#undef	PORTABLE
#undef	ffsLib_PORTABLE
#endif

#if (defined(ffsLib_PORTABLE)||(CPU == MC68000)||(CPU == MC68010)||(CPU==CPU32) || \
	       (CPU_FAMILY == ARM) || (CPU_FAMILY == COLDFIRE))
#define ffsLib_FFS_TABLE
#endif

#ifdef ffsLib_FFS_TABLE

UINT8 ffsMsbTbl [256] =			/* lookup table for ffsMsb() */
    {
    0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    };

UINT8 ffsLsbTbl [256] =                 /* lookup table for ffsLsb() */
    {
    0, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    7, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    };

#endif	/* ffsLib_FFS_TABLE */

#ifdef ffsLib_PORTABLE

/*******************************************************************************
*
* ffsMsb - find most significant bit set
*
* This routine finds the first bit set in the 32 bit argument passed it and
* returns the index of that bit.  Bits are numbered starting at 1 from the
* least signifficant bit.  A return value of zero indicates that the value
* passed is zero.
*
* RETURNS: most significant bit set
*/

int ffsMsb
    (
    UINT32 i        /* argument to find first set bit in */
    )
    {
    UINT16 msw = (UINT16) (i >> 16);		/* most significant word */
    UINT16 lsw = (UINT16) (i & 0xffff);		/* least significant word */
    UINT8  byte;

    if (i == 0)
	return (0);

    if (msw)
	{
	byte = (UINT8) (msw >> 8);		/* byte is bits [24:31] */
	if (byte)
	    return (ffsMsbTbl[byte] + 24 + 1);
	else
	    return (ffsMsbTbl[(UINT8) msw] + 16 + 1);
	}
    else
	{
	byte = (UINT8) (lsw >> 8);		/* byte is bits [8:15] */
	if (byte)
	    return (ffsMsbTbl[byte] + 8 + 1);
	else
	    return (ffsMsbTbl[(UINT8) lsw] + 1);
	}
    }
/*******************************************************************************
*
* ffsLsb - find least significant bit set
*
* This routine finds the first bit set in the 32 bit argument passed it and
* returns the index of that bit.  Bits are numbered starting at 1 from the
* least signifficant bit.  A return value of zero indicates that the value
* passed is zero.
*
* RETURNS: least significant bit set
*/

int ffsLsb
    (
    UINT32 i        /* argument to find first set bit in */
    )
    {
    UINT16 msw = (UINT16) (i >> 16);		/* most significant word */
    UINT16 lsw = (UINT16) (i & 0xffff);		/* least significant word */
    UINT8  byte;

    if (i == 0)
	return (0);

    if (lsw)
	{
	byte = (UINT8) (lsw & 0xff);
	if (byte)				/* byte is bits [0:7] */
	    return (ffsLsbTbl[byte] + 1);
	else					/* byte is bits [8:15] */
	    return (ffsLsbTbl[(UINT8) (lsw >> 8)] + 8 + 1);
	}
    else
	{
	byte = (UINT8) (msw & 0xff);		/* byte is bits [16:23] */
	if (byte)
	    return (ffsLsbTbl[byte] + 16 + 1);
	else					/* byte is bits [24:31] */
	    return (ffsLsbTbl[(UINT8) (msw >> 8)] + 24 + 1);
	}
    }

#endif	/* ffsLib_PORTABLE */
