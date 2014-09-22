/* fmod.c - fmod math routine */

/* Copyright 1992-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
03b,01apr02,pfl  clean up fix for SPR 20486 plus 
                 ISO/IEC 9899:1999 conformance
02a,31oct01,md3  +gkc +ycl rewrite: fixed SPR #20486.
01f,05feb93,jdi  doc changes based on kdl review.
01e,02dec92,jdi  doc tweaks.
01d,28oct92,jdi  documentation cleanup.
01c,20sep92,smb  documentation additions
01b,30jul92,kdl  changed _d_type() calls to fpTypeGet().
01a,08jul92,smb  documentation.
*/

/*
DESCRIPTION
SEE ALSO: American National Standard X3.159-1989
NOMANUAL
*/

#include "vxWorks.h"
#include "math.h"
#include "stddef.h"
#include "errno.h"
#include "private/mathP.h"
#include "errnoLib.h"

/* mask to extract exponent part of a double */

#define  MASK_DB_EXP  0x7ff00000

/* mask to extract high 20 significant bits */

#define  MASK_DB_HI20  0x000fffff

/* mask to filter exponent part of double */

#define  MASK_DB_MANT  0x800fffff

/* for big endian machine */

#if (_BYTE_ORDER==_BIG_ENDIAN)   

/* high 32 bits of Db - 1-bit sign + 11-bits exponent + 20-bit significant */

#define DB_HI(d) ((unsigned int) *((int*)&d))

/* hi 20 bits of a double's significant */

#define DB_HI_SIG(d) ((unsigned int) *((int*)&d) & MASK_DB_HI20)  

/* lower 32 bits of double's significant */

#define DB_LO_SIG(d) ((unsigned int) *(1+(int*)&d))      

/* for little endian machine */

#else /* (_BYTE_ORDER != _BIG_ENDIAN) */  

/* high 32 bits of Db - 1-bit sign + 11-bits exponent + 20-bit significant */

#define DB_HI(d) ((unsigned int) *(1+(int*)&d))      

/* hi 20 bits of a double's significant */

#define DB_HI_SIG(d) ((unsigned int) *(1+(int*)&d) & MASK_DB_HI20)  

/* lower 32 bits of double's significant */

#define DB_LO_SIG(d) ((unsigned int) *((int*)&d))                           

#endif /* (_BYTE_ORDER==_BIG_ENDIAN) */

/* return (significant of d1) > (significant of d2) */

#define DB_GT_SIG(d1,d2) \
        ((DB_HI_SIG(d1) > DB_HI_SIG(d2)) || \
         ((DB_HI_SIG(d1) == DB_HI_SIG(d2)) && (DB_LO_SIG(d1) > DB_LO_SIG(d2))))

/* NAN value */

#define NAN_VAL(d) {*(int *)(&d) = 0xffffffff; *(1+(int *)&d)=0xffffffff;}

/*******************************************************************************
*
* fmod - compute the remainder of x/y (ANSI)
*
* This routine returns the remainder of <x>/<y> with the sign of <x>,
* in double precision.
* 
* INCLUDE FILES: math.h
*
* RETURNS: The value <x> - <i> * <y>, for some integer <i>.  If <y> is
* non-zero, the result has the same sign as <x> and magnitude less than the
* magnitude of <y>.  If <y> is zero, fmod() returns zero.
*
* ERRNO: EDOM
*
* SEE ALSO: mathALib
*/

double fmod
    (
    double x,    /* numerator   */
    double y     /* denominator */
    )
    {
    double       negative = 1.0;                /* used for sign of return   */
    double       yMult2n;   	                /* computed next decrement   */
    unsigned int zeroExp_y;            		/* factor for next decrement */

    int          errx = fpTypeGet (x, NULL);    /* determine number type */
    int          erry = fpTypeGet (y, NULL);    /* determine number type */

    /* 
     * Per ISO/IEC 9899:1999, ANSI/IEEE 754-1985
     * fmod ( +-0,     y) = +-0 (if y != 0)
     * fmod (   x,     y) = NAN (if x  = INF or y = 0) 
     * fmod (   x, +-INF) = x   (if x != INF)
     */
   
    /* check for boundary conditions returning NAN */
    
    if (errx == NAN || erry == NAN || errx == INF || erry == ZERO)
        {
        errnoSet(EDOM);         /* Domain error is set to maintain backward
				   compatibility with ANSI X3.159-1989 */
	NAN_VAL(x);
        return (x);		/* return NAN */
        }

    /* check for boundary conditions returning X */

    if (errx == ZERO || erry == INF)
        return (x);

    /* make Y absolute */

    if (y < 0.0)
        y = -y;

    /* make X absolute, record sign for return. */

    if (x < 0.0)
        {
        x = -x;
        negative = -1.0;
        }

    /* initial decrementing conditions */

    yMult2n = y;
    zeroExp_y= MASK_DB_MANT & (DB_HI(y));

    /* compute fmod by subtracting estimated decrementations */

    while (x>=y) 
        {
        if (DB_GT_SIG(y,x))
            {
             (DB_HI(yMult2n)) = ( (DB_HI(x)-0x100000) & MASK_DB_EXP ) | zeroExp_y;
             }
         else
             {
             (DB_HI(yMult2n)) = ( DB_HI(x) & MASK_DB_EXP ) | zeroExp_y;
             }
         x-= yMult2n ;
         }
    return (negative*x);
    }
