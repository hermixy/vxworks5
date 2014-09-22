# Makefile - makefile for target/src/vxcom
#
# modification history
# --------------------
# 01k,22oct01,dbs  add extra-subdirs option
# 01j,17oct01,dbs  improve IDL-build rules to allow for veloce hacks, remove
#                  build script
# 01i,28sep01,nel  Remove vxdcom_build_host file check.
# 01h,21sep01,nel  Change rules.vxcom to defs.vxcom.
# 01g,20sep01,nel  Add target/h/make/rules.vxcom.
# 01f,02aug01,dbs  no need to figure out build-host
# 01e,24jul01,dbs  add unix IDL compilation support
# 01d,16jul01,dbs  add unix build support
# 01c,13jul01,dbs  remove unneccesary rules
# 01b,28jun01,dbs  fix build in non-std places
# 01a,25jun01,dbs  created from original T2/3 Makefiles
#
# DESCRIPTION
# This file contains the makefile rules for building the vxcom library
#
#

DEFS_VXCOM = $(WIND_BASE)/target/h/make/defs.vxcom

include $(DEFS_VXCOM)

# Force 'idl' directory to be made first
SUBDIRS		= idl com debug comcore os $(VXCOM_EXTRA_SUBDIRS)

# Handle unix-build differently...
ifeq ($(VXDCOM_BUILD_HOST),unix)
LIB            	= $(HOST_LIB_DIR)/libvxcom.so
BASE_DIR        = $(VXCOM_BASE)
INC_DIRS        = h unixh com debug comcore os/$(WIND_HOST_TYPE)/vxemu
SRC_DIRS        = comcore com debug idl os os/$(WIND_HOST_TYPE)/vxemu
else
# Building for VxWorks, so allow the usual SUBDIRS rule to take over
endif

include $(VXCOM_BASE)/h/make/rules.library.$(VXDCOM_BUILD_HOST)

# Special rule for building IDL-derived files
.PHONY: idlcomp
idlcomp:
	$(MAKE) -C idl DEFS_VXCOM=$(DEFS_VXCOM) CPU=$(CPU) TOOL=$(TOOL) idlcomp

