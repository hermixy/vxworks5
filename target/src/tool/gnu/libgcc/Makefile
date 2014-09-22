# Makefile - makefile for extracting objects from libgcc.a
#
# modification history
# --------------------
# 01r,13nov01,tam  moved archive into new directory hierarchy
# 01q,13nov01,sn   clean up
# 01p,12nov01,sn   back out last change
# 01o,06nov01,sn   specify an explicit list of objects rather than extracting everything
# 01x,30oct01,sn   moved to tool/gnu/libgcc
# 01w,12oct01,tam  added support for repackaging
# 01v,05oct01,mem  Added _fixunsdfsi & _fixunssfsi to OBJS_FLOAT.
# 01u,26sep01,dat  make rules moved to h/tool/...
# 01t,27jul01,to   remove ARM specific stuff
# 01s,23jul01,scm  change XScale name to conform to coding standards...
# 01r,07aug01,mem  New runtime arrangement support.
# 01q,04may01,scm  add support for XScale and STRONGARM...
# 01p,23may01,kab  Patch for Win32 directory seperator
# 01o,22may01,max  To fix a simulator build problem
# 01n,09apr01,max  Introducing AltiVec run time support symbols
# 01m,01mar01,t_m  merge in XScale changes
# 01l,22mar01,mrs  Fix slashes for windows.
# 01k,07feb01,sn   don't pull in tramp.o
# 01j,18jan01,mem  Strip debug info for MIPS.
# 01i,14jun00,cdp  added _fixunsdfsi, _fixunssfsi for ARM, Thumb.
# 01h,00jun00,sn   create "reduced" version of libgcc.a
#                  deprecated use of this Makefile
# 01g,09mar00,cdp  ARM merge: added extra objects for ARM, Thumb.
# 01f,02mar00,dra  Added dp-bit.o and fp-bit.o to file list for soft-float
#                  MIPS targets.
# 01e,18feb00,tdl  added ucmpdi2 / lshrdi3
# 01d,17feb00,tdl  added TRAMP_O stuff (used by dejagnu testing)
# 01c,23nov99,sn   ensure that TOOLENV is known when we use it
# 01b,17nov99,sn   added more long long objects
# 01a,15nov99,sn   wrote
#
# NOTE
# This Makefile is now deprecated. Please do not add further
# libgcc components or extend the list of objects we extract
# below. In future we may remove this subdirectory
# altogether.
#
# DESCRIPTION
# This file contains the definitions needed for extracting objects
# from libgcc.a (GNU C and C++ compiler support).

TGT_DIR=$(WIND_BASE)/target
TOOL=gnu

LIB_BASE_NAME	= gcc
LIB_COMMON	= true

GCCLIB		= $(WIND_BASE)/host/$(WIND_HOST_TYPE)/lib/gcc-lib
GNULIBDIR	= $(GCCLIB)/$(CC_MACHINE)/$(CC_VERSION)
GNULIBRARY	= libgcc.a

CONFIGLETTE_NAME    = gcc_intrinsics

include $(TGT_DIR)/src/tool/gnu/rules.gnu
