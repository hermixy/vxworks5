# Makefile - makefile for vw/src/math
#
# modification history
# --------------------
# 01z,18dec01,to   add ARMARCH5(_T) support
# 01y,29nov01,dee  add Coldfire libraries
# 01x,23aug01,pch  Add PPC440
# 01w,29oct01,sn   moved contents of target/src/gnu/libm here
# 01v,12oct01,tam  added repackaging support
# 01u,02oct01,sn   added cabs/hypot support for arches that lacked it
# 01t,23jul01,scm  change XScale name to conform to coding standards...
# 01s,04may01,scm  add STRONGARM support
# 01r,26oct00,s_m  renamed PPC405 cpu types
# 01q,06jul00,alp  Added PPC405 Support
# 01p,11dec00,scm  replace references to ARMSA2 with XScale
# 01o,31aug00,scm  add sa2 support...
# 01n,20jan99,cdp  removed support for old ARM libraries.
# 01m,13nov98,cdp  added generic ARMARCH3/4/4_T support.
# 01l,20mar01,zl   removed obsolete SH CPU types.
# 01k,03mar00,zl   merged SH support from T1
# 01j,28dec00,zmm  add support for MIPS32sfgnu
# 01i,27dec00,roz  add support for MIPS64gnu
# 01h,18aug98,tpr  added PowerPC EC 603 support.
# 01g,09apr98,hdn  added support for Pentium.
# 01f,07apr97,tpr  removed cabs.o cbrt.o rint.o for the I960.
# 01e,26mar97,cdp  added ARM7TDMI_T support.
# 01d,04mar97,kkk  added R4650 support
# 01c,03dec96,cdp  added OBJS_ARM*.
# 01b,26nov96,pr   added SIMSPARCSOLARIS support
# 01a,18jun96,yp   created from 02c of MakeSkel
#
# DESCRIPTION
# This file contains the makefile rules for building the non-ANSI portable
# math routines.
#
# Not all modules are used in all architectures.
#
#*/

TGT_DIR=$(WIND_BASE)/target

LIB_BASE_NAME   = os

OBJS_MIPS64		= fpType.o cabs.o
OBJS_MIPS32             = fpType.o rint.o cabs.o

OBJS_I960KA		= fpType.o cabs.o
OBJS_I960KB		= fpType.o cabs.o
OBJS_I960CA		= fpType.o cabs.o
OBJS_I960JX		= fpType.o cabs.o
OBJS_I960HX		= fpType.o cabs.o

OBJS_MC68000		= fpType.o
OBJS_MC68010		= fpType.o
OBJS_MC68020		= fpType.o
OBJS_MC68040		= fpType.o
OBJS_MC68LC040		= fpType.o
OBJS_MC68060		= fpType.o
OBJS_CPU32		= fpType.o

OBJS_R3000		= fpType.o
OBJS_R4000		= fpType.o
OBJS_R4650		= fpType.o
OBJS_R4000_64		= fpType.o

OBJS_SPARC		= fpType.o cabs.o cbrt.o rint.o
OBJS_SPARClite		= fpType.o cabs.o cbrt.o rint.o
OBJS_SIMSPARCSUNOS	= fpType.o cabs.o cbrt.o rint.o
OBJS_SIMSPARCSOLARIS    = fpType.o cabs.o cbrt.o rint.o
OBJS_SIMHPPA		= fpType.o cabs.o cbrt.o rint.o
OBJS_SIMNT		= fpType.o cabs.o cbrt.o rint.o

OBJS_G100		= fpType.o
OBJS_G200		= fpType.o

OBJS_I80386		= fpType.o
OBJS_I80486		= fpType.o
OBJS_PENTIUM		= fpType.o
OBJS_PENTIUM2		= fpType.o
OBJS_PENTIUM3		= fpType.o
OBJS_PENTIUM4		= fpType.o

OBJS_AM29030		= fpType.o
OBJS_AM29200		= fpType.o

OBJS_PPC403		= fpType.o cabs.o
OBJS_PPC405F		= fpType.o cabs.o
OBJS_PPC405		= fpType.o cabs.o
OBJS_PPC440		= fpType.o cabs.o
OBJS_PPC505		= fpType.o cabs.o
OBJS_PPC509		= fpType.o cabs.o
OBJS_PPC555		= fpType.o cabs.o
OBJS_PPC601		= fpType.o cabs.o
OBJS_PPC603		= fpType.o cabs.o
OBJS_PPCEC603		= fpType.o cabs.o
OBJS_PPC604		= fpType.o cabs.o
OBJS_PPC860		= fpType.o cabs.o

OBJS_ARMARCH4		= fpType.o cabs.o
OBJS_ARMARCH4_T		= fpType.o cabs.o
OBJS_ARMARCH5		= fpType.o cabs.o
OBJS_ARMARCH5_T		= fpType.o cabs.o
OBJS_STRONGARM		= fpType.o cabs.o
OBJS_XSCALE		= fpType.o cabs.o

 
OBJS_SH7000		= fpType.o cabs.o
OBJS_SH7600		= fpType.o cabs.o
OBJS_SH7700		= fpType.o cabs.o
OBJS_SH7750		= fpType.o cabs.o

OBJS_MCF5200		= fpType.o
OBJS_MCF5400		= fpType.o

OBJS			= $(OBJS_$(CPU)) e_fmod.o e_pow.o e_remainder.o e_sqrt.o s_rint.o s_scalbn.o

include $(TGT_DIR)/h/make/rules.library
