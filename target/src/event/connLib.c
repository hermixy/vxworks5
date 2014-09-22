/* connLib.c -  target-host connection library (WindView) */

/* Copyright 1984-1995 Wind River Systems, Inc. */
/*
modification history
--------------------
01g,18jul96,pr  fixed bug in connectionInit for SPR 5144
01f,01feb95,rhp library man page: add ref to User's Guide
01e,05apr94,smb and more documentation tweaks
01d,07mar94,smb more documentation tweaks
01c,20jan94,smb documentation tweaks
01b,17jan94,smb added documentation
01a,10dec93,smb created
*/

/*
DESCRIPTION
This library provides routines for configuring the WindView target-host 
connection.

By default, the routines provide target-host communication over TCP
sockets.  Users can replace these routines with their own, without modifying
the kernel or the WindView architecture.  For example, the user may want
to write the WindView event buffer to shared memory, diskette, or hard disks.

Four routines are required: An initialization routine, a close connection
routine, an error handler, and a routine that transfers the data
from the event buffer to another location. 

The data transfer routine must complete its job before the 
next data transfer cycle.  If it fails to do so, a bandwidth
exceeded condition occurs and event logging stops.

INCLUDE FILES: connLib.h

SEE ALSO: wvLib,
.I WindView User's Guide
*/

#include "vxWorks.h"
#include "connLib.h"
#include "private/connLibP.h"

LOCAL FUNCPTR       connectionInitRtn = NULL;
LOCAL VOIDFUNCPTR   connectionCloseRtn;
LOCAL VOIDFUNCPTR   connectionErrorRtn;
LOCAL VOIDFUNCPTR   dataTransferRtn;

/*******************************************************************************
*
* connRtnSet - set up connection routines for target-host communication (WindView)
*
* This routine establishes four target-host communication routines; by default
* they are TCP socket routines.  Users can replace these routines with
* their own communication routines.  
*
* Four routines are required: An initialization routine, a close connection
* routine, an error handler, and a routine that transfers the data
* from the event buffer to another location. 
*
* The data transfer routine must complete its job before the 
* next data transfer cycle.  If it fails to do so, a bandwidth
* exceeded condition occurs and event logging stops.
*
* FUNCPTR and VOIDFUNCPTR are defined as follows,
* .CS
* typedef int                (*FUNCPTR) (...);
* typedef void               (*VOIDFUNCPTR) (...);
* .CE
*
* RETURNS: N/A.
*
* SEE ALSO: wvLib
*/

void connRtnSet 
    (
    FUNCPTR initRtn, 		/* connection initialization routine */
    VOIDFUNCPTR closeRtn, 	/* connection close routine */
    VOIDFUNCPTR errorRtn, 	/* connection error handling routine */
    VOIDFUNCPTR dataXferRtn	/* connection data transfer routine */
    )
    {
    /* connect supplied connection routines */
    connectionInitRtn = initRtn;
    connectionCloseRtn = closeRtn;
    connectionErrorRtn = errorRtn;
    dataTransferRtn = dataXferRtn;
    }


/*******************************************************************************
*
* connectionInit - set up the connection to the host receiver (WindView)
*
* RETURN: OK, otherwise ERROR if connection cannot be initialized.
*
* SEE ALSO:
* NOMANUAL
*/

STATUS connectionInit (void)
    {
    if (connectionInitRtn == NULL)
        return (ERROR);

    return((* connectionInitRtn) ());
    }

/*******************************************************************************
*
* connectionClose - close the connection to the host receiver (WindView)
*
* SEE ALSO:
* NOMANUAL
*/

void connectionClose (void)
    {
    (* connectionCloseRtn) ();
    }

/*******************************************************************************
*
* connectionError - Indicate an error has occurred (WindView)
*
* SEE ALSO:
* NOMANUAL
*/

void connectionError (void)
    {
    (* connectionErrorRtn) ();
    }


/*******************************************************************************
*
* dataTransfer - transfer data from the target to the host (WindView)
*
* SEE ALSO:
* NOMANUAL
*/

void dataTransfer
    (
    char * bufAddress,                          /* address of buffer */
    size_t bufSize                              /* amount of data in buffer */
    )
    {
    (* dataTransferRtn) (bufAddress, bufSize);
    }
