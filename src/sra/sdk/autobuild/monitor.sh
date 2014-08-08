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

# monitoring script for the autobuild process
# $1 - working directory
# $2 - pilot build (e.g. LDD64)
# $3 - build monkey's email address

WDIR="$1"
if [ "${WDIR}" == "" ]; then
    WDIR="."
fi

PILOT="$2"
if [ "${PILOT}" == "" ]; then
    PILOT="L64"
fi

EMAIL="$3"
if [ "${EMAIL}" == "" ]; then
    EMAIL="anatoly.boshkin@nih.gov"
fi

STATE_FILE="${WDIR}/state"
LAST_BUILD="${WDIR}/last_build"
LAST_CHECKOUT="${WDIR}/last_checkout"
RESULTS="${WDIR}/results"
LOG="${RESULTS}/monitor.log"

POST_CHECKOUT_WAIT=5

function set_state
{
    echo "$1" >${STATE_FILE}
    echo "$(date) new state $1" >>${LOG}
}
function email_error
{
    (cat ${WDIR}/stderr_header; tail -n 50 ${RESULTS}/$1.stderr; \
     cat ${WDIR}/stdout_header; tail -n 50 ${RESULTS}/$1.stdout \
    ) | mail -s "$1 $2" $EMAIL
    chmod 664 ${RESULTS}/$1.stderr ${RESULTS}/$1.stdout
}
function time_since_mins
{
    t1=$(date -r $1 +%s)
    now=$(date +%s)
    return $(((${now} - ${t1}) / 60))
}
function check_for_changes
{
    if [ ${LAST_CHECKOUT} -nt ${LAST_BUILD} ]
    then # wait for a while without a checkin
        time_since_mins ${LAST_CHECKOUT}
        DIFF=$?
        if [ ${DIFF} -ge ${POST_CHECKOUT_WAIT} ]; then        
            set_state pilot
        else 
            set_state idle
        fi
    fi
}

if [ ! -e ${STATE_FILE} ]; then # coming up for the first time
    echo "$(date) starting monitor.sh $*" >${LOG}
    pwd >>${LOG}
    env >>${LOG}
    set >>${LOG}
    date >${LAST_BUILD}
    date >${LAST_CHECKOUT}
    mkdir -p ${RESULTS}
    set_state pilot # kick off a build immediately
    exit 0
fi

STATE=$(< ${STATE_FILE})
case "${STATE}" in
idle)  # wait for some time without new checkins, then go to state 'pilot'
    check_for_changes
    ;;
pilot) # a pilot build should start once we update the state file; wait for it to finish
    if [ -e ${RESULTS}/${PILOT}.failed ]; then
        echo "$(date) pilot failed! - emailing stderr" >>${LOG}
        email_error ${PILOT} "build failed"
        mv ${RESULTS}/${PILOT}.failed ${RESULTS}/${PILOT}.failed.last
        set_state idle # no need to do anything until the next checkin
        date >${LAST_BUILD}
    elif [ -e ${RESULTS}/${PILOT}.done ]; then
        echo "$(date) pilot done - kicking off the builds" >>${LOG}
        mv ${RESULTS}/${PILOT}.done ${RESULTS}/${PILOT}.done.last
        set_state running # the pilot has succeeded, kick off other builds
        date >${LAST_BUILD}
    else
        check_for_changes # may go back to idle if a checkout occurs
    fi
    ;;
running) # watch the builds finish, report bad ones
    # build scripts leave a xxx.failed or a xxx.done file in the RESULTS directory before terminating; consume them as they appear
    cd ${RESULTS}
    FAILED=$(ls *.failed 2>/dev/null)
    TEST_FAILED=$(ls *.test_failed 2>/dev/null)
    DONE=$(ls *.done 2>/dev/null)
    cd - >/dev/null
    if [ "${FAILED}" != "" ]; then
        for f in ${FAILED}; do
            echo "$(date) failed build detected: ${f}" >>${LOG}
            email_error ${f%.failed} "build failed"
            mv ${RESULTS}/${f} ${RESULTS}/${f}.last
        done
        date >${LAST_BUILD}
    elif [ "${TEST_FAILED}" != "" ]; then
        for f in ${TEST_FAILED}; do
            echo "$(date) failed test detected: ${f}" >>${LOG}
            email_error ${f%.test_failed} "test failed"
            mv ${RESULTS}/${f} ${RESULTS}/${f}.last
        done
        date >${LAST_BUILD}
    elif [ "${DONE}" != "" ]; then
        for d in ${DONE}; do
            echo "$(date) done build detected: ${d}" >>${LOG}
            mv ${RESULTS}/${d} ${RESULTS}/${d}.last
        done
        date >${LAST_BUILD}
    fi
    # whenever there is a checkout while we are running, go back to idle and start over.
    # the builds that are running at the moment will notice the checkout as well and should go away quitely, without creating xxx.failed or xxx.done
    check_for_changes
    ;;
esac

