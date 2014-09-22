/* tcic.c - Databook TCIC/2 PCMCIA host bus adaptor chip driver */

/* Copyright 1984-1996 Wind River Systems, Inc. */
/* Copyright (c) 1994 David A. Hinds -- All Rights Reserved */
#include "copyright_wrs.h"

/*
modification history
--------------------
01f,21jun00,rsh  upgrade to dosFs 2.0
01e,29mar96,jdi  doc: fixed error.
01d,28mar96,jdi  doc: cleaned up language and format.
01c,08mar96,hdn  added more descriptions.
01b,22feb96,hdn  cleaned up
01a,03oct95,hdn  written based on David Hinds's version 2.2.3.
*/


/*
DESCRIPTION
This library contains routines to manipulate the PCMCIA functions on the
Databook DB86082 PCMCIA chip.  

The initialization routine tcicInit() is the only global function and is
included in the PCMCIA chip table `pcmciaAdapter'.  If tcicInit() finds
the TCIC chip, it registers all function pointers of the PCMCIA_CHIP
structure.

*/


#include "vxWorks.h"
#include "taskLib.h"
#include "intLib.h"
#include "sysLib.h"
#include "drv/pcmcia/pcmciaLib.h"
#include "drv/pcmcia/tcic.h"


/* defines */

#define TCIC_GETB(reg)		sysInByte  (tcicBase+reg)
#define TCIC_GETW(reg)		sysInWord  (tcicBase+reg)
#define TCIC_SETB(reg, value)	sysOutByte (tcicBase+reg, value)
#define TCIC_SETW(reg, value)	sysOutWord (tcicBase+reg, value)


/* imports */

IMPORT PCMCIA_CTRL	pcmciaCtrl;


/* globals */

int	tcicBase;


/* locals */

LOCAL char	tcicCardStatus[TCIC_MAX_SOCKS];
LOCAL SEMAPHORE	tcicMuteSem;


/* forward declarations */

LOCAL STATUS	tcicReset	(int sock);
LOCAL int	tcicStatus	(int sock);
LOCAL int	tcicFlagGet	(int sock);
LOCAL STATUS	tcicFlagSet	(int sock, int flag);
LOCAL STATUS	tcicCscOn	(int sock, int irq);
LOCAL STATUS	tcicCscOff	(int sock, int irq);
LOCAL int	tcicCscPoll	(int sock);
LOCAL int	tcicIrqGet	(int sock);
LOCAL STATUS	tcicIrqSet	(int sock, int irq);
LOCAL STATUS	tcicIowinGet	(int sock, PCMCIA_IOWIN *io);
LOCAL STATUS	tcicIowinSet	(int sock, PCMCIA_IOWIN *io);
LOCAL STATUS	tcicMemwinGet	(int sock, PCMCIA_MEMWIN *mem);
LOCAL STATUS	tcicMemwinSet	(int sock, PCMCIA_MEMWIN *mem);
LOCAL char	tcicAuxGetb	(int reg);
LOCAL void	tcicAuxSetb	(int reg, char value);
#ifdef __unused__
LOCAL short	tcicAuxGetw	(int reg);
#endif
LOCAL void	tcicAuxSetw	(int reg, short value);
LOCAL long	tcicGetl	(int reg);
LOCAL void	tcicSetl	(int reg, long value);


/*******************************************************************************
*
* tcicInit - initialize the TCIC chip
*
* This routine initializes the TCIC chip.
*
* RETURNS: OK, or ERROR if the TCIC chip cannot be found.
*/

STATUS tcicInit
    (
    int     ioBase,		/* IO base address */
    int     intVec,		/* interrupt vector */
    int     intLevel,		/* interrupt level */
    FUNCPTR showRtn		/* show routine */
    )
    {
    PCMCIA_CTRL *pCtrl	= &pcmciaCtrl;
    PCMCIA_CHIP *pChip	= &pCtrl->chip;
    int sock		= 0;
    int ix;

    tcicBase = ioBase;

    TCIC_SETW (TCIC_ADDR, 0);
    if (TCIC_GETW (TCIC_ADDR) == 0)
	{
	TCIC_SETW (TCIC_ADDR, 0xc3a5);
	if (TCIC_GETW (TCIC_ADDR) == 0xc3a5)
	    sock = 2;
	}
    
    if (sock == 0)
	return (ERROR);
    
    semMInit (&tcicMuteSem, SEM_Q_PRIORITY | SEM_DELETE_SAFE | 
	      SEM_INVERSION_SAFE);
    semTake (&tcicMuteSem, WAIT_FOREVER);	/* mutual execlusion start */

    tcicAuxSetb (TCIC_AUX_WCTL, TCIC_WAIT_SENSE | TCIC_WAIT_ASYNC | 7);
    tcicAuxSetw (TCIC_AUX_SYSCFG, TCIC_SYSCFG_AUTOBUSY | 0x0a00);
    for (ix = 0; ix < sock; ix++)
	{
        tcicSetl (TCIC_ADDR, (sock << TCIC_ADDR_SS_SHFT) | 
		  TCIC_ADDR_INDREG | TCIC_SCF1(sock));
        tcicAuxSetb (TCIC_AUX_ILOCK, TCIC_ILOCK_HOLD_CCLK | TCIC_ILOCK_CWAIT |
		     TCIC_ILOCK_CRESENA);
        tcicCardStatus[sock] = TCIC_GETB (TCIC_SSTAT);
	}

    pChip->name		= "Databook TCIC2";
    pChip->reset	= (FUNCPTR) tcicReset;
    pChip->status	= (FUNCPTR) tcicStatus;
    pChip->flagGet	= (FUNCPTR) tcicFlagGet;
    pChip->flagSet	= (FUNCPTR) tcicFlagSet;
    pChip->cscOn	= (FUNCPTR) tcicCscOn;
    pChip->cscOff	= (FUNCPTR) tcicCscOff;
    pChip->cscPoll	= (FUNCPTR) tcicCscPoll;
    pChip->irqGet	= (FUNCPTR) tcicIrqGet;
    pChip->irqSet	= (FUNCPTR) tcicIrqSet;
    pChip->iowinGet	= (FUNCPTR) tcicIowinGet;
    pChip->iowinSet	= (FUNCPTR) tcicIowinSet;
    pChip->memwinGet	= (FUNCPTR) tcicMemwinGet;
    pChip->memwinSet	= (FUNCPTR) tcicMemwinSet;
    pChip->type		= PCMCIA_TCIC;
    pChip->socks	= sock;
    pChip->installed	= TRUE;
    pChip->intLevel	= intLevel;
    pChip->memWindows	= TCIC_MEM_WINDOWS;
    pChip->ioWindows	= TCIC_IO_WINDOWS;
    pChip->showRtn	= showRtn;

    semGive (&tcicMuteSem);		 	/* mutual execlusion stop */

    return (OK);
    }

/*******************************************************************************
*
* tcicStatus - Get status of the socket.
*
* This routine gets status of the socket.
* This routine can be called in interrupt level.
*
* RETURNS: The status of the socket.
*/

LOCAL int tcicStatus
    (
    int sock			/* socket no. */
    )
    {
    BOOL intMode = intContext ();

    int valueAddr = 0;

    char value;
    int status;

    if (intMode)
	valueAddr = tcicGetl (TCIC_ADDR);
    else
        semTake (&tcicMuteSem, WAIT_FOREVER);	/* mutual execlusion start */
	
    tcicSetl (TCIC_ADDR, (sock << TCIC_ADDR_SS_SHFT)
	      | TCIC_ADDR_INDREG | TCIC_SCF1(sock));

    value = TCIC_GETB (TCIC_SSTAT);

    status  = (value & TCIC_SSTAT_CD) ? PC_IS_CARD : PC_NO_CARD;
    status |= (value & TCIC_SSTAT_WP) ? PC_WRPROT : 0;

    if (TCIC_GETW (TCIC_DATA) & TCIC_SCF1_IOSTS)
	status |= (value & TCIC_SSTAT_LBAT1) ? PC_STSCHG : 0;
    else
	{
	status |= (value & TCIC_SSTAT_RDY) ? PC_READY : 0;
	status |= (value & TCIC_SSTAT_LBAT1) ? PC_BATDEAD : 0;
	status |= (value & TCIC_SSTAT_LBAT2) ? PC_BATWARN : 0;
        }

    value = TCIC_GETB (TCIC_PWR);
    if (value & (TCIC_PWR_VCC (sock) | TCIC_PWR_VPP (sock)))
	status |= PC_POWERON;

    if (intMode)
	tcicSetl (TCIC_ADDR, valueAddr);
    else
        semGive (&tcicMuteSem);		 	/* mutual execlusion stop */
	
    return (status);
    }
  
/*******************************************************************************
*
* tcicFlagGet - Get configuration flag from the socket.
*
* This routine gets configuration flag from the socket.
*
* RETURNS: The configuration flag of the socket.
*/

LOCAL int tcicFlagGet
    (
    int sock			/* socket no. */
    )
    {
    short value;
    int flag;

    semTake (&tcicMuteSem, WAIT_FOREVER);	/* mutual execlusion start */

    tcicSetl (TCIC_ADDR, (sock << TCIC_ADDR_SS_SHFT) |
	      TCIC_ADDR_INDREG | TCIC_SCF1(sock));

    flag = (TCIC_GETW (TCIC_DATA) & TCIC_SCF1_IOSTS) ? PC_IOCARD : 0;

    value = TCIC_GETB (TCIC_PWR);
    if (value & TCIC_PWR_VCC (sock))
	{
	if (value & TCIC_PWR_VPP (sock))
	    flag |= PC_VCC_5V;
	else
	    flag |= PC_VCC_5V | PC_VPP_5V;
	}
    else
	if (value & TCIC_PWR_VPP (sock))
	    flag |= PC_VCC_5V | PC_VPP_12V;

    value = tcicAuxGetb (TCIC_AUX_ILOCK);
    flag |= (value & TCIC_ILOCK_CRESET) ? PC_RESET : 0;
    
    semGive (&tcicMuteSem);		 	/* mutual execlusion stop */

    return (flag);
    }

/*******************************************************************************
*
* tcicFlagSet - Set configuration flag into the socket.
*
* This routine sets configuration flag into the socket.
*
* RETURNS: OK (always).
*/

LOCAL STATUS tcicFlagSet
    (
    int sock,			/* socket no. */
    int flag			/* configuration flag */
    )
    {
    char value;
    short scf1;
    short scf2;

    semTake (&tcicMuteSem, WAIT_FOREVER);	/* mutual execlusion start */

    TCIC_SETW (TCIC_ADDR+2, (sock << TCIC_SS_SHFT) | TCIC_ADR2_INDREG);

    value = TCIC_GETB (TCIC_PWR);
    value &= ~(TCIC_PWR_VCC (sock) | TCIC_PWR_VPP (sock));
    if (flag & PC_VCC_5V)
	{
	if (flag & PC_VPP_12V)
	    value |= TCIC_PWR_VPP (sock);
	else if (flag & PC_VPP_5V)
	    value |= TCIC_PWR_VCC (sock);
	else
	    value |= TCIC_PWR_VCC (sock) | TCIC_PWR_VPP (sock);
        }
    TCIC_SETB (TCIC_PWR, value);
    taskDelay (sysClkRateGet () >> 4);

    TCIC_SETW (TCIC_ADDR, TCIC_SCF1 (sock));
    scf1 = TCIC_GETW (TCIC_DATA);
    TCIC_SETW (TCIC_ADDR, TCIC_SCF2 (sock));
    scf2 = TCIC_GETW (TCIC_DATA);
    if (flag & PC_IOCARD) 
	{
	scf1 |= TCIC_SCF1_IOSTS | TCIC_SCF1_SPKR;
	scf2 |= TCIC_SCF2_MRDY;
        }
    else
	{
	scf1 &= ~(TCIC_SCF1_IOSTS | TCIC_SCF1_SPKR);
	scf2 &= ~TCIC_SCF2_MRDY;
        }
    TCIC_SETW (TCIC_ADDR, TCIC_SCF1 (sock));
    TCIC_SETW (TCIC_DATA, scf1);
    TCIC_SETW (TCIC_ADDR, TCIC_SCF2 (sock));
    TCIC_SETW (TCIC_DATA, scf2);

    if (flag & PC_VCC_MASK)
	TCIC_SETB (TCIC_SCTRL, TCIC_SCTRL_ENA);

    value = tcicAuxGetb (TCIC_AUX_ILOCK) & ~TCIC_ILOCK_CRESET;
    if (flag & PC_RESET)
	value |= TCIC_ILOCK_CRESET;
    tcicAuxSetb (TCIC_AUX_ILOCK, value);
    
    semGive (&tcicMuteSem);		 	/* mutual execlusion stop */

    return (OK);
    }
  
/*******************************************************************************
*
* tcicCscOn - Enables card status change interrupt.
*
* This routine enables card status change interrupt.
*
* RETURNS: OK (always).
*/

LOCAL STATUS tcicCscOn
    (
    int sock,			/* socket no. */
    int irq			/* IRQ for CSC */
    )
    {
    char value;
    short scf2;

    semTake (&tcicMuteSem, WAIT_FOREVER);	/* mutual execlusion start */

    /* Turn on all card status change interrupts */

    tcicSetl (TCIC_ADDR, (sock << TCIC_ADDR_SS_SHFT) | 
	      TCIC_ADDR_INDREG | TCIC_SCF2(sock));

    value = tcicAuxGetb (TCIC_AUX_SYSCFG);
    tcicAuxSetb (TCIC_AUX_SYSCFG, value | irq);

    TCIC_SETB (TCIC_IENA, TCIC_IENA_CDCHG | TCIC_IENA_CFG_HIGH);

    scf2 = TCIC_GETW (TCIC_DATA);
    scf2 &= TCIC_SCF2_MDBR | TCIC_SCF2_IDBR | TCIC_SCF2_RI;
    TCIC_SETW (TCIC_DATA, scf2);

    /* flush current card status changes */

    TCIC_SETB (TCIC_ICSR, TCIC_ICSR_CLEAR);

    semGive (&tcicMuteSem);		 	/* mutual execlusion stop */

    return (OK);
    }

/*******************************************************************************
*
* tcicCscOff - Disable card status change interrupt.
*
* This routine disables card status change interrupt.
*
* RETURNS: OK (always).
*/

LOCAL STATUS tcicCscOff
    (
    int sock,			/* socket no. */
    int irq			/* IRQ for CSC */
    )
    {

    semTake (&tcicMuteSem, WAIT_FOREVER);	/* mutual execlusion start */

    TCIC_SETW (TCIC_ADDR+2, (sock << TCIC_SS_SHFT));
    TCIC_SETB (TCIC_IENA, TCIC_IENA_CFG_OFF);

    semGive (&tcicMuteSem);		 	/* mutual execlusion stop */

    return (OK);
    }

/*******************************************************************************
*
* tcicCscPoll - Get the card status by polling the register.
*
* This routine gets the card status by polling the register.
* This routine can be called in interrupt level.
*
* RETURNS: The card status of the socket.
*/

LOCAL int tcicCscPoll
    (
    int sock			/* socket no. */
    )
    {
    char latch	 = 0;
    BOOL intMode = intContext ();
    char value;
    int  status;
    int  ix;
    int  valueAddr = 0;
    
    if (intMode)
	valueAddr = tcicGetl (TCIC_ADDR);
    else
        semTake (&tcicMuteSem, WAIT_FOREVER);	/* mutual execlusion start */
	
    tcicSetl (TCIC_ADDR, (sock << TCIC_ADDR_SS_SHFT) |
	      TCIC_ADDR_INDREG | TCIC_SCF1(sock));

    for (ix = 0; ix < 10; ix++)
	{
	/* What bits have changed state? */

	value = TCIC_GETB (TCIC_SSTAT);
	latch |= value ^ tcicCardStatus[sock];
	tcicCardStatus[sock] = value;

	/* Try to reset interrupt status register */

	if (TCIC_GETB (TCIC_ICSR) != 0)
	    TCIC_SETB (TCIC_ICSR, TCIC_ICSR_CLEAR);
	else
	    break;
        }

    status  = (latch & TCIC_SSTAT_CD) ? PC_DETECT : 0;
    status |= (latch & TCIC_SSTAT_WP) ? PC_WRPROT : 0;
    if (TCIC_GETW (TCIC_DATA) & TCIC_SCF1_IOSTS)
	status |= (latch & TCIC_SSTAT_LBAT1) ? PC_STSCHG : 0;
    else
	{
	status |= (latch & TCIC_SSTAT_RDY)   ? PC_READY : 0;
	status |= (latch & TCIC_SSTAT_LBAT1) ? PC_BATDEAD : 0;
	status |= (latch & TCIC_SSTAT_LBAT2) ? PC_BATWARN : 0;
        }

    if (intMode)
	tcicSetl (TCIC_ADDR, valueAddr);
    else
        semGive (&tcicMuteSem);		 	/* mutual execlusion stop */
	
    return (status);
    }

/*******************************************************************************
*
* tcicIrqGet - Get IRQ level of the socket.
*
* This routine gets IRQ level of the socket.
*
* RETURNS: The IRQ level of the socket.
*/

LOCAL int tcicIrqGet
    (
    int sock			/* socket no. */
    )
    {
    int value;

    semTake (&tcicMuteSem, WAIT_FOREVER);	/* mutual execlusion start */

    tcicSetl (TCIC_ADDR, (sock << TCIC_ADDR_SS_SHFT) |
	      TCIC_ADDR_INDREG | TCIC_SCF1(sock));
    value = TCIC_GETW (TCIC_DATA) & 0x0f;

    semGive (&tcicMuteSem);		 	/* mutual execlusion stop */

    return (value);
    }
  
/*******************************************************************************
*
* tcicIrqSet - Set IRQ level of the socket.
*
* This routine sets IRQ level of the socket.
*
* RETURNS: OK, or ERROR if the IRQ level is greater than 15.
*/

LOCAL STATUS tcicIrqSet
    (
    int sock,			/* socket no. */
    int irq			/* IRQ for PC card */
    )
    {
    short scf1;
    
    if (irq > 15)
	return (ERROR);

    semTake (&tcicMuteSem, WAIT_FOREVER);	/* mutual execlusion start */

    tcicSetl (TCIC_ADDR, (sock << TCIC_ADDR_SS_SHFT) |
	      TCIC_ADDR_INDREG | TCIC_SCF1(sock));
    
    scf1 = TCIC_GETW (TCIC_DATA);
    scf1 &= ~(TCIC_SCF1_IRQOC | TCIC_SCF1_IRQ_MASK);
    scf1 |= irq;
    TCIC_SETW (TCIC_DATA, scf1);
    
    semGive (&tcicMuteSem);		 	/* mutual execlusion stop */

    return (OK);
    }

/*******************************************************************************
*
* tcicReset - Reset a card in the socket.
*
* This routine resets a card in the socket.
*
* RETURNS: OK (always).
*/

LOCAL STATUS tcicReset
    (
    int sock			/* socket no. */
    )
    {
    int ix;
    short value;
    
    semTake (&tcicMuteSem, WAIT_FOREVER);	/* mutual execlusion start */

    /* Turn off interrupts, select memory-only interface */

    tcicSetl (TCIC_ADDR, (sock << TCIC_ADDR_SS_SHFT) |
	      TCIC_ADDR_INDREG | TCIC_SCF1(sock));
    value = TCIC_GETW (TCIC_DATA);
    value &= ~(TCIC_SCF1_IRQ_MASK | TCIC_SCF1_IOSTS);
    TCIC_SETW (TCIC_DATA, value);
    
    /* Reset event mask, since we've turned off I/O mode */

    TCIC_SETW (TCIC_ADDR, TCIC_SCF2(sock));
    value = TCIC_GETW (TCIC_DATA);
    value &= TCIC_SCF2_MDBR | TCIC_SCF2_IDBR | TCIC_SCF2_RI;
    TCIC_SETW (TCIC_DATA, value);
    
    /* Turn off I/O windows */

    for (ix = 0; ix < 2; ix++)
	{
	TCIC_SETW (TCIC_ADDR, TCIC_IWIN(sock, ix) + TCIC_ICTL_X);
	TCIC_SETW (TCIC_DATA, 0);
        }
    
    /* Turn off memory windows */

    for (ix = 0; ix < 4; ix++)
	{
	TCIC_SETW (TCIC_ADDR, TCIC_MWIN(sock, ix) + TCIC_MCTL_X);
	TCIC_SETW (TCIC_DATA, 0);
        }
    
    semGive (&tcicMuteSem);		 	/* mutual execlusion stop */

    return (OK);
    }

/*******************************************************************************
*
* tcicIowinGet - Get the IO window of the socket.
*
* This routine gets the IO window of the socket.
*
* RETURNS: OK, or ERROR if the window number is greater than max windows.
*/

LOCAL STATUS tcicIowinGet
    (
    int		 sock,		/* socket no. */
    PCMCIA_IOWIN *io		/* IO window structure to get */
    )
    {
    short base;
    short ioctl;
    long  addr;
    
    if (io->window >= TCIC_IO_WINDOWS)
	return (ERROR);

    semTake (&tcicMuteSem, WAIT_FOREVER);	/* mutual execlusion start */

    TCIC_SETW (TCIC_ADDR+2, TCIC_ADR2_INDREG | (sock << TCIC_SS_SHFT));
    addr = TCIC_IWIN(sock, io->window);
    TCIC_SETW (TCIC_ADDR, addr + TCIC_IBASE_X);
    base = TCIC_GETW (TCIC_DATA);
    TCIC_SETW (TCIC_ADDR, addr + TCIC_ICTL_X);
    ioctl = TCIC_GETW (TCIC_DATA);

    if (ioctl & TCIC_ICTL_TINY)
	io->start = io->stop = base;
    else
	{
	io->start = base & (base-1);
	io->stop = io->start + (base ^ (base-1));
        }
    io->extraws	= ioctl & TCIC_ICTL_WSCNT_MASK;
    io->flags	= (ioctl & TCIC_ICTL_ENA) ? MAP_ACTIVE : 0;
    if ((ioctl & TCIC_ICTL_BW_MASK) == TCIC_ICTL_BW_16)
	io->flags |= MAP_16BIT;

    semGive (&tcicMuteSem);		 	/* mutual execlusion stop */

    return (OK);
    }

/*******************************************************************************
*
* tcicIowinSet - Set the IO window of the socket.
*
* This routine sets the IO window of the socket.
*
* RETURNS: OK, or ERROR if there is an error.
*/

LOCAL STATUS tcicIowinSet
    (
    int		 sock,		/* socket no. */
    PCMCIA_IOWIN *io		/* IO window structure to set */
    )
    {
    long addr;
    int base;
    int len;
    int ix;
    short ioctl;
    
    if ((io->window >= TCIC_IO_WINDOWS) || (io->start > 0xffff) ||
	(io->stop > 0xffff) || (io->stop < io->start))
	return (ERROR);

    semTake (&tcicMuteSem, WAIT_FOREVER);	/* mutual execlusion start */

    TCIC_SETW (TCIC_ADDR+2, TCIC_ADR2_INDREG | (sock << TCIC_SS_SHFT));
    addr = TCIC_IWIN(sock, io->window);
    base = io->start;
    len  = io->stop - io->start + 1;

    /* adject base address according to the length */

    for (ix = 0; ix < 16; ix++)
	if ((1 << ix) >= len)
	    break;
    if (len != 1)
	{
	base &= ~((1 << ix) - 1);
	base += 1 << (ix - 1);
	}

    TCIC_SETW (TCIC_ADDR, addr + TCIC_IBASE_X);
    TCIC_SETW (TCIC_DATA, base);
    
    ioctl  = TCIC_ICTL_QUIET | (sock << TCIC_ICTL_SS_SHFT);
    ioctl |= (len == 0) ? TCIC_ICTL_TINY : 0;
    ioctl |= (io->flags & MAP_ACTIVE) ? TCIC_ICTL_ENA : 0;
    ioctl |= io->extraws & TCIC_ICTL_WSCNT_MASK;
    ioctl |= (io->flags & MAP_16BIT) ? TCIC_ICTL_BW_16 : TCIC_ICTL_BW_8;
    TCIC_SETW (TCIC_ADDR, addr + TCIC_ICTL_X);
    TCIC_SETW (TCIC_DATA, ioctl);
    
    semGive (&tcicMuteSem);		 	/* mutual execlusion stop */

    return (OK);
    }

/*******************************************************************************
*
* tcicMemwinGet - Get the memory window of the socket.
*
* This routine gets the memory window of the socket.
*
* RETURNS: OK, or ERROR if the window number is greater than max windows.
*/

LOCAL STATUS tcicMemwinGet
    (
    int		  sock,		/* socket no. */
    PCMCIA_MEMWIN *mem		/* memory window structure to get */
    )
    {
    long base;
    long mmap;
    short addr;
    short ctl;
    
    if (mem->window >= TCIC_MEM_WINDOWS)
	return (ERROR);

    semTake (&tcicMuteSem, WAIT_FOREVER);	/* mutual execlusion start */

    TCIC_SETW (TCIC_ADDR+2, TCIC_ADR2_INDREG | (sock << TCIC_SS_SHFT));
    addr = TCIC_MWIN(sock, mem->window);
    
    TCIC_SETW (TCIC_ADDR, addr + TCIC_MBASE_X);
    base = TCIC_GETW (TCIC_DATA);
    if (base & TCIC_MBASE_4K_BIT)
	{
	mem->start = base & TCIC_MBASE_HA_MASK;
	mem->stop  = mem->start;
        }
    else
	{
	base &= TCIC_MBASE_HA_MASK;
	mem->start = (base & (base-1));
	mem->stop  = mem->start + (base ^ (base-1));
        }
    mem->start = mem->start << TCIC_MBASE_HA_SHFT;
    mem->stop  = (mem->stop << TCIC_MBASE_HA_SHFT) + 0x0fff;
    
    TCIC_SETW (TCIC_ADDR, addr + TCIC_MMAP_X);
    mmap = TCIC_GETW(TCIC_DATA);
    mem->flags = (mmap & TCIC_MMAP_REG) ? MAP_ATTRIB : 0;
    mmap = (base + mmap) & TCIC_MMAP_CA_MASK;
    mem->cardstart = mmap << TCIC_MMAP_CA_SHFT;
    
    TCIC_SETW (TCIC_ADDR, addr + TCIC_MCTL_X);
    ctl = TCIC_GETW (TCIC_DATA);
    mem->flags	|= (ctl & TCIC_MCTL_ENA) ? MAP_ACTIVE : 0;
    mem->flags	|= (ctl & TCIC_MCTL_B8) ? 0 : MAP_16BIT;
    mem->flags	|= (ctl & TCIC_MCTL_WP) ? MAP_WRPROT : 0;
    mem->extraws = (ctl & TCIC_MCTL_WSCNT_MASK);
    
    semGive (&tcicMuteSem);		 	/* mutual execlusion stop */

    return (OK);
    }

/*******************************************************************************
*
* tcicMemwinSet - Set the memory window of the socket.
*
* This routine sets the memory window of the socket.
*
* RETURNS: OK, or ERROR if there is an error.
*/

LOCAL STATUS tcicMemwinSet
    (
    int		  sock,		/* socket no. */
    PCMCIA_MEMWIN *mem		/* memory window structure to set */
    )
    {
    short addr;
    short ctl;
    long base;
    long len;
    long mmap;
    
    if ((mem->window >= TCIC_MEM_WINDOWS) || (mem->cardstart > 0xffffff) ||
	(mem->start > 0xffffff) || (mem->stop > 0xffffff) ||
	(mem->start > mem->stop) || (mem->extraws > 7))
	return (ERROR);

    semTake (&tcicMuteSem, WAIT_FOREVER);	/* mutual execlusion start */

    TCIC_SETW (TCIC_ADDR+2, TCIC_ADR2_INDREG | (sock << TCIC_SS_SHFT));
    addr = TCIC_MWIN(sock, mem->window);

    base = mem->start & ~0xfff;
    len  = (mem->stop - base) | 0xfff;

    if (len == 0x0fff)
	base = (base >> TCIC_MBASE_HA_SHFT) | TCIC_MBASE_4K_BIT;
    else
	base = (base | (len+1)>>1) >> TCIC_MBASE_HA_SHFT;
    TCIC_SETW (TCIC_ADDR, addr + TCIC_MBASE_X);
    TCIC_SETW (TCIC_DATA, base);
    
    mmap = (mem->cardstart >> TCIC_MMAP_CA_SHFT) - base;
    mmap &= TCIC_MMAP_CA_MASK;
    if (mem->flags & MAP_ATTRIB)
	mmap |= TCIC_MMAP_REG;
    TCIC_SETW (TCIC_ADDR, addr + TCIC_MMAP_X);
    TCIC_SETW (TCIC_DATA, mmap);

    ctl  = TCIC_MCTL_QUIET | (sock << TCIC_MCTL_SS_SHFT);
    ctl |= mem->extraws & TCIC_MCTL_WSCNT_MASK;
    ctl |= (mem->flags & MAP_16BIT) ? 0 : TCIC_MCTL_B8;
    ctl |= (mem->flags & MAP_WRPROT) ? TCIC_MCTL_WP : 0;
    ctl |= (mem->flags & MAP_ACTIVE) ? TCIC_MCTL_ENA : 0;
    TCIC_SETW (TCIC_ADDR, addr + TCIC_MCTL_X);
    TCIC_SETW (TCIC_DATA, ctl);
    
    semGive (&tcicMuteSem);		 	/* mutual execlusion stop */

    return (OK);
    }

/*******************************************************************************
*
* tcicSetl - Set a long word value to the register.
*
* This routine sets a long word value to the register.
*
* RETURNS: N/A
*/

LOCAL void tcicSetl
    (
    int  reg,			/* register no. */
    long value			/* value to set */
    )
    {
    sysOutWord (tcicBase+reg,   value & 0xffff);
    sysOutWord (tcicBase+reg+2, value >> 16);
    }

/*******************************************************************************
*
* tcicGetl - Get a long word value from the register.
*
* This routine gets a long word value from the register.
*
* RETURNS: A long word value of the register.
*/

LOCAL long tcicGetl
    (
    int  reg			/* register no. */
    )
    {
    long value0;
    long value1;

    value0 = sysInWord (tcicBase+reg);
    value1 = sysInWord (tcicBase+reg+2);

    return ((value1 << 16) | (value0 & 0xffff));
    }

/*******************************************************************************
*
* tcicAuxGetb - Get a byte value from the aux register.
*
* This routine gets a byte value from the aux register.
*
* RETURNS: A byte value of the register.
*/

LOCAL char tcicAuxGetb
    (
    int reg			/* register no. */
    )
    {
    char mode = (TCIC_GETB (TCIC_MODE) & TCIC_MODE_PGMMASK) | reg;

    TCIC_SETB (TCIC_MODE, mode);
    return (TCIC_GETB (TCIC_AUX));
    }

/*******************************************************************************
*
* tcicAuxSetb - Set a byte value to the aux register.
*
* This routine sets a byte value to the aux register.
*
* RETURNS: N/A
*/

LOCAL void tcicAuxSetb
    (
    int  reg,			/* register no. */
    char value			/* value to set */
    )
    {
    char mode = (TCIC_GETB (TCIC_MODE) & TCIC_MODE_PGMMASK) | reg;

    TCIC_SETB (TCIC_MODE, mode);
    TCIC_SETB (TCIC_AUX, value);
    }
#ifdef __unused__

/*******************************************************************************
*
* tcicAuxGetw - Get a word value from the aux register.
*
* This routine gets a word value from the aux register.
*
* RETURNS: A word value of the register.
*/

LOCAL short tcicAuxGetw
    (
    int reg			/* register no. */
    )
    {
    char mode = (TCIC_GETB (TCIC_MODE) & TCIC_MODE_PGMMASK) | reg;

    TCIC_SETB (TCIC_MODE, mode);
    return (TCIC_GETW (TCIC_AUX));
    }
#endif

/*******************************************************************************
*
* tcicAuxSetw - Set a word value to the aux register.
*
* This routine sets a word value to the aux register.
*
* RETURNS: N/A
*/

LOCAL void tcicAuxSetw
    (
    int   reg,			/* register no. */
    short value			/* value to set */
    )
    {
    char mode = (TCIC_GETB (TCIC_MODE) & TCIC_MODE_PGMMASK) | reg;

    TCIC_SETB (TCIC_MODE, mode);
    TCIC_SETW (TCIC_AUX, value);
    }

