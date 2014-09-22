# Makefile - makefile for vw/src/netwrs/resolv
#
# Copyright 1997-1998 Wind River Systems, Inc.
#
# modification history
# --------------------
# 01d,08aug01,mem  New runtime arrangement support.
# 01c,06oct98,jmp  replaced resolvLib.c by resolvLibDoc.c in DOC_FILES.
# 01b,30apr97,jag  added DOC_FILES
# 01a,02mar97,vin  sorted and merged bsd44 to tor1_0_1, merge with 
#		   01a of MakeSkel	
#
# DESCRIPTION
# This file contains the makefile rules for building the vx library
#
#*/

TGT_DIR=$(WIND_BASE)/target

DOC_FILES=      resolvLibDoc.c

LIB_BASE_NAME=net

OBJS=  gethostnamadr.o  res_comp.o  \
       res_debug.o res_mkquery.o res_query.o  \
       res_send.o resolvLib.o

include $(TGT_DIR)/h/make/rules.library
