/* usbPciLib.h - System-specific PCI Functions */

/* Copyright 2000 Wind River Systems, Inc. */
/*
Modification history
--------------------
01c,08mar00,rcb  Correct prototype for usbPciMemioOffset()...
		 was incorrectly called usbPciMemOffset().
01b,10dec99,rcb  Add definition for usbPciMemioOffset().
01a,31may99,rcb  First.
*/

/*
DESCRIPTION

This file defines platform-independent functions accessing PCI bus 
capabilities.  These functions allow PCI device drivers to be written 
independent of the underlying O/S's PCI access mechanisms.
*/

#ifndef __INCusbPciLibh
#define __INCusbPciLibh


/* Includes */

#include "usb/pciConstants.h"


#ifdef	__cplusplus
extern "C" {
#endif


/* typedefs */

/* function prototype for an interrupt vector */

typedef VOID (*INT_HANDLER_PROTOTYPE) (pVOID param);


/* Function prototypes */

BOOL usbPciClassFind
    (
    UINT8 pciClass,		/* PCI device class */
    UINT8 subClass,		/* PCI device sub-class */
    UINT8 pgmIf,		/* Programming interface */
    UINT16 index,		/* Caller wants nth matching dev */
    pUINT8 pBusNo,		/* Bus number of matching dev */
    pUINT8 pDeviceNo,		/* Device number of matching dev */
    pUINT8 pFuncNo		/* Function number of matching dev */
    );


UINT8 usbPciByteGet 
    (
    UINT8 busNo,		/* Bus number of device */
    UINT8 deviceNo,		/* Device number of device */
    UINT8 funcNo,		/* Function number of device */
    UINT16 regOffset		/* Offset into PCI config space */
    );

#define USB_PCI_CFG_BYTE_GET(busNo, deviceNo, funcNo, regOffset) \
    usbPciByteGet (busNo, deviceNo, funcNo, regOffset)


UINT32 usbPciWordGet
    (
    UINT8 busNo,		/* Bus number of device */
    UINT8 deviceNo,		/* Device number of device */
    UINT8 funcNo,		/* Function number of device */
    UINT16 regOffset		/* Offset into PCI config space */
    );

#define USB_PCI_CFG_WORD_GET(busNo, deviceNo, funcNo, regOffset) \
    usbPciWordGet (busNo, deviceNo, funcNo, regOffset)


UINT32 usbPciDwordGet
    (
    UINT8 busNo,		/* Bus number of device */
    UINT8 deviceNo,		/* Device number of device */
    UINT8 funcNo,		/* Function number of device */
    UINT16 regOffset		/* Offset into PCI config space */
    );

#define USB_PCI_CFG_DWORD_GET(busNo, deviceNo, funcNo, regOffset) \
    usbPciDwordGet (busNo, deviceNo, funcNo, regOffset)


VOID usbPciConfigHeaderGet
    (
    UINT8 busNo,		/* Bus number of device */
    UINT8 deviceNo,		/* Device number of device */
    UINT8 funcNo,		/* Function number of device */
    pPCI_CFG_HEADER pCfgHdr	/* Buffer provided by caller */
    );


UINT8 usbPciByteIn
    (
    UINT32 address		/* PCI I/O address */
    );

#define USB_PCI_BYTE_IN(address) usbPciByteIn (address)


UINT16 usbPciWordIn
    (
    UINT32 address		/* PCI I/O address */
    );

#define USB_PCI_WORD_IN(address) usbPciWordIn (address)


UINT32 usbPciDwordIn
    (
    UINT32 address		/* PCI I/O address */
    );

#define USB_PCI_DWORD_IN(address) usbPciDwordIn (address)


VOID usbPciByteOut
    (
    UINT32 address,		/* PCI I/O address */
    UINT8 value 		/* value */
    );

#define USB_PCI_BYTE_OUT(address, value) usbPciByteOut (address, value)


VOID usbPciWordOut
    (
    UINT32 address,		/* PCI I/O address */
    UINT16 value		/* value */
    );

#define USB_PCI_WORD_OUT(address, value) usbPciWordOut (address, value)


VOID usbPciDwordOut
    (
    UINT32 address,		/* PCI I/O address */
    UINT32 value		/* value */
    );

#define USB_PCI_DWORD_OUT(address, value) usbPciDwordOut (address, value)


UINT32 usbPciMemioOffset (void);

#define USB_PCI_MEMIO_OFFSET()	usbPciMemioOffset ()



UINT32 usbMemToPci
    (
    pVOID pMem			/* memory address to convert */
    );

#define USB_MEM_TO_PCI(pMem) usbMemToPci (pMem)


pVOID usbPciToMem
    (
    UINT32 pciAdrs		/* 32-bit PCI address to be converted */
    );

#define USB_PCI_TO_MEM(pciAdrs) usbPciToMem (pciAdrs)


VOID usbPciMemInvalidate
    (
    pVOID pMem, 		/* base of memory region to invalidate */
    UINT32 size 		/* size of region to invalidate */
    );

#define USB_PCI_MEM_INVALIDATE(pMem, size) usbPciMemInvalidate (pMem, size)


VOID usbPciMemFlush
    (
    pVOID pMem, 		/* base of memory region to invalidate */
    UINT32 size 		/* size of region to invalidate */
    );

#define USB_PCI_MEM_FLUSH(pMem, size) usbPciMemFlush (pMem, size) 


STATUS usbPciIntConnect
    (
    INT_HANDLER_PROTOTYPE func,     /* new interrupt handler */
    pVOID param,		    /* parameter for int handler */
    UINT16 intNo		    /* interrupt vector number */
    );


VOID usbPciIntRestore
    (
    INT_HANDLER_PROTOTYPE func,     /* int handler to be removed */
    pVOID param,		    /* parameter for int handler */
    UINT16 intNo		    /* interrupt vector number */
    );



#ifdef	__cplusplus
}
#endif

#endif	/* __INCusbPciLibh */


/* End of file. */


