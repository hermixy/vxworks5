# Makefile - makefile for vw/src/arch/i86
#
# modification history
# --------------------
# 01i,12jun02,hdn  added vmArch3[26]Lib.o and vmBaseArch3[26]Lib.o
# 01h,28mar02,hdn  added ipiArchLib and ipiALib for PENTIUM2/3/4
# 01g,25oct01,tam  updated for repackaging
# 01f,15oct01,pad  Added compilation of elfI86.c
# 01e,04sep01,hdn  renamed mmu[A]Lib.o to mmuI86[A]Lib.o
# 01d,16aug01,hdn  added PENTIUM2/3/4 support
# 01c,09apr98,hdn  added support for Pentium.
# 01b,08jan98,dbt  removed wdbArchLib.o, wdbALib.o and dbgALib.o. Added
#                  wdbDbgArchLib.o and wdbDbgALib.o
# 01a,18jun96,yp   created from 01c of MakeSkel
#
# DESCRIPTION
# This file contains the makefile rules for building the vx library
#
#*/

TGT_DIR=$(WIND_BASE)/target

DOC_FILES =

LIB_BASE_NAME   = arch

OBJS_COMMON	= bALib.o cacheALib.o cacheArchLib.o \
	     	  dbgArchLib.o dllALib.o dsmLib.o excALib.o excArchLib.o \
	     	  excArchShow.o ffsALib.o fppALib.o fppArchLib.o intALib.o \
	     	  intArchLib.o qPriBMapALib.o qPriHeapALib.o semALib.o \
		  semCALib.o semMALib.o sllALib.o taskArchLib.o trcLib.o \
		  vxmIfLib.o vxALib.o vxLib.o windALib.o workQALib.o \
		  sigCtxLib.o sigCtxALib.o unixALib.o wdbDbgArchLib.o \
		  wdbDbgALib.o vxShow.o elfI86.o

OBJS_I80386	= $(OBJS_COMMON) mmuI86Lib.o mmuI86ALib.o mmuPro32Lib.o \
		  mmuPro36Lib.o mmuPro32ALib.o mmuPro36ALib.o \
		  pentiumLib.o pentiumALib.o pentiumShow.o vmBaseArch32Lib.o \
		  vmBaseArch36Lib.o vmArch32Lib.o vmArch36Lib.o

OBJS_I80486	= $(OBJS_COMMON) mmuI86Lib.o mmuI86ALib.o mmuPro32Lib.o \
		  mmuPro36Lib.o mmuPro32ALib.o mmuPro36ALib.o \
		  pentiumLib.o pentiumALib.o pentiumShow.o vmBaseArch32Lib.o \
		  vmBaseArch36Lib.o vmArch32Lib.o vmArch36Lib.o

OBJS_PENTIUM	= $(OBJS_COMMON) mmuI86Lib.o mmuI86ALib.o mmuPro32Lib.o \
		  mmuPro36Lib.o mmuPro32ALib.o mmuPro36ALib.o \
		  pentiumLib.o pentiumALib.o pentiumShow.o vmBaseArch32Lib.o \
		  vmBaseArch36Lib.o vmArch32Lib.o vmArch36Lib.o

OBJS_PENTIUM2	= $(OBJS_COMMON) mmuI86Lib.o mmuI86ALib.o mmuPro32Lib.o \
		  mmuPro36Lib.o mmuPro32ALib.o mmuPro36ALib.o \
		  pentiumLib.o pentiumALib.o pentiumShow.o \
		  ipiALib.o ipiArchLib.o vmBaseArch32Lib.o \
		  vmBaseArch36Lib.o vmArch32Lib.o vmArch36Lib.o

OBJS_PENTIUM3	= $(OBJS_COMMON) mmuI86Lib.o mmuI86ALib.o mmuPro32Lib.o \
		  mmuPro36Lib.o mmuPro32ALib.o mmuPro36ALib.o \
		  pentiumLib.o pentiumALib.o pentiumShow.o \
		  ipiALib.o ipiArchLib.o vmBaseArch32Lib.o \
		  vmBaseArch36Lib.o vmArch32Lib.o vmArch36Lib.o

OBJS_PENTIUM4	= $(OBJS_COMMON) mmuI86Lib.o mmuI86ALib.o mmuPro32Lib.o \
		  mmuPro36Lib.o mmuPro32ALib.o mmuPro36ALib.o \
		  pentiumLib.o pentiumALib.o pentiumShow.o \
		  ipiALib.o ipiArchLib.o vmBaseArch32Lib.o \
		  vmBaseArch36Lib.o vmArch32Lib.o vmArch36Lib.o

OBJS=$(OBJS_$(CPU))

include $(TGT_DIR)/h/make/rules.library

