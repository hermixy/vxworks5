/* smObjShow.c - shared memory objects show routines (VxMP Option) */

/* Copyright 1984-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01h,06may02,mas  cache flush and volatile fix (SPR 68334); bridge flush fix
		 (SPR 68844)
01g,24oct01,mas  doc update (SPR 71149)
01f,14feb93,jdi  documentation cleanup for 5.1.
01e,29jan93,pme  added little endian support.
01d,08dec92,jdi  documentation cleanup.
01c,02oct92,pme  added SPARC support. documentation cleanup.
01b,29sep92,pme  added anchor local address in smObjShow.
01a,19jul92,pme  written.
*/

/*
DESCRIPTION
This library provides routines to show shared memory object
statistics, such as the current number of shared tasks, semaphores,
message queues, etc.

CONFIGURATION
The routines in this library are included by default if the component
INCLUDE_SM_OBJ is included.

AVAILABILITY
This module is distributed as a component of the unbundled shared memory
objects support option, VxMP.

INCLUDE FILES: smObjLib.h

SEE ALSO: smObjLib,
\tb VxWorks Programmer's Guide: Shared Memory Objects
*/

/* includes */

#include "vxWorks.h"
#include "cacheLib.h"
#include "errno.h"
#include "intLib.h"
#include "logLib.h"
#include "qFifoGLib.h"
#include "smDllLib.h"
#include "smLib.h"
#include "smUtilLib.h"
#include "smNameLib.h"
#include "semLib.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "private/smObjLibP.h"
#include "private/msgQSmLibP.h"
#include "private/semSmLibP.h"
#include "private/smMemLibP.h"
#include "private/memPartLibP.h"

/* smObj Information Structure */

typedef struct sm_obj_info      /* SM_OBJ_INFO */
    {
    int numCpuAttached;         /* number of attached CPU */

    int maxSems;                /* maximum number of semaphores */
    int maxMsgQueues;           /* maximum number of messages queues */
    int maxTasks;		/* maximum number of tasks */
    int maxMemParts;            /* max # of shared memory partitions */
    int maxNames;               /* max # of name of shared objects */

    int curNumSemB;		/* current number of binary semaphores */
    int curNumSemC;		/* current number of counting semaphores */
    int curNumMsgQ;		/* current number of messages queues */
    int curNumTask;		/* current number of tasks */
    int curNumPart;             /* current number of shared partitions */
    int curNumName;             /* current number of names */

    int curMaxLockTries;	/* current number of maximum number of tries */
    } SM_OBJ_INFO;

/******************************************************************************
*
* smObjShowInit - initialize shared memory objects show routine
*
* This routine links the shared memory objects show routine into the VxWorks
* system.  These routines are included automatically by defining
* \%INCLUDE_SHOW_ROUTINES in configAll.h.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void smObjShowInit (void)
    {
    }

/******************************************************************************
*
* smObjShow - display the current status of shared memory objects (VxMP Option)
*
* This routine displays useful information about the current status of
* shared memory objects facilities.
*
* WARNING
* The information returned by this routine is not static and may be obsolete
* by the time it is examined.  This information is generally used for
* debugging purposes only.
*
* EXAMPLE:
* \cs
*     -> smObjShow
*     Shared Mem Anchor Local Addr: 0x600.
*     Shared Mem Hdr Local Addr:    0xb1514.
*     Attached CPU :                5
*     Max Tries to Take Lock:       1
* 
*     Shared Object Type    Current    Maximum  Available
*     -------------------- ---------- --------- ----------
*     Tasks                         1        20         19
*     Binary Semaphores             8        30         20
*     Counting Semaphores           2        30         20
*     Messages Queues               3        10          7
*     Memory Partitions             1         4          3
*     Names in Database            16       100         84
* \ce
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled shared memory
* objects support option, VxMP.
* 
* RETURNS: OK, or ERROR if no shared memory objects are initialized.
*
* ERRNO:
*  S_smObjLib_NOT_INITIALIZED
*
* SEE ALSO: smObjLib
*/

STATUS smObjShow (void) 
    {
    SM_INFO            smInfo;		/* shared mem info */
    SM_FIX_BLK_PART_ID pTcbPartId;	/* pointer to shared tcb partition */
    SM_OBJ_MEM_HDR     localSmObjHdr; 	/* local copy of smObj Header */
    int		       numTask;		/* number of tasks using smObj */

    pTcbPartId = (SM_FIX_BLK_PART_ID) GLOB_TO_LOC_ADRS (smTcbPartId);

    /* check if smObj facility is initialized */

    if (pSmObjHdr == NULL)
	{
	errno = S_smObjLib_NOT_INITIALIZED;
	return (ERROR);
	}

    /* get number of attached CPU using smInfoGet */

    if (smInfoGet (&smObjDesc.smDesc, &smInfo) != OK)
        {
	return (ERROR);
        }

    /* copy the shared memory header all at once to reduce bus traffic */

    bcopy ((char *) pSmObjHdr, (char *) &localSmObjHdr, 
	   sizeof (SM_OBJ_MEM_HDR));

    /* 
     * Get current number of task using shared memory objects
     * by the number of allocated blocks in the shared TCB partition.
     */
 
    numTask = ntohl (pTcbPartId->curBlocksAllocated);

    /* now print important data */

    printf ("\n");
    printf ("%-29s: %#-10x\n", "Shared Mem Anchor Local Addr", 
	    localToGlobalOffset);
    printf ("%-29s: %#-10x\n", "Shared Mem Hdr Local Addr", 
	    (unsigned int) pSmObjHdr);
    printf ("%-29s: %-10d\n", "Attached CPU", smInfo.attachedCpus);
    printf ("%-29s: %-10d\n", "Max Tries to Take Lock", smCurMaxTries);  

    printf ("\n");
    printf ("Shared Object Type     Current   Maximum  Available\n");
    printf ("-------------------- ---------- --------- ----------\n");
    printf ("Tasks                      %4d      %4d       %4d\n",
	    numTask, ntohl (localSmObjHdr.maxTasks), 
	    ntohl (localSmObjHdr.maxTasks) - numTask);

    printf ("Binary Semaphores          %4d      %4d       %4d\n",
	    ntohl (localSmObjHdr.curNumSemB), ntohl (localSmObjHdr.maxSems), 
	    ntohl (localSmObjHdr.maxSems) - ntohl (localSmObjHdr.curNumSemB) - 
	    ntohl (localSmObjHdr.curNumSemC));

    printf ("Counting Semaphores        %4d      %4d       %4d\n",
	    ntohl (localSmObjHdr.curNumSemC), ntohl (localSmObjHdr.maxSems), 
	    ntohl (localSmObjHdr.maxSems) - ntohl (localSmObjHdr.curNumSemC) - 
	    ntohl (localSmObjHdr.curNumSemB));

    printf ("Messages Queues            %4d      %4d       %4d\n",
	    ntohl (localSmObjHdr.curNumMsgQ), 
	    ntohl (localSmObjHdr.maxMsgQueues), 
	    ntohl (localSmObjHdr.maxMsgQueues)-ntohl(localSmObjHdr.curNumMsgQ));

    printf ("Memory Partitions          %4d      %4d       %4d\n",
	    ntohl (localSmObjHdr.curNumPart), ntohl (localSmObjHdr.maxMemParts),
	    ntohl (localSmObjHdr.maxMemParts)-ntohl (localSmObjHdr.curNumPart));

    printf ("Names in Database          %4d      %4d       %4d\n\n",
	    ntohl (localSmObjHdr.curNumName), ntohl (localSmObjHdr.maxNames),
	    ntohl (localSmObjHdr.maxNames) - ntohl (localSmObjHdr.curNumName));
    
    return (OK);
    }

/******************************************************************************
*
* smObjObjShow - generic shared memory objects object show routine
*
* This routine calls the appropriate show routine for each type 
* of shared memory object.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void smObjObjShow 
    (
    int smObjId,	/* shared object identifier to use */
    int level		/* level of information displayed */
    )
    {
    SM_SEM_ID volatile pseudoSmObjId;	/* hack to get a compatible type */ 
    void *             tmp;

    pseudoSmObjId = (SM_SEM_ID volatile) SM_OBJ_ID_TO_ADRS (smObjId);

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    tmp = (void *) pseudoSmObjId->verify;       /* PCI bridge bug [SPR 68844]*/

    if (SM_OBJ_VERIFY (pseudoSmObjId) != OK)
	{
	printf ("Object not found.\n");
	return;
	}
    
    switch (ntohl (pseudoSmObjId->objType))
	{
	case SEM_TYPE_SM_BINARY :
	case SEM_TYPE_SM_COUNTING :
	    {
	    (*semSmShowRtn) ((SM_SEM_ID) SM_OBJ_ID_TO_ADRS (smObjId), level);
	    break;
	    }

	case MSG_Q_TYPE_SM :
	    {
	    (*msgQSmShowRtn) ((SM_MSG_Q_ID) SM_OBJ_ID_TO_ADRS (smObjId),level);
	    break;
	    }

	case MEM_PART_TYPE_SM_STD :
	    {
	    (*smMemPartShowRtn) ((SM_PART_ID) SM_OBJ_ID_TO_ADRS (smObjId), 
				  level);
	    break;
	    }

	default :
	    break;
	}
    }

