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
MODULE = test/tools/sra-pileup

include $(TOP)/build/Makefile.env

INT_TOOLS =

EXT_TOOLS = \

ALL_TOOLS = \
	$(INT_TOOLS) \
	$(EXT_TOOLS)

#SAMTOOLS="/panfs/pan1/sra-test/bin/samtools"
#PILEUP =  "$(SAMTOOLS) mpileup"
PILEUP = "$(RUN_REMOTELY) $(BINDIR)/sra-pileup"

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
	@ rm -f *.vers.h actual/?-?

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
RWORKDIR = $(TOP)/$(MODULE)
RUN = @ export LD_LIBRARY_PATH=$(LIBDIR);./runtestcase.sh $(PILEUP)
smalltests: std
	@ echo ++++++++++++++++++++++++++++++++++++++++++++++++++++++; 
	@ echo Run $(MODULE); 
	@ mkdir -p actual 
	$(RUN) 1-1  test.csra 0 -r MT:100-200         # no placements in the reference window (RW)
	$(RUN) 1-2  test.csra 0 -r MT:600-1100        # 1 placement fully inside the RW, containing matches and mismatches
	$(RUN) 1-21 test.csra 0 -r MT:5000-5040       # 1 placement fully inside the RW, containing an insert
	$(RUN) 1-22 test.csra 0 -r MT:5040-5080       # 1 placement fully inside the RW, containing a delete
	$(RUN) 1-3  test.csra 0 -r MT:600-5040        # 2 placements fully inside RW, not overlapping with each other, all fully inside RW
	$(RUN) 1-4  test.csra 0 -r MT:900-1000        # 1 placement, starts outside RW
	$(RUN) 1-5  test.csra 0 -r MT:800-900         # 1 placement, ends outside RW
	$(RUN) 1-6  test.csra 0 -r MT:900-920         # 1 placement, starts and ends outside RW
	$(RUN) 1-7  test.csra 0 -r MT:13000-1000     # RW wraps around the end of a circular reference
	$(RUN) 2-2  test.csra 0 -r MT:9300-9400       # 2 placements overlap
	$(RUN) 2-3  test.csra 0 -r MT:10000-11000     # 3 placements overlap
	$(RUN) 2-4  test.csra 0 -r MT:13000-14000     # 3 placements overlap, some start at the same pos within RW, some end at the same pos within RW
	$(RUN) 3-1  test.csra 0 -r MT:0-1000          # RW starts at 0
	$(RUN) 3-2  test.csra 0 -r MT:1-1000          # RW starts at the beginning of the reference
	$(RUN) 3-3  test.csra 0 -r MT:1-16573         # RW ends at the end of the reference
	$(RUN) 3-4  test.csra 0 -r MT:1-1000000       # RW ends beyond the end of the reference
	$(RUN) 3-5  test.csra 0 -r MT:1000000-1100000 # RW starts beyond the end of the reference ( no problem any more, wraps around )
	$(RUN) 3-6  test.csra 0 -r MT:896-896         # RW size is 1
	$(RUN) 3-7  test.csra 0 -r MT:600-1100 -r MT:5000-5040    # 2 RWs specified, no overlap, both contain placements
	$(RUN) 3-8  test.csra 0 -r MT:5031-5066 -r MT:5035-5071   # 2 overlapping RWs specified, the overlapping part contains a placement
	$(RUN) 3-9   "testA.csra testB.csra" 0 -r MT:5000-5040    # 2 input files are specified, only one contains placements in the RW
	$(RUN) 3-10  "testA.csra testB.csra" 0 -r MT:5000-5080    # 2 input files are specified, both contain placements in the RW, no overlap
	$(RUN) 3-11  "test.csra testA.csra"  0 -r MT:600-1100 -r MT:5000-5040         # 2 input files are specified, both contain overlapping placements in the RW
	$(RUN) 3-12  "testA.csra testA.csra" 0 -r MT:5000-5080    # same input file specified twice
	$(RUN) 4-1  qq            3 -r MT:896-900 # Input file (table, database?) does not exist
	$(RUN) 4-2  test.bam.bai  3 -r MT:896-900 # Input file is not in a recognized format
	$(RUN) 4-3  test.csra     0 -r MT:896-800 # RW size is negative (error message, empty output?) - need a non-circular reference

onetest: 
	$(RUN) 3-5  test.csra 0 -r MT:1000000-1100000 # RW starts beyond the end of the reference ( no problem any more, wraps around )
    
runtests: smalltests
    
.PHONY: smalltests onetest
