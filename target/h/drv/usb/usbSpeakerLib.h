/* usbSpeakerLib.h - Definitions for USB speaker class driver */

/* Copyright 2000 Wind River Systems, Inc. */

/*
Modification history
--------------------
01e,18sep01,wef  merge from wrs.tor2_0.usb1_1-f for veloce
01d,07may01,wef	 changed module number to be (module num << 8) | M_usbHostLib
01c,02may01,wef  changed module number to be M_<module> + M_usbHostLib
01b,05dec00,wef  moved Module number defs to vwModNum.h - add this
                 to #includes
01a,12jan00,rcb  First.
*/

#ifndef __INCusbSpeakerLibh
#define __INCusbSpeakerLibh

#ifdef	__cplusplus
extern "C" {
#endif


/* includes */

#include "seqIo.h"
#include "usb/usbPlatform.h"
#include "vwModNum.h"           /* USB Module Number Def's */


/* defines */

/* usbSpeakerLib error values */

/* 
 * USB errnos are defined as being part of the USB host Module, as are all
 * vxWorks module numbers, but the USB Module number is further divided into 
 * sub-modules.  Each sub-module has upto 255 values for its own error codes
 */
 
#define USB_SPEAKER_SUB_MODULE 	9 

#define M_usbSpeakerLib ( (USB_SPEAKER_SUB_MODULE << 8) | M_usbHostLib )

#define usbSpkrErr(x)	(M_usbSpeakerLib | (x))

#define S_usbSpeakerLib_NOT_INITIALIZED     usbSpkrErr (1)
#define S_usbSpeakerLib_BAD_PARAM	    usbSpkrErr (2)
#define S_usbSpeakerLib_OUT_OF_MEMORY	    usbSpkrErr (3)
#define S_usbSpeakerLib_OUT_OF_RESOURCES    usbSpkrErr (4)
#define S_usbSpeakerLib_GENERAL_FAULT	    usbSpkrErr (5)
#define S_usbSpeakerLib_NOT_IMPLEMENTED     usbSpkrErr (6)
#define S_usbSpeakerLib_USBD_FAULT	    usbSpkrErr (7)
#define S_usbSpeakerLib_NOT_REGISTERED	    usbSpkrErr (8)
#define S_usbSpeakerLib_NOT_LOCKED	    usbSpkrErr (9)


/* USB_SPKR_xxxx define "attach codes" used by USB_SPKR_ATTACH_CALLBACK. */

#define USB_SPKR_ATTACH 0	    /* new speaker attached */
#define USB_SPKR_REMOVE 1	    /* speaker has been removed */
				    /* SEQ_DEV no longer valid */


/* IOCTL codes unique to usbSpeakerLib. */

#define USB_SPKR_IOCTL_GET_FORMAT_COUNT     0x8000
#define USB_SPKR_IOCTL_GET_FORMAT_LIST	    0x8001

#define USB_SPKR_IOCTL_GET_CHANNEL_COUNT    0x8010
#define USB_SPKR_IOCTL_GET_CHANNEL_CONFIG   0x8011
#define USB_SPKR_IOCTL_GET_CHANNEL_CAPS     0x8012

#define USB_SPKR_IOCTL_SET_AUDIO_FORMAT     0x8020
#define USB_SPKR_IOCTL_OPEN_AUDIO_STREAM    0x8021
#define USB_SPKR_IOCTL_CLOSE_AUDIO_STREAM   0x8022
#define USB_SPKR_IOCTL_AUDIO_STREAM_STATUS  0x8023

#define USB_SPKR_IOCTL_SET_MUTE 	    0x8030
#define USB_SPKR_IOCTL_SET_VOLUME	    0x8031
#define USB_SPKR_IOCTL_SET_BASS 	    0x8032
#define USB_SPKR_IOCTL_SET_MID		    0x8033
#define USB_SPKR_IOCTL_SET_TREBLE	    0x8034


/* Status bit mask returned by USB_SPKR_IOCTL_AUDIO_STREAM_STATUS. */

#define USB_SPKR_STATUS_OPEN		    0x0001
#define USB_SPKR_STATUS_DATA_IN_BFR	    0x0002


/* typedefs */

/* USB_SPKR_ATTACH_CALLBACK defines a callback routine which will be
 * invoked by usbSpeakerLib.c when the attachment or removal of a speaker
 * is detected.  When the callback is invoked with an attach code of
 * USB_SPKR_ATTACH, the pSeqDev points to a newly created SEQ_DEV.  When
 * the attach code is USB_SPKR_REMOVE, the pSeqDev points to a pSeqDev
 * for a speaker which is no longer attached.
 */

typedef VOID (*USB_SPKR_ATTACH_CALLBACK) 
    (
    pVOID arg,			    /* caller-defined argument */
    SEQ_DEV *pSeqDev,		    /* pointer to affected SEQ_DEV */
    UINT16 attachCode		    /* defined as USB_SPKR_xxxx */
    );


/* USB_SPKR_CTL_CAPS defines the capabilities of a given speaker control */

typedef struct usb_spkr_ctl_caps
    {
    BOOL supported;		    /* true if capability supported */
    UINT16 res; 		    /* resolution of capability */
    INT16 min;			    /* minimum setting */
    INT16 max;			    /* maximum setting */
    INT16 cur;			    /* current setting */
    } USB_SPKR_CTL_CAPS, *pUSB_SPKR_CTL_CAPS;


/* USB_SPKR_CHANNEL_CAPS defines the audio capabilities for a given channel. */

typedef struct usb_spkr_channel_caps
    {
    UINT16 capsLen;		    /* length of this structure */

    USB_SPKR_CTL_CAPS mute;	    /* mute */
    USB_SPKR_CTL_CAPS volume;	    /* volume */
    USB_SPKR_CTL_CAPS bass;	    /* bass */
    USB_SPKR_CTL_CAPS mid;	    /* mid-range */
    USB_SPKR_CTL_CAPS treble;	    /* treble */

    } USB_SPKR_CHANNEL_CAPS, *pUSB_SPKR_CHANNEL_CAPS;


/* USB_SPKR_AUDIO_FORMAT defines an audio format supported by the speaker. */

typedef struct usb_spkr_audio_format
    {
    UINT8 interface;		    /* interface number */
    UINT8 altSetting;		    /* alternate setting for this fmt */
    UINT8 delay;		    /* internal delay on this endpoint */
    UINT8 endpoint;		    /* endpoint to receive data */
    UINT16 maxPacketSize;	    /* max packet size for endpoint */
    UINT16 formatTag;		    /* format tag */
    UINT8 formatType;		    /* type I, II, or III. */

				    /* fields for Type I & III formats */
    UINT8 channels;		    /* number of channels */
    UINT8 subFrameSize; 	    /* size of audio sub frame */
    UINT8 bitRes;		    /* bit resolution per sample */
    UINT32 sampleFrequency;	    /* frequency specified by caller */

				    /* Fields for Type II formats */
    UINT16 maxBitRate;		    /* max bps supported by interface */
    UINT16 samplesPerFrame;	    /* nbr of audio samples per frame */

    } USB_SPKR_AUDIO_FORMAT, *pUSB_SPKR_AUDIO_FORMAT;


/* function prototypes */

STATUS usbSpeakerDevInit (void);
STATUS usbSpeakerDevShutdown (void);

STATUS usbSpeakerDynamicAttachRegister
    (
    USB_SPKR_ATTACH_CALLBACK callback,	/* new callback to be registered */
    pVOID arg				/* user-defined arg to callback */
    );

STATUS usbSpeakerDynamicAttachUnRegister
    (
    USB_SPKR_ATTACH_CALLBACK callback,	/* callback to be unregistered */
    pVOID arg				/* user-defined arg to callback */
    );

STATUS usbSpeakerSeqDevLock
    (
    SEQ_DEV *pSeqDev			/* SEQ_DEV to be marked as in use */
    );

STATUS usbSpeakerSeqDevUnlock
    (
    SEQ_DEV *pSeqDev			/* SEQ_DEV to be marked as unused */
    );


#ifdef	__cplusplus
}
#endif

#endif	/* __INCusbSpeakerLibh */


/* End of file. */

