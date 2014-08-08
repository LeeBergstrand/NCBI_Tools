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
MODULE = test/blast

include $(TOP)/build/Makefile.env

INT_TOOLS = \
	 test-blast \
	 unit-test-blast

EXT_TOOLS =

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
tag: \
	$(addsuffix _tag,$(ALL_TOOLS))

.PHONY: tag $(addsuffix _tag,$(ALL_TOOLS))


#-------------------------------------------------------------------------------
# test vdb-blast
#
TEST_BLAST_SRC = \
	test-blast

TEST_BLAST_OBJ = \
	$(addsuffix .$(OBJX),$(TEST_BLAST_SRC))

TEST_BLAST_LIB = \
	-lktst \
	-lkapp \
	-ssrapath \
	-lalign-reader \
	-lvdb-blast \
	-lsraschema \
	$(READONLY_SCHEMA_LIBS) \
	-lvdb \
	-lkdb \
	-lkfg \
	-lvfs \
	-lkrypto \
    -lkproc \
	-lkfs \
	-lksrch \
	-lklib \
	-lpthread \
	-lm

#	-ldl \
	-lz \
	-lbz2

$(BINDIR)/test-blast: $(TEST_BLAST_OBJ)
	$(LP) --exe -o $@ $^ $(TEST_BLAST_LIB)

#-------------------------------------------------------------------------------
# unit test vdb-blast
#
UNIT_TEST_BLAST_SRC = \
	unit-test-blast

UNIT_TEST_BLAST_LIB = \
	$(TEST_BLAST_LIB)

UNIT_TEST_BLAST_OBJ = \
	$(addsuffix .$(OBJX),$(UNIT_TEST_BLAST_SRC))

$(BINDIR)/unit-test-blast: $(UNIT_TEST_BLAST_OBJ)
	$(LP) --exe -o $@ $^ $(UNIT_TEST_BLAST_LIB)
