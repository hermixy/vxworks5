/* cisLib.c - PCMCIA CIS library */

/* Copyright 1984-1996 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01f,21jun00,rsh  upgrade to dosFs 2.0
01g,17sep98,lrn  reworked DosFs 2.0 support and memory leak issues
01f,14jul98,lrn  DosFs2.0: can not delete DOS devices
01e,16jan97,hdn  added pCtrl->memBase, CIS_MAX_TUPLES.
01d,28mar96,jdi  doc: cleaned up language and format.
01c,08mar96,hdn  added more descriptions.
01b,22feb96,hdn  cleaned up
01a,10feb95,hdn  written.
*/


/*
DESCRIPTION
This library contains routines to manipulate the CIS (Configuration
Information Structure) tuples and the card configuration registers.
The library uses a memory window which is defined in `pcmciaMemwin'
to access the CIS of a PC card.
All CIS tuples in a PC card are read and stored in a linked list,
`cisTupleList'.  If there are configuration tuples, they are interpreted
and stored in another link list, `cisConifigList'.  After the CIS is read,
the PC card's enabler routine allocates resources and initializes a device
driver for the PC card.

If a PC card is inserted, the CSC (Card Status Change) interrupt handler
gets a CSC event from the PCMCIA chip and adds a cisGet() job to the
PCMCIA daemon.  The PCMCIA daemon initiates the cisGet() work.  The CIS
library reads the CIS from the PC card and makes a linked list of CIS
tuples.  It then enables the card.

If the PC card is removed, the CSC interrupt handler gets a CSC event from
the PCMCIA chip and adds a cisFree() job to the PCMCIA daemon.  The PCMCIA
daemon initiates the cisFree() work.  The CIS library frees allocated 
memory for the linked list of CIS tuples.
*/


#include "vxWorks.h"
#include "taskLib.h"
#include "sysLib.h"
#include "stdlib.h"
#include "string.h"
#include "drv/pcmcia/pcmciaLib.h"
#include "drv/pcmcia/cisLib.h"


/* defines */


/* imports */

IMPORT PCMCIA_CTRL	pcmciaCtrl;
IMPORT PCMCIA_MEMWIN	pcmciaMemwin[];
IMPORT PCCARD_ENABLER   pccardEnabler[];
IMPORT int		pccardEnablerNumEnt;


/* globals */

SEMAPHORE	cisMuteSem;


/* locals */

LOCAL u_char	powerMantissa [] = {10, 12, 13, 15, 20, 25, 30, 35,
				 40, 45, 50, 55, 60, 70, 80, 90};
LOCAL u_char	powerExponent [] = {0, 0, 0, 0, 0, 1, 10, 100};


/* forward declarations */

LOCAL STATUS	cisTupleGet	(int sock);
LOCAL STATUS	cisConfigGet	(int sock);
LOCAL STATUS	cisResourceGet	(int sock);


/*******************************************************************************
*
* cisGet - get information from a PC card's CIS
*
* This routine gets information from a PC card's CIS, configures the PC card,
* and allocates resources for the PC card.
*
* RETURNS: OK, or ERROR if it cannot get the CIS information,
* configure the PC card, or allocate resources.
*/

STATUS cisGet
    (
    int sock			/* socket no. */
    )
    {
    PCMCIA_CTRL *pCtrl	= &pcmciaCtrl;
    PCMCIA_CHIP *pChip	= &pCtrl->chip;
    PCMCIA_CARD *pCard	= &pCtrl->card[sock];
    STATUS status	= ERROR;
    DL_NODE *pNode;
    DL_NODE *pTemp;


    semTake (&cisMuteSem, WAIT_FOREVER);	/* mutual exclusion begin */
    (void) (* pChip->cscOff) (sock, pChip->intLevel);

    for (pNode = DLL_FIRST(&pCard->cisTupleList); pNode != NULL; pNode = pTemp)
	{
	pTemp = DLL_NEXT(pNode);
	free (pNode);
	}

    for (pNode = DLL_FIRST(&pCard->cisConfigList); pNode != NULL; pNode = pTemp)
	{
	pTemp = DLL_NEXT(pNode);
	free (pNode);
	}

    dllInit (&pCard->cisTupleList);
    dllInit (&pCard->cisConfigList);

    if ((cisTupleGet (sock) == OK) &&
	(cisConfigGet (sock) == OK) &&
    	(cisResourceGet (sock) == OK))
	status = OK;

    (void) (* pChip->cscPoll) (sock);

    (void) (* pChip->cscOn) (sock, pChip->intLevel);
    semGive (&cisMuteSem);			/* mutual exclusion end */

    return (status);
    }

/*******************************************************************************
*
* cisTupleGet - get tuples from PC card and attach it to the link list
*
* This routine get tuples from PC card and attach it to the link list.
*
* RETURNS: OK, or ERROR if it couldn't set a value to the PCMCIA chip.
*/

LOCAL STATUS cisTupleGet
    (
    int sock			/* socket no. */
    )

    {
    BOOL endList	= FALSE;
    BOOL noLink		= FALSE;
    u_long longlinkA	= 0;
    u_long longlinkC	= 0;
    PCMCIA_CTRL *pCtrl	= &pcmciaCtrl;
    PCMCIA_CHIP *pChip	= &pCtrl->chip;
    PCMCIA_CARD *pCard	= &pCtrl->card[sock];
    int ix		= 0;
    PCMCIA_MEMWIN cisMemwin;
    CIS_TUPLE_A *pTupleA;
    CIS_TUPLE_C *pTupleC;
    DL_NODE *pNode;
    u_char *pChar;
    u_char *pd;
    u_char *ps;
    int flag;
    int tuples;

    /* XXX
     * Assumption
     * 1. CIS tuple chain is in between start and stop in pcmciaMemwin[0].
     * 2. Longlink from Common memory to Attribute memory doesn't occur
     *
     * Card reset/power signal sequence (See p.60 PCMCIA Developer's guide)
     *                  ______________________________________________
     *   Card detect  __|
     *                   >50ms-
     *                         _______________________________________
     *   Vcc          _________|
     *                          >300ms--------
     *   Card signal                          ________________________
     *       enabled  ________________________|
     *                                        __________
     *   Reset        ________________________|        |______________
     *                                         >10ms---
     *                                                  >20ms----
     *   Card access                                             _____
     *       allowed  ___________________________________________|
     */

    if (pChip->installed != TRUE)
	return (ERROR);

    taskDelay (sysClkRateGet() >> 3);		/* 7 * 16ms = 112ms */

    if ((* pChip->reset)(sock) != OK)
	return (ERROR);

    flag = PC_PWR_AUTO | PC_VCC_5V | PC_VPP_5V;
    if ((* pChip->flagSet)(sock, flag) != OK)
	return (ERROR);

    taskDelay (sysClkRateGet() >> 1);		/* 30 * 16ms = 500ms */

    flag = PC_PWR_AUTO | PC_VCC_5V | PC_VPP_5V | PC_RESET;
    if ((* pChip->flagSet)(sock, flag) != OK)
	return (ERROR);

    taskDelay (sysClkRateGet() >> 4);		/* 3 * 16ms = 48ms */

    flag = PC_PWR_AUTO | PC_VCC_5V | PC_VPP_5V;
    if ((* pChip->flagSet)(sock, flag) != OK)
	return (ERROR);

    taskDelay (sysClkRateGet() >> 2);		/* 15 * 16ms = 240ms */

    flag = PC_READY | PC_POWERON;
    while ((((* pChip->status)(sock) & flag) != flag) && (ix++ < 8))
	taskDelay (sysClkRateGet() >> 2);

    do  {

	/* map attribute memory */

	pChar			= (u_char *)pcmciaMemwin[CIS_MEM_TUPLE].start +
				  pCtrl->memBase;
        cisMemwin.window	= PCMCIA_CIS_WINDOW;
        cisMemwin.flags		= MAP_ACTIVE | MAP_16BIT | MAP_ATTRIB;
        cisMemwin.extraws	= 2;
        cisMemwin.start		= pcmciaMemwin[CIS_MEM_TUPLE].start;
        cisMemwin.stop		= pcmciaMemwin[CIS_MEM_TUPLE].stop;
        cisMemwin.cardstart	= longlinkA;
	if ((* pChip->memwinSet)(sock, &cisMemwin) != OK)
	    return (ERROR);

	/* check the link target tuple if we had longlink tuple */

	if (longlinkA != 0)
	    {
	    longlinkA = 0;
	    pTupleA = (CIS_TUPLE_A *)pChar;
	    if (pTupleA->code == CISTPL_LINKTARGET)
		pChar += (pTupleA->link + 2) * 2;
	    else
		break;
	    }
	
	/* parse the tuple chain */

	endList = FALSE;
	for (tuples = 0; !endList; tuples++)
	    {
	    pTupleA = (CIS_TUPLE_A *)pChar;

	    switch (pTupleA->code)
		{
		case CISTPL_NULL:
		    pChar += 2;
		    break;
		
		case CISTPL_LONGLINK_A:
		    ps = pChar + 4;
		    pd = (u_char *)&longlinkA;
		    for (ix = 0; ix < 4; ix++)
			*pd++ = *ps++;
		    pChar += (pTupleA->link + 2) * 2;
		    break;
		
		case CISTPL_LONGLINK_C:
		    ps = pChar + 4;
		    pd = (u_char *)&longlinkC;
		    for (ix = 0; ix < 4; ix++)
			*pd++ = *ps++;
		    pChar += (pTupleA->link + 2) * 2;
		    break;
		
		case CISTPL_NO_LINK:
		    noLink = TRUE;
		    pChar += (pTupleA->link + 2) * 2;
		    break;

		case CISTPL_END:
		    if (tuples != 0)
		        endList = TRUE;
		    pChar += (pTupleA->link + 2) * 2;
		    break;

		case CISTPL_CHECKSUM:
		    /* XXX ignore now */
		    pChar += (pTupleA->link + 2) * 2;
		    break;
		
		default:
		    pNode = (DL_NODE *)malloc (pTupleA->link + 2 +
					       sizeof(DL_NODE));
		    pd = (u_char *)pNode + sizeof(DL_NODE);
		    ps = pChar;
		    for (ix = 0; ix < (pTupleA->link + 2); ix++)
			{
			*pd++ = *ps++;
			ps++;
			}
		    dllAdd (&pCard->cisTupleList, pNode);
		    pChar += (pTupleA->link + 2) * 2;
		    break;
		}
	    if ((pTupleA->link == 0xff) || (tuples >= CIS_MAX_TUPLES))
		endList = TRUE;
	    }
	} while (longlinkA != 0);

    if (noLink)
	{
        /* unmap memory window */

        cisMemwin.window	= 0;
        cisMemwin.flags		= 0;
        cisMemwin.extraws	= 0;
        cisMemwin.start		= 0;
        cisMemwin.stop		= 0;
        cisMemwin.cardstart	= 0;
        if ((* pChip->memwinSet)(sock, &cisMemwin) != OK)
            return (ERROR);
        return (OK);
	}


    do  {

	/* map common memory */

	pChar			= (u_char *)pcmciaMemwin[CIS_MEM_TUPLE].start +
				  pCtrl->memBase;
        cisMemwin.window	= PCMCIA_CIS_WINDOW;
        cisMemwin.flags		= MAP_ACTIVE | MAP_16BIT;
        cisMemwin.extraws	= 2;
        cisMemwin.start		= pcmciaMemwin[CIS_MEM_TUPLE].start;
        cisMemwin.stop		= pcmciaMemwin[CIS_MEM_TUPLE].stop;
        cisMemwin.cardstart	= longlinkC;
	if ((* pChip->memwinSet)(sock, &cisMemwin) != OK)
	    return (ERROR);

	/* check the link target tuple */

	pTupleC = (CIS_TUPLE_C *)pChar;
	if (pTupleC->code == CISTPL_LINKTARGET)
	    pChar += pTupleC->link + 2;
	else
	    break;

	/* parse the tuple chain */

	endList = FALSE;
	for (tuples = 0; !endList; tuples++)
	    {
	    pTupleC = (CIS_TUPLE_C *)pChar;
	    switch (pTupleC->code)
		{
		case CISTPL_NULL:
		    pChar++;
		    break;

		case CISTPL_LONGLINK_A:
		    ps = pChar + 2;
		    pd = (u_char *)&longlinkA;
		    for (ix = 0; ix < 4; ix++)
			*pd++ = *ps++;
		    
		    pChar += pTupleC->link + 2;
		    break;

		case CISTPL_LONGLINK_C:
		    ps = pChar + 2;
		    pd = (u_char *)&longlinkC;
		    for (ix = 0; ix < 4; ix++)
			*pd++ = *ps++;
		    
		    pChar += pTupleC->link + 2;
		    break;
		
		case CISTPL_NO_LINK:
		    noLink = TRUE;
		    pChar += pTupleC->link + 2;
		    break;
		
		case CISTPL_END:
		    if (tuples != 0)
		        endList = TRUE;
		    pChar += pTupleC->link + 2;
		    break;

		case CISTPL_CHECKSUM:
		    /* XXX ignore now */
		    pChar += pTupleC->link + 2;
		    break;
		
		default:
		    pNode = (DL_NODE *)malloc (pTupleC->link + 2 + 
					       sizeof(DL_NODE));
		    pd = (u_char *)pNode + sizeof(DL_NODE);
		    ps = pChar;
		    for (ix = 0; ix < (pTupleC->link + 2); ix++)
			*pd++ = *ps++;
		    dllAdd (&pCard->cisTupleList, pNode);
		    pChar += pTupleC->link + 2;
		    break;
		}
	    if ((pTupleC->link == 0xff) || (tuples >= CIS_MAX_TUPLES))
		endList = TRUE;
	    }
	} while (longlinkC != 0);

    /* unmap memory window */

    cisMemwin.window		= 0;
    cisMemwin.flags		= 0;
    cisMemwin.extraws		= 0;
    cisMemwin.start		= 0;
    cisMemwin.stop		= 0;
    cisMemwin.cardstart		= 0;
    if ((* pChip->memwinSet)(sock, &cisMemwin) != OK)
        return (ERROR);

    return (OK);
    }

/*******************************************************************************
*
* cisConfigGet - get configuration information from the CIS tuple link list
*
* Get configuration information from the CIS tuple link list.
*
* RETURNS: OK (always).
*/

LOCAL STATUS cisConfigGet
    (
    int sock			/* socket no. */
    )

    {
    PCMCIA_CTRL *pCtrl		= &pcmciaCtrl;
    PCMCIA_CARD *pCard		= &pCtrl->card[sock];
    CIS_CONFIG *pDefault	= 0;
    CIS_CONFIG *pConfig;
    DL_NODE *pNode;
    CIS_TUPLE *pTuple;
    CIS_BYTE4 base;
    CIS_BYTE4 mask;
    CIS_BYTE4 addr;
    CIS_BYTE4 length;
    CIS_BYTE4 cAddr;
    CIS_BYTE4 hAddr;
    int lengthSize;
    int addrSize;
    int maskSize;
    int windows;
    int ix;
    u_char *pChar;
    u_char *pEnd;
    u_char *pv;
    u_char featureSelection;
    u_char parameterSelection;
    u_char hostaddr;
    u_char waitScale;
    u_char busyScale;
    u_char reservedScale;


    for (pNode = DLL_FIRST (&pCard->cisTupleList);
	 pNode != NULL;
	 pNode = DLL_NEXT(pNode))
	{
	pTuple	= (CIS_TUPLE *)((char *)pNode + sizeof (DL_NODE));
	pChar	= (u_char *)pTuple + sizeof (CIS_TUPLE);
	pEnd	= pChar + pTuple->link;

	switch (pTuple->code)
	    {
	    case CISTPL_CONFIG:
		pCard->regBase = 0;
		pCard->regMask = 0;
		addrSize = (*pChar & 0x03) + 1;
		maskSize = ((*pChar & 0x3c) >> 2) + 1;
		pChar += 2;
		if (pChar > pEnd)
		    break;

		base.l = 0;
		for (ix = 0; ix < addrSize; ix++)
		    base.c[ix] = *pChar++;

		mask.l = 0;
		for (ix = 0; ix < maskSize; ix++)
		    mask.c[ix] = *pChar++;
		
		pCard->regBase = base.l;
		pCard->regMask = mask.l;
		break;
	    
	    case CISTPL_CFTABLE_ENTRY:
		pConfig = (CIS_CONFIG *)calloc(1, sizeof(CIS_CONFIG));
		if (*pChar & 0x40)
		    pDefault = pConfig;
		else if (pDefault != 0)
		    {
		    bcopy ((char *)pDefault, (char *)pConfig, 
			   sizeof(CIS_CONFIG));
		    dllInit ((DL_LIST *)&pConfig->node);
		    }

		dllAdd (&pCard->cisConfigList, &pConfig->node);

		pConfig->index = *pChar & 0x3f;
		if (*pChar++ & 0x80)
		    pConfig->interfaceType = *pChar++ & 0x0f;
		featureSelection = *pChar++;

		if (featureSelection & 0x03)	/* power description */
		    {
		    for (ix=1; ix <= (featureSelection & 0x03); ix++)
			{
			if (ix == 1)
			    pv = &pConfig->vcc[0];
			else if (ix == 2)
			    pv = &pConfig->vpp1[0];
			else
			    pv = &pConfig->vpp2[0];
			
			parameterSelection = *pChar++;
			if (parameterSelection & 0x01)
			    {
			    *pv++ = powerMantissa[(*pChar & 0x78) >> 3] *
				    powerExponent[(*pChar & 0x07)];
			    while (*pChar++ & 0x80)
				;
			    }
			if (parameterSelection & 0x02)
			    {
			    *pv++ = powerMantissa[(*pChar & 0x78) >> 3] *
				    powerExponent[(*pChar & 0x07)];
			    while (*pChar++ & 0x80)
				;
			    }
			if (parameterSelection & 0x04)
			    {
			    *pv++ = powerMantissa[(*pChar & 0x78) >> 3] *
				    powerExponent[(*pChar & 0x07)];
			    while (*pChar++ & 0x80)
				;
			    }
			if (parameterSelection & 0x08)
			    {
			    while (*pChar++ & 0x80)
				;
			    }
			if (parameterSelection & 0x10)
			    {
			    while (*pChar++ & 0x80)
				;
			    }
			if (parameterSelection & 0x20)
			    {
			    while (*pChar++ & 0x80)
				;
			    }
			if (parameterSelection & 0x40)
			    {
			    while (*pChar++ & 0x80)
				;
			    }
			}
		    }
		if (featureSelection & 0x04)	/* timing description */
		    {
		    waitScale	  = *pChar & 0x03;
		    busyScale	  = (*pChar & 0x1c) >> 2;
		    reservedScale = (*pChar & 0xe0) >> 5;

		    pChar++;
		    if (waitScale != 0x03)
			while (*pChar++ & 0x80)
			    ;
		    if (busyScale != 0x03)
			while (*pChar++ & 0x80)
			    ;
		    if (reservedScale != 0x03)
			while (*pChar++ & 0x80)
			    ;
		    }
		if (featureSelection & 0x08)	/* IO description */
		    {
		    pConfig->ioAddrlines = *pChar & 0x1f;
		    pConfig->ioBuswidth	 = (*pChar & 0x60) >> 5;
		    pConfig->ioRanges	 = 0;
		    if (*pChar++ & 0x80)
			{
			pConfig->ioRanges = (*pChar & 0x07) + 1;
			addrSize   = (*pChar & 0x30) >> 4;
			lengthSize = (*pChar & 0xc0) >> 6;
			pChar++;
			for (ix = 0; ix < pConfig->ioRanges; ix++)
			    {
			    addr.l = 0;
			    if (addrSize == 1)
				addr.c[0] = *pChar++;
			    else if (addrSize == 2)
				{
				addr.c[0] = *pChar++;
				addr.c[1] = *pChar++;
				}
			    else if (addrSize == 3)
				{
				addr.c[0] = *pChar++;
				addr.c[1] = *pChar++;
				addr.c[2] = *pChar++;
				addr.c[3] = *pChar++;
				}
			    length.l = 0;
			    if (lengthSize == 1)
				length.c[0] = *pChar++;
			    else if (lengthSize == 2)
				{
				length.c[0] = *pChar++;
				length.c[1] = *pChar++;
				}
			    else if (lengthSize == 3)
				{
				length.c[0] = *pChar++;
				length.c[1] = *pChar++;
				length.c[2] = *pChar++;
				length.c[3] = *pChar++;
				}
			    pConfig->io[ix].start = addr.l;
			    pConfig->io[ix].stop  = addr.l + length.l;
			    }
			}
		    }
		if (featureSelection & 0x10)	/* IRQ description */
		    {
		    pConfig->irqMode = (*pChar & 0xe0) >> 5;
		    pConfig->irqMask = (*pChar & 0x10) >> 4;
		    if (pConfig->irqMask == 1)
			{
			pConfig->irqSpecial  = *pChar++ & 0x0f;
			pConfig->irqBit.c[0] = *pChar++;
			pConfig->irqBit.c[1] = *pChar++;
			}
		    else
			{
			pConfig->irqSpecial  = 0;
			pConfig->irqBit.c[0] = 0;
			pConfig->irqBit.c[1] = 0;
			pConfig->irqLevel    = *pChar++ & 0x0f;
			}
		    }
		if (featureSelection & 0x60)	/* memory description */
		    {
		    if ((featureSelection & 0x60) == 0x20)
			{
			length.l = 0;
			length.c[0] = *pChar++;
			length.c[1] = *pChar++;
			pConfig->mem[0].length = length.l;
			pConfig->mem[0].cAddr  = 0;
			pConfig->mem[0].hAddr  = 0;
			}
		    else if ((featureSelection & 0x60) == 0x40)
			{
			length.l    = 0;
			cAddr.l     = 0;
			length.c[0] = *pChar++;
			length.c[1] = *pChar++;
			cAddr.c[0]  = *pChar++;
			cAddr.c[1]  = *pChar++;
			pConfig->mem[0].length = length.l;
			pConfig->mem[0].cAddr  = cAddr.l;
			pConfig->mem[0].hAddr  = 0;
			}
		    else if ((featureSelection & 0x60) == 0x60)
			{
			windows    = (*pChar & 0x07) + 1;
			lengthSize = (*pChar & 0x18) >> 3;
			addrSize   = (*pChar & 0x60) >> 5;
			hostaddr   = *pChar & 0x80;
			for (ix = 0; ix < windows; ix++)
			    {
			    length.l = 0;
			    cAddr.l  = 0;
			    hAddr.l  = 0;
			    if (lengthSize == 1)
				length.c[0] = *pChar++;
			    else if (lengthSize == 2)
				{
				length.c[0] = *pChar++;
				length.c[1] = *pChar++;
				}
			    else if (lengthSize == 3)
				{
				length.c[0] = *pChar++;
				length.c[1] = *pChar++;
				length.c[2] = *pChar++;
				length.c[3] = *pChar++;
				}

			    if (addrSize == 1)
				cAddr.c[0] = *pChar++;
			    else if (addrSize == 2)
				{
				cAddr.c[0] = *pChar++;
				cAddr.c[1] = *pChar++;
				}
			    else if (addrSize == 3)
				{
				cAddr.c[0] = *pChar++;
				cAddr.c[1] = *pChar++;
				cAddr.c[2] = *pChar++;
				cAddr.c[3] = *pChar++;
				}
		    
			    if (hostaddr == 0x80)
				{
			        if (addrSize == 1)
				    cAddr.c[0] = *pChar++;
			        else if (addrSize == 2)
				    {
				    cAddr.c[0] = *pChar++;
				    cAddr.c[1] = *pChar++;
				    }
			        else if (addrSize == 3)
				    {
				    cAddr.c[0] = *pChar++;
				    cAddr.c[1] = *pChar++;
				    cAddr.c[2] = *pChar++;
				    cAddr.c[3] = *pChar++;
				    }
				}
			    pConfig->mem[ix].length = length.l;
			    pConfig->mem[ix].cAddr  = cAddr.l;
			    pConfig->mem[ix].hAddr  = hAddr.l;
			    }
			}
		    }
		if (featureSelection & 0x80)	/* misc info. description */
		    {
		    pConfig->twins    = *pChar & 0x07;
		    pConfig->audio    = (*pChar & 0x08) >> 3;
		    pConfig->readonly = (*pChar & 0x10) >> 4;
		    pConfig->pwrdown  = (*pChar & 0x20) >> 5;
		    }
		break;
	    }
	}

    return (OK);
    }

/*******************************************************************************
*
* cisResourceGet - config the PC card from the configuration table link list
*
* Config the PC card from the configuration table link list.
*
* RETURNS: OK, or ERROR if the enabler couldn't initialize the PC card.
*/

LOCAL STATUS cisResourceGet
    (
    int sock			/* socket no. */
    )

    {
    PCMCIA_CTRL *pCtrl	= &pcmciaCtrl;
    PCMCIA_CARD *pCard	= &pCtrl->card[sock];
    STATUS status	= ERROR;
    PCCARD_ENABLER *pEnabler;
    int ix;

    for (ix = 0; ix < pccardEnablerNumEnt; ix++)
	{
	pEnabler = &pccardEnabler[ix];
	if (pEnabler->enableRtn != NULL)
	    {
	    if ((pCard->initStatus = (* pEnabler->enableRtn) (
					sock, 
					pEnabler->pResource,
					pEnabler->resourceNumEnt,
					pEnabler->showRtn)) == ERROR_FIND)
		{
		continue;
		}
	    else
		{
		break;
		}
	    }
	}

    if (pCard->initStatus == OK)
	status = OK;

    return (status);
    }

/*******************************************************************************
*
* cisFree - free tuples from the linked list
*
* This routine free tuples from the linked list.
*
* RETURNS: N/A
*
* INTERNAL
* This funcion should not do it by itself, it should called an enabler
* supplied destroy function to delete the device (if possible) in device
* dependent manner.
*/

void cisFree
    (
    int sock			/* socket no. */
    )

    {
    PCMCIA_CTRL *pCtrl	= &pcmciaCtrl;
    PCMCIA_CHIP *pChip	= &pCtrl->chip;
    PCMCIA_CARD *pCard	= &pCtrl->card[sock];
    PCMCIA_IOWIN iowin;
    PCMCIA_MEMWIN memwin;
    DL_NODE *pNode;
    DL_NODE *pTemp;
    int ix;


    semTake (&cisMuteSem, WAIT_FOREVER);	/* mutual exclusion begin */
    (void) (* pChip->cscOff) (sock, pChip->intLevel);

    pCard->type		= 0;
    pCard->sock		= 0;
    pCard->ctrl		= 0;
    pCard->detected	= FALSE;
    pCard->installed	= FALSE;
    pCard->changed	= TRUE;
    pCard->regBase	= 0;
    pCard->regMask	= 0;
    pCard->initStatus	= 0;
    pCard->cscIntr	= NULL;
    pCard->showRtn	= NULL;
    pCard->pResource	= NULL;

    if (pCard->pBlkDev != NULL)
        pCard->pBlkDev->bd_readyChanged = TRUE ;

    if (pCard->pNetIf != NULL)
        free ((char *)pCard->pNetIf);  /* XXX - lrn */
    pCard->pNetIf	= NULL;

    if (pCard->pDos != NULL)
	{
#if FALSE	/* DosFs 2.0 - can not remove dos device */
	iosDevDelete (&pCard->pDos->dosvd_devHdr);
	free ((char *)pCard->pDos);
#endif /*FALSE*/
	}
    pCard->pDos = NULL;

    for (pNode = DLL_FIRST(&pCard->cisTupleList); pNode != NULL; pNode = pTemp)
	{
	pTemp = DLL_NEXT(pNode);
	free (pNode);
	}
    dllInit (&pCard->cisTupleList);

    for (pNode = DLL_FIRST(&pCard->cisConfigList); pNode != NULL; pNode = pTemp)
	{
	pTemp = DLL_NEXT(pNode);
	free (pNode);
	}
    dllInit (&pCard->cisConfigList);

    (void) (* pChip->irqSet) (sock, 0);

    (void) (* pChip->flagSet) (sock, PC_PWR_AUTO);

    memwin.window	= 0;
    memwin.flags	= 0;
    memwin.extraws	= 0;
    memwin.start	= 0;
    memwin.stop		= 0;
    memwin.cardstart	= 0;
    for (ix = 0; ix < pChip->memWindows; ix++)
	{
        memwin.window	= ix;
        (void) (* pChip->memwinSet) (sock, &memwin);
	}

    iowin.window	= 0;
    iowin.flags		= 0;
    iowin.extraws	= 0;
    iowin.start		= 0;
    iowin.stop		= 0;
    for (ix = 0; ix < pChip->ioWindows; ix++)
	{
        iowin.window	= ix;
        (void) (* pChip->iowinSet) (sock, &iowin);
	}

    (void) (* pChip->cscPoll) (sock);

    (void) (* pChip->cscOn) (sock, pChip->intLevel);
    semGive (&cisMuteSem);			/* mutual exclusion end */
    }

/*******************************************************************************
*
* cisConfigregGet - get the PCMCIA configuration register
*
* This routine gets that PCMCIA configuration register.
*
* RETURNS: OK, or ERROR if it cannot set a value on the PCMCIA chip.
*/

STATUS cisConfigregGet
    (
    int sock,			/* socket no. */
    int reg,			/* configuration register no. */
    int *pValue			/* content of the register */
    )

    {
    PCMCIA_CTRL *pCtrl	= &pcmciaCtrl;
    PCMCIA_CHIP *pChip	= &pCtrl->chip;
    PCMCIA_CARD *pCard	= &pCtrl->card[sock];
    PCMCIA_MEMWIN memWin;
    char *pReg;

    if ((pCard->regBase == 0) || ((pCard->regMask & (1 << reg)) == 0) ||
        (pChip->installed != TRUE))
	return (ERROR);
    
    memWin.window	= PCMCIA_CIS_WINDOW;
    memWin.flags	= MAP_ACTIVE | MAP_16BIT | MAP_ATTRIB;
    memWin.extraws	= 2;
    memWin.start	= pcmciaMemwin[CIS_MEM_CONFIG].start;
    memWin.stop		= pcmciaMemwin[CIS_MEM_CONFIG].stop;
    memWin.cardstart	= pCard->regBase;
    if ((* pChip->memwinSet)(sock, &memWin) != OK)
	return (ERROR);

    pReg = (char *)memWin.start + (pCard->regBase & 0xfff) + (reg * 2) +
	   pCtrl->memBase;
    *pValue = *pReg;

    memWin.window	= 0;
    memWin.flags	= 0;
    memWin.extraws	= 0;
    memWin.start	= 0;
    memWin.stop		= 0;
    memWin.cardstart	= 0;
    if ((* pChip->memwinSet)(sock, &memWin) != OK)
	return (ERROR);
    
    return (OK);
    }

/*******************************************************************************
*
* cisConfigregSet - set the PCMCIA configuration register
*
* This routine sets the PCMCIA configuration register.
*
* RETURNS: OK, or ERROR if it cannot set a value on the PCMCIA chip.
*/

STATUS cisConfigregSet
    (
    int sock,			/* socket no. */
    int reg,			/* register no. */
    int value			/* content of the register */
    )

    {
    PCMCIA_CTRL *pCtrl	= &pcmciaCtrl;
    PCMCIA_CHIP *pChip	= &pCtrl->chip;
    PCMCIA_CARD *pCard	= &pCtrl->card[sock];
    PCMCIA_MEMWIN memWin;
    char *pReg;

    if ((pCard->regBase == 0) || ((pCard->regMask & (1 << reg)) == 0) ||
        (pChip->installed != TRUE))
	return (ERROR);
    
    memWin.window	= PCMCIA_CIS_WINDOW;
    memWin.flags	= MAP_ACTIVE | MAP_16BIT | MAP_ATTRIB;
    memWin.extraws	= 2;
    memWin.start	= pcmciaMemwin[CIS_MEM_CONFIG].start;
    memWin.stop		= pcmciaMemwin[CIS_MEM_CONFIG].stop;
    memWin.cardstart	= pCard->regBase;
    if ((* pChip->memwinSet)(sock, &memWin) != OK)
	return (ERROR);

    pReg = (char *)memWin.start + (pCard->regBase & 0xfff) + (reg * 2) +
	   pCtrl->memBase;
    *pReg = value;

    memWin.window	= 0;
    memWin.flags	= 0;
    memWin.extraws	= 0;
    memWin.start	= 0;
    memWin.stop		= 0;
    memWin.cardstart	= 0;
    if ((* pChip->memwinSet)(sock, &memWin) != OK)
	return (ERROR);
    
    return (OK);
    }

