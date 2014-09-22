/* usbdStructures.h - USBD control structures */

/* Copyright 2000-2002 Wind River Systems, Inc. */

/*
Modification history
--------------------
01f,08nov11,wef  change definition of hubStatus to be pHubStatus
01e,20mar00,rcb  Add USB_DEV_DESCR field to USBD_NODE.
01d,26jan00,rcb  Redefine "bandwidth" field in USBD_PIPE as UINT32.
01c,23nov99,rcb  Add support for HCD_PIPE_HANDLE.
01b,07sep99,rcb  Add support for management callbacks.
01a,08jun99,rcb  First.
*/

/*
DESCRIPTION

This file defines structures & constants used by the USBD to manage USB
buses attached to the system. 
*/

#ifndef __INCusbdStructuresh
#define __INCusbdStructuresh

#ifdef	__cplusplus
extern "C" {
#endif


/* includes */

#include "usb/ossLib.h"
#include "usb/usbHandleLib.h"
#include "usb/usbListLib.h"
#include "usb/usb.h"
#include "drv/usb/usbHcd.h"

/* defines */

/* signatures used to validate handles - values are arbitrary */

#define USBD_CLIENT_SIG 	((UINT32) 0x00bd000c)
#define USBD_HCD_SIG		((UINT32) 0x00bd00cd)
#define USBD_NODE_SIG		((UINT32) 0x00bd00de)
#define USBD_PIPE_SIG		((UINT32) 0x00bd000e)


/* defines related to hubs */

/* MAX_HUB_STATUS_LEN is the maximum length of data which can be
 * returned when querying a hub's status endpoint.  The status
 * has one bit per port plus one bit for the hub itself.  The length
 * of the array is rounded up to the next whole byte.
 */

#define MAX_HUB_STATUS_LEN	((USB_MAX_DEVICES + 1 + 7) / 8)


/* USB_DEV_VECTOR_LEN is the length of an array of bytes which can
 * hold one bit for each USB device.
 */

#define USB_DEV_VECTOR_LEN	((USB_MAX_DEVICES + 7) / 8)


/* typedefs */

/*
 * USBD_NOTIFY_REQ
 *
 * Each time a client registers for dynamic attach/removal notification,
 * a USBD_NOTIFY_REQ structure is linked to the client's USBD_CLIENT struct.
 *
 * A value of USBD_NOTIFY_ALL in deviceClass, deviceSubclass, or devicePgmIf
 * will match any value reported by the device.
 */

typedef struct usbd_notify_req
    {
    LINK reqLink;		    /* linked list of registrations */
    UINT16 deviceClass; 	    /* desired device class */
    UINT16 deviceSubClass;	    /* desired device sub-class */
    UINT16 deviceProtocol;	    /* desired protocol */
    USBD_ATTACH_CALLBACK callback;  /* client's callback routine */
    } USBD_NOTIFY_REQ, *pUSBD_NOTIFY_REQ;


/*
 * USBD_NODE_CLASS
 *
 * Each device exposes one or more class/subclass/pgmif types.	Some
 * devices expose only one type through the device descriptor.	Others
 * expose many types, one through each interface descriptor.
 */

typedef struct usbd_node_class
    {
    LINK classLink;		    /* linked list of class types */
    UINT16 configuration;	    /* corresponding configuration (or 0) */
    UINT16 interface;		    /* corresponding interface (or 0) */
    UINT16 deviceClass; 	    /* device class */
    UINT16 deviceSubClass;	    /* device subclass */
    UINT16 deviceProtocol;	    /* device protocol */
    } USBD_NODE_CLASS, *pUSBD_NODE_CLASS;


/* 
 * USBD_PIPE
 *
 * Each open channel of communication between the host and an endpoint on a
 * device/hub is termed a "pipe".  USBD_PIPE maintains all currently available
 * information for each open pipe.
 *
 * NOTE: By convention, the USBD uses the usbdLink field in each USB_IRP
 * structure to point back to the USBD_PIPE with which the IPR is associated.
 */

typedef struct usbd_pipe
    {
    USBD_PIPE_HANDLE handle;		/* handled assigned to pipe by USBD */
    HCD_PIPE_HANDLE hcdHandle;		/* handled assigned to pipe by HCD */

    struct usbd_client *pClient;	/* pointer to client owning this pipe */

    BOOL pipeDeletePending;		/* TRUE when pipe being deleted. */

    struct usbd_node *pNode;		/* node to which pipe is addressed */
    UINT16 endpoint;			/* endpoint address */
    UINT16 configuration;		/* config w/which pipe associated */
    UINT16 interface;			/* interface w/which pipe associated */
    UINT16 transferType;		/* type of pipe */
    UINT16 direction;			/* direction of pipe */
    UINT16 maxPacketSize;		/* max packet size */
    UINT32 bandwidth;			/* bandwidth used by pipe (bytes/sec) */
    UINT16 interval;			/* service interval (intrp pipes) */
    UINT16 dataToggle;			/* current DATA0/DATA1 toggle */

    UINT32 nanoseconds; 		/* bandwidth required if pipe is */
					/* isochronous or interrupt, else 0 */

    LINK clientPipeLink;		/* link on list of client's pipes */
    LINK nodePipeLink;			/* link on list of node's pipes */

    LIST_HEAD irps;			/* list of IRPs on this pipe */

    pUSB_IRP irpBeingDeleted;		/* used to synchronize IRP deletion */
    BOOL irpDeleted;			/*  "	 "	"	"     "     */

    } USBD_PIPE, *pUSBD_PIPE;


/*
 * USBD_CLIENT
 *
 * Each client registered with the USBD is tracked by a USBD_CLIENT structure.
 */

typedef struct usbd_client
    {
    LINK clientLink;			/* client linked list */

    USBD_CLIENT_HANDLE handle;		/* handle assigned to client */

    USBD_MNGMT_CALLBACK mngmtCallback;	/* client's mngmt callback */
    pVOID mngmtCallbackParam;		/* client-define parameter */

    THREAD_HANDLE callbackThread;	/* thread used for client callbacks */
    QUEUE_HANDLE callbackQueue; 	/* queue of callback requests */
    SEM_HANDLE callbackExit;		/* signalled when callback thrd exits */

    LIST_HEAD pipes;			/* list of pipes owned by client */

    LIST_HEAD notifyReqs;		/* list of dynamic attach requests */

    char clientName [USBD_NAME_LEN+1];	/* client's text name */

    } USBD_CLIENT, *pUSBD_CLIENT;


/*
 * USBD_PORT
 *
 * USB hubs have one or more ports to which additional USB devices/hubs
 * may be attached.  The USBD_PORT structure maintains all currently available
 * inforamtion for each port on a hub.
 */

typedef struct usbd_port
    {
    struct usbd_node *pNode;		/* node attached to port */
    } USBD_PORT, *pUSBD_PORT;


/*
 * USBD_NODE
 *
 * Each device or hub attached to the USB is termed a "node" by the USBD.
 * The USBD_NODE structure maintains all currently available information
 * about a node.  If the node is a hub, then space will be allocated for
 * a ports[] array large enough to accomodate each port on the hub.
 */

typedef struct usbd_node
    {
    USBD_NODE_ID nodeHandle;		/* handle assigned to node */
    USBD_NODE_INFO nodeInfo;		/* Current node information */
    struct usbd_bus *pBus;		/* pointer to parent bus */
    UINT16 busAddress;			/* USB bus address for dev/hub */
    UINT16 topologyDepth;		/* number of cable hops from root */
    UINT16 maxPower;			/* power draw for selected config */

    USB_DEVICE_DESCR devDescr;		/* Device descriptor */
    LIST_HEAD classTypes;		/* list of device class types */

    BOOL nodeDeletePending;		/* TRUE when node being deleted */

    /* NOTE: The following fields are used during control pipe transfers to
     * this node.  controlSem is used to ensure that only one control pipe
     * request is active for each device at any given time.
     */

    USBD_PIPE_HANDLE controlPipe;	/* control pipe handle */
    SEM_HANDLE controlSem;		/* only one outstanding request */
					/* to control pipe at a time */
    pUSBD_CLIENT pClient;		/* client invoking the control URB */
    pURB_HEADER pUrb;			/* Ptr to pending control xfr URB */
    pUINT16 pActLen;			/* Ptr to bfr to receive IN actlen */

    USB_SETUP setup;			/* control pipe Setup packet */
    USB_IRP irp;			/* irp to describe control transfer */
    USB_BFR_LIST extra [2];		/* irp includes room for a single */
					/* USB_BFR_LIST.  We need room for 3 */
					/* so we allocate an extra here */

    LIST_HEAD pipes;			/* list of pipes addressed to node */

    /* NOTE: The following fields are used only for hub nodes. */

    UINT16 pwrGoodDelay;		/* power-ON to power good delay */
    UINT16 hubContrCurrent;		/* hub controller power requirements */
    BOOL selfPowered;			/* TRUE if hub/port is self powered */
    UINT16 maxPowerPerPort;		/* max power port can provide */

    USBD_PIPE_HANDLE hubStatusPipe;	/* status pipe handle */
    USB_IRP hubIrp;			/* IRP used to monitor hub status */
    UINT16 numPorts;			/* number of ports, used for hubs */
    pUSBD_PORT pPorts;			/* Array of ports, used for hubs */
    UINT8 * pHubStatus; 		/* receives hub status */

    } USBD_NODE, *pUSBD_NODE;


/*
 * USBD_BUS
 *
 * Each HCD attached to the USBD may control one or more bus.  The USBD_BUS
 * structure contains all information for a given bus.
 */

typedef struct usbd_bus
    {
    struct usbd_hcd *pHcd;		/* pointer to parent HCD */
    UINT16 busNo;			/* this bus's index with HCD */

    pUSBD_NODE pRoot;			/* root node for this bus */

    THREAD_HANDLE busThread;		/* thread to monitor bus events */
    QUEUE_HANDLE busQueue;		/* queue to receive bus events */
    SEM_HANDLE busExit; 		/* signallen when bus thd exits */

    UINT32 nanoseconds; 		/* bus bandwidth in use for */
					/* isochronous and interrupt pipes */

    pUSBD_CLIENT pSofMasterClient;	/* client which is SOF master or NULL */

    USBD_STATS stats;			/* bus operating statistics */

    UINT8 adrsVec [USB_MAX_DEVICES];	/* USB addresses in use */

    BOOL suspended;			/* TRUE if bus currently SUSPENDed */

    } USBD_BUS, *pUSBD_BUS;


/*
 * USBD_HCD
 *
 * The USBD maintains a list of currently attached HCDs.  Each HCD may control
 * one or more buses.
 */

typedef struct usbd_hcd
    {
    LINK hcdLink;			/* linked list of HCDs */

    GENERIC_HANDLE attachToken; 	/* Attach token returned for this HCD */
    HCD_NEXUS nexus;			/* identifies connection to HCD */

    UINT16 busCount;			/* number of buses managed by HCD */
    pUSBD_BUS pBuses;			/* buses managed by HCD */
    } USBD_HCD, *pUSBD_HCD;


#ifdef	__cplusplus
}
#endif

#endif	/* __INCusbdStructuresh */


/* End of file. */

