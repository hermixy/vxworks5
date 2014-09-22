/* msgQDistGrpShow.h - distributed group show library header (VxFusion) */

/* Copyright 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01a,09jul97,ur   written.
*/

#ifndef __INCmsgQDistGrpShowh
#define __INCmsgQDistGrpShowh

#include "vxWorks.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* function declarations */

#if defined(__STDC__) || defined(__cplusplus)

void msgQDistGrpShowInit (void);
STATUS msgQDistGrpShow (char *distGrpName);
STATUS msgQDistGrpDbShow (void);

#else   /* __STDC__ */

void msgQDistGrpShowInit ();
STATUS msgQDistGrpShow ();
STATUS msgQDistGrpDbShow ();

#endif  /* __STDC__ */


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif	/* __INCmsgQDistGrpShowh */

