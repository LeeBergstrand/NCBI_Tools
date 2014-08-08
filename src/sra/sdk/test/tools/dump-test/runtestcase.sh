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

# run a dumper against an ecrypted file without a password
# $1 - dumper executable
# $2 - test case id
# $3 - expected return code
# $4 - first expected internal return code in the XML file
# $5 - second expected internal return code in the XML file
# $6 - additional arguments

EXE="$1"
shift 1
CASE_ID="$1"
shift 1
RC="$1"
shift 1
INT_RC_1="$1"
shift 1
INT_RC_2="$1"
shift 1
EXTRA_ARGS="$*"

echo "$CASE_ID looking for '$INT_RC_1' then '$INT_RC_2'"
XML="${HOME}/ncbi_error_report.xml"
 
rm -f actual/${CASE_ID}.stderr ${XML}

CMD="${EXE} encrypted.sra ${EXTRA_ARGS} --no-user-settings " 
$CMD 1>actual/${CASE_ID} 2>actual/${CASE_ID}.stderr
rc=$?
#remove time stamps
sed -ib 's/^....-..-..T..:..:..//g' actual/${CASE_ID}
rm actual/${CASE_ID}b
if [ "${rc}" != "${RC}" ]
then
    echo "${CMD}"
    echo "*** ${EXE} returned ${rc} (expected ${RC})"
    cat actual/${CASE_ID}
    cat actual/${CASE_ID}.stderr
else
    rc=0
    # analyze stdout
    grep "encrypted, but could not be opened" actual/${CASE_ID}
    if [ $? == 0 ]
    then
        echo "${CMD}"
        echo "expected text not found on stdout"
        rc=1
    else # search xml for the expected error code
        grep "Result rc" ${XML} | grep ${INT_RC_1} | grep -q ${INT_RC_2}
        if [ $? != 0 ]
        then
            echo "${CMD}"
            echo "expected text not found in the .xml file"
            rc=2
        fi
    fi

    if [ ${rc} == 0 ]
    then
        diff expected/${CASE_ID} actual/${CASE_ID} 
        rc=$?
    fi
fi

exit ${rc}
