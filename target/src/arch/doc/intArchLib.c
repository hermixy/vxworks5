/* intArchLib.c - architecture-dependent interrupt library */

/* Copyright 1984-2001 Wind River Systems, Inc. */

/*
modification history
--------------------
02v,05jun02,wsl  remove reference to SPARC and i960
02u,09may02,wsl  fix minor formatting error
02t,20nov01,hdn  updated x86 specific sections
02s,13nov01,hbh  Updated for simulators.
02r,30oct01,zl   corrected table in SH section of intConnect().
02q,02oct01,hdn  added intStackEnable() for PENTIUM (spr 69832: int stack)
02p,14sep01,hdn  added intHandlerCreateI86()/intVec[SG]et2() doc (spr 30292)
02o,19feb01,hk   update intConnect()/intVecBaseSet()/intVecSet() docs for T2/SH.
02n,03mar00,zl   merged SH support into T2
02m,16mar99,elg  add information about conversion macros (SPR 7473).
02l,12mar99,elg  delete the SEE ALSO comment in intLevelSet() (SPR 22809).
02k,11feb99,wsl  add comment documenting ERRNO value
02j,05oct98,jmp  doc: added {...} when necessary.
02i,21apr98,jpd  updated ARM-specific documentation.
02h,08jul97,dgp  doc: add info about PowerPC to intLock() (SPR 7768)
02h,03mar97,jpd  added ARM-specific documentation.
02g,09dec96,dgp  doc: fix SPR #7580 (add INCLUDE FILE: intLib.h) and change
		      MIPS R3000/R4000 to MIPS
02f,14nov96,dgp  doc: specify MIPS and PPC support for specific routines
02e,30oct96,dgp  doc: intHandlerCreate() does not exist in PowerPC per 
		 SPR 5851
02d,25nov95,jdi  removed 29k stuff.
02c,06feb95,rhp  add AM29K note to intHandlerCreate() man page.
            jdi  changed 80960 to i960.
02b,26jan95,rhp  doc: included i386/i486 info, R4000 info, and misc doc tweaks
02a,12jan95,rhp  propagated no-syscall caveat to intLevelSet doc (SPR#1094);
                 added refs to intLock() and taskLock() to SEE ALSO line 
                 for intLockLevelSet(), and second example to intLock() man page
                 (SPR#1304).
01d,18apr94,pme  added Am29K specific information to intLevelSet()
                 fixed intVecGet() manual for Am29200 and Am29030.
01c,07dec93,pme  doc tweaks.
		 added Am29K family support.
01h,22sep94,rhp  Restore paragraph break accidentally deleted with previous fix.
01g,22sep94,rhp  fix SPARC-specific intVecSet() description (SPR#2513).
01f,20sep94,rhp  propagate new SPARC info for intVecBaseSet()
01e,20sep94,rhp  propagate new SPARC info for intVecSet(); '94 copyright
01d,19sep94,rhp  do not mention exceptions in intArchLib man page (SPR#1494).
01c,16sep94,rhp  add caveats re avoiding system calls under intLock (SPR#3582).
01b,20jan93,jdi  documentation cleanup.
01a,23sep92,jdi  written, based on intALib.s and intArchLib.c for
		 mc68k, sparc, i960, mips.
*/

/*
DESCRIPTION
This library provides architecture-dependent routines to manipulate
and connect to hardware interrupts.  Any C language routine can be
connected to any interrupt by calling intConnect().  Vectors can be
accessed directly by intVecSet() and intVecGet().  The vector (trap)
base register (if present) can be accessed by the routines
intVecBaseSet() and intVecBaseGet().

Tasks can lock and unlock interrupts by calling intLock() and intUnlock().
The lock-out level can be set and reported by intLockLevelSet() and
intLockLevelGet() (MC680x0, x86, ARM and SH only).
The routine intLevelSet() changes the current interrupt level of the
processor (MC680x0, ARM, SimSolaris and SH).

WARNING
Do not call VxWorks system routines with interrupts locked.
Violating this rule may re-enable interrupts unpredictably.

INTERRUPT VECTORS AND NUMBERS
Most of the routines in this library take an interrupt vector as a
parameter, which is generally the byte offset into the vector table.
Macros are provided to convert between interrupt vectors and interrupt
numbers:
.iP IVEC_TO_INUM(intVector) 10
converts a vector to a number.
.iP INUM_TO_IVEC(intNumber)
converts a number to a vector.
.iP TRAPNUM_TO_IVEC(trapNumber)
converts a trap number to a vector.

EXAMPLE
To switch between one of several routines for a particular interrupt,
the following code fragment is one alternative:
.CS
    vector  = INUM_TO_IVEC(some_int_vec_num);
    oldfunc = intVecGet (vector);
    newfunc = intHandlerCreate (routine, parameter);
    intVecSet (vector, newfunc);
    ...
    intVecSet (vector, oldfunc);    /@ use original routine @/
    ...
    intVecSet (vector, newfunc);    /@ reconnect new routine @/
.CE

INCLUDE FILES: iv.h, intLib.h

SEE ALSO: intLib
*/


/*******************************************************************************
*
* intLevelSet - set the interrupt level (MC680x0, x86, ARM, SimSolaris, SimNT and SH)
*
* This routine changes the interrupt mask in the status register to take on
* the value specified by <level>.  Interrupts are locked out at or below
* that level.  The value of <level> must be in the following range:
*
* .TS
* tab(|);
* l l.
*     MC680x0:	        | 0 - 7
*     SH:	        | 0 - 15
*     ARM:	        | BSP-specific
*     SimSolaris:	| 0 - 1
*     x86:	        | interrupt controller specific
* .TE
*
* \"On SPARC systems, traps must be enabled before the call.
* On x86 systems, there are no interrupt level in the processor
* and the external interrupt controller manages the interrupt level.
* Therefore this routine does nothing and returns OK always.
*
* NOTE SIMNT: 
* This routine does nothing.
*
* WARNING
* Do not call VxWorks system routines with interrupts locked.
* Violating this rule may re-enable interrupts unpredictably.
*
* RETURNS: The previous interrupt level.
*/

int intLevelSet
    (
    int level	/* new interrupt level mask */
    )

    {
    ...
    }



/*******************************************************************************
*
* intLock - lock out interrupts
* 
* This routine disables interrupts.  The intLock() routine returns an
* architecture-dependent lock-out key representing the interrupt level
* prior to the call; this key can be passed to intUnlock() to
* re-enable interrupts.
* 
* For MC680x0, x86, and SH architectures, interrupts
* are disabled at the level set by intLockLevelSet().  The default
* lock-out level is the highest interrupt level (MC680x0 = 7,
* x86 = 1, SH = 15).  
* 
* For SimSolaris architecture, interrupts are masked. Lock-out level returned
* is 1 if interrupts were already locked, 0 otherwise.
*
* For SimNT, a windows semaphore is used to lock the interrupts.
* Lock-out level returned is 1 if interrupts were already locked, 0 otherwise.
*
* For MIPS processors, interrupts are disabled at the
* master lock-out level; this means no interrupt can occur even if
* unmasked in the IntMask bits (15-8) of the status register.
*
* For ARM processors, interrupts (IRQs) are disabled by setting the I bit
* in the CPSR. This means no IRQs can occur.
*
* For PowerPC processors, there is only one interrupt vector.  The external
* interrupt (vector offset 0x500) is disabled when intLock() is called; this
* means that the processor cannot be interrupted by any external event.
*
* IMPLEMENTATION
* The lock-out key is implemented differently for different architectures:
*
* .TS
* tab(|);
* l l.
*     MC680x0:     | interrupt field mask
*     MIPS:        | status register
*     x86:         | interrupt enable flag (IF) bit from EFLAGS register
*     PowerPC:     | MSR register value
*     ARM          | I bit from the CPSR
*     SH:          | status register
*     SimSolaris:  | 1 or 0 
*     SimNT:       | 1 or 0 
* .TE
* 
* WARNINGS
* Do not call VxWorks system routines with interrupts locked.
* Violating this rule may re-enable interrupts unpredictably.
*
* The routine intLock() can be called from either interrupt or task level.
* When called from a task context, the interrupt lock level is part of the
* task context.  Locking out interrupts does not prevent rescheduling.
* Thus, if a task locks out interrupts and invokes kernel services that
* cause the task to block (e.g., taskSuspend() or taskDelay()) or that cause a
* higher priority task to be ready (e.g., semGive() or taskResume()), then
* rescheduling occurs and interrupts are unlocked while other tasks
* run.  Rescheduling may be explicitly disabled with taskLock().
* Traps must be enabled when calling this routine.
* 
*
* EXAMPLES
* .CS
*     lockKey = intLock ();
*
*      ... (work with interrupts locked out)
*
*     intUnlock (lockKey);
* .CE
*
* To lock out interrupts and task scheduling as well (see WARNING above):
* .CS
*     if (taskLock() == OK)
*         {
*         lockKey = intLock ();
*
*         ... (critical section)
*
*         intUnlock (lockKey);
*         taskUnlock();
*         }
*      else
*         {
*         ... (error message or recovery attempt)
*         }
* .CE
*
* RETURNS
* An architecture-dependent lock-out key for the interrupt level
* prior to the call.
*
* SEE ALSO: intUnlock(), taskLock(), intLockLevelSet()
*/

int intLock (void)

    {
    ...
    }



/*******************************************************************************
*
* intUnlock - cancel interrupt locks
*
* This routine re-enables interrupts that have been disabled by intLock().
* The parameter <lockKey> is an architecture-dependent lock-out key
* returned by a preceding intLock() call.
*
* RETURNS: N/A
*
* SEE ALSO: intLock()
*/

void intUnlock
    (
    int lockKey		/* lock-out key returned by preceding intLock() */
    )

    {
    ...
    }

/*******************************************************************************
*
* intEnable - enable corresponding interrupt bits (MIPS, PowerPC, ARM)
* 
* This routine enables the input interrupt bits on the present status
* register of the MIPS and PowerPC processors.
*
* NOTE ARM:
* ARM processors generally do not have on-chip interrupt controllers.
* Control of interrupts is a BSP-specific matter.  This routine calls a
* BSP-specific routine to enable the interrupt.  For each interrupt
* level to be used, there must be a call to this routine before it will
* be allowed to interrupt.
*
* NOTE MIPS:
* For MIPS, it is strongly advised that the level be a combination of
* `SR_IBIT1' - `SR_IBIT8'.
*
* RETURNS: OK or ERROR. (MIPS: The previous contents of the status register).
* 
*/

int intEnable
    (
    int level	  /* new interrupt bits (0x00 - 0xff00) */
    )

    {
    ...
    }


/*******************************************************************************
*
* intDisable - disable corresponding interrupt bits (MIPS, PowerPC, ARM)
* 
* On MIPS and PowerPC architectures, this routine disables the corresponding
* interrupt bits from the present status register.  
*
* NOTE ARM:
* ARM processors generally do not have on-chip interrupt controllers.
* Control of interrupts is a BSP-specific matter. This routine calls a
* BSP-specific routine to disable a particular interrupt level,
* regardless of the current interrupt mask level.
*
* NOTE MIPS:
* For MIPS, the macros `SR_IBIT1' - `SR_IBIT8' define bits that may be set.
*
* RETURNS: OK or ERROR. (MIPS: The previous contents of the status register).
*/

int intDisable
    (
    int level	  /* new interrupt bits (0x0 - 0xff00) */
    )

    {
    ...
    }


/*******************************************************************************
*
* intCRGet - read the contents of the cause register (MIPS)
*
* This routine reads and returns the contents of the MIPS cause
* register.
*
* RETURNS: The contents of the cause register.
*/

int intCRGet (void)

    {
    ...
    }



/*******************************************************************************
*
* intCRSet - write the contents of the cause register (MIPS)
*
* This routine writes the contents of the MIPS cause register.
*
* RETURNS: N/A
*/

void intCRSet
    (
    int value      /* value to write to cause register */
    )

    {
    ...
    }


/*******************************************************************************
*
* intSRGet - read the contents of the status register (MIPS)
*
* This routine reads and returns the contents of the MIPS status
* register.
*
* RETURNS: The previous contents of the status register.
*/

int intSRGet (void)

    {
    ...
    }


/*******************************************************************************
*
* intSRSet - update the contents of the status register (MIPS)
*
* This routine updates and returns the previous contents of the MIPS
* status register.
*
* RETURNS: The previous contents of the status register.
*/

int intSRSet
    (
    int value	  /* value to write to status register */
    )

    {
    ...
    }

/*******************************************************************************
*
* intConnect - connect a C routine to a hardware interrupt
*
* This routine connects a specified C routine to a specified interrupt
* vector.  The address of <routine> is generally stored at <vector> so
* that <routine> is called with <parameter> when the interrupt occurs.
* The routine is invoked in supervisor mode at interrupt level.  A proper
* C environment is established, the necessary registers saved, and the
* stack set up.
*
* The routine can be any normal C code, except that it must not invoke
* certain operating system functions that may block or perform I/O
* operations.
*
* This routine generally simply calls intHandlerCreate() and
* intVecSet().  The address of the handler returned by intHandlerCreate()
* is what actually goes in the interrupt vector.
*
* This routine takes an interrupt vector as a parameter, which is the byte
* offset into the vector table. Macros are provided to convert between interrupt
* vectors and interrupt numbers, see `intArchLib'.
*
* NOTE ARM:
* ARM processors generally do not have on-chip interrupt controllers.
* Control of interrupts is a BSP-specific matter. This routine calls a
* BSP-specific routine to install the handler such that, when the
* interrupt occurs, <routine> is called with <parameter>.
*
* NOTE X86:
* Refer to the special x86 routine intHandlerCreateI86().
* 
* NOTE SH:
* The on-chip interrupt controller (INTC) design of SH architecture depends
* on the processor type, but there are some similarities.  The number of
* external interrupt inputs are limited, so it may necessary to multiplex
* some interrupt requests.  However most of them are auto-vectored, thus have
* only one vector to an external interrupt input.  As a framework to handle
* this type of multiplexed interrupt, you can use your original intConnect
* code by hooking it to _func_intConnectHook pointer.  If _func_intConnectHook
* is set, the SH version of intConnect() simply calls the hooked routine with
* same arguments, then returns the status of hooked routine.  A sysLib sample
* is shown below:
*
* .CS
* #include "intLib.h"
* #include "iv.h"		/@ INUM_INTR_HIGH for SH7750/SH7700 @/
*
* #define SYS_INT_TBL_SIZE	(255 - INUM_INTR_HIGH)
*
* typedef struct
*     {
*     VOIDFUNCPTR routine;	/@ routine to be called @/
*     int         parameter;	/@ parameter to be passed @/
*     } SYS_INT_TBL;
*
* LOCAL SYS_INT_TBL sysIntTbl [SYS_INT_TBL_SIZE]; /@ local vector table @/
*
* LOCAL int sysInumVirtBase = INUM_INTR_HIGH + 1;
*
* STATUS sysIntConnect
*     (
*     VOIDFUNCPTR *vec,		/@ interrupt vector to attach to     @/
*     VOIDFUNCPTR routine,	/@ routine to be called              @/
*     int         param		/@ parameter to be passed to routine @/
*     )
*     {
*     FUNCPTR intDrvRtn;
*  
*     if (vec >= INUM_TO_IVEC (0) && vec < INUM_TO_IVEC (sysInumVirtBase))
*         {
*         /@ do regular intConnect() process @/
*
*         intDrvRtn = intHandlerCreate ((FUNCPTR)routine, param);
*  
*         if (intDrvRtn == NULL)
*             return ERROR;
*  
*         /@ make vector point to synthesized code @/
*  
*         intVecSet ((FUNCPTR *)vec, (FUNCPTR)intDrvRtn);
*         }
*     else
*         {
*         int index = IVEC_TO_INUM (vec) - sysInumVirtBase;
*  
*         if (index < 0 || index >= SYS_INT_TBL_SIZE)
*             return ERROR;
*  
*         sysIntTbl [index].routine   = routine;
*         sysIntTbl [index].parameter = param;
*         }
*  
*     return OK;
*     }
*
* void sysHwInit (void)
*     {
*     ...
*     _func_intConnectHook = (FUNCPTR)sysIntConnect;
*     }
*
* LOCAL void sysVmeIntr (void)
*     {
*     volatile UINT32 vec = *VME_VEC_REGISTER;	/@ get VME interrupt vector @/
*     int i = vec - sysInumVirtBase;
*  
*     if (i >= 0 && i < SYS_INT_TBL_SIZE && sysIntTbl[i].routine != NULL)
*         (*sysIntTbl[i].routine)(sysIntTbl[i].parameter);
*     else
*         logMsg ("uninitialized VME interrupt: vec = %d\n", vec,0,0,0,0,0);
*     }
*
* void sysHwInit2 (void)
*     {
*     int i;
*     ...
*     /@ initialize VME interrupts dispatch table @/
*
*     for (i = 0; i < SYS_INT_TBL_SIZE; i++)
*         {
*         sysIntTbl[i].routine   = (VOIDFUNCPTR)NULL;
*         sysIntTbl[i].parameter = NULL;
*         }
*
*     /@ connect generic VME interrupts handler @/
*
*     intConnect (INT_VEC_VME, sysVmeIntr, NULL);
*     ...
*     }
* .CE
*
* The used vector numbers of SH processors are limited to certain ranges,
* depending on the processor type. The `sysInumVirtBase' should be initialized
* to a value higher than the last used vector number, defined as INUM_INTR_HIGH.
* It is typically safe to set `sysInumVirtBase' to (INUM_INTR_HIGH + 1).
*
* The sysIntConnect() routine simply acts as the regular intConnect() if 
* <vector> is smaller than INUM_TO_IVEC (sysInumVirtBase), so sysHwInit2() 
* connects a common VME interrupt dispatcher `sysVmeIntr' to the multiplexed
* interrupt vector. If <vector> is equal to or greater than INUM_TO_IVEC 
* (sysInumVirtBase), the sysIntConnect() fills a local vector entry in 
* sysIntTbl[] with an individual VME interrupt handler, in a coordinated 
* manner with `sysVmeIntr'.
*
* RETURNS: OK, or ERROR if the interrupt handler cannot be built.
*
* SEE ALSO: intHandlerCreate(), intVecSet()
*/

STATUS intConnect
    (
    VOIDFUNCPTR *  vector,      /* interrupt vector to attach to     */
    VOIDFUNCPTR    routine,     /* routine to be called              */
    int            parameter    /* parameter to be passed to routine */
    )

    {
    ...
    }


/*******************************************************************************
*
* intHandlerCreate - construct an interrupt handler for a C routine (MC680x0, x86, MIPS, SimSolaris)
*
* This routine builds an interrupt handler around the specified C routine.
* This interrupt handler is then suitable for connecting to a specific
* vector address with intVecSet().  The interrupt handler is invoked in
* supervisor mode at interrupt level.  A proper C environment is
* established, the necessary registers saved, and the stack set up.
*
* The routine can be any normal C code, except that it must not invoke
* certain operating system functions that may block or perform I/O
* operations.
*
* RETURNS: A pointer to the new interrupt handler, or NULL if memory 
* is insufficient.
* 
*/

FUNCPTR intHandlerCreate
    (
    FUNCPTR  routine,     /* routine to be called              */
    int      parameter    /* parameter to be passed to routine */
    )

    {
    ...
    }

/*******************************************************************************
*
* intLockLevelSet - set the current interrupt lock-out level (MC680x0, x86, ARM, SH, SimSolaris, SimNT)
* 
* This routine sets the current interrupt lock-out level and stores it
* in the globally accessible variable `intLockMask'.  The specified
* interrupt level is masked when interrupts are locked by
* intLock().  The default lock-out level (MC680x0 = 7,
* x86 = 1, SH = 15) is initially set by kernelInit() when
* VxWorks is initialized.
*
* NOTE SIMSOLARIS, SIMNT:
* This routine does nothing.
*
* NOTE ARM:
* On the ARM, this call establishes the interrupt level to be set when
* intLock() is called.
*
* RETURNS: N/A
* 
* SEE ALSO: intLockLevelGet(), intLock(), taskLock()
*/

void intLockLevelSet
    (
    int  newLevel        /* new interrupt level */
    )

    {
    ...
    }

/*******************************************************************************
*
* intLockLevelGet - get the current interrupt lock-out level (MC680x0, x86, ARM, SH, SimSolaris, SimNT)
* 
* This routine returns the current interrupt lock-out level, which is
* set by intLockLevelSet() and stored in the globally accessible
* variable `intLockMask'.  This is the interrupt level currently
* masked when interrupts are locked out by intLock().  The default
* lock-out level (MC680x0 = 7, x86 = 1, SH = 15)
* is initially set by kernelInit() when VxWorks is initialized.
*
* NOTE SIMNT:
* This routine does nothing.
*
* RETURNS: The interrupt level currently stored in the interrupt
* lock-out mask. (ARM = ERROR always)
*
* SEE ALSO: intLockLevelSet()
*/

int intLockLevelGet (void)

    {
    ...
    }

/*******************************************************************************
*
* intVecBaseSet - set the vector (trap) base address (MC680x0, x86, MIPS, ARM, SimSolaris, SimNT)
*
* This routine sets the vector (trap) base address.  The CPU's vector base
* register is set to the specified value, and subsequent calls to intVecGet()
* or intVecSet() will use this base address.  The vector base address is
* initially 0, until modified by calls to this routine.
*
* \"NOTE SPARC:
* \"On SPARC processors, the vector base address must be on a 4 Kbyte boundary
* \"(that is, its bottom 12 bits must be zero).
* \"
* NOTE 68000:
* The 68000 has no vector base register; thus, this routine is a no-op for
* 68000 systems.
*
* \"NOTE I960:
* \"This routine is a no-op for i960 systems.  The interrupt vector table is
* \"located in sysLib, and moving it by intVecBaseSet() would require
* \"resetting the processor.  Also, the vector base is cached on-chip in the
* \"PRCB and thus cannot be set from this routine.
* \"
* NOTE MIPS:
* The MIPS processors have no vector base register;
* thus this routine is a no-op for this architecture.
*
* NOTE SH77XX:
* This routine sets <baseAddr> to vbr, then loads an interrupt dispatch
* code to (vbr + 0x600).  When SH77XX processor accepts an interrupt request,
* it sets an exception code to INTEVT register and jumps to (vbr + 0x600).
* Thus this dispatch code is commonly used for all interrupts' handling.
*
* The exception codes are 12bits width, and interleaved by 0x20.  VxWorks
* for SH77XX locates a vector table at (vbr + 0x800), and defines the vector
* offsets as (exception codes / 8).  This vector table is commonly used by
* all interrupts, exceptions, and software traps.
*
* All SH77XX processors have INTEVT register at address 0xffffffd8.  The SH7707
* processor has yet another INTEVT2 register at address 0x04000000, to identify
* its enhanced interrupt sources.  The dispatch code obtains the address
* of INTEVT register from a global constant `intEvtAdrs'.  The constant is
* defined in `sysLib', thus the selection of INTEVT/INTEVT2 is configurable
* at BSP level.  The `intEvtAdrs' is loaded to (vbr + 4) by intVecBaseSet().
*
* After fetching the exception code, the interrupt dispatch code applies
* a new interrupt mask to the status register, and jumps to an individual
* interrupt handler.  The new interrupt mask is taken from `intPrioTable[]',
* which is defined in `sysALib'.  The `intPrioTable[]' is loaded to
* (vbr + 0xc00) by intVecBaseSet().
*
* NOTE ARM:
* The ARM processors have no vector base register;
* thus this routine is a no-op for this architecture.
*
* NOTE SIMSOLARIS, SIMNT:
* This routine does nothing.
*
* RETURNS: N/A
*
* SEE ALSO: intVecBaseGet(), intVecGet(), intVecSet()
*/

void intVecBaseSet
    (
    FUNCPTR *  baseAddr     /* new vector (trap) base address */
    )

    {
    ...
    }

/*******************************************************************************
*
* intVecBaseGet - get the vector (trap) base address (MC680x0, x86, MIPS, ARM, SimSolaris, SimNT)
*
* This routine returns the current vector base address, which is set
* with intVecBaseSet().
*
* RETURNS: The current vector base address
* (MIPS = 0 always, ARM = 0 always, SimSolaris = 0 always and 
* SimNT = 0 always).
*
* SEE ALSO: intVecBaseSet()
*/

FUNCPTR *intVecBaseGet (void)

    {
    ...
    }

/******************************************************************************
*
* intVecSet - set a CPU vector (trap) (MC680x0, x86, MIPS, SH, SimSolaris, SimNT)
*
* This routine attaches an exception/interrupt/trap handler to a specified 
* vector.  The vector is specified as an offset into the CPU's vector table. 
* This vector table starts, by default, at:
* 
* .TS
* tab(|);
* l l.
*     MC680x0:     | 0
*     MIPS:        | `excBsrTbl' in excArchLib
*     x86:         | 0
*     SH702x/SH703x/SH704x/SH76xx: | `excBsrTbl' in excArchLib
*     SH77xx:      | vbr + 0x800
*     SimSolaris:  | 0
* .TE
*
* However, the vector table may be set to start at any address with
* intVecBaseSet() (on CPUs for which it is available).  The vector table is
* set up in usrInit().
*
* This routine takes an interrupt vector as a parameter, which is the byte
* offset into the vector table. Macros are provided to convert between interrupt
* vectors and interrupt numbers, see `intArchLib'.
*
* \"NOTE SPARC:
* \"This routine generates code to:
* \".IP (1) 4
* \"save volatile registers;
* \".IP (2)
* \"fix possible window overflow;
* \".IP (3)
* \"read the processor state register into register %L0; and 
* \".IP (4)
* \"jump to the specified address.
* \".LP
* \"
* The intVecSet() routine puts this generated code into the trap table
* entry corresponding to <vector>.
* 
* Window overflow and window underflow are sacred to
* the kernel and may not be pre-empted.  They are written here
* only to track changing trap base registers (TBRs).
* With the "branch anywhere" scheme (as opposed to the branch PC-relative
* +/-8 megabytes) the first instruction in the vector table must not be a 
* change of flow control nor affect any critical registers.  The JMPL that 
* replaces the BA will always execute the next vector's first instruction.
*
* \"NOTE I960:
* \"Vectors 0-7 are illegal vectors; using them puts the vector into the
* \"priorities/pending portion of the table, which yields undesirable
* \"actions.  The i960CA caches the NMI vector in internal RAM at system
* \"power-up.  This is where the vector is taken when the NMI occurs.  Thus, it
* \"is important to check to see if the vector being changed is the NMI
* \"vector, and, if so, to write it to internal RAM.
* \"
* NOTE MIPS:
* On MIPS CPUs the vector table is set up statically in software.
*
* NOTE SH77XX:
* The specified interrupt handler <function> has to coordinate with an interrupt
* stack frame which is specially designed for SH77XX version of VxWorks:
*
*.CS
*    [ task's stack ]       [ interrupt stack ]
*
*	|  xxx  | high address
*	|  yyy  |		+-------+
*	|__zzz__|<--------------|task'sp|  0
*	|	|		|INTEVT | -4
*	|	| low address	|  ssr  | -8
*				|_ spc _| -12 <- sp (non-nested interrupt)
*				:	:
*				:	:
*				:_______:
*                               |INTEVT |  0
*                               |  ssr  | -4
*                               |_ spc _| -8  <- sp (nested interrupt)
*                               |       |
*.CE
*
* This interrupt stack frame is formed by a common interrupt dispatch code
* which is loaded at (vbr + 0x600).  You usually do not have to pay any
* attention to this stack frame, since intConnect() automatically appends
* an appropriate stack manipulation code to your interrupt service routine.
* The intConnect() assumes that your interrupt service routine (ISR) is
* written in C, thus it also wraps your ISR in minimal register save/restore
* codes.  However if you need a very fast response time to a particular
* interrupt request, you might want to skip this register save/restore
* sequence by directly attaching your ISR to the corresponding vector table
* entry using intVecSet().  Note that this technique is only applicable to
* an interrupt service with NO VxWorks system call.  For example it is not
* allowed to use semGive() or logMsg() in the interrupt service routine which
* is directly attached to vector table by intVecSet().  To facilitate the
* direct usage of intVecSet() by user, a special entry point to exit an
* interrupt context is provided within the SH77XX version of VxWorks kernel.
* This entry point is located at address (vbr + intRte1W), here the intRte1W
* is a global symbol for the vbr offset of the entry point in 16 bit length.
* This entry point `intRte1' assumes that the current register bank is 0
* (SR.RB == 0), and r1 and r0 are still saved on the interrupt stack, and
* it also requires 0x70000000 in r0. Then `intRte1' properly cleans up the
* interrupt stack and executes <rte> instruction to return to the previous
* interrupt or task context.  The following code is an example of `intRte1'
* usage.  Here the corresponding intPrioTable[] entry is assumed to be
* 0x400000X0, namely MD=1, RB=0, BL=0 at the beginning of `usrIsr1'.
*
*.CS
*	.text
*	.align	2
*	.global	_usrIsr1
*	.type	_usrIsr1,@function
*	.extern	_usrRtn
*	.extern intRte1W
*				/@ intPrioTable[] sets SR to 0x400000X0 @/
* _usrIsr1:
*	mov.l	r0,@-sp		/@ must save r0 first (BANK0) @/
*	mov.l	r1,@-sp		/@ must save r1 second (BANK0) @/
*
*	mov.l	r2,@-sp		/@ save rest of volatile registers (BANK0) @/
*	mov.l	r3,@-sp
*	mov.l	r4,@-sp
*	mov.l	r5,@-sp
*	mov.l	r6,@-sp
*	mov.l	r7,@-sp
*	sts.l	pr,@-sp
*	sts.l	mach,@-sp
*	sts.l	macl,@-sp
*
*	mov.l	UsrRtn,r0
*	jsr	@r0		/@ call user's C routine @/
*	nop			/@ (delay slot) @/
*
*	lds.l	@sp+,macl	/@ restore volatile registers (BANK0) @/
*	lds.l	@sp+,mach
*	lds.l	@sp+,pr
*	mov.l	@sp+,r7
*	mov.l	@sp+,r6
*	mov.l	@sp+,r5
*	mov.l	@sp+,r4
*	mov.l	@sp+,r3
*	mov.l	@sp+,r2
*				/@ intRte1 restores r1 and r0 @/
*	mov.l	IntRte1W,r1
*	mov.w	@r1,r0
*	stc	vbr,r1
*	add	r0,r1
*	mov.l	IntRteSR,r0	/@ r0: 0x70000000 @/
*	jmp	@r1		/@ let intRte1 clean up stack, then rte @/
*	nop			/@ (delay slot) @/
*
*		.align	2
* UsrRtn:	.long	_usrRtn		/@ user's C routine @/
* IntRteSR:	.long	0x70000000	/@ MD=1, RB=1, BL=1 @/
* IntRte1W:	.long	intRte1W
* .CE
*
* The `intRte1' sets r0 to status register (SR: 0x70000000), to safely restore
* SPC/SSR and to clean up the interrupt stack.  Note that TLB mishit exception
* immediately reboots CPU while SR.BL=1.  To avoid this fatal condition, VxWorks
* loads the `intRte1' code and the interrupt stack to a physical address space
* (P1) where no TLB mishit happens.
*
* Furthermore, there is another special entry point called `intRte2' at an
* address (vbr + intRte2W).  The `intRte2' assumes that SR is already set to
* 0x70000000 (MD: 1, RB: 1, BL: 1), then it does not restore r1 and r0.
* While SR value is 0x70000000, you may use r0,r1,r2,r3 in BANK1 as volatile
* registers.  The rest of BANK1 registers (r4,r5,r6,r7) are non-volatile, so
* if you need to use them then you have to preserve their original values by
* saving/restoring them on the interrupt stack.  So, if you need the ultimate
* interrupt response time, you may set the corresponding intPrioTable[] entry
* to NULL and manage your interrupt service only with r0,r1,r2,r3 in BANK1
* as shown in the next sample code:
*
* .CS
*	.text
*	.global  _usrIsr2
*	.type    _usrIsr2,@function
*	.extern  _usrIntCnt	/@ interrupt counter @/
*	.extern  intRte2W
*	.align   2
*				/@ MD=1, RB=1, BL=1, since SR is not @/
*				/@ substituted from intPrioTable[].  @/
* _usrIsr2:
*	mov.l	UsrIntAck,r1
*	mov	#0x1,r0
*	mov.b	r0,@r1		/@ acknowledge interrupt @/
*
*	mov.l	UsrIntCnt,r1
*	mov.l	X1FFFFFFF,r2
*	mov.l	X80000000,r3
*	and	r2,r1
*	or	r3,r1		/@ r1: _usrIntCnt address in P1 @/
*	mov.l	@r1,r0
*	add	#1,r0
*	mov.l	r0,@r1		/@ increment counter @/
*
*	mov.l	IntRte2W,r1
*	and	r2,r1
*	or	r3,r1		/@ r1: intRte2W address in P1 @/
*	mov.w	@r1,r0
*	stc	vbr,r1
*	add	r1,r0
*	jmp	@r0		/@ let intRte2 clean up stack, then rte @/
*	nop			/@ (delay slot) @/
*
*		.align	2
* UsrIntAck:	.long	0xa0001234	/@ interrupt acknowledge register @/
* UsrIntCnt:	.long	_usrIntCnt
* IntRte2W:	.long	intRte2W
* X1FFFFFFF:	.long	0x1fffffff
* X80000000:	.long	0x80000000
* .CE
*
* Note that the entire interrupt service is executed under SR.BL=1 in this
* sample code.  It means that any access to virtual address space may reboot
* CPU, since TLB mishit exception is blocked.  Therefore `usrIsr2' has to
* access `usrIntCnt' and `intRte2W' from P1 region.  Also `usrIsr2' itself
* has to be executed on P1 region, and it can be done by relocating the address
* of `usrIsr2' to P1 as shown below:
*
* .CS
* IMPORT void usrIsr2 (void);
*
* intVecSet (vector, (FUNCPTR)(((UINT32)usrIsr2 & 0x1fffffff) | 0x80000000));
* .CE
*
* In conclusion, you have to guarantee that the entire ISR does not access to
* any virtual address space if you set the corresponding intPrioTable[] entry
* to NULL.
* 
* NOTE SIMNT:
* This routine does nothing.
*
* RETURNS: N/A
*
* SEE ALSO: intVecBaseSet(), intVecGet()
*/

void intVecSet
    (
    FUNCPTR *	vector,		/* vector offset              */
    FUNCPTR	function	/* address to place in vector */
    )

    {
    ...
    }

/*******************************************************************************
*
* intVecGet - get an interrupt vector (MC680x0, x86, MIPS, SH, SimSolaris, SimNT)
*
* This routine returns a pointer to the exception/interrupt handler attached
* to a specified vector.  The vector is specified as an offset into the CPU's
* vector table.  This vector table starts, by default, at:
* 
* .TS
* tab(|);
* l l.
*     MC680x0:     | 0
*     MIPS:        | `excBsrTbl' in excArchLib
*     x86:         | 0
*     SH702x/SH703x/SH704x/SH76xx: | `excBsrTbl' in excArchLib
*     SH77xx:      | vbr + 0x800
*     SimSolaris:  | 0
* .TE
*
* However, the vector table may be set to start at any address with
* intVecBaseSet() (on CPUs for which it is available).
*
* This routine takes an interrupt vector as a parameter, which is the byte
* offset into the vector table. Macros are provided to convert between interrupt
* vectors and interrupt numbers, see `intArchLib'.
*
* \"NOTE I960:
* \"The interrupt table location is reinitialized to <sysIntTable> after
* \"booting.  This location is returned by intVecBaseGet().
* \"
* NOTE SIMNT:
* This routine does nothing and always returns 0.
*
* RETURNS:
* A pointer to the exception/interrupt handler attached to the specified vector.
*
* SEE ALSO: intVecSet(), intVecBaseSet()
*/

FUNCPTR intVecGet
    (
    FUNCPTR *  vector     /* vector offset */
    )

    {
    ...
    }


/*******************************************************************************
*
* intVecTableWriteProtect - write-protect exception vector table (MC680x0, x86, ARM, SimSolaris, SimNT)
*
* If the unbundled Memory Management Unit (MMU) support package (VxVMI) is
* present, this routine write-protects the exception vector table to
* protect it from being accidentally corrupted.
*
* Note that other data structures contained in the page will also be 
* write-protected.  In the default VxWorks configuration, the exception vector
* table is located at location 0 in memory.  Write-protecting this affects
* the backplane anchor, boot configuration information, and potentially the
* text segment (assuming the default text location of 0x1000.)  All code
* that manipulates these structures has been modified to write-enable 
* memory for the duration of the operation.  If you select a different
* address for the exception vector table, be sure it resides in a page
* separate from other writable data structures.
*
* NOTE SIMSOLARIS, SIMNT:
* This routine always returns ERROR on simulators.
*
* RETURNS: OK, or ERROR if memory cannot be write-protected.
*
* ERRNO: S_intLib_VEC_TABLE_WP_UNAVAILABLE
*/

STATUS intVecTableWriteProtect (void)

    {
    ...
    }


/********************************************************************************
* intUninitVecSet - set the uninitialized vector handler (ARM)
*
* This routine installs a handler for the uninitialized vectors to be
* called when any uninitialised vector is entered.
*
* RETURNS: N/A.
*/

void intUninitVecSet
    (
    VOIDFUNCPTR routine /* ptr to user routine */
    )

    {
    ...
    }


/*******************************************************************************
*
* intHandlerCreateI86 - construct an interrupt handler for a C routine (x86)
*
* This routine builds an interrupt handler around a specified C routine.
* This interrupt handler is then suitable for connecting to a specific
* vector address with intVecSet().  The interrupt handler is invoked in
* supervisor mode at interrupt level.  A proper C environment is
* established, the necessary registers saved, and the stack set up.
* 
* The routine can be any normal C code, except that it must not invoke
* certain operating system functions that may block or perform I/O
* operations.
*
* IMPLEMENTATION:
* This routine builds an interrupt handler of the following form in
* allocated memory:
*
* .CS
* 00  e8 kk kk kk kk		call	_intEnt		* tell kernel
* 05  50			pushl	%eax		* save regs
* 06  52			pushl	%edx
* 07  51			pushl	%ecx
* 08  68 pp pp pp pp		pushl	$_parameterBoi	* push BOI param
* 13  e8 rr rr rr rr		call	_routineBoi	* call BOI routine
* 18  68 pp pp pp pp		pushl	$_parameter	* push param
* 23  e8 rr rr rr rr		call	_routine	* call C routine
* 28  68 pp pp pp pp		pushl	$_parameterEoi	* push EOI param
* 33  e8 rr rr rr rr		call	_routineEoi	* call EOI routine
* 38  83 c4 0c			addl	$12, %esp	* pop param
* 41  59			popl	%ecx		* restore regs
* 42  5a			popl	%edx
* 43  58			popl	%eax
* 44  e9 kk kk kk kk		jmp	_intExit	* exit via kernel
* .CE
* 
* Third and fourth parameter of intHandlerCreateI86() are the BOI routine 
* address and its parameter that are inserted into the code as "routineBoi" 
* and "parameterBoi". 
* Fifth and sixth parameter of intHandlerCreateI86() are the EOI routine 
* address and its parameter that are inserted into the code as "routineEoi" 
* and "parameterEoi". 
* The BOI routine detects if this interrupt is stray/spurious/phantom by
* interrogating the interrupt controller, and returns from the interrupt
* if it is.  The EOI routine issues End Of Interrupt signal to the 
* interrupt controller, if it is required by the controller.  
* Each interrupt controller has its own BOI and EOI routine.  They are
* located in the BSP, and their address and parameter are taken by the
* intEoiGet function (set to sysIntEoiGet() in the BSP).
* The Tornado 2, and later, BSPs should use the BOI and EOI mechanism with
* intEoiGet function pointer.
*
* To keep the Tornado 101 BSP backward compatible, the function pointer 
* intEOI is not removed.  If intEoiGet is NULL, it should be set to the
* sysIntEoiGet() routine in the BSP, intHandlerCreate() and the intEOI 
* function pointer (set to sysIntEOI() in the Tornado 101 BSP) is used.
* 
* RETURNS: A pointer to the new interrupt handler, or NULL if memory
* is insufficient.
*/

FUNCPTR intHandlerCreateI86
    (
    FUNCPTR routine,		/* routine to be called              */
    int parameter,		/* parameter to be passed to routine */
    FUNCPTR routineBoi,		/* BOI routine to be called          */
    int parameterBoi,		/* parameter to be passed to routineBoi */
    FUNCPTR routineEoi,		/* EOI routine to be called          */
    int parameterEoi		/* parameter to be passed to routineEoi */
    )
    {
    ...
    }


/******************************************************************************
*
* intVecSet2 - set a CPU vector, gate type(int/trap), and selector (x86)
*
* This routine attaches an exception handler to a specified vector,
* with the type of the gate and the selector of the gate.  
* The vector is specified as an offset into the CPU's vector table.  This
* vector table starts, by default, at address 0.  
* However, the vector table may be set to start at any address with
* intVecBaseSet().  The vector table is set up in usrInit().
*
* RETURNS: N/A
*
* SEE ALSO: intVecBaseSet(), intVecGet(), intVecSet(), intVecGet2()
*/

void intVecSet2
    (
    FUNCPTR * vector,		/* vector offset              */
    FUNCPTR function,		/* address to place in vector */
    int idtGate,		/* IDT_TRAP_GATE or IDT_INT_GATE */
    int idtSelector		/* sysCsExc or sysCsInt */
    )
    {
    ...
    }


/******************************************************************************
*
* intVecGet2 - get a CPU vector, gate type(int/trap), and gate selector (x86)
*
* This routine gets a pointer to the exception/interrupt handler attached
* to a specified vector, the type of the gate, the selector of the gate.  
* The vector is specified as an offset into the CPU's vector table.  
* This vector table starts, by default, at address 0.
* However, the vector table may be set to start at any address with
* intVecBaseSet().
*
* RETURNS: N/A
*
* SEE ALSO: intVecBaseSet(), intVecGet(), intVecSet(), intVecSet2()
*/

void intVecGet2
    (
    FUNCPTR * vector,		/* vector offset              */
    FUNCPTR * pFunction,	/* address to place in vector */
    int *     pIdtGate,		/* IDT_TRAP_GATE or IDT_INT_GATE */
    int *     pIdtSelector	/* sysCsExc or sysCsInt */
    )
    {
    ...
    }

/*******************************************************************************
*
* intStackEnable - enable or disable the interrupt stack usage (x86)
*
* This routine enables or disables the interrupt stack usage and is only 
* callable from the task level. An Error is returned for any other calling 
* context. The interrupt stack usage is disabled in the default configuration
* for the backward compatibility.  Routines that manipulate the interrupt
* stack, are located in the file i86/windALib.s. These routines include
* intStackEnable(), intEnt() and intExit().
*
* RETURNS: OK, or ERROR if it is not in the task level.
*/

STATUS intStackEnable 
    (
    BOOL enable		/* TRUE to enable, FALSE to disable */
    )
    {
    ...
    }

