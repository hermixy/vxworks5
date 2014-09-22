/* excArchLib.c - i80X86 exception handling facilities */

/* Copyright 1984-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01j,20nov01,hdn  doc clean up for 5.5
01i,28aug01,hdn  added Pentium4/SSE/SSE2/MCE support
		 added esp, ss, esp0, cr[23], esp0[07] to EXC_INFO.
		 added "error" parameter to exc{Exc,Int}Handle.
01h,29jul96,sbs  Made windview conditionally compile.
01g,15nov94,hdn  WindView instrumetation.
01f,01nov94,hdn  added X86CPU_PENTIUM of sysProcessor.
01e,29may94,hdn  changed I80486 to sysProcessor.
01d,14nov93,hdn  deleted excExcepHook.
01c,03jun93,hdn  updated to 5.1
                  -changed functions to ansi style
		  -changed includes to have absolute path from h/
		  -fixed #else and #endif
		  -changed READ, WRITE and UPDATE to O_RDONLY O_WRONLY O_RDWR
		  -changed VOID to void
		  -changed copyright notice
01b,26mar93,hdn  supported 486's exception, alignment check.
01a,28feb92,hdn  written based on TRON, 68k version.
*/

/*
This module contains I80X86 architecture dependent portions of the
exception handling facilities.  See excLib for the portions that are
architecture independent.

SEE ALSO: dbgLib, sigLib, intLib, "Debugging"
*/

#include "vxWorks.h"
#include "esf.h"
#include "iv.h"
#include "sysLib.h"
#include "intLib.h"
#include "taskLib.h"
#include "qLib.h"
#include "errno.h"
#include "string.h"
#include "vxLib.h"
#include "logLib.h"
#include "rebootLib.h"
#include "private/funcBindP.h"
#include "private/kernelLibP.h"
#include "private/taskLibP.h"
#include "private/sigLibP.h"


/* externals */

IMPORT UINT	sysProcessor;           /* 0=386, 1=486, 2=P5, 4=P6, 5=P7 */
IMPORT char	excCallTbl [];		/* table of Calls in excALib.s */
IMPORT UINT	sysIntIdtType;		/* interrupt or trap gate */
IMPORT CPUID	sysCpuId;		/* CPUID features */
IMPORT int	sysCsInt;		/* CS for interrupt */
IMPORT int	sysCsExc;		/* CS for exception */


/* globals */


/* locals */


/* forward declarations */

LOCAL void	excGetInfoFromESF (int vecNum, ESF0 * pEsf, REG_SET * pRegs,
			      	   EXC_INFO * pExcInfo, BOOL error);
LOCAL BOOL	programError (int vecNum);


/*******************************************************************************
*
* excVecInit - initialize the exception/interrupt vectors
*
* This routine sets all exception vectors to point to the appropriate
* default exception handlers.  These handlers will safely trap and report
* exceptions caused by program errors or unexpected hardware interrupts.
* All vectors from vector 0 (address 0x0000) to 255 (address 0x07f8) are
* initialized.
*
* WHEN TO CALL
* This routine is usually called from the system start-up routine
* usrInit() in usrConfig, before interrupts are enabled.
*
* RETURNS: OK (always).
*/

STATUS excVecInit (void)
    {
    FAST int vecNum;

    /* 
     * make all exception vectors point to either generic exception or
     * interrupt trap handler
     */

    for (vecNum = LOW_VEC; vecNum <= HIGH_VEC; ++vecNum)
	{
	intVecSet2 ((FUNCPTR *)INUM_TO_IVEC (vecNum),
		    (FUNCPTR) &excCallTbl[vecNum * 5],
		    programError (vecNum) ? IDT_TRAP_GATE : sysIntIdtType,
		    programError (vecNum) ? sysCsExc : sysCsInt);
	}

    /* enable the Machine Check exception if it is supported */

    if (sysCpuId.featuresEdx & CPUID_MCE)
	vxCr4Set (vxCr4Get () | CR4_MCE);

    /* enable the SSE or SSE+SSE2 if it is supported */

    if ((sysCpuId.featuresEdx & CPUID_SSE) ||
        (sysCpuId.featuresEdx & CPUID_SSE2))
	vxCr4Set (vxCr4Get () | CR4_OSXMMEXCEPT);

    return (OK);
    }

/*******************************************************************************
*
* excExcHandle - interrupt level handling of exceptions
*
* This routine handles exception traps.  It is never to be called except
* from the special assembly language interrupt stub routine.
*
* It prints out a bunch of pertinent information about the trap that
* occurred via excTask.
*
* Note that this routine runs in the context of the task that got the exception.
*
* NOMANUAL
*/

void excExcHandle
    (
    int		vecNum,	/* exception vector number */
    ESF0 *	pEsf,	/* pointer to exception stack frame */
    REG_SET *	pRegs,	/* pointer to register info on stack */
    BOOL	error	/* TRUE if ESF has error-code */
    )
    {
    EXC_INFO excInfo;

    /* fill excInfo/pRegs */

    excGetInfoFromESF (vecNum, pEsf, pRegs, &excInfo, error);

    if ((_func_excBaseHook != NULL) && 			/* user hook around? */
	((* _func_excBaseHook) (vecNum, pEsf, pRegs, &excInfo)))
	return;						/* user hook fixed it */

#ifdef WV_INSTRUMENTATION

    /* windview - level 3 event logging */
    EVT_CTX_1(EVENT_EXCEPTION, vecNum);

#endif  /* WV_INSTRUMENTATION */

    /* if exception occured in an isr or before multi tasking then reboot */

    if ((INT_CONTEXT ()) || (Q_FIRST (&activeQHead) == NULL))
	{
	if (_func_excPanicHook != NULL)			/* panic hook? */
	    (*_func_excPanicHook) (vecNum, pEsf, pRegs, &excInfo);

	reboot (BOOT_WARM_AUTOBOOT);
	return;						/* reboot returns?! */
	}

    /* task caused exception */

    taskIdCurrent->pExcRegSet = pRegs;			/* for taskRegs[GS]et */

    taskIdDefault ((int)taskIdCurrent);			/* update default tid */

    bcopy ((char *) &excInfo, (char *) &(taskIdCurrent->excInfo),
	   sizeof (EXC_INFO));				/* copy in exc info */

    if (_func_sigExcKill != NULL)			/* signals installed? */
	(*_func_sigExcKill) (vecNum, INUM_TO_IVEC(vecNum), pRegs);

    if (_func_excInfoShow != NULL)			/* default show rtn? */
	(*_func_excInfoShow) (&excInfo, TRUE);

    if (excExcepHook != NULL)				/* 5.0.2 hook? */
        (* excExcepHook) (taskIdCurrent, vecNum, pEsf);

    taskSuspend (0);					/* whoa partner... */

    taskIdCurrent->pExcRegSet = (REG_SET *) NULL;	/* invalid after rts */
    }

/*******************************************************************************
*
* excIntHandle - interrupt level handling of interrupts
*
* This routine handles interrupts.  It is never to be called except
* from the special assembly language interrupt stub routine.
*
* It prints out a bunch of pertinent information about the trap that
* occurred via excTask().
*
* NOMANUAL
*/

void excIntHandle
    (
    int		vecNum,	/* exception vector number */
    ESF0 *	pEsf,	/* pointer to exception stack frame */
    REG_SET *	pRegs,	/* pointer to register info on stack */
    BOOL	error	/* TRUE if ESF has error-code */
    )
    {
    EXC_INFO excInfo;

    /* fill excInfo/pRegs */

    excGetInfoFromESF (vecNum, pEsf, pRegs, &excInfo, error);

#ifdef WV_INSTRUMENTATION

    /* windview - level 3 event logging */
    EVT_CTX_1(EVENT_EXCEPTION, vecNum);

#endif  /* WV_INSTRUMENTATION */

    if (_func_excIntHook != NULL)
	(*_func_excIntHook) (vecNum, pEsf, pRegs, &excInfo);

    if (Q_FIRST (&activeQHead) == NULL)			/* pre kernel */
	reboot (BOOT_WARM_AUTOBOOT);			/* better reboot */
    }

/*****************************************************************************
*
* excGetInfoFromESF - get relevent info from exception stack frame
*
*/

LOCAL void excGetInfoFromESF
    (
    int 	vecNum,		/* vector number */
    ESF0 *	pEsf,		/* pointer to exception stack frame */
    REG_SET *	pRegs,		/* pointer to register info on stack */
    EXC_INFO *	pExcInfo,	/* where to fill in exception info */
    BOOL	errorCode	/* TRUE if ESF has errorCode */
    )
    {
    int size;

    pExcInfo->valid  = EXC_VEC_NUM;		/* vecNum is valid */
    pExcInfo->vecNum = vecNum;			/* set vecNum */

    /* find out a type of ESF with errorCode and CS in ESF */

    if (errorCode)
	{
	/* it happened in supervisor mode, type is ESF1, PL not changed */

	pExcInfo->valid	 |= EXC_ERROR_CODE;
	pExcInfo->errCode = ((ESF1 *)pEsf)->errCode;
	pExcInfo->pc	  = ((ESF1 *)pEsf)->pc;
	pExcInfo->cs	  = ((ESF1 *)pEsf)->cs;
	pExcInfo->eflags  = ((ESF1 *)pEsf)->eflags;
	size              = ESF1_NBYTES;
	}
    else
	{
	/* it happened in supervisor mode, type is ESF0, PL not changed */

	pExcInfo->pc	  = ((ESF0 *)pEsf)->pc;
	pExcInfo->cs	  = ((ESF0 *)pEsf)->cs;
	pExcInfo->eflags  = ((ESF0 *)pEsf)->eflags;
	size              = ESF0_NBYTES;
	}

    if (vecNum == IN_PAGE_FAULT)
	{
	pExcInfo->valid	|= EXC_CR2;
	pExcInfo->cr2 = vxCr2Get ();		/* get CR2 that is PF addr */
	}

    pExcInfo->cr3 = vxCr3Get ();		/* get CR3 */
    pExcInfo->esp0 = (UINT32)pEsf + size;	/* get ESP before exception */

    /* adjust the stack pointer ESP in REG_SET */

    pRegs->spReg = (ULONG)((char *) pEsf + size);

    /* get content of the supervisor stack: 8 long words */

    pExcInfo->esp00 = *(UINT32 *)pExcInfo->esp0;
    pExcInfo->esp01 = *((UINT32 *)pExcInfo->esp0 + 1);
    pExcInfo->esp02 = *((UINT32 *)pExcInfo->esp0 + 2);
    pExcInfo->esp03 = *((UINT32 *)pExcInfo->esp0 + 3);
    pExcInfo->esp04 = *((UINT32 *)pExcInfo->esp0 + 4);
    pExcInfo->esp05 = *((UINT32 *)pExcInfo->esp0 + 5);
    pExcInfo->esp06 = *((UINT32 *)pExcInfo->esp0 + 6);
    pExcInfo->esp07 = *((UINT32 *)pExcInfo->esp0 + 7);
    }

/*******************************************************************************
*
* programError - determine if exception is program error
*
* RETURNS:
*   TRUE if exception indicates program error,
*   FALSE if hardware interrupt or failure.
*/

LOCAL BOOL programError
    (
    int vecNum		/* exception vector number */
    )
    {
    if (sysProcessor != X86CPU_386)	/* is the CPU 486 or Upper ? */
        {
	if ((sysCpuId.featuresEdx & CPUID_SSE) ||
	    (sysCpuId.featuresEdx & CPUID_SSE2))
            return (vecNum <= IN_SIMD);
	else
            return (vecNum <= IN_MACHINE_CHECK);
        }
    else
        return (vecNum <= IN_CP_ERROR);
    }
