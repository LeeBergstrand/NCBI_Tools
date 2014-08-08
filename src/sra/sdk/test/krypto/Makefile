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
MODULE = test/krypto

include $(TOP)/build/Makefile.env
RWORKDIR = $(TOP)/$(MODULE)

INT_TOOLS = \
	test-modes \
	test-block-cross-error

ifeq (linux,$(OS))
INT_TOOLS += \
	test-aes-ciphers \
	test-cipher-speed
endif


#	test-parse

#	test-aesavs

# \
#	test-wga-enc \
#	test-encfile

#	test-wga-enc-write \
#	test-csprng \


TEST_TOOLS = \
	test-encv2 \
	test-encapptrunc \
	testreenc \
	testencrypt

EXT_TOOLS =

ALL_TOOLS = \
	$(INT_TOOLS) \
	$(EXT_TOOLS) \
	$(TEST_TOOLS)

#-------------------------------------------------------------------------------
# outer targets
#
all std: makedirs
	@ $(MAKE_CMD) $(TARGDIR)/std

$(ALL_TOOLS): makedirs
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





#============


TEST_PARSE_SRC = \
	test-parse \
	test-parse-lex \
	test-parse-bison

$(SRCDIR)/test-parse-lex.c : $(SRCDIR)/test-parse-lex.l $(SRCDIR)/test-parse-priv.h

$(SRCDIR)/test-parse-bison.c $(SRCDIR)/test-parse-tokens.h: $(SRCDIR)/test-parse-bison.y $(SRCDIR)/test-parse-priv.h
	$(YACC) -o $(SRCDIR)/test-parse-bison.c --defines=$(SRCDIR)/test-parse-tokens.h $<

TEST_PARSE_OBJ = \
	$(addsuffix .$(OBJX),$(TEST_PARSE_SRC))

TEST_PARSE_LIB = \
	-lkapp \
	-lkfs \
	-lklib

$(TEST_BINDIR)/test-parse: $(TEST_PARSE_OBJ)
	$(LD) --exe -o $@ $^ $(TEST_PARSE_LIB)




#-------------------------------------------------------------------------------
# test encryption modes
#
TEST_MODES_SRC = \
	test-modes

TEST_MODES_OBJ = \
	$(addsuffix .$(OBJX),$(TEST_MODES_SRC))

TEST_MODES_LIB = \
	-lkapp \
	-lvfs \
	-lkfg \
	-lkrypto \
	-lkproc \
	-lkfs \
	-lklib \
	-lz \
	-lbz2

$(TEST_BINDIR)/test-modes: $(TEST_MODES_OBJ)
	$(LD) --exe -o $@ $^ $(TEST_MODES_LIB)

#-------------------------------------------------------------------------------
# test AES encryption ciphers
#
TEST_AES_CIPHERS_SRC = \
	test-aes-ciphers

TEST_AES_CIPHERS_OBJ = \
	$(addsuffix .$(OBJX),$(TEST_AES_CIPHERS_SRC))

TEST_AES_CIPHERS_LIB = \
	-lkapp \
	-lvfs \
	-lkfg \
	-lkryptotest \
	-lkproc \
	-lkfs \
	-lklib \
	-lz \
	-lbz2

$(TEST_BINDIR)/test-aes-ciphers: $(TEST_AES_CIPHERS_OBJ)
	$(LD) --exe -o $@ $^ $(TEST_AES_CIPHERS_LIB)

#-------------------------------------------------------------------------------
# test cipher speed
# run a null cipher and the aes cipher in all implmentations supported on the
# test machine.
# Comparing specific two versions will test possible speed gains from verious 
# optimizations.  Comparing the null cipher (just a copy) versions against each
# other offer insight into whether vector and vector register optimiztions are
# beneficial and by how much
#
TEST_CIPHER_SPEED_SRC = \
	test-cipher-speed

TEST_CIPHER_SPEED_OBJ = \
	$(addsuffix .$(OBJX),$(TEST_CIPHER_SPEED_SRC))

TEST_CIPHER_SPEED_LIB = \
	-lkapp \
	-lvfs \
	-lkfg \
	-lkryptotest \
	-lkproc \
	-lkfs \
	-lklib \
	-lz \
	-lbz2

$(TEST_BINDIR)/test-cipher-speed: $(TEST_CIPHER_SPEED_OBJ)
	$(LD) --exe -o $@ $^ $(TEST_CIPHER_SPEED_LIB)

#-------------------------------------------------------------------------------
# test AES encryption ciphers
#
TEST_AESSAVS_SRC = \
	test-aesavs

TEST_AESSAVS_OBJ = \
	$(addsuffix .$(OBJX),$(TEST_AESSAVS_SRC))

TEST_AESSAVS_LIB = \
	-lkrypto \
	-lkapp \
	-lkproc \
	-lkfs \
	-lklib \
	-lz \
	-lbz2

$(TEST_BINDIR)/test-aesavs: $(TEST_AESSAVS_OBJ)
	$(LD) --exe -o $@ $^ $(TEST_AESSAVS_LIB)

#-------------------------------------------------------------------------------
# test KEncfile-Write-Read : 
#
TEST_ENCFILE_SRC = \
	test-encfile

TEST_ENCFILE_OBJ = \
	$(addsuffix .$(OBJX),$(TEST_ENCFILE_SRC))

TEST_ENCFILE_LIB = \
	-lkrypto \
	-lkapp \
	-lkproc \
	-lkfs \
	-lklib \
	-lz \
	-lbz2

$(TEST_BINDIR)/test-encfile: $(TEST_ENCFILE_OBJ)
	$(LD) --exe -o $@ $^ $(TEST_ENCFILE_LIB)

#-------------------------------------------------------------------------------
#
#
TEST_BLOCK_CROSS_ERROR_SRC = \
	test-block-cross-error

TEST_BLOCK_CROSS_ERROR_OBJ = \
	$(addsuffix .$(OBJX),$(TEST_BLOCK_CROSS_ERROR_SRC))

TEST_BLOCK_CROSS_ERROR_LIB = \
	-lkrypto \
	-lkapp \
	-lkproc \
	-lvfs \
	-lkfs \
	-lkfg \
	-lklib \
	-lz \
	-lbz2

$(TEST_BINDIR)/test-block-cross-error: $(TEST_BLOCK_CROSS_ERROR_OBJ)
	$(LD) --exe -o $@ $^ $(TEST_BLOCK_CROSS_ERROR_LIB)

#-------------------------------------------------------------------------------
# test KCSPRng : cryptographically secure pseudo-random number generator
#
TEST_CSPRNG_SRC = \
	test-csprng

TEST_CSPRNG_OBJ = \
	$(addsuffix .$(OBJX),$(TEST_CSPRNG_SRC))

TEST_CSPRNG_LIB = \
	-lkrypto \
	-lkapp \
	-lkfs \
	-lklib \
	-lz \
	-lbz2

$(TEST_BINDIR)/test-csprng: $(TEST_CSPRNG_OBJ)
	$(LD) --exe -o $@ $^ $(TEST_CSPRNG_LIB)

#-------------------------------------------------------------------------------
# test test-wga-enc
#
TEST_WGA_ENC_SRC = \
	test-wga-enc

TEST_WGA_ENC_OBJ = \
	$(addsuffix .$(OBJX),$(TEST_WGA_ENC_SRC))

TEST_WGA_ENC_LIB = \
	-lkapp \
	-lkproc \
	-lkrypto \
	-lkfs \
	-lklib \
	-ldl \
	-lz \
	-lbz2

$(TEST_BINDIR)/test-wga-enc: $(TEST_WGA_ENC_OBJ)
	$(LD) --exe -o $@ $^ $(TEST_WGA_ENC_LIB)


#-------------------------------------------------------------------------------
# test test-wga-enc-write
#
TEST_WGA_ENC_WRITE_SRC = \
	test-wga-enc-write

TEST_WGA_ENC_WRITE_OBJ = \
	$(addsuffix .$(OBJX),$(TEST_WGA_ENC_WRITE_SRC))

TEST_WGA_ENC_WRITE_LIB = \
	-lkapp \
	-lkproc \
	-lkrypto \
	-lkfs \
	-lklib \
	-ldl \
	-lz \
	-lbz2

$(TEST_BINDIR)/test-wga-enc-write: $(TEST_WGA_ENC_WRITE_OBJ)
	$(LD) --exe -o $@ $^ $(TEST_WGA_ENC_WRITE_LIB)


#-------------------------------------------------------------------------------
# test-encapptrunc
#
TEST_ENCAPPTRUNC_SRC = \
	test-encapptrunc

TEST_ENCAPPTRUNC_OBJ = \
	$(addsuffix .$(OBJX),$(TEST_ENCAPPTRUNC_SRC))

TEST_ENCAPPTRUNC_LIB = \
	-lkapp \
	-lvfs \
	-lkfg \
	-lkrypto \
	-lkfs \
	-lkproc \
	-lklib \
	-lktst

$(TEST_BINDIR)/test-encapptrunc: $(TEST_ENCAPPTRUNC_OBJ)
	$(LP) --exe -o $@ $^ $(TEST_ENCAPPTRUNC_LIB)

#-------------------------------------------------------------------------------
# testreenc
#
TESTREENC_SRC = \
	testreenc 

TESTREENC_OBJ = \
	$(addsuffix .$(OBJX),$(TESTREENC_SRC))

TESTREENC_LIB = \
	-lkapp \
	-lvfs \
	-lkfg \
	-lkrypto \
	-lkfs \
	-lkproc \
	-lklib \
	-lktst

$(TEST_BINDIR)/testreenc: $(TESTREENC_OBJ)
	$(LP) --exe -o $@ $^ $(TESTREENC_LIB)

#-------------------------------------------------------------------------------
# testencrypt
#
TESTENCRYPT_SRC = \
	testencrypt 

TESTENCRYPT_OBJ = \
	$(addsuffix .$(OBJX),$(TESTENCRYPT_SRC))

TESTENCRYPT_LIB = \
	-lkapp \
	-lvfs \
	-lkfg \
	-lkrypto \
	-lkfs \
	-lkproc \
	-lklib \
	-lktst

$(TEST_BINDIR)/testencrypt: $(TESTENCRYPT_OBJ)
	$(LP) --exe -o $@ $^ $(TESTENCRYPT_LIB)

#-------------------------------------------------------------------------------
# test-encv2
#
TEST_ENCV2_SRC = \
	encv2test 

TEST_ENCV2_OBJ = \
	$(addsuffix .$(OBJX),$(TEST_ENCV2_SRC))

TEST_ENCV2_LIB = \
	-lkapp \
	-lvfs \
	-lkfg \
	-lkrypto \
	-lkfs \
	-lkproc \
	-lklib \
	-lktst

$(TEST_BINDIR)/test-encv2: $(TEST_ENCV2_OBJ)
	$(LP) --exe -o $@ $^ $(TEST_ENCV2_LIB)

