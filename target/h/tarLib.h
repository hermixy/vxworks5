/* tarLib.h - UNIX tar compatible library */

/* Copyright 1999-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,20sep01,jkf  SPR#69031, common code for both AE & 5.x.
01b,31jul99,jkf  T2 merge, tidiness & spelling.
01a,08jun98,lrn  written
*/

#ifndef __INCtarLibh
#define __INCtarLibh

#ifdef __cplusplus
extern "C" {
#endif

/* function prototypes */

#if defined(__STDC__) || defined(__cplusplus)
IMPORT void tarHelp ( void );
IMPORT STATUS tarExtract (char * pTape, int bfactor, BOOL verbose);
IMPORT STATUS tarArchive ( char *pTape, int bfactor, BOOL verbose, char *pName);
IMPORT STATUS tarToc ( char * tape, int bfactor);
#else
IMPORT void tarHelp ();
IMPORT STATUS tarExtract ();
IMPORT STATUS tarArchive ();
IMPORT STATUS tarToc ();
#endif

/* globals */
IMPORT char * TAPE ;	/* default archive file */

#ifdef __cplusplus
}
#endif

#endif /* __INCtarLibh */
