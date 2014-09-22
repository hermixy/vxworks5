/* usbHid.h - USB HID (Human Interface Devices) definitions */

/* Copyright 2000 Wind River Systems, Inc. */

/*
Modification history
--------------------
01b,07oct99,rcb  Add definitions for mouse device.
01a,30aug99,rcb  First.
*/

#ifndef __INCusbHidh
#define __INCusbHidh

#ifdef	__cplusplus
extern "C" {
#endif


/* defines */

/* USB HID subclass codes */

#define USB_SUBCLASS_HID_NONE	0x00
#define USB_SUBCLASS_HID_BOOT	0x01


/* USB HID BOOT protocol codes (part of interface class) 
 *
 * NOTE: Do not confuse with similarly named USB_HID_PROTOCOL_xxxx defines
 * later in this file. */

#define USB_PROTOCOL_HID_BOOT_NONE	0x00
#define USB_PROTOCOL_HID_BOOT_KEYBOARD	0x01
#define USB_PROTOCOL_HID_BOOT_MOUSE	0x02


/* USB requests for HID devices */

#define USB_REQ_HID_GET_REPORT	    0x01
#define USB_REQ_HID_GET_IDLE	    0x02
#define USB_REQ_HID_GET_PROTOCOL    0x03
#define USB_REQ_HID_SET_REPORT	    0x09
#define USB_REQ_HID_SET_IDLE	    0x0a
#define USB_REQ_HID_SET_PROTOCOL    0x0b


/* USB HID report types */

#define USB_HID_RPT_TYPE_INPUT		0x01
#define USB_HID_RPT_TYPE_OUTPUT 	0x02
#define USB_HID_RPT_TYPE_FEATURE	0x03


/* USB HID idle interval */

#define USB_HID_IDLE_MSEC_PER_UNIT	4


/* USB HID protocol values (used for USB_REQ_HID_SET_PROTOCOL) 
 *
 * NOTE: Do not confuse with similarly named USB_PROTOCOL_HID_xxxx
 * defines earlier in this file. 
 */

#define USB_HID_PROTOCOL_BOOT		0
#define USB_HID_PROTOCOL_REPORT 	1


/* HID keyboard definitions */

#define BOOT_RPT_KEYCOUNT   6	    /* 6 keys returned in std boot report */


/* HID keyboard modifier key definitions */

#define MOD_KEY_LEFT_CTRL   0x01
#define MOD_KEY_LEFT_SHIFT  0x02
#define MOD_KEY_LEFT_ALT    0x04
#define MOD_KEY_LEFT_GUI    0x08
#define MOD_KEY_RIGHT_CTRL  0x10
#define MOD_KEY_RIGHT_SHIFT 0x20
#define MOD_KEY_RIGHT_ALT   0x40
#define MOD_KEY_RIGHT_GUI   0x80

#define MOD_KEY_CTRL	    (MOD_KEY_LEFT_CTRL | MOD_KEY_RIGHT_CTRL)
#define MOD_KEY_SHIFT	    (MOD_KEY_LEFT_SHIFT | MOD_KEY_RIGHT_SHIFT)
#define MOD_KEY_ALT	    (MOD_KEY_LEFT_ALT | MOD_KEY_RIGHT_ALT)
#define MOD_KEY_GUI	    (MOD_KEY_LEFT_GUI | MOD_KEY_RIGHT_GUI)


/* HID keyboard LED definitions for output report */

#define RPT_LED_NUM_LOCK    0x01
#define RPT_LED_CAPS_LOCK   0x02
#define RPT_LED_SCROLL_LOCK 0x04
#define RPT_LED_COMPOSE     0x08
#define RPT_LED_KANA	    0x10


/* HID mouse report definitions */

#define MOUSE_BUTTON_1	    0x01
#define MOUSE_BUTTON_2	    0x02
#define MOUSE_BUTTON_3	    0x04


/* Maximum length for a HID "boot report" */

#define HID_BOOT_REPORT_MAX_LEN     8


/* typedefs */

/*
 * HID_KBD_BOOT_REPORT
 */

typedef struct hid_kbd_boot_report
    {
    UINT8 modifiers;		    /* modifier keys */
    UINT8 reserved;		    /* reserved */
    UINT8 scanCodes [BOOT_RPT_KEYCOUNT]; /* individual scan codes */
    } HID_KBD_BOOT_REPORT, *pHID_KBD_BOOT_REPORT;


/*
 * HID_MSE_BOOT_REPORT
 */

typedef struct hid_mse_boot_report
    {
    UINT8 buttonState;		    /* buttons */
    char xDisplacement; 	    /* signed x-displacement */
    char yDisplacement; 	    /* signed y-displacement */
    } HID_MSE_BOOT_REPORT, *pHID_MSE_BOOT_REPORT;


#ifdef	__cplusplus
}
#endif

#endif	/* __INCusbHidh */


/* End of file. */
