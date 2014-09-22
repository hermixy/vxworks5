/* ffsALib.s - i80x86 find first set function */

/* Copyright 1984-2001 Wind River Systems, Inc. */

/*
modification history
--------------------
01d,22aug01,hdn  imported from T31 ver 01g
01c,01jun93,hdn  updated to 5.1.
		  - fixed #else and #endif
		  - changed VOID to void
		  - changed ASMLANGUAGE to _ASMLANGUAGE
		  - changed copyright notice
01b,13oct92,hdn  debugged.
01a,07apr92,hdn  written based on TRON version.
*/

/*
DESCRIPTION
This module defines an optimized version of the C routine in ffsLib.c.
By taking advantage of the BSR instruction of 80X86 processors, the
implementation determines the first bit set in constant time.
*/

#define _ASMLANGUAGE
#include "vxWorks.h"
#include "asm.h"


	.data
	.globl  FUNC(copyright_wind_river)
	.long   FUNC(copyright_wind_river)


#if defined(PORTABLE)
#define ffsALib_PORTABLE
#endif

#ifndef ffsALib_PORTABLE

	/* internals */

	.globl	GTEXT(ffsMsb)
	.globl	GTEXT(ffsLsb)


	.text
	.balign 16

/*******************************************************************************
*
* ffsMsb - find first set bit (searching from the most significant bit)
*
* This routine finds the first bit set in the argument passed it and
* returns the index of that bit.  Bits are numbered starting
* at 1 from the least significant bit to 32 the most significant bit.
* A return value of zero indicates that the value passed is zero.
*
* RETURNS: bit position from 1 to 32, or 0 if the argument is zero.

* int ffsMsb (i)
*     int i;       /* argument to find first set bit in *

*/

FUNC_LABEL(ffsMsb)
	movl	SP_ARG1(%esp),%edx		/* %edx = i */
	bsrl	%edx,%eax			/* search bit from 31 */
	je	ffsNoBitSet 			/* zeros means no bit is set */
	incl	%eax				/* found it, increment 1 */
	ret

ffsNoBitSet:					/* couldn't find it */
	xorl	%eax,%eax			/* return 0 */
	ret

/*******************************************************************************
*
* ffsLsb - find first set bit (searching from the least significant bit)
*
* This routine finds the first bit set in the argument passed it and
* returns the index of that bit.  Bits are numbered starting
* at 1 from the least significant bit to 32 the most significant bit.
* A return value of zero indicates that the value passed is zero.
*
* RETURNS: bit position from 1 to 32, or 0 if the argument is zero.

* int ffsLsb (i)
*     int i;       /* argument to find first set bit in *

*/

	.balign 16,0x90
FUNC_LABEL(ffsLsb)
	movl	SP_ARG1(%esp),%edx		/* %edx = i */
	bsfl	%edx,%eax			/* search bit from 0 */
	je	ffsNoBitSet 			/* zeros means no bit is set */
	incl	%eax				/* found it, increment 1 */
	ret

#endif /* ! ffsALib_PORTABLE */
