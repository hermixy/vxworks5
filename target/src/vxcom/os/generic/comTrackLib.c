/* comTrackLib.c - vxcom ComTrack support library */

/* Copyright (c) 2001 Wind River Systems, Inc. */

/*

modification history
--------------------
01a,21jun01,nel  created

*/

/*

DESCRIPTION

Methods and data to support the Inspector.

*/

/* Data structure referenced by the Inspector to browse COM objects */

struct _comTrackSupport
    {
    unsigned long	pd;
    void *		classSymbol;
    void *		interfaceSymbol;
    } comTrackSupport = { 0, 0, 0 };

/**************************************************************************
*
* setComTrackEntry - set's the COM track entry point.
*
* This function is used to set a kernel symbol to point to the PD
* containt the D/COM objects. A pointer within this domain is also set
* to the start of the linked list contain the comTrack information. This
* is used by the browser to browse D/COM objects.
*
* RETURNS: nothing
* .NOMANUAL
*/

void setComTrackEntry 
    (
    unsigned long   pd, 		/* PD that contains the COM classes */
    void          * classSymbol,	/* List of CoClasses */
    void          * interfaceSymbol	/* List of interfaces */
    )
    {
    comTrackSupport.pd = pd;
    comTrackSupport.classSymbol = classSymbol;
    comTrackSupport.interfaceSymbol = interfaceSymbol;
    }
