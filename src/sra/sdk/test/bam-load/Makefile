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
MODULE = test/bam-load

RUNTESTS_OVERRIDE = 1
include $(TOP)/build/Makefile.env

INT_TOOLS = \

EXT_TOOLS = \

TEST_TOOLS = \

ALL_TOOLS = \
	$(INT_TOOLS) \
	$(EXT_TOOLS) \
	$(TEST_TOOLS)

#-------------------------------------------------------------------------------
# outer targets
#

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

#-------------------------------------------------------------------------------
# currently only running tests on 64 bit linux+mac
ifeq (rwin,$(BUILD_OS))
runtests:
	@ true
else ifeq (32,$(BITS))
runtests:
	@ true
else

#-------------------------------------------------------------------------------
# runtests
#
ifeq (linux,$(BUILD_OS))
    SRA_TEST ?= /panfs/pan1.be-md.ncbi.nlm.nih.gov/sra-test
    #BAM_LOAD_2 ?= /panfs/pan1/sra-test/TOOLKIT/src/sratoolkit.2.2.2/sratoolkit.2.2.2-centos_linux64/bin/bam-load.2
    BAM_LOAD_2 ?= $(BINDIR)/bam-load
    SAMTOOLS ?= $(SRA_TEST)/bin/samtools
else 
ifeq (mac,$(BUILD_OS))
    SRA_TEST ?= /net/pan1/sra-test
    # need a copy of samtools handy
    SAMTOOLS ?= samtools
endif
endif

RWORKDIR = $(TOP)/$(MODULE)

RUN = @ export LD_LIBRARY_PATH=$(LIBDIR);$(RWORKDIR)/runtestcase.sh $(BINDIR) $(SAMTOOLS) $(RWORKDIR)/temp
runtests: std 
	@ echo ++++++++++++++++++++++++++++++++++++++++++++++++++++++; 
	@ echo Run $(MODULE); 
	@ mkdir -p $(RWORKDIR)/actual $(RWORKDIR)/temp
	$(RUN) 1.0		# Paired primary alignment
	$(RUN) 1.0.1	# Paired primary alignment, with flag 0x1 clear (result: spot assembly not done)
	$(RUN) 1.0.2	# Paired primary alignment, with flag 0x2 clear
	@#
	$(RUN) 2.0 0 	# Paired primary alignment plus a secondary alignment
	$(RUN) 2.0.1	# Paired primary alignment plus a secondary alignment, with flag 0x1 clear (result: spot assembly not done)
	$(RUN) 2.0.2	# Paired primary alignment plus a secondary alignment, with flag 0x2 clear
	@#
	$(RUN) 2.1		# Paired primary alignment plus multiple secondary alignments
	$(RUN) 2.1.1	# Paired primary alignment plus multiple secondary alignments, with flag 0x1 clear (result: spot assembly not done)
	$(RUN) 2.1.2	# Paired primary alignment plus multiple secondary alignments, with flag 0x2 clear
	@#
	$(RUN) 2.2		# Paired primary alignment, no matches on the secondary
	$(RUN) 2.2.1	# Paired primary alignment, no matches on the secondary, with flag 0x1 clear (result: spot assembly not done)
	$(RUN) 2.2.2	# Paired primary alignment, no matches on the secondary, with flag 0x2 clear
	@#
	$(RUN) 2.3 	# Paired primary alignment, bad position on the secondary
	$(RUN) 2.3.1 	# Paired primary alignment, bad position on the secondary, with flag 0x1 clear (result: spot assembly not done)
	$(RUN) 2.3.2 	# Paired primary alignment, bad position on the secondary, with flag 0x2 clear
	@#
	$(RUN) 3.0 	# Half aligned pair (one read aligned, one isn't)
	$(RUN) 3.0.1 	# Half aligned pair, with flag 0x1 clear (result: spot assembly not done)
	$(RUN) 3.0.2 	# Half aligned pair, with flag 0x2 clear
	@#
	$(RUN) 3.1		# Half aligned, half poorly aligned (no matches)
	$(RUN) 3.1.1	# Half aligned, half poorly aligned, with flag 0x1 clear (result: spot assembly not done)
	$(RUN) 3.1.2	# Half aligned, half poorly aligned, with flag 0x2 clear
	@#
	$(RUN) 3.2 	# Half aligned, half poorly aligned (bad position)
	$(RUN) 3.2.1 	# Half aligned, half poorly aligned, with flag 0x1 clear (result: spot assembly not done)
	$(RUN) 3.2.2 	# Half aligned, half poorly aligned, with flag 0x2 clear
	@#
	$(RUN) 4.0 	# Unaligned pair
	$(RUN) 4.0.1 	# Unaligned pair, with bit 0x1 clear (result: spot assembly not done)
	$(RUN) 4.0.2 	# Unaligned pair, with flag 0x2 clear
	@#
	$(RUN) 4.1 	# Badly aligned, unaligned pair
	$(RUN) 4.1.1 	# Badly aligned, unaligned pair, with flag 0x1 clear (result: spot assembly not done)
	$(RUN) 4.1.2 	# Badly aligned, unaligned pair, with flag 0x2 clear
	@#
	$(RUN) 5.0 	# Unaligned, unpaired, flag 0x2 set
	$(RUN) 5.0.2 	# Unaligned, unpaired, flag 0x2 clear
	@#
	$(RUN) 5.1 	# Badly aligned, unpaired, flag 0x2 set
	$(RUN) 5.1.2 	# Badly aligned, nunpaired, flag 0x2 clear
	@#
	$(RUN) 6.0 	# Aligned, unpaired
	$(RUN) 6.0.2 	# Aligned, unpaired, flag 0x2 clear
	@#
	$(RUN) 7.0		# Aligned, unpaired with secondary, unpaired
	$(RUN) 7.0.2	# Aligned, unpaired with secondary, unpaired, flag 0x2 clear
	@#
	$(RUN) 8.0 3   # hard clipping - failure
	$(RUN) 8.1 0 --accept-hard-clip		# hard clipping - success
	@#
	$(RUN) 9.0.1 0                 # verify-references
	$(RUN) 9.0.2 0 --no-verify     # no-verify-references
	$(RUN) 9.0.3 0 nodb --only-verify   # only-verify-references (does not create an object)
	@#
	$(RUN) 10.0 0 # half-mated unaligned 
	@#
	$(RUN) 11.0 0 # Inconsistent sequence lengths
	@#
	@# rm -rf actual temp	

.PHONY: runtests

# copy $(RUN) line from above to do one case at a time
onetest: std 
	@ mkdir -p $(RWORKDIR)/actual $(RWORKDIR)/temp
	$(RUN) 10.0 0 # half-mated unaligned 
	@ #rm -rf actual temp	

#-------------------------------------------------------------------------------
# big_loads
#

BAM_TESTDIR=$(SRA_TEST)/bam

BIG_BAMS = \
    HG00121.chrom20.SOLID.bfast.GBR.low_coverage.20101123.bam\
    HG00155.chrom20.ILLUMINA.bwa.GBR.low_coverage.20101123.bam\
    NA06986.chromMT.ILLUMINA.bwa.CEU.exon_targetted.20100311.bam\
    NA06986.nonchrom.ILLUMINA.bwa.CEU.exon_targetted.20100311.bam\
    NA07346.mapped.LS454.ssaha2.CEU.low_coverage.20101123.bam\
    NA12043.chrom20.LS454.ssaha2.CEU.low_coverage.20101123.bam\
    NA12043.unmapped.LS454.ssaha2.CEU.low_coverage.20101123.bam\
    NA12878.chrom11.ILLUMINA.bwa.CEU.high_coverage.20100311.bam\
    NA12878.chrom20.ILLUMINA.bwa.CEU.high_coverage.20100311.bam\
    NA12878.chrom4.SOLID.bfast.CEU.high_coverage.20100125.bam\
    NA12878.nonchrom.SOLID.bfast.CEU.high_coverage.20100125.bam\
    NA12878.unmapped.ILLUMINA.bwa.CEU.exon_targetted.20100311.bam\
    NA12878.unmapped.SOLID.bfast.CEU.high_coverage.20100125.bam\
    NA18147.chrom20.LS454.ssaha2.CHD.exon_targetted.20100311.bam\
    NA18147.unmapped.LS454.ssaha2.CHD.exon_targetted.20100311.bam\
    NA18525.mapped.SOLID.bfast.CHB.low_coverage.20101123.bam\
    NA18525.unmapped.SOLID.bfast.CHB.low_coverage.20101123.bam\
    NA18595.mapped.illumina.mosaik.CHB.exome.20110521.bam\
    NA18612.chrom20.ILLUMINA.bwa.CHB.exon_targetted.20100311.bam\
    NA18628.chromY.LS454.ssaha2.CHB.exon_targetted.20100311.bam\
    NA18628.nonchrom.LS454.ssaha2.CHB.exon_targetted.20100311.bam\
    NA18633.mapped.ILLUMINA.bwa.CHB.low_coverage.20101123.bam\
    NA19172.unmapped.ILLUMINA.bwa.YRI.low_coverage.20101123.bam\
    NA19238.nonchrom.ILLUMINA.bwa.YRI.high_coverage.20100311.bam\
    NA19239.unmapped.ILLUMINA.bwa.YRI.high_coverage.20100311.bam\
    NA19240.chrom20.LS454.ssaha2.YRI.high_coverage.20100311.bam\
    NA19240.chrom20.SOLID.bfast.YRI.high_coverage.20100311.bam\
    NA19240.chromY.LS454.ssaha2.YRI.high_coverage.20100311.bam\
    NA19240.nonchrom.LS454.ssaha2.YRI.high_coverage.20100311.bam\
    NA19240.unmapped.LS454.ssaha2.YRI.high_coverage.20100311.bam\
    NA19764.mapped.SOLID.bfast.MXL.exome.20110411.bam

BAM_CFG=$(BAM_TESTDIR)/analysis.bam.cfg 
BIGWORKDIR = $(SRA_TEST)/$(USER)/bam-load
BAM_LOAD_FLAGS = -d -E 0 --accept-nomatch -k $(BAM_CFG)

# compare vdb-dump outputs of objects created with bam-load.2 vs bam-load.3
$(BIG_BAMS):    
	@ mkdir -p $(BIGWORKDIR)/expected $(BIGWORKDIR)/actual $(BIGWORKDIR)/temp
	# running $@    
	 export LD_LIBRARY_PATH=$(LIBDIR) && \
		cd $(BIGWORKDIR) && \
        $(BAM_LOAD_2) $(BAM_LOAD_FLAGS) $(BAM_TESTDIR)/$@ -o temp/$@  1>expected/$@.stdout 2>expected/$@.stderr && \
		$(BINDIR)/vdb-dump -E temp/$@ 1>expected/$@.vdb-dump.stdout  2>expected/$@.vdb-dump.stderr && \
		$(BINDIR)/vdb-dump    temp/$@ 1>>expected/$@.vdb-dump.stdout 2>>expected/$@.vdb-dump.stderr;
	 export LD_LIBRARY_PATH=$(LIBDIR) && \
		cd $(BIGWORKDIR) && \
		$(BINDIR)/bam-load.3 $(BAM_LOAD_FLAGS) $(BAM_TESTDIR)/$@ -o temp/$@ 1>actual/$@.stdout 2>actual/$@.stderr && \
		$(BINDIR)/vdb-dump -E temp/$@ 1>actual/$@.vdb-dump.stdout  2>actual/$@.vdb-dump.stderr && \
		$(BINDIR)/vdb-dump    temp/$@ 1>>actual/$@.vdb-dump.stdout 2>>actual/$@.vdb-dump.stderr;
	 diff $(BIGWORKDIR)/expected/$@.vdb-dump.stdout $(BIGWORKDIR)/actual/$@.vdb-dump.stdout >$(BIGWORKDIR)/actual/$@.diff
	 rm -rf $(BIGWORKDIR)/*

big_loads: std $(BIG_BAMS)

wbbam:
	$(SAMTOOLS) view -bS wb-test-bam-loader.sam >wb-test-bam-loader.bam

.PHONY: onetest big_loads $(BIG_BAMS)
endif


