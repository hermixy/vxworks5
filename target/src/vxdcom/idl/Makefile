# Makefile - makefile for target/src/vxdcom/idl
#
# modification history
# --------------------
# 01o,22oct01,dbs  only clean derived files when auto-idlcomp is enabled
# 01n,19oct01,dbs  exclude certain IDL files from p/s generation
# 01m,17oct01,dbs  improve IDL-build rules to allow for veloce hacks
# 01l,11oct01,nel  Include defs file by default as it's required by SIMNT
#                  build.
# 01k,10oct01,dbs  fix inclusion of rules library
# 01j,20aug01,nel  Add rpcDceProto.idl.
# 01i,20jul01,dbs  use new WIDL flags to point output to local dir
# 01h,16jul01,dbs  add separate IDL-compilation rule
# 01g,13jul01,dbs  fix file list
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
# We turn all IDL files into proxy/stub files, headers and UUID files (the
# *_ps.cpp, *.h and *_i.c files, respectively) except for one specific 
# case, ClassFactory.idl, which we have to manually generate the p/s file
# for (for now).
#

DEFS_VXCOM = $(WIND_BASE)/target/h/make/defs.vxcom

include $(DEFS_VXCOM)
include $(VXDCOM_BASE)/h/make/rules.library.$(VXDCOM_BUILD_HOST)

ifeq ($(WIND_HOST_TYPE),x86-linux2)
LN = ln -sf
endif

ifeq ($(WIND_HOST_TYPE),sun4-solaris2)
LN = ln -sf
endif

ifeq ($(WIND_HOST_TYPE),x86-win32)
LN = copy
endif

H_DIR		= $(TGT_DIR)/h
VXCOM_DIR	= ../../vxcom/idl

# Public/private IDL source files (until we stop shipping OPC headers)
IDL_SRCS_pub	= opcda.idl opccomn.idl opc_ae.idl
IDL_SRCS_prv	= RemoteActivation.idl OxidResolver.idl orpc.idl RemUnknown.idl rpcDceProto.idl

# Exclude certain IDL files from p/s generation
IDL_SRCS_nops	= rpcDceProto.idl

HDRS_pub	= $(patsubst %.idl,%.h,$(IDL_SRCS_pub))
HDRS_prv	= $(patsubst %.idl,%.h,$(IDL_SRCS_prv))

IDL_SRCS	= $(IDL_SRCS_prv) $(IDL_SRCS_pub)
I_SRCS		= $(patsubst %.idl,%_i.c,$(IDL_SRCS))
PS_SRCS		= $(patsubst %.idl,%_ps.cpp,$(filter-out $(IDL_SRCS_nops),$(IDL_SRCS)))

# Generated files
PRIVATE_HDRS	= $(patsubst %,$(VXDCOM_PRIVATE_H_DIR)/%,$(HDRS_prv))
PUBLIC_HDRS	= $(patsubst %,$(VXDCOM_PUBLIC_H_DIR)/%,$(HDRS_pub))
GUID_SRCS	= $(patsubst %,$(VXDCOM_GUID_DIR)/%, $(I_SRCS))
PROXY_SRCS	= $(patsubst %,$(VXDCOM_PS_DIR)/%, $(PS_SRCS))

DERIVED_SRCS	= $(GUID_SRCS) $(PROXY_SRCS)

DEPEND_FILE	= depend.$(CPU)$(TOOL)

ifeq ($(VXCOM_NO_AUTOMATIC_IDLCOMP),)
LOCAL_CLEAN	= $(PUBLIC_HDRS) $(PRIVATE_HDRS) $(DERIVED_SRCS)
endif

#
# The default rule builds the IDL-derived headers and installs them. 
# It also builds the proxy/stubs from the vxcom IDL directory,
# then sub-makes the 'lib' target to compile the *_i.c and *_ps.cpp
# files, which didn't exist when the original make was entered, hence 
# the use of a sub-make.
#

# Check for IDL-compilation prevention...
ifeq ($(VXCOM_NO_AUTOMATIC_IDLCOMP),)
Default: idlcomp
	$(MAKE) CPU=$(CPU) TOOL=$(TOOL) lib
endif

# just run IDL compilation
.PHONY: idlcomp
idlcomp: vxcom_ps $(PUBLIC_HDRS) $(PRIVATE_HDRS) $(GUID_SRCS) $(PROXY_SRCS)

# make vxcom p/s sources
.PHONY: vxcom_ps
vxcom_ps:
	(cd ../../vxcom/idl ; $(MAKE) CPU=$(CPU) TOOL=$(TOOL) proxystubs)

verify:
	@echo PRIVATE_HDRS=$(PRIVATE_HDRS)
	@echo PUBLIC_HDRS=$(PUBLIC_HDRS) 
	@echo GUID_SRCS=$(GUID_SRCS) 
	@echo PROXY_SRCS=$(PROXY_SRCS) 
	@echo OBJS=$(OBJS) 
