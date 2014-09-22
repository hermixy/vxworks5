/* scsiCtrlLib.c - SCSI thread-level controller library (SCSI-2) */

/* Copyright 1989-1996 Wind River Systems, Inc. */
#include "copyright_wrs.h"


/*
modification history
--------------------
01d,28mar97,dds  SPR 8220: added check for parity errors.
01c,29oct96,dgp  doc: editing for newly published SCSI libraries
01b,21oct96,dds removed NOMANUAL from functions called by
                the driver interface.
01a,12sep96,dds added the thread-level controller routines from
                scsi2Lib.c
*/

/*
DESCRIPTION
The purpose of the SCSI controller library is to support basic SCSI controller 
drivers that rely on a higher level of software in order to manage 
SCSI transactions. More advanced SCSI I/O processors do not require this
protocol engine since software support for SCSI transactions is provided
at the SCSI I/O processor level.

This library provides all the high-level routines that manage the state of 
the SCSI threads and guide the SCSI I/O transaction through its various stages:
.iP
selecting a SCSI peripheral device;
.iP
sending the identify message in order to establish the ITL nexus;
.iP
cycling through information transfer, message and data, and status phases;
.iP
handling bus-initiated reselects.
.LP

The various stages of the SCSI I/O transaction are reported to the SCSI manager
as SCSI events. Event selection and management is handled by routines
in this library. 

INCLUDE FILES
scsiLib.h, scsi2Lib.h

SEE ALSO: scsiLib, scsi2Lib, scsiCommonLib, scsiDirectLib, scsiSeqLib,
scsiMgrLib,
.I  "American National Standard for Information Systems - Small Computer"
.I  "System Interface (SCSI-2), ANSI X3T9,"
.pG "I/O System, Local File Systems"
*/

#define  INCLUDE_SCSI2
#include "vxWorks.h"
#include "ioLib.h"
#include "intLib.h"
#include "ctype.h"
#include "cacheLib.h"
#include "stdlib.h"
#include "errnoLib.h"
#include "taskLib.h"
#include "lstLib.h"
#include "logLib.h"
#include "msgQLib.h"
#include "string.h"
#include "stdio.h"
#include "sysLib.h"
#include "scsiLib.h"
#include "wdLib.h"

/* global functions */

SCSI_THREAD * scsiCtrlIdentThreadCreate (SCSI_CTRL *pScsiCtrl);
STATUS 	      scsiCtrlThreadInit        (SCSI_CTRL   * pScsiCtrl,
					 SCSI_THREAD * pThread);
STATUS        scsiCtrlThreadActivate    (SCSI_CTRL   * pScsiCtrl,
					 SCSI_THREAD * pThread);
VOID          scsiCtrlEvent             (SCSI_CTRL   * pScsiCtrl,
					 SCSI_EVENT  * pEvent);
BOOL          scsiCtrlThreadAbort       (SCSI_CTRL   * pScsiCtrl,
					 SCSI_THREAD * pThread);
/* Generic thread-level controller driver */

LOCAL void          scsiCtrlThreadEvent      (SCSI_THREAD * pThread,
				              SCSI_EVENT  * pEvent);
LOCAL void          scsiCtrlThreadConnect    (SCSI_THREAD * pThread,
				              SCSI_EVENT  * pEvent);
LOCAL void          scsiCtrlThreadDisconnect (SCSI_THREAD * pThread,
				              SCSI_EVENT  * pEvent);
LOCAL void          scsiCtrlThreadSelect     (SCSI_THREAD * pThread,
				              SCSI_EVENT  * pEvent);
LOCAL void          scsiCtrlThreadInfoXfer   (SCSI_THREAD * pThread,
				              SCSI_EVENT  * pEvent);
LOCAL void          scsiCtrlIdentOutXfer     (SCSI_THREAD * pThread,
				              SCSI_EVENT  * pEvent);
LOCAL void          scsiCtrlIdentInCommence  (SCSI_THREAD * pThread,
				              SCSI_EVENT  * pEvent);
LOCAL void          scsiCtrlIdentInXfer      (SCSI_THREAD * pThread,
				              SCSI_EVENT  * pEvent);
LOCAL void          scsiCtrlIdentInContinue  (SCSI_THREAD * pThread);
LOCAL void          scsiCtrlThreadReconnect  (SCSI_THREAD * pThread);
LOCAL void          scsiCtrlNormalXfer       (SCSI_THREAD * pThread,
				              SCSI_EVENT  * pEvent);
LOCAL void          scsiCtrlAbortXfer        (SCSI_THREAD * pThread,
				              SCSI_EVENT  * pEvent);
LOCAL STATUS        scsiCtrlDataOutAction (SCSI_CTRL   * pScsiCtrl,
				           SCSI_THREAD * pThread);
LOCAL STATUS        scsiCtrlDataInAction  (SCSI_CTRL   * pScsiCtrl,
				           SCSI_THREAD * pThread);
LOCAL STATUS        scsiCtrlCommandAction (SCSI_CTRL   * pScsiCtrl,
				           SCSI_THREAD * pThread);
LOCAL STATUS        scsiCtrlStatusAction  (SCSI_CTRL   * pScsiCtrl,
				           SCSI_THREAD * pThread);
LOCAL STATUS        scsiCtrlMsgOutAction  (SCSI_CTRL   * pScsiCtrl,
				           SCSI_THREAD * pThread);
LOCAL STATUS        scsiCtrlMsgInAction   (SCSI_CTRL   * pScsiCtrl,
				           SCSI_THREAD * pThread);
LOCAL STATUS        scsiCtrlMsgInXfer     (SCSI_CTRL   * pScsiCtrl);
LOCAL STATUS        scsiCtrlXferParamsSet (SCSI_THREAD * pThread);
LOCAL STATUS        scsiCtrlMsgInAck      (SCSI_CTRL   * pScsiCtrl);
LOCAL void          scsiCtrlThreadComplete (SCSI_THREAD * pThread);
LOCAL void          scsiCtrlThreadDefer    (SCSI_THREAD * pThread);
LOCAL void          scsiCtrlThreadFail     (SCSI_THREAD * pThread, int errNum);
LOCAL void          scsiCtrlThreadStateSet (SCSI_THREAD * pThread, 
				            SCSI_THREAD_STATE state);

/*******************************************************************************
*
* scsiCtrlIdentThreadCreate - create thread context for incoming identification
*
* Allocate and initialise a thread context which will be used to read
* incoming identification messages.  Note that the normal thread allocator
* is not used because this thread is somewhat special.
*
* NOTE:
* This routine is currently called no matter what type of controller is in
* use.   In theory, it should be controller-specific.
*
* RETURNS: thread ptr, or 0 if an error occurs
*
* NOMANUAL
*/
SCSI_THREAD * scsiCtrlIdentThreadCreate
    (
    SCSI_CTRL * pScsiCtrl
    )
    {
    SCSI_THREAD * pThread;

    if ((pThread = malloc (pScsiCtrl->threadSize)) == 0)
	return (NULL);

    /*
     *	Initialise thread structure (mostly unused)
     */
    bzero ((char *) pThread, pScsiCtrl->threadSize);
    
    pThread->pScsiCtrl = pScsiCtrl;
    pThread->role      = SCSI_ROLE_IDENT_INIT;

    return (pThread);
    }


/*******************************************************************************
*
* scsiCtrlThreadActivate - request (re)activation of a thread
*
* Set whatever thread/controller state variables need to be set.  Ensure that
* all buffers used by the thread are coherent with the contents of the
* system caches (if any).
*
* For initiator threads, issue a SELECT command.  For target threads, issue a
* RESELECT command.  Do not wait for it to complete, as this is signalled by
* an event which should be one of:
*
*   CONNECTED	    - the connection attempt was successful (thread continues)
*
*   TIMEOUT 	    - the connection attempt timed out	    (thread fails)
*
*   (RE)SELECTED    - we lost a race against being selected (thread defers)
*
* RETURNS: OK, or ERROR if activation fails
*
* NOMANUAL
*/
STATUS scsiCtrlThreadActivate
    (
    SCSI_CTRL *   pScsiCtrl,
    SCSI_THREAD * pThread
    )
    {
    SCSI_TARGET * pScsiTarget = pThread->pScsiTarget;

    scsiCacheSynchronize (pThread, SCSI_CACHE_PRE_COMMAND);

    /*
     *	Reset controller state variables for the new thread
     */
    pScsiCtrl->msgOutState = SCSI_MSG_OUT_NONE;
    pScsiCtrl->msgInState  = SCSI_MSG_IN_NONE;
    
    /*
     *	Initiate synchronous transfer negotiation
     */
    scsiSyncXferNegotiate (pScsiCtrl, pScsiTarget, SYNC_XFER_NEW_THREAD);

    /*
     *	Initiate (asynchronous) selection of the target
     */
    if ((*pScsiCtrl->scsiDevSelect) (pScsiCtrl,
				     pScsiTarget->scsiDevBusId,
				     pScsiTarget->selTimeOut,
				     pThread->identMsg,
				     pThread->identMsgLength) != OK)
	{
	SCSI_DEBUG_MSG ("scsiCtrlThreadActivate: failed to select device.\n",
			0, 0, 0, 0, 0, 0);

	return (ERROR);
	}

    pScsiCtrl->pThread = pThread;

    scsiCtrlThreadStateSet (pThread, SCSI_THREAD_CONNECTING);

    return (OK);
    }


/*******************************************************************************
*
* scsiCtrlThreadAbort - abort the specified thread
*
* If the thread is not currently connected, do nothing and return FALSE to
* indicate that the SCSI manager should abort the thread.
*
* Otherwise (the thread is active onthe controller), build an ABORT or
* ABORT TAG message which will (eventually) be sent, causing the taget to
* disconnect.  Set the state of the thread accordingly, and return TRUE to
* indicate that the controller driver will handle the abort process.
*
* RETURNS: TRUE if the thread is being aborted by this driver (i.e. it is
* currently active on the controller, else FALSE.
*
* NOMANUAL
*/
BOOL scsiCtrlThreadAbort
    (
    SCSI_CTRL *   pScsiCtrl,
    SCSI_THREAD * pThread
    )
    {
    BOOL  tagged;

    if (pThread != pScsiCtrl->pThread)
	return (FALSE);
    
    switch (pThread->state)
	{
	case SCSI_THREAD_INACTIVE:
	case SCSI_THREAD_WAITING:
	case SCSI_THREAD_DISCONNECTED:
	    return (FALSE);
	    break;

	default:
	    /*
	     *	Build an ABORT (or ABORT TAG) message.  When this has been
	     *	sent, the target should disconnect.  Mark the thread aborted
	     *	and save the error code until disconnection.
	     */
	    tagged = (pThread->tagNumber != NONE);

	    pScsiCtrl->msgOutBuf[0] = tagged ? SCSI_MSG_ABORT_TAG
		    	    	    	     : SCSI_MSG_ABORT;
	    pScsiCtrl->msgOutLength = 1;
	    pScsiCtrl->msgOutState  = SCSI_MSG_OUT_PENDING;

	    scsiCtrlThreadStateSet (pThread, SCSI_THREAD_ABORTING);
	    break;
	}

    return (TRUE);
    }


/*******************************************************************************
*
* scsiCtrlThreadInit - initialise a newly created thread structure
*
* This is a controller-specific function to initialise a thread.  For the
* generic thread-level controller, a standard (exported) initialisation
* function suffices.
*
* RETURNS: OK, or ERROR if initialisation fails
*
* NOMANUAL
*/
STATUS scsiCtrlThreadInit
    (
    SCSI_CTRL *   pScsiCtrl,
    SCSI_THREAD * pThread
    )
    {
    return (scsiThreadInit (pThread));
    }


/*******************************************************************************
*
* scsiCtrlEvent - SCSI controller event processing routine
*
* Parse the event type and act accordingly.  Controller-level events are
* handled within this function, and the event is then passed to the current
* thread (if any) for thread-level processing.
*
* Note the special case when (re)selection occurs: if there is a current
* thread when the event occurs, it receives the event (and is assumed to
* defer itself) before the identification thread is made current.  The
* event is then forwarded to the identification thread.
*
* RETURNS: N/A
*
* NOMANUAL
*/
VOID scsiCtrlEvent
    (
    SCSI_CTRL *  pScsiCtrl,
    SCSI_EVENT * pEvent
    )
    {
    SCSI_THREAD * pThread = pScsiCtrl->pThread;
    
    SCSI_DEBUG_MSG ("scsiCtrlEvent: received event %d, thread = 0x%08x\n",
		    pEvent->type, (int) pThread, 0, 0, 0, 0);

    /*
     *	Do controller-level event processing
     */
    switch (pEvent->type)
	{
	case SCSI_EVENT_SELECTED:
	case SCSI_EVENT_RESELECTED:
    	    if (pThread != 0)
	    	scsiCtrlThreadEvent (pThread, pEvent);

    	    pScsiCtrl->peerBusId = pEvent->busId;

    	    pThread = pScsiCtrl->pThread = pScsiCtrl->pIdentThread;

	    pThread->role = (pEvent->type == SCSI_EVENT_SELECTED)
		    	  ? SCSI_ROLE_IDENT_TARG
			  : SCSI_ROLE_IDENT_INIT;

	    scsiMgrCtrlEvent (pScsiCtrl, SCSI_EVENT_CONNECTED);
	    break;

	case SCSI_EVENT_CONNECTED:
    	    pScsiCtrl->peerBusId = pEvent->busId;

	    scsiMgrCtrlEvent (pScsiCtrl, SCSI_EVENT_CONNECTED);

    	    /* assert (pThread != 0); */
	    break;

	case SCSI_EVENT_DISCONNECTED:
	case SCSI_EVENT_TIMEOUT:
    	    pScsiCtrl->peerBusId = NONE;
    	    pScsiCtrl->pThread   = 0;

	    scsiMgrCtrlEvent (pScsiCtrl, SCSI_EVENT_DISCONNECTED);
	    
    	    /* assert (pThread != 0); */
	    break;

	case SCSI_EVENT_XFER_REQUEST:
    	    /* assert (pThread != 0); */
	    break;

        case SCSI_EVENT_PARITY_ERR:
            break;

	case SCSI_EVENT_BUS_RESET:
    	    pScsiCtrl->peerBusId = NONE;
    	    pScsiCtrl->pThread   = 0;

    	    scsiMgrBusReset (pScsiCtrl);
	    break;

	default:
	    logMsg ("scsiCtrlEvent: invalid event type (%d)\n",
		    pEvent->type, 0, 0, 0, 0, 0);
	    break;
	}

    /*
     *	If there's a thread on the controller, forward the event to it
     */
    if (pThread != 0)
	scsiCtrlThreadEvent (pThread, pEvent);
    }
    
/*******************************************************************************
*
* scsiCtrlThreadEvent - SCSI thread event processing routine
*
* Parse the event type and forward it to the appropriate handler.  If there
* is an established thread, and there is an outgoing message pending,
* assert the SCSI ATN signal.
*
* RETURNS: N/A
*/
LOCAL void scsiCtrlThreadEvent
    (
    SCSI_THREAD * pThread,
    SCSI_EVENT *  pEvent
    )
    {
    SCSI_CTRL * pScsiCtrl = pThread->pScsiCtrl;

    SCSI_DEBUG_MSG ("scsiCtrlThreadEvent: thread = 0x%08x: received event %d\n",
		    (int) pThread, pEvent->type, 0, 0, 0, 0);

    switch (pEvent->type)
	{
	case SCSI_EVENT_CONNECTED:
    	    scsiCtrlThreadConnect (pThread, pEvent);
	    break;

	case SCSI_EVENT_DISCONNECTED:
    	    scsiCtrlThreadDisconnect (pThread, pEvent);
	    break;

	case SCSI_EVENT_SELECTED:
	case SCSI_EVENT_RESELECTED:
    	    scsiCtrlThreadSelect (pThread, pEvent);
	    break;

	case SCSI_EVENT_XFER_REQUEST:
    	    scsiCtrlThreadInfoXfer (pThread, pEvent);
	    break;

	case SCSI_EVENT_TIMEOUT:
    	    scsiCtrlThreadFail (pThread, S_scsiLib_SELECT_TIMEOUT);
	    break;

        case SCSI_EVENT_PARITY_ERR:
	    errnoSet (S_scsiLib_HARDWARE_ERROR);
    	    scsiCtrlThreadFail (pThread, S_scsiLib_HARDWARE_ERROR);
	    break;

	case SCSI_EVENT_BUS_RESET:
	    /* SCSI manager handles this */
	    scsiCtrlThreadStateSet (pThread, SCSI_THREAD_INACTIVE);
	    break;

	default:
	    logMsg ("scsiCtrlThreadEvent: invalid event type (%d)\n",
		    pEvent->type, 0, 0, 0, 0, 0);
	    break;
	}

    switch (pThread->state)
	{
	case SCSI_THREAD_ESTABLISHED:
	    /* assert ATN if there is a pending message out */

	    if (pScsiCtrl->msgOutState == SCSI_MSG_OUT_PENDING)
	    	{
	    	if ((*pScsiCtrl->scsiBusControl) (pScsiCtrl,
					      	  SCSI_BUS_ASSERT_ATN) != OK)
		    {
		    SCSI_ERROR_MSG ("scsiCtrlEvent: failed to assert ATN.\n",
				    0, 0, 0, 0, 0, 0);

		    scsiCtrlThreadFail (pThread, errno);
		    }
		}
	    break;

	default:
	    break;
	}
    }
    
/*******************************************************************************
*
* scsiCtrlThreadConnect - thread connection event processing routine
*
* Set transfer parameters for the newly connected thread.  Set the thread
* state to ESTABLISHED if the whole identification message has been sent,
* otherwise set its state to IDENTIFICATION OUT.
*
* RETURNS: N/A
*/
LOCAL void scsiCtrlThreadConnect
    (
    SCSI_THREAD * pThread,
    SCSI_EVENT *  pEvent
    )
    {
    SCSI_PHYS_DEV * pScsiPhysDev = pThread->pScsiPhysDev;
    SCSI_TARGET *   pScsiTarget  = pThread->pScsiTarget;
    
    if (pThread->state != SCSI_THREAD_CONNECTING)
	{
	logMsg ("scsiCtrlThreadConnect: thread 0x%08x: invalid state (%d)\n",
		(int) pThread, pThread->state, 0, 0, 0, 0);
	return;
	}

    SCSI_DEBUG_MSG ("scsi: connected to target %d, phys dev = 0x%08x\n",
		    pScsiTarget->scsiDevBusId, (int)pScsiPhysDev, 0, 0, 0, 0);

    pThread->nBytesIdent = pEvent->nBytesIdent;

    if (scsiCtrlXferParamsSet (pThread) != OK)
	{
        scsiCtrlThreadFail (pThread, errno);
	return;
	}

    if (pThread->nBytesIdent < pThread->identMsgLength)
	{
    	scsiCtrlThreadStateSet (pThread, SCSI_THREAD_IDENT_OUT);
	}
    else
	{
	scsiCtrlThreadStateSet (pThread, SCSI_THREAD_ESTABLISHED);
	}
    }


/*******************************************************************************
*
* scsiCtrlThreadDisconnect - thread disconnect event processing routine
*
* There are basically three cases:
*
*   1)	after a DISCONNECT message has been received: disconnect the thread
*
*   2)	after a COMMAND COMPLETE message has been received, or an ABORT
*   	message has been sent, complete the thread normally
*
*   3)	otherwise, fail the thread with an "unexpected disconnection" error
*
* RETURNS: N/A
*/
LOCAL void scsiCtrlThreadDisconnect
    (
    SCSI_THREAD * pThread,
    SCSI_EVENT *  pEvent
    )
    {
    if (pThread->pScsiCtrl->msgOutState == SCSI_MSG_OUT_PENDING)
	{
	SCSI_DEBUG_MSG ("scsiCtrlThreadDisconnect: thread 0x%08x: "
			"message out not sent (ignored).\n",
			(int) pThread, 0, 0, 0, 0, 0);
	}

    switch (pThread->state)
	{
	case SCSI_THREAD_WAIT_DISCONNECT:
    	    scsiMgrThreadEvent (pThread, SCSI_THREAD_EVENT_DISCONNECTED);

    	    scsiCtrlThreadStateSet (pThread, SCSI_THREAD_DISCONNECTED);
	    break;

	case SCSI_THREAD_WAIT_COMPLETE:
	    scsiCtrlThreadComplete (pThread);
	    break;

	case SCSI_THREAD_WAIT_ABORT:
	    scsiCtrlThreadComplete (pThread);
	    break;

	default:
	    SCSI_ERROR_MSG ("scsiCtrlThreadDisconnect: thread 0x%08x: "
			    "unexpected disconnection\n",
			    (int) pThread, 0, 0, 0, 0, 0);

	    scsiCtrlThreadFail (pThread, S_scsiLib_DISCONNECTED);
	    break;
	}
    }

    
/*******************************************************************************
*
* scsiCtrlThreadSelect - thread selection event processing routine
*
* Forward the event to the proper handler depending on the role played by
* the thread.
*
* RETURNS: N/A
*/
LOCAL void scsiCtrlThreadSelect
    (
    SCSI_THREAD * pThread,
    SCSI_EVENT *  pEvent
    )
    {
    switch (pThread->state)
	{
	case SCSI_THREAD_CONNECTING:
	    scsiCtrlThreadDefer (pThread);
	    break;

	case SCSI_THREAD_INACTIVE:
	    scsiCtrlIdentInCommence (pThread, pEvent);
	    break;

	default:
	    logMsg ("scsiCtrlThreadConnect: thread 0x%08x: "
		    "invalid state (%d)\n",
		    (int) pThread, pThread->state, 0, 0, 0, 0);
	    break;
	}
    }


/*******************************************************************************
*
* scsiCtrlThreadInfoXfer - thread info transfer request event processing
*
* Forward the event to the proper handler depending on the thread's current
* state.
*
* RETURNS: N/A
*/
LOCAL void scsiCtrlThreadInfoXfer
    (
    SCSI_THREAD * pThread,
    SCSI_EVENT *  pEvent
    )
    {
    switch (pThread->state)
	{
	case SCSI_THREAD_IDENT_OUT:
	    scsiCtrlIdentOutXfer (pThread, pEvent);
	    break;

	case SCSI_THREAD_IDENT_IN:
	    scsiCtrlIdentInXfer (pThread, pEvent);
	    break;

	case SCSI_THREAD_ABORTING:
	    scsiCtrlAbortXfer (pThread, pEvent);
	    break;
	    
	default:
	    scsiCtrlNormalXfer (pThread, pEvent);
	    break;
	}
    }

    
/*******************************************************************************
*
* scsiCtrlIdentOutXfer - process info. xfer request during identification out
*
* Check that a message out transfer is being requested: fail the thread if
* not.  Transfer the remainder of the identification message, including a
* queue tag message if applicable.  Set the thread state to ESTABLISHED.
*
* This code assumes that the target cannot request a new info transfer phase
* until it has read the entire identification sequence (either 1 or 3 bytes).
* (This seems to be required by the SCSI standard.)
*
* RETURNS: N/A
*/
LOCAL void scsiCtrlIdentOutXfer
    (
    SCSI_THREAD * pThread,
    SCSI_EVENT *  pEvent
    )
    {
    SCSI_CTRL * pScsiCtrl = pThread->pScsiCtrl;
    int         nBytes;
    int         maxBytes;

    /*
     *	Check for logical errors
     */
    if (pThread->state != SCSI_THREAD_IDENT_OUT)
	{
	logMsg ("scsiCtrlIdentOutXfer: invalid state (%d)\n",
		pThread->state, 0, 0, 0, 0, 0);
	return;
	}
    
    if ((maxBytes = pThread->identMsgLength - pThread->nBytesIdent) == 0)
	{
	logMsg ("scsiCtrlIdentOutXfer: no identification message!\n",
		0, 0, 0, 0, 0, 0);
	return;
	}
    
    if (pEvent->phase != SCSI_MSG_OUT_PHASE)
	{
	SCSI_ERROR_MSG ("scsiCtrlIdentOutXfer: unexpected phase (%d) "
			"requested during identification out\n",
		        pEvent->phase, 0, 0, 0, 0, 0);

	scsiCtrlThreadFail (pThread, S_scsiLib_INVALID_PHASE);
        return;
	}

    /*
     *	Transfer the whole remaining identification sequence (in one go)
     */
    nBytes = (*pScsiCtrl->scsiInfoXfer) (pScsiCtrl,
				         SCSI_MSG_OUT_PHASE,
                                         pThread->identMsg + pThread->nBytesIdent,
				         maxBytes);

    pThread->nBytesIdent += nBytes;

    if (nBytes != maxBytes)
        {
	/*
	 *  The target has most likely requested a new phase.  The only
	 *  legitimate reason for this is to reject a queue tag message if
	 *  it does not support queueing - this should be dealt with by the
	 *  normal message reject mechanism.  (The thread should be treated
	 *  as if it were untagged.)
	 *
	 *  Hence, we continue and set the thread established anyway.
	 */
        SCSI_DEBUG_MSG ("scsiCtrlIdentOutXfer: identification incomplete - "
			"%d of %d bytes sent\n",
			pThread->nBytesIdent, pThread->identMsgLength,
    	    	    	0, 0, 0, 0);
	
        }

    scsiCtrlThreadStateSet (pThread, SCSI_THREAD_ESTABLISHED);
    }
    

/*******************************************************************************
*
* scsiCtrlIdentInCommence - commence incoming identification
*
* Copy the identification message read by the driver into the thread's buffer.
* Parse the identification message so far (see "scsiCtrlIdentInContinue()").
* Acknowledge the last message-in byte read (if any).
*
* RETURNS: N/A
*/
LOCAL void scsiCtrlIdentInCommence
    (
    SCSI_THREAD * pThread,
    SCSI_EVENT *  pEvent
    )
    {
    bcopy ((char *)pEvent->identMsg, (char *)pThread->identMsg,
	   pEvent->nBytesIdent);

    pThread->nBytesIdent = pEvent->nBytesIdent;

    scsiCtrlIdentInContinue (pThread);

    if (pThread->nBytesIdent > 0)
	{
	scsiCtrlMsgInAck (pThread->pScsiCtrl);
	}
    }
    
/*******************************************************************************
*
* scsiCtrlIdentInXfer - process info. xfer during incoming identification.
*
* Check that a message in transfer is being requested; if not, fail the
* thread (even though, at this stage, it's not a client thread).  Transfer
* the message byte in, then parse the message so far.  Acknowledge the
* message in byte.
*
* RETURNS: N/A
*/
LOCAL void scsiCtrlIdentInXfer
    (
    SCSI_THREAD * pThread,
    SCSI_EVENT *  pEvent
    )
    {
    SCSI_CTRL * pScsiCtrl = pThread->pScsiCtrl;
    
    if (pThread->state != SCSI_THREAD_IDENT_IN)
	{
	logMsg ("scsiCtrlIdentInXfer: invalid state (%d)\n",
		pThread->state, 0, 0, 0, 0, 0);
	return;
	}
    
    if (pEvent->phase != SCSI_MSG_IN_PHASE)
	{
	SCSI_ERROR_MSG ("scsiCtrlIdentInXfer: unexpected phase (%d) "
			"requested during identification in\n",
		        pEvent->phase, 0, 0, 0, 0, 0);

	scsiCtrlThreadFail (pThread, 0);
        return;
	}

    if ((*pScsiCtrl->scsiInfoXfer) (pScsiCtrl,
				    SCSI_MSG_IN_PHASE,
                                    pThread->identMsg + pThread->nBytesIdent,
				    1) != 1)
        {
        SCSI_ERROR_MSG ("scsiCtrlIdentInXfer: message in transfer failed\n",
			0, 0, 0, 0, 0, 0);
	
	scsiCtrlThreadFail (pThread, 0);
        return;
        }

    ++pThread->nBytesIdent;

    scsiCtrlIdentInContinue (pThread);

    scsiCtrlMsgInAck (pScsiCtrl);
    }
    

/*******************************************************************************
*
* scsiCtrlIdentInContinue - continue incoming identification
*
* Parse the message built up so far.  If it is not yet complete, do nothing.
* If the message is complete, attempt to reconnect the thread it identifies,
* and deactivate this thread (the identification thread is no longer active).
* Otherwise (identification has failed), abort the identification sequence.
*
* RETURNS: N/A
*/
LOCAL void scsiCtrlIdentInContinue
    (
    SCSI_THREAD * pThread
    )
    {
    SCSI_THREAD * pNewThread;
    SCSI_CTRL   * pScsiCtrl  = pThread->pScsiCtrl;
    SCSI_IDENT_STATUS status;
    SCSI_THREAD_STATE state;

    status = scsiIdentMsgParse (pScsiCtrl, pThread->identMsg,
			       	    	   pThread->nBytesIdent,
			       	    	  &pThread->pScsiPhysDev,
			       	    	  &pThread->tagNumber);

    switch (status)
	{
	case SCSI_IDENT_INCOMPLETE:
	    state = SCSI_THREAD_IDENT_IN;
	    break;

	case SCSI_IDENT_COMPLETE:
	    scsiMgrThreadEvent (pThread, SCSI_THREAD_EVENT_RECONNECTED);

	    if ((pNewThread = scsiMgrPhysDevActiveThreadFind (
						pThread->pScsiPhysDev,
						pThread->tagNumber)) == 0)
		{
		state = SCSI_THREAD_IDENT_ABORTING;
		}
	    else
		{
	    	scsiCtrlThreadReconnect (pNewThread);

	    	state = SCSI_THREAD_INACTIVE;
		}
	    break;

	case SCSI_IDENT_FAILED:
	    state = SCSI_THREAD_IDENT_ABORTING;
	    break;

	default:
	    logMsg ("scsiCtrlIdentInContinue: invalid ident status (%d)\n",
    	    	    status, 0, 0, 0, 0, 0);
	    state = SCSI_THREAD_INACTIVE;
	    break;
	}

    if (state == SCSI_THREAD_IDENT_ABORTING)
	scsiCtrlThreadAbort (pScsiCtrl, pThread);

    scsiCtrlThreadStateSet (pThread, state);
    }

    
/*******************************************************************************
*
* scsiCtrlThreadReconnect - reconnect a thread
*
* Restore the SCSI pointers for the thread (this really should be in a more
* generic section of code - perhaps part of the SCSI manager's thread event
* procesing ?).  Set the controller's transfer parameters for the newly 
* connected thread.  Set the thread's state to ESTABLISHED.
*
* RETURNS: N/A
*/
LOCAL void scsiCtrlThreadReconnect
    (
    SCSI_THREAD * pThread
    )
    {
    SCSI_CTRL * pScsiCtrl = pThread->pScsiCtrl;
    
    SCSI_DEBUG_MSG ("scsiCtrlThreadReconnect: reconnecting thread 0x%08x\n",
		    (int) pThread, 0, 0, 0, 0, 0);

    pScsiCtrl->pThread = pThread;

    /*
     *  Reset controller state variables for the new thread
     */
    pScsiCtrl->msgOutState = SCSI_MSG_OUT_NONE;
    pScsiCtrl->msgInState  = SCSI_MSG_IN_NONE;

    /* Implied RESTORE POINTERS action: see "scsiMsgInComplete ()" */

    pThread->activeDataAddress = pThread->savedDataAddress;
    pThread->activeDataLength  = pThread->savedDataLength;

    if (scsiCtrlXferParamsSet (pThread) != OK)
	{
	SCSI_ERROR_MSG ("scsiCtrlThreadReconnect: failed to set xfer params.\n",
			0, 0, 0, 0, 0, 0);

	scsiCtrlThreadFail (pThread, errno);
	return;
	}

    scsiCtrlThreadStateSet (pThread, SCSI_THREAD_ESTABLISHED);
    }


/*******************************************************************************
*
* scsiCtrlNormalXfer - initiator info transfer request event processing
*
* Check for phase changes during message-in and message-out transfers,
* handling them accordingly (neither is necessarily an error condition).
* Call the appropriate handler routine for the message transfer phase
* requested by the target.
*
* RETURNS: N/A
*/
LOCAL void scsiCtrlNormalXfer
    (
    SCSI_THREAD * pThread,
    SCSI_EVENT *  pEvent
    )
    {
    STATUS status;

    SCSI_CTRL * pScsiCtrl = pThread->pScsiCtrl;
    int         phase     = pEvent->phase;

    switch (pThread->state)
	{
	case SCSI_THREAD_ESTABLISHED:
	case SCSI_THREAD_WAIT_COMPLETE:
	case SCSI_THREAD_WAIT_DISCONNECT:
	case SCSI_THREAD_WAIT_ABORT:
	    break;

	default:
	    logMsg ("scsiCtrlNormalXfer: thread 0x%08x: invalid state (%d)\n",
		    (int) pThread, pThread->state, 0, 0, 0, 0);
	    return;
	}
    
    SCSI_DEBUG_MSG ("scsiCtrlNormalXfer: thread 0x%08x: "
		    "target requesting %s phase\n",
		    (int) pThread, (int) scsiPhaseNameGet (phase),
		    0, 0, 0, 0);
	
    /*
     *	Check for phase change during message in handling: this probably
     *	results from our having asserted ATN in order to send a message out.
     *	This would occur if we reject a partially read incoming message.
     *
     *	Reset the message-in state in this case.
     */
    if (phase != SCSI_MSG_IN_PHASE)
	{
    	switch (pScsiCtrl->msgInState)
	    {
	    case SCSI_MSG_IN_NONE:
	    	break;

	    default:
            	SCSI_DEBUG_MSG ("scsiCtrlNormalXfer: phase change "
				"during msg in\n",
			    	0, 0, 0, 0, 0, 0);
	    	pScsiCtrl->msgInState = SCSI_MSG_IN_NONE;
	    break;
	    }
	}
	
    /*
     *	Check for phase change during (immediately after) message out.
     *
     *	If this happens while there is a pending message out, there are
     *	two possibilities.  Either the target has not yet noticed our ATN
     *	(i.e., no message data has yet been transferred) or it has changed
     *	phase in mid-transfer (probably to reject the message we're sending).
     *	In either case, the message state is left pending so that it will be
     *	(re-)sent or rejected as appropriate.
     *
     *	If it happens just after a message out has been sent, it means the
     *	target has accepted the message.  In this case the message out state
     *	is reset, and the message out completion routine is called.
     */
    if (phase != SCSI_MSG_OUT_PHASE)
	{
    	switch (pScsiCtrl->msgOutState)
	    {
	    case SCSI_MSG_OUT_NONE:
	    case SCSI_MSG_OUT_PENDING:
	    	break;

	    case SCSI_MSG_OUT_SENT:
	    	pScsiCtrl->msgOutState = SCSI_MSG_OUT_NONE;

	    	(void) scsiMsgOutComplete (pScsiCtrl, pThread);
	    	break;
            default:
	        break;
	    }
	}
	    
    /*
     *	Perform information transfer requested by target
     */
    switch (phase)
	{
	case SCSI_DATA_OUT_PHASE:
	    status = scsiCtrlDataOutAction (pScsiCtrl, pThread);
	    break;
	    
	case SCSI_DATA_IN_PHASE:
	    status = scsiCtrlDataInAction (pScsiCtrl, pThread);
	    break;
	    
	case SCSI_COMMAND_PHASE:
	    status = scsiCtrlCommandAction (pScsiCtrl, pThread);
	    break;
	    
	case SCSI_STATUS_PHASE:
	    status = scsiCtrlStatusAction (pScsiCtrl, pThread);
	    break;
	    
	case SCSI_MSG_OUT_PHASE:
	    status = scsiCtrlMsgOutAction (pScsiCtrl, pThread);
	    break;
	    
	case SCSI_MSG_IN_PHASE:
	    status = scsiCtrlMsgInAction (pScsiCtrl, pThread);
	    break;
	    
	default:
	    SCSI_ERROR_MSG ("scsiCtrlNormalXfer: invalid phase (%d)\n",
		      	    phase, 0, 0, 0, 0, 0);
	    scsiCtrlThreadFail (pThread, S_scsiLib_INVALID_PHASE);
	    return;
	}

    if (status != OK)
        {
        SCSI_ERROR_MSG ("scsiCtrlNormalXfer: transfer failed\n",
			0, 0, 0, 0, 0, 0);
	scsiCtrlThreadFail (pThread, errno);
        }
    }

    
/*******************************************************************************
*
* scsiCtrlAbortXfer - process info. xfer request for an aborting thread
*
* Check that the phase requested is message out.  Transfer the single-byte
* message out.  Set the thread's state to indicate that it's waiting for the
* target to disconnect after an abort message.
*
* RETURNS: N/A
*/
LOCAL void scsiCtrlAbortXfer
    (
    SCSI_THREAD * pThread,
    SCSI_EVENT *  pEvent
    )
    {
    SCSI_CTRL * pScsiCtrl = pThread->pScsiCtrl;
    
    if (pThread->state != SCSI_THREAD_ABORTING)
	{
	logMsg ("scsiCtrlAbortXfer: invalid state (%d)\n",
		pThread->state, 0, 0, 0, 0, 0);
	return;
	}
    
    if (pEvent->phase != SCSI_MSG_OUT_PHASE)
	{
	SCSI_ERROR_MSG ("scsiCtrlAbortXfer: unexpected phase (%d) "
			"requested during thread abort\n",
		        pEvent->phase, 0, 0, 0, 0, 0);
        return;
	}

    if ((*pScsiCtrl->scsiInfoXfer) (pScsiCtrl,
				    SCSI_MSG_OUT_PHASE,
                                    pScsiCtrl->msgOutBuf,
				    1) != 1)
        {
        SCSI_ERROR_MSG ("scsiCtrlAbortXfer: message out transfer failed\n",
			0, 0, 0, 0, 0, 0);
        return;
        }

    scsiCtrlThreadStateSet (pThread, SCSI_THREAD_WAIT_ABORT);
    }
    

/******************************************************************************
*
* scsiCtrlDataOutAction - handle DATA OUT information transfer phase
*
* Transfer out the remaining data count or the maximum possible for the
* controller, whichever is the smaller.  Update the active data pointer
* and count to reflect the number of bytes transferred - this may be fewer
* than requested, e.g. if the target requests a different phase.
*
* RETURNS: OK, or ERROR if transfer fails.
*/

LOCAL STATUS scsiCtrlDataOutAction
    (
    SCSI_CTRL   *pScsiCtrl,		/* ptr to SCSI controller info */
    SCSI_THREAD *pThread		/* ptr to thread info */
    )
    {
    UINT xferCount;			/* number of bytes transferred */
    UINT maxBytes;			/* max number of bytes to send */

    maxBytes = min (pThread->activeDataLength, pScsiCtrl->maxBytesPerXfer);
    
    xferCount = (*pScsiCtrl->scsiInfoXfer) (pScsiCtrl,
					    SCSI_DATA_OUT_PHASE,
                                            pThread->activeDataAddress,
                                            maxBytes);

    if (xferCount == ERROR)
    	return (ERROR);

    pThread->activeDataAddress += xferCount;
    pThread->activeDataLength  -= xferCount;

    return (OK);
    }


/******************************************************************************
*
* scsiCtrlDataInAction - handle DATA IN information transfer phase
*
* Transfer in the remaining data count or the maximum possible for the
* controller, whichever is the smaller.  Update the active data pointer
* and count to reflect the number of bytes transferred - this may be fewer
* than requested, e.g. if the target requests a different phase.
*
* NOTE: the "additional length byte" handling done in previous versions of
* the SCSI library is unnecessary when disconnect/reconnect is supported.
*
* SCSI requires the target to honour the initiator's maximum allocation
* length, so there can never be more incoming data than was requested.
* Conversely, if the target has less data to send than the initiator
* expects, it will simply terminate the DATA IN transfer phase early.
*
* Unfortunately the currently defined SCSI transaction interface does not
* allow the client task to find out how many data bytes were actually
* received in either case.
* 
* RETURNS: OK, or ERROR if transfer fails.
*/

LOCAL STATUS scsiCtrlDataInAction
    (
    SCSI_CTRL   *pScsiCtrl,		/* ptr to SCSI controller info */
    SCSI_THREAD *pThread		/* ptr to thread info */
    )
    {
    UINT xferCount;			/* number of bytes transferred */
    UINT maxBytes;			/* max number of bytes to recv */

    maxBytes = min (pThread->activeDataLength, pScsiCtrl->maxBytesPerXfer);
    
    xferCount = (*pScsiCtrl->scsiInfoXfer) (pScsiCtrl,
					    SCSI_DATA_IN_PHASE,
                                            pThread->activeDataAddress,
                                            maxBytes);

    if (xferCount == ERROR)
    	return (ERROR);

    pThread->activeDataAddress += xferCount;
    pThread->activeDataLength  -= xferCount;

    return (OK);
    }


/******************************************************************************
*
* scsiCtrlCommandAction - handle COMMAND information transfer phase
*
* Note: it is assumed that a command transfer must be completed fully, i.e.
* the target cannot disconnect.  It is also assumed that the length of a
* command transfer never exceeds the maximum byte count the controller can
* handle.
*
* RETURNS: OK, or ERROR if transfer fails.
*/

LOCAL STATUS scsiCtrlCommandAction
    (
    SCSI_CTRL   *pScsiCtrl,		/* ptr to SCSI controller info */
    SCSI_THREAD *pThread		/* ptr to thread info */
    )
    {
    UINT xferCount;			/* number of bytes transferred */

    xferCount = (*pScsiCtrl->scsiInfoXfer) (pScsiCtrl,
					    SCSI_COMMAND_PHASE,
                                            pThread->cmdAddress,
                                            pThread->cmdLength);

    return ((xferCount == pThread->cmdLength) ? OK : ERROR);
    }


/******************************************************************************
*
* scsiCtrlStatusAction - handle STATUS information transfer phase
*
* Note: it is assumed that a status transfer must be completed fully, i.e.
* the target cannot disconnect.  It is also assumed that the length of a
* status transfer never exceeds the maximum byte count the controller can
* handle.
*
* RETURNS: OK, or ERROR if transfer fails.
*/

LOCAL STATUS scsiCtrlStatusAction
    (
    SCSI_CTRL   *pScsiCtrl,		/* ptr to SCSI controller info */
    SCSI_THREAD *pThread		/* ptr to thread info */
    )
    {
    UINT xferCount;			/* number of bytes transferred */

    xferCount = (*pScsiCtrl->scsiInfoXfer) (pScsiCtrl,
					    SCSI_STATUS_PHASE,
                                            pThread->statusAddress,
                                            pThread->statusLength);

    return ((xferCount == pThread->statusLength) ? OK : ERROR);
    }


/******************************************************************************
*
* scsiCtrlMsgOutAction - respond to a request to send a SCSI message
*
* This routine handles a SCSI message out transfer request.  If there is no
* message available to send to the target, a SCSI NO-OP message is sent.
* If there is a message which has already been sent, ATN is asserted (this is
* a requirement imposed by the SCSI specification).
*
* Data is output from the message-out buffer in a single transfer.  The
* controller hardware must keep ATN asserted until just before ACK is
* asserted during the last byte of the outgoing message, per SCSI 5.2.1.
*
* Assuming the information transfer does not fail altogether, there are two
* "successful" outcomes:
*
* 1)    the entire message is transferred - in this case the message state
*   	is set to SENT.	 However, the target is not deemed to have accepted
*   	the message until it requests a different phase - this is detected
*   	and acted upon in "scsiCtrlNormalXfer()".  (If the target continues
*   	to request a MSG OUT transfer, it wants the current message to be
*   	re-sent.)
*
* 2)	the message is only partially transferred - most likely because the
*   	target has changed phase (e.g., to send a Reject message) before
*   	all the message out bytes have been transferred.  In this case the
*   	message state remains PENDING.  If the target is about to reject it,
*   	the state will be reset then, otherwise we will try to send it again
*   	when the target next requests a message out transfer.
*
* Note: it is assumed that the length of the message never exceeds the
* maximum byte count the controller can handle.
*
* RETURNS: OK, or ERROR if information transfer phase fails.
*/

LOCAL STATUS scsiCtrlMsgOutAction
    (
    SCSI_CTRL   *pScsiCtrl,		/* ptr to SCSI controller info */
    SCSI_THREAD *pThread		/* ptr to thread info */
    )
    {
    UINT xferCount;			/* number of bytes transferred */
    
    if ((pScsiCtrl->msgOutState == SCSI_MSG_OUT_SENT) &&
	(pScsiCtrl->msgOutLength > 1))
	{
	/* target is retrying: need to assert ATN (SCSI 5.1.9.2) */
	
	if ((*pScsiCtrl->scsiBusControl) (pScsiCtrl,
				          SCSI_BUS_ASSERT_ATN) != OK)
	    {
	    SCSI_ERROR_MSG ("scsiCtrlMsgOutAction: can't assert ATN.\n",
			    0, 0, 0, 0, 0, 0);
	    return (ERROR);
	    }
	}
    
    if (pScsiCtrl->msgOutState == SCSI_MSG_OUT_NONE)
	{
	pScsiCtrl->msgOutBuf[0] = SCSI_MSG_NO_OP;
	pScsiCtrl->msgOutLength = 1;
	}

    xferCount = (*pScsiCtrl->scsiInfoXfer) (pScsiCtrl,
					    SCSI_MSG_OUT_PHASE,
                                            pScsiCtrl->msgOutBuf,
                                            pScsiCtrl->msgOutLength);

    if (xferCount == ERROR)
	return (ERROR);

    if (xferCount == pScsiCtrl->msgOutLength)
	pScsiCtrl->msgOutState =  SCSI_MSG_OUT_SENT;

    return (OK);
    }


/******************************************************************************
*
* scsiCtrlMsgInAction - respond to an incoming SCSI message
*
* Read and handle an incoming message from the target.
*
* Note that if the incoming message is a SCSI Extended Message, it needs to
* be read in three chunks (the message type, the additional length and the
* extended message itself).  This is achieved by using a finite state machine
* which cycles through the chunks, returning to the phase sequencing code
* until the message is complete.
*
* RETURNS: OK, or ERROR if information transfer phase fails.
*/

LOCAL STATUS scsiCtrlMsgInAction
    (
    SCSI_CTRL   *pScsiCtrl,		/* ptr to SCSI controller info */
    SCSI_THREAD *pThread		/* ptr to thread info */
    )
    {
    /*
     *	Handle (possibly partial) message transfer
     */
    if (pScsiCtrl->msgOutState == SCSI_MSG_OUT_PENDING)
	SCSI_DEBUG_MSG ("scsiCtrlMsgInAction: msg in while msg out pending\n",
			0, 0, 0, 0, 0, 0);

    if (scsiCtrlMsgInXfer (pScsiCtrl) != OK)
	return (ERROR);

    /*
     *  If we have a complete message, parse it and respond appropriately
     */
    if (pScsiCtrl->msgInState == SCSI_MSG_IN_NONE)
        (void) scsiMsgInComplete (pScsiCtrl, pThread);

    /*
     *	Negate ACK to allow target to continue; if rejecting message,
     *	also assert ATN.
     */
    if (scsiCtrlMsgInAck (pScsiCtrl) != OK)
	{
	SCSI_ERROR_MSG ("scsiCtrlMsgInAction: scsiCtrlMsgInAck failed.\n",
			0, 0, 0, 0, 0, 0);
	return (ERROR);
	}

    return (OK);
    }


/******************************************************************************
*
* scsiCtrlMsgInXfer - handle MSG IN information transfer
*
* Note: does not necessarily read a complete message.
*
* Note: it is assumed that a message in transfer must be completed fully, i.e.
* the target cannot disconnect.  It is also assumed that the length of the
* message (fragment) does not exceed the maximum byte count the controller
* can handle.
*
* RETURNS: OK, or ERROR if transfer fails.
*/

LOCAL STATUS scsiCtrlMsgInXfer
    (
    SCSI_CTRL *pScsiCtrl		/* ptr to SCSI controller info */
    )
    {
    UINT xferCount;			/* number of bytes transferred */
    UINT maxBytes;			/* max number of bytes to read */

    SCSI_MSG_IN_STATE state = pScsiCtrl->msgInState;

    switch (state)
        {
    	case SCSI_MSG_IN_NONE:
	    pScsiCtrl->msgInLength = 0;
	    maxBytes = 1;
	    break;

	case SCSI_MSG_IN_SECOND_BYTE:
	    maxBytes = 1;
	    break;
	    
	case SCSI_MSG_IN_EXT_MSG_LEN:
	    maxBytes = 1;
	    break;

	case SCSI_MSG_IN_EXT_MSG_DATA:
	    if ((maxBytes = pScsiCtrl->msgInBuf[SCSI_EXT_MSG_LENGTH_BYTE]) == 0)
		maxBytes = SCSI_EXT_MSG_MAX_LENGTH;
	    break;

	default:
            SCSI_MSG ("scsiCtrlMsgInXfer: invalid state (%d)\n", state,
		      0, 0, 0, 0, 0);
	    pScsiCtrl->msgInState = SCSI_MSG_IN_NONE;
            return (ERROR);
	}
	    
    xferCount = (*pScsiCtrl->scsiInfoXfer) (pScsiCtrl,
					    SCSI_MSG_IN_PHASE,
					    pScsiCtrl->msgInBuf +
					        pScsiCtrl->msgInLength,
					    maxBytes);
    
    if (xferCount != maxBytes)
	{
	SCSI_ERROR_MSG ("scsiCtrlMsgInXfer: transfer failed\n",
			0, 0, 0, 0, 0, 0);
	
	if (xferCount != ERROR)
	    errnoSet (S_scsiLib_HARDWARE_ERROR);
	
	return (ERROR);
	}

    switch (state)
	{
        case SCSI_MSG_IN_NONE:
            if (pScsiCtrl->msgInBuf[0] == SCSI_MSG_EXTENDED_MESSAGE)
                state = SCSI_MSG_IN_EXT_MSG_LEN;

	    else if (SCSI_IS_TWO_BYTE_MSG (pScsiCtrl->msgInBuf[0]))
		state = SCSI_MSG_IN_SECOND_BYTE;

	    else
                state = SCSI_MSG_IN_NONE;
            break;

	case SCSI_MSG_IN_SECOND_BYTE:
	    state = SCSI_MSG_IN_NONE;
	    break;

        case SCSI_MSG_IN_EXT_MSG_LEN:
            state = SCSI_MSG_IN_EXT_MSG_DATA;
            break;

        case SCSI_MSG_IN_EXT_MSG_DATA:
            state = SCSI_MSG_IN_NONE;
            break;

        default:
            SCSI_MSG ("scsiCtrlMsgInXfer: invalid state (%d)\n", state,
		      0, 0, 0, 0, 0);
	    pScsiCtrl->msgInState = SCSI_MSG_IN_NONE;
            return (ERROR);
        }

    pScsiCtrl->msgInState   = state;
    pScsiCtrl->msgInLength += xferCount;
    
    return (OK);
    }


/*******************************************************************************
*
* scsiCtrlThreadComplete - complete execution of a client thread
*
* Set the thread status and errno appropriately, depending on whether or
* not the thread has been aborted.  Set the thread inactive, and notify
* the SCSI manager of the completion.
*
* RETURNS: N/A
*/
LOCAL void scsiCtrlThreadComplete
    (
    SCSI_THREAD * pThread
    )
    {
    SCSI_DEBUG_MSG ("scsiCtrlThreadComplete: thread 0x%08x completed\n",
		    (int) pThread, 0, 0, 0, 0, 0);

    if (pThread->state == SCSI_THREAD_WAIT_ABORT)
	{
	pThread->status = ERROR;
	pThread->errNum = S_scsiLib_ABORTED;
	}
    else
	{
    	pThread->status = OK;
    	pThread->errNum = 0;
    	}
    
    scsiCtrlThreadStateSet (pThread, SCSI_THREAD_INACTIVE);

    scsiCacheSynchronize (pThread, SCSI_CACHE_POST_COMMAND);

    scsiMgrThreadEvent (pThread, SCSI_THREAD_EVENT_COMPLETED);
    }


/*******************************************************************************
*
* scsiCtrlThreadDefer - defer execution of a thread
*
* Set the thread's state to INACTIVE and notify the SCSI manager of the
* deferral event.
*
* RETURNS: N/A
*/
LOCAL void scsiCtrlThreadDefer
    (
    SCSI_THREAD * pThread
    )
    {
    SCSI_DEBUG_MSG ("scsiCtrlThreadDefer: thread 0x%08x deferred\n",
		    (int) pThread, 0, 0, 0, 0, 0);

    scsiCtrlThreadStateSet (pThread, SCSI_THREAD_INACTIVE);

    scsiMgrThreadEvent (pThread, SCSI_THREAD_EVENT_DEFERRED);
    }
    
	
/*******************************************************************************
*
* scsiCtrlThreadFail - complete execution of a thread, with error status
*
* Set the thread's status and errno according to the type of error.  Set
* the thread's state to INACTIVE, and notify the SCSI manager of the
* completion event.
*
* RETURNS: N/A
*/
LOCAL void scsiCtrlThreadFail
    (
    SCSI_THREAD * pThread,
    int           errNum
    )
    {
    SCSI_DEBUG_MSG ("scsiCtrlThreadFail: thread 0x%08x failed (errno = %d)\n",
		    (int) pThread, errNum, 0, 0, 0, 0);

    pThread->status = ERROR;

    if (pThread->state == SCSI_THREAD_WAIT_ABORT)
	pThread->errNum = S_scsiLib_ABORTED;

    else
    	pThread->errNum = errNum;
    
    scsiCtrlThreadStateSet (pThread, SCSI_THREAD_INACTIVE);

    scsiMgrThreadEvent (pThread, SCSI_THREAD_EVENT_COMPLETED);
    }
    
	
/*******************************************************************************
*
* scsiCtrlThreadStateSet - set the state of a thread
*
* This is really just a place-holder for debugging and possible future
* enhancements such as state-change logging.
*
* RETURNS: N/A
*/
LOCAL void scsiCtrlThreadStateSet
    (
    SCSI_THREAD *     pThread,		/* ptr to thread info */
    SCSI_THREAD_STATE state
    )
    {
    SCSI_DEBUG_MSG ("scsiCtrlThreadStateSet: thread 0x%08x: %d -> %d\n",
		    (int) pThread, pThread->state, state, 0, 0, 0);

    pThread->state = state;
    }


/*******************************************************************************
*
* scsiCtrlXferParamsSet - set transfer parameters for a thread
*
* Call the controller-specific routine to set transfer parameters to the
* values required by this thread's target device.
*
* RETURNS: OK, or ERROR if transfer parameters cannot be set
*/
LOCAL STATUS scsiCtrlXferParamsSet
    (
    SCSI_THREAD * pThread
    )
    {
    SCSI_TARGET * pScsiTarget = pThread->pScsiTarget;
    SCSI_CTRL *   pScsiCtrl   = pThread->pScsiCtrl;
    
    /*
     *	Set transfer parameters for thread's target device
     */
    if ((*pScsiCtrl->scsiXferParamsSet) (pScsiCtrl,
					 pScsiTarget->xferOffset,
					 pScsiTarget->xferPeriod) != OK)
	{
	SCSI_ERROR_MSG ("scsiCtrlXferParamsSet: can't set transfer parameters.\n",
			0, 0, 0, 0, 0, 0);
	return (ERROR);
	}

    return (OK);
    }

/*******************************************************************************
*
* scsiCtrlMsgInAck - acknowledge an incoming message byte
*
* Call the controller-specific routine to negate the SCSI ACK signal, and
* assert the ATN signal if there is a pending message out.  Note that the
* controller driver must ensure that ATN is asserted, if necessary, before
* ACK is negated.
*
* RETURNS: OK, or ERROR if the signals could not be driven as required
*/
LOCAL STATUS scsiCtrlMsgInAck
    (
    SCSI_CTRL * pScsiCtrl
    )
    {
    UINT busCommand = SCSI_BUS_NEGATE_ACK;

    if (pScsiCtrl->msgOutState == SCSI_MSG_OUT_PENDING)
	{
	busCommand |= SCSI_BUS_ASSERT_ATN;
	}
    
    if ((*pScsiCtrl->scsiBusControl) (pScsiCtrl, busCommand) != OK)
	{
	SCSI_ERROR_MSG ("scsiCtrlMsgInAck: scsiBusControl failed.\n",
			0, 0, 0, 0, 0, 0);
	return (ERROR);
	}

    return (OK);
    }
