# Makefile - makefile for target/src/arch/arm
#
# modification history
# --------------------
# 01u,14dec01,to   sync with T3. clean up. bring back lost mod-hist.
# 01t,05dec01,rec  ARCHV5 changes
# 01s,13nov01,to   add cacheArchVars and mmuArchVars
# 01r,23oct01,jn   put arm elf reloc unit into arch directory
# 01s,09nov01,t_m  Update for ARMARCH5
# 01r,25oct01,tam  updated for repackaging
# 01s,09oct01,rec  archive OBJS_ARMARCH5_COMMON files in OBJS_ARMARCH4
# 01r,25sep01,rec  archv5 changes
# 01q,23jul01,scm  change XScale name to conform to coding standards...
# 01p,04may01,scm  add support for STRONGARM...
# 01o,11dec00,scm  replaces references to ARMSA2 with XScale
# 01n,31aug00,scm  add initial sa2 support
# 01n,07mar01,t_m  merge in XScale changes
# 01m,19jul00,jpd  added ARM946E support.
# 01l,30jul98,jpd  added ARM740T, ARM920T support.
# 01k,20jan99,cdp  removed support for old ARM libraries.
# 01j,24nov98,cdp  added support for generic ARMARCH3/4/4_T;
#	      jpd  added cacheALib2.o, mmuALib2.o and mmuMapLib.o.
# 01i,20apr98,dbt  replaced wdbArchLib.o with wdbDbgArchLib.o
# 01h,20mar98,cdp  undo 01g (add sem*Lib and workQALib back to optimised list).
# 01g,21oct97,kkk  take out sem*Lib.o and workQLib.o from optimized list.
# 01f,29may97,jpd  Finished adding assembler optimised files.
# 01e,22apr97,jpd  Removed assembler optimised files from ARM7TDMI_T.
# 01d,16apr97,cdp  Added support for ARM7TDMI_T.
# 01c,11apr97,jpd  Added assembler optimised files.
# 01b,29jan97,jpd  Added cacheArchLib.o for ARM7TDMI.
# 01a,28nov96,cdp  created from 01a of MakeSkel.
#
# DESCRIPTION
# This file contains the makefile rules for building the VxWorks
# architecture-dependent library.
#

TGT_DIR=$(WIND_BASE)/target

DOC_FILES =

LIB_BASE_NAME   = arch

OBJS_COMMON = \
		dbgArchLib.o dbgArmLib.o dsmLib.o \
		elfArm.o excALib.o excArchLib.o excArchShow.o \
		fppArchLib.o intALib.o intArchLib.o \
		sigCtxALib.o sigCtxLib.o taskArchLib.o trcLib.o \
		vxALib.o vxLib.o wdbDbgArchLib.o windALib.o

OBJS_ARM_OPTIM = \
		bALib.o dllALib.o ffsALib.o \
		qPriBMapALib.o qPriHeapALib.o \
		semALib.o semCALib.o semMALib.o \
		sllALib.o unixALib.o workQALib.o

OBJS_ARMARCH4_COMMON = \
		cacheArchVars.o cacheALib2.o \
		cache720tLib.o cache720tALib.o \
		cache740tLib.o cache740tALib.o \
		cache810Lib.o cache810ALib.o \
		cache920tLib.o cache920tALib.o \
		cache940tLib.o cache940tALib.o \
		mmuArchVars.o mmuALib2.o mmuMapLib.o \
		mmu720tLib.o mmu720tALib.o \
		mmu740tLib.o mmu740tALib.o \
		mmu810Lib.o mmu810ALib.o \
		mmu920tLib.o mmu920tALib.o \
		mmu940tLib.o mmu940tALib.o

OBJS_ARMARCH5_COMMON = \
		cacheArchVars.o cacheALib2.o \
		cache946eLib.o cache946eALib.o \
		mmuArchVars.o mmuALib2.o mmuMapLib.o \
		mmu946eLib.o mmu946eALib.o

OBJS_STRONGARM_COMMON = \
		cacheArchVars.o cacheALib2.o \
		cacheSA110Lib.o cacheSA110ALib.o \
		cacheSA1100Lib.o cacheSA1100ALib.o \
		cacheSA1500Lib.o cacheSA1500ALib.o \
		mmuArchVars.o mmuALib2.o mmuMapLib.o \
		mmuSA110Lib.o mmuSA110ALib.o \
		mmuSA1100Lib.o mmuSA1100ALib.o \
		mmuSA1500Lib.o mmuSA1500ALib.o

OBJS_XSCALE_COMMON = \
		cacheArchVars.o cacheALib2.o \
		cacheXSCALELib.o cacheXSCALEALib.o \
		mmuArchVars.o mmuALib2.o mmuMapLib.o \
		mmuXSCALELib.o mmuXSCALEALib.o

OBJS_ARMARCH4	= $(OBJS_COMMON) $(OBJS_ARM_OPTIM) $(OBJS_ARMARCH4_COMMON)

OBJS_ARMARCH5	= $(OBJS_COMMON) $(OBJS_ARM_OPTIM) $(OBJS_ARMARCH5_COMMON)

OBJS_STRONGARM	= $(OBJS_COMMON) $(OBJS_ARM_OPTIM) $(OBJS_STRONGARM_COMMON)

OBJS_XSCALE	= $(OBJS_COMMON) $(OBJS_ARM_OPTIM) $(OBJS_XSCALE_COMMON)

OBJS_ARMARCH4_T	= $(OBJS_COMMON) $(OBJS_ARMARCH4_COMMON)

OBJS_ARMARCH5_T	= $(OBJS_COMMON) $(OBJS_ARMARCH5_COMMON)

OBJS=$(OBJS_$(CPU))

include $(TGT_DIR)/h/make/rules.library
