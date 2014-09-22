/* usrVxFusion.c - VxFusion distributed objects initialization */

/* Copyright 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01d,01jun99,drm  Adding code to check for and extract shared memory network
                 information.
01c,19may99,drm  Changing windmp to vxfusion.
01b,10mar99,drm  changing WindMP to VxFusion to reflect new product name
01a,01sep98,drm  written.
*/

/*
DESCRIPTION
This file is used to initialize the VxFusion distributed object support.  

NOMANUAL
*/

#ifndef  __INCusrVxFusionc
#define  __INCusrVxFusionc

/* includes */

#include "vxWorks.h"
#include "vxfusion/distIfLib.h"
#include "vxfusion/distLib.h"
#include "drv/vxfusion/distIfUdp.h"



/*******************************************************************************
*
* usrVxFusionInit - initialize VxFusion (distributed objects)
* 
* This function parses the boot string and then calls distInit() to initialize
* VxFusion using the reference UDP adapter.  It uses the boot device name and 
* unit number to determine the interface which is passed to the UDP adapter
* as adapter-specific configuration data.
*
*
* RETURNS: OK, or ERROR if unsuccessful.
*
* NOMANUAL
*/

STATUS usrVxFusionInit 
    (
    char * bootString       /* boot parameter string */
    )

    {
    STATUS rval;                            /* the return val from distInit() */
    BOOT_PARAMS   params;                   /* boot paramters */
    char ipStrAddr [INET_ADDR_LEN];         /* ip address portion of string */
    u_long ipAddress;                       /* ipAddress in integer form */
    char *pLoc;                             /* ptr to delimiting character */
    char bootDevStr [BOOT_DEV_LEN];         /* boot device */
    char interface [BOOT_DEV_LEN];          /* interface */


    if (bootString == NULL)
        bootString = BOOT_LINE_ADRS;

    /* interpret boot command */

    if (usrBootLineCrack (bootString, &params) != OK)
        return (ERROR);


    /*
     * Determine the booting network interface.  If the booting interface is
     * a non-shared memory interface, assume it is an ethernet interface
     * and use the ethernet internet address as the node id.  If the booting
     * interface is a shared memory interface, use the backplane internet
     * address as the node id.
     *
     * Once these values are determined, call distInit() filling in the rest
     * of the parameters using the default values.
     */

    strcpy (bootDevStr, params.bootDev);
    if ( strncmp (bootDevStr, "sm=",3) == 0)
        {
        /* Shared memory */

        pLoc = strchr (bootDevStr,'=');
        if (pLoc)
            *pLoc = '\0';
        sprintf (interface,"%s%d",bootDevStr, params.unitNum);

        strncpy (ipStrAddr, params.bad, INET_ADDR_LEN);
        ipStrAddr[INET_ADDR_LEN-1] = '\0';
        pLoc = strchr (ipStrAddr,':');  /* Strip subnet mask */
        if (pLoc)
            *pLoc = '\0';
        ipAddress = inet_network (ipStrAddr);

        }
    else
        {
        /* Not shared memory - assume an ethernet interface */

        pLoc = strchr (bootDevStr,'(');
        if (pLoc)
            *pLoc = '\0';
        sprintf (interface,"%s%d",bootDevStr, params.unitNum);

        strncpy (ipStrAddr, params.ead, INET_ADDR_LEN);
        ipStrAddr[INET_ADDR_LEN-1] = '\0';
        pLoc = strchr (ipStrAddr,':');  /* Strip subnet mask */
        if (pLoc)
            *pLoc = '\0';
        ipAddress = inet_network (ipStrAddr);
        }

    printf ("Initializing VxFusion with parameters: \n");
    printf ("  node id: 0x%lx\n", ipAddress);
    printf ("  interface: %s\n", interface);
    printf ("  max number of TBufs: %u\n", 1 << DIST_MAX_TBUFS_LOG2_DFLT);
    printf ("  max number of nodes in node DB: %u\n", 
            1 << DIST_MAX_NODES_LOG2_DFLT);
    printf ("  max number of queues on this node: %u\n", 
            1 << DIST_MAX_QUEUES_LOG2_DFLT);
    printf ("  max number of groups in group DB: %u\n", 
            1 << DIST_MAX_GROUPS_LOG2_DFLT);
    printf ("  max number of entries in name DB: %u\n", 
            1 << DIST_MAX_NAME_DB_ENTRIES_LOG2_DFLT);

    printf ("  number of clock ticks to wait: ");
    if (DIST_MAX_TICKS_TO_WAIT_DFLT < 0)
       {
       printf ("FOREVER\n");
       }
    else 
       { 
       printf ("%d\n", DIST_MAX_TICKS_TO_WAIT_DFLT); 
       }

    rval = distInit (ipAddress,
                     distIfUdpInit,
                     interface,
                     DIST_MAX_TBUFS_LOG2_DFLT,
                     DIST_MAX_NODES_LOG2_DFLT,
                     DIST_MAX_QUEUES_LOG2_DFLT,
                     DIST_MAX_GROUPS_LOG2_DFLT,
                     DIST_MAX_NAME_DB_ENTRIES_LOG2_DFLT,
                     DIST_MAX_TICKS_TO_WAIT_DFLT );

    if (rval != OK)
        {
        printf ("Unable to initialize VxFusion.\n");
        return (ERROR);
        }

    printf ("VxFusion initialization successful.\n");
    return (OK);
    }

#endif /* __INCusrVxFusionc */

