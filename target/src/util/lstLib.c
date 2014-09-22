/* lstLib.c - doubly linked list subroutine library */

/* Copyright 1984-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
02a,19sep01,pcm  added lstLibInit () to bring module into image (SPR 20698)
01z,05oct98,jmp  doc: cleanup.
01y,14oct95,jdi  doc: fixed typo in lstNth().
01x,13feb95,jdi  doc format change.
01w,20jan93,jdi  documentation cleanup for 5.1.
01v,09jul92,hdn  put an optimized lstGet()
01u,26may92,rrr  the tree shuffle
01t,25nov91,rrr  cleanup of some ansi warnings.
01s,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed VOID to void
		  -changed copyright notice
01r,30apr91,jdi	 documentation tweaks.
01q,05apr91,jdi	 documentation -- removed header parens and x-ref numbers;
		 doc review by dnw.
01p,11feb91,jaa	 documentation cleanup.
01o,05nov87,jlf  documentation
01n,02apr87,ecs  hushed lint in lstFree.
01m,25mar87,jlf  documentation
01l,21dec86,dnw  changed to not get include files from default directories.
01k,01jul86,jlf  documentation.
01j,21may86,llk	 added lstFree and lstNStep.
01i,09apr86,rdc  added lstFind.
01h,20jul85,jlf  documentation.
01g,19sep84,jlf  fixed spacing in comments by adding .ne's.
01f,08sep84,jlf  added comments and copyright notice.
01e,29jun84,dnw  added lstConcat and lstExtract.
01d,03jun84,dnw  added lstFirst, lstLast.
		 changed list.{head,tail} to list.node.{next,previous}.
		 cleaned up comments, etc.
01c,07may84,ecs  added lstNext, lstPrevious, and lstCount.
01b,09jun83,ecs  modified the documentation
01a,06aug82,dnw  created from old singly-linked-list lib which is now "slllb".
*/


/*
DESCRIPTION
This subroutine library supports the creation and maintenance of a
doubly linked list.  The user supplies a list descriptor (type LIST)
that will contain pointers to the first and last nodes in the list,
and a count of the number of nodes in the list.  The nodes in the
list can be any user-defined structure, but they must reserve space
for two pointers as their first elements.  Both the forward and
backward chains are terminated with a NULL pointer.

The linked-list library simply manipulates the linked-list data structures;
no kernel functions are invoked.  In particular, linked lists by themselves
provide no task synchronization or mutual exclusion.  If multiple tasks will
access a single linked list, that list must be guarded with some
mutual-exclusion mechanism (e.g., a mutual-exclusion semaphore).

NON-EMPTY LIST:
.CS
   ---------             --------          --------
   | head--------------->| next----------->| next---------
   |       |             |      |          |      |      |
   |       |       ------- prev |<---------- prev |      |
   |       |       |     |      |          |      |      |
   | tail------    |     | ...  |    ----->| ...  |      |
   |       |  |    v                 |                   v
   |count=2|  |  -----               |                 -----
   ---------  |   ---                |                  ---
              |    -                 |                   -
              |                      |
              ------------------------
.CE

EMPTY LIST:
.CS
	-----------
        |  head------------------
        |         |             |
        |  tail----------       |
        |         |     |       v
        | count=0 |   -----   -----
        -----------    ---     ---
                        -	-
.CE

INCLUDE FILES: lstLib.h
*/


/* LINTLIBRARY */

#include "vxWorks.h"
#include "lstLib.h"
#include "stdlib.h"

#define HEAD	node.next		/* first node in list */
#define TAIL	node.previous		/* last node in list */

/*********************************************************************
*
* lstLibInit - initializes lstLib module
*
* This routine pulls lstLib into the vxWorks image.
*
* RETURNS: N/A
*/
void lstLibInit (void)
    {
    return;
    }

/*********************************************************************
*
* lstInit - initialize a list descriptor
*
* This routine initializes a specified list to an empty list.
*
* RETURNS: N/A
*/

void lstInit
    (
    FAST LIST *pList     /* ptr to list descriptor to be initialized */
    )
    {
    pList->HEAD	 = NULL;
    pList->TAIL  = NULL;
    pList->count = 0;
    }
/*************************************************************************
*
* lstAdd - add a node to the end of a list
*
* This routine adds a specified node to the end of a specified list.
*
* RETURNS: N/A
*/

void lstAdd
    (
    LIST *pList,        /* pointer to list descriptor */
    NODE *pNode         /* pointer to node to be added */
    )
    {
    lstInsert (pList, pList->TAIL, pNode);
    }
/**************************************************************************
*
* lstConcat - concatenate two lists
*
* This routine concatenates the second list to the end of the first list.
* The second list is left empty.  Either list (or both) can be
* empty at the beginning of the operation.
*
* RETURNS: N/A
*/

void lstConcat
    (
    FAST LIST *pDstList,        /* destination list */
    FAST LIST *pAddList         /* list to be added to dstList */
    )
    {
    if (pAddList->count == 0)		/* nothing to do if AddList is empty */
	return;

    if (pDstList->count == 0)
	*pDstList = *pAddList;
    else
	{
	/* both lists non-empty; update DstList pointers */

	pDstList->TAIL->next     = pAddList->HEAD;
	pAddList->HEAD->previous = pDstList->TAIL;
	pDstList->TAIL           = pAddList->TAIL;

	pDstList->count += pAddList->count;
	}

    /* make AddList empty */

    lstInit (pAddList);
    }
/**************************************************************************
*
* lstCount - report the number of nodes in a list
*
* This routine returns the number of nodes in a specified list.
*
* RETURNS:
* The number of nodes in the list.
*/

int lstCount
    (
    LIST *pList         /* pointer to list descriptor */
    )
    {
    return (pList->count);
    }
/**************************************************************************
*
* lstDelete - delete a specified node from a list
*
* This routine deletes a specified node from a specified list.
*
* RETURNS: N/A
*/

void lstDelete
    (
    FAST LIST *pList,   /* pointer to list descriptor */
    FAST NODE *pNode    /* pointer to node to be deleted */
    )
    {
    if (pNode->previous == NULL)
	pList->HEAD = pNode->next;
    else
	pNode->previous->next = pNode->next;

    if (pNode->next == NULL)
	pList->TAIL = pNode->previous;
    else
	pNode->next->previous = pNode->previous;

    /* update node count */

    pList->count--;
    }
/************************************************************************
*
* lstExtract - extract a sublist from a list
*
* This routine extracts the sublist that starts with <pStartNode> and ends
* with <pEndNode> from a source list.  It places the extracted list in
* <pDstList>.
*
* RETURNS: N/A
*/

void lstExtract
    (
    FAST LIST *pSrcList,      /* pointer to source list */
    FAST NODE *pStartNode,    /* first node in sublist to be extracted */
    FAST NODE *pEndNode,      /* last node in sublist to be extracted */
    FAST LIST *pDstList       /* ptr to list where to put extracted list */
    )
    {
    FAST int i;
    FAST NODE *pNode;

    /* fix pointers in original list */

    if (pStartNode->previous == NULL)
	pSrcList->HEAD = pEndNode->next;
    else
	pStartNode->previous->next = pEndNode->next;

    if (pEndNode->next == NULL)
	pSrcList->TAIL = pStartNode->previous;
    else
	pEndNode->next->previous = pStartNode->previous;


    /* fix pointers in extracted list */

    pDstList->HEAD = pStartNode;
    pDstList->TAIL = pEndNode;

    pStartNode->previous = NULL;
    pEndNode->next       = NULL;


    /* count number of nodes in extracted list and update counts in lists */

    i = 0;

    for (pNode = pStartNode; pNode != NULL; pNode = pNode->next)
	i++;

    pSrcList->count -= i;
    pDstList->count = i;
    }
/************************************************************************
*
* lstFirst - find first node in list
*
* This routine finds the first node in a linked list.
*
* RETURNS
* A pointer to the first node in a list, or
* NULL if the list is empty.
*/

NODE *lstFirst
    (
    LIST *pList         /* pointer to list descriptor */
    )
    {
    return (pList->HEAD);
    }
/************************************************************************
*
* lstGet - delete and return the first node from a list
*
* This routine gets the first node from a specified list, deletes the node
* from the list, and returns a pointer to the node gotten.
*
* RETURNS
* A pointer to the node gotten, or
* NULL if the list is empty.
*/

NODE *lstGet
    (
    FAST LIST *pList    /* ptr to list from which to get node */
    )
    {
    FAST NODE *pNode = pList->HEAD;

    if (pNode != NULL)                      /* is list empty? */
	{
	pList->HEAD = pNode->next;          /* make next node be 1st */

	if (pNode->next == NULL)            /* is there any next node? */
	    pList->TAIL = NULL;             /*   no - list is empty */
	else
	    pNode->next->previous = NULL;   /*   yes - make it 1st node */

	pList->count--;                     /* update node count */
	}

    return (pNode);
    }
/************************************************************************
*
* lstInsert - insert a node in a list after a specified node
*
* This routine inserts a specified node in a specified list.
* The new node is placed following the list node <pPrev>.
* If <pPrev> is NULL, the node is inserted at the head of the list.
*
* RETURNS: N/A
*/

void lstInsert
    (
    FAST LIST *pList,   /* pointer to list descriptor */
    FAST NODE *pPrev,   /* pointer to node after which to insert */
    FAST NODE *pNode    /* pointer to node to be inserted */
    )
    {
    FAST NODE *pNext;

    if (pPrev == NULL)
	{				/* new node is to be first in list */
	pNext = pList->HEAD;
	pList->HEAD = pNode;
	}
    else
	{				/* make prev node point fwd to new */
	pNext = pPrev->next;
	pPrev->next = pNode;
	}

    if (pNext == NULL)
	pList->TAIL = pNode;		/* new node is to be last in list */
    else
	pNext->previous = pNode;	/* make next node point back to new */


    /* set pointers in new node, and update node count */

    pNode->next		= pNext;
    pNode->previous	= pPrev;

    pList->count++;
    }
/************************************************************************
*
* lstLast - find the last node in a list
*
* This routine finds the last node in a list.
*
* RETURNS
* A pointer to the last node in the list, or
* NULL if the list is empty.
*/

NODE *lstLast
    (
    LIST *pList         /* pointer to list descriptor */
    )
    {
    return (pList->TAIL);
    }
/************************************************************************
*
* lstNext - find the next node in a list
*
* This routine locates the node immediately following a specified node.
*
* RETURNS:
* A pointer to the next node in the list, or
* NULL if there is no next node.
*/

NODE *lstNext
    (
    NODE *pNode         /* ptr to node whose successor is to be found */
    )
    {
    return (pNode->next);
    }
/************************************************************************
*
* lstNth - find the Nth node in a list
*
* This routine returns a pointer to the node specified by a number <nodenum>
* where the first node in the list is numbered 1.
* Note that the search is optimized by searching forward from the beginning
* if the node is closer to the head, and searching back from the end
* if it is closer to the tail.
*
* RETURNS:
* A pointer to the Nth node, or
* NULL if there is no Nth node.
*/

NODE *lstNth
    (
    FAST LIST *pList,           /* pointer to list descriptor */
    FAST int nodenum            /* number of node to be found */
    )
    {
    FAST NODE *pNode;

    /* verify node number is in list */

    if ((nodenum < 1) || (nodenum > pList->count))
	return (NULL);


    /* if nodenum is less than half way, look forward from beginning;
	otherwise look back from end */

    if (nodenum < (pList->count >> 1))
	{
	pNode = pList->HEAD;

	while (--nodenum > 0)
	    pNode = pNode->next;
	}

    else
	{
	nodenum -= pList->count;
	pNode = pList->TAIL;

	while (nodenum++ < 0)
	    pNode = pNode->previous;
	}

    return (pNode);
    }
/************************************************************************
*
* lstPrevious - find the previous node in a list
*
* This routine locates the node immediately preceding the node pointed to 
* by <pNode>.
*
* RETURNS:
* A pointer to the previous node in the list, or
* NULL if there is no previous node.
*/

NODE *lstPrevious
    (
    NODE *pNode         /* ptr to node whose predecessor is to be found */
    )
    {
    return (pNode->previous);
    }
/************************************************************************
*
* lstNStep - find a list node <nStep> steps away from a specified node
*
* This routine locates the node <nStep> steps away in either direction from 
* a specified node.  If <nStep> is positive, it steps toward the tail.  If
* <nStep> is negative, it steps toward the head.  If the number of steps is
* out of range, NULL is returned.
*
* RETURNS:
* A pointer to the node <nStep> steps away, or
* NULL if the node is out of range.
*/

NODE *lstNStep
    (
    FAST NODE *pNode,           /* the known node */
    int nStep                   /* number of steps away to find */
    )
    {
    int i;

    for (i = 0; i < abs (nStep); i++)
	{
	if (nStep < 0)
	    pNode = pNode->previous;
	else if (nStep > 0)
	    pNode = pNode->next;
	if (pNode == NULL)
	    break;
	}
    return (pNode);
    }

/************************************************************************
*
* lstFind - find a node in a list
*
* This routine returns the node number of a specified node (the 
* first node is 1).
*
* RETURNS:
* The node number, or
* ERROR if the node is not found.
*/

int lstFind
    (
    LIST *pList,                /* list in which to search */
    FAST NODE *pNode            /* pointer to node to search for */
    )
    {

    FAST NODE *pNextNode;
    FAST int index = 1;

    pNextNode = lstFirst (pList);

    while ((pNextNode != NULL) && (pNextNode != pNode))
	{
	index++;
	pNextNode = lstNext (pNextNode);
	}

    if (pNextNode == NULL)
	return (ERROR);
    else
	return (index);
    }

/************************************************************************
*
* lstFree - free up a list
*
* This routine turns any list into an empty list.
* It also frees up memory used for nodes.
*
* RETURNS: N/A
*
* SEE ALSO: free()
*/

void lstFree
    (
    LIST *pList         /* list for which to free all nodes */
    )
    {
    NODE *p1, *p2;

    if (pList->count > 0)
	{
	p1 = pList->HEAD;
	while (p1 != NULL)
	    {
	    p2 = p1->next;
	    free ((char *)p1);
	    p1 = p2;
	    }
	pList->count = 0;
	pList->HEAD = pList->TAIL = NULL;
	}
    }
