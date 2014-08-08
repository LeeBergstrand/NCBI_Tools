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
MODULE = test/xml

include $(TOP)/build/Makefile.env

INT_TOOLS = \
	test-xfs \
	test-xfs2 \
	test-xfs-uncompressed
#	test-xfs-gz

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
# test copycat xml kxfs
#
TEST_XFS_SRC = \
	test-xfs

TEST_XFS_OBJ = \
	$(addsuffix .$(OBJX),$(TEST_XFS_SRC))

TEST_XFS_LIB = \
	-lkxfs \
	-lkxml \
	-lkapp \
	-lvfs \
	-lkfs \
	-lklib \
	-ldl \
	-lz \
	-lbz2 \
	-lxml2

$(BINDIR)/test-xfs: $(TEST_XFS_OBJ)
	$(LD) --exe -o $@ $^ $(TEST_XFS_LIB)

#-------------------------------------------------------------------------------
# test copycat xml kxfs
#
TEST_XFS_SRC = \
	test-xfs2

TEST_XFS_OBJ = \
	$(addsuffix .$(OBJX),$(TEST_XFS_SRC))

TEST_XFS_LIB = \
	-lkxfs \
	-lkxml \
	-lkapp \
	-lvfs \
	-lkfg \
	-lkfs \
	-lklib \
	-ldl \
	-lz \
	-lbz2 \
	-lxml2

$(BINDIR)/test-xfs2: $(TEST_XFS_OBJ)
	$(LD) --exe -o $@ $^ $(TEST_XFS_LIB)

#-------------------------------------------------------------------------------
# test copycat xml kxfs (gzip reading)
#
TEST_XFS_GZ_SRC = \
	test-xfs-gz

TEST_XFS_GZ_OBJ = \
	$(addsuffix .$(OBJX),$(TEST_XFS_GZ_SRC))

TEST_XFS_GZ_LIB = \
	-lkxfs \
	-lkxml \
	-lkapp \
	-lvfs \
	-lkfs \
	-lklib \
	-ldl \
	-lz \
	-lbz2 \
	-lxml2

$(BINDIR)/test-xfs-gz: $(TEST_XFS_GZ_OBJ)
	$(LD) --exe -o $@ $^ $(TEST_XFS_GZ_LIB)

#----------------------------------------------------------------------------
# test copycat xml kxfs
# (reading directory uncompressed by copycat with KXTocDir)
#
TEST_XFS_UNCOMPRESSED_SRC = \
	test-xfs-uncompressed

TEST_XFS_UNCOMPRESSED_OBJ = \
	$(addsuffix .$(OBJX),$(TEST_XFS_UNCOMPRESSED_SRC))

TEST_XFS_UNCOMPRESSED_LIB = \
	-lkxfs \
	-lkxml \
	-lkapp \
	-lkfg \
	-lvfs \
	-lkfs \
	-lklib \
	-ldl \
	-lz \
	-lbz2 \
	-lxml2

$(BINDIR)/test-xfs-uncompressed: $(TEST_XFS_UNCOMPRESSED_OBJ)
	$(LD) --exe -o $@ $^ $(TEST_XFS_UNCOMPRESSED_LIB)
