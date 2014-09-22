/* usbHcd.h - General definitions for a USB HCD (Host Controller Driver) */

/* Copyright 2000 Wind River Systems, Inc. */

/*
Modification history
--------------------
01i,18sep01,wef  merge from wrs.tor2_0.usb1_1-f for veloce
01h,07may01,wef  changed module number to be (module num << 8) | M_usbHostLib
01g,02may01,wef  changed module number to be M_<module> + M_usbHostLib
01f,05dec00,wef  moved Module number defs to vwModNum.h - add this
                 to #includes
01e,26jan00,rcb  Change "bytesPerFrame" field in HRB_PIPE_CREATE to
		 "bandwidth" and redefined as UINT32.
01d,29nov99,rcb  Remove obsolete function HCD_FNC_BUS_RESET.
		 Increase frame number fields to 32-bits.
01c,23nov99,rcb  Replace bandwidth alloc/release functions with pipe
		 create/destroy functions...generalizes approach for use
		 with OHCI HCD.
01b,07sep99,rcb  Add management callback to attach function.
01a,03jun99,rcb  First.
*/

/*
DESCRIPTION

This file defines the interface to a USB HCD (Host Controller Driver).	This
interface is generic across HCD implementations.

NOTE: All HCD functions execute synchronously.	However, all HCD functions
have been designed to allow for rapid processing.  Delayed results, such as the
completion of an IRP, are reported through callbacks.

NOTE: The USB specification states that a host controller will incorporate the
"root hub".  The caller of the HCD, generally the USBD, communicates with this
root hub using the same IRPs that are used to communicate with other USB 
devices and hubs.  The HCD is responsible for recognizing IRPs addressed to
the root hub and processing them correctly, often by emulating the USB request
behavior of the root hub.
*/


#ifndef __INCusbHcdh
#define __INCusbHcdh

#ifdef	__cplusplus
extern "C" {
#endif

/* includes */

#include "usb/usbHandleLib.h"
#include "usb/usb.h"
#include "vwModNum.h"           /* USB Module Number Def's */


/* defines */

/* HRB function codes */

#define HCD_FNC_ATTACH		    0x0000  /* attach/init */
#define HCD_FNC_DETACH		    0x0001  /* detach/shutdown */

#define HCD_FNC_SET_BUS_STATE	    0x0100  /* set bus suspend/resume state */
#define HCD_FNC_SOF_INTERVAL_GET    0x0101  /* retrieve SOF interval */
#define HCD_FNC_SOF_INTERVAL_SET    0x0102  /* set SOF interval */

#define HCD_FNC_CURRENT_FRAME_GET   0x0200  /* get current frame no */
#define HCD_FNC_IRP_SUBMIT	    0x0201  /* submit an IRP for exection */
#define HCD_FNC_IRP_CANCEL	    0x0202  /* cancel a pending IRP */

#define HCD_FNC_PIPE_CREATE	    0x0300  /* create pipe & reserve bandwidth */
#define HCD_FNC_PIPE_DESTROY	    0x0301  /* destroy pipe */
#define HCD_FNC_PIPE_MODIFY	    0x0302  /* modify pipe parameters */


/* HRB result codes */

/* 
 * USB errnos are defined as being part of the USB host Module, as are all
 * vxWorks module numbers, but the USB Module number is further divided into 
 * sub-modules.  Each sub-module has upto 255 values for its own error codes
 */
 
#define USB_HCD_SUB_MODULE  4

#define M_usbHcdLib 	( (USB_HCD_SUB_MODULE  << 8) | M_usbHostLib )

#define hcdErr(x)	(M_usbHcdLib | (x))

#define S_usbHcdLib_BAD_CLIENT		hcdErr(1)
#define S_usbHcdLib_BAD_PARAM		hcdErr(2)
#define S_usbHcdLib_BAD_HANDLE		hcdErr(3)
#define S_usbHcdLib_OUT_OF_MEMORY	hcdErr(4)
#define S_usbHcdLib_OUT_OF_RESOURCES	hcdErr(5) 
#define S_usbHcdLib_NOT_IMPLEMENTED	hcdErr(6)
#define S_usbHcdLib_GENERAL_FAULT	hcdErr(7)
#define S_usbHcdLib_NOT_INITIALIZED	hcdErr(8)
#define S_usbHcdLib_INT_HOOK_FAILED	hcdErr(9)
#define S_usbHcdLib_STRUCT_SIZE_FAULT	hcdErr(10)
#define S_usbHcdLib_HW_NOT_READY	hcdErr(11)
#define S_usbHcdLib_NOT_SUPPORTED	hcdErr(12)
#define S_usbHcdLib_SHUTDOWN		hcdErr(13)
#define S_usbHcdLib_IRP_CANCELED	hcdErr(14)
#define S_usbHcdLib_STALLED		hcdErr(15)
#define S_usbHcdLib_DATA_BFR_FAULT	hcdErr(16)
#define S_usbHcdLib_BABBLE		hcdErr(17)
#define S_usbHcdLib_CRC_TIMEOUT 	hcdErr(18)
#define S_usbHcdLib_TIMEOUT		hcdErr(19)
#define S_usbHcdLib_BITSTUFF_FAULT	hcdErr(20)
#define S_usbHcdLib_SHORT_PACKET	hcdErr(21)
#define S_usbHcdLib_CANNOT_CANCEL	hcdErr(22)
#define S_usbHcdLib_BANDWIDTH_FAULT	hcdErr(23)
#define S_usbHcdLib_SOF_INTERVAL_FAULT	hcdErr(24)
#define S_usbHcdLib_DATA_TOGGLE_FAULT	hcdErr(25)
#define S_usbHcdLib_PID_FAULT		hcdErr(26)
#define S_usbHcdLib_ISOCH_FAULT 	hcdErr(27)


/* management events */

#define HCD_MNGMT_RESUME	1	    /* remote resume */


/* bus states, bit mask */

#define HCD_BUS_SUSPEND 	0x0001
#define HCD_BUS_RESUME		0x0002


/* typedefs */

/* HCD_CLIENT_HANDLE */

typedef GENERIC_HANDLE HCD_CLIENT_HANDLE, *pHCD_CLIENT_HANDLE;


/* HCD_PIPE_HANDLE */

typedef GENERIC_HANDLE HCD_PIPE_HANDLE, *pHCD_PIPE_HANDLE;


/*
 * HCD_EXEC_FUNC
 *
 * HCD_EXEC_FUNC is the primary entry point for an HCD.  The caller passes 
 * HRBs (HCD Request Blocks) through this interface for execution by the HCD.
 */

typedef STATUS (*HCD_EXEC_FUNC) (pVOID pHrb);


/* management notification callback function */

typedef VOID (*USB_HCD_MNGMT_CALLBACK)
    (
    pVOID mngmtCallbackParam,	    /* caller-defined param */
    HCD_CLIENT_HANDLE handle,	    /* handle to host controller */
    UINT16 busNo,		    /* bus number */
    UINT16 mngmtCode		    /* management code */
    );


/*
 * HRB_HEADER
 *
 * All requests to an HCD begin with an HRB (HCD Request Block) header.
 */

typedef struct hrb_header
    {
    HCD_CLIENT_HANDLE handle;	/* I/O	caller's HCD client handle */
    UINT16 function;		/* IN	HCD function code */
    UINT16 hrbLength;		/* IN	Length of the total HRB */
    } HRB_HEADER, *pHRB_HEADER;


/*
 * HRB_ATTACH
 */

typedef struct hrb_attach
    {
    HRB_HEADER header;		/*	HRB header */
    pVOID param;		/* IN	HCD-specific parameter */
    USB_HCD_MNGMT_CALLBACK mngmtCallback;
				/* IN	USBD's callback for mngmt events */
    pVOID mngmtCallbackParam;	/* IN	USBD-defined parameter to callback */
    UINT16 busCount;		/* OUT	number of buses managed by HCD */
    } HRB_ATTACH, *pHRB_ATTACH;


/*
 * HRB_DETACH
 */

typedef struct hrb_detach
    {
    HRB_HEADER header;		/*	HRB header */
    } HRB_DETACH, *pHRB_DETACH;


/*
 * HRB_BUS_RESET
 */

typedef struct hrb_bus_reset
    {
    HRB_HEADER header;		/*	HRB header */
    UINT16 busNo;		/* IN	bus number to reset */
    } HRB_BUS_RESET, *pHRB_BUS_RESET;


/*
 * HRB_SET_BUS_STATE
 */

typedef struct hrb_set_bus_state
    {
    HRB_HEADER header;		/*	HRB header */
    UINT16 busNo;		/* IN	bus number */
    UINT16 busState;		/* IN	new bus state, HCD_BUS_xxxx */
    } HRB_SET_BUS_STATE, *pHRB_SET_BUS_STATE;


/*
 * HRB_CURRENT_FRAME_GET
 */

typedef struct hrb_current_frame_get
    {
    HRB_HEADER header;		/*	HRB header */
    UINT16 busNo;		/* IN	bus index: 0, 1, ... */
    UINT32 frameNo;		/* OUT	current frame number */
    UINT32 frameWindow; 	/* OUT	frame window size */
    } HRB_CURRENT_FRAME_GET, *pHRB_CURRENT_FRAME_GET;


/*
 * HRB_PIPE_CREATE
 */

typedef struct hrb_pipe_create
    {
    HRB_HEADER header;		/*	HRB header */
    UINT16 busNo;		/* IN	bus index: 0, 1, ... */
    UINT16 busAddress;		/* IN	bus address of USB device */
    UINT16 endpoint;		/* IN	endpoint on device */
    UINT16 transferType;	/* IN	transfer type */
    UINT16 direction;		/* IN	transfer/pipe direction */
    UINT16 speed;		/* IN	transfer speed */
    UINT16 maxPacketSize;	/* IN	packet size */
    UINT32 bandwidth;		/* IN	bandwidth required by pipe */
    UINT16 interval;		/* IN	service interval */
    UINT32 time;		/* OUT	calculated transfer time in nanoseconds */
    HCD_PIPE_HANDLE pipeHandle; /* OUT	pipe handle */
    } HRB_PIPE_CREATE, *pHRB_PIPE_CREATE;


/*
 * HRB_PIPE_DESTROY
 */

typedef struct hrb_pipe_destroy
    {
    HRB_HEADER header;		/*	HRB header */
    HCD_PIPE_HANDLE pipeHandle; /* IN	pipe to be destroyed */
    } HRB_PIPE_DESTROY, *pHRB_PIPE_DESTROY;


/*
 * HRB_PIPE_MODIFY
 */

typedef struct hrb_pipe_modify
    {
    HRB_HEADER header;		/*	HRB header */
    HCD_PIPE_HANDLE pipeHandle; /* IN	pipe to modify */
    UINT16 busAddress;		/* IN	new bus address, or 0 if unchanged */
    UINT16 maxPacketSize;	/* IN	new max packet size, or 0 if unchanged */
    } HRB_PIPE_MODIFY, *pHRB_PIPE_MODIFY;


/* 
 * HRB_IRP_SUBMIT
 */

typedef struct hrb_irp_submit
    {
    HRB_HEADER header;		/*	HRB header */
    HCD_PIPE_HANDLE pipeHandle; /* IN	pipe to which IRP is directed */
    pUSB_IRP pIrp;		/* IN	ptr to IRP */
    } HRB_IRP_SUBMIT, *pHRB_IRP_SUBMIT;


/*
 * HRB_IRP_CANCEL
 */

typedef struct hrb_irp_cancel
    {
    HRB_HEADER header;		/*	HRB header */
    pUSB_IRP pIrp;		/* IN	ptr to IPR to be canceled */
    } HRB_IRP_CANCEL, *pHRB_IRP_CANCEL;


/*
 * HRB_SOF_INTERVAL_GET_SET
 */

typedef struct hrb_sof_interval_get_set
    {
    HRB_HEADER header;		/*	HRB header */
    UINT16 busNo;		/* IN	bus index */
    UINT16 sofInterval; 	/* I/O	SOF interval */
    } HRB_SOF_INTERVAL_GET_SET, *pHRB_SOF_INTERVAL_GET_SET;


#ifdef	__cplusplus
}
#endif

#endif	/* __INCusbHcdh */


/* End of file. */

