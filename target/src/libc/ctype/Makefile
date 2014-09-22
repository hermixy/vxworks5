# Makefile - makefile for vw/src/libc/ctype
#
# modification history
# --------------------
# 01b,12oct01,tam  added repackaging support
# 01a,18jun96,yp   created from 01c of MakeSkel
#
# DESCRIPTION
# This file contains the makefile rules for building the vx library
#
#*/

TGT_DIR=$(WIND_BASE)/target

LIB_BASE_NAME   = os

DOC_FILES=	ansiCtype.c

## ansiCtype.c includes the following files
#       isalnum.c isalpha.c iscntrl.c isdigit.c isgraph.c 
#	islower.c isprint.c ispunct.c isspace.c isupper.c 
#	isxdigit.c tolower.c toupper.c



OBJS=	__ctype_tab.o isalnum.o isalpha.o iscntrl.o isdigit.o isgraph.o \
      	islower.o isprint.o ispunct.o isspace.o isupper.o isxdigit.o \
	tolower.o toupper.o

include $(TGT_DIR)/h/make/rules.library

