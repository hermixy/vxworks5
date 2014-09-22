/* vxdcomExtent.cpp - VXDCOM_EXTENT implementation (VxDCOM) */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01l,17dec01,nel  Add include file for diab build.
01k,28feb00,dbs  fix extent size
01j,18feb00,dbs  fix compilation issues
01i,09sep99,drm  Adding include for new include file which contains
                 GUID_VXDCOM_EXTENT definition.
01h,19aug99,aim  change assert to VXDCOM_ASSERT
01g,03aug99,drm  Changing long to int.
01f,29jul99,drm  Removing unnecessary code.
01e,29jul99,drm  Changing VXDCOM_EXTENT to accomodate VXDCOMEXTENT structure.
01d,15jul99,drm  Removing separate default constructor.
01c,13jul99,drm  changed call to constructor to class::class form 
01b,17jun99,aim  changed assert to assert
01a,17may99,drm  created
*/

#include "dcomProxy.h"		// For VXDCOM_DREP_LITTLE_ENDIAN
#include "TraceCall.h"
#include "dcomExtent.h"		// For GUID_VXDCOM_EXTENT, VXDCOMEXTENT
#include "vxdcomExtent.h"	// For interface declaration

/* Include symbol for diab */
extern "C" int include_vxdcom_vxdcomExtent (void)
    {
    return 0;
    }

///////////////////////////////////////////////////////////////////////////////
//
// VXDCOM_EXTENT - default / non-default constructor 
//
// This constructor creates a new VXDCOM_EXTENT object of with priority
// <priority>.  If no priority is provided, then a default priority of
// 0 is used.
//
// RETURNS: none
//
// nomanual
//
 
VXDCOM_EXTENT::VXDCOM_EXTENT
    (
    int priority		// client priority to be propagated (default = 0)
    )
    {
    TRACE_CALL;

    id = GUID_VXDCOM_EXTENT;
    size = sizeof (VXDCOMEXTENT);
    extent.dummy = 0;
    
    setPriority (priority); 
    }
 


///////////////////////////////////////////////////////////////////////////////
//
// setPriority - set the priority to a new value 
//
// This method sets the priority of the VXDCOM_EXTENT object to a new value
// specified by the <priority> argument.  The value is not kept in a separate
// variable, but rather stored as an already marshalled value in the data
// array of bytes. 
//
// RETURNS: S_OK
//
// nomanual
//  

void VXDCOM_EXTENT::setPriority
    (
    int priority		// priority to set data to
    )
    {
    TRACE_CALL;

    // Copy the priority into the array of bytes and use ndr_make_right()
    // to set the byte order to a known byte order.  We cannot make assumptions
    // about the byte order because we don't know the byte order of the 
    // remote node. 
    ndr_make_right (priority, VXDCOM_DREP_LITTLE_ENDIAN);
    *((long*) data) = (long) priority;
    }


///////////////////////////////////////////////////////////////////////////////
//
// getPriority - get the priority of the current object 
//
// This routine return the encoded priority from a VXDCOM_EXTENT structure 
//
// RETURNS: encoded priority
//
// nomanual
//

int VXDCOM_EXTENT::getPriority ()
    {
    TRACE_CALL;

    // Get the priority and convert it back to the correct byte order
    // before returning...
    int priority = (int) *((long*)data);
    ndr_make_right (priority, VXDCOM_DREP_LITTLE_ENDIAN);
    return priority;
    }


