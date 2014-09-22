/* usbAudio.h - Definitions for USB audio class */

/* Copyright 2000 Wind River Systems, Inc. */

/*
Modification history
--------------------
01b,20mar00,rcb  Re-define UINT16 members of structures as arrays of
		 UINT8...UINT16 fields may not be word aligned and that 
		 causes fits for some processor families.
01a,12jan00,rcb  First.
*/

#ifndef __INCusbAudioh
#define __INCusbAudioh

#ifdef	__cplusplus
extern "C" {
#endif


/* defines */

/* USB audio subclass codes */

#define USB_SUBCLASS_AUDIO_NONE 	    0x00
#define USB_SUBCLASS_AUDIO_AUDIOCONTROL     0x01
#define USB_SUBCLASS_AUDIO_AUDIOSTREAMING   0x02
#define USB_SUBCLASS_AUDIO_MIDISTREAMING    0x03


/* USB audio protocol codes */

#define USB_PROTOCOL_AUDIO_NONE 	    0x00


/* USB audio class-specific descriptor types */

#define USB_DESCR_AUDIO_UNDEFINED	    0x20
#define USB_DESCR_AUDIO_DEVICE		    0x21
#define USB_DESCR_AUDIO_CONFIGURATION	    0x22
#define USB_DESCR_AUDIO_STRING		    0x23
#define USB_DESCR_AUDIO_INTERFACE	    0x24
#define USB_DESCR_AUDIO_ENDPOINT	    0x25


/* USB audio class-specific AC interface descriptor subtypes */

#define USB_DESCR_AUDIO_AC_UNDEFINED	    0x00
#define USB_DESCR_AUDIO_AC_HEADER	    0x01
#define USB_DESCR_AUDIO_AC_INPUT_TERMINAL   0x02
#define USB_DESCR_AUDIO_AC_OUTPUT_TERMINAL  0x03
#define USB_DESCR_AUDIO_AC_MIXER_UNIT	    0x04
#define USB_DESCR_AUDIO_AC_SELECTOR_UNIT    0x05
#define USB_DESCR_AUDIO_AC_FEATURE_UNIT     0x06
#define USB_DESCR_AUDIO_AC_PROCESSING_UNIT  0x07
#define USB_DESCR_AUDIO_AC_EXTENSION_UNIT   0x08


/* USB audio class-specific AS interface descriptor subtypes */

#define USB_DESCR_AUDIO_AS_UNDEFINED	    0x00
#define USB_DESCR_AUDIO_AS_GENERAL	    0x01
#define USB_DESCR_AUDIO_AS_FORMAT_TYPE	    0x02
#define USB_DESCR_AUDIO_AS_FORMAT_SPECIFIC  0x03


/* processing unit process types */

#define USB_AUDIO_PROCESS_UNDEFINED	    0x00
#define USB_AUDIO_PROCESS_UP_DOWN_MIX	    0x01
#define USB_AUDIO_PROCESS_DOLBY_PROLOGIC    0x02
#define USB_AUDIO_PROCESS_3D_STEREO_EXT     0x03
#define USB_AUDIO_PROCESS_REVERB	    0x04
#define USB_AUDIO_PROCESS_CHORUS	    0x05
#define USB_AUDIO_PROCESS_DYN_RANGE_COMP    0x06


/* audio class-specific endpoint descriptor subtypes */

#define USB_DESCR_AUDIO_EP_UNDEFINED	    0x00
#define USB_DESCR_AUDIO_EP_GENERAL	    0x01


/* USB requests for audio devices */

#define USB_REQ_AUDIO_UNDEFINED 	    0x00
#define USB_REQ_AUDIO_SET_CUR		    0x01
#define USB_REQ_AUDIO_GET_CUR		    0x81
#define USB_REQ_AUDIO_SET_MIN		    0x02
#define USB_REQ_AUDIO_GET_MIN		    0x82
#define USB_REQ_AUDIO_SET_MAX		    0x03
#define USB_REQ_AUDIO_GET_MAX		    0x83
#define USB_REQ_AUDIO_SET_RES		    0x04
#define USB_REQ_AUDIO_GET_RES		    0x84
#define USB_REQ_AUDIO_SET_MEM		    0x05
#define USB_REQ_AUDIO_GET_MEM		    0x86
#define USB_REQ_AUDIO_GET_STAT		    0xff


/* terminal control selectors */

#define USB_AUDIO_TCS_UNDEFINED 	    0x00
#define USB_AUDIO_TCS_COPY_PROTECT	    0x01


/* feature unit control selectors */

#define USB_AUDIO_FCS_UNDEFINED 	    0x00
#define USB_AUDIO_FCS_MUTE		    0x01
#define USB_AUDIO_FCS_VOLUME		    0x02
#define USB_AUDIO_FCS_BASS		    0x03
#define USB_AUDIO_FCS_MID		    0x04
#define USB_AUDIO_FCS_TREBLE		    0x05
#define USB_AUDIO_FCS_GRAPHIC_EQUALIZER     0x06
#define USB_AUDIO_FCS_AUTOMATIC_GAIN	    0x07
#define USB_AUDIO_FCS_DELAY		    0x08
#define USB_AUDIO_FCS_BASS_BOOST	    0x09
#define USB_AUDIO_FCS_LOUDNESS		    0x0a


/* control bit mask in a feature unit descriptor */

#define USB_AUDIO_FCM_MUTE		    0x0001
#define USB_AUDIO_FCM_VOLUME		    0x0002
#define USB_AUDIO_FCM_BASS		    0x0004
#define USB_AUDIO_FCM_MID		    0x0008
#define USB_AUDIO_FCM_TREBLE		    0x0010
#define USB_AUDIO_FCM_GRAPHIC_EQUALIZER     0x0020
#define USB_AUDIO_FCM_AUTOMATIC_GAIN	    0x0040
#define USB_AUDIO_FCM_DELAY		    0x0080
#define USB_AUDIO_FCM_BASS_BOOST	    0x0100
#define USB_AUDIO_FCM_LOUDNESS		    0x0200


/* Width of values for feature unit controls */

#define USB_AUDIO_MUTE_ATTR_WIDTH	    1
#define USB_AUDIO_VOLUME_ATTR_WIDTH	    2
#define USB_AUDIO_BASS_ATTR_WIDTH	    1
#define USB_AUDIO_MID_ATTR_WIDTH	    1
#define USB_AUDIO_TREBLE_ATTR_WIDTH	    1


/* up/down mix processing unit selectors */

#define USB_AUDIO_UDM_UNDEFINED 	    0x00
#define USB_AUDIO_UDM_ENABLE		    0x01
#define USB_AUDIO_UDM_MODE_SELECT	    0x02


/* dolby prologic processing unit control sectors */

#define USB_AUDIO_DPL_UNDEFINED 	    0x00
#define USB_AUDIO_DPL_ENABLE		    0x01
#define USB_AUDIO_DPL_MODE_SELECT	    0x02


/* 3d stereo extender processing unit control selectors */

#define USB_AUDIO_3D_UNDEFINED		    0x00
#define USB_AUDIO_3D_ENABLE		    0x01
#define USB_AUDIO_3D_SPACIOUSNESS	    0x03


/* reverberation processing unit control selectors */

#define USB_AUDIO_RCS_UNDEFINED 	    0x00
#define USB_AUDIO_RCS_ENABLE		    0x01
#define USB_AUDIO_RCS_REVERB_LEVEL	    0x02
#define USB_AUDIO_RCS_REVERB_TIME	    0x03
#define USB_AUDIO_RCS_REVERB_FEEDBACK	    0x04


/* chorus processing unit control selectors */

#define USB_AUDIO_CH_UNDEFINED		    0x00
#define USB_AUDIO_CH_ENABLE		    0x01
#define USB_AUDIO_CH_CHORUS_LEVEL	    0x02
#define USB_AUDIO_CH_CHORUS_RATE	    0x03
#define USB_AUDIO_CH_CHORUS_DEPTH	    0x04


/* dynamic range compressor processing unit control selectors */

#define USB_AUDIO_DRC_UNDEFINED 	    0x00
#define USB_AUDIO_DRC_ENABLE		    0x01
#define USB_AUDIO_DRC_COMPRESSION_RATE	    0x02
#define USB_AUDIO_DRC_MAXAMPL		    0x03
#define USB_AUDIO_DRC_THRESHOLD 	    0x04
#define USB_AUDIO_DRC_ATTACK_TIME	    0x05
#define USB_AUDIO_DRC_RELEASE_TIME	    0x06


/* extension unit control selectors */

#define USB_AUDIO_ECS_UNDEFINED 	    0x00
#define USB_AUDIO_ECS_ENABLE		    0x01


/* endpoint control selectors */

#define USB_AUDIO_EPS_UNDEFINED 	    0x00
#define USB_AUDIO_EPS_SAMPLING_FREQ	    0x01
#define USB_AUDIO_EPS_PITCH		    0x02


/* spatial locations in an audio cluster */

#define USB_AUDIO_LOC_LEFT_FRONT	    0x0001
#define USB_AUDIO_LOC_RIGHT_FRONT	    0x0002
#define USB_AUDIO_LOC_CENTER_FRONT	    0x0004
#define USB_AUDIO_LOC_LOW_FREQ_ENHANCE	    0x0008
#define USB_AUDIO_LOC_LEFT_SURROUND	    0x0010
#define USB_AUDIO_LOC_RIGHT_SURROUND	    0x0020
#define USB_AUDIO_LOC_LEFT_OF_CENTER	    0x0040
#define USB_AUDIO_LOC_RIGHT_OF_CENTER	    0x0080
#define USB_AUDIO_LOC_SURROUND		    0x0100
#define USB_AUDIO_LOC_SIDE_LEFT 	    0x0200
#define USB_AUDIO_LOC_SIDE_RIGHT	    0x0400
#define USB_AUDIO_LOC_TOP		    0x0800


/* terminal types */

#define USB_AUDIO_TERM_UNDEFINED	    0x0100
#define USB_AUDIO_TERM_STREAMING	    0x0101
#define USB_AUDIO_TERM_VENDOR_SPECIFIC	    0x01ff


/* input terminal types */

#define USB_AUDIO_INTERM_UNDEFINED	    0x0200
#define USB_AUDIO_INTERM_MIC		    0x0201
#define USB_AUDIO_INTERM_DESKTOP_MIC	    0x0202
#define USB_AUDIO_INTERM_PERSONAL_MIC	    0x0203
#define USB_AUDIO_INTERM_OMNI_DIR_MIC	    0x0204
#define USB_AUDIO_INTERM_MIC_ARRAY	    0x0205
#define USB_AUDIO_INTERM_PROC_MIC_ARRAY     0x0206


/* output terminal types */

#define USB_AUDIO_OUTTERM_UNDEFINED	    0x0300
#define USB_AUDIO_OUTTERM_SPEAKER	    0x0301
#define USB_AUDIO_OUTTERM_HEADPHONES	    0x0302
#define USB_AUDIO_OUTTERM_HEAD_MOUNT	    0x0303
#define USB_AUDIO_OUTTERM_DESKTOP_SPKR	    0x0304
#define USB_AUDIO_OUTTERM_ROOM_SPKR	    0x0305
#define USB_AUDIO_OUTTERM_COMM_SPKR	    0x0306
#define USB_AUDIO_OUTTERM_LOW_FREQ_SPKR     0x0307


/* bi-directional terminal types */

#define USB_AUDIO_BITERM_UNDEFINED	    0x0400
#define USB_AUDIO_BITERM_HEADSET	    0x0401
#define USB_AUDIO_BITERM_HEADSET_ALT	    0x0402
#define USB_AUDIO_BITERM_SPKRPHONE	    0x0403
#define USB_AUDIO_BITERM_SPKRPHONE_ECHO_SUP 0x0404
#define USB_AUDIO_BITERM_SPKRPHONE_ECHO_CAN 0x0405


/* telephony terminal types */

#define USB_AUDIO_TELTERM_UNDEFINED	    0x0500
#define USB_AUDIO_TELTERM_PHONE_LINE	    0x0501
#define USB_AUDIO_TELTERM_TELEPHONE	    0x0502
#define USB_AUDIO_TELTERM_DOWN_LINE_PHONE   0x0503


/* external terminal types */

#define USB_AUDIO_EXTTERM_UNDEFINED	    0x0600
#define USB_AUDIO_EXTTERM_ANALOG	    0x0601
#define USB_AUDIO_EXTTERM_DIGITAL	    0x0602
#define USB_AUDIO_EXTTERM_LINE		    0x0603
#define USB_AUDIO_EXTTERM_LEGACY	    0x0604
#define USB_AUDIO_EXTTERM_SPDIF 	    0x0605
#define USB_AUDIO_EXTTERM_1394_DA	    0x0606
#define USB_AUDIO_EXTTERM_1394_DV_SOUND     0x0607


/* embedded function terminal types */

#define USB_AUDIO_EMBTERM_UNDEFINED	    0x0700
#define USB_AUDIO_EMBTERM_LEVEL_CAL_NOISE   0x0701
#define USB_AUDIO_EMBTERM_EQUAL_NOISE	    0x0702
#define USB_AUDIO_EMBTERM_CD_PLAYER	    0x0703
#define USB_AUDIO_EMBTERM_DAT		    0x0704
#define USB_AUDIO_EMBTERM_DCC		    0x0705
#define USB_AUDIO_EMBTERM_MINIDISK	    0x0706
#define USB_AUDIO_EMBTERM_ANALOG_TAPE	    0x0707
#define USB_AUDIO_EMBTERM_PHONOGRAPH	    0x0708
#define USB_AUDIO_EMBTERM_VCR_AUDIO	    0x0709
#define USB_AUDIO_EMBTERM_VIDEO_DISC	    0x070a
#define USB_AUDIO_EMBTERM_DVD_AUDIO	    0x070b
#define USB_AUDIO_EMBTERM_TV_TUNER	    0x070c
#define USB_AUDIO_EMBTERM_SAT_RECEIVER	    0x070d
#define USB_AUDIO_EMBTERM_CABLE_TUNER	    0x070e
#define USB_AUDIO_EMBTERM_DSS		    0x070f
#define USB_AUDIO_EMBTERM_RADIO_RECEIVER    0x0710
#define USB_AUDIO_EMBTERM_RADIO_TRANSMITTER 0x0711
#define USB_AUDIO_EMBTERM_MULTI_TRACK_REC   0x0712
#define USB_AUDIO_EMBTERM_SYNTHESIZER	    0x0713


/* audio data format type I codes */

#define USB_AUDIO_TYPE1_UNDEFINED	    0x0000
#define USB_AUDIO_TYPE1_PCM		    0x0001
#define USB_AUDIO_TYPE1_PCM8		    0x0002
#define USB_AUDIO_TYPE1_IEEE_FLOAT	    0x0003
#define USB_AUDIO_TYPE1_ALAW		    0x0004
#define USB_AUDIO_TYPE1_MULAW		    0x0005


/* audio data format type II codes */

#define USB_AUDIO_TYPE2_UNDEFINED	    0x1000
#define USB_AUDIO_TYPE2_MPEG		    0x1001
#define USB_AUDIO_TYPE2_AC3		    0x1002


/* audio data format type III codes */

#define USB_AUDIO_TYPE3_UNDEFINED	    0x2000
#define USB_AUDIO_TYPE3_1937_AC3	    0x2001
#define USB_AUDIO_TYPE3_1937_MPEG1_L1	    0x2002
#define USB_AUDIO_TYPE3_1937_MPEG1_L2_3     0x2003
#define USB_AUDIO_TYPE3_1937_MPEG2_NOEXT    0x2003
#define USB_AUDIO_TYPE3_1937_MPEG2_EXT	    0x2004
#define USB_AUDIO_TYPE3_1937_MPEG2_L1_LS    0x2005
#define USB_AUDIO_TYPE3_1937_MPEG2_L2_3_LS  0x2006


/* format type codes */

#define USB_AUDIO_FORMAT_UNDEFINED	    0x00
#define USB_AUDIO_FORMAT_TYPE1		    0x01
#define USB_AUDIO_FORMAT_TYPE2		    0x02
#define USB_AUDIO_FORMAT_TYPE3		    0x03


/* typedefs */

/* status word format */

typedef struct usb_audio_status_word
    {
    UINT8 statusType;
    UINT8 originatorId;
    } USB_AUDIO_STATUS_WORD, *pUSB_AUDIO_STATUS_WORD;


/* USB_AUDIO_STATUS_WORD.statusType */

#define USB_AUDIO_ST_AUDIO_CONTROL	    0x00
#define USB_AUDIO_ST_AUDIO_STREAMING_IF     0x01
#define USB_AUDIO_ST_AUDIO_STREAMING_EP     0x02
#define USB_AUDIO_ST_MEMORY_CHANGED	    0x40
#define USB_AUDIO_ST_INT_PENDING	    0x80


/* common audio descriptor header */

typedef struct usb_audio_descr_header
    {
    UINT8 length;		    /* length of descriptor in bytes */
    UINT8 descriptorType;	    /* descriptor type */
    UINT8 descriptorSubType;	    /* descriptor sub type */
    } USB_AUDIO_DESCR_HEADER, *pUSB_AUDIO_DESCR_HEADER;


/* standard AC (AudioControl) interface descriptor.
 *
 * NOTE: This structure is identical to the standard USB interface descriptor,
 * except that interfaceClass is defined as the Audio interface class, 
 * interfaceSubClass is defined as the audio interface subclass, and
 * interfaceProtocol is always 0.
 */


/* class-specific AC interface descriptor */

typedef struct usb_audio_ac_descr
    {
    UINT8 length;		    /* length of descriptor in bytes */
    UINT8 descriptorType;	    /* descriptor type */
    UINT8 descriptorSubType;	    /* descriptor sub type */
    UINT8 bcdAdc [2];		    /* release level in BCD */
    UINT8 totalLength [2];	    /* combined length of all descr */
    UINT8 inCollection; 	    /* number of streaming interfaces */
    UINT8 interfaceNbr [1];	    /* variable number of interface numbers */
    } USB_AUDIO_AC_DESCR, *pUSB_AUDIO_AC_DESCR;


/* header common to all AudioControl unit/terminal descriptors */

typedef struct usb_audio_ac_common
    {
    UINT8 length;		    /* length of descriptor in bytes */
    UINT8 descriptorType;	    /* descriptor type */
    UINT8 descriptorSubType;	    /* descriptor sub type */
    UINT8 unitId;		    /* unit/terminal ID */
    } USB_AUDIO_AC_COMMON, *pUSB_AUDIO_AC_COMMON;


/* input terminal descriptor */

typedef struct usb_audio_input_term_descr
    {
    UINT8 length;		    /* length of descriptor in bytes */
    UINT8 descriptorType;	    /* descriptor type */
    UINT8 descriptorSubType;	    /* descriptor sub type */
    UINT8 terminalId;		    /* ID of this terminal */
    UINT8 terminalType [2];	    /* type of terminal */
    UINT8 assocTerminal;	    /* ID of associated output terminal */
    UINT8 channels;		    /* count of channels */
    UINT8 channelConfig [2];	    /* see USB_AUDIO_LOC_xxxx */
    UINT8 channelNamesIndex;	    /* index of first string descr */
    UINT8 terminalNameIndex;	    /* index of string descr for this term */
    } USB_AUDIO_INPUT_TERM_DESCR, *pUSB_AUDIO_INPUT_TERM_DESCR;


/* output terminal descriptor */

typedef struct usb_audio_output_term_descr
    {
    UINT8 length;		    /* length of descriptor in bytes */
    UINT8 descriptorType;	    /* descriptor type */
    UINT8 descriptorSubType;	    /* descriptor sub type */
    UINT8 terminalId;		    /* ID of this terminal */
    UINT8 terminalType [2];	    /* type of terminal */
    UINT8 assocTerminal;	    /* ID of associate input terminal */
    UINT8 sourceId;		    /* ID of connected unit/terminal */
    UINT8 terminalNameIndex;	    /* index of string desdr for this term */
    } USB_AUDIO_OUTPUT_TERM_DESCR, *pUSB_AUDIO_OUTPUT_TERM_DESCR;


/* mixer unit descriptor */

typedef struct usb_audio_mixer_unit_descr
    {
    UINT8 length;		    /* length of descriptor in bytes */
    UINT8 descriptorType;	    /* descriptor type */
    UINT8 descriptorSubType;	    /* descriptor sub type */
    UINT8 unitId;		    /* ID of this unit */
    UINT8 nbrInPins;		    /* number of input pins */
    UINT8 sourceId [1]; 	    /* array of sources */
				    /* followed by a cluster descr */
				    /* followed by a controls byte */
				    /* followed by iMixer */
    } USB_AUDIO_MIXER_UNIT_DESCR, *pUSB_AUDIO_MIXER_UNIT_DESCR;


/* selector unit descriptor */

typedef struct usb_audio_selector_unit_descr
    {
    UINT8 length;		    /* length of descriptor in bytes */
    UINT8 descriptorType;	    /* descriptor type */
    UINT8 descriptorSubType;	    /* descriptor sub type */
    UINT8 unitId;		    /* ID of this unit */
    UINT8 nbrInPins;		    /* number of input pins */
    UINT8 sourceId [1]; 	    /* array of sources */
				    /* followed by iMixer */
    } USB_AUDIO_SELECTOR_UNIT_DESCR, *pUSB_AUDIO_SELECTOR_UNIT_DESCR;


/* feature unit descriptor */

typedef struct usb_audio_feature_unit_descr
    {
    UINT8 length;		    /* length of descriptor in bytes */
    UINT8 descriptorType;	    /* descriptor type */
    UINT8 descriptorSubType;	    /* descriptor sub type */
    UINT8 unitId;		    /* unique ID of this unit */
    UINT8 sourceId;		    /* ID of source unit/terminal */
    UINT8 controlSize;		    /* size of entries in controls array */
    UINT8 controls [1]; 	    /* variable length, 1 + no. of channels */
				    /* iFeature byte follows controls array */
    } USB_AUDIO_FEATURE_UNIT_DESCR, *pUSB_AUDIO_FEATURE_UNIT_DESCR;


/* processing unit descriptor (common part) */

typedef struct usb_audio_process_unit_descr
    {
    UINT8 length;		    /* length of descriptor in bytes */
    UINT8 descriptorType;	    /* descriptor type */
    UINT8 descriptorSubType;	    /* descriptor sub type */
    UINT8 unitId;		    /* unique ID of this unit */
    UINT8 processType [2];	    /* type of process performed by unit */
    UINT8 nbrInPins;		    /* number of input pins */
    UINT8 sourceId [1]; 	    /* array of sources */
				    /* ... */
    } USB_AUDIO_PROCESS_UNIT_DESCR, *pUSB_AUDIO_PROCESS_UNIT_DESCR;


/* extension unit descriptor */

typedef struct usb_audio_ext_unit_descr
    {
    UINT8 length;		    /* length of descriptor in bytes */
    UINT8 descriptorType;	    /* descriptor type */
    UINT8 descriptorSubType;	    /* descriptor sub type */
    UINT8 unitId;		    /* unique ID of this unit */
    UINT8 extensionCode [2];	    /* type of extension */
    UINT8 nbrInPins;		    /* number of input pins */
    UINT8 sourceId [1]; 	    /* array of sources */
				    /* ... */
    } USB_AUDIO_EXT_UNIT_DESCR, *pUSB_AUDIO_EXT_UNIT_DESCR;


/* standard AC interrupt endpoint descriptor */

typedef struct usb_audio_int_ep_descr
    {
    UINT8 length;		    /* bLength */
    UINT8 descriptorType;	    /* bDescriptorType */
    UINT8 endpointAddress;	    /* bEndpointAddress */
    UINT8 attributes;		    /* bmAttributes */
    UINT8 maxPacketSize [2];	    /* wMaxPacketSize */
    UINT8 interval;		    /* bInterval */
    UINT8 refresh;		    /* reset to 0 */
    UINT8 synchAddress; 	    /* reset to 0 */
    } USB_AUDIO_INT_EP_DESCR, *pUSB_AUDIO_INT_EP_DESCR;


/* standard AS (AudioStreaming) interface descriptor
 *
 * NOTE: This structure is identical to the standard USB interface descriptor,
 * except that interfaceClass is defined as the Audio interface class, 
 * interfaceSubClass is defined as the audio interface subclass, and
 * interfaceProtocol is always 0.
 */


/* class-specific AS interface descriptor */

typedef struct usb_audio_as_descr 
    {
    UINT8 length;		    /* length of descriptor in bytes */
    UINT8 descriptorType;	    /* descriptor type */
    UINT8 descriptorSubType;	    /* descriptor sub type */
    UINT8 terminalLink; 	    /* ID of connected terminal */
    UINT8 delay;		    /* delay introduced by data path */
    UINT8 formatTag [2];	    /* audio data format */
    } USB_AUDIO_AS_DESCR, *pUSB_AUDIO_AS_DESCR;


/* standard AS isochronous audio data endpoint descriptor */

typedef struct usb_audio_std_isoch_ep_descr
    {
    UINT8 length;		    /* bLength */
    UINT8 descriptorType;	    /* bDescriptorType */
    UINT8 endpointAddress;	    /* bEndpointAddress */
    UINT8 attributes;		    /* bmAttributes */
    UINT8 maxPacketSize [2];	    /* wMaxPacketSize */
    UINT8 interval;		    /* bInterval */
    UINT8 refresh;		    /* reset to 0 */
    UINT8 synchAddress; 	    /* address of synch endpoint */
    } USB_AUDIO_STD_ISOCH_EP_DESCR, *pUSB_AUDIO_STD_ISOCH_EP_DESCR;


/* class-specific isochronous audio data endpoint descriptor */

typedef struct usb_audio_class_isoch_ep_descr
    {
    UINT8 length;		    /* length of descriptor in bytes */
    UINT8 descriptorType;	    /* descriptor type */
    UINT8 descriptorSubType;	    /* descriptor sub type */
    UINT8 attributes;		    /* bmAttributes */
    UINT8 lockDelayUnits;	    /* indicates units for lockDelay */
    UINT8 lockDelay;		    /* internal clock lock time */
    } USB_AUDIO_CLASS_ISOCH_EP_DESCR, *pUSB_AUDIO_CLASS_ISOCH_EP_DESCR;


/* standard AS isoch synch endpoint descriptor */

typedef struct usb_audio_isoch_synch_ep_descr
    {
    UINT8 length;		    /* bLength */
    UINT8 descriptorType;	    /* bDescriptorType */
    UINT8 endpointAddress;	    /* bEndpointAddress */
    UINT8 attributes;		    /* bmAttributes */
    UINT8 maxPacketSize [2];	    /* wMaxPacketSize */
    UINT8 interval;		    /* bInterval */
    UINT8 refresh;		    /* synch refresh rate (pwr of 2) */
    UINT8 synchAddress; 	    /* reset to 0 */
    } USB_AUDIO_ISOCH_SYNCH_EP_DESCR, *pUSB_AUDIO_ISOCH_SYNCH_EP_DESCR;


/* common sampling frequency information */

typedef UINT8 USB_AUDIO_SAM_FREQ [3];

typedef struct usb_audio_sample_info
    {
    UINT8 freqType;		    /* how sampling freq. can be programmed */
    union
	{
	struct
	    {
	    USB_AUDIO_SAM_FREQ lowerFreq;   /* lower bound */
	    USB_AUDIO_SAM_FREQ upperFreq;   /* upper bound */
	    } continous;
	struct
	    {
	    USB_AUDIO_SAM_FREQ freq [1];    /* discrete sampling frequency */
	    } discrete;
	} freq;
    } USB_AUDIO_SAMPLE_INFO, *pUSB_AUDIO_SAMPLE_INFO;


/* type I format type descriptor */

typedef struct usb_audio_type_1_type_descr
    {
    UINT8 length;		    /* length of descriptor in bytes */
    UINT8 descriptorType;	    /* descriptor type */
    UINT8 descriptorSubType;	    /* descriptor sub type */
    UINT8 formatType;		    /* identifies format type */
    UINT8 nbrChannels;		    /* number of channels */
    UINT8 subFrameSize; 	    /* number of bytes for one audio subframe */
    UINT8 bitResolution;	    /* number of bits used per audio subframe */
    USB_AUDIO_SAMPLE_INFO sample;   /* sampling frequency info */
    } USB_AUDIO_TYPE_1_TYPE_DESCR, *pUSB_AUDIO_TYPE_1_TYPE_DESCR;


/* type II format type descriptor */

typedef struct usb_audio_type_2_type_descr
    {
    UINT8 length;		    /* length of descriptor in bytes */
    UINT8 descriptorType;	    /* descriptor type */
    UINT8 descriptorSubType;	    /* descriptor sub type */
    UINT8 formatType;		    /* identifies format type */
    UINT8 maxBitRate [2];	    /* max bits per second sup. by interface */
    UINT8 samplesPerFrame [2];	    /* nbr PCM audio samples in audio frame */
    USB_AUDIO_SAMPLE_INFO sample;   /* sampling frequency info */
    } USB_AUDIO_TYPE_2_TYPE_DESCR, *pUSB_AUDIO_TYPE_2_TYPE_DESCR;


/* type III format type descriptor */

typedef struct usb_audio_type_3_type_descr
    {
    UINT8 length;		    /* length of descriptor in bytes */
    UINT8 descriptorType;	    /* descriptor type */
    UINT8 descriptorSubType;	    /* descriptor sub type */
    UINT8 formatType;		    /* identifies format type */
    UINT8 nbrChannels;		    /* number of channels */
    UINT8 subFrameSize; 	    /* number of bytes for one audio subframe */
    UINT8 bitResolution;	    /* number of bits used per audio subframe */
    USB_AUDIO_SAMPLE_INFO sample;   /* sampling frequency info */
    } USB_AUDIO_TYPE_3_TYPE_DESCR, *pUSB_AUDIO_TYPE_3_TYPE_DESCR;


/* composite type descriptor */

typedef struct usb_audio_type_descr
    {
    UINT8 length;		    /* length of descriptor in bytes */
    UINT8 descriptorType;	    /* descriptor type */
    UINT8 descriptorSubType;	    /* descriptor sub type */
    UINT8 formatType;		    /* identifies format type */
    union
	{
	struct
	    {
	    UINT8 nbrChannels;	    /* number of channels */
	    UINT8 subFrameSize;     /* number of bytes for one audio subframe */
	    UINT8 bitResolution;    /* number of bits used per audio subframe */
	    USB_AUDIO_SAMPLE_INFO sample;   /* sampling frequency info */
	    } type1;
	struct
	    {
	    UINT8 maxBitRate [2];   /* max bits per second sup. by interface */
	    UINT8 samplesPerFrame [2];	/* nbr PCM audio samples in audio frame */
	    USB_AUDIO_SAMPLE_INFO sample;   /* sampling frequency info */
	    } type2;
	struct
	    {
	    UINT8 nbrChannels;	    /* number of channels */
	    UINT8 subFrameSize;     /* number of bytes for one audio subframe */
	    UINT8 bitResolution;    /* number of bits used per audio subframe */
	    USB_AUDIO_SAMPLE_INFO sample;   /* sampling frequency info */
	    } type3;
	} ts;
    } USB_AUDIO_TYPE_DESCR, *pUSB_AUDIO_TYPE_DESCR;


#ifdef	__cplusplus
}
#endif

#endif	/* __INCusbAudioh */


/* End of file. */

