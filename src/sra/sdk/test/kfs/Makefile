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
MODULE = test/kfs

include $(TOP)/build/Makefile.env
RWORKDIR = $(TOP)/$(MODULE)

INT_TOOLS = \
	test-sysfs \
	test-buffile \
	ktartest \
	test-pagefile \
	test-kpath-read-path \
	test-bzip-concat \
	test-sysfile-timeout \
	test-headfile


TEST_TOOLS = \
	test-kfs \
	test-ramfile \
	test-resolve

ifneq (win,$(OS))
	INT_TOOLS += test-kfsmanager
endif

# sparse file is automagic in unix so no testing
ifeq (win,$(OS))
	TEST_TOOLS += test-sparse
endif

EXT_TOOLS =

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
	@ rm -f $(addsuffix *,$(addprefix $(TEST_BINDIR)/,$(ALL_TOOLS)))

.PHONY: clean

#-------------------------------------------------------------------------------
# tag
#
tag: \
	$(addsuffix _tag,$(ALL_TOOLS))

.PHONY: tag $(addsuffix _tag,$(ALL_TOOLS))

#-------------------------------------------------------------------------------
# test os-native file system
#
TEST_SYSFS_SRC = \
	test-sysfs

TEST_SYSFS_OBJ = \
	$(addsuffix .$(OBJX),$(TEST_SYSFS_SRC))

TEST_SYSFS_LIB = \
	-lkapp \
	-lvfs \
	-lkfg \
	-lkrypto \
	-lkfs \
	-lkproc \
	-lklib \
	-ldl \
	-lz \
	-lbz2

$(TEST_BINDIR)/test-sysfs: $(TEST_SYSFS_OBJ)
	$(LD) --exe -o $@ $^ $(TEST_SYSFS_LIB)

#-------------------------------------------------------------------------------
# test buffered file
#
TEST_BUFFILE_SRC = \
	test-buffile

TEST_BUFFILE_OBJ = \
	$(addsuffix .$(OBJX),$(TEST_BUFFILE_SRC))

TEST_BUFFILE_LIB = \
	-lkapp \
	-lvfs \
	-lkfg \
	-lkrypto \
	-lkfs \
	-lkproc \
	-lklib \
	-ldl \
	-lz \
	-lbz2

$(TEST_BINDIR)/test-buffile: $(TEST_BUFFILE_OBJ)
	$(LD) --exe -o $@ $^ $(TEST_BUFFILE_LIB)


#-------------------------------------------------------------------------------
# test tar component of KFS - gives us our view of a tar file
#
KTARTEST_SRC = \
	ktartest

KTARTEST_OBJ = \
	$(addsuffix .$(OBJX),$(KTARTEST_SRC))

KTARTEST_LIB = \
	-lkapp \
	-lvfs \
	-lkfg \
	-lkrypto \
	-lkfs \
	-lkproc \
	-lklib \
	-ldl \
	-lz \
	-lbz2

$(TEST_BINDIR)/ktartest: $(KTARTEST_OBJ)
	$(LD) --exe -o $@ $^ $(KTARTEST_LIB)


#-------------------------------------------------------------------------------
# test pagefile
#
TEST_PAGEFILE_SRC = \
	test-pagefile

TEST_PAGEFILE_OBJ = \
	$(addsuffix .$(OBJX),$(TEST_PAGEFILE_SRC))

TEST_PAGEFILE_LIB = \
	-lkapp \
	-lvfs \
	-lkfg \
	-lkrypto \
	-lkfs \
	-lkproc \
	-lklib \
	-ldl \
	-lz \
	-lbz2

$(TEST_BINDIR)/test-pagefile: $(TEST_PAGEFILE_OBJ)
	$(LD) --exe -o $@ $^ $(TEST_PAGEFILE_LIB)


#-------------------------------------------------------------------------------
# test test-kpath-read-path
#
TEST_KPATH_READ_PATH_SRC = \
	test-kpath-read-path

TEST_KPATH_READ_PATH_OBJ = \
	$(addsuffix .$(OBJX),$(TEST_KPATH_READ_PATH_SRC))

TEST_KPATH_READ_PATH_LIB = \
	-lkapp \
	-lvfs \
	-lkfg \
	-lkrypto \
	-lkfs \
	-lkproc \
	-lklib \
	-ldl \
	-lz \
	-lbz2

$(TEST_BINDIR)/test-kpath-read-path: $(TEST_KPATH_READ_PATH_OBJ)
	$(LD) --exe -o $@ $^ $(TEST_KPATH_READ_PATH_LIB)

#-------------------------------------------------------------------------------
# test-bzip-concat
#
TEST_BZIP_CONCAT_SRC = \
	test-bzip-concat

TEST_BZIP_CONCAT_OBJ = \
	$(addsuffix .$(OBJX),$(TEST_BZIP_CONCAT_SRC))

TEST_BZIP_CONCAT_LIB = \
	-lkapp \
	-lvfs \
	-lkfg \
	-lkrypto \
	-lkfs \
	-lkproc \
	-lklib \
	-ldl \
	-lbz2

$(TEST_BINDIR)/test-bzip-concat: $(TEST_BZIP_CONCAT_OBJ)
	$(LD) --exe -o $@ $^ $(TEST_BZIP_CONCAT_LIB)

#-------------------------------------------------------------------------------
# test-sysfile-timeout
#
TEST_SYSFILE_TIMEOUT_SRC = \
	test-sysfile-timeout

TEST_SYSFILE_TIMEOUT_OBJ = \
	$(addsuffix .$(OBJX),$(TEST_SYSFILE_TIMEOUT_SRC))

TEST_SYSFILE_TIMEOUT_LIB = \
	-lkapp \
	-lvfs \
	-lkfg \
	-lkrypto \
	-lkfs \
	-lkproc \
	-lklib \
	-ldl \
	-lbz2

$(TEST_BINDIR)/test-sysfile-timeout: $(TEST_SYSFILE_TIMEOUT_OBJ)
	$(LD) --exe -o $@ $^ $(TEST_SYSFILE_TIMEOUT_LIB)

#-------------------------------------------------------------------------------
# test-kfsmanager
#
TEST_KFSMANAGER_SRC = \
	test-kfsmanager

TEST_KFSMANAGER_OBJ = \
	$(addsuffix .$(OBJX),$(TEST_KFSMANAGER_SRC))

TEST_KFSMANAGER_LIB = \
	-lkapp \
	-lvfs \
	-lkrypto \
	-lkfg \
	-lkfs \
	-lkproc \
	-lklib \
	-ldl \
	-lbz2

$(TEST_BINDIR)/test-kfsmanager: $(TEST_KFSMANAGER_OBJ)
	$(LD) --exe -o $@ $^ $(TEST_KFSMANAGER_LIB)


#-------------------------------------------------------------------------------
# test headfile
#
TEST_HEADFILE_SRC = \
	test-headfile

TEST_HEADFILE_OBJ = \
	$(addsuffix .$(OBJX),$(TEST_HEADFILE_SRC))

TEST_HEADFILE_LIB = \
	-lkapp \
	-lvfs \
	-lkfg \
	-lkrypto \
	-lkfs \
	-lkproc \
	-lklib \
	-ldl \
	-lz \
	-lbz2

$(TEST_BINDIR)/test-headfile: $(TEST_HEADFILE_OBJ)
	$(LD) --exe -o $@ $^ $(TEST_HEADFILE_LIB)


#-------------------------------------------------------------------------------
# test-kfs
#
TEST_KFS_SRC = \
	kfstest 

TEST_KFS_OBJ = \
	$(addsuffix .$(OBJX),$(TEST_KFS_SRC))

TEST_KFS_LIB = \
	-lkapp \
	-lvfs \
	-lkfg \
	-lkrypto \
	-lkfs \
	-lkproc \
	-lklib \
	-lktst

$(TEST_BINDIR)/test-kfs: $(TEST_KFS_OBJ)
	$(LP) --exe -o $@ $^ $(TEST_KFS_LIB)

#-------------------------------------------------------------------------------
# test-ramfile
#
TEST_RAMFILE_SRC = \
	ramfiletest 

TEST_RAMFILE_OBJ = \
	$(addsuffix .$(OBJX),$(TEST_RAMFILE_SRC))

TEST_RAMFILE_LIB = \
	-lkapp \
	-lvfs \
	-lkfg \
	-lkrypto \
	-lkfs \
	-lkproc \
	-lklib \
	-lktst

$(TEST_BINDIR)/test-ramfile: $(TEST_RAMFILE_OBJ)
	$(LP) --exe -o $@ $^ $(TEST_RAMFILE_LIB)


#-------------------------------------------------------------------------------
# test-resolve
#
TEST_RESOLVE_SRC = \
	resolvetest 

TEST_RESOLVE_OBJ = \
	$(addsuffix .$(OBJX),$(TEST_RESOLVE_SRC))

TEST_RESOLVE_LIB = \
	-lkapp \
	-lvfs \
	-lkfg \
	-lkrypto \
	-lkfs \
	-lkproc \
	-lklib \
	-lktst

$(TEST_BINDIR)/test-resolve: $(TEST_RESOLVE_OBJ)
	$(LP) --exe -o $@ $^ $(TEST_RESOLVE_LIB)

#-------------------------------------------------------------------------------
# test-sparse
#
TEST_SPARSE_SRC = \
	sparsetest 

TEST_SPARSE_OBJ = \
	$(addsuffix .$(OBJX),$(TEST_SPARSE_SRC))

TEST_SPARSE_LIB = \
	-lkapp \
	-lvfs \
	-lkfg \
	-lkrypto \
	-lkfs \
	-lkproc \
	-lklib \
	-lktst

$(TEST_BINDIR)/test-sparse: $(TEST_SPARSE_OBJ)
	$(LP) --exe -o $@ $^ $(TEST_SPARSE_LIB)
