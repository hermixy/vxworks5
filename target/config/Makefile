# Makefile - makefile for target/config
#
# modification history
# --------------------
# 01j,30mar02,dat  new clean_release target, SPR 72823
# 01i,27nov01,wsl  fix docs built in wrong destination dir; remove intman rule
# 01h,01oct01,dat  Update for T2.2, removed old make includes
# 01g,26apr01,mem  Only build man pages in BSPs with target.nr files.
# 01f,31may99,fle  added intman rule for internal doc builds
# 01e,02nov98,fle  added man target for win32 hosts
# 01d,04dec97,jmp  added synopsis rules
# 01c,19nov96,yp   removed SIBLING_CPUS support
# 01b,06nov96,yp   added support for SIBLING_CPUS
# 01a,18aug96,yp   derived from 01j of MakeSkel
#
# DESCRIPTION
# This file contains the makefile rules for building the BSPs associated
# with a particular CPU
#
#

CPU=MC68020
TOOL=gnu
TGT_DIR=$(WIND_BASE)/target

include $(TGT_DIR)/h/make/defs.bsp

TARGET=default

exe: 
	@ $(TCL) $(BSPBUILD_TCL) $(CPU) $(TOOL) $(TARGET) $(TGT_DIR) \
	    $(wildcard */Makefile)

ifneq ($(WIND_HOST_TYPE),x86-win32)
man:
	@ DIRS=`find ./* -type d -print -prune`  ; \
	for DIR in $$DIRS ; \
	do \
	    if test -f $$DIR/Makefile ; then \
		( echo $$DIR ; cd $$DIR ; \
		  $(MAKE) TGT_DIR=$(TGT_DIR) man ) ; \
	    fi ; \
	done

synopsis:
clean_synopsis:
else
man:
	$(WTXTCL) ./Makefile.win32.tcl man $*
synopsis:
	@echo *** can\'t do synopsis on windows
	@exit (1)
endif


release:
	$(MAKE) CPU=$(CPU) TOOL=$(TOOL) TGT_DIR=$(TGT_DIR) TARGET=release 

vxWorks:
	$(MAKE) CPU=$(CPU) TOOL=$(TOOL) TGT_DIR=$(TGT_DIR) TARGET=vxWorks 

vxWorks_rom:
	$(MAKE) CPU=$(CPU) TOOL=$(TOOL) TARGET=vxWorks_rom TGT_DIR=$(TGT_DIR) 

vxWorks_rom.hex:
	$(MAKE) CPU=$(CPU) TOOL=$(TOOL) TARGET=vxWorks_rom.hex \
	    TGT_DIR=$(TGT_DIR) 

vxWorks.st:
	$(MAKE) CPU=$(CPU) TOOL=$(TOOL) TARGET=vxWorks.st TGT_DIR=$(TGT_DIR) 

vxWorks.st_rom:
	$(MAKE) CPU=$(CPU) TOOL=$(TOOL) TARGET=vxWorks.st_rom \
		TGT_DIR=$(TGT_DIR)

vxWorks.st_rom.hex:
	$(MAKE) CPU=$(CPU) TOOL=$(TOOL) TARGET=vxWorks.st_rom.hex \
		TGT_DIR=$(TGT_DIR) 

vxWorks.res_rom:
	$(MAKE) CPU=$(CPU) TOOL=$(TOOL) TARGET=vxWorks.res_rom \
		TGT_DIR=$(TGT_DIR)

vxWorks.res_rom.hex:
	$(MAKE) CPU=$(CPU) TOOL=$(TOOL) TARGET=vxWorks.res_rom.hex \
		TGT_DIR=$(TGT_DIR)

bootrom:
	$(MAKE) CPU=$(CPU) TOOL=$(TOOL) TARGET=bootrom TGT_DIR=$(TGT_DIR) 

bootrom.hex:
	$(MAKE) CPU=$(CPU) TOOL=$(TOOL) TARGET=bootrom.hex TGT_DIR=$(TGT_DIR) 

bootrom_res:
	$(MAKE) CPU=$(CPU) TOOL=$(TOOL) TARGET=bootrom_res TGT_DIR=$(TGT_DIR) 

bootrom_res.hex:
	$(MAKE) CPU=$(CPU) TOOL=$(TOOL) TARGET=bootrom_res.hex \
		TGT_DIR=$(TGT_DIR)

bootrom_uncmp:
	$(MAKE) CPU=$(CPU) TOOL=$(TOOL) TARGET=bootrom_uncmp \
		TGT_DIR=$(TGT_DIR)

bootrom_uncmp.hex:
	$(MAKE) CPU=$(CPU) TOOL=$(TOOL) TARGET=bootrom_uncmp.hex \
		TGT_DIR=$(TGT_DIR)

clean rclean:
	$(MAKE) CPU=$(CPU) TOOL=$(TOOL) TARGET=clean TGT_DIR=$(TGT_DIR) 

clean_release:
	$(MAKE) CPU=$(CPU) TOOL=$(TOOL) TARGET=clean_release \
		TGT_DIR=$(TGT_DIR)
