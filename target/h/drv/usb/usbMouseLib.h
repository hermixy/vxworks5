/* usbMouseLib.h - USB mouse SIO driver definitions */

/* Copyright 2000 Wind River Systems, Inc. */

/*
Modification history
--------------------
01e,18sep01,wef  merge from wrs.tor2_0.usb1_1-f for veloce
01d,07may01,wef changed module number to be (module num << 8) | M_usbHostLib
01c,02may01,wef  changed module number to be M_<module> + M_usbHostLib
01b,05dec00,wef  moved Module number defs to vwModNum.h - add this
                 to #includes
01a,07oct99,rcb  First.
*/

#ifndef __INCusbMouseLibh
#define __INCusbMouseLibh

#ifdef	__cplusplus
extern "C" {
#endif


/* includes */

#include "sioLib.h"
#include "usb/usbPlatform.h"
#include "usb/usbHid.h"
#include "vwModNum.h"           /* USB Module Number Def's */


/* defines */

/* usbMouseLib error values */

/* 
 * USB errnos are defined as being part of the USB host Module, as are all
 * vxWorks module numbers, but the USB Module number is further divided into 
 * sub-modules.  Each sub-module has upto 255 values for its own error codes
 */
 
#define USB_MOUSE_SUB_MODULE 8 

#define M_usbMouseLib 	( (USB_MOUSE_SUB_MODULE << 8) | M_usbHostLib )

#define usbMseErr(x)	(M_usbMouseLib | (x))

#define S_usbMouseLib_NOT_INITIALIZED	usbMseErr (1)
#define S_usbMouseLib_BAD_PARAM 	usbMseErr (2)
#define S_usbMouseLib_OUT_OF_MEMORY	usbMseErr (3)
#define S_usbMouseLib_OUT_OF_RESOURCES	usbMseErr (4)
#define S_usbMouseLib_GENERAL_FAULT	usbMseErr (5)
#define S_usbMouseLib_QUEUE_FULL	usbMseErr (6)
#define S_usbMouseLib_QUEUE_EMPTY	usbMseErr (7)
#define S_usbMouseLib_NOT_IMPLEMENTED	usbMseErr (8)
#define S_usbMouseLib_USBD_FAULT	usbMseErr (9)
#define S_usbMouseLib_NOT_REGISTERED	usbMseErr (10)
#define S_usbMouseLib_NOT_LOCKED	usbMseErr (11)


/* Additional callback types for "callbackInstall" function */

#define SIO_CALLBACK_PUT_MOUSE_REPORT	128


/* USB_MSE_xxxx define "attach codes" used by USB_MSE_ATTACH_CALLBACK. */

#define USB_MSE_ATTACH	0	    /* new mouse attached */
#define USB_MSE_REMOVE	1	    /* mouse has been removed */
				    /* SIO_CHAN no longer valid */


/* typedefs */

/* USB_MSE_ATTACH_CALLBACK defines a callback routine which will be
 * invoked by usbMouseLib.c when the attachment or removal of a mouse
 * is detected.  When the callback is invoked with an attach code of
 * USB_MSE_ATTACH, the pSioChan points to a newly created SIO_CHAN.  When
 * the attach code is USB_MSE_REMOVE, the pSioChan points to a pSioChan
 * for a mouse which is no longer attached.
 */

typedef VOID (*USB_MSE_ATTACH_CALLBACK) 
    (
    pVOID arg,			    /* caller-defined argument */
    SIO_CHAN *pChan,		    /* pointer to affected SIO_CHAN */
    UINT16 attachCode		    /* defined as USB_MSE_xxxx */
    );


/* function prototypes */

STATUS usbMouseDevInit (void);
STATUS usbMouseDevShutdown (void);

STATUS usbMouseDynamicAttachRegister
    (
    USB_MSE_ATTACH_CALLBACK callback,	/* new callback to be registered */
    pVOID arg				/* user-defined arg to callback */
    );

STATUS usbMouseDynamicAttachUnRegister
    (
    USB_MSE_ATTACH_CALLBACK callback,	/* callback to be unregistered */
    pVOID arg				/* user-defined arg to callback */
    );

STATUS usbMouseSioChanLock
    (
    SIO_CHAN *pChan		    /* SIO_CHAN to be marked as in use */
    );

STATUS usbMouseSioChanUnlock
    (
    SIO_CHAN *pChan		    /* SIO_CHAN to be marked as unused */
    );


#ifdef	__cplusplus
}
#endif

#endif	/* __INCusbMouseLibh */


/* End of file. */

