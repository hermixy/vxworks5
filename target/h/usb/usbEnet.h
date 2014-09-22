/* usbEnet.h - Class-specific definitions for USB Ethernet Adapters */

/* Copyright 2000-2001 Wind River Systems, Inc. */

/*
Modification history
--------------------
01a,02may00,  vis  Created
*/

#ifndef __INCusbEneth
#define __INCusbEneth

#ifdef	__cplusplus
extern "C" {
#endif	/* __cplusplus */

/* includes */

#include "usb/usbCommdevices.h"
#include "usb/usbPlatform.h"
#include "usb/ossLib.h"
#include "usb/usbListLib.h"
#include "usb/usbQueueLib.h"
#include "usb/usbdLib.h"
#include "usb/usbdCoreLib.h"	


/* defines */

/* USB Ethernet Control Model Sub class-specific requests */

#define USB_ENET_REQ_SEND_ENCAP_COMMAND		0x00
#define USB_ENET_REQ_GET_ENCAP_RESPONSE		0x01
#define USB_ENET_REQ_SET_MCAST_FILTERS		0x40
#define USB_ENET_REQ_SET_POWMGMT_FILTER		0x41
#define USB_ENET_REQ_GET_POWMGMT_FILTER		0x42
#define USB_ENET_REQ_SET_PACKET_FILTER		0x43
#define USB_ENET_REQ_GET_ENET_STATISTIC		0x44


/* USB Ethernet Control Model Notification codes.*/

#define USB_ENET_NOTIFY_NETWORK_CONNECTION	0x00
#define USB_ENET_NOTIFY_RESPONSE_AVAILABLE	0x01
#define USB_ENET_NOTIFY_CONNECTION_SPD_CHANGE	0x2a


/* 
 * USB Ethernet Control Model - Statistics
 * These values are used in 2 places.
 * 1. In GetEthernetStatistic request, as feature selector
 * 2. In decifering the Function descriptor. 
 */

#define USB_ENET_STAT_XMIT_OK			1
#define USB_ENET_STAT_RVC_OK			2
#define USB_ENET_STAT_XMIT_ERROR		3
#define USB_ENET_STAT_RVC_ERROR			4
#define USB_ENET_STAT_RVC_NO_BUFFER		5
#define USB_ENET_STAT_DIRECTED_BYTES_XMIT	6
#define USB_ENET_STAT_DIRECTED_FRAMES_XMIT	7
#define USB_ENET_STAT_MULTICAST_BYTES_XMIT	8
#define USB_ENET_STAT_MULTICAST_FRAMES_XMIT	9
#define USB_ENET_STAT_BROADCAST_BYTES_XMIT	10
#define USB_ENET_STAT_BROADCAST_FRAMES_XMIT	11
#define USB_ENET_STAT_DIRECTED_BYTES_RCV	12
#define USB_ENET_STAT_DIRECTED_FRAMES_RCV	13
#define USB_ENET_STAT_MULTICAST_BYTES_RCV	14
#define USB_ENET_STAT_MULTICAST_FRAMES_RCV	15
#define USB_ENET_STAT_BROADCAST_BYTES_RCV	16
#define USB_ENET_STAT_BROADCAST_FRAMES_RCV	17
#define USB_ENET_STAT_RCV_CRC_ERROR		18
#define USB_ENET_STAT_TRANSMIT_QUEUE_LENGTH	19
#define USB_ENET_STAT_RCV_ERROR_ALIGNMENT	20
#define USB_ENET_STAT_XMIT_ONE_COLLISION	21
#define USB_ENET_STAT_XMIT_MORE_COLLISIONS	22
#define USB_ENET_STAT_XMIT_DEFERRED		23
#define USB_ENET_STAT_XMIT_MAX_COLLISIONS	24
#define USB_ENET_STAT_RCV_OVERRUN		25
#define USB_ENET_STAT_XMIT_UNDERRUN		26
#define USB_ENET_STAT_XMIT_HEARTBEAT_FAILURE	27
#define USB_ENET_STAT_XMIT_TIMES_CRS_LOST	28
#define USB_ENET_STAT_XMIT_LATE_COLLISIONS	29

/* here is how the Function descriptor is deciphered */

#define GetEnetStat(x)	(0x1<<(x-1))	


#define MCAST_FILTER_MASK	0x1000


/* 
 * USB Ethernet Control Model - Packet Filter Setup Options.
 * These are used in the SetEthernetPacketFilter request.
 * The Packet Filter is the inclusive OR ofthe below options
 */

#define USB_ENET_PKT_TYPE_PROMISCOUS		0x0001
#define USB_ENET_PKT_TYPE_ALL_MULTICAST		0x0002
#define USB_ENET_PKT_TYPE_DIRECTED		0x0004
#define USB_ENET_PKT_TYPE_BROADCAST		0x0008
#define USB_ENET_PKT_TYPE_MULTICAST		0x0010


/* USB Ethernet Control Model - Function descriptor */

typedef struct usbEnet_Func_descr
    {
    UINT8 length;		    /* bFunctionLength */
    UINT8 descriptorType;	    /* bDescriptorType */
    UINT8 descriptorSubType;	    /* bDescriptorType */
    UINT8 macAddressIndex;	    /* iMACAddress */
    UINT32 statisticsBitmap;	    /* bmEthernetStatistics */
    UINT16 maxSegmentSize;	    /* wMaxSegmentSize */
    UINT16 noOfMCastFilters;	    /* wNumbereMCFilters */
    UINT8 noOfPowerFilters;	    /* bNumberPowerFilters */
    } USB_ENET_FUNC_DESCR, *pUSB_ENET_FUNC_DESCR;

#define USB_ENET_FUNC_DESCR_LEN		13


#ifdef	__cplusplus
}
#endif  /* __cplusplus */

#endif	/* __INCusbEneth */
