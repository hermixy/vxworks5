/* distNameShow.c - distributed name database show routines (VxFusion option) */

/* Copyright 1999-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01g,30oct01,jws  fix man pages (SPR 71239)
01f,22oct01,jws  update distNameShow man page (SPR 70849)
01e,24may99,drm  added vxfusion prefix to VxFusion related includes
01d,19feb99,wlf  update output example
01d,18feb99,wlf  doc cleanup
01c,13feb99,drm  changed code to display UINT64 type as BIG ENDIAN
01b,28oct98,drm  documentation modifications
01a,22sep97,ur   written.
*/

/*
DESCRIPTION
This library provides routines for displaying the contents of the
distributed name database.

AVAILABILITY
This module is distributed as a component of the unbundled distributed
message queues option, VxFusion.

INCLUDE FILES: distNameLib.h

SEE ALSO: distNameLib
*/

#include "vxWorks.h"
#include "stdio.h"
#include "msgQLib.h"
#include "vxfusion/distNameLib.h"
#include "vxfusion/private/distNodeLibP.h"
#include "vxfusion/private/distNameLibP.h"

LOCAL BOOL distNameDbShowOne (DIST_NAME_DB_NODE *pNode, int unused);

/***************************************************************************
*
* distNameShowInit - initialize show routine (VxFusion option)
*
* This routine currently does nothing.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: N/A
* NOMANUAL
*/

void distNameShowInit (void)
    {
    }

/***************************************************************************
*
* distNameShow - display the entire distributed name database (VxFusion option)
*
* This routine displays the entire contents of the distributed name database.
* The data displayed includes the symbolic ASCII name, the type, and the value.
* If the type is not pre-defined, it is printed in decimal and the
* value shown in a hexdump.
*
* NOTE:
* Option VX_FP_TASK should be set when spawning any task in which
* distNameShow() is called unless it is certain that no floating
* point values will be in the database.  The target shell has this option set.
* 
*
* EXAMPLE:
* \cs
* -> distNameShow()
*         NAME              TYPE               VALUE
* -------------------- -------------- -------------------------
* nile                    T_DIST_NODE 0x930b2617 (2466981399)
* columbia                T_DIST_NODE 0x930b2616 (2466981398)
* dmq-01                 T_DIST_MSG_Q 0x3ff9fb
* dmq-02                 T_DIST_MSG_Q 0x3ff98b
* dmq-03                 T_DIST_MSG_Q 0x3ff94b
* dmq-04                 T_DIST_MSG_Q 0x3ff8db
* dmq-05                 T_DIST_MSG_Q 0x3ff89b
* gData                          4096 0x48 0x65 0x6c 0x6c 0x6f 0x00 
* gCount                T_DIST_UINT32 0x2d (45)
* grp1                   T_DIST_MSG_Q 0x3ff9bb
* grp2                   T_DIST_MSG_Q 0x3ff90b
* value = 0 = 0x0
* \ce
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: N/A
*/

void distNameShow (void)
    {
    printf ("        NAME              TYPE               VALUE\n");
    printf ("-------------------- -------------- -------------------------\n");
    distNameEach ((FUNCPTR) distNameDbShowOne, (-1));
    }

/***************************************************************************
*
* distNameFilterShow - display the distributed name database filtered by type (VxFusion option)
*
* This routine displays the contents of the distributed name database
* filtered by <type>.  The data displayed includes the symbolic ASCII name, the 
* type, and the value.  If the type is not pre-defined, it is printed 
* in decimal and the value shown in a hexdump.
*
* NOTE:
* Option VX_FP_TASK should be set when spawning any task in which
* distNameFilterShow() is called unless it is certain that no floating
* point values will be displayed.  The target shell has this option set.
*
* EXAMPLE:
* \cs
* -> distNameFilterShow(0)
*         NAME              TYPE               VALUE
* -------------------- -------------- -------------------------
* dmq-01                 T_DIST_MSG_Q 0x3ff9fb
* dmq-02                 T_DIST_MSG_Q 0x3ff98b
* dmq-03                 T_DIST_MSG_Q 0x3ff94b
* dmq-04                 T_DIST_MSG_Q 0x3ff8db
* dmq-05                 T_DIST_MSG_Q 0x3ff89b
* grp1                   T_DIST_MSG_Q 0x3ff9bb
* grp2                   T_DIST_MSG_Q 0x3ff90b
* value = 0 = 0x0
* \ce
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
* 
* RETURNS: N/A
*/

void distNameFilterShow
    (
    DIST_NAME_TYPE    type  /* type to filter the database by */
    )
    {
    printf ("        NAME              TYPE               VALUE\n");
    printf ("-------------------- -------------- -------------------------\n");
    distNameEach ((FUNCPTR) distNameDbShowOne, type);
    }

/***************************************************************************
*
* distNameDbShowOne - helper function to display one single node of database (VxFusion option)
*
* This routine display one node of the database.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: TRUE
*
* NOMANUAL
*/

LOCAL BOOL distNameDbShowOne
    (
    DIST_NAME_DB_NODE *    pNode,    /* node whose data to show */
    int                    filter    /* type to show, or -1 for all */
    )
    {
    if (filter != (-1) && pNode->type != filter)
        return (TRUE);

    printf ("%-20s ", (char *) &(pNode->symName));
    switch (pNode->type)
        {
        case T_DIST_MSG_Q:
            printf ("  T_DIST_MSG_Q %p",
                    *((MSG_Q_ID *) &pNode->value));
            break;
        case T_DIST_NODE:
            printf ("   T_DIST_NODE 0x%lx (%lu)",
                    *((DIST_NODE_ID *) &pNode->value),
                    *((DIST_NODE_ID *) &pNode->value));
            break;
        case T_DIST_UINT8:
            printf ("  T_DIST_UINT8 0x%x (%u)",
                    *((uint8_t *) &pNode->value),
                    *((uint8_t *) &pNode->value));
            break;
        case T_DIST_UINT16:
            printf (" T_DIST_UINT16 0x%x (%u)",
                    *((uint16_t *) &pNode->value),
                    *((uint16_t *) &pNode->value));
            break;
        case T_DIST_UINT32:
            printf (" T_DIST_UINT32 0x%lx (%lu)",
                    *((uint32_t *) &pNode->value),
                    *((uint32_t *) &pNode->value));
            break;
        case T_DIST_UINT64:
            /* Display UINT64 most significant byte first */

            printf (" T_DIST_UINT64 ");
            #if _BYTE_ORDER == _BIG_ENDIAN
            {
            int i;
            printf ("0x");
            for (i=0; i<pNode->valueLen; i++)
                printf ("%02x", *(((uint8_t *) &pNode->value) + i));
            }
            #endif
            #if _BYTE_ORDER == _LITTLE_ENDIAN
            {
            int i;
            printf("0x");
            for (i=0; i<pNode->valueLen; i++)
                printf ("%02x",
                        *(((uint8_t *) &pNode->value) +
                          (pNode->valueLen)-1-i));
            }
            #endif
            break;
        case T_DIST_FLOAT:
            printf ("  T_DIST_FLOAT %f",
                    *((float *) &pNode->value));
            break;
        case T_DIST_DOUBLE:
            printf (" T_DIST_DOUBLE %f",
                    *((double *) &pNode->value));
            break;
        default:
            {
            int i;
            printf ("%14d ", pNode->type);
            for (i=0; i<pNode->valueLen; i++)
                printf ("0x%02x ", *(((uint8_t *) &pNode->value) + i));
            }
        }
    printf ("\n");

    return (TRUE);    /* continue */
    }

