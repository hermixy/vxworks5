# Makefile - makefile for target/src/util
#
# modification history
# --------------------
# 01e,31oct01,mas  moved qFifoGLib, smDllLib to target/src/vxmp/util
# 01d,12oct01,tam  added repackaging support
# 01c,20oct97,nps  Added rBuffShow.o object.
# 01b,28jul97,nps  Added in rBuffLib.o object.
# 01a,17aug96,yp   derived from 01i of MakeSkel
#
# DESCRIPTION
# This file contains the makefile rules for building the vx library
#
#*/

TGT_DIR=$(WIND_BASE)/target

LIB_BASE_NAME   = os

DOC_FILES=	bLib.c bootLib.c lstLib.c rngLib.c inflateLib.c rBuffLib.c

OBJS=	bLib.o bootLib.o bufLib.o cksumLib.o dllLib.o lstLib.o qFifoLib.o \
	qJobLib.o qLib.o qPriBMapLib.o qPriDeltaLib.o \
	qPriHeapLib.o qPriListLib.o rngLib.o sllLib.o smUtilLib.o \
	uncompress.o inflateLib.o rBuffLib.o rBuffShow.o

include $(TGT_DIR)/h/make/rules.library

