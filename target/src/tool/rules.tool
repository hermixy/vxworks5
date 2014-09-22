# rules.tool - rules for extracting objects from toolchain libraries
#
# modification history
# --------------------
# 01m,03dec01,sn   allow configlette rules to be used seperately from TOOL_LIB 
#                  extraction rules
# 01l,13nov01,sn   include defs.exclude
# 01k,07nov01,tam  removed LIBDIR & lib rule
# 01j,06nov01,sn   prefix object file names to prevent clashes with VxWorks objects
# 01i,15oct01,sn   moved GNU specific material to tool/defs.gnu
# 01h,24jan01,sn   append a "marker symbol" to each module
# 01h,18jan01,mem  Strip debug info from MIPS objects to work around VxGDB
#                  issue.
# 01g,15nov99,sn   wrote
#
# DESCRIPTION
# Generic rules for extracting objects from toolchain libraries.
# Subdirectory Makefiles for specific toolchains should
# set TOOL_LIBS and include this file.
#

include $(TGT_DIR)/h/make/rules.library

GEN_CONFIG 	=  wtxtcl $(TGT_DIR)/src/tool/genConfig.tcl
OBJS_EXCLUDE    =  wtxtcl $(TGT_DIR)/src/tool/objsExclude.tcl

include $(TGT_DIR)/src/tool/$(TOOL_FAMILY)/defs.exclude

# Compute PRE_OBJS (all objects except $(CONFIGLETTE_O))
ifneq ($(TOOL_LIB),)
ifeq ($(PRE_OBJS),)
ifeq ($(REAL_OBJS),)
REAL_OBJS	:= $(shell $(AR) t $(TOOL_LIB) | $(OBJS_EXCLUDE) $(EXCLUDE_OBJS))
endif # REAL_OBJS undefined

# We prefix each object file coming out of a toolchain library
# with OBJ_PREFIX; the hope is that this will avoid clashes
# with object files in the VxWorks library.

PRE_OBJS := $(patsubst %,$(OBJ_PREFIX)%,$(REAL_OBJS))
endif # PRE_OBJS undefined
endif # TOOL_LIB defined

ifneq ($(CONFIGLETTE_NAME),)
CONFIGLETTE_O	= __$(CONFIGLETTE_NAME).o
CONFIGLETTE_C	= $(LIBDIR)/__$(CONFIGLETTE_NAME).c
endif

OBJS		= $(PRE_OBJS) $(CONFIGLETTE_O)

# rules.library only looks in the current directory for
# source files when it determines the objects we can build.
# In our case the objects have already been built in the
# GNU build tree (in libgcc.a) and we just want to pull them out.

PRE_LIBOBJS    = $(foreach file, $(PRE_OBJS), $(LIBDIR)/$(file)) 
LIBOBJS         = $(foreach file, $(OBJS), $(LIBDIR)/$(file))

# On windows there needs to be a target called $(file)_clean for each file
# in $(LIBOBJS).
ifeq ($(WIND_HOST_TYPE),x86-win32)
clean : $(foreach file,$(subst /,$(DIRCHAR),$(LIBOBJS) $(LOCAL_CLEAN)),$(file)_clean)
$(foreach file,$(subst /,$(DIRCHAR),$(LIBOBJS) $(LOCAL_CLEAN)),$(file)_clean):
	$(RM) $(subst _clean,,$@)
endif

# Repeat the work of rules.library with the correct object list

objs : $(LIBOBJS) Makefile
$(subst /,$(DIRCHAR),$(TGT_DIR)/lib/$(LIBNAME)) : $(LIBOBJS)

ifeq ($(WIND_HOST_TYPE),x86-win32)
    SEMI=;
else
    SEMI=";"
endif

WRAPPER=$(LIBDIR)/_tmp_wrapper.c
TMP_OBJ=$(LIBDIR)/_tmp_obj.o

ifneq ($(TOOL_LIB),)
$(LIBDIR)/%.o : $(TOOL_LIB) Makefile
	@echo Wrapping $@ ...
	$(RM) $(subst /,$(DIRCHAR),$@)
# This just adds a symbol __object_o to object.o. We chose
# to use a one-line C fragment rather than using the linker
# to directly add a symbol because this way we don't have
# to worry about whether or not the compiler prepends an underscore.
	echo char __$(subst -,_,$(notdir $*))_o = 0$(SEMI) > $(WRAPPER)
ifeq ($(WIND_HOST_TYPE),x86-win32)
	(cd $(LIBDIR) & $(AR) -x $(TOOL_LIB) $(patsubst $(OBJ_PREFIX)%,%,$(notdir $@)) & move $(patsubst $(OBJ_PREFIX)%,%,$(notdir $@)) $(notdir $@))
else
	(cd $(LIBDIR) ; \
         $(AR) -x $(TOOL_LIB) $(patsubst $(OBJ_PREFIX)%,%,$(notdir $@)) ; \
         mv $(patsubst $(OBJ_PREFIX)%,%,$(notdir $@)) $@)
endif
	$(CC) -c $(CFLAGS) $(WRAPPER) -o $(TMP_OBJ)
	$(LD) $(LD_PARTIAL_FLAGS) -r $(TMP_OBJ) $@ -o $@_tmp
	$(CP) $(subst /,$(DIRCHAR),$@_tmp) $(subst /,$(DIRCHAR),$@)
	$(RM) $(WRAPPER) $(TMP_OBJ) $@_tmp

endif # $(TOOL_LIB) defined

ifneq ($(CONFIGLETTE_O),)
$(LIBDIR)/$(CONFIGLETTE_O) : $(PRE_LIBOBJS)
	$(GEN_CONFIG) $(CONFIGLETTE_NAME) $(PRE_OBJS) > $(CONFIGLETTE_C)
	$(CC) -c $(CFLAGS) $(CONFIGLETTE_C) -o $@
	$(RM) $(CONFIGLETTE_C)
endif

