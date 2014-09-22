/* pcmciaLib.c - generic PCMCIA event-handling facilities */

/* Copyright 1984-1996 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01g,21jun00,rsh  upgrade to dosFs 2.0
01f,16jan97,hdn  added pCtrl->socks to force a number of sockets.
01e,08nov96,dgp  doc: final formatting
01d,28mar96,jdi  doc: cleaned up language and format.
01c,08mar96,hdn  added more descriptions.
01b,22feb96,hdn  cleaned up
01a,05apr95,hdn  written.
*/


/*
DESCRIPTION
This library provides generic facilities for handling PCMCIA events.

USER-CALLABLE ROUTINES
Before the driver can be used, it must be initialized by calling pcmciaInit().
This routine should be called exactly once, before any PC card device driver
is used.  Normally, it is called from usrRoot() in usrConfig.c.

The pcmciaInit() routine performs the following actions:
.iP
Creates a message queue.
.iP
Spawns a PCMCIA daemon, which handles jobs in the message queue.
.iP
Finds out which PCMCIA chip is installed and fills out the
PCMCIA_CHIP structure.
.iP
Connects the CSC (Card Status Change) interrupt handler.
.iP
Searches all sockets for a PC card.  If a card is found, it:

    gets CIS (Card Information Structure) information from a card

    determines what type of PC card is in the socket

    allocates a resource for the card if the card is supported

    enables the card
.iP
Enables the CSC interrupt.
.LP

The CSC interrupt handler performs the following actions:
.iP
Searches all sockets for CSC events.
.iP
Calls the PC card's CSC interrupt handler, if there is a PC card in the socket.
.iP
If the CSC event is a hot insertion, it asks the PCMCIA daemon to call cisGet()
at task level.  This call reads the CIS, determines the type of PC card, and
initializes a device driver for the card.
.iP
If the CSC event is a hot removal, it asks the PCMCIA daemon to call cisFree()
at task level.  This call de-allocates resources.

*/


/* LINTLIBRARY */

#include "vxWorks.h"
#include "taskLib.h"
#include "msgQLib.h"
#include "intLib.h"
#include "logLib.h"
#include "iv.h"
#include "private/funcBindP.h"
#include "drv/pcmcia/pcmciaLib.h"
#include "drv/pcmcia/cisLib.h"


/* externs */

IMPORT PCMCIA_CTRL	pcmciaCtrl;
IMPORT PCMCIA_ADAPTER	pcmciaAdapter[];
IMPORT int		pcmciaAdapterNumEnt;
IMPORT SEMAPHORE	cisMuteSem;


/* globals */

MSG_Q_ID	pcmciaMsgQId;		/* ID of msgQ to pcmciad */
int		pcmciadId;		/* pcmciad parameters */
int		pcmciadPriority		= 2;
int		pcmciadOptions		= VX_SUPERVISOR_MODE | VX_UNBREAKABLE;
int		pcmciadStackSize	= 8000;
BOOL		pcmciaDebug		= FALSE;


/* locals */

LOCAL int	pcmciaMsgsLost;		/* count of msgs to pcmciad lost */


/* forward declarations */

LOCAL void	pcmciaCscIntr	(void);


/*******************************************************************************
*
* pcmciaInit - initialize the PCMCIA event-handling package
*
* This routine installs the PCMCIA event-handling facilities and spawns
* pcmciad(), which performs special PCMCIA event-handling functions that
* need to be done at task level.  It also creates the message queue used to
* communicate with pcmciad().
*
* RETURNS:
* OK, or ERROR if a message queue cannot be created or pcmciad() cannot be
* spawned.
*
* SEE ALSO: pcmciad()
*/

STATUS pcmciaInit (void)

    {
    PCMCIA_CTRL *pCtrl		= &pcmciaCtrl;
    PCMCIA_CHIP *pChip		= &pCtrl->chip;
    PCMCIA_CARD *pCard;
    PCMCIA_ADAPTER *pAdapter 	= NULL;
    int sock;
    int ix;


    pcmciaMsgQId = msgQCreate (PCMCIA_MAX_MSGS, sizeof(PCMCIA_MSG), MSG_Q_FIFO);

    if (pcmciaMsgQId == NULL)
	return (ERROR);

    pcmciadId = taskSpawn ("tPcmciad", pcmciadPriority,
			   pcmciadOptions, pcmciadStackSize,
			   (FUNCPTR) pcmciad, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    if (pcmciadId == ERROR)
	return (ERROR);

    for (ix = 0; ix < pcmciaAdapterNumEnt; ix++)
	{
        pAdapter = &pcmciaAdapter[ix];
        if ((* pAdapter->initRtn) (pAdapter->ioBase,
				   pAdapter->intVec,
				   pAdapter->intLevel,
				   pAdapter->showRtn) == OK)
	    break;
	}

    if (ix >= pcmciaAdapterNumEnt)
	return (ERROR);

    semMInit (&cisMuteSem, SEM_Q_PRIORITY | SEM_DELETE_SAFE |
	      SEM_INVERSION_SAFE);

    (void) intConnect ((VOIDFUNCPTR *)INUM_TO_IVEC(pAdapter->intVec),
		       (VOIDFUNCPTR)pcmciaCscIntr, 0);

    if (pCtrl->socks != 0)		/* if explicitely defined, use it */
        pChip->socks = pCtrl->socks;

    for (sock = 0; sock < pChip->socks; sock++)
	{
	pCard = &pCtrl->card[sock];

	if ((pCard->cardStatus = (* pChip->status) (sock)) & PC_IS_CARD)
	    (void) cisGet (sock);

        (void) (* pChip->cscPoll) (sock);
        (void) (* pChip->cscOn) (sock, pChip->intLevel);
	}

    sysIntEnablePIC (pAdapter->intLevel);

    return (OK);
    }

/*******************************************************************************
*
* pcmciaJobAdd - request a task-level function call from interrupt level
*
* This routine allows interrupt level code to request a function call
* to be made by pcmciad at task-level.
*
* NOMANUAL
*
* RETURNS: OK, or ERROR if the message is lost.
*/

STATUS pcmciaJobAdd
    (
    VOIDFUNCPTR func,		/* function pointer */
    int         arg1,		/* argument 1 */
    int         arg2,		/* argument 2 */
    int         arg3,		/* argument 3 */
    int         arg4,		/* argument 4 */
    int         arg5,		/* argument 5 */
    int         arg6		/* argument 6 */
    )

    {
    PCMCIA_MSG msg;

    msg.func	= func;
    msg.arg[0]	= arg1;
    msg.arg[1]	= arg2;
    msg.arg[2]	= arg3;
    msg.arg[3]	= arg4;
    msg.arg[4]	= arg5;
    msg.arg[5]	= arg6;

    if (msgQSend (pcmciaMsgQId, (char *) &msg, sizeof (msg),
		  INT_CONTEXT() ? NO_WAIT : WAIT_FOREVER, MSG_PRI_NORMAL) != OK)
        {
        ++pcmciaMsgsLost;
        return (ERROR);
        }

    return (OK);
    }

/*******************************************************************************
*
* pcmciad - handle task-level PCMCIA events
*
* This routine is spawned as a task by pcmciaInit() to perform functions
* that cannot be performed at interrupt or trap level.  It has a priority of 0.
* Do not suspend, delete, or change the priority of this task.
*
* RETURNS: N/A
*
* SEE ALSO: pcmciaInit()
*/

void pcmciad (void)

    {
    static int oldMsgsLost = 0;

    int newMsgsLost;
    PCMCIA_MSG msg;

    FOREVER
	{
	if (msgQReceive (pcmciaMsgQId, (char *) &msg, sizeof (msg),
			 WAIT_FOREVER) != sizeof (msg))
	    {
	    if (_func_logMsg != NULL)
	        (* _func_logMsg) ("pcmciad: error receive msg, status = %#x\n",
				  errno, 0, 0, 0, 0, 0);
	    }
        else
            (* msg.func) (msg.arg[0], msg.arg[1], msg.arg[2],
			  msg.arg[3], msg.arg[4], msg.arg[5]);

	/* check to see if interrupt level lost any more calls */

	if ((newMsgsLost = pcmciaMsgsLost) != oldMsgsLost)
	    {
	    if (_func_logMsg != NULL)
	        (* _func_logMsg) ("%d messages from interrupt level lost.\n",
				  newMsgsLost - oldMsgsLost, 0, 0, 0, 0, 0);

	    oldMsgsLost = newMsgsLost;
	    }
	}
    }

/*******************************************************************************
*
* pcmciaCscIntr - interrupt handler for card status change
*
* This routine is interrupt handler for card status change.
*
* RETURNS: N/A
*/

LOCAL void pcmciaCscIntr (void)

    {
    PCMCIA_CTRL *pCtrl = &pcmciaCtrl;
    PCMCIA_CHIP *pChip = &pCtrl->chip;
    PCMCIA_CARD *pCard;
    int sock;
    int csc;
    int status;

    for (sock = 0; sock < pChip->socks; sock++)
	{
	pCard = &pCtrl->card[sock];

	/* get changed status bits */

	if ((csc = (* pChip->cscPoll) (sock)) != 0)
	    status = (* pChip->status) (sock);
	else
	    continue;

	/* ignore if the status doesn't change */

	if (status ^ pCard->cardStatus)
	    pCard->cardStatus = status;
	else
	    continue;

	if ((pcmciaDebug) && (_func_logMsg != NULL))
	    (* _func_logMsg) ("CSC sock=%d csc=0x%-4x status=0x%-4x\n",
			      sock, csc, pCard->cardStatus, 0, 0, 0);

	/* card's CSC interrupt handler. go next sock if it returns ERROR */

	if (pCard->cscIntr != NULL)
	    if ((* pCard->cscIntr) (sock, csc) != OK)
		continue;

	/* hot insertion */

	if ((csc & PC_DETECT) && (pCard->cardStatus & PC_IS_CARD))
	    {
	    pcmciaJobAdd ((VOIDFUNCPTR)cisGet, sock, 0,0,0,0,0);
	    if ((pcmciaDebug) && (_func_logMsg != NULL))
	        (* _func_logMsg) ("Inserted: pcmciaJobAdd (cisGet)\n",
				  0, 0, 0, 0, 0, 0);
	    }

	/* hot removal */

	if ((csc & PC_DETECT) && (pCard->cardStatus & PC_NO_CARD))
	    {
	    pcmciaJobAdd (cisFree, sock, 0,0,0,0,0);
	    if ((pcmciaDebug) && (_func_logMsg != NULL))
	        (* _func_logMsg) ("Removed: pcmciaJobAdd (cisFree)\n",
				  0, 0, 0, 0, 0, 0);
	    }
	}
    }

