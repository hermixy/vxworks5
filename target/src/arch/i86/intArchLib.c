/* intArchLib.c - I80x86 interrupt subroutine library */

/* Copyright 1984-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01p,20nov01,hdn  doc clean up for 5.5
01o,09nov01,hdn  replaced magic numbers with intConnectCode offset macros
01n,29aug01,hdn  got rid of VXM stuff.
		 added intVecGet2() correspond to intVecSet2().
		 replaced {exc,int}IdtSelector with sysCs{Exc,Int}.
		 added document for intHandlerCreateI86() (spr 30292)
01m,14dec00,pai  Added stub routines for intEnable() and intDisable() (SPR
                 #63046).
01l,09feb99,wsl  add comment to document ERRNO value
01k,29may98,hdn  added intHandlerCreateX86(), modified intConnectCode[]
		 to solve EOI issue and sprious interrupt issue.
		 removed intEOI.  added intEoiGet.
01j,13apr98,hdn  changed malloc to memalign in intHandlerCreate().
01i,14jan98,dbt  added dbgLib.h include
01h,23aug95,ms   removed taskSafe/Unsafe calls from intVecSet().
01g,15jun95,ms   added intRegsLock, intRegsUnlock
01f,14jun95,hdn  added intVecSetEnt and intVecSetExit.
		 renamed pSysEndOfInt to intEOI.
01e,26jan95,rhp  doc tweaks
01d,29jul93,hdn  added intVecTableWriteProtect().
		  - intVecSet checks taskIdCurrent before TASK_{SAFE,UNSAFE}.
		  - deleted mention of intContext() and intCount().
		  - added vxmIfVecxxx callout for monitor support.
01c,03jun93,hdn  updated to 5.1
		  - changed functions to ansi style
		  - changed includes to have absolute path from h/
		  - fixed #else and #endif
		  - changed VOID to void
		  - changed copyright notice
01b,15oct92,hdn  supported nested interrupt.
01a,28feb92,hdn  written based on TRON, 68k version.
*/

/*
DESCRIPTION
This library provides architecture-dependent routines to manipulate
and connect to hardware interrupts.  Any C language routine can be
connected to any interrupt by calling intConnect().  Vectors can be
accessed directly by intVecSet() and intVecGet(), or by intVecSet2()
and intVecGet2().  The vector (trap) base register can be accessed 
by the routines intVecBaseSet() and intVecBaseGet().

Tasks can lock and unlock interrupts by calling intLock() and intUnlock().
The lock-out level can be set and reported by intLockLevelSet() and
intLockLevelGet().

WARNING
Do not call VxWorks system routines with interrupts locked.
Violating this rule may re-enable interrupts unpredictably.

INTERRUPT VECTORS AND NUMBERS
Most of the routines in this library take an interrupt vector as a
parameter, which is the byte offset into the vector table.  Macros are
provided to convert between interrupt vectors and interrupt numbers:
.iP IVEC_TO_INUM(intVector) 10
changes a vector to a number.
.iP INUM_TO_IVEC(intNumber)
turns a number into a vector.
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
    intVecSet (vector, oldfunc);    /@ use original routine  @/
        ...
    intVecSet (vector, newfunc);    /@ reconnect new routine @/
.CE

INCLUDE FILE: iv.h

SEE ALSO: intLib, intALib
*/

/* LINTLIBRARY */

#include "vxWorks.h"
#include "cacheLib.h"
#include "errnoLib.h"
#include "intLib.h"
#include "memLib.h"
#include "sysLib.h"
#include "taskLib.h"
#include "string.h"
#include "stdlib.h"
#include "dbgLib.h"
#include "private/vmLibP.h"


/* imports */

IMPORT void intVBRSet (FUNCPTR *baseAddr);
IMPORT void intEnt ();				/* interrupt entrance stub */
IMPORT void intExit ();				/* interrupt exit stub */
IMPORT int sysCsExc;
IMPORT int sysCsInt;


/* globals */

/* The routine intLock(), found in intALib.s uses intLockMask to construct a
 * new EFLAGS with the correct interrupt lock-out level.  The difficulty is
 * intLock() may be called from either interrupt level, or task level, so
 * simply reserving a EFLAGS such as 0x80000000 does not work because such a 
 * EFLAGS would assume task-level code.
 */

UINT intLockMask	  = 0x0;	/* interrupt lock mask - level 0 */
VOIDFUNCPTR intVecSetEnt  = NULL;	/* entry hook for intVecSet() */
VOIDFUNCPTR intVecSetExit = NULL;	/* exit  hook for intVecSet() */
VOIDFUNCPTR intEOI	  = NULL;	/* pointer to EOI routine in BSP */
VOIDFUNCPTR intEoiGet	  = NULL;	/* pointer to EoiGet routine in BSP */

LOCAL int  dummy (void) { return ERROR; }  /* dummy, returns ERROR   */
FUNCPTR    sysIntLvlEnableRtn  = dummy;    /* enable a single level  */
FUNCPTR    sysIntLvlDisableRtn = dummy;    /* disable a single level */


/* locals */

LOCAL FUNCPTR * intVecBase = 0;		/* vector base address */

LOCAL UCHAR intConnectCode []	=	/* intConnect stub */
    {
/*
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
 */
     0xe8, 0x00, 0x00, 0x00, 0x00,	/* _intEnt filled in at runtime */
     0x50,
     0x52,
     0x51,
     0x68, 0x00, 0x00, 0x00, 0x00,	/* BOI parameter filled in at runtime */
     0xe8, 0x00, 0x00, 0x00, 0x00,	/* BOI routine filled in at runtime */
     0x68, 0x00, 0x00, 0x00, 0x00,	/* parameter filled in at runtime */
     0xe8, 0x00, 0x00, 0x00, 0x00,	/* routine filled in at runtime */
     0x68, 0x00, 0x00, 0x00, 0x00,	/* EOI parameter filled in at runtime */
     0xe8, 0x00, 0x00, 0x00, 0x00,	/* EOI routine filled in at runtime */
     0x83, 0xc4, 0x0c,			/* pop parameters */
     0x59,
     0x5a,
     0x58,
     0xe9, 0x00, 0x00, 0x00, 0x00,	/* _intExit filled in at runtime */
    };


/* forward declarations */


/*******************************************************************************
*
* intConnect - connect a C routine to a hardware interrupt
*
* This routine connects a specified C routine to a specified interrupt
* vector.  The address of <routine> is stored at <vector> so that <routine>
* is called with <parameter> when the interrupt occurs.  The routine is
* invoked in supervisor mode at interrupt level.  A proper C environment
* is established, the necessary registers saved, and the stack set up.
*
* The routine can be any normal C code, except that it must not invoke
* certain operating system functions that may block or perform I/O
* operations.
*
* This routine simply calls intHandlerCreate()/intHandlerCreateI86() and 
* intVecSet().  The address of the handler returned by intHandlerCreate()/
* intHandlerCreateI86() is what actually gets put in the interrupt vector.
*
* RETURNS:
* OK, or ERROR if the interrupt handler cannot be built.
*
* SEE ALSO: intHandlerCreate(), intVecSet(), intHandlerCreateI86()
*/

STATUS intConnect
    (
    VOIDFUNCPTR *vector,	/* interrupt vector to attach to     */
    VOIDFUNCPTR routine,	/* routine to be called              */
    int parameter		/* parameter to be passed to routine */
    )
    {
    FUNCPTR intDrvRtn;
    VOIDFUNCPTR routineBoi;
    VOIDFUNCPTR routineEoi;
    int parameterBoi;
    int parameterEoi;

    if (intEoiGet == NULL)
	{
        intDrvRtn = intHandlerCreate ((FUNCPTR)routine, parameter); 
	}
    else
	{
        (* intEoiGet) (vector, &routineBoi, &parameterBoi, 
			       &routineEoi, &parameterEoi);
        intDrvRtn = intHandlerCreateI86 ((FUNCPTR)routine, parameter,
					 (FUNCPTR)routineBoi, parameterBoi,
					 (FUNCPTR)routineEoi, parameterEoi); 
	}

    if (intDrvRtn == NULL)
	return (ERROR);

    /* make vector point to synthesized code */

    intVecSet ((FUNCPTR *)vector, (FUNCPTR)intDrvRtn);

    return (OK);
    }

/*******************************************************************************
*
* intEnable - enable a specific interrupt level
*
* Enable a specific interrupt level.  For each interrupt level to be used,
* there must be a call to this routine before it will be
* allowed to interrupt.
*
* RETURNS:
* OK or ERROR for invalid arguments.
*/
int intEnable
    (
    int level   /* level to be enabled */
    )
    {
    return (*sysIntLvlEnableRtn) (level);
    }

/*******************************************************************************
*
* intDisable - disable a particular interrupt level
*
* This call disables a particular interrupt level, regardless of the current
* interrupt mask level.
*
* RETURNS:
* OK or ERROR for invalid arguments.
*/
int intDisable
    (
    int level   /* level to be disabled */
    )
    {
    return (*sysIntLvlDisableRtn) (level);
    }

/*******************************************************************************
*
* intHandlerCreate - construct an interrupt handler for a C routine
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
* RETURNS: A pointer to the new interrupt handler, or NULL if memory
* is insufficient.
*/

FUNCPTR intHandlerCreate
    (
    FUNCPTR routine,		/* routine to be called              */
    int parameter		/* parameter to be passed to routine */
    )
    {
    FAST UCHAR *pCode;		/* pointer to newly synthesized code */
    FAST int ix;

    pCode = (UCHAR *)memalign (_CACHE_ALIGN_SIZE, sizeof (intConnectCode));

    if (pCode != NULL)
	{
	/* copy intConnectCode into new code area */

	bcopy ((char *)intConnectCode, (char *)pCode, sizeof (intConnectCode));

	/* set the addresses & instructions */

	*(int *)&pCode[ICC_INT_ENT] = (int)intEnt - 
				      (int)&pCode[ICC_INT_ENT + 4];

	for (ix = 0; ix < 10; ix++)
	    pCode[ICC_BOI_PUSH + ix] = 0x90;	/* no BOI so replace by NOP */
	pCode[ICC_ADD_N] = 8;			/* pop two parameters */

	*(int *)&pCode[ICC_INT_PARAM] = (int)parameter;
	*(int *)&pCode[ICC_INT_ROUTN] = (int)routine - 
					(int)&pCode[ICC_INT_ROUTN + 4];

	if (intEOI == NULL)
	    {
	    for (ix = 0; ix < 5; ix++)
		pCode[ICC_EOI_CALL + ix] = 0x90;	/* replace by NOP */
	    }
	else
	    {
	    *(int *)&pCode[ICC_EOI_ROUTN] = (int)intEOI - 
					    (int)&pCode[ICC_EOI_ROUTN + 4];
	    }
	*(int *)&pCode[ICC_INT_EXIT] = (int)intExit - 
				       (int)&pCode[ICC_INT_EXIT + 4];
	}

    CACHE_TEXT_UPDATE ((void *) pCode, sizeof (intConnectCode));

    return ((FUNCPTR)(int)pCode);
    }

/*******************************************************************************
*
* intHandlerCreateI86 - construct an interrupt handler for a C routine
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
    FAST UCHAR *pCode;		/* pointer to newly synthesized code */
    FAST int ix;

    pCode = (UCHAR *)memalign (_CACHE_ALIGN_SIZE, sizeof (intConnectCode));

    if (pCode != NULL)
	{
	/* copy intConnectCode into new code area */

	bcopy ((char *)intConnectCode, (char *)pCode, sizeof (intConnectCode));

	/* set the addresses & instructions */

	*(int *)&pCode[ICC_INT_ENT] = (int)intEnt - 
				      (int)&pCode[ICC_INT_ENT + 4];

	if (routineBoi == NULL)			/* BOI routine */
	    {
	    for (ix = 0; ix < 10; ix++)
		pCode[ICC_BOI_PUSH + ix] = 0x90;	/* replace by NOP */
	    pCode[ICC_ADD_N] = 8;			/* pop two params */
	    }
	else
	    {
	    *(int *)&pCode[ICC_BOI_PARAM] = (int)parameterBoi;
	    *(int *)&pCode[ICC_BOI_ROUTN] = (int)routineBoi - 
					    (int)&pCode[ICC_BOI_ROUTN + 4];
	    }

	*(int *)&pCode[ICC_INT_PARAM] = (int)parameter;
	*(int *)&pCode[ICC_INT_ROUTN] = (int)routine - 
					(int)&pCode[ICC_INT_ROUTN + 4];

	if (routineEoi == NULL)			/* EOI routine */
	    {
	    for (ix = 0; ix < 5; ix++)
		pCode[ICC_EOI_CALL + ix] = 0x90;	/* replace by NOP */
	    }
	else
	    {
	    *(int *)&pCode[ICC_EOI_PARAM] = (int)parameterEoi;
	    *(int *)&pCode[ICC_EOI_ROUTN] = (int)routineEoi - 
					    (int)&pCode[ICC_EOI_ROUTN + 4];
	    }
	*(int *)&pCode[ICC_INT_EXIT] = (int)intExit - 
				       (int)&pCode[ICC_INT_EXIT + 4];
	}

    CACHE_TEXT_UPDATE ((void *) pCode, sizeof (intConnectCode));

    return ((FUNCPTR)(int)pCode);
    }

/*******************************************************************************
*
* intLockLevelSet - set the current interrupt lock-out level
*
* This routine sets the current interrupt lock-out level and stores it
* in the globally accessible variable `intLockMask'.  The specified
* interrupt level is masked when interrupts are locked by
* intLock().  The default lock-out level (1 for I80x86 processors)
* is initially set by kernelInit() when VxWorks is initialized.
* 
* RETURNS: N/A
*/

void intLockLevelSet
    (
    int newLevel		/* new interrupt level */
    )
    {
    intLockMask    = newLevel;
    }

/*******************************************************************************
*
* intLockLevelGet - get the current interrupt lock-out level
* 
* This routine returns the current interrupt lock-out level, which is
* set by intLockLevelSet() and stored in the globally accessible
* variable `intLockMask'.  This is the interrupt level currently
* masked when interrupts are locked out by intLock().  The default
* lock-out level is 1 for I80x86 processors, and is initially set by
* kernelInit() when VxWorks is initialized.
*
* RETURNS:
* The interrupt level currently stored in the interrupt lock-out mask.
*/

int intLockLevelGet (void)

    {
    return ((int)(intLockMask));
    }

/*******************************************************************************
*
* intVecBaseSet - set the vector base address
*
* This routine sets the vector base address.  The CPU's vector base register
* is set to the specified value, and subsequent calls to intVecGet() or
* intVecSet() will use this base address.  The vector base address is
* initially 0, until modified by calls to this routine.
*
* RETURNS: N/A
*
* SEE ALSO: intVecBaseGet(), intVecGet(), intVecSet()
*/

void intVecBaseSet
    (
    FUNCPTR *baseAddr	    /* new vector base address */
    )
    {
    char idt[6];		/* interrupt descriptor table register */
    char *p = idt;

    intVecBase = baseAddr;	/* keep the base address in a static variable */
    *(short *)p = 0x07ff;		/* IDT limit */
    *(int *)(p + 2) = (int)baseAddr;	/* IDT base address */

    intVBRSet ((FUNCPTR *)idt);		/* set the actual IDT register */

    CACHE_TEXT_UPDATE ((void *)baseAddr, 256 * 8);
    }

/*******************************************************************************
*
* intVecBaseGet - get the vector base address
*
* This routine returns the current vector base address, which is set
* with intVecBaseSet().
*
* RETURNS: The current vector base address.
*
* SEE ALSO: intVecBaseSet()
*/

FUNCPTR *intVecBaseGet (void)

    {
    return (intVecBase);
    }

/******************************************************************************
*
* intVecSet - set a CPU vector
*
* This routine attaches an exception/interrupt handler to a specified vector.
* The vector is specified as an offset into the CPU's vector table.  This
* vector table starts, by default, at address 0.  
* However, the vector table may be set to start at any address with
* intVecBaseSet().  The vector table is set up in usrInit().
*
* RETURNS: N/A
*
* SEE ALSO: intVecBaseSet(), intVecGet(), intVecGet2(), intVecSet2()
*/

void intVecSet
    (
    FUNCPTR *vector,		/* vector offset              */
    FUNCPTR function		/* address to place in vector */
    )
    {
    FUNCPTR *	newVector;
    UINT	state;
    BOOL	writeProtected = FALSE;
    int		pageSize = 0;
    char *	pageAddr = 0;

    if (intVecSetEnt != NULL)				/* entry hook */
	(* intVecSetEnt) (vector, function);

    /* vector is offset by the vector base address */

    newVector = (FUNCPTR *) ((int) vector + (int) intVecBaseGet ());

    /* see if we need to write enable the memory */

    if (vmLibInfo.vmLibInstalled)
	{
	pageSize = VM_PAGE_SIZE_GET();

	pageAddr = (char *) ((UINT) newVector / pageSize * pageSize);

	if (VM_STATE_GET (NULL, (void *) pageAddr, &state) != ERROR)
	    if ((state & VM_STATE_MASK_WRITABLE) == VM_STATE_WRITABLE_NOT)
		{
		writeProtected = TRUE;
		VM_STATE_SET (NULL, pageAddr, pageSize, VM_STATE_MASK_WRITABLE,
			      VM_STATE_WRITABLE);
		}

	}

    *(int *)newVector = (sysCsInt << 16) | ((int)function & 0xffff);
    *((short *)newVector + 3) = (short)(((int)function & 0xffff0000) >> 16);

    if (writeProtected)
	{
	VM_STATE_SET (NULL, pageAddr, pageSize, 
		      VM_STATE_MASK_WRITABLE, VM_STATE_WRITABLE_NOT);
	}

    if (intVecSetExit != NULL)				/* exit hook */
	(* intVecSetExit) (vector, function);

    CACHE_TEXT_UPDATE ((void *)newVector, 8);
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
    FUNCPTR *	newVector;
    UINT	state;
    BOOL	writeProtected = FALSE;
    int		pageSize = 0;
    char *	pageAddr = 0;

    if (intVecSetEnt != NULL)				/* entry hook */
	(* intVecSetEnt) (vector, function);

    /* vector is offset by the vector base address */

    newVector = (FUNCPTR *) ((int) vector + (int) intVecBaseGet ());

    /* see if we need to write enable the memory */

    if (vmLibInfo.vmLibInstalled)
	{
	pageSize = VM_PAGE_SIZE_GET();

	pageAddr = (char *) ((UINT) newVector / pageSize * pageSize);

	if (VM_STATE_GET (NULL, (void *) pageAddr, &state) != ERROR)
	    if ((state & VM_STATE_MASK_WRITABLE) == VM_STATE_WRITABLE_NOT)
		{
		writeProtected = TRUE;
		VM_STATE_SET (NULL, pageAddr, pageSize, VM_STATE_MASK_WRITABLE,
			      VM_STATE_WRITABLE);
		}

	}

    *(int *)newVector = (idtSelector << 16) | ((int)function & 0x0000ffff);
    *((int *)newVector + 1) = ((int)function & 0xffff0000) | idtGate;

    if (writeProtected)
	{
	VM_STATE_SET (NULL, pageAddr, pageSize, 
		      VM_STATE_MASK_WRITABLE, VM_STATE_WRITABLE_NOT);
	}

    if (intVecSetExit != NULL)				/* exit hook */
	(* intVecSetExit) (vector, function);

    CACHE_TEXT_UPDATE ((void *)newVector, 8);
    }

/*******************************************************************************
*
* intVecGet - get an interrupt vector
*
* This routine returns a pointer to the exception/interrupt handler attached
* to a specified vector.  The vector is specified as an offset into the CPU's
* vector table.  This vector table starts, by default, at address 0.
* However, the vector table may be set to start at any address with
* intVecBaseSet().
*
* RETURNS:
* A pointer to the exception/interrupt handler attached to the specified vector.
*
* SEE ALSO: intVecSet(), intVecBaseSet(), intVecGet2(), intVecSet2()
*/

FUNCPTR intVecGet
    (
    FUNCPTR *vector	/* vector offset */
    )
    {
    FUNCPTR *newVector;

    /* vector is offset by vector base address */

    newVector = (FUNCPTR *) ((int) vector + (int) intVecBaseGet ());
    return ((FUNCPTR)(((int)*(newVector + 1) & 0xffff0000) | 
			 ((int)*newVector & 0x0000ffff)));
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
    FUNCPTR *newVector;

    /* vector is offset by vector base address */

    newVector = (FUNCPTR *) ((int) vector + (int) intVecBaseGet ());

    /* get a pointer to the handler */

    *pFunction = ((FUNCPTR)(((int)*(newVector + 1) & 0xffff0000) | 
			    ((int)*newVector & 0x0000ffff)));

    /* get a gate selector */

    *pIdtSelector = ((int)*newVector >> 16) & 0x0000ffff;

    /* get a gate type (int/trap) */

    *pIdtGate = (int)*(newVector + 1) & 0x0000ffff;
    }

/*******************************************************************************
*
* intVecTableWriteProtect - write protect exception vector table
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
* that manipulates these structures has been modified to write-enable the
* memory for the duration of the operation.  If you select a different
* address for the exception vector table, be sure it resides in a page
* separate from other writable data structures.
* 
* RETURNS: OK, or ERROR if unable to write protect memory.
*
* ERRNO: S_intLib_VEC_TABLE_WP_UNAVAILABLE
*/

STATUS intVecTableWriteProtect
    (
    void
    )
    {
    int pageSize;
    UINT vectorPage;

    if (!vmLibInfo.vmLibInstalled)
	{
	errno = S_intLib_VEC_TABLE_WP_UNAVAILABLE;
	return (ERROR);
	}

    pageSize = VM_PAGE_SIZE_GET();

    vectorPage = (UINT) intVecBaseGet () / pageSize * pageSize;

    return (VM_STATE_SET (0, (void *) vectorPage, pageSize, 
			  VM_STATE_MASK_WRITABLE, VM_STATE_WRITABLE_NOT));
    }

/******************************************************************************
*
* intRegsLock - modify a REG_SET to have interrupts locked.
*/

int intRegsLock
    (
    REG_SET *pRegs                      /* register set to modify */
    )
    {
    int oldFlags = pRegs->eflags;
    pRegs->eflags &= ~INT_FLAG;
    return (oldFlags);
    }

/******************************************************************************
*
* intRegsUnlock - restore an REG_SET's interrupt lockout level.
*/

void intRegsUnlock
    (
    REG_SET *   pRegs,                  /* register set to modify */
    int         oldFlags                /* int lock level to restore */
    )
    {
    pRegs->eflags &= ~INT_FLAG;
    pRegs->eflags |= (oldFlags & INT_FLAG);
    }

