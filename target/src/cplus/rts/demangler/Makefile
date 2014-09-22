# Makefile - makefile for target/src/cplus/rts
#
# modification history
# --------------------
# 01b,13mar02,sn   SPR 74061 - integrated EDG demangler
# 01a,26nov01,sn   created
# 
# DESCRIPTION
# This file contains the makefile rules for building the C++ demangler
#

TGT_DIR         = $(WIND_BASE)/target

LIB_BASE_NAME	= cplus
LIB_COMMON	= false

COPIED_DEM_SOURCES     = cplus-dem.c safe-ctype.c edg-decode.c
DEM_NON_COPIED_SOURCES = cp-demangle.c 
COPIED_DEM_HEADERS     = demangle.h safe-ctype.h libiberty.h ansidecl.h

SOURCES                = $(COPIED_DEM_SOURCES) $(DEM_NON_COPIED_SOURCES)
COPIED_FILES           = $(COPIED_DEM_SOURCES) $(COPIED_DEM_HEADERS)

OBJS            = cplusDem.o \
                  cplusDemStub.o \
                  $(patsubst %.c,%.o,$(SOURCES))

BUILD_ALL_OBJS  = true

ifeq ($(WIND_HOST_TYPE),x86-win32)
GNU             = gnu.win32
else
GNU             = gnu
endif

LIBIBERTY_DIR   = $(WIND_BASE)/host/src/$(GNU)/libiberty
INCLUDE_DIR     = $(WIND_BASE)/host/src/$(GNU)/include
EXTRA_DEFINE    = $(OPTION_DEFINE_MACRO)HAVE_STDLIB_H
EXTRA_INCLUDE   = -I.

include $(TGT_DIR)/h/make/rules.library

$(LIBOBJS) : $(COPIED_FILES)

%.c : $(LIBIBERTY_DIR)/%.c
	-$(CP) $(subst /,$(DIRCHAR),$<) $@

%.h : $(INCLUDE_DIR)/%.h
	-$(CP) $(subst /,$(DIRCHAR),$<) $@

# Remove the copied in files.
# Note to source customers: avoid using this unless
# you've installed the GNU Toolkit sources (under
# host/src/gnu).
fullclean : clean
	-$(RM) $(COPIED_FILES)
