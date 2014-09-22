/* msgQDistShow - distributed message queue show routines (VxFusion option) */

/* Copyright 1999-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01h,30oct01,jws  fix man pages (SPR 71239)
01g,24may99,drm  added vxfusion prefix to VxFusion related includes
01f,18feb99,wlf  doc cleanup
01e,17feb99,drm  Changing msgQDistShow to display node portion of unique ID
01d,29oct98,drm  documentation updates
01c,11aug98,drm  fixed group message queue display problem
01b,09may98,ur   removed 8 bit node id restriction
01a,02oct97,ur   written.
*/

/*
DESCRIPTION
This library provides show routines for distributed message queues.  The
user does not call these show routines directly.  Instead, he uses
the msgQShow library routine msgQShow() to display the contents of a
message queue, regardless of its type.  The msgQShow() routine calls the
distributed show routines, as necessary.

AVAILABILITY
This module is distributed as a component of the unbundled distributed
message queues option, VxFusion.

INCLUDE FILES: msgQDistShow.h

SEE ALSO: msgQDistLib, msgQShow
*/

#include "vxWorks.h"
#include "stdio.h"
#include "errnoLib.h"
#include "msgQLib.h"
#include "vxfusion/msgQDistLib.h"
#include "vxfusion/msgQDistGrpLib.h"
#include "vxfusion/distNameLib.h"
#include "vxfusion/distStatLib.h"
#include "vxfusion/private/msgQDistLibP.h"
#include "vxfusion/private/msgQDistGrpLibP.h"
#include "vxfusion/private/distNodeLibP.h"

/* local prototypes */

LOCAL STATUS msgQDistShow (MSG_Q_ID msgQId, int level);
LOCAL STATUS msgQDistInfoGet (MSG_Q_ID msgQId, MSG_Q_INFO *pInfo);

/***************************************************************************
*
* msgQDistShowInit - initialize the distributed message queue show package (VxFusion option)
*
* This routine initializes the distributed message queue show package.
*
* NOTE: This routine is called automatically when a target boots using a 
* VxWorks image with VxFusion installed and show routines enabled.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: N/A
*/

void msgQDistShowInit (void)
    {
    msgQDistShowRtn       = (FUNCPTR) msgQDistShow;
    msgQDistInfoGetRtn    = (FUNCPTR) msgQDistInfoGet;
    }

/***************************************************************************
*
* msgQDistShow - show information about a distributed message queue (VxFusion option)
*
* This routine prints information about a distributed message queue.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: OK, unless <msgQId> is invalid.
*
* ERRNO: S_distLib_OBJ_ID_ERROR
*
* NOMANUAL
*/

LOCAL STATUS msgQDistShow
    (
    MSG_Q_ID    msgQId,        /* message queue to display */
    int         level          /* 0 = summary, 1 = details */
    )
    {
    DIST_MSG_Q_ID    dMsgQId;
    DIST_OBJ_NODE *  pObjNode;
    BOOL             msgQHasName = FALSE;
    char             nameMsgQ[DIST_NAME_MAX_LENGTH + 1];

    if (DIST_OBJ_VERIFY (msgQId) == ERROR)
        {
        errnoSet (S_distLib_OBJ_ID_ERROR);
        return (ERROR);
        }

    pObjNode = MSG_Q_ID_TO_DIST_OBJ_NODE (msgQId);
    if (! IS_DIST_MSG_Q_OBJ (pObjNode))
        {
        errnoSet (S_distLib_OBJ_ID_ERROR);
        return (ERROR); /* legal object id, but not a message queue */
        }

    if (distNameFindByValueAndType (&msgQId, T_DIST_MSG_Q,
            (char *) &nameMsgQ, NO_WAIT) == OK)
        msgQHasName = TRUE;

    printf ("Message Queue Id    : 0x%lx", (u_long) msgQId);
    if (msgQHasName)
        printf (" (\"%s\")\n", nameMsgQ);
    else
        printf (" (not in name db)\n");

    dMsgQId = (DIST_MSG_Q_ID) pObjNode->objNodeId;

    if (IS_DIST_MSG_Q_TYPE_GRP (dMsgQId))
        {
        /* group */
        
        DIST_MSG_Q_GRP_ID    distGrpId;
        DIST_GRP_DB_NODE *   pNode;
        char *               state;

        printf ("Global unique Id    : 0x%lx\n", (u_long) dMsgQId);
        printf ("Type                : group\n");

        distGrpId = DIST_MSG_Q_ID_TO_DIST_MSG_Q_GRP_ID (dMsgQId);
        pNode = msgQDistGrpLclFindById (distGrpId);
        printf ("Group Name          : \"%s\"\n",
                (char *) &(pNode->grpDbName));

        switch (pNode->grpDbState)
            {
            case DIST_GRP_STATE_LOCAL_TRY:
                state = "try";
                break;
            case DIST_GRP_STATE_WAIT:
                state = "wait";
                break;
            case DIST_GRP_STATE_WAIT_TRY:
                state = "retry";
                break;
            case DIST_GRP_STATE_GLOBAL:
                {
                DIST_NODE_ID    nodeIdCreator = pNode->grpDbNodeId;
                BOOL            creatorHasName = FALSE;
                char            nameCreator[DIST_NAME_MAX_LENGTH + 1];

                if (distNameFindByValueAndType (&nodeIdCreator, T_DIST_NODE,
                        (char *) &nameCreator, NO_WAIT) == OK)
                    creatorHasName = TRUE;

                printf ("Creating Node       : 0x%08lx",
                        (u_long) nodeIdCreator);

                if (creatorHasName)
                    printf (" (\"%s\")\n", nameCreator);
                else
                    printf (" (not in name db)\n");

                state = "global";
                break;
                }
            default:
                state = "unknown";
            }
        printf ("State               : %s\n", state);

        }
    else
        {
        /*  queue */
        
        DIST_NODE_ID    nodeIdHome;
        BOOL            homeHasName = FALSE;
        char            nameHome[DIST_NAME_MAX_LENGTH + 1];
        char *          type;

        nodeIdHome = pObjNode->objNodeReside;

        printf ("Global unique Id    : 0x%lx:%lx\n", (u_long) nodeIdHome,
                (u_long) dMsgQId);

        if (nodeIdHome == distNodeLocalGetId())
            type = "queue";
        else
            type = "remote queue";
        printf ("Type                : %s\n", type);

        if (distNameFindByValueAndType (&nodeIdHome, T_DIST_NODE,
                (char *) &nameHome, NO_WAIT) == OK)
            homeHasName = TRUE;
        printf ("Home Node           : 0x%08lx",
                (u_long) nodeIdHome);
        if (homeHasName)
            printf (" (\"%s\")\n", nameHome);
        else
            printf (" (not in name db)\n");

        if (nodeIdHome == distNodeLocalGetId())
            {
            MSG_Q_ID    mapped = msgQDistGetMapped (msgQId);

            printf ("Mapped to           : 0x%lx\n", (u_long) mapped);
            if (msgQShow (mapped, level) == ERROR)
                return (ERROR);
            }
        }

    return (OK);
    }

/***************************************************************************
*
* msgQDistInfoGet - get information about a distributed message queue (VxFusion option)
*
* This routine currently does nothing.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: OK, unless <msgQId> is invalid.
*
* NOMANUAL
*/

LOCAL STATUS msgQDistInfoGet
    (
    MSG_Q_ID       msgQId,      /* message queue to query */
    MSG_Q_INFO *   pInfo        /* where to return msg info */
    )
    {
    MSG_Q_ID    mappedQId;

    if ((mappedQId = msgQDistGetMapped (msgQId)) == NULL)
        return ERROR;

    return (msgQInfoGet (mappedQId, pInfo));
    }

