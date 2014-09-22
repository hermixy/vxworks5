/* vxLib.c - miscellaneous support routines */

/* Copyright 1984-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01v,20nov01,hdn  doc clean up for 5.5
01u,12nov01,ahm  added rudimentary support for power mgmt (SPR#32599)
01t,29aug01,hdn  included vxI86Lib.h 
		 replaced intVecSet/Get with intVecSet2/Get2
01s,04jun97,dat  added _func_vxMemProbeHook and vxMemArchProbe, SPR 8658.
01r,25apr94,hdn  supported both PROTECTION_FAULT and PAGE_FAULT.
01q,02jun93,hdn  updated to 5.1
		  - changed functions to ansi style
		  - fixed #else and #endif
		  - changed READ, WRITE to VX_READ, VX_WRITE
		  - changed copyright notice
01p,27aug92,hdn  added I80X86 support.
01o,12jul91,gae  reworked i960 vxMemProbe(); defined sysBErrVec here.
01n,09jul91,del  fixed vxMemProbe (I960) to use sysBErrVec global as bus
		 error vector. Installed new vxTas that will work on 
		 non-aligned bytes.
01m,01jun91,gae  fixed return of 960's vxTas().
01l,24may91,jwt  added ASI parameters to create vxMemProbeAsi.
01n,29apr91,hdn  added defines and macros for TRON architecture.
01m,29apr91,del  reference sysMemProbe trap for board dependent reasons.
01l,28apr91,del  I960 integration. added  vxMemProbeSup and vxTas here,
		 as callouts to system (board) dependent functions because
		 some i960 implementations require different interfaces 
		 to hardware.
01k,30mar91,jdi  documentation cleanup; doc review by dnw.
01j,19mar90,jdi  documentation cleanup.
01i,22aug89,jcf  fixed vxMemProbe for 68020/68030.
01h,29apr89,mcl  vxMemProbe for SPARC; merged versions.
01g,01sep88,gae  documentation.
01f,22jun88,dnw  removed include of ioLib.h.
01e,05jun88,dnw  changed from kLib to vxLib.
		 removed taskRegsShow(), exit(), and breakpoint rtns to taskLib.
01d,30may88,dnw  changed to v4 names.
01c,28may88,dnw  removed reboot to rebootLib.
01b,21apr88,gae  added include of ioLib.h for READ/WRITE/UPDATE.
01a,28jan88,jcf	 written and soon to be redistributed.
*/

/*
DESCRIPTION
This module contains miscellaneous VxWorks support routines.

SEE ALSO: vxALib
*/

#include "vxWorks.h"
#include "taskLib.h"
#include "intLib.h"
#include "iv.h"
#include "esf.h"
#include "vxLib.h"


/* imports */

IMPORT int	sysCsInt;
IMPORT void	vxIdleAutoHalt (void);


/* globals */

FUNCPTR	_func_vxMemProbeHook = NULL;	/* hook for BSP vxMemProbe */
UINT32 vxPowerMode = VX_POWER_MODE_DISABLE;	/* Pwr Mgmt disabled at start */
FUNCPTR vxIdleRtn  = (FUNCPTR)NULL;	/* PwrMgmt Rtn to call in Idle loop */


/*******************************************************************************
*
* vxMemArchProbe - architecture specific probe routine (x86)
*
* This is the routine implementing the architecture specific part of the
* vxMemProbe routine.  It traps the relevant exceptions
* while accessing the specified address.  If an
* exception occurs, then the result will be ERROR.  If no exception occurs
* then the result will be OK.
*
* INTERNAL
* This routine functions by setting the bus error trap vector to
* vxMemProbeTrap and then trying to read/write the specified byte.  If the
* address doesn't exist, vxMemProbeTrap will return ERROR.  Note that this
* routine saves and restores the bus error vector that was there prior to
* this call.  The entire procedure is done with interrupts locked out.
*
* RETURNS: OK or ERROR if an exception occurred during access.
*/

STATUS vxMemArchProbe
    (
    FAST void *adrs,	/* address to be probed          */
    int mode,		/* VX_READ or VX_WRITE           */
    int length,		/* 1, 2, or 4                    */
    FAST void *pVal	/* where to return value,        */
			/* or ptr to value to be written */
    )
    {
    STATUS status;
    int oldLevel;
    FUNCPTR oldVec1;
    FUNCPTR oldVec2;
    int oldType1;
    int oldType2;
    int oldSel1;
    int oldSel2;

    oldLevel = intLock ();			/* LOCK INTERRUPTS */

    /* save the vector for General Protection Fault and Page Fault */

    intVecGet2 ((FUNCPTR *)IV_PROTECTION_FAULT, &oldVec1, &oldType1, &oldSel1);
    intVecGet2 ((FUNCPTR *)IV_PAGE_FAULT, &oldVec2, &oldType2, &oldSel2);

    /* set new one to catch these exception */

    intVecSet2 ((FUNCPTR *)IV_PROTECTION_FAULT, (FUNCPTR)vxMemProbeTrap,
	        IDT_INT_GATE, sysCsInt);
    intVecSet2 ((FUNCPTR *)IV_PAGE_FAULT, (FUNCPTR)vxMemProbeTrap,
	        IDT_INT_GATE, sysCsInt);

    /* do probe */

    if (mode == VX_READ)
	status = vxMemProbeSup (length, adrs, pVal);
    else
	status = vxMemProbeSup (length, pVal, adrs);

    /* restore original vector(s) */

    intVecSet2 ((FUNCPTR *)IV_PROTECTION_FAULT, oldVec1, oldType1, oldSel1);
    intVecSet2 ((FUNCPTR *)IV_PAGE_FAULT, oldVec2, oldType2, oldSel2);

    intUnlock (oldLevel);			/* UNLOCK INTERRUPTS */

    return (status);
    }

/*******************************************************************************
*
* vxMemProbe - probe an address for bus error
*
* This routine probes a specified address to see if it is readable or
* writable, as specified by <mode>.  The address will be read or written as
* 1, 2, or 4 bytes, as specified by <length>.  (Values other than 1, 2, or 4
* yield unpredictable results).  If the probe is a READ, the value read will
* be copied to the location pointed to by <pVal>.  If the probe is a
* WRITE, the value written will be taken from the location pointed to by
* <pVal>.  In either case, <pVal> should point to a value of 1, 2, or 4
* bytes, as specified by <length>.
* 
* Note that only bus errors are trapped during the probe, and that the
* access must otherwise be valid (i.e., not generate an address error).
*
* EXAMPLE
* .CS
*     testMem (adrs)
*         char *adrs;
*         {
*         char testW = 1;
*         char testR;
*
*         if (vxMemProbe (adrs, VX_WRITE, 1, &testW) == OK)
*             printf ("value %d written to adrs %x\en", testW, adrs);
*
*         if (vxMemProbe (adrs, VX_READ, 1, &testR) == OK)
*             printf ("value %d read from adrs %x\en", testR, adrs);
*         }
* .CE
*
* MODIFICATION
* The BSP can modify the behaviour of this routine by supplying an alternate
* routine and placing the address of the routine in the global
* variable _func_vxMemProbeHook.  The BSP routine will be called instead of
* the architecture specific routine vxMemArchProbe().
*
* RETURNS:
* OK if the probe is successful, or ERROR if the probe caused a bus error or
* an address misalignment.
*
* SEE ALSO:
* vxMemArchProbe()
*/

STATUS vxMemProbe
    (
    FAST char *adrs,	/* address to be probed          */
    int mode,		/* VX_READ or VX_WRITE           */
    int length,		/* 1, 2, or 4                    */
    FAST char *pVal	/* where to return value,        */
			/* or ptr to value to be written */
    )
    {
    STATUS status;

    if (_func_vxMemProbeHook != NULL)
	{
	/* BSP specific probe routine */

	status = (* _func_vxMemProbeHook) (adrs, mode, length, pVal);
	}
    else
	{
	/* architecture specific probe routine */

	status = vxMemArchProbe (adrs, mode, length, pVal);
	}
    
    return (status);
    }

/*******************************************************************************
*
* vxPowerModeSet - set the power management mode (x86)
*
* This routine sets the power management mode which will be activated
* only when the kernel is idling.
* vxPowerModeSet() is normally called in the BSP initialization routine
* (sysHwInit).
*
* Power management modes include the following:
* .iP "VX_POWER_MODE_DISABLE (0x1)"
* Power management is disable: this prevents halting the CPU.
* .iP "VX_POWER_MODE_AUTOHALT (0x4)"
* Power management is enable: this allows halting the CPU.
*
* RETURNS: OK, or ERROR if <mode> is incorrect or not supported by the
* processor.
*
* SEE ALSO:
* vxPowerModeGet()
*/

STATUS vxPowerModeSet
    (
    UINT32 mode		/* power management mode to set */
    )
    {
    /* set vxPowMgtEnable and vxPowMgtMode according to <mode> */

    switch (mode)
	{
	case VX_POWER_MODE_DISABLE:
	    vxPowerMode = VX_POWER_MODE_DISABLE;
	    vxIdleRtn = (FUNCPTR)NULL;
	    break;

	case VX_POWER_MODE_AUTOHALT:
	    vxPowerMode = VX_POWER_MODE_AUTOHALT;
	    vxIdleRtn = (FUNCPTR)vxIdleAutoHalt;
	    break;

	default:
	    return (ERROR);     /* mode not supported */
	}

    return (OK);
    }

/*******************************************************************************
*
* vxPowerModeGet - get the power management mode (x86)
*
* This routine returns the power management mode set when kernel is idling.
*
* RETURNS:
* the power management mode (VX_POWER_MODE_DISABLE, VX_POWER_MODE_AUTOHALT)
*
* SEE ALSO:
* vxPowerModeSet()
*/

UINT32 vxPowerModeGet (void)
    {
    return (vxPowerMode);
    }
