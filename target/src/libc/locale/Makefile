# Makefile - makefile for vw/src/libc/locale
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

DOC_FILES=	ansiLocale.c 

## ansiLocale.c includes localeconv.c setlocale.c
#

OBJS= localeconv.o setlocale.o


include $(TGT_DIR)/h/make/rules.library

