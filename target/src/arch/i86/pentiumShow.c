/* pentiumShow.c - Pentium and Pentium[234] specific show routines */

/* Copyright 1984-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01g,01nov01,hdn  added pentiumMsrShow()
01f,21aug01,hdn  updated CPUID structure for PENTIUM4
01e,05oct98,jmp  doc: cleanup.
01d,20may98,hdn  changed sysCpuid to sysCpuId.
01c,17apr98,hdn  fixed typo.
01b,17apr98,hdn  added documentation.
01a,09jul97,hdn  written.
*/

/*
DESCRIPTION
This library provides Pentium and Pentium[234] specific show routines. 

pentiumMcaShow() shows Machine Check Global Control Registers and
Error Reporting Register Banks.  pentiumPmcShow() shows PMC0 and
PMC1, and reset them if the parameter zap is TRUE.

SEE ALSO:
.pG "Configuration"
*/


/* includes */

#include "vxWorks.h"
#include "regs.h"
#include "arch/i86/pentiumLib.h"
#include "stdio.h"
#include "fioLib.h"


/* defines */


/* externals */

IMPORT CPUID		sysCpuId;
IMPORT int		sysProcessor;
IMPORT PENTIUM_MSR	pentiumMsrP5[];
IMPORT INT32		pentiumMsrP5NumEnt;
IMPORT PENTIUM_MSR	pentiumMsrP6[];
IMPORT INT32		pentiumMsrP6NumEnt;
IMPORT PENTIUM_MSR	pentiumMsrP7[];
IMPORT INT32		pentiumMsrP7NumEnt;


/* locals */


/*******************************************************************************
*
* pentiumMcaShow - show MCA (Machine Check Architecture) registers 
*
* This routine shows Machine-Check global control registers and Error-Reporting
* register banks.  Number of the Error-Reporting register banks is kept in a
* variable mcaBanks.  MCi_ADDR and MCi_MISC registers in the Error-Reporting
* register bank are showed if MCi_STATUS indicates that these registers are
* valid.
*
* RETURNS: N/A
*/

void pentiumMcaShow (void)
    {
    UINT32 zero[2] = {0,0};
    UINT32 cap[2];		/* MCG_CAP */
    UINT32 stat[2];		/* MCG_STATUS, MCI_STATUS */
    UINT32 addr[2];		/* MCI_ADDR, P5_MC_ADDR */
    UINT32 misc[2];		/* MCI_MISC, P5_MC_TYPE */
    int mcaBanks;		/* MCA Error-Reporting Bank Registers */
    int ix;

    if ((sysCpuId.featuresEdx & CPUID_MCE) == CPUID_MCE)
	{
        if ((sysCpuId.featuresEdx & CPUID_MCA) == CPUID_MCA)
	    {
	    pentiumMsrGet (MSR_MCG_CAP, (LL_INT *)&cap);
	    mcaBanks = cap[0] & MCG_COUNT;	/* get number of banks */

	    pentiumMsrGet (MSR_MCG_STATUS, (LL_INT *)&stat);
	    printExc ("MCG_STATUS: 0x%.8x%.8x\n", stat[1], stat[0], 0, 0, 0);

	    for (ix = 0; ix < mcaBanks; ix++)
		{
		pentiumMsrGet (MSR_MC0_STATUS + (ix * 4), (LL_INT *)&stat);
		if (stat[1] & MCI_VAL)
		    {
		    if (stat[1] & MCI_ADDRV)
			pentiumMsrGet (MSR_MC0_ADDR + (ix * 4), 
				   (LL_INT *)&addr);
		    if (stat[1] & MCI_MISCV)
			pentiumMsrGet (MSR_MC0_MISC + (ix * 4), 
				   (LL_INT *)&misc);
		    }
		pentiumMsrSet (MSR_MC0_STATUS + (ix * 4), (LL_INT *)&zero);
		pentiumSerialize ();

		printExc ("MC%d_STATUS: 0x%.8x%.8x\n", ix, stat[1], stat[0],
			  0, 0);
		if (stat[1] & MCI_ADDRV)
		    printExc ("MC%d_ADDR: 0x%.8x%.8x\n", ix, addr[1], addr[0],
			      0, 0);
		if (stat[1] & MCI_MISCV)
		    printExc ("MC%d_MISC: 0x%.8x%.8x\n", ix, misc[1], misc[0],
			      0, 0);
		}
	    }
	else
	    {
            pentiumMsrGet (MSR_P5_MC_ADDR, (LL_INT *)&addr);
            pentiumMsrGet (MSR_P5_MC_TYPE, (LL_INT *)&misc);
	    printExc ("P5_MC_ADDR: 0x%.8x%.8x\n", addr[1], addr[0], 0, 0, 0);
	    printExc ("P5_MC_TYPE: 0x%.8x%.8x\n", misc[1], misc[0], 0, 0, 0);
	    }
	pentiumMsrGet (MSR_MCG_STATUS, (LL_INT *)&stat);
	stat[0] &= ~MCG_MCIP;
	pentiumMsrSet (MSR_MCG_STATUS, (LL_INT *)&stat);
	}
    }

/*******************************************************************************
*
* pentiumPmcShow - show PMCs (Performance Monitoring Counters)
*
* This routine shows Performance Monitoring Counter 0 and 1.
* Monitored events are selected by Performance Event Select Registers in 
* in pentiumPmcStart ().  These counters are cleared to 0 if the parameter "zap"
* is TRUE.
*
* RETURNS: N/A
*/

void pentiumPmcShow
    (
    BOOL zap			/* 1: reset PMC0 and PMC1 */
    )
    {
    LL_INT pmcCtr0;	/* PMC0 */
    LL_INT pmcCtr1;	/* PMC1 */

    pentiumPmcGet (&pmcCtr0, &pmcCtr1);
    printf ("PMC0=%d, PMC1=%d\n", (int)pmcCtr0, (int)pmcCtr1);
    if (zap)
	pentiumPmcReset ();
    }

/*******************************************************************************
*
* pentiumMsrShow - show all the MSR (Model Specific Register)
*
* This routine shows all the MSRs in the Pentium and Pentium[234].
*
* RETURNS: N/A
*/

void pentiumMsrShow (void)
    {
    PENTIUM_MSR * pMsr;		/* pointer to the MSR table */
    INT32 msrNumEnt;		/* number of entries */
    UINT32 value[2];
    int ix;

    if ((sysCpuId.featuresEdx & CPUID_MSR) == 0)
	{
        printf ("This CPU does not support RDMSR and WRMSR\n");
	return;
	}

    switch (sysProcessor)
	{
	case X86CPU_PENTIUM:
	    pMsr      = pentiumMsrP5;
	    msrNumEnt = pentiumMsrP5NumEnt;
	    break;

	case X86CPU_PENTIUMPRO:
	    pMsr      = pentiumMsrP6;
	    msrNumEnt = pentiumMsrP6NumEnt;
	    break;

	case X86CPU_PENTIUM4:
	    pMsr      = pentiumMsrP7;
	    msrNumEnt = pentiumMsrP7NumEnt;
	    break;

	default:
	    printf ("This CPU(sysProcessor=%d) doesn't have MSRs\n", sysProcessor);
	    return;
	}

    for (ix = 0; ix < msrNumEnt; ix++, pMsr++)
	{
	pentiumMsrGet (pMsr->addr, (LL_INT *)&value);
	printf ("addr=0x%04x [31:0]=0x%08x [63:32]=0x%08x name=%s\n",
	        (UINT16)pMsr->addr, value[0], value[1], pMsr->name);
	}
    }

