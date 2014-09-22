# Makefile - makefile for target/src/wdb/xdr
#
# modification history
# --------------------
# 01b,16oct01,tam  updated for re-pack
# 01a,17aug96,yp  derived from 01b of MakeSkel
#
# DESCRIPTION
# This file contains the makefile rules for building the vx library
#*/

TGT_DIR=$(WIND_BASE)/target

LIB_BASE_NAME   = wdb

DOC_FILES=

EXTRA_INCLUDE= -I../../../../share/src/agents/wdb

OBJS=	ctx.o ctxcreat.o ctxstep.o evtdata.o evtpoint.o \
	memory.o regs.o rpccksum.o tgtinfo.o wrapper.o xdrcore.o 

include $(TGT_DIR)/h/make/rules.library

