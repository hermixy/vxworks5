/* pppHookLib.c - PPP hook library */

/* Copyright 1995 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01d,14mar99,jdi  doc: removed refs to config.h and/or configAll.h (SPR 25663).
01c,20aug98,fle  doc : removed tab from func headers
01b,19dec95,vin  doc changes.
01a,29nov95,vin  written.
*/

/*
DESCRIPTION
This library provides routines to add and delete connect and disconnect
routines. The connect routine, added on a unit basis, is called before the 
initial phase of link option negotiation. The disconnect routine, added on
a unit basis is called before the PPP connection is closed. These connect 
and disconnect routines can be used to hook up additional software.
If either connect or disconnect hook returns ERROR, the connection is 
terminated immediately.

This library is automatically linked into the VxWorks system image when
the configuration macro INCLUDE_PPP is defined.

INCLUDE FILES: pppLib.h

SEE ALSO: pppLib,
.pG "Network"
*/

/* includes */

#include "vxWorks.h"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"
#include "errnoLib.h"
#include "semLib.h"
#include "pppLib.h"

/* externs */

IMPORT PPP_HOOK_RTNS *	pppHookRtns [];		/* declared in pppLib.c */

/* locals */

LOCAL SEM_ID 	pppHookSemId = NULL;		/* protect access to table */

/*******************************************************************************
*
* pppHookLibInit - initialize the PPP hook connect and disconnect facility
*
* This routine is initilaizes the connect and disconnect facility
* It is called from usrNetwork.c when the configuration macro INCLUDE_PPP
* is defined.
*
* RETURNS: N/A
*
* NOMANUAL
*/

STATUS pppHookLibInit (void)
    {
    int		ix;			/* index variable */

    if (pppHookSemId == NULL)		/* already initialized? */
	{
	for (ix = 0; ix < NPPP; ix++)
	    pppHookRtns [ix] = NULL;

        if ((pppHookSemId = semMCreate (SEM_Q_PRIORITY | SEM_INVERSION_SAFE))
	    == NULL)
            return (ERROR);
	}

    return (OK);
    }

/*******************************************************************************
*
* pppHookAdd - add a hook routine on a unit basis
*
* This routine adds a hook to the Point-to-Point Protocol (PPP) channel. 
* The parameters to this routine specify the unit number (<unit>) of the
* PPP interface, the hook routine (<hookRtn>), and the type of hook
* specifying either a connect hook or a disconnect hook (<hookType>).
* The following hook types can be specified for the <hookType> parameter:
* .iP PPP_HOOK_CONNECT
* Specify a connect hook.
* .iP PPP_HOOK_DISCONNECT
* Specify a disconnect hook.
* 
* RETURNS: OK, or ERROR if the hook cannot be added to the unit.
*
* SEE ALSO: pppHookDelete()
*/

STATUS pppHookAdd
    (
    int			unit,		/* unit number */
    FUNCPTR		hookRtn,	/* hook routine */	
    int			hookType	/* hook type connect/disconnect */
    )
    {
    PPP_HOOK_RTNS *	pPPPHookRtns;

    if (pppHookSemId == NULL)
	{
        errno = S_pppHookLib_NOT_INITIALIZED;
	return (ERROR);
	}

    if (unit < 0 || unit > NPPP)
	{
	errno = S_pppHookLib_INVALID_UNIT;
        return (ERROR);
	}

    if ((hookType != PPP_HOOK_CONNECT) && (hookType != PPP_HOOK_DISCONNECT))
	{
	errno = S_pppHookLib_HOOK_UNKNOWN;
	return (ERROR);
	}

    if ((pPPPHookRtns = pppHookRtns[unit]) == NULL)
	{
	/* allocate it, the first time it is called. */
	
	if ((pPPPHookRtns = 
	     (PPP_HOOK_RTNS *) calloc (1, sizeof (PPP_HOOK_RTNS))) == NULL)
	    return (ERROR);
	}

    else	/* check for adding the hook over again */
    	{
	if (((hookType == PPP_HOOK_CONNECT) && 
	     (pppHookRtns[unit]->connectHook != NULL)) ||
	    ((hookType == PPP_HOOK_DISCONNECT) && 
	     (pppHookRtns[unit]->disconnectHook != NULL)))
	    {
	    errno = S_pppHookLib_HOOK_ADDED;
	    return (ERROR);
	    }
	}

    semTake (pppHookSemId, WAIT_FOREVER);	/* exclusive access to list */
    
    pppHookRtns [unit] = pPPPHookRtns;

    if (hookType == PPP_HOOK_CONNECT)
	pPPPHookRtns->connectHook = hookRtn;
    else 
	pPPPHookRtns->disconnectHook = hookRtn;
        
    semGive (pppHookSemId);			/* give up access */

    return (OK);
    }

/*******************************************************************************
*
* pppHookDelete - delete a hook routine on a unit basis
*
* This routine deletes a hook added previously to the 
* Point-to-Point Protocol (PPP) channel. The parameters to this routine 
* specify the unit number (<unit>) of the PPP interface and the type of 
* hook specifying either a connect hook or a disconnect hook (<hookType>).
* The following hook types can be specified for the <hookType> parameter:
* .iP PPP_HOOK_CONNECT
* Specify a connect hook.
* .iP PPP_HOOK_DISCONNECT
* Specify a disconnect hook.
* 
* RETURNS: OK, or ERROR if the hook cannot be deleted for the unit.
*
* SEE ALSO: pppHookAdd()
*/

STATUS pppHookDelete
    (
    int			unit,		/* unit number */
    int			hookType	/* hook type connect/disconnect */
    )
    {

    if (pppHookSemId == NULL)
	{
        errno = S_pppHookLib_NOT_INITIALIZED;
	return (ERROR);
	}

    if (unit < 0 || unit > NPPP)
	{
	errno = S_pppHookLib_INVALID_UNIT;
        return (ERROR);
	}

    if ((hookType != PPP_HOOK_CONNECT) && (hookType != PPP_HOOK_DISCONNECT))
	{
	errno = S_pppHookLib_HOOK_UNKNOWN;
	return (ERROR);
	}

    if ((pppHookRtns[unit] == NULL) || (((hookType == PPP_HOOK_CONNECT) && 
					 (pppHookRtns[unit]->connectHook 
					  == NULL)) ||
					((hookType == PPP_HOOK_DISCONNECT) && 
					 (pppHookRtns[unit]->disconnectHook
					  == NULL))))
    	{
	errno = S_pppHookLib_HOOK_DELETED;
	return (ERROR);
	}

    else 
	{
	semTake (pppHookSemId, WAIT_FOREVER);	/* exclusive access to list */
	
	if (hookType == PPP_HOOK_CONNECT)
	    pppHookRtns[unit]->connectHook = NULL;
	else
	    pppHookRtns[unit]->disconnectHook = NULL;

	if ((pppHookRtns[unit]->connectHook == NULL) && 
	    (pppHookRtns[unit]->disconnectHook == NULL))
	    {
	    free (pppHookRtns[unit]);	/* free if both hooks are deleted */
	    pppHookRtns [unit] = NULL;
	    }
	semGive (pppHookSemId);		/* give up access */
	}

    return (OK);
    }
