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
MODULE = test/printf

include $(TOP)/build/Makefile.env

INT_TOOLS = \

EXT_TOOLS = \
	test-float \
	test-error \
	test_printf

ALL_TOOLS = \
	$(INT_TOOLS) \
	$(EXT_TOOLS)

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
	$(addprefix $(BINDIR)/,$(ALL_TOOLS))

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
# my-app
#
TEST_SRC = \
	test_printf

TEST_OBJ = \
	$(addsuffix .$(OBJX),$(TEST_SRC))

TEST_LIB = \
	-skapp \
	-lvfs \
	-lkfg \
	-lkfs \
	-lkrypto \
	-lkproc \
	-lklib \
	-ldl

$(BINDIR)/test_printf: $(TEST_OBJ)
	$(LD) --exe $(SRCDIR) -o $@ $^ $(TEST_LIB)

test_printf_tag:
	@ $(TOP)/build/tag-module.sh $(MODULE) test_printf $(TEST_OBJ)

#-------------------------------------------------------------------------------
TEST_FLOAT_SRC = \
	test-float

TEST_FLOAT_OBJ = \
	$(addsuffix .$(OBJX),$(TEST_FLOAT_SRC))

TEST_FLOAT_LIB = \
	-skapp \
	-lvfs \
	-lkfg \
	-lkfs \
	-lkrypto \
	-lkproc \
	-lklib \
	-ldl

$(BINDIR)/test-float: $(TEST_FLOAT_OBJ)
	$(LD) --exe $(SRCDIR) -o $@ $^ $(TEST_FLOAT_LIB)

test_float_tag:
	@ $(TOP)/build/tag-module.sh $(MODULE) test-float $(TEST_FLOAT_OBJ)


#-------------------------------------------------------------------------------
TEST_ERROR_SRC = \
	test-error

TEST_ERROR_OBJ = \
	$(addsuffix .$(OBJX),$(TEST_ERROR_SRC))

TEST_ERROR_LIB = \
	-skapp \
	-lvfs \
	-lkfg \
	-lkfs \
	-lkrypto \
	-lkproc \
	-lklib \
	-ldl

$(BINDIR)/test-error: $(TEST_ERROR_OBJ)
	$(LD) --exe $(SRCDIR) -o $@ $^ $(TEST_ERROR_LIB)

test_error_tag:
	@ $(TOP)/build/tag-module.sh $(MODULE) test-error $(TEST_ERROR_OBJ)
