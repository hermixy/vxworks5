# Makefile - makefile for vw/src/os
#
# modification history
# --------------------
# 01p,18dec01,to   add ARMARCH5(_T) support, fix XSCALE
# 01o,01nov01,tam  moved vmLib.c and vmShow.c to src/vxvmi
# 01n,31oct01,mas  moved smXxxLib/Show modules to target/src/vxmp/os
# 01w,26oct01,tam  added repackaging support
# 01l,25oct01,jab  use make variable instead of GNU-specific -fvec
# 01k,20aug01,zl   added dspLib.o and dspShow.o for SH builds, removed
#                  dosFsLib.o from object list.
# 01j,02may01,kab  Build fioLib.c for Altivec for PPC604
# 01i,04may01,scm  add STRONGARM support
# 01h,11dec00,scm  replace references to ARMSA2 with XScale
# 01g,31aug00,scm  add sa2 support...
# 01f,13nov98,jpd  added vmMpuLib.o for ARM.
# 01e,21jun00,rsh  remove references to dosFsLib and rawFsLib a'la dosfs2
#                  upgrade
# 01d,03mar00,zl   merged SH support into T2
# 01c,27aug98,fle  doc : set ntPassFsLib.c in DOC_FILES (not ntPassFsLib.o)
# 01b,04nov96,p_m  added modules scsiMgrLib.o and scsiCtrlLib.o
# 01a,18jun96,yp   created from 01t of MakeSkel
#
# DESCRIPTION
# This file contains the makefile rules for building the vx library
#
#*/

TGT_DIR=$(WIND_BASE)/target

DOC_FILES=	cacheLib.c clockLib.c dirLib.c dspLib.c dspShow.c \
		envLib.c errnoLib.c \
		excLib.c fioLib.c floatLib.c fppLib.c fppShow.c intLib.c \
		ioLib.c iosLib.c iosShow.c logLib.c memLib.c \
		memPartLib.c memShow.c ntPassFsLib.c pipeDrv.c ptyDrv.c \
		rebootLib.c rt11FsLib.c \
		scsiLib.c scsi1Lib.c cdromFsLib.c \
		scsi2Lib.c scsiCommonLib.c scsiDirectLib.c scsiSeqLib.c \
		scsiMgrLib.c scsiCtrlLib.c \
		selectLib.c sigLib.c \
		symLib.c taskHookLib.c taskHookShow.c \
		tapeFsLib.c \
		taskVarLib.c timerLib.c tyLib.c vmBaseLib.c \
		passFsLib.c unixDrv.c \
		ttyDrv.c

LIB_BASE_NAME	= os

OBJS_COMMON=	cacheLib.o classLib.o classShow.o clockLib.o copyright.o \
	dirLib.o envLib.o errnoLib.o excLib.o \
	ffsLib.o fioLib.o floatLib.o fppLib.o fppShow.o funcBind.o \
	hashLib.o intLib.o ioLib.o iosLib.o iosShow.o logLib.o \
	memLib.o memPartLib.o memShow.o objLib.o pathLib.o \
	pipeDrv.o ptyDrv.o rebootLib.o rt11FsLib.o \
	scsiLib.o scsi1Lib.o cdromFsLib.o \
	scsi2Lib.o scsiCommonLib.o scsiDirectLib.o scsiSeqLib.o \
	scsiMgrLib.o scsiCtrlLib.o \
	selectLib.o sigLib.o smLib.o smPktLib.o \
	symLib.o symShow.o taskHookLib.o taskHookShow.o taskVarLib.o \
	tapeFsLib.o \
	timerLib.o ttyDrv.o tyLib.o vmBaseLib.o vmData.o \
	passFsLib.o unixDrv.o ntPassFsLib.o

OBJS_ARMARCH4	= vmMpuLib.o
OBJS_ARMARCH4_T	= vmMpuLib.o
OBJS_ARMARCH5	= vmMpuLib.o
OBJS_ARMARCH5_T	= vmMpuLib.o
OBJS_STRONGARM	= vmMpuLib.o
OBJS_XSCALE	= vmMpuLib.o

OBJS_SH7600     = dspLib.o dspShow.o 
OBJS_SH7700     = dspLib.o dspShow.o 

OBJS		= $(OBJS_COMMON) $(OBJS_$(CPU))

ifeq ($(CPU),PPC604)
CFLAGS_fioLib.o = $(OPTION_ALTIVEC_C)
endif

include $(TGT_DIR)/h/make/rules.library
