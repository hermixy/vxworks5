/* usbPlatform.h - Basic platform definitions for USB driver stack */

/* Copyright 2000 Wind River Systems, Inc. */

/*
Modification history
--------------------
01e,18sep01,wef  merge from wrs.tor2_0.usb1_1-f for veloce
01d,05dec00,wef  removed M_usb* and put in vwModNum.h
01c,12jan00,rcb  Add definition for M_usbSpeakerLib.
01b,26may99,rcb  Add definition of VOLATILE.
01a,31may99,rcb  First.
*/

/*
DESCRIPTION

This file contains basic platform definitions used to write portable (O/S-
independent) code.  

By convention, this should be the only file in a set of "portable" code which
contains #ifdef's and other constructs used to accomodate machine or O/S
dependencies.
*/


#ifndef __INCusbPlatformh
#define __INCusbPlatformh

#ifdef	__cplusplus
extern "C" {
#endif


/*
 * Structure packing
 *
 * By convention, all code wants "zero-byte packing".  That is, the compiler
 * should assemble structures so that fields are contiguous in memory...no
 * padding should be added.  CMC's use of this convention was approved by
 * D'Anne Thompson of WRS on 09jun99.
 */


/* 
 * Basic data types
 *
 * vxWorks.h defines UINT8, UINT16, UINT32, BOOL, VOID, and STATUS.
 *
 * vxWorks.h defines min(x,y), max(x,y).
 *
 * vxWorks.h defines _BIG_ENDIAN, _BYTE_ORDER, MSB, LSB, LONGSWAP.
 *
 * vxWorks.h defines OK, ERROR.
 */

#include "vxWorks.h"

/* pointers to data types */

typedef UINT8 *pUINT8;
typedef UINT16 *pUINT16;
typedef UINT32 *pUINT32;
typedef BOOL *pBOOL;
typedef char *pCHAR;
typedef VOID *pVOID;


/* VOLATILE */

#define VOLATILE    volatile


/* Macros to deal with byte order issues */

#if (_BYTE_ORDER == _BIG_ENDIAN)

#define FROM_LITTLEW(w) 	(MSB((w)) | (LSB((w)) << 8))
#define FROM_LITTLEL(l) 	(LONGSWAP((l)))
#define FROM_BIGW(w)		(w)
#define FROM_BIGL(l)		(l)

#else

#define FROM_LITTLEW(w) 	(w)
#define FROM_LITTLEL(l) 	(l)
#define FROM_BIGW(w)		(MSB((w)) | (LSB((w)) << 8))
#define FROM_BIGL(l)		(LONGSWAP((l)))

#endif


#define TO_LITTLEW(w)		FROM_LITTLEW((w))
#define TO_LITTLEL(l)		FROM_LITTLEL((l))
#define TO_BIGW(w)		FROM_BIGW((w))
#define TO_BIGL(w)		FROM_BIGL((l))


#ifdef	__cplusplus
}
#endif

#endif	/* __INCusbPlatformh */


/* End of file. */

