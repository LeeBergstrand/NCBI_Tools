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


TOP ?= $(abspath ..)
MODULE = test

include $(TOP)/build/Makefile.shell

#-------------------------------------------------------------------------------
# default
#
SUBDIRS = \
	ktst \
	klib \
	printf \
	krypto-test \
	kfs \
	kfg \
	kapp  \
	krypto \
	kdb \
	pmem \
	align-test \
	vdb \
	re-enc \
	tools \
    vfs \
	bam-load \
    
#fail, possibly outdated:
#	checksum \
#do not build::
#	compression \
#	curl_fetch \
#	dir_table \
#	ext-link \
#	kdf5 \
#	xml \
#shell scripts:
#	build \
#perl scripts:
#	cg-load \
#	dumper-test \
#	integration \
#	sra-stat \
#nothing inside...
#	ncbi \
#code exists, no Makefile:
#	search \
#not tests
#	proxy_exec \
#	proxy_srv \
#component not used anymore
#	sra \


default: $(SUBDIRS)

$(SUBDIRS):
	@ $(MAKE) -C $@

.PHONY: default $(SUBDIRS)

#-------------------------------------------------------------------------------
# all
#
SUBDIRS_ALL = $(addsuffix _all,$(SUBDIRS))

all: $(SUBDIRS_ALL)

$(SUBDIRS_ALL):
	@ $(MAKE) -C $(subst _all,,$@) all

.PHONY: all $(SUBDIRS_ALL)

#-------------------------------------------------------------------------------
# std
#
SUBDIRS_STD = $(addsuffix _std,$(SUBDIRS))

std: $(SUBDIRS_STD)

$(SUBDIRS_STD):
	@ $(MAKE) -C $(subst _std,,$@) std

.PHONY: std $(SUBDIRS_STD)

#-------------------------------------------------------------------------------
# clean
#
SUBDIRS_CLEAN = $(addsuffix _clean,$(SUBDIRS) proxy_exec)

clean: $(SUBDIRS_CLEAN)

$(SUBDIRS_CLEAN):
	@ $(MAKE) -C $(subst _clean,,$@) clean

.PHONY: clean $(SUBDIRS_CLEAN)


#-------------------------------------------------------------------------------
# tag
#
SUBDIRS_TAG = $(addsuffix _tag,$(SUBDIRS))

tag: $(SUBDIRS_TAG)

$(SUBDIRS_TAG):
	@ $(MAKE) -C $(subst _tag,,$@) tag

.PHONY: tag $(SUBDIRS_TAG)

#-------------------------------------------------------------------------------
# runtests
#
SUBDIRS_RUNTESTS = $(addsuffix _runtests,$(SUBDIRS))

runtests: $(SUBDIRS_RUNTESTS)

$(SUBDIRS_RUNTESTS):
	@ $(MAKE) -C $(subst _runtests,,$@) runtests

.PHONY: runtests $(SUBDIRS_RUNTESTS)

#-------------------------------------------------------------------------------
# pass-through
#
out debug profile release GCC ICC VC++:
	@ $(MAKE) TOP=$(TOP) -f $(TOP)/build/Makefile.env $@

.PHONY: out debug profile release GCC ICC VC++
