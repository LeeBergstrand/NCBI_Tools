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
MODULE = test/tools/nencvalid

INT_TOOLS = \

EXT_TOOLS =

ALL_TOOLS = \
	$(INT_TOOLS) \
	$(EXT_TOOLS)

RUNTESTS_OVERRIDE=1
include $(TOP)/build/Makefile.env

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
	@ rm -f actual/*-*   

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
ifneq (win,$(OS))
SETUP = @ unset VDB_PWFILE VDB_CONFIG; export LD_LIBRARY_PATH=$(LIBDIR) 
EXE="$(RUN_REMOTELY) $(BINDIR)/nencvalid"
RWORKDIR = $(TOP)/$(MODULE)

runtests: std 
	@ echo ++++++++++++++++++++++++++++++++++++++++++++++++++++++; 
	@ echo Run $(MODULE); 
	@ mkdir -p actual 
	$(SETUP); export VDB_CONFIG="./empty.kfg";            ./runtestcase.sh $(EXE) 1-1 test1.wga_enc  0 # 1-1. Valid encrypted file, password not provided
	$(SETUP); export VDB_PWFILE=pwfile;                   ./runtestcase.sh $(EXE) 1-2 test1nomd5     0 # 1-2. Valid file, no checksum in the header 
	$(SETUP); export VDB_PWFILE=pwfile;                   ./runtestcase.sh $(EXE) 1-3 test1.wga_enc  0 # 1-3. Valid encrypted file, correct password from a file ponted to by environment variable VDB_PWFILE 
	$(SETUP); export VDB_PWFILE=pwfilebad;                ./runtestcase.sh $(EXE) 1-4 test1.wga_enc  3 # 1-4. Valid encrypted file, incorrect password from a file ponted to by environment variable VDB_PWFILE 
	$(SETUP); export VDB_PWFILE=pwnotthere;               ./runtestcase.sh $(EXE) 1-5 test1.wga_enc  3 # 1-5. Valid encrypted file, environment variable VDB_PWFILE points to a non-existing file 
	$(SETUP); export VDB_PWFILE=/dev/stdin;echo elcaro |  ./runtestcase.sh $(EXE) 1-6 test1.wga_enc  0 # 1-6. Valid encrypted file, correct password piped from VDB_PWFILE=/dev/stdin 
	$(SETUP); export VDB_PWFILE=/dev/stdin;echo wrong  |  ./runtestcase.sh $(EXE) 1-7 test1.wga_enc  3 # 1-7. Valid encrypted file, incorrect password piped from VDB_PWFILE=/dev/stdin 
	$(SETUP); export VDB_CONFIG="./config.kfg";           ./runtestcase.sh $(EXE) 1-8 test1.wga_enc  0 # 1-8. Valid encrypted file, correct password from a file ponted to by kfg variable krypto/pwfile 
	$(SETUP); export VDB_CONFIG="./configbad.kfg";        ./runtestcase.sh $(EXE) 1-9 test1.wga_enc  3 # 1-9. Valid encrypted file, incorrect password from a file ponted to by kfg variable krypto/pwfile 
	$(SETUP);                                            ./runtestcase.sh  $(EXE) 1-10 config.kfg    3 # 1-10. Invalid file (not a WGA format) 
	$(SETUP); export VDB_PWFILE=pwfile;                   ./runtestcase.sh $(EXE) 1-11 test1badheader 3 #1-11. Invalid file (bad header) 
	$(SETUP); export VDB_PWFILE=pwfile;                   ./runtestcase.sh $(EXE) 1-12 test1trunc    3 # 1-12. Invalid file (truncated: "head -c7263 test1.wga_enc >test1trunc", test1's length=7264);
	$(SETUP); export VDB_PWFILE=pwfile;                 ./runtestcase.sh $(EXE) 1-13 test1doubled  3 # 1-13. Invalid file (excessive length: "cat test1.wga_enc test1.wga_enc >test1doubled")
	$(SETUP); export VDB_PWFILE=pwfile;                   ./runtestcase.sh $(EXE) 1-14 test1badmd5   3 # 1-14. Invalid file (checksum present, mismatched) 
	$(SETUP); export VDB_PWFILE=pwfile;                   ./runtestcase.sh $(EXE) 1-15 test1.wga_enc 0 -q # 1-15. Validation passed, quiet mode
	# compatibility mode                                                  
	$(SETUP); export VDB_CONFIG="./empty.kfg";            ./runtestcase.sh $(EXE) 2-1 test1.wga_enc  105 --compatibility-mode # 2-1. Valid encrypted file, password not provided
	$(SETUP); export VDB_PWFILE=pwfile;                   ./runtestcase.sh $(EXE) 2-2 test1nomd5     104 --compatibility-mode # 2-2. Valid file, no checksum in the header 
	$(SETUP); export VDB_PWFILE=pwfile;                   ./runtestcase.sh $(EXE) 2-3 test1.wga_enc  0 --compatibility-mode # 2-3. Valid encrypted file, correct password from a file ponted to by environment variable VDB_PWFILE 
	$(SETUP); export VDB_PWFILE=pwfilebad;                ./runtestcase.sh $(EXE) 2-4 test1.wga_enc  106 --compatibility-mode # 2-4. Valid encrypted file, incorrect password from a file ponted to by environment variable VDB_PWFILE 
	$(SETUP); export VDB_PWFILE=pwnotthere;               ./runtestcase.sh $(EXE) 2-5 test1.wga_enc  3 --compatibility-mode # 2-5. Valid encrypted file, environment variable VDB_PWFILE points to a non-existing file 
	$(SETUP); export VDB_PWFILE=/dev/stdin;echo elcaro |  ./runtestcase.sh $(EXE) 2-6 test1.wga_enc  0 --compatibility-mode # 2-6. Valid encrypted file, correct password piped from VDB_PWFILE=/dev/stdin 
	$(SETUP); export VDB_PWFILE=/dev/stdin;echo wrong  |  ./runtestcase.sh $(EXE) 2-7 test1.wga_enc  106 --compatibility-mode # 2-7. Valid encrypted file, incorrect password piped from VDB_PWFILE=/dev/stdin 
	$(SETUP); export VDB_CONFIG="./config.kfg";           ./runtestcase.sh $(EXE) 2-8 test1.wga_enc  0 --compatibility-mode # 2-8. Valid encrypted file, correct password from a file ponted to by kfg variable krypto/pwfile 
	$(SETUP); export VDB_CONFIG="./configbad.kfg";        ./runtestcase.sh $(EXE) 2-9 test1.wga_enc  106 --compatibility-mode # 2-9. Valid encrypted file, incorrect password from a file ponted to by kfg variable krypto/pwfile 
	$(SETUP);                                             ./runtestcase.sh $(EXE) 2-10 config.kfg    100 --compatibility-mode # 2-10. Invalid file (not a WGA format) 
	$(SETUP); export VDB_PWFILE=pwfile;                   ./runtestcase.sh $(EXE) 2-11 test1badheader 100 --compatibility-mode #2-11. Invalid file (bad header) 
	$(SETUP); export VDB_PWFILE=pwfile;                   ./runtestcase.sh $(EXE) 2-12 test1trunc    102 --compatibility-mode # 2-12. Invalid file (truncated: "head -c7263 test1.wga_enc >test1trunc", test1's length=7264);
	$(SETUP); export VDB_PWFILE=pwfile;                 ./runtestcase.sh $(EXE) 2-13 test1doubled  103 --compatibility-mode # 2-13. Invalid file (excessive length: "cat test1.wga_enc test1.wga_enc >test1doubled")
	$(SETUP); export VDB_PWFILE=pwfile;                   ./runtestcase.sh $(EXE) 2-14 test1badmd5   106 --compatibility-mode # 2-14. Invalid file (checksum present, mismatched) 
	$(SETUP); export VDB_PWFILE=pwfile;                   ./runtestcase.sh $(EXE) 2-15 test1.wga_enc 0 -q --compatibility-mode # 2-15. Validation passed, quiet mode
else
runtests: std 

endif
