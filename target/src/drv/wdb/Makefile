# Makefile - makefile for bsp/src/drv/vio
#
# Copyright 1984-1998 Wind River Systems, Inc.
#
# modification history
# --------------------
# 01f,12mar02,dat  SPR 72518 Drivers docs in separate directory
# 01e,12oct01,tam  added repackaging support
# 01d,12jun98,dat  added wdbTsfsDrv.c to doc files
# 01c,11mar98,pdn  added wdbPipePktDrv.c
# 01c,02mar97,vin  added wdbEndPktDrv.c merge to tor1_0_1
# 01b,12nov96,c_s  add target server file system support
# 01a,18jun96,yp   created from 01d of MakeSkel
#
# DESCRIPTION
# This file contains the makefile rules for building the vx library
#

TGT_DIR=$(WIND_BASE)/target

DOC_FILES= 	wdbVioDrv.c wdbUlipPktDrv.c wdbSlipPktDrv.c \
		wdbNetromPktDrv.c wdbEndPktDrv.c wdbPipePktDrv.c \
		wdbTsfsDrv.c

LIB_BASE_NAME	= wdb

NODEPENDOBJS=wdbNetromPktDrv.c

EXTRA_INCLUDE= -I$(TGT_DIR)/src/drv/

OBJS =  wdbVioDrv.o wdbUlipPktDrv.o wdbSlipPktDrv.o wdbTsfsDrv.o \
	wdbEndPktDrv.o wdbPipePktDrv.o

include $(TGT_DIR)/h/make/rules.library

DOC_DIR = $(DOCS_ROOT)/vxworks/ref/drv
DOC_CHAPTER=Driver_Libraries
