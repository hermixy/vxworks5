# Makefile - makefile for vw/src/rip
#
# modification history
# --------------------
# 01g,23oct01,tam  updated for re-packaging
# 01f,15oct01,rae  merge from truestack ver 01f, base o1e (add md5.o)
# 01e,10nov00,spm  merged from version 01e of tor3_x branch (SPR #21906 fix)
# 01d,11sep98,spm  removed unused trace.c module (SPR #22350)
# 01c,11aug97,gnn  changed the name of timer.c to ripTimer.c
# 01b,07apr97,gnn  added MIB-II RIP library.
# 01a,02mar97,vin  sorted and merged bsd44 to tor1_0_1, merge with 
#		   01b of MakeSkel	
#
# DESCRIPTION
# This file contains the makefile rules for building the vx library
#
#*/

TGT_DIR=$(WIND_BASE)/target

DOC_FILES= ripLib.c m2RipLib.c

LIB_BASE_NAME   = net

OBJS= 	routed_if.o ripLib.o tables.o inet.o output.o ripTimer.o af.o \
	input.o startup.o m2RipLib.o md5.o

include $(TGT_DIR)/h/make/rules.library
