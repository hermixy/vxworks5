/* dhcpcShow.c - DHCP run-time client information display routines */

/* Copyright 1984 - 2001 Wind River Systems, Inc.  */
#include "copyright_wrs.h"

/*
modification history
--------------------
01q,12may02,kbw  man page edits
01p,10may02,kbw  making man page edits
01o,23apr02,wap  use dhcpTime() instead of time() (SPR #68900)
01n,15oct01,rae  merge from truestack ver 01m, base 01l
01m,16oct00,niq  Integrating T3 DHCP
01l,17jan98,kbw  removed NOMANUAL from dhcpcShowInit()
01k,14dec97,kbw  making minor man page fixes
01j,10dec97,kbw  making minor man page fixes
01i,04dec97,spm  added code review modifications
01h,21oct97,kbw  making minor man page fixes
01g,26aug97,spm  major overhaul: changed user interface to support multiple 
                 leases at run-time; changed function names and renamed
                 module to comply with coding standards
01f,06aug97,spm  removed parameters linked list to reduce memory required
01e,30jul97,kbw  fixed man page problems found in beta review
01d,02jun97,spm  updated man pages and added ERRNO entries
01c,17apr97,kbw  fixed minor format errors in man page, spell-checked
01b,07apr97,spm  corrected array indexing bugs, modified displayed parameter 
                 descriptions, rewrote documentation
01a,31jan97,spm  created by extracting routines from netShow.c
*/

/*
DESCRIPTION
This library provides routines that display various data related to
the DHCP run-time client library such as the lease timers and responding 
server.  The dhcpcShowInit() routine links the show facility into the VxWorks
image.  This happens automatically if INCLUDE_NET_SHOW and INCLUDE_DHCPC 
are defined at the time the image is built.

INCLUDE FILES: dhcpcLib.h

SEE ALSO: dhcpcLib
*/

#include <stdio.h>

#include "vxWorks.h"
#include "inetLib.h"

#include "dhcpcLib.h"
#include "dhcp/dhcpcCommonLib.h"
#include "dhcp/dhcpcStateLib.h"

#include "time.h"

IMPORT SEM_ID           dhcpcMutexSem;   /* protects the status indicators */

/* forward declarations */

STATUS dhcpcServerShow (void *);
STATUS dhcpcTimersShow (void *);
STATUS dhcpcParamsShow (void *);

/******************************************************************************
*
* dhcpcShowInit - initialize the DHCP show facility
*
* This routine links the DHCP show facility into the VxWorks system image. 
* It is called from usrNetwork.c automatically if INCLUDE_DHCP and 
* INCLUDE_NET_SHOW are defined at the time the image is constructed.
*
*/

void dhcpcShowInit (void)
    {
    return;
    }

/*******************************************************************************
*
* dhcpcServerShow - display current DHCP server
*
* This routine prints the IP address of the DHCP server that provided the 
* parameters for the lease identified by <pCookie>.  It has no effect if the
* indicated lease is not currently active.
*
* RETURNS: OK, or ERROR if lease identifier unknown.
*
* ERRNO: 
*  S_dhcpcLib_BAD_COOKIE
*
*/

STATUS dhcpcServerShow 
    (
    void * 	pCookie 	/* identifier returned by dhcpcInit() */
    )
    {
    int 		offset;
    LEASE_DATA * 	pLeaseData = NULL;
    char 		addrBuf [INET_ADDR_LEN];
    STATUS 		result = OK;

    /*
     * Use the cookie to access the lease-specific data structures.  For now,
     * just typecast the cookie.  This translation could be replaced with a more
     * sophisticated lookup at some point.
     */

    pLeaseData = (LEASE_DATA *)pCookie;

    for (offset = 0; offset < dhcpcMaxLeases; offset++)
        if (dhcpcLeaseList [offset] != NULL &&
                dhcpcLeaseList [offset] == pLeaseData)
            break;

    if (offset == dhcpcMaxLeases)
        {
        errno = S_dhcpcLib_BAD_COOKIE;
        return (ERROR);
        }

    /* Ignore show request if lease is not initialized or not bound.  */

    if (!dhcpcInitialized || !pLeaseData->initFlag)
        return (OK);

    semTake (dhcpcMutexSem, WAIT_FOREVER);
    if (!pLeaseData->leaseGood)
        result = ERROR;
    semGive (dhcpcMutexSem);

    if (result == ERROR)
        return (OK);

    if (pLeaseData->leaseType == DHCP_BOOTP)
        {
        printf ("DHCP server (BOOTP proxy): "); 
        inet_ntoa_b (pLeaseData->dhcpcParam->siaddr, addrBuf);
        printf ("%-18s\n", addrBuf);
        return (OK);
        }

    printf ("DHCP server: ");
    inet_ntoa_b (pLeaseData->dhcpcParam->server_id, addrBuf);
    printf("%-18s\n", addrBuf);

    return (OK);
    }

/*******************************************************************************
*
* dhcpcTimersShow - display current lease timers
*
* This routine prints the time remaining with each of the DHCP lease timers
* for the lease identified by <pCookie>.  It has no effect if the indicated
* lease is not currently active.
*
* RETURNS: OK if show routine completes, or ERROR otherwise.
*
* ERRNO: 
*  S_dhcpcLib_BAD_COOKIE
*
*/

STATUS dhcpcTimersShow
    (
    void * 	pCookie 	/* identifier returned by dhcpcInit() */
    )
    {
    int 		offset;
    LEASE_DATA * 	pLeaseData = NULL;
    STATUS 		result = OK;

    time_t current = 0;
    long t1;
    long t2;

    /*
     * Use the cookie to access the lease-specific data structures.  For now,
     * just typecast the cookie.  This translation could be replaced with a more
     * sophisticated lookup at some point.
     */

    pLeaseData = (LEASE_DATA *)pCookie;

    for (offset = 0; offset < dhcpcMaxLeases; offset++)
        if (dhcpcLeaseList [offset] != NULL &&
                dhcpcLeaseList [offset] == pLeaseData)
            break;

    if (offset == dhcpcMaxLeases)
        {
        errno = S_dhcpcLib_BAD_COOKIE;
        return (ERROR);
        }

    /* Ignore show request if lease is not initialized or not bound.  */

    if (!dhcpcInitialized || !pLeaseData->initFlag)
        return (OK);

    semTake (dhcpcMutexSem, WAIT_FOREVER);
    if (!pLeaseData->leaseGood)
        result = ERROR;
    semGive (dhcpcMutexSem);

    if (result == ERROR)
        return (OK);

    if (pLeaseData->leaseType == DHCP_BOOTP)
        {
        printf ("No timer values: BOOTP reply accepted.\n");
        return (OK);
        }
 
    if (dhcpTime (&current) == -1)
        {
        printf ("time() error in dhcpcTimerShow() routine.\n");
        return (ERROR);
        }

    t1 = pLeaseData->dhcpcParam->lease_origin + 
             pLeaseData->dhcpcParam->dhcp_t1 - current;
    t2 = pLeaseData->dhcpcParam->lease_origin + 
             pLeaseData->dhcpcParam->dhcp_t2 - current;
   
    if (t1 <= 0)
        printf ("Timer T1 expired.\n");
    else
        printf ("Timer T1: %ld seconds remaining.\n", t1);
   
    if (t2 <= 0)
        printf ("Timer T2 expired.\n");
    else
        printf ("Timer T2: %ld seconds remaining.\n", t2);

    return (OK);
    }

/******************************************************************************
*
* dhcpcParamsShow - display current lease parameters
*
* This routine prints all lease parameters for the lease identified by 
* <pCookie>.  It has no effect if the indicated lease is not currently active.
* 
* RETURNS: OK, or ERROR if lease identifier unknown.
*
* ERRNO: 
*  S_dhcpcLib_BAD_COOKIE
*
*/

STATUS dhcpcParamsShow
    (
    void * 	pCookie 	/* identifier returned by dhcpcInit() */
    )
    {
    int 		offset;
    LEASE_DATA * 	pLeaseData = NULL;
    STATUS 		result = OK;

    int loop;
    struct dhcp_param * 	pDhcpcParam;

    char addrBuf [INET_ADDR_LEN];

    /*
     * Use the cookie to access the lease-specific data structures.  For now,
     * just typecast the cookie.  This translation could be replaced with a more
     * sophisticated lookup at some point.
     */

    pLeaseData = (LEASE_DATA *)pCookie;

    for (offset = 0; offset < dhcpcMaxLeases; offset++)
        if (dhcpcLeaseList [offset] != NULL &&
                dhcpcLeaseList [offset] == pLeaseData)
            break;

    if (offset == dhcpcMaxLeases)
        {
        errno = S_dhcpcLib_BAD_COOKIE;
        return (ERROR);
        }

    /* Ignore show request if lease is not initialized or not bound.  */

    if (!dhcpcInitialized || !pLeaseData->initFlag)
        return (OK);

    semTake (dhcpcMutexSem, WAIT_FOREVER);
    if (!pLeaseData->leaseGood)
        result = ERROR;
    semGive (dhcpcMutexSem);

    if (result == ERROR)
        return (OK);

    if (pLeaseData->leaseType == DHCP_BOOTP)
        {
        printf ("No parameter values: BOOTP reply accepted.\n");
        return (OK);
        }

    pDhcpcParam = pLeaseData->dhcpcParam;

    /* Print any string parameters.  */

    if (pDhcpcParam->sname != NULL)
        printf ("DHCP server name: %s\n", pDhcpcParam->sname);
 
    if (pDhcpcParam->file != NULL)
        printf ("Boot file name: %s\n", pDhcpcParam->file);

    if (pDhcpcParam->hostname != NULL)
        printf ("DHCP client name: %s\n", pDhcpcParam->hostname);

    if (pDhcpcParam->merit_dump != NULL)
        printf ("Merit dump file: %s\n", pDhcpcParam->merit_dump);

    if (pDhcpcParam->dns_domain != NULL)
        printf ("DNS domain name: %s\n", pDhcpcParam->dns_domain);

    if (pDhcpcParam->root_path != NULL)
        printf ("Client root path: %s\n", pDhcpcParam->root_path);

    if (pDhcpcParam->extensions_path != NULL)
        printf ("Options extension path: %s\n", pDhcpcParam->extensions_path);

    if (pDhcpcParam->nis_domain != NULL)
        printf ("NIS domain: %s\n", pDhcpcParam->nis_domain);

    if (pDhcpcParam->nb_scope != NULL)
        printf ("NetBIOS over TCP/IP scope: %s\n", pDhcpcParam->nb_scope);

    if (pDhcpcParam->errmsg != NULL)
        printf ("Error message: %s\n", pDhcpcParam->errmsg);

    if (pDhcpcParam->nisp_domain != NULL)
        printf ("NIS+ domain: %s\n", pDhcpcParam->nisp_domain);

    /* Print all TRUE boolean parameters.  */

    if (pDhcpcParam->ip_forward)
        printf ("IP forwarding enabled.\n");

    if (pDhcpcParam->nonlocal_srcroute)
        printf ("Non-local source route forwarding enabled.\n");

    if (pDhcpcParam->all_subnet_local)
        printf ("All subnets are local is TRUE.\n");
 
    if (pDhcpcParam->mask_discover)
        printf ("ICMP mask discovery enabled.\n");

    if (pDhcpcParam->mask_supplier)
        printf ("ICMP mask supplier is TRUE.\n");

    if (pDhcpcParam->router_discover)
        printf ("Router discovery enabled.\n");

    if (pDhcpcParam->trailer)
        printf ("ARP trailers enabled.\n");
  
    if (pDhcpcParam->ether_encap)
        printf ("RFC 1042 Ethernet encapsulation enabled.\n");
    else
        printf ("RFC 894 Ethernet encapsulation enabled.\n");

    if (pDhcpcParam->keepalive_garba)
        printf ("TCP keepalive garbage octet enabled.\n");

    /* Print all non-zero single numeric parameters.  */

    if (pDhcpcParam->time_offset != 0)
        printf ("Client time offset: %ld\n", pDhcpcParam->time_offset);

    if (pDhcpcParam->bootsize != 0)
        printf ("Boot image size: %d\n", pDhcpcParam->bootsize);
 
    if (pDhcpcParam->max_dgram_size != 0)
        printf ("Maximum datagram size: %d\n", pDhcpcParam->max_dgram_size);

    if (pDhcpcParam->default_ip_ttl != 0)
        printf ("Default IP Time-to-live: %d\n",
                pDhcpcParam->default_ip_ttl);

    if (pDhcpcParam->mtu_aging_timeout != 0)
        printf ("Path MTU timeout: %ld\n", pDhcpcParam->mtu_aging_timeout);

    if (pDhcpcParam->intf_mtu != 0)
        printf ("Interface MTU: %d\n", pDhcpcParam->intf_mtu);

    if (pDhcpcParam->arp_cache_timeout != 0)
        printf ("ARP cache timeout: %ld\n", pDhcpcParam->arp_cache_timeout);

    if (pDhcpcParam->default_tcp_ttl != 0)
        printf ("Default TCP Time-to-live: %d\n",
                pDhcpcParam->default_tcp_ttl);

    if (pDhcpcParam->keepalive_inter != 0)
        printf ("TCP keepalive interval: %ld\n",
                pDhcpcParam->keepalive_inter);

    if (pDhcpcParam->nb_nodetype != 0)
        printf ("NetBIOS node type: %d\n", pDhcpcParam->nb_nodetype);

    printf ("Client lease origin: %ld\n", pDhcpcParam->lease_origin);
    printf ("Client lease duration: %ld\n", pDhcpcParam->lease_duration);

    if (pDhcpcParam->dhcp_t1 != 0)
        printf ("Client renewal (T1) time value: %ld\n",
                pDhcpcParam->dhcp_t1);

    if (pDhcpcParam->dhcp_t2 != 0)
        printf ("Client rebinding (T2) time value: %ld\n",
                pDhcpcParam->dhcp_t2);

    /* Print multiple numeric parameters.  */

    if (pDhcpcParam->mtu_plateau_table != NULL)
        {
         printf ("MTU plateau table:\n ");
         if (pDhcpcParam->mtu_plateau_table->shortnum != NULL)
            {
            for (loop = 0; loop < pDhcpcParam->mtu_plateau_table->num; loop++)
               printf ("%d ", pDhcpcParam->mtu_plateau_table->shortnum [loop]);
            printf ("\n");
            }
         else
            printf ("empty.\n");
        }

    /* Print any single IP addresses.  */

    if (pDhcpcParam->server_id.s_addr != 0)
        {
        printf ("DHCP server: ");
        inet_ntoa_b (pDhcpcParam->server_id, addrBuf);
        printf ("%-18s\n", addrBuf);
        }

    if (pDhcpcParam->ciaddr.s_addr != 0)
        {
        printf ("Client IP address: ");
        inet_ntoa_b (pDhcpcParam->ciaddr, addrBuf);
        printf ("%-18s\n", addrBuf);
        }

    if (pDhcpcParam->yiaddr.s_addr != 0)
        {
        printf ("Assigned IP address: ");
        inet_ntoa_b (pDhcpcParam->yiaddr, addrBuf);
        printf ("%-18s\n", addrBuf);
        }

    if (pDhcpcParam->siaddr.s_addr != 0)
        {
        printf ("Next server IP address: ");
        inet_ntoa_b (pDhcpcParam->siaddr, addrBuf);
        printf ("%-18s\n", addrBuf);
        }

    if (pDhcpcParam->giaddr.s_addr != 0)
        {
        printf ("Relay agent IP address: ");
        inet_ntoa_b (pDhcpcParam->giaddr, addrBuf);
        printf ("%-18s\n", addrBuf);
        }

    if (pDhcpcParam->subnet_mask != NULL)
        {
        printf ("Client subnet mask: ");
        inet_ntoa_b (*pDhcpcParam->subnet_mask, addrBuf);
        printf ("%-18s\n", addrBuf);
        }

    if (pDhcpcParam->swap_server != NULL)
        {
        printf ("Client swap server: ");
        inet_ntoa_b (*pDhcpcParam->swap_server, addrBuf);
        printf ("%-18s\n", addrBuf);
        }

    if (pDhcpcParam->brdcast_addr != NULL)
        {
        printf ("Client broadcast address: ");
        inet_ntoa_b (*pDhcpcParam->brdcast_addr, addrBuf);
        printf ("%-18s\n", addrBuf);
        }

    if (pDhcpcParam->router_solicit.s_addr != 0)
        {
        printf ("Client router solicitation address: ");
        inet_ntoa_b (pDhcpcParam->router_solicit, addrBuf);
        printf ("%-18s\n", addrBuf);
        }

    /* Print any multiple IP addresses.  */

    if (pDhcpcParam->router != NULL)
        {
        printf ("Client IP routers:\n");
        if (pDhcpcParam->router->addr != NULL)
            {
            for (loop = 0; loop < pDhcpcParam->router->num; loop++)
                {
                inet_ntoa_b (pDhcpcParam->router->addr [loop], addrBuf);
                printf ("\t%-18s\n", addrBuf);
                }
            }
        else
            printf ("\tnone specified.\n");
        }

    if (pDhcpcParam->time_server != NULL)
        {
        printf ("Client time servers:\n");
        if (pDhcpcParam->time_server->addr != NULL)
            {
            for (loop = 0; loop < pDhcpcParam->time_server->num; loop++)
                {
                inet_ntoa_b (pDhcpcParam->time_server->addr [loop], addrBuf);
                printf ("\t%-18s\n", addrBuf);
                }
            }
        else
            printf ("\tnone specified.\n");
        }

    if (pDhcpcParam->name_server != NULL)
        {
        printf ("Client IEN 116 name servers:\n");
        if (pDhcpcParam->name_server->addr != NULL)
            {
            for (loop = 0; loop < pDhcpcParam->name_server->num; loop++)
                {
                inet_ntoa_b (pDhcpcParam->name_server->addr [loop], addrBuf);
                printf ("\t%-18s\n", addrBuf);
                }
            }
        else
            printf ("\tnone specified.\n");
        }

    if (pDhcpcParam->dns_server != NULL)
        {
        printf ("Client DNS name servers:\n");
        if (pDhcpcParam->dns_server->addr != NULL)
            {
            for (loop = 0; loop < pDhcpcParam->dns_server->num; loop++)
                {
                inet_ntoa_b (pDhcpcParam->dns_server->addr [loop], addrBuf);
                printf ("\t%-18s\n", addrBuf);
                }
            }
        else
            printf ("\tnone specified.\n");
        }

    if (pDhcpcParam->log_server != NULL)
        {
        printf ("Client log servers:\n");
        if (pDhcpcParam->log_server->addr != NULL)
            {
            for (loop = 0; loop < pDhcpcParam->log_server->num; loop++)
                {
                inet_ntoa_b (pDhcpcParam->log_server->addr [loop], addrBuf);
                printf ("\t%-18s\n", addrBuf);
                }
            }
        else
            printf ("\tnone specified.\n");
        }

    if (pDhcpcParam->cookie_server != NULL)
        {
        printf ("Client cookie servers:\n");
        if (pDhcpcParam->cookie_server->addr != NULL)
            {
            for (loop = 0; loop < pDhcpcParam->cookie_server->num; loop++)
                {
                inet_ntoa_b (pDhcpcParam->cookie_server->addr [loop], addrBuf);
                printf ("\t%-18s\n", addrBuf);
                }
            }
        else
            printf ("\tnone specified.\n");
        }

    if (pDhcpcParam->lpr_server != NULL)
        {
        printf ("Client line printer servers:\n");
        if (pDhcpcParam->lpr_server->addr != NULL)
            {
            for (loop = 0; loop < pDhcpcParam->lpr_server->num; loop++)
                {
                inet_ntoa_b (pDhcpcParam->lpr_server->addr [loop], addrBuf);
                printf ("\t%-18s\n", addrBuf);
                }
            }
        else
            printf ("\tnone specified.\n");
        }

    if (pDhcpcParam->impress_server != NULL)
        {
        printf ("Client impress servers:\n");
        if (pDhcpcParam->impress_server->addr != NULL)
            {
            for (loop = 0; loop < pDhcpcParam->impress_server->num; loop++)
                {
                inet_ntoa_b (pDhcpcParam->impress_server->addr [loop],
                             addrBuf);
                printf ("\t%-18s\n", addrBuf);
                }
            }
        else
            printf ("\tnone specified.\n");
        }

    if (pDhcpcParam->rls_server != NULL)
        {
        printf ("Client resource location servers:\n");
        if (pDhcpcParam->rls_server->addr != NULL)
            {
            for (loop = 0; loop < pDhcpcParam->rls_server->num; loop++)
                {
                inet_ntoa_b (pDhcpcParam->rls_server->addr [loop], addrBuf);
                printf ("\t%-18s\n", addrBuf);
                }
            }
        else
            printf ("\tnone specified.\n");
        }

    if (pDhcpcParam->nis_server != NULL)
        {
        printf ("Client NIS servers:\n");
        if (pDhcpcParam->nis_server->addr != NULL)
            {
            for (loop = 0; loop < pDhcpcParam->nis_server->num; loop++)
                {
                inet_ntoa_b (pDhcpcParam->nis_server->addr [loop], addrBuf);
                printf ("\t%-18s\n", addrBuf);
                }
            }
        else
            printf ("\tnone specified.\n");
        }

    if (pDhcpcParam->ntp_server != NULL)
        {
        printf ("Client NTP servers:\n");
        if (pDhcpcParam->ntp_server->addr != NULL)
            {
            for (loop = 0; loop < pDhcpcParam->ntp_server->num; loop++)
                {
                inet_ntoa_b (pDhcpcParam->ntp_server->addr [loop], addrBuf);
                printf ("\t%-18s\n", addrBuf);
                }
            }
        else
            printf ("\tnone specified.\n");
        }

    if (pDhcpcParam->nbn_server != NULL)
        {
        printf ("Client NetBIOS name servers:\n");
        if (pDhcpcParam->nbn_server->addr != NULL)
            {
            for (loop = 0; loop < pDhcpcParam->nbn_server->num; loop++)
                {
                inet_ntoa_b (pDhcpcParam->nbn_server->addr [loop], addrBuf);
                printf ("\t%-18s\n", addrBuf);
                }
            }
        else
            printf ("\tnone specified.\n");
        }

    if (pDhcpcParam->nbdd_server != NULL)
        {
        printf ("Client NetBIOS datagram distribution servers:\n");
        if (pDhcpcParam->nbdd_server->addr != NULL)
            {
            for (loop = 0; loop < pDhcpcParam->nbdd_server->num; loop++)
                {
                inet_ntoa_b (pDhcpcParam->nbdd_server->addr [loop], addrBuf);
                printf ("\t%-18s\n", addrBuf);
                }
            }
        else
            printf ("\tnone specified.\n");
        }

    if (pDhcpcParam->xfont_server != NULL)
        {
        printf ("Client X Window System font servers:\n");
        if (pDhcpcParam->xfont_server->addr != NULL)
            {
            for (loop = 0; loop < pDhcpcParam->xfont_server->num; loop++)
                {
                inet_ntoa_b (pDhcpcParam->xfont_server->addr [loop], addrBuf);
                printf ("\t%-18s\n", addrBuf);
                }
            }
        else
            printf ("\tnone specified.\n");
        }

    if (pDhcpcParam->xdisplay_manager != NULL)
        {
        printf ("Available X Window Display Manager systems:\n");
        if (pDhcpcParam->xdisplay_manager->addr != NULL)
            {
            for (loop = 0; loop < pDhcpcParam->xdisplay_manager->num; loop++)
               {
               inet_ntoa_b (pDhcpcParam->xdisplay_manager->addr[loop],
                            addrBuf);
               printf ("\t%-18s\n", addrBuf);
               }
            }
        else
            printf ("\tnone specified.\n");
        }

    if (pDhcpcParam->nisp_server != NULL)
        {
        printf ("Available NIS+ servers:\n");
        if (pDhcpcParam->nisp_server->addr != NULL)
            {
            for (loop = 0; loop < pDhcpcParam->nisp_server->num; loop++)
                {
                inet_ntoa_b (pDhcpcParam->nisp_server->addr [loop], addrBuf);
                printf ("\t%-18s\n", addrBuf);
                }
            }
        else
            printf ("\tnone specified.\n");
        }

    if (pDhcpcParam->mobileip_ha != NULL)
        {
        printf ("Available mobile IP home agents:\n");
        if (pDhcpcParam->mobileip_ha->addr != NULL)
            {
            for (loop = 0; loop < pDhcpcParam->mobileip_ha->num; loop++)
                {
                inet_ntoa_b (pDhcpcParam->mobileip_ha->addr [loop], addrBuf);
                printf ("\t%-18s\n", addrBuf);
                }
            }
        else
            printf ("\tnone specified.\n");
        }

    if (pDhcpcParam->smtp_server != NULL)
        {
        printf ("Available SMTP servers:\n");
        if (pDhcpcParam->smtp_server->addr != NULL)
            {
            for (loop = 0; loop < pDhcpcParam->smtp_server->num; loop++)
                {
                inet_ntoa_b (pDhcpcParam->smtp_server->addr [loop], addrBuf);
                printf ("\t%-18s\n", addrBuf);
                }
            }
        else
            printf ("\tnone specified.\n");
        }

    if (pDhcpcParam->pop3_server != NULL)
        {
        printf ("Available POP3 servers:\n");
        if (pDhcpcParam->pop3_server->addr != NULL)
            {
            for (loop = 0; loop < pDhcpcParam->pop3_server->num; loop++)
                {
                inet_ntoa_b (pDhcpcParam->pop3_server->addr [loop], addrBuf);
                printf ("\t%-18s\n", addrBuf);
                }
            }
        else
            printf ("\tnone specified.\n");
        }

    if (pDhcpcParam->nntp_server != NULL)
        {
        printf ("Available NNTP servers:\n");
        if (pDhcpcParam->nntp_server->addr != NULL)
            {
            for (loop = 0; loop < pDhcpcParam->nntp_server->num; loop++)
                {
                inet_ntoa_b (pDhcpcParam->nntp_server->addr [loop], addrBuf);
                printf ("\t%-18s\n", addrBuf);
                }
            }
        else
            printf ("\tnone specified.\n");
        }

    if (pDhcpcParam->dflt_www_server != NULL)
        {
        printf ("Available World Wide Web servers:\n");
        if (pDhcpcParam->dflt_www_server->addr != NULL)
            {
            for (loop = 0; loop < pDhcpcParam->dflt_www_server->num; loop++)
                {
                inet_ntoa_b (pDhcpcParam->dflt_www_server->addr [loop],
                             addrBuf);
                printf ("\t%-18s\n", addrBuf);
                }
            }
        else
            printf ("\tnone specified.\n");
        }

    if (pDhcpcParam->dflt_finger_server != NULL)
        {
        printf ("Available Finger servers:\n");
        if (pDhcpcParam->dflt_finger_server->addr != NULL)
            {
            for (loop = 0; loop < pDhcpcParam->dflt_finger_server->num; loop++)
                {
                inet_ntoa_b (pDhcpcParam->dflt_finger_server->addr [loop],
                             addrBuf);
                printf ("\t%-18s\n", addrBuf);
                }
            }
        else
            printf ("\tnone specified.\n");
        }

    if (pDhcpcParam->dflt_irc_server != NULL)
        {
        printf ("Available IRC servers:\n");
        if (pDhcpcParam->dflt_irc_server->addr != NULL)
            {
            for (loop = 0; loop < pDhcpcParam->dflt_irc_server->num; loop++)
                {
                inet_ntoa_b (pDhcpcParam->dflt_irc_server->addr [loop],
                             addrBuf);
                printf ("\t%-18s\n", addrBuf);
                }
            }
        else
            printf ("\tnone specified.\n");
        }

    if (pDhcpcParam->streettalk_server != NULL)
        {
        printf ("Available StreetTalk servers:\n");
        if (pDhcpcParam->streettalk_server->addr != NULL)
            {
            for (loop = 0; loop < pDhcpcParam->streettalk_server->num; loop++)
                {
                inet_ntoa_b (pDhcpcParam->streettalk_server->addr [loop],
                             addrBuf);
                printf ("\t%-18s\n", addrBuf);
                }
            }
        else
            printf ("\tnone specified.\n");
        }

    if (pDhcpcParam->stda_server != NULL)
        {
        printf ("Available StreetTalk Directory Assistance servers:\n");
        if (pDhcpcParam->stda_server->addr != NULL)
            {
            for (loop = 0; loop < pDhcpcParam->stda_server->num; loop++)
                {
                inet_ntoa_b (pDhcpcParam->stda_server->addr [loop], addrBuf);
                printf ("\t%-18s\n", addrBuf);
                }
            }
        else
            printf ("\tnone specified.\n");
        }

    /* Print any multiple IP address pairs.  */

    if (pDhcpcParam->policy_filter != NULL)
        {
        printf ("Client policy filters:\n");
        if (pDhcpcParam->policy_filter->addr != NULL)
            {
            printf ("Destination        Subnet mask\n");
            printf ("-----------        -----------\n");
            for (loop = 0; loop < pDhcpcParam->policy_filter->num; loop++)
               {
               inet_ntoa_b (pDhcpcParam->policy_filter->addr[loop * 2],
                            addrBuf);
               printf ("%-18s ", addrBuf);
               inet_ntoa_b (pDhcpcParam->policy_filter->addr[loop * 2 + 1],
                            addrBuf);
               printf ("%-18s\n", addrBuf);
               }
            }
        else
            printf ("\tnone specified.\n");
        }

    if (pDhcpcParam->static_route != NULL)
        {
        printf ("Client static routes:\n");
        if (pDhcpcParam->static_route->addr != NULL)
            {
            printf ("Destination        Router IP\n");
            printf ("-----------        ---------\n");
            for (loop = 0; loop < pDhcpcParam->static_route->num; loop++)
                {
                inet_ntoa_b (pDhcpcParam->static_route->addr [loop * 2],
                             addrBuf);
                printf ("%-18s ", addrBuf);
                inet_ntoa_b (pDhcpcParam->static_route->addr [loop * 2 + 1],
                             addrBuf);
                printf ("%-18s\n", addrBuf);
                }
            }
        else
            printf ("\tnone specified.\n");
        }

    /* Print any lists.  */

    if (pDhcpcParam->vendlist != NULL)
        {
        printf ("Vendor-specific data:\n ");
        if (pDhcpcParam->vendlist->list != NULL)
            {
            for (loop = 0; loop < pDhcpcParam->vendlist->len; loop++)
                printf("%d ", pDhcpcParam->vendlist->list [loop]);
            printf("\n");
            }
        else
            printf ("none specified.\n");
        }
    return (OK);
    }
