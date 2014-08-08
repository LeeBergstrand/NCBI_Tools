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

# Auto-build monitor script tests.
#
# to run:
#
# make sure monitor.sh is not running in a cronjob
# cd <dir where monitor.sh is>
# source results/start.sh
# cd tests
# source monitor_test.sh
# watch the output and exit code for results (rc=0: all passed)
#

WDIR=".."

echo "Auto-build monitor script tests."

function wait_for_state
{
    echo "Waiting for the monitor to reach the state '$1'..."
    while : ; do
        STATE=$(cat ${WDIR}/state);
        if [ "${STATE}" == "$1" ]; then
            return
        fi
#        echo "state=${STATE}"
        sleep 1; 
    done
}
function send_to_state
{
    echo $1 >${WDIR}/state
}

rm -f ${WDIR}/results/*.done ${WDIR}/results/*.last ${WDIR}/results/*.failed >/dev/null

wait_for_state idle

###################################
echo "Test 1. The pilot build starts and fails."

touch -d '-20 minutes' ${WDIR}/last_build
wait_for_state pilot
echo "error message" >${WDIR}/results/LDD64.stderr
touch ${WDIR}/results/LDD64.failed 
wait_for_state idle
if [ -e "${WDIR}/results/LDD64.failed.last" ] ; then
    echo "passed"
else
    echo "failed"
    exit 1    
fi    
send_to_state idle

###################################
echo "Test 2. The pilot build succeeds, a regular builds fails."

send_to_state pilot
echo "no error" >${WDIR}/results/LDD64.stderr
touch ${WDIR}/results/LDD64.done
wait_for_state running

echo "error" >${WDIR}/results/build1.stderr
touch ${WDIR}/results/build1.failed
while ! test -e ${WDIR}/results/build1.failed.last
do
    sleep 1
done
if ! test -e ${WDIR}/results/build1.failed ; then
    echo "passed"
else
    echo "failed"
    exit 2
fi   
send_to_state idle

###################################
echo "Test 3. The pilot build succeeds, a regular builds succeeds."

send_to_state running
echo "no error" >${WDIR}/results/build1.stderr
touch ${WDIR}/results/build1.done
while ! test -e ${WDIR}/results/build1.done.last
do
    sleep 1
done
if ! test -e ${WDIR}/results/build1.done ; then
    echo "passed"
else
    echo "failed"
    exit 3
fi   
send_to_state idle

###################################
echo "Test 4. The pilot build succeeds, multiple regular builds succeed."
send_to_state running
rm -f ${WDIR}/results/*.done >/dev/null
echo "no error" >${WDIR}/results/build1.stderr
echo "no error" >${WDIR}/results/build2.stderr
touch ${WDIR}/results/build1.done
touch ${WDIR}/results/build2.done
while ! test -e ${WDIR}/results/build1.done.last || ! test -e ${WDIR}/results/build2.done.last
do
    sleep 1
done
if ! test -e ${WDIR}/results/build1.done || ! test -e ${WDIR}/results/build2.done ; then
    echo "passed"
else
    echo "failed"
    exit 4
fi   
send_to_state idle

