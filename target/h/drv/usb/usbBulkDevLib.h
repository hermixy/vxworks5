/* usbBulkDevLib.h - USB Bulk only Mass Storage class header file */

/* Copyright 1999-2000 Wind River Systems, Inc. */

/*
modification history
--------------------
01g,07may01,wef changed module number to be (module num << 8) | M_usbHostLib
01f,02may01,wef changed module number to be M_<module> + M_usbHostLib
01e,30mar01,wef added READ10/WRITE10 command
01d,05dec00,wef moved Module number defs to vwModNum.h - add this
                to #includes, removed command status codes that lived
                here and in usbCbiUfiDevLib.h and put into own file in
                h/usb/usbMassStorage.h
01c,02sep00,bri added support for multiple devices.
01b,04aug00,bri updated as per review. 
01a,22may00,bri created.
*/

#ifndef __INCusbBulkDevLibh
#define __INCusbBulkDevLibh


#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE

/* includes */

#include "vwModNum.h"           /* USB Module Number Def's */
#include "usb/usbMassStorage.h"     /* Command Status codes */

/* Module number and error code definitions */


/* 
 * USB errnos are defined as being part of the USB host Module, as are all
 * vxWorks module numbers, but the USB Module number is further divided into 
 * sub-modules.  Each sub-module has upto 255 values for its own error codes
 */
 
#define USB_BULK_SUB_MODULE  11

#define M_usbBulkLib 	( (USB_BULK_SUB_MODULE  << 8) | M_usbHostLib )

#define usbBulkErr(x)   (M_usbBulkLib | (x))

#define S_usbBulkDevLib_NOT_INITIALIZED     usbBulkErr (1)
#define S_usbBulkDevLib_BAD_PARAM           usbBulkErr (2)
#define S_usbBulkDevLib_OUT_OF_MEMORY       usbBulkErr (3)
#define S_usbBulkDevLib_OUT_OF_RESOURCES    usbBulkErr (4)
#define S_usbBulkDevLib_GENERAL_FAULT       usbBulkErr (5)
#define S_usbBulkDevLib_QUEUE_FULL          usbBulkErr (6)
#define S_usbBulkDevLib_QUEUE_EMPTY         usbBulkErr (7)
#define S_usbBulkDevLib_NOT_IMPLEMENTED     usbBulkErr (8)
#define S_usbBulkDevLib_USBD_FAULT          usbBulkErr (9)
#define S_usbBulkDevLib_NOT_REGISTERED      usbBulkErr (10)
#define S_usbBulkDevLib_NOT_LOCKED          usbBulkErr (11)

#define USB_CLASS_MASS_STORAGE           0x08 /* USB Mass storage class code */
#define USB_SUBCLASS_SCSI_COMMAND_SET    0x06 /* SCSI command set definition */
#define USB_INTERFACE_PROTOCOL_BULK_ONLY 0x50 /* BULK only interface protocol*/

/* 
 * usbBulkDevIoctl function codes - chosen outside existing IO control 
 * codes supported by file systems
 */

#define USB_BULK_DESCRIPTOR_GET          0xF0 /* Shows all descriptors  */ 
#define USB_BULK_DEV_RESET               0xF1 /* Class-specific reset   */
#define USB_BULK_MAX_LUN                 0xF2 /* Max. LUN on the device */
#define USB_BULK_EJECT_MEDIA             0xF3 /* Eject media command    */


/* Bulk only protocol constants */

#define USB_CBW_SIGNATURE                0x43425355   /* Command block ID  */
#define USB_CBW_TAG                      0xA5A5A5A5   /* Command block Tag */
#define USB_CSW_SIGNATURE                0x53425355   /* Status block ID   */

#define USB_CBW_DIR_NONE                 0x00 /* No direction => no data xfer */  
#define USB_CBW_DIR_OUT                  0x00 /* Direction OUT - to device    */
#define USB_CBW_DIR_IN                   0x80 /* Direction IN  - from device  */
#define USB_CBW_LUN_MASK                 0x0F /* Mask for LUN field in CBW    */
#define USB_CBW_CBLEN_MASK               0x1F /* Mask for Command length field*/
#define USB_CBW_LENGTH                   0x1F /* Length of CBW                */ 
#define USB_CBW_MAX_CBLEN                0x10 /* Max. length of command block */
#define USB_CSW_LENGTH                   0x0D /* Length of CSW                */

/* Status code in CSW */

#define USB_CSW_STATUS_PASS              0x00 /* CBW Command success */ 
#define USB_CSW_STATUS_FAIL              0x01 /* CBW Command failed  */
#define USB_CSW_PHASE_ERROR              0x02 /* Phase Error         */

/* IRP Time out in millisecs */

#define USB_BULK_IRP_TIME_OUT            5000

/* SCSI Commands and related constants */

#define USB_SCSI_WRITE6                  0x0A /* 6-byte WRITE command     */ 
#define USB_SCSI_WRITE10                 0x2A /* 6-byte WRITE command     */ 
#define USB_SCSI_READ6	                 0x08 /* 6-byte READ command      */ 
#define USB_SCSI_READ10	                 0x28 /* 10-byte READ command      */ 
#define USB_SCSI_INQUIRY                 0x12 /* Standard INQUIRY command */
#define USB_SCSI_START_STOP_UNIT         0x1B /* Start Stop Unit command  */
#define USB_SCSI_REQ_SENSE               0x03 /* REQUEST SENSE data       */ 
#define USB_SCSI_TEST_UNIT_READY         0x00 /* TEST UNIT READY command  */
#define USB_SCSI_READ_CAPACITY           0x25 /* READ CAPACITY command    */
#define USB_SCSI_PREVENT_REMOVAL         0x1E /* Prevent media removal    */
#define USB_SCSI_FORMAT_UNIT             0x04 /* FORMAT UNIT Command      */  

/* specific bit definitions in SCSI commands */

#define USB_SCSI_STD_INQUIRY_LEN         0x24 /* Length of std INQUIRY data */
#define USB_SCSI_REQ_SENSE_LEN           0x0E /* Length of Req Sense data   */
#define USB_SCSI_READ_CAP_LEN            0x08 /* Length of RD_CAP response  */
#define USB_SCSI_INQUIRY_RMB_BIT         0x80 /* Media Type bit             */
#define USB_SCSI_START_STOP_LOEJ         0x02 /* Media load eject bit       */
#define USB_SCSI_START_STOP_START        0x01 /* Media start bit            */

/* definitions with in request sense data */

#define USB_SCSI_SENSE_KEY_OFFSET        0x02 /* Sense key offset in sense   */
#define USB_SCSI_SENSE_ASC               0x0C /* Add'tl sense code offset    */
#define USB_SCSI_SENSE_ASCQ              0x0D /* Add'tl sense code qualifier */ 
#define USB_SCSI_SENSE_CUR_ERR           0x70 /* code for Current Errors     */ 
#define USB_SCSI_SENSE_KEY_MASK          0x0F /* Mask for sense key          */
#define USB_SCSI_KEY_NO_SENSE            0x00 /* No specific sense key       */
#define USB_SCSI_KEY_NOT_READY           0x02 /* Not ready value for key     */
#define USB_SCSI_KEY_UNIT_ATTN           0x06 /* Unit Attn value for key     */
#define USB_SCSI_KEY_HW_ERROR            0x04 /* Hardware Err value for key  */ 
#define USB_SCSI_ASC_NO_MEDIA            0x3A /* Media not present code      */ 
#define USB_SCSI_ASC_RESET               0x29 /* Reset or media change code  */

/* device create flag bits */

/* 
 * The fourth parameter to usbBulkBlkDevCreate is an int inteded to be a bit 
 * field.  The first bit is used to determine which type of SCSI read / write
 * command is used.  All other bits are undefined and available for furture use.
 */

#define USB_SCSI_FLAG_READ_WRITE10	0x00000001	/* READ/WRITE10 */
#define USB_SCSI_FLAG_READ_WRITE6	0x00000000	/* READ/WRITE6 */


/* Class specific commands */

#define USB_BULK_RESET                   0xFF /* Mass storage reset command  */ 
#define USB_BULK_GET_MAX_LUN             0xFE /* Acquire Max. LUN command    */

/* Attach codes used by USB_BULK_ATTACH_CALLBACK. */

#define USB_BULK_ATTACH	                0      /* Bulk-only Device attached  */
#define USB_BULK_REMOVE	                1      /* Bulk-only Device removed   */


/* Swap macros */

/* 
 * SCSI Response is in BIG ENDIAN format. Needs swapping on LITTLE ENDIAN 
 * platforms.
 */

#if (_BYTE_ORDER == _BIG_ENDIAN)

#define USB_SCSI_SWAP_32
#define USB_SCSI_SWAP_16

#else

#define USB_SCSI_SWAP_32(x)  LONGSWAP((UINT)x)
#define USB_SCSI_SWAP_16(x)  ((LSB(x) << 8)|MSB(x))

#endif

/* 
 * Command blocks for Bulk-only devices are in LITTLE ENDIAN format. Need 
 * swapping on BIG ENDIAN platforms.
 */

#if (_BYTE_ORDER == _BIG_ENDIAN)

#define USB_BULK_SWAP_32(x)  LONGSWAP((UINT)x)
#define USB_BULK_SWAP_16(x)  ((LSB(x) << 8)|MSB(x))

#else   /* _BYTE_ORDER == _BIG_ENDIAN   */

#define USB_BULK_SWAP_32   
#define USB_BULK_SWAP_16

#endif  /* _BYTE_ORDER == _BIG_ENDIAN   */

/* command block wrapper */

typedef struct usbBulkCbw
    {
    UINT32	signature;              /* CBW Signature */
    UINT32	tag;                    /* Tag field     */
    UINT32	dataXferLength;         /* Size of data (bytes) */
    UINT8	direction;              /* direction bit */
    UINT8	lun;                    /* Logical Unit Number */
    UINT8	length;                 /* Length of command block */
    UINT8	CBD [USB_CBW_MAX_CBLEN];/* buffer for command block */
    } USB_BULK_CBW, *pUSB_BULK_CBW;


typedef struct usbBulkCsw
    {
    UINT32	signature;              /* CBW Signature */
    UINT32	tag;                    /* Tag field  */
    UINT32	dataResidue;            /* Size of residual data(bytes) */
    UINT8	status;                 /* buffer for command block */
    } USB_BULK_CSW, *pUSB_BULK_CSW;

/* USB_BULK_ATTACH_CALLBACK defines a callback routine which will be
 * invoked by usbBulkDevLib.c when the attachment or removal of a MSC/SCSI/
 * BULK-ONLY device is detected.  When the callback is invoked with an attach 
 * code of USB_BULK_ATTACH, the nodeId represents the ID of newly added device.
 * When the attach code is USB_BULK_REMOVE, nodeId points to the Bulk-only device 
 * which is no longer attached.
 */

typedef VOID (*USB_BULK_ATTACH_CALLBACK) 
    (
    pVOID arg,           /* caller-defined argument           */
    USBD_NODE_ID nodeId, /* nodeId of the bulk-only device    */
    UINT16 attachCode    /* attach or remove code             */
    );

/* function prototypes */

STATUS usbBulkDevInit (void);
STATUS usbBulkDevShutDown (int errCode);
STATUS usbBulkDynamicAttachRegister ( USB_BULK_ATTACH_CALLBACK callback, 
                                        pVOID arg);
BLK_DEV *usbBulkBlkDevCreate (USBD_NODE_ID nodeId, UINT32 numBlks, 
                              UINT32 blkOffset, UINT32 flags);

STATUS usbBulkDevIoctl (BLK_DEV * pBlkDev, int request, int someArg); 

STATUS usbBulkDynamicAttachUnregister ( USB_BULK_ATTACH_CALLBACK callback,
                                        pVOID arg);
STATUS usbBulkDevLock (USBD_NODE_ID nodeId);
STATUS usbBulkDevUnlock (USBD_NODE_ID nodeId);

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus

}

#endif /* __cplusplus */

#endif /* __INCusbBulkDevLibh */
