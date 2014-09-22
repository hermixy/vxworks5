/* wdbAltivecLib.c - Altivec register support for the external WDB agent */

/* Copyright 1984-1996 Wind River Systems, Inc. */

/*
modification history
--------------------
01h,30apr02,pcs  Remove the pointer corruption in fn. wdbAltivecLibInit
01g,07jun01,kab  Testing only
01f,29may01,kab  Hide altivecLib.h from non-altivec builds
01e,13apr01,pcs  Move decleration of altivecRegSetObj to
                 wdb/wdbRegs.h
Allocated mem for ALTIVEC_REG_SET_OBJ at init
                 time and make sure that
it is 16 byte aligned.
01d,23mar01,dtr  Tidying up.
01c,19mar01,tpw  Rename WDB_REG_SET_ALTIVEC to WDB_REG_SET_AV for consistency
                 with WTX and other REG_SET names.
01b,19mar01,dtr  Changing header file name altiVecLib.h.
01a,01mar01,dtr Created library.
*/

/*
DESCPRIPTION

This library contains routines to save, restore, get, and
set the altivec registers. These operations are
not task-specific.
*/
#include "vxWorks.h"

#ifdef  _WRS_ALTIVEC_SUPPORT

#include "wdb/wdbRegs.h"
#include "string.h"
#include "altivecLib.h"
/* 
 * The ALTIVEC_REG_SET_OBJ is defined as a pointer and mem for it allocated at
 * run time. The reason for this is to ensure that this object especially the
 * first element altivecContext is 16 byte aligned.
 */

ALTIVEC_REG_SET_OBJ * pAltivecRegSetObj;


/******************************************************************************
*
* wdbAltivecSave - save the altivec registers.
*/ 

void wdbAltivecSave (void)
    {
    altivecSave (&pAltivecRegSetObj->altivecContext);
    }

/******************************************************************************
*
* wdbAltivecRestore - restore the previously saved altivec regs.
*/ 

void wdbAltivecRestore (void)
    {
    altivecRestore (&pAltivecRegSetObj->altivecContext);
    }

/******************************************************************************
*
* wdbAltivecGet - get a pointer to the altivec reg block.
*/ 

void wdbAltivecGet
    (
    void ** ppRegs
    )
    {
    *ppRegs = (void *) &pAltivecRegSetObj->altivecContext;
    }

/******************************************************************************
*
* wdbAltivecSet - set the Altivec reg block.
*/ 

void wdbAltivecSet
    (
    void * pRegs
    )
    {
    bcopy ((char *)pRegs, (char *) &pAltivecRegSetObj->altivecContext, sizeof (ALTIVEC_CONTEXT));
    }

/******************************************************************************
*
* wdbAltivecLibInit - initialize a WDB_REG_SET_OBJ representing altivec regs.
*
* RETURNS: a pointer to a WDB_REG_SET_OBJ
*/ 

WDB_REG_SET_OBJ * wdbAltivecLibInit (void)
    {

    WDB_REG_SET_OBJ * pRegSet;


    /*  Ensure that the ALTIVEC_REG_SET_OBJ object is 16 byte aligned. This is with the assumption
        that altiveccontext is the first element in the object.
     */
    pAltivecRegSetObj = (ALTIVEC_REG_SET_OBJ *) memalign (16, sizeof (ALTIVEC_REG_SET_OBJ));
    bzero ((char *)pAltivecRegSetObj, sizeof (ALTIVEC_REG_SET_OBJ));

    altivecSave (&pAltivecRegSetObj->altivecContext);

    pRegSet = &pAltivecRegSetObj->regSet;
    pRegSet->regSetType	= WDB_REG_SET_AV;
    pRegSet->save	= wdbAltivecSave;
    pRegSet->load	= wdbAltivecRestore;
    pRegSet->get	= (void (*) (char **)) wdbAltivecGet;
    pRegSet->set	= (void (*) (char *))  wdbAltivecSet;

    return (pRegSet);
    }

#endif /* (_WRS_ALTIVEC_SUPPORT==TRUE) */



