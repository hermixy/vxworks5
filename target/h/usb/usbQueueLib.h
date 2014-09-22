/* usbQueueLib.h - O/S-independent queue services */

/* Copyright 2000 Wind River Systems, Inc. */
/*
Modification history
--------------------
01f,18sep01,wef  merge from wrs.tor2_0.usb1_1-f for veloce
01e,07may01,wef  changed module number to be (module sub num << 8) | 
		 M_usbHostLib
01d,02may01,wef  changed module number to be M_<module> + M_usbHostLib
01c,05dec00,wef  moved Module number defs to vwModNum.h - add this
                 to #includes
01b,07mar00,rcb  Change QUEUE_HANDLE from UINT32 to pVOID.
01a,13jul99,rcb  First.
*/

/*
DESCRIPTION

Defines an O/S-independent queueing mechanism which is typically used for
inter-thread communication in a multi-threaded environment.
*/


#ifndef __INCusbQueueLibh
#define __INCusbQueueLibh

#ifdef	__cplusplus
extern "C" {
#endif


/* includes */

#include "usb/ossLib.h"
#include "vwModNum.h"           /* USB Module number def's */


/* Defines */

/* USB return codes. */

/* 
 * USB errnos are defined as being part of the USB host Module, as are all
 * vxWorks module numbers, but the USB Module number is further divided into 
 * sub-modules.  Each sub-module has upto 255 values for its own error codes
 */
 
#define USB_QUEUE_SUB_MODULE  	2

#define M_usbQueueLib 		( (USB_QUEUE_SUB_MODULE  << 8) | M_usbHostLib )

#define usbQueueErr(x)		(M_usbQueueLib | (x))

#define S_usbQueueLib_BAD_HANDLE	usbQueueErr (1)
#define S_usbQueueLib_BAD_PARAMETER	usbQueueErr (2)
#define S_usbQueueLib_OUT_OF_MEMORY	usbQueueErr (3)
#define S_usbQueueLib_OUT_OF_RESOURCES	usbQueueErr (4)
#define S_usbQueueLib_Q_NOT_AVAILABLE	usbQueueErr (5)


/* Structure definitions. */

/* Handles. */

typedef pVOID QUEUE_HANDLE;	    /* Queue handle */
typedef QUEUE_HANDLE *pQUEUE_HANDLE;


/* 
 * USB_MESSAGE 
 *
 * Defines the format of a message which can be passed through a queue.
 */

typedef struct usb_message
    {
    UINT16 msg; 		    /* Message code - application specific */
    UINT16 wParam;		    /* WORD parameter - application specific */
    UINT32 lParam;		    /* DWORD parameter - application specific */
    } USB_MESSAGE, *pUSB_MESSAGE;


/* function prototypes */

STATUS usbQueueCreate 
    (
    UINT16 depth,		    /* Max entries queue can handle */
    pQUEUE_HANDLE pQueueHandle	    /* Handle of newly created queue */
    );


STATUS usbQueueDestroy 
    (
    QUEUE_HANDLE queueHandle	    /* Hnadle of queue to destroy */
    );


STATUS usbQueuePut 
    (
    QUEUE_HANDLE queueHandle,	    /* queue handle */
    UINT16 msg, 		    /* app-specific message */
    UINT16 wParam,		    /* app-specific parameter */
    UINT32 lParam,		    /* app-specific parameter */
    UINT32 blockFlag		    /* specifies blocking action */
    );


STATUS usbQueueGet 
    (
    QUEUE_HANDLE queueHandle,	    /* queue handle */
    pUSB_MESSAGE pMsg,		    /* USB_MESSAGE to receive msg */
    UINT32 blockFlag		    /* specifies blocking action */
    );


#ifdef	__cplusplus
}
#endif

#endif	/* __INCusbQueueLibh */


/* End of file. */

