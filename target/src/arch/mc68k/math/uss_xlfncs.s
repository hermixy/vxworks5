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

|       ttl     FPAC 68K/XLFNCS: IEEE Precision Translation Functions
|XLFNCS idnt    1,0             ; IEEE Precision Translation Functions
|                               ; XLFNCS.A68
|
|* * * * * * * * * *
|
|       68000 DPAC  --  Precision Translations Functions
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

NOMANUAL
*/

#define	_ASMLANGUAGE

#include "uss_fp.h"

|
|       opt     BRS             ; Default to forward branches SHORT
|
|
        .globl  SINGLE,DOUBLE
|
|
|       .set    FBIAS,127
|       .set    DBIAS,1023
|
|       xref    GETDP1,DOPRSL,DZERRS,DINFRS,DNANRS
|       xref    GETFP1,FOPRSL,FZERRS,FINFRS,FNANRS
|
|       page
|
        .text
/*
|
|  SINGLE
|  ======
|  Convert the double precision value on the stack to single precision
|
*/

SINGLE:
        jsr     GETDP1          | Fetch the operand
|
        andw    d0,d0
	jne	SNG001
        jmp     FZERRS          | J/ 0.0 parameter/result
|
SNG001:
        cmpiw   #0x7FF,d0
        jeq     SNG010          | J/ operand is NaN/INF
|
        addiw   #FBIAS-DBIAS,d0 | Change exponent bias
        jmp     FOPRSL          | Standard single prec. result routine
|
SNG010:
        lsll    #1,d2           | Remove implicit bit
        jmp     FNANRS          | J/ NaN converts to NaN
        jmp     FINFRS          |    INF converts to INF
/*
|
|
|
|  DOUBLE
|  ======
|  Extend the single precision argument to double precision
|
*/

DOUBLE:
        jsr     GETFP1          | Fetch the operand
|
        andw    d0,d0
	jne	DBL001
        jmp     DZERRS          | J/ 0.0 converts to 0.0
|
DBL001:
        cmpiw   #0xFF,d0
        jeq     DBL010          | J/ operand is NaN/INF
|
        addiw   #DBIAS-FBIAS,d0 | Change exponent bias
        clrl    d3              | Extend precision of mantissa
        jmp     DOPRSL          | Standard double prec. result routine
|
DBL010:
        lsll    #1,d2
        jmp     DNANRS          | J/ NaN converts to NaN
        jmp     DINFRS          |    INF converts to INF
|
|
|       end
