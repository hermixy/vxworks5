/*
**  File:  dptarget.h     
**  Description:
**	This file contains 'C' language definitions to ease the use of
**	the dualport RAM features of the NetROM.
**
**      Copyright (c) 1996 Applied Microsystems Corp.
**                          All Rights Reserved
**
** Redistribution and use in source and binary forms are permitted 
** provided that the above copyright notice and this paragraph are 
** duplicated in all such forms and that any documentation,
** advertising materials, and other materials related to such
** distribution and use acknowledge that the software was developed 
** by Applied Microsystems Corp. (the Company).  The name of the 
** Company may not be used to endorse or promote products derived
** from this software without specific prior written permission. 
** THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR 
** IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
** WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE. 
**
**  Modification History:
**	 8-21-95 sch Change READ_ and WRITE_POD so they don't do
**                   multiplies.  Speeds things up a bit and some
**                   29Ks don't have a multiply instruction.
**
*/

#ifndef _dptarget_h
#define	_dptarget_h

/* target-native data storage */
typedef unsigned long   uInt32;		/* 32 bits unsigned */
typedef unsigned short  uInt16;		/* 16 bits unsigned */
typedef short           Int16;		/* 16 bits signed */
typedef unsigned char   uChar;		/* 8 bits unsigned */

/* prevents private stuff from appearing in the link map */
#define	STATIC	static

/* macro to allow other processes to run in a multitasking system */
#ifdef vxworks
#include "taskLib.h"
#define YIELD_CPU() taskDelay(1) /* closest thing in VxWorks */
#else
#define YIELD_CPU()
#endif

/* Size (in bytes) of the ram-based routine used by read-only target
 * systems to communicate with NetROM while NetROM sets emulation
 * memory.  See the routine ra_setmem_sendval().  */
#define RA_SETMEM_RTN_SIZE	512

/* dualport access macros */

#define	READ_POD(cp, addr) \
  (* ((volatile uChar *) ((cp)->dpbase + ((cp)->width * (addr)) + (cp)->index)))
#define	WRITE_POD(cp, addr, val) \
  (* ((volatile uChar *) ((cp)->dpbase + ((cp)->width * (addr)) + (cp)->index))) = (val)

/* include target-independent stuff */
#include "wdb/dualport.h"

#endif	/* _dptarget_h */








