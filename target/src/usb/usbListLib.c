/* usbListLib.c - Linked list utility functions */

/* Copyright 2000 Wind River Systems, Inc. */

/*
Modification history
--------------------
01a,10jun99,rcb  First.
*/

/*
DESCRIPTION

This file inmplements a set of general-purpose linked-list functions which are
portable across OS's.  Linked lists are a collection of LINK structures.  Each
LINK structure contains a forward a backward list pointer.  Each LINK structure
also contains a <pStruct> field which points (typically) to the caller's 
structure which contains the LINK structure.

usbListLink() and usbListUnlink() are used to add and remove LINK structures
in a linked list.  The LINK field may be placed anywhere in the client's 
structure, and the client's structure may even contain more than one LINK field
- allowing the structure to be linked into multiple lists simultaneously.

usbListFirst() retrieves the first structure on a linked list and usbListNext() 
retrieves subsequent structures.
*/


/* defines */

#define DEBUG_LIST


/* includes */

#include "usb/usbPlatform.h"
#include "usb/ossLib.h"
#include "usb/usbListLib.h"	/* our API */

#ifdef DEBUG_LIST
#include "assert.h"
#endif


/* functions */

/***************************************************************************
*
* usbListLink - Add an element to a linked list
*
* Using the LINK structure <pLink>, add <pStruct> to the list of which the 
* list head is <pHead>.  <flag> must be LINK_HEAD or LINK_TAIL.
*
* RETURNS: N/A
*/

VOID usbListLink 
    (
    pLIST_HEAD pHead,		/* list head */
    pVOID pStruct,		/* ptr to base of structure to be linked */
    pLINK pLink,		/* ptr to LINK structure to be linked */
    UINT16 flag 		/* indicates LINK_HEAD or LINK_TAIL */
    )

    {
    pLINK *ppTail;

    #ifdef DEBUG_LIST

    /* See if the indicated pStruct is already on the list. */

    pVOID pElement = usbListFirst (pHead);
    pUINT8 pElementLink;

    while (pElement != NULL)
	{
	assert (pElement != pStruct);

	pElementLink = (pUINT8) pElement;
	pElementLink += ((pUINT8) pLink) - ((pUINT8) pStruct);

	pElement = usbListNext ((pLINK) pElementLink);
	}


    #endif /* #ifdef DEBUG_LIST */


    pLink->pStruct = pStruct;

    if (flag == LINK_HEAD) 
	{

	/* Add the Entry to the head of the list. */

	pLink->linkFwd = pHead->pLink;
	pLink->linkBack = (pLINK) pHead;

	if (pHead->pLink != NULL)
	    pHead->pLink->linkBack = pLink;

	pHead->pLink = pLink;
	}
    else 
	{
	
	/* Add the entry to the tail of the list. */

	for (ppTail = &pHead->pLink; *ppTail != NULL; 
	    ppTail = &((*ppTail)->linkFwd))
	    ;

	pLink->linkFwd = NULL;
	pLink->linkBack = (pLINK) ppTail;

	*ppTail = pLink;
	}
    }


/***************************************************************************
*
* usbListLinkProt - Add an element to a list guarded by a mutex
*
* This function is similar to linkList() with the addition that this
* function will take the <mutex> prior to manipulating the list.
*
* NOTE: The function will block forever if the mutex does not become
* available.
*
* RETURNS: N/A
*/

VOID usbListLinkProt 
    (
    pLIST_HEAD pHead,		/* list head */
    pVOID pStruct,		/* ptr to base of structure to be linked */
    pLINK pLink,		/* ptr to LINK structure to be linked */
    UINT16 flag,		/* indicates LINK_HEAD or LINK_TAIL */
    MUTEX_HANDLE mutex		/* list guard mutex */
    )

    {
    OSS_MUTEX_TAKE (mutex, OSS_BLOCK);
    usbListLink (pHead, pStruct, pLink, flag);
    OSS_MUTEX_RELEASE (mutex);
    }
    

/***************************************************************************
*
* usbListUnlink - Remove an entry from a linked list
*
* Removes <pLink> from a linked list.
*
* RETURNS: N/A
*/

VOID usbListUnlink 
    (
    pLINK pLink 		/* LINK structure to be unlinked */
    )

    {

    /* De-link pElement from its linked-lis. */

    if (pLink->linkBack) {
	pLink->linkBack->linkFwd = pLink->linkFwd;
    }

    if (pLink->linkFwd) {
	pLink->linkFwd->linkBack = pLink->linkBack;
    }

    pLink->linkBack = pLink->linkFwd = NULL;
    }



/***************************************************************************
*
* usbListUnlinkProt - Removes an element from a list guarged by a mutex
*
* This function is the same as usbListUnlink() with the addition that this
* function will take the <mutex> prior to manipulating the list.
*
* NOTE: The function will block forever if the mutex does not become
* available.
*
* RETURNS: N/A
*/

VOID usbListUnlinkProt
    (
    pLINK pLink,		/* LINK structure to be unlinked */
    MUTEX_HANDLE mutex		/* list guard mutex */
    )

    {
    OSS_MUTEX_TAKE (mutex, OSS_BLOCK);
    usbListUnlink (pLink);
    OSS_MUTEX_RELEASE (mutex);
    }


/***************************************************************************
*
* usbListFirst - Returns first entry on a linked list
*
* Returns the pointer to the first structure in a linked list given a
* pointer to the LIST_HEAD.
*
* RETURNS: <pStruct> of first structure on list or NULL if list empty.
*/

pVOID usbListFirst
    (
    pLIST_HEAD pListHead	/* head of linked list */
    )

    {
    if (pListHead == NULL || pListHead->pLink == NULL)
	return NULL;

    return pListHead->pLink->pStruct;
    }


/***************************************************************************
*
* usbListNext - Retrieves next pStruct in a linked list
*
* Returns the pointer to the next structure in a linked list given a 
* <pLink> pointer.  The value returned is the <pStruct> of the element
* in the linked list which follows the current <pLink>, not a pointer to
* the following <pLink>.  (Typically, a client is more interested in
* walking its own list of structures rather than the LINK structures
* used to maintain the linked list.
*
* RETURNS: <pStruct> of next structure in list or NULL if end of list.
*/

pVOID usbListNext
    (
    pLINK pLink 		/* LINK structure */
    )

    {
    if (pLink == NULL || pLink->linkFwd == NULL)
	return NULL;

    return pLink->linkFwd->pStruct;
    }


/* End of file. */
