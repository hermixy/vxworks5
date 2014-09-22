# Makefile - makefile for target/src/arch/i960/math
#
# modification history
# --------------------
# 01d,12mar97,tpr  added sincosf.o & trndfsf.o, replace DMSH_FIRST by DLSH_FIRST
# 01c,07dec96,tpr  moved frexp.o to OBJ_COMMON.
# 01b,03dec96,tpr  added OBJ_HW_SUPPORT and OBJ_SW_SUPPORT
# 01a,12jul96,yp   created from 01c of MakeSkel
#
# DESCRIPTION
# This file contains the makefile rules for building the vx library
#
#*/

TGT_DIR=$(WIND_BASE)/target

DOC_FILES	=

LIBNAME=lib$(CPU)$(TOOL)vx.a
LIBDIRNAME=obj$(CPU)$(TOOL)vx

EXTRA_DEFINE	= -DIEEE -DLSH_FIRST

OBJ_COMMON	= a_expm1.o const_dp.o cosh.o environ.o error.o expf.o fabs.o \
		  frexp.o fmod.o ldexp.o modf.o sinh.o tanh.o

OBJ_HW_SUPPORT	= acoss.o acosd.o asins.o asind.o atan.o cos.o sin.o	     \
		  tan.o ceilf.o ceil.o ceill.o class.o constant.o floorf.o   \
		  floor.o floorl.o exp2m1.o exps.o expd.o fp_exits.o isnan.o \
		  log_sup.o logs.o logd.o loge.o mathHardLib.o nan.o pows.o  \
		  powd.o rem.o round.o scale.o sqrt.o

OBJ_SW_SUPPORT	= a_acos.o a_asin.o a_atan.o a_atanf.o a_exp.o a_expf.o      \
		  a_fpow.o a_log.o a_log10.o a_logf.o a_pow.o  		     \
		  a_sqrt.o a_sqrtf.o a_sqrtfi.o a_sqrti.o a_tan.o a_tanf.o   \
		  afpcnand.o afpcnans.o clsdfsi.o clssfsi.o clstfsi.o	     \
		  ipow.o ipowf.o mathALib.o mathSoftLib.o matherr.o opndf3.o \
		  sincos.o sincosf.o trndfsf.o

OBJ_I960CA 	= $(OBJ_COMMON) $(OBJ_SW_SUPPORT)
OBJ_I960HX      = $(OBJ_I960CA)
OBJ_I960JX 	= $(OBJ_I960CA)
OBJ_I960KA 	= $(OBJ_I960CA)
OBJ_I960KB 	= $(OBJ_COMMON) $(OBJ_HW_SUPPORT)

OBJS=$(OBJ_$(CPU))

include $(TGT_DIR)/h/make/rules.library


