/* vxShow.c - Intel Architecture processor show routines */

/* Copyright 1984-2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01g,12mar02,pai  Add an initialization routine for configuring the show
                 routines into a build configuration (SPR 74103).  Added
                 vxSseShow() routine and updated documentation.
01f,07mar02,hdn  updated with AP485 rev-019 for HTT (spr 73359)
01e,20nov01,hdn  doc clean up for 5.5
01d,07nov01,hdn  added header file vxI86Lib.h to shut off the warning
01c,01nov01,hdn  added vxDrShow() to see the debug registers
01b,11oct01,hdn  uncommented out the "Fast System Call" printing
01a,14aug01,hdn  taken from T31 ver 01h, added PENTIUM4 support
*/

/*
DESCRIPTION
This library contains routines for printing various types of information
on Intel Architecture CPUs.

To use these routines, install the library using the vxShowInit()
routine, which is called automatically when either of the following
configuration methods is employed:

.iP
INCLUDE_SHOW_ROUTINES is defined in a BSP config.h file for
command-line builds.

.iP
The INCLUDE_SHOW_ROUTINES Component is selected for Tornado
Project Facility builds.
.LP

Included in this library is a routine for reporting details on Intel
Architecture 80x86 and Pentium generation processors and coprocessors.
This vxCpuShow() routine is essentially a VxWorks implementation of the
code found in the Intel Application Note, "AP-485, Intel Processor
Identification and the CPUID Instruction".  The vxCpuShow() routine may
be useful for determining specific features found on the target processor
upon which VxWorks is running.

In addition to the vxCpuShow() routine, there are routines for showing
processer register contents.  Some of these routines show certain processor
registers independent of any task context, while others show register
values associated with a particular task context.

There is a simple routine for showing the contents of the processor debug
registers.  The debug registers (DR0 through DR7) control, and allow
monitoring of, the processor's debugging operations.

This library provides a routine for showing a task's optional Streaming
SIMD Extension (SSE) register context.  Target-based task information
routines, such as ti(), will display a task's SSE context, if any, when
this library is built into a VxWorks system.

SEE ALSO:
.pG "Configuration"
*/


/* includes */

#include "vxWorks.h"
#include "regs.h"
#include "vxLib.h"
#include "stdio.h"
#include "string.h"
#include "taskLib.h"
#include "fioLib.h"
#include "fppLib.h"
#include "arch/i86/pentiumLib.h"
#include "private/funcBindP.h"


/* defines */

#define FPU_FLAG	0x00000001	/* floating point unit on chip */
#define VME_FLAG 	0x00000002	/* virtual mode extension */
#define DE_FLAG 	0x00000004	/* debugging extension */
#define PSE_FLAG 	0x00000008	/* page size extension */
#define TSC_FLAG 	0x00000010	/* time stamp counter */
#define MSR_FLAG 	0x00000020	/* model specific register */
#define PAE_FLAG 	0x00000040	/* physical address extension */
#define MCE_FLAG 	0x00000080	/* machine check exception */
#define CX8_FLAG 	0x00000100	/* CMPXCHG8 instruction supported */
#define APIC_FLAG 	0x00000200	/* on-chip APIC hardware support */
#define SEP_FLAG 	0x00000800	/* fast system call */
#define MTRR_FLAG 	0x00001000	/* memory type range registers */
#define PGE_FLAG 	0x00002000	/* page global enable */
#define MCA_FLAG 	0x00004000	/* machine check architecture */
#define CMOV_FLAG 	0x00008000	/* conditional move instruction */
#define PAT_FLAG 	0x00010000	/* page attribute table */
#define PSE36_FLAG 	0x00020000	/* 36-bit page size extension */
#define PSNUM_FLAG 	0x00040000	/* processor serial no is enabled */
#define CLFLUSH_FLAG 	0x00080000	/* CLFLUSH instruction supported */
#define DTS_FLAG 	0x00200000	/* Debug Trace Store */
#define ACPI_FLAG 	0x00400000	/* ACPI registers in MSR space */
#define MMX_FLAG 	0x00800000	/* IA MMX technology supported */
#define FXSR_FLAG 	0x01000000	/* fast floating point save, restore */
#define SSE_FLAG 	0x02000000	/* Streaming SIMD Extension */
#define SSE2_FLAG 	0x04000000	/* Streaming SIMD Extension 2 */
#define SS_FLAG 	0x08000000	/* Self Snoop */
#define TM_FLAG 	0x20000000	/* Thermal Monitor */
#define HTT_FLAG 	0x10000000	/* Hyper Threading Technology */

#define BRAND_TABLE_SIZE	10	/* brand table size */

typedef struct brand_entry		/* brand table entry */
	{
	long brand_value;		/* brand id */
	char * brand_string;		/* corresponding brand name */
	} BRAND_ENTRY;


/* externals */

IMPORT CPUID	sysCpuId;
IMPORT int	sysProcessor;
IMPORT int	sysCoprocessor;


/* locals */

#if	FALSE	/* Unlike AE, 5.5 does not have TSS per task */

LOCAL const char * tssFmt0 = "\n\
link : 0x%08x\n\
esp0 : 0x%08x   ss0 : 0x%08x\n\
esp1 : 0x%08x   ss1 : 0x%08x\n\
esp2 : 0x%08x   ss2 : 0x%08x\n\
 cr3 : 0x%08x   eip : 0x%08x eflags : 0x%08x\n\
 eax : 0x%08x   ecx : 0x%08x    edx : 0x%08x    ebx : 0x%08x\n\
 esp : 0x%08x   ebp : 0x%08x    esi : 0x%08x    edi : 0x%08x\n\
  cs : 0x%08x    ds : 0x%08x     ss : 0x%08x\n\
  es : 0x%08x    fs : 0x%08x     gs : 0x%08x\n\
 ldt : 0x%08x tflag : 0x%08x iomapb : 0x%08x\n";

#endif

LOCAL char intelId[] = "GenuineIntel";

LOCAL BRAND_ENTRY brand_table [BRAND_TABLE_SIZE] =
	{
	{0x01, " Genuine Intel Celeron(R) CPU"},
	{0x02, " Genuine Intel Pentium(R) III CPU"},
	{0x03, " Genuine Intel Pentium(R) III Xeon(TM) CPU"},
	{0x04, " Genuine Intel Pentium(R) III CPU"},
	{0x06, " Genuine Mobile Intel Pentium(R) III CPU-M"},
	{0x07, " Genuine Mobile Intel Celeron(R) CPU"},
	{0x08, " Genuine Intel Pentium(R) 4 CPU"},
	{0x09, " Genuine Intel Pentium(R) 4 CPU"},
	{0x0B, " Genuine Intel Xeon(TM) CPU"},
	{0x0E, " Genuine Intel Xeon(TM) CPU"}
	};

LOCAL const char * const dregFmt = "\n\
dr0 : 0x%08x   dr1 : 0x%08x    dr2 : 0x%08x    dr3 : 0x%08x\n\
dr4 : 0x%08x   dr5 : 0x%08x    dr6 : 0x%08x    dr7 : 0x%08x\n";


/* forward declarations */

void vxCpuShow (void);
void vxDrShow (void);
void vxSseShow (int);



/*******************************************************************************
*
* vxShowInit - initialize the Intel 80x86 and Pentium show routines
*
* This routine links Intel 80x86 and Pentium family show routines into
* the VxWorks system.  It is called automatically when the show facility
* is configured into VxWorks using either of the following methods:
*
* .iP
* INCLUDE_SHOW_ROUTINES is defined in a BSP config.h file for
* command-line builds.
*
* .iP
* The INCLUDE_SHOW_ROUTINES Component is selected for Tornado
* Project Facility builds.
* .LP
*
* INTERNAL
* This routine could be called before the RTOS is fully initialized.  The
* routine is used primarilly as a linkage symbol to pull the library into
* a build.  Other initialization should be very simple.  For example, set
* the initial values for file-scope and global variables.  Do not try to
* allocate memory in this routine.  Do not call VxWorks kernel routines.
*
* The <sysCpuId> structure is populated by calling sysCpuProbe().  Thus,
* sysCpuProbe() must be called if there is to be any hope of correctly
* determining whether to bind <_func_sseTaskRegsShow> to the SSE show
* routine.
*
* RETURNS: N/A
*/

void vxShowInit (void)
    {
    const int features_edx = sysCpuId.featuresEdx;

    /* Bind the Streaming SIMD register show routine ? */

    if ((features_edx & SSE_FLAG) || (features_edx & SSE2_FLAG))
        {
        _func_sseTaskRegsShow = (FUNCPTR) vxSseShow;
        }
    }

#if	FALSE	/* Unlike AE, 5.5 does not have TSS per task */

/*******************************************************************************
*
* vxTssShow - show content of the TSS (Task State Segment)
*
* This routine shows content of the TSS (Task State Segment)
*
* RETURNS: N/A
*
* NOMANUAL
*/

void vxTssShow
    (
    int  tid                    	/* task ID of task */
    )
    {
    WIND_TCB * pTcb;			/* pointer to TCB */
    TSS * pTss;				/* pointer to TSS */

    if (tid == 0)
	pTcb = taskIdCurrent;
    else
	pTcb = (WIND_TCB *) tid;

    pTss = pTcb->tss;			/* get the address of TSS */

    printf (tssFmt0,     pTss->link,   pTss->esp0,  pTss->ss0,
	    pTss->esp1,  pTss->ss1,    pTss->esp2,  pTss->ss2,
	    pTss->cr3,   pTss->eip,    pTss->eflags,
	    pTss->eax,   pTss->ecx,    pTss->edx,   pTss->ebx,
	    pTss->esp,   pTss->ebp,    pTss->esi,   pTss->edi,
	    pTss->cs,    pTss->ds,     pTss->ss,
	    pTss->es,    pTss->fs,     pTss->gs,
	    pTss->ldt,   pTss->tflag,  pTss->iomapb);
    }

#endif

/*******************************************************************************
*
* vxCpuShow - show CPU type, family, model, and supported features.
*
* This routine prints information on Intel Architecture processors
* and coprocessors, including CPU type, family, model, and supported
* features.
*
* INTERNAL
* This source code came from Intel AP-485 and kept as is for ease of
* future upgrade / maintenance.
*
* RETURNS: N/A
*/

void vxCpuShow (void)
    {
    char cpu_type	= (sysCpuId.signature & CPUID_FAMILY) >> 8; 
    char fpu_type	= 0;
    int  cpu_signature	= sysCpuId.signature; 
    int  features_ebx	= sysCpuId.featuresEbx;
#if	FALSE
    int  features_ecx	= sysCpuId.featuresEcx;
#endif	/* FALSE */
    int  features_edx	= sysCpuId.featuresEdx;
    int  cache_eax	= sysCpuId.cacheEax;  
    int  cache_ebx	= sysCpuId.cacheEbx;
    int  cache_ecx	= sysCpuId.cacheEcx;
    int  cache_edx	= sysCpuId.cacheEdx;
    char cpuid_flag	= (sysCpuId.highestValue != 0); 
    char intel_CPU	= FALSE; 
    char vendor_id[12]; 
    int  cache_temp; 
    int  celeron_flag;
    int  pentiumxeon_flag;
    char brand_string[48];
    int	 brand_index	= 0;


    if (sysProcessor == NONE)
        {
	printf ("sysCpuProbe() is not executed.\n");
	return;
	}

    if ((sysCoprocessor == X86FPU_387) || (sysCoprocessor == X86FPU_487))
        fpu_type = 3;

    bcopy ((char *)sysCpuId.vendorId, vendor_id, sizeof (vendor_id));
    bcopy ((char *)sysCpuId.brandString, brand_string, sizeof (brand_string));

    if (strncmp (intelId, (char *)sysCpuId.vendorId, sizeof (vendor_id)) == 0)
	intel_CPU = TRUE;

    printf ("This system has a"); 
    if (cpuid_flag == 0) 
        {
	switch (cpu_type) 
	    { 
#if	FALSE
	    case 0: 
		printf ("n 8086/8088 CPU"); 
		if (fpu_type) 
		    printf (" and an 8087 math coCPU"); 
		break; 

	    case 2: 
		printf ("n 80286 CPU"); 
		if (fpu_type) 
		    printf (" and an 80287 math coCPU"); 
		break; 
#endif	/* FALSE */

	    case 3: 
		printf ("n 80386 CPU"); 
		if (fpu_type == 2) 
		    printf (" and an 80287 math coCPU"); 
		else if (fpu_type) 
		    printf (" and an 80387 math coCPU"); 
		break; 

	    case 4: 
		if (fpu_type) 
		    printf ("n 80486DX, 80486DX2 CPU or 80487SX math coCPU"); 
		else 
		    printf ("n 80486SX CPU"); 
		break; 

	    default: 
		printf ("n unknown CPU"); 
	    }
        } 
    else 
        { 

	/* using cpuid instruction */ 

	if (brand_string[0])
	    {
	    brand_index = 0;
	    while ((brand_string[brand_index] == ' ') && (brand_index < 48))
		brand_index++;		/* remove leading Space */
	    if (brand_index != 48)
		printf ("%s", &brand_string[brand_index]);
	    }

	if (intel_CPU) 
	    { 
	    if (cpu_type == 4) 
	        { 
		switch ((cpu_signature >> 4) & 0xf) 
		    { 
		    case 0: 
		    case 1: 
			printf (" Genuine Intel486(TM) DX CPU");
		        break; 

		    case 2: 
			printf (" Genuine Intel486(TM) SX CPU"); 
			break; 

		    case 3: 
			printf (" Genuine IntelDX2(TM) CPU"); 
			break; 

		    case 4: 
			printf (" Genuine Intel486(TM) CPU"); 
			break; 

		    case 5: 
			printf (" Genuine IntelSX2(TM) CPU"); 
			break; 

		    case 7: 
			printf (" Genuine WriteBack Enhanced IntelDX2(TM) CPU");
			break; 

		    case 8: 
			printf (" Genuine IntelDX4(TM) CPU"); 
			break; 

		    default: 
			printf (" Genuine Intel486(TM) CPU"); 
		    }
	        } 
	    else if (cpu_type == 5) 
		printf (" Genuine Intel Pentium(R) CPU"); 
	    else if ((cpu_type == 6) && (((cpu_signature >> 4) & 0xf) == 1)) 
		printf (" Genuine Intel Pentium(R) Pro CPU"); 
	    else if ((cpu_type == 6) && (((cpu_signature >> 4) & 0xf) == 3)) 
		printf (" Genuine Intel Pentium(R) II CPU, model 3"); 
	    else if (((cpu_type == 6) && (((cpu_signature >> 4) & 0xf) == 5)) ||
		     ((cpu_type == 6) && (((cpu_signature >> 4) & 0xf) == 7)))
		{ 
		celeron_flag = 0; 
		pentiumxeon_flag = 0; 
		cache_temp = cache_eax & 0xFF000000; 
		if (cache_temp == 0x40000000) 
		    celeron_flag = 1; 
		if ((cache_temp >= 0x44000000) && (cache_temp <= 0x45000000)) 
		    pentiumxeon_flag = 1; 
	
		cache_temp = cache_eax & 0xFF0000; 
		if (cache_temp == 0x400000) 
		    celeron_flag = 1; 
		if ((cache_temp >= 0x440000) && (cache_temp <= 0x450000)) 
		    pentiumxeon_flag = 1;

		cache_temp = cache_eax & 0xFF00; 
		if (cache_temp == 0x4000) 
		    celeron_flag = 1; 
		if ((cache_temp >= 0x4400) && (cache_temp <= 0x4500))
		    pentiumxeon_flag = 1; 

		cache_temp = cache_ebx & 0xFF000000; 
		if (cache_temp == 0x40000000) 
		    celeron_flag = 1; 
		if ((cache_temp >= 0x44000000) && (cache_temp <= 0x45000000)) 
		    pentiumxeon_flag = 1; 

		cache_temp = cache_ebx & 0xFF0000; 
		if (cache_temp == 0x400000) 
		    celeron_flag = 1; 
		if ((cache_temp >= 0x440000) && (cache_temp <= 0x450000)) 
		    pentiumxeon_flag = 1;

		cache_temp = cache_ebx & 0xFF00; 
		if (cache_temp == 0x4000) 
		    celeron_flag = 1; 
		if ((cache_temp >= 0x4400) && (cache_temp <= 0x4500)) 
		    pentiumxeon_flag = 1; 

		cache_temp = cache_ebx & 0xFF; 
		if (cache_temp == 0x40) 
		    celeron_flag = 1; 
		if ((cache_temp >= 0x44) && (cache_temp <= 0x45)) 
		    pentiumxeon_flag = 1;

		cache_temp = cache_ecx & 0xFF000000; 
		if (cache_temp == 0x40000000) 
		    celeron_flag = 1; 
		if ((cache_temp >= 0x44000000) && (cache_temp <= 0x45000000)) 
		    pentiumxeon_flag = 1; 
	
		cache_temp = cache_ecx & 0xFF0000; 
		if (cache_temp == 0x400000) 
		    celeron_flag = 1; 
		if ((cache_temp >= 0x440000) && (cache_temp <= 0x450000)) 
		    pentiumxeon_flag = 1;

		cache_temp = cache_ecx & 0xFF00; 
		if (cache_temp == 0x4000) 
		    celeron_flag = 1; 
		if ((cache_temp >= 0x4400) && (cache_temp <= 0x4500)) 
		    pentiumxeon_flag = 1; 

		cache_temp = cache_ecx & 0xFF; 
		if (cache_temp == 0x40) 
		    celeron_flag = 1; 
		if ((cache_temp >= 0x44) && (cache_temp <= 0x45)) 
		    pentiumxeon_flag = 1;

		cache_temp = cache_edx & 0xFF000000; 
		if (cache_temp == 0x40000000) 
		    celeron_flag = 1; 
		if ((cache_temp >= 0x44000000) && (cache_temp <= 0x45000000)) 
		    pentiumxeon_flag = 1;

		cache_temp = cache_edx & 0xFF0000; 
		if (cache_temp == 0x400000) 
		    celeron_flag = 1; 
		if ((cache_temp >= 0x440000) && (cache_temp <= 0x450000)) 
		    pentiumxeon_flag = 1; 

		cache_temp = cache_edx & 0xFF00; 
		if (cache_temp == 0x4000) 
		    celeron_flag = 1; 
		if ((cache_temp >= 0x4400) && (cache_temp <= 0x4500)) 
		    pentiumxeon_flag = 1; 

		cache_temp = cache_edx & 0xFF; 
		if (cache_temp == 0x40) 
		    celeron_flag = 1; 
		if ((cache_temp >= 0x44) && (cache_temp <= 0x45)) 
		    pentiumxeon_flag = 1;

		if (celeron_flag == 1) 
		    printf (" Genuine Intel Celeron(R) CPU, model 5"); 
		else 
		    { 
		    if (pentiumxeon_flag == 1) 
		        { 
			printf (" Genuine Intel Pentium(R) "); 
			if (((cpu_signature >> 4) & 0x0f) == 5) 
			    printf ("II Xeon(TM) CPU"); 
			else 
			    printf ("III Xeon(TM) CPU");
			printf (" model 7");
		        } 
		    else 
		        { 
			if (((cpu_signature >> 4) & 0x0f) == 5)
			    { 
			    printf (" Genuine Intel Pentium(R) II CPU, "); 
			    printf ("model 5 or Intel Pentium(R) II Xeon(TM)");
			    printf (" CPU");
			    } 
			else 
			    { 
			    printf (" Genuine Intel Pentium(R) III CPU, ");
			    printf ("model 7 or Intel Pentium(R) III Xeon(TM)");
			    printf (" CPU, model 7");
			    }
		        }
		    }
	        }
	    else if ((cpu_type == 6) && (((cpu_signature >> 4) & 0xf) == 6))
		printf (" Genuine Intel Celeron(R) CPU, model 6");
	    else if ((features_ebx & 0xff) != 0) 
		{
		while ((brand_index < BRAND_TABLE_SIZE) &&
		       ((features_ebx & 0xff) != 
				       brand_table[brand_index].brand_value))
		    brand_index++;
		if (brand_index < BRAND_TABLE_SIZE)
		    {
		    if ((cpu_signature == 0x6B1) &&
		        (brand_table[brand_index].brand_value == 0x3))
			printf (" Genuine Intel Celeron(R) CPU");
		    else
		        printf ("%s", brand_table[brand_index].brand_string);
		    }
		else
		    printf ("n unknown Genuine Intel CPU"); 
		}
	    else 
		printf ("n unknown Genuine Intel CPU"); 

	    printf ("\nProcessor Family: %X", cpu_type);
	    if (cpu_type == 0xf)
		printf ("\n Extended Family: %x",(cpu_signature >> 20) & 0xff);
	    printf ("\nModel: %X", (cpu_signature>>4) & 0xf); 
	    if (((cpu_signature >> 4) & 0xf) == 0xf)
		printf ("\n Extended Model: %x", (cpu_signature >> 16) & 0xf);
	    printf ("\nStepping: %X\n", cpu_signature & 0xf); 

	    if (cpu_signature & 0x1000) 
		printf ("\nThe CPU is an OverDrive(R) CPU"); 
	    else if (cpu_signature & 0x2000) 
		printf ("\nThe CPU is the upgrade CPU in a dual CPU system"); 
	    if (features_edx & FPU_FLAG) 
		printf ("\nThe CPU contains an on-chip FPU"); 
	    if (features_edx & VME_FLAG) 
		printf ("\nThe CPU supports Virtual Mode Extensions");
	    if (features_edx & DE_FLAG) 
		printf ("\nThe CPU supports the Debugging Extensions"); 
	    if (features_edx & PSE_FLAG) 
		printf ("\nThe CPU supports Page Size Extensions"); 
	    if (features_edx & TSC_FLAG) 
		printf ("\nThe CPU supports Time Stamp Counter"); 
	    if (features_edx & MSR_FLAG) 
		printf ("\nThe CPU supports Model Specific Registers"); 
	    if (features_edx & PAE_FLAG) 
		printf ("\nThe CPU supports Physical Address Extension");
	    if (features_edx & MCE_FLAG) 
		printf ("\nThe CPU supports Machine Check Exceptions");
	    if (features_edx & CX8_FLAG) 
		printf ("\nThe CPU supports the CMPXCHG8B instruction");
	    if (features_edx & APIC_FLAG)
		printf ("\nThe CPU contains an on-chip APIC");
	    if (features_edx & SEP_FLAG)
	        { 
		if ((cpu_type == 6) && ((cpu_signature & 0xff) < 0x33))
		    printf ("\nThe CPU does not support the Fast System Call");
		else 
		    {
#if	FALSE	/* no system call in T2.2 */
    		    long long sysenterCs;
    		    long long sysenterEsp;
    		    long long sysenterEip;

		    printf ("\nThe CPU supports the Fast System Call");
		    pentiumMsrGet (MSR_SYSENTER_CS, &sysenterCs);
		    pentiumMsrGet (MSR_SYSENTER_ESP, &sysenterEsp);
		    pentiumMsrGet (MSR_SYSENTER_EIP, &sysenterEip);
		    printf (":  CS=0x%x, ESP=0x%x, EIP=0x%x", (int)sysenterCs,
			    (int)sysenterEsp, (int)sysenterEip);
#else
		    printf ("\nThe CPU supports the Fast System Call");
#endif	/* FALSE */
		    }
	        }
	    if (features_edx & MTRR_FLAG) 
		printf ("\nThe CPU supports the Memory Type Range Registers"); 
	    if (features_edx & PGE_FLAG) 
		printf ("\nThe CPU supports Page Global Enable"); 
	    if (features_edx & MCA_FLAG) 
		printf ("\nThe CPU supports the Machine Check Architecture");
	    if (features_edx & CMOV_FLAG) 
		printf ("\nThe CPU supports the Conditional Move Instruction");
	    if (features_edx & PAT_FLAG)
		printf ("\nThe CPU supports the Page Attribute Table");
	    if (features_edx & PSE36_FLAG)
		printf ("\nThe CPU supports 36-bit Page Size Extension");
	    if (features_edx & PSNUM_FLAG)
		printf ("\nThe CPU supports the CPU serial number");
	    if (features_edx & CLFLUSH_FLAG)
		printf ("\nThe CPU supports the CLFLUSH instruction");
	    if (features_edx & DTS_FLAG)
		printf ("\nThe CPU supports the Debug Trace Store feature");
	    if (features_edx & ACPI_FLAG)
		printf ("\nThe CPU supports ACPI registers in MSR space");
	    if (features_edx & MMX_FLAG)
		printf ("\nThe CPU supports Intel Architecture MMX(TM)");
	    if (features_edx & FXSR_FLAG)
		printf ("\nThe CPU supports the Fast FPP save and restore");
	    if (features_edx & SSE_FLAG)
		printf ("\nThe CPU supports the Streaming SIMD Extensions");
	    if (features_edx & SSE2_FLAG)
		printf ("\nThe CPU supports the Streaming SIMD Extensions 2");
	    if (features_edx & SS_FLAG)
		printf ("\nThe CPU supports Self-Snoop");
	    if ((features_edx & HTT_FLAG) && 
		(((features_ebx >> 16) & 0x0FF) > 1))
		printf ("\nThe CPU supports the Hyper-Threading Technology");
	    if (features_edx & TM_FLAG)
		printf ("\nThe CPU supports the Thermal Monitor");
	    }
	else 
	    { 
	    printf ("t least an 80486 CPU. \n"); 
	    printf ("It does not contain a Genuine Intel part and\n");
	    printf ("as a result, the CPUID detection information\n"); 
	    printf ("cannot be determined at this time.");
	    }
        }
    printf ("\n");
    }

/*******************************************************************************
*
* vxDrShow - show content of the Debug Registers 0 to 7
*
* This routine prints the contents of the Debug Registers 0 to 7 (DR0
* through DR7) to the standard output device.  The displayed register
* context is not associated with any particular task context, as this
* routine will simply fetch the current content of each physical debug
* register when it is called.
*
* Note that debug registers DR4 and DR5 are reserved when debug extensions
* are enabled (when the DE flag in control register CR4 is set), and
* attempts to reference the DR4 and DR5 registers cause an invalid-opcode
* exception to be generated.  When debug extensions are not enabled (when
* the DE flag is clear), these registers are aliased to debug registers
* DR6 and DR7.
*
* RETURNS: N/A
*/

void vxDrShow (void)
    {
    int dreg[8];			/* debug registers */

    vxDrGet (&dreg[0], &dreg[1], &dreg[2], &dreg[3],
    	     &dreg[4], &dreg[5], &dreg[6], &dreg[7]);

    printf (dregFmt, dreg[0], dreg[1], dreg[2], dreg[3],
    		     dreg[4], dreg[5], dreg[6], dreg[7]);
    }

/*******************************************************************************
*
* vxSseShow - print the contents of a task's SIMD registers
*
* This routine prints the contents of a task's Streaming SIMD
* Extension (SSE) register context, if any, to the standard output
* device.
*
* RETURNS: N/A
*/

void vxSseShow
    (
    int taskId          /* ID of task to display SSE registers for */
    )
    {
    const int features_edx = sysCpuId.featuresEdx;


    if ((features_edx & SSE_FLAG) || (features_edx & SSE2_FLAG))
        {
        const FP_CONTEXT *     pFpContext;

        const WIND_TCB * const pTcb = taskTcb (taskId);


        if ((pTcb != NULL) && ((pFpContext = pTcb->pFpContext) != NULL))
            {
            const FPX_CONTEXT * const pFpxContext = &(pFpContext->u.x);

            const UINT32 * pInt = (UINT32 *) &(pFpxContext->xmm[0]);
            printf ("\nxmm0   = %08x_%08x_%08x_%08x\n",
                    pInt[3], pInt[2], pInt[1], pInt[0]);

            pInt = (UINT32 *) &(pFpxContext->xmm[1]);
            printf ("xmm1   = %08x_%08x_%08x_%08x\n",
                    pInt[3], pInt[2], pInt[1], pInt[0]);

            pInt = (UINT32 *) &(pFpxContext->xmm[2]);
            printf ("xmm2   = %08x_%08x_%08x_%08x\n",
                    pInt[3], pInt[2], pInt[1], pInt[0]);

            pInt = (UINT32 *) &(pFpxContext->xmm[3]);
            printf ("xmm3   = %08x_%08x_%08x_%08x\n",
                    pInt[3], pInt[2], pInt[1], pInt[0]);

            pInt = (UINT32 *) &(pFpxContext->xmm[4]);
            printf ("xmm4   = %08x_%08x_%08x_%08x\n",
                    pInt[3], pInt[2], pInt[1], pInt[0]);

            pInt = (UINT32 *) &(pFpxContext->xmm[5]);
            printf ("xmm5   = %08x_%08x_%08x_%08x\n",
                    pInt[3], pInt[2], pInt[1], pInt[0]);

            pInt = (UINT32 *) &(pFpxContext->xmm[6]);
            printf ("xmm6   = %08x_%08x_%08x_%08x\n",
                    pInt[3], pInt[2], pInt[1], pInt[0]);

            pInt = (UINT32 *) &(pFpxContext->xmm[7]);
            printf ("xmm7   = %08x_%08x_%08x_%08x\n",
                    pInt[3], pInt[2], pInt[1], pInt[0]);

            return;
            }

        printf ("\ntask does not have SIMD context\n");
        return;
        }

    printf ("\nCPU does not support SIMD\n");
    }
