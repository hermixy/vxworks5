/* l_ssin.s - Motorola 68040 FP sine/cosine routines (LIB) */

/* Copyright 1991-1993 Wind River Systems, Inc. */
	.data
	.globl	_copyright_wind_river
	.long	_copyright_wind_river

/*
modification history
--------------------
01f,21jul93,kdl  added .text (SPR #2372).
01e,23aug92,jcf  changed bxxx to jxx.
01d,26may92,rrr  the tree shuffle
01c,10jan92,kdl  general cleanup.
01b,17dec91,kdl	 put in changes from Motorola v3.3 (from FPSP 2.1):
		 reduce argument by one step before general reduction
		 loop.
01a,15aug91,kdl  original version, from Motorola FPSP v2.0.
*/

/*
DESCRIPTION

	ssinsa 3.2 12/18/90

	The entry point sSIN computes the sine of an input argument
	sCOS computes the cosine, and sSINCOS computes both. The
	corresponding entry points with a "d" computes the same
	corresponding function values for denormalized inputs.

	Input: Double-extended number X in location pointed to
		by address register a0.

	Output: The funtion value sin(X) or cos(X) returned in Fp0 if SIN or
		COS is requested. Otherwise, for SINCOS, sin(X) is returned
		in Fp0, and cos(X) is returned in Fp1.

	Modifies: Fp0 for SIN or COS|  both Fp0 and Fp1 for SINCOS.

	Accuracy and Monotonicity: The returned result is within 1 ulp in
		64 significant bit, i.e. within 0.5001 ulp to 53 bits if the
		result is subsequently rounded to double precision. The
		result is provably monotonic in double precision.

	Speed: The programs sSIN and sCOS take approximately 150 cycles for
		input argument X such that |X| < 15Pi, which is the the usual
		situation. The speed for sSINCOS is approximately 190 cycles.

	Algorithm:

	SIN and COS:
	1. If SIN is invoked, set AdjN := 0|  otherwise, set AdjN := 1.

	2. If |X| >= 15Pi or |X| < 2**(-40), go to 7.

	3. Decompose X as X = N(Pi/2) + r where |r| <= Pi/4. Let
		k = N mod 4, so in particular, k = 0,1,2,or 3. Overwirte
		k by k := k + AdjN.

	4. If k is even, go to 6.

	5. (k is odd) Set j := (k-1)/2, sgn := (-1)**j. Return sgn*cos(r)
		where cos(r) is approximated by an even polynomial in r,
		1 + r*r*(B1+s*(B2+ |... + s*B8)),	s = r*r.
		Exit.

	6. (k is even) Set j := k/2, sgn := (-1)**j. Return sgn*sin(r)
		where sin(r) is approximated by an odd polynomial in r
		r + r*s*(A1+s*(A2+ |... + s*A7)),	s = r*r.
		Exit.

	7. If |X| > 1, go to 9.

	8. (|X|<2**(-40)) If SIN is invoked, return X|  otherwise return 1.

	9. Overwrite X by X := X rem 2Pi. Now that |X| <= Pi, go back to 3.

	SINCOS:
	1. If |X| >= 15Pi or |X| < 2**(-40), go to 6.

	2. Decompose X as X = N(Pi/2) + r where |r| <= Pi/4. Let
		k = N mod 4, so in particular, k = 0,1,2,or 3.

	3. If k is even, go to 5.

	4. (k is odd) Set j1 := (k-1)/2, j2 := j1 (EOR) (k mod 2), i.e.
		j1 exclusive or with the ls.b. of k.
		sgn1 := (-1)**j1, sgn2 := (-1)**j2.
		SIN(X) = sgn1 * cos(r) and COS(X) = sgn2*sin(r) where
		sin(r) and cos(r) are computed as odd and even polynomials
		in r, respectively. Exit

	5. (k is even) Set j1 := k/2, sgn1 := (-1)**j1.
		SIN(X) = sgn1 * sin(r) and COS(X) = sgn1*cos(r) where
		sin(r) and cos(r) are computed as odd and even polynomials
		in r, respectively. Exit

	6. If |X| > 1, go to 8.

	7. (|X|<2**(-40)) SIN(X) = X and COS(X) = 1. Exit.

	8. Overwrite X by X := X rem 2Pi. Now that |X| <= Pi, go back to 2.


		Copyright (C) Motorola, Inc. 1990
			All Rights Reserved

	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA
	The copyright notice above does not evidence any
	actual or intended publication of such source code.

SSIN	idnt	2,1 Motorola 040 Floating Point Software Package

	section	8

NOMANUAL
*/

#include "fpsp040L.h"

BOUNDS1:	.long 0x3FD78000,0x4004BC7E
TWOBYPI:	.long 0x3FE45F30,0x6DC9C883

SINA7:	.long 0xBD6AAA77,0xCCC994F5
SINA6:	.long 0x3DE61209,0x7AAE8DA1

SINA5:	.long 0xBE5AE645,0x2A118AE4
SINA4:	.long 0x3EC71DE3,0xA5341531

SINA3:	.long 0xBF2A01A0,0x1A018B59,0x00000000,0x00000000

SINA2:	.long 0x3FF80000,0x88888888,0x888859AF,0x00000000

SINA1:	.long 0xBFFC0000,0xAAAAAAAA,0xAAAAAA99,0x00000000

COSB8:	.long 0x3D2AC4D0,0xD6011EE3
COSB7:	.long 0xBDA9396F,0x9F45AC19

COSB6:	.long 0x3E21EED9,0x0612C972
COSB5:	.long 0xBE927E4F,0xB79D9FCF

COSB4:	.long 0x3EFA01A0,0x1A01D423,0x00000000,0x00000000

COSB3:	.long 0xBFF50000,0xB60B60B6,0x0B61D438,0x00000000

COSB2:	.long 0x3FFA0000,0xAAAAAAAA,0xAAAAAB5E
COSB1:	.long 0xBF000000

INVTWOPI: .long 0x3FFC0000,0xA2F9836E,0x4E44152A

TWOPI1:	.long 0x40010000,0xC90FDAA2,0x00000000,0x00000000
TWOPI2:	.long 0x3FDF0000,0x85A308D4,0x00000000,0x00000000

|	xref	__l_PITBL

#define	INARG		FP_SCR4

#define	X		FP_SCR5
#define	XDCARE		X+2
#define	XFRAC		X+4

#define	RPRIME		FP_SCR1
#define	SPRIME		FP_SCR2

#define	POSNEG1		L_SCR1
#define	TWOTO63		L_SCR1

#define	ENDFLAG		L_SCR2
#define	N		L_SCR2

#define	ADJN		L_SCR3

|	xref	__l_t_frcinx
|	xref	__l_t_extdnrm
|	xref	__l_sto_cos


	.text

	.globl	__l_ssind
__l_ssind:
|--SIN(X) = X FOR DENORMALIZED X
	jra 		__l_t_extdnrm

	.globl	__l_scosd
__l_scosd:
|--COS(X) = 1 FOR DENORMALIZED X

	.long 0xf23c4400,0x3f800000 /*	fmoves	&0x3F800000,fp0 */
|
|	9D25B Fix: Sometimes the previous fmoves sets fpsr bits
|
	fmovel		#0,fpsr
|
	jra 		__l_t_frcinx

	.globl	__l_ssin
__l_ssin:
|--SET ADJN TO 0
	movel		#0,a6@(ADJN)
	jra 		SINBGN

	.globl	__l_scos
__l_scos:
|--SET ADJN TO 1
	movel		#1,a6@(ADJN)

SINBGN:
|--SAVE fpcr, FP1. CHECK IF |X| IS TOO SMALL OR LARGE

	fmovex		a0@,fp0	|...lOAD INPUT

	movel		A0@,d0
	movew		A0@(4),d0
	fmovex		fp0,a6@(X)
	andil		#0x7FFFFFFF,d0		|...COMPACTIFY X

	cmpil		#0x3FD78000,d0		|...|X| >= 2**(-40)?
	jge 		SOK1
	jra 		SINSM

SOK1:
	cmpil		#0x4004BC7E,d0		|...|X| < 15 PI?
	jlt 		SINMAIN
	jra 		REDUCEX

SINMAIN:
|--THIS IS THE USUAL CASE, |X| <= 15 PI.
|--THE ARGUMENT REDUCTION IS DONE BY TABLE LOOK UP.
	fmovex		fp0,fp1
	fmuld		TWOBYPI,fp1	|...X*2/PI

|--HIDE THE NEXT THREE INSTRUCTIONS
	lea		__l_PITBL+0x200,a1 |...TABLE OF N*PI/2, N = -32,...,32


|--FP1 IS NOW READY
	fmovel		fp1,a6@(N)		|...CONVERT TO INTEGER

	movel		a6@(N),d0
	asll		#4,d0
	addal		d0,a1	|...A1 IS THE ADDRESS OF N*PIBY2
|				|...wHICH IS IN TWO PIECES Y1 # Y2

	fsubx		A1@+,fp0	|...X-Y1
|--HIDE THE NEXT ONE
	fsubs		A1@,fp0	|...FP0 IS R = (X-Y1)-Y2

SINCONT:
|--continuation from REDUCEX

|--GET N+ADJN AND SEE IF SIN(R) OR COS(R) IS NEEDED
	movel		a6@(N),d0
	addl		a6@(ADJN),d0	|...SEE IF d0 IS ODD OR EVEN
	rorl		#1,d0	|...D0 WAS ODD IFF d0 IS NEGATIVE
	cmpil		#0,d0
	jlt 		COSPOLY

SINPOLY:
|--LET J BE THE LEAST SIG. BIT OF D0, LET SGN := (-1)**J.
|--THEN WE RETURN	SGN*SIN(R). SGN*SIN(R) IS COMPUTED BY
/* |--R' + R'*S*(A1 + S(A2 + S(A3 + S(A4 + |... + SA7)))), WHERE */
/* |--R' = SGN*R, S=R*R. THIS CAN BE REWRITTEN AS */
/* |--R' + R'*S*( [A1+T(A3+T(A5+TA7))] + [S(A2+T(A4+TA6))]) */
|--WHERE T=S*S.
|--NOTE THAT A3 THROUGH A7 ARE STORED IN DOUBLE PRECISION
|--WHILE A1 AND A2 ARE IN DOUBLE-EXTENDED FORMAT.
	fmovex		fp0,a6@(X)	|...X IS R
	fmulx		fp0,fp0	|...FP0 IS S
|---HIDE THE NEXT TWO WHILE WAITING FOR FP0
	fmoved		SINA7,fp3
	fmoved		SINA6,fp2
|--FP0 IS NOW READY
	fmovex		fp0,fp1
	fmulx		fp1,fp1	|...FP1 IS T
|--HIDE THE NEXT TWO WHILE WAITING FOR FP1

	rorl		#1,d0
	andil		#0x80000000,d0
|				|...lEAST SIG. BIT OF D0 IN SIGN POSITION
	eorl		d0,a6@(X)	/* |...X IS NOW R'= SGN*R */

	fmulx		fp1,fp3	|...TA7
	fmulx		fp1,fp2	|...TA6

	faddd		SINA5,fp3 |...A5+TA7
	faddd		SINA4,fp2 |...A4+TA6

	fmulx		fp1,fp3	|...T(A5+TA7)
	fmulx		fp1,fp2	|...T(A4+TA6)

	faddd		SINA3,fp3 |...A3+T(A5+TA7)
	faddx		SINA2,fp2 |...A2+T(A4+TA6)

	fmulx		fp3,fp1	|...T(A3+T(A5+TA7))

	fmulx		fp0,fp2	|...S(A2+T(A4+TA6))
	faddx		SINA1,fp1 |...A1+T(A3+T(A5+TA7))
	fmulx		a6@(X),fp0	/* |...R'*S */

	faddx		fp2,fp1	|...[A1+T(A3+T(A5+TA7))]+[S(A2+T(A4+TA6))]
|--FP3 RELEASED, RESTORE NOW AND TAKE SOME ADVANTAGE OF HIDING
|--FP2 RELEASED, RESTORE NOW AND TAKE FULL ADVANTAGE OF HIDING


	fmulx		fp1,fp0		/* |...SIN(R')-R' */
|--FP1 RELEASED.

	fmovel		d1,fpcr		| restore users exceptions
	faddx		a6@(X),fp0		| last inst - possible exception set
	jra 		__l_t_frcinx


COSPOLY:
|--LET J BE THE LEAST SIG. BIT OF D0, LET SGN := (-1)**J.
|--THEN WE RETURN	SGN*COS(R). SGN*COS(R) IS COMPUTED BY
/* |--SGN + S'*(B1 + S(B2 + S(B3 + S(B4 + |... + SB8)))), WHERE */
/* |--S=R*R AND S'=SGN*S. THIS CAN BE REWRITTEN AS */
/* |--SGN + S'*([B1+T(B3+T(B5+TB7))] + [S(B2+T(B4+T(B6+TB8)))]) */
|--WHERE T=S*S.
|--NOTE THAT B4 THROUGH B8 ARE STORED IN DOUBLE PRECISION
|--WHILE B2 AND B3 ARE IN DOUBLE-EXTENDED FORMAT, B1 IS -1/2
|--AND IS THEREFORE STORED AS SINGLE PRECISION.

	fmulx		fp0,fp0	|...FP0 IS S
|---HIDE THE NEXT TWO WHILE WAITING FOR FP0
	fmoved		COSB8,fp2
	fmoved		COSB7,fp3
|--FP0 IS NOW READY
	fmovex		fp0,fp1
	fmulx		fp1,fp1	|...FP1 IS T
|--HIDE THE NEXT TWO WHILE WAITING FOR FP1
	fmovex		fp0,a6@(X)	|...X IS S
	rorl		#1,d0
	andil		#0x80000000,d0
|			|...lEAST SIG. BIT OF D0 IN SIGN POSITION

	fmulx		fp1,fp2	|...TB8
|--HIDE THE NEXT TWO WHILE WAITING FOR THE XU
	eorl		d0,a6@(X)	/* |...X IS NOW S'= SGN*S */
	andil		#0x80000000,d0

	fmulx		fp1,fp3	|...TB7
|--HIDE THE NEXT TWO WHILE WAITING FOR THE XU
	oril		#0x3F800000,d0	|...D0 IS SGN IN SINGLE
	movel		d0,a6@(POSNEG1)

	faddd		COSB6,fp2 |...B6+TB8
	faddd		COSB5,fp3 |...B5+TB7

	fmulx		fp1,fp2	|...T(B6+TB8)
	fmulx		fp1,fp3	|...T(B5+TB7)

	faddd		COSB4,fp2 |...B4+T(B6+TB8)
	faddx		COSB3,fp3 |...B3+T(B5+TB7)

	fmulx		fp1,fp2	|...T(B4+T(B6+TB8))
	fmulx		fp3,fp1	|...T(B3+T(B5+TB7))

	faddx		COSB2,fp2 |...B2+T(B4+T(B6+TB8))
	fadds		COSB1,fp1 |...B1+T(B3+T(B5+TB7))

	fmulx		fp2,fp0	|...S(B2+T(B4+T(B6+TB8)))
|--FP3 RELEASED, RESTORE NOW AND TAKE SOME ADVANTAGE OF HIDING
|--FP2 RELEASED.


	faddx		fp1,fp0
|--FP1 RELEASED

	fmulx		a6@(X),fp0

	fmovel		d1,fpcr		| restore users exceptions
	fadds		a6@(POSNEG1),fp0	| last inst - possible exception set
	jra 		__l_t_frcinx


SINBORS:
|--IF |X| > 15PI, WE USE THE GENERAL ARGUMENT REDUCTION.
|--IF |X| < 2**(-40), RETURN X OR 1.
	cmpil		#0x3FFF8000,d0
	jgt 		REDUCEX


SINSM:
	movel		a6@(ADJN),d0
	cmpil		#0,d0
	jgt 		COSTINY

SINTINY:
	movew		#0x0000,a6@(XDCARE)	|...JUST IN CASE
	fmovel		d1,fpcr			| restore users exceptions
	fmovex		a6@(X),fp0		| last inst - possible exception set
	jra 		__l_t_frcinx


COSTINY:
	.long 0xf23c4400,0x3f800000	 /*	fmoves	&0x3F800000,fp0 */


	fmovel		d1,fpcr		| restore users exceptions
	.long 0xf23c4428,0x00800000	 /*	fsubs	&0x00800000,fp0 */
	jra 		__l_t_frcinx


REDUCEX:
|--WHEN REDUCEX IS USED, THE CODE WILL INEVITABLY BE SLOW.
|--THIS REDUCTION METHOD, HOWEVER, IS MUCH FASTER THAN USING
|--THE REMAINDER INSTRUCTION WHICH IS NOW IN SOFTWARE.

	fmovemx	fp2-fp5,A7@-	|...save fp2 through fp5
	movel		d2,A7@-
	.long 0xf23c4480,0x00000000	/*	fmoves	&0x00000000,fp1 */

|--If compact form of abs(arg) in d0=0x7ffeffff, argument is so large that
|--there is a danger of unwanted overflow in first LOOP iteration.  In this
|--case, reduce argument by one remainder step to make subsequent reduction
|--safe.
	cmpil	#0x7ffeffff,d0			| is argument dangerously large?
	jne 	LOOP
	movel	#0x7ffe0000,a6@(FP_SCR2)	| yes
|						| create 2**16383*PI/2
	movel	#0xc90fdaa2,a6@(FP_SCR2+4)
	clrl	a6@(FP_SCR2+8)
	ftstx	fp0				| test sign of argument
	movel	#0x7fdc0000,a6@(FP_SCR3)	| create low half of 2**16383*
|						| PI/2 at FP_SCR3
	movel	#0x85a308d3,a6@(FP_SCR3+4)
	clrl  	a6@(FP_SCR3+8)
	fblt	red_neg
	orw	#0x8000,a6@(FP_SCR2)		| positive arg
	orw	#0x8000,a6@(FP_SCR3)
red_neg:
	faddx 	a6@(FP_SCR2),fp0	| high part of reduction is exact
	fmovex  fp0,fp1			| save high result in fp1
	faddx 	a6@(FP_SCR3),fp0	| low part of reduction
	fsubx   fp0,fp1			| determine low component of result
	faddx 	a6@(FP_SCR3),fp1	| fp0/fp1 are reduced argument.

|--ON ENTRY, FP0 IS X, ON RETURN, FP0 IS X REM PI/2, |X| <= PI/4.
|--integer quotient will be stored in N
|--Intermeditate remainder is 66-bit long|  (R,r) in (FP0,FP1)

LOOP:
	fmovex		fp0,a6@(INARG)	|...+-2**K * F, 1 <= F < 2
	movew		a6@(INARG),d0
        movel          d0,a1		|...save a copy of d0
	andil		#0x00007FFF,d0
	subil		#0x00003FFF,d0	|...D0 IS K
	cmpil		#28,d0
	jle 		LASTLOOP
CONTLOOP:
	subil		#27,d0	 |...D0 IS L := K-27
	movel		#0,a6@(ENDFLAG)
	jra 		WORK
LASTLOOP:
	clrl		d0		|...D0 IS L := 0
	movel		#1,a6@(ENDFLAG)

WORK:
|--FIND THE REMAINDER OF (R,r) W.R.T.	2**L * (PI/2). L IS SO CHOSEN
|--THAT	INT( X * (2/PI) / 2**(L) ) < 2**29.

|--CREATE 2**(-L) * (2/PI), SIGN(INARG)*2**(63),
|--2**L * (PIby2_1), 2**L * (PIby2_2)

	movel		#0x00003FFE,d2	|...BIASED EXPO OF 2/PI
	subl		d0,d2		|...BIASED EXPO OF 2**(-L)*(2/PI)

	movel		#0xA2F9836E,a6@(FP_SCR1+4)
	movel		#0x4E44152A,a6@(FP_SCR1+8)
	movew		d2,a6@(FP_SCR1)	|...FP_SCR1 is 2**(-L)*(2/PI)

	fmovex		fp0,fp2
	fmulx		a6@(FP_SCR1),fp2
|--WE MUST NOW FIND INT(FP2). SINCE WE NEED THIS VALUE IN
/* |--FLOATING POINT FORMAT, THE TWO FMOVE'S	FMOVE.l FP <--> N */
|--WILL BE TOO INEFFICIENT. THE WAY AROUND IT IS THAT
|--(SIGN(INARG)*2**63	+	FP2) - SIGN(INARG)*2**63 WILL GIVE
|--US THE DESIRED VALUE IN FLOATING POINT.

|--HIDE SIX CYCLES OF INSTRUCTION
        movel		a1,d2
        swap		d2
	andil		#0x80000000,d2
	oril		#0x5F000000,d2	|...D2 IS SIGN(INARG)*2**63 IN SGL
	movel		d2,a6@(TWOTO63)

	movel		d0,d2
	addil		#0x00003FFF,d2	|...BIASED EXPO OF 2**L * (PI/2)

|--FP2 IS READY
	fadds		a6@(TWOTO63),fp2	|...THE FRACTIONAL PART OF fp1 IS ROUNDED

|--HIDE 4 CYCLES OF INSTRUCTION|  creating 2**(L)*Piby2_1  and  2**(L)*Piby2_2
        movew		d2,a6@(FP_SCR2)
	clrw          	a6@(FP_SCR2+2)
	movel		#0xC90FDAA2,a6@(FP_SCR2+4)
	clrl		a6@(FP_SCR2+8)		|...FP_SCR2 is  2**(L) * Piby2_1

|--FP2 IS READY
	fsubs		a6@(TWOTO63),fp2		|...FP2 is N

	addil		#0x00003FDD,d0
        movew		d0,a6@(FP_SCR3)
	clrw          	a6@(FP_SCR3+2)
	movel		#0x85A308D3,a6@(FP_SCR3+4)
	clrl		a6@(FP_SCR3+8)		|...FP_SCR3 is 2**(L) * Piby2_2

	movel		a6@(ENDFLAG),d0

|--We are now ready to perform (R+r) - N*P1 - N*P2, P1 = 2**(L) * Piby2_1 and
|--P2 = 2**(L) * Piby2_2
	fmovex		fp2,fp4
	fmulx		a6@(FP_SCR2),fp4		|...w = N*P1
	fmovex		fp2,fp5
	fmulx		a6@(FP_SCR3),fp5		|...w = N*P2
	fmovex		fp4,fp3
|--we want P+p = W+w  but  |p| <= half ulp of P
|--Then, we need to compute  A := R-P   and  a := r-p
	faddx		fp5,fp3			|...FP3 is P
	fsubx		fp3,fp4			|...w-P

	fsubx		fp3,fp0			|...FP0 is A := R - P
        faddx		fp5,fp4			|...FP4 is p = (W-P)+w

	fmovex		fp0,fp3			|...FP3 A
	fsubx		fp4,fp1			|...FP1 is a := r - p

|--Now we need to normalize (A,a) to  "new (R,r)" where R+r = A+a but
|--|r| <= half ulp of R.
	faddx		fp1,fp0			|...FP0 is R := A+a
|--No need to calculate r if this is the last loop
	cmpil		#0,d0
	jgt 		RESTORE

|--Need to calculate r
	fsubx		fp0,fp3			|...A-R
	faddx		fp3,fp1			|...FP1 is r := (A-R)+a
	jra 		LOOP

RESTORE:
        fmovel		fp2,a6@(N)
	movel		A7@+,d2
	fmovemx	A7@+,fp2-fp5


	movel		a6@(ADJN),d0
	cmpil		#4,d0

	jlt 		SINCONT
	jra 		SCCONT

	.globl	__l_ssincosd
__l_ssincosd:
|--SIN AND COS OF X FOR DENORMALIZED X

	.long 0xf23c4480,0x3f800000	/*	fmoves	&0x3F800000,fp1 */
	bsrl		__l_sto_cos		| store cosine result
	jra 		__l_t_extdnrm

	.globl	__l_ssincos
__l_ssincos:
|--SET ADJN TO 4
	movel		#4,a6@(ADJN)

	fmovex		a0@,fp0			|...lOAD INPUT

	movel		A0@,d0
	movew		A0@(4),d0
	fmovex		fp0,a6@(X)
	andil		#0x7FFFFFFF,d0		|...COMPACTIFY X

	cmpil		#0x3FD78000,d0		|...|X| >= 2**(-40)?
	jge 		SCOK1
	jra 		SCSM

SCOK1:
	cmpil		#0x4004BC7E,d0		|...|X| < 15 PI?
	jlt 		SCMAIN
	jra 		REDUCEX


SCMAIN:
|--THIS IS THE USUAL CASE, |X| <= 15 PI.
|--THE ARGUMENT REDUCTION IS DONE BY TABLE LOOK UP.
	fmovex		fp0,fp1
	fmuld		TWOBYPI,fp1	|...X*2/PI

|--HIDE THE NEXT THREE INSTRUCTIONS
	lea		__l_PITBL+0x200,a1 |...TABLE OF N*PI/2, N = -32,...,32


|--FP1 IS NOW READY
	fmovel		fp1,a6@(N)	|...CONVERT TO INTEGER

	movel		a6@(N),d0
	asll		#4,d0
	addal		d0,a1		|...ADDRESS OF N*PIBY2, IN Y1, Y2

	fsubx		A1@+,fp0	|...X-Y1
        fsubs		A1@,fp0		|...FP0 IS R = (X-Y1)-Y2

SCCONT:
|--continuation point from REDUCEX

|--HIDE THE NEXT TWO
	movel		a6@(N),d0
	rorl		#1,d0

	cmpil		#0,d0		|...D0 < 0 IFF N IS ODD
	jge 		NEVEN

NODD:
|--REGISTERS SAVED SO FAR: D0, A0, FP2.

	fmovex		fp0,a6@(RPRIME)
	fmulx		fp0,fp0	 	|...FP0 IS S = R*R
	fmoved		SINA7,fp1	|...A7
	fmoved		COSB8,fp2	|...B8
	fmulx		fp0,fp1	 	|...SA7
	movel		d2,A7@-
	movel		d0,d2
	fmulx		fp0,fp2	 	|...SB8
	rorl		#1,d2
	andil		#0x80000000,d2

	faddd		SINA6,fp1	|...A6+SA7
	eorl		d0,d2
	andil		#0x80000000,d2
	faddd		COSB7,fp2	|...B7+SB8

	fmulx		fp0,fp1	 	|...S(A6+SA7)
	eorl		d2,a6@(RPRIME)
	movel		A7@+,d2
	fmulx		fp0,fp2	 	|...S(B7+SB8)
	rorl		#1,d0
	andil		#0x80000000,d0

	faddd		SINA5,fp1	|...A5+S(A6+SA7)
	movel		#0x3F800000,a6@(POSNEG1)
	eorl		d0,a6@(POSNEG1)
	faddd		COSB6,fp2	|...B6+S(B7+SB8)

	fmulx		fp0,fp1	 	|...S(A5+S(A6+SA7))
	fmulx		fp0,fp2	 	|...S(B6+S(B7+SB8))
	fmovex		fp0,a6@(SPRIME)

	faddd		SINA4,fp1	|...A4+S(A5+S(A6+SA7))
	eorl		d0,a6@(SPRIME)
	faddd		COSB5,fp2	|...B5+S(B6+S(B7+SB8))

	fmulx		fp0,fp1	 	|...S(A4+...)
	fmulx		fp0,fp2	 	|...S(B5+...)

	faddd		SINA3,fp1	|...A3+S(A4+...)
	faddd		COSB4,fp2	|...B4+S(B5+...)

	fmulx		fp0,fp1	 	|...S(A3+...)
	fmulx		fp0,fp2	 	|...S(B4+...)

	faddx		SINA2,fp1	|...A2+S(A3+...)
	faddx		COSB3,fp2	|...B3+S(B4+...)

	fmulx		fp0,fp1	 	|...S(A2+...)
	fmulx		fp0,fp2	 	|...S(B3+...)

	faddx		SINA1,fp1	|...A1+S(A2+...)
	faddx		COSB2,fp2	|...B2+S(B3+...)

	fmulx		fp0,fp1	 	|...S(A1+...)
	fmulx		fp2,fp0	 	|...S(B2+...)



	fmulx		a6@(RPRIME),fp1	/* |...R'S(A1+...) */
	fadds		COSB1,fp0	|...B1+S(B2...)
	fmulx		a6@(SPRIME),fp0	/* |...S'(B1+S(B2+...)) */

	movel		d1,a7@-		| restore users mode # precision
	andil		#0xff,d1	| mask off all exceptions
	fmovel		d1,fpcr
	faddx		a6@(RPRIME),fp1	|...COS(X)
	bsrl		__l_sto_cos	| store cosine result
	fmovel		a7@+,fpcr	| restore users exceptions
	fadds		a6@(POSNEG1),fp0	|...SIN(X)

	jra 		__l_t_frcinx


NEVEN:
|--REGISTERS SAVED SO FAR: FP2.

	fmovex		fp0,a6@(RPRIME)
	fmulx		fp0,fp0	 		|...FP0 IS S = R*R
	fmoved		COSB8,fp1		|...B8
	fmoved		SINA7,fp2		|...A7
	fmulx		fp0,fp1	 		|...SB8
	fmovex		fp0,a6@(SPRIME)
	fmulx		fp0,fp2	 		|...SA7
	rorl		#1,d0
	andil		#0x80000000,d0
	faddd		COSB7,fp1		|...B7+SB8
	faddd		SINA6,fp2		|...A6+SA7
	eorl		d0,a6@(RPRIME)
	eorl		d0,a6@(SPRIME)
	fmulx		fp0,fp1	 		|...S(B7+SB8)
	oril		#0x3F800000,d0
	movel		d0,a6@(POSNEG1)
	fmulx		fp0,fp2	 		|...S(A6+SA7)

	faddd		COSB6,fp1		|...B6+S(B7+SB8)
	faddd		SINA5,fp2		|...A5+S(A6+SA7)

	fmulx		fp0,fp1	 		|...S(B6+S(B7+SB8))
	fmulx		fp0,fp2	 		|...S(A5+S(A6+SA7))

	faddd		COSB5,fp1		|...B5+S(B6+S(B7+SB8))
	faddd		SINA4,fp2		|...A4+S(A5+S(A6+SA7))

	fmulx		fp0,fp1	 		|...S(B5+...)
	fmulx		fp0,fp2	 		|...S(A4+...)

	faddd		COSB4,fp1		|...B4+S(B5+...)
	faddd		SINA3,fp2		|...A3+S(A4+...)

	fmulx		fp0,fp1	 		|...S(B4+...)
	fmulx		fp0,fp2	 		|...S(A3+...)

	faddx		COSB3,fp1		|...B3+S(B4+...)
	faddx		SINA2,fp2		|...A2+S(A3+...)

	fmulx		fp0,fp1	 		|...S(B3+...)
	fmulx		fp0,fp2	 		|...S(A2+...)

	faddx		COSB2,fp1		|...B2+S(B3+...)
	faddx		SINA1,fp2		|...A1+S(A2+...)

	fmulx		fp0,fp1	 		|...S(B2+...)
	fmulx		fp2,fp0	 		|...s(a1+...)



	fadds		COSB1,fp1		|...B1+S(B2...)
	fmulx		a6@(RPRIME),fp0	/* 	|...R'S(A1+...) */
	fmulx		a6@(SPRIME),fp1	/* 	|...S'(B1+S(B2+...)) */

	movel		d1,a7@-			| save users mode # precision
	andil		#0xff,d1		| mask off all exceptions
	fmovel		d1,fpcr
	fadds		a6@(POSNEG1),fp1	|...COS(X)
	bsrl		__l_sto_cos		| store cosine result
	fmovel		a7@+,fpcr		| restore users exceptions
	faddx		a6@(RPRIME),fp0		|...SIN(X)

	jra 		__l_t_frcinx

SCBORS:
	cmpil		#0x3FFF8000,d0
	jgt 		REDUCEX


SCSM:
	movew		#0x0000,a6@(XDCARE)
	.long 0xf23c4480,0x3f800000	/*	fmoves	&0x3F800000,fp1 */

	movel		d1,a7@-			| save users mode # precision
	andil		#0xff,d1		| mask off all exceptions
	fmovel		d1,fpcr
	.long 0xf23c44a8,0x00800000	/*	fsubs	&0x00800000,fp1 */
	bsrl		__l_sto_cos		| store cosine result
	fmovel		a7@+,fpcr		| restore users exceptions
	fmovex		a6@(X),fp0
	jra 		__l_t_frcinx

|	end
