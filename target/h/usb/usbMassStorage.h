/* usbMassStorage.h - Definitions for USB mass storage class */

/* Copyright 2000 Wind River Systems, Inc. */

/*
Modification history
--------------------
01a,14dec00,wef  First.
*/

#ifndef __INCusbMassStorageh
#define __INCusbMassStorageh

#ifdef	__cplusplus
extern "C" {
#endif


/* typedefs */


/* USB Command execution status codes   */
 
typedef enum
    {
    USB_COMMAND_SUCCESS = 0,
    USB_COMMAND_FAILED,
    USB_INVALID_CSW,
    USB_PHASE_ERROR,
    USB_IRP_FAILED,
    USB_DATA_INCOMPLETE,
    USB_BULK_IO_ERROR,
    USB_INTERNAL_ERROR
    } USB_COMMAND_STATUS;



#ifdef	__cplusplus
}
#endif

#endif	/* __INCusbMassStorageh */


/* End of file. */

