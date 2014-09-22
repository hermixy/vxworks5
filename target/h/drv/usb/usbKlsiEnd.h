/* usbKlsiEnd.h - USB Klsi End driver header */

/* Copyright 2000-2001 Wind River Systems, Inc. */

/*
Modification history
--------------------
01h,07may01,wef  undid 01f
01g,07may01,wef  changed module number to be (module num << 8) | M_usbHostLib
01f,03may01,wef  changed pUsbListDev to pUSB_KLSI_DEV
01e,02may01,wef  changed module number to be M_<module> + M_usbHostLib
01d,30apr01,wef  changed USB_DEV struct to USB_KLSI_DEV
01c,05dec00,wef  moved Module number defs to vwModNum.h - add this
                 to #includes
02b,11aug00,sab  KLSI_DEVICE structure modified, added ENET_IRP structure.
01a,02may00,vis  Created
*/

#ifndef __INCusbklsiEndh
#define __INCusbklsiEndh

#ifdef	__cplusplus
extern "C" {
#endif  /* __cplusplus */


/* includes */

#include "endLib.h"
#include "usb/usbEnet.h"
#include "vwModNum.h"           /* USB Module Number Def's */


/* defines */

/* Error Numbers as set the usbEnetLib  */

/* usbEnetLib error values */

/* 
 * USB errnos are defined as being part of the USB host Module, as are all
 * vxWorks module numbers, but the USB Module number is further divided into 
 * sub-modules.  Each sub-module has upto 255 values for its own error codes
 */
 
#define USB_KLSI_SUB_MODULE  13

#define M_usbKlsiLib 	( (USB_KLSI_SUB_MODULE  << 8) | M_usbHostLib )

#define usbKlsiErr(x)	(M_usbKlsiLib | (x))

#define S_usbKlsiLib_NOT_INITIALIZED	usbKlsiErr (1)
#define S_usbKlsiLib_BAD_PARAM	    	usbKlsiErr (2)
#define S_usbKlsiLib_OUT_OF_MEMORY	usbKlsiErr (3)
#define S_usbKlsiLib_OUT_OF_RESOURCES	usbKlsiErr (4)
#define S_usbKlsiLib_GENERAL_FAULT	usbKlsiErr (5)
#define S_usbKlsiLib_QUEUE_FULL	    	usbKlsiErr (6)
#define S_usbKlsiLib_QUEUE_EMPTY	usbKlsiErr (7)
#define S_usbKlsiLib_NOT_IMPLEMENTED	usbKlsiErr (8)
#define S_usbKlsiLib_USBD_FAULT	    	usbKlsiErr (9)
#define S_usbKlsiLib_NOT_REGISTERED	usbKlsiErr (10)
#define S_usbKlsiLib_NOT_LOCKED	    	usbKlsiErr (11)



/* Klsi specific constants */

/* List of Vendor Ids and Product Ids of devices employing KLSI adapter */

#define NETGEAR_VENDOR_ID		0x0846
#define NETGEAR_PRODUCT_ID		0x1001

#define SMC_VENDOR_ID			0x0707
#define SMC_PRODUCT_ID			0x0100


/* Packet Filter Bitmap Constants */

#define PACKET_TYPE_MULTICAST		0x10
#define PACKET_TYPE_BROADCAST		0x08
#define PACKET_TYPE_DIRECTED		0x04
#define PACKET_TYPE_ALL_MULTICAST	0x02
#define PACKET_TYPE_PROMISCOUS		0x01


/* Vendor Specific Commands */

#define USB_REQ_KLSI_ETHDESC_GET	0x00
#define USB_REQ_KLSI_SET_MCAST_FILTER	0x01
#define USB_REQ_KLSI_SET_PACKET_FILTER	0x02
#define USB_REQ_KLSI_GET_STATS		0x03
#define USB_REQ_KLSI_GET_AUX_INPUTS	0x04
#define USB_REQ_KLSI_SET_AUX_OUTPUTS	0x05
#define	USB_REQ_KLSI_SET_TEMP_MAC	0x06
#define	USB_REQ_KLSI_GET_TEMP_MAC	0x07
#define	USB_REQ_KLSI_SET_URB_SIZE	0x08
#define	USB_REQ_KLSI_SET_SOFS_TO_WAIT	0x09
#define	USB_REQ_KLSI_SET_EVEN_PACKETS	0x0a
#define	USB_REQ_KLSI_SCAN		0xff


/* Offsets for reading the Ethernet Functional Descriptor */

#define KLSI_OFFSET_MACADRS		3
#define KLSI_OFFSET_STAT_BMP		9
#define KLSI_OFFSET_SEGMENT_SIZE	13
#define KLSI_OFFSET_MCAST		15

/* Buffer Sizes */

#define KLSI_OUT_BFR_SIZE       1550	    	/* size of output bfr */
#define KLSI_IN_BFR_SIZE        1550		/* size of input bfr */
#define KLSI_M_BULK_NUM		512		/* number of mbulks */ 
#define KLSI_CL_NUMBER   	256		/* number of clusters */


#define KLSI_INTERRUPT_TO_USE	100		/* interrupt used in */
						/* downloading firmware */
#define USB_KLSI_ATTACH	                0      /* KLSI Device attached */
#define USB_KLSI_REMOVE	                1      /* KLSI Device removed  */



/* typedefs */

/* 
 * usb Device structure 
 * This structure is a priliminary structure and is used in the 
 * call back functions for dynamic attachment.
 * Later when the final end_obj structure is created, this structure
 * will be linked to the end_obj structure.
 */ 

typedef struct usb_klsi_dev 
    {
    LINK 		devLink;	/* linked list of device structs */    
    USBD_NODE_ID 	nodeId;		/* Node Id of the device */
    VOID * 		pDevStructure;	/* link to KLSI_DEVICE Structure */
    UINT16		configuration;	/* configuration of the device */
    UINT16 		interface;	/* interface of the device */
    UINT16 		vendorId;	/* vendor identification number */	
    UINT16 		productId;	/* product identification number */
    BOOL                connected;      /* TRUE if KLSI device connected  */ 
    UINT16              lockCount;      /* Count of times structure locked */
 
    }USB_KLSI_DEV, *pUsbListDev;


/* 
 * Statistics, to be maintained by the device/driver 
 * As on today all the stats are not supported by the driver.
 */

typedef struct klsi_stats
    {

    UINT32 bitmap;	/* bitmap indicating the supported stats */
			/* This bitmap will be in the same way as given */
			/* in the Function descriptor of the device. */
			/* Corresponding bit will be set if the stat is */
			/* supported both by the device and the driver */

    UINT32 xmitOk;	/* Frames Transmitted without errors. XMIT_OK */
    UINT32 rvcOk;	/* Frames received without errors. RVC_OK */
    UINT32 xmitErr;	/* Frames not transmitted or transmitted with errors */
    UINT32 rvcErr;	/* Frames recd. with errors that are not delivered */
			/* to usb Host */
    UINT32 rvcNoBuf;	/* Frames missed, nobuffers */
    UINT32 rvcCrcErr;	/* Frames recd. with CRC or FCS error */
    UINT32 rvcOverRun;	/* Frames not recd. due to over run */

    }KLSI_STATS, *pKlsiStats;

/* 
 * the multicast filter support details. 
 */

typedef struct klsi_mcast
    {

    UINT8  isMCastPerfect;	/* if TRUE, the device supports perfect */
				/* multicast address filtering (no hashing) */

    UINT16  noMCastFilters;	/* No.of Multicast Address filters Supported */

    }KLSI_MCAST_DETAILS, *pKlsiMCastDetails;

/* 
 * Irp buffer structure
 */

 typedef struct klsi_enet_irp
	{
	USB_IRP outIrp;		    /* IRP to transmit output data */
   	BOOL outIrpInUse;	    /* TRUE while IRP is outstanding */
	} KLSI_ENET_IRP; 
	 

/* 
 * Usb Ethernet END Device structure.. 
 * This structure is used both by the file usbKlsiEnd.c.
 */

typedef struct klsi_device
    {
    END_OBJ endObj;		     /* must be first field */

    USB_KLSI_DEV * pDev;	     /* the device info */

    UINT8 unit;

    UINT8 communicateOk;	    /* TRUE after Starting */
				    /* and FALSE if stopped */    

    UINT8 macAdrs[6];		    /* MAC adress */

    CL_POOL_ID pClPoolId;	    /* Pointer to the Cluster Pool */

    BOOL connected;		    /* TRUE if device is currently connected */

    USBD_PIPE_HANDLE outPipeHandle; /* USBD pipe handle for bulk OUT pipe */
	
    KLSI_ENET_IRP * pEnetIrp;	    /* pointer to details of Irp structure*/
    int noOfIrps;		    /* no of Irps */		

    int	 txIrpIndex;		    /* What the last submitted IRP is */

    UINT8 * outBfr;		    /* pointer to output buffer */
    UINT16 outBfrLen;		    /* size of output buffer */
    UINT32 outErrors;		    /* count of IRP failures */

    USBD_PIPE_HANDLE inPipeHandle;  /* USBD pipe handle for bulk IN pipe */
    USB_IRP inIrp;		    /* IRP to monitor input from printer */
    BOOL inIrpInUse;		    /* TRUE while IRP is outstanding */
		     
    UINT8 **	pInBfrArray;	     /* pointer to input buffers */	
    int		noOfInBfrs;	     /* no of input buffers*/
    int         rxIndex;             /* where current buffer is */

    UINT16 inBfrLen;		    /* size of input buffer */
    UINT32 inErrors;		    /* count of IRP failures */

    KLSI_STATS stats;	    /* Statistics */

    KLSI_MCAST_DETAILS mCastFilters; /* Multicast address filter details */

    UINT16 maxSegmentSize;	    /* Max. Segment supported by the device */

    UINT8 maxPower;		    /* Max. Power Consumption of the device */
				    /* in 2mA units */

    } KLSI_DEVICE, *pKlsiDevCtrl;


/* USB_KLSI_ATTACH_CALLBACK defines a callback routine which will be
 * invoked by usbKlsiEnd.c when the attachment or removal of a KLSI
 * device is detected.  When the callback is invoked with an attach code of
 * USB_KLSI_ATTACH, the nodeId represents the ID of newly added device.  When
 * the attach code is USB_KLSI_REMOVE, nodeId points to the KLSI device which 
 * is no longer attached.
 */

typedef VOID (*USB_KLSI_ATTACH_CALLBACK) 
    (
    pVOID arg,           /* caller-defined argument     */
    USBD_NODE_ID nodeId, /* nodeId of the KLSI device    */
    UINT16 attachCode    /* attach or remove code       */
    );


LOCAL UINT16 klsiAdapterList[][2] = {
			 	{ NETGEAR_VENDOR_ID, NETGEAR_PRODUCT_ID },			 	
				{ SMC_VENDOR_ID, SMC_PRODUCT_ID }
			       };

/* function prototypes*/

STATUS usbKlsiDynamicAttachRegister ( USB_KLSI_ATTACH_CALLBACK callback, 
                                        pVOID arg);
STATUS usbKlsiDynamicAttachUnregister ( USB_KLSI_ATTACH_CALLBACK callback,
                                          pVOID arg);
STATUS usbKlsiDevLock (USBD_NODE_ID nodeId);
STATUS usbKlsiDevUnlock (USBD_NODE_ID nodeId);



#ifdef	__cplusplus
}
#endif	/* __cplusplus */

#endif	/* __INCusbKlsiEndh */
