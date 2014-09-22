/* vxdcomGlobals */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01e,02aug01,dbs  add globals for SCM task stack and prio
01d,18jul01,dbs  move g_defaultServerPriority to comCoreLib
01c,17aug99,aim  added g_vxdcomExportAddress globals
01b,13aug99,aim  changed minThreads and maxThreads initial values
01a,13aug99,aim  created
*/

#ifndef __INCvxdcomGlobals_h
#define __INCvxdcomGlobals_h

/* __EC__ is EXTERN_C but hopefully unused elsewhere. */

#ifdef ALLOC_GLOBALS
#define __EC__
#define __I(X) = X
#else
#define __EC__ extern "C"
#define __I(X)
#endif

#include "dcomLib.h"

__EC__ int g_vxdcomMinThreads __I(1);

__EC__ int g_vxdcomMaxThreads __I(5);

__EC__ int g_vxdcomThreadPoolPriority __I(100);

__EC__ DWORD g_defaultAuthnLevel __I(RPC_C_AUTHN_LEVEL_NONE);

__EC__ DWORD g_defaultImpLevel __I(RPC_C_IMP_LEVEL_ANONYMOUS);

__EC__ bool g_clientPriorityPropagation __I(true);

__EC__ int g_vxdcomDefaultStackSize __I(32 * 1024);

__EC__ int g_scmTaskStackSize __I(32 * 1024);

__EC__ int g_scmTaskPriority __I(150);

__EC__ int vxdcomBSTRPolicy __I(0);
// non-zero == marshal BSTRs as char array

__EC__ int g_vxdcomObjectExporterPortNumber __I(13535);

#endif // __INCvxdcomGlobals_h
