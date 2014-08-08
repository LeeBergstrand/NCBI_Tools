# ==============================================================================
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

TOP ?= $(abspath ../../)
MODULE = services/open-server

include $(TOP)/build/Makefile.env
include $(SRCDIR)/Makefile.$(COMP)

ifeq (dbg, $(BUILD))
	NCBIC_LIBS = $(NCBI_ALTLIB)
        # cannot use dbg here, logging functions names conflicting w/SRA
        TRACEDB_LIB = $(TOP)/trace-old/tracedb-x86_64-rel.o
else
	NCBIC_LIBS = $(NCBI_LIBDIR)
        TRACEDB_LIB = $(TOP)/trace-old/tracedb-x86_64-rel.o
endif

DEFINES += -DLINUX_VERSION -DUSE_SOCKETPAIR -DSYB_LP64

ALL_TOOLS = TraceOS

INCDIRS += $(addprefix -I,$(NCBI_INCDIR))

INCDIRS += \
	-I$(NCBI_SYBASE)/include \
	-I$(NCBI)/c++.current/src/internal/third_party/sybase/os \
	-I$(TOP)/trace-old -I$(TOP)/trace-old/inc -I$(TOP)/trace-old/inc/x86_64

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
# TraceOS
#
TRACEOS_OBJ = \
	TraceOS.o \
	TraceOSutils.o \
	tros_srautils.o

TRACEOS_LIB = \
	-ssradb \
	-ssrapath \
	$(ALWAYS_STATIC_SCHEMA_LIBS) \
	-salign-reader \
	-sksrch \
	-svdb \
	-skdb \
	-svfs \
	-skrypto \
	-skfg \
	-skfs-nommap \
	-skproc \
	-sklib \
	$(TRACEDB_LIB) \
	-X$(NCBIC_LIBS) \
	-X$(NCBICPP_LIBS) \
	-X$(NCBI_SYBASE)/lib \
	-lblk_r64 \
	-lct_r64 \
	-lcs_r64 \
	-lsybtcl_r64 \
	-lcomn_r64 \
	-lintl_r64 \
	-ldl \
	-lncbi \
	-lctutils -losutils \
	-lsybase_os \
	-lclog \
	-lpthread \
	-lnsl \
	-lm \
	-lz \
	-lbz2

$(BINDIR)/TraceOS: $(TRACEOS_OBJ)
	$(LD) --exe --vers $(SRCDIR) -o $@ $^ $(TRACEOS_LIB)

TraceOS_tag:
	@ $(TOP)/build/tag-module.sh $(MODULE) TraceOS $(TRACEOS_OBJ)
