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

# ===========================================================================
# run a command from a list on a (Windows) build server
# $1 - path to proxy_exec
# $2 - server name 
# $3 - server port
# $4 - server path mapped to a client dir (e.g. Z:)
# $5 - client path to the dir mapped on the server
# $6 - server path to the working directory (e.g. Z:internal\asm-trace)
# $7 - command (can be a single name e.g. cl, or a client path which will be translated into a server path) 
# $... - arguments

#TODO: translate command name to id

PROXY_TOOL=$1
if [ ! -e $PROXY_TOOL ]
then
    echo "$0 $*"
    echo "$0: proxy tool ($1) is not found."
    exit 5
fi

RHOST=$2
RPORT=$3
RHOME=$4
LHOME=$5
WORKDIR=$6
CMD=$7
shift 7
ARGS=$*

if [ "$(dirname $CMD)" != "." ] ; then # executable located on the client; translate the path for the server
    RCMD="$RHOME${CMD#$LHOME}.exe"
    RCMD="$(echo $RCMD | tr '/' '\\')"

    if [ "${WORKDIR}" == "." ] ; then # run in the directory of the executable
        # use the original CMD
        WORKDIR="${CMD%$(basename $CMD)}"
        WORKDIR="$RHOME${WORKDIR#$LHOME}"
    else # translate the workdir
        WORKDIR="$RHOME${WORKDIR#$LHOME}"
    fi
#    echo "WORKDIR=$WORKDIR"    
    WORKDIR="$(echo $WORKDIR | tr '/' '\\')"
#    echo "$WORKDIR"    

else # executable is located on the server
    # translate the workdir
    RCMD=$CMD
    WORKDIR="$RHOME${WORKDIR#$LHOME}"
    WORKDIR="$(echo $WORKDIR | tr '/' '\\')"
fi

#echo "$PROXY_TOOL -D $WORKDIR -S $RHOST -P $RPORT $RCMD $ARGS"
echo "$RCMD $ARGS" | $PROXY_TOOL -D $WORKDIR -S $RHOST -P $RPORT
exit $?




