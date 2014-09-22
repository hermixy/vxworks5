/* usbPrinter.h - Class-specific definitions for USB printers */

/* Copyright 2000 Wind River Systems, Inc. */

/*
Modification history
--------------------
01a,,rcb  First.
*/

#ifndef __INCusbPrinterh
#define __INCusbPrinterh

#ifdef	__cplusplus
extern "C" {
#endif


/* defines */

/* USB PRINTER subclass codes */

#define USB_SUBCLASS_PRINTER	0x01


/* USB PRINTER protocol codes */

#define USB_PROTOCOL_PRINTER_UNIDIR	0x01
#define USB_PROTOCOL_PRINTER_BIDIR	0x02


/* USB PRINTER class-specific requests */

#define USB_REQ_PRN_GET_DEVICE_ID	0x00
#define USB_REQ_PRN_GET_PORT_STATUS	0x01
#define USB_REQ_PRN_SOFT_RESET		0x02


/* USB_PRINTER_PORT_STATUS.status bit definitions */

#define USB_PRN_STS_PAPER_EMPTY 	0x20
#define USB_PRN_STS_SELECTED		0x10
#define USB_PRN_STS_NOT_ERROR		0x08


#define USB_PRN_MAX_DEVICE_ID_LEN   256


/* typedefs */

/* USB_PRINTER_CAPABILITIES
 *
 * NOTE: The USB_PRINTER_CAPABILITIES structure matches that of the IEEE-1284
 * "device ID" string.
 */

typedef struct usb_printer_capabilities
    {
    UINT16 length;		    /* length of string, big-endian */
    UINT8 caps [1];		    /* variable length string */
    } USB_PRINTER_CAPABILITIES, *pUSB_PRINTER_CAPABILITIES;


/* USB_PRINTER_PORT_STATUS */

typedef struct usb_printer_port_status
    {
    UINT8 status;
    } USB_PRINTER_PORT_STATUS, *pUSB_PRINTER_PORT_STATUS;


#ifdef	__cplusplus
}
#endif

#endif	/* __INCusbPrinterh */


/* End of file. */

