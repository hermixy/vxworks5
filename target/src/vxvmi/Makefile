# Makefile - makefile for vw/src/vxvmi
#
# modification history
# --------------------
# 01a,01nov01,tam   created
#
# DESCRIPTION
# This file contains the makefile rules for building the vx library
#
#*/

TGT_DIR=$(WIND_BASE)/target

DOC_FILES	= vmLib.c vmShow.c

LIB_BASE_NAME	= vxvmi

OBJS_COMMON	= vmLib.o vmShow.o

OBJS		= $(OBJS_COMMON) $(OBJS_$(CPU))

include $(TGT_DIR)/h/make/rules.library
