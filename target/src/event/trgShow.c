/* trgShow.c - trigger show routine */

/* Copyright 1984-1997 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01e,04mar99,dgp  replace INCLUDE_SHOW_ROUTINES with INCLUDE_TRIGGER_SHOW
01d,07sep98,pr   replaced LIST_MAX_LENGTH with TRG_MAX_REQUESTS
01c,28aug98,dgp  FCS man page edit
01b,07may98,dgp  clean up man pages for WV 2.0 beta release
01a,10feb98,pr   extracted from trgLib.c (based upon semShow.c).
*/

/*
DESCRIPTION
This library provides routines to show event triggering information, such as
list of triggers, associated actions, trigger states, and so on.

The routine trgShowInit() links the triggering show facility into the VxWorks
system.  It is called automatically when INCLUDE_TRIGGER_SHOW is defined.

SEE ALSO: trgLib
*/

#include "vxWorks.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "errno.h"

#include "private/trgLibP.h"

/* global */

extern TRIGGER_ID  * trgList;
extern CLASS_ID  trgClassId;     /* trg class ID */

/******************************************************************************
*
* trgShowInit - initialize the trigger show facility
*
* This routine links the trigger show facility into the VxWorks system.
* These routines are included automatically when INCLUDE_TRIGGER_SHOW is
* defined.
*
* RETURNS: N/A
*/

void trgShowInit (void)
    {
    classShowConnect (trgClassId, (FUNCPTR)trgShow);
    }

/*******************************************************************************
*
* trgShow - show trigger information 
*
* This routine displays trigger information. If <trgId> is passed, only the
* summary for that trigger is displayed.  If no parameter is passed, the list 
* of existing triggers is displayed with a summary of their state.  For
* example:
*
* .CS
*    trgID      Status  EvtID    ActType Action     Dis    Chain
*    -----------------------------------------------------------
*    0xffedfc   disabled  101          3 0x14e7a4     Y 0xffe088
*    0xffe088   enabled    55          1 0x10db58     Y      0x0
* .CE
*
* If <level> is 1, then more detailed information is displayed.
*
* EXAMPLE
* .CS
*     -> trgShow trgId, 1
* .CE
*
* RETURNS: OK.
*
* SEE ALSO: trgLib
*
*/

STATUS trgShow
    (
    TRIGGER_ID trgId,
    int        level
    )
    {
    TRIGGER_ID  pTrg;
    int index;

    if (trgId == NULL)
	{

        printf ("trgID      Status  EvtID    ActType Action     Dis Chain\n");
        printf ("--------------------------------------------------------\n");

        for (index = 0; index < TRG_MAX_REQUESTS ; index ++)
	    {
	    pTrg = trgList[index];
	    while (pTrg != NULL )
		{
	        if ( OBJ_VERIFY (pTrg, trgClassId) == OK)
		    {
	            printf ("%-10p %-7s %-8hd %-7d %-10p %-3s %-10p\n", 
			    pTrg, (pTrg->status? "enabled": "disabled"), 
			    pTrg->eventId, pTrg->actionType, 
			    pTrg->actionFunc, (pTrg->disable? "Y":"N"), 
			    pTrg->chain);
	            }
		pTrg = pTrg->next;
		}
	    }
	}
    else
	{
	pTrg = trgId;
        if ( OBJ_VERIFY (pTrg, trgClassId) == OK)
	    {
            if (level == 1)
	        {

                printf(" event Id           = %hd \n", pTrg->eventId);
                printf(" initial status     = %s\n", (pTrg->status? "enabled": "disabled"));
                printf(" number if hits     = %d \n", pTrg->hitCnt);
                printf(" contextType        = %d \n", pTrg->contextType);
                printf(" contextId          = %p\n", (int *)pTrg->contextId);
                printf(" objId              = %p\n", pTrg->objId);
                printf(" conditional?       = %s\n", (pTrg->conditional? "TRUE": "FALSE"));
                printf(" condType           = %s \n", (pTrg->condType? "var":"func"));
                printf(" condEx1            = %p\n", pTrg->condEx1);
                printf(" condOp             = %p\n", (int *)pTrg->condOp);
                printf(" condEx2            = %p\n", (int *)pTrg->condEx2);
                printf(" disable trigger    = %s\n", (pTrg->disable? "Y":"N"));
                printf(" chained trigger    = %p\n", pTrg->chain);
                printf(" actionType         = %d\n", pTrg->actionType);
                printf(" actionFunc         = %p\n", pTrg->actionFunc);
                printf(" actionDef          = %d\n", pTrg->actionDef);
                printf(" action parameter   = %p\n", (int *)pTrg->actionArg);
                printf(" next trigger       = %p\n", pTrg->next);

	        }
            else
	        { 
                printf ("trgID      Status  EvtID    ActType Action     Dis Chain\n");
                printf ("--------------------------------------------------------\n");
	        printf ("%-10p %-7s %-8hd %-7d %-10p %-3s %-10p\n", 
    		        pTrg, (pTrg->status? "enabled": "disabled"), 
		        pTrg->eventId, pTrg->actionType, 
		        pTrg->actionFunc, (pTrg->disable? "Y":"N"), 
		        pTrg->chain);
	        } 
	    }
	}

    printf ("\n\n");

    return(OK);
    }

