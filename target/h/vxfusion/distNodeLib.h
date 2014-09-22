/* distNodeLib - distributed objects node library header (VxFusion) */

/* Copyright 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,24may99,drm  adding vxfusion prefix to VxFusion related includes
01b,10nov98,drm  added defines for node-related control parameters
01a,21oct97,ur   written.
*/

#ifndef __INCdistNodeLibh
#define __INCdistNodeLibh

/* includes */

#include "vxWorks.h"
#include "vxfusion/distLib.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* defines */

/* Node-related control function codes */

#define DIST_CTL_TYPE_NODE                  DIST_ID_NODE_LIB
#define DIST_CTL_OPERATIONAL_HOOK           ((0 << 8) | DIST_CTL_TYPE_NODE)
#define DIST_CTL_CRASHED_HOOK               ((1 << 8) | DIST_CTL_TYPE_NODE)
#define DIST_CTL_GET_LOCAL_ID               ((2 << 8) | DIST_CTL_TYPE_NODE)
#define DIST_CTL_GET_LOCAL_STATE            ((3 << 8) | DIST_CTL_TYPE_NODE)
#define DIST_CTL_RETRY_TIMEOUT              ((4 << 8) | DIST_CTL_TYPE_NODE)
#define DIST_CTL_MAX_RETRIES                ((5 << 8) | DIST_CTL_TYPE_NODE)
#define DIST_CTL_NACK_SUPPORT               ((6 << 8) | DIST_CTL_TYPE_NODE)
#define DIST_CTL_PGGYBAK_UNICST_SUPPORT     ((7 << 8) | DIST_CTL_TYPE_NODE)
#define DIST_CTL_PGGYBAK_BRDCST_SUPPORT     ((8 << 8) | DIST_CTL_TYPE_NODE)

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __INCdistNodeLibh */

