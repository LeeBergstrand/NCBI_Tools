# ===========================================================================
#
#                            PUBLIC DOMAIN NOTICE
#               National Center for Biotechnology Information
#
#  This software/database is a "United States Government Work" under the
#  terms of the United States Copyright Act.  It was written as part of
#  the author's official duties as a United States Government employee and
#  thus cannot be copyrighted.  This software/database is freely available
#  to the public for use. The National Library of Medicine and the U.S.
#  Government have not placed any restriction on its use or reproduction.
#
#  Although all reasonable efforts have been taken to ensure the accuracy
#  and reliability of the software and data, the NLM and the U.S.
#  Government do not and cannot warrant the performance or results that
#  may be obtained by using this software or data. The NLM and the U.S.
#  Government disclaim all warranties, express or implied, including
#  warranties of performance, merchantability or fitness for any particular
#  purpose.
#
#  Please cite the author in any work or product based on this material.
#
# ===========================================================================


default: std

TOP ?= $(abspath ../..)
MODULE = test/dir_table

include $(TOP)/build/Makefile.env

INT_TOOLS = \

EXT_TOOLS = \
	dir_table

ALL_TOOLS = \
	$(INT_TOOLS) \
	$(EXT_TOOLS)

#-------------------------------------------------------------------------------
# outer targets
#
all std: makedirs
	@ $(MAKE_CMD) $(TARGDIR)/$@

$(ALL_TOOLS): makedirs
	@ $(MAKE_CMD) $(BINDIR)/$@

.PHONY: all std $(ALL_TOOLS)

#-------------------------------------------------------------------------------
# all
#
$(TARGDIR)/all: \
	$(addprefix $(BINDIR)/,$(ALL_TOOLS))

.PHONY: $(TARGDIR)/all

#-------------------------------------------------------------------------------
# std
#
$(TARGDIR)/std: \
	$(addprefix $(BINDIR)/,$(EXT_TOOLS))

.PHONY: $(TARGDIR)/std

#-------------------------------------------------------------------------------
# clean
#
clean: stdclean

.PHONY: clean

#-------------------------------------------------------------------------------
# tag
#
tag: $(addsuffix _tag,$(ALL_TOOLS))

.PHONY: tag $(addsuffix _tag,$(ALL_TOOLS))

#-------------------------------------------------------------------------------
# dir_table
#  what is this?
#
DIR_TABLE_SRC = \
	dir_table

DIR_TABLE_OBJ = \
	$(addsuffix .$(OBJX),$(DIR_TABLE_SRC))

DIR_TABLE_LIB = \
	-ssrapath \
	-swsraschema \
	$(UPDATE_SCHEMA_LIBS) \
	-lwvdb \
	-lwkdb \
	-lkapp \
	-lvfs \
	-lkfg \
	-lkfs \
	-lksrch \
	-lklib \
	-ldl \
	-lz \
	-lbz2 \
	-lm

$(BINDIR)/dir_table: $(DIR_TABLE_OBJ)
	$(LD) --exe -o $@ $^ $(DIR_TABLE_LIB)

dir_table_tag:
	@ $(TOP)/build/tag-module.sh $(MODULE) dir_table $(DIR_TABLE_OBJ)
