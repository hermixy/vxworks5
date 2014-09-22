# Makefile - makefile for target/src/arch/sh
#
# modification history
# --------------------
# 01m,25oct01,tam  updated for repackaging
# 01l,21mar01,frf  added cacheSh7622Lib.o and cacheSh7622ALib.o to OBJS_SH7600.
# 01k,02feb01,hk   added cacheSh7604ALib.o to OBJS_SH7600.
# 01j,09sep00,hk   added cacheSh7729ALib.o to OBJS_SH7700.
# 01i,22aug00,hk   merge SH7729 to SH7700. merge SH7410 to SH7600.
# 01h,20aug00,hk   merge SH7040 to SH7600. add cacheSh7700ALib.o to OBJS_SH7700.
# 01g,31jul00,hk   added mmuSh7700ALib.o to OBJS_SH7750.
# 01f,12apr00,frf  Deleted dbgAlib.o, wdbALib.s, wdbArchLib.c. Added
#                  wdbDbgALib.o, wdbDbgArchLib.c.
# 01e,06oct98,hk   chenged to use mmuSh7750Lib.o for SH7750.
# 01d,30sep98,hk   deleted mmuSh7700ALib.o from OBJS_SH7750.
# 01c,17sep98,hk   merged mmuSh7750[A]Lib.o to mmuSh7700[A]Lib.o.
# 01b,06jul98,st   added SH7750 support.
# 01d,22jul98,mem  added dspALib and dspArchLib.
# 01c,24jun98,jmc  added cacheSh7729Lib.
# 01b,08may98,jmc  added support for SH-DSP and SH3-DSP.
# 01a,25apr97,hk   created from rev 01i of MakeSkel
#
# DESCRIPTION
# This file contains the makefile rules for building the vx library
#
#*/

TGT_DIR=$(WIND_BASE)/target

DOC_FILES =

LIB_BASE_NAME   = arch

OBJS_COMMON = bALib.o dbgArchLib.o dllALib.o dsmLib.o excALib.o \
	      excArchLib.o excArchShow.o ffsALib.o fppALib.o fppArchLib.o \
	      intALib.o intArchLib.o qPriBMapALib.o semALib.o semCALib.o \
	      semMALib.o sllALib.o taskArchLib.o trcLib.o unixALib.o \
	      vxmIfLib.o vxALib.o vxLib.o wdbDbgArchLib.o wdbDbgALib.o \
	      windALib.o workQALib.o sigCtxLib.o sigCtxALib.o

OBJS_DSP    = dspArchLib.o dspALib.o

OBJS_SH7000 = $(OBJS_COMMON)
OBJS_SH7600 = $(OBJS_COMMON) $(OBJS_DSP) \
		cacheSh7604Lib.o cacheSh7604ALib.o cacheSh7040Lib.o \
		cacheSh7622Lib.o cacheSh7622ALib.o
OBJS_SH7700 = $(OBJS_COMMON) $(OBJS_DSP) mmuSh7700Lib.o mmuSh7700ALib.o \
		cacheSh7729Lib.o cacheSh7729ALib.o \
		cacheSh7700Lib.o cacheSh7700ALib.o
OBJS_SH7750 = $(OBJS_COMMON) mmuSh7750Lib.o mmuSh7700ALib.o \
		cacheSh7750Lib.o cacheSh7750ALib.o

OBJS=$(OBJS_$(CPU))

include $(TGT_DIR)/h/make/rules.library


# override the default rule in rules.library to assemble dspALib.o with -Wa,-dsp

$(LIBDIR)/dspALib.o : dspALib.s
	@ $(RM) $(subst /,$(DIRCHAR),$@)
	$(CC) -Wa,-dsp $(CFLAGS_AS) $(OPTION_OBJECT_ONLY) $(OPTION_OBJECT_NAME)$@ $<
