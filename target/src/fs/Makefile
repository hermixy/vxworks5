# Makefile - makefile for target/src/fs
#
# modification history
# --------------------
# 01f,12oct01,tam  added repackaging support
# 01e,24sep01,jkf  move tarLib to src/usr, remove DOC_XX overrides.
# 01d,26oct99,jkf  added -category to DOC_OPTS
# 01c,08oct99,jkf  added a kludge to get these into the T2 component
#                  documentation directory "dosfs2".  This change should be
#                  be removed for T3, since dosfs2 will no longer be a
#                  component release.
# 01b,15oct98,lrn  moved rawFsLib to this directory
# 01a,05jul98,lrn  derived from target/src/usr
#
# DESCRIPTION
# This file contains the makefile rules for building the vx library
#
#*/

TGT_DIR=$(WIND_BASE)/target

DOC_FILES=		\
    cbioLib.c		\
    dcacheCbio.c	\
    dosFsFmtLib.c	\
    dosFsLib.c		\
    dpartCbio.c		\
    rawFsLib.c

LIB_BASE_NAME	= os

OBJS=	\
    cbioLib.o	\
    dcacheCbio.o	\
    dosChkLib.o		\
    dosDirOldLib.o	\
    dosFsFat.o		\
    dosFsFmtLib.o	\
    dosFsLib.o		\
    dosVDirLib.o	\
    dpartCbio.o		\
    print64Lib.o	\
    rawFsLib.o


include $(TGT_DIR)/h/make/rules.library
