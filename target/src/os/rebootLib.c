/* rebootLib.c - reboot support library */

/* Copyright 1984-1993 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
02i,14jul97,dgp  doc: change ^X to CTRL-X in library description
02h,22oct95,jdi  doc: added bit values for reboot() <startType>.
02g,10oct95,jdi  doc: added .tG Shell to SEE ALSO for reboot().
02f,27jan93,jdi  documentation cleanup for 5.1;
                 SPR#1594: change include file to rebootLib.h
02e,23oct92,jcf  made cacheClear a macro.
02d,19oct92,jcf  removed disInst/disData.  Locked interrupts in reboot().
02c,12oct92,jdi  fixed mangen problem for reboot() by moving declarations
		 of disInst & disData to top section as customary.
02b,01oct92,jcf  added cacheDisable() to push out any data before reboot.
02a,04jul92,jcf  reworked to use dllLib.
01l,13dec91,gae  ANSI cleanup.
01k,19nov91,rrr  shut up some ansi warnings.
01j,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed VOID to void
		  -changed copyright notice
01i,05apr91,jdi	 documentation -- removed header parens and x-ref numbers;
		 doc review by gae.
01h,24mar91,jaa  documentation cleanup.
01g,20jun90,gae  changed reboot documentation to match new startTypes.
01f,19mar90,jdi  documentation tweak.
01e,07mar90,jdi  documentation cleanup.
01d,19aug88,gae  documentation.
01c,22jun88,dnw  name tweaks.
01b,30may88,dnw  changed to v4 names.
01a,28may88,dnw  extracted from hookLib and kLib.
		 modified to used malloc'ed linked list instead of fixed size
		 table.
*/

/*
DESCRIPTION
This library provides reboot support.  To restart VxWorks, the routine
reboot() can be called at any time by typing CTRL-X from the shell.  Shutdown
routines can be added with rebootHookAdd().  These are typically used to
reset or synchronize hardware.  For example, netLib adds a reboot hook
to cause all network interfaces to be reset.  Once the reboot hooks have
been run, sysToMonitor() is called to transfer control to the boot ROMs.
For more information, see the manual entry for bootInit.

DEFICIENCIES
The order in which hooks are added is the order in which they are run.
As a result, netLib will kill the network, and no user-added hook routines
will be able to use the network.  There is no rebootHookDelete() routine.

INCLUDE FILES: rebootLib.h

SEE ALSO: sysLib, bootConfig, bootInit
*/

#include "vxWorks.h"
#include "stdlib.h"
#include "cacheLib.h"
#include "dllLib.h"
#include "memLib.h"
#include "rebootLib.h"
#include "taskLib.h"
#include "intLib.h"

typedef struct
    {
    DL_NODE node;
    FUNCPTR func;
    } REBOOT_HOOK;

LOCAL DL_LIST	rebootHookList;
LOCAL BOOL	rebootHooksInitialized;

/*******************************************************************************
*
* reboot - reset network devices and transfer control to boot ROMs
*
* This routine returns control to the boot ROMs after calling a series of
* preliminary shutdown routines that have been added via
* rebootHookAdd(), including routines to reset all network devices.
* After calling the shutdown routines, interrupts are locked, all caches
* are cleared, and control is transferred to the boot ROMs.
*
* The bit values for <startType> are defined in sysLib.h:
* .iP "BOOT_NORMAL  (0x00)" 8
* causes the system to go through the countdown sequence and try to reboot
* VxWorks automatically.  Memory is not cleared.
* .iP "BOOT_NO_AUTOBOOT  (0x01)"
* causes the system to display the VxWorks boot prompt and wait for user
* input to the boot ROM monitor.  Memory is not cleared.
* .iP "BOOT_CLEAR  (0x02)"
* the same as BOOT_NORMAL, except that memory is cleared.
* .iP "BOOT_QUICK_AUTOBOOT  (0x04)"
* the same as BOOT_NORMAL, except the countdown is shorter.
*
* RETURNS: N/A
*
* SEE ALSO: sysToMonitor(), rebootHookAdd(),
* .pG "Target Shell,"
* windsh,
* .tG "Shell"
*/

void reboot
    (
    int startType       /* how the boot ROMS will reboot */
    )
    {
    FAST REBOOT_HOOK *pHook;
    int lock;

    if (rebootHooksInitialized)
	{
	for (pHook = (REBOOT_HOOK *) DLL_FIRST (&rebootHookList);
	     pHook != NULL;
	     pHook = (REBOOT_HOOK *) DLL_NEXT (&pHook->node))
	    {
	    (* pHook->func) (startType);
	    }
	}

    lock = intLock ();					/* LOCK INTERRUPTS */

    if (cacheLib.clearRtn != NULL)			/* clear out cache */
	(* cacheLib.clearRtn) (DATA_CACHE, NULL, ENTIRE_CACHE);

    (void) sysToMonitor (startType);
    }
/*******************************************************************************
*
* rebootHookAdd - add a routine to be called at reboot
*
* This routine adds the specified routine to a list of routines to be
* called when VxWorks is rebooted.  The specified routine should be
* declared as follows:
* .CS
*     void rebootHook
*         (
*         int startType   /@ startType is passed to all hooks @/
*         )
* .CE
*
* RETURNS: OK, or ERROR if memory is insufficient.
*
* SEE ALSO: reboot()
*/

STATUS rebootHookAdd
    (
    FUNCPTR rebootHook          /* routine to be called at reboot */
    )
    {
    REBOOT_HOOK *pHook;

    /* initialize hook list if this is the first time */

    if (!rebootHooksInitialized)
	{
	dllInit (&rebootHookList);
	rebootHooksInitialized = TRUE;
	}

    /* allocate and initialize hook node */

    pHook = (REBOOT_HOOK *) memPartAlloc (memSysPartId, sizeof (REBOOT_HOOK));

    if (pHook == NULL)
	return (ERROR);

    pHook->func = rebootHook;

    /* add it to end of hook list */

    taskLock ();				/* LOCK PREEMPTION */
    dllAdd (&rebootHookList, &pHook->node);
    taskUnlock ();				/* UNLOCK PREEMPTION */

    return (OK);
    }
