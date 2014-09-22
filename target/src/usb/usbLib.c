/* usbLib.c - USB utility functions */

/* Copyright 2000-2002 Wind River Systems, Inc. */

/*
Modification history
--------------------
01l,09nov01,wef  Test for NULL on OSS_xALLOC calls, change sizeof () to 
		 structure size macros
01k,25oct01,wef  repalce automatic buffer creations with calls to OSS_MALLOC
                 in places where the buffer is passed to the hardware (related
                 to SPR 70492).
01j,18sep01,wef  merge from wrs.tor2_0.usb1_1-f for veloce
01e,25jul01,jgn  fixed spr 69287
01d,26jan00,rcb  Modify usbRecurringTime() to accept <bandwidth> instead
		 of <bytesPerFrame>.
		 Add usbDescrCopy32() and usbDescrStrCopy32().
01c,17jan00,rcb  Add usbConfigDescrGet() function.
01b,23nov99,rcb  Add usbRecurringTime() function.
01a,16jul99,rcb  First.
*/

/*
DESCRIPTION

This modules contains miscellaneous functions which may be used by the
USB driver (USBD), USB HCD (USB Host Controller Driver), or by USBD clients.
*/


/* includes */

#include "usb/usbPlatform.h"

#include "string.h"

#include "usb/usbdLib.h"
#include "usb/usbLib.h"


/* defines */

/* USB time constants.
 *
 * NOTE: The following constants use units of picoseconds for improved
 * calculation resolution.  The calcTdTime() function - below - rounds
 * the final calculated to the next higher nanosecond, and returns the
 * calculated value in terms of nanoseconds.
 */

#define TIME_OVRHD_INPUT_NON_ISOCH  ((UINT32) 9107000L)
#define TIME_OVRHD_INPUT_ISOCH	    ((UINT32) 7268000L)
#define TIME_OVRHD_OUTPUT_NON_ISOCH ((UINT32) 9107000L)
#define TIME_OVRHD_OUTPUT_ISOCH     ((UINT32) 6265000L)

#define TIME_OVRHD_INPUT_LS	    ((UINT32) 64060000L)
#define TIME_OVRHD_OUTPUT_LS	    ((UINT32) 64107000L)

#define TIME_BIT		    ((UINT32) 83540L)
#define TIME_BIT_LS_INPUT	    ((UINT32) 676670L)
#define TIME_BIT_LS_OUTPUT	    ((UINT32) 667000L)


/* BIT_STUFF calculates the worst case number of bits for the indicated
 * number of bytes.  (The "19" in the following equation is equal to
 * 3.167*6.  Since 19 is added to the bit total which is then divided by
 * 6, this has the effect of ading 3.167 bit times to the total.
 */

#define BIT_STUFF(bytes)    ((UINT32) ((((UINT32) bytes) * 8 * 7 + 19) / 6))


/* functions */

/***************************************************************************
*
* usbTransferTime - Calculates bus time required for a USB transfer
*
* This function calculates the amount of time a transfer of a given number
* of bytes will require on the bus - measured in nanoseconds (10E-9 seconds).  
* The formulas used here are taken from Section 5.9.3 of Revision 1.1 of the 
* USB spec.
*
* <transferType>, <direction>, and <speed> should describe the characteristics
* of the pipe/transfer as USB_XFRTYPE_xxxx, USB_DIR_xxxx, and USB_SPEED_xxxx,
* repsectively.  <bytes> is the size of the packet for which the transfer time 
* should be calculated. <hostDelay> and <hostHubLsSetup> are the host delay and 
* low-speed hub setup times in nanoseconds, respectively, and are host-controller 
* specific.
*
* RETURNS: Worst case number of nanoseconds required for transfer
*/

UINT32 usbTransferTime
    (
    UINT16 transferType,	/* transfer type */
    UINT16 direction,		/* transfer direction */
    UINT16 speed,		/* speed of pipe */
    UINT32 bytes,		/* number of bytes for packet to be calc'd */
    UINT32 hostDelay,		/* host controller delay per packet */
    UINT32 hostHubLsSetup	/* host controller time for low-speed setup */
    )

    {
    UINT32 bits = BIT_STUFF (bytes);
    UINT32 ps;

    if (speed == USB_SPEED_FULL)
	{
	if (direction == USB_DIR_IN)
	    {
	    if (transferType == USB_XFRTYPE_ISOCH)
		{
		ps = TIME_OVRHD_INPUT_ISOCH + TIME_BIT * bits;
		}
	    else
		{
		ps = TIME_OVRHD_INPUT_NON_ISOCH + TIME_BIT * bits;
		}
	    }
	else
	    {
	    if (transferType == USB_XFRTYPE_ISOCH)
		{
		ps = TIME_OVRHD_OUTPUT_ISOCH + TIME_BIT * bits;
		}
	    else
		{
		ps = TIME_OVRHD_OUTPUT_NON_ISOCH + TIME_BIT * bits;
		}
	    }
	}
    else
	{
	if (direction == USB_DIR_IN)
	    {
	    ps = TIME_OVRHD_INPUT_LS + 2 * hostHubLsSetup * 1000L + 
		TIME_BIT_LS_INPUT * bits;
	    }
	else
	    {
	    ps = TIME_OVRHD_OUTPUT_LS + 2 * hostHubLsSetup * 1000L +
		TIME_BIT_LS_OUTPUT * bits;
	    }
	}

    ps += hostDelay * 1000L;

    return (ps + 999L) / 1000L;     /* returns value in nanoseconds */
    }


/***************************************************************************
*
* usbRecurringTime - calculates recurring time for interrupt/isoch transfers
*
* For recurring transfers (e.g., interrupt or isochronous transfers) an HCD
* needs to be able to calculate the amount of bus time - measured in 
* nanoseconds - which will be used by the transfer.
*
* <transferType> specifies the type of transfer.  For USB_XFRTYPE_CONTROL and
* USB_XFRTYPE_BULK, the calculated time is always 0...these are not recurring
* transfers.  For USB_XFRTYPE_INTERRUPT, <bandwidth> must express the number
* of bytes to be transferred in each frame.  For USB_XFRTYPE_ISOCH, <bandwidth>
* must express the number of bytes to be transferred in each second.  The
* parameter is treated differently to allow greater flexibility in determining
* the true bandwidth requirements for each type of pipe.
*
* RETURNS: worst case number of nanoseconds required for transfer.
*/

UINT32 usbRecurringTime
    (
    UINT16 transferType,	/* transfer type */
    UINT16 direction,		/* transfer direction */
    UINT16 speed,		/* speed of pipe */
    UINT16 packetSize,		/* max packet size for endpoint */
    UINT32 bandwidth,		/* bytes/frame or bytes/sec depending on pipe */
    UINT32 hostDelay,		/* host controller delay per packet */
    UINT32 hostHubLsSetup	/* host controller time for low-speed setup */
    )

    {
    UINT16 packets;
    UINT32 nanosecondsPerPacket;
    UINT16 remainderBytes;
    UINT32 remainderNanoseconds;
    UINT32 bytesPerFrame;


    /* Non-recurring transfers always return 0. */

    if (transferType == USB_XFRTYPE_CONTROL || transferType == USB_XFRTYPE_BULK)
	return 0;


    /* Calculate time for recurring transfer */

    if (transferType == USB_XFRTYPE_INTERRUPT)
	bytesPerFrame = bandwidth;
    else /* transferType == USB_XFRTYPE_ISOCH */
	bytesPerFrame = (bandwidth + 999L) / 1000L;

    packets = bytesPerFrame / packetSize;
    remainderBytes = bytesPerFrame % packetSize;

    nanosecondsPerPacket = usbTransferTime (transferType, direction,
	speed, packetSize, hostDelay, hostHubLsSetup);

    if (remainderBytes > 0)
	remainderNanoseconds = usbTransferTime (transferType, direction, 
	    speed, remainderBytes, hostDelay, hostHubLsSetup);
    else
	remainderNanoseconds = 0;

    return packets * nanosecondsPerPacket + remainderNanoseconds;
    }


/***************************************************************************
*
* usbDescrParseSkip - search for a descriptor and increment buffer
*
* Searches <ppBfr> up to <pBfrLen> bytes for a descriptor of type matching
* <descriptorType>.  Returns a pointer to the descriptor if found.  <ppBfr> 
* and <pBfrLen> are updated to reflect the next location in the buffer and 
* the remaining size of the buffer, respectively.
*
* RETURNS: pointer to indicated descriptor, or NULL if descr not found.
*/

pVOID usbDescrParseSkip
    (
    pUINT8 *ppBfr,		/* buffer to parse */
    pUINT16 pBfrLen,		/* length of buffer to parse */
    UINT8 descriptorType	/* type of descriptor being sought */
    )

    {
    pUSB_DESCR_HDR pHdr;

    /* The remaining buffer length must be at least two bytes to accommodate
     * the descriptor length and descriptorType fields.  If not, we're done. 
     */

    while (*pBfrLen >= sizeof (pHdr->length) + sizeof (pHdr->descriptorType))
	{
	pHdr = (pUSB_DESCR_HDR) *ppBfr;

	if (pHdr->length == 0)
	    return NULL;	/* must be invalid */

	*ppBfr += min (pHdr->length, *pBfrLen);
	*pBfrLen -= min (pHdr->length, *pBfrLen);

	if (pHdr->descriptorType == descriptorType)
	    return (pVOID) pHdr;
	}

    return NULL;
    }
    

/***************************************************************************
*
* usbDescrParse - search a buffer for the a particular USB descriptor
*
* Searches <pBfr> up to <bfrLen> bytes for a descriptor of type matching
* <descriptorType>.  Returns a pointer to the descriptor if found. 
*
* RETURNS: pointer to indicated descriptor, or NULL if descr not found
*/

pVOID usbDescrParse
    (
    pUINT8 pBfr,		/* buffer to parse */
    UINT16 bfrLen,		/* length of buffer to parse */
    UINT8 descriptorType	/* type of descriptor being sought */
    )

    {
    return usbDescrParseSkip (&pBfr, &bfrLen, descriptorType);
    }


/***************************************************************************
*
* usbConfigCountGet - Retrieves number of device configurations
*
* Using the <usbdClientHandle> provided by the caller, this function
* reads the <nodeId>'s device descriptor and returns the number of 
* configurations supported by the device in <pNumConfig>.
*
* RETURNS: OK, or ERROR if unable to read device descriptor
*/

STATUS usbConfigCountGet
    (
    USBD_CLIENT_HANDLE usbdClientHandle,    /* caller's USBD client handle */
    USBD_NODE_ID nodeId,		    /* device node ID */
    pUINT16 pNumConfig			    /* bfr to receive nbr of config */
    )

    {
    USB_DEVICE_DESCR * pDevDescr;
    UINT16 actLen;

    if ((pDevDescr = OSS_MALLOC (USB_DEVICE_DESCR_LEN)) == NULL)
	return ERROR;

    /* Read the device descriptor to determine the number of configurations. */

    if (usbdDescriptorGet (usbdClientHandle, 
			   nodeId, 
			   USB_RT_STANDARD | USB_RT_DEVICE, USB_DESCR_DEVICE, 
			   0, 
			   0, 
			   USB_DEVICE_DESCR_LEN, 
			   (UINT8 *) pDevDescr, 
			   &actLen) != OK ||
	actLen < USB_DEVICE_DESCR_LEN)
	{
	OSS_FREE (pDevDescr);
	return ERROR;
	}

    if (pNumConfig)
	*pNumConfig = pDevDescr->numConfigurations;

    OSS_FREE (pDevDescr);

    return OK;
    }


/***************************************************************************
*
* usbConfigDescrGet - reads full configuration descriptor from device
*
* This function reads the configuration descriptor <cfgNo> and all associated
* descriptors (interface, endpoint, etc.) for the device specified by
* <nodeId>.  The total amount of data returned by a device is variable,
* so, this function pre-reads just the configuration descriptor and uses
* the "totalLength" field from that descriptor to determine the total
* length of the configuration descriptor and its associated descriptors.
*
* This function uses the macro OSS_MALLOC() to allocate a buffer for the
* complete descriptor.	The size and location of the buffer are returned
* in <ppBfr and pBfrLen>.  It is the caller's responsibility to free the
* buffer using the OSS_FREE() macro.
*
* RETURNS: OK, or ERROR if unable to read descriptor
*/

STATUS usbConfigDescrGet
    (
    USBD_CLIENT_HANDLE usbdClientHandle,    /* caller's USBD client handle */
    USBD_NODE_ID nodeId,		    /* device node ID */
    UINT16 cfgNo,			    /* specifies configuration nbr */
    pUINT16 pBfrLen,			    /* receives length of buffer */
    pUINT8 *ppBfr			    /* receives pointer to buffer */
    )

    {
    USB_CONFIG_DESCR * pCfgDescr;
    UINT16 actLen;
    UINT16 totalLength;
    pUINT8 pBfr;

    if ((pCfgDescr = OSS_MALLOC (USB_CONFIG_DESCR_LEN)) == NULL)
	return ERROR;

    /* Validate parameters.  ppBfr must be non-NULL */

    if (ppBfr == NULL)
	return ERROR;

    *ppBfr = NULL;


    /* Read the configuration descriptor to determine the totalLengh of the
     * configuration.
     */

    if (usbdDescriptorGet (usbdClientHandle, 
			   nodeId, 
			   USB_RT_STANDARD | USB_RT_DEVICE, 
			   USB_DESCR_CONFIGURATION, 
			   cfgNo, 
			   0, 
			   USB_CONFIG_DESCR_LEN, 
			   (UINT8 *) pCfgDescr, 
			   &actLen) 
			!= OK ||
	actLen < USB_CONFIG_DESCR_LEN)
	{
	OSS_FREE (pCfgDescr);
	return ERROR;
	}


    totalLength = FROM_LITTLEW (pCfgDescr->totalLength);

    OSS_FREE (pCfgDescr);

    if (pBfrLen != NULL)
	*pBfrLen = totalLength;


    /* Allocate a buffer of totalLength bytes. */

    if ((pBfr = OSS_MALLOC (totalLength)) == NULL)
	return ERROR;


    /* Read the full configuration descriptor. */

    if (usbdDescriptorGet (usbdClientHandle, nodeId,
	USB_RT_STANDARD | USB_RT_DEVICE, USB_DESCR_CONFIGURATION, 
	cfgNo, 0, totalLength, pBfr, NULL) != OK)
	{
	OSS_FREE (pBfr);
	return ERROR;
	}

    *ppBfr = pBfr;

    return OK;
    }


/***************************************************************************
*
* usbHidReportSet - Issues a SET_REPORT request to a USB HID
*
* Using the <usbdClientHandle> provided by the caller, this function 
* issues a SET_REPORT request to the indicated <nodeId>.  The caller
* must also specify the <interface>, <reportType>, <reportId>, <reportBfr>,
* and <reportLen>.  Refer to Section 7.2.2 of the USB HID specification
* for further detail.
*
* RETURNS: OK, or ERROR if unable to issue SET_REPORT request.
*/

STATUS usbHidReportSet
    (
    USBD_CLIENT_HANDLE usbdClientHandle,    /* caller's USBD client handle */
    USBD_NODE_ID nodeId,		    /* desired node */
    UINT16 interface,			    /* desired interface */
    UINT16 reportType,			    /* report type */
    UINT16 reportId,			    /* report Id */
    pUINT8 reportBfr,			    /* report value */
    UINT16 reportLen			    /* length of report */
    )

    {
    UINT16 actLen;

    if (usbdVendorSpecific (usbdClientHandle, nodeId,
	USB_RT_HOST_TO_DEV | USB_RT_CLASS | USB_RT_INTERFACE,
	USB_REQ_HID_SET_REPORT, (reportType << 8) | reportId, interface,
	reportLen, reportBfr, &actLen) != OK ||
	actLen < reportLen)
	return ERROR;

    return OK;
    }


/***************************************************************************
*
* usbHidIdleSet - Issues a SET_IDLE request to a USB HID
*
* Using the <usbdClientHandle> provided by the caller, this function
* issues a SET_IDLE request to the indicated <nodeId>.	The caller must
* also specify the <interface>, <reportId>, and <duration>.  If the 
* <duration> is zero, the idle period is infinite.  If <duration> is
* non-zero, then it expresses time in 4msec units (e.g., a <duration>
* of 1 = 4msec, 2 = 8msec, and so forth).  Refer to Section 7.2.4 of the 
* USB HID specification for further details.
*
* RETURNS: OK, or ERROR if unable to issue SET_IDLE request.
*/

STATUS usbHidIdleSet
    (
    USBD_CLIENT_HANDLE usbdClientHandle,    /* caller's USBD client handle */
    USBD_NODE_ID nodeId,		    /* desired node */
    UINT16 interface,			    /* desired interface */
    UINT16 reportId,			    /* desired report */
    UINT16 duration			    /* idle duration */
    )

    {
    return usbdVendorSpecific (usbdClientHandle, nodeId,
	USB_RT_HOST_TO_DEV | USB_RT_CLASS | USB_RT_INTERFACE,
	USB_REQ_HID_SET_IDLE, (duration << 8) | reportId, interface,
	0, NULL, NULL);
    }


/***************************************************************************
*
* usbHidProtocolSet - Issues a SET_PROTOCOL request to a USB HID
*
* Using the <usbdClientHandle> provided by the caller, this function
* issues a SET_PROTOCOL request to the indicated <nodeId>.  The caller
* must specify the <interface> and the desired <protocol>.  The <protocol>
* is expressed as USB_HID_PROTOCOL_xxxx.  Refer to Section 7.2.6 of the
* USB HID specification for further details.
*
* RETURNS: OK, or ERROR if unable to issue SET_PROTOCOL request.
*/

STATUS usbHidProtocolSet
    (
    USBD_CLIENT_HANDLE usbdClientHandle,    /* caller's USBD client handle */
    USBD_NODE_ID nodeId,		    /* desired node */
    UINT16 interface,			    /* desired interface */
    UINT16 protocol			    /* USB_HID_PROTOCOL_xxxx */
    )

    {
    return usbdVendorSpecific (usbdClientHandle, nodeId,
	USB_RT_HOST_TO_DEV | USB_RT_CLASS | USB_RT_INTERFACE,
	USB_REQ_HID_SET_PROTOCOL, protocol, interface, 0, NULL, NULL);
    }


/* End of file. */

