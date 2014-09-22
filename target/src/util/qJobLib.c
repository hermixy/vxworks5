/* qJobLib.c - fifo queue with singly-linked lists with intLocks */

/* Copyright 1990-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01w,19oct01,bwa  Added MSG_Q_EVENTSEND_ERR_NOTIFY options support.
01v,06sep00,bwa  added extra argument to qJobGet() (msgQid)
                 also added an OBJ_VERIFY() on that msgQId (SPR #20195)
01u,07sep01,bwa  Added VxWorks events support.
01t,24jun96,sbs  made windview instrumentation conditionally compiled
01s,14mar95,rdc  fixed MSG_OFFSET macro.
01r,02feb94,smb  corrected side effect of errno and OBJ_VERIFY SPR #2984
		 corrected modhist 01q
01q,24jan94,smb  added instrumentation macros
01p,18jan94,smb  instrumentation corrections for msgQDelete
01o,17jan94,smb  instrumentation corrections
01n,10dec93,smb  added instrumentation for windView
01m,23aug92,jcf  cleanup.
01l,30jul92,rrr  added restarting, put in msgQ fix.
01k,18jul92,smb  Changed errno.h to errnoLib.h.
01j,04jul92,jcf  scalable/ANSI/cleanup effort.
01i,26may92,rrr  the tree shuffle
01h,19nov91,rrr  shut up some ansi warnings.
01g,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed VOID to void
		  -changed copyright notice
01f,21aug91,wmd  Conditionalized define of PORTABLE flag for MIPS.
01e,10aug91,del  changed interface to qInit to pass all "optional" args.
01d,07jun91,wmd  Conditionalized define of PORTABLE flag for CPU!=SPARC.
01c,10aug90,dnw  changed qJobPut() to void.
01b,18jul90,dnw  added missing call of windPendQTerminate() to qJobTerminate()
01a,01may90,dnw  written by modifying qFifoLib.c
*/

/*
DESCRIPTION
Deviations for standard queue defs:
    qCreate/Init type
    qGet timeout
    size (bigger than Q_HEAD)

.CS
windview INSTRUMENTATION
------------------------
This Library should not be used by anything other than message queues.

        LEVEL 1 N/A

        LEVEL 2
                qJobTerminate causes EVENT_OBJ_MSGDELETE
                qJobPut causes EVENT_OBJ_MSGSEND
                qJobGet causes EVENT_OBJ_MSGRECEIVE

        LEVEL 3 N/A
.CE
*/

#include "vxWorks.h"
#include "qJobLib.h"
#include "objLib.h"
#include "stdlib.h"
#include "intLib.h"
#include "errnoLib.h"
#include "eventLib.h"
#include "private/windLibP.h"
#include "private/workQLibP.h"
#include "private/sigLibP.h"
#include "private/msgQLibP.h"
#include "private/eventP.h"


/* forward static functions */

LOCAL STATUS qJobNullRtn (void);
LOCAL void qJobPendQGet (MSG_Q_ID msgQId, Q_JOB_HEAD *pQHead);

/* locals */

LOCAL Q_CLASS qJobClass =
    {
    (FUNCPTR)qJobCreate,
    (FUNCPTR)qJobInit,
    (FUNCPTR)qJobDelete,
    (FUNCPTR)qJobTerminate,
    (FUNCPTR)qJobPut,
    (FUNCPTR)qJobGet,
    (FUNCPTR)qJobNullRtn,	/* no remove routine */
    (FUNCPTR)qJobNullRtn,
    (FUNCPTR)qJobNullRtn,
    (FUNCPTR)qJobNullRtn,
    (FUNCPTR)qJobNullRtn,
    (FUNCPTR)qJobNullRtn,
    (FUNCPTR)qJobInfo,
    (FUNCPTR)qJobEach,
    &qJobClass
    };

/* globals */

Q_CLASS_ID qJobClassId = &qJobClass;

#ifdef WV_INSTRUMENTATION
/* defines */
/* windview - this is needed by event logging in qJobLib */
#define MSGQ_VERIFY(objId)                                   		 \
    (                                                                    \
    ((((OBJ_ID)(objId))->pObjClass == msgQClassId) ||  			 \
     (((OBJ_ID)(objId))->pObjClass == (CLASS_ID)(msgQClassId->initRtn))	 \
    )                                                                    \
        ?                                                                \
            OK                                                           \
        :                                                                \
            ERROR				                         \
    )

#define MSG_OFFSET   (OFFSET(MSG_Q, msgQ))
#define FREE_OFFSET   (OFFSET(MSG_Q, freeQ))
#endif

/* local defines */

/******************************************************************************
*
* qJobCreate - create a job queue
*/

Q_JOB_HEAD *qJobCreate
    (
    Q_CLASS_ID pendQType        /* queue class for task pend queue */
    )
    {
    FAST Q_JOB_HEAD *pQHead = (Q_JOB_HEAD *) malloc (sizeof (Q_JOB_HEAD));

    if (pQHead == NULL)
	return (NULL);

    if (qJobInit (pQHead, pendQType) != OK)
	{
	free ((char *) pQHead);
	return (NULL);
	}

    return (pQHead);
    }

/******************************************************************************
*
* qJobInit -
*/

STATUS qJobInit
    (
    FAST Q_JOB_HEAD *pQHead,    /* queue head to initialize */
    Q_CLASS_ID pendQType        /* queue class for task pend queue */
    )
    {
    pQHead->first	= NULL;
    pQHead->last	= NULL;
    pQHead->count	= 0;

    qInit (&pQHead->pendQ, pendQType, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    return (OK);
    }

/*******************************************************************************
*
* qJobDelete -
*/

STATUS qJobDelete
    (
    Q_JOB_HEAD *pQHead
    )
    {
    qJobTerminate (pQHead);
    free ((char *) pQHead);
    return OK;
    }

/*******************************************************************************
*
* qJobTerminate -
*/

STATUS qJobTerminate
    (
    Q_JOB_HEAD *pQHead          /* queue head to terminate */
    )
    {
    windPendQTerminate (&pQHead->pendQ);

    return (OK);
    }
/*******************************************************************************
*
* qJobPut -
*/

STATUS qJobPut
    (
    MSG_Q_ID    msgQId,
    Q_JOB_HEAD *pQHead,
    Q_JOB_NODE *pNode,
    int         key
    )
    {
    FAST int level;

    if (key == 0)
	{
	/* add node to tail of queue */

	pNode->next = NULL;

	level = intLock ();			/* LOCK INTERRUPTS */

	if (pQHead->first == NULL)
	    pQHead->last = pQHead->first = pNode;
	else
	    {
	    pQHead->last->next = pNode;
	    pQHead->last = pNode;
	    }
	}
    else
	{
	/* add node to head of queue */

	level = intLock ();			/* LOCK INTERRUPTS */

	if ((pNode->next = pQHead->first) == NULL)
	    pQHead->last = pNode;

	pQHead->first = pNode;
	}

    pQHead->count++;

    /* interrupts are still locked out */

    if (kernelState == TRUE)			/* interrupted kernel? */
	{
	intUnlock (level);			/* UNLOCK INTERRUPTS */

	workQAdd2 ((FUNCPTR)qJobPendQGet, (int)msgQId, (int)pQHead);
        }
    else 
	{
	if (Q_FIRST (&pQHead->pendQ) == NULL)	/* anybody waiting for msg? */
	    {
	    /*
	     * look if we are putting back the node on the msg Q. If so, it
	     * means that we are putting a msg on the queue and since no task
	     * is waiting for it, we have to send events.
	     */

	    if (pQHead == &msgQId->msgQ)
		{
		if (msgQId->events.taskId != (int)NULL)
		    {
		    STATUS evStatus;
		    STATUS retStatus = OK;
		    int oldErrno = errno;

		    kernelState = TRUE;		/* KERNEL ENTER */
		    intUnlock (level);		/* UNLOCK INTERRUPTS */

		    evStatus = eventRsrcSend(msgQId->events.taskId,
					     msgQId->events.registered);

		    if (evStatus != OK)
	    		{
			if ((msgQId->options & MSG_Q_EVENTSEND_ERR_NOTIFY) != 0)
			    {
	    		    oldErrno = S_eventLib_EVENTSEND_FAILED;
			    retStatus = ERROR;
			    }

			/*
	    		 * Error, so remove registration so that next time the
	     		 * queue becomes empty it doesn't try to send events.
	     		 */

	    		msgQId->events.taskId = (int)NULL;
	    		}
		    else if ((msgQId->events.options & EVENTS_SEND_ONCE) != 0x0)
	    		msgQId->events.taskId = (int)NULL;

		    windExit ();		/* KERNEL EXIT */

		    errnoSet (oldErrno);
		    return (retStatus);
		    }
		else
		    intUnlock (level);		/* UNLOCK INTERRUPTS */
		}
	    else
		intUnlock (level);		/* UNLOCK INTERRUPTS */
	    }
	else
	    {
	    kernelState = TRUE;			/* KERNEL ENTER */
	    intUnlock (level);			/* UNLOCK INTERRUPTS */

#ifdef WV_INSTRUMENTATION
             /* windview - level 2 instrumentation */

	     if (pQHead == &msgQId->msgQ)
	         EVT_TASK_1 (EVENT_OBJ_MSGSEND, msgQId);
	     else
	         EVT_TASK_1 (EVENT_OBJ_MSGRECEIVE, msgQId);
#endif

	    windPendQGet (&pQHead->pendQ);	/* unblock receiver */

	    windExit ();			/* KERNEL EXIT */
	    }
	}

    return (OK);
    }

/*******************************************************************************
*
* qJobPendQGet - work routine to unblock a receiver (if any)
*
* If a task is waiting then unblock first task in pendQ.  This routine must
* be called with kernelState = TRUE.  It is called as deferred work from
* qJobPut().
*
*/
 
LOCAL void qJobPendQGet
    (
    MSG_Q_ID msgQId,
    Q_JOB_HEAD *pQHead
    )
    {
    /* this IS a msgQSend since msgQReceive can't be done from ISR */

    if (Q_FIRST (&pQHead->pendQ) != NULL)       /* any receivers? */
	{

#ifdef WV_INSTRUMENTATION
     	/* windview - level 2 instrumentation */

	EVT_TASK_1 (EVENT_OBJ_MSGSEND,msgQId);
#endif

	windPendQGet (&pQHead->pendQ);          /* unblock receiver */
	}
    else
	{
	if (msgQId->events.taskId != (int)NULL)
	    {
	    if (eventRsrcSend (msgQId->events.taskId,
			       msgQId->events.registered) != OK)
	    	{
		/*
	    	 * Error, so remove registration so that next time the
	     	 * queue becomes empty it doesn't try to send events.
	     	 */

	    	msgQId->events.taskId = (int)NULL;
		return;
	    	}
	    else if ((msgQId->events.options & EVENTS_SEND_ONCE) != 0x0)
	  	msgQId->events.taskId = (int)NULL;
	    }
	}
    }

/*******************************************************************************
*
* qJobGet -
*
* WORKS FROM INT LVL IF TIMEOUT == 0!
*
* RETURNS: pNode or NULL if empty list
*
* ERRNO: S_objLib_OBJ_UNAVAILABLE
*
*/

Q_JOB_NODE *qJobGet
    (
    MSG_Q_ID msgQId,
    FAST Q_JOB_HEAD *pQHead,
    FAST int timeout
    )
    {
    int		level;
    int		status;
    Q_JOB_NODE *pNode;

    level = intLock ();				/* LOCK INTERRUPTS */

    while ((pNode = pQHead->first) == NULL)
	{
	if (timeout == 0)
	    {
	    intUnlock (level);			/* UNLOCK INTERRUPTS */
	    errnoSet (S_objLib_OBJ_UNAVAILABLE);
	    return (NULL);
	    }
	/* assume not at intlvl since timeout != 0 (?) */

	kernelState = TRUE;			/* KERNEL ENTER */
	intUnlock (level);			/* UNLOCK INTERRUPTS */

#ifdef WV_INSTRUMENTATION
        /* windview - level 2 instrumentation 
         * EVENT_OBJ_MSGRECEIVE needs to return the msgQId so MSG_OFFSET is
         * used to calulate the msgQId from the pQHead
     	 * EVENT_OBJ_MSGSEND needs FREE_OFFSET to calulate the 
	 * msgQId from the pQHead
	 *
	 * msgQDelete() also call this routine but the msgQId will be 
	 * an invalid id. and thus no events will get logged.
         */
        if (MSGQ_VERIFY ((char *) pQHead - (MSG_OFFSET)) == OK)
       	    EVT_TASK_1 (EVENT_OBJ_MSGRECEIVE, 
	                ((char *)pQHead - (MSG_OFFSET)));
	else
            if (MSGQ_VERIFY ((char *) pQHead - (FREE_OFFSET)) == OK)
    	        EVT_TASK_1 (EVENT_OBJ_MSGSEND,
			    ((char *)pQHead - (FREE_OFFSET)));
#endif

	windPendQPut (&pQHead->pendQ, timeout);

	if ((status = windExit ()) == RESTART)	/* KERNEL EXIT */
	    return ((Q_JOB_NODE *) NONE);	/* restart */

	if (status != OK)
	    return (NULL);			/* timeout */

	if (OBJ_VERIFY (msgQId, msgQClassId) != OK)
            {
            errnoSet (S_objLib_OBJ_DELETED); /* queue got deleted */
	    return NULL;
            }

	level = intLock ();			/* LOCK INTERRUPTS */
	}


    /* exit above loop with queue not empty (pNode != NULL) and
     * interrupts locked out
     */

    pQHead->first = pNode->next;
    pQHead->count--;

    intUnlock (level);		/* UNLOCK INTERRUPTS */

    return (pNode);
    }

/*******************************************************************************
*
* qJobInfo -
*
* ARGSUSED
*/

int qJobInfo
    (
    Q_JOB_HEAD *pQHead,         /* job queue to gather list for */
    FAST int nodeArray[],       /* array of node pointers to be filled in */
    FAST int maxNodes           /* max node pointers nodeArray can accomodate */
    )
    {
    FAST Q_JOB_NODE *pNode;
    FAST int *pElement;
    FAST int level;

    if (nodeArray == NULL)
	return (pQHead->count);

    pElement = nodeArray;

    level = intLock ();			/* LOCK INTERRUPTS */

    pNode = pQHead->first;

    while ((pNode != NULL) && (--maxNodes >= 0))
	{
	*(pElement++) = (int)pNode;			/* fill in table */
	pNode = pNode->next;				/* next node */
	}

    intUnlock (level);			/* UNLOCK INTERRUPTS */

    return (pElement - nodeArray);	/* return count of nodes */
    }

/*******************************************************************************
*
* qJobEach - call a routine for each node in a queue
*
* This routine calls a user-supplied routine once for each node in the
* queue.  The routine should be declared as follows:
* .CS
*  BOOL routine (pNode, arg)
*      Q_JOB_NODE *pNode;	/@ pointer to a queue node          @/
*      int	   arg;		/@ arbitrary user-supplied argument @/
* .CE
* The user-supplied routine should return TRUE if qJobEach (2) is to
* continue calling it for each entry, or FALSE if it is done and qJobEach
* can exit.
*
* WARNING: calls user routine with interrupts locked out!
*
* RETURNS: NULL if traversed whole queue, or pointer to Q_JOB_NODE that
*          qJobEach stopped on.
*/

Q_JOB_NODE *qJobEach
    (
    Q_JOB_HEAD *pQHead,         /* queue head of queue to call routine for */
    FUNCPTR     routine,        /* the routine to call for each table entry */
    int         routineArg      /* arbitrary user-supplied argument */
    )
    {
    FAST Q_JOB_NODE *pNode;
    FAST int level;

    level = intLock ();			/* LOCK INTERRUPTS */

    pNode = pQHead->first;

    while ((pNode != NULL) && ((* routine) (pNode, routineArg)))
	pNode = pNode->next;		/* next node */

    intUnlock (level);			/* UNLOCK INTERRUPTS */

    return (pNode);			/* return node we ended with */
    }

/*******************************************************************************
*
* qJobNullRtn -
*
* RETURNS: OK
*/

LOCAL STATUS qJobNullRtn (void)
    {
    return (OK);
    }
