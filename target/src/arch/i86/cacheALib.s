/* cacheALib.s - i80x86 cache management assembly routines */

/* Copyright 1984-2002 Wind River Systems, Inc. */
	
/*
modification history
--------------------
01j,07mar02,hdn  added bytes checking before the CLFLUSH loop (spr 73360)
01i,04dec01,hdn  cleaned up cachePen4Flush and cachePen4Clear
01h,23aug01,hdn  added FUNC/FUNC_LABEL, replaced .align with .balign
		 added support for Pentium4.
01g,27oct94,hdn  changed NW bit not to set.
01f,27sep94,hdn  changed cacheClear to use wbinvd.
01e,29may94,hdn  changed a macro I80486 to sysProcessor.
01d,05nov93,hdn  added cacheFlush.
01c,27jul93,hdn  changed cacheClear that uses wbinvd now.
01b,08jun93,hdn  updated to 5.1.
01a,16mar93,hdn  written based on TRON version.
*/

/*
DESCRIPTION
This module contains routines to modify the i80x86 cache control registers.

SEE ALSO: "i80x86 Microprocessor User's Manual"
*/

#define _ASMLANGUAGE
#include "vxWorks.h"
#include "asm.h"
#include "regs.h"
#include "cacheLib.h"


	.data
	.globl  FUNC(copyright_wind_river)
	.long   FUNC(copyright_wind_river)


	/* internals */

	.globl	GTEXT(cacheI86Reset)
	.globl	GTEXT(cacheI86Enable)
	.globl	GTEXT(cacheI86Disable)
	.globl	GTEXT(cacheI86Lock)
	.globl	GTEXT(cacheI86Unlock)
	.globl	GTEXT(cacheI86Clear)
	.globl	GTEXT(cacheI86Flush)
	.globl	GTEXT(cachePen4Clear)
	.globl	GTEXT(cachePen4Flush)


	.text
	.balign 16

/*******************************************************************************
*
* cacheI86Reset - reset a cache by clearing it and disabling
*
* This routine resets the all caches.
*
* RETURNS: OK

* STATUS cacheI86Reset ()

*/

	.balign 16,0x90
FUNC_LABEL(cacheI86Reset)
	
	movl	%cr0,%eax
	orl	$ CR0_CD,%eax
	andl	$ CR0_NW_NOT,%eax
	movl	%eax,%cr0
	wbinvd					/* writeback and invalidate */
	ret

/*******************************************************************************
*
* cacheI86Enable - enable a cache
*
* This routine enables the specified cache.
*
* RETURNS: OK, or ERROR if cache, or control not supported.

* STATUS cacheI86Enable (cache)
*     int cache;		/* cache to enable *

*/

	.balign 16,0x90
FUNC_LABEL(cacheI86Enable)
FUNC_LABEL(cacheI86Unlock)

	movl	%cr0,%eax
	andl	$ CR0_CD_NOT,%eax
	andl	$ CR0_NW_NOT,%eax
	movl	%eax,%cr0
	ret

/*******************************************************************************
*
* cacheI86Disable - disable a cache
*
* This routine disables the specified cache.
*
* RETURNS: OK, or ERROR if cache, or control not supported.

* STATUS cacheI86Disable (cache)
*     int cache;		/* cache to disable *

*/

	.balign 16,0x90
FUNC_LABEL(cacheI86Disable)

	movl	%cr0,%eax
	orl	$ CR0_CD,%eax
	andl	$ CR0_NW_NOT,%eax
	movl	%eax,%cr0
	wbinvd					/* writeback and invalidate */
	ret

/*******************************************************************************
*
* cacheI86Lock - lock all entries in a cache
*
* This routine locks all entries in the specified cache.
*
* RETURNS: OK

* STATUS cacheI86Lock ()

*/


	.balign 16,0x90
FUNC_LABEL(cacheI86Lock)
	
	movl	%cr0,%eax
	andl	$ CR0_NW_NOT,%eax
	orl	$ CR0_CD,%eax
	movl	%eax,%cr0
	ret

/*******************************************************************************
*
* cacheI86Clear - clear all entries in a cache
*
* This routine clear all entries in the specified cache.
*
* RETURNS: OK, or ERROR if cache, or control not supported.

* STATUS cacheI86Clear (cache)
*     int cache;		/* cache to clear *

*/

	.balign 16,0x90
FUNC_LABEL(cacheI86Clear)
	
	wbinvd					/* writeback and invalidate */
	ret

/*******************************************************************************
*
* cacheI86Flush - flush all entries in a cache
*
* This routine flush all entries in the specified cache.
*
* RETURNS: OK, or ERROR if cache, or control not supported.

* STATUS cacheI86Flush (cache)
*     int cache;		/* cache to clear *

*/

	.balign 16,0x90
FUNC_LABEL(cacheI86Flush)
	
	wbinvd					/* writeback and invalidate */
	ret

/*******************************************************************************
*
* cachePen4Clear - clear specified entries in the cache for Pentium4
*
* This routine clear specified entries in the cache for Pentium4.
*
* RETURNS: OK, or ERROR if cache, or control not supported.

* STATUS cachePen4Clear (CACHE_TYPE cache, void * addr, size_t bytes)
*     CACHE_TYPE cache;		/* cache to clear *
*     void * addr;		/* address to clear *
*     size_t bytes;		/* bytes to clear *

*/

	/* see below */	

/*******************************************************************************
*
* cachePen4Flush - flush specified entries in the cache for Pentium4
*
* This routine flush specified entries in the cache for Pentium4.
* This routine uses following method assuming that cacheFlushBytes
* is power of 2 (2**n).
*
*   bytes += (address % cacheFlushBytes);
*   if ((rem = bytes % cacheFlushBytes) != 0)
*     bytes += (cacheFlushBytes - rem);
*   loopCount = bytes / cacheFlushBytes;
*   address -= (address % cacheFlushBytes);
*   do {
*     CLFLUSH (address);
*     address += cacheFlushBytes;
*   } while (loopCount--);
*
* INTERNAL
* CLFLUSH instruction invalidates the cache line that contains
* the linear address specified with the source operand from all
* levels of the processor cache hierarchy (data and instruction).
* The validation is broadcast throughout the cache coherence 
* domain.  If, at any level of the cache hierarchy, the line is
* inconsistent with memory (dirty) it is written to memory before
* invalidation.  The source operand is a byte memory location.
*
* RETURNS: OK, or ERROR if cache, or control not supported.

* STATUS cachePen4Flush (CACHE_TYPE cache, void * addr, size_t bytes)
*     CACHE_TYPE cache;		/* cache to clear *
*     void * addr;		/* address to flush *
*     size_t bytes;		/* bytes to flush *

*/

	.balign 16,0x90
FUNC_LABEL(cachePen4Clear)
FUNC_LABEL(cachePen4Flush)
	
	movl	SP_ARG2(%esp), %eax		/* get address in EAX */
	movl	SP_ARG3(%esp), %edx		/* get bytes in EDX */
	cmpl	$0, %edx			/* return if (bytes == 0) */
	jz	cachePen4Ret
	movl	%edx, %ecx
	andl	$ ~CLFLUSH_MAX_BYTES, %ecx	/* WBINVD if (bytes > MAX) */
	jnz	FUNC(cacheI86Flush)

	pushl	%ebx				/* save EBX */
	pushl	%esi				/* save ESI */
	pushl	%edi				/* save EDI */
	movl	%eax, %esi			/* get address in ESI */
	movl	%edx, %edi			/* get bytes in EDI */
	movl	FUNC(cacheFlushBytes), %ebx	/* get flushBytes in EBX */

	/* bytes += (address % cacheFlushBytes); */	

	movl	%ebx, %edx			/* get flushBytes in EDX */
	subl	$1, %edx			/* create lowerbit mask */
	andl	%edx, %eax			/* get the lowerbit rem */
	addl	%eax, %edi			/* add the rem to bytes */

	/* if ((rem = bytes % cacheFlushBytes) != 0)	*/
	/*   bytes += (cacheFlushBytes - rem);		*/

	movl	%edi, %ecx			/* get bytes in ECX */
	andl	%edx, %ecx			/* get the lowerbit rem */
	jz	cachePen4Flush0			/* skip if (rem == 0) */
	subl	%ecx, %edi			/* sub the rem from bytes */
	addl	%ebx, %edi			/* add the flushBytes */ 

	/* loopCount = bytes / cacheFlushBytes; */

cachePen4Flush0:
	bsfl	%ebx, %ecx			/* find the LSB, ECX=[0-31] */
	shrl	%cl, %edi			/* shift right ECX bit */

	/* address -= (address % cacheFlushBytes); */

	movl	%esi, %eax			/* get address in EAX */
	xorl	$0xffffffff, %edx		/* create the upperbit mask */
	andl	%edx, %eax			/* get the upperbit */

	/* do {					*/
	/*   CLFLUSH (address);			*/
	/*   address += cacheFlushBytes;	*/
	/* } while (loopCount--);		*/

	movl	%edi, %ecx			/* set the loopCount */
cachePen4FlushLoop:
	/* clflush (%eax)                        * flush the line */
	.byte	0x0f, 0xae, 0x38		/* mod=00b reg=111b r/m=000b */
	addl	%ebx, %eax			/* address += flushBytes */
	loop	cachePen4FlushLoop		/* loop if (ECX-- != 0) */

	popl	%edi				/* restore EDI */
	popl	%esi				/* restore ESI */
	popl	%ebx				/* restore EBX */

cachePen4Ret:
	ret

