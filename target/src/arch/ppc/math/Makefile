# Makefile - makefile for vw/src/arch/ppc/math
#
# modification history
# --------------------
# 01i,12oct01,tam  added support for new packaging scheme
# 01h,23aug01,pch  Add PPC440
# 01g,26oct00,s_m  renamed PPC405 cpu types
# 01f,11oct00,sm   fixed typo in PPC405
# 01e,06jul00,alp  Added PPC405 support
# 01d,18aug98,tpr  added PowerPC EC 603 support.
# 01c,10dec96,tpr  reworked to support usSoftware library for PPC403 and PPC860.
# 01b,10nov96,tpr  added mathHardLib.o andmathSoftLib.o
# 01a,05nov96,tam  written.
#
# DESCRIPTION
# This file contains the makefile rules for building the vxWorks architecture
# dependant library.
#
# NOTE
# The binaries without sources have to be added to the src-arch ressource file
#*/

TGT_DIR=$(WIND_BASE)/target

LIB_BASE_NAME	= arch

SUBDIRS_PPC403		= usSoftwareLib
SUBDIRS_PPC405F		= motorolaLib
SUBDIRS_PPC405		= $(SUBDIRS_PPC403)
SUBDIRS_PPC440		= $(SUBDIRS_PPC403)
SUBDIRS_PPC505		= 
SUBDIRS_PPC509		= $(SUBDIRS_PPC603)
SUBDIRS_PPC555		= $(SUBDIRS_PPC603)
SUBDIRS_PPC603		= motorolaLib
SUBDIRS_PPCEC603	= $(SUBDIRS_PPC403)
SUBDIRS_PPC604		= $(SUBDIRS_PPC603)
SUBDIRS_PPC860		= $(SUBDIRS_PPC403)

OBJ_PPC403		= mathSoftLib.o
OBJ_PPC405		= mathSoftLib.o
OBJ_PPC405F		= mathHardLib.o
OBJ_PPC440		= mathSoftLib.o
OBJ_PPC505		= 
OBJ_PPC509		= $(OBJ_PPC603)
OBJ_PPC555		= $(OBJ_PPC603)
OBJ_PPC603		= mathHardLib.o
OBJ_PPCEC603		= $(OBJ_PPC403)
OBJ_PPC604		= $(OBJ_PPC603)
OBJ_PPC860		= $(OBJ_PPC403)

OBJS			= $(OBJ_$(CPU))

SUBDIRS			= $(SUBDIRS_$(CPU))

include $(TGT_DIR)/h/make/rules.library
