# Makefile - makefile for vw/src/tffs
#
# modification history
# --------------------
# 01e,06aug01,mem  Build support for new runtime arrangement.
# 01d,23apr01,mem  Remove nftllite.c
# 01c,16jan98,yp   added to DOC_FILES
# 01b,11dec97,hdn  renamed flsysvxw to tffsLib.
# 01a,27may97,yp   created 
#
# DESCRIPTION
# This file contains the makefile rules for building the vx library
#
#*/

TGT_DIR=$(WIND_BASE)/target

EXTRA_INCLUDE=-I$(TGT_DIR)/h/tffs

DOC_FILES= tffsDrv.c

LIB_BASE_NAME=tffs

VXWORKS_DRIVER_OBJS	= tffsDrv.o tffsLib.o flbase.o
DOS_FAT_LAYER_OBJS      = fatlite.o flparse.o dosformt.o
FTL_LAYER_OBJS          = ftllite.o fltl.o ssfdc.o
MTD_LAYER_OBJS          = flflash.o reedsol.o
SOCKET_LAYER_OBJS       = flsocket.o
SOCKET_POLLING_OBJS	= backgrnd.o

OBJS=   $(DOS_FAT_LAYER_OBJS) $(FTL_LAYER_OBJS) \
	$(MTD_LAYER_OBJS) $(SOCKET_LAYER_OBJS)  \
	$(SOCKET_POLLING_OBJS) $(VXWORKS_DRIVER_OBJS)

include $(TGT_DIR)/h/make/rules.library

CFLAGS += -fsigned-char

# end of target/src/tffs/Makefile
