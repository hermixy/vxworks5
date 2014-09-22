# Makefile - makefile for target/src/arch/sparc/math
#
# modification history
# --------------------
# 01a,12jul96,yp   created from 01f of MakeSkel
#
# DESCRIPTION
# This file contains the makefile rules for building the vx library
#
#*/

TGT_DIR=$(WIND_BASE)/target

LIBNAME=lib$(CPU)$(TOOL)vx.a
LIBDIRNAME=obj$(CPU)$(TOOL)vx

OBJS_COMMON = mathHardLib.o mathHardALib.o mathSoftLib.o
OBJS_EXTRA  = intMathALib.o
OBJS_SPARC  = 
OBJS_SPARClite_gnu = arc32.o ceil32.o dp32.o exp32.o fdmul32.o floor32.o \
                 fp32.o fparc32.o fpceil32.o fpexp32.o fpfloo32.o fphyp32.o \
                 fplog32.o fpmod32.o fppow32.o fpsqrt32.o fptrig32.o func32.o \
                 hyp32.o log32.o mod32.o pow32.o sqrt32.o trig32.o
OBJS_SPARClite_fpgnu =
OBJS_SPARClite	= $(OBJS_SPARClite_$(TOOL))

OBJS        = $(OBJS_COMMON) $(OBJS_EXTRA) $(OBJS_$(CPU))

include $(TGT_DIR)/h/make/rules.library

