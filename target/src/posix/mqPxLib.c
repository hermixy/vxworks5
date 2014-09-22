/* mqPxLib.c - message queue library (POSIX) */

/* Copyright 1984-2002 Wind River Systems, Inc.  */
#include "copyright_wrs.h"

/*
modification history
--------------------
01s,17jul00,jgn  merge DOT-4 pthreads changes
01r,29sep00,pai  added Andrew Gaiarsa's fix for mq_send() during kernelState
                 (SPR #32033)
01q,16oct96,dgp  doc: modify mq_send to show maximum priority <= 31 
		      (SPR #7019)
01p,19aug96,dbt  added use of MQ_PRIO_MAX (SPR #7039).
01o,11feb95,jdi  corrected spelling error.
01n,25jan95,rhp  doc tweaks.
    19jan95,jdi  doc cleanup.
01m,15apr94,dvs  fix mq_send to unlock interrupts on error.
	   +rrr
01l,08apr94,dvs  doc cleanup of mq_close (SPR #3099).
01k,08apr94,dvs  fixed error in args when calling symFindByName (SPR #3090).
01j,03feb94,kdl  moved structure definitions to mqPxLibP.h.
01i,01feb94,dvs  documentation changes.
01h,30jan94,kdl  changed #if for conditional ffsMsb() prototype (!=I960).
01g,28jan94,kdl  added check for invalid msg and queue sizes in mq_open().
01f,28jan94,dvs  fixed i960 ffsMsb() problem
01e,24jan94,smb  added instrumentation macros
01d,12jan94,rrr  fixed bug sending message to self; change to follow wrs 
	   +kdl  coding conventions; change mqLibInit() to mqPxLibInit().
01c,21dec93,kdl	 fixed errno if opening non-existant queue.
01b,12nov93,rrr	 rewrite
01a,06apr93,smb	 created
*/

/*
DESCRIPTION
This library implements the message-queue interface defined in the
POSIX 1003.1b standard, as an alternative to the VxWorks-specific
message queue design in msgQLib.  These message queues are accessed
through names; each message queue supports multiple sending and receiving 
tasks.

The message queue interface imposes a fixed upper bound on the size of
messages that can be sent to a specific message queue.  The size is set on
an individual queue basis.  The value may not be changed dynamically.

This interface allows a task be notified asynchronously of the
availability of a message on the queue.  The purpose of this feature is to
let the task to perform other functions and yet still be notified that a
message has become available on the queue.

MESSAGE QUEUE DESCRIPTOR DELETION
The mq_close() call terminates a message queue descriptor and
deallocates any associated memory.  When deleting message queue
descriptors, take care to avoid interfering with other tasks that are
using the same descriptor.  Tasks should only close message
queue descriptors that the same task has opened successfully.

The routines in this library conform to POSIX 1003.1b.

INCLUDE FILES: mqueue.h

SEE ALSO: POSIX 1003.1b document, msgQLib,
.pG "Basic OS"
*/

#include "vxWorks.h"
#include "errno.h"
#include "stdarg.h"
#include "string.h"
#include "intLib.h"
#include "qLib.h"
#include "fcntl.h"
#include "objLib.h"
#include "taskLib.h"
#include "semLib.h"
#include "private/mqPxLibP.h"
#include "private/sigLibP.h"
#include "private/kernelLibP.h"
#include "private/windLibP.h"
#include "private/workQLibP.h"
#include "private/eventP.h"
#include "symLib.h"
#include "memLib.h"
#define __PTHREAD_SRC
#include "pthread.h"

#if CPU_FAMILY != I960
extern int ffsMsb (unsigned long);	/* not in any header except for i960 */
#endif


#undef FREAD
#undef FWRITE

#define FREAD	1
#define FWRITE	2



static OBJ_CLASS 	mqClass;
CLASS_ID 		mqClassId = &mqClass;
SYMTAB_ID 		mqNameTbl;
BOOL 			mqLibInstalled = FALSE;

#ifdef __GNUC__
#define INLINE	__inline__
#else
#define INLINE
#endif

/*******************************************************************************
*
* sll_ins - insert into a singly linked list
*
* These list have the following form:
*
* ---------
* | head  |-------------
* ---------             \
*                        \
*          ----------     \   ----------      ----------        ----------
*      ,-->| New - 1|-------->| Newest |----->| Oldest |--~~~-->|        |-
*     /    ----------         ----------      ----------        ---------- \
*     \____________________________________________________________________/
*
* To insert, put it after the Newest and move the head up one.
*/

static INLINE void sll_ins
    (
    struct sll_node *   pNode,		/* node to insert */
    struct sll_node **  ppHead		/* addr of head of list */
    )
    {
    if (*ppHead == NULL)
	*ppHead = pNode->sll_next = pNode;
    else
	{
	pNode->sll_next = (*ppHead)->sll_next;
	(*ppHead)->sll_next = pNode;
	*ppHead = pNode;
	}
    }

/*******************************************************************************
*
* sll_head - remove the oldest node off a singly linked list
*
* These list have the following form:
*
* ---------
* | head  |-------------
* ---------             \
*                        \
*          ----------     \   ----------      ----------        ----------
*      ,-->| New - 1|-------->| Newest |------| Oldest |--~~~-->|        |-
*     /    ----------         ----------      ----------        ---------- \
*     \____________________________________________________________________/
*
* To remove, pull off the one after the Newest, then make sure there are
* still some nodes left otherwise null out the head.
*
* RETURNS: A pointer to the oldest node.
*/

static INLINE struct sll_node *sll_head
    (
    struct sll_node ** ppHead		/* addr of head of list */
    )
    {
    struct sll_node *pRetval;

    if ((pRetval = (*ppHead)->sll_next) == *ppHead)
	*ppHead = NULL;
    else
	(*ppHead)->sll_next = pRetval->sll_next;

    return (pRetval);
    }

/*******************************************************************************
*
* mqPxLibInit - initialize the POSIX message queue library
*
* This routine initializes the POSIX message queue facility.  If <hashSize> is
* 0, the default value is taken from MQ_HASH_SIZE_DEFAULT.
*
* RETURNS: OK or ERROR.
*/

int mqPxLibInit
    (
    int hashSize		/* log2 of number of hash buckets */
    )
    {
    if (!mqLibInstalled)
	{
	if (hashSize == 0)
	    hashSize = MQ_HASH_SIZE_DEFAULT;

	if (((mqNameTbl = symTblCreate (hashSize, FALSE, memSysPartId)) != NULL)
	    && (classInit (mqClassId, sizeof (struct mq_des),
			  OFFSET (struct mq_des, f_objCore),
			  (FUNCPTR) NULL,(FUNCPTR) NULL,(FUNCPTR) NULL) == OK))
	    mqLibInstalled = TRUE;
	}

    return (mqLibInstalled) ? OK : ERROR;
    }

/*******************************************************************************
*
* mq_init - quick and dirty init of the guts of a message queue.
*
* The needs to be redone with objects!!! XXX
*
*/

static void mq_init
    (
    void *	pMem,		/* memory to hold queue */
    int		nMsgs,		/* number of messages in queue */
    int		msgSize		/* size of each message */
    )
    {
    struct sll_node *pNode;
    struct msg_que *pMq;
    size_t nBytes;

    nBytes = MEM_ROUND_UP (msgSize + sizeof (struct sll_node));

    pMq = (struct msg_que *) pMem;

    bzero ((void *) pMq, sizeof (struct msg_que));


    qInit (&pMq->msgq_cond_read, Q_PRI_LIST, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    qInit (&pMq->msgq_cond_data, Q_PRI_LIST, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    pMq->msgq_sigTask		= -1;
    pMq->msgq_attr.mq_maxmsg	= nMsgs;
    pMq->msgq_attr.mq_msgsize	= msgSize;

    pNode = (struct sll_node *) (void *) ((char *)pMem +
	    MEM_ROUND_UP (sizeof (struct msg_que)));

    while (nMsgs-- > 0)
	{
	sll_ins (pNode, &pMq->msgq_free_list);
	pNode = (struct sll_node *) (void *) ((char *)pNode + nBytes);
	}
    }

/*******************************************************************************
*
* mq_terminate - quick and dirty termination of the guts of a message queue.
*
* The needs to be redone with objects!!! XXX
*
*/

static void mq_terminate
    (
    struct msg_que *pMq
    )
    {

    kernelState = TRUE;			/* ENTER KERNEL */

    /* windview - level 2 instrumentation */
    EVT_TASK_1 (EVENT_OBJ_MSGDELETE, pMq);

    windPendQTerminate (&pMq->msgq_cond_read);

    /* windview - level 2 instrumentation */
    EVT_TASK_1 (EVENT_OBJ_MSGDELETE, pMq);

    windPendQTerminate (&pMq->msgq_cond_data);

    windExit ();			/* EXIT KERNEL */

    /*
     * notification processing
     */
    if (pMq->msgq_sigTask != -1)
	{
	sigPendDestroy (&pMq->msgq_sigPend);
	pMq->msgq_sigTask = -1;
	}

    free (pMq);
    }

/*******************************************************************************
*
* mq_create - create a message queue
*
* The function mq_create() is used to create a message queue.  The maximum
* size of any message that can be sent is <msgSize>.  The message queue
* can hold <nMsgs> messages before a mq_send() will block.
*
* RETURNS: Upon successful completion, mq_create() will return a
* message queue descriptor.  Otherwise, the function will return (<mqd_t>)-1
* and set <errno> to indicate the error.
*
* ERRORS: If any of the following conditions occur, the mq_create() function
* will return (<mqd_t>)-1 and set <errno> to the corresponding value:
* .iP ENOMEM
*
* NOMANUAL
*/

mqd_t mq_create
    (
    size_t nMsgs,		/* number of messages in queue */
    size_t msgSize		/* size of each message in queue */
    )
    {
    struct mq_des	*pMqDesc;
    void		*pQMem;

    if (INT_RESTRICT () != OK)
	return ((mqd_t) -1);	/* restrict ISR from calling */

    if ((!mqLibInstalled) && (mqPxLibInit (0) != OK))
	return ((mqd_t) -1);	/* package init problem */


    pQMem = malloc (MEM_ROUND_UP (sizeof (struct msg_que)) + 
	        nMsgs * MEM_ROUND_UP (msgSize + sizeof (struct sll_node)));

    if (pQMem == NULL)
	{
	errno = ENOSPC;
	return ((mqd_t) -1);
	}

    if ((pMqDesc = (mqd_t) objAlloc (mqClassId)) == NULL)
	{
	free (pQMem);
       	errno = ENOSPC;
       	return ((mqd_t) -1);
	}

    mq_init (pQMem, nMsgs, msgSize);

    pMqDesc->f_flag = FREAD | FWRITE;
    pMqDesc->f_data = pQMem;
    pMqDesc->f_data->msgq_links++;

    objCoreInit (&pMqDesc->f_objCore, mqClassId);	/* valid file obj */

    return (pMqDesc);
    }

/*******************************************************************************
*
* mq_open - open a message queue (POSIX)
*
* This routine establishes a connection between a named message queue and the
* calling task.  After a call to mq_open(), the task can reference the
* message queue using the address returned by the call.  The message queue
* remains usable until the queue is closed by a successful call to mq_close().
*
* The <oflags> argument controls whether the message queue is created or merely
* accessed by the mq_open() call.  The following flag bits can be set
* in <oflags>:
* .iP O_RDONLY
* Open the message queue for receiving messages.  The task can use the 
* returned message queue descriptor with mq_receive(), but not mq_send().
* .iP O_WRONLY
* Open the message queue for sending messages.  The task can use the
* returned message queue descriptor with mq_send(), but not mq_receive().
* .iP O_RDWR
* Open the queue for both receiving and sending messages.  The task can use
* any of the functions allowed for O_RDONLY and O_WRONLY.
* .LP
*
* Any combination of the remaining flags can be specified in <oflags>:
* .iP O_CREAT
* This flag is used to create a message queue if it does not already exist.
* If O_CREAT is set and the message queue already exists, then O_CREAT has
* no effect except as noted below under O_EXCL.  Otherwise, mq_open()
* creates a message queue.  The O_CREAT flag requires a third and fourth
* argument: <mode>, which is of type `mode_t', and <pAttr>, which is of type
* pointer to an `mq_attr' structure.  The value of <mode> has no effect in
* this implementation.  If <pAttr> is NULL, the message queue is created
* with implementation-defined default message queue attributes.  If <pAttr>
* is non-NULL, the message queue attributes `mq_maxmsg' and `mq_msgsize' are
* set to the values of the corresponding members in the `mq_attr' structure
* referred to by <pAttr>; if either attribute is less than or equal to zero,
* an error is returned and errno is set to EINVAL.
* .iP O_EXCL
* This flag is used to test whether a message queue already exists.
* If O_EXCL and O_CREAT are set, mq_open() fails if the message queue name
* exists.
* .iP O_NONBLOCK
* The setting of this flag is associated with the open message queue descriptor
* and determines whether a mq_send() or mq_receive() will wait for resources
* or messages that are not currently available, or fail with errno set 
* to EAGAIN.  
* .LP
*
* The mq_open() call does not add or remove messages from the queue.
*
* NOTE:
* Some POSIX functionality is not yet supported:
*
*     - A message queue cannot be closed with calls to _exit() or exec().
*     - A message queue cannot be implemented as a file.
*     - Message queue names will not appear in the file system.
*
* RETURNS: A message queue descriptor, otherwise -1 (ERROR).
*
* ERRNO: EEXIST, EINVAL, ENOENT, ENOSPC
*
* SEE ALSO: mq_send(), mq_receive(), mq_close(), mq_setattr(), mq_getattr(),
* mq_unlink()
*
*/ 

mqd_t mq_open
    (
    const char	*mqName,	/* name of queue to open */
    int		oflags,		/* open flags */
    ...				/* extra optional parameters */
    )
    {
    va_list		vaList;		/* arg list (if O_CREAT) */
    struct mq_des *	pMqDesc;	/* memory for queue descriptor */
    struct mq_attr *	pAttr;		/* pointer to attr passed in */
    void *		pQMem;		/* memory queue will be placed in */
    mode_t		mode;		/* not used by vxWorks */
    int			nMsgs;		/* number of messages in queue */
    int			msgSize;	/* size of each message in queue */
    int			nBytes;		/* amount of mem needed for queue */
    SYM_TYPE		dummy;		/* dummy var for symFindByName */

    if (INT_RESTRICT () != OK)	/* restrict ISR from calling */
	return ((mqd_t) -1);

    if ((!mqLibInstalled) && (mqPxLibInit (0) != OK))
	return ((mqd_t) -1);	/* package init problem */

    if ((oflags & 3) == 3)
	{
	errno = EINVAL;
	return ((mqd_t) -1);
	}

    semTake (&mqNameTbl->symMutex, WAIT_FOREVER);

    if (symFindByName (mqNameTbl, (char *) mqName, (char **)&pQMem, 
                       &dummy) == OK)
	{
	/*
	 * Found a mq, see if we should use it
	 */
	if (O_EXCL & oflags)
	    {
	    semGive (&mqNameTbl->symMutex);
	    errno = EEXIST;
	    return ((mqd_t) -1);
	    }

	if ((pMqDesc = (mqd_t) objAlloc (mqClassId)) == NULL)
	    {
	    semGive (&mqNameTbl->symMutex);
	    errno = ENOSPC;
	    return ((mqd_t) -1);
	    }
	}
    else
	{
	/*
	 * There was no mq, create one if asked
	 */
	if (!(O_CREAT & oflags))
	    {
	    semGive (&mqNameTbl->symMutex);
	    errno = ENOENT;
	    return ((mqd_t) -1);		/* queue does not exist */
	    }

	va_start (vaList, oflags);
	mode = va_arg (vaList, mode_t);
	pAttr = va_arg (vaList, struct mq_attr *);
	va_end (vaList);

	if (pAttr != NULL)
	    {
	    nMsgs = pAttr->mq_maxmsg;
	    msgSize = pAttr->mq_msgsize;

	    if ((nMsgs <= 0) || (msgSize <= 0))
		{
	    	semGive (&mqNameTbl->symMutex);
	    	errno = EINVAL;
	    	return ((mqd_t) -1);		/* invalid size specified */
	    	}

	    oflags |= (pAttr->mq_flags & O_NONBLOCK);
	    }
	else
	    {
	    nMsgs =   MQ_NUM_MSGS_DEFAULT;
	    msgSize = MQ_MSG_SIZE_DEFAULT;
	    }

	nBytes = MEM_ROUND_UP (sizeof(struct msg_que)) + 
		 nMsgs * MEM_ROUND_UP (msgSize + sizeof (struct sll_node));
	pQMem = malloc (nBytes + strlen (mqName) + 1);

	if (pQMem == 0)
	    {
	    semGive (&mqNameTbl->symMutex);
	    errno = ENOSPC;
	    return ((mqd_t) -1);
	    }

	mq_init (pQMem, nMsgs, msgSize);

	((struct msg_que *) pQMem)->msgq_sym.name = (char *) pQMem + nBytes;
	((struct msg_que *) pQMem)->msgq_sym.value = (char *) pQMem;
	strcpy ((char *)pQMem + nBytes, mqName);


	if ((pMqDesc = (mqd_t) objAlloc (mqClassId)) == NULL)
	    {
	    free (pQMem);
	    semGive (&mqNameTbl->symMutex);
	    errno = ENOSPC;
	    return ((mqd_t) -1);
	    }

	symTblAdd (mqNameTbl, &((struct msg_que *) pQMem)->msgq_sym);
	}

    /*
     * (oflags & 3) + 1 is magic for transposing O_RDONLY, O_RDWR or
     * O_WRONLY into the bit field flags FREAD and FWRITE
     */
    pMqDesc->f_flag = (oflags & O_NONBLOCK) | ((oflags & 3) + 1);
    pMqDesc->f_data = pQMem;
    pMqDesc->f_data->msgq_links++;
    objCoreInit (&pMqDesc->f_objCore, mqClassId);	/* valid file obj */

    semGive (&mqNameTbl->symMutex);

    return (pMqDesc);
    }


/*******************************************************************************
*
* mq_receive - receive a message from a message queue (POSIX)
*
* This routine receives the oldest of the highest priority message from
* the message queue specified by <mqdes>.  If the size of the buffer in
* bytes, specified by the <msgLen> argument, is less than the `mq_msgsize'
* attribute of the message queue, mq_receive() will fail and return an
* error.  Otherwise, the selected message is removed from the queue and
* copied to <pMsg>.
*
* If <pMsgPrio> is not NULL, the priority of the selected message
* will be stored in <pMsgPrio>.
*
* If the message queue is empty and O_NONBLOCK is not set in the message
* queue's description, mq_receive() will block until a message is added to
* the message queue, or until it is interrupted by a signal.  If more than
* one task is waiting to receive a message when a message arrives at an
* empty queue, the task of highest priority that has been waiting the
* longest will be selected to receive the message.  If the specified message
* queue is empty and O_NONBLOCK is set in the message queue's description,
* no message is removed from the queue, and mq_receive() returns an error.
*
* RETURNS: The length of the selected message in bytes, otherwise -1 (ERROR). 
*
* ERRNO: EAGAIN, EBADF, EMSGSIZE, EINTR
*
* SEE ALSO: mq_send()
*/

ssize_t mq_receive
    (
    mqd_t	mqdes,		/* message queue descriptor */
    void	*pMsg,		/* buffer to receive message */
    size_t	msgLen,		/* size of buffer, in bytes */
    int		*pMsgPrio	/* if not NULL, priority of message */
    )
    {
    struct sll_node *pNode;
    struct msg_que *pMq;
    int status;
    int level;
    int prio;
    int error = 0;
    int savtype;

    if (INT_RESTRICT () != OK)		/* restrict ISR from calling */
	return (-1);

    TASK_LOCK ();			/* TASK LOCK */

    if ((OBJ_VERIFY (mqdes, mqClassId) != OK) ||
	((mqdes->f_flag & FREAD) == 0))
	{
	error = EBADF;
	goto bad;
	}

    pMq = mqdes->f_data;

    /* Link into pthreads support code */

    if (_func_pthread_setcanceltype != NULL)
        {
        _func_pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &savtype);
        }

    if (msgLen < pMq->msgq_attr.mq_msgsize)
	{
	error = EMSGSIZE;
	goto bad;
	}

    level = intLock ();			/* LOCK INTERRUPTS */

    if (pMq->msgq_bmap == 0)
	{
	if (mqdes->f_flag & O_NONBLOCK)
	    {
	    intUnlock (level);		/* UNLOCK INTERRUPTS */
	    error = EAGAIN;
	    goto bad;
	    }

	/*
	 * This needs to be in a loop, the reason is that someone
	 * can wake us up but we may not run.  During that time someone
	 * else can come in and steal our message.
	 */
	while (pMq->msgq_bmap == 0)
	    {
	    kernelState = TRUE;		/* ENTER KERNEL */
	    intUnlock (level);		/* UNLOCK INTERRUPTS */

	    /* windview - level 2 instrumentation */
            EVT_TASK_1 (EVENT_OBJ_MSGRECEIVE, pMq);

	    if (windPendQPut (&pMq->msgq_cond_data, WAIT_FOREVER) != OK)
		{
		windExit ();			/* EXIT KERNEL */
		error = EBADF;			/* what should ernno be??? */
		goto bad;
		}

	    status = windExit ();		/* EXIT KERNEL */

	    if (status != 0)
		{
		error = (status == RESTART) ? EINTR : EAGAIN;
		goto bad;
		}

	    level = intLock ();		/* LOCK INTERRUPTS */
	    }
	}

    prio = ffsMsb (pMq->msgq_bmap) - 1;

    pNode = sll_head (&pMq->msgq_data_list[prio]);
    --pMq->msgq_attr.mq_curmsgs;

    if (pMq->msgq_data_list[prio] == NULL)
	pMq->msgq_bmap &= ~(1 << prio);

    intUnlock (level);			/* UNLOCK INTERRUPTS */

    msgLen = pNode->sll_size;

    bcopy ((void *) (pNode + 1), pMsg, msgLen);

    if (pMsgPrio != 0)
	*pMsgPrio = prio;

    level = intLock ();			/* LOCK INTERRUPTS */

    sll_ins (pNode, &pMq->msgq_free_list);

    if (Q_FIRST (&pMq->msgq_cond_read) != 0)
	{
	kernelState = TRUE;		/* ENTER KERNEL */
	intUnlock (level);		/* UNLOCK INTERRUPTS */

	/* windview - level 2 instrumentation */
        EVT_TASK_1 (EVENT_OBJ_MSGRECEIVE, pMq);

	windPendQGet (&pMq->msgq_cond_read);
	windExit ();			/* EXIT KERNEL */
	}
    else
	intUnlock (level);		/* UNLOCK INTERRUPTS */

bad:
    TASK_UNLOCK();			/* TASK UNLOCK */

    /* Link into pthreads support code */

    if (_func_pthread_setcanceltype != NULL)
        {
        _func_pthread_setcanceltype(savtype, NULL);
        }

    if (error != 0)
	{
	errno = error;
	return (-1);
	}

    return (msgLen);
    }

/*******************************************************************************
*
* mq_work - handle mq work
*
* The routine is always done as work. It is needed because windPendQGet
* does not check it the queue is empty.
*/

LOCAL void mq_work
    (
    struct msg_que *pMq,
    int flag
    )
    {
    if (Q_FIRST (&pMq->msgq_cond_data) != NULL)
	{
	/* windview - level 2 instrumentation */
        EVT_TASK_1 (EVENT_OBJ_MSGSEND, pMq);

	windPendQGet (&pMq->msgq_cond_data);
	}
    else if ((flag == 0) && (pMq->msgq_sigTask != -1))
	{
        /* SPR #32033
         * An interrupt context is faked while executing sigPendKill().  
         * Since the work queue is drained with kernelState set to TRUE,
         * sigPendKill() will perform an excJobAdd() of itself.  Without
         * faking an interrupt context, the ensuing msgQSend() will perform
         * taskLock/taskUnlock to protect it's critical section.  The 
         * taskUnlock() would then re-enter the kernel via windExit().
         * The kernel cannot be re-entered while in the process of draining
         * the work queue.
         */
   
	++intCnt;     /* no need to lock interrupts around increment */
	sigPendKill (pMq->msgq_sigTask, &pMq->msgq_sigPend);
	--intCnt;     /* no need to lock interrupts around decrement */
	pMq->msgq_sigTask = -1;
	}
    }

/*******************************************************************************
*
* mq_send - send a message to a message queue (POSIX)
*
* This routine adds the message <pMsg> to the message queue
* <mqdes>.  The <msgLen> parameter specifies the length of the message in
* bytes pointed to by <pMsg>.  The value of <pMsg> must be less than or
* equal to the `mq_msgsize' attribute of the message queue, or mq_send()
* will fail.
*
* If the message queue is not full, mq_send() will behave as if the message
* is inserted into the message queue at the position indicated by the 
* <msgPrio> argument.  A message with a higher numeric value for <msgPrio>
* is inserted before messages with a lower value.  The value
* of <msgPrio> must be less than or equal to 31.
*
* If the specified message queue is full and O_NONBLOCK is not set in the
* message queue's, mq_send() will block until space becomes available to
* queue the message, or until it is interrupted by a signal.  The priority
* scheduling option is supported in the event that there is more than one
* task waiting on space becoming available.  If the message queue is full
* and O_NONBLOCK is set in the message queue's description, the message is
* not queued, and mq_send() returns an error.
*
* USE BY INTERRUPT SERVICE ROUTINES
* This routine can be called by interrupt service routines as well as
* by tasks.  This is one of the primary means of communication
* between an interrupt service routine and a task.  If mq_send()
* is called from an interrupt service routine, it will behave as if
* the O_NONBLOCK flag were set.
*
* RETURNS: 0 (OK), otherwise -1 (ERROR).
*
* ERRNO: EAGAIN, EBADF, EINTR, EINVAL, EMSGSIZE
*
* SEE ALSO: mq_receive()
*/

int mq_send
    (
    mqd_t	mqdes,		/* message queue descriptor */
    const void	*pMsg,		/* message to send */
    size_t	msgLen,		/* size of message, in bytes */
    int		msgPrio		/* priority of message */
    )
    {
    struct sll_node *pNode;
    struct msg_que *pMq;
    int status;
    int level;
    int flag;
    int sigTaskSave;		/* saved task id */
    int error = 0;
    int savtype;		/* saved cancellation type */

    if (msgPrio > MQ_PRIORITY_MAX)
	{
	errno = EINVAL;
	return (-1);
	}

    /* Link into pthreads support code */
    
    if (_func_pthread_setcanceltype != NULL)
        {
        _func_pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &savtype);
        }

     if (!INT_CONTEXT ())
	TASK_LOCK ();			/* TASK LOCK */

    if ((OBJ_VERIFY (mqdes, mqClassId) != OK) ||
	((mqdes->f_flag & FWRITE) == 0))
	{
	error = EBADF;
	goto bad;
	}

    pMq = mqdes->f_data;

    if (msgLen > pMq->msgq_attr.mq_msgsize)
	{
	error = EMSGSIZE;
	goto bad;
	}

    level = intLock ();			/* LOCK INTERRUPTS */

    if (pMq->msgq_free_list == NULL)
	{
	if (mqdes->f_flag & O_NONBLOCK)
	    {
	    intUnlock (level);		/* UNLOCK INTERRUPTS */
	    error = EAGAIN;
	    goto bad;
	    }

	if (INT_CONTEXT ())
	    {
	    intUnlock (level);		/* UNLOCK INTERRUPTS */
	    error = EAGAIN;		/* what should errno be??? */
	    goto bad;
	    }

	while (pMq->msgq_free_list == NULL)
	    {
	    kernelState = TRUE;		/* ENTER KERNEL */
	    intUnlock (level);		/* UNLOCK INTERRUPTS */

	    /* windview - level 2 instrumentation */
            EVT_TASK_1 (EVENT_OBJ_MSGSEND, pMq);

	    if (windPendQPut (&pMq->msgq_cond_read, WAIT_FOREVER) != OK)
		{
		windExit ();		/* EXIT KERNEL */
		error = EBADF;		/* what should errno be??? */
		goto bad;
		}

	    if ((status = windExit ()) != 0)	/* EXIT KERNEL */
		{
		error = (status == RESTART) ? EINTR : EAGAIN;
		goto bad;
		}

	    level = intLock ();		/* LOCK INTERRUPTS */
	    }
	}

    pNode = sll_head (&pMq->msgq_free_list);

    intUnlock (level);			/* UNLOCK INTERRUPTS */

    bcopy (pMsg, (void *) (pNode + 1), msgLen);

    pNode->sll_size = msgLen;

    level = intLock ();			/* LOCK INTERRUPTS */

    sll_ins (pNode, &pMq->msgq_data_list[msgPrio]);
    flag = pMq->msgq_attr.mq_curmsgs++;
    pMq->msgq_bmap |= 1 << (msgPrio);

    if (kernelState)
	{
	intUnlock (level);		/* UNLOCK INTERRUPTS */
	workQAdd2 ((FUNCPTR)mq_work, (int) pMq, flag);
	}
    else
	{
	if (Q_FIRST (&pMq->msgq_cond_data) != 0)
	    {
	    kernelState = TRUE;		/* ENTER KERNEL */
	    intUnlock (level);		/* UNLOCK INTERRUPTS */

	    /* windview - level 2 instrumentation */
            EVT_TASK_1 (EVENT_OBJ_MSGSEND, pMq);

	    windPendQGet (&pMq->msgq_cond_data);
	    windExit ();		/* EXIT KERNEL */
	    }
	else
	    {
	    intUnlock (level);		/* UNLOCK INTERRUPTS */
	    if ((flag == 0) && (pMq->msgq_sigTask != -1))
		{
		sigTaskSave = pMq->msgq_sigTask;
		pMq->msgq_sigTask = -1;
		sigPendKill (sigTaskSave, &pMq->msgq_sigPend);
		}
	    }
	}


bad:
    if (!INT_CONTEXT ())
	TASK_UNLOCK ();			/* TASK UNLOCK */

    /* Link into pthreads support code */

    if (_func_pthread_setcanceltype != NULL)
        {
        _func_pthread_setcanceltype(savtype, NULL);
        }

    if (error)
	{
	errno = error;
	return (-1);
	}

    return (0);
    }

/*******************************************************************************
*
* mq_close - close a message queue (POSIX)
*
* This routine is used to indicate that the calling task is finished
* with the specified message queue <mqdes>.  
* The mq_close() call deallocates any system resources allocated 
* by the system for use by this task for its message queue.
* The behavior of a task that is blocked on either a mq_send() or
* mq_receive() is undefined when mq_close() is called.
* The <mqdes> parameter will no longer be a valid message queue ID.
*
* RETURNS: 0 (OK) if the message queue is closed successfully,
* otherwise -1 (ERROR).
*
* ERRNO: EBADF
*
* SEE ALSO: mq_open()
*/

int mq_close 
    (
    mqd_t mqdes				/* message queue descriptor */
    )
    {

    if (INT_RESTRICT () != OK)	/* restrict ISR from calling */
	return (-1);

    TASK_SAFE ();			/* TASK SAFE */
    TASK_LOCK ();			/* LOCK PREEMPTION */

    /*
     * validate message queue
     */
    if (OBJ_VERIFY (mqdes, mqClassId) != OK)
  	{
        TASK_UNLOCK ();			/* TASK UNLOCK */
        TASK_UNSAFE ();			/* TASK UNSAFE */
	errno = EBADF;
        return (-1);			/* invalid object */
	}

    /*
     * invalidate message queue descriptor
     */
    objCoreTerminate (&mqdes->f_objCore);

    TASK_UNLOCK ();			/* TASK UNLOCK */

    semTake (&mqNameTbl->symMutex, WAIT_FOREVER);

    /*
     * No effect unless the name has been unlinked from name table
     */
    if ((--mqdes->f_data->msgq_links == 0) &&
	(mqdes->f_data->msgq_sym.name == NULL))
	mq_terminate(mqdes->f_data);

    semGive (&mqNameTbl->symMutex);

    /*
     * free message queue descriptor
     */
    objFree (mqClassId, (char *) mqdes);

    TASK_UNSAFE ();			/* TASK UNSAFE */

    return (0);
    }

/*******************************************************************************
*
* mq_unlink - remove a message queue (POSIX)
*
* This routine removes the message queue named by the pathname <mqName>.
* After a successful call to mq_unlink(), a call to mq_open() on the same
* message queue will fail if the flag O_CREAT is not set.  If one or more
* tasks have the message queue open when mq_unlink() is called, removal of
* the message queue is postponed until all references to the message queue
* have been closed.
*
* RETURNS: 0 (OK) if the message queue is unlinked successfully,
* otherwise -1 (ERROR).
*
* ERRNO: ENOENT
*
* SEE ALSO: mq_close(), mq_open()
*
*/

int mq_unlink 
    (
    const char * mqName			/* name of message queue */
    )
    {
    struct msg_que	*pMq;
    SYM_TYPE		dummy;		/* dummy var for symFindByName */

    /*
     * The following semTake is used to insure that mq_open cannot
     * attach a task to the message queue between finding the message queue
     * name in the table and unlinking it from the table.
     */

    semTake (&mqNameTbl->symMutex, WAIT_FOREVER);

    if (symFindByName (mqNameTbl, (char *) mqName, (char **) &pMq, 
		       &dummy) == ERROR)
        {
	semGive (&mqNameTbl->symMutex);
        errno = ENOENT;
        return (-1);
        }

    /*
     * Remove name from table
     */
    symTblRemove (mqNameTbl, &pMq->msgq_sym);
    pMq->msgq_sym.name = 0;

    if (pMq->msgq_links == 0)
	mq_terminate (pMq);

    semGive (&mqNameTbl->symMutex);

    return (0);
    }

/*******************************************************************************
*
* mq_notify - notify a task that a message is available on a queue (POSIX)
*
* If <pNotification> is not NULL, this routine attaches the specified
* <pNotification> request by the calling task to the specified message queue
* <mqdes> associated with the calling task.  The real-time signal specified
* by <pNotification> will be sent to the task when the message queue changes
* from empty to non-empty.  If a task has already attached a notification
* request to the message queue, all subsequent attempts to attach a
* notification to the message queue will fail.  A task is able to attach a
* single notification to each <mqdes> it has unless another task has already
* attached one.
*
* If <pNotification> is NULL and the task has previously attached a
* notification request to the message queue, the attached notification
* request is detached and the queue is available for another task to attach
* a notification request.
*
* If a notification request is attached to a message queue and any task
* is blocked in mq_receive() waiting to receive a message when a message
* arrives at the queue, then the appropriate mq_receive() will be completed
* and the notification request remains pending.
*
* RETURNS: 0 (OK) if successful, otherwise -1 (ERROR).
*
* ERRNO: EBADF, EBUSY, EINVAL
*
* SEE ALSO: mq_open(), mq_send()
*
*/

int mq_notify 
    (
    mqd_t 		    mqdes, 		/* message queue descriptor */
    const struct sigevent * pNotification	/* real-time signal */
    )
    {
    struct siginfo * sigInfoP;
    int error = 0;

    if (INT_RESTRICT () != OK)	/* restrict ISR from calling */
	return (-1);

    TASK_LOCK ();		/* TASK LOCK */

    if (OBJ_VERIFY (mqdes, mqClassId) != OK)
  	{
	error = EBADF;
        goto done;		/* invalid object */
	}

    /*
     * free realtime signal
     */
    if (pNotification == NULL) 
	{
	if (taskIdSelf () == mqdes->f_data->msgq_sigTask)
	    {
	    mqdes->f_data->msgq_sigTask = -1;
	    sigPendDestroy (&mqdes->f_data->msgq_sigPend);
	    }
        else
	    error = EINVAL;

	goto done;
	}

    /*
     * already in use
     */
    if (mqdes->f_data->msgq_sigTask != -1)
	{
	error = EBUSY;
	goto done;
	}

    mqdes->f_data->msgq_sigTask = taskIdSelf ();

    sigInfoP = &mqdes->f_data->msgq_sigPend.sigp_info;
    sigInfoP->si_signo = pNotification->sigev_signo;	/* signal number */
    sigInfoP->si_code  = SI_MESGQ;			/* signal code */
    sigInfoP->si_value = pNotification->sigev_value;	/* signal value */

    /* initialize signal */
    sigPendInit (&mqdes->f_data->msgq_sigPend);

done:
    TASK_UNLOCK ();		/* TASK UNLOCK */

    if (error != 0)
	{
	errno = error;
	return (-1);
	}

    return (0);
    }

/*******************************************************************************
*
* mq_setattr - set message queue attributes (POSIX)
*
* This routine sets attributes associated with the specified message
* queue <mqdes>.
*
* The message queue attributes corresponding to the following members
* defined in the `mq_attr' structure are set to the specified values upon
* successful completion of the call:
* .iP `mq_flags'
* The value the O_NONBLOCK flag.
* .LP
*
* If <pOldMqStat> is non-NULL, mq_setattr() will store, in the
* location referenced by <pOldMqStat>, the previous message queue attributes
* and the current queue status.  These values are the same as would be returned
* by a call to mq_getattr() at that point.
*
* RETURNS: 0 (OK) if attributes are set successfully, otherwise -1 (ERROR).
*
* ERRNO: EBADF
*
* SEE ALSO: mq_open(), mq_send(), mq_getattr()
*/

int mq_setattr 
    (
    mqd_t 		   mqdes, 	/* message queue descriptor */
    const struct mq_attr * pMqStat,	/* new attributes */
    struct mq_attr *       pOldMqStat	/* old attributes */
    )
    {

    if (INT_RESTRICT () != OK)	/* restrict ISR from calling */
	return (-1);

    TASK_LOCK ();		/* TASK LOCK */

    if (OBJ_VERIFY (mqdes, mqClassId) != OK)
  	{
    	TASK_UNLOCK ();		/* TASK UNLOCK */
	errno = EBADF;
        return (-1);		/* invalid object */
	}

    if (pOldMqStat)
	{
	*pOldMqStat = mqdes->f_data->msgq_attr;
	pOldMqStat->mq_flags = mqdes->f_flag;
	}

    if (pMqStat)
	{
	mqdes->f_flag &= ~O_NONBLOCK;
	mqdes->f_flag |= (pMqStat->mq_flags & O_NONBLOCK);
	}

    TASK_UNLOCK ();		/* TASK UNLOCK */

    return (0);
    }

/*******************************************************************************
*
* mq_getattr - get message queue attributes (POSIX)
*
* This routine gets status information and attributes associated with a
* specified message queue <mqdes>.  Upon return, the following members of
* the `mq_attr' structure referenced by <pMqStat> will contain the values
* set when the message queue was created but with modifications made by
* subsequent calls to mq_setattr():
* .iP `mq_flags'
* May be modified by mq_setattr().
* .LP
*
* The following were set at message queue creation:
* .iP `mq_maxmsg'
* Maximum number of messages.
* .iP `mq_msgsize'
* Maximum message size.
* .LP
*
* .iP `mq_curmsgs'
* The number of messages currently in the queue.
* .LP
*
* RETURNS: 0 (OK) if message attributes can be determined, otherwise -1 (ERROR).
*
* ERRNO: EBADF
*
* SEE ALSO: mq_open(), mq_send(), mq_setattr()
*
*/

int mq_getattr 
    (
    mqd_t 	     mqdes, 	/* message queue descriptor */
    struct mq_attr * pMqStat	/* buffer in which to return attributes */
    )
    {
    return (mq_setattr (mqdes, NULL, pMqStat));
    }
