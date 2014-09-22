/* pciConstants.h - Defines constants related to the PCI bus. */

/* Copyright 2000 Wind River Systems, Inc. */

/*
Modification history
--------------------
01c,18sep01,wef  merge from wrs.tor2_0.usb1_1-f for veloce
01b,05dec00,wef  removed some macros that were defined in pciConfigLib.h
01a,29may99,rcb  First.
*/

#ifndef __INCpciConstantsh
#define __INCpciConstantsh

#ifdef	__cplusplus
extern "C" {
#endif

/* includes */

#include "drv/pci/pciConfigLib.h"

/* Constants */

#ifndef PCI_MAX_BUS
#define PCI_MAX_BUS	255	    /* Max number of PCI buses in system */
#endif	/* PCI_MAX_BUS */

#ifndef PCI_MAX_DEV
#define PCI_MAX_DEV	32	    /* Max number of PCI devices in system */
#endif	/* PCI_MAX_DEV */


/* Constants related to PCI configuration */

#define PCI_CFG_NUM_BASE_REG	6   /* Number of base address registers */
				    /* in PCI config. space for a device */


/* The following PCI_CFG_ADDRESS_xxxx constants qualify the address in each
of the PCI configuration base address registers. */

#define PCI_CFG_BASE_IO     0x00000001	/* Mask indicates adrs is I/O adrs */
#define PCI_CFG_BASE_MEM    0x00000000	/* Mask indicates adrs is mem adrs */

#define PCI_CFG_IOBASE_MASK	~(0x3)
#define PCI_CFG_MEMBASE_MASK	~(0xf)


/*
 * PCI_CFG_HEADER
 *
 * PCI_CFG_HEADER defines the invariant portion of a PCI devices configuration
 * space.  
 *
 * NOTE: If 0-byte packing is in effect, this structure will match the PCI
 * configuration header byte-for-byte.
 */

typedef struct pci_cfg_header
    {
    UINT16 vendorId;		/* PCI-assigned vendor ID */
    UINT16 deviceId;		/* Vendor-assigned device ID */
    UINT16 command;		/* Config. cmd register */
    UINT16 status;		/* Cfg status register */
    UINT8 revisionId;		/* Revision ID */
    UINT8 pgmIf;		/* Programming interface */
    UINT8 subClass;		/* PCI Sub-class */
    UINT8 pciClass;		/* PCI Class code */
    UINT8 cacheLineSize;	/* Cache line size */
    UINT8 latencyTimer; 	/* Latency timer */
    UINT8 headerType;		/* Header type */
    UINT8 bist; 		/* BIST */
    UINT32 baseReg [PCI_CFG_NUM_BASE_REG];  /* Base adrs registers */
    UINT32 reserved [2];
    UINT32 romBase;		/* Expansion ROM base address */
    UINT32 reserved1 [2];
    UINT8 intLine;		/* Interrupt line chosen by POST */
    UINT8 intPin;		/* HW int assignment (INTA = 1, etc) */
    UINT8 minGrant;		/* Minimum grant */
    UINT8 maxLatency;		/* Maximum latency */
    } PCI_CFG_HEADER, *pPCI_CFG_HEADER;

#ifdef	__cplusplus
}
#endif

#endif	/* __INCpciConstantsh */


/* End of file. */

