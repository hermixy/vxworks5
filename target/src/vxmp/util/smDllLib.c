/* smDllLib.c - shared memory doubly linked list subroutine library (VxMP Option) */

/* Copyright 1984-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01f,06may02,mas  cache flush and volatile fix (SPR 68334); bridge flush fix
		 (SPR 68844)
01e,24oct01,mas  fixed diab warnings (SPR 71120); doc update (SPR 71149)
01d,29jan93,pme  added little endian support
01c,02oct92,pme  added SPARC support.
01b,27jul92,pme  cleanup, documentation.
01a,19jul92,pme  optimized smDllGet.
                 written from dllLib.
*/


/*
DESCRIPTION
This subroutine library supports the creation and maintenance of shared memory
doubly linked lists.

There are no user callable routines.

AVAILABILITY
This module is distributed as a component of the unbundled shared memory
support option, VxMP.

INTERNAL
The user supplies a list descriptor (type SM_DL_LIST)
that will contain pointers to the first and last nodes in the list stored
in shared memory.
The nodes in the list can be any user-defined structure, but they must reserve
space for a pointer as their first element.  The forward chain is terminated
with a NULL pointer.

All pointers stored in the list are global addresses of nodes (offset in
shared memory pool) thus each CPU must convert these addresses to their 
local values to manipulate the list. Parameters passed to smDll functions 
are always local address of nodes or lists.

\is
\i `NON-EMPTY LIST:'
\cs

   ---------             --------          --------
   | head--------------->| next----------->| next---------
   |       |             |      |          |      |      |
   |       |        ------ prev |<---------- prev |      |
   |       |       |     |      |          |      |      |
   | tail------    |     | ...  |    ----->| ...  |      |
   |-------|  |    v                 |                   v
              |  -----               |                 -----
              |   ---                |                  ---
              |    -                 |                   -
              ------------------------

\ce

\i `EMPTY LIST:'
\cs

	-----------
        |  head------------------
        |         |             |
        |  tail----------       |
        |         |     v       v
        |         |   -----   -----
        -----------    ---     ---
                        -	-

\ce
\ie
INCLUDE FILE: smDllLib.h

SEE ALSO : smLib

NOROUTINES
*/


/* LINTLIBRARY */

#include "vxWorks.h"
#include "cacheLib.h"
#include "smObjLib.h"
#include "smDllLib.h"
#include "stdlib.h"
#include "netinet/in.h"

/*****************************************************************************
*
* smDllInit - initialize shared memory doubly linked list descriptor
*
* Initialize the specified list to an empty list.
*
* RETURNS: OK
*
* NOMANUAL
*/

STATUS smDllInit 
    (
    SM_DL_LIST * pList  /* pointer to list descriptor to be initialized */
    )
    {
    SM_DL_LIST volatile * pListv = (SM_DL_LIST volatile *) pList;
    void *                temp;   /* temp storage */

    pListv->head = NULL;
    pListv->tail = NULL;

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    temp = (void *) pListv->head;               /* BRIDGE FLUSH  [SPR 68334] */

    return (OK);
    }

/*****************************************************************************
*
* smDllTerminate - terminate shared memory doubly linked list head
*
* Terminate the specified list without deallocating associated memory.
*
* RETURNS: OK
*
* NOMANUAL
*/

STATUS smDllTerminate 
    (
    SM_DL_LIST * pList     /* pointer to list head to be terminated */
    )
    {
    /* appease the Diab gods (SPR 71120) */

    if (pList == NULL)
        ;

    return (OK);
    }


/*****************************************************************************
*
* smDllInsert - insert node in shared memory list after specified node
*
* This routine inserts the specified node in the specified shared list.
* The new node is placed following the specified 'previous' node in the list.
* If the specified previous node is NULL, the node is inserted at the head
* of the list.
*
* RETURNS: N/A
* 
* NOMANUAL
*/

void smDllInsert 
    (
    SM_DL_LIST * pList,       /* pointer to list descriptor */
    SM_DL_NODE * pPrev,       /* pointer to node after which to insert */
    SM_DL_NODE * pNode        /* pointer to node to be inserted */
    )
    {
    SM_DL_NODE volatile * pNext;
    SM_DL_LIST volatile * pListv = (SM_DL_LIST volatile *) pList;
    SM_DL_NODE volatile * pPrevv = (SM_DL_NODE volatile *) pPrev;
    SM_DL_NODE volatile * pNodev = (SM_DL_NODE volatile *) pNode;
    void *                temp;

    if (pPrevv == NULL)
        {
	pPrevv = LOC_NULL;		/* convert to local null pointer */
        }

    if (pPrevv == LOC_NULL)		/* new node is to be first in list */
	{
        CACHE_PIPE_FLUSH ();                    /* CACHE FLUSH   [SPR 68334] */
        temp = (void *) pListv->head;           /* PCI bridge bug [SPR 68844]*/

	pNext = (SM_DL_NODE volatile *) \
                                  GLOB_TO_LOC_ADRS (ntohl((int) pListv->head));

	/* store global adrs of first node */

	pListv->head = (SM_DL_NODE *) htonl (LOC_TO_GLOB_ADRS (pNode));
	}
    else				/* make prev node point fwd to new */
	{
        CACHE_PIPE_FLUSH ();                    /* CACHE FLUSH   [SPR 68334] */
        temp = (void *) pPrevv->next;           /* PCI bridge bug [SPR 68844]*/

	pNext = (SM_DL_NODE volatile *) \
                                  GLOB_TO_LOC_ADRS (ntohl((int) pPrevv->next));
	pPrevv->next = (SM_DL_NODE *) htonl (LOC_TO_GLOB_ADRS (pNode));
	}

    if (pNext == LOC_NULL)		/* new node is to be last in list */
        {
	pListv->tail    = (SM_DL_NODE *) htonl (LOC_TO_GLOB_ADRS (pNode));
        }
    else
        {
	pNext->previous = (SM_DL_NODE *) htonl (LOC_TO_GLOB_ADRS (pNode));
        }
       
    /* set pointers in new node */

    pNodev->next     = (SM_DL_NODE *) htonl (LOC_TO_GLOB_ADRS (pNext));
    pNodev->previous = (SM_DL_NODE *) htonl (LOC_TO_GLOB_ADRS (pPrevv));	

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    temp = (void *) pNodev->next;               /* BRIDGE FLUSH  [SPR 68334] */
    }

/*****************************************************************************
*
* smDllAdd - add node to end of shared memory list
*
* This routine adds the specified node to the end of the specified list.
*
* RETURNS: N/A
* 
* NOMANUAL
*/

void smDllAdd 
    (
    SM_DL_LIST * pList,     /* pointer to list descriptor */
    SM_DL_NODE * pNode      /* pointer to node to be added */
    )
    {
    SM_DL_LIST volatile * pListv = (SM_DL_LIST volatile *) pList;
    void *                temp;

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    temp = (void *) pListv->tail;               /* PCI bridge bug [SPR 68844]*/

    smDllInsert (pList, 
		 (SM_DL_NODE *) GLOB_TO_LOC_ADRS (ntohl ((int) pListv->tail)),
		 pNode);
    }

/*****************************************************************************
*
* smDllRemove - remove specified node in shared memory list
*
* Remove the specified node in the shared memory doubly linked list.
*
* RETURNS: N/A
* 
* NOMANUAL
*/

void smDllRemove 
    (
    SM_DL_LIST * pList,       /* pointer to list descriptor */
    SM_DL_NODE * pNode        /* pointer to node to be deleted */
    )
    {
    SM_DL_NODE volatile * temp;
    SM_DL_LIST volatile * pListv = (SM_DL_LIST volatile *) pList;
    SM_DL_NODE volatile * pNodev = (SM_DL_NODE volatile *) pNode;
    void *                tmp;

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    tmp = (void *) pNodev->previous;            /* PCI bridge bug [SPR 68844]*/

    if (pNodev->previous == NULL)
        {
	pListv->head = pNodev->next;       /* removing first node */
        }
    else
	{
	temp = (SM_DL_NODE volatile *) \
                             GLOB_TO_LOC_ADRS (ntohl ((int) pNodev->previous));
	temp->next = pNodev->next;
	}

    if (pNodev->next == NULL)
        {
	pListv->tail = pNodev->previous;   /* removing last node */
        }
    else
	{
	temp = (SM_DL_NODE volatile *) \
                             GLOB_TO_LOC_ADRS (ntohl ((int) pNodev->next));
	temp->previous = pNodev->previous;
	}

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    tmp = (void *) pNodev->next;                /* BRIDGE FLUSH  [SPR 68334] */
    }

/*****************************************************************************
*
* smDllGet - get (delete and return) first node from shared memory list
*
* This routine gets the first node from the specified list, deletes the node
* from the list, and returns a pointer to the node gotten.
*
* RETURNS: Pointer to the node gotten, or LOC_NULL if the list is empty.
*
* NOMANUAL
*/

SM_DL_NODE * smDllGet 
    (
    SM_DL_LIST * pList         /* pointer to list from which to get node */
    )
    {
    SM_DL_NODE volatile * tmp;
    SM_DL_NODE volatile * pNode;
    SM_DL_LIST volatile * pListv = (SM_DL_LIST volatile *) pList;
    void *                temp;
    
    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    temp = (void *) pListv->head;               /* PCI bridge bug [SPR 68844]*/

    pNode = (SM_DL_NODE volatile *) \
                                 GLOB_TO_LOC_ADRS (ntohl ((int) pListv->head));

    if (pNode != LOC_NULL)		/* is list empty ? */
	{
	pListv->head = pNode->next;	/* make next node be first */

    	if (pNode->next == NULL)	/* is there any next node */
	    {
	    pListv->tail = NULL;        /* no - empty list */
	    }

    	else
	    {
	    tmp = (SM_DL_NODE volatile *) \
                                  GLOB_TO_LOC_ADRS (ntohl ((int) pNode->next));
	    tmp->previous = NULL; 	/* yes - make it first node */
	    }

        CACHE_PIPE_FLUSH ();                    /* CACHE FLUSH   [SPR 68334] */
        temp = (void *) pListv->head;           /* BRIDGE FLUSH  [SPR 68334] */
   	}

    return ((SM_DL_NODE *) pNode);
    }

/*****************************************************************************
*
* smDllCount - report number of nodes in shared memory list
*
* This routine returns the number of nodes in the given list.
*
* CAVEAT
* This routine must actually traverse the list to count the nodes.  Therefore,
* it is assumed the list has been locked to prevent change.
*
* RETURNS: Number of nodes in specified list.
*
* NOMANUAL
*/

int smDllCount 
    (
    SM_DL_LIST * pList      /* pointer to list descriptor */
    )
    {
    SM_DL_NODE volatile * pNode = LOC_NULL;
    int                   count = 0;

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    pNode = (SM_DL_NODE volatile *) SM_DL_FIRST (pList);  /* PCI [SPR 68844] */

    pNode = (SM_DL_NODE volatile *) SM_DL_FIRST (pList);
    while (pNode != LOC_NULL)
	{
	count++;
	pNode = (SM_DL_NODE volatile *) SM_DL_NEXT (pNode);
	}

    return (count);
    }

/*****************************************************************************
*
* smDllEach - call a routine for each node in a linked list
*
* This routine calls a user-supplied <routine> once for each node in the
* linked list.  The <routine> should be declared as follows:
*
* \cs
*  BOOL routine (pNode, arg)
*      SM_DL_NODE volatile * pNode;	/@ pointer to a linked list node    @/
*      int                   arg;       /@ arbitrary user-supplied argument @/
* \ce
*
* The user-supplied <routine> should return TRUE if smDllEach() is to
* continue calling it with the remaining nodes, or FALSE if it is done and
* smDllEach() can exit.
*
* RETURNS: LOC_NULL if traversed whole linked list, or pointer to 
*          SM_DL_NODE that smDllEach ended with.
*
* WARNING:
* This function is not used by shared memory objects and has not been tested.
*
* NOMANUAL
*/

SM_DL_NODE * smDllEach 
    (
    SM_DL_LIST * pList,          /* linked list of nodes to call routine for */
    FUNCPTR      routine,        /* the routine to call for each list node */
    int          routineArg      /* arbitrary user-supplied argument */
    )
    {
    SM_DL_NODE volatile * pNode = LOC_NULL;

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    pNode = (SM_DL_NODE volatile *) SM_DL_FIRST (pList);  /* PCI [SPR 68844] */

    pNode = (SM_DL_NODE volatile *) SM_DL_FIRST (pList);
    while ((pNode != LOC_NULL) && ((* routine) (pNode, routineArg)))
        {
	pNode = (SM_DL_NODE *) SM_DL_NEXT (pNode);
        }

    return ((SM_DL_NODE *) pNode);		/* return node we ended with */
    }

/*****************************************************************************
*
* smDllConcat - concatenate two shared lists
*
* This routine concatenates the second list to the end of the first list.
* The second list is left empty.  Either list (or both) can be
* empty at the beginning of the operation.
*
* All parameters are local addresses of shared list.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void smDllConcat
    (
    SM_DL_LIST * pDstList,        /* Destination list */
    SM_DL_LIST * pAddList         /* List to be added to dstList */
    )
    {
    SM_DL_LIST volatile * pDstListv = (SM_DL_LIST volatile *) pDstList;
    SM_DL_LIST volatile * pAddListv = (SM_DL_LIST volatile *) pAddList;
    BOOL                  temp;

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    temp = SM_DL_EMPTY (pAddListv);             /* PCI bridge bug [SPR 68844]*/

    if SM_DL_EMPTY (pAddListv)         /* nothing to do if AddList is empty */
        {
        return;
        }

    if SM_DL_EMPTY (pDstListv)         /* destination list empty */
        {
        *pDstListv = *pAddListv;
        }
    else
        {
        /* both lists non-empty; update DstList pointers */

        ((SM_DL_NODE*)(GLOB_TO_LOC_ADRS(ntohl((int)pDstListv->tail))))->next
                        	= pAddListv->head;
        ((SM_DL_NODE*)(GLOB_TO_LOC_ADRS(ntohl((int)pAddListv->head))))->previous
				= pDstListv->tail;
        pDstListv->tail         = pAddListv->tail;
        }

    /* make AddList empty */

    pAddListv->head = NULL;
    pAddListv->tail = NULL;

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    temp = SM_DL_EMPTY (pAddListv);             /* PCI bridge bug [SPR 68844]*/
    }

