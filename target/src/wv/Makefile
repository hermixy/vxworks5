# Makefile - makefile for target/src/wv
#
# modification history
# --------------------
# 01e,17feb98,dvs  added memLib.o and memPartLib.o (pr)
# 01d,13dec97,pr   added sigLib.o
# 01c,07feb97,dvs  updating LIBDIRNAME to reflect name chage for SETUP
# 01b,25nov96,ms   suppress archive generation
# 01a,17aug96,yp   derived from 01a of MakeSkel
#
# DESCRIPTION
# This file contains the makefile rules for building the vx library
#
#*/

TGT_DIR=$(WIND_BASE)/target

LIBNAME=lib$(CPU)$(TOOL)wv.a
LIBDIRNAME=obj$(CPU)$(TOOL)vxwv
EXTRA_DEFINE=-UWV_INSTRUMENTATION

OBJS=	classLib.o memLib.o memPartLib.o msgQLib.o msgQShow.o objLib.o \
	qJobLib.o schedLib.o semBLib.o semCLib.o semLib.o semMLib.o \
	semShow.o sigLib.o taskLib.o taskShow.o tickLib.o wdLib.o \
	wdShow.o windLib.o workQLib.o

include $(TGT_DIR)/h/make/rules.library

