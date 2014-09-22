# Makefile - makefile for target/src/arch/i86/math
#
# modification history
# --------------------
# 01d,25oct01,tam  updated for repackaging
# 01c,10aug01,hdn  added support for PENTIUM2/3/4.
# 01b,09apr98,hdn  added support for Pentium.
# 01a,12jul96,yp   created from 01b of MakeSkel
#
# DESCRIPTION
# This file contains the makefile rules for building the vx library
#
#*/

TGT_DIR=$(WIND_BASE)/target

DOC_FILES	=

LIB_BASE_NAME   = arch

OBJ_HW_SUPPORT	= mathHardALib.o mathHardLib.o

OBJ_I80386 	= emuMainALib.o emuFncALib.o emuOperALib.o emuSubALib.o \
		  mathALib.o mathSoftLib.o $(OBJ_HW_SUPPORT)
OBJ_I80486 	= emuMainALib.o emuFncALib.o emuOperALib.o emuSubALib.o \
		  mathALib.o mathSoftLib.o $(OBJ_HW_SUPPORT)
OBJ_PENTIUM 	= emuMainALib.o emuFncALib.o emuOperALib.o emuSubALib.o \
		  mathALib.o mathSoftLib.o $(OBJ_HW_SUPPORT)
OBJ_PENTIUM2 	= emuMainALib.o emuFncALib.o emuOperALib.o emuSubALib.o \
		  mathALib.o mathSoftLib.o $(OBJ_HW_SUPPORT)
OBJ_PENTIUM3 	= emuMainALib.o emuFncALib.o emuOperALib.o emuSubALib.o \
		  mathALib.o mathSoftLib.o $(OBJ_HW_SUPPORT)
OBJ_PENTIUM4 	= emuMainALib.o emuFncALib.o emuOperALib.o emuSubALib.o \
		  mathALib.o mathSoftLib.o $(OBJ_HW_SUPPORT)

OBJS=$(OBJ_$(CPU))

include $(TGT_DIR)/h/make/rules.library

