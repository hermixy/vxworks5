# Makefile - makefile for target/src/arch/mips/math
#
# This file has been developed or significantly modified by the
# MIPS Center of Excellence Dedicated Engineering Staff.
# This notice is as per the MIPS Center of Excellence Master Partner
# Agreement, do not remove this notice without checking first with
# WR/Platforms MIPS Center of Excellence engineering management.
#
# modification history
# --------------------
# 01n,10dec01,mem  Added support for specialized Vr5432 build.
# 01m,26oct01,tam  updated for re-packaging
# 01l,16jul01,ros  add CofE comment
# 01k,24apr01,mem  Add mathHardLib.c for sf builds.
# 01j,19apr01,roz  change MIPS64gnu5400le to MIPS64gnule
# 01i,10jan01,pes  Add floating point components for all new MIPS
#                  combinations.
# 01h,28dec00,zmm  add support for MIPS32sfgnu
# 01g,27dec00,roz  add support for MIPS64
# 01f,07sep99,myz  added OBJ_CW4000_16gnu for mips16 support.
# 01e,19jan99,dra  added CW4000, CW4011, VR4100, VR5000 and VR5400 support.
# 01d,14jan97,kkk  added little endian support.
# 01c,17dec96,kkk  fixed OBJS, use $(CPU) instead of <CPU>.
# 01b,16dec96,kkk  split up the object modules for the different CPUs.
# 01a,12jul96,yp   created from 01c of MakeSkel
#
# DESCRIPTION
# This file contains the makefile rules for building the vx library
#
#*/

TGT_DIR=$(WIND_BASE)/target

LIB_BASE_NAME	= arch

#
# Specialization of the build for the NEC Vr5432
#
ifeq ($(findstring 5432,$(LIB_DIR_TAG)),5432)

# Install workaround for Vr5432 branch-prediction errata
EXTRA_DEFINE	+= -D_WRS_MIPS_VR5400_ERRATA

endif

ifeq ($(findstring sf,$(TOOL)),sf)
OBJS_MATH = mathHardLib.o mathSoftLib.o
else
OBJS_MATH = mathHardLib.o mathHardALib.o
endif

OBJS	= $(OBJS_MATH) fpSoft.o fpIntr.o

include $(TGT_DIR)/h/make/rules.library

