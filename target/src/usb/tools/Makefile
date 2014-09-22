# Makefile - makefile for bsp/src/drv/sio
#
# modification history
# --------------------
# 01b,19oct01,tam  added support for new archive directory hierarchy
# 01a,25jan00,wef   created
#
# DESCRIPTION
# This file contains the makefile rules for building the usb tools library
#
#*/

TGT_DIR=$(WIND_BASE)/target

DOC_FILES=	cmdParser.c


LIB_BASE_NAME   = usb

OBJS =		cmdParser.o


include $(TGT_DIR)/h/make/rules.library

DOC_DIR         = $(DOCS_ROOT)/vxworks/ref/usb
DOC_CHAPTER     = USB_libraries
DOC_OPTS        = $(DOC_OPTS_MAN) -category USB
