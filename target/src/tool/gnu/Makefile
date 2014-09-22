# Makefile - makefile for target/src/tool/gnu
#
# modification history
# --------------------
# 01b,30oct01,sn   moved under target/src/tool
# 01a,15nov99,sn   wrote
# 
# DESCRIPTION
# The GNU libraries are compiled as part of the cross-toolchain
# build rather than as part of the VxWorks runtime build.
# They are deposited in host/$(WIND_HOST_TYPE)/... in
# subdirectories labelled $(CPU)$(TOOL). Here we simply extract 
# specific objects from these libraries and deposit them in the 
# appropriate VxWorks object directory.
#
# Each subdirectory of target/src/tool/gnu corresponds to a particular
# GNU library. Its Makefile should define:
#     GNULIBDIR  - the location of the library
#     GNULIBRARY - the name of the library, e.g. libgcc.a
#     OBJS       - a list of modules to extract from the library
# The Makefile should end by including $(TGT_DIR)/src/tool/gnu/rules.gnu,
# which does all the actual work.
#

TGT_DIR	= $(WIND_BASE)/target

include $(TGT_DIR)/h/make/rules.library

TOOL	= gnu
