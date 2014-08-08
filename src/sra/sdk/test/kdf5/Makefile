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
MODULE = test/kdf5

include $(TOP)/build/Makefile.env

INT_TOOLS = \

EXT_TOOLS = \
	kdf5-test

ALL_TOOLS = \
	$(INT_TOOLS) \
	$(EXT_TOOLS)

#-------------------------------------------------------------------------------
# where to find include/libs
#
ifdef NCBI
ifeq (dbg,$(BUILD))
	DFLT_LIBHDF5_INCLUDES = $(NCBI)/hdf5/Debug/include
	LIBHDF5_LPATH = $(NCBI)/hdf5/Debug/lib
else
	DFLT_LIBHDF5_INCLUDES = $(NCBI)/hdf5/Release/include
	LIBHDF5_LPATH = $(NCBI)/hdf5/Release/lib
endif
endif

DFLT_LIBHDF5_INCLUDES ?= /usr/include/hdf5
LIBHDF5_INCLUDES ?= $(DFLT_LIBHDF5_INCLUDES)
INCDIRS += $(addprefix -I,$(LIBHDF5_INCLUDES))


#-------------------------------------------------------------------------------
# outer targets
#
ifeq (win,$(OS))
all std:
	@ echo "not building vdb-copy under Windows"
else
all std: makedirs
	@ $(MAKE_CMD) $(TARGDIR)/$@
endif

$(ALL_TOOLS): makedirs
	@ $(MAKE_CMD) $(BINDIR)/$@

.PHONY: all std $(ALL_TOOLS)

#-------------------------------------------------------------------------------
# all
#
$(TARGDIR)/all: \
	$(addprefix $(BINDIR)/,$(ALL_TOOLS))

.PHONY: $(TARGDIR)/all

#-------------------------------------------------------------------------------
# std
#
$(TARGDIR)/std: \
	$(addprefix $(BINDIR)/,$(EXT_TOOLS))

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
# vdb-copy
#  vdb copy tool
#
KDF5_TEST_SRC = \
	kdf5-test

KDF5_TEST_OBJ = \
	$(addsuffix .$(OBJX),$(KDF5_TEST_SRC))

KDF5_TEST_LIB = \
	-lkapp \
	-lklib \
	-lvfs \
	-lkrypto \
	-lkfg \
	-lkfs \
	-lkdf5

ifdef LIBHDF5_LPATH
KDF5_TEST_LIB += -X$(LIBHDF5_LPATH) -lhdf5 
endif

$(BINDIR)/kdf5-test: $(KDF5_TEST_OBJ)
	$(warning KDF5_TEST_LIB=$(KDF5_TEST_LIB))$(LD) --exe -o $@ $^ $(KDF5_TEST_LIB)

hdf5-test_tag:
	@ $(TOP)/build/tag-module.sh $(MODULE) hdf5-test $(HDF5_TEST_OBJ)
