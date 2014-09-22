/* DebugHooks.h -- Table of debug hooks  */

/* Copyright 2001 Wind River Systems, Inc. */

/*
modification history
--------------------
01d,02apr02,nel  Add WV Support.
01c,01oct01,nel  Add hook for dcomShow printing string.
01b,26sep01,nel  Modify hooks to pass IP and Port.
01a,21aug01,nel  created
*/

/*
DESCRIPTION
This module holds a table of hooks that are placed at certain points in the code to
allow debug data to be passed to routines and processed into debug output.
*/

#ifndef _DebugHooks_h
#define _DebugHooks_h

#ifdef __cplusplus
extern "C" 
    {
#endif

/*
RPC library hooks.  All of the format:
hook (buffer, buffer_len, endpoint_ip, host_port, target_port
*/
extern void (*pRpcClientOutput)(const BYTE *, DWORD, const char *, int, int);
extern void (*pRpcClientInput)(const BYTE *, DWORD, const char *, int, int);
extern void (*pRpcServerOutput)(const BYTE *, DWORD, const char *, int, int);
extern void (*pRpcServerInput)(const BYTE *, DWORD, const char *, int, int);

/* DCOM Show hooks */

extern void (*pDcomShowPrintStr)(const char *);

#ifdef __cplusplus
    }
#endif

/* Macros to set and clear hooks */
#define SETDEBUGHOOK(name, pfn) ((name) = (pfn))
#define CLEARDEBUGHOOK(name) SETDEBUGHOOK(name, NULL)

/*
Macro to call a hook. Calls should be of the followinf format:
HOOK(name)(p1, p2, p3, ...)
*/
#define HOOK(name) (name)

/* Macro to check if action on a hook is required */
#define CHECKHOOK(name) ((name) != NULL)

/*
Macro to place a hook. Calls should be of the following format:
DEBUGHOOK(name)(p1, p2, p3, ...)
*/
#define DEBUGHOOK(name) if CHECKHOOK(name) HOOK(name)

#endif
