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
MODULE = test/klib

include $(TOP)/build/Makefile.env

STD_TOOLS = \
	md5appendtest \
	time-test 

TEST_TOOLS = \
	test-klib

ifneq (win,$(OS))
TEST_TOOLS += \
	printf-test
endif

ALL_TOOLS = \
	$(STD_TOOLS) \
	$(TEST_TOOLS) \
#	triecreate

RWORKDIR = $(BINDIR)

#-------------------------------------------------------------------------------
# outer targets
#
all std: makedirs
	@ $(MAKE_CMD) $(TARGDIR)/$@

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
# all
#
$(TARGDIR)/all: \
	$(addprefix $(TEST_BINDIR)/,$(ALL_TOOLS))

.PHONY: $(TARGDIR)/all

#-------------------------------------------------------------------------------
# clean
#
clean: stdclean

.PHONY: clean

#----------------------------------------------------------------
# triecreate
#

TRIECREATE_SRC = \
	triecreate

TRIECREATE_OBJ = \
	$(addsuffix .$(OBJX),$(TRIECREATE_SRC))

TRIECREATE_LIB = \
	-skapp \
	-skfs \
	-sklib \
	-ldl \
	-lz \
	-lbz2

$(TEST_BINDIR)/triecreate: $(TRIECREATE_OBJ)
	$(LD) --exe -o $@ $^ $(TRIECREATE_LIB)

triecreate_tag:
	@ $(TOP)/build/tag-module.sh $(MODULE) triecreate $(TRIECREATE_OBJ)


#----------------------------------------------------------------
# md5appendtest
#

MD5APPENDTEST_SRC = \
	md5appendtest

MD5APPENDTEST_OBJ = \
	$(addsuffix .$(OBJX),$(MD5APPENDTEST_SRC))

MD5APPENDTEST_LIB = \
	-skapp \
	-svfs \
	-skfg \
	-skrypto \
	-skfs \
	-skproc \
	-sklib \
	-ldl \
	-lz \
	-lbz2

$(TEST_BINDIR)/md5appendtest: $(MD5APPENDTEST_OBJ)
	$(LD) --exe -o $@ $^ $(MD5APPENDTEST_LIB)

md5appendtest_tag:
	@ $(TOP)/build/tag-module.sh $(MODULE) md5appendtest $(MD5APPENDTEST_OBJ)




#----------------------------------------------------------------
# time-test
#

TIME_TEST_SRC = \
	time-test

TIME_TEST_OBJ = \
	$(addsuffix .$(OBJX),$(TIME_TEST_SRC))

TIME_TEST_LIB = \
	-skapp \
	-svfs \
	-skfg \
	-skrypto \
	-skfs \
	-skproc \
	-sklib

$(TEST_BINDIR)/time-test: $(TIME_TEST_OBJ)
	$(LD) --exe -o $@ $^ $(TIME_TEST_LIB)

time-test_tag:
	@ $(TOP)/build/tag-module.sh $(MODULE) time-test $(TIME_TEST_OBJ)


#----------------------------------------------------------------
# printf-test
#

PRINTF_TEST_SRC = \
	printf-test

PRINTF_TEST_OBJ = \
	$(addsuffix .$(OBJX),$(PRINTF_TEST_SRC))

PRINTF_TEST_LIB = \
	-lkapp \
	-lvfs \
	-lkfg \
	-lkrypto \
	-lkfs \
	-lkproc \
	-lklib

$(TEST_BINDIR)/printf-test: $(PRINTF_TEST_OBJ)
	$(LD) --exe -o $@ $^ $(PRINTF_TEST_LIB)

printf-test_tag:
	@ $(TOP)/build/tag-module.sh $(MODULE) printf_test $(PRINTF_TEST_OBJ)

#-------------------------------------------------------------------------------
# test-klib
#
TEST_KLIB_SRC = \
	test-klib

TEST_KLIB_OBJ = \
	$(addsuffix .$(OBJX),$(TEST_KLIB_SRC))

TEST_KLIB_LIB = \
	-lkapp \
	-lvfs \
	-lkfg \
	-lkrypto \
	-lkfs \
	-lkproc \
	-lklib \
	-lktst

$(TEST_BINDIR)/test-klib: $(TEST_KLIB_OBJ)
	$(LP) --exe -o $@ $^ $(TEST_KLIB_LIB)
