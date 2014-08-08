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
MODULE = test/align-test

include $(TOP)/build/Makefile.env

INT_TOOLS = \

EXT_TOOLS = \
#	samview \
#	align-test 

TEST_TOOLS = \
    test-align \

ifneq (win,$(OS))
    INT_TOOLS += test-ref-list
    INT_TOOLS += test-reference-mgr
    INT_TOOLS += test-ref_sub_select
endif    

ALL_TOOLS = \
	$(INT_TOOLS) \
	$(EXT_TOOLS) \
	$(TEST_TOOLS)

RWORKDIR = $(abspath .)
TEST_BINDIR=$(BINDIR)

#-------------------------------------------------------------------------------
# outer targets
#

#the following is for the benefit of align-test which is not build at all for now
#ifeq (win,$(OS))
#all std:
#	@ echo "not building align-test under Windows"
#else
all std: makedirs
	@ $(MAKE_CMD) $(TARGDIR)/std
#endif

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
	@ rm -f $(addsuffix *,$(addprefix $(TEST_BINDIR)/,$(ALL_TOOLS)))

.PHONY: clean

#-------------------------------------------------------------------------------
# tag
#
tag: \
	$(addsuffix _tag,$(ALL_TOOLS))

.PHONY: tag $(addsuffix _tag,$(ALL_TOOLS))

#-------------------------------------------------------------------------------
# align-test
#
ALIGN_TEST_SRC = \
	align_test

ALIGN_TEST_OBJ = \
	$(addsuffix .$(OBJX),$(ALIGN_TEST_SRC))

ALIGN_TEST_LIB = \
	-lkapp \
	-lkfg \
	-lkfs \
	-lklib \
	-lalign-access \
	-ldl \
	-lz \
	-lbz2 \
	-lm
#	-lksrch \


$(BINDIR)/align-test: $(ALIGN_TEST_OBJ)
	$(LD) --exe -o $@ $^ $(ALIGN_TEST_LIB)

align-test_tag:
	@ $(TOP)/build/tag-module.sh $(MODULE) align-test $(ALIGN_TEST_OBJ)

#-------------------------------------------------------------------------------
# samview
#
SAMVIEW_SRC = \
	samview \
#	bam

SAMVIEW_OBJ = \
	$(addsuffix .$(OBJX),$(SAMVIEW_SRC))

SAMVIEW_LIB = \
	-lkfs \
	-lklib \
	-lalign-access \
	-ldl \
	-lz \
	-lbz2


$(BINDIR)/samview: $(SAMVIEW_OBJ)
	$(LD) --exe -o $@ $^ $(SAMVIEW_LIB)

samview_tag:
	@ $(TOP)/build/tag-module.sh $(MODULE) align-test $(SAMVIEW_OBJ)

#-------------------------------------------------------------------------------
# test-align
#
TEST_ALIGN_SRC = \
	aligntest 

TEST_ALIGN_OBJ = \
	$(addsuffix .$(OBJX),$(TEST_ALIGN_SRC))

TEST_ALIGN_LIB = \
	-lkapp \
	-lalign-reader \
	$(READONLY_SCHEMA_LIBS) \
	-lsraschema \
	-lvdb \
	-lkdb \
	-lvfs \
	-ssrapath \
	-lkrypto \
	-lkfg \
	-lkfs \
	-lksrch \
	-lklib \
	-lm \
	-lktst \
    -lkproc

$(TEST_BINDIR)/test-align: $(TEST_ALIGN_OBJ)
	$(LP) --exe -o $@ $^ $(TEST_ALIGN_LIB)

#-------------------------------------------------------------------------------
# test-ref-list
#
TEST_REF_LIST_SRC = \
	test-ref-list

TEST_REF_LIST_OBJ = \
	$(addsuffix .$(OBJX),$(TEST_REF_LIST_SRC))

TEST_REF_LIST_LIB = \
	-lkapp \
	-ssrapath \
	-lalign-reader \
	$(READONLY_SCHEMA_LIBS) \
	-lsraschema \
	-lvdb \
	-lkdb \
	-lvfs \
	-lkrypto \
	-lkproc \
	-lkfg \
	-lkfs \
	-lksrch \
	-lklib \
	-lm \
	-lktst

$(TEST_BINDIR)/test-ref-list: $(TEST_REF_LIST_OBJ)
	$(LP) --exe -o $@ $^ $(TEST_REF_LIST_LIB)

#-------------------------------------------------------------------------------
# test-reference-mgr
#
TEST_REFERENCE_MGR_SRC = \
	test-reference-mgr

TEST_REFERENCE_MGR_OBJ = \
	$(addsuffix .$(OBJX),$(TEST_REFERENCE_MGR_SRC))

TEST_REFERENCE_MGR_LIB = \
	-lkapp \
	-lalign-writer \
	$(UPDATE_SCHEMA_LIBS) \
	-lsraschema \
	-lwvdb \
	-lwkdb \
	-lvfs \
	-ssrapath \
	-lkrypto \
	-lkproc \
	-lkfg \
	-lkfs \
	-lksrch \
	-lklib \
	-lm \
	-lktst

$(TEST_BINDIR)/test-reference-mgr: $(TEST_REFERENCE_MGR_OBJ)
	$(LP) --exe -o $@ $^ $(TEST_REFERENCE_MGR_LIB)

#-------------------------------------------------------------------------------
# test-ref_sub_select
#
TEST_REF_SUB_SELECT_SRC = \
	test-ref_sub_select

TEST_REF_SUB_SELECT_OBJ = \
	$(addsuffix .$(OBJX),$(TEST_REF_SUB_SELECT_SRC))

TEST_REF_SUB_SELECT_LIB = \
	-lalign-writer \
	-lsraschema \
	$(UPDATE_SCHEMA_LIBS) \
	-lwvdb \
	-lwkdb \
	-lvfs \
	-ssrapath \
	-lkrypto \
	-lkproc \
	-lkfg \
	-lkfs \
	-lksrch \
	-lkapp \
	-lklib \
	-lm \
	-lktst

$(TEST_BINDIR)/test-ref_sub_select: $(TEST_REF_SUB_SELECT_OBJ)
	$(LP) --exe -o $@ $^ $(TEST_REF_SUB_SELECT_LIB)
