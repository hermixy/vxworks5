/* pcmciaShow.c - PCMCIA show library */

/* Copyright 1984-1996 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01c,28mar96,jdi  doc: cleaned up language and format.
01b,22feb96,hdn  cleaned up
01a,12oct95,hdn  written.
*/


/*
DESCRIPTION
This library provides a show routine that shows the status of the
PCMCIA chip and the PC card.

*/


#include "vxWorks.h"
#include "sysLib.h"
#include "drv/pcmcia/pcmciaLib.h"
#include "drv/pcmcia/cisLib.h"
#include "drv/pcmcia/pcic.h"
#include "drv/pcmcia/tcic.h"
#include "drv/pcmcia/sramDrv.h"
#include "drv/hdisk/ataDrv.h"


/* defines */



/* imports */

IMPORT PCMCIA_CTRL	pcmciaCtrl;
IMPORT PCMCIA_ADAPTER	pcmciaAdapter [];
IMPORT PCCARD_ENABLER	pccardEnabler [];
IMPORT int		pcmciaAdapterNumEnt;
IMPORT int		pccardEnablerNumEnt;


/* globals */



/* locals */

LOCAL FUNCPTR cisShowRtn;


/* forward declarations */



/*******************************************************************************
*
* pcmciaShowInit - initialize all show routines for PCMCIA drivers
*
* This routine initializes all show routines related to PCMCIA drivers.
*
* RETURNS: N/A
*/

void pcmciaShowInit (void)

    {
    int ix;
    PCMCIA_ADAPTER *pAdapter;

    for (ix = 0; ix < pcmciaAdapterNumEnt; ix++)
	{
	pAdapter = &pcmciaAdapter [ix];
	if (pAdapter->type == PCMCIA_PCIC)
	    pAdapter->showRtn = (FUNCPTR)pcicShow;
	if (pAdapter->type == PCMCIA_TCIC)
	    pAdapter->showRtn = (FUNCPTR)tcicShow;
	}

    cisShowRtn		= (FUNCPTR)cisShow;

    pccardShowInit ();

    }

/*******************************************************************************
*
* pcmciaShow - show all configurations of the PCMCIA chip
*
* This routine shows all configurations of the PCMCIA chip.
*
* RETURNS: N/A
*/

void pcmciaShow
    (
    int sock			/* socket no. */
    )

    {
    PCMCIA_CTRL *pCtrl		= &pcmciaCtrl;
    PCMCIA_CHIP *pChip		= &pCtrl->chip;
    PCMCIA_CARD *pCard		= &pCtrl->card[sock];

    if (pChip->showRtn != NULL)
	(* pChip->showRtn) (sock);

    if (pCard->showRtn != NULL)
	(* pCard->showRtn) (sock);
    }
