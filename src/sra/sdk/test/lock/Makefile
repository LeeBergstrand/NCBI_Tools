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
MODULE = test/lock

TEST_TOOLS = \
	test-lock \
	test-lock2

EXT_TOOLS =

ALL_TOOLS = \
	$(EXT_TOOLS)

RWORKDIR = $(BINDIR)

RUNTESTS_OVERRIDE = 1

include $(TOP)/build/Makefile.env

#-------------------------------------------------------------------------------
# outer targets
#
all std: makedirs
	@ $(MAKE_CMD) $(TARGDIR)/std

$(ALL_TOOLS): makedirs
	@ $(MAKE_CMD) $(BINDIR)/$@

$(TEST_TOOLS): makedirs
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
	@ rm -f $(addsuffix *,$(addprefix $(TEST_BINDIR)/,$(ALL_TOOLS))); \
	rm -f file.lock

.PHONY: clean

#-------------------------------------------------------------------------------
# tag
#
tag: \
	$(addsuffix _tag,$(ALL_TOOLS))

.PHONY: tag $(addsuffix _tag,$(ALL_TOOLS))

#-------------------------------------------------------------------------------
# test-lock
#
TEST_LOCK_SRC = \
	locktest 

TEST_LOCK2_SRC = \
	locktest2 

TEST_LOCK_OBJ = \
	$(addsuffix .$(OBJX),$(TEST_LOCK_SRC))

TEST_LOCK2_OBJ = \
	$(addsuffix .$(OBJX),$(TEST_LOCK2_SRC))

TEST_LOCK_LIB = \
	-lkapp \
	-lvfs \
	-lkfg \
	-lkfs \
	-lkrypto \
	-lkproc \
	-lklib \
	-sktst

$(TEST_BINDIR)/test-lock: $(TEST_LOCK_OBJ)
	$(LP) --exe -o $@ $^ $(TEST_LOCK_LIB)

$(TEST_BINDIR)/test-lock2: $(TEST_LOCK2_OBJ)
	$(LP) --exe -o $@ $^ $(TEST_LOCK_LIB)


#-------------------------------------------------------------------------------
# runtests
#
runtests: std $(TEST_TOOLS)
	@ echo "$0 $*"; export LD_LIBRARY_PATH=$(LIBDIR);\
	echo ++++++++++++++++++++++++++++++++++++++++++++++++++++++;\
	eval "$(RUN_REMOTELY) $(TEST_BINDIR)/test-lock -l=all &"; pid=$$!;\
	eval "$(RUN_REMOTELY) $(TEST_BINDIR)/test-lock2 -l=all"; test "$$?" -eq "0" && wait $$pid

.PHONY: runtests $(TEST_TOOLS)
