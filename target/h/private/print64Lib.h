/* print64Lib.h - pretty print 64-bit integers library */

/* Copyright 1999-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01d,20sep01,jkf  SPR#69031, common code for both AE & 5.x.
01c,29feb00,jkf  T3 merge, cleanup.
01b,31jul99,jkf  T2 merge, tidiness & spelling.
01a,29jun98,vld  written.
*/

#ifndef __INTprint64Libh
#define __INTprint64Libh

#include "private/dosFsLibP.h"

#ifdef __cplusplus
extern "C" {
#endif

/* function prototypes */

IMPORT void print64Fine ( char * pHeader, fsize_t val, char * pFooter,
			  u_int radix );
IMPORT void print64Row ( char * pHeader, fsize_t val, char * pFooter,
                          u_int radix );
IMPORT void print64Mult ( char * pHeader, fsize_t val, char * pFooter,
                          u_int radix );
IMPORT void print64 ( char * pHeader, fsize_t val, char * pFooter,
                          u_int radix, u_int groupeSize );

#ifdef __cplusplus
    }
#endif
 
#endif /* __INTprint64Libh */

