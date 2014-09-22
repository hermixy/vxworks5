/* usbHcdOhciLib.h - Defines entry point for OHCI HCD */

/* Copyright 2000 Wind River Systems, Inc. */
/*
Modification history
--------------------
01a,23nov99,rcb  First.
*/

/*
DESCRIPTION

This file exports the primary entry point for the OHCI host controller
driver for USB.  

The <param> to the HRB_ATTACH request should be a pointer to a 
PCI_CFG_HEADER which contains the PCI configuration header for the
open host controller to be managed.  Each invocation of the HRB_ATTACH
function will return an HCD_CLIENT_HANDLE for which a single host controller
will be exposed.
*/


#ifndef __INCusbHcdOhciLibh
#define __INCusbHcdOhciLibh

#ifdef	__cplusplus
extern "C" {
#endif


/* function prototypes */

STATUS usbHcdOhciExec
    (
    pVOID pHrb			    /* HRB to be executed */
    );


#ifdef	__cplusplus
}
#endif

#endif	/* __INCusbHcdOhciLibh */


/* End of file. */

