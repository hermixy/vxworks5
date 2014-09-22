# Makefile - makefile for extracting objects from libtorn.a
#
# modification history
# --------------------
# 01c,13nov01,tam  moved archive into new directory hierarchy
# 01b,13nov01,sn  (implicitly) use exclude list from defs.exclude
# 01a,24oct01,sn   wrote
#
# DESCRIPTION
# This file contains the definitions needed for extracting objects
# from libtorn.a

TGT_DIR          = $(WIND_BASE)/target
TOOL             = diab

LIB_BASE_NAME	= dcc
LIB_COMMON	= true

DIABLIBRARY      = libtorn.a

CONFIGLETTE_NAME = diab_intrinsics_torn

include $(TGT_DIR)/src/tool/diab/rules.diab

