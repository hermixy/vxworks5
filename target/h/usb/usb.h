/* usb.h - Basic USB (Universal Serial Bus) definitions */

/* Copyright 2000-2002 Wind River Systems, Inc. */

/*
Modification history
--------------------
01i,28mar02,h_k  removed #if (CPU==SH7700)&&(_BYTE_ORDER==_BIG_ENDIAN) and set
                 USB_MAX_DESCR_LEN 255 for SH7700 (SPR #73896).
01h,16nov11,wef  Pack ENDPOINT_DESCRIPTOR structure.
01g,08nov11,wef  add provisions for SH - pack structures and conditionally make 
		 USB_MAX_DESCR_LEN a multiple of 4
01f,18sep01,wef  merge from wrs.tor2_0.usb1_1-f for veloce
01e,25jul01,rcb  fixed spr #69287
01d,26jan00,rcb  Add "dataBlockSize" field to USB_IRP to give application
		 greater control over isochronous transfer sizes.
		 Change USB_BFR_LIST "bfrLen" and "actLen" fields to UINT32.
01c,12jan00,rcb  Add definition for USB_CLASS_AUDIO.
		 Add typedef for USB_DESCR_HDR_SUBHDR.
01b,07sep99,rcb  Add definition for USB_TIME_RESUME.
01a,03jun99,rcb  First.
*/

/*
DESCRIPTION

This file defines constants and structures related to the USB (Universal
Serial Bus).

NOTE: The USB specification defines that multi-byte fields will be stored
in little-endian format.
*/

#ifndef __INCusbh
#define __INCusbh

#ifdef	__cplusplus
extern "C" {
#endif


/* includes */

#include "usb/usbListLib.h"
#include "usb/usbHandleLib.h"


/* Defined by the USB */

/* defines */

#define USB_MAX_DEVICES 	127

#define USB_MAX_TOPOLOGY_DEPTH	6

#define USB_MAX_DESCR_LEN	255


/* USB class codes */

#define USB_CLASS_AUDIO 	0x01
#define USB_CLASS_HID		0x03
#define USB_CLASS_PRINTER	0x07
#define USB_CLASS_HUB		0x09


/* USB requestType (bmRequestType) bit fields */

#define USB_RT_DIRECTION_MASK	0x80
#define USB_RT_HOST_TO_DEV	0x00
#define USB_RT_DEV_TO_HOST	0x80

#define USB_RT_CATEGORY_MASK	0x60
#define USB_RT_STANDARD 	0x00
#define USB_RT_CLASS		0x20
#define USB_RT_VENDOR		0x40

#define USB_RT_RECIPIENT_MASK	0x03
#define USB_RT_DEVICE		0x00
#define USB_RT_INTERFACE	0x01
#define USB_RT_ENDPOINT 	0x02
#define USB_RT_OTHER		0x03


/* USB requests (bRequest) */

#define USB_REQ_GET_STATUS	    0
#define USB_REQ_CLEAR_FEATURE	    1
#define USB_REQ_GET_STATE	    2
#define USB_REQ_SET_FEATURE	    3
#define USB_REQ_SET_ADDRESS	    5
#define USB_REQ_GET_DESCRIPTOR	    6
#define USB_REQ_SET_DESCRIPTOR	    7
#define USB_REQ_GET_CONFIGURATION   8
#define USB_REQ_SET_CONFIGURATION   9
#define USB_REQ_GET_INTERFACE	    10
#define USB_REQ_SET_INTERFACE	    11
#define USB_REQ_GET_SYNCH_FRAME     12


/* USB descriptor types */

#define USB_DESCR_DEVICE	    0x01
#define USB_DESCR_CONFIGURATION     0x02
#define USB_DESCR_STRING	    0x03
#define USB_DESCR_INTERFACE	    0x04
#define USB_DESCR_ENDPOINT	    0x05
#define USB_DESCR_HID		    0x21
#define USB_DESCR_REPORT	    0x22    /* HID report descriptor */
#define USB_DESCR_PHYSICAL	    0x23    /* HID physical descriptor */
#define USB_DESCR_HUB		    0x29


/* configuration descriptor attributes values */

#define USB_ATTR_SELF_POWERED	    0x40
#define USB_ATTR_REMOTE_WAKEUP	    0x20


/* Max power available from a hub port */

#define USB_POWER_SELF_POWERED	    500     /* 500 mA */
#define USB_POWER_BUS_POWERED	    100     /* 100 mA */


/* power requirements as reported in configuration descriptors */

#define USB_POWER_MA_PER_UNIT	    2	    /* 2 mA per unit */


/* endpoint descriptor attributes values */

#define USB_ATTR_EPTYPE_MASK	    0x03
#define USB_ATTR_CONTROL	    0x00
#define USB_ATTR_ISOCH		    0x01
#define USB_ATTR_BULK		    0x02
#define USB_ATTR_INTERRUPT	    0x03


/* standard USB feature selectors */

#define USB_FSEL_DEV_REMOTE_WAKEUP	1
#define USB_FSEL_DEV_ENDPOINT_HALT	0


/* hub feature selections */

#define USB_HUB_FSEL_C_HUB_LOCAL_POWER	0
#define USB_HUB_FSEL_C_HUB_OVER_CURRENT 1
#define USB_HUB_FSEL_PORT_CONNECTION	0
#define USB_HUB_FSEL_PORT_ENABLE	1
#define USB_HUB_FSEL_PORT_SUSPEND	2
#define USB_HUB_FSEL_PORT_OVER_CURRENT	3
#define USB_HUB_FSEL_PORT_RESET 	4
#define USB_HUB_FSEL_PORT_POWER 	8
#define USB_HUB_FSEL_PORT_LOW_SPEED	9
#define USB_HUB_FSEL_C_PORT_CONNECTION	16
#define USB_HUB_FSEL_C_PORT_ENABLE	17
#define USB_HUB_FSEL_C_PORT_SUSPEND	18
#define USB_HUB_FSEL_C_PORT_OVER_CURRENT 19
#define USB_HUB_FSEL_C_PORT_RESET	20


/* endpoint direction mask */

#define USB_ENDPOINT_DIR_MASK		0x80
#define USB_ENDPOINT_OUT		0x00
#define USB_ENDPOINT_IN 		0x80


/* standard endpoints */

#define USB_MAX_ENDPOINT_NUM		15
#define USB_ENDPOINT_MASK		0xf

#define USB_ENDPOINT_DEFAULT_CONTROL	0


/* hub characteristics */

#define USB_HUB_GANGED_POWER		0x0000
#define USB_HUB_INDIVIDUAL_POWER	0x0001

#define USB_HUB_NOT_COMPOUND		0x0000
#define USB_HUB_COMPOUND		0x0004

#define USB_HUB_GLOBAL_OVERCURRENT	0x0000
#define USB_HUB_INDIVIDUAL_OVERCURRENT	0x0008


/* standard device status */

#define USB_DEV_STS_LOCAL_POWER 	0x0001
#define USB_DEV_STS_REMOTE_WAKEUP	0x0002


/* standard endpoint status */

#define USB_ENDPOINT_STS_HALT		0x0001


/* hub status & change */

#define USB_HUB_STS_LOCAL_POWER 	0x0001
#define USB_HUB_STS_OVER_CURRENT	0x0002

#define USB_HUB_C_LOCAL_POWER		0x0001
#define USB_HUB_C_OVER_CURRENT		0x0002


/* hub port status & change */

#define USB_HUB_STS_PORT_CONNECTION	0x0001
#define USB_HUB_STS_PORT_ENABLE 	0x0002
#define USB_HUB_STS_PORT_SUSPEND	0x0004
#define USB_HUB_STS_PORT_OVER_CURRENT	0x0008
#define USB_HUB_STS_PORT_RESET		0x0010
#define USB_HUB_STS_PORT_POWER		0x0100
#define USB_HUB_STS_PORT_LOW_SPEED	0x0200

#define USB_HUB_C_PORT_CONNECTION	0x0001
#define USB_HUB_C_PORT_ENABLE		0x0002
#define USB_HUB_C_PORT_SUSPEND		0x0004
#define USB_HUB_C_PORT_OVER_CURRENT	0x0008
#define USB_HUB_C_PORT_RESET		0x0010


/* hub status endpoint */

#define USB_HUB_ENDPOINT_STS_HUB	0x01
#define USB_HUB_ENDPOINT_STS_PORT0	0x02


/* USB timing */

#define USB_TIME_POWER_ON		100	/* 100 msec */
#define USB_TIME_RESET			50	/* 50 msec */
#define USB_TIME_RESET_RECOVERY 	10	/* 10 msec */
#define USB_TIME_SET_ADDRESS		2	/* 2 msec */
#define USB_TIME_REQUEST		5000	/* 5 seconds */
#define USB_TIME_RESUME 		20	/* 20 msec */


/* USB packet identifiers */

#define USB_PID_SETUP			0x2d
#define USB_PID_OUT			0xe1
#define USB_PID_IN			0x69


/* packet size range */

#define USB_MIN_CTRL_PACKET_SIZE	8   /* min for control pipe */
#define USB_MAX_CTRL_PACKET_SIZE	64  /* max for control pipe */


/* maximum bandwidth (expressed as nanoseconds per frame) which may be
 * allocated for various purposes.
 */

#define USB_LIMIT_ISOCH_INT		((UINT32) 900000L)  /* 900 usec */
#define USB_LIMIT_ALL			((UINT32) 999000L)  /* 999 usec */


/* Typedefs */

/*
 * USB_SETUP
 */

typedef struct usb_setup
    {
    UINT8 requestType;		    /* bmRequestType */
    UINT8 request;		    /* bRequest */
    UINT16 value;		    /* wValue */
    UINT16 index;		    /* wIndex */
    UINT16 length;		    /* wLength */
    } USB_SETUP, *pUSB_SETUP;


/*
 * USB_DESCR_HDR
 *
 * header common to all USB descriptors
 */

typedef struct usb_descr_hdr
    {
    UINT8 length;		    /* bLength */
    UINT8 descriptorType;	    /* bDescriptorType */
    } USB_DESCR_HDR, *pUSB_DESCR_HDR;

#define USB_DESCR_HDR_LEN       2

/*
 * USB_DESCR_HDR_SUBHDR
 *
 * Some classes of USB devices (e.g., audio) use descriptors with a
 * header and a subheader.
 */

typedef struct usb_descr_hdr_subhdr
    {
    UINT8 length;		    /* bLength */
    UINT8 descriptorType;	    /* bDescriptorType */
    UINT8 descriptorSubType;	    /* bDescriptorSubType */
    } USB_DESCR_HDR_SUBHDR, *pUSB_DESCR_HDR_SUBHDR;


/*
 * USB_DEVICE_DESCR
 */

typedef struct usb_device_descr
    {
    UINT8 length;		    /* bLength */
    UINT8 descriptorType;	    /* bDescriptorType */
    UINT16 bcdUsb;		    /* bcdUSB - USB release in BCD */
    UINT8 deviceClass;		    /* bDeviceClass */
    UINT8 deviceSubClass;	    /* bDeviceSubClass */
    UINT8 deviceProtocol;	    /* bDeviceProtocol */
    UINT8 maxPacketSize0;	    /* bMaxPacketSize0 */
    UINT16 vendor;		    /* idVendor */
    UINT16 product;		    /* idProduct */
    UINT16 bcdDevice;		    /* bcdDevice - dev release in BCD */
    UINT8 manufacturerIndex;	    /* iManufacturer */
    UINT8 productIndex; 	    /* iProduct */
    UINT8 serialNumberIndex;	    /* iSerialNumber */
    UINT8 numConfigurations;	    /* bNumConfigurations */
    } WRS_PACK_ALIGN(4) USB_DEVICE_DESCR, *pUSB_DEVICE_DESCR;

#define USB_DEVICE_DESCR_LEN	18


/*
 * USB_CONFIG_DESCR
 */

typedef struct usb_config_descr
    {
    UINT8 length;		    /* bLength */
    UINT8 descriptorType;	    /* bDescriptorType */
    UINT16 totalLength; 	    /* wTotalLength */
    UINT8 numInterfaces;	    /* bNumInterfaces */
    UINT8 configurationValue;	    /* bConfigurationValue */
    UINT8 configurationIndex;	    /* iConfiguration */
    UINT8 attributes;		    /* bmAttributes */
    UINT8 maxPower;		    /* MaxPower */
    } WRS_PACK_ALIGN(4) USB_CONFIG_DESCR, *pUSB_CONFIG_DESCR;

#define USB_CONFIG_DESCR_LEN	9


/*
 * USB_INTERFACE_DESCR
 */

typedef struct usb_interface_descr
    {
    UINT8 length;		    /* bLength */
    UINT8 descriptorType;	    /* bDescriptorType */
    UINT8 interfaceNumber;	    /* bInterfaceNumber */
    UINT8 alternateSetting;	    /* bAlternateSetting */
    UINT8 numEndpoints; 	    /* bNumEndpoints */
    UINT8 interfaceClass;	    /* bInterfaceClass */
    UINT8 interfaceSubClass;	    /* bInterfaceSubClass */
    UINT8 interfaceProtocol;	    /* bInterfaceProtocol */
    UINT8 interfaceIndex;	    /* iInterface */
    } USB_INTERFACE_DESCR, *pUSB_INTERFACE_DESCR;

#define USB_INTERFACE_DESCR_LEN 9


/*
 * USB_ENDPOINT_DESCR
 */

typedef struct usb_endpoint_descr
    {
    UINT8 length;		    /* bLength */
    UINT8 descriptorType;	    /* bDescriptorType */
    UINT8 endpointAddress;	    /* bEndpointAddress */
    UINT8 attributes;		    /* bmAttributes */
    UINT16 maxPacketSize;	    /* wMaxPacketSize */
    UINT8 interval;		    /* bInterval */
    } WRS_PACK_ALIGN(1) USB_ENDPOINT_DESCR, *pUSB_ENDPOINT_DESCR;

#define USB_ENDPOINT_DESCR_LEN	7


/*
 * USB_LANGUAGE_DESCR
 */

typedef struct usb_language_descr
    {
    UINT8 length;		    /* bLength */
    UINT8 descriptorType;	    /* bDescriptorType */
    UINT16 langId [1];		    /* wLANGID[] - variable len */
    } USB_LANGUAGE_DESCR, *pUSB_LANGUAGE_DESCR;


/*
 * USB_STRING_DESCR
 */

typedef struct usb_string_descr
    {
    UINT8 length;		    /* bLength */
    UINT8 descriptorType;	    /* bDescriptorType */
    UINT8 string [1];		    /* bString - variable len */
    } USB_STRING_DESCR, *pUSB_STRING_DESCR;


/*
 * USB_HID_DESCR
 */

typedef struct usb_descr_typlen
    {
    UINT8 type; 		    /* bDescriptorType */
    UINT16 length;		    /* wDescriptorLength */
    } USB_DESCR_TYPLEN;

typedef struct usb_hid_descr
    {
    UINT8 length;		    /* bLength */
    UINT8 descriptorType;	    /* bDescriptorType */
    UINT16 bcdHid;		    /* bcdHID */
    UINT8 countryCode;		    /* bCountryCode */
    UINT8 numDescriptors;	    /* bNumDescriptors */
    USB_DESCR_TYPLEN descriptor [1]; /* (variable length) */
    } USB_HID_DESCR, *pUSB_HID_DESCR;


/*
 * USB_HUB_DESCR
 */

typedef struct usb_hub_descr
    {
    UINT8 length;		    /* bLength */
    UINT8 descriptorType;	    /* bDescriptorType */
    UINT8 nbrPorts;		    /* bNbrPorts */
    UINT16 hubCharacteristics;	    /* wHubCharacteristics */
    UINT8 pwrOn2PwrGood;	    /* bPwrOn2PwrGood */
    UINT8 hubContrCurrent;	    /* bHubContrCurrent */
    UINT8 deviceRemovable [1];	    /* DeviceRemovable - variable len */
    UINT8 portPwrCtrlMask [1];	    /* portPwrCtrlMask - variable len */
    } WRS_PACK_ALIGN(4) USB_HUB_DESCR, *pUSB_HUB_DESCR;

#define USB_HUB_DESCR_LEN   9


/*
 * USB_STANDARD_STATUS
 */

typedef struct usb_standard_status
    {
    UINT16 status;		    /* status word */
    } USB_STANDARD_STATUS, *pUSB_STANDARD_STATUS;


/*
 * USB_HUB_STATUS
 */

typedef struct usb_hub_status
    {
    UINT16 status;		    /* wHubStatus or wPortStatus*/
    UINT16 change;		    /* wHubChange or wPortChange*/
    } USB_HUB_STATUS, *pUSB_HUB_STATUS;


/* Implementation-specific host definitions */

/* defines */

/* node types */

#define USB_NODETYPE_NONE	0
#define USB_NODETYPE_HUB	1
#define USB_NODETYPE_DEVICE	2


/* node speeds */

#define USB_SPEED_FULL		0   /* 12 mbit device */
#define USB_SPEED_LOW		1   /* low speed device (1.5 mbit) */


/* standard endpoints */

#define USB_ENDPOINT_CONTROL	0


/* transfer (pipe) types */

#define USB_XFRTYPE_CONTROL	1
#define USB_XFRTYPE_ISOCH	2
#define USB_XFRTYPE_INTERRUPT	3
#define USB_XFRTYPE_BULK	4


/* direction indicators for IRPs */

#define USB_DIR_OUT		1
#define USB_DIR_IN		2
#define USB_DIR_INOUT		3


/* data toggle specifiers for USB_IRP */

#define USB_DATA0		0
#define USB_DATA1		1


/* flags for IRPs */

#define USB_FLAG_SHORT_OK	0x00
#define USB_FLAG_SHORT_FAIL	0x01
#define USB_FLAG_ISO_ASAP	0x02


/* IRP timeouts */

#define USB_TIMEOUT_DEFAULT	5000	    /* 5 seconds */
#define USB_TIMEOUT_NONE	0xffffffff  /* no timeout */


/* typedefs */

/*
 * IRP_CALLBACK
 */

typedef VOID (*IRP_CALLBACK) (pVOID pIrp);


/*
 * USB_BFR_LIST
 */

typedef struct usb_bfr_list
    {
    UINT16 pid; 		/* Specifies packet type as USB_PID_xxxx */
    pUINT8 pBfr;		/* Pointer to bfr */
    UINT32 bfrLen;		/* Length of buffer */
    UINT32 actLen;		/* actual length transferred */
    } USB_BFR_LIST, *pUSB_BFR_LIST;
    

/*
 * USB_IRP
 *
 * NOTE: There are certain requirements on the layout of buffers described
 * in the bfrList[].  
 *
 * For control transfers, the first bfrList [] entry must be the Setup packet.
 * If there is a data stage, the bfrList [] entry for the data stage should
 * follow.  Finally, a zero-length bfrList [] entry must follow which serves
 * as a place-holder for the status stage.
 *
 * For isochronous, interrupt, and bulk transfers there may be one or more
 * bfrList[] entries.
 *
 * If there is more than one bfrList[] entry for an isochronous, interrupt, 
 * or bulk transfers or more than two bfrList [] entries for control 
 * transfers, then each bfrList[].bfrLen (except the last) must be an exact
 * multiple of the maxPacketSize.  The HCD and underlying hardware will 
 * make no attempt to gather (during OUT) or scatter (during IN) a single
 * USB packet across multiple bfrList[] entries.
 */

typedef struct usb_irp
    {
    LINK usbdLink;		/* Link field used internally by USBD */
    pVOID usbdPtr;		/* Ptr field for use by USBD */
    LINK hcdLink;		/* Link field used internally by USB HCD */
    pVOID hcdPtr;		/* Ptr field for use by USB HCD */
    pVOID userPtr;		/* Ptr field for use by client */
    UINT16 irpLen;		/* Total length of IRP structure */
    int result; 		/* IRP completion result: S_usbHcdLib_xxxx */
    IRP_CALLBACK usbdCallback;	/* USBD completion callback routine */
    IRP_CALLBACK userCallback;	/* client's completion callback routine */
    UINT16 dataToggle;		/* IRP should start with DATA0/DATA1. */
    UINT16 flags;		/* Defines other IRP processing options */
    UINT32 timeout;		/* IRP timeout in milliseconds */
    UINT16 startFrame;		/* Start frame for isoch transfer */
    UINT16 dataBlockSize;	/* Data granularity for isoch transfer */
    UINT32 transferLen; 	/* Total length of data to be transferred */
    UINT16 bfrCount;		/* Indicates count of buffers in BfrList */
    USB_BFR_LIST bfrList [1];
    } USB_IRP, *pUSB_IRP;


/* Implementation-specific target definitions */

/*
 * ERP_CALLBACK
 */

typedef VOID (*ERP_CALLBACK) (pVOID pErp);


/*
 * USB_ERP
 *
 * The USB_ERP is similar to the USB_IRP, except that it applies to data transfers
 * as viewed from the perspective of a USB device - as opposed to the USB_IRP, which
 * is used by the USB host.
 *
 * Each USB_ERP (USB Endpoint Request Packet) describes a transfer through a device
 * endpoint.  As with USB_IRPs, the USB_ERP carries a bfrList[] which is an array
 * of USB_BFR_LIST structures - each describing a block of data to be transferred.
 * 
 * There are certain limitations on the bfrList[].  If the first entry in a bfrList[]
 * has a PID of USB_PID_SETUP, then there may be only a single bfrList[] entry.
 * 
 * bfrList[] entries must be all IN or all OUT.  A single ERP cannot describe 
 * transfers in both directions.
 */

typedef struct usb_erp
    {
    LINK targLink;		/* Link field used internally by usbTargLib */
    pVOID targPtr;		/* Ptr field for use by usbTargLib */
    LINK tcdLink;		/* Link field used internally by USB TCD */
    pVOID tcdPtr;		/* Ptr field for use by USB TCD */
    pVOID userPtr;		/* Ptr field for use by client */
    UINT16 erpLen;		/* Total length of ERP structure */
    int result; 		/* ERP completion result: S_usbTcdLib_xxxx */
    ERP_CALLBACK targCallback;	/* usbTargLib completion callback routine */
    ERP_CALLBACK userCallback;	/* client's completion callback routine */
    UINT16 endpointId;		/* device endpoint */
    UINT16 transferType;	/* Type of ERP: control, bulk, etc. */
    UINT16 dataToggle;		/* ERP should start with DATA0/DATA1. */
    UINT16 bfrCount;		/* Indicates count of buffers in BfrList */
    USB_BFR_LIST bfrList [1];
    } USB_ERP, *pUSB_ERP;


#ifdef	__cplusplus
}
#endif

#endif	/* __INCusbh */


/* End of file. */

