/* usbdCoreLib.h - Defines internal between usbdLib and usbdCoreLib */

/* Copyright 2000 Wind River Systems, Inc. */

/*
Modification history
--------------------
01d,26jan00,rcb  Redefine "bandwidth" field in URB_PIPE_CREATE as UINT32.
01c,29nov99,rcb  Increase frame number fields to 32 bits in 
		 URB_CURRENT_FRAME_GET.
01b,07sep99,rcb  Add support for management callbacks and set-bus-state API.
01a,20aug99,rcb  First.
*/

#ifndef __INCusbdCoreLibh
#define __INCusbdCoreLibh

#ifdef	__cplusplus
extern "C" {
#endif


/* includes */

#include "usb/usbdLib.h"


/* defines */

/* USBD Function Request Codes - URB_HEADER.Function */

#define USBD_FNC_ASYNC_MASK		0x8000	/* Set if USBD function */
						/* execute asynchronously */

/* 
 * Synchronous USBD functions always execute synchronously.
 *
 * Asynchronous USBD functions may or may not execute asynchronously, 
 * depending on the USBD implementation.  However, clients must expect that 
 * such functions may execute asynchronously. 
 */

#define USBD_FNC_INITIALIZE		0x0000	/* initialize USBD */
#define USBD_FNC_SHUTDOWN		0x0001	/* shut down USBD */

#define USBD_FNC_CLIENT_REG		0x0100	/* Client register */
#define USBD_FNC_CLIENT_UNREG		0x0101	/* Client unregister */
#define USBD_FNC_MNGMT_CALLBACK_SET	0x0102	/* set client's mngmt callback */

#define USBD_FNC_VERSION_GET		0x0200	/* Get USBD version */
#define USBD_FNC_HCD_ATTACH		0x0201	/* Attach HCD to USBD */
#define USBD_FNC_HCD_DETACH		0x0202	/* Detach HCD from USBD */
#define USBD_FNC_STATISTICS_GET 	0x0203	/* Get USBD statistics */

#define USBD_FNC_BUS_COUNT_GET		0x0300	/* Get bus count */
#define USBD_FNC_ROOT_ID_GET		0x0301	/* Get root id */
#define USBD_FNC_HUB_PORT_COUNT_GET	0x0302	/* Get count of hub ports */
#define USBD_FNC_NODE_ID_GET		0x0303	/* Get node id */
#define USBD_FNC_NODE_INFO_GET		0x0304	/* Get node information */

#define USBD_FNC_DYNA_ATTACH_REG	0x0400	/* Dynamic attach register */
#define USBD_FNC_DYNA_ATTACH_UNREG	0x0401	/* Dynamic attach unregister */

#define USBD_FNC_FEATURE_CLEAR		0x8500	/* Clear feature */
#define USBD_FNC_FEATURE_SET		0x8501	/* Set feature */
#define USBD_FNC_CONFIG_GET		0x8502	/* Get configuration */
#define USBD_FNC_CONFIG_SET		0x8503	/* Set configuration */
#define USBD_FNC_DESCRIPTOR_GET 	0x8504	/* Get descriptor */
#define USBD_FNC_DESCRIPTOR_SET 	0x8505	/* Set descriptor */
#define USBD_FNC_INTERFACE_GET		0x8506	/* Get interface */
#define USBD_FNC_INTERFACE_SET		0x8507	/* Set interface */
#define USBD_FNC_STATUS_GET		0x8508	/* Get status */
#define USBD_FNC_ADDRESS_GET		0x0509	/* Get device address */
#define USBD_FNC_ADDRESS_SET		0x850a	/* Set device address */
#define USBD_FNC_VENDOR_SPECIFIC	0x850b	/* Vendor specific */

#define USBD_FNC_PIPE_CREATE		0x0600	/* Create pipe */
#define USBD_FNC_PIPE_DESTROY		0x0601	/* Destroy pipe */
#define USBD_FNC_TRANSFER		0x0602	/* Initiate transfer */
#define USBD_FNC_TRANSFER_ABORT 	0x0603	/* Abort transfers */
#define USBD_FNC_SYNCH_FRAME_GET	0x8604	/* Get synch frame */
#define USBD_FNC_CURRENT_FRAME_GET	0x8605	/* Get current frame */
#define USBD_FNC_SOF_MASTER_TAKE	0x0606	/* Become SOF master */
#define USBD_FNC_SOF_MASTER_RELEASE	0x0607	/* release SOF master */
#define USBD_FNC_SOF_INTERVAL_GET	0x0608	/* retrieve SOF */
#define USBD_FNC_SOF_INTERVAL_SET	0x0609	/* set SOF (master only) */
#define USBD_FNC_BUS_STATE_SET		0x060a	/* set bus state */


/* typedefs */

/*
 * URB_HEADER
 *
 * The URB_HEADER must be the first field in each URB data structure.  It
 * identifies the USBD function to be executed, the size of the URB, etc.
 */

typedef struct urb_header
    {
    pVOID link; 		/* n/a	USBD private link ptr */
    USBD_CLIENT_HANDLE handle;	/* I/O	Client's handle with USBD */
    UINT16 function;		/* IN	USBD function code */
    int result; 		/* OUT	Final URB result: S_usbdLib_xxxx */
    UINT16 urbLength;		/* IN	Length of the total URB */
    URB_CALLBACK callback;	/* IN	Completion callback */
    pVOID userPtr;		/* IN	Generic pointer for client use */
    } URB_HEADER, *pURB_HEADER;


/* URBs */

/*
 * URB_CLIENT_REG
 *
 * Note: For this function and only this function, the client does *not* pass
 * its USBD_CLIENT_HANDLE in the URB_HEADER.Handle field.  Instead, upon
 * completion of this function, the USBD will have stored a newly assigned
 * USBD_CLIENT_HANDLE in the URB_HEADER.Handle field, from which it should
 * be retrieved by the client.
 */

typedef struct urb_client_reg
    {
    URB_HEADER header;			/*	URB header */
    char clientName [USBD_NAME_LEN+1];	/* IN	Client name */
    } URB_CLIENT_REG, *pURB_CLIENT_REG;


/*
 * URB_CLIENT_UNREG
 */

typedef struct urb_client_unreg
    {
    URB_HEADER header;		/*	URB header */
    } URB_CLIENT_UNREG, *pURB_CLIENT_UNREG;


/*
 * URB_MNGMT_CALLBACK_SET
 */

typedef struct urb_mngmt_callback_set
    {
    URB_HEADER header;			/*	URB header */
    USBD_MNGMT_CALLBACK mngmtCallback;	/* IN	management callback or NULL */
    pVOID mngmtCallbackParam;		/* IN	client-defined mngmt callback */
    } URB_MNGMT_CALLBACK_SET, *pURB_MNGMT_CALLBACK_SET;
		

/*
 * URB_BUS_STATE_SET
 */

typedef struct urb_bus_state_set
    {
    URB_HEADER header;		/*	URB header */
    USBD_NODE_ID nodeId;	/* IN	node ID */
    UINT16 busState;		/* IN	desired bus state */
    } URB_BUS_STATE_SET, *pURB_BUS_STATE_SET;


/*
 * URB_BUS_COUNT_GET
 */

typedef struct urb_bus_count_get
    {
    URB_HEADER header;		/*	URB header */
    UINT16 busCount;		/* OUT	bus count */
    } URB_BUS_COUNT_GET, *pURB_BUS_COUNT_GET;


/*
 * URB_ROOT_ID_GET
 */

typedef struct urb_root_id_get
    {
    URB_HEADER header;		/*	URB header */
    UINT16 busIndex;		/* IN	bus index */
    USBD_NODE_ID rootId;	/* OUT	Node Id for root hub */
    } URB_ROOT_ID_GET, *pURB_ROOT_ID_GET;


/*
 * URB_HUB_PORT_COUNT_GET
 */

typedef struct urb_hub_port_count_get
    {
    URB_HEADER header;		/*	URB header */
    USBD_NODE_ID hubId; 	/* IN	Node Id for desired hub */
    UINT16 portCount;		/* OUT	port count for specified hub */
    } URB_HUB_PORT_COUNT_GET, *pURB_HUB_PORT_COUNT_GET;


/*
 * URB_NODE_ID_GET
 */

typedef struct urb_node_id_get
    {
    URB_HEADER header;		/*	URB header */
    USBD_NODE_ID hubId; 	/* IN	Node Id for desired hub */
    UINT16 portIndex;		/* IN	Index of desired port */
    UINT16 nodeType;		/* OUT	Type of node attached to port */
    USBD_NODE_ID nodeId;	/* OUT	Node Id for device attached to port */
    } URB_NODE_ID_GET, *pURB_NODE_ID_GET;


/*
 * URB_NODE_INFO_GET
 */

typedef struct urb_node_info_get
    {
    URB_HEADER header;		/*	URB header */
    USBD_NODE_ID nodeId;	/* IN	Node id of device/hub */
    pUSBD_NODE_INFO pNodeInfo;	/* IN	Ptr to struct to receive info */
    UINT16 infoLen;		/* IN	Length of struct allocated by client */
    } URB_NODE_INFO_GET, *pURB_NODE_INFO_GET;


/* 
 * URB_DYNAMIC_ATTACH_REG_UNREG
 */

typedef struct urb_dynamic_attach_reg_unreg
    {
    URB_HEADER header;		/*	URB header */
    UINT16 deviceClass; 	/* IN	USB Device class to register */
    UINT16 deviceSubClass;	/* IN	USB Device sub-class to register */
    UINT16 deviceProtocol;	/* IN	USB Device protocol */
    USBD_ATTACH_CALLBACK attachCallback;
				/* IN	Caller-supplied notification callback */
    } URB_DYNA_ATTACH_REG_UNREG, *pURB_DYNA_ATTACH_REG_UNREG;


/*
 * URB_FEATURE_CLEAR_SET
 *
 * NOTE: Same URB is used for USBD_FNC_FEATURE_CLEAR and USBD_FNC_FEATURE_SET.
 */

typedef struct urb_feature_clear_set
    {
    URB_HEADER header;		/*	URB header */
    USBD_NODE_ID nodeId;	/* IN	Node ID of device/hub */
    UINT16 requestType; 	/* IN	Selects request type */
    UINT16 feature;		/* IN	Feature selector */
    UINT16 index;		/* IN	Interface/endpoint index */
    } URB_FEATURE_CLEAR_SET, *pURB_FEATURE_CLEAR_SET;


/*
 * URB_CONFIG_GET_SET
 *
 * NOTE: Same URB is used for USBD_FNC_CONFIG_GET and USBD_FNC_CONFIG_SET.
 */

typedef struct urb_config_get_set
    {
    URB_HEADER header;		/*	URB header */
    USBD_NODE_ID nodeId;	/* IN	Node Id of device/hub */
    UINT16 maxPower;		/* IN	Max power to be consumed (set only) */
    UINT16 configuration;	/* I/O	Configuration value */
    } URB_CONFIG_GET_SET, *pURB_CONFIG_GET_SET;


/*
 * URB_DECSRIPTOR_GET_SET
 *
 * NOTE: Same URB is used for USBD_FNC_DESCRIPTOR_GET and 
 * USBD_FNC_DESCRIPTOR_SET.
 */

typedef struct urb_descriptor_get_set
    {
    URB_HEADER header;		/*	URB header */
    USBD_NODE_ID nodeId;	/* IN	Node Id of device/hub */
    UINT8 requestType;		/* IN	selects request type */
    UINT8 descriptorType;	/* IN	Type of descriptor */
    UINT8 descriptorIndex;	/* IN	Index of descriptor */
    UINT16 languageId;		/* IN	Language ID */
    UINT16 bfrLen;		/* IN	Max length of data to be returned */
    pUINT8 pBfr;		/* IN	Pointer to bfr to receive data */
    UINT16 actLen;		/* OUT	Actual length of data transferred */
    } URB_DESCRIPTOR_GET_SET, *pURB_DESCRIPTOR_GET_SET;


/*
 * URB_INTERFACE_GET_SET
 *
 * NOTE: Same URB is used for USBD_FNC_INTERFACE_GET and USBD_FNC_INTERFACE_SET.
 */

typedef struct urb_interface_get_set
    {
    URB_HEADER header;		/*	URB header */
    USBD_NODE_ID nodeId;	/* IN	Node Id of device/hub */
    UINT16 interfaceIndex;	/* IN	Index of interface */
    UINT16 alternateSetting;	/* I/O	Current alternate setting */
    } URB_INTERFACE_GET_SET, *pURB_INTERFACE_GET_SET;


/*
 * URB_STATUS_GET
 */

typedef struct urb_status_get
    {
    URB_HEADER header;		/*	URB header */
    USBD_NODE_ID nodeId;	/* IN	Node Id of device/hub */
    UINT8 requestType;		/* IN	selects request type */
    UINT16 index;		/* IN	Interface/endpoint index */
    UINT16 bfrLen;		/* IN	max len of status to receive */
    pUINT8 pBfr;		/* OUT	bfr to receive status from device */
    UINT16 actLen;		/* OUT	actual length of status received */
    } URB_STATUS_GET, *pURB_STATUS_GET;


/*
 * URB_ADDRESS_GET_SET
 *
 * NOTE: Same URB is used for USBD_FNC_ADDRESS_GET and USBD_FNC_ADDRESS_SET.
 */

typedef struct urb_address_get_set
    {
    URB_HEADER header;		/*	URB header */
    USBD_NODE_ID nodeId;	/* IN	Node Id of device/hub */
    UINT16 deviceAddress;	/* I/O	device address */
    } URB_ADDRESS_GET_SET, *pURB_ADDRESS_GET_SET;


/*
 * URB_VENDOR_SPECIFIC
 */

typedef struct urb_vendor_specific
    {
    URB_HEADER header;		/*	URB header */
    USBD_NODE_ID nodeId;	/* IN	Node Id of device/hub */
    UINT8 requestType;		/* IN	bmRequestType in USB spec. */
    UINT8 request;		/* IN	bRequest in USB spec. */
    UINT16 value;		/* IN	wValue in USB spec. */
    UINT16 index;		/* IN	wIndex in USB spec. */
    UINT16 length;		/* IN	wLength in USB spec. */
    pUINT8 pBfr;		/* IN	ptr to data buffer */
    UINT16 actLen;		/* OUT	actual length transferred */
    } URB_VENDOR_SPECIFIC, *pURB_VENDOR_SPECIFIC;


/*
 * URB_PIPE_CREATE
 */

typedef struct urb_pipe_create
    {
    URB_HEADER header;		/*	URB header */
    USBD_NODE_ID nodeId;	/* IN	Node Id of device/hub */
    UINT16 endpoint;		/* IN	Endpoint number */
    UINT16 configuration;	/* IN	config w/which pipe associated */
    UINT16 interface;		/* IN	interface w/which pipe associated */
    UINT16 transferType;	/* IN	Type of transfer: control, bulk, etc. */
    UINT16 direction;		/* IN	Specifies IN or OUT endpoint */
    UINT16 maxPayload;		/* IN	Maximum data payload per packet */
    UINT32 bandwidth;		/* IN	Bandwidth required for pipe */
    UINT16 serviceInterval;	/* IN	Required service interval */
    USBD_PIPE_HANDLE pipeHandle;/* OUT	pipe handle returned by USBD */
    } URB_PIPE_CREATE, *pURB_PIPE_CREATE;


/*
 * URB_PIPE_DESTROY
 */

typedef struct urb_pipe_destroy
    {
    URB_HEADER header;		/*	URB header */
    USBD_PIPE_HANDLE pipeHandle;/* IN	handle returned by usbdPipeCreate */
    } URB_PIPE_DESTROY, *pURB_PIPE_DESTROY;


/*
 * URB_TRANSFER
 */

typedef struct urb_transfer
    {
    URB_HEADER header;		/*	URB header */
    USBD_PIPE_HANDLE pipeHandle;/* IN	Pipe handle */
    pUSB_IRP pIrp;		/* IN	ptr to I/O request packet */
    } URB_TRANSFER, *pURB_TRANSFER;


/*
 * URB_SYNCH_FRAME_GET
 */

typedef struct urb_synch_frame_get
    {
    URB_HEADER header;		/*	URB header */
    USBD_NODE_ID nodeId;	/* IN	Node Id of device/hub */
    UINT16 endpoint;		/* IN	Endpoint to be queried */
    UINT16 frameNo;		/* OUT	Frame number returned by device */
    } URB_SYNCH_FRAME_GET, *pURB_SYNCH_FRAME_GET;


/* 
 * URB_CURRENT_FRAME_GET
 */

typedef struct urb_current_frame_get
    {
    URB_HEADER header;		/*	URB header */
    USBD_NODE_ID nodeId;	/* IN	Node Id of a device on desired USB */
    UINT32 frameNo;		/* OUT	Current frame number for USB */
    UINT32 frameWindow; 	/* OUT	Frame scheduling window */
    } URB_CURRENT_FRAME_GET, *pURB_CURRENT_FRAME_GET;


/*
 * URB_SOF_MASTER
 */

typedef struct urb_sof_master
    {
    URB_HEADER header;		/*	URB header */
    USBD_NODE_ID nodeId;	/* IN	node ID of a device on desired USB */
    } URB_SOF_MASTER, *pURB_SOF_MASTER;


/*
 * URB_SOF_INTERVAL_GET_SET
 */

typedef struct urb_sof_interval_get_set
    {
    URB_HEADER header;		/*	URB header */
    USBD_NODE_ID nodeId;	/* IN	node ID of a device on desired USB */
    UINT16 sofInterval; 	/* I/O	SOF interval in bit times */
    } URB_SOF_INTERVAL_GET_SET, *pURB_SOF_INTERVAL_GET_SET;


/*
 * URB_VERSION_GET
 */

typedef struct urb_version_get
    {
    URB_HEADER header;		/*	URB header */
    UINT16 version;		/* OUT	USBD version in BCD */
    UINT8 mfg [USBD_NAME_LEN+1];/* OUT	USBD manufacturer name */
    } URB_VERSION_GET, *pURB_VERSION_GET;


/*
 * URB_HCD_ATTACH
 */

typedef struct urb_hcd_attach
    {
    URB_HEADER header;		    /*	    URB header */
    HCD_EXEC_FUNC hcdExecFunc;	    /* IN   HCD primary entry point */
    pVOID param;		    /* IN   HCD-specific parameter */
    GENERIC_HANDLE attachToken;     /* OUT  attach token */
    } URB_HCD_ATTACH, *pURB_HCD_ATTACH;


/*
 * URB_HCD_DETACH
 */

typedef struct urb_hcd_detach
    {
    URB_HEADER header;		    /*	    URB header */
    GENERIC_HANDLE attachToken;     /* IN   attach token */
    } URB_HCD_DETACH, *pURB_HCD_DETACH;


/*
 * URB_STATISTICS_GET
 */

typedef struct urb_statistics_get
    {
    URB_HEADER header;		/*	URB header */
    USBD_NODE_ID nodeId;	/* IN	Node Id of a node on desired USB */
    pUSBD_STATS pStatistics;	/* IN	Ptr to structure to receive stats */
    UINT16 statLen;		/* IN	Len of stats bfr provided by caller */
    } URB_STATISTICS_GET, *pURB_STATISTICS_GET;


/* function prototypes */

STATUS usbdCoreEntry
    (
    pURB_HEADER pUrb			/* URB to be executed */
    );


#ifdef	__cplusplus
}
#endif

#endif	/* __INCusbdCoreLibh */


/* End of file. */

