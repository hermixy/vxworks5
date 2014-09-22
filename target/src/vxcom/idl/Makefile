# Makefile - makefile for target/src/vxcom/idl
#
# modification history
# --------------------
# 01o,22oct01,dbs  only clean derived files when auto-idlcomp is enabled
# 01n,17oct01,dbs  improve IDL-build rules to allow for veloce hacks
# 01m,11oct01,nel  Include defs file by default as it's required by SIMNT
#                  build.
# 01l,10oct01,nel  Add rule to build widl.
# 01k,20jul01,dbs  remove symlinking of output headers
# 01j,16jul01,dbs  add separate IDL-compilation rule
# 01i,13jul01,dbs  add ConnectionPoint IDL
# 01h,29jun01,nel  Correct creation of logical links.
# 01g,27jun01,dbs  defeat p/s file compilation
# 01f,09mar01,nel  Change rules so that view private symbolic links are
#                  created for headers/idl files in target/h.
# 01e,08mar01,nel  Remove manualclean rule and restore rclean rule
# 01d,25oct00,nel  Add manualclean rule to delete widl generated files and
#                  modify rclean rule to NOT delete widl generated files.
# 01c,14jun00,nel  make dependencies work correctly for idl files.
# 01b,30may00,nel  Renamed Stream.idl to vxStream.idl.
# 01a,09feb00,dbs  created
#
# This Makefile builds the IDL-derived sources first, then compiles them
# as per normal target build rule 'lib'
#
# We turn all IDL files into headers and UUID files (the *.h and *_i.c
# files, respectively).
#

DEFS_VXCOM = $(WIND_BASE)/target/h/make/defs.vxcom

include $(DEFS_VXCOM)
include $(VXCOM_BASE)/h/make/rules.library.$(VXDCOM_BUILD_HOST)

ifeq ($(WIND_HOST_TYPE),x86-linux2)
LN = ln -sf
endif

ifeq ($(WIND_HOST_TYPE),sun4-solaris2)
LN = ln -sf
endif

ifeq ($(WIND_HOST_TYPE),x86-win32)
LN = copy
endif

# Public/private IDL source files
IDL_SRCS_pub	= $(wildcard *.idl)
IDL_SRCS_prv	=

HDRS_pub	= $(patsubst %.idl,%.h,$(IDL_SRCS_pub))
HDRS_prv	= $(patsubst %.idl,%.h,$(IDL_SRCS_prv))

IDL_SRCS	= $(IDL_SRCS_prv) $(IDL_SRCS_pub)
I_SRCS		= $(patsubst %.idl,%_i.c,$(IDL_SRCS))
PS_SRCS		= $(patsubst %.idl,%_ps.cpp,$(IDL_SRCS))

# Generated files
PRIVATE_HDRS	= $(patsubst %,$(VXCOM_PRIVATE_H_DIR)/%,$(HDRS_prv))
PUBLIC_HDRS	= $(patsubst %,$(VXCOM_PUBLIC_H_DIR)/%,$(HDRS_pub))
GUID_SRCS	= $(patsubst %,$(VXCOM_GUID_DIR)/%, $(I_SRCS))
PROXY_SRCS	= $(patsubst %,$(VXCOM_PS_DIR)/%, $(PS_SRCS))

DERIVED_SRCS	= $(GUID_SRCS) $(PROXY_SRCS)

ifeq ($(VXCOM_NO_AUTOMATIC_IDLCOMP),)
LOCAL_CLEAN	= $(PUBLIC_HDRS) $(PRIVATE_HDRS) $(DERIVED_SRCS)
endif

DEPEND_FILE	= depend.$(CPU)$(TOOL)

CHMOD		= chmod

.PRECIOUS : $(GUID_SRCS) $(PUBLIC_HDRS)

#
# The default rule builds the IDL-derived headers directly into the
# public header dir, then sub-makes the 'lib' target to compile the .c
# files, which didn't exist when the original make was entered, hence 
# the use of a sub-make.
#

# Check for IDL-compilation prevention...
ifeq ($(VXCOM_NO_AUTOMATIC_IDLCOMP),)
Default: idlcomp 
	$(MAKE) CPU=$(CPU) TOOL=$(TOOL) lib
endif

# just run IDL compilation(s) via implicit dependencies
.PHONY: idlcomp
idlcomp: $(PUBLIC_HDRS) $(GUID_SRCS)

.PHONY: proxystubs
proxystubs: $(PROXY_SRCS)

verify:
	@echo IDL_SRCS=$(IDL_SRCS)
	@echo PRIVATE_HDRS=$(PRIVATE_HDRS)
	@echo PUBLIC_HDRS=$(PUBLIC_HDRS) 
	@echo GUID_SRCS=$(GUID_SRCS) 
	@echo PROXY_SRCS=$(PROXY_SRCS) 
	@echo OBJS=$(OBJS) 
