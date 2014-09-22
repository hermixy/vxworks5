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


/* externals */

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

