# Makefile - makefile for vw/src/dhcp
#
# modification history
# --------------------
# 01e,12oct01,tam  added repackaging support
# 01d,27aug97,spm  removed modules from DOC_FILES (no user callable routines)
# 01c,18apr97,spm  restored dhcpcBoot.o module to reduce size of boot ROM
# 01b,03mar97,spm  added modules for DHCP server and relay agent,
#                  removed unused module dhcpcBoot.o 
# 01a,02mar97,vin  sorted and merged bsd44 to tor1_0_1,
# 		   created from 01b of MakeSkel
#
# DESCRIPTION
# This file contains the makefile rules for building the vx library
#
#*/

TGT_DIR=$(WIND_BASE)/target

LIB_BASE_NAME   = net

DOC_FILES=

OBJS=		dhcpc.o dhcpcBoot.o dhcpcState1.o dhcpcState2.o \
		dhcpc_subr.o common_subr.o flushroute.o \
                database.o delarp.o dhcps.o hash.o interface.o \
                dhcpr.o dhcpRelay.o

include $(TGT_DIR)/h/make/rules.library
