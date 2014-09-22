# Makefile - makefile for target/src/wind
#
# modification history
# --------------------
# 01e,15nov01,aeg  added eventShow.o to OBJS
# 01d,31oct01,mas  moved semSm*, msgQSm* to target/src/vxmp/wind
# 01c,04sep01,bwa  Added eventLib.[co], semEvLib.[co] and msgQEvLib.[co]
# 01b,06aug01,mem  Build support for new runtime arrangement
# 01a,17aug96,yp   derived from 01g of MakeSkel
#
# DESCRIPTION
# This file contains the makefile rules for building the vx library
#
#*/

TGT_DIR=$(WIND_BASE)/target

DOC_FILES=	eventLib.c kernelLib.c msgQEvLib.c msgQLib.c msgQShow.c \
		semBLib.c semCLib.c semEvLib.c semLib.c semMLib.c \
		semOLib.c semShow.c taskInfo.c taskLib.c taskShow.c \
		tickLib.c wdLib.c wdShow.c

LIB_BASE_NAME	= wind

OBJS=	eventLib.o eventShow.o kernelLib.o msgQEvLib.o msgQLib.o msgQShow.o \
	schedLib.o semBLib.o semCLib.o semEvLib.o semLib.o \
	semMLib.o semOLib.o semShow.o taskLib.o \
	taskInfo.o taskShow.o tickLib.o wdLib.o wdShow.o windLib.o workQLib.o

include $(TGT_DIR)/h/make/rules.library

