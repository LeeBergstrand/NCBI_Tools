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
MODULE = test/kfg

include $(TOP)/build/Makefile.env

TEST_TOOLS = \
	test-kfg

EXT_TOOLS =

ALL_TOOLS = \
	$(TEST_TOOLS) \
	$(EXT_TOOLS)

RWORKDIR = $(TOP)/$(MODULE)

#-------------------------------------------------------------------------------
# outer targets
#
all std: makedirs
	@ $(MAKE_CMD) $(TARGDIR)/std

$(ALL_TOOLS): makedirs
	@ $(MAKE_CMD) $(BINDIR)/$@

.PHONY: all std $(ALL_TOOLS)

#-------------------------------------------------------------------------------
# std
#
$(TARGDIR)/std: \
	$(addprefix $(TEST_BINDIR)/,$(ALL_TOOLS)) 

.PHONY: $(TARGDIR)/std

#-------------------------------------------------------------------------------
# clean
#
clean: stdclean

.PHONY: clean

#-------------------------------------------------------------------------------
# tag
#
tag: \
	$(addsuffix _tag,$(ALL_TOOLS))

.PHONY: tag $(addsuffix _tag,$(ALL_TOOLS))

#-------------------------------------------------------------------------------
# test-kfg
#
TEST_KFG_SRC = \
	kfgtest 

TEST_KFG_OBJ = \
	$(addsuffix .$(OBJX),$(TEST_KFG_SRC))

TEST_KFG_LIB = \
	-lkapp \
	-lvfs \
	-lkfg \
	-lkfs \
	-lkrypto \
	-lkproc \
	-lklib \
	-ldl \
	-lz \
	-lbz2 \
	-lktst

$(TEST_BINDIR)/test-kfg: $(TEST_KFG_OBJ)
	$(LP) --exe -o $@ $^ $(TEST_KFG_LIB)
