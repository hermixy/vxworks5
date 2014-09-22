# Makefile - makefile for target/src/wdb
#
# modification history
# --------------------
# 01h,12oct01,tam  added repackaging support
# 01g,29jan01,dtr  Adding altivec register support library.
# 01f,03mar00,zl   merged SH support from T1
# 01e,25mar98,dbt  added wdbLib.c in doc for wdbSystemSuspend() routine.
# 01d,27feb98,dbt  added wdbCtxStartLib.o and wdbUserEvtLib.o
#		   added wdbUserEvtLib.c in DOC_FILES.
# 01c,26jan98,dbt  added wdbEvtptLib.o
# 01b,13jan98,dbt  added wdbDbgLib.o
# 01a,17aug96,yp   derived from 01c of MakeSkel
#
# DESCRIPTION
# This file contains the makefile rules for building the vx library
#
#*/

TGT_DIR=$(WIND_BASE)/target

LIB_BASE_NAME   = wdb

DOC_FILES= wdbUserEvtLib.c wdbLib.c

EXTRA_INCLUDE= -I../../../share/src/agents/wdb

OBJS =	wdbLib.o wdbRpcLib.o wdbSvcLib.o wdbCnctLib.o wdbCtxLib.o \
	wdbMemCoreLib.o wdbMemLib.o wdbRegLib.o wdbEvtLib.o wdbCallLib.o \
	wdbGopherLib.o wdbArchLib.o wdbBpLib.o wdbVioLib.o wdbUdpSockLib.o \
	wdbTyCoDrv.o wdbUdpLib.o wdbFuncBind.o wdbTaskBpLib.o \
	wdbDirectCallLib.o wdbExcLib.o wdbCtxExitLib.o wdbFpLib.o \
	wdbSioTestLib.o wdbDbgLib.o wdbEvtptLib.o wdbCtxStartLib.o \
	wdbUserEvtLib.o wdbDspLib.o wdbAltivecLib.o

include $(TGT_DIR)/h/make/rules.library
