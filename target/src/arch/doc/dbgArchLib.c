/* dbgArchLib.c - architecture-dependent debugger library */
  
/* Copyright 1984-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01p,05jun02,wsl  remove references to SPARC and i960
01o,26nov01,hdn  updated the x86 section
01n,22nov01,hbh  Updated for simulatorrs.
01m,16nov01,tlc  Update for MIPS architecture.
01l,30oct01,zl   corrected arch names in library description, updated SH
                 section.
01k,03mar00,zl   merged SH support into T2
01j,27feb97,jpd  Added ARM-specific documentation.
01i,25nov95,jdi  removed 29k stuff.
01h,10oct95,jdi  doc: changed .tG Shell back to .pG "Target Shell".
01g,06oct95,jdi  changed Debugging .pG's to .tG Shell.
01f,12mar95,dvs  changed sr() to srShow() for am29k (SPR #4084) and gr() -> 
		 grShow() for consistancy.
01e,10feb95,jdi  changed 80960 to i960; synchronized with ../<arch>/dbgArchLibs.
01d,27jan95,rhp  added MIPS R3000/R4000 and Intel i386/i486;
                 minor doc cleanup on Am29K.
01c,02dec93,pme  added Am29K family support.
01b,14mar93,jdi  changed underscores in fsrShow() and psrShow() examples to
		 dashes, for aesthetics and compatibility with pretroff.
01a,13feb93,jdi  written, based on dbgArchLib.c for mc68k, sparc, and i960.
*/

/*
DESCRIPTION
This module provides architecture-specific support functions for dbgLib.
It also includes user-callable functions for accessing the contents of
registers in a task's TCB (task control block).  These routines include:

.TS
tab(|);
l0 l0 l.
\&`MC680x0':
         | a0() - a7()  | - address registers (`a0' - `a7')
         | d0() - d7()  | - data registers (`d0' - `d7')
         | sr()         | - status register (`sr')

\&`MIPS':
  | dbgBpTypeBind() | - bind a breakpoint handler to a breakpoint type

\&`x86/SimNT':
         | edi() - eax() | - named register values
         | eflags()      | - status register value

\&`SH':    
         | r0() - r15() | - general registers (`r0' - `r15')
         | sr()         | - status register (`sr')
         | gbr()        | - global base register (`gbr')
         | vbr()        | - vector base register (`vbr')
         | mach()       | - multiply and accumulate register high (`mach')
         | macl()       | - multiply and accumulate register low (`macl')
         | pr()         | - procedure register (`pr')

\&`ARM':
         | r0() - r14() | - general-purpose registers (`r0' - `r14')
         | cpsr()       | - current processor status reg (`cpsr')
         | psrShow()    | - `psr' value, symbolically

\&`SimSolaris':
         | g0() - g7()  | - global registers (`g0' - `g7')
         | o0() - o7()  | - out registers (`o0' - `o7', note lower-case "o")
         | l0() - l7()  | - local registers (`l0' - `l7', note lower-case "l")
         | i0() - i7()  | - in registers (`i0' - `i7')
         | npc()        | - next program counter (`npc')
         | psr()        | - processor status register (`psr')
         | wim()        | - window invalid mask (`wim')
         | y()          | - `y' register 
.TE

NOTE:  The routine pc(), for accessing the program counter, is found
in usrLib.

SEE ALSO: dbgLib,
.pG "Target Shell"

INTERNAL
Note that the program counter (pc) is handled by a non-architecture-specific
routine in usrLib.

This module is created by cat'ing together the following files AFTER the
first entry below - sr(), g0().

	mc68k/dbgArchLib.c
XX	sparc/dbgArchLib.c
XX	i960/dbgArchLib.c
	mips/dbgArchLib.c
	i86/dbgArchLib.c
	sh/dbgArchLib.c
	arm/dbgArchLib.c
	simsolaris/dbgArchLib.c
	simnt/dbgArchLib.c
*/

/*******************************************************************************
*
* g0 - return the contents of register `g0', also `g1' - `g7' (SPARC) and `g1' - `g14' (i960)
*
* This command extracts the contents of global register `g0' from the TCB of a
* specified task.  If <taskId> is omitted or 0, the current default task is
* assumed.
*
* Routines are provided for all global registers:
* .TS
* tab(|);
* l l l.
* \&`SPARC':      | g0() - g7()  | (`g0' - `g7')
* \&`i960':       | g0() - g14() | (`g0' - `g14')
* \&`SimSolaris': | g0() - g7()  | (`g0' - `g7')
* .TE
*
* RETURNS: The contents of register `g0' (or the requested register).
*
* SEE ALSO:
* .pG "Target Shell"
*
* NOMANUAL
*/

int g0
    (
    int taskId		/* task ID, 0 means default task */
    )
    
    { ... }



/* cat arch libraries after here */



/* dbgArchLib.c - MC680x0-dependent debugger library */
  
/* Copyright 1984-1995 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01h,10feb95,jdi  doc cleanup for 5.2.
01g,13feb93,jdi  documentation cleanup for 5.1; added discrete entry for d0().
01f,22sep92,kdl  added single-register display routines, from 5.0 usrLib.
01e,19sep92,kdl  made dbgRegsAdjust() clear padding bytes before SR in reg set.
01d,23aug92,jcf  changed filename.
01c,06jul92,yao  removed dbgCacheClear().  made user uncallable globals
		 started with '_'.
01b,03jul92,jwt  first pass at converting cache calls to 5.1 cache library.
01a,18jun92,yao  written based on mc68k/dbgLib.c ver08g.
*/

/*
DESCRIPTION
This module provides the Motorola 680x0 specific support functions for dbgLib.

NOMANUAL
*/

#include "vxWorks.h"
#include "private/dbgLibP.h"
#include "taskLib.h"
#include "taskArchLib.h"
#include "intLib.h"
#include "regs.h"
#include "iv.h"
#include "cacheLib.h"
#include "ioLib.h"
#include "dsmLib.h"
#include "vxLib.h"
#include "usrLib.h"

/* externals */

/* interrupt driver routines from dbgALib.s */

IMPORT dbgBpStub ();		/* breakpoint interrupt driver */
IMPORT dbgTraceStub ();		/* trace interrupt driver */

IMPORT int dsmNbytes ();
IMPORT int dsmInst ();

/* globals */

extern char * _archHelp_msg = "";

LOCAL USHORT interruptSR;	/* old SR value before interrupt break */

/*******************************************************************************
*
* _dbgArchInit - architecture dependent initialization routine
*
* This routine initialize global function pointers that are architecture 
* specific.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void _dbgArchInit (void)

    {
    _dbgDsmInstRtn = (FUNCPTR) dsmInst;
    }
/*******************************************************************************
*
* _dbgVecInit - insert new breakpoint and trace vectors
*
* RETURNS: N/A
*
* NOMANUAL
*/

void _dbgVecInit (void)
    
    {
    /* insert the new breakpoint and trace vectors */

    intVecSet ((FUNCPTR *) TRAPNUM_TO_IVEC (DBG_TRAP_NUM), dbgBpStub);
    intVecSet ((FUNCPTR *) IV_TRACE, dbgTraceStub);
    }
/*******************************************************************************
*
* _dbgInstSizeGet - set up breakpoint instruction
*
* RETURNS: size of the instruction at specified location.
*
* NOMANUAL
*/

int _dbgInstSizeGet
    (
    INSTR * pBrkInst		/* pointer to hold breakpoint instruction */
    )

    {
    return (dsmNbytes (pBrkInst) / sizeof (INSTR));
    }
/*******************************************************************************
*
* _dbgRetAdrsGet - get return address for current routine
*
* RETURNS: return address for current routine.
*
* NOMANUAL
*/

INSTR * _dbgRetAdrsGet
    (
    REG_SET * pRegSet		/* pointer to register set */
    )
    {
    /* if next instruction is a LINK or RTS, return address is on top of stack;
     * otherwise it follows saved frame pointer */

    if (INST_CMP(pRegSet->pc,LINK,LINK_MASK) || 
	INST_CMP(pRegSet->pc,RTS,RTS_MASK))
        return (*(INSTR **)pRegSet->spReg);
    else
	return (*((INSTR **)pRegSet->fpReg + 1));
    }
/*******************************************************************************
*
* _dbgSStepClear - clear single step mode
*
* RETURNS: N/A
*
* NOMANUAL
*/

void _dbgSStepClear (void)
    {
    }
/*******************************************************************************
*
* _dbgSStepSet - set single step mode
*
* RETURNS: N/A
*
* NOMANUAL
*/

void _dbgSStepSet 
    (
    BREAK_ESF * pInfo		/* pointer to info saved on stack */
    )
    {
    pInfo->sr |= TRACE_BIT | 0x700; /* set trace bit and lock interrupts */
    }
/******************************************************************************
*
* _dbgTaskSStepSet - set single step mode of task
*
* Set the `pcw' and `tcw' of the given task for trace (instruction) break
* mode, so that when the task is switched in, it will execute the
* next instruction and break.
*
* NOTE
* Interrupts are locked out for this task, until single stepping
* for the task is cleared.
*
* RETURNS: OK or ERROR if invalid <tid>.
*
* NOMANUAL
*/

void _dbgTaskSStepSet
    (
    int tid		/* task's id */
    )
    {
    REG_SET regSet;

    taskRegsGet (tid, &regSet);
    taskSRSet (tid, regSet.sr | TRACE_BIT);
    }
/******************************************************************************
*
* _dbgTaskBPModeSet - set breakpoint mode of task
*
* NOMANUAL
*/

void _dbgTaskBPModeSet 
    (
    int tid		/* task's id */
    )
    {
    }
/******************************************************************************
*
* _dbgTaskBPModeClear - clear breakpoint mode of task
*
* NOMANUAL
*/

void _dbgTaskBPModeClear 
    (
    int tid
    )
    {
    }
/*******************************************************************************
*
* _dbgFuncCallCheck - check next instruction
*
* This routine checks to see if the next instruction is a JSR or BSR.
* If it is, it returns TRUE, otherwise, returns FALSE.
*
* RETURNS: TRUE if next instruction is JSR or BSR, or FALSE otherwise.
*
* NOMANUAL
*/

BOOL _dbgFuncCallCheck
    (
    INSTR * addr		/* pointer to instruction */
    )
    {
    return (INST_CMP (addr, JSR, JSR_MASK) || INST_CMP (addr, BSR, BSR_MASK));
    }
/*******************************************************************************
*
* _dbgRegsAdjust - set register set
*
* RETURNS: N/A
*
* NOMANUAL
*/

void _dbgRegsAdjust
    (
    FAST int   tid,		/* id of task that hit breakpoint */
    TRACE_ESF * pInfo,		/* pointer to esf info saved on stack */
    int *       regs,		/* pointer to buf containing saved regs */
    BOOL stepBreakFlag		/* TRUE if this was a trace exception */
				/* FALSE if this was a SO or CRET breakpoint */
    )
    {
    REG_SET regSet;
    int ix;

    for (ix = 0; ix < 8; ++ix)
	regSet.dataReg [ix] = regs[ix];

    for (ix =0; ix < 7; ++ix)
	regSet.addrReg [ix] = regs[ix+8];

    if (stepBreakFlag)
	regSet.spReg = (ULONG) ((char *)pInfo + sizeof (TRACE_ESF));
    else
	regSet.spReg = (ULONG) ((char *)pInfo + sizeof (BREAK_ESF));
    regSet.pc    = pInfo->pc;
    regSet.pad   = 0;
    regSet.sr    = pInfo->sr;

    taskRegsSet (tid, &regSet);
    }
/*******************************************************************************
*
* _dbgIntrInfoSave - restore register set
*
* RETURNS: N/A
*
* NOMANUAL
*/

void _dbgIntrInfoSave
    (
    BREAK_ESF * pInfo		/* pointer to info saved on stack */
    )
    {
    interruptSR = pInfo->sr;	/* save SR */
    }
/******************************************************************************
*
* _dbgIntrInfoRestore - restore the info saved by dbgIntrInfoSave
*
* NOMANUAL
*/

void _dbgIntrInfoRestore
    (
    TRACE_ESF *	pBrkInfo	/* pointer to execption frame */
    )
    {
    pBrkInfo->sr = interruptSR;
    }
/******************************************************************************
*
* _dbgInstPtrAlign - align pointer to appropriate boundary
*
* REUTRNS: align given instruction pointer to appropriate boundary
*
* NOMANUAL
*/

INSTR * _dbgInstPtrAlign 
    (
    INSTR * addr		/* instuction pointer */
    )
    {
#if (CPU==MC68000 || CPU==MC68010 || CPU==CPU32)
    addr = (INSTR *) ((int)addr & ~(0x01));	/* force address even */
#endif	/* (CPU==MC68000 || CPU==MC68010 || CPU==CPU32) */
    return (addr);
    }
/*******************************************************************************
*
* _dbgInfoPCGet - get pc
*
* RETURNS: value of pc saved on stack
*
* NOMANUAL
*/

INSTR * _dbgInfoPCGet
    (
    BREAK_ESF * pInfo		/* pointer to info saved on stack */
    )
    {
    return (pInfo->pc);
    }
/*******************************************************************************
*
* _dbgTaskPCSet - set task's pc
*
* RETURNS: N/A
*
* NOMANUAL
*/

void _dbgTaskPCSet
    (
    int    tid,		/* task id */
    INSTR* pc,		/* task's pc */
    INSTR* npc		/* not supoorted on MC680X0 */
    )
    {
    REG_SET regSet;		/* task's register set */

    taskRegsGet (tid, &regSet);

    regSet.pc = pc;

    taskRegsSet (tid, &regSet);
    }
/*******************************************************************************
*
* _dbgTaskPCGet - restore register set
*
* RETURNS: N/A
*
* NOMANUAL
*/

INSTR * _dbgTaskPCGet
    (
    int tid		/* task id */
    )
    {
    REG_SET regSet;		/* task's register set */

    taskRegsGet (tid, &regSet);

    return (regSet.pc);
    }
/*******************************************************************************
*
* _dbgTraceDisable - disable trace mode
*
* NOMANUAL
*/

void _dbgTraceDisable (void)

    {
    /* already disabled in dbgTraceStub */
    }

/*******************************************************************************
*
* getOneReg - return the contents of one register
*
* Given a task's ID, this routine returns the contents of the register
* specified by the register code.  This routine is used by `a0', `d0', `sr',
* etc.  The register codes are defined in dbgMc68kLib.h.
*
* RETURNS: register contents, or ERROR.
*/

LOCAL int getOneReg (taskId, regCode)
    int		taskId;		/* task's id, 0 means default task */
    int		regCode;	/* code for specifying register */

    {
    REG_SET	regSet;		/* get task's regs into here */

    taskId = taskIdFigure (taskId);	/* translate super name to id */

    if (taskId == ERROR)		/* couldn't figure out super name */
	return (ERROR);
    taskId = taskIdDefault (taskId);	/* set the default id */

    if (taskRegsGet (taskId, &regSet) != OK)
	return (ERROR);

    switch (regCode)
	{
	case D0: return (regSet.dataReg [0]);	/* data registers */
	case D1: return (regSet.dataReg [1]);
	case D2: return (regSet.dataReg [2]);
	case D3: return (regSet.dataReg [3]);
	case D4: return (regSet.dataReg [4]);
	case D5: return (regSet.dataReg [5]);
	case D6: return (regSet.dataReg [6]);
	case D7: return (regSet.dataReg [7]);

	case A0: return (regSet.addrReg [0]);	/* address registers */
	case A1: return (regSet.addrReg [1]);
	case A2: return (regSet.addrReg [2]);
	case A3: return (regSet.addrReg [3]);
	case A4: return (regSet.addrReg [4]);
	case A5: return (regSet.addrReg [5]);
	case A6: return (regSet.addrReg [6]);
	case A7: return (regSet.addrReg [7]);	/* a7 is the stack pointer */

	case SR: return (regSet.sr);
	}

    return (ERROR);		/* unknown regCode */
    }

/*******************************************************************************
*
* a0 - return the contents of register `a0' (also `a1' - `a7') (MC680x0)
*
* This command extracts the contents of register `a0' from the TCB of a specified
* task.  If <taskId> is omitted or zero, the last task referenced is assumed.
*
* Similar routines are provided for all address registers (`a0' - `a7'):
* a0() - a7().
*
* The stack pointer is accessed via a7().
*
* RETURNS: The contents of register `a0' (or the requested register).
*
* SEE ALSO:
* .pG "Target Shell"
*/

int a0
    (
    int taskId		/* task ID, 0 means default task */
    )

    {
    return (getOneReg (taskId, A0));
    }

int a1 (taskId) int taskId; { return (getOneReg (taskId, A1)); }
int a2 (taskId) int taskId; { return (getOneReg (taskId, A2)); }
int a3 (taskId) int taskId; { return (getOneReg (taskId, A3)); }
int a4 (taskId) int taskId; { return (getOneReg (taskId, A4)); }
int a5 (taskId) int taskId; { return (getOneReg (taskId, A5)); }
int a6 (taskId) int taskId; { return (getOneReg (taskId, A6)); }
int a7 (taskId) int taskId; { return (getOneReg (taskId, A7)); }


/*******************************************************************************
*
* d0 - return the contents of register `d0' (also `d1' - `d7') (MC680x0)
*
* This command extracts the contents of register `d0' from the TCB of a specified
* task.  If <taskId> is omitted or zero, the last task referenced is assumed.
*
* Similar routines are provided for all data registers (`d0' - `d7'):
* d0() - d7().
*
* RETURNS: The contents of register `d0' (or the requested register).
*
* SEE ALSO:
* .pG "Target Shell"
*/

int d0
    (
    int taskId		/* task ID, 0 means default task */
    )

    {
    return (getOneReg (taskId, D0));
    }

int d1 (taskId) int taskId; { return (getOneReg (taskId, D1)); }
int d2 (taskId) int taskId; { return (getOneReg (taskId, D2)); }
int d3 (taskId) int taskId; { return (getOneReg (taskId, D3)); }
int d4 (taskId) int taskId; { return (getOneReg (taskId, D4)); }
int d5 (taskId) int taskId; { return (getOneReg (taskId, D5)); }
int d6 (taskId) int taskId; { return (getOneReg (taskId, D6)); }
int d7 (taskId) int taskId; { return (getOneReg (taskId, D7)); }


/*******************************************************************************
*
* sr - return the contents of the status register (MC680x0, SH)
*
* This command extracts the contents of the status register from the TCB of a
* specified task.  If <taskId> is omitted or zero, the last task referenced is
* assumed.
*
* RETURNS: The contents of the status register.
*
* SEE ALSO:
* .pG "Target Shell"
*/

int sr
    (
    int taskId		/* task ID, 0 means default task */
    )

    {
    return (getOneReg (taskId, SR));
    }



/* dbgArchLib.c - SPARC-dependent debugger library */
  
/* Copyright 1984-1995 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01i,10feb95,jdi  doc cleanup for 5.2.
01i,13feb93,jdi  documentation cleanup for 5.1; created discrete entries for
<                register routines.
01h,09feb92,yao  pulled in register show routines from 5.0.
01g,15oct92,jwt  cleaned up ANSI compiler warnings.
01f,01oct92,yao  removed redundant delcaration for _dbgStepAdd().
		 changed to pass pointer to information and registers
		 saved on stack to _dbgStepAdd().
01e,23aug92,jcf  changed filename.
01d,15jul92,jwt  moved fsrShow() to dbgSparcLib.c - scalable kernel.
01c,06jul92,yao  removed dbgArchCacheClear(). made user uncallable globals
		 started with '_'.
01b,03jul92,jwt  converted to 5.1 cache library support; cleanup;
                 added more Fujitsu chips to psrShow().
01a,18jun92,yao  written based on sparc/dbgLib.c ver2f.
*/

/*
DESCRIPTION
This module provides the SPARC specific support functions for dbgLib.

NOMANUAL
*/

#include "vxWorks.h"
#include "private/dbgLibP.h"
#include "taskLib.h"
#include "fppLib.h"
#include "taskArchLib.h"
#include "intLib.h"
#include "regs.h"
#include "arch/sparc/psl.h"
#include "iv.h"
#include "cacheLib.h"
#include "ioLib.h"
#include "dsmLib.h"
#include "vxLib.h"
#include "stdio.h"
#include "usrLib.h"

LOCAL void fsrExcShow ();
void fsrShow ();

/* interrupt driver routines from dbgALib.s */

IMPORT dbgBpStub ();		/* breakpoint interrupt driver */

IMPORT int dsmNbytes ();
IMPORT int dsmInst ();

/* globals */

char * _archHelp_msg =
    "i0-i7,l0-l7,o0-o7,g1-g7,\n"
    "pc,npc,psr,wim,y  [task]        Display a register of a task\n"
    "psrShow   value                 Display meaning of psr value\n";

LOCAL oldIntLevel;		/* old interrupt level */

/*******************************************************************************
*
* _dbgArchInit - architecture dependent initialization routine
*
* This routine initialize global function pointers that are architecture 
* specific.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void _dbgArchInit (void)

    {
    _dbgDsmInstRtn = (FUNCPTR) dsmInst;
    }
/*******************************************************************************
*
* _dbgVecInit - insert new breakpoint and trace vectors
*
* RETURNS: N/A
*
* NOMANUAL
*/

void _dbgVecInit (void)
    
    {
    /* insert the new breakpoint and trace vectors */

    intVecSet ((FUNCPTR *) TRAPNUM_TO_IVEC (DBG_TRAP_NUM), (FUNCPTR) dbgBpStub);
    }
/*******************************************************************************
*
* _dbgInstSizeGet - set up breakpoint instruction
*
* RETURNS: size of the instruction at specified location.
*
* NOMANUAL
*/

int _dbgInstSizeGet
    (
    INSTR * pBrkInst		/* pointer to hold breakpoint instruction */
    )
    {
    return ((2 * sizeof (INSTR)) / sizeof (INSTR));
    }
/*******************************************************************************
*
* _dbgRetAdrsGet - get return address for current routine
*
* RETURNS: return address for current routine.
*
* NOMANUAL
*/

INSTR * _dbgRetAdrsGet
    (
    REG_SET * pRegSet		/* pointer to register set */
    )
    {
    int* sp = (int *) pRegSet->spReg;

    if (INST_CMP (I7_CONTENTS (sp), INST_CALL, INST_CALL_MASK) || 
	INST_CMP (I7_CONTENTS (sp), JMPL_o7, JMPL_o7_MASK))
	{
        return ((INSTR *) (I7_CONTENTS(sp) + 2));
	}
    else
	return (NULL);
    }
/*******************************************************************************
*
* _dbgTraceDisable - disable trace mode
*
* Since trace mode is not supported on SPARC, this routine does nothing.
*
* NOMANUAL
*/

void _dbgTraceDisable (void)
    {
    }
/*******************************************************************************
*
* _dbgTaskBPModeClear - clear breakpoint enable mode
*
* NOMANUAL
*/

void _dbgTaskBPModeClear 
    (
    int tid	/* task id */
    )
    {
    /* THIS ROUTINE MUST BE USED IN PAIR WITH dbgTaskBPModeSet. */

    oldIntLevel = intLock ();
    }
/*******************************************************************************
*
* _dbgSStepClear - clear single step mode
*
* RETURNS: N/A
*
* NOMANUAL
*/

void _dbgSStepClear (void)
    {
    }
/*******************************************************************************
*
* _dbgSStepSet - set single step mode
*
* RETURNS: N/A
*
* NOMANUAL
*/

void _dbgSStepSet 
    (
    BREAK_ESF * pInfo		/* pointer to info saved on stack */
    )
    {
    int tid = taskIdSelf ();

    _dbgStepAdd  (tid, BRK_TEMP, NULL, NULL);
    }
/******************************************************************************
*
* _dbgTaskSStepSet - set single step mode of task
*
* RETURNS: N/A
*
* NOMANUAL
*/

void _dbgTaskSStepSet
    (
    int tid		/* task's id */
    )
    {
    }
/******************************************************************************
*
* _dbgTaskBPModeSet - set breakpoint mode of task
*
* RETURNS: N/A
*
* NOMANUAL
*/

void _dbgTaskBPModeSet 
    (
    int tid		/* task's id */
    )
    {
    /* THIS ROUTINE MUST BE USED IN PAIR WITH dbgTaskBPModeClear. */

    intUnlock (oldIntLevel);
    }
/*******************************************************************************
*
* _dbgFuncCallCheck - check next instruction
*
* This routine checks to see if the next instruction is a JSR or BSR.
* If it is, it returns TRUE, otherwise, returns FALSE.
*
* RETURNS: TRUE if next instruction is JSR or BSR, or FALSE otherwise.
*
* NOMANUAL
*/

BOOL _dbgFuncCallCheck
    (
    INSTR * addr		/* pointer to instruction */
    )
    {
    return (INST_CMP (addr, INST_CALL, INST_CALL_MASK) || 
	    INST_CMP (addr, JMPL_o7, JMPL_o7_MASK));
    }
/*******************************************************************************
*
* _dbgRegsAdjust - set register set
*
* RETURNS: N/A
*
* NOMANUAL
*/

void _dbgRegsAdjust
    (
    FAST int 	tid,		/* id of task that hit breakpoint */
    TRACE_ESF *	pInfo,		/* pointer to esf info saved on stack */
    int * 	pRegs,		/* pointer to buf containing saved regs */
    BOOL 	stepBreakFlag	/* TRUE if this was a trace exception */
				/* FALSE if this was a SO or CRET breakpoint */
    )
    {
    REG_SET regSet;
    int ix;
    ESF * pESF = (ESF *) pRegs;

    for (ix = 0; ix < 8; ++ix)
	regSet.global [ix] = pESF->esf.global[ix];

    for (ix =0; ix < 8; ++ix)
	regSet.out [ix] = pESF->esf.out[ix];

    for (ix =0; ix < 8; ++ix)
	regSet.local [ix] = pESF->esf.local[ix];

    for (ix =0; ix < 8; ++ix)
	regSet.in [ix] = pESF->esf.in[ix];

    regSet.pc  = pESF->esf.pc;
    regSet.npc = pESF->esf.npc;
    regSet.psr = pESF->esf.psr;
    regSet.wim = pESF->esf.wim;
    regSet.tbr = pESF->esf.tbr;
    regSet.y   = pESF->esf.y;

    taskRegsSet (tid, &regSet);
    }
/*******************************************************************************
*
* _dbgIntrInfoSave - restore register set
*
* RETURNS: N/A
*
* NOMANUAL
*/

void _dbgIntrInfoSave
    (
    BREAK_ESF *	pInfo		/* pointer to info saved on stack */
    )
    {
    /* does nothing */
    }
/******************************************************************************
*
* _dbgIntrInfoRestore - restore the info saved by dbgIntrInfoSave
*
* NOMANUAL
*/

void _dbgIntrInfoRestore
    (
    TRACE_ESF *  pInfo		/* pointer to execption frame */
    )
    {
    /* does nothing */
    }
/*******************************************************************************
*
* _dbgInfoPCGet - get pc from stack
*
* RETURNS: value of pc saved on stack
*
* NOMANUAL
*/

INSTR * _dbgInfoPCGet
    (
    BREAK_ESF * pInfo		/* pointer to info saved on stack */
    )
    {
    return (pInfo->pc);
    }
/*******************************************************************************
*
* _dbgTaskPCSet - set task's pc
*
* RETURNS: N/A
*
* NOMANUAL
*/

void _dbgTaskPCSet
    (
    int     tid,		/* task id */
    INSTR * pc,			/* task's pc */
    INSTR * npc			/* task's npc */
    )
    {
    REG_SET regSet;		/* task's register set */

    taskRegsGet (tid, &regSet);

    regSet.pc = pc;
    if (npc == NULL)
	regSet.npc = pc + 1;
    else
	regSet.npc = npc;

    taskRegsSet (tid, &regSet);
    }
/*******************************************************************************
*
* _dbgTaskPCGet - get task's pc
*
* RETURNS: N/A
*
* NOMANUAL
*/

INSTR * _dbgTaskPCGet
    (
    int tid	/* task's id */
    )
    {
    REG_SET	regSet;

    taskRegsGet (tid, &regSet);
    return ((INSTR *) regSet.pc);
    }
/*******************************************************************************
*
* _dbgStepAdd -  installs a break point of the given type at the next
*		    executable address of the given task.
*
* NOMANUAL
*/

STATUS _dbgStepAdd 
    (
    int tid,				/* task id */
    int breakType,			/* breakpoint type */
    BREAK_ESF * pInfo,			/* pointer to info saved on stack */
    int *  pRegSet			/* pointer to saved register set */
    )
    {
    INSTR      machInstr;		/* Machine instruction */
    INSTR *    npc;			/* next program counter */
    char       branchCondition;		/* Branch condition */
    REG_SET    regSet;			/* pointer to task registers */
    REG_SET *  pRegs = &regSet;		/* pointer to task registers */
    BRKENTRY * bp;
    UINT       op2;

    if (taskRegsGet (tid, pRegs) != OK)
        {
	return (ERROR);
	}
    npc	= pRegs->npc;             /* Default nPC */

    if ((bp = dbgBrkGet (pRegs->pc, tid, FALSE)) != NULL)
	machInstr = bp->code;
    else
    	machInstr = *(pRegs->pc);

    /* Conditional branch instruction with annul bit set */

    if (((machInstr & 0xe0000000) == A_1) &&
	 (((op2 = (machInstr >> 22) & 0x7) == 2) || (op2 == 6) || (op2 == 7)))
	{
	/* bits:25-28 common to all branch types */

	branchCondition   = ( machInstr >> 25 ) & 0xF; 

	/* Switch on the type of branch (Bicc, CBccc, FBfcc).  No coprocessor 
	 * is defined at this time (CBccc).  The location of status bits for 
	 * coprocessors is implementation specific 
	 */

	switch (op2)
	    {
	    case (2): /* Bicc  - Integer Conditional Branch */
		{
		BOOL 	carryFlag;
		BOOL	overflowFlag;
		BOOL	zeroFlag;
		BOOL	negativeFlag;

		carryFlag    = ((pRegs -> psr) >> 20) & 1; /* Carry */
		overflowFlag = ((pRegs -> psr) >> 21) & 1; /* oVerflow */
		zeroFlag     = ((pRegs -> psr) >> 22) & 1; /* Zero */
		negativeFlag = ((pRegs -> psr) >> 23) & 1; /* Negative */

		switch (branchCondition)
		    {
		    case (0): /* Branch Never */
			    ++npc;
			break;
		    case (1): /* Branch On equal */
			if ( !zeroFlag )
			    ++npc;
			break;
		    case (2): /* Branch On Less or Equal */
			if ( !(zeroFlag | ( negativeFlag ^ overflowFlag )))
			    ++npc;
			break;
		    case (3): /* Branch on less than */
			if (!(negativeFlag ^ overflowFlag))
			    ++npc;
			break;
		    case (4): /* Branch on less, or equal, unsigned */
			if (!(carryFlag | zeroFlag))
			    ++npc;
			break;
		    case (5): /* Branch on carry set (less than, unsigned) */
			if (!carryFlag )
			    ++npc;
			break;
		    case (6): /* Branch on negative */
			if ( ! negativeFlag )
			    ++npc;
			break;
		    case (7): /* Branch on oVerflow */
			if ( ! overflowFlag )
			    ++npc;
			break;
		    case (8): /* Branch Always */
			if ((machInstr & DISP22_SIGN) == DISP22_SIGN)
			    {
			    /* Negative displacement - sign-extend, 
			     * two's complement 
			     */
			    machInstr = ~DISP22 | (machInstr & DISP22);
			    machInstr = ~machInstr + 1;
			    npc = (INSTR *)((INSTR)pRegs->pc
					    - (machInstr << DISP22_SHIFT_CT));
			    }
			else
			    {
			    /* Positive displacement */
			    machInstr &= DISP22;
			    npc = (INSTR *)((INSTR)pRegs->pc
					    + (machInstr << DISP22_SHIFT_CT));
			    }
			break;
		    case (9): /* Branch on Not Equal */
			if ( zeroFlag )
			    ++npc;
			break;
		    case (0xa): /* Branch on Greater */
			if ( ( zeroFlag | ( negativeFlag ^ overflowFlag )))
			    ++npc;
			break;
		    case (0xb): /* Branch on Greater or Equal */
			if ( negativeFlag ^ overflowFlag )
			    ++npc;
			break;
		    case (0xc): /* Branch on Greater, Unsigned */
			if ( carryFlag | zeroFlag )
			    ++npc;
			break;
		    case (0xd): /* Branch on Carry Clear */
			if ( carryFlag )
			    ++npc;
			break;
		    case (0xe): /* Branch on Positive */
			if ( negativeFlag )
			    ++npc;
			break;
		    case (0xf): /* Branch on oVerflow Clear*/
			if ( overflowFlag )
			    ++npc;
			break;
		    }
		}
		break;

	    case (6): /* FBfcc - Floating-Point Conditional Branch */
		{
		BOOL	equalState = FALSE;
		BOOL	lessState = FALSE;
		BOOL	greaterState = FALSE;
		BOOL	unorderedState = FALSE;

		UINT	state  = (((struct fpContext *)
				    ( taskTcb(tid) -> pFpContext ))
				      -> fsr & 0xc00 ) >> 10;
		switch (state)
		    {
		    case (0):
			equalState = TRUE;
			break;
		    case (1):
			lessState = TRUE;
			break;
		    case (2):
			greaterState = TRUE;
			break;
		    case (3):
			unorderedState = TRUE;
			break;
		    default:
			printf ("Impossible state:%d\n", state);
			return (ERROR);
		    }

		switch (branchCondition)
		    {
		    case (0): /* Branch Never */
		        npc++;
			break;
		    case (1): /* Branch On Not equal */
			if ( !( unorderedState | lessState | greaterState ))
			    ++npc;
			break;
		    case (2): /* Branch On Less or Greater */
			if ( ! ( lessState | greaterState ))
			    ++npc;
			break;
		    case (3): /* Branch on Unordered or Less */
			if ( ! ( unorderedState | lessState ))
			    ++npc;
			break;
		    case (4): /* Branch on less */
			if ( !lessState )
			    ++npc;
			break;
		    case (5): /* Branch on Unordered or Greater */
			if ( ! (unorderedState | greaterState ))
			    ++npc;
			break;
		    case (6): /* Branch on Greater */
			if ( !greaterState )
			    ++npc;
			break;
		    case (7): /* Branch on Unordered */
			if ( !unorderedState )
			    ++npc;
			break;
		    case (8): /* Branch Always */
			if ((machInstr & DISP22_SIGN) == DISP22_SIGN)
			    {
			    /* Negative displacement - sign-extend, 
			     * two's complement 
			     */
			    machInstr = ~DISP22 | (machInstr & DISP22);
			    machInstr = ~machInstr + 1;
			    npc = (INSTR *)((INSTR)pRegs->pc
					    - (machInstr << DISP22_SHIFT_CT));
			    }
			else
			    {
			    /* Positive displacement */
			    machInstr &= DISP22;
			    npc = (INSTR *)((INSTR)pRegs->pc
					    + (machInstr << DISP22_SHIFT_CT));
			    }
			break;
		    case (9): /* Branch Equal */
			if ( !equalState )
			    ++npc;
			break;
		    case (0xa): /* Branch on Unordered or Equal */
			if ( !(unorderedState | equalState ))
			    ++npc;
			break;
		    case (0xb): /* Branch on Greater or Equal */
			if (!(greaterState | equalState))
			    ++npc;
			break;
		    case (0xc): /* Branch on Unordered or Greater, or Equal */
			if ( !( unorderedState | greaterState | equalState ))
			    ++npc;
			break;
		    case (0xd): /* Branch on Less or Equal */
			if ( !(lessState | equalState))
			    ++npc;
			break;
		    case (0xe): /* Branch on Unordered or Less or Equal */
			if ( !(unorderedState | lessState | equalState))
			    ++npc;
			break;
		    case (0xf): /* Branch on Unordered */
			if ( !(lessState | greaterState | equalState))
			    ++npc;
			break;
		    }
		}
		break;

	    case (7):  /* CBccc - Coprocessor Conditional Branch */
		printf ("Coprocessor conditional branches not supported\n");
	        return (ERROR);
	        break;

	    default:
		printf ("Invalid conditional branch instruction\n");
		return (ERROR);
		break;
	    }
	}

    if (dbgBrkAdd (npc, tid, 0, breakType) != OK)
	{
	printf ("error inserting breakpoint\n");
	return (ERROR);
	}

    return (OK);
    }

/*******************************************************************************
*
* psrShow - display the meaning of a specified `psr' value, symbolically (SPARC)
*
* This routine displays the meaning of all the fields in a specified `psr' value,
* symbolically.
*
* Extracted from psl.h:
* .CS
* Definition of bits in the Sun-4 PSR (Processor Status Register)
*  ------------------------------------------------------------------------
* | IMPL | VER |      ICC      | resvd | EC | EF | PIL | S | PS | ET | CWP |
* |      |     | N | Z | V | C |       |    |    |     |   |    |    |     |
* |------|-----|---|---|---|---|-------|----|----|-----|---|----|----|-----|
*  31  28 27 24  23  22  21  20 19   14  13   12  11  8   7   6    5  4   0
* .CE
* For compatibility with future revisions, reserved bits are defined to be
* initialized to zero and, if written, must be preserved.
*
* EXAMPLE
* .CS
*      -> psrShow 0x00001FE7
*     Implementation 0, mask version 0:
*     Fujitsu MB86900 or LSI L64801, 7 windows
*             no SWAP, FSQRT, CP, extended fp instructions
*         Condition codes: . . . .
*         Coprocessor enables: . EF
*         Processor interrupt level: f
*         Flags: S PS ET
*         Current window pointer: 0x07
*      ->
* .CE
*
* RETURNS: N/A
*
* SEE ALSO: psr(),
* .I "SPARC Architecture Manual"
*
* NOMANUAL
*/

void psrShow 
    (
    ULONG psrValue	/* psr value to show */
    )

    {
    printf ("Implementation %x, mask version %x:\n",
	    (psrValue & PSR_IMPL) >> 28,
	    (psrValue & PSR_VER) >> 28);

    switch ((psrValue & PSR_IMPL) >> 28)
	{
	case (0x0):
	    if (((psrValue & PSR_VER) >> 24) == 0)
		{
		printf ("Fujitsu MB86900 or LSI L64801, 7 windows\n");
		}
	    else if (((psrValue & PSR_VER) >> 24) == 2)
		{
		printf ("Fujitsu MB86930, 8 windows\n");
		}
	    break;

	case (0x1):
	    printf ("Cypress CY7C601, 8 windows.\n");
	    printf ("        no extended fp instructions\n");
	    if (((psrValue & PSR_VER) >> 24) > 1)
		printf ("        (or possibly a Cypress/Ross successor).\n");
	    break;

	case (0x2):
	    printf ("BIT B5000, 7 windows\n");
	    printf ("        no extended fp instructions\n");
	    printf ("        'some new hardware traps'\n");
	    if (((psrValue & PSR_VER) >> 24) > 0)
		printf ("        (or possibly a BIT successor).\n");
	    break;

	case (0x9):
	    if (((psrValue & PSR_VER) >> 24) == 0)
		{
		printf ("Fujitsu MB86903, 8 windows\n");
		}
	    break;

	default:
	    break;
	}

    printf ("    Condition codes: ");
    if (psrValue & PSR_N)
	printf ("N ");
    else
	printf (". ");
    if (psrValue & PSR_Z)
	printf ("Z ");
    else
	printf (". ");
    if (psrValue & PSR_V)
	printf ("V ");
    else
	printf (". ");
    if (psrValue & PSR_C)
	printf ("C\n");
    else
	printf (".\n");

    printf ("    Coprocessor enables: ");
    if (psrValue & PSR_EC)
	printf ("EC ");
    else
	printf (". ");
    if (psrValue & PSR_EF)
	printf ("EF\n");
    else
	printf (".\n");

    printf ("    Processor interrupt level: %x\n",
	(psrValue & PSR_PIL) >> 8);
    printf ("    Flags: ");
    if (psrValue & PSR_S)
	printf ("S ");
    else
	printf (". ");
    if (psrValue & PSR_PS)
	printf ("PS ");
    else
	printf (". ");
    if (psrValue & PSR_ET)
	printf ("ET\n");
    else
	printf (".\n");

    printf ("    Current window pointer: 0x%02x\n",
	psrValue & PSR_CWP);
    }

/*******************************************************************************
*
* fsrShow - display the meaning of a specified fsr value, symbolically (SPARC)
*
* This routine displays the meaning of all the fields in a specified `fsr' value,
* symbolically.
*
* Extracted from reg.h:
* .CS
* Definition of bits in the Sun-4 FSR (Floating-point Status Register)
*   -------------------------------------------------------------
*  |  RD |  RP | TEM |  res | FTT | QNE | PR | FCC | AEXC | CEXC |
*  |-----|---- |-----|------|-----|-----|----|-----|------|------|
*   31 30 29 28 27 23 22  17 16 14   13   12  11 10 9    5 4    0
* .CE
* For compatibility with future revisions, reserved bits are defined to be
* initialized to zero and, if written, must be preserved.
*
* EXAMPLE
* .CS
*     -> fsrShow 0x12345678
*     Rounding Direction: nearest or even if tie.
*     Rounding Precision: single.
*     Trap Enable Mask:
*        underflow.
*     Floating-point Trap Type: IEEE exception.
*     Queue Not Empty: FALSE;
*     Partial Remainder: TRUE;
*     Condition Codes: less than.
*     Accumulated exceptions:
*        inexact divide-by-zero invalid.
*     Current exceptions:
*        overflow invalid
* .CE
*
* RETURNS: N/A
*
* SEE ALSO:
* .I "SPARC Architecture Manual"
*
* NOMANUAL
*/

void fsrShow
    (
    UINT fsrValue	/* fsr value to show */
    )

    {
    printf ("    Rounding Direction: ");
    switch ((fsrValue & FSR_RD) >> FSR_RD_SHIFT)
	{
	case (RD_NEAR):
	    printf ("nearest or even if tie.");
	    break;

	case (RD_ZER0):
	    printf ("to zero.");
	    break;

	case (RD_POSINF):
	    printf ("positive infinity.");
	    break;

	case (RD_NEGINF):
	    printf ("negative infinity.");
	    break;
	}

    printf ("\n    Rounding Precision: ");
    switch ((fsrValue & FSR_RP) >> FSR_RP_SHIFT)
	{
	case (RP_DBLEXT):
	    printf ("double-extended.");
	    break;

	case (RP_SINGLE):
	    printf ("single.");
	    break;

	case (RP_DOUBLE):
	    printf ("double.");
	    break;

	case (RP_RESERVED):
	    printf ("unused and reserved.");
	    break;
	}

    printf ("    Trap Enable Mask:");
    fsrExcShow ((fsrValue & FSR_TEM) >> FSR_TEM_SHIFT);

    printf ("    Floating-point Trap Type: ");
    switch ((fsrValue & FSR_FTT) >> FSR_FTT_SHIFT)
	{
	case (FTT_NONE):
	    printf ("none.");
	    break;

	case (FTT_IEEE):
	    printf ("IEEE exception.");
	    break;

	case (FTT_UNFIN):
	    printf ("unfinished fpop.");
	    break;

	case (FTT_UNIMP):
	    printf ("unimplemented fpop.");
	    break;

	case (FTT_SEQ):
	    printf ("sequence error.");
	    break;

	case (FTT_ALIGN):
	    printf ("alignment, by software convention.");
	    break;

	case (FTT_DFAULT):
	    printf ("data fault, by software convention.");
	    break;

	default:
	    printf ("unknown exception %x.",
		(fsrValue & FSR_FTT_SHIFT) >> FSR_FTT_SHIFT);
	}

    printf ("\n    Queue Not Empty: ");
    if (fsrValue & FSR_QNE)
	printf ("TRUE");
    else
	printf ("FALSE");
    printf ("\n    Partial Remainder: ");
    if (fsrValue & FSR_PR)
	printf ("TRUE");
    else
	printf ("FALSE");
    printf ("\n    Condition Codes: ");
    if ((fsrValue & FSR_FCC) == FSR_FCC_EQUAL)
	printf ("equal.");
    if ((fsrValue & FSR_FCC) == FSR_FCC_LT)
	printf ("less than.");
    if ((fsrValue & FSR_FCC) == FSR_FCC_GT)
	printf ("greater than.");
    if ((fsrValue & FSR_FCC) == FSR_FCC_UNORD)
	printf ("unordered.");

    printf ("\n    Accumulated exceptions:");
    fsrExcShow ((fsrValue & FSR_AEXC) >> FSR_AEXC_SHIFT);

    printf ("    Current exceptions:");
    fsrExcShow (fsrValue & FSR_CEXC);
    }

/*******************************************************************************
*
* fsrExcShow - display the meaning of a specified `fsr' exception field
*
* This routine displays the meaning of all the fields in a specified `fsr'
* exception field, symbolically.
*
* SEE ALSO: SPARC Architecture manual.
*
* NOMANUAL
*/

LOCAL void fsrExcShow (fsrExcField)
    UINT fsrExcField;

    {
    printf ("       ");
    if ((fsrExcField & FSR_CEXC) == 0)
	printf ("none");
    else
	{
	if (fsrExcField & FSR_CEXC_NX)
	    printf (" inexact");
	if (fsrExcField & FSR_CEXC_DZ)
	    printf (" divide-by-zero");
	if (fsrExcField & FSR_CEXC_UF)
	    printf (" underflow");
	if (fsrExcField & FSR_CEXC_OF)
	    printf (" overflow");
	if (fsrExcField & FSR_CEXC_NV)
	    printf (" invalid");
	}
    printf (".\n");
    }
/*******************************************************************************
*
* getOneReg - return the contents of one register
*
* Given a task's ID, this routine returns the contents of the register
* specified by the register code.  This routine is used by g0(), i0(), psr(), 
* and so on.
* The register codes are defined in regsSparc.h.
*
* RETURNS: register contents, or ERROR.
*/

LOCAL int getOneReg
    (
    int taskId,		/* task ID, 0 means default task */
    int regCode		/* code for specifying register */
    )
    {
    REG_SET regSet;	/* get task's regs into here */

    taskId = taskIdFigure (taskId);	/* translate super name to ID */

    if (taskId == ERROR)		/* couldn't figure out super name */
	return (ERROR);
    taskId = taskIdDefault (taskId);	/* set the default ID */

    if (taskRegsGet (taskId, &regSet) != OK)
	return (ERROR);

    return (*(int *)((int)&regSet + regCode));
    }

/*******************************************************************************
*
* g0 - return the contents of register `g0' (also `g1' - `g7') (SPARC)
*
* This command extracts the contents of global register `g0' from the TCB of a
* specified task.  If <taskId> is omitted or 0, the current default task is
* assumed.
*
* Similar routines are provided for all global registers (`g0' - `g7'):
* g0() - g7().
*
* RETURNS: The contents of register `g0' (or the requested register).
*
* SEE ALSO:
* .pG "Target Shell"
*
* INTERNAL
* Although this routine is hereby marked NOMANUAL, it actually gets
* published, but from arch/doc/dbgArchLib.c.
* ...not any more -- SPARC no longer supported.
*
* NOMANUAL
*/

int g0
    (
    int taskId		/* task ID, 0 means default task */
    )
    
    {
    return (getOneReg (taskId, REG_SET_GLOBAL(0)));
    }

int g1 (int taskId) { return (getOneReg (taskId, REG_SET_GLOBAL(1))); }
int g2 (int taskId) { return (getOneReg (taskId, REG_SET_GLOBAL(2))); }
int g3 (int taskId) { return (getOneReg (taskId, REG_SET_GLOBAL(3))); }
int g4 (int taskId) { return (getOneReg (taskId, REG_SET_GLOBAL(4))); }
int g5 (int taskId) { return (getOneReg (taskId, REG_SET_GLOBAL(5))); }
int g6 (int taskId) { return (getOneReg (taskId, REG_SET_GLOBAL(6))); }
int g7 (int taskId) { return (getOneReg (taskId, REG_SET_GLOBAL(7))); }

/*******************************************************************************
*
* o0 - return the contents of register `o0' (also `o1' - `o7') (SPARC)
*
* This command extracts the contents of out register `o0' from the TCB of a
* specified task.  If <taskId> is omitted or 0, the current default task is
* assumed.
*
* Similar routines are provided for all out registers (`o0' - `o7'):
* o0() - o7().
*
* The stack pointer is accessed via `o6'.
*
* RETURNS: The contents of register `o0' (or the requested register).
*
* SEE ALSO:
* .pG "Target Shell"
*
* NOMANUAL
*/

int o0
    (
    int taskId		/* task ID, 0 means default task */
    )
    
    {
    return (getOneReg (taskId, REG_SET_OUT(0)));
    }

int o1 (int taskId) { return (getOneReg (taskId, REG_SET_OUT(1))); }
int o2 (int taskId) { return (getOneReg (taskId, REG_SET_OUT(2))); }
int o3 (int taskId) { return (getOneReg (taskId, REG_SET_OUT(3))); }
int o4 (int taskId) { return (getOneReg (taskId, REG_SET_OUT(4))); }
int o5 (int taskId) { return (getOneReg (taskId, REG_SET_OUT(5))); }
int o6 (int taskId) { return (getOneReg (taskId, REG_SET_OUT(6))); }
int o7 (int taskId) { return (getOneReg (taskId, REG_SET_OUT(7))); }

/*******************************************************************************
*
* l0 - return the contents of register `l0' (also `l1' - `l7') (SPARC)
*
* This command extracts the contents of local register `l0' from the TCB of a
* specified task.  If <taskId> is omitted or 0, the current default task is
* assumed.
*
* Similar routines are provided for all local registers (`l0' - `l7'):
* l0() - l7().
*
* RETURNS: The contents of register `l0' (or the requested register).
*
* SEE ALSO:
* .pG "Target Shell"
*
* NOMANUAL
*/

int l0
    (
    int taskId		/* task ID, 0 means default task */
    )
    
    {
    return (getOneReg (taskId, REG_SET_LOCAL(0)));
    }

int l1 (int taskId) { return (getOneReg (taskId, REG_SET_LOCAL(1))); }
int l2 (int taskId) { return (getOneReg (taskId, REG_SET_LOCAL(2))); }
int l3 (int taskId) { return (getOneReg (taskId, REG_SET_LOCAL(3))); }
int l4 (int taskId) { return (getOneReg (taskId, REG_SET_LOCAL(4))); }
int l5 (int taskId) { return (getOneReg (taskId, REG_SET_LOCAL(5))); }
int l6 (int taskId) { return (getOneReg (taskId, REG_SET_LOCAL(6))); }
int l7 (int taskId) { return (getOneReg (taskId, REG_SET_LOCAL(7))); }

/*******************************************************************************
*
* i0 - return the contents of register `i0' (also `i1' - `i7') (SPARC)
*
* This command extracts the contents of in register `i0' from the TCB of a
* specified task.  If <taskId> is omitted or 0, the current default task is
* assumed.
*
* Similar routines are provided for all in registers (`i0' - `i7'):
* i0() - i7().
*
* The frame pointer is accessed via `i6'.
*
* RETURNS: The contents of register `i0' (or the requested register).
*
* SEE ALSO:
* .pG "Target Shell"
*
* NOMANUAL
*/

int i0
    (
    int taskId		/* task ID, 0 means default task */
    )
    
    {
    return (getOneReg (taskId, REG_SET_IN(0)));
    }

int i1 (int taskId) { return (getOneReg (taskId, REG_SET_IN(1))); }
int i2 (int taskId) { return (getOneReg (taskId, REG_SET_IN(2))); }
int i3 (int taskId) { return (getOneReg (taskId, REG_SET_IN(3))); }
int i4 (int taskId) { return (getOneReg (taskId, REG_SET_IN(4))); }
int i5 (int taskId) { return (getOneReg (taskId, REG_SET_IN(5))); }
int i6 (int taskId) { return (getOneReg (taskId, REG_SET_IN(6))); }
int i7 (int taskId) { return (getOneReg (taskId, REG_SET_IN(7))); }

/*******************************************************************************
*
* npc - return the contents of the next program counter (SPARC)
*
* This command extracts the contents of the next program counter from the TCB
* of a specified task.  If <taskId> is omitted or 0, the current default
* task is assumed.
*
* RETURNS: The contents of the next program counter.
*
* SEE ALSO: ti()
*
* NOMANUAL
*/

int npc
    (
    int taskId                 /* task ID, 0 means default task */
    )

    {
    return (getOneReg (taskId, REG_SET_NPC));
    }

/*******************************************************************************
*
* psr - return the contents of the processor status register (SPARC)
*
* This command extracts the contents of the processor status register from
* the TCB of a specified task.  If <taskId> is omitted or 0, the default
* task is assumed.
*
* RETURNS: The contents of the processor status register.
*
* SEE ALSO: psrShow(),
* .pG "Target Shell"
*
* NOMANUAL
*/

int psr
    (
    int taskId			/* task ID, 0 means default task */
    )
    {
    return (getOneReg (taskId, REG_SET_PSR));
    }

/*******************************************************************************
*
* wim - return the contents of the window invalid mask register (SPARC)
*
* This command extracts the contents of the window invalid mask register from
* the TCB of a specified task.  If <taskId> is omitted or 0, the default
* task is assumed.
*
* RETURNS: The contents of the window invalid mask register.
*
* SEE ALSO:
* .pG "Target Shell"
*
* NOMANUAL
*/

int wim
    (
    int taskId 			/* task ID, 0 means default task */
    )

    {
    return (getOneReg (taskId, REG_SET_WIM));
    }

/*******************************************************************************
*
* y - return the contents of the `y' register (SPARC)
*
* This command extracts the contents of the `y' register from the TCB of a
* specified task.  If <taskId> is omitted or 0, the default task is assumed.
*
* RETURNS: The contents of the y register.
*
* SEE ALSO:
* .pG "Target Shell"
*
* NOMANUAL
*/

int y
    (
    int taskId 			/* task ID, 0 means default task */
    )
    {
    return (getOneReg (taskId, REG_SET_Y));
    }


/* dbgArchLib.c - Intel i960 architecture-dependent debugger library */

/* Copyright 1984-1995 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01u,10feb95,jdi  changed 80960 to i960; added missing parens to register names.
01t,13feb93,jdi  documentation cleanup for 5.1; created discrete entries
		 for register routines.
01s,10feb93,yao  installed register display routines based on 5.0.6.
01r,25sep92,wmd  fix ansi warnings.
01q,23aug92,jcf  made filenames consistant.
01p,13aug92,yao  fixed bug in dbgTraceFaultHandle() which restores register
		 sets incorrectly.
01o,10jul92,yao  changed reference of pTcb->pDbgState->pDbgSave->taskPCW
		 to pTcb->dbgPCWSave.
01n,06jul92,yao  removed dbgCacheClear().  made user uncallable globals
		 started with '_'.
01m,04jul92,jcf  scalable/ANSI/cleanup effort.
01l,09jun92,yao  made dbgInterruptPCW LOCAL.  added dbgInit(), dbgHwAdrsCheck().
		 removed dbgDsmInst(), dbgBreakInstGet(), dbgHwBpFree(), 
		 dbgHwBpList, dbgCodeInsert(), dbgHwBpGet(), dbgHwBpInit(), 
		 dbgRegSetPCGet(), dbgTaskProcStatusSave(),dbgTaskPC{S,G}et().
		 changed dbgTaskBPModeSet(), dbgTaskSStepSet() to void.  added
		 dbgArchHwBpFree().
01k,26may92,rrr  the tree shuffle
01j,23apr92,wmd  fixed ansi warnings.
01i,02mar92,wmd  added conditionals for ICE support for i960KB.
01h,04oct91,rrr  passed through the ansification filter
		  -fixed #else and #endif
		  -changed READ, WRITE and UPDATE to O_RDONLY O_WRONLY O_RDWR
		  -changed VOID to void
		  -changed copyright notice
01g,14aug91,del  changes by intel to support KA/KB processors.
01f,12jul91,gae  changed many sys{IMR,IPND,...} to vx{...} names;
		 removal of 68k code comments left for reference.
01e,20apr91,del  added code to dbgReturnAddrGet to deal with leaf procedures.
01d,19mar91,del  redesigned hardware breakpoints.
01c,08jan91,del  documentation.
01b,06jan91,del  changed to work with new REG_SET structure. cleanup.
01a,21oct89,del  written.
*/

/*
DESCRIPTION
This module provides the Intel i960-specific support functions for dbgLib.

NOMANUAL
*/

#include "vxWorks.h"
#include "private/taskLibP.h"
#include "private/windLibP.h"
#include "private/kernelLibP.h"
#include "private/dbgLibP.h"
#include "lstLib.h"
#include "symLib.h"
#include "sysSymTbl.h"
#include "logLib.h"
#include "string.h"
#include "vxLib.h"
#include "stdio.h"
#include "ioLib.h"
#include "usrLib.h"
#include "fppLib.h"

#if	CPU==I960CA
#define	REG_NUM_IPB0	0	/* instruction address breakpoint register 0 */
#define REG_NUM_IPB1	1	/* instruction  address breakpoint register 1 */
#define REG_NUM_DAB0	2	/* data address breakpoint register 0 */
#define	REG_NUM_DAB1	3	/* data address breakpoint register 1 */
#endif	/* CPU==I960CA */

#define MOV_G14_MASK	0xff87ffff	/* mov g14, gx instruction mask */
#define INST_MOV_G14	0x5c80161e	/* mov g14, gx instruction */
#define	SRC_REG_MASK	0x00f80000	/* register mask */

/* externals */
 
IMPORT	void	   dbgBreakpoint ();	/* higher level of bp handling	*/
IMPORT	void	   dbgTrace ();		/* higher level of trace handling */
IMPORT 	void 	   dbgBpStub ();	/* lowest level of bp handling	*/
IMPORT 	BRKENTRY * dbgBrkGet ();

IMPORT int         dsm960Inst ();	/* 960 disassembler routine */
IMPORT BOOL        dsmMemInstrCheck ();
IMPORT UINT32      dsmMEMInstrRefAddrGet ();
IMPORT int         dsmInstrSizeGet ();
IMPORT UINT32      sysFaultVecSet ();
IMPORT BOOL        dsmFuncCallCheck ();

IMPORT UINT32      sysCtrlTable[];
IMPORT int	   ansiFix;		/* fix ansi warnings */
/* globals */

extern char * _archHelp_msg = 		/* help message */
    "pfp, tsp, rip, fp  [task]       Display pfp, sp, rip, and fp of a task\n"
    "r3-r15, g0-g14     [task]       Display a register of a task\n"

#if	CPU == I960CA
    "bh addr[,access[,task[,count[,quite]]]] Set hardware breakpoint\n"
    "                           0 - store only       1 - load/store\n"
    "                           2 - data/inst fetch  3 - any access\n" 
    "                           4 - instruct\n"
#endif	/* CPU == I960CA */

#if (CPU == I960KB)
    "fp0-fp3            [task]  Display floating point register of a task\n"
#endif	/* CPU == I960KB */
    ;

/* forward declaration */

LOCAL int dbgInterruptPCW;	/* used by dbgIntrInfoSave/Restore */
LOCAL int dbgHwRegStatus;	/* keep status of hardware regs being used */


/*******************************************************************************
*
* _dbgArchInit - architecture dependent initialization routine
*
* This routine initialize global function pointers that are architecture 
* specific.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void _dbgArchInit (void)

    {
    _dbgDsmInstRtn = (FUNCPTR) dsm960Inst;
    }
/******************************************************************************
*
*  _dbgTraceDisable - disable trace mode
*
* NOMANUAL
*/

void _dbgTraceDisable (void)

    {
    vxPCWClear (PCW_TRACE_ENABLE_MASK);
    }
/******************************************************************************
*
* _dbgFuncCallCheck - check if opcode is a function call
*
* RETURNS: TRUE if op code at the given addr is a subroutine call.
*
* NOMANUAL
*/

BOOL _dbgFuncCallCheck 
    (
    INSTR * pInst		/* pointer to instruction to check */
    )
    {
    return (dsmFuncCallCheck (pInst));
    }
#if	(BRK_HW_BP != 0)
/****************************************************************************** *
* _dbgHwAdrsCheck - verify address is valid
*
* Checks to make sure the given adrs is on a 32-bit boundary, and
* that it is accessable.
*
* RETURNS: OK or ERROR if address not on a 32-bit boundary or unaccessible.
*
* NOMANUAL
*/

STATUS _dbgHwAdrsCheck 
    (
    INSTR *	adrs,		/* address to check */
    int		access		/* access type */
    )
    {
    UINT32 val;			/* dummy for vxMemProbe */

    if ((int) adrs & 0x03)
	return (ERROR);

#if	CPU==I960CA
    switch (access)
	{
	case DAB_ACCESS_STORE:
	    if (vxMemProbe ((char *)adrs, O_WRONLY, 4, (char *) &val) != OK)
		{
		return (ERROR);
		}
	    break;

	case HW_INST_BRK:
	    if (vxMemProbe ((char *)adrs, O_RDONLY, 4, (char *) &val) != OK)
		{
		return (ERROR);
		}
	    break;

        case DAB_ACCESS_DATA_LOAD_OR_STORE:
	case DAB_ACCESS_DATA_OR_INSTR_FETCH: 
	case DAB_ACCESS_ANY_ACCESS: 
	     if (vxMemProbe ((char *)adrs, O_RDONLY, 4, (char *) &val) != OK ||
	         vxMemProbe ((char *)adrs, O_WRONLY, 4, (char *) &val) != OK)
		 {
		 return (ERROR);
		 }
	    break;

	default:
	    break;
	}
#endif	/* CPU==I960CA */

    return (OK);
    }
/******************************************************************************
*
* _dbgHwDisplay - display a hardware breakpoint 
*
* NOMANUAL
*/

void _dbgHwDisplay 
    (
    BRKENTRY * bp		/* breakpoint table entry */
    )
    {
    if ((bp->type & BRK_HW_BP) == 0)
	return;

    printf (" (hard-");

#if CPU==I960CA
    switch (bp->pHwBp->hbAccess)
	{
	case DAB_ACCESS_STORE:
	    printf ("store only)");
	    break;

	case DAB_ACCESS_DATA_LOAD_OR_STORE:
	    printf ("data only)");
	    break;

	case DAB_ACCESS_DATA_OR_INSTR_FETCH:
	    printf ("data/instr fetch)");
	    break;

	case DAB_ACCESS_ANY_ACCESS:
	    printf ("any access)");
	    break;

	case HW_INST_BRK:
	    printf ("instruction)");
	    break;

	default:
	    printf ("unknown)");
	    break;
	}
#endif	/* CPU==I960CA */
    }
/******************************************************************************
*
* _dbgHwBpSet - set a data breakpoint register
*
* Access is the type of access that will generate a breakpoint.
*  000 - store only
*  001 - data only (load or store)
*  010 - data or instruction fetch
*  011 - any access
*  100 - instruction breakpoint
*
* NOMANUAL
*/

void _dbgHwBpSet 
    (
    INSTR * 	addr,		/* address on which to break */
    HWBP *	pHwBp		/* hardware breakpoint */
    )
    {
#if CPU==I960CA
    switch (pHwBp->hbRegNum)
	{
	case REG_NUM_DAB0:
	    dbgDAB0Set ((char *) addr);
	    dbgDAB0Enable (pHwBp->hbAccess);
	    break;
	case REG_NUM_DAB1:
	    dbgDAB1Set ((char *) addr);
	    dbgDAB1Enable (pHwBp->hbAccess);
	    break;
	case REG_NUM_IPB0:
	    (void) dbgIPB0Set ((char *)addr);
	    break;
	case REG_NUM_IPB1:
	    (void) dbgIPB1Set ((char *)addr);
	    break;
	default:
	    break;
	}
#endif	/* CPU==I960CA */
    }
/******************************************************************************
*
* _dbgHwBpCheck - check if instruction could have caused a data breakpoint
*
* Check to see if the instruction at the given address caused a
* memory reference that matches one of the DAB registers.
* Or if the address of the instruction itself caused a break.
* This would be true if the access type was for instruction fetch.
*
* RETURNS: TRUE if match, FALSE if no match.
*
* NOMANUAL
*/

BOOL _dbgHwBpCheck
    (
    INSTR *	pInstr,	/* ptr to instruction to check 	*/
    BRKENTRY *	bp,		/* pointer to breakpoint entry */
    int		tid,		/* task's id */
    volatile BOOL checkFlag	/* flag to check if breakpoint hit */
    )
    {
#if CPU==I960CA
    UINT32 	addr1;
    REG_SET	regSet;

    if ((bp->type & BRK_HW_BP) == 0)
	return (FALSE);

    /* check to see if instruction addr caused break */
    if (pInstr == bp->addr)
	return (TRUE);

    taskRegsGet (tid, &regSet);

    /* check to see if memory reference caused break */
    if (dsmMemInstrCheck(pInstr))
	{
	addr1 = dsmMEMInstrRefAddrGet(pInstr, (UINT32 *)&regSet);
	if (bp->addr == (INSTR *) addr1)
	    return (TRUE);
	}

    if ((UINT32)bp->addr == (sysCtrlTable[0]&0xfffffffc))
	return (TRUE);

    if ((UINT32)bp->addr == (sysCtrlTable[1]&0xfffffffc))
	return (TRUE);

#endif	/* CPU==I960CA */
    return (FALSE);
    }
/******************************************************************************
*
* _dbgHwBpClear - clear a data breakpoint
*
* Clears a data breakpoint based on given regiser number, but does not
* free the resource. This is the mechanism used to clear disable
* hardware breakpoints when a task is unbreakable.
*
* NOMANUAL
*/

void _dbgHwBpClear 
    (
    HWBP * pHwBp		/* hardware breakpoint */
    )
    {
#if CPU==I960CA
    switch (pHwBp->hbRegNum)
	{
	case REG_NUM_DAB0:
	    dbgDAB0Set ((char *) -1);
	    dbgDAB0Disable ();
	    break;

	case REG_NUM_DAB1:
	    dbgDAB1Set ((char *) -1);
	    dbgDAB1Disable ();
	    break;
	case REG_NUM_IPB0:
	    (void) dbgIPB0Disable ();
	    break;

	case REG_NUM_IPB1:
	    (void) dbgIPB1Disable ();
	    break;
	}

#endif	/* CPU==I960CA */
    }
/******************************************************************************
*
* _dbgArchHwBpFree - free hardware breakpoint data entry
*
* NOMANUAL
*/

void _dbgArchHwBpFree
    (
    HWBP * pHwBp	/* pointer to hardware breakpoint data structure */
    )
    {
    dbgHwRegStatus &= ~(1<< pHwBp->hbRegNum);
    }
#endif	/*  (BRK_HW_BP != 0) */
/******************************************************************************
*
* _dbgInfoPCGet - exception frame's PC/IP (program counter/instruction 
*                    pointer)
*
* RETURNS: PC/IP
*
* NOMANUAL
*/

INSTR * _dbgInfoPCGet
    (
    BREAK_ESF * pBrkInfo	/* pointer to exception frame */
    )
    {
    return ((INSTR *) pBrkInfo->faultIP);
    }
/******************************************************************************
*
* _dbgInstSizeGet - get size of instruction in sizeof(INSTR)'s
*
* RETURNS: The size of the instruction.
*
* NOMANUAL
*/

int _dbgInstSizeGet 
    (
    INSTR * pInst	/* pointer to instruction to check */
    )
    {
    return (dsmNbytes (pInst) / sizeof (INSTR));
    }
/******************************************************************************
*
* _dbgIntrInfoRestore - restore the info saved by dbgIntrInfoSave
*
* NOMANUAL
*/

void _dbgIntrInfoRestore
    ( 
    BREAK_ESF *	pBrkInfo		/* pointer to execption frame */
    )
    {
    pBrkInfo->procCtrl = dbgInterruptPCW;
    }
/******************************************************************************
*
* _dbgIntrInfoSave - save info when breakpoints are hit at interrupt level.
*
* NOMANUAL
*/

void _dbgIntrInfoSave
    (
    BREAK_ESF *	pBrkInfo		/* pointer to breakpoint ESF */
    )
    {
    dbgInterruptPCW = pBrkInfo->procCtrl;
    }
/******************************************************************************
*
* _dbgRegsAdjust - adjust stack pointer
*
* Adjust the stack pointer of the given REG_SET to remove the 
* exception frame, caused by the TRACE FAULT.
*
* NOMANUAL
*/

void _dbgRegsAdjust
    (
    int		tid,		/* task's id */
    TRACE_ESF *	pBrkInfo,	/* pointer to exception frame 	 */
    int *	pRegSet,	/* pointer to tasks register set */
    volatile BOOL flag		/* tells what type of ESF to use */
    )
    {
    ansiFix = (int)pBrkInfo->faultIP;	 /* fix ansi warning */

    /* NOP for 80960 because fault record (ESF) is
     * stuck in just before the stack frame for the
     * fault handler.
     */
    taskRegsSet (tid, (REG_SET *) pRegSet);
    }
/******************************************************************************
*
* _dbgTaskPCSet - set task's pc
*
* NOMANUAL
*/

void _dbgTaskPCSet
    (
    int		tid,	/* task's id */
    INSTR *	pc,	/* pc to set */
    INSTR *	npc 	/* npc to set (not supported by 960) */
    )
    {
    REG_SET regSet;

    ansiFix = (int)npc;		/* fix ansi warning */

    if (taskRegsGet (tid, &regSet) ==OK)
	{
	regSet.rip = (UINT32) pc;
	taskRegsSet (tid, &regSet);
	}
    }
/******************************************************************************
*
* _dbgTaskPCGet - get task's pc
*
* NOMANUAL
*/

INSTR * _dbgTaskPCGet
    (
    int	tid		/* task's id */
    )
    {
    REG_SET regSet;

    taskRegsGet (tid, &regSet);
    return ((INSTR *) regSet.rip);
    }
/******************************************************************************
*
* _dbgRetAdrsGet - address of the subroutine in which has hit a breakpoint
*
* RETURNS: Address of the next instruction to be executed upon
*  	   return of the current subroutine.
*
* INTERNAL
* BAL instruction will need to be taken into account here.
*
* NOMANUAL
*/

INSTR * _dbgRetAdrsGet
    (
    REG_SET * pRegSet	/* reg set of broken task */
    )
    {
#define MAX_NAME_LEN 80

    UINT32	retVal;
    UINT32	type 	= 0x00;
    UINT32	regNum;
    INT8	name[MAX_NAME_LEN];
    REG_SET *	pPrevRegSet = (REG_SET *)pRegSet->pfp;
    INSTR * 	pInstr0 = (INSTR *)(pRegSet->rip & 0xfffffffc);
    INSTR * 	pInstr1 = pInstr0;


    /* from this address, find the symbol in the symbol table that
     * is closest yet lower than the address. If this symbol is a leaf-proc
     * entry, then get the return address either from gx or g14,
     * depending on whether or not the mov g14, gx instruction has been
     * executed. If the symbol is not a leaf_proc entry point,
     * it is a regular function entry. Get the return address from
     * the rip of the previous frame. Caveat: if an assembly language
     * symbol starts with an `_', this could fool this algo. into
     * thinking it is the start of a function.
     */

    strcpy (name, "\0");

    symFindByValue (sysSymTbl, (UINT) pInstr0, (char *) &name, 
		    (int *) &pInstr0, (SYM_TYPE *) &type);

    if (!strncmp (&name[strlen(name) - 3], ".lf", 3))
	{
	/* this is a leaf_proc. first we must check to see if we
	 * are sitting on the mov g14, gx instruction If we are,
	 * return the value in g14, otherwise, disect the instruction
	 * and figure out what register is holding the return value */

	if ((*pInstr1 & MOV_G14_MASK) == INST_MOV_G14)
	    retVal = pRegSet->g14;
	else
	    {
	    regNum = (*pInstr0 & SRC_REG_MASK) >> 0x13;
	    retVal = *(((UINT32 *)pRegSet) + regNum);
	    printf ("regNum: 0x%x, retVal: 0x%x 0x%x \n",
		    regNum, retVal, *pInstr0);
	    }

	/* we still have to figure out if we got to this point in the leaf-proc
	 * via a 'call' or 'bal' instruction. If we came via a call instruction,
	 * the address 'retVal' will point to the 'ret' instruction at the
	 * end of the function. Otherwise, it will be the address of the
	 * instruction following the 'bal'. If 'retVal' does not point to a
	 * 'ret' instruction, we can just return it.
	 *
	 * NOTE: We could still be fooled if caller's 'bal' or 'balx' is
	 * followed by 'ret.' Perhaps we should also check to see if the
	 * instruction preceding 'ret' is 'bx gx' and or some similar check.
	 */

	if (((*(INSTR *)retVal) & 0xff000000) != 0x0a000000)
	    return ((INSTR *)retVal);
	}

    /* non-leaf_proc function or 'called' leaf-proc */

    return ((INSTR *)(pPrevRegSet->rip & 0xfffffffc));
    }
/******************************************************************************
*
* _dbgSStepSet - set single step mode
*
* Function to set things up so that instruction trace mode (single step)
* will occur when the 'broken' task is run.
* This is done by setting the Trace Enable bit of the PCW in the
* exception frame, and setting the TCW to Instruction Trace mode.
*
* NOTE
* Trace mode is disabled in the process control word so that
* we don't start instruction tracing until we return to the
* the task that broke.
*
* NOMANUAL
*/

void _dbgSStepSet
    (
    BREAK_ESF *	pBrkInfo	/* pointer to exception frame */
    )
    {
    vxPCWClear (PCW_TRACE_ENABLE_MASK); 
#ifndef ICE960KB
    pBrkInfo->procCtrl |= PCW_TRACE_ENABLE_MASK; 
#endif
    vxTCWSet (TCW_MODE_INSTR_MASK);
    }
/******************************************************************************
*
* _dbgSStepClear - clear single step mode
*
* Turn off instruction trace mode (single stepping).
*
* NOMANUAL
*/

void _dbgSStepClear (void)

    {
    vxTCWClear (TCW_MODE_INSTR_MASK);
    }
/******************************************************************************
*
* _dbgTaskBPModeSet - set breakpoint mode of task
*
* Function to set the PCW and TCW in the given TCB so breakpoints will
* be enabled when the task is switched in.
*
* RETURNS: OK or ERROR if invalid <tid>.
*
* NOMANUAL
*/

void _dbgTaskBPModeSet
    (
    int 	tid		/* task's id */
    )
    {
    WIND_TCB* pTcb = taskTcb (tid);

    if (pTcb == NULL)
	return;

#ifndef ICE960KB
				/* enable tracing in PCW of task */
    pTcb->regs.pcw |= PCW_TRACE_ENABLE_MASK;

				/* enable break point tracing in TCW of task */
    pTcb->regs.tcw |= TCW_MODE_BP_MASK;

#endif
    }
/******************************************************************************
*
* _dbgTaskBPModeClear - clear breakpoint mode of task
*
* NOMANUAL
*/

void _dbgTaskBPModeClear
    (
    volatile int tid		/* task's id */
    )
    {
    }
/******************************************************************************
*
* _dbgTaskSStepClear - clear single step mode in task
*
* Set the PCW and TCW of the given task for trace (instruction) break
* mode, so that when the task is switched in, it will execute the
* next instruction and break.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void _dbgTaskSStepClear
    (
    int	tid		/* id of task to set */
    )
    {
    WIND_TCB * pTcb = taskTcb (tid);

    if (pTcb == NULL)
	return;

    if (pTcb->regs.tcw & TCW_MODE_INSTR_MASK)
	{
	pTcb->regs.tcw &= ~TCW_MODE_INSTR_MASK;
	pTcb->regs.pcw = pTcb->dbgPCWSave;
	}
    }
/******************************************************************************
*
* _dbgTaskSStepSet - set single step mode of task
*
* Set the PCW and TCW of the given task for trace (instruction) break
* mode, so that when the task is switched in, it will execute the
* next instruction and break.
*
* NOTE
* Interrupts are locked out for this task, until single stepping
* for the task is cleared.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void _dbgTaskSStepSet
    (
    int	tid		/* id of task to set */
    )
    {
    WIND_TCB * pTcb = taskTcb (tid);

    if (pTcb == NULL)
	return;

    pTcb->dbgPCWSave = pTcb->regs.pcw;
#ifdef ICE960KB
    pTcb->regs.pcw |= PCW_INT_LOCK;
#else
    pTcb->regs.pcw |= PCW_TRACE_ENABLE_MASK | PCW_INT_LOCK;
    pTcb->regs.tcw |= TCW_MODE_INSTR_MASK;
#endif
    }
/******************************************************************************
*
* _dbgVecInit - initialize the trace fault handler vector
*
* NOMANUAL
*/

void _dbgVecInit (void)

    {
    sysFaultVecSet (dbgBpStub, FT_TRACE, FTE_SYSTEM_CALL);
    }
#if	(BRK_HW_BP != 0)
/*******************************************************************************
* _dbgHwBrkExists - check if hardware breakpoint already exists
*
* RETURNS: TRUE if instuction pointer matches given hardware breakpoint entry
*          (always FALSE for KB).
*
* NOMANUAL
*/

BOOL _dbgHwBrkExists 
    (
    INSTR *	addr,			/* pointer to instruction hit break */
    BRKENTRY *	pBrkEntry,		/* pointer to breakpoint entry */
    int 	access,			/* access type */
    void *	pHwInfo			/* pointer to hardware brkpoint info */
    )
    {
    ansiFix = (int)pHwInfo;	/* fix ansi warning */

#if	CPU==I960CA
    if (pBrkEntry == NULL)
        return (FALSE);
    else
	return((pBrkEntry->addr == addr) && 
	       (pBrkEntry->pHwBp->hbAccess == access));
#else
    return (FALSE);
#endif	/* CPU==I960CA */
    }
/******************************************************************************
*
* _dbgHwBpGet - get a free hardware breakpoint register
*
* Increment the count of data breakpoints in the system, and check for
* max number reached.
*
* RETURNS: pointer to a free hardware breakpoint or NULL if out of resources.
*
* NOMANUAL
*/

HWBP * _dbgHwBpGet 
    (
    int    access,		/* access type */
    LIST * pDbgHwBpList		/* hardware breakpoint list */
    )
    {
    HWBP * pHwBp;
    int    regNum;

    switch (access)
	{
	case DAB_ACCESS_STORE:
	case DAB_ACCESS_DATA_LOAD_OR_STORE:
	case DAB_ACCESS_DATA_OR_INSTR_FETCH:
	case DAB_ACCESS_ANY_ACCESS:
	    /* data address breakpoint registers */

	    if ((dbgHwRegStatus & (1 << REG_NUM_DAB0)) == 0)
		regNum = REG_NUM_DAB0;
	    else if ((dbgHwRegStatus & (1 << REG_NUM_DAB1)) == 0)
		regNum = REG_NUM_DAB1;
	    else
		regNum = -1;
	    break;

	case HW_INST_BRK:
	    /* instruction address breakpoint registers */

	    if ((dbgHwRegStatus & (1 << REG_NUM_IPB0)) == 0)
		regNum = REG_NUM_IPB0;
	    else if ((dbgHwRegStatus & (1 << REG_NUM_IPB1)) == 0)
		regNum = REG_NUM_IPB1;
	    else
		regNum = -1;
	    break;
	default:
	    regNum = -1;
	}
    if (regNum < 0)
	return ((HWBP *) NULL);
    else
	{
	dbgHwRegStatus |= 1 << regNum;
	pHwBp = (HWBP *) lstGet (pDbgHwBpList);
	if (pHwBp != NULL)
	    pHwBp->hbRegNum = regNum;
	}
    return (pHwBp);
    }
#endif 	/* BRK_HW_BP */

/* interrupt level stuff */

/******************************************************************************
*
* dbgLocalRegsToRegSet - save local registers to given register set
*
* NOMANUAL
*/

void dbgLocalRegsToRegSet
    (
    int *	pRegs,			/* saved local registers */
    REG_SET *	pRegSet			/* pointer to register set to fill */
    )
    {
    bcopy ((char *) pRegs, (char *) &pRegSet->pfp, NUM_I960_LOCAL_REGS * 4);
    }
/******************************************************************************
*
* dbgGlobalRegsToRegSet - save local registers to given register set
*
* NOMANUAL
*/

void dbgGlobalRegsToRegSet
    (
    FAST int *		pRegs,			/* saved local registers */
    FAST REG_SET *	pRegSet			/* REG_SET to fill */
    )
    {
    bcopy ((char *) pRegs, (char *) &pRegSet->g0, NUM_I960_GLOBAL_REGS * 4);
    }
/******************************************************************************
*
* dbgRegSetToLocalRegs - move local registers to memory
*
* NOMANUAL
*/

void dbgRegSetToLocalRegs
    (
    FAST int *		pRegs,		/* location to put regs */
    FAST REG_SET *	pRegSet		/* pointer to reg set to copy */
    )
    {
    bcopy ((char *) &pRegSet->pfp, (char *) pRegs, NUM_I960_LOCAL_REGS * 4);
    }
/******************************************************************************
*
* dbgRegSetToGlobalRegs - move global registers to memory
*
* NOMANUAL
*/

void dbgRegSetToGlobalRegs
    (
    FAST int *		pRegs,		/* location to put regs */
    FAST REG_SET *	pRegSet		/* pointer reg set to copy */
    )
    {
    bcopy ((char *) &pRegSet->g0, (char *) pRegs, NUM_I960_GLOBAL_REGS * 4);
    }
/******************************************************************************
*
* dbgRegSetFramePtrAdjust - adjust frame pointer
*
* Change the frame pointer in the given REG_SET to the given value.
* Used to 'chain back' so the saved REG_SET reflects the exact values
* of the task environment at the time of the trace fault.
*
* NOMANUAL
*/

void dbgRegSetFramePtrAdjust
    (
    REG_SET *	pRegSet,		/* register set to modify */
    UINT32 *	pFrame			/* value to set REG_SET FP */
    )
    {
    pRegSet->fp = (UINT32)pFrame;
    }
/******************************************************************************
*
* dbgTraceFaultAck - clear trace fault pending bit according to fault subtype
*/

LOCAL void dbgTraceFaultAck
    (
    UINT32	subType			/* trace fault subtype */
    )
    {
    vxTCWClear (subType << 0x10);
    }
/******************************************************************************
*
* dbgTraceFaultHandle - interrupt level handling of trace fault
*
* Function to determine what type of trace fault has occured.
* Trace fault subtypes are; breakpoint, instruction (ss), return,
* prereturn, call, branch, supervisor.
*
* NOMANUAL
*/

INSTR * dbgTraceFaultHandle
    (
    TRACE_ESF *	pBrkInfo,		/* trace fault exception frame */
    int *	pLocalRegs,		/* local registers */
    int *	pGlobalRegs		/* global registers */
    )
    {
    REG_SET	dbgRegSet;
    WIND_TCB *	pTcb = taskTcb (taskIdSelf ());

    /* save local and global regs to temporary REG_SET */

    dbgLocalRegsToRegSet (pLocalRegs, &dbgRegSet);
    dbgGlobalRegsToRegSet (pGlobalRegs, &dbgRegSet);
    dbgRegSetFramePtrAdjust ((REG_SET *) &dbgRegSet, (UINT32 *) pLocalRegs);

    switch (pBrkInfo->faultSubtype)
	{
	case TRACE_FAULT_SUBTYPE_INSTR:
	    dbgTraceFaultAck (TRACE_FAULT_SUBTYPE_INSTR);
	    _dbgSStepClear ();	/* make sure single stepping is off */
	    _dbgTaskSStepClear (taskIdSelf ());
	    dbgRegSet.pcw = pTcb->regs.pcw;
	    dbgRegSet.acw = pBrkInfo->arithCtrl;
	    dbgRegSet.tcw = vxTCWGet ();

	    dbgTrace (pBrkInfo, &dbgRegSet);

	    /* we come here when we single-stepped from a breakpoint. */

	    dbgRegSetToLocalRegs ((int *) pLocalRegs, (REG_SET *) &dbgRegSet);
	    dbgRegSetToGlobalRegs ((int *) pGlobalRegs, (REG_SET *) &dbgRegSet);
	    taskRegsGet (taskIdSelf (), &dbgRegSet);
	    pBrkInfo->procCtrl = dbgRegSet.pcw;
	    return ((INSTR *)(((REG_SET *)pLocalRegs)->rip & 0xfffffffc));
	    break;

	case TRACE_FAULT_SUBTYPE_BP:
	    dbgTraceFaultAck (TRACE_FAULT_SUBTYPE_BP);
	    dbgRegSet.pcw = pBrkInfo->procCtrl;
	    dbgRegSet.acw = pBrkInfo->arithCtrl;
	    dbgRegSet.tcw = vxTCWGet ();

	    /* if we hit a breakpoint, we need to set the rip back to
	     * the address of the fault */

	    dbgRegSet.rip = ((UINT32) _dbgInfoPCGet (pBrkInfo) & 0xfffffffc)
			    | (dbgRegSet.rip & 0x03);
	    dbgBreakpoint (pBrkInfo, &dbgRegSet);

	    /* we come here ONLY if bp was hit at interrupt level */

	    dbgRegSetToLocalRegs ((int *) pLocalRegs, (REG_SET *) &dbgRegSet);
	    dbgRegSetToGlobalRegs ((int *) pGlobalRegs, (REG_SET *) &dbgRegSet);
	    return ((INSTR *)(((REG_SET *)pLocalRegs)->rip & 0xfffffffc));
	    break;

	case TRACE_FAULT_SUBTYPE_BRANCH:
	    /* not implemented yet */

	case TRACE_FAULT_SUBTYPE_CALL:
	    /* not implemented yet */

	case TRACE_FAULT_SUBTYPE_RETURN:
	    /* not implemented yet */

	case TRACE_FAULT_SUBTYPE_PRERET:
	    /* not implemented yet */

	case TRACE_FAULT_SUBTYPE_SPV:
	    /* not implemented yet */

	default:
	    logMsg ("faultSubtype: 0x%x\n", pBrkInfo->faultSubtype, 0, 0, 0, 0, 0);
	    break;
	}
    return ((INSTR *) -1);
    }

/*******************************************************************************
*
* getOneReg - return the contents of one register
*
* Given a task's ID, this routine returns the contents of the register
* specified by the register code.  This routine is used by `a0', `d0', `sr', etc.
* The register codes are defined in regsI960.h.
*
* RETURNS: register contents, or ERROR.
*
* NOMANUAL
*/

static int getOneReg
    (
    int taskId,		/* task ID, 0 means default task */
    int regOffset	/* code for specifying register */
    )
    {
    REG_SET regSet;	/* get task's regs into here */

    taskId = taskIdFigure (taskId);	/* translate super name to ID */

    if (taskId == ERROR)		/* couldn't figure out super name */
	return (ERROR);
    taskId = taskIdDefault (taskId);	/* set the default ID */

    if (taskRegsGet (taskId, &regSet) != OK)
	return (ERROR);

    return (*(int *)((int)&regSet + regOffset));
    }

#if CPU==I960KB
/*******************************************************************************
*
* getOneFPReg - return the contents of an FP register
*
* Given a task's ID, this routine returns the contents of the FP register
* specified by the register code.  This routine is used by fp[0-3]. The 
* register codes are defined in fppI960Lib.h.
*
* RETURNS: register contents, or ERROR.
*
* NOMANUAL
*/

static double getOneFPReg
    (
    volatile int taskId,               /* task's ID, 0 means default task */
    int regOffset
    )
    {
    FPREG_SET fpregs;      /* floating point data registers */

    taskId = taskIdFigure (taskId);     /* translate super name to ID */

    if (taskId == ERROR)                /* couldn't figure out super name */
	return ((double) ERROR);

    taskId = taskIdDefault (taskId);    /* set the default ID */

    if (fppTaskRegsGet (taskId, (FPREG_SET *) &fpregs) != OK)
        {
        int status[] = { ERROR, ERROR };
        return (*((double *) status));
        }

    return  (*(double *) ((int)&fpregs + regOffset));
    }
#endif /* CPU == I960KB */
/*******************************************************************************
*
* pfp - return the contents of register `pfp' (i960)
*
* This command extracts the contents of register `pfp', the previous frame
* pointer, from the TCB of a specified task.
* If <taskId> is omitted or 0, the current default task is assumed.
*
* RETURNS: The contents of the `pfp' register.
*
* SEE ALSO:
* .pG "Target Shell"
*
* NOMANUAL
*/

int pfp
    (
    int taskId		/* task ID, 0 means default task */
    )

    {
    return (getOneReg (taskId, PFP_OFFSET));
    }


/*******************************************************************************
*
* tsp - return the contents of register `sp' (i960)
*
* This command extracts the contents of register `sp', the stack pointer,
* from the TCB of a specified task.
* If <taskId> is omitted or 0, the current default task is assumed.
*
* Note:  The name tsp() is used because sp() (the logical name choice)
* conflicts with the routine sp() for spawning a task with default parameters.
*
* RETURNS: The contents of the `sp' register.
*
* SEE ALSO:
* .pG "Target Shell"
*
* NOMANUAL
*/

int tsp
    (
    int taskId		/* task ID, 0 means default task */
    )
    
    {
    return (getOneReg (taskId, SP_OFFSET));
    }

/*******************************************************************************
*
* rip - return the contents of register `rip' (i960)
*
* This command extracts the contents of register `rip', the return
* instruction pointer, from the TCB of a specified task.
* If <taskId> is omitted or 0, the current default task is assumed.
*
* RETURNS: The contents of the `rip' register.
*
* SEE ALSO:
* .pG "Target Shell"
*
* NOMANUAL
*/

int rip
    (
    int taskId		/* task ID, 0 means default task */
    )
    
    {
    return (getOneReg (taskId, RIP_OFFSET));
    }

/*******************************************************************************
*
* r3 - return the contents of register `r3' (also `r4' - `r15') (i960)
*
* This command extracts the contents of register `r3' from the TCB of a
* specified task.
* If <taskId> is omitted or 0, the current default task is assumed.
*
* Routines are provided for all local registers (`r3' - `r15'):
* r3() - r15().
*
* RETURNS: The contents of the `r3' register (or the requested register).
*
* SEE ALSO:
* .pG "Target Shell"
*
* NOMANUAL
*/

int r3
    (
    int taskId		/* task ID, 0 means default task */
    )
    
    {
    return (getOneReg (taskId, R_REG_OFFSET(3))); 
    }

int r4  (int taskId) { return (getOneReg (taskId, R_REG_OFFSET(4))); }
int r5  (int taskId) { return (getOneReg (taskId, R_REG_OFFSET(5))); }
int r6  (int taskId) { return (getOneReg (taskId, R_REG_OFFSET(6))); }
int r7  (int taskId) { return (getOneReg (taskId, R_REG_OFFSET(7))); }
int r8  (int taskId) { return (getOneReg (taskId, R_REG_OFFSET(8))); }
int r9  (int taskId) { return (getOneReg (taskId, R_REG_OFFSET(9))); }
int r10 (int taskId) { return (getOneReg (taskId, R_REG_OFFSET(10))); }
int r11 (int taskId) { return (getOneReg (taskId, R_REG_OFFSET(11))); }
int r12 (int taskId) { return (getOneReg (taskId, R_REG_OFFSET(12))); }
int r13 (int taskId) { return (getOneReg (taskId, R_REG_OFFSET(13))); }
int r14 (int taskId) { return (getOneReg (taskId, R_REG_OFFSET(14))); }
int r15 (int taskId) { return (getOneReg (taskId, R_REG_OFFSET(15))); }

/*******************************************************************************
*
* g0 - return the contents of register `g0' (also `g1' - `g14') (i960)
*
* This command extracts the contents of register `g0' from the TCB of a
* specified task.
* If <taskId> is omitted or 0, the current default task is assumed.
*
* Routines are provided for all global registers (`g0' - `g14'):
* g0() - g14().
*
* RETURNS: The contents of the `g0' register (or the requested register).
*
* SEE ALSO:
* .pG "Target Shell"
*
* INTERNAL
* Although this routine is hereby marked NOMANUAL, it actually gets
* published from arch/doc/dbgArchLib.c.
* ...not any more -- i960 no longer supported.
*
* NOMANUAL
*/

int g0
    (
    int taskId		/* task ID, 0 means default task */
    )
    
    {
    return (getOneReg (taskId, G_REG_OFFSET(0))); 
    }

int g1  (int taskId) { return (getOneReg (taskId, G_REG_OFFSET(1))); }
int g2  (int taskId) { return (getOneReg (taskId, G_REG_OFFSET(2))); }
int g3  (int taskId) { return (getOneReg (taskId, G_REG_OFFSET(3))); }
int g4  (int taskId) { return (getOneReg (taskId, G_REG_OFFSET(4))); }
int g5  (int taskId) { return (getOneReg (taskId, G_REG_OFFSET(5))); }
int g6  (int taskId) { return (getOneReg (taskId, G_REG_OFFSET(6))); }
int g7  (int taskId) { return (getOneReg (taskId, G_REG_OFFSET(7))); }
int g8  (int taskId) { return (getOneReg (taskId, G_REG_OFFSET(8))); }
int g9  (int taskId) { return (getOneReg (taskId, G_REG_OFFSET(9))); }
int g10 (int taskId) { return (getOneReg (taskId, G_REG_OFFSET(10))); }
int g11 (int taskId) { return (getOneReg (taskId, G_REG_OFFSET(11))); }
int g12 (int taskId) { return (getOneReg (taskId, G_REG_OFFSET(12))); }
int g13 (int taskId) { return (getOneReg (taskId, G_REG_OFFSET(13))); }
int g14 (int taskId) { return (getOneReg (taskId, G_REG_OFFSET(14))); }

/*******************************************************************************
*
* fp - return the contents of register `fp' (i960)
*
* This command extracts the contents of register `fp', the frame pointer,
* from the TCB of a specified task.
* If <taskId> is omitted or 0, the current default task is assumed.
*
* RETURNS: The contents of the `fp' register.
*
* SEE ALSO:
* .pG "Target Shell"
*
* NOMANUAL
*/

int fp
    (
    int taskId		/* task ID, 0 means default task */
    )
    
    {
    return (getOneReg (taskId, FP_OFFSET)); 
    }

#if CPU==I960KB
/*******************************************************************************
*
* fp0 - return the contents of register `fp0' (also `fp1' - `fp3') (i960KB, i960SB)
*
* This command extracts the contents of the floating-point register `fp0' from
* the TCB of a specified task.
* If <taskId> is omitted or 0, the current default task is assumed.
*
* Routines are provided for the floating-point registers `fp0' - `fp3':
* fp0() - fp3().
*
* RETURNS: The contents of the `fp0' register (or the requested register).
*
* SEE ALSO:
* .pG "Target Shell"
*
* NOMANUAL
*/

double fp0
    (
    volatile int taskId		/* task ID, 0 means default task */
    ) 
    
    { 
    return (getOneFPReg (taskId, FPX_0)); 
    }

double fp1 (volatile int taskId) { return (getOneFPReg (taskId, FPX_1)); }
double fp2 (volatile int taskId) { return (getOneFPReg (taskId, FPX_2)); }
double fp3 (volatile int taskId) { return (getOneFPReg (taskId, FPX_3)); }
#endif /* CPU==I960KB */

/*******************************************************************************
*
* pcw - return the contents of the `pcw' register (i960)
*
* This command extracts the contents of the `pcw' register from the TCB of a
* specified task.
* If <taskId> is omitted or 0, the current default task is assumed.
*
* RETURNS: The contents of the `pcw' register.
*
* SEE ALSO:
* .pG "Target Shell"
*
* NOMANUAL
*/

int pcw
    (
    int taskId 			/* task ID, 0 means default task */
    )

    {
    return (getOneReg (taskId, PCW_OFFSET));
    }

/*******************************************************************************
*
* tcw - return the contents of the `tcw' register (i960)
*
* This command extracts the contents of the `tcw' register from the TCB of a
* specified task.
* If <taskId> is omitted or 0, the current default task is assumed.
*
* RETURNS: The contents of the `tcw' register.
*
* SEE ALSO:
* .pG "Target Shell"
*
* NOMANUAL
*/

int tcw
    (
    int taskId 			/* task ID, 0 means default task */
    )

    {
    return (getOneReg (taskId, TCW_OFFSET));
    }

/*******************************************************************************
*
* acw - return the contents of the `acw' register (i960)
*
* This command extracts the contents of the `acw' register from the TCB of a
* specified task.
* If <taskId> is omitted or 0, the current default task is assumed.
*
* RETURNS: The contents of the `acw' register.
*
* SEE ALSO:
* .pG "Target Shell"
*
* NOMANUAL
*/

int acw
    (
    int taskId			/* task ID, 0 means default task */
    )

    {
    return (getOneReg (taskId, ACW_OFFSET));
    }


/* dbgArchLib.c - MIPS architecture dependent debugger library */
  
/* Copyright 1984-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
 * This file has been developed or significantly modified by the
 * MIPS Center of Excellence Dedicated Engineering Staff.
 * This notice is as per the MIPS Center of Excellence Master Partner
 * Agreement, do not remove this notice without checking first with
 * WR/Platforms MIPS Center of Excellence engineering management.
 */

/*
modification history
--------------------
01r,16jul01,ros  add CofE comment
01q,20dec00,pes  Update for MIPS32/MIPS64 target combinations.
01p,22sep99,myz  added CW4000_16 support.
01o,29jul99,alp  added CW4000 and CW4010 support.
01n,18jan99,elg  Authorize breakpoints on branch delay slot (SPR 24356).
01m,08jan98,dbt  modified for new breakpoint scheme
01l,14oct96,kkk  added R4650 support.
01k,10feb95,jdi  doc tweaks.
01j,27jan95,rhp  doc cleanup.
01i,19oct93,cd   added R4000 support
01h,29sep93,caf  undid fix of SPR #2359.
01g,07jul93,yao  fixed to preserve parity error bit of status
		 register (SPR #2359).  changed copyright notice.
01f,01oct92,ajm  added dynamically bound handlers, general cleanup
01e,23aug92,jcf  made filename consistant.
01d,22jul92,yao  fixed bug when adding a temporary breakpoint at a branch 
		 instruction in _dbgStepAdd().
01c,06jul92,yao  removed dbgCacheClear().  made user uncallable globals
		 started with '_'.
01b,04jul92,jcf  scalable/ANSI/cleanup effort.
01a,16jun92,yao  written based on mips dbgLib.c ver01k.
*/

/*
DESCRIPTION
NOMANUAL
*/

#include "vxWorks.h"
#include "private/dbgLibP.h"
#include "private/taskLibP.h"
#include "taskArchLib.h"
#include "intLib.h"
#include "excLib.h"
#include "regs.h"
#include "iv.h"
#include "cacheLib.h"
#include "ioLib.h"
#include "dsmLib.h"
#include "vxLib.h"
#include "stdio.h"
#include "wdb/wdbDbgLib.h"
#include "dbgLib.h"

/* externals */

IMPORT int 	dsmInst (FAST long * binInst, int address, FUNCPTR prtAddress);
IMPORT FUNCPTR	wdbDbgArchHandler[8];
IMPORT int      dsmNbytes (ULONG);
IMPORT BOOL mips16Instructions(ULONG);

/* globals */

char * _archHelp_msg = 		/* help message */
#if     (DBG_HARDWARE_BP)
    "bh addr[,access[,task[,count[,quiet]]]] Set hardware breakpoint\n"
    "        access :      1 - write            2 - read\n"
    "                      3 - read/write"
    "        For R4650 processors:\n"
    "        access :      0 - instruction      1 - write\n"
    "                      2 - read             3 - read/write"
#endif	/* (DBG_HARDWARE_BP) */
    "\n";

/*******************************************************************************
*
* _dbgArchInit - architecture dependent initialization routine
*
* This routine initialize global function pointers that are specific for 
* MIPS architecture.
*
* RETURNS:N/A
* 
* NOMANUAL
*/

void _dbgArchInit (void)

    {
    _dbgDsmInstRtn = (FUNCPTR) dsmInst;
    }

/*******************************************************************************
*
* _dbgInstSizeGet - set up breakpoint instruction
*
* RETURNS: size of the instruction at specified location.
* 
* NOMANUAL
*/

int _dbgInstSizeGet
    (
    INSTR * brkInst		/* pointer to hold breakpoint instruction */
    )
    {
    return (2);
    }

/*******************************************************************************
*
* _dbgRetAdrsGet - get return address for current routine
*
* RETURNS: return address for current routine.
* 
* NOMANUAL
*/

INSTR * _dbgRetAdrsGet
    (
    REG_SET * pRegSet		/* pointer to register set */
    )
    {
#if	FALSE
    INSTR * scanpc;		/* instruction scan pc pointer */

    /*
    * scan instructions forward. If we find a "sw ra,x(sp)" or a "jr ra"
    * then the return address in already in register "ra".  If we find
    * a "lw ra,x(sp)" then the return address is saved in offset "x"
    * on the stack. If the instruction space is corrupted, could get
    * a bus error eventually or could find a return address for a
    * neighboring subprogram.
    */

    for (scanpc = pRegSet->pc; TRUE; scanpc++)	
	{
	/* match "sw ra,x(sp)" or "jr ra" means return address in ra */
	if (INST_CMP(scanpc,(SW_INSTR|RA<<RT_POS|SP<<BASE_POS),
		(GENERAL_OPCODE_MASK|RT_MASK|BASE_MASK)) ||
	    INST_CMP(scanpc,(SPECIAL|JR_INSTR|RA<<RS_POS),
		(GENERAL_OPCODE_MASK|SPECIAL_MASK|RS_MASK)))
	    {
	    return ((INSTR *) pRegSet->raReg);
	    }

	/* match "lw ra, x(sp)" means return address is on the stack */
	if (INST_CMP(scanpc,(LW_INSTR|RA<<RT_POS|SP<<BASE_POS),
		(GENERAL_OPCODE_MASK|RT_MASK|BASE_MASK)))
	    {
	    /* Note that the "C" compiler treats "short" as the lower
	     * 16 bits of the word and automatically performs the sign
	     * extend when the "short" is converted to a "long"
	     */

	    return ((INSTR *)(*(INSTR **) (pRegSet->spReg + (short) *scanpc)));
	    }
	}

    return (NULL);
#endif	/* FALSE */
    return ((INSTR *) ERROR);
    }

/*******************************************************************************
*
* _dbgFuncCallCheck - check next instruction
*
* This routine checks to see if the next instruction is a JAL or BAL.
* If it is, it returns TRUE, otherwise, returns FALSE.
*
* RETURNS: TRUE if next instruction is JAL or BAL, or FALSE otherwise.
* 
* NOMANUAL
*/

BOOL _dbgFuncCallCheck
    (
    INSTR * addr		/* pointer to instruction */
    )
    {
    if (mips16Instructions((ULONG)addr))
	{
	/* mips16 jal, jalr instructions */

        if ( (M16_INSTR_OPCODE(*(UINT16 *)addr) == M16_JALNX_INSTR)||
	     (((*(UINT16 *)addr) & 0xf81f) == 0xe800) )  /* j(al)r */
	     return (TRUE);
        else
	     return(FALSE);
        }

    return (INST_CMP (addr, JAL_INSTR, GENERAL_OPCODE_MASK) || 
#ifdef _WRS_MIPS16
#define JALX_INSTR  0x74000000
	    INST_CMP (addr, JALX_INSTR, GENERAL_OPCODE_MASK) ||
#endif
	    INST_CMP (addr, (SPECIAL|JALR_INSTR), 
		(GENERAL_OPCODE_MASK | SPECIAL_MASK)) ||
	    INST_CMP (addr, (BCOND|BLTZAL_INSTR), 
		(GENERAL_OPCODE_MASK | BCOND_MASK)) ||
	    INST_CMP (addr, (BCOND | BGEZAL_INSTR), 
		(GENERAL_OPCODE_MASK | BCOND_MASK)) ||
	    INST_CMP (addr, (BCOND | BLTZALL_INSTR), 
		(GENERAL_OPCODE_MASK | BCOND_MASK)) ||
	    INST_CMP (addr, (BCOND | BGEZALL_INSTR), 
		(GENERAL_OPCODE_MASK | BCOND_MASK))
	    );
    }

/*******************************************************************************
*
* _dbgTaskPCGet - get task's pc
*
* RETURNS:task's program counter
* 
* NOMANUAL
*/

INSTR * _dbgTaskPCGet
    (
    int tid	/* task's id */
    )
    {
    REG_SET	regSet;

    (void) taskRegsGet (tid, &regSet);

#ifdef _WRS_MIPS16

    /* mask off possible mips16 function indicator */

    return((INSTR *)((int)(regSet.pc) & ~0x1));
#else
    return ((INSTR *) regSet.pc);
#endif
    }

/*******************************************************************************
*
* _dbgTaskPCSet - set task's pc
*
* RETURNS:N/A
*
* NOMANUAL
*/

void _dbgTaskPCSet
    (
    int		tid,	/* task id */
    INSTR *	pc,	/* task's pc */
    INSTR *	npc	/* task's npc */
    )
    {
    REG_SET regSet;	/* task's register set */

    if (taskRegsGet (tid, &regSet) != OK)
	return;

    regSet.pc = pc;

    taskRegsSet (tid, &regSet);
    }

/*******************************************************************************
*
* dbgBpTypeBind - bind a breakpoint handler to a breakpoint type (MIPS R3000, R4000, R4650)
* 
* Dynamically bind a breakpoint handler to breakpoints of type 0 - 7.
* By default only breakpoints of type zero are handled with the
* vxWorks breakpoint handler (see dbgLib).  Other types may be used for
* Ada stack overflow or other such functions.  The installed handler
* must take the same parameters as excExcHandle() (see excLib).
*
* RETURNS:
* OK, or
* ERROR if <bpType> is out of bounds.
* 
* SEE ALSO
* dbgLib, excLib
*/

STATUS dbgBpTypeBind
    (
    int		bpType,		/* breakpoint type */
    FUNCPTR	routine		/* function to bind */
    )
    {
    if ((bpType > 7) || (bpType < 0))
	{
	return (ERROR);
	}
    else
	{
	wdbDbgArchHandler[bpType] = routine;
	return (OK);
	}
    }

#if	(DBG_HARDWARE_BP)
/******************************************************************************
*
* _dbgBrkDisplayHard - print hardware breakpoint
*
* This routine print hardware breakpoint.
*
* NOMANUAL
*/

void _dbgBrkDisplayHard
    (
    BRKPT *	pBp		/* breakpoint table entry */
    )
    {
    int type;

    if ((pBp->bp_flags & BRK_HARDWARE) == 0)
	return;

    type = pBp->bp_flags & BRK_HARDMASK;

    printf (" (hard-");

    switch (type)
	{
	case BRK_INST:
	    printf ("inst.)");
	    break;

	case BRK_READ:
	    printf ("data read)");
	    break;

	case BRK_WRITE:
	    printf ("data write)");
	    break;

	case BRK_RW:
	    printf ("data r/w)");
	    break;

	default:
	    printf ("unknown)");
	    break;
	}
    }
#endif  /* DBG_HARDWARE_BP */

/* dbgArchLib.c - i80x86 architecture-specific debugging facilities */

/* Copyright 1984-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01h,20nov01,hdn  doc clean up for 5.5.  revived edi() - eflags().
01g,08jan98,dbt  modified for new breakpoint scheme
01f,10feb95,jdi  doc tweak for 5.2.
01e,14dec93,hdn  added _archHelp_msg.
01d,29nov93,hdn  added eax() - eflags().
01c,27aug93,hdn  added _dbgTaskPCSet().
01b,16jun93,hdn  updated to 5.1.
		  - changed functions to ansi style
		  - changed VOID to void
		  - changed copyright notice
01a,08jul92,hdn  written based on tron/dbgLib.c.
*/

/*
DESCRIPTION
This module provides the architecture dependent support functions for
dbgLib. 
x86 including P5(Pentium), P6(PentiumPro, II, III), and P7(Pentium4) family
processors have four breakpoint registers and the following types of 
hardware breakpoint:
.CS
   BRK_INST             /@ instruction hardware breakpoint @/
   BRK_DATAW1           /@ data write 1 byte breakpoint @/
   BRK_DATAW2           /@ data write 2 byte breakpoint @/
   BRK_DATAW4           /@ data write 4 byte breakpoint @/
   BRK_DATARW1          /@ data read-write 1 byte breakpoint @/
   BRK_DATARW2          /@ data read-write 2 byte breakpoint @/
   BRK_DATARW4          /@ data read-write 4 byte breakpoint @/
.CE

NOMANUAL
*/

/* LINTLIBRARY */

#include "vxWorks.h"
#include "taskLib.h"
#include "taskArchLib.h"
#include "intLib.h"
#include "regs.h"
#include "iv.h"
#include "cacheLib.h"
#include "ioLib.h"
#include "dsmLib.h"
#include "vxLib.h"
#include "usrLib.h"
#include "stdio.h"
#include "dbgLib.h"


/* defines */

#define DSM(addr,inst,mask)     ((*(addr) & (mask)) == (inst))


/* externs */

IMPORT int 	dsmInst ();


/* globals */

char * _archHelp_msg = 
#ifdef  DBG_HARDWARE_BP
    "bh addr[,access[,task[,count[,quiet]]]] Set hardware breakpoint\n"
    "         access :      0 - instruction        1 - write 1 byte\n"
    "                       3 - read/write 1 byte  5 - write 2 bytes\n"
    "                       7 - read/write 2 bytes d - write 4 bytes\n"
    "                       f - read/write 4 bytes"
#endif	/* DBG_HARDWARE_BP */
    "\n";


/* forward declarations */

LOCAL int	getOneReg (int taskId, int regCode);


/*******************************************************************************
*
* _dbgArchInit - architecture dependent initialization routine
*
* This routine initialize global function pointers that are architecture 
* specific.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void _dbgArchInit (void)

    {
    _dbgDsmInstRtn = (FUNCPTR) dsmInst;
    }

/*******************************************************************************
*
* _dbgRetAdrsGet - get a next instruction for cret ()
*
* if next instruction is a ENTER or RET, return address is on top of stack.
* otherwise it follows saved frame pointer.
*
*
* NOMANUAL
*/

INSTR * _dbgRetAdrsGet
    (
    REG_SET *	pRegSet		/* register set */
    )
    {
    INSTR *returnAddress;

    if (DSM(pRegSet->pc,   PUSH_EBP, PUSH_EBP_MASK) && 
	DSM(pRegSet->pc+1, MOV_ESP0, MOV_ESP0_MASK) &&
	DSM(pRegSet->pc+2, MOV_ESP1, MOV_ESP1_MASK))
	{
	returnAddress = *(INSTR **)pRegSet->spReg;
	}

    else if (DSM(pRegSet->pc-1, PUSH_EBP, PUSH_EBP_MASK) && 
	     DSM(pRegSet->pc,   MOV_ESP0, MOV_ESP0_MASK) &&
	     DSM(pRegSet->pc+1, MOV_ESP1, MOV_ESP1_MASK))
	{
	returnAddress = *((INSTR **)pRegSet->spReg + 1);
	}

    else if (DSM(pRegSet->pc, ENTER, ENTER_MASK))
	{
	returnAddress = *(INSTR **)pRegSet->spReg;
	}

    else if ((DSM(pRegSet->pc, RET,    RET_MASK)) ||
	     (DSM(pRegSet->pc, RETADD, RETADD_MASK)))
	{
	returnAddress = *(INSTR **)pRegSet->spReg;
	}

    else
	{
	returnAddress = *((INSTR **)pRegSet->fpReg + 1);
	}

    return (returnAddress);
    }

/*******************************************************************************
*
* _dbgFuncCallCheck - check next instruction
*
* This routine checks to see if the next instruction is a CALL
* If it is, it returns TRUE, otherwise, returns FALSE.
*
* RETURNS: TRUE if next instruction is a CALL, or FALSE otherwise.
*
* NOMANUAL
*/

BOOL _dbgFuncCallCheck
    (
    INSTR * addr			/* pointer to instruction */
    )
    {
    return ((DSM (addr,		CALL_INDIR0,	CALL_INDIR0_MASK) &&
	     DSM (addr + 1,	CALL_INDIR1,	CALL_INDIR1_MASK)) || 
	    (DSM (addr,		CALL_DIR,	CALL_DIR_MASK)));
    }

/*******************************************************************************
*
* _dbgInstSizeGet - set up the breakpoint instruction
*
* RETURNS: size of the instruction at specified location.
*
* NOMANUAL
*/

int _dbgInstSizeGet
    (
    INSTR * pBrkInst		/* pointer to hold breakpoint instruction */
    )
    {
    return (dsmNbytes (pBrkInst));
    }

/*******************************************************************************
*
* _dbgTaskPCGet - get task's program counter PC
*
* RETURNS:task's program counter
*
* NOMANUAL
*/

INSTR * _dbgTaskPCGet
    (
    int tid	/* task's id */
    )
    {
    REG_SET	regSet;

    (void) taskRegsGet (tid, &regSet);
    return ((INSTR *) regSet.pc);
    }

/*******************************************************************************
*
* _dbgTaskPCSet - set task's program counter PC
*
* RETURNS: N/A
*
* NOMANUAL
*/

void _dbgTaskPCSet
    (
    int		task,		/* task id */
    INSTR *	pc,		/* new PC */
    INSTR *	npc		/* not supported on I80X86 */
    )
    {
    REG_SET regSet;

    if (taskRegsGet (task, &regSet) != OK)
        return;

    regSet.pc = pc;
    (void)taskRegsSet (task, &regSet);
    }

#ifdef	DBG_HARDWARE_BP
/*******************************************************************************
*
* _dbgBrkDisplayHard - display a hardware breakpoint
*
* This routine displays a hardware breakpoint.
*
* NOMANUAL
*/

void _dbgBrkDisplayHard
    (
    BRKPT *	pBp	/* breakpoint table entry */
    )
    {
    int type;

    if ((pBp->bp_flags & BRK_HARDWARE) == 0)
	return;

    type = pBp->bp_flags & BRK_HARDMASK;

    printf (" (hard-");

    switch (type)
	{
	case BRK_INST:
	    printf ("inst)");
	    break;

	case BRK_DATAW1:
	    printf ("dataw1)");
		break;

	case BRK_DATAW2:
	    printf ("dataw2)");
	    break;

	case BRK_DATAW4:
	    printf ("dataw4)");
	    break;

	case BRK_DATARW1:
	    printf ("datarw1)");
	    break;

	case BRK_DATARW2:
	    printf ("datarw2)");
	    break;

	case BRK_DATARW4:
	    printf ("datarw4)");
	    break;

	default:
	    printf ("unknown)");
	    break;
	}
    }
#endif	/* DBG_HARDWARE_BP */

/*******************************************************************************
*
* getOneReg - return the contents of one register
*
* Given a task's ID, this routine returns the contents of the register
* specified by the register code.  This routine is used by eax, edx, etc.
* The register codes are defined in dbgI86Lib.h.
*
* RETURNS: register contents, or ERROR.
*/

LOCAL int getOneReg (taskId, regCode)
    int		taskId;		/* task's id, 0 means default task */
    int		regCode;	/* code for specifying register */

    {
    REG_SET	regSet;		/* get task's regs into here */

    taskId = taskIdFigure (taskId);	/* translate super name to id */

    if (taskId == ERROR)		/* couldn't figure out super name */
	return (ERROR);
    taskId = taskIdDefault (taskId);	/* set the default id */

    if (taskRegsGet (taskId, &regSet) != OK)
	return (ERROR);

    switch (regCode)
	{
	case EDI: return (regSet.edi);
	case ESI: return (regSet.esi);
	case EBP: return (regSet.ebp);
	case ESP: return (regSet.esp);
	case EBX: return (regSet.ebx);
	case EDX: return (regSet.edx);
	case ECX: return (regSet.ecx);
	case EAX: return (regSet.eax);

	case EFLAGS: return (regSet.eflags);
	}

    return (ERROR);		/* unknown regCode */
    }

/*******************************************************************************
*
* edi - return the contents of register `edi' (also `esi' - `eax') (x86)
*
* This command extracts the contents of register `edi' from the TCB of a
* specified task.  If <taskId> is omitted or zero, the last task
* referenced is assumed.
*
* Similar routines are provided for all general registers (`edi' - `eax'):
* edi() - eax().
*
* The stack pointer is accessed via eax().
*
* RETURNS: The contents of register `edi' (or the requested register).
*
* SEE ALSO:
* .pG "Debugging"
*/

int edi
    (
    int taskId		/* task ID, 0 means default task */
    )

    {
    return (getOneReg (taskId, EDI));
    }

int esi (taskId) int taskId; { return (getOneReg (taskId, ESI)); }
int ebp (taskId) int taskId; { return (getOneReg (taskId, EBP)); }
int esp (taskId) int taskId; { return (getOneReg (taskId, ESP)); }
int ebx (taskId) int taskId; { return (getOneReg (taskId, EBX)); }
int edx (taskId) int taskId; { return (getOneReg (taskId, EDX)); }
int ecx (taskId) int taskId; { return (getOneReg (taskId, ECX)); }
int eax (taskId) int taskId; { return (getOneReg (taskId, EAX)); }

/*******************************************************************************
*
* eflags - return the contents of the status register (x86)
*
* This command extracts the contents of the status register from the TCB of a
* specified task.  If <taskId> is omitted or zero, the last task referenced is
* assumed.
*
* RETURNS: The contents of the status register.
*
* SEE ALSO:
* .pG "Debugging"
*/

int eflags
    (
    int taskId		/* task ID, 0 means default task */
    )

    {
    return (getOneReg (taskId, EFLAGS));
    }

/* dbgArchLib.c - ARM-dependent debugger library */

/* Copyright 1996-1997 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01b,20feb97,jpd  Tidied comments/documentation.
01a,18jul96,jpd  written, based on 680x0 version 01h.
*/

/*
DESCRIPTION
This module provides the Advanced Risc Machines Ltd, ARM-specific support
functions for dbgLib. Note that no support is provided here (yet) for Thumb
state code or for the EmbeddedICE hardware debugging facilities.

NOMANUAL
*/

#include "vxWorks.h"
#include "private/dbgLibP.h"
#include "taskLib.h"
#include "regs.h"
#include "dsmLib.h"
#include "usrLib.h"
#include "arch/arm/arm.h"
#include "stdio.h"
#include "string.h"

/* externals */

/* architecture-independent breakpoint handling routine from dbgLib.c */

IMPORT STATUS dbgBreakpoint (BREAK_ESF *pInfo, int * Regs);


/* architecture-depdendent instruction decoding routines from dbgArmLib.c */

IMPORT INSTR * armGetNpc (INSTR, REG_SET *);
IMPORT BOOL armInstrChangesPc (INSTR *);


/* globals */

extern char * _archHelp_msg;
char * _archHelp_msg =
    "r0-r14    [task]                Display a register of a task\n"
    "cpsr      [task]                Display cpsr of a task\n"
    "psrShow   value                 Display meaning of psr value\n";


/* locals */


/* forward declarations */

LOCAL void armBreakpoint(ESF *pEsf, REG_SET * pRegs);


/* pseudo-register num to pass to getOneReg() to get CPSR, local to this file */

#define ARM_REG_CPSR	16


/*******************************************************************************
*
* _dbgArchInit - architecture dependent initialization routine
*
* This routine initialises global function pointers that are architecture
* specific.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void _dbgArchInit (void)

    {
    /* Install the ARM disassembler from dsmLib.c */

    _dbgDsmInstRtn = dsmInst;


    /*
     * The ARM undefined instruction exception handler will check that the
     * undefined instruction is the breakpoint instruction and pass control
     * to an installed breakpoint handler via a function pointer. So, install
     * our handler.
     */

    _func_excBreakpoint = armBreakpoint;

    }

/*******************************************************************************
*
* _dbgVecInit - insert new breakpoint and trace vectors
*
* NOTE
* Does nothing, since neither hardware breakpoints nor trace mode are
* supported on the ARM and the breakpoint handler has been installed in
* _dbgArchInit() above.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void _dbgVecInit (void)
    {
    }

/*******************************************************************************
*
* _dbgInstSizeGet - get size of breakpoint instruction
*
* NOTE
* In contrast to the Architecture Porting Guidelines, this routine should not
* return the size in units of 16-bit words. It should return the size in units
* of sizeof(INSTR). The only place this routine is called from, is in so(), in
* dbgLib.c which uses this to add a breakpoint at:
* (INSTR *)(pc + _dbgInstSizeGet(pc).
*
* RETURNS: size of the instruction at specified location.
*
* NOMANUAL
*/

int _dbgInstSizeGet
    (
    INSTR * pBrkInst		/* pointer to hold breakpoint instruction */
    )

    {
    return dsmNbytes (pBrkInst) / sizeof(INSTR);
    }

/*******************************************************************************
*
* _dbgRetAdrsGet - get return address for current routine
*
* This routine is used by the cret() routine to set a breakpoint at the return
* address of the current subroutine.
*
* NOTE
* In order to find the return address, a number of assumptions are made.
* In general, it will work for all C language routines and for assembly
* language routines that start with a standard entry sequence i.e.
*    MOV   ip,sp
*    STMDB sp!,{..fp,ip,lr,pc}
*    SUB   fp,ip,#4
*
* This will need extending for Thumb.
*
* Most VxWorks assembly language routines establish a stack frame in this
* fashion for exactly this reason. However, routines written in other
* languages, strange entries into routines, or tasks with corrupted stacks
* can confuse this routine
*
* RETURNS: return address for current routine.
*
* NOMANUAL
*/

INSTR * _dbgRetAdrsGet
    (
    REG_SET * pRegSet		/* pointer to register set */
    )
    {
    int		i;			/* an index */
    INSTR	instr;			/* next instruction */
    FAST INSTR	*pc = pRegSet->pc;	/* pointer to instruction */

    /*
     * If the current routine doesn't have a stack frame, then we will have
     * to guess that the return address is in the link register.
     * We KNOW we don't have a stack frame in a few restricted but useful
     * cases:
     *  1) we are in the entry sequence of a routine which establishes the
     *     stack frame. We try to cope with this.
     *  2) we are in a routine which doesn't create a stack frame. We cannot
     *     do much about this.
     */

    instr = *pc;

    /*
     * look for the first instruction of the entry sequence which can be up
     * to two instructions before the current pc
     */

    for (i = 0; i >= -2 ; --i)
	if (INSTR_IS(pc[i], MOV_IP_SP))
	    break;

    /*
     * If either the frame pointer is 0 or we are in the entry sequence of the
     * routine, use lr.
     */

    if ((pRegSet->fpReg == 0) ||
        ((i >= -2) &&
		INSTR_IS(pc[i + 1], STMDB_SPP_FP_IP_LR_PC) &&
		INSTR_IS(pc[i + 2], SUB_FP_IP_4)))
	return (INSTR *)pRegSet->r[14];
    else
	return *(((INSTR **)(pRegSet->fpReg)) - 1);
    }

/*******************************************************************************
*
* _dbgSStepClear - clear single step mode
*
* RETURNS: N/A
*
* NOMANUAL
*
* We believe that this routine can be null on the ARM which has no Single-Step
* processor mode.
*
*/

void _dbgSStepClear (void)
    {
    }

/*******************************************************************************
*
* _dbgSStepSet - set single step mode
*
* RETURNS: N/A
*
* NOMANUAL
*
* I believe that this routine can be null on the ARM which has no Single-Step
* processor mode. The SPARC architecture, sets a temporary breakpoint using
* _dbgStepAdd(), but I believe that this is redundant, as such a temporary
* breakpoint has already been set by dbgBreakpoint() in dbgLib.c which calls
* this routine.
*
*/

void _dbgSStepSet
    (
    BREAK_ESF * pInfo		/* pointer to info saved on stack */
    )
    {
    }

/******************************************************************************
*
* _dbgTaskSStepSet - set single step mode of task
*
* NOTE
* Does nothing since trace mode are not supported and temporary breakpoint
* is added in c() or s().
*
* RETURNS:N/A
*
* NOMANUAL
*/

void _dbgTaskSStepSet
    (
    int tid		/* task's id */
    )
    {
    }

/******************************************************************************
*
* _dbgTaskBPModeSet - set breakpoint mode of task
*
* NOMANUAL
*
* It has been suggested by WRS that it may be necessary to lock interrupts in
* this routine on some architectures to protect dbgTaskSwitch().
*
*/

void _dbgTaskBPModeSet
    (
    int tid		/* task's id */
    )
    {
    }

/******************************************************************************
*
* _dbgTaskBPModeClear - clear breakpoint mode of task
*
* NOMANUAL
*
* It has been suggested by WRS that it may be necessary to unlock interrupts in
* this routine on some architectures.
*
*/

void _dbgTaskBPModeClear
    (
    int tid
    )
    {
    }

/*******************************************************************************
*
* _dbgFuncCallCheck - check if instruction calls a function
*
* This routine checks to see if the instruction calls a function.
* On the ARM, many classes of instruction could be used to do this. We check to
* see if the instruction is a BL, or if it changes the PC and the previous
* instruction is a MOV lr, pc instruction.
*
* This will need extending for Thumb.
*
* RETURNS: TRUE if next instruction calls a function, FALSE otherwise.
*
* NOMANUAL
*/

BOOL _dbgFuncCallCheck
    (
    INSTR * addr		/* pointer to instruction */
    )
    {
    return (INSTR_IS (*addr, BL) ||
	(INSTR_IS (*(addr - 1), MOVXX_LR_PC) &&
	armInstrChangesPc (addr)));
    }

/*******************************************************************************
*
* _dbgRegsAdjust - set register set
*
* Comments from Am29k version:
*
* This routine restores the task's registers. It uses the register set stored
* in the task's memory stack to update the task's regSet.
*
* INTERNAL
* This routine is required since the breakpoint/trace ISR never returns in
* the breakpoint/trace stub (except when the breakpoint is ignored). So,
* excExit() cannot generally be used to re-fill the task's TCB with the
* register set saved in the ESF.
* RETURNS: N/A
*
* NOMANUAL
*/

void _dbgRegsAdjust
    (
    FAST int   tid,		/* id of task that hit breakpoint */
    TRACE_ESF * pInfo,		/* pointer to esf info saved on stack */
    int *       pRegs,		/* pointer to buf containing saved regs */
    BOOL stepBreakFlag		/* TRUE if this was a trace exception */
				/* FALSE if this was a SO or CRET breakpoint */
    )
    {
    /*
     * In the ARM implementation, may reload the task regSet using the pointer
     * on this saved regSet.
     */

    taskRegsSet (tid, (REG_SET *) pRegs);

    }

/*******************************************************************************
*
* _dbgIntrInfoSave  - save information when breakpoints are hit at interrupt
*                     level
*
* RETURNS: N/A
*
* NOMANUAL
*
* On advice from philm, this pair of routines can be null on the ARM as there
* is no trace or step-specific bit to save.
*
*/

void _dbgIntrInfoSave
    (
    BREAK_ESF * pInfo		/* pointer to info saved on stack */
    )
    {
    }

/******************************************************************************
*
* _dbgIntrInfoRestore - restore the info saved by dbgIntrInfoSave
*
* NOMANUAL
*/

void _dbgIntrInfoRestore
    (
    TRACE_ESF *	pInfo		/* pointer to execption frame */
    )
    {
    }

/******************************************************************************
*
* _dbgInstPtrAlign - align pointer to appropriate boundary
*
* REUTRNS: align given instruction pointer to appropriate boundary
*
* NOMANUAL
*/

INSTR * _dbgInstPtrAlign
    (
    INSTR * addr		/* instruction pointer */
    )
    {
    addr = (INSTR *) ((int)addr & ~(0x03));	/* force address to a long
						 * word boundary.
						 */

   /* This will need extending for Thumb */

    return addr;

    }

/*******************************************************************************
*
* _dbgInfoPCGet - get pc
*
* RETURNS: value of pc saved on stack
*
* NOMANUAL
*/

INSTR * _dbgInfoPCGet
    (
    BREAK_ESF * pInfo		/* pointer to info saved on stack */
    )
    {
    return pInfo->pc;
    }

/*******************************************************************************
*
* _dbgTaskPCSet - set task's pc
*
* RETURNS: N/A
*
* NOMANUAL
*/

void _dbgTaskPCSet
    (
    int    tid,		/* task id */
    INSTR* pc,		/* task's pc */
    INSTR* npc		/* next pc, not supported on ARM */
    )
    {
    REG_SET regSet;		/* task's register set */

    if (taskRegsGet (tid, &regSet) != OK)
        return;

    regSet.pc = pc;

    taskRegsSet (tid, &regSet);

    }

/*******************************************************************************
*
* _dbgTaskPCGet - restore register set
*
* RETURNS: N/A
*
* NOMANUAL
*/

INSTR * _dbgTaskPCGet
    (
    int tid		/* task id */
    )
    {
    REG_SET regSet;		/* task's register set */

    taskRegsGet (tid, &regSet);

    return regSet.pc;

    }

/*******************************************************************************
*
* _dbgTraceDisable - disable trace mode
*
* NOMANUAL
*
* Can be a null routine on the ARM which has no trace mode.
*/

void _dbgTraceDisable (void)
    {
    }

/*******************************************************************************
*
* getOneReg - return the contents of one register
*
* Given a task's ID, this routine returns the contents of the register
* specified by the register code.  This routine is used by `a1', `cpsr', etc.
*
* RETURNS: register contents, or ERROR.
*/

LOCAL int getOneReg (taskId, regCode)
    int		taskId;		/* task's id, 0 means default task */
    int		regCode;	/* code for specifying register */
    {
    REG_SET	regSet;		/* get task's regs into here */

    taskId = taskIdFigure (taskId);	/* translate super name to id */

    if (taskId == ERROR)		/* couldn't figure out super name */
	return ERROR;
    taskId = taskIdDefault (taskId);	/* set the default id */

    if (taskRegsGet (taskId, &regSet) != OK)
	return ERROR;

    switch (regCode)
	{
	case 0:  return regSet.r[0];	/* general registers */
	case 1:  return regSet.r[1];
	case 2:  return regSet.r[2];
	case 3:  return regSet.r[3];
	case 4:  return regSet.r[4];
	case 5:  return regSet.r[5];
	case 6:  return regSet.r[6];
	case 7:  return regSet.r[7];
	case 8:  return regSet.r[8];
	case 9:  return regSet.r[9];
	case 10: return regSet.r[10];
	case 11: return regSet.r[11];
	case 12: return regSet.r[12];
	case 13: return regSet.r[13];
	case 14: return regSet.r[14];
	case 15: return (int) regSet.pc;

	case ARM_REG_CPSR: return regSet.cpsr;

	}

    return ERROR;		/* unknown regCode */

    }

/*******************************************************************************
*
* r0 - return the contents of register `r0' (also `r1' - `r14') (ARM)
*
* This command extracts the contents of register `r0' from the TCB of a
* specified task.  If <taskId> is omitted or zero, the last task referenced is
* assumed.
*
* Similar routines are provided for registers (`r1' - `r14'):
* r1() - r14().
*
* RETURNS: The contents of register `r0' (or the requested register).
*
* SEE ALSO:
* .pG "Debugging"
*/

int r0
    (
    int taskId		/* task ID, 0 means default task */
    )

    {
    return getOneReg (taskId, 0);
    }

int r1  (taskId) int taskId; { return getOneReg (taskId, 1); }
int r2  (taskId) int taskId; { return getOneReg (taskId, 2); }
int r3  (taskId) int taskId; { return getOneReg (taskId, 3); }
int r4  (taskId) int taskId; { return getOneReg (taskId, 4); }
int r5  (taskId) int taskId; { return getOneReg (taskId, 5); }
int r6  (taskId) int taskId; { return getOneReg (taskId, 6); }
int r7  (taskId) int taskId; { return getOneReg (taskId, 7); }
int r8  (taskId) int taskId; { return getOneReg (taskId, 8); }
int r9  (taskId) int taskId; { return getOneReg (taskId, 9); }
int r10 (taskId) int taskId; { return getOneReg (taskId, 10); }
int r11 (taskId) int taskId; { return getOneReg (taskId, 11); }
int r12 (taskId) int taskId; { return getOneReg (taskId, 12); }
int r13 (taskId) int taskId; { return getOneReg (taskId, 13); }
int r14 (taskId) int taskId; { return getOneReg (taskId, 14); }

/*******************************************************************************
*
* cpsr - return the contents of the current processor status register (ARM)
*
* This command extracts the contents of the status register from the TCB of a
* specified task.  If <taskId> is omitted or zero, the last task referenced is
* assumed.
*
* RETURNS: The contents of the current processor status register.
*
* SEE ALSO:
* .pG "Debugging"
*/

int cpsr
    (
    int taskId		/* task ID, 0 means default task */
    )
    {
    return getOneReg (taskId, ARM_REG_CPSR);
    }

/*******************************************************************************
*
* psrShow - display the meaning of a specified PSR value, symbolically (ARM)
*
* This routine displays the meaning of all fields in a specified PSR value,
* symbolically.
*
* RETURNS: OK, always.
*
*/

STATUS psrShow
    (
    UINT32 psrval		/* psr value to show */
    )
    {
    char str[16];		/* NZVCIFTSYSTEM32 */

    strcpy(str, "nzcvift");

    if (psrval & N_BIT)
	str[0] = 'N';

    if (psrval & Z_BIT)
	str[1] = 'Z';

    if (psrval & C_BIT)
	str[2] = 'C';

    if (psrval & V_BIT)
	str[3] = 'V';

    if (psrval & I_BIT)
	str[4] = 'I';

    if (psrval & F_BIT)
	str[5] = 'F';

    if (psrval & T_BIT)
	str[6] = 'T';

    switch (psrval & 0x1F)
	{
	case MODE_USER32:
	    strcat(str, "USER32");
	    break;

	case MODE_FIQ32:
	    strcat(str, "FIQ32");
	    break;

	case MODE_IRQ32:
	    strcat(str, "IRQ32");
	    break;

	case MODE_SVC32:
	    strcat(str, "SVC32");
	    break;

	case MODE_ABORT32:
	    strcat(str, "ABORT32");
	    break;

	case MODE_UNDEF32:
	    strcat(str, "UNDEF32");
	    break;

	case MODE_SYSTEM32:
	    strcat(str, "SYSTEM32");
	    break;

	default:
	    strcat(str, "------");
	    break;
	    }

    printf("%s\n", str);

    return OK;

    }

/*******************************************************************************
*
* armBreakpoint - handle breakpoint
*
* This routine is installed via a function pointer into the exception handling
* code. It handles the breakpoint exception and chains on to the
* architecture-independent breakpoint handling code from dbgLib.c.
* Note that this and wdbArchLib cannot be used at the same time as they
* use the same mechanism.
*
* RETURNS: N/A
*
*/

LOCAL void armBreakpoint
    (
    ESF * pInfo,		/* pointer to info saved on stack */
    REG_SET *pRegs		/* pointer to saved registers */
    )
    {
	dbgBreakpoint ((BREAK_ESF *)pInfo, (int *)pRegs);
    }

/*******************************************************************************
*
* _dbgStepAdd - add a breakpoint
*
* NOMANUAL
*
* This requirement for this routine appears to be to set a single-stepping
* breakpoint at the "next" instruction. Branches and so on must be predicted
* and we are allowed to put breakpoints at all possible "next" instructions.
* Deciding all possible next instructions on the ARM is sufficiently complex
* that in fact, we might as well work out exactly where the next instruction
* will be and put only one breakpoint there.
*
* RETURNS: status of adding breakpoints
*
*/

STATUS _dbgStepAdd
    (
    int		task,	/* task for which breakpoint is to be set */
    int		type,	/* breakpoint type (either BRK_STEP or BRK_TEMP) */
    BREAK_ESF * pEsf,
    int * 	pRegs
    )
    {
    REG_SET regSet;	/* task's register set */


    /* It appears to be the case that if the pointer to the ESF is null, then
     * there will be no regs either, so get them */

    if (pEsf == NULL)
	(void) taskRegsGet (task, &regSet);
    else
	regSet.pc = _dbgInfoPCGet (pEsf);


    /* find the next instruction to be executed and set a breakpoint there */

    return dbgBrkAdd (armGetNpc (*(regSet.pc), &regSet), task, 0, type);

    }

/* dbgArchLib.c - SH-dependent debugger library */
  
/* Copyright 1984-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
02t,24oct01,zl   fixes for doc builds.
02s,15nov00,zl   fixed _dbgInstSizeGet().
02r,06sep00,zl   simplified CPU conditionals. Updated _archHelp_msg.
02q,03may00,rsh  fix instruction mask error and add some better documentation
02p,21apr00,rsh  implement cret
02o,19apr00,frf  Modified dbgHelp and dbgBrkDisplayHard functions
02n,13apr00,frf  Removed BRKENTRY and HWBP
02m,27mar00,frf  Add SH support for T2: dbg API updated
02l,11mar99,hk   changed TBH_ to TSH_BH_, simplified CPU conditionals.
                 merged _archHelp_msg for all SH CPUs.
02k,09mar99,hk   changed to include CPU specific header for UBC register defs.
02j,09mar99,hk   changed macro prefix BH_ to TBH_, to recover target shell tool.
02i,02mar99,hk   retrieved _archHelp_msg for non-SENS branch.
02h,09oct98,hk   code review: sorted CPU conditionals. fixed dBRCR for SH7750.
02g,07oct98,st   changed BBRA,BBRB default setting for SH7750 from
                 BBR_BREAK_AT_INST_OR_DATA_ACCESS to BBR_BREAK_AT_INST_FETCH.
02f,16jul98,st   added support for SH7750.
02g,15oct98,kab  removed obsolete archHelp_msg.
02f,08may97,jmc  added support for SH-DSP and SH3-DSP.
02e,23apr98,hk   fixed _dbgStepAdd() against slot instr exception by s().
02d,25apr97,hk   changed SH704X to SH7040.
02c,09feb97,hk   renamed excBpHandle/excBpHwHandle to dbgBpStub/dbgHwBpStub.
02b,08aug96,hk   code layout review. changed some #if (CPU==SH7xxx) controls.
02a,24jul96,ja   added support for SH7700.
01z,21may96,hk   workarounded for SH7700 build.
01y,10may96,hk   added support for SH7700 (first phase).
01x,19dec95,hk   added support for SH704X.
01w,08aug95,sa   fixed _dbgStepAdd().
01v,28jun95,hk   rewrote _dbgBranchDelay().
01u,27jun95,hk   deleted _dbgBranchDelay().
01t,16mar95,hk   added bypass to the delay slot checking in _dbgBranchDelay().
01s,28feb95,hk   changed _dbgVecInit() to conform ivSh.h 01e.
01r,22feb95,hk   added SH7000 support. moved printBbr(), printBrcr() to sysLib.
01q,21feb95,hk   obsoleted bh(,4), more refinements, wrote some docs.
01p,20feb95,hk   limited data break setup only for ch.B.
01o,17feb95,hk   added bh(,4) to allow parameter customization.
01n,15feb95,hk   debugging bh() problem.
01m,07feb95,hk   copyright year 1995. more rewriting.
01l,11jan95,hk   rewriting h/w breakpoint stuff.
01k,25dec94,hk   fixed _archHelp_msg, clean-up. added _dbgBranchDelay().
		 fixed _dbgInstSizeGet(), so() now functional.
01j,23dec94,hk   changing macro names.
01i,21dec94,hk   working on UBC code. adding sequence diagram.
01h,18dec94,hk   writing UBC support code.
01g,15dec94,hk   adding hardware breakpoint function prototypes from i960 01t.
01f,15dec94,hk   use SR_BIT_T.
01e,15dec94,hk   more fixing. Now s() command is functional.
01d,06dec94,hk   fixing.
01c,01dec94,hk   included archPortKit notes. wrote most routines.
01b,26nov94,hk   wrote _dbgArchInit body.
01a,09oct94,hk   written based on sparc 01i.
*/

/*
DESCRIPTION
This module provides the SH specific support functions for dbgLib.

NOMANUAL

INTERNAL
This architecture-dependent debugger library contains some simple routines
that support the architecture-independent dbgLib.c. The complex portions of
the debugger have been abstracted. 

*/

#include "vxWorks.h"
#include "private/dbgLibP.h"
#include "taskLib.h"
#include "fppLib.h"
#include "taskArchLib.h"
#include "intLib.h"
#include "regs.h"
#include "iv.h"
#include "cacheLib.h"
#include "ioLib.h"
#include "dsmLib.h"
#include "vxLib.h"
#include "stdio.h"
#include "usrLib.h"


IMPORT int    dsmNbytes ();
IMPORT int    dsmInst ();
IMPORT INST * dsmCheck ();


/* globals */

/* _archHelp_msg
 *
 * INTERNAL
 * Architecture-specific help routines for the debugger are summarized in this
 * string. The routine syntax is added to the end of the architecture-indepent
 * routines displayed by dbgHelp(). The register display routines comprise the
 * minimal set, and it should include any additional functionality that may be
 * useful for debugging. [Arch port kit]
 */
char * _archHelp_msg =
 "bh        addr[,access[,task[,count[,quiet]]]] Set hardware breakpoint\n"
 "                access values:\n"
 "                 - Break on any access         (              00)\n"
 "                 - Break on instruction fetch  (              01)\n"
 "                 - Break on data access        (              10)\n"
 "                 - Bus cycle any               (            00  )\n"
 "                 - Bus cycle read              (            01  )\n"
 "                 - Bus cycle write             (            10  )\n"
 "                 - Operand size any            (          00    )\n"
 "                 - Operand size byte           (          01    )\n"
 "                 - Operand size word           (          10    )\n"
 "                 - Operand size long           (          11    )\n"
 "                 - CPU access                  (        00      )\n"
 "                 - DMAC access                 (        01      )\n"
 "                 - CPU or DMAC access          (        10      )\n"
 "                 - IBUS                        (      00        )\n"
 "                 - XBUS                        (      01        )\n"
 "                 - YBUS                        (      10        )\n"
 "   *Not all access combinations are supported by all SuperH CPUs.\n"
 "    Use of an invalid combination is not always reported as an error.\n"
 "r0-r15,sr,gbr,vbr,mach,macl,pr,pc [task]       Get a register of a task\n";

/* forward declarations */



/*******************************************************************************
*
* _dbgArchInit - architecture dependent initialization routine
*
* This routine initialize global function pointers that are architecture 
* specific.
*
* NOMANUAL
*
* INTERNAL
* This function is identical for all architectures. It is used to link the
* architecture-specific routines in this file to the architecture-independent
* debugger support. The generic function call attaches the new processor's
* debugger library support. [Arch port kit]
*
* NOTE
* This routine is called from dbgInit() only.
*/

void _dbgArchInit (void)
    {
    _dbgDsmInstRtn   = (FUNCPTR)  dsmInst;
    }

/*******************************************************************************
*
* _dbgInstSizeGet - set up breakpoint instruction
*
* RETURNS: size of the instruction at specified location.
*
* NOMANUAL
*
* INTERNAL
* This routine currently returns the number of 16-bit words needed to implement
* the breakpoint instruction at the specified address. It returns 16-bit words,
* instead of bytes, for compatibility with the original 68K debugger design;
* this does not make much sense for other architectures. In some future release
* the return value will be more architecture-independent, in other words, in
* bytes. [Arch port kit]
*
* NOTE
* This routine is called from so() only.  Any SH instruction is 16-bit length,
* but we treat a delayed branch instruction as 32-bit.  Otherwise we may insert
* the trapa instruction in a delay slot and gets an illegal slot exception.
*/

int _dbgInstSizeGet
    (
    INSTR * pBrkInst		/* pointer to hold breakpoint instruction */
    )
    {
    return (dsmNbytes (pBrkInst) / sizeof (INSTR));
    }

/*******************************************************************************
*
* _dbgRetAdrsGet - get return address for current routine
*
* RETURNS: return address for current routine.
*
* NOMANUAL
*
* INTERNAL
* A pointer to a REG_SET is passed to this routine. It returns the adress of the
* instruction in the calling routine that will be executed when this function
* returns. Depending on the processor's function call mechanism and pipelining,
* the calling routine's program counter may have to be adjusted to create the
* return address. [Arch port kit]
*
* INTERNAL
* While executing a leaf procedure, the pr register always holds the correct
* return address.  In case of a non-leaf procedure, this is not always true.
* After returning from a subroutine, pr keeps holding a return address of the
* subroutine.  The correct return address of non-leaf procedure is on stack.
*
* ex. proc: <<< pr valid >>>
*	     :
* 4f22      sts.l  pr, @-sp
*	     :
*	    mov.l  &subr,r0
*	    jsr    @r0              
*	    nop
*	     :
*	    <<< pr invalid >>>  ---> pr contains the return adrs of "subr".
*	     :
* 4f26      lds.l  @sp+,pr    ---> return adrs of "proc" is popped at here.
*	     :
* 000b      rts
*	    nop
*
* NOTE
* This routine currently only detects #imm adjustment of the stack. Consequently,
* it will not find the correct frame pointer adjustment in functions which have
* greater than 127 (7 #imm bits) bytes of parameters and local data. A search
* through usrConfig.o indicates such a case only occurs once. For Beta, this
* should be sufficient.
*
*/

INSTR * _dbgRetAdrsGet
    (
    REG_SET * pRegSet
    )
    {

    INSTR * scanpc;        /* instruction scan pc pointer for forward scan */
    UINT16 immed;
    INT32 offset;
    void * fp;

    /*
    * scan instructions forward. If we find a "sts.l pr,@-sp" or a "jsr @rm"
    * then the return address in already in the link register.  If we
    * find a "lds.l @sp+,pr" then the return address is saved on the
    * stack. We need to search back to find the offset. 
    * If we find "rts" without encountering the above instructions, it is
    * a leaf function and the return address is in register "pr".
    */
 
 
    for (scanpc = pRegSet->pc; TRUE; scanpc++)
        {
        /* 
         * if inst is "sts.l pr,@-sp" we are in the prolog.
         * if inst is "rts" we are in a leaf proceedure. Note that
         * this assumes you cannot break in the epilog, which would be
         * true for c code, but not necessarily for assembly.
         * Either way, the TCB's pr value is valid. 
         */

        if ((INST_CMP(scanpc, INST_PUSH_PR, 0xffff)) ||
            (INST_CMP(scanpc, INST_RTS, 0xffff)))
            {
            return (pRegSet->pr);
            }

        /*
         * we are somewhere in the function body of a non-leaf
         * routine and the pr may have been modified by a previous
         * function call. The correct pr is on the stack and must
         * be retrieved. Break from here and enter the search backwards
         * loop.
         */

        else if (INST_CMP(scanpc, INST_POP_PR, 0xffff))
            {
            break;
            }
        }

    /* if we arrive here, we are inside the function body and the current
     * tcb's pr value may be invalid (i.e. we may have called a subroutine
     * within the current function body which would have modified pr). 
     * Consequently, we'll need to search backwards to find 1) the current
     * frame pointer (stored in r14) and 2) the offset from the current
     * frame pointer back to the pr location on the stack. The sh compiler
     * sets the frame pointer to the stack location of the last parameter
     * or local allocation so that we have a variable offset back to the
     * pr location.
     */

    scanpc = pRegSet->pc;

    /* search back until we have the SET_FP instruction (mov.l sp,r14) */

    while (!(INST_CMP(scanpc, INST_SET_FP, 0xffff)))
        {
        scanpc--;
        }

    /* search back until the PUSH_PR instruction looking for a frame
     * adjustment instruction that modifies r15 before storing to r14.
     * (add #imm,sp). The #imm argument 
     */

    while (!(INST_CMP(scanpc, INST_PUSH_PR, 0xffff)))
        {
        if (INST_CMP(scanpc, INST_ADD_IMM_SP, MASK_ADD_IMM_SP)) 
            {
            immed = *(scanpc) & 0x00ff;

            /* "add #imm,sp" instruction sign extends #imm. Since this instruction
             * descremented the sp, #imm will be a negative value. sign extend
             * it to get it's proper negative value. And then reverse the sign.
             */

            offset = (0xffffff00 | (long) immed);   /* negative offset */
            offset = 0 - (offset);

            /* add offset to frame pointer */ 

            (ULONG *) fp = pRegSet->fpReg;

            (char *) fp += offset;

            /* retrieve and return pr */

            return ((INSTR *) *((ULONG *) fp));
            }
        scanpc--;
        }

    /* if we get here, then the offset is zero, so just return the value
     * held in r14 (the frame pointer).
     */

    return ((INSTR *) *((ULONG *) pRegSet->fpReg));

#if FALSE
    return ((INSTR *) ERROR);
#endif
    }

/*******************************************************************************
*
* _dbgFuncCallCheck - check next instruction
*
* This routine checks to see if the next instruction is a JSR or BSR.
* If it is, it returns TRUE, otherwise, returns FALSE.
*
* RETURNS: TRUE if next instruction is JSR or BSR, or FALSE otherwise.
*
* NOMANUAL
*
* INTERNAL
* This routine checks the instruction pointed to by the input argument to
* determine if it is an instruction that is used to implement a function call.
* If so, the function returns TRUE, otherwise the return value is FALSE.
* Note the use of the INST_CMP macro defined in dbgLib.h. [Arch port kit]
*
* NOTE
* This routine is called from so() only.
*/

BOOL _dbgFuncCallCheck
    (
    INSTR * addr
    )
    {
    /* SH JSR and BSR instructions:
     * 
     * JSR	@Rn		0100nnnn00001011	itAtOneReg  - 2/3
     * BSRF	Rn		0000nnnn00000011	itBraDispRn - 2/2
     * BSR	disp		1011dddddddddddd	itBraDisp12 - 2/2
     */
    return (INST_CMP (addr, 0x400b, 0xf0ff)	/* JSR  */
	||  INST_CMP (addr, 0x0003, 0xf0ff)	/* BSRF */
	||  INST_CMP (addr, 0xb000, 0xf000)	/* BSR  */ );
    }

/*******************************************************************************
*
* _dbgInfoPCGet - get pc from stack
*
* RETURNS: value of pc saved on stack
*
* NOMANUAL
*
* INTERNAL
* This routine returns a pointer to the instruction addressed by the program
* counter. The input argument is a pointer to the breakpoint stack frame. The
* return value is the program counter element of that structure, whose type
* should be an INSTR*. [Arch port kit]
*
* NOTE
* This routine is called from dbgBreakpoint() only.
*/

INSTR * _dbgInfoPCGet
    (
    BREAK_ESF * pInfo
    )
    {
    return (pInfo->pc);
    }

/*******************************************************************************
*
* _dbgTaskPCSet - set task's pc
*
* NOMANUAL
*
* INTERNAL
* The task identification and the program counter(s) are passed to this
* function which will set new program counter(s) for the specified task.
* A local copy of REG_SET is filled by the call to taskRegsGet(), the program
* counter(s) set, and then copied back to the task's TCB by taskRegsSet().
* This routine is similar for all architectures. [Arch port kit]
*
* NOTE
* This routine is called from c() and s().
*/

void _dbgTaskPCSet
    (
    int     tid,
    INSTR * pc,		/* task's pc                        */
    INSTR * npc		/* task's npc (not supported by SH) */
    )
    {
    REG_SET regSet;

    if (taskRegsGet (tid, &regSet) != OK)
	return;

    regSet.pc = pc;

    taskRegsSet (tid, &regSet);
    }

/*******************************************************************************
*
* _dbgTaskPCGet - get task's pc
*
* RETURNS: specified task's program counter
*
* NOMANUAL
*
* INTERNAL
* This routine returns a pointer to the instruction addressed by the program
* counter. The input argument is the task identifier used with taskRegsGet().
* The return value is the program counter element of that structure, whose
* type should be an INSTR*. [Arch port kit]
*
* NOTE
* This routine is called from c(), so(), dbgTlSnglStep(), and dbgTaskSwitch().
*/

INSTR * _dbgTaskPCGet
    (
    int     tid
    )
    {
    REG_SET regSet;

    (void) taskRegsGet (tid, &regSet);

    return ((INSTR *) regSet.pc);
    }



/*******************************************************************************
*
* getOneReg - return the contents of one register
*
* Given a task's ID, this routine returns the contents of the register
* specified by the register code.  This routine is used by r0, sr, etc.
* The register codes are defined in regsSh.h.
*
* NOMANUAL
*
* RETURNS: register contents, or ERROR.
*
* INTERNAL
* This routine gets the contents of a specific register in the REG_SET based on
* the task identifier and the register index. A call is made to taskIdFigure(),
* and the return value checked for an ERROR. taskIdDefault() and taskRegsGer()
* are called to fill a local copy of REG_SET. The index is used to return the
* contents of the register. [Arch port kit]
*
*/

LOCAL int getOneReg
    (
    int     taskId,			/* task ID, 0 means default task */
    int     regCode			/* code for specifying register */
    )
    {
    REG_SET regSet;			/* get task's regs into here */

    taskId = taskIdFigure (taskId);	/* translate super name to ID */

    if (taskId == ERROR)		/* couldn't figure out super name */
	return (ERROR);
    taskId = taskIdDefault (taskId);	/* set the default ID */

    if (taskRegsGet (taskId, &regSet) != OK)
	return (ERROR);

    return (*(int *)((int)&regSet + regCode));
    }

/*******************************************************************************
*
* r0 - return the contents of general register `r0' (also `r1'-`r15') (SH)
*
* This command extracts the contents of register `r0' from the TCB of a specified
* task.  If <taskId> is omitted or zero, the last task referenced is assumed.
*
* Similar routines are provided for all general registers (`r1' - `r15'):
* r1() - r15().
*
* RETURNS: The contents of register r0 (or the requested register).
*
* SEE ALSO:
* .pG "Debugging"
*
* INTERNAL
* Each control and general-purpose register should have a routine to display
* its contents in the REG_SET structure in the TCB. The task identifier and
* a register index is passed to the hidden (local) function getOneReg() which
* returns the contents. [Arch port kit]
*/

int r0
    (
    int taskId		/* task ID, 0 means default task */
    )
    {
    return (getOneReg (taskId, REG_SET_R0  ));
    }

int r1  (int taskId) { return (getOneReg (taskId, REG_SET_R1  )); }
int r2  (int taskId) { return (getOneReg (taskId, REG_SET_R2  )); }
int r3  (int taskId) { return (getOneReg (taskId, REG_SET_R3  )); }
int r4  (int taskId) { return (getOneReg (taskId, REG_SET_R4  )); }
int r5  (int taskId) { return (getOneReg (taskId, REG_SET_R5  )); }
int r6  (int taskId) { return (getOneReg (taskId, REG_SET_R6  )); }
int r7  (int taskId) { return (getOneReg (taskId, REG_SET_R7  )); }
int r8  (int taskId) { return (getOneReg (taskId, REG_SET_R8  )); }
int r9  (int taskId) { return (getOneReg (taskId, REG_SET_R9  )); }
int r10 (int taskId) { return (getOneReg (taskId, REG_SET_R10 )); }
int r11 (int taskId) { return (getOneReg (taskId, REG_SET_R11 )); }
int r12 (int taskId) { return (getOneReg (taskId, REG_SET_R12 )); }
int r13 (int taskId) { return (getOneReg (taskId, REG_SET_R13 )); }
int r14 (int taskId) { return (getOneReg (taskId, REG_SET_R14 )); }
int r15 (int taskId) { return (getOneReg (taskId, REG_SET_R15 )); }

/*******************************************************************************
*
* sr - return the contents of control register `sr' (also `gbr', `vbr') (SH)
*
* This command extracts the contents of register sr from the TCB of a specified
* task.  If <taskId> is omitted or zero, the last task referenced is assumed.
*
* Similar routines are provided for all control registers (`gbr', `vbr'):
* gbr(), vbr().
*
* RETURNS: The contents of register sr (or the requested control register).
*
* SEE ALSO:
* .pG "Debugging"
*
* INTERNAL
* Each control and general-purpose register should have a routine to display
* its contents in the REG_SET structure in the TCB. The task identifier and
* a register index is passed to the hidden (local) function getOneReg() which
* returns the contents. [Arch port kit]
*/

int sr
    (
    int taskId		/* task ID, 0 means default task */
    )
    {
    return (getOneReg (taskId, REG_SET_SR  ));
    }

int gbr (int taskId) { return (getOneReg (taskId, REG_SET_GBR )); }
int vbr (int taskId) { return (getOneReg (taskId, REG_SET_VBR )); }

/*******************************************************************************
*
* mach - return the contents of system register `mach' (also `macl', `pr') (SH)
*
* This command extracts the contents of register mach from the TCB of
* a specified task.  If <taskId> is omitted or zero, the last task referenced
* is assumed.
*
* Similar routines are provided for other system registers (`macl', `pr'):
* macl(), pr().  Note that pc() is provided by usrLib.c.
*
* RETURNS: The contents of register mach (or the requested system register).
*
* SEE ALSO:
* .pG "Debugging"
*
* INTERNAL
* Each control and general-purpose register should have a routine to display
* its contents in the REG_SET structure in the TCB. The task identifier and
* a register index is passed to the hidden (local) function getOneReg() which
* returns the contents. [Arch port kit]
*/

int mach
    (
    int taskId		/* task ID, 0 means default task */
    )
    {
    return (getOneReg (taskId, REG_SET_MACH));
    }

int macl(int taskId) { return (getOneReg (taskId, REG_SET_MACL)); }
int pr  (int taskId) { return (getOneReg (taskId, REG_SET_PR  )); }

#if	FALSE
int pc  (int taskId) { return (getOneReg (taskId, REG_SET_PC  )); }
#endif	/* FALSE, usrLib provides this. */


#if	DBG_HARDWARE_BP  	/* TO THE END OF THIS FILE */
/******************************************************************************
*
* _dbgBrkDisplayHard - display a hardware breakpoint
*
* NOMANUAL
*
* NOTE
* This routine is called from dbgBrkDisplay() only.
*/

void _dbgBrkDisplayHard
    (
    BRKPT *	pBp            /* breakpoint table entry */
    )
    {
    int type;

    if ((pBp->bp_flags & BRK_HARDWARE) == 0) 
        return;

    type = pBp->bp_flags & BRK_HARDMASK;

    printf ("\n            UBC");

    switch (type & BH_BREAK_MASK)
	{
	/* HW breakpoint on bus... */
	case BH_BREAK_INSN:  printf(" INST");	break;	/* istruction access */
	case BH_BREAK_DATA:  printf(" DATA");	break;	/* data access */
	default:             printf(" I/D");	break;	/* any */
	}
    switch (type & BH_CYCLE_MASK)
	{
        /* HW breakpoint on bus cycle... */
	case BH_CYCLE_READ:  printf(" READ");	break;	/* read  */
	case BH_CYCLE_WRITE: printf(" WRITE");	break;	/* write */
	default:             printf(" R/W");	break;  /* any */
	}
    switch (type & BH_SIZE_MASK)
	{
	/* HW breakpoint on operand size */
	case BH_8:           printf(" BYTE");	break;	/*  8 bit */
	case BH_16:          printf(" WORD");	break;	/* 16 bit */
	case BH_32:          printf(" LONG");	break;	/* 32 bit */
	}
    switch (type & BH_CPU_MASK)
	{
        /* HW breakpoint on bus cycle... */
	case BH_CPU:	     printf(" CPU");	break;	/* CPU */
	case BH_DMAC:	     printf(" DMA");	break;	/* DMA ctrl */
	case BH_DMAC_CPU:    printf(" DMA/CPU");break;	/* DMA/CPU */
	}
    switch (type & BH_BUS_MASK)
	{
        /* HW breakpoint on bus cycle... */
	case BH_XBUS:        printf(" XBUS");	break;	/* XBUS, DSP only */
	case BH_YBUS:        printf(" YBUS");	break;	/* YBUS, DSP only */
	}
    }
#endif	/* DBG_HARDWARE_BP */


/* dbgArchLib.c - solaris simulator debugger library */

/* Copyright 1993-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
02e,13nov01,hbh  Fixed return address in _dbgRetAdrsGet and updated comments.
02d,30apr98,dbt  removed unused _dbgInfoPCGet() routine.
02c,09jan98,dbt  modified for new breakpoint scheme.
02b,26jan96,ism  cleaned up
02a,07jun95,ism  converted to simsolaris
01f,26jan94,gae  minor improvement to exception display.
01e,17dec93,gae  fixed "Segmentaion" spelling.
01d,23aug93,rrr  fixup of trcStack and excShow routines.
01c,14jul93,gae  trcStack prints not supported; excShowInit() installs routines.
01b,09jul93,rrr  added trap handling.
01a,19jun93,rrr  written.
*/

/*
DESCRIPTION
This module provides the simsolaris specific support
functions for dbgLib.c.
*/

#include "vxWorks.h"
#include "private/dbgLibP.h"
#include "taskLib.h"
#include "taskArchLib.h"
#include "intLib.h"
#include "ioLib.h"
#include "iv.h"
#include "dsmLib.h"
#include "regs.h"
#include "stdio.h"
#include "usrLib.h"

/* externs */

IMPORT int dsmNbytes ();
IMPORT int dsmInst ();


/* defines */

#undef pc

/* globals */

char * _archHelp_msg =
    "i0-i7,l0-l7,o0-o7,g1-g7,\n"
    "pc,npc,psr,wim,y  [task]        Display a register of a task\n"
    "psrShow   value                 Display meaning of psr value\n";

/*******************************************************************************
*
* _dbgArchInit - architecture dependent initialization routine
*
* This routine initialize global function pointers that are architecture 
* specific.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void _dbgArchInit (void)
    {
    _dbgDsmInstRtn = (FUNCPTR) dsmInst;
    }

/*******************************************************************************
*
* _dbgInstSizeGet - set up breakpoint instruction
*
* RETURNS: size of the instruction at specified location.
*
* NOMANUAL
*/

int _dbgInstSizeGet
    (
    INSTR * pBrkInst		/* pointer to hold breakpoint instruction */
    )
    {
    return ((2 * sizeof (INSTR)) / sizeof (INSTR));
    }

/*******************************************************************************
*
* _dbgRetAdrsGet - get return address for current routine
*
* RETURNS: return address for current routine.
*
* NOMANUAL
*/

INSTR * _dbgRetAdrsGet
    (
    REG_SET * pRegSet		/* pointer to register set */
    )
    {
    int *sp = (int *) pRegSet->spReg;

    /* 
     * check if the following instruction is like :
     * 9de3bxxx		save	%sp, 0xffffffxx, %sp
     * if yes, then return address is in o7 register not in i7
     */

    if (INST_CMP (((INSTR *) (pRegSet->reg_pc)),INST_SAV,INST_SAV_MASK))
        return ((INSTR *) ((pRegSet->reg_out[7]) + 8));

    if (I7_CONTENTS (sp) != 0)
    	{
    	if (INST_CMP (I7_CONTENTS (sp), INST_CALL, INST_CALL_MASK) ||
            INST_CMP (I7_CONTENTS (sp), JMPL_o7, JMPL_o7_MASK))
            {
            return ((INSTR *) (I7_CONTENTS(sp) + 2));
            }
	}

    return (NULL);
    }

/*******************************************************************************
*
* _dbgFuncCallCheck - check next instruction
*
* This routine checks to see if the next instruction is a JSR or BSR.
* If it is, it returns TRUE, otherwise, returns FALSE.
*
* RETURNS: TRUE if next instruction is JSR or BSR, or FALSE otherwise.
*
* NOMANUAL
*/

BOOL _dbgFuncCallCheck
    (
    INSTR * addr		/* pointer to instruction */
    )
    {
    return (INST_CMP (addr, INST_CALL, INST_CALL_MASK) ||
            INST_CMP (addr, JMPL_o7, JMPL_o7_MASK));
    }

/*******************************************************************************
*
* _dbgTaskPCSet - set task's pc
*
* RETURNS: N/A
*
* NOMANUAL
*/

void _dbgTaskPCSet
    (
    int     tid,		/* task id */
    INSTR * pc,			/* task's pc */
    INSTR * npc			/* task's npc */
    )
    {
    REG_SET regSet;		/* task's register set */

    taskRegsGet (tid, &regSet);

    regSet.reg_pc = pc;
    if (npc == NULL)
	regSet.reg_npc = pc + 1;
    else
	regSet.reg_npc = npc;

    taskRegsSet (tid, &regSet);
    }

/*******************************************************************************
*
* _dbgTaskPCGet - get task's pc
*
* RETURNS: N/A
*
* NOMANUAL
*/

INSTR * _dbgTaskPCGet
    (
    int tid	/* task's id */
    )
    {
    REG_SET	regSet;

    taskRegsGet (tid, &regSet);
    return ((INSTR *) regSet.reg_pc);
    }

/*******************************************************************************
*
* getOneReg - return the contents of one register
*
* Given a task's ID, this routine returns the contents of the register
* specified by the register code.  This routine is used by g0, i0, psr, etc.
* The register codes are defined in regsSimsolaris.h.
*
* RETURNS: register contents, or ERROR.
*/

LOCAL int getOneReg
    (
    int taskId,		/* task ID, 0 means default task */
    int regCode		/* code for specifying register */
    )
    {
    REG_SET regSet;	/* get task's regs into here */

    taskId = taskIdFigure (taskId);	/* translate super name to ID */

    if (taskId == ERROR)		/* couldn't figure out super name */
	return (ERROR);
    taskId = taskIdDefault (taskId);	/* set the default ID */

    if (taskRegsGet (taskId, &regSet) != OK)
	return (ERROR);

    return (*(int *)((int)&regSet + regCode));
    }

/*******************************************************************************
*
* g0 - return the contents of register g0 (also g1-g7) (SimSolaris)
*
* This command extracts the contents of global register g0 from the TCB of a
* specified task.  If <taskId> is omitted or 0, the current default task is
* assumed.
*
* Similar routines are provided for all global registers (g0 - g7):
* g0() - g7().
*
* RETURNS: The contents of register g0 (or the requested register).
*
* SEE ALSO:
* .pG "Debugging"
*
* INTERNAL
* Although this routine is hereby marked NOMANUAL, it actually gets
* published, but from arch/doc/dbgArchLib.c.
*/

int g0
    (
    int taskId		/* task ID, 0 means default task */
    )
    
    {
    return (getOneReg (taskId, REG_SET_GLOBAL(0)));
    }

int g1 (int taskId) { return (getOneReg (taskId, REG_SET_GLOBAL(1))); }
int g2 (int taskId) { return (getOneReg (taskId, REG_SET_GLOBAL(2))); }
int g3 (int taskId) { return (getOneReg (taskId, REG_SET_GLOBAL(3))); }
int g4 (int taskId) { return (getOneReg (taskId, REG_SET_GLOBAL(4))); }
int g5 (int taskId) { return (getOneReg (taskId, REG_SET_GLOBAL(5))); }
int g6 (int taskId) { return (getOneReg (taskId, REG_SET_GLOBAL(6))); }
int g7 (int taskId) { return (getOneReg (taskId, REG_SET_GLOBAL(7))); }

/*******************************************************************************
*
* o0 - return the contents of register o0 (also o1-o7) (SimSolaris)
*
* This command extracts the contents of out register o0 from the TCB of a
* specified task.  If <taskId> is omitted or 0, the current default task is
* assumed.
*
* Similar routines are provided for all out registers (o0 - o7):
* o0() - o7().
*
* The stack pointer is accessed via o6.
*
* RETURNS: The contents of register o0 (or the requested register).
*
* SEE ALSO:
* .pG "Debugging"
*/

int o0
    (
    int taskId		/* task ID, 0 means default task */
    )
    
    {
    return (getOneReg (taskId, REG_SET_OUT(0)));
    }

int o1 (int taskId) { return (getOneReg (taskId, REG_SET_OUT(1))); }
int o2 (int taskId) { return (getOneReg (taskId, REG_SET_OUT(2))); }
int o3 (int taskId) { return (getOneReg (taskId, REG_SET_OUT(3))); }
int o4 (int taskId) { return (getOneReg (taskId, REG_SET_OUT(4))); }
int o5 (int taskId) { return (getOneReg (taskId, REG_SET_OUT(5))); }
int o6 (int taskId) { return (getOneReg (taskId, REG_SET_OUT(6))); }
int o7 (int taskId) { return (getOneReg (taskId, REG_SET_OUT(7))); }

/*******************************************************************************
*
* l0 - return the contents of register l0 (also l1-l7) (SimSolaris)
*
* This command extracts the contents of local register l0 from the TCB of a
* specified task.  If <taskId> is omitted or 0, the current default task is
* assumed.
*
* Similar routines are provided for all local registers (l0 - l7):
* l0() - l7().
*
* RETURNS: The contents of register l0 (or the requested register).
*
* SEE ALSO:
* .pG "Debugging"
*/

int l0
    (
    int taskId		/* task ID, 0 means default task */
    )
    
    {
    return (getOneReg (taskId, REG_SET_LOCAL(0)));
    }

int l1 (int taskId) { return (getOneReg (taskId, REG_SET_LOCAL(1))); }
int l2 (int taskId) { return (getOneReg (taskId, REG_SET_LOCAL(2))); }
int l3 (int taskId) { return (getOneReg (taskId, REG_SET_LOCAL(3))); }
int l4 (int taskId) { return (getOneReg (taskId, REG_SET_LOCAL(4))); }
int l5 (int taskId) { return (getOneReg (taskId, REG_SET_LOCAL(5))); }
int l6 (int taskId) { return (getOneReg (taskId, REG_SET_LOCAL(6))); }
int l7 (int taskId) { return (getOneReg (taskId, REG_SET_LOCAL(7))); }

/*******************************************************************************
*
* i0 - return the contents of register i0 (also i1-i7) (SimSolaris)
*
* This command extracts the contents of in register i0 from the TCB of a
* specified task.  If <taskId> is omitted or 0, the current default task is
* assumed.
*
* Similar routines are provided for all in registers (i0 - i7):
* i0() - i7().
*
* The frame pointer is accessed via i6.
*
* RETURNS: The contents of register i0 (or the requested register).
*
* SEE ALSO:
* .pG "Debugging"
*/

int i0
    (
    int taskId		/* task ID, 0 means default task */
    )
    
    {
    return (getOneReg (taskId, REG_SET_IN(0)));
    }

int i1 (int taskId) { return (getOneReg (taskId, REG_SET_IN(1))); }
int i2 (int taskId) { return (getOneReg (taskId, REG_SET_IN(2))); }
int i3 (int taskId) { return (getOneReg (taskId, REG_SET_IN(3))); }
int i4 (int taskId) { return (getOneReg (taskId, REG_SET_IN(4))); }
int i5 (int taskId) { return (getOneReg (taskId, REG_SET_IN(5))); }
int i6 (int taskId) { return (getOneReg (taskId, REG_SET_IN(6))); }
int i7 (int taskId) { return (getOneReg (taskId, REG_SET_IN(7))); }

/*******************************************************************************
*
* npc - return the contents of the next program counter (SimSolaris)
*
* This command extracts the contents of the next program counter from the TCB
* of a specified task.  If <taskId> is omitted or 0, the current default
* task is assumed.
*
* RETURNS: The contents of the next program counter.
*
* SEE ALSO: ti()
*/

int npc
    (
    int taskId                 /* task ID, 0 means default task */
    )

    {
    return (getOneReg (taskId, REG_SET_NPC));
    }

/*******************************************************************************
*
* psr - return the contents of the processor status register (SimSolaris)
*
* This command extracts the contents of the processor status register from
* the TCB of a specified task.  If <taskId> is omitted or 0, the default
* task is assumed.
*
* RETURNS: The contents of the processor status register.
*
* SEE ALSO: 
* .pG "Debugging"
*/

int psr
    (
    int taskId			/* task ID, 0 means default task */
    )
    {
    return (getOneReg (taskId, REG_SET_PSR));
    }

/*******************************************************************************
*
* wim - return the contents of the window invalid mask register (SimSolaris)
*
* This command extracts the contents of the window invalid mask register from
* the TCB of a specified task.  If <taskId> is omitted or 0, the default
* task is assumed.
*
* RETURNS: The contents of the window invalid mask register.
*
* SEE ALSO:
* .pG "Debugging"
*/

int wim
    (
    int taskId 			/* task ID, 0 means default task */
    )

    {
    return (getOneReg (taskId, REG_SET_WIM));
    }

/*******************************************************************************
*
* y - return the contents of the y register (SimSolaris)
*
* This command extracts the contents of the y register from the TCB of a
* specified task.  If <taskId> is omitted or 0, the default task is assumed.
*
* RETURNS: The contents of the y register.
*
* SEE ALSO:
* .pG "Debugging"
*/

int y
    (
    int taskId 			/* task ID, 0 means default task */
    )
    {
    return (getOneReg (taskId, REG_SET_Y));
    }


/* dbgArchLib.c - windows NT debugger library */

/* Copyright 1993-1998 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01c,23jul98,dbt  code cleanup
01b,19feb98,jmb  fix typo in symbol name and switch to dbgLibNew header.
01a,13jan98,cym  written.
*/

/*
DESCRIPTION
This module provides the windows specific support
functions for dbgLib.c.
*/

#include "vxWorks.h"
#include "dbgLib.h"
#include "taskLib.h"
#include "fppLib.h"
#include "taskArchLib.h"
#include "intLib.h"
#include "ioLib.h"
#include "iv.h"
#include "dsmLib.h"
#include "regs.h"
#include "vxLib.h"
#include "logLib.h"
#include "fioLib.h"
#include "stdio.h"
#include "usrLib.h"

/* interrupt driver routines from dsmLib.c */

IMPORT int dsmNbytes ();
IMPORT int dsmInst ();

/* globals */

char * _archHelp_msg =
    "Sorry, no help yet\n";

LOCAL oldIntLevel;		/* old interrupt level */

/*******************************************************************************
*
* _dbgArchInit - architecture dependent initialization routine
*
* This routine initialize global function pointers that are architecture 
* specific.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void _dbgArchInit (void)

    {
    _dbgDsmInstRtn = (FUNCPTR) dsmInst;
    }

/*******************************************************************************
*
* _dbgInstSizeGet - set up breakpoint instruction
*
* RETURNS: size of the instruction at specified location.
*
* NOMANUAL
*/

int _dbgInstSizeGet
    (
    INSTR * pBrkInst		/* pointer to hold breakpoint instruction */
    )
    {
    return (dsmNbytes(pBrkInst));
    }

/*******************************************************************************
*
* _dbgRetAdrsGet - get return address for current routine
*
* RETURNS: return address for current routine.
*
* NOMANUAL
*/

INSTR * _dbgRetAdrsGet
    (
    REG_SET * pRegSet		/* pointer to register set */
    )
    {
    INSTR *returnAddress;

#if FALSE
    if (DSM(pRegSet->pc,   PUSH_EBP, PUSH_EBP_MASK) &&
        DSM(pRegSet->pc+1, MOV_ESP0, MOV_ESP0_MASK) &&
        DSM(pRegSet->pc+2, MOV_ESP1, MOV_ESP1_MASK))
        {
        returnAddress = *(INSTR **)pRegSet->spReg;
        }
    else if (DSM(pRegSet->pc-1, PUSH_EBP, PUSH_EBP_MASK) &&
             DSM(pRegSet->pc,   MOV_ESP0, MOV_ESP0_MASK) &&
             DSM(pRegSet->pc+1, MOV_ESP1, MOV_ESP1_MASK))
        {
        returnAddress = *((INSTR **)pRegSet->spReg + 1);
        }
    else if (DSM(pRegSet->pc, ENTER, ENTER_MASK))
        {
        returnAddress = *(INSTR **)pRegSet->spReg;
        }
    else if ((DSM(pRegSet->pc, RET,    RET_MASK)) ||
             (DSM(pRegSet->pc, RETADD, RETADD_MASK)))
        {
        returnAddress = *(INSTR **)pRegSet->spReg;
        }
    else
        {
        returnAddress = *((INSTR **)pRegSet->fpReg + 1);
        }

#endif
    return (returnAddress);
    }

/*******************************************************************************
*
* _dbgFuncCallCheck - check next instruction
*
* This routine checks to see if the next instruction is a JSR or BSR.
* If it is, it returns TRUE, otherwise, returns FALSE.
*
* RETURNS: TRUE if next instruction is JSR or BSR, or FALSE otherwise.
*
* NOMANUAL
*/

BOOL _dbgFuncCallCheck
    (
    INSTR * addr		/* pointer to instruction */
    )
    {
#if FALSE
    return (INST_CMP (addr, INST_CALL, INST_CALL_MASK) ||
            INST_CMP (addr, JMPL_o7, JMPL_o7_MASK));
#else
return 0; /* XXX Change me!!! */
#endif 
    }

/*******************************************************************************
*
* _dbgTaskPCSet - set task's pc
*
* RETURNS: N/A
*
* NOMANUAL
*/
void _dbgTaskPCSet
    (
    int task,           /* task id */
    INSTR *pc,          /* new PC */
    INSTR *npc          /* not supported on I80X86 */
    )
    {
    REG_SET regSet;

    if (taskRegsGet (task, &regSet) != OK)
        return;

    regSet.pc = pc;
    (void)taskRegsSet (task, &regSet);
    }

/*******************************************************************************
*
* _dbgTaskPCGet - get task's pc
*
* RETURNS: N/A
*
* NOMANUAL
*/

INSTR * _dbgTaskPCGet
    (
    int tid	/* task's id */
    )
    {
    REG_SET	regSet;

    taskRegsGet (tid, &regSet);
    return ((INSTR *) regSet.pc);
    }

/*******************************************************************************
*
* getOneReg - return the contents of one register
*
* Given a task's ID, this routine returns the contents of the register
* specified by the register code.  This routine is used by eax, edx, etc.
* The register codes are defined in dbgI86Lib.h.
*
* RETURNS: register contents, or ERROR.
*/

LOCAL int getOneReg (taskId, regCode)
    int		taskId;		/* task's id, 0 means default task */
    int		regCode;	/* code for specifying register */

    {
    REG_SET	regSet;		/* get task's regs into here */

    taskId = taskIdFigure (taskId);	/* translate super name to id */

    if (taskId == ERROR)		/* couldn't figure out super name */
	return (ERROR);
    taskId = taskIdDefault (taskId);	/* set the default id */

    if (taskRegsGet (taskId, &regSet) != OK)
	return (ERROR);

    switch (regCode)
	{
	case EDI: return (regSet.edi);
	case ESI: return (regSet.esi);
	case EBP: return (regSet.ebp);
	case ESP: return (regSet.esp);
	case EBX: return (regSet.ebx);
	case EDX: return (regSet.edx);
	case ECX: return (regSet.ecx);
	case EAX: return (regSet.eax);

	case EFLAGS: return (regSet.eflags);
	}

    return (ERROR);		/* unknown regCode */
    }

/*******************************************************************************
*
* edi - return the contents of register `edi' (also `esi' - `eax') (x86/SimNT)
*
* This command extracts the contents of register `edi' from the TCB of a
* specified task.  If <taskId> is omitted or zero, the last task
* referenced is assumed.
*
* Similar routines are provided for all address registers (`edi' - `eax'):
* edi() - eax().
*
* The stack pointer is accessed via eax().
*
* RETURNS: The contents of register `edi' (or the requested register).
*
* SEE ALSO:
* .pG "Debugging"
*/

int edi
    (
    int taskId		/* task ID, 0 means default task */
    )

    {
    return (getOneReg (taskId, EDI));
    }

int esi (taskId) int taskId; { return (getOneReg (taskId, ESI)); }
int ebp (taskId) int taskId; { return (getOneReg (taskId, EBP)); }
int esp (taskId) int taskId; { return (getOneReg (taskId, ESP)); }
int ebx (taskId) int taskId; { return (getOneReg (taskId, EBX)); }
int edx (taskId) int taskId; { return (getOneReg (taskId, EDX)); }
int ecx (taskId) int taskId; { return (getOneReg (taskId, ECX)); }
int eax (taskId) int taskId; { return (getOneReg (taskId, EAX)); }

/*******************************************************************************
*
* eflags - return the contents of the status register (x86/SimNT)
*
* This command extracts the contents of the status register from the TCB of a
* specified task.  If <taskId> is omitted or zero, the last task referenced is
* assumed.
*
* RETURNS: The contents of the status register.
*
* SEE ALSO:
* .pG "Debugging"
*/

int eflags
    (
    int taskId		/* task ID, 0 means default task */
    )

    {
    return (getOneReg (taskId, EFLAGS));
    }
