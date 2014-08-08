#!/bin/bash
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
#echo "$0 $*"

# compare outputs of 2 versions of fastq-load
# $1 - path to vdb tools (fastq-load.2, fastq-load.3, vdb-dump)
# $2 - scratch directory (will create as needed; removed on success)
# $3 - source directory (with Run.xml, Experiment.xml, and input files)
# $4, $5, ... - source file(s) and additional options for fastq-load.3 
#
# return codes:
# 0 - passed
# 1 - fastq-load.2 failed 
# 2 - vdb-dump failed on the output of fastq-load.2
# 3 - fastq-load.3 failed 
# 4 - vdb-dump failed on the output of fastq-load.3
# 5 - outputs differ

BINDIR=$1
BINDIR=/panfs/traces01/trace_software/sdk/linux/debug/x86_64/bin
WORKDIR=$2
SOURCEDIR=$3
shift 3
INPUTS=$*

# not checking: READ, QUALITY
# temporarily removed RD_FILTER,
#COMMON_FIELDS="BASE_COUNT,BIO_BASE_COUNT,CMP_BASE_COUNT,COLOR_MATRIX,CS_KEY,CS_NATIVE,MAX_SPOT_ID,MIN_SPOT_ID,PLATFORM,READ_FILTER,READ_LEN,READ_SEG,READ_START,SIGNAL_LEN,SPOT_COUNT,SPOT_GROUP,SPOT_ID,TRIM_START,READ"
#DUMP="$BINDIR/vdb-dump -C $COMMON_FIELDS"
DUMP="$BINDIR/vdb-dump -C READ,QUALITY"
#DUMP="$BINDIR/sam-dump --fastq"

mkdir -p $WORKDIR
rm -rf $WORKDIR/*

echo "running $SOURCEDIR"
cd $SOURCEDIR; 

export LD_LIBRARY_PATH=$BINDIR/../lib; 

#$BINDIR/fastq-load.2.1.18 -r run.xml -e exp.xml -o $WORKDIR/obj.2 1>$WORKDIR/expected.stdout 2>$WORKDIR/expected.stderr || (cat $WORKDIR/expected.stderr && exit 1)

#$BINDIR/fastq-load.3 $INPUTS -o $WORKDIR/obj.3 1>$WORKDIR/actual.stdout 2>$WORKDIR/actual.stderr || exit 3
$BINDIR/fastq-load.2 -r Run.xml -e Experiment.xml -o $WORKDIR/obj.2 1>$WORKDIR/expected.stdout 2>$WORKDIR/expected.stderr || exit 1
    
#$DUMP $WORKDIR/obj.3 1>$WORKDIR/actual.dump.stdout 2>$WORKDIR/actual.dump.stderr || exit 4

#$DUMP $WORKDIR/obj.2 1>$WORKDIR/expected.dump.stdout 2>$WORKDIR/expected.dump.stderr || exit 2

diff $WORKDIR/expected.dump.stdout $WORKDIR/actual.dump.stdout >$WORKDIR/diff || exit 5

#rm -rf $WORKDIR/*

exit 0
