# Makefile - makefile for .../target/src/vxfusion/wind
#
# modification history
# --------------------
# 01b,19oct01,tam  added support for new archive directory hierarchy
# 01a,07nov96,yp  written 
#
# DESCRIPTION
# This file contains the makefile rules for building the VxFusion library
#
#*/

TGT_DIR=$(WIND_BASE)/target

LIB_BASE_NAME   = vxfusion

EXTRA_INCLUDE	=
EXTRA_DEFINE	=

DOC_FILES 	= distLib.c msgQDistLib.c msgQDistShow.c msgQDistGrpLib.c \
			  msgQDistGrpShow.c distTBufLib.c

OBJS=		distGapLib.o distIncoLib.o distLib.o distNetLib.o \
			distNodeLib.o distTBufLib.o msgQDistGrpLib.o \
			msgQDistGrpShow.o msgQDistLib.o \
			msgQDistShow.o

include $(TGT_DIR)/h/make/rules.library
