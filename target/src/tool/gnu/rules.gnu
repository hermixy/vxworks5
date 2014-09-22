# rules.gnu - rules for extracting objects from GNU libraries
#
# modification history
# --------------------
# 01b,06nov01,sn   define OBJ_PREFIX
# 01a,15oct01,sn   wrote
#
# DESCRIPTION
# Generic rules for extracting objects from GNU libraries.
# Subdirectory Makefiles for specific libraries should
# set GNULIBDIR and GNULIBRARY and include this file.
#
  
# Ask the compiler driver for the version (e.g. gcc-2.6) and
# machine (e.g. powerpc-wrs-vxworksae). This info may
# be used by subdirectory Makefiles to compute GNULIBDIR.

CC_VERSION	= $(shell $(CC) -dumpversion)
CC_MACHINE	= $(shell $(CC) -dumpmachine)

MULTISUBDIR	= $(CPU)$(TOOL)

TOOL_LIB	= $(GNULIBDIR)/$(MULTISUBDIR)/$(GNULIBRARY)

OBJ_PREFIX      = _x_gnu_

include $(TGT_DIR)/src/tool/rules.tool
