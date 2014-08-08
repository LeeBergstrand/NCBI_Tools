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

TOP ?= $(abspath ../../..)
MODULE = test/tools/fastq-loader

TEST_TOOLS = \
	test-fastq-loader 

ALL_TOOLS = \

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

.PHONY: clean

#-------------------------------------------------------------------------------
# tag
#
tag: \
	$(addsuffix _tag,$(ALL_TOOLS))

.PHONY: tag $(addsuffix _tag,$(ALL_TOOLS))

#-------------------------------------------------------------------------------
# valgrind
#
valgrind: $(TEST_TOOLS)
	@echo ++++++++++++++++++++++++++++++++++++++++++++++++++++++
	@echo Run $(TEST_BINDIR)/$(TEST_TOOLS)
	valgrind $(RUN_REMOTELY) $(TEST_BINDIR)/$(TEST_TOOLS)

#-------------------------------------------------------------------------------
# fastqtest-loader
#
FASTQTEST_LOADER_SRC = \
	test-fastq-loader 

FASTQTEST_LOADER_OBJ = \
	$(addsuffix .$(OBJX),$(FASTQTEST_LOADER_SRC))

FASTQTEST_LOADER_LIB = \
    -dwvxf \
	-dwaxf \
	-dwsraxf \
	-lksrch \
	-sfastqloader \
    -sloader \
    -salign-writer \
	-skapp \
	-sktst \
    -sload \
    -swvdb \
    -swkdb \
    -svfs \
    -skfg \
    -skrypto \
    -skq \
    -skproc \
    -skfs \
	-sklib
    

$(TEST_BINDIR)/test-fastq-loader: $(FASTQTEST_LOADER_OBJ)
	$(LP) --exe -o $@ $^ $(FASTQTEST_LOADER_LIB)

#-------------------------------------------------------------------------------
# big_loads
#
ifneq (win,$(BUILD_OS))
FASTQTEST_DIR=/panfs/pan1.be-md.ncbi.nlm.nih.gov/trace_work/backup/qa/cases/load

FASTQTEST_DATA_TAR=\
    $(FASTQTEST_DIR)/illFastq/DRR000093/081222_30LMBAAXX_s_1_sequence.txt.tar\
    $(FASTQTEST_DIR)/illFastq/DRR000093/081222_30LMBAAXX_s_2_sequence.txt.tar\
    $(FASTQTEST_DIR)/illFastq/DRR000093/081222_30LMBAAXX_s_3_sequence.txt.tar\
    $(FASTQTEST_DIR)/illFastq/SRR014056/KN-930_1.tar\
    $(FASTQTEST_DIR)/illFastq/SRR014056/KN-930_2.tar\
    $(FASTQTEST_DIR)/illFastq/SRR014056/KN-930_3.tar\
    $(FASTQTEST_DIR)/illFastq/SRR025104/KN-930_1.tar\
    $(FASTQTEST_DIR)/illFastq/SRR029194/30KY8AAXX01.tar\
    $(FASTQTEST_DIR)/illFastq/XXX001656/CBCB-Pseudomonas_aeruginosa_PAb1-s_1.fastq.tar\

SRA_TEST ?= /panfs/pan1.be-md.ncbi.nlm.nih.gov/sra-test

# compare vdb-dump outputs of objects created with fastq-load.2 vs fastq-load.3
RUN = @$(SRCDIR)/fastqcompare.sh $(BINDIR) $(SRA_TEST)/$(USER)/fastq-load

big_load: std
#	$(RUN) /panfs/traces01.be-md.ncbi.nlm.nih.gov/sra_review/static/bug_reports/SRR516178.blob_corrupt:run_flush_thread:run_trigger_prods_failed
	$(RUN) $(FASTQTEST_DIR)/illFastq/SRR020795 -p ILLUMINA -q PHRED_33  SRR020795_orig.fastq

big_loads: std
	$(RUN) $(FASTQTEST_DIR)/illFastq/SRR015261 -p ILLUMINA -q PHRED_64  SRR015261_orig.fastq \
                                                                        brain_s_7_sequence.fastq
	$(RUN) $(FASTQTEST_DIR)/illFastq/SRR020188 -p ILLUMINA -q PHRED_64  R4_s_1_1_sequence.txt\
                                                                        R4_s_1_2_sequence.txt\
                                                                        R4_s_3_1_sequence.txt\
                                                                        R4_s_3_2_sequence.txt\
                                                                        R4_s_5_1_sequence.txt\
                                                                        R4_s_5_2_sequence.txt\
                                                                        R4_s_6_1_sequence.txt\
                                                                        R4_s_6_2_sequence.txt\
                                                                        R4_s_7_1_sequence.txt\
                                                                        R4_s_7_2_sequence.txt\
                                                                        R4_s_8_1_sequence.txt\
                                                                        R4_s_8_2_sequence.txt
	$(RUN) $(FASTQTEST_DIR)/illFastq/SRR020795 -p ILLUMINA -q PHRED_33  SRR020795_orig.fastq
	$(RUN) $(FASTQTEST_DIR)/illFastq/SRR029167 -p ILLUMINA -q PHRED_33  SPM4_filter.txt
	$(RUN) $(FASTQTEST_DIR)/illFastq/SRR030833 -p ILLUMINA -q PHRED_33  090626_I360_FC42AUBAAXX_L6_HUMefdRAH.fq
	$(RUN) $(FASTQTEST_DIR)/illFastq/SRR036999/-p ILLUMINA -q PHRED_33  irahall_unknown_1_F.fastq \
                                                                        irahall_unknown_1_R.fastq
	$(RUN) $(FASTQTEST_DIR)/illFastq/SRR037749 -p ILLUMINA -q PHRED_64  FA0001_42HY7AAXX_1_1.fq \
                                                                        FA0001_42HY7AAXX_1_2.fq
	$(RUN) $(FASTQTEST_DIR)/illFastq/SRR058403 -p ILLUMINA -q PHRED_64  2x_090127_SNPSTER3_0264_30JGGAAXX_PE_2_1.txt \
                                                                        2x_090127_SNPSTER3_0264_30JGGAAXX_PE_2_2.txt
	$(RUN) $(FASTQTEST_DIR)/illFastq/SRR065453 -p ILLUMINA -q PHRED_64  s_7_1.fsq
	$(RUN) $(FASTQTEST_DIR)/illFastq/SRR089777 -p ILLUMINA -q PHRED_64  s_8_1_sequence1.txt\
                                                                        s_8_1_sequence2.txt\
                                                                        s_8_2_sequence1.txt\
                                                                        s_8_2_sequence2.txt
	$(RUN) $(FASTQTEST_DIR)/illFastq/SRR094419 -p ILLUMINA -q PHRED_33  I3kT.741.6.920.fq \
                                                                        I3kT.741.7.920.fq
	$(RUN) $(FASTQTEST_DIR)/illFastq/SRR171628 -p ILLUMINA -q PHRED_64  LS44-PE-ILI3-01_709M9AAXX_8_1.fastq \
                                                                        LS44-PE-ILI3-01_709M9AAXX_8_2.fastq
	$(RUN) $(FASTQTEST_DIR)/illFastq/XXX000001 -p ILLUMINA -q PHRED_33  XXX000001_orig.fastq
	$(RUN) $(FASTQTEST_DIR)/illFastq/XXX000002 -p ILLUMINA -q PHRED_33  XXX000002_orig_1.fastq \
                                                                        XXX000002_orig_2.fastq
	$(RUN) $(FASTQTEST_DIR)/illFastq/XXX000003 -p ILLUMINA -q PHRED_33  XXX000003_orig_1.fastq \
                                                                        XXX000003_orig_2.fastq
	$(RUN) $(FASTQTEST_DIR)/illFastq/XXX000100 -p ILLUMINA -q PHRED_33  DCK1.rat.pineal.polyA.PE1 \
                                                                        DCK1.rat.pineal.polyA.PE2
	$(RUN) $(FASTQTEST_DIR)/illFastq/XXX013547 -p ILLUMINA -q PHRED_33  GSM343832.fastq
	$(RUN) $(FASTQTEST_DIR)/454Fastq/SRR307780 -p LS454    -q PHRED_64  LOA_BAC_454_2.fq
	#create Experiment and Run.xml! $(RUN) /panfs/pan1.be-md.ncbi.nlm.nih.gov/sra-test/LOADER/fastq/ -p PACBIO salmon.filtered.PacBio.fastq 
    
tars: $(TEST_TOOLS)
	@for i in $(FASTQTEST_DATA_TAR); \
	do\
    	mkdir -p tmp; \
        echo $$i; \
	    tar -vxf $$i -C tmp/ >/dev/null; \
	    cd tmp; \
	    nice --adjustment=19 $(TEST_BINDIR)/test-fastq-loader $$(ls -x) ;\
        cd ..; rm -rf files tmp; \
	done
    
.PHONY: test-fastq-loader

SMALLRUN = @$(SRCDIR)/runtestcase.sh $(BINDIR) $(SRCDIR)
smalltests:
	-rm -rf $(SRCDIR)/actual
#   Small tests
#   Input:
#       Quality encoding: phred ascii/decimal
#           1.1 phred 33 
	$(SMALLRUN) 1.1 0 $(SRCDIR)/input/1.1.fastq -q PHRED_33
#           1.2 phred 64 
	$(SMALLRUN) 1.2 0 $(SRCDIR)/input/1.2.fastq -q PHRED_64
#           1.3 phred 33, decimal qualities
	$(SMALLRUN) 1.3 3 $(SRCDIR)/input/1.3.fastq -q PHRED_33
#       
#       Single input file
#           2.1 reads only, basespace
	$(SMALLRUN) 2.1 0 $(SRCDIR)/input/2.1.fastq -q PHRED_33
#           2.1.1 reads only, colorspace
	$(SMALLRUN) 2.1.1 0 $(SRCDIR)/input/2.1.1.fastq -q PHRED_33
#           2.2 qualities only
	-$(SMALLRUN) 2.2 0 $(SRCDIR)/input/2.2.fastq -q PHRED_33
#           2.3 reads+qualities, qualities before reads
	-$(SMALLRUN) 2.3 0 $(SRCDIR)/input/2.3.fastq -q PHRED_33
#           2.4 reads+qualities, reads before qualities
	-$(SMALLRUN) 2.4 0 $(SRCDIR)/input/2.4.fastq -q PHRED_33
#           2.5 reads+qualities, paired up
	$(SMALLRUN) 2.5 0 $(SRCDIR)/input/2.5.fastq -q PHRED_33
#           2.6 spot assembly
	$(SMALLRUN) 2.6 0 $(SRCDIR)/input/2.6.fastq -q PHRED_33
#           2.7 spot assembly, read numbers not specified
	$(SMALLRUN) 2.7 0 $(SRCDIR)/input/2.7.fastq -q PHRED_33
#           2.8 handling of N in reads, ALTREAD is a static column
#           TODO; investigate intermittent failure (FIXED_SPOT_LEN == 0, not 3)
	-$(SMALLRUN) 2.8 0 $(SRCDIR)/input/2.8.fastq -q PHRED_33
#           2.8.1 handling of N in reads, ALTREAD is a physical column
	$(SMALLRUN) 2.8.1 0 $(SRCDIR)/input/2.8.1.fastq -q PHRED_33
#
#       Multiple files
#           3.1 reads+qualities together
	$(SMALLRUN) 3.1 0 $(SRCDIR)/input/3.1a.fastq $(SRCDIR)/input/3.1b.fastq -q PHRED_33
#           3.2 qualities separately from reads, reads file comes first
	-$(SMALLRUN) 3.2 0 $(SRCDIR)/input/3.2a.fastq $(SRCDIR)/input/3.2b.fastq -q PHRED_33
#           3.3 qualities separately from reads, qualities file comes first (reject?)
	-$(SMALLRUN) 3.3 0 $(SRCDIR)/input/3.3a.fastq $(SRCDIR)/input/3.3b.fastq -q PHRED_33
#           3.4 default read number 
#	$(SMALLRUN) 3.4 0 -r 1 $(SRCDIR)/input/3.4a.fastq -r 2 $(SRCDIR)/input/3.4b.fastq -q PHRED_33
#
#   Command line options:
#           4.1 --tmpfs             temporary files created at the specified location
	@echo "running 4.1"; export LD_LIBRARY_PATH=$(LIBDIR);$(BINDIR)/latf-load $(SRCDIR)/input/0.0.fastq --tmpfs $(SRCDIR)/actual/ -v -o actual/4.1.obj -q PHRED_33 | grep "Path for scratch files: $(SRCDIR)/actual/" >/dev/null
#           4.2 --qual-quant        quality values are quantized as specified
	$(SMALLRUN) 4.2 0 $(SRCDIR)/input/0.0.fastq --qual-quant 1:10,10:20,20:30,30:- -q PHRED_33
#           4.3 --cache-size        handled correctly
	@echo "running 4.3"; export LD_LIBRARY_PATH=$(LIBDIR);$(BINDIR)/latf-load $(SRCDIR)/input/0.0.fastq --cache-size 10 -v -o actual/4.1.obj -q PHRED_33 | grep "Cache size: 10M" >/dev/null
#           4.3.1 --cache-size      rejected if invalid
	$(SMALLRUN) 4.3.1 3 $(SRCDIR)/input/0.0.fastq --cache-size BIG -q PHRED_33
#           4.4 --max-rec-count     stops at the specified record #
	$(SMALLRUN) 4.4 0 $(SRCDIR)/input/4.4.fastq --max-rec-count 3 -q PHRED_33
#           4.5 --max-err-count     stops after the specified # of errors
	$(SMALLRUN) 4.5 3 $(SRCDIR)/input/4.5.fastq --max-err-count 2 -q PHRED_33
#           4.6 --platform          saves correct value in the PLATFORM column 
	$(SMALLRUN) 4.6 0 $(SRCDIR)/input/0.0.fastq -p LS454 -q PHRED_33
#           4.6.1 --platform        rejects invalid values
	$(SMALLRUN) 4.6.1 3 $(SRCDIR)/input/0.0.fastq -p SOMETHINGELSE -q PHRED_33
#           4.7 input file missing
	$(SMALLRUN) 4.7 3 $(SRCDIR)/input/not_there -q PHRED_33

#   Gzipped input
	$(SMALLRUN) 5.0 0 $(SRCDIR)/input/5.0.fastq.gz -q PHRED_33

#   Misparsed quality
	$(SMALLRUN) 6.0 0 $(SRCDIR)/input/6.0.fastq -q PHRED_33
    
onetest:
	-#rm -rf $(SRCDIR)/actual
	$(SMALLRUN) 4.5 3 $(SRCDIR)/input/4.5.fastq --max-err-count 2 -q PHRED_33
    
oldload:
	-#rm -rf $(SRCDIR)/actual
	$(BINDIR)/fastq-load -e exp.xml -r run.xml -o old 
    
.PHONY: big_loads 

runtests: smalltests

endif


