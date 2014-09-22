/* distStatLibP.h - statistics library private header (VxFusion) */

/* Copyright 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01a,11sep98,drm   written.
*/

#ifndef __INCdistStatLibPh
#define __INCdistStatLibPh

#ifdef __cplusplus
extern "C" {
#endif

/* function declarations */

#if defined(__STDC__) || defined(__cplusplus)

void distStatInit (void);

#else   /* __STDC__ */

void distStatInit ();

#endif  /* __STDC__ */

#ifdef __cplusplus
}
#endif

#endif __INCdistStatLibPh
