/* usbAcmLib.h - USB Communications / ACM Class Class Driver definitions */

/* Copyright 2000-2001 Wind River Systems, Inc. */

/*
Modification history
--------------------
01d,07may01,wef  changed module number to be (module num << 8) | M_usbHostLib
01c,02may01,wef  changed module number to be M_<module> + M_usbHostLib
01b,05dec00,wef  moved Module number defs to vwModNum.h - add vwModNum.h 
                 to #includes
01a,19sep00,bri  Created
*/

#ifndef __INCusbAcmLibh
#define __INCusbAcmLibh

#ifdef	__cplusplus
extern "C" {
#endif  /* __cplusplus */


/* includes */

#include "vxWorks.h"
#include "string.h"
#include "sioLib.h"
#include "errno.h"
#include "ctype.h"
#include "logLib.h"

#include "usb/usbPlatform.h"	    
#include "usb/ossLib.h"		    /* operations system srvcs */
#include "usb/usb.h"		    /* general USB definitions */
#include "usb/usbListLib.h"	    /* linked list functions */
#include "usb/usbdLib.h"	    /* USBD interface */
#include "usb/usbLib.h"		    /* USB utility functions */

#include "usb/usbCommdevices.h"

#include "vwModNum.h"           /* USB Module Number Def's */

/* defines */

/* communication feature selection codes */

#define USB_ACM_COMM_FEATURE_RESERVED           0x00
#define USB_ACM_COMM_FEATURE_ABSTRACT_STATE     0x01
#define USB_ACM_COMM_FEATURE_COUNTRY_SETTING    0x02

#define USB_ACM_IDLE_END_POINTS                 0x0001
#define USB_ACM_ENABLE_MULTIPLEX_CALLMGMT       0x0002

/* Line coding */

#define USB_ACM_STOPBITS_1                      0
#define USB_ACM_STOPBITS_1_5                    1
#define USB_ACM_STOPBITS_2                      2

#define USB_ACM_PARITY_NONE                     0
#define USB_ACM_PARITY_ODD                      1
#define USB_ACM_PARITY_EVEN                     2
#define USB_ACM_PARITY_MARK                     3
#define USB_ACM_PARITY_SPACE                    4

/* Control Line State Setting */

#define USB_ACM_HALFDPLX_ENABLE_CARRIER         0x2
#define USB_ACM_DTE_PRESENT                     0x1

/* UART (Serial) State masks */

#define USB_ACM_SSTATE_RX_CARRIER               0x0001
#define USB_ACM_SSTATE_TX_CARRIER               0x0002
#define USB_ACM_SSTATE_BREAK                    0x0004
#define USB_ACM_SSTATE_RING_SIGNAL              0x0008
#define USB_ACM_SSTATE_FRAMING                  0x0010
#define USB_ACM_SSTATE_PARITY                   0x0020
#define USB_ACM_SSTATE_OVERRUN                  0x0040


/* baudrate range supported */

#define USB_ACM_BAUD_MIN                4800
#define USB_ACM_BAUD_MAX                56000

/*
 *  Abstract Control Model Requests and Notifications
 */

#define USB_ACM_REQ_SEND_ENCAP          0x0000
#define USB_ACM_REQ_GET_ENCAP           0x0001
#define USB_ACM_REQ_FEATURE_SET         0x0002
#define USB_ACM_REQ_FEATURE_GET         0x0003
#define USB_ACM_REQ_FEATURE_CLEAR       0x0004
#define USB_ACM_REQ_LINE_CODING_SET	0x0020
#define USB_ACM_REQ_LINE_CODING_GET	0x0021
#define USB_ACM_REQ_CTRL_LINE_STATE_SET	0x0022
#define USB_ACM_REQ_SEND_BREAK	        0x0023

#define USB_ACM_NOTIFY_NET_CONN         0x0000
#define USB_ACM_NOTIFY_RESP_AVAIL       0x0001
#define USB_ACM_NOTIFY_SERIAL_STATE     0x0020

/* Error codes as set by usbAcmLib  */

/* usbEnetLib error values */

/* 
 * USB errnos are defined as being part of the USB host Module, as are all
 * vxWorks module numbers, but the USB Module number is further divided into 
 * sub-modules.  Each sub-module has upto 255 values for its own error codes 
 */

#define USB_ACM_SUB_MODULE  10

#define M_usbAcmLib 	( (USB_ACM_SUB_MODULE  << 8) | M_usbHostLib )

#define usbAcmErr(x)	(M_usbAcmLib | (x))

#define S_usbAcmLib_NOT_INITIALIZED	usbAcmErr (1)
#define S_usbAcmLib_BAD_PARAM	    	usbAcmErr (2)
#define S_usbAcmLib_OUT_OF_MEMORY	usbAcmErr (3)
#define S_usbAcmLib_OUT_OF_RESOURCES	usbAcmErr (4)
#define S_usbAcmLib_GENERAL_FAULT	usbAcmErr (5)
#define S_usbAcmLib_QUEUE_FULL	    	usbAcmErr (6)
#define S_usbAcmLib_QUEUE_EMPTY		usbAcmErr (7)
#define S_usbAcmLib_NOT_IMPLEMENTED	usbAcmErr (8)
#define S_usbAcmLib_USBD_FAULT	    	usbAcmErr (9)
#define S_usbAcmLib_NOT_REGISTERED	usbAcmErr (10)
#define S_usbAcmLib_NOT_LOCKED	    	usbAcmErr (11)

/* Callback Codes */

#define USB_ACM_CALLBACK_ATTACH         (0x1<<8)
#define USB_ACM_CALLBACK_DETACH         (0x1<<9)
#define USB_ACM_CALLBACK_SIO_TX         (0x1<<10)
#define USB_ACM_CALLBACK_SIO_RX         (0x1<<11)
#define USB_ACM_CALLBACK_BLK_RX         (0x1<<12)
#define USB_ACM_CALLBACK_MODEM_CMD      (0x1<<13)

/* We use this Bit to see if the callback is for a Block send */
    
#define USB_ACM_CALLBACK_BLK_TX         0x0100      


/* IOCTL codes */

/* VxWorks SIO Model Ioctl codes */

#define USB_ACM_SIO_BAUD_SET            SIO_BAUD_SET
#define USB_ACM_SIO_BAUD_GET            SIO_BAUD_GET
#define USB_ACM_SIO_MODE_SET            SIO_MODE_SET
#define USB_ACM_SIO_MODE_GET            SIO_MODE_GET
#define USB_ACM_SIO_AVAIL_MODES_GET     SIO_AVAIL_MODES_GET
#define USB_ACM_SIO_HW_OPTIONS_SET      SIO_HW_OPTS_SET
#define USB_ACM_SIO_HW_OPTIONS_GET      SIO_HW_OPTS_GET
#define USB_ACM_SIO_HUP                 SIO_HUP
#define USB_ACM_SIO_OPEN                SIO_OPEN

/* Ioctl codes to support the usb ACM specification.*/

#define acmIoctl(x)                     (0x8000 | x)

#define USB_ACM_SIO_SEND_ENCAP          acmIoctl(USB_ACM_REQ_SEND_ENCAP)
#define USB_ACM_SIO_GET_ENCAP           acmIOctl(USB_ACM_REQ_GET_ENCAP)
#define USB_ACM_SIO_FEATURE_SET         acmIoctl(USB_ACM_REQ_FEATURE_SET)
#define USB_ACM_SIO_FEATURE_GET         acmIoctl(USB_ACM_REQ_FEATURE_GET)
#define USB_ACM_SIO_FEATURE_CLEAR       acmIoctl(USB_ACM_REQ_FEATURE_CLEAR)
#define USB_ACM_SIO_LINE_CODING_SET	acmIoctl(USB_ACM_REQ_LINE_CODING_SET)
#define USB_ACM_SIO_LINE_CODING_GET	acmIoctl(USB_ACM_REQ_LINE_CODING_GET)
#define USB_ACM_SIO_SEND_BREAK	        acmIoctl(USB_ACM_REQ_SEND_BREAK)
#define USB_ACM_SIO_CTRL_LINE_STATE_SET	\
                                acmIoctl(USB_ACM_REQ_CTRL_LINE_STATE_SET)
#define USB_ACM_SIO_MAX_BUF_SIZE_GET    acmIoctl(0x00aa)


/* Buffer Sizes */

#define ACM_OUT_BFR_SIZE       512       /* size of output bfr */
#define ACM_IN_BFR_SIZE        512	 /* size of input bfr */
#define ACM_INTR_BFR_SIZE      512 	 /* size of input bfr */

/* typedefs */

/*
 * usbAcmLib allows client to obtain notification of certain types of events
 * by registering a callback routine for each of the events that are of interest
 * to the client. The attach codes are described in usbAcmLib.h.
 * 
 * The USB_ACM_CALLBACK defines a callback routine which will be invoked
 * by usbAcmLib.c when any of these events happen, provided that the user
 * registered a callback for such an event. 
 * Note that all these fields are not required for all of the events.
 * They will be filled NULL or ZERO (0) when the callback is executed.
 *
 */

typedef STATUS (*USB_ACM_CALLBACK)
    (
    pVOID       arg,	            /* caller-defined argument */
    SIO_CHAN * pChan,		    /* pointer to affected SIO_CHAN */
    UINT16 callbackType,	    /* defined as USB_ACM_CALLBACK_xxxx */
    UINT8 * pBuf,                   /* pointer to data buffer, if any data */
                                    /* transfer is involved. Otherwise NULL */
    UINT16 count                    /* No of bytes of data transferred */
                                    /* if a data transfer is involved. */
                                    /* 0 otherwise. */
    );

typedef struct acmLineCode {
    UINT32  baudRate;               /* Data terminal rate in Bits per sec*/
    UINT8   noOfStopBits;           /* 1, 1.5 or 2 */
    UINT8   parityType;             /* None, Even, Odd, Mark or Space */
    UINT8   noOfDataBits;           /* 5,6,7,8 or 16 */
    }LINE_CODE;


/*
 * The structure we associate each Modem with.
 */


typedef struct acmStruct {

    SIO_CHAN sioChan;		    /* must be first field */

    int unitNo;			    /* modem device */

    LINK sioLink;		    /* linked list of acmStructs */

    UINT16 lockCount;		    /* Count of times structure locked */

    USBD_NODE_ID nodeId;	    /* modem node Id */

    UINT16 vendorId;                /* The information which */
    UINT16 productId;               /* allows to identify an SIO_CHAN */
    UINT16 serialNo;                /* uniquely */

    UINT16 configuration;	    /* configuration reported as a modem */
    UINT16 ifaceCommClass;          /* Communication Interface Class */
    UINT16 ifaceCommAltSetting;     /* Alternate Setting */
    UINT16 ifaceDataClass;          /* Data Interface Class */
    UINT16 ifaceDataAltSetting;     /* Alternate Setting */
    UINT16 protocol;		    /* protocol reported by device */

    BOOL connected;		    /* TRUE if modem currently connected */

    USBD_PIPE_HANDLE outPipeHandle; /* USBD pipe handle for bulk OUT pipe */
    USB_IRP outIrp;		    /* IRP to transmit output data */
    BOOL outIrpInUse;		    /* TRUE while IRP is outstanding */
    UINT8 * outBfr;		    /* pointer to output buffer */
    UINT16 outBfrLen;		    /* maximum size of output buffer */
    UINT32 outErrors;		    /* count of IRP failures */

    USBD_PIPE_HANDLE inPipeHandle;  /* USBD pipe handle for bulk IN pipe */
    USB_IRP inIrp;		    /* IRP to monitor input from printer */
    BOOL inIrpInUse;		    /* TRUE while IRP is outstanding */
    UINT8 * inBfr;		    /* pointer to input buffer */
    UINT16 inBfrLen;		    /* size of input buffer */
    UINT32 inErrors;		    /* count of IRP failures */

    USBD_PIPE_HANDLE intrPipeHandle; /* USBD pipe handle for bulk OUT pipe */
    USB_IRP intrIrp;		    /* IRP to transmit output data */
    BOOL intrIrpInUse;		    /* TRUE while IRP is outstanding */
    UINT8 * intrBfr;		    /* pointer to output buffer */
    UINT16 intrBfrLen;		    /* size of output buffer */
    UINT32 intrErrors;		    /* count of IRP failures */

    struct acmLineCode lineCode;    /* Communication Settings */
    int     options;                /* SIO style options */

    int mode;			    /* always SIO_MODE_INT */
    UINT16 maxPktSize;              /* Maximum size for Block Transfers */

    UINT16 callbackStatus;          /* Tells whether a callback is */
                                    /* installed or Not */

    STATUS (*getTxCharCallback) (); /* tx callback */
    void * getTxCharArg;            /* tx callback argument */

    STATUS (*putRxCharCallback) (); /* rx callback */
    void * putRxCharArg;	    /* rx callback argument */

    USB_ACM_CALLBACK putRxBlockCallback;/* Block Rx callback */
    void * putRxBlockArg;           /* Block Rx Callback Argument */

    USB_ACM_CALLBACK putModemResponseCallback;  /* Modem Response callback */
    void * putModemResponseArg;             /* Modem Response Callback Arg */

    } USB_ACM_SIO_CHAN, *pUSB_ACM_SIO_CHAN;


/* function prototypes */

STATUS usbAcmLibInit 		(void);
STATUS usbAcmLibShutdown 	(void);

STATUS usbAcmSioChanLock
    (
    SIO_CHAN *pChan		    /* SIO_CHAN to be marked as in use */
    );

STATUS usbAcmSioChanUnlock
    (
    SIO_CHAN *pChan		    /* SIO_CHAN to be marked as unused */
    );

int usbAcmCallbackRegister
    (
    SIO_CHAN * pChan,                   /* SIO_CHAN */
    int  callbackType,                  /* Callback Type */
    FUNCPTR callback,	        	/* callback to be registered */
    pVOID arg				/* user-defined arg to callback */
    );

STATUS usbAcmCallbackRemove
    (
    SIO_CHAN * pChan,                   /* Channel */
    UINT callbackType,                  /* Callback type for which callback */
    USB_ACM_CALLBACK callback           /* is to be de-installed */
    );

STATUS usbAcmBlockSend
    (
    SIO_CHAN * pChan,                   /* Channel for Block Transmission */
    UINT8 * pBuf,                       /* data to be transmitted */
    UINT16 count                        /* no of bytes to be transmitted */
    );

STATUS usbAcmModemCommandSend
    (
    SIO_CHAN * pChan,                   /* Channel */
    UINT8 * pBuf,                       /* Pointer to command buffer */
    UINT16 count                        /* no of command bytes */
    );

int usbAcmIoctl
    (
    SIO_CHAN * pChan,                   /* Channel */
    int  request,                       /* IOCTL request */
    void * pBuf                         /* buffer */
    );


#ifdef	__cplusplus
}
#endif	/* __cplusplus */

#endif	/* __INCusbAcmLibh */
