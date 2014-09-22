/* usbCbiUfiDevLib.h - USB CBI Mass Storage class driver for UFI sub-class */ 

/* Copyright 1989-2000 Wind River Systems, Inc. */

/*
modification history
--------------------
01f,07may01,wef  changed module number to be (module num << 8) | M_usbHostLib
01e,02may01,wef  changed module number to be M_<module> + M_usbHostLib
01d,05dec00,wef  moved Module number defs to vwModNum.h - add this
                 to #includes, removed command status codes that lived
		 here and in usbBulkDevLib.h and put into own file in
		 h/usb/usbMassStorage.h 
01c,02sep00,bri  added support for multiple devices.
01b,04aug00,bri  updated as per review.
01a,26jun00,bri  created.
*/

#ifndef __INCusbCbiUfiDevLibh
#define __INCusbCbiUfiDevLibh


#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE

/* includes */

#include "vwModNum.h"           /* USB Module Number Def's */
#include "usb/usbMassStorage.h"	/* Command status codes */

/* Module number and error code definitions */

/* 
 * USB errnos are defined as being part of the USB host Module, as are all
 * vxWorks module numbers, but the USB Module number is further divided into 
 * sub-modules.  Each sub-module has upto 255 values for its own error codes
 */
 
#define USB_CBI_SUB_MODULE  	12

#define M_usbCbiUfiLib 		( (USB_CBI_SUB_MODULE  << 8) | M_usbHostLib )

#define usbCbiUfiErr(x)	  	(M_usbCbiUfiLib | (x))

#define S_usbCbiUfiDevLib_OUT_OF_RESOURCES  usbCbiUfiErr (1)
#define S_usbCbiUfiDevLib_USBD_FAULT	    usbCbiUfiErr (2)
#define S_usbCbiUfiDevLib_NOT_INITIALIZED   usbCbiUfiErr (3)
#define S_usbCbiUfiDevLib_BAD_PARAM	    usbCbiUfiErr (4)
#define S_usbCbiUfiDevLib_OUT_OF_MEMORY	    usbCbiUfiErr (5)
#define S_usbCbiUfiDevLib_NOT_REGISTERED    usbCbiUfiErr (6)
#define S_usbCBiUfiDevLib_NOT_LOCKED        usbCbiUfiErr (7)

#define USB_CLASS_MASS_STORAGE           0x08 /* USB Mass storage class code */
#define USB_SUBCLASS_UFI_COMMAND_SET     0x04 /* UFI command set definition  */
#define USB_INTERFACE_PROTOCOL_CBI       0x00 /* CBI interface protocol      */

#define USB_UFI_MAX_CMD_LEN              0x0C /* UFI Maximum command length  */  

/* UFI Commands */

#define USB_UFI_FORMAT_UNIT              0x04 /* UFI Format Unit command   */
#define USB_UFI_INQUIRY                  0x12 /* UFI Inquiry command       */
#define USB_UFI_MODE_SELECT              0x55 /* UFI Mode Select command   */
#define USB_UFI_MODE_SENSE               0x5A /* UFI Mode Sense command    */
#define USB_UFI_PREVENT_MEDIA_REMOVAL    0x1E /* UFI Prevent/ Allow media  */ 
                                              /* removal command           */
#define USB_UFI_READ10                   0x28 /* UFI 10-byte Read comamnd  */
#define USB_UFI_READ12                   0xA8 /* UFI 12-byte Read command  */
#define USB_UFI_READ_CAPACITY            0x25 /* UFI Read Capacity command */ 
#define USB_UFI_READ_FORMAT_CAPACITY     0x23 /* UFI Read Format capacities*/
#define USB_UFI_REQUEST_SENSE            0x03 /* UFI Request sense command */ 
#define USB_UFI_REZERO                   0x01 /* UFI Rezero command        */
#define USB_UFI_SEEK10                   0x2B /* UFI 10-byte Seek command  */
#define USB_UFI_SEND_DIAGNOSTIC          0x1D /* UFI Send Diagnostic cmd   */
#define USB_UFI_START_STOP_UNIT          0x1B /* UFI Start Stop unit cmd   */
#define USB_UFI_TEST_UNIT_READY          0x00 /* UFI Test Unit Ready cmd   */
#define USB_UFI_VERIFY                   0x2F /* UFI Verify command        */
#define USB_UFI_WRITE10                  0x2A /* UFI 10-byte Write command */
#define USB_UFI_WRITE12                  0xAA /* UFI 12-byte Write command */
#define USB_UFI_WRITE_AND_VERIFY         0x2E /* UFI Write and Verify cmd  */


/* Bit Definitions with in UFI Command blocks */

#define USB_UFI_FORMAT_FMTDATA           0x10 /* Format Data bit       */
#define USB_UFI_FORMAT_FMT_DEFECT        0x07 /* Defect List format    */    
#define USB_UFI_MODE_SEL_PF              0x10 /* Page Format bit       */  
#define USB_UFI_MEDIA_REMOVAL_BIT        0x01 /* Prevent media removal */ 
#define USB_UFI_INQUIRY_RMB_BIT          0x80 /* Removable bit         */ 
#define USB_UFI_START_STOP_LOEJ          0x02 /* Media load eject bit  */
#define USB_UFI_WRITE_PROTECT_BIT        0x80 /* Write protect bit     */

/* Ioctl Control codes */

#define USB_UFI_ALL_DESCRIPTOR_GET       0xF0 /* Shows all descriptors */   
#define USB_UFI_DEV_RESET                0xF1 /* Performs command reset*/ 

/* Miscellaneous Definitions */

#define UFI_STD_REQ_SENSE_LEN            0x12 /* Request sense data length  */ 
#define UFI_STD_INQUIRY_LEN              0x24 /* Length of Inquiry data     */
#define USB_UFI_MS_HEADER_LEN            0x08 /* Mode sense Header Length   */

#define USB_UFI_DIR_IN                   0x80 /* Direction bit -> to host   */
#define USB_UFI_DIR_OUT                  0x00 /* Direction bit -> to device */ 

#define USB_UFI_SENSE_KEY_OFFSET        0x02 /* Sense key offset in sense   */
#define USB_UFI_SENSE_ASC               0x0C /* Add'tl sense code offset    */
#define USB_UFI_SENSE_ASCQ              0x0D /* Add'tl sense code qualifier */ 
#define USB_UFI_SENSE_KEY_MASK          0x0F /* Mask for sense key          */
#define USB_UFI_KEY_UNIT_ATTN           0x06 /* Unit Attn value for key     */
#define USB_UFI_KEY_NOT_READY           0x02 /* Not ready value for key     */
#define USB_UFI_ASC_NO_MEDIA            0x3A /* Media not present code      */ 
#define USB_UFI_ASC_MEDIA_CHANGE        0x28 /* Media change code  */

/* Sense key values */

#define USB_UFI_NO_SENSE                0x00 /* No Sense data                */
#define USB_UFI_RECOVERED_ERROR         0x01 /* Recovered from error         */
#define USB_UFI_NOT_READY               0x02 /* Device is not ready          */
#define USB_UFI_MEDIUM_ERROR            0x03 /* Flaw in the medium or data   */
#define USB_UFI_HARDWARE_ERROR          0x04 /* Non-recoverable h/w error    */
#define USB_UFI_ILL_REQUEST             0x05 /* Illegal parameter in command */
#define USB_UFI_UNIT_ATTN               0x06 /* Media change or reset        */
#define USB_UFI_DATA_PROTECT            0x07 /* Media write protect          */

/* ASC and ASCQ combinations */

#define USB_UFI_WRITE_PROTECT           0x2700 /* Write protected media */  
#define USB_UFI_MEDIA_CHANGE            0x2800 /* Media change          */
#define USB_UFI_POWER_ON_RESET          0x2900 /* Power on reset        */
#define USB_UFI_COMMAND_SUCCESS         0x0000 /* No sense data-success */         


#define USB_CBI_IRP_TIME_OUT            5000 

/* Attach codes used by USB_UFI_ATTACH_CALLBACK. */

#define USB_UFI_ATTACH	                0      /* CBI_UFI Device attached */
#define USB_UFI_REMOVE	                1      /* CBI_UFI Device removed  */

/* Swap macros */

#if (_BYTE_ORDER == _BIG_ENDIAN)

#define USB_SWAP_32   
#define USB_SWAP_16

#else   /* _BYTE_ORDER == _BIG_ENDIAN   */

#define USB_SWAP_32(x)  LONGSWAP((UINT)x)
#define USB_SWAP_16(x)  ((LSB(x) << 8)|MSB(x))

#endif  /* _BYTE_ORDER == _BIG_ENDIAN   */

/* typedefs */



typedef struct usbUfiCmdBlock
    {
    UINT   dataXferLen;                  /* data transfer length       */ 
    UINT   direction;                    /* direction of data transfer */
    UINT8  cmd [USB_UFI_MAX_CMD_LEN];    /* UFI Command block          */ 
    } USB_UFI_CMD_BLOCK, *pUSB_UFI_CMD_BLOCK;
  
/* USB_UFI_ATTACH_CALLBACK defines a callback routine which will be
 * invoked by usbCbiUfiDevLib.c when the attachment or removal of a CBI_UFI
 * device is detected.  When the callback is invoked with an attach code of
 * USB_UFI_ATTACH, the nodeId represents the ID of newly added device.  When
 * the attach code is USB_UFI_REMOVE, nodeId points to the CBI_UFI device which 
 * is no longer attached.
 */

typedef VOID (*USB_UFI_ATTACH_CALLBACK) 
    (
    pVOID arg,           /* caller-defined argument     */
    USBD_NODE_ID nodeId, /* nodeId of the UFI device    */
    UINT16 attachCode    /* attach or remove code       */
    );

/* function prototypes */

STATUS usbCbiUfiDevInit (void);
STATUS usbCbiUfiDevShutDown (int errCode);
STATUS usbCbiUfiDynamicAttachRegister ( USB_UFI_ATTACH_CALLBACK callback, 
                                        pVOID arg);
BLK_DEV *usbCbiUfiBlkDevCreate (USBD_NODE_ID nodeId);

STATUS usbCbiUfiDevIoctl (BLK_DEV * pBlkDev, UINT request, UINT someArg); 

STATUS usbCbiUfiDynamicAttachUnregister ( USB_UFI_ATTACH_CALLBACK callback,
                                          pVOID arg);
STATUS usbCbiUfiDevLock (USBD_NODE_ID nodeId);
STATUS usbCbiUfiDevUnlock (USBD_NODE_ID nodeId);

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus

}

#endif /* __cplusplus */

#endif /* __INCusbCbiUfiDevLibh */
