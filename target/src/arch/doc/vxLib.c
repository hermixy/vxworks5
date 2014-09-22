/* vxLib.c - miscellaneous support routines */

/* Copyright 1984-2001 Wind River Systems, Inc. */

/*
modification history
--------------------
01t,05jun02,wsl  remove reference to SPARC and i960
01s,20nov01,hdn  updated x86 specific sections
01r,16nov01,tlc  Update for MIPS architecture.
01q,29oct01,zl   SH documentation updates.
01p,03mar00,zl   merged SH support into T2
01o,12mar99,elg  VX_POWER_MODE_NAP is not supported by PPC860 (SPR 22432).
01n,05oct98,jmp  doc: added {...} when necessary.
01m,18aug98,fle  doc : added star character at the beginning of the
                 vxPowerDown description
01l,21apr98,jpd  added ARM SWPB note.
01k,04jun97,dat  added vxMemArchProbe, SPR 8658
01j,12nov96,dgp  doc: add vxPowerModeGet/Set/Down routines
01i,25nov95,jdi  removed 29k stuff.
01h,23oct95,jdi  doc: added bit values to vxMemProbe() arg (SPR 4276).
01g,22oct95,jdi  doc: added am29k test-and-set instruction (SPR 4356).
01f,20mar95,jdi  added new vxSSEnable()/vxSSDisable() stuff from
			mc68k/vxALib.s.
01e,31jan95,rhp  doc tweaks.
            jdi  changed 80960 to i960.
01d,08nov93,pme Added vxUnalignedAccessEnable() and vxUnalignedAccessDisable().
01c,15feb93,jdi made NOMANUAL: vxAtomicModify(), vxIACSend(), vxIMRClear(),
		vxIMRSet(), vxSysctlSend().
01b,20jan93,jdi documentation cleanup.
01a,23sep92,jdi written, based on vxALib.s and vxLib.c for
		mc68k, sparc, i960, mips.
*/

/*
DESCRIPTION
This module contains miscellaneous VxWorks support routines.

INCLUDE FILES: vxLib.h
*/

/*******************************************************************************
*
* vxTas - C-callable atomic test-and-set primitive
*
* This routine provides a C-callable interface to a test-and-set
* instruction.  The test-and-set instruction is executed on the specified
* address.  The architecture test-and-set instruction is:
*
*	68K:	'tas'
*	x86:    'lock bts'
*	SH:	'tas.b'
*	ARM:	'swpb'
*
* This routine is equivalent to sysBusTas() in sysLib.
*
* MIPS:
* Because VxWorks does not support the MIPS MMU, only kseg0 and kseg1
* addresses are accepted; other addresses return FALSE. 
*
* NOTE X86:
* BTS "Bit Test and Set" instruction is executed with LOCK instruction prefix
* to lock the Bus during the execution.  The bit position 0 is toggled.
*
* NOTE SH:
* The SH version of vxTas() simply executes the `tas.b' instruction, and the
* test-and-set (atomic read-modify-write) operation may require an external
* bus locking mechanism on some hardware.  In this case, wrap the vxTas()
* with a bus locking and unlocking code in the sysBusTas().
*
* RETURNS:
* TRUE if the value had not been set (but is now), or FALSE if the
* value was set already.
*
* SEE ALSO: sysBusTas()
*/

BOOL vxTas 
    (
    void *	address		/* address to test and set */
    )

    {
    ...
    }


/*******************************************************************************
*
* vxMemArchProbe - architecture specific part of vxMemProbe
*
* This is the routine implementing the architecture specific part of the
* vxMemProbe routine.  It traps the relevant exceptions
* while accessing the specified address.  If an
* exception occurs, then the result will be ERROR.  If no exception occurs
* then the result will be OK.
*
* RETURNS: OK or ERROR if an exception occurred during access.
*/

STATUS vxMemArchProbe 
    (
    FAST char *  adrs,    /* address to be probed          */
    int          mode,    /* VX_READ or VX_WRITE           */
    int          length,  /* 1, 2, 4, or 8                 */
    FAST char *  pVal     /* where to return value,        */
			  /* or ptr to value to be written */
    )

    {
    ...
    }


/*******************************************************************************
*
* vxMemProbe - probe an address for a bus error
*
* This routine probes a specified address to see if it is readable or
* writable, as specified by <mode>.  The address is read or written as
* 1, 2, or 4 bytes, as specified by <length> (values other than 1, 2, or 4
* yield unpredictable results).  If the probe is a VX_READ (0), the value
* read is copied to the location pointed to by <pVal>.  If the probe is a
* VX_WRITE (1), the value written is taken from the location pointed to by
* <pVal>.  In either case, <pVal> should point to a value of 1, 2, or 4
* bytes, as specified by <length>.
*
* Note that only bus errors are trapped during the probe, and that the
* access must otherwise be valid (i.e., it must not generate an 
* address error).
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
* The BSP can modify the behaviour of vxMemProbe() by supplying an alternate
* routine and placing the address in the global variable
* _func_vxMemProbeHook.  The BSP routine will be called instead of the
* architecture specific routine vxMemArchProbe().
*
* INTERNAL
* This routine functions by setting the bus error trap vector to
* vxMemProbeTrap and then trying to read/write the specified byte.  If the
* address doesn't exist, vxMemProbeTrap will return ERROR.  Note that this
* routine saves and restores the bus error vector that was there prior to
* this call.  The entire procedure is done with interrupts locked out.
*
* RETURNS:
* OK, or ERROR if the probe caused a bus error or was misaligned.
*
* SEE ALSO:
* vxMemArchProbe()
*/

STATUS vxMemProbe 
    (
    FAST char *  adrs,    /* address to be probed          */
    int          mode,    /* VX_READ or VX_WRITE           */
    int          length,  /* 1, 2, 4, or 8                 */
    FAST char *  pVal     /* where to return value,        */
			  /* or ptr to value to be written */
    )

    {
    ...
    }


/*******************************************************************************
*
* vxMemProbeAsi - probe address in ASI space for bus error (SPARC)
*
* This routine probes the specified address to see if it is readable or
* writable, as specified by <mode>.  The address will be
* read/written as 1, 2, 4, or 8 bytes as specified by <length> 
* (values other than 1, 2, 4, or 8 return ERROR).  If the probe is a
* VX_READ (0), then the value read will be returned in the location pointed to
* by <pVal>.  If the probe is a VX_WRITE (1), then the value written will be
* taken from the location pointed to by <pVal>.  In either case, <pVal>
* should point to a value of the appropriate length, 1, 2, 4, or 8 bytes, as
* specified by <length>.
*
* The fifth parameter <adrsAsi> is the ASI parameter used to modify
* the <adrs> parameter.
*
* EXAMPLE
* .CS
*     testMem (adrs)
*        char *adrs;
*        {
*        char testW = 1;
*        char testR;
*
*        if (vxMemProbeAsi (adrs, VX_WRITE, 1, &testW) == OK)
*            printf ("value %d written to adrs %x\en", testW, adrs);
*
*        if (vxMemProbeAsi (adrs, VX_READ, 1, &testR) == OK)
*            printf ("value %d read from adrs %x\en", testR, adrs);
*        }
* .CE
*
* INTERNAL
* This routine functions by setting the bus error trap vector to vxMemProbeTrap
* and then trying to read/write the specified byte.
* If the address doesn't exist, vxMemProbeTrap will return ERROR.
* Note that this routine saves and restores the bus error vector that was
* there prior to this call.  The entire procedure is done with interrupts
* locked out.
*
* RETURNS:
* OK, or ERROR if the probe caused a bus error or
* was misaligned.
*
* NOMANUAL
*/

STATUS vxMemProbeAsi
    (
    FAST char *  adrs,	  /* address to be probed              */
    int          mode,	  /* VX_READ or VX_WRITE               */
    int          length,  /* 1, 2, 4, or 8                     */ 
    FAST char *  pVal,	  /* where to return value,            */
			  /* or ptr to value to be written     */
    int	adrsAsi		  /* ASI field of address to be probed */
    )

    {
    ...
    }

/*******************************************************************************
*
* vxAtomicModify - interface to the 'atmod' instruction (i960)
*
* This routine atomically modifies a location in memory.
*
* RETURNS: The original value in the memory location.
*
* NOMANUAL
*/

UINT8 vxAtomicModify
    (
    UINT8    value,         /* value to place in memory */
    UINT8 *  pMem,          /* location to modify       */
    UINT8    mask           /* bit mask for value       */
    )

    {
    ...
    }

/******************************************************************************
*
* vxCacheInvalidate - invalidate the instruction cache (i960)
*
* This routine invalidates the instruction cache.  The entire instruction
* cache is flushed.
*
* RETURNS: N/A
*
* NOMANUAL
*/  

vxCacheInvalidate (void)

    {
    ...
    }

/******************************************************************************
*
* vxSysctlSend - send a SYSCTL message to the processor (i960 CA)
*
* This routine sends a SYSCTL message to the processor.
*
* RETURNS: N/A
*
* NOMANUAL
*/

vxSysctlSend
    (
    UINT32	f12,	/* arg0 contains field2 messageType field1 */
    UINT32	f3,	/* arg1 contains field3                    */
    UINT32	f4	/* arg2 contains field4                    */
    )

    {
    ...
    }

/******************************************************************************
*
* vxIACSend - send an IAC message to the processor (i960 KB/SB)
*
* This routine sends an intra-agent communication (IAC) message to 
* the processor.
*
* RETURNS: N/A
*
* NOMANUAL
*/

vxIACSend
    (
    UINT32 *  iac	/* arg0 contains pointer to IAC structure */
    )

    {
    ...
    }


/******************************************************************************
*
* vxIMRClear - clear the interrupt mask register (i960)
*
* This routine clears bits in the interrupt mask register.
*
* RETURNS: N/A
*
* NOMANUAL
*/

vxIMRClear
    (
    UINT32	mask	/* bits to clear in interrupt mask reg */
    )

    {
    ...
    }


/******************************************************************************
*
* vxIMRSet - set the interrupt mask register (i960)
*
* This routine sets bits in the interrupt mask register.
*
* RETURNS: N/A
*
* NOMANUAL
*/

vxIMRSet
    (
    UINT32	mask	/* bits to set in interrupt mask reg */
    )

    {
    ...
    }


/*******************************************************************************
*
* vxSSEnable - enable the superscalar dispatch (MC68060)
*
* This function sets the ESS bit of the Processor Configuration Register (PCR)
* to enable the superscalar dispatch.
*
* RETURNS : N/A
*/

void vxSSEnable (void)

    {
    ...
    }

/*******************************************************************************
*
* vxSSDisable - disable the superscalar dispatch (MC68060)
*
* This function resets the ESS bit of the Processor Configuration Register (PCR)
* to disable the superscalar dispatch.
*
* RETURNS : N/A
*/

void vxSSDisable (void)

    {
    ...
    }

/*******************************************************************************
*
* vxPowerModeSet - set the power management mode (PowerPC, SH, x86)
*
* This routine selects the power management mode to be activated when
* vxPowerDown() is called.  vxPowerModeSet() is normally called in the BSP
* initialization routine sysHwInit().
*
* USAGE PPC:
* Power management modes include the following:
* .iP "VX_POWER_MODE_DISABLE (0x1)"
* Power management is disabled; this prevents the MSR(POW) bit from being
* set (all PPC).
* .iP "VX_POWER_MODE_FULL (0x2)"
* All CPU units are active while the kernel is idle (PPC603, PPCEC603 and
* PPC860 only).
* .iP "VX_POWER_MODE_DOZE (0x4)"
* Only the decrementer, data cache, and bus snooping are active while the
* kernel is idle (PPC603, PPCEC603 and PPC860).
* .iP "VX_POWER_MODE_NAP (0x8)"
* Only the decrementer is active while the kernel is idle (PPC603, PPCEC603 and
* PPC604 ).
* .iP "VX_POWER_MODE_SLEEP (0x10)"
* All CPU units are inactive while the kernel is idle (PPC603, PPCEC603 and
* PPC860 - not recommended for the PPC603 and PPCEC603 architecture).
* .iP "VX_POWER_MODE_DEEP_SLEEP (0x20)"
* All CPU units are inactive while the kernel is idle (PPC860 only - not
* recommended).
* .iP "VX_POWER_MODE_DPM (0x40)"
* Dynamic Power Management Mode (PPC603 and PPCEC603 only).
* .iP "VX_POWER_MODE_DOWN (0x80)"
* Only a hard reset causes an exit from power-down low power mode (PPC860 only
* - not recommended).
*
* USAGE SH:
* Power management modes include the following:
* .iP "VX_POWER_MODE_DISABLE (0x0)"
* Power management is disabled.
* .iP "VX_POWER_MODE_SLEEP (0x1)"
* The core CPU is halted, on-chip peripherals operating, external memory
* refreshing.
* .iP "VX_POWER_MODE_DEEP_SLEEP (0x2)"
* The core CPU is halted, on-chip peripherals operating, external memory
* self-refreshing (SH-4 only).
* .iP "VX_POWER_MODE_USER (0xff)"
* Set up to three 8-bit standby registers with user-specified values:
* .CS
*     vxPowerModeSet (VX_POWER_MODE_USER | sbr1<<8 | sbr2<<16 | sbr3<<24);
* .CE
* The sbr1 value is written to the STBCR or SBYCR1, sbr2 is written to
* the STBCR2 or SBYCR2, and sbr3 is written to the STBCR3 register (when
* available), depending on the SH processor type.
*
* .LP
* USAGE X86:
* vxPowerModeSet() is called in the BSP initialization routine sysHwInit().
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
* vxPowerModeGet(), vxPowerDown()
*/

STATUS vxPowerModeSet
    (
    UINT32 mode                 /* power management mode to select */
    )

    {
    ...
    }


/*******************************************************************************
*
* vxPowerModeGet - get the power management mode (PowerPC, SH, x86)
*
* This routine returns the power management mode set by vxPowerModeSet().
*
* RETURNS:
* The power management mode, or ERROR if no mode has been selected or if 
* power management is not supported.
*
* SEE ALSO:
* vxPowerModeSet(), vxPowerDown()
*/

UINT32 vxPowerModeGet (void)

    {
    ...
    }


/*******************************************************************************
*
* vxPowerDown - place the processor in reduced-power mode (PowerPC, SH)
*
* This routine activates the reduced-power mode if power management is enabled.
* It is called by the scheduler when the kernel enters the idle loop.
* The power management mode is selected by vxPowerModeSet().
*
* RETURNS: OK, or ERROR if power management is not supported or if external
* interrupts are disabled.
*
* SEE ALSO: vxPowerModeSet(), vxPowerModeGet()
*
*/


UINT32 vxPowerDown (void)

    {
    ...
    }

/*******************************************************************************
*
* vxCr0Get - get a content of the Control Register 0 (x86)
*
* This routine gets a content of the Control Register 0. 
*
* RETURNS: a value of the Control Register 0
*/

int vxCr0Get (void)

    {
    ...
    }

/*******************************************************************************
*
* vxCr0Set - set a value to the Control Register 0 (x86)
*
* This routine sets a value to the Control Register 0.
*
* RETURNS: N/A
*/

void vxCr0Set
    (
    int value			/* CR0 value */
    )

    {
    ...
    }

/*******************************************************************************
*
* vxCr2Get - get a content of the Control Register 2 (x86)
*
* This routine gets a content of the Control Register 2. 
*
* RETURNS: a value of the Control Register 2
*/

int vxCr2Get (void)

    {
    ...
    }

/*******************************************************************************
*
* vxCr2Set - set a value to the Control Register 2 (x86)
*
* This routine sets a value to the Control Register 2.
*
* RETURNS: N/A
*/

void vxCr2Set
    (
    int value			/* CR2 value */
    )

    {
    ...
    }

/*******************************************************************************
*
* vxCr3Get - get a content of the Control Register 3 (x86)
*
* This routine gets a content of the Control Register 3. 
*
* RETURNS: a value of the Control Register 3
*/

int vxCr3Get (void)

    {
    ...
    }

/*******************************************************************************
*
* vxCr3Set - set a value to the Control Register 3 (x86)
*
* This routine sets a value to the Control Register 3.
*
* RETURNS: N/A
*/

void vxCr3Set
    (
    int value			/* CR3 value */
    )

    {
    ...
    }

/*******************************************************************************
*
* vxCr4Get - get a content of the Control Register 4 (x86)
*
* This routine gets a content of the Control Register 4. 
*
* RETURNS: a value of the Control Register 4
*/

int vxCr4Get (void)

    {
    ...
    }

/*******************************************************************************
*
* vxCr4Set - set a value to the Control Register 4 (x86)
*
* This routine sets a value to the Control Register 4.
*
* RETURNS: N/A
*/

void vxCr4Set
    (
    int value			/* CR4 value */
    )

    {
    ...
    }

/*******************************************************************************
*
* vxEflagsGet - get a content of the EFLAGS register (x86)
*
* This routine gets a content of the EFLAGS register
*
* RETURNS: a value of the EFLAGS register
*/

int vxEflagsGet (void)

    {
    ...
    }

/*******************************************************************************
*
* vxEflagsSet - set a value to the EFLAGS register (x86)
*
* This routine sets a value to the EFLAGS register
*
* RETURNS: N/A
*/

void vxEflagsSet
    (
    int value			/* EFLAGS value */
    )

    {
    ...
    }

/*******************************************************************************
*
* vxDrGet - get a content of the Debug Register 0 to 7 (x86)
*
* This routine gets a content of the Debug Register 0 to 7. 
*
* RETURNS: N/A
*/

void vxDrGet
    (
    int * pDr0,			/* DR0 */
    int * pDr1,			/* DR1 */
    int * pDr2,			/* DR2 */
    int * pDr3,			/* DR3 */
    int * pDr4,			/* DR4 */
    int * pDr5,			/* DR5 */
    int * pDr6,			/* DR6 */
    int * pDr7			/* DR7 */
    )

    {
    ...
    }

/*******************************************************************************
*
* vxDrSet - set a value to the Debug Register 0 to 7 (x86)
*
* This routine sets a value to the Debug Register 0 to 7. 
*
* RETURNS: N/A
*/

void vxDrSet
    (
    int dr0,			/* DR0 */
    int dr1,			/* DR1 */
    int dr2,			/* DR2 */
    int dr3,			/* DR3 */
    int dr4,			/* DR4 */
    int dr5,			/* DR5 */
    int dr6,			/* DR6 */
    int dr7			/* DR7 */
    )

    {
    ...
    }

/*******************************************************************************
*
* vxTssGet - get a content of the TASK register (x86)
*
* This routine gets a content of the TASK register
*
* RETURNS: a value of the TASK register
*/

int vxTssGet (void)

    {
    ...
    }

/*******************************************************************************
*
* vxTssSet - set a value to the TASK register (x86)
*
* This routine sets a value to the TASK register
*
* RETURNS: N/A
*/

void vxTssSet
    (
    int value			/* TASK register value */
    )

    {
    ...
    }

/*******************************************************************************
*
* vxGdtrGet - get a content of the Global Descriptor Table Register (x86)
*
* This routine gets a content of the Global Descriptor Table Register
*
* RETURNS: N/A
*/

void vxGdtrGet
    (
    long long int * pGdtr	/* memory to store GDTR */
    )

    {
    ...
    }

/*******************************************************************************
*
* vxIdtrGet - get a content of the Interrupt Descriptor Table Register (x86)
*
* This routine gets a content of the Interrupt Descriptor Table Register
*
* RETURNS: N/A
*/

void vxIdtrGet
    (
    long long int * pIdtr	/* memory to store IDTR */
    )

    {
    ...
    }

/*******************************************************************************
*
* vxLdtrGet - get a content of the Local Descriptor Table Register (x86)
*
* This routine gets a content of the Local Descriptor Table Register
*
* RETURNS: N/A
*/

void vxLdtrGet
    (
    long long int * pLdtr	/* memory to store LDTR */
    )

    {
    ...
    }

