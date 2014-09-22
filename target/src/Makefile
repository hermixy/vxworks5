# Makefile - top level makefile for target/src to build runtime libraries
#
# modification history
# --------------------
# 02h,31jul02,tpr  removed security and test frim the build for SRC CD.
# 02g,26jun02,pes  Remove MIPS32 builds from bcm125x
#                  CPU_VARIANT_CPU_TOOL_LIST_bcm125x.
# 02f,08may02,pes  Change MIPS r3xxx CPU_VARIANT designation to rc3000
# 02e,23apr02,pes  Add support for rc4xxx, rc5xxx, rm52xx, rm57xx, tx49xx and
#		   vr50xx processor families.
# 02d,14mar02,t_m  move the rule for man after the include so the default rule
#                  is still Default
# 02c,07mar02,pch  Add arch/Makefile dependency for "man" target to fix cpuGen
#                  fallout - SPR 73999
# 02b,27feb02,pes  Adjust per-cpu-family Library archive selections
# 02a,11jan02,pes  Insert macro definitions for MIPS repackaged libarch.a
#                  libraries
# download from www.programsalon.com
# 01z,12dec01,tam  made CPU variant builds automatic; updated comments
# 01y,12dec01,mem  Fix specialization test for MIPS.
# 01x,07dec01,tam  Added CPU variant support
# 01w,27nov01,jmp  Changed to expect simpc instead of SIMNT for VX_CPU_FAMILY.
# 01v,20nov01,tam  move demo to dual build
# 01u,16nov01,mil  Changed to expect ppc instead of PPC for VX_CPU_FAMILY.
# 01t,12nov01,tam  added dual build support
# 01s,07nov01,tam  added vxvmi directory
# 01r,31oct01,mas  added vxmp subdirectory
# 01q,30oct01,sn   pushed gnu under new tool directory
# 01p,29oct01,dbs  re-instate vxcom and vxdcom
# 01o,18oct01,dbs  temporarily remove vxcom and vxdcom from build
# 01n,11oct01,tpr  Removed VxWorks 5.5 unsupported directories
# 01m,04oct01,nel  Add VxDCOM to build.
# 01l,27apr01,mem  Added vxFusion, zinc, missing dirs, remove ospf, wweb merge
# 01k,07dec00,af   add cplus and gnu back to default build list.
# 01j,06nov00,sn   add target for just building C++
# 01i,09oct00,sn   but do build everything on tor2_x-fred-toolchain
# 01h,09oct00,sn   don't build C++
# 01g,11dec98,j_w  define CLEANLIBS -- for removing lib$(CPU)$(TOOL)*.a
# 01f,22may98,ms   removed SUBDIRS hack
# 01e,29apr98,dbt  removed rdb directory
# 01d,05nov97,kkk  added rip, dhcp directory
# 01c,28oct97,kkk  added ospf directory
# 01b,15nov96,jag  added snmpv1 directory
# 01a,17jun96,yp   written
#
# DESCRIPTION
# This file contains the makefile rules to select subdirectories that contain
# sources for building the VxWorks runtime archive and builds them.
#
# Some architecures support only one toolchain family (either GNU or DIAB). 
# The toolchain supported is defined by VENDOR_DEFAULT below. 
# For instance to build the source tree for the solaris simulator which supports
# only the GNU toolchain, you should issue the following commands:
#		> cd $WIND_BASE/target/src
#		> make CPU=SIMSPARCSOLARIS TOOL=gnu
#
# Other architecures support both toolchain families (GNU and DIAB).
# However most of the VxWorks source code under $(WIND_BASE)/target/src/
# is compiled with only one toolchain family (either GNU or DIAB). This
# default toolchain is defined again by VENDOR_DEFAULT. Nevertheless part of
# the VxWorks source tree must still be built with the secondary toolchain
# family. This dual build is accomplished in two passes.
#
# For instance to build the source tree for CPU=PPC604 which supports both
# GNU and DIAB, the following commands must be issued:
#		> cd $WIND_BASE/target/src
#		> make CPU=PPC604 TOOL=diab
#		> make CPU=PPC604 TOOL=gnu
#
#
# CPU VARIANTS
# To compile part of the source tree for a CPU variant, first a tag must be
# defined with which archive directory names are modified. In addition several
# variables must be defined for each CPU variant build. Refer to the example
# further below for more details. Note that CPU variant builds are normaly
# started automatically at the end of the corresponding generic CPU build.
# 
#
# FOR TESTING PURPOSES
# If a dual build of the source tree should not be performed for a CPU type
# which by default supports both toolchain families, the value of DUAL_BUILD
# must be overwritten on the command line.
# For instance to build successfuly the entire source tree for CPU=PPC604 with
# the DIAB toolchain despite it is not the default toolchain for this 
# architecture, issue the following commands:
#		> cd $WIND_BASE/target/src
#		> make CPU=PPC604 TOOL=diab DUAL_BUILD=false
#
# You may need to build the entire source tree with both toolchain families
# (GNU and DIAB). Every source file must be compiled with both toolchains
# and the resulting objects stored in a different locations. As above 
# DUAL_BUILD must be set to false and in addition the COMMON_DIR variable
# must be overwritten and set to false:
#		> cd $WIND_BASE/target/src
#		> make CPU=PPC604 TOOL=diab DUAL_BUILD=false COMMON_DIR=false
#		> make CPU=PPC604 TOOL=gnu DUAL_BUILD=false COMMON_DIR=false
#
# INTERNAL
# The REAME.txt file under $(WIND_BASE)/target/src/ should be updated whenever
# this file is modified, if appropriate.
#*/

TGT_DIR=$(WIND_BASE)/target

# remove lib$(CPU)$(TOOL)*.a if make rclean is done at target/src level
CLEANLIBS = YES

# IMPORTANT:
#
# New directories under $WIND_BASE/src must be added to one of the following
# list, and only one:
#
#		SKIP_SUBDIRS:	directory(ies) not being compiled
#		COMMON_SUBDIRS:	directories compiled with the default compiler
#				only (cf value of VENDOR_DEFAULT)
#		DUAL_SUBDIRS:	directories compiled with each toolchain.
#				NOTE: any directory including C++ files must be
#				added to this list.
#		GNU_SUBDIRS:	directories compiled with GNU only
#		DIAB_SUBDIRS:	directories compiled with DIAB only

SKIP_SUBDIRS	= CodeTEST http java snmpv1 streams ugl zinc zlib

COMMON_SUBDIRS	= arch bpf dhcp drv event fs libc math	\
        netinet netwrs os ostool posix rip rpc \
        tffs usb usr util vxfusion vxmp vxvmi wdb wind wrn 	\
	wtx wv

DUAL_SUBDIRS	= cplus demo tool vxcom vxdcom
					# - other C++ based code to be added 
					# here
					# - other source directories not ported
					# to new archive directory hierarchy to
					# be added here too.

GNU_SUBDIRS	=

DIAB_SUBDIRS	=


include $(TGT_DIR)/h/make/defs.default
include $(TGT_DIR)/h/tool/$(TOOL_FAMILY)/make.$(CPU)$(TOOL)


## Architectures default toolchain and dual build status

#
# ARM: DIAB & GNU (DIAB as default)
#

ifeq ($(VX_CPU_FAMILY),arm)
    VENDOR_DEFAULT	= diab
    DUAL_BUILD		= true
endif

#
# COLDFIRE: supports only DIAB
#

ifeq ($(VX_CPU_FAMILY),cf)
    VENDOR_DEFAULT	= diab
    DUAL_BUILD		= false
endif

#
# MC68K: supports only GNU
#

ifeq ($(VX_CPU_FAMILY),68k)
    VENDOR_DEFAULT	= gnu
    DUAL_BUILD		= false
endif

#
# MIPS: DIAB & GNU (DIAB as default)
#

ifeq ($(VX_CPU_FAMILY),mips)
    VENDOR_DEFAULT	= diab
    DUAL_BUILD		= true
endif

#
# PENTIUM: supports only GNU
#

ifeq ($(VX_CPU_FAMILY),pentium)
    VENDOR_DEFAULT	= gnu
    DUAL_BUILD		= false
endif

#
# PPC: GNU & DIAB (GNU as default)
#

ifeq ($(VX_CPU_FAMILY),ppc)
    VENDOR_DEFAULT	= gnu
    DUAL_BUILD		= true
endif

#
# SH (Hitachi): GNU & DIAB (GNU as default)
#

ifeq ($(VX_CPU_FAMILY),sh)
    VENDOR_DEFAULT	= gnu
    DUAL_BUILD		= true
endif

#
# SIMNT: supports only GNU
#

ifeq ($(VX_CPU_FAMILY),simpc)
    VENDOR_DEFAULT	= gnu
    DUAL_BUILD		= false
endif

#
# SIMSOLARIS: supports only GNU
#

ifeq ($(VX_CPU_FAMILY),simso)
    VENDOR_DEFAULT	= gnu
    DUAL_BUILD		= false
endif

# defaults for VENDOR_DEFAULT and DUAL_BUILD

ifeq ($(DUAL_BUILD),)
    DUAL_BUILD		= false
endif

ifeq ($(VENDOR_DEFAULT),)
    VENDOR_DEFAULT	= gnu
endif

# Select the toolchain specific directories

ifeq ($(TOOL), $(TOOL_GNU))
    TOOL_DIRS	= $(GNU_SUBDIRS)
endif
ifeq ($(TOOL), $(TOOL_DIAB))
    TOOL_DIRS	= $(DIAB_SUBDIRS)
endif

## CPU variants builds
#
# For CPU variants builds, the name of the directory(ies) where archives are 
# installed is appended with a unique tag defined by LIB_DIR_TAG. The make
# variable CPU_VARIANT_LIST define the whole list of these CPU variant tags
# and should be updated everytime a new CPU variant build is introduced. For
# instance:
#	CPU_VARIANT_LIST = _cpuVariant1 _cpuVariant2 _cpuVariant3

CPU_VARIANT_LIST	= 	_au1xxx \
				_bcm125x \
				_mti4kx \
				_mti5kx \
				_mti20kx \
				_rc32xxx \
				_rc4xxx \
				_rc5xxx \
				_rm52xx \
				_rm57xx \
				_rm7xxx \
				_tx49xx \
				_vr50xx \
				_vr41xx \
				_vr54xx \
				_vr55xx \
				_rc3000 \

# Each CPU variant build must define the following variables:
#
#	- VX_ARCHIVE_LIST$(LIB_DIR_TAG): this is the list of archive(s) to 
#	  build for this CPU variant. Note that $(LIB_DIR_TAG) must be equal
#	  to one of the item defined in CPU_VARIANT_LIST. For instance:
#	  	VX_ARCHIVE_LIST_cpuVariant1 = libarch.a libdrv.a
#
#	- CPU_VARIANT_CPU_TOOL_LIST$(LIB_DIR_TAG): this is the list of
#	  CPU/TOOL combination(s) for which the CPU variant build must be
#	  run. For instance:
#		CPU_VARIANT_CPU_TOOL_LIST_cpuVariant2 = MIPS32diab MIPS64diab
#
# NOTE: CPU variant build(s) are kicked off by the default build rule
# in target/h/make/rules.library.

VX_ARCHIVE_LIST_au1xxx	= libarch.a
VX_ARCHIVE_LIST_bcm125x	= libarch.a
VX_ARCHIVE_LIST_mti4kx	= libarch.a
VX_ARCHIVE_LIST_mti5kx	= libarch.a
VX_ARCHIVE_LIST_mti20kx	= libarch.a
VX_ARCHIVE_LIST_rc32xxx	= libarch.a
VX_ARCHIVE_LIST_rc4xxx	= libarch.a
VX_ARCHIVE_LIST_rc5xxx	= libarch.a
VX_ARCHIVE_LIST_rm52xx	= libarch.a
VX_ARCHIVE_LIST_rm57xx	= libarch.a
VX_ARCHIVE_LIST_rm7xxx	= libarch.a
VX_ARCHIVE_LIST_tx49xx	= libarch.a
VX_ARCHIVE_LIST_vr41xx	= libarch.a
VX_ARCHIVE_LIST_vr50xx	= libarch.a
VX_ARCHIVE_LIST_vr54xx	= libarch.a
VX_ARCHIVE_LIST_vr55xx	= libarch.a
VX_ARCHIVE_LIST_rc3000	= libarch.a

CPU_VARIANT_CPU_TOOL_LIST_au1xxx	= MIPS32sfdiab MIPS32sfdiable
CPU_VARIANT_CPU_TOOL_LIST_bcm125x	= MIPS64diab MIPS64diable
CPU_VARIANT_CPU_TOOL_LIST_mti4kx	= MIPS32sfdiab MIPS32sfdiable
CPU_VARIANT_CPU_TOOL_LIST_mti5kx	= MIPS32sfdiab MIPS32sfdiable
CPU_VARIANT_CPU_TOOL_LIST_mti20kx	= MIPS32sfdiab MIPS32sfdiable \
					  MIPS64diab MIPS64diable
CPU_VARIANT_CPU_TOOL_LIST_rc32xxx	= MIPS32sfdiab MIPS32sfdiable
CPU_VARIANT_CPU_TOOL_LIST_rc4xxx	= MIPS32sfdiab MIPS32sfdiable \
					  MIPS64diab MIPS64diable
CPU_VARIANT_CPU_TOOL_LIST_rc5xxx	= MIPS32sfdiab MIPS32sfdiable \
					  MIPS64diab MIPS64diable
CPU_VARIANT_CPU_TOOL_LIST_rm52xx	= MIPS32sfdiab MIPS32sfdiable \
					  MIPS64diab MIPS64diable
CPU_VARIANT_CPU_TOOL_LIST_rm57xx	= MIPS32sfdiab MIPS32sfdiable \
					  MIPS64diab MIPS64diable
CPU_VARIANT_CPU_TOOL_LIST_rm7xxx	= MIPS32sfdiab MIPS32sfdiable \
					  MIPS64diab MIPS64diable
CPU_VARIANT_CPU_TOOL_LIST_tx49xx	= MIPS32sfdiab MIPS32sfdiable \
					  MIPS64diab MIPS64diable
CPU_VARIANT_CPU_TOOL_LIST_vr41xx	= MIPS32sfdiab MIPS32sfdiable
CPU_VARIANT_CPU_TOOL_LIST_vr50xx	= MIPS32sfdiab MIPS32sfdiable \
					  MIPS64diab MIPS64diable
CPU_VARIANT_CPU_TOOL_LIST_vr54xx	= MIPS32sfdiab MIPS32sfdiable \
					  MIPS64diab MIPS64diable
CPU_VARIANT_CPU_TOOL_LIST_vr55xx	= MIPS32sfdiab MIPS32sfdiable \
					  MIPS64diab MIPS64diable
CPU_VARIANT_CPU_TOOL_LIST_rc3000	= MIPS32sfr3kdiab


## Compute list of directories to build: 
#
# Set SUBDIRS according to DUAL_BUILD and VENDOR_DEFAULT

ifeq ($(DUAL_BUILD),false)
    SUBDIRS = $(COMMON_SUBDIRS) $(DUAL_SUBDIRS) $(TOOL_DIRS)
else
    ifeq ($(DUAL_BUILD),true)	# just to make sure
        ifeq ($(TOOL_VENDOR),$(VENDOR_DEFAULT))
            SUBDIRS = $(COMMON_SUBDIRS) $(DUAL_SUBDIRS) $(TOOL_DIRS)
        else
            SUBDIRS = $(DUAL_SUBDIRS) $(TOOL_DIRS)
        endif
    endif
endif

include $(TGT_DIR)/h/make/rules.library

# If doing a doc build, ensure arch/Makefile is built.
# The rules are in $(TGT_DIR)/h/make/rules-lib.unix

man:	$(TGT_DIR)/src/arch/Makefile

