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

# run a single nencvalid test case
# $1 - nencvalid executable pathname
# $2 - test case id
# $3 - input file(s) (specify multiple files in a space-separated string, e.g. "a.csra b.scra")
# $4 - expected return code
# $5 - additional arguments

NENCVALID="$1"
shift 1
CASE_ID="$1"
shift 1
read -a INPUT <<<"$1"
shift 1
RC="$1"
shift 1
EXTRA_ARGS="$*"

echo ${CASE_ID}
 
CMD="${NENCVALID} ${INPUT[*]} ${EXTRA_ARGS} --no-user-settings" 
$CMD 1>actual/${CASE_ID} 2>/dev/null 
rc=$?
# make sure to remove time stamps
sed -ib 's/^.*T.* nencvalid\.[^:]*: //g' actual/${CASE_ID}
rm actual/${CASE_ID}b
if [ "${rc}" != "${RC}" ]
then
    echo "*** nencvalid returned ${rc} (expected ${RC}). The command issued:"
    echo "${CMD}"
    cat actual/${CASE_ID}.stderr
    exit 1
else
    rm -f actual/${CASE_ID}.stderr    
fi

#echo +++++
#echo diff expected/${CASE_ID} actual/${CASE_ID} 
#echo +++++
diff expected/${CASE_ID} actual/${CASE_ID} 
exit $?
