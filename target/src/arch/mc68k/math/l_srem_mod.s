/* l_srem_mods.s - Motorola 68040 FP modulo/remainder routines (LIB) */

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
01c,10jan92,kdl  added modification history; general cleanup.
01b,01jan92,jcf	 reversed order of cmp <reg>,<reg>
01a,15aug91,kdl  original version, from Motorola FPSP v2.0.
*/

/*
DESCRIPTION

	srem_modsa 3.1 12/10/90

 WIND RIVER MODIFICATION HISTORY

      The entry point sMOD computes the floating point MOD of the
      input values X and Y. The entry point sREM computes the floating
      point (IEEE) REM of the input values X and Y.

      INPUT
      -----
      Double-extended value Y is pointed to by address in register
      A0. Double-extended value X is located in	A0@(-12). The values
      of X and Y are both nonzero and finite|  although either or both
      of them can be denormalized. The special cases of zeros, NaNs,
      and infinities are handled elsewhere.

      OUTPUT
      ------
      FREM(X,Y) or FMOD(X,Y), depending on entry point.

       ALGORITHM
       ---------

       Step 1.  Save and strip signs of X and Y: signX := sign(X),
                signY := sign(Y), X := |X|, Y := |Y|,
                signQ := signX EOR signY. Record whether MOD or REM
                is requested.

       Step 2.  Set L := expo(X)-expo(Y), k := 0, Q := 0.
                If (L < 0) then
                   R := X, go to Step 4.
                else
                   R := 2^(-L)X, j := L.
                endif

       Step 3.  Perform MOD(X,Y)
            3.1 If R = Y, go to Step 9.
            3.2 If R > Y, then { R := R - Y, Q := Q + 1}
            3.3 If j = 0, go to Step 4.
            3.4 k := k + 1, j := j - 1, Q := 2Q, R := 2R. Go to
                Step 3.1.

       Step 4.  At this point, R = X - QY = MOD(X,Y). Set
                Last_Subtract := false (used in Step 7 below). If
                MOD is requested, go to Step 6.

       Step 5.  R = MOD(X,Y), but REM(X,Y) is requested.
            5.1 If R < Y/2, then R = MOD(X,Y) = REM(X,Y). Go to
                Step 6.
            5.2 If R > Y/2, then { set Last_Subtract := true,
                Q := Q + 1, Y := signY*Y }. Go to Step 6.
            5.3 This is the tricky case of R = Y/2. If Q is odd,
                then { Q := Q + 1, signX := -signX }.

       Step 6.  R := signX*R.

       Step 7.  If Last_Subtract = true, R := R - Y.

       Step 8.  Return signQ, last 7 bits of Q, and R as required.

       Step 9.  At this point, R = 2^(-j)*X - Q Y = Y. Thus,
                X = 2^(j)*(Q+1)Y. set Q := 2^(j)*(Q+1),
                R := 0. Return signQ, last 7 bits of Q, and R.



		Copyright (C) Motorola, Inc. 1990
			All Rights Reserved

	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA
	The copyright notice above does not evidence any
	actual or intended publication of such source code.

SREM_MOD    idnt    2,1 Motorola 040 Floating Point Software Package

	section    8

NOMANUAL
*/

#include "fpsp040L.h"

#define	Mod_Flag  	L_SCR3
#define	SignY     	FP_SCR3+4
#define	SignX     	FP_SCR3+8
#define	SignQ     	FP_SCR3+12
#define	Sc_Flag   	FP_SCR4

#define	Y         	FP_SCR1
#define	Y_Hi      	Y+4
#define	Y_Lo      	Y+8

#define	R         	FP_SCR2
#define	R_Hi      	R+4
#define	R_Lo      	R+8


Scale:     .long	0x00010000,0x80000000,0x00000000,0x00000000

|	xref	__l_t_avoid_unsupp


	.text

        .globl        __l_smod
__l_smod:

   movel               #0,a6@(Mod_Flag)
   jra                 Mod_Rem

        .globl        __l_srem
__l_srem:

   movel               #1,a6@(Mod_Flag)

Mod_Rem:
|..Save sign of X and Y
   moveml              d2-d7,A7@-     |...save data registers
   movew              	A0@,d3
   movew               d3,a6@(SignY)
   andil               #0x00007FFF,d3   |...Y := |Y|

|
   movel              	A0@(4),d4
   movel              	A0@(8),d5        |...(D3,d4,d5) is |Y|

   tstl                d3
   jne                 Y_Normal

   movel               #0x00003FFE,d3	|...0x3FFD + 1
   tstl                d4
   jne                 HiY_not0

HiY_0:
   movel               d5,d4
   clrl                d5
   subil               #32,d3
   clrl                d6
   bfffo                d4{#0:#32},d6
   lsll                d6,d4
   subl                d6,d3           |...(D3,d4,d5) is normalized
|                                       |...with bias 0x7FFD
   jra                 Chk_X

HiY_not0:
   clrl                d6
   bfffo                d4{#0:#32},d6
   subl                d6,d3
   lsll                d6,d4
   movel               d5,d7           |...a copy of d5
   lsll                d6,d5
   negl                d6
   addil               #32,d6
   lsrl                d6,d7
   orl                 d7,d4           |...(D3,d4,d5) normalized
|                                       |...with bias 0x7FFD
   jra                 Chk_X

Y_Normal:
   addil               #0x00003FFE,d3   |...(D3,d4,d5) normalized
|                                       |...with bias 0x7FFD

Chk_X:
   movew              	A0@(-12),d0
   movew               d0,a6@(SignX)
   movew              	a6@(SignY),d1
   eorl                d0,d1
   andil               #0x00008000,d1
   movew               d1,a6@(SignQ)	|...sign(Q) obtained
   andil               #0x00007FFF,d0
   movel              	A0@(-8),d1
   movel              	A0@(-4),d2       |...(D0,d1,d2) is |X|
   tstl                d0
   jne                 X_Normal
   movel               #0x00003FFE,d0
   tstl                d1
   jne                 HiX_not0

HiX_0:
   movel               d2,d1
   clrl                d2
   subil               #32,d0
   clrl                d6
   bfffo                d1{#0:#32},d6
   lsll                d6,d1
   subl                d6,d0           |...(D0,d1,d2) is normalized
|                                       |...with bias 0x7FFD
   jra                 Init

HiX_not0:
   clrl                d6
   bfffo                d1{#0:#32},d6
   subl                d6,d0
   lsll                d6,d1
   movel               d2,d7           |...a copy of d2
   lsll                d6,d2
   negl                d6
   addil               #32,d6
   lsrl                d6,d7
   orl                 d7,d1           |...(D0,d1,d2) normalized
|                                       |...with bias 0x7FFD
   jra                 Init

X_Normal:
   addil               #0x00003FFE,d0   |...(D0,d1,d2) normalized
|                                       |...with bias 0x7FFD

Init:
|
   movel               d3,a6@(L_SCR1)   |...save biased expo(Y)
   movel		d0,a6@(L_SCR2)	| save d0
   subl                d3,d0           |...l := expo(X)-expo(Y)
|   Movel               D0,L            |...D0 is j
   clrl                d6              |...D6 := carry <- 0
   clrl                d3              |...D3 is Q
   moveal              #0,a1           |...A1 is k|  j+k=L, Q=0

|..(Carry,D1,D2) is R
   tstl                d0
   jge                 Mod_Loop

|..expo(X) < expo(Y). Thus X = mod(X,Y)
|
   movel		a6@(L_SCR2),d0	| restore d0
   jra                 Get_Mod

|..At this point  R = 2^(-L)X|  Q = 0|  k = 0|  and  k+j = L


Mod_Loop:
   tstl                d6              |...test carry bit
   jgt                 R_GT_Y

|..At this point carry = 0, R = (D1,D2), Y = (D4,D5)
   cmpl                d1,d4           |...compare hi(R) and hi(Y)
   jne                 R_NE_Y
   cmpl                d2,d5           |...compare lo(R) and lo(Y)
   jne                 R_NE_Y

|..At this point, R = Y
   jra                 Rem_is_0

R_NE_Y:
|..use the borrow of the previous compare
   jcs                 R_LT_Y          |...borrow is set iff R < Y

R_GT_Y:
|..If Carry is set, then Y < (Carry,D1,D2) < 2Y. Otherwise, Carry = 0
|..and Y < (D1,D2) < 2Y. Either way, perform R - Y
   subl                d5,d2           |...lo(R) - lo(Y)
   subxl               d4,d1           |...hi(R) - hi(Y)
   clrl                d6              |...clear carry
   addql               #1,d3           |...Q := Q + 1

R_LT_Y:
|..At this point, Carry=0, R < Y. R = 2^(k-L)X - QY|  k+j = L|  j >= 0.
   tstl                d0              |...see if j = 0.
   jeq                 PostLoop

   addl                d3,d3           |...Q := 2Q
   addl                d2,d2           |...lo(R) = 2lo(R)
   roxll               #1,d1           |...hi(R) = 2hi(R) + carry
   scs                 d6              |...set Carry if 2(R) overflows
   addql               #1,a1           |...k := k+1
   subql               #1,d0           |...j := j - 1
|..At this point, R=(Carry,D1,D2) = 2^(k-L)X - QY, j+k=L, j >= 0, R < 2Y.

   jra                 Mod_Loop

PostLoop:
|..k = L, j = 0, Carry = 0, R = (D1,D2) = X - QY, R < Y.

|..normalize R.
   movel              	a6@(L_SCR1),d0           |...new biased expo of R
   tstl                d1
   jne                 HiR_not0

HiR_0:
   movel               d2,d1
   clrl                d2
   subil               #32,d0
   clrl                d6
   bfffo                d1{#0:#32},d6
   lsll                d6,d1
   subl                d6,d0           |...(D0,d1,d2) is normalized
|                                       |...with bias 0x7FFD
   jra                 Get_Mod

HiR_not0:
   clrl                d6
   bfffo                d1{#0:#32},d6
   jmi                 Get_Mod         |...already normalized
   subl                d6,d0
   lsll                d6,d1
   movel               d2,d7           |...a copy of d2
   lsll                d6,d2
   negl                d6
   addil               #32,d6
   lsrl                d6,d7
   orl                 d7,d1           |...(D0,d1,d2) normalized

|
Get_Mod:
   cmpil		#0x000041FE,d0
   jge 		No_Scale
Do_Scale:
   movew		d0,a6@(R)
   clrw		a6@(R+2)
   movel		d1,a6@(R_Hi)
   movel		d2,a6@(R_Lo)
   movel		a6@(L_SCR1),d6
   movew		d6,a6@(Y)
   clrw		a6@(Y+2)
   movel		d4,a6@(Y_Hi)
   movel		d5,a6@(Y_Lo)
   fmovex		a6@(R),fp0		|...no exception
   movel		#1,a6@(Sc_Flag)
   jra 		ModOrRem
No_Scale:
   movel		d1,a6@(R_Hi)
   movel		d2,a6@(R_Lo)
   subil		#0x3FFE,d0
   movew		d0,a6@(R)
   clrw		a6@(R+2)
   movel		a6@(L_SCR1),d6
   subil		#0x3FFE,d6
   movel		d6,a6@(L_SCR1)
   fmovex		a6@(R),fp0
   movew		d6,a6@(Y)
   movel		d4,a6@(Y_Hi)
   movel		d5,a6@(Y_Lo)
   movel		#0,a6@(Sc_Flag)

|


ModOrRem:
   movel              	a6@(Mod_Flag),d6
   jeq                 Fix_Sign

   movel              	a6@(L_SCR1),d6           |...new biased expo(Y)
   subql               #1,d6           |...biased expo(Y/2)
   cmpl                d0,d6
   jlt                 Fix_Sign
   jgt                 Last_Sub

   cmpl                d1,d4
   jne                 Not_EQ
   cmpl                d2,d5
   jne                 Not_EQ
   jra                 Tie_Case

Not_EQ:
   jcs                 Fix_Sign

Last_Sub:
|
   fsubx		a6@(Y),fp0		|...no exceptions
   addql               #1,d3           |...Q := Q + 1

|

Fix_Sign:
|..Get sign of X
   movew              	a6@(SignX),d6
   jge 		Get_Q
   fnegx		fp0

|..Get Q
|
Get_Q:
   clrl		d6
   movew              	a6@(SignQ),d6        |...D6 is sign(Q)
   movel               #8,d7
   lsrl                d7,d6
   andil               #0x0000007F,d3   |...7 bits of Q
   orl                 d6,d3           |...sign and bits of Q
   swap                 d3
   fmovel              fpsr,d6
   andil               #0xFF00FFFF,d6
   orl                 d3,d6
   fmovel              d6,fpsr         |...put Q in fpsr

|
Restore:
   moveml             	A7@+,d2-d7
   fmovel             	a6@(USER_FPCR),fpcr
   movel              	a6@(Sc_Flag),d0
   jeq                 Finish
   fmulx		pc@(Scale),fp0	|...may cause underflow
   jra 			__l_t_avoid_unsupp	| check for denorm as a
|					| result of the scaling

Finish:
	fmovex		fp0,fp0		| capture exceptions # round
	rts

Rem_is_0:
|..R = 2^(-j)X - Q Y = Y, thus R = 0 and quotient = 2^j (Q+1)
   addql               #1,d3
   cmpil               #8,d0           |...D0 is j
   jge                 Q_Big

   lsll                d0,d3
   jra                 Set_R_0

Q_Big:
   clrl                d3

Set_R_0:
   .long 0xf23c4400,0x00000000	 /* fmoves	&0x00000000,fp0 */
   movel		#0,a6@(Sc_Flag)
   jra                 Fix_Sign

Tie_Case:
|..Check parity of Q
   movel               d3,d6
   andil               #0x00000001,d6
   tstl                d6
   jeq                 Fix_Sign		|...Q is even

|..Q is odd, Q := Q + 1, signX := -signX
   addql               #1,d3
   movew              	a6@(SignX),d6
   eoril               #0x00008000,d6
   movew               d6,a6@(SignX)
   jra                 Fix_Sign

|   end
