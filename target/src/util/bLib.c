/* bLib.c - buffer manipulation library */

/* Copyright 1984-1998 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
02z,11nov01,dee  Add CPU_FAMILY==COLDFIRE
02y,01dec98,cdp  make ARM CPUs with ARM_THUMB==TRUE use portable routines;
	    jpd  improve swab for ARM.
02y,03mar00,zl   merged SH support from T1
02x,22apr97,jpd  added support for ARM optimised routines.
02w,19mar95,dvs  removing tron references.
02v,06mar95,jdi  doc addition to swab() and uswab().
02u,11feb95,jdi  doc tweak.
02u,21mar95,caf  added PPC support.
02t,20jul94,dvs  fixed documentation for return values of bcmp (SPR #2493).
02s,09jun93,hdn  added a support for I80X86
02r,17oct94,rhp  delete obsolete doc references to strLib, spr#3712	
02q,20jan93,jdi  documentation cleanup for 5.1.
02p,14sep92,smb  changed prototype of bcopy to include const.
02o,23aug92,jcf  fixed uswab to swap all of buffer.  removed memcmp call.
02n,30jul92,jwt  fixed error in bcopy() backwards overrunning dest address.
02m,08jul92,smb  removed string library ANSI routines.  added index and rindex.
02l,08jun92,ajm  added mips to list of optimized cpu's
02k,26may92,rrr  the tree shuffle
02j,09dec91,rrr  removed memcpy for 960, already in i960/bALib.s
02i,26nov91,llk  fixed error with placement of bLib_PORTABLE.
02h,25nov91,llk  changed bfillBytes() parameter from unsigned char to int.
		 added ansi routines:  memcpy(), memmove(), memcmp(),
		   strcoll(), strxfrm(), strchr(), strcspn(), strpbrk(),
		   strrchr(), strspn(), strstr(), strtok(), memset()
02g,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -fixed #else and #endif
		  -changed VOID to void
		  -changed copyright notice
02f,30jul91,hdn  added conditional macro for optimized TRON codes.
02e,29jul91,del  fixed comment.
02d,16jul91,ajm/del  added uswab for unaligned byte swaps (see dosFsLib.c)
02c,09jun91,del  mods to allow using gnu960 library functions.
02b,26apr91,hdn  added defines and macros for TRON architecture,
		 modified bcopy(), bcopyBytes(), bfill().
02a,05apr91,jdi	 documentation -- removed header parens and x-ref numbers;
		 doc review by dnw.
01z,24mar91,del  added I960 to check for portable version.
01y,24jan91,jaa	 documentation.
01x,20dec90,gae  added forward declaration of bfill, ALIGNMENT set to 1 for 020.
           +shl  fixed bug in bfill of (occasionally) filling too far.
01w,20sep90,dab  changed ALIGNMENT value to be 3 across architectures.
01v,19jul90,dnw  fixed mangen problem
01u,10may90,jcf  fixed PORTABLE definition.
01t,14mar90,jdi  documentation cleanup.
01s,07aug89,gae  added C versions of bcopy,bfill,etc.
01r,30aug88,gae  more documentation tweaks.
01q,20aug88,gae  documentation.
01p,05jun88,dnw  changed from bufLib to bLib.
01o,30may88,dnw  changed to v4 names.
01n,19mar87,gae  oops in 01m!
01m,19mar87,gae  fixed swab to work right when from/to are the same.
		 made swab type void.
01l,16feb87,llk  added swab().
01k,21dec86,dnw  changed to not get include files from default directories.
01j,01jul86,jlf  minor documentation cleanup.
01i,06aug85,jlf  removed cpybuf,movbuf, and filbuf, which are now in
		 asm language.  Made the remaining routines accept ints instead
		 of shorts, and removed the screwy loop constructs.
01h,06sep84,jlf  Added copyright notice.
01g,29jun84,ecs  changed cmpbuf to return -1, 0, 1.
01f,18jun84,dnw  Added LINTLIBRARY for lint.
01e,17nov83,jlf  Added movbuf
01d,09jun83,ecs  added some commentary
01c,07may83,dnw  enabled filbuf to fill more than 64K bytes
01b,04apr83,dnw  added invbuf.
01a,15mar83,dnw  created from old utillb.c
*/

/*
DESCRIPTION
This library contains routines to manipulate buffers of variable-length
byte arrays.  Operations are performed on long words when possible, even
though the buffer lengths are specified in bytes.  This occurs only when
source and destination buffers start on addresses that are both odd or both
even.  If one buffer is even and the other is odd, operations must be done
one byte at a time (because of alignment problems inherent in the MC68000),
thereby slowing down the process.

Certain applications, such as byte-wide memory-mapped peripherals, may
require that only byte operations be performed.  For this purpose, the
routines bcopyBytes() and bfillBytes() provide the same functions as bcopy()
and bfill(), but use only byte-at-a-time operations.  These routines do
not check for null termination.

INCLUDE FILES: string.h

SEE ALSO: ansiString
*/

/* LINTLIBRARY */

#include "vxWorks.h"
#include "string.h"

/*
 * optimized version available for 680X0, I960, MIPS, I80X86, PPC, SH and ARM,
 * but not yet Thumb.
 */

#if (defined(PORTABLE) || \
     ((CPU_FAMILY != MC680X0) && \
      (CPU_FAMILY!=I960) && \
      (CPU_FAMILY != MIPS) && \
      (CPU_FAMILY != I80X86) && \
      (CPU_FAMILY != PPC) && \
      (CPU_FAMILY != SH) && \
      (CPU_FAMILY != COLDFIRE) && \
      (CPU_FAMILY != ARM)) || \
     ((CPU_FAMILY == ARM) && ARM_THUMB))
#define bLib_PORTABLE
#endif	/* defined(PORTABLE) */

/* optimized swab() available for PPC */

#if (defined(PORTABLE) || (CPU_FAMILY != PPC))
#define swab_PORTABLE
#endif	/* defined(PORTABLE) */

#if	(CPU==MC68020) || (CPU_FAMILY==I80X86) || (CPU_FAMILY == COLDFIRE)
#define	ALIGNMENT	1	/* 2-byte */
#else
#define	ALIGNMENT	3	/* quad */
#endif	/* (CPU==MC68020) || (CPU_FAMILY==I80X86) */

#undef bcmp /* so routine gets built for those who don't include header files */
/*******************************************************************************
*
* bcmp - compare one buffer to another
*
* This routine compares the first <nbytes> characters of <buf1> to <buf2>.
*
* RETURNS:
*   0 if the first <nbytes> of <buf1> and <buf2> are identical,
*   less than 0 if <buf1> is less than <buf2>, or
*   greater than 0 if <buf1> is greater than <buf2>.
*/

int bcmp
    (
    FAST char *buf1,            /* pointer to first buffer    */
    FAST char *buf2,            /* pointer to second buffer   */
    FAST int nbytes             /* number of bytes to compare */
    )
    {
    const unsigned char *p1;
    const unsigned char *p2;

    /* size of memory is zero */

    if (nbytes == 0)
	return (0);

    /* compare array 2 into array 1 */

    p1 = (const unsigned char *)buf1;
    p2 = (const unsigned char *)buf2;

    while (*p1++ == *p2++)
	{
	if (--nbytes == 0)
	    return (0);
        }

    return ((*--p1) - (*--p2));
    }
/*******************************************************************************
*
* binvert - invert the order of bytes in a buffer
*
* This routine inverts an entire buffer, byte by byte.  For example,
* the buffer {1, 2, 3, 4, 5} would become {5, 4, 3, 2, 1}.
*
* RETURNS: N/A
*/

void binvert
    (
    FAST char *buf,             /* pointer to buffer to invert  */
    int nbytes                  /* number of bytes in buffer    */
    )
    {
    FAST char *buf_end = buf + nbytes - 1;
    FAST char temp;

    while (buf < buf_end)
	{
	temp       = *buf;
	*buf       = *buf_end;
	*buf_end   = temp;

	buf_end--;
	buf++;
	}
    }
/*******************************************************************************
*
* bswap - swap buffers
*
* This routine exchanges the first <nbytes> of the two specified buffers.
*
* RETURNS: N/A
*/

void bswap
    (
    FAST char *buf1,            /* pointer to first buffer  */
    FAST char *buf2,            /* pointer to second buffer */
    FAST int nbytes             /* number of bytes to swap  */
    )
    {
    FAST char temp;

    while (--nbytes >= 0)
	{
	temp = *buf1;
	*buf1++ = *buf2;
	*buf2++ = temp;
	}
    }

#ifdef	swab_PORTABLE

/*******************************************************************************
*
* swab - swap bytes
*
* This routine gets the specified number of bytes from <source>,
* exchanges the adjacent even and odd bytes, and puts them in <destination>.
* The buffers <source> and <destination> should not overlap.
*
* \&NOTE:  On some CPUs, swab() will cause an exception if the buffers are
* unaligned.  In such cases, use uswab() for unaligned swaps.  On ARM
* family CPUs, swab() may reorder the bytes incorrectly without causing
* an exception if the buffers are unaligned.  Again, use uswab() for
* unaligned swaps.
*
* It is an error for <nbytes> to be odd.
*
* RETURNS: N/A
*
* SEE ALSO: uswab()
*/

void swab
    (
    char *source,               /* pointer to source buffer      */
    char *destination,          /* pointer to destination buffer */
    int nbytes                  /* number of bytes to exchange   */
    )
#if (CPU_FAMILY == ARM)
    {
    /*
     * This generates much better code for the ARM, and might well be
     * faster on gcc-based compilers for other architectures.
     */

    FAST unsigned short *src = (unsigned short *) source;
    FAST unsigned short *dst = (unsigned short *) destination;
    FAST unsigned short *dst_end = (unsigned short *) (destination + nbytes);

    for (; dst < dst_end; dst++, src++)
	{
	*dst = ((*src & 0x00ff) << 8) | ((*src & 0xff00) >> 8);
	}
    }
#else /* (CPU_FAMILY == ARM) */
    {
    FAST short *src = (short *) source;
    FAST short *dst = (short *) destination;
    FAST short *dst_end = dst + (nbytes / 2);

    for (; dst < dst_end; dst++, src++)
	{
	*dst = ((*src & 0x00ff) << 8) | ((*src & 0xff00) >> 8);
	}
    }
#endif /* (CPU_FAMILY == ARM) */

#endif	/* swab_PORTABLE */

#if (CPU_FAMILY != I960) || !defined(__GNUC__) || defined(VX_IGNORE_GNU_LIBS)

/*******************************************************************************
*
* uswab - swap bytes with buffers that are not necessarily aligned
*
* This routine gets the specified number of bytes from <source>,
* exchanges the adjacent even and odd bytes, and puts them in <destination>.
*
* \&NOTE:  Due to speed considerations, this routine should only be used when
* absolutely necessary.  Use swab() for aligned swaps.
*
* It is an error for <nbytes> to be odd.
*
* RETURNS: N/A
*
* SEE ALSO: swab()
*/

void uswab
    (
    char *source,               /* pointer to source buffer      */
    char *destination,          /* pointer to destination buffer */
    int nbytes                  /* number of bytes to exchange   */
    )
    {
    FAST char *dst = (char *) destination;
    FAST char *dst_end = dst + nbytes;
    FAST char byte1;
    FAST char byte2;
    while (dst < dst_end)
	{
	byte1 = *source++;
	byte2 = *source++;
	*dst++ = byte2;
	*dst++ = byte1;
	}
    }

/*******************************************************************************
*
* bzero - zero out a buffer
*
* This routine fills the first <nbytes> characters of the
* specified buffer with 0.
*
* RETURNS: N/A
*/

void bzero
    (
    char *buffer,               /* buffer to be zeroed       */
    int nbytes                  /* number of bytes in buffer */
    )
    {
    bfill (buffer, nbytes, 0);
    }

#endif	/* IGNORE GNU LIBS */

#ifdef bLib_PORTABLE

#if (CPU_FAMILY != I960) || !defined(__GNUC__) || defined(VX_IGNORE_GNU_LIBS)
/*******************************************************************************
*
* bcopy - copy one buffer to another
*
* This routine copies the first <nbytes> characters from <source> to
* <destination>.  Overlapping buffers are handled correctly.  Copying is done
* in the most efficient way possible, which may include long-word, or even
* multiple-long-word moves on some architectures.  In general, the copy
* will be significantly faster if both buffers are long-word aligned.
* (For copying that is restricted to byte, word, or long-word moves, see
* the manual entries for bcopyBytes(), bcopyWords(), and bcopyLongs().)
*
* RETURNS: N/A
*
* SEE ALSO: bcopyBytes(), bcopyWords(), bcopyLongs()
*/

void bcopy
    (
    const char *source,       	/* pointer to source buffer      */
    char *destination,  	/* pointer to destination buffer */
    int nbytes          	/* number of bytes to copy       */
    )
    {
    FAST char *dstend;
    FAST long *src;
    FAST long *dst;
    int tmp = destination - source;

    if (tmp <= 0 || tmp >= nbytes)
	{
	/* forward copy */

	dstend = destination + nbytes;

	/* do byte copy if less than ten or alignment mismatch */

	if (nbytes < 10 || (((int)destination ^ (int)source) & ALIGNMENT))
	    goto byte_copy_fwd;

	/* if odd-aligned copy byte */

	while ((int)destination & ALIGNMENT)
	    *destination++ = *source++;

	src = (long *) source;
	dst = (long *) destination;

	do
	    {
	    *dst++ = *src++;
	    }
	while (((char *)dst + sizeof (long)) <= dstend);

	destination = (char *)dst;
	source      = (char *)src;

byte_copy_fwd:
	while (destination < dstend)
	    *destination++ = *source++;
	}
    else
	{
	/* backward copy */

	dstend       = destination;
	destination += nbytes - sizeof (char);
	source      += nbytes - sizeof (char);

	/* do byte copy if less than ten or alignment mismatch */

	if (nbytes < 10 || (((int)destination ^ (int)source) & ALIGNMENT))
	    goto byte_copy_bwd;

	/* if odd-aligned copy byte */

	while ((int)destination & ALIGNMENT)
	    *destination-- = *source--;

	src = (long *) source;
	dst = (long *) destination;

	do
	    {
	    *dst-- = *src--;
	    }
	while (((char *)dst + sizeof(long)) >= dstend);

	destination = (char *)dst + sizeof (long);
	source      = (char *)src + sizeof (long);

byte_copy_bwd:
	while (destination >= dstend)
	    *destination-- = *source--;
	}
    }
#endif	/* IGNORE GNU LIBS */
/*******************************************************************************
*
* bcopyBytes - copy one buffer to another one byte at a time
*
* This routine copies the first <nbytes> characters from <source> to
* <destination> one byte at a time.  This may be desirable if a buffer can
* only be accessed with byte instructions, as in certain byte-wide
* memory-mapped peripherals.
*
* RETURNS: N/A
*
* SEE ALSO: bcopy()
*/

void bcopyBytes
    (
    char *source,       /* pointer to source buffer      */
    char *destination,  /* pointer to destination buffer */
    int nbytes          /* number of bytes to copy       */
    )
    {
    FAST char *dstend;
    int tmp = destination - source;   /* XXX does it work if MSB was 1 ? */

    if (tmp == 0)
        return;
    else if (tmp < 0 || tmp >= nbytes)
    	{
	/* forward copy */
	dstend = destination + nbytes;
	while (destination < dstend)
	    *destination++ = *source++;
	}
    else
	{
	/* backward copy */
	dstend       = destination;
	destination += nbytes - 1;
	source      += nbytes - 1;
	while (destination >= dstend)
	    *destination-- = *source--;
	}
    }
/*******************************************************************************
*
* bcopyWords - copy one buffer to another one word at a time
*
* This routine copies the first <nwords> words from <source> to <destination>
* one word at a time.  This may be desirable if a buffer can only be accessed
* with word instructions, as in certain word-wide memory-mapped peripherals.
* The source and destination must be word-aligned.
*
* RETURNS: N/A
*
* SEE ALSO: bcopy()
*/

void bcopyWords
    (
    char *source,       /* pointer to source buffer      */
    char *destination,  /* pointer to destination buffer */
    int nwords          /* number of words to copy       */
    )
    {
    FAST short *dstend;
    FAST short *src = (short *) source;
    FAST short *dst = (short *) destination;
    int tmp = destination - source;   /* XXX does it work if MSB was 1 ? */
    int nbytes = nwords << 1;           /* convert to bytes */

    if (tmp == 0)
    return;
    else if (tmp < 0 || tmp >= nbytes)
	{
	/* forward copy */
	dstend = dst + nwords;
	while (dst < dstend)
	    *dst++ = *src++;
	}
    else
	{
	/* backward copy */
	dstend = dst;
        dst   += nwords - 1;
	src   += nwords - 1;
	while (dst >= dstend)
	    *dst-- = *src--;
	}
    }
/*******************************************************************************
*
* bcopyLongs - copy one buffer to another one long word at a time
*
* This routine copies the first <nlongs> characters from <source> to
* <destination> one long word at a time.  This may be desirable if a buffer
* can only be accessed with long instructions, as in certain long-word-wide
* memory-mapped peripherals.  The source and destination must be
* long-aligned.
*
* RETURNS: N/A
*
* SEE ALSO: bcopy()
*/

void bcopyLongs
    (
    char *source,       /* pointer to source buffer      */
    char *destination,  /* pointer to destination buffer */
    int nlongs          /* number of longs to copy       */
    )
    {
    FAST long *dstend;
    FAST long *src = (long *) source;
    FAST long *dst = (long *) destination;
    int tmp = destination - source;     /* XXX does it work if MSB was 1 ? */
    int nbytes = nlongs << 2;           /* convert to bytes */

    if (tmp == 0)
        return;
    else if (tmp < 0 || tmp >= nbytes)
	{
	/* forward copy */
	dstend = dst + nlongs;
	while (dst < dstend)
	    *dst++ = *src++;
	}
    else
	{
	/* backward copy */
        dstend = dst;
	dst   += nlongs - 1;
	src   += nlongs - 1;
	while (dst >= dstend)
	    *dst-- = *src--;
	}
    }

#undef bfill /* so bfill gets built for those who don't include header files */
/*******************************************************************************
*
* bfill - fill a buffer with a specified character
*
* This routine fills the first <nbytes> characters of a buffer with the
* character <ch>.  Filling is done in the most efficient way possible,
* which may be long-word, or even multiple-long-word stores, on some
* architectures.  In general, the fill will be significantly faster if
* the buffer is long-word aligned.  (For filling that is restricted to
* byte stores, see the manual entry for bfillBytes().)
*
* RETURNS: N/A
*
* SEE ALSO: bfillBytes()
*/

void bfill
    (
    FAST char *buf,           /* pointer to buffer              */
    int nbytes,               /* number of bytes to fill        */
    FAST int ch     	      /* char with which to fill buffer */
    )
    {
#if (CPU_FAMILY != I960) || !defined(__GNUC__) || defined(VX_IGNORE_GNU_LIBS)
    FAST long *pBuf;
    char *bufend = buf + nbytes;
    FAST char *buftmp;
    FAST long val;

    if (nbytes < 10)
	goto byte_fill;

    val = (ch << 24) | (ch << 16) | (ch << 8) | ch;

    /* start on necessary alignment */

    while ((int)buf & ALIGNMENT)
	*buf++ = ch;

    buftmp = bufend - sizeof (long); /* last word boundary before bufend */

    pBuf = (long *)buf;

    /* fill 4 bytes at a time; don't exceed buf endpoint */

    do
	{
	*pBuf++ = val;
	}
    while ((char *)pBuf < buftmp);

    buf = (char *)pBuf - sizeof (long);

    /* fill remaining bytes one at a time */

byte_fill:
    while (buf < bufend)
	*buf++ = ch;
#else	/* IGNORE GNU LIBS */
    (void) memset ((void *)buf, (int) ch, (size_t) nbytes);
#endif	/* IGNORE GNU LIBS */
    }
/*******************************************************************************
*
* bfillBytes - fill buffer with a specified character one byte at a time
*
* This routine fills the first <nbytes> characters of the specified buffer
* with the character <ch> one byte at a time.  This may be desirable if a
* buffer can only be accessed with byte instructions, as in certain
* byte-wide memory-mapped peripherals.
*
* RETURNS: N/A
*
* SEE ALSO: bfill()
*/

void bfillBytes
    (
    FAST char *buf,        /* pointer to buffer              */
    int nbytes,            /* number of bytes to fill        */
    FAST int ch		   /* char with which to fill buffer */
    )
    {
    FAST char *bufend = buf + nbytes;

    while (buf < bufend)
	*buf++ = ch;
    }
#endif	/* bLib_PORTABLE */

#undef index /* so index gets built for those who don't include header files */
/*******************************************************************************
*
* index - find the first occurrence of a character in a string
*
* This routine finds the first occurrence of character <c>
* in string <s>.
*
* RETURNS:
* A pointer to the located character, or
* NULL if <c> is not found.
*
* SEE ALSO: strchr().
*/

char *index
    (
    FAST const char *s,      /* string in which to find character */
    FAST int c               /* character to find in string       */
    )
    {
    FAST char ch;

    while (((ch = *(s++)) != c) && (ch != EOS))
	;

    return (((ch == EOS) && c != EOS) ? NULL : (char *) --s);
    }

#undef rindex /* so rindex is built for those who don't include header files */
/*******************************************************************************
*
* rindex - find the last occurrence of a character in a string
*
* This routine finds the last occurrence of character <c>
* in string <s>.
*
* RETURNS:
* A pointer to <c>, or
* NULL if <c> is not found.
*/

char *rindex
    (
    FAST const char *s,       	/* string in which to find character */
    int c              		/* character to find in string       */
    )
    {
    int i;	/* must be signed! */

    i = (int) strlen (s);	/* point to null terminator of s */

    while (i >= 0)
	{
	if (s [i] == c)
	    return ((char *) &s [i]);

	i--;
	}

    return ((char *) NULL);
    }
