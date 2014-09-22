/* ptyDrv.c - pseudo-terminal driver */

/* Copyright 1984-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01z,12oct01,brk  added SELECT functionality to Master (SPR #65498) 
01y,19oct01,dcb  Fix the routine title line to match the coding conventions.

01x,14feb01,spm  merged from version 01x of tor2_0_x branch (base 01w):
                 added removal of pty device (SPR #28675)
01w,12mar99,p_m  Fixed SPR 9124 by mentioning that there is no way to delete a
                 pty device.
01v,03feb93,jdi  documentation cleanup for 5.1.
01u,13nov92,dnw  added include of semLibP.h
01t,21oct92,jdi  removed mangen SECTION designation.
01s,18jul92,smb  Changed errno.h to errnoLib.h.
01r,04jul92,jcf  scalable/ANSI/cleanup effort.
01q,26may92,rrr  the tree shuffle
01p,19nov91,rrr  shut up some ansi warnings.
01o,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed includes to have absolute path from h/
		  -changed VOID to void
		  -changed copyright notice
01n,05apr91,jdi	 documentation -- removed header parens and x-ref numbers;
		 doc review by rdc.
01m,27feb91,jaa	 documentation update.
01l,26jun90,jcf  changed semaphore type for 5.0.  Embedded semaphore.
01k,23mar90,dab  removed ptyMasterDelete() and ptySlaveDelete().
01j,15feb90,dab  added ptyMasterDelete() and ptySlaveDelete(). documentation.
01i,30may88,dnw  changed to v4 names.
01h,04may88,jcf  changed semaphores to be consistent with new semLib.
01g,06nov87,ecs  documentation.
01f,24oct87,gae  added pty{Master,Slave}Close() so that reads on
		   corresponding would return 0/ERROR.
01e,20oct87,gae  made ptyDrv() return correct status on succesive calls.
		 documentation.
01d,25mar87,jlf  documentation
01c,21dec86,dnw  changed to not get include files from default directories.
01b,02jul86,jlf  documentation
01a,01apr86,rdc  wrotten.
*/

/*
The pseudo-terminal driver provides a tty-like interface between a master and
slave process, typically in network applications.  The master process
simulates the "hardware" side of the driver (e.g., a USART serial chip), while
the slave process is the application program that normally talks to the driver.

USER-CALLABLE ROUTINES
Most of the routines in this driver are accessible only through the I/O
system.  However, the following routines must be called directly: ptyDrv() to
initialize the driver, ptyDevCreate() to create devices, and ptyDevRemove()
to remove an existing device.

INITIALIZING THE DRIVER
Before using the driver, it must be initialized by calling ptyDrv().
This routine must be called before any reads, writes, or calls to
ptyDevCreate().

CREATING PSEUDO-TERMINAL DEVICES
Before a pseudo-terminal can be used, it must be created by calling
ptyDevCreate():
.CS
    STATUS ptyDevCreate
	(
	char  *name,	  /@ name of pseudo terminal      @/
	int   rdBufSize,  /@ size of terminal read buffer @/
	int   wrtBufSize  /@ size of write buffer         @/
	)
.CE
For instance, to create the device pair "/pty/0.M" and "/pty/0.S",
with read and write buffer sizes of 512 bytes, the proper call would be:
.CS
   ptyDevCreate ("/pty/0.", 512, 512);
.CE
When ptyDevCreate() is called, two devices are created, a master and
slave.  One is called <name>M and the other <name>S.  They can
then be opened by the master and slave processes.  Data written to the
master device can then be read on the slave device, and vice versa.  Calls
to ioctl() may be made to either device, but they should only apply to the
slave side, since the master and slave are the same device.

The ptyDevRemove() routine will delete an existing pseudo-terminal device
and reclaim the associated memory. Any file descriptors associated with
the device will be closed.

IOCTL FUNCTIONS
Pseudo-terminal drivers respond to the same ioctl() functions used by
tty devices.  These functions are defined in ioLib.h and documented in
the manual entry for tyLib.

INCLUDE FILES: ioLib.h, ptyDrv.h

SEE ALSO: tyLib,
.pG "I/O System"
*/

#include "vxWorks.h"
#include "ioLib.h"
#include "iosLib.h"
#include "semLib.h"
#include "stdlib.h"
#include "wdLib.h"
#include "selectLib.h"
#include "tyLib.h"
#include "ptyDrv.h"
#include "string.h"
#include "errnoLib.h"
#include "private/semLibP.h"
#include "private/funcBindP.h" 

IMPORT STATUS tyDevRemove(TY_DEV_ID pTyDev);

#define PTY_WRITE_THRESHOLD		20	


/* local variables */

LOCAL int ptySlaveDrvNum;	/* driver number assigned to slave driver */
LOCAL int ptyMasterDrvNum;	/* driver number assigned to master drv */
LOCAL int ptyWrtThreshold  = PTY_WRITE_THRESHOLD;
 /* min bytes free in output buffer */

/* forward static functions */

static int ptyMasterOpen (PSEUDO_DEV *pPseudoDev, char *name, int mode); 
static int ptySlaveOpen (PSEUDO_DEV *pPseudoDev, char *name, int mode);
static STATUS ptySlaveClose (PSEUDO_DEV *pPseudoDev);
static STATUS ptyMasterClose (PSEUDO_DEV *pPseudoDev);
static int ptySlaveRead (PSEUDO_DEV *pPseudoDev, char *buffer, int maxbytes);
static int ptyMasterRead (PSEUDO_DEV *pPseudoDev, char *buffer, int
		maxbytes);
static int ptySlaveWrite (PSEUDO_DEV *pPseudoDev, char *buffer, int nbytes);
static int ptyMasterWrite (PSEUDO_DEV *pPseudoDev, char *buffer, int nbytes);
static STATUS ptySlaveIoctl (PSEUDO_DEV *pPseudoDev, int request, int arg);
static STATUS ptyMasterIoctl (PSEUDO_DEV *pPseudoDev, int request, int arg);
static void ptyMasterStartup (PSEUDO_DEV *pPseudoDev);

/*******************************************************************************
*
* ptyDrv - initialize the pseudo-terminal driver
*
* This routine initializes the pseudo-terminal driver.
* It must be called before any other routine in this module.
*
* RETURNS: OK, or ERROR if the master or slave devices cannot be installed.
*/

STATUS ptyDrv (void)
    {
    static BOOL done;	/* FALSE = not done, TRUE = done */
    static STATUS status;

    if (!done)
	{
	done = TRUE;

	ptySlaveDrvNum = iosDrvInstall (ptySlaveOpen, (FUNCPTR) NULL,
					ptySlaveOpen, ptySlaveClose,
					ptySlaveRead, ptySlaveWrite,
					ptySlaveIoctl);

	ptyMasterDrvNum = iosDrvInstall (ptyMasterOpen, (FUNCPTR) NULL,
					ptyMasterOpen, ptyMasterClose,
					ptyMasterRead, ptyMasterWrite,
					ptyMasterIoctl);

	status = (ptySlaveDrvNum != ERROR && ptyMasterDrvNum != ERROR) ? OK
								       : ERROR;
	}

    return (status);
    }
/*******************************************************************************
*
* ptyDevCreate - create a pseudo terminal
*
* This routine creates a master and slave device which can then be opened by
* the master and slave processes.  The master process simulates the "hardware"
* side of the driver, while the slave process is the application program that
* normally talks to a tty driver.   Data written to the master device can then
* be read on the slave device, and vice versa.
*
* RETURNS: OK, or ERROR if memory is insufficient.
*/

STATUS ptyDevCreate
    (
    char *name,         /* name of pseudo terminal */
    int rdBufSize,      /* size of terminal read buffer */
    int wrtBufSize      /* size of write buffer */
    )
    {
    STATUS status;
    char nameBuf [MAX_FILENAME_LENGTH];
    PSEUDO_DEV *pPseudoDev;

    if (ptySlaveDrvNum < 1 || ptyMasterDrvNum < 1)
	{
	errnoSet (S_ioLib_NO_DRIVER);
	return (ERROR);
	}

    pPseudoDev = (PSEUDO_DEV *) malloc (sizeof (PSEUDO_DEV));

    if (pPseudoDev == NULL)
	return (ERROR);

    /* initialize device descriptor */

    if (tyDevInit ((TY_DEV_ID) pPseudoDev, rdBufSize, wrtBufSize,
		   (FUNCPTR)ptyMasterStartup) != OK)
	{
	free ((char *)pPseudoDev);
	return (ERROR);
	}

    /* initialize the select stuff for the master pty fd */

    if (_func_selWakeupListInit != NULL)
        (* _func_selWakeupListInit) (&pPseudoDev->masterSelWakeupList);

    semBInit (&pPseudoDev->masterReadSyncSem, SEM_Q_PRIORITY, SEM_EMPTY);

    /* add Slave and Master devices */

    strcpy (nameBuf, name);
    strcat (nameBuf, "S");
    status = iosDevAdd (&pPseudoDev->slaveDev, nameBuf, ptySlaveDrvNum);

    if (status == OK)
	{
	strcpy (nameBuf, name);
	strcat (nameBuf, "M");
	status = iosDevAdd ((DEV_HDR *) pPseudoDev, nameBuf, ptyMasterDrvNum);
	}

    return (status);
    }

/******************************************************************************
*
* ptyDevRemove - destroy a pseudo terminal
*
* This routine removes an existing master and slave device and releases all
* allocated memory. It will close any open files using either device.
*
* RETURNS: OK, or ERROR if terminal not found
*
* INTERNAL
* This routine reverses the ptyDevCreate() routine operations.
*/

STATUS ptyDevRemove
    (
    char * 	pName         /* name of pseudo terminal to remove */
    )
    {
    char * 	pTail = NULL; 	/* Pointer to tail of device name. */
    DEV_HDR * 	pMasterDev;
    DEV_HDR * 	pSlaveDev;

    char nameBuf [MAX_FILENAME_LENGTH];
    PSEUDO_DEV *pPseudoDev;

    if (ptySlaveDrvNum < 1 || ptyMasterDrvNum < 1)
        {
        errnoSet (S_ioLib_NO_DRIVER);
        return (ERROR);
        }

    strcpy (nameBuf, pName);
    strcat (nameBuf, "M");

    pMasterDev = iosDevFind (nameBuf, &pTail);
    if (pMasterDev == NULL || pTail == nameBuf)     /* Device not found? */
        return (ERROR);

    strcpy (nameBuf, pName);
    strcat (nameBuf, "S");

    pSlaveDev = iosDevFind (nameBuf, &pTail);
    if (pSlaveDev == NULL || pTail == nameBuf)      /* Device not found? */
        return (ERROR);

    /* Close any open files and remove the master and slave devices. */

    iosDevDelete (pSlaveDev);
    iosDevDelete (pMasterDev);

    /*
     * The master device header is also the header for the ty device and
     * a pointer to the overall pseudo device structure. Remove those
     * data structures.
     */

    pPseudoDev = (PSEUDO_DEV *)pMasterDev;

    tyDevRemove ( (TY_DEV_ID)pMasterDev);

    free (pPseudoDev);

    return (OK);
    }

/*******************************************************************************
*
* ptyMasterOpen - open the Master side of a pseudo terminal
*
* ARGSUSED1
*/

LOCAL int ptyMasterOpen
    (
    PSEUDO_DEV *pPseudoDev,
    char *name,
    int mode
    )
    {
    return ((int) pPseudoDev);
    }

/*******************************************************************************
*
* ptySlaveOpen - open the Slave side of a pseudo terminal
*
* ARGSUSED1
*/

LOCAL int ptySlaveOpen
    (
    PSEUDO_DEV *pPseudoDev,
    char *name,
    int mode
    )
    {
    return ((int) pPseudoDev - (OFFSET(PSEUDO_DEV, slaveDev)));
    }
/*******************************************************************************
*
* ptySlaveClose - close the Slave side of a pseudo terminal
*
* Closing the slave side should cause the master to read 0 bytes.
*
* RETURNS: OK
*/


LOCAL STATUS ptySlaveClose
    (
    PSEUDO_DEV *pPseudoDev
    )
    {
    pPseudoDev->ateof = FALSE;	/* indicate to master read of close */
    semGive (&pPseudoDev->masterReadSyncSem);
 
    if(_func_selWakeupAll != NULL) 
    	{
    	/* wake up any process waiting so they read the 0 bytes */

	(* _func_selWakeupAll) (&pPseudoDev->masterSelWakeupList, SELREAD);
    	}

    return (OK);
    }

/*******************************************************************************
*
* ptyMasterClose - close the Master side of a pseudo terminal
*
* Closing the master side will cause the slave's read to return ERROR.
*
* RETURNS: OK
*/

LOCAL STATUS ptyMasterClose
    (
    PSEUDO_DEV *pPseudoDev
    )
    {
    tyIoctl ((TY_DEV_ID) pPseudoDev, FIOCANCEL, 0 /*XXX*/);
    return (OK);
    }
/*******************************************************************************
*
* ptySlaveRead - slave read routine
*
* The slave device simply calls tyRead to get data out of the ring buffer.
*
* RETURNS: whatever tyRead returns (number of bytes actually read)
*/

LOCAL int ptySlaveRead
    (
    PSEUDO_DEV *pPseudoDev,
    char *buffer,
    int maxbytes
    )
    {
    int i;

    i=tyRead ((TY_DEV_ID) pPseudoDev, buffer, maxbytes);

    if (_func_selWakeupAll != NULL) 
    	{
    	/* When anything gets read from the slave and the buffer is below
         * the threshhold wake processes waiting
         * on SELWRITE of master pty fd 
         */

 	if (rngNBytes (pPseudoDev->tyDev.rdBuf) < ptyWrtThreshold )  
    		(* _func_selWakeupAll) (&pPseudoDev->masterSelWakeupList, SELWRITE );
    	}

    return(i); 	
    }
/*******************************************************************************
*
* ptyMasterRead - master read routine
*
* The master read routine calls tyITx to empty the pseudo terminal's buffer.
*
* RETURNS: number of characters actually read
*/

LOCAL int ptyMasterRead
    (
    PSEUDO_DEV *pPseudoDev,
    char *buffer,               /* where to return characters read */
    int maxbytes
    )
    {
    int i;
    char ch;

    pPseudoDev->ateof = TRUE;

    for (i = 0; i == 0;)
	{
	while ((i < maxbytes) && (tyITx ((TY_DEV_ID) pPseudoDev, &ch) == OK))
	    buffer [i++] = ch;

	if (!pPseudoDev->ateof)
	    break;

	if (i == 0)
	    semTake (&pPseudoDev->masterReadSyncSem, WAIT_FOREVER);
	}
    
    return (i);
    }
/*******************************************************************************
*
* ptySlaveWrite - pseudo terminal Slave write routine
*
* This routine simply calls tyWrite.
*
* RETURNS: whatever tyWrite returns (number of bytes actually written)
*/

LOCAL int ptySlaveWrite
    (
    PSEUDO_DEV *pPseudoDev,
    char *buffer,
    int nbytes
    )
    {
    int written;
     
    written = tyWrite ((TY_DEV_ID) pPseudoDev, buffer, nbytes);
    
    if(_func_selWakeupAll != NULL) 
    	{
    	/* When anything gets written to the slave wake processes waiting
         * on SELREAD of master pty fd 
         */

 	if(rngNBytes(pPseudoDev->tyDev.wrtBuf))   	
		(* _func_selWakeupAll) (&pPseudoDev->masterSelWakeupList, SELREAD);
    	}

    return(written);	
    }
/*******************************************************************************
*
* ptyMasterWrite - pseudo terminal Master write routine
*
* This routine calls tyIRd to put data in the pseudo terminals ring buffer.
*
* RETURNS: nbytes
*/

LOCAL int ptyMasterWrite
    (
    PSEUDO_DEV *pPseudoDev,
    char *buffer,
    int nbytes
    )
    {
    int i;

    for (i = 0; i < nbytes; i++)
	(void)tyIRd ((TY_DEV_ID) pPseudoDev, buffer [i]);

     return (i);
    }
/*******************************************************************************
*
* ptySlaveIoctl - special device control
*/

LOCAL STATUS ptySlaveIoctl
    (
    PSEUDO_DEV *pPseudoDev,     /* device to control */
    int request,                /* request code */
    int arg                     /* some argument */
    )
    {
    return (tyIoctl ((TY_DEV_ID) pPseudoDev, request, arg));
    }
/*******************************************************************************
*
* ptyMasterIoctl - special device control
*/

LOCAL STATUS ptyMasterIoctl
    (
    PSEUDO_DEV *pPseudoDev,     /* device to control */
    int request,                /* request code */
    int arg                     /* some argument */
    )
    {
	switch (request)
	{	
	   case FIOSELECT:
		if ( _func_selPtyAdd != NULL )
		   	_func_selPtyAdd(pPseudoDev, arg);  
		break;

       	   case FIOUNSELECT:
		if ( _func_selPtyDelete != NULL )
		   	_func_selPtyDelete(pPseudoDev, arg);  
		break;

	   default:
	    	return (tyIoctl ((TY_DEV_ID) pPseudoDev, request, arg));
	}
	return(OK);
    }

/*******************************************************************************
*
* ptyMasterStartup - start up the Master read routine
*/

LOCAL void ptyMasterStartup
    (
    PSEUDO_DEV *pPseudoDev
    )
    {
    semGive (&pPseudoDev->masterReadSyncSem);
    }

#undef PTY_DEBUG
#ifdef PTY_DEBUG

extern DL_LIST 	iosDvList;			/* list of I/O device headers */

/*******************************************************************************
*
*  ptyShow - show the state of the Pty Buffers 
*/

void ptyShow(void)
{

    PSEUDO_DEV *pDevHdr;
    int rd,wrt;	

    logMsg("%-20s %-8s %-8s\n", "name","read","write",0,0,0);

    for (pDevHdr = (PSEUDO_DEV *) DLL_FIRST (&iosDvList); pDevHdr != NULL;
		pDevHdr = (PSEUDO_DEV *) DLL_NEXT (&pDevHdr->tyDev.devHdr.node))
    {	
        if( pDevHdr->tyDev.devHdr.drvNum == ptyMasterDrvNum )
	{	
		rd = rngNBytes (pDevHdr->tyDev.rdBuf);
		wrt = rngNBytes (pDevHdr->tyDev.wrtBuf);
		
		logMsg("%-20s %-8d %-8d\n", pDevHdr->tyDev.devHdr.name, wrt,rd,0,0,0);
	}
     }
}
#endif
