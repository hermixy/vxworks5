# Makefile - makefile for vw/src/posix
#
# modification history
# --------------------
# 01e,12oct01,tam  added repackaging support
# 01d,11sep00,jgn  add _pthreadLib.c (SPR #33375)
# 01c,17jul00,jgn  merge DOT-4 pthreads changes
# 01b,13dec96,p_m  renamed symSyncLib to symTblLockLib
# 01a,18jun96,yp   created from 01d of MakeSkel
#
# DESCRIPTION
# This file contains the makefile rules for building the vx library
#
#*/

TGT_DIR=$(WIND_BASE)/target

LIB_BASE_NAME   = os

DOC_FILES=	aioPxLib.c aioPxShow.c aioSysDrv.c mmanPxLib.c \
		mqPxLib.c mqPxShow.c semPxShow.c schedPxLib.c semPxLib.c \
		ftruncate.c pthreadLib.c

OBJS=	aioPxLib.o aioPxShow.o aioSysDrv.o ioQLib.o mmanPxLib.o \
	mqPxLib.o mqPxShow.o mutexPxLib.o posixNameLib.o pthreadLib.o \
	_pthreadLib.o semPxShow.o schedPxLib.o _schedPxLib.o semPxLib.o \
	symTblLockLib.o ftruncate.o

include $(TGT_DIR)/h/make/rules.library
