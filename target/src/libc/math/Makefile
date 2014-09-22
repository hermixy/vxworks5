# Makefile - makefile for vw/src/libc/math
#
# modification history
# --------------------
# 02d,27mar02,to   disable build for ARM again
# 02c,18dec01,to   add ARMARCH5(_T) support
# 02b,06dec01,pfl  enabled build of support.o for all archs.
# 02a,23aug01,pch  Add PPC440
# 01m,24oct01,tam  updated for repackaging
# 01l,09oct01,jb   re-enabling gnu math lib for ARMARCH4/STRONGARM/XSCALE
# 01k,05sep01,hdn  added hyperbolic routines for I80X86 (spr 25654)
# 01k,23jul01,scm  change XScale name to conform to coding standards...
# 01j,23jul01,to   removed ARMARCH4, STRONGARM and XScale in favor of fplib
#                  from ARM ltd.
# 01j,16aug01,hdn  added PENTIUM2/3/4 support
# 01i,26oct00,s_m  renamed PPC405 cpu types
# 01h,12oct00,sm   added list of objects for PPC405 and PPC405F
# 01l,08feb01,roz  use libc math functions for mips32 and 64
# 01n,04may01,scm  add STRONGARM support...
# 01m,09oct00,dh   MCF5400 support (coldfire v4)
# 01l,14jul00,dra  use pow() for vr5400
# 01k,13jan00,yvp  adding OBJS_FR500 for Fujitsu FR-V (a.k.a Venus) family
# 01j,21mar00,dra  added ColdFire support.
# 01i,07sep99,myz  added CW4000_16 support.
# 01g,24apr98,ur   added M.CORE CPU1* support.
# 01i,20dec99,dra  Updated VR4100 support.
# 01h,19jan99,dra  added CW4000, CW4011, VR4100, VR5000 and VR5400 support.
# 01h,06jun99,dra  added SPARCV9 support.
# 01j,20mar01,zl   removed obsolete SH CPU types.
# 01i,20oct00,hk   changed OBJS_SH7750 empty to use optimized math library.
# 01h,03mar00,zl   merged SH support from T1
# 01k,06feb01,roz  add pow function for all mips32, mips64
# 01j,01feb01,roz  include pow for vr5400 only
# 01i,31jan01,sru  add fabs.o to MIPS32 and MIPS64 soft float libraries
# 01h,25jan01,mem  update MIPS entries.
# 01g,18aug98,tpr  added PowerPC EC 603 support.
# 01j,30jun98,cym  added SIMNT support.
# 01i,09apr98,hdn  added Pentium support.
# 01h,26mar97,cdp  added ARM7TDMI_T support.
# 01g,02dec96,cdp  added OBJS_ARM*.
# 01f,16dec96,kkk  added R4650 support.
# 01e,07dec96,tpr  reworked PPC403 and PPC860 rules.
# 01d,07dec96,tpr  added supporty.o to I960KB rule.
# 01c,03dec96,tpr  reworked I960 rules.
# 01b,16oct96,tam  modified OBJS_PPC603 and OBJS_PPC604.
# 01a,18jun96,yp   created from 02c of MakeSkel
#
# DESCRIPTION
# This file contains the makefile rules for building the portable
# ANSI C math routines.
#
# Each cpu architecture requires a specific list of these functions.
# In many cases, some subset of the ANSI functions are defined in the
# <arch>/math directories, so not all files in this directory are
# required.  Files from this directory should be specified to provide
# all functions which are not supplied in an architecture-specific form.
#
# Note: modf() is required by portions of the ANSI C I/O libraries
# and should therefore be included for all architectures.
#
#*/

TGT_DIR=$(WIND_BASE)/target

DOC_FILES=	ansiMath.c

## ansiMath.c includes the following files
#	asincos.c atan.c atan2.c ceil.c 
#	cosh.c exp.c fabs.c floor.c fmod.c frexp.c ldexp.c log.c 
#	log10.c modf.c pow.c sincos.c sinh.c sqrt.c tan.c tanh.c

LIB_BASE_NAME   = os

OBJS_MIN=	frexp.o ldexp.o modf.o
OBJS_CPU32      = $(OBJS_MIN)
OBJS_I960CA     = ceil.o floor.o
OBJS_I960HX     = $(OBJS_I960CA)
OBJS_I960JX	= $(OBJS_I960CA)
OBJS_I960KA     = $(OBJS_I960CA)
OBJS_I960KB     = support.o
OBJS_MC68000    = $(OBJS_MIN)
OBJS_MC68010    = $(OBJS_MIN)
OBJS_MC68020    = $(OBJS_MIN) support.o
OBJS_MC68040    = $(OBJS_MIN) fmod.o pow.o support.o log__L.o exp__E.o
OBJS_MC68LC040  = $(OBJS_MIN) fmod.o pow.o support.o log__L.o exp__E.o
OBJS_MC68060    = $(OBJS_MIN) fmod.o pow.o support.o log__L.o exp__E.o
OBJS_MCF5200	= $(OBJS_MIN) acosh.o asincos.o asinh.o atan.o atan2.o \
		  atanh.o ceil.o cosh.o exp.o exp__E.o expm1.o floor.o \
		  fmod.o log.o log10.o log1p.o log__L.o pow.o sincos.o \
		  sinh.o sqrt.o support.o tan.o tanh.o fabs.o
OBJS_MCF5400	= $(OBJS_MIN) acosh.o asincos.o asinh.o atan.o atan2.o \
		  atanh.o ceil.o cosh.o exp.o exp__E.o expm1.o floor.o \
		  fmod.o log.o log10.o log1p.o log__L.o pow.o sincos.o \
		  sinh.o sqrt.o support.o tan.o tanh.o fabs.o
OBJS_MIPS32	= $(OBJS_MIN) acosh.o asincos.o asinh.o atan.o atan2.o \
		  atanh.o ceil.o cosh.o exp.o exp__E.o expm1.o fabs.o floor.o \
		  fmod.o log.o log10.o log1p.o log__L.o pow.o sincos.o \
		  sinh.o sqrt.o support.o tan.o tanh.o
OBJS_MIPS64	= $(OBJS_MIN) acosh.o asincos.o asinh.o atan.o atan2.o \
		  atanh.o ceil.o cosh.o exp.o exp__E.o expm1.o fabs.o floor.o \
		  fmod.o log.o log10.o log1p.o log__L.o pow.o sincos.o \
		  sinh.o sqrt.o support.o tan.o tanh.o
OBJS_SPARC      = $(OBJS_MIN) acosh.o asincos.o asinh.o atan.o atan2.o \
		  atanh.o ceil.o cosh.o exp.o exp__E.o expm1.o floor.o \
		  fmod.o log.o log10.o log1p.o log__L.o pow.o sincos.o \
		  sinh.o sqrt.o support.o tan.o tanh.o
OBJS_SPARClite  = acosh.o asinh.o atanh.o exp__E.o expm1.o log1p.o log__L.o \
                  support.o
OBJS_ULTRASPARC = $(OBJS_MIN) acosh.o asincos.o asinh.o atan.o atan2.o \
		  atanh.o ceil.o cosh.o exp.o exp__E.o expm1.o floor.o \
		  fmod.o log.o log10.o log1p.o log__L.o pow.o sincos.o \
		  sinh.o sqrt.o support.o tan.o tanh.o
OBJS_SIMSPARCSOLARIS    = $(OBJS_MIN) acosh.o asincos.o asinh.o atan.o atan2.o \
                  atanh.o ceil.o cosh.o exp.o exp__E.o expm1.o fabs.o floor.o \
                  fmod.o log.o log10.o log1p.o log__L.o pow.o sincos.o \
                  sinh.o sqrt.o support.o tan.o tanh.o
OBJS_SIMSPARCSUNOS    = $(OBJS_MIN) acosh.o asincos.o asinh.o atan.o atan2.o \
		  atanh.o ceil.o cosh.o exp.o exp__E.o expm1.o fabs.o floor.o \
		  fmod.o log.o log10.o log1p.o log__L.o pow.o sincos.o \
		  sinh.o sqrt.o support.o tan.o tanh.o
OBJS_SIMHPPA    = $(OBJS_MIN) acosh.o asincos.o asinh.o atan.o atan2.o \
		  atanh.o ceil.o cosh.o exp.o exp__E.o expm1.o fabs.o floor.o \
		  fmod.o log.o log10.o log1p.o log__L.o pow.o sincos.o \
		  sinh.o sqrt.o support.o tan.o tanh.o
OBJS_SIMNT	= $(OBJS_MIN) acosh.o asincos.o asinh.o atan.o atan2.o \
		  atanh.o ceil.o cosh.o exp.o exp__E.o expm1.o fabs.o floor.o \
		  fmod.o log.o log10.o log1p.o log__L.o pow.o sincos.o \
		  sinh.o sqrt.o support.o tan.o tanh.o
OBJS_I80386     = $(OBJS_MIN) cosh.o sinh.o tanh.o support.o expm1.o \
		  exp__E.o exp.o
OBJS_I80486     = $(OBJS_MIN) cosh.o sinh.o tanh.o support.o expm1.o \
		  exp__E.o exp.o
OBJS_PENTIUM    = $(OBJS_MIN) cosh.o sinh.o tanh.o support.o expm1.o \
		  exp__E.o exp.o
OBJS_PENTIUM2   = $(OBJS_MIN) cosh.o sinh.o tanh.o support.o expm1.o \
		  exp__E.o exp.o
OBJS_PENTIUM3   = $(OBJS_MIN) cosh.o sinh.o tanh.o support.o expm1.o \
		  exp__E.o exp.o
OBJS_PENTIUM4   = $(OBJS_MIN) cosh.o sinh.o tanh.o support.o expm1.o \
		  exp__E.o exp.o
OBJS_AM29030    = acosh.o asinh.o exp__E.o expm1.o log1p.o log__L.o support.o
OBJS_AM29200    = $(OBJS_AM29030)

OBJS_PPC403     = support.o
OBJS_PPC405     = $(OBJS_PPC403) 
OBJS_PPC405F   = $(OBJS_MIN) acosh.o asincos.o asinh.o atanh.o ceil.o cosh.o \
		  exp__E.o expm1.o floor.o fmod.o log1p.o log__L.o sinh.o \
		  support.o tan.o tanh.o fabs.o sqrt.o atan2.o exp.o atan.o \
		  sincos.o log.o log10.o pow.o
OBJS_PPC440     = $(OBJS_PPC403)
OBJS_PPC505     = $(OBJS_PPC403)
OBJS_PPC509     = $(OBJS_PPC603)
OBJS_PPC555     = $(OBJS_PPC603)
OBJS_PPC601     = $(OBJS_PPC403)
OBJS_PPC603     = $(OBJS_MIN) acosh.o asincos.o asinh.o atanh.o ceil.o cosh.o \
		  exp__E.o expm1.o floor.o fmod.o log1p.o log__L.o sinh.o \
		  support.o tan.o tanh.o fabs.o
OBJS_PPCEC603	= $(OBJS_PPC403)
OBJS_PPC604     = $(OBJS_PPC603)
OBJS_PPC860     = $(OBJS_PPC403)

OBJS_MCORE10	= acosh.o asincos.o asinh.o atan.o atan2.o \
		  atanh.o ceil.o cosh.o exp.o exp__E.o expm1.o fabs.o \
		  floor.o fmod.o frexp.o ldexp.o log.o log10.o log1p.o \
		  log__L.o modf.o pow.o sincos.o sinh.o sqrt.o support.o \
		  tan.o tanh.o
OBJS_MCORE15	= $(OBJS_MCORE10)

OBJS_ARM	= $(OBJS_MIN) acosh.o asincos.o asinh.o atan.o atan2.o \
		  atanh.o ceil.o cosh.o exp.o exp__E.o expm1.o floor.o \
		  fmod.o log.o log10.o log1p.o log__L.o pow.o sincos.o \
		  sinh.o sqrt.o support.o tan.o tanh.o fabs.o
OBJS_ARMARCH4	=
OBJS_ARMARCH4_T	= $(OBJS_ARM)
OBJS_ARMARCH5	=
OBJS_ARMARCH5_T	= $(OBJS_ARM)
OBJS_STRONGARM	=
OBJS_XSCALE	=

OBJS_FRV	= acosh.o asincos.o asinh.o atan.o atan2.o atanh.o ceil.o \
		  cosh.o exp.o exp__E.o expm1.o floor.o fabs.o floor.o    \
		  fmod.o frexp.o ldexp.o log.o log10.o log1p.o log__L.o   \
		  modf.o pow.o sincos.o sinh.o sqrt.o support.o tan.o tanh.o
OBJS_FR500	= $(OBJS_FRV)

OBJS_SH7000     = support.o
OBJS_SH7600     = $(OBJS_SH7000) 
OBJS_SH7700     = $(OBJS_SH7000) 
OBJS_SH7750     = $(OBJS_SH7000) 

OBJS=   $(OBJS_$(CPU))


include $(TGT_DIR)/h/make/rules.library

