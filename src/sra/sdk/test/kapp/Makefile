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
MODULE = test/kapp

include $(TOP)/build/Makefile.env

INT_TOOLS = \

ifneq (win,$(OS))
	INT_TOOLS += qfiletest
endif

EXT_TOOLS =

TEST_TOOLS = \
	args-test

ALL_TOOLS = \
	$(INT_TOOLS) \
	$(EXT_TOOLS) \
	$(TEST_TOOLS)

RWORKDIR = $(BINDIR)

#-------------------------------------------------------------------------------
# outer targets
#
all std: makedirs
	@ $(MAKE_CMD) $(TARGDIR)/std

$(ALL_TOOLS): makedirs
	@ $(MAKE_CMD) $(TEST_BINDIR)/$@

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
# args-test
#
ARGS_TEST_SRC = \
	args-test

ARGS_TEST_OBJ = \
	$(addsuffix .$(OBJX),$(ARGS_TEST_SRC))

ifneq (win,$(OS))
	ARGS_TEST_LOAD_LIB = -lkq
endif

ARGS_TEST_LIB = \
	-lkapp \
    -lload \
    -lwkdb \
	-lvfs \
	-lkfg \
	-lkrypto \
	-lkfs \
	$(ARGS_TEST_LOAD_LIB) \
	-lkproc \
	-lklib \
	-lm \
	-lktst 

$(TEST_BINDIR)/args-test: $(ARGS_TEST_OBJ)
	$(LP) --exe -o $@ $^ $(ARGS_TEST_LIB)

#-------------------------------------------------------------------------------
# example unit test
#
BYTEORDER_SRC = \
	test-byteorder

BYTEORDER_OBJ = \
	$(addsuffix .$(OBJX),$(BYTEORDER_SRC))

BYTEORDER_LIB = \

$(BINDIR)/test-byteorder: $(addprefix $(SRCDIR)/,$(BYTEORDER_COL))

.PHONY: test-byteorder

test-byteorder: $(BINDIR)/test-byteorder

$(BINDIR)/test-byteorder: $(BYTEORDER_OBJ)
	$(LD) --exe -o $@ $^ $(BYTEORDER_LIB)

#-------------------------------------------------------------------------------
# my-app
#
QFILETEST_SRC = \
	qfiletest

QFILETEST_OBJ = \
	$(addsuffix .$(OBJX),$(QFILETEST_SRC))

QFILETEST_LIB = \
	-lkapp \
	-lwvdb \
	-lwkdb \
	-lvfs \
	-lkfg \
	-lload \
	-lkrypto \
	-lkfs \
	-lkq \
	-lkproc \
	-lklib

$(TEST_BINDIR)/qfiletest: $(QFILETEST_OBJ)
	$(LD) --exe $(SRCDIR) -o $@ $^ $(QFILETEST_LIB)

qfiletest_tag:
	@ $(TOP)/build/tag-module.sh $(MODULE) qfiletest $(QFILETEST_OBJ)
