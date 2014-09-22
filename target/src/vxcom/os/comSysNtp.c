/* comSysNtp.c - vxcom NTP clock module */

/* Copyright (c) 2001 Wind River Systems, Inc. */

/*

modification history
--------------------
01b,29jun01,nel  Add code to indirect interface through function ptr
                 interface.
01a,20jun01,nel  created

*/

/*
DESCRIPTION

This module defines comSysGetTime so that the NTP library is used to get 
an acurate system time, rather than relying on the system time which may 
be inacurate.

INCLUDE FILES: VxWorks.h, stdio.h, sntpclib.h, CIOlib.h
*/

/* includes */

#include "vxWorks.h"
#include "stdio.h"
#include "sntpcLib.h"

/* locals */

static int		comSysInit		= FALSE;
static unsigned long 	comSysAdjustment	= 0;
static unsigned long 	comSysLastTime		= 0;
static char * 		pComSysTimeServer 	= NULL;
static unsigned int	comSysTimeout		= 5;


/* forward declarations */

int sysClkRateGet (void);
void comSysSetTimeGetPtr (unsigned long (*pPtr)(void));

/**************************************************************************
*
* comSysNtpTimeAdjust - Gets's the NTP adjustment needed to correct the
*                    system clock.
*
* This function get's the NTP time and calculates the adjustment needed to
* bring the system clock into line with the NTP time. The value is stored
* and used by comSysTimeGet. If the system clock is modified outside VxCOM
* the function must be called afterwards to ensure that VxCOM functions
* correctly.
*
* RETURNS: N/A
*
*/

void comSysNtpTimeAdjust
    (
    time_t *	    	pCurrent	/* Pointer to current time or NULL */
    )
    {
    struct timespec	ntpTime;

    comSysInit = FALSE;
    if (sntpcTimeGet
        (pComSysTimeServer, 
         sysClkRateGet () * comSysTimeout, 
         &ntpTime) == OK)
        {
        time_t current;

        if (pCurrent == NULL)
            pCurrent = &current;

        time (pCurrent);

        comSysAdjustment = ntpTime.tv_sec - *pCurrent;
        comSysInit = TRUE;
        }
    else
        {
        /* The adjustment failed so default to using system clock */
        comSysAdjustment = 0;
        }
    comSysLastTime = 0;
    }

/**************************************************************************
*
* comSysNtpTimeGet - returns the time in seconds since Jan 1, 1970
*
* This function returns the time in seconds since Jan 1, 1970. It get's
* this time by using the NTP time of a known server.
*
* RETURNS: the time in seconds since Jan 1, 1970.
*
*/

unsigned long comSysNtpTimeGet (void)
    {
    time_t current;

    time (&current);
    
    /* Check to see if adjustment has been made. If not get the NTP time */
    /* and set adjustment. */
    if (!comSysInit)
        {
        comSysNtpTimeAdjust (&current);
        }

    /* check the time hasn't gotten out of step */
    if (comSysLastTime > current)
        {
        comSysNtpTimeAdjust (&current);
        }

    comSysLastTime = current;

    return current + comSysAdjustment;
    }

/**************************************************************************
*
* comSysNtpInit - Initialize the NTP Time module.
*
* Initializes the NTP Time module and installs the module into comSysLib
* to be used in place of the existing time module. 
*
* RETURNS: N/A
*
*/

void comSysNtpInit 
    (
    int 		broadcast, 	/* use broadcast to get server */
    char * 		pServer, 	/* server name or IP address */
    unsigned int	timeout		/* timeout */
    )
    {
    comSysSetTimeGetPtr (comSysNtpTimeGet);
    if (broadcast)
        {
        pComSysTimeServer = NULL;
        }
    else
        {
        pComSysTimeServer = pServer;
        }
    comSysTimeout = timeout;
    }
