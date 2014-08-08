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
MODULE = test/curl_fetch

include $(TOP)/build/Makefile.env

INT_TOOLS = \

EXT_TOOLS = \
	curl-fetch \
	kget \
	kpost

ALL_TOOLS = \
	$(INT_TOOLS) \
	$(EXT_TOOLS)

#-------------------------------------------------------------------------------
# outer targets
#
all std: vers-includes
	@ $(MAKE_CMD) $(TARGDIR)/$@

$(ALL_TOOLS): vers-includes
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
tag: $(addsuffix _tag,$(ALL_TOOLS))

.PHONY: tag $(addsuffix _tag,$(ALL_TOOLS))


#-------------------------------------------------------------------------------
# curl-fetch
#
CURL_FETCH_SRC = \
	curl-fetch

CURL_FETCH_OBJ = \
	$(addsuffix .$(OBJX),$(CURL_FETCH_SRC))

#linking agains vfs automatically brings in kurl (what we need here)
CURL_FETCH_LIB = \
	-lkapp \
	-lvfs \
	-lkfg \
	-lkrypto \
	-lkfs \
	-lkproc \
	-lklib \
	-lktst

$(BINDIR)/curl-fetch: $(CURL_FETCH_OBJ)
	$(LD) --exe $(SRCDIR) -o $@ $^ $(CURL_FETCH_LIB)

curl-fetch_tag:
	@ $(TOP)/build/tag-module.sh $(MODULE) curl-fetch $(CURL_FETCH_OBJ)


#-------------------------------------------------------------------------------
# kget
#
KGET_SRC = \
	kget

KGET_OBJ = \
	$(addsuffix .$(OBJX),$(KGET_SRC))

#linking agains vfs automatically brings in kurl (what we need here)
KGET_LIB = \
	-lkapp \
	-lvfs \
	-lkfg \
	-lkrypto \
	-lkfs \
	-lkproc \
	-lklib \
	-lktst

$(BINDIR)/kget: $(KGET_OBJ)
	$(LD) --exe $(SRCDIR) -o $@ $^ $(KGET_LIB)

kget_tag:
	@ $(TOP)/build/tag-module.sh $(MODULE) kget $(KGET_OBJ)

#-------------------------------------------------------------------------------
# kpost
#
KPOST_SRC = \
	kpost

KPOST_OBJ = \
	$(addsuffix .$(OBJX),$(KPOST_SRC))

#linking agains vfs automatically brings in kurl (what we need here)
KPOST_LIB = \
	-lkapp \
	-lkurl \
	-lvfs \
	-lkfg \
	-lkrypto \
	-lkfs \
	-lkproc \
	-lklib \
	-lktst

$(BINDIR)/kpost: $(KPOST_OBJ)
	$(LD) --exe $(SRCDIR) -o $@ $^ $(KPOST_LIB)

kpost_tag:
	@ $(TOP)/build/tag-module.sh $(MODULE) kpost $(KPOST_OBJ)
