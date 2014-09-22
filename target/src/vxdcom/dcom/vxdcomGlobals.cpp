/* vxdcomGlobals */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,17dec01,nel  Add include symbol for diab build.
01b,19jul01,dbs  fix include path of vxdcomGlobals.h
01a,13aug99,aim  created
*/

#define ALLOC_GLOBALS

#include "private/vxdcomGlobals.h"

/* Include symbol for diab */
extern "C" int include_vxdcom_vxdcomGlobals (void)
    {
    return 0;
    }
