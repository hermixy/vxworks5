/* qPriBMapALib.s - i80x86 optimized bit-mapped priority queue internals */

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
This module contains internals to the VxWorks kernel.
These routines have been coded in assembler because they have been optimized
for performance.

INTERNAL
The C code versions of these routines can be found in qPriBMapLib.c.
Unlike 68K, Highest priority bit is LSB in the meta-map and bit-map.
.ne 36
68K:
.CS

    priority = 255 - priority
    * is highest priority (255)
    $ is lowest priority (0)

			31                                0
	metaBMap	*------- -------- -------- -------$

			7      0
	bMap[0]		-------$
	  :
	  :
	bMap[31]	*-------


			 
	listArray[0]	-----------          ---------    ---------
			$  pNext  ---------> | pNext ---> | pNext ---> 0
			-----------          ---------    ---------
			$  pPrev  --+   0 <--- pPrev | <--- pPrev | <---+
			----------- |        ---------    ---------     |
				    |                                   |
				    +-----------------------------------+
	  :
	  :
	listArray[255]	-----------          ---------
			*  pNext  ---------> | pNext ---> 0
			-----------          ---------
			*  pPrev  --+   0 <--- pPrev | <---+
			----------- |        ---------     |
				    |                      |
				    +----------------------+

.CE
.ne 36
i80x86:
.CS

    priority = priority
    * is highest priority (0)
    $ is lowest priority (255)

			0                                31
	metaBMap	*------- -------- -------- -------$

			0      7
	bMap[0]		*-------
	  :
	  :
	bMap[31]	-------$


			 
	listArray[0]	-----------          ---------    ---------
			*  pNext  ---------> | pNext ---> | pNext ---> 0
			-----------          ---------    ---------
			*  pPrev  --+   0 <--- pPrev | <--- pPrev | <---+
			----------- |        ---------    ---------     |
				    |                                   |
				    +-----------------------------------+
	  :
	  :
	listArray[255]	-----------          ---------
			$  pNext  ---------> | pNext ---> 0
			-----------          ---------
			$  pPrev  --+   0 <--- pPrev | <---+
			----------- |        ---------     |
				    |                      |
				    +----------------------+

.CE
*/

#define _ASMLANGUAGE
#include "vxWorks.h"
#include "asm.h"
#include "qPriNode.h"


	.data
	.globl	FUNC(copyright_wind_river)
	.long	FUNC(copyright_wind_river)
	

#ifndef PORTABLE

	/* internals */

	.globl	GTEXT(qPriBMapPut)
	.globl	GTEXT(qPriBMapGet)
	.globl	GTEXT(qPriBMapRemove)


	.text
	.balign 16

/*******************************************************************************
*
* qPriBMapPut - insert the specified TCB into the ready queue
*

*void qPriBMapPut (pQPriBMapHead, pQPriNode, key)
*    Q_PRI_BMAP_HEAD	*pQPriBMapHead;
*    Q_PRI_NODE		*pQPriNode;
*    int		key;


*/

FUNC_LABEL(qPriBMapPut)
	pushl	%ebx
	pushl	%esi
	pushl	%edi

	movl	SP_ARG1+12(%esp),%edx	/* %edx = ARG1 (pMHead) */
	movl	SP_ARG2+12(%esp),%ecx	/* %ecx = ARG2 (pPriNode) */
	movl	SP_ARG3+12(%esp),%eax	/* %eax = ARG3 (key) */

	movl	(%edx),%ebx		/* %ebx = highest node ready */
	cmpl	$0,%ebx
	je	qPriBMap0
	cmpl	Q_PRI_NODE_KEY(%ebx),%eax /* is eax higher priority? */
	jge	qPriBMap1
qPriBMap0:
	movl	%ecx,(%edx)		/* pPriNode is highest priority */
qPriBMap1:
	movl	%eax,Q_PRI_NODE_KEY(%ecx) /* move key into pPriNode */


/* qPriBMapMapSet - set the bits in the bit-map for the specified priority
 * %eax = priority
 * returns void
 */
	    movl    4(%edx),%esi	/* %esi = pMList (metaMap) */
	    btsl    %eax,4(%esi)	/* set %eax bit # in bit-map */
	    leal    0x24(%esi,%eax,8),%ebx /* %ebx = pList */
	    shrl    $3,%eax		/* %eax = top five bits of %eax */
	    btsl    %eax,(%esi)		/* set %eax bit # of meta-map */

	movl	4(%ebx),%edi		/* %edi = pList->tail = pPrev */


/* dllAdd - add node to end of list
 * %ebx = pList
 * %edi = pLastNode
 * %ecx = pNode
 * returns void
 */

	    movl    %edi,%esi		/* %esi = pPrev->next */
	    cmpl    $0,%edi
	    jne	    qPriBMap2
	    movl    %ebx,%esi		/* %esi = pList->head */
qPriBMap2:
	    movl    %ecx,(%esi)		/* (%esi) = pNode */
	    movl    %ecx,4(%ebx)	/* pList->tail = pNode */
	    movl    $0,(%ecx)		/* pNode->next     = pNext */
	    movl    %edi,4(%ecx)	/* pNode->previous = pPrev */

	popl	%edi
	popl	%esi
	popl	%ebx
	ret

/*******************************************************************************
*
* qPriBMapGet -
*

*Q_PRI_NODE *qPriBMapGet (pQPriBMapHead)
*    Q_PRI_BMAP_HEAD *pQPriBMapHead;

*/

	.balign 16,0x90
FUNC_LABEL(qPriBMapGet)
	movl	SP_ARG1(%esp),%edx	/* %edx = pMHead */
	pushl	(%edx)
	cmpl	$0,(%edx)
	je	qPriBMapG1		/* if highNode is NULL we're done */
	pushl	%edx			/* push pMHead */
	call	FUNC(qPriBMapRemove)		/* delete the node */
	addl	$4,%esp			/* clean up second argument */
qPriBMapG1:
	popl	%eax			/* return node */
	ret
	
/*******************************************************************************
*
* qPriBMapRemove
*

*void qPriBMapRemove (pQPriBMapHead, pQPriNode)
*    Q_PRI_BMAP_HEAD *pQPriBMapHead;
*    Q_PRI_NODE *pQPriNode;

*/

	.balign 16,0x90
FUNC_LABEL(qPriBMapRemove)
	pushl	%ebx
	pushl	%esi
	pushl	%edi

	movl	SP_ARG1+12(%esp),%edx		/* %edx = ARG1 (pMHead) */
	movl	SP_ARG2+12(%esp),%ecx		/* %ecx = ARG2 (pPriNode) */

	movl	Q_PRI_NODE_KEY(%ecx),%eax	/* %eax = key */
	movl	4(%edx),%ebx			/* %ebx = pMList (metaMap) */
	leal	0x24(%ebx,%eax,8),%esi		/* %esi = pList */

/* dllRemove - delete a node from a doubly linked list
 * %esi = pList
 * %ecx = pNode
 * returns void
 */
	    movl    4(%ecx),%ebx	/* %ebx = pNode->previous */
	    movl    %ebx,%edi		/* %edi = pNode->previous */
	    cmpl    $0,%ebx
	    jne	    qPriBMapR1
	    movl    %esi,%edi		/* %edi = pList */
qPriBMapR1:
	    pushl   (%ecx)
	    popl    (%edi)		/* pNode->next into (%edi) */
	    movl    (%ecx),%edi		/* %edi = pNode->next */
	    cmpl    $0,%edi		/* (pNode->next == NULL)? */
	    jne	    qPriBMapR3
	    movl    %esi,%edi		/* %edi = pList */
qPriBMapR3:
	    movl    %ebx,4(%edi)	/* pNode->previous into 4(%edi) */
	    
	movl	(%esi),%edi
	cmpl	$0,%edi			/* if (pList->head == NULL)         */
	je	clearMaps		/*     then we clear maps           */
	cmpl	(%edx),%ecx		/* if not deleting highest priority */
	jne	qPriBMapDExit		/*     then we are done             */
	movl	%edi,(%edx)		/* update the highest priority task */
	jmp	qPriBMapDExit


	.balign 16,0x90
clearMaps:

/* qPriBMapMapClear - clear the bits in the bit-maps for the specified priority
 * %eax = priority,
 * %ebx = &qPriBMapMetaMap,
 * returns void
 */
	    movl    4(%edx),%ebx	/* %ebx = pMList (meta-map) */
	    btrl    %eax,4(%ebx)	/* clear bit in bit-map */
	    shrl    $3,%eax		/* %eax = top five bits of %eax */
	    cmpb    $0,4(%ebx,%eax,1)
	    jne	    qPriBMapNoMeta	/* if not zero, we're done */
	    btrl    %eax,(%ebx)		/* clear bit in meta-map too */

qPriBMapNoMeta:

	cmpl	(%edx),%ecx		/* have we deleted highest priority */
	jne	qPriBMapDExit

/* qPriBMapMapHigh - return highest priority task
 * %ebx = &qPriBMapMetaMap,
 * returns priority in d0
 */

	    movl    (%ebx),%eax
	    bsfl    %eax,%esi		/* find the top meta priority */
	    je	    qPriBMapR11		/* if no bit is set, it is ERROR */
	    movzbl  4(%ebx,%esi,1),%eax
	    bsfl    %eax,%eax		/* find the top bitmap priority */
	    je	    qPriBMapR11		/* if no bit is set, it is ERROR */
	    shll    $3,%esi		/* multiply meta priority by 8 */
	    orl	    %esi,%eax		/* add to the priority */
	    andl    $0xff,%eax

qPriBMapR10:
	movl	0x24(%ebx,%eax,8),%eax	/* get highest task into highNode */
	movl	%eax,(%edx)

qPriBMapDExit:
	popl	%edi
	popl	%esi
	popl	%ebx
	ret

	.balign 16,0x90
qPriBMapR11:				/* It should not happen */
	xorl	%eax,%eax
	jmp	qPriBMapR10

#endif	/* !PORTABLE */
