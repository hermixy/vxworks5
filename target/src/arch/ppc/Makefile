# Makefile - makefile for vw/src/arch/ppc
#
# modification history
# --------------------
# 01j,14apr02,jtp  add PPC440 mmu support
# 01i,19nov01,pch  Add PPC440 support
# 01h,24oct01,tam  updated for repackaging
# 01g,23oct01,jab  use make variable instead of gnu-specific -fvec switch
# 01f,02apr01,kab  Custom rule for altivecALib.o
# 01e,27mar01,pcs  ADD altivecALib.o to OBJ_PPC604
# 01d,13mar01,pcs  ADDED altivec related files.
# 01f,26oct00,s_m  renamed PPC405GP and PPC405GF
# 01e,06jul00,alp  Added PPC405 support
# 01d,15mar99,zl   added PowerPC 509 and PowerPC 555 support
# 01c,29oct98,elg  added mmuArchVars.c
# 01b,18aug98,tpr  added PowerPC EC 603 support.
# 01b,09jan98,dbt  removed wdbALib.o, wdbArchLib.o and dbgALib.o. Added
#                  wdbDbgALib.o and wdbDbgArchLib.o
# 01a,18jun96,yp   created from 01m of MakeSkel
#
# DESCRIPTION
# This file contains the makefile rules for building the vx library
#
#*/

TGT_DIR=$(WIND_BASE)/target

DOC_FILES =

LIB_BASE_NAME   = arch

OBJS_COMMON= bALib.o   dsmLib.o      excALib.o    excArchLib.o excArchShow.o \
	     ffsALib.o fppALib.o     fppArchLib.o intArchLib.o intALib.o     \
	     trcLib.o  taskArchLib.o sigCtxALib.o sigCtxLib.o  vxALib.o      \
	     vxLib.o   wdbDbgALib.o  wdbDbgArchLib.o windALib.o

OBJS_PPC403	= $(OBJS_COMMON) cache403ALib.o cacheALib.o cacheArchLib.o \
			      dbgArchLib.o 

OBJS_PPC405	= $(OBJS_COMMON) cache405ALib.o cacheALib.o cacheArchLib.o \
                  dbgArchLib.o mmuArchVars.o mmu405Lib.o mmuPpcALib.o

OBJS_PPC405F	= $(OBJS_COMMON) cache405ALib.o cacheALib.o cacheArchLib.o \
                  dbgArchLib.o mmuArchVars.o mmu405Lib.o mmuPpcALib.o

OBJS_PPC440	= $(OBJS_COMMON) cache440ALib.o cacheALib.o cacheArchLib.o \
		  dbgArchLib.o mmu440Lib.o mmuPpcALib.o

OBJS_PPC505	= $(OBJS_COMMON)

OBJS_PPC509	= $(OBJS_COMMON) dbgArchLib.o

OBJS_PPC555	= $(OBJS_COMMON) dbgArchLib.o

OBJS_PPC601	= $(OBJS_COMMON) cacheALib.o    cacheArchLib.o  dbgArchLib.o \
				 mmuArchVars.o  mmuPpcALib.o    mmuPpcLib.o 

OBJS_PPC603	= $(OBJS_COMMON) cacheALib.o    cacheArchLib.o  dbgArchLib.o \
				 mmuArchVars.o  mmuPpcALib.o    mmuPpcLib.o

OBJS_PPCEC603	= $(OBJS_COMMON) cacheALib.o    cacheArchLib.o  dbgArchLib.o \
				 mmuArchVars.o  mmuPpcALib.o    mmuPpcLib.o

OBJS_PPC604	= $(OBJS_COMMON) cacheALib.o    cacheArchLib.o  dbgArchLib.o \
				 mmuArchVars.o  mmuPpcALib.o    mmuPpcLib.o  \
				 altivecArchLib.o altivecShow.o altivecLib.o \
				 altivecALib.o

OBJS_PPC860	= $(OBJS_COMMON) cache800ALib.o  cacheArchLib.o  dbgArchLib.o \
				 mmu800Lib.o     mmuArchVars.o   mmuPpcALib.o

OBJS=$(OBJS_$(CPU))

include $(TGT_DIR)/h/make/rules.library
$(LIBDIR)/altivecALib.o: altivecALib.s
	$(CC) $(CFLAGS_AS) $(OPTION_ALTIVEC_AS) $(OPTION_OBJECT_ONLY) $(OPTION_OBJECT_NAME)$@ $<

