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

TOP ?= $(abspath ../../..)
MODULE = test/tools/srapath

RUNTESTS_OVERRIDE = 1
include $(TOP)/build/Makefile.env

INT_TOOLS =

EXT_TOOLS = \

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
# runtests
#
TEST_KFG_DIR = $(TOP)/test/vfs/
RUN = @export VDB_CONFIG=$(TEST_KFG_DIR)/resolver.kfg; export LD_LIBRARY_PATH=$(LIBDIR); cd $(TEST_KFG_DIR); $(TOP)/$(MODULE)/runtestcase.sh

ifeq (mac,$(BUILD_OS))
    NET = /net
else
    NET = /panfs
endif

runtests: std 
	$(RUN) "$(NET)/traces01.be-md.ncbi.nlm.nih.gov/refseq/AE016830.1"              "$$($(BINDIR)/srapath AE016830.1)" 
	$(RUN) "x-ncbi-legrefseq:/panfs/traces01.be-md.ncbi.nlm.nih.gov/refseq/AAAB01#tbl/AAAB01000001.1"              "$$($(BINDIR)/srapath AAAB01000001.1)" 
	$(RUN) "$(TEST_KFG_DIR)/ncbi/public/sra/SRR999998"      "$$($(BINDIR)/srapath SRR999998)" 
	$(RUN) "$(TEST_KFG_DIR)/ncbi/public/refseq/ZZ999999.9"  "$$($(BINDIR)/srapath ZZ999999.9)" 
	-$(RUN) "$(TEST_KFG_DIR)/ncbi/public/wgs/YYZZ99.9"       "$$($(BINDIR)/srapath YYZZ99.9)" 
	-$(RUN) "$(TEST_KFG_DIR)/ncbi/protected1/SRR999997"      "$$($(BINDIR)/srapath SRR999997)" 
	-$(RUN) "$(TEST_KFG_DIR)/ncbi/protected2/sra/SRR999996"  "$$($(BINDIR)/srapath SRR999996)" 
	$(RUN) "/netmnt/traces04/sra0/SRR/000004/SRR005020"     "$$($(BINDIR)/srapath SRR005020)" 
	-$(RUN) "$(NET)/traces01/wgs01/WGS/AB/AB/ABAB01.1"       "$$($(BINDIR)/srapath ABAB01.1)" 
    
    
    
    
    
    
    
