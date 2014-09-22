/* smUtilLib.c - VxWorks shared memory utility library */

/* Copyright 1984-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01l,03may02,mas  cache flush and volatile fix (SPR 68334); bridge flush fix
		 (SPR 68844)
01k,24oct01,mas  fixed diab warnings (SPR 71120); doc update (SPR 71149)
01j,23nov99,tm    smUtilIntConnect() now returns OK for USER_1/2 (SPR 29548)
01i,17mar99,dat	  added SM_INT_USER_1 and SM_INT_USER_2, spr 25804
01h,08feb96,dat   fixed smUtilSoftTas, SPR #5888, fixed some
		  compiler warnings.
01g,12oct94,ism   added philm's workaround for SPR#2578
01f,12feb93,jdi   fixed parameter comments in smUtilIntGen() to keep mangen
		  happy -- in case we ever publish this one.
01e,23jun92,elh   cleanup, documentation.
01d,02jun92,elh   the tree shuffle
		  -changed includes to have absolute path from h/
01c,27may92,elh	  fixed bug in smUtilIntRoutine && smUtilPollTask.
01b,15may92,pme	  added smUtilIntConnect, smUtilIntRoutine, smUtilPollTask.
01a,07feb92,elh	  created.
*/

/*
DESCRIPTION

This module is the shared memory utility library.  It contains the 
routines needed of VxWorks by the shared memory libraries (smPktLib,
smObjLib and smLib).  

To fully support this library, a BSP and the associated architecture must have
support for the following system calls: vxMemProbe(), sysClkRateGet(),
sysProcNumGet(), sysBusTas(), sysBusIntGen(), sysBusToLocalAdrs(),
sysMailboxConnect(), and sysMailboxEnable().

INTERNAL
SunOS is no longer supported.

Refer to smLib for more information about the layering of shared 
memory modules.

SEE ALSO: if_sm, smPktLib, smLib, smObjLib
*/

/* includes */

#include "vxWorks.h"
#include "smLib.h"
#include "smUtilLib.h"
#include "netinet/in.h"
#include "iv.h"
#include "vxLib.h"
#include "taskLib.h"
#include "cacheLib.h"
#include "logLib.h"
#include "intLib.h"
#include "stdio.h"
#include "stdlib.h"
#include "sysLib.h"

/* defines */

#define DEFAULT_TAS_CHECKS	10		/* rechecks for soft tas */
#define TAS_CONST		0x80

/* globals */

int 		smUtilVerbose   = FALSE;	/* verbose mode */
int 		smUtilTasChecks = DEFAULT_TAS_CHECKS;
VOIDFUNCPTR	smUtilTasClearRtn = NULL;
FUNCPTR		smUtilUser1Rtn = NULL; /* returns STATUS */
FUNCPTR		smUtilUser2Rtn = NULL; /* returns STATUS */

				/* number of times to recheck soft tas */

/* list of shared memory event handling routines */

SM_ROUTINE 	smUtilNetRoutine = { NULL, 0};
SM_ROUTINE 	smUtilObjRoutine = { NULL, 0};

/* poll task parameters */

int     	smUtilPollTaskStackSize   = 0x2000;       /* stack size */
int     	smUtilPollTaskOptions     = 0;            /* options */
int     	smUtilPollTaskPriority    = 254;          /* priority */

/* locals */

LOCAL int	smUtilTasValue = 0;		/* special soft tas value */
LOCAL int  	smUtilPollTaskID;               /* polling task id */

/* forwards */

LOCAL	void 	smUtilPollTask (void);

/*****************************************************************************
*
* smUtilSoftTas - software emulation of test-and-set function
*
* This routine emulates an inidivisable hardware test-and-set function.
* The lock specified by <lockLocalAdrs> is read to confirm that it is
* initially zero.  The lock is then set to a special value used only by
* this cpu, and is repeatedly re-read to check if any other processor
* has attempted to obtain access to the lock.  If no such conflict is
* detected, this routine returns TRUE.
*
* The number of checks made after setting the lock value may be changed
* by setting the global variable, smUtilTasChecks.  The default number
* is 10.
*
* All the above is performed with interrupts disabled on the local processor.
*
* RETURNS: TRUE if value successfully set (was previously not set), or
* FALSE if value already set or another CPU stole lock.
*/

BOOL smUtilSoftTas
    (
    volatile int * lockLocalAdrs	/* local address of lock variable */
    )
    {
    int	value;			/* value to place in lock variable */
    int	nChecks;		/* number of times to re-check lock */
    int	count;			/* running count of re-checks */
    int	oldLvl;			/* previous interrupt level */
    int tmp;                    /* temp storage */

    if (smUtilTasValue == 0)
        {
	smUtilTasValue =  htonl ((TAS_CONST + sysProcNumGet())<< 24);
        }

    value   = smUtilTasValue;			/* special value to write */
    nChecks = smUtilTasChecks;			/* number of checks to do */

    /* Lock out interrupts */

    oldLvl = intLock ();

    /* Test that lock variable is initially empty */

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    tmp = *lockLocalAdrs;                       /* PCI bridge bug [SPR 68844]*/

    if (*lockLocalAdrs != 0)
	{
	intUnlock (oldLvl);
	return (FALSE);				/* someone else has lock */
	}

    /* Set lock value */

    *lockLocalAdrs = value;

    /* preliminary read compensates for PCI bridge failure (SPR 68844) */

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    count = *lockLocalAdrs;

    /* Check that no one else is trying to take lock */

    for (count = 0;  count < nChecks;  count++)
	{
	if (*lockLocalAdrs != value)
	    {
	    intUnlock (oldLvl);
	    return (FALSE);			/* someone else stole lock */
	    }
	}

    intUnlock (oldLvl);
    return (TRUE);				/* exclusive access obtained */
    }

/*****************************************************************************
*
* smUtilDelay - delay 
*
* This routine delays task execution by <ticks> number of system clock ticks.
*
* RETURNS: OK if successful, otherwise ERROR. 
*/

STATUS smUtilDelay
    (
    char *	pAddr,			/* address [not used] */
    int 	ticks           	/* number of ticks to delay */
    )
    {
    /* appease the Diab gods (SPR 71120) */

    if (pAddr == NULL)
        ;

    return (taskDelay (ticks));
    }

/*****************************************************************************
*
* smUtilMemProbe - probe memory
*
* This routine safely probes a location in memory.  
*
* RETURNS:
* OK if the probe is successful, or ERROR if the probe caused a bus error or
* an address misalignment.
*/

STATUS smUtilMemProbe
    (
    char *	pAddr,    /* address to be probed          */
    int 	op, 	  /* VX_READ or VX_WRITE           */
    int 	size,     /* 1, 2, or 4                    */
    char *	pVal      /* where to return value,        */
                          /* or ptr to value to be written */
    )
    {
    if (smUtilVerbose)
        {
	printf ("smUtilMemProbe:op[%s] addr:[0x%x] size:[%d]\n", 
		(op == VX_READ) ? "read" : "write", (UINT) pAddr, size);
        }

    return (vxMemProbe (pAddr, op, size, pVal));
    }

/*****************************************************************************
*
* smUtilRateGet - get the system clock rate
*
* This routine calls the BSP-specific routine sysClkRateGet() which returns
* the local system clock rate in ticks per second.
*
* RETURNS: The number of ticks per second of the system clock.
*/

int smUtilRateGet (void)
    {
    return (sysClkRateGet ());
    }

/*****************************************************************************
*
* smUtilProcNumGet - get the processor number
*
* This routine calls the BSP-specific routine sysProcNumGet() which returns
* the number assigned to the local CPU.
*
* RETURNS: The processor number for this CPU board.
*/

int smUtilProcNumGet (void)
    {
    return (sysProcNumGet ());
    }

/*****************************************************************************
*
* smUtilTas - shared memory hardware test-and-set function
*
* This routine calls the BSP-specific TAS routine sysBusTas() which performs
* an atomic test-and-set (TAS) on the specified address <adrs>.
*
* RETURNS: TRUE if the value had not been set but is now, or
* FALSE if the value was set already.
*/

BOOL smUtilTas
    (
    char *	adrs			/* address to be tested and set */
    )
    {
    return (sysBusTas (adrs));
    }

/*****************************************************************************
*
* smUtilTasClear - shared memory clear hardware test-and-set function
*
* This routine calls a TAS clear routine through the hook 'smUtilTasClearRtn'
* if it is not NULL, else, it clears the address <adrs> itself.
*
* INTERNAL
* The function pointer smUtilTasClearRtn is currently set by the BSPs
* if a test-and-set clear routine is required of the board support package.
* Ultimately, the desired solution is that each BSP will provide a
* sysBusTasClear routine that can be called directly from smUtilTasClear().
*
* RETURNS: N/A
*/

void smUtilTasClear
    (
    char *	adrs			/* address to be tested and set */
    )
    {
    int         tmp;            /* temp storage */

    if (smUtilTasClearRtn != NULL)
        {
	(*smUtilTasClearRtn) (adrs);
        }
    else
        {
	*adrs = 0;
        CACHE_PIPE_FLUSH ();                    /* CACHE FLUSH   [SPR 68334] */
        tmp = *adrs;                            /* BRIDGE FLUSH  [SPR 68334] */
        }
    }

/*****************************************************************************
*
* smUtilIntGen - interrupt the destination cpu.
*
* smUtilIntGen() gets called by the shared memory libraries to interrupt a
* CPU to notify it of an incoming packet or an event.  smUtilIntGen() provides
* the following methods for interrupting the destination CPU:
* mail box interrupts (SM_INT_MAILBOX), bus interrupts (SM_INT_BUS), user
* defined procedure 1 (SM_INT_USER1), user defined procedure 2 (SM_INT_USER2),
* and none (SM_INT_NONE).  Refer to smLib.h for the actual values.
* 
* RETURNS: OK if successful, otherwise ERROR.
*/

STATUS smUtilIntGen
    (
    SM_CPU_DESC *	pCpuDesc,	/* local addr of destination */
					/* cpu descriptor */
    int			cpuNum 		/* destination cpu number */
    )
    {
    volatile int	bogus;		/* bogus space */
    char * 		pMailbox = NULL;/* mailbox */
    int			intType;	/* interrupt type */
    int			intArg1;
    int			intArg2;
    int			intArg3;

    /* appease the Diab gods (SPR 71120) */

    if (cpuNum == 0)
        ;

    intType = ntohl (pCpuDesc->intType);	/* byte swap */

    /* polling needs no arguments */

    if (intType == SM_INT_NONE)
        {
    	return (OK);				/* destination polling */
        }

    /* Bus interrupts use 2 arguments */

    intArg1 = ntohl (pCpuDesc->intArg1);
    intArg2 = ntohl (pCpuDesc->intArg2);

    if (intType == SM_INT_BUS)
	{
	return (sysBusIntGen (intArg1, intArg2));
	}

    /* all other methods use all 3 arguments */

    intArg3 = ntohl (pCpuDesc->intArg3);

    if ((intType >= SM_INT_MAILBOX_1) && (intType <= SM_INT_MAILBOX_R4))
	{
	/* For mailboxes, arg1 is addrSpaceId, arg2 is address */

	if (sysBusToLocalAdrs (intArg1, (char *) intArg2, &pMailbox) == ERROR)
	    {
	    if (smUtilVerbose)
	        {
		logMsg ("smUtilIntGen:unable to calculate mailbox [0x%x]\n",
			intArg2, 0, 0, 0, 0, 0);
	        }
	    return (ERROR);
	    }
	}

    switch (intType)
	{
    	case SM_INT_MAILBOX_1:
	    *pMailbox = (char) intArg3;
            CACHE_PIPE_FLUSH ();                 /* CACHE FLUSH  [SPR 68334] */
	    bogus = *(volatile char *) pMailbox; /* BRIDGE FLUSH [SPR 68334] */
	    break;

    	case SM_INT_MAILBOX_2:
	    *(short *) pMailbox = (short) intArg3;
            CACHE_PIPE_FLUSH ();                 /* CACHE FLUSH  [SPR 68334] */
	    bogus = *(volatile short *) pMailbox; /* BRIDGE FLUSH [SPR 68334]*/
	    break;

     	case SM_INT_MAILBOX_4:
	    *(long *) pMailbox = (long) intArg3;
            CACHE_PIPE_FLUSH ();                 /* CACHE FLUSH  [SPR 68334] */
	    bogus = *(volatile long *) pMailbox; /* BRIDGE FLUSH [SPR 68334] */
	    break;

    	case SM_INT_MAILBOX_R1:
	    bogus = *(volatile char *) pMailbox;
	    break;

    	case SM_INT_MAILBOX_R2:
	    bogus = *(volatile short *) pMailbox;
	    break;

    	case SM_INT_MAILBOX_R4:
	    bogus = *(volatile long *) pMailbox;
	    break;

	case SM_INT_USER_1:
	    if (smUtilUser1Rtn != NULL)
	        {
		return (*smUtilUser1Rtn)(intArg1, intArg2, intArg3);
	        }
	    return ERROR;

	case SM_INT_USER_2:
	    if (smUtilUser2Rtn != NULL)
	        {
		return (*smUtilUser2Rtn)(intArg1, intArg2, intArg3);
	        }
	    return ERROR;

	default:
	    printf ("smUtilIntGen:Unknown intType [%x]\n", intType);
	    return (ERROR);			/* unknown interrupt type */
	}
    return (OK);
    }

/*****************************************************************************
*
* smUtilIntConnect - connect the shared memory interrupts
*
* This routine connects up how this CPU (ie mailbox, bus or polling task) gets
* notified of incoming shared memory packets or shared memory events.
*
* RETURNS: OK if successful, otherwise ERROR.
*/

STATUS smUtilIntConnect
    (
    int			priorityType,	/* handler priority LOW or HIGH */
    FUNCPTR		routine,	/* routine to connect */
    int                 param,          /* unit number */
    int                 intType,        /* interrupt method */
    int                 intArg1,        /* interrupt argument #1 */
    int                 intArg2,        /* interrupt argument #2 */
    int                 intArg3         /* interrupt argument #3 */
    )
    {
    STATUS              status = ERROR; /* return status */

    /* appease the Diab gods (SPR 71120) */

    if (intArg3 == 0)
        ;

    if (priorityType == LOW_PRIORITY)
	{
	smUtilNetRoutine.routine = routine;
	smUtilNetRoutine.param   = param;
	}
    else
	{
	smUtilObjRoutine.routine = routine;
	smUtilObjRoutine.param   = param;
	}

    switch (intType)
        {
        case SM_INT_BUS:                        /* bus interrupts */
            if ((intConnect (INUM_TO_IVEC (intArg2), smUtilIntRoutine, 0)
		 != ERROR) && (sysIntEnable (intArg1) != ERROR))
		{
                status = OK;
		}
            break;

        case SM_INT_NONE:                       /* polling */

           if ((taskIdVerify (smUtilPollTaskID) == OK)   ||
                ((smUtilPollTaskID =
                 taskSpawn ("tsmPollTask", smUtilPollTaskPriority,
                             smUtilPollTaskOptions, smUtilPollTaskStackSize,
                             (FUNCPTR) smUtilPollTask, 0, 0, 0, 0, 0, 0, 0,
                             0, 0, 0)) != ERROR))
                {
            	status = OK;
                }
            break;

        case SM_INT_MAILBOX_1:                  /* mailbox interrupts */
        case SM_INT_MAILBOX_2:
        case SM_INT_MAILBOX_4:
        case SM_INT_MAILBOX_R1:
        case SM_INT_MAILBOX_R2:
        case SM_INT_MAILBOX_R4:
            if ((sysMailboxConnect ((FUNCPTR) smUtilIntRoutine, 0) != ERROR) &&
                (sysMailboxEnable ((char *) intArg2) != ERROR))
                {
                status = OK;
                }
            break;

        case SM_INT_USER_1:
        case SM_INT_USER_2:

	    /* No special action, user code does it all */

	    status = OK;

	    break;

        default:
            logMsg ("smUtilIntConnect:Unknown interrupt type %d\n",
                    intType, 0, 0, 0, 0, 0);
            break;
        }

    return (status);
    }

/*****************************************************************************
*
* smUtilIntRoutine - generic shared memory interrupt handler
*
* This routine is connected to the shared memory interrupts. It calls
* dedicated handlers for incoming shared memory packets or shared
* memory objects events.
*
* RETURNS: N/A
*/

void smUtilIntRoutine (void)
    {
    /* call each routine stored in the list */

    if (smUtilObjRoutine.routine != NULL)
        {
	(* smUtilObjRoutine.routine) (smUtilObjRoutine.param);
        }

    if (smUtilNetRoutine.routine != NULL)
        {
	(* smUtilNetRoutine.routine) (smUtilNetRoutine.param);
        }

    }

/*****************************************************************************
*
* smUtilPollTask - task level generic shared memory events handler
*
* This task handles the shared memory events.  It calls dedicated 
* handlers for incomming shared memory packets or shared memory 
* objects events.
*
* RETURNS: N/A
*/

LOCAL void smUtilPollTask (void)
    {
    FOREVER
	{
	if (smUtilObjRoutine.routine != NULL)
	    {
	    (* smUtilObjRoutine.routine) (smUtilObjRoutine.param);
	    }

	if (smUtilNetRoutine.routine != NULL)
	    {
	    (* smUtilNetRoutine.routine) (smUtilNetRoutine.param);
	    }
	}
    }

