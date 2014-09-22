# Makefile - makefile for extracting objects from libimpl.a
#
# modification history
# --------------------
# 01d,14nov01,sn   fixed comment
# 01c,13nov01,tam  moved archive into new directory hierarchy
# 01b,13nov01,sn  (implicitly) use exclude list from defs.exclude
# 01a,24oct01,sn   wrote
#
# DESCRIPTION
# This file contains the definitions needed for extracting objects
# from libimpl.a (C intrinsics)

TGT_DIR          = $(WIND_BASE)/target
TOOL             = diab

LIB_BASE_NAME	= dcc
LIB_COMMON	= true

DIABLIBRARY      = libimpl.a

CONFIGLETTE_NAME = diab_intrinsics_impl

include $(TGT_DIR)/src/tool/diab/rules.diab

