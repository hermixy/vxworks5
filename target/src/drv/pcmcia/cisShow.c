/* cisShow.c - PCMCIA CIS show library */

/* Copyright 1984-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01e,10oct01,dat  SPR 70829, add note about VX_FP_TASK being necessary
01d,21jun00,rsh  upgrade to dosFs 2.0
01c,28mar96,jdi  doc: cleaned up language and format.
01b,22feb96,hdn  cleaned up
01a,09feb95,hdn  written.
*/


/*
DESCRIPTION
This library provides a show routine for CIS tuples. This is provided
for engineering debug use.

This module uses floating point calculations.  Any task calling cisShow()
needs to have the VX_FP_TASK bit set in the task flags.
*/


#include "vxWorks.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "drv/pcmcia/pcmciaLib.h"
#include "drv/pcmcia/cisLib.h"


/* defines */


/* imports */

IMPORT PCMCIA_CTRL	pcmciaCtrl;


/* globals */



/* locals */

LOCAL u_char cisFuncid		= 0;
LOCAL CIS_TUPLE *cisCdefault	= (CIS_TUPLE *)0;
LOCAL u_char systemInitByte	= 0;
LOCAL u_int deviceInfos		= 0;
LOCAL double speedMantisa []	= {0.0, 1.0, 1.2, 1.3, 1.5, 2.0, 2.5, 3.0,
			           3.5, 4.0, 4.5, 5.0, 5.5, 6.0, 7.0, 8.0 };
LOCAL double speedExponent []	= {1.0e-9, 10.0e-9, 100.0e-9, 1.0e-6,
			    	   10.0e-6, 100.0e-6, 1.0e-3, 10.0e-3 };
LOCAL u_int deviceSize []	= {512, 2000, 8000, 32000, 128000,
			    	   512000, 2000000, 0 };
LOCAL char *interfaceType[]	= {"memory", "IO", "reserved", "reserved",
			    	   "custom interface 0", "custom interface 1",
			    	   "custom interface 2", "custom interface 3" };


/* forward declarations */

LOCAL void cisTupleShow		(int sock);
LOCAL void cisConfigShow	(int sock);
LOCAL void cisShowDevice	(CIS_TUPLE *pTuple);
LOCAL void cisShowVers1		(CIS_TUPLE *pTuple);
LOCAL void cisShowVers2		(CIS_TUPLE *pTuple);
LOCAL void cisShowFormat	(CIS_TUPLE *pTuple);
LOCAL void cisShowGeometry	(CIS_TUPLE *pTuple);
LOCAL void cisShowJedec		(CIS_TUPLE *pTuple);
LOCAL void cisShowDevgeo	(CIS_TUPLE *pTuple);
LOCAL void cisShowManfid	(CIS_TUPLE *pTuple);
LOCAL void cisShowFuncid	(CIS_TUPLE *pTuple);
LOCAL void cisShowFunce		(CIS_TUPLE *pTuple);
LOCAL void cisShowFunceSerial	(CIS_TUPLE *pTuple);
LOCAL void cisShowFunceModem	(CIS_TUPLE *pTuple);
LOCAL void cisShowFunceDmodem	(CIS_TUPLE *pTuple);
LOCAL void cisShowFunceFmodem	(CIS_TUPLE *pTuple);
LOCAL void cisShowFunceVmodem	(CIS_TUPLE *pTuple);
LOCAL void cisShowConfig	(CIS_TUPLE *pTuple);
LOCAL void cisShowCtable	(CIS_TUPLE *pTuple);
LOCAL u_char *cisShowCpower	(u_char *pChar, u_char featureSelection);
LOCAL u_char *cisShowCtiming	(u_char *pChar, u_char featureSelection);
LOCAL u_char *cisShowCio	(u_char *pChar, u_char featureSelection);
LOCAL u_char *cisShowCirq	(u_char *pChar, u_char featureSelection);
LOCAL u_char *cisShowCmem	(u_char *pChar, u_char featureSelection);
LOCAL u_char *cisShowCmisc	(u_char *pChar, u_char featureSelection);
LOCAL void cisShowCsub		(CIS_TUPLE *pTuple);


/*******************************************************************************
*
* cisShow - show CIS information
*
* This routine shows CIS information.
*
* NOTE: This routine uses floating point calculations.  The calling task needs
* to be spawned with the VX_FP_TASK flag.  If this is not done, the data
* printed by cisShow may be corrupted and unreliable.
*
* RETURNS: N/A
*/

void cisShow
    (
    int sock			/* socket no. */
    )

    {

    cisTupleShow (sock);
    cisConfigShow (sock);
    }

/*******************************************************************************
*
* cisTupleShow - Traverse the tuple link list, and show information in a tuple.
*
* This routine traverses the link list of tuples and shows CIS (Configuration
* Information Structure) in each tuple.
*
* RETURNS: N/A
*/

LOCAL void cisTupleShow
    (
    int sock			/* socket no. */
    )

    {
    PCMCIA_CTRL *pCtrl		= &pcmciaCtrl;
    PCMCIA_CARD *pCard		= &pCtrl->card[sock];
    DL_LIST *pList		= &pCard->cisTupleList;
    DL_NODE *pNode;
    CIS_TUPLE *pTuple;

    for (pNode = DLL_FIRST(pList); pNode != NULL; pNode = DLL_NEXT(pNode))
	{
	pTuple = (CIS_TUPLE *)((char *)pNode + sizeof(DL_NODE));
	switch (pTuple->code)
	    {
	    case CISTPL_DEVICE:
		printf ("Device information tuple for common memory\n");
		cisShowDevice (pTuple);
		break;

	    case CISTPL_DEVICE_A:
		printf ("Device information tuple for attribute memory\n");
		cisShowDevice (pTuple);
		break;
		
	    case CISTPL_VERS_1:
		printf ("Level-1 version/product-information tuple\n");
		cisShowVers1 (pTuple);
		break;
		
	    case CISTPL_GEOMETRY:
		printf ("Geometry tuple\n");
		cisShowGeometry (pTuple);
		break;
		
	    case CISTPL_JEDEC_C:
		printf ("JEDEC tuple for common memory\n");
		cisShowJedec (pTuple);
		break;
		
	    case CISTPL_JEDEC_A:
		printf ("JEDEC tuple for attribute memory\n");
		cisShowJedec (pTuple);
		break;
		
	    case CISTPL_DEVICE_GEO:
		printf ("Device geometry tuple for common memory\n");
		cisShowDevgeo (pTuple);
		break;
		
	    case CISTPL_DEVICE_GEO_A:
		printf ("Device geometry tuple for attribute memory\n");
		cisShowDevgeo (pTuple);
		break;
		
	    case CISTPL_MANFID:
		printf ("Manufacturer identification tuple\n");
		cisShowManfid (pTuple);
		break;
		
	    case CISTPL_FUNCID:
		printf ("Function identification tuple\n");
		cisShowFuncid (pTuple);
		break;
		
	    case CISTPL_FUNCE:
		printf ("Function extension tuple\n");
		cisShowFunce (pTuple);
		break;
		
	    case CISTPL_CONFIG:
		printf ("Configuration tuple\n");
		cisShowConfig (pTuple);
		break;
		
	    case CISTPL_CFTABLE_ENTRY:
		printf ("Configuration table entry tuple\n");
		cisShowCtable (pTuple);
		break;
		
	    case CISTPL_VERS_2:
		printf ("Level-2 version/product-information tuple\n");
		cisShowVers2 (pTuple);
		break;
		
	    case CISTPL_FORMAT:
		printf ("Format tuple\n");
		cisShowFormat (pTuple);
		break;
		
	    case CISTPL_NULL:
	    case CISTPL_LONGLINK_A:
	    case CISTPL_LONGLINK_C:
	    case CISTPL_NO_LINK:
	    case CISTPL_END:
	    case CISTPL_CHECKSUM:
		break;

	    default:
		printf ("Unsupported tuple 0x%x\n", pTuple->code);
    	    }
        }
    }

/*******************************************************************************
*
* cisConfigShow - Show information in a configuration table link list.
*
* This routine traverses the link list of configuration table and shows 
* information in each entry.
*
* RETURNS: N/A
*/

LOCAL void cisConfigShow
    (
    int sock			/* socket no. */
    )

    {
    PCMCIA_CTRL *pCtrl	= &pcmciaCtrl;
    PCMCIA_CARD *pCard	= &pCtrl->card[sock];
    DL_LIST *pList	= &pCard->cisConfigList;
    DL_NODE *pNode;
    CIS_CONFIG *pConfig;
    int ix;

    printf ("socket        = %d\n", sock);
    printf ("configReg base= 0x%-8x mask    = 0x%-8x\n",
	    pCard->regBase, pCard->regMask);

    for (pNode = DLL_FIRST(pList); pNode != NULL; pNode = DLL_NEXT(pNode))
	{
	pConfig = (CIS_CONFIG *)pNode;
	printf ("index         = 0x%-8x\n", pConfig->index);
	printf ("interface     = 0x%-8x\n", pConfig->interfaceType);
	printf ("Vcc  nom      = 0x%-8x min     = 0x%-8x max    = 0x%-8x\n",
		pConfig->vcc[0], pConfig->vcc[1], pConfig->vcc[2]);
	printf ("Vpp1  nom     = 0x%-8x min     = 0x%-8x max    = 0x%-8x\n",
		pConfig->vpp1[0], pConfig->vpp1[1], pConfig->vpp1[2]);
	printf ("Vpp2  nom     = 0x%-8x min     = 0x%-8x max    = 0x%-8x\n",
		pConfig->vpp2[0], pConfig->vpp2[1], pConfig->vpp2[2]);
	printf ("IO Bus width  = 0x%-8x Lines   = 0x%-8x Ranges = 0x%-8x\n",
		pConfig->ioBuswidth, pConfig->ioAddrlines, pConfig->ioRanges);
	for (ix = 0; ix < pConfig->ioRanges; ix++)
	    printf ("  start         = 0x%-8x stop    = 0x%-8x\n",
		    pConfig->io[ix].start, pConfig->io[ix].stop);
	printf ("IRQ Mode      = 0x%-8x Mask    = 0x%-8x\n",
		pConfig->irqMode, pConfig->irqMask);
	printf ("IRQ Level     = 0x%-8x Special = 0x%-8x Bitmap = 0x%4x\n",
		pConfig->irqLevel, pConfig->irqSpecial, pConfig->irqBit.s);
	for (ix = 0; ix < 8; ix++)
	    {
	    if ((pConfig->mem[ix].length == 0) &&
		(pConfig->mem[ix].cAddr == 0) && 
	    	(pConfig->mem[ix].hAddr == 0))
		continue;
	    printf ("  length        = 0x%-8x cAddr   = 0x%-8x hAddr  = 0x%-8x\n",
		    pConfig->mem[ix].length, pConfig->mem[ix].cAddr, 
		    pConfig->mem[ix].hAddr);
	    }
	printf ("twins         = 0x%-8x audio   = 0x%-8x\n",
		pConfig->twins, pConfig->audio);
	printf ("readonly      = 0x%-8x pwrdown = 0x%-8x\n\n",
		pConfig->readonly, pConfig->pwrdown);
	}
    }

/*******************************************************************************
*
* cisShowDevice - Show information in device information tuple.
*
* This routine shows CIS in the device information tuple.  Uses floating
* point calculations.  The calling task needs to have VX_FP_TASK bit set
* in its task flags.
*
* RETURNS: N/A
*/

LOCAL void cisShowDevice
    (
    CIS_TUPLE *pTuple		/* pointer to a tuple */
    )

    {
    u_char *pChar	= (u_char *)pTuple + sizeof(CIS_TUPLE);
    u_char *pEnd	= pChar + pTuple->link;
    BOOL speedExtended	= FALSE;
    BOOL typeExtended	= FALSE;
    u_char unit;
    u_int size;
    double speed;


    while ((pChar < pEnd) && (*pChar != 0xff))
	{
	deviceInfos++;

	switch (*pChar & 0xf0)
	    {
	    case DTYPE_NULL:
		printf ("  Not a memory device\n");
		break;

	    case DTYPE_ROM:
		printf ("  Masked ROM\n");
		break;

	    case DTYPE_OTPROM:
		printf ("  One-time programmable ROM\n");
		break;

	    case DTYPE_EPROM:
		printf ("  UV EPROM\n");
		break;

	    case DTYPE_EEPROM:
		printf ("  EEPROM\n");
		break;

	    case DTYPE_FLASH:
		printf ("  Flash EPROM\n");
		break;

	    case DTYPE_SRAM:
		printf ("  SRAM\n");
		break;

	    case DTYPE_DRAM:
		printf ("  DRAM\n");
		break;

	    case DTYPE_FUNCSPEC:
		printf ("  Function-specific memory address range\n");
		break;

	    case DTYPE_EXTEND:
		typeExtended = TRUE;
		break;

	    default:
		printf ("  Reserved device type\n");
	    }

	if (*pChar & 0x08)
	    printf ("  Always writable\n");
	else
	    printf ("  WPS and WP signal indicates it\n");
	
	switch (*pChar & 0x07)
	    {
	    case DSPEED_NULL:
		break;

	    case DSPEED_250NS:
		printf ("  250ns\n");
		break;

	    case DSPEED_200NS:
		printf ("  200ns\n");
		break;

	    case DSPEED_150NS:
		printf ("  150ns\n");
		break;

	    case DSPEED_100NS:
		printf ("  100ns\n");
		break;

	    case DSPEED_EXT:
		speedExtended = TRUE;
		break;

	    default:
		printf ("  Reserved device speed\n");
	    }

	pChar++;
	while (speedExtended)
	    {
	    if ((*pChar & 0x80) == 0x00)
		speedExtended = FALSE;
	    
	    speed = speedMantisa[(*pChar & 0x78) >> 3] * 
		    speedExponent[(*pChar & 0x07)];
	    printf ("  Speed=%1.9f\n", speed);
	    pChar++;
	    }

	while (typeExtended)
	    {
	    if ((*pChar & 0x80) == 0x00)
		typeExtended = FALSE;
	    pChar++;
	    }
	    
	unit = ((*pChar & 0xf8) >> 3) + 1;
	size = deviceSize[*pChar & 0x07];
	printf ("  Device size = %d * 0x%x = 0x%x\n", unit, size, unit * size);
	pChar++;
	}
    }

/*******************************************************************************
*
* cisShowVers1 - Show information in level-1 version/product information tuple.
*
* This routine shows CIS in the level-1 version/product information tuple.
*
* RETURNS: N/A
*/

LOCAL void cisShowVers1
    (
    CIS_TUPLE *pTuple		/* pointer to a tuple */
    )

    {
    u_char *pChar	= (u_char *)pTuple + sizeof(CIS_TUPLE);
    u_char *pEnd	= pChar + pTuple->link;

    printf ("  Major version number = %d\n", *pChar++);
    printf ("  Minor version number = %d\n  ", *pChar++);

    while ((pChar < pEnd) && (*pChar != 0xff))
	pChar += printf ("%s", pChar) + 1;
    printf ("\n");
    }

/*******************************************************************************
*
* cisShowGeometry - Show information in geometry tuple.
*
* This routine shows CIS in the geometry tuple.
*
* RETURNS: N/A
*/

LOCAL void cisShowGeometry
    (
    CIS_TUPLE *pTuple		/* pointer to a tuple */
    )

    {
    u_char *pChar	= (u_char *)pTuple + sizeof(CIS_TUPLE);
    CIS_BYTE2 cylinder;

    printf ("  Sectors/track = %d\n", *pChar++);
    printf ("  Tracks/cylinder = %d\n", *pChar++);
    cylinder.c[0] = *pChar++;
    cylinder.c[1] = *pChar++;
    printf ("  Cylinder = %d\n", cylinder.s);
    }

/*******************************************************************************
*
* cisShowJedec - Show information in JEDEC programming information tuple.
*
* This routine shows CIS in the JEDEC programming information tuple.
*
* RETURNS: N/A
*/

LOCAL void cisShowJedec
    (
    CIS_TUPLE *pTuple		/* pointer to a tuple */
    )

    {
    u_char *pChar	= (u_char *)pTuple + sizeof(CIS_TUPLE);
    u_char *pEnd	= pChar + pTuple->link;

    while ((pChar < pEnd) && (deviceInfos-- != 0))
	{
	printf ("  Manufacturer ID = 0x%x\n", *pChar++);
	printf ("  Manufacturer's info = 0x%x\n", *pChar++);
	}
    }

/*******************************************************************************
*
* cisShowDevgeo - Show information in Device geometry tuple.
*
* This routine shows CIS in the Device geometry tuple.
*
* RETURNS: N/A
*/

LOCAL void cisShowDevgeo
    (
    CIS_TUPLE *pTuple		/* pointer to a tuple */
    )

    {
    u_char *pChar	= (u_char *)pTuple + sizeof(CIS_TUPLE);
    u_char *pEnd	= pChar + pTuple->link;

    while ((pChar < pEnd) && (*pChar != 0xff))
	{
	printf ("  Internal bus width of card = %d bytes\n",
		1 << (*pChar++ - 1)); 
	printf ("  Erase geometry block size = %d\n", 1 << (*pChar++ - 1)); 
	printf ("  Read geometry block size = %d\n", 1 << (*pChar++ - 1)); 
	printf ("  Write geometry block size = %d\n", 1 << (*pChar++ - 1)); 
	printf ("  Partition size = %d\n", 1 << (*pChar++ - 1)); 
	printf ("  Interleave size = %d\n", 1 << (*pChar++ - 1)); 
	}

    }

/*******************************************************************************
*
* cisShowManfid - Show information in Manufacturer ID tuple.
*
* This routine shows CIS in the Manufacturer ID tuple.
*
* RETURNS: N/A
*/

LOCAL void cisShowManfid
    (
    CIS_TUPLE *pTuple		/* pointer to a tuple */
    )

    {
    u_char *pChar	= (u_char *)pTuple + sizeof(CIS_TUPLE);
    CIS_BYTE2 manufacturerCode;
    CIS_BYTE2 manufacturerInfo;

    manufacturerCode.c[0] = *pChar++;
    manufacturerCode.c[1] = *pChar++;
    manufacturerInfo.c[0] = *pChar++;
    manufacturerInfo.c[1] = *pChar++;
    printf ("  Manufacturer code = 0x%x\n", manufacturerCode.s);
    printf ("  Manufacturer info = 0x%x\n", manufacturerInfo.s);
    }

/*******************************************************************************
*
* cisShowFuncid - Show information in Function ID tuple.
*
* This routine shows CIS in the Function ID tuple.
*
* RETURNS: N/A
*/

LOCAL void cisShowFuncid
    (
    CIS_TUPLE *pTuple		/* pointer to a tuple */
    )

    {
    u_char *pChar	= (u_char *)pTuple + sizeof(CIS_TUPLE);

    switch (*pChar++)
	{
	case FUNC_MULTI:
            cisFuncid = FUNC_MULTI;
	    printf ("  Multi function\n");
	    break;

	case FUNC_MEMORY:
            cisFuncid = FUNC_MEMORY;
	    printf ("  Memory\n");
	    break;

	case FUNC_SERIAL:
            cisFuncid = FUNC_SERIAL;
	    printf ("  Serial port\n");
	    break;

	case FUNC_PARALLEL:
            cisFuncid = FUNC_PARALLEL;
	    printf ("  Parallel port\n");
	    break;

	case FUNC_FIXEDDISK:
            cisFuncid = FUNC_FIXEDDISK;
	    printf ("  Fixed disk\n");
	    break;

	case FUNC_VIDEO:
            cisFuncid = FUNC_VIDEO;
	    printf ("  Video adaptor\n");
	    break;

	case FUNC_LAN:
            cisFuncid = FUNC_LAN;
	    printf ("  Network LAN adaptor\n");
	    break;

	case FUNC_AIMS:
            cisFuncid = FUNC_AIMS;
	    printf ("  AIMS\n");
	    break;

	default:
	    printf ("  Reserved\n");
	    break;
	}
    
    systemInitByte |= *pChar;
    if (systemInitByte & 0x01)
	printf ("  System initialization is POST\n");
    else
	printf ("  System initialization is ROM\n");
    }

/*******************************************************************************
*
* cisShowFunce - Show information in Function extension tuple.
*
* This routine shows CIS in the Function extension tuple.
*
* RETURNS: N/A
*/

LOCAL void cisShowFunce
    (
    CIS_TUPLE *pTuple		/* pointer to a tuple */
    )

    {
    u_char *pChar	= (u_char *)pTuple + sizeof(CIS_TUPLE);

    if (cisFuncid == FUNC_SERIAL)
	{
	switch (*pChar & 0x0f)
	    {
	    case 0x00:
	    case 0x08:
	    case 0x09:
	    case 0x0a:
		printf ("  Serial port interface\n");
		cisShowFunceSerial (pTuple);
		break;

	    case 0x01:
	    case 0x05:
	    case 0x06:
	    case 0x07:
		printf ("  Modem interface\n");
		cisShowFunceModem (pTuple);
		break;

	    case 0x02:
		printf ("  Data modem interface\n");
		cisShowFunceDmodem (pTuple);
		break;

	    case 0x03:
		printf ("  Facsimile modem interface\n");
		cisShowFunceFmodem (pTuple);
		break;

	    case 0x04:
		printf ("  Voice modem interface\n");
		cisShowFunceVmodem (pTuple);
		break;
	    
	    default:
		printf ("  Reserved\n");
	    }
	}

    if (cisFuncid == FUNC_FIXEDDISK)
	{
	switch (*pChar++)
	    {
	    case 0x01:
		if (*pChar == 0x01)
		    printf ("  ATA interface\n");
		else
		    printf ("  Non ATA interface\n");
		break;

	    case 0x02:
		if ((*pChar & 0x03) == 0x00)
		    printf ("  Vpp power not required\n");
		else if ((*pChar & 0x03) == 0x01)
		    printf ("  Vpp power required for media modification access\n");
		else if ((*pChar & 0x03) == 0x02)
		    printf ("  Vpp power required for all media access\n");
		else if ((*pChar & 0x03) == 0x03)
		    printf ("  Vpp power required continuously\n");
		if (*pChar & 0x04)
		    printf ("  Silicon device\n");
		else
		    printf ("  Rotating device\n");
		if (*pChar & 0x08)
		    printf ("  Serial number is guaranteed unique\n");
		else
		    printf ("  Serial number may not be unique\n");
		
		pChar++;
		if (*pChar & 0x01)
		    printf ("  Sleep mode supported\n");
		else
		    printf ("  Sleep mode not supported\n");
		if (*pChar & 0x02)
		    printf ("  Standby mode supported\n");
		else
		    printf ("  Standby mode not supported\n");
		if (*pChar & 0x04)
		    printf ("  Idle mode supported\n");
		else
		    printf ("  Idle mode not supported\n");
		if (*pChar & 0x08)
		    printf ("  Drive automatically minimize power\n");
		else
		    printf ("  Low power mode use required to minimize power\n");
		if (*pChar & 0x10)
		    printf ("  Some primary or secondly IO addressing modes exclude 3F7 and/or 377 for floppy interface\n");
		else
		    printf ("  All primary and secondly IO addressing modes include ports 3F7/377\n");
		if (*pChar & 0x20)
		    printf ("  Index bit is supported or emulated\n");
		else
		    printf ("  Index bit is not supported or emulated\n");
		if (*pChar & 0x40)
		    printf ("  -IOis16 is asserted only for data register on twin-card configuration\n");
		else
		    printf ("  -IOis16 use is unspecified on twin-card configuration\n");
	    }
	}
    }

/*******************************************************************************
*
* cisShowFunceSerial - Show information in Serial port Function extension tuple.
*
* This routine shows CIS in the Serial port Function extension tuple.
*
* RETURNS: N/A
*/

LOCAL void cisShowFunceSerial
    (
    CIS_TUPLE *pTuple		/* pointer to a tuple */
    )

    {
    u_char *pChar	= (u_char *)pTuple + sizeof(CIS_TUPLE);

    pChar++;	/* skip the type of extended data */
    switch (*pChar++)
	{
	case 0x0:
	    printf ("  8250 UART is present\n");
	    break;

	case 0x01:
	    printf ("  16450 UART is present\n");
	    break;
	
	case 0x02:
	    printf ("  16550 UART is present\n");
	    break;
	
	default:
	    printf ("  Reserved\n");
	}
    
    if (*pChar & 0x01)
	printf ("  Space parity supported\n");
    else
	printf ("  Space parity not supported\n");
    if (*pChar & 0x02)
	printf ("  Mark parity supported\n");
    else
	printf ("  Mark parity not supported\n");
    if (*pChar & 0x04)
	printf ("  Odd parity supported\n");
    else
	printf ("  Odd parity not supported\n");
    if (*pChar & 0x08)
	printf ("  Even parity supported\n");
    else
	printf ("  Even parity not supported\n");
    
    pChar++;
    if (*pChar & 0x01)
	printf ("  5 bit data format is supported\n");
    else
	printf ("  5 bit data format is not supported\n");
    if (*pChar & 0x02)
	printf ("  6 bit data format is supported\n");
    else
	printf ("  6 bit data format is not supported\n");
    if (*pChar & 0x04)
	printf ("  7 bit data format is supported\n");
    else
	printf ("  7 bit data format is not supported\n");
    if (*pChar & 0x08)
	printf ("  8 bit data format is supported\n");
    else
	printf ("  8 bit data format is not supported\n");
    if (*pChar & 0x10)
	printf ("  1 stop bit is supported\n");
    else
	printf ("  1 stop bit is not supported\n");
    if (*pChar & 0x20)
	printf ("  1.5 stop bit is supported\n");
    else
	printf ("  1.5 stop bit is not supported\n");
    if (*pChar & 0x40)
	printf ("  2 stop bit is supported\n");
    else
	printf ("  2 stop bit is not supported\n");
    } 

/*******************************************************************************
*
* cisShowFunceModem - Show information in Modem Function extension tuple.
*
* This routine shows CIS in the Modem Function extension tuple.
*
* RETURNS: N/A
*/

LOCAL void cisShowFunceModem
    (
    CIS_TUPLE *pTuple		/* pointer to a tuple */
    )

    {
    u_char *pChar	= (u_char *)pTuple + sizeof(CIS_TUPLE);
    CIS_BYTE4 rxBuf;
    CIS_BYTE4 txBuf;

    pChar++;
    if (*pChar & 0x01)
	printf ("  Tx xon/xoff is supported\n");
    else
	printf ("  Tx xon/xoff is not supported\n");
    if (*pChar & 0x02)
	printf ("  Rx xon/xoff is supported\n");
    else
	printf ("  Rx xon/xoff is not supported\n");
    if (*pChar & 0x04)
	printf ("  Tx CTS is supported\n");
    else
	printf ("  Tx CTS is not supported\n");
    if (*pChar & 0x08)
	printf ("  Rx RTS is supported\n");
    else
	printf ("  Rx RTS is not supported\n");
    if (*pChar & 0x10)
	printf ("  Transparent flow control is supported\n");
    else
	printf ("  Transparent flow control is not supported\n");
    
    pChar++;
    printf ("  Size of command buffer = 0x%x\n", (*pChar++ + 1) * 4);

    rxBuf.c[0] = *pChar++;
    rxBuf.c[1] = *pChar++;
    rxBuf.c[2] = *pChar++;
    rxBuf.c[3] = 0x00;
    printf ("  Size of rx buffer = 0x%x\n", rxBuf.l);

    txBuf.c[0] = *pChar++;
    txBuf.c[1] = *pChar++;
    txBuf.c[2] = *pChar++;
    txBuf.c[3] = 0x00;
    printf ("  Size of tx buffer = 0x%x\n", txBuf.l);
    }

/*******************************************************************************
*
* cisShowFunceDmodem - Show information in Data Modem Function extension tuple.
*
* This routine shows CIS in the Data Modem Function extension tuple.
*
* RETURNS: N/A
*/

LOCAL void cisShowFunceDmodem
    (
    CIS_TUPLE *pTuple		/* pointer to a tuple */
    )

    {
    u_char *pChar	= (u_char *)pTuple + sizeof(CIS_TUPLE);
    u_char *pEnd	= pChar + pTuple->link;
    CIS_BYTE2 maxdataRate;

    pChar++;
    maxdataRate.c[1] = *pChar++;
    maxdataRate.c[0] = *pChar++;
    printf ("  DTE to UART max data rate = %d\n", maxdataRate.s);

    printf ("  Modulation standart: ");
    if (*pChar & 0x01)
	printf ("Bell-103 ");
    if (*pChar & 0x02)
	printf ("V.21 ");
    if (*pChar & 0x04)
	printf ("V.23 ");
    if (*pChar & 0x08)
	printf ("V.22-A&B ");
    if (*pChar & 0x10)
	printf ("Bell-212A ");
    if (*pChar & 0x20)
	printf ("V.22-bis ");
    if (*pChar & 0x40)
	printf ("V.26 ");
    if (*pChar & 0x80)
	printf ("V.26-bis ");
    pChar++;
    if (*pChar & 0x01)
	printf ("V.27-bis ");
    if (*pChar & 0x02)
	printf ("V.29 ");
    if (*pChar & 0x04)
	printf ("V.32 ");
    if (*pChar & 0x08)
	printf ("V.32-bis ");
    if (*pChar & 0x10)
	printf ("VFAST ");
    printf ("\n");

    pChar++;
    printf ("  Error correction/detection protocol: ");
    if (*pChar & 0x01)
	printf ("MNP2-4 ");
    if (*pChar & 0x02)
	printf ("V.42/LAPM ");
    printf ("\n");
    
    pChar++;
    printf ("  Data compression protocol: ");
    if (*pChar & 0x01)
	printf ("V.42-bis ");
    if (*pChar & 0x02)
	printf ("MNP5 ");
    printf ("\n");
	    
    pChar++;
    printf ("  Command protocols: ");
    if (*pChar & 0x01)
	printf ("AT1 ");
    if (*pChar & 0x02)
	printf ("AT2 ");
    if (*pChar & 0x04)
	printf ("AT3 ");
    if (*pChar & 0x08)
	printf ("MNP-AT ");
    if (*pChar & 0x10)
	printf ("V.25-bis ");
    if (*pChar & 0x20)
	printf ("V.25-A ");
    if (*pChar & 0x40)
	printf ("DMCL ");
    printf ("\n");

    pChar++;
    printf ("  Escape mechanisms: ");
    if (*pChar & 0x01)
	printf ("BREAK ");
    if (*pChar & 0x02)
	printf ("+++ ");
    if (*pChar & 0x04)
	printf ("user defined ");
    printf ("\n");

    pChar++;
    printf ("  Data encryption: reserved\n");

    pChar++;
    printf ("  End user feature selection: ");
    if (*pChar & 0x01)
	printf ("caller ID decoding\n");
    else
	printf ("caller ID not decoding\n");

    pChar++;
    printf ("  Country code: ");
    while ((pChar < pEnd) && (*pChar != 0xff))
	printf ("0x%x ", *pChar++);
    printf ("\n");
    }

/*******************************************************************************
*
* cisShowFunceFmodem - Show information in Fax Modem Function extension tuple.
*
* This routine shows CIS in the Fax Modem Function extension tuple.
*
* RETURNS: N/A
*/

LOCAL void cisShowFunceFmodem
    (
    CIS_TUPLE *pTuple		/* pointer to a tuple */
    )

    {
    u_char *pChar	= (u_char *)pTuple + sizeof(CIS_TUPLE);
    u_char *pEnd	= pChar + pTuple->link;
    CIS_BYTE2 maxdataRate;

    if ((*pChar & 0xf0) == 0x10)
	printf ("  EIA/TIA-578 Service Class 1\n");
    if ((*pChar & 0xf0) == 0x20)
	printf ("  EIA/TIA-578 Service Class 2\n");
    if ((*pChar & 0xf0) == 0x30)
	printf ("  EIA/TIA-578 Service Class 3\n");

    pChar++;
    maxdataRate.c[1] = *pChar++;
    maxdataRate.c[0] = *pChar++;
    printf ("  DTE to UART max data rate = %d\n", maxdataRate.s * 75);

    printf ("  Moduration standard: ");
    if (*pChar & 0x01)
	printf ("V.21-C2 ");
    if (*pChar & 0x02)
	printf ("V.27ter ");
    if (*pChar & 0x04)
	printf ("V.29 ");
    if (*pChar & 0x08)
	printf ("V.17 ");
    if (*pChar & 0x10)
	printf ("V.33 ");
    printf ("\n");
    
    pChar++;
    printf ("  Document Facsimile Feature Selection: ");
    if (*pChar & 0x01)
	printf ("T.3 ");
    if (*pChar & 0x02)
	printf ("T.4 ");
    if (*pChar & 0x04)
	printf ("T.6 ");
    if (*pChar & 0x08)
	printf ("error-correction-mode ");
    if (*pChar & 0x10)
	printf ("voice-request ");
    if (*pChar & 0x20)
	printf ("polling ");
    if (*pChar & 0x40)
	printf ("file-transfer ");
    if (*pChar & 0x80)
	printf ("password ");
    printf ("\n");

    pChar += 2;
    printf ("  Country code: ");
    while ((pChar < pEnd) && (*pChar != 0xff))
	printf ("0x%x ", *pChar++);
    printf ("\n");
    }

/*******************************************************************************
*
* cisShowFunceVmodem - Show information in Voice Modem Function extension tuple.
*
* This routine shows CIS in the Voice Modem Function extension tuple.
*
* RETURNS: N/A
*/

LOCAL void cisShowFunceVmodem
    (
    CIS_TUPLE *pTuple		/* pointer to a tuple */
    )

    {
    u_char *pChar	= (u_char *)pTuple + sizeof(CIS_TUPLE);
    CIS_BYTE2 maxdataRate;

    printf ("  Service class %d\n", (*pChar & 0xf0) >> 14);

    pChar++;
    maxdataRate.c[1] = *pChar++;
    maxdataRate.c[0] = *pChar++;
    printf ("  DTE to UART max data rate = %d\n", maxdataRate.s * 75);

    while (*pChar != 0x00)
	{
	printf ("  Sample rate = %d\n", *pChar * 1000 + *(pChar+1) * 10);
	pChar += 2;
	}

    pChar++;
    while (*pChar != 0x00)
	{
	printf ("  Sample size = %d.%d\n", *pChar, *(pChar+1));
	pChar += 2;
	}

    printf ("  Voice compression methods: reserved\n");
    }

/*******************************************************************************
*
* cisShowConfig - Show information in Configuration tuple.
*
* This routine shows CIS in the Configuration tuple.
*
* RETURNS: N/A
*/

LOCAL void cisShowConfig
    (
    CIS_TUPLE *pTuple		/* pointer to a tuple */
    )

    {
    u_char *pChar	= (u_char *)pTuple + sizeof(CIS_TUPLE);
    u_char *pEnd	= pChar + pTuple->link;
    u_char mask[16];
    u_char cisCentries;
    u_int addrSize;
    u_int maskSize;
    u_int ix;
    u_int size;
    CIS_BYTE4 base;
    CIS_BYTE4 interface;
    CIS_TUPLE subtuple;

    addrSize = (*pChar & 0x03) + 1;
    maskSize = ((*pChar & 0x3c) >> 2) + 1;
    printf ("  Sizeof configuration register base address = %d\n", addrSize);
    printf ("  Sizeof configuration register presence mask = %d\n", maskSize);

    pChar++;
    if (pChar > pEnd)
	return;
    
    cisCentries = *pChar++;
    printf ("  Last index = %d\n", cisCentries);

    base.l = 0;
    for (ix = 0; ix < addrSize; ix++)
	base.c[ix] = *pChar++;
    
    printf ("  Base address = 0x%x\n", base.l);
    printf ("  Mask = ");
    for (ix = 0; ix < maskSize; ix++)
	{
	mask[ix] = *pChar++;
	printf ("0x%x ", mask[ix]);
	}
    printf ("\n");

    pChar++;
    while ((pChar < pEnd) && (*pChar != 0xff))
	{
	subtuple.code = *pChar++;
	subtuple.link = *pChar++;
	pEnd = pChar + subtuple.link;

	size = ((*pChar & 0xc0) >> 6) + 1;
	interface.l = 0;
	for (ix = 0; ix < size; ix++)
	    interface.c[ix] = *pChar++;

	pChar++;
        printf ("  ");
	while ((pChar < pEnd) && (*pChar != 0xff))
	    pChar += printf ("%s", pChar) + 1;
	printf ("\n");
	}
    }

/*******************************************************************************
*
* cisShowCtable - Show information in Configuration tuple.
*
* This routine shows CIS in the Configuration tuple.
*
* RETURNS: N/A
*/

LOCAL void cisShowCtable
    (
    CIS_TUPLE *pTuple		/* pointer to a tuple */
    )

    {
    u_char *pChar	= (u_char *)pTuple + sizeof(CIS_TUPLE);
    u_char featureSelection;
    u_char index;
    
    index = *pChar & 0x3f;
    printf ("  Index = %d\n", index);
    if (*pChar & 0x40)
	cisCdefault = pTuple;
    if (*pChar++ & 0x80)
	{
	printf ("  Interface type is %s\n", interfaceType[*pChar & 0x07]);

	if (*pChar & 0x10)
	    printf ("  BVDs Active\n");
	else
	    printf ("  BVDs Inactive\n");
	if (*pChar & 0x20)
	    printf ("  WP Active\n");
	else
	    printf ("  WP Inactive\n");
	if (*pChar & 0x40)
	    printf ("  RdyBsy Active\n");
	else
	    printf ("  RdyBsy Inactive\n");
	if (*pChar & 0x80)
	    printf ("  M Wait Required\n");
	else
	    printf ("  M Wait Not required\n");
	pChar++;
	}

    featureSelection = *pChar++;
    if (featureSelection & 0x03)
	{
	printf ("  Power description\n");
	pChar = cisShowCpower (pChar, featureSelection);
	}
    if (featureSelection & 0x04)
	{
	printf ("  Timing description\n");
	pChar = cisShowCtiming (pChar, featureSelection);
	}
    if (featureSelection & 0x08)
	{
	printf ("  IOspace description\n");
	pChar = cisShowCio (pChar, featureSelection);
	}
    if (featureSelection & 0x10)
	{
	printf ("  IRQ description\n");
	pChar = cisShowCirq (pChar, featureSelection);
	}
    if (featureSelection & 0x60)
	{
	printf ("  MEMspace description\n");
	pChar = cisShowCmem (pChar, featureSelection);
	}
    if (featureSelection & 0x80)
	{
	printf ("  Misc description\n");
	pChar = cisShowCmisc (pChar, featureSelection);
	}
    
    cisShowCsub ((CIS_TUPLE *)pChar);
    }

/*******************************************************************************
*
* cisShowCpower - Show information in Power description structure.
*
* This routine shows CIS in the Power description structure.
*
* RETURNS: A pointer to next entry.
*/

LOCAL u_char *cisShowCpower
    (
    u_char *pChar,		/* pointer to power description */
    u_char featureSelection	/* feature selection byte */
    )

    {
    u_char parameterSelection;
    char *pV = "";
    u_int ix;

    featureSelection &= 0x03;
    for (ix = 1; ix <= featureSelection; ix++)
	{
	if (ix == 1)
	    pV = "Vcc";
	if ((ix == 2) && (featureSelection == 2))
	    pV = "Vpp";
	if ((ix == 2) && (featureSelection == 3))
	    pV = "Vpp1";
	if (ix == 3)
	    pV = "Vpp2";
	
	parameterSelection = *pChar++;
	if (parameterSelection & 0x01)
	    do  {
		printf ("    %s nominal V = 0x%x\n", pV, *pChar++ & 0x7f);
		} while (*(pChar-1) & 0x80);
	if (parameterSelection & 0x02)
	    do  {
		printf ("    %s minimum V = 0x%x\n", pV, *pChar++ & 0x7f);
		} while (*(pChar-1) & 0x80);
	if (parameterSelection & 0x04)
	    do  {
		printf ("    %s maximum V = 0x%x\n", pV, *pChar++ & 0x7f);
		} while (*(pChar-1) & 0x80);
	if (parameterSelection & 0x08)
	    do  {
		printf ("    %s static I = 0x%x\n", pV, *pChar++ & 0x7f);
		} while (*(pChar-1) & 0x80);
	if (parameterSelection & 0x10)
	    do  {
		printf ("    %s average I = 0x%x\n", pV, *pChar++ & 0x7f);
		} while (*(pChar-1) & 0x80);
	if (parameterSelection & 0x20)
	    do  {
		printf ("    %s no peak I = 0x%x\n", pV, *pChar++ & 0x7f);
		} while (*(pChar-1) & 0x80);
	if (parameterSelection & 0x40)
	    do  {
		printf ("    %s power down I = 0x%x\n", pV, *pChar++ & 0x7f);
		} while (*(pChar-1) & 0x80);
	}

    return (pChar);
    }

/*******************************************************************************
*
* cisShowCtiming - Show information in Timing description structure.
*
* This routine shows CIS in the Timing description structure.
*
* RETURNS: A pointer to next entry.
* ARGSUSED
*/

LOCAL u_char *cisShowCtiming
    (
    u_char *pChar,		/* pointer to timing description */
    u_char featureSelection	/* feature selection byte */
    )

    {
    u_int waitScale	= *pChar & 0x03;
    u_int busyScale	= (*pChar & 0x1c) >> 2;
    u_int reservedScale	= (*pChar & 0xe0) >> 5;
    BOOL speedExtended;
    double waitTime;
    double busyTime;
    double reservedTime;

    pChar++;
    if (waitScale != 0x03)
	{
	speedExtended = TRUE;
	while (speedExtended)
	    {
	    if ((*pChar & 0x80) == 0x00)
		speedExtended = FALSE;
	    waitTime = speedMantisa [(*pChar & 0x78) >> 3] *
		       speedExponent [(*pChar & 0x07)] * waitScale;
	    printf ("    Wait time = %1.9f\n", waitTime);
	    pChar++;
	    }
	}
    else
	printf ("    Wait signal is not used\n");

    if (busyScale != 0x07)
	{
	speedExtended = TRUE;
	while (speedExtended)
	    {
	    if ((*pChar & 0x80) == 0x00)
		speedExtended = FALSE;
	    busyTime = speedMantisa [(*pChar & 0x78) >> 3] *
		       speedExponent [(*pChar & 0x07)] * busyScale;
	    printf ("    Busy time = %1.9f\n", busyTime);
	    pChar++;
	    }
	}
    else
	printf ("    Ready/Busy signal is not used\n");

    if (reservedScale != 0x07)
	{
	speedExtended = TRUE;
	while (speedExtended)
	    {
	    if ((*pChar & 0x80) == 0x00)
		speedExtended = FALSE;
	    reservedTime = speedMantisa [(*pChar & 0x78) >> 3] *
			   speedExponent [(*pChar & 0x07)] * reservedScale;
	    printf ("    Reserved time = %1.9f\n", reservedTime);
	    pChar++;
	    }
	}
    else
	printf ("    No reserved-time is defined\n");
    
    return (pChar);
    }

/*******************************************************************************
*
* cisShowCio - Show information in IO description structure.
*
* This routine shows CIS in the IO description structure.
*
* RETURNS: A pointer to next entry.
*/

LOCAL u_char *cisShowCio
    (
    u_char *pChar,		/* pointer to IO description */
    u_char featureSelection	/* feature selection byte */
    )

    {
    CIS_BYTE4 addr;
    CIS_BYTE4 length;
    u_int ioRanges;
    u_int addrSize;
    u_int lengthSize;

    printf ("    IO address lines = %d\n", *pChar & 0x1f);
    printf ("    Bus width = ");
    if (*pChar & 0x20)
	printf ("8 ");
    if (*pChar & 0x40)
	printf ("16 ");
    printf ("\n");

    if (*pChar++ & 0x80)
	{
	ioRanges = (*pChar & 0x07) + 1;
	addrSize = (*pChar & 0x30) >> 4;
	lengthSize = (*pChar & 0xc0) >> 6;

	pChar++;
	while (ioRanges--)
	    {
	    addr.l = 0;
	    length.l = 0;

	    if (addrSize == 1)
		addr.c[0] = *pChar++;
	    if (addrSize == 2)
		{
		addr.c[0] = *pChar++;
		addr.c[1] = *pChar++;
		}
	    if (addrSize == 3)
		{
		addr.c[0] = *pChar++;
		addr.c[1] = *pChar++;
		addr.c[2] = *pChar++;
		addr.c[3] = *pChar++;
		}

	    if (lengthSize == 1)
		length.c[0] = *pChar++;
	    if (lengthSize == 2)
		{
		length.c[0] = *pChar++;
		length.c[1] = *pChar++;
		}
	    if (lengthSize == 3)
		{
		length.c[0] = *pChar++;
		length.c[1] = *pChar++;
		length.c[2] = *pChar++;
		length.c[3] = *pChar++;
		}
	    printf ("    Addr = 0x%x length = 0x%x\n", addr.l, length.l + 1);
	    }
	}
    
    return (pChar);
    }

/*******************************************************************************
*
* cisShowCirq - Show information in IRQ description structure.
*
* This routine shows CIS in the IRQ description structure.
*
* RETURNS: A pointer to next entry.
*/

LOCAL u_char *cisShowCirq
    (
    u_char *pChar,		/* pointer to IRQ description */
    u_char featureSelection	/* feature selection byte */
    )

    {
    u_int ix;

    printf ("    Interrupt request mode is ");
    if (*pChar & 0x80)
	printf ("Share ");
    if (*pChar & 0x40)
	printf ("Pulse ");
    if (*pChar & 0x20)
	printf ("Level ");
    printf ("\n");

    printf ("    Interrupt request line is ");
    if (*pChar & 0x10)
	{
	if (*pChar & 0x08)
	    printf ("VEND ");
	if (*pChar & 0x04)
	    printf ("BERR ");
	if (*pChar & 0x02)
	    printf ("IOCK ");
	if (*pChar & 0x01)
	    printf ("NMI ");
    
        for (ix = 0; ix < 8; ix++)
	    if (*(pChar+1) & (1 << ix))
	        printf ("%d ", ix);
        for (ix = 0; ix < 8; ix++)
	    if (*(pChar+2) & (1 << ix))
	        printf ("%d ", ix + 8);
    
        pChar += 3;
        }
    else
	{
	printf ("%d", *pChar & 0x0f);
	pChar++;
	}
    printf ("\n");

    return (pChar);
    }

/*******************************************************************************
*
* cisShowCmem - Show information in Memory description structure.
*
* This routine shows CIS in the Memory description structure.
*
* RETURNS: A pointer to next entry.
*/

LOCAL u_char *cisShowCmem
    (
    u_char *pChar,		/* pointer to memory description */
    u_char featureSelection	/* feature selection byte */
    )

    {
    u_char addrSize;
    u_char lengthSize;
    u_char windows;
    u_char hostaddr;
    CIS_BYTE4 cAddr;
    CIS_BYTE4 hAddr;
    CIS_BYTE4 length;

    if ((featureSelection & 0x60) == 0x20)
	{
	length.l = 0;
	length.c[0] = *pChar++;
	length.c[1] = *pChar++;
	printf ("    Length = 0x%x  cardAddress = 0\n", length.l * 256);
	}
    else if ((featureSelection & 0x60) == 0x40)
	{
	length.l = 0;
	cAddr.l = 0;
	length.c[0] = *pChar++;
	length.c[1] = *pChar++;
	cAddr.c[0] = *pChar++;
	cAddr.c[1] = *pChar++;
	printf ("    Length = 0x%x cardAddress = 0x%x\n",
		length.l * 256, cAddr.l * 256);
	}
    else if ((featureSelection & 0x60) == 0x60)
	{
	windows = (*pChar & 0x07) + 1;
	lengthSize = (*pChar & 0x18) >> 3;
	addrSize = (*pChar & 0x60) >> 5;
	hostaddr = *pChar & 0x80;
	while (windows--)
	    {
	    length.l = 0;
	    cAddr.l = 0;
	    hAddr.l = 0;
	    if (lengthSize == 1)
		length.c[0] = *pChar++;
	    if (lengthSize == 2)
		{
		length.c[0] = *pChar++;
		length.c[1] = *pChar++;
		}
	    if (lengthSize == 3)
		{
		length.c[0] = *pChar++;
		length.c[1] = *pChar++;
		length.c[2] = *pChar++;
		length.c[3] = *pChar++;
		}
	    if (addrSize == 1)
		cAddr.c[0] = *pChar++;
	    if (addrSize == 2)
		{
		cAddr.c[0] = *pChar++;
		cAddr.c[1] = *pChar++;
		}
	    if (addrSize == 3)
		{
		cAddr.c[0] = *pChar++;
		cAddr.c[1] = *pChar++;
		cAddr.c[2] = *pChar++;
		cAddr.c[3] = *pChar++;
		}
	    if (hostaddr == 0x80)
		{
	        if (addrSize == 1)
		    hAddr.c[0] = *pChar++;
	        if (addrSize == 2)
		    {
		    hAddr.c[0] = *pChar++;
		    hAddr.c[1] = *pChar++;
		    }
	        if (addrSize == 3)
		    {
		    hAddr.c[0] = *pChar++;
		    hAddr.c[1] = *pChar++;
		    hAddr.c[2] = *pChar++;
		    hAddr.c[3] = *pChar++;
		    }
	        }
	    printf ("    Length = 0x%x cardAddress = 0x%x hostAddress = 0x%x\n",
		    length.l * 256, cAddr.l * 256, hAddr.l * 256);
	    }
	}
    
    return (pChar);
    }

/*******************************************************************************
*
* cisShowCmisc - Show information in Misc description structure.
*
* This routine shows CIS in the Misc description structure.
*
* RETURNS: A pointer to next entry.
*/

LOCAL u_char *cisShowCmisc
    (
    u_char *pChar,		/* pointer to misc description */
    u_char featureSelection	/* feature selection byte */
    )

    {
    BOOL extension = TRUE;

    while (extension)
	{
	if ((*pChar & 0x80) == 0x00)
	    extension = FALSE;
	
	printf ("    Max twins = %d\n", *pChar & 0x07);
	if (*pChar & 0x08)
	    printf ("    Audio\n");
	if (*pChar & 0x10)
	    printf ("    Read only\n");
	if (*pChar & 0x20)
	    printf ("    Power down bit is supported\n");
	if (*pChar & 0x40)
	    printf ("    Reserved\n");
	pChar++;
	}
    
    return (pChar);
    }

/*******************************************************************************
*
* cisShowCsub - Show information in Sub tuple.
*
* This routine shows CIS in the Sub tuple.
*
* RETURNS: N/A
*/

LOCAL void cisShowCsub
    (
    CIS_TUPLE *pTuple		/* pointer to a tuple */
    )

    {
    u_char *pChar	= (u_char *)pTuple + sizeof(CIS_TUPLE);
    u_char *pEnd	= pChar + pTuple->link;

    while ((pTuple->code == 0xc0) || (pTuple->code == 0xc1))
	{
        printf ("    Configuration subtuple\n");
        printf ("    ");
	while ((pChar < pEnd) && (*pChar != 0xff))
	    pChar += printf ("%s", pChar) + 1;
	printf ("\n");
	pTuple = (CIS_TUPLE *)pEnd;
	}
    }

/*******************************************************************************
*
* cisShowVers2 - Show information in level-2 version/product information tuple.
*
* This routine shows CIS in the level-2 version/product information tuple.
*
* RETURNS: N/A
*/

LOCAL void cisShowVers2
    (
    CIS_TUPLE *pTuple		/* pointer to a tuple */
    )

    {
    u_char *pChar	= (u_char *)pTuple + sizeof(CIS_TUPLE);
    CIS_BYTE2  index;

    pChar++;
    printf ("  Level of compliance = %d\n", *pChar++);
    index.c[0] = *pChar++;
    index.c[1] = *pChar++;
    printf ("  Address of first data byte in card = 0x%x\n", index.s);
    pChar += 2;
    printf ("  Vender specific bytes = 0x%x 0x%x\n", *pChar++, *pChar++);
    printf ("  Number of copies of the CIS = 0x%x\n", *pChar++);
    printf ("  ");
    pChar += printf ("%s", pChar) + 1;
    printf ("\n  ");
    printf ("%s\n", pChar);
    }

/*******************************************************************************
*
* cisShowFormat - Show information in format tuple.
*
* This routine shows CIS in the format tuple.
*
* RETURNS: N/A
*/

LOCAL void cisShowFormat
    (
    CIS_TUPLE *pTuple		/* pointer to a tuple */
    )

    {
    u_char *pChar	= (u_char *)pTuple + sizeof(CIS_TUPLE);
    u_char type;
    u_char flag;
    u_char edc;
    CIS_BYTE4 offset;
    CIS_BYTE4 nBytes;
    CIS_BYTE4 nBlocks;
    CIS_BYTE4 edcLoc;
    CIS_BYTE4 addr;
    CIS_BYTE2 blockSize;

    type = *pChar++;
    if (type == 0)
	printf ("  Format type = Disk\n");
    else if (type == 0)
	printf ("  Format type = Memory\n");
    else
	printf ("  Format type = Vender specific 0x%x\n", type);

    edc = *pChar++;
    printf ("  EDC type   = 0x%x\n", (edc & 0x78) >> 3);
    printf ("  EDC length = 0x%x\n", (edc & 0x07));
    offset.c[0] = *pChar++;
    offset.c[1] = *pChar++;
    offset.c[2] = *pChar++;
    offset.c[3] = *pChar++;
    printf ("  Address of first data byte  = 0x%x\n", offset.l);
    nBytes.c[0] = *pChar++;
    nBytes.c[1] = *pChar++;
    nBytes.c[2] = *pChar++;
    nBytes.c[3] = *pChar++;
    printf ("  Number of data bytes  = 0x%x\n", nBytes.l);

    if (type == 0)
	{
	blockSize.c[0]	= *pChar++;
	blockSize.c[1]	= *pChar++;
	nBlocks.c[0]	= *pChar++;
	nBlocks.c[1]	= *pChar++;
	nBlocks.c[2]	= *pChar++;
	nBlocks.c[3]	= *pChar++;
	printf ("  Block size            = %d\n", 1 << blockSize.s);
	printf ("  Number of data blocks = %d\n", nBlocks.l);
	edcLoc.c[0]	= *pChar++;
	edcLoc.c[1]	= *pChar++;
	edcLoc.c[2]	= *pChar++;
	edcLoc.c[3]	= *pChar++;
	if ((edc == 1) || (edc == 2))
	    printf ("  EDC Location          = 0x%x\n", edcLoc.l);
	else if (edc == 3)
	    printf ("  EDC PCC               = 0x%x\n", edcLoc.c[0]);
	}
    else if (type == 1)
	{
	flag = *pChar++;
	pChar++;
	addr.c[0]	= *pChar++;
	addr.c[1]	= *pChar++;
	addr.c[2]	= *pChar++;
	addr.c[3]	= *pChar++;
	edcLoc.c[0]	= *pChar++;
	edcLoc.c[1]	= *pChar++;
	edcLoc.c[2]	= *pChar++;
	edcLoc.c[3]	= *pChar++;
	printf ("  Flags = 0x%x\n", flag);
	if (flag & 1)
	    printf ("  Mapped physical address = 0x%x\n", addr.l);
	printf ("  EDC location = 0x%x\n", edcLoc.l);
	}
    }

