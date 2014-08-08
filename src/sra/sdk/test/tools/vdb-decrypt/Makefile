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

#
# Not yet in the test:
#   Windows - need to work on the setting of the VDB_PWFILE environment variable
#   within the Makefile. Feature recently added to remote execution
#
#   Using bad passwords and no passwords - need to figure out how to handle
#   failures as successes.
#


default: std

TOP ?= $(abspath ../../..)
MODULE = test/tools/vdb-decrypt

include $(TOP)/build/Makefile.env

INT_TOOLS =

EXT_TOOLS = \

ALL_TOOLS = \
	$(INT_TOOLS) \
	$(EXT_TOOLS)


#TEST_TOOLS = $(BINDIR)/vdb-decrypt


DECRYPT = $(RUN_REMOTELY) $(BINDIR)/vdb-decrypt

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

RWORKDIR = $(TOP)/$(MODULE)

header:
	@ echo ++++++++++++++++++++++++++++++++++++++++++++++++++++++; 
	@ echo Run $(MODULE); 


SETPWD=export VDB_PWFILE=$(RWORKDIR)/password
SETWRONGPWD=VDB_PWFILE=$(RWORKDIR)/wrongpassword
CLRPWD=VDB_PWFILE=

CONTROLDIR=$(RWORKDIR)/CONTROL
SOURCEDIR=$(RWORKDIR)/SOURCE
DESTINATIONDIR=$(RWORKDIR)/DESTINATION




#
# F0 single block text old encrypted extension
# F1 two block text new encrypted extension
# F2 much larger text no encrypted extension
# F3 even  larger text no encrypted extension, also not encrypted
# S0 unencrypted kar file no extension
# S1 encrypted kar file no extension
# S2 encrypted kar file with sra extension
# S3 encrypted kar file with csra extension and new decrypt extension
#

# original version of files.
CTLF0=$(CONTROLDIR)/F0
CTLF1=$(CONTROLDIR)/F1
CTLF2=$(CONTROLDIR)/F2
CTLF3=$(CONTROLDIR)/F3
CTLS0=$(CONTROLDIR)/S0
CTLS1=$(CONTROLDIR)/S1
CTLS2=$(CONTROLDIR)/S2.sra
CTLS3=$(CONTROLDIR)/S3.csra

# source version of files for directory sourced calls
SRCF0=$(SOURCEDIR)/F0.ncbi_enc
SRCF1=$(SOURCEDIR)/F1.nenc
SRCF2=$(SOURCEDIR)/F2
SRCF3=$(SOURCEDIR)/F3
SRCS0=$(SOURCEDIR)/S0
SRCS1=$(SOURCEDIR)/S1
SRCS2=$(SOURCEDIR)/S2.sra
SRCS3=$(SOURCEDIR)/S3.csra.nenc

# mixed encrypted/decrypted version of target names
# the result of directory to directory calls
# the source of in place calls
DSTF0=$(DESTINATIONDIR)/F0.ncbi_enc
DSTF1=$(DESTINATIONDIR)/F1.nenc
DSTF2=$(DESTINATIONDIR)/F2
DSTF3=$(DESTINATIONDIR)/F3
DSTS0=$(DESTINATIONDIR)/S0
DSTS1=$(DESTINATIONDIR)/S1
DSTS2=$(DESTINATIONDIR)/S2.sra
DSTS3=$(DESTINATIONDIR)/S3.csra.nenc

# encrypted version of names
ENCF0=$(DESTINATIONDIR)/F0.ncbi_enc
ENCF1=$(DESTINATIONDIR)/F1.nenc
ENCF2=$(DESTINATIONDIR)/F2.nenc
#ENCF3=$(DESTINATIONDIR)/F3
#ENCS0=$(DESTINATIONDIR)/S0
ENCS1=$(DESTINATIONDIR)/S1.nenc
ENCS2=$(DESTINATIONDIR)/S2.sra.nenc
ENCS3=$(DESTINATIONDIR)/S3.csra.nenc

#decrypted version of names
DECF0=$(DESTINATIONDIR)/F0
DECF1=$(DESTINATIONDIR)/F1
DECF2=$(DESTINATIONDIR)/F2
DECF3=$(DESTINATIONDIR)/F3
DECS0=$(DESTINATIONDIR)/S0
DECS1=$(DESTINATIONDIR)/S1
DECS2=$(DESTINATIONDIR)/S2.sra
DECS3=$(DESTINATIONDIR)/S3.csra

#
# Create original files in CONTROLDIR
# Create a known starting environment in SOURCEDIR
# TARGETDIR is created a new for each test
#
make-test-env:
	@echo make-test-env:
	@-rm -fr $(CONTROLDIR) 2>/dev/null
	@-rm -fr $(SOURCEDIR) 2>/dev/null
	@mkdir $(CONTROLDIR) 2>/dev/null
	@mkdir $(SOURCEDIR) 2>/dev/null
	@mkdir $(SOURCEDIR)/dummy 2>/dev/null
	@# CTLF0
	@echo ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 >$(CTLF0)
	@echo ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 >>$(CTLF0)
	@echo ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 >>$(CTLF0)
	@echo ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 >>$(CTLF0)
	@echo ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 >>$(CTLF0)
	@echo ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 >>$(CTLF0)
	@echo ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 >>$(CTLF0)
	@echo ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 >>$(CTLF0)
	@echo ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 >>$(CTLF0)
	@echo ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 >>$(CTLF0)
	@echo ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 >>$(CTLF0)
	@echo ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 >>$(CTLF0)
	@echo ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 >>$(CTLF0)
	@echo ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 >>$(CTLF0)
	@echo ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 >>$(CTLF0)
	@echo ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 >>$(CTLF0)
	@echo ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 >>$(CTLF0)
	@echo ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 >>$(CTLF0)
	@echo ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 >>$(CTLF0)
	@echo ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 >>$(CTLF0)
	@# CTLF1
	@cat $(CTLF0) $(CTLF0) $(CTLF0) $(CTLF0) $(CTLF0) $(CTLF0) $(CTLF0) $(CTLF0) $(CTLF0) $(CTLF0)  >$(CTLF1)
	@cat $(CTLF0) $(CTLF0) $(CTLF0) $(CTLF0) $(CTLF0) $(CTLF0) $(CTLF0) $(CTLF0) $(CTLF0) $(CTLF0) >>$(CTLF1)
	@cat $(CTLF0) $(CTLF0) $(CTLF0) $(CTLF0) $(CTLF0) $(CTLF0) $(CTLF0) $(CTLF0) $(CTLF0) $(CTLF0) >>$(CTLF1)
	@cat $(CTLF0) $(CTLF0) $(CTLF0) $(CTLF0) $(CTLF0) $(CTLF0) $(CTLF0) $(CTLF0) $(CTLF0) $(CTLF0) >>$(CTLF1)
	@cat $(CTLF0) $(CTLF0) $(CTLF0) $(CTLF0) $(CTLF0) $(CTLF0) $(CTLF0) $(CTLF0) $(CTLF0) $(CTLF0) >>$(CTLF1)
	@# CTLF2
	@cat $(CTLF1) $(CTLF1) $(CTLF1) $(CTLF1) $(CTLF1) $(CTLF1) $(CTLF1) $(CTLF1) $(CTLF1) $(CTLF1)  >$(CTLF2)
	@cat $(CTLF1) $(CTLF1) $(CTLF1) $(CTLF1) $(CTLF1) $(CTLF1) $(CTLF1) $(CTLF1) $(CTLF1) $(CTLF1) >>$(CTLF2)
	@cat $(CTLF1) $(CTLF1) $(CTLF1) $(CTLF1) $(CTLF1) $(CTLF1) $(CTLF1) $(CTLF1) $(CTLF1) $(CTLF1) >>$(CTLF2)
	@cat $(CTLF1) $(CTLF1) $(CTLF1) $(CTLF1) $(CTLF1) $(CTLF1) $(CTLF1) $(CTLF1) $(CTLF1) $(CTLF1) >>$(CTLF2)
	@cat $(CTLF1) $(CTLF1) $(CTLF1) $(CTLF1) $(CTLF1) $(CTLF1) $(CTLF1) $(CTLF1) $(CTLF1) $(CTLF1) >>$(CTLF2)
	@# CLTF3
	@cat $(CTLF2) $(CTLF2) $(CTLF2) $(CTLF2) $(CTLF2) $(CTLF2) $(CTLF2) $(CTLF2) $(CTLF2) $(CTLF2)  >$(CTLF3)
	@cat $(CTLF2) $(CTLF2) $(CTLF2) $(CTLF2) $(CTLF2) $(CTLF2) $(CTLF2) $(CTLF2) $(CTLF2) $(CTLF2) >>$(CTLF3)
	@cat $(CTLF2) $(CTLF2) $(CTLF2) $(CTLF2) $(CTLF2) $(CTLF2) $(CTLF2) $(CTLF2) $(CTLF2) $(CTLF2) >>$(CTLF3)
	@cat $(CTLF2) $(CTLF2) $(CTLF2) $(CTLF2) $(CTLF2) $(CTLF2) $(CTLF2) $(CTLF2) $(CTLF2) $(CTLF2) >>$(CTLF3)
	@cat $(CTLF2) $(CTLF2) $(CTLF2) $(CTLF2) $(CTLF2) $(CTLF2) $(CTLF2) $(CTLF2) $(CTLF2) $(CTLF2) >>$(CTLF3)
	@# SRCS0  made "out of order"
	@kar -d $(CONTROLDIR) -c $(SRCS0) -f
	@# CTLS0
	@cp $(SRCS0) $(CTLS0)
	@# CTLS1
	@cp $(SRCS0) $(CTLS1)
	@# CTLS2
	@cp $(SRCS0) $(CTLS2)
	@# CTLS3
	@cp $(SRCS0) $(CTLS3)
	@# SRCF0
	@nenctool -f $(CTLF0) "ncbi-file:$(SRCF0)?enc&pwfile=password"
	@# SRCF1
	@nenctool -f $(CTLF1) "ncbi-file:$(SRCF1)?enc&pwfile=password"
	@# SRCF2
	@nenctool -f $(CTLF2) "ncbi-file:$(SRCF2)?enc&pwfile=password"
	@# SRCF3
	@cp $(CTLF3) $(SRCF3)
	@# SRCS1
	@nenctool -f $(CTLS1) "ncbi-file:$(SRCS1)?enc&pwfile=password"
	@# SRCS2
	@cp $(SRCS1) $(SRCS2)
	@# SRCS3
	@cp $(SRCS1) $(SRCS3)

file-in-place: make-test-env
	@echo ++++++++++++++++++++++++++++++++++
	@echo file-in-place:
	@-rm -fr $(DESTINATIONDIR)
	@-mkdir $(DESTINATIONDIR)
	@# DSTF0
	@cp $(SRCF0) $(DSTF0)
	@# DSTF1
	@cp $(SRCF1) $(DSTF1)
	@# DSTF2
	@cp $(SRCF2) $(DSTF2)
	@# DSTF3
	@cp $(SRCF3) $(DSTF3)
	@# DSTS0
	@cp $(SRCS0) $(DSTS0)
	@# DSTS1
	@cp $(SRCS1) $(DSTS1)
	@# DSTS2
	@cp $(SRCS2) $(DSTS2)
	@# DSTS3
	@cp $(SRCS3) $(DSTS3)
	@#ls -l $(DESTINATIONDIR)
	@# ENCF0 DECF0
	@$(SETPWD) ; $(DECRYPT) $(DSTF0)
	@# ENCF1 DECF1
	@($(SETPWD) ; $(DECRYPT) $(DSTF1) )
	@# ENCF2 DECF2
	@($(SETPWD) ; $(DECRYPT) $(DSTF2) ; $(CLRPWD))
	@# ENCF3 DECF3
	@($(SETPWD) ; $(DECRYPT) $(DSTF3) ; $(CLRPWD))
	@# ENCS0 DECS0
	@($(SETPWD) ; $(DECRYPT) $(DSTS0) ; $(CLRPWD))
	@# ENCS1 DECS1
	@($(SETPWD) ; $(DECRYPT) $(DSTS1) ; $(CLRPWD))
	@# ENCS2 DECS2
	@($(SETPWD) ; $(DECRYPT) $(DSTS2) ; $(CLRPWD))
	@# ENCS3 DECS3
	@($(SETPWD) ; $(DECRYPT) $(DSTS3) ; $(CLRPWD))
	@#ls -l $(DESTINATIONDIR)
	@#
	@diff $(CTLF0) $(DECF0)
	@diff $(SRCF0) $(ENCF0)
	@#
	@diff $(CTLF1) $(DECF1)
	@diff $(SRCF1) $(ENCF1)
	@#
	@diff $(CTLF2) $(DECF2)
	@diff $(SRCF2) $(ENCF2)
	@#
	@diff $(CTLF3) $(DECF3)
	@#
	@diff $(CTLS0) $(DECS0)
	@#
	@diff $(SRCS1) $(DECS1)
	@#
	@diff $(SRCS2) $(DECS2)
	@#
	@diff $(SRCS3) $(ENCS3)
	@-rm -fr $(DESTINATIONDIR)

directory-in-place: make-test-env
	@echo ++++++++++++++++++++++++++++++++++
	@echo directory-in-place:
	@-rm -fr $(DESTINATIONDIR)
	@-mkdir $(DESTINATIONDIR)
	@# DSTF0
	@cp $(SRCF0) $(DSTF0)
	@# DSTF1
	@cp $(SRCF1) $(DSTF1)
	@# DSTF2
	@cp $(SRCF2) $(DSTF2)
	@# DSTF3
	@cp $(SRCF3) $(DSTF3)
	@# DSTS0
	@cp $(SRCS0) $(DSTS0)
	@# DSTS1
	@cp $(SRCS1) $(DSTS1)
	@# DSTS2
	@cp $(SRCS2) $(DSTS2)
	@# DSTS3
	@cp $(SRCS3) $(DSTS3)
	@#ls -l $(DESTINATIONDIR)
	@# ENCF0-6 DECF0-6
	@($(SETPWD) ;$(DECRYPT) $(DESTINATIONDIR))
	@#ls -l $(DESTINATIONDIR)
	@diff $(CTLF0) $(DECF0)
	@diff $(SRCF0) $(ENCF0)
	@diff $(CTLF1) $(DECF1)
	@diff $(SRCF1) $(ENCF1)
	@diff $(CTLF2) $(DECF2)
	@diff $(SRCF2) $(ENCF2)
	@diff $(CTLF3) $(DECF3)
	@diff $(CTLS0) $(DSTS0)
	@diff $(SRCS1) $(DSTS1)
	@diff $(SRCS2) $(DSTS2)
	@diff $(SRCS3) $(DSTS3)
	@-rm -fr $(DESTINATIONDIR)

file-to-file: make-test-env
	@echo ++++++++++++++++++++++++++++++++++
	@echo file-to-file:
	@-rm -fr $(DESTINATIONDIR)
	@-mkdir $(DESTINATIONDIR)
	@# DSTF0
	@$(SETPWD) ;$(DECRYPT) $(SRCF0) $(DECF0)
	@# DECF1
	@$(SETPWD) ;$(DECRYPT) $(SRCF1) $(DECF1)
	@# DECF2
	@$(SETPWD) ;$(DECRYPT) $(SRCF2) $(DECF2)
	@# DECF3
	@$(SETPWD) ;$(DECRYPT) $(SRCF3) $(DECF3)
	@# DECS0
	@$(SETPWD) ;$(DECRYPT) $(SRCS0) $(DECS0)
	@# DECS1
	@$(SETPWD) ;$(DECRYPT) $(SRCS1) $(DECS1)
	@# DECS2
	@$(SETPWD) ;$(DECRYPT) $(SRCS2) $(DECS2)
	@# DECS3
	@$(SETPWD) ;$(DECRYPT) $(SRCS3) $(DECS3)
	@#ls -l $(DESTINATIONDIR)
	@diff $(CTLF0) $(DECF0)
	@diff $(CTLF1) $(DECF1)
	@diff $(CTLF2) $(DECF2)
	@diff $(CTLF3) $(DECF3)
	@diff $(SRCS0) $(DECS0)
	@diff $(SRCS1) $(DECS1)
	@diff $(SRCS2) $(DECS2)
	@diff $(SRCS3) $(DECS3)
	@-rm -fr $(DESTINATIONDIR)

file-to-directory: make-test-env
	@echo ++++++++++++++++++++++++++++++++++
	@echo file-to-directory:
	@-rm -fr $(DESTINATIONDIR)
	@-mkdir $(DESTINATIONDIR)
	@# DSTF0
	@$(SETPWD) ;$(DECRYPT) $(SRCF0) $(DESTINATIONDIR)
	@# DSTF1
	@$(SETPWD) ;$(DECRYPT) $(SRCF1) $(DESTINATIONDIR)
	@# DSTF2
	@$(SETPWD) ;$(DECRYPT) $(SRCF2) $(DESTINATIONDIR)
	@# DSTF3
	@$(SETPWD) ;$(DECRYPT) $(SRCF3) $(DESTINATIONDIR)
	@# DSTS0
	@$(SETPWD) ;$(DECRYPT) $(SRCS0) $(DESTINATIONDIR)
	@# DSTS1
	@$(SETPWD) ;$(DECRYPT) $(SRCS1) $(DESTINATIONDIR)
	@# DSTS2
	@$(SETPWD) ;$(DECRYPT) $(SRCS2) $(DESTINATIONDIR)
	@# DSTS3
	@$(SETPWD) ;$(DECRYPT) $(SRCS3) $(DESTINATIONDIR)
	@#ls -l $(DESTINATIONDIR)
	@diff $(CTLF0) $(DSTF0)
	@diff $(CTLF1) $(DSTF1)
	@diff $(CTLF2) $(DSTF2)
	@diff $(CTLF3) $(DSTF3)
	@diff $(SRCS0) $(DSTS0)
	@diff $(SRCS1) $(DSTS1)
	@diff $(SRCS2) $(DSTS2)
	@diff $(SRCS3) $(DSTS3)
	@-rm -fr $(DESTINATIONDIR)

#directory-to-directory: make-test-env
#	@echo ++++++++++++++++++++++++++++++++++
#	@echo directory-to-directory:
#	@-rm -fr $(DESTINATIONDIR)
#	@-mkdir $(DESTINATIONDIR)
#	@# DSTF0-6
#	@$(SETPWD) ;$(DECRYPT) $(SOURCEDIR) $(DESTINATIONDIR)
#	@#ls -l $(DESTINATIONDIR)
#	@diff $(CTLF0) $(DSTF0)
#	@diff $(CTLF1) $(DSTF1)
#	@diff $(CTLF2) $(DSTF2)
#	@diff $(CTLF3) $(DSTF3)
#	@diff $(CTLS0) $(DSTS0)
#	@diff $(SRCS1) $(DSTS1)
#	@diff $(SRCS2) $(DSTS2)
#	@diff $(SRCS3) $(DSTS3)
#	@# if the below fails dummy was copied incorrectly
#	@mkdir $(DESTINATIONDIR)/dummy
#	@-rm -fr $(DESTINATIONDIR)

force-flag: make-test-env
	@echo ++++++++++++++++++++++++++++++++++
	@echo force-flag:
	@-rm -fr $(DESTINATIONDIR)
	@$(SETPWD) ;$(DECRYPT) $(SRCF0) --force $(DECF0)
	@echo The next line should have a error that is ignored
	@$(SETPWD) ;$(DECRYPT) $(SRCF0) $(DECF0) && false || true
	@$(SETPWD) ;$(DECRYPT) --force $(SRCF0) $(DECF0)
	@$(SETPWD) ;$(DECRYPT) -f $(SRCF0) $(DECF0)
	@-rm -fr $(DESTINATIONDIR)

sra-flag: make-test-env
	@echo ++++++++++++++++++++++++++++++++++
	@echo sra-flag:
	@-rm -fr $(DESTINATIONDIR)
	@$(SETPWD) ;$(DECRYPT) $(SRCS0) $(DSTS0)
	@$(SETPWD) ;$(DECRYPT) $(SRCS1) $(DSTS1)
	@$(SETPWD) ;$(DECRYPT) $(SRCS2) $(DSTS2)	
	@$(SETPWD) ;$(DECRYPT) $(SRCS3) $(DSTS3)
	@#ls -l $(DESTINATIONDIR)
	@diff $(SRCS0) $(DSTS0)
	@diff $(SRCS1) $(DSTS1)
	@diff $(SRCS2) $(DSTS2)
	@diff $(SRCS3) $(DSTS3)
	@$(SETPWD) ;$(DECRYPT) --decrypt-sra-files $(DSTS0)
	@$(SETPWD) ;$(DECRYPT) --decrypt-sra-files $(DSTS1)
	@$(SETPWD) ;$(DECRYPT) --decrypt-sra-files $(DSTS2)	
	@$(SETPWD) ;$(DECRYPT) --decrypt-sra-files $(DSTS3)
	@#ls -l $(DESTINATIONDIR)
	@diff $(CTLS0) $(DECS0)
	@diff $(CTLS1) $(DECS1)
	@diff $(CTLS2) $(DECS2)
	@diff $(CTLS3) $(DECS3)
	@diff $(SRCS1) $(ENCS1)
	@diff $(SRCS2) $(ENCS2)
	@diff $(SRCS3) $(ENCS3)
	@-rm -fr $(DESTINATIONDIR)

teardown:
	@-rm -fr $(CONTROLDIR) $(SOURCEDIR) $(DESTINATIONDIR)

#ALL_TESTS=file-in-place directory-in-place file-to-file file-to-directory directory-to-directory force-flag sra-flag
#ALL_TESTS=file-to-file file-to-directory directory-to-directory force-flag sra-flag
#ALL_TESTS=file-to-file file-to-file file-to-directory directory-in-place force-flag sra-flag
ALL_TESTS=

ifneq (win,$(OS))
#runtests: std header make-test-env $(ALL_TESTS) teardown
runtests: std header
endif

.PHONY: runtests header make-test-env $(ALL_TESTS) teardown
