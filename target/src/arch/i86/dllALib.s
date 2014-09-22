/* dllALib.s - i80x86 assembly doubly linked list manipulation */

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
doubly linked list.  The user supplies a list descriptor (type DL_LIST)
that will contain pointers to the first and last nodes in the list.
The nodes in the list can be any user-defined structure, but they must reserve
space for a pointer as their first element.  The forward chain is terminated
with a NULL pointer.

This library in conjunction with dllLib.c, and the macros defined in dllLib.h,
provide a reduced version of the routines offered in lstLib(1).  For
efficiency, the count field has been eliminated, and enqueueing and dequeueing
functions have been hand optimized.

.ne 16
NON-EMPTY LIST:
.CS

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
*/

#define _ASMLANGUAGE
#include "vxWorks.h"
#include "asm.h"


	.data
	.globl	FUNC(copyright_wind_river)
	.long	FUNC(copyright_wind_river)
	

#ifndef PORTABLE

	/* internals */

	.globl	GTEXT(dllInsert)
	.globl	GTEXT(dllAdd)
	.globl	GTEXT(dllRemove)
	.globl	GTEXT(dllGet)


	.text
	.balign 16

/*******************************************************************************
*
* dllInsert - insert node in list after specified node
*
* This routine inserts the specified node in the specified list.
* The new node is placed following the specified 'previous' node in the list.
* If the specified previous node is NULL, the node is inserted at the head
* of the list.

* void dllInsert (pList, pPrev, pNode)
*    FAST DL_LIST *pList;	/* pointer to list descriptor *
*    FAST DL_NODE *pPrev;	/* pointer to node after which to insert *
*    FAST DL_NODE *pNode;	/* pointer to node to be inserted *

* INTERNAL
    {
    FAST DL_NODE *pNext;

    if (pPrev == NULL)
	{				/* new node is to be first in list *
	pNext = pList->head;
	pList->head = pNode;
	}
    else
	{				/* make prev node point fwd to new *
	pNext = pPrev->next;
	pPrev->next = pNode;
	}

    if (pNext == NULL)
	pList->tail = pNode;		/* new node is to be last in list *
    else
	pNext->previous = pNode;	/* make next node point back to new *

    pNode->next		= pNext;   	/* set pointers in new node *
    pNode->previous	= pPrev;
    }

*/

FUNC_LABEL(dllInsert)
	pushl	%ebx
	pushl	%esi
	
	movl	SP_ARG1+8(%esp),%eax	/* pList into %eax */
	movl	SP_ARG2+8(%esp),%edx	/* pPrev into %edx */
	movl	SP_ARG3+8(%esp),%ecx	/* pNode into %ecx */

	movl	%eax,%esi		/* %esi = pList */
	cmpl	$0,%edx
	je	dllInsert1
	movl	%edx,%esi		/* %esi = pPrev */

dllInsert1:
	movl	(%esi),%ebx		/* %ebx = (%esi)->next */
	movl	%ecx,(%esi)		/* (%esi)->next = pNode */

dllInsert2:
	movl	%eax,%esi		/* %esi = pList */
	cmpl	$0,%ebx			/* (pNext == NULL)? */
	je	dllInsert3
	movl	%ebx,%esi		/* %esi = pPrev */

dllInsert3:
	movl	%ecx,4(%esi)		/* (%esi)->previous = pNode */
	movl	%ebx,(%ecx)		/* pNode->next     = pNext */
	movl	%edx,4(%ecx)		/* pNode->previous = pPrev */

	popl	%esi
	popl	%ebx
	ret

/*******************************************************************************
*
* dllAdd - add node to end of list
*
* This routine adds the specified node to the end of the specified list.

* void dllAdd (pList, pNode)
*    DL_LIST *pList;	/* pointer to list descriptor *
*    DL_NODE *pNode;	/* pointer to node to be added *

* INTERNAL

    {
    dllInsert (pList, pList->tail, pNode);
    }

*/

	.balign 16,0x90
FUNC_LABEL(dllAdd)
	movl	SP_ARG1(%esp),%eax	/* %eax = pList */
	movl	SP_ARG2(%esp),%ecx	/* %ecx = pNode */
	movl	4(%eax),%edx		/* %edx = pList->tail = pPrev */

	movl	%ecx,4(%eax)		/* pList->tail     = pNode */
	movl	%edx,4(%ecx)		/* pNode->previous = pPrev/NULL */
	movl	$0,(%ecx)		/* pNode->next     = NULL */
	cmpl	$0,%edx
	je	dllAdd1
	movl	%ecx,(%edx)		/* pPrev->next	   = pNode */
	ret

	.balign 16,0x90
dllAdd1:
	movl	%ecx,(%eax)		/* pList->head     = pNode */
	ret

/*******************************************************************************
*
* dllRemove - remove specified node in list
*
* Remove the specified node in the doubly linked list.

* void dllRemove (pList, pNode)
*    DL_LIST *pList;		/* pointer to list descriptor *
*    DL_NODE *pNode;		/* pointer to node to be deleted *

* INTERNAL

    {
    if (pNode->previous == NULL)
	pList->head = pNode->next;
    else
	pNode->previous->next = pNode->next;

    if (pNode->next == NULL)
	pList->tail = pNode->previous;
    else
	pNode->next->previous = pNode->previous;
    }

*/

	.balign 16,0x90
FUNC_LABEL(dllRemove)
	pushl	%ebx
	pushl	%esi

	movl	SP_ARG1+8(%esp),%edx	/* %edx = pList */
	movl	SP_ARG2+8(%esp),%eax	/* %eax = pNode */
	movl	4(%eax),%ecx		/* %ecx = pNode->previous */

	movl	%edx,%ebx		/* %ebx = pList */
	cmpl	$0,%ecx			/* (pNode->previous == NULL)? */
	je	dllRemove1
	movl	%ecx,%ebx		/* %ebx = pNode->previous */

dllRemove1:
	movl	(%eax),%esi		/* (%ebx) = pNode->next */
	movl	%esi,(%ebx)
	
	movl	%edx,%ebx		/* %ebx = pList */
	cmpl	$0,%esi			/* (pNode->next == NULL)? */
	je	dllRemove2
	movl	%esi,%ebx		/* %ebx = pNode->next */

dllRemove2:
	movl	%ecx,4(%ebx)		/* 4(%ebx) = pNode->previous */

	popl	%esi
	popl	%ebx
	ret

/*******************************************************************************
*
* dllGet - get (delete and return) first node from list
*
* This routine gets the first node from the specified list, deletes the node
* from the list, and returns a pointer to the node gotten.
*
* RETURNS
*	Pointer to the node gotten, or
*	NULL if the list is empty.

* DL_NODE *dllGet (pList)
*    FAST DL_LIST *pList;	/* pointer to list from which to get node *

* INTERNAL

    {
    FAST DL_NODE *pNode = pList->head;

    if (pNode != NULL)
	dllRemove (pList, pNode);

    return (pNode);
    }

*/

	.balign 16,0x90
FUNC_LABEL(dllGet)
	movl	SP_ARG1(%esp),%edx	/* %edx = pList */
	movl	(%edx),%eax		/* %eax = pList->head(is pNode) */
	cmpl	$0,%eax			/* if (%eax == NULL) we're done */
	je	dllGet2

	movl	(%eax),%ecx		/* %ecx = pNode->next */
	movl	%ecx,(%edx)		/* (%edx) = %ecx */
	cmpl	$0,%ecx			/* (pNode->next == NULL)? */
	je	dllGet1

	movl	%ecx,%edx		/* %edx = pNode->next */

dllGet1:
	movl	$0,4(%edx)		/* 4(%edx) = NULL */

dllGet2:
	ret

#endif	/* (!PORTABLE) */
