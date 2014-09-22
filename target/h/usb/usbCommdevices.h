/* usbCommdevices.h - Class-specific definitions for USB Communications class */

/* Copyright 2000-2001 Wind River Systems, Inc. */

/*
Modification history
--------------------

01a,02may00,vis  created.

*/

#ifndef __INCusbCommDevicesh
#define __INCusbCommDevicesh

#ifdef	__cplusplus
extern "C" {
#endif  /* __cplusplus */


/* defines */

/* Communications Device Class code */

#define USB_CLASS_COMMDEVICE		0x02

/* Communications Interface Class code */

#define USB_CLASS_COMMINTERFACE		0x02

/* Data Interface Class code */

#define USB_CLASS_DATAINTERFACE		0x0a


/* Communications Interface Class Subclass codes */

#define USB_SUBCLASS_DLCM		0x01	/* Direct Line Control Model */
#define USB_SUBCLASS_ACM		0x02	/* Abstract Control Model */
#define USB_SUBCLASS_TCM		0x03	/* Telephone Control Model */
#define USB_SUBCLASS_MCCM		0x04	/* MultiChannel Control Model */
#define USB_SUBCLASS_CAPI		0x05	/* CAPI Control Model */
#define USB_SUBCLASS_ENET		0x06	/* Ethernet Control Model */
#define USB_SUBCLASS_ATM		0x07	/* ATM Control Model */

/* Data Interface Class Subclass codes */

#define USB_SUBCLASS_DATA		0x00	/* Currently not used */


/* Communication Interface Class Control Protocol codes */

#define USB_COMM_PROTOCOL_NONE		0x00	/* No protocol required */
#define USB_COMM_PROTOCOL_COMMONAT	0x01	/* common AT commands (hayes */
						/* compatible ) */
#define USB_COMM_PROTOCOL_VENDOR	0xff	/* Vendor Specific protocol */


/* Data Interface Class Control Protocol codes */

#define USB_COMM_PROTOCOL_ISDN		0x30	/* Physical interface protocol*/
						/* for ISDN BRI */
#define USB_COMM_PROTOCOL_HDLC		0x31	/* HDLC */
#define USB_COMM_PROTOCOL_TRANSPARENT	0x32	/* None.. */
#define USB_COMM_PROTOCOL_Q921M		0x50	/* Management protocol for */
						/* Q.921 data link protocol */
#define USB_COMM_PROTOCOL_Q921		0x51	/* data link protocol for */
						/* Q.921  */
#define USB_COMM_PROTOCOL_Q921T		0x52	/* TEI multiplexor for */
						/* Q.921 data link protocol */
#define USB_COMM_PROTOCOL_V42BIS	0x90	/* Data compression procedures */
#define USB_COMM_PROTOCOL_Q931		0x91	/* Q.931 or Euro ISDN */
#define USB_COMM_PROTOCOL_V120		0x92	/* V.24 adaptation to ISDN */
#define USB_COMM_PROTOCOL_CAPI20	0x93	/* CAPI Commands */
#define USB_COMM_PROTOCOL_HBD		0xfd	/* Host based driver */
#define USB_COMM_PROTOCOL_CDC		0xfe	/* CDC Specification */



#ifdef	__cplusplus
}
#endif	/* __cplusplus */

#endif	/* __INCusbCommDevicesh */
