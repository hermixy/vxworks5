# Makefile - makefile for bsp/src/drv/pcmcia
#
# modification history
# --------------------
# 01i,12mar02,dat  SPR 72518 Drivers docs in separate directory
# 01h,15jan02,jkf  SPR#72617, removing 01f kludge
# 01g,12oct01,tam  added repackaging support
# 01f,21jun00,rsh  upgrade to dosFs 2.0
# 01g,26oct99,jkf  added DOC_OPTS = $(DOC_OPTS_MAN) -category DOSFS2
# 01f,08oct99,jkf  added a kludge to get dosfs2 changes into the T2 component
#                  documentation directory "dosfs2".  This change should be
#                  be removed for T3, since dosfs2 will no longer be a
#                  component release.
# 01e,03jun98,dat  Fixed EXTRA_INCLUDE to use TGT_DIR, all binaries are
#		   now built for all architectures.
# 01d,29may98,dbt  added support for PENTIUM
# 01c,17nov96,jdi  doc: added pccardLib.c to DOC_FILES.
# 01b,05nov96,yp   changed <CPU> to $(CPU)
# 01a,05nov96,hdn  created from 01c of MakeSkel
#
# DESCRIPTION
# This file contains the makefile rules for building the vx library
#
# All files are binary objects except pccardLib.c which is a source
# driver that the user can customize through macros and code changes.
#
#*/

TGT_DIR=$(WIND_BASE)/target

DOC_FILES= pcmciaLib.c cisLib.c pcic.c tcic.c sramDrv.c \
	   pcmciaShow.c cisShow.c pcicShow.c tcicShow.c pccardLib.c

LIB_BASE_NAME=drv

EXTRA_INCLUDE=-I$(TGT_DIR)/h/

OBJS_COMMON = pcmciaLib.o cisLib.o pcic.o tcic.o sramDrv.o \
	      pcmciaShow.o cisShow.o pcicShow.o tcicShow.o

OBJS        = $(OBJS_COMMON) $(OBJS_$(CPU))

include $(TGT_DIR)/h/make/rules.library

CC_OPTIM=$(CC_OPTIM_DRIVER)
DOC_DIR = $(DOCS_ROOT)/vxworks/ref/drv
DOC_CHAPTER=Driver_Libraries
