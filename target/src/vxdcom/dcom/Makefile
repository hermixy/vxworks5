# Makefile - makefile for target/src/vxdcom/com
#
# modification history
# --------------------
# 01q,06dec01,nel  Add dcomShow.cpp to docs build.
# 01p,17oct01,dbs  improve build rules to allow for veloce hacks
# 01o,11oct01,nel  Include defs file by default as it's required by SIMNT
#                  build.
# 01n,13jul01,dbs  become common to t2/3
# 01m,30oct00,wsl  change chapter name for documentation to "COM and VxDCOM
#                  Libraries"
# 01l,21mar00,dgp  add SYN_OPTS to correct synopsis build
# 01k,16mar00,dgp  change doc build options again
# 01j,17feb00,dgp  change doc build options for dcomLib.cpp
# 01i,10feb00,dbs  fix doc build
# 01h,19oct99,dbs  fix up doc dirs for references
# 01g,13oct99,dbs  add reference material/man pages
# 01f,28apr99,aim  makefile rework
# 01e,22apr99,aim  added dcom.out rule
# 01d,21apr99,dbs  add ntlmssp to build
# 01c,21apr99,dbs  add RpcOleMessage to build
# 01b,21apr99,dbs  change RPCMSG to RPCPDU
# 01a,20apr99,dbs  created during Grand Renaming
#
# DESCRIPTION
# This file contains the makefile rules for building the vx library
#
#

DOC_FILES=dcomLib.cpp dcomShow.cpp

DEFS_VXCOM = $(WIND_BASE)/target/h/make/defs.vxcom

include $(DEFS_VXCOM)
include $(VXDCOM_BASE)/h/make/rules.library.$(VXDCOM_BUILD_HOST)

# to change the DOC_DIR, its definition has to be placed after the include,
# because a default value for DOC_DIR is set in the defs.default file

DOC_DIR         = $(DOCS_ROOT)/vxworks/ref/vxdcom
DOC_CHAPTER     = COM_and_VxDCOM_Libraries
DOC_OPTS        = -config C2html
DOC_OPTS_CPP    =
SYN_OPTS	= -config C2html
