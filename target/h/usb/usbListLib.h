/* usbListLib.h - Linked list utility functions */

/* Copyright 2000 Wind River Systems, Inc. */

/*
Modification history
--------------------
01a,10jun99,rcb  First.
*/

/*
DESCRIPTION

This file defines a set of general-purpose linked-list functions which are
portable across OS's.
*/

#ifndef __INCusbListLibh
#define __INCusbListLibh

#ifdef	__cplusplus
extern "C" {
#endif


/* includes */

#include "usb/ossLib.h"


/* defines */

/* Arguments passed to the usbListLink/usbListLinkProt utility function. */

#define LINK_HEAD   0
#define LINK_TAIL   1


/* Arguments passed to the usbListFind utility function. */

#define LINK_TEST   0
#define LINK_REMOVE 1


/* typedefs */

/*
 * LINK
 * 
 * Linked-list management structure. 
 *
 * NOTE: Code relies on the fact that LINK.linkFwd and LIST_HEAD.pLink are 
 * both the first fields in their respective structures.
 */

typedef struct link 
    {
    struct link *linkFwd;	/* Link to next entry */
    struct link *linkBack;	/* Link to previous entry */
    pVOID pStruct;		/* Points to base of structure being linked */
    } LINK, *pLINK;


/*
 * LIST_HEAD
 *
 * Defines the head of a linked list.
 */

typedef struct list_head
    {
    pLINK pLink;		/* Pointer to first entry on list */
    } LIST_HEAD, *pLIST_HEAD;
    

/* Function prototypes. */

VOID usbListLink 
    (
    pLIST_HEAD pHead,		/* list head */
    pVOID pStruct,		/* ptr to base of structure to be linked */
    pLINK pLink,		/* ptr to LINK structure to be linked */
    UINT16 flag 		/* indicates LINK_HEAD or LINK_TAIL */
    );

VOID usbListLinkProt 
    (
    pLIST_HEAD pHead,		/* list head */
    pVOID pStruct,		/* ptr to base of structure to be linked */
    pLINK pLink,		/* ptr to LINK structure to be linked */
    UINT16 flag,		/* indicates LINK_HEAD or LINK_TAIL */
    MUTEX_HANDLE mutex		/* list guard mutex */
    );
    

VOID usbListUnlink 
    (
    pLINK pLink 		/* LINK structure to be unlinked */
    );


VOID usbListUnlinkProt
    (
    pLINK pLink,		/* LINK structure to be unlinked */
    MUTEX_HANDLE mutex		/* list guard mutex */
    );


pVOID usbListFirst
    (
    pLIST_HEAD pListHead	/* head of linked list */
    );


pVOID usbListNext
    (
    pLINK pLink 		/* LINK structure */
    );


#ifdef	__cplusplus
}
#endif

#endif	/* __INCusbListLibh */


/* End of file. */

