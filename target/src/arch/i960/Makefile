# Makefile - makefile for vw/src/arch/i960
#
# modification history
# --------------------
# 01c,08jan98,dbt  removed wdbArchLib.o, wdbALib.o and dbgALib.o. Added
#                  wdbDbgArchLib.o and wdbDbgALib.o
# 01b,03dec96,tpr  removed mathHardLib.o, now in math directory.
# 01a,18jun96,yp   created from 01n of MakeSkel
#
# DESCRIPTION
# This file contains the makefile rules for building the vx library
#*/

TGT_DIR=$(WIND_BASE)/target

DOC_FILES = cacheI960JxALib.s cacheI960JxLib.c cacheI960CxALib.s \
	    cacheI960CxLib.c

LIBNAME=lib$(CPU)$(TOOL)vx.a
LIBDIRNAME=obj$(CPU)$(TOOL)vx


OBJS_COMMON = bALib.o cacheArchLib.o dbgArchLib.o \
	      dsmArchLib.o excArchLib.o excALib.o excArchShow.o ffsALib.o \
	      fppALib.o fppArchLib.o intArchLib.o intALib.o \
	      sigCtxLib.o sigCtxALib.o swapLib.o taskArchLib.o trcLib.o \
	      vxmIfLib.o vxALib.o vxLib.o wdbDbgArchLib.o wdbDbgALib.o \
	      windALib.o workQALib.o

OBJS_JX_SPECIFIC = cacheI960JxALib.o cacheI960JxLib.o
OBJS_CX_SPECIFIC = cacheI960CxALib.o cacheI960CxLib.o

OBJS_I960JX  = $(OBJS_COMMON) $(OBJS_JX_SPECIFIC)
OBJS_I960CA  = $(OBJS_COMMON) $(OBJS_CX_SPECIFIC)
OBJS_I960KA  = $(OBJS_COMMON)
OBJS_I960KB  = $(OBJS_COMMON)
OBJS_I960HX  = $(OBJS_COMMON) $(OBJS_JX_SPECIFIC)

OBJS=$(OBJS_$(CPU))

include $(TGT_DIR)/h/make/rules.library

