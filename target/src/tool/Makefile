# Makefile - makefile for target/src/tool
#
# modification history
# --------------------
# 01b,03dec01,sn   added common
# 01a,16oct01,sn   wrote
# 
# DESCRIPTION
# The toolchain libraries are compiled as part of the cross-toolchain
# build rather than as part of the VxWorks runtime build.
# They are deposited in the toolchain installation area. Here we simply
# extract specific objects from these libraries and deposit them in the 
# appropriate VxWorks object directory.

TGT_DIR=$(WIND_BASE)/target

SUBDIRS=$(TOOL_FAMILY) common

include $(TGT_DIR)/h/make/rules.library


