# Makefile - makefile for extracting objects from libstl.a
#
# modification history
# --------------------
# 01d,13nov01,sn  (implicitly) use exclude list from defs.exclude
# 01c,06nov01,sn   extract objects into a special directory to avoid stomping
#                  on VxWorks object files with the same name
# 01b,31oct01,tam  updated for repackaging
# 01a,24oct01,sn   wrote
#
# DESCRIPTION
# This file contains the definitions needed for extracting objects
# from libstl.a (C++ Standard Library).

TGT_DIR          = $(WIND_BASE)/target
TOOL             = diab

LIB_BASE_NAME	 = cplus
LIB_COMMON	 = false

DIABLIBRARY      = libstl.a

CONFIGLETTE_NAME = diab_cplusplus_std_library

include $(TGT_DIR)/src/tool/diab/rules.diab

