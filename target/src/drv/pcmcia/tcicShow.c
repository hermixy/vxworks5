/* tcicShow.c - Databook TCIC/2 PCMCIA host bus adaptor chip show library */

/* Copyright 1984-1996 Wind River Systems, Inc. */
/* Copyright (c) 1994 David A. Hinds -- All Rights Reserved */
#include "copyright_wrs.h"

/*
modification history
--------------------
01c,28mar96,jdi  doc: cleaned up language and format.
01b,22feb96,hdn  cleaned up
01a,04oct95,hdn  written based on David Hinds's version 2.2.3.
*/


/*
DESCRIPTION
This is a driver show routine for the Databook DB86082 PCMCIA chip.
tcicShow() is the only global function and is installed in the 
PCMCIA chip table `pcmciaAdapter' in pcmciaShowInit().
*/


#include "vxWorks.h"
#include "stdio.h"
#include "dllLib.h"
#include "drv/pcmcia/pcmciaLib.h"
#include "drv/pcmcia/tcic.h"


/* defines */

#define TCIC_GETB(reg)		sysInByte  (tcicBase+reg)
#define TCIC_GETW(reg)		sysInWord  (tcicBase+reg)
#define TCIC_SETB(reg, value)	sysOutByte (tcicBase+reg, value)
#define TCIC_SETW(reg, value)	sysOutWord (tcicBase+reg, value)


/* imports */

IMPORT PCMCIA_CTRL	pcmciaCtrl;
IMPORT int		tcicBase;


/* globals */



/* locals */



/* forward declarations */

LOCAL STATUS	tcicProbe	(void);
LOCAL void	tcicShowStatus	(int sock);
LOCAL void	tcicShowPower	(int sock);
LOCAL void	tcicShowIcsr	(int sock);
LOCAL void	tcicShowIena	(int sock);
LOCAL void	tcicShowScf1	(int sock);
LOCAL void	tcicShowScf2	(int sock);
LOCAL void	tcicShowMemwin	(int sock, int win);
LOCAL void	tcicShowIowin	(int sock, int win);
LOCAL void	tcicSetl	(int reg, long value);


/*******************************************************************************
*
* tcicShow - show all configurations of the TCIC chip
*
* This routine shows all configurations of the TCIC chip.
*
* RETURNS: N/A
*/

void tcicShow
    (
    int sock			/* socket no. */
    )
    {
    int ix;

    if (tcicProbe () != OK)
	return;

    tcicShowStatus (sock);
    tcicShowPower (sock);
    tcicShowIcsr (sock);
    tcicShowIena (sock);
    tcicShowScf1 (sock);
    tcicShowScf2 (sock);

    for (ix = 0; ix < TCIC_MEM_WINDOWS; ix++)
	tcicShowMemwin (sock, ix);

    for (ix = 0; ix < TCIC_IO_WINDOWS; ix++)
	tcicShowIowin (sock, ix);

    printf("\n");
    }

/*******************************************************************************
*
* tcicProbe - probe the TCIC chip
*
* This routine probes the TCIC chip.
*
* RETURNS: OK, or ERROR if it could not find the TCIC chip.
*/

LOCAL STATUS tcicProbe (void)
    {
    int sock;

    printf ("Databook TCIC/2 probe: ");
    sock = 0;

    TCIC_SETW (TCIC_ADDR, 0);
    if (TCIC_GETW (TCIC_ADDR) == 0)
	{
	TCIC_SETW (TCIC_ADDR, 0xc3a5);
	if (TCIC_GETW (TCIC_ADDR) == 0xc3a5)
	    sock = 2;
        }
    
    if (sock == 0)
	{
	printf ("not found.\n");
	return (ERROR);
        }
    
    printf ("%d sockets\n", sock);
    return (OK);
    }

/*******************************************************************************
*
* tcicShowStatus - show status of the TCIC chip
*
* This routine shows status of the TCIC chip.
*
* RETURNS: N/A
*/

LOCAL void tcicShowStatus
    (
    int sock			/* socket no. */
    )
    {
    char value;

    tcicSetl (TCIC_ADDR, (sock << TCIC_ADDR_SS_SHFT));
    value = TCIC_GETB (TCIC_SSTAT);
    printf("  Socket status = %#2.2x\n", value);
    printf("   ");
    if (value & TCIC_SSTAT_CD)
	printf(" [CD]");
    if (value & TCIC_SSTAT_WP)
	printf(" [WP]");
    if (value & TCIC_SSTAT_RDY)
	printf(" [RDY]");
    if (value & TCIC_SSTAT_LBAT1)
	printf(" [LBAT1]");
    if (value & TCIC_SSTAT_LBAT2)
	printf(" [LBAT2]");
    if (value & TCIC_SSTAT_PROGTIME)
	printf(" [PROGTIME]");
    if (value & TCIC_SSTAT_10US)
	printf(" [10us]");
    if (value & TCIC_SSTAT_6US)
	printf(" [6us]");
    printf("\n");
    }

/*******************************************************************************
*
* tcicShowPower - show power configuration of the TCIC chip
*
* This routine shows power configuration of the TCIC chip.
*
* RETURNS: N/A
*/

LOCAL void tcicShowPower
    (
    int sock			/* socket no. */
    )
    {
    char value = TCIC_GETB (TCIC_PWR);

    printf ("  Power control = %#2.2x\n", value);
    printf ("   ");
    printf (value & TCIC_PWR_VCC(sock) ? " [Vcc ON]" : " [Vcc OFF]");
    printf (value & TCIC_PWR_VPP(sock) ? " [Vpp ON]" : " [Vpp OFF]");
    if (value & TCIC_PWR_CLIMENA)
	printf (" [CLIMENA]");
    if (value & TCIC_PWR_CLIMSTAT)
	printf (" [CLIMSTAT]");
    printf ("\n");
    }

/*******************************************************************************
*
* tcicShowIcsr - show interrupt control/status of the TCIC chip
*
* This routine shows interrupt control/status of the TCIC chip.
*
* RETURNS: N/A
*/

LOCAL void tcicShowIcsr
    (
    int sock			/* socket no. */
    )
    {
    char value;

    tcicSetl (TCIC_ADDR, (sock << TCIC_ADDR_SS_SHFT));
    value = TCIC_GETB (TCIC_ICSR);
    printf ("  Interrupt control status = %#2.2x\n", value);
    printf ("   ");
    if (value & TCIC_ICSR_IOCHK)
	printf(" [IOCHK]");
    if (value & TCIC_ICSR_CDCHG)
	printf(" [CDCHG]");
    if (value & TCIC_ICSR_ERR)
	printf(" [ERR]");
    if (value & TCIC_ICSR_PROGTIME)
	printf(" [PROGTIME]");
    if (value & TCIC_ICSR_ILOCK)
	printf(" [ILOCK]");
    if (value & TCIC_ICSR_STOPCPU)
	printf(" [STOPCPU]");
    printf("\n");
    }

/*******************************************************************************
*
* tcicShowIena - show interrupt enable of the TCIC chip
*
* This routine shows interrupt enable of the TCIC chip.
*
* RETURNS: N/A
*/

LOCAL void tcicShowIena
    (
    int sock			/* socket no. */
    )
    {
    char value;

    tcicSetl (TCIC_ADDR, (sock << TCIC_ADDR_SS_SHFT));
    value = TCIC_GETB (TCIC_IENA);
    printf ("  Interrupt enable = %#2.2x\n", value);
    printf ("   ");
    switch (value & TCIC_IENA_CFG_MASK)
	{
        case TCIC_IENA_CFG_OFF:
	    printf(" [OFF]");
	    break;
        case TCIC_IENA_CFG_OD:
	    printf(" [OD]");
	    break;
        case TCIC_IENA_CFG_LOW:
	    printf(" [LOW]");
	    break;
        case TCIC_IENA_CFG_HIGH:
	    printf(" [HIGH]");
	    break;
        }
    if (value & TCIC_IENA_ILOCK)
	printf(" [ILOCK]");
    if (value & TCIC_IENA_PROGTIME)
	printf(" [PROGTIME]");
    if (value & TCIC_IENA_ERR)
	printf(" [ERR]");
    if (value & TCIC_IENA_CDCHG)
	printf(" [CDCHG]");
    printf("\n");
    }

/*******************************************************************************
*
* tcicShowScf1 - show socket configuration of the TCIC chip
*
* This routine shows socket configuration of the TCIC chip.
*
* RETURNS: N/A
*/

LOCAL void tcicShowScf1
    (
    int sock			/* socket no. */
    )
    {
    short value;

    tcicSetl (TCIC_ADDR, (sock << TCIC_ADDR_SS_SHFT)
	      | TCIC_ADDR_INDREG | TCIC_SCF1(sock));
    value = TCIC_GETW (TCIC_DATA);
    printf("  Socket config register 1: %#4.4x\n", value);
    printf("   ");
    if (value & TCIC_SCF1_HD7IDE)
	printf(" [HD7IDE]");
    if (value & TCIC_SCF1_DELWR)
	printf(" [DELWR]");
    if (value & TCIC_SCF1_FINPACK)
	printf(" [FINPACK]");
    if (value & TCIC_SCF1_SPKR)
	printf(" [SPKR]");
    if (value & TCIC_SCF1_IOSTS)
	printf(" [IOSTS]");
    if (value & TCIC_SCF1_ATA)
	printf(" [ATA]");
    if (value & TCIC_SCF1_IRDY)
	printf(" [IRDY]");
    if (value & TCIC_SCF1_PCVT)
	printf(" [PCVT]");
    if (value & TCIC_SCF1_IRQOC)
	printf(" [IRQOC]");
    printf(" [irq = %d]\n", value & TCIC_SCF1_IRQ_MASK);
    }

/*******************************************************************************
*
* tcicShowScf2 - show socket configuration of the TCIC chip
*
* This routine shows socket configuration of the TCIC chip.
*
* RETURNS: N/A
*/

LOCAL void tcicShowScf2
    (
    int sock			/* socket no. */
    )
    {
    short value;

    tcicSetl (TCIC_ADDR, (sock << TCIC_ADDR_SS_SHFT)
	      | TCIC_ADDR_INDREG | TCIC_SCF2(sock));
    value = TCIC_GETW (TCIC_DATA);
    printf ("  Socket config register 2: %#4.4x\n", value);
    printf ("   ");
    if (value & TCIC_SCF2_RI)
	printf(" [RI]");
    if (value & TCIC_SCF2_IDBR)
	printf(" [IDBR]");
    if (value & TCIC_SCF2_MDBR)
	printf(" [MDBR]");
    if (value & TCIC_SCF2_MLBAT1)
	printf(" [MLBAT1]");
    if (value & TCIC_SCF2_MLBAT2)
	printf(" [MLBAT2]");
    if (value & TCIC_SCF2_MRDY)
	printf(" [MRDY]");
    if (value & TCIC_SCF2_MWP)
	printf(" [MWP]");
    if (value & TCIC_SCF2_MCD)
	printf(" [MCD]");
    printf("\n");
    }

/*******************************************************************************
*
* tcicShowMemwin - show memory window of the TCIC chip
*
* This routine shows memory window of the TCIC chip.
*
* RETURNS: N/A
*/

LOCAL void tcicShowMemwin
    (
    int sock,			/* socket no. */
    int win			/* memory window no. */
    )
    {
    short base;
    short mmap;
    short ctl;

    printf ("  Memory window %d:", win);
    TCIC_SETW (TCIC_ADDR+2, TCIC_ADR2_INDREG | (sock << TCIC_SS_SHFT));
    
    TCIC_SETW (TCIC_ADDR, TCIC_MWIN(sock, win) + TCIC_MCTL_X);
    ctl = TCIC_GETW (TCIC_DATA);
    printf (ctl & TCIC_MCTL_ENA ? " [ON]" : " [OFF]");
    printf (ctl & TCIC_MCTL_B8 ? " [8 Bit]" : " [16 Bit]");
    if (ctl & TCIC_MCTL_QUIET)
	printf (" [QUIET]");
    if (ctl & TCIC_MCTL_WP)
	printf (" [WP]");
    if (ctl & TCIC_MCTL_ACC)
	printf (" [ACC]");
    if (ctl & TCIC_MCTL_KE)
	printf (" [KE]");
    if (ctl & TCIC_MCTL_EDC)
	printf (" [EDC]");
    printf (ctl & TCIC_MCTL_WCLK ? " [BCLK]" : " [CCLK]");
    printf (" [ws = %d]", ctl & TCIC_MCTL_WSCNT_MASK);
    
    TCIC_SETW (TCIC_ADDR, TCIC_MWIN(sock, win) + TCIC_MBASE_X);
    base = TCIC_GETW (TCIC_DATA);
    if (base & TCIC_MBASE_4K_BIT)
	printf (" [4K]");
    
    TCIC_SETW (TCIC_ADDR, TCIC_MWIN(sock, win) + TCIC_MMAP_X);
    mmap = TCIC_GETW (TCIC_DATA);
    if (mmap & TCIC_MMAP_REG)
	printf (" [REG]");

    printf ("\n    base = %#4.4x, mmap = %4.4x, ctl = %4.4x\n",
	   base, mmap, ctl);
    }

/*******************************************************************************
*
* tcicShowIowin - show IO window of the TCIC chip
*
* This routine shows IO window of the TCIC chip.
*
* RETURNS: N/A
*/

LOCAL void tcicShowIowin
    (
    int sock,			/* socket no. */
    int win			/* IO window no. */
    )
    {
    short base;
    short ctl;

    printf ("  IO window %d:", win);
    TCIC_SETW (TCIC_ADDR+2, TCIC_ADR2_INDREG | (sock << TCIC_SS_SHFT));
    
    TCIC_SETW (TCIC_ADDR, TCIC_IWIN(sock, win) + TCIC_IBASE_X);
    base = TCIC_GETW (TCIC_DATA);
    base &= (base-1);
    
    TCIC_SETW (TCIC_ADDR, TCIC_IWIN(sock, win) + TCIC_ICTL_X);
    ctl = TCIC_GETW (TCIC_DATA);
    printf (ctl & TCIC_ICTL_ENA ? " [ON]" : " [OFF]");
    if (ctl & TCIC_ICTL_1K)
	printf (" [1K]");
    if (ctl & TCIC_ICTL_PASS16)
	printf (" [PASS16]");
    if (ctl & TCIC_ICTL_ACC)
	printf (" [ACC]");
    if (ctl & TCIC_ICTL_TINY)
	printf (" [TINY]");
    switch (ctl & TCIC_ICTL_BW_MASK)
	{
        case TCIC_ICTL_BW_DYN:
	    printf (" [BW_DYN]");
	    break;
        case TCIC_ICTL_BW_8:
	    printf (" [BW_8]");
	    break;
        case TCIC_ICTL_BW_16:
	    printf (" [BW_16]");
	    break;
        case TCIC_ICTL_BW_ATA:
	    printf (" [BW_ATA]");
	    break;
        }

    printf (" [ws = %d]", ctl & TCIC_ICTL_WSCNT_MASK);
    printf ("\n    base = %#4.4x, ctl = %#4.4x\n", base, ctl);
    }

/*******************************************************************************
*
* tcicSetl - set long word value to the register
*
* This routine sets long word value to the register.
*
* RETURNS: N/A
*/

LOCAL void tcicSetl
    (
    int  reg,			/* register no. */
    long value			/* value to set */
    )
    {
    sysOutWord (tcicBase+reg, value & 0xffff);
    sysOutWord (tcicBase+reg+2, value >> 16);
    }

