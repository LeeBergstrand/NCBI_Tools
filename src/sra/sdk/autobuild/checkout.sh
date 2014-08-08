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

# checkout script for the autobuild process
# $1 - working directory

export CVSROOT=:pserver:${USER}@cvsvault:/src/NCBI/vault.ncbi

WDIR="$1"
if [ "${WDIR}" == "" ]; then
    WDIR="."
fi

LOG=${WDIR}/autobuild/results/checkout.log

cd ${WDIR}

# imitate an update, search for lines indicating changed files
cvs -qn up -dA 2>/dev/null | grep "^U" >/dev/null 
if [ $? == 0 ] ; then
    echo "$(date) changes found..." >>${LOG}
    # now, update for real
    cvs -q up -dA 1>>${LOG} 2>>${LOG}
    # report the time of the latest checkout, this will kick off the build
    date >${WDIR}/autobuild/last_checkout
fi

