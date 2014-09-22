/* funcBind.c - function binding library */

/* Copyright 1984-2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
02m,26mar02,pai  added _func_sseTaskRegsShow (SPR 74103).
02l,14mar02,elr  replaced ftpErrorSuppress with ftplDebug (SPR 71496)
02k,09nov01,jn   add new internal symLib api
02j,02nov01,brk  added _func_selPtyAdd, _func_selPtyDelete (SPR 65498)
02i,05aug00,jgn  merge DOT-4 pthreads code
02h,29oct01,tcr  add Windview networking variables
02g,26sep01,jws  add vxMP and vxFusion show & info rtn ptrs (SPR 36055)
01f,20sep01,aeg  added _func_selWakeupListTerm.
01e,03apr01,kab  Added _WRS_ALTIVEC_SUPPORT
01d,16mar01,pcs  ADDED _func_altivecTaskRegsShow
01c,03mar00,zl   merged SH support into T2
01b,10aug98,pr   added WindView function pointers for i960
02a,15apr98,cth  added _func_evtLogReserveTaskName for WV2.0
01w,08apr98,pr   added _func_evtLogT0_noInt. Set evtAction as UINT32.
01y,13dec97,pr   replaced somw WindView 1.0 variables with WV2.0 ones
01x,17nov97,cth  removed all scrPad and evtBuf references, for WV2.0
01w,22jul97,pr   added _func_trgCheck
01v,24jun97,pr   added evtInstMode
01v,09oct97,tam  added _func_ioTaskStdSet
01u,21feb97,tam  added _dbgDsmInstRtn
01t,08jul96,pr   added _func_evtLogT1_noTS
01s,12may95,p_m  added _func_printErr, _func_symFindByValue, _func_spy*,
		       _func_taskCreateHookAdd and _func_taskDeleteHookAdd.
01r,24jan94,smb  added windview event logging functions for portable kernel.
01q,10dec93,smb  added windview event logging functions.
01p,05sep93,jcf  added _remCurId[SG]et.
01o,23aug93,jmm  added _func_bdall
01n,22jul93,jmm  added _netLsByName (spr 2305)
01m,13feb93,kdl  added _procNumWasSet (SPR #2011).
01l,13nov92,jcf  added _func_logMsg.
01k,29sep92,jwt  merged cacheLibInit(), cacheReset(), and cacheModeSet().
01j,20sep92,kdl  added _func_ftpLs, ftpErrorSuppress.
01i,31aug92,rrr  added _func_sigprocmask
01h,23aug92,jcf  added cache*, _func_{sel*,excJobAdd,memalign,valloc}
01g,02aug92,jcf  added/tuned _func_exc*.
01f,30jul92,rdc  added additional field to vmLibInfo.
01e,29jul92,jcf  added _func_fclose.
01d,29jul92,rrr  added hooks for signals and exceptions
01c,27jul92,rdc  added vmLibInfo.
01b,19jul92,pme  added _func_smObjObjShow.
01a,04jul92,jcf  created.
*/

/*
INTERNAL
This library contains global function pointers to bind libraries together
at runtime.  Someday a tool chain might initialize these variables
automatically.  For now the library initialization routines fill in the
function pointer for other libraries to utilize.
NOMANUAL
*/

#include "vxWorks.h"
#include "cacheLib.h"
#include "private/funcBindP.h"
#include "private/vmLibP.h"
#include "wvNetLib.h"

/* global variables */

FUNCPTR     _func_ioTaskStdSet;
FUNCPTR     _func_bdall;
FUNCPTR     _func_dspTaskRegsShow;
VOIDFUNCPTR _func_dspRegsListHook;	/* arch dependent DSP regs list */
FUNCPTR	    _func_dspMregsHook;	/* arch dependent mRegs() hook */
FUNCPTR     _func_excBaseHook;
FUNCPTR     _func_excInfoShow;
FUNCPTR     _func_excIntHook;
FUNCPTR     _func_excJobAdd;
FUNCPTR     _func_excPanicHook;
FUNCPTR     _func_fclose;
FUNCPTR     _func_fppTaskRegsShow;
#ifdef _WRS_ALTIVEC_SUPPORT
FUNCPTR     _func_altivecTaskRegsShow;
#endif /* _WRS_ALTIVEC_SUPPORT */
FUNCPTR     _func_ftpLs;
FUNCPTR     _func_netLsByName;
FUNCPTR     _func_logMsg;
FUNCPTR     _func_memalign;
FUNCPTR     _func_printErr;
FUNCPTR     _func_selPtyAdd;
FUNCPTR     _func_selPtyDelete;
FUNCPTR     _func_pthread_setcanceltype;
FUNCPTR     _func_selTyAdd;
FUNCPTR     _func_selTyDelete;
FUNCPTR     _func_selWakeupAll;
FUNCPTR     _func_selWakeupListInit;
FUNCPTR	    _func_selWakeupListTerm;
VOIDFUNCPTR _func_sigExcKill;
FUNCPTR     _func_sigprocmask;
FUNCPTR     _func_sigTimeoutRecalc;
FUNCPTR     _func_smObjObjShow;
FUNCPTR     _func_spy;
FUNCPTR     _func_spyStop;
FUNCPTR     _func_spyClkStart;
FUNCPTR     _func_spyClkStop;
FUNCPTR     _func_spyReport;
FUNCPTR     _func_spyTask;
FUNCPTR     _func_sseTaskRegsShow;
FUNCPTR     _func_symFindByValueAndType;   /* obsolete - do not use. */
FUNCPTR     _func_symFindByValue;          /* obsolete - do not use. */
FUNCPTR     _func_symFindSymbol;
FUNCPTR     _func_symNameGet;
FUNCPTR     _func_symValueGet;
FUNCPTR     _func_symTypeGet;
FUNCPTR     _func_taskCreateHookAdd;
FUNCPTR     _func_taskDeleteHookAdd;
FUNCPTR     _func_valloc;
FUNCPTR     _func_remCurIdGet;
FUNCPTR     _func_remCurIdSet;

FUNCPTR	    _dbgDsmInstRtn = (FUNCPTR) NULL;	/* disassembler routine */

UINT32	    ftplDebug = 0; /* FTPL_DEBUG_OFF */
BOOL	    _procNumWasSet = FALSE;	

/* triggering function pointers */

VOIDFUNCPTR _func_trgCheck;

/* windview function pointers */

/* level 1 event logging function pointers */
VOIDFUNCPTR _func_evtLogO;
VOIDFUNCPTR _func_evtLogOIntLock;

/* level 2 event logging function pointers */
VOIDFUNCPTR _func_evtLogM0;
VOIDFUNCPTR _func_evtLogM1;
VOIDFUNCPTR _func_evtLogM2;
VOIDFUNCPTR _func_evtLogM3;

/* level 3 event logging function pointers */
VOIDFUNCPTR _func_evtLogT0;
VOIDFUNCPTR _func_evtLogT0_noInt;
VOIDFUNCPTR _func_evtLogT1;
VOIDFUNCPTR _func_evtLogT1_noTS;
VOIDFUNCPTR _func_evtLogTSched;

VOIDFUNCPTR _func_evtLogString;
FUNCPTR     _func_evtLogPoint;
FUNCPTR	    _func_evtLogReserveTaskName;

#if CPU_FAMILY==I960

VOIDFUNCPTR _func_windInst1;
VOIDFUNCPTR _func_windInstDispatch;
VOIDFUNCPTR _func_windInstIdle;
VOIDFUNCPTR _func_windInstIntEnt;
VOIDFUNCPTR _func_windInstIntExit;

#endif

/* time stamp function pointer */
FUNCPTR     _func_tmrStamp;
FUNCPTR     _func_tmrStampLock;
FUNCPTR     _func_tmrFreq;
FUNCPTR     _func_tmrPeriod;
FUNCPTR     _func_tmrConnect;
FUNCPTR     _func_tmrEnable;
FUNCPTR     _func_tmrDisable;

int     	trgCnt;       /* global variable for triggering */

BOOL   wvInstIsOn;                /* windview instrumentation ON/OFF */
BOOL   wvObjIsEnabled;            /* Level 1 event collection enable */

/* ....FIXME  I am not sure these should not be defined here */

UINT32          evtAction = 0; /* this one might go in evtLogLib.c */
UINT32          trgEvtClass; /* this one might go in trgLib.c */
UINT32          wvEvtClass; /* this one might go in wvLib.c */

#if 0
BOOL   evtLogTIsOn;               /* event collection ON/OFF */
BOOL   evtLogOIsOn;               /* Level 1 event collection ON/OFF */
#endif

/* end of windview function pointers */

/* WindView for networking variables */

EVENT_MASK *    pWvNetEventMap;
FUNCPTR         _func_wvNetAddressFilterTest;
FUNCPTR         _func_wvNetPortFilterTest;

/* End of WindView for networking variables */

VM_LIB_INFO vmLibInfo =
    {
    FALSE,
    FALSE,
    FALSE,
    (FUNCPTR) NULL,
    (FUNCPTR) NULL,
    (FUNCPTR) NULL,
    (FUNCPTR) NULL,
    (FUNCPTR) NULL
    };

/* Cache function pointers need to be in "data", not "bss" */

CACHE_LIB cacheLib = 		/* the cache primitives */
    {
    NULL, 			/* cacheEnable() */
    NULL, 			/* cacheDisable() */
    NULL, 			/* cacheLock() */
    NULL, 			/* cacheUnlock() */
    NULL, 			/* cacheFlush() */
    NULL, 			/* cacheInvalidate() */
    NULL, 			/* cacheClear() */
    NULL,			/* cacheTextUpdate() */
    NULL,			/* cachePipeFlush() */
    NULL, 			/* cacheDmaMalloc() */
    NULL,			/* cacheDmaFree() */
    NULL, 			/* cacheDmaVirtToPhys() */
    NULL			/* cacheDmaPhysToVirt() */
    };

CACHE_FUNCS cacheNullFuncs = 	/* null cache functions for non-cached mem */
    {
    NULL,
    NULL,
    NULL,
    NULL
    };

CACHE_FUNCS cacheDmaFuncs =	/* cache functions for dma memory */
    {
    NULL,			/* cacheDmaFlush() */
    NULL,			/* cacheDmaInvalidate() */
    NULL,			/* cacheDmaVirtToPhys() */
    NULL			/* cacheDmaPhysToVirt() */
    };

CACHE_FUNCS cacheUserFuncs =	/* cache functions for normal (user) memory */
    {
    NULL,			/* cacheUserFlush() */
    NULL,			/* cacheUserInvalidate() */
    NULL,			/* Always NULL */
    NULL			/* Always NULL */
    };

/* vxMP and vxFusion show and info routine pointers */

FUNCPTR  msgQSmShowRtn;		/* shared message Q show routine pointer */
FUNCPTR  msgQSmInfoGetRtn;	/* shared message Q info get  routine pointer */
FUNCPTR  semSmShowRtn; 		/* shared semaphore show routine pointer */
FUNCPTR  semSmInfoRtn; 		/* shared semaphore info routine pointer */
FUNCPTR  smMemPartShowRtn;	/* shared memory partition show routine */

FUNCPTR  msgQDistShowRtn;       /* distributed msgQ show routine pointer */
FUNCPTR  msgQDistInfoGetRtn;	/* distributed msgQ info get routine pointer */
