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
MODULE = test/sra

include $(TOP)/build/Makefile.env

RWORKDIR = $(abspath .)

TEST_BINDIR=$(BINDIR)

TEST_TOOLS = \
	test-srapath

ifneq (win,$(OS))
	TEST_TOOLS += fd-leak
endif

EXT_TOOLS =

ALL_TOOLS = \
	$(TEST_TOOLS) \
	$(EXT_TOOLS)

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

#----------------------------------------------------------------
# test-sra
#

SRATEST_SRC = \
	sratest 

SRATEST_OBJ = \
	$(addsuffix .$(OBJX),$(SRATEST_SRC))

SRATEST_LIB = \
    -lkapp \
    -lvdb \
    -lkdb \
	-lksrch \
	-lvfs \
	-lkfg \
    -lkfs \
	-lkrypto \
	-lkproc \
    -lklib \
    -lktst

$(TEST_BINDIR)/test-srapath: $(SRATEST_OBJ)
	$(LP) --exe -o $@ $^ $(SRATEST_LIB)


#----------------------------------------------------------------
# fd-leak
#

FDLEAK_SRC = \
	fd-leak 

FDLEAK_OBJ = \
	$(addsuffix .$(OBJX),$(FDLEAK_SRC))

FDLEAK_LIB = \
	-lkapp \
	-lksrch \
	-lvfs \
	-lsradb \
	-lvdb \
	-lkdb \
	-lkfg \
	-lkfs \
	-lkrypto \
	-lkproc \
	-lklib \
    -lktst

$(TEST_BINDIR)/fd-leak: $(FDLEAK_OBJ)
	$(LP) --exe -o $@ $^ $(FDLEAK_LIB)

