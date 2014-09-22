/* unixALib.s - network assembly language routines */

/* Copyright 1995-2000 Wind River Systems, Inc. */

	.data
	.global	_copyright_wind_river
	.long	_copyright_wind_river

/*
modification history
--------------------
01q,21aug00,hk   merge SH7729 to SH7700. simplify CPU conditionals.
01p,28mar00,hk   added .type directive to function names.
01o,17mar00,zl   made use of alignment macro _ALIGN_TEXT
01n,05oct99,zl   updated cksum for little endian release.
01m,25feb99,hk   simplified CPU conditional in default of shld instruction.
01l,14sep98,hk   simplified CPU conditionals.
01k,16jul98,st   added SH7750 support.
01k,08may98,jmc  added support for SH-DSP and SH3-DSP.
01j,25apr97,hk   changed SH704X to SH7040.
01i,05aug96,hk   simpified PORTABLE control. deleted unnecessary comments.
		 added shld optimization to csLongBoundary.
01h,29jul96,hk   changed to use 'mova', added DEBUG_LOCAL_SYMBOLS option.
01g,10jul96,ja   added support for SH7700.
01f,23jan96,hk   changed bf/s machine code in csLongLoop to use bf.s.
01e,18dec95,hk   added support for SH704X.
01d,19may95,hk   tweaking a little. worked around 'mova' alignment problem.
01c,15may95,hk   optimized.
01b,12may95,hk   mostly optimized.
01a,08may95,hk   written based on mc68k-01t.
*/

/* optimized version available for the SH family */

#if (defined(PORTABLE) || (CPU_FAMILY != SH))
#define unixALib_PORTABLE
#endif

#ifndef unixALib_PORTABLE

#define _ASMLANGUAGE
#include "vxWorks.h"
#include "asm.h"

	/* internals */

	.global	_cksum
	.global	__insque
	.global	__remque
#undef	DEBUG_LOCAL_SYMBOLS
#ifdef	DEBUG_LOCAL_SYMBOLS
	.global	csEvenSum
	.global	csEvenBoundary
	.global	csLongBoundary
	.global	csLongLoop
	.global	csStartLoop
	.global	csFoldAddT
	.global	csChkSwapped
	.global	csNotSwapped
	.global	csLastSum
	.global	csLastFold
#endif	/* DEBUG_LOCAL_SYMBOLS */

	.text

/**************************************************************************
*
* cksum - compute check sum
*
* return 16bit one's complement sum of 'sum' and the 16bit one's
* complement sum of the string 'string' of byte length 'len'.
* Complicated by 'len' or 'sumlen' not being even.

* #define FOLD { lBox.asLong = cSum;  cSum = lBox.asShort[0] + lBox.asShort[1];
*                cSum > 0xffff ? cSum -= 0xffff : cSum; }

* int cksum
*   (
*   int       cSum,			/@ previously calculated checksum  @/
*   u_short * mPtr,			/@ address of string to be summed  @/
*   int       mLen,			/@ length  of string to be summed  @/
*   int       sLen			/@ previously summed string length @/
*   )
*   {
*   int                                            swapped = 0;
*   union { char    asChar [2]; u_short asShort; } sBox;
*   union { u_short asShort[2]; long    asLong;  } lBox;
*
*   if (sLen & 0x1)
*	{
*	sBox.asChar[0] = 0;
*	sBox.asChar[1] = *(char *)mPtr;		/@ 		sBox [0:B] @/
*	cSum += sBox.asShort;			/@ 		sBox [_:_] @/
*	mPtr = (u_short *)((char *)mPtr + 1);
*	mLen--;
*	}
*   if ((1 & (int)mPtr) && (mLen > 0))		/@ odd boundary            @/
*	{					/@                         @/
*	FOLD; cSum <<= 8; swapped = 1;		/@ swap byte order         @/
*						/@                         @/
*	sBox.asChar[0] = *(u_char *)mPtr;	/@ 		sBox [X:_] @/
*	mPtr = (u_short *)((char *)mPtr + 1);	/@                         @/
*	mLen--;					/@ force even boundary     @/
*	}
*   while ((mLen -= 2) >= 0) cSum += *mPtr++;	/@ sum up                  @/
*						/@ mLen: -2 or -1          @/
*   if (swapped)
*	{					/@ sum is byte-swapped     @/
*	FOLD; cSum <<= 8; swapped = 0;		/@ return to normal order  @/
*
*	if (mLen == -1)
*	    {					/@ swapped, mLen was even. @/
*	    sBox.asChar[1] = *(char *)mPtr;	/@ 		sBox [X:Y] @/
*	    cSum += sBox.asShort;		/@ 		sBox [_:_] @/
*	    }
*	else{					/@ swapped, mLen was odd.  @/
*	    sBox.asChar[1] = 0;			/@ 		sBox [X:0] @/
*	    cSum += sBox.asShort;		/@ 		sBox [_:_] @/
*	    }
*	}
*   else{
*	if (mLen == -1)
*	    {					/@ not swapped, mLen was odd. @/
*	    sBox.asChar[0] = *(char *)mPtr;	/@                            @/
*	    sBox.asChar[1] = 0;			/@ 		sBox [A:0]    @/
*	    cSum += sBox.asShort;		/@ 		sBox [_:_]    @/
*	    }
*	else;					/@ not swapped, mLen was even.@/
*	}
*   FOLD;
*   return (cSum & 0xffff);
*   }

*/
	.align	_ALIGN_TEXT
	.type	_cksum,@function
					/* r4: cSum      */
					/* r5: mPtr      */
					/* r6: mLen (>0) */
_cksum:					/* r7: sLen      */
	mov	r7,r0
	mov	#0,r7			/* r7: swapped = 0 */
	tst	#0x1,r0
	bt	csEvenSum

	mov.b	@r5+,r1;
	add	#-1,r6			/* r6: mLen--     */
	extu.b	r1,r1			/* r1: 0x000000XX */
#if (_BYTE_ORDER == _LITTLE_ENDIAN)
	shll8	r1			/* r1: 0x0000XX00 */
#endif
	add	r1,r4			/* r4: 0x0001ZZZZ */
csEvenSum:				/* r6: mLen (>=0) */
	mov	r5,r0			/* r5: mPtr */
	tst	#0x1,r0
	bt	csEvenBoundary		/*     no swap */

	cmp/pl	r6			/* r6: mLen */
	bf	csEvenBoundary		/*     no swap */

	/*-- odd boundary, do swap --*/	/* r4: 0x0001ZZZZ */
	shll8	r4			/* r4: 0x01ZZZZ00 */
	extu.w	r4,r2			/* r2: 0x0000ZZ00 */
	shlr16	r4			/* r4: 0x000001ZZ */
	add	r2,r4			/* r4: 0x0001VVVV */
	mov	#1,r7
	rotr	r7			/* r7: 0x80000000 (swapped) */
	mov.b	@r5+,r3
	add	#-1,r6			/* r6: mLen-- */
	extu.b	r3,r3
#if (_BYTE_ORDER == _BIG_ENDIAN)
	shll8	r3			/* r3: 0x0000XX00 */
#endif
	or	r3,r7			/* r7: 0x8000XX00 (swapped | sBox) */
	/*-- forced even, swapped --*/

csEvenBoundary:
	mov	r5,r0			/* r0: mPtr */
	tst	#0x2,r0			/*     long boundary?               */
	bt	csLongBoundary		/*     if yes, go to long-add stage */

	add	#-2,r6			/* r6: mLen -= 2 */
	mov	r6,r0			/* r0: mLen (used at csChkSwapped) */
	cmp/pz	r6
	bf	csChkSwapped

	mov.w	@r5+,r1
	extu.w	r1,r1
	add	r1,r4			/*     overflow not possible */

csLongBoundary:				/* r6: mLen                      */
	mov	r6,r0			/*     find fractions of a chunk */
	and	#0x3c,r0
	neg	r0,r2			/* r2: offset from ckStartLoop */
	mova	csStartLoop,r0		/* (csStartLoop must be long aligned) */
	add	r0,r2
	mov	r6,r0
	and	#0x3,r0			/* r0: mLen % 4 */
#if (CPU==SH7000 || CPU==SH7600)
	shlr2	r6
	shlr2	r6
	shlr2	r6
#else
	mov	#-6,r1
	shld	r1,r6			/* r6: mLen/64 */
#endif
	add	#1,r6			/* r6: mLen/64 + 1 */
	jmp	@r2;
	clrt				/*     clear T */
					/* r7: (swapped | sBox) */
					/* r6: mLen/64 + 1      */
					/* r5: mPtr             */
					/* r4: cSum             */
					/* r3: T-bit storage    */
					/* r2:                  */
					/* r1:                  */
					/* r0: mLen % 4         */
	.align	2
#if (CPU==SH7000)
	nop
csLongLoop:
	rotr	r3					/* restore T */
#else
csLongLoop:
#endif
	mov.l	@r5+,r1;	addc	r1,r4
	mov.l	@r5+,r1;	addc	r1,r4
	mov.l	@r5+,r1;	addc	r1,r4
	mov.l	@r5+,r1;	addc	r1,r4
	mov.l	@r5+,r1;	addc	r1,r4
	mov.l	@r5+,r1;	addc	r1,r4
	mov.l	@r5+,r1;	addc	r1,r4
	mov.l	@r5+,r1;	addc	r1,r4
	mov.l	@r5+,r1;	addc	r1,r4
	mov.l	@r5+,r1;	addc	r1,r4
	mov.l	@r5+,r1;	addc	r1,r4
	mov.l	@r5+,r1;	addc	r1,r4
	mov.l	@r5+,r1;	addc	r1,r4
	mov.l	@r5+,r1;	addc	r1,r4
	mov.l	@r5+,r1;	addc	r1,r4
	mov.l	@r5+,r1;	addc	r1,r4
csStartLoop:
	movt	r3					/* save T */
#if (CPU==SH7000)
	add	#-1,r6
	tst	r6,r6
	bf	csLongLoop
#else
	dt	r6
	bf.s	csLongLoop
	rotr	r3			/*     restore T */
	rotl	r3			/*     save T */
#endif
					/* r0: mLen ( 0,  1, 2, 3) */
	add	#-2,r0			/* r0: mLen (-2, -1, 0, 1) */
	cmp/pz	r0
	bf	csFoldAddT
					/* r0: mLen ( 0,  1) */
	mov.w	@r5+,r1
	rotr	r3			/*     restore T */
	extu.w	r1,r1
	addc	r1,r4
	movt	r3			/*     save T */
	add	#-2,r0
csFoldAddT:				/* r0: mLen (-2, -1) */
	extu.w	r4,r2			/* r2: 0x0000FFFF */
	shlr16	r4			/* r4: 0x0000FFFF */
	add	r2,r4			/* r4: 0x0001FFFE */
	add	r3,r4			/* r4: 0x0001FFFF */

csChkSwapped:				/* r0: mLen (-1, -2) */
	cmp/pz	r7			/* r7: swapped ?  */
	extu.w	r7,r3			/* r3: 0x0000XX00 */
	bt	csNotSwapped
					/* r4: 0xFFFFFFFF */	/* unswap */
	extu.w	r4,r2			/* r2: 0x0000FFFF */
	shlr16	r4			/* r4: 0x0000FFFF */
	add	r2,r4			/* r4: 0x0001FFFE */
	shll8	r4			/* r4: 0x01FFFE00 */

	cmp/eq	#-1,r0			/* r0:(mLen == -1)? */
	bf	csLastSum		/* r3: 0x0000XX00 */

	mov.b	@r5,r1
	extu.b	r1,r1			/* r1: 0x000000YY */
#if (_BYTE_ORDER == _LITTLE_ENDIAN)
	shll8	r1			/* r3: 0x0000YY00 */
#endif
	bra	csLastSum;
	or	r1,r3			/* r3: 0x0000XXYY */

csNotSwapped:
	cmp/eq	#-1,r0			/* r0:(mLen == -1)? */
	bf	csLastFold

	mov.b	@r5,r3
	extu.b	r3,r3
#if (_BYTE_ORDER == _BIG_ENDIAN)
	shll8	r3			/* r3: 0x0000AA00 */
#endif
csLastSum:
	add	r3,r4			/* r4: cSum += sBox.asShort   */
					/*     overflow not possible. */
csLastFold:				/* r4: 0xFFFFFFFF */
	extu.w	r4,r2			/* r2: 0x0000FFFF */
	shlr16	r4			/* r4: 0x0000FFFF */
	add	r2,r4			/* r4: 0x0001FFFE */

	extu.w	r4,r0			/* r0: 0x0000FFFE */
	shlr16	r4			/* r4: 0x00000001 */
	rts;
	add	r4,r0			/* r0: 0x0000FFFF */

/****************************************************************************
*
* insque - insert a node into a linked list
*
*/
	.align	_ALIGN_TEXT
	.type	__insque,@function

				/* r4: pNode				*/
__insque:			/* r5: pPrev				*/
	mov.l   @r5,r0;		/* r0: pNext           = pPrev->next	*/
	mov.l   r4,@r5		/*     pPrev->next     = pNode		*/
	mov.l   r4,@(4,r0)	/*     pNext->previous = pNode		*/
	mov.l   r0,@r4		/*     pNode->next     = pNext		*/
	rts;			/*					*/
	mov.l   r5,@(4,r4)	/*     pNode->previous = pPrev		*/

/****************************************************************************
*
* remque - delete a node from a linked list
*
* NOTE:	This routine is only used by remque() in h/net/systm.h
*/
	.align	_ALIGN_TEXT
	.type	__remque,@function

__remque:		       /* r4: pNode				      */
	mov.l   @r4,r0;	       /* r0: pNode->next			      */
	mov.l   @(4,r4),r1;    /* r1: pNode->previous			      */
	mov.l   r0,@r1	       /*     pNode->previous->next = pNode->next     */
	rts;		       /*					      */
	mov.l   r1,@(4,r0)     /*     pNode->next->previous = pNode->previous */

#endif	/* !unixALib_PORTABLE */
