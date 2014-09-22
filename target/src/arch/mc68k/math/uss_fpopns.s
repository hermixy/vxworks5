/* Copyright 1991-1992 Wind River Systems, Inc. */
	.data
	.globl	_copyright_wind_river
	.long	_copyright_wind_river

/*
modification history
--------------------
01g,14mar95,tmk  inverted conditional assembly logic for 68000/10 to allow for
		 CPUs other than 68020.
01f,23aug92,jcf  changed bxxx to jxx.
01e,26may92,rrr  the tree shuffle
01d,30mar92,kdl  added include of "uss_fp.h"; commented-out ".set" directives
		 (SPR #1398).
01c,04oct91,rrr  passed through the ansification filter
		  -changed ASMLANGUAGE to _ASMLANGUAGE
		  -changed copyright notice
01b,29jan91,kdl  added include of vxWorks.h for conditional assembly.
01a,28jan91,kdl  modified original US Software version to use conditional
		 assembly for 68000/10 multiply and divide operations.
*/


/*
DESCRIPTION

|       ttl     FPAC 68K/FPOPNS: IEEE Single Precision Operations
|FPOPNS idnt    1,0             ; IEEE Single Precision Operations
|                               ; FPOPNS.A68
|
| * * * * * * * * * *
|
|       Copyright (c) 1985,1989 by
|       United States Software Corporation
|       14215 N.W. Science Park Drive
|       Portland, Oregon  97229
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
|       Released:       12 January 1989         V2.0
|
| * * * * * * * * * *
|

NOMANUAL
*/

#define	_ASMLANGUAGE
#include "vxWorks.h"
#include "uss_fp.h"


|       .set    comp64,0                |the 64 bit multiply/divide flag

|       opt     BRS             ; Default to forward branches SHORT
|
        .globl  FLOAT
        .globl  FIX
        .globl  INT
        .globl  AINT
        .globl  FPADD
        .globl  FPMUL
        .globl  FPDIV
        .globl  FPRDIV
        .globl  FPCMP
|
        .globl  GETFP1,FOPRSL
|
        .globl  FNANRS,FINFRS,FUNFRS,FZERRS
|
|
|       xref    FPERR,NANFLG,INFFLG,UNFFLG
|
|
|       .set    FBIAS,127               | Single precision format exponent bias
|
|
|       .set    CCRC,0x01               | Carry bit in CCR
|       .set    CCRV,0x02               | Overflow bit in CCR
|       .set    CCRZ,0x04               | Zero bit in CCR
|       .set    CCRN,0x08               | Negative bit in CCR
|       .set    CCRX,0x10               | Extend bit in CCR
|
|
|       .set    ERNAN,3
|       .set    EROVF,2
|       .set    ERUNF,1
|
|
        .text           | General code section
/*
|
|       page
|
|  FLOAT
|  =====
|  Float the integer value in D0 into a single precision floating
|  point value on the stack.
|
*/

FLOAT:
        moveal  sp@+,a0 | Return addr into a0
        subal   a2,a2           | Set a2 = 0
        andl    d0,d0
        jne     FLT01           | J/ value <> 0
|
        movel   d0,sp@- | Place 0 value on stack
        jmp     a0@             | Return
|
FLT01:
        jge     FLT02           | J/ value > 0
        subql   #1,a2           | Set a2 = -1
        negl    d0
|
FLT02:
        movel   #FBIAS+15,d2    | Default bias value
        swap    d0
        andw    d0,d0
        jeq     FLT03           | J/ 16 bit shift required
|
        swap    d0              | Undo the shift
        addw    #16,d2          | Reflect the larger number
|
FLT03:
        andl    d0,d0
        jmi     FLT06           | J/ value normalized
|
FLT04:
        subqw   #1,d2           | Dec exponent, shift mantissa
        asll    #1,d0
        jpl     FLT04           | J/ more shifts to do
|
FLT06:
        exg     d0,d2           | Position to standard d0/d2/a2 form
        jra     FOPRSL          | J/ single precision result (w/ round)
/*
|
|       page
|
|  FIX
|  ===
|  Routine to convert the single precision argument on the stack
|  to an integer value (with a dropoff flag).
|
*/

FIX:
        bsr     GETFP1          | Extract/unpack one single prec val
        bsr     FIX00           | Use internal routine
        jmp     a0@             | Return to caller
|
|
FIX00:
        andw    d0,d0
        jne     FIX01           | J/ value <> 0.0
|
        subl    d0,d0           | Return a zero value, no drop off
        rts
|
FIX01:
        cmpiw   #FBIAS,d0
        jcc     FIX02           | J/ abs() >= 1.0  [BCC == BHS]
|
        subl    d0,d0           | Return a zero value
        orib    #CCRC+CCRX,ccr  | Set carry/extend bits
| ###   ORI     #$11,CCR
        rts
|
FIX02:
        subiw   #FBIAS+31,d0
        jcs     FIX03           | J/ abs() < 2^31
|
        moveq   #-1,d0          | Set d0 to the maximum integer value
        lsrl    #1,d0           | d0 = 0x7FFFFFFF
        subl    a2,d0           | Account for the sign of the arg.
        rts
|
FIX03:
        negw    d0              | Positive shift count
        rorl    d0,d2           | Multibit shift
        moveq   #-1,d1          | Mask for bit dropout check
        lsrl    d0,d1
        notl    d1
        andl    d2,d1           | Bit(s) dropped left in d1
        eorl    d1,d2           | Integer value in d2
        movel   a2,d0
        eorl    d2,d0           | Negate as required, move to d0
        subl    a2,d0
        moveq   #-1,d2
        addl    d2,d1           | Set carry/extend if bits lost
        rts
/*
|
|       page
|
|  INT
|  ===
|  Return the largest integer smaller than the argument provided
|
*/

INT:
        bsr     GETFP1
        bsr     FIX00
|
        jcc     INT00           | J/ no bits lost
        cmpaw   #0,a2
        jeq     INT00           | J/ not negative
        subql   #1,d0           | Decrement integer value
INT00:
        jmp     a0@
/*
|
|       page
|
|  AINT
|  ====
|  Floating point corollary to the INT function
|
*/

AINT:
        bsr     GETFP1
        cmpiw   #FBIAS+23,d0    | Check for value too large
        jcc     FOPRSL          | J/ return with same value
|
        movew   d0,d3
        subiw   #FBIAS-1,d3
        jgt     AINT02          | J/ abs() >= 1.0
|
        movew   a2,d2
        jne     AINT01          | J/ 0.0 > value > -1.0
|
        movel   #0,sp@- | Return a zero value
        jmp     a0@
|
AINT01:
        movel   #0xBF800000,sp@-        | Return -1.0
        jmp     a0@
|
AINT02:
        moveq   #-1,d1          | Fill d1 with ones
        lsrl    d3,d1           | Shift mask over
        moveq   #1,d3
        addl    d1,d3           | Create increment bit
        andl    d2,d1           | Extract bits to drop
        jeq     FOPRSL          | J/ no drop off, return as provided
|
        eorl    d1,d2           | Remove bits that must be dropped
        cmpaw   #0,a2
        jeq     FOPRSL          | Bits dropped from a positive number
|
        addl    d3,d2           | Bump the magnitude (negative number)
        jcc     AINT03          | J/ no overflow
|
        roxll   #1,d2
        addqw   #1,d0
|
AINT03:
        jra     FOPRSL          | Return computed value
/*
|
|       page
|
|  FPADD
|  =====
|  Single precision add routine
|
*/

FPADD:
        bsr     GETFP2          | Fetch both operands
        cmpiw   #0xFF,d0
|
        jne     FPA010          | J/ operand not NaN/INF
|
        lsll    #1,d2           | Remove implicit bit
        jne     FNANRS          | J/  ?  + NaN -> NaN
|
        cmpiw   #0xFF,d1
        jne     FINFRS          | J/  0,num + INF -> INF
|
        lsll    #1,d3           | Remove implicit bit
        jne     FNANRS          | J/ INF + NaN -> NaN
|
        cmpal   a2,a3
        jne     FNANRS          | J/ INF - INF -> NaN
        jra     FINFRS          |    INF + INF -> INF
|
|
FPA010:
        cmpiw   #0xFF,d1
        jne     FPA040          | J/ not NaN or INF
|
        lsll    #1,d3           | Remove implicit bit
        jne     FNANRS          | J/ NaN + 0,num -> NaN
|
        moveal  a3,a2           | Move sign over
        jra     FINFRS          | INF result
|
|
FPXSUB:
        |dsw    0               | Entry for FPCMP
|
FPA040:
        andw    d1,d1
        jeq     FPA041          | J/ 0,num + 0 -> 0,num
|
        andw    d0,d0
        jne     FPA045          | J/ no zeroes involved
|
        movew   d1,d0           | Copy over data
        movel   d3,d2
        moveal  a3,a2
FPA041:
        jra     FOPR02          | Return w/o range check
|
|
FPA045:
        |dsw    0
|
        cmpw    d1,d0
        jcc     FPA060          | J/ op1.exp >= op2.exp
|
        exg     d2,d3           | Flip mantissas
        exg     d0,d1
        exg     a2,a3
|
FPA060:
        subw    d0,d1
        negw    d1
        cmpiw   #24,d1
        jhi     FOPRSL          | J/ op2 too small to matter
|
        lsrl    d1,d3
        cmpal   a2,a3
        jne     FPS100          | J/ subtract operation
|
        addl    d3,d2
        jcc     FOPRSL          | J/ no carry out
|
        roxrl   #1,d2           | Handle carry out
        addqw   #1,d0           | Bump the exponent
        jra     FOPRSL
|
|
FPS100:
        subl    d3,d2           | Do the subtract
        jeq     FZERRS          | J/ zero result
        jcc     FPS110
|
        negl    d2
|
        moveal  a3,a2           | Flip sign
|
FPS110:
        andl    d2,d2           | Normalization section
        jmi     FOPRSL          | J/ normalized
|
        subqw   #1,d0           | Decrease exponent value
FPS120:
        addl    d2,d2           | Left shift d2
        dbmi    d0,FPS120       | J/ not normalized
|
        jra     FOPRSL
/*
|
|       page
|
|  FPMUL
|  =====
|  Single precision multiply routine.
|
*/

FPMUL:
        bsr     GETFP2          | Fetch both operands
        movew   a2,d4
        movew   a3,d5
        eorw    d4,d5
        moveaw  d5,a2           | /* Result's sign */
|
        andw    d0,d0
        jne     FPM010          | J/ operand <> 0.0
|
        cmpiw   #0xFF,d1
        jeq     FNANRS          | J/ 0.0 * NaN,INF -> NaN
        jra     FZERRS          | J/ 0.0 * 0.0,num -> 0.0
|
FPM010:
        cmpiw   #0xFF,d0
        jne     FPM020          | J/ operand is a number
|
        lsll    #1,d2
        jne     FNANRS          | J/ NaN *  ?  -> NaN
|
        andw    d1,d1
        jeq     FNANRS          | J/ INF * 0.0 -> NaN
|
        cmpiw   #0xFF,d1
        jne     FINFRS          | J/ INF * num -> INF
        lsll    #1,d3
        jeq     FINFRS          | J/ INF * INF -> INF
        jra     FNANRS          | J/ INF * NaN -> NaN
|
FPM020:
        andw    d1,d1
        jeq     FZERRS          | J/ num * 0.0 -> 0.0
|
        cmpiw   #0xFF,d1
        jne     FPM040          | J/ num * num
|
        lsll    #1,d3
        jeq     FINFRS          | J/ num * INF -> INF
        jra     FNANRS          | J/ num * NaN -> NaN
|
|
FPM040:
        addw    d1,d0           | Calculate result`s exponent
        subw    #FBIAS-1,d0     | Remove double bias, assume shift
|
#if (CPU != MC68000 && CPU != MC68010)
        mulul   d2,d2:d3
#else
|       ifne    comp64-1        ;+++++

        movew   d2,d4           | Copy Lo byte to d4 (B)
        movew   d3,d5           | Copy Lo byte to d5 (D)
        swap    d2              | Position for high byte multiply (A)
        mulu    d2,d5           | A mid multiplication result (AD)
        swap    d3              | Position for high byte multiply (C)
        mulu    d3,d4           | A mid multiplication result (CB)
        mulu    d3,d2           | High order words multiplied (CA)
        addl    d4,d5           | Combine mid multiply results (CB + AD)
        subxw   d5,d5           | Preserve carry while...
        negw    d5              | ...positioning the...
        swap    d5              | ...result before...
        addl    d5,d2           | ...combining partial products
#endif
|       endc                    ;+++++
        jmi     FOPRSL          | Result normalized w/o shift
|
        subqw   #1,d0
        addl    d2,d2           | Do a left shift
        jra     FOPRSL
/*
|
|       page
|
|  FPDIV
|  =====
|  Single precision division operation.
|
*/

FPRDIV:
        bsr     GETFP2          | Get both operands
        exg     a2,a3           | Flip the operands
        exg     d0,d1
        exg     d2,d3
        jra     FPD001
|
FPDIV:
        bsr     GETFP2
|
FPD001:
        movew   a2,d4           | /* Compute result's sign */
        movew   a3,d5
        eorw    d4,d5
        moveaw  d5,a2
|
        andw    d0,d0
        jne     FPD010          | J/ divisor is not zero
|
        andw    d1,d1
        jeq     FNANRS          | J/ 0.0 / 0.0 -> NaN
        cmpiw   #0xFF,d1
        jne     FINFRS          | J/ num / 0.0 -> INF
        lsll    #1,d3
        jeq     FINFRS          | J/ INF / 0.0 -> INF
        jra     FNANRS          | J/ NaN / 0.0 -> NaN
|
FPD010:
        cmpiw   #0xFF,d0
        jne     FPD020          | J/ divisor is a normal number
|
        lsll    #1,d2
        jne     FNANRS          | J/  ?  / NaN -> NaN
        andw    d1,d1
        jeq     FZERRS          | J/ 0.0 / INF -> 0.0
        cmpiw   #0xFF,d1
        jne     FUNFRS          | J/ num / INF -> 0.0 (w/ underflow)
        jra     FNANRS          | J/ NaN,INF / INF -> NaN
|
FPD020:
        andw    d1,d1
        jeq     FZERRS          | J/ 0.0 / num -> 0.0
        cmpiw   #0xFF,d1
        jne     FPD040          | J/ num / num
|
        lsll    #1,d3
        jeq     FINFRS          | J/ INF / num -> INF
        jra     FNANRS          | J/ NaN / num -> NaN
/*
|
|
|  Division Algorithm:   A/(B+C) = A/B - A/B*C/B + A/B*(C/B)^2 - ...
|                                = A/B * (1 - C/B + (C/B)^2)
|
|  Choose C to be the low order byte of the 24 bit mantissa.  The
|  third and succeeding corrections terms (C squared and above)
|  can be neglected because they are at least thirty bits down.
|
*/

FPD040:
        subw    d1,d0
        negw    d0
        addw    #FBIAS,d0       | Restore bias
#if (CPU != MC68000 && CPU != MC68010)
        lsrl    #1,d3
        clrl    d4
        divul   d2,d3:d4
        movel   d4,d2
#else
|       ifne    comp64-1        ;+++++
        exg     d2,d3
|
        movel   d2,d4           | Copy A
        lsrl    #1,d4           | Insure no overflow during divide
        swap    d3
        divu    d3,d4           | Top fifteen bits of A/B
        movew   d4,d2           | A/B ultimately into d2
        movew   d4,d5           | Copy for later opn (A/B * C)
        swap    d2              | Position MS word
        clrw    d4              | Zero low order bits in d4
        divu    d3,d4           | Division of shifted remainder
        movew   d4,d2           | Complete A/B operation
|
        movel   d3,d4
        clrw    d4              | Create a shifted C in d4
        lsrl    #1,d4           | Insure no division overflow
        divu    d3,d4           | Division (complete since C is 8 bits)
|
        mulu    d5,d4           | A/B * C
        lsrl    #8,d4           | Position A/B * C
        lsrl    #7,d4
        subl    d4,d2
#endif
|       endc                    ;+++++
        jmi     FOPRSL          | J/ result normalized
|
        subqw   #1,d0
        addl    d2,d2           | Normalize it
        jra     FOPRSL
/*
|
|       page
|
|  FPCMP
|  =====
|  Single Precision comparison routine.
|
|  Compare the two arguments provided and set the condition code
|  register bits N, Z, and V as follows:
|
|      N  Z  V   Relation
|      =  =  =   ====================================
|      1  0  0   X > Y   (X is top argument on stack)
|      0  1  0   X = Y   (within FFUZZ specification)
|      0  0  0   X < Y
|      0  0  1   X does not compare to Y
|
|
*/

|       .set    FFUZZ,20                | Twenty bits of fuzz

FPCMP:
        bsr     GETFP2
        cmpiw   #0xFF,d0
        jne     FPC020          | J/ X is not INF or NaN
|
        lsll    #1,d2           | Remove implicit bit
        jeq     FPC005          | J/ X is INF
|
FPC001:
        |dsw    0               | **  X does not compare to Y  **
        moveq   #CCRV,d0        | CCR V bit
        movew   d0,ccr
        jmp     a0@             | Return
|
FPC005:
        cmpiw   #0xFF,d1
        jne     FPC009          | J/ Y is not INF or NaN
|
        lsll    #1,d3
        jne     FPC001          | J/ Y is NaN (does not compare)
|
        cmpaw   a2,a3
        jeq     FPC001          | /* J/ INF's with same sign - no compare */
|
FPC009:
        movew   a2,d0           | /* Result based on compl. of X's sign */
        eoriw   #CCRN,d0
        jra     FPC011
|
FPC010:
        movew   a3,d0           | /* Set result based on Y's sign */
FPC011:
        andiw   #CCRN,d0
        movew   d0,ccr
        jmp     a0@
|
|
FPC020:
        cmpiw   #0xFF,d1
        jne     FPC030          | J/ Y is not NaN or INF
|
        lsll    #1,d3
        jne     FPC001          | J/ Y is NaN - no comparison
        jra     FPC010          | /* Result is based on Y's sign */
|
FPC030:
        cmpaw   a2,a3
        jne     FPC010          | /* J/ signs different - use Y's sign */
|
        movew   d0,d4
        movew   d0,d5           | /* Assume X's exp is larger */
        subw    d1,d4           | Calc difference in exponents
        jpl     FPC031          | J/ positive
        movew   d1,d5           | /* Y's exp is larger */
        negw    d4
FPC031:
        |dsw    0
|
        lsrw    #1,d4
        jeq     FPC040          | Must subtract to obtain result
|
        subw    d0,d1
        subxw   d0,d0           | Set d0 to sign of Y[exp]-X[exp]
        movew   a2,d1
        eorw    d1,d0           | Flip if negative values
        jra     FPC011          | Result based on value in d0
|
FPC040:
        movew   d5,sp@- | Save max exp value, return address
        movel   a0,sp@-
|
        movew   a2,d5
        notw    d5              | Flip sign of opnd (force subtract)
        moveaw  d5,a2
|
        pea     FPC041
        movel   sp@+,a0 | Return address in a0
|
        jra     FPXSUB
|
FPC041:
        movel   sp@+,d0
        movel   sp@,a0
        movel   d0,sp@
        movel   a0,sp@-
|
        bsr     GETFP1          | Fetch and unpack result
|
        movew   sp@+,d5 | Recall max exp value
|
        andw    d0,d0
        jeq     FPC050          | J/ zero result
|
        subw    d0,d5
        cmpiw   #FFUZZ,d5
        jge     FPC050          | J/ within FFUZZ specification - zero
|
        movew   a2,d0           | Sign of result into d0
        jra     FPC011          | Comparison result from sign of sub
|
FPC050:
        subw    d0,d0           | Force CCR to say "Z"
        jmp     a0@             | Return
/*
|
|       page
|
|  GETFP2
|  ======
|  Routine called to extract two single precision arguments from
|  the system stack and place them in the 68000`s registers.
|
*/

GETFP2:
        moveal  sp@+,a1 | /* GETFP2's return address */
        moveal  sp@+,a0 | Calling routines return address
|
        movel   sp@+,d0 | Get TOS (source) operand
        asll    #1,d0           | Sign bit to carry
        subxw   d2,d2           | Fill d2 with sign bit
        movew   d2,a2           | Sign bit info to a2
        roll    #8,d0           | Left justify mantissa, position exp
        movel   d0,d2           | Copy into mantissa register
        andiw   #0xFF,d0                | Mask to exponent field
        jeq     GETF21          | J/ zero value
|
        andiw   #0xFE00,d2      | Zero sign bit and exponent bits in d2
        lsrl    #1,d2           | Position mantissa
        bset    #31,d2          | Set implicit bit in d2
|
GETF21:
        |dsw    0
|
        movel   sp@+,d1 | Get NOS (source) operand
        asll    #1,d1           | Sign bit to carry
        subxw   d3,d3           | Replicate sign bit throughout d3
        movew   d3,a3           | Sign bit info into a3
        roll    #8,d1           | Left justify mantissa, position exp
        movel   d1,d3           | Copy into mantissa register
        andiw   #0xFF,d1                | Mask to exponent field
        jeq     GETF22          | J/ zero value
|
        andiw   #0xFE00,d3      | Zero sign bit and exponent bits in d3
        lsrl    #1,d3           | Position mantissa
        bset    #31,d3          | Set implicit bit in d3
|
GETF22:
        jmp     a1@             | Return to caller, its ret addr in a0
/*
|
|
|       page
|
|  GETFP1
|  ======
|  Routine called to extract a single precision argument from the
|  system stack and place it (unpacked) into the 68000's registers.
|
*/

GETFP1:
        moveal  sp@+,a1 | /* Get GETFP1's return address */
        moveal  sp@+,a0 | Get calling routines return address
|
        movel   sp@+,d0 | Get argument
        asll    #1,d0           | Sign bit into carry
        subxw   d2,d2           | Replicate sign bit throughout d2
        movew   d2,a2           | Sign bit info into a2
        roll    #8,d0           | Left justify mantissa, position exp
        movel   d0,d2
        andiw   #0xFF,d0                | Mask to exponent field
        jeq     GETF11          | J/ zero value
|
        andiw   #0xFE00,d2      | Zero sign bit and exponent bits in d2
        lsrl    #1,d2           | Position mantissa
        bset    #31,d2          | Set implicit bit
|
GETF11:
        jmp     a1@             | Return to caller, its ret addr in a0
/*
|
|
|       page
|
|  FOPRSL
|  ======
|  Single precision floating point result (main entry w/ round).
|  Mantissa in D2, exponent in D0, sign in A2, and return address
|  in A0.  Place a format value on the stack.
|
*/

FOPRSL:
        addl    #0x80,d2                | Round the value
        jcc     FOPR01          | J/ no carry out
|
        roxrl   #1,d2           | Adjust mantissa and exponent
        addqw   #1,d0
|
FOPR01:
        andw    d0,d0
        jle     FUNFRS          | J/ underflow
|
        cmpiw   #0xFF,d0                | Check for overflow
        jge     FINFRS          | J/ overflow
|
FOPR02:
        andiw   #0xFF00,d2      | Trim mantissa
        asll    #1,d2           | Drop implicit bit
        addw    d0,d2           | Blend in the exponent
        rorl    #8,d2           | Reposition value
        movew   a2,d0
        aslw    #1,d0           | Sign bit into carry/extend
        roxrl   #1,d2           | Finish construction of value
        movel   d2,sp@-
|
        moveb   #0,FPERR        | No floating point error
        jmp     a0@             | Return
|
|
|
FNANRS:
        moveq   #-1,d0          | Set d0 to 0xFFFFFFFF
        movel   d0,sp@- | NaN value
|
        moveb   #ERNAN,FPERR
        moveb   #-1,NANFLG
        jmp     a0@
|
|
FINFRS:
        movel   a2,d0           | Get sign information
        movel   #0xFF000000,d1
        aslw    #1,d0
        roxrl   #1,d1
        movel   d1,sp@-
|
        moveb   #EROVF,FPERR
        moveb   #-1,INFFLG
        jmp     a0@
|
|
FUNFRS:
        moveb   #ERUNF,FPERR
        moveb   #-1,UNFFLG
        jra     FZER01
|
|
FZERRS:
        moveb   #0,FPERR
|
|
FZER01:
        subl    d0,d0
        movel   d0,sp@-
        jmp     a0@
|
|
|       end
