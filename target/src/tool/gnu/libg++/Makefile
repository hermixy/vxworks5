# Makefile - makefile for extracting objects from libgcc.a
#
# modification history
# --------------------
# 01d,13nov01,sn   gnuc++ -> gnucplus
# 01c,06nov01,sn   extract objects into a special directory to avoid stomping
#                  on VxWorks object files with the same name
# 01b,31oct01,tam  updated for repackaging
# 01a,24oct01,sn   wrote
#
# DESCRIPTION
# This file contains the definitions needed for extracting objects
# from libgcc.a (GNU C++ compiler support).

TGT_DIR=$(WIND_BASE)/target
TOOL=gnu

LIB_BASE_NAME	= gnucplus
LIB_COMMON	= false

GCCLIB		= $(WIND_BASE)/host/$(WIND_HOST_TYPE)/lib/gcc-lib
GNULIBDIR	= $(GCCLIB)/$(CC_MACHINE)/$(CC_VERSION)
GNULIBRARY	= libgcc.a

CONFIGLETTE_NAME    = gplusplus_intrinsics

# C++ runtime library

include $(TGT_DIR)/src/tool/gnu/libg++/defs.g++

REAL_OBJS=$(OBJS_CPLUS)

include $(TGT_DIR)/src/tool/gnu/rules.gnu
