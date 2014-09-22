/* sllALib.s - i80x86 assembly singly linked list manipulation */

/* Copyright 1984-2001 Wind River Systems, Inc. */
	
/*
modification history
--------------------
01d,22aug01,hdn  added FUNC/FUNC_LABEL, replaced .align with .balign
01c,01jun93,hdn  updated to 5.1.
		  - fixed #else and #endif
		  - changed VOID to void
		  - changed ASMLANGUAGE to _ASMLANGUAGE
		  - changed copyright notice
01b,13oct92,hdn  debugged.
01a,07apr92,hdn  written based on TRON version.
*/

/*
DESCRIPTION
This subroutine library supports the creation and maintenance of a
singly linked list.  The user supplies a list descriptor (type SL_LIST)
that will contain pointers to the first and last nodes in the list.
The nodes in the list can be any user-defined structure, but they must reserve
space for a pointer as their first element.  The forward chain is terminated
with a NULL pointer.

.ne 16
NON-EMPTY LIST:
.CS

   ---------             --------          --------
   | head--------------->| next----------->| next---------
   |       |             |      |          |      |      |
   |       |             |      |          |      |      |
   | tail------          | ...  |    ----->| ...  |      |
   |-------|  |                      |                   v
              |                      |                 -----
              |                      |                  ---
              |                      |                   -
              ------------------------

.CE
.ne 12
EMPTY LIST:
.CS

	-----------
        |  head------------------
        |         |             |
        |  tail----------       |
        |         |     |       v
        |         |   -----   -----
        -----------    ---     ---
                        -	-

.CE

INCLUDE FILE: sllLib.h
*/

#define _ASMLANGUAGE
#include "vxWorks.h"
#include "asm.h"


	.data
	.globl	FUNC(copyright_wind_river)
	.long	FUNC(copyright_wind_river)


#ifndef PORTABLE

	/* internals */

	.globl	GTEXT(sllPutAtHead)	/* put node at head of list */
	.globl	GTEXT(sllPutAtTail)	/* put node at tail of list */
	.globl	GTEXT(sllGet)		/* get and delete node from head */


	.text
	.balign 16

/*******************************************************************************
*
* sllPutAtHead - add node to beginning of list
*
* This routine adds the specified node to the beginning of the specified list.
*
* RETURNS: void
*
* SEE ALSO: sllPutAtTail (2)

* void sllPutAtHead (pList, pNode)
*     SL_LIST *pList;	/* pointer to list descriptor *
*     SL_NODE *pNode;	/* pointer to node to be added *

* INTERNAL

    {
    if ((pNode->next = pList->head) == NULL)
	pList->head = pList->tail = pNode;
    else
	pList->head = pNode;
    }
*/

FUNC_LABEL(sllPutAtHead)
	movl	SP_ARG1(%esp),%eax	/* pList */
	movl	SP_ARG2(%esp),%edx	/* pNode */

	movl	(%eax),%ecx		/* pNode->next = pList->head */
	movl	%ecx,(%edx)
	cmpl	$0,%ecx			/* (pList->head == NULL) ? */
	jne	sllHead1

	movl	%edx,4(%eax)		/* pList->tail = pNode */

sllHead1:
	movl	%edx,(%eax)		/* pList->head = pNode */

	ret

/*******************************************************************************
*
* sllPutAtTail - add node to end of list
*
* This routine adds the specified node to the end of the specified singly
* linked list.
*
* RETURNS: void
*
* SEE ALSO: sllPutAtHead (2)

* void sllPutAtTail (pList, pNode)
*     SL_LIST *pList;	/* pointer to list descriptor *
*     SL_NODE *pNode;	/* pointer to node to be added *

* INTERNAL

    {
    pNode->next = NULL;

    if (pList->head == NULL)
	pList->tail = pList->head = pNode;
    else
	pList->tail->next = pNode;
	pList->tail = pNode;
    }
*/

	.balign 16,0x90
FUNC_LABEL(sllPutAtTail)
	movl	SP_ARG1(%esp),%eax	/* pList */
	movl	SP_ARG2(%esp),%edx	/* pNode */

	movl	$0,(%edx)		/* pNode->next = NULL */
	cmpl	$0,(%eax)		/* if (pList->head == NULL) */
	je	sllTail1
	movl	4(%eax),%ecx		/* ecx = pList->tail */
	movl	%edx,(%ecx)		/* pList->tail->next = pNode */
	movl	%edx,4(%eax)		/* pList->tail = NODE */

	ret

	.balign 16,0x90
sllTail1:
	movl	%edx,(%eax)		/* pList->head = NODE */
	movl	%edx,4(%eax)		/* pList->tail = NODE */

	ret

/*******************************************************************************
*
* sllGet - get (delete and return) first node from list
*
* This routine gets the first node from the specified singly linked list,
* deletes the node from the list, and returns a pointer to the node gotten.
*
* RETURNS
*	Pointer to the node gotten, or
*	NULL if the list is empty.

*SL_NODE *sllGet (pList)
*    FAST SL_LIST *pList;	/* pointer to list from which to get node *

* INTERNAL

    {
    FAST SL_NODE *pNode;

    if ((pNode = pList->head) != NULL)
	pList->head = pNode->next;

    return (pNode);
    }
*/

	.balign 16,0x90
FUNC_LABEL(sllGet)
	movl	SP_ARG1(%esp),%edx	/* pList */

	movl	(%edx),%eax		/* %eax = pList->head */
	cmpl	$0,%eax
	je	sllGet1			/* if pList->head == NULL then done */

	movl	(%eax),%ecx		/* (pNode)->next to pList->head */
	movl	%ecx,(%edx)

sllGet1:
	ret

#endif	/* !PORTABLE */
