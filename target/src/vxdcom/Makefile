# Makefile - makefile for target/src/vxdcom
#
# modification history
# --------------------
# 01j,22oct01,dbs  add extra-subdirs option
# 01i,17oct01,dbs  improve IDL-build rules to allow for veloce hacks, remove
#                  build script
# 01h,28sep01,nel  Remove vxdcom_build_host file check.
# 01g,28sep01,nel  Veloce Development Branch
# 01f,21sep01,nel  Change rules.vxcom to defs.vxcom.
# 01e,20sep01,nel  Add target/h/make/rules.vxcom.
# 01d,02aug01,dbs  no need to figure out build-host
# 01c,24jul01,dbs  add unix IDL compilation support
# 01b,16jul01,dbs  add unix build support
# 01a,13jul01,dbs  copied from vxcom makefile
#
# DESCRIPTION
# This file contains the makefile rules for building the vxdcom library
#
#

DEFS_VXCOM = $(WIND_BASE)/target/h/make/defs.vxcom

include $(DEFS_VXCOM)

# Force 'idl' directory to be made first
SUBDIRS		= idl dcom rpc reactor $(VXDCOM_EXTRA_SUBDIRS)

# Check for build-kind, which is either T2/T3 or Unix
ifeq ($(VXDCOM_BUILD_HOST),unix)
LIB            	= $(HOST_LIB_DIR)/libvxdcom.so
BASE_DIR	= $(VXDCOM_BASE)
_VXCOM_INC_DIRS = h unixh debug com os/$(WIND_HOST_TYPE)/vxemu
INC_DIRS        = h unixh $(SUBDIRS) $(addprefix ../vxcom/,$(_VXCOM_INC_DIRS))
SRC_DIRS        = $(SUBDIRS)
else
# Building for VxWorks, so let the default SUBDIRS rule take over
endif

include h/make/rules.library.$(VXDCOM_BUILD_HOST)

# Special rule for building IDL-derived files
.PHONY: idlcomp
idlcomp:
	$(MAKE) -C idl DEFS_VXCOM=$(DEFS_VXCOM) CPU=$(CPU) TOOL=$(TOOL) idlcomp


