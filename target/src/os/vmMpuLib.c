/* vmMpuLib.c - MPU virtual memory support library */

/* Copyright 1984-1998 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01a,02jul98,jpd  written.
*/

/* 
This library provides MMU (Memory Management Unit) support on a
processor with a Protection Unit style of MMU (MPU) rather than a full
page-table based MMU.

INTERNAL
The file defines the MACRO BUILD_MPU_LIB and includes the file
vmBaseLib.c, which is then compiled to produce routines with different
names (to avoid clashes with vmBaseLib.c). This is because the
libraries are very similar, and we wish to avoid having two copies of
the file, which do not differ greatly.

CONFIGURATION
To include the MPU support library in VxWorks, define INCLUDE_MMU_MPU
in config.h.  Note that the three options INCLUDE_MMU_MPU,
INCLUDE_MMU_BASIC and INCLUDE_MMU_FULL are mutually exclusive: only
one of the three can be selected.

SEE ALSO: vmBaseLib, vmLib, vmShow,
.pG "Virtual Memory"
*/

/* compile conditionally for MPUs */

#define BUILD_MPU_LIB


/* ensure the non-static routines will have different names */

#define vmBaseLibInit vmMpuLibInit
#define vmBaseGlobalMapInit vmMpuGlobalMapInit
#define vmBasePageSizeGet vmMpuPageSizeGet
#define vmBaseStateSet vmMpuStateSet
#define mutexOptionsVmBaseLib mutexOptionsVmMpuLib

#include "vmBaseLib.c"

