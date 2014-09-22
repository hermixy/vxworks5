/* trcLib.c - i80x86 stack trace library */

/* Copyright 1984-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01f,16jan02,pai  replaced obsolete symFindByValue (_func_symFindByValue) with
                 symByValueFind.  Removed FAST variable qualifier.
                 Cleaned up formatting and updated copyright for T2.2.
01e,12may95,p_m  adapted to support host based tt().
01d,26oct94,hdn  added a MAX_LOOPCOUNT to avoid infinite loop.
01c,09dec93,hdn  added a forward declaration of trcCountArgs().
                 commented out trcFollowJmp(pc) in trcFindFuncStart().
01b,01jun93,hdn  added third parameter tid to trcStack().
                 updated to 5.1.
                  - changed functions to ansi style
                  - changed VOID to void
                  - changed copyright notice
01a,16jul92,hdn  written based on TRON version.
*/

/*
This module provides a routine, trcStack(), which traces a stack
given the current frame pointer, stack pointer, and program counter.
The resulting stack trace lists the nested routine calls and their arguments.

This module provides the low-level stack trace facility.
A higher-level symbolic stack trace, implemented on top of this facility,
is provided by the routine tt() in dbgLib.

SEE ALSO: dbgLib, tt(),
.pG "Debugging"
*/

#include "vxWorks.h"
#include "regs.h"
#include "stdio.h"
#include "stdlib.h"
#include "symLib.h"
#include "sysSymTbl.h"
#include "private/funcBindP.h"

#define MAX_TRACE_DEPTH       40  /* max number of levels of stack to trace */
#define MAX_LOOPCOUNT        128  /* max loop count */

/* instruction patterns and masks */

#define ADDI08_0                0x83
#define ADDI08_1                0xc4
#define ADDI32_0                0x81
#define ADDI32_1                0xc4
#define LEAD08_0                0x8d
#define LEAD08_1                0x64
#define LEAD08_2                0x24
#define LEAD32_0                0x8d
#define LEAD32_1                0xa4
#define LEAD32_2                0x24
#define JMPD08                  0xeb
#define JMPD32                  0xe9
#define ENTER                   0xc8
#define PUSH_EBP                0x55
#define MOV_ESP0                0x89
#define MOV_ESP1                0xe5
#define LEAVE                   0xc9
#define RET                     0xc3
#define RETADD                  0xc2
#define CALL_DIR                0xe8
#define CALL_INDIR0             0xff
#define CALL_INDIR1             0x10

#define ADDI08_0_MASK           0xff
#define ADDI08_1_MASK           0xff
#define ADDI32_0_MASK           0xff
#define ADDI32_1_MASK           0xff
#define LEAD08_0_MASK           0xff
#define LEAD08_1_MASK           0xff
#define LEAD08_2_MASK           0xff
#define LEAD32_0_MASK           0xff
#define LEAD32_1_MASK           0xff
#define LEAD32_2_MASK           0xff
#define JMPD08_MASK             0xff
#define JMPD32_MASK             0xff
#define ENTER_MASK              0xff
#define PUSH_EBP_MASK           0xff
#define MOV_ESP0_MASK           0xff
#define MOV_ESP1_MASK           0xff
#define LEAVE_MASK              0xff
#define RET_MASK                0xff
#define RETADD_MASK             0xff
#define CALL_DIR_MASK           0xff
#define CALL_INDIR0_MASK        0xff
#define CALL_INDIR1_MASK        0x38

#define DSM(addr,inst,mask)     ((*(addr) & (mask)) == (inst))


/* globals */

int trcDefaultArgs = 5;         /* default # of args to print if trc
                                 * can't figure out how many */

/* forward declarations */

LOCAL INSTR * trcFindCall (INSTR * returnAdrs);
LOCAL INSTR * trcFindFuncStart (int * fp, INSTR * pc);
LOCAL INSTR * trcFindDest (INSTR * callAdrs);
LOCAL INSTR * trcFollowJmp (INSTR * addr);
LOCAL void    trcDefaultPrint (INSTR * callAdrs, INSTR * funcAdrs, int nargs,
                               int * args);
LOCAL void    trcStackLvl (int * fp, INSTR * pc, int depth, FUNCPTR printRtn);
LOCAL int     trcCountArgs (INSTR * returnAdrs);

/*******************************************************************************
*
* trcStack - print a trace of function calls from the stack
*
* This routine provides the low-level stack trace function.  A higher-level
* symbolic stack trace, built on top of trcStack(), is provided by tt() in
* dbgLib.
*
* This routine prints a list of the nested routine calls that are on the
* stack, showing each routine with its parameters.
*
* The stack being traced should be quiescent.  The caller should avoid
* tracing its own stack.
*
* PRINT ROUTINE
* In order to allow symbolic or alternative printout formats, the call to
* this routine includes the <printRtn> parameter, which specifies a
* user-supplied routine to be called at each nesting level to print out the
* routine name and its arguments.  This routine should be declared as
* follows:
* .ne 7
* .CS
*     void printRtn
*         (
*         INSTR *  callAdrs,  /@ address from which routine was called @/
*         int      rtnAdrs,   /@ address of routine called             @/
*         int      nargs,     /@ number of arguments in call           @/
*         int *    args       /@ pointer to arguments                  @/
*         )
* .CE
* If <printRtn> is NULL, a default routine will be used that prints out just
* the call address, the function address, and the arguments as hexadecimal
* values.
*
* CAVEAT
* In order to do the trace, a number of assumptions are made.  In general,
* the trace will work for all C language routines and for assembly language
* routines that start with an PUSH %EBP MOV %ESP %EBP instruction.  Most 
* VxWorks assembly language routines include PUSH %EBP MOV %ESP %EBP 
* instructions for exactly this reason.
* However, routines written in other languages, strange entries into
* routines, or tasks with corrupted stacks can confuse the trace.  Also,
* all parameters are assumed to be 32-bit quantities, therefore structures
* passed as parameters will be displayed as a number of long integers.
*
* EXAMPLE
* The following sequence can be used
* to trace a VxWorks task given a pointer to the task's TCB:
* .CS
* REG_SET regSet;        /@ task's data registers @/
*
* taskRegsGet (taskId, &regSet);
* trcStack (&regSet, (FUNCPTR)printRtn, tid);
* .CE
*
* SEE ALSO: tt()
*
* NOMANUAL
*/

void trcStack
    (
    REG_SET * pRegSet,        /* pointer to register set */
    FUNCPTR   printRtn,       /* routine to print single function call */
    int       tid             /* task's id */
    )
    {
    int       val;            /* address gotten from symbol table */
    SYM_TYPE  type;           /* type associated with val */
    INSTR *   addr;           /* next instruction */
    int       stackSave;
    char *    pName = NULL;   /* string associated with val */

    INSTR *   pc    = pRegSet->pc;             /* program counter */
    int *     fp    = (int *)pRegSet->fpReg;   /* stack frame pointer (EBP) */
    int *     sp    = (int *)pRegSet->spReg;   /* stack pointer */

    /* use default print routine if none specified */

    if (printRtn == NULL)
        printRtn = (FUNCPTR)trcDefaultPrint;

    /*
     * if the current routine doesn't have a stack frame, then we fake one
     * by putting the old one on the stack and making fp point to that;
     * we KNOW we don't have a stack frame in a few restricted but useful
     * cases:
     *  1) we are at a PUSH %EBP MOV %ESP %EBP or RET or ENTER instruction,
     *  2) we are the first instruction of a subroutine (this may NOT be
     *     a PUSH %EBP MOV %ESP %EBP instruction with some compilers)
     */

    addr = trcFollowJmp (pc);

    if ((DSM(addr,   PUSH_EBP, PUSH_EBP_MASK) && 
         DSM(addr+1, MOV_ESP0, MOV_ESP0_MASK) &&
         DSM(addr+2, MOV_ESP1, MOV_ESP1_MASK)) ||

        (DSM(addr,   ENTER,    ENTER_MASK))    ||
        (DSM(addr,   RET,      RET_MASK))      ||
        (DSM(addr,   RETADD,   RETADD_MASK))   ||

        ((sysSymTbl != NULL) &&
         (symByValueFind (sysSymTbl, (UINT) pc, &pName, &val, &type) == OK) &&
         (val == (int) pc)))
        {
        /* no stack frame - fake one */

        stackSave = *(sp - 1);      /* save value we're going to clobber */
        *(sp - 1) = (int)fp;        /* make new frame pointer by */
                                    /* sticking old one on stack */
        fp = sp - 1;                /* and pointing to it */

        trcStackLvl (fp, pc, 0, printRtn);        /* do stack trace */

        *(sp - 1) = stackSave;                    /* restore stack */
        }
    else if ((DSM(addr-1, PUSH_EBP, PUSH_EBP_MASK) && 
              DSM(addr,   MOV_ESP0, MOV_ESP0_MASK) &&
              DSM(addr+1, MOV_ESP1, MOV_ESP1_MASK)))
        {
        fp = sp;
        trcStackLvl (fp, pc, 0, printRtn);        /* do stack trace */
        }
    else
        {
        trcStackLvl (fp, pc, 0, printRtn);        /* do stack trace */
        }

    if (pName != NULL)
        {
        free (pName);  /* new API requires this */
        }
    }
/************************************************************************
*
* trcStackLvl - recursive stack trace routine
*
* This routine is recursive, being called once for each level of routine
* nesting.  The maximum recursion depth is limited to 40 to prevent
* garbage stacks from causing this routine to continue unbounded.
* The "depth" parameter on the original call should be 0.
*/

LOCAL void trcStackLvl
    (
    int *    fp,        /* stack frame pointer (EBP) */
    INSTR *  pc,        /* program counter */
    int      depth,     /* recursion depth */
    FUNCPTR  printRtn   /* routine to print single function call */
    )
    {
    INSTR *  returnAdrs;


    if (fp == NULL)
        return;        /* stack is untraceable */

    returnAdrs = (INSTR *) *(fp + 1);

    /* handle oldest calls first, up to MAX_TRACE_DEPTH of them */

    if (((void *)*fp != NULL) && (depth < MAX_TRACE_DEPTH))
        {
        trcStackLvl ((int *) *fp, returnAdrs, depth + 1, printRtn);
        }

    (* printRtn) (trcFindCall (returnAdrs), trcFindFuncStart (fp, pc),
                  trcCountArgs (returnAdrs), fp + 2);
    }
/*******************************************************************************
*
* trcDefaultPrint - print a function call
*
* This routine is called by trcStack to print each level in turn.
*
* If nargs is specified as 0, then a default number of args (trcDefaultArgs)
* is printed in brackets ("[..]"), since this often indicates that the
* number of args is unknown.
*/

LOCAL void trcDefaultPrint
    (
    INSTR * callAdrs,   /* address from which function was called */
    INSTR * funcAdrs,   /* address of function called */
    int     nargs,      /* number of arguments in function call */
    int *   args        /* pointer to function args */
    )
    {
    int     i;
    BOOL    doingDefault = FALSE;

    /* if there is no printErr routine do nothing */

    if (_func_printErr == NULL)
        return;

    /* print call address and function address */

    (* _func_printErr) ("%8x: %x (", callAdrs, funcAdrs);

    /* if no args are specified, print out default number (see doc at top) */

    if ((nargs == 0) && (trcDefaultArgs != 0))
        {
        doingDefault = TRUE;
        nargs = trcDefaultArgs;
        (* _func_printErr) ("[");
        }

    /* print args */

    for (i = 0; i < nargs; ++i)
        {
        if (i != 0)
            {
            (* _func_printErr) (", ");
            }

        (* _func_printErr) ("%x", args[i]);
        }

    if (doingDefault)
        {
        (* _func_printErr) ("]");
        }

    (* _func_printErr) (")\n");
    }
/****************************************************************************
*
* trcFindCall - get address from which function was called
*
* RETURNS: address from which current subroutine was called, or NULL.
*/

LOCAL INSTR * trcFindCall
    (
    INSTR * returnAdrs     /* return address */
    )
    {
    INSTR * addr;
    int     ix = 0;

    /* starting at the word preceding the return adrs, search for CALL */

    for (addr = returnAdrs - 1; addr != NULL; --addr)
        {
        if ((DSM(addr,   CALL_INDIR0, CALL_INDIR0_MASK) &&
             DSM(addr+1, CALL_INDIR1, CALL_INDIR1_MASK)) ||

            (DSM(addr,   CALL_DIR,    CALL_DIR_MASK)))
            {
            return (addr);              /* found it */
            }

        if (ix++ > MAX_LOOPCOUNT)       /* XXX */
            {
            break;
            }
        }

    return (NULL);                      /* not found */
    }
/****************************************************************************
*
* trcFindDest - find destination of call instruction
*
* RETURNS: address to which call instruction (CALL) will branch or NULL if
* unknown
*/

LOCAL INSTR * trcFindDest
    (
    INSTR * callAdrs
    )
    {
    if (DSM(callAdrs, CALL_DIR, CALL_DIR_MASK))
        {
        /* PC relative offset */

        int displacement = *(int *)(callAdrs + 1);

        /* program counter */

        INSTR * pc = (INSTR *)((int)callAdrs + 1 + sizeof(int));

        return ((INSTR *) ((int)pc + displacement));
        }
    
    return (NULL);           /* don't know destination */
    }
/****************************************************************************
*
* trcCountArgs - find number of arguments to function
*
* This routine finds the number of arguments passed to the called function
* by examining the stack-pop at the return address.  Many compilers offer
* optimization that defeats this (e.g. by coalescing stack-pops), so a return
* value of 0, may mean "don't know".
*
* RETURNS: number of arguments of function
*/

LOCAL int trcCountArgs
    (
    INSTR * returnAdrs    /* return address of function call */
    )
    {
    INSTR * addr;
    int     nbytes;

    /* if inst is a JMP, use the target of the JMP as the returnAdrs */

    addr = trcFollowJmp (returnAdrs);

    if (DSM(addr,   ADDI08_0, ADDI08_0_MASK) &&
        DSM(addr+1, ADDI08_1, ADDI08_1_MASK))
        {
        nbytes = *(char *)(addr + 2);
        }
    else if (DSM(addr,   ADDI32_0, ADDI32_0_MASK) &&
             DSM(addr+1, ADDI32_1, ADDI32_1_MASK))
        {
        nbytes = *(int *)(addr + 2);
        }
    else if (DSM(addr,   LEAD08_0, LEAD08_0_MASK) &&
             DSM(addr+1, LEAD08_1, LEAD08_1_MASK) &&
             DSM(addr+2, LEAD08_2, LEAD08_2_MASK))
        {
        nbytes = *(char *)(addr + 3);
        }
    else if (DSM(addr,   LEAD32_0, LEAD32_0_MASK) &&
             DSM(addr+1, LEAD32_1, LEAD32_1_MASK) &&
             DSM(addr+2, LEAD08_2, LEAD08_2_MASK))
        {
        nbytes = *(int *)(addr + 3);
        }
    else
        {
        nbytes = 0;                                /* no args, or unknown */
        }

    if (nbytes < 0)
        nbytes = 0 - nbytes;

    return (nbytes >> 2);
    }
/****************************************************************************
*
* trcFindFuncStart - find the starting address of a function
*
* This routine finds the starting address of a function by one of several ways.
*
* If the given frame pointer points to a legitimate frame pointer, then the
* long word following the frame pointer pointed to by the frame pointer should
* be the return address of the function call. Then the instruction preceding
* the return address would be the function call, and the address can be gotten
* from there, provided that the CALL was to an pc-relative address. If it was,
* use that address as the function address.  Note that a routine that is
* called by other than a call-direct (e.g. indirectly) will not meet these
* requirements.
* 
* If the above check fails, we search backward from the given pc until a
* PUSH %EBP MOV %ESP %EBP instruction is found.  If the compiler is putting 
* PUSH %EBP MOV %ESP %EBP instructions
* as the first instruction of ALL subroutines, then this will reliably find
* the start of the routine.  However, some compilers allow routines, especially
* "leaf" routines that don't call any other routines, to NOT have stack frames,
* which will cause this search to fail.
*
* In either of the above cases, the value is bounded by the nearest
* routine in the system symbol table, if there is one.
* If neither method returns a legitimate value, then the value from the
* symbol table is use.  Note that the routine may not be in the
* symbol table if it is LOCAL, etc.
*
* Note that the start of a routine that is not called by call-direct and
* doesn't start with a PUSH %EBP MOV %ESP %EBP and isn't in the symbol table,
* may not be possible to locate.
*/

LOCAL INSTR * trcFindFuncStart
    (
    int *    fp,              /* frame pointer resulting from function call */
    INSTR *  pc               /* address somewhere within the function */
    )
    {
    INSTR *  ip;              /* instruction pointer */
    INSTR *  minPc;           /* lower bound on program counter */
    int      val;             /* address gotten from symbol table */
    SYM_TYPE type;            /* type associated with val */
    char *   pName = NULL;    /* string associated with val */
    int      ix    = 0;

    /*
     * if there is a symbol table, use value from table that's <= pc as
     * lower bound for function start 
     */

    minPc = NULL;

    if ((sysSymTbl != NULL) && (symByValueFind
        (sysSymTbl, (int) pc, &pName, &val, &type) == OK))
        {
        minPc = (INSTR *) val;
        }

    if (pName != NULL)
        {
        free (pName);  /* new API requires this */
        }

    /* try to find current function by looking up call */

    if (fp != NULL)    /* frame pointer legit? */
        {
        ip = trcFindCall ((INSTR *) *(fp + 1));
        if (ip != NULL)
            {
            ip = trcFindDest (ip);
            if ((ip != NULL) && (ip >= minPc) && (ip <= pc))
                return (ip);
            }
        }

    /* search backward for PUSH %EBP MOV %ESP %EBP */

    for (; pc >= minPc; --pc)
        {
        /* XXX 
         * it often causes an exception in excTask then shell doesn't restart.
         * ip = trcFollowJmp (pc);
         */
        ip = pc;

        if ((DSM(ip,   PUSH_EBP, PUSH_EBP_MASK) &&
             DSM(ip+1, MOV_ESP0, MOV_ESP0_MASK) &&
             DSM(ip+2, MOV_ESP1, MOV_ESP1_MASK)) ||

            (DSM(ip,   ENTER,    ENTER_MASK)))
            {
            return (pc);             /* return address of PUSH or ENTER */
            }

        if (ix++ > MAX_LOOPCOUNT)    /* XXX */
            {
            break;
            }
        }

    return (minPc);   /* return nearest symbol in symtbl */
    }
/*********************************************************************
*
* trcFollowJmp - resolve any JMP instructions to final destination
*
* This routine returns a pointer to the next non-JMP instruction to be
* executed if the pc were at the specified <adrs>.  That is, if the instruction
* at <adrs> is not a JMP, then <adrs> is returned.  Otherwise, if the
* instruction at <adrs> is a JMP, then the destination of the JMP is
* computed, which then becomes the new <adrs> which is tested as before.
* Thus we will eventually return the address of the first non-JMP instruction
* to be executed.
*
* The need for this arises because compilers may put JMPs to instructions
* that we are interested in, instead of the instruction itself.  For example,
* optimizers may replace a stack pop with a JMP to a stack pop.  Or in very
* UNoptimized code, the first instruction of a subroutine may be a JMP to
* a PUSH %EBP MOV %ESP %EBP, instead of a PUSH %EBP MOV %ESP %EBP (compiler
* may omit routine "post-amble" at end of parsing the routine!).  We call
* this routine anytime we are looking for a specific kind of instruction,
* to help handle such cases.
*
* RETURNS: address that chain of branches points to.
*/

LOCAL INSTR * trcFollowJmp
    (
    INSTR * addr
    )
    {
    int     displacement;        /* PC relative offset */
    int     length;              /* instruction length */

    /* while instruction is a JMP, get destination adrs */

    while (DSM(addr, JMPD08, JMPD08_MASK) ||
           DSM(addr, JMPD32, JMPD32_MASK))
        {
        if (DSM(addr, JMPD08, JMPD08_MASK))
            {
            displacement = *(char *)(addr + 1);
            length = 2;
            addr = (INSTR *) (addr + length + displacement);
            }
        else if (DSM(addr, JMPD32, JMPD32_MASK))
            {
            displacement = *(int *)(addr + 1);
            length = 5;
            addr = (INSTR *) (addr + length + displacement);
            }
        }

    return (addr);
    }
