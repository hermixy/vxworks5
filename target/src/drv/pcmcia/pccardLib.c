/* pccardLib.c - PC CARD enabler library */

/* Copyright 1984-1996 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01k,21jun00,rsh  upgrade to dosFs 2.0
01o,31aug99,jkf  changed from cbio.h to cbioLib.h
01n,03nov98,lrn  cleanup warnings
01m,06oct98,lrn  merge DosFs 2.0 changes into T2.0, DosFs 2.0 API for TFFS
01l,17sep98,lrn  reworked DosFs 2.0 support and memory leak issues
01k,13jul98,lrn  DosFs 2.0 - can not delete a DOS device
01j,19jan98,hdn  added a check to see if the socket is registered.
01i,11dec97,hdn  added TFFS support for flash PC card.
01h,19mar97,hdn  deleted a line that writes 0 to the ELT's address config reg.
01g,16jan97,hdn  initialized pcmciaCtrl.
01f,17nov96,jdi  doc: tweaks.
01e,08nov96,dgp  doc: final formatting
01d,28mar96,jdi  doc: cleaned up language and format.
01c,08mar96,hdn  added more descriptions.
01b,22feb96,hdn  cleaned up
01a,19oct95,hdn  written.
*/

/*
DESCRIPTION
This library provides generic facilities for enabling PC CARD.
Each PC card device driver needs to provide an enabler routine and
a CSC interrupt handler.  The enabler routine must be in the
`pccardEnabler' structure.
Each PC card driver has its own resource structure, `xxResources'.  The
ATA PC card driver resource structure is `ataResources' in
sysLib, which also supports a local IDE disk.
The resource structure has a PC card common resource structure in
the first member.  Other members are device-driver dependent resources.

The PCMCIA chip initialization routines tcicInit() and pcicInit() are
included in the PCMCIA chip table `pcmciaAdapter'.
This table is scanned when the PCMCIA library is initialized.  If the
initialization routine finds the PCMCIA chip, it registers all function
pointers of the PCMCIA_CHIP structure.

A memory window defined in `pcmciaMemwin' is used to access
the CIS of a PC card through the routines in cisLib.

SEE ALSO
pcmciaLib, cisLib, tcic, pcic

*/

/* LINTLIBRARY */

#include "private/funcBindP.h"
#include "drv/pcmcia/pcmciaLib.h"
#include "drv/pcmcia/cisLib.h"
#include "drv/pcmcia/pcic.h"
#include "drv/pcmcia/tcic.h"
#include "drv/pcmcia/sramDrv.h"
#include "drv/hdisk/ataDrv.h"
#include "cbioLib.h"
#include "dcacheCbio.h"
#include "dpartCbio.h"

#ifdef	INCLUDE_ELT
#include "net/if.h"
#include "netinet/if_ether.h"
#include "drv/netif/if_elt.h"
#endif	/* INCLUDE_ELT */

#ifdef	INCLUDE_TFFS
#include "tffs/tffsDrv.h"
#endif	/* INCLUDE_TFFS */


/* defines */

#define ELT_PCMCIA_ID0		0x0101	/* manufacturer code */
#define ELT_PCMCIA_ID1_562	0x0562	/* manufacturer info for 3C562 */
#define ELT_PCMCIA_ID1_589	0x0589	/* manufacturer info for 3C589B */

/* XXX it should go in pc386/pc.h */
#define TFFS0_MEM_START		0xd8000	/* mem start addr for TFFS in sock 0 */
#define TFFS0_MEM_STOP		0xd9fff	/* mem stop addr for TFFS in sock 0 */
#define TFFS1_MEM_START		0xda000	/* mem start addr for TFFS in sock 1 */
#define TFFS1_MEM_STOP		0xdbfff	/* mem stop addr for TFFS in sock 1 */


/* externs */
IMPORT BOOL pcmciaDebug;
/* dont use partition table decoder unless ATA is included */
#define	USE_PARTITIONS	FALSE

#ifdef	INCLUDE_ELT
IMPORT ELT_CTRL		*pEltCtrl [];
LOCAL STATUS	pccardEltCscIntr  (int sock, int csc);
#endif	/* INCLUDE_ELT */

#ifdef	INCLUDE_ATA
IMPORT ATA_CTRL		ataCtrl [ATA_MAX_CTRLS];
LOCAL STATUS	pccardAtaCscIntr  (int sock, int csc);
#undef	USE_PARTITIONS
#define	USE_PARTITIONS	TRUE
#endif	/* INCLUDE_ATA */

#ifdef	INCLUDE_SRAM
IMPORT SRAM_CTRL	sramCtrl;
IMPORT int		sramResourceNumEnt;
LOCAL STATUS	pccardSramCscIntr (int sock, int csc);
#endif	/* INCLUDE_SRAM */

#ifdef	INCLUDE_TFFS
IMPORT char		pcDriveNo [];
LOCAL STATUS	pccardTffsCscIntr (int sock, int csc);
#endif	/* INCLUDE_TFFS */


/* globals */

PCMCIA_CTRL pcmciaCtrl = {PCMCIA_SOCKS, PCMCIA_MEMBASE} ;

PCMCIA_MEMWIN pcmciaMemwin [] =
    {
    {0, 0, 0, CIS_MEM_START, CIS_MEM_STOP, 0},	/* CIS extraction */
    {0, 0, 0, CIS_REG_START, CIS_REG_STOP, 0},	/* config registers */
    };

PCMCIA_ADAPTER pcmciaAdapter [] =
    {
    {PCMCIA_PCIC, PCIC_BASE_ADR, PCIC_INT_VEC, PCIC_INT_LVL, pcicInit, NULL},
    {PCMCIA_TCIC, TCIC_BASE_ADR, TCIC_INT_VEC, TCIC_INT_LVL, tcicInit, NULL}
    };

#ifdef	INCLUDE_ELT
ELT_RESOURCE eltResources[] =
    {
    {
     {
     5, 0,
     {ELT0_IO_START, 0}, {ELT0_IO_STOP, 0}, 0,
     0, 0, 0, 0, 0
     },
     ELT0_INT_VEC, ELT0_INT_LVL, ELT0_NRF, ELT0_CONFIG
    },
    {
     {
     5, 0,
     {ELT1_IO_START, 0}, {ELT1_IO_STOP, 0}, 0,
     0, 0, 0, 0, 0
     },
     ELT1_INT_VEC, ELT1_INT_LVL, ELT1_NRF, ELT1_CONFIG
    }
    };
#endif	/* INCLUDE_ELT */

#ifdef	INCLUDE_SRAM
SRAM_RESOURCE sramResources[] =
    {
    {
     {
     5, 0,
     {0, 0}, {0, 0}, 0,
     SRAM0_MEM_START, SRAM0_MEM_STOP, 1, 0x0, SRAM0_MEM_LENGTH
     }
    },
    {
     {
     5, 0,
     {0, 0}, {0, 0}, 0,
     SRAM1_MEM_START, SRAM1_MEM_STOP, 1, 0x0, SRAM1_MEM_LENGTH
     }
    },
    {
     {
     5, 0,
     {0, 0}, {0, 0}, 0,
     SRAM2_MEM_START, SRAM2_MEM_STOP, 1, 0x0, SRAM2_MEM_LENGTH
     }
    },
    {
     {
     5, 0,
     {0, 0}, {0, 0}, 0,
     SRAM3_MEM_START, SRAM3_MEM_STOP, 1, 0x0, SRAM3_MEM_LENGTH
     }
    }
    };
#endif	/* INCLUDE_SRAM */

#ifdef	INCLUDE_TFFS
TFFS_RESOURCE tffsResources[] =
    {
    {
     {
     5 /* vcc */, 0 /* vpp */,
     {0, 0}, {0, 0}, 0,
     TFFS0_MEM_START, TFFS0_MEM_STOP, 2 /* extra wait state */,
     0x0 /* offset card address */, 0x0 /* size of the memory */
     }
    },
    {
     {
     5 /* vcc */, 0 /* vpp */,
     {0, 0}, {0, 0}, 0,
     TFFS1_MEM_START, TFFS1_MEM_STOP, 2 /* extra wait state */,
     0x0 /* offset card address */, 0x0 /* size of the memory */
     }
    }
    };
#endif	/* INCLUDE_TFFS */

PCCARD_ENABLER pccardEnabler [] =
    {
#ifdef	INCLUDE_ELT
    {
     PCCARD_LAN_ELT, (void *)eltResources, NELEMENTS(eltResources),
     (FUNCPTR)pccardEltEnabler, NULL
    },
#endif	/* INCLUDE_ELT */

#ifdef	INCLUDE_ATA
    {
     PCCARD_ATA, (void *)ataResources, NELEMENTS(ataResources),
     (FUNCPTR)pccardAtaEnabler, NULL
    },
#endif	/* INCLUDE_ATA */

#ifdef	INCLUDE_TFFS
    {
     PCCARD_FLASH, (void *)tffsResources, NELEMENTS(tffsResources),
     (FUNCPTR)pccardTffsEnabler, NULL
    },
#endif	/* INCLUDE_TFFS */

#ifdef	INCLUDE_SRAM
    {
     PCCARD_SRAM, (void *)sramResources, NELEMENTS(sramResources),
     (FUNCPTR)pccardSramEnabler, NULL
    }
#endif	/* INCLUDE_SRAM */
    };

int pcmciaAdapterNumEnt = NELEMENTS (pcmciaAdapter);
int pccardEnablerNumEnt = NELEMENTS (pccardEnabler);


/* locals */

#ifdef	INCLUDE_SRAM
LOCAL int sramSizeTable [] = {512, 2000, 8000, 32000,
			      128000, 512000, 2000000, 0};
#endif	/* INCLUDE_SRAM */

#ifdef INCLUDE_DOSFS
#define ATA_CACHE_SIZE	(128*1024)
#define SRAM_CACHE_SIZE	(2*1024)
#define TFFS_CACHE_SIZE	(32*1024)

CBIO_DEV_ID sockCbioDevs[ PCIC_MAX_SOCKS * NELEMENTS (pcmciaAdapter) ];
char * sockDevNames[ PCIC_MAX_SOCKS * NELEMENTS (pcmciaAdapter) ] =
		 { "/card0", "/card1", "/card2", "/card3",
    	     	   "/card4", "/card5", "/card6", "/card7", NULL };
#endif /* INCLUDE_DOSFS */
#ifdef INCLUDE_ATA
#endif

#ifdef INCLUDE_SRAM
#endif

#ifdef INCLUDE_ELT
#endif

#ifdef INCLUDE_TFFS
#endif

/*******************************************************************************
*
* pccardMount - mount a DOS file system
*
* This routine mounts a DOS file system.
*
* RETURNS: OK or ERROR.
*/
STATUS pccardMount
    (
    int	 sock,			/* socket number */
    char *pName			/* name of a device */
    )

    {

    printErr( "This function is discontinued\n" );
    return ERROR;

#if FALSE
    PCMCIA_CTRL *pCtrl		= &pcmciaCtrl;
    PCMCIA_CHIP *pChip		= &pCtrl->chip;
    PCMCIA_CARD *pCard		= &pCtrl->card[sock];
    DOS_VOL_DESC *pDos		= pCard->pDos;

    if ((!pChip->installed) || (!pCard->installed) || (sock >= pChip->socks))
        return (ERROR);

    if (pDos != NULL)
	{
	iosDevDelete (&pDos->dosvd_devHdr);
	free ((char *)pDos);
	return (ERROR);
	}

    pCard->pDos = dosFsDevInit (pName, pCard->pBlkDev, NULL);
    if (pCard->pDos == NULL)
	return (ERROR);

    return (OK);
#endif /* FALSE */
    }

/*******************************************************************************
*
* pccardMkfs - initialize a device and mount a DOS file system
*
* This routine initializes a device and mounts a DOS file system.
*
* RETURNS: OK or ERROR.
*/
STATUS pccardMkfs
    (
    int	 sock,			/* socket number */
    char *pName			/* name of a device */
    )

    {

    printErr( "This function is discontinued. Use dosFsVolFormat()\n" );
    return ERROR;

#if FALSE
    PCMCIA_CTRL *pCtrl		= &pcmciaCtrl;
    PCMCIA_CHIP *pChip		= &pCtrl->chip;
    PCMCIA_CARD *pCard		= &pCtrl->card[sock];
    DOS_VOL_DESC *pDos		= pCard->pDos;

    if ((!pChip->installed) || (!pCard->installed) || (sock >= pChip->socks))
        return (ERROR);

    if (pDos != NULL)
	{
	iosDevDelete (&pDos->dosvd_devHdr);
	free ((char *)pDos);
	return (ERROR);
	}

    pCard->pDos = dosFsMkfs (pName, pCard->pBlkDev);
    if (pCard->pDos == NULL)
	return (ERROR);

    return (OK);
#endif /* FALSE */
    }

#ifdef INCLUDE_DOSFS
/*******************************************************************************
*
* pccardDosDevCreate - create DOSFS device for on PCMCIA socket.
*
* This routine creates a DOS FS device for PCMCIA socket # <sock>
* with predefined name based on socket number.  The device will use
* device driver pointed by <pBlkDev> to access device on the low level.
*
* If DOS device already exists, only low level driver is changed to
* reflect the new devices physical parameters.
*
* RETURNS: STATUS.
*/
LOCAL STATUS pccardDosDevCreate
    (
    int		sock,
    void *	pBlkDev,
    int		cacheSize,
    BOOL	partitions
    )
    {
    void * subDev = NULL ;
    void * masterCbio ;
    IMPORT STATUS usrFdiskPartRead();
    STATUS stat;

    /* create cache of appropriate size and DOS device on top of it */

    if ( sockCbioDevs[ sock ] == NULL )
    	{

    	sockCbioDevs[ sock ] =
    	    	dcacheDevCreate(pBlkDev, 0, cacheSize,
    	    	    	    	 sockDevNames[ sock ] );
    	if( sockCbioDevs[ sock ] == NULL )
    	    {
    	    printErr ("Error during dcacheDevCreate: %p\n",
    	    	      (void *)errno );
    	    return (ERROR);
    	    }
   	if ( partitions )
	    {
    	    masterCbio = dpartDevCreate(sockCbioDevs[ sock ] ,
			 1, (FUNCPTR) usrFdiskPartRead );

    	    if( masterCbio == NULL)
            	{
	    	printErr ("Error creating partition manager: %x\n", errno);
            	return (ERROR);
            	}

	    subDev = (void *) dpartPartGet(masterCbio,0);
	    }
	else
	    {
	    subDev = sockCbioDevs[ sock ];
	    }
    	return dosFsDevCreate( sockDevNames[ sock ],
    	    	    	       subDev, 0,0 );
    	}

    /* resize cache and RESET device */
    dcacheDevMemResize (sockCbioDevs[ sock ], cacheSize);

    stat = cbioIoctl (sockCbioDevs[ sock ], CBIO_RESET, pBlkDev );

    if( stat != OK )
	{
	printErr("pccardDosDevCreate: error substituting block driver, "
		"errno=%#x\n", errno);
	}

    return stat;
    } /* pccardDosDevCreate() */
#endif /* INCLUDE_DOSFS */

#ifdef	INCLUDE_ATA
/*******************************************************************************
*
* pccardAtaEnabler - enable the PCMCIA-ATA device
*
* This routine enables the PCMCIA-ATA device.
*
* RETURNS:
* OK, ERROR_FIND if there is no ATA card, or ERROR if another error occurs.
*/
STATUS pccardAtaEnabler
    (
    int		 sock,		/* socket no. */
    ATA_RESOURCE *pAtaResource,	/* pointer to ATA resources */
    int		 numEnt,	/* number of ATA resource entries */
    FUNCPTR	 showRtn 	/* ATA show routine */
    )

    {
    PCMCIA_CTRL *pCtrl		= &pcmciaCtrl;
    PCMCIA_CHIP *pChip		= &pCtrl->chip;
    PCMCIA_CARD *pCard		= &pCtrl->card[sock];
    CIS_CONFIG  *pConfig	= (CIS_CONFIG *)&pCard->cisConfigList;
    PCCARD_RESOURCE *pResource	= &pAtaResource->resource;
    int drive			= 0;
    int functionCode		= 0;
    int diskInterface		= 0;
    PCMCIA_IOWIN pcmciaIowin;
    DL_LIST *pList;
    DL_NODE *pNode;
    CIS_TUPLE *pTuple;
    ATA_CTRL *pAtaCtrl;
    int flag;
    int ctrl;
    int sock0;
    char *pChar;

    if (!pChip->installed)
	return (ERROR);

    pList = &pCard->cisTupleList;
    for (pNode = DLL_FIRST (pList); pNode != NULL; pNode = DLL_NEXT(pNode))
	{
	pTuple	= (CIS_TUPLE *)((char *)pNode + sizeof (DL_NODE));
	pChar	= (char *)pTuple + sizeof (CIS_TUPLE);

	switch (pTuple->code)
	    {
	    case CISTPL_FUNCID:
		functionCode = *pChar;
		break;

	    case CISTPL_FUNCE:
		if (*pChar++ == FUNCE_TYPE_DISK)
		    diskInterface = *pChar;
		break;
	    }
	}

    if ((pcmciaDebug) && (_func_logMsg != NULL))
	(* _func_logMsg) ("ATA sock=%d functionCode=%x diskInterface=%x\n",
			      sock, functionCode, diskInterface, 0, 0, 0);
    /* return if we didn't recognize the card */

    if ((functionCode != FUNC_FIXEDDISK) || (diskInterface != FUNCE_DATA_ATA))
	return (ERROR_FIND);

    if ((pcmciaDebug) && (_func_logMsg != NULL))
	(* _func_logMsg) ("ATA sock=%d detected\n",
			      sock, 0, 0, 0, 0, 0);
    /* get un-used resource */

    for (ctrl = 0; ctrl < numEnt; ctrl++)
	{
        pResource = &pAtaResource->resource;

	if (pAtaResource->ctrlType == ATA_PCMCIA)
	    {
	    for (sock0 = 0; sock0 < pChip->socks; sock0++)
		{
    		PCMCIA_CARD *pCard0 = &pCtrl->card[sock0];
		if (pCard0->pResource == pResource)	/* used resource */
		    break;
		}
	    if (sock0 == pChip->socks)			/* un-used resource */
		break;
	    }

	pAtaResource++;
	}

    if (ctrl == numEnt)
	{
	if (_func_logMsg != NULL)
	    (* _func_logMsg) ("pccardAtaEnabler: sock=%d out of resource\n",
			      sock, 0, 0, 0, 0, 0);
	return (ERROR);
	}

    /* configure the card with the resource */

    pList = &pCard->cisConfigList;
    for (pNode = DLL_FIRST(pList); pNode != NULL; pNode = DLL_NEXT(pNode))
	{
	pConfig = (CIS_CONFIG *)pNode;

	if ((pConfig->ioRanges == 2) &&
	    (pConfig->io[0].start == pResource->ioStart[0]) &&
	    (pConfig->io[1].start == pResource->ioStart[1]))
	    {
	    pAtaCtrl		= &ataCtrl[ctrl];
	    pAtaCtrl->ctrlType	= ATA_PCMCIA;

	    pCard->type		= PCCARD_ATA;
	    pCard->sock		= sock;
	    pCard->ctrl		= ctrl;
	    pCard->detected	= TRUE;
	    pCard->pResource	= pResource;
            pCard->cardStatus	= (* pChip->status)(sock);
	    pCard->cscIntr	= (FUNCPTR)pccardAtaCscIntr;
	    pCard->showRtn	= (FUNCPTR)showRtn;

	    flag = PC_PWR_AUTO | PC_IOCARD;
	    if (pResource->vcc == PCCARD_5V)
	        flag |= PC_VCC_5V;
	    else if (pResource->vcc == PCCARD_3V)
	        flag |= PC_VCC_3V;
	    if (pResource->vpp == PCCARD_5V)
	        flag |= PC_VPP_5V;
	    else if (pResource->vpp == PCCARD_12V)
	        flag |= PC_VPP_12V;
	    if ((* pChip->flagSet)(sock, flag) != OK)
	        return (ERROR);

	    cisConfigregSet (sock, CONFIG_STATUS_REG, 0x00);
	    cisConfigregSet (sock, PIN_REPLACEMENT_REG,
			     PRR_RBVD1 | PRR_RBVD2 | PRR_RRDY | PRR_RWPROT);
	    cisConfigregSet (sock, SOCKET_COPY_REG, 0x00 | (sock & 0x0f));
	    cisConfigregSet (sock, CONFIG_OPTION_REG,
			     COR_LEVIREQ | pConfig->index);

	    pcmciaIowin.window	= PCCARD_IOWIN0;
	    pcmciaIowin.flags	= MAP_ACTIVE | MAP_16BIT;
	    pcmciaIowin.extraws	= pResource->ioExtraws;
	    pcmciaIowin.start	= pResource->ioStart[0];
	    pcmciaIowin.stop	= pResource->ioStop[0];
	    if ((* pChip->iowinSet)(sock, &pcmciaIowin) != OK)
	        return (ERROR);

	    pcmciaIowin.window	= PCCARD_IOWIN1;
	    pcmciaIowin.flags	= MAP_ACTIVE | MAP_16BIT;
	    pcmciaIowin.extraws	= pResource->ioExtraws;
	    pcmciaIowin.start	= pResource->ioStart[1];
	    pcmciaIowin.stop	= pResource->ioStop[1];
	    if ((* pChip->iowinSet)(sock, &pcmciaIowin) != OK)
	        return (ERROR);

	    if ((* pChip->irqSet)(sock, pAtaResource->intLevel) != OK)
	        return (ERROR);

	    if ((pCard->initStatus = ataDrv (ctrl,
					     pAtaResource->drives,
					     pAtaResource->intVector,
					     pAtaResource->intLevel,
					     pAtaResource->configType,
					     pAtaResource->semTimeout,
					     pAtaResource->wdgTimeout
					     )) != OK)
		{
    		if (_func_logMsg != NULL)
			(* _func_logMsg) ("ATA sock=%d ataDrv failed %x\n",
			      sock, errno, 0, 0, 0, 0);
	        return (ERROR);
		}

	    pCard->installed = TRUE;
    	    taskDelay (sysClkRateGet());		/* 1 sec */
	    break;
	    }
	}

    /* return if we didn't install the driver */

    if (!pCard->installed)
	return (ERROR);

    if (pCard->pBlkDev != NULL)
	free ((char *)pCard->pBlkDev);

    if ((pCard->pBlkDev = ataDevCreate(ctrl, drive, 0, 0)) ==
	(BLK_DEV *)NULL)
        {
        printErr ("Error during ataDevCreate: %x\n", errno);
	return (ERROR);
        }

#ifdef INCLUDE_DOSFS
    {
    return pccardDosDevCreate( sock, pCard->pBlkDev,
	ATA_CACHE_SIZE, USE_PARTITIONS );
    }
#else
    printErr ("DosFs not included, card in socket %d ignored\n", sock);

    return (OK);
#endif /* INCLUDE_DOSFS */
    }

/*******************************************************************************
*
* pccardAtaCscIntr - ATA controller PCMCIA card status change interrupt handler.
*
* RETURNS: OK (always).
*/

LOCAL STATUS pccardAtaCscIntr
    (
    int sock,			/* socket no. */
    int csc			/* CSC bits */
    )

    {
    PCMCIA_CTRL *pCtrl		= &pcmciaCtrl;
    PCMCIA_CARD *pCard		= &pCtrl->card[sock];
    ATA_RESOURCE *pAtaResource	= (ATA_RESOURCE *)pCard->pResource;
    ATA_CTRL *pAtaCtrl		= &ataCtrl[pCard->ctrl];


    /* hot insertion */


    /* hot removal */

    if ((csc & PC_DETECT) && ((pCard->cardStatus & PC_DETECT) == 0x0))
	{
        if (pAtaResource != NULL)
            sysIntDisablePIC (pAtaResource->intLevel);

	pAtaCtrl->installed	= FALSE;
	pAtaCtrl->changed	= FALSE;

	semFlush (&pAtaCtrl->syncSem);
	pcmciaJobAdd ((VOIDFUNCPTR)semTerminate, (int)&pAtaCtrl->muteSem,
		      0,0,0,0,0);
	}

    return (OK);
    }
#endif	/* INCLUDE_ATA */

#ifdef	INCLUDE_SRAM
/*******************************************************************************
*
* pccardSramEnabler - enable the PCMCIA-SRAM driver
*
* This routine enables the PCMCIA-SRAM driver.
*
* RETURNS:
* OK, ERROR_FIND if there is no SRAM card, or ERROR if another error occurs.
*/

STATUS pccardSramEnabler
    (
    int		  sock,		  /* socket no. */
    SRAM_RESOURCE *pSramResource, /* pointer to SRAM resources */
    int		  numEnt,	  /* number of SRAM resource entries */
    FUNCPTR	  showRtn 	  /* SRAM show routine */
    )

    {
    PCMCIA_CTRL *pCtrl		= &pcmciaCtrl;
    PCMCIA_CHIP *pChip		= &pCtrl->chip;
    PCMCIA_CARD *pCard		= &pCtrl->card[sock];
    BOOL typeExtended		= FALSE;
    BOOL speedExtended		= FALSE;
    int formatOffset		= 0;
    int formatNbytes		= 0;
    int deviceType		= 0;
    int deviceSize		= 0;
    int functionCode		= NONE;
    PCCARD_RESOURCE *pResource;
    DL_NODE *pNode;
    CIS_TUPLE *pTuple;
    CIS_BYTE4 offset;
    CIS_BYTE4 nbytes;
    int unit;
    int size;
    int flag;
    char *pChar;

    if ((!pChip->installed) || (sock >= numEnt))
	return (ERROR);

    for (pNode = DLL_FIRST (&pCard->cisTupleList);
	 pNode != NULL;
	 pNode = DLL_NEXT(pNode))
	{
	pTuple	= (CIS_TUPLE *)((char *)pNode + sizeof (DL_NODE));
	pChar	= (char *)pTuple + sizeof (CIS_TUPLE);

	switch (pTuple->code)
	    {
	    case CISTPL_FUNCID:
		functionCode = *pChar;
		break;

	    case CISTPL_DEVICE:
		deviceType = *pChar & 0xf0;
		if (deviceType == DTYPE_EXTEND)
		    typeExtended = TRUE;
		if ((*pChar & 0x0f) == DSPEED_EXT)
		    speedExtended = TRUE;
		pChar++;
		while (speedExtended)
		    if ((*pChar++ & 0x80) == 0)
			speedExtended = FALSE;
		while (typeExtended)
		    if ((*pChar++ & 0x80) == 0)
			typeExtended = FALSE;
		unit = ((*pChar & 0xf8) >> 3) + 1;
		size = sramSizeTable [*pChar & 0x07];
		deviceSize = unit * size;
		break;

	    case CISTPL_FORMAT:
		pChar += 2;
		offset.c[0] = *pChar++;
		offset.c[1] = *pChar++;
		offset.c[2] = *pChar++;
		offset.c[3] = *pChar++;
		nbytes.c[0] = *pChar++;
		nbytes.c[1] = *pChar++;
		nbytes.c[2] = *pChar++;
		nbytes.c[3] = *pChar++;
		formatOffset = offset.l;
		formatNbytes = nbytes.l;
		break;
	    }
	}

    /* configure the card with a resource which is assigned for the socket */

    if (((functionCode == NONE) || (functionCode == FUNC_MEMORY)) &&
        ((deviceType == DTYPE_SRAM) || (formatNbytes != 0)))
	{
        sramResourceNumEnt	= numEnt;
        pSramResource		+= sock;
        pResource		= &pSramResource->resource;
        pCard->type		= PCCARD_SRAM;
        pCard->sock		= sock;
        pCard->detected		= TRUE;
        pCard->pResource	= pResource;
        pCard->cardStatus	= (* pChip->status)(sock);
        pCard->cscIntr		= (FUNCPTR)pccardSramCscIntr;
        pCard->showRtn		= (FUNCPTR)showRtn;
        pResource->memStart	&= 0xfffff000;
        pResource->memStop	|= 0x00000fff;
        pResource->memOffset	= formatOffset;
        if (deviceSize != 0)
	    pResource->memLength = deviceSize;
        else if (formatNbytes != 0)
	    pResource->memLength = formatNbytes;
	else
	    return (ERROR);

        flag = PC_PWR_AUTO;
        if (pResource->vcc == PCCARD_5V)
	    flag |= PC_VCC_5V;
        else if (pResource->vcc == PCCARD_3V)
	    flag |= PC_VCC_3V;
        if (pResource->vpp == PCCARD_5V)
	    flag |= PC_VPP_5V;
        else if (pResource->vpp == PCCARD_12V)
	    flag |= PC_VPP_12V;
        if ((* pChip->flagSet)(sock, flag) != OK)
	    return (ERROR);

        if ((pCard->initStatus = sramDrv (sock)) != OK)
	    return (ERROR);

        pCard->installed = TRUE;
	}
    else
	return (ERROR_FIND);

    /* return if we didn't recognize the card or didn't install the driver */

    if (!pCard->installed)
	return (ERROR);

    if (pCard->pBlkDev != NULL)
	free ((char *)pCard->pBlkDev);

    size = pResource->memLength / DEFAULT_SEC_SIZE - 1;	/* number of blocks */
    pCard->pBlkDev = sramDevCreate (sock, DEFAULT_SEC_SIZE, size, size, 0);
    if (pCard->pBlkDev == (BLK_DEV *)NULL)
	{
	printErr ("Error during sramDevCreate: %x\n", errno);
	return (ERROR);
	}

#ifdef INCLUDE_DOSFS
    /*
     * init DOS device with new block device driver and
     * valid disk cache size
     */
    return pccardDosDevCreate( sock, pCard->pBlkDev,
	SRAM_CACHE_SIZE, USE_PARTITIONS );
#else
    return (OK);
#endif /* INCLUDE_DOSFS */
    }

/*******************************************************************************
*
* pccardSramCscIntr - PCMCIA memory card status change interrupt handler
*
* RETURNS: OK, or ERROR if the CSC event is Ready.
*/

LOCAL STATUS pccardSramCscIntr
    (
    int sock,			/* socket no. */
    int csc			/* CSC bits */
    )

    {
    PCMCIA_CTRL *pCtrl		= &pcmciaCtrl;
    PCMCIA_CARD *pCard		= &pCtrl->card[sock];
    SEM_ID muteSemID		= &sramCtrl.muteSem[sock];
    SEM_ID syncSemID		= &sramCtrl.syncSem[sock];


    /* hot insertion */


    /* hot removal */

    if ((csc & PC_DETECT) && ((pCard->cardStatus & PC_DETECT) == 0x0))
	{
	semFlush (syncSemID);
	pcmciaJobAdd ((VOIDFUNCPTR)semTerminate, (int)muteSemID, 0, 0, 0, 0, 0);
	}

    /* card is ready */

    if (csc & PC_READY)
	{
	semGive (syncSemID);
	return (ERROR);
	}

    /* other card status changes */

    if ((csc & PC_BATDEAD) && (_func_logMsg != NULL))
	(* _func_logMsg) ("sramDrv: socket=%d Battery dead\n",
			  sock, 0, 0, 0, 0, 0);

    if ((csc & PC_BATWARN) && (_func_logMsg != NULL))
	(* _func_logMsg) ("sramDrv: socket=%d Battery warn\n",
			  sock, 0, 0, 0, 0, 0);

    if ((csc & PC_WRPROT) && (_func_logMsg != NULL))
	(* _func_logMsg) ("sramDrv: socket=%d Write protect\n",
			  sock, 0, 0, 0, 0, 0);

    return (OK);
    }
#endif	/* INCLUDE_SRAM */

#ifdef	INCLUDE_ELT
/*******************************************************************************
*
* pccardEltEnabler - enable the PCMCIA Etherlink III card
*
* This routine enables the PCMCIA Etherlink III (ELT) card.
*
* RETURNS:
*
* OK, ERROR_FIND if there is no ELT card, or ERROR if another error occurs.
*/
STATUS pccardEltEnabler
    (
    int		 sock,		/* socket no. */
    ELT_RESOURCE *pEltResource, /* pointer to ELT resources */
    int		 numEnt,	/* number of ELT resource entries */
    FUNCPTR	 showRtn 	/* show routine */
    )

    {
    PCMCIA_CTRL *pCtrl		= &pcmciaCtrl;
    PCMCIA_CHIP *pChip		= &pCtrl->chip;
    PCMCIA_CARD *pCard		= &pCtrl->card[sock];
    DL_LIST *pList		= &pCard->cisTupleList;
    PCCARD_RESOURCE *pResource	= &pEltResource->resource;
    short manufacturerID0	= 0;
    short manufacturerID1	= 0;
    int functionCode		= 0;
    DL_NODE *pNode;
    PCMCIA_IOWIN pcmciaIowin;
    CIS_TUPLE *pTuple;
    char *pChar;
    int flag;
    int ctrl;
    int sock0;

    if (!pChip->installed)
	return (ERROR);

    for (pNode = DLL_FIRST (pList); pNode != NULL; pNode = DLL_NEXT(pNode))
	{
	pTuple	= (CIS_TUPLE *)((char *)pNode + sizeof (DL_NODE));
	pChar	= (char *)pTuple + sizeof (CIS_TUPLE);

	switch (pTuple->code)
	    {
	    case CISTPL_FUNCID:
		functionCode = *pChar;
		break;

	    case CISTPL_MANFID:
		manufacturerID0 = *(short *)pChar;
		manufacturerID1 = *(short *)(pChar+2);
		break;
	    }
	}

    /* return if we didn't recognize the card */

    if ((functionCode != FUNC_LAN) ||
        (manufacturerID0 != ELT_PCMCIA_ID0) ||
        ((manufacturerID1 != ELT_PCMCIA_ID1_589) &&
	 (manufacturerID1 != ELT_PCMCIA_ID1_562)))
	return (ERROR_FIND);

    /* get un-used resource */

    for (ctrl = 0; ctrl < numEnt; ctrl++)
	{
        pResource = &pEltResource->resource;

	for (sock0 = 0; sock0 < pChip->socks; sock0++)
	    {
    	    PCMCIA_CARD *pCard0 = &pCtrl->card[sock0];
	    if (pCard0->pResource == pResource)
		break;
	    }
	if (sock0 == pChip->socks)
	    break;

	pEltResource++;
	}

    if (ctrl == numEnt)
	{
	if (_func_logMsg != NULL)
	    (* _func_logMsg) ("pccardEltEnabler: sock=%d out of resource\n",
			      sock, 0, 0, 0, 0, 0);
	return (ERROR);
	}

    /* configure the card with the resource */

    pCard->type		= PCCARD_LAN_ELT;
    pCard->sock		= sock;
    pCard->ctrl		= ctrl;
    pCard->detected	= TRUE;
    pCard->pResource	= pResource;
    pCard->cardStatus	= (* pChip->status)(sock);
    pCard->cscIntr	= (FUNCPTR)pccardEltCscIntr;
    pCard->showRtn	= (FUNCPTR)showRtn;

    flag = PC_PWR_AUTO | PC_IOCARD;
    if (pResource->vcc == PCCARD_5V)
        flag |= PC_VCC_5V;
    else if (pResource->vcc == PCCARD_3V)
        flag |= PC_VCC_3V;
    if (pResource->vpp == PCCARD_5V)
        flag |= PC_VPP_5V;
    else if (pResource->vpp == PCCARD_12V)
        flag |= PC_VPP_12V;
    if ((* pChip->flagSet)(sock, flag) != OK)
        return (ERROR);

    cisConfigregSet (sock, CONFIG_STATUS_REG, 0x00);
    cisConfigregSet (sock, CONFIG_OPTION_REG, COR_LEVIREQ | 0x03);

    pcmciaIowin.window	= PCCARD_IOWIN0;
    pcmciaIowin.flags	= MAP_ACTIVE | MAP_16BIT;
    pcmciaIowin.extraws	= pResource->ioExtraws;
    pcmciaIowin.start	= pResource->ioStart[0];
    pcmciaIowin.stop	= pResource->ioStop[0];
    if ((* pChip->iowinSet)(sock, &pcmciaIowin) != OK)
        return (ERROR);

    if ((* pChip->irqSet)(sock, pEltResource->intLevel) != OK)
        return (ERROR);

    sysOutWord (pResource->ioStart[0] + ELT_COMMAND, SELECT_WINDOW|WIN_CONFIG);
    sysOutWord (pResource->ioStart[0] + RESOURCE_CONFIG, 0x3f00);


    if (pCard->pNetIf != NULL)
	free (pCard->pNetIf);

    if ((pCard->pNetIf = malloc (sizeof(NETIF))) == NULL)
	return (ERROR);

    pCard->pNetIf->ifName	= "pcmcia";
    pCard->pNetIf->attachRtn	= eltattach;
    pCard->pNetIf->arg1		= (char *)pResource->ioStart[0];
    pCard->pNetIf->arg2		= pEltResource->intVector;
    pCard->pNetIf->arg3		= pEltResource->intLevel;
    pCard->pNetIf->arg4		= pEltResource->rxFrames;
    pCard->pNetIf->arg5		= pEltResource->connector;
    pCard->pNetIf->arg6		= (int)pCard->pNetIf->ifName;
    pCard->pNetIf->arg7		= 0;
    pCard->pNetIf->arg8		= 0;

    pCard->installed		= TRUE;

    return (OK);
    }

/*******************************************************************************
*
* pccardEltCscIntr - ELT controller PCMCIA card status change interrupt handler.
*
* RETURNS: OK (always).
*/

LOCAL STATUS pccardEltCscIntr
    (
    int sock,			/* socket no. */
    int csc			/* CSC bits */
    )

    {
    PCMCIA_CTRL *pCtrl		= &pcmciaCtrl;
    PCMCIA_CARD *pCard		= &pCtrl->card[sock];
    ELT_RESOURCE *pEltResource	= (ELT_RESOURCE *)pCard->pResource;
    ELT_CTRL *pDrvCtrl		= pEltCtrl [pCard->ctrl];


    /* card is inserted */


    /* card is removed */

    if ((csc & PC_DETECT) && ((pCard->cardStatus & PC_DETECT) == 0x0))
	{
        if (pCard->pResource != 0)
            sysIntDisablePIC (pEltResource->intLevel);

        pDrvCtrl->idr.ac_if.if_flags	= 0;
        pDrvCtrl->attached		= FALSE;
	}

    return (OK);
    }
#endif	/* INCLUDE_ELT */

#ifdef	INCLUDE_TFFS
/*******************************************************************************
*
* pccardTffsEnabler - enable the PCMCIA-TFFS driver
*
* This routine enables the PCMCIA-TFFS driver.
*
* RETURNS:
* OK, ERROR_FIND if there is no TFFS(Flash) card, or ERROR if another error occurs.
*/

STATUS pccardTffsEnabler
    (
    int		  sock,		  /* socket no. */
    TFFS_RESOURCE *pTffsResource, /* pointer to TFFS resources */
    int		  numEnt,	  /* number of SRAM resource entries */
    FUNCPTR	  showRtn 	  /* TFFS show routine */
    )

    {
    PCMCIA_CTRL *pCtrl		= &pcmciaCtrl;
    PCMCIA_CHIP *pChip		= &pCtrl->chip;
    PCMCIA_CARD *pCard		= &pCtrl->card[sock];
    int deviceType		= 0;
    PCCARD_RESOURCE *pResource;
    DL_NODE *pNode;
    CIS_TUPLE *pTuple;
    int flag;
    char *pChar;

    if ((!pChip->installed) || (sock >= numEnt))
	return (ERROR);

    for (pNode = DLL_FIRST (&pCard->cisTupleList);
	 pNode != NULL;
	 pNode = DLL_NEXT(pNode))
	{
	pTuple	= (CIS_TUPLE *)((char *)pNode + sizeof (DL_NODE));
	pChar	= (char *)pTuple + sizeof (CIS_TUPLE);

	switch (pTuple->code)
	    {
	    case CISTPL_DEVICE:
		deviceType = *pChar & 0xf0;
		break;
	    }
	}

    /* configure the card with a resource which is assigned for the socket */

    if (deviceType == DTYPE_FLASH)
	{
        pTffsResource		+= sock;
        pResource		= &pTffsResource->resource;
        pCard->type		= PCCARD_FLASH;
        pCard->sock		= sock;
        pCard->detected		= TRUE;
        pCard->pResource	= pResource;
        pCard->cardStatus	= (* pChip->status)(sock);
        pCard->cscIntr		= (FUNCPTR)pccardTffsCscIntr;
        pCard->showRtn		= (FUNCPTR)showRtn;
        pResource->memStart	&= 0xfffff000;
        pResource->memStop	|= 0x00000fff;

        flag = PC_PWR_AUTO;
        if (pResource->vcc == PCCARD_5V)
	    flag |= PC_VCC_5V;
        else if (pResource->vcc == PCCARD_3V)
	    flag |= PC_VCC_3V;
        if ((* pChip->flagSet)(sock, flag) != OK)
	    return (ERROR);

        if ((pCard->initStatus = tffsDrv ()) != OK)	/* just in case */
	    return (ERROR);

        pCard->installed = TRUE;
	}
    else
	return (ERROR_FIND);

    /* return if we didn't recognize the card or didn't install the driver */

    if (!pCard->installed)
	return (ERROR);

    /* we assume that a media on the socket is formated by tffsDevFormat() */

    if (pcDriveNo[sock] == NONE)
	{
	printErr ("pccardTffs:pcDriveNo[%d] == NONE\n", sock);
	return ERROR ;
	}

    if (pCard->pBlkDev != NULL)
	free ((char *)pCard->pBlkDev);

    pCard->pBlkDev = tffsDevCreate (pcDriveNo[sock], TRUE);
    if (pCard->pBlkDev == (BLK_DEV *)NULL)
	    {
	    printErr ("Error during tffsDevCreate: %x\n", errno);
	    return (ERROR);
	    }
    if ((pcmciaDebug) && (_func_logMsg != NULL))
	(* _func_logMsg) ("TFFS sock=%d blkDev=%x\n",
			      sock,pCard->pBlkDev , 0, 0, 0, 0);

#ifdef INCLUDE_DOSFS
    /*
     * init DOS device with new block device driver and
     * valid disk cache size
     */
    return pccardDosDevCreate( sock, pCard->pBlkDev,
	TFFS_CACHE_SIZE, USE_PARTITIONS );
#else
    printErr ("DosFs not included, card in socket %d ignored\n", sock);
#endif /* INCLUDE_DOSFS */
    return (OK);
    }

/*******************************************************************************
*
* pccardTffsCscIntr - PCMCIA flash card status change interrupt handler
*
* RETURNS: OK, or ERROR if the CSC event is Ready.
*/

LOCAL STATUS pccardTffsCscIntr
    (
    int sock,			/* socket no. */
    int csc			/* CSC bits */
    )

    {
    PCMCIA_CTRL *pCtrl		= &pcmciaCtrl;
    PCMCIA_CARD *pCard		= &pCtrl->card[sock];


    /* hot insertion */


    /* hot removal */

    if ((csc & PC_DETECT) && ((pCard->cardStatus & PC_DETECT) == 0x0))
	{
	}

    /* card is ready */

    if (csc & PC_READY)
	{
	return (ERROR);
	}

    /* other card status changes */

    if ((csc & PC_WRPROT) && (_func_logMsg != NULL))
	(* _func_logMsg) ("TFFS: socket=%d Write protect\n",
			  sock, 0, 0, 0, 0, 0);

    return (OK);
    }
#endif	/* INCLUDE_TFFS */

