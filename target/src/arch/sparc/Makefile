# Makefile - makefile for vw/src/arch/sparc
#
# modification history
# --------------------
# 01b,09jan98,dbt  removed wdbALib.o, wdbArchLib.o and dbgALib.o. Added
#                  wdbDbgALib.o and wdbDbgArchLib.o
# 01a,18jun96,yp   created from 01w of MakeSkel
#
# DESCRIPTION
# This file contains the makefile rules for building the vx library
#
#*/

TGT_DIR=$(WIND_BASE)/target

DOC_FILES =

LIBNAME=lib$(CPU)$(TOOL)vx.a
LIBDIRNAME=obj$(CPU)$(TOOL)vx

OBJS_COMMON = bALib.o dbgArchLib.o dsmLib.o \
              excALib.o excArchLib.o excArchShow.o ffsALib.o fppALib.o \
	      fppArchLib.o intALib.o intArchLib.o \
	      mmuSparcLib.o mmuSparcALib.o taskArchLib.o trcLib.o \
	      unixALib.o vxmIfLib.o vxALib.o vxLib.o windALib.o sigCtxLib.o \
	      sigCtxALib.o sparcLib.o _varargs.o wdbDbgArchLib.o wdbDbgALib.o

OBJS_SPARC    = cacheCy604Lib.o cacheSun4Lib.o cy604ALib.o sun4ALib.o \
                mmuSun4ILib.o mmuSun4Lib.o mmuSun4pILib.o mmuSun4pLib.o \
		cacheSun4pLib.o cacheTi390Lib.o ti390ALib.o mmuTi390Lib.o \
		mmuSparcILib.o cacheMicroSparcLib.o microSparcALib.o \
		ioMmuMicroSparcLib.o mmuSparcIALib.o tiTms390ALib.o \
		cacheTiTms390Lib.o mccALib.o mmuL64862Lib.o

OBJS_SPARClite= mb930ALib.o cacheMb930Lib.o

OBJS        = $(OBJS_COMMON) $(OBJS_$(CPU))

include $(TGT_DIR)/h/make/rules.library

