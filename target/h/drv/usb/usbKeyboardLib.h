/* usbKeyboardLib.h - USB keyboard SIO driver definitions */

/* Copyright 2000 Wind River Systems, Inc. */

/*
Modification history
--------------------
01e,18sep01,wef  merge from wrs.tor2_0.usb1_1-f for veloce
01d,07may01,wef  changed module number to be (module num << 8) | M_usbHostLib
01c,02may01,wef  changed module number to be M_<module> + M_usbHostLib
01b,05dec00,wef  moved Module number defs to vwModNum.h - add this
                 to #includes
01a,27jul99,rcb  First.
*/

#ifndef __INCusbKeyboardLibh
#define __INCusbKeyboardLibh

#ifdef	__cplusplus
extern "C" {
#endif


/* includes */

#include "sioLib.h"
#include "usb/usbPlatform.h"
#include "vwModNum.h"           /* USB Module Number Def's */


/* defines */

/* usbKeyboardLib error values */

/* 
 * USB errnos are defined as being part of the USB host Module, as are all
 * vxWorks module numbers, but the USB Module number is further divided into 
 * sub-modules.  Each sub-module has upto 255 values for its own error codes
 */
 
#define USB_KEYBOARD_SUB_MODULE  6

#define M_usbKeyboardLib 	( (USB_KEYBOARD_SUB_MODULE  << 8) | \
				  M_usbHostLib )

#define usbKbdErr(x)		(M_usbKeyboardLib | (x))

#define S_usbKeyboardLib_NOT_INITIALIZED    usbKbdErr (1)
#define S_usbKeyboardLib_BAD_PARAM	    usbKbdErr (2)
#define S_usbKeyboardLib_OUT_OF_MEMORY	    usbKbdErr (3)
#define S_usbKeyboardLib_OUT_OF_RESOURCES   usbKbdErr (4)
#define S_usbKeyboardLib_GENERAL_FAULT	    usbKbdErr (5)
#define S_usbKeyboardLib_QUEUE_FULL	    usbKbdErr (6)
#define S_usbKeyboardLib_QUEUE_EMPTY	    usbKbdErr (7)
#define S_usbKeyboardLib_NOT_IMPLEMENTED    usbKbdErr (8)
#define S_usbKeyboardLib_USBD_FAULT	    usbKbdErr (9)
#define S_usbKeyboardLib_NOT_REGISTERED     usbKbdErr (10)
#define S_usbKeyboardLib_NOT_LOCKED	    usbKbdErr (11)


/* USB_KBD_xxxx define "attach codes" used by USB_KBD_ATTACH_CALLBACK. */

#define USB_KBD_ATTACH	0	    /* new keyboard attached */
#define USB_KBD_REMOVE	1	    /* keyboard has been removed */
				    /* SIO_CHAN no longer valid */


/* typedefs */

/* USB_KBD_ATTACH_CALLBACK defines a callback routine which will be
 * invoked by usbKeyboardLib.c when the attachment or removal of a keyboard
 * is detected.  When the callback is invoked with an attach code of
 * USB_KBD_ATTACH, the pSioChan points to a newly created SIO_CHAN.  When
 * the attach code is USB_KBD_REMOVE, the pSioChan points to a pSioChan
 * for a keyboard which is no longer attached.
 */

typedef VOID (*USB_KBD_ATTACH_CALLBACK) 
    (
    pVOID arg,			    /* caller-defined argument */
    SIO_CHAN *pChan,		    /* pointer to affected SIO_CHAN */
    UINT16 attachCode		    /* defined as USB_KBD_xxxx */
    );


/* function prototypes */

STATUS usbKeyboardDevInit (void);
STATUS usbKeyboardDevShutdown (void);

STATUS usbKeyboardDynamicAttachRegister
    (
    USB_KBD_ATTACH_CALLBACK callback,	/* new callback to be registered */
    pVOID arg				/* user-defined arg to callback */
    );

STATUS usbKeyboardDynamicAttachUnRegister
    (
    USB_KBD_ATTACH_CALLBACK callback,	/* callback to be unregistered */
    pVOID arg				/* user-defined arg to callback */
    );

STATUS usbKeyboardSioChanLock
    (
    SIO_CHAN *pChan		    /* SIO_CHAN to be marked as in use */
    );

STATUS usbKeyboardSioChanUnlock
    (
    SIO_CHAN *pChan		    /* SIO_CHAN to be marked as unused */
    );


#ifdef	__cplusplus
}
#endif

#endif	/* __INCusbKeyboardLibh */


/* End of file. */

