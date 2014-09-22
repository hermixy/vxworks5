/* usbDescrCopyLib.h - USB descriptor copy utility functions */

/* Copyright 2000 Wind River Systems, Inc. */

/*
Modification history
--------------------
01b,13apr00,wef  fixed #ifndef bug
01a,05apr00,wef  First -removed these functions from usbLib.c to decouple host /
	    peripheral stack interdependency.
*/

#ifndef __INCusbDescrCopyLibh
#define __INCusbDescrCopyLibh

#ifdef	__cplusplus
extern "C" {
#endif


/* includes */

#include "usb/usb.h"


/* function prototypes */

VOID usbDescrCopy
    (
    pUINT8 pBfr,		/* destination buffer */
    pVOID pDescr,		/* source buffer */
    UINT16 bfrLen,		/* dest len */
    pUINT16 pActLen		/* actual length copied */
    );


VOID usbDescrCopy32
    (
    pUINT8 pBfr,		/* destination buffer */
    pVOID pDescr,		/* source buffer */
    UINT32 bfrLen,		/* dest len */
    pUINT32 pActLen		/* actual length copied */
    );


VOID usbDescrStrCopy
    (
    pUINT8 pBfr,		/* destination buffer */
    char *pStr, 		/* source buffer */
    UINT16 bfrLen,		/* dest len */
    pUINT16 pActLen		/* actual length copied */
    );


VOID usbDescrStrCopy32
    (
    pUINT8 pBfr,		/* destination buffer */
    char *pStr, 		/* source buffer */
    UINT32 bfrLen,		/* dest len */
    pUINT32 pActLen		/* actual length copied */
    );


#ifdef	__cplusplus
}
#endif

#endif	/* __INCusbDescrCopyLibh*/

/* End of file. */

