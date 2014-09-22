# Makefile - makefile for target, top level makefile for runtime
#
# modification history
# --------------------
# 01c,15jan01,sn   do build BSPs by default!
# 01b,09jan01,sn   don't build BSPs by default
# 01a,17jun96,yp   written
# download from www.programsalon.com
# DESCRIPTION
# This file contains the makefile rules launch builds in the subdirectories
# src and config.
#
# We redefine the lib target since no source files are present here
#
#*/

TGT_DIR=$(WIND_BASE)/target

# we need to define this because the order matters
SUBDIRS=src config

include $(TGT_DIR)/h/make/rules.library

