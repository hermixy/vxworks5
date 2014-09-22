/* distLibP.h - distributed object support routines private header (VxFusion) */

/* Copyright 1999 - 2001 Wind River Systems, Inc. */

/*
modification history
--------------------
01d,10oct01,jws  change MSEC macro definitions to eliminate float ops (SPR70851)
01c,19feb99,drm  Adding rounding to DIST_TICKS_... and DIST_MSEC_...
01b,11sep98,drm  added distDump() to list of functions
01a,04sep97,ur   written.
*/

#ifndef __INCdistLibPh
#define __INCdistLibPh

#include "vxWorks.h"

#ifdef __cplusplus
extern "C" {
#endif


/*
 * Convert from one tick rate to another.  Quietly fails if the
 * output is greater than 2**32 - 1.
 * Note - the literal 2LL promotes the calculation to long long,
 *        so keep it leftmost.
 */

#define TICKS_TO_TICKS(t, rateIn, rateOut) \
    (  \
    ((t) == WAIT_FOREVER)  ?  \
        (t) :  \
        (UINT32)(((2LL * ((UINT32)(t)) * (rateOut)) / (rateIn) + 1LL) / 2LL)  \
    )

#define DIST_TICKS_TO_MSEC(ticks)  TICKS_TO_TICKS(ticks, sysClkRateGet(), 1000)

#define DIST_MSEC_TO_TICKS(msec)  TICKS_TO_TICKS(msec, 1000, sysClkRateGet())


#if defined(__STDC__) || defined(__cplusplus)

void	distPanic (char *fmt, ...);
void	distLog (char *fmt, ...);
void    distDump (char *buffer, int len);

#else   /* __STDC__ */

void	distPanic ();
void	distLog ();
void    distDump ();

#endif  /* __STDC__ */

#ifdef __cplusplus
}
#endif

#endif	/* __INCdistLibPh */

