/* usbNC1080End.h - USB Netchip End driver header */

/* Copyright 2000-2001 Wind River Systems, Inc. */

/*
Modification history
--------------------
01d,07may01,wef  changed module number to be (module num << 8) | M_usbHostLib
01c,03may01,wef  changed pUsbListDev to USB_NC1080_DEV, moved attach_request 
		 definition to .c file from .h file.
01b,02may01,wef  changed module number to be M_<module> + M_usbHostLib
01a,02may00,bri  Created
*/

#ifndef __INCusbNC1080Endh
#define __INCusbNC1080Endh

#ifdef	__cplusplus
extern "C" {
#endif  /* __cplusplus */


/* includes */

#include "vxWorks.h"
#include "usb/usbEnet.h"
#include "endLib.h"

/* defines */

/* vendor and product id's */

#define NETCHIP_PRODUCT_ID	0x1080
#define NETCHIP_VENDOR_ID	0x0525


/* NetChip Register Definitions */

#define NETCHIP_USBCTL		((UINT8)0x04)	    /* USB Control */
#define NETCHIP_TTL		    ((UINT8)0x10)       /* Time to Live */
#define NETCHIP_STATUS		((UINT8)0x11)	    /* Status */

#define NETCHIP_REQ_REGISTER_READ ((UINT8)0x10)	/* bRequest */

#define NETCHIP_TTLVAL 		255	                /* msec*/

/* Control Register */

#define	USBCTL_WRITABLE_MASK	0x1f0f
/* bits 15-13 reserved */
#define	USBCTL_ENABLE_LANG	(1 << 12)
#define	USBCTL_ENABLE_MFGR	(1 << 11)
#define	USBCTL_ENABLE_PROD	(1 << 10)
#define	USBCTL_ENABLE_SERIAL	(1 << 9)
#define	USBCTL_ENABLE_DEFAULTS	(1 << 8)
/* bits 7-4 reserved */
#define	USBCTL_FLUSH_OTHER	(1 << 3)
#define	USBCTL_FLUSH_THIS	(1 << 2)
#define	USBCTL_DISCONN_OTHER	(1 << 1)
#define	USBCTL_DISCONN_THIS	(1 << 0)

 /* Status register */

#define	STATUS_PORT_A		(1 << 15)

#define	STATUS_CONN_OTHER	(1 << 14)
#define	STATUS_SUSPEND_OTHER	(1 << 13)
#define	STATUS_MAILBOX_OTHER	(1 << 12)
#define	STATUS_PACKETS_OTHER(n)	(((n) >> 8) && 0x03)

#define	STATUS_CONN_THIS	(1 << 6)
#define	STATUS_SUSPEND_THIS	(1 << 5)
#define	STATUS_MAILBOX_THIS	(1 << 4)
#define	STATUS_PACKETS_THIS(n)	(((n) >> 0) && 0x03)

#define	STATUS_UNSPEC_MASK	0x0c8c
#define	STATUS_NOISE_MASK 	((u16)~(0x0303|STATUS_UNSPEC_MASK))



/* Configuration items */

#define NETCHIP_MTU		296		

#define NETCHIP_BUFSIZ      	(NETCHIP_MTU + ENET_HDR_REAL_SIZ + 6)	
#define EH_SIZE			(14)

#define NETCHIP_SPEED_10M	10000000	/* 10Mbs */
#define NETCHIP_SPEED_100M	100000000	/* 100Mbs */
#define NETCHIP_SPEED       	NETCHIP_SPEED_10M

/* A shortcut for getting the hardware address from the MIB II stuff. */

#define NETCHIP_HADDR(pEnd)	\
		((pEnd)->mib2Tbl.ifPhysAddress.phyAddress)

#define NETCHIP_HADDR_LEN(pEnd) \
		((pEnd)->mib2Tbl.ifPhysAddress.addrLength)

#define CLIENT_NAME	"usbNC1080Lib"

#define NETCHIP_NAME    "netChip"
#define NETCHIP_NAME_LEN (sizeof(NETCHIP_NAME))

/* Buffer definitions */

#define NETCHIP_NUM_IN_BFRS	10
#define NETCHIP_NUM_OUT_BFRS	10

#define NETCHIP_OUT_BFR_SIZE	1000
#define NETCHIP_IN_BFR_SIZE	1000

/* Error Numbers as set the usbNC1080Lib  */

/* usbEnetLib error values */

/* 
 * USB errnos are defined as being part of the USB host Module, as are all
 * vxWorks module numbers, but the USB Module number is further divided into 
 * sub-modules.  Each sub-module has upto 255 values for its own error codes
 */
 
#define USB_NC1080_SUB_MODULE  	15

#define M_usbNC1080Lib 		( (USB_NC1080_SUB_MODULE  << 8) | M_usbHostLib )

#define usbNetChipErr(x)	(M_usbNC1080Lib | (x))

#define S_usbNC1080Lib_NOT_INITIALIZED		usbNetChipErr (1)
#define S_usbNC1080Lib_BAD_PARAM	    	usbNetChipErr (2)
#define S_usbNC1080Lib_OUT_OF_MEMORY		usbNetChipErr (3)
#define S_usbNC1080Lib_OUT_OF_RESOURCES		usbNetChipErr (4)
#define S_usbNC1080Lib_GENERAL_FAULT		usbNetChipErr (5)
#define S_usbNC1080Lib_QUEUE_FULL	    	usbNetChipErr (6)
#define S_usbNC1080Lib_QUEUE_EMPTY		usbNetChipErr (7)
#define S_usbNC1080Lib_NOT_IMPLEMENTED		usbNetChipErr (8)
#define S_usbNC1080Lib_USBD_FAULT	    	usbNetChipErr (9)
#define S_usbNC1080Lib_NOT_REGISTERED		usbNetChipErr (10)
#define S_usbNC1080Lib_NOT_LOCKED	    	usbNetChipErr (11)

/* NETCHIP Device attached */ 
#define USB_NETCHIP_ATTACH			0

/* NETCHIP Device removed  */
#define USB_NETCHIP_REMOVE			1

/* typedefs */

/* 
 * usb Device structure 
 * This structure is a priliminary structure and is used in the 
 * call back functions for dynamic attachment.
 * Later when the final end_obj structure is created, this structure
 * will be linked to the end_obj structure.
 */ 

typedef struct usb_nc1080_dev
    {
    LINK devLink;		/* linked list of device structs */
    USBD_NODE_ID nodeId;	/* Node Id of the device */
    VOID * pDevStructure;	/* Net chip device structure*/
    UINT16 configuration;	/* configuration of the device */
    UINT16 interface;		/* interface of the device */

    UINT16 vendorId;		/* these three fields together */
    UINT16 productId;		/* uniquely */
    UINT16 serialNo;		/* identify a device */

    UINT8 maxPower;		    /* Max. Power Consumption of the device */
    BOOL  connected;		/* TRUE if device is connected */
    UINT16 lockCount;	

    } USB_NC1080_DEV;

/* The definition of the driver control structure */

typedef struct usb_nc1080_end_device
    {
    END_OBJ	endObj;			    /* The class we inherit from. */
    int		unit;			    /* unit number */
    long	flags;			    /* Our local flags. */
    UCHAR	enetAddr[6];		/* ethernet address */
    CL_POOL_ID	pClPoolId;		/* cluster pool */
    BOOL	rxHandling;		/* rcv task is scheduled */

    /* usb specifics */
    USB_NC1080_DEV * 	pDev;		    /* the device info */

    USBD_PIPE_HANDLE outPipeHandle;  /* USBD pipe handle for Tx pipe */
    USBD_PIPE_HANDLE inPipeHandle;   /* USBD pipe handle for Rx pipe */ 

    USB_IRP	outIrp;		     /* IRP to transmit output data */
    BOOL	outIrpInUse;	     /* TRUE while IRP is outstanding */

    USB_IRP	inIrp;		     /* IRP to monitor input from printer */
    BOOL	inIrpInUse;	     /* TRUE while IRP is outstanding */

    int		txBufIndex; 	     /* What the last submitted IRP is */
    int		rxBufIndex;        /* where current buffer is */

    int		noOfInBfrs;	 /* no of input buffers*/
    int		noOfIrps;	 /* no of Irps */

    UINT16	outBfrLen;	     	/* size of output buffer */
    UINT16	inBfrLen;		        /* size of input buffer */
	
    UINT8 **  pOutBfrArray;	 	/*pointer to Output buffers*/
    UINT8 **  pInBfrArray;	        /* pointer to input buffers */	

    int		txPkts;		/* used for packet_id of the Header */


    UINT32	outErrors;		/* count of IRP failures */
    UINT32	inErrors;		    	/* count of IRP failures */

    } NC1080_END_DEV;

/*
 * As mentioned earlier, the data needs to be packetized before sending 
 * to the Netchip1080 device for transmission. Also the received data
 * has to be de-packetized before we send it up. The packetizing protocol
 * is
 * -----------------------------
 * |header|    data    |footer|
 * -----------------------------
 * 
 * the header , footer format is given here 
 */

typedef struct netchip_header 
    {
    UINT16	hdr_len;		/* sizeof netchip_header */
    UINT16	packet_len;		/* packet size */
    UINT16	packet_id;		/* detects dropped packets */

    /* 
     * This is the minimum header required. If anything else is to be there,
     * the netchip_header struct should continue like..
     * UINT16	vendorId;
     * UINT16	productId;
     * ...
     */
    
    }NC_HEADER;

typedef struct netchip_footer 
    {

    UINT16	packet_id;		/* for cross checking */

    }NC_FOOTER;


typedef VOID (*USB_NETCHIP_ATTACH_CALLBACK) 
    (
    pVOID arg,           /* caller-defined argument     */
    USB_NC1080_DEV * pDev,	     /* structure of netchip device */
    UINT16 attachCode    /* attach or remove code       */
    );		             /*added for Multiple devices  */


#ifdef	__cplusplus
}
#endif	/* __cplusplus */

#endif	/* __INCusbNC1080Endh */
