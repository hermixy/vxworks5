/* pcicShow.c - Intel 82365SL PCMCIA host bus adaptor chip show library */

/* Copyright 1984-1996 Wind River Systems, Inc. */
/* Copyright (c) 1994 David A. Hinds -- All Rights Reserved */
#include "copyright_wrs.h"

/*
modification history
--------------------
01d,01nov96,hdn  fixed PCIC_PORT().
01c,28mar96,jdi  doc: cleaned up language and format.
01b,22feb96,hdn  cleaned up
01a,15feb95,hdn  written based on David Hinds's version 2.2.3.
*/


/*
DESCRIPTION
This is a driver show routine for the Intel 82365 series PCMCIA chip.
pcicShow() is the only global function and is installed in the 
PCMCIA chip table `pcmciaAdapter' in pcmciaShowInit().
*/


#include "vxWorks.h"
#include "stdio.h"
#include "dllLib.h"
#include "drv/pcmcia/pcmciaLib.h"
#include "drv/pcmcia/pcic.h"
#include "drv/pcmcia/pd67.h"


/* defines */

#define PCIC_PORT(slot)		(pcicBase + (2*((slot) >> 2)))
#define PCIC_REG(slot, reg)	((((slot) & 3) * 0x40) + reg)


/* imports */

IMPORT PCMCIA_CTRL	pcmciaCtrl;
IMPORT int		pcicBase;


/* globals */



/* locals */

LOCAL BOOL cirrus = FALSE;


/* forward declarations */

LOCAL STATUS	pcicProbe	(void);
LOCAL void	pcicShowStatus	(int sock);
LOCAL void	pcicShowPower	(int sock);
LOCAL void	pcicShowIntctl	(int sock);
LOCAL void	pcicShowCsc	(int sock);
LOCAL void	pcicShowCscint	(int sock);
LOCAL void	pcicShowGenctl	(int sock);
LOCAL void	pcicShowGblctl	(int sock);
LOCAL void	pcicShowMisc	(int sock);
LOCAL void	pcicShowTime	(char *pName, int value);
LOCAL void	pcicShowTiming	(int sock, int reg);
LOCAL void	pcicShowMemwin	(int sock, int win);
LOCAL void	pcicShowIowin	(int sock, int win);
LOCAL int	pcicGet		(int sock, int reg);
LOCAL int	pcicGet2	(int sock, int reg);
LOCAL void	pcicSet		(int sock, int reg, char value);


/*******************************************************************************
*
* pcicShow - show all configurations of the PCIC chip
*
* This routine shows all configurations of the PCIC chip.
*
* RETURNS: N/A
*/

void pcicShow
    (
    int sock			/* socket no. */
    )
    {
    int ix;

    if (pcicProbe () != OK)
	return;

    printf("Identification and revision = 0x%2.2x\n",
	   pcicGet (sock, PCIC_IDENT));

    if (cirrus)
	printf("Chip information = 0x%2.2x\n", pcicGet (sock, PD67_CHIP_INFO));

    pcicShowStatus (sock);
    pcicShowPower  (sock);
    pcicShowIntctl (sock);
    pcicShowCsc    (sock);
    pcicShowCscint (sock);
 
    if (cirrus)
	pcicShowMisc (sock);
    else
	{
	pcicShowGenctl (sock);
	pcicShowGblctl (sock);
        }

    for (ix = 0; ix < PCIC_MEM_WINDOWS; ix++)
	pcicShowMemwin (sock, ix);
    for (ix = 0; ix < PCIC_IO_WINDOWS; ix++)
	pcicShowIowin (sock, ix);

    if (cirrus)
	for (ix = 0; ix < 2; ix++)
	    pcicShowTiming (sock, ix);

    printf ("\n");
    }

/*******************************************************************************
*
* pcicProbe - Probe the PCIC chip.
*
* This routine probes the PCIC chip.
*
* RETURNS: OK, or ERROR if it could not find the PCIC chip.
*/

LOCAL STATUS pcicProbe (void)
    {
    char *name	= "i82365sl";
    int id	= 0;
    BOOL done	= FALSE;
    int sock;
    int value;

    printf ("Intel PCIC probe: ");
    
    for (sock = 0; sock < PCIC_MAX_SOCKS; sock++)
	{
	value = pcicGet (sock, PCIC_IDENT);
	switch (value)
	    {
	    case 0x82:
	    case 0x83:
		name = "INTEL 82365SL";
		break;

	    case 0x84:
		name = "VLSI 82C146";
		break;

	    case 0x88:
	    case 0x89:
		name = "IBM Clone";
		break;

	    case 0x8b:
		name = "VADEM Clone";
		break;

	    default:
		done = TRUE;
	    }
	id = value & 7;
	if (done)
	    break;
        }
    
    if (sock == 0)
	{
	printf ("not found:  value=0x%x.\n", value);
	return (ERROR);
        }
    
    /* Check for Cirrus CL-PD67xx chips */

    pcicSet (0, PD67_CHIP_INFO, 0);
    value = pcicGet (0, PD67_CHIP_INFO);
    if ((value & PD67_INFO_CHIP_ID) == PD67_INFO_CHIP_ID)
	{
	value = pcicGet (0, PD67_CHIP_INFO);
	if ((value & PD67_INFO_CHIP_ID) == 0)
	    {
	    cirrus = TRUE;
	    if (value & PD67_INFO_SLOTS)
		name = "Cirrus CL-PD672x";
	    else
		{
		name = "Cirrus CL-PD6710";
		sock = 1;
	        }
	    id = 8 - ((value & PD67_INFO_REV) >> 2);
	    }
        }

    printf ("%s rev %d found, %d sockets\n", name, id, sock);
    return (OK);
    }

/*******************************************************************************
*
* pcicShowStatus - Show status of the PCIC chip.
*
* This routine shows status of the PCIC chip.
*
* RETURNS: N/A
*/

LOCAL void pcicShowStatus
    (
    int sock			/* socket no. */
    )
    {
    u_char value = pcicGet (sock, PCIC_STATUS);

    printf ("Interface status = 0x%2.2x\n", value);
    printf ("  ");
    if (value & PCIC_CS_BVD1)
	printf ("[BVD1/STSCHG] ");
    if (value & PCIC_CS_BVD2)
	printf ("[BVD2/SPKR] ");
    if (value & PCIC_CS_DETECT)
	printf ("[DETECT] ");
    if (value & PCIC_CS_WRPROT)
	printf ("[WRPROT] ");
    if (value & PCIC_CS_READY)
	printf ("[READY] ");
    if (value & PCIC_CS_POWERON)
	printf ("[POWERON] ");
    if (value & PCIC_CS_GPI)
	printf ("[GPI] ");
    printf ("\n");
    }

/*******************************************************************************
*
* pcicShowPower - Show power configuration of the PCIC chip.
*
* This routine shows power configuration of the PCIC chip.
*
* RETURNS: N/A
*/

LOCAL void pcicShowPower
    (
    int sock			/* socket no. */
    )
    {
    u_char value = pcicGet(sock, PCIC_POWER);

    printf ("Power control = 0x%2.2x\n", value);
    printf ("  ");
    if (value & PCIC_PWR_OUT)
	printf ("[OUTPUT] ");
    if (!(value & PCIC_PWR_NORESET))
	printf ("[RESETDRV] ");
    if (value & PCIC_PWR_AUTO)
	printf ("[AUTO] ");
    switch (value & PCIC_VCC_MASK)
	{
        case PCIC_VCC_5V:
	    printf ("[Vcc=5v] ");
	    break;
        case PCIC_VCC_3V:
	    printf ("[Vcc=3v] ");
	    break;
        case 0:
	    printf ("[Vcc off] ");
	    break;
        }
    switch (value & PCIC_VPP_MASK)
	{
        case PCIC_VPP_5V:
	    printf ("[Vpp=5v] ");
	    break;
        case PCIC_VPP_12V:
	    printf ("[Vpp=12v] ");
	    break;
        case 0:
	    printf ("[Vpp off] ");
	    break;
        }
    printf ("\n");
    }

/*******************************************************************************
*
* pcicShowIntctl - Show interrupt control configuration of the PCIC chip.
*
* This routine shows interrupt control configuration of the PCIC chip.
*
* RETURNS: N/A
*/

LOCAL void pcicShowIntctl
    (
    int sock			/* socket no. */
    )
    {
    u_char value = pcicGet (sock, PCIC_INTCTL);

    printf ("Interrupt and general control = 0x%2.2x\n", value);
    printf ("  ");
    if (value & PCIC_RING_ENA)
	printf ("[RING_ENA] ");
    if (!(value & PCIC_PC_RESET))
	printf ("[RESET] ");
    if (value & PCIC_PC_IOCARD)
	printf ("[IOCARD] ");
    if (value & PCIC_INTR_ENA)
	printf ("[INTR_ENA] ");
    printf ("[irq=%d]\n", value & PCIC_IRQ_MASK);
}

/*******************************************************************************
*
* pcicShowCsc - Show card status change of the PCIC chip.
*
* This routine shows card status change of the PCIC chip.
*
* RETURNS: N/A
*/

LOCAL void pcicShowCsc
    (
    int sock			/* socket no. */
    )
    {
    u_char value = pcicGet (sock, PCIC_CSC);

    printf ("Card status change = 0x%2.2x\n", value);
    printf ("  ");
    if (value & PCIC_CSC_BVD1)
	printf ("[BVD1/STSCHG] ");
    if (value & PCIC_CSC_BVD2)
	printf ("[BVD2] ");
    if (value & PCIC_CSC_DETECT)
	printf ("[DETECT] ");
    if (value & PCIC_CSC_READY)
	printf ("[READY] ");
    if (value & PCIC_CSC_GPI)
	printf ("[GPI] ");
    printf ("\n");
    }

/*******************************************************************************
*
* pcicShowCscint - Show card status change interrupt configuration.
*
* This routine shows card status change interrupt configuration of 
* the PCIC chip.
*
* RETURNS: N/A
*/

LOCAL void pcicShowCscint
    (
    int sock			/* socket no. */
    )
    {
    u_char value = pcicGet (sock, PCIC_CSCINT);

    printf ("Card status change interrupt control = 0x%2.2x\n", value);
    printf ("  ");
    if (value & PCIC_CSC_BVD1)
	printf ("[BVD1/STSCHG] ");
    if (value & PCIC_CSC_BVD2)
	printf ("[BVD2] ");
    if (value & PCIC_CSC_DETECT)
	printf ("[DETECT] ");
    if (value & PCIC_CSC_READY)
	printf ("[READY] ");
    printf ("[irq = %d]\n", value >> 4);
    }

/*******************************************************************************
*
* pcicShowGenctl - Show general control of the PCIC chip.
*
* This routine shows general control of the PCIC chip.
*
* RETURNS: N/A
*/

LOCAL void pcicShowGenctl
    (
    int sock			/* socket no. */
    )
    {
    u_char value = pcicGet (sock, PCIC_GENCTL);

    printf ("Card detect and general control = 0x%2.2x\n", value);
    printf ("  ");
    if (value & PCIC_CTL_16DELAY)
	printf ("[16DELAY] ");
    if (value & PCIC_CTL_RESET_ENA)
	printf ("[RESET] ");
    if (value & PCIC_CTL_GPI_ENA)
	printf ("[GPI_ENA] ");
    if (value & PCIC_CTL_GPI_CTL)
	printf ("[GPI_CTL] ");
    if (value & PCIC_CTL_RESUME)
	printf ("[RESUME] ");
    printf ("\n");
    }

/*******************************************************************************
*
* pcicShowGblctl - Show global control of the PCIC chip.
*
* This routine shows global control of the PCIC chip.
*
* RETURNS: N/A
*/

LOCAL void pcicShowGblctl
    (
    int sock			/* socket no. */
    )
    {
    u_char value = pcicGet (sock, PCIC_GBLCTL);

    printf ("Global control = 0x%2.2x\n", value);
    printf ("  ");
    if (value & PCIC_GBL_PWRDOWN)
	printf ("[PWRDOWN] ");
    if (value & PCIC_GBL_CSC_LEV)
	printf ("[CSC_LEV] ");
    if (value & PCIC_GBL_WRBACK)
	printf ("[WRBACK] ");
    if (value & PCIC_GBL_IRQ_0_LEV)
	printf ("[IRQ_0_LEV] ");
    if (value & PCIC_GBL_IRQ_1_LEV)
	printf ("[IRQ_1_LEV] ");
    printf ("\n");
    }

/*******************************************************************************
*
* pcicShowMisc - Show misc control of the PCIC chip.
*
* This routine shows misc control of the PCIC chip.
*
* RETURNS: N/A
*/

LOCAL void pcicShowMisc
    (
    int sock			/* socket no. */
    )
    {
    u_char value = pcicGet(sock, PD67_MISC_CTL_1);

    printf ("Misc control 1 = 0x%2.2x\n", value);
    printf ("  ");
    if (value & PD67_MC1_5V_DET)
	printf ("[5V_DET] ");
    if (value & PD67_MC1_VCC_3V)
	printf ("[VCC_3V] ");
    if (value & PD67_MC1_PULSE_MGMT)
	printf ("[PULSE_MGMT] ");
    if (value & PD67_MC1_PULSE_IRQ)
	printf ("[PULSE_IRQ] ");
    if (value & PD67_MC1_SPKR_ENA)
	printf ("[SPKR] ");
    if (value & PD67_MC1_INPACK_ENA)
	printf ("[INPACK] ");
    printf ("\n");

    value = pcicGet(sock, PD67_MISC_CTL_2);
    printf ("Misc control 2 = 0x%2.2x\n", value);
    printf ("  ");
    if (value & PD67_MC2_FREQ_BYPASS)
	printf ("[FREQ_BYPASS] ");
    if (value & PD67_MC2_DYNAMIC_MODE)
	printf ("[DYNAMIC_MODE] ");
    if (value & PD67_MC2_SUSPEND)
	printf ("[SUSPEND] ");
    if (value & PD67_MC2_5V_CORE)
	printf ("[5V_CORE] ");
    if (value & PD67_MC2_LED_ENA)
	printf ("[LED_ENA] ");
    if (value & PD67_MC2_3STATE_BIT7)
	printf ("[3STATE_BIT7] ");
    if (value & PD67_MC2_DMA_MODE)
	printf ("[DMA_MODE] ");
    if (value & PD67_MC2_IRQ15_RI)
	printf ("[IRQ15_RI] ");
    printf ("\n");
    }

/*******************************************************************************
*
* pcicShowTime - Show time scale of the PCIC chip.
*
* This routine shows time scale of the PCIC chip.
*
* RETURNS: N/A
*/

LOCAL void pcicShowTime
    (
    char *pName,		/* name of time scale */
    int  value			/* value of a time scale */
    )
    {
    printf ("%s = %d", pName, value & PD67_TIME_MULT);
    switch (value & PD67_TIME_SCALE)
	{
        case PD67_TIME_SCALE_16:
	    printf ("[*16] ");
	    break;
        case PD67_TIME_SCALE_256:
	    printf ("[*256] ");
	    break;
        case PD67_TIME_SCALE_4096:
	    printf ("[*4096] ");
	    break;
        }
    }

/*******************************************************************************
*
* pcicShowTiming - Show timing of the PCIC chip.
*
* This routine shows timing of the PCIC chip.
*
* RETURNS: N/A
*/

LOCAL void pcicShowTiming
    (
    int sock,			/* socket no. */
    int reg			/* timing register no. */
    )
    {
    printf ("Timing set %d: ", reg);
    pcicShowTime ("setup", pcicGet (sock, PD67_TIME_SETUP(reg)));
    pcicShowTime (", command", pcicGet (sock, PD67_TIME_CMD(reg)));
    pcicShowTime (", recovery", pcicGet (sock, PD67_TIME_RECOV(reg)));
    printf ("\n");
    }

/*******************************************************************************
*
* pcicShowMemwin - Show memory window of the PCIC chip.
*
* This routine shows memory window of the PCIC chip.
*
* RETURNS: N/A
*/

LOCAL void pcicShowMemwin
    (
    int sock,			/* socket no. */
    int win			/* memory window no. */
    )
    {
    u_short start = pcicGet2 (sock, PCIC_MEM(win)+PCIC_W_START);
    u_short stop  = pcicGet2 (sock, PCIC_MEM(win)+PCIC_W_STOP);
    u_short off	  = pcicGet2 (sock, PCIC_MEM(win)+PCIC_W_OFF);

    printf ("Memory window %d: ", win);

    if (pcicGet (sock, PCIC_ADDRWIN) & PCIC_ENA_MEM(win))
	printf ("[ON] ");
    else
	printf ("[OFF] ");

    if (start & PCIC_MEM_16BIT)
	printf ("[16BIT] ");

    if (cirrus)
	{
	if (stop & PCIC_MEM_WS1)
	    printf ("[TIME1] ");
	else
	    printf ("[TIME0] ");
        }
    else
	{
	if (start & PCIC_MEM_0WS)
	    printf ("[0WS] ");
	if (stop & PCIC_MEM_WS1)
	    printf ("[WS1] ");
	if (stop & PCIC_MEM_WS0)
	    printf ("[WS0] ");
        }

    if (off & PCIC_MEM_WRPROT)
	printf ("[WRPROT] ");

    if (off & PCIC_MEM_REG)
	printf ("[REG] ");

    printf ("\n  start = 0x%4.4x", start & 0x3fff);
    printf (", stop = 0x%4.4x", stop & 0x3fff);
    printf (", offset = 0x%4.4x\n", off & 0x3fff);
    }

/*******************************************************************************
*
* pcicShowIowin - Show IO window of the PCIC chip.
*
* This routine shows IO window of the PCIC chip.
*
* RETURNS: N/A
*/

LOCAL void pcicShowIowin
    (
    int sock,			/* socket no. */
    int win			/* IO window no. */
    )
    {
    u_char  ctl   = pcicGet  (sock, PCIC_IOCTL);
    u_short start = pcicGet2 (sock, PCIC_IO(win)+PCIC_W_START);
    u_short stop  = pcicGet2 (sock, PCIC_IO(win)+PCIC_W_STOP);

    printf ("I/O window %d: ", win);

    if (pcicGet (sock, PCIC_ADDRWIN) & PCIC_ENA_IO(win))
	printf ("[ON] ");
    else
	printf ("[OFF] ");
    
    if (cirrus)
	{
	if (ctl & PCIC_IOCTL_WAIT(win))
	    printf (" [TIME1]");
	else
	    printf (" [TIME0]");
        }
    else
	{
	if (ctl & PCIC_IOCTL_WAIT(win))
	    printf ("[WAIT] ");
	if (ctl & PCIC_IOCTL_0WS(win))
	    printf ("[0WS] ");
        }

    if (ctl & PCIC_IOCTL_CS16(win))
	printf ("[CS16] ");

    if (ctl & PCIC_IOCTL_16BIT(win))
	printf ("[16BIT] ");
    
    printf ("\n  start = 0x%4.4x, stop = 0x%4.4x", start, stop);

    if (cirrus)
	{
        u_short off = pcicGet2 (sock, PD67_IO_OFF(win));
	printf (", offset = 0x%4.4x", off);
        }

    printf ("\n");
    }

/*******************************************************************************
*
* pcicGet - Get a value from the register.
*
* This routine gets a value from the register.
*
* RETURNS: a value of the register.
*/

LOCAL int pcicGet
    (
    int sock,			/* socket no. */
    int reg			/* register no. */
    )
    {
    short port	= PCIC_PORT (sock);
    char offset	= PCIC_REG (sock, reg);

    sysOutByte (port, offset);
    return (sysInByte (port+1));
    }

/*******************************************************************************
*
* pcicGet2 - Get a value from the consecutive registers.
*
* This routine gets a value from the consecutive registers.
*
* RETURNS: a value of the register.
*/

LOCAL int pcicGet2
    (
    int sock,			/* socket no. */
    int reg			/* register no. */
    )
    {
    short a	= pcicGet (sock, reg);
    short b	= pcicGet (sock, reg+1);

    return (a + (b<<8));
    }

/*******************************************************************************
*
* pcicSet - Set value into the register.
*
* This routine sets value into the register.
*
* RETURNS: N/A
*/

LOCAL void pcicSet
    (
    int  sock,			/* socket no. */
    int  reg,			/* register no. */
    char value			/* value to set */
    )
    {
    short port	= PCIC_PORT (sock);
    char offset	= PCIC_REG (sock, reg);

    sysOutByte (port, offset);
    sysOutByte (port+1, value);
    }

