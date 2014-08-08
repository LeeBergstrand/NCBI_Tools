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
MODULE = test/proxy_srv

include $(TOP)/build/Makefile.env

INT_TOOLS = \

EXT_TOOLS = \
	proxy-srv

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
tag: $(addsuffix _tag,$(ALL_TOOLS))

.PHONY: tag $(addsuffix _tag,$(ALL_TOOLS))

#-------------------------------------------------------------------------------
# proxy-srv
#
PROXY_SRV_SRC = \
	proxy-srv

PROXY_SRV_OBJ = \
	$(addsuffix .$(OBJX),$(PROXY_SRV_SRC))

PROXY_SRV_LIB = \
	-lkapp \
	-lvfs \
	-lkfg \
	-lkrypto \
	-lkfs \
	-lkproc \
	-lklib \
	-lm \
	-ldl \
	-lws2

$(BINDIR)/proxy-srv: $(PROXY_SRV_OBJ)
ifeq (win,$(OS))
	$(LD) --exe $(SRCDIR) -o $@ $^ $(PROXY_SRV_LIB)
else
	@ echo "not building proxy-srv on "$(OS)
endif

proxy-exec_tag:
	@ $(TOP)/build/tag-module.sh $(MODULE) proxy-srv $(PROXY_SRV_OBJ)
