/* pentiumALib.s - Pentium and PentiumPro specific routines */

/* Copyright 1984-2001 Wind River Systems, Inc. */

/*
modification history
--------------------
01e,21aug01,hdn  added P5/P6 PMC routines
01d,24mar99,jdi  doc: added basic formatting commands, absent despite
		 thorough and long-standing coverage in WRS Coding Conventions.
01b,17apr98,hdn  fixed typo.
01b,17apr98,hdn  added documentation.
01a,09jul97,hdn  written.
*/

/*
DESCRIPTION
This module contains Pentium and PentiumPro specific routines written in 
assembly language.

.SS "MCA (Machine Check Architecture)"
The Pentium processor introduced a new exception called the machine-check 
exception (interrupt-18).  This exception is used to signal hardware-related
errors, such as a parity error on a read cycle.  The PentiumPro processor
extends the types of errors that can be detected and that generate a machine-
check exception.  It also provides a new machine-check architecture that
records information about a machine-check error and provides the basis for an
extended error logging capability.

MCA is enabled and its status registers are cleared zero in sysHwInit().
Its registers are accessed by pentiumMsrSet() and pentiumMsrGet().

.SS "PMC (Performance Monitoring Counters)"
The P5 and P6 family of processor has two performance-monitoring counters for
use in monitoring internal hardware operations.  These counters are duration
or event counters that can be programmed to count any of approximately 100
different types of events, such as the number of instructions decoded, number
of interrupts received, or number of cache loads. However, the set of events
can be counted with PMC is different in the P5 and P6 family of processors;
and the locations and bit difinitions of the related counter and control
registers are also different. So there are two set of PMC routines, one for
P6 family and one for p5 family respectively.

There are nine routines to interface the PMC of P6 family processors.  These
nine routines are:
.CS
  STATUS pentiumP6PmcStart
         (
	 int pmcEvtSel0;	/@ performance event select register 0 @/
	 int pmcEvtSel1;	/@ performance event select register 1 @/
         )
  void   pentiumP6PmcStop (void)
  void   pentiumP6PmcStop1 (void)
  void   pentiumP6PmcGet
	 (
	 long long int * pPmc0;	/@ performance monitoring counter 0 @/
	 long long int * pPmc1;	/@ performance monitoring counter 1 @/
	 )
  void   pentiumP6PmcGet0
	 (
	 long long int * pPmc0;	/@ performance monitoring counter 0 @/
	 )
  void   pentiumP6PmcGet1
	 (
	 long long int * pPmc1;	/@ performance monitoring counter 1 @/
	 )
  void   pentiumP6PmcReset (void)
  void   pentiumP6PmcReset0 (void)
  void   pentiumP6PmcReset1 (void)
.CE
pentiumP6PmcStart() starts both PMC0 and PMC1. 
pentiumP6PmcStop() stops them, and pentiumP6PmcStop1() stops only PMC1.  
pentiumP6PmcGet() gets contents of PMC0 and PMC1.  pentiumP6PmcGet0() gets
contents of PMC0, and pentiumP6PmcGet1() gets contents of PMC1.
pentiumP6PmcReset() resets both PMC0 and PMC1.  pentiumP6PmcReset0() resets
PMC0, and pentiumP6PmcReset1() resets PMC1.
PMC is enabled in sysHwInit().  Selected events in the default configuration
are PMC0 = number of hardware interrupts received and PMC1 = number of 
misaligned data memory references.

There are ten routines to interface the PMC of P5 family processors.  These
ten routines are:
.CS
  STATUS pentiumP5PmcStart0
         (
	 int pmc0Cesr;	/@ PMC0 control and event select @/
         )
  STATUS pentiumP5PmcStart1
         (
	 int pmc1Cesr;	/@ PMC1 control and event select @/
         )
  void   pentiumP5PmcStop0 (void)
  void   pentiumP5PmcStop1 (void)
  void   pentiumP5PmcGet
	 (
	 long long int * pPmc0;	/@ performance monitoring counter 0 @/
	 long long int * pPmc1;	/@ performance monitoring counter 1 @/
	 )
  void   pentiumP5PmcGet0
	 (
	 long long int * pPmc0;	/@ performance monitoring counter 0 @/
	 )
  void   pentiumP5PmcGet1
	 (
	 long long int * pPmc1;	/@ performance monitoring counter 1 @/
	 )
  void   pentiumP5PmcReset (void)
  void   pentiumP5PmcReset0 (void)
  void   pentiumP5PmcReset1 (void)
.CE
pentiumP5PmcStart0() starts PMC0, and pentiumP5PmcStart1() starts PMC1. 
pentiumP5PmcStop0() stops PMC0, and pentiumP5PmcStop1() stops PMC1.  
pentiumP5PmcGet() gets contents of PMC0 and PMC1.  pentiumP5PmcGet0() gets
contents of PMC0, and pentiumP5PmcGet1() gets contents of PMC1.
pentiumP5PmcReset() resets both PMC0 and PMC1.  pentiumP5PmcReset0() resets
PMC0, and pentiumP5PmcReset1() resets PMC1.
PMC is enabled in sysHwInit().  Selected events in the default configuration
are PMC0 = number of hardware interrupts received and PMC1 = number of 
misaligned data memory references.

.SS "MSR (Model Specific Register)"
The concept of model-specific registers (MSRs) to control hardware functions
in the processor or to monitor processor activity was introduced in the 
PentiumPro processor.  The new registers control the debug extensions, the 
performance counters, the machine-check exception capability, the machine
check architecture, and the MTRRs.  The MSRs can be read and written to using
the RDMSR and WRMSR instructions, respectively.

There are two routines to interface the MSR.  These two routines are:
.CS
  void pentiumMsrGet
       (
       int address,		/@ MSR address @/
       long long int * pData	/@ MSR data @/
       )

  void pentiumMsrSet
       (
       int address,		/@ MSR address @/
       long long int * pData	/@ MSR data @/
       )
.CE
pentiumMsrGet() get contents of the specified MSR, and 
pentiumMsrSet() sets value to the specified MSR.

.SS "TSC (Time Stamp Counter)"
The PentiumPro processor provides a 64-bit time-stamp counter that is 
incremented every processor clock cycle.  The counter is incremented even
when the processor is halted by the HLT instruction or the external STPCLK#
pin.  The time-stamp counter is set to 0 following a hardware reset of the
processor.  The RDTSC instruction reads the time stamp counter and is 
guaranteed to return a monotonically increasing unique value whenever 
executed, except for 64-bit counter wraparound.  Intel guarantees, 
architecturally, that the time-stamp counter frequency and configuration will
be such that it will not wraparound within 10 years after being reset to 0.
The period for counter wrap is several thousands of years in the PentiumPro
and Pentium processors.

There are three routines to interface the TSC.  These three routines are:
.CS
  void pentiumTscReset (void)

  void pentiumTscGet32 (void)

  void pentiumTscGet64
       (
       long long int * pTsc	/@ TSC @/
       )
.CE
pentiumTscReset() resets the TSC.  pentiumTscGet32() gets the lower half of the
64Bit TSC, and pentiumTscGet64() gets the entire 64Bit TSC.

Four other routines are provided in this library.  They are:
.CS
  void   pentiumTlbFlush (void)

  void   pentiumSerialize (void)

  STATUS pentiumBts
	 (
         char * pFlag                   /@ flag address @/
	 )

  STATUS pentiumBtc (pFlag)
	 (
         char * pFlag                   /@ flag address @/
	 )
.CE
pentiumTlbFlush() flushes TLBs (Translation Lookaside Buffers). 
pentiumSerialize() does serialization by executing CPUID instruction.
pentiumBts() executes an atomic compare-and-exchange instruction to set a bit.
pentiumBtc() executes an atomic compare-and-exchange instruction to clear a bit.


INTERNAL
Many routines in this module doesn't use the "c" frame pointer %ebp@ !
This is only for the benefit of the stacktrace facility to allow it 
to properly trace tasks executing within these routines.

SEE ALSO: 
.I "Pentium, PentiumPro Family Developer's Manual"
*/


#define _ASMLANGUAGE
#include "vxWorks.h"
#include "asm.h"
#include "regs.h"
#include "arch/i86/pentiumLib.h"


        .data
	.globl  FUNC(copyright_wind_river)
	.long   FUNC(copyright_wind_river)


	/* internals */

	.globl	GTEXT(pentiumCr4Get)
	.globl	GTEXT(pentiumCr4Set)
	.globl	GTEXT(pentiumP6PmcStart)
	.globl	GTEXT(pentiumP6PmcStop)
	.globl	GTEXT(pentiumP6PmcStop1)
	.globl	GTEXT(pentiumP6PmcGet)
	.globl	GTEXT(pentiumP6PmcGet0)
	.globl	GTEXT(pentiumP6PmcGet1)
	.globl	GTEXT(pentiumP6PmcReset)
	.globl	GTEXT(pentiumP6PmcReset0)
	.globl	GTEXT(pentiumP6PmcReset1)
        .globl  GTEXT(pentiumP5PmcStart0)
        .globl  GTEXT(pentiumP5PmcStart1)
        .globl  GTEXT(pentiumP5PmcStop0)
        .globl  GTEXT(pentiumP5PmcStop1)
        .globl  GTEXT(pentiumP5PmcReset)
        .globl  GTEXT(pentiumP5PmcReset0)
        .globl  GTEXT(pentiumP5PmcReset1)
        .globl  GTEXT(pentiumP5PmcGet)
        .globl  GTEXT(pentiumP5PmcGet0)
        .globl  GTEXT(pentiumP5PmcGet1)
	.globl	GTEXT(pentiumTscGet64)
	.globl	GTEXT(pentiumTscGet32)
	.globl	GTEXT(pentiumTscReset)
	.globl	GTEXT(pentiumMsrGet)
	.globl	GTEXT(pentiumMsrSet)
	.globl	GTEXT(pentiumTlbFlush)
	.globl	GTEXT(pentiumSerialize)
	.globl	GTEXT(pentiumBts)
	.globl	GTEXT(pentiumBtc)


	.data
	.balign 16,0x90
_pmcBusy:
	.byte	0x00			/* PMC busy flag, 1 = busy */
_pmc0Busy:
        .byte   0x00                    /* PMC0 busy flag, 1 = busy */
_pmc1Busy:
        .byte   0x00                    /* PMC1 busy flag, 1 = busy */

	.text
	.balign 16,0x90

/*******************************************************************************
*
* pentiumCr4Get - get contents of CR4 register
*
* SYNOPSIS
* \ss
* int pentiumCr4Get (void)
* \se
*
* This routine gets the contents of the CR4 register.
*
* RETURNS: Contents of CR4 register.
*/

FUNC_LABEL(pentiumCr4Get)
	movl	%cr4,%eax
	ret

/*******************************************************************************
*
* pentiumCr4Set - sets specified value to the CR4 register
*
* SYNOPSIS
* \ss
* void pentiumCr4Set (cr4)
*    int cr4;		/@ value to write CR4 register @/
* \se
* This routine sets a specified value to the CR4 register.
*
*
* RETURNS: N/A
*/

	.balign 16,0x90
FUNC_LABEL(pentiumCr4Set)
	movl	SP_ARG1(%esp),%eax
	movl	%eax,%cr4
	ret

/*******************************************************************************
*
* pentiumP6PmcStart - start both PMC0 and PMC1
*
* SYNOPSIS
* \ss
* STATUS pentiumP6PmcStart (pmcEvtSel0, pmcEvtSel1)
*     int pmcEvtSel0;		/@ Performance Event Select Register 0 @/
*     int pmcEvtSel1;		/@ Performance Event Select Register 1 @/
* \se
* 
* This routine starts both PMC0 (Performance Monitoring Counter 0) and PMC1
* by writing specified events to Performance Event Select Registers. 
* The first parameter is a content of Performance Event Select Register 0,
* and the second parameter is for the Performance Event Select Register 1.
*
* RETURNS: OK or ERROR if PMC is already started.
*/

        .balign 16,0x90
FUNC_LABEL(pentiumP6PmcStart)
	xorl	%eax,%eax
	movl	$ TRUE, %edx
	lock				/* lock the BUS */
	cmpxchgb %dl,_pmcBusy		/* if (_pmcBusy == 0) */
	jnz	pentiumP6PmcStart0	/*   {ZF = 1; _pmcBusy = TRUE;} */

	movl	SP_ARG1(%esp),%eax	/* low-order 32 bits */
	xorl	%edx,%edx		/* high-order 32 bits */
	movl	$ MSR_EVNTSEL0,%ecx	/* specify MSR_EVNTSEL0 */
	wrmsr				/* write %edx:%eax to MSR_EVNTSEL0 */
	movl	SP_ARG2(%esp),%eax	/* low-order 32 bits */
	xorl	%edx,%edx		/* high-order 32 bits */
	movl	$ MSR_EVNTSEL1,%ecx	/* specify MSR_EVNTSEL1 */
	wrmsr				/* write %edx:%eax to MSR_EVNTSEL1 */
	xorl	%eax,%eax		/* return OK */
	ret

pentiumP6PmcStart0:
	movl	$ ERROR,%eax
	ret

/*******************************************************************************
*
* pentiumP6PmcStop - stop both PMC0 and PMC1
*
* SYNOPSIS
* \ss
* void pentiumP6PmcStop (void)
* \se
* 
* This routine stops both PMC0 (Performance Monitoring Counter 0) and PMC1
* by clearing two Performance Event Select Registers.
*
* RETURNS: N/A
*/

        .balign 16,0x90
FUNC_LABEL(pentiumP6PmcStop)
	xorl	%eax,%eax		/* zero low-order 32 bits */
	xorl	%edx,%edx		/* zero high-order 32 bits */
	movl	$ MSR_EVNTSEL0,%ecx	/* specify MSR_EVNTSEL0 */
	wrmsr				/* write %edx:%eax to MSR_EVNTSEL0 */
	movl	$ MSR_EVNTSEL1,%ecx	/* specify MSR_EVNTSEL1 */
	wrmsr				/* write %edx:%eax to MSR_EVNTSEL1 */

	movl	$ TRUE, %eax
	xorl	%edx,%edx
	lock				/* lock the BUS */
	cmpxchgb %dl,_pmcBusy		/* if (_pmcBusy == TRUE) */
	ret				/*   {ZF = 1; _pmcBusy = 0;} */

/*******************************************************************************
*
* pentiumP6PmcStop1 - stop PMC1
*
* SYNOPSIS
* \ss
* void pentiumP6PmcStop1 (void)
* \se
* 
* This routine stops only PMC1 (Performance Monitoring Counter 1)
* by clearing the Performance Event Select Register 1.
* Note, clearing the Performance Event Select Register 0 stops both counters,
* PMC0 and PMC1.
*
* RETURNS: N/A
*/

        .balign 16,0x90
FUNC_LABEL(pentiumP6PmcStop1)
	xorl	%eax,%eax		/* zero low-order 32 bits */
	xorl	%edx,%edx		/* zero high-order 32 bits */
	movl	$ MSR_EVNTSEL1,%ecx	/* specify MSR_EVNTSEL1 */
	wrmsr				/* write %edx:%eax to MSR_EVNTSEL1 */
	ret

/*******************************************************************************
*
* pentiumP6PmcGet - get the contents of PMC0 and PMC1
*
* SYNOPSIS
* \ss
* void pentiumP6PmcGet (pPmc0, pPmc1)
*     long long int * pPmc0;		/@ Performance Monitoring Counter 0 @/
*     long long int * pPmc1;		/@ Performance Monitoring Counter 1 @/
* \se
* 
* This routine gets the contents of both PMC0 (Performance Monitoring Counter 0)
* and PMC1.  The first parameter is a pointer of 64Bit variable to store
* the content of the Counter 0, and the second parameter is for the Counter 1.
*
* RETURNS: N/A
*/

        .balign 16,0x90
FUNC_LABEL(pentiumP6PmcGet)
	movl	$0,%ecx			/* specify PMC0 */
	rdpmc				/* read PMC0 to %edx:%eax */
	movl	SP_ARG1(%esp),%ecx
	movl	%eax,(%ecx)		/* save low-order 32 bits */
	movl	%edx,4(%ecx)		/* save high-order 32 bits */
	movl	$1,%ecx			/* specify PMC1 */
	rdpmc				/* read PMC1 to %edx:%eax */
	movl	SP_ARG2(%esp),%ecx
	movl	%eax,(%ecx)		/* save low-order 32 bits */
	movl	%edx,4(%ecx)		/* save high-order 32 bits */
	ret

/*******************************************************************************
*
* pentiumP6PmcGet0 - get the contents of PMC0
*
* SYNOPSIS
* \ss
* void pentiumP6PmcGet0 (pPmc0)
*     long long int * pPmc0;		/@ Performance Monitoring Counter 0 @/
* \se
* 
* This routine gets the contents of PMC0 (Performance Monitoring Counter 0).
* The parameter is a pointer of 64Bit variable to store the content of
* the Counter.
*
* RETURNS: N/A
*/

        .balign 16,0x90
FUNC_LABEL(pentiumP6PmcGet0)
	movl	$0,%ecx			/* specify PMC0 */
	rdpmc				/* read PMC0 to %edx:%eax */
	movl	SP_ARG1(%esp),%ecx
	movl	%eax,(%ecx)		/* save low-order 32 bits */
	movl	%edx,4(%ecx)		/* save high-order 32 bits */
	ret

/*******************************************************************************
*
* pentiumP6PmcGet1 - get the contents of PMC1
*
* SYNOPSIS
* \ss
* void pentiumP6PmcGet1 (pPmc1)
*     long long int * pPmc1;		/@ Performance Monitoring Counter 1 @/
* \se
* 
* This routine gets a content of PMC1 (Performance Monitoring Counter 1).
* Parameter is a pointer of 64Bit variable to store the content of the Counter.
*
* RETURNS: N/A
*/

        .balign 16,0x90
FUNC_LABEL(pentiumP6PmcGet1)
	movl	$1,%ecx			/* specify PMC1 */
	rdpmc				/* read PMC1 to %edx:%eax */
	movl	SP_ARG1(%esp),%ecx
	movl	%eax,(%ecx)		/* save low-order 32 bits */
	movl	%edx,4(%ecx)		/* save high-order 32 bits */
	ret

/*******************************************************************************
*
* pentiumP6PmcReset - reset both PMC0 and PMC1
*
* SYNOPSIS
* \ss
* void pentiumP6PmcReset (void)
* \se
* 
* This routine resets both PMC0 (Performance Monitoring Counter 0) and PMC1.
*
* RETURNS: N/A
*/

        .balign 16,0x90
FUNC_LABEL(pentiumP6PmcReset)
	xorl	%eax,%eax		/* zero low-order 32 bits */
	xorl	%edx,%edx		/* zero high-order 32 bits */
	movl	$ MSR_PERFCTR0,%ecx	/* specify MSR_PERFCTR0 */
	wrmsr				/* write %edx:%eax to MSR_PERFCTR0 */
	movl	$ MSR_PERFCTR1,%ecx	/* specify MSR_PERFCTR1 */
	wrmsr				/* write %edx:%eax to MSR_PERFCTR1 */
	ret

/*******************************************************************************
*
* pentiumP6PmcReset0 - reset PMC0
*
* SYNOPSIS
* \ss
* void pentiumP6PmcReset0 (void)
* \se
* 
* This routine resets PMC0 (Performance Monitoring Counter 0).
*
* RETURNS: N/A
*/

        .balign 16,0x90
FUNC_LABEL(pentiumP6PmcReset0)
	xorl	%eax,%eax		/* zero low-order 32 bits */
	xorl	%edx,%edx		/* zero high-order 32 bits */
	movl	$ MSR_PERFCTR0,%ecx	/* specify MSR_PERFCTR0 */
	wrmsr				/* write %edx:%eax to MSR_PERFCTR0 */
	ret

/*******************************************************************************
*
* pentiumP6PmcReset1 - reset PMC1
*
* SYNOPSIS
* \ss
* void pentiumP6PmcReset1 (void)
* \se
* 
* This routine resets PMC1 (Performance Monitoring Counter 1).
*
* RETURNS: N/A
*/

        .balign 16,0x90
FUNC_LABEL(pentiumP6PmcReset1)
	xorl	%eax,%eax		/* zero low-order 32 bits */
	xorl	%edx,%edx		/* zero high-order 32 bits */
	movl	$ MSR_PERFCTR1,%ecx	/* specify MSR_PERFCTR1 */
	wrmsr				/* write %edx:%eax to MSR_PERFCTR1 */
	ret

/*******************************************************************************
*
* pentiumP5PmcStart0 - start PMC0
*
* SYNOPSIS
* \ss
* STATUS pentiumP5PmcStart0 (pmc0Cesr)
*       int pmc0Cesr;           /@ PMC0 control and event select @/
* \se
*
* This routine starts PMC0 (Performance Monitoring Counter 0) 
* by writing specified PMC0 events to Performance Event Select Registers. 
* The only parameter is the content of Performance Event Select Register.
*
* RETURNS: OK or ERROR if PMC0 is already started.
*/

        .balign 16,0x90
FUNC_LABEL(pentiumP5PmcStart0)
	xorl	%eax,%eax
	movl	$ TRUE, %edx
	lock				/* lock the BUS */
	cmpxchgb %dl,_pmc0Busy		/* if (_pmc0Busy == 0) */
	jnz	pentiumP5PmcStart0L0	/*   {ZF = 1; _pmc0Busy = TRUE;} */

	movl	$ MSR_CESR,%ecx		/* specify MSR_CESR */
	rdmsr				/* read from MSR_CESR */
	andl	$ 0xFFFF0000,%eax	/* mask out PMC0 bits */
	movl	SP_ARG1(%esp),%edx	/* new PMC0 bits */
	andl	$ 0x0000FFFF,%edx	/* mask out the high 16 bits */
	orl	%edx,%eax		/* merge with orig PMC1 bits */ 
	xorl	%edx,%edx		/* high-order 32 bits */
	wrmsr				/* write %edx:%eax to MSR_CESR */
	xorl	%eax,%eax		/* return OK */
	ret

pentiumP5PmcStart0L0:
	movl	$ ERROR,%eax
	ret

/*******************************************************************************
*
* pentiumP5PmcStart1 - start PMC1
*
* SYNOPSIS
* \ss
* STATUS pentiumP5PmcStart1 (pmc1Cesr)
*       int pmc1Cesr;           /@ PMC1 control and event select @/
* \se
*
* This routine starts PMC1 (Performance Monitoring Counter 0) 
* by writing specified PMC1 events to Performance Event Select Registers. 
* The only parameter is the content of Performance Event Select Register.
*
* RETURNS: OK or ERROR if PMC1 is already started.
*/

        .balign 16,0x90
FUNC_LABEL(pentiumP5PmcStart1)
	xorl	%eax,%eax
	movl	$ TRUE, %edx
	lock				/* lock the BUS */
	cmpxchgb %dl,_pmc1Busy		/* if (_pmc1Busy == 0) */
	jnz	pentiumP5PmcStart1L0	/*   {ZF = 1; _pmcBusy = TRUE;} */

	movl	$ MSR_CESR,%ecx		/* specify MSR_CESR */
	rdmsr				/* read from MSR_CESR */
	andl	$ 0x0000FFFF,%eax	/* mask out PMC1 bits */
	movl	SP_ARG1(%esp),%edx	/* new PMC1 bits */
	shll	$ 16,%edx     		/* shift PMC1 bits to higher 16 bits */
	orl	%edx,%eax		/* merge with orig PMC1 bits */ 
	xorl	%edx,%edx		/* high-order 32 bits */
	wrmsr				/* write %edx:%eax to MSR_CESR */
	xorl	%eax,%eax		/* return OK */
	ret

pentiumP5PmcStart1L0:
	movl	$ ERROR,%eax
	ret

/*******************************************************************************
*
* pentiumP5PmcStop - stop both P5 PMC0 and PMC1
*
* SYNOPSIS
* \ss
* void pentiumP5PmcStop (void)
* \se
*
* This routine stops both PMC0 (Performance Monitoring Counter 0)
* and PMC1 by clearing two Performance Event Select Registers.
*
* RETURNS: N/A
*/

        .balign 16,0x90
FUNC_LABEL(pentiumP5PmcStop)
	xorl	%eax,%eax		/* zero low-order 32 bits */
	xorl	%edx,%edx		/* zero high-order 32 bits */
	movl	$ MSR_CESR,%ecx		
	wrmsr				/* write %edx:%eax to CESR reg */

	movl	$ TRUE, %eax
	xorl	%edx,%edx
	lock				/* lock the BUS */
	cmpxchgb %dl,_pmc0Busy		/* if (_pmc0Busy == TRUE) */
					/*   {ZF = 1; _pmcBusy = 0;} */
	movl	$ TRUE, %eax
	xorl	%edx,%edx
	lock				/* lock the BUS */
	cmpxchgb %dl,_pmc1Busy		/* if (_pmc0Busy == TRUE) */
					/*   {ZF = 1; _pmcBusy = 0;} */
	ret

/*******************************************************************************
*
* pentiumP5PmcStop0 - stop P5 PMC0
*
* SYNOPSIS
* \ss
* void pentiumP5PmcStop0 (void)
* \se
*
* This routine stops only PMC0 (Performance Monitoring Counter 0)
* by clearing the PMC0 bits of Control and Event Select Register.
*
* RETURNS: N/A
*/

        .balign 16,0x90
FUNC_LABEL(pentiumP5PmcStop0)
	movl	$ MSR_CESR,%ecx		/* select MSR_CESR register */
	rdmsr
	andl	$ 0xFFFF0000,%eax	/* clear PMC0 bits in MSR_CESR */
	xorl	%edx,%edx		/* zero high-order 32 bits */
	wrmsr				/* write %edx:%eax to MSR_CESR */

	movl	$ TRUE, %eax
	xorl	%edx,%edx
	lock				/* lock the BUS */
	cmpxchgb %dl,_pmc0Busy		/* if (_pmc0Busy == TRUE) */
	ret

/*******************************************************************************
*
* pentiumP5PmcStop1 - stop P5 PMC1
*
* SYNOPSIS
* \ss
* void pentiumP5PmcStop1 (void)
* \se
*
* This routine stops only PMC1 (Performance Monitoring Counter 1)
* by clearing the PMC1 bits of Control and Event Select Register.
*
* RETURNS: N/A
*/

        .balign 16,0x90
FUNC_LABEL(pentiumP5PmcStop1)
	movl	$ MSR_CESR,%ecx		/* select MSR_CESR register */
	rdmsr
	andl	$ 0x0000FFFF,%eax	/* clear PMC1 bits in MSR_CESR */
	xorl	%edx,%edx		/* zero high-order 32 bits */
	wrmsr				/* write %edx:%eax to MSR_CESR */

	movl	$ TRUE, %eax
	xorl	%edx,%edx
	lock				/* lock the BUS */
	cmpxchgb %dl,_pmc1Busy		/* if (_pmc1Busy == TRUE) */
	ret

/*******************************************************************************
*
* pentiumP5PmcGet - get the contents of P5 PMC0 and PMC1
*
* SYNOPSIS
* \ss
* void pentiumP5PmcGet (pPmc0, pPmc1)
*     long long int * pPmc0;            /@ Performance Monitoring Counter 0 @/
*     long long int * pPmc1;            /@ Performance Monitoring Counter 1 @/
* \se
*
* This routine gets the contents of both PMC0 (Performance Monitoring Counter 0)
* and PMC1.  The first parameter is a pointer of 64Bit variable to store
* the content of the Counter 0, and the second parameter is for the Counter 1.
*
* RETURNS: N/A
*/

        .balign 16,0x90
FUNC_LABEL(pentiumP5PmcGet)
	movl	$ MSR_CTR0,%ecx		/* specify PMC0 */
	rdmsr				/* read PMC0 to %edx:%eax */
	movl	SP_ARG1(%esp),%ecx
	movl	%eax,(%ecx)		/* save low-order 32 bits */
	movl	%edx,4(%ecx)		/* save high-order 32 bits */
	movl	$ MSR_CTR1,%ecx		/* specify PMC1 */
	rdmsr				/* read PMC1 to %edx:%eax */
	movl	SP_ARG2(%esp),%ecx
	movl	%eax,(%ecx)		/* save low-order 32 bits */
	movl	%edx,4(%ecx)		/* save high-order 32 bits */
	ret

/*******************************************************************************
*
* pentiumP5PmcGet0 - get the contents of P5 PMC0
*
* SYNOPSIS
* \ss
* void pentiumP5PmcGet0 (pPmc0)
*     long long int * pPmc0;            /@ Performance Monitoring Counter 0 @/
* \se
*
* This routine gets the contents of PMC0 (Performance Monitoring Counter 0).
* The parameter is a pointer of 64Bit variable to store the content of
* the Counter.
*
* RETURNS: N/A
*/

        .balign 16,0x90
FUNC_LABEL(pentiumP5PmcGet0)
	movl	$ MSR_CTR0,%ecx		/* specify PMC0 */
	rdmsr				/* read PMC0 to %edx:%eax */
	movl	SP_ARG1(%esp),%ecx
	movl	%eax,(%ecx)		/* save low-order 32 bits */
	movl	%edx,4(%ecx)		/* save high-order 32 bits */
	ret

/*******************************************************************************
*
* pentiumP5PmcGet1 - get the contents of P5 PMC1
*
* SYNOPSIS
* \ss
* void pentiumP5PmcGet1 (pPmc1)
*     long long int * pPmc1;            /@ Performance Monitoring Counter 1 @/
* \se
*
* This routine gets a content of PMC1 (Performance Monitoring Counter 1).
* Parameter is a pointer of 64Bit variable to store the content of the Counter.
*
* RETURNS: N/A
*/

        .balign 16,0x90
FUNC_LABEL(pentiumP5PmcGet1)
	movl	$ MSR_CTR1,%ecx		/* specify PMC1 */
	rdmsr				/* read PMC1 to %edx:%eax */
	movl	SP_ARG1(%esp),%ecx
	movl	%eax,(%ecx)		/* save low-order 32 bits */
	movl	%edx,4(%ecx)		/* save high-order 32 bits */
	ret

/*******************************************************************************
*
* pentiumP5PmcReset - reset both PMC0 and PMC1
*
* SYNOPSIS
* \ss
* void pentiumP5PmcReset (void)
* \se
*
* This routine resets both PMC0 (Performance Monitoring Counter 0) and PMC1.
*
* RETURNS: N/A 
*/

        .balign 16,0x90
FUNC_LABEL(pentiumP5PmcReset)
	xorl	%eax,%eax		/* zero low-order 32 bits */
	xorl	%edx,%edx		/* zero high-order 32 bits */
	movl	$ MSR_CTR0,%ecx 	/* specify MSR_CTR0 */
	wrmsr				/* write %edx:%eax to MSR_CTR0 */
	movl	$ MSR_CTR1,%ecx 	/* specify MSR_CRT1 */
	wrmsr				/* write %edx:%eax to MSR_CTR1 */
	ret

/*******************************************************************************
*
* pentiumP5PmcReset0 - reset PMC0
*
* SYNOPSIS
* \ss
* void pentiumP5PmcReset0 (void)
* \se
*
* This routine resets PMC0 (Performance Monitoring Counter 0).
*
* RETURNS: N/A 
*/

        .balign 16,0x90
FUNC_LABEL(pentiumP5PmcReset0)
	xorl	%eax,%eax		/* zero low-order 32 bits */
	xorl	%edx,%edx		/* zero high-order 32 bits */
	movl	$ MSR_CTR0,%ecx		/* specify MSR_CTR0 */
	wrmsr				/* write %edx:%eax to MSR_CTR1 */
	ret

/*******************************************************************************
*
* pentiumP5PmcReset1 - reset PMC1
*
* SYNOPSIS
* \ss
* void pentiumP5PmcReset1 (void)
* \se
*
* This routine resets PMC1 (Performance Monitoring Counter 1).
*
* RETURNS: N/A
*/

        .balign 16,0x90
FUNC_LABEL(pentiumP5PmcReset1)
	xorl	%eax,%eax		/* zero low-order 32 bits */
	xorl	%edx,%edx		/* zero high-order 32 bits */
	movl	$ MSR_CTR1,%ecx		/* specify MSR_CTR1 */
	wrmsr				/* write %edx:%eax to MSR_CTR1 */
	ret

/*******************************************************************************
*
* pentiumTscGet64 - get 64Bit TSC (Timestamp Counter)
*
* SYNOPSIS
* \ss
* void pentiumTscGet64 (pTsc)
*     long long int * pTsc;		/@ Timestamp Counter @/
* \se
* 
* This routine gets 64Bit TSC by RDTSC instruction.
* Parameter is a pointer of 64Bit variable to store the content of the Counter.
*
* RETURNS: N/A
*/

        .balign 16,0x90
FUNC_LABEL(pentiumTscGet64)
	movl	SP_ARG1(%esp),%ecx
	rdtsc				/* read TSC to %edx:%eax */
	movl	%eax,(%ecx)		/* save low-order 32 bits */
	movl	%edx,4(%ecx)		/* save high-order 32 bits */
	ret

/*******************************************************************************
*
* pentiumTscGet32 - get the lower half of the 64Bit TSC (Timestamp Counter)
*
* SYNOPSIS
* \ss
* UINT32 pentiumTscGet32 (void)
* \se
* 
* This routine gets a lower half of the 64Bit TSC by RDTSC instruction.
* RDTSC instruction saves the lower 32Bit in EAX register, so this routine
* simply returns after executing RDTSC instruction.
*
* RETURNS: Lower half of the 64Bit TSC (Timestamp Counter)
*/

        .balign 16,0x90
FUNC_LABEL(pentiumTscGet32)
	rdtsc				/* read TSC to %edx:%eax */
	ret

/*******************************************************************************
*
* pentiumTscReset - reset the TSC (Timestamp Counter)
*
* SYNOPSIS
* \ss
* void pentiumTscReset (void)
* \se
* 
* This routine resets the TSC by writing zero to the TSC with WRMSR
* instruction.  
*
* RETURNS: N/A
*/

        .balign 16,0x90
FUNC_LABEL(pentiumTscReset)
	xorl	%eax,%eax		/* zero low-order 32 bits */
	xorl	%edx,%edx		/* zero high-order 32 bits */
	movl	$ MSR_TSC,%ecx		/* specify MSR_TSC */
	wrmsr				/* write %edx:%eax to TSC */
	ret

/*******************************************************************************
*
* pentiumMsrGet - get the contents of the specified MSR (Model Specific Register)
*
* SYNOPSIS
* \ss
* void pentiumMsrGet (addr, pData)
*     int addr;			/@ MSR address @/
*     long long int * pData;		/@ MSR data @/
* \se
* 
* This routine gets the contents of the specified MSR.  The first parameter is
* an address of the MSR.  The second parameter is a pointer of 64Bit variable.
*
* RETURNS: N/A
*/

        .balign 16,0x90
FUNC_LABEL(pentiumMsrGet)
	movl	SP_ARG1(%esp),%ecx	/* specify MSR to read */
	rdmsr				/* read the MSR to %edx:%eax */
	movl	SP_ARG2(%esp),%ecx
	movl	%eax,(%ecx)		/* save low-order 32 bits */
	movl	%edx,4(%ecx)		/* save high-order 32 bits */
	ret

/*******************************************************************************
*
* pentiumMsrSet - set a value to the specified MSR (Model Specific Registers)
*
* SYNOPSIS
* \ss
* void pentiumMsrSet (addr, pData)
*     int addr;				/@ MSR address @/
*     long long int * pData;		/@ MSR data @/
* \se
* 
* This routine sets a value to a specified MSR.  The first parameter is an
* address of the MSR.  The second parameter is a pointer of 64Bit variable.
*
* RETURNS: N/A
*/

        .balign 16,0x90
FUNC_LABEL(pentiumMsrSet)
	movl	SP_ARG2(%esp),%ecx
	movl	(%ecx),%eax		/* low-order 32 bits */
	movl	4(%ecx),%edx		/* high-order 32 bits */
	movl	SP_ARG1(%esp),%ecx	/* specify MSR to read */
	wrmsr				/* write %edx:%eax to the MSR */
	ret

/*******************************************************************************
*
* pentiumTlbFlush - flush TLBs (Translation Lookaside Buffers)
*
* SYNOPSIS
* \ss
* void pentiumTlbFlush (void)
* \se
* 
* This routine flushes TLBs by loading the CR3 register.
* All of the TLBs are automatically invalidated any time the CR3 register is
* loaded.  The page global enable (PGE) flag in register CR4 and the global
* flag in a page-directory or page-table entry can be used to frequently used
* pages from being automatically invalidated in the TLBs on a load of CR3
* register.  The only way to deterministically invalidate global page entries
* is to clear the PGE flag and then invalidate the TLBs.
*
* RETURNS: N/A
*/

        .balign 16,0x90
FUNC_LABEL(pentiumTlbFlush)
	movl    %cr3,%eax
	movl    %eax,%cr3		/* flush the TLB */
	jmp     pentiumTlbFlush0	/* flush the prefetch queue */
pentiumTlbFlush0:
	ret

/*******************************************************************************
*
* pentiumSerialize - execute a serializing instruction CPUID
*
* SYNOPSIS
* \ss
* void pentiumSerialize (void)
* \se
* 
* This routine executes a serializing instruction CPUID.
* Serialization means that all modifications to flags, registers, and memory
* by previous instructions are completed before the next instruction is fetched
* and executed and all buffered writes have drained to memory.
*
* RETURNS: N/A
*/

        .balign 16,0x90
FUNC_LABEL(pentiumSerialize)
	pushl	%ebx			/* save ebx which is used by cpuid */
	cpuid				/* serializing instruction */
	popl	%ebx			/* restore ebx */
	ret

/*******************************************************************************
*
* pentiumBts - execute atomic compare-and-exchange instruction to set a bit
*
* SYNOPSIS
* \ss
* STATUS pentiumBts (pFlag)
*     char * pFlag;			/@ flag address @/
* \se
* 
* This routine compares a byte specified by the first parameter with 0.
* If it is 0, it changes it to TRUE and returns OK.
* If it is not 0, it returns ERROR.  LOCK and CMPXCHGB are used to get 
* the atomic memory access.
*
* RETURNS: OK or ERROR if the specified flag is not zero.
*/

        .balign 16,0x90
FUNC_LABEL(pentiumBts)
	movl	SP_ARG1(%esp),%ecx
	xorl	%eax,%eax
	movl	$ TRUE,%edx
	lock				/* lock the BUS */
	cmpxchgb %dl,(%ecx)		/* if (flag == 0) */
	jnz	pentiumBts0		/*   {ZF = 1; flag = TRUE;} */
	ret
pentiumBts0:
	movl	$ ERROR,%eax
	ret

/*******************************************************************************
*
* pentiumBtc - execute atomic compare-and-exchange instruction to clear a bit
*
* SYNOPSIS
* \ss
* STATUS pentiumBtc (pFlag)
*     char * pFlag;			/@ flag address @/
* \se
* 
* This routine compares a byte specified by the first parameter with TRUE.
* If it is TRUE, it changes it to 0 and returns OK.
* If it is not TRUE, it returns ERROR.  LOCK and CMPXCHGB are used to get 
* the atomic memory access.
*
* RETURNS: OK or ERROR if the specified flag is not TRUE
*/

        .balign 16,0x90
FUNC_LABEL(pentiumBtc)
	movl	SP_ARG1(%esp),%ecx
	movl	$ TRUE,%eax
	xorl	%edx,%edx
	lock				/* lock the BUS */
	cmpxchgb %dl,(%ecx)		/* if (flag == TRUE) */
	jnz	pentiumBtc0		/*   {ZF = 1; flag = 0;} */
	xorl	%eax,%eax
	ret
pentiumBtc0:
	movl	$ ERROR,%eax
	ret

