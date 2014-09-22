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

|       ttl     FPAC 68K/DPFNCS: IEEE Double Precision Functions
|DPFNCS idnt    1,0             ; IEEE Double Precision Functions
|                               ; DPFNCS.A68
|
|* * * * * * * * * *
|
|       68000 DPAC  --  Double Precision Functions
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
|  Double Precision Floating Point Function library.
|
|  These functions operate on double precision floating point values,
|  yielding double precision floating point results.  All functions
|  process the value on top of the stack and return a result on the
|  stack.  The XTOI function expects its integer argument in D0.
|
|
|  NAME       ARGUMENT                DESCRIPTION
|  ========== =====================   ====================================
|  EXP        -708.40 < a < 709.78    e to the power specified.
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
|       opt     BRS             ; Default forward branches to SHORT
|
        .globl  DPEXP
        .globl  DPLN
        .globl  DPLOG
        .globl  DPXTOI
        .globl  DPSQRT
        .globl  DPATN
        .globl  DPCOS
        .globl  DPSIN
        .globl  DPTAN
|
|
|       xref    DPMUL           ;Floating point operations
|       xref    DPDIV
|       xref    DPRDIV
|       xref    DPADD
|       xref    DAINT           ;Basic floating point functions
|       xref    DFLOAT
|       xref    DINT
|
|       xref    DNANRS
|       xref    DINFRS
|       xref    DUNFRS
|       xref    DZERRS
|
|
|
|
|       .set    DBIAS,1023
|
|
|       xref    FPERR
|
|
|       .set    ERUNF,1         |FPERR UNDERFLOW ERROR CODE
|       .set    EROVF,2         |FPERR OVERFLOW ERROR CODE
|       .set    ERNAN,3         |FPERR INVALID OPERATION ERROR CODE
/*
|
|
|
| ###   SUBTTL  DPEXP: Exponentiation of e Function
|       page
|
|
|  EXP:   Calculate e to the power specified by the stack argument.
|         The result is returned on the stack.
|         placed at DI.
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
DEXPCN:
        |dsw    0               | EXP CONSTANT LIST (2**X EVALUATION)
|
| ???     (LN 2)^16/16!   =   1.3570 24794 87551 47193 D-16
| ???   DC.L    $3CA38E89,$AE79F8B4
|         (LN 2)^15/15!   =   3.1324 36707 08842 86216 D-15
        .long   0x3CEC36E8,0x43B04022
|         (LN 2)^14/14!   =   6.7787 26354 82254 56335 D-14
        .long   0x3D331496,0x4D5878A9
|         (LN 2)^13/13!   =   1.3691 48885 39041 28881 D-12
        .long   0x3D781619,0x3166D0F9
|         (LN 2)^12/12!   =   2.5678 43599 34882 05142 D-11
        .long   0x3DBC3BD6,0x50FC2986
|         (LN 2)^11/11!   =   4.4455 38271 87081 14976 D-10
        .long   0x3DFE8CAC,0x7351BB25
|         (LN 2)^10/10!   =   7.0549 11620 80112 33299 D-09
        .long   0x3E3E4CF5,0x158B8ECA
|         (LN 2)^ 9/ 9!   =   1.0178 08600 92396 99728 D-07
        .long   0x3E7B5253,0xD395E7C4
|         (LN 2)^ 8/ 8!   =   1.3215 48679 01443 09488 D-06
        .long   0x3EB62C02,0x23A5C824
|         (LN 2)^ 7/ 7!   =   1.5252 73380 40598 40280 D-05
        .long   0x3EEFFCBF,0xC588B0C7
|         (LN 2)^ 6/ 6!   =   1.5403 53039 33816 09954 D-04
        .long   0x3F243091,0x2F86C787
|         (LN 2)^ 5/ 5!   =   1.3333 55814 64284 43423 D-03
        .long   0x3F55D87F,0xE78A6731
|         (LN 2)^ 4/ 4!   =   9.6181 29107 62847 71620 D-03
        .long   0x3F83B2AB,0x6FBA4E77
|         (LN 2)^ 3/ 3!   =   5.5504 10866 48215 79953 D-02
        .long   0x3FAC6B08,0xD704A0C0
|         (LN 2)^ 2/ 2!   =   2.4022 65069 59100 71233 D-01
        .long   0x3FCEBFBD,0xFF82C58F
|         (LN 2)^1/ 1!   =   6.9314 71805 59945 30942 D-01
DLN2:
        .long   0x3FE62E42,0xFEFA39EF
|                             1.0000 00000 00000 00000 D+00
        .long   0x3FF00000,0x00000000
|
|       .set    DNEXPC,16               | Number of constants in DEXP poly
|
|         1 / LN 2  =  1.4426 95040 88896 34074 D+00
DINLN2:
        .long   0x3FF71547,0x652B82FE
|
|
|
| ###   PUBLIC  DPEXP
|
DPEXP:
        |dsw    0
        bsr     DPFADJ          | Set-up for DP functions
|
        jvs     DFNANR          | NaN parm -> NaN result
        jcc     DEXP01          | J/ arg not INF
|
        jmi     DFUNFR          | -INF parm -> underflow result
        jra     DFPINR          | +INF parm -> positive INF result
|
DEXP01:
        movel   DINLN2+4,sp@-   | 1/LN 2 on stack
        movel   DINLN2+0,sp@-
        jsr     DPMUL           | Convert to 2^ function
|
        movew   sp@,d1
        andiw   #0x7FF0,d1
        cmpiw   #16*DBIAS+144,d1
        jhi     DEXP10          | J/ overflow or underflow  >= +/- 2^10
        jne     DEXP20          | J/ within range            < +/- 2^09
        tstb    sp@
        jpl     DEXP20          | J/ result >= 1.0
|
        movel   sp@,d2          | Examine top 12 bits of mantissa
        rorl    #8,d2
        andiw   #0x0FFF,d2
        cmpiw   #0x0FF8,d2
        jcs     DEXP20          | J/ result >= 2^-1022
|
DEXP10:
        tstb    sp@
        jpl     DFPINR          | J/ DP function +INF result
        jra     DFUNFR          | J/ DP function underflow result
|
DEXP20:
        movel   a7@(4),sp@-
        movel   a7@(4),sp@-     | Double the argument on the stack
|
        jsr     DINT
|
        movew   d1,a7@(8)       | Save result of DINT
|
        negl    d1
        negxl   d0              | Negate integer
        jsr     DFLOAT
        jsr     DPADD           | Special DFRAC
|
        pea     DEXPCN
        movel   sp@+,a6 | Point a6 to constant list
        moveq   #DNEXPC ,d0      | d0 holds the number of constants
        moveq   #DNEXPC, d0      | d0 holds the number of constants
        moveq   #16,d0      | XXX DEBUG DEBUG
        bsr     DXSER           | Polynomial series
|
        movew   a7@(8),d0       | Fetch power of two scaling
        lslw    #4,d0
        swap    d0
        clrw    d0
        addl    sp@+,d0 | Combine with DXSER result
|
        movel   sp@,a7@(4)      | Downshift result
        movel   d0,sp@
        jmp     a4@             | Return

/*
|
| ###   SUBTTL  DPLOG, DPLN: Logarithm Functions
|       page
|
|  LOGARITHM (BASE E) FUNCTION.
|
|  The natural logarithm of the double precision floating point
|  value on the stack is computed using a split domain polynomial
|  approximation.  The common log is computed by scaling the
|  result of the natural log computation.
|
|  TWOS = FLOAT(ARG.EXPONENT) * LOG(2)
|
|  The argument, (after the two's power scaling, 2.0 > arg' >= 1.0),
|  is sorted into one of five classes.  Within that class, a center
|  point with a known log value is used in combination with a 9th
|  degree Chebyshey polynomial approximation, to calculate a
|  logarithm.
|
|  The center point of each class is adjusted so that its natural
|  logarithm expressed in double precision form is accurate to
|  more than 80 bits.
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

DLNCNS:
        |dsw    0               | LN constants list
|
|         c1  =   0.22330 022
        .long   0x3FCC951A,0x03000000
|         c2  =   0.28571 20487
        .long   0x3FD2491B,0x30450000
|         c3  =   0.40000 00018 947
        .long   0x3FD99999,0x9BA26900
|         c4  =   0.66666 66666 66105
        .long   0x3FE55555,0x55554192
|         c5  =   2.00000 00000 00000 00000
        .long   0x40000000,0x00000000
|
|       .set    DNLNCN,5                | Number of constants in DLN poly
|
|
DLNCEN:
        |dsw    0               | Center Points
|         Center  #1 = 1.0000 00000 00000 00000
        .long   0x3FF00000,0x00000000
|         Center  #2 = 1.1892 07115 00272 05647  (close to 2^(1/4))
        .long   0x3FF306FE,0x0A31B713
|         Center  #3 = 1.4142 13562 37309 38548  (close to 2^(1/2))
        .long   0x3FF6A09E,0x667F3BC7
|         Center  #4 = 1.6817 92830 50742 54625  (close to 2^(3/4))
        .long   0x3FFAE89F,0x995AD39D
|         Center  #5 = 2.0000 00000 00000 00000
| ---     shifted to 1.0 center..
|
DLNLOG:
        |dsw    0               | DLNCEN values adj for exact ln values
|         LN(Center #2) = 0.17328 67951 39985 90522
        .long   0x3FC62E42,0xFEFA39E0
|         LN(Center #3) = 0.34657 35902 79971 81045
        .long   0x3FD62E42,0xFEFA39E0
|         LN(Center #4) = 0.51986 03854 19956 82749
        .long   0x3FE0A2B2,0x3F3BAB60
|
|
|         1 / LN 10  =  0.43429 44819 03251 82765
DILN10:
        .long   0x3FDBCB7B,0x1526E50E
|
|
|
| ###   PUBLIC  DPLN
| ###   PUBLIC  DPLOG
|
DPLOG:
        |dsw    0
        moveq   #-1,d0          | Signal common log scaling
        jra     DLN000
|
DPLN:
        |dsw    0
        clrb    d0              | Signal no post calculation scaling
|
DLN000:
        bsr     DPFADJ          | Adjust parameter on stack
|
        jvs     DFNANR          | J/ NaN arg -> NaN result
        jmi     DFNANR          | J/ Neg arg -> NaN result
        jeq     DFMINR          | J/ 0.0 arg -> -INF result
        jcs     DFPINR          | J/ +INF arg -> +INF result
|
        moveb   d0,a7@(10)      | Save natural/common flag
|
        movew   sp@,d1          | Prepare to calc parm sign and exp
        lsrw    #4,d1           | Position exponent (right justified)
        subiw   #DBIAS,d1
        movew   d1,a7@(8)       | /* Save two's exponent value */
|
        lslw    #4,d1           | Scale parameter
        subw    d1,sp@
|
        clrw    d0              | Set class number to 0
        movel   sp@,d1
        lsrl    #8,d1           | d1.W = 0xEMMM
|
        cmpiw   #0xF000+371,d1
        jcs     DLN050          | J/ < 1 +  371/4096  (1.090576)
        addqw   #8,d0
        cmpiw   #0xF000+1216,d1
        jcs     DLN050          | J/ < 1 + 1216/4096  (1.296875)
        addqw   #8,d0
        cmpiw   #0xF000+2221,d1
        jcs     DLN050          | J/ < 1 + 2221/4096  (1.542236)
        addqw   #8,d0
        cmpiw   #0xF000+3416,d1
        jcs     DLN050          | J/ < 1 + 3416/4096  (1.833984)
        addqw   #1,a7@(8)       | /* Class 4: Bump two's exp, then class 0 */
        clrw    d0
        subib   #0x10,a7@(1)    | Reduce scaled exponent
|
DLN050:
        moveb   d0,a7@(11)      | Save center number
|
        pea     DLNCEN
        movel   sp@+,a2
        movel   a2@(4,d0:W),sp@- |Place center on stack
        movel   a2@(0,d0:W),sp@-
|
        movel   a7@(12),sp@-    | Copy scaled argument
        movel   a7@(12),sp@-
|
        movel   a2@(4,d0:W),sp@- |Place center on stack
        movel   a2@(0,d0:W),sp@-
|
        jsr     DPADD           | /* Calc  (CENTER + PARM') */
|
        movel   sp@,d0          | Exch top stack item w/ 3rd
        movel   a7@(16),d1
        movel   d1,sp@
        movel   d0,a7@(16)
        movel   a7@(4),d0
        movel   a7@(20),d1
        movel   d1,a7@(4)
        movel   d0,a7@(20)
|
        bset    #7,a7@(8)       | Negate CENTER
|
        jsr     DPADD           | /* Calc  - (CENTER - PARM') */
        jsr     DPRDIV          | /* T = - (CENTER-PARM')/(CENTER+PARM) */
|
        movel   a7@(4),sp@-     | Duplicate t
        movel   a7@(4),sp@-
|
        pea     DLNCNS
        movel   sp@+,a6 | Poly approx
        moveq   #DNLNCN,d0
        bsr     DX2SER
|
        jsr     DPMUL           | Times t
        clrw    d0
        moveb   a7@(11),d0
        jeq     DLN060          | J/ center = 0 -> no additive log val
|
        pea     DLNLOG-8
        movel   sp@+,a6
        movel   a6@(4,d0:W),sp@-
        movel   a6@(0,d0:W),sp@-
        jsr     DPADD           | Add in center log value
|
DLN060:
        movew   a7@(8),d1       | /* Get two's exponent value */
        clrl    d0
        extl    d1
        jpl     DLN061
        moveq   #-1,d0
DLN061:
        |dsw    0
|
        jsr     DFLOAT
|
        movel   DLN2+4,sp@-     | Log of 2 on stack
        movel   DLN2+0,sp@-
        jsr     DPMUL
        jsr     DPADD
|
        tstb    a7@(10)
        jeq     DLN070          | J/ natural log
|
        movel   DILN10+4,sp@-   | Scaling value
        movel   DILN10+0,sp@-
        jsr     DPMUL
|
DLN070:
        movel   a7@(4),a7@(8)
        movel   sp@+,sp@
        jmp     a4@

/*
| ###   SUBTTL  DPXTOI: Floating Point Number to Integer Power Function
|       page
|
|  X to I power Function.
|
|  The double precision floating point value on the stack is
|  raised to the power specified in D0.W then returned on stack.
|
|  A shift and multiply technique is used (possibly with a trailing
|  recipication.
|
|
| ###   PUBLIC  DPXTOI
|
*/

DPXTOI:
        |dsw    0
        bsr     DPFADJ
|
        jvs     DFNANR          | J/ NaN arg -> NaN result
        jcc     DPXT10          | J/ arg is not INF
|
        tstw    d0
        jeq     DFNANR          | J/ arg is +/- INF, I is 0 -> NaN
        jmi     DFUNFR          | J/ x is +/- INF, I < 0 -> underflow
|
        rorw    #1,d0
        andw    sp@,d0
        jmi     DFMINR          | J/ arg is -INF, I is +odd -> -INF
        jra     DFPINR          | else -> +INF
|
|
DPXT10:
        jne     DPXT15          | J/ parm is a number <> 0.0
|
        tstw    d0
        jmi     DFPINR          | J/ 0.0 to -int -> +INF
        jeq     DFNANR          | J/ 0.0 to 0 -> NaN
        jra     DFZERR          | J/ 0.0 to +int -> 0.0
|
DPXT15:
        tstw    d0
        jeq     DFONER          | J/ num to 0 -> 1.0
|
        movew   d0,a7@(10)      | Save int as its own sign flag
        jpl     DPXT20
        negw    d0
DPXT20:
        |dsw    0
|
        movel   a7@(4),sp@-     | Result init to parm value
        movel   a7@(4),sp@-
|
        moveq   #16,d1          | Find MS bit of power
DPXT21:
        lslw    #1,d0
        jcs     DPXT22          | J/ MS bit moved into carry
        dbra    d1,DPXT21       | Dec d1 and jump (will always jump)
|
DPXT22:
        movew   d0,a7@(16)      | Power pattern on stack
        moveb   d1,a7@(19)      | Bit slots left count on stack
|
|
DPXT30:
        subqb   #1,a7@(19)      | Decrement bit slots left
        jeq     DPXT35          | J/ evaluation complete
|
        movel   a7@(4),sp@-     | Square result value
        movel   a7@(4),sp@-
        jsr     DPMUL           | Square it
|
        lslw    a7@(16)         | Shift power pattern
        jcc     DPXT30          | J/ this product bit not set
|
        movel   a7@(12),sp@-    | Copy parm value
        movel   a7@(12),sp@-
        jsr     DPMUL
        jra     DPXT30
|
DPXT35:
        tstb    a7@(18)         | Check for recipocation
        jpl     DPXT36          | J/ no recipocation
|
        clrl    sp@-            | Place 1.0 on stack
        movel   #0x3FF00000,sp@-
        jsr     DPRDIV
|
DPXT36:
        movel   sp@+,a7@(8)     | Shift result value
        movel   sp@+,a7@(8)
        addql   #4,sp           | Delete excess stack area
        jmp     a4@             | Return.
/*
|
| ###   SUBTTL  DPSQRT: Square Root Function
|       page
|
|  Square Root Function.
|
|  Take square root of the double precision floating point value
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
| ###   PUBLIC  DPSQRT
|
*/

DPSQRT:
        |dsw    0
        bsr     DPFADJ
|
        jvs     DFNANR          | J/ NaN arg -> NaN result
        jmi     DFNANR          | J/ neg arg -> NaN result
        jcs     DFPINR          | J/ +INF arg -> +INF result
        jeq     DFZERR          | J/ 0.0 arg -> 0.0 result
|
        movew   sp@,d1          | Get S/E/M word
        subiw   #16*DBIAS,d1    | Extract argument's two's exp
        andib   #0xE0,d1                | Make it a factor of two
        subw    d1,sp@          | /* Scale arg. range to 4.0 > arg' >= 1.0 */
        asrw    #1,d1           | Square root of scaled two power
        movew   d1,a7@(8)       | /* Save two's exp of result on stack */
|
        movel   sp@,d1          | Create fixed point integer for approx
        movew   a7@(4),d2
        lsll    #8,d1           | /* Produce arg' * 2^30 in d1 */
        lsll    #3,d1
        lsrl    #5,d2
        orw     d2,d1
        bset    #31,d1          | Set implicit bit
        jeq     DPSQ10          | /* J/ arg' >= 2.0 */
|
        lsrl    #1,d1           | Adjust d1
|
DPSQ10:
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
        addl    d4,d2           | X3 in d2 - to 29 bits
        subxl   d4,d4
        orl     d4,d2           | Top out at 1.9999999995
|
        movel   a7@(4),sp@-     | /* Down shift arg' */
        movel   a7@(4),sp@-
|
        lsll    #1,d2           | Create DP of X3 (good to 29 bits) ...
        clrl    d3              | ... in d2:d3
        movew   d2,d3
        andiw   #0xFFF,d3
        eorw    d3,d2
        oriw    #DBIAS,d2       | Scale to floating point
        moveq   #12,d0          | Shift count in d0
        rorl    d0,d2           | Position bits
        rorl    d0,d3
|
        movel   d2,a7@(8)       | /* On stack: X4, arg', X4, <flags> */
        movel   d3,a7@(12)
        movel   d3,sp@-
        movel   d2,sp@-
        jsr     DPDIV           | Last iter - to X4 - in DP domain
        jsr     DPADD
        subiw   #0x0010,sp@     | "Divide" by 2
|
        movew   a7@(8),d7
        addw    d7,sp@          | Scale result
        movel   a7@(4),a7@(8)   | Down shift result
        movel   sp@+,sp@
        jmp     a4@

/*
| ###   SUBTTL  DPATN: Arctangent Function
|       page
|
|  ARCTANGENT Function.
|
|  The arctangent of the double precision floating point value at SI is
|  computed by using a split domain with a polynomial approximation.
|  A principal range radian value is returned.
|
|  The domain is split at 0.125 (eight) intervals.  A polynomial is
|  used to approximate the arctangent for magnitudes less than 1/16.
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
|         C4  =    1.1022 81616 12614 90000E-01
*/

DATNCN:
        .long   0x3FBC37E9,0xAD397134
|         C3  =   -1.4285 41305 08745 00000E-01
        .long   0xBFC2490B,0x4D511901
|         C2  =    1.9999 99958 01446 40000E-01
        .long   0x3FC99999,0x90956BB6
|         C1  =   -3.3333 33333 31284 50000E-01
        .long   0xBFD55555,0x5554C529
|         C0  =    1.0000 00000 00000 00000E+00
        .long   0x3FF00000,0x00000000
|
|       .set    NDATNC,5
|
|
|
|  Table of ARCTANGENT values at 0.125 intervals
|
|         ATAN(1/8)  =  1.2435 49945 46761 43503E-01
DATNTB:
        .long   0x3FBFD5BA,0x9AAC2F6E
|         ATAN(2/8)  =  2.4497 86631 26864 15417E-01
        .long   0x3FCF5B75,0xF92C80DE
|         ATAN(3/8)  =  3.5877 06702 70572 22040E-01
        .long   0x3FD6F619,0x41E4DEF1
|         ATAN(4/8)  =  4.6364 76090 00806 11621E-01
        .long   0x3FDDAC67,0x0561BB4F
|         ATAN(5/8)  =  5.5859 93153 43562 43597E-01
        .long   0x3FE1E00B,0xABDEFEB4
|         ATAN(6/8)  =  6.4350 11087 93284 38680E-01
        .long   0x3FE4978F,0xA3269EE1
|         ATAN(7/8)  =  7.1882 99996 21624 50542E-01
        .long   0x3FE700A7,0xC5784634
|         ATAN(8/8)  =  7.8539 81633 97448 30962E-01   ( = PI/4)
        .long   0x3FE921FB,0x54442D18
|
|
|  DP PI/2 (duplicate of DPIO2 because of assembler bug)
|
DXPIO2:
        .long   0x3FF921FB,0x54442D18
|
| ###   PUBLIC  DPATN
|
DPATN:
        |dsw    0
        bsr     DPFADJ
|
        jvs     DFNANR          | J/ NaN arg -> NaN result
        jcc     DPAT10          | J/ not INF
|
        movel   DXPIO2+0,d1     | Get top long word of PI/2
        roxll   #1,d1
        roxlw   sp@             | INF sign bit into X
        roxrl   #1,d1           | PI/2 given sign of INF
        addql   #4,sp           | Delete four bytes from the stack
        movel   DXPIO2+4,a7@(4)
        movel   d1,sp@
        jmp     a4@
|
DPAT10:
        jeq     DFZERR          | J/ 0.0 arg -> 0.0 result
|
        bclr    #7,sp@          | Insure argument positive
        sne     d0              | Create flag byte (0xFF iff negative)
        andib   #0x80,d0                | Keep sign bit only
        moveb   d0,a7@(10)      | Save flag byte
|
        movew   sp@,d1
        cmpiw   #16*DBIAS,d1
        jle     DPAT20          | J/ arg < 1 + 1/16
|
        addqb   #1,a7@(10)
|
        clrl    sp@-            | Place 1.0 onstack
        movel   #0x3FF00000,sp@-
        jsr     DPRDIV          | Invert the number
|
DPAT20:
        movew   sp@,d1
        cmpiw   #16*DBIAS-64,d1
        jlt     DPAT30          | J/ arg < 1/16
|
        moveb   d1,d2           | Number of sixteenths in d2
        andib   #0x0F,d2
        orib    #0x10,d2                | Implicit bit
        lsrw    #4,d1
        moveq   #-1,d3
        subb    d1,d3
        lsrb    d3,d2
        addqb   #1,d2
        lsrb    #1,d2           | Rounded eighths in d2.B
|
        clrl    d0
        clrl    d1
        moveb   d2,d1           | 64 bit integer in d0:d1
|
        lslb    #3,d2
        addb    d2,a7@(10)      | Save 8 * eigths on stack
|
        jsr     DFLOAT
        subiw   #16*3,sp@       | Produce y, floating point eighths
|
        moveq   #4-1,d0
DPAT25:
        movel   a7@(12),sp@-
        dbra    d0,DPAT25       | On stack:  y, arg', y, arg', <temps>
|
        jsr     DPMUL           | /* arg'*y */
        clrl    sp@-            | Place 1.0 on stack
        movel   #0x3FF00000,sp@-
        jsr     DPADD           | /* 1.0 + arg'*y */
|
        movel   a7@(16),d0      | Exchange stack item
        movel   a7@(20),d1
        movel   sp@,a7@(16)
        movel   a7@(4),a7@(20)
        movel   d0,sp@
        movel   d1,a7@(4)       | On stack: arg', y, (1+arg'*y), <tmps>
        bset    #7,a7@(8)       | Negate y
        jsr     DPADD           | /* (arg'-y) */
        jsr     DPRDIV          | /* (arg'-y) / (1 + arg'*y) */
|
DPAT30:
        |dsw    0
        movel   a7@(4),sp@-     | Duplicate z
        movel   a7@(4),sp@-
|
        pea     DATNCN
        movel   sp@+,a6 | Polynomial approximation to small ATN
        moveq   #NDATNC,d0
        bsr     DX2SER
        jsr     DPMUL           | Complete approximation
|
        moveb   a7@(10),d0
        andiw   #0x0078,d0      | Trim to table index
        jeq     DPAT40          | J/ y = 0.0, ARCTAN(y) = 0.0
|
        pea     DATNTB-8
        movel   sp@+,a6
        movel   a6@(4,d0:W),sp@-
        movel   a6@(0,d0:W),sp@-
        jsr     DPADD           | Add in ARCTAN(y)
|
DPAT40:
        btst    #0,a7@(10)      | Check for inversion
        jeq     DPAT50          | J/ no inversion
|
        bset    #7,sp@          | Negate ARCTAN
        movel   DXPIO2+4,sp@-
        movel   DXPIO2+0,sp@-
        jsr     DPADD           | Inversion via subtraction
|
DPAT50:
        tstb    a7@(10)         | Check sign of result
        jpl     DPAT60          | J/ positive
|
        bset    #7,sp@          | Negate result
|
DPAT60:
        movel   a7@(4),a7@(8)   | Downshift result
        movel   sp@+,sp@
        jmp     a4@

/*
|
| ###   SUBTTL  DPCOS, DPSIN, DPTAN: Trigonometric Functions
|       page
|
|  TRIG ROUTINES.
|
|  The support routine DTRGSV converts the radian mode argument to
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
|         C16  =   6.5659 63114 97947 23622E-11
*/

DCOSCN:
        .long   0x3DD20C62,0xC2F2D7F5
|         C14  =  -6.3866 03083 79185 22411E-09
        .long   0xBE3B6E24,0xF44B128F
|         C12  =   4.7108 74778 81817 15037E-07
        .long   0x3E9F9D38,0xA3763CC3
|         C10  =  -2.5202 04237 30606 05481E-05
        .long   0xBEFA6D1F,0x2A204A8C
|         C08  =   9.1926 02748 39426 58024E-04
        .long   0x3F4E1F50,0x6891BABB
|         C06  =  -2.0863 48076 33529 96087E-02
        .long   0xBF955D3C,0x7E3CBFFA
|         C04  =   2.5366 95079 01048 01364E-01
        .long   0x3FD03C1F,0x081B5AC4
|         C02  =  -1.2337 00550 13616 98274E+00
        .long   0xBFF3BD3C,0xC9BE45DE
|         C00  =   1.0000 00000 00000 00000E+00
        .long   0x3FF00000,0x00000000
|
|       .set    NDCOSC,9
|
|
|
|         C15  =  -6.6880 35109 81146 72325E-10
DSINCN:
        .long   0xBE06FADB,0x9F155744
|         C13  =   5.6921 72921 96792 68118E-08
        .long   0x3E6E8F43,0x4D018D63
|         C11  =  -3.5988 43235 21208 53405E-06
        .long   0xBECE3074,0xFDE8871F
|         C09  =   1.6044 11847 87359 82187E-04
        .long   0x3F250783,0x487EE782
|         C07 =   -4.6817 54135 31868 81007E-03
        .long   0xBF732D2C,0xCE62BD86
|         C05  =   7.9692 62624 61670 45121E-02
        .long   0x3FB466BC,0x6775AAE2
|         C03  =  -6.4596 40975 06246 25366E-01
        .long   0xBFE4ABBC,0xE625BE53
|         C01  =   1.5707 96326 79489 66192E+00
DPIO2:
        .long   0x3FF921FB,0x54442D18
|
|       .set    NDSINC,8
|
|
|
|         Q3  =    1.7732 32244 08118 84863E+01
DTNQCN:
        .long   0x4031BB79,0x7BC569F8
|         Q2  =   -8.0485 82645 07638 77427E+02
        .long   0xC08926DD,0xB9C83D02
|         Q1  =    6.4501 55566 23337 83845E+03
        .long   0x40B93227,0xD3304CB9
|         Q0  =   -5.6630 29630 56808 22115E+03
        .long   0xC0B61F07,0x95DE70E0
|
|       .set    NDTNQC,4
|
|
|         P3  =    1.0000 00000 00000 00000E+00
DTNPCN:
        .long   0x3FF00000,0x00000000
|         P2  =   -1.5195 78275 66504 79235E+02
        .long   0xC062FEA6,0x85FF2B0D
|         P1  =    2.8156 53021 77302 44048E+03
        .long   0x40A5FF4E,0x58DEAD6E
|         P0  =   -8.8954 66142 22700 41047E+03
        .long   0xC0C15FBB,0xAA8C6A22
|
|       .set    NDTNPC,4
|
|
|         INV2PI  =  1 / 2*PI  =   1.5915 49430 91895 33577E-01
DIN2PI:
        .long   0x3FC45F30,0x6DC9C883
|
|
| ###   PUBLIC  DPCOS
|
DPCOS:
        bsr     DTRGSV          | Prepare argument for operation
|
        moveb   d0,d1           | Copy into d1
        andib   #3,d0           | Strip sign bit
        addqb   #1,d0
        rorb    #2,d0           | Move sign bit to B7
        moveb   d0,a7@(8)       | Save it
        rorb    #1,d1
        jcs     DSINOP          | Compute sine
|
|
DCOSOP:
        pea     DCOSCN
        movel   sp@+,a6
        moveq   #NDCOSC,d0
        bsr     DX2SER
|
DTRGFN:
        moveb   a7@(8),d0
        andib   #0x80,d0                | Isolate sign bit
        eorb    d0,sp@
|
        movel   a7@(4),a7@(8)
        movel   sp@+,sp@
        jmp     a4@
|
|
|
| ###   PUBLIC  DPSIN
|
DPSIN:
        bsr     DTRGSV          | Prepare argument
|
        addib   #0x7E,a7@(8)    | Compute sign
        rorb    #1,d0
        jcs     DCOSOP
|
|
DSINOP:
        movel   a7@(4),sp@-     | Duplicate argument
        movel   a7@(4),sp@-
|
        pea     DSINCN
        movel   sp@+,a6
        moveq   #NDSINC,d0
        bsr     DX2SER
        jsr     DPMUL
|
        jra     DTRGFN
|
|
|
| ###   PUBLIC  DPTAN
|
DPTAN:
        bsr     DTRGSV          | Prepare argument
|
        rorb    #1,d0
        andib   #0x80,d0
        eorb    d0,a7@(8)
|
        moveq   #4-1,d1         | Double duplication of argument
DPTN01:
        movel   a7@(4),sp@-
        dbra    d1,DPTN01
|
        pea     DTNPCN
        movel   sp@+,a6
        moveq   #NDTNPC,d0
        bsr     DX2SER
        jsr     DPMUL
|
        movel   a7@(8),d0
        movel   a7@(12),d1
        movel   sp@,a7@(8)
        movel   a7@(4),a7@(12)
        movel   d0,sp@
        movel   d1,a7@(4)
|
        pea     DTNQCN
        movel   sp@+,a6
        moveq   #NDTNQC,d0
        bsr     DX2SER
|
        btst    #0,a7@(16)
        jne     DPTN20          | J/ reverse division
|
        jsr     DPDIV
        jra     DTRGFN
|
DPTN20:
        jsr     DPRDIV
        jra     DTRGFN
/*
|
|       page
|
|  Trigonometric Service Routine
|
|  The double precision floating point value to be processed is on the
|  stack.  Excess 2 PI's are removed from the argument.  The range of
|  the argument is then scaled to within PI/4 of 0.  The shift size
|  (in number of quadrants) is returned in D0.  The original sign bit
|  is returned in the D0.B sign bit.
|
|  If the argument is NaN, +INF, -INF, or too large (>= 2^16), this
|  routine causes a return to the caller with a NaN result.
|
*/

DTRGSV:
        moveal  sp@+,a6 | DTRGSV return addr
|
        bsr     DPFADJ
        jcs     DFNANR          | Return NAN if +/- INF
        jvs     DFNANR          | Return NaN if NaN
|
        clrw    a7@(8)          | Create arg type return byte
|
        roxlw   sp@             | Sign bit into X
        roxrw   a7@(8)          | Sign bit into temp byte
        roxrw   sp@             | Restore DP value (with sign = +)
|
        cmpiw   #16*DBIAS+256,sp@
        jge     DFNANR          | J/ return NaN if ABS(arg) >= 2^16
|
        movel   DIN2PI+4,sp@-   | Scale arg by 1/(2*PI)
        movel   DIN2PI+0,sp@-
        jsr     DPMUL
|
        cmpiw   #16*DBIAS,sp@
        jlt     DTS10           | /* J/ no excess two PI's */
|
        movel   a7@(4),sp@-     | Double the scaled argument
        movel   a7@(4),sp@-
        jsr     DINT
        negl    d1
        negxl   d0
        jsr     DFLOAT
        jsr     DPADD
|
DTS10:
        tstw    sp@
        jeq     DTS20
|
        addiw   #16*3,sp@       | Multiply value by 8
        cmpiw   #16*DBIAS,sp@
        jlt     DTS20           | J/ < 1
|
        movel   a7@(4),sp@-     | Double parameter
        movel   a7@(4),sp@-
        jsr     DINT
        addqb   #1,d1
        lsrb    #1,d1
        addb    d1,a7@(8)       | Mix quadrant number with sign bit
|
        lslb    #1,d1
        negl    d1
        negxl   d0
        jsr     DFLOAT
        jsr     DPADD
|
DTS20:
        tstw    sp@
        jeq     DTS30           | J/ arg = 0.0
|
        subiw   #16*1,sp@       | Range result to -0.5 to 0.5
|
DTS30:
        andib   #0x83,a7@(8)    | Limit quadrant shift to 0 thru 3
        moveb   a7@(8),d0       | Return cntl byte in d0 and at a7@(8)
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
|  DXSER   = C[1]*X^(N-1)  + C[2]*X^(N-2)  + ... + C[N-1]*X   + C[N]
|
|  DX2SER  = C[1]*X^(2N-2) + C[2]*X^(2N-4) + ... + C[N-1]*X^2 + C[N]
|
|
*/

DX2SER:
        moveal  a4,a5           | Save a4 in a5
        bsr     DPFADJ
|
        moveb   d0,a7@(8)       | Save count
|
        movel   a7@(4),sp@-     | Square the argument
        movel   a7@(4),sp@-
        jsr     DPMUL
        jra     DXSR01          | J/ join DXSER routine
|
DXSER:
        moveal  a4,a5
        bsr     DPFADJ
|
        moveb   d0,a7@(8)
|
DXSR01:
        movel   a6@(4),sp@-     | Place C[1] on stack as accum
        movel   a6@,sp@-
        addql   #8,a6
|
        subqb   #1,a7@(16)
|
DXSR02:
        movel   a7@(12),sp@-    | Copy X
        movel   a7@(12),sp@-
        jsr     DPMUL
        movel   a6@(4),sp@-     | Add in next coefficient
        movel   a6@,sp@-
        addql   #8,a6           | Advance to next coefficient
        jsr     DPADD
|
        subqb   #1,a7@(16)
        jne     DXSR02          | J/ more coefficients
|
        movel   a7@(4),a7@(16)  | Adjust return
        movel   sp@+,a7@(8)
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
|  DPFADJ - Adjust the stack for double precision functions.
|          Move the return address to the function caller into A4.
|          Shift the double precision floating point argument down
|          four bytes, leaving a temporary area of four bytes at
|          8(A7) thru 11(A7).  Set the condition code bits N, Z,
|          C (INF), and V (NaN) to quickly type the argument.
|
*/

DPFADJ:
        moveal  sp@+,a3 | Function return addr
        moveal  sp@,a4          | Caller return addr
        movel   a7@(4),sp@      | Down shift parameter
        movel   a7@(8),a7@(4)
|
        movew   sp@,d2          | Get SEM word
        rolw    #1,d2
        cmpiw   #32*0x7FF,d2
        jcs     DPFA10          | J/ not INF or NaN
|
        andiw   #0x1E,d2
        jeq     DPFA05
|
        orib    #0x02,ccr       | Set -V- bit -> NaN argument
        jmp     a3@
|
DPFA05:
        tstb    sp@             | /* Set N bit as req'd, clear Z bit. */
        orib    #0x01,ccr       | Set -C- bit -> INF argument
        jmp     a3@
|
DPFA10:
        tstw    sp@             | /* Set N, Z bits as req'd, clear V, C */
        jmp     a3@
|
|
|
DFPINR:
        |dsw    0               | Result is positive overflow
        moveq   #0,d0           | Sign is positive
        jra     DFIN01
|
DFMINR:
        |dsw    0               | Result in negative overflow
        moveq   #-1,d0          | Sign is negative
|
DFIN01:
        moveaw  d0,a2           | Sign into a2
        addql   #8,sp           | Delete twelve bytes from the stack
        addql   #4,sp
        moveal  a4,a0           | Return addr in a0
        jmp     DINFRS          | (Use DPOPNS exception return)
|
|
DFNANR:
        |dsw    0               | Result is NaN
        addql   #8,sp           | Delete twelve bytes from the stack
        addql   #4,sp
        moveal  a4,a0
        jmp     DNANRS
|
|
DFZERR:
        |dsw    0               | Result is zero
        addql   #8,sp
        addql   #4,sp
        moveal  a4,a0
        jmp     DZERRS
|
|
DFUNFR:
        |dsw    0               | Underflow result
        addql   #8,sp           | Delete twelve bytes from the stack
        addql   #4,sp
        moveal  a4,a0           | Return address into a0
        jmp     DUNFRS
|
|
DFONER:
        |dsw    0               | Result is 1.0
        addql   #4,sp
        movel   #0x3FF00000,sp@
        clrl    a7@(4)
        clrb    FPERR
        jmp     a4@
|
|       end
