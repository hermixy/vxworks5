/* usbDescrCopyLib.c - USB descriptor copy utility functions */

/* Copyright 2000 Wind River Systems, Inc. */

/*
Modification history
--------------------
01c,18sep01,wef  merge from wrs.tor2_0.usb1_1-f for veloce
01b,25jul01,wef	fixed spr 69287
01a,05apr00,wef	First -removed these functions from usbLib.c to decouple host
	    	peripheral stack interdependency.
*/

/*
DESCRIPTION

This modules contains miscellaneous functions which may be used by the
USB driver (USBD), USB HCD (USB Host Controller Driver), USB HCD (USB Target
Controller Driver) or by USBD clients.
*/


/* includes */

#include "usb/usbPlatform.h"
#include "usb/usb.h"		    /* Basic USB definitions */

#include "string.h"

#include "usb/usbDescrCopyLib.h"


/* functions */


/***************************************************************************
*
* usbDescrCopy32 - copies descriptor to a buffer
*
* This function is the same as usbDescrCopy() except that <bfrLen> and
* <pActLen> refer to UINT32 quantities.
*
* RETURNS: N/A
*/

VOID usbDescrCopy32
    (
    pUINT8 pBfr,		    /* destination buffer */
    pVOID pDescr,		    /* source buffer */
    UINT32 bfrLen,		    /* dest len */
    pUINT32 pActLen		    /* actual length copied */
    )

    {
    pUSB_DESCR_HDR pHdr = (pUSB_DESCR_HDR) pDescr;

    bfrLen = min (bfrLen, pHdr->length);
    memcpy ((char *) pBfr, (char *) pDescr, (int) bfrLen);

    if (pActLen != NULL)
	*pActLen = bfrLen;
    }



/***************************************************************************
*
* usbDescrCopy - copies descriptor to a buffer
*
* Copies the USB descriptor at <pDescr> to the <pBfr> of length <bfrLen>.
* Returns the actual number of bytes copied - which is the shorter of 
* the <pDescr> or <bfrLen> - in <pActLen> if <pActLen> is non-NULL.
*
* RETURNS: N/A
*/

VOID usbDescrCopy
    (
    pUINT8 pBfr,		    /* destination buffer */
    pVOID pDescr,		    /* source buffer */
    UINT16 bfrLen,		    /* dest len */
    pUINT16 pActLen		    /* actual length copied */
    )

    {
    UINT32 actLen;

    usbDescrCopy32 (pBfr, pDescr, (UINT32) bfrLen, &actLen);

    if (pActLen != NULL)
	*pActLen = (UINT16) actLen;
    }



/***************************************************************************
*
* usbDescrStrCopy32 - copies an ASCII string to a string descriptor
*
* This function is the same as usbDescrStrCopy() except that <bfrLen> and
* <pActLen> refer to UINT32 quantities.
*
* RETURNS: N/A
*/

VOID usbDescrStrCopy32
    (
    pUINT8 pBfr,		    /* destination buffer */
    char *pStr, 		    /* source buffer */
    UINT32 bfrLen,		    /* dest len */
    pUINT32 pActLen		    /* actual length copied */
    )

    {
    UINT8 bfr [USB_MAX_DESCR_LEN];
    pUSB_STRING_DESCR pString = (pUSB_STRING_DESCR) bfr;
    UINT32 i;
    
    pString->length = USB_DESCR_HDR_LEN + strlen (pStr) * 2;
    pString->descriptorType = USB_DESCR_STRING;

    for (i = 0; i < strlen (pStr); i++)
	{
	pString->string [i*2] = pStr [i];
	pString->string [i*2 + 1] = 0;
	}

    usbDescrCopy32 (pBfr, pString, bfrLen, pActLen);
    }

/***************************************************************************
*
* usbDescrStrCopy - copies an ASCII string to a string descriptor
*
* This function constructs a properly formatted USB string descriptor
* in <pBfr>.  The ASCII string <pStr> is copied to <pBfr> as a UNICODE
* string - as required by the USB spec.  The actual length of the 
* resulting descriptor is returned in <pActLen> if <pActLen> is non-NULL.
*
* NOTE: The complete length of the string descriptor can be calculated
* as 2 * strlen (pStr) + 2.  The <pActLen> will be the shorter of <bfrLen>
* or this value.
*
* RETURNS: N/A
*/

VOID usbDescrStrCopy
    (
    pUINT8 pBfr,		    /* destination buffer */
    char *pStr, 		    /* source buffer */
    UINT16 bfrLen,		    /* dest len */
    pUINT16 pActLen		    /* actual length copied */
    )

    {
    UINT32 actLen;

    usbDescrStrCopy32 (pBfr, pStr, (UINT32) bfrLen, &actLen);

    if (pActLen != NULL)
	*pActLen = (UINT16) actLen;
    }



/* End of file. */

