# Makefile - makefile for vw/src/arch/ppc/math/motorolaLib
#
# modification history
# --------------------
# 01b,12oct01,tam  added support for new packaging scheme
# 01a,07dec96,tpr  written.
#
# DESCRIPTION
# This file contains the makefile rules for building the vxWorks architecture
# dependant library.
#
# NOTE
# The binaries without sources have to be added to the src-arch ressource file
#*/

TGT_DIR=$(WIND_BASE)/target

LIB_BASE_NAME	= arch

OBJ_PPC403 	= 
OBJ_PPC550 	= 
OBJ_PPC555 	= $(OBJ_PPC603) 
OBJ_PPC509 	= $(OBJ_PPC603) 
OBJ_PPC601 	=
OBJ_PPC603 	= $(LIBDIR)/atan.o $(LIBDIR)/pow.o $(LIBDIR)/sqrt.o \
		  $(LIBDIR)/trig.o
OBJ_PPC604 	= $(OBJ_PPC603)
OBJ_PPC860   	=

OBJS            = $(OBJ_$(CPU))

include $(TGT_DIR)/h/make/rules.library


$(TGT_DIR)/lib/$(LIBNAME) : $(OBJS)

$(LIBDIR)/atan.o:
	$(RM) $(subst /,$(DIRCHAR),$@)
	$(CP) atan.o $(subst /,$(DIRCHAR),$@)

$(LIBDIR)/pow.o:
	$(RM) $(subst /,$(DIRCHAR),$@)
	$(CP) pow.o $(subst /,$(DIRCHAR),$@)

$(LIBDIR)/sqrt.o:
	$(RM) $(subst /,$(DIRCHAR),$@)
	$(CP) sqrt.o $(subst /,$(DIRCHAR),$@)

$(LIBDIR)/trig.o:
	$(RM) $(subst /,$(DIRCHAR),$@)
	$(CP) trig.o $(subst /,$(DIRCHAR),$@)
