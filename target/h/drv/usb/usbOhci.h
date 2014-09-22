/* usbOhci.h - USB OHCI controller definitions */

/* Copyright 2000 Wind River Systems, Inc. */

/*
Modification history
--------------------
01b,28jan00,rcb  Redefine page masks for isochronous TDs.
01a,04oct99,rcb  First.
*/
/*
DESCRIPTION

Defines the USB OHCI (Open Host Controller Interface) register file
interface and memory structures shared between USB software and the OHCI
hardware.  

Please refer to the OpenHCI Specification, Release 1.0a or later, for more 
information.
*/

#ifndef __INCusbOhcih
#define __INCusbOhcih

#ifdef	__cplusplus
extern "C" {
#endif


/* defines */

/* OpenHCI PCI class defintion */

#define OHCI_CLASS		0x0c	    /* BASEC value for serial bus */
#define OHCI_SUBCLASS		0x03	    /* SCC value for USB */
#define OHCI_PGMIF		0x10	    /* OpenHCI */


/* completion codes */

#define OHCI_CC_NO_ERROR	0x0	    /* no errors detected */
#define OHCI_CC_CRC		0x1	    /* CRC error */
#define OHCI_CC_BITSTUFF	0x2	    /* bit stuffing error */
#define OHCI_CC_DATA_TOGGLE	0x3	    /* data toggle mismatch */
#define OHCI_CC_STALL		0x4	    /* endpoint stall */
#define OHCI_CC_NO_RESPONSE	0x5	    /* device not responding */
#define OHCI_CC_PID_CHECK	0x6	    /* check bits on PID failed */
#define OHCI_CC_UNEXPECTED_PID	0x7	    /* unexpected PID */
#define OHCI_CC_DATA_OVERRUN	0x8	    /* packet exceeded MPS */
#define OHCI_CC_DATA_UNDERRUN	0x9	    /* packet less than MPS */
#define OHCI_CC_BFR_OVERRUN	0xc	    /* host bus couldn't keep up */
#define OHCI_CC_BFR_UNDERRUN	0xd	    /* host bus couldn't keep up */
#define OHCI_CC_NOT_ACCESSED	0xe	    /* set by software */


/* typedefs */

/* Host Controller Operational Registers
 *
 * NOTE: The following values are offsets relative to the base of the memory
 * window allocated for the OHCI controller.
 */

#define OHCI_HC_REVISION	0x0
#define OHCI_HC_CONTROL 	0x4
#define OHCI_HC_COMMAND_STATUS	0x8
#define OHCI_HC_INT_STATUS	0xc
#define OHCI_HC_INT_ENABLE	0x10
#define OHCI_HC_INT_DISABLE	0x14
#define OHCI_HC_HCCA		0x18
#define OHCI_HC_PERIOD_CUR_ED	0x1c
#define OHCI_HC_CONTROL_HEAD_ED 0x20
#define OHCI_HC_CONTROL_CUR_ED	0x24
#define OHCI_HC_BULK_HEAD_ED	0x28
#define OHCI_HC_BULK_CUR_ED	0x2c
#define OHCI_HC_DONE_HEAD	0x30
#define OHCI_HC_FM_INTERVAL	0x34
#define OHCI_HC_FM_REMAINING	0x38
#define OHCI_HC_FM_NUMBER	0x3c
#define OHCI_HC_PERIODIC_START	0x40
#define OHCI_HC_LS_THRESHOLD	0x44
#define OHCI_HC_RH_DESCR_A	0x48
#define OHCI_HC_RH_DESCR_B	0x4c
#define OHCI_HC_RH_STATUS	0x50
#define OHCI_HC_RH_PORT_STATUS	0x54	    /* array of port regs */


/* OHCI_HC_REVISION register */

#define OHCI_RREV_REV_MASK	0x000000ff  /* BCD OpenHCI revision */


/* OHCI_HC_CONTROL register */

#define OHCI_CTL_CBSR_MASK	0x00000003  /* contro/bulk srvc ratio */
#define OHCI_CTL_CBSR_ROT	0
#define OHCI_CTL_CBSR_1TO1	0x00000000
#define OHCI_CTL_CBSR_2TO1	0x00000001
#define OHCI_CTL_CBSR_3TO1	0x00000002
#define OHCI_CTL_CBSR_4TO1	0x00000003

#define OHCI_CTL_PLE		0x00000004  /* periodic list enable */
#define OHCI_CTL_IE		0x00000008  /* isochronous list enable */
#define OHCI_CTL_CLE		0x00000010  /* control list enable */
#define OHCI_CTL_BLE		0x00000020  /* bulk list enable */

#define OHCI_CTL_HCFS_MASK	0x000000c0  /* host controller func state */
#define OHCI_CTL_HCFS_RESET	0x00000000  /* UsbReset */
#define OHCI_CTL_HCFS_RESUME	0x00000040  /* UsbResume */
#define OHCI_CTL_HCFS_OP	0x00000080  /* UsbOperational */
#define OHCI_CTL_HCFS_SUSPEND	0x000000c0  /* UsbSuspend */

#define OHCI_CTL_IR		0x00000100  /* interrupt routing */
#define OHCI_CTL_RWC		0x00000200  /* remote wakeup connected */
#define OHCI_CTL_RWE		0x00000400  /* remote wakeup enable */

/* macros to format/retrieve OHCI_TCL_CBSR (control/bulk ratio) field */

#define OHCI_CTL_CBSR(x)     (((x) & OHCI_CTL_CBSR_MASK) >> OHCI_CTL_CBSR_ROT)
#define OHCI_CTL_CBSR_FMT(x) (((x) << OHCI_CTL_CBSR_ROT) & OHCI_CTL_CBSR_MASK)


/* OHCI_HC_COMMAND_STATUS register */

#define OHCI_CS_HCR		0x00000001  /* host controller reset */
#define OHCI_CS_CLF		0x00000002  /* control list filled */
#define OHCI_CS_BLF		0x00000004  /* bulk list filled */
#define OHCI_CS_OCR		0x00000008  /* ownership change request */

#define OHCI_CS_SOC_MASK	0x00030000  /* scheduling overrun count */
#define OHCI_CS_SOC_ROT 	16

/* macro to retrieve OHCI_CS_SOC (control/bulk ratio) field */

#define OHCI_CS_SOC(x)	(((x) & OHCI_CS_SOC_MASK) >> OHCI_CS_SOC_ROT)


/* OHCI_HC_INT_STATUS, OHCI_HC_INT_ENABLE, and OHCI_HC_INT_DISABLE registers */

#define OHCI_INT_SO		0x00000001  /* scheduling overrun */
#define OHCI_INT_WDH		0x00000002  /* writeback done head */
#define OHCI_INT_SF		0x00000004  /* start of frame */
#define OHCI_INT_RD		0x00000008  /* resume detected */
#define OHCI_INT_UE		0x00000010  /* unrecoverable error */
#define OHCI_INT_FNO		0x00000020  /* frame number overflow */
#define OHCI_INT_RHSC		0x00000040  /* root hub status change */
#define OHCI_INT_OC		0x40000000  /* ownership change */
#define OHCI_INT_MIE		0x80000000  /* master interrupt enable */


/* OHCI_HC_FM_INTERVAL register */

#define OHCI_FMI_FI_MASK	0x00003fff  /* frame interval */
#define OHCI_FMI_FI_ROT 	0
#define OHCI_FMI_FI_DEFAULT	11999

#define OHCI_FMI_FSMPS_MASK	0x7fff0000  /* FS largest data packet */
#define OHCI_FMI_FSMPS_ROT	16

#define OHCI_FMI_FIT		0x80000000  /* frame interval toggle */

/* macros to format/retrieve OHCI_FMI_FI (frame interval) field */

#define OHCI_FMI_FI(x)	    (((x) & OHCI_FMI_FI_MASK) >> OHCI_FMI_FI_ROT)
#define OHCI_FMI_FI_FMT(x)  (((x) << OHCI_FMI_FI_ROT) & OHCI_FMI_FI_MASK)

/* macros to format/retrieve OHCI_FMI_FSMPS (FS largest data packet) field */

#define OHCI_FMI_FSMPS(x)     (((x) & OHCI_FMI_FSMPS_MASK) >> OHCI_FMI_FSMPS_ROT)
#define OHCI_FMI_FSMPS_FMT(x) (((x) << OHCI_FMI_FSMPS_ROT) & OHCI_FMI_FSMPS_MASK)


/* OHCI_HC_FM_REMAINING register */

#define OHCI_FMR_FR_MASK	0x00003fff  /* frame remaining */
#define OHCI_FMR_FR_ROT 	0

#define OHCI_FMR_FRT		0x80000000  /* frame remaining toggle */

/* macro to retrieve OHCI_FMI_FR (frame remaining) field */

#define OHCI_FMI_FR(x)	(((x) & OHCI_FMI_FR_MASK) >> OHCI_FMI_FR_ROT)


/* OHCI_HC_FM_NUMBER register */

#define OHCI_FMN_FN_MASK	0x0000ffff  /* frame number */
#define OHCI_FMN_FN_ROT 	0

/* macro to retrieve OHCI_FMN_FN (frame number) field */

#define OHCI_FMN_FN(x)	(((x) & OHCI_FMN_FN_MASK) >> OHCI_FMN_FN_ROT)

/* OHCI_FRAME_WINDOW defines the frame number range supported by OHCI. */

#define OHCI_FRAME_WINDOW	0x10000


/* OHCI_HC_PERIODIC_START register */

#define OHCI_PS_PS_MASK 	0x00003fff  /* periodic start */
#define OHCI_PS_PS_ROT		0

/* macros to format/retrieve OHCI_PS_PS (perodic start) field */

#define OHCI_PS_PS(x)	  (((x) & OHCI_PS_PS_MASK) >> OHCI_PS_PS_ROT)
#define OHCI_PS_PS_FMT(x) (((x) << OHCI_PS_PS_ROT) & OHCI_PS_PS_MASK)


/* OHCI_LS_THRESHOLD resgister */

#define OHCI_LS_LST_MASK	0x000003ff  /* lowspeed threshold */
#define OHCI_LS_LST_ROT 	0

/* macros to format/retrieve OHCI_LS_LST (low speed threshold) field */

#define OHCI_LS_LST(x)	   (((x) & OHCI_LS_LST_MASK) >> OHCI_LS_LST_ROT)
#define OHCI_LS_LST_FMT(x) (((x) << OHCI_LS_LST_ROT) & OHCI_LS_LST_MASK)


/* OHCI_RH_DESCR_A register */

#define OHCI_RHDA_NDP_MASK	0x000000ff  /* number of downstream ports */
#define OHCI_RHDA_NDP_ROT	0

#define OHCI_RHDA_PSM		0x00000100  /* power swithcing mode */
#define OHCI_RHDA_NPS		0x00000200  /* no power switching */
#define OHCI_RHDA_DT		0x00000400  /* device type */
#define OHCI_RHDA_OCPM		0x00000800  /* overcurrent protection mode */
#define OHCI_RHDA_NOCP		0x00001000  /* no overcurrent protection */

#define OHCI_RHDA_POTPGT_MASK	0xff000000  /* power on to pwr good time */
#define OHCI_RHDA_POTPGT_ROT	24

/* macro to retrieve OHCI_RHDA_NDP (nbr of downstream ports) field */

#define OHCI_RHDA_NDP(x)    (((x) & OHCI_RHDA_NDP_MASK) >> OHCI_RHDA_NDP_ROT)

/* macro to retrieve OHCI_RHDA_POTPGT (pwr on to pwr good time) field */

#define OHCI_RHDA_POTPGT(x) (((x) & OHCI_RHDA_POTPGT_MASK) >> OHCI_RHDA_POTPGT_ROT)


/* OHCI_RH_DESCR_B register */

#define OHCI_RHDB_DR_MASK	0x0000ffff  /* device removable */
#define OHCI_RHDB_DR_ROT	0

#define OHCI_RHDB_PPCM_MASK	0xffff0000  /* port power control mask */
#define OHCI_RHDB_PPCM_ROT	0

/* macros to format/retrieve OHCI_RHDB_DR (device removable) field */

#define OHCI_RHDB_DR(x)     (((x) & OHCI_RHDB_DR_MASK) >> OHCI_RHDB_DR_ROT)
#define OHCI_RHDB_DR_FMT(x) (((x) << OHCI_RHDB_DR_ROT) & OHCI_RHDB_DR_MASK)

/* macros to format/retrieve OHCI_RHDB_PPCM (port pwr control mask) field */

#define OHCI_RHDB_PPCM(x)     (((x) & OHCI_RHDB_PPCM_MASK) >> OHCI_RHDB_PPCM_ROT)
#define OHCI_RHDB_PPCM_FMT(x) (((x) << OHCI_RHDB_PPCM_ROT) & OHCI_RHDB_PPCM_MASK)


/* OHCI_RH_STATUS register */

#define OHCI_RHS_LPS		0x00000001  /* local power status */
#define OHCI_RHS_CLR_GPWR	0x00000001  /* write '1' to clear global pwr */
#define OHCI_RHS_OCI		0x00000002  /* over current indicator */
#define OHCI_RHS_DRWE		0x00008000  /* device remote wakeup enable */
#define OHCI_RHS_SET_RWE	0x00008000  /* write '1' to set remote wakeup */
#define OHCI_RHS_LPSC		0x00010000  /* local power status change */
#define OHCI_RHS_SET_GPWR	0x00010000  /* write '1' to set global pwr */
#define OHCI_RHS_OCIC		0x00020000  /* over current indicator change */
#define OHCI_RHS_CRWE		0x80000000  /* clear remote wakeup enable */
#define OHCI_RHS_CLR_RWE	0x80000000  /* write '1' to clear wakeup enable */


/* OHCI_RH_PORT_STATUS register(s) */

#define OHCI_RHPS_CCS		0x00000001  /* current connect status */
#define OHCI_RHPS_CLR_PE	0x00000001  /* write '1' to clear port enable */
#define OHCI_RHPS_PES		0x00000002  /* port enable status */
#define OHCI_RHPS_SET_PE	0x00000002  /* write '1' to set port enable */
#define OHCI_RHPS_PSS		0x00000004  /* port suspend status */
#define OHCI_RHPS_SET_PS	0x00000004  /* write '1' to set port suspend */
#define OHCI_RHPS_POCI		0x00000008  /* port over current indicator */
#define OHCI_RHPS_CLR_PS	0x00000008  /* write '1' to clear port suspend */
#define OHCI_RHPS_PRS		0x00000010  /* port reset status */
#define OHCI_RHPS_PPS		0x00000100  /* port power status */
#define OHCI_RHPS_SET_PWR	0x00000100  /* write '1' to set port power */
#define OHCI_RHPS_LSDA		0x00000200  /* low speed device attached */
#define OHCI_RHPS_CLR_PWR	0x00000200  /* write '1' to clear port power */
#define OHCI_RHPS_CSC		0x00010000  /* connect status change */
#define OHCI_RHPS_PESC		0x00020000  /* port enable status change */
#define OHCI_RHPS_PSSC		0x00040000  /* port suspend status change */
#define OHCI_RHPS_OCIC		0x00080000  /* port over current indicator chg */
#define OHCI_RHPS_PRSC		0x00100000  /* port reset status change */


/* OHCI_ED - OHCI Endpoint Descriptor
 *
 * NOTE: OHCI_ED must be aligned to a 16-byte boundary,
 */

#define OHCI_ED_ALIGNMENT	16	    /* alignment */

typedef struct ohci_ed
    {
    UINT32 control;		    /* control word */
    UINT32 tdTail;		    /* TD queue tail pointer */
    UINT32 tdHead;		    /* TD queue head pointer */
    UINT32 nextEd;		    /* next OHCI_ED */
    } OHCI_ED, *pOHCI_ED;   

#define OHCI_ED_LEN		16  /* expected size of OHCI_ED */
#define OHCI_ED_ACTLEN		sizeof (OHCI_ED)    /* actual */


/* OHCI_ED.control definitions */

#define OHCI_EDCTL_FA_MASK	0x0000007f  /* function address */
#define OHCI_EDCTL_FA_ROT	0

#define OHCI_EDCTL_EN_MASK	0x00000780  /* endpoint number */
#define OHCI_EDCTL_EN_ROT	7

#define OHCI_EDCTL_DIR_MASK	0x00001800  /* direction */
#define OHCI_EDCTL_DIR_TD	0x00000000  /* get direction from TD */
#define OHCI_EDCTL_DIR_OUT	0x00000800  /* OUT */
#define OHCI_EDCTL_DIR_IN	0x00001000  /* get direction from TD */

#define OHCI_EDCTL_SPD_MASK	0x00002000  /* speed */
#define OHCI_EDCTL_SPD_FULL	0x00000000  /* full speed (12mbit) device */
#define OHCI_EDCTL_SPD_LOW	0x00002000  /* low speed device */

#define OHCI_EDCTL_SKIP 	0x00004000  /* skip ED */

#define OHCI_EDCTL_FMT_MASK	0x00008000  /* TD format */
#define OHCI_EDCTL_FMT_GEN	0x00000000  /* general: ctl/bulk/int */
#define OHCI_EDCTL_FMT_ISO	0x00008000  /* isochronous TDs */

#define OHCI_EDCTL_MPS_MASK	0x07ff0000  /* maximum packet size */
#define OHCI_EDCTL_MPS_ROT	16

/* macros to format/retrieve control.FA (function address) field */

#define OHCI_EDCTL_FA(x)     (((x) & OHCI_EDCTL_FA_MASK) >> OHCI_EDCTL_FA_ROT)
#define OHCI_EDCTL_FA_FMT(x) (((x) << OHCI_EDCTL_FA_ROT) & OHCI_EDCTL_FA_MASK)

/* macros to format/retrieve control.EN (endpoint number) field */

#define OHCI_EDCTL_EN(x)     (((x) & OHCI_EDCTL_EN_MASK) >> OHCI_EDCTL_EN_ROT)
#define OHCI_EDCTL_EN_FMT(x) (((x) << OHCI_EDCTL_EN_ROT) & OHCI_EDCTL_EN_MASK)

/* macros to format/retrieve control.MPS (max packet size) field */

#define OHCI_EDCTL_MPS(x)     (((x) & OHCI_EDCTL_MPS_MASK) >> OHCI_EDCTL_MPS_ROT)
#define OHCI_EDCTL_MPS_FMT(x) (((x) << OHCI_EDCTL_MPS_ROT) & OHCI_EDCTL_MPS_MASK)


/* Memory pointer definitions.
 *
 * NOTE: Some/all of these definitions apply to the OHCI_ED.tdTail, tdHead, and 
 * nextEd fields.
 */

#define OHCI_PTR_MEM_MASK	0xfffffff0  /* mask for memory pointer */
#define OHCI_PTR_HALTED 	0x00000001  /* indicates TD queue is halted */
#define OHCI_PTR_TGL_CARRY	0x00000002  /* toggle carry */

#define OHCI_PAGE_SIZE		0x00001000  /* 4k pages */
#define OHCI_PAGE_MASK		0xfffff000  /* page # mask */

#define OHCI_ISO_OFFSET_MASK	0x00000fff  /* mask for 4k page */
#define OHCI_ISO_OFFSET_BP0	0x00000000  /* offset within BufferPage0 page */
#define OHCI_ISO_OFFSET_BE	0x00001000  /* offset within BufferEnd page */


/* OHCI_TD_GEN - OHCI Transfer Descriptor - General: Ctl/Bulk/Int 
 *
 * NOTE: Must be aligned to 16 byte boundary.
 */

#define OHCI_TD_GEN_ALIGNMENT	16	    /* alignment */

typedef struct ohci_td_gen
    {
    UINT32 control;		    /* control word */
    UINT32 cbp; 		    /* current buffer pointer */
    UINT32 nextTd;		    /* next TD */
    UINT32 be;			    /* buffer end */
    } OHCI_TD_GEN, *pOHCI_TD_GEN;

#define OHCI_TD_GEN_LEN 	    16	/* expected size of OHCI_TD_ISO */
#define OHCI_TD_GEN_ACTLEN	    sizeof (OHCI_TD_GEN)    /* actual */


/* OHCI_TD_GEN.control definitions */

#define OHCI_TGCTL_BFR_RND	0x00040000  /* buffer rounding */

#define OHCI_TGCTL_PID_MASK	0x00180000  /* direction/PID */
#define OHCI_TGCTL_PID_SETUP	0x00000000  /* SETUP */
#define OHCI_TGCTL_PID_OUT	0x00080000  /* OUT */
#define OHCI_TGCTL_PID_IN	0x00100000  /* IN */

#define OHCI_TGCTL_DI_MASK	0x00e00000  /* delay interrupt */
#define OHCI_TGCTL_DI_ROT	21
#define OHCI_TGCTL_DI_NONE	7

#define OHCI_TGCTL_TOGGLE_DATA0 0x00000000  /* indicates DATA0 */
#define OHCI_TGCTL_TOGGLE_DATA1 0x01000000  /* indicates DATA1 */
#define OHCI_TGCTL_TOGGLE_USEED 0x00000000  /* use toggle from ED */
#define OHCI_TGCTL_TOGGLE_USETD 0x02000000  /* use toggle from TD */

#define OHCI_TGCTL_ERRCNT_MASK	0x0c000000  /* error count */

#define OHCI_TGCTL_CC_MASK	0xf0000000  /* condition code */
#define OHCI_TGCTL_CC_ROT	28

/* macros to format/retrieve control.DI (delay interrupt) field */

#define OHCI_TGCTL_DI(x)     (((x) & OHCI_TGCTL_DI_MASK) >> OHCI_TGCTL_DI_ROT)
#define OHCI_TGCTL_DI_FMT(x) (((x) << OHCI_TGCTL_DI_ROT) & OHCI_TGCTL_DI_MASK)

/* macro to retrieve control.CC (completion code) field */

#define OHCI_TGCTL_CC(x)    (((x) & OHCI_TGCTL_CC_MASK) >> OHCI_TGCTL_CC_ROT)


/* OHCI_TD_ISO - OHCI Transfer Descriptor - Isochronous 
 *
 * NOTE: Must be aligned to 32 byte boundary.
 */

#define OHCI_TD_ISO_ALIGNMENT	32	    /* alignment */
#define OHCI_ISO_PSW_CNT	8	    /* count of pkt status words */

typedef struct ohci_td_iso
    {
    UINT32 control;		    /* control word */
    UINT32 bp0; 		    /* buffer page 0 */
    UINT32 nextTd;		    /* next TD */
    UINT32 be;			    /* buffer end */
    UINT16 psw [OHCI_ISO_PSW_CNT];  /* array of packet status words */
    } OHCI_TD_ISO, *pOHCI_TD_ISO;

#define OHCI_TD_ISO_LEN 	32  /* expected size of OHCI_TD_ISO */
#define OHCI_TD_ISO_ACTLEN	sizeof (OHCI_TD_ISO)	/* actual */


/* OHCI_TD_ISO.control definitions */

#define OHCI_TICTL_SF_MASK	0x0000ffff  /* starting frame */
#define OHCI_TICTL_SF_ROT	0

#define OHCI_TICTL_DI_MASK	0x00e00000  /* delay interrupt */
#define OHCI_TICTL_DI_ROT	21
#define OHCI_TICTL_DI_NONE	7

#define OHCI_TICTL_FC_MASK	0x07000000  /* frame count */
#define OHCI_TICTL_FC_ROT	24
#define OHCI_FC_SIZE_MASK	0x7	    /* mask for 3-bit count */

#define OHCI_TICTL_CC_MASK	0xf0000000  /* condition code */
#define OHCI_TICTL_CC_ROT	28

/* macros to format/retrieve control.SF (starting frame) field */

#define OHCI_TICTL_SF(x)     (((x) & OHCI_TICTL_SF_MASK) >> OHCI_TICTL_SF_ROT)
#define OHCI_TICTL_SF_FMT(x) (((x) << OHCI_TICTL_SF_ROT) & OHCI_TICTL_SF_MASK)

/* Macros to retrieve/format control.FC (frame count) field
 *
 * NOTE: OHCI frame count is one less than the real frame count. The macros 
 * compensate automatically.
 */

#define OHCI_TICTL_FC(x)     ((((x) & OHCI_TICTL_FC_MASK) >> OHCI_TICTL_FC_ROT) + 1)
#define OHCI_TICTL_FC_FMT(x) ((((x) - 1) & OHCI_FC_SIZE_MASK) << OHCI_TICTL_FC_ROT)

/* macro to retrieve control.CC (completion code) field */

#define OHCI_TICTL_CC(x)    (((x) & OHCI_TICTL_CC_MASK) >> OHCI_TICTL_CC_ROT)


/* OHCI_TD_ISO.psw definitions */

#define OHCI_TIPSW_SIZE_MASK	0x07ff	    /* size of packet */
#define OHCI_TIPSW_SIZE_ROT	0

#define OHCI_TIPSW_CC_MASK	0xf000	    /* completion code */
#define OHCI_TIPSW_CC_ROT	12

/* macro to retrieve psw.size field */

#define OHCI_TIPSW_SIZE(x)  (((x) & OHCI_TIPSW_SIZE_MASK) >> OHCI_TIPSW_SIZE_ROT)

/* macro to retrieve psw.CC (completion code) field */

#define OHCI_TIPSW_CC(x)     (((x) & OHCI_TIPSW_CC_MASK) >> OHCI_TIPSW_CC_ROT)
#define OHCI_TIPSW_CC_FMT(x) (((x) << OHCI_TIPSW_CC_ROT) & OHCI_TIPSW_CC_MASK)


/* OHCI_HCCA - Host Controller Communications Area
 *
 * NOTE: This structure must be aligned to a 256 byte boundary.
 */

#define OHCI_HCCA_ALIGNMENT	256	    /* HCCA alignment requirement */
#define OHCI_INT_ED_COUNT	32	    /* count of interrupt ED heads */
#define OHCI_HCCA_HC_RESERVED	120	    /* count of reserved bytes */
#define OHCI_HCCA_LEN		256	    /* expected size of structure */

typedef struct ohci_hcca
    {
    UINT32 intEdTable [OHCI_INT_ED_COUNT];  /* ptrs to interrupt EDs */
    UINT16 frameNo;			    /* frame no in low 16-bits */
    UINT16 pad1;			    /* set to 0 when updating frameNo */
    UINT32 doneHead;			    /* ptr to first completed TD */
    UINT8 hcReserved [OHCI_HCCA_HC_RESERVED]; /* reserved by HC */
    } OHCI_HCCA, *pOHCI_HCCA;

#define OHCI_HCCA_LEN		256	    /* expected size of structure */
#define OHCI_HCCA_ACTLEN	sizeof (OHCI_HCCA)  /* actual size */


/* If the low bit of the doneHead field is written as '1' by the HC, then
 * an additional interrupt condition is present and the host controller driver
 * should interrogate the HcInterruptStatus register to determine the cause.
 */

#define OHCI_DONE_HEAD_OTHER_INT    0x00000001


#ifdef	__cplusplus
}
#endif

#endif	/* __INCusbOhcih */


/* End of file. */

