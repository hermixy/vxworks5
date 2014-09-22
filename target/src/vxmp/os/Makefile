# Makefile - makefile for target/src/vxmp/os
#
# modification history
# --------------------
# 01b,31oct01,tam  updated for repackaging
# 01a,31oct01,mas  created from 01i of target/src/os/Makefile
#
# DESCRIPTION
# This file contains the makefile rules for building the vxmp/os library
#
#*/

TGT_DIR=$(WIND_BASE)/target

EXTRA_INCLUDE	=
EXTRA_DEFINE	=

DOC_FILES=	smMemLib.c smMemShow.c smNameLib.c smNameShow.c smObjLib.c \
		smObjShow.c

LIB_BASE_NAME	= vxmp

OBJS=	smFixBlkLib.o smFixBlkShow.o smMemLib.o \
	smMemShow.o smNameLib.o smNameShow.o smObjLib.o smObjShow.o

include $(TGT_DIR)/h/make/rules.library
