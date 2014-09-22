/* uncompress.c - uncompression module */

/* Copyright 1990-1992 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
02h,04nov93,caf  fixed two more variables that were not being initialized
		 during the R3000 bootstrap.
02g,07sep93,caf  modified not to rely on initialized data, to work around
		 a.out assumption in current bootstrap.
02f,26may92,rrr  the tree shuffle
02e,14nov91,rrr  shut up some warnings
02d,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -fixed #else and #endif
		  -changed copyright notice
02c,25jul91,yao  restore previous version 02a.
02b,06jul91,yao  made it work with little endian mode. added forward
		 declaration.
02a,16may90,gae  revised initial bootrom uncompression scheme --
	           ROM startup in separate module.
01a,21apr90,rdc  modified from compress source.
*/

/*
DESCRIPTION
This is a modified version of the Public Domain uncompress program.
It is used to uncompress the VxWorks bootrom executable linked with it.
Compressing object code typically achieves a 40% compression factor.

SEE ALSO:
compress(1), romInit(1)

AUTHOR
The original compress program was written by:
Spencer W. Thomas, Jim McKie, Steve Davies, Ken Turkowski,
James A. Woods, Joe Orost.
*/

#include "vxWorks.h"


/* Set USERMEM to the maximum amount of physical user memory available
 * in bytes.  USERMEM is used to determine the maximum BITS that can be used
 * for compression.  If USERMEM is big enough, use fast compression algorithm.
 *
 * SACREDMEM is the amount of physical memory saved for others; compress
 * will hog the rest.
 */

#define	USERMEM		500000	/* .5M for uncompression data structures */
#define SACREDMEM	0

/*
 * Define FBITS for machines with several MB of physical memory, to use
 * table lookup for (b <= FBITS).  If FBITS is made too large, performance
 * will decrease due to increased swapping/paging.  Since the program minus
 * the fast lookup table is about a half Meg, we can allocate the rest of
 * available physical memory to the fast lookup table.
 *
 * If FBITS is set to 12, a 2 MB array is allocated, but only 1 MB is
 * addressed for parity-free input (i.e. text).
 *
 * FBITS=10 yields 1/2 meg lookup table + 4K code memory
 * FBITS=11 yields 1 meg lookup table + 8K code memory
 * FBITS=12 yields 2 meg lookup table + 16K code memory
 * FBITS=13 yields 4 meg lookup table + 32K code memory
 *
 */

#ifdef USERMEM
# if USERMEM >= (2621440+SACREDMEM)
#  if USERMEM >= (4718592+SACREDMEM)
#   define FBITS		13
#   define PBITS	16
#else	/* 2.5M <= USERMEM < 4.5M */
#   define FBITS		12
#   define PBITS	16
#endif	/* USERMEM <=> 4.5M */
#else	/* USERMEM < 2.5M */
#  if USERMEM >= (1572864+SACREDMEM)
#   define FBITS		11
#   define PBITS	16
#else	/* USERMEM < 1.5M */
#   if USERMEM >= (1048576+SACREDMEM)
#    define FBITS		10
#    define PBITS	16
#else	/* USERMEM < 1M */
#    if USERMEM >= (631808+SACREDMEM)
#     define PBITS	16
#    else
#     if USERMEM >= (329728+SACREDMEM)
#      define PBITS	15
#     else
#      if USERMEM >= (178176+SACREDMEM)
#       define PBITS	14
#      else
#       if USERMEM >= (99328+SACREDMEM)
#        define PBITS	13
#       else
#        define PBITS	12
#       endif
#      endif
#     endif
#    endif
#    undef USERMEM
#endif	/* USERMEM <=> 1M */
#endif	/* USERMEM <=> 1.5M */
#endif	/* USERMEM <=> 2.5M */
#endif	/* USERMEM */

#ifdef PBITS			/* Preferred BITS for this memory size */
# ifndef BITS
#  define BITS PBITS
#endif	/* BITS */
#endif	/* PBITS */

#if BITS == 16
# define HSIZE	69001		/* 95% occupancy */
#endif	/* BITS */
#if BITS == 15
# define HSIZE	35023		/* 94% occupancy */
#endif	/* BITS */
#if BITS == 14
# define HSIZE	18013		/* 91% occupancy */
#endif	/* BITS */
#if BITS == 13
# define HSIZE	9001		/* 91% occupancy */
#endif	/* BITS */
#if BITS == 12
# define HSIZE	5003		/* 80% occupancy */
#endif	/* BITS */
#if BITS == 11
# define HSIZE	2591		/* 79% occupancy */
#endif	/* BITS */
#if BITS == 10
# define HSIZE	1291		/* 79% occupancy */
#endif	/* BITS */
#if BITS == 9
# define HSIZE	691		/* 74% occupancy */
#endif	/* BITS */

/* BITS < 9 will cause an error */

/*
 * a code_int must be able to hold 2**BITS values of type int, and also -1
 */

#if BITS > 15
typedef long int code_int;
#else
typedef int     code_int;
#endif	/* BITS */

typedef long int count_int;
typedef unsigned char char_type;

/* defines for magic number {"\037\235"} */

#define	MAGIC_HEADER_0	0x1f
#define	MAGIC_HEADER_1	0x9d

/* Defines for third byte of header */
#define BIT_MASK	0x1f
#define BLOCK_MASK	0x80

/* Masks 0x40 and 0x20 are free.  I think 0x20 should mean that there is
   a fourth header byte (for expansion).
*/
#define INIT_BITS 9		/* initial number of bits/code */

LOCAL int             n_bits;		/* number of bits/code */
LOCAL int             maxbits;
LOCAL code_int        maxcode;	/* maximum code, given n_bits */
LOCAL code_int        maxmaxcode;	/* should NEVER generate this code */

#define MAXCODE(n_bits)	((1 << (n_bits)) - 1)

/*
 * One code could conceivably represent (1<<BITS) characters, but
 * to get a code of length N requires an input string of at least
 * N*(N-1)/2 characters.  With 5000 chars in the stack, an input
 * file would have to contain a 25Mb string of a single character.
 * This seems unlikely.
 */

#define MAXSTACK    8000	/* size of output stack */

LOCAL unsigned short  codetab[HSIZE];
LOCAL code_int        hsize;		/* for dynamic table sizing */
LOCAL count_int       fsize;

#define tab_prefix	codetab	/* prefix code for this entry */
LOCAL char_type       tab_suffix[1 << BITS];	/* last char in this entry */
LOCAL code_int        free_ent;	/* first unused entry */

#define	NOMAGIC	0	/* use 2 byte magic number header, unless old file */

/*
 * block compression parameters -- after all codes are used up,
 * and compression rate changes, start over.
 */
LOCAL int block_compress;
LOCAL int clear_flg;

LOCAL char_type rmask[9];	/* initialize in uncompress() */


/* The next two codes should not be changed lightly, as they must not
 * lie within the contiguous general code space.
 */
#define FIRST	257		/* first free entry */
#define	CLEAR	256		/* table clear output code */


LOCAL UCHAR *binEnd;
LOCAL UCHAR *binArray;
LOCAL int offset;		/* zero'd by uncompress(), used by getcode() */
LOCAL int size;			/* zero'd by uncompress(), used by getcode() */


/* forward static functions */

static code_int getcode (void);


/*******************************************************************************
*
* uncompress - uncompress `source' to `destination'
*
* This routine uncompresses the image found at `source' to the `destination'
* address, where the source is `len' bytes long.
*/

STATUS uncompress
    (
    UCHAR *source,              /* start of compressed image */
    FAST UCHAR *destination,    /* destination for uncompressed result */
    int len                     /* length of compressed image */
    )
    {
    static char stack [MAXSTACK];

    FAST int stack_top = MAXSTACK;
    FAST code_int code;
    FAST code_int oldcode;
    FAST code_int incode;
    FAST int finchar;

    binEnd = &source [len];
    binArray = source;

    /* initialize */

    offset = 0;			/* used by getcode() */
    size = 0;			/* used by getcode() */

    block_compress  = BLOCK_MASK;

    fsize           = 200000;
    clear_flg       = 0;

    maxbits        = BITS;

    if (maxbits < INIT_BITS)
	maxbits = INIT_BITS;
    if (maxbits > BITS)
	maxbits = BITS;

    maxmaxcode = 1 << maxbits;

    rmask[0] = 0x00;
    rmask[1] = 0x01;
    rmask[2] = 0x03;
    rmask[3] = 0x07;
    rmask[4] = 0x0f;
    rmask[5] = 0x1f;
    rmask[6] = 0x3f;
    rmask[7] = 0x7f;
    rmask[8] = 0xff;

    /* check the magic number */

    if (NOMAGIC == 0)
        {
	if ((*binArray++) != (MAGIC_HEADER_0) ||
	    (*binArray++) != (MAGIC_HEADER_1))
	    {
	    return (ERROR);	/* bad magic number */
	    }

	maxbits        = *binArray++;		/* set -b from file */
	block_compress = maxbits & BLOCK_MASK;
	maxbits       &= BIT_MASK;
	maxmaxcode     = (1 << maxbits);

	if (maxbits > BITS)
	    return (ERROR);	/* compressed with too many bits per code */
        }

    /* tune hash table size for small files -- ad hoc */

#if HSIZE > 5003
    if (fsize < (1 << 12))
	hsize = 5003;

#if HSIZE > 9001
    else if (fsize < (1 << 13))
	hsize = 9001;

#if HSIZE > 18013
    else if (fsize < (1 << 14))
	hsize = 18013;

#if HSIZE > 35023
    else if (fsize < (1 << 15))
	hsize = 35023;
    else if (fsize < 47000)
	hsize = 50021;
#endif	/* HSIZE > 35023 */
#endif	/* HSIZE > 18013 */
#endif	/* HSIZE > 9001 */

    else
#endif	/* HSIZE > 5003 */

	hsize = HSIZE;


    /* initialize the first 256 entries in the table */

    maxcode = MAXCODE (n_bits = INIT_BITS);
    for (code = 255; code >= 0; code--)
        {
	tab_prefix[code] = 0;
	tab_suffix[code] = (char_type) code;
        }

    free_ent = ((block_compress) ? FIRST : 256);

    finchar = oldcode = getcode ();

    *(destination++) = (char) finchar;

    while ((code = getcode ()) != -1)
        {
	if (code == CLEAR && block_compress)
	    {
	    for (code = 255; code > 0; code -= 4)
	        {
		tab_prefix[code-3] = 0;
		tab_prefix[code-2] = 0;
		tab_prefix[code-1] = 0;
		tab_prefix[code]   = 0;
	        }

	    clear_flg = 1;
	    free_ent = FIRST - 1;

	    if ((code = getcode ()) == -1) /* Oh, untimely death! */
		break;
	    }

	incode = code;

	/* special case for KwKwK string */
	if (code >= free_ent)
	    {
	    stack[--stack_top] = finchar;
	    code = oldcode;
	    }

	/* generate output characters in reverse order */

	while (code >= 256)
	    {
	    stack[--stack_top] = tab_suffix[code];
	    code = tab_prefix[code];
	    }

	stack[--stack_top] = finchar = tab_suffix[code];

	/* and put them out in forward order */
	for (; stack_top < MAXSTACK; stack_top++)
	    *(destination++) = stack[stack_top];

	stack_top = MAXSTACK;

	/* Generate the new entry */

	if ((code = free_ent) < maxmaxcode)
	    {
	    tab_prefix[code] = (unsigned short) oldcode;
	    tab_suffix[code] = finchar;
	    free_ent = code + 1;
	    }

	oldcode = incode;	/* remember previous code */
        }

    return (OK);
    }
/*******************************************************************************
*
* getcode - read one code from the compressed image
*
* RETURNS: Code or -1 at end of image.
*/

LOCAL code_int getcode (void)
    {
    static char_type buf [BITS];

    FAST code_int code;
    FAST int r_off;
    FAST int bits;
    FAST char_type *bp;

    if (clear_flg > 0 || offset >= size || free_ent > maxcode)
        {
	/*
	 * If the next entry will be too big for the current code size, then
	 * we must increase the size.  This implies reading a new buffer
	 * full, too.
	 */
	if (free_ent > maxcode)
	    {
	    n_bits++;
	    if (n_bits == maxbits)
		maxcode = maxmaxcode;	/* won't get any bigger now */
	    else
		maxcode = MAXCODE (n_bits);
	    }
	if (clear_flg > 0)
	    {
	    maxcode = MAXCODE (n_bits = INIT_BITS);
	    clear_flg = 0;
	    }

	if (&binArray [n_bits] > binEnd)
	    size = binEnd - binArray;
	else
	    size = n_bits;

	if (size <= 0)
	    return (-1);		/* end of file */

	bp = buf;

	while (bp < (char_type *) ((int)buf + size))
	    *bp++ = *binArray++;

	offset = 0;

	/* Round size down to integral number of codes */
	size = (size << 3) - (n_bits - 1);
        }

    r_off = offset;
    bits  = n_bits;

    /* get to the first byte */

    bp    = buf + (r_off >> 3);
    r_off &= 7;

    /* get first part (low order bits) */

    code  = (*bp++ >> r_off);
    bits -= (8 - r_off);
    r_off = 8 - r_off;		/* now, offset into code word */

    /* get any 8 bit parts in the middle (<=1 for up to 16 bits) */
    if (bits >= 8)
        {
	code  |= *bp++ << r_off;
	r_off += 8;
	bits  -= 8;
        }

    /* high order bits */
    code |= (*bp & rmask[bits]) << r_off;

    offset += n_bits;

    return (code);
    }
