/* dcomExtent.h - header for VxDCOM ORPC_EXTENT*/

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*

modification history
--------------------
01c,28feb00,dbs  fix extent size
01b,15feb00,dbs  add all extent-related defs
01a,13aug99,drm  created

*/

#ifndef __INCdcomExtent_h
#define __INCdcomExtent_h


#ifdef _WIN32
#include <windows.h>
#else
#include "comLib.h"
#endif


// defines

#define VXDCOM_MAX_EXTENT_DATA  sizeof(VXDCOMEXTENT)
#define VXDCOM_MAX_EXTENT_ARRAY 8


//////////////////////////////////////////////////////////////////////////
//
// GUID and structure for VxDCOM extenstions
//

const GUID GUID_VXDCOM_EXTENT =
    {0x5c460fb8,0x8b2,0x11d3,{0x83,0x45,0x0,0x60,0x8,0x1e,0x90,0x8}};

typedef struct
    {
    long priority;			// client propagated priority
    long dummy;				// force size up to 8
    } VXDCOMEXTENT;


#endif
