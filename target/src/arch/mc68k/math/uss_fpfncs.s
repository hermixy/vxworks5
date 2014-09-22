/* Copyright 1991-1992 Wind River Systems, Inc. */
	.data
	.globl	_copyright_wind_river
	.long	_copyright_wind_river

/*
modification history
--------------------
01f,23aug92,jcf  changed bxxx to jxx.
01e,05jun92,kdl  changed external branches to jumps (and bsr's to jsr's).
01d,26may92,rrr  the tree shuffle
01c,30mar92,kdl  added include of "uss_fp.h"; commented-out ".set" directives
		 (SPR #1398).
01b,04oct91,rrr  passed through the ansification filter
		  -changed ASMLANGUAGE to _ASMLANGUAGE
		  -changed copyright notice
01a,28jan91,kdl  original US Software version.
*/


/*
DESCRIPTION

|       ttl     FPAC 68K/FPFNCS: IEEE Single Precision Functions
|FPFNCS idnt    1,0             ; IEEE Single Precision Functions
|                               ; FPFNCS.A68
|
|* * * * * * * * * *
|
|       68000 FPAC  --  Single Precision Functions
|
|       Copyright (C) 1984 - 1990 By
|       United States Software Corporation
|       14215 N.W. Science Park Drive
|       Portland, Oregon 97229
|
|       This software is furnished under a license and may be used
|       and copied only in accordance with the terms of such license
|       and with the inclusion of the above copyright notice.
|       This software or any other copies thereof may not be provided
|       or otherwise made available to any other person.  No title to
|       and ownership of the software is hereby transferred.
|
|       The information in this software is subject to change without
|       notice and should not be construed as a commitment by United
|       States Software Corporation.
|
|       Version:        See VSNLOG.TXT
|       Released:       01 Jan 1984
|
|* * * * * * * * * *
|
|
|  Single Precision Floating Point Function library.
|
|  These functions operate on single precision floating point values,
|  yielding single precision floating point results.  All functions
|  process the value on top of the stack and return a result on the
|  stack.  The XTOI function expects its integer argument in D0.
|
|
|  NAME       ARGUMENT                DESCRIPTION
|  ========== =====================   =================================
|  EXP        -87.43 < a < 88.72      e to the power specified.
|                                     (if out of range, 0.0 or +INF)
|
|  LN         a > 0.0                 natural log of value specified.
|                                     (invalid opn if neg or zero)
|
|  XTOI       all value pairs         number raised to integer power.
|
|  ATN        any numeric value       arc whose tanget is given value
|                                     (no domain restriction, result in
|                                     the range is -PI/2 to +PI/2)
|
|  SQRT       a >= 0.0                square root of the given value
|                                     (invalid operation if negative)
|
|  COS                                All trigonometric functions
|  SIN                                expect a radian mode argument
|  TAN                                whose magnitude is < 65536 (2^16)
|

NOMANUAL
*/

#define	_ASMLANGUAGE

#include "uss_fp.h"

|       page
|
|       opt     BRS             ; Default to forward branches SHORT
|
        .globl  FPEXP
        .globl  FPLN
        .globl  FPLOG
        .globl  FPXTOI
        .globl  FPATN
        .globl  FPSQRT
        .globl  FPCOS
        .globl  FPSIN
        .globl  FPTAN
|
|       xref    FPMUL   ;Floating point operations
|       xref    FPDIV
|       xref    FPRDIV
|       xref    FPADD
|       xref    FLOAT
|       xref    INT
|
|       xref    FNANRS
|       xref    FINFRS
|       xref    FUNFRS
|       xref    FZERRS
|
|       xref    FPERR
|
|
|       .set    FBIAS,127
|
|       .set    ERUNF,1 |FPERR UNDERFLOW ERROR CODE
|       .set    EROVF,2 |FPERR OVERFLOW ERROR CODE
|       .set    ERNAN,3 |FPERR INVALID OPERATION ERROR CODE
/*
|
|
|
| ###   SUBTTL  FPEXP: Exponentiation of e Function
|       page
|
|
|  EXP:   Calculate e to the power specified by the stack argument.
|         The result is returned on the stack.
|
|         FPERR is set upon overflow, underflow, or NaN argument.
|
|  Algorithm:
|  The argument is multiplied by 1/LN 2.  The integer portion of the
|  product becomes the two*s exponent.  A Taylor series approximation
|  about zero is used to compute 2**FRAC(ARG * 1/LN 2).
|
*/

        .text
|
FEXPCN:
        |dsw    0       | EXP CONSTANT LIST (2**X EVALUATION)
|
|         (LN 2)^ 9/ 9!   =   1.0178 08601E-07
        .long   0x33DA929E
|         (LN 2)^ 8/ 8!   =   1.3215 48679E-06
        .long   0x35B16011
|         (LN 2)^ 7/ 7!   =   1.5252 73380E-05
        .long   0x377FE5FE
|         (LN 2)^ 6/ 6!   =   1.5403 53039E-04
        .long   0x39218489
|         (LN 2)^ 5/ 5!   =   1.3333 55815E-03
        .long   0x3AAEC3FF
|         (LN 2)^ 4/ 4!   =   9.6181 29108E-03
        .long   0x3C1D955B
|         (LN 2)^ 3/ 3!   =   5.5504 10866E-02
        .long   0x3D635847
|         (LN 2)^ 2/ 2!   =   2.4022 65070E-01
        .long   0x3E75FDF0
|         (LN 2)^ 1/ 1!   =   6.9314 71806E-01
FLN2:
        .long   0x3F317218
|                             1.0000 00000E+00
        .long   0x3F800000
|
|       .set    FNEXPC,10               | Number of constants in FEXP poly
|
|         1 / LN 2  =  1.4426 95040 88896 34074 D+00
FINLN2:
        .long   0x3FB8AA3B
|
|
|
| ###   PUBLIC  FPEXP
|
FPEXP:
        |dsw    0
        bsr     FPFADJ          | Set-up for FP functions
|
        jvs     FFNANR          | NaN parm -> NaN result
        jcc     FEXP01          | J/ arg not INF
|
        jmi     FFUNFR          | -INF parm -> underflow result
        jra     FFPINR          | +INF parm -> positive INF result
|
FEXP01:
        movel   FINLN2,sp@-     | 1/LN 2 on stack
        jsr     FPMUL           | Convert to 2^ function
|
        movew   sp@,d1
        rorw    #7,d1           | Exponent in d1.B
        cmpib   #FBIAS+6,d1
        jhi     FEXP10          | J/ overflow or underflow  >= +/- 2^7
        jne     FEXP20          | J/ within range            < +/- 2^6
        tstb    sp@
        jpl     FEXP20          | J/ result >= 1.0
|
        cmpiw   #0xF800+0x0100+FBIAS+6,d1       | Check ms 7 man bits (+s+exp)
        jcs     FEXP20                  | J/ result >= 2^-126
|
FEXP10:
        tstb    sp@
        jpl     FFPINR          | J/ FP function +INF result
        jra     FFUNFR          | J/ FP function underflow result
|
FEXP20:
        movel   sp@,sp@-        | Duplicate the argument on the stack
|
        jsr     INT
|
        movew   d0,a7@(4)       | Save result of INT
|
        negl    d0              | Negate integer
        jsr     FLOAT
        jsr     FPADD           | Special FFRAC
|
        pea     FEXPCN
        movel   sp@+,a6 | Point a6 to constant list
        moveq   #FNEXPC,d0      | d0 holds the number of constants
        bsr     FXSER           | Polynomial series
|
        movew   a7@(4),d0       | Fetch power of two scaling
        lslw    #7,d0
        addw    d0,sp@          | Combine with FXSER result
|
        movel   sp@+,sp@        | Downshift result
        jmp     a4@             | Return

/*
|
| ###   SUBTTL  FPLOG, FPLN: Logarithm Functions
|       page
|
|  LOGARITHM (BASE E) FUNCTION.
|
|  The natural logarithm of the single precision floating point
|  value on the stack is computed using a split domain polynomial
|  approximation.  The common log is computed by scaling the
|  result of the natural log computation.
|
|  TWOS = FLOAT(ARG.EXPONENT) * LOG(2)
|
|  The argument, (after the two's power scaling, 2.0 > arg' >= 1.0),
|  is sorted into one of five classes.  Within that class, a center
|  point with a known log value is used in combination with a 7th
|  degree Chebyshey polynomial approximation, to calculate a
|  logarithm.
|
|  The center point of each class is adjusted so that its natural
|  logarithm expressed in single precision form is accurate to
|  more than 40 bits.
|
|  log(arg) = two's log + center point log + polynomial approximation
|
|  Polynomial approximation:
|
|  log((1-t)/(1+t)) = t*(c1*t^8 + c2*t^6 + c3*t^4 + c4*t^2 + c5)
|         (accurate with 1.0E-17 for t within 0.1 of 1.0)
|
|  t = - (center - arg')/(center + arg')  [to calc log of arg'/center]
|
*/

FLNCNS:
        |dsw    0               | LN constants list
|
|         c1  =   0.28571 20487
        .long   0x3E9248DA
|         c2  =   0.40000 00019
        .long   0x3ECCCCCD
|         c3  =   0.66666 66667
        .long   0x3F2AAAAB
|         c4  =   2.00000 00000
        .long   0x40000000
|
|       .set    FNLNCN,4                | Number of constants in FLN poly
|
|
FLNCEN:
        |dsw    0               | Center Points
|         Center  #1 = 1.0000 00000 00000 00000
        .long   0x3F800000
|         Center  #2 = 1.1892 07098     (close to 2^(1/4))
        .long   0x3F9837F0
|         Center  #3 = 1.4142 13521     (close to 2^(1/2))
        .long   0x3FB504F3
|         Center  #4 = 1.6817 92733     (close to 2^(3/4))
        .long   0x3FD744FC
|         Center  #5 = 2.0000 00000
| ---     shifted to 1.0 center..
|
FLNLOG:
        |dsw    0               | FLNCEN values adj for exact ln values
|         LN(Center #2) = 0.17328 67807
        .long   0x3E317217
|         LN(Center #3) = 0.34657 35614
        .long   0x3EB17217
|         LN(Center #4) = 0.51986 03272
        .long   0x3F051591
|
|
|         1 / LN 10  =  0.43429 44819
FILN10:
        .long   0x3EDE5BD9
|
|
|
| ###   PUBLIC  FPLN
| ###   PUBLIC  FPLOG
|
FPLOG:
        |dsw    0
        moveq   #-1,d0          | Signal common log scaling
        jra     FLN000
|
FPLN:
        |dsw    0
        clrb    d0              | Signal no post calculation scaling
|
FLN000:
        bsr     FPFADJ          | Adjust parameter on stack
|
        jvs     FFNANR          | J/ NaN arg -> NaN result
        jmi     FFNANR          | J/ Neg arg -> NaN result
        jeq     FFMINR          | J/ 0.0 arg -> -INF result
        jcs     FFPINR          | J/ +INF arg -> +INF result
|
        moveb   d0,a7@(6)       | Save natural/common flag
|
        movew   sp@,d1          | Prepare to calc parm sign and exp
        lsrw    #7,d1           | Position exponent (right justified)
        subiw   #FBIAS,d1
        movew   d1,a7@(4)       | /* Save two's exponent value */
|
        lslw    #7,d1           | Scale parameter
        subw    d1,sp@
|
        clrw    d0              | Set class number to 0
        movel   sp@,d1
        lsrl    #8,d1           | d1.W = 0xEMMM
|
        cmpiw   #0x8000+2966,d1
        jcs     FLN050          | J/ < 1 +  2966/32768  (1.090515)
        addqw   #4,d0
        cmpiw   #0x8000+9727,d1
        jcs     FLN050          | J/ < 1 +  9727/32768  (1.296844)
        addqw   #4,d0
        cmpiw   #0x8000+17767,d1
        jcs     FLN050          | J/ < 1 + 17767/32768  (1.542206)
        addqw   #4,d0
        cmpiw   #0x8000+27329,d1
        jcs     FLN050          | J/ < 1 + 27329/32768  (1.834015)
        addqw   #1,a7@(4)       | /* Class 4: Bump two's exp, then class 0 */
        clrw    d0
        subib   #0x80,a7@(1)    | Reduce scaled exponent
|
FLN050:
        moveb   d0,a7@(7)       | Save center number
|
        pea     FLNCEN
        movel   sp@+,a2
        movel   a2@(0,d0:W),sp@- |Place center on stack
|
        movel   a7@(4),sp@-     | Copy scaled argument
|
        movel   a2@(0,d0:W),sp@- |Place center on stack
|
        jsr     FPADD           | /* Calc  (CENTER + PARM') */
|
        movel   sp@,d0          | Exch top stack item w/ 3rd
        movel   a7@(8),d1
        movel   d1,sp@
        movel   d0,a7@(8)
|
        bset    #7,a7@(4)       | Negate CENTER
|
        jsr     FPADD           | /* Calc  - (CENTER - PARM') */
        jsr     FPRDIV          | /* T = - (CENTER-PARM')/(CENTER+PARM) */
|
        movel   sp@,sp@-        | Duplicate t
|
        pea     FLNCNS
        movel   sp@+,a6 | Poly approx
        moveq   #FNLNCN,d0
        bsr     FX2SER
|
        jsr     FPMUL           | Times t
        clrw    d0
        moveb   a7@(7),d0
        jeq     FLN060          | J/ center = 0 -> no additive log val
|
        pea     FLNLOG-4
        movel   sp@+,a6
        movel   a6@(0,d0:W),sp@-
        jsr     FPADD           | Add in center log value
|
FLN060:
        movew   a7@(4),d0       | /* Get two's exponent value */
        extl    d0
| ##    DC.W    $48C0           ; (Assembled  EXT.L D0 instruction)
        jsr     FLOAT
|
        movel   FLN2,sp@-       | Log of 2 on stack
        jsr     FPMUL
        jsr     FPADD
|
        tstb    a7@(6)
        jeq     FLN070          | J/ natural log
|
        movel   FILN10,sp@-     | Scaling value
        jsr     FPMUL
|
FLN070:
        movel   sp@+,sp@
        jmp     a4@
/*
| ###   SUBTTL  FPXTOI: Floating Point Number to Integer Power Function
|       page
|
|  X to I power Function.
|
|  The single precision floating point value on the stack is
|  raised to the power specified in D0.W then returned on stack.
|
|  A shift and multiply technique is used (possibly with a trailing
|  recipication.
|
|
| ###   PUBLIC  FPXTOI
|
*/

FPXTOI:
        |dsw    0
        bsr     FPFADJ
|
        jvs     FFNANR          | J/ NaN arg -> NaN result
        jcc     FPXT10          | J/ arg is not INF
|
        tstw    d0
        jeq     FFNANR          | J/ arg is +/- INF, I is 0 -> NaN
        jmi     FFUNFR          | J/ x is +/- INF, I < 0 -> underflow
|
        rorb    #1,d0
        andb    sp@,d0
        jmi     FFMINR          | J/ arg is -INF, I is +odd -> -INF
        jra     FFPINR          | else -> +INF
|
|
FPXT10:
        jne     FPXT15          | J/ parm is a number <> 0.0
|
        tstw    d0
        jmi     FFPINR          | J/ 0.0 to -int -> +INF
        jeq     FFNANR          | J/ 0.0 to 0 -> NaN
        jra     FFZERR          | J/ 0.0 to +int -> 0.0
|
FPXT15:
        tstw    d0
        jeq     FFONER          | J/ num to 0 -> 1.0
|
        movew   d0,a7@(6)       | Save int as its own sign flag
        jpl     FPXT20
        negw    d0
FPXT20:
        |dsw    0
|
        movel   sp@,sp@-        | Result init to parm value
|
        moveq   #16,d1          | Find MS bit of power
FPXT21:
        lslw    #1,d0
        jcs     FPXT22          | J/ MS bit moved into carry
        dbra    d1,FPXT21       | Dec d1 and jump (will always jump)
|
FPXT22:
        movew   d0,a7@(8)       | Power pattern on stack
        moveb   d1,a7@(11)      | Bit slots left count on stack
|
|
FPXT30:
        subqb   #1,a7@(11)      | Decrement bit slots left
        jeq     FPXT35          | J/ evaluation complete
|
        movel   sp@,sp@-        | Square result value
        jsr     FPMUL           | Square it
|
        lslw    a7@(8)          | Shift power pattern
        jcc     FPXT30          | J/ this product bit not set
|
        movel   a7@(4),sp@-     | Copy parm value
        jsr     FPMUL
        jra     FPXT30
|
FPXT35:
        tstb    a7@(10)         | Check for recipocation
        jpl     FPXT36          | J/ no recipocation
|
        movel   #0x3F800000,sp@- |Place 1.0 on stack
        jsr     FPRDIV
|
FPXT36:
        movel   sp@+,a7@(4)     | Shift result value
        addql   #4,sp           | Delete excess stack area
        jmp     a4@             | Return.
/*
|
| ###   SUBTTL  FPSQRT: Square Root Function
|       page
|
|  Square Root Function.
|
|  Take square root of the single precision floating point value
|  on the top of the stack.
|
|  Use the Newton iteration technique to compute the square root.
|
|      X(n+1) = (X(n) + Z/X(n)) / 2
|
|  The two*s exponent is scaled to restrict the solution domain to 1.0
|  through 4.0.  A linear approximation to the square root is used to
|  produce a first guess with greater than 4 bits of accuracy.  Three
|  successive iterations are performed in registers to obtain accuracy
|  of about 30 bits.  The final iteration is performed in the floating
|  point domain.
|
| ###   PUBLIC  FPSQRT
|
*/

FPSQRT:
        |dsw    0
        bsr     FPFADJ
|
        jvs     FFNANR          | J/ NaN arg -> NaN result
        jmi     FFNANR          | J/ neg arg -> NaN result
        jcs     FFPINR          | J/ +INF arg -> +INF result
        jeq     FFZERR          | J/ 0.0 arg -> 0.0 result
|
        movew   sp@,d1          | Get S/E/M word
        subiw   #128*FBIAS,d1   | Extract argument's two's exp
        clrb    d1              | Make it a factor of two
        subw    d1,sp@          | /* Scale arg. range to 4.0 > arg' >= 1.0 */
        asrw    #8,d1           | Square root of scaled two power
        moveb   d1,a7@(4)       | /* Save two's exp of result on stack */
|
        movel   sp@,d1          | Create fixed point integer for approx
        lsll    #8,d1           | /* Produce arg' * 2^30 in d1 */
        bset    #31,d1          | Set implicit bit
        jeq     FPSQ10          | /* J/ arg' >= 2.0 */
|
        lsrl    #1,d1           | Adjust d1
|
FPSQ10:
        movew   #42720-65536,d2 | d2 = 0.325926 * 2^17
        swap    d1              | /* d1.W = arg' * 2^14 */
        mulu    d1,d2           | /* d2 = arg' * 0.325926 * 2^31 */
        swap    d2
        addiw   #23616,d2       | + 0.7207 * 2^15 - to 4+ bits
        subxw   d3,d3
        orw     d3,d2           | Top out approximation at 1.99997
|
        swap    d1
        lsrl    #1,d1           | /* Arg' * 2^29 in d1 (prevent overflow) */
|
        movel   d1,d3           | Copy into d3
        divu    d2,d3           | /* Arg'/X0 * 2^14 in d3 */
        lsrw    #1,d2
        addw    d3,d2           | X1 in d2 - to 8 bits
|
        movel   d1,d3           | Second in-register iteration
        divu    d2,d3
        lsrw    #1,d2
        addw    d3,d2           | X2 in d2 - to 16 bits
|
        movel   d1,d3
        divu    d2,d3
        movew   d3,d4
        clrw    d3
        swap    d4
        divu    d2,d3
        movew   d3,d4           | 32 bit division result
        swap    d2
        clrw    d2
        lsrl    #1,d2
        addl    d4,d2           | X3 in d2 - to 24+ bits
|
        moveq   #0x7F,d1
        addl    d1,d2           | Round value in d2 to 24 bits
        roll    #1,d2           | Strip implicit bit
|       .set    XBIAS,127               | *** for assembler bug ***
        moveb   #XBIAS,d2       | Place bias value
        addb    a7@(4),d2       | Scale result
        rorl    #8,d2           | Position FP value
        lsrl    #1,d2           | Force sign bit to 0
        addql   #4,sp           | Delete four bytes from the stack
        movel   d2,sp@
        jmp     a4@
/*
|
| ###   SUBTTL  FPATN: Arctangent Function
|       page
|
|  ARCTANGENT Function.
|
|  The arctangent of the single precision floating point value on
|  the stack computed by using a split domain with a polynomial
|  approximation.  A principal range radian value is returned.
|
|  The domain is split at 0.25 (four) intervals.  A polynomial is
|  used to approximate the arctangent for magnitudes less than 1/8.
|
|  Using the trigonometric identity:
|         ARCTAN(y) + ARCTAN(z) = ARCTAN((y+z)/(1+yz))
|         If z = (x-y)/(1+xy) then ARCTAN((y+z)/(1+yz)) = ARCTAN(x).
|
|         ARCTAN(-v) = -ARCTAN(v)        * make argument positive
|         ARCTAN(1/v) = PI/2 - ARCTAN(v) * reduce argument to <= 1.0
|
|
|  ARCTANGENT approximation polynomical coefficients
|
|         C3  =   -1.4285 71429E-01     = -1/7
*/

FATNCN:
        .long   0xBE124925
|         C2  =    2.0000 00000E-01     =  1/5
        .long   0x3E4CCCCD
|         C1  =   -3.3333 33333E-01     = -1/3
        .long   0xBEAAAAAB
|         C0  =    1.0000 00000E+00     =  1/1
        .long   0x3F800000
|
|       .set    NFATNC,4
|
|
|
|  Table of ARCTANGENT values at 0.125 intervals
|
|         ATAN(1/4)  =  2.4497 86631E-01
FATNTB:
        .long   0x3E7ADBB0
|         ATAN(2/4)  =  4.6364 76090E-01
        .long   0x3EED6338
|         ATAN(3/4)  =  6.4350 11088E-01
        .long   0x3F24BC7D
|         ATAN(1/1)  =  7.8539 81634E-01   ( = PI/4)
        .long   0x3F490FDB
|
|
|  FP PI/2 (duplicate of FPIO2 because of assembler bug)
|
FXPIO2:
        .long   0x3FC90FDB
|
| ###   PUBLIC  FPATN
|
FPATN:
        |dsw    0
        bsr     FPFADJ
|
        jvs     FFNANR          | J/ NaN arg -> NaN result
        jcc     FPAT10          | J/ not INF
|
        movel   FXPIO2,d1       | Get PI/2
        roxll   #1,d1
        roxlw   sp@             | INF sign bit into X
        roxrl   #1,d1           | PI/2 given sign of INF
        addql   #4,sp           | Delete four bytes from the stack
        movel   d1,sp@
        jmp     a4@
|
FPAT10:
        jeq     FFZERR          | J/ 0.0 arg -> 0.0 result
|
        bclr    #7,sp@          | Insure argument positive
        sne     d0              | Create flag byte (0xFF iff negative)
        andib   #0x80,d0                | Keep sign bit only
        moveb   d0,a7@(6)       | Save flag byte
|
        movew   sp@,d1
        cmpiw   #128*FBIAS+0x10,d1      | (top word of 1.125)
        jlt     FPAT20                  | J/ arg < 1 + 1/8
|
        addqb   #1,a7@(6)
|
        movel   #0x3F800000,sp@- |Place 1.0 onstack
        jsr     FPRDIV          | Invert the number
|
FPAT20:
        movew   sp@,d1
        cmpiw   #128*FBIAS-256,d1
        jlt     FPAT30          | J/ arg < 1/4
|
        moveb   d1,d2           | Number of eights in d2
        orib    #0x80,d2                | Implicit bit
        lsrw    #7,d1
        moveq   #FBIAS+4-256,d3
        subb    d1,d3
        lsrb    d3,d2
        addqb   #1,d2
        lsrb    #1,d2           | Rounded quarters in d2.B
|
        clrl    d0
        moveb   d2,d0           | 32 bit integer in d0
|
        lslb    #2,d2
        addb    d2,a7@(6)       | Save 4 * quarters on stack
|
        jsr     FLOAT
        subiw   #128*2,sp@      | Produce y, floating point quarters
|
        movel   a7@(4),sp@-
        movel   a7@(4),sp@-     | On stack:  y, arg', y, arg', <temps>
|
        jsr     FPMUL           | /* arg'*y */
        movel   #0x3F800000,sp@-
        jsr     FPADD           | /* 1.0 + arg'*y */
|
        movel   a7@(8),d0       | Exchange stack items
        movel   sp@,a7@(8)
        movel   d0,sp@          | On stack: arg', y, (1+arg'*y), <tmps>
        bset    #7,a7@(4)       | Negate y
        jsr     FPADD           | /* (arg'-y) */
        jsr     FPRDIV          | (arg'-y) / (1 + arg'*y)
|
FPAT30:
        |dsw    0
        movel   sp@,sp@-        | Duplicate z
|
        pea     FATNCN
        movel   sp@+,a6 | Polynomial approximation to small ATN
        moveq   #NFATNC,d0
        bsr     FX2SER
        jsr     FPMUL           | Complete approximation
|
        moveb   a7@(6),d0
        andiw   #0x001C,d0      | Trim to table index
        jeq     FPAT40          | J/ y = 0.0, ARCTAN(y) = 0.0
|
        pea     FATNTB-4
        movel   sp@+,a6
        movel   a6@(0,d0:W),sp@-
        jsr     FPADD           | Add in ARCTAN(y)
|
FPAT40:
        btst    #0,a7@(6)       | Check for inversion
        jeq     FPAT50          | J/ no inversion
|
        bset    #7,sp@          | Negate ARCTAN
        movel   FXPIO2,sp@-
        jsr     FPADD           | Inversion via subtraction
|
FPAT50:
        tstb    a7@(6)          | Check sign of result
        jpl     FPAT60          | J/ positive
|
        bset    #7,sp@          | Negate result
|
FPAT60:
        movel   sp@+,sp@        | Downshift result
        jmp     a4@
/*
|
| ###   SUBTTL  FPCOS, FPSIN, FPTAN: Trigonometric Functions
|       page
|
|  TRIG ROUTINES.
|
|  The support routine FTRGSV converts the radian mode argument to
|  an quadrant value between -0.5 and 0.5 (quadrants).  The sign of the
|  is saved (the argument is forced non-negative).  Computations are
|  performed for values quadrant values of -0.5 to 0.5* other values
|  are transformed as per the following transformation formulae
|  and table:
|
|     Let:     z = shifted quadrant value
|     Recall:  sin(-x) = -sin(x)    tan(-x) = -tan(x)
|              cos(-x) =  cos(x)
|
|     l========l=================l=================l=================l
|     l  QUAD  l       SIN       l       COS       l       TAN       l
|     l========l=================l=================l=================l
|     l    0   l      sin(z)     l      cos(z)     l      tan(z)     l
|     l    1   l      cos(z)     l     -sin(z)     l   -1/tan(z)     l
|     l    2   l     -sin(z)     l     -cos(z)     l      tan(z)     l
|     l    3   l     -cos(z)     l      sin(z)     l   -1/tan(z)     l
|     l========l=================l=================l=================l
|
|  The sign of the argument is, by the equations above, important only
|  when computing SIN and TAN.
|
|  Chebyshev Polynomials are used to approximate the trigonometric
|  value in the range -0.5 to 0.5 quandrants.  (Note: constants are
|  adjusted to reflect calculations done in quadrant mode instead
|  of radian mode).
|
|
|
|         C08  =   9.1926 02748E-04
*/

FCOSCN:
        .long   0x3A70FA83
|         C06  =  -2.0863 48076E-02
        .long   0xBCAAE9E4
|         C04  =   2.5366 95079E-01
        .long   0x3E81E0F8
|         C02  =  -1.2337 00550E+00
        .long   0xBF9DE9E6
|         C00  =   1.0000 00000E+00
        .long   0x3F800000
|
|       .set    NFCOSC,5
|
|
|
|         C09  =   1.6044 11848E-04
FSINCN:
        .long   0x39283C1A
|         C07 =   -4.6817 54135E-03
        .long   0xBB996966
|         C05  =   7.9692 62625E-02
        .long   0x3DA335E3
|         C03  =  -6.4596 40975E-01
        .long   0xBF255DE7
|         C01  =   1.5707 96327E+00
FPIO2:
        .long   0x3FC90FDB
|
|       .set    NFSINC,5
|
|
|
|         Q3  =    1.7732 32244E+01
FTNQCN:
        .long   0x418DDBCC
|         Q2  =   -8.0485 82645E+02
        .long   0xC44936EE
|         Q1  =    6.4501 55566E+03
        .long   0x45C9913F
|         Q0  =   -5.6630 29631E+03
        .long   0xC5B0F83D
|
|       .set    NFTNQC,4
|
|
|         P3  =    1.0000 00000E+00
FTNPCN:
        .long   0x3F800000
|         P2  =   -1.5195 78276E+02
        .long   0xC317F534
|         P1  =    2.8156 53022E+03
        .long   0x452FFA73
|         P0  =   -8.8954 66142E+03
        .long   0xC60AFDDD
|
|       .set    NFTNPC,4
|
|
|         INV2PI  =  1 / 2*PI  =   1.5915 49431E-01
FIN2PI:
        .long   0x3E22F983
|
|
| ###   PUBLIC  FPCOS
|
FPCOS:
        bsr     FTRGSV          | Prepare argument for operation
|
        moveb   d0,d1           | Copy into d1
        andib   #3,d0           | Strip sign bit
        addqb   #1,d0
        rorb    #2,d0           | Move sign bit to B7
        moveb   d0,a7@(4)       | Save it
        rorb    #1,d1
        jcs     FSINOP          | Compute sine
|
|
FCOSOP:
        pea     FCOSCN
        movel   sp@+,a6
        moveq   #NFCOSC,d0
        bsr     FX2SER
|
FTRGFN:
        moveb   a7@(4),d0
        andib   #0x80,d0                | Isolate sign bit
        eorb    d0,sp@
|
        movel   sp@+,sp@
        jmp     a4@
|
|
|
| ###   PUBLIC  FPSIN
|
FPSIN:
        bsr     FTRGSV          | Prepare argument
|
        addib   #0x7E,a7@(4)    | Compute sign
        rorb    #1,d0
        jcs     FCOSOP
|
|
FSINOP:
        movel   sp@,sp@-        | Duplicate argument
|
        pea     FSINCN
        movel   sp@+,a6
        moveq   #NFSINC,d0
        bsr     FX2SER
        jsr     FPMUL
|
        jra     FTRGFN
|
|
|
| ###   PUBLIC  FPTAN
|
FPTAN:
        bsr     FTRGSV          | Prepare argument
|
        rorb    #1,d0
        andib   #0x80,d0
        eorb    d0,a7@(4)
|
        movel   sp@,sp@-        | Double duplication of argument
        movel   sp@,sp@-
|
        pea     FTNPCN
        movel   sp@+,a6
        moveq   #NFTNPC,d0
        bsr     FX2SER
        jsr     FPMUL
|
        movel   a7@(4),d0
        movel   sp@,a7@(4)
        movel   d0,sp@
|
        pea     FTNQCN
        movel   sp@+,a6
        moveq   #NFTNQC,d0
        bsr     FX2SER
|
        btst    #0,a7@(8)
        jne     FPTN20          | J/ reverse division
|
        jsr     FPDIV
        jra     FTRGFN
|
FPTN20:
        jsr     FPRDIV
        jra     FTRGFN
/*
|
|       page
|
|  Trigonometric Service Routine
|
|  The single precision floating point value to be processed is on the
|  stack.  Excess 2 PI's are removed from the argument.  The range of
|  the argument is then scaled to within PI/4 of 0.  The shift size
|  (in number of quadrants) is returned in D0.  The original sign bit
|  is returned in the D0.B sign bit.
|
|  If the argument is NaN, +INF, -INF, or too large (>= 2^16), this
|  routine causes a return to the caller with a NaN result.
|
*/

FTRGSV:
        moveal  sp@+,a6 | FTRGSV return addr
|
        bsr     FPFADJ
        jcs     FFNANR          | Return NAN if +/- INF
        jvs     FFNANR          | Return NaN if NaN
|
        clrw    a7@(4)          | Create arg type return byte
|
        roxlw   sp@             | Sign bit into X
        roxrw   a7@(4)          | Sign bit into temp byte
        roxrw   sp@             | Restore FP value (with sign = +)
|
        cmpiw   #128*FBIAS+2048,sp@     | (2048 = 128*16)
        jge     FFNANR                  | J/ return NaN if ABS(arg) >= 2^16
|
        movel   FIN2PI,sp@-     | Scale arg by 1/(2*PI)
        jsr     FPMUL
|
        cmpiw   #128*FBIAS,sp@
        jlt     FTS10           | /* J/ no excess two PI's */
|
        movel   sp@,sp@-        | Double the scaled argument
        jsr     INT
        negl    d0
        jsr     FLOAT
        jsr     FPADD
|
FTS10:
        tstw    sp@
        jeq     FTS20
|
        addiw   #128*3,sp@      | Multiply value by 8
        cmpiw   #128*FBIAS,sp@
        jlt     FTS20           | J/ < 1
|
        movel   sp@,sp@-        | Double parameter
        jsr     INT
        addqb   #1,d0
        lsrb    #1,d0
        addb    d0,a7@(4)       | Mix quadrant number with sign bit
|
        lslb    #1,d0
        negl    d0
        jsr     FLOAT
        jsr     FPADD
|
FTS20:
        tstw    sp@
        jeq     FTS30           | J/ arg = 0.0
|
        subiw   #128*1,sp@      | Range result to -0.5 to 0.5
|
FTS30:
        andib   #0x83,a7@(4)    | Limit quadrant shift to 0 thru 3
        moveb   a7@(4),d0       | Return cntl byte in d0 and at a7@(4)
        jmp     a6@
/*
|
| ###   SUBTTL  Polynomial Evaluation Routine
|       page
|
|  Polynomial Evaluation Routines.
|
|  A list of constants is pointed to by A6.  X is on the stack.  D0
|  contains the number of constants (the polynomial degree plus one).
|
|  A6 enters pointing to C[1].  Upon return, the value on stack has
|  been replaced with:
|  FXSER   = C[1]*X^(N-1)  + C[2]*X^(N-2)  + ... + C[N-1]*X   + C[N]
|
|  FX2SER  = C[1]*X^(2N-2) + C[2]*X^(2N-4) + ... + C[N-1]*X^2 + C[N]
|
|
*/

FX2SER:
        moveal  a4,a5           | Save a4 in a5
        bsr     FPFADJ
|
        moveb   d0,a7@(4)       | Save count
|
        movel   sp@,sp@-        | Square the argument
        jsr     FPMUL
        jra     FXSR01          | J/ join FXSER routine
|
FXSER:
        moveal  a4,a5
        bsr     FPFADJ
|
        moveb   d0,a7@(4)
|
FXSR01:
        movel   a6@+,sp@-       | Place C[1] on stack as accum
|
        subqb   #1,a7@(8)
|
FXSR02:
        movel   a7@(4),sp@-     | Copy X
        jsr     FPMUL
        movel   a6@+,sp@-       | Add in next coefficient
        jsr     FPADD
|
        subqb   #1,a7@(8)
        jne     FXSR02          | J/ more coefficients
|
        movel   sp@,a7@(8)      | Adjust return
        addql   #8,sp
|
        moveal  a4,a3
        moveal  a5,a4           | Restore return address
        jmp     a3@
/*
|
| ###   SUBTTL  Function Completion Segments
|       page
|
|  FPFADJ - Adjust the stack for single precision functions.
|          Move the return address to the function caller into A4.
|          Shift the single precision floating point argument down
|          four bytes, leaving a temporary area of four bytes at
|          4(A7) thru 7(A7).  Set the condition code bits N, Z,
|          C (INF), and V (NaN) to quickly type the argument.
|
*/

FPFADJ:
        moveal  sp@+,a3 | Function return addr
        moveal  sp@,a4          | Caller return addr
        movel   a7@(4),sp@      | Down shift parameter
|
        movew   sp@,d2          | Get SEM word
        rolw    #1,d2
        cmpiw   #256*0xFF,d2
        jcs     FPFA10          | J/ not INF or NaN
|
        andiw   #0xFE,d2
        jeq     FPFA05
|
        orib    #0x02,ccr       | Set -V- bit -> NaN argument
| ##    DC.W    $003C,$0002
        jmp     a3@
|
FPFA05:
        tstb    sp@             | /* Set N bit as req'd, clear Z bit. */
        orib    #0x01,ccr       | Set -C- bit -> INF argument
| ##    DC.W    $003C,$0001
        jmp     a3@
|
FPFA10:
        tstw    sp@             | /* Set N, Z bits as req'd, clear V, C */
        jmp     a3@
|
|
|
FFPINR:
        |dsw    0               | Result is positive overflow
        moveq   #0,d0           | Sign is positive
        jra     FFIN01
|
FFMINR:
        |dsw    0               | Result in negative overflow
        moveq   #-1,d0          | Sign is negative
|
FFIN01:
        moveaw  d0,a2           | Sign into a2
        addql   #8,sp           | Delete eight bytes from the stack
        moveal  a4,a0           | Return addr in a0
        jmp     FINFRS          | (Use FPOPNS exception return)
|
|
FFNANR:
        |dsw    0               | Result is NaN
        addql   #8,sp           | Delete eight bytes from the stack
        moveal  a4,a0
        jmp     FNANRS
|
|
FFZERR:
        |dsw    0               | Result is zero
        addql   #8,sp
        moveal  a4,a0
        jmp     FZERRS
|
|
FFUNFR:
        |dsw    0               | Underflow result
        addql   #8,sp           | Delete eight bytes from the stack
        moveal  a4,a0           | Return address into a0
        jmp     FUNFRS
|
|
FFONER:
        |dsw    0               | Result is 1.0
        addql   #4,sp
        movel   #0x3F800000,sp@
        clrb    FPERR
        jmp     a4@
|
|
|       end
