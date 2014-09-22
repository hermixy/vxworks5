/* Copyright 1991-1992 Wind River Systems, Inc. */
	.data
	.globl	_copyright_wind_river
	.long	_copyright_wind_river

/*
modification history
--------------------
01h,04sep98,yh   fixed DAINT for floor function.
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

|       ttl     FPAC 68K/DPOPNS: IEEE Double Precision Operations
|DPOPNS idnt    1,0             ; IEEE Double Precision Operations
|                               ; DPOPNS.A68
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
|

NOMANUAL
*/

#define	_ASMLANGUAGE
#include "vxWorks.h"
#include "uss_fp.h"

|      .set    comp64,0                |flag for 64 bit multiply/divide

|       opt     BRS             ; Default to forward branches SHORT
|
        .globl  DFLOAT
        .globl  DFIX
        .globl  DINT
        .globl  DAINT
        .globl  DPADD
        .globl  DPMUL
        .globl  DPDIV
        .globl  DPRDIV
        .globl  DPCMP
|
        .globl  DNANRS
        .globl  DINFRS
        .globl  DUNFRS
        .globl  DZERRS
|
        .globl  GETDP1,DOPRSL
|
|
|
|       .set    DBIAS,1023              | Double precision format exponent bias
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
|       xref    FPERR
|       xref    NANFLG
|       xref    INFFLG
|       xref    UNFFLG
|
|
        .text
|
/*
|       page
|
|  DFLOAT
|  ======
|  Float the integer value in D0:D1 into a double precision floating
|  point value on the stack.
|
*/

DFLOAT:
        moveal  sp@+,a0 | Return addr into a0
        subal   a2,a2           | Set a2 = 0
        andl    d0,d0
        jne     DFLT01          | J/ value <> 0
        andl    d1,d1
        jne     DFLT02          | J/ value > 0
|
        movel   d0,sp@- | Place 0 value on stack
        movel   d0,sp@-
        jmp     a0@             | Return
|
DFLT01:
        jge     DFLT02          | J/ value > 0
        subql   #1,a2           | Set a2 = -1
        negl    d1
        negxl   d0
|
DFLT02:
        movel   #DBIAS+63,d2    | Default bias value
        andl    d0,d0
        jne     DFLT03          | J/ 32 bit shift not required
|
        subw    #32,d2          | Reduce exponent
        exg     d0,d1           | Do shift (since d0 is zero)
|
DFLT03:
        cmpil   #0x0000FFFF,d0
        jhi     DFLT04          | J/ 16 bit shift not required
|
        swap    d0              | Do the shift
        swap    d1
        movew   d1,d0
        clrw    d1
        subiw   #16,d2
|
DFLT04:
        andl    d0,d0
        jmi     DFLT07          | J/ value normalized
|
DFLT05:
        subqw   #1,d2           | Dec exponent, shift mantissa
        asll    #1,d1
        roxll   #1,d0
        jpl     DFLT05          | J/ more shifts to do
|
DFLT07:
        exg     d0,d2           | Position to standard d0/d2:d3/a2 form
        movel   d1,d3
        jra     DOPRSL          | J/ double precision result (w/ round)
/*
|
|       page
|
|  DFIX
|  ====
|  Routine to convert the double precision argument on the stack
|  to an integer value (with a dropoff flag).
|
*/

DFIX:
        bsr     GETDP1          | Extract/unpack one double prec val
        bsr     DFIX00          | Use internal routine
        jmp     a0@             | Return to caller
|
|
DFIX00:
        andw    d0,d0
        jne     DFIX01          | J/ value <> 0.0
|
        subl    d0,d0           | Return a zero value, no drop off
        clrl    d1
        rts
|
DFIX01:
        cmpiw   #DBIAS,d0
        jcc     DFIX02          | J/ abs() >= 1.0  [BCC == BHS]
|
        clrl    d0              | Return a zero value
        clrl    d1
        orib    #CCRC+CCRX,ccr  | Set carry/extend bits
| ##    ORI     #$11,CCR
        rts
|
DFIX02:
        subiw   #DBIAS+63,d0
        jlt     DFIX03          | J/ abs() < 2^63
|
        moveq   #-1,d0          | Set d0:d1 to the maximum integer value
        moveq   #-1,d1
        lsrl    #1,d0           | d0:d1 = 0x7FFFFFFFFFFFFFFF
        movel   a2,d2
        subl    d2,d1           | Account for the sign of the arg.
        subxl   d2,d0
        rts
|
DFIX03:
        clrl    d1              | Clear bit drop off accum
|
        negw    d0              | Positive shift count
        cmpiw   #32,d0
        jlt     DFIX04          | J/ less than a word shift
|
        andl    d3,d3
        sne     d1              | Set d1 = 0FFH if d3 <> 0
|
        movel   d2,d3
        clrl    d2
|
        subiw   #32,d0
|
DFIX04:
        cmpiw   #16,d0
        jlt     DFIX05          | J/ less than a swap left
|
        orw     d3,d1           | Accum any bits dropped off
|
        movew   d2,d3           | Do a swap shift (16 bits)
        swap    d3
        clrw    d2
        swap    d2
|
        subiw   #16,d0
|
DFIX05:
        subqw   #1,d0
        jlt     DFIX07          | J/ shifting complete
|
DFIX06:
        lsrl    #1,d2
        roxrl   #1,d3
|
        roxll   #1,d1
|
        dbra    d0,DFIX06
|
DFIX07:
        cmpaw   #0,a2           | Check for negative value
        jeq     DFIX08          | J/ positive
|
        negl    d3
        negxl   d2
|
DFIX08:
        moveq   #-1,d0
        addl    d1,d0           | Set carry if bits lost
|
        exg     d2,d0           | Move integer result to d0:d1
        exg     d3,d1
        rts
/*
|
|       page
|
|  DINT
|  ====
|  Return the largest integer smaller than the argument provided
|
*/

DINT:
        bsr     GETDP1
        bsr     DFIX00
|
        jcc     DINT00          | J/ no bits lost
        cmpaw   #0,a2
        jeq     DINT00          | J/ not negative
        subql   #1,d1           | Decrement integer value
        jcc     DINT00          | J/ no borrow
        subql   #1,d0
DINT00:
        jmp     a0@
/*
|
|       page
|
|  DAINT
|  =====
|  Floating point corollary to the DINT function
|
*/

DAINT:
        bsr     GETDP1
        cmpiw   #DBIAS+52,d0    | Check for value too large
        jcc     DAIN10          | J/ return with same value
|
        movew   d0,d4           | Copy the exponent value
        subiw   #DBIAS-1,d4
        jgt     DAIN02          | J/ abs() >= 1.0
|
	cmpiw   #0, d2          | check if mantissa is zero
	jne     DAIN09
	cmpiw   #0, d3
	jeq     DAIN08

DAIN09:	
        movew   a2,d4
        jne     DAIN01          | J/ 0.0 > value > -1.0
|
DAIN08:	
        clrl    sp@-            | Return a zero value
        clrl    sp@-
        jmp     a0@
|
DAIN01:
        clrl    sp@-            | Return -1.0
        movel   #0xBFF00000,sp@-
        jmp     a0@
|
DAIN02:
        moveq   #-1,d1          | Fill d1 with ones
|
        cmpiw   #32,d4          | See which word needs to be masked
        jle     DAIN03          | J/ low order word zeroed, mask hi wd
|
        subiw   #32,d4
        lsrl    d4,d1           | Adjust mask
        movel   d3,d4
        andl    d1,d4           | Extract bits to drop
        jeq     DAIN10          | J/ no drop off, return as provided
|
        eorl    d4,d3           | Strip the bits
        cmpaw   #0,a2
        jeq     DAIN10          | J/ positive number
|
        clrl    d4              | (for ADDX below)
        addql   #1,d1           | Change mask to increment value
        addl    d1,d3
        addxl   d4,d2           | Perform any carry
        jra     DAIN04          | J/ rejoin flow
|
DAIN03:
        lsrl    d4,d1           | Adjust high word mask
        movel   d2,d4
        andl    d1,d4           | Get bits to strip
        eorl    d4,d2           | Strip bits
|
        orl     d3,d4           | Record any dropped bits from lo word
        clrl    d3              | Clear the low word
|
        tstl    d4
        jeq     DAIN10          | J/ no dropoff
        cmpaw   #0,a2
        jeq     DAIN10          | J/ positive number
|
        addql   #1,d1           | Turn mask into increment value
        addl    d1,d2
|
DAIN04:
        jcc     DAIN10          | J/ no overflow
|
        roxrl   #1,d2           | Right shift the mantissa
|**     ROXR.L  #1,D3           ; (not nec -> mantissa = 80..00)
        addqw   #1,d0           | Bump the exponent
|
DAIN10:
        jra     DOPRSL          | Return computed value
/*
|
|       page
|
|  DPADD
|  =====
|  Double precision add routine
|
*/

DPADD:
        bsr     GETDP2          | Fetch both operands
        cmpiw   #0x7FF,d0
|
        jne     DPA010          | J/ operand not NaN/INF
|
        lsll    #1,d2           | Remove implicit bit
        jne     DNANRS          | J/  ?  + NaN -> NaN
|
        cmpiw   #0x7FF,d1
        jne     DINFRS          | J/  0,num + INF -> INF
|
        lsll    #1,d4           | Remove implicit bit
        jne     DNANRS          | J/ INF + NaN -> NaN
|
        cmpal   a2,a3
        jne     DNANRS          | J/ INF - INF -> NaN
        jra     DINFRS          |    INF + INF -> INF
|
|
DPA010:
        cmpiw   #0x7FF,d1
        jne     DPA040          | J/ not NaN or INF
|
        lsll    #1,d4           | Remove implicit bit
        jne     DNANRS          | J/ NaN + 0,num -> NaN
|
        moveal  a3,a2           | Move sign over
        jra     DINFRS          | INF result
|
|
DPXSUB:
        |dsw    0               | Entry for DPCMP
|
DPA040:
        andw    d1,d1
        jeq     DOPRSL          | J/ 0,num + 0 -> 0,num
|
        andw    d0,d0
        jne     DPA045          | J/ no zeroes involved
|
        movew   d1,d0           | Copy over data
        movel   d4,d2
        movel   d5,d3
        moveal  a3,a2
        jra     DOPRSL
|
|
DPA045:
        |dsw    0
|
        cmpw    d1,d0
        jcc     DPA060          | J/ op1.exp >= op2.exp
|
        exg     d2,d4           | Flip mantissas
        exg     d3,d5
        exg     d0,d1
        exg     a2,a3
|
DPA060:
        subw    d0,d1
        negw    d1
        cmpiw   #53,d1
        jhi     DOPRSL          | J/ op2 too small to matter
|
        cmpiw   #32,d1
        jlt     DPA061          | J/ less than a word shift
|
        movel   d4,d5
        clrl    d4
|
        subiw   #32,d1
|
DPA061:
        cmpiw   #16,d1
        jlt     DPA062          | J/ less than a swap shift left
|
        movew   d4,d5           | Do a swap shift (16 bits)
        clrw    d4
        swap    d5
        swap    d4
|
        subiw   #16,d1
|
DPA062:
        moveq   #-1,d7          | Mask in d7
        lsrl    d1,d7
        rorl    d1,d4
        rorl    d1,d5
        andl    d7,d5           | Trim bits
        eorl    d4,d5           | Mix in d4 -> d5 bits
        andl    d7,d4           | Strip d4 -> d5 bits
        eorl    d4,d5           | Finish   LSL  d0,d4:d5
|
        cmpal   a2,a3
        jne     DPS100          | J/ subtract operation
|
        addl    d5,d3
        addxl   d4,d2
        jcc     DOPRSL          | J/ no carry out
|
        roxrl   #1,d2           | Handle carry out
        roxrl   #1,d3
        addqw   #1,d0           | Bump the exponent
        jra     DOPRSL
|
|
DPS100:
        subl    d5,d3           | Do the subtract
        subxl   d4,d2
        jcc     DPS110
|
        negl    d3
        negxl   d2
|
        moveal  a3,a2           | Flip sign
|
DPS110:
        andl    d2,d2           | Normalization section
        jne     DPS115          | J/ top word is not zero
|
        exg     d2,d3
        subiw   #32,d0          | Reduce exponent appropriately
|
        andl    d2,d2
        jeq     DZERRS          | J/ zero result
|
DPS115:
        cmpil   #0x0000FFFF,d2
        jhi     DPS118          | J/ less than a swap shift left
|
        swap    d2              | Do a swap shift (16 bits)
        swap    d3
        movew   d3,d2
        clrw    d3
|
        subiw   #16,d0
|
DPS118:
        tstl    d2
        jmi     DOPRSL          | J/ normalized
|
        subqw   #1,d0           | Decrease exponent value
DPS120:
        lsll    #1,d3
        roxll   #1,d2
        dbmi    d0,DPS120       | J/ not normalized
|
        jra     DOPRSL
/*
|
|       page
|
|  DPMUL
|  =====
|  Single precision multiply routine.
|
*/

DPMUL:
        bsr     GETDP2          | Fetch both operands
        movew   a2,d6
        movew   a3,d7
        eorw    d6,d7
        moveaw  d7,a2           | /* Result's sign */
|
        andw    d0,d0
        jne     DPM010          | J/ operand <> 0.0
|
        cmpiw   #0x7FF,d1
        jeq     DNANRS          | J/ 0.0 * NaN,INF -> NaN
        jra     DZERRS          | J/ 0.0 * 0.0,num -> 0.0
|
DPM010:
        cmpiw   #0x7FF,d0
        jne     DPM020          | J/ operand is a number
|
        lsll    #1,d2
        jne     DNANRS          | J/ NaN *  ?  -> NaN
|
        andw    d1,d1
        jeq     DNANRS          | J/ INF * 0.0 -> NaN
|
        cmpiw   #0x7FF,d1
        jne     DINFRS          | J/ INF * num -> INF
        lsll    #1,d4
        jeq     DINFRS          | J/ INF * INF -> INF
        jra     DNANRS          | J/ INF * NaN -> NaN
|
DPM020:
        andw    d1,d1
        jeq     DZERRS          | J/ num * 0.0 -> 0.0
|
        cmpiw   #0x7FF,d1
        jne     DPM040          | J/ num * num
|
        lsll    #1,d4
        jeq     DINFRS          | J/ num * INF -> INF
        jra     DNANRS          | J/ num * NaN -> NaN
|
|
DPM040:
        addw    d1,d0           | Calculate result`s exponent
        subw    #DBIAS-1,d0     | Remove double bias, assume shift
        movew   d0,a3           | Save exponent in a3, free up d0
|
DPM050:
        |dsw    0               | Entry for DPDIV......
        movel   d2,d0           | A * D
        movel   d5,d1
#if (CPU != MC68000 && CPU != MC68010)
|       ifne    comp64-1        ;+++++
        mulul   d0,d0:d1
#else
        bsr     DPM500          | 32 x 32 partial multiply
#endif
|       endc                    ;+++++
        movel   d0,d5           | Save word
|
        movel   d3,d0           | B * C
        movel   d4,d1
#if (CPU != MC68000 && CPU != MC68010)
|       ifne    comp64-1        ;+++++
        mulul   d0,d0:d1
#else
        bsr     DPM500          | 32 x 32 partial multiply
#endif
|       endc                    ;+++++
        movel   d0,d3
|
        movel   d2,d0           | A * C
        movel   d4,d1

#if (CPU != MC68000 && CPU != MC68010)
|       ifne    comp64-1        ;+++++
        mulul   d0,d0:d1
#else
        bsr     DPM500          | 32 x 32 partial multiply
#endif
|       endc                    ;+++++
|
        clrl    d2              | Sum partial multiply results
        addl    d5,d3
        addxl   d2,d2           | Preserve possible carry out
        addl    d1,d3
        addxl   d0,d2
        movew   a3,d0           | Restore exponent
|
        tstl    d2              | Check for normalized result
        jmi     DOPRSL          | J/ result normalized
|
        asll    #1,d3           | One left shift to normalize
        roxll   #1,d2
        subqw   #1,d0
        jra     DOPRSL

#if (CPU == MC68000 || CPU == MC68010)
|       ifne    comp64-1        ;+++++
|
|  DPM500: 32 x 32 partial multiply routine
|
|  Multiply D0 by D1 (long words) yielding a 64 bit result.
|
DPM500:
        movew   d0,d6           | Copy over B
        swap    d1
        mulu    d1,d6           | Produce BC
        swap    d0
        swap    d1
        movew   d1,d7
        mulu    d0,d7           | Produce AD
        addl    d6,d7           | Produce BC+AD in d6:d7
        subxl   d6,d6
        negl    d6
        swap    d6              | Properly position BC+AD
        swap    d7
        movew   d7,d6
|
        movew   d1,d7           | Save D (slide in under BC+AD)
        swap    d1
        mulu    d0,d1           | Produce AC
        swap    d0
        mulu    d7,d0
        clrw    d7              | Restore d6:d7
        exg     d0,d1           | AC:BD, then result, in d0:d1
        addl    d7,d1
        addxl   d6,d0
        rts
#endif
/*
|       endc                    ;+++++
|
|       page
|
|  DPDIV
|  =====
|  Double precision division operation.
|
*/

DPRDIV:
        bsr     GETDP2          | Get both operands
        exg     a2,a3           | Flip the operands
        exg     d0,d1
        exg     d2,d4
        exg     d3,d5
        jra     DPD001
|
DPDIV:
        bsr     GETDP2
|
DPD001:
        movew   a2,d6           | /* Compute result's sign */
        movew   a3,d7
        eorw    d6,d7
        moveaw  d7,a2
|
        andw    d0,d0
        jne     DPD010          | J/ divisor is not zero
|
        andw    d1,d1
        jeq     DNANRS          | J/ 0.0 / 0.0 -> NaN
        cmpiw   #0x7FF,d1
        jne     DINFRS          | J/ num / 0.0 -> INF
        lsll    #1,d4
        jeq     DINFRS          | J/ INF / 0.0 -> INF
        jra     DNANRS          | J/ NaN / 0.0 -> NaN
|
DPD010:
        cmpiw   #0x7FF,d0
        jne     DPD020          | J/ divisor is a normal number
|
        lsll    #1,d2
        jne     DNANRS          | J/  ?  / NaN -> NaN
        andw    d1,d1
        jeq     DZERRS          | J/ 0.0 / INF -> 0.0
        cmpiw   #0x7FF,d1
        jne     DUNFRS          | J/ num / INF -> 0.0 (w/ underflow)
        jra     DNANRS          | J/ NaN,INF / INF -> NaN
|
DPD020:
        andw    d1,d1
        jeq     DZERRS          | J/ 0.0 / num -> 0.0
        cmpiw   #0x7FF,d1
        jne     DPD040          | J/ num / num
|
        lsll    #1,d4
        jeq     DINFRS          | J/ INF / num -> INF
        jra     DNANRS          | J/ NaN / num -> NaN

DPD040:
#if (CPU != MC68000 && CPU != MC68010)
        subw    d0,d1
        addw    #DBIAS,d1
        movew   d1,d0
        lsrl    #1,d4
        roxrl   #1,d5
        divul   d2,d4:d5
        movel   d5,d7
        mulul   d3,d6:d7
        negl    d7
        subxl   d6,d4
        jcc     dpd54
dpd53:
        subl    #1,d5
        addl    d3,d7
        addxl   d2,d4
        jcc     dpd53
        cmpl    d2,d4
        jne     dpd54
        addl    #1,d5
        clrl    d3
        jra     dpd55
dpd54:
        divul   d2,d4:d7
        movel   d7,d3
dpd55:
        movel   d5,d2
        jmi     dpd52
        addl    d3,d3
        addxl   d2,d2
        subl    #1,d0
dpd52:
        jra     DOPRSL


#else

|       ifne    comp64-1        ;+++++
|
/*
|  Division Algorithm:
|
|  (1)  Use a two stage recipication approximation to obtain 1/B
|       (a)  X0 = 1 / (B0 + B1)    (B0 is ms 16 bits of B, B1 = B - B0)
|               = 1/B0 * (1 - B1/B0)  { accurate to 28+ bits)
|       (b)  X1 = X0 * (2 - B*X0)  (N-R iteration, 55+ bits accuracy)
|  (2)  Use DPMUL entry to produce A * X1 (64 bit computation)
*/

|
|
|DPD040:
        subw    d0,d1
        addw    #DBIAS,d1               | Restore bias
        movew   d1,a3           | Save exponent in a3
|
        movel   d5,sp@- | Save dividend mantissa on stack
        movel   d4,sp@-
|
        moveq   #1,d6           | Create a 1.0 entry
        rorl    #2,d6           | Set d4 = 4000 0000H
        swap    d2
        divu    d2,d6           | Top fifteen bits of 1/B
        movew   d6,d4           | 1/B ultimately into d4:d5
        movew   d6,d7           | Save for correction term
        swap    d4              | Position MS word
        clrw    d6              | Zero low order bits in d6
        divu    d2,d6           | Division of shifted remainder
        movew   d6,d4           | Complete 1/B approximation
|
        movel   d2,d6
        clrw    d6              | Create a shifted C in d6 (16 bits)
        lsrl    #1,d6           | Insure no division overflow
        divu    d2,d6           | C/B approximation
        mulu    d7,d6
        lsrl    #8,d6           | Position correction term
        lsrl    #7,d6
        subl    d6,d4
        lsll    #1,d4           | Left shift approximation
        subxl   d6,d6           | Do not overflow
        orl     d6,d4           | 28+ bit 1/B approx in d4, B in d2:d3
        swap    d2
|
        movel   d4,d0           | Save X0 in d0
        bsr     DPD500          | d4 * d2:d3 -> d2:d3 (in place)
        negl    d3
        negxl   d2
        movel   d0,d4
        bsr     DPD500
|
        asll    #1,d3
        roxll   #1,d2
        jcc     DPD080          | J/ no shift out
        moveq   #-1,d2          | Set d2:d3 to FFFFFFFF:FFFFFFFF
        moveq   #-1,d3
DPD080:
        |dsw    0
|
        movel   sp@+,d4 | Fetch dividend
        movel   sp@+,d5
        jra     DPM050
|
|  DPD500:  Multiply D2:D3 by D4, top 64 bits of result in D2:D3
|
DPD500:
        movew   d2,d1
        mulu    d4,d1           | BY partial product
        swap    d2
        swap    d3
        movew   d3,d7
        movew   d2,d6
        mulu    d4,d7
        mulu    d4,d6           | AY:CY in d6:d7
        movew   d6,d7
        clrw    d6
        swap    d6
        swap    d7              | LSR #16,d6:d7
        addl    d1,d7
        clrl    d1
        addxl   d1,d6           | 00:BY + 0A:YC
|
        swap    d4
        movew   d3,d5
        movew   d2,d1
        mulu    d4,d5
        mulu    d4,d1           | AX:CX in d1:d5
        addl    d5,d7
        addxl   d1,d6           | AX:CX + 00:BY + 0A:CY
|
        swap    d2
        swap    d3
        mulu    d4,d3
        mulu    d4,d2           | BX:DX in d2:d3
        movew   d2,d3
        clrw    d2
        swap    d2
        swap    d3              | LSR #16,d2:d3
        addl    d7,d3
        addxl   d6,d2           | Result in d2:d3
|
        swap    d4              | Restore d4
        rts
#endif
|       endc                    ;+++++

/*
|
|       page
|
|  DPCMP
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
|
|
*/

|       .set    DFUZZ,51                | Fifty-one bits of fuzz

DPCMP:
        bsr     GETDP2
        cmpiw   #0x7FF,d0
        jne     DPC020          | J/ X is not INF or NaN
|
        lsll    #1,d2           | Remove implicit bit
        jeq     DPC005          | J/ X is INF
|
DPC001:
        |dsw    0               | **  X does not compare to Y  **
        moveq   #CCRV,d0        | CCR V bit
        movew   d0,ccr
        jmp     a0@             | Return
|
DPC005:
        cmpiw   #0x7FF,d1
        jne     DPC009          | J/ Y is not INF or NaN
|
        lsll    #1,d4
        jne     DPC001          | J/ Y is NaN (does not compare)
|
        cmpaw   a2,a3
        jeq     DPC001          | /* J/ INF's with same sign - no compare */
|
DPC009:
        movew   a2,d0           | /* Result based on compl. of X's sign */
        eoriw   #CCRN,d0
        jra     DPC011
|
DPC010:
        movew   a3,d0           | /* Set result based on Y's sign */
DPC011:
        andiw   #CCRN,d0
        movew   d0,ccr
        jmp     a0@
|
|
DPC020:
        cmpiw   #0x7FF,d1
        jne     DPC030          | J/ Y is not NaN or INF
|
        lsll    #1,d4
        jne     DPC001          | J/ Y is NaN - no comparison
        jra     DPC010          | /* Result is based on Y's sign */
|
DPC030:
        cmpaw   a2,a3
        jne     DPC010          | /* J/ signs different - use Y's sign */
|
        movew   d0,d6
        movew   d0,d7           | /* Assume X's exp is larger */
        subw    d1,d6           | Calc difference in exponents
        jpl     DPC031          | J/ positive
        movew   d1,d7           | /* Y's exp is larger */
        negw    d6
DPC031:
        |dsw    0
|
        lsrw    #1,d6
        jeq     DPC040          | Must subtract to obtain result
|
        subw    d0,d1
        subxw   d0,d0           | Set d0 to sign of Y[exp]-X[exp]
        movew   a2,d1
        eorw    d1,d0           | Flip if negative values
        jra     DPC011          | Result based on value in d0
|
DPC040:
        movew   d7,sp@- | Save max exp value, return address
        movel   a0,sp@-
|
        movew   a2,d7
        notw    d7              | Flip sign of opnd (force subtract)
        moveaw  d7,a2
|
        pea     DPC041
        movel   sp@+,a0 | Return address in a0
|
        jra     DPXSUB
|
DPC041:
        movel   sp@+,d0
        movel   sp@+,d1
        moveal  sp@,a0
        movel   d1,sp@
        movel   d0,sp@-
        movel   a0,sp@-
        bsr     GETDP1          | Fetch and unpack result
|
        movew   sp@+,d5 | Recall max exp value
|
        andw    d0,d0
        jeq     DPC050          | J/ zero result
|
        subw    d0,d5
        cmpiw   #DFUZZ,d5
        jge     DPC050          | J/ within FFUZZ specification - zero
|
        movew   a2,d0           | Sign of result into d0
        jra     DPC011          | Comparison result from sign of sub
|
DPC050:
        subw    d0,d0           | Force CCR to say "Z"
        jmp     a0@             | Return
/*
|
|       page
|
|  GETDP2
|  ======
|  Routine called to extract two double precision arguments from
|  the system stack and place them in the 68000`s registers.
|
*/

GETDP2:
        moveal  sp@+,a1 | /* GETDP2's return address */
        moveal  sp@+,a0 | Calling routines return address
|
        movel   sp@+,d0 | Get TOS (source) operand
        movel   sp@+,d3 | Get low long word
        subw    d2,d2           | Clear carry
        roxll   #1,d0           | Sign bit to bit 0
        subxw   d2,d2           | Fill d2 with sign bit
        moveaw  d2,a2           | Sign bit info to a2
        roll    #8,d0           | Left justify mantissa, position exp
        roll    #3,d0
        movel   d0,d2           | Copy into mantissa register
        andiw   #0x7FF,d0       | Mask to exponent field
        jeq     GETD21          | J/ zero value
|
        eorw    d0,d2           | Zero exponent bits in d2
        lsrl    #1,d2           | Position mantissa
        bset    #31,d2          | Set implicit bit in d2
        roll    #8,d3           | Position lo long word of mantissa
        roll    #3,d3
        eorw    d3,d2           | Clever use of EOR to move bits
        andiw   #0xF800,d3      | Trim off bits moved into d2
        eorw    d3,d2           | Remove noise in d2
|
GETD21:
        |dsw    0
|
        movel   sp@+,d1 | Get NOS (source) operand
        movel   sp@+,d5
        subw    d4,d4           | Clear carry
        roxll   #1,d1           | Sign bit carry
        subxw   d4,d4           | Replicate sign bit throughout d4
        moveaw  d4,a3           | Sign bit info into a3
        roll    #8,d1           | Left justify mantissa, position exp
        roll    #3,d1
        movel   d1,d4           | Copy into mantissa register
        andiw   #0x7FF,d1       | Mask to exponent field
        jeq     GETD22          | J/ zero value
|
        eorw    d1,d4           | Zero exponent bits in d4
        lsrl    #1,d4           | Position mantissa
        bset    #31,d4          | Set implicit bit in d4
        roll    #8,d5           | Position low long word of mantissa
        roll    #3,d5
        eorw    d5,d4           | Clever use of EOR to move bits
        andiw   #0xF800,d5      | Trim off bits moved into d4
        eorw    d5,d4           | Remove noise in d4
|
GETD22:
        jmp     a1@             | Return to caller, its ret addr in a0
/*
|
|
|       page
|
|  GETDP1
|  ======
|  Routine called to extract a double precision argument from the
|  system stack and place it (unpacked) into the 68000's registers.
|
*/

GETDP1:
        moveal  sp@+,a1 | /* Get GETDP1's return address */
        moveal  sp@+,a0 | Get calling routines return address
|
        movel   sp@+,d0 | Get argument
        movel   sp@+,d3
        subw    d2,d2           | Clear carry
        roxll   #1,d0           | Sign bit into carry
        subxw   d2,d2           | Replicate sign bit throughout d2
        moveaw  d2,a2           | Sign bit info into a2
        roll    #8,d0           | Left justify mantissa, position exp
        roll    #3,d0
        movel   d0,d2
        andiw   #0x7FF,d0       | Mask to exponent field
        jeq     GETD11          | J/ zero value
|
        eorw    d0,d2           | Zero exponent bits in d2
        lsrl    #1,d2           | Position mantissa
        bset    #31,d2          | Set implicit bit
        roll    #8,d3           | Position lo long word of mantissa
        roll    #3,d3
        eorw    d3,d2           | Clever use of EOR to move bits
        andiw   #0xF800,d3      | Trim off bits moved to d3
        eorw    d3,d2           | Remove noise in d2
|
GETD11:
        jmp     a1@             | Return to caller, its ret addr in a0
/*
|
|
|       page
|
|  DOPRSL
|  ======
|  Double precision floating point result (main entry w/ round).
|  Mantissa in D2:D3, exponent in D0, sign in A2, and return address
|  in A0.  Place a formatted value on the stack.
|
*/

DOPRSL:
        addl    #0x400,d3       | Round the value
        jcc     DOPR01          | J/ no carry out
        addql   #1,d2
        jcc     DOPR01          | J/ no carry out
|
        roxrl   #1,d2           | Adjust mantissa and exponent
        roxrl   #1,d3
        addqw   #1,d0
|
DOPR01:
        andw    d0,d0
        jle     DUNFRS          | J/ underflow
|
        cmpiw   #0x7FF,d0       | Check for overflow
        jge     DINFRS          | J/ overflow
|
        andiw   #0xF800,d3      | Trim mantissa
        eorw    d2,d3           | EOR to move over 11 bits
        andiw   #0xF800,d2      | Remove bits moved
        eorw    d2,d3           | Remove noise in d3
        subw    d1,d1           | Clear carry
        roxll   #1,d2           | Implicit bit into carry
        addw    d0,d2           | Exponent into d2
        rorl    #8,d2           | Reposition high word
        rorl    #3,d2
        movew   a2,d0           | Sign bit into carry
        aslw    #1,d0
        roxrl   #1,d2
        rorl    #8,d3
        rorl    #3,d3
        movel   d3,sp@- | Place value on stack
        movel   d2,sp@-
|
        moveb   #0,FPERR        | No floating point error
        jmp     a0@             | Return
|
|
|
DNANRS:
        moveq   #-1,d0          | Set d0 to 0xFFFFFFFF
        movel   d0,sp@- | NaN value
        movel   d0,sp@-
|
        moveb   #ERNAN,FPERR
        moveb   #-1,NANFLG
        jmp     a0@
|
|
DINFRS:
        movel   a2,d0           | Get sign information
        movel   #0xFFE00000,d1
        aslw    #1,d0           | Sign bit into carry
        roxrl   #1,d1
        clrl    sp@-            | Low long word = 00000000
        movel   d1,sp@-
|
        moveb   #EROVF,FPERR
        moveb   #-1,INFFLG
        jmp     a0@
|
|
DUNFRS:
        moveb   #ERUNF,FPERR
        moveb   #-1,UNFFLG
        jra     DZER01
|
|
DZERRS:
        moveb   #0,FPERR
|
|
DZER01:
        clrl    sp@-
        clrl    sp@-
        jmp     a0@
|
|       end
