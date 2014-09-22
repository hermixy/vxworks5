/* distObjLib.c - distributed objects library (VxFusion option) */

/* Copyright 1999 - 2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,23oct01,jws  fix complier warnings (SPR 71117)
01b,24may99,drm  added vxfusion prefix to VxFusion related includes
01a,13oct97,ur   written.
*/

/*
DESCRIPTION
This library contains utility functions used internally by VxFusion.

AVAILABILITY
This module is distributed as a component of the unbundled distributed
message queues option, VxFusion.

*/

#include "vxWorks.h"
#if defined (DIST_OBJ_REPORT) || defined (DIST_DIAGNOSTIC)
#include "stdio.h"
#endif
#include "stdlib.h"
#include "semLib.h"
#include "private/semLibP.h"
#include "hashLib.h"
#include "vxfusion/private/distObjLibP.h"

#define UNUSED_ARG(x)  if(sizeof(x)) {} /* to suppress compiler warnings */

#define KEY            13
#define KEY_CMP        0    /* unused */

LOCAL DIST_INQ_ID    distInqIdNext = 0;
LOCAL SEMAPHORE      distInqLock;
LOCAL HASH_ID        distInqHashId;
LOCAL BOOL           distInqInstalled = FALSE;

LOCAL BOOL    distInqHCmpId (DIST_INQ *pMatchNode, DIST_INQ *pHNode,
                             int unused);
LOCAL BOOL    distInqHFuncId (int elements, DIST_INQ *pHNode, int divisor);

/***************************************************************************
*
* distObjNodeGet - allocate space for a distributed object node (VxFusion option)
*
* This routine allocates space for a distributed object node.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: A pointer to a node, or NULL.
*
* NOMANUAL
*/

DIST_OBJ_NODE * distObjNodeGet (void)
    {
    return ((DIST_OBJ_NODE *) malloc (sizeof (DIST_OBJ_NODE)));
    }

/***************************************************************************
*
* distObjNodeFree - free distributed object node space (VxFusion option)
*
* This routine frees distributed object node space.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: N/A
* 
* NOMANUAL
*/

void distObjNodeFree
    (
    DIST_OBJ_NODE * pNode  /* node to free */
    )
    {
    free (pNode);
    }

/***************************************************************************
*
* distInqInit - initialize Inq functionality (VxFusion option)
*
* This routine initializes Inq functionality.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: OK, if successful.
*
* NOMANUAL
*/

STATUS distInqInit
    (
    int        hashTblSzLog2   /* allocate 2^^hashTblSzLog2 entries */
    )
    {
    if (distInqInstalled)
        return (OK);
    
    distInqHashId = hashTblCreate (hashTblSzLog2, distInqHCmpId,
                                   distInqHFuncId, KEY);
    if (distInqHashId == NULL)
        return (ERROR);

    distInqLockInit();
    
    distInqInstalled = TRUE;
    return (OK);
    }

/***************************************************************************
*
* distInqRegister - register an Inq (VxFusion option)
*
* This routine registers an Inq.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: An Inq ID.
* 
* NOMANUAL
*/

DIST_INQ_ID distInqRegister
    (
    DIST_INQ * pInqRegister  /* Inq to register */
    )
    {
    DIST_INQ_ID    inqIdThis;
    DIST_INQ       inqMatchNode;

    distInqLock();

    do
        {
        inqMatchNode.inqId = (inqIdThis = distInqIdNext++);
        }
    while (hashTblFind (distInqHashId, (HASH_NODE *) &inqMatchNode, KEY_CMP));
    
    pInqRegister->inqId = inqIdThis;
    hashTblPut (distInqHashId, (HASH_NODE *) pInqRegister);

    distInqUnlock();
    return (inqIdThis);
    }

/***************************************************************************
*
* distInqGetId - Get the Inq ID of an Inq (VxFusion option)
*
* This routine gets the Inq ID of an Inq.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: An Inq ID.
* 
* NOMANUAL
*/

DIST_INQ_ID distInqGetId
    (
    DIST_INQ * pInqGetIdOf   /* Inq whose ID to get */
    )
    {
    return (pInqGetIdOf->inqId);
    }

/***************************************************************************
*
* distInqFind - given an Inq ID, get the Inq (VxFusion option)
*
* Given an Inq ID, this routine gets the Inq.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: An Inq.
*
* NOMANUAL
*/

DIST_INQ * distInqFind
    (
    DIST_INQ_ID InqIdFind      /* Inq ID */
    )
    {
    DIST_INQ     inqMatchNode;
    DIST_INQ *   pInq;

    inqMatchNode.inqId = InqIdFind;

    distInqLock();

    pInq = (DIST_INQ *) hashTblFind (distInqHashId,
                                     (HASH_NODE *) &inqMatchNode,
                                     KEY_CMP);

    distInqUnlock();
    return (pInq);
    }

/***************************************************************************
*
* distInqCancel - cancel (unregister) an Inq (VxFusion option)
*
* This routine cancels (unregisters) an Inq.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void distInqCancel
    (
    DIST_INQ * pInqCancel   /* Inq to cancel */
    )
    {

    distInqLock();
    hashTblRemove (distInqHashId, (HASH_NODE *) pInqCancel);
    distInqUnlock();

    }

/***************************************************************************
*
* distInqHCmpId - hash compare function for Inq IDs (VxFusion option)
*
* This routine is the hash compare function for Inq IDs.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: TRUE, if the IDs match.
*
* NOMANUAL
*/

LOCAL BOOL distInqHCmpId
    (
    DIST_INQ * pMatchNode,   /* the first Inq */
    DIST_INQ * pHNode,       /* the second Inq */
    int        unused
    )
    {
    
    UNUSED_ARG(unused);
        
    if (pMatchNode->inqId == pHNode->inqId)
        {
        return (TRUE);
        }

    return (FALSE);
    }

/***************************************************************************
*
* distInqHFuncId - hash function for Inq IDs (VxFusion option)
*
* This routine is the hash function for Inq IDs.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: A hash index.
*
* NOMANUAL
*/

LOCAL int distInqHFuncId
    (
    int        elements,  /* size of hash table */
    DIST_INQ * pHNode,    /* Inq to hash */
    int        divisor    /* used by hash computation */
    )
    {
    return ((pHNode->inqId % divisor) & (elements - 1));
    }

