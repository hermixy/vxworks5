/* usbdCoreLib.c - USBD core logic */

/* Copyright 2000-2002 Wind River Systems, Inc. */

/*
Modification history
--------------------
01l,08nov01,wef  USBD_NODE struture element changed from hubStatus to 
		 pHubStatus, reflect this change
01k,25oct01,wef  repalce automatic buffer creations with calls to OSS_MALLOC
		 in places where the buffer is passed to the hardware (related 
		 to SPR 70492).
01j,18sep01,wef  merge from wrs.tor2_0.usb1_1-f for veloce
01i,08aug01,dat  Removing warnings
01h,12aug00,bri  Modify interrogateDeviceClass() to 
		 1. Allow multifunction devices that belong to same USB class
		    but have interfaces belonging to different subclasses.
		 2. Store the configuration Value as mentioned in the 
		    configuration descriptor in the nodeClass.
		 3. Ensure nodeclasses are created for all devices.
01g,20mar00,rcb  Add USB_DEVICE_DESCR field to USBD_NODE.
01f,17jan00,rcb  Modify interrogateDeviceClass() to use usbConfigDescrGet()...
		 ensures that all descriptors associated with a configuration
		 descriptor will be read, even if the total length is very long.
01e,20dec99,rcb  Fix benign bug in fncConfigSet() regarding pRoot.
		 Fix benign bug in HCD detach logic which detached the HCD
		 before deleting all pipes.
01d,23nov99,rcb  Replace HCD bandwidth alloc/release with pipe create/destroy
		 in order to generalize approach for OHCI HCD.
01c,24sep99,rcb  Change packet count calculation in transferIrpCallback()
		 to handle case where a single 0-length packet is transferred.
01b,07sep99,rcb  Add support for management callbacks and set-bus-state API.
01a,08jun99,rcb  First.
*/

/*
DESCRIPTION

This module contains the primary USBD entry point, usbdUrbExec() and the
individual USBD function execution routines.  usbdUrbExec() is responsible for 
interpreting each URB's function code and then fanning-out to the individual 
function processor.  

IMPLEMENTATION NOTES

When the USBD is first initialized it creates an internal "client" which is 
used by the USBD when performing control pipe transfers to each hub/device.  This
"client" remains active until the USBD is shut down.

For each client registered with the USBD, including the internal client, the USBD
allocates an individual task, and that task inherits the priority of the client
task which invokes the usbdClientRegister() function.  This task is normally 
inactive, and only wakes up when a client callback is to be executed.  Therefore, 
each client's callbacks are invoked on a task which is unique to that client.
Other USBD tasks (see below) are therefore shielded from the behavoir of an
individual client's callback routine.

For each USB which is attached to the USBD through the usbdHcdAttach() function,
the USBD also creates a unique "bus monitor" task.  This task is responsible
for configuring and monitoring each hub on a given USB.  Whenever a hub or device
is attached/removed from the USB, the bus monitor thread is responsible for
updating internal data structures, configuring/re-configuring hubs, and for
triggering "dynamic attach/removal" callbacks - which themselves are performed by
each individual client's callback task.

All USBD internal data structures, e.g., client lists, HCD lists and releated
node structures, are protected by a single mutex, structMutex.	All threads which
rely on the stability of internal data structures or which can modify internal
data structures take this mutex.  usbdCoreEntry() is the single entry point
responsible for taking this mutex when clients invoke the USBD.  Each "bus
monitor" thread also takes this mutex each time it may update bus structures.  
IRP callback, however, do not take the mutex.  While certain IRP "userPtr" fields
may point to other USBD structures, the organization of the code guarantees that
IRPs will be completed or canceled prior to dismantling the USBD structures with
which they are associated.
*/


/* defines */

#define USBD_VERSION	0x0001			    /* USBD version in BCD */
#define USBD_MFG	"Wind River Systems, Inc."  /* USBD Mfg */

#define INTERNAL_CLIENT_NAME	"usbd"


/* includes */

#include "usb/usbPlatform.h"

#include "string.h"

#include "usb/ossLib.h"
#include "usb/usbQueueLib.h"
#include "usb/usbLib.h" 	/* USB utility functions */
#include "usb/usbdCoreLib.h"	/* our API */

#include "drv/usb/usbHcd.h"	/* HCD interface */
#include "usb/usbHcdLib.h"	/* HCD function library */

#include "usb/usbdStructures.h" /* usbdCoreLib data structures */


/* defines */

#define PENDING 		1

#define CALLBACK_Q_DEPTH	128	/* Needs to be fairly deep to handle */
					/* a potentially large number of */
					/* notification callbacks */

#define BUS_Q_DEPTH		32	/* Outstanding bus service requests */


/* clientThread request codes...stored in msg field of OSS_MESSAGE. */

#define CALLBACK_FNC_TERMINATE	    0	/* Terminate the callback thread */
#define CALLBACK_FNC_IRP_COMPLETE   1	/* issue an IRP completion callback */
#define CALLBACK_FNC_NOTIFY_ATTACH  2	/* issue an attach callback */
#define CALLBACK_FNC_MNGMT_EVENT    3	/* management event callback */


#define CALLBACK_TIMEOUT	5000	/* Wait 5 seconds for callback to */
					/* exit in response to terminate fnc */

/* busThread request codes...stored in msg field of OSS_MESSAGE. */

#define BUS_FNC_TERMINATE	0	/* Terminate the bus monitor thread */
#define BUS_FNC_UPDATE_HUB	1	/* update a hub */


#define BUS_TIMEOUT		5000	/* Wait 5 seconds for bus monitor */
					/* thread to terminate */


/* general timeout */

#define IO_TIMEOUT		5000	/* 5 seconds */
#define EXCLUSION_TIMEOUT	10000	/* 10 seconds */


/* HUB_STATUS_LEN() returns length of status structure for indicated hub node */

#define HUB_STATUS_LEN(pNode)	\
    min ((pNode->numPorts + 1 + 7) / 8, (int) MAX_HUB_STATUS_LEN)


/* Constants used by resetDataToggle(). */

#define ANY_CONFIGURATION	0xffff
#define ANY_INTERFACE		0xffff
#define ANY_ENDPOINT		0xffff


/* typedefs */

/* NOTIFICATION
 * 
 * Created by notifyIfMatch() and consumed by client's callback thread.
 */

typedef struct notification
    {
    USBD_ATTACH_CALLBACK callback;	/* client's callback routine */
    USBD_NODE_ID nodeId;		/* node being attached/removed */
    UINT16 attachCode;			/* attach code */
    UINT16 configuration;		/* config matching notify request */
    UINT16 interface;			/* interface matching notify request */
    UINT16 deviceClass; 		/* device/interface class */
    UINT16 deviceSubClass;		/* device/interface sub class */
    UINT16 deviceProtocol;		/* device/interface protcol */
    } NOTIFICATION, *pNOTIFICATION;


/* locals */

LOCAL int initCount = 0;

LOCAL MUTEX_HANDLE structMutex = NULL;	/* guards USBD structures */

LOCAL LIST_HEAD hcdList = {0};		/* List of attached HCDs */
LOCAL LIST_HEAD clientList = {0};	/* list of registered clients */

LOCAL USBD_CLIENT_HANDLE internalClient = NULL; /* internal client */


/* forward declarations */

LOCAL BOOL initHubIrp
    (
    pUSBD_NODE pNode
    );

LOCAL pUSBD_NODE createNode 
    (
    pUSBD_BUS pBus,		    /* node's parent bus */
    USBD_NODE_ID rootId,	    /* root id */
    USBD_NODE_ID parentHubId,	    /* parent hub id */
    UINT16 parentHubPort,	    /* parent hub port no */
    UINT16 nodeSpeed,		    /* node speed */
    UINT16 topologyDepth	    /* this node's depth in topology */
    );


/* functions */

/***************************************************************************
*
* validateClient - Determines if a client handle is valid
*
* RETURNS: TRUE if client valid, else FALSE
*/

LOCAL BOOL validateClient
    (
    USBD_CLIENT_HANDLE clientHandle,	/* client to be validated */
    pUSBD_CLIENT *ppClient		/* ptr to USBD_CLIENT if valid */
    )

    {
    if (usbHandleValidate (clientHandle, USBD_CLIENT_SIG, (pVOID *) ppClient) 
	!= OK)
	return FALSE;

    return TRUE;
    }


/***************************************************************************
*
* validateNode - Determines if a node handle is valid
*
* RETURNS: TRUE if node valid, else FALSE
*/

LOCAL BOOL validateNode
    (
    USBD_NODE_ID nodeId,	    /* Node to be validated */
    pUSBD_NODE *ppNode		    /* ptr to USBD_NODE if valid */
    )

    {
    if (usbHandleValidate (nodeId, USBD_NODE_SIG, (pVOID *) ppNode) != OK ||
	(*ppNode)->nodeDeletePending)
	return FALSE;

    return TRUE;
    }


/***************************************************************************
*
* validatePipe - Determines if a pipe is valid
*
* RETURNS: TRUE if pipe valid, else FALSE
*/

LOCAL BOOL validatePipe
    (
    USBD_PIPE_HANDLE pipeHandle,    /* pipe to be validated */
    pUSBD_PIPE *ppPipe		    /* ptr to USBD_PIPE if valid */
    )

    {
    if (usbHandleValidate (pipeHandle, USBD_PIPE_SIG, (pVOID *) ppPipe) != OK ||
	(*ppPipe)->pipeDeletePending || (*ppPipe)->pNode->nodeDeletePending)
	return FALSE;

    return TRUE;
    }


/***************************************************************************
*
* validateUrb - Performs validation checks on URB
*
* Checks the URB length set in the URB_HEADER against the <expectedLen>
* passed by the caller.  If <pClient> is not NULL, also attempts to
* validate the USBD_CLIENT_HANDLE.  If successful, stores the client
* handle in <pClient>
*
* RETURNS: S_usbdLib_xxxx
*/

LOCAL int validateUrb
    (
    pVOID pUrb,
    UINT16 expectedLen,
    pUSBD_CLIENT *ppClient
    )

    {
    pURB_HEADER pHeader = (pURB_HEADER) pUrb;

    if (initCount == 0)
	return S_usbdLib_NOT_INITIALIZED;

    if (pHeader->urbLength != expectedLen)
	return S_usbdLib_BAD_PARAM;

    if (ppClient != NULL)
	{
	if (!validateClient (pHeader->handle, ppClient))
	    {
	    return S_usbdLib_BAD_HANDLE;
	    }
	}


    return OK;
    }


/***************************************************************************
*
* setUrbResult - Sets URB_HEADER.status and .result fields
*
* Based on the <result> parameter passed by the caller, set the 
* status and result fields in the URB_HEADER of <pUrb>. 
*
* RETURNS: Value of <result> parameter
*/

LOCAL int setUrbResult
    (
    pURB_HEADER pUrb,		/* Completed URB */
    int result			/* S_usbdLib_xxxx code */
    )

    {
    if (result != PENDING)
	{
	pUrb->result = result;

	if (pUrb->callback != NULL)
	    (*pUrb->callback) (pUrb);
	}

    return result;
    }


/***************************************************************************
*
* doShutdown - Shut down USBD core lib
*
* RETURNS: N/A
*/

LOCAL VOID doShutdown (void)
    {
    /* unregister any clients which may still be registered */

    pUSBD_CLIENT pClient;
    pUSBD_HCD pHcd;

    while ((pClient = usbListFirst (&clientList)) != NULL)
	usbdClientUnregister (pClient->handle);

    /* detach any HCDs which may still be attached */

    while ((pHcd = usbListFirst (&hcdList)) != NULL)
	usbdHcdDetach (pHcd->attachToken);

    /* release the structure guard mutex */

    if (structMutex != NULL)
	{
	OSS_MUTEX_DESTROY (structMutex);
	structMutex = NULL;
	}

    /* Shut down libraries */

    usbHandleShutdown ();
    ossShutdown ();
    }


/***************************************************************************
*
* fncInitialize - Initialize USBD
*
* RETURNS: S_usbdLib_xxxx
*/

LOCAL int fncInitialize
    (
    pURB_HEADER pUrb
    )

    {
    int s = OK;

    if (initCount == 0)
	{

	/* Initialize the osServices library */

	if (ossInitialize () != OK)
	    return S_usbdLib_GENERAL_FAULT;

	if (usbHandleInitialize (0 /* use default */) != OK)
	    {
	    ossShutdown ();
	    return S_usbdLib_GENERAL_FAULT;
	    }
    
	++initCount;

	/* Initialize the lists of HCDs and clients */

	if (OSS_MUTEX_CREATE (&structMutex) != OK)
	    s = S_usbdLib_OUT_OF_RESOURCES;

	memset (&hcdList, 0, sizeof (hcdList));
	memset (&clientList, 0, sizeof (clientList));
	}

    if (s == OK)
	{
	/* Register an internal client for hub I/O, etc. */

	internalClient = NULL;

	if (usbdClientRegister (INTERNAL_CLIENT_NAME, &internalClient) != OK)
	    s = S_usbdLib_GENERAL_FAULT;
	}

    if (s != OK)
	{
	doShutdown ();
	initCount = 0;
	}

    return s;
    }


/***************************************************************************
*
* fncShutdown - Shuts down USBD
*
* RETURNS: S_usbdLib_xxxx
*/

LOCAL int fncShutdown
    (
    pURB_HEADER pUrb
    )

    {
    int s;

    /* validate URB */

    if ((s = validateUrb (pUrb, sizeof (*pUrb), NULL)) != OK)
	return s;

    if (initCount == 1)
	doShutdown ();

    --initCount;

    return s;
    }


/***************************************************************************
*
* clientThread - client callback thread
*
* A separate clientThread() thread is spawned for each registered
* client.  This thread is responsible for invoking client callback
* routines.  Using a separate callback thread for each client helps to
* ensure that one client's behavior (such as blocking) won't affect the
* throughput of other client requests.
*
* By convention, the <param> is a pointer to the USBD_CLIENT structure
* for the associated client.  This thread waits on the callback queue
* in the client structure.  At the time this thread is first created,
* the USBD_CLIENT structure is only guaranteed to have an initialized
* queue...other fields may not yet be initialized.
*
* NOTE: This thread does not need to take the structMutex.  The mainline
* code ensures that the clientThread() for a given USBD_CLIENT is
* terminated prior to dismantling the USBD_CLIENT structure.
*
* RETURNS: N/A
*/

LOCAL VOID clientThread
    (
    pVOID param 		    /* thread parameter */
    )

    {
    pUSBD_CLIENT pClient = (pUSBD_CLIENT) param;
    USB_MESSAGE msg;
    pUSB_IRP pIrp;
    pNOTIFICATION pNotification;
    
    /* Execute messages from the callbackQueue until a CLIENT_FNC_TERMINATE
    message is received. */

    do
	{
	if (usbQueueGet (pClient->callbackQueue, &msg, OSS_BLOCK) != OK)
	    break;

	switch (msg.msg)
	    {
	    case CALLBACK_FNC_IRP_COMPLETE:

		/* invoke a client's IRP callback routine.  The msg.lParam
		 * is a pointer to the completed USB_IRP.
		 */

		pIrp = (pUSB_IRP) msg.lParam;

		if (pIrp->userCallback != NULL)
		    (*pIrp->userCallback) (pIrp);

		break;


	    case CALLBACK_FNC_NOTIFY_ATTACH:

		/* invoke a client's dynamic attach routine.  The msg.lParam
		 * is a pointer to a NOTIFICATION structure.  
		 *
		 * NOTE: We dispose of the NOTIFICATION request after
		 * consuming it.
		 */

		pNotification = (pNOTIFICATION) msg.lParam;

		(*pNotification->callback) (pNotification->nodeId, 
		    pNotification->attachCode, pNotification->configuration, 
		    pNotification->interface, pNotification->deviceClass, 
		    pNotification->deviceSubClass, 
		    pNotification->deviceProtocol);

		OSS_FREE (pNotification);

		break;

    
	    case CALLBACK_FNC_MNGMT_EVENT:

		/* invoke a client's managment callback routine.  The 
		 * msg.wParam is the management code and the msg.lParam is
		 * the USBD_NODE_ID of the root for the affected bus.
		 */

		if (pClient->mngmtCallback != NULL)
		    (*pClient->mngmtCallback) (pClient->mngmtCallbackParam,
			(USBD_NODE_ID) msg.lParam, msg.wParam);

		break;
	    }
	}
    while (msg.msg != CALLBACK_FNC_TERMINATE);


    /* Mark the callback routine as having terminated. */

    OSS_SEM_GIVE (pClient->callbackExit);
    }


/***************************************************************************
*
* doTransferAbort - Cancel an outstanding IRP
*
* Directs the HCD to cancel the IRP and waits for the IRP callback to
* be invoked signalling that the IRP has been unlinked successfully.
*
* RETURNS: S_usbdLib_xxxx
*/

LOCAL int doTransferAbort
    (
    pUSBD_PIPE pPipe,		    /* Pipe owning transfer */
    pUSB_IRP pIrp		    /* IRP to be cancelled */
    )

    {
    /* The callback which indicates that an IRP has been deleted is
     * asynchronous.  However, when deleting an IRP (such as when
     * destroying a pipe) we generally need to know when the IRP
     * callback has actually been invoked - and hence the IRP unlinked
     * from the list of outstanding IRPs on the pipe.
     */

    pPipe->irpBeingDeleted = pIrp;
    pPipe->irpDeleted = FALSE;

    /* Instruct the HCD to cancel the IRP */

    if (usbHcdIrpCancel (&pPipe->pNode->pBus->pHcd->nexus, pIrp) != OK)
	return S_usbdLib_CANNOT_CANCEL;

    /* Wait for the IRP callback to be invoked. */

    while (!pPipe->irpDeleted)
	OSS_THREAD_SLEEP (1);

    return OK;
    }


/***************************************************************************
*
* destroyNotify - de-allocates a USBD_NOTIFY_REQ
*
* RETURNS: N/A
*/

LOCAL VOID destroyNotify
    (
    pUSBD_NOTIFY_REQ pNotify
    )

    {
    usbListUnlink (&pNotify->reqLink);
    OSS_FREE (pNotify);
    }


/***************************************************************************
*
* destroyPipe - de-allocates a USBD_PIPE and its resources
*
* RETURNS: N/A
*/

LOCAL VOID destroyPipe
    (
    pUSBD_PIPE pPipe
    )

    {
    pUSB_IRP pIrp;
    pUSBD_BUS pBus;


    if (pPipe != NULL)
	{
	pPipe->pipeDeletePending = TRUE;


	/* Cancel all IRPs outstanding on this pipe.
	 *
	 * NOTE: Since the IRP completion callbacks are on a different thread,
	 * we need to wait until all callbacks have been completed.  This
	 * functionality is built into doTransferAbort().
	 */

	while ((pIrp = usbListFirst (&pPipe->irps)) != NULL)
	    doTransferAbort (pPipe, pIrp);


	/* Release bandwidth and notify HCD that the pipe is going away. */

	pBus = pPipe->pNode->pBus;
	pBus->nanoseconds -= pPipe->nanoseconds;

	usbHcdPipeDestroy (&pBus->pHcd->nexus, pPipe->hcdHandle);


	/* Unlink pipe from owning client's list of pipes */

	if (pPipe->pClient != NULL)
	    usbListUnlink (&pPipe->clientPipeLink);


	/* Unlink pipe from owning node's list of pipes */

	if (pPipe->pNode != NULL)
	    usbListUnlink (&pPipe->nodePipeLink);


	/* Release pipe handle */

	if (pPipe->handle != NULL)
	    usbHandleDestroy (pPipe->handle);


	OSS_FREE (pPipe);
	}
    }


/***************************************************************************
*
* destroyClient - tears down a USBD_CLIENT structure
*
* RETURNS: N/A
*/

LOCAL VOID destroyClient
    (
    pUSBD_CLIENT pClient
    )

    {
    pUSBD_NOTIFY_REQ pNotify;
    pUSBD_PIPE pPipe;
    pUSBD_HCD pHcd;
    UINT16 busNo;
    
    /* unlink client from list of clients.
     *
     * NOTE: usbListUnlink is smart and only unlinks the structure if it is
     * actually linked.
     */

    usbListUnlink (&pClient->clientLink);


    /* destroy all notification requests outstanding for the client */

    while ((pNotify = usbListFirst (&pClient->notifyReqs)) != NULL)
	destroyNotify (pNotify);


    /* destroy all pipes owned by this client */

    while ((pPipe = usbListFirst (&pClient->pipes)) != NULL)
	destroyPipe (pPipe);


    /* If this client is the current SOF master for any USBs, then release
     * the SOF master status.
     */

    pHcd = usbListFirst (&hcdList);

    while (pHcd != NULL)
	{
	for (busNo = 0; busNo < pHcd->busCount; busNo++)
	    if (pHcd->pBuses [busNo].pSofMasterClient == pClient)
		pHcd->pBuses [busNo].pSofMasterClient = NULL;

	pHcd = usbListNext (&pHcd->hcdLink);
	}


    /* Note: callbackQueue is always created after callbackExit and
     * before callbackThread
     */

    if (pClient->callbackThread != NULL)
	{
	/* Terminate the client callback thread */

	usbQueuePut (pClient->callbackQueue, CALLBACK_FNC_TERMINATE,
	    0, 0, CALLBACK_TIMEOUT);
	OSS_SEM_TAKE (pClient->callbackExit, CALLBACK_TIMEOUT);
	OSS_THREAD_DESTROY (pClient->callbackThread);
	}

    if (pClient->callbackQueue != NULL)
	usbQueueDestroy (pClient->callbackQueue);

    if (pClient->callbackExit != NULL)
	OSS_SEM_DESTROY (pClient->callbackExit);

    if (pClient->handle != NULL)
	usbHandleDestroy (pClient->handle);

    OSS_FREE (pClient);
    }


/***************************************************************************
*
* fncClientReg - Register a new USBD client
*
* RETURNS: S_usbdLib_xxxx
*/

LOCAL int fncClientReg
    (
    pURB_CLIENT_REG pUrb
    )

    {
    pUSBD_CLIENT pClient;
    int s;

    /* validate URB */

    if ((s = validateUrb (pUrb, sizeof (*pUrb), NULL)) != OK)
	return s;


    /* Create structures/resources/etc., required by new client */

    if ((pClient = OSS_CALLOC (sizeof (*pClient))) == NULL)
	return S_usbdLib_OUT_OF_MEMORY;

    memcpy (pClient->clientName, pUrb->clientName, USBD_NAME_LEN);


    if (usbHandleCreate (USBD_CLIENT_SIG, pClient, &pClient->handle) != OK ||
	OSS_SEM_CREATE (1, 0, &pClient->callbackExit) != OK ||
	usbQueueCreate (CALLBACK_Q_DEPTH, &pClient->callbackQueue) != OK ||
	OSS_THREAD_CREATE (clientThread, pClient, OSS_PRIORITY_INHERIT, 
	    "tUsbdClnt", &pClient->callbackThread) != OK)

	    s = S_usbdLib_OUT_OF_RESOURCES;
    else
	{

	/* The client was initialized successfully. Add it to the list */

	usbListLink (&clientList, pClient, &pClient->clientLink, LINK_TAIL);

	/* return the client's USBD_CLIENT_HANDLE */

	pUrb->header.handle = pClient->handle;
	}
	

    if (s != OK)
	{
	destroyClient (pClient);
	}

    return s;
    }


/***************************************************************************
*
* fncClientUnreg - Unregister a USBD client
*
* RETURNS: S_usbdLib_xxxx
*/

LOCAL int fncClientUnreg
    (
    pURB_CLIENT_UNREG pUrb
    )

    {
    pUSBD_CLIENT pClient;
    int s;

    /* validate Urb */

    if ((s = validateUrb (pUrb, sizeof (*pUrb), &pClient)) != OK)
	return s;

    /* destroy client */

    destroyClient (pClient);

    return s;
    }


/***************************************************************************
*
* fncMngmtCallbackSet - sets management callback for a client
*
* RETURNS: S_usbdLib_xxxx
*/

LOCAL int fncMngmtCallbackSet
    (
    pURB_MNGMT_CALLBACK_SET pUrb
    )

    {
    pUSBD_CLIENT pClient;
    int s;

    /* validate URB */

    if ((s = validateUrb (pUrb, sizeof (*pUrb), &pClient)) != OK)
	return s;

    /* Set the management callback */

    pClient->mngmtCallback = pUrb->mngmtCallback;
    pClient->mngmtCallbackParam = pUrb->mngmtCallbackParam;

    return s;
    }


/***************************************************************************
*
* fncVersionGet - Return USBD version
*
* RETURNS: S_usbdLib_xxxx
*/

LOCAL int fncVersionGet
    (
    pURB_VERSION_GET pUrb
    )

    {
    int s;

    /* validate urb */

    if ((s = validateUrb (pUrb, sizeof (*pUrb), NULL)) != OK)
	return s;


    /* return version information */

    pUrb->version = USBD_VERSION;
    strncpy ((char *)pUrb->mfg, USBD_MFG, USBD_NAME_LEN);

    return s;
    }


/***************************************************************************
*
* releaseAddress - release a USB device address
*
* RETURNS: N/A
*/

LOCAL VOID releaseAddress
    (
    pUSBD_NODE pNode
    )

    {
    pUSBD_BUS pBus = pNode->pBus;
    pBus->adrsVec [pNode->busAddress / 8] &= ~(0x1 << (pNode->busAddress % 8));
    }


/***************************************************************************
*
* assignAddress - assigns a unique USB address to a node
*
* RETURNS: TRUE if successful, else FALSE
*/

LOCAL BOOL assignAddress
    (
    pUSBD_NODE pNode
    )

    {
    pUSBD_BUS pBus = pNode->pBus;
    UINT16 i;

    /* Find an available address */

    for (i = 1; i < USB_MAX_DEVICES; i++)
	{
	if ((pBus->adrsVec [i / 8] & (0x1 << (i % 8))) == 0)
	    {
	    /* i is the value of an unused address.  Set the device adrs. */

	    if (usbdAddressSet (internalClient, pNode->nodeHandle, i) == OK)
		{
		pNode->busAddress = i;
		pBus->adrsVec [i / 8] |= 0x1 << (i % 8);
		return TRUE;
		}
	    else
		{
		return FALSE;
		}
	    }
	}

    return FALSE;
    }


/***************************************************************************
*
* notifyIfMatch - Invoke attach callback if appropriate.
*
* Compares the device class/subclass/protocol for <pClassType> and <pNotify>.
* If the two match, then invokes the callback in <pNotify> with an attach
* code of <attachCode>.
*
* RETURNS: N/A
*/

LOCAL VOID notifyIfMatch
    (
    pUSBD_NODE pNode,
    pUSBD_NODE_CLASS pClassType,
    pUSBD_CLIENT pClient,
    pUSBD_NOTIFY_REQ pNotify,
    UINT16 attachCode
    )

    {
    pNOTIFICATION pNotification;


    /* Do the pClassType and pNotify structures contain matching class
     * descriptions?
     */

    if (pNotify->deviceClass == USBD_NOTIFY_ALL ||
	pClassType->deviceClass == pNotify->deviceClass)
	{
	if (pNotify->deviceSubClass == USBD_NOTIFY_ALL ||
	    pClassType->deviceSubClass == pNotify->deviceSubClass)
	    {
	    if (pNotify->deviceProtocol == USBD_NOTIFY_ALL ||
		pClassType->deviceProtocol == pNotify->deviceProtocol)
		{
		/* We have a match.  Schedule the client attach callback. 
		 *
		 * NOTE: The pNotification structure is created here and
		 * released when consumed in the client callback thread.
		 *
		 * NOTE: In a very large USB topology (at least several
		 * dozen nodes) there is a chance that we could overrun
		 * a client's callback queue depending on the type of 
		 * notification requests the client has made.  If this happens,
		 * the following call to usbQueuePut() may block.  If *that*
		 * happens, there is a chance of a deadlock if the client's
		 * callback code - invoked from the clientThread() reenters
		 * the USBD.  A simple solution, if that situation is observed,
		 * is to increase the depth of the callback queue.
		 */

		if ((pNotification = OSS_CALLOC (sizeof (*pNotification))) 
		    != NULL)
		    {

		    pNotification->callback = pNotify->callback;
		    pNotification->nodeId = pNode->nodeHandle;
		    pNotification->attachCode = attachCode;
		    pNotification->configuration = pClassType->configuration;
		    pNotification->interface = pClassType->interface;
		    pNotification->deviceClass = pClassType->deviceClass;
		    pNotification->deviceSubClass = pClassType->deviceSubClass;
		    pNotification->deviceProtocol = pClassType->deviceProtocol;

		    usbQueuePut (pClient->callbackQueue, 
			CALLBACK_FNC_NOTIFY_ATTACH, 0, (UINT32) pNotification, 
			OSS_BLOCK);
		    }
		}
	    }
	}
    }


/***************************************************************************
*
* notifyClients - notify clients of a dynamic attach/removal
*
* Scans the clientList looking for clients who have requested dynamic
* attach/removal notification for a class matching <pNode>.  If any
* are found, their dynamic attach callback routines will be invoked
* with an attach code of <attachCode>.
*
* RETURNS: N/A
*/

LOCAL VOID notifyClients
    (
    pUSBD_NODE pNode,
    UINT16 attachCode
    )

    {
    pUSBD_CLIENT pClient;
    pUSBD_NOTIFY_REQ pNotify;
    pUSBD_NODE_CLASS pClassType;


    /* Scan for clients which have requested dynamic attach notification. */

    pClient = usbListFirst (&clientList);

    while (pClient != NULL)
	{
	/* Walk through the list of notification requests for this client */

	pNotify = usbListFirst (&pClient->notifyReqs);

	while (pNotify != NULL)
	    {
	    /* Walk through the class types for this node, looking for one
	     * which matches the current client notification request.
	     */

	    pClassType = usbListFirst (&pNode->classTypes);

	    while (pClassType != NULL)
		{
		notifyIfMatch (pNode, pClassType, pClient, pNotify, attachCode);
		pClassType = usbListNext (&pClassType->classLink);
		}

	    pNotify = usbListNext (&pNotify->reqLink);
	    }

	pClient = usbListNext (&pClient->clientLink);
	}
    }


/***************************************************************************
*
* createNodeClass - Creates a USBD_NODE_CLASS structure
*
* Creates a USBD_NODE_CLASS structure, initializes it with the passed
* parameters, and attaches it to the <pNode>.
*
* RETURNS: TRUE if successful, else FALSE if out of memory.
*/

LOCAL BOOL createNodeClass
    (
    pUSBD_NODE pNode,
    UINT16 configuration,
    UINT16 interface,
    UINT16 deviceClass,
    UINT16 deviceSubClass,
    UINT16 deviceProtocol
    )

    {
    pUSBD_NODE_CLASS pClassType;

    if ((pClassType = OSS_CALLOC (sizeof (*pClassType))) == NULL)
	return FALSE;

    pClassType->configuration = configuration;
    pClassType->interface = interface;
    pClassType->deviceClass = deviceClass;
    pClassType->deviceSubClass = deviceSubClass;
    pClassType->deviceProtocol = deviceProtocol;

    usbListLink (&pNode->classTypes, pClassType, &pClassType->classLink,
	LINK_TAIL);

    return TRUE;
    }


/***************************************************************************
*
* interrogateDeviceClass - Retrieve's device class information
*
* Interrogate the device class and construct one or more USBD_NODE_CLASS 
* structures and attach them to <pNode>.  <deviceClass>, <deviceSubClass>,
* and <deviceProtocol> are the device-level fields for this device.  If
* <deviceClass> is 0, then this routine will automatically interrogate
* the interface-level class information. If the information is provided
* at the device level then the interfaces are not probed. 
*
* RETURNS: TRUE if successful, else FALSE if error.
*/

LOCAL BOOL interrogateDeviceClass
    (
    pUSBD_NODE pNode,
    UINT16 deviceClass,
    UINT16 deviceSubClass,
    UINT16 deviceProtocol
    )

    {
    pUSB_CONFIG_DESCR pCfgDescr;
    pUSB_INTERFACE_DESCR pIfDescr;
    pUINT8 pCfgBfr;
    pUINT8 pRemBfr;
    UINT16 cfgLen;
    UINT16 remLen;
    UINT16 numConfig;
    UINT8 configIndex;
    BOOL retcode = TRUE;
    UINT8 configValue = 1;
    

    /* If class information has been specified at the device level, then
     * record it and return.
     */

    if ((deviceClass != 0) && (deviceSubClass != 0) && (deviceProtocol != 0))
	return createNodeClass (pNode, 0, 0, deviceClass, deviceSubClass,
	    deviceProtocol); 


    /* Read the device descriptor to determine the number of configurations. */

    if (usbConfigCountGet (internalClient, pNode->nodeHandle, &numConfig) != OK)
	return FALSE;

  
    /* Read and process each configuration. */

    for (configIndex = 0; configIndex < numConfig && retcode; configIndex++)
	{
	/* Read the next configuration descriptor. */
	
	if (usbConfigDescrGet (internalClient, pNode->nodeHandle, configIndex,
	    &cfgLen, &pCfgBfr) != OK)
	    {
	    retcode = FALSE;
	    }
	else
	    {
	    if ((pCfgDescr = usbDescrParse (pCfgBfr, cfgLen, 
		USB_DESCR_CONFIGURATION)) != NULL)
		{
		/* Scan each interface descriptor. 
		 *
		 * NOTE: If we can't find an interface descriptor, we just keep
		 * going. 
		 */

		pRemBfr = pCfgBfr;
		remLen = cfgLen;
		configValue = pCfgDescr->configurationValue;

		while ((pIfDescr = usbDescrParseSkip (&pRemBfr, &remLen, 
		    USB_DESCR_INTERFACE)) != NULL && retcode)
		    {
		    if (!createNodeClass (pNode, configValue,
			pIfDescr->interfaceNumber, pIfDescr->interfaceClass, 
			pIfDescr->interfaceSubClass, pIfDescr->interfaceProtocol))
			{
			retcode = FALSE;
			}
		    }
		}

	    /* Release bfr allocated by usbConfigDescrGet(). */
    
	    OSS_FREE (pCfgBfr);
	    }
	}


    return retcode;
    }


/***************************************************************************
*
* destroyNode - deletes node structure
*
* RETURNS: N/A
*/

LOCAL VOID destroyNode
    (
    pUSBD_NODE pNode
    )

    {
    pUSBD_NODE_CLASS pClassType;
    pUSBD_PIPE pPipe;


    /* Mark node as "going down" */

    pNode->nodeDeletePending = TRUE;


    /* Cancel any other pipes associated with this node...This has the
     * effect of cancelling any IRPs outstanding on this node. */

    while ((pPipe = usbListFirst (&pNode->pipes)) != NULL)
	destroyPipe (pPipe);


    /* Notify an interested clients that this node is going away. */

    notifyClients (pNode, USBD_DYNA_REMOVE);


    /* Delete any class type structures associated with this node. */

    while ((pClassType = usbListFirst (&pNode->classTypes)) != NULL)
	{
	usbListUnlink (&pClassType->classLink);
	OSS_FREE (pClassType);
	}


    /* release bus address associated with node */

    releaseAddress (pNode);


    /* Release node resources/memory. */

    if (pNode->controlSem != NULL)
	OSS_SEM_DESTROY (pNode->controlSem);

    if (pNode->nodeHandle != NULL)
	usbHandleDestroy (pNode->nodeHandle);

    OSS_FREE (pNode->pHubStatus);

    OSS_FREE (pNode);
    }


/***************************************************************************
*
* destroyAllNodes - destroys all nodes in a tree of nodes
*
* Recursively destroys all nodes from <pNode> and down.  If <pNode> is
* NULL, returns immediately.
*
* RETURNS: N/A
*/

LOCAL VOID destroyAllNodes
    (
    pUSBD_NODE pNode
    )

    {
    int i;

    if (pNode != NULL)
	{
	if (pNode->nodeInfo.nodeType == USB_NODETYPE_HUB &&
	    pNode->pPorts != NULL)
	    {
	    /* destroy all children of this node */

	    for (i = 0; i < pNode->numPorts; i++)
		destroyAllNodes (pNode->pPorts [i].pNode);
	    }

	destroyNode (pNode);
	}
    }


/***************************************************************************
*
* updateHubPort - Determine if a device has been attached/removed
*
* Note: The <port> parameter to this function is 0-based (0 = first port,
* 1 = second port, etc.)  However, on the USB interface itself, the index
* passed in a Setup packet to identify a port is 1-based.
*
* RETURNS: N/A
*/

LOCAL VOID updateHubPort
    (
    pUSBD_NODE pNode,
    UINT16 port
    )

    {
    USB_HUB_STATUS * pStatus;
    UINT16 actLen;
    UINT16 nodeSpeed;

    pStatus = OSS_MALLOC (sizeof (USB_HUB_STATUS));

    /* retrieve status for the indicated hub port. */

    if (usbdStatusGet (internalClient, 
		       pNode->nodeHandle, 
		       USB_RT_CLASS | USB_RT_OTHER, 
		       port + 1, 
		       sizeof (USB_HUB_STATUS), 
		       (UINT8 *) pStatus, 
		       &actLen) != OK ||
	actLen < sizeof (USB_HUB_STATUS))
	{
	OSS_FREE (pStatus);
	return;
	}

    /* correct the status byte order */

    pStatus->status = FROM_LITTLEW (pStatus->status);
    pStatus->change = FROM_LITTLEW (pStatus->change);


    /* Process the change indicated by status.change */

    /* NOTE: To see if a port's connection status has changed we also see
     * whether our internal understanding of the port's connection status
     * matches the hubs's current connection status.  If the two do not
     * agree, then we also assume a connection change, whether the hub is
     * indicating one or not. This covers the case at power on where the
     * hub does not report a connection change (i.e., the device is already
     * plugged in), but we need to initialize the port for the first time.
     */

    if ((pStatus->change & USB_HUB_C_PORT_CONNECTION) != 0 ||
	((pNode->pPorts [port].pNode == NULL) != 
	    ((pStatus->status & USB_HUB_STS_PORT_CONNECTION) == 0)))
	{
	/* Given that there has been a change on this port, it stands to
	 * reason that all nodes connected to this port must be invalidated -
	 * even if a node currently appears to be connected to the port.
	 */

	destroyAllNodes (pNode->pPorts [port].pNode);
	pNode->pPorts [port].pNode = NULL;


	/* If a node appears to be connected to the port, reset the port. */

	if ((pStatus->status & USB_HUB_STS_PORT_CONNECTION) != 0)
	    {

	    /* Wait 100 msec after detecting a connection to allow power to
	     * stabilize.  Then enable the port and issue a reset.
	     */

	    OSS_THREAD_SLEEP (USB_TIME_POWER_ON);

	    usbdFeatureSet (internalClient, pNode->nodeHandle,
		USB_RT_CLASS | USB_RT_OTHER, USB_HUB_FSEL_PORT_RESET, port + 1);

	    /* 
	     * Acoording to the USB spec (refer to section 7.1.7.3) we should
 	     * wait 50 msec for reset signaling + an additional 10 msec reset 
	     * recovery time.  We double this time for safety's sake since 
	     * we have seen a few devices that do not respond to the suggested 
	     * time of 60 msec.
	     */ 

	    OSS_THREAD_SLEEP (2*USB_TIME_RESET + USB_TIME_RESET_RECOVERY);
	    }


	/* Clear the port connection change indicator - whether or not it
	 * appears that a device is currently connected. */

	usbdFeatureClear (internalClient, pNode->nodeHandle, 
	    USB_RT_CLASS | USB_RT_OTHER, USB_HUB_FSEL_C_PORT_CONNECTION, port + 1);


	/* If a node appears to be connected, create a new node structure for 
	 * it and attach it to this hub. */

	if ((pStatus->status & USB_HUB_STS_PORT_CONNECTION) != 0)
	    {
	    /* If the following call to createNode() fails, it returns NULL.
	     * That's fine, as the corresponding USBD_PORT structure will be
	     * initialized correctly either way.
	     */

	    nodeSpeed = ((pStatus->status & USB_HUB_STS_PORT_LOW_SPEED) != 0) ?
		USB_SPEED_LOW : USB_SPEED_FULL;

	    if ((pNode->pPorts [port].pNode = createNode (pNode->pBus,
		pNode->pBus->pRoot->nodeHandle, pNode->nodeHandle, port,
		nodeSpeed, pNode->topologyDepth + 1)) == NULL)
		{
		/* Attempt to initialize the port failed.  Disable the port */

		usbdFeatureClear (internalClient, pNode->nodeHandle,
		    USB_RT_CLASS | USB_RT_OTHER, USB_HUB_FSEL_PORT_ENABLE, 
			port + 1);
		}
	    }
	}

    if ((pStatus->change & USB_HUB_C_PORT_ENABLE) != 0)
	{
	/* Clear the port enable change indicator */

	usbdFeatureClear (internalClient, pNode->nodeHandle, 
	    USB_RT_CLASS | USB_RT_OTHER, USB_HUB_FSEL_C_PORT_ENABLE, port + 1);
	}

    if ((pStatus->change & USB_HUB_C_PORT_SUSPEND) != 0)
	{
	/* Clear the suspend change indicator */

	usbdFeatureClear (internalClient, pNode->nodeHandle, 
	    USB_RT_CLASS | USB_RT_OTHER, USB_HUB_FSEL_C_PORT_SUSPEND, port + 1);
	}

    if ((pStatus->change & USB_HUB_C_PORT_OVER_CURRENT) != 0)
	{
	/* Clear the over current change indicator */

	usbdFeatureClear (internalClient, pNode->nodeHandle,
	    USB_RT_CLASS | USB_RT_OTHER, USB_HUB_FSEL_C_PORT_OVER_CURRENT, 
	    port + 1);
	}

    if ((pStatus->change & USB_HUB_C_PORT_RESET) != 0)
	{
	/* Clear the reset change indicator */

	usbdFeatureClear (internalClient, pNode->nodeHandle, 
	    USB_RT_CLASS | USB_RT_OTHER, USB_HUB_FSEL_C_PORT_RESET, port + 1);
	}
    }


/***************************************************************************
*
* hubIrpCallback - called when hub status IRP completes
*
* By convention, the USB_IRP.userPtr field is a pointer to the USBD_NODE
* for this transfer.
*
* RETURNS: N/A
*/

LOCAL VOID hubIrpCallback
    (
    pVOID p			/* completed IRP */
    )

    {
    pUSB_IRP pIrp = (pUSB_IRP) p;
    pUSBD_NODE pNode = (pUSBD_NODE) pIrp->userPtr;


    /* If the IRP was canceled, it means that the hub node is being torn
     * down, so don't queue a request for the bus thread.
     */

    if (pIrp->result != S_usbHcdLib_IRP_CANCELED)
	{
	/* Let the bus thread update the port and re-queue the hub IRP. */

	usbQueuePut (pNode->pBus->busQueue, BUS_FNC_UPDATE_HUB,
	    0, (UINT32) pNode->nodeHandle, OSS_BLOCK);
	}
    }


/***************************************************************************
*
* initHubIrp - initialize IRP used to listen for hub status
*
* RETURNS: TRUE if successful, else FALSE
*/

LOCAL BOOL initHubIrp
    (
    pUSBD_NODE pNode
    )

    {
    pUSB_IRP pIrp = &pNode->hubIrp;

    
    /* initialize IRP */

    memset (pIrp, 0, sizeof (*pIrp));

    pIrp->userPtr = pNode;
    pIrp->irpLen = sizeof (USB_IRP);
    pIrp->userCallback = hubIrpCallback;
    pIrp->flags = USB_FLAG_SHORT_OK;
    pIrp->timeout = USB_TIMEOUT_NONE;

    pIrp->transferLen = HUB_STATUS_LEN (pNode);

    pIrp->bfrCount = 1;
    pIrp->bfrList [0].pid = USB_PID_IN;
    pIrp->bfrList [0].pBfr = (pUINT8) pNode->pHubStatus;
    pIrp->bfrList [0].bfrLen = pIrp->transferLen;

    memset (pNode->pHubStatus, 0, MAX_HUB_STATUS_LEN);


    /* submit the IRP. */

    if (usbdTransfer (internalClient, pNode->hubStatusPipe, pIrp) != OK)
	return FALSE;

    return TRUE;
    }


/***************************************************************************
*
* initHubNode - initialize a hub
*
* RETURNS: TRUE if successful, else FALSE
*/

LOCAL BOOL initHubNode
    (
    pUSBD_NODE pNode
    )

    {
    pUSB_CONFIG_DESCR pCfgDescr;
    pUSB_INTERFACE_DESCR pIfDescr;
    pUSB_ENDPOINT_DESCR pEpDescr;
    pUSB_HUB_DESCR pHubDescrBuf;
    UINT8 * pConfDescrBfr;
    UINT16 actLen;
    UINT16 port;

    pConfDescrBfr = OSS_MALLOC (USB_MAX_DESCR_LEN);
    pHubDescrBuf = OSS_MALLOC (sizeof (USB_HUB_DESCR));


    /* Read the hub's descriptors. */

    if (usbdDescriptorGet (internalClient, 
			   pNode->nodeHandle, 
			   USB_RT_STANDARD | USB_RT_DEVICE, 
			   USB_DESCR_CONFIGURATION, 
			   0, 
			   0, 
			   USB_MAX_DESCR_LEN, 
			   pConfDescrBfr, 
			   &actLen) 
			 != OK)
	{
	OSS_FREE (pConfDescrBfr);
	OSS_FREE (pHubDescrBuf);
	return FALSE;
	}

    if ((pCfgDescr = usbDescrParse (pConfDescrBfr, 
				    actLen, 
				    USB_DESCR_CONFIGURATION)) 
				== NULL ||
	(pIfDescr = usbDescrParse (pConfDescrBfr, 
				   actLen, 
				   USB_DESCR_INTERFACE))
			  	== NULL ||
	(pEpDescr = usbDescrParse (pConfDescrBfr, 
				   actLen, 
				   USB_DESCR_ENDPOINT))
				== NULL)
	{
        OSS_FREE (pConfDescrBfr);
        OSS_FREE (pHubDescrBuf);
	return FALSE;
	}

    if (usbdDescriptorGet (internalClient, 
			   pNode->nodeHandle, 
			   USB_RT_CLASS | USB_RT_DEVICE, 
			   USB_DESCR_HUB, 
			   0, 
			   0, 
			   sizeof (USB_HUB_DESCR), 
			   (UINT8 *) pHubDescrBuf, 
			   &actLen) 
			!= OK ||
	actLen < USB_HUB_DESCR_LEN)
	{
        OSS_FREE (pConfDescrBfr);
        OSS_FREE (pHubDescrBuf);
	return FALSE;
	}


    /* We now have all important descriptors for this hub.  Pick out
     * important information and configure hub.
     */

    /* Record hub power capability */

    pNode->pwrGoodDelay = pHubDescrBuf->pwrOn2PwrGood * 2; /* 2 msec per unit */
    pNode->hubContrCurrent = pHubDescrBuf->hubContrCurrent;

    if ((pCfgDescr->attributes & USB_ATTR_SELF_POWERED) != 0)
	{
	pNode->selfPowered = TRUE;
	pNode->maxPowerPerPort = USB_POWER_SELF_POWERED;
	}
    else
	{
	pNode->selfPowered = FALSE;
	pNode->maxPowerPerPort = USB_POWER_BUS_POWERED;
	}

    pNode->numPorts = pHubDescrBuf->nbrPorts;


    /* If this hub is not already at the maximum USB topology depth, then
     * create a pipe to monitor it's status and enable each of its ports.
     */

    if (pNode->topologyDepth < USB_MAX_TOPOLOGY_DEPTH)
	{
	/* Set the hub's configuration. */

	if (usbdConfigurationSet (internalClient, 
				  pNode->nodeHandle, 
				  pCfgDescr->configurationValue, 
				  pNode->hubContrCurrent) 
			  != OK)
	    {
            OSS_FREE (pConfDescrBfr);
            OSS_FREE (pHubDescrBuf);
	    return FALSE;
	    }


	/* Create a pipe for interrupt transfers from this device. */

	if (usbdPipeCreate (internalClient, 
			    pNode->nodeHandle, 
			    pEpDescr->endpointAddress, 
			    pCfgDescr->configurationValue, 
			    0, 
			    USB_XFRTYPE_INTERRUPT, 
			    USB_DIR_IN, 
			    FROM_LITTLEW (pEpDescr->maxPacketSize), 
			    HUB_STATUS_LEN (pNode), 
			    pEpDescr->interval, 
			    &pNode->hubStatusPipe) 
			  != OK)
	    {
            OSS_FREE (pConfDescrBfr);
            OSS_FREE (pHubDescrBuf);
	    return FALSE;
	    }


	/* Allocate structures for downstream nodes. */

	if ((pNode->pPorts = OSS_CALLOC (pNode->numPorts * sizeof (USBD_PORT))) 
	    == NULL)
	    {
            OSS_FREE (pConfDescrBfr);
            OSS_FREE (pHubDescrBuf);
	    return FALSE;
	    }


	/* Initialize each hub port */

	for (port = 0; port < pNode->numPorts; port++)
	    {
	    /* Enable power to the port */

	    usbdFeatureSet (internalClient, pNode->nodeHandle, 
		USB_RT_CLASS | USB_RT_OTHER, USB_HUB_FSEL_PORT_POWER, port + 1);
	    }


	OSS_THREAD_SLEEP (pNode->pwrGoodDelay); /* let power stabilize */


	/* Initialize an IRP to listen for status changes on the hub */

   	if (!initHubIrp (pNode))
	    {
            OSS_FREE (pConfDescrBfr);
            OSS_FREE (pHubDescrBuf);
	    return FALSE;
	    }
	}
    
    OSS_FREE (pConfDescrBfr);
    OSS_FREE (pHubDescrBuf);
 
    return TRUE;
    }


/***************************************************************************
*
* createNode - Creates and initializes new USBD_NODE
*
* If the node is a hub, then automatically initializes hub.
*
* RETURNS: pointer to newly created USBD_NODE, or
*	   NULL if not successful
*/

LOCAL pUSBD_NODE createNode 
    (
    pUSBD_BUS pBus,		    /* node's parent bus */
    USBD_NODE_ID rootId,	    /* root id */
    USBD_NODE_ID parentHubId,	    /* parent hub id */
    UINT16 parentHubPort,	    /* parent hub port no */
    UINT16 nodeSpeed,		    /* node speed */
    UINT16 topologyDepth	    /* this node's depth in topology */
    )

    {
    pUSBD_NODE pNode;
    pUSBD_PIPE pPipe;
    UINT16 actLen;


    /* Allocate/initialize USBD_NODE */

    if ((pNode = OSS_CALLOC (sizeof (*pNode))) == NULL)
	return NULL;

    /* 
     * The hub status buffer gets touched by hardware.  Ensure that 
     * it does not span a cache line
     */

    if ((pNode->pHubStatus = OSS_CALLOC (MAX_HUB_STATUS_LEN)) == NULL)
	{
	OSS_FREE (pNode);
	return NULL;
	}

    if (usbHandleCreate (USBD_NODE_SIG, pNode, &pNode->nodeHandle) != OK ||
	OSS_SEM_CREATE (1, 1, &pNode->controlSem) != OK)
	{
	destroyNode (pNode);
	return NULL;
	}

    pNode->pBus = pBus;
    pNode->nodeInfo.nodeSpeed = nodeSpeed;
    pNode->nodeInfo.parentHubId = parentHubId;
    pNode->nodeInfo.parentHubPort = parentHubPort;
    pNode->nodeInfo.rootId = (rootId == 0) ? pNode->nodeHandle : rootId;
    pNode->topologyDepth = topologyDepth;


    /* Create a pipe for control transfers to this device. */

    if (usbdPipeCreate (internalClient, pNode->nodeHandle, USB_ENDPOINT_CONTROL,
	0, 0, USB_XFRTYPE_CONTROL, USB_DIR_INOUT, USB_MIN_CTRL_PACKET_SIZE, 0, 0, 
	&pNode->controlPipe) != OK)
	{
	destroyNode (pNode);
	return NULL;
	}


    /* Read the device descriptor to get the maximum payload size and to
     * determine if this is a hub or not.
     *
     * NOTE: We read only the first 8 bytes of the device descriptor (which
     * takes us through the maxPacketSize field) as suggested by the USB
     * spec.
     */

    if (usbdDescriptorGet (internalClient, pNode->nodeHandle,
	USB_RT_STANDARD | USB_RT_DEVICE, USB_DESCR_DEVICE, 0, 0,
	USB_MIN_CTRL_PACKET_SIZE, (pUINT8) &pNode->devDescr, &actLen) != OK ||
	actLen < USB_MIN_CTRL_PACKET_SIZE)
	{
	destroyNode (pNode);
	return NULL;
	}


    /* Now that we've read the device descriptor, we know the actual
     * packet size supported by the control pipe.  Update the pipe
     * accordingly.
     */

    validatePipe (pNode->controlPipe, &pPipe);
    pPipe->maxPacketSize = pNode->devDescr.maxPacketSize0;  /* field is byte wide */


    /* Set a unique address for this device. */

    if (!assignAddress (pNode))
	{
	destroyNode (pNode);
	return NULL;
	}


    /* Notify the HCD of the change in device address and max packet size */

    usbHcdPipeModify (&pBus->pHcd->nexus, pPipe->hcdHandle, 
	pNode->busAddress, pPipe->maxPacketSize);


    /* If this is a hub node, it requires additional initialization.  Otherwise,
     * we're done unless and until a client performs additional I/O to this
     * device.
     */

    if (pNode->devDescr.deviceClass == USB_CLASS_HUB)
	{
	pNode->nodeInfo.nodeType = USB_NODETYPE_HUB;
	if (!initHubNode (pNode))
	    {
	    destroyNode (pNode);
	    return NULL;
	    }
	}
    else
	{
	pNode->nodeInfo.nodeType = USB_NODETYPE_DEVICE;
	}


    /* Read the device class type(s) from the device and notify interested
     * clients of the device's insertion.
     */

    interrogateDeviceClass (pNode, pNode->devDescr.deviceClass,
	pNode->devDescr.deviceSubClass, pNode->devDescr.deviceProtocol);

    notifyClients (pNode, USBD_DYNA_ATTACH);

    return pNode;
    }


/***************************************************************************
*
* checkHubStatus - checks hub status and updates hub as necessary
*
* Note: We take a Node ID as our parameter (instead of, say, a pointer
* to a USBD_NODE structure) so that we can validate the node upon entry.
* There are cases where this routine may be called when the underlying
* node has already disappeared.
*
* RETURNS: N/A
*/

LOCAL VOID checkHubStatus
    (
    USBD_NODE_ID nodeId
    )

    {
    pUSBD_NODE pNode;
    pUSB_IRP pIrp;
    UINT16 port;
    UINT8 portMask;
    UINT8 statusIndex;


    /* Is the node still valid? */

    if (!validateNode (nodeId, &pNode))
	return;

    pIrp = &pNode->hubIrp;

    /* Determine what status is available.
     *
     * The hubStatus vector contains one bit for the hub itself and 
     * then one bit for each port.
     */

    if (pIrp->result == OK)
	{
	port = 0;
	statusIndex = 0;
	portMask = USB_HUB_ENDPOINT_STS_PORT0;

	while (port < pNode->numPorts && statusIndex < pIrp->bfrList [0].actLen)
	    {
	    if ((*(pNode->pHubStatus + statusIndex) & portMask) != 0)
		{
		/* This port appears to have status to report */

		updateHubPort (pNode, port);
		}

	    port++;
	    portMask <<= 1;

	    if (portMask == 0)
		{
		portMask = 0x01;    /* next byte starts with bit 0 */
		statusIndex++;
		}
	    }
	}


    /* resubmit the IRP */

    if (pIrp->result != S_usbHcdLib_IRP_CANCELED)
	initHubIrp (pNode);
    }


/***************************************************************************
*
* busThread - bus monitor thread
*
* A separate busThread() thread is spawned for each bus currently
* attached to the USBD.  This thread is responsible for monitoring and
* responding to bus events, like the attachment and removal of devices.
* Using a separate thread for each bus helps to ensure that one bus's 
* behavior won't affect the throughput of other buses.
*
* By convention, the <param> is a pointer to the USBD_BUS structure
* for the associated bus.  This thread waits on the bus queue
* in the bus structure.  At the time this thread is first created,
* the USBD_BUS structure is only guaranteed to have an initialized
* queue...other fields may not yet be initialized.
*
* RETURNS: N/A
*/

LOCAL VOID busThread
    (
    pVOID param 		    /* thread parameter */
    )

    {
    pUSBD_BUS pBus = (pUSBD_BUS) param;
    USB_MESSAGE msg;


    /* Execute messages from the busQueue until a BUS_FNC_TERMINATE
    message is received. */

    do
	{
	if (usbQueueGet (pBus->busQueue, &msg, OSS_BLOCK) != OK)
	    break;

	switch (msg.msg)
	    {
	    case BUS_FNC_UPDATE_HUB:

		OSS_MUTEX_TAKE (structMutex, OSS_BLOCK);
		checkHubStatus ((USBD_NODE_ID) msg.lParam);
		OSS_MUTEX_RELEASE (structMutex);

		break;
	    }
	}
    while (msg.msg != BUS_FNC_TERMINATE);


    /* Mark the callback routine as having terminated. */

    OSS_SEM_GIVE (pBus->busExit);
    }


/***************************************************************************
*
* hcdMngmtCallback - invoked when HCD detects managment event
*
* RETURNS: N/A
*/

LOCAL VOID hcdMngmtCallback
    (
    pVOID mngmtCallbackParam,	    /* caller-defined param */
    HCD_CLIENT_HANDLE handle,	    /* handle to host controller */
    UINT16 busNo,		    /* bus number */
    UINT16 mngmtCode		    /* management code */
    )

    {
    pUSBD_HCD pHcd = (pUSBD_HCD) mngmtCallbackParam;
    pUSBD_BUS pBus;
    pUSBD_CLIENT pClient;


    /* In an unusual case, this routine could be invoked by the HCD
     * before we have completely initialized our structures.  In that
     * case, just ignore the event.
     */

    if (pHcd->pBuses == NULL)
	return;

    pBus = &pHcd->pBuses [busNo];

    if (pBus->busThread == NULL || pBus->busQueue == NULL ||
	pBus->pRoot == NULL || pBus->pRoot->nodeHandle == NULL)
	return;


    /* Reflect the management event to interested clients */

    OSS_MUTEX_TAKE (structMutex, OSS_BLOCK);

    pClient = usbListFirst (&clientList);

    while (pClient != NULL)
	{
	if (pClient->mngmtCallback != NULL)
	    usbQueuePut (pClient->callbackQueue, CALLBACK_FNC_MNGMT_EVENT, 
		mngmtCode, (UINT32) pBus->pRoot->nodeHandle, OSS_BLOCK);

	pClient = usbListNext (&pClient->clientLink);
	}

    OSS_MUTEX_RELEASE (structMutex);
    }


/***************************************************************************
*
* initHcdBus - Initialize USBD_BUS element of USBD_HCD structure
*
* <pHcd> points to the USBD_HCD being initialized.  <busNo> is the index
* of the HCD bus to be initialized.
*
* RETURNS: S_usbdLib_xxxx
*/

LOCAL int initHcdBus
    (
    pUSBD_HCD pHcd,		    /* USBD_HCD being initialized */
    UINT16 busNo		    /* Bus number to initialize */
    )

    {
    pUSBD_BUS pBus = &pHcd->pBuses [busNo];
    int s = OK;

    /* Allocate resources for this bus. */

    pBus->pHcd = pHcd;
    pBus->busNo = busNo;

    if (OSS_SEM_CREATE (1, 0, &pBus->busExit) != OK ||
	usbQueueCreate (BUS_Q_DEPTH, &pBus->busQueue) != OK ||
	OSS_THREAD_CREATE (busThread, pBus, OSS_PRIORITY_HIGH, "tUsbdBus", 
	    &pBus->busThread) != OK ||
	(pBus->pRoot = createNode (pBus, 0, 0, 0, USB_SPEED_FULL, 0)) == NULL)
	{
	/* NOTE: If we fail here, destroyHcd() will clean up partially
	 * allocate structures/resources/etc.
	 */

	return S_usbdLib_OUT_OF_RESOURCES;
	}


    return s;
    }


/***************************************************************************
*
* destroyHcd - tears down a USBD_HCD structure
*
* Detaches the indicated HCD and tears down the HCD structures.
*
* RETURNS: N/A
*/

LOCAL VOID destroyHcd
    (
    pUSBD_HCD pHcd
    )

    {
    pUSBD_BUS pBus;
    UINT16 busNo;


    /* Unlink the HCD */

    usbListUnlink (&pHcd->hcdLink);


    /* Begin by de-allocating resources related to each bus */

    for (busNo = 0; pHcd->pBuses != NULL && busNo < pHcd->busCount; busNo++)
	{

	pBus = &pHcd->pBuses [busNo];

	/* Destroy all nodes associated with this bus */

	destroyAllNodes (pBus->pRoot);


	/* NOTE: The busQueue is always allocated before the busThread. */

	if (pBus->busThread != NULL)
	    {
	    /* Issue a terminate request to the bus thread */

	    usbQueuePut (pBus->busQueue, BUS_FNC_TERMINATE, 0, 0, BUS_TIMEOUT);
	    OSS_SEM_TAKE (pBus->busExit, BUS_TIMEOUT);
	    OSS_THREAD_DESTROY (pBus->busThread);
	    }

	if (pBus->busQueue != NULL)
	    usbQueueDestroy (pBus->busQueue);

	if (pBus->busExit != NULL)
	    OSS_SEM_DESTROY (pBus->busQueue);
	}



    /* Detach the HCD */

    if (pHcd->nexus.hcdExecFunc != NULL && pHcd->nexus.handle != NULL)
	usbHcdDetach (&pHcd->nexus);


    /* Release USBD_HCD resources */

    if (pHcd->attachToken != NULL)
	usbHandleDestroy (pHcd->attachToken);

    OSS_FREE (pHcd);
    }


/***************************************************************************
*
* fncHcdAttach - Attach an HCD to the USBD
*
* RETURNS: S_usbdLib_xxxx
*/

LOCAL int fncHcdAttach
    (
    pURB_HCD_ATTACH pUrb
    )

    {
    pUSBD_HCD pHcd = NULL;
    UINT16 busNo;
    int s;

    /* validate URB */

    if ((s = validateUrb (pUrb, sizeof (*pUrb), NULL)) != OK)
	return s;


    /* Allocate structure for this host controller */

    if ((pHcd = OSS_CALLOC (sizeof (*pHcd))) == NULL)
	return S_usbdLib_OUT_OF_MEMORY;


    /* Issue an attach request to the HCD.  If it succeeds, determine the
     * number of buses managed by the HCD.
     */

    if (usbHcdAttach (pUrb->hcdExecFunc, pUrb->param, hcdMngmtCallback,
	pHcd, &pHcd->nexus, &pHcd->busCount) != OK)
	{
	OSS_FREE (pHcd);
	return S_usbdLib_GENERAL_FAULT;
	}


    if ((pHcd->pBuses = OSS_CALLOC (sizeof (USBD_BUS) * pHcd->busCount)) == NULL)
	{
	destroyHcd (pHcd);
	return S_usbdLib_OUT_OF_MEMORY;
	}


    /* Initialize the USBD_HCD */

    if (usbHandleCreate (USBD_HCD_SIG, pHcd, &pHcd->attachToken) != OK)
	{
	s = S_usbdLib_OUT_OF_RESOURCES;
	}
    else
	{
	/* Fetch information about each bus from the HCD. */

	for (busNo = 0; busNo < pHcd->busCount; busNo++)
	    {
	    if ((s = initHcdBus (pHcd, busNo)) != OK)
		break;
	    }
	}


    /* If we succeeded in initializing each bus, then the USBD_HCD
     * is fully initialized...link it into the list of HCDs
     */

    if (s == OK)
	{

	usbListLink (&hcdList, pHcd, &pHcd->hcdLink, LINK_TAIL);

	/* Return attachToken to caller */

	pUrb->attachToken = pHcd->attachToken;
	}
    else
	{
	/* Failed to attach...release allocated structs, etc. */

	destroyHcd (pHcd);
	}

    return s;
    }


/***************************************************************************
*
* fncHcdDetach - Detach an HCD from the USBD
*
* RETURNS: S_usbdLib_xxxx
*/

LOCAL int fncHcdDetach
    (
    pURB_HCD_DETACH pUrb
    )

    {
    pUSBD_HCD pHcd;
    int s;

    /* Validate URB */
    
    if ((s = validateUrb (pUrb, sizeof (*pUrb), NULL)) != OK)
	return s;


    /* Validate attachToken */

    if (usbHandleValidate (pUrb->attachToken, USBD_HCD_SIG, (pVOID *) &pHcd) 
	!= OK)
	return S_usbdLib_BAD_HANDLE;

    
    /* Detach the HCD and release HCD structures, etc. */

    destroyHcd (pHcd);

    return s;
    }


/***************************************************************************
*
* fncStatisticsGet - Return USBD operating statistics
*
* RETURNS: S_usbdLib_xxxx
*/

LOCAL int fncStatisticsGet
    (
    pURB_STATISTICS_GET pUrb
    )

    {
    pUSBD_NODE pNode;

    /* Validate root */

    if (!validateNode (pUrb->nodeId, &pNode))
	return S_usbdLib_BAD_HANDLE;

    /* Validate other parameters */

    if (pUrb->pStatistics == NULL)
	return S_usbdLib_BAD_PARAM;

    /* Copy statistics to callers buffer */

    memcpy (pUrb->pStatistics, &pNode->pBus->stats,
	min (pUrb->statLen, sizeof (pNode->pBus->stats)));

    return OK;
    }


/***************************************************************************
*
* fncBusCountGet - Returns number of USB buses in system
*
* RETURNS: S_usbdLib_xxxx
*/

LOCAL int fncBusCountGet
    (
    pURB_BUS_COUNT_GET pUrb
    )

    {
    pUSBD_HCD pHcd;
    UINT16 busCount;

    busCount = 0;
    pHcd = usbListFirst (&hcdList);

    while (pHcd != NULL)
	{
	busCount += pHcd->busCount;
	pHcd = usbListNext (&pHcd->hcdLink);
	}

    pUrb->busCount = busCount;

    return OK;
    }


/***************************************************************************
*
* fncRootIdGet - Returns root node id for a USB
*
* RETURNS: S_usbdLib_xxxx
*/

LOCAL int fncRootIdGet
    (
    pURB_ROOT_ID_GET pUrb
    )

    {
    pUSBD_HCD pHcd;
    UINT16 i;
    int s = OK;

    /* Find the HCD/bus corresponding to the index pUrb->busCount */

    i = pUrb->busIndex;
    pHcd = usbListFirst (&hcdList);

    while (pHcd != NULL)
	{
	if (i < pHcd->busCount)
	    {
	    if (pHcd->pBuses [i].pRoot != NULL)
		pUrb->rootId = pHcd->pBuses [i].pRoot->nodeHandle;
	    else
		s = S_usbdLib_INTERNAL_FAULT;
	    break;
	    }
	else
	    {
	    i -= pHcd->busCount;
	    pHcd = usbListNext (&pHcd->hcdLink);
	    }
	}

    if (pHcd == NULL)
	s = S_usbdLib_BAD_PARAM;

    return s;
    }


/***************************************************************************
*
* fncHubPortCountGet - Returns number of hubs on a port
*
* RETURNS: S_usbdLib_xxxx
*/

LOCAL int fncHubPortCountGet
    (
    pURB_HUB_PORT_COUNT_GET pUrb
    )

    {
    pUSBD_NODE pNode;

    /* validate node handle */

    if (!validateNode (pUrb->hubId, &pNode))
	return S_usbdLib_BAD_HANDLE;

    if (pNode->nodeInfo.nodeType != USB_NODETYPE_HUB)
	return S_usbdLib_NOT_HUB;

    
    /* return number of ports on this hub */

    pUrb->portCount = pNode->numPorts;

    return OK;
    }


/***************************************************************************
*
* fncNodeIdGet - Returns id of node attached to a hub port
*
* RETURNS: S_usbdLib_xxxx
*/

LOCAL int fncNodeIdGet
    (
    pURB_NODE_ID_GET pUrb
    )

    {
    pUSBD_NODE pNode;

    /* validate node handle */

    if (!validateNode (pUrb->hubId, &pNode))
	return S_usbdLib_BAD_HANDLE;

    if (pNode->nodeInfo.nodeType != USB_NODETYPE_HUB)
	return S_usbdLib_NOT_HUB;


    /* If the port index is valid, return the node id, if any, attached
     * to this port.
     */

    if (pUrb->portIndex >= pNode->numPorts)
	return S_usbdLib_BAD_PARAM;

    pUrb->nodeType = USB_NODETYPE_NONE;

    if (pNode->pPorts != NULL &&
	pNode->pPorts [pUrb->portIndex].pNode != NULL) 
	{
	pUrb->nodeType = pNode->pPorts [pUrb->portIndex].pNode->nodeInfo.nodeType;
	pUrb->nodeId = pNode->pPorts [pUrb->portIndex].pNode->nodeHandle;
	}
	
    return OK;
    }


/***************************************************************************
*
* scanClassTypes - Scans nodes for matching class types
*
* Scan <pNode> and all child nodes to see if they have exposed one or more
* class types matching that described in <pNotify>.  If we find match(es),
* then invoke the corresponding notification callbacks.
*
* RETURNS: N/A
*/

LOCAL VOID scanClassTypes
    (
    pUSBD_NODE pNode,
    pUSBD_CLIENT pClient,
    pUSBD_NOTIFY_REQ pNotify
    )

    {
    pUSBD_NODE_CLASS pClassType;
    UINT16 portNo;


    if (pNode != NULL)
	{
	/* Scan all class types exposed for this node. */

	pClassType = usbListFirst (&pNode->classTypes);

	while (pClassType != NULL)
	    {
	    notifyIfMatch (pNode, pClassType, pClient, pNotify, USBD_DYNA_ATTACH);

	    pClassType = usbListNext (&pClassType->classLink);
	    }

    
	/* If this node is a hub, then recursively scan child nodes. */

	if (pNode->nodeInfo.nodeType == USB_NODETYPE_HUB &&
	    pNode->pPorts != NULL)
	    {
	    for (portNo = 0; portNo < pNode->numPorts; portNo++)
		scanClassTypes (pNode->pPorts [portNo].pNode, pClient, pNotify);
	    }
	}
    }


/***************************************************************************
*
* fncDynaAttachReg - Register a client for dynamic attach notification
*
* RETURNS: S_usbdLib_xxxx
*/

LOCAL int fncDynaAttachReg
    (
    pURB_DYNA_ATTACH_REG_UNREG pUrb
    )

    {
    pUSBD_CLIENT pClient;
    pUSBD_NOTIFY_REQ pNotify;
    pUSBD_HCD pHcd;
    UINT16 busNo;
    int s;


    /* Validate parameters. */

    if ((s = validateUrb (pUrb, sizeof (*pUrb), &pClient)) != OK)
	return s;

    if (pUrb->attachCallback == NULL)
	return S_usbdLib_BAD_PARAM;


    /* Create a new dynamic notification request structure */

    if ((pNotify = OSS_CALLOC (sizeof (*pNotify))) == NULL)
	return S_usbdLib_OUT_OF_MEMORY;

    pNotify->deviceClass = pUrb->deviceClass;
    pNotify->deviceSubClass = pUrb->deviceSubClass;
    pNotify->deviceProtocol = pUrb->deviceProtocol;
    pNotify->callback = pUrb->attachCallback;


    /* Link this to the list of notification requests for the client */

    usbListLink (&pClient->notifyReqs, pNotify, &pNotify->reqLink, LINK_TAIL);

    /* At the time of the initial notification registration (now), it is
     * necessary to scan the list of devices already attached to the USB(s)
     * in order to see if any match the request.  Devices attached/removed
     * later will be handled by the busThread.
     */

    pHcd = usbListFirst (&hcdList);

    while (pHcd != NULL)
	{
	for (busNo = 0; busNo < pHcd->busCount; busNo++)
	    scanClassTypes (pHcd->pBuses [busNo].pRoot, pClient, pNotify);

	pHcd = usbListNext (&pHcd->hcdLink);
	}


    return OK;
    }


/***************************************************************************
*
* fncDynaAttachUnreg - Unregister a client for dynamic attach notification
*
* RETURNS: S_usbdLib_xxxx
*/

LOCAL int fncDynaAttachUnreg
    (
    pURB_DYNA_ATTACH_REG_UNREG pUrb
    )

    {
    pUSBD_CLIENT pClient;
    pUSBD_NOTIFY_REQ pNotify;
    int s;


    /* Validate parameters. */

    if ((s = validateUrb (pUrb, sizeof (*pUrb), &pClient)) != OK)
	return s;


    /* Search the client's list of notification requests for one which
     * matches exactly the parameters in the URB.
     */

    pNotify = usbListFirst (&pClient->notifyReqs);

    while (pNotify != NULL)
	{
	if (pNotify->deviceClass == pUrb->deviceClass &&
	    pNotify->deviceSubClass == pUrb->deviceSubClass &&
	    pNotify->deviceProtocol == pUrb->deviceProtocol &&
	    pNotify->callback == pUrb->attachCallback)
	    {
	    /* We found a matching notification request.  Destroy it. */

	    destroyNotify (pNotify);
	    return OK;
	    }

	pNotify = usbListNext (&pNotify->reqLink);
	}


    /* If we get this far, no matching request was found.  Declare an error */

    return S_usbdLib_GENERAL_FAULT;
    }


/***************************************************************************
*
* controlIrpCallback - called when control pipe Irp completes
*
* By convention, the USB_IRP.userPtr field is a pointer to the USBD_NODE
* for this transfer.
*
* RETURNS: N/A
*/

LOCAL VOID controlIrpCallback
    (
    pVOID p			/* completed IRP */
    )

    {
    pUSB_IRP pIrp = (pUSB_IRP) p;
    pUSBD_NODE pNode = pIrp->userPtr;
    pURB_HEADER pUrb = pNode->pUrb;

    /* Store the actual length transferred */

    if (pNode->pActLen != NULL)
	*pNode->pActLen = pIrp->bfrList [1].actLen;


    /* Store the completion result in the URB */

    setUrbResult (pUrb, (pIrp->result == OK) ? OK : S_usbdLib_IO_FAULT);


    /* We're done with the control pipe structures for this node. */

    OSS_SEM_GIVE (pNode->controlSem);
    }


/***************************************************************************
*
* controlRequest - Formats and submits a control transfer request
*
* This is an internal utility function which formats a USB Setup packet
* and sends it to the control pipe of a node.
*
* RETURNS: S_usbdLib_xxxx
*/

LOCAL int controlRequest
    (
    pURB_HEADER pUrb,		    /* URB header */
    USBD_NODE_ID nodeId,	    /* node id */
    UINT8 requestType,		    /* bmRequestType */
    UINT8 request,		    /* bRequest */
    UINT16 value,		    /* wValue */
    UINT16 index,		    /* wIndex */
    UINT16 length,		    /* wLength */
    pVOID pBfr, 		    /* data */
    pUINT16 pActLen		    /* actual len for IN data */
    )

    {
    pUSBD_CLIENT pClient;
    pUSBD_NODE pNode;
    pUSB_SETUP pSetup;
    pUSB_IRP pIrp;


    /* validate the client handle for this request */

    if (!validateClient (pUrb->handle, &pClient))
	return S_usbdLib_BAD_HANDLE;


    /* validate the node to which this request is directed */

    if (!validateNode (nodeId, &pNode))
	return S_usbdLib_BAD_HANDLE;


    /* Only one control request can be outstanding at a time. */

    if (OSS_SEM_TAKE (pNode->controlSem, EXCLUSION_TIMEOUT) != OK)
	return S_usbdLib_TIMEOUT;


    /* we now own the control mutex for this node.  format and issue the
     * control request to this node.
     */

    pSetup = &pNode->setup;
    pIrp = &pNode->irp;

    
    /* format the setup packet */

    pSetup->requestType = requestType;
    pSetup->request = request;
    pSetup->value = TO_LITTLEW (value);
    pSetup->index = TO_LITTLEW (index);
    pSetup->length = TO_LITTLEW (length);


    /* format an IRP to execute this control transfer */

    memset (pIrp, 0, sizeof (USB_IRP) + sizeof (USB_BFR_LIST));

    pIrp->userPtr = pNode;
    pIrp->irpLen = sizeof (USB_IRP) + sizeof (USB_BFR_LIST);
    pIrp->userCallback = controlIrpCallback;
    pIrp->flags = USB_FLAG_SHORT_OK;
    pIrp->timeout = USB_TIMEOUT_DEFAULT;

    pIrp->transferLen = sizeof (USB_SETUP) + length;

    /* format bfrList [] entry for Setup packet */

    pIrp->bfrCount = 0;

    pIrp->bfrList [pIrp->bfrCount].pid = USB_PID_SETUP;
    pIrp->bfrList [pIrp->bfrCount].pBfr = (pUINT8) pSetup;
    pIrp->bfrList [pIrp->bfrCount].bfrLen = sizeof (USB_SETUP);
    pIrp->bfrCount++;	

    /* format bfrList [] entry for data stage, if any. */

    if (length > 0)
	{
	pIrp->bfrList [pIrp->bfrCount].pid = 
	    ((requestType & USB_RT_DEV_TO_HOST) != 0) ? USB_PID_IN : USB_PID_OUT;
	pIrp->bfrList [pIrp->bfrCount].pBfr = pBfr;
	pIrp->bfrList [pIrp->bfrCount].bfrLen = length;
	pIrp->bfrCount++;
	}

    /* All control transfers are followed by a "status" packet for which
     * the direction is the opposite of that for the data stage and the
     * length of the transfer is 0. */

    pIrp->bfrList [pIrp->bfrCount].pid = 
	((requestType & USB_RT_DEV_TO_HOST) != 0) ? USB_PID_OUT : USB_PID_IN;
    pIrp->bfrList [pIrp->bfrCount].pBfr = NULL;
    pIrp->bfrList [pIrp->bfrCount].bfrLen = 0;
    pIrp->bfrCount++;


    /* Store info about pending control IRP in node struct */

    pNode->pClient = pClient;
    pNode->pUrb = pUrb;
    pNode->pActLen = pActLen;


    /* submit the control transfer IRP.
     *
     * NOTE: If usbHcdIrpSubmit fails, it will still invoke the IRP callback
     * which will in turn release the controlSem taken above.  The
     * callback will also set the URB completion status.
     */

    if (usbdTransfer (internalClient, pNode->controlPipe, pIrp) != OK)
	return S_usbdLib_GENERAL_FAULT;

    return PENDING;
    }



/***************************************************************************
*
* resetDataToggle - reset data toggle on affected pipes
*
* This function is called when a "configuration event" is detected
* for a given node.  This function searches all pipes associated with
* the node for any that might be affected by the configuration event
* and resets their data toggles to DATA0.
*
* RETURNS: N/A
*/

LOCAL VOID resetDataToggle
    (
    USBD_NODE_ID nodeId,
    UINT16 configuration,
    UINT16 interface,
    UINT16 endpoint
    )

    {
    pUSBD_NODE pNode;
    pUSBD_PIPE pPipe;

    if (!validateNode (nodeId, &pNode))
	return;

    pPipe = usbListFirst (&pNode->pipes);

    while (pPipe != NULL)
	{
	if (configuration == ANY_CONFIGURATION ||
	    configuration == pPipe->configuration)
	    {
	    if (interface == ANY_INTERFACE ||
		interface == pPipe->interface)
		{
		if (endpoint == ANY_ENDPOINT ||
		    endpoint == pPipe->endpoint)
		    {
		    pPipe->dataToggle = USB_DATA0;
		    }
		}
	    }

	pPipe = usbListNext (&pPipe->nodePipeLink);
	}
    }


/***************************************************************************
*
* fncFeatureClear - Clear a USB feature
*
* RETURNS: S_usbdLib_xxxx
*/

LOCAL int fncFeatureClear
    (
    pURB_FEATURE_CLEAR_SET pUrb
    )

    {
    /* Check if this constitutes a configuration event.  So, reset the 
     * data toggle for any affected pipes.
     */

    if (pUrb->requestType == (USB_RT_STANDARD | USB_RT_ENDPOINT) &&
	pUrb->feature == USB_FSEL_DEV_ENDPOINT_HALT)
	{
	resetDataToggle (pUrb->nodeId, ANY_CONFIGURATION, ANY_INTERFACE, 
	    pUrb->index);
	}

    return controlRequest (&pUrb->header, pUrb->nodeId, 
	USB_RT_HOST_TO_DEV | pUrb->requestType, USB_REQ_CLEAR_FEATURE, 
	pUrb->feature, pUrb->index, 0, NULL, NULL);
    }


/***************************************************************************
*
* fncFeatureSet - Set a USB feature
*
* RETURNS: S_usbdLib_xxxx
*/

LOCAL int fncFeatureSet
    (
    pURB_FEATURE_CLEAR_SET pUrb
    )

    {
    return controlRequest (&pUrb->header, pUrb->nodeId, 
	USB_RT_HOST_TO_DEV | pUrb->requestType, USB_REQ_SET_FEATURE, 
	pUrb->feature, pUrb->index, 0, NULL, NULL);
    }


/***************************************************************************
*
* fncConfigGet - Get a device's configuration
*
* RETURNS: S_usbdLib_xxxx
*/

LOCAL int fncConfigGet
    (
    pURB_CONFIG_GET_SET pUrb
    )

    {
    pUrb->configuration = 0;

    return controlRequest (&pUrb->header, pUrb->nodeId, 
	USB_RT_DEV_TO_HOST | USB_RT_STANDARD | USB_RT_DEVICE,
	USB_REQ_GET_CONFIGURATION, 0, 0, 1, &pUrb->configuration, NULL);
    }


/***************************************************************************
*
* fncConfigSet - Set a device's configuration
*
* RETURNS: S_usbdLib_xxxx
*/

LOCAL int fncConfigSet
    (
    pURB_CONFIG_GET_SET pUrb
    )

    {
    pUSBD_NODE pNode;
    pUSBD_NODE pParentNode;


    /* Validate parameters */

    if (!validateNode (pUrb->nodeId, &pNode))
	return S_usbdLib_BAD_HANDLE;


    /* Verify that the port to which this device is connected is capable of
     * providing the current needed by this device.
     */

    if (pNode != pNode->pBus->pRoot && pNode->pBus->pRoot != NULL)
	{
	/* Retrieve the pointer to the parent hub for this node */

	validateNode (pNode->nodeInfo.parentHubId, &pParentNode);

	/* Check the power required against the parent's capability */

	if (pUrb->maxPower > pParentNode->maxPowerPerPort)
	    return S_usbdLib_POWER_FAULT;
	}


    /* This constitutes a configuration event.	So, reset the data toggle
     * for any affected pipes.
     */

    resetDataToggle (pUrb->nodeId, pUrb->configuration, ANY_INTERFACE, 
	ANY_ENDPOINT);


    /* Set the device's configuration */

    return controlRequest (&pUrb->header, pUrb->nodeId, 
	USB_RT_HOST_TO_DEV | USB_RT_STANDARD | USB_RT_DEVICE,
	USB_REQ_SET_CONFIGURATION, pUrb->configuration, 0, 0, NULL, NULL);
    }


/***************************************************************************
*
* fncDescriptorGet - Retrieve a USB descriptor
*
* RETURNS: S_usbdLib_xxxx
*/

LOCAL int fncDescriptorGet
    (
    pURB_DESCRIPTOR_GET_SET pUrb
    )

    {
    return controlRequest (&pUrb->header, pUrb->nodeId,
	USB_RT_DEV_TO_HOST | pUrb->requestType, USB_REQ_GET_DESCRIPTOR,
	pUrb->descriptorType << 8 | pUrb->descriptorIndex,
	pUrb->languageId, pUrb->bfrLen, pUrb->pBfr, &pUrb->actLen);
    }


/***************************************************************************
*
* fncDescriptorSet - Sets a USB descriptor
*
* RETURNS: S_usbdLib_xxxx
*/

LOCAL int fncDescriptorSet
    (
    pURB_DESCRIPTOR_GET_SET pUrb
    )

    {
    return controlRequest (&pUrb->header, pUrb->nodeId,
	USB_RT_HOST_TO_DEV | pUrb->requestType, USB_REQ_SET_DESCRIPTOR,
	pUrb->descriptorType << 8 | pUrb->descriptorIndex,
	pUrb->languageId, pUrb->bfrLen, pUrb->pBfr, NULL);
    }


/***************************************************************************
*
* fncInterfaceGet - Returns a device's interface setting
*
* RETURNS: S_usbdLib_xxxx
*/

LOCAL int fncInterfaceGet
    (
    pURB_INTERFACE_GET_SET pUrb
    )

    {
    pUrb->alternateSetting = 0;

    return controlRequest (&pUrb->header, pUrb->nodeId,
	USB_RT_DEV_TO_HOST | USB_RT_STANDARD | USB_RT_INTERFACE,
	USB_REQ_GET_INTERFACE, 0, pUrb->interfaceIndex, 1, 
	&pUrb->alternateSetting, NULL);
    }


/***************************************************************************
*
* fncInterfaceSet - Sets a device's interface setting
*
* RETURNS: S_usbdLib_xxxx
*/

LOCAL int fncInterfaceSet
    (
    pURB_INTERFACE_GET_SET pUrb
    )

    {
    /* This constitutes a configuration event.	So, reset the data toggle
     * for any affected pipes.
     */

    resetDataToggle (pUrb->nodeId, ANY_CONFIGURATION, pUrb->interfaceIndex,
	ANY_ENDPOINT);

    return controlRequest (&pUrb->header, pUrb->nodeId,
	USB_RT_HOST_TO_DEV | USB_RT_STANDARD | USB_RT_INTERFACE,
	USB_REQ_SET_INTERFACE, pUrb->alternateSetting, pUrb->interfaceIndex,
	0, NULL, NULL);
    }


/***************************************************************************
*
* fncStatusGet - Returns a device/interface/endpoint status word
*
* RETURNS: S_usbdLib_xxxx
*/

LOCAL int fncStatusGet
    (
    pURB_STATUS_GET pUrb
    )

    {
    return controlRequest (&pUrb->header, pUrb->nodeId,
	USB_RT_DEV_TO_HOST | pUrb->requestType, USB_REQ_GET_STATUS, 
	0, pUrb->index, pUrb->bfrLen, pUrb->pBfr, &pUrb->actLen);
    }


/***************************************************************************
*
* fncAddressGet - Returns a node's USB address
*
* RETURNS: S_usbdLib_xxxx
*/

LOCAL int fncAddressGet
    (
    pURB_ADDRESS_GET_SET pUrb
    )

    {
    pUSBD_NODE pNode;

    /* validate node handle */

    if (!validateNode (pUrb->nodeId, &pNode))
	return S_usbdLib_BAD_HANDLE;

    /* return current USB address for node */

    pUrb->deviceAddress = pNode->busAddress;

    return OK;
    }


/***************************************************************************
*
* fncAddressSet - Sets a node's USB address
*
* RETURNS: S_usbdLib_xxxx
*/

LOCAL int fncAddressSet
    (
    pURB_ADDRESS_GET_SET pUrb
    )

    {
    int s;

    s = controlRequest (&pUrb->header, pUrb->nodeId, 
	USB_RT_HOST_TO_DEV | USB_RT_STANDARD | USB_RT_DEVICE,
	USB_REQ_SET_ADDRESS, pUrb->deviceAddress, 0, 0, NULL, NULL);

    OSS_THREAD_SLEEP (USB_TIME_SET_ADDRESS);

    return s;
    }


/***************************************************************************
*
* fncNodeInfoGet - Returns information about a node
*
* RETURNS: S_usbdLib_xxxx
*/

LOCAL int fncNodeInfoGet
    (
    pURB_NODE_INFO_GET pUrb
    )

    {
    pUSBD_NODE pNode;

    /* Validate root */

    if (!validateNode (pUrb->nodeId, &pNode))
	return S_usbdLib_BAD_HANDLE;

    /* Validate other parameters */

    if (pUrb->pNodeInfo == NULL)
	return S_usbdLib_BAD_PARAM;

    /* Copy node info to caller's buffer */

    memcpy (pUrb->pNodeInfo, &pNode->nodeInfo,
	min (pUrb->infoLen, sizeof (pNode->nodeInfo)));

    return OK;
    }


/***************************************************************************
*
* fncVendorSpecific - Executes a vendor-specific USB request
*
* RETURNS: S_usbdLib_xxxx
*/

LOCAL int fncVendorSpecific
    (
    pURB_VENDOR_SPECIFIC pUrb
    )

    {
    return controlRequest (&pUrb->header, pUrb->nodeId,
	pUrb->requestType, pUrb->request, pUrb->value, pUrb->index,
	pUrb->length, pUrb->pBfr, &pUrb->actLen);
    }


/***************************************************************************
*
* fncPipeCreate - Create a new transfer pipe
*
* RETURNS: S_usbdLib_xxxx
*/

LOCAL int fncPipeCreate
    (
    pURB_PIPE_CREATE pUrb
    )

    {
    pUSBD_CLIENT pClient;
    pUSBD_NODE pNode;
    pUSBD_PIPE pPipe;
    pUSBD_BUS pBus;
    UINT32 nanoseconds = 0;
    HCD_PIPE_HANDLE hcdPipeHandle;


    /* validate the client handle for this request */

    if (!validateClient (pUrb->header.handle, &pClient))
	return S_usbdLib_BAD_HANDLE;


    /* validate the node to which this request is directed */

    if (!validateNode (pUrb->nodeId, &pNode))
	return S_usbdLib_BAD_HANDLE;


    /* Validate other parameters */

    if ((pUrb->transferType == USB_XFRTYPE_ISOCH ||
	pUrb->transferType == USB_XFRTYPE_INTERRUPT) &&
	pUrb->bandwidth == 0)
	return S_usbdLib_BAD_PARAM;

    if (pUrb->transferType == USB_XFRTYPE_INTERRUPT &&
	pUrb->serviceInterval == 0)
	return S_usbdLib_BAD_PARAM;


    /* Make sure a default packet size is specified */

    if (pUrb->maxPayload == 0)
	pUrb->maxPayload = USB_MIN_CTRL_PACKET_SIZE;


    /* Notify HCD of new pipe and make sure enough bandwidth is available to 
     * create the pipe. 
     */

    pBus = pNode->pBus;

    if (usbHcdPipeCreate (&pBus->pHcd->nexus, pBus->busNo, 
	pNode->busAddress, pUrb->endpoint, 
	pUrb->transferType, pUrb->direction, pNode->nodeInfo.nodeSpeed, 
	pUrb->maxPayload, pUrb->bandwidth, pUrb->serviceInterval, 
	&nanoseconds, &hcdPipeHandle) != OK)
	{
	return S_usbdLib_BANDWIDTH_FAULT;
	}


    /* Try to create a new pipe structure */

    if ((pPipe = OSS_CALLOC (sizeof (*pPipe))) == NULL ||
	usbHandleCreate (USBD_PIPE_SIG, pPipe, &pPipe->handle) != OK)
	{
	destroyPipe (pPipe);
	return S_usbdLib_OUT_OF_MEMORY;
	}

    pPipe->hcdHandle = hcdPipeHandle;
    pPipe->pClient = pClient;
    pPipe->pNode = pNode;
    pPipe->endpoint = pUrb->endpoint;
    pPipe->configuration = pUrb->configuration;
    pPipe->interface = pUrb->interface;
    pPipe->transferType = pUrb->transferType;
    pPipe->direction = pUrb->direction;
    pPipe->maxPacketSize = pUrb->maxPayload;
    pPipe->bandwidth = pUrb->bandwidth;
    pPipe->interval = pUrb->serviceInterval;
    pPipe->dataToggle = USB_DATA0;

    pPipe->nanoseconds = nanoseconds;
    pBus->nanoseconds += nanoseconds;


    /* Link pipe to list of pipes owned by this client */

    usbListLink (&pClient->pipes, pPipe, &pPipe->clientPipeLink, LINK_HEAD);


    /* Link pipe to list of pipes addressed to this node */

    usbListLink (&pNode->pipes, pPipe, &pPipe->nodePipeLink, LINK_HEAD);


    /* Return pipe handle */

    pUrb->pipeHandle = pPipe->handle;

    return OK;
    }


/***************************************************************************
*
* fncPipeDestroy - Destroy a transfer pipe
*
* RETURNS: S_usbdLib_xxxx
*/

LOCAL int fncPipeDestroy
    (
    pURB_PIPE_DESTROY pUrb
    )

    {
    pUSBD_PIPE pPipe;


    /* validate the pipe handle */

    if (!validatePipe (pUrb->pipeHandle, &pPipe))
	return S_usbdLib_BAD_HANDLE;


    /* Destroy the pipe */

    destroyPipe (pPipe);

    return OK;
    }


/***************************************************************************
*
* transferIrpCallback - invoked when a client's IRP completes
*
* Removes the IRP from the list of IRPs being tracked by the USBD and
* triggers the invocation of the client's IRP callback routine.
*
* NOTE: By convention, the USB_IRP.usbdPtr field points to the USBD_PIPE
* on which this transfer is being performed.
*
* RETURNS: N/A
*/

LOCAL VOID transferIrpCallback
    (
    pVOID p			/* pointer to completed IRP */
    )

    {
    pUSB_IRP pIrp = (pUSB_IRP) p;
    pUSBD_PIPE pPipe = (pUSBD_PIPE) pIrp->usbdPtr;
    UINT16 packets;
    UINT16 i;


    /* Unlink the IRP from the list of IRPs on this pipe. */

    usbListUnlink (&pIrp->usbdLink);


    /* Check if this IRP is being deleted.  If so, let the foreground
     * thread know the callback has been invoked.
     */

    if (pIrp == pPipe->irpBeingDeleted)
	pPipe->irpDeleted = TRUE;


    /* Update bus statistics */

    if (pIrp->result != OK)
	{
	if (pIrp->bfrList [0].pid == USB_PID_IN)
	    pPipe->pNode->pBus->stats.totalReceiveErrors++;
	else 
	    pPipe->pNode->pBus->stats.totalTransmitErrors++;
	}


    /* Update data toggle for pipe.  Control and isochronous transfers
     * always begin with DATA0, which is set at pipe initialization -
     * so we don't change it here.  Bulk and interrupt pipes alternate
     * between DATA0 and DATA1, and we need to keep a running track of
     * the state across IRPs.
     */

    if (pPipe->transferType == USB_XFRTYPE_INTERRUPT ||
	pPipe->transferType == USB_XFRTYPE_BULK)
	{
	/* Calculate the number of packets exchanged to determine the
	 * next data toggle value.  If the count of packets is odd, then
	 * the data toggle needs to switch.
	 *
	 * NOTE: If the IRP is successful, then at least one packet MUST
	 * have been transferred.  However, it may have been a 0-length
	 * packet.  This case is handled after the following "for" loop.
	 */

	packets = 0;

	for (i = 0; i < pIrp->bfrCount; i++)
	    {
	    packets += (pIrp->bfrList [i].actLen + pPipe->maxPacketSize - 1) /
		pPipe->maxPacketSize;
	    }

	if (pIrp->result == OK)
	    packets = max (packets, 1);

	if ((packets & 1) != 0)
	    pPipe->dataToggle = (pPipe->dataToggle == USB_DATA0) ?
		USB_DATA1 : USB_DATA0;
	}


    /* Invoke the user's callback routine */

    if (pIrp->userCallback != NULL)
	{
	/* If the userCallback routine is internal to the USBD, then
	 * invoke it directly as its behavior is known.  This
	 * streamlines processing of internally generated IRPs.
	 * Moreover, this eliminates a deadlock condition which can
	 * occur if a client's code invoked on the clientThread()
	 * reenters the USBD.  That thread might block waiting for
	 * the execution of a clientThread() callback - which would
	 * never execute.
	 */

	if (pIrp->userCallback == hubIrpCallback)
	    hubIrpCallback (pIrp);

	else if (pIrp->userCallback == controlIrpCallback)
	    controlIrpCallback (pIrp);

	else
	    usbQueuePut (pPipe->pClient->callbackQueue, 
		CALLBACK_FNC_IRP_COMPLETE, 0, (UINT32) pIrp, OSS_BLOCK);
	}
    }


/***************************************************************************
*
* fncTransfer - Initiates a transfer through a pipe
*
* RETURNS: S_usbdLib_xxxx
*/

LOCAL int fncTransfer
    (
    pURB_TRANSFER pUrb
    )

    {
    pUSBD_PIPE pPipe;
    pUSB_IRP pIrp;


    /* validate the pipe handle */

    if (!validatePipe (pUrb->pipeHandle, &pPipe))
	return S_usbdLib_BAD_HANDLE;


    /* validate the IRP */

    if ((pIrp = pUrb->pIrp) == NULL || pIrp->userCallback == NULL)
	return S_usbdLib_BAD_PARAM;


    /* Fill in IRP fields */

    pIrp->usbdPtr = pPipe;
    pIrp->usbdCallback = transferIrpCallback;
    pIrp->dataToggle = pPipe->dataToggle;


    /* Set an appropriate timeout value */

    if (pPipe->transferType == USB_XFRTYPE_CONTROL ||
	pPipe->transferType == USB_XFRTYPE_BULK)
	{
	if (pIrp->timeout == 0)
	    pIrp->timeout = USB_TIMEOUT_DEFAULT;
	}


    /* Update bus statistics */

    if (pIrp->bfrList [0].pid == USB_PID_IN)
	pPipe->pNode->pBus->stats.totalTransfersIn++;
    else 
	pPipe->pNode->pBus->stats.totalTransfersOut++;


    /* Link the IRP to the list of IRPs we're tracking on this pipe. */

    usbListLink (&pPipe->irps, pIrp, &pIrp->usbdLink, LINK_TAIL);


    /* Deliver the IRP to the HCD */

    if (usbHcdIrpSubmit (&pPipe->pNode->pBus->pHcd->nexus, 
	pPipe->hcdHandle, pIrp) != OK)
	{
	usbListUnlink (&pIrp->usbdLink);
	return S_usbdLib_HCD_FAULT;
	}

    return OK;
    }


/***************************************************************************
*
* fncTransferAbort - Abort a transfer request
*
* RETURNS: S_usbdLib_xxxx
*/

LOCAL int fncTransferAbort
    (
    pURB_TRANSFER pUrb
    )

    {
    pUSBD_PIPE pPipe;
    pUSB_IRP pIrp;


    /* validate the pipe handle */

    if (!validatePipe (pUrb->pipeHandle, &pPipe))
	return S_usbdLib_BAD_HANDLE;


    /* validate the IRP */

    if ((pIrp = pUrb->pIrp) == NULL)
	return S_usbdLib_BAD_PARAM;


    /* Cancel the IRP */

    return doTransferAbort (pPipe, pIrp);
    }


/***************************************************************************
*
* fncSynchFrameGet - Returns isochronous synch frame for an endpoint
*
* RETURNS: S_usbdLib_xxxx
*/

LOCAL int fncSynchFrameGet
    (
    pURB_SYNCH_FRAME_GET pUrb
    )

    {
    return controlRequest (&pUrb->header, pUrb->nodeId,
	USB_RT_DEV_TO_HOST | USB_RT_STANDARD | USB_RT_ENDPOINT,
	USB_REQ_GET_SYNCH_FRAME, 0, pUrb->endpoint, 2, 
	&pUrb->frameNo, NULL);
    }


/***************************************************************************
*
* fncCurrentFrameGet - Returns current frame number on a USB
*
* RETURNS: S_usbdLib_xxxx
*/

LOCAL int fncCurrentFrameGet
    (
    pURB_CURRENT_FRAME_GET pUrb
    )

    {
    pUSBD_NODE pNode;

    /* validate node handle */

    if (!validateNode (pUrb->nodeId, &pNode))
	return S_usbdLib_BAD_HANDLE;

    /* Retrieve the current frame number and frame window size from the
     * HCD for this root.
     */

    if (usbHcdCurrentFrameGet (&pNode->pBus->pHcd->nexus, pNode->pBus->busNo,
	&pUrb->frameNo, &pUrb->frameWindow) != OK)
	return S_usbdLib_HCD_FAULT;

    return OK;
    }


/***************************************************************************
*
* fncSofMasterTake - take SOF master ownership for a bus
*
* RETURNS: S_usbdLib_xxxx
*/

LOCAL int fncSofMasterTake
    (
    pURB_SOF_MASTER pUrb
    )

    {
    pUSBD_CLIENT pClient;
    pUSBD_NODE pNode;
    int s;

    /* validate parameters */

    if ((s = validateUrb (pUrb, sizeof (*pUrb), &pClient)) != OK)
	return s;

    if (!validateNode (pUrb->nodeId, &pNode))
	return S_usbdLib_BAD_HANDLE;


    /* Check if another client already is SOF master for the indicated bus */

    if (pNode->pBus->pSofMasterClient != NULL)
	return S_usbdLib_SOF_MASTER_FAULT;


    /* Make this client the SOF master. */

    pNode->pBus->pSofMasterClient = pClient;

    return OK;
    }


/***************************************************************************
*
* fncSofMasterRelease - release SOF master ownership for a bus
*
* RETURNS: S_usbdLib_xxxx
*/

LOCAL int fncSofMasterRelease
    (
    pURB_SOF_MASTER pUrb
    )

    {
    pUSBD_CLIENT pClient;
    pUSBD_NODE pNode;
    int s;

    /* validate parameters */

    if ((s = validateUrb (pUrb, sizeof (*pUrb), &pClient)) != OK)
	return s;

    if (!validateNode (pUrb->nodeId, &pNode))
	return S_usbdLib_BAD_HANDLE;


    /* Check if this client is the SOF master. */

    if (pNode->pBus->pSofMasterClient != pClient)
	return S_usbdLib_SOF_MASTER_FAULT;


    /* Clear the SOF master */

    pNode->pBus->pSofMasterClient = NULL;

    return OK;
    }


/***************************************************************************
*
* fncSofIntervalGet - retrieve SOF interval for a bus
*
* RETURNS: S_usbdLib_xxxx
*/

LOCAL int fncSofIntervalGet
    (
    pURB_SOF_INTERVAL_GET_SET pUrb
    )

    {
    pUSBD_CLIENT pClient;
    pUSBD_NODE pNode;
    int s;

    /* validate parameters */

    if ((s = validateUrb (pUrb, sizeof (*pUrb), &pClient)) != OK)
	return s;

    if (!validateNode (pUrb->nodeId, &pNode))
	return S_usbdLib_BAD_HANDLE;


    /* Retrieve the current SOF interval for this bus. */

    if (usbHcdSofIntervalGet (&pNode->pBus->pHcd->nexus, pNode->pBus->busNo,
	&pUrb->sofInterval) != OK)
	return S_usbdLib_HCD_FAULT;

    return OK;
    }


/***************************************************************************
*
* fncSofIntervalSet - set SOF interval for a bus
*
* RETURNS: S_usbdLib_xxxx
*/

LOCAL int fncSofIntervalSet
    (
    pURB_SOF_INTERVAL_GET_SET pUrb
    )

    {
    pUSBD_CLIENT pClient;
    pUSBD_NODE pNode;
    int s;

    /* validate parameters */

    if ((s = validateUrb (pUrb, sizeof (*pUrb), &pClient)) != OK)
	return s;

    if (!validateNode (pUrb->nodeId, &pNode))
	return S_usbdLib_BAD_HANDLE;


    /* Check if this client is the SOF master. */

    if (pNode->pBus->pSofMasterClient != pClient)
	return S_usbdLib_SOF_MASTER_FAULT;


    /* Set new SOF interval */

    if (usbHcdSofIntervalSet (&pNode->pBus->pHcd->nexus, pNode->pBus->busNo,
	pUrb->sofInterval) != OK)
	return S_usbdLib_HCD_FAULT;

    return OK;
    }


/***************************************************************************
*
* fncBusStateSet - sets bus state (e.g., suspend/resume)
*
* RETURNS: S_usbdLib_xxxx
*/

LOCAL int fncBusStateSet
    (
    pURB_BUS_STATE_SET pUrb
    )

    {
    pUSBD_CLIENT pClient;
    pUSBD_NODE pNode;
    pUSBD_BUS pBus;
    int s;

    /* validate parameters */

    if ((s = validateUrb (pUrb, sizeof (*pUrb), &pClient)) != OK)
	return s;

    if (!validateNode (pUrb->nodeId, &pNode))
	return S_usbdLib_BAD_HANDLE;

    /* Set the indicated bus state */

    pBus = pNode->pBus;

    if ((pUrb->busState & USBD_BUS_SUSPEND) != 0)
	{
	/* SUSPEND bus */

	if (!pBus->suspended)
	    {
	    if (usbHcdSetBusState (&pBus->pHcd->nexus, pBus->busNo,
		HCD_BUS_SUSPEND) != OK)
		return S_usbdLib_HCD_FAULT;

	    pBus->suspended = TRUE;
	    }
	}

    if ((pUrb->busState & USBD_BUS_RESUME) != 0)
	{
	/* RESUME bus */

	if (pBus->suspended)
	    {
	    if (usbHcdSetBusState (&pBus->pHcd->nexus, pBus->busNo,
		HCD_BUS_RESUME) != OK)
		return S_usbdLib_HCD_FAULT;

	    pBus->suspended = FALSE;
	    }
	}

    return OK;
    }


/***************************************************************************
*
* usbdCoreEntry - Primary entry point
*
* usbdCoreEntry is the primary entry point to the USBD through which callers
* invoke individual URBs (USB Request Blocks).
*
* NOTE: This function is intended exclusively for the use of functions in
* usbdLib.  Clients should not construct URBs manually and invoke this 
* function directly.
*
* RETURNS: OK or ERROR
*
* ERRNO:
*  S_usbdLib_BAD_PARAM
*/

STATUS usbdCoreEntry
    (
    pURB_HEADER pUrb			/* URB to be executed */
    )

    {
    int s;

    
    /* Validate parameters */

    if (pUrb == NULL || pUrb->urbLength < sizeof (URB_HEADER))
	return ossStatus (S_usbdLib_BAD_PARAM);


    /* Unless this call is for initialization or shutdown, take the
     * structMutex to prevent other threads from tampering with USBD
     * data structures.
     */

    if (pUrb->function != USBD_FNC_INITIALIZE && 
	pUrb->function != USBD_FNC_SHUTDOWN)
	{
	OSS_MUTEX_TAKE (structMutex, OSS_BLOCK);
	}


    /* Fan-out to appropriate function processor */

    switch (pUrb->function)
	{
	case USBD_FNC_INITIALIZE:   
	    s = fncInitialize (pUrb);
	    break;

	case USBD_FNC_SHUTDOWN:
	    s = fncShutdown (pUrb);
	    break;

	case USBD_FNC_CLIENT_REG:
	    s = fncClientReg ((pURB_CLIENT_REG) pUrb);
	    break;

	case USBD_FNC_CLIENT_UNREG:
	    s = fncClientUnreg ((pURB_CLIENT_UNREG) pUrb);
	    break;

	case USBD_FNC_MNGMT_CALLBACK_SET:
	    s = fncMngmtCallbackSet ((pURB_MNGMT_CALLBACK_SET) pUrb);
	    break;

	case USBD_FNC_VERSION_GET:
	    s = fncVersionGet ((pURB_VERSION_GET) pUrb);
	    break;

	case USBD_FNC_HCD_ATTACH:
	    s = fncHcdAttach ((pURB_HCD_ATTACH) pUrb);
	    break;

	case USBD_FNC_HCD_DETACH:
	    s = fncHcdDetach ((pURB_HCD_DETACH) pUrb);
	    break;

	case USBD_FNC_STATISTICS_GET:
	    s = fncStatisticsGet ((pURB_STATISTICS_GET) pUrb);
	    break;

	case USBD_FNC_BUS_COUNT_GET:
	    s = fncBusCountGet ((pURB_BUS_COUNT_GET) pUrb);
	    break;

	case USBD_FNC_ROOT_ID_GET:
	    s = fncRootIdGet ((pURB_ROOT_ID_GET) pUrb);
	    break;

	case USBD_FNC_HUB_PORT_COUNT_GET:
	    s = fncHubPortCountGet ((pURB_HUB_PORT_COUNT_GET) pUrb);
	    break;

	case USBD_FNC_NODE_ID_GET:
	    s = fncNodeIdGet ((pURB_NODE_ID_GET) pUrb);
	    break;

	case USBD_FNC_DYNA_ATTACH_REG:
	    s = fncDynaAttachReg ((pURB_DYNA_ATTACH_REG_UNREG) pUrb);
	    break;

	case USBD_FNC_DYNA_ATTACH_UNREG:
	    s = fncDynaAttachUnreg ((pURB_DYNA_ATTACH_REG_UNREG) pUrb);
	    break;

	case USBD_FNC_FEATURE_CLEAR:
	    s = fncFeatureClear ((pURB_FEATURE_CLEAR_SET) pUrb);
	    break;

	case USBD_FNC_FEATURE_SET:
	    s = fncFeatureSet ((pURB_FEATURE_CLEAR_SET) pUrb);
	    break;

	case USBD_FNC_CONFIG_GET:
	    s = fncConfigGet ((pURB_CONFIG_GET_SET) pUrb);
	    break;

	case USBD_FNC_CONFIG_SET:
	    s = fncConfigSet ((pURB_CONFIG_GET_SET) pUrb);
	    break;

	case USBD_FNC_DESCRIPTOR_GET:
	    s = fncDescriptorGet ((pURB_DESCRIPTOR_GET_SET) pUrb);
	    break;

	case USBD_FNC_DESCRIPTOR_SET:
	    s = fncDescriptorSet ((pURB_DESCRIPTOR_GET_SET) pUrb);
	    break;

	case USBD_FNC_INTERFACE_GET:
	    s = fncInterfaceGet ((pURB_INTERFACE_GET_SET) pUrb);
	    break;

	case USBD_FNC_INTERFACE_SET:
	    s = fncInterfaceSet ((pURB_INTERFACE_GET_SET) pUrb);
	    break;

	case USBD_FNC_STATUS_GET:
	    s = fncStatusGet ((pURB_STATUS_GET) pUrb);
	    break;

	case USBD_FNC_ADDRESS_GET:
	    s = fncAddressGet ((pURB_ADDRESS_GET_SET) pUrb);
	    break;

	case USBD_FNC_ADDRESS_SET:
	    s = fncAddressSet ((pURB_ADDRESS_GET_SET) pUrb);
	    break;

	case USBD_FNC_NODE_INFO_GET:
	    s = fncNodeInfoGet ((pURB_NODE_INFO_GET) pUrb);
	    break;

	case USBD_FNC_VENDOR_SPECIFIC:
	    s = fncVendorSpecific ((pURB_VENDOR_SPECIFIC) pUrb);
	    break;

	case USBD_FNC_PIPE_CREATE:
	    s = fncPipeCreate ((pURB_PIPE_CREATE) pUrb);
	    break;

	case USBD_FNC_PIPE_DESTROY:
	    s = fncPipeDestroy ((pURB_PIPE_DESTROY) pUrb);
	    break;

	case USBD_FNC_TRANSFER:
	    s = fncTransfer ((pURB_TRANSFER) pUrb);
	    break;

	case USBD_FNC_TRANSFER_ABORT:
	    s = fncTransferAbort ((pURB_TRANSFER) pUrb);
	    break;

	case USBD_FNC_SYNCH_FRAME_GET:
	    s = fncSynchFrameGet ((pURB_SYNCH_FRAME_GET) pUrb);
	    break;

	case USBD_FNC_CURRENT_FRAME_GET:
	    s = fncCurrentFrameGet ((pURB_CURRENT_FRAME_GET) pUrb);
	    break;

	case USBD_FNC_SOF_MASTER_TAKE:
	    s = fncSofMasterTake ((pURB_SOF_MASTER) pUrb);
	    break;

	case USBD_FNC_SOF_MASTER_RELEASE:
	    s = fncSofMasterRelease ((pURB_SOF_MASTER) pUrb);
	    break;

	case USBD_FNC_SOF_INTERVAL_GET:
	    s = fncSofIntervalGet ((pURB_SOF_INTERVAL_GET_SET) pUrb);
	    break;

	case USBD_FNC_SOF_INTERVAL_SET:
	    s = fncSofIntervalSet ((pURB_SOF_INTERVAL_GET_SET) pUrb);
	    break;

	case USBD_FNC_BUS_STATE_SET:
	    s = fncBusStateSet ((pURB_BUS_STATE_SET) pUrb);
	    break;

	default:    /* Bad function code */

	    s = S_usbdLib_BAD_PARAM;
	    break;
	}


    /* Store result. */

    setUrbResult (pUrb, s);


    /* release structMutex */

    if (pUrb->function != USBD_FNC_INITIALIZE && 
	pUrb->function != USBD_FNC_SHUTDOWN)
	{
	OSS_MUTEX_RELEASE (structMutex);
	}


    /* return status to caller */

    if (s == OK || s == PENDING)
	return OK;
    else
	return ossStatus (s);
    }


/* End of file. */

