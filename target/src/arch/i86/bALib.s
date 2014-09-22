/* bALib.s - i80x86 buffer manipulation library assembly routines */

/* Copyright 1984-2001 Wind River Systems, Inc. */

/*
modification history
--------------------
01d,22aug01,hdn  added FUNC/FUNC_LABEL, replaced .align with .balign
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
This library contains optimized versions of the routines in bLib.c
for manipulating buffers of variable-length byte arrays.

NOMANUAL

INTERNAL

i80386 transfer bus cycles for bytes, words, dwords.
 -------------------------------------+----+----------------+---------------+
  byte length of logical operand      	 1         2                4
 -------------------------------------+----+----------------+---------------+
  physical byte address in memory      	xx   00  01  10  11   00  01  10  11
 -------------------------------------+----+----------------+---------------+
  transfer cycles over 32bits data bus	 b    w   w   w  hb    d  hb  hw  h3
					                 lb       l3  lw  lb
 -------------------------------------+----+----------------+---------------+
  transfer cycles over 16bits data bus	 b    w  lb   w  hb   lw  hb  hw  mw
					         hb      lb   hw  lb  lw  hb
							 	  mw      lb
 -------------------------------------+----+----------------+---------------+
	b: byte transfer		l: low order portion
	w: word transfer		h: high order portion
	3: 3-byte transfer		m: middle order portion
	d: dword transfer		x: don't care

SEE ALSO: bLib(1), strLib(1)
*/

#define _ASMLANGUAGE
#include "vxWorks.h"
#include "asm.h"


	.data
	.globl  FUNC(copyright_wind_river)
	.long   FUNC(copyright_wind_river)


#ifndef	PORTABLE

	/* internals */

	.globl	GTEXT(bcopy)
	.globl	GTEXT(bcopyBytes)
	.globl	GTEXT(bcopyWords)
	.globl	GTEXT(bcopyLongs)
	.globl	GTEXT(bfill)
	.globl	GTEXT(bfillBytes)
	.globl	GTEXT(bfillWords)
	.globl	GTEXT(bfillLongs)


	.text
	.balign 16

/*******************************************************************************
*
* bcopy - copy one buffer to another
*
* This routine copies the first `nbytes' characters from
* `source' to `destination'.  Overlapping buffers are handled correctly.
* The copy is optimized by copying 4 bytes at a time if possible,
* (see bcopyBytes (2) for copying a byte at a time only).
*
* SEE ALSO: bcopyBytes (2)
*
* NOMANUAL - manual entry in bLib (1)

* void bcopy (source, destination, nbytes)
*     char *source;       /* pointer to source buffer      *
*     char *destination;  /* pointer to destination buffer *
*     int nbytes;         /* number of bytes to copy       *

*/

FUNC_LABEL(bcopy)
	pushl	%ebp
	movl	%esp,%ebp
	pushl	%esi
	pushl	%edi
	pushf

	/* put src in %esi, dest in %edi, and count in %edx */

	movl	ARG1(%ebp),%esi		/* source */
	movl	ARG2(%ebp),%edi		/* destination */
	movl	ARG3(%ebp),%edx		/* nbytes */

	/* Find out if there is an overlap problem.
	 * We have to copy backwards if destination lies within source,
	 * i.e. ((destination - source) > 0 && < nbytes) */

	movl	%edi,%eax		/* destination */
	subl	%esi,%eax		/* - source */
	jle	bCopyFwd0		/* <= 0 means copy forward */
	cmpl	%edx,%eax		/* compare to nbytes */
	jl	bCopyBck0		/* < nbytes means copy backwards */

bCopyFwd0:
	cld

	/* if length is less than 10, it's cheaper to do a byte copy */

	cmpl	$10,%edx		/* test count */
	jl	bCopyFwd5		/* do byte copy */

	/* If destination and source are not both odd, or both even,
	 * we must do a byte copy, rather than a long copy */

	movl	%edi,%eax
	xorl	%esi,%eax		/* %eax = destination ^ source */
	btl	$0,%eax
	jc	bCopyFwd5

	/* If the buffers are odd-aligned, copy the first 1 - 3 bytes */

	movl	%esi,%eax
	andl	$3,%eax			/* %eax has 0 - 3 */
	je	bCopyFwd1		/* if long-aligned ? */
	movl	%eax,%ecx		/* %ecx has count */
	rep	
	movsb				/* copy the bytes */
	subl	%eax,%edx		/* decrement count by %eax */

bCopyFwd1:
	/* Since we're copying 4 bytes at a crack, divide count by 4.
	 * Keep the remainder in %edx, so we can do those bytes at the 
	 * end of the loop. */

	movl	%edx,%ecx
	shrl	$2,%ecx			/* count /= 4 */
	je	bCopyFwd2		/* do the test first */

	rep
	movsl				/* copy long by long */

bCopyFwd2:
	andl	$3,%edx			/* remainder in %edx */


	/* Copy byte by byte */

bCopyFwd5:
	movl	%edx,%ecx		/* Set up %ecx as the loop ctr */
	cmpl	$0,%ecx
	je	bCopy6			/* do the test first */
	rep
	movsb				/* copy byte by byte */

	jmp	bCopy6

	/* ---------------------- copy backwards ---------------------- */

	.balign 16,0x90
bCopyBck0:
	addl	%edx,%esi
	addl	%edx,%edi
	std

	/* if length is less than 10, it's cheaper to do a byte copy */

	cmpl	$10,%edx		/* test count */
	jl	bCopyBck5		/* do byte copy */

	/* If destination and source are not both odd, or both even,
	 * we must do a byte copy, rather than a long copy */

	movl	%edi,%eax
	xorl	%esi,%eax		/* %eax = destination ^ source */
	btl	$0,%eax
	jc	bCopyBck5

	/* If the buffers are odd-aligned, copy the first 1 - 3 bytes */

	movl	%esi,%eax
	andl	$3,%eax			/* %eax has 0 - 3 */
	je	bCopyBck1		/* if long-aligned ? */
	movl	%eax,%ecx		/* %ecx has count */

	decl	%esi
	decl	%edi
	rep	
	movsb				/* copy the bytes */
	incl	%esi
	incl	%edi

	subl	%eax,%edx		/* decrement count by %eax */

bCopyBck1:
	/* Since we're copying 4 bytes at a crack, divide count by 4.
	 * Keep the remainder in %edx, so we can do those bytes at the 
	 * end of the loop. */

	movl	%edx,%ecx
	shrl	$2,%ecx			/* count /= 4 */
	je	bCopyBck2		/* do the test first */

	addl	$-4,%esi
	addl	$-4,%edi
	rep
	movsl				/* copy long by long */
	addl	$4,%esi
	addl	$4,%edi

bCopyBck2:
	andl	$3,%edx			/* remainder in %edx */


	/* Copy byte by byte */

bCopyBck5:
	movl	%edx,%ecx		/* Set up %ecx as the loop ctr */
	cmpl	$0,%ecx
	je	bCopy6			/* do the test first */

	decl	%esi
	decl	%edi
	rep
	movsb				/* copy byte by byte */

bCopy6:
	popf
	popl	%edi
	popl	%esi
	leave
	ret

/*******************************************************************************
*
* bcopyBytes - copy one buffer to another a byte at a time
*
* This routine copies the first `nbytes' characters from
* `source' to `destination'.
* It is identical to bcopy except that the copy is always performed
* a byte at a time.  This may be desirable if one of the buffers
* can only be accessed with byte instructions, as in certain byte-wide
* memory-mapped peripherals.
*
* SEE ALSO: bcopy (2)
*
* NOMANUAL - manual entry in bLib (1)

* void bcopyBytes (source, destination, nbytes)
*     char *source;       /* pointer to source buffer      *
*     char *destination;  /* pointer to destination buffer *
*     int nbytes;         /* number of bytes to copy       *

*/

	.balign 16,0x90
FUNC_LABEL(bcopyBytes)
	pushl	%ebp
	movl	%esp,%ebp
	pushl	%esi
	pushl	%edi
	pushf

	/* put src in %esi, dest in %edi, and count in %ecx */

	movl	ARG1(%ebp),%esi		/* source */
	movl	ARG2(%ebp),%edi		/* destination */
	movl	ARG3(%ebp),%ecx		/* count */

	cmpl	$0,%ecx			/* if (count==0) */
	je	bCopyB1			/*   then exit */

	/* Find out if there is an overlap problem.
	 * We have to copy backwards if destination lies within source,
	 * i.e. ((destination - source) > 0 && < nbytes) */

	cld
	movl	%edi,%eax		/* destination */
	subl	%esi,%eax		/* - source */
	jle	bCopyB0			/* <= 0 means copy forward */
	cmpl	%ecx,%eax		/* compare to nbytes */
	jge	bCopyB0			/* >= nbytes means copy forward */
	addl	%ecx,%esi
	addl	%ecx,%edi
	decl	%esi
	decl	%edi
	std

	/* Copy the whole thing, byte by byte */

bCopyB0:
	rep
	movsb

bCopyB1:
	popf
	popl	%edi
	popl	%esi
	leave
	ret


/*******************************************************************************
*
* bcopyWords - copy one buffer to another a word at a time
*
* This routine copies the first `nwords' words from
* `source' to `destination'.
* It is similar to bcopy except that the copy is always performed
* a word at a time.  This may be desirable if one of the buffers
* can only be accessed with word instructions, as in certain word-wide
* memory-mapped peripherals.  The source and destination must be word-aligned.
*
* SEE ALSO: bcopy (2)
*
* NOMANUAL - manual entry in bLib (1)

* void bcopyWords (source, destination, nwords)
*     char *source;       /* pointer to source buffer      *
*     char *destination;  /* pointer to destination buffer *
*     int nwords;         /* number of words to copy       *

*/

	.balign 16,0x90
FUNC_LABEL(bcopyWords)
	pushl	%ebp
	movl	%esp,%ebp
	pushl	%esi
	pushl	%edi
	pushf


	/* put src in %esi, dest in %edi, and count in %ecx */

	movl	ARG1(%ebp),%esi		/* source */
	movl	ARG2(%ebp),%edi		/* destination */
	movl	ARG3(%ebp),%ecx		/* count */

	shll	$1,%ecx			/* convert count to bytes */

	/* Find out if there is an overlap problem.
	 * We have to copy backwards if destination lies within source,
	 * i.e. ((destination - source) > 0 && < nbytes) */

	cld
	movl	%edi,%eax		/* destination */
	subl	%esi,%eax		/* - source */
	jle	bCopyW0			/* <= 0 means copy forward */
	cmpl	%ecx,%eax		/* compare to nbytes */
	jge	bCopyW0			/* >= nbytes means copy forward */
	addl	%ecx,%esi
	addl	%ecx,%edi
	subl	$2,%esi
	subl	$2,%edi
	std

	/* Copy the whole thing, word by word */

bCopyW0:
	shrl	$1,%ecx			/* convert count to words */
	je	bCopyW1			/* do the test first */
	rep
	movsw

bCopyW1:
	popf
	popl	%edi
	popl	%esi
	leave
	ret


/*******************************************************************************
*
* bcopyLongs - copy one buffer to another a long at a time
*
* This routine copies the first `nlongs' longs from
* `source' to `destination'.
* It is similar to bcopy except that the copy is always performed
* a long at a time.  This may be desirable if one of the buffers
* can only be accessed with long instructions, as in certain long-wide
* memory-mapped peripherals.  The source and destination must be long-aligned.
*
* SEE ALSO: bcopy (2)
*
* NOMANUAL - manual entry in bLib (1)

* void bcopyLongs (source, destination, nlongs)
*     char *source;       /* pointer to source buffer      *
*     char *destination;  /* pointer to destination buffer *
*     int nlongs;         /* number of longs to copy       *

*/

	.balign 16,0x90
FUNC_LABEL(bcopyLongs)
	pushl	%ebp
	movl	%esp,%ebp
	pushl	%esi
	pushl	%edi
	pushf

	/* put src in %esi, dest in %edi, and count in %ecx */

	movl	ARG1(%ebp),%esi		/* source */
	movl	ARG2(%ebp),%edi		/* destination */
	movl	ARG3(%ebp),%ecx		/* count */

	shll	$2,%ecx			/* convert count to bytes */

	/* Find out if there is an overlap problem.
	 * We have to copy backwards if destination lies within source,
	 * i.e. ((destination - source) > 0 && < nbytes) */

	cld
	movl	%edi,%eax		/* destination */
	subl	%esi,%eax		/* - source */
	jle	bCopyL0			/* <= 0 means copy forward */
	cmpl	%ecx,%eax		/* compare to nbytes */
	jge	bCopyL0			/* >= nbytes means copy forward */
	addl	%ecx,%esi
	addl	%ecx,%edi
	subl	$4,%esi
	subl	$4,%edi
	std

	/* Copy the whole thing, long by long */

bCopyL0:
	shrl	$2,%ecx			/* convert count to longs */
	je	bCopyL1			/* do the test first */
	rep
	movsl

bCopyL1:
	popf
	popl	%edi
	popl	%esi
	leave
	ret


/*******************************************************************************
*
* bfill - fill buffer with character
*
* This routine fills the first `nbytes' characters of the specified buffer
* with the specified character.
* The fill is optimized by filling 4 bytes at a time if possible,
* (see bfillBytes (2) for filling a byte at a time only).
*
* SEE ALSO: bfillBytes (2)
*
* NOMANUAL - manual entry in bLib (1)

* void bfill (buf, nbytes, ch)
*     char *buf;		/* pointer to buffer              *
*     int nbytes;		/* number of bytes to fill        *
*     char ch;			/* char with which to fill buffer *

*/

	.balign 16,0x90
FUNC_LABEL(bfill)
	pushl	%ebp
	movl	%esp,%ebp
	pushl	%ebx
	pushl	%edi
	pushf

	/* put buf in %edi, nbytes in %edx, and ch in %eax */

	movl	ARG1(%ebp),%edi		/* get buf */
	movl	ARG2(%ebp),%edx		/* nbytes */
	movl	ARG3(%ebp),%eax		/* ch */

	/* if length is less than 20, cheaper to do a byte fill */

	cld
	cmpl	$20,%edx		/* test count */
	jl	bFill2			/* do byte fill */

	/* Put ch in four bytes of %eax, so we can fill 4 bytes at a crack */

	movb	%al,%ah			/* move ch into 2nd byte of %eax */
	movzwl	%ax,%ecx		/* move low-word of %eax into %ecx */
	shll	$16,%eax		/* shift ch into high-word of %eax */
	orl	%ecx,%eax		/* or ch back into low-word of %eax */

	/* If the buffer is odd-aligned, copy the first 1 - 3 byte */

	movl	%edi,%ebx
	andl	$3,%ebx
	movl	%ebx,%ecx
	cmpl	$0,%ecx			/* if long-aligned */
	je	bFill0			/*   then goto bFill0 */

	rep				/* fill the 1 - 3 byte */
	stosb

	subl	%ebx,%edx		/* decrement count by 1 */

	/* Since we're copying 4 bytes at a crack, divide count by 4.
	 * Keep the remainder in %edx, so we can do those bytes at the
	 * end of the loop. */

bFill0:
	movl	%edx,%ecx
	shrl	$2,%ecx			/* count /= 4 */
	je	bFill1			/* do the test first */

	rep
	stosl


bFill1:
	andl	$3,%edx			/* remainder in %edx */

	/* do the extras at the end */

bFill2:
	movl	%edx,%ecx
	cmpl	$0,%ecx
	je	bFill3			/* do the test first */
	rep
	stosb

bFill3:
	popf
	popl	%edi
	popl	%ebx
	leave
	ret


/*******************************************************************************
*
* bfillBytes - fill buffer with character a byte at a time
*
* This routine fills the first `nbytes' characters of the
* specified buffer with the specified character.
* It is identical to bfill (2) except that the fill is always performed
* a byte at a time.  This may be desirable if the buffer
* can only be accessed with byte instructions, as in certain byte-wide
* memory-mapped peripherals.
*
* SEE ALSO: bfill (2)
*
* NOMANUAL - manual entry in bLib (1)

* void bfillBytes (buf, nbytes, ch)
*     char *buf;	/* pointer to buffer              *
*     int nbytes;	/* number of bytes to fill        *
*     char ch;		/* char with which to fill buffer *

*/

	.balign 16,0x90
FUNC_LABEL(bfillBytes)
	pushl	%ebp
	movl	%esp,%ebp
	pushl	%edi
	pushf

	/* put buf in %edi, nbytes in %ecx, and ch in %eax */

	movl	ARG1(%ebp),%edi		/* get destination */
	movl	ARG2(%ebp),%ecx		/* count */
	movl	ARG3(%ebp),%eax		/* ch */

	cmpl	$0,%ecx
	je	bFillB0			/* do the test first */

	/* Copy the whole thing, byte by byte */

	cld
	rep
	stosb

bFillB0:
	popf
	popl	%edi
	leave
	ret

/*******************************************************************************
*
* bfillWords - fill buffer with specified word(2-byte) a word at a time
*
* This routine fills the first `nwords' characters of the specified buffer
* with the specified word.  The fill is optimized by using the stos 
* instruction.
* It is identical to bfill (2) except that the fill is always performed
* a word at a time.  This may be desirable if the buffer
* can only be accessed with word instructions, as in certain 2byte-wide
* memory-mapped peripherals.
*

* SEE ALSO: bfill (2)
*
* NOMANUAL - manual entry in bLib (1)
*
* void bfillWords (buf, nwords, value)
*     char * buf;               /* pointer to buffer              *
*     int nwords;               /* number of words to fill        *
*     short value;              /* short with which to fill buffer *
*
*/

        .balign 16,0x90
FUNC_LABEL(bfillWords)
        pushl   %ebp
        movl    %esp,%ebp
        pushl   %edi

        /* put buf in %edi, nlongs in %ecx, and value in %eax */

        movl    ARG1(%ebp),%edi         /* get buf */
        movl    ARG2(%ebp),%ecx         /* get nwords */
        movl    ARG3(%ebp),%eax         /* get value */

        cld
	rep
        stosw                              /* fill it, word by word */

        popl    %edi
	leave
        ret

/*******************************************************************************
*
* bfillLongs - fill buffer with specified long(4-byte) a long at a time
*
* This routine fills the first `nlongs' characters of the specified buffer
* with the specified long.  The fill is optimized by using the stos 
* instruction.
* It is identical to bfill (2) except that the fill is always performed
* a long at a time.  This may be desirable if the buffer
* can only be accessed with long instructions, as in certain 4byte-wide
* memory-mapped peripherals.
*

* SEE ALSO: bfill (2)
*
* NOMANUAL - manual entry in bLib (1)
*
* void bfillLongs (buf, nlongs, value)
*     char * buf;               /* pointer to buffer              *
*     int nlongs;               /* number of longs to fill        *
*     long value;               /* long with which to fill buffer *
*
*/

        .balign 16,0x90
FUNC_LABEL(bfillLongs)
        pushl   %ebp
        movl    %esp,%ebp
        pushl   %edi

        /* put buf in %edi, nlongs in %ecx, and value in %eax */

        movl    ARG1(%ebp),%edi         /* get buf */
        movl    ARG2(%ebp),%ecx         /* get nlongs */
        movl    ARG3(%ebp),%eax         /* get value */

        cld
	rep
        stosl                              /* fill it, long by long */

        popl    %edi
        leave
        ret

#endif	/* !PORTABLE */
