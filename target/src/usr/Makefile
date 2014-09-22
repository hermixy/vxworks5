# Makefile - makefile for target/src/usr
#
# modification history
# --------------------
# 01i,08dec01,jkf  fixing statTbl.c dependencies, per yogu's T3 work and
#                  SPR#8389.  adding memDrv.c and ramDrv.c into DOCS.
# 01h,06dec01,jkf  SPR#70580, makeStatTbl support readded for Windows host
# 01g,12oct01,tam  added repackaging support
# 01f,24sep01,jkf  moving tarLib.c into src/usr directory, removed
#                  DOC_DIR changes.
# 01e,28jul00,rec  changes to support multiple architectures for ARM
#                  added $(LIBDIR)/*.o to LOCAL_CLEAN
# 01e,21jun00,rsh  upgrade to dosFs 2.0
# 01g,26oct99,jkf  added DOC_OPTS = $(DOC_OPTS_MAN) -category DOSFS2
# 01f,08oct99,jkf  more doc kludges for T2 component, remove for T3
# 01e,08oct99,jkf  added a kludge to get these into the T2 component
#                  documentation directory "dosfs2".  This change should be
#                  be removed for T3, since dosfs2 will no longer be a
#                  component release.
# 01d,14oct98,lrn  added devSplit to OBJS, merged from ms.
# 01c,06jul98,lrn  added new dosFs files, separated I/O utils from usrLib.c
# 01b,02dec96,yp   made build of statTbl.o more reliable. We find makeStatTbl
#		   from search path now
# 01a,17aug96,yp   derived from 01e of MakeSkel
#
# DESCRIPTION
# This file contains the makefile rules for building the vx library
#
#*/

TGT_DIR=$(WIND_BASE)/target

LIB_BASE_NAME   = os

DOC_FILES= usrLib.c usrFsLib.c usrFdiskPartLib.c ramDiskCbio.c \
           tarLib.c memDrv.c ramDrv.c

# the files that contain status codes are listed here. To prevent
# the need to update this list every time a new file with status
# codes is added we wildcard all .h files from the directories
# specifed. The specified directory is expected to have a file
# that lists module numbers.

STATCODEDIRS= $(TGT_DIR)/h
STATFILES= $(wildcard $(foreach dir, $(STATCODEDIRS), $(dir)/*.h))

OBJS=usrLib.o statTbl.o memDrv.o ramDrv.o devSplit.o	\
	usrDosFsOld.o usrFsLib.o usrFdiskPartLib.o ramDiskCbio.o \
        tarLib.o

NODEPENDOBJS=statTbl.c

LOCAL_CLEAN=$(NODEPENDOBJS)

include $(TGT_DIR)/h/make/rules.library

$(subst /,$(DIRCHAR),$(TGT_DIR)/lib/$(LIBNAME)): $(LIBDIR)/statTbl.o

$(LIBDIR)/statTbl.o:	statTbl.c

statTbl.c: $(STATFILES)
	@ $(RM) $@                              
ifneq ($(WIND_HOST_TYPE),x86-win32)
	makeStatTbl $(STATCODEDIRS) > statTbl.c 
else
	makeStatTbl $(STATCODEDIRS)
endif
