# Makefile - makefile for target/src/drv/vxfusion
#
# modification history
# --------------------
# 01d,12oct01,tam  added repackaging support
# 01c,24may99,drm  Removing EXTRA_INCLUDE entries related to VxFusion
# 01b,19may99,drm  Changing vxfusion to vxfusion
# 01a,02sep98,drm  created. 
#
# DESCRIPTION
# This file contains the makefile rules for building VxFusion adapters
# into the VxWorks library.
#
#*/

TGT_DIR=$(WIND_BASE)/target

DOC_FILES=

LIB_BASE_NAME	= vxfusion

EXTRA_INCLUDE=

OBJS_COMMON = distIfUdp.o

OBJS        = $(OBJS_COMMON) $(OBJS_$(CPU))

include $(TGT_DIR)/h/make/rules.library

CC_OPTIM=$(CC_OPTIM_DRIVER)
