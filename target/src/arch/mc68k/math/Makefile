# Makefile - makefile for target/src/arch/mc68k/math
#
# modification history
# --------------------
# 01b,25oct01,tam  updated for repackaging
# 01a,12jul96,yp   created from 01g of MakeSkel
#
# DESCRIPTION
# This file contains the makefile rules for building the vx library
#
#*/

TGT_DIR=$(WIND_BASE)/target

DOC_FILES	=

LIB_BASE_NAME   = arch

OBJ_HW_SUPPORT	= mathHardALib.o mathHardLib.o

OBJ_GNU    	= gccMathLib.o gccMathALib.o gccUssALib.o \
	          mathSoftALib.o mathSoftLib.o \
	          uss_dpfncs.o uss_dpopns.o uss_fpfncs.o uss_fpopns.o \
		  uss_xlfncs.o

OBJ_040_GNU   	= gccUss040ALib.o

OBJ_040_FP_EXC 	= excFppALib.o bindec.o binstr.o bugfix.o decbin.o do_func.o \
		  gen_except.o get_op.o kernel_ex.o res_func.o round.o sacos.o \
		  sasin.o satan.o satanh.o scale.o scosh.o setox.o \
		  sgetem.o sint.o slog2.o slogn.o smovecr.o srem_mod.o \
		  ssin.o ssinh.o stan.o stanh.o sto_res.o stwotox.o \
		  tbldo.o util.o x_bsun.o x_operr.o x_ovfl.o \
		  x_snan.o x_store.o x_unfl.o x_unsupp.o

OBJ_040_FP_LIB 	= l_do_func.o l_entry.o l_round.o l_sacos.o \
		  l_sasin.o l_satan.o l_satanh.o l_scale.o \
		  l_scosh.o l_setox.o l_sgetem.o l_sint.o \
		  l_slog2.o l_slogn.o l_srem_mod.o l_ssin.o \
		  l_ssinh.o l_stan.o l_stanh.o l_stwotox.o \
		  l_support.o

OBJ_060_UII_EXC = isp060ALib.o isp060Lib.o os060ALib.o fpsp060ALib.o \
		  fpsp060Lib.o fplsp060ALib.o

OBJ_040_FP 	= $(OBJ_040_FP_EXC) $(OBJ_040_FP_LIB)

OBJ_MC68000 	= mathALib.o fixunssfsi.o $(OBJ_GNU)
OBJ_MC68010 	= mathALib.o fixunssfsi.o $(OBJ_GNU)
OBJ_MC68020 	= mathALib.o fixunssfsi.o $(OBJ_HW_SUPPORT) $(OBJ_GNU)
OBJ_MC68040 	= mathALib.o fixunssfsi.o $(OBJ_HW_SUPPORT) $(OBJ_040_GNU) $(OBJ_040_FP)
OBJ_CPU32   	= mathALib.o fixunssfsi.o $(OBJ_HW_SUPPORT) $(OBJ_GNU)
OBJ_MC68LC040 	= mathALib.o fixunssfsi.o $(OBJ_GNU)
OBJ_MC68060 	= mathALib.o fixunssfsi.o $(OBJ_HW_SUPPORT) $(OBJ_060_UII_EXC) \
		  $(OBJ_040_GNU)

OBJS=$(OBJ_$(CPU))

include $(TGT_DIR)/h/make/rules.library

SUBDIRS=
