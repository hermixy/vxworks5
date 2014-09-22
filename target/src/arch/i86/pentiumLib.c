/* pentiumLib.c - Pentium and Pentium[234] library */

/* Copyright 1984-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01f,20nov01,hdn  doc clean up for 5.5
01e,01nov01,hdn  added pentiumMsrInit() and pentiumMcaEnable().
01d,21aug01,hdn  added P5/P6 PMC routines from T31 ver 01e
		 fixed a bug in pentiumMtrr[GS]et() (spr 68103)
01c,24mar99,jdi  doc: added basic formatting cmds, particularly .CS/.CE
		 to set off code snippets as per standard practice.
01b,17apr98,hdn  fixed typo.
01b,17apr98,hdn  added documentation.
01a,09jul97,hdn  written.
*/

/*
DESCRIPTION
This library provides Pentium and Pentium[234] specific routines. 

.SS "MTRR (Memory Type Range Register)"
MTRR (Memory Type Range Register) are a new feature introduced in the 
P6 family processor that allow the processor to optimize memory operations
for different types of memory, such as RAM, ROM, frame buffer memory, and
memory-mapped IO.  MTRRs configure an internal map of how physical address 
ranges are mapped to various types of memory.  The processor uses this internal
map to determine the cacheability of various physical memory locations and the
optimal method of accessing memory locations.  For example, if a memory
location is specified in an MTRR as write-through memory, the processor handles
accesses to this location as follows.  It reads data from that location in 
lines and caches the read data or maps all writes to that location to the bus
and updates the cache to maintain cache coherency.  In mapping the physical
address space with MTRRs, the processor recognizes five types of memory:
uncacheable (UC), write-combining (WC), write-through (WT),
write-protected (WP), and write-back (WB).

There is one table - sysMtrr[] in sysLib.c - and four routines to interface
the MTRR.  These four routines are:
.CS
  void pentiumMtrrEnable (void)

  void pentiumMtrrDisable (void)

  STATUS pentiumMtrrGet
      (
      MTRR * pMtrr		/@ MTRR table @/
      )

  STATUS pentiumMtrrSet (void)
      (
      MTRR * pMtrr		/@ MTRR table @/
      )
.CE
pentiumMtrrEnable() enables MTRR, pentiumMtrrDisable() disables MTRR.
pentiumMtrrGet() gets MTRRs to the specified MTRR table.
pentiumMtrrGet() sets MTRRs from the specified MTRR table.
The MTRR table is defined as follows:
.CS
typedef struct mtrr_fix         /@ MTRR - fixed range register @/
    {
    char type[8];		/@ address range: [0]=0-7 ... [7]=56-63 @/
    } MTRR_FIX;

typedef struct mtrr_var         /@ MTRR - variable range register @/
    {
    long long int base;		/@ base register @/
    long long int mask;		/@ mask register @/
    } MTRR_VAR;

typedef struct mtrr             /@ MTRR @/
    {
    int cap[2];                 /@ MTRR cap register @/
    int deftype[2];             /@ MTRR defType register @/
    MTRR_FIX fix[11];           /@ MTRR fixed range registers @/
    MTRR_VAR var[8];            /@ MTRR variable range registers @/
    } MTRR;
.CE
Fixed Range Register's type array can be one of following memory types.
MTRR_UC (uncacheable), MTRR_WC (write-combining), 
MTRR_WT (write-through), MTRR_WP (write-protected), and MTRR_WB (write-back).
MTRR is enabled in sysHwInit().

.SS "PMC (Performance Monitoring Counters)"
The P5 and P6 family of processors has two performance-monitoring counters
for use in monitoring internal hardware operations. These counters are
duration or event counters that can be programmed to count any of approximately
100 different types of events, such as the number of instructions decoded,
number of interrupts received, or number of cache loads. However, the set of
events can be counted with PMC is different in the P5 and P6 family of
processors; and the locations and bit difinitions of the related counter and
control registers are also different. So there are two set of PMC routines,
one for P6 family and one for P5 family respectively in pentiumALib. For
convenience, the PMC routines here are acting as wrappers to those routines
in pentiumALib. They will call the P5 or P6 routine depending on the processor 
type.

There are twelve routines to interface the PMC.  These twelve routines are:
.CS
  STATUS pentiumPmcStart
         (
         int pmcEvtSel0;        /@ performance event select register 0 @/
         int pmcEvtSel1;        /@ performance event select register 1 @/
         )
  STATUS pentiumPmcStart0
         (
         int pmcEvtSel0;        /@ performance event select register 0 @/
         )
  STATUS pentiumPmcStart1
         (
         int pmcEvtSel1;        /@ performance event select register 1 @/
         )
  void   pentiumPmcStop (void)
  void   pentiumPmcStop0 (void)
  void   pentiumPmcStop1 (void)
  void   pentiumPmcGet
         (
         long long int * pPmc0; /@ performance monitoring counter 0 @/
         long long int * pPmc1; /@ performance monitoring counter 1 @/
         )
  void   pentiumPmcGet0
         (
         long long int * pPmc0; /@ performance monitoring counter 0 @/
         )
  void   pentiumPmcGet1
         (
         long long int * pPmc1; /@ performance monitoring counter 1 @/
         )
  void   pentiumPmcReset (void)
  void   pentiumPmcReset0 (void)
  void   pentiumPmcReset1 (void)
.CE
pentiumPmcStart() starts both PMC0 and PMC1. pentiumPmcStart0() starts PMC0,
and pentiumPmcStart1() starts PMC1.
pentiumPmcStop() stops both PMC0 and PMC1. pentiumPmcStop0() stops PMC0, 
and pentiumPmcStop1() stops PMC1.
pentiumPmcGet() gets contents of PMC0 and PMC1.  pentiumPmcGet0() gets
contents of PMC0, and pentiumPmcGet1() gets contents of PMC1.
pentiumPmcReset() resets both PMC0 and PMC1.  pentiumPmcReset0() resets
PMC0, and pentiumPmcReset1() resets PMC1.
PMC is enabled in sysHwInit().  Selected events in the default configuration
are PMC0 = number of hardware interrupts received and PMC1 = number of
misaligned data memory references.

.SS "MSR (Model Specific Registers)"
The P5(Pentium), P6(PentiumPro, II, III), and P7(Pentium4) family processors
contain a model-specific registers (MSRs).  These registers are implement-
ation specific.  They are provided to control a variety of hardware and 
software related features including the performance monitoring, the debug
extensions, the machine check architecture, etc.

There is one routine - pentiumMsrInit() - to initialize all the MSRs.
This routine initializes all the MSRs in the processor and works on either
P5, P6 or P7 family processors.

.SS "MCA (Machine Check Architecture)"
The P5(Pentium), P6(PentiumPro, II, III), and P7(Pentium4) family processors
have a machine-check architecture that provides a mechanism for detecting 
and reporting hardware (machine) errors, such as system bus errors, ECC
errors, parity errors, cache errors and TLB errors.  It consists of a set
of model-specific registers (MSRs) that are used to set up machine checking 
and additional banks of MSRs for recording errors that are detected.
The processor signals the detection of a machine-check error by generating
a machine-check exception, which an abort class exception.  The implement-
ation of the machine-check architecture, does not ordinarily permit the 
processor to be restarted reliably after generating a machine-check 
exception.  However, the machine-check exception handler can collect
information about the machine-check error from the machine-check MSRs.

There is one routine - pentiumMcaEnable() - to enable or disable the MCA.
The routine enables or disables 1) the Machine Check Architecture and its 
Error Reporting register banks 2) the Machine Check Exception by toggling
the MCE bit in the CR4.  This routine works on either P5, P6 or P7 family.

SEE ALSO:
.I PentiumALib, "Pentium, Pentium[234] Family Developer's Manual"
*/

/* includes */

#include "vxWorks.h"
#include "regs.h"
#include "arch/i86/pentiumLib.h"
#include "vxLib.h"
#include "intLib.h"
#include "cacheLib.h"


/* defines */

#define	IA32_MISC_ENABLE_DEFAULT	(MSC_FAST_STRING_ENABLE | \
					 MSC_THERMAL_MON_ENABLE)
#define MSR_ROB_CR_BKUPTMPDR6_DEFAULT	0x4


/* externals */

IMPORT CPUID sysCpuId;
IMPORT int sysProcessor;


/* globals */

PENTIUM_MSR pentiumMsrP5[] = {
    {MSR_P5_MC_ADDR,		"P5_MC_ADDR"},
    {MSR_P5_MC_TYPE,		"P5_MC_TYPE"},
    {MSR_TSC,			"TSC"},
    {MSR_CESR,			"CESR"},
    {MSR_CTR0,			"CTR0"},
    {MSR_CTR1,			"CTR1"},
    };
INT32 pentiumMsrP5NumEnt = NELEMENTS (pentiumMsrP5);

PENTIUM_MSR pentiumMsrP6[] = {
    {MSR_P5_MC_ADDR,		"P5_MC_ADDR"},
    {MSR_P5_MC_TYPE,		"P5_MC_TYPE"},
    {MSR_TSC,			"TSC"},
    {IA32_PLATFORM_ID,		"IA32_PLATFORM_ID"},
    {MSR_APICBASE,		"APICBASE"},
    {MSR_EBL_CR_POWERON,	"EBL_CR_POWERON"},
    {MSR_TEST_CTL,		"TEST_CTL"},
    /* RW triggers the loading of the microcode update.
    {MSR_BIOS_UPDT_TRIG,	"BIOS_UPDT_TRIG"},
    */
    {MSR_BBL_CR_D0,		"BBL_CR_D0"},
    {MSR_BBL_CR_D1,		"BBL_CR_D1"},
    {MSR_BBL_CR_D2,		"BBL_CR_D2"},
    {MSR_BIOS_SIGN,		"BIOS_SIGN"},
    {MSR_PERFCTR0,		"PERFCTR0"},
    {MSR_PERFCTR1,		"PERFCTR1"},
    {MSR_MTRR_CAP,		"MTRR_CAP"},
    {MSR_BBL_CR_ADDR,		"BBL_CR_ADDR"},
    {MSR_BBL_CR_DECC,		"BBL_CR_DECC"},
    {MSR_BBL_CR_CTL,		"BBL_CR_CTL"},
    /* write only with Data=0, reading generates GPF exception.
    {MSR_BBL_CR_TRIG,		"BBL_CR_TRIG"},
    */
    {MSR_BBL_CR_BUSY,		"BBL_CR_BUSY"},
    {MSR_BBL_CR_CTL3,		"BBL_CR_CTL3"},
    {MSR_SYSENTER_CS,		"SYSENTER_CS"},
    {MSR_SYSENTER_ESP,		"SYSENTER_ESP"},
    {MSR_SYSENTER_EIP,		"SYSENTER_EIP"},
    {MSR_MCG_CAP,		"MCG_CAP"},
    {MSR_MCG_STATUS,		"MCG_STATUS"},
    /* generate GPF exception if not present.
    {MSR_MCG_CTL,		"MCG_CTL"},
    */
    {MSR_EVNTSEL0,		"EVNTSEL0"},
    {MSR_EVNTSEL1,		"EVNTSEL1"},
    {MSR_DEBUGCTLMSR,		"DEBUGCTLMSR"},
    {MSR_LASTBRANCH_FROMIP,	"LASTBRANCH_FROMIP"},
    {MSR_LASTBRANCH_TOIP,	"LASTBRANCH_TOIP"},
    {MSR_LASTINT_FROMIP,	"LASTINT_FROMIP"},
    {MSR_LASTINT_TOIP,		"LASTINT_TOIP"},
    {MSR_ROB_CR_BKUPTMPDR6,	"ROB_CR_BKUPTMPDR6"},
    {MSR_MTRR_PHYS_BASE0,	"MTRR_PHYS_BASE0"},
    {MSR_MTRR_PHYS_MASK0,	"MTRR_PHYS_MASK0"},
    {MSR_MTRR_PHYS_BASE1,	"MTRR_PHYS_BASE1"},
    {MSR_MTRR_PHYS_MASK1,	"MTRR_PHYS_MASK1"},
    {MSR_MTRR_PHYS_BASE2,	"MTRR_PHYS_BASE2"},
    {MSR_MTRR_PHYS_MASK2,	"MTRR_PHYS_MASK2"},
    {MSR_MTRR_PHYS_BASE3,	"MTRR_PHYS_BASE3"},
    {MSR_MTRR_PHYS_MASK3,	"MTRR_PHYS_MASK3"},
    {MSR_MTRR_PHYS_BASE4,	"MTRR_PHYS_BASE4"},
    {MSR_MTRR_PHYS_MASK4,	"MTRR_PHYS_MASK4"},
    {MSR_MTRR_PHYS_BASE5,	"MTRR_PHYS_BASE5"},
    {MSR_MTRR_PHYS_MASK5,	"MTRR_PHYS_MASK5"},
    {MSR_MTRR_PHYS_BASE6,	"MTRR_PHYS_BASE6"},
    {MSR_MTRR_PHYS_MASK6,	"MTRR_PHYS_MASK6"},
    {MSR_MTRR_PHYS_BASE7,	"MTRR_PHYS_BASE7"},
    {MSR_MTRR_PHYS_MASK7,	"MTRR_PHYS_MASK7"},
    {MSR_MTRR_FIX_00000,	"MTRR_FIX_00000"},
    {MSR_MTRR_FIX_80000,	"MTRR_FIX_80000"},
    {MSR_MTRR_FIX_A0000,	"MTRR_FIX_A0000"},
    {MSR_MTRR_FIX_C0000,	"MTRR_FIX_C0000"},
    {MSR_MTRR_FIX_C8000,	"MTRR_FIX_C8000"},
    {MSR_MTRR_FIX_D0000,	"MTRR_FIX_D0000"},
    {MSR_MTRR_FIX_D8000,	"MTRR_FIX_D8000"},
    {MSR_MTRR_FIX_E0000,	"MTRR_FIX_E0000"},
    {MSR_MTRR_FIX_E8000,	"MTRR_FIX_E8000"},
    {MSR_MTRR_FIX_F0000,	"MTRR_FIX_F0000"},
    {MSR_MTRR_FIX_F8000,	"MTRR_FIX_F8000"},
    {MSR_MTRR_DEFTYPE,		"MTRR_DEFTYPE"},
    /* use pentiumMcaEnable() or pentiumMcaShow()
    {MSR_MC0_CTL,		"MC0_CTL"},
    {MSR_MC0_STATUS,		"MC0_STATUS"},
    {MSR_MC0_ADDR,		"MC0_ADDR"},
    {MSR_MC0_MISC,		"MC0_MISC"},
    {MSR_MC1_CTL,		"MC1_CTL"},
    {MSR_MC1_STATUS,		"MC1_STATUS"},
    {MSR_MC1_ADDR,		"MC1_ADDR"},
    {MSR_MC1_MISC,		"MC1_MISC"},
    {MSR_MC2_CTL,		"MC2_CTL"},
    {MSR_MC2_STATUS,		"MC2_STATUS"},
    {MSR_MC2_ADDR,		"MC2_ADDR"},
    {MSR_MC2_MISC,		"MC2_MISC"},
    {MSR_MC4_CTL,		"MC4_CTL"},
    {MSR_MC4_STATUS,		"MC4_STATUS"},
    {MSR_MC4_ADDR,		"MC4_ADDR"},
    {MSR_MC4_MISC,		"MC4_MISC"},
    {MSR_MC3_CTL,		"MC3_CTL"},
    {MSR_MC3_STATUS,		"MC3_STATUS"},
    {MSR_MC3_ADDR,		"MC3_ADDR"},
    {MSR_MC3_MISC,		"MC3_MISC"},
    */
    };
INT32 pentiumMsrP6NumEnt = NELEMENTS (pentiumMsrP6);

PENTIUM_MSR pentiumMsrP7[] = {
    {IA32_P5_MC_ADDR,		"IA32_P5_MC_ADDR"},
    {IA32_P5_MC_TYPE,		"IA32_P5_MC_TYPE"},
    {IA32_TIME_STAMP_COUNTER,	"IA32_TIME_STAMP_COUNTER"},
    {IA32_PLATFORM_ID,		"IA32_PLATFORM_ID"},
    {IA32_APIC_BASE,		"IA32_APIC_BASE"},
    {MSR_EBC_HARD_POWERON,	"EBC_HARD_POWERON"},
    {MSR_EBC_SOFT_POWERON,	"EBC_SOFT_POWERON"},
    {MSR_EBC_FREQUENCY_ID,	"EBC_FREQUENCY_ID"},
    /* RW triggers the loading of the microcode update.
    {IA32_BIOS_UPDT_TRIG,	"IA32_BIOS_UPDT_TRIG"},
     */
    {IA32_BIOS_SIGN_ID,		"IA32_BIOS_SIGN_ID"},
    {IA32_MTRRCAP,		"IA32_MTRRCAP"},
    {IA32_MISC_CTL,		"IA32_MISC_CTL"},
    {IA32_SYSENTER_CS,		"IA32_SYSENTER_CS"},
    {IA32_SYSENTER_ESP,		"IA32_SYSENTER_ESP"},
    {IA32_SYSENTER_EIP,		"IA32_SYSENTER_EIP"},
    {IA32_MCG_CAP,		"IA32_MCG_CAP"},
    {IA32_MCG_STATUS,		"IA32_MCG_STATUS"},
    /* generate GPF exception if not present.
    {IA32_MCG_CTL,		"IA32_MCG_CTL"},
    {IA32_MCG_EAX,		"IA32_MCG_EAX"},
    {IA32_MCG_EBX,		"IA32_MCG_EBX"},
    {IA32_MCG_ECX,		"IA32_MCG_ECX"},
    {IA32_MCG_EDX,		"IA32_MCG_EDX"},
    {IA32_MCG_ESI,		"IA32_MCG_ESI"},
    {IA32_MCG_EDI,		"IA32_MCG_EDI"},
    {IA32_MCG_EBP,		"IA32_MCG_EBP"},
    {IA32_MCG_ESP,		"IA32_MCG_ESP"},
    {IA32_MCG_EFLAGS,		"IA32_MCG_EFLAGS"},
    {IA32_MCG_EIP,		"IA32_MCG_EIP"},
    {IA32_MCG_MISC,		"IA32_MCG_MISC"},
     */
    {IA32_THERM_CONTROL,	"IA32_THERM_CONTROL"},
    {IA32_THERM_INTERRUPT,	"IA32_THERM_INTERRUPT"},
    {IA32_THERM_STATUS,		"IA32_THERM_STATUS"},
    {IA32_MISC_ENABLE,		"IA32_MISC_ENABLE"},
    {MSR_LER_FROM_LIP,		"LER_FROM_LIP"},
    {MSR_LER_TO_LIP,		"LER_TO_LIP"},
    {IA32_DEBUGCTL,		"IA32_DEBUGCTL"},
    {MSR_LASTBRANCH_TOS,	"LASTBRANCH_TOS"},
    {MSR_LASTBRANCH_0,		"LASTBRANCH_0"},
    {MSR_LASTBRANCH_1,		"LASTBRANCH_1"},
    {MSR_LASTBRANCH_2,		"LASTBRANCH_2"},
    {MSR_LASTBRANCH_3,		"LASTBRANCH_3"},
    {IA32_MTRR_PHYSBASE0,	"IA32_MTRR_PHYSBASE0"},
    {IA32_MTRR_PHYSMASK0,	"IA32_MTRR_PHYSMASK0"},
    {IA32_MTRR_PHYSBASE1,	"IA32_MTRR_PHYSBASE1"},
    {IA32_MTRR_PHYSMASK1,	"IA32_MTRR_PHYSMASK1"},
    {IA32_MTRR_PHYSBASE2,	"IA32_MTRR_PHYSBASE2"},
    {IA32_MTRR_PHYSMASK2,	"IA32_MTRR_PHYSMASK2"},
    {IA32_MTRR_PHYSBASE3,	"IA32_MTRR_PHYSBASE3"},
    {IA32_MTRR_PHYSMASK3,	"IA32_MTRR_PHYSMASK3"},
    {IA32_MTRR_PHYSBASE4,	"IA32_MTRR_PHYSBASE4"},
    {IA32_MTRR_PHYSMASK4,	"IA32_MTRR_PHYSMASK4"},
    {IA32_MTRR_PHYSBASE5,	"IA32_MTRR_PHYSBASE5"},
    {IA32_MTRR_PHYSMASK5,	"IA32_MTRR_PHYSMASK5"},
    {IA32_MTRR_PHYSBASE6,	"IA32_MTRR_PHYSBASE6"},
    {IA32_MTRR_PHYSMASK6,	"IA32_MTRR_PHYSMASK6"},
    {IA32_MTRR_PHYSBASE7,	"IA32_MTRR_PHYSBASE7"},
    {IA32_MTRR_PHYSMASK7,	"IA32_MTRR_PHYSMASK7"},
    {IA32_MTRR_FIX64K_00000,	"IA32_MTRR_FIX64K_00000"},
    {IA32_MTRR_FIX16K_80000,	"IA32_MTRR_FIX16K_80000"},
    {IA32_MTRR_FIX16K_A0000,	"IA32_MTRR_FIX16K_A0000"},
    {IA32_MTRR_FIX4K_C0000,	"IA32_MTRR_FIX4K_C0000"},
    {IA32_MTRR_FIX4K_C8000,	"IA32_MTRR_FIX4K_C8000"},
    {IA32_MTRR_FIX4K_D0000,	"IA32_MTRR_FIX4K_D0000"},
    {IA32_MTRR_FIX4K_D8000,	"IA32_MTRR_FIX4K_D8000"},
    {IA32_MTRR_FIX4K_E0000,	"IA32_MTRR_FIX4K_E0000"},
    {IA32_MTRR_FIX4K_E8000,	"IA32_MTRR_FIX4K_E8000"},
    {IA32_MTRR_FIX4K_F8000,	"IA32_MTRR_FIX4K_F8000"},
    {IA32_CR_PAT,		"IA32_CR_PAT"},
    {IA32_MTRR_DEF_TYPE,	"IA32_MTRR_DEF_TYPE"},
    {MSR_BPU_COUNTER0,		"BPU_COUNTER0"},
    {MSR_BPU_COUNTER1,		"BPU_COUNTER1"},
    {MSR_BPU_COUNTER2,		"BPU_COUNTER2"},
    {MSR_BPU_COUNTER3,		"BPU_COUNTER3"},
    {MSR_MS_COUNTER0,		"MS_COUNTER0"},
    {MSR_MS_COUNTER1,		"MS_COUNTER1"},
    {MSR_MS_COUNTER2,		"MS_COUNTER2"},
    {MSR_MS_COUNTER3,		"MS_COUNTER3"},
    {MSR_FLAME_COUNTER0,	"FLAME_COUNTER0"},
    {MSR_FLAME_COUNTER1,	"FLAME_COUNTER1"},
    {MSR_FLAME_COUNTER2,	"FLAME_COUNTER2"},
    {MSR_FLAME_COUNTER3,	"FLAME_COUNTER3"},
    {MSR_IQ_COUNTER0,		"IQ_COUNTER0"},
    {MSR_IQ_COUNTER1,		"IQ_COUNTER1"},
    {MSR_IQ_COUNTER2,		"IQ_COUNTER2"},
    {MSR_IQ_COUNTER3,		"IQ_COUNTER3"},
    {MSR_IQ_COUNTER4,		"IQ_COUNTER4"},
    {MSR_IQ_COUNTER5,		"IQ_COUNTER5"},
    {MSR_BPU_CCCR0,		"BPU_CCCR0"},
    {MSR_BPU_CCCR1,		"BPU_CCCR1"},
    {MSR_BPU_CCCR2,		"BPU_CCCR2"},
    {MSR_BPU_CCCR3,		"BPU_CCCR3"},
    {MSR_MS_CCCR0,		"MS_CCCR0"},
    {MSR_MS_CCCR1,		"MS_CCCR1"},
    {MSR_MS_CCCR2,		"MS_CCCR2"},
    {MSR_MS_CCCR3,		"MS_CCCR3"},
    {MSR_FLAME_CCCR0,		"FLAME_CCCR0"},
    {MSR_FLAME_CCCR1,		"FLAME_CCCR1"},
    {MSR_FLAME_CCCR2,		"FLAME_CCCR2"},
    {MSR_FLAME_CCCR3,		"FLAME_CCCR3"},
    {MSR_IQ_CCCR0,		"IQ_CCCR0"},
    {MSR_IQ_CCCR1,		"IQ_CCCR1"},
    {MSR_IQ_CCCR2,		"IQ_CCCR2"},
    {MSR_IQ_CCCR3,		"IQ_CCCR3"},
    {MSR_IQ_CCCR4,		"IQ_CCCR4"},
    {MSR_IQ_CCCR5,		"IQ_CCCR5"},
    {MSR_BSU_ESCR0,		"BSU_ESCR0"},
    {MSR_BSU_ESCR1,		"BSU_ESCR1"},
    {MSR_FSB_ESCR0,		"FSB_ESCR0"},
    {MSR_FSB_ESCR1,		"FSB_ESCR1"},
    {MSR_FIRM_ESCR0,		"FIRM_ESCR0"},
    {MSR_FIRM_ESCR1,		"FIRM_ESCR1"},
    {MSR_FLAME_ESCR0,		"FLAME_ESCR0"},
    {MSR_FLAME_ESCR1,		"FLAME_ESCR1"},
    {MSR_DAC_ESCR0,		"DAC_ESCR0"},
    {MSR_DAC_ESCR1,		"DAC_ESCR1"},
    {MSR_MOB_ESCR0,		"MOB_ESCR0"},
    {MSR_MOB_ESCR1,		"MOB_ESCR1"},
    {MSR_PMH_ESCR0,		"PMH_ESCR0"},
    {MSR_PMH_ESCR1,		"PMH_ESCR1"},
    {MSR_SAAT_ESCR0,		"SAAT_ESCR0"},
    {MSR_SAAT_ESCR1,		"SAAT_ESCR1"},
    {MSR_U2L_ESCR0,		"U2L_ESCR0"},
    {MSR_U2L_ESCR1,		"U2L_ESCR1"},
    {MSR_BPU_ESCR0,		"BPU_ESCR0"},
    {MSR_BPU_ESCR1,		"BPU_ESCR1"},
    {MSR_IS_ESCR0,		"IS_ESCR0"},
    {MSR_IS_ESCR1,		"IS_ESCR1"},
    {MSR_ITLB_ESCR0,		"ITLB_ESCR0"},
    {MSR_ITLB_ESCR1,		"ITLB_ESCR1"},
    {MSR_CRU_ESCR0,		"CRU_ESCR0"},
    {MSR_CRU_ESCR1,		"CRU_ESCR1"},
    {MSR_IQ_ESCR0,		"IQ_ESCR0"},
    {MSR_IQ_ESCR1,		"IQ_ESCR1"},
    {MSR_RAT_ESCR0,		"RAT_ESCR0"},
    {MSR_RAT_ESCR1,		"RAT_ESCR1"},
    {MSR_SSU_ESCR0,		"SSU_ESCR0"},
    {MSR_MS_ESCR0,		"MS_ESCR0"},
    {MSR_MS_ESCR1,		"MS_ESCR1"},
    {MSR_TBPU_ESCR0,		"TBPU_ESCR0"},
    {MSR_TBPU_ESCR1,		"TBPU_ESCR1"},
    {MSR_TC_ESCR0,		"TC_ESCR0"},
    {MSR_TC_ESCR1,		"TC_ESCR1"},
    {MSR_IX_ESCR0,		"IX_ESCR0"},
    {MSR_IX_ESCR1,		"IX_ESCR1"},
    {MSR_ALF_ESCR0,		"ALF_ESCR0"},
    {MSR_ALF_ESCR1,		"ALF_ESCR1"},
    {MSR_CRU_ESCR2,		"CRU_ESCR2"},
    {MSR_CRU_ESCR3,		"CRU_ESCR3"},
    {MSR_CRU_ESCR4,		"CRU_ESCR4"},
    {MSR_CRU_ESCR5,		"CRU_ESCR5"},
    {MSR_TC_PRECISE_EVENT,	"TC_PRECISE_EVENT"},
    {IA32_PEBS_ENABLE,		"IA32_PEBS_ENABLE"},
    {MSR_PEBS_MATRIX_VERT,	"PEBS_MATRIX_VERT"},
    /* use pentiumMcaEnable() or pentiumMcaShow()
    {IA32_MC0_CTL,		"IA32_MC0_CTL"},
    {IA32_MC0_STATUS,		"IA32_MC0_STATUS"},
    {IA32_MC0_ADDR,		"IA32_MC0_ADDR"},
    {IA32_MC0_MISC,		"IA32_MC0_MISC"},
    {IA32_MC1_CTL,		"IA32_MC1_CTL"},
    {IA32_MC1_STATUS,		"IA32_MC1_STATUS"},
    {IA32_MC1_ADDR,		"IA32_MC1_ADDR"},
    {IA32_MC1_MISC,		"IA32_MC1_MISC"},
    {IA32_MC2_CTL,		"IA32_MC2_CTL"},
    {IA32_MC2_STATUS,		"IA32_MC2_STATUS"},
    {IA32_MC2_ADDR,		"IA32_MC2_ADDR"},
    {IA32_MC2_MISC,		"IA32_MC2_MISC"},
    {IA32_MC3_CTL,		"IA32_MC3_CTL"},
    {IA32_MC3_STATUS,		"IA32_MC3_STATUS"},
    {IA32_MC3_ADDR,		"IA32_MC3_ADDR"},
    {IA32_MC3_MISC,		"IA32_MC3_MISC"},
    */
    {IA32_DS_AREA,		"IA32_DS_AREA"},
    };
INT32 pentiumMsrP7NumEnt = NELEMENTS (pentiumMsrP7);


/* locals */

LOCAL char mtrrBusy = FALSE;		/* MTRR busy flag */


/*******************************************************************************
*
* pentiumMtrrEnable - enable MTRR (Memory Type Range Register)
*
* This routine enables the MTRR that provide a mechanism for associating the
* memory types with physical address ranges in system memory.
*
* RETURNS: N/A
*/

void pentiumMtrrEnable (void)
    {
    int oldLevel;
    int deftype[2];
    int oldCr4;

    oldLevel = intLock ();                      /* LOCK INTERRUPT */
    cacheFlush (DATA_CACHE, NULL, 0);		/* flush cache w WBINVD */
    oldCr4 = pentiumCr4Get ();			/* save CR4 */
    pentiumCr4Set (oldCr4 & ~CR4_PGE);		/* clear PGE bit */
    pentiumTlbFlush ();				/* flush TLB */

    pentiumMsrGet (MSR_MTRR_DEFTYPE, (LL_INT *)&deftype);
    deftype[0] |= (MTRR_E | MTRR_FE);		/* set enable bits */
    pentiumMsrSet (MSR_MTRR_DEFTYPE, (LL_INT *)&deftype);

    cacheFlush (DATA_CACHE, NULL, 0);		/* flush cache w WBINVD */
    pentiumTlbFlush ();				/* flush TLB */
    pentiumCr4Set (oldCr4);			/* restore CR4 */
    intUnlock (oldLevel);			/* UNLOCK INTERRUPT */
    }

/*******************************************************************************
*
* pentiumMtrrDisable - disable MTRR (Memory Type Range Register)
*
* This routine disables the MTRR that provide a mechanism for associating the
* memory types with physical address ranges in system memory.
*
* RETURNS: N/A
*/

void pentiumMtrrDisable (void)
    {
    int oldLevel;
    int deftype[2];
    int oldCr4;

    oldLevel = intLock ();                      /* LOCK INTERRUPT */
    cacheFlush (DATA_CACHE, NULL, 0);		/* flush cache w WBINVD */
    oldCr4 = pentiumCr4Get ();			/* save CR4 */
    pentiumCr4Set (oldCr4 & ~CR4_PGE);		/* clear PGE bit */
    pentiumTlbFlush ();				/* flush TLB */

    pentiumMsrGet (MSR_MTRR_DEFTYPE, (LL_INT *)&deftype);
    deftype[0] &= ~(MTRR_E | MTRR_FE);		/* clear enable bits */
    pentiumMsrSet (MSR_MTRR_DEFTYPE, (LL_INT *)&deftype);

    cacheFlush (DATA_CACHE, NULL, 0);		/* flush cache w WBINVD */
    pentiumTlbFlush ();				/* flush TLB */
    pentiumCr4Set (oldCr4);			/* restore CR4 */
    intUnlock (oldLevel);			/* UNLOCK INTERRUPT */
    }

/*******************************************************************************
*
* pentiumMtrrGet - get MTRRs to a specified MTRR table
*
* This routine gets MTRRs to a specified MTRR table with RDMSR instruction.
* The read MTRRs are CAP register, DEFTYPE register, fixed range MTRRs, and
* variable range MTRRs.
*
* RETURNS: OK, or ERROR if MTRR is being accessed.
*/

STATUS pentiumMtrrGet
    (
    MTRR * pMtrr		/* MTRR table */
    )
    {
    int ix;
    int addr = MSR_MTRR_PHYS_BASE0;

    /* mutual exclusion ON */

    if (pentiumBts (&mtrrBusy) != OK)
	return (ERROR);	

    pentiumMsrGet (MSR_MTRR_CAP, (LL_INT *)&pMtrr->cap);
    pentiumMsrGet (MSR_MTRR_DEFTYPE, (LL_INT *)&pMtrr->deftype);
    if (pMtrr->cap[0] & MTRR_FIX_SUPPORT)
	{
        pentiumMsrGet (MSR_MTRR_FIX_00000, (LL_INT *)&pMtrr->fix[0].type);
        pentiumMsrGet (MSR_MTRR_FIX_80000, (LL_INT *)&pMtrr->fix[1].type);
        pentiumMsrGet (MSR_MTRR_FIX_A0000, (LL_INT *)&pMtrr->fix[2].type);
        pentiumMsrGet (MSR_MTRR_FIX_C0000, (LL_INT *)&pMtrr->fix[3].type);
        pentiumMsrGet (MSR_MTRR_FIX_C8000, (LL_INT *)&pMtrr->fix[4].type);
        pentiumMsrGet (MSR_MTRR_FIX_D0000, (LL_INT *)&pMtrr->fix[5].type);
        pentiumMsrGet (MSR_MTRR_FIX_D8000, (LL_INT *)&pMtrr->fix[6].type);
        pentiumMsrGet (MSR_MTRR_FIX_E0000, (LL_INT *)&pMtrr->fix[7].type);
        pentiumMsrGet (MSR_MTRR_FIX_E8000, (LL_INT *)&pMtrr->fix[8].type);
        pentiumMsrGet (MSR_MTRR_FIX_F0000, (LL_INT *)&pMtrr->fix[9].type);
        pentiumMsrGet (MSR_MTRR_FIX_F8000, (LL_INT *)&pMtrr->fix[10].type);
	}
    for (ix = 0; ix < (pMtrr->cap[0] & MTRR_VCNT); ix++)
	{
        pentiumMsrGet (addr++, &pMtrr->var[ix].base);
        pentiumMsrGet (addr++, &pMtrr->var[ix].mask);
	}

    /* mutual exclusion OFF */

    (void) pentiumBtc (&mtrrBusy);
    return (OK);
    }

/*******************************************************************************
*
* pentiumMtrrSet - set MTRRs from specified MTRR table with WRMSR instruction.
*
* This routine sets MTRRs from specified MTRR table with WRMSR instruction.
* The written MTRRs are DEFTYPE register, fixed range MTRRs, and variable
* range MTRRs.
*
* RETURNS: OK, or ERROR if MTRR is enabled or being accessed.
*/

STATUS pentiumMtrrSet 
    (
    MTRR * pMtrr		/* MTRR table */
    )
    {
    int ix;
    int addr = MSR_MTRR_PHYS_BASE0;

    /* MTRR should be disabled */

    pentiumMsrGet (MSR_MTRR_DEFTYPE, (LL_INT *)&pMtrr->deftype);
    if ((pMtrr->deftype[0] & (MTRR_E | MTRR_FE)) != 0)
	return (ERROR);

    /* mutual exclusion ON */

    if (pentiumBts (&mtrrBusy) != OK)
	return (ERROR);	

    pentiumMsrSet (MSR_MTRR_DEFTYPE, (LL_INT *)&pMtrr->deftype);
    pentiumMsrGet (MSR_MTRR_CAP, (LL_INT *)&pMtrr->cap);
    if (pMtrr->cap[0] & MTRR_FIX_SUPPORT)
	{
        pentiumMsrSet (MSR_MTRR_FIX_00000, (LL_INT *)&pMtrr->fix[0].type);
        pentiumMsrSet (MSR_MTRR_FIX_80000, (LL_INT *)&pMtrr->fix[1].type);
        pentiumMsrSet (MSR_MTRR_FIX_A0000, (LL_INT *)&pMtrr->fix[2].type);
        pentiumMsrSet (MSR_MTRR_FIX_C0000, (LL_INT *)&pMtrr->fix[3].type);
        pentiumMsrSet (MSR_MTRR_FIX_C8000, (LL_INT *)&pMtrr->fix[4].type);
        pentiumMsrSet (MSR_MTRR_FIX_D0000, (LL_INT *)&pMtrr->fix[5].type);
        pentiumMsrSet (MSR_MTRR_FIX_D8000, (LL_INT *)&pMtrr->fix[6].type);
        pentiumMsrSet (MSR_MTRR_FIX_E0000, (LL_INT *)&pMtrr->fix[7].type);
        pentiumMsrSet (MSR_MTRR_FIX_E8000, (LL_INT *)&pMtrr->fix[8].type);
        pentiumMsrSet (MSR_MTRR_FIX_F0000, (LL_INT *)&pMtrr->fix[9].type);
        pentiumMsrSet (MSR_MTRR_FIX_F8000, (LL_INT *)&pMtrr->fix[10].type);
	}
    for (ix = 0; ix < (pMtrr->cap[0] & MTRR_VCNT); ix++)
	{
        pentiumMsrSet (addr++, &pMtrr->var[ix].base);
        pentiumMsrSet (addr++, &pMtrr->var[ix].mask);
	}

    /* mutual exclusion OFF */

    (void) pentiumBtc (&mtrrBusy);
    return (OK);
    }

/*******************************************************************************
*
* pentiumPmcStart - start both PMC0 and PMC1
*
* SYNOPSIS
* \ss
* STATUS pentiumPmcStart (pmcEvtSel0, pmcEvtSel1)
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
STATUS   pentiumPmcStart
    (
    int pmcEvtSel0,
    int pmcEvtSel1
    )
    {
    STATUS retVal;

    retVal = ERROR;
    if (sysCpuId.featuresEdx & CPUID_MSR)
        {
        if (sysProcessor == X86CPU_PENTIUM)
	    {
            if(pentiumP5PmcStart0(pmcEvtSel0) == OK    
                    && pentiumP5PmcStart1(pmcEvtSel1) == OK) 
                retVal = OK;
            }
        else if (sysProcessor == X86CPU_PENTIUMPRO)
            retVal = pentiumP6PmcStart(pmcEvtSel0, pmcEvtSel1);
        }

    return (retVal);
    }
   
/*******************************************************************************
*
* pentiumPmcStart0 - start PMC0
*
* SYNOPSIS
* \ss
* STATUS pentiumPmcStart0 (pmcEvtSel0)
*       int pmcEvtSel0;           /@ PMC0 control and event select @/
* \se
*
* This routine starts PMC0 (Performance Monitoring Counter 0) 
* by writing specified PMC0 events to Performance Event Select Registers. 
* The only parameter is the content of Performance Event Select Register.
*
* RETURNS: OK or ERROR if PMC is already started.
*/
STATUS   pentiumPmcStart0
    (
    int pmcEvtSel0
    )
    {

    if ((sysCpuId.featuresEdx & CPUID_MSR) && (sysProcessor == X86CPU_PENTIUM))
       return (pentiumP5PmcStart0(pmcEvtSel0));            
    else
       return (ERROR);
    }

/*******************************************************************************
*
* pentiumPmcStart1 - start PMC1
*
* SYNOPSIS
* \ss
* STATUS pentiumPmcStart1 (pmcEvtSel1)
*       int pmcEvtSel1;           /@ PMC1 control and event select @/
* \se
*
* This routine starts PMC1 (Performance Monitoring Counter 0) 
* by writing specified PMC1 events to Performance Event Select Registers. 
* The only parameter is the content of Performance Event Select Register.
*
* RETURNS: OK or ERROR if PMC1 is already started.
*/
STATUS   pentiumPmcStart1
    (
    int pmcEvtSel1
    )
    {
    if ((sysCpuId.featuresEdx & CPUID_MSR) && (sysProcessor == X86CPU_PENTIUM))
        return (pentiumP5PmcStart1(pmcEvtSel1));                
    else
        return (ERROR);
    }

/*******************************************************************************
*
* pentiumPmcStop - stop both PMC0 and PMC1
*
* SYNOPSIS
* \ss
* void pentiumPmcStop (void)
* \se
*
* This routine stops both PMC0 (Performance Monitoring Counter 0)
* and PMC1 by clearing two Performance Event Select Registers.
*
* RETURNS: N/A
*/
void pentiumPmcStop (void)
    {
    if (sysCpuId.featuresEdx & CPUID_MSR)
	{
        if (sysProcessor == X86CPU_PENTIUM)
            {
            pentiumP5PmcStop0();
            pentiumP5PmcStop1();
            }
        else if (sysProcessor == X86CPU_PENTIUMPRO)
            pentiumP6PmcStop();
        }
    }

/*******************************************************************************
*
* pentiumPmcStop0 - stop PMC0
*
* SYNOPSIS
* \ss
* void pentiumPmcStop0 (void)
* \se
*
* This routine stops only PMC0 (Performance Monitoring Counter 0)
* by clearing the PMC0 bits of Control and Event Select Register.
*
* RETURNS: N/A
*/
void pentiumPmcStop0 (void)
    {
    if ((sysCpuId.featuresEdx & CPUID_MSR) && (sysProcessor == X86CPU_PENTIUM))
        pentiumP5PmcStop0();
    }

/*******************************************************************************
*
* pentiumPmcStop1 - stop PMC1
*
* SYNOPSIS
* \ss
* void pentiumPmcStop1 (void)
* \se
*
* This routine stops only PMC1 (Performance Monitoring Counter 1)
* by clearing the PMC1 bits of Control and Event Select Register.
*
* RETURNS: N/A
*/
void pentiumPmcStop1 (void)
    {
    if (sysCpuId.featuresEdx & CPUID_MSR)
        {
        if (sysProcessor == X86CPU_PENTIUM)
            pentiumP5PmcStop1();
        else if (sysProcessor == X86CPU_PENTIUMPRO)
            pentiumP6PmcStop1();
        }
    }

/*******************************************************************************
*
* pentiumPmcGet - get the contents of PMC0 and PMC1
*
* SYNOPSIS
* \ss
* void pentiumPmcGet (pPmc0, pPmc1)
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
void pentiumPmcGet
    (
    long long int * pPmc0, 
    long long int * pPmc1
    )
    {
    if (sysCpuId.featuresEdx & CPUID_MSR)
        {
        if (sysProcessor == X86CPU_PENTIUM)
            pentiumP5PmcGet(pPmc0, pPmc1);
        else if (sysProcessor == X86CPU_PENTIUMPRO)
            pentiumP6PmcGet(pPmc0, pPmc1);
        }
    }

/*******************************************************************************
*
* pentiumPmcGet0 - get the contents of PMC0
*
* SYNOPSIS
* \ss
* void pentiumPmcGet0 (pPmc0)
*     long long int * pPmc0;            /@ Performance Monitoring Counter 0 @/
* \se
*
* This routine gets the contents of PMC0 (Performance Monitoring Counter 0).
* The parameter is a pointer of 64Bit variable to store the content of
* the Counter.
*
* RETURNS: N/A
*/
void pentiumPmcGet0
    (
    long long int * pPmc0
    )
    {
    if (sysCpuId.featuresEdx & CPUID_MSR)
        {
        if (sysProcessor == X86CPU_PENTIUM)
            pentiumP5PmcGet0(pPmc0);
        else if (sysProcessor == X86CPU_PENTIUMPRO)
            pentiumP6PmcGet0(pPmc0);
        }
    }

/*******************************************************************************
*
* pentiumPmcGet1 - get the contents of PMC1
*
* SYNOPSIS
* \ss
* void pentiumPmcGet1 (pPmc1)
*     long long int * pPmc1;            /@ Performance Monitoring Counter 1 @/
* \se
*
* This routine gets a content of PMC1 (Performance Monitoring Counter 1).
* Parameter is a pointer of 64Bit variable to store the content of the Counter.
*
* RETURNS: N/A
*/
void pentiumPmcGet1
    (
    long long int * pPmc1
    )
    {
    if (sysCpuId.featuresEdx & CPUID_MSR)
        {
        if (sysProcessor == X86CPU_PENTIUM)
            pentiumP5PmcGet1(pPmc1);
        else if (sysProcessor == X86CPU_PENTIUMPRO)
            pentiumP6PmcGet1(pPmc1);
        }
    }
    
/*******************************************************************************
*
* pentiumPmcReset - reset both PMC0 and PMC1
*
* SYNOPSIS
* \ss
* void pentiumPmcReset (void)
* \se
* 
* This routine resets both PMC0 (Performance Monitoring Counter 0) and PMC1.
*
* RETURNS: N/A
*/
void pentiumPmcReset (void)
    {
    if (sysCpuId.featuresEdx & CPUID_MSR)
        {
        if (sysProcessor == X86CPU_PENTIUM)
            pentiumP5PmcReset();
        else if (sysProcessor == X86CPU_PENTIUMPRO)
            pentiumP6PmcReset();
        }
    }

/*******************************************************************************
*
* pentiumPmcReset0 - reset PMC0
*
* SYNOPSIS
* \ss
* void pentiumPmcReset0 (void)
* \se
*
* This routine resets PMC0 (Performance Monitoring Counter 0).
*
* RETURNS: N/A 
*/
void pentiumPmcReset0 (void)
    {
    if (sysCpuId.featuresEdx & CPUID_MSR)
        {
        if (sysProcessor == X86CPU_PENTIUM)
            pentiumP5PmcReset0();
        else if (sysProcessor == X86CPU_PENTIUMPRO)
            pentiumP6PmcReset0();
        }
    }

/*******************************************************************************
*
* pentiumPmcReset1 - reset PMC1
*
* SYNOPSIS
* \ss
* void pentiumPmcReset1 (void)
* \se
*
* This routine resets PMC1 (Performance Monitoring Counter 1).
*
* RETURNS: N/A
*/
void pentiumPmcReset1 (void)
    {
    if (sysCpuId.featuresEdx & CPUID_MSR)
        {
        if (sysProcessor == X86CPU_PENTIUM)
            pentiumP5PmcReset1();
        else if (sysProcessor == X86CPU_PENTIUMPRO)
            pentiumP6PmcReset1();
        }
    }

/*******************************************************************************
*
* pentiumMsrInit - initialize all the MSRs (Model Specific Register)
*
* This routine initializes all the MSRs in the processor.
* This routine works on either P5, P6 or P7 family processors.
*
* RETURNS: OK, or ERROR if RDMSR/WRMSR instructions are not supported.
*/

STATUS pentiumMsrInit (void)
    {
    PENTIUM_MSR * pMsr;		/* pointer to the MSR table */
    UINT32 value[2] = {0,0};
    UINT32 zero[2]  = {0,0};
    int ix;

    /* just return if RDMSR and WRMSR instruction are not supported */

    if ((sysCpuId.featuresEdx & CPUID_MSR) == 0)
	return (ERROR);

    switch (sysProcessor)
	{
	case X86CPU_PENTIUM:
	    pMsr = pentiumMsrP5;
    	    for (ix = 0; ix < pentiumMsrP5NumEnt; ix++, pMsr++)
	        {
	        switch (pMsr->addr)
	            {
	            case MSR_P5_MC_ADDR:	/* disable the MCA */
		        pentiumMcaEnable (FALSE);
	                break;

	            case MSR_CESR:		/* disable the PM */
	                pentiumMsrSet (pMsr->addr, (LL_INT *)&zero);
	                break;

	            default:
	                break;
	            }
	        }
	    break;

	case X86CPU_PENTIUMPRO:			/* PENTIUM[23] */
	    pMsr = pentiumMsrP6;
    	    for (ix = 0; ix < pentiumMsrP6NumEnt; ix++, pMsr++)
	        {
	        switch (pMsr->addr)
	            {
	            case MSR_APICBASE:		/* don't touch */
		        /* bit-11 can't be re-enabled except by hard reset */
	                break;

	            case MSR_ROB_CR_BKUPTMPDR6:	/* set the default value */
	                pentiumMsrGet (pMsr->addr, (LL_INT *)&value);
	  	        value[0] = MSR_ROB_CR_BKUPTMPDR6_DEFAULT;
	                pentiumMsrSet (pMsr->addr, (LL_INT *)&value);
	                break;

	            case MSR_MCG_CAP:		/* disable the MCA */
		        pentiumMcaEnable (FALSE);
	                break;

	            case MSR_DEBUGCTLMSR:	/* disable the debug features */
	            case MSR_EVNTSEL0:		/* disable the PM */
	            case MSR_EVNTSEL1:		/* disable the PM */
	            case MSR_PERFCTR0:		/* disable the PM */
	            case MSR_PERFCTR1:		/* disable the PM */
	                pentiumMsrSet (pMsr->addr, (LL_INT *)&zero);
	                break;

	            default:
	                break;
	            }
	        }
	    break;

	case X86CPU_PENTIUM4:
	    pMsr = pentiumMsrP7;
    	    for (ix = 0; ix < pentiumMsrP7NumEnt; ix++, pMsr++)
	        {
	        switch (pMsr->addr)
	            {
	            case IA32_APIC_BASE:	/* don't touch */
		        /* bit-11 can't be re-enabled except by hard reset */
	                break;

	            case IA32_MISC_ENABLE:	/* set the default value */
	                pentiumMsrGet (pMsr->addr, (LL_INT *)&value);
	  	        value[0] = IA32_MISC_ENABLE_DEFAULT;
	                pentiumMsrSet (pMsr->addr, (LL_INT *)&value);
	                break;

	            case IA32_MCG_CAP:		/* disable the MCA */
		        pentiumMcaEnable (FALSE);
	                break;

	            case IA32_MISC_CTL:		/* disable the serial number */
	            case IA32_THERM_INTERRUPT:	/* disable the thermal int. */
	            case IA32_DEBUGCTL:		/* disable the debug features */
	            case MSR_TC_PRECISE_EVENT:	/* disable the FE tagging */
	            case IA32_PEBS_ENABLE:	/* disable the PEBS */
	            case MSR_BPU_CCCR0:		/* disable the PM */
	            case MSR_BPU_CCCR1:		/* disable the PM */
	            case MSR_BPU_CCCR2:		/* disable the PM */
	            case MSR_BPU_CCCR3:		/* disable the PM */
	            case MSR_MS_CCCR0:		/* disable the PM */
	            case MSR_MS_CCCR1:		/* disable the PM */
	            case MSR_MS_CCCR2:		/* disable the PM */
	            case MSR_MS_CCCR3:		/* disable the PM */
	            case MSR_FLAME_CCCR0:	/* disable the PM */
	            case MSR_FLAME_CCCR1:	/* disable the PM */
	            case MSR_FLAME_CCCR2:	/* disable the PM */
	            case MSR_FLAME_CCCR3:	/* disable the PM */
	            case MSR_IQ_CCCR0:		/* disable the PM */
	            case MSR_IQ_CCCR1:		/* disable the PM */
	            case MSR_IQ_CCCR2:		/* disable the PM */
	            case MSR_IQ_CCCR3:		/* disable the PM */
	            case MSR_IQ_CCCR4:		/* disable the PM */
	            case MSR_IQ_CCCR5:		/* disable the PM */
	                pentiumMsrSet (pMsr->addr, (LL_INT *)&zero);
	                break;

	            default:
	                break;
	            }
	        }
	    break;

	default:
	    break;
	}

    return (OK);
    }

/*******************************************************************************
*
* pentiumMcaEnable - enable/disable the MCA (Machine Check Architecture)
*
* This routine enables/disables 1) the Machine Check Architecture and its 
* Error Reporting register banks 2) the Machine Check Exception by toggling
* the MCE bit in the CR4.  This routine works on either P5, P6 or P7 family.
*
* RETURNS: N/A
*/

void pentiumMcaEnable 
    (
    BOOL enable			/* TRUE to enable, FALSE to disable the MCA */
    )
    {
    UINT32 zero[2] = {0x00000000,0x00000000};
    UINT32 one[2]  = {0xffffffff,0xffffffff};
    UINT32 cap[2];		/* MCG_CAP */
    int mcaBanks;		/* MCA Error-Reporting Bank Registers */
    int ix;

    if ((sysCpuId.featuresEdx & CPUID_MCE) == CPUID_MCE)
	{
        if ((sysCpuId.featuresEdx & CPUID_MCA) == CPUID_MCA)
	    {
	    pentiumMsrGet (MSR_MCG_CAP, (LL_INT *)&cap);

	    /* enable/disable all the machine-check features */

	    if (cap[0] & MCG_CTL_P)
		{
		if (enable)
		    pentiumMsrSet (MSR_MCG_CTL, (LL_INT *)&one);
		else
		    pentiumMsrSet (MSR_MCG_CTL, (LL_INT *)&zero);
		}

	    mcaBanks = cap[0] & MCG_COUNT;	/* get number of banks */

	    /* enable all error-logging banks except MC0_CTL register */

	    for (ix = 1; ix < mcaBanks; ix++)
		pentiumMsrSet (MSR_MC0_CTL+(ix * 4), (LL_INT *)&one);
	
	    /* clear all errors */

	    for (ix = 0; ix < mcaBanks; ix++)
		pentiumMsrSet (MSR_MC0_STATUS+(ix * 4), (LL_INT *)&zero);

	    }
	
	/* enable/disable the MCE exception */

	if (enable)
	    vxCr4Set (vxCr4Get () | CR4_MCE);
	else
	    vxCr4Set (vxCr4Get () & ~CR4_MCE);
	}
    }

