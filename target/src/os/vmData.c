/* vmData.c - data exported by either vmLib or vmBaseLib */

/* Copyright 1984-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01c,16nov01,to   declare global variables in vmData.c to avoid getting multiple
                 vm*Lib.o pulled
01b,18nov93,edm  changed BOOL mmuPhysAddrShifted to int mmuPhysAddrShift.
01a,05apr93,edm  written.
    26apr93,ccc  SPECIAL for sunec.
*/

/* 
This file merely declares the data exported by both vmLib and vmBaseLib.
The data is required to be here, and not in either vmLib or vmBaseLib, so
that the proper level of MMU support is extracted from the library file
linked in the BSP directories.

SEE ALSO: vmLib, vmBaseLib, vmShow, mmuLib
*/

#include "vxWorks.h"
#include "private/vmLibP.h"

int mmuPageBlockSize;                           /* initialized by mmuLib.c */
STATE_TRANS_TUPLE *mmuStateTransArray;          /* initialized by mmuLib.c */
int mmuStateTransArraySize;                     /* initialized by mmuLib.c */
MMU_LIB_FUNCS mmuLibFuncs;                      /* initialized by mmuLib.c */

int mmuPhysAddrShift = 0;                    /* may be altered by mmuLib.c */
