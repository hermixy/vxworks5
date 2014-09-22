/* usbHcdLib.h - HCD functional API */

/* Copyright 2000 Wind River Systems, Inc. */

/*
Modification history
--------------------
01e,26jan00,rcb  Change <bytesPerFrame> parameter in usbHcdPipeCreate() to
		 <bandwidth> and redefined as UINT32.
01d,29nov99,rcb  Remove obsolete function usbHcdBusReset().
		 Increase frame number fields to 32 bits.
01c,23nov99,rcb  Replace bandwidth alloc/release functions with pipe
		 create/destroy functions...generalizes approach for use
		 with OHCI HCD.
01b,07sep99,rcb  Add support for management callbacks in attach.
		 Add set-bus-state API.
01a,08jun99,rcb  First.
*/

/*
DESCRIPTION

This file defines a functional interface to the HCD.
*/

#ifndef __INCusbHcdLibh
#define __INCusbHcdLibh

#ifdef	__cplusplus
extern "C" {
#endif


/* includes */

#include "drv/usb/usbHcd.h"


/* defines */


/* typedefs */

/*
 * HCD_NEXUS
 *
 * HCD_NEXUS contains the entry point and HCD_CLIENT_HANDLE needed by an
 * HCD caller to invoke an HCD.
 */

typedef struct hcd_nexus
    {
    HCD_EXEC_FUNC hcdExecFunc;	    /* HCD primary entry point */
    HCD_CLIENT_HANDLE handle;	    /* client's handle with HCD */
    } HCD_NEXUS, *pHCD_NEXUS;


/* functions */

STATUS usbHcdAttach
    (
    HCD_EXEC_FUNC hcdExecFunc,	    /* HCD's primary entry point */
    pVOID param,		    /* HCD-specific param */
    USB_HCD_MNGMT_CALLBACK callback,/* management callback */
    pVOID callbackParam,	    /* parameter to management callback */
    pHCD_NEXUS pNexus,		    /* nexus will be initialized on return */
    pUINT16 pBusCount
    );


STATUS usbHcdDetach
    (
    pHCD_NEXUS pNexus		    /* client's nexus */
    );


STATUS usbHcdSetBusState
    (
    pHCD_NEXUS pNexus,		    /* client's nexus */
    UINT16 busNo,		    /* bus number to suspend */
    UINT16 busState		    /* desired bus state */
    );


STATUS usbHcdSofIntervalGet
    (
    pHCD_NEXUS pNexus,		    /* client's nexus */
    UINT16 busNo,		    /* bus number */
    pUINT16 pSofInterval	    /* bfr to receive SOF interval */
    );


STATUS usbHcdSofIntervalSet
    (
    pHCD_NEXUS pNexus,		    /* client's nexus */
    UINT16 busNo,		    /* bus number */
    UINT16 sofInterval		    /* new SOF interval */
    );


STATUS usbHcdCurrentFrameGet
    (
    pHCD_NEXUS pNexus,		    /* client's nexus */
    UINT16 busNo,		    /* bus number */
    pUINT32 pFrameNo,		    /* current frame number */
    pUINT32 pFrameWindow	    /* size of frame window */
    );


STATUS usbHcdIrpSubmit
    (
    pHCD_NEXUS pNexus,		    /* client's nexus */
    HCD_PIPE_HANDLE pipeHandle,     /* pipe to which IRP is directed */
    pUSB_IRP pIrp		    /* IRP to be executed */
    );


STATUS usbHcdIrpCancel
    (
    pHCD_NEXUS pNexus,		    /* client's nexus */
    pUSB_IRP pIrp		    /* IRP to be canceled */
    );


STATUS usbHcdPipeCreate
    (
    pHCD_NEXUS pNexus,		    /* client's nexus */
    UINT16 busNo,		    /* bus number for IRP */
    UINT16 busAddress,		    /* bus address of USB device */
    UINT16 endpoint,		    /* endpoint on device */
    UINT16 transferType,	    /* transfer type */
    UINT16 direction,		    /* pipe/transfer direction */
    UINT16 speed,		    /* transfer speed */
    UINT16 maxPacketSize,	    /* packet size */
    UINT32 bandwidth,		    /* bandwidth required by pipe */
    UINT16 interval,		    /* service interval */
    pUINT32 pTime,		    /* calculated packet time on return */
    pHCD_PIPE_HANDLE pPipeHandle    /* HCD pipe handle */
    );


STATUS usbHcdPipeDestroy
    (
    pHCD_NEXUS pNexus,		    /* client's nexus */
    HCD_PIPE_HANDLE pipeHandle	    /* pipe to be destroyed */
    );


STATUS usbHcdPipeModify
    (
    pHCD_NEXUS pNexus,		    /* client's nexus */
    HCD_PIPE_HANDLE pipeHandle,     /* pipe to be modified */
    UINT16 busAddress,		    /* new bus address or 0 */
    UINT16 maxPacketSize	    /* new max packet size or 0 */
    );


#ifdef	__cplusplus
}
#endif

#endif	/* __INCusbHcdLibh */


/* End of file. */

