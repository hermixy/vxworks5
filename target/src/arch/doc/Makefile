# Makefile - makefile for vw/src/arch/doc
#
# modification history
# --------------------
# 01o,05jun02,wsl  remove SPARC libraries
# 01n,23may02,jmt  Add cache lib for tx49xx family
# 01m,22may02,zmm  Changed cacheAu1000Lib.c to cacheAuLib.c, SPR 77333.
# 01l,18mar02,mas  Removed ref to src/config/usrSmObj.c (part of SPR 73371)
# 01k,16nov01,tlc  Update cacheR4kLib.c for MIPS.
# 01j,16nov01,tlc  Update for MIPS architecture.
# 01i,29oct01,zl   added cacheSh7622Lib.c.
# 01h,18oct01,dgp  remove i960 files from doc build
# 01g,26oct99,jpd  added ../arm/mmuMapLib.c.
# 01g,03mar00,zl   merged SH support into T2
# 01f,21mar99,dgp  Fixed SPR 24881 and 21459 by adding cacheArchLib.c
#		   removed ../mc68k/cacheArchLib.c and ../arm/cacheArchLib.c
# 01e,12mar99,p_m  Fixed SPR 4711 by adding cacheR333x0Lib manuals
# 01d,21apr98,hdn  doc: add Pentium specific documentation.
# 01b,14jul97,dgp  doc: add usrAta.c
# 01b,27feb97,jpd  added ARM-specific documentation.
# 01a,18jun96,yp   created from 01r of MakeSkel
#
# DESCRIPTION
# This file contains the makefile rules for building man pages from
# special documentation-only modules that are generic to all 
# architectures.
#
#*/

TGT_DIR=$(WIND_BASE)/target

DOC_FILES=	dbgArchLib.c excArchLib.c fppArchLib.c \
		intArchLib.c taskArchLib.c vxLib.c \
		cacheArchLib.c \
		../arm/mmuMapLib.c \
		../i86/mmuPro32Lib.c \
		../i86/pentiumALib.s \
		../i86/pentiumLib.c \
		../i86/pentiumShow.c \
		../mc68k/math/mathALib.s ../mc68k/math/mathHardLib.c \
		../mc68k/math/mathSoftLib.c \
		../mips/cache4kcLib.c ../mips/cacheAuLib.c \
		../mips/cacheR10kLib.c ../mips/cacheR32kLib.c \
		../mips/cacheR33kLib.c ../mips/cacheR333x0Lib.c \
		../mips/cacheR3kLib.c ../mips/cacheR4kLib.c \
		../mips/cacheR5kLib.c ../mips/cacheR7kLib.c \
		../mips/cacheTx49Lib.c \
		../sh/cacheSh7040Lib.c ../sh/cacheSh7604Lib.c \
		../sh/cacheSh7729Lib.c ../sh/cacheSh7622Lib.c \
		../sh/cacheSh7700Lib.c ../sh/cacheSh7750Lib.c \
		../sh/mmuSh7700Lib.c ../sh/mmuSh7750Lib.c \
		$(TGT_DIR)/config/all/bootConfig.c \
		$(TGT_DIR)/config/all/bootInit.c \
		$(TGT_DIR)/config/all/usrConfig.c \
		$(TGT_DIR)/config/doc/sysLib.c \
		$(TGT_DIR)/src/config/usrAta.c \
		$(TGT_DIR)/src/config/usrFd.c \
		$(TGT_DIR)/src/config/usrIde.c \
		$(TGT_DIR)/src/config/usrScsi.c \
		$(TGT_DIR)/src/drv/netif/smNetLib.c \
		$(TGT_DIR)/src/drv/netif/smNetShow.c

include $(TGT_DIR)/h/make/rules.library

