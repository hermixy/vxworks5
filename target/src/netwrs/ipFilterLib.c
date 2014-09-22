/* ipFilterLib.c - IP filter hooks library */

/* Copyright 1984 - 2000 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01d,10may02,kbw  making man page edits
01c,07may02,kbw  man page edits
01b,25oct00,ham  doc: cleanup for vxWorks AE 1.0.
01a,17apr97,vin  written
*/

/*
DESCRIPTION
This library provides utilities that give direct access to IP packets.
You can examine or process incoming raw IP packets using the hooks you  
installed by calling ipFilterHookAdd().  Using a filter hook, you can 
build IP traffic monitoring and testing tools.  

However, you should not use an IP filter hook as a standard means to 
provide network access to an application.  The filter hook lets you 
see, process, and even consume packets before their intended recipients 
have seen the packets.  For most network applications, this is too much 
responsibility. Thus, most network applications should access the network 
access through the higher-level socket interface provided by sockLib.  

The ipFilterLibInit() routine links the IP filtering facility into the 
VxWorks system.  This is performed automatically if INCLUDE_IP_FILTER 
is defined.

VXWORKS AE PROTECTION DOMAINS
Under VxWorks AE, you can call ipFilterHookAdd() from within the kernel 
protection domain only, and the function referenced in the <ipFilterHook> 
parameter must reside in the kernel protection domain.  This restriction 
does not apply to non-AE versions of VxWorks.  

*/

/* includes */

#include "vxWorks.h"

/* externs */

IMPORT FUNCPTR  _ipFilterHook; 	/* IP filter hook defined in netLib.c */

/******************************************************************************
*
* ipFilterLibInit - initialize IP filter facility
*
* This routine links the IP filter facility into the VxWorks system.
* These routines are included automatically if INCLUDE_IP_FILTER
* is defined.
*
* RETURNS: N/A
*/

void ipFilterLibInit (void)
    {
    }

/*******************************************************************************
*
* ipFilterHookAdd - add a routine to receive all internet protocol packets
*
* This routine adds a hook routine that will be called for every IP
* packet that is received.
*
* The filter hook routine should be of the form:
* .CS
* BOOL ipFilterHook
*     (
*     struct ifnet *pIf,        /@ interface that received the packet @/
*     struct mbuf  **pPtrMbuf,  /@ pointer to pointer to an mbuf chain @/
*     struct ip    **pPtrIpHdr, /@ pointer to pointer to IP header @/ 
*     int          ipHdrLen,    /@ IP packet header length @/
*     )
* .CE
* The hook routine should return TRUE if it has handled the input packet. A 
* returned value of TRUE effectively consumes the packet from the viewpoint 
* of IP, which will never see the packet. As a result, when the filter hook 
* returns TRUE, it must handle the freeing of any resources associated with 
* the packet.  For example, the filter hook routine would be responsible for 
* freeing the packet's 'mbuf' chain by calling 'm_freem(*pPtrMbuf)'. 
* 
* The filter hook routine should return FALSE if it has not handled the 
* packet.  In response to a FALSE, the network stack submits the packet 
* for normal IP processing.
*
* Within the packet's IP header (the filter hook can obtain a pointer to the 
* IP header by dereferencing 'pPtrIpHdr'), you will find that the values in 
* the 'ip_len' field, the 'ip_id' field, and 'ip_offset' field have been 
* converted to the host byte order before the packet was handed to the 
* filter hook.
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call ipFilterHookAdd() from within the kernel 
* protection domain only, and the function referenced in the <ipFilterHook> 
* parameter must reside in the kernel protection domain.  This restriction 
* does not apply to non-AE versions of VxWorks.  
*
* RETURNS: OK, always.
*/

STATUS ipFilterHookAdd
    (
    FUNCPTR ipFilterHook   	/* routine to receive raw IP packets */
    )
    {
    _ipFilterHook = ipFilterHook; 
    return (OK);
    }

/*******************************************************************************
*
* ipFilterHookDelete - delete a IP filter hook routine
*
* This routine deletes an IP filter hook.
*
* RETURNS: N/A
*/

void ipFilterHookDelete (void)
    {
    _ipFilterHook = NULL;
    }


