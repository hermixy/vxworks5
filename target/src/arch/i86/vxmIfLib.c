/* vxmIfLib.c - interface library to VxM  */

/* Copyright 1984-1992 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01b,23aug92,jcf  removed VXM_IF_OPS typedef.
01a,01aug92,del  created.
*/

/*
DESCRIPTION
------------
This is the interface library for VxE and VxM.
*/

#include "vxWorks.h"
#include "private/vxmIfLibP.h"


/* globals */

VXM_IF_OPS		vxmIfOps;
VXM_IF_ANCHOR *		pVxmIfAnchor;
BOOL			vxmIfLibInstalled;
VXM_IF_OPS *		pVxmIfOps = &vxmIfOps;


/******************************************************************************
*
* vxmIfInit - initialize the interface
*/  

STATUS vxmIfInit 
    (
    VXM_IF_ANCHOR *	pAnchor
    )
    {
    FUNCPTR	getFunc;

    if (vxmIfLibInstalled)
	return (OK);

    pVxmIfAnchor = pAnchor;

    if (pVxmIfAnchor->ifMagic != VXM_IF_MAGIC)
	return (ERROR);

    /* the the interface access function from the anchor */
    getFunc = pVxmIfAnchor->ifGetFunc;

    /* initialize the interface operations using the access function */
    vxmIfOps.vxmTblGet	   = getFunc;
    vxmIfOps.vxmIntVecSet  = (FUNCPTR) (*getFunc) (VXM_IF_INT_VEC_SET_FUNC);
    vxmIfOps.vxmBufRead	   = (FUNCPTR) (*getFunc) (VXM_IF_BUF_RD_FUNC);
    vxmIfOps.vxmBufWrite   = (FUNCPTR) (*getFunc) (VXM_IF_BUF_WRT_FUNC);
    vxmIfOps.vxmWrtBufFlush =(FUNCPTR) (*getFunc) (VXM_IF_WRTBUF_FLUSH_FUNC);
    vxmIfOps.vxmHostQuery  = (FUNCPTR) (*getFunc) (VXM_IF_QUERY_FUNC);
    vxmIfOps.vxmClbkAdd    = (FUNCPTR) (*getFunc) (VXM_IF_CALLBACK_ADD_FUNC);
    vxmIfOps.vxmClbkState  = (FUNCPTR) (*getFunc) (VXM_IF_CALLBACK_STATE_FUNC);
    vxmIfOps.vxmClbkQuery  = (FUNCPTR) (*getFunc) (VXM_IF_CALLBACK_QUERY_FUNC);

    vxmIfLibInstalled = TRUE;

    return (OK);
    }

/******************************************************************************
*
* vxmIfInstalled - check for the presence of the ROM monitor.
*/  

BOOL vxmIfInstalled (void)
    {
    return (vxmIfLibInstalled);
    }

/******************************************************************************
*
* vxmIfVecSet - set an interrupt vector via the ROM monitor.
*/  

STATUS vxmIfVecSet 
    (
    FUNCPTR * 	vector,
    FUNCPTR	function
    )
    {
    if (pVxmIfOps->vxmIntVecSet == NULL)
	return (ERROR);

    return ((* pVxmIfOps->vxmIntVecSet) (vector, function)); 
    }

/******************************************************************************
*
* vxmIfVecGet - get an interrupt vector via the ROM monitor.
*/  

FUNCPTR vxmIfVecGet 
    (
    FUNCPTR * 	vector
    )
    {
    if (pVxmIfOps->vxmIntVecGet == NULL)
	return (NULL);

    return ((FUNCPTR)(* pVxmIfOps->vxmIntVecGet) (vector)); 
    }

/******************************************************************************
*
* vxmIfHostQuery - query the host via VxM for input data.
*/   

BOOL vxmIfHostQuery ()
    {
    if (pVxmIfOps->vxmHostQuery == NULL)
	return (FALSE);

    return ((* pVxmIfOps->vxmHostQuery) ());
    }

/******************************************************************************
*
* vxmIfWrtBufFlush - flush the ROM monitor write buffer.
*/  

void vxmIfWrtBufFlush ()

    {
    if (pVxmIfOps->vxmWrtBufFlush != NULL)
	(* pVxmIfOps->vxmWrtBufFlush) ();
    }

/******************************************************************************
*
* vxmIfBufRead - read a buffer of data from the ROM monitor.
*/  

int vxmIfBufRead
    (
    char *	pBuf,
    int		nBytes
    )
    {
    if (pVxmIfOps->vxmBufRead == NULL)
	return (0);

    return ((* pVxmIfOps->vxmBufRead) (pBuf, nBytes));
    }

/******************************************************************************
*
* vxmIfBufWrite - write a buffer of data to the ROM monitor.
*/  

int vxmIfBufWrite
    (
    char *	pBuf,
    int		nBytes
    )
    {
    if (pVxmIfOps->vxmBufWrite == NULL)
	return (0);

    return ((* pVxmIfOps->vxmBufWrite) (pBuf, nBytes));
    }

/******************************************************************************
*
* vxmIfCallbackAdd - add a callback to the interface.
*/  

STATUS vxmIfCallbackAdd
    (
    int		funcNo,		/* callback function number */
    FUNCPTR 	func,		/* callback function */
    UINT32	arg,		/* required argument to pass */ 
    UINT32	maxargs,	/* max. number of optional args */
    UINT32	state		/* initial state of callback */
    )
    {
    if (pVxmIfOps->vxmClbkAdd == NULL)
	return (ERROR);

    return ((* pVxmIfOps->vxmClbkAdd) (funcNo, func, arg, maxargs, state));
    }

/******************************************************************************
*
* vxmIfCallbackReady - set the state of the given callback to ready.
*
*/  

STATUS vxmIfCallbackReady 
    (
    int	funcNo			/* number of callback */
    )
    {
    if (pVxmIfOps->vxmClbkState == NULL)
	return (ERROR);

    return ((* pVxmIfOps->vxmClbkState) (funcNo));
    }

/******************************************************************************
*
* vxmIfCallbackQuery - query the host for action on the given callback.
* 
*/  

STATUS vxmIfCallbackQuery
    (
    int	funcNo			/* callback to query */
    )
    {
    if (pVxmIfOps->vxmClbkQuery == NULL)
	return (ERROR);

    return ((* pVxmIfOps->vxmClbkQuery) (funcNo));
    }
