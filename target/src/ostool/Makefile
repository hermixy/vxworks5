# Makefile - makefile for vw/src/ostool
#
# modification history
# --------------------
# 01h,08nov01,jn   remove coff files from build
# 01g,12oct01,tam  added repackaging support
# 01f,27jun01,agf  add LONGCALL flag
# 01e,14feb01,spm  merged from version 01e of tor2_0_x branch (base 01d):
#                  added remShellLib.o for telnet upgrade (SPR #28675)
# 01d,17mar99,p_m  Added how to rebuild shell.c and shell_slex_c as comments
# 01c,28apr98,cym  added module loadPecofflib.o
# 01c,08jan98,dbt  Removed dbgLibNew.o. Added dbgTaskLib.o
# 01b,01nov96,p_m  get rid of shell.c and shell_slex_c targets since they
#		   are now checked in.
# 01a,18jun96,yp   created from 01y of MakeSkel
#
# DESCRIPTION
# This file contains the makefile rules for building the vx library
#
#*/

TGT_DIR=$(WIND_BASE)/target

LIB_BASE_NAME   = os

DOC_FILES=	ledLib.c loadLib.c loginLib.c moduleLib.c \
		remShellLib.c shellLib.c spyLib.c timexLib.c unldLib.c dbgLib.c

YACCOUT=y.tab.c

CFLAGS_repeatHost.o	= $(LONGCALL)
CFLAGS_periodHost.o	= $(LONGCALL)
CFLAGS_spyLib.o		= $(LONGCALL)
CFLAGS_ttHostLib.o	= $(LONGCALL)

OBJS=	bootAoutLib.o bootElfLib.o \
	bootLoadLib.o \
	dbgLib.o dbgTaskLib.o\
	ledLib.o \
	loadAoutLib.o \
	loadElfLib.o loadLib.o loadPecoffLib.o\
	loginLib.o moduleLib.o periodHost.o repeatHost.o \
	remShellLib.o shell.o shellLib.o spyLib.o \
	timexLib.o ttHostLib.o \
	unldLib.o 

# if shell.c gets checked in this will break. If so we'll need to comment 
# this out. Also it needs to be noted that if shell.c is not built on a
# sunos4 machine it will break
# LOCAL_CLEAN=$(NODEPENDOBJS)

include $(TGT_DIR)/h/make/rules.library

$(LIBDIR)shell.o : shell.c shell_slex_c

# Here is how one should rebuild shell.c and shell_slex_c when 
# shell.yacc or shell.slex are modified.

# shell.c : shell.yacc
#	@ $(RM) $@ $(YACCOUT)
#	yacc shell.yacc
#	sed "1s/^extern char \*malloc().*//" < $(YACCOUT) > shell.c
#	@ $(RM) $(YACCOUT)

# shell_slex_c : shell.slex
#	@ $(RM) $@
#	sh slex shell.slex > shell_slex_c

# end of vw/src/ostool/Makefile
