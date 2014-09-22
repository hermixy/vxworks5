/* msgQDistGrpShow.c - distributed message queue group show routines (VxFusion option) */

/* Copyright 1999 - 2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01i,23oct01,jws  fix compiler warnings (SPR 71117); fix man pages (SPR 71239)
01h,24may99,drm  added vxfusion prefix to VxFusion related includes
01g,23feb99,wlf  doc edits
01f,19feb99,wlf  update output example
01e,18feb99,wlf  doc cleanup
01d,29oct98,drm  documentation update
01c,20may98,drm  removed some warning messages by initializing pointers to NULL
01b,30mar98,ur   set errno when group not found.
01a,09jul97,ur   written.
*/

/*
DESCRIPTION
This library provides a routine to show either the contents of the entire 
message queue group database or the contents of single message queue group.

AVAILABILITY
This module is distributed as a component of the unbundled distributed
message queues option, VxFusion.

INCLUDE FILES: msgQDistGrpShow.h

SEE ALSO: msgQDistGrpLib
*/

#include <vxWorks.h>
#include <stdio.h>
#include <errnoLib.h>
#include <sllLib.h>
#include <hashLib.h>
#include <msgQLib.h>
#include <vxfusion/msgQDistGrpLib.h>
#include <vxfusion/msgQDistGrpShow.h>
#include <vxfusion/private/msgQDistGrpLibP.h>

/* defines */

#define UNUSED_ARG(x)  if(sizeof(x)) {} /* to suppress compiler warnings */

/* forward declarations */

LOCAL void msgQDistGrpNodeShow (DIST_GRP_DB_NODE *distGrpDbNode);
LOCAL BOOL msgQDistGrpShowEach (DIST_GRP_HASH_NODE *pNode, int dummy);

/***************************************************************************
*
* msgQDistGrpShowInit - initialize group show module (VxFusion option)
*
* This routine currently does nothing.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void msgQDistGrpShowInit (void)
    {
    }

/***************************************************************************
*
* msgQDistGrpShow - display all or one group with its members (VxFusion option)
*
* This routine displays either all distributed message queue groups 
* or a specified group in the group database.  For each group displayed on the
* node, this routine lists only members added (using msgQDistGrpAdd()) from the
* node executing the msgQDistGrpShow() call.  
* 
* If <distGrpName> is NULL, all groups and their locally added
* members are displayed.  Otherwise, only the group specified by 
* <distGrpName> and its locally added members are displayed.
*
* NOTE: The concept of "locally added" is an important one.  All nodes in the 
* system can add groups to a message queue group.  However, only those message
* queues (including remote distributed message queues) that were added to 
* the group from the local node are displayed by this routine.
*
* EXAMPLE:
* \cs
-> msgQDistGrpShow(0)
* NAME OF GROUP         GROUP ID   STATE  MEMBER ID TYPE OF MEMBER
* ------------------- ---------- ------- ---------- ---------------------------
* grp1                  0x3ff9e3  global   0x3ff98b distributed msg queue
*                                          0x3ff9fb distributed msg queue
* grp2                  0x3ff933  global   0x3ff89b distributed msg queue
*                                          0x3ff8db distributed msg queue
*                                          0x3ff94b distributed msg queue
* value = 0 = 0x0
* \ce
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: OK, unless name not found.
*
* ERRNO:
* \is
* \i S_msgQDistGrpLib_NO_MATCH
* The group name was not found in the database.
* \ie
*
*/

STATUS msgQDistGrpShow
    (
    char * distGrpName  /* name of the group to display or NULL for all */
    )
    {
    DIST_GRP_DB_NODE * distGrpDbNode = NULL;

    if (distGrpName != NULL &&
        (distGrpDbNode = msgQDistGrpLclFindByName (distGrpName)) == NULL)
        {
        errnoSet (S_msgQDistGrpLib_NO_MATCH);
        return (ERROR);
        }

    printf ("NAME OF GROUP         GROUP ID   STATE  MEMBER ID ");
    printf ("TYPE OF MEMBER\n");
    printf ("------------------- ---------- ------- ---------- ");
    printf ("-----------------------------\n");

    if (distGrpName != NULL)
        msgQDistGrpNodeShow (distGrpDbNode);
    else
        msgQDistGrpLclEach (msgQDistGrpShowEach, 0);

    return (OK);
    }

/***************************************************************************
*
* msgQDistGrpShowEach - helper for msgQDistGrpShow (VxFusion option)
*
* This routine prints information about a group node given the group
* hash node.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: TRUE.
*
* NOMANUAL
*/

LOCAL BOOL msgQDistGrpShowEach
    (
    DIST_GRP_HASH_NODE *  pDistGrpHashNode,   /* group hash node */
    int                   dummy               /* unused argument */
    )
    {
    
    UNUSED_ARG(dummy);    
    
    msgQDistGrpNodeShow (pDistGrpHashNode->pDbNode);
    return (TRUE);
    }

/***************************************************************************
*
* msgQDistGrpNodeShow - print informations from struct DIST_GRP_DB_NODE (VxFusion option)
*
* This routine prints information about a group node.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: N/A
*
* NOMANUAL
*/

LOCAL void msgQDistGrpNodeShow
    (
    DIST_GRP_DB_NODE * pDistGrpDbNode   /* node for which to display info */
    )
    {
    SL_NODE *       pNode;
    MSG_Q_ID        msgQId;
    char *          state;
    DIST_MSG_Q_ID   dMsgQId;
    DIST_OBJ_ID     dObjId;

#ifdef UNDEFINED    /* apparently not used */
    DIST_MSG_Q_GRP_ID_TO_DIST_MSG_Q_ID (pDistGrpDbNode->grpDbId);
#endif
    printf ("%-19s %10p ", pDistGrpDbNode->grpDbName,
            pDistGrpDbNode->grpDbMsgQId);

    switch (pDistGrpDbNode->grpDbState)
        {
        case DIST_GRP_STATE_LOCAL_TRY:
            state = "lcl try";
            break;
        case DIST_GRP_STATE_REMOTE_TRY:
            state = "rmt try";
            break;
        case DIST_GRP_STATE_WAIT:
            state = "wait";
            break;
        case DIST_GRP_STATE_WAIT_TRY:
            state = "retry";
            break;
        case DIST_GRP_STATE_GLOBAL:
            state = "global";
            break;
        default:
            state = "unknown";
        }
    printf ("%7s ", state);

    pNode = SLL_FIRST ((SL_LIST *) &pDistGrpDbNode->grpDbMsgQIdLst);
    if (pNode == NULL)
        {
        printf ("--no local member in this group--\n");
        return;
        }

    FOREVER
        {
        msgQId = ((DIST_GRP_MSG_Q_NODE *) pNode)->msgQId;

        printf ("%10p ", msgQId);
        switch (((uint32_t) msgQId) & VX_TYPE_OBJ_MASK)
            {
            case VX_TYPE_DIST_OBJ:
                dObjId = (MSG_Q_ID_TO_DIST_OBJ_NODE (msgQId))->objNodeId;
                dMsgQId = DIST_OBJ_ID_TO_DIST_MSG_Q_ID (dObjId);

                if (IS_DIST_MSG_Q_TYPE_GRP (dMsgQId))
                    printf ("distributed msg queue group");
                else
                    printf ("distributed msg queue");
                break;
            case VX_TYPE_SM_OBJ:
                printf ("shared memory msg queue");
                break;
            case VX_TYPE_STD_OBJ:
                printf ("local msg queue");
                break;
            default:
                printf ("unknown");
                break;
            }

        if ((pNode = SLL_NEXT (pNode)) == NULL)
            {
            printf ("\n");
            break;
            }
        else
            printf ("\n                                       ");
        }

    }

