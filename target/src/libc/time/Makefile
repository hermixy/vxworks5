# Makefile - makefile for vw/src/libc/ctype
#
# modification history
# --------------------
# 01b,12oct01,tam  added repackaging support
# 01a,18jun96,yp   created from 01d of MakeSkel
#
# DESCRIPTION
# This file contains the makefile rules for building the vx library
#
#*/

TGT_DIR=$(WIND_BASE)/target

LIB_BASE_NAME   = os

DOC_FILES=	ansiTime.c

## ansiTime.c includes the following files
#	asctime.c clock.c ctime.c difftime.c gmtime.c localtime.c 
#	mktime.c strftime.c time.c

OBJS= locTime.o asctime.o clock.o ctime.o difftime.o gmtime.o localtime.o \
      mktime.o strftime.o time.o

include $(TGT_DIR)/h/make/rules.library

