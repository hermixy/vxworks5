# Makefile - makefile for the Berkeley Packet Filter (target/src/bpf directory)
#
# modification history
# --------------------
# 01c,16oct01,tam  updated for re-pack
# 01b,07feb00,dgp  remove bpfProto.c from docs build
# 01a,24nov99,spm  written
#
# DESCRIPTION
# This file contains the makefile rules for building the object files for
# the Berkeley Packet Filter.
#
#*/

TGT_DIR=$(WIND_BASE)/target

LIB_BASE_NAME   = net

DOC_FILES=	bpfDrv.c

OBJS=	bpfDrv.o bpfProto.o bpf_filter.o

include $(TGT_DIR)/h/make/rules.library
