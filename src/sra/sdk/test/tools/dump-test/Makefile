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
MODULE = test/tools/dump-test

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
	@ rm -f *.vers.h actual/*-*

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
RWORKDIR = $(abspath .)

header:
	@ echo ++++++++++++++++++++++++++++++++++++++++++++++++++++++; 
	@ echo Run $(MODULE); 

MD5SETUP= @export LD_LIBRARY_PATH=$(LIBDIR);./checkmd5.sh
md5: 
	# dumpers produce output with correct checksums on ncbi-acc:SRR000123:
	$(MD5SETUP) "$(RUN_REMOTELY) $(BINDIR)/abi-dump"      'ncbi-acc:SRR000123' "c9b1d80155a5ae79e3be00befcb2e875" SRR000123_F3.csfasta ; rm -f SRR000123_F3_QV.qual          # md5 generated using "abi-dump.2.1.2 SRR000123"
	$(MD5SETUP) "$(RUN_REMOTELY) $(BINDIR)/fastq-dump"    'ncbi-acc:SRR000123' "48c0210babc8dbcffddc1b54c80a2c40" SRR000123.fastq                                          # md5 generated using "fastq-dump.2.0.6 SRR000123"
	$(MD5SETUP) "$(RUN_REMOTELY) $(BINDIR)/illumina-dump" 'ncbi-acc:SRR000123' "23a25fa8151fcb3a6aa7c5af61bcc34c" SRR000123_0_0001_seq.txt ; rm SRR000123_0_0001_qcal.txt    # md5 generated using "illumina-dump.2.0.6 SRR000123"
	$(MD5SETUP) "$(RUN_REMOTELY) $(BINDIR)/sam-dump"      'ncbi-acc:SRR000123' "68b1a37ad157778940454150fc53f128"                                                          # md5 generated using "sam-dump.3.0.1 SRR000123"
	$(MD5SETUP) "$(RUN_REMOTELY) $(BINDIR)/sff-dump"      'ncbi-acc:SRR000123' "2fcb231391bb920d2fa38f77b0dc1f8c" SRR000123.sff                                            # md5 generated using "sff-dump.2.0.6 SRR000123"
	$(MD5SETUP) "$(RUN_REMOTELY) $(BINDIR)/vdb-dump"      'ncbi-acc:SRR000123' "bf43538cc6e9f1aaa92558f3ffa507b4"                                                          # md5 generated using "vdb-dump.2.1.7 SRR000123"
#	$(MD5SETUP) "$(RUN_REMOTELY) $(BINDIR)/sra-dump"      'ncbi-acc:SRR000123' "b81f8e0d4651f17abf85ab0a300fa7ab"                                                          # md5 generated using "sra-dump.2.0.6 SRR000123"
	# dumpers produce output with correct checksums on SRR000123 through a filesystem path:
	$(MD5SETUP) "$(RUN_REMOTELY) $(BINDIR)/abi-dump"      `$(BINDIR)/srapath SRR000123` "c9b1d80155a5ae79e3be00befcb2e875" SRR000123_F3.csfasta ; rm -f SRR000123_F3_QV.qual          # md5 generated using "abi-dump.2.1.2 SRR000123"
	$(MD5SETUP) "$(RUN_REMOTELY) $(BINDIR)/fastq-dump"    `$(BINDIR)/srapath SRR000123` "48c0210babc8dbcffddc1b54c80a2c40" SRR000123.fastq                                          # md5 generated using "fastq-dump.2.0.6 SRR000123"
	$(MD5SETUP) "$(RUN_REMOTELY) $(BINDIR)/illumina-dump" `$(BINDIR)/srapath SRR000123` "23a25fa8151fcb3a6aa7c5af61bcc34c" SRR000123_0_0001_seq.txt ; rm SRR000123_0_0001_qcal.txt    # md5 generated using "illumina-dump.2.0.6 SRR000123"
	$(MD5SETUP) "$(RUN_REMOTELY) $(BINDIR)/sam-dump"      `$(BINDIR)/srapath SRR000123` "68b1a37ad157778940454150fc53f128"                                                          # md5 generated using "sam-dump.3.0.1 SRR000123"
	$(MD5SETUP) "$(RUN_REMOTELY) $(BINDIR)/sff-dump"      `$(BINDIR)/srapath SRR000123` "2fcb231391bb920d2fa38f77b0dc1f8c" SRR000123.sff                                            # md5 generated using "sff-dump.2.0.6 SRR000123"
	$(MD5SETUP) "$(RUN_REMOTELY) $(BINDIR)/vdb-dump"      `$(BINDIR)/srapath SRR000123` "bf43538cc6e9f1aaa92558f3ffa507b4"                                                          # md5 generated using "vdb-dump.2.1.7 SRR000123"
#	$(MD5SETUP) "$(RUN_REMOTELY) $(BINDIR)/sra-dump"      `$(BINDIR)/srapath SRR000123` "b81f8e0d4651f17abf85ab0a300fa7ab"                                                          # md5 generated using "sra-dump.2.0.6 SRR000123"

# longer decryption tests for dumpers
DECRYPT_ARC='ncbi-file:SRR000123.sra.nenc?enc&pwfile=password-file'
decrypt:
	export LD_LIBRARY_PATH=$(LIBDIR):${LD_LIBRARY_PATH}; $(BINDIR)/nenctool -f `$(BINDIR)/srapath SRR000123` ${DECRYPT_ARC}
	# dumpers produce output with correct checksums on ncbi-file:SRR000123.sra.nenc?enc&pwfile=password-file:
	@rm -f *.kfg;
	$(MD5SETUP) "$(RUN_REMOTELY) $(BINDIR)/vdb-dump"      ${DECRYPT_ARC} "bf43538cc6e9f1aaa92558f3ffa507b4"                                                          # md5 generated using "vdb-dump.2.1.7 SRR000123"
	$(MD5SETUP) "$(RUN_REMOTELY) $(BINDIR)/abi-dump"      ${DECRYPT_ARC} "c9b1d80155a5ae79e3be00befcb2e875" SRR000123_F3.csfasta ; rm -f SRR000123_F3_QV.qual          # md5 generated using "abi-dump.2.1.2 SRR000123"
	$(MD5SETUP) "$(RUN_REMOTELY) $(BINDIR)/fastq-dump"    ${DECRYPT_ARC} "48c0210babc8dbcffddc1b54c80a2c40" SRR000123.fastq                                          # md5 generated using "fastq-dump.2.0.6 SRR000123"
	$(MD5SETUP) "$(RUN_REMOTELY) $(BINDIR)/illumina-dump" ${DECRYPT_ARC} "23a25fa8151fcb3a6aa7c5af61bcc34c" SRR000123_0_0001_seq.txt;rm SRR000123_0_0001_qcal.txt    # md5 generated using "illumina-dump.2.0.6 SRR000123"
	$(MD5SETUP) "$(RUN_REMOTELY) $(BINDIR)/sam-dump"      ${DECRYPT_ARC} "68b1a37ad157778940454150fc53f128"                                                          # md5 generated using "sam-dump.3.0.1 SRR000123"
	$(MD5SETUP) "$(RUN_REMOTELY) $(BINDIR)/sff-dump"      ${DECRYPT_ARC} "2fcb231391bb920d2fa38f77b0dc1f8c" SRR000123.sff                                            # md5 generated using "sff-dump.2.0.6 SRR000123"
#	@#$(MD5SETUP) "$(RUN_REMOTELY) $(BINDIR)/sra-dump       ${DECRYPT_ARC} "b81f8e0d4651f17abf85ab0a300fa7ab"                                                          # md5 generated using "sra-dump.2.0.6 SRR000123"
	@-rm SRR000123.sra.nenc

# short decryption tests checking for failure modes rather than successful decryption
ENCSETUP= export LD_LIBRARY_PATH=$(LIBDIR); unset VDB_CONFIG VDB_PWFILE

encrypt: checkpwdfile
	@#@ echo VDB_CONFIG ${VDB_CONFIG}
	@#@ echo VDB_PWFILE ${VDB_PWFILE}
	# dumpers handle undecryptable files gracefully:    
	@ mkdir -p actual 
	@ # no environment variable; FIXME: now this picks up a config from /etc/ncbi/, which is likely to point at $HOME/.ncbi/pwfile, and these 2 test cases will fail if it exists
	@ $(ENCSETUP); ./runtestcase.sh $(BINDIR)/fastq-dump 1-1 3 "rcEncryptionKey" "rcOpening,rcFile"
	@ $(ENCSETUP); ./runtestcase.sh $(BINDIR)/sam-dump   1-2 3 "rcReading,rcItem" "rcNotFound"
	@ # no config file; FIXME: now this picks up a config from /etc/ncbi/, which is likely to point at $HOME/.ncbi/pwfile, and these 2 test cases will fail if it exists
	@ $(ENCSETUP); export VDB_CONFIG="notthere.kfg"; ./runtestcase.sh $(BINDIR)/fastq-dump 2-1 3 "rcEncryptionKey" "rcOpening,rcFile"
	@ $(ENCSETUP); export VDB_CONFIG="notthere.kfg"; ./runtestcase.sh $(BINDIR)/sam-dump   2-2 3 "rcReading,rcItem" "rcNotFound"
	@ # no password file in config
	@ echo "" >empty.kfg
	@ $(ENCSETUP); export VDB_CONFIG="empty.kfg"; ./runtestcase.sh $(BINDIR)/fastq-dump 3-1 3 "rcEncryptionKey" "rcFile,rcUnknown"
	@ $(ENCSETUP); export VDB_CONFIG="empty.kfg"; ./runtestcase.sh $(BINDIR)/sam-dump   3-2 3 "rcReading,rcItem" "rcNotFound"
	@ rm empty.kfg
	@ # wrong password
	@ $(ENCSETUP); export VDB_PWFILE="badpwd-file"; ./runtestcase.sh $(BINDIR)/fastq-dump 4-1 3 "rcEncryptionKey" "rcEncryption,rcIncorrect"
	@ $(ENCSETUP); export VDB_PWFILE="badpwd-file"; ./runtestcase.sh $(BINDIR)/sam-dump   4-2 3 "rcReading,rcItem" "rcNotFound"
	@ # missing config file
	@ $(ENCSETUP); export VDB_PWFILE="notthere.pwd"; ./runtestcase.sh $(BINDIR)/fastq-dump 5-1 3 "rcEncryptionKey" "rcFile,rcNotFound"
	@ $(ENCSETUP); export VDB_PWFILE="notthere.pwd"; ./runtestcase.sh $(BINDIR)/sam-dump   5-2 3 "rcReading,rcItem" "rcNotFound"

KILLRUN=@ export LD_LIBRARY_PATH=$(LIBDIR);./killtestcase.sh
kill:
	# dumpers do not leave behind an xml when killed:
	@unset VDB_CONFIG VDB_PWDFILE; 
	$(KILLRUN) $(BINDIR)/fastq-dump ; rm -f  SRR331456.fastq
	$(KILLRUN) $(BINDIR)/sam-dump 

split_files_fastq:
	@export LD_LIBRARY_PATH=$(LIBDIR); $(BINDIR)/fastq-dump --no-user-settings --split-files  SRR331456 SRR331457; rm -f SRR331456*.fastq SRR331457*.fastq
    
ifneq (win,$(OS))
runtests: std header md5 decrypt encrypt kill split_files_fastq
endif

.PHONY: runtests header md5 encrypt kill decrypt checkpwdfile split_files_fastq
