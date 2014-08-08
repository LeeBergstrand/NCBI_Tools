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
MODULE = tools/sra-pileup

include $(TOP)/build/Makefile.env

INT_TOOLS = \

EXT_TOOLS = \
	sra-pileup \
	sam-dump
#	sam-dump2

ALL_TOOLS = \
	$(INT_TOOLS) \
	$(EXT_TOOLS)

#-------------------------------------------------------------------------------
# outer targets
#
all std: vers-includes
	@ $(MAKE_CMD) $(TARGDIR)/$@

$(ALL_TOOLS): vers-includes
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
# vers-includes
#
$(TARGDIR)/vers-includes: $(addsuffix .vers.h,$(EXT_TOOLS))

.PHONY: $(TARGDIR)/vers-includes

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
# sra-pileup
#
TOOL_SRC = \
	cmdline_cmn \
	reref \
	sra-pileup

TOOL_OBJ = \
	$(addsuffix .$(OBJX),$(TOOL_SRC))

TOOL_LIB = \
	-lkapp \
	-ssrapath \
	-lalign-reader \
	$(READONLY_SCHEMA_LIBS) \
	-lsraschema \
	-lvdb \
	-lkdb \
	-lvfs \
	-lkrypto \
	-lkfg \
	-lkfs \
	-lksrch \
	-lkproc \
	-lklib \
	-lm

$(BINDIR)/sra-pileup: $(TOOL_OBJ)
	$(LD) --exe --vers $(SRCDIR) -o $@ $^ $(TOOL_LIB)

sra-pileup_tag:
	@ $(TOP)/build/tag-module.sh $(MODULE) sra-pileup $(TOOL_OBJ)

#-------------------------------------------------------------------------------
# sam-dump2
#
SAMDUMP2_SRC = \
	cmdline_cmn \
	writer \
	sam-dump

SAMDUMP2_OBJ = \
	$(addsuffix .$(OBJX),$(SAMDUMP2_SRC))

SAMDUMP2_LIB = \
	-lkapp \
	-ssrapath \
	-lalign-reader \
	$(READONLY_SCHEMA_LIBS) \
	-lsraschema \
	-lvdb \
	-lkdb \
	-lvfs \
	-lkrypto \
	-lkfg \
	-lkfs \
	-lksrch \
	-lkproc \
	-lklib \
	-lm

ifdef NCBI
CFLAGS += -DNCBI
endif

$(BINDIR)/sam-dump2: $(SAMDUMP2_OBJ)
	$(LD) --exe --vers $(SRCDIR) -o $@ $^ $(SAMDUMP2_LIB)

samdump2_tag:
	@ $(TOP)/build/tag-module.sh $(MODULE) samdump2 $(SAMDUMP2_OBJ)


#-------------------------------------------------------------------------------
# sam-dump
#
SAMDUMP3_SRC = \
	inputfiles \
	sam-dump-opts \
	out_redir \
	sam-hdr \
	matecache \
	read_fkt \
	sam-aligned \
	sam-unaligned \
	cg_tools \
	sam-dump \
	sam-dump3

SAMDUMP3_OBJ = \
	$(addsuffix .$(OBJX),$(SAMDUMP3_SRC))

SAMDUMP3_LIB = \
	-lkapp \
	-ssrapath \
	-lalign-reader \
	$(READONLY_SCHEMA_LIBS) \
	-lsraschema \
	-lvdb \
	-lkdb \
	-lvfs \
	-lkrypto \
	-lkfg \
	-lkfs \
	-lksrch \
	-lkproc \
	-lklib \
	-lm

ifdef NCBI
CFLAGS += -DNCBI
endif

$(BINDIR)/sam-dump: $(SAMDUMP3_OBJ)
	$(LD) --exe --vers $(SRCDIR) -o $@ $^ $(SAMDUMP3_LIB)

sam-dump_tag:
	@ $(TOP)/build/tag-module.sh $(MODULE) sam-dump $(SAMDUMP3_OBJ)
