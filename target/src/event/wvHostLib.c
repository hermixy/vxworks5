/* wvHostLib.c -  host information library (WindView) */

/* Copyright 1984-1995 Wind River Systems, Inc. */
/*
modification history
--------------------
01i,03feb95,rhp more docn tweaks, already in last printed man pages
01h,01feb95,rhp docn formatting tweaks
01g,01feb95,rhp library man page: add ref to User's Guide
01f,05apr94,smb documentation changes.
01e,04mar94,smb changed prototype for wvHostInfoInit (SPR #3089)
01d,20jan94,smb documentation tweaks
01c,19jan94,smb documentation changes
01b,30dec93,c_s added routine wvHostInfoShow (), and fixed memory leak in 
		  wvHostInfoInit (SPRs #2793, #2802).
01a,10dec93,smb created
*/

/*
DESCRIPTION
This library provides a means of communicating characteristics
of the host to the target.

INCLUDE FILES: wvLib.h

SEE ALSO: wvLib,
.I WindView User's Guide
*/

#include "vxWorks.h"
#include "bootLib.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "sysLib.h"
#include "private/wvHostLibP.h"

HOSTINFO * pHostInfo;

/*****************************************************************************n
*
* wvHostInfoInit - initialize host connection information (WindView)
*
* This routine initializes the host connection information, and it can be
* called any time afer the wvInstInit() call in usrConfig.c and before event
* logging is enabled with wvEvtLogEnable().
*
* If the default port number is desired, set <port> to 0 or DEFAULT_PORT.
*
* If the <pIpAddress> is NULL then the booting host IP address is the 
* default host address.
*
* RETURN:
* OK, or ERROR if the host address cannot be calculated.
*
* SEE ALSO: wvLib
*/

STATUS wvHostInfoInit
    (
    char *    pIpAddress,                  /* host ip address */
    ushort_t  port                         /* host port to connect to */
    )
    {
    BOOT_PARAMS bootInfo;               /* used to store boot information */

    if (pHostInfo == NULL)
	{
	/* allocate some memory for the boot parameter structure */

	if ((pHostInfo = malloc (sizeof (struct hostInfo))) == NULL)
	    {
	    return (ERROR);
	    }
	}

    if (pIpAddress == NULL)
        {
        /* convert the boot string into an understandable structure */
        bootStringToStruct (sysBootLine, &bootInfo);

        /* get host ipAddress from bootParam */
        strcpy (pHostInfo->ipAddress, bootInfo.had);
        }
    else
        {
        strcpy (pHostInfo->ipAddress, pIpAddress);
        }

    if (port == 0)
        pHostInfo->port = DEFAULT_PORT;          /* default port number */
    else
        pHostInfo->port = port;                 /* port number */

    return (OK);
    }

/*****************************************************************************n
*
* wvHostInfoShow - show host connection information (WindView)
*
* This routine prints the WindView host connection information.  If this
* information has not been initialized with wvHostInfoInit(), a message is
* printed to that effect.
*
* RETURN:
* OK, or ERROR if the host information has not been initialized.
*
* SEE ALSO: wvHostInfoInit(), wvLib
*/

STATUS wvHostInfoShow (void)

    {
    if (pHostInfo == NULL)
	{
	printf ("WindView host connection information has not been set.\n");
	return ERROR;
	}
    else
	{
	printf ("Host IP Address  : %s\n", pHostInfo->ipAddress);
	printf ("Host Port Number : %hd\n", pHostInfo->port);
	return OK;
	}
    }
