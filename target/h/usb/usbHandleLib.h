/* usbHandleLib.h - handle utility functions */

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
01b,07mar00,rcb  Change definition of GENERIC_HANDLE from UINT32 to
		 pVOID so handles can be compared against NULL without
		 generating warnings (e.g., on MIPS gnu toolchain).
01a,07jun99,rcb  First.
*/

/*
DESCRIPTION

Defines a set of general-purpose handle creation and validation functions.

Using these services, libraries can return handles to callers which can 
subsequently be validated for authenticity.  This provides libraries with
an additional measure of "bullet-proofing."
*/

#ifndef __INCusbHandleLibh
#define __INCusbHandleLibh

#ifdef	__cplusplus
extern "C" {
#endif

/* includes */

#include "usb/usbPlatform.h" 		/* USB Module number def's */
#include "vwModNum.h" 		/* USB Module number def's */


/* defines */

/* Handle utility library return values */

/* 
 * USB errnos are defined as being part of the USB host Module, as are all
 * vxWorks module numbers, but the USB Module number is further divided into 
 * sub-modules.  Each sub-module has upto 255 values for its own error codes
 */
 
#define USB_HANDLE_SUB_MODULE  3

#define M_usbHandleLib 	( (USB_HANDLE_SUB_MODULE  << 8) | M_usbHostLib )

#define hdlErr(x)	(M_usbHandleLib | (x))

#define S_usbHandleLib_OUT_OF_MEMORY	    hdlErr(1)
#define S_usbHandleLib_OUT_OF_RESOURCES     hdlErr(2)
#define S_usbHandleLib_OUT_OF_HANDLES	    hdlErr(3)
#define S_usbHandleLib_BAD_PARAM	    hdlErr(4)
#define S_usbHandleLib_BAD_HANDLE	    hdlErr(5)
#define S_usbHandleLib_NOT_INITIALIZED	    hdlErr(6)
#define S_usbHandleLib_GENERAL_FAULT	    hdlErr(7)


/* typedefs */

typedef pVOID GENERIC_HANDLE;	    /* type of a generic handle */
typedef GENERIC_HANDLE *pGENERIC_HANDLE;


/* functions */

STATUS usbHandleInitialize
    (
    UINT32 maxHandles		    /* max handles allocated by library */
    );


STATUS usbHandleShutdown (void);


STATUS usbHandleCreate
    (
    UINT32 handleSignature,	    /* Arbitrary handle signature */
    pVOID handleParam,		    /* Arbitrary handle parameter */
    pGENERIC_HANDLE pHandle	    /* Newly allocated handle */
    );


STATUS usbHandleDestroy
    (
    GENERIC_HANDLE handle	    /* handle to be destroyed */
    );


STATUS usbHandleValidate
    (
    GENERIC_HANDLE handle,	    /* handle to be validated */
    UINT32 handleSignature,	    /* signature used to validate handle */
    pVOID *pHandleParam 	    /* Handle parameter on return */
    );


#ifdef	__cplusplus
}
#endif

#endif	/* __INCusbHandleLibh */


/* End of file. */

