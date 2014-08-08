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
MODULE = test/checksum

RUNTESTS_OVERRIDE = 1
include $(TOP)/build/Makefile.env

TEST_TOOLS = \
	basic-checksum-test \
	vxf-checksum-test

#-------------------------------------------------------------------------------
# outer targets
#

ifeq (win,$(OS))
all std runtests:
	@ echo "not building checksum tests under Windows"
else
all std: makedirs
	@ $(MAKE_CMD) $(TARGDIR)/std
endif

$(TEST_TOOLS): makedirs
	@ $(MAKE_CMD) $(TEST_BINDIR)/$@    

.PHONY: all std runtests $(TEST_TOOLS)

#-------------------------------------------------------------------------------
# std
#
$(TARGDIR)/std: \
	$(addprefix $(TEST_BINDIR)/,$(TEST_TOOLS)) \
    
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
# runtests
#
ifneq (win,$(OS))
runtests: std
	@echo ++++++++++++++++++++++++++++++++++++++++++++++++++++++
	@echo Run basic-checksum-test
	@( mkdir test-checksum-dir; cd test-checksum-dir; cp $(SRCDIR)/basic-test.c .; $(TEST_BINDIR)/basic-checksum-test; rc=$$?; cd ..; rm -fr test-checksum-dir; exit $$rc )
	@echo Run vxf-checksum-test
	@( mkdir test-checksum-dir; cd test-checksum-dir; cp $(SRCDIR)/vdb-test.c .; $(TEST_BINDIR)/vxf-checksum-test; rc=$$?; cd ..; rm -fr test-checksum-dir; exit $$rc )
endif

#-------------------------------------------------------------------------------
# basic-test
#
BASIC_SRC = \
	basic-test

BASIC_OBJ = \
	$(addsuffix .$(OBJX),$(BASIC_SRC))

BASIC_LIB = \
	-skapp \
    -lvfs \
    -lkrypto \
    -lkfg \
    -lkfs \
	-lklib \
	-ldl \
	-lz \
	-lbz2


$(TEST_BINDIR)/basic-checksum-test: $(BASIC_OBJ)
	$(LD) --exe -o $@ $^ $(BASIC_LIB)

basic_checksum_test_tag:
	@ $(TOP)/build/tag-module.sh $(MODULE) basic-checksum-test $(BASIC_OBJ)

#-------------------------------------------------------------------------------
# vdb-test
#
VXF_TEST_SRC = \
	vdb-test

VXF_TEST_OBJ = \
	$(addsuffix .$(OBJX),$(VXF_TEST_SRC))

VXF_TEST_LIB = \
	-skapp \
    -lvfs \
    -lkrypto \
	-lkfs \
	-lklib \
	-lkfg \
	-lwkdb \
	-lwvdb \
	-ldl \
	-lz \
	-lbz2

$(TEST_BINDIR)/vxf-checksum-test: $(VXF_TEST_OBJ)
	$(LD) --exe -o $@ $^ $(VXF_TEST_LIB)

vxf_checksum_test_tag:
	@ $(TOP)/build/tag-module.sh $(MODULE) vxf-checksum-test $(VXF_TEST_OBJ)

