/* usbdLib.c - USBD functional interface */

/* Copyright 2000-2001 Wind River Systems, Inc. */

/*
Modification history
--------------------
01f,18sep01,wef  merge from wrs.tor2_0.usb1_1-f for veloce
01e,08aug01,dat  Removing warnings
01d,26jan00,rcb  Redefine <bandwidth> parameter to usbdPipeCreate() to
		 express the number of bytes per second or bytes per frame
		 depending on type of pipe.
01c,29nov99,rcb  Increase frame number fields to 32-bits in 
		 usbdCurrentFrameGet().
01b,07sep99,rcb  Add management callbacks and set-bus-state API.
01a,03jun99,rcb  First.
*/

/*
DESCRIPTION

Implements the USBD functional interface.  Functions are provided to invoke
each of the underlying USBD URBs (request blocks).

In order to use the USBD, it is first necessary to invoke usbdInitialize().
Multiple calls to usbdInitialize() may be nested so long as a corresponding
number of calls to usbdShutdown() are also made.  This allows multiple USBD
clients to be written independently and without concern for coordinating the
initialization of the independent clients.

After calling usbdInitialize(), a typical system will call usbdHcdAttach()
at least one time in order to attach a USB HCD (Host Controller Driver) to 
the USBD.  The call to usbdHcdAttach() is not typically made by normal USBD
clients.  Rather, this call is generally made by a "super client" or by
system initialization code.  

After the USBD has been initialized and at least one HCD has been attached,
then normal USBD operation may begin.  Normal USBD clients must register
with the USBD by calling usbdClientRegister().	In response to this call, the
USBD allocates per-client data structures and a client callback task.
Callbacks for each client are invoked from this client-unique task.  This
improves the USBD's ability to shield clients from one-another and to help
ensure the real-time response for all clients.

After a client has registered, it will most often also register for dynamic
attachment notification using usbdDynamicAttachRegister().  This function
allows a special client callback routine to be invoked each time a USB device
is attached or removed from the system.  In this way, clients may discover the
real-time attachment and removal of devices.

Finally, clients may use a combination of the USBD configuration and transfer
functions to configure and exchange data with USB devices.
*/


/* includes */

#include "usb/usbPlatform.h"

#include "string.h"

#include "usb/ossLib.h" 	/* OS services */
#include "usb/usbQueueLib.h"
#include "usb/usbdLib.h"	/* our API */
#include "usb/usbdCoreLib.h"	/* interface to USBD core library */


/* defines */

#define MAX_SYNCH_SEM	    8		/* Max simultaneous requests which */
					/* can be synchronized */

#define SYNCH_SEM_TIMEOUT   1000	/* Time to wait for synch sem to be */
					/* released during shutdown */


/* locals */

LOCAL int initCount = 0;		/* init nesting count */
LOCAL BOOL ossInitialized = FALSE;	/* TRUE if we initialized ossLib */
LOCAL BOOL coreLibInitialized = FALSE;	/* TRUE if we initialized usbdCoreLib */

LOCAL QUEUE_HANDLE semPoolQueue = NULL; /* pool of synchronizing semaphores */


/***************************************************************************
*
* urbInit - Initialize a URB
*
* Initialize the caller's URB to zeros, then store the <clientHandle>,
* <function>, <callback>, <userPtr>, and <totalLen> fields passed by the
* caller.
*
* RETURNS: N/A
*/

LOCAL VOID urbInit
    (
    pURB_HEADER pUrb,			/* Clients URB */
    USBD_CLIENT_HANDLE clientHandle,	/* Client handle */
    UINT16 function,			/* USBD function code for header */
    URB_CALLBACK callback,		/* Completion callback routine */
    pVOID userPtr,			/* User-defined pointer */
    UINT16 totalLen			/* Total len of URB to be allocated */
    )

    {
    memset (pUrb, 0, totalLen);

    pUrb->handle = clientHandle;
    pUrb->function = function;
    pUrb->callback = callback;
    pUrb->userPtr = userPtr;
    pUrb->urbLength = totalLen;
    }


/***************************************************************************
*
* urbCallback - Internal callback used for asynchronous USBD functions
*
* This callback will be used by urbExecBlock() as the URB_HEADER.callback
* routine when urbExecBlock() executes an asynchronous USBD function.
* By convention, the URB_HEADER.userPtr field will contain the SEM_HANDLE
* of a semaphore which should be signalled.
*
* RETURNS: N/A
*/

LOCAL VOID urbCallback
    (
    pVOID pUrb				/* Completed URB */
    )

    {
    OSS_SEM_GIVE ((SEM_HANDLE) ((pURB_HEADER) pUrb)->userPtr);
    }


/***************************************************************************
*
* urbExecBlock - Block on the execution of the caller's URB
*
* Execute the <pUrb> passed by the caller and block until execution
* completes. 
*
* RETURNS: OK, or ERROR if error detected while trying to execute URB
*/

LOCAL STATUS urbExecBlock
    (
    pURB_HEADER pUrb			/* Caller's URB */
    )

    {
    USB_MESSAGE msg;

    
    /* Have we been initialized? */

    if (initCount == 0)
	return ossStatus (S_usbdLib_NOT_INITIALIZED);


    /* Get a semaphore from the pool of synchronization semaphores to be
     * used for this URB.
     */

    if (usbQueueGet (semPoolQueue, &msg, OSS_BLOCK) != OK)
	return ossStatus (S_usbdLib_OUT_OF_RESOURCES);

    /* msg.lParam is an available SEM_HANDLE. */

    pUrb->callback = urbCallback;
    pUrb->userPtr = (pVOID) msg.lParam;

    if (usbdCoreEntry (pUrb) == OK)
	{
	/* wait for the URB to complete */

	OSS_SEM_TAKE ((SEM_HANDLE) msg.lParam, OSS_BLOCK);
	}
    else
	{
	/* If the USBD reported an error, we don't know if the URB
	 * callback was invoked or not (depends on how bad an error
	 * the USBD detected, e.g., malformed URB).  So, we need to
	 * make sure the semaphore is returned to the cleared state
	 * before we put it back on the queue.
	 */

	OSS_SEM_TAKE ((SEM_HANDLE) msg.lParam, OSS_DONT_BLOCK);
	}

    usbQueuePut (semPoolQueue, 0, 0, msg.lParam, OSS_BLOCK);

    return (pUrb->result == OK) ? OK : ERROR;
    }


/***************************************************************************
*
* usbdInitialize - Initialize the USBD
*
* usbdInitialize() must be called at least once prior to calling other
* USBD functions.  usbdInitialize() prepares the USBD to process URBs.
* Calls to usbdInitialize() may be nested, allowing multiple USBD clients
* to be written independently.
*
* RETURNS: OK, or ERROR if initialization failed.
*
* ERRNO:
*  S_usbdLib_GENERAL_FAULT
*  S_usbdLib_OUT_OF_RESOURCES
*  S_usbdLib_INTERNAL_FAULT
*/

STATUS usbdInitialize (void)
    {
    URB_CLIENT_UNREG urb;
    SEM_HANDLE semHandle;
    STATUS s = OK;
    int i;


    /* If not already initialized... */

    if (++initCount == 1)
	{

	/* Initialize osServices */

	if (ossInitialize () != OK)
	    {
	    s = ossStatus (S_usbdLib_GENERAL_FAULT);
	    }
	else
	    {

	    ossInitialized = TRUE;

	    /* Create a pool of semaphores which will be used to synchronize 
	     * URB completion. */

	    if (usbQueueCreate (MAX_SYNCH_SEM, &semPoolQueue) != OK)
		{
		s = ossStatus (S_usbdLib_OUT_OF_RESOURCES);
		}
	    else
		{

		for (i = 0; s == OK && i < MAX_SYNCH_SEM; i++)
		    {
		    if (OSS_SEM_CREATE (1, 0, &semHandle) != OK ||
			usbQueuePut (semPoolQueue, 0, 0, (UINT32) semHandle,
			    OSS_DONT_BLOCK) != OK)
			{
			s = ossStatus (S_usbdLib_INTERNAL_FAULT);
			}
		    }

		/* Initialize USBD core library */

		if (s == OK)
		    {
		    urbInit (&urb.header, NULL, USBD_FNC_INITIALIZE, NULL, NULL,
			sizeof (urb));

		    if ((s = urbExecBlock (&urb.header)) == OK)
			{
			coreLibInitialized = TRUE;
			}
		    }
		}
	    }
	}

    if (s != OK)
	usbdShutdown ();

    return s;
    }


/***************************************************************************
*
* usbdShutdown - Shuts down the USBD
*
* usbdShutdown() should be called once for every successful call to 
* usbdInitialize().  This function frees memory and other resources used
* by the USBD.
*
* RETURNS: OK, or ERROR if shutdown failed.
*
* ERRNO:
*  S_usbdLib_NOT_INITIALIZED
*/

STATUS usbdShutdown (void)
    {
    URB_CLIENT_UNREG urb;
    USB_MESSAGE msg;
    STATUS s = OK;
    int i;

    if (initCount == 0)
	{

	/* Not initialized */

	s = ossStatus (S_usbdLib_NOT_INITIALIZED);
	}
    else
	{

	/* We've been initialized at least once. */

	if (initCount == 1)
	    {

	    if (coreLibInitialized)
		{
		/* Execute Shutdown URB */

		urbInit (&urb.header, NULL, USBD_FNC_SHUTDOWN, NULL, NULL,
		    sizeof (urb));

		urbExecBlock (&urb.header);
		coreLibInitialized = FALSE;
		}

	    if (semPoolQueue != NULL)
		{
		for (i = 0; i < MAX_SYNCH_SEM &&
		    usbQueueGet (semPoolQueue, &msg, SYNCH_SEM_TIMEOUT) == OK; 
		    i++)
		    {
		    OSS_SEM_DESTROY ((SEM_HANDLE) msg.lParam); 
		    }

		usbQueueDestroy (semPoolQueue);
		semPoolQueue = NULL;
		}

	    if (ossInitialized)
		{
		/* Shut down osServices library */

		ossShutdown ();
		ossInitialized = FALSE;
		}
	    }

	--initCount;
	}

    return s;
    }


/***************************************************************************
*
* usbdClientRegister - Registers a new client with the USBD
*
* This routine invokes the USBD function to register a new client.  
* <pClientName> should point to a string of not more than USBD_NAME_LEN 
* characters (excluding terminating NULL) which can be used to uniquely 
* identify the client.	If successful, upon return the <pClientHandle>
* will be filled with a newly assigned USBD_CLIENT_HANDLE.
*
* RETURNS: OK, or ERROR if unable to register new client.
*/

STATUS usbdClientRegister 
    (
    pCHAR pClientName,			/* Client name */
    pUSBD_CLIENT_HANDLE pClientHandle	/* Client hdl returned by USBD */
    )

    {
    URB_CLIENT_REG urb;
    STATUS s;

    
    /* Initialize URB */

    urbInit (&urb.header, 0, USBD_FNC_CLIENT_REG, NULL, NULL, sizeof (urb));
    
    if (pClientName != NULL)
	strncpy (urb.clientName, pClientName, USBD_NAME_LEN);


    /* Execute URB */

    s = urbExecBlock (&urb.header);


    /* Return result */

    if (pClientHandle != NULL)
	*pClientHandle = urb.header.handle;

    return s;
    }


/***************************************************************************
*
* usbdClientUnregister - Unregisters a USBD client
*
* A client invokes this function to release a previously assigned 
* USBD_CLIENT_HANDLE.  The USBD will release all resources allocated to 
* the client, aborting any outstanding URBs which may exist for the client.  
*
* Once this function has been called with a given clientHandle, the client
* must not attempt to reuse the indicated <clientHandle>.
*
* RETURNS: OK, or ERROR if unable to unregister client.
*/

STATUS usbdClientUnregister
    (
    USBD_CLIENT_HANDLE clientHandle	/* Client handle */
    )

    {
    URB_CLIENT_UNREG urb;


    /* Initialize URB */

    urbInit (&urb.header, clientHandle, USBD_FNC_CLIENT_UNREG, NULL, NULL,
	sizeof (urb));


    /* Execute URB */

    return urbExecBlock (&urb.header);
    }


/***************************************************************************
*
* usbdMngmtCallbackSet - sets management callback for a client
*
* Management callbacks provide a mechanism for the USBD to inform clients
* of asynchronous management events on the USB.  For example, if the USB
* is in the SUSPEND state - see usbdBusStateSet() - and a USB device
* drives RESUME signalling, that event can be reported to a client through
* its management callback.
*
* <clientHandle> is a client's registered handled with the USBD.  
* <mngmtCallback> is the management callback routine of type
* USBD_MNGMT_CALLBACK which will be invoked by the USBD when management
* events are detected.	<mngmtCallbackParam> is a client-defined parameter
* which will be passed to the <mngmtCallback> each time it is invoked.
* Passing a <mngmtCallback> of NULL cancels management event callbacks.
*
* When the <mngmtCallback> is invoked, the USBD will also pass it the
* USBD_NODE_ID of the root node on the bus for which the management event
* has been detected and a code signifying the type of management event
* as USBD_MNGMT_xxxx.
*
* Clients are not required to register a management callback routine.
* Clients that do use a management callback are permitted to register at
* most one management callback per USBD_CLIENT_HANDLE.	
*
* RETURNS: OK, or ERROR if unable to register management callback
*/

STATUS usbdMngmtCallbackSet
    (
    USBD_CLIENT_HANDLE clientHandle,	/* Client handle */
    USBD_MNGMT_CALLBACK mngmtCallback,	/* management callback */
    pVOID mngmtCallbackParam		/* client-defined parameter */
    )

    {
    URB_MNGMT_CALLBACK_SET urb;


    /* Initialize URB */

    urbInit (&urb.header, clientHandle, USBD_FNC_MNGMT_CALLBACK_SET, NULL, 
	NULL, sizeof (urb));

    urb.mngmtCallback = mngmtCallback;
    urb.mngmtCallbackParam = mngmtCallbackParam;


    /* Execute URB */

    return urbExecBlock (&urb.header);
    }


/***************************************************************************
*
* usbdBusStateSet - Sets bus state (e.g., suspend/resume)
*
* This function allows a client to set the state of the bus to which
* the specified <nodeId> is attached.  The desired <busState> is specified
* as USBD_BUS_xxxx.
*
* Typically, a client will use this function to set a bus to the SUSPEND
* or RESUME state.  Clients must use this capability with care, as it will
* affect all devices on a given bus - and hence all clients communicating
* with those devices.
*
* When setting a bus to the SUSPEND state, a client must also be aware
* that the USBD will not automatically return the bus to the RESUME state...
* the client must RESUME the bus explicitly through this function.  This
* point is most important when considering the USB "remote wakeup" feature.
* This feature allows a remote device to drive RESUME signalling on the
* bus.	However, it is the client's responsibility to recognize this
* condition by monitoring management events through the use of management
* callbacks - see usbdMngmtCallbackSet().  When a USBD_MNGMT_RESUME event
* is detected, the client chooses to RESUME the bus (or not).
*
* RETURNS: OK, or ERROR if unable to set specified bus state
*/

STATUS usbdBusStateSet
    (
    USBD_CLIENT_HANDLE clientHandle,	/* Client handle */
    USBD_NODE_ID nodeId,		/* node ID */
    UINT16 busState			/* new bus state: USBD_BUS_xxxx */
    )

    {
    URB_BUS_STATE_SET urb;


    /* Initialize URB */

    urbInit (&urb.header, clientHandle, USBD_FNC_BUS_STATE_SET, NULL, 
	NULL, sizeof (urb));

    urb.nodeId = nodeId;
    urb.busState = busState;


    /* Execute URB */

    return urbExecBlock (&urb.header);
    }


/***************************************************************************
*
* usbdBusCountGet - Get number of USBs attached to the host.
*
* This function returns the total number of USB host controllers in the 
* system.  Each host controller has its own root hub as required by the USB 
* specification; and clients planning to enumerate USB devices using the Bus 
* Enumeration Functions need to know the total number of host controllers in 
* order to retrieve the Node Ids for each root hub.
*
* <pBusCount> must point to a UINT16 variable in which the total number of 
* USB host controllers will be stored.
*
* Note: The number of USB host controllers is not constant.  Bus controllers 
* can be added by calling usbdHcdAttach() and removed by calling 
* usbdHcdDetach().  Again, the Dynamic Attach Functions deal with these 
* situations automatically, and are the preferred mechanism by which most 
* clients should be informed of device attachment and removal.
*
* RETURNS: OK, or ERROR if unable to retrieve bus count
*/

STATUS usbdBusCountGet
    (
    USBD_CLIENT_HANDLE clientHandle,	/* Client handle */
    pUINT16 pBusCount			/* Word bfr to receive bus count */
    )

    {
    URB_BUS_COUNT_GET urb;
    STATUS s;


    /* Initalize URB */

    urbInit (&urb.header, clientHandle, USBD_FNC_BUS_COUNT_GET, NULL, NULL,
	sizeof (urb));


    /* Execute URB */

    s = urbExecBlock (&urb.header);


    /* Return result */

    if (pBusCount != NULL)
	*pBusCount = urb.busCount;

    return s;
    }


/***************************************************************************
*
* usbdRootNodeIdGet - Returns root node for a specific USB
*
* This function returns the Node Id for the root hub for the specified 
* USB host controller.	<busIndex> is the index of the desired USB host 
* controller.  The first host controller is index 0 and the last host 
* controller's index is the total number of USB host controllers - as 
* returned by usbdBusCountGet() - minus 1. < pRootId> must point to a 
* USBD_NODE_ID variable in which the Node Id of the root hub will be stored.
* 
* RETURNS: OK, or ERROR if unable to get root node ID.
*/

STATUS usbdRootNodeIdGet
    (
    USBD_CLIENT_HANDLE clientHandle,	/* Client handle */
    UINT16 busIndex,			/* Bus index */
    pUSBD_NODE_ID pRootId		/* bfr to receive Root Id */
    )

    {
    URB_ROOT_ID_GET urb;
    STATUS s;


    /* Initalize URB */

    urbInit (&urb.header, clientHandle, USBD_FNC_ROOT_ID_GET, NULL, NULL,
	sizeof (urb));

    urb.busIndex = busIndex;


    /* Execute URB */

    s = urbExecBlock (&urb.header);


    /* Return result */

    if (pRootId != NULL)
	*pRootId = urb.rootId;

    return s;
    }


/***************************************************************************
*
* usbdHubPortCountGet - Returns number of ports connected to a hub
*
* usbdHubPortCountGet() provides clients with a convenient mechanism to 
* retrieve the number of downstream ports provided by the specified hub. 
* Clients can also retrieve this information by retrieving configuration 
* descriptors from the hub using the Configuration Functions describe in 
* a following section.	
*
* <hubId> must be the Node Id for the desired USB hub.	An error will be 
* returned if <hubId> does not refer to a hub.	<pPortCount> must point to 
* a UINT16 variable in which the total number of ports on the specified 
* hub will be stored.
*
* RETURNS: OK, or ERROR if unable to get hub port count.
*/

STATUS usbdHubPortCountGet
    (
    USBD_CLIENT_HANDLE clientHandle,	/* Client handle */
    USBD_NODE_ID hubId, 		/* Node Id for desired hub */
    pUINT16 pPortCount			/* bfr to receive port count */
    )

    {
    URB_HUB_PORT_COUNT_GET urb;
    STATUS s;


    /* Initalize URB */

    urbInit (&urb.header, clientHandle, USBD_FNC_HUB_PORT_COUNT_GET, NULL, NULL,
	sizeof (urb));

    urb.hubId = hubId;


    /* Execute URB */

    s = urbExecBlock (&urb.header);


    /* Return result */

    if (pPortCount != NULL)
	*pPortCount = urb.portCount;

    return s;
    }


/***************************************************************************
*
* usbdNodeIdGet - Gets the id of the node connected to a hub port
*
* Clients use this function to retrieve the Node Id for devices attached to
* each of a hub’s ports.  <hubId> and <portIndex> identify the hub and port 
* to which a device may be attached.  <pNodeType> must point to a UINT16 
* variable to receive a type code as follows:
*
* .IP "USB_NODETYPE_NONE"
* No device is attached to the specified port.
* .IP "USB_NODETYPE_HUB"
* A hub is attached to the specified port.
* .IP "USB_NODETYPE_DEVICE"
* A device (non-hub) is attached to the specified port.
*
* If the node type is returned as USBD_NODE_TYPE_NONE, then a Node Id is 
* not returned and the value returned in <pNodeId> is undefined.  If the 
* node type indicates a hub or device is attached to the port, then 
* <pNodeId> will contain that hub or device’s nodeId upon return.
*
* RETURNS: OK, or ERROR if unable to get node ID.
*/

STATUS usbdNodeIdGet
    (
    USBD_CLIENT_HANDLE clientHandle,	/* Client handle */
    USBD_NODE_ID hubId, 		/* Node Id for desired hub */
    UINT16 portIndex,			/* Port index */
    pUINT16 pNodeType,			/* bfr to receive node type */
    pUSBD_NODE_ID pNodeId		/* bfr to receive Node Id */
    )

    {
    URB_NODE_ID_GET urb;
    STATUS s;


    /* Initalize URB */

    urbInit (&urb.header, clientHandle, USBD_FNC_NODE_ID_GET, NULL, NULL,
	sizeof (urb));

    urb.hubId = hubId;
    urb.portIndex = portIndex;


    /* Execute URB */

    s = urbExecBlock (&urb.header);


    /* Return result */

    if (pNodeType != NULL)
	*pNodeType = urb.nodeType;

    if (pNodeId != NULL)
	*pNodeId = urb.nodeId;

    return s;
    }


/***************************************************************************
*
* usbdNodeInfoGet - Returns information about a USB node
*
* This function retrieves information about the USB device specified by 
* <nodeId>.  The USBD copies node information into the <pNodeInfo> structure 
* provided by the caller.  This structure is of the form USBD_NODEINFO as
* shown below:
*
* .CS
* typedef struct usbd_nodeinfo
*     {
*     UINT16 nodeType;
*     UINT16 nodeSpeed;
*     USBD_NODE_ID parentHubId;
*     UINT16 parentHubPort;
*     USBD_NODE_ID rootId;
*     } USBD_NODEINFO, *pUSBD_NODEINFO;
* .CE
*
* <nodeType> specifies the type of node identified by <nodeId> and is defined 
* as USB_NODETYPE_xxxx.  <nodeSpeed> identifies the speed of the device and 
* is defined as USB_SPEED_xxxx.  <parentHubId> and <parentHubPort> identify 
* the Node Id and port of the hub to which the indicated node is attached 
* upstream.  If the indicated <nodeId> happens to be a root hub, then 
* <parentHubId> and <parentHubPort> will both be 0.  
*
* Similarly, <rootId> identifies the Node Id of the root hub for the USB to 
* which nodeId is attached.  If <nodeId> itself happens to be the root hub, 
* then the same value will be returned in <rootId>.
*
* It is anticipated that this structure may grow over time.  To provide 
* backwards compatibility, the client must pass the total size of the 
* USBD_NODEINFO structure it has allocated in <infoLen>.  The USBD will copy 
* fields into this structure only up to the <infoLen> indicated by the caller.
*
* RETURNS: OK, or ERROR if unable to retrieve node information.
*/

STATUS usbdNodeInfoGet
    (
    USBD_CLIENT_HANDLE clientHandle,	/* Client handle */
    USBD_NODE_ID nodeId,		/* Node Id of device/hub */
    pUSBD_NODE_INFO pNodeInfo,		/* Structure to receive node info */
    UINT16 infoLen			/* Len of bfr allocated by client */
    )

    {
    URB_NODE_INFO_GET urb;


    /* Initalize URB */

    urbInit (&urb.header, clientHandle, USBD_FNC_NODE_INFO_GET, NULL, NULL,
	sizeof (urb));

    urb.nodeId = nodeId;
    urb.pNodeInfo = pNodeInfo;
    urb.infoLen = infoLen;


    /* Execute URB */

    return urbExecBlock (&urb.header);
    }


/***************************************************************************
*
* usbdDynamicAttachRegister - Registers client for dynamic attach notification
*
* Clients call this function to indicate to the USBD that they wish to be 
* notified whenever a device of the indicated class/sub-class/protocol is attached 
* or removed from the USB.  A client may specify that it wants to receive 
* notification for an entire device class or only for specific sub-classes
* within that class.
*
* <deviceClass>, <deviceSubClass>, and <deviceProtocol> must specify a USB 
* class/sub-class/protocol combination according to the USB specification.  For 
* the client’s convenience, usbdLib.h automatically includes usb.h which defines a 
* number of USB device classes as USB_CLASS_xxxx and USB_SUBCLASS_xxxx.  A 
* value of USBD_NOTIFY_ALL in any/all of these parameters acts like a wildcard
* and matches any value reported by the device for the corresponding field. 
*
* <attachCallback> must be a non-NULL pointer to a client-supplied callback 
* routine of the form USBD_ATTACH_CALLBACK:
*
* .CS
* typedef VOID (*USBD_ATTACH_CALLBACK) 
*     (
*     USBD_NODE_ID nodeId, 
*     UINT16 attachAction, 
*     UINT16 configuration,
*     UINT16 interface,
*     UINT16 deviceClass, 
*     UINT16 deviceSubClass, 
*     UINT16 deviceProtocol
*     );
* .CE
*
* Immediately upon registration the client should expect that it may begin 
* receiving calls to the <attachCallback> routine.  Upon registration, USBD 
* will call the <attachCallback> for each device of the specified class which 
* is already attached to the system.  Thereafter, the USBD will call the 
* <attachCallback> whenever a new device of the specified class is attached to 
* the system or an already-attached device is removed.
*
* Each time the <attachCallback> is called, USBD will pass the Node Id of the 
* device in <nodeId> and an attach code in <attachAction> which explains the 
* reason for the callback.  Attach codes are defined as:
*
* .IP "USBD_DYNA_ATTACH"
* USBD is notifying the client that nodeId is a device which is now attached 
* to the system.
* .IP "USBD_DYNA_REMOVE"
* USBD is notifying the client that nodeId has been detached (removed) from 
* the system.
*
* When the <attachAction> is USBD_DYNA_REMOVE the <nodeId> refers to a Node Id 
* which is no longer valid.  The client should interrogate its internal data 
* structures and delete any references to the specified Node Id.  If the client 
* had outstanding requests to the specified <nodeId>, such as data transfer 
* requests, then the USBD will fail those outstanding requests prior to calling 
* the <attachCallback> to notify the client that the device has been removed.  
* In general, therefore, transfer requests related to removed devices should 
* already be taken care of before the <attachCallback> is called.
*
* A client may re-use a single <attachCallback> for multiple notification 
* registrations.  As a convenience to the <attachCallback> routine, the USBD 
* also passes the <deviceClass>, <deviceSubClass>, and <deviceProtocol> of the 
* attached/removed <nodeId> each time it calls the <attachCallback>.  
*
* Finally, clients need to be aware that not all USB devices report their class
* information at the "device" level.  Rather, some devices report class types
* on an interface-by-interface basis.  When the device reports class information
* at the device level, then the USBD passes a <configuration> value of zero to 
* the attach callback and calls the callback only a single time for each device.
* When the device reports class information at the interface level, then the
* USBD invokes the attach callback once for each interface which matches the
* client's <deviceClass>/<deviceSubClass>/<deviceProtocol> specification.  In this
* case, the USBD also passes the corresponding configuration & interface numbers
* in <configuration> and <interface> each time it invokes the callback.
*
* RETURNS: OK, or ERROR if unable to register for attach/removal notification.
*/

STATUS usbdDynamicAttachRegister
    (
    USBD_CLIENT_HANDLE clientHandle,	/* Client handle */
    UINT16 deviceClass, 		/* USB class code */
    UINT16 deviceSubClass,		/* USB sub-class code */
    UINT16 deviceProtocol,		/* USB device protocol code */
    USBD_ATTACH_CALLBACK attachCallback /* User-supplied callback routine */
    )

    {
    URB_DYNA_ATTACH_REG_UNREG urb;


    /* Initalize URB */

    urbInit (&urb.header, clientHandle, USBD_FNC_DYNA_ATTACH_REG, NULL, NULL,
	sizeof (urb));

    urb.deviceClass = deviceClass;
    urb.deviceSubClass = deviceSubClass;
    urb.deviceProtocol = deviceProtocol;
    urb.attachCallback = attachCallback;


    /* Execute URB */

    return urbExecBlock (&urb.header);
    }


/***************************************************************************
*
* usbdDynamicAttachUnRegister - Unregisters client for attach notification
*
* This function cancels a client’s earlier request to be notified for the 
* attachment and removal of devices within the specified class.  <deviceClass>,
* <deviceSubClass>, <deviceProtocol>, and <attachCallback> are defined as for the 
* usbdDynamicAttachRegister() function and must match exactly the parameters
* passed in an earlier call to usbdDynamicAttachRegister.
*
* RETURNS: OK, or ERROR if unable to unregister for attach/removal notification.
*/

STATUS usbdDynamicAttachUnRegister
    (
    USBD_CLIENT_HANDLE clientHandle,	/* Client handle */
    UINT16 deviceClass, 		/* USB class code */
    UINT16 deviceSubClass,		/* USB sub-class code */
    UINT16 deviceProtocol,		/* USB device protocol code */
    USBD_ATTACH_CALLBACK attachCallback /* user-supplied callback routine */
    )

    {
    URB_DYNA_ATTACH_REG_UNREG urb;


    /* Initalize URB */

    urbInit (&urb.header, clientHandle, USBD_FNC_DYNA_ATTACH_UNREG, NULL, 
	NULL, sizeof (urb));

    urb.deviceClass = deviceClass;
    urb.deviceSubClass = deviceSubClass;
    urb.deviceProtocol = deviceProtocol;
    urb.attachCallback = attachCallback;


    /* Execute URB */

    return urbExecBlock (&urb.header);
    }


/***************************************************************************
*
* usbdFeatureClear - Clears a USB feature
*
* This function allows a client to "clear" a USB feature.  <nodeId> specifies 
* the Node Id of the desired device and <requestType> specifies whether the 
* feature is related to the device, to an interface, or to an endpoint as:
*
* .IP "USB_RT_DEVICE"
* Device
* .IP "USB_RT_INTERFACE"
* Interface
* .IP "USB_RT_ENDPOINT"
* Endpoint
*
* <requestType> also specifies if the request is standard, class-specific,
* etc., as:
*
* .IP "USB_RT_STANDARD"
* Standard
* .IP "USB_RT_CLASS"
* Class-specific
* .IP "USB_RT_VENDOR"
* Vendor-specific
*
* For example, USB_RT_STANDARD | USB_RT_DEVICE in <requestType> specifies a
* standard device request.  
*
* The client must pass the device’s feature selector in <feature>.  If 
* <featureType> specifies an interface or endpoint, then <index> must contain 
* the interface or endpoint index.  <index> should be zero when <featureType> 
* is USB_SELECT_DEVICE.
*
* RETURNS: OK, or ERROR if unable to clear feature.
*/

STATUS usbdFeatureClear
    (
    USBD_CLIENT_HANDLE clientHandle,	/* Client handle */
    USBD_NODE_ID nodeId,		/* Node Id of device/hub */
    UINT16 requestType, 		/* Selects request type */
    UINT16 feature,			/* Feature selector */
    UINT16 index			/* Interface/endpoint index */
    )

    {
    URB_FEATURE_CLEAR_SET urb;


    /* Initalize URB */

    urbInit (&urb.header, clientHandle, USBD_FNC_FEATURE_CLEAR, NULL, NULL,
	sizeof (urb));

    urb.nodeId = nodeId;
    urb.requestType = requestType;
    urb.feature = feature;
    urb.index = index;


    /* Execute URB */

    return urbExecBlock (&urb.header);
    }


/***************************************************************************
*
* usbdFeatureSet - Sets a USB feature
*
* This function allows a client to "set" a USB feature.  <nodeId> specifies 
* the Node Id of the desired device and <requestType> specifies the nature
* of the feature feature as defined for the usbdFeatureClear() function.
* 
* The client must pass the device’s feature selector in <feature>.  If 
* <requestType> specifies an interface or endpoint, then <index> must contain 
* the interface or endpoint index.  <index> should be zero when <requestType>
* includes USB_SELECT_DEVICE.
*
* RETURNS: OK, or ERROR if unable to set feature.
*/

STATUS usbdFeatureSet
    (
    USBD_CLIENT_HANDLE clientHandle,	/* Client handle */
    USBD_NODE_ID nodeId,		/* Node Id of device/hub */
    UINT16 requestType, 		/* Selects request type */
    UINT16 feature,			/* Feature selector */
    UINT16 index			/* Interface/endpoint index */
    )

    {
    URB_FEATURE_CLEAR_SET urb;


    /* Initalize URB */

    urbInit (&urb.header, clientHandle, USBD_FNC_FEATURE_SET, NULL, NULL,
	sizeof (urb));

    urb.nodeId = nodeId;
    urb.requestType = requestType;
    urb.feature = feature;
    urb.index = index;


    /* Execute URB */

    return urbExecBlock (&urb.header);
    }


/***************************************************************************
*
* usbdConfigurationGet - Gets USB configuration for a device
*
* This function returns the currently selected configuration for the device 
* or hub indicated by <nodeId>.  The current configuration value is returned 
* in the low byte of <pConfiguration>.	The high byte is currently reserved 
* and will be 0.
*
* RETURNS: OK, or ERROR if unable to get configuration.
*/

STATUS usbdConfigurationGet
    (
    USBD_CLIENT_HANDLE clientHandle,	/* Client handle */
    USBD_NODE_ID nodeId,		/* Node Id of device/hub */
    pUINT16 pConfiguration		/* bfr to receive config value */
    )

    {
    URB_CONFIG_GET_SET urb;
    STATUS s;


    /* Initalize URB */

    urbInit (&urb.header, clientHandle, USBD_FNC_CONFIG_GET, NULL, NULL,
	sizeof (urb));

    urb.nodeId = nodeId;


    /* Execute URB */

    s = urbExecBlock (&urb.header);


    /* Return result */

    if (pConfiguration != NULL)
	*pConfiguration = urb.configuration;

    return s;
    }


/***************************************************************************
*
* usbdConfigurationSet - Sets USB configuration for a device
*
* This function sets the current configuration for the device identified 
* by <nodeId>.	The client should pass the desired configuration value in 
* the low byte of <configuration>.  The high byte is currently reserved and 
* should be 0.
*
* The client must also pass the maximum current which will be used by this
* configuration in <maxPower>.	Typically, the maximum power will be
* determined by reading the corresponding configuration descriptor for a
* device.  Prior to setting the chosen configuration, the USBD will verify
* that the hub port to which <pNode> is connected is capable of providing
* the requested current.  If the port cannot provide <maxPower>, then
* the configuration will not be set and the function will return an error.
* <maxPower> should be expressed in milliamps (e.g., 100 = 100mA).  For
* self-powered devices, the caller may pass zero in <maxPower>.
*
* RETURNS: OK, or ERROR if unable to set configuration.
*/

STATUS usbdConfigurationSet
    (
    USBD_CLIENT_HANDLE clientHandle,	/* Client handle */
    USBD_NODE_ID nodeId,		/* Node Id of device/hub */
    UINT16 configuration,		/* New configuration to be set */
    UINT16 maxPower			/* max power this config will draw */
    )

    {
    URB_CONFIG_GET_SET urb;


    /* Initalize URB */

    urbInit (&urb.header, clientHandle, USBD_FNC_CONFIG_SET, NULL, NULL,
	sizeof (urb));

    urb.nodeId = nodeId;
    urb.configuration = configuration;
    urb.maxPower = maxPower;


    /* Execute URB */

    return urbExecBlock (&urb.header);
    }


/***************************************************************************
*
* usbdDescriptorGet - Retrieves a USB descriptor
*
* A client uses this function to retrieve a descriptor from the USB device 
* identified by <nodeId>.  <requestType> is defined as documented for the
* usbdFeatureClear() function.	<descriptorType> specifies the type of the 
* descriptor to be retrieved and must be one of the following values:
*
* .IP "USB_DESCR_DEVICE"
* Specifies the DEVICE descriptor.
* .IP "USB_DESCR_CONFIG"
* Specifies the CONFIGURATION descriptor.
* .IP "USB_DESCR_STRING"
* Specifies a STRING descriptor.
* .IP "USB_DESCR_INTERFACE"
* Specifies an INTERFACE descriptor.
* .IP "USB_DESCR_ENDPOINT"
* Specifies an ENDPOINT descriptor.
*
* <descriptorIndex> is the index of the desired descriptor.
*
* For string descriptors the <languageId> should specify the desired 
* language for the string.  According to the USB Specification, strings 
* descriptors are returned in UNICODE format and the <languageId> should 
* be the "sixteen-bit language ID (LANGID) defined by Microsoft for 
* Windows as described in .I "Developing International Software for Windows 
* 95 and Windows NT."  Please refer to Section 9.6.5 of revision 1.1 of the 
* USB Specification for more detail.  For device and configuration 
* descriptors, <languageId> should be 0.
*
* The caller must provide a buffer to receive the descriptor data.  <pBfr> 
* is a pointer to a caller-supplied buffer of length <bfrLen>.	If the 
* descriptor is too long to fit in the buffer provided, the descriptor will 
* be truncated.  If a non-NULL pointer is passed in <pActLen>, the actual 
* length of the data transferred will be stored in <pActLen> upon return.
*
* RETURNS: OK, or ERROR if unable to get descriptor.
*/

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
    )

    {
    URB_DESCRIPTOR_GET_SET urb;
    STATUS s;


    /* Initalize URB */

    urbInit (&urb.header, clientHandle, USBD_FNC_DESCRIPTOR_GET, NULL, NULL,
	sizeof (urb));

    urb.nodeId = nodeId;
    urb.requestType = requestType;
    urb.descriptorType = descriptorType;
    urb.descriptorIndex = descriptorIndex;
    urb.languageId = languageId;
    urb.bfrLen = bfrLen;
    urb.pBfr = pBfr;


    /* Execute URB */

    s = urbExecBlock (&urb.header);


    /* Return result */

    if (pActLen != NULL)
	*pActLen = urb.actLen;

    return s;
    }


/***************************************************************************
*
* usbdDescriptorSet - Sets a USB descriptor
*
* A client uses this function to set a descriptor on the USB device identified 
* by <nodeId>.	The parameters <requestType>, <descriptorType>, 
* <descriptorIndex>, and <languageId> are the same as those described for the 
* usbdDescriptorGet() function.  <pBfr> is a pointer to a buffer of length 
* <bfrLen> which contains the descriptor data to be sent to the device.
*
* RETURNS: OK, or ERROR if unable to set descriptor.
*/

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
    )

    {
    URB_DESCRIPTOR_GET_SET urb;


    /* Initalize URB */

    urbInit (&urb.header, clientHandle, USBD_FNC_DESCRIPTOR_SET, NULL, NULL,
	sizeof (urb));

    urb.nodeId = nodeId;
    urb.requestType = requestType;
    urb.descriptorType = descriptorType;
    urb.descriptorIndex = descriptorIndex;
    urb.languageId = languageId;
    urb.bfrLen = bfrLen;
    urb.pBfr = pBfr;


    /* Execute URB */

    return urbExecBlock (&urb.header);
    }


/***************************************************************************
*
* usbdInterfaceGet - Retrieves a device's current interface
*
* This function allows a client to query the current alternate setting for 
* a given device’s interface.  <nodeId> and <interfaceIndex> specify the 
* device and interface to be queried, respectively.  <pAlternateSetting> 
* points to a UINT16 variable in which the alternate setting will be stored 
* upon return.
*
* RETURNS: OK, or ERROR if unable to get interface.
*/

STATUS usbdInterfaceGet
    (
    USBD_CLIENT_HANDLE clientHandle,	/* Client handle */
    USBD_NODE_ID nodeId,		/* Node Id of device/hub */
    UINT16 interfaceIndex,		/* Index of interface */
    pUINT16 pAlternateSetting		/* Current alternate setting */
    )

    {
    URB_INTERFACE_GET_SET urb;
    STATUS s;


    /* Initalize URB */

    urbInit (&urb.header, clientHandle, USBD_FNC_INTERFACE_GET, NULL, NULL,
	sizeof (urb));

    urb.nodeId = nodeId;
    urb.interfaceIndex = interfaceIndex;


    /* Execute URB */

    s = urbExecBlock (&urb.header);

    /* Return result */

    if (pAlternateSetting != NULL)
	*pAlternateSetting = urb.alternateSetting;

    return s;
    }


/***************************************************************************
*
* usbdInterfaceSet - Sets a device's current interface
*
* This function allows a client to select an alternate setting for a given 
* device’s interface.  <nodeId> and <interfaceIndex> specify the device and 
* interface to be modified, respectively.  <alternateSetting> specifies the 
* new alternate setting.
*
* RETURNS: OK, or ERROR if unable to set interface.
*/

STATUS usbdInterfaceSet
    (
    USBD_CLIENT_HANDLE clientHandle,	/* Client handle */
    USBD_NODE_ID nodeId,		/* Node Id of device/hub */
    UINT16 interfaceIndex,		/* Index of interface */
    UINT16 alternateSetting		/* Alternate setting */
    )

    {
    URB_INTERFACE_GET_SET urb;


    /* Initalize URB */

    urbInit (&urb.header, clientHandle, USBD_FNC_INTERFACE_SET, NULL, NULL,
	sizeof (urb));

    urb.nodeId = nodeId;
    urb.interfaceIndex = interfaceIndex;
    urb.alternateSetting = alternateSetting;


    /* Execute URB */

    return urbExecBlock (&urb.header);
    }


/***************************************************************************
*
* usbdStatusGet - Retrieves USB status from a device/interface/etc.
*
* This function retrieves the current status from the device indicated
* by <nodeId>.	<requestType> indicates the nature of the desired status
* as documented for the usbdFeatureClear() function.
*
* The status word is returned in <pBfr>.  The meaning of the status  
* varies depending on whether it was queried from the device, an interface, 
* or an endpoint, class-specific function, etc. as described in the USB 
* Specification.
*
* RETURNS: OK, or ERROR if unable to get status.
*/

STATUS usbdStatusGet
    (
    USBD_CLIENT_HANDLE clientHandle,	/* Client handle */
    USBD_NODE_ID nodeId,		/* Node Id of device/hub */
    UINT16 requestType, 		/* Selects device/interface/endpoint */
    UINT16 index,			/* Interface/endpoint index */
    UINT16 bfrLen,			/* length of bfr */
    pUINT8 pBfr,			/* bfr to receive status */
    pUINT16 pActLen			/* bfr to receive act len xfr'd */
    )

    {
    URB_STATUS_GET urb;
    STATUS s;


    /* Initalize URB */

    urbInit (&urb.header, clientHandle, USBD_FNC_STATUS_GET, NULL, NULL,
	sizeof (urb));

    urb.nodeId = nodeId;
    urb.requestType = requestType;
    urb.index = index;
    urb.bfrLen = bfrLen;
    urb.pBfr = pBfr;


    /* Execute URB */

    s = urbExecBlock (&urb.header);


    /* Return result */

    if (pActLen != NULL)
	*pActLen = urb.actLen;

    return s;
    }


/***************************************************************************
*
* usbdAddressGet - Gets the USB address for a given device
*
* This function returns the USB address assigned to device specified by 
* <nodeId>.  USB addresses are assigned by the USBD, so there is generally 
* no need for clients to query the USB device address.	Furthermore, this 
* function has no counterpart in the USB functions described in Chapter 9 
* of the USB Specification.
*
* The USBD assigns device addresses such that they are unique within the 
* scope of each USB host controller.  However, it is possible that two or 
* more devices attached to different USB host controllers may have identical 
* addresses.
*
* RETURNS: OK, or ERROR if unable to set address.
*/

STATUS usbdAddressGet
    (
    USBD_CLIENT_HANDLE clientHandle,	/* Client handle */
    USBD_NODE_ID nodeId,		/* Node Id of device/hub */
    pUINT16 pDeviceAddress		/* Currently assigned device address */
    )

    {
    URB_ADDRESS_GET_SET urb;
    STATUS s;


    /* Initalize URB */

    urbInit (&urb.header, clientHandle, USBD_FNC_ADDRESS_GET, NULL, NULL,
	sizeof (urb));

    urb.nodeId = nodeId;


    /* Execute URB */

    s = urbExecBlock (&urb.header);


    /* Return result */

    if (pDeviceAddress != NULL)
	*pDeviceAddress = urb.deviceAddress;

    return s;
    }


/***************************************************************************
*
* usbdAddressSet - Sets the USB address for a given device
*
* This function sets the USB address at which a device will respond to future 
* requests.  Upon return, the address of the device identified by <nodeId> 
* will be changed to the value specified in <deviceAddress>.  <deviceAddress> 
* must be in the range from 0..127.  The <deviceAddress> must also be unique 
* within the scope of each USB host controller.
*
* The USBD manages USB device addresses automatically, and this function 
* should never be called by normal USBD clients.  Changing a device address 
* may cause serious problems, including device address conflicts, and may 
* cause the USB to cease operation.
*
* RETURNS: OK, or ERROR if unable to get current device address.
*/

STATUS usbdAddressSet
    (
    USBD_CLIENT_HANDLE clientHandle,	/* Client handle */
    USBD_NODE_ID nodeId,		/* Node Id of device/hub */
    UINT16 deviceAddress		/* New device address */
    )

    {
    URB_ADDRESS_GET_SET urb;


    /* Initalize URB */

    urbInit (&urb.header, clientHandle, USBD_FNC_ADDRESS_SET, NULL, NULL,
	sizeof (urb));

    urb.nodeId = nodeId;
    urb.deviceAddress = deviceAddress;


    /* Execute URB */

    return urbExecBlock (&urb.header);
    }


/***************************************************************************
*
* usbdVendorSpecific - Allows clients to issue vendor-specific USB requests
*
* Certain devices may implement vendor-specific USB requests which cannot 
* be generated using the standard functions described elsewhere.  This 
* function allows a client to specify directly the exact parameters for a 
* USB control pipe request.
*
* <requestType>, <request>, <value>, <index>, and <length> correspond 
* exactly to the bmRequestType, bRequest, wValue, wIndex, and wLength fields 
* defined by the USB Specfication.  If <length> is greater than zero, then 
* <pBfr> must be a non-NULL pointer to a data buffer which will provide or 
* accept data, depending on the direction of the transfer. 
*
* Vendor specific requests issued through this function are always directed 
* to the control pipe of the device specified by <nodeId>.  This function
* formats and sends a Setup packet based on the parameters provided.  If a 
* non-NULL <pBfr> is also provided, then additional IN or OUT transfers
* will be performed following the Setup packet.  The direction of these
* transfers is inferred from the direction bit in the <requestType> param.
* For IN transfers, the actual length of the data transferred will be
* stored in <pActLen> if <pActLen> is not NULL.
*
* RETURNS: OK, or ERROR if unable to execute vendor-specific request.
*/

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
    )

    {
    URB_VENDOR_SPECIFIC urb;
    STATUS s;


    /* Initalize URB */

    urbInit (&urb.header, clientHandle, USBD_FNC_VENDOR_SPECIFIC, NULL, NULL,
	sizeof (urb));

    urb.nodeId = nodeId;
    urb.requestType = requestType;
    urb.request = request;
    urb.value = value;
    urb.index = index;
    urb.length = length;
    urb.pBfr = pBfr;


    /* Execute URB */

    s = urbExecBlock (&urb.header);


    /* return results */

    if (pActLen != NULL)
	*pActLen = urb.actLen;

    return s;
    }


/***************************************************************************
*
* usbdPipeCreate - Creates a USB pipe for subsequent transfers
*
* This function establishes a pipe which can subsequently be used by a 
* client to exchange data with a USB device endpoint.  
*
* <nodeId> and <endpoint> identify the device and device endpoint, 
* respectively, to which the pipe should be "connected."  <configuration>
* and <interface> specify the configuration and interface, respectively,
* with which the pipe is associated.  (The USBD uses this information to
* keep track of "configuration events" associated with the pipe).
*
* <transferType> specifies the type of data transfers for which this pipe 
* will be used:
*
* .IP "USB_XFRTYPE_CONTROL"
* Control transfer pipe (message).
* .IP "USB_XFRTYPE_ISOCH"
* Isochronous transfer pipe (stream).
* .IP "USB_XFRTYPE_INTERRUPT"
* Interrupt transfer pipe (stream).
* .IP "USB_XFRTYPE_BULK"
* Bulk transfer pipe (stream).
*
* <direction> specifies the direction of the pipe as:
*
* .IP "USB_DIR_IN"
* Data moves from device to host.
* .IP "USB_DIR_OUT"
* Data moves from host to device.
* .IP "USB_DIR_INOUT"
* Data moves bidirectionally (message pipes only).
*
* If the <direction> is specified as USB_DIR_INOUT, the USBD assumes that 
* both the IN and OUT endpoints identified by endpoint will be used by 
* this pipe (see the discussion of message pipes in Chapter 5 of the USB 
* Specification).  USB_DIR_INOUT may be specified only for Control pipes.
*
* <maxPayload> specifies the largest data payload supported by this endpoint.  
* Normally a USB device will declare the maximum payload size it supports on 
* each endpoint in its configuration descriptors.  The client will typically 
* read these descriptors using the USBD Configuration Functions and then 
* parse the descriptors to retrieve the appropriate maximum payload value.
* 
* <bandwidth> specifies the bandwidth required for this pipe.  For control
* and bulk pipes, this parameter should be 0.  For interrupt pipes, this
* parameter should express the number of bytes per frame to be transferred.
* for isochronous pipes, this parameter should express the number of bytes
* per second to be transferred.
*
* <serviceInterval> specifies the maximum latency for the pipe in 
* milliseconds.  So, if a pipe needs to be serviced, for example, at least 
* every 20 milliseconds, then the <serviceInterval> value should be 20.  The 
* <serviceInterval> parameter is required only for interrupt pipes.  For 
* other types of pipes, <serviceInterval> should be 0.
*
* If the USBD succeeds in creating the pipe it returns a pipe handle in 
* <pPipeHandle>.  The client must use the pipe handle to identify the pipe 
* in subsequent calls to the USBD Transfer Functions.  If there is 
* insufficient bus bandwidth available to create the pipe (as might happen 
* for an isochronous or interrupt pipe), then the USBD will return an error 
* and a NULL handle in <pPipeHandle>.
*
* RETURNS: OK, or ERROR if pipe could not be create
*/

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
    )

    {
    URB_PIPE_CREATE urb;
    STATUS s;


    /* Initalize URB */

    urbInit (&urb.header, clientHandle, USBD_FNC_PIPE_CREATE, NULL, NULL,
	sizeof (urb));

    urb.nodeId = nodeId;
    urb.endpoint = endpoint;
    urb.configuration = configuration;
    urb.interface = interface;
    urb.transferType = transferType;
    urb.direction = direction;
    urb.maxPayload = maxPayload;
    urb.bandwidth = bandwidth;
    urb.serviceInterval = serviceInterval;


    /* Execute URB */

    s = urbExecBlock (&urb.header);


    /* Return result */

    if (pPipeHandle != NULL)
	*pPipeHandle = urb.pipeHandle;

    return s;
    }


/***************************************************************************
*
* usbdPipeDestroy - Destroys a USB data transfer pipe
*
* This function destroys a pipe previous created by calling usbdPipeCreate().  
* The caller must pass the <pipeHandle> originally returned by usbdPipeCreate().
*
* If there are any outstanding transfer requests for the pipe, the USBD will 
* abort those transfers prior to destroying the pipe.  
*
* RETURNS: OK, or ERROR if unable to destroy pipe.
*/

STATUS usbdPipeDestroy
    (
    USBD_CLIENT_HANDLE clientHandle,	/* Client handle */
    USBD_PIPE_HANDLE pipeHandle 	/* pipe handle */
    )

    {
    URB_PIPE_DESTROY urb;


    /* Initalize URB */

    urbInit (&urb.header, clientHandle, USBD_FNC_PIPE_DESTROY, NULL, NULL,
	sizeof (urb));

    urb.pipeHandle = pipeHandle;


    /* Execute URB */

    return urbExecBlock (&urb.header);
    }


/***************************************************************************
*
* usbdTransfer - Initiates a transfer on a USB pipe
*
* A client uses this function to initiate an transfer on the pipe indicated 
* by <pipeHandle>.  The transfer is described by an IRP, or I/O request 
* packet, which must be allocated and initialized by the caller prior to 
* invoking usbdTransfer().
*
* The USB_IRP structure is defined in usb.h as:
*
* .CS
* typedef struct usb_bfr_list
*     {
*     UINT16 pid;
*     pUINT8 pBfr;
*     UINT16 bfrLen;
*     UINT16 actLen;
*     } USB_BFR_LIST;
*
* typedef struct usb_irp
*     {
*     LINK usbdLink;		    // used by USBD
*     pVOID usbdPtr;		    // used by USBD
*     LINK hcdLink;		    // used by HCD
*     pVOID hcdPtr;		    // used by HCD
*     pVOID userPtr;
*     UINT16 irpLen;
*     int result;		    // returned by USBD/HCD
*     IRP_CALLBACK usbdCallback;    // used by USBD
*     IRP_CALLBACK userCallback;
*     UINT16 dataToggle;	    // filled in by USBD
*     UINT16 flags; 
*     UINT32 timeout;		    // defaults to 5 seconds if zero
*     UINT16 startFrame;
*     UINT16 transferLen;
*     UINT16 dataBlockSize;
*     UINT16 bfrCount;
*     USB_BFR_LIST bfrList [1];
*     } USB_IRP, *pUSB_IRP;
* .CE
*
* The length of the USB_IRP structure must be stored in <irpLen> and varies 
* depending on the number of <bfrList> elements allocated at the end of the 
* structure.  By default, the default structure contains a single <bfrList>
* element, but clients may allocate a longer structure to accommodate a larger 
* number of <bfrList> elements.  
*
* <flags> define additional transfer options.  The currently defined flags are:
*
* .IP "USB_FLAG_SHORT_OK"
* Treats receive (IN) data underrun as OK.
* .IP "USB_FLAG_SHORT_FAIL"
* Treats receive (IN) data underrun as error.
* .IP "USB_FLAG_ISO_ASAP"
* Start an isochronous transfer immediately.
*
* When the USB is transferring data from a device to the host the data may 
* "underrun".  That is, the device may transmit less data than anticipated by 
* the host.  This may or may not indicate an error condition depending on the 
* design of the device.  For many devices, the underrun is completely normal 
* and indicates the end of data stream from the device.  For other devices, 
* the underrun indicates a transfer failure.  By default, the USBD and 
* underlying USB HCD (Host Controller Driver) treat underrun as the end-of-data 
* indicator and do not declare an error.  If the USB_FLAG_SHORT_FAIL flag is 
* set, then the USBD/HCD will instead treat underrun as an error condition.
*
* For isochronous transfers the USB_FLAG_ISO_ASAP specifies that the 
* isochronous transfer should begin as soon as possible.  If USB_FLAG_ISO_ASAP
* is not specified, then <startFrame> must specify the starting frame number 
* for the transfer.  The usbdCurrentFrameGet() function allows a client to
* retrieve the current frame number and a value called the frame scheduling 
* window for the underlying USB host controller.  The frame window specifies 
* the maximum number of frames into the future (relative to the current frame 
* number) which may be specified by <startFrame>.  <startFrame> should be 
* specified only for isochronous transfers.
*
* <dataBlockSize> may also be specified for isochronous transfers.  If non-0,
* the <dataBlockSize> defines the granularity of isochronous data being sent.
* When the underlying Host Controller Driver (HCD) breaks up the transfer into
* individual frames, it will ensure that the amount of data transferred in 
* each frame is a multiple of this value.  
*
* <timeout> specifies the IRP timeout in milliseconds.	If the caller passes
* a value of zero, then the USBD sets a default timeout of USB_TIMEOUT_DEFAULT.
* If no timeout is desired, then <timeout> should be set to USB_TIMEOUT_NONE.
* Timeouts apply only to control and bulk transfers.  Isochronous and
* interrupt transfers do not time out.
*
* <bfrList> is an array of buffer descriptors which describe data buffers to 
* be associated with this IRP.	If more than the one <bfrList> element is 
* required then the caller must allocate the IRP by calculating the size as 
*
* .CS
* irpLen = sizeof (USB_IRP) + (sizeof (USB_BFR_DESCR) * (bfrCount - 1))
* .CE
*
* <transferLen> must be the total length of data to be transferred.  In other 
* words, transferLen is the sum of all <bfrLen> entries in the <bfrList>.
*
* <pid> specifies the packet type to use for the indicated buffer and is
* specified as USB_PID_xxxx.
*
* The IRP <userCallback> routine must point to a client-supplied IRP_CALLBACK
* routine.  The usbdTransfer() function returns as soon as the IRP has been
* successfully enqueued.  If there is a failure in delivering the IRP to the
* HCD, then usbdTransfer() returns an error.  The actual result of the IRP
* should be checked after the <userCallback> routine has been invoked.
*
* RETURNS: OK, or ERROR if unable to submit IRP for transfer.
*/

STATUS usbdTransfer
    (
    USBD_CLIENT_HANDLE clientHandle,	/* Client handle */
    USBD_PIPE_HANDLE pipeHandle,	/* Pipe handle */
    pUSB_IRP pIrp			/* ptr to I/O request packet */
    )

    {
    URB_TRANSFER urb;


    /* Initalize URB */

    urbInit (&urb.header, clientHandle, USBD_FNC_TRANSFER, NULL, NULL,
	sizeof (urb));

    urb.pipeHandle = pipeHandle;
    urb.pIrp = pIrp;


    /* Execute URB */

    return urbExecBlock (&urb.header);
    }


/***************************************************************************
*
* usbdTransferAbort - Aborts a transfer
*
* This function aborts an IRP which was previously submitted through
* a call to usbdTransfer().  
*
* RETURNS: OK, or ERROR if unable to abort transfer.
*/

STATUS usbdTransferAbort
    (
    USBD_CLIENT_HANDLE clientHandle,	/* Client handle */
    USBD_PIPE_HANDLE pipeHandle,	    /* Pipe handle */
    pUSB_IRP pIrp			/* ptr to I/O to abort */
    )

    {
    URB_TRANSFER urb;


    /* Initalize URB */

    urbInit (&urb.header, clientHandle, USBD_FNC_TRANSFER_ABORT, NULL, NULL,
	sizeof (urb));

    urb.pipeHandle = pipeHandle;
    urb.pIrp = pIrp;


    /* Execute URB */

    return urbExecBlock (&urb.header);
    }


/***************************************************************************
*
* usbdSynchFrameGet - Returns a device's isochronous synch. frame
*
* It is sometimes necessary for clients to re-synchronize with devices when 
* the two are exchanging data isochronously.  This function allows a client
* to query a reference frame number maintained by the device.  Please refer 
* to the USB Specification for more detail.
*
* <nodeId> specifies the node to query and <endpoint> specifies the endpoint 
* on that device.  Upon return the device’s frame number for the specified 
* endpoint is returned in <pFrameNo>.
*
* RETURNS: OK, or ERROR if unable to retrieve synch. frame.
*/

STATUS usbdSynchFrameGet
    (
    USBD_CLIENT_HANDLE clientHandle,	/* Client Handle */
    USBD_NODE_ID nodeId,		/* Node Id of device/hub */
    UINT16 endpoint,			/* Endpoint to be queried */
    pUINT16 pFrameNo			/* Frame number returned by device */
    )

    {
    URB_SYNCH_FRAME_GET urb;
    STATUS s;


    /* Initalize URB */

    urbInit (&urb.header, clientHandle, USBD_FNC_SYNCH_FRAME_GET, NULL, NULL,
	sizeof (urb));

    urb.nodeId = nodeId;
    urb.endpoint = endpoint;


    /* Execute URB */

    s = urbExecBlock (&urb.header);


    /* Return result */

    if (pFrameNo != NULL)
	*pFrameNo = urb.frameNo;

    return s;
    }


/***************************************************************************
*
* usbdCurrentFrameGet - Returns the current frame number for a USB
*
* It is sometimes necessary for clients to retrieve the current USB frame 
* number for a specified host controller.  This function allows a client to 
* retrieve the current USB frame number for the host controller to which
* <nodeId> is connected.  Upon return, the current frame number is stored 
* in <pFrameNo>.
*
* If <pFrameWindow> is not NULL, the USBD will also return the maximum frame 
* scheduling window for the indicated USB host controller.  The frame 
* scheduling window is essentially the number of unique frame numbers 
* tracked by the USB host controller.  Most USB host controllers maintain an 
* internal frame count which is a 10- or 11-bit number, allowing them to 
* track typically 1024 or 2048 unique frames.  When starting an isochronous 
* transfer, a client may wish to specify that the transfer will begin in a 
* specific USB frame.  For the given USB host controller, the starting frame 
* number can be no more than <frameWindow> frames from the current <frameNo>.
*
* Note: The USBD is capable of simultaneously managing multiple USB host 
* controllers, each of which operates independently.  Therefore, it is 
* important that the client specify the correct <nodeId> when retrieving the 
* current frame number.  Typically, a client will be interested in the 
* current frame number for the host controller to which a specific device is 
* attached.
*
* RETURNS: OK, or ERROR if unable to retrieve current frame number.
*/

STATUS usbdCurrentFrameGet
    (
    USBD_CLIENT_HANDLE clientHandle,	/* Client handle */
    USBD_NODE_ID nodeId,		/* Node Id of node on desired USB */
    pUINT32 pFrameNo,			/* bfr to receive current frame no. */
    pUINT32 pFrameWindow		/* bfr to receive frame window */
    )

    {
    URB_CURRENT_FRAME_GET urb;
    STATUS s;


    /* Initalize URB */

    urbInit (&urb.header, clientHandle, USBD_FNC_CURRENT_FRAME_GET, NULL, NULL,
	sizeof (urb));

    urb.nodeId = nodeId;


    /* Execute URB */

    s = urbExecBlock (&urb.header);


    /* Return result */

    if (pFrameNo != NULL)
	*pFrameNo = urb.frameNo;

    if (pFrameWindow != NULL)
	*pFrameWindow = urb.frameWindow;

    return s;
    }


/***************************************************************************
*
* usbdSofMasterTake - Takes SOF master ownership
*
* A client which is performing isochronous transfers may need to adjust
* the USB frame time by a small amount in order to synchronize correctly
* with one or more devices.  In order to adjust the USB frame interval,
* a client must first attempt to become the "SOF Master" for bus to which
* a device is attached.
*
* <nodeId> specifies the id of a node on the bus for which the client
* wishes to become the SOF master.  Each USB managed by the USBD may have
* a different SOF master.
*
* RETURNS: OK, or ERROR if client cannot become SOF master
*/

STATUS usbdSofMasterTake
    (
    USBD_CLIENT_HANDLE clientHandle,	/* Client handle */
    USBD_NODE_ID nodeId 		/* Node Id of node on desired USB */
    )
    
    {
    URB_SOF_MASTER urb;

    /* Initalize URB */

    urbInit (&urb.header, clientHandle, USBD_FNC_SOF_MASTER_TAKE, NULL, NULL,
	sizeof (urb));

    urb.nodeId = nodeId;

    /* Execute URB */

    return urbExecBlock (&urb.header);
    }


/***************************************************************************
*
* usbdSofMasterRelease - Releases ownership of SOF master
*
* A client which has previously become the SOF master for a given USB
* may release SOF master status by calling this function.  <nodeId> should
* identify a node on the USB for which SOF master ownership should be 
* released.
*
* RETURNS: OK, or ERROR if SOF ownership cannot be released
*/

STATUS usbdSofMasterRelease
    (
    USBD_CLIENT_HANDLE clientHandle,	/* Client handle */
    USBD_NODE_ID nodeId 		/* Node Id of node on desired USB */
    )
    
    {
    URB_SOF_MASTER urb;

    /* Initalize URB */

    urbInit (&urb.header, clientHandle, USBD_FNC_SOF_MASTER_RELEASE, NULL, NULL,
	sizeof (urb));

    urb.nodeId = nodeId;

    /* Execute URB */

    return urbExecBlock (&urb.header);
    }


/***************************************************************************
*
* usbdSofIntervalGet - Retrieves current SOF interval for a bus
*
* This function retrieves the current SOF interval for the bus to which
* <nodeId> is connected.  The SOF interval returned in <pSofInterval> is
* expressed in high speed bit times in a frame, with 12,000 being the 
* typical default value.
*
* NOTE: A client does not need to be the current SOF master in order to
* retrieve the SOF interval for a bus.
*
* RETURNS: OK, or ERROR if SOF interval cannot be retrieved.
*/

STATUS usbdSofIntervalGet
    (
    USBD_CLIENT_HANDLE clientHandle,	/* Client handle */
    USBD_NODE_ID nodeId,		/* Node Id of node on desired USB */
    pUINT16 pSofInterval		/* bfr to receive SOF interval */
    )

    {
    URB_SOF_INTERVAL_GET_SET urb;
    STATUS s;

    /* Initalize URB */

    urbInit (&urb.header, clientHandle, USBD_FNC_SOF_INTERVAL_GET, NULL, 
	NULL, sizeof (urb));

    urb.nodeId = nodeId;

    /* Execute URB */

    s = urbExecBlock (&urb.header);

    /* return results */

    if (pSofInterval != NULL)
	*pSofInterval = urb.sofInterval;

    return s;
    }


/***************************************************************************
*
* usbdSofIntervalSet - Sets current SOF interval for a bus
*
* This function sets the SOF interval for the bus to which <nodeId> is
* connected to the number of bit times specified in <sofInterval>.  In
* order to call this function, a client must first become the "SOF master"
* for the indicated bus by calling usbdSofMasterTake().
*
* RETURNS: OK, or ERROR if cannot set SOF interval.
*/

STATUS usbdSofIntervalSet
    (
    USBD_CLIENT_HANDLE clientHandle,	/* Client handle */
    USBD_NODE_ID nodeId,		/* Node Id of node on desired USB */
    UINT16 sofInterval			/* new SOF interval */
    )

    {
    URB_SOF_INTERVAL_GET_SET urb;

    /* Initalize URB */

    urbInit (&urb.header, clientHandle, USBD_FNC_SOF_INTERVAL_SET, NULL, NULL, 
	sizeof (urb));

    urb.nodeId = nodeId;
    urb.sofInterval = sofInterval;

    /* Execute URB */

    return urbExecBlock (&urb.header);
    }


/***************************************************************************
*
* usbdVersionGet - Returns USBD version information
*
* This function returns the USBD version.  If <pVersion> is not NULL, the 
* USBD returns its version in BCD in <pVersion>.  For example, version 
* "1.02" would be coded as 01h in the high byte and 02h in the low byte.
*
* If <pMfg> is not NULL it must point to a buffer of at least USBD_NAME_LEN 
* bytes in length in which the USBD will store the NULL terminated name of
* the USBD manufacturer (e.g., "Wind River Systems" + \0).
*
* RETURNS: OK, or ERROR if unable to retrieve USBD version info.
*/

STATUS usbdVersionGet
    (
    pUINT16 pVersion,			/* UINT16 bfr to receive version */
    pCHAR pMfg				/* bfr to receive USBD mfg string */
    )

    {
    URB_VERSION_GET urb;
    STATUS s;


    /* Initalize URB */

    urbInit (&urb.header, NULL, USBD_FNC_VERSION_GET, NULL, NULL,
	sizeof (urb));


    /* Execute URB */

    s = urbExecBlock (&urb.header);


    /* Return result */

    if (pVersion != NULL)
	*pVersion = urb.version;

    if (pMfg != NULL)
	strncpy (pMfg, (char *)urb.mfg, USBD_NAME_LEN + 1);

    return s;
    }


/***************************************************************************
*
* usbdHcdAttach - Attaches an HCD to the USBD
*
* A special client or system initialization code must call the
* usbdHcdAttach() function to register at least one HCD (Host Controller 
* Driver) with the USBD.  Most systems will call this function at system 
* initialization in order to register one or more HCDs.  However, the USBD 
* is capable of supporting the attachment and removal of host controllers 
* and their associated HCDs at runtime.  Systems which implement hot-swap 
* capabilities may find this of use.
* 
* This is one of the few USBD functions which does not require a caller 
* first to register using the usbdClientRegister() function, so there is 
* no need to pass a USBD_CLIENT_HANDLE to this function as there is with 
* most other USBD functions.
*
* The <hcdExecFunc> passed by the caller must point to an HCD’s primary
* entry point as defined below:
*
* .CS
* typedef UINT16 (*HCD_EXEC_FUNC) (pHRB_HEADER pHrb);
* .CE
*
* The USBD will invoke the <hcdExecFunc> with an "attach" request which
* includes the <param> passed to the usbdHcdAttach() function.	The <param> 
* is HCD-specific and is not defined in this specification.  
*
* If the HCD attach is successful, the USBD will then proceed to 
* initialize the USB(s) associated with the HCD by calling its
* HCD_EXEC_FUNC repeatedly.  If the initialization is completely 
* successful, the USBD returns a non-NULL attach token in <pAttachToken>.  
* If the caller needs to request the USBD to detach this HCD in the 
* future, it must pass the attach token value to the usbdHcdDetach() 
* function.  If usbdHcdAttach() fails to initialize the HCD successfully, 
* it returns an error and a NULL value in <pAttachToken>.  The caller 
* should not call usbdHcdDetach() for an HCD which failed to initialize 
* successfully.
*
* A complete description of the <param>, the HCD_EXEC_FUNC, and the 
* USBD/HCD initialization sequence is beyond the scope of this document.  
* For a complete description of the HCD, please refer to the .I "Wind 
* River USB Host Controller Hardware Abstraction Interface Specification."
*
* Note: A system which implements multiple USB host controllers which are 
* compatible with a single HCD may select one of two approaches to 
* initialization.  In the first approach, the usbdHcdAttach() function 
* may be called a single time with the <hcdExecFunc> for the HCD and the 
* HCD may reveal multiple USB host controllers to the USBD.  In the second 
* approach, the usbdHcdAttach() function may be called multiple times, 
* once for each USB host controller, and the HCD may reveal only a single 
* USB host controller to the USBD each time.  Each approach has advantages 
* and the approach selected is up to the system vendor responsible for 
* implementing the HCD.
*
* RETURNS: OK, or ERROR if unable to attach HCD.
*/

STATUS usbdHcdAttach
    (
    HCD_EXEC_FUNC hcdExecFunc,		/* Ptr to HCD’s primary entry point */
    pVOID param,			/* HCD-specific parameter */
    pGENERIC_HANDLE pAttachToken	/* Token to identify HCD in future */
    )

    {
    URB_HCD_ATTACH urb;
    STATUS s;


    /* Initalize URB */

    urbInit (&urb.header, NULL, USBD_FNC_HCD_ATTACH, NULL, NULL,
	sizeof (urb));

    urb.hcdExecFunc = hcdExecFunc;
    urb.param = param;


    /* Execute URB */

    s = urbExecBlock (&urb.header);


    /* Return result */

    if (pAttachToken != NULL)
	*pAttachToken = urb.attachToken;

    return s;
    }


/***************************************************************************
*
* usbdHcdDetach - Detaches an HCD from the USBD
*
* This function is typically called during system shutdown to instruct the 
* USBD to detach from an HCD to which it had previously attached in response 
* to a call to usbdHcdAttach().  Certain systems may also call this function 
* at runtime in order to detach a USB host controller and its associated HCD
* in order to implement hot-swapping.
*
* The <attachToken> must be the attach token originally returned by 
* usbdHcdAttach() when it first attached to the HCD. 
*
* RETURNS: OK, or ERROR if unable to detach HCD.
*/

STATUS usbdHcdDetach
    (
    GENERIC_HANDLE attachToken		/* AttachToken returned */
    )

    {
    URB_HCD_DETACH urb;


    /* Initalize URB */

    urbInit (&urb.header, NULL, USBD_FNC_HCD_DETACH, NULL, NULL,
	sizeof (urb));

    urb.attachToken = attachToken;


    /* Execute URB */

    return urbExecBlock (&urb.header);
    }


/***************************************************************************
*
* usbdStatisticsGet - Retrieves USBD operating statistics
*
* This function returns operating statistics for the USB to which the
* specified <nodeId> is connected.    
*
* The USBD copies the current operating statistics into the <pStatistics>
* structure provided by the caller.  This structure is defined as:
*
* .CS
* typedef struct usbd_stats
*     {
*     UINT16 totalTransfersIn;
*     UINT16 totalTransfersOut;
*     UINT16 totalReceiveErrors;
*     UINT16 totalTransmitErrors;
*     } USBD_STATS, *pUSBD_STATS;
* .CE
*
* It is anticipated that this structure may grow over time.  To provide 
* backwards compatibility, the client must pass the total size of the 
* USBD_STATS structure it has allocated in <statLen>.  The USBD will copy 
* fields into this structure only up to the statLen indicated by the caller.
*
* RETURNS: OK, or ERROR if unable to retrieve operating statistics.
*/

STATUS usbdStatisticsGet
    (
    USBD_CLIENT_HANDLE clientHandle,	/* Client handle */
    USBD_NODE_ID nodeId,		/* Node Id of node on desired USB */
    pUSBD_STATS pStatistics,		/* Ptr to structure to receive stats */
    UINT16 statLen			/* Len of bfr provided by caller */
    )

    {
    URB_STATISTICS_GET urb;


    /* Initalize URB */

    urbInit (&urb.header, clientHandle, USBD_FNC_STATISTICS_GET, NULL, NULL,
	sizeof (urb));

    urb.nodeId = nodeId;
    urb.pStatistics = pStatistics;
    urb.statLen = statLen;


    /* Execute URB */

    return urbExecBlock (&urb.header);
    }



/* End of file. */

