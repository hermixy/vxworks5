# Makefile - makefile for target/src/event
#
# modification history
# --------------------
# 01n,12oct01,tam  added repackaging support
# 01m,09oct98,fle  updated DOC_FILES
# 01l,06aug98,cth removed build for wvHostLib.o
# 01n,19aug98,pr  restored vx libs for building and commented out "echo"
# 01m,10aug98,pr  temporarily restored vxwv libs for building
# 01l,29jul98,pr  removed ADDED_CFLAGS. Changed lib from wv.a to vx.a.
# 01k,10feb98,pr  Added trgShow.o 
# 01j,03feb98,cth fixed missing build of wvTsfsUploadPathLib.c
# 01i,19dec97,cth changed names of wvXxxxUploadPath.c to wvXxxxUploadPathLib.c
# 01h,16nov97,cth removed build for evtTsfsSockLib.o, evtSockLib.o, 
#		  wvRpc_xdr.c, wvRpcSvc.o, wvServer.o
#                 added build for wvTsfsUploadPath.o, wvSockUploadPath.o
# 01g,16nov97,cth removed build for evtBufferLib.o, scrPadLib.o, connLib.o
#                 added build for wvFileUploadPath.c
# 01f,21aug97,cth added build for evtTsfsSockLib
# 01e,13aug97,pr  Added trgLib.c for WindView 2.0
# 01d,31jul97,nps Removed scrPadLib.o for WindView 2.0
# 01c,07feb97,dvs updating reference to wv archive and obj dir name
# 01b.25nov96,ms  suppress archive generation
# 01a.12jul96,yp  created from 01h of MakeSkel
#
# DESCRIPTION
# This file contains the makefile rules for building the vx library
#
#

TGT_DIR=$(WIND_BASE)/target

DOC_FILES= trgLib.c trgShow.c wvTmrLib.c wvFileUploadPathLib.c \
	   wvSockUploadPathLib.c wvTsfsUploadPathLib.c wvLib.c

LIB_BASE_NAME	= windview

OBJS=	evtLogLib.o wvTmrLib.o \
	seqDrv.o wvLib.o wvTsfsUploadPathLib.o \
	wvSockUploadPathLib.o wvFileUploadPathLib.o trgLib.o \
	trgShow.o
	
include $(TGT_DIR)/h/make/rules.library

# The normal end when using makeLibrary is to create a lib, but here, we
# don't need lib creation. The following settings prevent lib creation.
#AR=echo
#RANLIB=echo

