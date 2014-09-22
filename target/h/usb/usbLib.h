/* usbLib.h - USB utility functions */

/* Copyright 2000 Wind River Systems, Inc. */

/*
Modification history
--------------------
01d,26jan00,rcb  Modify usbRecurringTime() to accept <bandwidth> instead
		 of <bytesPerFrame>.
		 Add usbDescrCopy32() and usbDescrStrCopy32().
01c,17jan99,rcb  Add usbConfigDescrGet() function.
01b,23nov99,rcb  Add usbRecurringTime() function.
01a,16jul99,rcb  First.
*/

#ifndef __INCusbLibh
#define __INCusbLibh

#ifdef	__cplusplus
extern "C" {
#endif


/* includes */

#include "usb/usb.h"
#include "usb/usbHid.h"
#include "usb/usbdLib.h"
#include "usb/usbDescrCopyLib.h"


/* function prototypes */

UINT32 usbTransferTime
    (
    UINT16 transferType,	/* transfer type */
    UINT16 direction,		/* transfer direction */
    UINT16 speed,		/* speed of pipe */
    UINT32 bytes,		/* number of bytes for packet to be calc'd */
    UINT32 hostDelay,		/* host controller delay per packet */
    UINT32 hostHubLsSetup	/* host controller time for low-speed setup */
    );


UINT32 usbRecurringTime
    (
    UINT16 transferType,	/* transfer type */
    UINT16 direction,		/* transfer direction */
    UINT16 speed,		/* speed of pipe */
    UINT16 packetSize,		/* max packet size for endpoint */
    UINT32 bandwidth,		/* number of bytes to transfer per second */
    UINT32 hostDelay,		/* host controller delay per packet */
    UINT32 hostHubLsSetup	/* host controller time for low-speed setup */
    );


pVOID usbDescrParseSkip
    (
    pUINT8 *ppBfr,		/* buffer to parse */
    pUINT16 pBfrLen,		/* length of buffer to parse */
    UINT8 descriptorType	/* type of descriptor being sought */
    );
    

pVOID usbDescrParse
    (
    pUINT8 pBfr,		/* buffer to parse */
    UINT16 bfrLen,		/* length of buffer to parse */
    UINT8 descriptorType	/* type of descriptor being sought */
    );


STATUS usbConfigCountGet
    (
    USBD_CLIENT_HANDLE usbdClientHandle,    /* caller's USBD client handle */
    USBD_NODE_ID nodeId,		    /* device node ID */
    pUINT16 pNumConfig			    /* bfr to receive nbr of config */
    );


STATUS usbConfigDescrGet
    (
    USBD_CLIENT_HANDLE usbdClientHandle,    /* caller's USBD client handle */
    USBD_NODE_ID nodeId,		    /* device node ID */
    UINT16 cfgNo,			    /* specifies configuration nbr */
    pUINT16 pBfrLen,			    /* receives length of buffer */
    pUINT8 *ppBfr			    /* receives pointer to buffer */
    );


STATUS usbHidReportSet
    (
    USBD_CLIENT_HANDLE usbdClientHandle,    /* caller's USBD client handle */
    USBD_NODE_ID nodeId,		    /* desired node */
    UINT16 interface,			    /* desired interface */
    UINT16 reportType,			    /* report type */
    UINT16 reportId,			    /* report Id */
    pUINT8 reportBfr,			    /* report value */
    UINT16 reportLen			    /* length of report */
    );


STATUS usbHidIdleSet
    (
    USBD_CLIENT_HANDLE usbdClientHandle,    /* caller's USBD client handle */
    USBD_NODE_ID nodeId,		    /* desired node */
    UINT16 interface,			    /* desired interface */
    UINT16 reportId,			    /* desired report */
    UINT16 duration			    /* idle duration */
    );


STATUS usbHidProtocolSet
    (
    USBD_CLIENT_HANDLE usbdClientHandle,    /* caller's USBD client handle */
    USBD_NODE_ID nodeId,		    /* desired node */
    UINT16 interface,			    /* desired interface */
    UINT16 protocol			    /* USB_HID_PROTOCOL_xxxx */
    );


#ifdef	__cplusplus
}
#endif

#endif	/* __INCusbLibh */


/* End of file. */

