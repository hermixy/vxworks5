# Makefile - makefile for /target/src/usb
#
# modification history
# --------------------
# 01c,19oct01,tam  added support for new archive directory hierarchy
# 01b,05apr00,wef   added usbDescrCopy.c
# 01a,18jan00,wef   created from 01t of MakeSkel
#
# DESCRIPTION
# This file contains the makefile rules for building the usb library
#
#*/

TGT_DIR=$(WIND_BASE)/target

DOC_FILES=	ossLib.c usbdCoreLib.c usbDescrCopyLib.c usbdLib.c \
		usbHandleLib.c usbHcdLib.c usbLib.c usbListLib.c \
		usbQueueLib.c

LIB_BASE_NAME	= usb

OBJS=		ossLib.o usbdCoreLib.o usbDescrCopyLib.o usbdLib.o \
		usbHandleLib.o usbHcdLib.o usbLib.o usbListLib.o \
		usbQueueLib.o

include $(TGT_DIR)/h/make/rules.library

DOC_DIR         = $(DOCS_ROOT)/vxworks/ref/usb
DOC_CHAPTER     = USB_libraries
DOC_OPTS        = $(DOC_OPTS_MAN) -category USB
