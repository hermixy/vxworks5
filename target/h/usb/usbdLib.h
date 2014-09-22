/* usbdLib.h - USBD functional interface definition */

/* Copyright 2000 Wind River Systems, Inc. */

/*
Modification history
--------------------
01h,18sep01,wef  merge from wrs.tor2_0.usb1_1-f for veloce
01g,07may01,wef  changed module number to be (module sub num << 8) | 
		 M_usbHostLib
01f,02may01,wef  changed module number to be M_<module> + M_usbHostLib
01e,05dec00,wef  moved Module number defs to vwModNum.h - add this
                 to #includes
01d,26jan00,rcb  Redefine <bandwidth> parameter to usbdPipeCreate() as UINT32.
01c,29nov99,rcb  Increase frame number fields to 32-bits in 
		 usbdCurrentFrameGet().
01b,07sep99,rcb  Add management callbacks and set-bus-state API.
01a,07may99,rcb  First.
*/

/*
DESCRIPTION

Defines the USBD functional interface.	Functions are provided to invoke
each of the underlying USBD URBs (request blocks).
*/

#ifndef __INCusbdLibh
#define __INCusbdLibh

#ifdef	__cplusplus
extern "C" {
#endif


/* includes */

#include "usb/usbHandleLib.h"	    /* handle utility funcs */
#include "usb/usb.h"		    /* Basic USB definitions */
#include "drv/usb/usbHcd.h"	    /* USB HCD interface definitions */
#include "vwModNum.h"		    /* USB Module number def's */ 

/* defines */

/* USBD Results - URB_HEADER.Result */

/* 
 * USB errnos are defined as being part of the USB host Module, as are all
 * vxWorks module numbers, but the USB Module number is further divided into 
 * sub-modules.  Each sub-module has upto 255 values for its own error codes
 */
 
#define USB_USBD_SUB_MODULE  5

#define M_usbdLib 	( (USB_USBD_SUB_MODULE  << 8) | M_usbHostLib )

#define usbdErr(x)	(M_usbdLib | (x))

#define S_usbdLib_BAD_CLIENT	    usbdErr(1)
#define S_usbdLib_BAD_PARAM	    usbdErr(2)
#define S_usbdLib_BAD_HANDLE	    usbdErr(3)
#define S_usbdLib_OUT_OF_MEMORY     usbdErr(4)
#define S_usbdLib_OUT_OF_RESOURCES  usbdErr(5)
#define S_usbdLib_NOT_IMPLEMENTED   usbdErr(6)
#define S_usbdLib_GENERAL_FAULT     usbdErr(7)
#define S_usbdLib_NOT_INITIALIZED   usbdErr(8)
#define S_usbdLib_INTERNAL_FAULT    usbdErr(9)
#define S_usbdLib_TIMEOUT	    usbdErr(10)
#define S_usbdLib_HCD_FAULT	    usbdErr(11)
#define S_usbdLib_IO_FAULT	    usbdErr(12)
#define S_usbdLib_NOT_HUB	    usbdErr(13)
#define S_usbdLib_CANNOT_CANCEL     usbdErr(14)
#define S_usbdLib_BANDWIDTH_FAULT   usbdErr(15)
#define S_usbdLib_POWER_FAULT	    usbdErr(16)
#define S_usbdLib_SOF_MASTER_FAULT  usbdErr(17)


/* String length definitions */

#define USBD_NAME_LEN		32	/* Maximum length for name */


/* management events */

#define USBD_MNGMT_RESUME	1	/* remote wakeup/resume */


/* bus states - see usbdBusStateSet() */

#define USBD_BUS_SUSPEND	0x0001	/* suspend bus */
#define USBD_BUS_RESUME 	0x0002	/* resume bus */


/* USBD_NOTIFY_ALL pertains to dynamic attach/removal notification */

#define USBD_NOTIFY_ALL 	0xffff

#define USBD_DYNA_ATTACH	0
#define USBD_DYNA_REMOVE	1


/* typedefs */

/* Handles and callbacks. */

typedef GENERIC_HANDLE USBD_CLIENT_HANDLE, *pUSBD_CLIENT_HANDLE;
typedef GENERIC_HANDLE USBD_NODE_ID, *pUSBD_NODE_ID;
typedef GENERIC_HANDLE USBD_PIPE_HANDLE, *pUSBD_PIPE_HANDLE;

typedef VOID (*URB_CALLBACK) (pVOID pUrb);

typedef VOID (*USBD_ATTACH_CALLBACK) 
    (
    USBD_NODE_ID nodeId, 
    UINT16 attachAction, 
    UINT16 configuration,
    UINT16 interface,
    UINT16 deviceClass, 
    UINT16 deviceSubClass, 
    UINT16 deviceProtocol
    );

typedef VOID (*USBD_MNGMT_CALLBACK)
    (
    pVOID callbackParam,
    USBD_NODE_ID nodeId,
    UINT16 mngmtCode
    );


/*
 * USBD_NODE_INFO
 */

typedef struct usbd_node_info
    {
    UINT16 nodeType;		/* Type of node */
    UINT16 nodeSpeed;		/* Speed of node, e.g., 12MBit, 1.5MBit */
    USBD_NODE_ID parentHubId;	/* Node Id of hub to which node is connected */
    UINT16 parentHubPort;	/* Port on parent hub to which connected */
    USBD_NODE_ID rootId;	/* Node Id of root for USB to which connected */
    } USBD_NODE_INFO, *pUSBD_NODE_INFO;


/*
 * USBD_STATS
 */

typedef struct usbd_stats
    {
    UINT16 totalTransfersIn;	/* Total # of inbound transfers */
    UINT16 totalTransfersOut;	/* Total # of outbound transfers */
    UINT16 totalReceiveErrors;	/* Errors on inbound traffic */
    UINT16 totalTransmitErrors; /* Errors on transmit */
    } USBD_STATS, *pUSBD_STATS;


/* function prototypes */

STATUS usbdInitialize (void);


STATUS usbdShutdown (void);


STATUS usbdClientRegister 
    (
    pCHAR pClientName,			/* Client name */
    pUSBD_CLIENT_HANDLE pClientHandle	/* Client hdl returned by USBD */
    );


STATUS usbdClientUnregister
    (
    USBD_CLIENT_HANDLE clientHandle	/* Client handle */
    );


STATUS usbdMngmtCallbackSet
    (
    USBD_CLIENT_HANDLE clientHandle,	/* Client handle */
    USBD_MNGMT_CALLBACK mngmtCallback,	/* management callback */
    pVOID mngmtCallbackParam		/* client-defined parameter */
    );


STATUS usbdBusStateSet
    (
    USBD_CLIENT_HANDLE clientHandle,	/* Client handle */
    USBD_NODE_ID nodeId,		/* node ID */
    UINT16 busState			/* new bus state: USBD_BUS_xxxx */
    );	


STATUS usbdBusCountGet
    (
    USBD_CLIENT_HANDLE clientHandle,	/* Client handle */
    pUINT16 pBusCount			/* Word bfr to receive bus count */
    );


STATUS usbdRootNodeIdGet
    (
    USBD_CLIENT_HANDLE clientHandle,	/* Client handle */
    UINT16 busIndex,			/* Bus index */
    pUSBD_NODE_ID pRootId		/* bfr to receive Root Id */
    );


STATUS usbdHubPortCountGet
    (
    USBD_CLIENT_HANDLE clientHandle,	/* Client handle */
    USBD_NODE_ID hubId, 		/* Node Id for desired hub */
    pUINT16 pPortCount			/* UINT16 bfr to receive port count */
    );


STATUS usbdNodeIdGet
    (
    USBD_CLIENT_HANDLE clientHandle,	/* Client handle */
    USBD_NODE_ID hubId, 		/* Node Id for desired hub */
    UINT16 portIndex,			/* Port index */
    pUINT16 pNodeType,			/* bfr to receive node type */
    pUSBD_NODE_ID pNodeId		/* bfr to receive Node Id */
    );


STATUS usbdNodeInfoGet
    (
    USBD_CLIENT_HANDLE clientHandle,	/* Client handle */
    USBD_NODE_ID nodeId,		/* Node Id of device/hub */
    pUSBD_NODE_INFO pNodeInfo,		/* Structure to receive node info */
    UINT16 infoLen			/* Len of bfr allocated by client */
    );


STATUS usbdDynamicAttachRegister
    (
    USBD_CLIENT_HANDLE clientHandle,	/* Client handle */
    UINT16 deviceClass, 		/* USB class code */
    UINT16 deviceSubClass,		/* USB sub-class code */
    UINT16 deviceProtocol,		/* USB device protocol */
    USBD_ATTACH_CALLBACK attachCallback /* User-supplied callback routine */
    );


STATUS usbdDynamicAttachUnRegister
    (
    USBD_CLIENT_HANDLE clientHandle,	/* Client handle */
    UINT16 deviceClass, 		/* USB class code */
    UINT16 deviceSubClass,		/* USB sub-class code */
    UINT16 deviceProtocol,		/* USB device protocol */
    USBD_ATTACH_CALLBACK attachCallback /* User-supplied callback routine */
    );


STATUS usbdFeatureClear
    (
    USBD_CLIENT_HANDLE clientHandle,	/* Client handle */
    USBD_NODE_ID nodeId,		/* Node Id of device/hub */
    UINT16 requestType, 		/* Selects request type */
    UINT16 feature,			/* Feature selector */
    UINT16 index			/* Interface/endpoint index */
    );


STATUS usbdFeatureSet
    (
    USBD_CLIENT_HANDLE clientHandle,	/* Client handle */
    USBD_NODE_ID nodeId,		/* Node Id of device/hub */
    UINT16 requestType, 		/* Selects request type */
    UINT16 feature,			/* Feature selector */
    UINT16 index			/* Interface/endpoint index */
    );


STATUS usbdConfigurationGet
    (
    USBD_CLIENT_HANDLE clientHandle,	/* Client handle */
    USBD_NODE_ID nodeId,		/* Node Id of device/hub */
    pUINT16 pConfiguration		/* bfr to receive config value */
    );


STATUS usbdConfigurationSet
    (
    USBD_CLIENT_HANDLE clientHandle,	/* Client handle */
    USBD_NODE_ID nodeId,		/* Node Id of device/hub */
    UINT16 configuration,		/* New configuration to be set */
    UINT16 maxPower			/* max power this config will draw */
    );


STATUS usbdDescriptorGet
    (
    USBD_CLIENT_HANDLE clientHandle,	/* Client handle */
    USBD_NODE_ID nodeId,		/* Node Id of device/hub */
    UINT8 requestType,			/* specifies type of request */
    UINT8 descriptorType,		/* Type of descriptor */
    UINT8 descriptorIndex,		/* Index of descriptor */
    UINT16 languageId,			/* Language ID */
    UINT16 bfrLen,			/* Max length of data to be returned */
    pUINT8 pBfr,			/* Pointer to bfr to receive data */
    pUINT16 pActLen			/* bfr to receive actual length */
    );


STATUS usbdDescriptorSet
    (
    USBD_CLIENT_HANDLE clientHandle,	/* Client handle */
    USBD_NODE_ID nodeId,		/* Node Id of device/hub */
    UINT8 requestType,			/* selects request type */
    UINT8 descriptorType,		/* Type of descriptor */
    UINT8 descriptorIndex,		/* Index of descriptor */
    UINT16 languageId,			/* Language ID */
    UINT16 bfrLen,			/* Max length of data to be returned */
    pUINT8 pBfr 			/* Pointer to bfr to receive data */
    );


STATUS usbdInterfaceGet
    (
    USBD_CLIENT_HANDLE clientHandle,	/* Client handle */
    USBD_NODE_ID nodeId,		/* Node Id of device/hub */
    UINT16 interfaceIndex,		/* Index of interface */
    pUINT16 pAlternateSetting		/* Current alternate setting */
    );


STATUS usbdInterfaceSet
    (
    USBD_CLIENT_HANDLE clientHandle,	/* Client handle */
    USBD_NODE_ID nodeId,		/* Node Id of device/hub */
    UINT16 interfaceIndex,		/* Index of interface */
    UINT16 alternateSetting		/* Alternate setting */
    );


STATUS usbdStatusGet
    (
    USBD_CLIENT_HANDLE clientHandle,	/* Client handle */
    USBD_NODE_ID nodeId,		/* Node Id of device/hub */
    UINT16 requestType, 		/* Selects device/interface/endpoint */
    UINT16 index,			/* Interface/endpoint index */
    UINT16 bfrLen,			/* length of bfr */
    pUINT8 pBfr,			/* bfr to receive status */
    pUINT16 pActLen			/* bfr to receive act len xfr'd */
    );


STATUS usbdAddressGet
    (
    USBD_CLIENT_HANDLE clientHandle,	/* Client handle */
    USBD_NODE_ID nodeId,		/* Node Id of device/hub */
    pUINT16 pDeviceAddress		/* Currently assigned device address */
    );


STATUS usbdAddressSet
    (
    USBD_CLIENT_HANDLE clientHandle,	/* Client handle */
    USBD_NODE_ID nodeId,		/* Node Id of device/hub */
    UINT16 deviceAddress		/* New device address */
    );


STATUS usbdVendorSpecific
    (
    USBD_CLIENT_HANDLE clientHandle,	/* Client handle */
    USBD_NODE_ID nodeId,		/* Node Id of device/hub */
    UINT8 requestType,			/* bmRequestType in USB spec. */
    UINT8 request,			/* bRequest in USB spec. */
    UINT16 value,			/* wValue in USB spec. */
    UINT16 index,			/* wIndex in USB spec. */
    UINT16 length,			/* wLength in USB spec. */
    pUINT8 pBfr,			/* ptr to data buffer */
    pUINT16 pActLen			/* actual length of IN */
    );


STATUS usbdPipeCreate
    (
    USBD_CLIENT_HANDLE clientHandle,	/* Client handle */
    USBD_NODE_ID nodeId,		/* Node Id of device/hub */
    UINT16 endpoint,			/* Endpoint number */
    UINT16 configuration,		/* config w/which pipe associated */
    UINT16 interface,			/* interface w/which pipe associated */
    UINT16 transferType,		/* Type of transfer: control, bulk... */
    UINT16 direction,			/* Specifies IN or OUT endpoint */
    UINT16 maxPayload,			/* Maximum data payload per packet */
    UINT32 bandwidth,			/* Bandwidth required for pipe */
    UINT16 serviceInterval,		/* Required service interval */
    pUSBD_PIPE_HANDLE pPipeHandle	/* pipe handle returned by USBD */
    );


STATUS usbdPipeDestroy
    (
    USBD_CLIENT_HANDLE clientHandle,	/* Client handle */
    USBD_PIPE_HANDLE pipeHandle 	/* pipe handle */
    );


STATUS usbdTransfer
    (
    USBD_CLIENT_HANDLE clientHandle,	/* Client handle */
    USBD_PIPE_HANDLE pipeHandle,	/* Pipe handle */
    pUSB_IRP pIrp			/* ptr to I/O request packet */
    );


STATUS usbdTransferAbort
    (
    USBD_CLIENT_HANDLE clientHandle,	/* Client handle */
    USBD_PIPE_HANDLE pipeHandle,	    /* Pipe handle */
    pUSB_IRP pIrp			/* ptr to I/O to abort */
    );


STATUS usbdSynchFrameGet
    (
    USBD_CLIENT_HANDLE clientHandle,	/* Client Handle */
    USBD_NODE_ID nodeId,		/* Node Id of device/hub */
    UINT16 endpoint,			/* Endpoint to be queried */
    pUINT16 pFrameNo			/* Frame number returned by device */
    );


STATUS usbdCurrentFrameGet
    (
    USBD_CLIENT_HANDLE clientHandle,	/* Client handle */
    USBD_NODE_ID nodeId,		/* Node Id of root for desired USB */
    pUINT32 pFrameNo,			/* bfr to receive current frame no. */
    pUINT32 pFrameWindow		/* bfr to receive frame window */
    );


STATUS usbdSofMasterTake
    (
    USBD_CLIENT_HANDLE clientHandle,	/* Client handle */
    USBD_NODE_ID nodeId 		/* Node Id of node on desired USB */
    );
    

STATUS usbdSofMasterRelease
    (
    USBD_CLIENT_HANDLE clientHandle,	/* Client handle */
    USBD_NODE_ID nodeId 		/* Node Id of node on desired USB */
    );


STATUS usbdSofIntervalGet
    (
    USBD_CLIENT_HANDLE clientHandle,	/* Client handle */
    USBD_NODE_ID nodeId,		/* Node Id of node on desired USB */
    pUINT16 pSofInterval		/* bfr to receive SOF interval */
    );


STATUS usbdSofIntervalSet
    (
    USBD_CLIENT_HANDLE clientHandle,	/* Client handle */
    USBD_NODE_ID nodeId,		/* Node Id of node on desired USB */
    UINT16 sofInterval			/* new SOF interval */
    );


STATUS usbdVersionGet
    (
    pUINT16 pVersion,			/* UINT16 bfr to receive version */
    pCHAR pMfg				/* bfr to receive USBD mfg string */
    );


STATUS usbdHcdAttach
    (
    HCD_EXEC_FUNC hcdExecFunc,		/* Ptr to HCD’s primary entry point */
    pVOID param,			/* HCD-specific parameter */
    pGENERIC_HANDLE pAttachToken	/* Token to identify HCD in future */
    );


STATUS usbdHcdDetach
    (
    GENERIC_HANDLE attachToken		/* AttachToken returned */
    );


STATUS usbdStatisticsGet
    (
    USBD_CLIENT_HANDLE clientHandle,	/* Client handle */
    USBD_NODE_ID nodeId,		/* Node Id of a node on desired USB */
    pUSBD_STATS pStatistics,		/* Ptr to structure to receive stats */
    UINT16 statLen			/* Len of bfr provided by caller */
    );


#ifdef	__cplusplus
}
#endif

#endif	/* __INCusbdLibh */


/* End of file. */

