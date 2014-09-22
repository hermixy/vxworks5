/* usbHcdLib.c - Implements HCD functional API */

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
01b,07sep99,rcb  Add support for management callback param in attach.
		 Add set-bus-state API.
01a,09jun99,rcb  First.
*/

/*
DESCRIPTION

This file implements the functional interface to the HCD.
*/


/* includes */

#include "usb/usbPlatform.h"

#include "string.h"

#include "usb/usbHcdLib.h"	/* our API */


/* functions */

/***************************************************************************
*
* hrbInit - Initialize an HCD request block
*
* RETURNS: N/A
*/

LOCAL VOID hrbInit 
    (
    pHRB_HEADER pHrb, 
    pHCD_NEXUS pNexus, 
    UINT16 function,
    UINT16 totalLen
    )

    {
    memset (pHrb, 0, totalLen);

    if (pNexus != NULL)
	pHrb->handle = pNexus->handle;

    pHrb->function = function;
    pHrb->hrbLength = totalLen;
    }


/***************************************************************************
*
* usbHcdAttach - Attach to the HCD
*
* Attempts to connect the caller to the HCD.  The <param> value is HCD-
* implementation-specific.  Returns an HCD_CLIENT_HANDLE if the HCD was
* able to initialize properly.	If <pBusCount> is not NULL, also returns
* number of buses managed through this nexus.
*
* <callback> is an optional pointer to a routine which should be invoked
* if the HCD detects "management events" (e.g., remote wakeup/resume).
* <callbackParam> is a caller-defined parameter which will be passed to
* the <callback> routine each time it is invoked.
*
* RETURNS: OK, or ERROR if unable to initialize HCD.
*/

STATUS usbHcdAttach
    (
    HCD_EXEC_FUNC hcdExecFunc,	    /* HCD's primary entry point */
    pVOID param,		    /* HCD-specific param */
    USB_HCD_MNGMT_CALLBACK callback,/* management callback */
    pVOID callbackParam,	    /* parameter to management callback */
    pHCD_NEXUS pNexus,		    /* nexus will be initialized on return */
    pUINT16 pBusCount
    )

    {
    HRB_ATTACH hrb;
    STATUS s;


    /* Initialize HRB */

    hrbInit (&hrb.header, NULL, HCD_FNC_ATTACH, sizeof (hrb));

    hrb.param = param;
    hrb.mngmtCallback = callback;
    hrb.mngmtCallbackParam = callbackParam;


    /* Execute HRB */

    s = (*hcdExecFunc) ((pVOID) &hrb);


    /* Return results */
    
    if (pNexus != NULL)
	{
	pNexus->hcdExecFunc = hcdExecFunc;
	pNexus->handle = hrb.header.handle;
	}

    if (pBusCount != NULL)
	*pBusCount = hrb.busCount;

    return s;
    }


/***************************************************************************
*
* usbHcdDetach - Detach from the HCD
*
* Disconnects a caller which has previously attached to an HCD.
*
* RETURNS: OK, or ERROR if unable to shutdown HCD.
*/

STATUS usbHcdDetach
    (
    pHCD_NEXUS pNexus		    /* client's nexus */
    )

    {
    HRB_DETACH hrb;


    /* Initialize HRB */

    hrbInit (&hrb.header, pNexus, HCD_FNC_DETACH, sizeof (hrb));


    /* Execute HRB */

    return (*pNexus->hcdExecFunc) ((pVOID) &hrb);
    }


/***************************************************************************
*
* usbHcdSetBusState - sets bus suspend/resume state
*
* Sets the state for <bus> no as specified in <busState>.  <busState>
* is a bit mask.  Typically, the caller will set USB_BUS_SUSPEND or
* USB_BUS_RESUME to suspend or resume the indicated bus.
*
* RETURNS: OK, or ERROR if unable to place bus in specified state
*/

STATUS usbHcdSetBusState
    (
    pHCD_NEXUS pNexus,		    /* client's nexus */
    UINT16 busNo,		    /* bus number */
    UINT16 busState		    /* desired bus state */
    )

    {
    HRB_SET_BUS_STATE hrb;


    /* Initialize HRB */

    hrbInit (&hrb.header, pNexus, HCD_FNC_SET_BUS_STATE, sizeof (hrb));

    hrb.busNo = busNo;
    hrb.busState = busState;


    /* Execute HRB */

    return (*pNexus->hcdExecFunc) ((pVOID) &hrb);
    }


/***************************************************************************
*
* usbHcdCurrentFrameGet - Returns current frame number for a bus
*
* Returns the current <pFrameNo> and the frame window, <pFrameWindow>
* for the specified bus.
*
* RETURNS: OK, or ERROR if unable to retrieve current frame number.
*/

STATUS usbHcdCurrentFrameGet
    (
    pHCD_NEXUS pNexus,		    /* client's nexus */
    UINT16 busNo,		    /* bus number */
    pUINT32 pFrameNo,		    /* current frame number */
    pUINT32 pFrameWindow	    /* size of frame window */
    )

    {
    HRB_CURRENT_FRAME_GET hrb;
    STATUS s;


    /* Initialize HRB */

    hrbInit (&hrb.header, pNexus, HCD_FNC_CURRENT_FRAME_GET, sizeof (hrb));

    hrb.busNo = busNo;


    /* Execute HRB */

    s = (*pNexus->hcdExecFunc) ((pVOID) &hrb);


    /* return results */

    if (pFrameNo != NULL)
	*pFrameNo = hrb.frameNo;

    if (pFrameWindow != NULL)
	*pFrameWindow = hrb.frameWindow;

    return s;
    }


/***************************************************************************
*
* usbHcdIrpSubmit - Submits an IRP to the HCD for execution
*
* This function passes the <pIrp> to the HCD for scheduling.  The function
* returns as soon as the HCD has queued/scheduled the IRP.  The <pIrp>
* must include a non-NULL <callback> which will be invoked upon IRP
* completion.
*
* RETURNS: OK, or ERROR if unable to submit IRP for transfer.
*/

STATUS usbHcdIrpSubmit
    (
    pHCD_NEXUS pNexus,		    /* client's nexus */
    HCD_PIPE_HANDLE pipeHandle,     /* pipe to which IRP is directed */
    pUSB_IRP pIrp		    /* IRP to be executed */
    )

    {
    HRB_IRP_SUBMIT hrb;


    /* Initialize HRB */

    hrbInit (&hrb.header, pNexus, HCD_FNC_IRP_SUBMIT, sizeof (hrb));

    hrb.pipeHandle = pipeHandle;
    hrb.pIrp = pIrp;


    /* Execute HRB */

    return (*pNexus->hcdExecFunc) ((pVOID) &hrb);
    }


/***************************************************************************
*
* usbHcdIrpCancel - Requests the HCD to cancel a pending IRP
*
* This function requests the HCD to cancel the specified <pIrp>.  If
* the IRP can be canceled before it completes execution normally, its 
* result will be set to S_usbHcdLib_IRP_CANCELED and the IRPs callback
* will be invoked.
*
* There is no guarantee that an IRP, once submitted to the HCD, can be
* canceled before it otherwise completes normally (or times out).
*
* RETURNS: OK, or ERROR if unable to cancel transfer.
*/


STATUS usbHcdIrpCancel
    (
    pHCD_NEXUS pNexus,		    /* client's nexus */
    pUSB_IRP pIrp		    /* IRP to be canceled */
    )

    {
    HRB_IRP_CANCEL hrb;


    /* Initialize HRB */

    hrbInit (&hrb.header, pNexus, HCD_FNC_IRP_CANCEL, sizeof (hrb));

    hrb.pIrp = pIrp;


    /* Execute HRB */

    return (*pNexus->hcdExecFunc) ((pVOID) &hrb);
    }


/***************************************************************************
*
* usbHcdPipeCreate - create a pipe and calculate / reserve bus bandwidth
*
* The USBD calls this function to notify the HCD that it is attempting to
* create a new pipe.  The USBD passes the type of pipe in <transferType>.
*
* If the pipe is an interrupt or isochronous pipe, the HCD calculates the 
* amount of time a transfer of a given number of bytes will require on the 
* bus - measured in nanoseconds (10E-9 seconds).  The formulas used here are 
* taken from Section 5.9.3 of Revision 1.1 of the USB spec.
*
* If enough bus bandwidth is available, then that amount of bandwidth will
* be reserved by the HCD.  Reserved bandwidth must later be released using 
* usbHcdPipeDestroy().
*
* <transferType>, <direction>, and <speed> should describe the characteristics
* of the pipe/transfer as USB_XFRTYPE_xxxx, USB_DIR_xxxx, and USB_SPEED_xxxx,
* repsectively.  <packetSize> is the size of packets to be used.
*
* For interrupt pipes, <bandwidth> is the total number of bytes which will 
* be sent each frame.  For isochronous pipes, <bandwidth> is the number of
* bytes per second.  <bandwidth> should be 0 for control and bulk pipes.
*
* The worst-case transfer time is returned in <pTime>.
*
* The HCD will return an HCD_PIPE_HANDLE in <pPipeHandle>.  The USBD will use
* this HCD_PIPE_HANDLE to identify the pipe in the future.  
*
* RETURNS: OK, or ERROR if unable to reserve bandwdith.
*/

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
    )

    {
    HRB_PIPE_CREATE hrb;
    STATUS s;


    /* Initialize HRB */

    hrbInit (&hrb.header, pNexus, HCD_FNC_PIPE_CREATE, sizeof (hrb));

    hrb.busNo = busNo;
    hrb.busAddress = busAddress;
    hrb.endpoint = endpoint;
    hrb.transferType = transferType;
    hrb.direction = direction;
    hrb.speed = speed;
    hrb.maxPacketSize = maxPacketSize;
    hrb.bandwidth = bandwidth;
    hrb.interval = interval;


    /* Execute HRB */

    s = (*pNexus->hcdExecFunc) ((pVOID) &hrb);


    /* return results */

    if (pTime != NULL)
	*pTime = hrb.time;

    if (pPipeHandle != NULL)
	*pPipeHandle = hrb.pipeHandle;

    return s;
    }
    

/***************************************************************************
*
* usbHcdPipeDestroy - destroys pipe and releases previously allocated bandwidth
*
* Destroys the pipe identified by <pipeHandle> and releases any bandwidth used
* by the pipe.	
*
* RETURNS: OK, or ERROR if HCD fails to destroy pipe.
*/

STATUS usbHcdPipeDestroy
    (
    pHCD_NEXUS pNexus,		    /* client's nexus */
    HCD_PIPE_HANDLE pipeHandle	    /* pipe to be destroyed */
    )

    {
    HRB_PIPE_DESTROY hrb;


    /* Initialize HRB */

    hrbInit (&hrb.header, pNexus, HCD_FNC_PIPE_DESTROY, sizeof (hrb));
    hrb.pipeHandle = pipeHandle;


    /* Execute HRB */

    return (*pNexus->hcdExecFunc) ((pVOID) &hrb);
    }	


/***************************************************************************
*
* usbHcdPipeModify - modify characteristics of an existing pipe
*
* Two characteristics of a pipe, the device's USB bus address and the 
* maximum packet size, may change after a pipe is first created.  Typically,
* this will happen only with the default control pipe for a given device,
* which must be created before issuing SET_ADDRESS to the device and before
* reading the device descriptor to determine the maximum packet size supported
* by the default control endpoint.  The USBD will typically use this function
* to update either the <busAddress> or the <maxPacketSize> for the indicated
* <pipeHandle>.  If either <busAddress> or <maxPacketSize> is 0, then the
* corresponding pipe attribute remains unchanged.  
*
* RETURNS: OK, or ERROR if HCD fails to modify pipe
*/

STATUS usbHcdPipeModify
    (
    pHCD_NEXUS pNexus,		    /* client's nexus */
    HCD_PIPE_HANDLE pipeHandle,     /* pipe to be modified */
    UINT16 busAddress,		    /* new bus address or 0 */
    UINT16 maxPacketSize	    /* new max packet size or 0 */
    )

    {
    HRB_PIPE_MODIFY hrb;


    /* Initialize HRB */

    hrbInit (&hrb.header, pNexus, HCD_FNC_PIPE_MODIFY, sizeof (hrb));
    hrb.pipeHandle = pipeHandle;
    hrb.busAddress = busAddress;
    hrb.maxPacketSize = maxPacketSize;


    /* Execute HRB */

    return (*pNexus->hcdExecFunc) ((pVOID) &hrb);
    }	


/***************************************************************************
*
* usbHcdSofIntervalGet - retrieves SOF interval for a bus
*
* Returns the SOF interval for <busNo> in <pSofInterval>.  The SOF 
* interval is expressed in terms of high-speed bit times, and is typically
* close to 12,000.
*
* RETURNS: OK, or ERROR if HCD failed to retrieve SOF interval
*/

STATUS usbHcdSofIntervalGet
    (
    pHCD_NEXUS pNexus,		    /* client's nexus */
    UINT16 busNo,		    /* bus number */
    pUINT16 pSofInterval	    /* bfr to receive SOF interval */
    )

    {
    HRB_SOF_INTERVAL_GET_SET hrb;
    STATUS s;


    /* Initialize HRB */

    hrbInit (&hrb.header, pNexus, HCD_FNC_SOF_INTERVAL_GET, sizeof (hrb));
    hrb.busNo = busNo;


    /* Execute HRB */

    s = (*pNexus->hcdExecFunc) ((pVOID) &hrb);


    /* return results */

    if (pSofInterval != NULL)
	*pSofInterval = hrb.sofInterval;

    return s;
    }	


/***************************************************************************
*
* usbHcdSofIntervalSet - sets SOF interval for a bus
*
* Sets the SOF interval for <busNo> to <sofInterval>.  <sofInterval>
* must express the new SOF interval in terms of high-speed bit times, and
* should be in the neighborhood of 12,000.  Certain HCD implementations
* may impose narrower or wider limits on the allowable <sofInterval>.
*
* RETURNS: OK, or ERROR if HCD failed to set SOF interval.
*/

STATUS usbHcdSofIntervalSet
    (
    pHCD_NEXUS pNexus,		    /* client's nexus */
    UINT16 busNo,		    /* bus number */
    UINT16 sofInterval		    /* new SOF interval */
    )

    {
    HRB_SOF_INTERVAL_GET_SET hrb;


    /* Initialize HRB */

    hrbInit (&hrb.header, pNexus, HCD_FNC_SOF_INTERVAL_SET, sizeof (hrb));
    hrb.busNo = busNo;
    hrb.sofInterval = sofInterval;


    /* Execute HRB */

    return (*pNexus->hcdExecFunc) ((pVOID) &hrb);
    }	


/* End of file. */

