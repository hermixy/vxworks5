/* usbQueueLib.c - O/S-independent queue functions */

/* Copyright 2000 Wind River Systems, Inc. */
/*
Modification history
--------------------
01a,07jun99,rcb  First.
*/

/*
DESCRIPTION

This file contains a generic implementation of O/S-independent queue
functions which are built on top of the the ossLib library's mutex
and semaphore functions.

The caller creates a queue of depth "n" by calling usbQueueCreate() and
receives a QUEUE_HANDLE in response.  The QUEUE_HANDLE must be used in
all subsequent calls to usbQueuePut(), usbQueueGet(), and usbQueueDestroy().

Each entry on a queue is described by a USB_QUEUE structure which contains
<msg>, <wParam>, and <lParam> fields.  The values of these fields are
completely arbitrary and may be used in any way desired by the calling
application.
*/

/* includes */

#include "usb/usbPlatform.h"

#include "string.h"

#include "usb/ossLib.h"     
#include "usb/usbQueueLib.h"	/* our API */


/* typedefs */

/* Internal queue control structure. */

typedef struct usb_queue 
    {
    UINT16 max; 		/* Number of USB_MESSAGE entries allocated */
				/* in messages array */
    UINT16 in;			/* Index of the next available message slot */
    UINT16 out; 		/* Index to the next message to be retrieved */
    SEM_HANDLE inUse;		/* Count of messages currently in queue */
    SEM_HANDLE empty;		/* Count of empty message slots in queue */
    MUTEX_HANDLE mutex; 	/* The handle of the mutex we use to gain */
				/* temporary control of the OSS_QUEUE struct */
    USB_MESSAGE messages [1];	/* Pointer to memory allocated for msg queue */
    } USB_QUEUE, *pUSB_QUEUE;


/* functions */

/***************************************************************************
*
* usbQueueCreate - Creates a O/S-independent queue structure
*
* This function creates a queue which can accomodate a number of 
* USB_MESSAGE entries according to the <depth> parameter.  Returns the
* <pQueueHandle) of the newly created queue if successful.
*
* RETURNS: OK or ERROR
*
* ERRNO:
*  S_usbQueueLib_BAD_PARAMETER
*  S_usbQueueLib_OUT_OF_MEMORY
*  S_usbQueueLib_OUT_OF_RESOURCES
*/

STATUS usbQueueCreate 
    (
    UINT16 depth,		    /* Max entries queue can handle */
    pQUEUE_HANDLE pQueueHandle	    /* Handle of newly created queue */
    )

    {
    pUSB_QUEUE pQueue;
    UINT16 len;


    /* Validate parameters (e.g., the depth of the queue must not be 0). */

    if (depth < 1 || pQueueHandle == NULL) {
	return ossStatus (S_usbQueueLib_BAD_PARAMETER);
    }


    /* Allocate the queue control structure. */

    len = sizeof (*pQueue) + sizeof (USB_MESSAGE) * (depth - 1);

    if ((pQueue = OSS_CALLOC (len)) == NULL)
	return ossStatus (S_usbQueueLib_OUT_OF_MEMORY);
    
    pQueue->max = depth;


    /* Allocate queue control resources. */

    if (OSS_SEM_CREATE (depth, 0, &pQueue->inUse) != OK ||
	OSS_SEM_CREATE (depth, depth, &pQueue->empty) != OK ||
	OSS_MUTEX_CREATE (&pQueue->mutex) != OK) 
	{
	usbQueueDestroy ((QUEUE_HANDLE) pQueue);
	return ossStatus (S_usbQueueLib_OUT_OF_RESOURCES);
	}

    *pQueueHandle = (QUEUE_HANDLE) pQueue;
    return OK;
}


/***************************************************************************
*
* usbQueueDestroy - Destroys a queue
*
* This function destroys a queue created by calling usbQueueCreate().
*
* RETURNS: OK or ERROR.
*
* ERRNO:
*  S_usbQueueLib_BAD_HANDLE
*/

STATUS usbQueueDestroy 
    (
    QUEUE_HANDLE queueHandle	    /* Hnadle of queue to destroy */
    )

    {
    pUSB_QUEUE pQueue = (pUSB_QUEUE) queueHandle;


    /* Qualify the queue handle. */

    if (pQueue == NULL)
	return ossStatus (S_usbQueueLib_BAD_HANDLE);


    /* Release resources and memory associated with the queue. */

    if (pQueue->inUse)
	OSS_SEM_DESTROY (pQueue->inUse);

    if (pQueue->empty)
	OSS_SEM_DESTROY (pQueue->empty);

    if (pQueue->mutex)
	OSS_SEM_DESTROY (pQueue->mutex);

    OSS_FREE (pQueue);

    return OK;
}


/***************************************************************************
*
* usbQueuePut - Puts a message onto a queue
*
* Places the specified <msg>, <wParam>, and <lParam> onto <queueHandle>.
* This function will only block if the specified queue is full.  If the 
* queue is full, <blockFlag> specifies the blocking behavior.  OSS_BLOCK
* blocks indefinitely.	OSS_DONT_BLOCK does not block and returns an error
* if the queue is full.  Other values of <blockFlag> are interpreted as
* a count of milliseconds to block before declaring a failure.
*
* RETURNS: OK or ERROR
*
* ERRNO:
*  S_usbQueueLib_BAD_HANDLE
*  S_usbQueueLib_Q_NOT_AVAILABLE
*/

STATUS usbQueuePut 
    (
    QUEUE_HANDLE queueHandle,	    /* queue handle */
    UINT16 msg, 		    /* app-specific message */
    UINT16 wParam,		    /* app-specific parameter */
    UINT32 lParam,		    /* app-specific parameter */
    UINT32 blockFlag		    /* specifies blocking action */
    )

    {
    pUSB_QUEUE pQueue = (pUSB_QUEUE) queueHandle;


    /* Qualify the queue handle. */

    if (pQueue == NULL)
	return ossStatus (S_usbQueueLib_BAD_HANDLE);


    /* Wait for space to become available in the queue */

    if ((OSS_SEM_TAKE (pQueue->empty, blockFlag)) != OK)
	{
	return ossStatus (S_usbQueueLib_Q_NOT_AVAILABLE);
	}
    else
	{

	/* Take the queue mutex before attempting to update the queue. */

	if ((OSS_MUTEX_TAKE (pQueue->mutex, OSS_BLOCK)) != OK)
	    {
	    return ossStatus (S_usbQueueLib_Q_NOT_AVAILABLE);
	    }
	else
	    {

	    /* Add an entry to the queue. */

	    pQueue->messages [pQueue->in].msg = msg;
	    pQueue->messages [pQueue->in].wParam = wParam;
	    pQueue->messages [pQueue->in].lParam = lParam;

	    if (++pQueue->in == pQueue->max)
		pQueue->in = 0;

	    OSS_SEM_GIVE (pQueue->inUse);

	    OSS_MUTEX_RELEASE (pQueue->mutex);
	    }
	}

    return OK;
    }


/***************************************************************************
*
* usbQueueGet - Retrieves a message from a queue
*
* Retrieves a message from the specified <queueHandle> and stores it in
* <pOssMsg>.  If the queue is empty, <blockFlag> specifies the blocking
* behavior.  OSS_BLOCK blocks indefinitely.  OSS_DONT_BLOCK does not block
* and returns an error immediately if the queue is empty.  Other values of
* <blockFlag> are interpreted as a count of milliseconds to block before
* declaring a failure.
*
* RETURNS: OK or ERROR
* 
* ERRNO:
*  S_usbQueueLib_BAD_HANDLE
*  S_usbQueueLib_Q_NOT_AVAILABLE
*/

STATUS usbQueueGet 
    (
    QUEUE_HANDLE queueHandle,	    /* queue handle */
    pUSB_MESSAGE pMsg,		    /* USB_MESSAGE to receive msg */
    UINT32 blockFlag		    /* specifies blocking action */
    )

    {
    pUSB_QUEUE pQueue = (pUSB_QUEUE) queueHandle;


    /* Qualify the queue handle and msg parameters. */

    if (pQueue == NULL)
	return ossStatus (S_usbQueueLib_BAD_HANDLE);

    if (pMsg == NULL)
	return ossStatus (S_usbQueueLib_BAD_PARAMETER);


    /* Wait for a message to arrive in the queue before proceeding */

    if ((OSS_SEM_TAKE (pQueue->inUse, blockFlag)) != OK) 
	{
	return ossStatus (S_usbQueueLib_Q_NOT_AVAILABLE);
	}
    else
	{

	/* Take the queue mutex before attempting to update the queue. */

	if ((OSS_MUTEX_TAKE (pQueue->mutex, OSS_BLOCK)) != OK) 
	    {
	    return ossStatus (S_usbQueueLib_Q_NOT_AVAILABLE);
	    }
	else
	    {

	    /* Take an entry off the queue. */

	    memcpy (pMsg, &pQueue->messages [pQueue->out], 
		sizeof (USB_MESSAGE));

	    if (++pQueue->out == pQueue->max)
		pQueue->out = 0;

	    OSS_SEM_GIVE (pQueue->empty);

	    OSS_MUTEX_RELEASE (pQueue->mutex);
	    }
	}

    return OK;
    }


/* End of file */

