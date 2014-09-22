# Makefile - makefile for target/src/vxcom/com
#
# modification history
# --------------------
# 01p,06dec01,nel  Add comShow.cpp to docs build.
# 01o,17oct01,dbs  improve build rules to allow for veloce hacks
# 01n,11oct01,nel  Include defs file by default as it's required by SIMNT
#                  build.
# 01m,16jul01,dbs  simplify makefile structure
# 01l,30oct00,wsl  change chapter name for documentation to "COM and VxDCOM"
# 01k,21mar00,dgp  add SYN_OPTS to correct synopsis build
# 01j,16mar00,dgp  change doc build options again
# 01i,17feb00,dgp  change doc build options for comLib.cpp
# 01h,10feb00,dbs  fix doc build
# 01g,19oct99,dbs  fix up doc dirs for references
# 01f,13oct99,dbs  add reference material/man pages
# 01e,28apr99,aim  makefile rework
# 01d,26apr99,dbs  remove DebugAllocator
# 01c,26apr99,dbs  add allocators to build
# 01b,22apr99,aim  added vxdcom.out rule
# 01a,20apr99,dbs  created during Grand Renaming
#
# DESCRIPTION
# This file contains the makefile rules for building the vx library
#
#

DOC_FILES       = comLib.cpp comShow.cpp

DEFS_VXCOM = $(WIND_BASE)/target/h/make/defs.vxcom
include $(DEFS_VXCOM)
include $(VXCOM_BASE)/h/make/rules.library.$(VXDCOM_BUILD_HOST)

# to change the DOC_DIR, its definition has to be placed after the include,
# because a default value for DOC_DIR is set in the defs.default file

DOC_DIR         = $(DOCS_ROOT)/vxworks/ref/vxdcom
DOC_CHAPTER     = COM_and_VxDCOM_Libraries
DOC_OPTS        = -config C2html
DOC_OPTS_CPP    =
SYN_OPTS        = -config C2html
