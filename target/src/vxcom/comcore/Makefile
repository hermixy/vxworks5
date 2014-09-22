# Makefile - makefile for target/src/vxcom/comcore
#
# modification history
# --------------------
# 01e,06dec01,nel  Add comCoreLib.c to the docs build.
# 01d,17oct01,dbs  improve build rules to allow for veloce hacks
# 01c,11oct01,nel  Include defs file by default as it's required by SIMNT
#                  build.
# 01b,16jul01,dbs  simplify makefile structure
# 01a,19jun01,nel  created.
#
# DESCRIPTION
# This file contains the makefile rules for building the vx library
#
#

DOC_FILES       = comCoreLib.c

DEFS_VXCOM = $(WIND_BASE)/target/h/make/defs.vxcom

include $(DEFS_VXCOM)
include $(VXCOM_BASE)/h/make/rules.library.$(VXDCOM_BUILD_HOST)

DOC_DIR         = $(DOCS_ROOT)/vxworks/ref/vxdcom
DOC_CHAPTER     = COM_and_VxDCOM_Libraries
DOC_OPTS        = -config C2html
DOC_OPTS_CPP    =
SYN_OPTS        = -config C2html

