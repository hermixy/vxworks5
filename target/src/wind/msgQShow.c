/* msgQShow.c - message queue show routines */

/* Copyright 1990-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01v,06nov01,aeg  added display of VxWorks event information.
01u,26sep01,jws  move vxMP and vxFusion show & info rtn ptrs
                 to funcBind.c (SPR36055)
01t,18dec00,pes  Correct compiler warnings
01s,17mar99,jdi  doc: updated w/ info about proj facility (SPR 25727).
01r,19may98,drm  merged code from 3rd party to add distributed message queue 
                 support  - merged code was originally based on version 01o 
01q,17apr98,rlp  canceled msgQShow and msgQInfoGet modifications for backward
                 compatibility.
01p,04nov97,rlp  modified msgQShow and msgQInfoGet for tracking messages sent.
01o,24jun96,sbs  made windview instrumentation conditionally compiled
01n,10oct95,jdi  doc: added .tG Shell to SEE ALSO for msgQShow().
01m,20may94,dvs  added level check for task delay list in msgQShow() (SPR 2966)
01m,16jan94,c_s  msgQShowInit () now initializes instrumented class.
01l,03feb93,jdi  changed INCLUDE_SHOW_RTNS to ...ROUTINES.
01k,02feb93,jdi  another doc tweak.
01j,02feb93,jdi  documentation tweaks.
01i,23nov92,jdi  documentation cleanup.
01h,13nov92,dnw  added include of smObjLib.h
01g,17sep92,pme  made msgQInfoGet() handle shared message Queue.
01f,30jul92,smb  changed format for printf to avoid zero padding.
01e,29jul92,pme  added NULL function pointer check for smObj routines.
01d,28jul92,jcf  changed msgQShowInit to call msgQLibInit.
01c,19jul92,pme  added shared message queue support.
                 added #include "errnoLib.h".
01b,12jul92,jcf  changed level compare to >=
01a,15jun92,jcf  extracted from v1l of msgQLib.c.
*/

/*
DESCRIPTION
This library provides routines to show message queue statistics,
such as the task queuing method, messages queued, receivers blocked, etc.

The routine msgQshowInit() links the message queue show facility into the
VxWorks system.  It is called automatically when the message queue show
facility is configured into VxWorks using either of the
following methods:
.iP
If you use the configuration header files, define
INCLUDE_SHOW_ROUTINES in config.h.
.iP
If you use the Tornado project facility, select INCLUDE_MSG_Q_SHOW.
.LP

INCLUDE FILES: msgQLib.h

SEE ALSO: pipeDrv,
.pG "Basic OS"
*/

/* LINTLIBRARY */

#include "vxWorks.h"
#include "errnoLib.h"
#include "intLib.h"
#include "stdlib.h"
#include "stdarg.h"
#include "string.h"
#include "stdio.h"
#include "smObjLib.h"
#include "private/msgQLibP.h"
#include "private/kernelLibP.h"
#include "private/taskLibP.h"
#include "private/msgQSmLibP.h"
#include "private/distObjTypeP.h"
#include "private/eventLibP.h"
#include "vxfusion/private/msgQDistLibP.h"

/* globals */


/* forward declarations */

static BOOL msgQInfoEach (MSG_NODE *pNode, MSG_Q_INFO *pInfo);


/******************************************************************************
*
* msgQShowInit - initialize the message queue show facility
*
* This routine links the message queue show facility into the VxWorks system.
* It is called automatically when the message queue show facility is
* configured into VxWorks using either of the following methods:
* .iP
* If you use the configuration header files, define
* INCLUDE_SHOW_ROUTINES in config.h.
* .iP
* If you use the Tornado project facility, select INCLUDE_MSG_Q_SHOW.
*
* RETURNS: N/A
*/

void msgQShowInit (void)
    {
    if (msgQLibInit () == OK)
	{
        classShowConnect (msgQClassId, (FUNCPTR)msgQShow);

#ifdef WV_INSTRUMENTATION
	classShowConnect (msgQInstClassId, (FUNCPTR)msgQShow);
#endif

	}
    }

/*******************************************************************************
*
* msgQInfoEach - support routine for msgQInfoGet
*
* RETURNS: TRUE or FALSE
*/

LOCAL BOOL msgQInfoEach
    (
    MSG_NODE *          pNode,
    MSG_Q_INFO *        pInfo
    )
    {
    if (pInfo->msgPtrList != NULL)
	pInfo->msgPtrList [pInfo->numMsgs] = MSG_NODE_DATA (pNode);

    if (pInfo->msgLenList != NULL)
	pInfo->msgLenList [pInfo->numMsgs] = pNode->msgLength;

    /* bump list count and signal quit (return FALSE) if we're at max */

    return (++pInfo->numMsgs < pInfo->msgListMax);
    }

/*******************************************************************************
*
* msgQInfoGet - get information about a message queue
*
* This routine gets information about the state and contents of a message
* queue.  The parameter <pInfo> is a pointer to a structure of type MSG_Q_INFO
* defined in msgQLib.h as follows:
*
* .CS
*  typedef struct		/@ MSG_Q_INFO @/
*     {
*     int     numMsgs;		/@ OUT: number of messages queued            @/
*     int     numTasks;		/@ OUT: number of tasks waiting on msg q     @/
*     int     sendTimeouts;	/@ OUT: count of send timeouts               @/
*     int     recvTimeouts;	/@ OUT: count of receive timeouts            @/
*     int     options;		/@ OUT: options with which msg q was created @/
*     int     maxMsgs;		/@ OUT: max messages that can be queued      @/
*     int     maxMsgLength;	/@ OUT: max byte length of each message      @/
*     int     taskIdListMax;	/@ IN: max tasks to fill in taskIdList       @/
*     int *   taskIdList;	/@ PTR: array of task IDs waiting on msg q   @/
*     int     msgListMax;	/@ IN: max msgs to fill in msg lists         @/
*     char ** msgPtrList;	/@ PTR: array of msg ptrs queued to msg q    @/
*     int *   msgLenList;	/@ PTR: array of lengths of msgs             @/
*     } MSG_Q_INFO;
* .CE
*
* If a message queue is empty, there may be tasks blocked on receiving.
* If a message queue is full, there may be tasks blocked on sending.
* This can be determined as follows:
* .iP "" 4
* If <numMsgs> is 0, then <numTasks> indicates the number of tasks blocked
* on receiving.
* .iP
* If <numMsgs> is equal to <maxMsgs>, then <numTasks> is the number of
* tasks blocked on sending.
* .iP
* If <numMsgs> is greater than 0 but less than <maxMsgs>, then <numTasks> 
* will be 0.
* .LP
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
* ERRNO: S_distLib_NOT_INITIALIZED, S_smObjLib_NOT_INITIALIZED,
*        S_objLib_OBJ_ID_ERROR
*
*/

STATUS msgQInfoGet
    (
    MSG_Q_ID       msgQId,         /* message queue to query */
    MSG_Q_INFO *   pInfo           /* where to return msg info */
    )
    {
    int		level;
    Q_HEAD *	pendQ;

    if (ID_IS_SHARED (msgQId))			/* shared message Q */
        {
		if (ID_IS_DISTRIBUTED (msgQId))     /* distributed message Q */
			{
			if (msgQDistInfoGetRtn == NULL)
				{
				errno = S_distLib_NOT_INITIALIZED;
				return (ERROR);
				}
			return ((*msgQDistInfoGetRtn) (msgQId, pInfo));
			}

        if (msgQSmInfoGetRtn == NULL)
            {
            errno = S_smObjLib_NOT_INITIALIZED;
            return (ERROR);
            }

	return ((*msgQSmInfoGetRtn) (SM_OBJ_ID_TO_ADRS (msgQId), pInfo));
	}

    /* message queue is local */

    level = intLock ();				/* LOCK INTERRUPTS */

    if (OBJ_VERIFY (msgQId, msgQClassId) != OK)
	{
	intUnlock (level);			/* UNLOCK INTERRUPTS */
	return (ERROR);
	}

    /* tasks can be blocked on either the msg queue (receivers) or
     * on the free queue (receivers), but not both.  Here we determine
     * which queue to get task info from: if there are no msgs queued
     * then there might be receivers pended, otherwise there might be
     * senders pended.
     */

    if (msgQId->msgQ.count == 0)
	pendQ = &msgQId->msgQ.pendQ;	/* might be receivers pended */
    else
	pendQ = &msgQId->freeQ.pendQ;	/* might be senders pended */

    if (pInfo->taskIdList != NULL)
	{
	/* get list of tasks pended on this message queue */

	Q_INFO (pendQ, pInfo->taskIdList, pInfo->taskIdListMax);
	}

    if ((pInfo->msgPtrList != NULL) || (pInfo->msgLenList != NULL))
	{
	/* get list of messages queued to this message queue */

	/* note: we use numMsgs field to hold count while we are filling
	 * in list so msgQInfoEach can remember the current index and
	 * know when to stop.  Afterwards, we fill in the actual numMsgs.
	 */

	pInfo->numMsgs = 0;

	if (pInfo->msgListMax > 0)
	    (void)Q_EACH (&msgQId->msgQ, msgQInfoEach, pInfo);
	}

    pInfo->numMsgs		= msgQId->msgQ.count;
    pInfo->numTasks		= Q_INFO (pendQ, NULL, 0);	/* XXX ? */

    pInfo->options		= msgQId->options;
    pInfo->maxMsgs		= msgQId->maxMsgs;
    pInfo->maxMsgLength		= msgQId->maxMsgLength;

    pInfo->sendTimeouts		= msgQId->sendTimeouts;
    pInfo->recvTimeouts		= msgQId->recvTimeouts;


    intUnlock (level);				/* UNLOCK INTERRUPTS */

    return (OK);
    }

/*******************************************************************************
*
* msgQShow - show information about a message queue
*
* This routine displays the state and optionally the contents of a message
* queue.
*
* A summary of the state of the message queue is displayed as follows:
* .CS
*     Message Queue Id    : 0x3f8c20
*     Task Queuing        : FIFO
*     Message Byte Len    : 150
*     Messages Max        : 50
*     Messages Queued     : 0
*     Receivers Blocked   : 1
*     Send timeouts       : 0
*     Receive timeouts    : 0
*     Options             : 0x1       MSG_Q_FIFO
*
*     VxWorks Events
*     --------------
*     Registered Task     : 0x3f5c70 (t1)
*     Event(s) to Send    : 0x1
*     Options             : 0x7       EVENTS_SEND_ONCE
*                                     EVENTS_ALLOW_OVERWRITE
*                                     EVENTS_SEND_IF_FREE
* .CE
*
* If <level> is 1, then more detailed information will be displayed.
* If messages are queued, they will be displayed as follows:
* .CS
*     Messages queued:
*       #    address length value
*       1 0x123eb204    4   0x00000001 0x12345678
* .CE
*
* If tasks are blocked on the queue, they will be displayed as follows:
* .CS
*     Receivers blocked:
*
*        NAME      TID    PRI DELAY
*     ---------- -------- --- -----
*     tExcTask     3fd678   0   21
* .CE
*
* RETURNS: OK or ERROR.
*
* ERRNO: S_distLib_NOT_INITIALIZED, S_smObjLib_NOT_INITIALIZED
*
* SEE ALSO:
* .pG "Target Shell,"
* windsh,
* .tG "Shell"
*/

STATUS msgQShow
    (
    MSG_Q_ID    msgQId,         /* message queue to display */
    int         level           /* 0 = summary, 1 = details */
    )
    {
    MSG_Q_INFO	info;
    int		taskIdList [20];
    int		taskDList [20];
    char *	msgPtrList [20];
    int		msgLenList [20];
    WIND_TCB *	pTcb;
    STATUS	status;
    int	        lock;
    int	        ix;
    int	        ix2;
    char *	pMsg;
    int	        len;
    EVENTS_RSRC msgQEvResource;


    if (ID_IS_SHARED (msgQId))			/* shared message Q */
        {
		if (ID_IS_DISTRIBUTED (msgQId))     /* distributed message Q */
			{
			if (msgQDistShowRtn == NULL)
				{
				errno = S_distLib_NOT_INITIALIZED;
				return (ERROR);
				}
			return ((*msgQDistShowRtn) (msgQId, level));
			}

        if (msgQSmShowRtn == NULL)
            {
            errno = S_smObjLib_NOT_INITIALIZED;
            return (ERROR);
            }

	return ((*msgQSmShowRtn) (SM_OBJ_ID_TO_ADRS (msgQId), level));
	}

    /* message queue is local */

    bzero ((char *) &info, sizeof (info));

    if (level >= 1)
	{
	/* for detailed info, fill in array pointers in info structure */

	info.taskIdList = taskIdList;
	info.taskIdListMax = NELEMENTS (taskIdList);

	info.msgPtrList = msgPtrList;
	info.msgLenList = msgLenList;
	info.msgListMax = NELEMENTS (msgPtrList);
	}

    lock = intLock ();					/* LOCK INTERRUPTS */

    if ((status = msgQInfoGet (msgQId, &info)) == ERROR)
	{
	intUnlock (lock);				/* UNLOCK INTERRUPTS */
	printf ("Invalid message queue id: %#x\n", (int)msgQId);
	return (ERROR);
	}

    if ((info.numTasks > 0) && (level >= 1))		/* record task delays */
	{
	for (ix = 0; ix < min (info.numTasks, NELEMENTS (taskIdList)); ix++)
	    {
	    pTcb = (WIND_TCB *)(taskIdList [ix]);
	    if (pTcb->status & WIND_DELAY)
		taskDList[ix] = Q_KEY (&tickQHead, &pTcb->tickNode, 1);
	    else
		taskDList[ix] = 0;
	    }
	}

    msgQEvResource = msgQId->events;		/* record event info */ 

    intUnlock (lock);				/* UNLOCK INTERRUPTS */

    /* show summary information */

    printf ("\n");
    printf ("%-20s: 0x%-10x\n", "Message Queue Id", (int)msgQId);

    if ((info.options & MSG_Q_TYPE_MASK) == MSG_Q_FIFO)
	printf ("%-20s: %-10s\n", "Task Queuing", "FIFO");
    else
	printf ("%-20s: %-10s\n", "Task Queuing", "PRIORITY");

    printf ("%-20s: %-10d\n", "Message Byte Len", info.maxMsgLength);
    printf ("%-20s: %-10d\n", "Messages Max", info.maxMsgs);
    printf ("%-20s: %-10d\n", "Messages Queued", info.numMsgs);

    if (info.numMsgs == info.maxMsgs) 
	printf ("%-20s: %-10d\n", "Senders Blocked", info.numTasks);
    else
	printf ("%-20s: %-10d\n", "Receivers Blocked", info.numTasks);

    printf ("%-20s: %-10d\n", "Send timeouts", info.sendTimeouts);
    printf ("%-20s: %-10d\n", "Receive timeouts", info.recvTimeouts);
    printf ("%-20s: 0x%x\t%s", "Options", info.options,
	    		       (info.options & MSG_Q_TYPE_MASK) == MSG_Q_FIFO ?
				"MSG_Q_FIFO\n" : "MSG_Q_PRIORITY\n");
    if ((info.options & MSG_Q_EVENTSEND_ERR_NOTIFY) != 0)
	printf ("\t\t\t\tMSG_Q_EVENTSEND_ERR_NOTIFY\n");

    /* display VxWorks events information */

    eventRsrcShow (&msgQEvResource);

    /* show detailed information */

    if (level >= 1)
	{
	/* show blocked tasks */

	if (info.numTasks > 0)
	    {
	    printf ("\n%s Blocked:\n",
		    (info.numMsgs == info.maxMsgs) ? "Senders" : "Receivers");

	    printf ("\n");
	    printf ("   NAME      TID    PRI TIMEOUT\n");
	    printf ("---------- -------- --- -------\n");

	    for (ix = 0; ix < min (info.numTasks, NELEMENTS (taskIdList)); ix++)
		printf ("%-11.11s%8x %3d %7u\n", 
			taskName (taskIdList [ix]),
			taskIdList [ix],
			((WIND_TCB *)taskIdList [ix])->priority,
			taskDList[ix]);
	    }

	/* show queued messages */

	if (info.numMsgs > 0)
	    {
	    printf ("\nMessages queued:\n  #    address length value\n");

	    for (ix = 0; ix < min (info.numMsgs, NELEMENTS (msgPtrList)); ix++)
		{
		pMsg = msgPtrList [ix];
		len = msgLenList [ix];

		printf ("%3d %#10x  %4d ", ix + 1, (int)pMsg, len);

		for (ix2 = 0; ix2 < min (len, 20); ix2++)
		    {
		    if ((ix2 % 4) == 0)
			printf (" 0x");

		    printf ("%02x", pMsg [ix2] & 0xff);
		    }

		if (len > 20)
		    printf (" ...");

		printf ("\n");
		}
	    }
	}

    printf ("\n");

    return (OK);
    }
