/* msgQSmShow.c - shared message queue show utility (VxMP Option) */

/* Copyright 1990-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01k,06may02,mas  cache flush and volatile fix (SPR 68334); bridge flush fix
		 (SPR 68844)
01j,24oct01,mas  doc update (SPR 71149)
01i,17apr98,rlp  canceled msgQSmShow and msgQSmInfoGet modifications for
                 backward compatibility.
01h,04nov97,rlp  modified msgQSmShow and msgQSmInfoGet for tracking messages
                 sent.
01g,03feb93,jdi  changed INCLUDE_SHOW_RTNS to ...ROUTINES.
01f,29jan93,pme  added little endian support.
01e,13nov92,dnw  added include of smObjLib.h and smObjLibP.h
01d,02oct92,pme  added SPARC support. documentation cleanup.
01c,17sep92,pme  added msgQSmInfoGet and msgQSmInfoEach.
                 implemented msgQSmShow.
01b,28jul92,jcf  changed msgQSmShowInit to call msgQLibInit.
01a,19jul92,pme  written
*/

/*
DESCRIPTION
This library provides routines to show shared message queue statistics,
such as task queueing method, messages queued, receivers blocked,
and so forth.

These routines are included automatically by including the component
INCLUDE_SM_OBJ.

There are no user callable routines.

AVAILABILITY: This module is distributed as a component of the unbundled shared
objects memory support option, VxMP.

INCLUDE FILE: msgQSmLib.h

SEE ALSO: msgQSmLib msgQLib smObjLib
\tb VxWorks Programmer's Guide: Shared Memory Objects",
\tb VxWorks Programmer's Guide: Basic OS

NOROUTINES
*/

/* LINTLIBRARY */

#include "vxWorks.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "cacheLib.h"
#include "smObjLib.h"
#include "netinet/in.h"
#include "private/smObjLibP.h"
#include "private/msgQSmLibP.h"

/* forward declarations */

LOCAL BOOL msgQSmInfoEach (SM_MSG_NODE * pNode, MSG_Q_INFO * pInfo);

/*****************************************************************************
*
* msgQSmShowInit - initialize shared message queues show routine
*
* This routine links the shared memory message queues show and information get
* routines into the VxWorks system.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void msgQSmShowInit (void)
    {
    if (msgQLibInit () == OK)
	{
	/*
	 * Initialize info and show routine pointer to allow msgQShow and
	 * msgQInfo to handle shared message queues ids.
	 */

	msgQSmShowRtn    = (FUNCPTR) msgQSmShow;
	msgQSmInfoGetRtn = (FUNCPTR) msgQSmInfoGet;
	}
    }

/*****************************************************************************
*
* msgQSmInfoEach - support routine for msgQSmInfoGet
*
* This routine fills the info structure at <pInfo> with the pointer to message
* and message length.  It is called by smDllEach() for each message in the
* message Queue.
*
* RETURNS: TRUE or FALSE
*
* NOMANUAL
*/

LOCAL BOOL msgQSmInfoEach
    (
    SM_MSG_NODE *	pNode,
    MSG_Q_INFO *	pInfo
    )
    {
    if (pInfo->msgPtrList != NULL)
        {
        pInfo->msgPtrList [pInfo->numMsgs] = SM_MSG_NODE_DATA (pNode);
        }

    if (pInfo->msgLenList != NULL)
        {
        pInfo->msgLenList [pInfo->numMsgs] = ntohl (pNode->msgLength);
        }

    /* bump list count and signal quit (return FALSE) if we're at max */

    return (++pInfo->numMsgs < pInfo->msgListMax);
    }

/*****************************************************************************
*
* msgQSmInfoGet - get information about a shared message queue
*
* This routine gets information about the state and contents of a shared
* message queue.  The parameter <pInfo> is a pointer to a structure of type 
* MSG_Q_INFO defined in msgQLib.h as follows:
*
* \cs
*  typedef struct               /@ MSG_Q_INFO @/
*     {
*     int     numMsgs;          /@ OUT: number of messages queued @/
*     int     numTasks;         /@ OUT: number of tasks waiting on msg q @/
*     int     sendTimeouts;     /@ OUT: count of send timeouts @/
*     int     recvTimeouts;     /@ OUT: count of receive timeouts @/
*     int     options;          /@ OUT: options with which msg q was created @/
*     int     maxMsgs;          /@ OUT: max messages that can be queued @/
*     int     maxMsgLength;     /@ OUT: max byte length of each message @/
*     int     taskIdListMax;    /@ IN: max tasks to fill in taskIdList @/
*     int *   taskIdList;       /@ PTR: array of task IDs waiting on msg q @/
*     int     msgListMax;       /@ IN: max msgs to fill in msg lists @/
*     char ** msgPtrList;       /@ PTR: array of msg ptrs queued to msg q @/
*     int *   msgLenList;       /@ PTR: array of lengths of msgs @/
*    } MSG_Q_INFO;
* \ce
*
* If a message queue is empty, there may be tasks blocked on receiving.
* If a message queue is full, there may be tasks blocked on sending.
* This can be determined as follows:
* \is
* \i `If'
* <numMsgs> is 0, then <numTasks> indicates the number of tasks blocked
* on receiving.
* \i `Else If'
* <numMsgs> is equal to <maxMsgs>, then <numTasks> is the number of
* tasks blocked on sending.
* \i `Else If'
* <numMsgs> is greater than 0 but less than <maxMsgs>, <numTasks> will be 0.
* \ie
*
* A list of pointers to the messages queued and their lengths can be
* obtained by setting <msgPtrList> and <msgLenList> to the addresses of
* arrays to receive the respective lists, and setting <msgListMax> to
* the maximum number of elements in those arrays.  If either list pointer
* is NULL, no data will be returned for that array.
*
* No more than <msgListMax> message pointers and lengths are returned,
* although <numMsgs> will always be returned with the actual number of messages
* queued.
*
* For example, if the caller supplies a <msgPtrList> and <msgLenList>
* with room for 10 messages and sets <msgListMax> to 10, but there are 20
* messages queued, then the pointers and lengths of the first 10 messages in
* the queue are returned in <msgPtrList> and <msgLenList>, but <numMsgs> will
* be returned with the value 20.
*
* A list of the task IDs of tasks blocked on the message queue can be obtained
* by setting <taskIdList> to the address of an array to receive the list, and
* setting <taskIdListMax> to the maximum number of elements in that array.
* If <taskIdList> is NULL, then no task IDs are returned.  No more than
* <taskIdListMax> task IDs are returned, although <numTasks> will always
* be returned with the actual number of tasks blocked.
*
* For example, if the caller supplies a <taskIdList> with room for 10 task IDs
* and sets <taskIdListMax> to 10, but there are 20 tasks blocked on the
* message queue, then the IDs of the first 10 tasks in the blocked queue
* will be returned in <taskIdList>, but <numTasks> will be returned with
* the value 20.
*
* Note that the tasks returned in <taskIdList> may be blocked for either send
* or receive.  As noted above this can be determined by examining <numMsgs>.
*
* The variables <sendTimeouts> and <recvTimeouts> are the counts of the number
* of times msgQSend() and msgQReceive() respectively returned with a timeout.
*
* The variables <options>, <maxMsgs>, and <maxMsgLength> are the parameters
* with which the message queue was created.
*
* WARNING
*
* The information returned by this routine is not static and may be
* obsolete by the time it is examined.  In particular, the lists of
* task IDs and/or message pointers may no longer be valid.  However,
* the information is obtained atomically, thus it will be an accurate
* snapshot of the state of the message queue at the time of the call.
* This information is generally used for debugging purposes only.
*
* WARNING
* The current implementation of this routine locks out interrupts while
* obtaining the information.  This can compromise the overall interrupt
* latency of the system.  Generally this routine is used for debugging
* purposes only.
*
* RETURNS: OK or ERROR.
*
* ERRNO: S_objLib_OBJ_ID_ERROR, S_smObjLib_LOCK_TIMEOUT
*
* NOMANUAL
*/

STATUS msgQSmInfoGet
    (
    SM_MSG_Q_ID    smMsgQId,	/* shared message queue to query */
    MSG_Q_INFO *   pInfo	/* where to return msg info */
    )
    {
    int                  level;           /* interrupt lock return value */
    SM_SEM_ID            pendSem;         /* shared sem on which tasks pend */
    int                  numPendTasks;    /* number of pended tasks */
    int                  smTcbList [100]; /* shared TCBs pending on msg Q */
    SM_MSG_Q_ID volatile smMsgQIdv = (SM_MSG_Q_ID volatile) smMsgQId;
    int                  temp;            /* temp storage */

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    temp = smMsgQIdv->verify;                   /* PCI bridge bug [SPR 68844]*/

    if (SM_OBJ_VERIFY (smMsgQIdv) != OK)
	{
	return (ERROR);
	}

    /* 
     * Tasks can be blocked on either the msg queue (receivers) or
     * on the free queue (receivers), but not both.  Here we determine
     * which queue to get task info from: if there are no msgs queued
     * then there might be receivers pended on the msg Q semaphore,
     * otherwise there might be senders pended on the free Q semaphore.
     */
    
    temp = smMsgQIdv->msgQSem.state.count;      /* PCI bridge bug [SPR 68844]*/
    if (ntohl (smMsgQIdv->msgQSem.state.count) == 0)
        {
	pendSem = &smMsgQId->msgQSem;	/* might be receivers pended */
        }
    else
        {
	pendSem = &smMsgQId->freeQSem;	/* might be senders pended */
        }

    /* 
     * Get number and list of pended tasks by calling semSmInfo() on
     * the appropriate semaphore embedded in the message Queue.
     * If only the number of tasks is needed we use an internal
     * list, smTcbList, to call semSmInfo().
     */
     	
    if (pInfo->taskIdList != NULL)
	{
	numPendTasks = semSmInfo (pendSem, pInfo->taskIdList, 
				     pInfo->taskIdListMax);
	}
    else 
	{
	numPendTasks = semSmInfo (pendSem, smTcbList, 100);
	}

    /* 
     * There is a possible inconsistency in the returned information
     * if another CPU sends or receives a message between the time
     * we get the number of tasks pended on the shared semaphore
     * and the time we go through the list of messages.
     * We can live with this problem since this routine
     * is only here to give a snapshot of the status of the message
     * queue and ID used only for debug.
     */

    if ((pInfo->msgPtrList != NULL) || (pInfo->msgLenList != NULL))
	{
	/* 
	 * Get list of messages queued to this message queue, before
	 * going through the message list we lock it access to avoid
	 * another CPU modifying the list.
	 */

	/* ENTER LOCKED SECTION */

    	if (SM_OBJ_LOCK_TAKE (&smMsgQId->msgQLock, &level) != OK)
            {
            smObjTimeoutLogMsg ("msgQInfoGet", (char *) &smMsgQId->msgQLock);
            return (ERROR);                         /* can't take lock */
            }

        /* 
	 * Note: we use numMsgs field to hold count while we are filling
         * in list so msgQSmInfoEach() can remember the current index and
         * know when to stop.  Afterwards, we fill in the actual numMsgs.
         */

	pInfo->numMsgs = 0;

	if (pInfo->msgListMax > 0)
	    {
	    (void) smDllEach (&smMsgQId->msgQ, (FUNCPTR) msgQSmInfoEach, 
			      (int) pInfo);
	    }

	/* EXIT LOCKED SECTION */

    	SM_OBJ_LOCK_GIVE (&smMsgQId->msgQLock, level);/* release lock */
	}

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    temp = smMsgQIdv->msgQSem.state.count;      /* PCI bridge bug [SPR 68844]*/

    pInfo->numMsgs	= ntohl (smMsgQIdv->msgQSem.state.count);
    pInfo->numTasks	= numPendTasks;

    pInfo->options	= ntohl (smMsgQIdv->options);
    pInfo->maxMsgs	= ntohl (smMsgQIdv->maxMsgs);
    pInfo->maxMsgLength	= ntohl (smMsgQIdv->maxMsgLength);

    pInfo->sendTimeouts	= ntohl (smMsgQIdv->sendTimeouts);
    pInfo->recvTimeouts	= ntohl (smMsgQIdv->recvTimeouts);

    return (OK);
    }

/*****************************************************************************
*
* msgQSmShow - show information about a message queue
*
* This routine displays the state and optionally the contents of a shared 
* message queue <smMsgQId>.
*
* A summary of the state of the message queue is displayed as follows:
* \cs
*
*	Message Queue Id    : 0x7f8c21
*	Task Queuing        : FIFO
*	Message Byte Len    : 128
*	Messages Max        : 10
*	Messages Queued     : 0
*	Receivers Blocked   : 1
*	Send timeouts       : 0
*	Receive timeouts    : 0
*
* \ce
*
* If <level> is 1, then more detailed information will be displayed.
* If messages are queued, they will be displayed as follows:
* \cs
*
*	Messages queued:
*	  #  local adrs length value
*	  1  0x123eb204    4   0x00000001 0x12345678
* \ce
*
* If tasks are blocked on the queue, they will be displayed as follows:
* \cs
*
*	Receivers blocked:
*
*	TID        CPU Number Shared TCB
*	---------- ---------- ----------
*	0xd0618        1      0x1364204
*
* \ce
*
* RETURNS: OK or ERROR.
*
* ERRNO: S_objLib_OBJ_ID_ERROR, S_smObjLib_LOCK_TIMEOUT
*
* NOMANUAL
*/

STATUS msgQSmShow 
    (
    SM_MSG_Q_ID smMsgQId,	/* message queue to display */
    int         level		/* 0 = summary, 1 = details */
    )
    {
    MSG_Q_INFO	info;		/* where to put msg Q informations */
    int 	smTcbList [40];	/* array of shared TCB pending on msg Q */
    int		msgPtrList [40];/* array of pointer to messages */
    int 	msgLenList [40];/* array of message length */
    int		ix;		/* useful counter */
    int		ix2;		/* useful counter */
    char *	pMsg;		/* pointer to message content */
    int		len;		/* message length */

    bzero ((char *) &info, sizeof (info));

    if (level >= 1)
        {
        /* for detailed info, fill in array pointers in info structure */

        info.msgPtrList 	= (char **) msgPtrList;
        info.msgLenList 	= msgLenList;
        info.msgListMax 	= NELEMENTS (msgPtrList);
        }

    /* 
     * Even if we don't want a detailed list of tasks pended we need
     * to fill taskIdList pointer to use semSmInfoGet to get the
     * number of pended tasks.
     */

    info.taskIdList 	= smTcbList;
    info.taskIdListMax 	= NELEMENTS (smTcbList);

    /* get informations about message queue */

    if (msgQSmInfoGet (smMsgQId, &info) == ERROR)
        {
        printf ("Invalid message queue id: %#x\n", (unsigned int) smMsgQId);
        return (ERROR);
        }

    /* show summary information */

    printf ("\n");
    printf ("%-20s: 0x%-10x\n", "Message Queue Id", 
	    SM_OBJ_ADRS_TO_ID (smMsgQId));

    if ((info.options & MSG_Q_TYPE_MASK) == MSG_Q_FIFO)
        {
        printf ("%-20s: %-10s\n", "Task Queuing", "FIFO");
        }
    else
        {
        printf ("%-20s: %-10s\n", "Task Queuing", "PRIORITY");
        }

    printf ("%-20s: %-10d\n", "Message Byte Len", info.maxMsgLength);
    printf ("%-20s: %-10d\n", "Messages Max", info.maxMsgs);
    printf ("%-20s: %-10d\n", "Messages Queued", info.numMsgs);

    if (info.numMsgs == info.maxMsgs)
        {
        printf ("%-20s: %-10d\n", "Senders Blocked", info.numTasks);
        }
    else
        {
        printf ("%-20s: %-10d\n", "Receivers Blocked", info.numTasks);
        }

    printf ("%-20s: %-10d\n", "Send timeouts", info.sendTimeouts);
    printf ("%-20s: %-10d\n", "Receive timeouts", info.recvTimeouts);
    printf ("\n");

    /* show detailed information */

    if (level >= 1)
        {
        /* show blocked tasks */

        if (info.numTasks > 0)
            {
            printf ("%s Blocked:\n",
                    (info.numMsgs == info.maxMsgs) ? "Senders" : "Receivers");

            printf ("\n");
            printf ("   TID     CPU Number Shared TCB\n");
            printf ("---------- ---------- ----------\n");

            for (ix = 0; ix < min (info.numTasks, NELEMENTS (smTcbList)); ix++)
                {
                printf ("%#-10x    %2d      %#-10x\n",
                        ntohl ((int)(((SM_OBJ_TCB *)smTcbList [ix])->localTcb)),
                        ntohl (((SM_OBJ_TCB *)smTcbList [ix])->ownerCpu),
                        (unsigned int) smTcbList[ix]);
                }
	    printf ("\n");
            }

        /* show queued messages */

        if (info.numMsgs > 0)
            {
            printf ("Messages queued:\n  # local adrs length value\n");

            for (ix = 0; ix < min (info.numMsgs, NELEMENTS (msgPtrList)); ix++)
                {
                pMsg = (char *) msgPtrList [ix];
                len  = msgLenList [ix];

                printf ("%3d %#10x  %4d   ", ix + 1, (unsigned int) pMsg, len);

                for (ix2 = 0; ix2 < min (len, 20); ix2++)
                    {
                    if ((ix2 % 4) == 0)
                        {
                        printf (" 0x");
                        }

                    printf ("%02x", pMsg [ix2] & 0xff);
                    }

                if (len > 20)
                    {
                    printf (" ...");
                    }

                printf ("\n");
                }
            }
        }

    printf ("\n");

    return (OK);
    }

