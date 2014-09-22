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

DOC_FILES=	ansiStdlib.c

## ansiStdlib.c includes the following files
# 	abort.c abs.c atexit.c atof.c atoi.c atol.c bsearch.c div.c 
#	labs.c ldiv.c multibyte.c qsort.c rand.c strtod.c strtol.c 
#	strtoul.c system.c

OBJS= abort.o abs.o atexit.o atof.o atoi.o atol.o bsearch.o div.o labs.o \
      ldiv.o multibyte.o qsort.o rand.o strtod.o strtol.o strtoul.o system.o

include $(TGT_DIR)/h/make/rules.library

