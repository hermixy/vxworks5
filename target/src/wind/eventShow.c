/* eventShow.c - VxWorks events show routines */

/* Copyright 2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01a,06nov01,aeg  written
*/

/*
DESCRIPTION
This library provides routines to show VxWorks events information.  

The routine eventTaskShow() displays the contents of the EVENTS structure
embedded in a task's control block (WIND_TCB).  The show routine taskShow()
utilizes eventTaskShow().

The routine eventRsrcShow() displays the contents of the EVENTS_RSRC
structure embedded in event resources, e.g. the semaphore (SEMAPHORE)
and message queue structures (MSG_Q).  The show routines semShow() and
msgQShow() utilize eventRsrcShow().

The module eventShow.o will be automatically configured into VxWorks when
INCLUDE_SHOW_ROUTINES is defined in config.h, or if the Tornado project
facility is used, when the INCLUDE_SEM_SHOW, INCLUDE_MSG_Q_SHOW, or 
INCLUDE_TASK_SHOW components are selected.

INCLUDE FILES: eventLibP.h

SEE ALSO: semLib, msgQLib, eventLib
.pG "Basic OS"
*/

#include "vxWorks.h"
#include "taskLib.h"
#include "stdio.h"
#include "string.h"
#include "eventLib.h"
#include "private/eventLibP.h"

/*******************************************************************************
*
* eventTaskShow - show a task's VxWorks events information
*
* This routine displays the contents of the EVENTS structure embedded in
* a task's control block (WIND_TCB).  The show routine taskShow()
* utilizes eventTaskShow().
*
* The VxWorks events related information is displayed as follows:
* .CS
*     VxWorks Events
*     --------------
*     Events Pended on   : 0x7f
*     Received Events    : 0x7e
*     Options            : 0x3        EVENTS_WAIT_ANY
*     				      EVENTS_RETURN_ALL
* .CE
*
* RETURNS: OK always.
*
* SEE ALSO:
* .pG "Target Shell,"
* windsh,
* .tG "Shell"
*
* NOMANUAL
*/

STATUS eventTaskShow
    (
    EVENTS *pEvents		/* VxWorks events pointer */
    )
    {
    printf ("\nVxWorks Events\n--------------\n");
    if ((pEvents->sysflags & EVENTS_SYSFLAGS_WAITING) == 0)
	{
	printf ("Events Pended on    : Not Pended\n"
		"Received Events     : 0x%x\n"
		"Options             : N/A\n", pEvents->received);
	}
    else
	{
	printf ("Events Pended on    : 0x%x\n"
		"Received Events     : 0x%x\n"
		"Options             : 0x%x\t", pEvents->wanted, 
						pEvents->received, 
						pEvents->options);

	/* dump options string after hex representation */

	if ((pEvents->options & EVENTS_WAIT_MASK) == EVENTS_WAIT_ALL)
	    printf ("EVENTS_WAIT_ALL\n");
	else
	    printf ("EVENTS_WAIT_ANY\n");

	if ((pEvents->options & EVENTS_RETURN_ALL) != 0)
	    printf ("\t\t\t\tEVENTS_RETURN_ALL\n");
	}

    return (OK);
    }

/*******************************************************************************
*
* eventRsrcShow - show a resource's VxWorks events information
*
* This routine displays the contents of the EVENTS_RSRC structure embedded
* in the semaphore (SEMAPHORE) and message queue structures (MSG_Q).  The
* show routines semShow() and msgQShow() utilize eventRsrcShow().
*
* The VxWorks events related information is displayed as follows:
* .CS
*     VxWorks Events
*     --------------
*     Registered Task     : 0x594f0 (t1)
*     Event(s) to Send    : 0x1
*     Options             : 0x7       EVENTS_SEND_ONCE
*                                     EVENTS_ALLOW_OVERWRITE
*                                     EVENTS_SEND_IF_FREE
* .CE
*
* RETURNS: OK always.
*
* SEE ALSO:
* .pG "Target Shell,"
* windsh,
* .tG "Shell"
*
* NOMANUAL
*/

STATUS eventRsrcShow
    (
    EVENTS_RSRC *pEvRsrc	/* VxWorks events resource pointer */
    )
    {
    char * pTaskName;

    printf ("\nVxWorks Events\n--------------\n");
    if (pEvRsrc->taskId == (int) NULL)
	{
	printf ("Registered Task     : NONE\n"
		"Event(s) to Send    : N/A\n"
		"Options             : N/A\n");
	}
    else
	{
	pTaskName = taskName (pEvRsrc->taskId);

	printf ("Registered Task     : 0x%x\t(%s)\n"
		"Event(s) to Send    : 0x%x\n"
		"Options             : 0x%x\t", 
		
			pEvRsrc->taskId,
			pTaskName == NULL ? "Deleted!" : pTaskName,
		   	pEvRsrc->registered,
		   	pEvRsrc->options);

	if (pEvRsrc->options == EVENTS_OPTIONS_NONE)
	    printf ("EVENTS_OPTIONS_NONE\n");
	else
	    {
	    if ((pEvRsrc->options & EVENTS_SEND_ONCE) != 0)
		printf ("EVENTS_SEND_ONCE\n\t\t\t\t");

	    if ((pEvRsrc->options & EVENTS_ALLOW_OVERWRITE) != 0)
		printf ("EVENTS_ALLOW_OVERWRITE\n\t\t\t\t");

	    if ((pEvRsrc->options & EVENTS_SEND_IF_FREE) != 0)
		printf ("EVENTS_SEND_IF_FREE\n");
	    }
        }

    return (OK);
    }
