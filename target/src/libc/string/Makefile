# Makefile - makefile for vw/src/libc/string
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

DOC_FILES=	ansiString.c

## ansiString.c includes the following files
#	memchr.c memcmp.c memcpy.c memmove.c memset.c strcat.c 
#	strchr.c strcmp.c strcoll.c strcpy.c strcspn.c strerror.c 
#	string.c strlen.c strncat.c strncmp.c strncpy.c strpbrk.c 
#	strrchr.c strspn.c strstr.c strtok.c strtok_r.c strxfrm.c


OBJS=	memchr.o memcmp.o memcpy.o memmove.o memset.o strcat.o strchr.o \
	strcmp.o strcoll.o strcpy.o strcspn.o strerror.o strlen.o strncat.o \
        strncmp.o strncpy.o strpbrk.o strrchr.o strspn.o strstr.o strtok.o \
        strtok_r.o strxfrm.o xstate.o

include $(TGT_DIR)/h/make/rules.library

