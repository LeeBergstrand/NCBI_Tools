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

# run a single bam-load test case
# $1 - path to vdb tools
# $2 - samtools executable
# $3 - scratch directory
# $4 - test case id (input test file assumed to be ./$1.sam)
# $5 - expected RC from bam-load (optional, default 0)
# $6 - if "nodb", bam-load is not expected to create a database
# $7 etc. passed to BAM_LOAD verbatim
#
# return codes:
# 0 - passed
# 1 - samtools failed
# 2 - unexpected RC from bam-load 
# 3 - unexpected RC from vdb-dump
# 4 - output of vdb-dump differs from expected

#BAMLOAD=/panfs/pan1/sra-test/TOOLKIT/src/sratoolkit.2.1.16/sratoolkit.2.1.16-centos_linux32/bin/bam-load.2
BAMLOAD=$1/bam-load.3
VDBDUMP=$1/vdb-dump
SAMTOOLS=$2
SCRATCH=$3
CASE_ID=$4
shift 4
RC=$1
if [ "${RC}" == "" ]
then
    RC="0"
    NODB=""
    EXTRA_ARGS=""
else
    shift
    if [ "$1" == "nodb" ]
    then
        NODB="nodb"
        shift 
    fi
    EXTRA_ARGS="$@"
fi
INPUT=${CASE_ID}.sam
OBJECT=${SCRATCH}/${CASE_ID}-obj
CONFIG="./bam-test.cfg"

echo "Running ${CASE_ID}"

$SAMTOOLS view -bS ${CASE_ID}.sam 1>${SCRATCH}/${CASE_ID}-temp.bam 2>/dev/null

rc=$?
if [ "${rc}" != "0" ]
then
    echo "*** samtools returned ${rc}."
    exit 1
fi

LOAD_CMD="${BAMLOAD} -k ${CONFIG} -o ${OBJECT} ${SCRATCH}/${CASE_ID}-temp.bam ${EXTRA_ARGS} 1>actual/${CASE_ID}.stdout 2>actual/${CASE_ID}.stderr"
#echo $LOAD_CMD
eval ${LOAD_CMD}
rc=$?
if [ "${rc}" != "${RC}" ]
then
    echo "*** ${BAMLOAD} returned ${rc} (expected ${RC}). The command issued:"
    echo "${LOAD_CMD}"
    echo "stderr:"
    cat actual/${CASE_ID}.stderr
    exit 2
fi

if [ "${RC}" == "0" ] && [ "${NODB}" == "" ]
then
    DUMP_CMD="${VDBDUMP} ${OBJECT} -T SEQUENCE 1>>actual/${CASE_ID}.stdout 2>>actual/${CASE_ID}.stderr"
#    echo $DUMP_CMD
    eval ${DUMP_CMD}
    rc=$?
    if [ "${rc}" != "0" ]
    then
        echo "*** ${VDBDUMP} returned ${rc} (expected ${RC}). The command issued:"
        echo "${DUMP_CMD}"
        exit 3
    fi

    if [ "$(${VDBDUMP} ${OBJECT} -E | grep PRIMARY_ALIGNMENT)" != "" ]
    then
        echo "================================== PRIMARY_ALIGNMENT ==================================" >>actual/${CASE_ID}.stdout
        DUMP_CMD="${VDBDUMP} ${OBJECT} -T PRIMARY_ALIGNMENT 1>>actual/${CASE_ID}.stdout 2>>actual/${CASE_ID}.stderr"
#        echo $DUMP_CMD
        eval ${DUMP_CMD}
        rc=$?
        if [ "${rc}" != "0" ]
        then
            echo "*** ${VDBDUMP} returned ${rc} (expected ${RC}). The command issued:"
            echo "${DUMP_CMD}"
            exit 3
        fi

        if [ "$(${VDBDUMP} ${OBJECT} -E | grep SECONDARY_ALIGNMENT)" != "" ]
        then
            echo "================================== SECONDARY_ALIGNMENT ==================================" >>actual/${CASE_ID}.stdout
            DUMP_CMD="${VDBDUMP} ${OBJECT} -T SECONDARY_ALIGNMENT 1>>actual/${CASE_ID}.stdout 2>>actual/${CASE_ID}.stderr"
#            echo $DUMP_CMD
            eval ${DUMP_CMD}
            rc=$?
            if [ "${rc}" != "0" ]
            then
                echo "*** ${VDBDUMP} returned ${rc} (expected ${RC}). The command issued:"
                echo "${DUMP_CMD}"
                exit 3
            fi
        fi	
    fi
fi

DIFF_CMD="diff -b expected/${CASE_ID}.stdout actual/${CASE_ID}.stdout"
#echo $DIFF_CMD
eval ${DIFF_CMD}
rc=$?
if [ "${rc}" != "0" ]
then
    echo "*** ${VDBDUMP} output differs from expected. Load command used:"
    echo $LOAD_CMD
    exit 4
fi

#rm -rf ${SCRATCH}/${CASE_ID}* actual/${CASE_ID}*

exit 0



