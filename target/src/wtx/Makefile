# Makefile - makefile for target/src/wtx
#
# modification history
# --------------------
# 01d,12oct01,tam  added repackaging support
# 01c,13dec96,elp  changed syncLib.c into symSyncLib.c
# 01b,13nov96,elp  added doc generation.
# 01a,07oct96,yp  derived from 01a of MakeSkel
#
# DESCRIPTION
# This file contains the makefile rules for building the vx library
#
#*/

TGT_DIR=$(WIND_BASE)/target

LIB_BASE_NAME   = os

DOC_FILES=symSyncLib.c

EXTRA_INCLUDE= -I../../../share/src/agents/wdb

OBJS =	regfree.o regerror.o regcomp.o regexec.o xdrwtx.o \
	wtxrpc.o wtxexch.o wtx.o symSyncLib.o

include $(TGT_DIR)/h/make/rules.library
