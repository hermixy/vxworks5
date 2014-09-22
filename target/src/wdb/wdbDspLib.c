/* wdbDspLib.c - DSP register support for the external WDB agent */

/* Copyright 1984-1998 Wind River Systems, Inc. */

/*
modification history
--------------------
01a,31jul98,kab created
*/

/*
DESCPRIPTION

This library contains routines to save, restore, get, and
set the DSP registers. These operations are not task-specific.
*/

#include "wdb/wdbRegs.h"
#include "string.h"

static struct
    {
    WDB_REG_SET_OBJ	regSet;		/* generic register set */
    WDB_DSP_REGS	dspContext;	/* the hardware context */
    } dspRegSetObj;

/******************************************************************************
*
* wdbDspSave - save the DSP registers.
*/ 

void wdbDspSave (void)
    {
    dspSave (&dspRegSetObj.dspContext);
    }

/******************************************************************************
*
* wdbDspRestore - restore the previously saved DSP regs.
*/ 

void wdbDspRestore (void)
    {
    dspRestore (&dspRegSetObj.dspContext);
    }

/******************************************************************************
*
* wdbDspGet - get a pointer to the DSP reg block.
*/ 

void wdbDspGet
    (
    void ** ppRegs
    )
    {
    *ppRegs = (void *)&dspRegSetObj.dspContext;
    }

/******************************************************************************
*
* wdbDspSet - set the DSP reg block.
*/ 

void wdbDspSet
    (
    void * pRegs
    )
    {
    bcopy ((char *)pRegs, (char *)&dspRegSetObj.dspContext, sizeof (DSP_CONTEXT));
    }

/******************************************************************************
*
* wdbDspObjInit - initialize a WDB_REG_SET_OBJ representing DSP regs.
*
* RETURNS: a pointer to a WDB_REG_SET_OBJ
*/ 

WDB_REG_SET_OBJ * wdbDspLibInit (void)
    {
    WDB_REG_SET_OBJ * pRegSet = &dspRegSetObj.regSet;

    pRegSet->regSetType	= WDB_REG_SET_DSP;
    pRegSet->save	= wdbDspSave;
    pRegSet->load	= wdbDspRestore;
    pRegSet->get	= (void (*) (char **)) wdbDspGet;
    pRegSet->set	= (void (*) (char *))  wdbDspSet;
    dspSave (&dspRegSetObj.dspContext);
    return (pRegSet);
    }

