/* seqDrv.c - sequential timer driver */

/* Copyright 1984-1995 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01c,01feb95,rhp  mark library man page NOMANUAL (per jdi)
01b,17jan94,smb  added documentation
01a,10dec93,smb  created
*/

/*
DESCRIPTION
This driver contains the routines used to maintain a sequential timer.

In WindView, this driver is used if a timestamp driver is not available
or disabled for the target board. Events are tagged with a sequence number
in the order they occur on the target. When displayed in WindView, they
are spaced equidistantly along the timeline.

INCLUDE FILES:

SEE ALSO:

NOMANUAL
*/

#include "vxWorks.h"
#include "private/seqDrvP.h"

/* locals */

static int sequence; /* a static variable used to hold the sequence */

/*******************************************************************************
*
* seqStamp - implements a sequencer.
*
* NOMANUAL
*/

UINT32 seqStamp (void)
    {
    sequence++;
    return (sequence);
    }

/*******************************************************************************
*
* seqStampLock - implements a sequencer.
*
* NOMANUAL
*/

UINT32 seqStampLock (void)
    {
    sequence++;
    return (sequence);
    }

/*******************************************************************************
*
* seqEnable - Enable the sequencer
*
* NOMANUAL
*/

STATUS seqEnable (void)
    {
    sequence = 0;
    return (OK);
    }

/*******************************************************************************
*
* seqDisable - disable the sequencer
*
* NOMANUAL
*/

STATUS seqDisable (void)
    {
    return (OK);
    }

/*******************************************************************************
*
* seqConnect - connect to a sequencer.
*
* NOMANUAL
*/

STATUS seqConnect 
    (
    FUNCPTR dummy,
    int dummyArg
    )
    {
    return (OK);
    }

/*******************************************************************************
*
* seqPeriod - period of sequencer.
*
* NOMANUAL
*/

UINT32 seqPeriod (void)
    {
    return (0xffffffff);
    }

/*******************************************************************************
*
* seqFreq - frequency of sequencer
*
* NOMANUAL
*/

UINT32 seqFreq (void)
    {
    return (1);
    }

