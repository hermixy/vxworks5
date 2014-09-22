/* aioPxShow.c - asynchronous I/O (AIO) show library */

/* Copyright 1984-1994 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01e,08apr94,kdl  changed aio_show() to aioShow(); added comment that arg unused.
01d,01feb94,dvs  documentation changes.
01c,26jan94,kdl  changed comment to refer to ioQLib.h, not ioQPxLib.h.
01b,12jan94,kdl  general cleanup.
01a,04apr93,elh  written.
*/

/* 
DESCRIPTION
This library implements the show routine for aioPxLib. 
*/

/* includes */

#include "vxWorks.h"
#include "aio.h"
#include "stdio.h"


/*****************************************************************************
* 
* aioShow - show AIO requests 
*
* This routine displays the outstanding AIO requests.
*
* CAVEAT
* The <drvNum> parameter is not currently used.
*
* RETURNS: OK, always.
*/

STATUS aioShow
    (
    int 		drvNum			/* drv num to show (IGNORED) */
    )
    {
    int			ix;			/* index */
    AIO_SYS *		pReq;			/* AIO request */
    DL_NODE *		pNode;			/* fd node */
    AIO_FD_ENTRY *	pEntry;			/* FD_ENTRY */
						/* defined aioPxLibP.h */
    LOCAL char * 	states [] = {"free", "completed", "ready", 
				     "queued", "wait", "running"};
						/*  defined in ioQLib.h */
    LOCAL char * 	ops []    = {"READ", "WRITE", "SYNC"};


    printf ("  aiocb   fd   op      state    prio  return     error\n");
    printf ("--------  --  -----  ---------  ----  ------  ----------\n");
   

    for (ix = 0; ix < maxFiles; ix++)
	{
	if ((pEntry = aioEntryFind (ix)) == NULL)
	    continue;

        semTake (&pEntry->ioQSem, WAIT_FOREVER);

	for (pNode = DLL_FIRST (&pEntry->ioQ); pNode != NULL; 
	     pNode = DLL_NEXT (pNode))

	    {
	    pReq = FD_NODE_TO_SYS (pNode);

            printf ("%#10x  %2d  %5s  %9s  %4d  ", (int) pReq->pAiocb, 
		    pReq->pAiocb->aio_fildes, ops [pReq->ioNode.op],
		    states [pReq->state], ((IO_NODE *) pReq)->prio);

	    if (pReq->state == AIO_COMPLETED)
		printf ("%6d %#10x", pReq->ioNode.retVal, 
			pReq->ioNode.errorVal);
	    printf ("\n");
	    }

        semGive (&pEntry->ioQSem);
	}

    printf ("\n");
    return (OK);
    }

/*****************************************************************************
* 
* aioVerboseEnbable - enable debugging/verbose mode. 
* 
* RETURNS: OK
* 
* NOMANUAL
*/

int aioVerboseEnable ()
    {
    aioPrintRtn = (FUNCPTR) printf;
    return (OK);
    }
