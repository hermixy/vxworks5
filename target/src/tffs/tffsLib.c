/* tffsLib.c - TFFS FLite library for VxWorks. */

/* Copyright 1984-1997 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/* FAT-FTL Lite Software Development Kit
 * Copyright (C) M-Systems Ltd. 1995-1997	*/

/*
modification history
--------------------
01f,13apr98,yp   made interval timer a task instead of exc task.(SPR #20693)
01e,06jan98,hdn  added INVERSION_SAFE to the mutex semaphore.
01d,17dec97,hdn  fixed a typo in return value at RETURNS:
01c,15dec97,yp   doc cleanup.
01b,07nov97,hdn  cleaned up.
01a,20mar96,ami  written by Amirban in M-Systems
*/

/*
DESCRIPTION
This library provides the OS binding to vxWorks for TFFS FLite .


INCLUDE FILES: 
*/


/* includes */

#include "flbase.h"
#include "tffsDrv.h"
#include "taskLib.h"


/* defines */
#if     (POLLING_INTERVAL > 0)
#define FL_POLL_TASK_PRIORITY 	100
#define FL_POLL_TASK_OPT 	0
#define FL_POLL_TASK_STACK_SIZE	2048
#endif /* (POLLING_INTERVAL > 0) */


/* externs */
#if	(POLLING_INTERVAL > 0)
IMPORT SEM_ID 	flPollSemId;
#endif	/* (POLLING_INTERVAL > 0) */


/* globals */



/* locals */

LOCAL int	flSysClkRate;	/* ticks per second       */
#if	(POLLING_INTERVAL > 0)
LOCAL int	flPollInterval;	/* socket polling interval in milliseconds */
LOCAL int	flPollTaskId;
#endif	/* (POLLING_INTERVAL > 0) */


/* forward declarations */

LOCAL void	flTimerInit (void);
#if	(POLLING_INTERVAL > 0)
LOCAL void	(*flSocketPollingRoutine) (void);
#endif	/* (POLLING_INTERVAL > 0) */


/*******************************************************************************
*
* flCreateMutex - create a mutex semaphore
*
* This translates a TFFS mutex create call to vxWorks.
*
* RETURNS: flOK, or flNotEnoughMemory if it fails.
*/

FLStatus flCreateMutex 
    (
    FLMutex * mutex	/* pointer to mutex */
    )
    {
    /* Io requests should be processed exactly in the same */
    /* sequence they are received                          */

    *mutex = semMCreate (SEM_DELETE_SAFE | SEM_INVERSION_SAFE | SEM_Q_PRIORITY);

    return ((*mutex != NULL) ? flOK : flNotEnoughMemory);
    }
/*******************************************************************************
*
* flTakeMutex - take a mutex semaphore
*
* This routine translates a mutex take call from TFFS to vxWorks.
*
* RETURNS: 0 if mutex not available otherwise returns 1.
*/

int flTakeMutex 
    (
    FLMutex * mutex,	/* pointer to mutex */
    int mode		/* 0 - nowait */
    )
    {
    STATUS status;

    status = semTake (*mutex, (mode == 0 ? NO_WAIT : WAIT_FOREVER));
    if (status == ERROR)
	return (0);
    else
        return (1);
    }

/*******************************************************************************
*
* flFreeMutex - release a mutex semaphore
*
* This routine translates a mutex release call from TFFS to vxWorks.
*
* RETURNS: N/A
*/

void flFreeMutex
    (
    FLMutex * mutex     /* pointer to mutex */
    )
    {
    semGive (*mutex);
    }

#ifdef EXIT

/*******************************************************************************
*
* flDeleteMutex - delete a mutex semaphore
*
* This routine translates a deletes mutex semaphore call from TFFS to vxWorks.
*
* RETURNS: N/A
*/

void flDeleteMutex
    (
    FLMutex * mutex     /* pointer to mutex */
    )
    {
    semDelete (*mutex);
    *mutex = NULL;
    }

#endif /* EXIT */


/*******************************************************************************
*
* flSysfunInit - initialize all the OS bindings
*
* This routine initializes all the OS bindings.
*
* RETURNS: N/A
*/

void flSysfunInit (void)
    {
    flTimerInit ();
    }

/*******************************************************************************
*
* flTimerInit - initialize timer for socket polling
*
* This routine initializes timer for socket polling.
*
* RETURNS: N/A
*/

static void flTimerInit (void)
    {
    flSysClkRate = sysClkRateGet ();
    }

#if	(POLLING_INTERVAL > 0)

/*******************************************************************************
*
* flPollTask - interval polling task
*
* This routine waits on the polling semaphore and invokes the interval
* timer work routine when the semaphore becomes available. The function 
* is not local so the task list on the shell can display the entry point.
*
* RETURNS: 
*/

void flPollTask (void)
    {
    int delay;

    semTake (flPollSemId, WAIT_FOREVER);
    delay = (flPollInterval * flSysClkRate )/1000;
    if (delay == 0)
        delay = 1;

    /* remove the polling semaphore */
    semDelete (flPollSemId);
    FOREVER
	{
	taskDelay (delay);
	(*flSocketPollingRoutine)();
	}
    }

/*******************************************************************************
*
* flInstallTimer - install the timer for socket polling
*
* This routine installs the timer used for socket polling.
*
* RETURNS: flOK, or flNotEnoughMemory if wdCreate() failed.
*/

FLStatus flInstallTimer 
    (
    void (*routine)(void),
    unsigned int intervalMsec
    )
    {

    flPollInterval         = intervalMsec;
    flSocketPollingRoutine = routine;

    /* Spawn the polling task */
    if ((flPollTaskId = taskSpawn ("tTffsPTask",
				  FL_POLL_TASK_PRIORITY,
				  FL_POLL_TASK_OPT,
				  FL_POLL_TASK_STACK_SIZE,
				  (FUNCPTR) flPollTask,0,0,0,0,0,0,0,0,0,0)) == ERROR)
        return (flNotEnoughMemory);

    return (flOK);
    }

#ifdef EXIT

/*******************************************************************************
*
* flRemoveTimer - cancel socket polling
*
* This routine cancels socket polling.
*
* RETURNS: N/A
*/

void flRemoveTimer (void)
    {
    /* Call it twice to shut down everything */

    (*flSocketPollingRoutine)();
    (*flSocketPollingRoutine)();

    /* remove socket polling */

    flSocketPollingRoutine = NULL;

    /* remove the polling task */
    taskDelete (flPollTaskId);
    }

#endif	/* EXIT */
#endif	/* POLLING_INTERVAL */


/*******************************************************************************
*
* flRandByte - return a random number from 0 to 255
*
* This routine returns a random number from 0 to 255.
*
* RETURNS: random number
*/

unsigned flRandByte (void)
    {
    /* XXX return (tickGet () & 0xff); */
    return (rand () & 0xff);
    }

/*******************************************************************************
*
* flCurrentTime - returns current time of day
*
* This routine Returns current time of day in the format described below.
*      bits  4 ..  0  - seconds
*      bits 10 ..  5  - minutes
*      bits 15 .. 11  - hours
*
* RETURNS: current time
*/

unsigned flCurrentTime (void)
    {
    unsigned char hour, minute, second;
    time_t        flAnsiTime;
    struct tm    *flLocalTimePtr;

    time (&flAnsiTime);

    flLocalTimePtr = localtime (&flAnsiTime);

    hour   = (unsigned char) flLocalTimePtr->tm_hour;
    minute = (unsigned char) flLocalTimePtr->tm_min;
    second = (unsigned char) flLocalTimePtr->tm_sec;

    return ((hour << 11) | (minute << 5) | second);
    }

/*******************************************************************************
*
* flCurrentDate - returns current date
*
* This routine Returns current date in the format described below.
*      bits  4 .. 0  - day
*      bits  8 .. 5  - month
*      bits 15 .. 9  - year (0 for 1980)
*
* RETURNS: current date
*/

unsigned flCurrentDate (void)
    {
    short int  year;
    char       month, day;
    struct tm *flLocalTimePtr;
    time_t     flAnsiTime;

    time (&flAnsiTime);

    flLocalTimePtr = localtime (&flAnsiTime);

    year  = (short int) (flLocalTimePtr->tm_year - 80);
    month = (char)       flLocalTimePtr->tm_mon + 1;
    day   = (char)       flLocalTimePtr->tm_mday;

    return ((year << 9) | (month << 5) | day);
    }

/*******************************************************************************
*
* flAddLongToFarPointer - add unsigned long offset to the far pointer
*
* This routine adds an unsigned long offset to a far pointer.
*
* RETURNS: far pointer
*/

void FAR0*  flAddLongToFarPointer
    (
    void FAR0 *ptr, 
    unsigned long offset
    )
    {
    return (physicalToPointer( pointerToPhysical(ptr) + offset, 0,0));
    }


#if	FALSE	/* moved to the BSP/sysTffs.c */
/*******************************************************************************
*
* flDelayMsecs - wait for specified number of milliseconds
*
* This routine waits for a specified number of milliseconds doing nothing.
*
* RETURNS: N/A
*/

void flDelayMsecs 
    (
    unsigned milliseconds	/* milliseconds to wait */
    )
    {
    unsigned long stop, ticksToWait;

    ticksToWait = (milliseconds * flSysClkRate) / 500;
    if (ticksToWait == 0x0l)
        ticksToWait++;

    stop = tickGet () + ticksToWait;
    while (tickGet () <= stop)
	;
    }

/*----------------------------------------------------------------------*/
/*                        f l D e l a y L o o p                         */
/*									*/
/* Short delay.                                                         */
/*                                                                      */
/* Parameters:                                                          */
/*      cycles          : wait loop cycles to perform                   */
/*----------------------------------------------------------------------*/

void flDelayLoop (int  cycles)     /* HOOK for VME-177 */
{

  while(--cycles) ; 
}
#endif	/* FLASE */
