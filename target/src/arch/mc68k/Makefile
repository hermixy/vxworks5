# Makefile - makefile for vw/src/arch/mc68k
#
# modification history
# --------------------
# 01c,25oct01,tam  updated for repackaging
# 01b,08jan98,dbt  removed wdbALib.o, wdbArchLib.o and dbgALib.o. Added
#                  wdbDbgALib.o and wdbDbgArchLib.o
# 01a,17jun96,yp   created from rev 01p of MakeSkel
#
# DESCRIPTION
# This file contains the makefile rules for building the vx library
#
#*/

TGT_DIR=$(WIND_BASE)/target

DOC_FILES =

LIB_BASE_NAME   = arch

OBJS_COMMON= bALib.o cacheALib.o cacheArchLib.o \
	     dbgArchLib.o dllALib.o dsmLib.o excALib.o excArchLib.o \
	     excArchShow.o ffsALib.o fppALib.o fppArchLib.o intALib.o \
	     intArchLib.o qPriBMapALib.o qPriHeapALib.o semALib.o semCALib.o \
	     semMALib.o sllALib.o taskArchLib.o trcLib.o vxmIfLib.o vxALib.o \
	     vxLib.o wdbDbgArchLib.o wdbDbgALib.o windALib.o workQALib.o \
	     sigCtxLib.o sigCtxALib.o

OBJS_MC68000 = $(OBJS_COMMON)
OBJS_MC68010 = $(OBJS_COMMON)
OBJS_MC68020 = $(OBJS_COMMON) unixALib.o mmu30Lib.o
OBJS_CPU32   = $(OBJS_COMMON)
OBJS_MC68040 = $(OBJS_COMMON) unixALib.o mmu40Lib.o
OBJS_MC68LC040 = $(OBJS_COMMON) unixALib.o mmu40Lib.o
OBJS_MC68060 = $(OBJS_COMMON) unixALib.o mmu40Lib.o

OBJS=$(OBJS_$(CPU))

include $(TGT_DIR)/h/make/rules.library
