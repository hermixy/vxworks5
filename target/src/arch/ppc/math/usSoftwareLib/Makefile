# Makefile - makefile for target/src/arch/ppc/math/usSoftwareLib/
#
# modification history
# --------------------
# 01e,12oct01,tam  added support for new packaging scheme
# 01d,23aug01,pch  Add PPC440
# 01c,10nov00,s_m  PPC405 support
# 01b,19aug98,tpr  added PowerPC EC 603 support.
# 01a,20nov96,tpr  created from 01a of target/src/arch/mc69k/math/Makefile
#
# DESCRIPTION
# This file contains the makefile rules for building the software emulation
# of the floating point library for PowePC.
#
#*/

TGT_DIR=$(WIND_BASE)/target

DOC_FILES	=

LIB_BASE_NAME	= arch

OBJ_USS		= arc32.o ceil32.o dp32.o dpcmp.o exp32.o floor32.o fp32.o \
		  fparc32.o fpceil32.o fpcmp.o fpexp32.o fpfloo32.o 	   \
		  fphyp32.o fplog32.o fpmod32.o fppow32.o fpsqrt32.o	   \
		  fptrig32.o func32.o hyp32.o log32.o mod32.o pow32.o 	   \
		  sqrt32.o trig32.o

OBJ_GCC		= gccMathLib.o

OBJ_PPC403 	= $(OBJ_USS) $(OBJ_GCC)
OBJ_PPCEC603 	= $(OBJ_PPC403)
OBJ_PPC860 	= $(OBJ_PPC403)
OBJ_PPC405	= $(OBJ_PPC403)
OBJ_PPC440	= $(OBJ_PPC403)

OBJS=$(OBJ_$(CPU))

include $(TGT_DIR)/h/make/rules.library

SUBDIRS=
