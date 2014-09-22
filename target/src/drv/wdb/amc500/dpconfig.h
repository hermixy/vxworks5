/*
** File:     dpconfig.h
** Version:  1.0.1
**
** Copyright 1996 Applied Microsystems, Inc.  All rights reserved.
**
** This file contains configuration information which needs to be ported
** to new platforms running the netrom dualport communication code.  Sections 
** requiring porting are marked with:
**
**		***	PORT THIS SECTION	***
*/
#ifndef _dpconfig_h
#define	_dpconfig_h

/* Generic macros */
#define	True		1
#define	False		0

/* 
** This section contains configuration information for the dualport protocols.
** These parameters essentially become inputs to the config_dpram() call
** which initializes the target side of the protocols.  The semantics of 
** these parameters are:
**
**  ROM_START      is the 32-bit target address of the start of the ROM
**                 words containing dualport memory.  This is**not*
**                 the address of dualport memory itself, but is the base
**                 address of the rom word containing pod 0.
** 
**  ROMWORDWIDTH   is the number of bytes making up a 'word'.  The
**                 value describes the memory sybsystem of the target,
**                 not the word size of the CPU.  For example,
**                 if ROM memory is organized as an array of 16 bit
**                 words, this number is two.  If the processor has a
**                 32-bit architecture, but only one 8-bit ROM is
**                 used to provide ROM memory, this value is 1.
**
**  POD_0_INDEX    is the index within a rom word of pod 0.  This number
**                 reflects the ROM byte emulated by pod 0.  The following
**                 table shows what value to use for common word sizes
**                 and podorder environment variable:
**                      
**
**                          Word size      podorder        POD_0_INDEX
**                          --------       --------        -----------
**                           8-bits     (any podorder)         0
**                          16-bits     podorder == 0:1        0
**                                      podorder == 1:0        1
**                          32-bits     podorder == 0:1:2:3    0
**                                      podorder == 3:2:1:0    3
**
**  NUMACCESSES    is the number of accesses the target processor makes
**                 to pod 0 in the course of reading a single 8-bit
**                 value from the pod.
**
**  ROMSIZE        is the number of bytes in an individual ROM.  This
**                 number should agree with NetROM's romtype environment
**                 variable.
**
**  DP_BASE        is the 32-bit target address of the first byte of dualport
**                 memory.  Dualport memory is always in pod 0.  The default
**                 location is 2K from the end of pod 0.  The formula below
**                 is correct for this default location.  If you have moved
**                 dualport RAM, you must adjust the formula to give the
**                 correct address.  This define should be the first argument
**                 in your call to config_dpram().
**
** Consult the NetROM user's manual for more information on the readaddress
** protocol and on the parameters to config_dpram().
**
*/

/* Sizes of common rom types */
#define RS_64K         (64  * 1024L)
#define RS_128K	       (128 * 1024L)
#define RS_256K        (256 * 1024L)
#define RS_512K        (512 * 1024L)
#define RS_1MEG        (1   * 1048576L)
#define RS_2MEG        (2   * 1048576L)
#define RS_4MEG        (4   * 1048576L)

#define	RS_27C010	RS_128K
#define	RS_27C020	RS_256K
#define	RS_27C040	RS_512K
#define	RS_27C080	RS_1MEG

/*---------------------------------------------------------------*/
/*		***	PORT THIS SECTION      ***               */
/*---------------------------------------------------------------*/

#define	VxWorks
#ifndef	VxWorks

#define ROMSTART        0xFFF00000L 
#define ROMWORDWIDTH     2
#define POD_0_INDEX      0
#define ROMSIZE          RS_27C020


/* MAX_WAIT_FTN_SIZE is used on read-only targets only.
** The wait_ftn() must execute from RAM while NetROM sets Podmem.
** MAX_WAIT_FTN_SIZE is the amount of memory to allocate in RAM
** for the wait_ftn().  nr_ConfigDP() copies wait_fnt() to RAM.
** If too little memory is set aside, nr_ConfigDP() will return
** with an error.  If extra memory is set aside, no problem...
** the nr_ConfigDP() only copies the minimum amount to RAM.
**
** Note: Using an MRI compiler w/ an i960 target,
**
** sizeof(wait_ftn()) = _nr_WaitEnd  -  _nr_Wait
**                    =  0xE0051400  -  0xE00514F0
**                    = 0xF0
*/
#define MAX_WAIT_FTN_SIZE  0x200

/* The following formula is correct ONLY if dualport is at the default 
** location, at the top of pod 0.
** If you move dualport RAM to somewhere else, redefine DP_BASE !!!
*/
#define DP_BASE (ROMSTART + ((ROMSIZE - DUALPORT_SIZE) * ROMWORDWIDTH))

/* The following #define locates DP_BASE at the beginning of Pod 0 */
/* #define DP_BASE (ROMSTART) */

/* Do NOT modify ROMEND */
#define ROMEND  (ROMSTART + ROMSIZE * ROMWORDWIDTH)

/* If your target CAN write to the memory emulated by NetROM,
** define READONLY_TARGET as False;
** If your target CANNOT write to the memory emulated by NetROM,
**   define READONLY_TARGET as True
*/
#define READONLY_TARGET True

/* Set to True  if your target is little-endian, for example, Intel
** Set to False if your target is big-endian,    for example, Motorola
*/
#define LITTLE_ENDIAN_TGT True 
    /* Big-endian / little-endian conversion routine */

#if(LITTLE_ENDIAN_TGT == True )
  #define swap32(x) \
    (( (long)(x) & 0x000000FF) << 24)  +  \
    (( (long)(x) & 0x0000FF00) << 8 )  +  \
    (( (long)(x) & 0x00FF0000) >> 8 )  +  \
    (( (long)(x) & 0xFF000000) >> 24) 
  #define swap16(x) \
    (( (int)(x) & 0x00FF) << 8 ) + (( (int)(x) & 0xFF00) >> 8 )
#else
  #define swap32(x) x
  #define swap16(x) x
#endif  /* LITTLE_ENDIAN_TGT */

/* Define VETHER only if you are using Virtual Ethernet */
/* #define VETHER */

/* macro to allow other processes to run in a multitasking system */
/* If you are NOT using vxWorks, define YIELD_CPU for your RTOS */
#ifdef vxworks
#include "taskLib.h"
#define nr_YieldCPU() taskDelay(1) /* closest thing in VxWorks */
#else
#define nr_YieldCPU()
#endif

/* Macros to turn interrupts ON and OFF 
** Required for nr_SetMem() which MUST be atomic. In multitasking 
** systems, unless your code assures that the channel will not be
** written to by another function while nr_SetMem() is running, you'll
** need to define these macros.  These macros are
** target-specific, you must supply them.
*/
#define nr_InterruptsOFF()
#define nr_InterruptsON() 

/* The following three macros control target caching.  They are
** required for dualport communication protocol on targets that
** cache the memory region emulated by NetROM. 
**
** Cache-related macros:
**   nr_HasCache      Boolean; equals True if your target caches
**                    the memory emulated by NetROM, False otherwise.
**   nr_DataCacheOff  Turns caching OFF--you must supply this for your target
**   nr_DataCacheOn   Restores caching--you must supply this for your target
*/
#define	nr_HasCache       False /* True */
#define nr_DataCacheOff()
#define nr_DataCacheOn()

#else	/* VxWorks */

/* VxWorks - specific definitions */

extern int wdbNetromKey;	/* interrupt lockout key */

#define ROMSTART	ROM_BASE_ADRS
#define ROMWORDWIDTH	WDB_NETROM_WIDTH
#define POD_0_INDEX	WDB_NETROM_INDEX
#define ROMSIZE         WDB_NETROM_ROMSIZE
#define DP_BASE		(ROMSTART + ((ROMSIZE - DUALPORT_SIZE) * ROMWORDWIDTH))
#define ROMEND		(ROMSTART + ROMSIZE * ROMWORDWIDTH)

#define MAX_WAIT_FTN_SIZE  0x200

#define READONLY_TARGET True

#if	(_BYTE_ORDER == _LITTLE_ENDIAN)
  #define LITTLE_ENDIAN_TGT True 
  #define swap32(x) \
    (( (long)(x) & 0x000000FF) << 24)  +  \
    (( (long)(x) & 0x0000FF00) << 8 )  +  \
    (( (long)(x) & 0x00FF0000) >> 8 )  +  \
    (( (long)(x) & 0xFF000000) >> 24) 
  #define swap16(x) \
    (( (int)(x) & 0x00FF) << 8 ) + (( (int)(x) & 0xFF00) >> 8 )
#else
  #define LITTLE_ENDIAN_TGT False
  #define swap32(x) x
  #define swap16(x) x
#endif

#define nr_YieldCPU()

#define nr_InterruptsOFF()	{wdbNetromLock = intLock();}
#define nr_InterruptsON() 	{intUnlock (wdbNetromLock);}

#define	nr_HasCache       False /* True */
#define nr_DataCacheOff()
#define nr_DataCacheOn()

#endif	/* VxWorks */

#endif	/* _dpconfig_h */
