/* excArchLib.c - architecture-specific exception-handling facilities */

/* Copyright 1984-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01o,05jun02,wsl  remove reference to SPARC and i960
01n,20nov01,hdn  updated x86 specific sections.
01m,13nov01,hbh  Updated for simulators.
01l,29oct01,zl   updated SH specific sections.
01k,03mar00,zl   merged SH support into T2
01j,26mar97,jdi  tam  added excCrtConnect() and excIntCrtConnect() for PPC 403.
01j,27feb97,jpd  added ARM-specific documentation.
01i,23mar96,jdi  doc: edits to PowerPC additions.
01h,08mar96,kkk  added PowerPC specific routines.
01g,25nov95,jdi  removed 29k stuff.
01f,06oct95,jdi  removed .pG "Debugging".
01e,10feb95,jdi  changed 80960 to i960.
01d,30jan95,rhp  added MIPS, i386/i486 doc.
01c,02dec93,pme  added Am29K family support.
01b,20jan93,jdi  documenation cleanup.
01a,23sep92,jdi  written, based on excArchLib.c for
		 mc68k, sparc, i960, mips.
*/

/*
This library contains exception-handling facilities that are architecture
dependent.  For information about generic (architecture-independent)
exception-handling, see the manual entry for excLib.

INCLUDE FILES: excLib.h

SEE ALSO: excLib, dbgLib, sigLib, intLib
*/


/*******************************************************************************
*
* excVecInit - initialize the exception/interrupt vectors
*
* This routine sets all exception vectors to point to the appropriate
* default exception handlers.  These handlers will safely trap and report
* exceptions caused by program errors or unexpected hardware interrupts.
* .IP `MC680x0': 8
* All vectors from vector 2 (address 0x0008) to 255 (address 0x03fc) are
* initialized.  Vectors 0 and 1 contain the reset stack pointer and program
* counter.
* \".IP `SPARC':
* \"All vectors from 0 (offset 0x000) through 255 (offset 0xff0) are
* \"initialized.
* \".IP `i960':
* \"The i960 fault table is filled with
* \"a default fault handler, and all non-reserved vectors in the i960
* \"interrupt table are filled with a default interrupt handler.
* .IP `MIPS':
* All MIPS exception, trap, and interrupt vectors are set to default handlers.
* .IP `x86':
* All vectors from vector 0 (address (0x0000) to 255 (address 0x07f8) are
* initialized to default handlers.
* .IP `PowerPC':
* There are 48 vectors and only vectors that are used are initialized.
* .IP `SH':
* There are 256 vectors, initialized with the default exception handler (for 
* exceptions) or the unitialized interrupt handler (for interupts).
* On SH-2, vectors 0 and 1 contain the power-on reset program counter and 
* stack pointer. Vectors 2 and 3 contain the manual reset program counter and
* stack pointer. On SH-3 and SH-4 processors the vector table is located at 
* (vbr + 0x800), and the (exception code / 8) value is used as vector offset.
* The first two vectors are reserved for special use: "trapa #0" (offset 0x0)
* to implement software breakpoint, and "trapa #1' (offset 0x4) to detect 
* integer zero divide exception.
* .IP `ARM':
* All exception vectors are initialized to default handlers except 0x14
* (Address) which is now reserved on the ARM and 0x1C (FIQ), which is not
* used by VxWorks.
* .IP `SimSolaris/SimNT':
* This routine does nothing on both simulators and always returns OK.
* .LP
*
* NOTE
* This routine is usually called from the system start-up routine,
* usrInit(), in usrConfig.c.  It must be called before interrupts are enabled.
* \"(SPARC: It must also be called when the system runs with the on-chip windows
* \"(no stack)).
*
* RETURNS: OK, always.
*
* SEE ALSO: excLib
*/

STATUS excVecInit (void)

    {
    ...
    }

/****************************************************************************
*
* excConnect - connect a C routine to an exception vector (PowerPC)
*
* This routine connects a specified C routine to a specified exception
* vector.  An exception stub is created and in placed at <vector> in the
* exception table.  The address of <routine> is stored in the exception stub
* code.  When an exception occurs, the processor jumps to the exception stub
* code, saves the registers, and calls the C routines.
*
* The routine can be any normal C code, except that it must not
* invoke certain operating system functions that may block or perform
* I/O operations.
*
* The registers are saved to an Exception Stack Frame (ESF) placed on the
* stack of the task that has produced the exception.  The structure of the
* ESF used to save the registers is defined in h/arch/ppc/esfPpc.h.
*
* The only argument passed by the exception stub to the C routine is a pointer
* to the ESF containing the registers values.  The prototype of this C routine
* is described below:
* .CS
*     void excHandler (ESFPPC *);
* .CE
*
* When the C routine returns, the exception stub restores the registers saved
* in the ESF and continues execution of the current task.
*
* RETURNS: OK, always.
*
* SEE ALSO: excIntConnect(), excVecSet()
*/

STATUS excConnect
    (
    VOIDFUNCPTR * vector,   /* exception vector to attach to */
    VOIDFUNCPTR   routine   /* routine to be called */
    )
    {
    ...
    }

/****************************************************************************
*
* excIntConnect - connect a C routine to an asynchronous exception vector (PowerPC, ARM)
*
* This routine connects a specified C routine to a specified asynchronous
* exception vector.
*
* When the C routine is invoked, interrupts are still locked.  It is the
* responsibility of the C routine to re-enable the interrupt.
*
* The routine can be any normal C code, except that it must not
* invoke certain operating system functions that may block or perform
* I/O operations.
*
* NOTE:
* On PowerPC, the vector is typically the external interrupt vector
* 0x500 and the decrementer vector 0x900.  An interrupt stub is created
* and placed at <vector> in the exception table.  The address of
* <routine> is stored in the interrupt stub code.  When the asynchronous
* exception occurs the processor jumps to the interrupt stub code, saves
* only the requested registers, and calls the C routines.
*
* Before saving the requested registers, the interrupt stub switches from the
* current task stack to the interrupt stack.  For nested interrupts, no
* stack-switching is performed, because the interrupt is already set.
*
* NOTE:
* On the ARM, the address of <routine> is stored in a function pointer
* to be called by the stub installed on the IRQ exception vector
* following an asynchronous exception.  This routine is responsible for
* determining the interrupt source and despatching the correct handler
* for that source.
*
* Before calling the routine, the interrupt stub switches to SVC mode,
* changes to a separate interrupt stack and saves necessary registers. In
* the case of a nested interrupt, no SVC stack switch occurs.
*
* RETURNS: OK, always.
*
* SEE ALSO: excConnect(), excVecSet()
*/

STATUS excIntConnect
    (
    VOIDFUNCPTR * vector,         /* exception vector to attach to */
    VOIDFUNCPTR   routine         /* routine to be called */
    )
    {
    ...
    }

/*******************************************************************************
*
* excCrtConnect - connect a C routine to a critical exception vector (PowerPC 403)
*
* This routine connects a specified C routine to a specified critical exception
* vector.  An exception stub is created and in placed at <vector> in the
* exception table.  The address of <routine> is stored in the exception stub
* code.  When an exception occurs, the processor jumps to the exception stub
* code, saves the registers, and call the C routines.
*
* The routine can be any normal C code, except that it must not
* invoke certain operating system functions that may block or perform
* I/O operations.
*
* The registers are saved to an Exception Stack Frame (ESF) which is placed
* on the stack of the task that has produced the exception.  The ESF structure
* is defined in h/arch/ppc/esfPpc.h.
*
* The only argument passed by the exception stub to the C routine is a pointer
* to the ESF containing the register values.  The prototype of this C routine
* is as follows:
* .CS
*     void excHandler (ESFPPC *);
* .CE
*
* When the C routine returns, the exception stub restores the registers saved
* in the ESF and continues execution of the current task.
*
* RETURNS: OK, always.
* 
* SEE ALSO: excIntConnect(), excIntCrtConnect, excVecSet()
*/

STATUS excCrtConnect
    (
    VOIDFUNCPTR * vector,               /* exception vector to attach to */
    VOIDFUNCPTR   routine               /* routine to be called */
    )
    {
    ...
    }

/*******************************************************************************
*
* excIntCrtConnect - connect a C routine to a critical interrupt vector (PowerPC 403)
*
* This routine connects a specified C routine to a specified asynchronous 
* critical exception vector such as the critical external interrupt vector 
* (0x100), or the watchdog timer vector (0x1020).  An interrupt stub is created 
* and placed at <vector> in the exception table.  The address of <routine> is 
* stored in the interrupt stub code.  When the asynchronous exception occurs,
* the processor jumps to the interrupt stub code, saves only the requested 
* registers, and calls the C routines.
*
* When the C routine is invoked, interrupts are still locked.  It is the
* C routine's responsibility to re-enable interrupts.
*
* The routine can be any normal C routine, except that it must not
* invoke certain operating system functions that may block or perform
* I/O operations.
*
* Before the requested registers are saved, the interrupt stub switches from the
* current task stack to the interrupt stack.  In the case of nested interrupts, no
* stack switching is performed, because the interrupt stack is already set.
*
* RETURNS: OK, always.
* 
* SEE ALSO: excConnect(), excCrtConnect, excVecSet()
*/

STATUS excIntCrtConnect
    (
    VOIDFUNCPTR * vector,               /* exception vector to attach to */
    VOIDFUNCPTR   routine               /* routine to be called */
    )
    {
    ...
    }

/****************************************************************************
*
* excVecSet - set a CPU exception vector (PowerPC, ARM)
*
* This routine specifies the C routine that will be called when the exception
* corresponding to <vector> occurs.  This routine does not create the
* exception stub; it simply replaces the C routine to be called in the
* exception stub.
*
* NOTE ARM:
* On the ARM, there is no excConnect() routine, unlike the PowerPC. The C
* routine is attached to a default stub using excVecSet().
*
* RETURNS: N/A
*
* SEE ALSO: excVecGet(), excConnect(), excIntConnect()
*/

void excVecSet
    (
    FUNCPTR * vector,     /* vector offset */
    FUNCPTR   function    /* address to place in vector */
    )
    {
    ...
    }

/****************************************************************************
*
* excVecGet - get a CPU exception vector (PowerPC, ARM)
*
* This routine returns the address of the C routine currently connected to
* <vector>.
*
* RETURNS: The address of the C routine.
*
* SEE ALSO: excVecSet()
*/

FUNCPTR excVecGet
    (
    FUNCPTR * vector   /* vector offset */
    )
    {
    ...
    }
