/* rBuffLib.c - dynamic ring buffer (rBuff) library */

/* Copyright 1984-1997 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01z,20apr00,max  Change tid == NULL to tid == 0 (like in es.coretools)
01y,28aug98,dgp  FCS man page edit
01x,27aug98,dgp  add lib description, edit wvRBuffMgrPrioritySet() for manpgs
01w,18aug98,cjtc event buffer full handled more gracefully (SPR 22133)
01v,22jul98,cjtc optimise ring buffer scheme for windview (SPR 21752)
		 Avoids multiple rBuffMgr tasks (SPR 21805)
01u,30jun98,cjtc make priority of rBuffMgr task variable (SPR 21543)
01t,06may98,pr   fix wrong spell of portWorkQAdd1 for x86.
01t,01may98,nps  reworked msg passing for rBuff maintenance.
01s,26mar98,nps  fix 2 buffer / continuous upload problem.
01r,25mar98,pr   used portable portWorkQAdd1 only for the I80X86.
                 moved portWorkQAdd1 into trgLib.c
01q,20mar98,pr   replaced workQAdd1 with portable version (for x86).
01p,19mar98,nps  only give upload threshold semaphore when necessary.
01o,13mar98,nps  added rBuffVerify.
01n,03mar98,nps  Source control structures from specfied source partition.
01m,21feb98,nps  Don't use msgQ fn to pass msgs - it causes problems when
                 used with WV instrumentation.
01l,04feb98,nps  remove diagnostic messages.
01k,03feb98,nps  Changed msgQSend option from WAIT_FOREVER to NO_WAIT.
01j,12jan98,nps  Don't initialise rBuff multiple times.
01i,18dec97,cth  updated include files, added flushRtn init to rBuffCreate
01h,25nov97,nps  Tom's fix incorporated.
                 Maintain statistic of peak buffer utilisation.
                 Fix count of empty buffers.
01g,16nov97,cth  changed every public routine to accept generic buffer id,
		 described in buffer.h, rather than rBuffId.
		 added initialization for generic buffer id in rBuffCreate
01f,15Sep97,nps  rBuffShow moved out to its own file.
                 SEMAPHOREs now init'd rather than created.
                 added support for 'infinite' extension.
                 added support for RBUFF_WRAPAROUND.
                 buffers for ring are now added and freed by dedicated task.
01e,18Aug97,nps  fixed counting of emptyBuffs.
01d,18Aug97,nps  use semBGiveDefer to signal event upload.
01c,11Aug97,nps  rBuffReset now resets *all* buffers.
01b,28jul97,nps	 further implementation/testing.
                 rBuffReset returns STATUS type.
                 added rBuffSetFd.
01a,14jul97,nps	 written.
*/

/*
DESCRIPTION
This library contains a routine for changing the default priority of the rBuff
manager task.

INTERNAL
This library also contains the non-public routines creating and managing the
dynamic ring buffer.  It provides both a WindView-specific version and a
generic version.

SEE ALSO: memLib, rngLib,
.pG "Basic OS"
*/
#undef RBUFF_DEBUG

#ifndef  GENERIC_RBUFF

/*
 * Below this point the WindView specific version of the ring buffer
 * library. In order to compile with the old (generic version of the
 * ring buffer, you should use EXTRA_DEFINE='-DGENERIC_RBUFF' in the
 * make command line, or #define GENERIC_RBUFF at the head of this
 * file.
 */


#include "vxWorks.h"
#include "semLib.h"
#include "classLib.h"
#include "errno.h"
#include "intLib.h"
#include "logLib.h"
#include "taskLib.h"
#include "fioLib.h"
#include "rBuffLib.h"


#include "private/wvBufferP.h"
#include "private/classLibP.h"
#include "private/objLibP.h"
#include "private/workQLibP.h"

#include "stdio.h"
#include "string.h"
#include "unistd.h"

extern int wvRBuffMgr();

#if (CPU_FAMILY == I80X86)
extern void portWorkQAdd1();
#endif

/* locals */

#if defined(__STDC__) || defined(__cplusplus)

LOCAL STATUS rBuffAdd
    (
    RBUFF_ID  rBuff,
    RBUFF_PTR buffToAdd
    );

LOCAL STATUS  rBuffFree
    (
    RBUFF_ID  rBuff,
    RBUFF_PTR buffToFree
    );

LOCAL STATUS rBuffHandleEmpty
    (
    RBUFF_ID rBuff
    );

#else	/* __STDC__ */

LOCAL STATUS rBuffAdd();
LOCAL STATUS rBuffFree();
LOCAL STATUS rBuffHandleEmpty();

#endif	/* __STDC__ */

extern  BOOL kernelState;

#if (CPU_FAMILY == I80X86)
#   define WQADD1 portWorkQAdd1
#else /* CPU_FAMILY == I80X86 */
#   define WQADD1 workQAdd1
#endif

#define RBUFF_SEND_MSG(RMSG_TYPE,RMSG_PRI,RB_ID,RMSG_PARAM)		    \
    {									    \
    if ((RB_ID)->nestLevel++ == 0)					    \
    	{								    \
    	BOOL doIt = TRUE ;						    \
    	if (RMSG_TYPE == RBUFF_MSG_ADD) 				    \
            {								    \
	    if ((RB_ID)->msgOutstanding)				    \
		{							    \
	    	doIt = FALSE;						    \
	    	}							    \
	    (RB_ID)->msgOutstanding = TRUE;				    \
	    }								    \
    	if (doIt)							    \
    	    {								    \
	    WV_RBUFF_MGR_ID pMgr;					    \
	    pMgr = (WV_RBUFF_MGR_ID)(RB_ID)->rBuffMgrId;		    \
	    pMgr->msg[pMgr->msgWriteIndex].ringId = (RB_ID);		    \
	    pMgr->msg[pMgr->msgWriteIndex].msgType = RMSG_TYPE;  	    \
    	    pMgr->msg[pMgr->msgWriteIndex].arg = (UINT32) RMSG_PARAM;  	    \
                                                              		    \
    	    if(++pMgr->msgWriteIndex > (WV_RBUFF_MGR_MSGQ_MAX - 1))    	    \
    	    	{                                                           \
            	pMgr->msgWriteIndex = 0;                         	    \
    	    	}                                                           \
									    \
    	    if(kernelState)						    \
	    	{							    \
            	WQADD1 ((FUNCPTR)semCGiveDefer, (int)&pMgr->msgSem);        \
    	    	}                                                           \
    	    else							    \
    	    	{							    \
            	semCGiveDefer (&pMgr->msgSem);			    	    \
    	    	}							    \
	    }								    \
    	}                                                                   \
    (RB_ID)->nestLevel--;						    \
    }

LOCAL BOOL   rBuffLibInstalled = FALSE;   /* protect from multiple inits */

/* global variables */

IMPORT BOOL  wvEvtBufferFullNotify;	/* event buffer full notification */

OBJ_CLASS rBuffClass;                   /* rBuff object Class */
CLASS_ID  rBuffClassId = &rBuffClass;   /* rBuff class ID */

WV_RBUFF_MGR_ID  wvRBuffMgrId = NULL;   /* WV rbuff mgr structure id */

/*******************************************************************************
*
* rBuffLibInit - initialise the ring of buffers library
*
* This routine initialises the ring of buffers library. If INCLUDE_RBUFF is
* defined in configAll.h, it is called by the root task, usrRoot(), in
* usrConfig.c.
*
* NOMANUAL
*
* RETURNS: OK, or ERROR if the library could not be initialized.
*/

STATUS rBuffLibInit (void)
    {   

    if (rBuffLibInstalled) 
        {
        return(TRUE);
        }


    /* Initialise the WindView rBuff Manager */

    if ((wvRBuffMgrId = malloc (sizeof (WV_RBUFF_MGR_TYPE))) == NULL)
	return (ERROR);

    wvRBuffMgrId->tid = 0;
    wvRBuffMgrId->priorityDefault = WV_RBUFF_MGR_PRIORITY;

    if (semCInit (&wvRBuffMgrId->msgSem, SEM_Q_PRIORITY, 0) == ERROR)
        {
        free (wvRBuffMgrId);
	wvRBuffMgrId = NULL;
        return (ERROR);
        }

    wvRBuffMgrId->msgWriteIndex = 0;
    wvRBuffMgrId->msgReadIndex = 0;

    rBuffLibInstalled = TRUE;

    /* initialize the rBuff class structure */

    return (classInit (rBuffClassId,
                sizeof (RBUFF_TYPE),
                OFFSET (RBUFF_TYPE, buffDesc),
                (FUNCPTR) rBuffCreate,
                (FUNCPTR) NULL,
                (FUNCPTR) rBuffDestroy));
    }
 

/*******************************************************************************
*
* rBuffCreate - create an extendable ring of buffers
*
* This routine creates an extendable ring of buffers from
* the specified partition.
*
* NOMANUAL
*/

BUFFER_ID rBuffCreate
    (
    void *Params
    )
    {

    RBUFF_ID              	rBuff;
    UINT32                	count;
    RBUFF_PTR             	newBuff;
    rBuffCreateParamsType * 	rBuffCreateParams = Params;

    if ((!rBuffLibInstalled) && (rBuffLibInit() == OK))
	{
	rBuffLibInstalled = TRUE;
	}

    if (!rBuffLibInstalled)
        {
#ifdef RBUFF_DEBUG
        logMsg ("rBuff: rBuffLib not installed\n",0,0,0,0,0,0);
#endif

        return (NULL);
        }

    /* validate params */

    if (rBuffCreateParams->minimum < 2 ||
            ((rBuffCreateParams->minimum > rBuffCreateParams->maximum) ||
               (rBuffCreateParams->maximum < 2 &&
                rBuffCreateParams->maximum != RBUFF_MAX_AVAILABLE)) ||
            rBuffCreateParams->buffSize == 0)
        {
#ifdef RBUFF_DEBUG
        logMsg ("rBuff: invalid params\n",0,0,0,0,0,0);
#endif

        return (NULL);
        }

    /*
     *  Set source partition for object class memory allocation
     *  - note this only works overall if all rBuffs share a source
     *    partition.
     */

    classMemPartIdSet(rBuffClassId,
        rBuffCreateParams->sourcePartition);

    /* allocate control structure */

    rBuff = (RBUFF_ID) objAlloc (rBuffClassId);

    if (rBuff == NULL)
        {
#ifdef RBUFF_DEBUG
        logMsg ("rBuff: objAlloc failed\n",0,0,0,0,0,0);
#endif

        return (NULL);
        }

    objCoreInit (&rBuff->buffDesc.objCore, rBuffClassId);

    /* record parameters */

    rBuff->info.srcPart   = rBuffCreateParams->sourcePartition;
    rBuff->info.options   = rBuffCreateParams->options;
    rBuff->info.buffSize  = rBuffCreateParams->buffSize;
    rBuff->info.threshold = rBuffCreateParams->threshold;
    rBuff->info.minBuffs  = rBuffCreateParams->minimum;
    rBuff->info.maxBuffs  = (unsigned int) rBuffCreateParams->maximum;
    rBuff->errorHandler   = rBuffCreateParams->errorHandler;

    if (semBInit (&rBuff->buffDesc.threshXSem, SEM_Q_PRIORITY, 
		  SEM_EMPTY) == ERROR)
        {
        objFree (rBuffClassId, (UINT8 *) rBuff);

        return (NULL);
        }

    if (semBInit (&rBuff->readBlk, SEM_Q_PRIORITY, SEM_FULL) == ERROR)
        {
        semTerminate (&rBuff->buffDesc.threshXSem);

        objFree (rBuffClassId, (UINT8 *) rBuff);

        return (NULL);
        }

    if (semBInit (&rBuff->bufferFull, SEM_Q_PRIORITY, SEM_EMPTY) == ERROR)
        {
        semTerminate (&rBuff->buffDesc.threshXSem);
        semTerminate (&rBuff->readBlk);

        objFree (rBuffClassId, (UINT8 *)rBuff);

        return (NULL);
        }

    if (semCInit (&rBuff->msgSem, SEM_Q_PRIORITY, 0) == ERROR)
        {
        semTerminate (&rBuff->bufferFull);
        semTerminate (&rBuff->buffDesc.threshXSem);
        semTerminate (&rBuff->readBlk);

        objFree (rBuffClassId, (UINT8 *)rBuff);

        return (NULL);
        }

    rBuff->rBuffMgrId = 0; /* ...so we can use rBuffDestroy safely */

    /*
     *  If things go wrong from here, use rBuffDestroy to throw back what
     *  we've caught.
     */

    /* allocate 'rBuffCreateParams->minimum' buffers */

    rBuff->info.currBuffs  =
    rBuff->info.emptyBuffs = 0;

    rBuff->buffWrite = NULL;

    for (count=0; count < rBuffCreateParams->minimum; count++)
        {

        /* First we need a buffer */

        newBuff =
            (RBUFF_PTR)
                memPartAlloc (rBuffCreateParams->sourcePartition,
                sizeof(RBUFF_BUFF_TYPE) + rBuffCreateParams->buffSize);

#ifdef RBUFF_DEBUG
        logMsg ("rBuff: adding buffer %p to ring\n",newBuff,0,0,0,0,0);
#endif

        /* newBuff will be returned as NULL if source partition is exhausted */

        /*  Don't need to lock ints around rBuffAdd as no-one knows about
         *  this rBuff.
         */

        if ((newBuff == NULL) || (rBuffAdd (rBuff, newBuff) == ERROR))
            {
#ifdef RBUFF_DEBUG
            logMsg ("rBuff: abandoned creation\n",0,0,0,0,0,0);
#endif
            rBuffDestroy ((BUFFER_ID) rBuff);

            return (NULL);
            }
        }

    /*
     * The first time around, spawn the rBuffMgr task. Once it exists,
     * don't create another one. It should never be deleted. This bit is
     * specific to WindView and will need attention for generic buffers
     */

    if (wvRBuffMgrId->tid == 0)
	{
    	wvRBuffMgrId->tid = taskSpawn (
    	    "tWvRBuffMgr",
    	    wvRBuffMgrId->priorityDefault,
            WV_RBUFF_MGR_OPTIONS,
            2048,
            wvRBuffMgr,
	    (int)wvRBuffMgrId,
            0,0,0,0,0,0,0,0,0);
	}

    rBuff->rBuffMgrId =	(int)wvRBuffMgrId;

    if (wvRBuffMgrId->tid == 0)
        {
#ifdef RBUFF_DEBUG
        logMsg ("rBuff: error creating wvRBuffMgr\n",0,0,0,0,0,0);
#endif
        rBuffDestroy ((BUFFER_ID) rBuff);

        return (NULL);
        }

    /* set control pointers */

    rBuff->nestLevel = 0;

    /* rBuff->buffWrite initialised by rBuffAdd */

    rBuff->buffRead  = rBuff->buffWrite;

    rBuff->dataWrite =
    rBuff->dataRead  = rBuff->buffWrite->dataStart;

    /* reset info */

    rBuff->info.maxBuffsActual   = rBuffCreateParams->minimum;

    rBuff->info.dataContent      =
    rBuff->info.writesSinceReset =
    rBuff->info.readsSinceReset  =
    rBuff->info.timesExtended    =
    rBuff->info.timesXThreshold  =
    rBuff->info.bytesWritten     =
    rBuff->info.bytesRead        =
    rBuff->info.bytesPeak        = 0;

    /* Reset msg passing mechanism */

    rBuff->msgOutstanding        = FALSE;

    rBuff->msgWriteIndex         =
    rBuff->msgReadIndex          = 0;

    /* now set up func ptrs allowing it to be called */

    rBuff->buffDesc.readReserveRtn = (FUNCPTR) rBuffReadReserve;
    rBuff->buffDesc.readCommitRtn  = (FUNCPTR) rBuffReadCommit;
    rBuff->buffDesc.writeRtn       = rBuffWrite;
    rBuff->buffDesc.flushRtn       = rBuffFlush;
    rBuff->buffDesc.threshold      = rBuffCreateParams->threshold;
    rBuff->buffDesc.nBytesRtn      = rBuffNBytes;

    /* made it! */

#ifdef RBUFF_DEBUG
    logMsg("Created rBuff with ID %p\n",rBuff,0,0,0,0,0);
#endif

    return ((BUFFER_ID) rBuff);
    }

int rBuffVerify 
    (
    BUFFER_ID rBuff
    )
    {
    if (OBJ_VERIFY (rBuff, rBuffClassId) == OK)	/* validate rBuff ID */
        {
	return (OK);
        }
    else
	{
	return (ERROR);
	}
    }

/*******************************************************************************
*
* rBuffWrite - Write data to a ring of buffers
*
* This routine writes data to an extendable ring of buffers. If the existing
* structure is full, and the existing number of buffers is less than the
* specified minimum, then this function will attempt to add another buffer to
* the ring.
*
* This function may be called from interrupt level.
*
* Mutually exclusive access must be guaranteed by the caller.
*
* NOMANUAL
*/

UINT8* rBuffWrite
    (
    BUFFER_ID buffId,		/* ring buffer id */
    UINT8 *   pDummy,		/* not used for WV ring buffer */
    UINT32    numOfBytes	/* number of bytes to reserve */
    )
    {
    BOOL startUploading;
    RBUFF_ID rBuff;			/* access this particular rBuff */
    UINT8 *returnPtr = (UINT8 *) ~0; /* Return a non-zero value if OK */

    /* Get access to the private members of this buffer's descriptor. */

    rBuff = (RBUFF_ID) buffId;

    /* Get out early if this request makes no sense. */

    if (numOfBytes > rBuff->info.buffSize)
        {
        return (NULL);
        }

    /* Critical Region Start */

    if (rBuff->info.dataContent >= rBuff->info.threshold)
        {
        startUploading = TRUE;
        }
    else
        {
        startUploading = FALSE;
        }

    if (rBuff->buffWrite->spaceAvail >= numOfBytes)
        {
        /* Record the start of the reserved area */

        returnPtr = rBuff->dataWrite;

        /*
         *  If we are consuming one of our precious empty buffs,
         *  check we have enough in reserve.
         */

        if (rBuff->buffWrite->dataLen == 0) 
            {
            --rBuff->info.emptyBuffs;
	    }

	if (rBuff->info.emptyBuffs < RBUFF_EMPTY_KEEP &&
            rBuff->info.currBuffs != rBuff->info.maxBuffs)
            {
            /* Schedule the rBuff Mgr to extend the buffer */

            RBUFF_SEND_MSG (RBUFF_MSG_ADD, MSG_PRI_URGENT, rBuff, 0);
	    }

        rBuff->buffWrite->dataLen    += numOfBytes;
        rBuff->dataWrite             += numOfBytes;
        rBuff->buffWrite->spaceAvail -= numOfBytes;

        /*
         *  It may be that the current buffer is full at this point,
         *  but don't do anything about it yet as the situation will
         *  be handled next time through. Also, it's possible that
         *  the current buffer may even have been emptied by then.
         */

        }
    else
        {

        /*
         *  Data won't fit in this buffer (at least not entirely),
         *  is the next buffer available?
         */

        if (rBuff->buffWrite->next == rBuff->buffRead)
            {

            /*  We've caught the reader. This will only happen when
             *  we have actually filled the maximum allocation of
             *  buffers *or* the reserved buffer has been filled
             *  before the buffer has had the opportunity to
             *  be extended. We cannot extend the buffer at this point
             *  as we may be in a critical region, the design is such
             *  that if the buffer could be extended it should have
             *  been done by now (for that matter, the wrap-around
             *  should have also occurred).
             *
             *  If we have filled the reserved buffer without having
             *  the opportunity to extend the buffer then the buffer
             *  is configured badly and must be retuned.
             *
             *  Meanwhile, at this moment, options are:
             *
             *  1) If we have filled the entire buffer and cannot wrap-
             *     around then return ERROR.
             *  2) If we have filled the entire buffer but can wrap-
             *     around then do that.
             */           

            if (rBuff->info.options & RBUFF_WRAPAROUND)
                {

                /*
                 *  OK, perform an inline ring extension supplying NULL
                 *  as the new buffer forcing the ring to wrap-around.
                 *  This is a deterministic action.
                 */

                /* Interrupts already locked out */

                rBuffAdd(rBuff, NULL);
                }
            else
                {
                /* Oh dear, can't wrap-round */

            	RBUFF_SEND_MSG (RBUFF_MSG_FULL, MSG_PRI_NORMAL, rBuff, 0);
		wvEvtBufferFullNotify = TRUE;
                return (NULL);
                }
            }

        /*
         *  OK.
         *  In the case that this function is writing the data, it
         *  can be shared between this and next buffer.
         *  In the case that we are only reserving the space, we must
         *  return a pointer to contiguous space and therefore skip the
         *  remainder of the previous buffer.
         */

        /* skip the remainder of the current buffer */

        rBuff->buffWrite->spaceAvail = 0;

        /* Move on to the next buffer */

        rBuff->buffWrite = rBuff->buffWrite->next;

        /* Record the start of the reserved area */

        returnPtr = rBuff->buffWrite->dataStart;

        /* Point dataWrite past the reserved area */

        rBuff->dataWrite = rBuff->buffWrite->dataStart;

        rBuff->buffWrite->dataLen    = numOfBytes;
        rBuff->dataWrite            += numOfBytes;
        rBuff->buffWrite->spaceAvail =
            rBuff->info.buffSize - numOfBytes;

        if (--rBuff->info.emptyBuffs < RBUFF_EMPTY_KEEP &&
            rBuff->info.currBuffs != rBuff->info.maxBuffs)
            {
            /* Schedule the rBuff Mgr to extend the buffer */

            RBUFF_SEND_MSG (RBUFF_MSG_ADD, MSG_PRI_URGENT, rBuff, 0);
            }
        }

    rBuff->info.dataContent  += numOfBytes;

    /* update info */

    if(rBuff->info.dataContent > rBuff->info.bytesPeak)
        {
        rBuff->info.bytesPeak = rBuff->info.dataContent;
        }

    rBuff->info.bytesWritten += numOfBytes;

    rBuff->info.writesSinceReset++;

    if (!startUploading && rBuff->info.dataContent >= rBuff->info.threshold)
        {
        rBuff->info.timesXThreshold++;

        if (!(rBuff->info.options & RBUFF_UP_DEFERRED))
            {
            /*
             *  Signal for uploading to begin. Note that if we have just
             *  reserved space it is imperative that uploading does not
             *  actually begin until the data is in the buffer.
             */

#if (CPU_FAMILY == I80X86)
            portWorkQAdd1 ((FUNCPTR)semBGiveDefer,
                (int) &rBuff->buffDesc.threshXSem);
#else
            workQAdd1 ((FUNCPTR)semBGiveDefer,
                (int) &rBuff->buffDesc.threshXSem);
#endif
            }
        }


    /* Critical Region End */

    return (returnPtr);
    }


/*******************************************************************************
*
* rBuffRead - Read data from a ring of buffers
*
* This routine reads data from an extendable ring of buffers.
*
* This function assumes mutually exclusive access is guaranteed by the caller.
*
* NOMANUAL
*/

INT32 rBuffRead
    (
    BUFFER_ID 	buffId,		/* generic buffer descriptor */
    UINT8 *   	dataDest,
    UINT32  	numOfBytes
    )
    {
    UINT32 	bytesToCopy;
    UINT32	remaining = numOfBytes;
    RBUFF_ID 	rBuff;		/* access private members of this rBuff desc */

    /* Get access to the private members of this particular rBuff. */

    rBuff = (RBUFF_ID) buffId;

    /* Critical Region Start */

    while (remaining > 0)
        {
        bytesToCopy = (rBuff->buffRead->dataLen < remaining ?
                        rBuff->buffRead->dataLen : remaining);

        if (bytesToCopy > 0)
            {
            memcpy (dataDest,rBuff->dataRead,bytesToCopy);

            remaining                -= bytesToCopy;
            dataDest                 += bytesToCopy;

            /* Update buffer */

            rBuff->buffRead->dataLen -= bytesToCopy;

            rBuff->dataRead          += bytesToCopy;
            rBuff->info.dataContent  -= bytesToCopy;
            }

        if (rBuff->buffRead->dataLen != 0)
            {
            /* This buffer is now empty */

            rBuffHandleEmpty (rBuff);
            }
        else
            {
            /* this buffer is not yet emptied */

            rBuff->dataRead += bytesToCopy;
            }
        }

    /* update info */

    rBuff->info.bytesRead += (numOfBytes - remaining);

    rBuff->info.readsSinceReset++;

    /* Critical Region End */

    return (numOfBytes - remaining);
    }


/*******************************************************************************
*
* rBuffReadReserve - Return the number of contiguous bytes available for
*                    reading.
*
* NOMANUAL
*/

UINT32 rBuffReadReserve
    (
    BUFFER_ID buffId,		/* generic identifier for this buffer */
    UINT8 **src
    )
    {
    RBUFF_ID rBuff;		/* specific identifier for this rBuff */
    INT32 bytesAvailable; 

    /* Get access to the private members of this particular rBuff. */

    rBuff = (RBUFF_ID) buffId;

    /* Generate and return the available contiguous bytes. */

    if ((bytesAvailable = rBuff->buffRead->dataLen))
        {
        *src = rBuff->dataRead;
        }
    else
        {
        *src = NULL;
        }
    return (bytesAvailable);
    }


/*******************************************************************************
*
* rBuffReadCommit - Move the read data ptr along a ring of buffers
*
* This routine moves the data ptr along a ring of buffers.
*
* It is equivalent to reading data from the buffers and should be used
* when the data has been copied elsewhere.
*
* This function assumes mutually exclusive access is guaranteed by the caller.
*
* NOMANUAL
*/

STATUS rBuffReadCommit
    (
    BUFFER_ID buffId,		/* generic identifier for this buffer */
    UINT32  numOfBytes
    )
    {
    RBUFF_ID rBuff;		/* specific identifier for this rBuff */

    /* Get access to the private members of this particular rBuff. */

    if (!numOfBytes)
    {
        return (OK);
    }

    rBuff = (RBUFF_ID) buffId;

    /* Critical Region Start */

    if (numOfBytes == rBuff->buffRead->dataLen)
        {
        rBuffHandleEmpty (rBuff);
        }
    else if (numOfBytes < rBuff->buffRead->dataLen)
        {
        rBuff->buffRead->dataLen -= numOfBytes;
        rBuff->dataRead += numOfBytes;
        }
    else
        {
        /* Moving ahead through multiple buffers */

        /* Not yet supported */

        return(ERROR);
        }

    rBuff->info.dataContent -= numOfBytes;

    /* update info */

    rBuff->info.bytesRead += numOfBytes;

    rBuff->info.readsSinceReset++;

    /* Critical Region End */

    return (OK);
    }


/*******************************************************************************
*
* rBuffFlush - Flush data from a ring of buffers
*
* This routine causes any data held in a buffer to be uploaded and is
* used to clear data that falls below the specified threshold.
*
* NOMANUAL
*/

STATUS rBuffFlush
    (
    BUFFER_ID buffId		/* generic identifier for this buffer */
    )
    {
    RBUFF_ID rBuff;		/* specific identifier for this rBuff */
    int result, originalThreshold;

    /* Get access to the private members of this particular rBuff. */

    rBuff = (RBUFF_ID) buffId;

    result = OK;

    if (!rBuff->info.dataContent)
        {
        return(OK);
        }

    /* Store the original threshold */

    originalThreshold = rBuff->info.threshold;

    rBuff->info.threshold = 0;

    while (rBuff->info.dataContent > 0 && result != ERROR)
        {
        if (semGive (&rBuff->buffDesc.threshXSem) == OK)
            {
            /* Cause a reschedule to allow uploader in */

            taskDelay(0);
            }
        else
            {
            result = ERROR;
            }
        }

    rBuff->info.threshold = originalThreshold;

    return(result);
}


/*******************************************************************************
*
* rBuffReset - reset an extendable ring of buffers
*
* This routine resets an already created extendable ring of buffers to its
* initial state. This loses any data held in the buffer. Any buffers held
* above the specified minimum number are deallocated.
*
* NOMANUAL
*/

STATUS rBuffReset
    (
    BUFFER_ID buffId		/* generic identifier for this buffer */
    )
    {
    RBUFF_ID rBuff;		/* specific identifier for this rBuff */
    RBUFF_LOCK (rBuff);

    /* Get access to the private members of this particular rBuff. */

    rBuff = (RBUFF_ID) buffId;

    /* Now reset the ring of buffers. */

    if (rBuff->info.currBuffs > rBuff->info.minBuffs)
        {
        UINT32 excessBuffs, count;

        /* drop the excess buffers */

        excessBuffs = rBuff->info.currBuffs - rBuff->info.minBuffs;

        for (count=0;count < excessBuffs;count++)
            {
            RBUFF_PTR tempPtr;

            tempPtr = rBuff->buffWrite->next; /* should not be NULL! */

            rBuffFree (rBuff,rBuff->buffWrite);

            rBuff->buffWrite = tempPtr;
            }
        }

    /* Make sure the 'read' buffer pointer points somewhere sensible */

    rBuff->buffRead = rBuff->buffWrite;

    /* and reset the byte ptrs */

    rBuff->dataRead         =
    rBuff->dataWrite        = rBuff->buffWrite->dataStart;

    rBuff->info.currBuffs        =
    rBuff->info.maxBuffsActual   = rBuff->info.minBuffs;

    /* now traverse the ring resetting the individual buffers */

    {
    RBUFF_PTR backHome = rBuff->buffWrite;
    RBUFF_PTR currBuff = rBuff->buffWrite;

    do
     	{
        currBuff->dataLen    = 0;
        currBuff->spaceAvail = rBuff->info.buffSize;

        currBuff = currBuff->next;
        }
    while (currBuff != backHome);

    }

    /* Reset msg passing mechanism */

    rBuff->msgOutstanding        = FALSE;

    rBuff->msgWriteIndex         =
    rBuff->msgReadIndex          = 0;

    rBuff->nestLevel = 0;

    /* reset info */

    rBuff->info.emptyBuffs       = rBuff->info.minBuffs;
    rBuff->info.dataContent      =
    rBuff->info.writesSinceReset =
    rBuff->info.readsSinceReset  =
    rBuff->info.timesExtended    =
    rBuff->info.timesXThreshold  =
    rBuff->info.bytesWritten     =
    rBuff->info.bytesRead        = 0;

    semGive (&rBuff->bufferFull);

    RBUFF_UNLOCK (rBuff);

    return (OK);
}


/*******************************************************************************
*
* rBuffNBytes - Returns the total number of bytes held in a ring of buffers
*
* NOMANUAL
*/

INT32 rBuffNBytes
    (
    BUFFER_ID buffId		/* generic identifier for this buffer */
    )
    {
    RBUFF_ID rBuff;		/* specific identifier for this rBuff */

    /* Get access to the private members of this particular rBuff. */

    rBuff = (RBUFF_ID) buffId;

    return (rBuff->info.dataContent);
    }


/*******************************************************************************
*
* rBuffAdd - Add a new buffer to an extendable ring of buffers
*
* This routine adds a further buffer to an already created or partially created
* extendable ring of buffers unless the maximum number is already allocated.
*
* This function assumes mutually exclusive access is guaranteed by the caller.
*
* NOMANUAL
*/

STATUS rBuffAdd
    (
    RBUFF_ID 	rBuff,
    RBUFF_PTR 	newBuff
    )
    {
    if (rBuff->info.emptyBuffs >= RBUFF_EMPTY_KEEP &&
        rBuff->info.currBuffs  >= rBuff->info.minBuffs)
        {
        /*
         *  OK, the uploader got in so now we don't need to extend the
         *  buffer.
         */

#ifdef RBUFF_DEBUG
        logMsg ("rBuff: abandoned add\n",0,0,0,0,0,0);
#endif

        if (newBuff)
            {
            memPartFree (rBuff->info.srcPart, (UINT8 *) newBuff);
            }

        return (OK);
        }

    if (rBuff->info.currBuffs == rBuff->info.maxBuffs || newBuff == NULL)
        {

        /*
         *  We are at the maximum ring size already or the source
         *  partition is exhausted, there's still hope...
         *
         *  ...can we wrap-around?
         */

        if (!(rBuff->info.options & RBUFF_WRAPAROUND))
            {
            /* We aren't allowed to wrap around */

            return (ERROR);
            }
        else
            {
            RBUFF_PTR reclaim;

            /*  We can wrap around by comandeering the 'oldest'
             *  buffer in the ring.
             *
             *  It is possible (probable?) that this buffer is the
             *  one to be read from next so handle this.
             */

            reclaim = rBuff->buffWrite->next;

            if (rBuff->buffRead == reclaim)
                {
                /* Move the reader along */

                rBuff->buffRead = rBuff->buffRead->next;
                rBuff->dataRead = rBuff->buffRead->dataStart;
                }

            rBuff->info.dataContent  -= reclaim->dataLen;

            /* Reset the buffer */

            reclaim->dataLen    = 0;
            reclaim->spaceAvail = rBuff->info.buffSize;

            rBuff->info.emptyBuffs++;

            return (OK);
            }
        }

    newBuff->dataStart  = (UINT8 *) newBuff + sizeof(RBUFF_BUFF_TYPE);
    newBuff->dataEnd    = newBuff->dataStart + rBuff->info.buffSize;
    newBuff->spaceAvail = rBuff->info.buffSize;
    newBuff->dataLen    = 0;

    /* Now link the buffer into the ring */

    if (rBuff->buffWrite != NULL)
        {
        RBUFF_PTR tempPtr;

        tempPtr = rBuff->buffWrite->next;

        rBuff->buffWrite->next = newBuff;

        /* close the ring */

        newBuff->next = tempPtr;

        }
    else
        {

        /* ring is empty, must be creation time */

        rBuff->buffWrite =
        rBuff->buffRead  =
        newBuff->next    = newBuff;
        }

    /* Maintain statistics */

    if (++(rBuff->info.currBuffs) > rBuff->info.maxBuffsActual)
        {
        rBuff->info.maxBuffsActual++;
        }

    rBuff->info.timesExtended++;
    rBuff->info.emptyBuffs++;

    return (OK);
}


/*******************************************************************************
*
* rBuffFree - Free a specified buffer
*
* This function must only be called from task level.
* Semaphores are used to protect access at task-level. However, protection
* from access at interrupt level must be guaranteed by the caller.
*
* NOMANUAL
*/

STATUS rBuffFree
    (
    RBUFF_ID 	rBuff,
    RBUFF_PTR 	buffToFree
    )
    {
    RBUFF_PTR 	ptrToBuff, backHome, ptrToPreviousBuff;

    if (buffToFree == NULL)
        {
        return(ERROR);
        }

    /* find the pointer to the buffer to be freed */

    ptrToBuff         = rBuff->buffWrite->next;
    backHome          = rBuff->buffWrite;
    ptrToPreviousBuff = rBuff->buffWrite;

    while ((ptrToBuff != buffToFree) && (ptrToBuff != backHome))
        {
        ptrToPreviousBuff = ptrToBuff;
        ptrToBuff         = buffToFree;
        }

    if (ptrToBuff == backHome)
        {
        /* an error has occured, possibly invalid buff ptr passed */

        return (ERROR);
        }

    /* OK, de-link the buffer to be freed */

    ptrToPreviousBuff->next = buffToFree->next;

    /* Schedule the rBuff Mgr to actually free the buffer */

    RBUFF_SEND_MSG (RBUFF_MSG_FREE, MSG_PRI_NORMAL, rBuff, buffToFree);

    /* Maintain statistics */

    rBuff->info.emptyBuffs--;
    rBuff->info.currBuffs--;

    return (OK);
    }


/*******************************************************************************
*
* rBuffDestroy - Destroy an extendable ring of buffers
*
* This routine destroys an already created or partially created extendable ring
* of buffers. This loses any data held in the buffer. All resources held are
* returned to the system.
*
* NOMANUAL
*/

STATUS rBuffDestroy
    (
    BUFFER_ID 	buffId		/* generic identifier for this buffer */
    )
    {
    RBUFF_ID 	rBuff;		/* specific identifier for this rBuff */
    RBUFF_PTR 	tempPtr;

    /* Get access to the private members of this particular rBuff. */

    rBuff = (RBUFF_ID) buffId;

    RBUFF_LOCK (rBuff);

#if 0
    if (OBJ_VERIFY (rBuff, rBuffClassId) != OK)	/* validate rBuff ID */
	{
	((OBJ_ID)rBuff)->pObjClass, rBuffClassId);
	RBUFF_UNLOCK (rBuff);
	return (ERROR);
	}
#endif

    objCoreTerminate (&rBuff->buffDesc.objCore);   /* invalidate this object */

    /* Break the ring and traverse the list, freeing off   */
    /* the buffers. buffWrite is used as the start point   */
    /* this works if the ring creation is abandoned early. */

    if (rBuff->buffWrite != NULL)
        {

        /* Break the ring */

        tempPtr = rBuff->buffWrite->next;
        rBuff->buffWrite->next = NULL;
        rBuff->buffWrite = tempPtr;

        /* traverse and free */

        while(rBuff->buffWrite != NULL)
            {
            tempPtr = rBuff->buffWrite->next;
            memPartFree (rBuff->info.srcPart,(UINT8 *)rBuff->buffWrite);
            rBuff->buffWrite = tempPtr;
            }
        }
    else
        {
        /* ring hasn't been created so don't worry about it */
        }

    /* now free off the control structure */

    semTerminate (&rBuff->buffDesc.threshXSem);
    semTerminate (&rBuff->readBlk);
    semTerminate (&rBuff->bufferFull);

    objFree (rBuffClassId, (UINT8 *) rBuff);

    /* we're done */

    return (OK);
    }


/*******************************************************************************
*
* rBuffSetFd - Set a new fd as the data upload destination
*
*
* NOMANUAL
*/

STATUS rBuffSetFd
    (
    BUFFER_ID 	buffId,		/* generic identifier for this buffer */
    int 	fd
    )
    {
    RBUFF_ID 	rBuff;		/* specific identifier for this rBuff */

    RBUFF_LOCK (rBuff);

    /* Get access to the private members of this particular rBuff. */

    rBuff = (RBUFF_ID) buffId;
    rBuff->fd = fd;
    RBUFF_UNLOCK (rBuff);

    return (OK);
    }


/*******************************************************************************
*
* rBuffHandleEmpty - Handles a newly empty buffer by deciding whether to free
*                    it off.
*
* This function assumes mutually exclusive access is guaranteed by the caller.
*
* NOMANUAL
*/

LOCAL STATUS rBuffHandleEmpty
    (
    RBUFF_ID rBuff
    )
    {
    BOOL      moveAlong = TRUE;
    RBUFF_PTR buffToFree;

    /*
     *  Determine if we are to move around the ring and if the
     *  newly empty buffer is to be freed, and then do the actions.
     *  This avoids having to fumble with ptrs as we move around
     *  from a buffer that has been freed.
     */

    if (rBuff->buffRead == rBuff->buffWrite)
        {

        /*
         *  We've caught the writer so might as well reset this
         *  buffer and make efficient use of it.
         */

        rBuff->dataRead  =
            rBuff->dataWrite = rBuff->buffWrite->dataStart;

        rBuff->buffRead->spaceAvail = rBuff->info.buffSize;
        rBuff->buffRead->dataLen     = 0;

        /* In this case, we don't move along to the next buffer */

        moveAlong = FALSE;
        }

    if (++(rBuff->info.emptyBuffs) > RBUFF_EMPTY_KEEP &&
           rBuff->info.currBuffs > rBuff->info.minBuffs)
        {
        /* We already have enough empty buffs, so free one off */

        if (rBuff->buffRead != rBuff->buffWrite)
            {
            buffToFree = rBuff->buffRead;
            }
        else
            {
            /*
             *  As the reader and writer are in the same buffer
             *  we know it's safe to free off the next buffer.
             */

            buffToFree = rBuff->buffRead->next;
            }
        }
    else
        {
        /* we've just emptied one of the 'core' buffers */

        rBuff->buffRead->spaceAvail = rBuff->info.buffSize;
        rBuff->buffRead->dataLen    = 0;

        /* Don't free */

        buffToFree = NULL;
        }

    if (moveAlong)
        {
        /* Move round the ring */

        rBuff->buffRead = rBuff->buffRead->next;
        rBuff->dataRead = rBuff->buffRead->dataStart;
        }
    else
        {
        /* reset flag for next time */

        moveAlong = TRUE;
        }

    if (/* there's a */ buffToFree)
        {
#ifdef RBUFF_DEBUG
        logMsg ("rBuff: Freeing buffer %p\n",
            buffToFree,0,0,0,0,0);
#endif
        rBuffFree (rBuff, buffToFree);
        }

    return (OK);
    }


/*******************************************************************************
*
* wvRBuffMgr - The function is spawned as a task to manage WV rBuff extension.
*
* It waits for a msg which carries the ID of a buffer which has had a change
* of the number of empty buffers held. If there are less the requested to be
* reserved than an attempt will be made to make more available. If there are
* more empty buffers than required than some will be freed.
*
* For each msg received, the task will achieve the required number of free
* buffers. This may result in redundant msgs being received.
*
* This manager is specific to WindView
*
* NOMANUAL
*/

int wvRBuffMgr
    (
    WV_RBUFF_MGR_TYPE * rBuffMgrId  /* id of the WindView rBuffMgr structure */	
    )
    {
    UINT32		    	busy;
    int          	    	key;
    RBUFF_PTR    	    	newBuff;
    WV_RBUFF_MGR_MSG_TYPE* 	pMsg;




#ifdef RBUFF_DEBUG
        logMsg ("rBuffMgr %p created\n",rBuffMgrId,0,0,0,0,0);
#endif

    for(;;)
        {

	semTake (&(rBuffMgrId->msgSem), WAIT_FOREVER);
	pMsg = &(rBuffMgrId->msg[rBuffMgrId->msgReadIndex]);

#ifdef RBUFF_DEBUG
        logMsg ("rBuffMgr: (%d) rBuff %p type 0x%x arg 0x%x\n",
		rBuffMgrId->msgReadIndex,
                pMsg->ringId, 
                pMsg->msgType, 
                pMsg->arg, 
                0, 0);
#endif

        busy = TRUE;

        /* Determine what needs to be done */

        switch (pMsg->msgType)
            {
            case RBUFF_MSG_ADD :
                {

                /*
                 *  This message may be generated one or more times by
                 *  event logging, and the condition that caused the msg
                 *  may no longer be TRUE, so just take this as a prompt
                 *  to see if the ring needs extending.
                 */

                /* Make sure that we don't request extension recursively */

                pMsg->ringId->nestLevel++;

                while
                    (pMsg->ringId->info.emptyBuffs < RBUFF_EMPTY_KEEP &&
                    (pMsg->ringId->info.currBuffs  < pMsg->ringId->info.maxBuffs ||
                        pMsg->ringId->info.options & RBUFF_WRAPAROUND) &&
                    busy)
                    {

                    /* Looks like we are going to need a buffer */

                    newBuff = (RBUFF_PTR)
                        memPartAlloc (pMsg->ringId->info.srcPart,
                            sizeof(RBUFF_BUFF_TYPE) + pMsg->ringId->info.buffSize);

                    /* of course, newBuff may be NULL... */

                    key = intLock();

#ifdef RBUFF_DEBUG
                    logMsg ("rBuffMgr: adding buffer %p\n",newBuff,0,0,0,0,0);
#endif

		    if (rBuffAdd (pMsg->ringId, newBuff) == ERROR)
                        {
                        /* ring cannot be extended so give up trying */

                        busy = FALSE;
                        }

                    intUnlock (key);
                    }
                }

                pMsg->ringId->nestLevel--;

            break;

            case RBUFF_MSG_FREE :
                {
                /* This message carries a ptr to the buffer to be freed */

#ifdef RBUFF_DEBUG
                logMsg ("rBuffMgr: freeing buffer %p\n", pMsg->arg, 0,0,0,0,0);
#endif

                memPartFree (pMsg->ringId->info.srcPart, (UINT8 *) pMsg->arg);
                }
            break;

            case RBUFF_MSG_FULL :
    		logMsg ("Ring Buffer %p full. Stopped event logging.\n",
    		        (unsigned int)pMsg->ringId,0,0,0,0,0);
	    break;

	    default :
                logMsg ("Unknown message type %d\n", pMsg->msgType, 0,0,0,0,0);
	    break;
            }

         if (++rBuffMgrId->msgReadIndex > (WV_RBUFF_MGR_MSGQ_MAX - 1))
             {
             rBuffMgrId->msgReadIndex = 0;
             }

         pMsg->ringId->msgOutstanding = FALSE;

         }
    }

/*******************************************************************************
*
* wvRBuffMgrPrioritySet - set the priority of the WindView rBuff manager (WindView)
*
* This routine changes the priority of the `tWvRBuffMgr' task to the value of 
* <priority>.  Priorities range from 0, the highest priority, to 255, the
* lowest priority.  If the task is not yet running, this priority is
* used when it is spawned.
* 
* RETURNS: OK, or ERROR if the priority can not be set.
*
* SEE ALSO: taskPrioritySet(),
* .pG "Basic OS"
*/

STATUS wvRBuffMgrPrioritySet
    (
    int	     priority           /* new priority */
    )
    {

    if (!rBuffLibInstalled)
        {
    	if (rBuffLibInit() == OK)
	    rBuffLibInstalled = TRUE;
    	else
            return (ERROR);
	}

    if (wvRBuffMgrId->tid != 0)
 	if (taskPrioritySet (wvRBuffMgrId->tid, priority) != OK)
	    return (ERROR);

    wvRBuffMgrId->priorityDefault = priority;
    return (OK);
    }

/*******************************************************************************
*
* rBuffUpload - Copy data that is added to the buffer to the specified 'fd'
*
* NOMANUAL
*/

STATUS rBuffUpload
    (
    BUFFER_ID 	buffId,		/* generic identifier for this buffer */
    int 	fd
    )
    {
    RBUFF_ID 	rBuff;		/* specific identifier for this rBuff */
    UINT8 *	src;
    int 	lockKey;
    STATUS 	result = OK;

    /* Get access to the private members of this particular rBuff. */

    rBuff = (RBUFF_ID) buffId;

    rBuff->fd = fd;

    while (result == OK)
        {

        /* Wait for the dinner gong */

        logMsg ("Waiting for data\n",0,0,0,0,0,0);

        semTake (&rBuff->buffDesc.threshXSem,WAIT_FOREVER);

        logMsg("Uploading...\n",0,0,0,0,0,0);

        /* tuck in... */

        while (result == OK && rBuff->info.dataContent >
              (rBuff->info.threshold / 4))
            {
            UINT32 bytesToWrite = rBuffReadReserve(buffId, &src);
            UINT32 bytesWritten;

            if (bytesToWrite > 4096)
                {
                    bytesToWrite = 4096;
                }

#ifdef RBUFF_DEBUG

            logMsg ("%#x bytes from %p (dataContent=%#x)\n",
                bytesToWrite,
                (int) src,
                rBuff->info.dataContent,0,0,0);
#endif


            /* move the data */

#if 0
{
UINT8 buffer[100];

bcopy(src,buffer,bytesToWrite);
buffer[bytesToWrite] = '\0';

printf("%s\n",buffer);
}
#endif

            if ((bytesWritten =
                write (fd, src, bytesToWrite)) == -1)
                {
                logMsg ("Upload error!\n",0,0,0,0,0,0);

                if (rBuff->errorHandler)
                    {

                    /* Dial 999 / 911 */

                    (*rBuff->errorHandler)();
                    }

                result = ERROR;
                }
            else
                {
                /* move the buffer pointers along */

                lockKey = intLock();

                result = rBuffReadCommit (buffId,bytesWritten);

                intUnlock (lockKey);

                taskDelay (0);
                }
            }
        }
    return (result);
    }

#else  /* GENERIC_RBUFF */

/*
 * The code below constitutes the generic ring buffer scheme on which
 * WindView 2.0 was originally based. If there is a need to go back to
 * it, #define GENERIC_RBUFF at the top of this file or build with
 * EXTRA_DEFINE="-DGENERIC_RBUFF" in the make command line
 */

#include "vxWorks.h"
#include "semLib.h"
#include "classLib.h"
#include "errno.h"
#include "intLib.h"
#include "logLib.h"
#include "taskLib.h"
#include "fioLib.h"
#include "rBuffLib.h"


#include "private/wvBufferP.h"
#include "private/classLibP.h"
#include "private/objLibP.h"
#include "private/workQLibP.h"

#include "stdio.h"
#include "string.h"
#include "unistd.h"

extern int rBuffMgr();

#if (CPU_FAMILY == I80X86)
extern void portWorkQAdd1();
#endif

/* locals */

#if defined(__STDC__) || defined(__cplusplus)

LOCAL STATUS rBuffAdd
    (
    RBUFF_ID  rBuff,
    RBUFF_PTR buffToAdd
    );

LOCAL STATUS  rBuffFree
    (
    RBUFF_ID  rBuff,
    RBUFF_PTR buffToFree
    );

LOCAL STATUS rBuffHandleEmpty
    (
    RBUFF_ID rBuff
    );

#else	/* __STDC__ */

LOCAL STATUS rBuffAdd();
LOCAL STATUS rBuffFree();
LOCAL STATUS rBuffHandleEmpty();

#endif	/* __STDC__ */

extern  BOOL kernelState;

#if (CPU_FAMILY == I80X86)

#define RBUFF_SEND_MSG(RMSG_TYPE,RMSG_PRI,RMSG_ID,RMSG_PARAM) \
if((!(RMSG_ID)->nestLevel++) && !(RMSG_ID)->msgOutstanding)   \
{                                                             \
    (RMSG_ID)->msgOutstanding = TRUE;                         \
    (RMSG_ID)->msg[(RMSG_ID)->msgWriteIndex][0] = RMSG_TYPE;  \
    (RMSG_ID)->msg[(RMSG_ID)->msgWriteIndex][1] = (UINT32) RMSG_PARAM; \
                                                              \
    if(++(RMSG_ID)->msgWriteIndex > RBUFF_MAX_MSGS)           \
    {                                                         \
        (RMSG_ID)->msgWriteIndex = 0;                         \
    }                                                         \
                                                              \
    if(kernelState)                                           \
    {                                                         \
        portWorkQAdd1((FUNCPTR)semCGiveDefer, (int)&(RMSG_ID)->msgSem); \
    }                                                         \
    else                                                      \
    {                                                         \
        semCGiveDefer(&(RMSG_ID)->msgSem);                    \
    }                                                         \
}                                                             \
                                                              \
(RMSG_ID)->nestLevel--;

#else /* CPU_FAMILY == I80X86 */

#define RBUFF_SEND_MSG(RMSG_TYPE,RMSG_PRI,RMSG_ID,RMSG_PARAM) \
if((!(RMSG_ID)->nestLevel++) && !(RMSG_ID)->msgOutstanding)   \
{                                                             \
    (RMSG_ID)->msgOutstanding = TRUE;                         \
    (RMSG_ID)->msg[(RMSG_ID)->msgWriteIndex][0] = RMSG_TYPE;  \
    (RMSG_ID)->msg[(RMSG_ID)->msgWriteIndex][1] = (UINT32) RMSG_PARAM; \
                                                              \
    if(++(RMSG_ID)->msgWriteIndex > RBUFF_MAX_MSGS)           \
    {                                                         \
        (RMSG_ID)->msgWriteIndex = 0;                         \
    }                                                         \
                                                              \
    if(kernelState)                                           \
    {                                                         \
        workQAdd1((FUNCPTR)semCGiveDefer, (int)&(RMSG_ID)->msgSem); \
    }                                                         \
    else                                                      \
    {                                                         \
        semCGiveDefer(&(RMSG_ID)->msgSem);                    \
    }                                                         \
}                                                             \
                                                              \
(RMSG_ID)->nestLevel--;

#endif /* CPU_FAMILY == I80X86 */

LOCAL BOOL   rBuffLibInstalled = FALSE;   /* protect from multiple inits */

/* global variables */

OBJ_CLASS rBuffClass;                   /* rBuff object Class */
CLASS_ID  rBuffClassId = &rBuffClass;   /* rBuff class ID */
int       rBuffMgrPriorityDefault = RBUFF_MGR_PRIORITY; /* default ring buff */
                                        /* manager priority */

/*******************************************************************************
*
* rBuffLibInit - initialise the ring of buffers library
*
* This routine initialises the ring of buffers library. If INCLUDE_RBUFF is
* defined in configAll.h, it is called by the root task, usrRoot(), in
* usrConfig.c.
*
* NOMANUAL
*
* RETURNS: OK, or ERROR if the library could not be initialized.
*/

STATUS rBuffLibInit (void)
    {   

    if(rBuffLibInstalled) {
        return(TRUE);
    }

    rBuffLibInstalled = TRUE;

    /* Initialise the rBuff Manager */

    /* initialize the rBuff class structure */

    return (classInit (rBuffClassId,
                sizeof (RBUFF_TYPE),
                OFFSET (RBUFF_TYPE, buffDesc),
                (FUNCPTR) rBuffCreate,
                (FUNCPTR) NULL,
                (FUNCPTR) rBuffDestroy));
    }
 

/*******************************************************************************
*
* rBuffCreate - create an extendable ring of buffers
*
* This routine creates an extendable ring of buffers from
* the specified partition.
*
* NOMANUAL
*/

BUFFER_ID rBuffCreate
    (
    void *Params
    )
    {

    RBUFF_ID              rBuff;
    UINT32                count;
    RBUFF_PTR             newBuff;
    rBuffCreateParamsType *rBuffCreateParams = Params;

    if ((!rBuffLibInstalled) && (rBuffLibInit()) == OK)
	{
	rBuffLibInstalled = TRUE;
	}

    if(!rBuffLibInstalled)
        {
#ifdef RBUFF_DEBUG
        logMsg ("rBuff: rBuffLib not installed\n",0,0,0,0,0,0);
#endif

        return(NULL);
        }

    /* validate params */

    if (rBuffCreateParams->minimum < 2 ||
            ((rBuffCreateParams->minimum > rBuffCreateParams->maximum) ||
               (rBuffCreateParams->maximum < 2 &&
                rBuffCreateParams->maximum != RBUFF_MAX_AVAILABLE)) ||
            rBuffCreateParams->buffSize == 0)
        {
#ifdef RBUFF_DEBUG
        logMsg ("rBuff: invalid params\n",0,0,0,0,0,0);
#endif

        return(NULL);
        }

    /*
     *  Set source partition for object class memory allocation
     *  - note this only works overall if all rBuffs share a source
     *    partition.
     */

    classMemPartIdSet(rBuffClassId,
        rBuffCreateParams->sourcePartition);

    /* allocate control structure */

    rBuff = (RBUFF_ID) objAlloc (rBuffClassId);

    if(rBuff == NULL)
        {
#ifdef RBUFF_DEBUG
        logMsg ("rBuff: objAlloc failed\n",0,0,0,0,0,0);
#endif

        return(NULL);
        }

    objCoreInit (&rBuff->buffDesc.objCore, rBuffClassId);

    /* record parameters */

    rBuff->info.srcPart   = rBuffCreateParams->sourcePartition;
    rBuff->info.options   = rBuffCreateParams->options;
    rBuff->info.buffSize  = rBuffCreateParams->buffSize;
    rBuff->info.threshold = rBuffCreateParams->threshold;
    rBuff->info.minBuffs  = rBuffCreateParams->minimum;
    rBuff->info.maxBuffs  = (unsigned int) rBuffCreateParams->maximum;
    rBuff->errorHandler   = rBuffCreateParams->errorHandler;

    if (semBInit (&rBuff->buffDesc.threshXSem, SEM_Q_PRIORITY, 
		  SEM_EMPTY) == ERROR)
        {
        objFree (rBuffClassId, (UINT8 *) rBuff);

        return (NULL);
        }

    if (semBInit (&rBuff->readBlk, SEM_Q_PRIORITY, SEM_FULL) == ERROR)
        {
        semTerminate (&rBuff->buffDesc.threshXSem);

        objFree (rBuffClassId, (UINT8 *) rBuff);

        return (NULL);
        }

    if (semBInit (&rBuff->bufferFull, SEM_Q_PRIORITY, SEM_EMPTY) == ERROR)
        {
        semTerminate (&rBuff->buffDesc.threshXSem);
        semTerminate (&rBuff->readBlk);

        objFree (rBuffClassId, (UINT8 *)rBuff);

        return (NULL);
        }

    if (semCInit (&rBuff->msgSem, SEM_Q_PRIORITY, 0) == ERROR)
        {
        semTerminate (&rBuff->bufferFull);
        semTerminate (&rBuff->buffDesc.threshXSem);
        semTerminate (&rBuff->readBlk);

        objFree (rBuffClassId, (UINT8 *)rBuff);

        return (NULL);
        }

    rBuff->rBuffMgrId = 0; /* ...so we can use rBuffDestroy safely */

    /*
     *  If things go wrong from here, use rBuffDestroy to throw back what
     *  we've caught.
     */

    /* allocate 'rBuffCreateParams->minimum' buffers */

    rBuff->info.currBuffs  =
    rBuff->info.emptyBuffs = 0;

    rBuff->buffWrite = NULL;

    for (count=0;count < rBuffCreateParams->minimum;count++)
        {

        /* First we need a buffer */

        newBuff =
            (RBUFF_PTR)
                memPartAlloc (rBuffCreateParams->sourcePartition,
                sizeof(RBUFF_BUFF_TYPE) + rBuffCreateParams->buffSize);

#ifdef RBUFF_DEBUG
        logMsg ("rBuff: adding buffer %p to ring\n",newBuff,0,0,0,0,0);
#endif

        /* newBuff will be returned as NULL if source partition is exhausted */

        /*  Don't need to lock ints around rBuffAdd as no-one knows about
         *  this rBuff.
         */

        if (newBuff == NULL || rBuffAdd (rBuff, newBuff) == ERROR)
            {
#ifdef RBUFF_DEBUG
            logMsg ("rBuff: abandoned creation\n",0,0,0,0,0,0);
#endif
            rBuffDestroy ((BUFFER_ID) rBuff);

            return (NULL);
            }
        }

    rBuff->rBuffMgrId = taskSpawn ("tRBuffMgr",
    	 rBuffMgrPriorityDefault,
         RBUFF_MGR_OPTIONS,
         2048,
         rBuffMgr,
         rBuff,
         0,0,0,0,0,0,0,0,0);

    if(rBuff->rBuffMgrId == NULL)
        {
#ifdef RBUFF_DEBUG
        logMsg ("rBuff: error creating rBuffMgr\n",0,0,0,0,0,0);
#endif
        rBuffDestroy ((BUFFER_ID) rBuff);

        return (NULL);
        }

    /* set control pointers */

    rBuff->nestLevel = 0;

    /* rBuff->buffWrite initialised by rBuffAdd */

    rBuff->buffRead  = rBuff->buffWrite;

    rBuff->dataWrite =
    rBuff->dataRead  = rBuff->buffWrite->dataStart;

    /* reset info */

    rBuff->info.maxBuffsActual   = rBuffCreateParams->minimum;

    rBuff->info.dataContent      =
    rBuff->info.writesSinceReset =
    rBuff->info.readsSinceReset  =
    rBuff->info.timesExtended    =
    rBuff->info.timesXThreshold  =
    rBuff->info.bytesWritten     =
    rBuff->info.bytesRead        =
    rBuff->info.bytesPeak        = 0;

    /* Reset msg passing mechanism */

    rBuff->msgOutstanding        = FALSE;

    rBuff->msgWriteIndex         =
    rBuff->msgReadIndex          = 0;

    /* now set up func ptrs allowing it to be called */

    rBuff->buffDesc.readReserveRtn = (FUNCPTR) rBuffReadReserve;
    rBuff->buffDesc.readCommitRtn  = (FUNCPTR) rBuffReadCommit;
    rBuff->buffDesc.writeRtn       = rBuffWrite;
    rBuff->buffDesc.flushRtn       = rBuffFlush;
    rBuff->buffDesc.threshold      = rBuffCreateParams->threshold;
    rBuff->buffDesc.nBytesRtn      = rBuffNBytes;

    /* made it! */

#ifdef RBUFF_DEBUG
    logMsg("Created rBuff with ID %p\n",rBuff,0,0,0,0,0);
#endif

    return ((BUFFER_ID) rBuff);
    }

int rBuffVerify (BUFFER_ID rBuff)
{
    if (OBJ_VERIFY (rBuff, rBuffClassId) == OK)	/* validate rBuff ID */
        {
	return (OK);
        }
        else
	{
	return (ERROR);
	}
}

/*******************************************************************************
*
* rBuffWrite - Write data to a ring of buffers
*
* This routine writes data to an extendable ring of buffers. If the existing
* structure is full, and the existing number of buffers is less than the
* specified minimum, then this function will attempt to add another buffer to
* the ring.
*
* This function may be called from interrupt level.
*
* Mutually exclusive access must be guaranteed by the caller.
*
* NOMANUAL
*/

UINT8 *rBuffWrite
    (
    BUFFER_ID buffId,
    UINT8 *dataSrc,
    UINT32 numOfBytes
    )
{
    BOOL startUploading;
    RBUFF_ID rBuff;			/* access this particular rBuff */
    UINT8 *returnPtr = (UINT8 *) ~NULL; /* Return a non-zero value if OK */

    /* Get access to the private members of this buffer's descriptor. */

    rBuff = (RBUFF_ID) buffId;

    /* Get out early if this request makes no sense. */

    if (numOfBytes > rBuff->info.buffSize)
        {
            return (NULL);
        }

    /* Critical Region Start */

    if (rBuff->info.dataContent >= rBuff->info.threshold)
        {
        startUploading = TRUE;
        }
    else
        {
        startUploading = FALSE;
        }

    if (rBuff->buffWrite->spaceAvail >= numOfBytes)
        {
        if (dataSrc)
            {
            /* We are writing the data, not just reserving the space */

            memcpy (rBuff->dataWrite, dataSrc, numOfBytes);
            }
        else
            {
            /* Record the start of the reserved area */

            returnPtr = rBuff->dataWrite;
            }

        /*
         *  If we are consuming one of our precious empty buffs,
         *  check we have enough in reserve.
         */

        if (rBuff->buffWrite->dataLen == 0) {
            --rBuff->info.emptyBuffs;
	     }

	if (rBuff->info.emptyBuffs < RBUFF_EMPTY_KEEP &&
            rBuff->info.currBuffs != rBuff->info.maxBuffs)
            {
            /* Schedule the rBuff Mgr to extend the buffer */

            RBUFF_SEND_MSG (RBUFF_MSG_ADD, MSG_PRI_URGENT, rBuff, 0);
	    }

        rBuff->buffWrite->dataLen    += numOfBytes;
        rBuff->dataWrite             += numOfBytes;
        rBuff->buffWrite->spaceAvail -= numOfBytes;

        /*
         *  It may be that the current buffer is full at this point,
         *  but don't do anything about it yet as the situation will
         *  be handled next time through. Also, it's possible that
         *  the current buffer may even have been emptied by then.
         */

        }
    else
        {
        UINT32 dataWritten1stBuff;
        UINT32 dataWritten2ndBuff;

        /*
         *  Data won't fit in this buffer (at least not entirely),
         *  is the next buffer available?
         */

        if (rBuff->buffWrite->next == rBuff->buffRead)
            {

            /*  We've caught the reader. This will only happen when
             *  we have actually filled the maximum allocation of
             *  buffers *or* the reserved buffer has been filled
             *  before the buffer has had the opportunity to
             *  be extended. We cannot extend the buffer at this point
             *  as we may be in a critical region, the design is such
             *  that if the buffer could be extended it should have
             *  been done by now (for that matter, the wrap-around
             *  should have also occurred).
             *
             *  If we have filled the reserved buffer without having
             *  the opportunity to extend the buffer then the buffer
             *  is configured badly and must be retuned.
             *
             *  Meanwhile, at this moment, options are:
             *
             *  1) If we have filled the entire buffer and cannot wrap-
             *     around then return ERROR.
             *  2) If we have filled the entire buffer but can wrap-
             *     around then do that.
             */           

            if (rBuff->info.options & RBUFF_WRAPAROUND)
                {

                /*
                 *  OK, perform an inline ring extension supplying NULL
                 *  as the new buffer forcing the ring to wrap-around.
                 *  This is a deterministic action.
                 */

                /* Interrupts already locked out */

                rBuffAdd(rBuff, NULL);
                }
            else
                {
                /* Oh dear, can't wrap-round */

                return (NULL);
                }
            }

        /*
         *  OK.
         *  In the case that this function is writing the data, it
         *  can be shared between this and next buffer.
         *  In the case that we are only reserving the space, we must
         *  return a pointer to contiguous space and therefore skip the
         *  remainder of the previous buffer.
         */

        if (dataSrc == NULL)
            {
            /* skip the remainder of the current buffer */

            rBuff->buffWrite->spaceAvail = 0;

            /* Move on to the next buffer */

            rBuff->buffWrite = rBuff->buffWrite->next;

            /* Record the start of the reserved area */

            returnPtr = rBuff->buffWrite->dataStart;

            /* Point dataWrite past the reserved area */

            rBuff->dataWrite = rBuff->buffWrite->dataStart;

            dataWritten1stBuff = 0;
            dataWritten2ndBuff = numOfBytes;
            }
        else
            {
            /* first make use of the space in this buffer */

            memcpy (rBuff->dataWrite,
                dataSrc,
                (dataWritten1stBuff = rBuff->buffWrite->spaceAvail));

            rBuff->buffWrite->dataLen    += dataWritten1stBuff;
            rBuff->buffWrite->spaceAvail = 0;

            /* Move on to the next buffer */

            dataWritten2ndBuff = numOfBytes - dataWritten1stBuff;

            rBuff->buffWrite = rBuff->buffWrite->next;
            rBuff->dataWrite = rBuff->buffWrite->dataStart;

            memcpy (rBuff->dataWrite,
                dataSrc + dataWritten1stBuff,
                dataWritten2ndBuff);
            }

        rBuff->buffWrite->dataLen    = dataWritten2ndBuff;
        rBuff->dataWrite            += dataWritten2ndBuff;
        rBuff->buffWrite->spaceAvail =
            rBuff->info.buffSize - dataWritten2ndBuff;

        if (--rBuff->info.emptyBuffs < RBUFF_EMPTY_KEEP &&
            rBuff->info.currBuffs != rBuff->info.maxBuffs)
            {
            /* Schedule the rBuff Mgr to extend the buffer */

            RBUFF_SEND_MSG (RBUFF_MSG_ADD, MSG_PRI_URGENT, rBuff, 0);
            }
        }

    rBuff->info.dataContent  += numOfBytes;

    /* update info */

    if(rBuff->info.dataContent > rBuff->info.bytesPeak)
        {
        rBuff->info.bytesPeak = rBuff->info.dataContent;
        }

    rBuff->info.bytesWritten += numOfBytes;

    rBuff->info.writesSinceReset++;

    if (!startUploading && rBuff->info.dataContent >= rBuff->info.threshold)
        {
        rBuff->info.timesXThreshold++;

        if (!(rBuff->info.options & RBUFF_UP_DEFERRED))
            {
            /*
             *  Signal for uploading to begin. Note that if we have just
             *  reserved space it is imperative that uploading does not
             *  actually begin until the data is in the buffer.
             */

#if (CPU_FAMILY == I80X86)
            portWorkQAdd1 ((FUNCPTR)semBGiveDefer,
                (int) &rBuff->buffDesc.threshXSem);
#else
            workQAdd1 ((FUNCPTR)semBGiveDefer,
                (int) &rBuff->buffDesc.threshXSem);
#endif
            }
        }


    /* Critical Region End */

    return(returnPtr);
}


/*******************************************************************************
*
* rBuffRead - Read data from a ring of buffers
*
* This routine reads data from an extendable ring of buffers.
*
* This function assumes mutually exclusive access is guaranteed by the caller.
*
* NOMANUAL
*/

INT32 rBuffRead
    (
    BUFFER_ID buffId,		/* generic buffer descriptor */
    UINT8    *dataDest,
    UINT32  numOfBytes
    )
    {
    UINT32 bytesToCopy, remaining = numOfBytes;
    RBUFF_ID rBuff;		/* access private members of this rBuff desc */

    /* Get access to the private members of this particular rBuff. */

    rBuff = (RBUFF_ID) buffId;

    /* Critical Region Start */

    while(remaining)
        {
        bytesToCopy = (rBuff->buffRead->dataLen < remaining ?
                        rBuff->buffRead->dataLen : remaining);

        if (bytesToCopy > 0)
            {
            memcpy (dataDest,rBuff->dataRead,bytesToCopy);

            remaining                -= bytesToCopy;
            dataDest                 += bytesToCopy;

            /* Update buffer */

            rBuff->buffRead->dataLen -= bytesToCopy;

            rBuff->dataRead          += bytesToCopy;
            rBuff->info.dataContent  -= bytesToCopy;
            }

        if (!rBuff->buffRead->dataLen)
            {
            /* This buffer is now empty */

            rBuffHandleEmpty (rBuff);
            }
        else
            {
            /* this buffer is not yet emptied */

            rBuff->dataRead += bytesToCopy;
            }
        }

    /* update info */

    rBuff->info.bytesRead += (numOfBytes - remaining);

    rBuff->info.readsSinceReset++;

    /* Critical Region End */

    return (numOfBytes - remaining);
    }


/*******************************************************************************
*
* rBuffReadReserve - Return the number of contiguous bytes available for
*                    reading.
*
* NOMANUAL
*/

UINT32 rBuffReadReserve
    (
    BUFFER_ID buffId,		/* generic identifier for this buffer */
    UINT8 **src
    )
    {
    RBUFF_ID rBuff;		/* specific identifier for this rBuff */
    INT32 bytesAvailable; 

    /* Get access to the private members of this particular rBuff. */

    rBuff = (RBUFF_ID) buffId;

    /* Generate and return the available contiguous bytes. */

    if ((bytesAvailable = rBuff->buffRead->dataLen))
        {
        *src = rBuff->dataRead;
        }
    else
        {
        *src = NULL;
        }
    return (bytesAvailable);
    }


/*******************************************************************************
*
* rBuffReadCommit - Move the read data ptr along a ring of buffers
*
* This routine moves the data ptr along a ring of buffers.
*
* It is equivalent to reading data from the buffers and should be used
* when the data has been copied elsewhere.
*
* This function assumes mutually exclusive access is guaranteed by the caller.
*
* NOMANUAL
*/

STATUS rBuffReadCommit
    (
    BUFFER_ID buffId,		/* generic identifier for this buffer */
    UINT32  numOfBytes
    )
    {
    RBUFF_ID rBuff;		/* specific identifier for this rBuff */

    /* Get access to the private members of this particular rBuff. */

    if (!numOfBytes)
    {
        return (OK);
    }

    rBuff = (RBUFF_ID) buffId;

    /* Critical Region Start */

    if (numOfBytes == rBuff->buffRead->dataLen)
        {
        rBuffHandleEmpty (rBuff);
        }
    else if (numOfBytes < rBuff->buffRead->dataLen)
        {
        rBuff->buffRead->dataLen -= numOfBytes;
        rBuff->dataRead += numOfBytes;
        }
    else
        {
        /* Moving ahead through multiple buffers */

        /* Not yet supported */

        return(ERROR);
        }

    rBuff->info.dataContent -= numOfBytes;

    /* update info */

    rBuff->info.bytesRead += numOfBytes;

    rBuff->info.readsSinceReset++;

    /* Critical Region End */

    return (OK);
    }


/*******************************************************************************
*
* rBuffFlush - Flush data from a ring of buffers
*
* This routine causes any data held in a buffer to be uploaded and is
* used to clear data that falls below the specified threshold.
*
* NOMANUAL
*/

STATUS rBuffFlush
    (
    BUFFER_ID buffId		/* generic identifier for this buffer */
    )
    {
    RBUFF_ID rBuff;		/* specific identifier for this rBuff */
    int result, originalThreshold;

    /* Get access to the private members of this particular rBuff. */

    rBuff = (RBUFF_ID) buffId;

    result = OK;

    if (!rBuff->info.dataContent)
        {
        return(OK);
        }

    /* Store the original threshold */

    originalThreshold = rBuff->info.threshold;

    rBuff->info.threshold = 0;

    while (rBuff->info.dataContent > 0 && result != ERROR)
        {
        if (semGive (&rBuff->buffDesc.threshXSem) == OK)
            {
            /* Cause a reschedule to allow uploader in */

            taskDelay(0);
            }
        else
            {
            result = ERROR;
            }
        }

    rBuff->info.threshold = originalThreshold;

    return(result);
}


/*******************************************************************************
*
* rBuffReset - reset an extendable ring of buffers
*
* This routine resets an already created extendable ring of buffers to its
* initial state. This loses any data held in the buffer. Any buffers held
* above the specified minimum number are deallocated.
*
* NOMANUAL
*/

STATUS rBuffReset
    (
    BUFFER_ID buffId		/* generic identifier for this buffer */
    )
    {
    RBUFF_ID rBuff;		/* specific identifier for this rBuff */
    RBUFF_LOCK (rBuff);

    /* Get access to the private members of this particular rBuff. */

    rBuff = (RBUFF_ID) buffId;

    /* Now reset the ring of buffers. */

    if(rBuff->info.currBuffs > rBuff->info.minBuffs)
        {
        UINT32 excessBuffs, count;

        /* drop the excess buffers */

        excessBuffs = rBuff->info.currBuffs - rBuff->info.minBuffs;

        for (count=0;count < excessBuffs;count++)
            {
            RBUFF_PTR tempPtr;

            tempPtr = rBuff->buffWrite->next; /* should not be NULL! */

            rBuffFree (rBuff,rBuff->buffWrite);

            rBuff->buffWrite = tempPtr;
            }
        }

    /* Make sure the 'read' buffer pointer points somewhere sensible */

    rBuff->buffRead = rBuff->buffWrite;

    /* and reset the byte ptrs */

    rBuff->dataRead         =
    rBuff->dataWrite        = rBuff->buffWrite->dataStart;

    rBuff->info.currBuffs        =
    rBuff->info.maxBuffsActual   = rBuff->info.minBuffs;

    /* now traverse the ring resetting the individual buffers */

    {
    RBUFF_PTR backHome = rBuff->buffWrite;
    RBUFF_PTR currBuff = rBuff->buffWrite;

    do  {
        currBuff->dataLen    = 0;
        currBuff->spaceAvail = rBuff->info.buffSize;

        currBuff = currBuff->next;
        }
    while (currBuff != backHome);

    }

    rBuff->nestLevel = 0;

    /* reset info */

    rBuff->info.emptyBuffs       = rBuff->info.minBuffs;
    rBuff->info.dataContent      =
    rBuff->info.writesSinceReset =
    rBuff->info.readsSinceReset  =
    rBuff->info.timesExtended    =
    rBuff->info.timesXThreshold  =
    rBuff->info.bytesWritten     =
    rBuff->info.bytesRead        = 0;

    semGive (&rBuff->bufferFull);

    RBUFF_UNLOCK (rBuff);

    return (OK);
}


/*******************************************************************************
*
* rBuffNBytes - Returns the total number of bytes held in a ring of buffers
*
* NOMANUAL
*/

INT32 rBuffNBytes
    (
    BUFFER_ID buffId		/* generic identifier for this buffer */
    )
    {
    RBUFF_ID rBuff;		/* specific identifier for this rBuff */

    /* Get access to the private members of this particular rBuff. */

    rBuff = (RBUFF_ID) buffId;

    return (rBuff->info.dataContent);
    }


/*******************************************************************************
*
* rBuffAdd - Add a new buffer to an extendable ring of buffers
*
* This routine adds a further buffer to an already created or partially created
* extendable ring of buffers unless the maximum number is already allocated.
*
* This function assumes mutually exclusive access is guaranteed by the caller.
*
* NOMANUAL
*/

STATUS rBuffAdd
    (
    RBUFF_ID rBuff,
    RBUFF_PTR newBuff
    )
    {
    if (rBuff->info.emptyBuffs >= RBUFF_EMPTY_KEEP &&
        rBuff->info.currBuffs  >= rBuff->info.minBuffs)
        {
        /*
         *  OK, the uploader got in so now we don't need to extend the
         *  buffer.
         */

#ifdef RBUFF_DEBUG
        logMsg ("rBuff: abandoned add\n",0,0,0,0,0,0);
#endif

        if (newBuff)
            {
            memPartFree (rBuff->info.srcPart, (UINT8 *) newBuff);
            }

        return (OK);
        }

    if (rBuff->info.currBuffs == rBuff->info.maxBuffs || newBuff == NULL)
        {

        /*
         *  We are at the maximum ring size already or the source
         *  partition is exhausted, there's still hope...
         *
         *  ...can we wrap-around?
         */

        if (!(rBuff->info.options & RBUFF_WRAPAROUND))
            {
            /* We aren't allowed to wrap around */

            return (ERROR);
            }
        else
            {
            RBUFF_PTR reclaim;

            /*  We can wrap around by comandeering the 'oldest'
             *  buffer in the ring.
             *
             *  It is possible (probable?) that this buffer is the
             *  one to be read from next so handle this.
             */

            reclaim = rBuff->buffWrite->next;

            if (rBuff->buffRead == reclaim)
                {
                /* Move the reader along */

                rBuff->buffRead = rBuff->buffRead->next;
                rBuff->dataRead = rBuff->buffRead->dataStart;
                }

            rBuff->info.dataContent  -= reclaim->dataLen;

            /* Reset the buffer */

            reclaim->dataLen    = 0;
            reclaim->spaceAvail = rBuff->info.buffSize;

            rBuff->info.emptyBuffs++;

            return (OK);
            }
        }

    newBuff->dataStart  = (UINT8 *) newBuff + sizeof(RBUFF_BUFF_TYPE);
    newBuff->dataEnd    = newBuff->dataStart + rBuff->info.buffSize;
    newBuff->spaceAvail = rBuff->info.buffSize;
    newBuff->dataLen    = 0;

    /* Now link the buffer into the ring */

    if (rBuff->buffWrite != NULL)
        {
        RBUFF_PTR tempPtr;

        tempPtr = rBuff->buffWrite->next;

        rBuff->buffWrite->next = newBuff;

        /* close the ring */

        newBuff->next = tempPtr;

        }
    else
        {

        /* ring is empty, must be creation time */

        rBuff->buffWrite =
        rBuff->buffRead  =
        newBuff->next    = newBuff;
        }

    /* Maintain statistics */

    if (++(rBuff->info.currBuffs) > rBuff->info.maxBuffsActual)
        {
        rBuff->info.maxBuffsActual++;
        }

    rBuff->info.timesExtended++;
    rBuff->info.emptyBuffs++;

    return (OK);
}


/*******************************************************************************
*
* rBuffFree - Free a specified buffer
*
* This function must only be called from task level.
* Semaphores are used to protect access at task-level. However, protection
* from access at interrupt level must be guaranteed by the caller.
*
* NOMANUAL
*/

STATUS rBuffFree
    (
    RBUFF_ID rBuff,
    RBUFF_PTR buffToFree
    )
    {
    RBUFF_PTR ptrToBuff, backHome, ptrToPreviousBuff;

    if (buffToFree == NULL)
        {
        return(ERROR);
        }

    /* find the pointer to the buffer to be freed */

    ptrToBuff         = rBuff->buffWrite->next;
    backHome          = rBuff->buffWrite;
    ptrToPreviousBuff = rBuff->buffWrite;

    while ((ptrToBuff != buffToFree) && (ptrToBuff != backHome))
        {

        ptrToPreviousBuff = ptrToBuff;
        ptrToBuff         = buffToFree;

        }

    if (ptrToBuff == backHome)
        {
        /* an error has occured, possibly invalid buff ptr passed */

        return (ERROR);
        }

    /* OK, de-link the buffer to be freed */

    ptrToPreviousBuff->next = buffToFree->next;

    /* Schedule the rBuff Mgr to actually free the buffer */

    RBUFF_SEND_MSG (RBUFF_MSG_FREE, MSG_PRI_NORMAL, rBuff, buffToFree);

    /* Maintain statistics */

    rBuff->info.emptyBuffs--;
    rBuff->info.currBuffs--;

    return (OK);
    }


/*******************************************************************************
*
* rBuffDestroy - Destroy an extendable ring of buffers
*
* This routine destroys an already created or partially created extendable ring
* of buffers. This loses any data held in the buffer. All resources held are
* returned to the system.
*
* NOMANUAL
*/

STATUS rBuffDestroy
    (
    BUFFER_ID buffId		/* generic identifier for this buffer */
    )
    {
    RBUFF_ID rBuff;		/* specific identifier for this rBuff */
    RBUFF_PTR tempPtr;

    /* Get access to the private members of this particular rBuff. */

    rBuff = (RBUFF_ID) buffId;

    RBUFF_LOCK (rBuff);

    if(rBuff->rBuffMgrId != NULL)
        {
        taskDelete(rBuff->rBuffMgrId);
        }

#if 0
    if (OBJ_VERIFY (rBuff, rBuffClassId) != OK)	/* validate rBuff ID */
	{
	((OBJ_ID)rBuff)->pObjClass, rBuffClassId);
	RBUFF_UNLOCK (rBuff);

	return (ERROR);
	}
#endif

    objCoreTerminate (&rBuff->buffDesc.objCore);   /* invalidate this object */

    /* Break the ring and traverse the list, freeing off   */
    /* the buffers. buffWrite is used as the start point   */
    /* this works if the ring creation is abandoned early. */

    if (rBuff->buffWrite != NULL)
        {

        /* Break the ring */

        tempPtr = rBuff->buffWrite->next;
        rBuff->buffWrite->next = NULL;
        rBuff->buffWrite = tempPtr;

        /* traverse and free */

        while(rBuff->buffWrite != NULL)
            {
            tempPtr = rBuff->buffWrite->next;

            memPartFree (rBuff->info.srcPart,(UINT8 *)rBuff->buffWrite);

            rBuff->buffWrite = tempPtr;
            }
        }
    else
        {
        /* ring hasn't been created so don't worry about it */
        }

    /* now free off the control structure */

    semTerminate (&rBuff->buffDesc.threshXSem);
    semTerminate (&rBuff->readBlk);
    semTerminate (&rBuff->bufferFull);

    objFree (rBuffClassId, (UINT8 *) rBuff);

    /* we're done */

    return (OK);
    }


/*******************************************************************************
*
* rBuffSetFd - Set a new fd as the data upload destination
*
*
* NOMANUAL
*/

STATUS rBuffSetFd
    (
    BUFFER_ID buffId,		/* generic identifier for this buffer */
    int fd
    )
    {
    RBUFF_ID rBuff;		/* specific identifier for this rBuff */
    RBUFF_LOCK (rBuff);

    /* Get access to the private members of this particular rBuff. */

    rBuff = (RBUFF_ID) buffId;

    rBuff->fd = fd;

    RBUFF_UNLOCK (rBuff);

    return (OK);
    }


/*******************************************************************************
*
* rBuffHandleEmpty - Handles a newly empty buffer by deciding whether to free
*                    it off.
*
* This function assumes mutually exclusive access is guaranteed by the caller.
*
* NOMANUAL
*/

LOCAL STATUS rBuffHandleEmpty
    (
    RBUFF_ID rBuff
    )
    {
    BOOL      moveAlong = TRUE;
    RBUFF_PTR buffToFree;

    /*
     *  Determine if we are to move around the ring and if the
     *  newly empty buffer is to be freed, and then do the actions.
     *  This avoids having to fumble with ptrs as we move around
     *  from a buffer that has been freed.
     */

    if (rBuff->buffRead == rBuff->buffWrite)
        {

        /*
         *  We've caught the writer so might as well reset this
         *  buffer and make efficient use of it.
         */

        rBuff->dataRead  =
            rBuff->dataWrite = rBuff->buffWrite->dataStart;

        rBuff->buffRead->spaceAvail = rBuff->info.buffSize;
        rBuff->buffRead->dataLen     = 0;

        /* In this case, we don't move along to the next buffer */

        moveAlong = FALSE;
        }

    if (++(rBuff->info.emptyBuffs) > RBUFF_EMPTY_KEEP &&
           rBuff->info.currBuffs > rBuff->info.minBuffs)
        {
        /* We already have enough empty buffs, so free one off */

        if (rBuff->buffRead != rBuff->buffWrite)
            {
            buffToFree = rBuff->buffRead;
            }
            else
            {
            /*
             *  As the reader and writer are in the same buffer
             *  we know it's safe to free off the next buffer.
             */

            buffToFree = rBuff->buffRead->next;
            }
        }
    else
        {
        /* we've just emptied one of the 'core' buffers */

        rBuff->buffRead->spaceAvail = rBuff->info.buffSize;
        rBuff->buffRead->dataLen    = 0;

        /* Don't free */

        buffToFree = NULL;
        }

    if (moveAlong)
        {
        /* Move round the ring */

        rBuff->buffRead = rBuff->buffRead->next;
        rBuff->dataRead = rBuff->buffRead->dataStart;
        }
    else
        {
        /* reset flag for next time */

        moveAlong = TRUE;
        }

    if (/* there's a */ buffToFree)
        {
#ifdef RBUFF_DEBUG
        logMsg ("rBuff: Freeing buffer %p\n",
            buffToFree,0,0,0,0,0);
#endif
        rBuffFree (rBuff, buffToFree);
        }

    return (OK);
    }


/*******************************************************************************
*
* rBuffMgr - The function is spawned as a task to manage rBuff extension.
*
* It waits for a msg which carries the ID of a buffer which has had a change
* of the number of empty buffers held. If there are less the requested to be
* reserved than an attempt will be made to make more available. If there are
* more empty buffers than required than some will be freed.
*
* For each msg received, the task will achieve the required number of free
* buffers. This may result in redundant msgs being received.
*
* NOMANUAL
*/

int rBuffMgr(RBUFF_TYPE *rBuffId)
    {
    UINT32     busy;
    int        key;
    RBUFF_PTR  newBuff;

#ifdef RBUFF_DEBUG
        logMsg ("rBuff: Mgr for rBuff %p created\n",rBuffId,0,0,0,0,0);
#endif

    for(;;)
        {

	semTake (&(rBuffId->msgSem), WAIT_FOREVER);

#ifdef RBUFF_DEBUG
        logMsg ("rBuff: Msg type 0x%x received from rBuff %p\n",
            rBuffId->msg[rBuffId->msgReadIndex][0],rBuffId,0,0,0,0);
#endif

        busy = TRUE;

        /* Determine what needs to be done */

        switch (rBuffId->msg[rBuffId->msgReadIndex][0])
            {
            case RBUFF_MSG_ADD :
                {

                /*
                 *  This message may be generated one or more times by
                 *  event logging, and the condition that caused the msg
                 *  may no longer be TRUE, so just take this as a prompt
                 *  to see if the ring needs extending.
                 */

                /* Make sure that we don't request extension recursively */

                rBuffId->nestLevel++;

                while
                    (rBuffId->info.emptyBuffs < RBUFF_EMPTY_KEEP &&
                    (rBuffId->info.currBuffs  < rBuffId->info.maxBuffs ||
                        rBuffId->info.options & RBUFF_WRAPAROUND) &&
                    busy)
                    {

                    /* Looks like we are going to need a buffer */

                    newBuff = (RBUFF_PTR)
                        memPartAlloc (rBuffId->info.srcPart,
                            sizeof(RBUFF_BUFF_TYPE) + rBuffId->info.buffSize);

                    /* of course, newBuff may be NULL... */

                    key = intLock();

#ifdef RBUFF_DEBUG
                    logMsg ("rBuff: adding buffer %p\n",newBuff,0,0,0,0,0);
#endif

		    if (rBuffAdd (rBuffId, newBuff) == ERROR)
                        {
                        /* ring cannot be extended so give up trying */

                        busy = FALSE;
                        }

                    intUnlock (key);
                    }
                }

                rBuffId->nestLevel--;

            break;

            case RBUFF_MSG_FREE :
                {
                /* This message carries a ptr to the buffer to be freed */

#ifdef RBUFF_DEBUG
                logMsg ("rBuff: freeing buffer %p\n",rBuffId->msg[rBuffId->msgReadIndex][1],0,0,0,0,0);
#endif

                memPartFree (rBuffId->info.srcPart,
                    (UINT8 *) rBuffId->msg[rBuffId->msgReadIndex][1]);
                }
            break;
            }

         if(++rBuffId->msgReadIndex > RBUFF_MAX_MSGS)
             {
             rBuffId->msgReadIndex = 0;
             }

         rBuffId->msgOutstanding = FALSE;

         }

   }

/*******************************************************************************
*
* rBuffMgrPrioritySet - set the priority of ring Buffer Manager task
*
* This routine sets the priority of the rBuffMgr task to the priority given.
* Two parameters are expected:
* rBuffId : can be NULL or a valid ring Buffer Id.
*           If NULL, the default priority of any future ring buffer manager
*           tasks spawned will be set to the priority given. The priority of any
*    	    existing ring buffer manager tasks running will be unaffected.
*
*           If a valid ring buffer id is given, the priority of its associated
*           ring buffer manager will be changed to that given. In addition, the
*           priority of any future ring buffer manager tasks spawned will be
*           changed to the priority given. If the priority of the associated
*           manager cannot be set for any reason, the routine returns ERROR and
*           the default remains unchanged.
* 
* priority :The desired priority of the ring buffer manager task (0-255).
*
* NOMANUAL
*
* RETURNS: OK, or ERROR if the priority could not be set.
*/

STATUS rBuffMgrPrioritySet
    (
    RBUFF_ID rBuffId,		/* rBuff identifier */
    int	     priority           /* new priority */
    )
    {
    if (rBuffId != NULL)
 	if (taskPrioritySet (rBuffId->rBuffMgrId, priority) != OK)
	    return (ERROR);

    rBuffMgrPriorityDefault = priority;
    return (OK);
    }

/*******************************************************************************
*
* rBuffUpload - Copy data that is added to the buffer to the specified 'fd'
*
* NOMANUAL
*/

STATUS rBuffUpload
    (
    BUFFER_ID buffId,		/* generic identifier for this buffer */
    int fd
    )
    {
    RBUFF_ID rBuff;		/* specific identifier for this rBuff */
    UINT8 *src;
    int lockKey;
    STATUS result = OK;

    /* Get access to the private members of this particular rBuff. */

    rBuff = (RBUFF_ID) buffId;

    rBuff->fd = fd;

    while(result == OK)
        {

        /* Wait for the dinner gong */

        logMsg ("Waiting for data\n",0,0,0,0,0,0);

        semTake (&rBuff->buffDesc.threshXSem,WAIT_FOREVER);

        logMsg("Uploading...\n",0,0,0,0,0,0);

        /* tuck in... */

        while (result == OK && rBuff->info.dataContent >
              (rBuff->info.threshold / 4))
            {
            UINT32 bytesToWrite = rBuffReadReserve(buffId, &src);
            UINT32 bytesWritten;

            if (bytesToWrite > 4096)
                {
                    bytesToWrite = 4096;
                }

#ifdef RBUFF_DEBUG

            logMsg ("%#x bytes from %p (dataContent=%#x)\n",
                bytesToWrite,
                (int) src,
                rBuff->info.dataContent,0,0,0);
#endif


            /* move the data */

#if 0
{
UINT8 buffer[100];

bcopy(src,buffer,bytesToWrite);
buffer[bytesToWrite] = '\0';

printf("%s\n",buffer);
}
#endif

            if ((bytesWritten =
                write (fd, src, bytesToWrite)) == -1)
                {
                logMsg ("Upload error!\n",0,0,0,0,0,0);

                if (rBuff->errorHandler)
                    {

                    /* Dial 999 / 911 */

                    (*rBuff->errorHandler)();
                    }

                result = ERROR;
                }
            else
                {
                /* move the buffer pointers along */

                lockKey = intLock();

                result = rBuffReadCommit (buffId,bytesWritten);

                intUnlock (lockKey);

                taskDelay (0);
                }
            }
        }

    return (result);
    }

#endif /* GENERIC_RBUFF */



#if (CPU_FAMILY == I80X86)

/*******************************************************************************
*
* portWorkQAdd1 - add work with one parameter to the wind work queue
*
* When the kernel is interrupted, new kernel work must be queued to an internal
* work queue.  The work queue is emptied by whatever task or interrupt service
* routine that entered the kernel first.  The work is emptied as the last
* code of reschedule().
*
* INTERNAL
* The work queue is single reader, multiple writer, so we should have to lock
* out interrupts while the writer is copying into the ring, but because the
* reader can never interrupt a writer, interrupts need only be locked while
* we advance the write queue pointer.
*
* RETURNS: OK
*
* SEE ALSO: reschedule()
*
* NOMANUAL
*/

void portWorkQAdd1
    (
    FUNCPTR func,       /* function to invoke */
    int arg1            /* parameter one to function */
    )
    {
    int level = intLock ();             /* LOCK INTERRUPTS */
    FAST JOB *pJob = (JOB *) &pJobPool [workQWriteIx];

    workQWriteIx += 4;                  /* advance write index */

    if (workQWriteIx == workQReadIx)
        workQPanic ();                  /* leave interrupts locked */

    intUnlock (level);                  /* UNLOCK INTERRUPTS */

    workQIsEmpty = FALSE;               /* we put something in it */

    pJob->function = func;              /* fill in function */
    pJob->arg1 = arg1;                  /* fill in argument */
    }

#endif /* CPU_FAMILY == I80X86 */
