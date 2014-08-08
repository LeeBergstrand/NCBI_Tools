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
MODULE = services/fuse

include $(TOP)/build/Makefile.env

INT_TOOLS =

EXT_TOOLS = \
	sra-fuser \
#	sra-makeidx

ALL_TOOLS = \
	$(INT_TOOLS) \
	$(EXT_TOOLS)

#-------------------------------------------------------------------------------
# outer targets
#
all std: vers-includes
	@ $(MAKE_CMD) $(TARGDIR)/std

$(ALL_TOOLS): vers-includes
	@ $(MAKE_CMD) $(BINDIR)/$@

.PHONY: all std $(ALL_TOOLS)

#-------------------------------------------------------------------------------
# std
#
$(TARGDIR)/std: \
	$(addprefix $(BINDIR)/,$(ALL_TOOLS))

.PHONY: $(TARGDIR)/std

#-------------------------------------------------------------------------------
# vers-includes
#
$(TARGDIR)/vers-includes: $(addsuffix .vers.h,$(EXT_TOOLS))

.PHONY: $(TARGDIR)/vers-includes

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
# sra-fuser
#
SRA_FUSER_SRC = \
        zlib-simple \
        log \
        node \
        tar-list \
        file \
        tar-file \
        tar-node \
        directory \
        formats \
        accessor \
        kfile-accessor \
        xml \
        text-file \
        sra-list \
        sra-directory \
        sra-node \
        sra-fastq \
        sra-sff \
        sra-fuser-sys \
        sra-fuser

SRA_FUSER_OBJ = \
	$(addsuffix .$(OBJX),$(SRA_FUSER_SRC))

LIBXML = -lxml2
ifdef NCBI
CFLAGS += -DSRAFUSER_LOGLOCALTIME
ifeq (linux,$(OS))
   LIBXML = -X$(LIBXML_LPATH) -lxml2-static
endif
ifeq (sun,$(OS))
   LIBXML = -X$(LIBXML_LPATH) -lxml2-static
endif
endif

SRA_FUSER_LIB = \
	-lkapp \
	$(ALWAYS_STATIC_SCHEMA_LIBS) \
	-salign-reader \
	-lsradb \
	-ssrapath \
	-ssrareader \
	-lvdb \
	-lkdb \
	-lvfs \
	-lkrypto \
	-lkfg \
	-lkproc \
	-lkfs \
	-dksrch \
	-lklib \
	-lkxml \
	$(LIBXML) \
	-ldl \
	-lfuse \
	-lz \
	-lbz2 \
	-lm \
	-lrt

$(BINDIR)/sra-fuser: $(SRA_FUSER_OBJ)
	$(LD) --exe --vers $(SRCDIR) -o $@ $^ $(SRA_FUSER_LIB)

sra-fuser_tag:
	@ $(TOP)/build/tag-module.sh $(MODULE) sra-fuser $(SRA_FUSER_OBJ)

#-------------------------------------------------------------------------------
# sra-fuser
#
SRA_MAKEIDX_SRC = \
    zlib-simple \
	sra-makeidx

SRA_MAKEIDX_OBJ = \
	$(addsuffix .$(OBJX),$(SRA_MAKEIDX_SRC))

SRA_MAKEIDX_LIB = \
	-lkapp \
	$(UPDATE_SCHEMA_LIBS) \
	-ssrapath\
	-ssrareader \
	-salign-reader \
	-lwsradb \
	-lwvdb \
	-lwkdb \
	-lvfs \
	-lkrypto \
	-lkfg \
	-lkfs \
	-dksrch \
	-lklib \
	-lkproc \
	-ldl \
	-lz \
	-lbz2 \
	-lpthread \
	-lm

$(BINDIR)/sra-makeidx: $(SRA_MAKEIDX_OBJ)
	$(LD) --exe --vers $(SRCDIR) -o $@ $^ $(SRA_MAKEIDX_LIB)

sra-makeidx_tag:
	@ $(TOP)/build/tag-module.sh $(MODULE) sra-makeidx $(SRA_MAKEIDX_OBJ)
