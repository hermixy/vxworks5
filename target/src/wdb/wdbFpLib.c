/* wdbFpLib.c - floating point register support for the external WDB agent */

/* Copyright 1984-2001 Wind River Systems, Inc. */

/*
modification history
--------------------
01d,21aug01,hdn imported PENTIUM2/3/4 support from T31 ver 01e 
01c,17dec96,ms  WDB now uses FP_CONTEXT instead of FPREG_SET (SPR 7654).
01b,23jan96,tpr added cast to compile with Diab Data tools.
01a,25may95,ms  written.
*/

/*
DESCPRIPTION

This library contains routines to save, restore, get, and
set the floating point registers. These operations are
not task-specific.
*/

#include "wdb/wdbRegs.h"
#include "string.h"

#if	(CPU_FAMILY == I80X86)
static struct
    {
    WDB_FPU_REGS	fpContext;	/* the hardware context */
    WDB_REG_SET_OBJ	regSet;		/* generic register set */
    } WRS_DATA_ALIGN_BYTES(_CACHE_ALIGN_SIZE) fpRegSetObj =
		  {{{{0}}}, {{0}}};	/* it must be in data section for now */
#else
static struct
    {
    WDB_REG_SET_OBJ	regSet;		/* generic register set */
    WDB_FPU_REGS	fpContext;	/* the hardware context */
    } fpRegSetObj;
#endif	/* (CPU_FAMILY == I80X86) */


/******************************************************************************
*
* wdbFppSave - save the floating point registers.
*/ 

void wdbFppSave (void)
    {
#if	((CPU == PENTIUM) || (CPU == PENTIUM2) || (CPU == PENTIUM3) || \
	 (CPU == PENTIUM4))
    (*_func_fppSaveRtn) (&fpRegSetObj.fpContext);
#else	/* ((CPU == PENTIUM) || (CPU == PENTIUM[234])) */
    fppSave (&fpRegSetObj.fpContext);
#endif	/* ((CPU == PENTIUM) || (CPU == PENTIUM[234])) */
    }

/******************************************************************************
*
* wdbFppRestore - restore the previously saved float regs.
*/ 

void wdbFppRestore (void)
    {
#if	((CPU == PENTIUM) || (CPU == PENTIUM2) || (CPU == PENTIUM3) || \
	 (CPU == PENTIUM4))
    (*_func_fppRestoreRtn) (&fpRegSetObj.fpContext);
#else	/* ((CPU == PENTIUM) || (CPU == PENTIUM[234])) */
    fppRestore (&fpRegSetObj.fpContext);
#endif	/* ((CPU == PENTIUM) || (CPU == PENTIUM[234])) */
    }

/******************************************************************************
*
* wdbFppGet - get a pointer to the fpp reg block.
*/ 

void wdbFppGet
    (
    void ** ppRegs
    )
    {
    *ppRegs = (void *)&fpRegSetObj.fpContext;
    }

/******************************************************************************
*
* wdbFppSet - set the floating point reg block.
*/ 

void wdbFppSet
    (
    void * pRegs
    )
    {
    bcopy ((char *)pRegs, (char *)&fpRegSetObj.fpContext, sizeof (FP_CONTEXT));
    }

/******************************************************************************
*
* wdbFpObjInit - initialize a WDB_REG_SET_OBJ representing float regs.
*
* RETURNS: a pointer to a WDB_REG_SET_OBJ
*/ 

WDB_REG_SET_OBJ * wdbFpLibInit (void)
    {
    WDB_REG_SET_OBJ * pRegSet = &fpRegSetObj.regSet;

    pRegSet->regSetType	= WDB_REG_SET_FPU;
    pRegSet->save	= wdbFppSave;
    pRegSet->load	= wdbFppRestore;
    pRegSet->get	= (void (*) (char **)) wdbFppGet;
    pRegSet->set	= (void (*) (char *))  wdbFppSet;
#if	((CPU == PENTIUM) || (CPU == PENTIUM2) || (CPU == PENTIUM3) || \
	 (CPU == PENTIUM4))
    (*_func_fppSaveRtn) (&fpRegSetObj.fpContext);
#else	/* ((CPU == PENTIUM) || (CPU == PENTIUM[234])) */
    fppSave (&fpRegSetObj.fpContext);
#endif	/* ((CPU == PENTIUM) || (CPU == PENTIUM[234])) */
    return (pRegSet);
    }

