/* cp-demangle.c - C++ symbol demangler stub */

/* Copyright 1998 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01a,26nov26,sn   wrote
*/

/*
DESCRIPTION
For now don't pull in the full g++ V3 demangler
since we don't currently use this demangling style.
Instead provide a stub definition (which should
never be called ...)

NOMANUAL
*/

char * cplus_demangle_v3 
    (
    const char * mangled
    )
    {
    return (char *) mangled;
    }
