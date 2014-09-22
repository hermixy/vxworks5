/* usbUhci.h - UHCI hardware interface definitions */

/* Copyright 2000 Wind River Systems, Inc. */
/*
Modification history
--------------------
01b,10may99,rcb Update style per suggestions from Claudio at WRS.
01a,27apr99,rcb First.
*/

#ifndef __INCusbUhcih
#define __INCusbUhcih

#ifdef	__cplusplus
extern "C" {
#endif

/*
DESCRIPTION

Defines the USB UHCI (Universal Host Controller Interface) register file
interface and memory structures shared between USB software and the UHCI
hardware.  

Please refer to Intel's UHCI specification, Revision 1.1 or later, for more 
information.

NOTE: The memory structures must not be padded, so the compiler must be
set to use the "no packing" or "0-byte packing" option.  Compile-time checks
are included in this file to ensure that structures compile to the correct
size.
*/


/***************************************************************************
 *			    Register Definitions
 ***************************************************************************/

/*
 * UHCI registers in the regular I/O address space.
 */

#define UHCI_USBCMD	    0x0     /* r/w  WORD    USB Command */
#define UHCI_USBSTS	    0x2     /* r/wc WORD    USB Status */
#define UHCI_USBINTR	    0x4     /* r/w  WORD    USB Interrupt Enable */
#define UHCI_FRNUM	    0x6     /* r/w  WORD    Frame Number */
#define UHCI_FRBASEADD	    0x8     /* r/w  DWORD   Frame List Base Address */
#define UHCI_SOFMOD	    0xc     /* r/w  BYTE    Start of Frame Modify */
#define UHCI_PORTSC1	    0x10    /* r/wc WORD    Port 1 Status/Control */
#define UHCI_PORTSC2	    0x12    /* r/wc WORD    Port 2 Status/Control */

/*
 * PCI registers relevant to UHCI in configuration space.
 */

#define UHCI_PCI_CLASSC     0x9     /* r/o  24-bits Class Code Register */
#define UHCI_PCI_USBBASE    0x20    /* r/w  DWORD   IO Space Base Address */
#define UHCI_PCI_SBRN	    0x60    /* r/o  BYTE    Serial Bus Release No. */

/*
 * UHCI_USBCMD register definition.
 */

#define UHCI_CMD_MAXP	    0x0080  /* Max Packet */
				    /* 1=64 bytes, 0=32 bytes */
#define UHCI_CMD_CF	    0x0040  /* Configure Flag */
#define UHCI_CMD_SWDBG	    0x0020  /* Software Debug. */
				    /* 1=debug mode, 0=normal mode */
#define UHCI_CMD_FGR	    0x0010  /* Force Global Resume. */
				    /* 1=host ctlr sends global resume */
#define UHCI_CMD_EGSM	    0x0008  /* Enter Global Suspect Mode */
				    /* 1=host ctlr enters global suspend */
#define UHCI_CMD_GRESET     0x0004  /* Global reset */
				    /* 1=host ctlr sends global reset */
#define UHCI_CMD_HCRESET    0x0002  /* Host Controller Reset */
				    /* 1=host ctlr reset */
#define UHCI_CMD_RS	    0x0001  /* Run/Stop. */
				    /* 1=run, 0=stop */


/*
 * UHCI_USBSTS register definition.
 */

#define UHCI_STS_HALTED     0x0020  /* Host Controller Halted */
#define UHCI_STS_PROCERR    0x0010  /* Host Controller Process Error */
#define UHCI_STS_HOSTERR    0x0008  /* Host System Error */
#define UHCI_STS_RESUME     0x0004  /* Resume Detect */
#define UHCI_STS_USBERR     0x0002  /* USB Error Interrupt */
#define UHCI_STS_USBINT     0x0001  /* USB Interrupt */

/*
 * UHCI_USBINTR register definition.
 */

#define UHCI_INTR_SHORT     0x0008  /* Short Packet Interrupt Enable */
				    /* 1=Enabled, 0=Disabled */
#define UHCI_INTR_COMPLETE  0x0004  /* Interrupt On Complete Enable */
				    /* 1=Enabled, 0=Disabled */
#define UHCI_INTR_RESUME    0x0002  /* Resume Interrupt Enable */
				    /* 1=Enabled, 0=Disabled */
#define UHCI_INTR_TIME_CRC  0x0001  /* Timeout/CRC Interrupt Enable */
				    /* 1=Enabled, 0=Disabled */

/*
 * UHCI_FRNUM register definition.
 */

#define UHCI_FRN_MASK	    0x07ff  /* The low 11-bits of UHCI_FRNUM contain */
				    /* the current USB frame number. */

/*
 * UHCI_FLBASEADD register definition.
 *
 * Bits 31:12 of this register define bits 31:12, respectively, of the base
 * address of the UHCI frame list in physical memory.  The frame list must be
 * aligned on a 4k boundary.
 */


/*
 * UHCI_SOFMOD register definition.
 */

#define UHCI_SOF_MASK	    0x7f    /* The low 7-bits of UHCI_SOFMOD contain */
				    /* a count from 0..127 which is added to */
				    /* UHCI_SOF_BASE to determine start-of- */
				    /* frame timing. */

#define UHCI_SOF_BASE	    11936   /* Base value for start-of-frame timing */
#define UHCI_SOF_MAX	    12063   /* max value for start-of-frame timing */
#define UHCI_SOF_DEFAULT    64	    /* UHCI_SOFMOD defaults to 64, yielding */
				    /* a default start-of-frame timing of */
				    /* 11936+64 = 12000. */

/*
 * UHCI_PORTSCn register definition.
 */

#define UHCI_PORT_SUSPEND   0x1000  /* Suspend state */
				    /* 1=port in suspend, 0=not suspended */
#define UHCI_PORT_RESET     0x0200  /* Port reset */
				    /* 1=Port is in reset, 0=port not reset */
#define UHCI_PORT_LOWSPEED  0x0100  /* Low Speed Device Attached */
				    /* 1=low speed dev, 0=full speed dev */
#define UHCI_PORT_RESUME    0x0040  /* Resume Detect */
				    /* 1=resume detected/driven, 0=no resume */
#define UHCI_PORT_LINESTS   0x0030  /* Line Status */
				    /* bit 4=D+, bit 5=D- logical levels */
#define UHCI_PORT_ENBCHG    0x0008  /* Port Enable/Disable Change */
				    /* 1=port status changed, 0=no change */
#define UHCI_PORT_ENABLE    0x0004  /* Port Enabled/Disabled */
				    /* 1=Enable, 0=Disable */
#define UHCI_PORT_CNCTCHG   0x0002  /* Connect Status Change */
				    /* 1=Connect status changed, 0=no change */
#define UHCI_PORT_CNCTSTS   0x0001  /* Current Connect Status */
				    /* 1=Device present, 0=no device */

/*
 * UHCI_PCI_CLASSC register definition.
 */

#define UHCI_CLASSC_BASEC_MASK	0xff0000    /* Base class code mask */
#define UHCI_CLASSC_BASEC_ROT	16	    /* Base class code bit loc */

#define UHCI_CLASSC_SCC_MASK	0x00ff00    /* Sub-Class code mask */
#define UHCI_CLASSC_SCC_ROT	8	    /* Sub-Class code bit loc */

#define UHCI_CLASSC_PI_MASK	0x0000ff    /* Programming Interface mask */
#define UHCI_CLASSC_PI_ROT	0	    /* Programming Interface bit loc */

/* Macros to retrieve class code fields */

#define UHCI_CLASSC_BASEC(x) \
    (((x) & UHCI_CLASSC_BASEC_MASK) >> UHCI_CLASSC_BASEC_ROT)

#define UHCI_CLASSC_SCC(x) \
    (((x) & UHCI_CLASSC_SCC_MASK) >> UHCI_CLASSC_SCC_ROT)

#define UHCI_CLASSC_PI(x) \
    (((x) & UHCI_CLASSC_PI_MASK) >> UHCI_CLASSC_PI_ROT)

#define UHCI_CLASS		0x0c	    /* BASEC value for serial bus */
#define UHCI_SUBCLASS		0x03	    /* SCC value for USB */
#define UHCI_PGMIF		0x00	    /* no specific pgm i/f defined */


/*
 * UHCI_PCI_USBBASE register definition.
 */

#define UHCI_USBBASE_MASK	0x0000ffe0  /* Bits 15:5 define I/O base */

/* Macro to retrieve base address */

#define UHCI_USBBASE(x) 	((x) & UHCI_USBBASE_MASK)


/* 
 * UHCI_PCI_SBRN register definition.
 *
 * Bits 7:0 define the USB release number supported by the host controller
 */

#define UHCI_SBRN_PREREL    0x00    /* Pre-release 1.0 */
#define UHCI_SBRN_REL10     0x10    /* Release 1.0 */


/* number of ports on a UHC */

#define UHCI_PORTS	    2


/* 
 * UHCI_FRAME_WINDOW defines the maximum number of frames "into the future"
 * for which an isochronous transfer may be scheduled.
 */

#define UHCI_FRAME_WINDOW	    1024    /* 1024 entries in frame list */


/* values for emulated UHC hub descriptor */

#define UHCI_PWR_ON_2_PWR_GOOD	    1	    /* = 2msec for pwr on to pwr good */
#define UHCI_HUB_CONTR_CURRENT	    0	    /* 0 mA for hub ctlr current */
#define UHCI_HUB_INTERVAL	    0xff    /* hub service interval */


/***************************************************************************
 *		       Memory Structure Definitions
 ***************************************************************************/

/*
 * UHCI link pointers, including frame list pointers and the pointers found
 * in UHCI_TD and UHCI_QH structures, use the same basic structure as defined 
 * below.  The exception is the VF (Depth/Breadth select) which is only used
 * in the UHCI_TD structure.
 */

#define UHCI_LINK_PTR_MASK	0xfffffff0  /* Adrs of first obj */

#define UHCI_LINK_VF		0x00000004  /* Depth/Breadth select */
					    /* 1=Depth first, 0=breadth first */

#define UHCI_LINK_QH		0x00000002  /* QH/TD Select */
#define UHCI_LINK_TD		0x00000000  /* 1=QH, 0=TD */

#define UHCI_LINK_TERMINATE	0x00000001  /* Terminate */
					    /* 1=End of list, 0=pointer valid */


/* the UHCI frame list has 1024 entries (1024 @ 4 bytes each = 4k) */

#define UHCI_FRAME_LIST_ENTRIES     1024
#define UHCI_FRAME_LIST_ALIGNMENT   4096

#define UHCI_TD_ALIGNMENT	    16
#define UHCI_QH_ALIGNMENT	    16


/*
 * UHCI_FRAME_LIST
 *
 * NOTE: Frame list must be allocated on a 4k page boundary
 */

typedef struct uhci_frame_list
    {
    UINT32  linkPtr [UHCI_FRAME_LIST_ENTRIES];	/* pointers to TD or QH */
    } UHCI_FRAME_LIST, *pUHCI_FRAME_LIST;


/*
 * Transfer Descriptor.
 *
 * NOTE: Each TD must be aligned on a 16-byte boundary.
 */

typedef struct uhci_td
    {
    UINT32  linkPtr;		    /* Link Pointer to another TD or QH */
    UINT32  ctlSts;		    /* Control and Status */
    UINT32  token;		    /* Token for packet header */
    UINT32  bfrPtr;		    /* Data buffer pointer */
    UINT32  software [4];	    /* 4 DWORDs reserved for software */
    } UHCI_TD, *pUHCI_TD;

#define UHCI_TD_LEN	32	    /* TD length required by UHCI hardware */
#define UHCI_TD_ACTLEN	sizeof (UHCI_TD)

/*
 * UHCI_TD.CtlSts definition.
 */

#define UHCI_TDCS_SHORT 	0x20000000  /* Short Packet Detect */
					    /* 1=Enable, 0=Disable */

#define UHCI_TDCS_ERRCTR_MASK	0x18000000  /* Error Counter Mask */
#define UHCI_TDCS_ERRCTR_NOLIM	0x00000000  /* No limit */
#define UHCI_TDCS_ERRCTR_1ERR	0x08000000  /* 1 error */
#define UHCI_TDCS_ERRCTR_2ERR	0x10000000  /* 2 errors */
#define UHCI_TDCS_ERRCTR_3ERR	0x18000000  /* 3 errors */

#define UHCI_TDCS_LOWSPEED	0x04000000  /* Low Speed Device */
					    /* 1=low speed dev, 0=full speed */
#define UHCI_TDCS_ISOCH 	0x02000000  /* Isochronous Select */
					    /* 1=isoch TD, 0=non-isoch TD */
#define UHCI_TDCS_COMPLETE	0x01000000  /* Interrupt on Complete */
					    /* 1=Issue IOC */

					    /* Cmd execution status bits */
#define UHCI_TDCS_STS_ACTIVE	0x00800000  /* Active */
#define UHCI_TDCS_STS_STALLED	0x00400000  /* Stalled */
#define UHCI_TDCS_STS_DBUFERR	0x00200000  /* Data buffer error */
#define UHCI_TDCS_STS_BABBLE	0x00100000  /* Babble Detected */
#define UHCI_TDCS_STS_NAK	0x00080000  /* NAK Received */
#define UHCI_TDCS_STS_TIME_CRC	0x00040000  /* CRC/Timeout error */
#define UHCI_TDCS_STS_BITSTUFF	0x00020000  /* Bitstuff error */

#define UHCI_TDCS_ACTLEN_MASK	0x000007ff  /* Actual Length mask */
#define UHCI_TDCS_ACTLEN_ROT	0	    /* Actual length bit loc */

#define UHCI_PACKET_SIZE_MASK	0x000007ff  /* Mask for 11-bit field */


/* Macro to retrieve UHCI_TDCS_ACTLEN
 *
 * NOTE: UHCI actlen is one less than the real length.	
 * The macro compensates automatically.
 */

#define UHCI_TDCS_ACTLEN(x) \
    ((((x) >> UHCI_TDCS_ACTLEN_ROT) + 1) & UHCI_PACKET_SIZE_MASK)


/*
 * UHCI_TD.Token definition.
 */

#define UHCI_TDTOK_MAXLEN_MASK	0xffe00000  /* Maximum length mask */
#define UHCI_TDTOK_MAXLEN_ROT	21	    /* Maximum length bit loc */

#define UHCI_TDTOK_DATA_TOGGLE	0x00080000  /* Data Toggle */
					    /* 0=DATA0, 1=DATA1 */

#define UHCI_TDTOK_ENDPT_MASK	0x00078000  /* Endpoint Mask */
#define UHCI_TDTOK_ENDPT_ROT	15	    /* Endpoint bit loc */

#define UHCI_TDTOK_DEVADRS_MASK 0x00007f00  /* Device Address Mask */
#define UHCI_TDTOK_DEVADRS_ROT	8	    /* Device Address bit loc */

#define UHCI_TDTOK_PID_MASK	0x000000ff  /* Packet ID mask */
#define UHCI_TDTOK_PID_ROT	0	    /* Packet ID bit loc */


/* Macros to retrieve/format UHCI_TDTOK_MAXLEN 
 *
 * NOTE: UHCI maxlen is one less than the real length.	The macro 
 * compensates automatically.
 */

#define UHCI_TDTOK_MAXLEN(x) \
    ((((x) >> UHCI_TDTOK_MAXLEN_ROT) + 1) & UHCI_PACKET_SIZE_MASK)

#define UHCI_TDTOK_MAXLEN_FMT(x) \
    ((((x) - 1) & UHCI_PACKET_SIZE_MASK) << UHCI_TDTOK_MAXLEN_ROT)

/* Macros to retrieve/format UHCI_TDTOK_ENDPT */

#define UHCI_TDTOK_ENDPT(x) \
    (((x) & UHCI_TDTOK_ENDPT_MASK) >> UHCI_TDTOK_ENDPT_ROT)

#define UHCI_TDTOK_ENDPT_FMT(x) \
    (((x) << UHCI_TDTOK_ENDPT_ROT) & UHCI_TDTOK_ENDPT_MASK)

/* Macros to retrieve/format UHCI_TDTOK_DEVADRS */

#define UHCI_TDTOK_DEVADRS(x) \
    (((x) & UHCI_TDTOK_DEVADRS_MASK) >> UHCI_TDTOK_DEVADRS_ROT)

#define UHCI_TDTOK_DEVADRS_FMT(x) \
    (((x) << UHCI_TDTOK_DEVADRS_ROT) & UHCI_TDTOK_DEVADRS_MASK)

/* Macros to retrieve/format UHCI_TDTOK_PID */

#define UHCI_TDTOK_PID(x) \
    (((x) & UHCI_TDTOK_PID_MASK) >> UHCI_TDTOK_PID_ROT)

#define UHCI_TDTOK_PID_FMT(x) \
    (((x) << UHCI_TDTOK_PID_ROT) & UHCI_TDTOK_PID_MASK)

/*
 * Queue Head.
 *
 * NOTE: Each Queue Head (QH) must be aligned on a 16-byte boundary.
 */

typedef struct uhci_qh
    {
    UINT32  qhLink;		/* Link pointer to next QH */
    UINT32  tdLink;		/* Link to queue element */
    } UHCI_QH, *pUHCI_QH;

#define UHCI_QH_LEN	8	    /* QH length required by UHCI hardware */
#define UHCI_QH_ACTLEN	sizeof (UHCI_QH)


#ifdef	__cplusplus
}
#endif

#endif	/* __INCusbUhcih */


/* End of file. */

