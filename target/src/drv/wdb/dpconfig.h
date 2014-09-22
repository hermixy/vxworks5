/*
**  File:   dpconfig.h  
**  
**  Description:
**  This file contains configuration information which needs to be ported
**  to new platforms running the netrom demo code.  Sections requiring
**  porting are marked with:
**		***	PORT THIS SECTION	***
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
*/


#ifndef _dpconfig_h
#define	_dpconfig_h

/* Sizes of common rom types */
#define	RS_27C010	(128 * 1024L)
#define	RS_27C020	(256 * 1024L)
#define	RS_27C040	(512 * 1024L)

/* Generic macros */
#define	True		1
#define	False		0

/* 
 * This section contains configuration information for the dualport protocols.
 * These parameters essentially become inputs to the config_dpram() call
 * which initializes the target side of the protocols.  The semantics of 
 * these parameters are:
 *
 *  ROM_START      is the 32-bit target address of the start of the ROM
 *                 words containing dualport memory.  This is *not*
 *                 the address of dualport memory itself, but is the base
 *                 address of the rom word containing pod 0.
 * 
 *  ROMWORDWIDTH   is the number of bytes making up a 'word'.  The
 *                 value describes the memory sybsystem of the target,
 *                 not the word size of the CPU.  For example,
 *                 if ROM memory is organized as an array of 16 bit
 *                 words, this number is two.  If the processor has a
 *                 32-bit architecture, but only one 8-bit ROM is
 *                 used to provide ROM memory, this value is 1.
 *
 *  POD_0_INDEX    is the index within a rom word of pod 0.  This number
 *                 reflects the ROM byte emulated by pod 0.  The following
 *                 table shows what value to use for common word sizes
 *                 and podorder environment variable:
 *                      
 *
 *                          Word size      podorder        POD_0_INDEX
 *                          --------       --------        -----------
 *                           8-bits     (any podorder)         0
 *                          16-bits     podorder == 0:1        0
 *                                      podorder == 1:0        1
 *                          32-bits     podorder == 0:1:2:3    0
 *                                      podorder == 3:2:1:0    3
 *
 *  NUMACCESSES    is the number of accesses the target processor makes
 *                 to pod 0 in the course of reading a single 8-bit
 *                 value from the pod.
 *
 *  ROMSIZE        is the number of bytes in an individual ROM.  This
 *                 number should agree with NetROM's romtype environment
 *                 variable.
 *
 *  DP_BASE        is the 32-bit target address of the first byte of dualport
 *                 memory.  Dualport memory is always in pod 0.  The default
 *                 location is 2K from the end of pod 0.  The formula below
 *                 is correct for this default location.  If you have moved
 *                 dualport RAM, you must adjust the formula to give the
 *                 correct address.  This define should be the first argument
 *                 in your call to config_dpram().
 *
 * Consult the NetROM user's manual for more information on the readaddress
 * protocol and on the parameters to config_dpram().
 *
 */

/*		***	PORT THIS SECTION	***
 */

#if 0
#define	ROM_START	 0x00000000L 
#define	ROMWORDWIDTH     2
#define	POD_0_INDEX	 0
#define	NUMACCESSES	 1
#define	ROMSIZE		 RS_27C020
/* The following formula is correct ONLY if dualport is at the default 
   location, at the top of pod 0 */
#define DP_BASE (ROM_START + ((ROMSIZE - DUALPORT_SIZE) * ROMWORDWIDTH))
#endif

#define	HASCACHE	 False 

#define READONLY_TARGET 
/* #define READWRITE_TARGET */

#define VETHER

#define USE_MSGS

#endif	/* _dpconfig_h */

