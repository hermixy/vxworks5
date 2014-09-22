/* msgQDisthow.h - distributed message queue show library header (VxFusion) */

/* Copyright 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01a,28jul98,drm  written.
*/

#ifndef __INCmsgQDistShowh
#define __INCmsgQDistShowh

#include "vxWorks.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* function declarations */

#if defined(__STDC__) || defined(__cplusplus)

void msgQDistShowInit (void);

#else   /* __STDC__ */

void msgQDistShowInit ();

#endif  /* __STDC__ */


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif	/* __INCmsgQDistShowh */

