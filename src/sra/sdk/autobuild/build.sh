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

# build-running script for the autobuild process
# $1 - build id ("ABCDD" where A=OS (L/M/W), B=linkage (D/S), C=build (D/R), DD=bits(64/32)
# $2 - working directory: used to communicate with the build monitor (state, logs, etc)
# $3 - top makefile directory
# $4 - required monitor state (pilot/running, running by default)
# $5 etc - get passed on to the build script

ID="$1"
WDIR="$2"
if [ "${WDIR}" == "" ]; then
    WDIR="."
fi
SRCDIR="$3"
if [ "${SRCDIR}" == "" ]; then
    SRCDIR="${WDIR}/../"
fi
MONITOR_STATE="$4"
if [ "${MONITOR_STATE}" == "" ]; then
    MONITOR_STATE="running"
fi
shift 4
ARGS=$*

STATE_FILE="${WDIR}/state"
LAST_BUILD="${WDIR}/last_build"
LAST_CHECKOUT="${WDIR}/last_checkout"
RESULTS="${WDIR}/results"

if [ ! -e ${WDIR}/${ID}.sh ]; then
    echo "build.sh: ${WDIR}/${ID}.sh not found"
    exit 1
fi
if [ ! -e ${LAST_CHECKOUT} ] ; then
    echo "build.sh: ${LAST_CHECKOUT} not found!"
    exit 2
fi
if [ ! -e ${STATE_FILE} ] ; then
    echo "build.sh: ${STATE_FILE} not found!"
    exit 3
fi
if [ ! -e ${RESULTS} ] ; then
    echo "build.sh: ${RESULTS} not found!"
    exit 4
fi

# the build is triggered when all of these are true
# 1. the state file is older than the checkout, (otherwise need to wait for the monitor to update the state file)
# 2. the monitor is in the required state
# 3. there is no instance with this build id running already
STATE=$(cat ${STATE_FILE})
if test ${STATE_FILE} -nt ${LAST_CHECKOUT} && test "${STATE}" == "${MONITOR_STATE}" && ! test -e ${RESULTS}/${ID}.active ; then
    # find newer of the ID.failed.last and ID.done.last files, compare its timestamp with last_checkout
    if [ -e ${RESULTS}/${ID}.failed.last ] ; then
        if [ -e ${RESULTS}/${ID}.done.last ] ; then
            if [ ${RESULTS}/${ID}.failed.last -nt ${RESULTS}/${ID}.done.last ] ; then
                test ${LAST_CHECKOUT} -nt ${RESULTS}/${ID}.failed.last
            else
                test ${LAST_CHECKOUT} -nt ${RESULTS}/${ID}.done.last
            fi
        else
            test ${LAST_CHECKOUT} -nt ${RESULTS}/${ID}.failed.last
        fi
        RUNMAKE=$?
    elif [ -e ${RESULTS}/${ID}.done.last ] ; then
        test ${LAST_CHECKOUT} -nt ${RESULTS}/${ID}.done.last
        RUNMAKE=$?
    else # no prior build results saved
        if test -e ${RESULTS}/${ID}.failed || test -e ${RESULTS}/${ID}.done ; then
            RUNMAKE=1 # wait for the monitor to process the result of the last build
        else
            RUNMAKE=0 # building for the first time
        fi
    fi
    # build if necessary
    if [ ${RUNMAKE} == 0 ] ; then
        rm -f ${RESULTS}/${ID}.* 2>/dev/null
        source ${WDIR}/${ID}.sh ${SRCDIR} ${ARGS} 1>${RESULTS}/${ID}.stdout 2>${RESULTS}/${ID}.stderr &
        PID=$!
        echo ${PID} >${RESULTS}/${ID}.active
        wait ${PID}
        RC=$?
        # if there has been a checkout while this build was running, discard the results, and rerun on a future invocation
        if test ${LAST_CHECKOUT} -nt ${RESULTS}/${ID}.active ; then
            rm -f ${RESULTS}/${ID}.* # go away quietly, no .done or .failed
        elif [ ${RC} == 0 ] ; then
            date >${RESULTS}/${ID}.done
        elif [ ${RC} == 4 ] ; then
            date >${RESULTS}/${ID}.done
            date >${RESULTS}/${ID}.test_failed
        else
            date >${RESULTS}/${ID}.failed
        fi
        rm -f ${RESULTS}/${ID}.active
    fi
fi

