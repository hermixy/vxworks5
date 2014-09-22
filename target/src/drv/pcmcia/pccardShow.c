/* pccardShow.c - PC CARD show library */

/* Copyright 1984-1996 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01d,11dec97,hdn  added TFFS support for flash PC card.
01c,08nov96,dgp  doc: final formatting
01b,22feb96,hdn  cleaned up
01a,10oct95,hdn  written.
*/

/*
DESCRIPTION
This library provides generic facilities for showing PC CARD status.
*/

/* LINTLIBRARY */

#include "drv/pcmcia/pcmciaLib.h"
#include "drv/pcmcia/pccardLib.h"
#include "drv/pcmcia/sramDrv.h"
#include "drv/hdisk/ataDrv.h"
#include "net/if.h"
#include "netinet/if_ether.h"
#include "drv/netif/if_elt.h"
#include "tffs/tffsDrv.h"


/* externs */

IMPORT PCMCIA_CTRL	pcmciaCtrl;
IMPORT char		pcDriveNo [];


/* globals */



/* locals */



/* forward declarations */

#ifdef  INCLUDE_SHOW_ROUTINES
LOCAL STATUS pccardShow		(int sock);
LOCAL STATUS pccardSramShow	(int sock);
LOCAL STATUS pccardAtaShow	(int sock);
LOCAL STATUS pccardEltShow	(int sock);
LOCAL STATUS pccardTffsShow	(int sock);
#endif  /* INCLUDE_SHOW_ROUTINES */


/*******************************************************************************
*
* pccardShowInit - initialize all show routines related to PC CARD drivers
*
* This routine initializes all show routines related to PC CARD drivers.
*
* RETURNS: N/A
*/

void pccardShowInit (void)

    {
#ifdef  INCLUDE_SHOW_ROUTINES

    int ix;
    PCCARD_ENABLER *pEnabler;

    for (ix = 0; ix < pccardEnablerNumEnt; ix++)
	{
	pEnabler = &pccardEnabler [ix];
#ifdef	INCLUDE_ATA
	if (pEnabler->type == PCCARD_ATA)
	    pEnabler->showRtn = (FUNCPTR)pccardAtaShow;
#endif	/* INCLUDE_ATA */
#ifdef	INCLUDE_SRAM
	if (pEnabler->type == PCCARD_SRAM)
	    pEnabler->showRtn = (FUNCPTR)pccardSramShow;
#endif	/* INCLUDE_SRAM */
#ifdef	INCLUDE_ELT
	if (pEnabler->type == PCCARD_LAN_ELT)
	    pEnabler->showRtn = (FUNCPTR)pccardEltShow;
#endif	/* INCLUDE_ELT */
#ifdef	INCLUDE_TFFS
	if (pEnabler->type == PCCARD_FLASH)
	    pEnabler->showRtn = (FUNCPTR)pccardTffsShow;
#endif	/* INCLUDE_TFFS */
	}

#endif  /* INCLUDE_SHOW_ROUTINES */
    }

#ifdef  INCLUDE_SHOW_ROUTINES
/*******************************************************************************
*
* pccardShow - show PC card generic status
*
* Show PC card generic status
*
* RETURNS: OK (always).
*/

LOCAL STATUS pccardShow
    (
    int sock			/* socket no. */
    )

    {
    PCMCIA_CTRL *pCtrl		= &pcmciaCtrl;
    PCMCIA_CARD *pCard		= &pCtrl->card[sock];
    PCCARD_RESOURCE *pResource	= pCard->pResource;

    /* info from PCMCIA_CARD */

    printf ("  ctrl       = 0x%-8x\n", pCard->ctrl);
    printf ("  installed  = %-10s  changed    = %-10s\n",
    	    pCard->installed ? "TRUE" : "FALSE",
    	    pCard->changed   ? "TRUE" : "FALSE");
    printf ("  cardStatus = 0x%-8x  initStatus = 0x%-8x\n",
            pCard->cardStatus, pCard->initStatus);
    printf ("  pBlockDev  = 0x%-8x  pNetIf     = 0x%-8x\n",
            (u_int)pCard->pBlkDev, (u_int)pCard->pNetIf);

    /* info from PCCARD_RESOURCE */

    printf ("  Vcc        = 0x%-8x  Vpp        = 0x%-8x\n",
	    pResource->vcc, pResource->vpp);

    printf ("  ioStart[0] = 0x%-8x  ioStop[0]  = 0x%-8x\n",
            pResource->ioStart[0], pResource->ioStop[0]);
    printf ("  ioStart[1] = 0x%-8x  ioStop[1]  = 0x%-8x\n",
            pResource->ioStart[1], pResource->ioStop[1]);
    printf ("  ioExtraws  = %-8d\n",
            pResource->ioExtraws);

    printf ("  memStart   = 0x%-8x  memStop    = 0x%-8x\n",
	    pResource->memStart, pResource->memStop);
    printf ("  memLength  = 0x%-8x  memOffset  = 0x%-8x\n",
	    pResource->memLength, pResource->memOffset);
    printf ("  memExtraws = 0x%-8x\n",
	    pResource->memExtraws);

    return (OK);
    }

#ifdef	INCLUDE_SRAM
/*******************************************************************************
*
* pccardSramShow - show SRAM status
*
* Show SRAM status
*
* RETURNS: OK, or ERROR if the driver is not installed.
*/

LOCAL STATUS pccardSramShow
    (
    int sock			/* socket no. */
    )

    {
    PCMCIA_CTRL *pCtrl		= &pcmciaCtrl;
    PCMCIA_CARD *pCard		= &pCtrl->card[sock];
    PCCARD_RESOURCE *pResource	= pCard->pResource;

    if (!pCard->detected)
	{
	printf ("Socket %d is Empty\n", sock);
	return (ERROR);
	}
    
    if (pResource == NULL)
	{
	printf ("Socket %d is Unknown PC card\n", sock);
	return (ERROR);
	}
    
    printf ("Socket %d is SRAM\n", sock);

    pccardShow (sock);

    return (OK);
    }
#endif	/* INCLUDE_SRAM */

#ifdef	INCLUDE_ATA
/*******************************************************************************
*
* pccardAtaShow - show ATA (PCMCIA) status
*
* Show ATA (PCMCIA) status
*
* RETURNS: OK, or ERROR if the driver is not installed.
*
* NOMANUAL
*/

LOCAL STATUS pccardAtaShow
    (
    int sock			/* socket no. */
    )

    {
    PCMCIA_CTRL *pCtrl		= &pcmciaCtrl;
    PCMCIA_CARD *pCard		= &pCtrl->card[sock];
    PCCARD_RESOURCE *pResource	= pCard->pResource;
    ATA_RESOURCE *pAta		= (ATA_RESOURCE *)pResource;

    if (!pCard->detected)
	{
	printf ("Socket %d is Empty\n", sock);
	return (ERROR);
	}
    
    if (pResource == NULL)
	{
	printf ("Socket %d is Unknown PC card\n", sock);
	return (ERROR);
	}
    
    printf ("Socket %d is ATA\n", sock);

    pccardShow (sock);

    printf ("  intVector  = 0x%-8x  intLevel   = 0x%-8x\n",
	    pAta->intVector, pAta->intLevel);
    printf ("  drives     = 0x%-8x  configType = 0x%-8x\n",
	    pAta->drives, pAta->configType);
    printf ("  twins      = 0x%-8x  pwrdown    = 0x%-8x\n",
            pAta->sockTwin, pAta->pwrdown);

    return (OK);
    }
#endif	/* INCLUDE_ATA */

#ifdef	INCLUDE_ELT
/*******************************************************************************
*
* pccardEltShow - show ELT (PCMCIA) status
*
* Show ELT (PCMCIA) status
*
* RETURNS: OK, or ERROR if the card is not installed.
*
* NOMANUAL
*/

LOCAL STATUS pccardEltShow
    (
    int sock			/* socket no. */
    )

    {
    PCMCIA_CTRL *pCtrl		= &pcmciaCtrl;
    PCMCIA_CARD *pCard		= &pCtrl->card[sock];
    PCCARD_RESOURCE *pResource	= pCard->pResource;
    ELT_RESOURCE *pElt		= (ELT_RESOURCE *)pResource;

    if (!pCard->detected)
	{
	printf ("Socket %d is Empty\n", sock);
	return (ERROR);
	}
    
    if (pResource == NULL)
	{
	printf ("Socket %d is Unknown PC card\n", sock);
	return (ERROR);
	}
    
    printf ("Socket %d is ELT\n", sock);

    pccardShow (sock);

    printf ("  intVector  = 0x%-8x  intLevel   = 0x%-8x\n",
	    pElt->intVector, pElt->intLevel);
    printf ("  rxFrames   = 0x%-8x  connector  = 0x%-8x\n",
            pElt->rxFrames, pElt->connector);

    return (OK);
    }
#endif	/* INCLUDE_ELT */

#ifdef	INCLUDE_TFFS
/*******************************************************************************
*
* pccardTffsShow - show TFFS (PCMCIA) status
*
* Show TFFS (PCMCIA) status
*
* RETURNS: OK, or ERROR if the card is not installed.
*
* NOMANUAL
*/

LOCAL STATUS pccardTffsShow
    (
    int sock			/* socket no. */
    )

    {
    PCMCIA_CTRL *pCtrl		= &pcmciaCtrl;
    PCMCIA_CARD *pCard		= &pCtrl->card[sock];
    PCCARD_RESOURCE *pResource	= pCard->pResource;

    if (!pCard->detected)
	{
	printf ("Socket %d is Empty\n", sock);
	return (ERROR);
	}
    
    if (pResource == NULL)
	{
	printf ("Socket %d is Unknown PC card\n", sock);
	return (ERROR);
	}
    
    printf ("Socket %d is Flash\n", sock);

    pccardShow (sock);

    if (pcDriveNo[sock] != NONE)
	tffsShow (pcDriveNo [sock]);

    return (OK);
    }
#endif	/* INCLUDE_ELT */
#endif  /* INCLUDE_SHOW_ROUTINES */

