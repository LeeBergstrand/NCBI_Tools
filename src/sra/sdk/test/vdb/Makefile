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
MODULE = test/vdb

include $(TOP)/build/Makefile.env

INT_TOOLS = \
	SRR574828-crash-test \
	kdb2vdb \
	double-VCursorCommit-test \
	test-static \
	test-vdb-resolve \
	re-compress

EXT_TOOLS =

ALL_TOOLS = \
	$(INT_TOOLS) \
	$(EXT_TOOLS)
    
TEST_TOOLS = \
    test-vdb

#-------------------------------------------------------------------------------
# outer targets
#
all std: makedirs
	@ $(MAKE_CMD) $(TARGDIR)/std

$(ALL_TOOLS): makedirs
	@ $(MAKE_CMD) $(BINDIR)/$@

$(TEST_TOOLS): makedirs
	@ $(MAKE_CMD) $(TEST_BINDIR)/$@
    
.PHONY: all std $(ALL_TOOLS) $(TEST_TOOLS)

#-------------------------------------------------------------------------------
# std
#
$(TARGDIR)/std: \
	$(addprefix $(BINDIR)/,$(ALL_TOOLS)) \
	$(addprefix $(TEST_BINDIR)/,$(TEST_TOOLS))
#	$(TARGDIR)/run_test_static

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
# vdb-test-aa - simple test of schema, table creation, and single column load
#
VDB_TEST_AA_SRC = \
	vdb-test-aa

VDB_TEST_AA_OBJ = \
	$(addsuffix .$(OBJX),$(VDB_TEST_AA_SRC))

VDB_TEST_AA_COL = \
	test-aa-col1

VDB_TEST_AA_LIB = \
	-lkapp \
	-lwvdb \
	-lwkdb \
	-lkfs \
	-lklib \
	-lkfg \
	-lm \
	-ldl \
	-lz \
	-lbz2

$(BINDIR)/vdb-test-aa: $(addprefix $(SRCDIR)/,$(VDB_TEST_AA_COL))

$(BINDIR)/vdb-test-aa: $(VDB_TEST_AA_OBJ)
	$(LD) --exe -o $@ $^ $(VDB_TEST_AA_LIB)

$(SRCDIR)/test-aa-col1: $(SRCDIR)/Makefile
	@ touch $@

vdb-test-aa_tag:
	@ $(TOP)/build/tag-module.sh $(MODULE) vdb-test-aa $(VDB_TEST_AA_OBJ)


#-------------------------------------------------------------------------------
# column testing tool
#
KDB2VDB_SRC = \
	kdb2vdb

KDB2VDB_OBJ = \
	$(addsuffix .$(OBJX),$(KDB2VDB_SRC))

KDB2VDB_LIB = \
	-lkapp \
	-lwvdb \
	-lwkdb \
	-lvfs \
	-lkfg \
	-lkfs \
	-lkrypto \
	-lkproc \
	-lklib \
	-lm \
	-ldl \
	-lz \
	-lbz2

$(BINDIR)/kdb2vdb: $(KDB2VDB_OBJ)
	$(LD) --exe -o $@ $^ $(KDB2VDB_LIB)

#-------------------------------------------------------------------------------
DOUBLE_VCURSOR_COMMIT_SRC = \
	double-VCursorCommit-test
DOUBLE_VCURSOR_COMMIT_OBJ = \
	$(addsuffix .$(OBJX),$(DOUBLE_VCURSOR_COMMIT_SRC))
DOUBLE_VCURSOR_COMMIT_LIB = \
	-lkapp \
	-lwvdb \
	-lwkdb \
	-lvfs \
	-lkfs \
	-lkrypto \
	-lkfg \
	-lkproc \
	-lklib

$(BINDIR)/double-VCursorCommit-test: $(DOUBLE_VCURSOR_COMMIT_OBJ)
	$(LD) --exe -o $@ $^ $(DOUBLE_VCURSOR_COMMIT_LIB)

#-------------------------------------------------------------------------------
SRR574828_SRC = \
	SRR574828-crash-test
SRR574828_OBJ = \
	$(addsuffix .$(OBJX),$(SRR574828_SRC))
SRR574828_LIB = \
	-lkapp \
	-lvdb \
	-lkdb \
	-lvfs \
	-lkfs \
	-lkrypto \
	-lkfg \
	-lkproc \
	-lklib

$(BINDIR)/SRR574828-crash-test: $(SRR574828_OBJ)
	$(LD) --exe -o $@ $^ $(SRR574828_LIB)

#-------------------------------------------------------------------------------

# static column unit test
#
TEST_STATIC_SRC = \
	test-static

TEST_STATIC_OBJ = \
	$(addsuffix .$(OBJX),$(TEST_STATIC_SRC))

TEST_STATIC_LIB = \
	-lkapp \
	-lkproc \
    -lvfs \
    -lkrypto \
	-lwvdb \
	-lwkdb \
	-lkfs \
	-lklib \
	-lkfg \
	-lm \
	-ldl \
	-lz \
	-lbz2

ifdef TEST_STATIC_COL
$(BINDIR)/test-static: $(addprefix $(SRCDIR)/,$(TEST_STATIC_COL))
endif

$(BINDIR)/test-static: $(TEST_STATIC_OBJ)
	$(LD) --exe -o $@ $^ $(TEST_STATIC_LIB)

$(TARGDIR)/run_test_static: $(BINDIR)/test-static
	@echo ++++++++++++++++++++++++++++++++++++++++++++++++++++++
	@echo Run test-static $(BINDIR)
	@( mkdir test-static-dir; cd test-static-dir; $(BINDIR)/test-static; cd ..; rm -fr test-static-dir)

.PHONY: $(TARGDIR)/run_test_static


#-------------------------------------------------------------------------------
# vdb-depend + resolver test
#

RESOLVE_SRC = \
	test-vdb-resolve

RESOLVE_OBJ = \
	$(addsuffix .$(OBJX),$(RESOLVE_SRC))

RESOLVE_LIB = \
	-dalign-reader \
	$(READONLY_SCHEMA_LIBS) \
	-dksrch \
	-lsradb \
	-lvdb \
	-lkdb \
	-lkapp \
	-lvfs \
	-lkrypto \
	-lkfg \
	-lkfs \
	-lkproc \
	-lklib \
	-lm

$(BINDIR)/test-vdb-resolve: $(RESOLVE_OBJ)
	$(LD) --exe --vers $(SRCDIR) -o $@ $^ $(RESOLVE_LIB)

test-vdb-resolve_tag:
	@ $(TOP)/build/tag-module.sh \
		$(MODULE) table-vers $(RESOLVE_OBJ)


#-------------------------------------------------------------------------------
# column recompression tool
#
RE_COMPRESS_SRC = \
	re-compress

RE_COMPRESS_OBJ = \
	$(addsuffix .$(OBJX),$(RE_COMPRESS_SRC))

RE_COMPRESS_LIB = \
	-lkapp \
	-lkproc \
    -lvfs \
    -lkrypto \
	-lwvdb \
	-lwkdb \
	-lkfs \
	-lklib \
	-lkfg \
	-lm \
	-ldl \
	-lz \
	-lbz2

$(BINDIR)/re-compress: $(RE_COMPRESS_OBJ)
	$(LD) --exe -o $@ $^ $(RE_COMPRESS_LIB)

#-------------------------------------------------------------------------------
# test-vdb
#
TEST_VDB_SRC = \
	test-vdb-manager

TEST_VDB_OBJ = \
	$(addsuffix .$(OBJX),$(TEST_VDB_SRC))

TEST_VDB_LIB = \
    -lvdb \
    -lkdb \
    -lvfs \
    -lkfs \
    -lkrypto \
    -lkfg \
    -lkapp \
	-lkproc \
    -lklib \
	-lktst \

$(TEST_BINDIR)/test-vdb: $(TEST_VDB_OBJ)
	$(LP) --exe -o $@ $^ $(TEST_VDB_LIB)
