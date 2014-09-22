/* wdbDbgArchLib.c - i86 specific debug support */

/* Copyright 1984-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01f,30may02,hdn  added wdbDbgCtxPc/Cs/Esp to save the state (spr 75694)
01e,29aug01,hdn  replaced intVecSet with intVecSet2
		 doc: cleanup.
01d,08jan98,dbt  modified for new breakpoint scheme. Added hardware
		 breakpoints support
01c,15jun95,ms	 removed sigCtxIntLock. Made traceModeSet/Clear use intRegsLock.
01b,25may95,ms	 cleaned up + added _sigCtxIntLock.
01a,18jan94,rrr  written.
*/

/*
DESCPRIPTION

Arch-specific debug support.

i80386 has four breakpoint registers and the following types of hardware
breakpoint:
.CS
   BRK_INST		/@ instruction hardware breakpoint @/
   BRK_DATAW1		/@ data write 1 byte breakpoint @/
   BRK_DATAW2		/@ data write 2 byte breakpoint @/
   BRK_DATAW4		/@ data write 4 byte breakpoint @/
   BRK_DATARW1		/@ data read-write 1 byte breakpoint @/
   BRK_DATARW2		/@ data read-write 2 byte breakpoint @/
   BRK_DATARW4		/@ data read-write 4 byte breakpoint @/
.CE

NOMANUAL
*/

#include "vxWorks.h"
#include "regs.h"
#include "iv.h"
#include "intLib.h"
#include "esf.h"
#include "wdb/wdbDbgLib.h"


/* externals */

extern void wdbDbgBpStub(void);
extern void wdbDbgTraceStub(void);
extern int  sysCsExc;


/* globals */

UINT32	wdbDbgCtxCs	= 0;	/* old CS */
UINT32	wdbDbgCtxPc	= 0;	/* old PC */
UINT32	wdbDbgCtxEsp	= 0;	/* old ESP */


/* forward declaration */

static void wdbDbgArchRegsGet (TRACE_ESF * info, int * regs, REG_SET * pRegSet,
				DBG_REGS * pDbgRegSet, BOOL traceException);


/*******************************************************************************
*
* wdbDbgArchInit - set exception handlers for the break and the trace.
*
* This routine set exception handlers for the break and the trace.
* And also make a break instruction.
*
* NOMANUAL
*/

void wdbDbgArchInit
    (
    void
    )
    {
    /* Insert the new breakpoint and trace vectors */

    intVecSet2 ((FUNCPTR *)IV_BREAKPOINT, (FUNCPTR) wdbDbgBpStub,
	                   IDT_INT_GATE, sysCsExc);
    intVecSet2 ((FUNCPTR *)IV_DEBUG, (FUNCPTR) wdbDbgTraceStub,
		           IDT_INT_GATE, sysCsExc);
    }

/*******************************************************************************
*
* wdbDbgTraceModeSet - set a debug-mode or a trace-mode
*
* This routine make CPU trace-enable.
*
* NOMANUAL
*/

int wdbDbgTraceModeSet
    (
    REG_SET *	pRegs
    )
    {
    int tmp;

    tmp = intRegsLock (pRegs);
    pRegs->eflags |= TRACE_FLAG;

    return (tmp);
    }

/*******************************************************************************
*
* wdbDbgTraceModeClear - clear a trace-mode
*
* This routine make CPU trace-disable.
*
* NOMANUAL
*/

void wdbDbgTraceModeClear
    (
    REG_SET *	pRegs,
    int		arg
    )
    {
    intRegsUnlock (pRegs, arg);
    pRegs->eflags &= ~TRACE_FLAG;
    }

/*******************************************************************************
*
* wdbDbgArchRegsGet - get a register set from saved registers on stack
*
* This routine gets a register set from saved registers on stack.
*
* RETURNS: N/A
*
* NOMANUAL
*/

static void wdbDbgArchRegsGet
    (
    TRACE_ESF *	info,		/* pointer to esf info saved on stack */
    int * 	regs,		/* pointer to regs saved on stack */
    REG_SET *	pRegSet,	/* pointer to register set */
    DBG_REGS *	pDbgRegSet,	/* pointer to register set */
    BOOL	traceException	/* TRUE if this was a trace exception */
    )
    {
    if (pDbgRegSet != NULL)
	{
	pDbgRegSet->db0 = *regs++;	/* read the debug registers */
	pDbgRegSet->db1 = *regs++;
	pDbgRegSet->db2 = *regs++;
	pDbgRegSet->db3 = *regs++;
	pDbgRegSet->db6 = *regs++;
	pDbgRegSet->db7 = *regs++;
	}
    else
	regs += 6;

    pRegSet->edi = *regs++;	/* read the global registers */
    pRegSet->esi = *regs++;
    pRegSet->ebp = *regs++;
    regs++;
    pRegSet->ebx = *regs++;
    pRegSet->edx = *regs++;
    pRegSet->ecx = *regs++;
    pRegSet->eax = *regs++;

    pRegSet->spReg = (ULONG) ((char *)info + sizeof (ESF0));

    pRegSet->pc  = info->pc;
    pRegSet->eflags = info->eflags;
    }

/******************************************************************************
*
* wdbDbgPreBreakpoint - handle a breakpoint
*
* This routine is the I86 specific handler for breakpoints (soft and hardware).
*
* RETURNS: N/A
*
* NOMANUAL
*/ 

void wdbDbgPreBreakpoint
    (
    BREAK_ESF *	info,		/* pointer to info saved on stack */
    int *	regs,		/* pointer to saved registers */
    BOOL	hardware	/* TRUE if it is hardware breakpoint */
    )
    {
    REG_SET	regSet;
    DBG_REGS	dbgRegSet;

    wdbDbgArchRegsGet (info, regs, &regSet, &dbgRegSet, FALSE);

    /* save the CS in ESF, to restore it in _wdbDbgCtxLoad() */

    wdbDbgCtxCs  = info->cs;
    wdbDbgCtxPc  = (UINT32)regSet.pc;
    wdbDbgCtxEsp = regSet.esp;

    wdbDbgBreakpoint((void *)info, &regSet, &dbgRegSet, hardware);
    }

/******************************************************************************
*
* wdbDbgPreTrace - handle a single step
*
* This routine is the I86 specific handler for single steps.
*
* RETURNS: N/A
*
* NOMANUAL
*/ 

void wdbDbgPreTrace
    (
    BREAK_ESF *	info,		/* pointer to info saved on stack */
    int *	regs,		/* pointer to saved registers */
    BOOL	hardware	/* TRUE if it is a hardware breakpoint */
    )
    {
    REG_SET	regSet;

    wdbDbgArchRegsGet (info, regs, &regSet, NULL, FALSE);

    /* save the CS in ESF, to restore it in _wdbDbgCtxLoad() */

    wdbDbgCtxCs  = info->cs;
    wdbDbgCtxPc  = (UINT32)regSet.pc;
    wdbDbgCtxEsp = regSet.esp;

    wdbDbgTrace((void *)info, &regSet);
    }

#if	DBG_HARDWARE_BP
/*******************************************************************************
*
* wdbDbgHwAddrCheck - check the address for the hardware breakpoint.
*
* This routine check the address for the hardware breakpoint.
*
* RETURNS: OK or ERROR if the address is not appropriate.
*
* NOMANUAL
*/

STATUS wdbDbgHwAddrCheck
    (
    UINT32 	addr,		/* address for hardware breakpoint */
    UINT32	type,		/* hardware breakpoint type */
    FUNCPTR	memProbeRtn	/* memProbe routine */
    )
    {
    ULONG dummy;

    type &= BRK_HARDMASK;

    /* be sure address isn't odd, or beyond end of memory */

    if ((type == BRK_DATAW1) || (type == BRK_DATARW1))
        {
        if (memProbeRtn ((char *) addr, READ, 1, (char*)&dummy) != OK)
            return (ERROR);
        }
    else if ((type == BRK_DATAW2) || (type == BRK_DATARW2))
        {
        if (((int)addr & 1) ||
            memProbeRtn ((char *) addr, READ, 2, (char*)&dummy) != OK)
	    {
            return (ERROR);
            }
        }
    else if ((type == BRK_DATAW4) || (type == BRK_DATARW4))
        {
        if ((addr & 3) ||
            memProbeRtn ((char *) addr, READ, 4, (char*)&dummy) != OK)
            {
            return (ERROR);
            }
        }

    return (OK);
    }

/******************************************************************************
*
* wdbDbgHwBpSet - set a data breakpoint register
*
* type is the type of access that will generate a breakpoint.
*
* NOMANUAL
*/

STATUS wdbDbgHwBpSet 
    (
    DBG_REGS *	pDbgRegs,	/* debug registers */
    UINT32	access,		/* access type */
    UINT32 	addr		/* breakpoint addr */
    )
    {
    switch (access)
	{
	case BRK_INST:
	case BRK_DATAW1:
	case BRK_DATAW2:
	case BRK_DATAW4:
	case BRK_DATARW1:
	case BRK_DATARW2:
	case BRK_DATARW4:
	    break;
	default:
	    return (WDB_ERR_INVALID_HW_BP);
	}

    if (pDbgRegs->db0 == 0)
	{
	pDbgRegs->db0 = addr;
	pDbgRegs->db7 |= (access << 16) | 0x02;
	}
    else if (pDbgRegs->db1 == 0)
	{
	pDbgRegs->db1 = addr;
	pDbgRegs->db7 |= (access << 20) | 0x08;
	}
    else if (pDbgRegs->db2 == 0)
	{
	pDbgRegs->db2 = addr;
	pDbgRegs->db7 |= (access << 24) | 0x20;
	}
    else if (pDbgRegs->db3 == 0)
	{
	pDbgRegs->db3 = addr;
	pDbgRegs->db7 |= (access << 28) | 0x80;
	}
    else
	return (WDB_ERR_HW_REGS_EXHAUSTED);

    /* set GE bit if it is data breakpoint */

    if (access & 0x300)
	pDbgRegs->db7 |= 0x200;

    return (OK);
    }

/******************************************************************************
*
* wdbDbgHwBpFind - Find the hardware breakpoint
*
* This routines find the type and the address of the address of the 
* hardware breakpoint that is set in the DBG_REGS structure.
* Those informations are stored in the breakpoint structure that is passed
* in parameter.
*
* RETURNS : OK or ERROR if unable to find a hardware breakpoint
*
* NOMANUAL
*/ 

STATUS wdbDbgHwBpFind
    (
    DBG_REGS *	pDbgRegs,	/* debug registers */
    UINT32 *	pType,		/* return type info via this pointer */
    UINT32 *	pAddr		/* return address info via this pointer */
    )
    {
    int ix;
    int type = 0;
    int addr = 0;
    int statusBit, enableBit;

    /* get address and type of breakpoint from DR6 and DR7 */

    for (ix=0; ix<4; ix++)
	{
	statusBit = 1 << ix;
	enableBit = 2 << (ix << 1);

	if ((pDbgRegs->db6 & statusBit) && (pDbgRegs->db7 & enableBit))
	    {
	    switch (ix)
		{
		case 0:
		    addr = pDbgRegs->db0;
		    type = (pDbgRegs->db7 & 0x000f0000) >> 16 | BRK_HARDWARE;
		    break;

		case 1:
		    addr = pDbgRegs->db1;
		    type = (pDbgRegs->db7 & 0x00f00000) >> 20 | BRK_HARDWARE;
		    break;

		case 2:
		    addr = pDbgRegs->db2;
		    type = (pDbgRegs->db7 & 0x0f000000) >> 24 | BRK_HARDWARE;
		    break;

		case 3:
		    addr = pDbgRegs->db3;
		    type = (pDbgRegs->db7 & 0xf0000000) >> 28 | BRK_HARDWARE;
		    break;
		}
	    }
	}

    if ((addr == 0) && (type == 0))
	return (ERROR);

    *pType = type;
    *pAddr = addr;

    return (OK);
    }

/*******************************************************************************
*
* wdbDbgRegsClear - clear hardware break point registers
*
* This routine cleans hardware breakpoint registers.
*
* RETURNS : N/A.
*
* NOMANUAL
*/

void wdbDbgRegsClear
    (
    void
    )
    {
    DBG_REGS dbgRegs;

    dbgRegs.db0 = 0;
    dbgRegs.db1 = 0;
    dbgRegs.db2 = 0;
    dbgRegs.db6 = 0;
    dbgRegs.db7 = 0;

    wdbDbgRegsSet (&dbgRegs);
    }
#endif	/* DBG_HARDWARE_BP */
