# Makefile - makefile for bsp/src/drv
#
# modification history
# --------------------
# 01e,09nov01,jmp  specified subdirs to build for simulators only.
# 01d,12oct01,tam  added repackaging support
# 01c,04jun98,dat  removed unnecessary EXTRA_INCLUDE, removed bspVersion.o
# 01b,06jan98,cym  Changed -I to $(OPTION_INCLUDE_DIR) to decouple from GNU
# 01a,18jun96,yp   created from 01f of MakeSkel
#
# DESCRIPTION
# This file contains the makefile rules for building the vx library
#
#*/

TGT_DIR=$(WIND_BASE)/target

LIB_BASE_NAME=drv

OBJS=

#
# define SUBDIRS for simulators only, to avoid building everything
#

ifneq ($(subst SIM,,$(CPU)),$(CPU))
SUBDIRS=end netif vxfusion wdb
endif

include $(TGT_DIR)/h/make/rules.library

