# Makefile - makefile for extracting objects from libd.a
#
# modification history
# --------------------
# 01d,13nov01,sn   diabc++ -> diabcplus
# 01c,06nov01,sn   extract objects into a special directory to avoid stomping
#                  on VxWorks object files with the same name
# 01b,31oct01,tam  updated for repackaging
# 01a,24oct01,sn   wrote
#
# DESCRIPTION
# This file contains the definitions needed for extracting objects
# from libd.a (C++ Intrinsics)

TGT_DIR          = $(WIND_BASE)/target
TOOL             = diab

LIB_BASE_NAME	 = diabcplus
LIB_COMMON	 = false

DIABLIBRARY      = libd.a

CONFIGLETTE_NAME = dplusplus_intrinsics

include $(TGT_DIR)/src/tool/diab/rules.diab

