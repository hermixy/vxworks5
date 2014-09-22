/* distNetLib.h - distribted objects network layer header file (VxFusion) */

/* Copyright 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,24may99,drm  adding vxfusion prefix to VxFusion related includes
01b,22feb99,drm  added distNetInput()
01a,10nov98,drm  written.
*/

#ifndef __INCdistNetLibh
#define __INCdistNetLibh

#ifdef __cplusplus
extern "C" {
#endif

/* includes */

#include "vxWorks.h"
#include "vxfusion/distLib.h"
#include "vxfusion/distNodeLib.h"
#include "vxfusion/distTBufLib.h"

/* defines */

#define DIST_CTL_TYPE_NET       DIST_ID_NET_LIB
#define DIST_CTL_SERVICE_HOOK   ((0 << 8) | DIST_CTL_TYPE_NET)
#define DIST_CTL_SERVICE_CONF   ((1 << 8) | DIST_CTL_TYPE_NET)

/* typedefs */

typedef struct  /* DIST_SERV_CONF */
    {
    int     servId;     /* ID of service to configure */
    int     taskPrio;   /* priority of service task */
    int     netPrio;    /* network priority of service */
    } DIST_SERV_CONF;

/* function declarations */

#if defined(__STDC__) || defined(__cplusplus)

void    distNetInput (DIST_NODE_ID nodeIdIn, int prioIn, DIST_TBUF *pTBufIn);

#else   /* __STDC__ */

void    distNetInput ();

#endif /* __STDC__ */

#ifdef __cplusplus
}
#endif

#endif  /* __INCdistNetLibh */

