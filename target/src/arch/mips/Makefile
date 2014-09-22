# Makefile - makefile for vw/src/arch/mips
#
# This file has been developed or significantly modified by the
# MIPS Center of Excellence Dedicated Engineering Staff.
# This notice is as per the MIPS Center of Excellence Master Partner
# Agreement, do not remove this notice without checking first with
# WR/Platforms MIPS Center of Excellence engineering management.
#
# modification history
# --------------------
# 02g,17may02,zmm  Global au1000 name changes. SPR 77333.
# 02g,22may02,jmt  Modify _tx49xx to point to cacheTx49Lib
# 02f,08may02,pes  Change MIPS r3xxx CPU_VARIANT designation to rc3000
# 02e,23apr02,pes  Add support for rc4xxx, rc5xxx, rm52xx, rm57xx, tx49xx and
#		   vr50xx processsor families.
# 02d,08mar02,pes  Remove CW4011 cache library from r3xxx processor variant
#                  archive
# 02c,27feb02,pes  Adjust libraries included for r3xxx processor family to
#                  include additional cache libraries.
# 02b,29jan02,pes  Add diab assembler switches for vr5432
# 02a,21jan02,tlc  Add EXTRA_DEFINE to the CPU_FAMILY bcm125x for "load
#                  linked-store conditionall" support.
# 01z,11jan02,pes  Support repackaged libarch.a archives
# 01y,02jan02,tlc  Add Vr4131 support.
# 01x,11dec01,pad  Added compilation of elfMips.o
# 01w,07dec01,mem  Added specialized build(s) for the NEC Vr5432.
# 01v,18nov01,agf  Added Broadcom Sb1 cache libs
# 01w,26oct01,tam  updated for re-packaging
# 01v,19oct01,tlc  Add conditional compilation to "-Wa, -mcpu=vr5400" compiler
#                  option for Diab.
# 01u,01aug01,mem  Diab integration
# 01t,16jul01,ros  add CofE comment
# 01s,09jul01,mem  Added cacheAu1000Lib
# 01r,07jun01,mem  Re-enable optimized kernel.
# 01q,05jun01,mem  Added Vr5432 support library.
# 01p,05jun01,agf  add intVectorALib.o to build list
# 01o,24apr01,mem  Add 5400 flags for MIPS64 builds.
# 01n,09apr01,mem  Added cacheR32k* modules, cleanup.
# 01m,28mar01,sru  add cacheCW400x library for R3K
# 01l,05jan01,pes  Adjust cache object modules to be included in
#                  MIPS32sfr3kgnu library.
# 01k,03jan01,pes  Fix cache module include breakage.
# 01j,03jan01,pes  Make cache libraries part of standard archive
# 01i,03jan01,pes  Add cache4kcAlib.o and cache4kcLib.o to list of build
#                  targets
# 01h,02jan01,tlc  Correct duplicate cacheR10kLib.o to cacheR10kALib.o.
# 01g,27dec00,roz  add cacheR7kLib for MIPS64
# 01f,20dec00,pes  Add rules for MIPS32 and MIPS64 targets.
# 01e,08sep99,myz  Added CW4000_16
# 01d,29jul,alp    Added CW4000 and CW4010 support.
# 01f,13jul99,dra  added cacheR5kLib.o and cacheR5kALib.o to VR5400.
# 01e,08jul99,dra  added cacheR5kLib.o and cacheR5kALib.o to VR5000.
# 01d,18jan99,dra  added CW4000, CW4011, VR4100, VR5000 and VR5400 support.
# 01d,26jul00,dra  Added QED Rm7000 cache support.
# 01c,08jan98,dbt  removed wdbArchLib.o, wdbALib.o and dbgALib.o. Added
#                  wdbDbgArchLib.o and wdbDbgALib.o
# 01b,16dec96,kkk  added R4650 support.
# 01a,18jun96,yp   created from 01q of MakeSkel
#
# DESCRIPTION
# This file contains the makefile rules for building the vx library
#*/

TGT_DIR=$(WIND_BASE)/target

DOC_FILES =

LIB_BASE_NAME	= arch

# Optimized modules
OBJS_OPT = \
	bALib.o \
	dllALib.o \
	qPriBMapALib.o \
	semALib.o \
	semCALib.o \
	semMALib.o \
	sllALib.o \
	workQALib.o

# Common modules
OBJS_COMMON = \
	dbgArchLib.o \
	dsmLib.o \
	elfMips.o \
	excALib.o \
	excArchLib.o \
	excArchShow.o \
	fppALib.o \
	fppArchLib.o \
	intALib.o \
	intArchLib.o \
	taskArchLib.o \
	trcLib.o \
	unixALib.o \
	vxALib.o \
	vxLib.o \
	windALib.o \
	sigCtxLib.o \
	sigCtxALib.o \
	wdbDbgALib.o \
	wdbDbgArchLib.o

# Generic cache support
OBJS_PAL	+= cacheMipsLib.o

ifeq ($(findstring au1xxx,$(LIB_DIR_TAG)),au1xxx)
#
# Specialization of the build for the Alchemy au1xxx
#
OBJS_PAL	+= cacheAuLib.o cacheAuALib.o
endif

ifeq ($(findstring bcm125x,$(LIB_DIR_TAG)),bcm125x)
#
# Specialization of the build for the Broadcom bcm125x 
#

# Install support for load-linked, store-conditional instructions 
EXTRA_DEFINE	+= -D_WRS_MIPS_LL_SC

# bcm125x support modules 
OBJS_PAL	+= cacheSb1Lib.o cacheSb1ALib.o

endif

ifeq ($(findstring mti4kx,$(LIB_DIR_TAG)),mti4kx)
#
# Specialization of the build for the MTI 4kx
#
OBJS_PAL	+= cache4kcLib.o cache4kcALib.o
endif

ifeq ($(findstring mti5kx,$(LIB_DIR_TAG)),mti5kx)
#
# Specialization of the build for the MTI 5kx
#
OBJS_PAL	+= cache4kcLib.o cache4kcALib.o
endif

ifeq ($(findstring rc32xxx,$(LIB_DIR_TAG)),rc32xxx)
#
# Specialization of the build for the IDT rc32xxx
#
OBJS_PAL	+= cacheR32kLib.o cacheR32kALib.o
endif

ifeq ($(findstring rc4xxx,$(LIB_DIR_TAG)),rc4xxx)
#
# Specialization of the build for the rc4xxx family
#
OBJS_PAL	+= cacheR4kLib.o cacheR4kALib.o
endif

ifeq ($(findstring rc5xxx,$(LIB_DIR_TAG)),rc5xxx)
#
# Specialization of the build for the rc5xxx family
#
OBJS_PAL	+= cacheR5kLib.o cacheR5kALib.o
endif

ifeq ($(findstring rm52xx,$(LIB_DIR_TAG)),rm52xx)
#
# Specialization of the build for the rm52xx family
#
OBJS_PAL	+= cacheR5kLib.o cacheR5kALib.o
endif

ifeq ($(findstring rm57xx,$(LIB_DIR_TAG)),rm57xx)
#
# Specialization of the build for the rm57xx family
#
OBJS_PAL	+= cacheR5kLib.o cacheR5kALib.o
endif

ifeq ($(findstring rm7xxx,$(LIB_DIR_TAG)),rm7xxx)
#
# Specialization of the build for the PMC-Sierra RM7xxx
#
OBJS_PAL	+= cacheR7kLib.o cacheR7kALib.o \
		   intVectorALib.o
endif

ifeq ($(findstring tx49xx,$(LIB_DIR_TAG)),tx49xx)
#
# Specialization of the build for the Toshiba tx49xx
#
OBJS_PAL	+= cacheTx49Lib.o cacheTx49ALib.o
endif

ifeq ($(findstring vr41xx,$(LIB_DIR_TAG)),vr41xx)
#
# Specialization of the build for the NEC Vr41xx
#
OBJS_PAL	+= cacheR4kLib.o cacheR4kALib.o \
		   cacheVr4131Lib.o cacheVr4131ALib.o
endif

ifeq ($(findstring vr50xx,$(LIB_DIR_TAG)),vr50xx)
#
# Specialization of the build for the vr50xx family
#
OBJS_PAL	+= cacheR5kLib.o cacheR5kALib.o
endif

ifeq ($(findstring vr54xx,$(LIB_DIR_TAG)),vr54xx)
#
# Specialization of the build for the NEC Vr5432
#

# Install workaround for Vr5432 branch-prediction errata
EXTRA_DEFINE	+= -D_WRS_MIPS_VR5400_ERRATA

# Vr5432 support modules
OBJS_PAL 	+= vr5432Lib.o vr5432ALib.o cacheR10kLib.o cacheR10kALib.o

# options needed for gnu build only
ifeq ($(findstring gnu,$(TOOL)),gnu)
CFLAGS_AS_vr5432ALib.o = -Wa,-mcpu=vr5400
endif
# options needed for diab build only
ifeq ($(findstring diab,$(TOOL)),diab)
CFLAGS_AS_vr5432ALib.o = -Wa,-Xcpu-R5432
endif
endif

ifeq ($(findstring vr55xx,$(LIB_DIR_TAG)),vr55xx)
#
# Specialization of the build for the NEC Vr55xx
#
OBJS_PAL	+= cacheR10kLib.o cacheR10kALib.o
endif

ifeq ($(findstring rc3000,$(LIB_DIR_TAG)),rc3000)
#
# Specialization of the build for r3k devices
#
OBJS_PAL	+= cacheCW400xLib.o cacheCW400xALib.o \
		   cacheR3kLib.o cacheR3kALib.o 
endif


OBJS	= $(OBJS_COMMON) $(OBJS_OPT) $(OBJS_PAL)

include $(TGT_DIR)/h/make/rules.library
