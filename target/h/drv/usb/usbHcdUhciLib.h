/* usbHcdUhciLib.h - Defines entry point for UHCI HCD */

/* Copyright 2000 Wind River Systems, Inc. */

/*
Modification history
--------------------
01b,23nov99,rcb  Remove comment block for usbHcdUhciExec().
01a,09jun99,rcb  First.
*/

/*
DESCRIPTION

This file exports the primary entry point for the UHCI host controller
driver for USB.  

The <param> to the HRB_ATTACH request should be a pointer to a 
PCI_CFG_HEADER which contains the PCI configuration header for the
universal host controller to be managed.  Each invocation of the HRB_ATTACH
function will return an HCD_CLIENT_HANDLE for which a single host controller
will be exposed.
*/


#ifndef __INCusbHcdUhciLibh
#define __INCusbHcdUhciLibh

#ifdef	__cplusplus
extern "C" {
#endif


STATUS usbHcdUhciExec
    (
    pVOID pHrb			    /* HRB to be executed */
    );


#ifdef	__cplusplus
}
#endif

#endif	/* __INCusbHcdUhciLibh */


/* End of file. */

