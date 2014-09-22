/* database.c - DHCP server data retrieval code */

/* Copyright 1984 - 2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01o,07may02,wap  Put debug messages under DHCPS_DEBUG (SPR #76495)
01n,29oct01,wap  Fix use of get_integer() with htons()/htonl() (SPR #34808)
01m,12oct01,rae  merge from truestack ver 01n, base 01j
                 SPRs 69547, 65163, 33576
01l,24oct00,spm  fixed modification history after tor3_x merge
01k,23oct00,niq  merged from version 01l of tor3_x branch (base version 01j)
01j,17dec97,spm  fixed byte order of address-based loop boundaries (SPR #20056)
01i,04dec97,spm  added code review modifications
01h,06oct97,spm  added newlines and component labels to default output
01g,26aug97,spm  modified comments for all routines
01f,02jun97,spm  added test for optional storage routine and updated man pages
01e,08may97,spm  corrected use of storage routines, removed memory leak, and
                 documented shutdown of server 
01d,30apr97,spm  added missing START call to DHCPS_LEASE_HOOK storage routine
01c,18apr97,spm  added conditional include DHCPS_DEBUG for displayed output
01b,08apr97,spm  corrected byte ordering when adding MTU plateau table option
01a,07apr97,spm  created by modifying WIDE project DHCP implementation
*/

/*
DESCRIPTION
This library contains the code used by the DHCP server to access permanent
storage through two optional function hooks provided by the user. 

The function specified by dhcpsAddressHookAdd() is used to store and retrieve 
address pool entries which are added by the user with dhcpsLeaseEntryAdd(). 
This capability allows the user to bypass the static table definition and
add entries without recompiling the runtime image. If the storage hook is
not provided, only the address pool entries contained in the static tables
will be retained across server reboots. As a result, any client using other
address pool entries will be unable to renew its lease.

The second storage hook, specified by dhcpsLeaseHookAdd(), is much more 
critical. It is used to store and retrieve the subset of address pool entries 
which have been offered to a client, or are currently in use. If this storage 
hook is not provided, the server might issue network addresses which are 
already in use, causing unpredictable results.

INCLUDE_FILES: dhcpsLib.h
*/

/*
 * WIDE Project DHCP Implementation
 * Copyright (c) 1995 Akihiro Tominaga
 * Copyright (c) 1995 WIDE Project
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided only with the following
 * conditions are satisfied:
 *
 * 1. Both the copyright notice and this permission notice appear in
 *    all copies of the software, derivative works or modified versions,
 *    and any portions thereof, and that both notices appear in
 *    supporting documentation.
 * 2. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by WIDE Project and
 *      its contributors.
 * 3. Neither the name of WIDE Project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE DEVELOPER ``AS IS'' AND WIDE
 * PROJECT DISCLAIMS ANY LIABILITY OF ANY KIND FOR ANY DAMAGES
 * WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE. ALSO, THERE
 * IS NO WARRANTY IMPLIED OR OTHERWISE, NOR IS SUPPORT PROVIDED.
 *
 * Feedback of the results generated from any improvements or
 * extensions made to this software would be much appreciated.
 * Any such feedback should be sent to:
 * 
 *  Akihiro Tominaga
 *  WIDE Project
 *  Keio University, Endo 5322, Kanagawa, Japan
 *  (E-mail: dhcp-dist@wide.ad.jp)
 *
 * WIDE project has the rights to redistribute these changes.
 */

/* includes */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <inetLib.h>
#include <logLib.h>

#include "dhcp/dhcp.h"
#include "dhcp/common.h"
#include "dhcpsLib.h"
#include "dhcp/hash.h"
#include "dhcp/database.h"

/* global variables for both virtual stack and regular stack */

IMPORT long dhcps_dflt_lease; 	/* Default for default lease length. */
IMPORT long dhcps_max_lease; 	/* Default for maximum lease length. */
IMPORT DHCPS_LEASE_DESC *	pDhcpsLeasePool;

#ifndef VIRTUAL_STACK

/* globals */

struct hash_tbl cidhashtable;
struct hash_tbl iphashtable;
struct hash_tbl nmhashtable;
struct hash_tbl relayhashtable;
struct hash_tbl paramhashtable;
struct hash_member *bindlist;
struct hash_member *reslist;

#else
#include "netinet/vsLib.h"
#include "netinet/vsDhcps.h"
#endif /* VIRTUAL_STACK */


IMPORT void dhcpsFreeResource (struct dhcp_resource *);

/*******************************************************************************
*
* dump_bind_db - write all active bindings to permanent storage
*
* This routine takes a snapshot of the linked list which contains the
* descriptors for each active or pending lease. It calls a hook routine, 
* supplied by the user, to record these entries in permanent storage. This 
* storage is required to maintain the consistency of the server data 
* structures and is the key to the integrity of the entire DHCP protocol. 
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*/

void dump_bind_db (void)
    {
    struct hash_member *bindptr = NULL;
    STATUS result;

    if (dhcpsLeaseHookRtn == NULL)
        return;

    result = (* dhcpsLeaseHookRtn) (DHCPS_STORAGE_STOP, NULL, 0);
    if (result != OK) 
         {
         logMsg ("Warning: cannot close the binding database.\n", 
                  0, 0, 0, 0, 0, 0);
         return;
         }

    /* Remove old values. */

    result = (* dhcpsLeaseHookRtn) (DHCPS_STORAGE_CLEAR, NULL, 0);
    if (result != OK) 
         {
         logMsg ("Warning: cannot clear the binding database.\n",
                  0, 0, 0, 0, 0, 0);
         return;
         }

    result = (* dhcpsLeaseHookRtn) (DHCPS_STORAGE_START, NULL, 0);
    if (result != OK) 
         {
         logMsg ("Warning: cannot open the binding database.\n", 
                  0, 0, 0, 0, 0, 0);
         return;
         }

    bindptr = bindlist;
    while (bindptr != NULL) 
        {
        dump_bind_entry(bindptr->data);
        bindptr = bindptr->next;
        }

    return;
    }

/*******************************************************************************
*
* dump_bind_entry - write a binding record to permanent storage
*
* This routine calls a hook routine, supplied by the user, to store the entry 
* for an active lease. The stored entry takes the following form:
*
*    <client ID>:<subnet number>:<H/W address>:<expiration time>:<entry name>
*
* where the entry name identifies the corresponding entry in the address pool.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*/

void dump_bind_entry
    ( 
    struct dhcp_binding *	bp	/* binding record to add to storage */
    )
    {
    char tmp [30];                /* Maximum length of quoted date string. */
    char new_entry [100];         /* 
                                   * Maximum length of entry -
                                   * type/ID, subnet, hardware address,
                                   * expiration time, and entry name. 
                                   */

    /* 
     * Do not store incomplete or static entries. Incomplete entries result
     * when the corresponding address fails an ICMP check, indicating
     * it is in use by an unknown client. A static binding entry is
     * created for each manual lease in the address pool. Since they
     * implicitly provide an infinite lease to a specific client, storage 
     * of their current state is meaningless. 
     */

    if ( (bp->flag & COMPLETE_ENTRY) == 0 || (bp->flag & STATIC_ENTRY) != 0)
        return;

    /* Convert the data structure into a formatted string. */

    sprintf (new_entry, "%s:", cidtos (&bp->cid, 1));
    sprintf (tmp, "%s:", haddrtos (&bp->haddr));
    strcat (new_entry, tmp);

    if (bp->expire_epoch == 0xffffffff) 
        {
        if (bp->flag & BOOTP_ENTRY)
            sprintf(tmp, "\"bootp\":");
        else
            sprintf(tmp, "\"infinity\":");
        }
    else if (bp->expire_epoch == 0)
        sprintf(tmp, "\"\":");
    else 
        {
        sprintf (tmp, "\"%s", (char *)ctime (&bp->expire_epoch));
    
        /* Replace '\n' added by ctime() with the closing quote. */

        tmp [strlen (tmp) - 1] = '"';
        strcat (tmp, ":");            /* Append field delimiter. */
        }
    strcat (new_entry, tmp);

    sprintf (tmp, "%s\n", bp->res_name);
    strcat (new_entry, tmp);

    /* Call the user-supplied storage hook to save the entry. */

    (* dhcpsLeaseHookRtn) (DHCPS_STORAGE_WRITE, new_entry, strlen (new_entry));

    return;
    }

/*******************************************************************************
*
* finish - cleanup and remove data structures before exiting
*
* This "routine" contains the functions needed for an orderly shutdown of 
* the server task. The dump_bind_db() routine stores a snapshot of all active
* leases and dhcpsCleanup() removes the installed packet filter device and all
* dynamically allocated memory. In the original code, this routine was 
* triggered by SIGTERM or SIGINT. This is not a working example, since
* the server task created at system startup is not removed. The code is not
* included in the DHCP release.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*/

/* void finish (void)
    {
    dump_bind_db ();

    dhcpsCleanup (6);

    /@ Instead of exit(), the entry point of the server task should return. @/

    exit (0); 
    }  
*/

/*******************************************************************************
*
* add_bind - record lease offers and active leases
*
* This routine stores the pending offers and active leases in the internal
* linked list. The contents of the list are periodically written to permanent
* storage by the dump_bind_db() routine defined above.
*
* RETURNS: 0 if successful, or -1 on memory allocation error.
*
* ERRNO: N/A
*
* NOMANUAL
*/

int add_bind
    (
    struct dhcp_binding *	bind	/* binding record to store in memory */
    )
    {
    struct hash_member *bindptr;
#ifndef VIRTUAL_STACK
    extern struct hash_member *bindlist;
#endif

    bindptr = (struct hash_member *)calloc (1, sizeof (struct hash_member));
    if (bindptr == NULL) 
        {
#ifdef DHCPS_DEBUG
        logMsg ("Warning: memory allocation error adding binding.\n",
                 0, 0, 0, 0, 0, 0);
#endif
        return (-1);
        }
    bindptr->next = bindlist;
    bindptr->data = (hash_datum *)bind;
    bindlist = bindptr;
    nbind++;

    return (0);
    }

/*******************************************************************************
*
* read_idtype - extract identifier type from input string
*
* This routine extracts the single byte which contains the numeric type from 
* a numeric pair of the form <type>:<value>. It is used when parsing entries 
* from the client identifier field of the address pool and binding databases, 
* and for reading from the hardware address field contained in the binding 
* database. Valid values for the ID type are listed in the "Assigned Numbers" 
* RFC under the ARP section. RFC 1700 defined the following types:
*
*	 Number Hardware Type (hrd)                 
* 	------ -----------------------------------
*	      1 Ethernet (10Mb)
*	      2 Experimental Ethernet (3Mb)
*	      3 Amateur Radio AX.25
*	      4 Proteon ProNET Token Ring
*	      5 Chaos
*	      6 IEEE 802 Networks
*	      7 ARCNET
*	      8 Hyperchannel
*	      9 Lanstar
*	     10 Autonet Short Address
*	     11 LocalTalk
*	     12 LocalNet (IBM PCNet or SYTEK LocalNET)
*	     13 Ultra link
*	     14 SMDS
*	     15 Frame Relay
*	     16 Asynchronous Transmission Mode (ATM)
*	     17 HDLC
*	     18 Fibre Channel
*	     19 Asynchronous Transmission Mode (ATM)
*	     20 Serial Line
*	     21 Asynchronous Transmission Mode (ATM)
*
* RETURNS: 0 if successful, or -1 on parse error.
*
* ERRNO: N/A
*
* NOMANUAL
*/

static int read_idtype
    (
    char **cp, 		/* current location in input string */
    u_char *idtype 	/* pointer to storage for extracted value */
    )
    {
    char tmpstr [MAXSTRINGLEN];
    int j;

    get_string (cp, tmpstr);
 
    if (sscanf (tmpstr, "%d", &j) != 1) 

        {
#ifdef DHCPS_DEBUG
        logMsg ("Warning: Can't extract type value.\n", 0, 0, 0, 0, 0, 0);
#endif
        return (-1);
        }

    *idtype = (u_char) j;

    return (0);
    }

/*******************************************************************************
*
* read_cid - extract identifier value from input string
*
* This routine extracts the client ID value from a numeric pair of the form
* <type>:<value>. It is used when parsing entries from the client identifier 
* field of the address pool and binding databases. The value field is
* represented as a string of characters representing a hexadecimal number,
* preceded by 0x.
*
* RETURNS: 0 if successful, or -1 on parse error.
*
* ERRNO: N/A
*
* NOMANUAL
*/

static int read_cid
    (
    char **cp, 			/* current location in input string */
    struct client_id *cid 	/* pointer to storage for extracted value */
    )
    {
    char tmp [MAXSTRINGLEN];
    int i, j;

    get_string (cp, tmp);
    bzero (cid->id, MAXOPT);

    /* 
     * Determine length of field by ignoring the leading "0x" characters,
     * then dividing for the two characters used for each byte.
     */

    cid->idlen = (strlen (tmp) - 2) / 2; 

    if (cid->idlen == 0)
        return (-1);

    if (cid->idlen > MAXOPT) 
        {
        cid->idlen = MAXOPT;
        logMsg ("Warning: client ID exceeds maximum length. Truncating.\n",
                 0, 0, 0, 0, 0, 0);
        }

    bzero (cid->id, cid->idlen);

    /* Interpret characters in string as sequence of hexadecimal bytes. */

    for (i = 0; i < cid->idlen; i++) 
        {
        if (sscanf (&tmp [i * 2  + 2], "%2x", &j) != 1) 
            {
#ifdef DHCPS_DEBUG
            logMsg ("Warning: can't extract client ID.\n", 0, 0, 0, 0, 0, 0);
#endif
            return (-1);
            }
        cid->id[i] = (char)j;
        }

    return (0);
    }

/*******************************************************************************
*
* read_haddr - extract client hardware address from input string
*
* This routine extracts the hardware address value from a numeric pair of the 
* form <type>:<value>. It is used when parsing entries from the hardware
* address field of the binding database. If preceded by 0x, the string is
* interpreted as a sequence of hexadecimal numbers. Otherwise, it is copied
* directly, without interpretation.
*
* RETURNS: 0 if successful, or -1 on parse error.
*
* ERRNO: N/A
*
* NOMANUAL
*/

static int read_haddr
    (
    char **cp, 			/* current location in input string */
    struct chaddr *haddr 	/* pointer to storage for extracted value */
    )
    {
    char tmp [MAXSTRINGLEN];
    int i, j;

    get_string (cp, tmp);

    /* Interpret string as hexadecimal number if preceded by 0x. */

    if (tmp[0] != '\0' && tmp[0] == '0' && (tmp[1] == 'x' || tmp[1] == 'X')) 
        {
        /*
         * Length consists of number of bytes (two characters each) and does
         * not include the leading 0x. 
         */

        haddr->hlen = (strlen (tmp) - 2) / 2;
        if (haddr->hlen > MAX_HLEN) 
            {
	    haddr->hlen = MAX_HLEN;
	    logMsg ("Hardware address exceeds maximum length. Truncating.\n",
                     0, 0, 0, 0, 0, 0);
            }
        bzero (haddr->haddr, haddr->hlen);

        for (i = 0; i < haddr->hlen; i++) 
            {
            if (sscanf (&tmp [i * 2 + 2], "%2x", &j) != 1) 
                {
#ifdef DHCPS_DEBUG
	        logMsg ("Warning: error extracting hardware address.\n",
                         0, 0, 0, 0, 0, 0);
#endif
	        return (-1);
                }
            haddr->haddr[i] = (char) j;
            }
        } 
    else 
        {
        haddr->hlen = strlen (tmp);
        if (haddr->hlen > MAX_HLEN)
            {
            haddr->hlen = MAX_HLEN;
	    logMsg ("Hardware address exceeds maximum length. Truncating.\n",
                     0, 0, 0, 0, 0, 0);
            }
        bzero (haddr->haddr, haddr->hlen);
        bcopy (tmp, haddr->haddr, haddr->hlen); 
        }
    return (0);
    }

/*******************************************************************************
*
* read_subnet - extract subnet number from input string
*
* This routine extracts the subnet number (in dotted decimal format) from the 
* input string into the provided buffer. It is used when parsing entries from 
* the subnet field of the binding database. If a client has changed subnets, 
* the lease recorded in the binding database will not be renewed.
*
* RETURNS: 0 if successful, or -1 on parse error.
*
* ERRNO: N/A
*
* NOMANUAL
*/

static int read_subnet
    (
    char **cp, 			/* current location in input string */
    struct in_addr *subnet 	/* pointer to storage for extracted value */
    )
    {
    char tmpstr [MAXSTRINGLEN];
    char *pTmp;
    STATUS result;

    get_string (cp, tmpstr);

    pTmp = tmpstr;
    result = get_ip (&pTmp, subnet);
    if (result != OK)
        {
#ifdef DHCPS_DEBUG
        logMsg ("Warning: can't extract subnet number.\n", 
                 0, 0, 0, 0, 0, 0);
#endif
        return (-1);
        }

    return (0);
    }

/*******************************************************************************
*
* read_bind_db - extract offered and active leases from permanent storage
*
* This routine calls a hook routine, supplied by the user, to retrieve the 
* entries for each active or pending lease. It builds the list of these 
* entries and updates the internal data storage to prevent re-assignment of 
* IP addresses already in use. It is called once on server startup.
*
* RETURNS: OK if data update completed, or ERROR otherwise.
*
* ERRNO: N/A
*
* NOMANUAL
*/

STATUS read_bind_db (void)
    {
    char buffer [MAXSTRINGLEN];
    char tmp [MAXSTRINGLEN];
    char *bufptr = NULL;
    unsigned buflen = 0;
    struct dhcp_binding *binding = NULL;
    STATUS result;

    if (dhcpsLeaseHookRtn == NULL)
        return (OK);

    result = (* dhcpsLeaseHookRtn) (DHCPS_STORAGE_START, NULL, 0);
    if (result != OK)
         {
         logMsg ("Warning: cannot open the binding database.\n",
                  0, 0, 0, 0, 0, 0);
         return (ERROR);
         }

  /*
   * Read binding information from entries with the following format:
   *
   *         idtype:id:subnet:htype:haddr:"expire_date":resource_name
   */

    FOREVER
        {
        buflen = sizeof (buffer);

        /* Extract single entry from storage as NULL-terminated string. */

        read_entry (buffer, &buflen);
        if (buflen == 0) 
            break;

        bufptr = buffer;

        if (buffer[0] == '\0')
            return (OK);

        /* Create linked list element. */
    
        binding = (struct dhcp_binding *)
                      calloc (1, sizeof (struct dhcp_binding));
        if (binding == NULL) 
            {
#ifdef DHCPS_DEBUG
            logMsg("Error: Couldn't allocate memory in read_bind_db\n", 
                    0, 0, 0, 0, 0, 0);
#endif
            return (ERROR); 
            }

        /* read client identifier type */

        if (read_idtype (&bufptr, &binding->cid.idtype) != 0)
            {
            free (binding);
            return (ERROR); 
            }

        /* read client identifier value */

        adjust (&bufptr);
        if (read_cid (&bufptr, &binding->cid) != 0)
            {
            free (binding);
            return (ERROR); 
            }

        /* read subnet number of client */

        adjust (&bufptr);
        if (read_subnet (&bufptr, &binding->cid.subnet) != 0)
            {
            free (binding);
            return (ERROR);
            }

        /* read hardware address type (e.g. "1" for 10 MB ethernet). */

        adjust (&bufptr);
        if (read_idtype (&bufptr, &binding->haddr.htype) != 0)
            {
            free (binding);
            return (ERROR);
            }

        /* read client hardware address */

        adjust (&bufptr);
        if (read_haddr (&bufptr, &binding->haddr) != 0)
            {
            free (binding);
            return (ERROR);
            }

        /* read expiration time for lease */

        adjust (&bufptr);

        get_string (&bufptr, tmp);
        if (strcmp (tmp, "infinity") == 0 || strcmp(tmp, "bootp") == 0) 
            binding->expire_epoch = 0xffffffff;
        else
            binding->expire_epoch = strtotime (tmp);

        /* read name of lease descriptor */

        adjust (&bufptr);
        get_string (&bufptr, tmp);
        if (strlen (tmp) > MAX_NAME)
            {
            bcopy (tmp, binding->res_name, MAX_NAME);
            binding->res_name [MAX_NAME] = '\0';
            }
        else
            {
            bcopy (tmp, binding->res_name, strlen (tmp));
            binding->res_name [strlen (tmp)] = '\0';
            }

        /* Find lease descriptor with given name. */

        binding->res = (struct dhcp_resource *) hash_find ( &nmhashtable, 
                                                            binding->res_name, 
                                                    strlen (binding->res_name),
		                                           resnmcmp, 
                                                           binding->res_name);

        if (binding->res == NULL) 
            {
#ifdef DHCPS_DEBUG
            logMsg ("Warning: can't find the resource \"%s\"", 
                     (int)binding->res_name, 0, 0, 0, 0, 0);
#endif

            /* free binding info */

            free (binding);
            continue;
            }

        /* Link lease record to lease descriptor and store in hash table. */

        binding->res->binding = binding;
        binding->flag |= COMPLETE_ENTRY;

        if (hash_ins (&cidhashtable, binding->cid.id, binding->cid.idlen,
		      bindcidcmp, &binding->cid, binding) < 0) 
            {
#ifdef DHCPS_DEBUG
            logMsg ("Error: Couldn't add client identifier to hash table.\n", 
                     0, 0, 0, 0, 0, 0);
#endif
            free (binding);
            return (ERROR); 
            }
  
        /* Add lease record to linked list. */

        if (add_bind (binding) != 0)
            {
            free (binding);
            return (ERROR); 
            }
        }

#ifdef DHCPS_DEBUG
    logMsg ("dhcps: read %d entries from binding and addr-pool database.\n", 
            nbind, 0, 0, 0, 0, 0);
#endif

    return (OK);
    }

/*******************************************************************************
*
* read_relay_db - parse relay agent database
*
* This routine creates the list of available relay agents. The DHCP server
* will accept messages forwarded from other subnets if they are delivered
* by a relay agent contained in the list.
*
* RETURNS: OK if list created, or ERROR otherwise.
*
* ERRNO: N/A
*
* NOMANUAL
*/

void read_relay_db 
    (
    int dbsize 	/* number of relay agents in database */
    )
    {
    char relayIP [INET_ADDR_LEN];
    char subnet_mask [INET_ADDR_LEN];
    int nrelay = 0;
    int loop;
    int result;
    struct relay_acl *acl = NULL;

    /*
     * Read database entries, which contain relay agent's IP address and 
     * subnet number. If it finds a NULL entry for pAddress it will stop
     * processing the table.
     */

    for (loop = 0; (loop < dbsize) &&
	 (pDhcpsRelaySourceTbl [loop].pAddress); loop++)
        {
        sprintf (relayIP, "%s", pDhcpsRelaySourceTbl [loop].pAddress);
        sprintf (subnet_mask, "%s", pDhcpsRelaySourceTbl [loop].pMask);

        acl = (struct relay_acl *)calloc (1, sizeof (struct relay_acl));
        if (acl == NULL) 
            {
#ifdef DHCPS_DEBUG
            logMsg ("Memory allocation error reading relay agent database.\n", 
                     0, 0, 0, 0, 0, 0);
#endif
            break;
            }
   
        acl->relay.s_addr = inet_addr (relayIP);
        acl->subnet_mask.s_addr = inet_addr (subnet_mask);
        if (acl->relay.s_addr == -1 || acl->subnet_mask.s_addr == -1) 
            {
#ifdef DHCPS_DEBUG
            logMsg ("Conversion error reading relay database.\n",
                     0, 0, 0, 0, 0, 0);
#endif
            free (acl);
            continue;
            }

        /* Store entry in hash table, keyed on relay agent IP address. */

        result = hash_ins (&relayhashtable, (char *)&acl->relay, 
                           sizeof (struct in_addr), relayipcmp, 
                           &acl->relay, acl);
        if (result < 0) 
            {
#ifdef DHCPS_DEBUG
            logMsg ("hash insertion failed in read_relay_db()", 
                    0, 0, 0, 0, 0, 0);
#endif
            free (acl);
            break; 
            }

        nrelay++;
        }

#ifdef DHCPS_DEBUG
    logMsg ("dhcps: read %d entries from relay agent database.\n", nrelay, 
             0, 0, 0, 0, 0);
#endif

    return;
    }

/*******************************************************************************
*
* strtotime - parse lease expiration time
*
* This routine converts the lease expiration time stored in the binding
* database into the internal calendar time. That field has the following
* format:
*                "Wed Feb 19 14:17:39 1997"
*
* On generic VxWorks systems, the internal calendar time is the number of 
* seconds since system startup. For correct operation of DHCP, it should be 
* the number of seconds since some absolute base time, such as 00:00:00 GMT, 
* Jan. 1, 1970, which is the base value used on Unix systems. Adding this
* functionality to VxWorks requires BSP modifications. Without this feature, 
* rebooting the server automatically extends all leases. This behavior will 
* reduce protocol efficiency, since the server will not re-assign addresses 
* that (incorrectly) are recorded as still in use, but it will not cause 
* duplicate IP address assignment.
*
* RETURNS: Calendar time, or 0 if not available.
*
* ERRNO: N/A
*
* NOMANUAL
*/

static time_t strtotime
    (
    char *date_str 	/* String containing lease expiration time. */
    )
    {
    struct tm   tmval;
    char week[4];
    char month[4];
    int year = 0; 
    int i = 0;
    struct map 
        {
        char symbol[4];
        int code;
        };
    struct map week_list[7];
    struct map month_list[12];

    if (*date_str == '\0')
        return (0);

    /* Initialize days of week and months of year. */

    week_list[0].code = 0;
    strcpy (week_list[0].symbol, "Sun");
    week_list[1].code = 1;
    strcpy (week_list[1].symbol, "Mon");
    week_list[2].code = 2;
    strcpy (week_list[2].symbol, "Tue");
    week_list[3].code = 3;
    strcpy (week_list[3].symbol, "Wed");
    week_list[4].code = 4;
    strcpy (week_list[4].symbol, "Thu");
    week_list[5].code = 5;
    strcpy (week_list[5].symbol, "Fri");
    week_list[6].code = 6;
    strcpy (week_list[6].symbol, "Sat");

    month_list[0].code = 0;
    strcpy (month_list[0].symbol, "Jan");
    month_list[1].code = 1;
    strcpy (month_list[1].symbol, "Feb");
    month_list[2].code = 2;
    strcpy (month_list[2].symbol, "Mar");
    month_list[3].code = 3;
    strcpy (month_list[3].symbol, "Apr");
    month_list[4].code = 4;
    strcpy (month_list[4].symbol, "May");
    month_list[5].code = 5;
    strcpy (month_list[5].symbol, "Jun");
    month_list[6].code = 6;
    strcpy (month_list[6].symbol, "Jul");
    month_list[7].code = 7;
    strcpy (month_list[7].symbol, "Aug");
    month_list[8].code = 8;
    strcpy (month_list[8].symbol, "Sep");
    month_list[9].code = 9;
    strcpy (month_list[9].symbol, "Oct");
    month_list[10].code = 10;
    strcpy (month_list[10].symbol, "Nov");
    month_list[11].code = 11;
    strcpy (month_list[11].symbol, "Dec");

    bzero ( (char *)&tmval, sizeof (tmval));
    bzero (week, sizeof (week));
    bzero (month, sizeof (month));

    /*
     * Parse date string into separate fields. Day of month, hour, 
     * minute, and second are in format used by mktime() below. 
     */

    sscanf (date_str, "%s %s %u %u:%u:%u %u", week, month, &(tmval.tm_mday),
	              &(tmval.tm_hour), &(tmval.tm_min), &(tmval.tm_sec), 
                      &year);

    /* Adjust entry for base year. */

    tmval.tm_year = year - 1900;

    /* Store value for day of week. (Sunday = 0). */
  
    for (i = 0; i < 7; i++) 
        if (strcmp(week_list[i].symbol, week) == 0)
	    break;
    if (i < 7) 
        tmval.tm_wday = week_list[i].code;
    else 
        return (0);

    /* Store value for month of year. (January = 0, December = 11). */

    for (i = 0; i < 12; i++)
         if (strcmp (month_list[i].symbol, month) == 0)
             break;
    if (i < 12) 
        tmval.tm_mon = month_list[i].code;
    else
         return (0);

    return (mktime (&tmval));
    }

/*******************************************************************************
*
* get_ip - extract numbers in IP address format
*
* This routine extracts IP addresses and subnet masks (in dotted decimal 
* format) from the input string into the provided structure. It is used when 
* retrieving entries from the address pool and binding databases.
*
* RETURNS: OK if successful, or ERROR otherwise.
*
* ERRNO: N/A
*
* NOMANUAL
*/

static STATUS get_ip
    (
    char **src, 		/* current location in input string */
    struct in_addr *target	/* pointer to storage for extracted value */
    )
    {
    if (prs_inaddr (src, &target->s_addr) != 0) 
        return (ERROR);

    return (OK);
    }

/*******************************************************************************
*
* resipcmp - verify IP addresses for address pool entries
*
* This routine compares a hash table key against the IP address of a address 
* pool entry. It is used when storing and retrieving lease descriptors with 
* the internal hash tables.
*
* RETURNS: TRUE if addresses match, or FALSE otherwise.
*
* ERRNO: N/A
*
* NOMANUAL
*/

int resipcmp
    (
    struct in_addr * 		key, 	/* pointer to hash table key */
    struct dhcp_resource * 	rp	/* pointer to lease resource */
    )
    {
    return (key->s_addr == rp->ip_addr.s_addr);
    }

/*******************************************************************************
*
* relayipcmp - verify IP addresses for relay agent entries
*
* This routine compares a hash table key against the IP address of a relay
* agent. It is used when storing and retrieving relay agent entries in the 
* internal hash table.
*
* RETURNS: TRUE if addresses match, or FALSE otherwise.
*
* ERRNO: N/A
*
* NOMANUAL
*/

int relayipcmp
    (
    struct in_addr * 		key, 	/* pointer to hash table key */
    struct relay_acl * 		aclp    /* pointer to relay agent entry */
    )
    {
    return (key->s_addr == aclp->relay.s_addr);
    }

/*******************************************************************************
*
* paramcidcmp - verify client identifiers for additional parameters
*
* This routine compares a hash table key against the client identifier 
* contained in a lease descriptor to verify matching types and values. It 
* also checks the recorded subnet against the actual subnet. It is used to
* verify client information when accessing the parameters hash table.
*
* RETURNS: TRUE if entries OK, or FALSE otherwise.
*
* ERRNO: N/A
*
* NOMANUAL
*/

int paramcidcmp
    (
    struct client_id * 		key, 	/* pointer to hash table key */
    struct dhcp_resource * 	rp	/* pointer to lease descriptor */
    )
    {
    return (rp != NULL && rp->binding != NULL &&
            key->idtype == rp->binding->cid.idtype && 
            key->idlen == rp->binding->cid.idlen &&
            bcmp (key->id, rp->binding->cid.id, key->idlen) == 0);
    }

/*******************************************************************************
*
* bindcidcmp - verify client identifiers for active or offered leases
*
* This routine compares a hash table key against the client identifier 
* contained in a lease record to verify matching types and values. It also 
* checks the recorded subnet against the actual subnet. It is used to verify 
* client information when establishing or renewing leases in response to DHCP
* request messages.
*
* RETURNS: TRUE if entries OK, or FALSE otherwise.
*
* ERRNO: N/A
*
* NOMANUAL
*/

int bindcidcmp
    (
    struct client_id * 		key, 	/* pointer to hash table key */
    struct dhcp_binding * 	bp 	/* pointer to lease record */
    )
    {
    return (bp != NULL &&
            key->subnet.s_addr == bp->cid.subnet.s_addr &&
            key->idtype == bp->cid.idtype && key->idlen == bp->cid.idlen &&
            bcmp (key->id, bp->cid.id, key->idlen) == 0);
    }

/*******************************************************************************
*
* set_default - provide values for mandatory configuration parameters
*
* This routine provides default values for fields in a lease descriptor if
* not specified by the administrator in the corresponding address pool entry. 
* It ensures that every lease descriptor includes a default and maximum lease
* length, and a subnet mask. Additionally, it makes entries which contain a 
* client identifier available for BOOTP clients, and resets the maximum lease 
* for all such entries to infinity.
*
* RETURNS: TRUE if entries OK, or FALSE otherwise.
*
* ERRNO: N/A
*
* NOMANUAL
*/

void set_default
    (
    struct dhcp_resource * 	rp	/* pointer to lease descriptor */
    )
    {
    if (ISCLR (rp->valid, S_DEFAULT_LEASE))
        {
        SETBIT (rp->valid, S_DEFAULT_LEASE);
        rp->default_lease = dhcps_dflt_lease;
        }

    if (ISCLR (rp->valid, S_MAX_LEASE)) 
        {
        SETBIT (rp->valid, S_MAX_LEASE);
        rp->max_lease = dhcps_max_lease;
        }
    if (ISSET (rp->valid, S_CLIENT_ID)) 
        {
        SETBIT (rp->valid, S_ALLOW_BOOTP);
        rp->allow_bootp = TRUE;
        }
    if (ISSET (rp->valid, S_ALLOW_BOOTP) && rp->allow_bootp == TRUE) 
        {
        SETBIT(rp->valid, S_MAX_LEASE);
        rp->max_lease = 0xffffffff;   /* infinity */
        }

    if (ISCLR(rp->valid, S_SUBNET_MASK)) 
        {
        SETBIT (rp->valid, S_SUBNET_MASK);
        default_netmask (&rp->ip_addr, &rp->subnet_mask);
        }
  return;
}

/*******************************************************************************
*
* default_netmask - determine the subnet for an IP address
*
* This routine assigns a netmask for the given IP address based on the IP 
* class value. It is called if no netmask was specified in the corresponding 
* address pool entry. It makes no provision for subnetting. The DHCP server 
* will not issue an address to a client if it is not on the client's subnet. 
* Therefore, if subnetting is used, the netmask for each address pool entry 
* must be specified explicitly.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*/

static void default_netmask
    (
    struct in_addr *ipaddr, 	/* pointer to IP address */
    struct in_addr *dfltnetmask /* pointer to storage for extracted value */
    )
    {
    int top = 0;

    top = ntohl (ipaddr->s_addr) >> 30;

    switch (top) 
        {
        case 0:  /* class A */
	case 1:  /* class A */
            dfltnetmask->s_addr = htonl (0xff000000);
            break;
        case 2:  /* class B */
            dfltnetmask->s_addr = htonl (0xffff0000);
            break;
        case 3:  /* class C */
            dfltnetmask->s_addr = htonl (0xffffff00);
            break;
        default:
            dfltnetmask->s_addr = 0xffffffff;
            break;
        }
    return;
    }

/*******************************************************************************
*
* proc_sname - extract the DHCP server name
*
* This routine sets the TFTP server name field of the lease descriptor to the
* value specified by a "snam" field in the address pool database. The field 
* is cleared when "snam@", specifying deletion, is encountered.
*
* RETURNS: 0, always.
*
* ERRNO: N/A
*
* NOMANUAL
*/

int proc_sname
    (
    int code, 			/* resource code for address pool field */
    int optype, 		/* operation type (addition or deletion) */
    char **symbol, 		/* current location in input string */
    struct dhcp_resource *rp 	/* pointer to lease descriptor */
    )
    {
    char tmpstr [MAXSTRINGLEN];

    if (optype == OP_ADDITION) 
        {
        get_string (symbol, tmpstr);
        if (strlen (tmpstr) > MAX_SNAME)
            {
            bcopy (tmpstr, rp->sname, MAX_SNAME);
            rp->sname [MAX_SNAME] = '\0';
            }
        else
            {
            bcopy (tmpstr, rp->sname, strlen (tmpstr));
            rp->sname [strlen (tmpstr)] = '\0';
            }
        } 
    else 
        rp->sname[0] = '\0';

    return (0);
    }

/*******************************************************************************
*
* proc_file - extract the boot image filename
*
* This routine sets the bootfile name field of the lease descriptor to the
* value specified by a "file" field in the address pool database. The
* field is cleared when "file@", specifying deletion, is encountered.
*
* RETURNS: 0, always.
*
* ERRNO: N/A
*
* NOMANUAL
*/

int proc_file
    (
    int code, 			/* resource code for address pool field */
    int optype, 		/* operation type (addition or deletion) */
    char **symbol, 		/* current location in input string */
    struct dhcp_resource *rp 	/* pointer to lease descriptor */
    )
    {
    char tmpstr [MAXSTRINGLEN];

    if (optype == OP_ADDITION) 
        {
        get_string (symbol, tmpstr);
        if (strlen (tmpstr) > MAX_FILE)
            {
            bcopy (tmpstr, rp->file, MAX_FILE);
            rp->file [MAX_FILE] = '\0';
            }
        else
            {
            bcopy (tmpstr, rp->file, strlen (tmpstr));
            rp->file [strlen (tmpstr)] = '\0';
            }
        } 
    else
        rp->file[0] = '\0';

    return (0);
    }

/*******************************************************************************
*
* proc_clid - handle the client identifier
*
* This routine updates the lease descriptor and internal data structures 
* after extracting a client identifier from "clid" or "pmid" entries in 
* the address pool database. The "clid" symbol specifies a manual lease 
* which will only be issued to a specific client on the appropriate subnet. 
* The "pmid" symbol is included in a lease descriptor specifying additional 
* parameters to be provided to a specific client along with a dynamic lease. 
*
* RETURNS: 0 if update completed, or -1 on error.
*
* ERRNO: N/A
*
* NOMANUAL
*/

int proc_clid
    (
    int code, 			/* resource code for address pool field */
    int optype, 		/* operation type (addition or deletion) */
    char **symbol, 		/* current location in input string */
    struct dhcp_resource *rp 	/* pointer to lease descriptor */
    )
    {
    char bufptr[MAXSTRINGLEN];
    struct in_addr tmpsubnet;
    struct dhcp_binding *binding;
    char *target;
    int result;

    /* For client ID (manual lease), return immediately if no IP address. */

    if (code == S_CLIENT_ID && rp->ip_addr.s_addr == 0)
        return (-1);

    /* For parameter ID, return error if client or class already specified. */
    
    if (ISSET (rp->valid, S_PARAM_ID) || ISSET (rp->valid, S_CLASS_ID))
        return (-1); 
    if (rp->binding != NULL)
        return (-1);

    /* Create a lease record to store the identifier information. */

    binding = (struct dhcp_binding *)calloc (1, sizeof (struct dhcp_binding));
    if (binding == NULL) 
        {
#ifdef DHCPS_DEBUG
        logMsg ("Warning: memory allocation error reading identifier.\n",
                 0, 0, 0, 0, 0, 0);
#endif
        return (-1);
        }

    target = (char *)bufptr;

    get_string (symbol, target);

    if (bufptr[0] == '\0') 
        {
        if (code == S_CLIENT_ID)
            free (binding);
        return (-1);
        }

    /* read client identifier type */

    if (read_idtype (&target, &binding->cid.idtype) != 0)
        {
        free (binding);
        return (-1);
        }

    /* read client identifier value */

    adjust (&target);
    if (read_cid (&target, &binding->cid) != 0)
        {
        free (binding);
        return (-1);
        }

    /* For manual leases, fill all fields in associated lease record. */

    if (code == S_CLIENT_ID)
        {
        /* get subnet number */

        if (rp->subnet_mask.s_addr != 0)
            binding->cid.subnet.s_addr =
                                rp->ip_addr.s_addr & rp->subnet_mask.s_addr;  
        else 
            {
            default_netmask (&rp->ip_addr, &tmpsubnet);
            binding->cid.subnet.s_addr = rp->ip_addr.s_addr & tmpsubnet.s_addr;
            }

        /* hardware address is unknown, so use client identifer */

        binding->haddr.htype = binding->cid.idtype;
        if (binding->cid.idlen > MAX_HLEN)
            {
            binding->haddr.hlen = MAX_HLEN;
            bcopy (binding->cid.id, binding->haddr.haddr, MAX_HLEN);
            }
        else
            {
            binding->haddr.hlen = binding->cid.idlen;
            bcopy (binding->cid.id, binding->haddr.haddr, binding->cid.idlen);
            }

        /* Expiration is meaningless for client-specific leases. */

        binding->expire_epoch = 0xffffffff;

        /* Store name of associated lease descriptor. */

        bcopy (rp->entryname, binding->res_name, strlen (rp->entryname));
        binding->res_name [strlen (rp->entryname)] = '\0';
        }

    /* Link identifier record to lease descriptor. */

    binding->res = rp;
    binding->res->binding = binding;

    /* Add entry for manual lease descriptor to appropriate hash table. */

    if (code == S_CLIENT_ID) 
        {
        binding->flag = (COMPLETE_ENTRY | STATIC_ENTRY);
        result = hash_ins (&cidhashtable, binding->cid.id, binding->cid.idlen,
	                   bindcidcmp, &binding->cid, binding);
        if (result < 0) 
            {
#ifdef DHCPS_DEBUG
            logMsg ("Warning: hash table insertion failed for client ID.\n",
                     0, 0, 0, 0, 0, 0);
#endif
            free (binding);
            return (-1);
            }

        result = add_bind (binding);
        if (result != 0)
            {
            free (binding);
            return (-1);
            }
        }
    else     /* For "pmid", insert into parameters hash table */
        {
        result = hash_ins (&paramhashtable, 
                           binding->cid.id, binding->cid.idlen,
	                   paramcidcmp, &binding->cid, rp);
        if (result < 0) 
            {
#ifdef DHCPS_DEBUG
            logMsg ("Warning: hash table insertion failed for client ID.\n",
                     0, 0, 0, 0, 0, 0);
#endif
            free (binding);
            return (-1);
            }
        }

    return (0);
    }

/*******************************************************************************
*
* proc_class - handle the class identifier
*
* This routine updates the lease descriptor and internal data structures after
* extracting a class identifier from "clas" field in the address pool database.
* The class identifier is included in an address pool entry specifying 
* additional parameters to be provided to any client which is a member of the 
* class.
*
* RETURNS: 0 if update completed, or -1 on error.
*
* ERRNO: N/A
*
* NOMANUAL
*/

int proc_class
    (
    int code, 			/* resource code for address pool field */
    int optype, 		/* operation type (always OP_ADDITION) */
    char **symbol, 		/* current location in input string */
    struct dhcp_resource *rp 	/* pointer to lease descriptor */
    )
    {
    char buffer[MAXSTRINGLEN];
    struct dhcp_binding *binding;
    char *target;
    int result;

    /* Return error if descriptor already has client or class parameters. */

    if (ISSET (rp->valid, S_PARAM_ID) || ISSET (rp->valid, S_CLASS_ID))
        return (-1);
    if (rp->binding != NULL)
        return (-1);

    target = (char *)buffer;

    get_string (symbol, target);

    /* Create a lease record to store the identifier information. */

    binding = (struct dhcp_binding *)calloc (1, sizeof (struct dhcp_binding));
    if (binding == NULL) 
        {
#ifdef DHCPS_DEBUG
        logMsg ("Warning: memory allocation error reading identifier.\n",
                 0, 0, 0, 0, 0, 0);
#endif
        return (-1);
        }

    target = (char *)buffer;

    get_string (symbol, target);

    /*
     * Record class identifier. The type remains 0, which is unused by
     * client identifiers.
     */

    if (strlen (buffer) > MAXOPT - 1)
        {
        binding->cid.idlen = MAXOPT - 1; 
        bcopy (buffer, binding->cid.id, MAXOPT - 1);
        binding->cid.id[MAXOPT - 1] = '\0';
        }
    else
        {
        binding->cid.idlen = strlen (buffer);
        bcopy (buffer, binding->cid.id, strlen (buffer));
        binding->cid.id [strlen (buffer)] = '\0';
        }

    /* Link identifier record to lease descriptor. */

    binding->res = rp;
    binding->res->binding = binding;

    /* Add entry to parameters hash table. */

    result = hash_ins (&paramhashtable, 
                       binding->cid.id, binding->cid.idlen,
                       paramcidcmp, &binding->cid, rp);
    if (result < 0) 
        {
#ifdef DHCPS_DEBUG
        logMsg ("Warning: hash table insertion failed for client ID.\n",
                 0, 0, 0, 0, 0, 0);
#endif
        free (binding);
        return (-1);
        }

    return (0);
    }

/*******************************************************************************
*
* proc_tblc - handle inclusion of address pool entries
*
* This routine processes entry quotation specified by a "tblc" (table
* continuation) entry in the address pool database. After retrieving the
* named descriptor, any values in the existing entry not already specified in
* the database are copied from the quoted entry.
*
* RETURNS: 0 if quotation completed, or -1 if matching entry not found.
*
* ERRNO: N/A
*
* NOMANUAL
*/

int proc_tblc
    (
    int code, 			/* resource code for address pool field */
    int optype, 		/* operation type (always OP_ADDITION) */
    char **symbol, 		/* current location in input string */
    struct dhcp_resource *rp1	/* pointer to lease descriptor */
    )
    {
    struct dhcp_resource *rp2 = NULL;
    char tmpstr [MAXSTRINGLEN];
    char *pTmp;
    int i = 0;

    get_string (symbol, tmpstr);

    pTmp = tmpstr;
    rp2 = (struct dhcp_resource *) hash_find (&nmhashtable, pTmp,
                                              strlen (pTmp), resnmcmp, pTmp);
    if (rp2 == NULL) 
        {
#ifdef DHCPS_DEBUG
        logMsg ("Warning: Can't find table entry \"%s\"\n", (int)tmpstr,
                 0, 0, 0, 0, 0);
#endif
        return (-1);
        }

    if (ISCLR (rp1->valid, S_SNAME)) 
        strcpy (rp1->sname, rp2->sname);
    if (ISCLR (rp1->valid, S_FILE)) 
        strcpy (rp1->file, rp2->file);
    if (ISCLR (rp1->valid, S_SIADDR))
        rp1->siaddr = rp2->siaddr;
    if (ISCLR (rp1->valid, S_ALLOW_BOOTP)) 
        rp1->allow_bootp = rp2->allow_bootp;
    if (ISCLR (rp1->valid, S_IP_ADDR)) 
        rp1->ip_addr = rp2->ip_addr;
    if (ISCLR (rp1->valid, S_MAX_LEASE)) 
        rp1->max_lease = rp2->max_lease;
    if (ISCLR (rp1->valid, S_DEFAULT_LEASE)) 
        rp1->default_lease = rp2->default_lease;
    if (ISCLR (rp1->valid, S_SUBNET_MASK)) 
        rp1->subnet_mask = rp2->subnet_mask;
    if (ISCLR (rp1->valid, S_TIME_OFFSET)) 
        rp1->time_offset = rp2->time_offset;
    if (ISCLR (rp1->valid, S_ROUTER)) 
        {
        rp1->router.addr = calloc (rp2->router.num, sizeof (struct in_addr));
        if (rp1->router.addr != NULL)
            {
            rp1->router.num = rp2->router.num;
            bcopy ((char *)rp2->router.addr, (char *)rp1->router.addr,
                   rp2->router.num * sizeof (struct in_addr));
            }
        }
    if (ISCLR (rp1->valid, S_TIME_SERVER)) 
        {
        rp1->time_server.addr = calloc (rp2->time_server.num, 
                                        sizeof (struct in_addr));
        if (rp1->time_server.addr != NULL)
            {
            rp1->time_server.num = rp2->time_server.num;
            bcopy ((char *)rp2->time_server.addr, (char *)rp1->time_server.addr,
                   rp2->time_server.num * sizeof (struct in_addr));
            }
        }
    if (ISCLR (rp1->valid, S_NAME_SERVER)) 
        {
        rp1->name_server.addr = calloc (rp2->name_server.num, 
                                        sizeof (struct in_addr));
        if (rp1->name_server.addr != NULL)
            {
            rp1->name_server.num = rp2->name_server.num;
            bcopy ((char *)rp2->name_server.addr, (char *)rp1->name_server.addr,
                   rp2->name_server.num * sizeof (struct in_addr));
            }
        }
    if (ISCLR (rp1->valid, S_DNS_SERVER)) 
        {
        rp1->dns_server.addr = calloc (rp2->dns_server.num, 
                                       sizeof (struct in_addr));
        if (rp1->dns_server.addr != NULL)
            {
            rp1->dns_server.num = rp2->dns_server.num;
            bcopy ((char *)rp2->dns_server.addr, (char *)rp1->dns_server.addr,
                   rp2->dns_server.num * sizeof (struct in_addr));
            }
        }
    if (ISCLR (rp1->valid, S_LOG_SERVER)) 
        {
        rp1->log_server.addr = calloc (rp2->log_server.num, 
                                       sizeof (struct in_addr));
        if (rp1->log_server.addr != NULL)
            {
            rp1->log_server.num = rp2->log_server.num;
            bcopy ((char *)rp2->log_server.addr, (char *)rp1->log_server.addr,
                   rp2->log_server.num * sizeof (struct in_addr));
            }
        }
    if (ISCLR (rp1->valid, S_COOKIE_SERVER)) 
        {
        rp1->cookie_server.addr = calloc (rp2->cookie_server.num, 
                                          sizeof (struct in_addr));
        if (rp1->cookie_server.addr != NULL)
            {
            rp1->cookie_server.num = rp2->cookie_server.num;
            bcopy ((char *)rp2->cookie_server.addr, 
                   (char *)rp1->cookie_server.addr,
                   rp2->cookie_server.num * sizeof (struct in_addr));
            }
        }
    if (ISCLR (rp1->valid, S_LPR_SERVER)) 
        {
        rp1->lpr_server.addr = calloc (rp2->lpr_server.num, 
                                       sizeof (struct in_addr));
        if (rp1->lpr_server.addr != NULL)
            {
            rp1->lpr_server.num = rp2->lpr_server.num;
            bcopy ((char *)rp2->lpr_server.addr, (char *)rp1->lpr_server.addr,
                   rp2->lpr_server.num * sizeof (struct in_addr));
            }
        }
    if (ISCLR (rp1->valid, S_IMPRESS_SERVER))
        {
        rp1->impress_server.addr = calloc (rp2->impress_server.num, 
                                           sizeof (struct in_addr));
        if (rp1->impress_server.addr != NULL)
            {
            rp1->impress_server.num = rp2->impress_server.num;
            bcopy ((char *)rp2->impress_server.addr, 
                   (char *)rp1->impress_server.addr,
                   rp2->impress_server.num * sizeof (struct in_addr));
            }
        }
    if (ISCLR (rp1->valid, S_RLS_SERVER)) 
        {
        rp1->rls_server.addr = calloc (rp2->rls_server.num, 
                                       sizeof (struct in_addr));
        if (rp1->rls_server.addr != NULL)
            {
            rp1->rls_server.num = rp2->rls_server.num;
            bcopy ((char *)rp2->rls_server.addr, (char *)rp1->rls_server.addr,
                   rp2->rls_server.num * sizeof (struct in_addr));
            }
        }
    if (ISCLR (rp1->valid, S_HOSTNAME)) 
        strcpy (rp1->hostname, rp2->hostname);
    if (ISCLR (rp1->valid, S_BOOTSIZE)) 
        rp1->bootsize = rp2->bootsize;
    if (ISCLR (rp1->valid, S_MERIT_DUMP))
        strcpy (rp1->merit_dump, rp2->merit_dump);
    if (ISCLR (rp1->valid, S_DNS_DOMAIN)) 
        strcpy (rp1->dns_domain, rp2->dns_domain);
    if (ISCLR (rp1->valid, S_SWAP_SERVER)) 
        rp1->swap_server = rp2->swap_server;
    if (ISCLR (rp1->valid, S_ROOT_PATH)) 
        strcpy (rp1->root_path, rp2->root_path);
    if (ISCLR (rp1->valid, S_EXTENSIONS_PATH))
        strcpy (rp1->extensions_path, rp2->extensions_path);
    if (ISCLR (rp1->valid, S_IP_FORWARD)) 
        rp1->ip_forward = rp2->ip_forward;
    if (ISCLR (rp1->valid, S_NONLOCAL_SRCROUTE))
        rp1->nonlocal_srcroute = rp2->nonlocal_srcroute;
    if (ISCLR (rp1->valid, S_POLICY_FILTER)) 
        {
        rp1->policy_filter.addr1 = calloc (rp2->policy_filter.num, 
                                           sizeof (struct in_addr));
        if (rp1->policy_filter.addr1 != NULL)
            {
            rp1->policy_filter.addr2 = calloc (rp2->policy_filter.num, 
                                               sizeof (struct in_addr));
            if (rp1->policy_filter.addr2 != NULL)
                {
                rp1->policy_filter.num = rp2->policy_filter.num;
                bcopy ((char *)rp2->policy_filter.addr1, 
                       (char *)rp1->policy_filter.addr1,
                       rp2->policy_filter.num * sizeof (struct in_addr));
                bcopy ((char *)rp2->policy_filter.addr1, 
                       (char *)rp1->policy_filter.addr1,
                       rp2->policy_filter.num * sizeof (struct in_addr));
                }
            else 
                free (rp1->policy_filter.addr1);
            }
        }
    if (ISCLR (rp1->valid, S_MAX_DGRAM_SIZE)) 
        rp1->max_dgram_size = rp2->max_dgram_size;
    if (ISCLR (rp1->valid, S_DEFAULT_IP_TTL))
        rp1->default_ip_ttl = rp2->default_ip_ttl;
    if (ISCLR (rp1->valid, S_MTU_AGING_TIMEOUT))
         rp1->mtu_aging_timeout = rp2->mtu_aging_timeout;
    if (ISCLR (rp1->valid, S_MTU_PLATEAU_TABLE))
        {
        rp1->mtu_plateau_table.shorts = calloc (rp2->mtu_plateau_table.num,
                                                sizeof (u_short));
        if (rp1->mtu_plateau_table.shorts != NULL)
            {
            rp1->mtu_plateau_table.num = rp2->mtu_plateau_table.num;
            bcopy ((char *)rp2->mtu_plateau_table.shorts, 
                   (char *)rp1->mtu_plateau_table.shorts,
                   rp2->mtu_plateau_table.num * sizeof (u_short));
            }
        }
    if (ISCLR (rp1->valid, S_IF_MTU)) 
        rp1->intf_mtu = rp2->intf_mtu;
    if (ISCLR (rp1->valid, S_ALL_SUBNET_LOCAL))
        rp1->all_subnet_local = rp2->all_subnet_local;
    if (ISCLR (rp1->valid, S_BRDCAST_ADDR)) 
        rp1->brdcast_addr = rp2->brdcast_addr;
    if (ISCLR (rp1->valid, S_MASK_DISCOVER)) 
        rp1->mask_discover = rp2->mask_discover;
    if (ISCLR (rp1->valid, S_MASK_SUPPLIER)) 
        rp1->mask_supplier = rp2->mask_supplier;
    if (ISCLR (rp1->valid, S_ROUTER_DISCOVER))
        rp1->router_discover = rp2->router_discover;
    if (ISCLR (rp1->valid, S_ROUTER_SOLICIT))
        rp1->router_solicit = rp2->router_solicit;
    if (ISCLR (rp1->valid, S_STATIC_ROUTE)) 
        {
        rp1->static_route.addr1 = calloc (rp2->static_route.num, 
                                          sizeof (struct in_addr));
        if (rp1->static_route.addr1 != NULL)
            {
            rp1->static_route.addr2 = calloc (rp2->static_route.num, 
                                              sizeof (struct in_addr));
            if (rp1->static_route.addr2 != NULL)
                {
                rp1->static_route.num = rp2->static_route.num;
                bcopy ( (char *)rp2->static_route.addr1, 
                        (char *)rp1->static_route.addr1,
                       rp2->static_route.num * sizeof (struct in_addr));
                bcopy ( (char *)rp2->static_route.addr1, 
                        (char *)rp1->static_route.addr1,
                       rp2->static_route.num * sizeof (struct in_addr));
                }
            else 
                free (rp1->static_route.addr1);
            }
        }
    if (ISCLR (rp1->valid, S_TRAILER)) 
        rp1->trailer = rp2->trailer;
    if (ISCLR (rp1->valid, S_ARP_CACHE_TIMEOUT))
        rp1->arp_cache_timeout = rp2->arp_cache_timeout;
    if (ISCLR (rp1->valid, S_ETHER_ENCAP)) 
        rp1->ether_encap = rp2->ether_encap;
    if (ISCLR (rp1->valid, S_DEFAULT_TCP_TTL))
        rp1->default_tcp_ttl = rp2->default_tcp_ttl;
    if (ISCLR (rp1->valid, S_KEEPALIVE_INTER))
        rp1->keepalive_inter = rp2->keepalive_inter;
    if (ISCLR (rp1->valid, S_KEEPALIVE_GARBA))
        rp1->keepalive_garba = rp2->keepalive_garba;
    if (ISCLR (rp1->valid, S_NIS_DOMAIN))
        strcpy (rp1->nis_domain, rp2->nis_domain);
    if (ISCLR (rp1->valid, S_NIS_SERVER)) 
        {
        rp1->nis_server.addr = calloc (rp2->nis_server.num, 
                                       sizeof (struct in_addr));
        if (rp1->nis_server.addr != NULL)
            {
            rp1->nis_server.num = rp2->nis_server.num;
            bcopy ((char *)rp2->nis_server.addr, (char *)rp1->nis_server.addr,
                   rp2->nis_server.num * sizeof (struct in_addr));
            }
        }
    if (ISCLR (rp1->valid, S_NTP_SERVER)) 
        {
        rp1->ntp_server.addr = calloc (rp2->ntp_server.num, 
                                       sizeof (struct in_addr));
        if (rp1->ntp_server.addr != NULL)
            {
            rp1->ntp_server.num = rp2->ntp_server.num;
            bcopy ((char *)rp2->ntp_server.addr, (char *)rp1->ntp_server.addr,
                   rp2->ntp_server.num * sizeof (struct in_addr));
            }
        }
    if (ISCLR (rp1->valid, S_NBN_SERVER)) 
        {
        rp1->nbn_server.addr = calloc (rp2->nbn_server.num, 
                                       sizeof (struct in_addr));
        if (rp1->nbn_server.addr != NULL)
            {
            rp1->nbn_server.num = rp2->nbn_server.num;
            bcopy ((char *)rp2->nbn_server.addr, (char *)rp1->nbn_server.addr,
                   rp2->nbn_server.num * sizeof (struct in_addr));
            }
        }
    if (ISCLR (rp1->valid, S_NBDD_SERVER)) 
        {
        rp1->nbdd_server.addr = calloc (rp2->nbdd_server.num, 
                                        sizeof (struct in_addr));
        if (rp1->nbdd_server.addr != NULL)
            {
            rp1->nbdd_server.num = rp2->nbdd_server.num;
            bcopy ((char *)rp2->nbdd_server.addr, (char *)rp1->nbdd_server.addr,
                   rp2->nbdd_server.num * sizeof (struct in_addr));
            }
        }
    if (ISCLR (rp1->valid, S_NB_NODETYPE)) 
        rp1->nb_nodetype = rp2->nb_nodetype;
    if (ISCLR (rp1->valid, S_NB_SCOPE)) 
        strcpy (rp1->nb_scope, rp2->nb_scope);
    if (ISCLR (rp1->valid, S_XFONT_SERVER)) 
        {
        rp1->xfont_server.addr = calloc (rp2->xfont_server.num, 
                                         sizeof (struct in_addr));
        if (rp1->xfont_server.addr != NULL)
            {
            rp1->xfont_server.num = rp2->xfont_server.num;
            bcopy ((char *)rp2->xfont_server.addr, 
                   (char *)rp1->xfont_server.addr,
                   rp2->xfont_server.num * sizeof (struct in_addr));
            }
        }
    if (ISCLR (rp1->valid, S_XDISPLAY_MANAGER))
        {
        rp1->xdisplay_manager.addr = calloc (rp2->xdisplay_manager.num, 
                                             sizeof (struct in_addr));
        if (rp1->xdisplay_manager.addr != NULL)
            {
            rp1->xdisplay_manager.num = rp2->xdisplay_manager.num;
            bcopy ((char *)rp2->xdisplay_manager.addr, 
                   (char *)rp1->xdisplay_manager.addr,
                   rp2->xdisplay_manager.num * sizeof (struct in_addr));
            }
        }
    if (ISCLR (rp1->valid, S_DHCP_T1)) 
        rp1->dhcp_t1 = rp2->dhcp_t1;
    if (ISCLR (rp1->valid, S_DHCP_T2)) 
        rp1->dhcp_t2 = rp2->dhcp_t2;
    if (ISCLR (rp1->valid, S_NISP_DOMAIN)) 
        strcpy (rp1->nisp_domain, rp2->nisp_domain);
    if (ISCLR (rp1->valid, S_NISP_SERVER)) 
        {
        rp1->nisp_server.addr = calloc (rp2->nisp_server.num, 
                                        sizeof (struct in_addr));
        if (rp1->nisp_server.addr != NULL)
            {
            rp1->nisp_server.num = rp2->nisp_server.num;
            bcopy ((char *)rp2->nisp_server.addr, (char *)rp1->nisp_server.addr,
                   rp2->nisp_server.num * sizeof (struct in_addr));
            }
        }
    if (ISCLR (rp1->valid, S_MOBILEIP_HA)) 
        {
        rp1->mobileip_ha.addr = calloc (rp2->mobileip_ha.num, 
                                        sizeof (struct in_addr));
        if (rp1->mobileip_ha.addr != NULL)
            {
            rp1->mobileip_ha.num = rp2->mobileip_ha.num;
            bcopy ((char *)rp2->mobileip_ha.addr, (char *)rp1->mobileip_ha.addr,
                   rp2->mobileip_ha.num * sizeof (struct in_addr));
            }
        }
    if (ISCLR (rp1->valid, S_SMTP_SERVER)) 
        {
        rp1->smtp_server.addr = calloc (rp2->smtp_server.num, 
                                        sizeof (struct in_addr));
        if (rp1->smtp_server.addr != NULL)
            {
            rp1->smtp_server.num = rp2->smtp_server.num;
            bcopy ((char *)rp2->smtp_server.addr, (char *)rp1->smtp_server.addr,
                   rp2->smtp_server.num * sizeof (struct in_addr));
            }
        }
    if (ISCLR (rp1->valid, S_POP3_SERVER)) 
        {
        rp1->pop3_server.addr = calloc (rp2->pop3_server.num, 
                                        sizeof (struct in_addr));
        if (rp1->pop3_server.addr != NULL)
            {
            rp1->pop3_server.num = rp2->pop3_server.num;
            bcopy ((char *)rp2->pop3_server.addr, (char *)rp1->pop3_server.addr,
                   rp2->pop3_server.num * sizeof (struct in_addr));
            }
        }
    if (ISCLR (rp1->valid, S_NNTP_SERVER)) 
        {
        rp1->nntp_server.addr = calloc (rp2->nntp_server.num, 
                                        sizeof (struct in_addr));
        if (rp1->nntp_server.addr != NULL)
            {
            rp1->nntp_server.num = rp2->nntp_server.num;
            bcopy ((char *)rp2->nntp_server.addr, (char *)rp1->nntp_server.addr,
                   rp2->nntp_server.num * sizeof (struct in_addr));
            }
        }
    if (ISCLR (rp1->valid, S_DFLT_WWW_SERVER))
        {
        rp1->dflt_www_server.addr = calloc (rp2->dflt_www_server.num, 
                                            sizeof (struct in_addr));
        if (rp1->dflt_www_server.addr != NULL)
            {
            rp1->dflt_www_server.num = rp2->dflt_www_server.num;
            bcopy ((char *)rp2->dflt_www_server.addr, 
                   (char *)rp1->dflt_www_server.addr,
                   rp2->dflt_www_server.num * sizeof (struct in_addr));
            }
        }
    if (ISCLR (rp1->valid, S_DFLT_FINGER_SERVER))
        {
        rp1->dflt_finger_server.addr = calloc (rp2->dflt_finger_server.num, 
                                               sizeof (struct in_addr));
        if (rp1->dflt_finger_server.addr != NULL)
            {
            rp1->dflt_finger_server.num = rp2->dflt_finger_server.num;
            bcopy ((char *)rp2->dflt_finger_server.addr, 
                   (char *)rp1->dflt_finger_server.addr,
                   rp2->dflt_finger_server.num * sizeof (struct in_addr));
            }
        }
    if (ISCLR (rp1->valid, S_DFLT_IRC_SERVER))
        {
        rp1->dflt_irc_server.addr = calloc (rp2->dflt_irc_server.num, 
                                            sizeof (struct in_addr));
        if (rp1->dflt_irc_server.addr != NULL)
            {
            rp1->dflt_irc_server.num = rp2->dflt_irc_server.num;
            bcopy ((char *)rp2->dflt_irc_server.addr, 
                   (char *)rp1->dflt_irc_server.addr,
                   rp2->dflt_irc_server.num * sizeof (struct in_addr));
            }
        }
    if (ISCLR (rp1->valid, S_STREETTALK_SERVER))
        {
        rp1->streettalk_server.addr = calloc (rp2->streettalk_server.num, 
                                              sizeof (struct in_addr));
        if (rp1->streettalk_server.addr != NULL)
            {
            rp1->streettalk_server.num = rp2->streettalk_server.num;
            bcopy ((char *)rp2->streettalk_server.addr, 
                   (char *)rp1->streettalk_server.addr,
                   rp2->streettalk_server.num * sizeof (struct in_addr));
            }
        }
    if (ISCLR (rp1->valid, S_STDA_SERVER)) 
        {
        rp1->stda_server.addr = calloc (rp2->stda_server.num, 
                                        sizeof (struct in_addr));
        if (rp1->stda_server.addr != NULL)
            {
            rp1->stda_server.num = rp2->stda_server.num;
            bcopy ((char *)rp2->stda_server.addr, (char *)rp1->stda_server.addr,
                   rp2->stda_server.num * sizeof (struct in_addr));
            }
        }

    for (i = 0; i < VALIDSIZE; i++) 
        {
        /* 
         * Set active and valid flags for all values provided by quoted
         * entry. The valid flag for an entry is always set if the 
         * corresponding active flag is set.
         */

        rp1->active[i] |= (~rp1->valid[i] & rp2->active[i]);
        rp1->valid[i] |= rp2->valid[i];
        }

    return(0);
    }

/*******************************************************************************
*
* proc_mtpt - extract the MTU plateau table values
*
* This routine sets the mtu_plateau_table field of a lease descriptor to the 
* values specified by a "mtpt" entry in the address pool database. The
* structure field is cleared when "mtpt@", specifying deletion, is encountered.
*
* RETURNS: 0, always.
*
* ERRNO: N/A
*
* NOMANUAL
*/

int proc_mtpt
    (
    int code, 			/* resource code for address pool field */
    int optype, 		/* operation type (addition or deletion) */
    char **symbol, 		/* current location in input string */
    struct dhcp_resource *rp 	/* pointer to lease descriptor */
    )
    {
    int i = 0;
    unsigned short tmpnum [MAX_MTUPLTSZ];

    if (optype == OP_ADDITION) 
        {
        for (i = 0; i <= MAX_MTUPLTSZ; i++) 
            {
            while (**symbol && isspace ( (int) **symbol)) 
                {
	        (*symbol)++;
                }
            if (! **symbol) 
                {                        /* Quit if nothing more */
	        break;
                }
            if (isdigit ( (int) **symbol))
                tmpnum[i] = (u_short)get_integer (symbol);
            else 
                break;
            }

        if (i == 0) 
            {
#ifdef DHCPS_DEBUG
            logMsg ("Warning: invalid MTU plateau table value.\n", 
                     0, 0, 0, 0, 0, 0);
#endif
            return (-1);
            }

        rp->mtu_plateau_table.num = i;
        if (rp->mtu_plateau_table.shorts != NULL)
            free (rp->mtu_plateau_table.shorts);
        rp->mtu_plateau_table.shorts = (u_short *)calloc (i, sizeof (u_short));
        if (rp->mtu_plateau_table.shorts == NULL) 
            {
#ifdef DHCPS_DEBUG
            logMsg ("Warning: memory allocation error reading MTU plateau.\n",
                     0, 0, 0, 0, 0, 0);
#endif
            return (-1);
            }

        for (i = 0; i < rp->mtu_plateau_table.num; i++) 
            rp->mtu_plateau_table.shorts[i] = htons (tmpnum[i]);
        } 
    else 
        {
        rp->mtu_plateau_table.num = 0;
        if (rp->mtu_plateau_table.shorts != NULL)
            free (rp->mtu_plateau_table.shorts);
        rp->mtu_plateau_table.shorts = NULL;
        }

    return (0);
    }

/*******************************************************************************
*
* proc_ip - process single IP addresses
*
* This routine sets the fields of the lease descriptor containing single IP 
* address values. It is called in response to any "siad", "ipad", "snmk", 
* "swsv", "brda", or "rtsl" entries in the address pool database. When an "@" 
* mark is appended to any of the above specifiers, the corresponding field is 
* cleared.
*
* RETURNS: 0 if processing completed, or -1 on parse error.
*
* ERRNO: N/A
*
* NOMANUAL
*/

int proc_ip
    (
    int code, 			/* resource code for address pool field */
    int optype, 		/* operation type (addition or deletion) */
    char **symbol, 		/* current location in input string */
    struct dhcp_resource *rp 	/* pointer to lease descriptor */
    )
    {
    struct in_addr *ipp = NULL;
    STATUS result;

    switch (code) 
        {
        case S_SIADDR:
            ipp = &rp->siaddr;
            break;
        case S_IP_ADDR:
            ipp = &rp->ip_addr;
            break;
        case S_SUBNET_MASK:
            ipp = &rp->subnet_mask;
            break;
        case S_SWAP_SERVER:
            ipp = &rp->swap_server;
            break;
        case S_BRDCAST_ADDR:
            ipp = &rp->brdcast_addr;
            break;
        case S_ROUTER_SOLICIT:
            ipp = &rp->router_solicit;
            break;
        }

    if (optype == OP_ADDITION) 
        {
        result = get_ip (symbol, ipp);
        if (result != OK)
            {
#ifdef DHCPS_DEBUG
            logMsg ("Warning: Invalid or missing IP address.\n",
                     0, 0, 0, 0, 0, 0);
#endif
            return (-1);
            }
        } 
    else 
        bzero ( (char *)ipp, sizeof (struct in_addr));
  
    return (0);
    }

/*******************************************************************************
*
* proc_ips - process multiple IP addresses
*
* This routine sets the fields of the lease descriptor containing lists of 
* IP addresses. These entries are indicated by names such as "rout" or "tmsv" 
* in the address pool database. When an "@" mark is appended to any of the 
* corresponding specifiers, the matching field is cleared.
*
* RETURNS: 0 if processing completed, or -1 on error.
*
* ERRNO: N/A
*
* NOMANUAL
*/

int proc_ips
    (
    int code, 			/* resource code for address pool field */
    int optype, 		/* operation type (addition or deletion) */
    char **symbol, 		/* current location in input string */
    struct dhcp_resource *rp 	/* pointer to lease descriptor */
    )
    {
    struct in_addrs *ipp = NULL;
    int i = 0;
    struct in_addr tmpaddr[MAX_IPS];
    STATUS result;

    switch (code) 
        {
        case S_ROUTER:
            ipp = &rp->router;
            break;
        case S_TIME_SERVER:
            ipp = &rp->time_server;
            break;
        case S_NAME_SERVER:
            ipp = &rp->name_server;
            break;
        case S_DNS_SERVER:
            ipp = &rp->dns_server;
            break;
        case S_LOG_SERVER:
            ipp = &rp->log_server;
            break;
        case S_COOKIE_SERVER:
            ipp = &rp->cookie_server;
            break;
        case S_LPR_SERVER:
            ipp = &rp->lpr_server;
            break;
        case S_IMPRESS_SERVER:
            ipp = &rp->impress_server;
            break;
        case S_RLS_SERVER:
            ipp = &rp->rls_server;
            break;
        case S_NIS_SERVER:
            ipp = &rp->nis_server;
            break;
        case S_NTP_SERVER:
            ipp = &rp->ntp_server;
            break;
        case S_NBN_SERVER:
            ipp = &rp->nbn_server;
            break;
        case S_NBDD_SERVER:
            ipp = &rp->nbdd_server;
            break;
        case S_XFONT_SERVER:
            ipp = &rp->xfont_server;
            break;
        case S_XDISPLAY_MANAGER:
            ipp = &rp->xdisplay_manager;
            break;
        case S_NISP_SERVER:
            ipp = &rp->nisp_server;
            break;
        case S_MOBILEIP_HA:
            ipp = &rp->mobileip_ha;
            break;
        case S_SMTP_SERVER:
            ipp = &rp->smtp_server;
            break;
        case S_POP3_SERVER:
            ipp = &rp->pop3_server;
            break;
        case S_NNTP_SERVER:
            ipp = &rp->nntp_server;
            break;
        case S_DFLT_WWW_SERVER:
            ipp = &rp->dflt_www_server;
            break;
        case S_DFLT_FINGER_SERVER:
            ipp = &rp->dflt_finger_server;
            break;
        case S_DFLT_IRC_SERVER:
            ipp = &rp->dflt_irc_server;
            break;
        case S_STREETTALK_SERVER:
            ipp = &rp->streettalk_server;
            break;
        case S_STDA_SERVER:
            ipp = &rp->stda_server;
            break;
        }

    if (optype == OP_ADDITION) 
        {
        for (i = 0; i <= MAX_IPS; i++) 
             {
             while (**symbol && isspace( (int) **symbol)) 
                 {
	         (*symbol)++;
                 }
             if (! **symbol)
	         break;
             result = get_ip (symbol, &tmpaddr[i]);
             if (result != OK)
                 break;
             }

        if (i == 0) 
            {
#ifdef DHCPS_DEBUG
            logMsg ("Warning: Invalid or missing IP address.\n", 
                     0, 0, 0, 0, 0, 0);
#endif
            return (-1);
            }

        ipp->num = i;
        ipp->addr = (struct in_addr *)calloc (i, sizeof (struct in_addr));
        if (ipp->addr == NULL) 
            {
#ifdef DHCPS_DEBUG
            logMsg ("Warning: Memory allocation error reading IP addresses.\n",
                     0, 0, 0, 0, 0, 0);
#endif
            return (-1);
            }

        for (i = 0; i < ipp->num; i++) 
            ipp->addr[i] = tmpaddr[i];
        } 
    else
        {
        ipp->num = 0;
        if (ipp->addr != NULL)
            free (ipp->addr);
        ipp->addr = NULL;
        }
    return (0);
    }

/*******************************************************************************
*
* proc_ippairs - process multiple IP address pairs
*
* This routine sets the fields of the lease descriptor containing one or more 
* IP pairs. It is called in response to any "plcy" or "strt", entries in the 
* address pool database.  When an "@" mark is appended to either of the above
* specifiers, the corresponding field is cleared.
*
* RETURNS: 0 if processing completed, or -1 on error.
*
* ERRNO: N/A
*
* NOMANUAL
*/

int proc_ippairs
    (
    int code, 			/* resource code for address pool field */
    int optype, 		/* operation type (addition or deletion) */
    char **symbol, 		/* current location in input string */
    struct dhcp_resource *rp 	/* pointer to lease descriptor */
    )
    {
    struct ip_pairs *ipp = NULL;
    int i = 0;
    struct in_addr tmpaddr1 [MAX_IPPAIR], tmpaddr2 [MAX_IPPAIR];
    STATUS result;
 
    switch (code) 
        {
        case S_POLICY_FILTER:
            ipp = &rp->policy_filter;
            break;
        case S_STATIC_ROUTE:
            ipp = &rp->static_route;
            break;
        }

    if (optype == OP_ADDITION)
        {
        for (i = 0; i <= MAX_IPPAIR; i++) 
            {
            /* Skip whitespace before first IP address. */

            while (**symbol && isspace ( (int)**symbol)) 
                   (*symbol)++;
            if (! **symbol)
                break;

            result = get_ip(symbol, &tmpaddr1[i]);
            if (result != OK)
	        break;

            /* Skip whitespace before second IP address. */

            while (**symbol && isspace ( (int) **symbol))
	       (*symbol)++;
            if (! **symbol)
                {
#ifdef DHCPS_DEBUG
	        logMsg ("Warning: Second IP address in pair is missing.\n",
                         0, 0, 0, 0, 0, 0);
#endif
	        break;
                }
            result = get_ip(symbol, &tmpaddr2[i]);
            if (result != OK) 
                {
#ifdef DHCPS_DEBUG
	        logMsg ("Warning: Second IP address invalid or missing.\n",
                         0, 0, 0, 0, 0, 0);
#endif
	        break;
                }
            }

        if (i == 0) 
            {
#ifdef DHCPS_DEBUG
            logMsg ("Warning: No valid IP addresses found.\n", 
                     0, 0, 0, 0, 0, 0);
#endif
            return (-1);
            }

        ipp->num = i;
        ipp->addr1 = (struct in_addr *)calloc (i, sizeof (struct in_addr));
        if (ipp->addr1 == NULL) 
            {
#ifdef DHCPS_DEBUG
            logMsg ("Warning: Memory allocation error reading IP addresses.\n",
                     0, 0, 0, 0, 0, 0);
#endif
            return (-1);
            }
        ipp->addr2 = (struct in_addr *)calloc (i, sizeof (struct in_addr));
        if (ipp->addr2 == NULL) 
            {
#ifdef DHCPS_DEBUG
            logMsg ("Warning: Memory allocation error reading IP addresses.\n",
                     0, 0, 0, 0, 0, 0);
#endif
            free (ipp->addr1);
            return (-1);
            }
        for (i = 0; i < ipp->num; i++) 
            {
            ipp->addr1[i] = tmpaddr1[i];
            ipp->addr2[i] = tmpaddr2[i];
            }
        } 
    else 
        {
        ipp->num = 0;
        if (ipp->addr1 != NULL)
            free (ipp->addr1);
        if (ipp->addr2 != NULL)
            free (ipp->addr2);
        }

    return (0);
    }

/*******************************************************************************
*
* proc_hl - store long (4 byte) numeric values in host byte order
*
* This routine sets the fields of the lease descriptor containing four byte 
* numeric entries which have no corresponding fields in the DHCP message 
* structure. These entries are indicated by "maxl" or "dfll" in the address 
* pool database, and are stored in host byte order. When an "@" mark is 
* appended to either of the corresponding specifiers, the matching field is 
* cleared.
*
* RETURNS: 0, always.
*
* ERRNO: N/A
*
* NOMANUAL
*/

int proc_hl
    (
    int code, 			/* resource code for address pool field */
    int optype, 		/* operation type (addition or deletion) */
    char **symbol, 		/* current location in input string */
    struct dhcp_resource *rp 	/* pointer to lease descriptor */
    )
    {
    u_long *lp = NULL;

    switch (code) 
        {
        case S_MAX_LEASE:
            lp = &rp->max_lease;
            break;
        case S_DEFAULT_LEASE:
            lp = &rp->default_lease;
            break;
        }

    if (optype == OP_ADDITION) 
        *lp = get_integer(symbol);
    else
        *lp = 0;

    return (0);
    }

/*******************************************************************************
*
* proc_hs - store short (2 byte) numeric values in host byte order
*
* This routine sets the fields of the lease descriptor containing two byte 
* numeric entries which have no corresponding fields in the DHCP message 
* structure. These  entries are indicated by "dht1" or "dht2" in the address 
* pool database, and are stored in host byte order. When an "@" mark is 
* appended to either of the corresponding specifiers, the matching field 
* is cleared.
*
* RETURNS: 0, always.
*
* ERRNO: N/A
*
* NOMANUAL
*/

int proc_hs
    (
    int code, 			/* resource code for address pool field */
    int optype, 		/* operation type (addition or deletion) */
    char **symbol, 		/* current location in input string */
    struct dhcp_resource *rp 	/* pointer to lease descriptor */
    )
    {
    u_short *sp = NULL;

    switch (code) 
        {
        case S_DHCP_T1:
            sp = &rp->dhcp_t1;
            break;
        case S_DHCP_T2:
            sp = &rp->dhcp_t2;
            break;
        }

    if (optype == OP_ADDITION) 
        *sp = (short)get_integer (symbol);
    else 
        *sp = 0;
    return (0); 
    }

/*******************************************************************************
*
* proc_nl - store long (4 byte) numeric values in network byte order
*
* This routine sets the fields of the lease descriptor containing four byte 
* numeric entries which are sent directly to the client. These entries are 
* indicated by "maxl" or "dfll" in the address pool database, and are stored 
* in network byte order. When an "@" mark is appended to either of the 
* corresponding specifiers, the matching field is cleared.
*
* RETURNS: 0, always.
*
* ERRNO: N/A
*
* NOMANUAL
*/

int proc_nl
    (
    int code, 			/* resource code for address pool field */
    int optype, 		/* operation type (addition or deletion) */
    char **symbol, 		/* current location in input string */
    struct dhcp_resource *rp 	/* pointer to lease descriptor */
    )
    {
    u_long *lp = NULL;
    u_long tmp;

    switch (code) 
        {
        case S_TIME_OFFSET:
            lp = (u_long *) &rp->time_offset;
            break;
        case S_MTU_AGING_TIMEOUT:
            lp = &rp->mtu_aging_timeout;
            break;
        case S_ARP_CACHE_TIMEOUT:
            lp = &rp->arp_cache_timeout;
            break;
        case S_KEEPALIVE_INTER:
            lp = &rp->keepalive_inter;
            break;
        }

    if (optype == OP_ADDITION)
        {
        tmp = get_integer (symbol);
        *lp = htonl (tmp);
        }
    else 
        *lp = 0;

    return (0);
    }

/*******************************************************************************
*
* proc_ns - store short (2 byte) numeric values in network byte order
*
* This routine sets the fields of the lease descriptor containing two byte 
* numeric entries which are sent directly to the client. These entries are 
* indicated by "btsz", "mdgs", or "ifmt" in the address pool database, and 
* are stored in network byte order. When an "@" mark is appended to any of 
* the corresponding specifiers, the matching field is cleared.
*
* RETURNS: 0, always.
*
* ERRNO: N/A
*
* NOMANUAL
*/

int proc_ns
    (
    int code, 			/* resource code for address pool field */
    int optype, 		/* operation type (addition or deletion) */
    char **symbol, 		/* current location in input string */
    struct dhcp_resource *rp 	/* pointer to lease descriptor */
    )
    {
    u_short *sp = NULL;
    u_short tmp;

    switch (code) 
        {
        case S_BOOTSIZE:
            sp = &rp->bootsize;
            break;
        case S_MAX_DGRAM_SIZE:
            sp = &rp->max_dgram_size;
            break;
        case S_IF_MTU:
            sp = &rp->intf_mtu;
            break;
        }

    if (optype == OP_ADDITION)
        {
        tmp = (short) get_integer (symbol);
        *sp = htons (tmp);
        }
    else
        *sp = 0;

    return (0);
    }

/*******************************************************************************
*
* proc_octet - store 1 byte numeric values
*
* This routine sets the fields of the lease descriptor containing single byte 
* numeric entries. These entries are indicated by "ditl", "dttl", or "nbnt" 
* in the address pool database. When an "@" mark is appended to any of the 
* corresponding specifiers, the matching field is cleared.
*
* RETURNS: 0, always.
*
* ERRNO: N/A
*
* NOMANUAL 
*/

int proc_octet
    (
    int code, 			/* resource code for address pool field */
    int optype, 		/* operation type (addition or deletion) */
    char **symbol, 		/* current location in input string */
    struct dhcp_resource *rp 	/* pointer to lease descriptor */
    )
    {
    u_char *cp = NULL;

    switch (code) 
        {
        case S_DEFAULT_IP_TTL:
            cp = &rp->default_ip_ttl;
            break;
        case S_DEFAULT_TCP_TTL:
            cp = &rp->default_tcp_ttl;
            break;
        case S_NB_NODETYPE:
            cp = &rp->nb_nodetype;
            break;
        }

    if (optype == OP_ADDITION)
        *cp = (u_char) get_integer(symbol);
    else
        *cp = 0;

    return (0);
    }

/*******************************************************************************
*
* proc_str - store string values
*
* This routine sets the fields of the lease descriptor containing string 
* entries. These entries are indicated by identifiers such as "hstn" and 
* "mdmp" in the address pool database. When an "@" mark is appended to any 
* of the corresponding specifiers, the matching field is cleared.
*
* RETURNS: 0, always.
*
* ERRNO: N/A
*
* NOMANUAL
*/

int proc_str
    (
    int code, 			/* resource code for address pool field */
    int optype, 		/* operation type (addition or deletion) */
    char **symbol, 		/* current location in input string */
    struct dhcp_resource *rp 	/* pointer to lease descriptor */
    )
    {
    char *cp = NULL;
    char tmpstr [MAXSTRINGLEN];

    switch (code) 
        {
        case S_HOSTNAME:
            cp = rp->hostname;
            break;
        case S_MERIT_DUMP:
            cp = rp->merit_dump;
            break;
        case S_DNS_DOMAIN:
            cp = rp->dns_domain;
            break;
        case S_ROOT_PATH:
            cp = rp->root_path;
            break;
        case S_EXTENSIONS_PATH:
            cp = rp->extensions_path;
            break;
        case S_NIS_DOMAIN:
            cp = rp->nis_domain;
            break;
        case S_NB_SCOPE:
            cp = rp->nb_scope;
            break;
        case S_NISP_DOMAIN:
            cp = rp->nisp_domain;
            break;
        }

    if (optype == OP_ADDITION) 
        {
        get_string (symbol, tmpstr);
        if (strlen (tmpstr) > MAXOPT)
            {
            bcopy (tmpstr, cp, MAXOPT);
            cp [MAXOPT] = '\0';
            }
        else
            {
            bcopy (tmpstr, cp, strlen (tmpstr));
            rp->sname [strlen (tmpstr)] = '\0';
            }
        }
    else 
        cp[0] = '\0';

    return (0);
    }

/*******************************************************************************
*
* proc_bool - process boolean values
*
* This routine sets the fields of the lease descriptor containing boolean 
* entries. These entries are indicated by identifiers such as "albp"
* and "ipfd" in the address pool database. When an "@" mark is appended
* to any of the corresponding specifiers, the matching field is cleared.
*
* RETURNS: 0, always.
*
* ERRNO: N/A
*
* NOMANUAL
*/

int proc_bool
    (
    int code, 			/* resource code for address pool field */
    int optype, 		/* operation type (addition or deletion) */
    char **symbol, 		/* current location in input string */
    struct dhcp_resource *rp 	/* pointer to lease descriptor */
    )
    {
    char tmpstr [MAXSTRINGLEN];
    char *cp = NULL;

    switch (code) 
        {
        case S_ALLOW_BOOTP:
            cp = &rp->allow_bootp;
            break;
        case S_IP_FORWARD:
            cp = &rp->ip_forward;
            break;
        case S_NONLOCAL_SRCROUTE:
            cp = &rp->nonlocal_srcroute;
            break;
        case S_ALL_SUBNET_LOCAL:
            cp = &rp->all_subnet_local;
            break;
        case S_MASK_DISCOVER:
            cp = &rp->mask_discover;
            break;
        case S_MASK_SUPPLIER:
            cp = &rp->mask_supplier;
            break;
        case S_ROUTER_DISCOVER:
            cp = &rp->router_discover;
            break;
        case S_TRAILER:
            cp = &rp->trailer;
            break;
        case S_ETHER_ENCAP:
            cp = &rp->ether_encap;
            break;
        case S_KEEPALIVE_GARBA:
            cp = &rp->keepalive_garba;
            break;
        }

    if (optype == OP_ADDITION) 
        {
        get_string (symbol, tmpstr);
        if (strcmp (tmpstr, "true") == 0)
            *cp = TRUE;
        else if (strcmp (tmpstr, "false") == 0)
            *cp = FALSE;
        else 
            {
#ifdef DHCPS_DEBUG
            logMsg("Warning: Invalid boolean value.\n", 0, 0, 0, 0, 0, 0);
#endif
            *cp = FALSE;
            return (-1);
            }
        }
    else 
        *cp = 0;

    return (0);
    }

/*******************************************************************************
*
* read_addrpool_file - extract runtime address pool entries
*
* This routine calls an optional hook routine, supplied by the user. It
* retrieves any address pool entries added with dhcpsLeaseEntryAdd() after 
* DHCP server startup from permanent storage. If no storage hook was provided,
* any entries in the binding database referencing the runtime address entries 
* will be discarded. The corresponding leases will not be renewed.
*
* This routine is called once on server startup.
*
* RETURNS: OK if data update completed, or ERROR otherwise.
*
* ERRNO: N/A
*
* NOMANUAL
*/

STATUS read_addrpool_file (void)
    {
    struct dhcp_resource *rp = NULL;
    DHCPS_ENTRY_DESC newEntry;
    struct hash_member *resptr = NULL;
    STATUS result = OK;
    char tmp [10];  /* sizeof ("tblc=dlft") for host requirements defaults. */
    char entryName [BASE_NAME + 1];   /* User-given name for address range. */
    char newName [MAX_NAME + 1];    /* Unique name for each entry in range. */
    char startAddress [INET_ADDR_LEN];
    char endAddress [INET_ADDR_LEN];
    char newAddress [INET_ADDR_LEN];
    char params [MAXENTRYLEN];
    struct in_addr nextAddr;
    int length;
    char *pTmp;
    u_long start = 0;
    u_long end = 0;
    u_long loop;
    u_long limit;
    int nresource = 0;

    if (dhcpsAddressHookRtn == NULL)
        return (OK);

    result = (* dhcpsAddressHookRtn) (DHCPS_STORAGE_START,
                                      NULL, NULL, NULL, NULL); 
    if (result == ERROR)
        {
        /* Fatal error - close address cache before server exits. */

        logMsg ("Unable to access address pool cache.\n", 0, 0, 0, 0, 0, 0);

        (* dhcpsAddressHookRtn) (DHCPS_STORAGE_STOP, NULL, NULL, NULL, NULL);

        return (ERROR);
        }

    newEntry.pName = newName;
    newEntry.pAddress = newAddress;
    newEntry.pParams = params;

    FOREVER
        {
        result = (* dhcpsAddressHookRtn) (DHCPS_STORAGE_READ, entryName,
                                          startAddress, endAddress, params);
        if (result == ERROR)         /* No more data. */
            {
            result = OK;
            break;
            }

        /* check resource entry name */

        length = strlen (entryName);
        if (length == 0)
            {
            logMsg ("Error extracting entry name for entry %d.\n",
                     nresource + 1, 0, 0, 0, 0, 0);
            continue;
            }

        /* check IP addresses */

        start = inet_addr (startAddress);
        end = inet_addr (endAddress);

        if (start == ERROR || end == ERROR)
            {
            logMsg ("Invalid address for entry %d.\n", nresource + 1,
                     0, 0, 0, 0, 0);
            continue;
            }

        limit = ntohl (end);
        for (loop = ntohl (start); loop <= limit; loop++)
            {           
            rp = (struct dhcp_resource *)calloc (1, 
                                                sizeof (struct dhcp_resource));
            if (rp == NULL)
                {
#ifdef DHCPS_DEBUG
                logMsg ("Memory allocation error in read_addrpool_file",
                         0, 0, 0, 0, 0, 0);
#endif
                result = ERROR;
                break;
                }

            /* Generate a unique name by appending IP address value. */

            sprintf (newEntry.pName, "%s%lx", entryName, loop);

            /* Assign current IP address in range. */

            nextAddr.s_addr = htonl (loop);
            inet_ntoa_b (nextAddr, newEntry.pAddress);

            if (process_entry (rp, &newEntry) < 0)
                {
                logMsg ("Error processing file entry %d.\n", nresource + 1,
                         0, 0, 0, 0, 0);
                dhcpsFreeResource (rp);
                break;
                }

            /* 
             * Add Host Requirements defaults to resource entry. Do not add
             * to entries containing client-specific or class-specific 
             * parameters. 
             */

            if (ISCLR (rp->valid, S_PARAM_ID) && ISCLR (rp->valid, S_CLASS_ID))
                { 
                sprintf (tmp, "%s", "tblc=dflt");
                pTmp = tmp;
                eval_symbol (&pTmp, rp); 
                }

            /* Set default values for entry, if not already assigned. */

            if (ISSET (rp->valid, S_IP_ADDR))
                set_default (rp);

            /*
             * Append entries to lease descriptor list. Exclude entries which 
             * specify additional client- or class-specific parameters.
             */

            if (ISCLR (rp->valid, S_PARAM_ID) && ISCLR (rp->valid, S_CLASS_ID))
                { 
                resptr = reslist;
                while (resptr->next != NULL)
                    resptr = resptr->next;

                resptr->next = 
                   (struct hash_member *)calloc (1, 
                                                 sizeof (struct hash_member));
                if (resptr->next == NULL)
                    {
#ifdef DHCPS_DEBUG
                    logMsg ("Memory allocation error in read_addrpool_file",
                             0, 0, 0, 0, 0, 0);
#endif
                    dhcpsFreeResource (rp);
                    result = ERROR;
                    break;
                    }
                resptr = resptr->next;
                resptr->next = NULL;
                resptr->data = (void *)rp;

                if (ISSET (rp->valid, S_IP_ADDR))
                    if (hash_ins (&iphashtable, (char *)&rp->ip_addr.s_addr,
                                  sizeof (u_long), resipcmp, &rp->ip_addr, rp) 
                        < 0)
                        {
#ifdef DHCPS_DEBUG
                        logMsg ("Hash table error reading address cache.\n",
                                 0, 0, 0, 0, 0, 0);
#endif
                        result = ERROR;
                        break;
                        }

                /* Add entryname to appropriate hash table. */

                result = hash_ins (&nmhashtable, rp->entryname, 
                                   strlen (rp->entryname), resnmcmp, 
                                   rp->entryname, rp);
                if (result < 0)
                    {
#ifdef DHCPS_DEBUG
                    logMsg ("Hash table error reading address cache.\n",
                             0, 0, 0, 0, 0, 0);
#endif
                    result = ERROR;
                    break;
                    }
                }
            nresource++;
            }
        if (result == ERROR)
            break;
        }
    if (result == OK)
        {
#ifdef DHCPS_DEBUG
        logMsg ("dhcps: read %d entries from address pool cache", nresource, 
                 0, 0, 0, 0, 0);
#endif
        }
    else
        {
        /* Fatal error - close address cache before server exits. */

        (* dhcpsAddressHookRtn) (DHCPS_STORAGE_STOP, NULL, NULL, NULL, NULL);
        } 

    return (result);
    }

/*
 *   The following code portions are Copyright (c) 1988 by Carnegie Mellon.
 *   Modified by Akihiro Tominaga, 1994. 
 *   Ported to VxWorks by Stephen Macmanus, 1996.
 */
/*
 * Copyright (c) 1988 by Carnegie Mellon.
 *
 * Permission to use, copy, modify, and distribute this program for any
 * purpose and without fee is hereby granted, provided that this copyright
 * and permission notice appear on all copies and supporting documentation,
 * the name of Carnegie Mellon not be used in advertising or publicity
 * pertaining to distribution of the program without specific prior
 * permission, and notice be given in supporting documentation that copying
 * and distribution is by permission of Carnegie Mellon and Stanford
 * University.  Carnegie Mellon makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 */

/*******************************************************************************
*
* read_addrpool_db - extract address entries from compiled image
*
* This routine retrieves the address pool entries hard-coded into usrNetwork.c
* as dhcpsLeaseTbl[] and creates the necessary descriptors to assign leases
* to DHCP or BOOTP clients.
*
* RETURNS: OK if database read successfully, or ERROR otherwise.
*
* ERRNO: N/A
*
* NOMANUAL
*/

STATUS read_addrpool_db 
    (
    int poolsize 	/* number of entries in hard-coded table */
    )
    {
    struct dhcp_resource *rp = NULL;
    DHCPS_ENTRY_DESC entryData;
    struct hash_member *resptr = NULL;
    char newName [MAX_NAME + 1];
    char entryName [BASE_NAME + 1];
    int len;
    char newAddress [INET_ADDR_LEN];
    char * 	pStartAddr;
    char * 	pEndAddr;
    struct in_addr newAddr;
    char tmp [10];
    char *pTmp;
    int nresource = 0;
    int loop;
    int result;

    u_long start;
    u_long end;
    u_long loop2;
    u_long limit;

    entryData.pName = newName;
    entryData.pAddress = newAddress;
 
    for (loop = 0; loop < poolsize; loop++)
        {
        pStartAddr = pDhcpsLeasePool [loop].pStartIp;
        pEndAddr = pDhcpsLeasePool [loop].pEndIp;

        /* Ignore bad values for range. */

        if (pStartAddr != NULL && pEndAddr == NULL)
            continue; 

        if (pStartAddr == NULL && pEndAddr != NULL)
            continue; 

        /* If no address specified, just process parameters once. */

        if (pStartAddr == NULL && pEndAddr == NULL)
            {
            start = 0;
            end = 0;
            }
        else
            {
            start = inet_addr (pStartAddr);
            end = inet_addr (pEndAddr);
            if (start == ERROR || end == ERROR)
                continue;
            }

        entryData.pParams = pDhcpsLeasePool [loop].pParams;

        len = strlen (pDhcpsLeasePool[loop].pName);
        if (len > BASE_NAME)
            {
            bcopy (pDhcpsLeasePool[loop].pName, entryName, BASE_NAME);
            entryName [BASE_NAME] = '\0';
            }
        else
            {
            bcopy (pDhcpsLeasePool[loop].pName, entryName, len);
            entryName [len] = '\0';
            }
 
        limit = ntohl (end); 
        for (loop2 = ntohl (start); loop2 <= limit; loop2++)
            {
            rp = 
             (struct dhcp_resource *)calloc (1, sizeof (struct dhcp_resource));
            if (rp == NULL) 
                {
#ifdef DHCPS_DEBUG
                logMsg ("Memory allocation error in read_addrpool_db", 
                         0, 0, 0, 0, 0, 0);
#endif
                return (ERROR);
                }

            /* Generate a unique name for each entry in range. */

           if (loop == 0) 	/* Don't modify name of default entry. */
               sprintf (entryData.pName, "%s", entryName);
           else
               sprintf (entryData.pName, "%s%lx", entryName, loop2);

            /* Store current IP address in range. */

            newAddr.s_addr = htonl (loop2);
            inet_ntoa_b (newAddr, entryData.pAddress);

            /* Add new entry to address pool. */

            if (process_entry (rp, &entryData) < 0) 
                {
                logMsg ("Error processing table entry %d.\n", loop + 1, 
                         0, 0, 0, 0, 0);
                dhcpsFreeResource (rp);
                break;
                }

            /* Add host requirements defaults to all lease entries. */
            if (loop != 0)     /* Ignore first entry, which holds defaults. */
                {
                /* Add Host Requirements Defaults to resource entry. Do not
                 * add to entries containing client-specific or class-specific
                 * parameters.
                 */ 
                if (ISCLR (rp->valid, S_PARAM_ID) && 
                      ISCLR (rp->valid, S_CLASS_ID))
                    {
                    sprintf (tmp, "%s", "tblc=dflt");
                    pTmp = tmp;
                    eval_symbol (&pTmp, rp);
                    }
                }

            /* Set default values for entry, if not assigned in table. */

            if (ISSET (rp->valid, S_IP_ADDR))
                set_default (rp);

            /*
             * make resource list for lease entries. Exclude entries which 
             * specify additional client- or class-specific parameters.
             */

            if (ISCLR (rp->valid, S_PARAM_ID) && ISCLR (rp->valid, S_CLASS_ID))
                { 
                if (reslist == NULL) 
                    {
                    resptr = reslist = 
                        (struct hash_member *)calloc (1, 
                                                  sizeof (struct hash_member));
                    if (resptr == NULL) 
                        {
#ifdef DHCPS_DEBUG
                        logMsg ("Memory allocation error in read_addrpool_db", 
                                 0, 0, 0, 0, 0, 0);
#endif
                        dhcpsFreeResource (rp);
                        return (ERROR);
                        }
                    } 
                else 
                    {
                    /* not first time */
                    resptr->next = 
                       (struct hash_member *)calloc (1, 
                                                  sizeof (struct hash_member));
                    if (resptr->next == NULL) 
                        {
#ifdef DHCPS_DEBUG
                        logMsg ("Memory allocation error in read_addrpool_db", 
                                 0, 0, 0, 0, 0, 0);
#endif
                        dhcpsFreeResource (rp);
                        return (ERROR);
                        }
                    resptr = resptr->next;
                    }
                resptr->next = NULL;
                resptr->data = (hash_datum *)rp;

                /* Store entry in hash table with IP address as key. */

                if (ISSET (rp->valid, S_IP_ADDR))
                    if (hash_ins (&iphashtable, (char *)&rp->ip_addr.s_addr,
		                  sizeof (u_long), resipcmp, &rp->ip_addr, rp) 
                        < 0) 
                        {
#ifdef DHCPS_DEBUG
                        logMsg ("Hash table error reading address database.\n", 
                                 0, 0, 0, 0, 0, 0);
#endif
                        return (ERROR); 
                        }
                }

            /* Store entry in hash table with resource entryname as key. */

            result = hash_ins (&nmhashtable, rp->entryname, 
                               strlen (rp->entryname), resnmcmp, 
                               rp->entryname, rp);
            if (result < 0) 
                {
#ifdef DHCPS_DEBUG
                logMsg ("Hash table error reading address database.\n", 
                         0, 0, 0, 0, 0, 0);
#endif
                return (ERROR); 
                }
            nresource++;
            }
        }

#ifdef DHCPS_DEBUG
    logMsg ("dhcps: read %d entries from addr-pool database.\n", nresource, 
             0, 0, 0, 0, 0);
#endif

    return (OK);
    }

/*******************************************************************************
*
* read_entry - retrieve database entries from permanent storage
*
* This routine calls a hook routine, supplied by the user, to retrieve entries
* from the binding database. Each entry consists of a string with the following
* format:
*         idtype:id:subnet:htype:haddr:"expire_date":resource_name
*
* The colon delimited fields may contain quoted strings. The '\' character is 
* used to embed special characters, such as a colon or double quote.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*/

static void read_entry
    (
    char *buffer, 		/* buffer to store extracted data */
    unsigned *bufsiz 		/* number of characters retrieved */
    )
    {
    int c = ' ';          /* Whitespace character. */
    int length = 0;
    int index = 0;
    char data [MAXSTRINGLEN];
    BOOL finished = FALSE;
    BOOL termfound = FALSE;   /* Terminator flag for special characters. */
    STATUS result;

    result = (* dhcpsLeaseHookRtn) (DHCPS_STORAGE_READ, data, MAXSTRINGLEN);
    if (result == ERROR)
        {
        buffer[0] = '\0';			/* Empty string */
        *bufsiz = 0;
        return; 
        }

    /*
     * read binding information 
     *
     * idtype:id:subnet:htype:haddr:"expire_date":resource_name
     */

    c = data[index++];
    while (c != '\0')
        { 
        /* Store the entry in the data buffer, processing special characters 
         * like double quotes (") and backslashes (\).
         */

        switch (c) 
            {
            case '\\':       /* Backslash - read a new character. */
                c = data[index++];
                *buffer++ = c;	     /* Store the literal character */
                length++;
                if (length >= *bufsiz - 1) 
                    finished = TRUE; 
                break;
            case '"':
                *buffer++ = '"';        /* Store double-quote */
                length++;
                if (length >= *bufsiz - 1) 
                    {
                    finished = TRUE;
                    break;
                    }
                termfound = FALSE;
                c = data[index++];
                while (c != '\0')       /* Special quote processing loop */
                    {
                    switch (c) 
                        {
                        case '"':
	                    *buffer++ = '"';  /* Store matching quote */
	                    length++;
	                    if (length < *bufsiz - 1) 
	                        termfound = TRUE;
                            else
	                        finished = TRUE;
                            break;  
                        case '\\':
                            c = data[index++];    /* Fall-through */
                        default:
	                    *buffer++ = c;    /* Other character, store it */
	                    length++;
	                    if (length >= *bufsiz - 1) 
                                finished = TRUE;   
                            break;
                        } 
                    if (finished || termfound)
                        break; 
                    c = data[index++];
                    }
                if (c == '\0')      /* Missing close quote. */
                    finished = TRUE;
                break;
            default:
                *buffer++ = c;		/* Store other characters */
                length++;
                if (length >= *bufsiz - 1) 
                    finished = TRUE;
                break;
            }
        if (finished)    /* Buffer limit reached - stop reading. */
            break;
        c = data[index++];
        }    
    *buffer = '\0';			/* Terminate string */
    *bufsiz = length;			/* Tell the caller its length */
    }


/*******************************************************************************
*
* process_entry - parse address pool entries  
*
* This routine forms lease descriptors from the address pool entries 
* retrieved from permanent storage or the compiled image. Each lease descriptor
* contains three strings: the entry name, the IP address, and a parameter list.
* The parameter list contains a colon-separated list of entries. The contents 
* of the parameter list determine the type of lease assignment. That type is 
* used by the server to select the priority with which lease entries are 
* issued to clients.
*
* RETURNS: 0 if processed successfully, or -1 otherwise. 
*
* ERRNO: N/A
*
* NOMANUAL
*/

int process_entry
    (
    struct dhcp_resource *rp, 		/* pointer to lease descriptor */
    DHCPS_ENTRY_DESC * 	leaseData 	/* pointer to address pool entry */
    )
    {
    char buffer[MAXENTRYLEN];
    char *src;

    if (!rp)
       return(-1);

    if (strlen (leaseData->pName) > MAX_NAME)
        {
        bcopy (leaseData->pName, rp->entryname, MAX_NAME);
        rp->entryname [MAX_NAME] = '\0';
        }
    else
        {
        bcopy (leaseData->pName, rp->entryname, strlen (leaseData->pName));
        rp->entryname [strlen (leaseData->pName)] = '\0';
        }

    if (leaseData->pAddress != NULL)
        {
        rp->ip_addr.s_addr = inet_addr (leaseData->pAddress);
        if (rp->ip_addr.s_addr != ERROR && rp->ip_addr.s_addr != 0)
            {
            SETBIT (rp->valid, S_IP_ADDR);
            SETBIT (rp->active, S_IP_ADDR);     /* Pass value to client. */
            }
        }

    sprintf (buffer, ":%s", leaseData->pParams);
    src = (char *)buffer;

    FOREVER 
        {
        adjust (&src);
        switch (eval_symbol (&src, rp)) 
            {
            case E_END_OF_ENTRY:    /* No more fields in entry. */
                return(0);
                break;
            case SUCCESS:           /* Continue processing fields. */
                break;
            default:
                return(-1);         /* Error in parameter entry. */
                break;
            }
        }
    return (0);                   /* Not reached. */
    }

/*******************************************************************************
*
* adjust - update pointer to access individual parameters
*
* This routine adjusts the caller's pointer to point just past the colon which
* separates address pool parameters. If it encounters a null character, the
* pointer update stops.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*/

static void adjust
    (
    char **s 	/* current location in input string */
    )
    {
    FAST char *t = NULL;

    t = *s;
    while (*t && (*t != ':')) 
        t++;
    if (*t) 
       t++;

    *s = t;
    return;
    }

/*******************************************************************************
*
* eval_symbol - process individual parameter entries
*
* This routine parses individual parameter entries. Each entry has one of
* two formats. Parameters may be added by including a string of the form
* <name>=<value> in the parameter field. Parameters may be deleted by
* including a parameter name followed by the "@" sign. This routine extracts
* the four character parameter name and uses it to determine the appopriate 
* processing function, named proc_* and defined above.
*
* Finally, parameter names may be preceded by the "!" character. This indicates
* that the added parameter is active. This designation is used to identify
* parameters with values which differ from those specified in the Host 
* Requirements RFC. Parameters designated as active are always sent to the
* DHCP client, which would otherwise use the Host Requirements values.
* 
* RETURNS: 0 if processed successfully, or -1 otherwise.
*
* ERRNO: N/A
*
* NOMANUAL
*/

int eval_symbol
    (
    char **symbol, 		/* current location in input string */
    struct dhcp_resource *rp 	/* pointer to lease descriptor */
    )
    {
    struct symbolmap *symbolptr = NULL;
    int i = 0;
    int result;
    int optype = 0;         /* Indicates boolean, addition, or deletion */
    int active = 0;

    if ( (*symbol)[0] == '\0') 
        return(E_END_OF_ENTRY);
    if ( (*symbol)[0] == ':') 
        return (SUCCESS);
  
    eat_whitespace (symbol);
  
    /*
     * Is it active parameter? 
     * (this parameter is not default of Host Requirements RFC)
     */

    if ( (*symbol)[0] == '!')
        {
        active = TRUE;
        (*symbol)++;
        }
    else 
        active = FALSE;

    /* Determine the type of operation to be done on this symbol */

    switch ( (*symbol)[4]) 
        {
        case '=':
            optype = OP_ADDITION;
            break;
        case '@':
            optype = OP_DELETION;
            break;
        default:
            logMsg ("Warning: Unknown operation in resource database.\n",
                     0, 0, 0, 0, 0, 0);
            return (-1);
        }
  
    symbolptr = symbol_list;
    for (i = 0; i <= S_LAST_OPTION; i++) 
        {
        if ( ( (symbolptr->symbol)[0] == (*symbol)[0]) &&
	     ( (symbolptr->symbol)[1] == (*symbol)[1]) &&
	     ( (symbolptr->symbol)[2] == (*symbol)[2]) &&
	     ( (symbolptr->symbol)[3] == (*symbol)[3])) 
            break;
        symbolptr++;
        }
    if (i > S_LAST_OPTION) 
        {
        logMsg ("Warning: unknown symbol \"%c%c%c%c\" in \"%s\"", 
                 (*symbol)[0], (*symbol)[1], (*symbol)[2], (*symbol)[3], 
                 (int)rp->entryname, 0);
        return (-1);
        }
  
    /*
     * Skip past the = or @ character (to point to the data) if this
     * isn't a boolean operation.  For boolean operations, just skip
     * over the two-character tag symbol (and nothing else. . . .).
     */

    (*symbol) += 5;

    if (symbolptr->func != NULL) 
        {
        result = (*symbolptr->func) (symbolptr->code, optype, symbol, rp);
        if (symbolptr->code != S_TABLE_CONT) 
            {
            if (optype == OP_DELETION || result != 0) 
                {
	        CLRBIT (rp->valid, symbolptr->code);
	        CLRBIT (rp->active, symbolptr->code);
                } 
            else 
                {
	        SETBIT (rp->valid, symbolptr->code);
	        if (active)
                    SETBIT (rp->active, symbolptr->code);
                else
                    CLRBIT (rp->active, symbolptr->code);
                }
            }
        return (result);
        } 
    else 
        {
#ifdef DHCPS_DEBUG
        logMsg ("Warning: No processing function for symbol.\n", 
                 0, 0, 0, 0, 0, 0); 
#endif
        return (-1);
        }
    }


/*******************************************************************************
*
* eat_whitespace - update pointer to access meaningful characters
*
* This routine adjusts the caller's pointer to point past excess whitespace.
* If it encounters a null character, the pointer update stops.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*/

static void eat_whitespace
    (
    char **s	/* current location in input string */
    )
    {
    FAST char *t = NULL;

    t = *s;
    while (*t && isspace ( (int) *t)) 
        t++;
    *s = t;
    }

/*******************************************************************************
*
* get_string - retrieve characters from input until delimiter reached
*
* This routine copies up to MAXSTRINGLEN characters into the provided buffer
* until the NULL character or a colon delimiter is encountered. The input
* pointer is also updated to point past the last character retrieved.
* It handles special characters preceded by "\" as well as quoted substrings.
* Before returning, trailing whitespace is removed from the extracted string.
* 
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*/

static void get_string
    (
    char **src, 	/* current location in input string */
    char *target 	/* pointer to storage for extracted characters */
    )
    {
    int n = 0; 
    int len = 0; 
    int quoteflag = 0;
    char *	tmpp = NULL;


    bzero (target, MAXSTRINGLEN);
    tmpp = target;
    quoteflag = FALSE;
    n = 0;
    len = MAXSTRINGLEN - 1;

    /* Extract characters until EOS or a colon delimiter is reached. */

    while ((n < MAXSTRINGLEN - 1) && (**src)) 
       {
       if (!quoteflag && (**src == ':')) 
           {
           break;
           }
       if (**src == '"') 
           {
           (*src)++;
           quoteflag = !quoteflag;
           continue;
           }
       if (**src == '\\') 
           {
           (*src)++;
           if (! **src) 
               {
	       break;
               }
           }
       *tmpp++ = *(*src)++;
       n++;
       }
  
    /* Remove that troublesome trailing whitespace. . .  */

    while ((n > 0) && isspace( (int) tmpp[-1])) 
       {
       tmpp--;
       n--;
       }

    *tmpp = '\0';

    return;
    }

/*******************************************************************************
*
* get_integer - retrieve numeric value from input until delimiter reached
*
* This routine converts the input string into a numeric value until NULL or 
* a non-digit character is reached. Hexadecimal numbers are specified by a 
* leading 0x and octal numbers by a leading 0. The input pointer is updated
* to indicate the first non-digit character.
*
* RETURNS: Extracted value, or 0 if none.
*
* ERRNO: N/A
*
* NOMANUAL
*/

static long get_integer
    (
    char **src 	/* current location in input string */
    )
    {
    FAST long value = 0;
    FAST long base = 0;
    int invert = 1;
    char c = 0;

    /*
     * Collect number up to first illegal character.  Values are specified
     * as for C:  0x=hex, 0=octal, other=decimal.
     */
    value = 0;
    base = 10;
    if (**src == '-') 
        {
        invert = -1;
        (*src)++;
        }
    if (**src == '0') 
        {
        base = 8;
        (*src)++;
        }
    if (**src == 'x' || **src == 'X') 
        {
        base = 16;
        (*src)++;
        }
    while ( (c = **src)) 
        {
        if (isdigit ( (int) c)) 
            {
            value = (value * base) + (c - '0');
            (*src)++;
            continue;
            }
        if (base == 16 && isxdigit ( (int)c)) 
            {
            /* Convert hex alpha digits to uppercase, then to numeric value. */
            value = (value << 4) + ((c & ~32) + 10 - 'A');
            (*src)++;
            continue;
            }
        break;
        }
    return (invert * value);
    }

/*******************************************************************************
*
* prs_inaddr - retrieve numeric value for IP address until delimiter reached
*
* This routine converts the dot notation input string into a numeric value,
* and stores the result (in network byte order) in the provided parameter.
* The input pointer is updated to indicate the next character in the string.
*
* RETURNS: 0 if successful, or -1 for bad format. 
*
* ERRNO: N/A
*
* NOMANUAL
*/

static int prs_inaddr
    (
    char **src, 	/* current location in input string */
    u_long *result 	/* pointer to storage for extracted value */
    )
    {
    FAST u_long value = 0;
    u_long parts[4]; 
    u_long *	pp = parts;
    int n;
  
    if (!isdigit ( (int) **src)) 
        return (-1);

    FOREVER
        {
        value = get_integer (src);
        if (**src == '.') 
            {
            /*
             * Internet format:
             *      a.b.c.d
             *      a.b.c      (with c treated as 16-bits)
             *      a.b        (with b treated as 24 bits)
             */
            if (pp >= parts + 4) 
                return (-1);
            *pp++ = value;
            (*src)++;
            continue;
            }
        break;
        }

    /*
     * Check for trailing characters.
     */
    if (**src && ! (isspace ( (int) **src) || (**src == ':'))) 
        return (-1);
    *pp++ = value;

    /* Construct the address according to the number of parts specified.  */

    n = pp - parts;
    switch (n) 
        {
        case 1:        /* a -- 32 bits */
            value = parts[0];
            break;
        case 2:        /* a.b -- 8.24 bits */
            value = (parts[0] << 24) | (parts[1] & 0xFFFFFF);
            break;
        case 3:        /* a.b.c -- 8.8.16 bits */
            value = (parts[0] << 24) | ((parts[1] & 0xFF) << 16) |
                    (parts[2] & 0xFFFF);
            break;
        case 4:        /* a.b.c.d -- 8.8.8.8 bits */
            value = (parts[0] << 24) | ( (parts[1] & 0xFF) << 16) |
                     ( (parts[2] & 0xFF) << 8) | (parts[3] & 0xFF);
            break;
        default:
            return (-1);
        }
    *result = htonl(value);
    return (0);
    }

/*******************************************************************************
*
* resnmcmp - verify resource name for address pool entries
*
* This routine compares a hash table key against the entry name of a 
* lease descriptor. It is used when storing and retrieving lease
* descriptors with the internal hash tables.
*
* RETURNS: TRUE if names match, or FALSE otherwise.
*
* ERRNO: N/A
*
* NOMANUAL
*/

int resnmcmp
    (
    char *name, 		/* pointer to hash table key */
    struct dhcp_resource *rp 	/* pointer to lease descriptor */
    )
    {
    return !strcmp (name, rp->entryname);
    }
