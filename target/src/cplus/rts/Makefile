# Makefile - makefile for vw/src/cplus/rts
#
# modification history
# --------------------
# 02f,21jan02,sn   removed cplusGlob.o
# 02e,04dec01,sn   moved demangle code to demangler/
# 02d,26oct01,tam  updated for repackaging
# 02c,16oct98,fle  set DOC_FILES to cplusLibDoc.c and removed the man target
# 02b,12jun98,sn   merged in fix for spr8947 which moves the real demangling
#                  work to cplus-dem.c
# 02a,10apr98,sn   added cplusDemStub.o to OBJS
# 01c,20nov96,jco  added man pages copy (man target).
# 01b,06nov96,p_m  removed cplusGlob.o, cplusInit.o, and cplusUsr.o from
#                  DOC_FILES definition
# 01a,18jun96,yp   created from 01b of MakeSkel
# 
# DESCRIPTION
# This file contains the makefile rules for building C++ runtime support
#

TGT_DIR=$(WIND_BASE)/target

DOC_FILES	= cplusLibDoc.c

LIB_BASE_NAME	= cplus
LIB_COMMON	= false

OBJS=		cplusLib.o	\
		cplusCore.o	\
		cplusInit.o	\
		cplusLoad.o	\
		cplusStr.o	\
		cplusUsr.o	\
		cplusXtors.o

MAKETAIL=	makeCfrontLibrary
CLEANTAIL=	cleanCfrontLibrary

include $(TGT_DIR)/h/make/rules.library

makeCfrontLibrary:
	$(MAKE) LIBNAME=lib$(CPU)$(TOOL)cf.a			\
		LIBDIRNAME=obj$(CPU)$(TOOL)cf		\
		C++_COMPILER='-ansi -pipe -nostdinc +a1 +V -dd=off -D"signed="' \
		CXX='$(CF)'

cleanCfrontLibrary:
	$(MAKE) LIBNAME=lib$(CPU)$(TOOL)cf.a			\
		LIBDIRNAME=obj$(CPU)$(TOOL)cf		\
		clean

# end of vw/src/cplus/rts/Makefile
